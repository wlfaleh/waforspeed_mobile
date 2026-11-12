// WaForSpeed — WFS-325 : banc « cockpit-vitrine » (voir vehicule_cockpit_vitrine.h).
// Scène dédiée (WFS-44) prise par un système inactif hors de ce banc : fond uni, soleil d'atelier au-dessus et derrière la caméra,
// sans ombre ni brouillard ; l'intérieur (Interieur::construire, le même qu'en course) est posé à l'origine du monde (repère de
// l'œil du conducteur) et vu de 4 places. Rien n'est construit ni dessiné en dehors de ce banc.
#include "vehicule_cockpit_vitrine.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "../core/ecran.h"
#include "../core/log.h"
#include "../core/system.h"
#include "../game/camera_vue.h"   // choisirVue : vue Cockpit avant la course (pipelines de l'intérieur créés au départ)
#include "../game/game.h"
#include "renderer.h"
#include "vehicule_cockpit.h"

namespace wfs {

namespace {
struct EtatVitrine { bool actif = false; cockpit::Interieur* interieur = nullptr; vec3 oeil{}, cible{}; float fov = 55; };
EtatVitrine& vitrine() { static EtatVitrine e; return e; }

// couleur sRGB d'un fond uni → valeur HDR qui ressort avec cette couleur (même approximation que scene_essai.cpp : ACES ≈ 0,214·x)
vec3 fondUni(unsigned hex, float exposition) {
    auto c = [&](int sh) { const float s = ((hex >> sh) & 255) / 255.f; return std::pow(s, 2.2f) / 0.214f / exposition; };
    return {c(16), c(8), c(0)};
}

class VitrineCockpit : public System {
public:
    const char* name() const override { return "cockpit-vitrine"; }
    bool scene(FrameParams& fp, SceneDediee& s, const GameView& g) override {
        const EtatVitrine& e = vitrine();
        if (!e.actif || !e.interieur) return false;   // hors du banc « cockpit-vitrine » : rien
        const float aspect = g.uiWidth / std::max(1.f, g.uiHeight);
        fp.view = lookAt(e.oeil, e.cible, {0, 1, 0});
        fp.proj = perspectiveVk(e.fov * PI / 180.f, aspect, 0.02f);
        fp.camPos = e.oeil;
        fp.exposure = 1.0f;
        const vec3 fond = fondUni(0x24262a, fp.exposure);
        fp.skyTop = fp.skyHorizon = fp.ground = fp.fogColor = fond;
        fp.fogDensity = 0;
        fp.sunIntensity = 3.0f; fp.sunDir = normalize(vec3{0.30f, 0.80f, 0.50f});   // au-dessus et derrière la caméra
        fp.ambient = 1.0f; fp.shadows = false; fp.hasFocus = false;
        s.cible = e.cible;   // route, joueur, commandes : non dessinés
        return true;
    }
    void render(Renderer& r, const GameView& g) override {
        const EtatVitrine& e = vitrine();
        if (!e.actif || !e.interieur || !g.sceneDediee) return;
        static const std::vector<vec2> sansRoute;
        e.interieur->dessiner(r, mat4::identity(), cockpit::EtatInterieur{}, sansRoute, {0, 0}, 0);
    }
};
WFS_SYSTEM(VitrineCockpit, 398)
} // namespace

void benchCockpitVitrine(Game& g, Renderer& r, const std::string& out, std::vector<std::string> voitures) {
    if (voitures.empty()) voitures = {"golf", "m4", "p911", "sf90"};
    double t = 1700;
    const VueCamera vueAvant = vueCamera();
    // vue Cockpit choisie avant la course : l'intérieur de la voiture est construit au départ, ses pipelines aussi (WFS-262 : aucun
    // pipeline créé pendant la course ; sans elle, l'intérieur de la vitrine en créait 4 au premier dessin)
    choisirVue(VueCamera::Cockpit);
    for (const std::string& car : voitures) {
        const cockpit::InterieurDef* d = cockpit::interieurPour(car.c_str());
        if (!d) { WFS_ERR("cockpit-vitrine : %s n'a pas d'intérieur", car.c_str()); continue; }
        setenv("WFS_MOMENT", "jour", 1);
        if (!g.init(r, car.c_str(), "ville")) { WFS_ERR("cockpit-vitrine : chargement impossible (%s)", car.c_str()); continue; }
        cockpit::Interieur in;
        in.construire(r, *d);
        // 2 images de la vitrine avant la course (à l'accueil) : les pipelines de la scène d'atelier sont créés hors course (WFS-262)
        vitrine() = {true, &in, {0, 0, 0}, {0, -0.3f, -0.9f}, 55};
        for (int i = 0; i < 2; i++) { g.render(t); t += 1.0 / 60; }
        g.allerA(Ecran::Course);
        for (int i = 0; i < 3; i++) { g.update(1.0 / 60); g.render(t); t += 1.0 / 60; }
        const cockpit::Largeur l = cockpit::largeurPour(*d);
        const float cx = l.oeil, demi = l.coudes * 0.5f, Y = d->plancheY, zF = cockpit::zFacePlanche(*d);
        struct Vue { const char* nom; vec3 oeil, cible; float fov; };
        const Vue vues[] = {
            {"centre", {cx + 0.05f, 0.02f, -0.15f}, {cx, Y - 0.30f, zF}, 50},                     // planche au milieu : aérateurs, commandes
            {"console", {cx + 0.10f, 0.05f, 0.05f}, {cx, -0.60f, -0.50f}, 55},                    // console vue d'au-dessus
            {"portiere", {cx + 0.10f, -0.05f, -0.15f}, {cx - demi, -0.36f, -0.60f}, 60},          // portière du conducteur
            {"gauche", {cx - demi + 0.25f, -0.06f, -0.40f}, {cx - demi + 0.10f, Y - 0.19f, zF}, 50},  // gauche de la planche, entre la portière et le volant
            {"portiere-bas", {cx + 0.05f, -0.20f, -0.30f}, {cx - demi, -0.66f, -0.80f}, 60},     // bas de la portière : haut-parleur, vide-poche
        };
        for (const Vue& v : vues) {
            vitrine().oeil = v.oeil; vitrine().cible = v.cible; vitrine().fov = v.fov;
            g.render(t); g.render(t + 1.0 / 60); t += 1.0 / 60;
            const std::string nom = "vitrine-" + car + "-" + v.nom;
            r.capturePng(out + "/" + nom + ".png");
            WFS_LOG("capture %s (WFS-325) : intérieur %s seul, éclairage d'atelier, %d appels de dessin, %.2f Mo", nom.c_str(), d->car,
                    in.appels(), in.octets() / 1048576.0);
        }
        vitrine() = {};
        in.detruire(r);
    }
    g.shutdown();
    unsetenv("WFS_MOMENT");
    choisirVue(vueAvant);
}

} // namespace wfs
