// WaForSpeed — test WFS-429 : boutons « Menu » et « Caméra » de la course, agrandis (5 %, puis × 1,5 : WFS-549) ; WFS-551 (RG-06) :
// en haut au centre (avant : à côté du score).
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
    // RG-02 : chaque dimension × BOUTON_ECHELLE ; WFS-549 (RG-07, maquette 17b) : 1,5 (40 → 60, 18 → 27, 14 → 21, biais 8 → 12), à k = 1 ; WFS-557 (maquette 17) : texte 29, écart icône-texte 10
    const hud::BoutonHud b1 = hud::mesuresBouton(1.f, 0.f);
    VERIFIE(std::fabs(b1.h - 60.f) < 1e-4f, "RG-02 : hauteur %.3f (attendu 60)", b1.h);
    VERIFIE(std::fabs(b1.icone - 27.f) < 1e-4f && std::fabs(b1.texte - 29.f) < 1e-3f && std::fabs(b1.marge - 21.f) < 1e-4f,
            "RG-02 : icône %.3f, texte %.3f, marge %.3f (attendu 27 / 29 / 21, WFS-557)", b1.icone, b1.texte, b1.marge);
    VERIFIE(std::fabs(b1.ecartIcone - 10.f) < 1e-3f && std::fabs(b1.biais - 12.f) < 1e-4f, "RG-02 : écart %.3f, biais %.3f", b1.ecartIcone, b1.biais);
    VERIFIE(std::fabs(hud::BOUTON_ECHELLE - 1.5f) < 1e-6f, "RG-02 : échelle %.3f", hud::BOUTON_ECHELLE);
    VERIFIE(b1.h <= hud::SCORE_H, "WFS-549 : boutons pas plus hauts que le score (%.1f)", b1.h);

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
                hud::placerBoutons(e.W, e.H, m, menu, cam, poursuite);
                const float xScore = e.W - m.droite - 16 * k - 180 * k, yScore = m.haut + 14 * k;
                char nom[96];
                std::snprintf(nom, sizeof nom, "%.0f×%.0f encoche %.0f%s%s", e.W, e.H, e.encoche, gaucher ? " gaucher" : "", poursuite ? " poursuite" : "");
                // RG-01 : ordre, écarts de 8 px, même rangée, centrés sur les 70 px du score
                // WFS-551 (RG-06) : la paire est centrée en haut de la zone sûre, à gauche du score sans le toucher
                if (!poursuite) VERIFIE(std::fabs((menu.x + cam.x + cam.w) / 2 - (m.gauche + e.W - m.droite) / 2) < 0.01f, "%s : RG-06 : paire au centre", nom);
                else VERIFIE(menu.x >= (m.gauche + e.W - m.droite) / 2 + 20 * e.H / 100 && cam.x + cam.w <= e.W - m.droite, "%s : RG-06 : poursuite : à droite du score de la poursuite", nom);
                VERIFIE(cam.x + cam.w + 8 * k <= xScore, "%s : RG-06 : Caméra avant le score", nom);
                VERIFIE(std::fabs(menu.x + menu.w + hud::BOUTONS_ECART * k - cam.x) < 0.01f, "%s : RG-01 : écart Menu-Caméra", nom);
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
