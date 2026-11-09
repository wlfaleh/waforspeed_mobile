// Onglets du panneau des réglages (WFS-291, WFS-318) : chaque ligne dans un seul onglet, ordre validé par le PO
// (2026-11-08 23:04:56 UTC), « Fantôme du record » en dernière ligne d'Affichage.
// usage : test_reglages_onglets
#include <cstdio>
#include "../src/ui/ecrans/reglages_onglets.h"
using namespace wfs::ecrans;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    for (int l = 0; l < N_LIGNES; l++) {   // chaque ligne dans un et un seul onglet
        int n = 0;
        for (int o = 0; o < N_ONGLETS; o++) n += dansOnglet(l, o);
        VERIFIE(n == 1, "ligne %d : dans %d onglet(s), 1 attendu", l, n);
    }
    VERIFIE(nLignesOnglet(O_AFFICHAGE) + nLignesOnglet(O_CONDUITE) + nLignesOnglet(O_SON) == N_LIGNES, "toutes les lignes, aucune en double");
    const int aff[] = {L_CAMERA, L_QUALITE, L_REGARD, L_MOUVEMENTS, L_BOUTONS, L_FANTOME};
    const int con[] = {L_DIRECTION, L_COMMANDES, L_GYRO, L_GYROZONE, L_AIDE, L_VIBRATIONS};
    const int son[] = {L_SON, L_MUSIQUE};
    VERIFIE(nLignesOnglet(O_AFFICHAGE) == 6 && nLignesOnglet(O_CONDUITE) == 6 && nLignesOnglet(O_SON) == 2, "6, 6 et 2 lignes");
    for (int i = 0; i < 6; i++) VERIFIE(LIGNES_ONGLET[O_AFFICHAGE][i] == aff[i], "Affichage, ligne %d dans l'ordre validé", i);
    for (int i = 0; i < 6; i++) VERIFIE(LIGNES_ONGLET[O_CONDUITE][i] == con[i], "Conduite, ligne %d dans l'ordre validé", i);
    for (int i = 0; i < 2; i++) VERIFIE(LIGNES_ONGLET[O_SON][i] == son[i], "Son, ligne %d dans l'ordre validé", i);
    VERIFIE(ongletDe(L_FANTOME) == O_AFFICHAGE && LIGNES_ONGLET[O_AFFICHAGE][5] == L_FANTOME, "Fantôme du record : dernière ligne d'Affichage (WFS-318)");
    VERIFIE(ongletDe(L_DIRECTION) == O_CONDUITE && ongletDe(L_MUSIQUE) == O_SON, "ongletDe");
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("onglets des réglages : OK\n");
    return 0;
}
