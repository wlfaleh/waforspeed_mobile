// Mode testeur (WFS-546) : bouton « +2500 ★ » du menu caché de diagnostic des Réglages.
// RG-02 : un appui = 2500 ★, un doigt posé qui bouge ne recrédite pas ; RG-06 : 3 appuis = 7500 ★, sauvegarde relue = 7500 ★.
// usage : test_reglages_etoiles
#include <cstdio>
#include <cstdlib>
#include "../src/ui/ecrans/reglages_etoiles.h"
using namespace wfs;
using namespace wfs::ecrans::testeur;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// un appui complet comme sur Android : posé, 3 mouvements (down = vrai, même doigt), levé
static int appui(BoutonEtoiles& b, Sauvegarde& s, int doigt) {
    int credits = b.poser(doigt, true, s) ? 1 : 0;
    for (int i = 0; i < 3; i++) credits += b.poser(doigt, true, s) ? 1 : 0;
    b.lever(doigt);
    return credits;
}

int main() {
    Sauvegarde s; s.defauts();
    VERIFIE(boutique::solde(s) == 0, "solde de départ 0 ★ (lu : %lld)", boutique::solde(s));
    BoutonEtoiles b;
    // RG-06 : 3 appuis → 7500 ★, un seul crédit par appui malgré les mouvements du doigt
    int credits = 0;
    for (int i = 0; i < 3; i++) credits += appui(b, s, 0);
    VERIFIE(credits == 3, "3 appuis = 3 crédits (lu : %d)", credits);
    VERIFIE(boutique::solde(s) == 7500, "3 appuis = 7500 ★ (lu : %lld)", boutique::solde(s));
    // RG-03 / RG-06 : sauvegarde relue
    Sauvegarde r; const bool lu = r.lire(s.texte());
    VERIFIE(lu && boutique::solde(r) == 7500, "sauvegarde relue : 7500 ★ (lu : %lld)", boutique::solde(r));
    // un doigt posé à côté du bouton, qui glisse dessus : il n'a pas appuyé sur le bouton au départ, il crédite en arrivant
    // dessus une seule fois (même règle que les autres boutons du menu, qui réagissent au toucher posé)
    Sauvegarde t; t.defauts();
    BoutonEtoiles c;
    VERIFIE(!c.poser(4, false, t) && boutique::solde(t) == 0, "doigt hors du bouton : rien");
    VERIFIE(c.poser(4, true, t) && !c.poser(4, true, t) && boutique::solde(t) == 2500, "arrivé sur le bouton : 2500 ★, une seule fois");
    // 2e doigt pendant que le 1er est posé : appui distinct
    VERIFIE(c.poser(5, true, t) && boutique::solde(t) == 5000, "2e doigt : 2e appui (5000 ★)");
    // lever perdu (menu fermé) : oublier libère, l'appui suivant du même identifiant compte
    c.oublier();
    VERIFIE(c.poser(5, true, t) && boutique::solde(t) == 7500, "après oublier() : appui compté");
    // lever d'un autre doigt : ne libère pas le doigt retenu
    c.lever(9);
    VERIFIE(!c.poser(5, true, t) && boutique::solde(t) == 7500, "lever d'un autre doigt : le doigt retenu ne recrédite pas");
    // pas de limite au nombre d'appuis (RG-02), solde borné par crediter (SOLDE_MAX)
    Sauvegarde u; u.defauts(); BoutonEtoiles d;
    for (int i = 0; i < 40; i++) appui(d, u, i % 3);
    VERIFIE(boutique::solde(u) == 40 * CREDIT_ETOILES, "40 appuis = 100 000 ★ (lu : %lld)", boutique::solde(u));
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("mode testeur (+2500 ★) : OK\n");
    return 0;
}
