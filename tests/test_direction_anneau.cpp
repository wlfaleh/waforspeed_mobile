// WaForSpeed — WFS-316 (ANO-172) : volant tenu, la voiture suit le virage serré et l'anneau à 25-50 km/h (ancienne conduite,
// voiture du joueur), à 20 images/s irrégulières (pas de 50 à 75 ms, consigne du doigt lue une fois par image) et à 60 :
//  - RG-01 : « avant » (g_avantWfs316) reproduit la sortie à l'extérieur ; part de chaque suspect (force centrifuge forfaitaire,
//    cap borné, images lentes) ;
//  - RG-03 : volant tenu (« Glisser » à 6 et 12 % de la largeur, ou flèche), le rayon de la trajectoire dans le monde est le même
//    dans l'anneau (R = 14 m) qu'en ligne droite (± 10 %) à 25 et 50 km/h : la route ne pousse plus la voiture ;
//  - RG-04 : un joueur qui suit sa voie (Glisser ou flèches, aide désactivée puis activée) passe l'anneau de la Ville (50 → 25 km/h),
//    l'épingle des lacets de la Corniche (30 km/h) et le virage à 90° de la Ville (40 km/h) sans sortir de la chaussée ;
//  - RG-05 : volant lâché, rien ne change (la force forfaitaire et la borne restent).
// usage : test_direction_anneau [--rapport]
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../src/game/cars.h"
#include "../src/game/direction.h"
#include "../src/game/player.h"
#include "../src/game/trace_cartes.h"
#include "../src/game/track.h"
#include "../src/game/virages_banc.h"
using namespace wfs;
using namespace wfs::banc_virages;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// images : 20 img/s irrégulières (50 à 75 ms, tirage fixe) ou 60 img/s ; la simulation avance par pas de 1/120 s (game.cpp)
struct Images {
    bool lentes; uint32_t graine = 12345; double reste = 0;
    double suivante() {
        if (!lentes) return 1.0 / 60;
        graine = graine * 1664525u + 1013904223u;
        return 0.050 + 0.025 * ((graine >> 8) / 16777216.0);
    }
};

// « Glisser » : doigt posé puis déplacé de `part` de la largeur (6 % : demi-volant, 12 % : butée), écran de 2400 px
static float doigt(double part, int sens) { const float W = 2400; return braquageGlisser(1000, 1000 + sens * (float)part * W, W); }

// rayon de la trajectoire dans le monde, volant tenu `steer`, vitesse tenue, de 0,4 à 1,0 s (braquage établi à 0,25 s :
// volant progressif de 4 /s ; fenêtre courte : volant à fond à 50 km/h, le cercle de 9 m ne tient pas dans la largeur de la route)
// n : position latérale de départ (m) ; touche : la voiture a touché un bord (mesure sans valeur)
static double rayonTenu(Track& t, double s, double n, double kmh, float steer, bool lentes, bool* touche = nullptr) {
    Player p = voiture("golf", Aide::Desactivee, t, s, n, kmh);
    if (touche) *touche = false;
    Images im{lentes};
    double temps = 0, arc = 0, cap0 = 0, cap1 = 0; bool debut = false;
    PoseMonde a = pose(p, t);
    while (temps < 1.0) {
        const DriveInput in = tenir(p, kmh, steer);   // consigne lue une fois par image
        for (double fin = temps + im.suivante(); temps + 1e-9 < fin; temps += DT) {
            p.update(DT, in, t, true);
            if (touche && std::abs(p.playerX) > 1) *touche = true;
            const PoseMonde b = pose(p, t);
            if (temps >= 0.4) { if (!debut) { debut = true; cap0 = b.cap; } else arc += std::hypot(b.x - a.x, b.z - a.z); cap1 = b.cap; }
            a = b;
        }
    }
    return arc / std::max(1e-6, std::abs(std::remainder(cap1 - cap0, 2 * 3.14159265358979)));
}

// joueur qui suit sa voie (file 0) : vitesse visée de v0 à v1 sur la scène, consigne lue une fois par image
struct Bilan { int sorties = 0; double ecartMax = 0; bool fini = false; };
static Bilan passer(Track& t, double debut, double fin, double v0, double v1, Aide aide, bool fleches, bool lentes) {
    Player p = voiture("golf", aide, t, debut - 40 / M, 0, v0);
    Images im{lentes};
    Bilan b; double temps = 0;
    while (temps < 40 && p.dist < fin + 30 / M) {
        const double f = std::clamp((p.dist - debut) / std::max(1.0, fin - debut), 0.0, 1.0), vise = v0 + (v1 - v0) * f;
        DriveInput in = tenir(p, vise, piloter(p, t, 0, fleches));
        if (p.kmh() > vise + 2) { in.gas = false; in.brake = true; }
        for (double fin2 = temps + im.suivante(); temps + 1e-9 < fin2; temps += DT) {
            p.update(DT, in, t, true);
            if (p.dist >= debut && p.dist <= fin) { b.ecartMax = std::max(b.ecartMax, std::abs(p.playerX)); b.sorties += std::abs(p.playerX) > 1; }
        }
    }
    b.fini = p.dist >= fin + 30 / M;
    return b;
}

int main(int argc, char** argv) {
    const bool rapport = argc > 1 && !std::strcmp(argv[1], "--rapport");
    static Track ville, plage;
    ville.build(mapById("ville"), trace::versionCarte("ville"));
    plage.build(mapById("plage"), trace::versionCarte("plage"));
    const trace::Section *rp = nullptr, *v90 = nullptr;
    for (const trace::Section& s : trace::sections(ville)) {
        if (s.type == trace::Type::RondPoint && !rp) rp = &s;
        if (s.type == trace::Type::Virage && std::abs(s.angle) >= 80 && std::abs(s.angle) <= 100 && s.rayon < 40 && !v90) v90 = &s;
    }
    VERIFIE(rp, "rond-point de la Ville introuvable");
    if (!rp) { std::printf("test_direction_anneau : %d échec(s)\n", echecs); return 1; }
    // milieu de l'anneau : courbure la plus forte
    double sMil = (rp->debut + rp->fin) * 0.5 * SEG, kMax = 0;
    for (double s = rp->debut * SEG; s < rp->fin * SEG; s += 0.5 / M) if (std::abs(courbure(ville, s)) > kMax) { kMax = std::abs(courbure(ville, s)); sMil = s; }
    const int sens = courbure(ville, sMil) > 0 ? 1 : -1;
    const double sDebutAnneau = sMil - 4 / M;   // un peu avant le cœur de l'anneau : la voiture y est quand le braquage est établi
    std::printf("## Rond-point de la Ville : anneau de %.1f m de rayon au %.0f m, virage %s\n", 1 / kMax, sMil * M, sens > 0 ? "à droite" : "à gauche");

    // ---------- RG-01, RG-03 : rayon volant tenu, anneau contre ligne droite ----------
    std::printf("\n## RG-01 / RG-03 : rayon de la trajectoire dans le monde, volant tenu (Golf, aide désactivée)\n");
    std::printf("| Volant | km/h | images/s | ligne droite (m) | anneau, avant (m) | anneau, après (m) | écart après |\n|---|---|---|---|---|---|---|\n");
    for (double part : {0.06, 0.12}) for (double kmh : {25.0, 50.0}) for (bool lentes : {true, false}) {
        const float steer = doigt(part, sens);
        // anneau : départ sur la file extérieure (6 m de l'axe), la voiture a la place de décrire son cercle sans toucher l'îlot
        bool toucheDroit = false;   // ligne droite : même départ décalé du côté opposé au virage (la voiture reste sur la chaussée)
        const double droit = rayonTenu(ville, 50 / M, -sens * 6.0, kmh, steer, lentes, &toucheDroit);
        VERIFIE(!toucheDroit, "volant %.0f %%, %.0f km/h : la voiture touche un bord en ligne droite (mesure sans valeur)", part * 100, kmh);
        bool toucheAvant = false, touche = false;
        g_avantWfs316 = true;  const double avant = rayonTenu(ville, sDebutAnneau, -sens * 6.0, kmh, steer, lentes, &toucheAvant);
        g_avantWfs316 = false; const double apres = rayonTenu(ville, sDebutAnneau, -sens * 6.0, kmh, steer, lentes, &touche);
        VERIFIE(!touche, "volant %.0f %%, %.0f km/h : la voiture touche un bord de l'anneau (mesure sans valeur)", part * 100, kmh);
        (void)toucheAvant; (void)avant;   // « avant » : la sortie se mesure à l'écart latéral (plus bas), pas au rayon
        const double ecart = apres / droit - 1;
        std::printf("| Glisser %.0f %% | %.0f | %s | %.1f | %.1f | %.1f | %+.0f %% |\n", part * 100, kmh, lentes ? "20" : "60", droit, avant, apres, 100 * ecart);
        VERIFIE(std::abs(ecart) <= 0.10, "volant %.0f %%, %.0f km/h, %s img/s : rayon dans l'anneau %.1f m contre %.1f m en ligne droite (± 10 %%)",
                part * 100, kmh, lentes ? "20" : "60", apres, droit);
    }
    // part de chaque suspect à 50 km/h, 20 img/s, demi-volant : force forfaitaire seule, borne seule (aide activée)
    {
        const float steer = doigt(0.06, sens);
        auto ecartLat = [&](Aide aide, bool avant) {   // écart latéral (m) vers l'extérieur après 1,2 s dans l'anneau
            g_avantWfs316 = avant;
            Player p = voiture("golf", aide, ville, sDebutAnneau, 0, 50);
            Images im{true}; double temps = 0;
            while (temps < 1.2) { const DriveInput in = tenir(p, 50, steer); for (double f = temps + im.suivante(); temps + 1e-9 < f; temps += DT) p.update(DT, in, ville, true); }
            g_avantWfs316 = false;
            return -sens * p.playerX * RW;
        };
        const double d0 = ecartLat(Aide::Desactivee, true), d1 = ecartLat(Aide::Desactivee, false);
        const double a0 = ecartLat(Aide::Activee, true), a1 = ecartLat(Aide::Activee, false);
        std::printf("\n## RG-01 : part de chaque suspect (50 km/h, demi-volant, 20 img/s, 1,2 s dans l'anneau ; écart vers l'extérieur)\n");
        std::printf("- aide désactivée : avant %+.2f m, après %+.2f m → force centrifuge forfaitaire : %.2f m (la borne n'agit pas sans aide)\n", d0, d1, d0 - d1);
        std::printf("- aide activée : avant %+.2f m, après %+.2f m → force forfaitaire + cap borné : %.2f m\n", a0, a1, a0 - a1);
        VERIFIE(d0 - d1 > 1.0, "la force forfaitaire devrait expliquer l'essentiel de la sortie (%.2f m)", d0 - d1);
    }

    // ---------- RG-04 : scènes, joueur qui suit sa voie ----------
    std::printf("\n## RG-04 : joueur qui suit sa voie, sorties de chaussée (|x| > 1) dans la scène\n");
    std::printf("| Scène | commande | aide | images/s | sorties | écart max (demi-largeurs) |\n|---|---|---|---|---|---|\n");
    struct Scene { const char* nom; Track* t; double debut, fin, v0, v1; };
    std::vector<Scene> scenes = {{"anneau de la Ville", &ville, rp->debut * SEG, rp->fin * SEG, 50, 25}};
    const std::vector<Epingle> ep = epingles(plage);
    if (!ep.empty()) scenes.push_back({"épingle de la Corniche", &plage, ep[0].debut, ep[0].fin, 30, 30});
    if (v90) scenes.push_back({"virage à 90° de la Ville", &ville, v90->debut * SEG, v90->fin * SEG, 40, 40});
    VERIFIE(scenes.size() == 3, "scènes introuvables (%zu sur 3)", scenes.size());
    for (const Scene& sc : scenes) for (bool fleches : {false, true}) for (Aide aide : {Aide::Desactivee, Aide::Activee}) for (bool lentes : {true, false}) {
        const Bilan b = passer(*sc.t, sc.debut, sc.fin, sc.v0, sc.v1, aide, fleches, lentes);
        std::printf("| %s | %s | %s | %s | %d | %.2f |\n", sc.nom, fleches ? "flèches" : "Glisser", aide == Aide::Activee ? "activée" : "désactivée", lentes ? "20" : "60", b.sorties, b.ecartMax);
        VERIFIE(b.fini && b.sorties == 0, "%s, %s, aide %s, %s img/s : %d pas hors de la chaussée (écart %.2f), %s", sc.nom, fleches ? "flèches" : "Glisser",
                aide == Aide::Activee ? "activée" : "désactivée", lentes ? "20" : "60", b.sorties, b.ecartMax, b.fini ? "scène finie" : "scène non finie");
    }

    // ---------- RG-05 : volant lâché, comportement d'avant gardé ----------
    {
        auto lache = [&](bool avant) { g_avantWfs316 = avant; Player p = voiture("golf", Aide::Desactivee, ville, sDebutAnneau, 0, 50);
            for (int k = 0; k < 120; k++) p.update(DT, tenir(p, 50, 0.f), ville, true); g_avantWfs316 = false; return p.playerX; };
        VERIFIE(lache(true) == lache(false), "volant lâché : la conduite a changé");
    }
    if (rapport) std::printf("\n(rapport : à recopier au journal)\n");
    if (echecs) { std::printf("test_direction_anneau : %d échec(s)\n", echecs); return 1; }
    std::printf("test_direction_anneau : OK\n");
    return 0;
}
