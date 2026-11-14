// WFS-542 (ANO-273) : le camion en travers de la Poursuite est un vrai camion porteur à l'échelle 1.
//  RG-01 : mesures du modèle livré (assets/world/camion/porteur.wfm, boîte de l'en-tête) = references/reel/camion_poursuite.json ±1 %,
//          poids ≤ 8 Mo, version allégée (.lod, RG-05) présente ;
//  RG-02 : boîte de choc = vraies mesures ; la file libre garde au moins la largeur de la plus large voiture du jeu + 1 m ;
//          côté fermé, l'écart au bord est plus étroit que la plus étroite des voitures (aucune n'y passe) ;
//  RG-04 : retiré 50 m derrière le joueur ou 60 s après son apparition.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include "../src/game/cars.h"
#include "../src/game/poursuite.h"
#include "../src/game/track.h"
using namespace wfs;
using namespace wfs::poursuite;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static bool proche(double v, double ref, double tol = 0.01) { return std::abs(v - ref) <= tol * ref; }

int main() {
    const std::string base = std::string(WFS_SOURCE_DIR) + "/assets/world/camion/porteur";
    // ---------- RG-01 : modèle livré, mesures du réel ±1 %
    std::ifstream f(base + ".wfm", std::ios::binary);
    std::vector<char> o((std::istreambuf_iterator<char>(f)), {});
    VERIFIE(o.size() > 40 && !std::memcmp(o.data(), "WFM1", 4), "porteur.wfm absent ou illisible (%zu octets)", o.size());
    if (o.size() > 40) {
        float b[6]; std::memcpy(b, o.data() + 20, sizeof b);   // 'WFM1' version nTex nMat nMesh, puis bbox min xyz, max xyz
        const double larg = b[3] - b[0], haut = b[4] - b[1], lon = b[5] - b[2];
        std::printf("porteur.wfm : %.3f m de long, %.3f m de haut, %.3f m de large (rétroviseurs), %.2f Mo\n", lon, haut, larg, o.size() / 1048576.0);
        VERIFIE(proche(lon, 6.19), "longueur %.3f m (6,19 ± 1 %%)", lon);
        VERIFIE(proche(haut, 2.93), "hauteur %.3f m (2,93 ± 1 %%)", haut);
        VERIFIE(proche(larg, 2.69), "largeur aux rétroviseurs %.3f m (2,69 ± 1 %%)", larg);
        VERIFIE(larg <= 2.55 + 0.3 && b[1] > -0.01 && b[1] < 0.01, "gabarit : posé au sol (y min %.3f), largeur %.2f m", b[1], larg);
        VERIFIE(proche(CAMION_L, lon) && proche(CAMION_H, haut), "constantes du code (%.2f × %.2f m) = modèle (%.3f × %.3f m)", CAMION_L, CAMION_H, lon, haut);
        VERIFIE(o.size() <= 8u * 1024 * 1024, "porteur.wfm : %.2f Mo (8 Mo au plus)", o.size() / 1048576.0);
    }
    std::ifstream l(base + ".lod", std::ios::binary);
    char tete[4] = {};
    VERIFIE(l.read(tete, 4) && !std::memcmp(tete, "WFL2", 4), "porteur.lod (version allégée, RG-05) absente ou illisible");
    VERIFIE(std::string(CAMION_MODELE) == "world/camion/porteur.wfm", "modèle chargé : %s", CAMION_MODELE);

    // ---------- RG-02 : boîte de choc à l'échelle 1, file libre, côté fermé infranchissable
    double largeMax = 0, etroiteMin = 9;
    for (int i = 0; i < CAR_COUNT; i++) { largeMax = std::max(largeMax, (double)CARS[i].d.W); etroiteMin = std::min(etroiteMin, (double)CARS[i].d.W); }
    for (int libre : {0, 2}) {
        const Travers tv = camionTravers(libre);
        VERIFIE(proche(2 * tv.demiX, CAMION_L, 1e-9) && proche(2 * tv.demiZ, CAMION_LARGEUR, 1e-9), "boîte %.2f × %.2f m (échelle 1)", 2 * tv.demiX, 2 * tv.demiZ);
        const double a = tv.xM - tv.demiX, b = tv.xM + tv.demiX;   // largeur couverte (m)
        const double libreM = libre == 0 ? a + RW : RW - b, fermeM = libre == 0 ? RW - b : a + RW;
        std::printf("camion (file libre %d) : couvre [%.2f ; %.2f] m, passage libre %.2f m (voiture la plus large %.2f m), écart côté fermé %.2f m (la plus étroite %.2f m)\n", libre, a, b, libreM, largeMax, fermeM, etroiteMin);
        VERIFIE(libreM >= largeMax + 1, "file libre %d : %.2f m < %.2f m (voiture la plus large + 1 m)", libre, libreM, largeMax + 1);
        VERIFIE(fermeM < etroiteMin, "côté fermé : %.2f m, une voiture de %.2f m y passerait", fermeM, etroiteMin);
        const double fl0 = FILES[libre] * RW - RW / 3, fl1 = FILES[libre] * RW + RW / 3;   // la file libre (5 m)
        VERIFIE(b <= fl0 || a >= fl1, "la file libre [%.1f ; %.1f] m est touchée [%.1f ; %.1f]", fl0, fl1, a, b);
        const int milieu = 1, fermee = 2 - libre;   // file du milieu et file fermée : barrées pour une voiture qui y roule au centre
        for (int ff : {milieu, fermee}) {
            const double c = FILES[ff] * RW;
            VERIFIE(c + etroiteMin / 2 > a && c - etroiteMin / 2 < b, "file %d (centre %.1f m) non barrée par [%.1f ; %.1f]", ff, c, a, b);
        }
    }
    // ---------- RG-04 : retrait
    VERIFIE(!camionARetirer(49.9, 59.9) && camionARetirer(50, 0) && camionARetirer(-200, 60) && !camionARetirer(-200, 0), "retrait du camion (50 m derrière, 60 s)");
    std::printf("camion de la Poursuite (WFS-542) : %s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
