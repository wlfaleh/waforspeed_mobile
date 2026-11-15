// WaForSpeed — WFS-550 (épic E23, interface R19 3/5) : jauges communes de l'interface 2D.
// Cadran à arc fin (garage, maquette 13), anneau du temps restant (cadeau du jour et voiture prêtée, maquettes 05 et 26),
// barre fine graduée (fiche du garage, chargement 08, combo du HUD 17) et segments inclinés de la position dans la liste
// des voitures (maquette 15). Toutes en trait fin, dégradé de la marque cyan → orange (verre::degradeMarque, WFS-548).
// Interface 2D seulement (skill design-street-racing) : le monde 3D n'est pas concerné.
// Les calculs (fractions, angles, place des segments) sont des fonctions pures, testées par tests/test_jauges.cpp.
#pragma once
#include <algorithm>
#include <cstdint>

namespace wfs {
class Renderer;
namespace jauges {

constexpr float PI_J = 3.14159265358979f;
// cadran : arc de 240° ouvert en bas (design-street-racing, « Compteur / jauge ») ; angles de dessin2d : 0 en haut, sens horaire
constexpr float CADRAN_DEBUT = -PI_J * 2 / 3, CADRAN_FIN = PI_J * 2 / 3;

// fraction bornée à [0, 1] (valeur hors bornes ou bornes inversées : jamais d'arc qui déborde)
inline float fraction(float valeur, float mini, float maxi) {
    if (!(maxi > mini)) return 0;
    const float f = (valeur - mini) / (maxi - mini);
    return f < 0 ? 0 : f > 1 ? 1 : (f == f ? f : 0);   // NaN → 0
}
// RG-03 : anneau d'une attente (cadeau du jour) : 0 au début, 1 quand c'est l'heure ; `restant` et `total` en secondes
inline float fractionAttente(long long restant, long long total) {
    if (total <= 0) return 1;
    return fraction((float)(total - restant), 0.f, (float)total);
}
// RG-03 : anneau d'un temps compté (voiture prêtée) : 1 au début, 0 à la fin
inline float fractionRestant(long long restant, long long total) {
    if (total <= 0) return 0;
    return fraction((float)restant, 0.f, (float)total);
}
// angle de fin du remplissage d'un cadran pour une fraction donnée
inline float angleCadran(float f) { return CADRAN_DEBUT + (CADRAN_FIN - CADRAN_DEBUT) * std::clamp(f, 0.f, 1.f); }

// RG-04 : état d'un segment de la position dans la liste des voitures
enum class Segment : std::uint8_t { Possedee, Bloquee, Affichee };
// place d'un segment : largeur normale `l`, segment affiché allongé (×2,5, maquette 15), écart `e` ; renvoie le bord gauche
// du segment i et sa largeur ; total = largeur de toute la rangée (pour la centrer)
struct PlaceSegment { float x, l; };
inline PlaceSegment placeSegment(const Segment* etats, int n, int i, float l, float e) {
    float x = 0;
    for (int j = 0; j < i && j < n; j++) x += (etats[j] == Segment::Affichee ? l * 2.5f : l) + e;
    return {x, (i >= 0 && i < n && etats[i] == Segment::Affichee) ? l * 2.5f : l};
}
inline float largeurSegments(const Segment* etats, int n, float l, float e) {
    if (n <= 0) return 0;
    const PlaceSegment p = placeSegment(etats, n, n - 1, l, e);
    return p.x + p.l;
}

// RG-02 : chargement en 4 étapes (Moteur, Modèles 3D, Voitures, Ville). Chaque segment commence au palier où son texte est
// annoncé (revue de la PR #424 : le segment en cours correspond toujours au texte affiché) :
//   0 % « Préparation du moteur » et 10 % « Construction de la route » → Moteur ;
//   20 % « Chargement du modèle 3D… » (Game::init) → Modèles 3D ;
//   70 % « Chargement des voitures » (Game::init, juste avant le trafic, système 50) → Voitures ;
//   85 % « Construction de la ville » (système d'annonce 99 de chargement.cpp, juste avant la ville, système 100) → Ville.
// Une étape n'est finie que quand la suivante est annoncée.
constexpr float BORNES_CHARGEMENT[5] = {0.f, 0.20f, 0.70f, 0.85f, 1.f};
inline float avancementEtape(float p, int etape) {   // 0 : pas commencée ; 1 : finie
    if (etape < 0 || etape > 3) return 0;
    return fraction(p, BORNES_CHARGEMENT[etape], BORNES_CHARGEMENT[etape + 1]);
}

// ---- dessin (pixels de l'interface, couleurs sRGB, `a` = opacité de l'écran)
// cadran : piste grise sur 240°, remplissage dégradé cyan → orange de la fin de la piste jusqu'à `f` (le bout est orange)
void cadran(Renderer& r, float cx, float cy, float rayon, float ep, float f, float a = 1);
// anneau complet du temps restant : piste grise, remplissage dégradé (départ en haut, sens horaire) jusqu'à `f`
void anneau(Renderer& r, float cx, float cy, float rayon, float ep, float f, float a = 1);
// barre fine : fond de verre, remplissage du dégradé de la marque pris sur toute la largeur (le bout montre où on en est),
// `graduations` traits fins (0 : aucun), bouts arrondis
void barreFine(Renderer& r, float x, float y, float w, float h, float f, float a = 1, int graduations = 0);
// barre d'étape du chargement (4 segments) : finie = blanche, en cours = orange partielle sur fond gris, à venir = grise
void barreEtape(Renderer& r, float x, float y, float w, float h, float avancement, bool enCours, float a = 1);
// segments inclinés (−10°) de la position, rangée centrée sur cx, de hauteur h ; renvoie la largeur dessinée.
// zones (facultatif) : rectangle de chaque segment (x, y, w, h), pour l'appui
struct Rect { float x, y, w, h; };
float segments(Renderer& r, float cx, float y, float h, const Segment* etats, int n, float a = 1, Rect* zones = nullptr);

} // namespace jauges
} // namespace wfs
