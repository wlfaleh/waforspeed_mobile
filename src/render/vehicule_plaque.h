// WaForSpeed — WFS-483 (épic E17, zone E10) : plaque d'immatriculation 3D, avant et arrière, de la voiture du joueur.
// La plaque porte le texte du profil (WFS-480, lu seulement). C'est un vrai modèle, aux mesures d'une plaque réelle (RG-01) :
//   - porte-plaque en plastique noir grainé, 540 × 130 mm, 7 mm d'épaisseur, coins arrondis, bords chanfreinés, lèvre qui
//     recouvre 2 mm du bord de la plaque (comme un cadre clipsé) ;
//   - plaque en aluminium de 1 mm, 520 × 110 mm, coins arrondis (rayon 5 mm), bords roulés vers l'arrière ;
//   - caractères en relief de 1 mm et bourrelet du bord embouti, rendus par la carte de normales de la texture de la plaque ;
//   - 2 vis à tête bombée fendue (zinguées), au milieu de la hauteur, à gauche et dans la bande de droite.
// Style inventé pour le jeu (RG-02) : fond blanc cassé, liseré noir, bande verticale gris anthracite à droite avec un chevron
// et « WFS », caractères noirs en Rajdhani Bold (OFL, police du jeu) ; ni bande européenne, ni code de pays, ni blason.
// Place (RG-04) : une ligne par voiture (vehicule_plaque_defs.cpp), relevée sur la plaque d'origine du .wfm, que le porte-plaque
// recouvre entièrement (elle n'est donc jamais doublée). Nuit (RG-05) : la plaque arrière est éclairée (éclaireur de plaque).
// Calcul de la géométrie et de l'image sur le processeur (tests/test_plaque_joueur.cpp) ; dessin : vehicule_plaque.cpp.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "../core/math.h"

namespace wfs {
class Renderer;
}

namespace wfs::plaque_joueur {

// ---- RG-01 : mesures réelles (mm). Plaque : format européen courant 520 × 110 mm, tôle d'aluminium de 1 mm, rayon des coins
// 5 mm ; caractères de 75 mm de haut en relief de 1 mm (arrêté du 9 février 2009, annexe : hauteur 75 mm). Porte-plaque : cadre
// clipsé du commerce, 540 × 130 mm hors tout, profondeur 7 mm, lèvre de 2 mm sur le bord de la plaque.
constexpr float PLAQUE_L_MM = 520.f, PLAQUE_H_MM = 110.f, PLAQUE_EP_MM = 1.f, PLAQUE_RAYON_MM = 5.f;
constexpr float CARACTERE_H_MM = 75.f, RELIEF_MM = 1.f;
constexpr float SUPPORT_L_MM = 540.f, SUPPORT_H_MM = 130.f, SUPPORT_EP_MM = 7.f, SUPPORT_RAYON_MM = 9.f, LEVRE_MM = 2.f;
constexpr float PLAQUE_Z_MM = 2.f;          // arrière de la plaque au-dessus du fond du porte-plaque (logement)
constexpr float BANDE_L_MM = 40.f;          // bande verticale de droite (RG-02)
constexpr float VIS_D_MM = 9.f, VIS_H_MM = 2.2f;   // tête de vis bombée (vis à tôle 4,2 mm, tête de 9 mm)
constexpr float VIS_X_GAUCHE_MM = 18.f;     // centre de la vis de gauche depuis le bord gauche ; la vis de droite est au milieu de la bande
constexpr float ZONE_TEXTE_G_MM = 32.f, ZONE_TEXTE_D_MM = 6.f;   // marge du texte à gauche (après la vis), à droite (avant la bande)
constexpr float CONDENSE = 0.80f;           // chasse des caractères : Rajdhani resserré à 80 % (constante : jamais étiré au texte)
constexpr float ECART_MIN_MM = 3.f;         // espace minimal entre deux caractères
// ---- RG-06 : texture du texte (≥ 512 × 108 px) : 1024 × 216 px, soit ≈ 2 px par mm
constexpr int IMAGE_L = 1024, IMAGE_H = 216;
constexpr int TEXTE_MAX = 8;

// ---- RG-04 : place d'une plaque dans le repère du modèle (m ; avant de la voiture vers −z) : centre de la plaque d'origine
// (sur sa face), normale vers l'extérieur ; la verticale du plan est déduite (la plus proche de +y)
// jours : distance (mm) du fond du porte-plaque à la caisse le long de −normale, en JOURS points répartis à intervalles égaux
// sur le contour du porte-plaque (rectangle 540 × 130 mm à coins arrondis, sens trigonométrique vu de l'extérieur, en partant
// du haut du côté droit), bornée à 60 mm (calandre ouverte). La jupe du porte-plaque descend jusqu'à la caisse en suivant ces
// jours (support adaptateur des vrais pare-chocs bombés) : il ne flotte jamais (RG-04, RG-08).
constexpr int JOURS = 64;
constexpr float JOUR_MAX_MM = 60.f;
struct Pose { vec3 centre; vec3 normale; float jours[JOURS]; };
float jourContour(const Pose& p, float s);          // jour interpolé à l'abscisse s ∈ [0, 1) du contour (mm)
vec3 pointContour(float s);                         // point du contour du porte-plaque à l'abscisse s (mm, z = 0)
struct PlacesVoiture { const char* id; Pose avant, arriere; };
const PlacesVoiture* places(const char* idVoiture);   // nullptr : voiture sans plaque connue (aucun dessin)
int nombreVoitures();
const PlacesVoiture& voiture(int i);

// repère de la plaque → modèle de la voiture : x vers la droite vue de l'extérieur, y vers le haut du plan, z = normale
mat4 repere(const Pose& p);

// ---- géométrie (repère de la plaque, m) : le fond du porte-plaque est à z = DECALAGE_M, sur la face de la plaque d'origine
constexpr float DECALAGE_M = 0.0008f;       // 0,8 mm devant la plaque d'origine : aucun z-fighting, aucun jour visible
struct Sommet { vec3 p, n; float u, v; };   // copié dans les Vertex du moteur au chargement (sans Vulkan : testable)
struct Maillage { std::vector<Sommet> sommets; std::vector<uint32_t> indices; };
struct Pieces { Maillage support, plaque, vis; };
// complet : coins à 8 segments, vis à 24 secteurs ; simple (LOD, RG-06) : coins à 3 segments, sans vis, plaque sans bord roulé
// pose : jours du porte-plaque (nullptr : porte-plaque à fond plat, sans support)
Pieces construire(bool complet, const Pose* pose = nullptr);

// ---- image de la plaque (RG-02, RG-03) : couleur sRGB (RGBA8) et normales (RGBA8, repère u vers la droite, v vers le bas)
struct ImagePlaque {
    std::vector<uint8_t> couleur, normales;
    float hauteurTexteMm = 0;               // hauteur des caractères posés (75 mm, sauf texte trop large : réduit, jamais étiré)
    float largeurTexteMm = 0;               // largeur du texte posé
    int caracteres = 0;                     // caractères dessinés
    int largeur = 0, hauteur = 0;           // taille de l'image (px)
};
// texte : 1 à 8 caractères A-Z, 0-9, « - » (profil.h) ; un autre caractère est ignoré. ttf : police Rajdhani Bold
// largeur (WFS-484) : largeur de l'image en px (64 à IMAGE_L), hauteur dans la proportion de la plaque (256 → 54 px)
bool dessinerImage(const std::string& texte, const uint8_t* ttf, ImagePlaque& sortie, int largeur = IMAGE_L);

// ---- matériaux et niveaux de détail (RG-05 à RG-07), partagés avec les plaques des autres voitures (WFS-484 RG-01)
// porte-plaque : polypropylène noir grainé (albédo ≈ 0,03, rugueux) ; vis : acier zingué (métal, satiné) ; plaque : film blanc mat
constexpr float SUPPORT_RVB[3] = {0.028f, 0.028f, 0.030f}, SUPPORT_RUGOSITE = 0.68f;
constexpr float VIS_RVB[3] = {0.62f, 0.64f, 0.66f}, VIS_RUGOSITE = 0.38f;
constexpr float PLAQUE_RUGOSITE = 0.62f;
// RG-05 : luminance de la plaque éclairée ≈ 2 à 20 cd/m² (règlement ONU n° 4 : ≥ 2,5 cd/m² en tout point) ; en unités
// d'émission du moteur (fenêtres éclairées de la ville : 0,9 ; enseignes : 2 à 6), 0,30 × la couleur de la plaque
constexpr float ECLAIREUR = 0.30f;
constexpr float LOD_M = 14.f;              // au-delà, porte-plaque et plaque simplifiés, sans vis (RG-06 : jamais absente)
constexpr float LOD_BASSE_M = 4.f;         // qualité Basse : version simple dès 4 m (RG-07 : coût par image)

// ---- dessin (thread principal, vehicule_plaque.cpp) ----
void preparer(Renderer& r);                 // maillages et matériaux (au chargement de la partie)
void voiturePreparee(Renderer& r, const char* idVoiture);   // porte-plaques de cette voiture et texture du texte (au choix de la voiture)
// dessine les 2 plaques de la voiture idVoiture posée en monde ; nuit : éclaireur de la plaque arrière allumé. Le texte est
// celui du profil (ou WFS_PLAQUE au banc) ; sa texture n'est refaite que lorsqu'il change (RG-07)
void dessiner(Renderer& r, const char* idVoiture, const mat4& monde, bool nuit, const vec3& camera);
void liberer(Renderer& r);
void texteBanc(const char* texte);          // banc seulement : texte imposé (nullptr : celui du profil)
void masquerBanc(bool masque);              // banc seulement (mesure RG-07) : plaques non dessinées
const std::string& texteAffiche();          // texte de la dernière texture faite (journal du banc)

} // namespace wfs::plaque_joueur
