// Règle d'éligibilité du shader léger (WFS-263 RG-01, test de WFS-264 RG-06) : distance, exclusions, cas limites.
// usage : test_shader_leger
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <limits>
#include "../src/render/shader_leger.h"
using namespace wfs::shader_leger;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    const uint32_t FACADE = 2097152u, DOUBLE = 16u, VTINT = 16384u, MASK = 32u, WALK = 512u, GROUND = 1024u;
    // distance : sphère entièrement au-delà de la fin du fondu (70 m)
    VERIFIE(eligible(FACADE, 100.f, 10.f), "façade à 100 m, rayon 10 (90 m au plus près) : légère");
    VERIFIE(!eligible(FACADE, 75.f, 10.f), "façade dont le point le plus proche est à 65 m : complète (dans le fondu)");
    VERIFIE(!eligible(FACADE, 80.f, 10.f), "point le plus proche à 70 m pile : complète (strictement au-delà)");
    VERIFIE(eligible(FACADE, 80.01f, 10.f), "70,01 m : légère");
    VERIFIE(!eligible(0u, 50.f, 1.f) && !eligible(0u, 5.f, 0.f), "près de la caméra : complète");
    VERIFIE(!eligible(0u, 1000.f, 2000.f), "objet immense qui entoure la caméra (route fusionnée, sol) : complète");
    // exclusions : voitures, vitres, route, transparents, fantôme, vitres d'immeuble (F_COMPLET de scene.frag)
    const uint32_t exclus[] = {1u, 2u, 4u, 8u, 64u, 128u, 256u, 131072u, 1048576u};
    for (uint32_t f : exclus) VERIFIE(!eligible(f, 500.f, 1.f), "drapeau %u exclu, même à 500 m", f);
    VERIFIE(!eligible(FACADE | 64u, 500.f, 1.f), "façade vitrée (GLASS) : complète");
    // drapeaux permis : décor ordinaire, double face, feuillage, découpe (la découpe est filtrée à part par le renderer), trottoir, sol
    for (uint32_t f : {0u, FACADE, DOUBLE, VTINT, MASK, WALK, GROUND}) VERIFIE(eligible(f, 200.f, 5.f), "drapeau %u permis à 200 m", f);
    // cas limites : sphère inconnue, distance invalide
    VERIFIE(!eligible(0u, 500.f, -1.f), "sphère inconnue (rayon < 0) : complète");
    VERIFIE(!eligible(0u, std::numeric_limits<float>::quiet_NaN(), 1.f), "distance NaN : complète");
    VERIFIE(eligible(0u, std::numeric_limits<float>::infinity(), 1.f), "distance infinie : légère");
    VERIFIE(DEBUT_M < FIN_M && FIN_M - DEBUT_M == 10.f, "fondu de 10 m (60 → 70 m)");
    if (echecs) { std::printf("test_shader_leger : %d échec(s)\n", echecs); return 1; }
    std::printf("shader léger (WFS-263, WFS-264 RG-06) : distance, exclusions et cas limites : OK\n");
    return 0;
}
