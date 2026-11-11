// WaForSpeed — décor générique des cartes de données (WFS-343, RG-03, RG-05) : le socle commun des cartes (CarteDecor, decor_carte.h :
// bandes du sol, arbres, lampadaires, chocs des objets, morceaux, LOD) joue n'importe quelle carte décrite par son paquet
// (carte_donnees.h). Rien de propre à une carte ici : ciel, sol, couleurs, maisons, arbres, lumières et obstacles viennent des blocs
// « decor » et « chocs ». Lumières : part P_TEXTE1 (inutilisée hors du Grand Prix), émission × couleur de la palette (MAT_EMTEX).
// Distance des checkpoints de la carte : système « réglages de la carte » (ordre 1, avant la course qui lit CHECK_DIST).
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include "carte_donnees.h"
#include "../decor_carte.h"
#include "../../core/configgame.h"
#include "../../core/log.h"

namespace wfs {
namespace carte {
namespace {

class CarteDonnees : public CarteDecor {
public:
    const char* name() const override { return "carte de données"; }
    void init(Renderer& r, const GameView& g) override {
        d_ = g.map ? cartes_v2::donnees(g.map->id) : nullptr;   // revue #312 : copie gardée par le décor (intacte si la carte est ré-enregistrée)
        id_ = d_ ? d_->id : std::string();
        cases_.clear();
        if (d_) {   // couleurs de la carte → cases libres de la palette (couleursCarte, appelée par CarteDecor::init)
            auto prendre = [&](uint32_t c) { if (!cases_.count(c) && C_NB + (int)cases_.size() < PAL_W) { const int k = C_NB + (int)cases_.size(); cases_[c] = k; } };
            for (const cartes_v2::Bande& b : d_->sol) prendre(b.couleur);
            for (const auto* v : {&d_->objets, &d_->obstacles}) for (const cartes_v2::Objet& o : *v) {
                if (o.type == cartes_v2::Objet::Maison || o.type == cartes_v2::Objet::Boite || o.type == cartes_v2::Objet::Cylindre) { prendre(o.couleur); prendre(o.couleur2); }
                if (o.type == cartes_v2::Objet::Lumiere) prendre(o.couleur);
            }
            for (const trace::Section& s : d_->sections) if (s.type == trace::Type::Tunnel) { prendre(BETON_TUNNEL); prendre(VOUTE_TUNNEL); break; }
        }
        CarteDecor::init(r, g);
        if (!active_) { d_ = nullptr; return; }
        // lumières (guirlandes, boules) : émission × couleur de la case (nuit : vives ; jour : douces)
        Material ml; ml.rough = 0.4f; ml.flags = MAT_DOUBLE | MAT_EMTEX; ml.emissive = vec3{1, 1, 1} * (night_ ? 3.f : 0.6f);
        mats_[P_TEXTE1] = r.makeMaterial(ml, &palTex_, nullptr, nullptr);
        WFS_LOG("cartes de données (WFS-343) : décor de %s : %zu couleurs de la carte dans la palette (%d cases libres)", id_.c_str(), cases_.size(), PAL_W - C_NB);
        preparerDecor(r);
    }

protected:
    const char* carteId() const override { return id_.empty() ? "\x01" : id_.c_str(); }   // jamais égal à un identifiant sans carte de données
    Env env() const override {
        if (!d_) return {0xd3e0d0, 820, 0x1b2236, 0x0d1218, 540};
        return {d_->brouillardJour, d_->lointainJour, d_->horizonNuit, d_->brouillardNuit, d_->lointainNuit};
    }
    void couleursCarte(std::array<uint32_t, PAL_W>& p) const override { for (const auto& [c, k] : cases_) p[(size_t)k] = c; }
    bool touffes() const override { return !d_ || d_->herbe; }
    bool solNaturel(int) const override { return true; }   // WFS-383 : toutes les bandes (seulement si la carte a sa ligne « sol »)
    int profil(const char*, int64_t, int, Band out[6]) const override {
        const float R = (float)RW;
        if (!d_ || d_->sol.empty()) { out[0] = {R, R + 400, 0, 0, C_VERGE}; return 1; }
        int n = 0;
        for (const cartes_v2::Bande& b : d_->sol) out[n++] = {b.a, b.b, b.ya, b.yb, kase(b.couleur)};
        return n;
    }
    void place(const char*, int64_t z0, int64_t z1, Mulberry32&) override {
        if (!d_) return;
        auto poser = [&](const cartes_v2::Objet& o, int indice) {
            if (o.seg < z0 || o.seg >= z1) return;
            Item it; it.side = o.lat < 0 ? -1 : 1; it.lat = std::fabs(o.lat); it.along = o.along; it.h = o.hauteur; it.col = indice;
            switch (o.type) {
            case cartes_v2::Objet::Sapin: it.t = T_TREE; it.k = K_PINE; break;
            case cartes_v2::Objet::Cypres: it.t = T_TREE; it.k = K_CYPRESS; break;
            case cartes_v2::Objet::Feuillu: it.t = T_TREE; it.k = K_ROUND; break;
            case cartes_v2::Objet::Lampe: it.t = T_LAMP; it.k = 0; break;
            case cartes_v2::Objet::Lumiere: it.t = T_LUMIERE; it.petit = true; break;
            case cartes_v2::Objet::Maison: it.t = T_MAISON; break;
            default: it.t = T_OBSTACLE; break;
            }
            item(o.seg, it);
        };
        // WFS-346 : tunnel (section « tunnel » du tracé) : piédroits et dalle de béton, un tronçon tous les PAS_TUNNEL segments ; la météo
        // (meteo.cpp, partDuCiel) ne fait tomber aucun flocon dessous
        for (const trace::Section& s : d_->sections) {
            if (s.type != trace::Type::Tunnel) continue;
            for (int64_t a = s.debut; a < s.fin; a += PAS_TUNNEL) if (a >= z0 && a < z1) { Item it; it.t = T_TUNNEL; it.l = (float)std::min<int64_t>(PAS_TUNNEL, s.fin - a); item(a, it); }
        }
        for (size_t i = 0; i < d_->objets.size(); i++) poser(d_->objets[i], (int)i);
        for (size_t i = 0; i < d_->obstacles.size(); i++) poser(d_->obstacles[i], (int)(d_->objets.size() + i));
    }
    int solide(const Item& it, chocs::ObstacleRoute o[4]) const override {
        if (const int n = CarteDecor::solide(it, o)) return n;   // arbres, lampadaires
        if (it.t != T_MAISON && it.t != T_OBSTACLE) return 0;
        const cartes_v2::Objet& b = objet(it);
        o[0] = {}; o[0].lat = (float)it.side * it.lat; o[0].along = it.along;
        if (b.type == cartes_v2::Objet::Cylindre) { o[0].r = b.rayon; o[0].genre = (chocs::Genre)b.genre; return 1; }
        o[0].forme = chocs::Forme::Boite; o[0].hx = b.largeur / 2; o[0].hz = b.profondeur / 2; o[0].angle = b.angle * PI / 180.f;
        o[0].genre = b.type == cartes_v2::Objet::Maison ? chocs::Genre::Batiment : (chocs::Genre)b.genre;
        return 1;
    }
    void dessine(Track&, const Item& it, int64_t a, const Pose& p, bool lod, Geo out[P_COUNT]) const override {
        if (!d_) return;
        if (it.t == T_TUNNEL) {   // WFS-346 : tronçon de tunnel (le long de la route : it.l segments ; + 0,3 m de recouvrement dans les courbes)
            const float L = it.l * (float)SEG_M + 0.3f, H = 5.6f, X = (float)RW + 5.0f;
            const vec3 m = p.o + p.f * (L * 0.5f - 0.15f);
            for (int c = -1; c <= 1; c += 2) box(out[P_UNI], m + p.r * (c * (X + 0.4f)) - vec3{0, 0.2f, 0}, p.r, p.f, 0.8f, H + 0.2f, L, uv(kase(BETON_TUNNEL)));
            box(out[P_UNI], m + vec3{0, H, 0}, p.r, p.f, 2 * X + 1.6f, 0.6f, L, uv(kase(VOUTE_TUNNEL)));
            return;
        }
        const cartes_v2::Objet& o = objet(it);
        const float y = solY(zone(a), a, it.side, it.lat);
        const vec3 c = lateral(p, (float)it.side * it.lat, y) + p.f * it.along;
        const float an = o.angle * PI / 180.f, ca = std::cos(an), sa = std::sin(an);
        const vec3 r = p.r * ca + p.f * sa, f = p.f * ca - p.r * sa;   // axes de l'objet tournés de son angle (degrés, sens horaire vu de dessus)
        switch (it.t) {
        case T_MAISON: {   // murs (largeur en travers de la route), toit à deux pans (faîtage le long de la route)
            box(out[P_UNI], c, r, f, o.largeur, o.hauteur, o.profondeur, uv(kase(o.couleur)));
            if (!lod || o.hauteur > 6) toit(out[P_UNI], c + vec3{0, o.hauteur, 0}, f, r, o.profondeur * 1.04f, std::max(1.2f, o.largeur * 0.35f), o.largeur * 1.08f, uv(kase(o.couleur2)));
            break;
        }
        case T_OBSTACLE:
            if (o.type == cartes_v2::Objet::Cylindre) cyl(out[P_UNI], c, o.rayon, o.hauteur, lod ? 6 : 12, uv(kase(o.couleur)));
            else box(out[P_UNI], c, r, f, o.largeur, o.hauteur, o.profondeur, uv(kase(o.couleur)));
            break;
        case T_LUMIERE: {   // petite boule (losange à 8 faces) émissive
            const float k = o.rayon;
            const vec3 m = c + vec3{0, o.hauteur, 0}, h{0, k, 0}, X = p.r * k, Z = p.f * k;
            const vec2 u = uv(kase(o.couleur));
            for (const vec3& v : {m + h, m - h}) {
                tri(out[P_TEXTE1], v, m + X, m + Z, u); tri(out[P_TEXTE1], v, m + Z, m - X, u);
                tri(out[P_TEXTE1], v, m - X, m - Z, u); tri(out[P_TEXTE1], v, m - Z, m + X, u);
            }
            break;
        }
        default: break;
        }
    }

private:
    enum { T_MAISON = T_CARTE, T_LUMIERE, T_OBSTACLE, T_TUNNEL };
    // WFS-346 : béton des tunnels (piédroits : béton brut, albédo ≈ 0,3 ; dalle : plus sombre, suie) ; un tronçon tous les 5 segments (6 m)
    static constexpr uint32_t BETON_TUNNEL = 0x96938c, VOUTE_TUNNEL = 0x55534f;
    static constexpr int64_t PAS_TUNNEL = 5;
    const cartes_v2::Objet& objet(const Item& it) const {
        const size_t i = (size_t)it.col, n = d_->objets.size();
        return i < n ? d_->objets[i] : d_->obstacles[i - n];
    }
    // case de la palette d'une couleur ; au-delà des cases libres : la plus proche parmi celles de la carte
    int kase(uint32_t c) const {
        if (auto x = cases_.find(c); x != cases_.end()) return x->second;
        int best = C_VERGE; long dmin = -1;
        for (const auto& [k, v] : cases_) {
            const long dr = (long)((k >> 16) & 255) - (long)((c >> 16) & 255), dg = (long)((k >> 8) & 255) - (long)((c >> 8) & 255), db = (long)(k & 255) - (long)(c & 255);
            const long d = dr * dr + dg * dg + db * db;
            if (dmin < 0 || d < dmin) { dmin = d; best = v; }
        }
        return best;
    }
    std::shared_ptr<const cartes_v2::DonneesCarte> d_;
    std::string id_;
    std::map<uint32_t, int> cases_;
};

// distance des checkpoints de la carte (RG-01) : CHECK_DIST est la valeur de configgame.json, sauf carte de données qui en donne une
class ReglagesCarte : public System {
public:
    const char* name() const override { return "réglages de la carte"; }
    void init(Renderer&, const GameView& g) override {
        const auto d = g.map ? cartes_v2::donnees(g.map->id) : nullptr;
        const double m = d && d->checkpointM > 0 ? d->checkpointM : config().checkpoint.distanceM;
        config().checkpoint.distanceU = m / M;
        if (d) WFS_LOG("cartes de données (WFS-343) : %s : checkpoints tous les %.0f m", d->id.c_str(), m);
    }
};

} // namespace
} // namespace carte

WFS_SYSTEM(carte::CarteDonnees, 112)   // cartes (100-199) ; active seulement sur une carte de données
WFS_SYSTEM(carte::ReglagesCarte, 1)    // avant la course (210) et le trafic (50)

} // namespace wfs
