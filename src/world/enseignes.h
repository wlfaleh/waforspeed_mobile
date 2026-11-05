// WaForSpeed — ville vivante (WFS-71, épic E2) : outils partagés par les enseignes, le portique et les commerces.
//   - géométrie : boîtes, cylindres, cônes et plans orientés selon la route (box / addGeo de js/world/decor.js) ;
//   - palette : toutes les couleurs unies du décor dans une seule petite texture (1 matériau, 1 appel de dessin) ;
//   - atlas de textes : enseignes, panneaux et portique écrits une fois au démarrage avec la police du jeu
//     (Rajdhani Bold, stb_truetype, comme le texte de l'interface WFS-26), lettres blanches sur fond transparent :
//     dessinées en alpha découpé avec une émission de la couleur du néon (lettres lumineuses, fond invisible).
// Systèmes : Enseignes (panneaux néon BOARD_TEXTS, ordre 107), Portique (checkpoint, 108), Commerces (SHOPS, 109).
#pragma once
#include <map>
#include <string>
#include <vector>
#include "../core/math.h"
#include "../render/renderer.h"

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
    void ajouter(const std::string& utf8);           // avant construire()
    bool construire(Renderer& r);
    const Texte& texte(const std::string& utf8) const;
    void detruire(Renderer& r);
    // matériau du néon d'une couleur (lettres en alpha découpé + émission), créé à la demande
    const Material& materiau(Renderer& r, unsigned hex, float emission = 1.6f, float seuil = 0.45f);
private:
    std::vector<std::string> textes_;
    std::map<std::string, Texte> places_;
    std::map<unsigned, Material> mats_;
    Image img_{};
};

// matériau néon uni (barres lumineuses) d'une couleur 0xRRGGBB
Material neon(Renderer& r, unsigned hex, float emission = 2.2f);
vec3 lin(unsigned hex);

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
