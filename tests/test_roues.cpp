// Roues animées (WFS-60) : chaque voiture .wfm donne 4 roues (rayon 0,25 à 0,45 m, centres symétriques gauche /
// droite), roulage = distance / rayon, braquage = angle × 1,6 borné à ±0,6 rad, Ackermann 1,12 / 0,9 (animWheels).
// usage : test_roues <dossier assets>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include "../src/game/roues.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// lecture minimale d'un .wfm (format de tools/convert_car.mjs) : maillages et drapeaux des matériaux
struct Wfm { std::vector<std::vector<float>> v; std::vector<std::vector<uint32_t>> i; std::vector<uint32_t> mat, flags; };
static bool lire(const std::string& chemin, Wfm& w) {
    std::ifstream f(chemin, std::ios::binary); if (!f) return false;
    std::vector<char> b((std::istreambuf_iterator<char>(f)), {});
    size_t o = 0;
    auto u32 = [&]() { uint32_t x; std::memcpy(&x, &b[o], 4); o += 4; return x; };
    if (b.size() < 44 || std::memcmp(b.data(), "WFM1", 4) != 0) return false;
    o = 4; u32(); const uint32_t nT = u32(), nM = u32(), nMe = u32(); o += 24;
    for (uint32_t t = 0; t < nT; t++) o += u32();
    for (uint32_t m = 0; m < nM; m++) { o += 52; w.flags.push_back(u32()); }
    for (uint32_t m = 0; m < nMe; m++) {
        w.mat.push_back(u32()); const uint32_t nv = u32(), ni = u32();
        w.v.emplace_back(nv * 8); std::memcpy(w.v.back().data(), &b[o], nv * 32); o += nv * 32;
        w.i.emplace_back(ni); std::memcpy(w.i.back().data(), &b[o], ni * 4); o += ni * 4;
    }
    // WFS-149 : .wfm version 2 = version 1 + section des tangentes 'TAN1' à la fin (src/render/mesh.h)
    return o == b.size() || (o + 4 <= b.size() && std::memcmp(&b[o], "TAN1", 4) == 0);
}

int main(int argc, char** argv) {
    const std::string dossier = argc > 1 ? argv[1] : "assets";
    for (const char* id : {"golf", "m4", "p911", "gallardo", "p208", "abarth", "tcross", "compass", "s3", "x5", "giulia", "a45",   // WFS-77 : les 20
                           "model3", "corsa", "gt86", "elantra", "v60", "rrsport", "mustang", "sf90"}) {
        Wfm w;
        if (!lire(dossier + "/cars/" + id + ".wfm", w)) { VERIFIE(false, "%s.wfm illisible", id); continue; }
        std::vector<roues::Maillage> ms;
        for (size_t k = 0; k < w.v.size(); k++)
            ms.push_back({w.v[k].data(), 8, (uint32_t)(w.v[k].size() / 8), w.i[k].data(), (uint32_t)w.i[k].size(), (w.flags[w.mat[k]] & 128) != 0});
        const roues::Decoupe d = roues::decouper(ms);
        VERIFIE(d.n == 4, "%s : %d roue(s)", id, d.n);
        if (d.n != 4) continue;
        size_t tri = 0, total = 0;
        for (size_t k = 0; k < ms.size(); k++) for (int8_t r : d.roueDuTriangle[k]) { tri += r >= 0; total++; }
        for (int k = 0; k < 4; k++) VERIFIE(d.roues[k].rayon >= 0.25f && d.roues[k].rayon <= 0.45f, "%s : rayon de la roue %d = %.3f m", id, k, d.roues[k].rayon);
        // symétrie gauche / droite (LF ↔ RF, LB ↔ RB) et avant devant (−Z) ; en z, 8 cm de tolérance : la 911 d'origine a
        // ses roues avant braquées (centre avant droit 7,6 cm derrière le gauche dans le modèle source)
        for (int a : {0, 2}) {
            const RoueModele &g = d.roues[a], &dr = d.roues[a + 1];
            VERIFIE(std::abs(g.centre.x + dr.centre.x) < 0.05f && std::abs(g.centre.z - dr.centre.z) < 0.08f && std::abs(g.centre.y - dr.centre.y) < 0.03f,
                    "%s : roues %d / %d non symétriques (x %.3f / %.3f, z %.3f / %.3f)", id, a, a + 1, g.centre.x, dr.centre.x, g.centre.z, dr.centre.z);
            VERIFIE(g.centre.x < 0 && dr.centre.x > 0, "%s : côtés des roues %d / %d", id, a, a + 1);
        }
        VERIFIE(d.roues[0].centre.z < 0 && d.roues[2].centre.z > 0 && d.roues[0].avant && !d.roues[2].avant, "%s : avant / arrière", id);
        // WFS-479 (RG-02, RG-05) : M4 (seule voiture vérifiée, RG-04) : chaque roue tourne autour de son moyeu. Axe de la roue
        // (normale de sa pièce la plus plate : voile de jante, disque) parallèle à l'axe X, lacet < 0,5° (les roues avant étaient
        // modélisées braquées de 20°, d'où le battement) ; centre de rotation à moins de 2 mm du centre du cercle de la bande
        // de roulement (sommets à plus de 0,9 R)
        if (std::strcmp(id, "m4") == 0 || std::strcmp(id, "p911") == 0) for (int k = 0; k < 4; k++) {
            const RoueModele& r = d.roues[k];
            auto resoudre = [](double M[3][4]) {   // Gauss-Jordan 3 × 3
                for (int i = 0; i < 3; i++) {
                    int pv = i; for (int j = i + 1; j < 3; j++) if (std::abs(M[j][i]) > std::abs(M[pv][i])) pv = j;
                    for (int j = 0; j < 4; j++) std::swap(M[i][j], M[pv][j]);
                    for (int j = 0; j < 3; j++) if (j != i) { const double f = M[j][i] / M[i][i]; for (int q = 0; q < 4; q++) M[j][q] -= f * M[i][q]; }
                }
                return std::array<double, 3>{M[0][3] / M[0][0], M[1][3] / M[1][1], M[2][3] / M[2][2]};
            };
            double A[3][4] = {}; int nb = 0;
            double lacet = 90, residuMin = 1e9;
            for (size_t mi = 0; mi < ms.size(); mi++) {
                std::vector<vec3> pts;
                for (uint32_t t = 0; t < ms[mi].nIdx / 3; t++) if (d.roueDuTriangle[mi][t] == k)
                    for (int j = 0; j < 3; j++) {
                        const float* p = ms[mi].sommets + (size_t)ms[mi].idx[t * 3 + j] * 8;
                        pts.push_back({p[0], p[1], p[2]});
                        if (std::hypot(p[1] - r.centre.y, p[2] - r.centre.z) > 0.9f * r.rayon) {   // cercle (Kåsa) : y² + z² = 2a y + 2b z + c
                            const double row[3] = {2.0 * p[1], 2.0 * p[2], 1}, rhs = (double)p[1] * p[1] + (double)p[2] * p[2];
                            for (int i = 0; i < 3; i++) { for (int j2 = 0; j2 < 3; j2++) A[i][j2] += row[i] * row[j2]; A[i][3] += row[i] * rhs; }
                            nb++;
                        }
                    }
                if (pts.size() < 300) continue;
                double B[3][4] = {};   // plan x = α + β y + γ z
                for (const vec3& q : pts) { const double rp[3] = {1, q.y, q.z}; for (int i = 0; i < 3; i++) { for (int j2 = 0; j2 < 3; j2++) B[i][j2] += rp[i] * rp[j2]; B[i][3] += rp[i] * q.x; } }
                const auto c = resoudre(B);
                double res = 0; for (const vec3& q : pts) res += std::pow(q.x - (c[0] + c[1] * q.y + c[2] * q.z), 2);
                res = std::sqrt(res / (double)pts.size());
                if (res < residuMin) { residuMin = res; lacet = std::atan(std::abs(c[2])) * 180 / 3.14159265358979; }
            }
            const auto h = resoudre(A);
            const double ecart = std::hypot(h[0] - r.centre.y, h[1] - r.centre.z) * 1000;
            VERIFIE(nb >= 24 && ecart < 2.0 && lacet < 0.5, "%s : roue %d : pivot à %.2f mm du moyeu, axe à %.2f° (RG-05 : < 2 mm, < 0,5°)", id, k, ecart, lacet);
            std::printf("%s : roue %d : pivot à %.2f mm du moyeu, axe à %.2f° de l'axe X (WFS-479)\n", id, k, ecart, lacet);
        }
        std::printf("%s : 4 roues, rayons %.3f %.3f %.3f %.3f m, centres avant z %.2f, arrière z %.2f, %zu triangles sur %zu tournent : OK\n",
                    id, d.roues[0].rayon, d.roues[1].rayon, d.roues[2].rayon, d.roues[3].rayon, d.roues[0].centre.z, d.roues[2].centre.z, tri, total);
    }

    // roulage = distance / rayon (sens : le haut de la roue avance vers −Z), modulo un tour
    const float rr = 0.33f;
    const RouesPose p = roues::poseJoueur(0, 0, 0.5 / 0.006, rr);   // 0,5 m parcourus
    VERIFIE(std::abs(p.roulage + 0.5f / rr) < 1e-4f, "roulage %.4f au lieu de %.4f", p.roulage, -0.5f / rr);
    const RoueModele roue{{0.8f, rr, -1.3f}, rr, true};
    const vec3 haut = roues::matrice(roue, p).transformPoint({0.8f, 2 * rr, -1.3f});
    VERIFIE(haut.z < -1.3f, "le haut de la roue doit avancer (z %.3f)", haut.z);
    // braquage : wa = steer × 0,6 / (1 + v/14), × 1,6, borné à ±0,6
    const RouesPose b0 = roues::poseJoueur(0.5, 0, 0, rr);
    VERIFIE(std::abs(b0.braquage - 0.48f) < 1e-5f, "braquage à l'arrêt %.4f au lieu de 0,48", b0.braquage);
    VERIFIE(std::abs(roues::poseJoueur(1, 0, 0, rr).braquage - 0.6f) < 1e-6f && std::abs(roues::poseJoueur(-1, 0, 0, rr).braquage + 0.6f) < 1e-6f, "borne ±0,6");
    const double v = 28 / 0.006;   // 28 m/s
    VERIFIE(std::abs(roues::poseJoueur(0.5, v, 0, rr).braquage - (float)(0.5 * 0.6 / 3 * 1.6)) < 1e-5f, "braquage réduit à haute vitesse");
    // Ackermann : à droite, la roue avant droite (intérieure) braque × 1,12, la gauche × 0,9 ; l'arrière ne braque pas
    const RoueModele avd{{0.8f, rr, -1.3f}, rr, true}, avg{{-0.8f, rr, -1.3f}, rr, true}, ard{{0.8f, rr, 1.3f}, rr, false};
    VERIFIE(roues::ackermann(avd, 0.3f) == 1.12f && roues::ackermann(avg, 0.3f) == 0.9f && roues::ackermann(ard, 0.3f) == 0.f, "Ackermann à droite");
    VERIFIE(roues::ackermann(avg, -0.3f) == 1.12f && roues::ackermann(avd, -0.3f) == 0.9f, "Ackermann à gauche");
    // braquage à droite : l'avant de la roue part vers +x
    const vec3 nez = roues::matrice(avd, {0, 0.3f}).transformPoint({0.8f, rr, -1.3f - rr});
    VERIFIE(nez.x > 0.8f + 0.05f, "roue braquée à droite (x %.3f)", nez.x);
    // pose nulle : matrice identité (voiture à l'arrêt inchangée)
    const vec3 q = roues::matrice(avd, {}).transformPoint({0.9f, 0.1f, -1.0f});
    VERIFIE(std::abs(q.x - 0.9f) < 1e-6f && std::abs(q.y - 0.1f) < 1e-6f && std::abs(q.z + 1.0f) < 1e-6f, "pose nulle ≠ identité");
    std::printf("roulage = distance / rayon, braquage × 1,6 borné à ±0,6, Ackermann 1,12 / 0,9 : OK\n");

    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("roues : tout est OK\n");
    return 0;
}
