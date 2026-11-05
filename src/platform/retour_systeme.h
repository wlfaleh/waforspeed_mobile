// WaForSpeed — bouton / geste « Retour » du téléphone (WFS-116, épic E7).
// Avant : AKEYCODE_BACK n'était pris que s'il venait d'une manette ; sinon onInput renvoyait 0 et Android fermait le jeu
// (un glissement depuis le bord de l'écran en pleine course faisait perdre la course).
// Maintenant (RG-01 à RG-05) : un appui = un cran, au relâcher, répétitions ignorées :
//   - en course : pause ; en pause : reprise ; course finie (écran de fin) : son bouton « Retour », sinon le menu ;
//   - chargement : appui pris et ignoré ;
//   - panneau « Choisis ton mode » ouvert : il se referme ;
//   - autres écrans : le bouton « Retour » de l'écran (zones marquées retour de la manette, comme B), sinon le menu ;
//   - menu principal (rien à fermer) : l'appui est laissé à Android → JeuActivity.onBackPressed → moveTaskToBack(true)
//     (le jeu passe en arrière-plan sans se fermer).
// Le mode manette n'est jamais activé par cette touche (pas de cadre cyan, RG-05).
#pragma once
#include "../core/ecran.h"

namespace wfs {
class Game;

enum class ActionRetour { Ignorer, Pause, Reprise, FermerModes, BoutonRetour, Menu, ArrierePlan };

// RG-02 / RG-03 / RG-04 : décision seule (testée par tests/test_retour_systeme.cpp)
inline ActionRetour actionRetour(Ecran e, bool courseFinie, bool zoneRetour, bool panneauModes) {
    switch (e) {
        case Ecran::Chargement: return ActionRetour::Ignorer;
        case Ecran::Course:
            if (!courseFinie) return ActionRetour::Pause;
            return zoneRetour ? ActionRetour::BoutonRetour : ActionRetour::Menu;
        case Ecran::Pause: return ActionRetour::Reprise;
        case Ecran::Menu:
            if (panneauModes) return ActionRetour::FermerModes;
            return zoneRetour ? ActionRetour::BoutonRetour : ActionRetour::ArrierePlan;   // réglages ouverts : « Fermer »
        default: return zoneRetour ? ActionRetour::BoutonRetour : ActionRetour::Menu;
    }
}
const char* nomActionRetour(ActionRetour a);

// touche Retour du téléphone (onInput d'android_main.cpp) : enfoncee = AKEY_EVENT_ACTION_DOWN, repetition = getRepeatCount.
// Renvoie vrai si le jeu garde l'événement, faux pour le laisser à Android (menu principal : arrière-plan).
bool retourSysteme(Game& g, bool enfoncee, int repetition);
// à chaque image, après Game::update : trace « retour (téléphone) : écran a → b » une fois l'écran changé (RG-07)
void retourSystemeApresImage(const Game& g);

} // namespace wfs
