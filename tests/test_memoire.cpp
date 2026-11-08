// WFS-297 (RG-06) : compteurs de mémoire et de chargement (src/core/memoire.h).
//  - mémoire du processus : lue (Linux, Android), positive, et elle grossit quand on touche 64 Mo ;
//  - « Lus pour cette partie » : seulement ce qui est lu depuis le début du chargement, pas le cumul depuis le lancement ;
//  - numéro de la partie : chaque course qui démarre (Rouler, Rejouer, manche) compte une partie ;
//  - mémoire de la carte graphique : Mo = octets comptés / 2^20.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../src/core/memoire.h"
using namespace wfs;

static int echecs = 0, cas = 0;
#define VERIFIE(c, ...) do { cas++; if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    memoire::Compteurs& c = memoire::compteurs();
    // mémoire du processus
    const double avant = memoire::processusMo();
    VERIFIE(avant > 1 && avant < 100000, "mémoire du processus lue : %.1f Mo", avant);
    std::vector<char> gros(64u << 20);
    std::memset(gros.data(), 1, gros.size());   // pages réellement touchées
    const double apres = memoire::processusMo();
    VERIFIE(apres - avant > 50 && apres - avant < 80, "64 Mo touchés : %.1f → %.1f Mo", avant, apres);
    // Mo lus : par chargement
    c.octetsLus = 300ll << 20;   // 300 Mo déjà lus par les parties précédentes
    memoire::debutChargement();
    c.octetsLus += 136ll << 20;  // ce chargement lit 136 Mo
    VERIFIE(std::fabs(memoire::lusChargementMo() - 136) < 1e-9, "lus pour cette partie : %.1f Mo (136 attendus)", memoire::lusChargementMo());
    VERIFIE(std::fabs(memoire::lusMo() - 436) < 1e-9, "cumul : %.1f Mo", memoire::lusMo());
    // numéro de la partie
    c.partie = 0;
    VERIFIE(memoire::nouvellePartie() == 1 && memoire::nouvellePartie() == 2 && c.partie.load() == 2, "1re puis 2e partie (Rejouer compte)");
    // carte graphique
    c.octetsGpu = 0; c.octetsGpu += 600ll << 20; c.octetsGpu -= 100ll << 20;
    VERIFIE(std::fabs(memoire::gpuMo() - 500) < 1e-9, "carte graphique : %.1f Mo (500 attendus)", memoire::gpuMo());
    std::printf("test_memoire : %d vérifications, %s (processus %.0f → %.0f Mo)\n", cas, echecs ? "ÉCHEC" : "TOUT EST VERT", avant, apres);
    return echecs ? 1 : 0;
}
