// WaForSpeed — WFS-73 : conducteurs dans les voitures (joueur, trafic, adversaire du duel).
//
// WFS-211 : humains réalistes (assets/people/humains, 12 morphotypes MakeHuman CC0 de WFS-209, 3 niveaux de détail) ; repli :
// personnages du jeu (assets/people/homme.wfp, femme.wfp, Quaternius CC0 : WFS-63, WFS-72). Pose « Sitting_Idle_Loop » figée,
// palette d'os calculée une fois (RG-02), dessinés par drawSkinned. Rien n'est créé : ni personnage, ni maillage.
// Place (RG-01, RG-03, RG-07) : à la place de la vue cockpit (x = −0,36 ; z = −L × 0,02), assis sur l'assise du modèle (WFS-201) ;
// échelle 1 = stature réelle d'ANSUR-II (hommes 1756, femmes 1629 mm, ± 1 écart-type par conducteur) ÷ hauteur du personnage,
// réduite jusqu'à 0,86 × pour garder la tête sous le pavillon mesuré au chargement (mesurerSiege).
// Tourné vers le volant (π ± 0,06), apparence et coiffure stables par voiture (graine).
// Coût (RG-05, RG-06) : les accroches ajoutent les voitures à une file fixe (aucune allocation) ; dessinerFile() dessine, en fin
// d'image (après les piétons : un conducteur ne prend jamais la place d'un piéton proche), les conducteurs à moins de 50 m, du
// plus proche au plus loin ; au-delà de SKIN_MAX_INSTANCES, les plus loin sont omis.
#pragma once
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include "../core/math.h"
#include "../render/renderer.h"   // Model

#include <string>

namespace wfs {
class Game;

namespace conducteurs {

constexpr float DISTANCE_MAX_M = 50.f;   // RG-05
constexpr int FILE_MAX = 24;             // voitures retenues par image (joueur + trafic + duel)
constexpr float X_PLACE = -0.36f;        // place du conducteur (vue cockpit, camera_vue.cpp) : à gauche
constexpr float Z_OEIL_L = -0.02f;       // × longueur de la voiture
constexpr float ECHELLE_MIN = 0.86f;     // RG-03 : voitures très basses
constexpr float GARDE_TOIT_M = 0.035f;   // haut du crâne → pavillon (épaisseur du pavillon et du ciel de toit comprise)

// RG-01 : mesure au chargement du .wfm (model_loader) : hauteur du pavillon au-dessus de la place du conducteur (max des sommets
// dans une colonne de ± 0,12 m en x et ± 0,2 m en z autour de la tête), gardée dans Model::siege (x : y du pavillon, w : 1 si mesuré)

// placement calculé (tests) : à partir de l'œil visé, de l'assise et du pavillon (repère du modèle, m) → échelle et œil final
struct Placement { float echelle = 1, oeilY = 0; };
// oeilRel : hauteur œil − siège du personnage dans sa pose (non mis à l'échelle) ; hautCrane : crâne − œil ; cibleRel : œil − siège visé

inline void mesurerSiege(Model& m, const float* xyz, size_t pas, size_t n) {
    const float cx = (m.bmin.x + m.bmax.x) * 0.5f, cz = (m.bmin.z + m.bmax.z) * 0.5f;
    const float x0 = cx + X_PLACE, z0 = cz + Z_OEIL_L * (m.bmax.z - m.bmin.z);
    float toit = -1e30f; size_t vus = 0;
    for (size_t i = 0; i < n; i++) {
        const float* p = xyz + i * pas;
        if (std::fabs(p[0] - x0) > 0.12f || std::fabs(p[2] - z0) > 0.2f) continue;
        toit = std::max(toit, p[1]); vus++;
    }
    m.siege = vus ? vec4{toit, x0, z0, 1.f} : vec4{m.bmax.y, x0, z0, 0.f};
}

inline Placement placer(float assise, float cibleRel, float oeilRel, float hautCrane, float toitY, bool toitMesure) {
    Placement P;
    const float k0 = cibleRel / std::max(oeilRel, 0.1f);
    float k = k0;
    if (toitMesure) {   // tête sous le pavillon : échelle réduite (jusqu'à 0,86), puis l'œil descend si cela ne suffit pas
        const float place = toitY - GARDE_TOIT_M - assise;
        if (k * (oeilRel + hautCrane) > place) k = std::max(place / (oeilRel + hautCrane), ECHELLE_MIN * k0);
    }
    P.echelle = k;
    P.oeilY = assise + k * oeilRel;
    if (toitMesure) P.oeilY = std::min(P.oeilY, toitY - GARDE_TOIT_M - k * hautCrane);
    return P;
}

// ---------------------------------------------------------------- WFS-211 : humains réalistes (MakeHuman, WFS-209)
// RG-02 : modèle tiré parmi les 12 morphotypes (assets/people/humains/<id>.wfp), stable par voiture ; repli homme / femme du jeu
constexpr int NB_HUMAINS = 12;
constexpr const char* HUMAINS[NB_HUMAINS] = {"h01", "h02", "h03", "h04", "h05", "h06", "f01", "f02", "f03", "f04", "f05", "f06"};
inline bool humaineFemme(int i) { return i >= 6; }
// origines (africaine, asiatique, européenne) des morphotypes : MORPHOTYPES de tools/convert_people_makehuman.py
constexpr float ORIGINES[NB_HUMAINS][3] = {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0.6f, 0, 0.4f}, {0, 0.5f, 0.5f},
                                           {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0.3f, 0.3f, 0.4f}, {0.5f, 0, 0.5f}};
// RG-03 (M-03) : albédo de la peau (sRGB linéaire), dans 0,25-0,65 (luminance, DA-realisme.md § 3) selon le teint :
//   européen : peau I (references/reel/materiaux.json, mat.peau_i.albedo 0,847 0,638 0,552 ; luminance 0,676) ramenée à 0,62 ;
//   asiatique : peau III (mat.peau_iii.albedo 0,623 0,433 0,343 ; luminance 0,467) ;
//   africain : moitié teinte III, moitié teinte VI (mat.peau_vi.albedo), luminance 0,27 (la peau VI publiée, 0,05, est sous le
//   plancher de la charte : le plancher fait foi, RG-03) ; mélange linéaire selon les origines du morphotype
inline vec3 albedoPeau(int i) {
    const vec3 afr{0.09f, 0.05f, 0.02f}, asi{0.623f, 0.433f, 0.343f}, eur{0.777f, 0.585f, 0.506f};   // WFS-210 (RG-02) : afr = peau VI mesurée (mat.peau_vi.albedo, references/reel/humains/peau.md)
    const float* o = ORIGINES[std::clamp(i, 0, NB_HUMAINS - 1)];
    return afr * o[0] + asi * o[1] + eur * o[2];
}
inline float luminance(const vec3& c) { return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z; }

// RG-01 : niveau de détail selon la distance à la caméra (corps-0 < 8 m ≤ corps-1 < 20 m ≤ corps-2) ; vue Proche (≈ 6 m) : corps-0.
// Hystérésis de ± 0,5 m autour de chaque seuil (precedent : niveau de l'image d'avant, −1 : aucun) : une voiture qui roule
// autour d'un seuil ne change pas de niveau à chaque image (avis expert-realisme-3d)
constexpr float LOD_M[2] = {8.f, 20.f}, LOD_HYST_M = 0.5f;
// WFS-317 (RG-02) : en Basse, 3e seuil des piétons de WFS-299 : corps-3 « très simple » (regroupement des sommets, squelette.cpp)
// de 30 m à la fin de la file (50 m) ; Moyenne et Haute inchangées (corps-2 jusqu'à 50 m)
constexpr float LOD_BASSE_M[3] = {8.f, 20.f, 30.f};
inline int niveauDetail(float d2, int precedent = -1, bool basse = false) {
    const float d = std::sqrt(d2);
    const float* s = basse ? LOD_BASSE_M : LOD_M;
    const int nS = basse ? 3 : 2;
    int n = 0;
    while (n < nS && d >= s[n]) n++;
    if (precedent >= 0 && precedent <= nS && n != precedent) {   // on ne quitte le niveau précédent qu'une fois 0,5 m passé au-delà de son seuil
        const float seuil = s[std::min(n, precedent)];
        if (std::fabs(d - seuil) < LOD_HYST_M && std::abs(n - precedent) == 1) n = precedent;
    }
    return n;
}

// RG-02 : attribution stable des modèles aux voitures de la file (aucune allocation) : modèle préféré tiré de la graine ;
// s'il est déjà porté par une voiture présente, le suivant libre (12 voitures distinctes au plus) ; une voiture garde son
// modèle tant qu'elle reste dans la file (pas de changement à vue quand une autre arrive)
struct Attribution {
    uint32_t graine[FILE_MAX] = {}; int8_t modele[FILE_MAX] = {}, lod[FILE_MAX] = {}; bool vu[FILE_MAX] = {}; int n = 0;
    static int prefere(uint32_t g) {   // finaliseur de MurmurHash3 : graines consécutives (trafic) bien mélangées
        g ^= g >> 16; g *= 0x85ebca6bu; g ^= g >> 13; g *= 0xc2b2ae35u; g ^= g >> 16;
        return (int)(g % NB_HUMAINS);
    }
    int choisir(uint32_t g) {
        for (int i = 0; i < n; i++) if (graine[i] == g) { vu[i] = true; return modele[i]; }
        int m = prefere(g);
        for (int k = 0; k < NB_HUMAINS; k++) {
            const int c = (prefere(g) + k) % NB_HUMAINS;
            bool pris = false;
            for (int i = 0; i < n && !pris; i++) pris = modele[i] == c;
            if (!pris) { m = c; break; }
        }
        if (n < FILE_MAX) { graine[n] = g; modele[n] = (int8_t)m; lod[n] = -1; vu[n] = true; n++; }
        return m;
    }
    int niveau(uint32_t g, float d2, bool basse = false) {   // niveau de détail avec hystérésis, gardé par voiture
        for (int i = 0; i < n; i++) if (graine[i] == g) return lod[i] = (int8_t)niveauDetail(d2, lod[i], basse);
        return niveauDetail(d2, -1, basse);
    }
    void finImage() {   // oublie les voitures sorties de la file
        int k = 0;
        for (int i = 0; i < n; i++) if (vu[i]) { graine[k] = graine[i]; modele[k] = modele[i]; lod[k] = lod[i]; vu[k] = false; k++; }
        n = k;
    }
};

bool preparer(Renderer& r);      // charge homme / femme et la pose assise (Game::init)
void liberer(Renderer& r);       // Game::shutdown
// accroche : une voiture de la scène (monde = matrice de dessin du modèle) ; carId : intérieur de la vue cockpit (assise)
void ajouter(const Model& m, const mat4& monde, const char* carId, uint32_t graine);
void dessinerFile(Renderer& r, const vec3& camera);   // fin d'image ; vide la file
int dessinesDerniereImage();
void benchConducteurs(Game& g, Renderer& r, const std::string& out);   // banc « conducteurs » (captures conducteurs-*)

} // namespace conducteurs
} // namespace wfs
