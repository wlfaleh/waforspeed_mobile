// WaForSpeed — test WFS-551 (épic E23, interface R19) : calculs des icônes de personnalisation, vérifiés sans écran.
//   - RG-01 : contour de la goutte (pointe en haut, ventre rond en bas) : convexe, sens horaire, dans sa case, pointe au milieu ;
//   - RG-03 : masques de l'icône du pistolet de l'utilisateur : blanc et noir → gris gardé (part colorée 0), couleurs vives → part
//     colorée 1, opacité recopiée ;
//   - RG-04 : part du cadenas-jauge = solde / prix, bornée à [0, 1] (l'anse s'ouvre à 100 %).
#include <cmath>
#include <cstdio>
#include "../src/ui/garage_achat.h"
#include "../src/ui/garage_couleur.h"
#include "../src/ui/garage_finition_pistolet.h"

using namespace wfs;
using namespace wfs::ecrans::garage;
static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    // RG-01 : goutte de 30 px de haut, pointe en (15 ; 0)
    vec2 p[1 + GOUTTE_ARC];
    const int n = contourGoutte(p, 15, 0, 30);
    VERIFIE(n == 1 + GOUTTE_ARC && n <= 64, "RG-01 : %d sommets (au plus 64, dessin2d)", n);
    VERIFIE(std::fabs(p[0].x - 15) < 1e-4f && std::fabs(p[0].y) < 1e-4f, "RG-01 : pointe en (%.2f ; %.2f)", p[0].x, p[0].y);
    float ymax = -1e9f, xmin = 1e9f, xmax = -1e9f;
    for (int i = 0; i < n; i++) {
        ymax = std::fmax(ymax, p[i].y); xmin = std::fmin(xmin, p[i].x); xmax = std::fmax(xmax, p[i].x);
        const vec2 a = p[i], b = p[(i + 1) % n], c = p[(i + 2) % n];
        const float z = (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x);
        VERIFIE(z >= -1e-3f, "RG-01 : contour non convexe ou pas dans le sens horaire au sommet %d (%.4f)", i, z);
    }
    VERIFIE(std::fabs(ymax - 30) < 0.05f, "RG-01 : bas de la goutte %.2f (attendu 30)", ymax);
    VERIFIE(xmin >= 15 - 30 * 0.36f - 1e-3f && xmax <= 15 + 30 * 0.36f + 1e-3f && std::fabs((xmin + xmax) / 2 - 15) < 0.05f,
            "RG-01 : largeur [%.2f ; %.2f], centrée sur la pointe", xmin, xmax);
    VERIFIE(contourGoutte(p, 0, 0, 0) == 0, "RG-01 : hauteur nulle : rien");

    // RG-03 : masques du pistolet
    using ecrans::garage::pistolet::masque;
    const auto blanc = masque(255, 255, 255, 255), noir = masque(0, 0, 0, 200), gris = masque(128, 128, 136, 255);
    VERIFIE(blanc.g == 255 && blanc.b == 0 && blanc.a == 255, "RG-03 : blanc (%d, %d, %d)", blanc.g, blanc.b, blanc.a);
    VERIFIE(noir.g == 0 && noir.b == 0 && noir.a == 200, "RG-03 : noir (%d, %d, %d)", noir.g, noir.b, noir.a);
    VERIFIE(gris.b == 0 && gris.g == 136, "RG-03 : gris gardé (%d, %d)", gris.g, gris.b);
    const auto bleu = masque(40, 80, 255, 255), jaune = masque(255, 200, 20, 128), magenta = masque(230, 30, 200, 255);
    VERIFIE(bleu.b == 255 && jaune.b == 255 && magenta.b == 255, "RG-03 : couleurs vives teintées (%d, %d, %d)", bleu.b, jaune.b, magenta.b);
    VERIFIE(jaune.a == 128 && bleu.g == 255, "RG-03 : opacité et luminosité recopiées (%d, %d)", jaune.a, bleu.g);
    const auto pastel = masque(200, 180, 160, 255);   // saturation 0,2 : surtout gris
    VERIFIE(pastel.b < 40, "RG-03 : couleur presque grise peu teintée (%d)", pastel.b);

    // RG-04 : part du cadenas-jauge
    VERIFIE(std::fabs(partJauge(1250, 6000) - 1250.f / 6000.f) < 1e-6f, "RG-04 : 1 250 / 6 000");
    VERIFIE(partJauge(7000, 6000) == 1.f && partJauge(6000, 6000) == 1.f, "RG-04 : solde suffisant : 100 %% (anse ouverte)");
    VERIFIE(partJauge(-5, 6000) == 0.f && partJauge(0, 6000) == 0.f, "RG-04 : solde nul ou négatif : 0 %%");
    VERIFIE(partJauge(10, 0) == 1.f, "RG-04 : prix nul : 100 %%");

    if (echecs) { std::printf("test_icones_r19 : %d échec(s)\n", echecs); return 1; }
    std::printf("test_icones_r19 : OK\n");
    return 0;
}
