// WFS-327 (ANO-176) : Course à 5, le joueur finissait toujours dernier : les ordinateurs prenaient les virages à la vitesse
// d'une table « arcade » (épingle de 20 m du Grand Prix à ≈ 127 km/h, ≈ 6 g). Course simulée sans GPU (comme test_course5),
// conduite du jeu (g_conduiteMonde faux), Golf, difficulté Normale, 2 km, 8 tirages par carte :
//  - RG-01 : un ordinateur ne dépasse jamais la vitesse de virage physiquement possible pour sa voiture
//    (course5::vitesseVirageOrdinateur : même règle que l'aide du joueur) ;
//  - RG-02 : aide activée, volant lâché : dans les 3 premiers au moins 4 fois sur 8 sur chaque carte (décision du PO) ;
//    sans aide (joueur simulé « Glisser », fruste) : au moins une victoire par carte ;
//  - RG-03 : la conduite du joueur ne change pas (rien dans player.* : vérifié par les tests de la conduite).
// usage : test_course5_ordinateurs
#include <cstdio>
#include "../src/game/course5.h"
#include "../src/game/duel.h"
#include "../src/game/trace_cartes.h"
#include "../src/game/virages_banc.h"
using namespace wfs;
using namespace wfs::banc_virages;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

struct R { int place = 5; double depassement = 0; };
static R simuler(Track& t, uint32_t graine, Aide aide, bool tenu) {
    Player pl; pl.reset(carById("golf")); pl.aide = aide;
    auto os = course5::tirer(graine, "golf", course5::NORMALE);
    const int place = std::min((int)os.size(), course5::tirerPlaceJoueur(graine));
    course5::grille(os, place);
    course5::placerSurGrille(pl, t, course5::placeGrille(place));
    const double finish = 2 * duel::KM, dt = 1.0 / 120;
    R r; bool fini = false; double ft = 0;
    for (int i = 0; i < 120 * 400; i++) {
        const double tc = (i + 1) * dt;
        DriveInput in; in.gas = true; in.steer = tenu ? piloter(pl, t, 0, false) : 0.f;
        if (!fini) pl.update(dt, in, t, true);
        course5::avancer(os, dt, finish, tc, pl.dist, pl.playerX, pl.speed, &t);
        for (const auto& o : os)   // RG-01 : vitesse de l'ordinateur ≤ vitesse permise (marge d'un pas de freinage)
            if (!o.g.done) r.depassement = std::max(r.depassement, o.g.v / course5::vitesseVirageOrdinateur(t, o.g.rd, carById(o.car.c_str())) - 1);
        if (!fini && pl.dist >= finish) { fini = true; ft = tc; }
        bool tous = fini; for (const auto& o : os) tous = tous && o.g.done;
        if (tous) break;
    }
    r.place = course5::position(course5::classement(os, "moi", pl.dist, ft, fini, finish));
    return r;
}

int main() {
    const bool mondeDefaut = g_conduiteMonde;
    g_conduiteMonde = false;
    for (const char* carte : {"ville", "campagne", "plage", "circuit"}) {
        static Track t; t.build(mapById(carte), trace::versionCarte(carte));
        int top3 = 0, victoires = 0; double pire = 0;
        for (uint32_t g = 1; g <= 8; g++) {
            const R a = simuler(t, g * 7919u, Aide::Activee, false), d = simuler(t, g * 7919u, Aide::Desactivee, true);
            top3 += a.place <= 3; victoires += d.place == 1;
            pire = std::max({pire, a.depassement, d.depassement});
        }
        std::printf("- %s : aide activée, volant lâché : dans les 3 premiers %d / 8 ; sans aide : %d victoire(s) / 8 ; ordinateurs au plus %+.1f %% de leur vitesse permise\n",
                    carte, top3, victoires, pire * 100);
        VERIFIE(top3 >= 4, "RG-02 : %s : aide activée, dans les 3 premiers %d fois sur 8 (4 au moins)", carte, top3);
        VERIFIE(victoires >= 1, "RG-02 : %s : sans aide, aucune victoire sur 8", carte);
        VERIFIE(pire <= 0.02, "RG-01 : %s : un ordinateur dépasse sa vitesse permise de %.1f %%", carte, pire * 100);
    }
    g_conduiteMonde = mondeDefaut;
    std::printf("adversaires de la Course à 5 (WFS-327) : %s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
