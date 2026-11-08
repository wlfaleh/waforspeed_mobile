// WaForSpeed — WFS-232 (RG-02) : pilotes casqués des deux-roues du trafic (scooter, routière, sportive : traffic.h DEUX_ROUES).
// Un humain réaliste du jeu (assets/people/humains, lecture seule, WFS-209) en pose « Sitting_Idle_Loop » figée, mains au guidon
// et pieds sur les repose-pieds par l'IK à 2 os de WFS-233 (squelette.h atteindre), casque intégral fermé (≈ 30 cm de haut) posé
// sur la tête. Les poses (homme, femme × 3 modèles, et couché au sol après un accident) sont calculées une fois au chargement
// (aucune allocation en course, rien de créé en course : WFS-268). RG-02 (retour du lead dev) : un pilote sur CHAQUE deux-roues
// dessiné, à toute distance (niveau de détail du corps selon la distance) ; le trafic n'en dessine pas plus de 14 par image.
#pragma once
#include <cstdint>
#include "../core/math.h"

namespace wfs {
class Renderer;
namespace deux_roues {

// gabarit d'un modèle (repère du .wfm : mètres, avant vers −Z, sol en y = 0) mesuré sur la géométrie convertie (journal WFS-232) :
// selle (point le plus bas du dessus de la selle à la place du pilote, + 1,5 cm de coussin), milieu des poignées, repose-pieds
// (plancher du scooter) ; penche : buste penché en avant (rad) selon la position de conduite
struct Gabarit { float selleY, selleZ, poigneeX, poigneeY, poigneeZ, piedX, piedY, piedZ, penche; };
inline constexpr Gabarit GABARITS[3] = {
    {0.79f, 0.20f, 0.33f, 1.06f, -0.29f, 0.17f, 0.30f, -0.22f, 0.00f},    // scooter : buste droit, pieds à plat sur le plancher
    {0.855f, 0.26f, 0.31f, 1.13f, -0.21f, 0.25f, 0.33f, 0.25f, 0.12f},   // routière (trail) : guidon large et haut, buste presque droit
    {0.835f, 0.28f, 0.31f, 0.87f, -0.45f, 0.18f, 0.42f, 0.18f, 0.55f},   // sportive : bracelets bas et avancés, buste penché (≈ 30°)
};
constexpr int PILOTES_MAX = 16;   // ≥ véhicules du trafic dessinés par image (14) : tous les deux-roues dessinés ont leur pilote
constexpr float LOD_PILOTE_M[2] = {8.f, 20.f};   // corps détaillé jusqu'à 8 m, moyen jusqu'à 20 m, léger au-delà
// casque intégral (DA-realisme § 3 : ≈ 30 cm de haut) : demi-axes de l'ellipsoïde (m), centre au-dessus de l'articulation de la tête.
// Mesures des maillages h02 / f02 (sommets de la tête au repos) : haut du crâne 0,160 / 0,167 m au-dessus de l'articulation,
// arrière du crâne 0,102 / 0,101 m derrière, nez 0,133 / 0,123 m devant, demi-largeur 0,089 / 0,086 m. Centre à +0,048 m
// (haut de la coque ≈ 3,5 cm au-dessus du crâne : coque + calotte) et +0,015 m vers l'avant (≈ 5 cm d'épaisseur derrière la
// tête et devant le nez, mentonnière comprise), 4 cm de chaque côté
constexpr float CASQUE_RX = 0.130f, CASQUE_RY = 0.150f, CASQUE_RZ = 0.165f, CASQUE_HAUT = 0.048f, CASQUE_AVANT = 0.015f;

bool preparer(Renderer& r);                                 // humains, poses, casques (une fois) ; faux : pas de pilote (journal)
void detruire(Renderer& r);
// deux-roues dessiné : monde = matrice du .wfm (inclinaison comprise) ; ejecte : accident, pilote couché au sol à côté (sol :
// repère de la chaussée sous le deux-roues, sans l'inclinaison ni la toupie de l'accident ; sens : côté de la chute, ±1)
void noter(const mat4& monde, int modele, uint32_t graine, bool ejecte = false, const mat4& sol = mat4::identity(), int sens = 1);
void dessinerFile(Renderer& r, const vec3& cam);            // pilotes de la file, puis file vidée
int dessinesDerniereImage();                                 // pilotes dessinés à la dernière image (banc d'essai)
int notesDerniereImage();                                    // deux-roues notés à la dernière image (banc d'essai : égal au précédent)
bool pret();                                                 // humains et casques chargés

} // namespace deux_roues
} // namespace wfs
