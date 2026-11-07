// WaForSpeed — WFS-257 (ANO-130) : apparition naturelle des personnes (épic E11). Règles pures, sans rendu, testées par
// test_apparition_pietons :
//   - RG-02 : une personne ne prend ou ne quitte sa place à l'écran que hors de la vue (derrière la caméra, hors du cadre) ;
//     au loin (bord de la portée, ≈ 138 m) elle entre et sort en fondu ; une personne affichée dans la vue garde sa place ;
//   - RG-03 : les changements d'opacité et de niveau de détail se font en fondu d'au moins 0,5 s (tramage écran 4 × 4 du
//     fondu des voitures de WFS-134, scene_skin.frag) ;
//   - RG-04 : le niveau de détail ne bascule pas d'une image à l'autre (hystérésis de 1,5 m).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace wfs::apparition {

constexpr float FONDU_S = 0.5f;          // RG-03 : durée minimale d'un fondu (s)
constexpr float HYSTERESE_M = 1.5f;      // RG-04 : marge autour des seuils de détail (m)
constexpr float BANDE_LOIN_M = 20.f;     // fondu de distance au bord de la portée (m)

// état d'affichage d'une personne d'une image à l'autre
enum class Lieu : uint8_t { Absente, HorsCadre, Affichee };   // hors portée ; dans la portée mais hors du cadre ; dessinée
struct Etat {
    float op = 0;          // opacité affichée (0 : invisible, 1 : pleine)
    int lod = -1;          // niveau de détail affiché (−1 : aucun encore)
    int lodPrec = -1;      // niveau précédent pendant un fondu croisé (−1 : aucun)
    float lodT = 1;        // avancement du fondu croisé (0 → 1)
    Lieu lieu = Lieu::Absente;
};

// opacité permise par la distance le long de la route : 1 jusqu'à fin − bande, 0 à fin (bord de la portée)
inline float opaciteDistance(double distM, double finM) {
    return (float)std::clamp((finM - distM) / BANDE_LOIN_M, 0.0, 1.0);
}

// RG-02 / RG-03 : opacité de l'image. cadre : dans le cadre de la caméra ; place : une place de dessin lui est donnée ;
// opMax : opacité de distance. Hors du cadre : l'état suit tout de suite (personne ne la voit). Dans le cadre : une
// personne qui arrive de hors du cadre entre telle quelle (elle passe le bord de l'image) ; sinon l'opacité ne varie pas
// plus vite qu'un fondu de FONDU_S.
inline void avancerOpacite(Etat& e, bool dansPortee, bool cadre, bool place, float opMax, double dt) {
    if (!dansPortee) { e.op = 0; e.lieu = Lieu::Absente; e.lod = -1; e.lodPrec = -1; e.lodT = 1; return; }
    if (!cadre) { e.op = opMax; e.lieu = Lieu::HorsCadre; e.lodPrec = -1; e.lodT = 1; return; }
    const float cible = place ? opMax : 0.f;
    if (e.lieu == Lieu::HorsCadre) e.op = cible;   // elle entre dans l'image par le bord (sans place : pas encore là)
    else {
        const float pas = (float)std::max(0.0, dt) / FONDU_S;
        e.op = cible > e.op ? std::min(cible, e.op + pas) : std::max(cible, e.op - pas);
    }
    e.lieu = Lieu::Affichee;
}

// niveau voulu pour la distance d (m), seuils {complet, moyen}, avec hystérésis autour du niveau affiché
inline int niveauVoulu(float d, const float seuils[2], int actuel) {
    int n = d < seuils[0] ? 0 : d < seuils[1] ? 1 : 2;
    if (actuel >= 0 && n != actuel) {
        const int bas = std::min(n, actuel);   // frontière franchie entre bas et bas + 1 (un seul niveau d'écart compté)
        if (std::abs(n - actuel) == 1 && std::fabs(d - seuils[bas]) < HYSTERESE_M) n = actuel;
    }
    return n;
}

// RG-03 / RG-04 : niveau de détail. Hors du cadre ou invisible : changement immédiat (personne ne le voit). Dans le
// cadre, pleinement visible : fondu croisé de FONDU_S entre l'ancien et le nouveau niveau ; pendant un fondu (opacité
// ou détail), le changement attend la fin.
inline void avancerNiveau(Etat& e, float distM, const float seuils[2], double dt) {
    if (e.lodPrec >= 0) {
        e.lodT = std::min(1.f, e.lodT + (float)std::max(0.0, dt) / FONDU_S);
        if (e.lodT >= 1.f) e.lodPrec = -1;
    }
    const int voulu = niveauVoulu(distM, seuils, e.lod);
    if (voulu == e.lod) return;
    if (e.lod < 0 || e.lieu != Lieu::Affichee || e.op <= 0.f) { e.lod = voulu; e.lodPrec = -1; e.lodT = 1; return; }
    if (e.op < 1.f || e.lodPrec >= 0) return;   // un fondu à la fois
    e.lodPrec = e.lod; e.lod = voulu; e.lodT = 0;
}

// valeur de tramage pour le shader (pc.emissive.w, voir fonduLod de vehicule.glsl) : 0 = sans fondu ;
// −(1 + t) : pixels gardés là où le seuil < t ; −(3 + t) : pixels gardés là où le seuil ≥ t (complément)
inline float tramage(float garde) { return garde >= 1.f ? 0.f : -(1.f + std::max(0.f, garde)); }
inline float tramageComplement(float t) { return t <= 0.f ? 0.f : -(3.f + std::min(1.f, t)); }

// places de dessin (RG-02) : parmi les personnes dans le cadre, celles déjà affichées gardent toujours leur place (aucune
// ne disparaît sous les yeux du joueur), puis les autres par distance tant que le budget le permet. cout : instances que
// coûte la personne ; budget : instances de l'image (les affichées comptent, mais passent même au-delà).
// Une famille (lien ≥ 0 : indice de l'autre membre dans la même liste) entre ou sort ensemble.
struct Candidat { int i = -1; int lien = -1; float dist = 0; bool affichee = false; int cout = 2; };
inline void choisirPlaces(std::vector<Candidat>& c, std::vector<int>& ordre, int budget, std::vector<char>& place) {
    ordre.resize(c.size());
    for (size_t k = 0; k < c.size(); k++) ordre[k] = (int)k;
    std::sort(ordre.begin(), ordre.end(), [&](int a, int b) {
        if (c[(size_t)a].affichee != c[(size_t)b].affichee) return c[(size_t)a].affichee;
        if (c[(size_t)a].dist != c[(size_t)b].dist) return c[(size_t)a].dist < c[(size_t)b].dist;
        return c[(size_t)a].i < c[(size_t)b].i;
    });
    place.assign(c.size(), 0);
    int utilise = 0;
    for (int k : ordre) {
        if (place[(size_t)k]) continue;
        const int l = c[(size_t)k].lien;
        const int besoin = c[(size_t)k].cout + (l >= 0 && !place[(size_t)l] ? c[(size_t)l].cout : 0);
        if (!c[(size_t)k].affichee && utilise + besoin > budget) continue;
        place[(size_t)k] = 1; utilise += besoin;
        if (l >= 0) place[(size_t)l] = 1;
    }
}

} // namespace wfs::apparition
