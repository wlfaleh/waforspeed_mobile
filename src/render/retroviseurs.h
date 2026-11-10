// WaForSpeed — WFS-324 : rétroviseurs de la vue Cockpit avec un vrai reflet (intérieur, extérieur gauche, extérieur droit).
//
// Méthode (miroir plan exact) : la glace est la « fenêtre » d'une projection décentrée (Kooima, « Generalized Perspective
// Projection », 2008) vue depuis l'œil de la caméra, et le monde est réfléchi par le plan de la glace. Le coin de la glace et
// le texel qui s'y affiche correspondent donc exactement, quelle que soit la position de la tête : la glace porte des uv fixes
// (0..1 sur son rectangle englobant, v vers le bas) et lit l'image de son reflet, rendue à basse résolution par la passe
// enfichable « rétroviseurs » (étape Ecran, avant la scène). Plan proche = plan de la glace (reculé de `recul` : rien de ce qui est
// devant la glace, côté non réfléchissant, n'apparaît) ; profondeur inversée, plan lointain à l'infini (comme perspectiveVk).
//
// Coût (RG-03) : une seule glace redessinée par image, à tour de rôle (intérieur, gauche, intérieur, droite : l'intérieur, le plus
// grand et le plus regardé, deux fois plus souvent), une image sur deux en Basse ; images de 352 × 88 et 192 × 112 texels ; voiture du
// joueur (vue dans les glaces seulement) en version allégée (Basse : 4 pièces), sans ses vitres ; seuls les objets que voit la glace
// de l'image sont redessinés (bit VIS_RETRO du Renderer) ; ombres lues dans la carte d'ombre de l'image (aucune passe d'ombre en
// plus), personnes au-delà de 25 m non redessinées ; rien hors de la vue Cockpit (RG-04).
#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "../core/math.h"
#include "renderer.h"

namespace wfs::cockpit {
struct InterieurDef;
}

namespace wfs::retro {

enum Glace : int { INTERIEUR = 0, GAUCHE = 1, DROITE = 2, NB_GLACES = 3 };

// une glace, dans le repère de l'œil du siège (repère de l'intérieur : x à droite, y en haut, −z devant) : repère de la glace
// (x le long de la largeur, y le long de la hauteur, +z = normale vers le conducteur), taille du rectangle englobant (m)
struct Miroir {
    mat4 repere = mat4::identity();
    float w = 0, h = 0;
    float reflexion = 0.8f;   // part de la lumière renvoyée (glace argentée de l'intérieur, chromée des extérieurs)
    float recul = 0.f;        // plan proche reculé derrière la glace (m) : coque du rétroviseur du modèle de la voiture
    float rayonExterieur = 0.f;   // rayon du coin côté extérieur de la voiture (glaces extérieures : contour en « goutte »)
    // glace extérieure : rétroviseur du modèle 3D mesuré ; decalageVue (repère de l'œil, m) : de la glace dessinée à la glace du modèle.
    // La glace est dessinée à sa place dans l'habitacle (cohérente avec le pied du pare-brise et la vitre) ; le reflet est calculé
    // depuis la glace du modèle (œil décalé d'autant) : le flanc de la voiture et sa coque y sont à leur vraie place
    bool mesure = false; vec3 decalageVue{};
};

// mesures des glaces : sources et statut au journal de WFS-324 (E = estimé : sites des constructeurs bloqués par le réseau des agents)
struct CotesExterieur { float w, h; };   // glace extérieure (m)
CotesExterieur cotesExterieur(const char* car);

// mesure au chargement d'un .wfm (model_loader) : rétroviseurs extérieurs du modèle = sommets du haut de la carrosserie (40 à 90 %
// de la hauteur), dans la moitié avant, qui dépassent de plus de 2 cm la largeur de l'arrière de la voiture à la même hauteur (rien
// d'autre ne dépasse : la caisse se resserre vers le haut) ; boîte de chaque côté dans Model::retroMin / retroMax (repère du modèle)
void mesurerRetroviseurs(Model& m, const float* xyz, size_t pas, size_t n);

// la voiture dont l'intérieur porte les glaces : largeur et longueur de sa fiche (m), modèle 3D (rétroviseurs mesurés, ou nul)
struct Carrosserie { float largeur = 0, longueur = 0; const Model* modele = nullptr; };

// les trois glaces d'un intérieur. cx, demi : centre de la voiture dans le repère de l'œil et demi-largeur aux coudes (WFS-315).
// Glaces extérieures : place estimée d'après l'habitacle et la largeur de la fiche ; reflet vu depuis la glace du rétroviseur du
// modèle quand il est mesuré (Miroir::decalageVue ; sa coque reste devant la glace, coupée par le plan proche)
std::array<Miroir, NB_GLACES> miroirs(const cockpit::InterieurDef& d, float cx, float demi, const Carrosserie& c);

// ---------- projection (fonctions pures, tests/test_retroviseurs.cpp) ----------
// coins de la glace dans le monde : bas gauche, bas droit, haut gauche (vus de l'œil) ; avecDecalage : glace du modèle (reflet)
struct Fenetre { vec3 pa, pb, pc; };
Fenetre fenetre(const Miroir& m, const mat4& repereInterieur, bool avecDecalage = false);
// réflexion par le plan (point p, normale unitaire n)
mat4 reflexion(vec3 p, vec3 n);
// vue-projection du reflet vu depuis `oeil` (monde) par la fenêtre f ; oeilReflet : œil réfléchi (lumière, brouillard, ciel) ;
// faux si l'œil est derrière la glace (rien à voir)
bool vueProjMiroir(const Fenetre& f, vec3 oeil, float recul, mat4& vp, vec3& oeilReflet);

// ---------- état de l'image ----------
// à appeler avant Renderer::beginFrame (système « camera », ambiance) : actif = vue Cockpit avec un intérieur ; règle fp.retro et
// fp.retroVueProj pour la glace redessinée à cette image ; basse (qualité Basse) : une glace toutes les deux images
void preparer(FrameParams& fp, bool actif, const std::array<Miroir, NB_GLACES>& m, const mat4& repereInterieur, bool basse = false);
bool actif();                 // vrai si une glace est redessinée à cette image
int glaceDeLImage();          // glace redessinée à cette image (−1 : aucune)
uint64_t imagesRendues(int glace);   // banc : images de reflet rendues depuis le lancement
struct Comptes { uint32_t appels = 0, triangles = 0, trianglesJoueur = 0; };
Comptes comptesGlace(int glace);   // banc : appels de dessin et triangles de la dernière image de cette glace
// banc : image du reflet d'une glace relue (GPU à l'arrêt), lumière HDR linéaire rgba ligne par ligne
bool lireImage(Renderer& r, int glace, std::vector<float>& rgba, uint32_t& w, uint32_t& h);
void forcerGlace(int glace);  // banc : glace redessinée aux images suivantes (−1 : à tour de rôle)
void desactiver(bool d);      // banc : aucune glace redessinée (mesure du coût : avec / sans)
// banc : rectangle de la glace à l'écran à la dernière image préparée (pixels d'une image W × H) ; faux si hors de l'image
bool rectEcran(int glace, float W, float H, float& x0, float& y0, float& x1, float& y1);

// images des reflets (créées à la première demande, GPU attendu) : texture de la glace (HDR linéaire, lue par scene.frag)
const Image* image(Renderer& r, int glace);

// matériau d'une glace à reflet : texture du reflet × réflexion, sans diffus (scene.frag : F_EMTEX et params.z = CUTOFF_RETRO)
constexpr float CUTOFF_RETRO = -2.f;
Material materiau(Renderer& r, int glace, float reflexion);

} // namespace wfs::retro
