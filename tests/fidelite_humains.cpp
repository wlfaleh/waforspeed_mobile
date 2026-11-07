// WFS-209 (RG-01 à RG-03 ; M-01, M-02) : les 12 humains réalistes (assets/people/humains/*.wfp, MakeHuman CC0) mesurés contre
// le référentiel du réel (references/reel/anthropometrie.json, ANSUR-II), en pose de repos (T), sur le fichier lui-même :
//   - M-01 : stature (y max − y min) : moyenne et écart-type des 6 hommes et des 6 femmes = ANSUR-II ± 1 % ;
//   - M-02 : entrejambe / stature et envergure / stature = ratios d'ANSUR-II ± 2 % ; tête (vertex-menton) / stature = 1 / 7,5 ± 2 % ;
//   - format (RG-03) : version 3 (tangentes unitaires), squelette identique à celui des personnages du jeu (noms, ordre, parents :
//     clips de WFS-63 utilisables), palette ≤ 64, poids normalisés, plage « corps-0 », clips présents.
// Définitions des mesures : les mêmes que tools/convert_people_makehuman.py (mesurer).
// usage : test_fidelite_humains [dossier du dépôt] [fichier du rapport]
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include "../src/render/bench_realisme.h"

#ifndef WFS_SOURCE_DIR
#define WFS_SOURCE_DIR "."
#endif

static int echecs = 0, mesures = 0, conformes = 0;
static std::ostringstream rapport;
static void cas(const std::string& quoi, bool ok) { std::printf("humains : %-96s → %s\n", quoi.c_str(), ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }
static void mesure(const std::string& quoi, double v, double ref, double tol) {
    const bool ok = std::fabs(v / ref - 1) <= tol + 1e-9;
    mesures++; if (ok) conformes++;
    char l[256]; std::snprintf(l, sizeof l, "%s | %.4f | réel %.4f ± %.0f %% | %+.2f %% | %s\n", quoi.c_str(), v, ref, tol * 100, (v / ref - 1) * 100, ok ? "conforme" : "HORS TOLÉRANCE");
    rapport << l;
    cas(quoi + " " + std::to_string(v).substr(0, 6), ok);
}

struct Wfp {
    uint32_t version = 0, nV = 0, nI = 0; float corps[4] = {};
    std::vector<float> P, N, T;           // positions, normales, tangentes (xyz w)
    std::vector<float> poids;             // 4 par sommet
    std::vector<float> C;                 // _cloth (bras, hauteur, jambe, partie)
    std::vector<uint8_t> pal;             // palette d'os (4 par sommet)
    std::vector<uint32_t> I; std::vector<std::pair<uint32_t, uint32_t>> plagesI;   // WFS-211 : indices, plages (premier, nombre)
    std::vector<std::string> plages, os; std::vector<int> parents; uint32_t nPal = 0, nClips = 0;
    std::vector<uint32_t> palette;        // os du jeu de chaque entrée de la palette
};
static bool lire(const std::string& chemin, Wfp& w) {
    std::ifstream f(chemin, std::ios::binary);
    if (!f) return false;
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)), {});
    size_t o = 0;
    auto u32 = [&]() { uint32_t v = 0; if (o + 4 <= b.size()) std::memcpy(&v, &b[o], 4); o += 4; return v; };
    auto f32 = [&]() { float v = 0; if (o + 4 <= b.size()) std::memcpy(&v, &b[o], 4); o += 4; return v; };
    if (b.size() < 8 || std::memcmp(b.data(), "WFP1", 4) != 0) return false;
    o = 4; w.version = u32();
    const uint32_t nT = u32();
    for (uint32_t i = 0; i < nT; i++) o += u32();
    f32(); f32(); f32(); for (float& c : w.corps) c = f32();
    w.nV = u32(); w.nI = u32(); const uint32_t nP = u32(), nO = u32(); w.nClips = u32();
    const size_t pas = w.version == 3 ? 84 : 68;
    if (o + (size_t)w.nV * pas > b.size()) return false;
    for (uint32_t v = 0; v < w.nV; v++) {
        const uint8_t* s = &b[o + v * pas];
        float x[16]; std::memcpy(x, s, 32);
        w.P.insert(w.P.end(), x, x + 3); w.N.insert(w.N.end(), x + 3, x + 6);
        if (w.version == 3) { float t[4]; std::memcpy(t, s + 32, 16); w.T.insert(w.T.end(), t, t + 4); }
        float p[4]; std::memcpy(p, s + (w.version == 3 ? 52 : 36), 16); w.poids.insert(w.poids.end(), p, p + 4);
        float c[4]; std::memcpy(c, s + (w.version == 3 ? 68 : 52), 16); w.C.insert(w.C.end(), c, c + 4);
        w.pal.insert(w.pal.end(), s + (w.version == 3 ? 48 : 32), s + (w.version == 3 ? 52 : 36));
    }
    o += (size_t)w.nV * pas;
    if (o + (size_t)w.nI * 4 > b.size()) return false;
    w.I.resize(w.nI); std::memcpy(w.I.data(), &b[o], (size_t)w.nI * 4); o += (size_t)w.nI * 4;
    for (uint32_t i = 0; i < nP; i++) { w.plages.emplace_back((const char*)&b[o]); uint32_t a[2]; std::memcpy(a, &b[o + 32], 8); w.plagesI.push_back({a[0], a[1]}); o += 40; }
    for (uint32_t i = 0; i < nO; i++) { w.os.emplace_back((const char*)&b[o]); int32_t p; std::memcpy(&p, &b[o + 32], 4); w.parents.push_back(p); o += 32 + 4 + 40 + 64; }
    w.nPal = u32();
    for (uint32_t i = 0; i < w.nPal && i <= 64; i++) w.palette.push_back(u32());
    return o <= b.size();
}

// mêmes définitions que mesurer() de tools/convert_people_makehuman.py
struct Mesures { double stature, entrejambe, envergure, menton; };
// utilise : sommets du niveau de détail mesuré (vide : tous)
static Mesures mesurer(const Wfp& w, const std::vector<bool>& utilise = {}) {
    auto pris = [&](uint32_t v) { return utilise.empty() || utilise[v]; };
    double y0 = 1e9, y1 = -1e9, x0 = 1e9, x1 = -1e9;
    for (uint32_t v = 0; v < w.nV; v++) {
        if (!pris(v)) continue; y0 = std::min(y0, (double)w.P[v * 3 + 1]); y1 = std::max(y1, (double)w.P[v * 3 + 1]); x0 = std::min(x0, (double)w.P[v * 3]); x1 = std::max(x1, (double)w.P[v * 3]); }
    const double st = y1 - y0, cou = w.corps[0] - y0;
    double entre = 1e9, zcou = -1e9, menton = 1e9;
    for (uint32_t v = 0; v < w.nV; v++) {
        if (!pris(v)) continue;
        const double x = w.P[v * 3], y = w.P[v * 3 + 1] - y0, z = w.P[v * 3 + 2];
        if (std::fabs(x) >= 0.008) continue;
        if (w.N[v * 3 + 1] < -0.5 && y > 0.3 * st && y < 0.6 * st) entre = std::min(entre, y);
        if (y >= cou - 0.02 && y <= cou) zcou = std::max(zcou, z);
    }
    for (uint32_t v = 0; v < w.nV; v++) {
        if (!pris(v)) continue;
        const double x = w.P[v * 3], y = w.P[v * 3 + 1] - y0, z = w.P[v * 3 + 2];
        if (std::fabs(x) < 0.008 && y > cou - 0.02 && z > zcou + 0.03) menton = std::min(menton, y);
    }
    return {st, entre, x1 - x0, menton};
}

int main(int argc, char** argv) {
    const std::string depot = argc > 1 ? argv[1] : WFS_SOURCE_DIR, sortie = argc > 2 ? argv[2] : "";
    wfs::reel::Referentiel R;
    cas("référentiel du réel lisible", R.charger(depot + "/references/reel"));
    auto val = [&](const char* id) { const wfs::reel::Json* m = R.mesure(id); return m ? m->nombre("valeur", 0) : 0.0; };
    Wfp jeu[2];
    cas("personnages du jeu lisibles (squelette de référence)", lire(depot + "/assets/people/femme.wfp", jeu[0]) && lire(depot + "/assets/people/homme.wfp", jeu[1]));
    const char* const sexes[2] = {"femmes", "hommes"};
    for (int s = 0; s < 2; s++) {
        std::vector<double> st;
        for (int k = 1; k <= 6; k++) {
            const std::string id = std::string(s ? "h" : "f") + "0" + std::to_string(k);
            Wfp w;
            if (!lire(depot + "/assets/people/humains/" + id + ".wfp", w)) { cas(id + ".wfp lisible", false); continue; }
            // RG-03 : format et squelette
            bool tangentes = w.version == 3 && w.T.size() == (size_t)w.nV * 4, poids = true;
            for (uint32_t v = 0; tangentes && v < w.nV; v++) {
                const double l = std::sqrt(w.T[v * 4] * w.T[v * 4] + w.T[v * 4 + 1] * w.T[v * 4 + 1] + w.T[v * 4 + 2] * w.T[v * 4 + 2]);
                tangentes = std::fabs(l - 1) < 1e-3 && std::fabs(std::fabs(w.T[v * 4 + 3]) - 1) < 1e-6;
            }
            for (uint32_t v = 0; v < w.nV; v++) poids = poids && std::fabs(w.poids[v * 4] + w.poids[v * 4 + 1] + w.poids[v * 4 + 2] + w.poids[v * 4 + 3] - 1) < 1e-3;
            cas(id + " : version 3, tangentes unitaires, poids normalisés", tangentes && poids);
            cas(id + " : squelette du jeu (noms, ordre, parents), palette ≤ 64, corps-0, clips",
                w.os == jeu[s].os && w.parents == jeu[s].parents && w.nPal <= 64 && w.nClips == jeu[s].nClips
                && std::find(w.plages.begin(), w.plages.end(), "corps-0") != w.plages.end());
            const Mesures m = mesurer(w);
            st.push_back(m.stature);
            // WFS-211 (RG-01) : 3 niveaux de détail ; M-01 et M-02 tenus à chacun (sommets réellement dessinés par la plage)
            int niveaux = 0; bool memes = true;
            for (int lod = 0; lod < 3; lod++) {
                const auto it = std::find(w.plages.begin(), w.plages.end(), "corps-" + std::to_string(lod));
                if (it == w.plages.end()) continue;
                const auto [a, n] = w.plagesI[(size_t)(it - w.plages.begin())];
                if ((size_t)a + n > w.I.size() || n == 0) continue;
                std::vector<bool> u(w.nV, false);
                for (uint32_t i = a; i < a + n; i++) if (w.I[i] < w.nV) u[w.I[i]] = true;
                const Mesures l = mesurer(w, u);
                niveaux++;
                memes = memes && std::fabs(l.stature / m.stature - 1) < 0.005 && std::fabs(l.entrejambe / m.entrejambe - 1) < 0.005
                        && std::fabs(l.envergure / m.envergure - 1) < 0.005 && std::fabs(l.menton / m.menton - 1) < 0.005;
                if (lod > 0) cas(id + " : corps-" + std::to_string(lod) + " allégé (" + std::to_string(n / 3) + " triangles)", n < w.plagesI[0].second * (lod == 1 ? 0.5 : 0.2));
            }
            cas(id + " : 3 niveaux de détail, M-01 et M-02 identiques à ± 0,5 % sur chacun", niveaux == 3 && memes);
            // ANO-111 : aucun sommet de la tête ni du cou dans la zone du haut (hauteur _cloth sous le col : cou + 5 cm)
            const uint32_t cou = (uint32_t)(std::find(w.os.begin(), w.os.end(), "neck_01") - w.os.begin()),
                           tete = (uint32_t)(std::find(w.os.begin(), w.os.end(), "Head") - w.os.begin());
            int teteHabillee = 0;
            for (uint32_t v = 0; v < w.nV; v++) {
                const float* pw = &w.poids[v * 4]; const uint8_t* pp = &w.pal[v * 4];
                float wt = 0;
                for (int k = 0; k < 4; k++) if (pp[k] < w.palette.size() && (w.palette[pp[k]] == cou || w.palette[pp[k]] == tete)) wt += pw[k];
                if (wt > 0.5f && w.C[v * 4 + 3] < 0.5f && w.C[v * 4 + 1] <= w.corps[0] + 0.05f) teteHabillee++;
            }
            cas(id + " : ANO-111 tête et cou hors du vêtement (" + std::to_string(teteHabillee) + " sommet(s) dans le haut)", teteHabillee == 0);
            mesure(id + " : M-02 entrejambe / stature", m.entrejambe / m.stature, val((std::string("humain.") + sexes[s] + ".ratio_crotchheight").c_str()), 0.02);
            mesure(id + " : M-02 envergure / stature", m.envergure / m.stature, val((std::string("humain.") + sexes[s] + ".ratio_span").c_str()), 0.02);
            mesure(id + " : M-02 tête / stature", (m.stature - m.menton) / m.stature, 1 / 7.5, 0.02);
        }
        double moy = 0, ec = 0;
        for (double x : st) moy += x / st.size();
        for (double x : st) ec += (x - moy) * (x - moy) / st.size();
        ec = std::sqrt(ec);
        mesure(std::string(sexes[s]) + " : M-01 stature moyenne (m)", moy, val((std::string("humain.") + sexes[s] + ".stature").c_str()) / 1000, 0.01);
        const wfs::reel::Json* m = R.mesure(std::string("humain.") + sexes[s] + ".stature");
        mesure(std::string(sexes[s]) + " : M-01 écart-type de la stature (m)", ec, m ? m->nombre("ecart_type", 0) / 1000 : 0, 0.01);
    }
    const double indice = mesures ? 100.0 * conformes / mesures : 0;
    std::printf("humains : indice de fidélité : %.1f %% (%d / %d mesures)\n", indice, conformes, mesures);
    rapport << "indice de fidélité des humains : " << indice << " % (" << conformes << " / " << mesures << ")\n";
    cas("indice de fidélité ≥ 99 %", indice >= 99.0);
    if (!sortie.empty()) { std::ofstream f(sortie); f << rapport.str(); }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("humains : tout est OK\n");
    return 0;
}
