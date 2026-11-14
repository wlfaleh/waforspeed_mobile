// WaForSpeed — banc d'essai de WFS-540 : voir police_540_banc.h.
#include "police_540_banc.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "../audio/voitures_sons.h"   // poursuiteVoitures (accroche WFS-272) : police en lecture seule
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
#include "roues_banc.h"   // poursuiteBancPolice (accroche WFS-452) : pose de la 1re voiture de police

namespace wfs {
namespace {
std::string g_legende540;   // banc seulement : texte écrit en haut à gauche de l'image (vide : rien)

// banc seulement : légende des captures (mesures de la scène), par-dessus l'interface ; inactive hors banc
class LegendeBanc540 : public System {
public:
    const char* name() const override { return "legende-banc-540"; }
    void drawUi(Renderer& r, const GameView& g) override {
        if (g_legende540.empty()) return;
        const float u = g.uiHeight / 100.f, px = 2.6f * u, x = 2 * u, y = 45 * u;   // sous le bandeau de la Poursuite (ARRESTATION…)
        size_t n = 1; for (char c : g_legende540) n += c == '\n';
        r.uiRect(x - u, y - u, 66 * u, (float)n * px * 1.35f + 2 * u, {0, 0, 0, 0.72f});
        size_t p0 = 0; float yy = y;
        for (size_t p; (p = g_legende540.find('\n', p0)) != std::string::npos || p0 < g_legende540.size(); p0 = p + 1) {
            if (p == std::string::npos) p = g_legende540.size();
            ui::text(r, x, yy, px, g_legende540.substr(p0, p - p0), {1, 0.86f, 0.25f, 1}, ui::GAUCHE, 0, false);
            yy += px * 1.35f;
        }
    }
};
}   // namespace
WFS_SYSTEM(LegendeBanc540, 498)   // interface (400-499) : par-dessus le HUD

int benchPolice540(Game& g, Renderer& r, const std::string& out) {
    const ModeCourse mode0 = modeCourse();
    const VueCamera vue0 = vueCamera();
    constexpr double PAS = 1.0 / 60;   // une mise à jour = 2 pas de conduite de 1/120 s
    const CarDef& police = carById("p208");
    courseBenchRules(true);
    double t = 1500;
    Player& joueur = const_cast<Player&>(g.player());   // banc seulement : voiture placée, vitesse imposée
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
    auto demarrer = [&](const char* carte, const char* moment, VueCamera vue, bool programmeFixe) {
        setenv("WFS_DEPART", "0", 1); setenv("WFS_MOMENT", moment, 1);
        modeCourse() = ModeCourse::Poursuite; choisirVue(vue); poursuiteBancFixe(programmeFixe);
        if (!g.init(r, "golf", carte)) { WFS_ERR("police-540 : chargement de %s impossible", carte); return false; }
        g.allerA(Ecran::Course); g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{}; g.camMode = 0;
        for (int i = 0; i < 4 * 60; i++) { g.update(PAS); t += PAS; if (i % 12 == 0) g.render(t); }   // décompte
        return true;
    };
    auto finir = [&]() { g_legende540.clear(); g.manette = DriveInput{}; g.modeManette = false; g.camMode = 0; poursuiteBancFixe(false); g.shutdown(); };
    // la voiture de police mobile la plus proche du joueur
    auto proche = [&]() -> const poursuite::Police* {
        const std::vector<poursuite::Police>* ps = poursuiteVoitures(nullptr);
        const poursuite::Police* best = nullptr; double bd = 1e9;
        if (ps) for (const poursuite::Police& p : *ps) {
            if (p.fixe) continue;
            const double d = std::hypot((p.rd - joueur.dist) * M, (p.x - joueur.playerX) * RW);
            if (d < bd) { bd = d; best = &p; }
        }
        return best;
    };
    // pose la 1re voiture de police à ecartM du joueur (m, < 0 : derrière), au latéral x, à la vitesse du joueur, puis la lâche
    auto poserPolice = [&](double ecartM, double x) {
        poursuiteBancPolice(true, ecartM, x); g.update(PAS); t += PAS; poursuiteBancPolice(false, 0, 0);
    };
    // n mises à jour avec les commandes données par cmd() à chaque mise à jour
    auto rouler = [&](int n, auto cmd) { for (int i = 0; i < n; i++) { g.manette = cmd(); g.update(PAS); t += PAS; if (i % 12 == 0) g.render(t); } };
    auto legende = [&](const char* scene, int k, const char* texte) {
        const poursuite::Police* p = proche();
        char l[512];
        if (p) {
            const double dz = (p->rd - joueur.dist) * M, dx = (p->x - joueur.playerX) * RW;
            std::snprintf(l, sizeof l, "WFS-540 banc police : %s, image %02d\njoueur %.0f km/h%s\npolice la plus proche : %.0f km/h\n%+.1f m devant, %+.1f m a droite\n%s",
                          scene, k, joueur.kmh(), joueur.reversing ? " (marche arriere)" : "", p->v * M * 3.6, dz, dx, texte);
        } else std::snprintf(l, sizeof l, "WFS-540 banc police : %s, image %02d\njoueur %.0f km/h\n%s", scene, k, joueur.kmh(), texte);
        g_legende540 = l;
    };
    // WFS_SCENE (facultatif) : une seule scène (contourne, repart, arrestation, choc, camion, recul)
    const char* seule = std::getenv("WFS_SCENE");
    auto scene = [&](const char* nom) { return !seule || !*seule || std::string(seule) == nom; };
    auto tenir = [&](double kmh, float volant) { return [&joueur, kmh, volant]() { DriveInput in; in.gas = joueur.kmh() < kmh; in.steer = volant; return in; }; };

    // ---------- 1. contourne (ANO-272) : le joueur change de file, la police derrière le suit ----------
    if (scene("contourne") && demarrer("plage", "jour", VueCamera::Poursuite, false)) {
        rouler(60 * 6, tenir(30, 0));
        poserPolice(-12, joueur.playerX);
        rouler(30, tenir(30, 0));
        g.camMode = 2;
        for (int k = 0; k < 16; k++) {
            const float volant = k >= 1 && k < 6 ? -0.7f : 0.f;   // changement de file à gauche (1 s), puis volant lâché (l'aide tient la file)
            rouler(12, tenir(30, volant));   // 0,2 s
            viser(-13, 0, 34.f);             // de haut, derrière le joueur : la police (12 à 15 m derrière lui) est dans l'image
            legende("contourne (vue de dessus)", k, "le joueur change de file : la police doit tourner,\npas glisser de cote");
            char nom[64]; std::snprintf(nom, sizeof nom, "police540-contourne-%02d", k); image(nom);
        }
        finir();
    }
    // ---------- 2. repart (ANO-267) : joueur arrêté, police collée devant lui, GAZ appuyé ; vue Poursuite de nuit, puis Cockpit ----------
    struct Repart { const char* nom; VueCamera vue; };
    for (const Repart& rp : {Repart{"repart-nuit", VueCamera::Poursuite}, Repart{"repart-cockpit", VueCamera::Cockpit}}) {
        if (!scene("repart")) break;
        if (!demarrer("plage", "nuit", rp.vue, false)) break;
        rouler(60 * 4, tenir(20, 0));
        poserPolice((carById("golf").d.L + police.d.L) / 2 + 0.1, joueur.playerX);   // pare-chocs contre pare-chocs, devant
        joueur.speed = 0;                                                            // le joueur est arrêté contre elle
        rouler(60, [] { return DriveInput{}; });   // 1 s sans rien toucher
        for (int k = 0; k < 14; k++) {
            rouler(15, [] { DriveInput in; in.gas = true; return in; });   // GAZ tenu, 0,25 s par image
            legende(rp.nom, k, "joueur arrete, police collee devant lui,\nGAZ appuye : il doit repartir en 2 s au plus");
            char nom[64]; std::snprintf(nom, sizeof nom, "police540-%s-%02d", rp.nom, k); image(nom);
        }
        finir();
    }
    // ---------- 3. arrestation (ANO-268) : joueur arrêté, la police arrive ----------
    if (scene("arrestation") && demarrer("plage", "jour", VueCamera::Poursuite, false)) {
        rouler(60 * 4, tenir(20, 0));
        poserPolice(-30, joueur.playerX);
        joueur.speed = 0;
        for (int k = 0; k < 20; k++) {
            rouler(30, [] { return DriveInput{}; });   // 0,5 s par image, rien touché
            legende("arrestation", k, "joueur arrete, la police arrive :\ncompte a rebours ARRESTATION puis ARRETE");
            char nom[64]; std::snprintf(nom, sizeof nom, "police540-arrestation-%02d", k); image(nom);
        }
        rouler(90, [] { return DriveInput{}; });
        g_legende540.clear(); image("police540-arrestation-fin");
        finir();
    }
    // ---------- 4. choc de face (ANO-271) : barrage de police (programme du banc, 0,45 km), 30 km/h, sans frein ----------
    if (scene("choc") && demarrer("plage", "jour", VueCamera::Poursuite, true)) {
        joueur.playerX = 0;   // file du milieu : barrée
        int k = 0;
        for (int i = 0; i < 60 * 60 && k < 8; i++) {
            DriveInput in; in.gas = joueur.kmh() < 30;   // GAZ (tenu à 30 km/h), jamais de frein
            g.manette = in; g.update(PAS); t += PAS;
            const poursuite::Police* p = nullptr;
            if (const std::vector<poursuite::Police>* ps = poursuiteVoitures(nullptr))
                for (const poursuite::Police& q : *ps) if (q.fixe && std::abs(q.x - joueur.playerX) < 0.3 && q.rd > joueur.dist - 2 / M) { p = &q; break; }
            const double avant = p ? (p->rd - joueur.dist) * M : 1e9;
            if (avant < 9 && i % 9 == 0) {   // de 9 m avant le choc à 1 s après, une image toutes les 0,15 s
                char l[256]; std::snprintf(l, sizeof l, "WFS-540 banc police : choc de face, image %02d\njoueur %.0f km/h, GAZ, aucun frein\nbarrage a %.1f m\nle HUD ne doit jamais afficher R", k, joueur.kmh(), avant);
                g_legende540 = l;
                char nom[64]; std::snprintf(nom, sizeof nom, "police540-choc-%02d", k++); image(nom);
            }
        }
        if (k == 0) WFS_ERR("police-540 : barrage jamais atteint");
        finir();
    }
    // ---------- 5. camion en travers (RG-09) : programme du banc, camion à 1,95 km (files de gauche et du milieu barrées) ----------
    if (scene("camion") && demarrer("plage", "jour", VueCamera::Poursuite, true)) {
        joueur.dist = joueur.position = 1.55 * 1000 / M;   // 1er tour de la Corniche (5,5 km) : position = distance
        rouler(60, tenir(15, 0));   // le camion est posé 450 m avant
        joueur.dist = joueur.position = (poursuite::BANC_CAMION_KM * 1000 + 25) / M; joueur.playerX = poursuite::FILES[2];
        joueur.speed = 0;
        rouler(20, [] { return DriveInput{}; });
        poserPolice(-55, poursuite::FILES[0]);   // file de gauche, barrée par le camion
        g.camMode = 2;
        for (int k = 0; k < 16; k++) {
            rouler(20, tenir(8, 0));   // 0,33 s ; le joueur roule au pas, passé le camion
            viser(-75, 6, 45.f);        // de haut, loin derrière le joueur, regard vers lui : la police (≈ 40 à 60 m) puis le camion (25 m)
            legende("camion en travers (vue de haut)", k, "la police derriere le camion : elle garde sa file\nou s'arrete, sans aller-retour ni entrer dedans");
            char nom[64]; std::snprintf(nom, sizeof nom, "police540-camion-%02d", k); image(nom);
        }
        finir();
    }
    // ---------- 6. recul (RG-10) : Campagne de jour, le joueur recule à 30 km/h à côté de la police ----------
    if (scene("recul") && demarrer("campagne", "jour", VueCamera::Poursuite, false)) {
        rouler(60 * 6, tenir(30, 0));
        const double x = std::clamp(joueur.playerX, -0.25, 0.6);
        joueur.playerX = x;
        poserPolice(0.5, x - poursuite::COTE_X);   // à côté, à gauche, à la même vitesse
        joueur.speed = 0;
        g.camMode = 2;
        for (int k = 0; k < 16; k++) {
            rouler(20, [] { DriveInput in; in.brake = true; return in; });   // FREIN tenu : marche arrière (31 km/h au plus)
            viser(-2, 9, 22.f);
            legende("recul (vue de dessus)", k, "le joueur recule a cote de la police :\nelle ne doit pas reculer collee a lui ni glisser\nsur l'herbe");
            char nom[64]; std::snprintf(nom, sizeof nom, "police540-recul-%02d", k); image(nom);
        }
        finir();
    }

    choisirVue(vue0); modeCourse() = mode0; courseBenchRules(false);
    unsetenv("WFS_DEPART"); unsetenv("WFS_MOMENT");
    WFS_LOG("police-540 : bancs terminés");
    return 0;
}

}   // namespace wfs
