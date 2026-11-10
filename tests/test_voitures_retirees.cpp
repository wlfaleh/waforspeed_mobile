// WFS-404 : voitures retirées de l'application (licence non commerciale : A 45, Elantra N, X5). Liste unique
// VOITURES_RETIREES (src/game/cars.h) : ni au garage, ni chez les adversaires, ni en Poursuite, ni en duel, ni dans les
// listes du cockpit ; une sauvegarde avec une voiture retirée repart sur la 1re voiture permise.
#include <cstdio>
#include <cstring>
#include <string>
#include "../src/game/cars.h"
#include "../src/game/camera_vue.h"
#include "../src/game/course5.h"
#include "../src/game/duel.h"
#include "../src/game/poursuite.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    // RG-00 / RG-01 : les 3 voitures NC sont dans la liste, et aucune voiture permise n'a de crédit NC ou ND
    for (const char* id : {"x5", "a45", "elantra"}) VERIFIE(voitureRetiree(id), "%s devrait être retirée", id);
    const VoituresPermises& v = voituresPermises();
    VERIFIE(v.n == CAR_COUNT - (int)std::size(VOITURES_RETIREES) && v.n == 17, "%d voitures permises (17 attendues)", v.n);
    for (int p = 0; p < v.n; p++) {
        const CarDef& c = CARS[v.idx[p]];
        VERIFIE(!voitureRetiree(c.id), "%s permise et retirée", c.id);
        VERIFIE(!std::strstr(c.credit, "NC") && !std::strstr(c.credit, "ND"), "%s : licence non commerciale (%s)", c.id, c.credit);
        VERIFIE(&voiturePermise(c.id) == &c, "%s permise : la même voiture attendue", c.id);
    }
    // RG-04 : sauvegarde avec une voiture retirée (ou inconnue) → 1re voiture permise (Golf)
    for (const char* id : {"x5", "a45", "elantra", "inconnue", ""})
        VERIFIE(!std::strcmp(voiturePermise(id).id, "golf"), "sauvegarde « %s » → %s (golf attendue)", id, voiturePermise(id).id);
    VERIFIE(!std::strcmp(voiturePermise(nullptr).id, "golf"), "id nul → golf");
    // RG-01 et RG-09 : adversaires de la Course à 5, pour chaque voiture du joueur et 200 graines : jamais une voiture retirée,
    // ni le Range Rover Sport (écarté jusqu'à WFS-394 ; enlever « && o.car != \"rrsport\" » avec la liste de course5.cpp)
    int tirages = 0;
    for (int p = 0; p < v.n; p++)
        for (uint32_t gr = 1; gr <= 200; gr++)
            for (const course5::Ordinateur& o : course5::tirer(gr, CARS[v.idx[p]].id)) { tirages++; VERIFIE(!voitureRetiree(o.car.c_str()) && o.car != "rrsport", "adversaire %s (joueur %s, graine %u)", o.car.c_str(), CARS[v.idx[p]].id, gr); }
    // RG-03 : camion de la Poursuite et voiture d'un pilote du duel
    for (int i = 0; i < CAR_COUNT; i++) VERIFIE(!voitureRetiree(poursuite::camionCar(CARS[i].id)), "camion %s (joueur %s)", poursuite::camionCar(CARS[i].id), CARS[i].id);
    for (const char* id : {"x5", "a45", "elantra"}) { duel::Fields f{{"car", id}}; VERIFIE(!std::strcmp(duel::peerCar(f), "golf"), "duel : %s → %s", id, duel::peerCar(f)); }
    { duel::Fields f{{"car", "m4"}}; VERIFIE(!std::strcmp(duel::peerCar(f), "m4"), "duel : m4 gardée"); }
    // RG-03 : listes des captures du cockpit sans voiture retirée
    for (const auto* lot : {&COCKPIT_LOT1, &COCKPIT_LOT2A, &COCKPIT_LOT2B, &COCKPIT_LOT3A, &COCKPIT_LOT3B})
        for (const std::string& id : *lot) VERIFIE(!voitureRetiree(id.c_str()), "cockpit : %s", id.c_str());
    std::printf("voitures retirées (WFS-404) : %d permises, %d adversaires tirés sans voiture retirée : %s\n", v.n, tirages, echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
