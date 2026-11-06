// WaForSpeed — WFS-170 : correctifs des modèles de voitures au chargement (voir correctifs_voitures.h).
#include "correctifs_voitures.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace wfs::correctifs {

std::string idDe(const std::string& chemin) {
    const size_t a = chemin.find_last_of('/'), b = chemin.rfind('.');
    const size_t d = a == std::string::npos ? 0 : a + 1;
    return chemin.substr(d, b == std::string::npos || b < d ? std::string::npos : b - d);
}

bool normalesARecalculer(const std::string& id) {
    // ANO-71, ANO-75, ANO-77 ; Giulia : la « bande gris clair » de la porte arrière (ANO-73) vient de normales d'origine
    // retournées sur le chant de la porte (essai : disparaît avec les normales recalculées)
    for (const char* v : {"model3", "x5", "p208", "compass", "giulia"}) if (id == v) return true;
    return false;
}

uint32_t recalculerNormales(Vertex* V, uint32_t nv, const uint32_t* I, uint32_t ni) {
    if (nv < 3 || ni < 3) return 0;
    // positions identiques (au 1/10 de mm) → même identifiant
    std::vector<uint32_t> ordre(nv), pid(nv);
    auto cle = [&](uint32_t v) { return std::array<int64_t, 3>{std::llround(V[v].px * 1e4), std::llround(V[v].py * 1e4), std::llround(V[v].pz * 1e4)}; };
    for (uint32_t v = 0; v < nv; v++) ordre[v] = v;
    std::sort(ordre.begin(), ordre.end(), [&](uint32_t a, uint32_t b) { return cle(a) < cle(b); });
    uint32_t np = 0;
    for (uint32_t k = 0; k < nv; k++) { if (k && cle(ordre[k]) != cle(ordre[k - 1])) np++; pid[ordre[k]] = np; }
    np++;
    // normales des faces (× aire), orientées comme la moyenne des normales d'origine de leurs sommets
    const uint32_t nf = ni / 3;
    std::vector<vec3> fn(nf);
    std::vector<uint32_t> debut(np + 1, 0), faces;
    auto valide = [&](uint32_t t) { return I[t * 3] < nv && I[t * 3 + 1] < nv && I[t * 3 + 2] < nv; };
    for (uint32_t t = 0; t < nf; t++) {
        if (!valide(t)) continue;
        const Vertex &a = V[I[t * 3]], &b = V[I[t * 3 + 1]], &c = V[I[t * 3 + 2]];
        const vec3 A{a.px, a.py, a.pz}, B{b.px, b.py, b.pz}, C{c.px, c.py, c.pz};
        vec3 f = cross(B - A, C - A);
        const vec3 n0{a.nx + b.nx + c.nx, a.ny + b.ny + c.ny, a.nz + b.nz + c.nz};
        if (dot(f, n0) < 0) f = -f;
        fn[t] = f;
        for (int k = 0; k < 3; k++) debut[pid[I[t * 3 + k]] + 1]++;
    }
    for (uint32_t q = 0; q < np; q++) debut[q + 1] += debut[q];
    faces.resize(debut[np]);
    std::vector<uint32_t> pos(debut.begin(), debut.end() - 1);
    // face de référence de chaque sommet : la plus grande des faces qui l'utilisent
    std::vector<int64_t> ref(nv, -1);
    for (uint32_t t = 0; t < nf; t++) {
        if (!valide(t)) continue;
        for (int k = 0; k < 3; k++) {
            faces[pos[pid[I[t * 3 + k]]]++] = t;
            int64_t& r = ref[I[t * 3 + k]];
            if (r < 0 || length(fn[t]) > length(fn[(size_t)r])) r = t;
        }
    }
    const float c40 = std::cos(40.f * PI / 180.f), c10 = std::cos(10.f * PI / 180.f);
    std::vector<vec3> nouv(nv);
    uint32_t changes = 0;
    for (uint32_t v = 0; v < nv; v++) {
        nouv[v] = {V[v].nx, V[v].ny, V[v].nz};
        if (ref[v] < 0) continue;
        const vec3 r = normalize(fn[(size_t)ref[v]]);
        vec3 s{};
        for (uint32_t k = debut[pid[v]]; k < debut[pid[v] + 1]; k++) {
            const vec3& f = fn[faces[k]];
            const float lf = length(f);
            if (lf > 1e-12f && dot(f, r) > c40 * lf) s = s + f;
        }
        if (length(s) < 1e-12f) continue;
        nouv[v] = normalize(s);
        const vec3 n0 = normalize(vec3{V[v].nx, V[v].ny, V[v].nz});
        if (dot(nouv[v], n0) < c10) changes++;
    }
    for (uint32_t v = 0; v < nv; v++) { V[v].nx = nouv[v].x; V[v].ny = nouv[v].y; V[v].nz = nouv[v].z; }
    return changes;
}

namespace {
bool proche(const float* a, std::initializer_list<float> b, float tol = 0.015f) {
    size_t i = 0; for (float x : b) { if (std::fabs(a[i] - x) > tol) return false; i++; } return true;
}
}

std::vector<std::string> corrigerMateriaux(const std::string& id, std::vector<MatBrut>& m, std::vector<uint8_t>& retirer) {
    std::vector<std::string> fait;
    retirer.assign(m.size(), 0);
    char b[160];
    for (size_t i = 0; i < m.size(); i++) {
        MatBrut& x = m[i];
        // RG-01 (ANO-72) : 911 : peinture d'origine en métal pur, rugosité 1 (convertie avant WFS-77) : noire sous le ciel ;
        // peinture unie métallisée du jeu web (repaintMat : métal 0,45, rugosité 0,32, sans texture), comme les 16 autres
        if (id == "p911" && (x.flags & 1) && x.metal > 0.9f) {
            x.metal = 0.45f; x.rough = 0.32f; x.tC = x.tN = x.tMR = -1;
            std::snprintf(b, sizeof b, "matériau %zu : peinture unie métallisée (métal 0,45, rugosité 0,32, RG-01)", i); fait.push_back(b);
        }
        // RG-01 (ANO-72) : 911 : vernis séparé « coat » (transparent gris sombre, alpha 0,46, sur toute la carrosserie)
        if (id == "p911" && (x.flags & 2) && !(x.flags & 64) && proche(x.col, {0.15f, 0.15f, 0.15f, 0.46f})) {
            retirer[i] = 1; std::snprintf(b, sizeof b, "matériau %zu : vernis séparé retiré (peinture visible, RG-01)", i); fait.push_back(b);
        }
    }
    for (size_t i = 0; i < m.size(); i++) {
        MatBrut& x = m[i];
        const bool vitre = (x.flags & 64) != 0;
        // WFS-182 (RG-03, reste d'ANO-76) : SF90 : barres des feux arrière en verre rouge transparent (alpha 0,35) et
        // « métal » 1 : à peine visibles, l'éclat du stop disparaît dans la transparence → verre rouge opaque diélectrique
        // (F0 ≈ 0,04, charte DA-realisme.md § 3 : jamais de verre métallique)
        if (id == "sf90" && i == 14 && (x.flags & 2) && x.col[0] > 0.99f && x.col[1] < 0.01f && x.col[3] < 0.5f) {
            x.col[3] = 1; x.flags &= ~2u; x.metal = 0; x.rough = 0.1f;
            std::snprintf(b, sizeof b, "matériau %zu : barres des feux arrière → verre rouge opaque (WFS-182, RG-03)", i); fait.push_back(b);
        }
        // RG-02 (ANO-73) : Giulia : vitre à carte de normales de sérigraphie (points de 64 px répétés : reflets en points)
        // et d'alpha 0 → verre teinté uni, sans carte de normales
        if (id == "giulia" && vitre && x.col[3] < 0.01f) {
            x.col[0] = x.col[1] = x.col[2] = 0.02f; x.col[3] = 0.6f; x.rough = 0.05f; x.tC = x.tN = -1;
            std::snprintf(b, sizeof b, "matériau %zu : vitre à points (carte de normales) → verre teinté uni (RG-02)", i); fait.push_back(b);
        }
        // RG-03 (ANO-74) : S3 : lunette en double (vitre noire opaque, alpha 1, rugosité 1, posée sur la vitre teintée) : les
        // deux couches se battent pour la profondeur, d'où le camouflage (essai : sans elle, lunette unie) → retirée
        if (id == "s3" && vitre && x.col[3] > 0.99f && x.rough > 0.99f && std::max({x.col[0], x.col[1], x.col[2]}) < 0.001f) {
            retirer[i] = 1; std::snprintf(b, sizeof b, "matériau %zu : vitre en double retirée (camouflage de la lunette, RG-03)", i); fait.push_back(b);
            continue;
        }
        // RG-03 (ANO-74) : S3 : vitres noires opaques et rugueuses (rugosité 1, alpha 1 : reflets en camouflage) → verre
        // teinté (rugosité 0,05 : charte DA-realisme.md § 3, verre 0-0,05)
        if (id == "s3" && vitre && x.rough > 0.5f) {
            x.rough = 0.05f; x.col[3] = std::min(x.col[3], 0.7f); x.tC = -1;
            std::snprintf(b, sizeof b, "matériau %zu : vitre rugueuse opaque → verre teinté (RG-03)", i); fait.push_back(b);
        }
        // RG-03 : S3 : jantes blanc uni (métal 0) → aluminium gris avec leur texture brossée
        if (id == "s3" && !(x.flags & 1) && x.tC >= 0 && proche(x.col, {1, 1, 1, 1}) && std::fabs(x.rough - 0.86f) < 0.02f && x.metal < 0.01f && i == 24) {
            x.col[0] = x.col[1] = 0.6f; x.col[2] = 0.62f; x.metal = 1; x.rough = 0.35f;
            std::snprintf(b, sizeof b, "matériau %zu : jantes → aluminium gris (métal 1, rugosité 0,35, RG-03)", i); fait.push_back(b);
        }
        // RG-06 (ANO-77) : Compass : jantes métal très sombre (« noyées dans le noir ») → métal gris
        if (id == "compass" && x.metal >= 0.8f && std::max({x.col[0], x.col[1], x.col[2]}) < 0.15f && x.tC < 0 && std::fabs(x.rough - 0.30f) < 0.02f) {
            x.col[0] = x.col[1] = x.col[2] = 0.55f; x.metal = 1; x.rough = 0.35f;
            std::snprintf(b, sizeof b, "matériau %zu : jantes → métal gris (RG-06)", i); fait.push_back(b);
        }
    }
    return fait;
}

} // namespace wfs::correctifs
