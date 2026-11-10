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
#include "archi_cuit.h"
#include "archi_geo.h"
#include "../core/carte_cuite.h"   // WFS-416 : bloc « batiments » du paquet actif
#include "../render/vk.h"

namespace wfs::archi::carte {
namespace {

using cuit::Bati; using cuit::Lot; using cuit::NB_SLOTS; using cuit::NS; using cuit::MORCEAU;
using cuit::S_MUR; using cuit::S_TOIT; using cuit::S_ENCADREMENT; using cuit::S_VITRE; using cuit::S_MENUISERIE; using cuit::S_METAL;
using cuit::S_VOLET; using cuit::S_FIN; using cuit::S_CHASSIS; using cuit::S_IMAGE;
static_assert(sizeof(cuit::Sommet) == sizeof(Vertex) && offsetof(cuit::Sommet, tx) == offsetof(Vertex, tx), "WFS-416 : sommet cuit = Vertex");
std::vector<Bati> g_bats;
// même disposition en mémoire (vérifiée ci-dessus) : copie en bloc des sommets d'un lot vers des Vertex
std::vector<Vertex> sommets(const Lot& l, size_t debut = 0, size_t n = SIZE_MAX) {
    n = std::min(n, l.v.size() - std::min(debut, l.v.size()));
    std::vector<Vertex> v(n);
    if (n) std::memcpy((void*)v.data(), l.v.data() + debut, n * sizeof(Vertex));
    return v;
}

// WFS-416 : bâtiments cuits du paquet de la carte, lus une fois au chargement (premier besoin : occultants de la Ville, puis init)
struct Cuit { bool essaye = false, pret = false; cuit::Lu d; };
Cuit g_cuit;
bool chargerCuit() {
    if (g_cuit.essaye) return g_cuit.pret;
    g_cuit.essaye = true;
    const char* e = std::getenv("WFS_BATIMENTS_CUITS");
    const std::string c = wfs::carte::carteActive();
    std::shared_ptr<const std::vector<uint8_t>> o = (e && e[0] == '0') || c.empty() ? nullptr : wfs::carte::blocDuPaquet(c, cuit::NOM_BLOC);
    if (!o) {
        WFS_LOG("bâtiments (WFS-416) : paquet absent : kit de secours (bâtiments construits par le code)");
        return false;
    }
    const size_t taille = o->size();
    const char* raison = "";
    if (!cuit::lire(std::move(o), cuit::signature(g_bats), g_cuit.d, &raison)) {   // RG-06 : jamais de plantage, le kit reprend la main
        WFS_LOG("bâtiments (WFS-416) : %s : kit de secours (bâtiments construits par le code)", raison);
        return false;
    }
    WFS_LOG("bâtiments (WFS-416) : %zu morceaux lus dans le paquet (%.1f Mo), triangles par niveau : %zu / %zu / %zu, %zu modèles de baie, %zu images de menuiserie",
            g_cuit.d.morceaux.size(), taille / 1048576.0, g_cuit.d.triangles(0), g_cuit.d.triangles(1), g_cuit.d.triangles(2), g_cuit.d.modeles.size(), g_cuit.d.tuiles.size());
    return g_cuit.pret = true;
}
// un lot du bloc cuit, recopié en Vertex (au chargement seulement)
void lotCuit(const cuit::Vue& l, std::vector<Vertex>& v, std::vector<uint32_t>* i = nullptr) {
    v.resize(l.nv);
    if (i) i->resize(l.ni);
    g_cuit.d.copier(l, v.data(), i ? i->data() : nullptr, 0);
}

// RG-05 : niveau de détail selon la distance au joueur (m)
// WFS-402 (RG-09) : en qualité Basse, détail complet (menuiseries, volets, balcons) jusqu'à 30 m seulement : les fenêtres réelles
// ne coûtent aucune image par seconde au téléphone de l'utilisateur (banc perf, Ville de nuit, avant / après)
constexpr double LOD0_BASSE_M = 30;   // WFS-415 (RG-03) : seulement avec les ouvertures réelles (archi::ouverturesReelles)
constexpr double LOD0_M = 120, LOD1_M = 300, VUE_AVANT_M = 650, VUE_ARRIERE_M = 60;
constexpr double LOD0_CUIT_M = 20;    // WFS-416 (RG-04) : paquet cuit : menuiseries 3D jusqu'à 20 m, image avec relief au-delà
// WFS-416 : niveau 0 du paquet cuit : RESERVES tampons réservés au chargement (morceau du joueur, le suivant, le précédent, un de
// rechange pendant que la carte graphique finit ses images) ; au plus BUDGET_SOMMETS sommets tout prêts recopiés par image
constexpr int RESERVES = 4;
constexpr uint32_t BUDGET_SOMMETS = 24000;

// tampons d'un morceau du kit de secours ; murs : (bâtiment, emplacement, premier sommet, nombre) pour l'occlusion gardée
struct Cpu { Lot lot[NB_SLOTS]; struct Mur { size_t bat; int slot; size_t debut, n; }; std::vector<Mur> murs; };

// bâtiment b au niveau lod (kit de secours : construit par le code), ajouté aux tampons du morceau
void ajouter(const Bati& b, int lod, Cpu& out) {
    size_t debut = 0;
    cuit::ajouter(b, lod, Baies::Interrupteur, out.lot, &debut);
    const int s = cuit::slot(MUR, b.plan);
    out.murs.push_back({(size_t)(&b - g_bats.data()), s, debut, out.lot[s].v.size() - debut});
}
// sommets d'un emplacement d'un morceau du kit, occlusion gardée des murs appliquée (WFS-269)
std::vector<Vertex> sommetsKit(const Cpu& c, int s, int lod) {
    std::vector<Vertex> v = sommets(c.lot[s]);
    for (const Cpu::Mur& m : c.murs) if (m.slot == s) {
        std::vector<Vertex> t(v.begin() + (long)m.debut, v.begin() + (long)(m.debut + m.n));
        precalcul::appliquerKit(m.bat, lod, t, 0);
        std::copy(t.begin(), t.end(), v.begin() + (long)m.debut);
    }
    return v;
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
        mats_[S_MENUISERIE] = mat(cuit::COUL_MENUISERIE, 1, 0.6f, 0);
        mats_[S_METAL] = mat(ALB_ALUMINIUM, cuit::K_METAL, 0.4f, 1);
        mats_[S_FIN] = mat(ALB_ALUMINIUM, cuit::K_METAL, 0.4f, 1, MAT_DOUBLE | MAT_BLEND);   // WFS-370 : barreaux, couverture partielle (scene.frag, detailK)
        mats_[S_VOLET] = mat(cuit::COUL_VOLET, 1, 0.6f, 0);
        mats_[S_CHASSIS] = mat(cuit::COUL_CHASSIS, 1, 0.45f, 0);   // WFS-402 : menuiseries de bois peint
        {   // vitres + pièces de nuit (ST-3) : lampe 2850 K, murs 31,8 cd/m² (E = 200 lx, ρ = 0,5 : conception), 35 % allumées la nuit
            double c[3]; lumiere::couleurTemperature(2850, c);
            const float L = (float)(0.5 * 200.0 / lumiere::PI_L);
            Material m; m.color = {0.03f, 0.035f, 0.04f, 1}; m.rough = 0.05f; m.flags = MAT_DOUBLE | MAT_INTERIEUR;
            m.emissive = {(float)c[0] * L, (float)c[1] * L, (float)c[2] * L}; m.cutoff = nuit ? 0.35f : 0.f;
            mats_[S_VITRE] = r.makeMaterial(m, nullptr, nullptr, nullptr);
        }
        pret_ = true;
        if (chargerCuit()) { initCuit(r, g); return; }
        precalcul::occlusionKit(g_bats.size(), [](size_t k, int lod, std::vector<Vertex>& v) {   // WFS-269 : murs, au chargement
            Cpu c; cuit::ajouter(g_bats[k], lod, Baies::Interrupteur, c.lot); v = sommets(c.lot[cuit::slot(MUR, g_bats[k].plan)]); });
        size_t tri = 0;
        for (const Bati& b : g_bats) { Batiment x; construire(b.plan, b.profondeur, 0, x); for (const Maillage& m : x.part) tri += m.triangles(); }
        WFS_LOG("archi (WFS-161) : %zu bâtiments du kit posés, %zu triangles au niveau de détail 0 (tous), nuit %s", g_bats.size(), tri, nuit ? "oui" : "non");
    }

    void render(Renderer& r, const GameView& g) override {
        if (!pret_ || g.sceneDediee || !g.player || !g.track) return;
        frame_++;
        const int64_t L = std::max<int64_t>(1, (int64_t)g.track->segmentCount());
        const int64_t aP = ((int64_t)std::floor(g.player->dist / SEG) % L + L) % L;   // tours suivants : même plan (limite : le raccord du tour)
        // WFS-415 (RG-03) : la limite de 30 m en Basse ne vaut qu'avec les ouvertures réelles de WFS-402 ; interrupteur à 0
        // (jeu) : 120 m comme avant WFS-402, les morceaux ne sont plus reconstruits sans arrêt en roulant
        const bool lod0Basse = archi::ouverturesReelles();
        const double lod0 = cuit_ ? LOD0_CUIT_M : (r.qualiteBasse && lod0Basse ? LOD0_BASSE_M : LOD0_M);
        auto lodVoulu = [&](int64_t c) {   // distance du joueur au morceau (m), au plus près
            const double d0 = (double)(c * MORCEAU - aP) * SEG_M, d1 = d0 + MORCEAU * SEG_M;
            const double d = d0 > 0 ? d0 : (d1 < 0 ? -d1 : 0);
            return d < lod0 ? 0 : d < LOD1_M ? 1 : 2;
        };
        const int64_t c0 = (aP - (int64_t)(VUE_ARRIERE_M / SEG_M)) / MORCEAU, c1 = (aP + (int64_t)(VUE_AVANT_M / SEG_M)) / MORCEAU;
        const mat4 I = mat4::identity();
        if (cuit_) {   // WFS-416 (RG-03) : rien n'est construit ni créé : on choisit le morceau et le niveau
            suivreNiveau0(aP / MORCEAU, BUDGET_SOMMETS);
            // vitres envoyées en dernier (WFS-402, RG-09) : menuiseries, images et volets posés devant cachent la pièce de nuit
            for (int passe = 0; passe < 2; passe++)
                for (size_t k = 0; k < cuits_.size(); k++) {
                    const MorceauCuit& m = cuits_[k];
                    if (m.c < c0 || m.c > c1) continue;
                    const int lod = lodVoulu(m.c);
                    Reserve* res = lod == 0 ? reserveDe(k) : nullptr;   // niveau 0 pas encore recopié : image avec relief en attendant
                    if (res) res->dernierDessin = frame_;
                    const std::array<Mesh, NB_SLOTS>& lot = res ? res->m : m.m[(size_t)std::max(lod, 1) - 1];
                    for (int s = 0; s < NB_SLOTS; s++)
                        if ((s == S_VITRE) == (passe == 1) && lot[(size_t)s].indexCount) r.draw(lot[(size_t)s], mats_[s], I, s == S_FIN ? 0.016f : 0.f);
                }
            return;
        }
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
                for (int i = b; i < e; i++) for (const Bati& x : g_bats) if (x.seg / MORCEAU == manque[i]) ajouter(x, lodVoulu(manque[i]), cpu[i]);
            });
            for (size_t i = 0; i < manque.size(); i++) {
                Morceau& m = morceaux_[manque[i]];
                m.lod = lodVoulu(manque[i]);
                for (int s = 0; s < NB_SLOTS; s++) if (!cpu[i].lot[s].i.empty()) m.m[s] = r.makeMesh(sommetsKit(cpu[i], s, m.lod), cpu[i].lot[s].i, true);
            }
        }
        for (auto it = cimetiere_.begin(); it != cimetiere_.end();) {
            if (frame_ > it->first + 3) { r.destroy(it->second); it = cimetiere_.erase(it); } else ++it;
        }
        // WFS-402 (RG-09) : vitres envoyées en dernier (même profondeur de tri pour tous les morceaux : l'ordre d'envoi est gardé) :
        // les menuiseries et les volets fermés posés devant cachent la pièce de nuit, dont les pixels ne sont alors plus calculés
        for (int passe = 0; passe < 2; passe++)
            for (auto& [c, m] : morceaux_) for (int s = 0; s < NB_SLOTS; s++)
                if ((s == S_VITRE) == (passe == 1) && m.m[s].indexCount) r.draw(m.m[s], mats_[s], I, s == S_FIN ? 0.016f : 0.f);   // WFS-370 : épaisseur des barreaux
    }

    void shutdown(Renderer& r) override {
        for (auto& [c, m] : morceaux_) for (Mesh& x : m.m) if (x.indexCount) r.destroy(x);
        for (auto& x : cimetiere_) r.destroy(x.second);
        for (MorceauCuit& m : cuits_) for (auto& lot : m.m) for (Mesh& x : lot) if (x.indexCount) r.destroy(x);
        for (Reserve& x : reserves_) for (Mesh& y : x.m) if (y.vb.buf) r.destroy(y);
        if (pret_) for (Material& m : mats_) r.destroy(m);
        for (Image* im : {&atlasCouleur_, &atlasRelief_}) if (im->img) { r.vk().destroy(*im); *im = Image{}; }
        morceaux_.clear(); cimetiere_.clear(); cuits_.clear(); murs0_.clear();
        for (Reserve& x : reserves_) x = Reserve{};
        pret_ = cuit_ = false;
        vider();   // la carte suivante pose ses propres bâtiments (ou aucun : anciennes boîtes)
    }

private:
    // WFS-416 (RG-03) : au chargement, les niveaux 1 et 2 de tous les morceaux vont dans la carte graphique (maillages fixes) et les
    // tampons du niveau 0 sont réservés ; l'occlusion précalculée des murs (WFS-269) est appliquée ici, comme le faisait le kit.
    // Ensuite plus rien n'est construit ni créé : le niveau 0 des morceaux proches est recopié du paquet dans les tampons réservés.
    void initCuit(Renderer& r, const GameView& g) {
        const cuit::Lu& d = g_cuit.d;
        atlasCouleur_ = r.vk().uploadTexture(d.octets->data() + d.posCouleur, d.atlasL, d.atlasH, true);
        atlasRelief_ = r.vk().uploadTexture(d.octets->data() + d.posRelief, d.atlasL, d.atlasH, false);
        {   // RG-04 : menuiserie en image avec relief : couleur × atlas, vitrage découpé (alpha), relief par la carte de normales
            Material m; m.color = {1, 1, 1, 1}; m.rough = 0.5f; m.metal = 0; m.flags = MAT_DOUBLE | MAT_MASK; m.cutoff = 0.5f;
            mats_[S_IMAGE] = r.makeMaterial(m, &atlasCouleur_, &atlasRelief_, nullptr);
        }
        // occlusion des murs (niveaux 0 et 1) : un « bâtiment » de l'occlusion = les murs d'un style d'un morceau
        precalcul::occlusionKit(d.morceaux.size() * (size_t)NS, [&d](size_t k, int lod, std::vector<Vertex>& v) {
            lotCuit(d.morceaux[k / (size_t)NS].lot[lod][S_MUR + (int)(k % (size_t)NS)], v); });
        size_t fixes = 0, reserves = 0;
        cuits_.resize(d.morceaux.size()); murs0_.resize(d.morceaux.size());
        std::vector<Vertex> v; std::vector<uint32_t> i;
        for (size_t k = 0; k < d.morceaux.size(); k++) {
            cuits_[k].c = d.morceaux[k].c;
            for (int n = 1; n < cuit::NIVEAUX; n++) for (int s = 0; s < NB_SLOTS; s++) {
                const cuit::Vue& l = d.morceaux[k].lot[n][s];
                if (!l.ni) continue;
                lotCuit(l, v, &i);
                if (s < NS && n == 1) precalcul::appliquerKit(k * (size_t)NS + (size_t)s, 1, v, 0);
                cuits_[k].m[(size_t)n - 1][(size_t)s] = r.makeMesh(v, i, false);
                fixes += v.size() * sizeof(Vertex) + i.size() * sizeof(uint32_t);
            }
            for (int s = 0; s < NS; s++) {   // murs du niveau 0, occlusion appliquée, gardés en mémoire vive (recopiés tels quels en course)
                lotCuit(d.morceaux[k].lot[0][S_MUR + s], murs0_[k][(size_t)s]);
                precalcul::appliquerKit(k * (size_t)NS + (size_t)s, 0, murs0_[k][(size_t)s], 0);
            }
        }
        for (Reserve& x : reserves_) for (int s = 0; s < NB_SLOTS; s++) {
            if (!d.niMax[s]) continue;
            v.assign(d.nvMax[s], Vertex{}); i.assign(d.niMax[s], 0);
            x.m[(size_t)s] = r.makeMesh(v, i, true);
            x.m[(size_t)s].indexCount = 0;
            reserves += v.size() * sizeof(Vertex) + i.size() * sizeof(uint32_t);
        }
        cuit_ = true;
        if (g.player) {   // morceaux proches du départ : recopiés en entier ici, pendant le chargement
            const int64_t L = std::max<int64_t>(1, (int64_t)g.track->segmentCount());
            suivreNiveau0((((int64_t)std::floor(g.player->dist / SEG) % L + L) % L) / MORCEAU, UINT32_MAX);
        }
        WFS_LOG("bâtiments (WFS-416) : %zu morceaux prêts dans la carte graphique : niveaux 1 et 2 fixes %.1f Mo, %d tampons du niveau 0 %.1f Mo, atlas %ux%u ; aucune construction en course",
                cuits_.size(), fixes / 1048576.0, RESERVES, reserves / 1048576.0, d.atlasL, d.atlasH);
    }

    // ---- niveau 0 du paquet cuit : tampons réservés, remplis par recopie (aucune allocation, aucune construction)
    struct Reserve {
        size_t k = SIZE_MAX;             // morceau (indice dans cuits_) ; SIZE_MAX : libre
        bool pret = false;
        uint64_t dernierDessin = 0;
        std::array<Mesh, NB_SLOTS> m{};
        int etape = 0; size_t suivant = 0;   // reprise : 0 gros œuvre (emplacement suivant), 1 instances (instance suivante)
        uint32_t nv[NB_SLOTS]{}, ni[NB_SLOTS]{};
    };
    Reserve* reserveDe(size_t k) { for (Reserve& x : reserves_) if (x.k == k && x.pret) return &x; return nullptr; }
    size_t indiceMorceau(int64_t c) const {
        const auto it = std::lower_bound(cuits_.begin(), cuits_.end(), c, [](const MorceauCuit& m, int64_t x) { return m.c < x; });
        return it != cuits_.end() && it->c == c ? (size_t)(it - cuits_.begin()) : SIZE_MAX;
    }
    // recopie une tranche du niveau 0 du morceau de la réserve (au plus « budget » sommets) ; vrai quand le morceau est complet
    bool avancer(Reserve& x, uint32_t budget) {
        const cuit::Lu& d = g_cuit.d;
        const cuit::MorceauLu& m = d.morceaux[x.k];
        auto vb = [&](int s) { return (uint8_t*)x.m[(size_t)s].vb.mapped + (size_t)x.nv[s] * sizeof(Vertex); };
        auto ib = [&](int s) { return (uint32_t*)x.m[(size_t)s].ib.mapped + x.ni[s]; };
        uint32_t fait = 0;
        if (x.etape == 0) {
            for (; x.suivant < (size_t)NB_SLOTS; x.suivant++) {
                if (fait >= budget) return false;
                const int s = (int)x.suivant;
                const cuit::Vue& l = m.lot[0][s];
                if (!l.ni) continue;
                if (s < NS) { std::memcpy(vb(s), murs0_[x.k][(size_t)s].data(), (size_t)l.nv * sizeof(Vertex)); d.copier(l, nullptr, ib(s), x.nv[s]); }   // murs : occlusion déjà appliquée
                else d.copier(l, vb(s), ib(s), x.nv[s]);
                x.nv[s] += l.nv; x.ni[s] += l.ni; fait += l.nv;
            }
            x.etape = 1; x.suivant = 0;
        }
        for (; x.suivant < m.nbInstances; x.suivant++) {
            if (fait >= budget) return false;
            const cuit::Instance& in = d.instances[m.premiereInstance + x.suivant];
            for (int s = 0; s < NB_SLOTS; s++) {
                if (!d.modeles[in.modele][(size_t)s].ni) continue;
                const cuit::Vue l = d.developper(in, s, vb(s), ib(s), x.nv[s]);
                x.nv[s] += l.nv; x.ni[s] += l.ni; fait += l.nv;
            }
        }
        for (int s = 0; s < NB_SLOTS; s++) { Mesh& me = x.m[(size_t)s]; me.indexCount = x.ni[s]; me.centre = m.centre; me.rayon = m.rayon; }
        return x.pret = true;
    }
    // garde prêts le morceau du joueur, le suivant et le précédent ; un seul morceau en cours de recopie à la fois
    void suivreNiveau0(int64_t c, uint32_t budget) {
        const size_t voulus[3] = {indiceMorceau(c), indiceMorceau(c + 1), indiceMorceau(c - 1)};
        for (size_t k : voulus) {
            if (k == SIZE_MAX) continue;
            Reserve* res = nullptr;
            for (Reserve& x : reserves_) if (x.k == k) res = &x;
            if (!res) {   // tampon libre : ni voulu, ni encore lu par une image en vol (3 images après son dernier dessin)
                for (Reserve& x : reserves_) {
                    const bool voulu = x.k != SIZE_MAX && (x.k == voulus[0] || x.k == voulus[1] || x.k == voulus[2]);
                    if (!voulu && (x.k == SIZE_MAX || frame_ > x.dernierDessin + 3)) { res = &x; break; }
                }
                if (!res) return;
                for (Mesh& me : res->m) me.indexCount = 0;
                res->k = k; res->pret = false; res->etape = 0; res->suivant = 0;
                std::fill(std::begin(res->nv), std::end(res->nv), 0u); std::fill(std::begin(res->ni), std::end(res->ni), 0u);
            }
            if (res->pret) continue;
            if (!avancer(*res, budget)) return;   // la suite à l'image suivante
            if (budget != UINT32_MAX) return;     // en course : un morceau terminé par image au plus
        }
    }

    struct Morceau { int lod = 0; std::array<Mesh, NB_SLOTS> m{}; };
    struct MorceauCuit { int64_t c = 0; std::array<std::array<Mesh, NB_SLOTS>, 2> m{}; };   // niveaux 1 et 2 (fixes)
    bool parMorceau(int64_t c) const { for (const Bati& b : g_bats) if (b.seg / MORCEAU == c) return true; return false; }
    bool pret_ = false, cuit_ = false;
    uint64_t frame_ = 0;
    std::map<int64_t, Morceau> morceaux_;
    std::vector<std::pair<uint64_t, Mesh>> cimetiere_;
    std::vector<MorceauCuit> cuits_;
    std::vector<std::array<std::vector<Vertex>, NS>> murs0_;
    Reserve reserves_[RESERVES];
    Image atlasCouleur_{}, atlasRelief_{};
    Material mats_[NB_SLOTS]{};
};

} // namespace

void vider() { g_bats.clear(); g_cuit = Cuit{}; }

void poser(int64_t seg, vec3 origine, vec3 x, vec3 z, float largeur, float profondeur, float hauteur, Style style, uint32_t graine) {
    g_bats.push_back({seg, origine, x, z, profondeur, assembler(style, largeur, hauteur, graine)});
}

const std::vector<cuit::Bati>& poses() { return g_bats; }

void geometrie(int lod, std::vector<Vertex>& v, std::vector<uint32_t>& i) {   // WFS-269 (accroche) : voir archi_carte.h
    auto verser = [&](const Lot& l) {
        const uint32_t base = (uint32_t)v.size();
        const std::vector<Vertex> s = sommets(l);
        v.insert(v.end(), s.begin(), s.end()); for (uint32_t x : l.i) i.push_back(base + x);
    };
    if (chargerCuit()) {   // WFS-416 : géométrie cuite du paquet (l'image des menuiseries, découpée, ne cache pas le ciel)
        std::vector<Vertex> lv; std::vector<uint32_t> li;
        for (const cuit::MorceauLu& m : g_cuit.d.morceaux) for (int s = 0; s < NB_SLOTS; s++) {
            if (s == S_IMAGE) continue;
            lotCuit(m.lot[std::clamp(lod, 1, 2)][s], lv, &li);   // niveau 0 demandé : le niveau 1 (mêmes volumes, sans les menuiseries)
            const uint32_t base = (uint32_t)v.size();
            v.insert(v.end(), lv.begin(), lv.end()); for (uint32_t x : li) i.push_back(base + x);
        }
        return;
    }
    for (const Bati& b : g_bats) { Cpu c; cuit::ajouter(b, lod, Baies::Interrupteur, c.lot); for (int s = 0; s < NB_SLOTS; s++) verser(c.lot[s]); }
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
