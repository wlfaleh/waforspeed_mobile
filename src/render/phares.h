// WaForSpeed — feux de croisement du joueur (WFS-218, épic E4) : deux phares réels, coupure du faisceau.
// Répartition d'intensité d'un feu de croisement (règlements ONU n° 112 et n° 48, classe B, circulation à droite) :
//   - coupure horizontale à −0,57° (pente de 1 %, réglage d'usine R48 § 6.2.6) ; côté droit (bas-côté), la coupure remonte
//     à 15° depuis le coude (axe V-V) jusqu'à l'« épaule » (+1°, à confirmer : R112 ne fixe que des points de mesure) ;
//   - au-dessus de la coupure : 1,5 % de la crête (point B50L de R112 : ≤ 0,4 lx à 25 m pour ≈ 12 000 cd au cœur, soit
//     ≈ 250 cd ≈ 2 %) ; transition de ±0,3° (gradient de netteté G ≥ 0,13 par 0,1° de R112, annexe 9) ;
//   - cœur juste sous la coupure (jusqu'à −1,2°), puis décroissance douce vers le bas (premier plan) ; large en azimut
//     (≈ moitié à ±18°, 12 % à 30°, 2,5 % à 45° (WFS-226 : flanc des voitures voisines), nul au-delà de 80°) : bas-côtés et pieds de façade proches restent éclairés (RG-02).
// Intensité réelle : I(el, az) = I0 × intensiteRelative(el, az) en candelas, éclairement E = I cos θ / d² (lumière
// physique, 1/d²) ; le moteur reçoit E / LUX_PAR_UNITE (étalon des lampadaires, feux_modele.h).
// Flux (DA-realisme § 3 et § 4, references/reel/photometrie.json) : flux total des DEUX phares = flux d'un feu de
// croisement sourcé (PB-LUM : halogène 1500 lm, LED 3000 lm), à ±5 % → I0 = flux / 2 / INTEGRALE_SR.
// Le shader (shaders/eclairage.glsl, feuCroisement) reprend exactement ces constantes : toute modification se fait des deux
// côtés (tests/test_phares.cpp vérifie le flux, la coupure et la portée sur ce modèle CPU).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include "../core/math.h"

namespace wfs::phares {

constexpr float COUPURE_DEG = -0.57f;       // RG-01 : coupure à −1 % (−0,57°)
constexpr float RELEVE_PENTE = 0.26795f;     // RG-01 : tan 15°, remontée côté droit
constexpr float EPAULE_DEG = 1.0f;           // RG-01 : haut de la remontée côté droit (à confirmer)
constexpr float TRANSITION_DEG = 0.3f;       // RG-02 : bord doux de la coupure (±0,3°)
constexpr float RESIDU = 0.015f;             // RG-01 : lumière parasite au-dessus de la coupure (B50L)
// WFS-382 (RG-01, ANO-222) : plateau jusqu'à −1,5°, puis chute en 1 / (1 + t^1,6), t = (−el − 1,5) / 0,85 : ≈ 0,29 à −3°, 0,09 à −5°,
// 0,025 à −10°, 0,012 à −15° de la crête (R112 / R149 : 50V ≥ 5 100 cd, 25L / 25R ≥ 1 700 cd, zone I ≤ 2 × 50R ; mesures de
// feux LED de série, avis expert-realisme-3d du 2026-11-10). Avant : 1 / (1 + t²) avec 1,2° et 2,5° : premier plan trop clair
// (≈ 30 fois la chaussée à 30 m, sol brûlé devant la voiture) et crête trop basse (21 000 cd au lieu de 30 000 à 45 000)
constexpr float NOYAU_DEG = 1.5f, CHUTE_DEG = 0.85f, CHUTE_EXPOSANT = 1.6f;
constexpr float LARGEUR_DEG = 18.f, BORD_DEBUT_DEG = 60.f, BORD_FIN_DEG = 80.f;   // étalement en azimut
constexpr float INTEGRALE_SR = 0.05005f;     // ∫ intensiteRelative cos(el) dΩ (sr), vérifiée par tests/test_phares.cpp (WFS-382 : 0,07185 avant)
constexpr float FLUX_HALOGENE_LM = 1500.f, FLUX_LED_LM = 3000.f;   // lum.phare_halogene / lum.phare_led.intensite (typique)
constexpr float PORTEE_M = 200.f;            // fenêtre de calcul (au-delà : < 0,3 lx, effacé par le brouillard)
// teintes (rgb linéaire, luminance 1) : corps noir 3500 K (halogène, 3400-4000 K) et 5700 K (LED, 5000-6000 K),
// CIE 1931 → sRGB linéaire (x, y = 0,4045 ; 0,3915 et 0,3279 ; 0,3375)
inline const vec3 TEINTE_HALOGENE{1.5507f, 0.8964f, 0.4043f}, TEINTE_LED{1.1163f, 0.9757f, 0.8981f};

inline float lisse(float a, float b, float x) { const float t = std::clamp((x - a) / (b - a), 0.f, 1.f); return t * t * (3 - 2 * t); }

// élévation et azimut (degrés, azimut > 0 vers la droite) depuis l'axe du phare ; 1 au cœur du faisceau
inline float intensiteRelative(float el, float az) {
    const float lim = COUPURE_DEG + std::min(std::max(az, 0.f) * RELEVE_PENTE, EPAULE_DEG - COUPURE_DEG);
    const float coupure = 1.f - (1.f - RESIDU) * lisse(lim - TRANSITION_DEG, lim + TRANSITION_DEG, el);
    const float t = (-el - NOYAU_DEG) / CHUTE_DEG, vertical = el > -NOYAU_DEG ? 1.f : 1.f / (1.f + std::pow(t, CHUTE_EXPOSANT));
    const float a = az / LARGEUR_DEG, horizontal = (1.f - lisse(BORD_DEBUT_DEG, BORD_FIN_DEG, std::abs(az))) / (1.f + a * a * a * a);
    return vertical * horizontal * coupure;
}

// RG-00 / graphisme réaliste : LED ou halogène selon la génération du modèle (équipement de série, à confirmer par voiture ;
// la Gallardo a des phares au xénon, absents de la base sourcée : halogène, la teinte sourcée la plus proche du bas)
inline bool led(const char* id) {
    static const char* const HALOGENE[] = {"abarth", "compass", "gt86", "gallardo"};   // WFS-359 : « tcross » = Brezza 2022, projecteurs à DEL
    if (!id) return false;
    for (const char* h : HALOGENE) if (!std::strcmp(id, h)) return false;
    return true;
}
// crête d'un phare (cd) : la moitié du flux de la paire répartie selon intensiteRelative
inline float creteCd(bool estLed) { return (estLed ? FLUX_LED_LM : FLUX_HALOGENE_LM) * 0.5f / INTEGRALE_SR; }

// ---------- WFS-226 : phares des autres voitures (trafic, adversaires, police, camion) ----------
// Chaque voiture dessinée la nuit, sauf le joueur, est notée par Renderer::feuxVoiture (repère de ses optiques). Le
// système de nuit en tire, à l'image suivante, les vraies lumières des plus proches (RG-02 : 2 en Basse, 4 sinon, un
// projecteur par voiture au milieu de ses deux optiques, flux de la paire) ; les autres reçoivent la lumière au sol calculée
// avec la même répartition (eclairementSol), dessinée par le système de nuit à la même image.
struct PhareAutre {
    vec3 centre;        // milieu des deux optiques (monde)
    vec3 avant, droite; // axes de la voiture (normés)
    float demiEcart;    // demi-écart entre les optiques (m)
    float hauteur;      // hauteur des optiques au-dessus de la chaussée (m)
    vec3 sol;           // point de la chaussée sous le centre
};
constexpr int AUTRES_MAX = 64;
constexpr int AUTRES_ECLAIRANTS_BASSE = 2, AUTRES_ECLAIRANTS = 4;   // RG-02
constexpr float AUTRES_PORTEE_SPOT = 70.f;   // voitures à vraie lumière : à moins de 70 m du joueur
inline std::vector<PhareAutre>& autres() { static std::vector<PhareAutre> v; return v; }
inline void noterAutre(const PhareAutre& p) { auto& v = autres(); if ((int)v.size() < AUTRES_MAX) v.push_back(p); }

// ---------- WFS-450 : nuit en Basse, la lumière des lampadaires ne s'éteint plus d'un coup, phares des autres voitures en fondu ----------
// Défaut vu sur le téléphone (S21 FE, Basse) : chaque pixel ne garde que 4 lumières, prises dans l'ordre de la liste, rangée de la
// plus proche à la plus lointaine de la caméra. Deux voitures juste derrière la caméra passaient avant les lampadaires : 2 phares du
// joueur + 2 voitures = 4, les lampadaires n'étaient plus calculés sur la chaussée (noir d'un coup, puis rallumé d'un coup).
// RG-01 : en Basse, phares du joueur, PUIS lampadaires (éclairage fixe), puis phares des autres voitures ; à rang égal, la plus
//         proche de la caméra d'abord (WFS-263). Moyenne et Haute : ordre de la liste, inchangé.
constexpr uint8_t RANG_JOUEUR = 0, RANG_FIXE = 1, RANG_AUTRES = 2;
// banc « lumieres-mouvement » seulement (images « avant » de RG-06, comme Game::interpolation pour le banc « fluidite ») :
// vrai = ordre, quota et apparition des phares d'avant WFS-450
inline bool avant450 = false;
inline bool avantEnBasse(uint8_t rangA, float distCamA, uint8_t rangB, float distCamB) {
    if (avant450 || rangA == rangB) return distCamA < distCamB;
    return rangA < rangB;
}
// RG-03 : les phares d'une autre voiture apparaissent et disparaissent en fondu de FONDU_AUTRES_S (changement de rang), et avec
// la distance au joueur : pleins jusqu'à AUTRES_PORTEE_PLEINE, nuls à AUTRES_PORTEE_SPOT. Une voiture éclairante garde sa place
// tant qu'une autre n'est pas plus proche d'au moins FONDU_HYSTERESIS_M (pas de clignotement entre deux voitures à égale distance).
constexpr float FONDU_AUTRES_S = 0.3f;
constexpr float AUTRES_PORTEE_PLEINE = 55.f;
constexpr float FONDU_HYSTERESIS_M = 6.f;
constexpr float FONDU_SUIVI_M = 8.f;     // même voiture d'une image à la suivante : à moins de 8 m (250 km/h à 10 img/s ≈ 7 m)
constexpr float FONDU_DT_MAX = 0.1f;     // image plus longue (chargement, pause) : le fondu n'avance que de 0,1 s
constexpr int FONDU_PLACES = AUTRES_ECLAIRANTS + 2;   // 4 voitures éclairantes au plus et 2 qui s'éteignent
inline float fonduDistance(float d) { return 1.f - lisse(AUTRES_PORTEE_PLEINE, AUTRES_PORTEE_SPOT, d); }
struct FonduAutres {
    struct Place {
        PhareAutre voiture;      // dernière position connue
        float niveau = 0;        // 0 éteint … 1 plein (linéaire dans le temps)
        bool occupee = false, voulue = false;
        int indice = -1;         // rang dans la liste de cette image (−1 : voiture plus dessinée, elle s'éteint sur place)
    };
    Place places[FONDU_PLACES];
    void vider() { for (Place& p : places) p = Place{}; }
    // intensité à appliquer (0..1) : montée et descente douces (pente nulle aux deux bouts) × fondu de distance
    static float poids(const Place& p, const vec3& joueur) { return lisse(0.f, 1.f, p.niveau) * fonduDistance(length(p.voiture.centre - joueur)); }
    // un pas par image dessinée : v = voitures notées (n ≤ AUTRES_MAX), nMax = voitures éclairantes voulues (2 en Basse), dt en s.
    // Aucune allocation.
    void pas(const PhareAutre* v, int n, const vec3& joueur, int nMax, float dt) {
        n = std::clamp(n, 0, AUTRES_MAX); nMax = std::clamp(nMax, 0, AUTRES_ECLAIRANTS); dt = std::clamp(dt, 0.f, FONDU_DT_MAX);
        int placeDe[AUTRES_MAX]; bool voulu[AUTRES_MAX];
        for (int i = 0; i < n; i++) { placeDe[i] = -1; voulu[i] = false; }
        for (int k = 0; k < FONDU_PLACES; k++) {   // 1. suivi : chaque place retrouve sa voiture (la plus proche de sa dernière position)
            Place& p = places[k];
            if (!p.occupee) continue;
            p.indice = -1; float dMin = FONDU_SUIVI_M;
            for (int i = 0; i < n; i++) if (placeDe[i] < 0) { const float d = length(v[i].centre - p.voiture.centre); if (d < dMin) { dMin = d; p.indice = i; } }
            if (p.indice >= 0) { placeDe[p.indice] = k; p.voiture = v[p.indice]; }
        }
        for (int c = 0; c < nMax; c++) {   // 2. les nMax plus proches du joueur, à moins de 70 m ; une voiture déjà voulue garde un avantage
            int choix = -1; float sMin = 1e9f;
            for (int i = 0; i < n; i++) {
                if (voulu[i]) continue;
                const float d = length(v[i].centre - joueur);
                if (d >= AUTRES_PORTEE_SPOT) continue;
                const float s = d - (placeDe[i] >= 0 && places[placeDe[i]].voulue ? FONDU_HYSTERESIS_M : 0.f);
                if (s < sMin) { sMin = s; choix = i; }
            }
            if (choix < 0) break;
            voulu[choix] = true;
        }
        for (Place& p : places) p.voulue = p.occupee && p.indice >= 0 && voulu[p.indice];   // 3. cibles
        for (int i = 0; i < n; i++) {
            if (!voulu[i] || placeDe[i] >= 0) continue;
            for (int k = 0; k < FONDU_PLACES; k++) if (!places[k].occupee) { places[k] = Place{v[i], 0.f, true, true, i}; placeDe[i] = k; break; }
        }
        const float d = dt / FONDU_AUTRES_S;   // 4. niveaux
        for (Place& p : places) {
            if (!p.occupee) continue;
            p.niveau = std::clamp(p.niveau + (p.voulue ? d : -d), 0.f, 1.f);
            if (!p.voulue && p.niveau <= 0) p = Place{};
        }
    }
};

// WFS-450 (cause trouvée au banc « lumieres-mouvement ») : les 4 lampadaires éclairants étaient pris « de 4 segments derrière le
// joueur à 160 devant » : un lampadaire sortait de la liste 5 m derrière la voiture alors qu'il éclaire 26 m autour de lui : toute
// la chaussée autour de la voiture devenait noire d'un coup (−45 % d'une image à la suivante), puis se rallumait à l'approche du
// suivant. En Basse : un lampadaire reste candidat derrière le joueur tant que sa lumière atteint la chaussée visible
// (LAMPES_DERRIERE_SEG), les 4 retenus sont les plus proches (un lampadaire derrière compte LAMPES_POIDS_DERRIERE fois sa distance :
// la route devant garde la priorité), et chaque entrée ou sortie de la liste se fait en fondu de FONDU_AUTRES_S.
constexpr int LAMPES_ECLAIRANTES = 4;            // nuit.cpp, LAMPES_MAX
constexpr int LAMPES_DERRIERE_SEG = 30;          // 36 m : portée d'un lampadaire (26 m) + recul de la caméra (≈ 10 m)
constexpr int LAMPES_DEVANT_SEG = 160;
constexpr float LAMPES_POIDS_DERRIERE = 3.f;
inline float cleLampe(int64_t ecartSeg) { return ecartSeg >= 0 ? (float)ecartSeg : -(float)ecartSeg * LAMPES_POIDS_DERRIERE; }
constexpr int FONDU_LAMPES_PLACES = LAMPES_ECLAIRANTES + 2;   // 4 éclairants et 2 qui s'éteignent
struct FonduLampes {
    struct Place { int64_t id = -1; float niveau = 0; bool voulue = false; };   // id : indice du lampadaire (−1 : place libre)
    Place places[FONDU_LAMPES_PLACES];
    void vider() { for (Place& p : places) p = Place{}; }
    static float poids(const Place& p) { return lisse(0.f, 1.f, p.niveau); }
    // un pas par image dessinée : voulus = lampadaires retenus cette image (n ≤ 4) ; premier pas (toutes places libres) : pleins tout de suite
    void pas(const int64_t* voulus, int n, float dt) {
        dt = std::clamp(dt, 0.f, FONDU_DT_MAX); n = std::clamp(n, 0, LAMPES_ECLAIRANTES);
        bool vide = true; for (const Place& p : places) vide = vide && p.id < 0;
        for (Place& p : places) p.voulue = false;
        for (int i = 0; i < n; i++) {
            Place* trouve = nullptr; Place* libre = nullptr;
            for (Place& p : places) { if (p.id == voulus[i]) trouve = &p; else if (p.id < 0 && !libre) libre = &p; }
            if (!trouve && libre) { *libre = Place{voulus[i], vide ? 1.f : 0.f, true}; trouve = libre; }
            if (trouve) trouve->voulue = true;
        }
        const float d = dt / FONDU_AUTRES_S;
        for (Place& p : places) {
            if (p.id < 0) continue;
            p.niveau = std::clamp(p.niveau + (p.voulue ? d : -d), 0.f, 1.f);
            if (!p.voulue && p.niveau <= 0) p = Place{};
        }
    }
};

// éclairement (lx) de la chaussée en (x à droite, z devant) dans le repère de la voiture, par ses deux phares
// (paire LED, comme les voitures du trafic : RG-01 « même photométrie »)
inline float eclairementSol(float x, float z, float demiEcart, float h) {
    if (z <= 0.05f) return 0;
    float e = 0;
    for (float c : {-demiEcart, demiEcart}) {
        const float dx = x - c, d2 = dx * dx + z * z + h * h, d = std::sqrt(d2);
        const float el = -std::asin(h / d) * 57.29578f, az = std::atan2(dx, z) * 57.29578f;
        e += creteCd(true) * intensiteRelative(el, az) * (h / d) / d2;
    }
    return e;
}

} // namespace wfs::phares
