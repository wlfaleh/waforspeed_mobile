// Commandes tactiles agrandies, frein / marche arrière, gyroscope (WFS-108). Tests CPU, sans GPU.
// usage : test_commandes (aussi lancé par test_course : testCommandes(), la CI lance déjà test_course)
#include <cmath>
#include <cstdio>
#include "../src/game/direction.h"
#include "../src/game/player.h"
#include "../src/platform/gyroscope.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int testCommandes() {
    // ---------- RG-01 : tailles en dp, sans chevauchement, dans la zone sûre ----------
    struct Ecran { float W, H, dpi; Marges m; const char* nom; };
    const Ecran ecrans[] = {{2400, 1080, 420, {}, "téléphone 20:9 (420 ppp)"}, {2340, 1080, 440, {90, 0, 0, 0}, "téléphone à encoche"},
                            {1920, 1080, 400, {}, "téléphone 16:9"}, {1280, 720, 0, {}, "banc PC 720p"},
                            {2560, 1600, 320, {}, "tablette"}, {1600, 720, 280, {0, 0, 0, 24}, "petit téléphone"}};
    for (const Ecran& e : ecrans)
        for (const bool grand : {false, true}) {
            densiteCommandes() = e.dpi / 160.f; grandsBoutons() = grand;
            const DispositionCommandes d = dispositionCommandes(e.W, e.H, e.m);
            const BoutonsFleches b = boutonsFleches(e.W, e.H, e.m);
            const float dp = e.dpi > 0 ? e.dpi / 160.f : e.H / 420.f;   // pixels par dp
            const float gazDp = d.gaz.w / dp, freinDp = d.frein.w / dp, nitroDp = d.nitro.w / dp, flecheDp = b.c / dp;
            std::printf("  %s%s : gaz %.0f × %.0f dp, frein %.0f × %.0f dp, nitro %.0f dp, flèches %.0f dp\n", e.nom, grand ? " (grands)" : "",
                        gazDp, d.gaz.h / dp, freinDp, d.frein.h / dp, nitroDp, flecheDp);
            VERIFIE(gazDp >= 48 && freinDp >= 48 && nitroDp >= 48 && flecheDp >= 48 && d.gaz.h / dp >= 64 && d.frein.h / dp >= 64,
                    "%s : bouton de moins de 48 dp", e.nom);
            VERIFIE(gazDp >= 79 && freinDp >= 64, "%s : gaz sous 80 dp ou frein sous 64 dp", e.nom);
            if (grand) VERIFIE(freinDp >= 104 || e.W * 0.40f / 190.f < dp * 1.25f, "%s : « Grands » pas plus grands", e.nom);
            VERIFIE(d.gaz.x + d.gaz.w <= e.W - e.m.droite && d.gaz.y + d.gaz.h <= e.H - e.m.bas && d.frein.y + d.frein.h <= e.H - e.m.bas &&
                    d.nitro.y >= e.m.haut && d.frein.x + d.frein.w < d.gaz.x && d.nitro.y + d.nitro.h < d.frein.y,
                    "%s : pédales hors zone sûre ou qui se touchent", e.nom);
            VERIFIE(b.xg >= e.m.gauche && b.xg + b.c < b.xd && b.y + b.c <= e.H - e.m.bas && b.xd + b.c < d.nitro.x &&
                    dansZoneDirection(b.xd + b.c - 1, e.W, e.m), "%s : flèches hors zone ou sur les pédales", e.nom);
            // la zone de toucher déborde du dessin : 4 dp autour du frein et du gaz, 8 dp autour du nitro
            const Marges& m = e.m;
            VERIFIE(commandeSous(d.gaz.cx(), d.gaz.y - 4 * dp, e.W, e.H, m, false) == Commande::Gaz &&
                    commandeSous(d.frein.x - 4 * dp, d.frein.cy(), e.W, e.H, m, false) == Commande::Frein &&
                    commandeSous(d.nitro.cx(), d.nitro.y - 6 * dp, e.W, e.H, m, false) == Commande::Nitro &&
                    commandeSous(d.frein.cx(), d.frein.y + d.frein.h + 2, e.W, e.H, m, false) == Commande::Frein,
                    "%s : zone de toucher pas plus grande que le dessin", e.nom);
        }
    densiteCommandes() = 0; grandsBoutons() = false;
    std::printf("commandes : boutons ≥ 48 dp (gaz et frein ≥ 64), sans chevauchement, zone de toucher élargie : %s\n", echecs ? "ÉCHEC" : "OK");

    // ---------- RG-02 : frein maintenu à l'arrêt → marche arrière sans relâcher ; gaz → avant ----------
    {
        Track t; t.build(mapById("ville")); Player p; p.reset(carById("golf"));
        DriveInput in; in.gas = true;
        for (int i = 0; i < 120 * 4; i++) p.update(1.0 / 120, in, t, true);
        const double avant = p.kmh();
        in = {}; in.brake = true;
        int arret = -1, recul = -1;
        for (int i = 0; i < 120 * 10; i++) {
            p.update(1.0 / 120, in, t, true);
            if (arret < 0 && p.speed <= 0) arret = i;
            if (recul < 0 && p.reversing && p.speed < 0) recul = i;
        }
        std::printf("  frein maintenu depuis %.0f km/h : arrêt à %.2f s, marche arrière à %.2f s, %.1f km/h après 10 s (R : %s)\n", avant,
                    arret / 120.0, recul / 120.0, p.kmh(), p.reversing ? "oui" : "non");
        VERIFIE(avant > 60 && recul >= 0 && recul - arret <= 2 && p.speed < 0 && p.speed >= REV_MAX - 1e-6 && p.reversing,
                "frein maintenu : pas de marche arrière sans relâcher");
        in = {}; in.gas = true;
        double precedent = p.speed; bool monotone = true; int avantI = -1;
        for (int i = 0; i < 120 * 4; i++) {
            p.update(1.0 / 120, in, t, true);
            if (p.speed < precedent - 1e-9) monotone = false;
            precedent = p.speed;
            if (avantI < 0 && p.speed > 0) avantI = i;
        }
        std::printf("  gaz depuis la marche arrière : avant à %.2f s, %.0f km/h après 4 s\n", avantI / 120.0, p.kmh());
        VERIFIE(!p.reversing && avantI >= 0 && avantI < 120 && monotone && p.kmh() > 30, "gaz après la marche arrière : reprise pas fluide");
        // frein bref à l'arrêt : marche arrière tout de suite (aucune attente)
        Player q; q.reset(carById("golf")); in = {}; in.brake = true;
        q.update(1.0 / 60, in, t, true);
        VERIFIE(q.reversing && q.speed < 0, "frein à l'arrêt : marche arrière pas immédiate");
    }
    std::printf("commandes : frein maintenu → marche arrière, gaz → avant : %s\n", echecs ? "ÉCHEC" : "OK");

    // ---------- RG-03 : gyroscope (valeurs d'essai → braquage) ----------
    {
        const float deg = PI / 180.f;
        VERIFIE(braquageGyro(2 * deg, 0, 20, 3) == 0.f && braquageGyro(-2.9f * deg, 0, 20, 3) == 0.f, "zone morte : braquage");
        VERIFIE(std::fabs(braquageGyro(11.5f * deg, 0, 20, 3) - 0.5f) < 1e-4f && braquageGyro(25 * deg, 0, 20, 3) == 1.f &&
                braquageGyro(-40 * deg, 0, 20, 3) == -1.f, "sensibilité : 11,5° = 0,5, butées à ±1");
        VERIFIE(std::fabs(braquageGyro(-175 * deg, 178 * deg, 20, 3) - (7.f - 3.f) / 17.f) < 1e-4f, "passage de ±180° mal géré");
        // gravité publiée → angle ; tourner le téléphone dans le sens horaire (vu de face) = braquer à droite, en paysage
        // comme en paysage inversé
        for (const float signe : {1.f, -1.f}) {
            gyroscope::publier(9.81f * signe, 0); const float zero = gyroscope::etat().angle;
            const float a = 10 * deg, c = std::cos(a), s = std::sin(a);
            // rotation horaire de l'appareil de 10° : la gravité tourne de +10° dans son repère
            gyroscope::publier(9.81f * signe * c, 9.81f * signe * s);
            const float v = braquageGyro(gyroscope::etat().angle, zero, 20, 3);
            VERIFIE(std::fabs(v - 7.f / 17.f) < 1e-3f, "paysage %s : 10° horaire → %.3f (0,412 attendu)", signe > 0 ? "normal" : "inversé", v);
        }
        gyroscope::publier(0.5f, 0.3f);
        VERIFIE(!gyroscope::etat().valide, "téléphone à plat : angle accepté");
        // combinaison avec le tactile : gyroscope actif, inclinaison 20° ; réétalonnage au changement d'écran
        reglagesGyro() = {2, 1}; gyroscope::capteurPresent(true);
        gyroscope::publier(9.81f, 0); gyroscopeEtalonner();
        gyroscope::publier(9.81f * std::cos(20 * deg), 9.81f * std::sin(20 * deg));
        const float b20 = braquageIncline();
        gyroscopeEtalonner();   // nouveau départ (directionLacher) : le téléphone incliné devient le zéro
        const float apres = braquageIncline();
        std::printf("  gyroscope Moyen : 20° → %.2f ; après réétalonnage → %.2f\n", b20, apres);
        VERIFIE(b20 == 1.f && apres == 0.f && gyroscopeActif(), "gyroscope : braquage ou réétalonnage");
        reglagesGyro() = {}; VERIFIE(!gyroscopeActif() && braquageIncline() == 0.f, "gyroscope coupé : braquage");
    }
    std::printf("commandes : gyroscope (zone morte, sensibilité, sens, réétalonnage) : %s\n", echecs ? "ÉCHEC" : "OK");
    if (echecs) { std::printf("%d échec(s)\n", echecs); return echecs; }
    std::printf("test_commandes : tout est OK\n");
    return 0;
}
#ifdef TEST_COMMANDES_SEUL
int main() { return testCommandes() ? 1 : 0; }
#endif
