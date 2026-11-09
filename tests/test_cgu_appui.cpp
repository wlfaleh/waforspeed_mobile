// Appuis de l'écran des CGU avant le jeu (WFS-355, ANO-199) : destination des appuis (direct et rejoués par le veilleur),
// cible de 48 dp d'« Accepter », appui dont la pose ou le lever s'est perdu.
// usage : test_cgu_appui
#include <cstdio>
#include "../src/ui/ecrans/cgu_appui.h"
using namespace wfs::ecrans::cgu_appui;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    // destination (onInput et rejouerEntrees d'android_main.cpp)
    VERIFIE(destination(false, false, true, false) == Destination::Cgu, "CGU du premier lancement : appui aux CGU (cause n° 1 : perdu avant WFS-355)");
    VERIFIE(destination(false, true, false, false) == Destination::Paquets, "étape des paquets : appui aux paquets");
    VERIFIE(destination(true, false, false, false) == Destination::Erreur, "écran d'erreur du chargement");
    VERIFIE(destination(false, false, false, true) == Destination::Jeu, "jeu chargé : appui au jeu");
    VERIFIE(destination(false, false, false, false) == Destination::Rien, "rien d'ouvert, jeu pas prêt : appui ignoré");
    VERIFIE(destination(false, false, true, true) == Destination::Jeu, "jeu prêt : les CGU des Réglages passent par le jeu");

    // barre du bas des CGU sur téléphone 2340 × 1080 (2,625 px/dp) : mêmes mesures que Cgu::dessiner (bh = 50 × 1080 / 720)
    const float dp = 2.625f, bh = 75, by = 960;
    const Rect caseZ{60, by - 20, 420, 115}, acc{2030, by, 230, bh}, ref{1760, by, 230, bh}, fer{}, texte{50, 160, 2240, 770};
    const float ax = acc.x + acc.w / 2, cy = by + bh / 2, demi = 24 * dp;
    VERIFIE(cibleSous(caseZ, acc, ref, fer, texte, dp, ax, cy) == ACCEPTER, "centre d'« Accepter »");
    VERIFIE(cibleSous(caseZ, acc, ref, fer, texte, dp, ax, cy + demi - 1) == ACCEPTER, "bas de la cible de 48 dp (doigt un peu sous le bouton)");
    VERIFIE(cibleSous(caseZ, acc, ref, fer, texte, dp, ax, cy - demi + 1) == ACCEPTER, "haut de la cible de 48 dp (avant le texte)");
    VERIFIE(cibleSous(caseZ, acc, ref, fer, texte, dp, acc.x + acc.w + 4 * dp - 1, cy) == ACCEPTER, "bord droit + 4 dp");
    VERIFIE(cibleSous(caseZ, acc, ref, fer, texte, dp, ax, cy + demi + 2) == RIEN, "au-delà de 48 dp : rien");
    VERIFIE(cibleSous(caseZ, acc, ref, fer, texte, dp, ref.x + 10, cy) == REFUSER, "« Refuser » inchangé à sa place (RG-03)");
    VERIFIE(cibleSous(caseZ, acc, ref, fer, texte, dp, 100, cy) == CASE, "case");
    VERIFIE(cibleSous(caseZ, acc, ref, fer, texte, dp, 1000, 500) == TEXTE, "texte");
    VERIFIE(acc.x - 4 * dp > ref.x + ref.w + 4 * dp, "cibles d'« Accepter » et de « Refuser » séparées");

    // appui terminé
    VERIFIE(cibleAuLever(true, ACCEPTER, ACCEPTER) == ACCEPTER, "posé et levé sur « Accepter »");
    VERIFIE(cibleAuLever(false, RIEN, ACCEPTER) == ACCEPTER, "pose jamais reçue, levé sur « Accepter » : compte");
    VERIFIE(cibleAuLever(true, ACCEPTER, RIEN) == RIEN, "doigt glissé hors du bouton : annulé");
    VERIFIE(cibleAuLever(true, TEXTE, ACCEPTER) == RIEN, "défilement du texte fini sur « Accepter » : rien");
    VERIFIE(cibleAuLever(false, RIEN, TEXTE) == RIEN, "lever seul sur le texte : rien");
    VERIFIE(cibleAuLever(true, CASE, CASE) == CASE, "case");
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("appuis des CGU : OK\n");
    return 0;
}
