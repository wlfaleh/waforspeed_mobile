// WaForSpeed — WFS-612 : le halo de la passe finale calculé à demi-définition (passes 5 et 6 de post.frag) donne le même halo
// que l'ancien calcul de 16 lectures par pixel.
// Le test refait sur le processeur, avec un échantillonnage bilinéaire comme le GPU (centres des texels à i + 0,5, bords
// recopiés : CLAMP_TO_EDGE), l'ancien halo et le nouveau, sur des images HDR tirées au hasard avec des points très lumineux :
//  - pixels pairs (x et y pairs), loin des bords : même valeur au millième près (même calcul, réordonné) ;
//  - tous les pixels, comptés à l'écran (compression de nuit, ACES, sRGB) : écart moyen ≤ 2 niveaux sur 255 (RG-02), écart maximal
//    affiché. Pixels impairs : moyenne des deux pixels pairs voisins ; l'écart maximal est au bord du carré de 8 × 8 pixels du halo,
//    qui s'arrêtait net et descend maintenant sur un pixel (aucun motif nouveau) ;
//  - jour (poids égaux) et nuit (poids gaussiens) ;
//  - taille de l'image réduite = celle du niveau 0 du bloom (cw >> 1, ch >> 1), même pour une largeur impaire ;
//  - post.frag déclare bien l'image du halo (binding 2) et les passes 5 et 6.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {
int echecs = 0;
void verifier(bool ok, const char* quoi) {
    if (!ok) { std::printf("ÉCHEC : %s\n", quoi); echecs++; }
}

struct Img {
    int w = 0, h = 0;
    std::vector<float> p;   // RVB
    float* at(int x, int y) { return &p[3 * ((size_t)y * w + x)]; }
    const float* at(int x, int y) const { return &p[3 * ((size_t)y * w + x)]; }
};

// lecture bilinéaire en coordonnées de texels (centre du texel i à i + 0,5), bords recopiés
void lire(const Img& im, float u, float v, float out[3]) {
    float x = u - 0.5f, y = v - 0.5f;
    int x0 = (int)std::floor(x), y0 = (int)std::floor(y);
    float fx = x - x0, fy = y - y0;
    auto c = [&](int xi, int yi) { return im.at(std::clamp(xi, 0, im.w - 1), std::clamp(yi, 0, im.h - 1)); };
    for (int k = 0; k < 3; k++)
        out[k] = (c(x0, y0)[k] * (1 - fx) + c(x0 + 1, y0)[k] * fx) * (1 - fy) + (c(x0, y0 + 1)[k] * (1 - fx) + c(x0 + 1, y0 + 1)[k] * fx) * fy;
}

// fin de la passe finale (post.frag) : compression de nuit, ACES, code sRGB de l'écran (0..255)
float aces(float x) { return std::clamp((x * (2.51f * x + 0.03f)) / (x * (2.43f * x + 0.59f) + 0.14f), 0.f, 1.f); }
float code(float l) { l = std::clamp(l, 0.f, 1.f); return 255.f * (l < 0.0031308f ? l * 12.92f : 1.055f * std::pow(l, 1.f / 2.4f) - 0.055f); }
void ecran(const float scene[3], const float glow[3], bool nuit, float out[3]) {
    float e[3] = {glow[0] * 0.35f, glow[1] * 0.35f, glow[2] * 0.35f};
    if (nuit) { float y = 0.2126f * e[0] + 0.7152f * e[1] + 0.0722f * e[2]; for (float& v : e) v /= 1.f + y / 0.35f; }
    for (int k = 0; k < 3; k++) out[k] = code(aces(scene[k] + e[k]));
}

float poids(float o, bool nuit) { return nuit ? std::exp(-o * o / 6.f) : 1.f; }
float seuil(float s, float expo) { return std::max(std::min(s * expo, 24.f) - 2.5f, 0.f); }

// ancien halo (post.frag avant WFS-612) : 16 lectures autour du pixel (x, y), divisé par la somme des poids
void ancien(const Img& im, int x, int y, float expo, bool nuit, float out[3]) {
    float g[3] = {0, 0, 0}, somme = 0;
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) {
        float ox = i * 2.f - 3.5f, oy = j * 2.f - 3.5f, w = poids(ox, nuit) * poids(oy, nuit), s[3];
        somme += w;
        lire(im, x + 0.5f + ox, y + 0.5f + oy, s);
        for (int k = 0; k < 3; k++) g[k] += seuil(s[k], expo) * w;
    }
    for (int k = 0; k < 3; k++) out[k] = g[k] / somme;
}

// nouveau halo : passe 5 (horizontale, sur la scène), passe 6 (verticale), image réduite de moitié
Img passes(const Img& im, float expo, bool nuit) {
    Img X{std::max(1, im.w >> 1), std::max(1, im.h >> 1), {}}, Y;
    X.p.assign((size_t)X.w * X.h * 3, 0.f);
    Y = X;
    float somme = 0;
    for (int i = 0; i < 4; i++) somme += poids(i * 2.f - 3.5f, nuit);
    for (int n = 0; n < X.h; n++) for (int m = 0; m < X.w; m++) {
        float cx = (m + 0.5f) * 2.f, cy = (n + 0.5f) * 2.f;
        for (int i = 0; i < 4; i++) {
            float s[3];
            lire(im, cx + i * 2.f - 4.f, cy, s);
            for (int k = 0; k < 3; k++) X.at(m, n)[k] += seuil(s[k], expo) * poids(i * 2.f - 3.5f, nuit) / somme;
        }
    }
    for (int n = 0; n < X.h; n++) for (int m = 0; m < X.w; m++)
        for (int j = 0; j < 4; j++) {
            float s[3];
            lire(X, m + 0.5f, n + 0.5f + j - 2.f, s);
            for (int k = 0; k < 3; k++) Y.at(m, n)[k] += s[k] * poids(j * 2.f - 3.5f, nuit) / somme;
        }
    return Y;
}

void nouveau(const Img& halo, int x, int y, float out[3]) { lire(halo, (x + 0.5f + 0.5f) * 0.5f, (y + 0.5f + 0.5f) * 0.5f, out); }

uint32_t graine = 12345u;
float hasard() { graine = graine * 1664525u + 1013904223u; return (graine >> 8) / 16777216.f; }

Img image(int w, int h) {
    Img im{w, h, std::vector<float>((size_t)w * h * 3)};
    for (float& v : im.p) v = hasard() * hasard() * 1.2f;   // fond sous le seuil (rue, ciel)
    for (int n = 0; n < w * h / 1000; n++) {   // une source pour 1 000 pixels : plus que les images de nuit du banc (mesure de la RG-02 dans la PR)          // points et taches très lumineux (lampadaires, phares, reflets)
        int x = (int)(hasard() * w), y = (int)(hasard() * h), r = (int)(hasard() * 4.f);
        float e = 2.f + hasard() * 60.f, t[3] = {e, e * (0.6f + 0.4f * hasard()), e * (0.3f + 0.7f * hasard())};
        for (int dy = -r; dy <= r; dy++) for (int dx = -r; dx <= r; dx++) {
            int xx = std::clamp(x + dx, 0, w - 1), yy = std::clamp(y + dy, 0, h - 1);
            for (int k = 0; k < 3; k++) im.at(xx, yy)[k] = t[k];
        }
    }
    return im;
}

void essai(int w, int h, bool nuit) {
    const float expo = 1.f;
    Img im = image(w, h);
    Img halo = passes(im, expo, nuit);
    verifier(halo.w == (w >> 1) && halo.h == (h >> 1), "taille de l'image réduite = niveau 0 du bloom (cw >> 1, ch >> 1)");
    double ecartPair = 0, somme = 0, maxi = 0;
    long n = 0;
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        float a[3], b[3];
        ancien(im, x, y, expo, nuit, a);
        nouveau(halo, x, y, b);
        float ea[3], eb[3];   // pixel de l'écran (niveaux sur 255) avec l'ancien et le nouveau halo
        ecran(im.at(x, y), a, nuit, ea);
        ecran(im.at(x, y), b, nuit, eb);
        for (int k = 0; k < 3; k++) {
            double d = std::fabs(ea[k] - eb[k]);
            somme += d; n++; maxi = std::max(maxi, d);
            if (x % 2 == 0 && y % 2 == 0 && x >= 8 && y >= 8 && x < (w & ~1) - 8 && y < (h & ~1) - 8) ecartPair = std::max(ecartPair, d);
        }
    }
    const double moyen = somme / (double)n;
    std::printf("%dx%d %s : écart moyen %.3f niveau, maximal %.2f niveaux, pixels pairs %.5f niveau\n", w, h, nuit ? "nuit" : "jour", moyen, maxi, ecartPair);
    verifier(ecartPair < 1e-3, "pixels pairs : même halo qu'avant");
    verifier(moyen <= 2.0, "écart moyen ≤ 2 niveaux sur 255 (RG-02)");
}

std::string lireFichier(const std::string& chemin) {
    std::ifstream f(chemin);
    std::stringstream s;
    s << f.rdbuf();
    return s.str();
}
}  // namespace

int main() {
    for (bool nuit : {false, true}) {
        essai(160, 96, nuit);
        essai(161, 97, nuit);   // largeur et hauteur impaires
    }
    const std::string post = lireFichier(std::string(WFS_SOURCE_DIR) + "/shaders/post.frag");
    verifier(post.find("binding = 2) uniform sampler2D haloTex") != std::string::npos, "post.frag : image du halo (binding 2)");
    verifier(post.find("passeHalo") != std::string::npos && post.find("mode >= 5") != std::string::npos, "post.frag : passes 5 et 6 du halo");
    verifier(post.find("for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++)") == std::string::npos, "post.frag : plus de 16 lectures par pixel");
    if (echecs) { std::printf("test_halo_reduit : %d échec(s)\n", echecs); return 1; }
    std::printf("test_halo_reduit : OK\n");
    return 0;
}
