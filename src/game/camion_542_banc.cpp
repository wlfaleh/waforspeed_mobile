// WaForSpeed — banc d'essai de WFS-542 : voir camion_542_banc.h.
#include "camion_542_banc.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "../core/log.h"
#include "../core/system.h"
#include "../render/renderer.h"
#include "../ui/text.h"
#include "camera_vue.h"
#include "cars.h"
#include "course_system.h"
#include "game.h"
#include "poursuite.h"
#include "roues_banc.h"   // poursuiteBancPolice (accroche WFS-452) : pose de la 1re voiture de police

namespace wfs {
namespace {
std::string g_legende542;   // banc seulement : texte écrit sur l'image (vide : rien)

// banc seulement : légende des captures, par-dessus l'interface ; inactive hors banc
class LegendeBanc542 : public System {
public:
    const char* name() const override { return "legende-banc-542"; }
    void drawUi(Renderer& r, const GameView& g) override {
        if (g_legende542.empty()) return;
        const float u = g.uiHeight / 100.f, px = 2.6f * u, x = 2 * u, y = 45 * u;
        size_t n = 1; for (char c : g_legende542) n += c == '\n';
        r.uiRect(x - u, y - u, 70 * u, (float)n * px * 1.35f + 2 * u, {0, 0, 0, 0.72f});
        size_t p0 = 0; float yy = y;
        for (size_t p; (p = g_legende542.find('\n', p0)) != std::string::npos || p0 < g_legende542.size(); p0 = p + 1) {
            if (p == std::string::npos) p = g_legende542.size();
            ui::text(r, x, yy, px, g_legende542.substr(p0, p - p0), {1, 0.86f, 0.25f, 1}, ui::GAUCHE, 0, false);
            yy += px * 1.35f;
        }
    }
};
}   // namespace
WFS_SYSTEM(LegendeBanc542, 497)   // interface (400-499) : par-dessus le HUD

int benchCamion542(Game& g, Renderer& r, const std::string& out) {
    const ModeCourse mode0 = modeCourse();
    const VueCamera vue0 = vueCamera();
    constexpr double PAS = 1.0 / 60;
    const double sCamion = poursuite::BANC_CAMION_KM * 1000 / M;   // camion du programme du banc : files de gauche et du milieu barrées
    courseBenchRules(true);
    double t = 1500;
    Player& joueur = const_cast<Player&>(g.player());   // banc seulement : voiture placée
    auto image = [&](const std::string& nom) { g.render(t); g.render(t + PAS); t += PAS; r.capturePng(out + "/" + nom + ".png"); };
    auto viser = [&](double avantM, double droiteM, float hauteur) {   // caméra libre autour de la voiture du joueur
        const Game::Vue v = g.calculerVue(t);
        const vec3 f = normalize(vec3{v.fwd.x, 0, v.fwd.z}), d = cross(f, {0, 1, 0});
        const vec3 o = f * (float)avantM + d * (float)droiteM;
        g.orbit = std::atan2(o.x, o.z); g.orbitRecul = length(o); g.orbitHauteur = hauteur;
    };
    auto rouler = [&](int n, double kmh) {
        for (int i = 0; i < n; i++) { DriveInput in; in.gas = joueur.kmh() < kmh; in.brake = kmh <= 0 && joueur.kmh() > 0.5; g.manette = in; g.update(PAS); t += PAS; if (i % 12 == 0) g.render(t); }
    };
    // place le joueur à devantM du camion (m, distance au centre du camion), au latéral x ; le camion est posé 450 m avant
    auto placer = [&](double devantM, double x) {
        joueur.dist = joueur.position = sCamion - (devantM + 450) / M; joueur.playerX = x; joueur.speed = 0;
        rouler(30, 0);   // le camion est posé
        joueur.dist = joueur.position = sCamion - devantM / M; joueur.playerX = x; joueur.speed = 0; joueur.heading = 0;
        rouler(10, 0);
    };
    auto demarrer = [&](const char* moment, VueCamera vue) {
        setenv("WFS_DEPART", "0", 1); setenv("WFS_MOMENT", moment, 1);
        modeCourse() = ModeCourse::Poursuite; choisirVue(vue); poursuiteBancFixe(true);
        if (!g.init(r, "golf", "plage")) { WFS_ERR("camion-542 : chargement de la Corniche impossible"); return false; }
        g.allerA(Ecran::Course); g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{}; g.camMode = 0;
        for (int i = 0; i < 4 * 60; i++) { g.update(PAS); t += PAS; if (i % 12 == 0) g.render(t); }   // décompte
        return true;
    };
    auto finir = [&]() { g_legende542.clear(); g.manette = DriveInput{}; g.modeManette = false; g.camMode = 0; poursuiteBancFixe(false); g.shutdown(); };
    auto legende = [&](const char* scene, int k, const char* texte) {
        char l[512];
        std::snprintf(l, sizeof l, "WFS-542 banc camion : %s, image %02d\njoueur (Golf 4,28 x 1,80 m) a %.1f m du camion, %.0f km/h\n%s",
                      scene, k, (sCamion - joueur.dist) * M, joueur.kmh(), texte);
        g_legende542 = l;
    };
    const char* seule = std::getenv("WFS_SCENE");
    auto scene = [&](const char* nom) { return !seule || !*seule || std::string(seule) == nom; };
    const char* TEXTE = "camion en travers : vrai porteur 7,5 t attendu\n(6,19 m x 2,93 m, roues de 77 cm)";

    // ---------- 1 et 2. nuit et jour, vue Poursuite : le joueur arrive dans la file libre, police derrière lui ----------
    for (const char* moment : {"nuit", "jour"}) {
        if (!scene(moment)) continue;
        if (!demarrer(moment, VueCamera::Poursuite)) break;
        placer(42, poursuite::FILES[2] - 0.08);
        poursuiteBancPolice(true, -24, poursuite::FILES[2]); g.update(PAS); t += PAS; poursuiteBancPolice(false, 0, 0);
        int k = 0;
        for (double cible : {30.0, 20.0, 12.0}) {
            for (int i = 0; i < 60 * 20 && (sCamion - joueur.dist) * M > cible; i++) { DriveInput in; in.gas = joueur.kmh() < 18; g.manette = in; g.update(PAS); t += PAS; if (i % 12 == 0) g.render(t); }
            legende(moment, k, TEXTE);
            char nom[64]; std::snprintf(nom, sizeof nom, "camion542-%s-%02d", moment, k++); image(nom);
        }
        finir();
    }
    // ---------- 3. cockpit : voiture arrêtée à 10 m du flanc du camion, file fermée puis file du milieu (barrées) ----------
    if (scene("cockpit") && demarrer("jour", VueCamera::Cockpit)) {
        const double avant = 10 + 2.14 + 1.11;   // pare-chocs avant de la Golf (4,28 m) à 10 m du flanc
        placer(avant, poursuite::FILES[0] * 0.75);
        legende("cockpit", 0, TEXTE); image("camion542-cockpit-00");
        placer(avant, poursuite::FILES[1]);
        legende("cockpit", 1, TEXTE); image("camion542-cockpit-01");
        finir();
    }
    // ---------- 4. vitrine : la Golf arrêtée le long du camion, vue de côté puis de 3/4 ----------
    if (scene("vitrine") && demarrer("jour", VueCamera::Poursuite)) {
        // la Golf arrêtée devant le flanc du camion, pare-chocs à 0,5 m, dans la file fermée (même sol, même éclairage)
        placer(2.14 + 1.11 + 0.5, poursuite::FILES[0] * 0.8);
        g.camMode = 2;
        struct Vue { double avant, droite; float haut; };
        int k = 0;
        for (const Vue& v : {Vue{1.5, 11, 1.8f}, Vue{-9, 10, 4.f}, Vue{11, 8, 5.f}}) {   // de côté, de 3/4 arrière, de 3/4 avant
            viser(v.avant, v.droite, v.haut);
            legende("vitrine", k, "le camion seul a cote de la Golf (4,28 m, 1,48 m de haut)\ncamion attendu : 6,19 m de long, 2,93 m de haut");
            char nom[64]; std::snprintf(nom, sizeof nom, "camion542-vitrine-%02d", k++); image(nom);
        }
        finir();
    }
    // ---------- 5. retrait (RG-04) : le joueur 60 m après le camion, caméra tournée vers l'arrière ----------
    if (scene("retrait") && demarrer("jour", VueCamera::Poursuite)) {
        placer(-20, poursuite::FILES[2]);
        joueur.dist = joueur.position = sCamion + 60 / M; rouler(30, 0);
        g.camMode = 2; viser(14, 4, 6.f);   // caméra devant la voiture, tournée vers l'arrière (le camion était à 60 m)
        legende("retrait", 0, "60 m apres le camion, vue vers l'arriere :\nle camion doit etre retire (RG-04 : 50 m derriere)");
        image("camion542-retrait-00");
        finir();
    }
    choisirVue(vue0); modeCourse() = mode0; courseBenchRules(false);
    unsetenv("WFS_DEPART"); unsetenv("WFS_MOMENT");
    WFS_LOG("camion-542 : bancs terminés");
    return 0;
}

}   // namespace wfs
