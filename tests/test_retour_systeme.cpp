// Bouton / geste « Retour » du téléphone (WFS-116) : décision de actionRetour pour chaque écran, puis le parcours complet
// (RG-06) joué avec un petit modèle des écrans (ce que fait le bouton « Retour » de chacun dans le jeu).
// usage : test_retour_systeme
#include <cstdio>
#include "../src/platform/retour_systeme.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

const char* wfs::nomActionRetour(ActionRetour a) {   // (le vrai est dans retour_systeme.cpp, qui dépend du jeu)
    const char* n[] = {"ignoré", "pause", "reprise", "panneau des modes fermé", "bouton « Retour » de l'écran", "menu", "arrière-plan"};
    return n[(int)a];
}

// modèle des écrans : l'écran affiché, la course finie ou non, les panneaux ouverts au menu
struct Modele {
    Ecran e = Ecran::Menu; bool finie = false, modes = false, reglages = false, dehors = false;
    bool zoneRetour() const {   // écrans qui déclarent un bouton « Retour » (manette::focus(..., true))
        switch (e) {
            case Ecran::Garage: case Ecran::Terrain: case Ecran::Pause: return true;
            case Ecran::Course: return finie;   // écran de fin de la course Checkpoint : « Menu » (course_system.cpp)
            case Ecran::Menu: return reglages;  // panneau des réglages : « Fermer »
            default: return false;              // salon, chargement
        }
    }
    void retour() {   // une pression : décision puis effet (ce que fait le jeu)
        const ActionRetour a = actionRetour(e, finie, zoneRetour(), modes);
        const Ecran avant = e;
        switch (a) {
            case ActionRetour::Ignorer: break;
            case ActionRetour::Pause: e = Ecran::Pause; break;
            case ActionRetour::Reprise: e = Ecran::Course; break;
            case ActionRetour::FermerModes: modes = false; break;
            case ActionRetour::BoutonRetour:
                if (e == Ecran::Menu) reglages = false;   // « Fermer » des réglages
                else if (e == Ecran::Pause) e = Ecran::Course;   // « Reprendre »
                else e = Ecran::Menu;   // garage, terrain, fin de course : « Retour » / « Menu »
                break;
            case ActionRetour::Menu: e = Ecran::Menu; break;
            case ActionRetour::ArrierePlan: dehors = true; break;
        }
        std::printf("retour (téléphone) : écran %s → %s (%s)\n", nomEcran(avant), dehors ? "arrière-plan" : nomEcran(e), nomActionRetour(a));
    }
};

int main() {
    // RG-02 / RG-03 / RG-04 : décision par écran
    VERIFIE(actionRetour(Ecran::Chargement, false, false, false) == ActionRetour::Ignorer, "chargement : ignoré");
    VERIFIE(actionRetour(Ecran::Course, false, false, false) == ActionRetour::Pause, "course : pause");
    VERIFIE(actionRetour(Ecran::Course, false, true, false) == ActionRetour::Pause, "course (boutons à l'écran) : pause quand même");
    VERIFIE(actionRetour(Ecran::Pause, false, true, false) == ActionRetour::Reprise, "pause : reprise");
    VERIFIE(actionRetour(Ecran::Course, true, true, false) == ActionRetour::BoutonRetour, "fin de course : bouton Menu");
    VERIFIE(actionRetour(Ecran::Course, true, false, false) == ActionRetour::Menu, "fin de course à 5 / duel (sans zone) : menu");
    VERIFIE(actionRetour(Ecran::Menu, false, false, true) == ActionRetour::FermerModes, "menu, panneau des modes : fermé");
    VERIFIE(actionRetour(Ecran::Menu, false, true, false) == ActionRetour::BoutonRetour, "menu, réglages ouverts : Fermer");
    VERIFIE(actionRetour(Ecran::Menu, false, false, false) == ActionRetour::ArrierePlan, "menu principal : arrière-plan");
    VERIFIE(actionRetour(Ecran::Garage, false, true, false) == ActionRetour::BoutonRetour, "garage : Retour");
    VERIFIE(actionRetour(Ecran::Terrain, false, true, false) == ActionRetour::BoutonRetour, "terrain : Retour");
    VERIFIE(actionRetour(Ecran::Salon, false, false, false) == ActionRetour::Menu, "salon (sans zone) : menu");
    std::printf("décisions par écran : %s\n", echecs ? "ÉCHEC" : "OK");

    // RG-06 : parcours complet
    Modele m;
    m.modes = true; m.retour(); VERIFIE(m.e == Ecran::Menu && !m.modes && !m.dehors, "accueil → mode : panneau refermé, toujours au menu");
    m.e = Ecran::Garage; m.retour(); VERIFIE(m.e == Ecran::Menu, "garage → menu");
    m.e = Ecran::Terrain; m.retour(); VERIFIE(m.e == Ecran::Menu, "terrain → menu");
    m.e = Ecran::Course; m.retour(); VERIFIE(m.e == Ecran::Pause, "course → pause");
    m.retour(); VERIFIE(m.e == Ecran::Course, "pause → course");
    m.retour(); VERIFIE(m.e == Ecran::Pause, "course → pause (2e fois)");
    m.e = Ecran::Menu;   // « Menu » de la pause
    m.reglages = true; m.retour(); VERIFIE(m.e == Ecran::Menu && !m.reglages && !m.dehors, "réglages → fermés");
    m.e = Ecran::Course; m.finie = true; m.retour(); VERIFIE(m.e == Ecran::Menu, "fin de course → menu"); m.finie = false;
    m.e = Ecran::Chargement; m.retour(); VERIFIE(m.e == Ecran::Chargement && !m.dehors, "chargement : rien");
    m.e = Ecran::Menu; m.retour(); VERIFIE(m.dehors, "menu principal → arrière-plan (sans fermer le jeu)");
    std::printf("parcours complet : %s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
