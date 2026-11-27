// WaForSpeed — WFS-613 (RG-05) : lumières de nuit limitées par tuile d'écran (src/render/tuiles_lumieres.h).
// Vérifie, sur des caméras et des lumières tirées au hasard :
//  - une lumière à portée d'un point vu n'est JAMAIS sautée : le bit de la lumière est posé dans la tuile du point (même image) ;
//  - une lumière loin de la tuile n'est pas comptée : au moins la moitié des couples (tuile, lumière) sont écartés dans une rue
//    de nuit typique (sinon le ticket ne sert à rien) ;
//  - portée infinie (0) et sphère autour de la caméra : toutes les tuiles ;
//  - le shader lit le même masque : constantes TX, TY et lecture des mots identiques dans eclairage.glsl et common.glsl.
#include "../src/render/tuiles_lumieres.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

using namespace wfs;
namespace tl = tuiles_lumieres;

namespace {
int echecs = 0;
void verifier(bool ok, const char* quoi) {
    if (!ok) { std::printf("ÉCHEC : %s\n", quoi); echecs++; }
}
uint32_t graine = 2026u;
float hasard() { graine = graine * 1664525u + 1013904223u; return (graine >> 8) / 16777216.f; }
float entre(float a, float b) { return a + (b - a) * hasard(); }

std::string lireFichier(const std::string& chemin) {
    std::ifstream f(chemin);
    std::stringstream s;
    s << f.rdbuf();
    return s.str();
}
}  // namespace

int main() {
    long verifies = 0, ecartes = 0, couples = 0;
    for (int essai = 0; essai < 300; essai++) {
        // caméra de course : hauteur 1,2 à 3 m, regard vers l'avant avec un peu de lacet et de tangage
        const vec3 eye{entre(-50, 50), entre(1.2f, 3.f), entre(-50, 50)};
        const float lacet = entre(0, 6.2832f), tangage = entre(-0.15f, 0.05f);
        const vec3 cible{eye.x + std::sin(lacet), eye.y + tangage, eye.z + std::cos(lacet)};
        const mat4 vp = perspectiveVk(entre(0.8f, 1.2f), 2400.f / 1080.f, 0.1f) * lookAt(eye, cible, {0, 1, 0});
        // 12 lumières : lampadaires (portée 25 à 40 m, à 5-6 m de haut) et phares (portée 60 à 90 m), autour de la caméra
        vec4 pos[tl::LUMIERES_MAX];
        const int n = 1 + (int)(hasard() * 11.99f);
        for (int i = 0; i < n; i++) {
            const bool phare = hasard() < 0.3f;
            pos[i] = {eye.x + entre(-150, 150), phare ? 0.7f : entre(5.f, 6.f), eye.z + entre(-150, 150), phare ? entre(60, 90) : entre(25, 40)};
        }
        if (essai % 50 == 0) pos[0].w = 0;   // portée infinie
        uint32_t mots[tl::MOTS];
        tl::calculer(vp, pos, n, mots);
        if (essai % 50 == 0) {
            bool toutes = true;
            for (int t = 0; t < tl::TX * tl::TY; t++) toutes = toutes && (tl::lire(mots, t) & 1u);
            verifier(toutes, "portée infinie : la lumière est gardée dans toutes les tuiles");
        }
        // points vus à l'écran, proches des lumières (le cas qui compte) : chaque lumière à portée doit avoir son bit
        for (int k = 0; k < 4000; k++) {
            const int i = (int)(hasard() * (n - 0.001f));
            const float r = pos[i].w > 0 ? pos[i].w : 50.f;
            const vec3 p{pos[i].x + entre(-r, r), entre(0.f, 8.f), pos[i].z + entre(-r, r)};
            const float cx = vp.at(0, 0) * p.x + vp.at(0, 1) * p.y + vp.at(0, 2) * p.z + vp.at(0, 3);
            const float cy = vp.at(1, 0) * p.x + vp.at(1, 1) * p.y + vp.at(1, 2) * p.z + vp.at(1, 3);
            const float cw = vp.at(3, 0) * p.x + vp.at(3, 1) * p.y + vp.at(3, 2) * p.z + vp.at(3, 3);
            if (!(cw > 1e-4f) || std::fabs(cx / cw) > 1.f || std::fabs(cy / cw) > 1.f) continue;   // hors de l'écran : toutes gardées
            const uint32_t m = tl::lire(mots, tl::tuile(cx / cw, cy / cw));
            for (int j = 0; j < n; j++) {
                const float dx = p.x - pos[j].x, dy = p.y - pos[j].y, dz = p.z - pos[j].z;
                if (pos[j].w > 0 && std::sqrt(dx * dx + dy * dy + dz * dz) >= pos[j].w) continue;   // hors de portée (même test que le shader)
                verifies++;
                if (!(m & (1u << j))) { std::printf("lumière %d à portée sautée (essai %d)\n", j, essai); verifier(false, "une lumière à portée n'est jamais sautée"); }
            }
        }
        for (int t = 0; t < tl::TX * tl::TY; t++) {
            const uint32_t m = tl::lire(mots, t);
            for (int j = 0; j < n; j++) { couples++; if (!(m & (1u << j))) ecartes++; }
        }
    }
    const double part = (double)ecartes / (double)couples;
    std::printf("%ld lumières à portée vérifiées ; %.0f %% des couples (tuile, lumière) écartés\n", verifies, part * 100.0);
    verifier(verifies > 100000, "assez de points vérifiés");
    verifier(part >= 0.5, "au moins la moitié des lumières écartées par tuile dans une rue de nuit");
    {   // sphère qui touche la caméra : toutes les tuiles
        const mat4 vp = perspectiveVk(1.f, 2.f, 0.1f) * lookAt({0, 2, 0}, {0, 2, 1}, {0, 1, 0});
        vec4 pos[1] = {{0, 5, -3, 30}};
        uint32_t mots[tl::MOTS];
        tl::calculer(vp, pos, 1, mots);
        bool toutes = true;
        for (int t = 0; t < tl::TX * tl::TY; t++) toutes = toutes && (tl::lire(mots, t) & 1u);
        verifier(toutes, "sphère autour de la caméra : toutes les tuiles");
    }
    const std::string ecl = lireFichier(std::string(WFS_SOURCE_DIR) + "/shaders/eclairage.glsl");
    const std::string com = lireFichier(std::string(WFS_SOURCE_DIR) + "/shaders/common.glsl");
    verifier(ecl.find("const int TUILES_X = " + std::to_string(tl::TX) + ", TUILES_Y = " + std::to_string(tl::TY) + ";") != std::string::npos, "eclairage.glsl : même grille que tuiles_lumieres.h");
    verifier(com.find("uvec4 tuilesL[" + std::to_string(tl::MOTS / 4) + "]") != std::string::npos, "common.glsl : masques des tuiles dans le bloc Frame");
    verifier(ecl.find("uint masqueLumieres(vec3 p)") != std::string::npos, "eclairage.glsl : masqueLumieres");
    if (echecs) { std::printf("test_tuiles_lumieres : %d échec(s)\n", echecs); return 1; }
    std::printf("test_tuiles_lumieres : OK\n");
    return 0;
}
