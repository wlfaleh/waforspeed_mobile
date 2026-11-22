// WFS-597 (RG-02, RG-05) : image de fond de l'écran de chargement (image 2 de l'utilisateur, telle quelle).
// 1) cadrage « couverture » (uvFond) : l'écran est rempli, l'image n'est jamais déformée (rapport de la zone lue = rapport de
//    l'écran), coupée sur un seul axe et centrée, en 20:9, 16:9, 4:3 et pour des tailles limites ;
// 2) fichier livré assets/ui/chargement/fond.jpg : JPEG de 2400 × 1080, 600 Ko au plus.
// usage : test_fond_chargement [dossier assets] (par défaut : assets/ depuis la racine du dépôt, ../assets/ depuis build/)
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "../src/ui/ecrans/chargement.h"
using namespace wfs::ecrans;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static void cadrage(float W, float H, const char* nom) {
    const auto uv = uvFond(W, H);
    const float lu = (uv[2] - uv[0]) * FOND_LARGEUR, lv = (uv[3] - uv[1]) * FOND_HAUTEUR;   // zone lue, en pixels de l'image
    VERIFIE(uv[0] >= 0 && uv[1] >= 0 && uv[2] <= 1 && uv[3] <= 1 && uv[0] < uv[2] && uv[1] < uv[3], "%s : zone dans l'image", nom);
    VERIFIE(std::fabs(lu / lv - W / H) < 1e-3f * (W / H), "%s : pas de déformation (%.4f contre %.4f)", nom, lu / lv, W / H);
    VERIFIE(std::fabs(uv[0] + uv[2] - 1) < 1e-5f && std::fabs(uv[1] + uv[3] - 1) < 1e-5f, "%s : centrée", nom);
    VERIFIE((uv[0] == 0 && uv[2] == 1) || (uv[1] == 0 && uv[3] == 1), "%s : coupée sur un seul axe", nom);
    std::printf("%-22s u %.3f..%.3f  v %.3f..%.3f\n", nom, uv[0], uv[2], uv[1], uv[3]);
}

// taille d'un JPEG lue dans son en-tête SOF (aucune bibliothèque)
static bool tailleJpeg(const std::vector<unsigned char>& d, int& w, int& h) {
    if (d.size() < 4 || d[0] != 0xFF || d[1] != 0xD8) return false;
    for (size_t i = 2; i + 9 < d.size();) {
        if (d[i] != 0xFF) return false;
        const int m = d[i + 1], n = (d[i + 2] << 8) | d[i + 3];
        if (m >= 0xC0 && m <= 0xC3) { h = (d[i + 5] << 8) | d[i + 6]; w = (d[i + 7] << 8) | d[i + 8]; return true; }
        i += 2 + (size_t)n;
    }
    return false;
}

int main(int argc, char** argv) {
    cadrage(2400, 1080, "20:9 (2400 × 1080)");
    VERIFIE(uvFond(2400, 1080)[0] == 0 && std::fabs(uvFond(2400, 1080)[1]) < 1e-6f, "20:9 : image entière, rien de coupé");
    cadrage(1920, 1080, "16:9 (1920 × 1080)");
    cadrage(1280, 720, "16:9 (1280 × 720)");
    cadrage(1600, 1200, "4:3 (1600 × 1200)");
    cadrage(3200, 1080, "très large (3200 × 1080)");
    const auto z = uvFond(0, 0);   // taille inconnue : image entière, jamais de division par zéro
    VERIFIE(z[0] == 0 && z[2] == 1 && std::fabs(z[1]) < 1e-6f && std::fabs(z[3] - 1) < 1e-6f, "taille nulle : image entière");

    std::string dossier = argc > 1 ? argv[1] : "";
    std::vector<unsigned char> d;
    for (const std::string& base : {dossier, std::string("assets/"), std::string("../assets/")}) {
        if (base.empty() && argc > 1) continue;
        std::ifstream f(base + FICHIER_FOND_CHEMIN, std::ios::binary);
        if (f) { d.assign(std::istreambuf_iterator<char>(f), {}); break; }
    }
    int w = 0, h = 0;
    VERIFIE(!d.empty(), "%s introuvable", FICHIER_FOND_CHEMIN);
    VERIFIE(d.empty() || tailleJpeg(d, w, h), "%s : pas un JPEG lisible", FICHIER_FOND_CHEMIN);
    VERIFIE(w == FOND_LARGEUR && h == FOND_HAUTEUR, "%s : %d × %d au lieu de %d × %d", FICHIER_FOND_CHEMIN, w, h, FOND_LARGEUR, FOND_HAUTEUR);
    VERIFIE(d.size() <= 600u * 1000u, "%s : %zu octets (600 Ko au plus)", FICHIER_FOND_CHEMIN, d.size());
    std::printf("%s : %d × %d, %zu octets\n", FICHIER_FOND_CHEMIN, w, h, d.size());
    std::printf(echecs ? "test_fond_chargement : %d échec(s)\n" : "test_fond_chargement : OK\n", echecs);
    return echecs ? 1 : 0;
}
