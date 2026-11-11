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
#include "../render/eclairage_physique.h"   // WFS-235 : banc (captures des sondes suspendues pendant la mesure)
#include "../render/renderer.h"
#include "../render/vehicule_cockpit_geo.h"   // WFS-197 : primitives du cockpit (déplacées depuis ce fichier)
#include "../render/vehicule_cockpit.h"       // WFS-197 : intérieurs par modèle
#include "../audio/engine_synth.h"           // WFS-197 : EngineModel (régime et rapport du combiné, comme le son)
#include "../ui/minicarte.h"                 // WFS-197 (RG-05) : découpe de la carte (minicarte::decouper, lecture)
#include "../world/meteo.h"                  // WFS-197 (RG-06) : pluieActive (lecture)
#include "../ui/hud.h"    // WFS-429 : rangée des boutons à côté du score
#include "../ui/text.h"
#include "../../third_party/stb/stb_image.h"   // WFS-235 : banc (lecture des captures, part cachée par l'intérieur)
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
               double camHd = 0, distDyn = 0;              // WFS-98 : camHd et camDistDyn d'updateCamera du jeu web
               vec3 eyeT0{}, lookT0{}; double hdT0 = 0;            // WFS-221 : cibles de l'image précédente (filtre exact)
               double accMoy = 0, vLisse = 0, vT0 = 0; } g_suivi;   // WFS-293 : accélération de la caméra, vitesse lissée et sa cible précédente
// WFS-221 (RG-02 c) : filtre du premier ordre (constante k /s) résolu EXACTEMENT pour une cible qui avance linéairement pendant
// l'image (x = y − v/k + (x0 − y0 + v/k)·e^(−k·dt)) : même retard moyen qu'avant, mais il ne dépend plus de la durée de chaque
// image (l'ancien lerp prenait la cible pour immobile : à 200 km/h, retard de ≈ 4 m variable d'une image à l'autre : la voiture
// avançait et reculait dans le cadre)
template <class T> static void suivreExact(T& x, T& cible0, const T& cible, double k, float fdt) {
    if (!g_cameraFiltreExact) { x = lerp(x, cible, 1.f - (float)std::exp(-fdt * k)); cible0 = cible; return; }
    if (fdt <= 1e-5f) { cible0 = cible; return; }
    const T vk = (cible - cible0) * (float)(1.0 / (fdt * k));
    x = cible - vk + (x - cible0 + vk) * (float)std::exp(-fdt * k);
    cible0 = cible;
}
static void suivreExact(double& x, double& cible0, double cible, double k, float fdt) {
    if (!g_cameraFiltreExact) { x += (cible - x) * std::min(1.0, fdt * k); cible0 = cible; return; }   // ancien pas d'Euler
    if (fdt <= 1e-5f) { cible0 = cible; return; }
    const double vk = (cible - cible0) / (fdt * k);
    x = cible - vk + (x - cible0 + vk) * std::exp(-fdt * k);
    cible0 = cible;
}
EtatCockpit g_ck;
float g_ckEssuieT = 0;   // WFS-197 : phase des essuie-glaces au dernier pas (banc d'essai)
cockpit::EtatInterieur g_ckInt; int g_ckAppels = 0; size_t g_ckOctets = 0; const char* g_ckCar = nullptr;   // WFS-197 : banc d'essai
bool g_ckBalaisRepos = false;     // WFS-235 : banc d'essai : balais dessinés au repos (part cachée par l'intérieur fixe)
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

static_assert(COCKPIT_OEIL_PLUS_M == cockpit::OEIL_CAMERA_PLUS_M, "WFS-323 : bande lisible du combiné calculée pour l'œil de la caméra");

// ---------- WFS-235 : regard du conducteur en vue Cockpit (RG-02 à RG-04) ----------
namespace {
RegardCockpit g_regardChoisi = RegardCockpit::Normal;
float g_aspect = 2400.f / 1080.f;   // forme de l'écran (largeur / hauteur), relevée à chaque pas par le système « camera »
// lacet du regard (rad, + = à droite), tête (m, repère de la voiture), cap de la voiture à l'image précédente (accélération
// latérale : la route qui tourne compte autant que la voiture qui braque)
struct Regard { bool init = false; float lacet = 0; vec3 tete{}; double cap = 0; vec3 dir{0, 0, -1}; } g_regard;
Track* g_pisteRegard = nullptr;   // banc d'essai (virage qui arrive) : piste de la dernière image cockpit
}
static vec3 vueRegard() { return g_regard.dir; }   // banc : direction du regard de la dernière image (monde)
RegardCockpit regardCockpit() { return g_regardChoisi; }
void choisirRegard(RegardCockpit r) {
    if (r == g_regardChoisi) return;
    g_regardChoisi = r;
    WFS_LOG("caméra : regard cockpit %s", r == RegardCockpit::Bas ? "bas" : r == RegardCockpit::Haut ? "haut" : "normal");
}

// le regard vise le point de la route à ≈ 1,5 s, sur la file de la voiture, et en suit 40 % (±7°, au-dessus de 20 km/h),
// lissé à 1,5 /s (WFS-282, RG-02) ;
// la tête suit les accélérations (lissées à 4 /s) ; « Réduire les mouvements » : tête immobile, regard qui suit à moitié
static void regarderRoute(Track& track, const Player& pl, const mat4& carM, const vec3& oeilL, float fdt) {
    const bool calme = reglagesRessenti().reduireMouvements;
    const TrackPoint p = track.pointLisse(pl.dist + distanceRegard(pl.kmh()) / M, pl.playerX * RW);
    const vec3 q = inverseAffine(carM).transformPoint({(float)p.x, (float)p.y, (float)p.z});
    const float lacetT = lacetVirage(std::atan2(q.x - oeilL.x, -(q.z - oeilL.z)), pl.kmh()) * (calme ? 0.5f : 1.f);
    const vec3 av = carM.transformDir({0, 0, -1});
    const double cap = std::atan2(av.x, -av.z), v = std::abs(pl.speed) * M;
    double latAcc = 0;
    if (g_regard.init && fdt > 1e-4f) latAcc = v * std::atan2(std::sin(cap - g_regard.cap), std::cos(cap - g_regard.cap)) / fdt;
    const vec3 teteT = calme ? vec3{} : mouvementTete(pl.lonAcc, latAcc);
    if (!g_regard.init || g_suivi.vue != VueCamera::Cockpit || !g_suivi.init) { g_regard.lacet = lacetT; g_regard.tete = teteT; }
    g_regard.lacet += (lacetT - g_regard.lacet) * (1.f - std::exp(-fdt * COCKPIT_LISSAGE_REGARD));
    g_regard.tete = lerp(g_regard.tete, teteT, 1.f - std::exp(-fdt * 4.f));
    g_regard.cap = cap; g_regard.init = true;
}

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
        const double th = track.pointLisse(pl.dist, lat).th;   // WFS-221 : cap de la route lissé (continu)
        const double hdT = th + pl.heading + pl.spinYaw * 0.15;   // tête-à-queue : la caméra n'en suit qu'un peu (reste stable)
        const bool neuf = !g_suivi.init || g_suivi.vue != g_vue || length(g_suivi.eye - carPos) > 25.f + P.dist;   // camInit
        if (neuf) { g_suivi.camHd = hdT; g_suivi.distDyn = P.dist; g_suivi.hdT0 = hdT; }
        {   // ≈ 0,3 s de retard : la voiture pivote d'abord dans l'image (WFS-221 : filtre exact, cible ramenée près de camHd)
            double cible = hdT, c0 = g_suivi.hdT0;
            cible = g_suivi.camHd + std::atan2(std::sin(cible - g_suivi.camHd), std::cos(cible - g_suivi.camHd));
            c0 = cible - std::atan2(std::sin(cible - c0), std::cos(cible - c0));
            suivreExact(g_suivi.camHd, c0, cible, 3.4, fdt);
            g_suivi.hdT0 = c0;
        }
        // WFS-293 (RG-11) : accélération tirée de la vitesse lissée (filtre exact de 0,3 s) au lieu de l'accélération du dernier pas :
        // au gaz tout ou rien, lonAcc bascule d'un pas à l'autre ; lue une fois par image (66 ms à 15 img/s), elle tombait au hasard
        // d'un côté ou de l'autre et la distance de la caméra sautait (voiture qui avance et recule dans le cadre : 38 à 54 mm à
        // 15 img/s contre la même course rejouée à 120 Hz, 4,6 mm ensuite). Même valeur moyenne, indépendante de la cadence.
        if (neuf || fdt <= 1e-5f) { g_suivi.accMoy = pl.lonAcc; if (neuf) g_suivi.vLisse = g_suivi.vT0 = pl.speed * M; }
        else if (g_cameraFiltreExact) {   // vitesse lissée (filtre exact, 0,3 s : indépendant de la cadence), puis sa dérivée sur l'image
            const double avant = g_suivi.vLisse;
            suivreExact(g_suivi.vLisse, g_suivi.vT0, pl.speed * M, 3.3, fdt);
            g_suivi.accMoy = (g_suivi.vLisse - avant) / fdt;
        }
        else g_suivi.accMoy = pl.lonAcc;   // passe « avant » du banc : comme avant
        const double distT = P.dist + p * 0.6 + std::clamp(g_suivi.accMoy * 0.05, -0.4, 0.6);   // s'éloigne en accélérant
        g_suivi.distDyn += (distT - g_suivi.distDyn) * (g_cameraFiltreExact ? 1.0 - std::exp(-fdt * 2.5) : std::min(1.0, fdt * 2.5));   // WFS-221 : exact (même constante)
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
        if (neuf) { g_suivi.init = true; g_suivi.vue = g_vue; g_suivi.eye = eT; g_suivi.look = lT; g_suivi.fov = fovT; g_suivi.eyeT0 = eT; g_suivi.lookT0 = lT; }
        suivreExact(g_suivi.eye, g_suivi.eyeT0, eT, SUIVI_OEIL, fdt);      // WFS-221 : filtre exact (mêmes constantes)
        suivreExact(g_suivi.look, g_suivi.lookT0, lT, SUIVI_REGARD, fdt);
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
    if (V.cockpit) {   // œil du conducteur (à gauche) : inner.localToWorld(−0,36 ; H × 0,74 ; −L × 0,02)
        // WFS-197 (RG-07) : voiture qui a son intérieur : hauteur d'œil du modèle (assise), même écart œil → regard
        const cockpit::InterieurDef* idef = cockpit::interieurPour(car.id);
        // WFS-201 (RG-07) : œil = assise du modèle + hauteur des yeux assis d'ANSUR-II (cockpit commun : H × 0,74)
        const float oeil = idef ? cockpit::hauteurOeil(*idef) : d.H * 0.74f;
        // WFS-315 (RG-01) : écart œil-axe du modèle (cockpit commun : 0,36 m)
        eyeL = {idef ? -cockpit::largeurPour(*idef).oeil : -0.36f, oeil, -d.L * 0.02f};
        regarderRoute(track, pl, carM, eyeL, fdt);   // WFS-235 : regard et tête (repère de la voiture)
        // l'intérieur reste attaché à la voiture, à la place de l'œil (la tête tourne et bouge dedans)
        g_ck.repere = carM * translate(eyeL);
        // WFS-282 (RG-03) : l'œil de la caméra est 4 cm au-dessus de l'œil du siège (marge permise : 2 à 4 cm) ; l'intérieur
        // reste dans le repère du siège : la planche descend dans l'image et la rue gagne de la place
        eyeL = eyeL + vec3{0, COCKPIT_OEIL_PLUS_M, 0} + g_regard.tete + decalageCou(g_regard.lacet);
        const float pente = std::tan(inclinaisonRegard(g_regardChoisi));
        lookL = eyeL + vec3{std::sin(g_regard.lacet) * 30.f, pente * 30.f, -std::cos(g_regard.lacet) * 30.f};
    } else {           // capot : au milieu, devant le pare-brise, le capot reste dans le bas de l'image
        eyeL = {0.f, d.H * 0.84f, -d.L * 0.24f}; lookL = {0.f, d.H * 0.72f, -d.L * 0.24f - 30.f};
    }
    const vec3 eT = carM.transformPoint(eyeL), lT = carM.transformPoint(lookL);
    // WFS-282 (RG-00, ANO-152) : cockpit et capot : champ de configgame.json (cockpit : 72° en hauteur). Le champ horizontal de
    // 85° de WFS-235 (≈ 46° en hauteur sur 2340 × 1080) grossissait tout l'intérieur ≈ 1,7 fois et mangeait la rue
    const float fovT = fovVue(g_vue, pl.speed / MAX_SPEED, pl.nitroOn);
    if (!g_suivi.init || g_suivi.vue != g_vue || length(g_suivi.eye - eT) > 25.f) {
        g_suivi = {true, g_vue, eT, lT, fovT};
        g_ck.volant = volantCible(pl.steer);
    }
    // suivi lissé (C.hood : 60 /s, indépendant de la fréquence d'affichage), champ de vision lissé à 4 /s ;
    // WFS-235 : cockpit : l'œil est la tête du conducteur, assise dans la voiture : aucun retard (à 200 km/h, le suivi à
    // 60 /s la laissait ≈ 0,9 m derrière son siège) ; le regard et la tête ont leur propre lissage (regarderRoute)
    const float k = V.cockpit ? 1.f : 1.f - std::exp(-fdt * 60.f);
    g_suivi.eye = lerp(g_suivi.eye, eT, k); g_suivi.look = lerp(g_suivi.look, lT, k);
    g_suivi.fov = lisser(g_suivi.fov, fovT, fdt, 4.f);
    eye = g_suivi.eye; look = g_suivi.look; fov = g_suivi.fov; g_ck.fov = fov;
    if (V.cockpit) {
        g_ck.actif = true; g_regard.dir = look - eye; g_pisteRegard = &track;   // WFS-235 : banc d'essai
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
        // WFS-282 : double face (WFS-246 retire les faces arrière des opaques : pièces du cockpit vues de l'intérieur)
        Material m; m.flags = MAT_DOUBLE;
        m.color = {lin(0x2a2c33).x, lin(0x2a2c33).y, lin(0x2a2c33).z, 1}; m.rough = 0.7f; m.metal = 0.15f; sombre_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
        m.color = {lin(0x23242a).x, lin(0x23242a).y, lin(0x23242a).z, 1}; m.rough = 0.5f; m.metal = 0.1f; m.emissive = lin(0x08090c);
        cuir_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
        m.color = {lin(0x9aa0a8).x, lin(0x9aa0a8).y, lin(0x9aa0a8).z, 1}; m.rough = 0.3f; m.metal = 0.9f; m.emissive = {};
        alu_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
        auto basique = [&](unsigned c) { Material b; b.flags = MAT_DOUBLE; b.color = {0, 0, 0, 1}; b.rough = 1; b.metal = 0; b.emissive = lin(c); return r.makeMaterial(b, nullptr, nullptr, nullptr); };
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
        if (g.uiWidth > 0 && g.uiHeight > 0) g_aspect = g.uiWidth / g.uiHeight;   // WFS-235 : forme de l'écran (champ de vision)
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
        // WFS-235 : intérieur attaché à la voiture, à la place de l'œil au repos (avant : à la caméra, camera.add(g) du jeu web ;
        // le regard qui suit le virage et la tête qui bouge le feraient tourner avec eux)
        const mat4 cam = ck.repere;
        // WFS-197 (RG-01) : intérieur du modèle s'il existe (construit à la première vue Cockpit de cette voiture), sinon le
        // cockpit commun ci-dessous, inchangé
        const cockpit::InterieurDef* idef = g.car ? cockpit::interieurPour(g.car->id) : nullptr;
        if (idef != interieur_.def()) {
            if (interieur_.pret()) interieur_.detruire(r);
            if (idef) interieur_.construire(r, *idef);   // WFS-282 (RG-00) : intérieur d'avant WFS-235
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

    // #camBtn : même style que le bouton « Menu » de la course (.cam-btn), libellé = nom de la vue ;
    // WFS-429 (RG-01, RG-02) : entre le bouton Menu et le score, en haut à droite, +5 % (hud::rangeeBoutons)
    void drawUi(Renderer& r, const GameView& g) override {
        if (!boutonVisible(g) || !ui::fontReady()) { g_bouton = {}; return; }
        const float k = g.uiHeight / 720.f;
        const std::string t = ui::majuscules(VUES[(int)vueCamera()].nom);
        hud::BoutonHud menu, b;
        hud::rangeeBoutons(g.uiWidth, g.uiHeight, g.marges, t, menu, b);
        const float x = b.x, y = b.y, w = b.w, h = b.h, ic = b.icone;
        g_bouton = {x, y, w, h};
        biseau(r, x - k, y - k, w + 2 * k, h + 2 * k, b.biais, BORD);
        biseau(r, x, y, w, h, b.biais, GLASS);
        const vec4 c = hex(TEXTE);
        const float ix = x + b.marge, iy = y + (h - ic) / 2, u = ic / 24;   // icône appareil photo (path de #camBtn, 24 × 24)
        r.uiRect(ix + 3 * u, iy + 7 * u, 18 * u, 12 * u, c);
        r.uiRect(ix + 8.6f * u, iy + 5 * u, 6.8f * u, 2 * u, c);
        r.uiCircle(ix + 12 * u, iy + 13.5f * u, 3.3f * u, hex(BG, 0.9f));
        ui::text(r, std::round(ix + ic + b.ecartIcone), std::round(y + (h - b.texte) / 2), b.texte, t, c, ui::GAUCHE, 0.1f, false);
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
        e.essuie = g_ckBalaisRepos ? 0.f : essuie_.angle(interieur_.def()->arc);
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
void benchCockpit(Game& g, Renderer& r, const std::string& out, const std::vector<std::string>& voitures) {
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
    // WFS-235 (RG-05, critère arbitré par le PO le 2026-11-07) : parts de l'image cachées par l'intérieur, d'après le même
    // instant dessiné deux fois, l'intérieur normal puis tout en couleur clé (cockpit::g_interieurCle) : pixels qui changent =
    // pixels de l'intérieur (même géométrie : la lumière de la rue ne change pas ; l'interface est la même dans les deux) :
    //   fenêtre de la rue : de l'horizon à +8°, ±15° autour de l'axe de la voiture (0 % visé) ;
    //   bande de la rue : de l'horizon à +8°, toute la largeur (≤ vraie voiture + 2 points) ;
    //   au-dessus de l'horizon : tout ce qui est au-dessus (écrit seulement) ; direction de chaque pixel : caméra de l'image
    //   balaisRepos : sous la pluie, les balais en mouvement passent devant la rue (comme dans une vraie voiture) : la
    //   fenêtre de la rue se mesure balais au repos (intérieur fixe) ; la mesure balais en mouvement est seulement écrite
    auto partCachee = [&](const std::string& nom, bool balaisRepos = false) {
        const std::string a = out + "/" + nom + "-mesure.png", b = out + "/" + nom + "-cle.png", a2 = out + "/" + nom + "-controle.png";
        // même lumière pour les deux images : aucune nouvelle capture des sondes pendant la mesure, et les captures en route
        // finissent avant (relecture 2 images plus tard, puis calcul en tâche de fond : à la fin, l'exposition se recale d'un
        // coup) ; temps figé : pas d'adaptation de l'œil ; contrôle : image avec l'intérieur refaite après celle sans
        // halo (bloom) coupé pendant la mesure : les lueurs ne débordent ni de la couleur clé, ni des lampes cachées par le pavillon
        eclairage::suspendreCaptures(true);
        const float bloomAvant = r.bloomForce;
        r.bloomForce = 0;
        int wa = 0, ha = 0, ca, wb, hb, cb, w2, h2, c2;
        unsigned char *pa = nullptr, *pb = nullptr;
        float balais = 0, instable = 100;
        for (int essai = 0; essai < 3 && instable > 0.2f; essai++) {
            stbi_image_free(pa); stbi_image_free(pb); pa = pb = nullptr;   // nul accepté
            for (int i = 0; i < 3; i++) g.render(t);
            g_ckBalaisRepos = balaisRepos;
            g.render(t); r.capturePng(a);
            balais = g_ckInt.essuie;
            cockpit::g_interieurCle = true; g.render(t); r.capturePng(b); cockpit::g_interieurCle = false;
            g.render(t); r.capturePng(a2);
            g_ckBalaisRepos = false;
            pa = stbi_load(a.c_str(), &wa, &ha, &ca, 3); pb = stbi_load(b.c_str(), &wb, &hb, &cb, 3);
            unsigned char* p2 = stbi_load(a2.c_str(), &w2, &h2, &c2, 3);
            if (pa && p2 && w2 == wa && h2 == ha) {
                long n = 0;
                for (long i = 0; i < (long)wa * ha * 3; i += 3)
                    n += std::max({std::abs(pa[i] - p2[i]), std::abs(pa[i + 1] - p2[i + 1]), std::abs(pa[i + 2] - p2[i + 2])}) > 12;
                instable = 100.f * n / ((float)wa * ha);
            }
            stbi_image_free(p2);
        }
        eclairage::suspendreCaptures(false);   // banc hors ligne : la qualité automatique (qui peut aussi les suspendre) est inactive
        r.bloomForce = bloomAvant;
        std::remove(a2.c_str());
        if (instable > 0.2f) WFS_ERR("%s (WFS-235 RG-05) : lumière instable pendant la mesure (%.1f %% des pixels changent entre deux images "
                                    "avec l'intérieur) : mesure non retenue", nom.c_str(), instable);
        else if (pa && pb && wa == wb && ha == hb) {
            const vec3 f = normalize(vueRegard()), sd = normalize(cross(f, {0, 1, 0})), u = cross(sd, f);
            const vec3 av = etatCockpit().repere.transformDir({0, 0, -1});
            const float tanV = std::tan(etatCockpit().fov * PI / 360.f), tanH = tanV * (float)wa / (float)ha, azVoiture = std::atan2(av.x, -av.z);
            const float H8 = 8.f * PI / 180.f, A15 = 15.f * PI / 180.f;
            long n[4] = {0, 0, 0, 0}, c[4] = {0, 0, 0, 0};   // fenêtre, bande, au-dessus de l'horizon, toute l'image (WFS-282)
            for (int y = 0; y < ha; y++) for (int x = 0; x < wa; x++) {
                const vec3 d = f + sd * (((x + 0.5f) / wa * 2 - 1) * tanH) + u * ((1 - (y + 0.5f) / ha * 2) * tanV);
                const float el = std::atan2(d.y, std::sqrt(d.x * d.x + d.z * d.z));
                const unsigned char *p = pa + ((size_t)y * wa + x) * 3, *q = pb + ((size_t)y * wa + x) * 3;
                const bool cache = std::max({std::abs(p[0] - q[0]), std::abs(p[1] - q[1]), std::abs(p[2] - q[2])}) > 12;
                n[3]++; c[3] += cache;
                if (el < 0) continue;
                float az = std::atan2(d.x, -d.z) - azVoiture; az = std::atan2(std::sin(az), std::cos(az));
                const bool bande = el <= H8, fenetre = bande && std::fabs(az) <= A15;
                n[2]++; c[2] += cache;
                if (bande) { n[1]++; c[1] += cache; }
                if (fenetre) { n[0]++; c[0] += cache; }
            }
            auto pct = [&](int k) { return n[k] ? 100.0 * c[k] / n[k] : 0.0; };
            // WFS-282 (RG-03) : horizon au milieu de l'image (part de la hauteur depuis le haut ; 40 à 45 % visé)
            const float horizon = 0.5f + std::tan(std::asin(std::clamp(f.y, -1.f, 1.f))) / (2 * tanV);
            WFS_LOG("capture %s (WFS-235 RG-05) : regard %s ; cachés par l'intérieur : fenêtre de la rue %.1f %% (0 %% visé), bande de la rue %.1f %%, "
                    "au-dessus de l'horizon %.1f %%, toute l'image %.1f %% (WFS-282 : 30 à 35 %% visé) ; horizon à %.1f %% depuis le haut ; "
                    "balais %.2f rad%s ; tangage de la voiture %+.1f° ; contrôle %.2f %%", nom.c_str(),
                    regardCockpit() == RegardCockpit::Bas ? "bas" : regardCockpit() == RegardCockpit::Haut ? "haut" : "normal",
                    pct(0), pct(1), pct(2), pct(3), horizon * 100, balais, balais > 0.02f ? " (en mouvement, pluie : mesure écrite seulement)" : "",
                    std::asin(std::clamp(av.y, -1.f, 1.f)) * 180.f / PI, instable);   // tangage : nez vers le bas, la bande monte
        } else WFS_ERR("%s : parts cachées non mesurées (captures illisibles)", nom.c_str());
        stbi_image_free(pa); stbi_image_free(pb);   // nul accepté
        std::remove(a.c_str()); std::remove(b.c_str());
    };
    // RG-03 : les regards Bas et Haut tiennent aussi la fenêtre de la rue (même instant, regard changé, puis Normal)
    auto troisRegards = [&](const std::string& nom) {
        const RegardCockpit avantR = regardCockpit();
        for (RegardCockpit rg : {RegardCockpit::Normal, RegardCockpit::Bas, RegardCockpit::Haut}) { choisirRegard(rg); partCachee(nom); }
        choisirRegard(avantR);
    };
    // WFS-235 (RG-02) : virage : la course roule (pilote automatique) jusqu'à ce que la route tourne d'au moins 20° dans les
    // 40 m qui viennent, puis encore un peu (regard tourné vers l'intérieur du virage, lissé)
    auto jusquAuVirage = [&]() {
        for (int k = 0; k < 60 * 40; k++) {
            const Player& pl = g.player();
            if (!g_pisteRegard) return false;
            const double th0 = g_pisteRegard->pointLisse(pl.dist, 0).th, th1 = g_pisteRegard->pointLisse(pl.dist + 40.0 / M, 0).th;
            if (std::abs(std::atan2(std::sin(th1 - th0), std::cos(th1 - th0))) > 0.35) { rouler(40); return true; }
            g.update(1.0 / 60); t += 1.0 / 60; if (k % 30 == 0) g.render(t);
        }
        return false;
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
    for (const std::string& v : voitures) {
        const char* car = v.c_str();
        const std::string c = "cockpit-" + v;
        // RG-01 : voiture sans intérieur propre (Corsa, lot 3) : une capture du cockpit commun, qui doit rester inchangé
        if (!cockpit::interieurPour(car)) { if (course(car, "ville", false)) { rouler(90); capture(c); } continue; }
        if (course(car, "ville", false)) {
            rouler(90);
            capture(c + "-jour");
            troisRegards(c + "-jour");   // WFS-235 (RG-05)
            if (jusquAuVirage()) { capture(c + "-virage"); partCachee(c + "-virage"); }   // WFS-235 (RG-02, RG-05)
            else WFS_ERR("cockpit (WFS-235) : aucun virage trouvé en 40 s (%s)", car);
            // braquage : gaz et doigt glissé dans la zone de braquage (comme camera-cockpit)
            g.setAutopilot(false);
            g.onTouch({31, W * 0.89f, H * 0.74f, true});
            g.onTouch({32, W * 0.20f, H * 0.70f, true}); g.onTouch({32, W * 0.29f, H * 0.70f, true});
            rouler(24);
            capture(c + "-braquage");
            g.onTouch({32, W * 0.29f, H * 0.70f, false}); g.onTouch({31, W * 0.89f, H * 0.74f, false});
            g.setAutopilot(true);
            rouler(90);   // la carte montre la route qui arrive (virages)
            capture(c + "-carte");
        }
        if (course(car, "ville", true)) {   // pluie : la Ville de nuit (meteo::pluieActive) ; balais au milieu de leur course
            const float arc = cockpit::interieurPour(car)->arc;   // pas de rendu pendant l'attente (le banc reste court) ; angle relu à chaque image
            // phase 0,5 = balai en haut de sa course (angle = arc) : on s'arrête dès 0,40 (angle ≥ 0,90 × arc)
            for (int k = 0; k < 240 && (g_ckEssuieT < 0.40f || g_ckEssuieT > 0.5f); k++) { g.update(1.0 / 60); t += 1.0 / 60; }
            (void)arc;
            capture(c + "-pluie");
            // WFS-201 : nuit (même course : écrans de nuit, combiné lisible) quand les balais repassent au repos, entre deux balayages ;
            // « hors pluie, balais à l'arrêt » se voit sur -jour (une course de moins par voiture : banc plus court)
            for (int k = 0; k < 120 && g_ckEssuieT > 0.02f; k++) { g.update(1.0 / 60); t += 1.0 / 60; }
            capture(c + "-nuit");
            partCachee(c + "-nuit");   // WFS-235 (RG-05)
            if (jusquAuVirage()) {   // WFS-235 (RG-05) : balais où ils sont (écrit), puis au repos (critère)
                capture(c + "-nuit-virage"); partCachee(c + "-nuit-virage"); partCachee(c + "-nuit-virage", true);
            }
        }
        // WFS-282 : départ de nuit sur la Plage (capture de l'utilisateur, qa/retours/2026-11-07-cockpit-nuit-plage.jpg)
        if (course(car, "plage", true)) { rouler(70); capture(c + "-plage-nuit-depart"); partCachee(c + "-plage-nuit-depart"); }
    }
    g.shutdown();
    unsetenv("WFS_MOMENT");
    courseBenchRules(regles);
    choisirVue(avant);
}

// ---------- WFS-323 : banc « cockpit-reference » : les scènes de la référence de l'utilisateur ----------
// (qa/retours/2026-11-08-cockpit-reference-utilisateur.jpg) : Golf, M4 et X5 sur la Corniche de jour en ligne droite, Golf en
// Ville de nuit, pavillon (regard Haut), et une suite de 4 images d'un braquage (le volant tourne avec la direction, ANO-187).
// Chaque capture écrit au journal l'horizon (part de la hauteur depuis le haut, RG-01 : 40 à 45 %) et le champ horizontal.
void benchCockpitReference(Game& g, Renderer& r, const std::string& out) {
    const VueCamera avant = vueCamera();
    const RegardCockpit avantR = regardCockpit();
    const bool regles = courseBenchRulesOn();
    courseBenchRules(true);
    double t = 900;
    auto rouler = [&](int images) { for (int i = 0; i < images; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 6 == 0) g.render(t); } };
    auto capture = [&](const std::string& nom) {
        g.render(t); g.render(t + 1.0 / 60); t += 1.0 / 60;
        r.capturePng(out + "/" + nom + ".png");
        const EtatCockpit& ck = etatCockpit();
        const vec3 f = normalize(vueRegard());
        const float tanV = std::tan(ck.fov * PI / 360.f), tanH = tanV * (float)r.width() / (float)r.height();
        const float horizon = 0.5f + std::tan(std::asin(std::clamp(f.y, -1.f, 1.f))) / (2 * tanV);
        WFS_LOG("capture %s (WFS-323) : voiture %s ; horizon à %.1f %% depuis le haut (40 à 45 %% visé) ; champ vertical %.1f°, "
                "horizontal %.1f° ; volant %+.2f rad ; braquage %+.2f ; %.0f km/h ; intérieur : %d appels de dessin, %.2f Mo (RG-04 : coût)",
                nom.c_str(), g.player().car->id, horizon * 100, ck.fov, 2 * std::atan(tanH) * 180.f / PI, g_ckInt.volant, g.player().steer,
                g.player().kmh(), g_ckAppels, g_ckOctets / 1048576.0);
    };
    auto course = [&](const char* car, const char* carte, bool nuit) {
        setenv("WFS_MOMENT", nuit ? "nuit" : "jour", 1);
        if (!g.init(r, car, carte)) { WFS_ERR("cockpit-reference : chargement impossible (%s, %s)", car, carte); return false; }
        choisirVue(VueCamera::Cockpit); choisirRegard(RegardCockpit::Normal); g.camMode = 0; g.setAutopilot(true);
        rouler(60 * 3 + 30);   // décompte puis départ
        return true;
    };
    // ligne droite : la course roule jusqu'à ce que la route reste droite (moins de 3° de virage) sur les 120 m qui viennent
    auto ligneDroite = [&]() {
        for (int k = 0; k < 60 * 40; k++) {
            const Player& pl = g.player();
            if (g_pisteRegard && pl.kmh() > 50) {
                const double th0 = g_pisteRegard->pointLisse(pl.dist, 0).th, th1 = g_pisteRegard->pointLisse(pl.dist + 120.0 / M, 0).th;
                if (std::abs(std::atan2(std::sin(th1 - th0), std::cos(th1 - th0))) < 0.05) return;
            }
            g.update(1.0 / 60); t += 1.0 / 60; if (k % 30 == 0) g.render(t);
        }
    };
    for (const char* car : {"golf", "m4", "rrsport"})
        if (course(car, "plage", false)) {
            rouler(60); ligneDroite(); rouler(20);
            capture(std::string("cockpit-reference-") + car + "-corniche-jour");
            if (!std::strcmp(car, "golf")) {
                choisirRegard(RegardCockpit::Haut); rouler(120); capture("cockpit-reference-golf-pavillon");
                choisirRegard(RegardCockpit::Bas); rouler(120); capture("cockpit-reference-golf-regard-bas");
                choisirRegard(RegardCockpit::Normal); rouler(120);
                // ANO-187 : suite de 4 images d'un braquage à gauche (doigt glissé dans la zone de braquage, comme benchCockpit)
                const float W = (float)r.width(), H = (float)r.height();
                g.setAutopilot(false);
                g.onTouch({32, W * 0.20f, H * 0.70f, true});
                for (int k = 1; k <= 4; k++) {
                    g.onTouch({32, W * (0.20f - 0.02f * k), H * 0.70f, true});
                    rouler(8); capture("cockpit-reference-golf-volant-" + std::to_string(k));
                }
                g.onTouch({32, W * 0.12f, H * 0.70f, false});
                g.setAutopilot(true);
            }
        }
    if (course("golf", "ville", true)) { rouler(60); ligneDroite(); rouler(20); capture("cockpit-reference-golf-ville-nuit"); }
    // WFS-323 (ANO-188, revue du lead dev) : combiné de chaque voiture, de jour, en ligne droite (agrandissement fait à partir de
    // l'image entière) ; bande vue par l'ouverture haute du volant au journal
    for (int i = 0; i < cockpit::nombreInterieurs(); i++) {
        const cockpit::InterieurDef& d = cockpit::interieur(i);
        const cockpit::BandeCombine b = cockpit::bandeLisibleCombine(d);
        if (d.combine == cockpit::Combine::Aucun || d.combine == cockpit::Combine::PeugeotICockpit)
            WFS_LOG("combiné %s (WFS-323) : %s", d.car, d.combine == cockpit::Combine::Aucun ? "vitesse sur l'écran central" : "i-Cockpit, lu au-dessus du volant");
        else WFS_LOG("combiné %s (WFS-323) : hauteur %.0f mm, bande vue par le volant %+.0f..%+.0f mm du centre (%.0f %% de la hauteur)", d.car,
                d.combineH * 1000, b.bas * 1000, b.haut * 1000, d.combineH > 0 ? 100 * (b.haut - b.bas) / d.combineH : 0.f);
        if (course(d.car, "plage", false)) { rouler(60); ligneDroite(); rouler(20); capture(std::string("cockpit-reference-combine-") + d.car); }
    }
    g.shutdown();
    unsetenv("WFS_MOMENT");
    courseBenchRules(regles);
    choisirRegard(avantR);
    choisirVue(avant);
}

} // namespace wfs
