// WaForSpeed — bouton / geste « Retour » du téléphone (WFS-116) : voir retour_systeme.h.
#include "retour_systeme.h"
#include <algorithm>
#include "../core/log.h"
#include "../game/course5.h"
#include "../game/course_system.h"
#include "../game/game.h"
#include "manette.h"
#include "manette_jeu.h"
#include "../ui/ecrans/reglages.h"   // WFS-290 (accroche, RG-04) : réglages ouverts en pause

namespace wfs {

namespace {
bool g_enfoncee = false;       // un appui en cours (DOWN reçu)
bool g_laisse = false;         // cet appui est laissé à Android (menu principal)
int g_trace = 0;               // images restantes pour tracer le changement d'écran
Ecran g_avant = Ecran::Menu;
ActionRetour g_action = ActionRetour::Ignorer;

ActionRetour decider(const Game& g) {
    const std::vector<manette::Zone>& z = manette::zonesPrecedentes();
    const bool retour = std::any_of(z.begin(), z.end(), [](const manette::Zone& q) { return q.retour; });
    const Course* c = activeCourse();
    const bool finie = c && c->state == RaceState::Over;
    return actionRetourReglages(g.ecran(), finie, retour, g.ecran() == Ecran::Menu && choixModeOuvert(), ecrans::reglagesOuverts());   // WFS-290
}
} // namespace

const char* nomActionRetour(ActionRetour a) {
    switch (a) {
        case ActionRetour::Ignorer: return "ignoré";
        case ActionRetour::Pause: return "pause";
        case ActionRetour::Reprise: return "reprise";
        case ActionRetour::FermerModes: return "panneau des modes fermé";
        case ActionRetour::BoutonRetour: return "bouton « Retour » de l'écran";
        case ActionRetour::Menu: return "menu";
        case ActionRetour::ArrierePlan: return "arrière-plan";
    }
    return "?";
}

bool retourSysteme(Game& g, bool enfoncee, int repetition) {
    if (enfoncee) {   // la décision « laisser à Android » se prend à l'appui : l'appui et le relâcher vont au même endroit
        if (repetition > 0) return !g_laisse;   // touche tenue : répétitions ignorées (RG-01)
        g_enfoncee = true;
        g_laisse = decider(g) == ActionRetour::ArrierePlan;
        return !g_laisse;
    }
    if (!g_enfoncee) return true;   // relâcher sans appui (appui commencé avant le jeu) : pris, sans effet
    g_enfoncee = false;
    if (g_laisse) {
        g_laisse = false;
        WFS_LOG("retour (téléphone) : écran %s → arrière-plan (JeuActivity.moveTaskToBack)", nomEcran(g.ecran()));
        return false;
    }
    const ActionRetour a = decider(g);   // RG-01 : l'action part au relâcher
    g_avant = g.ecran(); g_action = a;
    switch (a) {
        case ActionRetour::Ignorer: WFS_LOG("retour (téléphone) : écran %s → ignoré", nomEcran(g.ecran())); return true;
        case ActionRetour::Pause: g.allerA(Ecran::Pause); break;
        case ActionRetour::Reprise: g.allerA(Ecran::Course); break;
        case ActionRetour::FermerModes: choixModeBanc(false); break;   // referme le panneau (course5.h)
        case ActionRetour::BoutonRetour: {   // comme B de la manette (manette_jeu.cpp), sans passer en mode manette (RG-05)
            const std::vector<manette::Zone>& z = manette::zonesPrecedentes();
            const auto r = std::find_if(z.begin(), z.end(), [](const manette::Zone& q) { return q.retour; });
            if (r != z.end()) manette::toucher(g, *r);
            break;
        }
        case ActionRetour::Menu: g.allerA(Ecran::Menu); break;
        case ActionRetour::ArrierePlan: return false;   // panneau fermé entre l'appui et le relâcher : laissé à Android
    }
    g_trace = 3;   // l'écran change à la mise à jour suivante (Game::allerA) : trace après une à trois images
    return true;
}

void retourSystemeApresImage(const Game& g) {
    if (g_trace <= 0) return;
    if (g.ecran() != g_avant || --g_trace == 0) {
        WFS_LOG("retour (téléphone) : écran %s → %s (%s)", nomEcran(g_avant), nomEcran(g.ecran()), nomActionRetour(g_action));
        g_trace = 0;
    }
}

} // namespace wfs
