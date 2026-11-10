// WaForSpeed — WFS-161 (ST-5) : bâtiments du kit posés dans les cartes (voir archi_carte.h).
#include "archi_carte.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/system.h"
#include "../game/player.h"
#include "../game/track.h"
#include "../render/eclairage_precalcule.h"   // WFS-269 (accroche)
#include "../render/eclairage_unites.h"
#include "../render/renderer.h"
#include "archi_geo.h"

namespace wfs::archi::carte {
namespace {

struct Bat { int64_t seg; vec3 o, x, z; float profondeur; Plan plan; };
std::vector<Bat> g_bats;

// emplacements de maillage d'un morceau : un par matériau (murs par style, toits par type, puis parties communes)
constexpr int NS = (int)Style::Nombre, NT = 3;
enum Slot : int { S_MUR = 0, S_TOIT = NS, S_ENCADREMENT = NS + NT, S_VITRE, S_MENUISERIE, S_METAL, S_VOLET, S_FIN, S_CHASSIS, NB_SLOTS };
int slot(int part, const Plan& p) {
    switch (part) {
    case MUR: return S_MUR + (int)p.style;
    case TOIT: return S_TOIT + (int)p.toit;
    case ENCADREMENT: return S_ENCADREMENT;
    case VITRE: return S_VITRE;
    case MENUISERIE: return S_MENUISERIE;
    case METAL: return S_METAL;
    case FIN: return S_FIN;
    case CHASSIS: return S_CHASSIS;   // WFS-402
    default: return S_VOLET;
    }
}

// RG-05 : niveau de détail selon la distance au joueur (m)
constexpr double LOD0_M = 120, LOD1_M = 300, VUE_AVANT_M = 650, VUE_ARRIERE_M = 60;
constexpr int64_t MORCEAU = 48;   // segments par morceau (≈ 58 m)

struct Cpu { std::vector<Vertex> v[NB_SLOTS]; std::vector<uint32_t> i[NB_SLOTS]; };

// bâtiment b au niveau lod, passé dans le repère du monde, ajouté aux tampons du morceau
void ajouter(const Bat& b, int lod, Cpu& out) {
    Batiment loc;
    construire(b.plan, b.profondeur, lod, loc);
    const vec3 up{0, 1, 0};
    for (int k = 0; k < NB_PARTS; k++) {
        const Maillage& m = loc.part[k];
        if (m.i.empty()) continue;
        const int s = slot(k, b.plan);
        const uint32_t base = (uint32_t)out.v[s].size();
        for (const Sommet& q : m.v) {
            const vec3 p = b.o + b.x * q.p.x + up * q.p.y + b.z * q.p.z, n = b.x * q.n.x + up * q.n.y + b.z * q.n.z;
            Vertex v{}; v.px = p.x; v.py = p.y; v.pz = p.z; v.nx = n.x; v.ny = n.y; v.nz = n.z; v.u = q.uv.x; v.v = q.uv.y;
            out.v[s].push_back(v);
        }
        for (uint32_t i : m.i) out.i[s].push_back(base + i);
        if (k == MUR) precalcul::appliquerKit((size_t)(&b - g_bats.data()), lod, out.v[s], base);   // WFS-269 : occlusion gardée des murs
    }
}

class ArchiCarte : public System {
public:
    const char* name() const override { return "archi-carte"; }

    void init(Renderer& r, const GameView& g) override {
        if (!actif() || !g.track) return;
        const char* mo = std::getenv("WFS_MOMENT"); const bool nuit = mo && !std::strcmp(mo, "nuit");
        auto mat = [&](const Albedo& a, float k, float rough, float metal, uint32_t flags = MAT_DOUBLE, float cutoff = 0.5f) {
            Material m; m.color = {a.r * k, a.g * k, a.b * k, 1}; m.rough = rough; m.metal = metal; m.flags = flags; m.cutoff = cutoff;
            return r.makeMaterial(m, nullptr, nullptr, nullptr);
        };
        for (int s = 0; s < NS; s++)   // murs : appareil procédural (decor.glsl), albédo moyen = référentiel (M-03) ; brique pour l'industriel
            mats_[S_MUR + s] = mat(s == (int)Style::Industriel ? ALB_BRIQUE : ALB_BETON, 1, 0.9f, 0, MAT_DOUBLE | MAT_FACADE | MAT_AO, 0.2f * (float)APPAREIL[s]);   // WFS-269 : MAT_AO   // × 0,2 : sous l'alpha 1 (shadow.frag jette alpha < cutoff)
        mats_[S_TOIT + (int)Toit::Zinc] = mat(ALB_INOX, 0.6f, 0.62f, 1);   // WFS-370 : zinc patiné (mat), plus de reflet blanc du ciel sur le terrasson
        mats_[S_TOIT + (int)Toit::Tuiles] = mat(ALB_TERRE_CUITE, 1, 0.8f, 0);
        mats_[S_TOIT + (int)Toit::Terrasse] = mat(ALB_BETON, 0.6f, 0.95f, 0);
        mats_[S_ENCADREMENT] = mat(ALB_BETON, 1, 0.8f, 0);
        mats_[S_MENUISERIE] = mat({"bois", 0.18f, 0.12f, 0.07f}, 1, 0.6f, 0);       // menuiserie bois (conception, provisoire)
        mats_[S_METAL] = mat(ALB_ALUMINIUM, 0.3f, 0.4f, 1);
        mats_[S_FIN] = mat(ALB_ALUMINIUM, 0.3f, 0.4f, 1, MAT_DOUBLE | MAT_BLEND);   // WFS-370 : barreaux, couverture partielle (scene.frag, detailK)
        mats_[S_VOLET] = mat({"volet", 0.09f, 0.2f, 0.13f}, 1, 0.6f, 0);           // volets peints (conception, provisoire)
        mats_[S_CHASSIS] = mat({"peinture blanc cassé", 0.70f, 0.68f, 0.62f}, 1, 0.45f, 0);   // WFS-402 : menuiseries de bois peint (RAL 9001, albédo ≈ 0,7 ; conception)
        {   // vitres + pièces de nuit (ST-3) : lampe 2850 K, murs 31,8 cd/m² (E = 200 lx, ρ = 0,5 : conception), 35 % allumées la nuit
            double c[3]; lumiere::couleurTemperature(2850, c);
            const float L = (float)(0.5 * 200.0 / lumiere::PI_L);
            Material m; m.color = {0.03f, 0.035f, 0.04f, 1}; m.rough = 0.05f; m.flags = MAT_DOUBLE | MAT_INTERIEUR;
            m.emissive = {(float)c[0] * L, (float)c[1] * L, (float)c[2] * L}; m.cutoff = nuit ? 0.35f : 0.f;
            mats_[S_VITRE] = r.makeMaterial(m, nullptr, nullptr, nullptr);
        }
        pret_ = true;
        precalcul::occlusionKit(g_bats.size(), [](size_t k, int lod, std::vector<Vertex>& v) {   // WFS-269 : murs, au chargement
            Cpu c; ajouter(g_bats[k], lod, c); v = std::move(c.v[slot(MUR, g_bats[k].plan)]); });
        size_t tri = 0;
        for (const Bat& b : g_bats) { Batiment x; construire(b.plan, b.profondeur, 0, x); for (const Maillage& m : x.part) tri += m.triangles(); }
        WFS_LOG("archi (WFS-161) : %zu bâtiments du kit posés, %zu triangles au niveau de détail 0 (tous), nuit %s", g_bats.size(), tri, nuit ? "oui" : "non");
    }

    void render(Renderer& r, const GameView& g) override {
        if (!pret_ || g.sceneDediee || !g.player || !g.track) return;
        frame_++;
        const int64_t L = std::max<int64_t>(1, (int64_t)g.track->segmentCount());
        const int64_t aP = ((int64_t)std::floor(g.player->dist / SEG) % L + L) % L;   // tours suivants : même plan (limite : le raccord du tour)
        auto lodVoulu = [&](int64_t c) {   // distance du joueur au morceau (m), au plus près
            const double d0 = (double)(c * MORCEAU - aP) * SEG_M, d1 = d0 + MORCEAU * SEG_M;
            const double d = d0 > 0 ? d0 : (d1 < 0 ? -d1 : 0);
            return d < LOD0_M ? 0 : d < LOD1_M ? 1 : 2;
        };
        const int64_t c0 = (aP - (int64_t)(VUE_ARRIERE_M / SEG_M)) / MORCEAU, c1 = (aP + (int64_t)(VUE_AVANT_M / SEG_M)) / MORCEAU;
        for (auto it = morceaux_.begin(); it != morceaux_.end();) {   // hors de vue ou mauvais niveau : rendu au cimetière (3 images)
            if (it->first < c0 || it->first > c1 || it->second.lod != lodVoulu(it->first)) {
                for (Mesh& m : it->second.m) if (m.indexCount) cimetiere_.push_back({frame_, m});
                it = morceaux_.erase(it);
            } else ++it;
        }
        std::vector<int64_t> manque;
        for (int64_t c = c0; c <= c1; c++) if (!morceaux_.count(c) && parMorceau(c)) manque.push_back(c);
        if (!manque.empty()) {
            std::vector<Cpu> cpu(manque.size());
            jobs().parallelFor((int)manque.size(), 1, [&](int b, int e) {   // construction pure (aucun Vulkan) sur les fils de travail
                for (int i = b; i < e; i++) for (const Bat& x : g_bats) if (x.seg / MORCEAU == manque[i]) ajouter(x, lodVoulu(manque[i]), cpu[i]);
            });
            for (size_t i = 0; i < manque.size(); i++) {
                Morceau& m = morceaux_[manque[i]];
                m.lod = lodVoulu(manque[i]);
                for (int s = 0; s < NB_SLOTS; s++) if (!cpu[i].i[s].empty()) m.m[s] = r.makeMesh(cpu[i].v[s], cpu[i].i[s], true);
            }
        }
        for (auto it = cimetiere_.begin(); it != cimetiere_.end();) {
            if (frame_ > it->first + 3) { r.destroy(it->second); it = cimetiere_.erase(it); } else ++it;
        }
        const mat4 I = mat4::identity();
        for (auto& [c, m] : morceaux_) for (int s = 0; s < NB_SLOTS; s++) if (m.m[s].indexCount) r.draw(m.m[s], mats_[s], I, s == S_FIN ? 0.016f : 0.f);   // WFS-370 : épaisseur des barreaux
    }

    void shutdown(Renderer& r) override {
        for (auto& [c, m] : morceaux_) for (Mesh& x : m.m) if (x.indexCount) r.destroy(x);
        for (auto& x : cimetiere_) r.destroy(x.second);
        if (pret_) for (Material& m : mats_) r.destroy(m);
        morceaux_.clear(); cimetiere_.clear();
        pret_ = false;
        vider();   // la carte suivante pose ses propres bâtiments (ou aucun : anciennes boîtes)
    }

private:
    struct Morceau { int lod = 0; std::array<Mesh, NB_SLOTS> m{}; };
    bool parMorceau(int64_t c) const { for (const Bat& b : g_bats) if (b.seg / MORCEAU == c) return true; return false; }
    bool pret_ = false;
    uint64_t frame_ = 0;
    std::map<int64_t, Morceau> morceaux_;
    std::vector<std::pair<uint64_t, Mesh>> cimetiere_;
    Material mats_[NB_SLOTS]{};
};

} // namespace

void vider() { g_bats.clear(); }

void poser(int64_t seg, vec3 origine, vec3 x, vec3 z, float largeur, float profondeur, float hauteur, Style style, uint32_t graine) {
    g_bats.push_back({seg, origine, x, z, profondeur, assembler(style, largeur, hauteur, graine)});
}

void geometrie(int lod, std::vector<Vertex>& v, std::vector<uint32_t>& i) {   // WFS-269 (accroche) : voir archi_carte.h
    for (const Bat& b : g_bats) { Cpu c; ajouter(b, lod, c); for (int s = 0; s < NB_SLOTS; s++) {
        const uint32_t base = (uint32_t)v.size(); v.insert(v.end(), c.v[s].begin(), c.v[s].end()); for (uint32_t x : c.i[s]) i.push_back(base + x); } }
}

bool actif() {
    const char* e = std::getenv("WFS_ARCHI");
    return !g_bats.empty() && !(e && !std::strcmp(e, "0"));
}

} // namespace wfs::archi::carte

namespace wfs {
using ArchiCarteSysteme = archi::carte::ArchiCarte;
WFS_SYSTEM(ArchiCarteSysteme, 110)   // monde (100-199) : après les cartes (Ville 100, Plage 105, Campagne 107), qui posent leurs bâtiments à leur init
}
