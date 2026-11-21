// WaForSpeed — ville vivante (WFS-71, épic E2) : outils partagés par les enseignes, le portique et les commerces.
//   - géométrie : boîtes, cylindres, cônes et plans orientés selon la route (box / addGeo de js/world/decor.js) ;
//   - palette : toutes les couleurs unies du décor dans une seule petite texture (1 matériau, 1 appel de dessin) ;
//   - atlas de textes : enseignes, panneaux et portique écrits une fois au démarrage avec la police du jeu
//     (Rajdhani Bold, stb_truetype, comme le texte de l'interface WFS-26), lettres blanches sur fond transparent :
//     dessinées en alpha découpé avec une émission de la couleur du néon (lettres lumineuses, fond invisible).
// Systèmes : Enseignes (caissons d’enseigne sur mâts BOARD_TEXTS, WFS-568, ordre 107), Portique (checkpoint, 108), Commerces (SHOPS, 109).
#pragma once
#include <map>
#include <string>
#include <tuple>
#include <vector>
#include "../core/math.h"
#include "../render/renderer.h"
#include "mobilier_ville.h"   // WFS-590 : enseignes en façade modélisées (pavés chanfreinés, tubes)

namespace wfs {
class Track;
struct MapDef;
class Game;

namespace vie {

struct Geo { std::vector<Vertex> v; std::vector<uint32_t> i; };

// repère de la route à l'abscisse s (unités) : origine sur l'axe, r vers la droite, f vers l'avant
struct Repere { vec3 o, r, f; };
Repere repere(Track& t, double s);
inline vec3 lateral(const Repere& p, float lat, float y) { return p.o + p.r * lat + vec3{0, y, 0}; }

// palette : couleur 0xRRGGBB → coordonnées de texture de sa case (couleurs enregistrées avant construire())
class Palette {
public:
    vec2 uv(unsigned hex);   // ajoute la couleur si besoin (64 au plus)
    bool construire(Renderer& r);
    Material variante(Renderer& r, const Material& m);   // WFS-568 : autre matériau sur la même texture (après construire)
    void detruire(Renderer& r);
    Material materiau;
private:
    std::vector<unsigned> couleurs_;
    Image img_{};
};

// boîte : base au centre c (au sol), largeur w selon r, hauteur h, profondeur d selon f (box du jeu web)
void boite(Geo& g, vec3 c, vec3 r, vec3 f, float w, float h, float d, vec2 uv);
// cylindre / cône verticaux (8 côtés) : base au centre c
void cylindre(Geo& g, vec3 c, float rayon, float h, vec2 uv, int n = 8);
void cone(Geo& g, vec3 c, float rayon, float h, vec2 uv, int n = 8);
// plan vertical centré en c, largeur selon x, hauteur selon y ; normale n ; coordonnées de texture (u0, v0) - (u1, v1)
void plan(Geo& g, vec3 c, vec3 x, vec3 y, vec3 n, float w, float h, float u0, float v0, float u1, float v1);

// atlas de textes : une ligne par texte, lettres blanches, transparence = couverture
struct Texte { float u0 = 0, v0 = 0, u1 = 0, v1 = 0, aspect = 1; bool ok = false; };
class AtlasTextes {
public:
    void ajouter(const std::string& utf8, unsigned couleur = 0xFFFFFF);   // avant construire() ; WFS-568 : couleur sRGB des lettres
    bool construire(Renderer& r);
    const Texte& texte(const std::string& utf8) const;
    void detruire(Renderer& r);
    // matériau du néon d'une couleur (lettres en alpha découpé + émission), créé à la demande
    const Material& materiau(Renderer& r, unsigned hex, float emission = 1.6f, float seuil = 0.45f);
    // WFS-568 : lettres de la couleur donnée à ajouter() (une couleur par commerce), émission × cette couleur
    const Material& materiauCouleurs(Renderer& r, float emission, float seuil = 0.45f, float rugosite = 0.5f);
private:
    using Cle = std::tuple<unsigned, float, float, float, bool>;   // couleur, émission, seuil, rugosité, lettres de couleur
    std::vector<std::string> textes_;
    std::map<std::string, Texte> places_;
    std::map<std::string, unsigned> couleurs_;
    std::map<Cle, Material> mats_;
    Image img_{};
};

// ---------------------------------------------------------------- WFS-590 : enseignes de commerce en façade (ANO-317)
// Ce qui existe vraiment dans une rue commerçante française (Code de l'environnement, art. R581-58 à R581-65 : enseigne à
// plat sur la façade, sans dépasser le mur ni les limites du 1er étage ; enseigne perpendiculaire : saillie ≤ 1/10 de la
// largeur de la rue et ≤ 2 m, règlement de voirie courant : bas à 2,50 m au moins au-dessus du trottoir). Repère de
// chaque modèle : origine au pied du mur, x hors du mur (vers la rue), y vers le haut, z le long de la façade.
// Lettres : quad du texte de l'atlas (couleur et allumage par le matériau de l'appelant), 1 mm devant la face.
struct Bandeau {
    float largeur = 6.f;      // le long de la façade (largeur de la devanture au plus)
    float hauteur = 0.70f;    // caisson de 0,60 à 0,80 m (lettres de 25 à 50 cm)
    float epaisseur = 0.15f;  // caisson lumineux de 10 à 20 cm, dos contre le mur
    float bas = 3.15f;        // bas du caisson au-dessus du trottoir (au-dessus de la vitrine et du store : 2,8 à 3,2 m)
    float lettres = 0.60f;    // hauteur de la ligne de texte de l'atlas (majuscules ≈ la moitié : 30 cm), réduite si le texte est trop long
};
// enseigne bandeau : cadre en profilé d'aluminium de 50 mm (chanfreins), joues, face imprimée en retrait de 20 mm, lettres
void enseigneBandeau(mobilier::Sortie cadre, mobilier::Sortie face, mobilier::Sortie lettres, const mobilier::Repere& R,
                     const Bandeau& b, const Texte& T, vec2 uvCadre, vec2 uvFond);
// enseigne drapeau (perpendiculaire au mur) : platine vissée (4 vis), potence en tube d'acier Ø 33,7 mm (bras et tirant),
// deux pattes, caisson double face 0,60 × 0,60 × 0,12 m de x = 0,16 à 0,76 m (saillie 0,79 m), bas à « bas » (≥ 2,5 m) ;
// T : mot court du commerce sur les deux faces (« CAFÉ », « PAIN »… : majuscules de 15 cm au plus)
void enseigneDrapeau(mobilier::Sortie metal, mobilier::Sortie face, mobilier::Sortie lettres, const mobilier::Repere& R, float bas,
                     const Texte& T, vec2 uvMetal, vec2 uvCadre, vec2 uvFond);
// croix de pharmacie (enseigne drapeau d'officine) : même platine et potence, croix grecque de 0,62 m (bras de 0,20 m),
// 0,10 m d'épaisseur, arêtes chanfreinées ; « croix » : la partie verte (allumée la nuit par l'appelant)
void croixPharmacie(mobilier::Sortie metal, mobilier::Sortie croix, const mobilier::Repere& R, float bas, vec2 uvMetal, vec2 uvCroix);

// matériau néon uni (barres lumineuses) d'une couleur 0xRRGGBB
Material neon(Renderer& r, unsigned hex, float emission = 2.2f);
vec3 lin(unsigned hex);

// WFS-568 : enseignes sur mâts le long de la route (Ville, Corniche) : segment, côté (−1 / +1), texte (indice de BOARD_TEXTS)
struct PanneauRoute { int64_t seg; int side, texte; };
std::vector<PanneauRoute> panneauxRoute(const MapDef& map, Track& t);
const char* textePanneauRoute(int texte);

// serviettes de la plage (beachLife du jeu web) : partagées entre leur dessin (Commerces) et les baigneurs (Pietons)
struct Serviette { double s; float lat, rot; int couleur; };
const std::vector<Serviette>& serviettes(const MapDef& map, int64_t segCount);
// clients attablés des terrasses (placeShop du jeu web) : s (segments), lat (m), yaw relatif à la route, hauteur
struct Client { double s; float lat, yaw, y; };
const std::vector<Client>& clientsTerrasses(const MapDef& map, int64_t segCount);
// baigneurs debout dans l'eau (beachLife : RW + 47 à 59 m, y = −2,25)
struct Baigneur { double s; float lat, yaw; };
const std::vector<Baigneur>& baigneurs(const MapDef& map, int64_t segCount);
float hauteurSable(float lat);   // sable de la Corniche côté mer (GROUND.beach), m

} // namespace vie

// banc d'essai (WFS-71) : vie-terrasse (jour), vie-enseigne-nuit, vie-portique, vie-plage, puis g.shutdown()
void benchVie(Game& g, Renderer& r, const std::string& outDir);

} // namespace wfs
