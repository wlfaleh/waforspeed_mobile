// WaForSpeed — banc d'essai de la police de la Poursuite (WFS-461) : voir police_banc.h.
#include "police_banc.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../core/system.h"
#include "../render/renderer.h"
#include "../ui/text.h"
#include "camera_vue.h"
#include "cars.h"
#include "course5.h"
#include "course_system.h"
#include "game.h"
#include "poursuite.h"
#include "roues_banc.h"   // poursuiteBancPolice (accroche du banc WFS-452)

namespace wfs {
namespace {
std::string g_legende;   // banc seulement : texte écrit en haut à gauche de l'image (vide : rien)

// banc seulement : légende des captures (mesures de la scène), dessinée par-dessus l'interface ; inactif hors banc
class LegendeBancPolice : public System {
public:
    const char* name() const override { return "legende-banc-police"; }
    void drawUi(Renderer& r, const GameView& g) override {
        if (g_legende.empty()) return;
        const float u = g.uiHeight / 100.f, px = 2.6f * u, x = 2 * u, y = 30 * u;
        size_t n = 1; for (char c : g_legende) n += c == '\n';
        r.uiRect(x - u, y - u, 62 * u, (float)n * px * 1.35f + 2 * u, {0, 0, 0, 0.72f});
        size_t p0 = 0; float yy = y;
        for (size_t p; (p = g_legende.find('\n', p0)) != std::string::npos || p0 < g_legende.size(); p0 = p + 1) {
            if (p == std::string::npos) p = g_legende.size();
            ui::text(r, x, yy, px, g_legende.substr(p0, p - p0), {1, 0.86f, 0.25f, 1}, ui::GAUCHE, 0, false);
            yy += px * 1.35f;
        }
    }
};
}   // namespace
WFS_SYSTEM(LegendeBancPolice, 499)   // interface (400-499) : en dernier, par-dessus le HUD

int benchPolice(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    const ModeCourse mode0 = modeCourse();
    const VueCamera vue0 = vueCamera();
    constexpr double PAS = 1.0 / 60;   // une mise à jour = 2 pas de conduite de 1/120 s
    const CarDef& joueur = carById("golf");
    const CarDef& police = carById("p208");
    const double vMaxPolice = police.spec.vmax;   // km/h, fiche (RG-03)
    courseBenchRules(true);
    double t = 900;
    auto image = [&](const std::string& nom) {
        g.render(t); g.render(t + PAS); t += PAS;
        r.capturePng(out + "/" + nom + ".png");
    };
    // caméra « vitrine » : point de vue à avantM devant et droiteM à droite de la voiture du joueur, à hauteur m
    auto viser = [&](double avantM, double droiteM, float hauteur) {
        const Game::Vue v = g.calculerVue(t);
        const vec3 f = normalize(vec3{v.fwd.x, 0, v.fwd.z}), d = cross(f, {0, 1, 0});
        const vec3 o = f * (float)avantM + d * (float)droiteM;
        g.orbit = std::atan2(o.x, o.z); g.orbitRecul = length(o); g.orbitHauteur = hauteur;
    };
    auto demarrer = [&]() {
        setenv("WFS_DEPART", "0", 1); setenv("WFS_MOMENT", "jour", 1);
        modeCourse() = ModeCourse::Poursuite;
        if (!g.init(r, joueur.id, "plage")) { WFS_ERR("police : chargement de la Corniche impossible"); ecarts++; return false; }
        g.allerA(Ecran::Course); g.modeManette = false; g.setAutopilot(true); g.camMode = 0;
        for (int i = 0; i < 4 * 60; i++) { g.update(PAS); t += PAS; if (i % 12 == 0) g.render(t); }   // décompte
        return true;
    };
    // vitesse (km/h) de la voiture de police la plus proche, mesurée sur son déplacement (même mesure avant / après)
    double rdPrec[8] = {}; int nPrec = 0; double vMaxToutes = 0;   // vitesse maxi de toutes les voitures de police (km/h)
    auto vitesses = [&](double dtMesure, double& vPolice, double& ecartM) {
        double rd[8]; bool fixe[8];
        const int n = poursuitePolice(rd, fixe, 8);
        vPolice = 0; ecartM = 1e9;
        for (int i = 0; i < n; i++) {
            if (fixe[i]) continue;
            const double e = (g.player().dist - rd[i]) * M;
            const double dM = i < nPrec ? (rd[i] - rdPrec[i]) * M : 0;
            const double v = std::abs(dM) < 10 ? dM / dtMesure * 3.6 : 0;   // saut (replacement après perte) : pas une vitesse
            if (std::abs(e) < std::abs(ecartM)) { ecartM = e; vPolice = v; }
            if (std::abs(dM) < 10) vMaxToutes = std::max(vMaxToutes, v);
        }
        for (int i = 0; i < n && i < 8; i++) rdPrec[i] = rd[i];
        nPrec = n;
    };

    // ---------- 1. contact au flanc vers 30 km/h (vue Poursuite, puis Cockpit) ----------
    for (int cockpit = 0; cockpit < 2; cockpit++) {
        choisirVue(cockpit ? VueCamera::Cockpit : VueCamera::Poursuite);
        if (!demarrer()) break;
        const float cible = 30;
        for (int i = 0; i < 60 * 25 && g.player().kmh() < cible - 1; i++) { g.setAutopilot(true); g.update(PAS); t += PAS; if (i % 12 == 0) g.render(t); }
        // la police se range contre le flanc gauche (5 cm de jour), 0,8 m devant : comme dans la vidéo
        const double touche = (joueur.d.W + police.d.W) / 2;
        double px = std::clamp(g.player().playerX, -0.25, 0.6);
        const double xPolice = px - (touche + 0.05) / RW;
        poursuiteBancPolice(true, 0.8, xPolice);
        g.setAutopilot(false); g.modeManette = true;
        auto commande = [&](float volant) { g.manette = DriveInput{}; g.manette.steer = volant; g.manette.gas = g.player().kmh() < cible; };
        for (int i = 0; i < 20; i++) { commande(0); g.update(PAS); t += PAS; }
        const double x0 = g.player().playerX, h0 = g.player().heading;
        const std::string base = cockpit ? "police-flanc-cockpit-" : "police-flanc-";
        for (int k = 0; k < 10; k++) {
            for (int i = 0; i < 9; i++) { commande(k < 4 ? -0.5f : 0.f); g.update(PAS); t += PAS; }   // braque 0,6 s vers la police, puis lâche
            const double dx = (g.player().playerX - x0) * RW, dh = (g.player().heading - h0) * 180 / 3.14159265358979;
            const double flanc = std::abs(g.player().playerX - xPolice) * RW - touche;   // jour entre les deux carrosseries (m)
            char l[256];
            std::snprintf(l, sizeof l, "WFS-461 banc police (%s) image %02d\njoueur %.0f km/h, volant %+.1f\ndeplacement lateral %+.2f m, cap %+.1f deg\njour au flanc de la police %+.2f m",
                          cockpit ? "cockpit" : "poursuite", k, g.player().kmh(), k < 4 ? -0.5 : 0.0, dx, dh, flanc);
            g_legende = l;
            char nom[64]; std::snprintf(nom, sizeof nom, "%s%02d", base.c_str(), k);
            image(nom);
            WFS_LOG("capture %s : joueur %.1f km/h, déplacement latéral %+.3f m, cap %+.2f°, jour au flanc %+.3f m", nom, g.player().kmh(), dx, dh, flanc);
        }
        g_legende.clear();
        poursuiteBancPolice(false, 0, 0); g.modeManette = false;
    }

    // ---------- 2. rattrapage : joueur vers 100 km/h dès le départ, la police (220 m derrière) arrive ----------
    choisirVue(VueCamera::Poursuite);
    if (demarrer()) {
        const float cible = 100;
        double vP = 0, e = 0;
        auto rouler = [&](int n) {   // vitesse de la police mesurée à chaque mise à jour (1/60 s)
            for (int i = 0; i < n; i++) { g.setAutopilot(g.player().kmh() < cible); g.update(PAS); t += PAS; vitesses(PAS, vP, e); }
        };
        vMaxToutes = 0;
        g.camMode = 2;
        for (int k = 0; k < 12; k++) {
            rouler(120);   // une image toutes les 2 s
            viser(9, 3.5, 3.2f);   // de devant, un peu à droite : la police derrière le joueur
            char l[256];
            std::snprintf(l, sizeof l, "WFS-461 banc police : rattrapage, %d s\njoueur %.0f km/h\npolice la plus proche : %.0f km/h (fiche 208 : %.0f km/h)\nderriere le joueur : %.0f m\nmaxi police depuis le depart : %.0f km/h",
                          2 * (k + 1), g.player().kmh(), vP, vMaxPolice, e, vMaxToutes);
            g_legende = l;
            char nom[64]; std::snprintf(nom, sizeof nom, "police-rattrapage-%02d", k);
            image(nom);
            WFS_LOG("capture %s : joueur %.1f km/h, police la plus proche %.1f km/h à %.1f m derrière, maxi %.1f km/h", nom, g.player().kmh(), vP, e, vMaxToutes);
        }
        WFS_LOG("police (WFS-461) : rattrapage, vitesse maxi mesurée %.1f km/h (fiche %.0f km/h)", vMaxToutes, vMaxPolice);
        if (vMaxToutes > vMaxPolice * 1.01) { WFS_ERR("police (WFS-461) : %.1f km/h, au-delà de sa fiche (%.0f km/h)", vMaxToutes, vMaxPolice); ecarts++; }
        g_legende.clear(); g.camMode = 0;
    }
    choisirVue(vue0); modeCourse() = mode0; courseBenchRules(false);
    unsetenv("WFS_DEPART"); unsetenv("WFS_MOMENT");
    return ecarts;
}

}   // namespace wfs
