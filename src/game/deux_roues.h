// WaForSpeed — WFS-232 (RG-02) : pilotes casqués des deux-roues du trafic (scooter, routière, sportive : traffic.h DEUX_ROUES).
// Un humain réaliste du jeu (assets/people/humains, lecture seule, WFS-209) en pose « Sitting_Idle_Loop » figée, mains au guidon
// et pieds sur les repose-pieds par l'IK à 2 os de WFS-233 (squelette.h atteindre), casque intégral fermé (≈ 30 cm de haut) posé
// sur la tête. Les 2 × 3 poses (homme, femme × 3 modèles) sont calculées une fois au chargement (aucune allocation en course,
// rien de créé en course : WFS-268) ; les pilotes sont dessinés du plus proche au plus loin, à moins de DISTANCE_MAX_M, au plus
// PILOTES_MAX par image (budget des instances à squelette, partagé avec les piétons et les conducteurs).
#pragma once
#include <cstdint>
#include "../core/math.h"

namespace wfs {
class Renderer;
namespace deux_roues {

// gabarit d'un modèle (repère du .wfm : mètres, avant vers −Z, sol en y = 0) mesuré sur la géométrie convertie (journal WFS-232) :
// selle (point où s'assoit le pilote, sommet de la selle au centre), bout des poignées (sous les rétroviseurs), repose-pieds
// (plancher du scooter) ; y_fiche : hauteur de selle de la fiche (references/reel/deux_roues.json), pour mémoire
struct Gabarit { float selleY, selleZ, poigneeX, poigneeY, poigneeZ, piedX, piedY, piedZ; };
inline constexpr Gabarit GABARITS[3] = {
    {0.81f, 0.22f, 0.33f, 1.06f, -0.29f, 0.17f, 0.30f, -0.22f},   // scooter : pieds à plat sur le plancher, devant la selle
    {0.88f, 0.25f, 0.36f, 1.12f, -0.29f, 0.21f, 0.36f, 0.02f},    // routière : guidon haut, repose-pieds sous la selle
    {0.84f, 0.28f, 0.31f, 0.87f, -0.45f, 0.18f, 0.42f, 0.18f},    // sportive : bracelets bas et avancés, commandes reculées
};
constexpr float DISTANCE_MAX_M = 60.f;
constexpr int PILOTES_MAX = 4;
// casque intégral (DA-realisme § 3 : ≈ 30 cm de haut) : demi-axes de l'ellipsoïde (m), centre au-dessus de l'articulation de la tête.
// Mesures des maillages h02 / f02 (sommets de la tête au repos) : haut du crâne 0,160 / 0,167 m au-dessus de l'articulation,
// arrière du crâne 0,102 / 0,101 m derrière, nez 0,133 / 0,123 m devant, demi-largeur 0,089 / 0,086 m. Centre à +0,048 m
// (haut de la coque ≈ 3,5 cm au-dessus du crâne : coque + calotte) et +0,015 m vers l'avant (≈ 5 cm d'épaisseur derrière la
// tête et devant le nez, mentonnière comprise), 4 cm de chaque côté
constexpr float CASQUE_RX = 0.130f, CASQUE_RY = 0.150f, CASQUE_RZ = 0.165f, CASQUE_HAUT = 0.048f, CASQUE_AVANT = 0.015f;

bool preparer(Renderer& r);                                 // humains, poses, casques (une fois) ; faux : pas de pilote (journal)
void detruire(Renderer& r);
void noter(const mat4& monde, int modele, uint32_t graine);  // deux-roues dessiné (monde : matrice du .wfm, inclinaison comprise)
void dessinerFile(Renderer& r, const vec3& cam);            // pilotes de la file, puis file vidée
int dessinesDerniereImage();                                 // banc d'essai, tests

} // namespace deux_roues
} // namespace wfs
