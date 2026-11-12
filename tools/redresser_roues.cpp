// WaForSpeed — WFS-479 (RG-01, RG-02, RG-05) : roues d'un .wfm remises d'aplomb à la source, pour une voiture dont les roues
// ont du « jeu » à l'écran (M4 du trafic : roues qui se dandinent alors que la voiture roule droit). Le moteur fait rouler
// chaque roue autour d'un axe parallèle à l'axe X de la voiture, passant par le centre de la boîte de ses pièces
// (roues::decouper et roues::matrice, WFS-60). Défauts du modèle M4 et corrections :
//   1. roues braquées dans le modèle source (les deux roues avant modélisées braquées de 20°) : une roue dont l'axe est tourné
//      « bat » à chaque tour. Chaque roue est tournée autour de la verticale de son moyeu jusqu'à ce que son axe soit l'axe X :
//      positions, normales et tangentes (section TAN1 des .wfm version 2), avec les morceaux de pneu que le moteur laisse
//      fixes dans le cylindre de la roue (sinon ils resteraient braqués à côté de la roue droite) ;
// Contrôle RG-05 (tests/test_roues.cpp) : écart entre ce centre de rotation et le moyeu (cercle ajusté sur la bande de
// roulement) < 2 mm.
// Axe d'une roue : normale de sa pièce la plus plate (disque, voile de jante ; analyse en composantes principales) ; centre de
// la rotation : centre de cette pièce. Uv, indices, matériaux et textures intacts : le .lod (indices seuls) reste valable.
// Rejouable : une roue déjà droite (lacet < 0,2°) ne bouge plus.
// Garde-fous : rien n'est écrit si un sommet tourné sert aussi à un triangle fixe hors de la roue (la caisse serait déformée)
// ou si le moteur ne retrouve pas ses 4 roues après la retouche.
// usage : redresser_roues [dossier du dépôt] id … ; --essai : mesure seulement (WFS-479 : la M4 seulement, RG-04)
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "../src/game/roues.h"

#ifndef WFS_SOURCE_DIR
#define WFS_SOURCE_DIR "."
#endif

using namespace wfs;
constexpr double PI_D = 3.14159265358979323846;
constexpr double LACET_MIN = 0.2 * PI_D / 180;   // rad : en dessous, la roue est droite (rejouable)

namespace {
struct Maillage { uint32_t mat = 0; std::vector<float> v; std::vector<uint32_t> idx; std::vector<int8_t> tan; };
struct Modele {
    std::string tete;                  // en-tête, textures et matériaux (recopiés tels quels, sauf nombre de maillages et boîte)
    std::vector<uint32_t> drapeauxMat;
    std::vector<Maillage> ms;
    bool tangentes = false;
};
uint32_t u32(const std::string& s, size_t o) { uint32_t x; std::memcpy(&x, &s[o], 4); return x; }
bool lire(const std::string& chemin, Modele& m) {
    std::ifstream in(chemin, std::ios::binary);
    if (!in) return false;
    const std::string s((std::istreambuf_iterator<char>(in)), {});
    if (s.size() < 44 || std::memcmp(s.data(), "WFM1", 4) != 0) return false;
    const uint32_t nT = u32(s, 8), nM = u32(s, 12), nMe = u32(s, 16);
    size_t o = 44;
    for (uint32_t t = 0; t < nT; t++) { if (o + 4 > s.size()) return false; o += 4 + u32(s, o); }
    for (uint32_t k = 0; k < nM; k++) { if (o + 56 > s.size()) return false; m.drapeauxMat.push_back(u32(s, o + 52)); o += 56; }
    m.tete = s.substr(0, o);
    for (uint32_t k = 0; k < nMe; k++) {
        if (o + 12 > s.size()) return false;
        Maillage x; x.mat = u32(s, o); const uint32_t nv = u32(s, o + 4), ni = u32(s, o + 8); o += 12;
        if (x.mat >= nM || o + (size_t)nv * 32 + (size_t)ni * 4 > s.size()) return false;
        x.v.resize((size_t)nv * 8); std::memcpy(x.v.data(), &s[o], (size_t)nv * 32); o += (size_t)nv * 32;
        x.idx.resize(ni); std::memcpy(x.idx.data(), &s[o], (size_t)ni * 4); o += (size_t)ni * 4;
        for (uint32_t i : x.idx) if (i >= nv) return false;
        m.ms.push_back(std::move(x));
    }
    if (o == s.size()) return true;
    if (o + 4 > s.size() || std::memcmp(&s[o], "TAN1", 4) != 0) return false;
    o += 4; m.tangentes = true;
    for (Maillage& x : m.ms) {
        const size_t n = x.v.size() / 8 * 4;
        if (o + n > s.size()) return false;
        x.tan.assign(s.begin() + (long)o, s.begin() + (long)(o + n)); o += n;
    }
    return o == s.size();   // rien d'inconnu après les tangentes (sinon on ne saurait pas le réécrire)
}
bool ecrire(const std::string& chemin, const Modele& m) {
    std::string s = m.tete;
    const uint32_t n = (uint32_t)m.ms.size(); std::memcpy(&s[16], &n, 4);
    float bb[6] = {1e9f, 1e9f, 1e9f, -1e9f, -1e9f, -1e9f};
    for (const Maillage& x : m.ms) for (uint32_t i : x.idx) for (int q = 0; q < 3; q++) { bb[q] = std::min(bb[q], x.v[8 * i + q]); bb[q + 3] = std::max(bb[q + 3], x.v[8 * i + q]); }
    std::memcpy(&s[20], bb, sizeof bb);
    auto ajoute = [&](const void* p, size_t k) { s.append(static_cast<const char*>(p), k); };
    for (const Maillage& x : m.ms) {
        const uint32_t e[3] = {x.mat, (uint32_t)(x.v.size() / 8), (uint32_t)x.idx.size()};
        ajoute(e, 12); ajoute(x.v.data(), x.v.size() * 4); ajoute(x.idx.data(), x.idx.size() * 4);
    }
    if (m.tangentes) { s += "TAN1"; for (const Maillage& x : m.ms) ajoute(x.tan.data(), x.tan.size()); }
    std::ofstream out(chemin, std::ios::binary);
    out.write(s.data(), (std::streamsize)s.size());
    return (bool)out;
}
bool estRoue(const Modele& m, const Maillage& x) { return (m.drapeauxMat[x.mat] & 128) != 0; }
roues::Decoupe decouper(Modele& m) {
    std::vector<roues::Maillage> ms;
    for (Maillage& x : m.ms) ms.push_back({x.v.data(), 8, (uint32_t)(x.v.size() / 8), x.idx.data(), (uint32_t)x.idx.size(), estRoue(m, x)});
    return roues::decouper(ms);
}
// analyse en composantes principales : normale (plus petite variance) et aplatissement (plus petite / moyenne variance)
struct Plan { std::array<double, 3> c{}, n{}; double plat = 1; size_t nb = 0; };
Plan plan(const std::vector<std::array<double, 3>>& p) {
    Plan r; r.nb = p.size();
    if (p.size() < 12) return r;
    for (const auto& q : p) for (int i = 0; i < 3; i++) r.c[i] += q[i] / (double)p.size();
    double A[3][3] = {};
    for (const auto& q : p) for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) A[i][j] += (q[i] - r.c[i]) * (q[j] - r.c[j]) / (double)p.size();
    double V[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    for (int it = 0; it < 60; it++) {   // Jacobi (symétrique 3 × 3)
        int a = 0, b = 1; for (int i = 0; i < 3; i++) for (int j = i + 1; j < 3; j++) if (std::fabs(A[i][j]) > std::fabs(A[a][b])) { a = i; b = j; }
        if (std::fabs(A[a][b]) < 1e-15) break;
        const double th = 0.5 * std::atan2(2 * A[a][b], A[b][b] - A[a][a]), c = std::cos(th), s = std::sin(th);
        for (int k = 0; k < 3; k++) { const double x = A[k][a], y = A[k][b]; A[k][a] = c * x - s * y; A[k][b] = s * x + c * y; }
        for (int k = 0; k < 3; k++) { const double x = A[a][k], y = A[b][k]; A[a][k] = c * x - s * y; A[b][k] = s * x + c * y; }
        for (int k = 0; k < 3; k++) { const double x = V[k][a], y = V[k][b]; V[k][a] = c * x - s * y; V[k][b] = s * x + c * y; }
    }
    int lo = 0, hi = 0; for (int i = 1; i < 3; i++) { if (A[i][i] < A[lo][lo]) lo = i; if (A[i][i] > A[hi][hi]) hi = i; }
    const int mid = 3 - lo - hi;
    r.n = {V[0][lo], V[1][lo], V[2][lo]};
    if (r.n[0] < 0) for (double& x : r.n) x = -x;
    r.plat = A[mid][mid] > 0 ? A[lo][lo] / A[mid][mid] : 1;
    return r;
}
const char* NOM[4] = {"avant gauche", "avant droite", "arrière gauche", "arrière droite"};
} // namespace

int main(int argc, char** argv) {
    std::string racine = WFS_SOURCE_DIR;
    std::vector<std::string> ids;
    bool essai = false;
    for (int i = 1; i < argc; i++) {
        const std::string a = argv[i];
        if (a == "--essai") essai = true;
        else if (a.find('/') != std::string::npos || a == ".") racine = a;
        else ids.push_back(a);
    }
    if (ids.empty()) { std::fprintf(stderr, "usage : redresser_roues [dossier du dépôt] id … [--essai]\n"); return 2; }
    int erreurs = 0;
    for (const std::string& id : ids) {
        const std::string chemin = racine + "/assets/cars/" + id + ".wfm";
        Modele m;
        if (!lire(chemin, m)) { std::fprintf(stderr, "%s : illisible\n", id.c_str()); erreurs++; continue; }
        const roues::Decoupe d0 = decouper(m);
        if (d0.n != 4) { std::fprintf(stderr, "%s : 4 roues non trouvées, voiture laissée telle quelle\n", id.c_str()); erreurs++; continue; }
        bool change = false;

        // morceau de pneu laissé fixe par le moteur (roues::decouper : médiane autour du centre du coin, trop étroite pour une
        // roue braquée) : triangle d'un matériau « roue » dont le centre est dans le cylindre de la roue (30 cm de côté, 1,15 R) ;
        // il tourne avec la roue qu'on redresse (sinon il resterait braqué, seul, à côté de la roue droite)
        auto dansZone = [&](size_t mi, size_t t, int k) {
            const Maillage& x = m.ms[mi];
            if (!estRoue(m, x) || d0.roueDuTriangle[mi][t] >= 0) return false;
            float c[3] = {0, 0, 0};
            for (int j = 0; j < 3; j++) for (int q = 0; q < 3; q++) c[q] += x.v[8 * x.idx[t * 3 + j] + q] / 3;
            const RoueModele& r = d0.roues[k];
            return ((c[0] > 0 ? 1 : 0) + (c[2] < 0 ? 0 : 2)) == k && std::fabs(c[0] - r.centre.x) < 0.30f &&
                   std::hypot(c[1] - r.centre.y, c[2] - r.centre.z) < 1.15f * r.rayon;
        };
        const roues::Decoupe& d1 = d0;

        // 1. roues braquées dans le modèle : tournées autour de la verticale de leur moyeu
        bool refus = false;
        for (int k = 0; k < 4 && !refus; k++) {
            Plan best;
            for (size_t mi = 0; mi < m.ms.size(); mi++) {
                const Maillage& x = m.ms[mi];
                std::vector<char> pris(x.v.size() / 8, 0);
                std::vector<std::array<double, 3>> pts;
                for (size_t t = 0; t < x.idx.size() / 3; t++) if (d1.roueDuTriangle[mi][t] == k)
                    for (int j = 0; j < 3; j++) if (const uint32_t v = x.idx[t * 3 + j]; !pris[v]) { pris[v] = 1; pts.push_back({x.v[8 * v], x.v[8 * v + 1], x.v[8 * v + 2]}); }
                if (pts.size() < 100) continue;
                const Plan p = plan(pts);
                if (p.plat < best.plat) best = p;
            }
            if (best.nb == 0) { std::fprintf(stderr, "%s : roue %d sans pièce plate (axe inconnu)\n", id.c_str(), k); refus = true; break; }
            const double lacet = std::atan2(best.n[2], best.n[0]), carrossage = std::asin(std::clamp(best.n[1], -1.0, 1.0));
            std::printf("%s : roue %s : axe lacet %+.2f°, carrossage %+.2f° (pièce plate : %zu sommets, aplatissement %.4f), moyeu x %.4f z %.4f — %s\n",
                        id.c_str(), NOM[k], lacet * 180 / PI_D, carrossage * 180 / PI_D, best.nb, best.plat, best.c[0], best.c[2],
                        std::fabs(lacet) < LACET_MIN ? "droite" : "à redresser");
            if (std::fabs(lacet) < LACET_MIN) continue;
            // l'axe (cos a, ·, sin a) devient l'axe X : X = x cos a + z sin a, Z = −x sin a + z cos a
            const double c = std::cos(lacet), s = std::sin(lacet), cx = best.c[0], cz = best.c[2];
            auto tourner = [&](double x, double z, double& X, double& Z) { X = x * c + z * s; Z = -x * s + z * c; };
            for (size_t mi = 0; mi < m.ms.size() && !refus; mi++) {
                Maillage& x = m.ms[mi];
                std::vector<int8_t> etat(x.v.size() / 8, 0);   // 1 : sommet de cette roue, 2 : sommet d'un triangle fixe
                for (size_t t = 0; t < x.idx.size() / 3; t++)
                    for (int j = 0; j < 3; j++) etat[x.idx[t * 3 + j]] |= d1.roueDuTriangle[mi][t] == k || dansZone(mi, t, k) ? 1 : d1.roueDuTriangle[mi][t] < 0 ? 2 : 0;
                for (size_t v = 0; v < etat.size(); v++) {
                    if (etat[v] == 3) { std::fprintf(stderr, "%s : maillage %zu, sommet %zu partagé entre la roue %s et la caisse : rien d'écrit\n", id.c_str(), mi, v, NOM[k]); refus = true; break; }
                    if (etat[v] != 1) continue;
                    float* p = &x.v[8 * v];
                    double X, Z;
                    tourner(p[0] - cx, p[2] - cz, X, Z); p[0] = (float)(cx + X); p[2] = (float)(cz + Z);
                    tourner(p[3], p[5], X, Z); p[3] = (float)X; p[5] = (float)Z;
                    if (!x.tan.empty()) {
                        int8_t* tg = &x.tan[4 * v];
                        tourner(tg[0] / 127.0, tg[2] / 127.0, X, Z);
                        auto oct = [](double y) { return (int8_t)std::lround(std::clamp(y, -1.0, 1.0) * 127); };
                        tg[0] = oct(X); tg[2] = oct(Z);
                    }
                }
            }
            change = true;
        }
        if (refus) { erreurs++; continue; }

        if (!change) { std::printf("%s : roues entières et droites, inchangée\n", id.c_str()); continue; }
        const roues::Decoupe d2 = decouper(m);
        if (d2.n != 4) { std::fprintf(stderr, "%s : 4 roues perdues après la retouche, rien d'écrit\n", id.c_str()); erreurs++; continue; }
        for (int k = 0; k < 4; k++)
            std::printf("%s : roue %s : centre (%.4f, %.4f, %.4f) → (%.4f, %.4f, %.4f), rayon %.4f → %.4f m\n", id.c_str(), NOM[k],
                        d0.roues[k].centre.x, d0.roues[k].centre.y, d0.roues[k].centre.z, d2.roues[k].centre.x, d2.roues[k].centre.y, d2.roues[k].centre.z,
                        d0.roues[k].rayon, d2.roues[k].rayon);
        if (essai) { std::printf("%s : essai, rien d'écrit\n", id.c_str()); continue; }
        if (!ecrire(chemin, m)) { std::fprintf(stderr, "%s : écriture impossible\n", id.c_str()); erreurs++; }
        else std::printf("%s : %s écrit\n", id.c_str(), chemin.c_str());
    }
    return erreurs ? 1 : 0;
}
