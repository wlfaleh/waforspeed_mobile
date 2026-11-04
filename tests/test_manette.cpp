// Vérifie la correspondance de la manette (WFS-22 / WFS-23) : zone morte 0,12, gâchettes, croix et stick → DriveInput,
// navigation des menus (front de la croix, réarmement du stick, bouton voisin).
#include <cmath>
#include <cstdio>
#include "../src/game/player.h"
#include "../src/platform/manette.h"
using namespace wfs;
using namespace wfs::manette;
static int echecs = 0;
static void verifie(bool ok, const char* quoi) { std::printf("%s %s\n", ok ? "ok    " : "ÉCHEC ", quoi); if (!ok) echecs++; }
static bool proche(float a, float b) { return std::fabs(a - b) < 1e-4f; }
int main() {
    // zone morte
    verifie(zoneMorte(0.11f) == 0 && zoneMorte(-0.119f) == 0, "zone morte : |x| < 0,12 → 0");
    verifie(proche(zoneMorte(0.12f), 0) && proche(zoneMorte(1), 1) && proche(zoneMorte(-1), -1), "zone morte : 0,12 → 0, ±1 → ±1 (sans saut)");
    verifie(proche(zoneMorte(0.56f), 0.5f), "zone morte : 0,56 → 0,5 (remise à l'échelle)");
    DriveInput in;
    Etat e;
    conduite(e, in);
    verifie(in.steer == 0 && !in.gas && !in.brake && !in.nitro, "manette au repos : aucune commande");
    e.stickX = 0.1f; e.gachetteD = 0.1f; e.gachetteG = 0.05f; conduite(e, in);
    verifie(in.steer == 0 && !in.gas && !in.brake, "stick et gâchettes dans la zone morte : aucune commande");
    e = Etat{}; e.gachetteD = 0.4f; e.stickX = -0.56f; conduite(e, in);
    verifie(in.gas && !in.brake && proche(in.steer, -0.5f), "gâchette droite = accélérer, stick à −0,56 → braquage −0,5");
    e = Etat{}; e.gachetteG = 1; conduite(e, in);
    verifie(in.brake && !in.gas, "gâchette gauche = freiner / marche arrière");
    e = Etat{}; e.bouton[R2] = true; e.bouton[L2] = true; conduite(e, in);
    verifie(in.gas && in.brake, "gâchettes numériques (R2, L2)");
    e = Etat{}; e.bouton[A] = true; conduite(e, in);
    verifie(in.nitro && in.gas, "A = nitro (avec les gaz, comme le bouton tactile)");
    e = Etat{}; e.croixX = 1; e.stickX = -0.8f; conduite(e, in);
    verifie(in.steer == 1, "croix à droite = braquage plein (prioritaire sur le stick)");
    e = Etat{}; e.bouton[GAUCHE] = true; conduite(e, in);
    verifie(in.steer == -1, "touche gauche de la croix = braquage plein à gauche");
    // menus : croix (un pas par appui) et stick (un pas par poussée)
    Etat p; bool arme = true; int dx, dy;
    e = Etat{}; e.croixY = 1; directionMenu(e, p, arme, dx, dy);
    verifie(dx == 0 && dy == 1, "croix en bas : un pas vers le bas");
    p = e; directionMenu(e, p, arme, dx, dy);
    verifie(dx == 0 && dy == 0, "croix tenue : pas de deuxième pas");
    p = Etat{}; e = Etat{}; e.stickX = 0.7f; directionMenu(e, p, arme, dx, dy);
    verifie(dx == 1 && dy == 0, "stick à droite (0,7) : un pas à droite");
    directionMenu(e, p, arme, dx, dy);
    verifie(dx == 0, "stick tenu : pas de deuxième pas");
    e.stickX = 0.2f; directionMenu(e, p, arme, dx, dy); e.stickX = 0.7f; directionMenu(e, p, arme, dx, dy);
    verifie(dx == 1, "stick revenu au centre puis poussé : nouveau pas");
    // bouton voisin : 3 choix en colonne, 2 boutons sur une ligne
    std::vector<Zone> z = {{100, 100, 400, 70, false}, {100, 176, 400, 70, false}, {100, 252, 400, 70, false},
                           {600, 400, 200, 60, false}, {820, 400, 200, 60, true}};
    verifie(voisin(z, 0, 0, 1) == 1 && voisin(z, 1, 0, 1) == 2 && voisin(z, 1, 0, -1) == 0, "menu : haut / bas entre les choix");
    verifie(voisin(z, 0, 0, -1) == 0, "menu : rien au-dessus du premier choix (focus inchangé)");
    verifie(voisin(z, 3, 1, 0) == 4 && voisin(z, 4, -1, 0) == 3, "boutons sur une ligne : gauche / droite");
    verifie(voisin(z, 2, 0, 1) == 3, "bas depuis le dernier choix : bouton le plus proche en dessous");
    // focus des écrans : zones de l'image, échangées en fin d'image
    zoneFocus(1, 2, 3, 4); focus(Zone{0, 0, 0, 0, false}); zoneFocus(5, 6, 7, 8, true);
    verifie(zonesImage().size() == 2, "zones vides ignorées");
    finImage();
    verifie(zonesImage().empty() && zonesPrecedentes().size() == 2 && zonesPrecedentes()[1].retour, "fin d'image : zones passées à l'image suivante");
    // mode manette (RG-03) et déconnexion (RG-05)
    verifie(!actif(), "au départ : mode tactile");
    axes(0.05f, 0, 0, 0, 0, 0);
    verifie(!actif(), "stick dans la zone morte : reste en mode tactile");
    axes(0, 0, 0, 0.8f, 0, 0);
    verifie(actif(), "gâchette pressée : mode manette");
    toucherEcran();
    verifie(!actif(), "toucher de l'écran : retour au tactile");
    bouton(START, true); bouton(START, false);
    verifie(actif() && !deconnexionRecue(), "bouton : mode manette, pas de déconnexion");
    deconnexion();
    verifie(!actif() && deconnexionRecue() && !deconnexionRecue(), "déconnexion : tactile, signalée une seule fois");
    std::printf("manette : %d échec(s)\n", echecs);
    return echecs ? 1 : 0;
}
