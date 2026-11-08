// WaForSpeed — WFS-273 (RG-01) : exemplaires du dessin instancié (Instance, renderer.h ; binding 1 de scene.vert et shadow.vert).
// Vérifié : les 3 lignes rangées redonnent la matrice monde (même reconstruction que scene.vert : mat4 en colonnes) ; le facteur
// de la normale est rangé en x.x ; l'exemplaire identité ne change ni un point ni une normale ; les deux vertex shaders lisent
// les emplacements 4 à 7 (sinon un exemplaire serait mal lu sans erreur visible au compilateur).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include "../src/render/renderer.h"

using namespace wfs;
static int g_controles = 0, g_echecs = 0;
#define VERIFIE(c, ...) do { g_controles++; if (!(c)) { g_echecs++; std::printf("[ÉCHEC] " __VA_ARGS__); std::printf("\n"); } else { std::printf("[OK] " __VA_ARGS__); std::printf("\n"); } } while (0)

// reconstruction de scene.vert : mat4(vec4(l0.x, l1.x, l2.x, 0), …, vec4(l0.w, l1.w, l2.w, 1)) appliquée à p (w = 1)
static vec3 appliquer(const Instance& i, vec3 p) {
    return {i.l0[0] * p.x + i.l0[1] * p.y + i.l0[2] * p.z + i.l0[3],
            i.l1[0] * p.x + i.l1[1] * p.y + i.l1[2] * p.z + i.l1[3],
            i.l2[0] * p.x + i.l2[1] * p.y + i.l2[2] * p.z + i.l2[3]};
}

static std::string lire(const char* chemin) {
    std::ifstream f(chemin);
    std::stringstream s; s << f.rdbuf();
    return s.str();
}

int main() {
    // arbre : échelle 1,7, tourné de 0,8 rad autour de y, posé en (12, 3, -40)
    const float s = 1.7f, a = 0.8f;
    mat4 m = mat4::identity();
    m.at(0, 0) = std::cos(a) * s; m.at(0, 2) = std::sin(a) * s;
    m.at(1, 1) = s;
    m.at(2, 0) = -std::sin(a) * s; m.at(2, 2) = std::cos(a) * s;
    m.at(0, 3) = 12; m.at(1, 3) = 3; m.at(2, 3) = -40;
    const Instance i = instance(m, 0.9f / s);
    float ecart = 0;
    for (vec3 p : {vec3{0, 0, 0}, vec3{1, 2, 3}, vec3{-4, 0.5f, 2}}) {
        const vec3 a1 = appliquer(i, p), a2 = m.transformPoint(p);
        ecart = std::max(ecart, length(a1 - a2));
    }
    VERIFIE(ecart < 1e-5f, "les 3 lignes redonnent la matrice monde (écart %.2g m)", ecart);
    VERIFIE(std::fabs(i.x[0] - 0.9f / s) < 1e-7f, "facteur de la normale rangé en x.x (%.4f)", i.x[0]);
    // normale : mat3(M) × n × facteur → longueur = teinte (0,9), l'échelle ne la change pas
    const vec3 n = normalize(vec3{0.3f, 0.8f, -0.5f});
    const vec3 nt = vec3{i.l0[0] * n.x + i.l0[1] * n.y + i.l0[2] * n.z, i.l1[0] * n.x + i.l1[1] * n.y + i.l1[2] * n.z,
                         i.l2[0] * n.x + i.l2[1] * n.y + i.l2[2] * n.z} * i.x[0];
    VERIFIE(std::fabs(length(nt) - 0.9f) < 1e-5f, "longueur de la normale = teinte malgré l'échelle (%.4f)", length(nt));
    const Instance id = instance(mat4::identity(), 1.f);
    const vec3 q{3, -2, 7};
    VERIFIE(length(appliquer(id, q) - q) < 1e-7f && id.x[0] == 1.f, "exemplaire identité : point et normale inchangés");
    VERIFIE(sizeof(Instance) == 64, "Instance : 64 octets (pas du binding 1)");
    for (const char* f : {WFS_SOURCE_DIR "/shaders/scene.vert", WFS_SOURCE_DIR "/shaders/shadow.vert"}) {
        const std::string t = lire(f);
        bool ok = !t.empty();
        for (int k = 4; k <= 7; k++) ok = ok && t.find("layout(location = " + std::to_string(k) + ") in vec4") != std::string::npos;
        VERIFIE(ok, "%s lit les emplacements 4 à 7 (exemplaire)", std::strrchr(f, '/') + 1);
    }
    std::printf("test_instances : %d contrôles, %d échec(s)\n", g_controles, g_echecs);
    return g_echecs ? 1 : 0;
}
