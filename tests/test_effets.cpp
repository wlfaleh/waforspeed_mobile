// Effets de conduite (WFS-65) : particules identiques à js/world/particles.js — réserves de 220 étincelles et
// 140 bouffées, réserve circulaire, durées, gravité, frottement, rebond des étincelles, fumée qui monte, qualité basse.
// usage : test_effets
#include <cmath>
#include <cstdio>
#include "../src/render/effets_particules.h"
using namespace wfs;
using namespace wfs::effets;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    Particules p;
    VERIFIE(p.sparks().p.size() == 220 && p.smoke().p.size() == 140, "tailles des réserves");
    VERIFIE(std::abs(p.sparks().taille - 0.22f) < 1e-6f && std::abs(p.smoke().taille - 2.4f) < 1e-6f && std::abs(p.smoke().opacite - 0.45f) < 1e-6f, "tailles et opacité");

    // étincelles : 50 (crash), durée 0,55 × [0,6 ; 1,2], couleurs du jeu web
    p.etincelles({0, 1, 0}, 0.5f, {0, 1, -20}, 50);
    VERIFIE(p.sparks().vivantes() == 50, "50 étincelles émises (%d)", p.sparks().vivantes());
    bool couleursOk = true, dureesOk = true;
    for (const auto& q : p.sparks().p) if (q.vivante()) {
        const bool blanc = q.c.x == 1 && std::abs(q.c.y - 0.95f) < 1e-6f && std::abs(q.c.z - 0.8f) < 1e-6f;
        const bool orange = q.c.x == 1 && q.c.y >= 0.45f && q.c.y < 0.75f && std::abs(q.c.z - 0.1f) < 1e-6f;
        couleursOk &= blanc || orange;
        dureesOk &= q.max >= 0.55f * 0.6f - 1e-6f && q.max <= 0.55f * 1.2f + 1e-6f;
    }
    VERIFIE(couleursOk && dureesOk, "couleurs / durées des étincelles");
    // elles retombent et rebondissent sur le sol (jamais en dessous), puis s'éteignent avant 0,66 s
    float minY = 1e9f;
    for (int i = 0; i < 40; i++) { p.update(1.f / 60); for (const auto& q : p.sparks().p) if (q.vivante()) minY = std::min(minY, q.p.y); }
    VERIFIE(minY >= 0.5f - 1e-4f, "étincelle sous le sol (%.3f)", minY);
    VERIFIE(p.sparks().vivantes() == 0, "étincelles éteintes après 0,67 s (%d)", p.sparks().vivantes());
    std::printf("étincelles : 50 émises, couleurs et durées du jeu web, rebond sur le sol, éteintes à 0,67 s : OK\n");

    // réserve circulaire : 300 émissions → 220 au plus, les plus anciennes remplacées
    p.etincelles({0, 1, 0}, 0, {}, 300);
    VERIFIE(p.sparks().vivantes() == 220, "réserve circulaire (%d)", p.sparks().vivantes());
    p.vider();

    // fumée : monte (gravité −0,4, départ 1,2 m/s), dure 2,2 × [0,6 ; 1,2] s, gris k ∈ [0,25 ; 0,4]
    p.fumee({0, 2, 0}, 2);
    VERIFIE(p.smoke().vivantes() == 2, "2 bouffées");
    for (int i = 0; i < 60; i++) p.update(1.f / 60);
    for (const auto& q : p.smoke().p) if (q.vivante()) {
        VERIFIE(q.p.y > 2.5f, "la fumée monte (%.2f)", q.p.y);
        VERIFIE(q.c.x >= 0.25f && q.c.x <= 0.4f && std::abs(q.c.z - q.c.x * 1.1f) < 1e-5f, "gris de la fumée");
    }
    for (int i = 0; i < 100; i++) p.update(1.f / 60);
    VERIFIE(p.smoke().vivantes() == 0, "fumée dissipée avant 2,64 s");
    std::printf("fumée : monte, gris du jeu web, dissipée avant 2,64 s : OK\n");

    // qualité basse (RG-05) : moitié moins de particules
    p.setReduit(true); p.etincelles({0, 1, 0}, 0, {}, 40);
    VERIFIE(p.sparks().vivantes() == 20, "qualité basse : 20 étincelles (%d)", p.sparks().vivantes());
    std::printf("qualité basse : moitié moins de particules : OK\n");

    // tirages déterministes : même graine → mêmes particules
    Particules a, b; a.etincelles({0, 1, 0}, 0, {}, 10); b.etincelles({0, 1, 0}, 0, {}, 10);
    bool memes = true; for (size_t i = 0; i < 10; i++) memes &= a.sparks().p[i].v.x == b.sparks().p[i].v.x && a.sparks().p[i].max == b.sparks().p[i].max;
    VERIFIE(memes, "tirages non reproductibles");

    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("effets : tout est OK\n");
    return 0;
}
