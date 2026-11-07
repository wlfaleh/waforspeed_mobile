// WaForSpeed — WFS-73 : conducteurs dans les voitures (joueur, trafic, adversaire du duel).
//
// Personnages du jeu (assets/people/homme.wfp, femme.wfp, Quaternius CC0 : WFS-63, WFS-72), pose « Sitting_Idle_Loop » figée,
// palette d'os calculée une fois (RG-02), dessinés par drawSkinned. Rien n'est créé : ni personnage, ni maillage.
// Place (RG-01, RG-03, RG-07) : la tête du conducteur est à l'œil de la vue cockpit (x = −0,36 ; z = −L × 0,02 ; hauteur des yeux
// assis d'ANSUR-II au-dessus de l'assise du modèle, WFS-201) ; l'échelle donne la hauteur des yeux assis réelle (hommes 804,5,
// femmes 747,7 mm, dossier à 25°), réduite jusqu'à 0,86 pour garder la tête sous le pavillon mesuré au chargement (mesurerSiege).
// Tourné vers le volant (π ± 0,06), apparence et coiffure stables par voiture (graine).
// Coût (RG-05, RG-06) : les accroches ajoutent les voitures à une file fixe (aucune allocation) ; dessinerFile() dessine, en fin
// d'image (après les piétons : un conducteur ne prend jamais la place d'un piéton proche), les conducteurs à moins de 50 m, du
// plus proche au plus loin ; au-delà de SKIN_MAX_INSTANCES, les plus loin sont omis.
#pragma once
#include <cstddef>
#include <cstdint>
#include "../core/math.h"

namespace wfs {
class Renderer;
struct Model;

namespace conducteurs {

constexpr float DISTANCE_MAX_M = 50.f;   // RG-05
constexpr int FILE_MAX = 24;             // voitures retenues par image (joueur + trafic + duel)
constexpr float X_PLACE = -0.36f;        // place du conducteur (vue cockpit, camera_vue.cpp) : à gauche
constexpr float Z_OEIL_L = -0.02f;       // × longueur de la voiture
constexpr float ECHELLE_MIN = 0.86f;     // RG-03 : voitures très basses
constexpr float GARDE_TOIT_M = 0.035f;   // haut du crâne → pavillon (épaisseur du pavillon et du ciel de toit comprise)

// RG-01 : mesure au chargement du .wfm (model_loader) : hauteur du pavillon au-dessus de la place du conducteur (max des sommets
// dans une colonne de ± 0,12 m en x et ± 0,2 m en z autour de la tête), gardée dans Model::siege (x : y du pavillon, w : 1 si mesuré)
void mesurerSiege(Model& m, const float* xyz, size_t pas, size_t n);

// placement calculé (tests) : à partir de l'œil visé, de l'assise et du pavillon (repère du modèle, m) → échelle et œil final
struct Placement { float echelle = 1, oeilY = 0; };
// oeilRel : hauteur œil − assise du personnage dans sa pose (à l'échelle 1) ; hautCrane : crâne − œil ; cibleRel : œil − assise réelle
Placement placer(float assise, float cibleRel, float oeilRel, float hautCrane, float toitY, bool toitMesure);

bool preparer(Renderer& r);      // charge homme / femme et la pose assise (Game::init)
void liberer(Renderer& r);       // Game::shutdown
// accroche : une voiture de la scène (monde = matrice de dessin du modèle) ; carId : intérieur de la vue cockpit (assise)
void ajouter(const Model& m, const mat4& monde, const char* carId, uint32_t graine);
void dessinerFile(Renderer& r, const vec3& camera);   // fin d'image ; vide la file
int dessinesDerniereImage();

} // namespace conducteurs
} // namespace wfs
