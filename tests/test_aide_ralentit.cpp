// WFS-258 (RG-00b, point 1) : aide à la direction activée, la voiture du joueur ralentit à temps pour passer les virages
// (vidéo de l'utilisateur : Grand Prix, aide activée, 228 km/h, sortie tout droit au 1er virage). Conduite du jeu (ancienne,
// g_conduiteMonde faux) :
//  - 1er virage du Grand Prix (épingle de 20 m) lancé à 228 km/h, Gallardo et Golf, aide activée, volant lâché, gaz tenu ou
//    lâché : aucune image hors de la route ; volant tenu (« Glisser ») : la limite cède au joueur (décision du PO, option A) ;
//  - aide désactivée : l'aide ne fait rien (ni gaz coupé, ni frein : RG-07 de WFS-241) ;
//  - un tour de chaque carte, gaz tenu, aide activée, volant lâché puis tenu (« Glisser ») : aucune image hors de la route
//    (au-dessus de 100 km/h, la limite ne cède qu'au frein : WFS-258 point 3), et le frein de l'aide ne clignote pas (10 fois
//    au plus par tour : écarts 80 / 100 / 110 km/h et 100 / 103 % de la vitesse permise) ;
//  - l'interrupteur du banc (g_avantWfs258) rend le comportement de main (pas de ralentissement) ;
//  - la table des ordinateurs de la Course à 5 est bien branchée (course5.cpp lié, comme dans le jeu) ;
//  - départ de la grille (revue du lead dev, PR #263) : voiture posée à x = 0,5, aide activée, volant lâché : à l'arrêt elle ne
//    tourne pas, au départ elle reste dans sa file (le maintien de voie reprend la file où la voiture est posée), aussi après
//    une course finie au bord gauche puis une remise sur la grille.
// usage : test_aide_ralentit
#include <cstdio>
#include "../src/game/trace_cartes.h"
#include "../src/game/virages_banc.h"
using namespace wfs;
using namespace wfs::banc_virages;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

struct Bilan { int herbe = 0, freinAide = 0; double vMin = 1e9; bool fini = false; };
// cas : 0 volant lâché gaz tenu, 1 volant lâché gaz lâché 100 m avant, 2 « Glisser » vers l'axe gaz tenu
static Bilan premierVirage(Track& t, double d0, double d1, const char* id, Aide a, int cas) {
    Player p = voiture(id, a, t, d0 - 400 / M, 0, 228);
    Bilan b;
    for (int k = 0; k < 120 * 30 && p.dist < d1 + 100 / M; k++) {
        DriveInput in; in.gas = cas != 1 || (d0 - p.dist) * M > 100;
        in.steer = cas == 2 ? piloter(p, t, 0, false) : 0.f;
        p.update(DT, in, t, true);
        b.freinAide += p.freinAide;
        if (p.dist > d0 - 20 / M) { b.herbe += std::abs(p.playerX) > 1; b.vMin = std::min(b.vMin, p.kmh()); }
    }
    b.fini = p.dist >= d1 + 100 / M;
    return b;
}

int main() {
    const bool mondeDefaut = g_conduiteMonde;
    g_conduiteMonde = false;   // conduite du jeu depuis WFS-258 RG-00
    static Track gp; gp.build(mapById("circuit"), trace::versionCarte("circuit"));
    double d0 = 0, d1 = 0; bool dans = false;   // 1er virage : rayon < 150 m après 200 m
    for (double s = 200 / M; s < gp.length(); s += 1 / M) {
        const double R = 1 / std::max(1e-9, std::abs(courbure(gp, s)));
        if (R < 150 && !dans) { dans = true; d0 = s; }
        if (dans && R > 300) { d1 = s; break; }
    }
    VERIFIE(g_vitesseVirageCourse5 != nullptr, "table des virages de la Course à 5 non branchée");
    std::printf("## 1er virage du Grand Prix (%.0f à %.0f m), lancé à 228 km/h\n", d0 * M, d1 * M);
    const char* noms[3] = {"volant lâché, gaz tenu", "volant lâché, gaz lâché 100 m avant", "Glisser vers l'axe, gaz tenu"};
    for (const char* id : {"gallardo", "golf"}) for (int cas = 0; cas < 3; cas++) {
        const Bilan b = premierVirage(gp, d0, d1, id, Aide::Activee, cas);
        std::printf("- %s, aide activée, %s : %d image(s) hors route, %.0f km/h au plus bas, aide qui freine %.1f s\n", id, noms[cas], b.herbe, b.vMin, b.freinAide / 120.0);
        if (cas != 2) VERIFIE(b.herbe == 0, "%s, aide activée, %s : %d image(s) hors route", id, noms[cas], b.herbe);
        if (cas != 1) VERIFIE(b.fini, "%s, aide activée, %s : virage non fini", id, noms[cas]);
    }
    {   // aide désactivée : aucune commande automatique
        const Bilan b = premierVirage(gp, d0, d1, "golf", Aide::Desactivee, 0);
        std::printf("- golf, aide désactivée, volant lâché, gaz tenu : aide qui freine %.1f s, %d image(s) hors route (au joueur de freiner)\n", b.freinAide / 120.0, b.herbe);
        VERIFIE(b.freinAide == 0, "aide désactivée : l'aide a freiné (%d pas)", b.freinAide);
    }
    {   // interrupteur du banc : comportement de main
        g_avantWfs258 = true;
        const Bilan b = premierVirage(gp, d0, d1, "golf", Aide::Activee, 0);
        g_avantWfs258 = false;
        std::printf("- banc, comportement de main rejoué : %d image(s) hors route\n", b.herbe);
        VERIFIE(b.herbe > 0 && b.freinAide == 0, "g_avantWfs258 ne rejoue pas main (%d image(s) hors route, %d pas de frein)", b.herbe, b.freinAide);
    }
    {   // départ de la grille : décompte de 3 s (voiture tenue à l'arrêt) puis 3 s gaz tenu, volant lâché
        static Track v; v.build(mapById("ville"), trace::versionCarte("ville"));
        auto depart = [&](Player& p, const char* quand) {
            p.dist = 60 / M; p.position = v.wrap(p.dist); p.playerX = 0.5; p.heading = 0; p.speed = 0;   // comme course5::placerSurGrille
            double capMax = 0;
            for (int k = 0; k < 360; k++) { p.update(DT, DriveInput{}, v, false); p.speed = 0; capMax = std::max(capMax, std::abs(p.heading)); }
            const double x0 = p.playerX; double ecart = 0;
            for (int k = 0; k < 360; k++) { DriveInput in; in.gas = true; p.update(DT, in, v, true); capMax = std::max(capMax, std::abs(p.heading)); ecart = std::max(ecart, std::abs(p.playerX - x0)); }
            std::printf("- départ de la grille (x = 0,5) %s : cap max %.3f rad, x bouge de %.3f demi-largeur en 3 s\n", quand, capMax, ecart);
            VERIFIE(capMax <= 0.02 && ecart <= 0.05, "départ de la grille %s : cap max %.3f rad, x bouge de %.3f", quand, capMax, ecart);
        };
        Player p = voiture("golf", Aide::Activee, v, 60 / M, 0.5 * RW, 0);
        depart(p, "après le reset");
        for (int k = 0; k < 120 * 20; k++) { DriveInput in; in.gas = true; in.steer = p.playerX > -0.6 ? -0.3f : 0.f; p.update(DT, in, v, true); }   // course finie au bord gauche
        for (int k = 0; k < 120 * 5; k++) { DriveInput in; in.brake = p.kmh() > 1; p.update(DT, in, v, true); }
        depart(p, "après une course finie au bord gauche");
    }
    std::printf("## Un tour de chaque carte, aide activée, gaz tenu\n");
    for (const char* carte : {"circuit", "campagne", "ville", "plage"}) {
        static Track t; t.build(mapById(carte), trace::versionCarte(carte));
        for (const char* id : {"golf", "gallardo"}) for (bool glisser : {false, true}) {
            Player p = voiture(id, Aide::Activee, t, 0, 0, 0);
            int herbe = 0, n = 0, clignote = 0, dernier = -1000; bool f0 = false;
            for (; n < 120 * 900 && p.dist < t.length(); n++) {
                DriveInput in; in.gas = true; in.steer = glisser ? piloter(p, t, 0, false) : 0.f;
                p.update(DT, in, t, true);
                herbe += std::abs(p.playerX) > 1;
                if (p.freinAide != f0) { if (n - dernier < 60) clignote++; dernier = n; }   // frein de l'aide qui change en moins de 0,5 s
                f0 = p.freinAide;
            }
            // remarque du lead dev (PR #271) : pas de va-et-vient du frein de l'aide (main : 221 à 1 317 par tour, doigt tenu)
            std::printf("  clignotements du frein de l'aide (moins de 0,5 s) : %d\n", clignote);
            VERIFIE(clignote <= 10, "%s, %s, %s : le frein de l'aide clignote %d fois", carte, id, glisser ? "Glisser" : "volant lâché", clignote);
            std::printf("- %s, %s, %s : tour %s en %.1f s, %d image(s) hors route\n", carte, id, glisser ? "Glisser" : "volant lâché", p.dist >= t.length() ? "fini" : "NON fini", n * DT, herbe);
            // volant tenu aussi : 0 sortie (revue du lead dev, PR #271 : la direction réduite au-dessus de 100 km/h ne doit pas faire
            // céder la limite de l'aide ; main : 0 à 35 images selon la carte)
            VERIFIE(p.dist >= t.length() && herbe == 0, "%s, %s, %s : %d image(s) hors route", carte, id, glisser ? "Glisser" : "volant lâché", herbe);
        }
    }
    g_conduiteMonde = mondeDefaut;
    std::printf("l'aide ralentit à temps (WFS-258) : %s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
