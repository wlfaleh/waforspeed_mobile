// WFS-227 (RG-01 à RG-03 ; M-01, M-02) : ombres du soleil en cascades stabilisées, pénombre physique (src/render/eclairage_ombres.h),
// sans GPU :
//   - RG-01 : 4 cascades en Ultra, 3 en Haute, 2 en Moyenne ; fins croissantes ; ombres jusqu'à 200 m en Haute (RG-03) ;
//   - stabilisées : un déplacement de la caméra décale chaque cascade d'un nombre ENTIER de texels (pas de grésillement des
//     bords), et le demi-côté ne change ni avec la position ni avec le regard ;
//   - M-01 : ombre d'un poteau de 4 m : la cascade projette le sommet du poteau et le bout de son ombre au sol (L = h / tan(hauteur
//     du soleil)) sur le même texel, pour 3 hauteurs du soleil (20°, 45°, 70°) : longueur juste à ± 1 % ;
//   - M-02 : pénombre à 4 m de l'occultant ≈ 3,7 cm (± 10 %) ; constantes du shader (shaders/eclairage.glsl) = constantes C++.
// usage : test_fidelite_lumiere_ombres [dossier du dépôt]
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include "../src/render/eclairage_ombres.h"

#ifndef WFS_SOURCE_DIR
#define WFS_SOURCE_DIR "."
#endif

using namespace wfs;
static int echecs = 0;
static void cas(const std::string& quoi, bool ok) { std::printf("ombres : %-96s → %s\n", quoi.c_str(), ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }

static vec3 soleil(double hauteurDeg, double azimutDeg) {
    const double h = hauteurDeg * PI / 180, a = azimutDeg * PI / 180;
    return {(float)(std::cos(h) * std::sin(a)), (float)std::sin(h), (float)(-std::cos(h) * std::cos(a))};
}

int main(int argc, char** argv) {
    const std::string depot = argc > 1 ? argv[1] : WFS_SOURCE_DIR;
    const uint32_t COTE = 2048;
    // RG-01, RG-03 : nombre et portée
    {
        const ombres::Cascades u = ombres::calculer(soleil(45, 30), {0, 1.5f, 0}, {0, 0, -1}, 3, COTE), h = ombres::calculer(soleil(45, 30), {0, 1.5f, 0}, {0, 0, -1}, 2, COTE),
                               m = ombres::calculer(soleil(45, 30), {0, 1.5f, 0}, {0, 0, -1}, 1, COTE);
        cas("RG-01 : 4 cascades en Ultra, 3 en Haute, 2 en Moyenne", u.n == 4 && h.n == 3 && m.n == 2);
        bool croissant = true;
        for (const ombres::Cascades* c : {&u, &h, &m}) for (int i = 1; i < c->n; i++) croissant = croissant && c->fin[i] > c->fin[i - 1] && c->demi[i] >= c->demi[i - 1];
        cas("cascades de plus en plus loin et de plus en plus grandes (fondu entre deux)", croissant);
        char t[160]; std::snprintf(t, sizeof t, "RG-03 : ombres jusqu'à %.0f m en Haute (≥ 200), %.0f en Ultra, %.0f en Moyenne ; 1re cascade jusqu'à %.1f m",
                                   h.fin[h.n - 1], u.fin[u.n - 1], m.fin[m.n - 1], h.fin[0]);
        cas(t, h.fin[h.n - 1] >= 200 - 1e-3f && u.fin[u.n - 1] >= h.fin[h.n - 1] && h.fin[0] >= 10);
        std::printf("ombres : Haute : texel %.1f / %.1f / %.1f cm (cascades de %.0f, %.0f, %.0f m de côté)\n", 2 * h.demi[0] / (COTE / 2) * 100,
                    2 * h.demi[1] / (COTE / 2) * 100, 2 * h.demi[2] / (COTE / 2) * 100, 2 * h.demi[0], 2 * h.demi[1], 2 * h.demi[2]);
    }
    // stabilité : déplacement quelconque de la caméra → décalage entier de texels ; demi-côté indépendant du regard
    {
        const vec3 L = soleil(38, 120);
        bool entier = true, memeTaille = true;
        const ombres::Cascades a = ombres::calculer(L, {10, 1.5f, -3}, {0.2f, -0.1f, -1}, 2, COTE);
        for (float dx : {0.013f, 0.37f, 1.91f, 7.3f}) {
            const ombres::Cascades b = ombres::calculer(L, {10 + dx, 1.5f, -3 - dx * 0.6f}, {0.2f, -0.1f, -1}, 2, COTE);
            const ombres::Cascades c = ombres::calculer(L, {10, 1.5f, -3}, {std::sin(dx), -0.3f, -std::cos(dx)}, 2, COTE);
            for (int i = 0; i < a.n; i++) {
                const vec3 P{3, 0, -20};   // un point fixe du monde (le pied d'un arbre)
                const vec3 pa = a.m[i].transformPoint(P), pb = b.m[i].transformPoint(P);
                const float tu = (float)(COTE / 2);
                const float sx = (pb.x - pa.x) * tu / 2, sy = (pb.y - pa.y) * tu / 2;   // décalage en texels
                entier = entier && std::fabs(sx - std::round(sx)) < 2e-2f && std::fabs(sy - std::round(sy)) < 2e-2f;
                memeTaille = memeTaille && std::fabs(b.demi[i] - a.demi[i]) < 1e-5f && std::fabs(c.demi[i] - a.demi[i]) < 1e-5f;
            }
        }
        cas("stabilisées : la carte suit la caméra par texels entiers (aucun grésillement des bords)", entier);
        cas("stabilisées : demi-côté fixe (position, regard et vitesse sans effet)", memeTaille);
    }
    // M-01 : poteau de 4 m, 3 hauteurs du soleil
    for (double hDeg : {20.0, 45.0, 70.0}) {
        const vec3 L = soleil(hDeg, 65);
        const ombres::Cascades c = ombres::calculer(L, {0, 1.5f, 6}, {0, -0.2f, -1}, 2, COTE);
        const vec3 pied{0, 0, 0}, sommet{0, 4, 0};
        // bout de l'ombre : point du sol que le soleil voit derrière le sommet ; même texel dans la cascade (repère du soleil)
        const vec3 bout = sommet - L * (4.f / L.y);
        const vec3 a = c.m[0].transformPoint(sommet), b = c.m[0].transformPoint(bout);
        const double Ljeu = length(bout - pied), Lreel = 4.0 / std::tan(hDeg * PI / 180);
        char t[160]; std::snprintf(t, sizeof t, "M-01 : soleil à %.0f° : ombre de %.3f m (réel %.3f m ± 1 %%), sommet et bout de l'ombre sur le même texel", hDeg, Ljeu, Lreel);
        cas(t, std::fabs(Ljeu / Lreel - 1) <= 0.01 && std::fabs(a.x - b.x) * 512 < 0.05f && std::fabs(a.y - b.y) * 512 < 0.05f);
    }
    // M-02 et constantes du shader
    {
        const double p4 = ombres::penombre(4.0);
        char t[128]; std::snprintf(t, sizeof t, "M-02 : pénombre à 4 m de l'occultant : %.2f cm (réel ≈ 3,7 cm ± 10 %%)", p4 * 100);
        cas(t, std::fabs(p4 / 0.037 - 1) <= 0.10);
        cas("M-02 : la pénombre grandit avec la distance (8 m : 2 × 4 m)", std::fabs(ombres::penombre(8) / p4 - 2) < 1e-9);
        std::ifstream f(depot + "/shaders/eclairage.glsl");
        const std::string g((std::istreambuf_iterator<char>(f)), {});
        const size_t i = g.find("const float RECUL_SOLEIL_M = ");
        const double recul = i == std::string::npos ? -1 : std::atof(g.c_str() + i + std::strlen("const float RECUL_SOLEIL_M = "));
        cas("shader : RECUL_SOLEIL_M identique au C++ ; pénombre = distance × F.ombreP.w (tan 0,53°)",
            std::fabs(recul - ombres::RECUL_SOLEIL_M) < 1e-6 && g.find("dB * F.ombreP.w") != std::string::npos);
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("ombres : tout est OK\n");
    return 0;
}
