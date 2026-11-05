// Un doigt = un appui (WFS-113) : la flèche du showroom avance d'exactement une voiture par appui, même quand le doigt
// bouge un peu pendant l'appui (sur Android, chaque mouvement arrive comme un toucher « appuyé »).
// usage : test_appui
#include <cstdio>
#include "../src/ui/appui.h"
using namespace wfs::ui;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// showroom simulé : 20 voitures, flèche « suivante » et « précédente » qui agissent à l'appui (comme garage.cpp)
struct Showroom {
    int index = 0; FiltreAppui filtre; bool avecFiltre = true;
    void toucher(int id, bool down, int dir) {
        if (avecFiltre && filtre.consomme(id, down)) return;
        if (!down) return;
        if (avecFiltre) filtre.prendre(id);
        index = (index + dir + 20) % 20;
    }
    // un appui réel : posé, `mouvements` micro-mouvements (Android : ACTION_MOVE → down), relevé
    void appui(int id, int dir, int mouvements = 6) {
        toucher(id, true, dir);
        for (int m = 0; m < mouvements; m++) toucher(id, true, dir);
        toucher(id, false, dir);
    }
};

int main() {
    // avant WFS-113 : 1 appui avec 6 micro-mouvements faisait avancer de 7 voitures
    Showroom avant; avant.avecFiltre = false; avant.appui(1, 1);
    std::printf("sans filtre : 1 appui (6 micro-mouvements) → %d voitures d'avance (bug reproduit)\n", avant.index);
    VERIFIE(avant.index == 7, "bug non reproduit (%d)", avant.index);

    // 20 appuis sur « suivante » : 20 voitures consécutives, puis retour à la 1re
    Showroom s;
    bool consecutif = true;
    for (int k = 1; k <= 20; k++) { s.appui(k, 1); consecutif = consecutif && s.index == k % 20; }
    VERIFIE(consecutif && s.index == 0, "20 appuis → 20 voitures puis la 1re (index %d)", s.index);
    VERIFIE(s.filtre.nombre() == 0, "doigts libérés au relâcher (%d)", s.filtre.nombre());
    std::printf("20 appuis : 20 voitures consécutives, retour à la 1re : OK\n");

    // 40 appuis alternés (suivante, précédente) : jamais de saut, retour au point de départ
    Showroom a; a.index = 5;
    bool ok = true;
    for (int k = 0; k < 40; k++) { const int avantIdx = a.index; a.appui(100 + k, k % 2 ? -1 : 1, 3 + k % 5);
        ok = ok && (a.index == (avantIdx + (k % 2 ? -1 : 1) + 20) % 20); }
    VERIFIE(ok && a.index == 5, "40 appuis alternés sans saut (index %d)", a.index);
    std::printf("40 appuis alternés : une voiture à chaque appui, aucun saut : OK\n");

    // deux doigts : chacun compte pour un appui, le mouvement de l'un n'agit pas
    Showroom d;
    d.toucher(1, true, 1); d.toucher(2, true, 1); d.toucher(1, true, 1); d.toucher(2, true, 1);
    d.toucher(1, false, 1); d.toucher(2, false, 1);
    VERIFIE(d.index == 2 && d.filtre.nombre() == 0, "deux doigts → deux voitures (%d)", d.index);
    std::printf("deux doigts : deux appuis, deux voitures : OK\n");

    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("un doigt = un appui : tout est OK\n");
    return 0;
}
