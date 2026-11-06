// WFS-149 (RG-04) : tangentes des sommets (src/render/mesh.cpp) et lecture des .wfm versionnés.
//   1. cas exacts : plan, uv tournées, uv en miroir (signe), uv dégénérées (repère arbitraire), orthogonalité
//   2. section 'TAN1' d'un .wfm version 2 : relue à l'identique ; tronquée ou absente : refusée (le moteur recalcule)
//   3. M-02 : sur la Golf (assets/cars/golf.wfm, version 1 : tangentes calculées comme au chargement), normale
//      reconstruite par le repère tangent du sommet (tangente quantifiée sur 8 bits comme sur le GPU, bitangente = signe ×
//      N × T) à partir d'une carte de normales plate codée sur 8 bits (128, 128, 255) : écart angulaire moyen à la normale
//      du modèle source ≤ 1° (le repère tangent ne déforme pas la normale du modèle).
//      Information (hors critère) : normale inclinée de 30° décodée avec le repère du sommet contre le repère exact de
//      chaque triangle (celui des dérivées écran de scene.frag avant WFS-149) : écart dû au lissage par sommet, attendu.
// usage : test_tangentes [dossier des assets]
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "../src/render/mesh.h"

using namespace wfs;
static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static vec3 T3(const Vertex& v) { const vec4 t = tangentes::lire(v); return {t.x, t.y, t.z}; }
static Vertex S(float x, float y, float z, float u, float v) { return Vertex{x, y, z, 0, 1, 0, u, v}; }   // sol : normale +y
static double angleDeg(const vec3& a, const vec3& b) { return std::acos(std::fmax(-1.0, std::fmin(1.0, (double)dot(normalize(a), normalize(b))))) * 180.0 / M_PI; }

int main(int argc, char** argv) {
    const std::string assets = argc > 1 ? argv[1] : "assets";
    const uint32_t I[6] = {0, 1, 2, 0, 2, 3};
    {   // plan horizontal, u le long de +x, v le long de +z : T = +x, B = w · N × T = +z → N × T = (0,1,0)×(1,0,0) = (0,0,−1) → w = −1
        Vertex q[4] = {S(0, 0, 0, 0, 0), S(1, 0, 0, 1, 0), S(1, 0, 1, 1, 1), S(0, 0, 1, 0, 1)};
        VERIFIE(tangentes::calculer(q, 4, I, 6) == 0, "plan : sommets sans tangente");
        for (const Vertex& v : q) {
            VERIFIE(angleDeg(T3(v), {1, 0, 0}) < 0.5, "plan : tangente (%.2f %.2f %.2f) ≠ +x", T3(v).x, T3(v).y, T3(v).z);
            VERIFIE(v.tw == -127, "plan : signe %d (−127 attendu : v le long de +z = −(N × T))", v.tw);
        }
        // uv en miroir (u le long de −x) : tangente −x, signe opposé
        Vertex m[4] = {S(0, 0, 0, 1, 0), S(1, 0, 0, 0, 0), S(1, 0, 1, 0, 1), S(0, 0, 1, 1, 1)};
        tangentes::calculer(m, 4, I, 6);
        for (const Vertex& v : m) VERIFIE(angleDeg(T3(v), {-1, 0, 0}) < 0.5 && v.tw == 127, "miroir : tangente ou signe faux");
        // uv tournées de 90° (u le long de +z) : tangente +z
        Vertex r[4] = {S(0, 0, 0, 0, 0), S(1, 0, 0, 0, -1), S(1, 0, 1, 1, -1), S(0, 0, 1, 1, 0)};
        tangentes::calculer(r, 4, I, 6);
        for (const Vertex& v : r) VERIFIE(angleDeg(T3(v), {0, 0, 1}) < 0.5, "uv tournées : tangente (%.2f %.2f %.2f) ≠ +z", T3(v).x, T3(v).y, T3(v).z);
        // uv dégénérées (toutes nulles) : repère arbitraire, unitaire et perpendiculaire à la normale, signe +
        Vertex d[4] = {S(0, 0, 0, 0, 0), S(1, 0, 0, 0, 0), S(1, 0, 1, 0, 0), S(0, 0, 1, 0, 0)};
        VERIFIE(tangentes::calculer(d, 4, I, 6) == 4, "uv dégénérées : 4 sommets sans uv exploitable attendus");
        for (const Vertex& v : d) VERIFIE(std::fabs(length(T3(v)) - 1) < 0.02f && std::fabs(T3(v).y) < 0.02f && v.tw == 127, "uv dégénérées : repère invalide");
        // indices hors bornes : ignorés, sans plantage
        const uint32_t faux[3] = {0, 9, 2};
        tangentes::calculer(q, 4, faux, 3);
        // quantification : erreur ≤ 0,5° sur un vecteur quelconque
        const vec3 a = normalize(vec3{0.3f, -0.8f, 0.52f});
        Vertex z{}; z.tx = tangentes::versOctet(a.x); z.ty = tangentes::versOctet(a.y); z.tz = tangentes::versOctet(a.z);
        VERIFIE(angleDeg(T3(z), a) < 0.5, "quantification sur 8 bits : %.3f°", angleDeg(T3(z), a));
    }
    std::printf("tangentes : plan, miroir, rotation, uv dégénérées, quantification vérifiés\n");

    {   // section 'TAN1' (.wfm version 2) : 2 maillages
        std::vector<uint8_t> sec = {'T', 'A', 'N', '1'};
        for (int k = 0; k < 5; k++) for (int c = 0; c < 4; c++) sec.push_back((uint8_t)(int8_t)(k * 10 - 20 + c));
        std::vector<Vertex> v(5);
        VERIFIE(tangentes::lireSectionTangentes(sec.data(), sec.data() + sec.size(), {0, 2}, {2, 3}, v.data()), "section TAN1 refusée");
        VERIFIE(v[4].tx == 20 && v[4].tw == 23 && v[0].tx == -20, "section TAN1 mal relue");
        VERIFIE(!tangentes::lireSectionTangentes(sec.data(), sec.data() + sec.size() - 1, {0, 2}, {2, 3}, v.data()), "section TAN1 tronquée acceptée");
        sec[0] = 'X';
        VERIFIE(!tangentes::lireSectionTangentes(sec.data(), sec.data() + sec.size(), {0, 2}, {2, 3}, v.data()), "section sans 'TAN1' acceptée");
        VERIFIE(!tangentes::lireSectionTangentes(nullptr, nullptr, {}, {}, v.data()), "section absente acceptée");
        // sommets du fichier (32 octets) : copiés sans tangente
        const float f[8] = {1, 2, 3, 0, 1, 0, 0.5f, 0.25f};
        Vertex w{}; w.tw = 5;
        tangentes::copierSommetsWfm(reinterpret_cast<const uint8_t*>(f), 1, &w);
        VERIFIE(w.px == 1 && w.v == 0.25f && w.tw == 0, "sommet .wfm mal copié");
    }
    std::printf("tangentes : section TAN1 du .wfm version 2 vérifiée\n");

    // ---- M-02 : la Golf
    std::ifstream in(assets + "/cars/golf.wfm", std::ios::binary);
    const std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    VERIFIE(b.size() > 44 && std::memcmp(b.data(), "WFM1", 4) == 0, "golf.wfm illisible dans %s", assets.c_str());
    if (b.size() > 44) {
        size_t o = 4;
        auto u32 = [&]() { uint32_t x = 0; if (o + 4 <= b.size()) std::memcpy(&x, &b[o], 4); o += 4; return x; };
        const uint32_t version = u32(), nT = u32(), nM = u32(), nMe = u32(); o += 24;
        for (uint32_t t = 0; t < nT; t++) o += u32();
        o += (size_t)nM * 56;
        double somme = 0, aire = 0, pire = 0, sommeN = 0, pireN = 0; size_t nSommets = 0, nTri = 0, sans = 0, nN = 0;
        for (uint32_t m = 0; m < nMe && o < b.size(); m++) {
            u32(); const uint32_t nv = u32(), ni = u32();
            if (o + (size_t)nv * 32 + (size_t)ni * 4 > b.size()) { VERIFIE(false, "golf.wfm tronqué"); break; }
            std::vector<Vertex> v(nv); std::vector<uint32_t> ix(ni);
            tangentes::copierSommetsWfm(&b[o], nv, v.data()); o += (size_t)nv * 32;
            std::memcpy(ix.data(), &b[o], (size_t)ni * 4); o += (size_t)ni * 4;
            sans += tangentes::calculer(v.data(), nv, ix.data(), ni);
            nSommets += nv;
            for (const Vertex& q : v) {   // M-02 : carte plate (128, 128, 255) décodée comme scene.frag : texel × 2 − 1
                const vec3 N = normalize(vec3{q.nx, q.ny, q.nz});
                if (length(N) < 0.5f) continue;
                const vec3 Tv = normalize(T3(q) - N * dot(N, T3(q))), Bv = cross(N, Tv) * (q.tw < 0 ? -1.f : 1.f);
                const float c = 128.f / 255.f * 2 - 1;
                const vec3 t = normalize(vec3{c, c, 1.f});   // scene.frag : normalize(texel × 2 − 1)
                const double e = angleDeg(Tv * t.x + Bv * t.y + N * t.z, N);
                sommeN += e; pireN = std::fmax(pireN, e); nN++;
            }
            for (uint32_t t = 0; t + 2 < ni; t += 3) {
                const Vertex* c[3] = {&v[ix[t]], &v[ix[t + 1]], &v[ix[t + 2]]};
                const vec3 p0{c[0]->px, c[0]->py, c[0]->pz}, e1 = vec3{c[1]->px, c[1]->py, c[1]->pz} - p0, e2 = vec3{c[2]->px, c[2]->py, c[2]->pz} - p0;
                const double s1 = c[1]->u - c[0]->u, t1 = c[1]->v - c[0]->v, s2 = c[2]->u - c[0]->u, t2 = c[2]->v - c[0]->v, r = s1 * t2 - s2 * t1;
                const double a = length(cross(e1, e2)) * 0.5;
                if (std::fabs(r) < 1e-12 || a < 1e-12) continue;
                const vec3 sdir = (e1 * (float)t2 - e2 * (float)t1) * (float)(1 / r), tdir = (e2 * (float)s1 - e1 * (float)s2) * (float)(1 / r);
                nTri++;
                for (int k = 0; k < 3; k++) {
                    const vec3 N = normalize(vec3{c[k]->nx, c[k]->ny, c[k]->nz});
                    const vec3 Tf = normalize(sdir - N * dot(N, sdir)), Bf = normalize(tdir - N * dot(N, tdir));   // repère exact du triangle
                    const vec3 Tv = normalize(T3(*c[k]) - N * dot(N, T3(*c[k])));
                    const vec3 Bv = cross(N, Tv) * (c[k]->tw < 0 ? -1.f : 1.f);                                     // repère du sommet (GPU)
                    for (const auto& [x, y] : {std::pair{0.5f, 0.f}, std::pair{-0.5f, 0.f}, std::pair{0.f, 0.5f}, std::pair{0.f, -0.5f}}) {
                        const float z = std::sqrt(1 - x * x - y * y);   // 30° de la normale
                        const double e = angleDeg(Tf * x + Bf * y + N * z, Tv * x + Bv * y + N * z);
                        somme += e * a; aire += a; pire = std::fmax(pire, e);
                    }
                }
            }
        }
        const double moyen = aire > 0 ? somme / aire : 99, moyenN = nN ? sommeN / nN : 99;
        std::printf("M-02 Golf (version %u) : %zu sommets (%zu sans uv exploitable : repère arbitraire), normale reconstruite par les tangentes : "
                    "écart moyen %.3f°, pire %.3f° (seuil 1°)\n", version, nSommets, sans, moyenN, pireN);
        std::printf("information : normale inclinée de 30°, repère du sommet contre repère exact de chacun des %zu triangles à uv : écart moyen %.2f° "
                    "(pondéré par l'aire), pire %.1f° (lissage par sommet, coutures d'uv cisaillées)\n", nTri, moyen, pire);
        VERIFIE(nN > 1000 && moyenN <= 1.0, "M-02 : écart angulaire moyen %.3f° > 1° (%zu sommets)", moyenN, nN);
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("test_tangentes : OK\n");
    return 0;
}
