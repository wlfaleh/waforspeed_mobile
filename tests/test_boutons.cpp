// WaForSpeed — test WFS-549 : contour arrondi des boutons R19 (ui::boutons::contourArrondi, calcul seul).
// Vérifie : sommets dans le rectangle, sens horaire à l'écran, polygone convexe (dessin2d::polygone), nombre borné
// (MAX_SOMMETS), coins vifs à rayon nul, pilule (rayon borné à la demi-hauteur) sans sommet en double, aire proche de la vraie.
#include <cmath>
#include <cstdio>
#include "../src/ui/boutons.h"

using namespace wfs;
static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static void verifier(float x, float y, float w, float h, float rayon) {
    vec2 p[ui::boutons::MAX_SOMMETS];
    const int n = ui::boutons::contourArrondi(p, x, y, w, h, rayon);
    VERIFIE(n >= 4 && n <= ui::boutons::MAX_SOMMETS, "%gx%g r=%g : %d sommets", w, h, rayon, n);
    double aire = 0;
    for (int i = 0; i < n; i++) {
        const vec2 a = p[i], b = p[(i + 1) % n], c = p[(i + 2) % n];
        VERIFIE(a.x >= x - 1e-3f && a.x <= x + w + 1e-3f && a.y >= y - 1e-3f && a.y <= y + h + 1e-3f, "%gx%g r=%g : sommet %d hors du rectangle", w, h, rayon, i);
        VERIFIE(std::hypot(b.x - a.x, b.y - a.y) > 1e-3f, "%gx%g r=%g : sommets %d et %d confondus", w, h, rayon, i, (i + 1) % n);
        // y vers le bas : sens horaire à l'écran = produit vectoriel positif ; convexe = jamais négatif
        const float z = (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x);
        VERIFIE(z >= -1e-3f, "%gx%g r=%g : angle rentrant au sommet %d (%.4f)", w, h, rayon, (i + 1) % n, z);
        aire += (double)a.x * b.y - (double)b.x * a.y;
    }
    aire /= 2;
    const float R = std::fmin(std::fmax(rayon, 0.f), std::fmin(w, h) / 2);
    const double vraie = (double)w * h - (4 - PI) * R * R;
    VERIFIE(aire > 0 && std::fabs(aire - vraie) <= 0.01 * vraie, "%gx%g r=%g : aire %.1f (attendu %.1f)", w, h, rayon, aire, vraie);
}

int main() {
    verifier(10, 20, 460, 70, 35);     // pilule du menu (Course solo, 460 × 70)
    verifier(0, 0, 200, 52, 26);       // Start/Stop « Acheter » du garage
    verifier(5, 5, 120, 120, 60);      // bouton rond (maquettes 19, 27)
    verifier(0, 0, 300, 40, 1000);     // rayon trop grand : borné à la demi-hauteur
    verifier(0, 0, 84, 40, 9);         // choix de 84 × 40 (distance du salon), coins arrondis
    verifier(0, 0, 50, 30, 0);         // coins vifs
    vec2 p[ui::boutons::MAX_SOMMETS];
    VERIFIE(ui::boutons::contourArrondi(p, 0, 0, 50, 30, 0) == 4, "rayon nul : 4 sommets");
    VERIFIE(ui::boutons::contourArrondi(p, 0, 0, 0, 30, 5) == 0, "largeur nulle : rien");
    if (echecs) { std::printf("test_boutons : %d échec(s)\n", echecs); return 1; }
    std::printf("test_boutons : OK\n");
    return 0;
}
