// WaForSpeed — tenue sur téléphone (WFS-123) : calcul seul (source du statut, plafond, rétrogradation) ; voir chauffe.h.
#include "chauffe.h"
#include <atomic>

namespace wfs {
namespace chauffe {
namespace {
std::atomic<Source> g_source{nullptr};
double g_tempsCourse = 0;
std::atomic<int> g_plafond{60};
}

void definirSource(Source s) { g_source = s; }
bool sourceActive() { return g_source.load() != nullptr; }
int statut() { const Source s = g_source.load(); return s ? s() : -1; }
double tempsCourse() { return g_tempsCourse; }
void avancerTempsCourse(double dt) { g_tempsCourse += dt; }
int plafondCourant() { return g_plafond; }
void definirPlafond(int ips) { g_plafond = ips; }

const char* nomQualite(int q) { return q <= 0 ? "Basse" : q == 1 ? "Moyenne" : "Haute"; }

int Regulateur::pas(double dt, int niveau, bool enCourse, bool horsCourse, double raceTime, int& qualite) {
    if (raceTime >= 0) {   // nouvelle course (décompte, « Rejouer », autre carte) : raceTime repart de 0
        if (raceTime + 1e-9 < dernierTemps) faitCourse = false;
        dernierTemps = raceTime;
    }
    if (choisi >= 0 && qualite != applique) choisi = -1;   // le joueur a changé lui-même la qualité : son choix
    froid = niveau == 0 ? froid + dt : 0;
    if (enCourse && niveau >= 2 && !faitCourse && qualite > 0) {   // RG-02 : la chauffe passe avant le choix du joueur
        if (choisi < 0) choisi = qualite;
        qualite--; applique = qualite; faitCourse = true;
        return RETRO;
    }
    if (horsCourse && choisi >= 0 && froid >= FROID_RETABLIR) {   // RG-02 : rétablie au menu, chauffe retombée depuis 60 s
        qualite = choisi; choisi = -1; applique = -1;
        return RETABLI;
    }
    return RIEN;
}

} // namespace chauffe

int chauffeNiveau() { return chauffe::niveauDepuisStatut(chauffe::statut()); }

} // namespace wfs
