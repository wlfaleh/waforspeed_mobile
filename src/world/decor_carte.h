// WaForSpeed — socle commun des cartes Campagne et Grand Prix (ticket WFS-53, épic E2), sur le modèle de la plage (WFS-14).
// Port des outils de js/world/renderer.js (box, lateral, axes, buildChunk) et de decor.js (PRIM, GROUND, DECOR) :
//   - couleurs unies dans une palette (texture PAL_W × 1, une case par couleur du jeu web) : sol, bâtiments, écorces…
//     → 1 maillage par morceau (matériau MAT_VTINT : la luminosité des taches d'écorce est portée par la normale) ;
//   - feuillage en cartes alpha découpées (atlas du jeu web, src/world/vegetation.*) : 2 matériaux (feuillus, conifères) ;
//   - lampes et projecteurs (« neon » du jeu web) et chaussée du circuit (texture roadCircuit) : 1 maillage chacun ;
//   - pales des éoliennes : 1 maillage par éolienne, tourné à chaque image (rotors de scene.js).
// Au plus 5 appels de dessin par morceau (25 morceaux), en pratique 3 ou 4, plus 3 au plus pour le texte des panneaux
// publicitaires du circuit (WFS-93) : ≤ 300 au total avec la route (journal du banc).
// Les bas-côtés partent du bord de la chaussée (GROUND du jeu web) : Game ne dessine ni son sol ni ses trottoirs.
#pragma once
#include "chaussee.h"   // WFS-212
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "../core/system.h"
#include "../game/track.h"
#include "../game/chocs_decor.h"   // WFS-256 (accroche)
#include "../render/lots_decor.h"   // WFS-268 : décor fixe préparé au chargement
#include "../render/renderer.h"
#include "giratoire.h"   // WFS-248 : giratoires reconstruits
#include "vegetation.h"

namespace wfs {
class Game;
namespace carte {

using Geo = veg::Geo;
// P_TEXTE0..2 : lettres des panneaux publicitaires du circuit (WFS-93), une part par couleur de lettres
enum Part { P_UNI, P_FEUILLU, P_CONIF, P_NEON, P_ROUTE, P_TEXTE0, P_TEXTE1, P_TEXTE2, P_TERRE, P_COUNT };   // WFS-383 : P_TERRE sol naturel (sols.glsl)
constexpr int PAL_W = 128;                // cases de la palette
constexpr int AHEAD = 22, BEHIND = 2;     // morceaux dessinés (comme la route de Game)
// WFS-268 (décision du PO, 2026-11-07 18:33 UTC) : Campagne et Grand Prix gardent l'ancienne gestion en course (un tour préparé
// = 175 à 212 Mo GPU, trop pour un téléphone de 4 Go) jusqu'aux instances des arbres (WFS-273) ; true = décor préparé au chargement
constexpr bool LOTS_CARTES_SOCLE = false;
constexpr double LOD_FAR_M = 200;         // au-delà : arbres allégés, petits objets non dessinés (SMALL_DECOR)

// bande de sol : de a à b (m depuis l'axe), hauteurs ya → yb, case de palette
struct Band { float a, b, ya, yb; int col; };

// repère du segment a (pose + axes du jeu web) : o sur l'axe, r vers la droite, f vers l'avant, th cap
struct Pose { vec3 o, r, f; float th; };
Pose poseAt(Track& t, int64_t a);
inline vec3 lateral(const Pose& p, float lat, float dy = 0) { return p.o + p.r * lat + vec3{0, dy, 0}; }

vec3 lin(vec3 c);
vec3 linHex(uint32_t h);

// ---------- primitives (PRIM du jeu web), uv = case de palette ----------
void tri(Geo& g, vec3 a, vec3 b, vec3 c, vec2 uv);
void quad(Geo& g, vec3 a, vec3 b, vec3 c, vec3 d, vec2 uv);
// quad horizontal (sol) : normale vers le haut quel que soit l'ordre des sommets
void quadSol(Geo& g, vec3 a, vec3 b, vec3 c, vec3 d, vec2 uv);
// box de renderer.js : base en o (+ y0), largeur w selon r, hauteur h, longueur l selon f, sans dessous ;
// taper : rétrécit le dessus (vignes)
void box(Geo& g, vec3 o, vec3 r, vec3 f, float w, float h, float l, vec2 uv, float y0 = 0, float taper = 1);
// PRIM.cyl / cyl12 : cylindre vertical, base en b, rayon rad, hauteur h, n côtés (avec couvercle)
void cyl(Geo& g, vec3 b, float rad, float h, int n, vec2 uv);
// cylindre couché d'axe unitaire ax, de b à b + ax · len (bottes de foin)
void cylAxe(Geo& g, vec3 b, vec3 ax, float rad, float len, int n, vec2 uv);
// PRIM.cone : base en b, rayon rad, hauteur h, n côtés, tourné de yaw
void cone(Geo& g, vec3 b, float rad, float h, int n, vec2 uv, float yaw = 0);
// PRIM.roof (prisme à section en losange) centré en c : faîtage selon r (longueur L), hauteur H, largeur W selon f
void toit(Geo& g, vec3 c, vec3 r, vec3 f, float L, float H, float W, vec2 uv);

// WFS-216 (RG-01, RG-04) : vue libre du banc d'essai (montagne du col, épingle vue d'en haut) : œil, point visé, champ
// vertical ; active, la carte dessine son décor dans la scène dédiée (route et voiture du joueur dessinées par Game)
struct VueBancCarte { bool actif = false; vec3 oeil{}, cible{}; float fov = 55; };
extern VueBancCarte g_vueCarte;

// ---------- système de carte ----------
class CarteDecor : public System {
public:
    void init(Renderer& r, const GameView& g) override;
    bool remplaceSol(const GameView&) const override { return active_; }
    bool remplaceTrottoir(const GameView&) const override { return active_; }
    void ambiance(FrameParams& fp, const GameView& g) override;
    void collide(double dt, Player& p, const GameView& g) override;
    void render(Renderer& r, const GameView& g) override;
    bool scene(FrameParams& fp, SceneDediee& sd, const GameView& g) override;   // WFS-216 : vue libre du banc
    void shutdown(Renderer& r) override;

protected:
    // identifiant de la carte (MAPS[].id)
    virtual const char* carteId() const = 0;
    // envFor (maps.js) : jour (couleur et distance du brouillard) ; nuit : horizon, brouillard, distance
    struct Env { uint32_t fogJour; float farJour; uint32_t horizonNuit, fogNuit; float farNuit; };
    virtual Env env() const = 0;
    // WFS-343 (accroche) : couleurs propres à la carte dans les cases libres de la palette (C_NB à PAL_W − 1) ; touffes d'herbe des bas-côtés
    virtual void couleursCarte(std::array<uint32_t, PAL_W>&) const {}
    virtual bool touffes() const { return true; }
    // WFS-383 : bande de sol naturel (herbe, champ, fossé, forêt, colline) dessinée avec le sol des cartes (sols.glsl, pente et taches)
    virtual bool solNaturel(int col) const;
    // profil des bas-côtés (GROUND) de la zone, côté side (±1), au segment a (couleurs qui dépendent du segment)
    virtual int profil(const char* zone, int64_t a, int side, Band out[6]) const = 0;
    // objet du décor (deco.items du jeu web) : type propre à la carte (T_TREE, T_VERGE, T_LAMP : communs), paramètres
public:
    enum { T_TREE, T_VERGE, T_LAMP, T_CARTE };
    enum Kind { K_ROUND, K_PINE, K_CYPRESS };
protected:
    struct Item { int t; int side = 1; float lat = 0, h = 0, w = 0, d = 0, l = 0, along = 0; int k = 0, roof = 0, floors = 0, col = 0; bool petit = false;
                  bool ilot = false; };   // WFS-248 : objet de l'îlot d'un giratoire (seul objet permis sur l'ouvrage)
    void item(int64_t n, const Item& it) { items_[(size_t)(((n % len()) + len()) % len())].push_back(it); }
    // placement de tout le décor (generateDecor), une fois ; zones [z0, z1) de même zone
    virtual void place(const char* zone, int64_t z0, int64_t z1, Mulberry32& D) = 0;
    // dessin d'un objet propre à la carte (appelé en parallèle : lit seulement la piste et les objets)
    virtual void dessine(Track& t, const Item& it, int64_t a, const Pose& p, bool lod, Geo out[P_COUNT]) const = 0;
    // WFS-256 (accroche) : formes de choc de l'objet (chocs_decor.h, 4 au plus), relevées sur son dessin ; 0 : rien de
    // solide. Ici : arbres et lampadaires (communs) ; chaque carte ajoute ses objets (campagne.cpp, circuit.cpp)
    virtual int solide(const Item& it, chocs::ObstacleRoute o[4]) const;
    // chaussée propre (circuit : texture roadCircuit) : false = celle de Game
    virtual bool routePropre() const { return false; }
    // dessins animés après les morceaux (rotors des éoliennes) ; morceaux c0..c1 chargés ; renvoie le nombre de dessins
    virtual int animes(Renderer&, const GameView&, int64_t, int64_t) { return 0; }
    virtual void shutdownCarte(Renderer&) {}

    const MapDef& map() const { return *track_->map(); }
    int64_t len() const { return (int64_t)track_->segmentCount(); }
    const char* zone(int64_t a) const;
    // hauteur du sol (groundY du jeu web) à la distance lat, d'après le profil de la zone
    float solY(const char* zone, int64_t a, int side, float lat) const;
    static vec2 uv(int c) { return {(c + 0.5f) / PAL_W, 0.5f}; }
    // segment absolu du morceau [a0, a1) qui porte l'objet du segment seg (la piste se répète)
    bool dansMorceau(int64_t seg, int64_t a0, int64_t a1, int64_t& abs) const;

    // DECOR.tree (life.js) pour la campagne et le circuit : feuillu (chêne), sapin / pin, cyprès, sous-bois
    void arbre(Track& t, const Item& it, int64_t a, bool lod, Geo out[P_COUNT]) const;
    // DECOR.verge : touffes d'herbe des bas-côtés (aucun tirage du décor)
    void touffes(Track& t, int64_t a, Geo out[P_COUNT]) const;
    // DECOR.lamp : lampadaire de ville (village) ou projecteur de circuit (flood)
    void lampe(const Pose& p, int side, bool flood, Geo out[P_COUNT]) const;
    // WFS-212 (RG-02, RG-04) : le point q (monde) tombe-t-il sur la chaussée d'une AUTRE partie du tracé (axe à moins de RW + marge),
    // autour d'un rond-point ou d'une épingle (anneau ± 80 m) ? la partie du tracé du segment a lui-même (± 30 m) est exclue
    bool surAutreChaussee(const vec3& q, int64_t a, float marge) const;
    bool procheSerre(int64_t a) const;   // segment a dans la zone d'un rond-point ou d'une épingle

    bool active_ = false, night_ = false;
    Track* track_ = nullptr;
    Material mats_[P_COUNT];
    bool solActif_ = false;   // WFS-383 : la carte a un climat de sol (cartes du code ; carte de données avec sa ligne « sol »)
    Image palTex_{}, leafTex_{}, routeTex_{};
    int lastDraws_ = 0;
    int64_t depart_ = 0;   // banc d'essai : départ du joueur avancé de depart_ segments (WFS_DEPART)
    // WFS-248 : giratoires reconstruits (un par rond-point du tracé 2) et segment (tour) de leur centre
    std::vector<giratoire::Giratoire> gir_;
    std::vector<int64_t> girSeg_;
    void preparerDecor(Renderer& r);   // WFS-268 : décor fixe préparé (fin de l'init de la carte)
    bool surGiratoire(const vec3& q, float marge) const { for (const giratoire::Giratoire& g : gir_) if (giratoire::dansOuvrage(g, q, marge)) return true; return false; }

private:
    void buildChunk(Track& t, int64_t c, bool lod, Geo out[P_COUNT]);
    std::vector<std::vector<Item>> items_;   // objets de chaque segment de la piste
    std::vector<chaussee::Axe> serres_;   // WFS-212 : axe de la route autour des sections repliées (chaussee.h)
    struct Chunk { std::array<Mesh, P_COUNT> m{}; bool lod = false; };
    std::map<int64_t, Chunk> chunks_;   // ancienne gestion (repli, lots::g_lotsDecor = false)
    lots::DecorFixe lots_;   // WFS-268 : décor du tour 0, préparé au chargement
    bool lotsTente_ = false;
    int64_t limiteTour_ = INT64_MAX;   // WFS-268 : fin des morceaux (= len() pendant la préparation : dernier morceau borné au tour)
    std::vector<std::pair<uint64_t, Mesh>> graveyard_;
    uint64_t frame_ = 0;
};

// palette commune des deux cartes (codes du jeu web : GCOL, GCOL2, FIELDS, C, C2, BARK)
enum Col {
    // sol
    C_VERGE, C_DITCH, C_FOREST, C_FARHILL, C_WALK, C_FIELD0, C_VINE_A = C_FIELD0 + 6, C_VINE_B, C_RED, C_WHITE, C_GRASS, C_GRASS2,
    C_GRAVEL, C_PITLANE,
    // écorces : deux cases voisines (mélange le long de u)
    C_OAK, C_OAK2, C_FIR, C_PINE, C_PINELOW,
    // campagne
    C_WOOD, C_HEDGE, C_WALL, C_WALL2, C_BARN, C_ROOF0, C_ROOF1, C_ROOF2, C_GLASS, C_HAY, C_TURB, C_WALLS0, C_CHURCH = C_WALLS0 + 6,
    C_VINE, C_METAL, C_SIGNW, C_SIGNR, C_POLE, C_ZEBRA, C_STEEL, C_CHECK_W, C_CHECK_B, C_LAMP_W, C_LAMP_C,
    // circuit
    C_TIRE, C_TIRE_R, C_TIRE_W, C_STAND, C_STAND_BACK, C_STAND_ROOF, C_SEAT0, C_AD0 = C_SEAT0 + 4, C_MARSHAL = C_AD0 + 8, C_MARSHAL_TOP,
    C_PITWALL, C_PITGLASS, C_GARAGE, C_GARAGE_DOOR, C_GARAGE0, C_FLAGPOLE = C_GARAGE0 + 6, C_FLAG0, C_PIERRE0 = C_FLAG0 + 6, C_NB = C_PIERRE0 + 3   // WFS-178 : 3 pierres calcaires
};
static_assert(C_NB <= PAL_W, "palette trop petite");

} // namespace carte

// banc d'essai (WFS-53) : captures carte-campagne-* et carte-circuit-* (src/world/cartes_banc.cpp)
void benchCartes(Game& g, Renderer& r, const std::string& out);
void benchVille(Game& g, Renderer& r, const std::string& out);   // WFS-137 : lieux de la Ville au tracé 2 (appelé par benchCartes)
// WFS-138 : campagne au tracé 2 : col, épingle, vignes, de jour et de nuit, caméra de course (carte-campagne-col…)
int benchCampagne(Game& g, Renderer& r, const std::string& out, bool parcours = true);
int benchRondsPoints(Game& g, Renderer& r, const std::string& out);   // WFS-223 : ronds-points des 4 cartes (rondpoint-*)   // parcours : départ → haut du col au pilote

} // namespace wfs
