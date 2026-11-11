// WaForSpeed — test WFS-429 : boutons « Menu » et « Caméra » de la course à côté du score, agrandis de 5 %.
// Vérifie par le calcul (hud::mesuresBouton, hud::placerBoutons) : RG-01 (rangée [MENU] [CAMÉRA] [SCORE], écarts de 8 px,
// centrés sur les 70 px du score), RG-02 (chaque dimension × 1,05), RG-04 (rien ne se recouvre : temps, score et combo,
// carte GPS, bandeaux, commandes, nitro ; HUD de la poursuite) à 2400 × 1080 et 1600 × 720, avec et sans encoche, droitier
// et gaucher (RG-07 : le score ne bouge pas, les boutons non plus).
#include <cmath>
#include <cstdio>
#include "../src/ui/minicarte.h"

using namespace wfs;
static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static Cadre cadre(const hud::BoutonHud& b) { return {b.x, b.y, b.w, b.h}; }

int main() {
    // RG-02 : chaque dimension × 1,05 (40 → 42, 18 → 18,9, 14 → 14,7, 6 → 6,3, biais 8 → 8,4), à k = 1
    const hud::BoutonHud b1 = hud::mesuresBouton(1.f, 0.f);
    VERIFIE(std::fabs(b1.h - 42.f) < 1e-4f, "RG-02 : hauteur %.3f (attendu 42)", b1.h);
    VERIFIE(std::fabs(b1.icone - 18.9f) < 1e-4f && std::fabs(b1.texte - 14.7f) < 1e-4f && std::fabs(b1.marge - 14.7f) < 1e-4f,
            "RG-02 : icône %.3f, texte %.3f, marge %.3f (attendu 18,9 / 14,7 / 14,7)", b1.icone, b1.texte, b1.marge);
    VERIFIE(std::fabs(b1.ecartIcone - 6.3f) < 1e-4f && std::fabs(b1.biais - 8.4f) < 1e-4f, "RG-02 : écart %.3f, biais %.3f", b1.ecartIcone, b1.biais);
    VERIFIE(std::fabs(hud::BOUTON_ECHELLE - 1.05f) < 1e-6f, "RG-02 : échelle %.3f", hud::BOUTON_ECHELLE);

    struct Ecran { float W, H, encoche; };
    const Ecran ecrans[] = {{2400, 1080, 0}, {1600, 720, 0}, {2400, 1080, 90}, {2340, 1080, 0}};
    for (const Ecran& e : ecrans)
        for (int gaucher = 0; gaucher < 2; gaucher++)
            for (int poursuite = 0; poursuite < 2; poursuite++) {
                const float k = e.H / 720.f;
                const Marges m{gaucher ? 0.f : e.encoche, 0, gaucher ? e.encoche : 0.f, 0};
                // largeurs de texte majorées (Rajdhani Bold ≈ 0,5 em par capitale ; 0,75 em + espacement 0,1 em ici) :
                // « MENU » et le plus long nom de vue (« COCKPIT », 7 lettres)
                const float tp = hud::BOUTON_TEXTE * k * hud::BOUTON_ECHELLE;
                hud::BoutonHud menu = hud::mesuresBouton(k, 4 * 0.85f * tp), cam = hud::mesuresBouton(k, 7 * 0.85f * tp);
                hud::placerBoutons(e.W, e.H, m, menu, cam);
                const float xScore = e.W - m.droite - 16 * k - 180 * k, yScore = m.haut + 14 * k;
                char nom[96];
                std::snprintf(nom, sizeof nom, "%.0f×%.0f encoche %.0f%s%s", e.W, e.H, e.encoche, gaucher ? " gaucher" : "", poursuite ? " poursuite" : "");
                // RG-01 : ordre, écarts de 8 px, même rangée, centrés sur les 70 px du score
                VERIFIE(std::fabs(cam.x + cam.w + 8 * k - xScore) < 0.01f, "%s : RG-01 : écart Caméra-score", nom);
                VERIFIE(std::fabs(menu.x + menu.w + 8 * k - cam.x) < 0.01f, "%s : RG-01 : écart Menu-Caméra", nom);
                VERIFIE(std::fabs(menu.y - cam.y) < 0.01f && std::fabs(cam.y + cam.h / 2 - (yScore + 35 * k)) < 0.01f,
                        "%s : RG-01 : rangée centrée sur le score", nom);
                VERIFIE(menu.x >= m.gauche && menu.y >= m.haut, "%s : boutons dans la zone sûre", nom);
                // RG-04 : aucun recouvrement (obstacles de la carte GPS : temps, score + combo, bandeaux, commandes, nitro,
                // flèches ; HUD de la poursuite) ni avec la carte elle-même
                const minicarte::Obstacles o = minicarte::obstacles(e.W, e.H, m, poursuite, gaucher);
                for (int i = 0; i < o.n; i++) {
                    VERIFIE(!minicarte::recoupe(cadre(menu), o.c[i], 0), "%s : RG-04 : Menu recouvre l'obstacle %d", nom, i);
                    VERIFIE(!minicarte::recoupe(cadre(cam), o.c[i], 0), "%s : RG-04 : Caméra recouvre l'obstacle %d", nom, i);
                }
                const Cadre carte = minicarte::placer(e.W, e.H, m, o, poursuite);
                VERIFIE(!minicarte::recoupe(cadre(menu), carte, 0) && !minicarte::recoupe(cadre(cam), carte, 0), "%s : RG-04 : carte GPS recouverte", nom);
                if (!gaucher && e.encoche == 0)
                    std::printf("%s : Menu (%.0f ; %.0f) %.0f × %.1f, Caméra (%.0f ; %.0f) %.0f × %.1f, score x = %.0f, carte (%.0f ; %.0f) %.0f\n",
                                nom, menu.x, menu.y, menu.w, menu.h, cam.x, cam.y, cam.w, cam.h, xScore, carte.x, carte.y, carte.w);
            }
    if (echecs) { std::printf("test_hud_boutons : %d échec(s)\n", echecs); return 1; }
    std::printf("test_hud_boutons : OK\n");
    return 0;
}
