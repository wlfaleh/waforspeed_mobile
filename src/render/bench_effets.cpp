// WaForSpeed — scènes du banc d'essai des effets de conduite (WFS-65) : voir bench_effets.h.
#include "bench_effets.h"
#include <cmath>
#include <cstdlib>
#include "../core/log.h"
#include "../game/camera_vue.h"
#include "../game/commandes_tactiles.h"
#include "../game/course_system.h"
#include "../game/direction.h"
#include "../game/game.h"
#include "../game/ressenti.h"
#include "../game/traffic_system.h"
#include "../ui/ecrans/reglages.h"
#include "effets_systeme.h"
#include "renderer.h"

namespace wfs {

namespace {

// WFS-107 (RG-03) : braquage aux flèches sur route dégagée (trafic retiré à chaque image : la course en ajoute aux points
// de passage), vue Proche (la caméra la plus près de la fumée : pire cas pour un voile), à 100, 150 (WFS-224) et 250 km/h, de jour et
// de nuit : captures braquage-gauche-<vitesse>-<moment>.png ; WFS-224 : aucune fumée attendue (pas de vrai dérapage)
// puis braquage-droite-…, chacune après 0,5 s de flèche tenue. 100 et 150 km/h : élan au pilote automatique, virages en
// roue libre (la vitesse reste près de la consigne) ; 250 km/h : la Golf plafonne vers 236 km/h sans nitro : élan
// jusqu'à 233 km/h, puis nitro jusqu'à 250 (réserve de départ 0,4 : ≈ 1,5 s) par l'entrée manette, qui garde la voiture
// au milieu comme le pilote automatique (lui coupe le nitro) ; virages gaz tenu. Commandes tactiles posées au centre de
// leur dessin (dispositionCommandes, boutonsFleches).
void scenesBraquage(Game& g, Renderer& r, const std::string& out) {
    const ModeDirection direction = reglagesRessenti().direction;
    const VueCamera vue = vueCamera();
    reglagesRessenti().direction = ModeDirection::Fleches;
    choisirVue(VueCamera::Proche);
    enum { DOIGT_GAZ = 5, DOIGT_FLECHE = 6 };
    for (const char* moment : {"jour", "nuit"}) {
        setenv("WFS_MOMENT", moment, 1);
        if (!g.init(r, "golf", "ville")) { WFS_ERR("effets : braquage de %s, chargement impossible", moment); continue; }
        g.camMode = 0;
        double t = 900;
        auto image = [&](bool rendu) {
            if (Traffic* tr = activeTrafficMut()) tr->cars.clear();   // route dégagée (RG-02)
            g.update(1.0 / 60); t += 1.0 / 60;
            if (rendu) g.render(t);
        };
        const float W = (float)r.width(), H = (float)r.height();
        const DispositionCommandes d = dispositionCommandes(W, H, ecrans::margesCommandes(g.marges()));
        const BoutonsFleches b = boutonsFleches(W, H, g.marges());
        for (const int vitesse : {100, 150, 250}) {   // WFS-224 (RG-04) : 100 et 150 km/h (vidéo de l'utilisateur : 107 à 158)
            const bool gaz = vitesse == 250;
            g.setAutopilot(true);
            int n = 0;
            for (; n < 60 * 90 && g.player().kmh() < (gaz ? 233 : vitesse); n++) image(n % 30 == 0);
            g.setAutopilot(false);
            if (gaz) {
                g.modeManette = true;
                for (int i = 0; i < 60 * 4 && g.player().kmh() < 250; i++, n++) {
                    const Player& p = g.player();
                    g.manette = {clampf((float)(-p.playerX * 3.0 - p.heading * 4.0), -1.f, 1.f), true, false, true};
                    image(i % 15 == 0);
                }
                g.onTouch({DOIGT_GAZ, d.gaz.cx(), d.gaz.cy(), true});
                g.manette = {}; g.modeManette = false;
            }
            WFS_LOG("effets : braquage %d km/h (%s) : élan de %.1f s, %.0f km/h, nitro %.2f", vitesse, moment, n / 60.0, g.player().kmh(), g.player().nitro);
            for (const bool gauche : {true, false}) {
                const float x = (gauche ? b.xg : b.xd) + b.c / 2, y = b.y + b.c / 2;
                g.onTouch({DOIGT_FLECHE, x, y, true});
                for (int i = 0; i < 30; i++) image(i % 6 == 5);
                const std::string nom = std::string("braquage-") + (gauche ? "gauche-" : "droite-") + std::to_string(vitesse) + "-" + moment;
                g.render(t + 0.001);
                r.capturePng(out + "/" + nom + ".png");
                const Player& p = g.player();
                WFS_LOG("capture %s : %.0f km/h, braquage %.2f, accélération latérale %.1f m/s², dérive %.1f° (fumée au-delà de 8°), sigma arrière %.2f (> 1), %d bouffées de fumée, %d étincelles, %zu voitures",
                        nom.c_str(), p.kmh(), p.steer, std::abs(p.latAcc), std::atan2(p.monde.v(), std::max(std::abs(p.monde.u()), 0.1)) * 57.29578, p.monde.sigmaAr,
                        effetsFumee(), effetsEtincelles(),
                        activeTraffic() ? activeTraffic()->cars.size() : (size_t)0);
                g.onTouch({DOIGT_FLECHE, x, y, false});
            }
            if (gaz) g.onTouch({DOIGT_GAZ, d.gaz.cx(), d.gaz.cy(), false});
        }
        g.shutdown();
    }
    unsetenv("WFS_MOMENT");
    reglagesRessenti().direction = direction;
    choisirVue(vue);
}

} // namespace

void benchEffets(Game& g, Renderer& r, const std::string& out) {
    effetsBanc(true);
    double t = 700;
    auto image = [&](bool rendu) { g.update(1.0 / 60); t += 1.0 / 60; if (rendu) g.render(t); };
    // WFS-224 (RG-02, RG-04) : la scène du choc tourne aussi de nuit (fumée du tête-à-queue, vrai dérapage, éclairée par
    // les seuls feux) : accident-fumee-nuit.png ; de jour, mêmes captures qu'avant
    for (const char* moment : {"jour", "nuit"}) {
        const bool nuit = moment[0] == 'n';
        if (nuit) setenv("WFS_MOMENT", "nuit", 1);
        if (!g.init(r, "golf", "ville")) { WFS_ERR("effets : chargement impossible"); unsetenv("WFS_MOMENT"); effetsBanc(false); return; }
        g.setAutopilot(true); g.camMode = 1;   // caméra haute : on voit l'avant de la voiture et la route devant
        for (int i = 0; i < 120; i++) image(i % 8 == 0);
        // choc : 80 km/h plus vite que la voiture de devant, dans sa file, sans freiner (même scène que trafic-choc)
        const Course* c = activeCourse();
        const int avant = c ? c->accidents + c->bumps + c->scrapes : 0;
        trafficBenchPlace(14, 0, 80, true);
        // WFS-292 (RG-03) : le pilote automatique est coupé dès le placement : il évitait la voiture de devant (changement
        // de file, journal de la CI c9eda61 : x 0,44 → 0,77 à 171 km/h, 0 accident en 3 s) ; la Golf file tout droit sur
        // son élan et la percute (règles des chocs de WFS-256 inchangées)
        g.setAutopilot(false);
        int n = 0;
        for (; n < 180; n++) { image(true); if (c && c->accidents + c->bumps + c->scrapes > avant) break; }
        const int emises = effetsDernierChoc();
        // WFS-101 : la caméra de poursuite suit le tête-à-queue et la Golf cache le point de contact (mesuré en rendant la
        // même image avec et sans étincelles : 0 pixel visible 2 images après le choc, ≈ 200 ensuite) ; capture de côté (vue
        // vitrine à 6,5 m de la Golf, perpendiculaire à la route, côté chaussée : ≈ 2 200 pixels), 8 images après le choc
        // (ralenti × 0,35 : ≈ 0,05 s de jeu) : la gerbe s'est ouverte entre les deux voitures ; puis la caméra haute reprend
        for (int k = 0; k < 8; k++) image(true);
        const float cap = effetsCapDernierChoc(), cote = g.player().playerX > 0 ? -1.f : 1.f;   // vers le milieu de la route
        g.camMode = 2; g.orbit = std::atan2(cote * std::cos(cap), cote * std::sin(cap));
        g.render(t + 0.001);
        if (!nuit) r.capturePng(out + "/choc-etincelles.png");
        g.camMode = 1;
        WFS_LOG("capture choc-etincelles (vue de côté) : choc après %.2f s (accidents %d, chocs %d, frottements %d), %d étincelles émises au choc, %d vivantes, %.0f km/h",
                n / 60.0, c ? c->accidents : 0, c ? c->bumps : 0, c ? c->scrapes : 0, emises, effetsEtincelles(), g.player().kmh());
        // fumée : voiture percutée en glissade (capot qui fume) et pneus de la voiture en tête-à-queue
        for (int k = 0; k < 55; k++) image(k % 4 == 0);   // même instant qu'avant WFS-101 (63 images après le choc)
        g.render(t + 0.001);
        r.capturePng(out + (nuit ? "/accident-fumee-nuit.png" : "/accident-fumee.png"));
        WFS_LOG("capture accident-fumee (%s) : %d bouffées de fumée, %d étincelles", moment, effetsFumee(), effetsEtincelles());
        g.shutdown();
    }
    unsetenv("WFS_MOMENT");
    // nitro : appui maintenu sur le bouton nitro (au-dessus de l'accélérateur) ; le pilote automatique est coupé pendant
    // l'appui (il force nitro = false, comme autopilot() du jeu web) : ligne droite, 0,75 s
    g.init(r, "golf", "ville"); g.setAutopilot(true); g.camMode = 0;
    for (int i = 0; i < 90; i++) image(i % 8 == 0);
    const float W = (float)r.width(), H = (float)r.height();
    g.setAutopilot(false);
    // WFS-374 (ANO-233, accroche) : appui au centre du bouton nitro (avant : coin haut droit, zone de nitro retirée)
    const DispositionCommandes dc = dispositionCommandes(W, H, g.marges());
    const float xn = dc.nitro.cx(), yn = dc.nitro.cy();
    g.onTouch({7, xn, yn, true});
    for (int i = 0; i < 45; i++) image(i % 8 == 0);
    g.render(t + 0.001);
    r.capturePng(out + "/nitro-lignes.png");
    WFS_LOG("capture nitro-lignes : nitro %s, réserve %.2f, %.0f km/h", g.player().nitroOn ? "active" : "inactive", g.player().nitro, g.player().kmh());
    g.onTouch({7, xn, yn, false});
    g.shutdown();
    // WFS-224 (RG-02, RG-04) : aspect des volutes en vrai dérapage (fumée forcée 1,2 s, la Golf roule au pilote automatique
    // vers 90 km/h, vue Proche, juste derrière la voiture) de jour et de nuit : fumee-derapage-<moment>.png
    {
        const VueCamera vue = vueCamera();
        choisirVue(VueCamera::Proche);
        for (const char* moment : {"jour", "nuit"}) {
            setenv("WFS_MOMENT", moment, 1);
            if (!g.init(r, "golf", "ville")) continue;
            g.setAutopilot(true); g.camMode = 0;
            for (int i = 0; i < 60 * 30 && g.player().kmh() < 90; i++) {
                if (Traffic* tr = activeTrafficMut()) tr->cars.clear();
                image(i % 30 == 0);
            }
            effetsBancDerapage(true);
            for (int i = 0; i < 72; i++) { if (Traffic* tr = activeTrafficMut()) tr->cars.clear(); image(i % 6 == 5); }
            g.render(t + 0.001);
            r.capturePng(out + "/fumee-derapage-" + moment + ".png");
            WFS_LOG("capture fumee-derapage-%s : %.0f km/h, %d bouffées de fumée", moment, g.player().kmh(), effetsFumee());
            // WFS-292 (RG-01, revue du lead dev) : suite de jour, 0,5 s et 1 s plus tard (la nappe reste accrochée aux roues)
            for (int k = 1; moment[0] == 'j' && k <= 2; k++) {
                for (int i = 0; i < 30; i++) { if (Traffic* tr = activeTrafficMut()) tr->cars.clear(); image(i % 6 == 5); }
                g.render(t + 0.001);
                r.capturePng(out + "/fumee-derapage-jour-" + (k == 1 ? "05s" : "1s") + ".png");
                WFS_LOG("capture fumee-derapage-jour-%s : %.0f km/h, %d bouffées de fumée", k == 1 ? "05s" : "1s", g.player().kmh(), effetsFumee());
            }
            effetsBancDerapage(false);
            g.shutdown();
        }
        unsetenv("WFS_MOMENT");
        choisirVue(vue);
    }
    scenesBraquage(g, r, out);   // WFS-107 : fumée au braquage (« feu blanc »)
    effetsBanc(false);
}

} // namespace wfs

