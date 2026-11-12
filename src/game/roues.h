// WaForSpeed — roues animées (WFS-60, absorbe WFS-61 et WFS-62) : roues séparées dans les voitures, roulage et
// braquage. Port de js/cars/models.js du jeu web : splitWheels (découpe par coin LF / RF / LB / RB, médiane autour du
// centre de la roue), looksLikeWheel, rigWheels (centre et rayon de chaque roue) et animWheels (roulage = distance /
// rayon, braquage des roues avant avec épure d'Ackermann 1,12 / 0,9) ; js/world/scene.js (angle des roues du joueur
// et du trafic). Calcul seul (aucun appel Vulkan) : testé par tests/test_roues.cpp.
#pragma once
#include <cstdint>
#include <vector>
#include "../core/math.h"
#include "track.h"   // WFS-452 : courbure de la route (Track::findSegment, en ligne)

namespace wfs {

// roue d'un modèle de voiture : centre (m, repère du modèle, avant vers −Z) et rayon
struct RoueModele { vec3 centre{}; float rayon = 0; bool avant = false; };
// pose des roues pour une image : roulage (rad, cumulé) et braquage des roues avant (rad, + = à droite)
struct RouesPose { float roulage = 0, braquage = 0; };

namespace roues {

constexpr float RW_DEFAUT = 0.34f;      // rayon de roue de référence (car.d.rw des voitures du catalogue)
enum Coin { LF = 0, RF = 1, LB = 2, RB = 3 };

// un maillage du modèle (un par matériau dans les .wfm) ; sommets : x, y, z au début de chaque sommet
struct Maillage {
    const float* sommets = nullptr; uint32_t stride = 8; uint32_t nSommets = 0;
    const uint32_t* idx = nullptr; uint32_t nIdx = 0;
    bool nomme = false;                  // matériau marqué « roue » par la conversion (drapeau 128)
};
struct Decoupe {
    RoueModele roues[4]; int n = 0;      // 4 roues trouvées, ou 0 (voiture inchangée)
    std::vector<std::vector<int8_t>> roueDuTriangle;   // par maillage, par triangle : coin 0-3, ou −1 (fixe)
};
// splitWheels + rigWheels ; puis, dans les matériaux non découpés (jantes fusionnées avec le chrome de la carrosserie,
// disques), les triangles entièrement dans le cylindre d'une roue, s'ils en font le tour (pas les étriers)
Decoupe decouper(const std::vector<Maillage>& m, float rw = RW_DEFAUT);

// matrice d'une roue : T(c) · Ry(−braquage × Ackermann) · Rx(roulage) · T(−c) (animWheels, groupes braquage puis rotation)
mat4 matrice(const RoueModele& r, const RouesPose& p);
// coefficient d'Ackermann : roue intérieure au virage 1,12, extérieure 0,9 (roues avant seulement)
float ackermann(const RoueModele& r, float braquage);

// angles du jeu web (scene.js) : joueur : wa = steer × 0,6 / (1 + |v| / 14), braquage = clamp(wa × 1,6, ±0,6) ;
// roulage = −distance / rayon (distance parcourue en m, signée)
RouesPose poseJoueur(double steer, double speedUnits, double distUnits, float rayon);
// trafic : braquage = cap de changement de file × 1,6 (0 après un accident), roulage selon la position sur la piste
RouesPose poseTrafic(double laneYaw, bool crashed, double zUnits, float rayon);

// WFS-452 : braquage des voitures conduites par le jeu (trafic, adversaires de la Course à 5, duel, police, fantôme), calculé
// à chaque pas fixe à partir du VRAI mouvement de la voiture : roues avant = atan(empattement × courbure de la trajectoire)
// (épure de la bicyclette), plus l'écart entre la direction où va la voiture et celle de sa caisse (contre-braquage d'une
// voiture qui glisse de côté, police comprise) ; borné à ±35° (vraie butée), lissé (deux filtres du premier ordre, puis
// vitesse de braquage bornée) : aucun saut d'une image à l'autre, 0 exactement en ligne droite sans changement de file.
// À l'arrêt, les roues gardent leur angle (comme une vraie voiture). Avant (WFS-60) : braquage du trafic = cap de file × 1,6,
// jusqu'à 2,5 rad (voiture qui traverse, voiture lente qui change de file) et en sauts (ANO-265) ; aucun braquage ailleurs.
inline bool g_avant452 = false;
inline bool g_journal452 = false;   // banc « roues-suivies » : roulage et braquage de la voiture suivie écrits au journal à chaque image   // banc « roues-suivies » seulement : vrai = rendu d'avant WFS-452 (captures « avant »)
constexpr float BRAQUAGE_MAX = 0.61f;          // rad aux roues (≈ 35°, butée d'une voiture de série)
constexpr float BRAQUAGE_VITESSE = 1.5f;       // rad/s aux roues, au plus (coup de volant rapide d'un conducteur)
constexpr double BRAQUAGE_TAU = 0.10;          // s, constante de chacun des deux filtres
struct SuiviRoues {
    float braquage = 0, braquagePrec = 0;      // angle après ce pas et avant (rendu interpolé, GameView::alpha)
    double courbF1 = 0, courbF2 = 0;           // courbure de la trajectoire filtrée (1/m), 1er et 2e filtre
    double ecartF1 = 0, ecartF2 = 0;           // écart direction du mouvement − cap de la caisse (rad), filtré
    double sPrec = 0, latPrec = 0, psiPrec = 0; bool init = false, psiConnu = false;
    // un pas fixe : s = abscisse sur la route (m), lat = écart latéral (m, + à droite), capCaisse = cap de la caisse par rapport
    // à la route (rad, + à droite), kappa = courbure de la route (1/m, + à droite), h = durée du pas (s) ;
    // droites = roues forcées droites (accident, voiture qui traverse le carrefour) ; un saut > 20 m (réapparition) : reprise
    void pas(double s, double lat, double capCaisse, double kappa, float empattement, double h, bool droites = false);
    float rendu(double alpha) const { return braquagePrec + (braquage - braquagePrec) * (float)alpha; }
};
// courbure de la route à l'abscisse s (unités) : 1/m, + à droite (Segment::curve × CURVE_K par segment de SEG_M)
inline double courbureRoute(const Track& t, double sUnits) { return t.findSegment(sUnits).curve * CURVE_K / SEG_M; }
// pose des roues d'une voiture suivie : roulage = −distance / rayon (distance en unités, interpolée par l'appelant), braquage lissé
RouesPose poseSuivi(const SuiviRoues& sv, double alpha, double distUnits, float rayon);

} // namespace roues
} // namespace wfs
