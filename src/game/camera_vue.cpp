// WaForSpeed — caméras de course (WFS-66) : voir camera_vue.h.
// Système « camera » (ordre 260) : cockpit dessiné par code (buildCockpit du jeu web) et bouton de vue en course (#camBtn).
#include "camera_vue.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../core/system.h"
#include "../render/renderer.h"
#include "../render/vehicule_cockpit_geo.h"   // WFS-197 : primitives du cockpit (déplacées depuis ce fichier)
#include "../render/vehicule_cockpit.h"       // WFS-197 : intérieurs par modèle
#include "../audio/engine_synth.h"           // WFS-197 : EngineModel (régime et rapport du combiné, comme le son)
#include "../ui/minicarte.h"                 // WFS-197 (RG-05) : découpe de la carte (minicarte::decouper, lecture)
#include "../world/meteo.h"                  // WFS-197 (RG-06) : pluieActive (lecture)
#include "../ui/text.h"
#include "cars.h"
#include "commandes_tactiles.h"   // WFS-98 : dessin2d (biseau à bords lissés)
#include "course.h"
#include "course_system.h"
#include "game.h"
#include "player.h"
#include "ressenti.h"   // WFS-70 : secousse de la caméra après un choc
#include "track.h"
#include "../platform/manette.h"   // WFS-22 : bouton de vue masqué à la manette

namespace wfs {

namespace {
VueCamera g_vue = VueCamera::Poursuite;   // store('waforspeed-cam') du jeu web : mémorisée pour la session
struct Suivi { bool init = false; VueCamera vue = VueCamera::Poursuite; vec3 eye{}, look{}; float fov = 72.f;
               double camHd = 0, distDyn = 0; } g_suivi;   // WFS-98 : camHd et camDistDyn d'updateCamera du jeu web
EtatCockpit g_ck;
float g_ckEssuieT = 0;   // WFS-197 : phase des essuie-glaces au dernier pas (banc d'essai)
cockpit::EtatInterieur g_ckInt; int g_ckAppels = 0; size_t g_ckOctets = 0; const char* g_ckCar = nullptr;   // WFS-197 : banc d'essai
struct Zone { float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; } } g_bouton;
}

VueCamera vueCamera() { return g_vue; }
void choisirVue(VueCamera v) {
    if (v == g_vue) return;
    g_vue = v; g_suivi.init = false;   // camInit = false du jeu web : la nouvelle vue part de sa position visée
    WFS_LOG("caméra : vue %s", VUES[(int)v].nom);
}
VueCamera vueSuivante() { choisirVue(vueApres(g_vue)); return g_vue; }
const EtatCockpit& etatCockpit() { return g_ck; }
bool boutonVue(float& cx, float& cy) { cx = g_bouton.x + g_bouton.w / 2; cy = g_bouton.y + g_bouton.h / 2; return g_bouton.w > 0; }

// updateCamera du jeu web, pour les vues liées à la carrosserie (C.hood) ; la poursuite reste celle de Game
static bool cameraVueBase(Track& track, const Player& pl, const CarDef& car, const mat4& carM, int camMode, Ecran ecran, float fdt,
                          vec3& eye, vec3& look, float& fov) {
    g_ck.actif = false;
    if (camMode == 2 || (ecran != Ecran::Course && ecran != Ecran::Pause)) { g_suivi.init = false; g_ck.fov = fov; return false; }
    const VueDef& V = VUES[(int)g_vue];
    if (!V.capot && camMode == 0) {
        // WFS-85 : poursuite « loin » (défaut) ou « proche » d'updateCamera du jeu web : œil derrière la voiture, regard entre
        // l'avant de la voiture et le virage qui arrive, suivi lissé, champ de vision qui s'élargit avec la vitesse.
        // WFS-98 (RG-07, ANO-30) : l'œil suit le cap de la VOITURE (cap de la route + lacet) avec retard (camHd), le recul
        // suit l'accélération (camDistDyn) ; voiture en travers (demi-tour) : recul ≥ 6 m et hauteur ≥ 2,5 m
        const Poursuite& P = g_vue == VueCamera::Proche ? POURSUITE_PROCHE : POURSUITE_LOIN;
        const double p = std::abs(pl.speed) / MAX_SPEED, lat = pl.playerX * RW;
        const vec3 carPos = carM.transformPoint({0, 0, 0});
        const double th = track.point(pl.dist, lat).th;
        const double hdT = th + pl.heading + pl.spinYaw * 0.15;   // tête-à-queue : la caméra n'en suit qu'un peu (reste stable)
        const bool neuf = !g_suivi.init || g_suivi.vue != g_vue || length(g_suivi.eye - carPos) > 25.f + P.dist;   // camInit
        if (neuf) { g_suivi.camHd = hdT; g_suivi.distDyn = P.dist; }
        double dh = hdT - g_suivi.camHd;
        dh = std::atan2(std::sin(dh), std::cos(dh));
        g_suivi.camHd += dh * std::min(1.0, fdt * 3.4);   // ≈ 0,3 s de retard : la voiture pivote d'abord dans l'image
        const double distT = P.dist + p * 0.6 + std::clamp(pl.lonAcc * 0.05, -0.4, 0.6);   // s'éloigne en accélérant
        g_suivi.distDyn += (distT - g_suivi.distDyn) * std::min(1.0, fdt * 2.5);
        const double t = std::clamp((std::abs(std::sin(pl.heading)) - 0.3) / 0.4, 0.0, 1.0), a = t * t * (3 - 2 * t);
        // demi-tour (RG-07) : recul ≥ 6 m, œil à 3 m au moins (≥ 2,5 m) et regard relevé de 0,8 m : la voiture, vue de plus
        // haut, descend dans le tiers bas de l'image (WFS-105 : planchers sans effet en Poursuite, déjà à 9 m et 6,3 m)
        const double dist = std::max(g_suivi.distDyn, g_suivi.distDyn + (6.0 - g_suivi.distDyn) * a);
        const double h = std::max((double)P.h, P.h + (3.0 - P.h) * a);
        const double hd = g_suivi.camHd, side = -pl.steer * 0.35 * std::min(1.0, p * 2);
        const vec3 f{(float)std::sin(hd), 0.f, (float)-std::cos(hd)}, droite{(float)std::cos(hd), 0.f, (float)std::sin(hd)};
        vec3 eT = carPos - f * (float)dist + droite * (float)side;
        eT.y = carPos.y + (float)(h + p * 0.12);
        // RG-03 : jamais sous la route dans les côtes (sol à la place de l'œil, devant ou derrière selon le cap)
        const float sol = (float)track.point(pl.dist - std::cos(hd - th) * dist / M, lat).y + 0.6f;
        eT.y = std::max(eT.y, sol);
        // WFS-90 : virage qui arrive dans le sens de marche (1 dans le sens de la route, −1 à contresens) ; en travers, droit
        // devant la voiture (le bord de la route n'attire plus le regard)
        const double sens = std::clamp(std::cos(pl.heading) * 3.0, -1.0, 1.0);
        const TrackPoint dev = track.point(pl.dist + sens * P.ahead * 1.6 / M, lat);
        const vec3 devant = lerp(carPos + f * (P.ahead * 1.6f), vec3{(float)dev.x, carPos.y, (float)dev.z}, (float)std::abs(sens));
        const vec3 lT{(carPos.x + f.x * P.ahead) * 0.6f + devant.x * 0.4f, carPos.y + P.lookH + 0.8f * (float)a,
                      (carPos.z + f.z * P.ahead) * 0.6f + devant.z * 0.4f};
        const float fovT = fovVue(g_vue, p, pl.nitroOn);
        if (neuf) { g_suivi.init = true; g_suivi.vue = g_vue; g_suivi.eye = eT; g_suivi.look = lT; g_suivi.fov = fovT; }
        g_suivi.eye = lerp(g_suivi.eye, eT, 1.f - std::exp(-fdt * SUIVI_OEIL));
        g_suivi.look = lerp(g_suivi.look, lT, 1.f - std::exp(-fdt * SUIVI_REGARD));
        g_suivi.fov = lisser(g_suivi.fov, fovT, fdt, 4.f);
        g_suivi.eye.y = std::max(g_suivi.eye.y, sol);
        eye = g_suivi.eye; look = g_suivi.look; fov = g_suivi.fov; g_ck.fov = fov;
        return false;
    }
    if (!V.capot) {   // vue haute du banc (camMode 1) : caméra de Game, jamais sous la route (ground = trackPoint(sP − dist).y + 0,6)
        const TrackPoint sol = track.point(pl.dist - 8.0 / M, pl.playerX * RW * 0.85);
        eye.y = std::max(eye.y, (float)sol.y + 0.6f);
        g_suivi.init = false; g_ck.fov = fov;
        return false;
    }
    const CarDims& d = car.d;
    vec3 eyeL, lookL;
    if (V.cockpit) {   // œil du conducteur (à gauche) : inner.localToWorld(−0,36 ; H × 0,74 ; −L × 0,02), regard 30 m devant
        // WFS-197 (RG-07) : voiture qui a son intérieur : hauteur d'œil du modèle (assise), même écart œil → regard
        const cockpit::InterieurDef* idef = cockpit::interieurPour(car.id);
        const float h = idef ? idef->oeilH : 0.74f;
        eyeL = {-0.36f, d.H * h, -d.L * 0.02f}; lookL = {-0.36f, d.H * (h - 0.08f), -d.L * 0.02f - 30.f};
    } else {           // capot : au milieu, devant le pare-brise, le capot reste dans le bas de l'image
        eyeL = {0.f, d.H * 0.84f, -d.L * 0.24f}; lookL = {0.f, d.H * 0.72f, -d.L * 0.24f - 30.f};
    }
    const vec3 eT = carM.transformPoint(eyeL), lT = carM.transformPoint(lookL);
    const float fovT = fovVue(g_vue, pl.speed / MAX_SPEED, pl.nitroOn);
    if (!g_suivi.init || g_suivi.vue != g_vue || length(g_suivi.eye - eT) > 25.f) {
        g_suivi = {true, g_vue, eT, lT, fovT};
        g_ck.volant = volantCible(pl.steer);
    }
    // suivi lissé (C.hood : 60 /s, indépendant de la fréquence d'affichage), champ de vision lissé à 4 /s
    const float k = 1.f - std::exp(-fdt * 60.f);
    g_suivi.eye = lerp(g_suivi.eye, eT, k); g_suivi.look = lerp(g_suivi.look, lT, k);
    g_suivi.fov = lisser(g_suivi.fov, fovT, fdt, 4.f);
    eye = g_suivi.eye; look = g_suivi.look; fov = g_suivi.fov; g_ck.fov = fov;
    if (V.cockpit) {
        g_ck.actif = true;
        g_ck.volant = lisser(g_ck.volant, volantCible(pl.steer), fdt, 14.f);
        g_ck.aiguille = aiguilleAngle(pl.kmh());
    }
    return V.cockpit;   // vue cockpit : playerMesh.visible = false (on ne voit pas sa propre voiture)
}

bool cameraVue(Track& track, const Player& pl, const CarDef& car, const mat4& carM, int camMode, Ecran ecran, float fdt,
               vec3& eye, vec3& look, float& fov) {
    const bool cache = cameraVueBase(track, pl, car, carM, camMode, ecran, fdt, eye, look, fov);
    // WFS-70 : secousse après un choc (scene.js : camera.position += décalage, puis lookAt du même point), toutes les vues
    // de course ; rien en vitrine ni hors course (aucun tirage aléatoire : image identique sans choc)
    if (camMode != 2 && ecran == Ecran::Course) eye = eye + ressentiDecalageCamera();
    return cache;
}

namespace {

// géométries construites par code (Geo, boite, anneau, plan, tore, cylindre) : src/render/vehicule_cockpit_geo.h
// (WFS-197 : partagées avec les intérieurs par modèle)
using namespace cockpit;

vec3 lin(unsigned h) {   // couleur hexadécimale → linéaire (comme hexColor de Game)
    auto l = [](float c) { return std::pow(c, 2.2f); };
    return {l(((h >> 16) & 255) / 255.f), l(((h >> 8) & 255) / 255.f), l((h & 255) / 255.f)};
}
vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
// jetons :root du jeu web (codes sRGB), comme le bouton « Menu » (WFS-39)
const unsigned TEXTE = 0xF2F4F8, BG = 0x0A0B10;
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.10f};

// parallélogramme (clip-path de .cam-btn) : haut décalé à droite de `biais` ; WFS-98 (RG-08) : un seul polygone à bords
// lissés (dessin2d), plus de bandes
void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c) {
    dessin2d::biseau(r, x, y, w, h, biais, dessin2d::uni(c));
}

// bouton visible : vraie course en cours (comme le bouton « Menu » de WFS-39 : pas de fin, pas de scène du banc sans règles)
bool boutonVisible(const GameView& g) {
    const Course* c = activeCourse();
    return c && g.ecran == Ecran::Course && !g.sceneDediee && c->state != RaceState::Over && c->timeLeft < 1e6 && !manette::actif();   // WFS-22 : masqué à la manette
}

class Camera : public System {
public:
    const char* name() const override { return "camera"; }

    void init(Renderer& r, const GameView&) override {
        // matériaux de buildCockpit (MeshStandardMaterial ; MeshBasicMaterial = sans lumière : noir + émission)
        Material m;
        m.color = {lin(0x2a2c33).x, lin(0x2a2c33).y, lin(0x2a2c33).z, 1}; m.rough = 0.7f; m.metal = 0.15f; sombre_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
        m.color = {lin(0x23242a).x, lin(0x23242a).y, lin(0x23242a).z, 1}; m.rough = 0.5f; m.metal = 0.1f; m.emissive = lin(0x08090c);
        cuir_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
        m.color = {lin(0x9aa0a8).x, lin(0x9aa0a8).y, lin(0x9aa0a8).z, 1}; m.rough = 0.3f; m.metal = 0.9f; m.emissive = {};
        alu_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
        auto basique = [&](unsigned c) { Material b; b.color = {0, 0, 0, 1}; b.rough = 1; b.metal = 0; b.emissive = lin(c); return r.makeMaterial(b, nullptr, nullptr, nullptr); };
        cadranMat_ = basique(0x0b1a22); cyan_ = basique(0x00e5ff); orange_ = basique(0xff6a00);
        const mat4 I = mat4::identity();
        Geo st, ca, an, ai, ja, br, re;
        boite(st, translate({0.32f, -0.66f, -0.9f}) * rotateX(0.16f), 3.2f, 0.3f, 1.1f);           // tableau de bord
        boite(st, translate({0.f, -0.43f, -0.8f}), 0.62f, 0.12f, 0.3f);                              // casquette du compteur
        for (auto [x, rz] : {std::pair{-1.12f, -0.1f}, std::pair{1.78f, 0.1f}})                       // montants de pare-brise
            boite(st, translate({x, 0.05f, -0.85f}) * rotateX(-0.55f) * rotateZ(rz), 0.05f, 1.3f, 0.05f);
        boite(st, translate({0.32f, 0.56f, -0.5f}), 2.6f, 0.08f, 0.5f);                              // pavillon
        anneau(ca, translate({0.f, -0.33f, -0.93f}), 0.f, 0.1f, 32);                                 // fond du compteur
        anneau(an, translate({0.f, -0.33f, -0.929f}), 0.095f, 0.104f, 40);                           // cercle cyan
        plan(ai, translate({0.f, 0.04f, 0.f}), 0.008f, 0.085f);                                      // aiguille (pivot en bas)
        tore(ja, I, 0.19f, 0.022f, 10, 40);                                                         // jante du volant
        cylindre(ja, rotateX(PI / 2), 0.055f, 0.06f, 0.04f, 20);                                     // moyeu
        for (float a : {0.f, PI * 0.62f, -PI * 0.62f}) boite(br, rotateZ(a) * translate({0.f, -0.085f, 0.f}), 0.03f, 0.17f, 0.018f);   // 3 branches
        boite(re, translate({0.f, 0.19f, 0.f}), 0.02f, 0.03f, 0.025f);                              // repère orange en haut
        for (auto [geo, mesh] : {std::pair{&st, &statique_}, {&ca, &cadran_}, {&an, &anneau_}, {&ai, &aiguille_}, {&ja, &jante_},
                                 {&br, &branches_}, {&re, &repere_}})
            *mesh = r.makeMesh(geo->v, geo->i);
    }

    // WFS-197 : modèle moteur (régime, rapport : mêmes entrées que le son, audio_system.cpp) et essuie-glaces, à chaque pas
    void update(double dt, const GameView& g) override {
        if (!g.player || !g.car) return;
        const Player& pl = *g.player;
        const double v = std::abs(pl.speed) * M;
        const bool gaz = pl.nitroOn || (!pl.braking && (pl.lonAcc > 0.4 || (v > 0.5 && pl.lonAcc > -0.6)));
        const double top = MAX_SPEED * pl.perf.top * (pl.nitroOn ? pl.perf.nitroMult : 1);
        moteur_.update(dt, pl.speed, top, MAX_SPEED, gaz, pl.nitroOn, true, engineVoiceFor(g.car->id).pitch, false, cibles_);
        essuie_.avancer((float)dt, pl.kmh(), meteo::pluieActive(g.map ? g.map->id : nullptr, nuitDemandee()));
        g_ckEssuieT = essuie_.phase;
    }

    void render(Renderer& r, const GameView& g) override {
        const EtatCockpit& ck = etatCockpit();
        if (!ck.actif || g.sceneDediee) return;
        const mat4 cam = inverseAffine(g.view);   // cockpit attaché à la caméra (camera.add(g))
        // WFS-197 (RG-01) : intérieur du modèle s'il existe (construit à la première vue Cockpit de cette voiture), sinon le
        // cockpit commun ci-dessous, inchangé
        const cockpit::InterieurDef* idef = g.car ? cockpit::interieurPour(g.car->id) : nullptr;
        if (idef != interieur_.def()) {
            if (interieur_.pret()) interieur_.detruire(r);
            if (idef) interieur_.construire(r, *idef);
        }
        if (interieur_.pret() && g.player && g.track) { dessinerInterieur(r, g, cam, ck); return; }
        r.draw(statique_, sombre_, cam);
        r.draw(cadran_, cadranMat_, cam);
        r.draw(anneau_, cyan_, cam);
        r.draw(aiguille_, orange_, cam * translate({0.f, -0.33f, -0.928f}) * rotateZ(ck.aiguille));
        // volant : position (0 ; −0,47 ; −0,62), inclinaison x −0,28, rotation z = braquage, échelle 0,88
        const mat4 vol = cam * translate({0.f, -0.47f, -0.62f}) * rotateX(-0.28f) * rotateZ(ck.volant) * scale({0.88f, 0.88f, 0.88f});
        r.draw(jante_, cuir_, vol);
        r.draw(branches_, alu_, vol);
        r.draw(repere_, orange_, vol);
    }

    // #camBtn : sous le bouton « Menu » de la course (WFS-39), même style (.cam-btn), libellé = nom de la vue
    void drawUi(Renderer& r, const GameView& g) override {
        if (!boutonVisible(g) || !ui::fontReady()) { g_bouton = {}; return; }
        const float k = g.uiHeight / 720.f, h = 40 * k, ic = 18 * k, pad = 14 * k, gap = 6 * k, tp = 14 * k;
        const std::string t = ui::majuscules(VUES[(int)vueCamera()].nom);
        const float w = pad + ic + gap + ui::width(tp, t, 0.1f) + pad;
        const float x = g.marges.gauche + 16 * k, y = g.marges.haut + 14 * k + 112 * k + 10 * k + h + 8 * k;   // .hud-btns gap 8 px
        g_bouton = {x, y, w, h};
        biseau(r, x - k, y - k, w + 2 * k, h + 2 * k, 8 * k, BORD);
        biseau(r, x, y, w, h, 8 * k, GLASS);
        const vec4 c = hex(TEXTE);
        const float ix = x + pad, iy = y + (h - ic) / 2, u = ic / 24;   // icône appareil photo (path de #camBtn, 24 × 24)
        r.uiRect(ix + 3 * u, iy + 7 * u, 18 * u, 12 * u, c);
        r.uiRect(ix + 8.6f * u, iy + 5 * u, 6.8f * u, 2 * u, c);
        r.uiCircle(ix + 12 * u, iy + 13.5f * u, 3.3f * u, hex(BG, 0.9f));
        ui::text(r, std::round(ix + ic + gap), std::round(y + (h - tp) / 2), tp, t, c, ui::GAUCHE, 0.1f, false);
    }

    bool onTouch(const Touch& t, const GameView& g) override {
        if (t.id == doigt_) { if (!t.down) doigt_ = -1; return true; }   // le doigt du bouton ne pilote pas la voiture
        if (t.down && boutonVisible(g) && g_bouton.contient(t.x, t.y)) { doigt_ = t.id; vueSuivante(); return true; }   // RG-01
        return false;
    }

    void onEcran(Ecran, Ecran nouveau, const GameView&) override { if (nouveau != Ecran::Course) { g_bouton = {}; doigt_ = -1; } }

    void shutdown(Renderer& r) override {
        interieur_.detruire(r);   // WFS-197
        for (Mesh* m : {&statique_, &cadran_, &anneau_, &aiguille_, &jante_, &branches_, &repere_}) r.destroy(*m);
        for (Material* m : {&sombre_, &cuir_, &alu_, &cadranMat_, &cyan_, &orange_}) r.destroy(*m);   // WFS-187 (accroche) : jeux de descripteurs rendus
        g_bouton = {}; g_ck = {};
    }

private:
    // nuit demandée : même source que l'ambiance de nuit et la pluie (WFS_MOMENT, fixé par Game au choix du moment ; meteo.cpp)
    static bool nuitDemandee() { const char* m = std::getenv("WFS_MOMENT"); return m && std::strcmp(m, "nuit") == 0; }
    // WFS-197 : intérieur par modèle : carte de l'écran central (RG-05), valeurs du combiné (RG-04), essuie-glaces (RG-06)
    void dessinerInterieur(Renderer& r, const GameView& g, const mat4& cam, const EtatCockpit& ck) {
        const Player& pl = *g.player;
        Track& t = *g.track;
        float W, H; interieur_.tailleCarte(W, H);
        route_.clear();
        const TrackPoint pj = t.point(pl.dist, pl.playerX * RW);
        const double cap = pj.th + pl.heading;
        const vec2 joueur{(float)pj.x, (float)pj.z};
        const float echelle = H / (float)(cockpit::CARTE_ARRIERE_M + cockpit::CARTE_AVANT_M), yJ = -H / 2 + H * cockpit::CARTE_JOUEUR_Y;
        vec2 prec{};
        const int n = (int)((cockpit::CARTE_ARRIERE_M + cockpit::CARTE_AVANT_M) / cockpit::CARTE_PAS_M);
        for (int k = 0; k <= n; k++) {
            const double s = pl.dist + (-cockpit::CARTE_ARRIERE_M + k * cockpit::CARTE_PAS_M) / M;
            const TrackPoint p = t.point(s, 0);
            const vec2 uv = cockpit::versVoiture({(float)p.x, (float)p.z}, joueur, cap);
            const vec2 e{uv.x * echelle, yJ + uv.y * echelle};
            if (k > 0) {
                vec2 a = prec, b = e;
                if (minicarte::decouper(a, b, -W / 2, -H / 2, W / 2, H / 2)) { route_.push_back(a); route_.push_back(b); }
            }
            prec = e;
        }
        cockpit::EtatInterieur e;
        e.volant = ck.volant;
        e.v.kmh = cockpit::vitesseAffichee(pl.kmh());
        e.v.regime = moteur_.rpm; e.v.rapport = pl.reversing ? -1 : (e.v.kmh < 1 ? 0 : moteur_.gear);
        e.v.nitro = (float)pl.nitro; e.v.nuit = nuitDemandee();
        e.essuie = essuie_.angle(interieur_.def()->arc);
        interieur_.dessiner(r, cam, e, route_, joueur, cap);
        g_ckInt = e; g_ckAppels = interieur_.appels(); g_ckOctets = interieur_.octets(); g_ckCar = interieur_.def()->car;
    }

    cockpit::Interieur interieur_;
    cockpit::EtatEssuie essuie_;
    EngineModel moteur_; EngineTargets cibles_;
    std::vector<vec2> route_;
    Material sombre_, cuir_, alu_, cadranMat_, cyan_, orange_;
    Mesh statique_, cadran_, anneau_, aiguille_, jante_, branches_, repere_;
    int doigt_ = -1;
};

} // namespace

WFS_SYSTEM(Camera, 260)

// ---------- WFS-197 : banc d'essai des intérieurs (captures cockpit-<modèle>-{jour,braquage,carte,pluie,nuit}, cockpit-corsa) ----------
void benchCockpit(Game& g, Renderer& r, const std::string& out) {
    const VueCamera avant = vueCamera();
    const bool regles = courseBenchRulesOn();
    courseBenchRules(true);
    double t = 900;
    auto rouler = [&](int images) { for (int i = 0; i < images; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 6 == 0) g.render(t); } };
    auto capture = [&](const std::string& nom) {
        g.render(t); g.render(t + 1.0 / 60); t += 1.0 / 60;
        r.capturePng(out + "/" + nom + ".png");
        const Player& pl = g.player();
        const cockpit::EtatInterieur& e = g_ckInt;
        if (g_ckCar && cockpit::interieurPour(g.player().car->id))
            WFS_LOG("capture %s : intérieur %s, %d appels de dessin, %.2f Mo ; jeu : %.0f km/h, braquage %+.2f, nitro %.2f ; combiné : "
                    "%d km/h, régime %.2f, rapport %d, nitro %.2f ; volant %+.2f rad ; essuie-glaces %.2f rad%s", nom.c_str(), g_ckCar, g_ckAppels,
                    g_ckOctets / 1048576.0, pl.kmh(), pl.steer, pl.nitro, e.v.kmh, e.v.regime, e.v.rapport, e.v.nitro, e.volant, e.essuie,
                    e.v.nuit ? ", nuit" : "");
        else WFS_LOG("capture %s : cockpit commun (voiture %s sans intérieur propre)", nom.c_str(), pl.car->id);
    };
    auto course = [&](const char* car, const char* carte, bool nuit) {
        setenv("WFS_MOMENT", nuit ? "nuit" : "jour", 1);   // gardé pendant la course (nuit, pluie : lus à chaque image)
        const bool ok = g.init(r, car, carte);
        if (!ok) { WFS_ERR("cockpit : chargement impossible (%s, %s)", car, carte); return false; }
        choisirVue(VueCamera::Cockpit); g.camMode = 0; g.setAutopilot(true);
        rouler(60 * 3 + 30);   // décompte puis départ
        return true;
    };
    const float W = (float)r.width(), H = (float)r.height();
    for (const char* car : {"golf", "m4", "p911", "sf90"}) {
        const std::string c = std::string("cockpit-") + car;
        if (course(car, "ville", false)) {
            rouler(90);
            capture(c + "-jour");
            // braquage : gaz et doigt glissé dans la zone de braquage (comme camera-cockpit)
            g.setAutopilot(false);
            g.onTouch({31, W * 0.89f, H * 0.74f, true});
            g.onTouch({32, W * 0.20f, H * 0.70f, true}); g.onTouch({32, W * 0.29f, H * 0.70f, true});
            rouler(24);
            capture(c + "-braquage");
            g.onTouch({32, W * 0.29f, H * 0.70f, false}); g.onTouch({31, W * 0.89f, H * 0.74f, false});
            g.setAutopilot(true);
            rouler(60 * 3);   // la carte montre la route qui arrive (virages)
            capture(c + "-carte");
        }
        if (course(car, "ville", true)) {   // pluie : la Ville de nuit (meteo::pluieActive) ; balais au milieu de leur course
            const float arc = cockpit::interieurPour(car)->arc;   // pas de rendu pendant l'attente (le banc reste court) ; angle relu à chaque image
            // phase 0,5 = balai en haut de sa course (angle = arc) : on s'arrête dès 0,40 (angle ≥ 0,90 × arc)
            for (int k = 0; k < 240 && (g_ckEssuieT < 0.40f || g_ckEssuieT > 0.5f); k++) { g.update(1.0 / 60); t += 1.0 / 60; }
            (void)arc;
            capture(c + "-pluie");
        }
        if (course(car, "plage", true)) {   // nuit sans pluie : balais au repos
            rouler(30);
            capture(c + "-nuit");
        }
    }
    if (course("corsa", "ville", false)) { rouler(90); capture("cockpit-corsa"); }   // RG-01 : cockpit commun inchangé
    g.shutdown();
    unsetenv("WFS_MOMENT");
    courseBenchRules(regles);
    choisirVue(avant);
}

} // namespace wfs
