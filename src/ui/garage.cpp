// WaForSpeed — garage (WFS-11, WFS-40) : voir garage.h.
#include "garage.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/cars.h"
#include "../game/commandes_tactiles.h"   // WFS-99 : dessin2d (biseaux d'un seul tenant, traits lissés)
#include "../game/game.h"
#include "../game/sauvegarde.h"   // WFS-24 : voiture et peintures gardées
#include "../game/course5.h"   // WFS-115 : « Retour » du parcours de course = panneau des modes
#include "../render/model_loader.h"
#include "../render/renderer.h"
#include "text.h"
#include "appui.h"   // WFS-113
#include "../platform/manette.h"   // WFS-22
#include "../audio/sfx.h"   // WFS-96 : son du changement de voiture

namespace wfs {
namespace ecrans {
namespace {
bool g_regarder = false;   // WFS-96 : garageFor = 'look'

// jetons :root du jeu web (codes sRGB) : css/style.css
vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SURFACE2 = 0x1E2130,
               ON_ACCENT = 0x0A0B10, TAG = 0xC9CDDA, BG = 0x0A0B10;
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.08f};
// WFS-99 (RG-05, ANO-33) : cap de départ 0,6 + π, voiture vue de 3/4 avant (gYaw = 0,6 de garage.js la montrait de l'arrière)
constexpr float CAP_DEPART = 0.6f + PI;
// WFS-99 (RG-04) : intensité du projecteur de contre-jour, 0,25 × celle des projecteurs du diffus (1,6)
constexpr float CONTRE_JOUR = 0.25f * 1.6f;
// couleur 0xRRGGBB → linéaire (même conversion que la peinture des voitures, game.cpp)
vec3 lin(unsigned h) {
    auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); };
    return {c(16), c(8), c(0)};
}
// couleur 0xRRGGBB → valeur HDR qui ressort à l'écran avec cette couleur (fond uni, comme bench_spots.cpp)
vec3 fondUni(unsigned h, float exposure) { return lin(h) * (1.f / (0.214f * exposure)); }

// PAINTS de js/cars/catalog.js (mêmes hex)
struct Peinture { const char* id; const char* nom; unsigned hex; };
const Peinture PAINTS[] = {
    {"blanc", "Blanc", 0xc9ccd2}, {"noir", "Noir", 0x0f1014}, {"gris", "Gris", 0x7b808a}, {"rouge", "Rouge", 0xb3122b},
    {"bleu", "Bleu", 0x1d3f8f}, {"orange", "Orange", 0xff6a00}, {"jaune", "Jaune", 0xc8940a}, {"vert", "Vert", 0x1f5a3c},
    {"nacre", "Blanc nacré", 0xe6e2d6}, {"bleunuit", "Bleu nuit", 0x141c3a}, {"anglais", "Vert anglais", 0x0f3324},
    {"bordeaux", "Bordeaux", 0x5a0f1e}, {"sable", "Sable", 0xb39a72}, {"violet", "Violet", 0x4b2380},
    {"turquoise", "Turquoise", 0x0f8f8c}, {"taxi", "Jaune taxi", 0xe8b400},
};
constexpr int NB_PEINTURES = sizeof(PAINTS) / sizeof(PAINTS[0]);
int peintureParId(const char* id) { for (int i = 0; i < NB_PEINTURES; i++) if (std::strcmp(PAINTS[i].id, id) == 0) return i; return 0; }

// champs de CARS (catalog.js) utilisés seulement par le garage : type (body), accroche (tag), néon (glow), peinture par défaut
struct Fiche { const char* id; const char* body; const char* tag; unsigned glow; const char* paint; };
const Fiche FICHES[] = {
    {"golf", "Compacte", "La compacte sportive de référence : équilibrée et précise.", 0x00E5FF, "bleu"},
    {"p208", "Citadine", "Citadine vive et légère : idéale pour se faufiler dans le trafic.", 0xFF6A00, "jaune"},
    {"abarth", "Mini sportive", "Petite bombe italienne : minuscule, nerveuse, très agile.", 0xFF6A00, "rouge"},
    {"tcross", "SUV compact", "SUV compact facile à conduire, stable dans les chocs.", 0xB6FF00, "orange"},
    {"compass", "SUV", "SUV robuste : haut, lourd, solide quand ça frotte.", 0xB6FF00, "vert"},
    {"s3", "Berline", "Berline compacte 4 roues motrices : rapide et sûre.", 0x00E5FF, "gris"},
    {"x5", "Grand SUV", "Grand SUV premium : puissant mais lourd dans les virages.", 0x00E5FF, "blanc"},
    {"giulia", "Berline sport", "Berline italienne de 510 ch : vitesse de pointe énorme.", 0xFF1F4B, "rouge"},
    {"m4", "Coupé", "Coupé de course pour la route : brutal en accélération.", 0xFF6A00, "noir"},
    {"p911", "Sportive", "La sportive absolue : 4 roues motrices, freins et virages parfaits.", 0x00E5FF, "gris"},
    {"a45", "Compacte sportive", "Compacte de 381 ch à 4 roues motrices : une fusée de poche.", 0x00E5FF, "gris"},
    {"model3", "Berline électrique", "Électrique : couple instantané, silence et grosse accélération.", 0x00E5FF, "blanc"},
    {"corsa", "Citadine", "Citadine légère et maniable : parfaite pour les petites rues.", 0xB6FF00, "jaune"},
    {"gt86", "Coupé", "Petit coupé propulsion : léger, joueur et facile à faire glisser.", 0xFF1F4B, "rouge"},
    {"elantra", "Berline sport", "Berline coréenne affûtée sur circuit : vive et précise.", 0x00E5FF, "bleu"},
    {"v60", "Break sport", "Break suédois de 350 ch : rapide, sûr et stable.", 0x00E5FF, "bleu"},
    {"mustang", "Muscle car", "V8 de 760 ch : la plus puissante, mais lourde à placer.", 0xFF6A00, "bleu"},
    {"rrsport", "Grand SUV", "SUV de luxe massif : imposant dans les chocs, lourd en virage.", 0xB6FF00, "noir"},
    {"gallardo", "Supercar cabriolet", "V10 à ciel ouvert : 4 roues motrices et plus de 320 km/h.", 0xB6FF00, "jaune"},
    {"sf90", "Hypercar hybride", "Hybride de 1030 ch : la plus rapide du garage, à manier avec soin.", 0xFF1F4B, "rouge"},
};
const Fiche& ficheDe(const char* id) { for (const auto& f : FICHES) if (std::strcmp(f.id, id) == 0) return f; return FICHES[0]; }

// carStats(c) du jeu web : notes sur 10
struct Stats { int v, a, m, p; };
Stats carStats(const CarDef& c) {
    const CarSpec& s = c.spec;
    auto rnd = [](float x) { return (int)std::floor(x + 0.5f); };
    return {rnd(clampN((s.vmax - 170) / 14.f, 1, 10)), rnd(clampN(13 - s.s100 * 1.05f, 1, 10)), c.grip, rnd(clampN(s.ch / 55.f, 1, 10))};
}
int indexDe(const CarDef* c) { for (int i = 0; i < CAR_COUNT; i++) if (c && &CARS[i] == c) return i; return 0; }
std::string deuxChiffres(int n) { return (n < 10 ? "0" : "") + std::to_string(n); }
std::string virgule(float v) { std::string s = ui::decimal(v, 1); return s; }

// zones touchables posées à la dernière image
struct Zone {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};
struct Zones { Zone prev, next, choisir, retour, peintures[NB_PEINTURES], points[CAR_COUNT]; } g_z;

// parallélogramme (clip-path / skewX du web) d'un seul tenant, haut décalé à droite de `biais` (WFS-99, ANO-20 / ANO-33 :
// plus de bandes qui se chevauchent, donc ni rayures ni escalier)
void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c) {
    dessin2d::biseau(r, x, y, w, h, biais, dessin2d::uni(c));
}
// dégradé linéaire d'un seul uiQuad sur le rectangle (x0, y0)-(x1, y1) : c0 → c1 de gauche à droite (horizontal)
// ou de haut en bas (WFS-99 : voiles du garage sans bandes)
void degrade(Renderer& r, float x0, float y0, float x1, float y1, const vec4& c0, const vec4& c1, bool horizontal) {
    const vec4& hg = c0; const vec4& hd = horizontal ? c1 : c0; const vec4& bd = c1; const vec4& bg = horizontal ? c0 : c1;
    const UiVertex q[4] = {{x0, y0, -1, -1, hg.x, hg.y, hg.z, hg.w}, {x1, y0, -1, -1, hd.x, hd.y, hd.z, hd.w},
                           {x1, y1, -1, -1, bd.x, bd.y, bd.z, bd.w}, {x0, y1, -1, -1, bg.x, bg.y, bg.z, bg.w}};
    r.uiQuad(q);
}

// disque horizontal (CircleGeometry couché) ou cylindre plat (dessus + tour) : repris de bench_spots.cpp (WFS-45)
Mesh disque(Renderer& r, float rayon, float y, float hauteur, int seg) {
    std::vector<Vertex> v; std::vector<uint32_t> idx;
    v.push_back({0, y, 0, 0, 1, 0, 0, 0});
    for (int i = 0; i <= seg; i++) {
        const float a = (float)i / seg * 2 * PI;
        v.push_back({std::cos(a) * rayon, y, -std::sin(a) * rayon, 0, 1, 0, std::cos(a) * rayon, std::sin(a) * rayon});
    }
    for (int i = 0; i < seg; i++) idx.insert(idx.end(), {0u, (uint32_t)i + 1, (uint32_t)i + 2});
    if (hauteur > 0) {
        const uint32_t k = (uint32_t)v.size();
        for (int i = 0; i <= seg; i++) {
            const float a = (float)i / seg * 2 * PI, cx = std::cos(a), sz = -std::sin(a);
            v.push_back({cx * rayon, y, sz * rayon, cx, 0, sz, 0, 0});
            v.push_back({cx * rayon, y - hauteur, sz * rayon, cx, 0, sz, 0, 1});
        }
        for (uint32_t i = 0; i < (uint32_t)seg; i++) { const uint32_t a = k + i * 2; idx.insert(idx.end(), {a, a + 1, a + 3, a, a + 3, a + 2}); }
    }
    return r.makeMesh(v, idx);
}
// TorusGeometry(R, r, segTube, segAnneau) couché (rotation.x = π/2), à la hauteur y
Mesh tore(Renderer& r, float R, float rt, int st, int sa, float y) {
    std::vector<Vertex> v; std::vector<uint32_t> idx;
    for (int i = 0; i <= sa; i++) for (int j = 0; j <= st; j++) {
        const float u = (float)i / sa * 2 * PI, w = (float)j / st * 2 * PI;
        const float cx = std::cos(u), cz = std::sin(u), nx = std::cos(w) * cx, ny = std::sin(w), nz = std::cos(w) * cz;
        v.push_back({(R + rt * std::cos(w)) * cx, y + rt * std::sin(w), (R + rt * std::cos(w)) * cz, nx, ny, nz, 0, 0});
    }
    for (int i = 0; i < sa; i++) for (int j = 0; j < st; j++) {
        const uint32_t a = i * (st + 1) + j, b = a + st + 1;
        idx.insert(idx.end(), {a, b, a + 1, b, b + 1, a + 1});
    }
    return r.makeMesh(v, idx);
}
// BoxGeometry(w, h, d) centrée
Mesh boite(Renderer& r, float w, float h, float d) {
    std::vector<Vertex> v; std::vector<uint32_t> idx;
    const float X = w / 2, Y = h / 2, Z = d / 2;
    const float f[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (auto& n : f) {
        const vec3 N{n[0], n[1], n[2]}, U = std::abs(n[1]) > 0 ? vec3{1, 0, 0} : vec3{0, 1, 0}, V = cross(N, U);
        const uint32_t k = (uint32_t)v.size();
        for (int s = 0; s < 4; s++) {
            const float a = (s & 1) ? 1.f : -1.f, b = (s & 2) ? 1.f : -1.f;
            const vec3 p{N.x * X + (U.x * a + V.x * b) * X, N.y * Y + (U.y * a + V.y * b) * Y, N.z * Z + (U.z * a + V.z * b) * Z};
            v.push_back({p.x, p.y, p.z, N.x, N.y, N.z, 0, 0});
        }
        idx.insert(idx.end(), {k, k + 1, k + 3, k, k + 3, k + 2, k, k + 3, k + 1, k, k + 2, k + 3});   // deux faces : sens sans importance
    }
    return r.makeMesh(v, idx);
}

class Garage : public System {
public:
    const char* name() const override { return "garage"; }

    void init(Renderer& r, const GameView&) override {
        r_ = &r;
        sol_ = disque(r, 30.f, 0.f, 0.f, 64); plateau_ = disque(r, 3.6f, 0.08f, 0.08f, 64);
        anneau_ = tore(r, 3.62f, 0.035f, 8, 96, 0.08f); barre_ = boite(r, 0.08f, 9.f, 0.08f);
        // WFS-99 (RG-03, ANO-23) : sol mat (#1A1C24, rugosité 0,6, métal 0,2) au lieu du miroir de garage.js
        // (#101219, 0,22, 0,7) : le diffus des 3 projecteurs y dessine ses taches orange, cyan et blanche
        Material ms; ms.color = {lin(0x1a1c24).x, lin(0x1a1c24).y, lin(0x1a1c24).z, 1}; ms.rough = 0.6f; ms.metal = 0.2f;
        Material mp; mp.color = {lin(0x161925).x, lin(0x161925).y, lin(0x161925).z, 1}; mp.rough = 0.4f; mp.metal = 0.6f;
        matSol_ = r.makeMaterial(ms, nullptr, nullptr, nullptr); matPlateau_ = r.makeMaterial(mp, nullptr, nullptr, nullptr);
        matSol_.flags |= MAT_DOUBLE; matPlateau_.flags |= MAT_DOUBLE;
        // MeshBasicMaterial (non éclairé) : couleur noire, tout en émission
        for (int i = 0; i < 3; i++) {
            Material me; me.color = {0, 0, 0, 1}; me.rough = 1; me.metal = 0;
            me.emissive = lin(i == 0 ? 0x00e5ff : i == 1 ? 0xff6a00 : 0x00b8d4) * 2.2f;
            matNeon_[i] = r.makeMaterial(me, nullptr, nullptr, nullptr); matNeon_[i].flags |= MAT_DOUBLE;
        }
        for (int i = 0; i < CAR_COUNT; i++) peinture_[i] = peintureParId(ficheDe(CARS[i].id).paint);
        for (int i = 0; i < CAR_COUNT; i++) if (const PeintureDef* p = sauvegarde().peintureChoisie(CARS[i].id)) peinture_[i] = peintureParId(p->id);   // WFS-24
    }

    void shutdown(Renderer& r) override {
        r.vk().waitIdle();
        r.destroy(sol_); r.destroy(plateau_); r.destroy(anneau_); r.destroy(barre_);
        if (voiture_.ok) r.destroy(voiture_);
        charge_ = -1;
    }

    void onEcran(Ecran ancien, Ecran nouveau, const GameView& g) override {
        if (nouveau == Ecran::Garage) {   // openGarage : voiture de la partie, plateau au cap de départ, panneau qui glisse
            appui_.vider();   // WFS-113 : aucun doigt retenu d'un écran précédent
            bouton_ = 0; doigtBouton_ = -1;   // WFS-115
            index_ = indexDe(g.car); yaw_ = CAP_DEPART; slide_ = 0; t0_ = -1; doigt_ = -1;
            charger(index_);
            WFS_LOG("garage : %s %s (%s / %d), peinture %s", CARS[index_].brand, CARS[index_].model, deuxChiffres(index_ + 1).c_str(),
                    CAR_COUNT, PAINTS[peinture_[index_]].nom);
        }
        if (ancien == Ecran::Garage) {
            g_z = Zones{};
            liberer();   // mémoire rendue pendant la course
            // « Choisir » : Garage → Menu → Course, appliqués avant l'image suivante (le menu n'est jamais dessiné) ;
            // l'entrée en course depuis le menu remet la course à zéro (même chemin que « Recommencer » de la pause)
            if (nouveau == Ecran::Menu && choisi_ && !g_regarder) { g.allerA(Ecran::Terrain); }   // WFS-41 : étape 2, le terrain
            // WFS-115 (RG-03) : « Retour » du garage ouvert par le parcours de course = un seul cran, le panneau des modes
            // (sans sauvegarde, banc d'essai, ouvrirChoixMode ne fait rien : le menu, comme avant)
            if (nouveau == Ecran::Menu && !choisi_ && !g_regarder && retour_) ouvrirChoixMode();
            retour_ = false;
            choisi_ = g_regarder = false;   // WFS-96 : mode regarder → on reste au menu
        }
    }

    void update(double dt, const GameView& g) override {
        if (g.ecran != Ecran::Garage) return;
        if (doigt_ < 0) yaw_ += (float)dt * 0.35f;                       // renderGarage : la voiture tourne seule
        slide_ *= std::exp(-(float)dt * 7); if (std::abs(slide_) < 0.002f) slide_ = 0;   // arrivée en glissant sur le côté
    }

    bool scene(FrameParams& fp, SceneDediee& s, const GameView& g) override {
        if (g.ecran != Ecran::Garage) return false;
        const CarDef& car = CARS[index_];
        const float dist = 1.9f * car.d.L + 1.f;   // renderGarage en paysage large : 1,9 × L + 1
        const vec3 eye{std::sin(0.55f) * dist, 2.3f, std::cos(0.55f) * dist}, look{0, -0.35f, 0};
        fp.view = lookAt(eye, look, {0, 1, 0});
        fp.proj = perspectiveVk(36.f * PI / 180.f, g.uiWidth / std::max(1.f, g.uiHeight), 0.1f);
        fp.camPos = eye; fp.exposure = 0.8f;   // GARAGE_GRADE.exposure
        const vec3 bg = fondUni(0x07070c, fp.exposure);   // scene.background et Fog(0x07070c, 14, 40)
        fp.skyTop = fp.skyHorizon = fp.ground = fp.fogColor = bg;
        fp.fogDensity = 0; fp.fogNear = 14; fp.fogFar = 40;
        fp.sunIntensity = 0; fp.ambient = 1; fp.shadows = false; fp.night = false;
        fp.hemiSky = lin(0x5060a0); fp.hemiGround = lin(0x050508); fp.hemiIntensity = 0.6f;
        const FrameParams::Spot spots[3] = {
            {{-6, 8, 4}, {0, 0, 0}, lin(0xff8a3a), 1.6f, 30, 0.6f, 0.6f, 1.2f},
            {{6, 8, -3}, {0, 0, 0}, lin(0x40e0ff), 1.6f, 30, 0.6f, 0.6f, 1.2f},
            {{0, 10, 8}, {0, 0, 0}, lin(0xffffff), 1.4f, 30, 0.5f, 0.7f, 1.2f},
        };
        for (int i = 0; i < 3; i++) fp.spots[i] = spots[i];
        // WFS-99 (RG-04, ANO-23) : 4e projecteur cyan en contre-jour, derrière la voiture à l'opposé de la caméra :
        // liseré sur la carrosserie (une voiture noire se détache du fond noir), force 0,25 × le diffus des projecteurs
        fp.spots[3] = {{-std::sin(0.55f) * 7.f, 3.2f, -std::cos(0.55f) * 7.f}, {0, 0.6f, 0}, lin(0x00e5ff), CONTRE_JOUR, 30, 0.6f, 0.6f, 1.2f};
        fp.spotCount = 4;
        s.route = false; s.joueur = false; s.commandes = false; s.cible = look;
        return true;
    }

    void render(Renderer& r, const GameView& g) override {
        if (g.ecran != Ecran::Garage) return;
        r.draw(sol_, matSol_, mat4::identity());
        r.draw(plateau_, matPlateau_, mat4::identity());
        // anneau néon de la couleur glow de la voiture (gRing.material.color.set(myCar.glow))
        // Renderer::draw garde un pointeur vers le matériau jusqu'à endFrame : matériau membre, pas local
        matAnneau_ = matNeon_[0]; matAnneau_.emissive = lin(ficheDe(CARS[index_].id).glow) * 2.2f;
        r.draw(anneau_, matAnneau_, mat4::identity());
        for (int i = 0; i < 9; i++) {   // 9 barres lumineuses en arc derrière la voiture
            // WFS-99 (RG-02, ANO-22) : arc de garage.js (rayon 16, centre z = 4) tourné vers la droite et resserré
            // (−0,9 + i × 0,32 → −0,36 + i × 0,18) : toute barre visible tombe à x ≥ 0,53 de l'écran (derrière la
            // voiture ou à droite), jamais sur le titre, l'accroche ni la barre d'index ; 4 barres visibles en 16/9
            const float a = -0.36f + i * 0.18f;
            r.draw(barre_, matNeon_[i % 3 == 1 ? 1 : 2], translate({std::sin(a) * 16, 4.5f, -std::cos(a) * 16 + 4}));
        }
        if (voiture_.ok) {   // RG-06 : que des vraies voitures, pas de silhouette provisoire
            const vec3 p = lin(PAINTS[peinture_[index_]].hex);
            r.draw(voiture_, translate({slide_ * 7, 0.08f, 0}) * rotateY(yaw_), &p);
        }
    }

    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran != Ecran::Garage) return false;
        // WFS-113 : un doigt = un appui. Le doigt qui a actionné un bouton est ignoré jusqu'à ce qu'il se relève
        // (sur Android, ses micro-mouvements arrivent aussi comme des appuis : la flèche sautait des voitures)
        if (appui_.consomme(t.id, t.down)) return true;
        // WFS-115 : « Choisir » et « Retour » agissent au relâcher, sur le bouton appuyé : le doigt est levé quand l'écran
        // change, ses mouvements n'atteignent jamais l'écran suivant (ils y arrivaient comme des appuis neufs)
        if (t.id == doigtBouton_) {
            if (t.down) return true;
            const int b = bouton_; bouton_ = 0; doigtBouton_ = -1;
            if (b == 1 && g_z.choisir.contient(t.x, t.y)) choisir(g);
            else if (b == 2 && g_z.retour.contient(t.x, t.y)) { WFS_LOG("garage : « Retour »"); choisi_ = false; retour_ = true; g.allerA(Ecran::Menu); }
            return true;
        }
        if (t.down) {
            if (doigt_ >= 0 && t.id == doigt_) {   // glisser : tourner la voiture (0,01 rad par pixel)
                if (glisse_) yaw_ = yaw0_ + (t.x - x0_) * 0.01f * (720.f / g.uiHeight);
                return true;
            }
            if (doigt_ >= 0) return true;
            // boutons : action à l'appui (comme les boutons de la pause)
            if (g_z.prev.contient(t.x, t.y)) { appui_.prendre(t.id); cycle(-1); return true; }
            if (g_z.next.contient(t.x, t.y)) { appui_.prendre(t.id); cycle(1); return true; }
            for (int i = 0; i < CAR_COUNT; i++) if (g_z.points[i].contient(t.x, t.y)) { appui_.prendre(t.id); if (i != index_) aller(i, i > index_ ? 1 : -1); return true; }
            for (int i = 0; i < NB_PEINTURES; i++)
                if (g_z.peintures[i].contient(t.x, t.y)) { appui_.prendre(t.id); peinture_[index_] = i; WFS_LOG("garage : peinture %s", PAINTS[i].nom); return true; }
            if (g_z.choisir.contient(t.x, t.y)) { bouton_ = 1; doigtBouton_ = t.id; return true; }   // WFS-115 : action au relâcher
            if (g_z.retour.contient(t.x, t.y)) { bouton_ = 2; doigtBouton_ = t.id; return true; }
            doigt_ = t.id; glisse_ = true; x0_ = t.x; yaw0_ = yaw_; tAppui_ = g.time;   // gDrag
            return true;
        }
        if (t.id != doigt_) return true;
        // geste rapide (< 320 ms, > 70 px) : voiture suivante / précédente
        const float dx = (t.x - x0_) * (720.f / g.uiHeight);
        if (glisse_ && g.time - tAppui_ < 0.32 && std::abs(dx) > 70) { yaw_ = yaw0_; cycle(dx < 0 ? 1 : -1); }
        doigt_ = -1; glisse_ = false;
        return true;   // le garage garde tous les touchers
    }

    void drawUi(Renderer& r, const GameView& g) override {
        if (g.ecran != Ecran::Garage || !ui::fontReady()) return;
        if (t0_ < 0) t0_ = g.time;
        const float W = g.uiWidth, H = g.uiHeight, k = H / 720.f, t = (float)(g.time - t0_);
        const Marges& mg = g.marges;
        const float gl = mg.gauche + 16 * k, gr = mg.droite + 16 * k, gt = mg.haut + 14 * k, gb = mg.bas + 14 * k;
        const CarDef& car = CARS[index_];
        const Fiche& fi = ficheDe(car.id);
        const float op = std::clamp(t / 0.3f, 0.f, 1.f);
        // .screen--show : voile du bas (linear-gradient 0deg, .92 → 0 à 42 %), un seul dégradé (WFS-99 : sans bandes)
        degrade(r, 0, H * 0.58f, W, H, hex(BG, 0), hex(BG, 0.92f * op), false);
        // WFS-99 (RG-01, ANO-22) : voile .screen--garage à gauche (linear-gradient 100deg, rendu horizontal) :
        // .92 à 0 %, .70 à 34 %, 0 à 62 % de la largeur, sous le titre, l'accroche et la barre d'index
        degrade(r, 0, 0, W * 0.34f, H, hex(BG, 0.92f * op), hex(BG, 0.70f * op), true);
        degrade(r, W * 0.34f, 0, W * 0.62f, H, hex(BG, 0.70f * op), hex(BG, 0), true);
        // ---------- .show-head : marque · index, nom, type + accroche, points des voitures ----------
        {
            const float x = gl + std::clamp(W * 0.04f, 0.f, 64 * k);
            float y = gt + 8 * k;
            const std::string eb = ui::majuscules(car.brand) + " · " + deuxChiffres(index_ + 1) + " / " + deuxChiffres(CAR_COUNT);
            ui::text(r, x, y, 12 * k, eb, hex(ACCENT2, op), ui::GAUCHE, 0.22f, false);
            y += 22 * k;
            const float np = std::min(84 * k, std::max(40 * k, W * 0.07f * 0.8f));
            ui::text(r, x, y, np, ui::majuscules(car.model), hex(TEXTE, op), ui::GAUCHE, 0, true, true);
            y += np * 1.0f + 8 * k;
            const std::string body = ui::majuscules(fi.body);
            const float cw = ui::width(11 * k, body, 0.16f) + 20 * k;
            biseau(r, x, y, cw, 20 * k, 3.5f * k, hex(ACCENT2, op));   // .chip : fond cyan, skewX(-10deg)
            // WFS-147 (ANO-47) : libellé calé au pixel entier (en 720p, le « I » de « HYBRIDE » tombait entre deux pixels)
            ui::text(r, std::round(x + 10 * k), std::round(y + 4 * k), 11 * k, body, hex(ON_ACCENT, op), ui::GAUCHE, 0.16f, false);
            ui::text(r, x + cw + 12 * k, y + 1 * k, 15 * k, fi.tag, hex(TAG, op), ui::GAUCHE, 0, true);
            y += 20 * k + 14 * k;
            float px = x;
            for (int i = 0; i < CAR_COUNT; i++) {   // .car-dots : 30 × 8, active 54 px cyan
                const bool on = i == index_;
                const float w = (CAR_COUNT > 10 ? (on ? 34 : 16) : (on ? 54 : 30)) * k;   // WFS-13 : 20 voitures sur une ligne
                if (on) biseau(r, px - 2 * k, y - 2 * k, w + 4 * k, 12 * k, 4 * k, hex(ACCENT2, 0.25f * op));
                biseau(r, px, y, w, 8 * k, 3 * k, on ? hex(ACCENT2, op) : hex(SURFACE2, op));
                g_z.points[i] = {px, y - 10 * k, w, 28 * k};
                px += w + 6 * k;
            }
        }
        // ---------- flèches .show-arrow (64 × 88, à 42 % de la hauteur) ----------
        for (int s = 0; s < 2; s++) {
            const float w = 64 * k, h = 88 * k, x = s == 0 ? gl : W - gr - w, y = H * 0.42f - h / 2;
            biseau(r, x, y, w, h, 12 * k, hex(0x14161F, 0.72f * op));
            const float cx = x + w / 2, cy = y + h / 2, d = s == 0 ? 1.f : -1.f, e = 3.2f * k;
            const vec2 chevron[3] = {{cx + 6 * k * d, cy - 14 * k}, {cx - 8 * k * d, cy}, {cx + 6 * k * d, cy + 14 * k}};
            dessin2d::trait(r, chevron, 3, e, hex(TEXTE, op));   // WFS-99 : trait lissé d'un seul tenant
            (s == 0 ? g_z.prev : g_z.next) = {x, y, w, h};
        }
        // ---------- .show-panel : fiche, barres, peintures, boutons ----------
        const float padX = 24 * k, padY = 18 * k;
        const float statH = 4 * 14 * k + 3 * 10 * k, paintH = 30 * k, panelH = padY * 2 + statH + 14 * k + paintH;
        const float px0 = gl, px1 = W - gr, py0 = H - gb - 26 * k - panelH;
        {   // fond vitré d'un seul tenant, coin haut droit coupé (WFS-99, RG-06 : 14 px), bord supérieur
            const float coin = 14 * k;
            const vec2 p[5] = {{px0, py0}, {px1 - coin, py0}, {px1, py0 + coin}, {px1, py0 + panelH}, {px0, py0 + panelH}};
            dessin2d::polygone(r, p, 5, dessin2d::uni({GLASS.x, GLASS.y, GLASS.z, GLASS.w * op}));
            r.uiRect(px0, py0, px1 - px0 - coin, 1 * k, {BORD.x, BORD.y, BORD.z, BORD.w * op});
        }
        float x = px0 + padX;
        const float y1 = py0 + padY;
        // .specs : Puissance (ch), 0 à 100 (s), Vitesse max (km/h)
        {
            const std::string val[3] = {std::to_string(car.spec.ch), virgule(car.spec.s100), std::to_string(car.spec.vmax)};
            const char* dt[3] = {"Puissance", "0 à 100", "Vitesse max"};
            const char* un[3] = {"ch", "s", "km/h"};
            const float sy = y1 + statH / 2 - 26 * k;
            for (int i = 0; i < 3; i++) {
                ui::text(r, x, sy, 11 * k, ui::majuscules(dt[i]), hex(MUTED, op), ui::GAUCHE, 0.16f, false);
                ui::text(r, x, sy + 16 * k, 34 * k, val[i], hex(TEXTE, op), ui::GAUCHE, 0, false, true);
                const float vw = ui::width(34 * k, val[i]);
                ui::text(r, x + vw + 4 * k, sy + 34 * k, 12 * k, ui::majuscules(un[i]), hex(MUTED, op), ui::GAUCHE, 0.1f, false);
                x += std::max(vw + 8 * k + ui::width(12 * k, ui::majuscules(un[i]), 0.1f), ui::width(11 * k, ui::majuscules(dt[i]), 0.16f)) + 26 * k;
            }
            x += 6 * k;
        }
        // boutons (colonne de droite) : Choisir (.btn orange), Retour (.btn--ghost)
        const float bw = 200 * k, bx = px1 - padX - bw;
        {
            const float hb = 52 * k, hg = 46 * k, gap = 12 * k, tot = hb + gap + hg, by = py0 + (panelH - tot) / 2;
            const bool pret = voiture_.ok;
            biseau(r, bx, by, bw, hb, 14 * k, pret ? hex(ACCENT, op) : hex(SURFACE2, op));
            ui::text(r, bx + bw / 2, by + 13 * k, 20 * k, ui::majuscules(pret ? "Choisir" : "Chargement…"), pret ? hex(ON_ACCENT, op) : hex(MUTED, op),
                     ui::CENTRE, 0.1f, false);
            g_z.choisir = {bx, by, bw, hb};
            const float gy = by + hb + gap;
            biseau(r, bx, gy, bw, hg, 14 * k, hex(SURFACE2, op));
            ui::text(r, bx + bw / 2, gy + 13 * k, 16 * k, ui::majuscules("Retour"), hex(TEXTE, op), ui::CENTRE, 0.1f, false);
            g_z.retour = {bx, gy, bw, hg};
            for (const Zone* z : {&g_z.choisir, &g_z.retour, &g_z.prev, &g_z.next}) manette::focus(*z, z == &g_z.retour);   // WFS-22
        }
        // .statlist : 4 barres sur 10 (Vitesse, Accélération, Maniabilité, Puissance)
        {
            const Stats st = carStats(car);
            const int v[4] = {st.v, st.a, st.m, st.p};
            const char* lab[4] = {"Vitesse", "Accélération", "Maniabilité", "Puissance"};
            const float sx = x, sw = bx - 32 * k - sx, barX = sx + 112 * k + 12 * k, barW = sw - 112 * k - 12 * k - 12 * k - 28 * k;
            for (int i = 0; i < 4; i++) {
                const float yy = y1 + i * 24 * k;
                ui::text(r, sx, yy, 13 * k, lab[i], hex(TAG, op), ui::GAUCHE, 0, false);
                const float by = yy + 4 * k, bh = 10 * k;
                biseau(r, barX, by, barW, bh, 1.8f * k, hex(SURFACE2, op));
                // remplissage en dégradé cyan → orange, découpé en 10 cases (mask repeating-linear-gradient)
                const float seg = barW / 10;
                for (int c = 0; c < v[i]; c++) {
                    const float u = (c + 0.5f) / 10;
                    const vec4 col{hex(ACCENT2).x + (hex(ACCENT).x - hex(ACCENT2).x) * u, hex(ACCENT2).y + (hex(ACCENT).y - hex(ACCENT2).y) * u,
                                   hex(ACCENT2).z + (hex(ACCENT).z - hex(ACCENT2).z) * u, op};
                    biseau(r, barX + c * seg, by, seg - 3 * k, bh, 1.8f * k, col);
                }
                ui::text(r, barX + barW + 12 * k + 28 * k, yy, 14 * k, std::to_string(v[i]), hex(TEXTE, op), ui::DROITE, 0, false, true);
            }
        }
        // .paint-line : « Couleur », 16 pastilles (30 × 30, skewX(-10deg)), nom de la peinture
        {
            const float yy = y1 + statH + 14 * k;
            float cx = px0 + padX;
            ui::text(r, cx, yy + 9 * k, 13 * k, ui::majuscules("Couleur"), hex(MUTED, op), ui::GAUCHE, 0.16f, false);
            cx += ui::width(13 * k, "COULEUR", 0.16f) + 16 * k;
            const float s = std::min(30 * k, (bx - 32 * k - cx - 140 * k) / NB_PEINTURES - 8 * k);
            for (int i = 0; i < NB_PEINTURES; i++) {
                const bool on = i == peinture_[index_];
                if (on) biseau(r, cx - 4 * k, yy + (paintH - s) / 2 - 4 * k, s + 8 * k, s + 8 * k, s * 0.18f, hex(ACCENT2, 0.35f * op));   // halo
                biseau(r, cx - 2 * k, yy + (paintH - s) / 2 - 2 * k, s + 4 * k, s + 4 * k, s * 0.18f, on ? hex(ACCENT2, op) : vec4{1, 1, 1, 0.08f * op});
                biseau(r, cx, yy + (paintH - s) / 2, s, s, s * 0.18f, hex(PAINTS[i].hex, op));
                g_z.peintures[i] = {cx - 4 * k, yy - 4 * k, s + 8 * k, paintH + 8 * k};
                manette::focus(g_z.peintures[i]);   // WFS-80 : pastille au focus de la manette (A = toucher)
                cx += s + 8 * k;
            }
            ui::text(r, cx + 6 * k, yy + 6 * k, 15 * k, PAINTS[peinture_[index_]].nom, hex(TEXTE, op), ui::GAUCHE, 0, false);
        }
        // .show-credit : crédit du modèle (licence obligatoire), en bas à droite ; WFS-99 (RG-06, ANO-33) : capitales de
        // 10 px à 720p (taille 14 = capitales + jambages), #8A90A6, sous le panneau
        ui::text(r, W - gr, H - gb - 17 * k, 14 * k, car.credit, hex(MUTED, op), ui::DROITE, 0, false);
        if (!voiture_.ok) {   // .show-loading
            const std::string l = "Chargement du modèle 3D…";
            const float lw = ui::width(14 * k, l, 0.08f) + 36 * k;
            r.uiRect(W / 2 - lw / 2, H * 0.44f - 22 * k, lw, 44 * k, {GLASS.x, GLASS.y, GLASS.z, GLASS.w * op});
            ui::text(r, W / 2, H * 0.44f - 8 * k, 14 * k, l, hex(TEXTE, op), ui::CENTRE, 0.08f, false);
        }
    }

    // banc d'essai : planche des 16 peintures (4 × 4 voitures sur le sol du garage)
    void dessinerPlanche(Renderer& r) {
        for (int i = 0; i < NB_PEINTURES; i++) {
            const vec3 p = lin(PAINTS[i].hex);
            if (voiture_.ok) r.draw(voiture_, translate({(i % 4 - 1.5f) * 5.2f, 0.0f, (i / 4 - 1.5f) * 2.7f}) * rotateY(PI / 2), &p);
        }
    }
    int index() const { return index_; }
    void choisirPeinture(int i) { peinture_[index_] = i; }
    bool planche = false;

private:
    void charger(int i) {
        if (charge_ == i && voiture_.ok) return;
        liberer();
        if (!loadCarModel(*r_, CARS[i].file, voiture_)) WFS_ERR("garage : modèle %s introuvable", CARS[i].file);
        charge_ = i;
    }
    void liberer() {
        if (!r_ || !voiture_.ok) return;
        r_->vk().waitIdle(); r_->destroy(voiture_); voiture_ = Model{}; charge_ = -1;
    }
    void aller(int i, int dir) {   // cycleCar / selectCar : nouvelle voiture, arrivée en glissant (gSlide = dir)
        index_ = (i + CAR_COUNT) % CAR_COUNT; slide_ = (float)dir; sfxInterface(SfxId::Garage);   // WFS-96 : selectCar
        charger(index_);
        WFS_LOG("garage : %s %s (%s / %d)", CARS[index_].brand, CARS[index_].model, deuxChiffres(index_ + 1).c_str(), CAR_COUNT);
    }
    void cycle(int dir) { aller(index_ + dir, dir); }
    // « Choisir » : voiture et peinture retenues ; parcours de course → terrain (via onEcran), Showroom → menu (WFS-96)
    void choisir(const GameView& g) {
        if (!voiture_.ok) return;   // « Chargement… » : bouton inactif
        const CarDef& c = CARS[index_];
        WFS_LOG("garage : « Choisir » %s %s, peinture %s", c.brand, c.model, PAINTS[peinture_[index_]].nom);
        if (g.choisirVoiture) g.choisirVoiture(c.id, lin(PAINTS[peinture_[index_]].hex));
        if (Sauvegarde& s = sauvegarde(); s.actif()) { s.car = c.id; s.paint[c.id] = PAINTS[peinture_[index_]].id; s.enregistrer(); }   // WFS-24
        choisi_ = true; g.allerA(Ecran::Menu);
    }

    Renderer* r_ = nullptr;
    Mesh sol_, plateau_, anneau_, barre_;
    Material matSol_, matPlateau_, matNeon_[3], matAnneau_;
    Model voiture_; int charge_ = -1;
    int index_ = 0, peinture_[CAR_COUNT] = {};
    float yaw_ = CAP_DEPART, slide_ = 0, yaw0_ = 0, x0_ = 0;
    double t0_ = -1, tAppui_ = 0;
    int doigt_ = -1; bool glisse_ = false, choisi_ = false;
    int bouton_ = 0, doigtBouton_ = -1; bool retour_ = false;   // WFS-115 : bouton appuyé (1 Choisir, 2 Retour) et son doigt
    ui::FiltreAppui appui_;   // WFS-113 : un doigt = un appui
};
Garage* g_garage = nullptr;
} // namespace
void garageRegarder() { g_regarder = true; }
namespace {

class GarageEnregistre : public Garage {
public:
    GarageEnregistre() { g_garage = this; }
    ~GarageEnregistre() override { if (g_garage == this) g_garage = nullptr; }
};

} // namespace

bool garageBouton(const char* nom, float& x, float& y) {
    const Zone* z = std::strcmp(nom, "choisir") == 0 ? &g_z.choisir : std::strcmp(nom, "retour") == 0 ? &g_z.retour
                  : std::strcmp(nom, "suivant") == 0 ? &g_z.next : std::strcmp(nom, "precedent") == 0 ? &g_z.prev : nullptr;
    if (!z || z->w <= 0) return false;
    x = z->x + z->w / 2; y = z->y + z->h / 2;
    return true;
}

void benchGarage(Game& g, Renderer& r, const std::string& out, const std::string& suf) {
    ui::chargerPolice(r);   // déjà chargée après l'écran de chargement ; nécessaire en WFS_BANC=garage
    if (!g.init(r, "golf", "ville")) { WFS_ERR("garage : chargement impossible"); return; }
    g.setAutopilot(false); g.camMode = 0;
    double t = 900;
    int n = 0;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; if (++n % 10 == 0) g.render(t); };
    auto capture = [&](const std::string& nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + suf + ".png");
        WFS_LOG("capture %s%s : écran %s, %s", nom.c_str(), suf.c_str(), nomEcran(g.ecran()), quoi);
    };
    auto toucher = [&](int id, float x, float y) { g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); image(); };
    for (int i = 0; i < 30; i++) g.update(1.0 / 60);
    g.allerA(Ecran::Garage);
    for (int i = 0; i < 40; i++) image();   // panneau posé
    if (!g_garage) { WFS_ERR("garage : système absent"); g.shutdown(); return; }
    const float H = (float)r.height(), W = (float)r.width();
    // garage-0/1/2 : 3 angles de la Golf (le plateau tourne seul : 0,35 rad/s ; glisser = tourner)
    for (int k = 0; k < 3; k++) {
        if (k > 0) {   // glisser lent de 180 px (0,01 rad / px) sur la voiture
            const float x0 = W * 0.4f, y0 = H * 0.5f, d = 180 * H / 720;
            g.onTouch({9, x0, y0, true}); image();
            for (int s = 1; s <= 30; s++) { g.onTouch({9, x0 + d * s / 30, y0, true}); image(); }
            g.onTouch({9, x0 + d, y0, false}); image();
        }
        capture("garage-" + std::to_string(k), k == 0 ? "Golf sur le plateau, fiche complète" : "autre angle (glisser)");
    }
    // garage-peinture-0/1 : deux peintures différentes (rouge, puis turquoise)
    g_garage->choisirPeinture(peintureParId("rouge")); for (int i = 0; i < 12; i++) image();
    capture("garage-peinture-0", "Golf en Rouge");
    g_garage->choisirPeinture(peintureParId("turquoise")); for (int i = 0; i < 12; i++) image();
    capture("garage-peinture-1", "Golf en Turquoise");
    // garage-m4 : geste rapide vers la gauche (< 320 ms, > 70 px) → voiture suivante… jusqu'à la M4
    float x, y;
    while (g_garage && std::strcmp(CARS[g_garage->index()].id, "m4") != 0) {
        g.onTouch({10, W * 0.6f, H * 0.5f, true}); g.update(1.0 / 60); t += 1.0 / 60; g.render(t);
        g.onTouch({10, W * 0.6f - 160 * H / 720, H * 0.5f, true}); g.update(1.0 / 60); t += 1.0 / 60; g.render(t);
        g.onTouch({10, W * 0.6f - 160 * H / 720, H * 0.5f, false});
        for (int i = 0; i < 8; i++) image();
        if (g_garage->index() == 0) break;   // tour complet sans M4
    }
    for (int i = 0; i < 60; i++) image();   // fin du glissement latéral
    capture("garage-m4", "BMW M4, nom et index à jour (geste rapide)");
    // flèche « suivant » : Porsche 911
    if (garageBouton("suivant", x, y)) { toucher(11, x, y); for (int i = 0; i < 60; i++) image(); capture("garage-suivant", "flèche suivante"); }
    // planche des 16 peintures (Golf)
    if (garageBouton("precedent", x, y)) { toucher(12, x, y); toucher(13, x, y); for (int i = 0; i < 30; i++) image(); }
    {
        FrameParams fp; SceneDediee s; GameView v; v.ecran = Ecran::Garage; v.uiWidth = W; v.uiHeight = H;
        g_garage->scene(fp, s, v);
        const vec3 eye{0, 17, 9}, look{0, 0, 0.3f};
        fp.view = lookAt(eye, look, {0, 1, 0}); fp.camPos = eye;
        fp.proj = perspectiveVk(36.f * PI / 180.f, W / H, 0.1f);
        fp.fogNear = 30; fp.fogFar = 60;
        if (r.beginFrame(fp)) { g_garage->dessinerPlanche(r); r.endFrame(); }
        if (r.beginFrame(fp)) { g_garage->dessinerPlanche(r); r.endFrame(); }
        r.capturePng(out + "/garage-peintures" + suf + ".png");
        WFS_LOG("capture garage-peintures%s : 16 peintures sur la %s", suf.c_str(), CARS[g_garage->index()].model);
    }
    // « Choisir » : la Golf en Turquoise part en course (décompte puis course)
    if (garageBouton("choisir", x, y)) {
        toucher(14, x, y);
        g.setAutopilot(true);
        for (int i = 0; i < 60 * 5; i++) image();
        g.setAutopilot(false);
        capture("garage-course", "course avec la voiture et la peinture choisies");
    }
    // « Retour » : du garage au menu sans changement
    g.allerA(Ecran::Garage); for (int i = 0; i < 30; i++) image();
    if (garageBouton("retour", x, y)) { toucher(15, x, y); for (int i = 0; i < 30; i++) image(); WFS_LOG("garage : « Retour » → écran %s", nomEcran(g.ecran())); }
    g.allerA(Ecran::Garage); for (int i = 0; i < 30; i++) image();
    // WFS-113 : 3 appuis sur « suivante », chacun avec des micro-mouvements du doigt (Android : ACTION_MOVE = appui) :
    // exactement une voiture de plus à chaque appui (showroom-suivante-1/2/3)
    for (int k = 1; k <= 3 && garageBouton("suivant", x, y); k++) {
        const int avant = g_garage ? g_garage->index() : -1;
        g.onTouch({20 + k, x, y, true});
        for (int m = 0; m < 6; m++) { g.onTouch({20 + k, x + (m % 2 ? 2.f : -2.f), y + 1.f, true}); image(); }
        g.onTouch({20 + k, x, y, false});
        for (int i = 0; i < 40; i++) image();
        capture("showroom-suivante-" + std::to_string(k), "une voiture de plus par appui malgré les micro-mouvements");
        WFS_LOG("showroom (WFS-113) : appui %d sur « suivante » → %s (%d → %d)", k, g_garage ? CARS[g_garage->index()].model : "?", avant + 1,
                g_garage ? g_garage->index() + 1 : 0);
    }
    g.shutdown();
}

// WFS-127 : voiture affichée au garage (lecture seule, son du showroom) ; nullptr si le garage n'existe pas
const char* garageVoitureAffichee() { return g_garage ? CARS[g_garage->index()].id : nullptr; }

} // namespace ecrans

WFS_SYSTEM(ecrans::GarageEnregistre, 440)   // interface (400-499) : avant le menu (450)

} // namespace wfs
