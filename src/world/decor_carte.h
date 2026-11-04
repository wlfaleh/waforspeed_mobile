// WaForSpeed — socle commun des cartes Campagne et Grand Prix (ticket WFS-53, épic E2), sur le modèle de la plage (WFS-14).
// Port des outils de js/world/renderer.js (box, lateral, axes) et de decor.js (PRIM, GROUND, DECOR.tree) :
//   - couleurs unies dans une palette (texture PAL_W × 1, une case par couleur du jeu web) : sol, bâtiments, écorces…
//     → 1 maillage par morceau ; écorces dans deux cases voisines (mélange des deux écorces le long de u) ;
//   - feuillage en cartes alpha découpées (atlas du jeu web, src/world/vegetation.*) : 2 matériaux (feuillus, conifères) ;
//   - éclairage « néon » (projecteurs du circuit) et texture de route propre (circuit) : 1 maillage chacun par morceau.
// Au plus 5 appels de dessin par morceau (25 morceaux), en pratique 3 ou 4 : ≤ 300 au total avec la route (journal du banc).
// Bas-côtés : Game dessine toujours ses trottoirs (RW → RW + 4 m, 0,15 m) ; une carte de campagne les recouvre (bande
// « face » : face verticale côté route + dessus à 0,17 m), puis suit le profil GROUND du jeu web au-delà.
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <vector>
#include "../core/system.h"
#include "../game/track.h"
#include "../render/renderer.h"
#include "vegetation.h"

namespace wfs {
class Game;
namespace carte {

using Geo = veg::Geo;
enum Part { P_UNI, P_FEUILLU, P_CONIF, P_NEON, P_ROUTE, P_COUNT };
constexpr int PAL_W = 64;                 // cases de la palette
constexpr int AHEAD = 22, BEHIND = 2;     // morceaux dessinés (comme la route de Game)
constexpr double LOD_FAR_M = 200;         // au-delà : arbres allégés, petits objets non dessinés (SMALL_DECOR)
constexpr float SOL_TROTTOIR = 0.17f;     // dessus de la bande qui recouvre le trottoir de Game (0,15 m)

// bande de sol : de a à b (m depuis l'axe), hauteurs ya → yb, case de palette ; face = face verticale en a (côté route)
struct Band { float a, b, ya, yb; int col; bool face = false; };

// repère du segment a (pose + axes du jeu web) : o sur l'axe, r vers la droite, f vers l'avant, th cap
struct Pose { vec3 o, r, f; float th; };
Pose poseAt(Track& t, int64_t a);
inline vec3 lateral(const Pose& p, float lat, float dy = 0) { return p.o + p.r * lat + vec3{0, dy, 0}; }

vec3 lin(vec3 c);
vec3 linHex(uint32_t h);

// ---------- primitives (PRIM du jeu web), uv = case de palette ----------
void tri(Geo& g, vec3 a, vec3 b, vec3 c, vec2 uv);
void quad(Geo& g, vec3 a, vec3 b, vec3 c, vec3 d, vec2 uv);
// box de renderer.js : base en o (+ y0), largeur w selon r, hauteur h, longueur l selon f (tailles entières), sans dessous ;
// taper : rétrécit le dessus (vignes)
void box(Geo& g, vec3 o, vec3 r, vec3 f, float w, float h, float l, vec2 uv, float y0 = 0, float taper = 1);
// PRIM.cyl / cyl12 : cylindre vertical, base en b, rayons rx / rz, hauteur h, n côtés (avec couvercle)
void cyl(Geo& g, vec3 b, float rx, float h, float rz, int n, vec2 uv);
// cylindre couché d'axe unitaire ax, de b à b + ax · len (bottes de foin)
void cylAxe(Geo& g, vec3 b, vec3 ax, float rad, float len, int n, vec2 uv);
// PRIM.cone : base en b, rayon rad, hauteur h, n côtés, tourné de yaw
void cone(Geo& g, vec3 b, float rad, float h, int n, vec2 uv, float yaw = 0);

// ---------- système de carte ----------
class CarteDecor : public System {
public:
    void init(Renderer& r, const GameView& g) override;
    bool remplaceSol(const GameView&) const override { return active_; }
    void ambiance(FrameParams& fp, const GameView& g) override;
    void render(Renderer& r, const GameView& g) override;
    void shutdown(Renderer& r) override;

protected:
    // identifiant de la carte (MAPS[].id), palette (codes du jeu web) et ses cases d'écorce
    virtual const char* carteId() const = 0;
    virtual const uint32_t* palette(int& n) const = 0;
    // envFor (maps.js) : brouillard de jour (couleur, fogFar m) ; nuit : horizon, brouillard, fogFar, hémisphère
    struct Env { uint32_t fogJour; float farJour; uint32_t horizonNuit, fogNuit; float farNuit; uint32_t hemiNuit; float hemiI; };
    virtual Env env() const = 0;
    // profil des bas-côtés (GROUND) de la zone au segment a, côté side (±1) ; renvoie le nombre de bandes
    virtual int profil(const char* zone, int64_t a, int side, Band out[8]) const = 0;
    // placement de tout le décor (generateDecor), une fois
    virtual void place(Track& t) = 0;
    // objets du morceau [a0, a1) (appelé en parallèle : lit seulement la piste et les données de placement)
    virtual void objets(Track& t, int64_t a0, int64_t a1, bool lod, Geo out[P_COUNT]) = 0;
    // chaussée propre (circuit : texture roadCircuit) : false = celle de Game
    virtual bool routePropre() const { return false; }
    virtual void initCarte(Renderer&) {}
    virtual void shutdownCarte(Renderer&) {}
    // dessins animés après les morceaux (rotors des éoliennes) ; c0..c1 = morceaux chargés
    virtual int animes(Renderer&, const GameView&, int64_t, int64_t) { return 0; }

    // hauteur du sol (groundY du jeu web) à la distance lat, d'après le profil
    float solY(const char* zone, int64_t a, int side, float lat) const;
    vec2 uv(int c) const { return {(c + 0.5f) / PAL_W, 0.5f}; }
    veg::Ecorce ecorce(int c0, int c1) const { return {uv(c0).x, uv(c1).x}; }
    // segment absolu du morceau [a0, a1) qui porte l'objet du segment seg (la piste se répète)
    bool dansMorceau(int64_t seg, int64_t a0, int64_t a1, int64_t len, int64_t& abs) const;

    // DECOR.tree (life.js) pour la campagne et le circuit : feuillu (chêne), sapin, pin, cyprès, sous-bois
    enum Kind { K_ROUND, K_PINE, K_CYPRESS };
    struct Tree { int64_t seg; int side; float lat, h; Kind kind; float along; };
    // cases d'écorce : chêne (2 cases voisines), sapin, pin + pin bas (voisines), pin bas seul
    int cOak_ = 0, cFir_ = 0, cPine_ = 0, cPineLow_ = 0;
    void arbre(Track& t, const Tree& it, int64_t a, bool lod, Geo out[P_COUNT]);
    // DECOR.verge : touffes d'herbe des bas-côtés (aucun tirage du décor)
    void touffes(Track& t, int64_t a, const char* zone, Geo out[P_COUNT]);

    bool active_ = false, night_ = false;
    Track* track_ = nullptr;
    Material mats_[P_COUNT];
    Image palTex_{}, leafTex_{}, routeTex_{};
    int lastDraws_ = 0;

private:
    void buildChunk(Track& t, int64_t c, bool lod, Geo out[P_COUNT]);
    struct Chunk { std::array<Mesh, P_COUNT> m{}; bool lod = false; };
    std::map<int64_t, Chunk> chunks_;
    std::vector<std::pair<uint64_t, Mesh>> graveyard_;
    uint64_t frame_ = 0;
};

} // namespace carte

// banc d'essai (WFS-53) : captures carte-campagne-* et carte-circuit-* (src/world/cartes_banc.cpp)
void benchCartes(Game& g, Renderer& r, const std::string& out);

} // namespace wfs
