// WaForSpeed — WFS-380 : vraie eau, règles vérifiables par le calcul.
//   RG-01 : Méditerranée par beau temps : hauteur significative 0,2 à 0,6 m ; les vagues qui portent au moins 90 % de l'énergie ont une
//           longueur d'onde de 5 à 30 m ; les vagues principales (amplitude ≥ 5 cm) ont une période de 3 à 5 s (eau profonde, ω² = g k) ;
//           directions unitaires ; aucune vague ne déferle (cambrure k A < 0,1) ; mêmes valeurs dans shaders/eau.glsl (EAU_VAGUES) ;
//   RG-07 : en Basse, 4 vagues (les plus longues) ;
//   RG-09 : pierres du rivage : blocs de 80 triangles, galets de 20, pas de boule (au moins 3 faces de cassure planes), normales
//           unitaires, triangles tournés vers l'extérieur, aucun triangle dégénéré, même graine = même pierre.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include "../src/render/eau_vagues.h"
#include "../src/render/eau_rivage.h"

using namespace wfs;

static int echecs = 0;
static void verifie(bool ok, const char* quoi) {
    std::printf("%s %s\n", ok ? "[OK]   " : "[ÉCHEC]", quoi);
    if (!ok) echecs++;
}

int main(int argc, char** argv) {
#ifdef WFS_SOURCE_DIR
    const std::string racine = argc > 1 ? argv[1] : WFS_SOURCE_DIR;
#else
    const std::string racine = argc > 1 ? argv[1] : ".";
#endif
    // ---- RG-01 : vagues
    const float hs = eau::hauteurSignificative();
    std::printf("hauteur significative : %.3f m\n", hs);
    verifie(hs >= 0.2f && hs <= 0.6f, "RG-01 : hauteur significative de 0,2 à 0,6 m");
    float vTot = 0, v5a30 = 0;
    bool periodes = true, unitaires = true, cambrure = true, ordre = true;
    for (int k = 0; k < 8; k++) {
        const eau::Vague& w = eau::VAGUES[k];
        const float v = 0.5f * w.amplitude * w.amplitude;
        vTot += v;
        if (w.longueur >= 5 && w.longueur <= 30) v5a30 += v;
        const float T = eau::periode(w.longueur);
        std::printf("  vague %d : %.1f m, amplitude %.3f m, période %.2f s, cambrure %.3f\n", k, w.longueur, w.amplitude, T, 6.2831853f / w.longueur * w.amplitude);
        if (w.amplitude >= 0.05f && (T < 3 || T > 5)) periodes = false;
        if (std::fabs(w.dx * w.dx + w.dz * w.dz - 1) > 1e-3f) unitaires = false;
        if (6.2831853f / w.longueur * w.amplitude >= 0.1f) cambrure = false;
        if (k > 0 && w.longueur > eau::VAGUES[k - 1].longueur) ordre = false;
    }
    verifie(v5a30 >= 0.9f * vTot, "RG-01 : au moins 90 % de l'énergie dans les vagues de 5 à 30 m");
    verifie(periodes, "RG-01 : vagues principales (≥ 5 cm) de période 3 à 5 s");
    verifie(unitaires, "RG-01 : directions unitaires");
    verifie(cambrure, "RG-01 : aucune vague ne déferle au large (k A < 0,1)");
    verifie(ordre && eau::VAGUES_BASSE == 4, "RG-07 : vagues de la plus longue à la plus courte, 4 en Basse");
    {   // mêmes valeurs dans le shader
        std::ifstream f(racine + "/shaders/eau.glsl");
        std::stringstream ss; ss << f.rdbuf();
        const std::string g = ss.str();
        const std::regex re(R"(vec4\(\s*(-?[0-9.]+),\s*(-?[0-9.]+),\s*([0-9.]+),\s*([0-9.]+)\))");
        const size_t debut = g.find("EAU_VAGUES[8] = vec4[8](");
        int n = 0; bool egales = debut != std::string::npos;
        if (egales) {
            auto it = std::sregex_iterator(g.begin() + (long)debut, g.end(), re);
            for (; it != std::sregex_iterator() && n < 8; ++it, ++n) {
                const eau::Vague& w = eau::VAGUES[n];
                const float a = std::stof((*it)[1]), b = std::stof((*it)[2]), c = std::stof((*it)[3]), d = std::stof((*it)[4]);
                if (std::fabs(a - w.dx) > 1e-4f || std::fabs(b - w.dz) > 1e-4f || std::fabs(c - w.longueur) > 1e-4f || std::fabs(d - w.amplitude) > 1e-4f) egales = false;
            }
        }
        verifie(egales && n == 8, "RG-01 : shaders/eau.glsl a les mêmes 8 vagues que src/render/eau_vagues.h");
    }
    // ---- RG-09 : pierres du rivage
    for (int sub : {0, 1}) {
        const rivage::Maille a = rivage::rocher(3, sub), b = rivage::rocher(3, sub), c = rivage::rocher(4, sub);
        const size_t tri = a.i.size() / 3;
        char msg[160];
        std::snprintf(msg, sizeof msg, "RG-09 : %s de %zu triangles (attendu %d)", sub ? "bloc" : "galet", tri, sub ? 80 : 20);
        verifie(tri == (sub ? 80u : 20u), msg);
        bool memes = a.v.size() == b.v.size(), differente = a.v.size() != c.v.size();
        for (size_t k = 0; k < a.v.size() && memes; k++) memes = a.v[k].px == b.v[k].px && a.v[k].py == b.v[k].py && a.v[k].pz == b.v[k].pz;
        for (size_t k = 0; k < a.v.size() && !differente && k < c.v.size(); k++) differente = a.v[k].px != c.v[k].px;
        verifie(memes && differente, "RG-09 : même graine, même pierre ; autre graine, autre pierre");
        bool unit = true, dehors = true, degenere = false;
        int facesPlanes = 0;
        vec3 centre{};
        for (const Vertex& v : a.v) centre = centre + vec3{v.px, v.py, v.pz} * (1.f / (float)a.v.size());
        for (size_t k = 0; k < a.i.size(); k += 3) {
            const Vertex &A = a.v[a.i[k]], &B = a.v[a.i[k + 1]], &C = a.v[a.i[k + 2]];
            const vec3 pa{A.px, A.py, A.pz}, pb{B.px, B.py, B.pz}, pc{C.px, C.py, C.pz};
            const vec3 n = cross(pb - pa, pc - pa);
            if (length(n) < 1e-7f) { degenere = true; continue; }
            if (dot(n, (pa + pb + pc) * (1.f / 3) - centre) < 0) dehors = false;
        }
        for (const Vertex& v : a.v) if (std::fabs(std::sqrt(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz) - 1) > 1e-3f) unit = false;
        // faces de cassure : triangles voisins coplanaires (normales de face presque égales) ; une boule n'en a aucune
        for (size_t k = 0; k < a.i.size(); k += 3) {
            const Vertex &A = a.v[a.i[k]], &B = a.v[a.i[k + 1]], &C = a.v[a.i[k + 2]];
            const vec3 n1 = normalize(cross(vec3{B.px - A.px, B.py - A.py, B.pz - A.pz}, vec3{C.px - A.px, C.py - A.py, C.pz - A.pz}));
            for (size_t j = k + 3; j < a.i.size(); j += 3) {
                const Vertex &D = a.v[a.i[j]], &E = a.v[a.i[j + 1]], &F = a.v[a.i[j + 2]];
                const vec3 n2v = cross(vec3{E.px - D.px, E.py - D.py, E.pz - D.pz}, vec3{F.px - D.px, F.py - D.py, F.pz - D.pz});
                if (length(n2v) < 1e-7f) continue;
                if (dot(n1, normalize(n2v)) > 0.999f && std::fabs(dot(n1, vec3{D.px - A.px, D.py - A.py, D.pz - A.pz})) < 1e-3f) { facesPlanes++; break; }
            }
        }
        verifie(unit, "RG-09 : normales unitaires");
        verifie(dehors, "RG-09 : triangles tournés vers l'extérieur");
        if (sub == 0) verifie(!degenere, "RG-09 : aucun triangle dégénéré sur un galet");
        if (sub == 1) {   // bloc d'enrochement : calcaire éclaté
            std::snprintf(msg, sizeof msg, "RG-09 : bloc : %d triangles posés sur une face plane de cassure (pas une boule, ≥ 6)", facesPlanes);
            verifie(facesPlanes >= 6, msg);
        } else {          // galet : roulé par la mer, aplati (hauteur ≤ 0,75 × longueur), pas une sphère
            float mn[3] = {1e9f, 1e9f, 1e9f}, mx[3] = {-1e9f, -1e9f, -1e9f};
            for (const Vertex& v : a.v) { const float p[3] = {v.px, v.py, v.pz}; for (int q = 0; q < 3; q++) { mn[q] = std::fmin(mn[q], p[q]); mx[q] = std::fmax(mx[q], p[q]); } }
            const float L = std::fmax(mx[0] - mn[0], mx[2] - mn[2]), H = mx[1] - mn[1];
            std::snprintf(msg, sizeof msg, "RG-09 : galet aplati (hauteur / longueur = %.2f ≤ 0,75)", H / L);
            verifie(H <= 0.75f * L, msg);
        }
    }
    std::printf("%s\n", echecs ? "test_eau : ÉCHEC" : "test_eau : OK");
    return echecs ? 1 : 0;
}
