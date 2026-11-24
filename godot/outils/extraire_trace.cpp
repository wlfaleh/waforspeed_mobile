// WaForSpeed — prototype Godot (E25) : extrait le tracé d'une carte du jeu actuel (même code, aucun fichier du jeu modifié)
// en JSON pour le projet Godot : un point tous les 2 m (x, y, z en mètres, Track::pose est déjà en mètres, cap th en radians, pente).
//   g++ -std=c++20 -O2 -I src godot/outils/extraire_trace.cpp <sources du tracé> -o build/extraire_trace
//   build/extraire_trace ville > godot/projet/donnees/ville.json
#include <cstdio>
#include <cmath>
#include "../../src/game/track.h"
#include "../../src/game/trace_cartes.h"
using namespace wfs;
int main(int argc, char** argv) {
    const char* id = argc > 1 ? argv[1] : "ville";
    Track t; t.build(mapById(id), trace::versionCarte(id));
    const double pas = 2.0 / M;   // 2 m en unités du jeu
    const int n = (int)(t.length() / pas);
    std::printf("{\"carte\":\"%s\",\"longueur_m\":%.2f,\"demi_largeur_m\":%.2f,\"points\":[", id, t.length() * M, RW);
    for (int i = 0; i < n; i++) {
        const TrackPoint p = t.pointLisse(i * pas, 0);
        std::printf("%s[%.3f,%.3f,%.3f,%.5f,%.5f]", i ? "," : "", p.x, p.y, p.z, p.th, p.slope);
    }
    std::printf("]}\n");
}
