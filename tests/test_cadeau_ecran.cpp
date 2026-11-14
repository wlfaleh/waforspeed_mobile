// WaForSpeed — tests de l'écran du cadeau du jour (WFS-501) : compte à rebours jusqu'à minuit local, heure reculée,
// format hh:mm, tableau « Chances » lu dans la configuration (jamais écrit dans le code), compteur de la garantie.
#include <cstdio>
#include <string>
#include "../src/ui/ecrans/cadeau_jour.h"

using namespace wfs;
using namespace wfs::ecrans::cadeau_jour;

static int g_echecs = 0;
#define VERIFIE(c) do { if (!(c)) { std::printf("ÉCHEC %s:%d : %s\n", __FILE__, __LINE__, #c); g_echecs++; } } while (0)

int main() {
    // 2026-11-13 19:47:00 UTC, Paris (UTC+1) : 20:47 locale ; prochain minuit local dans 3 h 13
    const cadeau::Instant t{1794599220LL, 3600};
    EtatCadeau c;
    VERIFIE(secondesAvantProchain(c, t) == -1);   // jamais ouvert : disponible, pas de compte à rebours
    c.heure = t.epoch - 600; c.decalage = 3600; c.jour = cadeau::texteJour(cadeau::jourLocal(t));
    VERIFIE(secondesAvantProchain(c, t) == 3 * 3600 + 13 * 60);
    VERIFIE(texteHeures(secondesAvantProchain(c, t)) == "03:13");
    // le jour change à minuit LOCAL : à New York (UTC−5), 14:47 → 9 h 13 avant minuit
    const cadeau::Instant ny{t.epoch, -5 * 3600};
    EtatCadeau cn = c; cn.decalage = -5 * 3600; cn.jour = cadeau::texteJour(cadeau::jourLocal(ny));
    VERIFIE(texteHeures(secondesAvantProchain(cn, ny)) == "09:13");
    // heure reculée (WFS-500 RG-07) : pas de compte à rebours (« bientôt »)
    const cadeau::Instant recul{t.epoch - 3 * 86400, 3600};
    VERIFIE(secondesAvantProchain(c, recul) == -1);
    // format : arrondi au-dessus, jamais « 00:00 » avant minuit, borné à 24 h
    VERIFIE(texteHeures(1) == "00:01");
    VERIFIE(texteHeures(59) == "00:01");
    VERIFIE(texteHeures(61) == "00:02");
    VERIFIE(texteHeures(0) == "00:00");
    VERIFIE(texteHeures(86400) == "24:00");
    VERIFIE(texteHeures(999999) == "24:00");
    // tableau « Chances » : 7 lignes, valeurs de la configuration
    ConfigJeu::Cadeau cfg;
    std::vector<LigneChance> l = lignesChances(cfg);
    VERIFIE(l.size() == 7);
    VERIFIE(l[0].v1 == "10" && l[0].v2 == "15" && l[0].chance == 45);
    VERIFIE(l[3].v1 == "41" && l[3].v2 == "50" && l[3].chance == 3);
    VERIFIE(l[4].cle == "cadeau.chances.coloris" && l[4].chance == 8);
    VERIFIE(l[5].cle == "cadeau.chances.skin" && l[5].chance == 4);
    VERIFIE(l[6].cle == "cadeau.chances.voiture" && l[6].v1 == "3" && l[6].chance == 3);
    cfg.chanceSkin = 6.5; cfg.pretHeures = 2; cfg.tranche[1] = {12, 30, 20};   // valeur changée dans configgame.json : suivie
    l = lignesChances(cfg);
    VERIFIE(l[5].chance == 6.5 && l[6].v1 == "2" && l[1].v1 == "12" && l[1].v2 == "30" && l[1].chance == 20);
    double somme = 0;
    for (const LigneChance& x : lignesChances(ConfigJeu::Cadeau{})) somme += x.chance;
    VERIFIE(somme > 99.99 && somme < 100.01);
    // garantie : 30 − cadeaux sans voiture ni skin, au moins 1
    EtatCadeau e;
    VERIFIE(restantsGarantie(ConfigJeu::Cadeau{}, e) == 30);
    e.sansRare = 29; VERIFIE(restantsGarantie(ConfigJeu::Cadeau{}, e) == 1);
    e.sansRare = 40; VERIFIE(restantsGarantie(ConfigJeu::Cadeau{}, e) == 1);
    if (g_echecs) std::printf("test_cadeau_ecran : %d échec(s)\n", g_echecs);
    else std::printf("test_cadeau_ecran : OK\n");
    return g_echecs ? 1 : 0;
}
