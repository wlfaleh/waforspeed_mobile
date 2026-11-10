// WFS-413 (RG-01, RG-03, RG-04, ANO-240) : optiques avant mesurées sur les modèles.
//  1. Cas construit : une optique de forme connue de chaque côté → centre de la glace retrouvé à moins de 2 mm, normale vers
//     l'extérieur ; la découpe des pièces « phare » garde tous les triangles et ne prend que ceux de la lampe.
//  2. Les voitures du jeu (assets/cars/*.wfm, liste PHARES_AVANT de correctifs_voitures.h) : les deux optiques trouvées,
//     point de départ de la lumière (1 cm derrière le centre de la glace) à 2 cm au plus de la surface réelle de l'optique,
//     centre dans la lampe, gauche et droite symétriques, à l'avant de la voiture, à une hauteur de phare ; l'ancien point
//     (3/4 de la demi-largeur, pare-chocs) est noté pour comparaison. Aucune voiture permise sans optique reconnue (RG-04).
// usage : test_phares_optiques [racine du dépôt]
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>
#include "../src/render/correctifs_voitures.h"
#include "../src/game/cars.h"
using namespace wfs;
using namespace wfs::feux_modele;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

struct Piece { uint32_t firstIndex, indexCount; int32_t vertexOffset; uint32_t material; int32_t roue = -1; int8_t feuArriere = 0; };
struct Modele { std::vector<vec3> v; std::vector<uint32_t> idx; std::vector<Piece> parts; uint32_t nMat = 0; };

// lecture bornée d'un .wfm (format de tools/convert_car.mjs : sommets x y z nx ny nz u v, avant vers −z)
static bool lire(const std::string& chemin, Modele& m) {
    std::ifstream f(chemin, std::ios::binary);
    if (!f) return false;
    const std::string b((std::istreambuf_iterator<char>(f)), {});
    size_t o = 4;
    auto u32 = [&](uint32_t& x) { if (o + 4 > b.size()) return false; std::memcpy(&x, &b[o], 4); o += 4; return true; };
    uint32_t version, nT, nMe;
    if (b.size() < 44 || std::memcmp(b.data(), "WFM1", 4) || !u32(version) || !u32(nT) || !u32(m.nMat) || !u32(nMe)) return false;
    o += 24;
    for (uint32_t t = 0; t < nT; t++) { uint32_t n; if (!u32(n)) return false; o += n; }
    o += (size_t)56 * m.nMat;
    for (uint32_t k = 0; k < nMe; k++) {
        uint32_t mi, nv, ni;
        if (!u32(mi) || !u32(nv) || !u32(ni) || o + (size_t)nv * 32 + (size_t)ni * 4 > b.size()) return false;
        const int32_t v0 = (int32_t)m.v.size();
        for (uint32_t i = 0; i < nv; i++) { float p[3]; std::memcpy(p, &b[o + (size_t)i * 32], 12); m.v.push_back({p[0], p[1], p[2]}); }
        o += (size_t)nv * 32;
        m.parts.push_back({(uint32_t)m.idx.size(), ni, v0, mi});
        for (uint32_t i = 0; i < ni; i++) { uint32_t x; std::memcpy(&x, &b[o + (size_t)i * 4], 4); if (x >= nv) return false; m.idx.push_back(x); }
        o += (size_t)ni * 4;
    }
    return true;
}

// distance d'un point à un triangle (projection sur le plan, sinon sur les arêtes)
static float distTri(const vec3& p, const vec3& a, const vec3& b, const vec3& c) {
    auto seg = [](const vec3& p, const vec3& a, const vec3& b) {
        const vec3 ab = b - a; const float t = std::clamp(dot(p - a, ab) / std::max(dot(ab, ab), 1e-12f), 0.f, 1.f);
        return length(p - (a + ab * t));
    };
    const vec3 n = cross(b - a, c - a); const float nl = length(n);
    if (nl > 1e-9f) {
        const vec3 u = n * (1.f / nl); const float d = dot(p - a, u); const vec3 q = p - u * d;
        const bool in = dot(cross(b - a, q - a), n) >= 0 && dot(cross(c - b, q - b), n) >= 0 && dot(cross(a - c, q - c), n) >= 0;
        if (in) return std::fabs(d);
    }
    return std::min({seg(p, a, b), seg(p, b, c), seg(p, c, a)});
}

static void casConstruit() {
    // une lampe (matériau 1) : carré de 20 × 10 cm à z = −2,00, centré en (±0,65 ; 0,70) ; une glace (matériau 2) : carré
    // de 30 × 14 cm à z = −2,05, centré en (±0,66 ; 0,70) ; un autre carré du matériau 1 hors de la zone (grille, x = 0)
    Modele m; m.nMat = 3;
    auto carre = [&](uint32_t mat, float cx, float cy, float z, float hx, float hy) {
        const int32_t v0 = (int32_t)m.v.size();
        m.v.insert(m.v.end(), {{cx - hx, cy - hy, z}, {cx + hx, cy - hy, z}, {cx + hx, cy + hy, z}, {cx - hx, cy + hy, z}});
        m.parts.push_back({(uint32_t)m.idx.size(), 6, v0, mat});
        m.idx.insert(m.idx.end(), {0, 1, 2, 0, 2, 3});
    };
    for (float s : {-1.f, 1.f}) { carre(1, s * 0.65f, 0.70f, -2.00f, 0.10f, 0.05f); carre(2, s * 0.66f, 0.70f, -2.05f, 0.15f, 0.07f); }
    carre(1, 0.f, 0.60f, -2.10f, 0.25f, 0.05f);
    const std::vector<ZoneOptique> z = {{1, false, 0.40f, 0.90f, 0.55f, 0.85f, -1.5f}, {2, true, 0.40f, 0.90f, 0.55f, 0.85f, -1.5f}};
    Optique o[2];
    auto pos = [&](size_t k) { return m.v[k]; };
    mesurerOptiques(m.idx, m.parts, pos, z, o);
    for (int s = 0; s < 2; s++) {
        const float sx = s ? 1.f : -1.f;
        VERIFIE(o[s].trouvee, "cas construit : optique %d non trouvée", s);
        VERIFIE(length(o[s].centre - vec3{sx * 0.66f, 0.70f, -2.05f}) < 0.002f, "cas construit : centre (%.3f, %.3f, %.3f)", o[s].centre.x, o[s].centre.y, o[s].centre.z);
        VERIFIE(o[s].normale.z < -0.99f, "cas construit : normale vers l'avant (%.2f)", o[s].normale.z);
        VERIFIE(std::fabs(o[s].largeur - 0.20f) < 1e-3f && std::fabs(o[s].hauteur - 0.10f) < 1e-3f, "cas construit : lampe %.3f × %.3f", o[s].largeur, o[s].hauteur);
    }
    const size_t avant = m.idx.size();
    const uint32_t n = marquerPhares(m.idx, m.parts, pos, z);
    uint32_t total = 0, phares = 0;
    for (const Piece& p : m.parts) { total += p.indexCount; if (p.feuArriere == PIECE_PHARE) phares += p.indexCount; }
    VERIFIE(n == 4 && phares == 12, "cas construit : %u triangles de lampe marqués (4 attendus), %u indices", n, phares);
    VERIFIE(total == avant, "cas construit : %u indices après découpe (%zu avant)", total, avant);
    for (const Piece& p : m.parts) VERIFIE(!(p.feuArriere == PIECE_PHARE && p.material != 1), "cas construit : glace marquée comme lampe");
    VERIFIE(!pieceFeuArriere(PIECE_PHARE) && pieceFeuArriere(PIECE_FEU_ARRIERE) && pieceFeuArriere(PIECE_TROISIEME_FEU), "pièce phare prise pour un feu arrière");
}

int main(int argc, char** argv) {
    const std::string racine = argc > 1 ? argv[1] : WFS_SOURCE_DIR;
    casConstruit();
    // RG-04 : chaque voiture permise a ses optiques repérées. Voitures retirées (licence NC, WFS-404) : A 45, Elantra N, X5.
    const std::set<std::string> retirees = {"a45", "elantra", "x5"};
    std::set<std::string> ids;
    for (const correctifs::PhareVoiture& p : correctifs::PHARES_AVANT) ids.insert(p.voiture);
    int permises = 0;
    for (int i = 0; i < CAR_COUNT; i++) {
        if (retirees.count(CARS[i].id)) continue;
        permises++;
        VERIFIE(ids.count(CARS[i].id), "%s : optiques avant non repérées (PHARES_AVANT)", CARS[i].id);
    }
    VERIFIE(permises == 17, "%d voitures permises (17 attendues)", permises);
    float pireEcart = 0, pireAncien = 1e9f;
    for (const std::string& id : ids) {
        Modele m;
        if (!lire(racine + "/assets/cars/" + id + ".wfm", m)) { VERIFIE(false, "%s.wfm illisible", id.c_str()); continue; }
        const std::vector<ZoneOptique> z = correctifs::zonesPhares(id, m.nMat);
        VERIFIE(!z.empty(), "%s : %u matériaux, autre nombre attendu (modèle changé ?)", id.c_str(), m.nMat);
        if (z.empty()) continue;
        Optique o[2];
        auto pos = [&](size_t k) { return m.v[k]; };
        mesurerOptiques(m.idx, m.parts, pos, z, o);
        vec3 mn{1e9f, 1e9f, 1e9f}, mx{-1e9f, -1e9f, -1e9f};
        for (const vec3& p : m.v) { mn = {std::min(mn.x, p.x), std::min(mn.y, p.y), std::min(mn.z, p.z)}; mx = {std::max(mx.x, p.x), std::max(mx.y, p.y), std::max(mx.z, p.z)}; }
        const float L = mx.z - mn.z;
        for (int s = 0; s < 2; s++) {
            const char* cote = s ? "droite" : "gauche";
            VERIFIE(o[s].trouvee && o[s].triangles >= 20, "%s %s : optique non trouvée (%u triangles)", id.c_str(), cote, o[s].triangles);
            if (!o[s].trouvee) continue;
            // RG-01 : point de départ de la lumière (nuit.cpp) à ≤ 2 cm de la surface de l'optique (glace ou lampe)
            const vec3 src = o[s].centre - o[s].normale * DERRIERE_GLACE;
            float d = 1e9f;
            for (const Piece& p : m.parts)
                for (const ZoneOptique& zz : z) {
                    if (p.material != zz.materiau) continue;
                    for (uint32_t t = 0; t + 2 < p.indexCount; t += 3) {
                        const vec3 a = m.v[p.vertexOffset + m.idx[p.firstIndex + t]], b = m.v[p.vertexOffset + m.idx[p.firstIndex + t + 1]], c = m.v[p.vertexOffset + m.idx[p.firstIndex + t + 2]];
                        if (dansZone(zz, (a + b + c) * (1.f / 3), s ? 1 : -1)) d = std::min(d, distTri(src, a, b, c));
                    }
                }
            pireEcart = std::max(pireEcart, d);
            VERIFIE(d <= 0.02f, "%s %s : lumière à %.1f cm de l'optique (≤ 2 cm)", id.c_str(), cote, d * 100);
            VERIFIE(o[s].centre.x * (s ? 1 : -1) > 0.3f, "%s %s : centre du mauvais côté (x %.3f)", id.c_str(), cote, o[s].centre.x);
            VERIFIE(o[s].centre.y > 0.45f && o[s].centre.y < 1.10f, "%s %s : hauteur de phare %.2f m", id.c_str(), cote, o[s].centre.y);
            VERIFIE(o[s].centre.z < mn.z + 0.15f * L, "%s %s : optique pas à l'avant (z %.2f, avant %.2f)", id.c_str(), cote, o[s].centre.z, mn.z);
            VERIFIE(o[s].normale.z < -0.3f, "%s %s : glace tournée vers l'arrière (n.z %.2f)", id.c_str(), cote, o[s].normale.z);
            VERIFIE(o[s].largeur > 0.08f && o[s].largeur < 0.7f && o[s].hauteur > 0.03f && o[s].hauteur < 0.45f, "%s %s : lampe %.2f × %.2f m", id.c_str(), cote, o[s].largeur, o[s].hauteur);
        }
        if (!o[0].trouvee || !o[1].trouvee) continue;
        VERIFIE(std::fabs(o[0].centre.x + o[1].centre.x) < 0.03f && std::fabs(o[0].centre.y - o[1].centre.y) < 0.02f && std::fabs(o[0].centre.z - o[1].centre.z) < 0.03f,
                "%s : optiques gauche et droite non symétriques (%.3f / %.3f)", id.c_str(), o[0].centre.x, o[1].centre.x);
        // ancien point (avant WFS-413) : 3/4 de la demi-largeur des pièces de phare, au pare-chocs : écart noté
        const float ancien = length(vec3{std::fabs(o[1].centre.x) * 0.75f, o[1].centre.y, mn.z} - o[1].centre);
        pireAncien = std::min(pireAncien, ancien);
        std::printf("%-8s gauche (%.3f, %.3f, %.3f)  droite (%.3f, %.3f, %.3f)  lampe %.2f × %.2f m  écart à la surface ≤ %.1f cm  (point d'avant : ≥ %.0f cm)\n",
                    id.c_str(), o[0].centre.x, o[0].centre.y, o[0].centre.z, o[1].centre.x, o[1].centre.y, o[1].centre.z, o[1].largeur, o[1].hauteur, pireEcart * 100, ancien * 100);
    }
    std::printf("%zu voitures : pire écart lumière / optique %.1f cm (≤ 2 cm) ; l'ancien point était à %.0f cm au moins\n", ids.size(), pireEcart * 100, pireAncien * 100);
    if (echecs) { std::printf("test_phares_optiques : %d échec(s)\n", echecs); return 1; }
    std::printf("test_phares_optiques : OK\n");
    return 0;
}
