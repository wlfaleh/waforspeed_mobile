// WaForSpeed — WFS-73 : conducteurs dans les voitures (joueur, trafic, adversaire du duel).
//
// Personnages du jeu (assets/people/homme.wfp, femme.wfp, Quaternius CC0 : WFS-63, WFS-72), pose « Sitting_Idle_Loop » figée,
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

bool preparer(Renderer& r);      // charge homme / femme et la pose assise (Game::init)
void liberer(Renderer& r);       // Game::shutdown
// accroche : une voiture de la scène (monde = matrice de dessin du modèle) ; carId : intérieur de la vue cockpit (assise)
void ajouter(const Model& m, const mat4& monde, const char* carId, uint32_t graine);
void dessinerFile(Renderer& r, const vec3& camera);   // fin d'image ; vide la file
int dessinesDerniereImage();
void benchConducteurs(Game& g, Renderer& r, const std::string& out);   // banc « conducteurs » (captures conducteurs-*)

} // namespace conducteurs
} // namespace wfs
