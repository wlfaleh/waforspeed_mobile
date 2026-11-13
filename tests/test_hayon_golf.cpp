// WFS-456 (RG-01, RG-02 ; ANO-259, ANO-258) : Golf sans emblème ni signature de marque, contrôlée sur assets/cars/golf.wfm.
//   1. RG-01 : plus aucun triangle de l'emblème rond du hayon (matériau 10, disque de 5,8 cm de rayon centré en x = 0,
//      y = 0,861 m, z > 2,0 m) : c'était le « disque noir plat » d'ANO-259 (texture repeinte en #101010 par WFS-192 / WFS-404) ;
//   2. RG-01 : la tôle peinte du hayon (matériau 4, peinture) couvre tout ce disque, vue de l'arrière : hayon continu, sans trou ;
//   3. RG-02 : aucun triangle des phares avant (matériau 10, boîte des deux phares) ne lit encore la bande rouge de la texture de
//      la lampe (u 0,955..0,996, v 0,17..0,55 : liseré rouge, ANO-258) ; les DEL (autres zones de la texture) restent lues.
// usage : test_hayon_golf [dossier des assets]
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

struct Maillage { uint32_t mat = 0; std::vector<float> v; std::vector<uint32_t> ix; };   // sommet : x y z nx ny nz u v

// lecture minimale d'un .wfm (format : src/render/model_loader.cpp) : seuls les maillages servent ici
static bool lire(const std::string& chemin, std::vector<Maillage>& ms) {
    std::ifstream in(chemin, std::ios::binary);
    const std::vector<char> b((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (b.size() < 44 || std::memcmp(b.data(), "WFM1", 4) != 0) return false;
    size_t o = 4;
    auto u32 = [&](uint32_t& x) { if (o + 4 > b.size()) return false; std::memcpy(&x, b.data() + o, 4); o += 4; return true; };
    uint32_t version, nT, nM, nMe;
    if (!u32(version) || !u32(nT) || !u32(nM) || !u32(nMe)) return false;
    o += 24;
    for (uint32_t i = 0; i < nT; i++) { uint32_t n; if (!u32(n)) return false; o += n; }
    o += 56ull * nM;
    for (uint32_t i = 0; i < nMe; i++) {
        Maillage m; uint32_t nv, ni;
        if (!u32(m.mat) || !u32(nv) || !u32(ni) || o + (size_t)nv * 32 + (size_t)ni * 4 > b.size()) return false;
        m.v.resize((size_t)nv * 8); std::memcpy(m.v.data(), b.data() + o, (size_t)nv * 32); o += (size_t)nv * 32;
        m.ix.resize(ni); std::memcpy(m.ix.data(), b.data() + o, (size_t)ni * 4); o += (size_t)ni * 4;
        ms.push_back(std::move(m));
    }
    return true;
}

int main(int argc, char** argv) {
    const std::string assets = argc > 1 ? argv[1] : "assets";
    std::vector<Maillage> ms;
    VERIFIE(lire(assets + "/cars/golf.wfm", ms), "golf.wfm illisible dans %s", assets.c_str());
    if (echecs) return 1;
    constexpr float CY = 0.861f, R = 0.058f;   // emblème du hayon (mesuré : x −0,054..0,054, y 0,807..0,915)
    int embleme = 0, phares = 0, pharesDel = 0, couverts = 0, points = 0;
    for (const Maillage& m : ms) {
        for (size_t t = 0; t + 2 < m.ix.size(); t += 3) {
            const uint32_t a = m.ix[t], b = m.ix[t + 1], c = m.ix[t + 2];
            if (a == b && b == c) continue;   // triangle vidé
            const float* P[3] = {&m.v[a * 8], &m.v[b * 8], &m.v[c * 8]};
            float g[5] = {};
            for (int k = 0; k < 3; k++) for (int q = 0; q < 3; q++) g[q] += P[k][q] / 3;
            for (int k = 0; k < 3; k++) { g[3] += P[k][6] / 3; g[4] += P[k][7] / 3; }
            if (m.mat == 10 && g[2] > 2.0f && std::hypot(g[0], g[1] - CY) < R) embleme++;
            if (m.mat == 10 && g[2] < -1.5f && std::fabs(g[0]) <= 0.86f && g[1] >= 0.685f && g[1] <= 0.76f) {
                if (g[3] >= 0.955f && g[3] <= 0.996f && g[4] >= 0.17f && g[4] <= 0.55f) phares++; else pharesDel++;
            }
        }
    }
    // hayon continu : chaque point d'une grille de 1 cm dans le disque est couvert par un triangle de peinture (vue de l'arrière)
    for (float y = CY - R; y <= CY + R; y += 0.01f)
        for (float x = -R; x <= R; x += 0.01f) {
            if (std::hypot(x, y - CY) > R) continue;
            points++;
            bool ok = false;
            for (const Maillage& m : ms) {
                if (m.mat != 4 || ok) continue;
                for (size_t t = 0; t + 2 < m.ix.size() && !ok; t += 3) {
                    const float* A = &m.v[m.ix[t] * 8]; const float* B = &m.v[m.ix[t + 1] * 8]; const float* C = &m.v[m.ix[t + 2] * 8];
                    if (A[2] < 2.0f) continue;
                    const float d = (B[1] - C[1]) * (A[0] - C[0]) + (C[0] - B[0]) * (A[1] - C[1]);
                    if (std::fabs(d) < 1e-12f) continue;
                    const float l0 = ((B[1] - C[1]) * (x - C[0]) + (C[0] - B[0]) * (y - C[1])) / d;
                    const float l1 = ((C[1] - A[1]) * (x - C[0]) + (A[0] - C[0]) * (y - C[1])) / d;
                    ok = l0 >= 0 && l1 >= 0 && l0 + l1 <= 1;
                }
            }
            couverts += ok;
        }
    std::printf("Golf : %d triangle(s) d'emblème au hayon, peinture sur %d / %d points du disque, phares : %d triangle(s) sur "
                "la bande rouge, %d sur le reste de la lampe\n", embleme, couverts, points, phares, pharesDel);
    VERIFIE(embleme == 0, "RG-01 : %d triangle(s) de l'emblème rond encore sur le hayon (ANO-259)", embleme);
    VERIFIE(points > 0 && couverts == points, "RG-01 : hayon troué : %d point(s) du disque sans peinture", points - couverts);
    VERIFIE(phares == 0, "RG-02 : %d triangle(s) des phares lisent encore le liseré rouge (ANO-258)", phares);
    VERIFIE(pharesDel > 800, "RG-02 : phares presque vides (%d triangles) : DEL et boîtier attendus", pharesDel);
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("test_hayon_golf : OK\n");
    return 0;
}
