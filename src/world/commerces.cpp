// WaForSpeed — commerces et terrasses de la ville, serviettes de la Corniche (WFS-71) : port de js/world/life.js
// (SHOPS, DECOR.shop, placeShop, terraceTables, DECOR.towel) et de beachLife (js/world/decor.js) du jeu web.
// Les commerces et leurs clients attablés viennent du plan des quartiers de la ville (src/world/ville_quartiers.*,
// placeShop de city.centre / residentiel, WFS-93), vitrine centrée sur la façade de l'immeuble ; la graine de la carte
// fixe tout : identique à chaque partie (RG-01).
// Dessin par morceaux de 4 × CH segments (≈ 150 m), autour du joueur seulement : quelques appels de dessin par morceau
// (palette unie, vitrines, jour / nuit, textes des enseignes par couleur) (RG-05).
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/game.h"
#include "../game/course_system.h"
#include "../game/player.h"
#include "../game/trace_cartes.h"   // WFS-139 (accroche)
#include "../game/track.h"
#include "enseignes.h"
#include "ville_quartiers.h"
#include "zones.h"

namespace wfs {
namespace plage { bool occupe(const MapDef& map, int64_t len, int64_t n); }   // WFS-180 (plage.cpp)
namespace vie {
extern double g_premiereTerrasse;
namespace {

// SHOPS de js/world/life.js (nom, fond, texte, store à rayures, terrasse)
struct Shop { const char* nom; unsigned bg, fg, aw0, aw1; bool terrasse; };
const Shop SHOPS[] = {
    {"CAFÉ DE LA GARE", 0x4a2a18, 0xf6e6c8, 0x7a1f1f, 0xf2e6cc, true}, {"RESTAURANT", 0x7a1414, 0xffffff, 0x7a1414, 0xf4efe6, true},
    {"BAR LE NÉON", 0x15142a, 0xff4fa3, 0x15142a, 0x3a2a6a, true}, {"PIZZERIA ROMA", 0x1f5a2c, 0xffffff, 0x1f5a2c, 0xf4efe6, true},
    {"BOULANGERIE", 0xc99a4a, 0x3a2410, 0xc99a4a, 0xf6e8c8, false}, {"BRASSERIE", 0x173450, 0xf2d48a, 0x173450, 0xd9c79a, true},
    {"GLACIER", 0xf39bb8, 0xffffff, 0xf39bb8, 0xffffff, true}, {"SUSHI BAR", 0x101010, 0xff3b30, 0x101010, 0xa01818, true},
    {"PHARMACIE", 0x0f7a3c, 0xffffff, 0x0f7a3c, 0xe9f3ec, false}, {"BEACH BAR", 0x1d8fb8, 0xffe9a8, 0x1d8fb8, 0xffe9a8, true},
};
constexpr int NB_SHOPS = 10;
// couleurs des serviettes (C2.flagC du jeu web : drapeaux vifs)
const unsigned SERVIETTES[6] = {0xe2262b, 0x1d6fb8, 0xffd23a, 0x2f8f4f, 0xff6a00, 0xf4f4f0};

using Commerce = ville::Commerce;
// terraceTables(lenM) : position des tables le long de la terrasse
std::vector<float> tables(float len) { std::vector<float> out; for (float d = 1.4f; d < len - 1; d += 2.6f) out.push_back(d); return out; }

// commerces du plan de la ville (city.centre : D() < 0,5 ; city.residentiel : D() < 0,4), devant un immeuble
const std::vector<Commerce>& commerces(const MapDef& map, int64_t len) { return ville::plan(map, len).commerces; }
float sable(float lat) {   // GROUND.beach (corniche, côté mer) : sable de RW + 5 (0,05 m) à RW + 38 (−1,1 m)
    const float R = (float)RW, t = std::clamp((lat - (R + 5)) / 33.f, 0.f, 1.f);
    return 0.05f + (-1.1f - 0.05f) * t;
}
} // namespace

const std::vector<Client>& clientsTerrasses(const MapDef& map, int64_t len) {
    static std::map<std::pair<uint32_t, int64_t>, std::vector<Client>> cache;
    auto& out = cache[{map.seed, len}];
    if (!out.empty()) return out;
    const float face = (float)RW + 6.45f, tLat = face - 2.2f;
    for (const Commerce& c : commerces(map, len)) {
        if (!SHOPS[c.kind].terrasse) continue;
        const double debut = c.seg + c.decal / SEG_M;   // vitrine centrée sur la façade de l'immeuble
        int k = 0;   // placeShop : bit 2 · table + (sd > 0) = chaise occupée (tirage D() < 0,65 du plan)
        for (float d : tables(c.len)) {
            for (int sd : {-1, 1}) if (c.clients >> (k * 2 + (sd > 0 ? 1 : 0)) & 1u)
                out.push_back({debut + (d + sd * 0.62) / SEG_M, c.side * tLat, sd > 0 ? 0.f : PI, 0.15f + 0.05f});
            k++;
        }
    }
    return out;
}
// WFS-139 (accroche) : au tracé 2, la plage n'est qu'au bord de l'eau : ni lacets ni falaise (route à plus de 3 m), ni tunnel,
// rond-point ou carrefour (serviettes et baigneurs tombaient sur les terrasses et les autres branches des lacets)
static bool horsDeLaPlage(const MapDef& map, int64_t len, int64_t n) {
    static std::map<std::pair<uint32_t, int64_t>, Track> pistes;
    Track& t = pistes[{map.seed, len}]; if (!t.segmentCount()) t.build(map, trace::versionCarte(map.id));
    const trace::Section* s = trace::sectionA(t, (double)n * SEG);
    return plage::occupe(map, len, n) || (s && (t.pose(n).y > 3 || s->type == trace::Type::Lacets || s->type == trace::Type::Tunnel || s->type == trace::Type::RondPoint || s->type == trace::Type::Feux));   // WFS-180 : ni sur une fête ni sur les jeux
}
const std::vector<Serviette>& serviettes(const MapDef& map, int64_t len) {
    static std::map<std::pair<uint32_t, int64_t>, std::vector<Serviette>> cache;
    auto& out = cache[{map.seed, len}];
    if (!out.empty() || std::strcmp(map.id, "plage")) return out;
    Mulberry32 D(map.seed * 31 + 29);
    for (int64_t n = 2; n < len - 4; n += 4 + (int64_t)(D.next() * 5)) {
        if (std::strcmp(zoneId(map, n, len), "corniche")) continue;
        if (D.next() >= 0.7 || horsDeLaPlage(map, len, n)) continue;
        const float lat = (float)RW + 10 + (float)D.next() * 26, rot = ((float)D.next() - 0.5f) * 0.6f;
        out.push_back({(double)n, lat, rot, (int)(D.next() * 6)});
    }
    return out;
}
const std::vector<Baigneur>& baigneurs(const MapDef& map, int64_t len) {
    static std::map<std::pair<uint32_t, int64_t>, std::vector<Baigneur>> cache;
    auto& out = cache[{map.seed, len}];
    if (!out.empty() || std::strcmp(map.id, "plage")) return out;
    Mulberry32 D(map.seed * 31 + 31);
    for (int64_t n = 4; n < len - 4; n += 5 + (int64_t)(D.next() * 8)) {
        if (std::strcmp(zoneId(map, n, len), "corniche")) continue;
        if (D.next() >= 0.6 || horsDeLaPlage(map, len, n)) continue;
        out.push_back({n + D.next() * 3, (float)RW + 47 + (float)D.next() * 12, (float)D.next() * 6});
    }
    return out;
}
float hauteurSable(float lat) { return sable(lat); }
double g_premiereTerrasse = -1;   // banc d'essai : abscisse (segments) du premier client attablé de la carte courante

} // namespace vie

namespace {
using namespace vie;
constexpr int GROUPE = 4 * CH;   // segments par morceau
constexpr int AVANT = 3, ARRIERE = 1;

class Commerces : public System {
public:
    const char* name() const override { return "commerces"; }
    void init(Renderer& r, const GameView& g) override {
        actif_ = false;
        if (!g.map || !g.track) return;
        ville_ = !std::strcmp(g.map->id, "ville"); plage_ = !std::strcmp(g.map->id, "plage");
        if (!ville_ && !plage_) return;
        len_ = (int64_t)g.track->segmentCount();
        if (ville_ && commerces(*g.map, len_).empty()) return;
        map_ = g.map;
        for (const Shop& s : SHOPS) { atlas_.ajouter(s.nom); for (unsigned c : {s.bg, s.aw0, s.aw1}) palette_.uv(c); }
        for (unsigned c : {0x2a2d33u, 0x3b4a58u, 0x2a2c30u, 0xe9e4dau, 0x3a3d44u, 0xd8d8d8u, 0x5a4a3au, 0x3f6f2au}) palette_.uv(c);
        for (unsigned c : SERVIETTES) palette_.uv(c);
        if (ville_ && !atlas_.construire(r)) return;
        palette_.construire(r);
        vitreNuit_ = neon(r, 0xffcf8a, 1.4f); bougie_ = neon(r, 0xffd9a0, 3.f);
        Material mv; const vec3 c = lin(0x3b4a58); mv.color = {c.x, c.y, c.z, 1}; mv.rough = 0.15f; mv.metal = 0.3f;
        vitreJour_ = r.makeMaterial(mv, nullptr, nullptr, nullptr);
        actif_ = true;
        g_premiereTerrasse = -1;
        if (ville_) for (const Client& cl : clientsTerrasses(*g.map, len_))   // plus proche du départ, côté droit
            if (cl.s > 40 && cl.lat > 0 && (g_premiereTerrasse < 0 || cl.s < g_premiereTerrasse)) g_premiereTerrasse = cl.s;
        WFS_LOG("vie : %zu commerces, %zu clients attablés, %zu serviettes (%s)", ville_ ? commerces(*g.map, len_).size() : 0,
                ville_ ? clientsTerrasses(*g.map, len_).size() : 0, plage_ ? serviettes(*g.map, len_).size() : 0, g.map->id);
    }
    void render(Renderer& r, const GameView& g) override {
        if (!actif_ || g.sceneDediee || !g.player) return;
        frame_++;
        Track& t = *g.track;
        const int64_t aP = (int64_t)std::floor(g.player->dist / SEG), pc = aP / GROUPE;
        for (auto it = morceaux_.begin(); it != morceaux_.end();) {
            if (it->first < pc - ARRIERE || it->first > pc + AVANT) { cimetiere(it->second); it = morceaux_.erase(it); } else ++it;
        }
        for (int64_t c = pc - ARRIERE; c <= pc + AVANT; c++) if (!morceaux_.count(c)) construire(r, t, c);
        for (auto it = graveyard_.begin(); it != graveyard_.end();) {
            if (frame_ > it->first + 3) { r.destroy(it->second); it = graveyard_.erase(it); } else ++it;
        }
        const mat4 I = mat4::identity();
        for (auto& [k, m] : morceaux_) {
            if (m.pal.indexCount) r.draw(m.pal, palette_.materiau, I);
            if (m.vitre.indexCount) r.draw(m.vitre, g.night ? vitreNuit_ : vitreJour_, I);
            if (g.night ? m.nuit.indexCount : m.jour.indexCount) r.draw(g.night ? m.nuit : m.jour, g.night ? bougie_ : palette_.materiau, I);
            for (auto& [coul, me] : m.textes) if (me.indexCount) r.draw(me, atlas_.materiau(r, coul, g.night ? 1.4f : 0.6f), I);
        }
    }
    void shutdown(Renderer& r) override {
        if (!actif_) return;
        r.vk().waitIdle();
        for (auto& [k, m] : morceaux_) cimetiere(m);
        morceaux_.clear();
        for (auto& gm : graveyard_) r.destroy(gm.second);
        graveyard_.clear();
        atlas_.detruire(r); palette_.detruire(r);
        for (Material* m : {&vitreJour_, &vitreNuit_, &bougie_}) r.destroy(*m);   // WFS-187 (accroche) : jeux de descripteurs rendus
        actif_ = false;
    }
private:
    struct Morceau { Mesh pal, vitre, jour, nuit; std::map<unsigned, Mesh> textes; };
    void cimetiere(Morceau& m) {
        for (Mesh* x : {&m.pal, &m.vitre, &m.jour, &m.nuit}) if (x->indexCount) graveyard_.push_back({frame_, *x});
        for (auto& [c, x] : m.textes) if (x.indexCount) graveyard_.push_back({frame_, x});
    }
    // DECOR.shop (life.js) : vitrine, lumière de vitrine, store à rayures, lambrequin, enseigne, terrasse
    void commerce(Track& t, const Commerce& c, int64_t abs, Geo& pal, Geo& vitre, Geo& jour, Geo& nuit, std::map<unsigned, Geo>& textes) {
        const Shop& sh = SHOPS[c.kind];
        const float s = (float)c.side, face = (float)RW + 6.45f, L = c.len;
        const double debut = abs + c.decal / SEG_M;   // vitrine centrée sur la façade de l'immeuble
        const Repere pm = repere(t, (debut + L / 2 / SEG_M) * SEG);
        const vec2 aw0 = palette_.uv(sh.aw0), aw1 = palette_.uv(sh.aw1), bg = palette_.uv(sh.bg);
        boite(pal, lateral(pm, s * (face - 0.06f), 0.15f), pm.r, pm.f, 0.08f, 3.1f, L * 0.9f, palette_.uv(0x2a2d33));
        boite(vitre, lateral(pm, s * (face - 0.11f), 0.45f), pm.r, pm.f, 0.04f, 2.2f, L * 0.78f, {0, 0});
        const int n = std::max(3, (int)std::lround(L / 0.9f));
        const float w = L * 0.92f / n;
        for (int k = 0; k < n; k++)
            boite(pal, lateral(pm, s * (face - 0.8f), 3.0f) + pm.f * (-L * 0.46f + w * (k + 0.5f)), pm.r, pm.f, 1.6f, 0.06f, w * 0.98f, k % 2 ? aw1 : aw0);
        boite(pal, lateral(pm, s * (face - 1.62f), 2.78f), pm.r, pm.f, 0.05f, 0.28f, L * 0.92f, aw0);
        // enseigne : fond de la couleur du commerce, nom en lettres de la couleur du texte (face à la rue)
        const float sw = std::min(L * 0.7f, 6.5f);
        boite(pal, lateral(pm, s * (face - 0.14f), 3.35f), pm.r, pm.f, 0.06f, 0.8f, sw, bg);
        const Texte& T = atlas_.texte(sh.nom);
        const float th = 0.5f, tw = std::min(sw * 0.92f, th * T.aspect);
        const vec3 versRue = pm.r * -s;
        plan(textes[sh.fg], lateral(pm, s * (face - 0.18f), 3.75f), pm.f * -s, {0, 1, 0}, versRue, tw, th * std::min(1.f, tw / (th * T.aspect)), T.u0, T.v0, T.u1, T.v1);
        if (!sh.terrasse) return;
        const float tLat = s * (face - 2.2f);
        for (float d : tables(L)) {
            const Repere pa = repere(t, (debut + d / SEG_M) * SEG);
            const vec3 b = lateral(pa, tLat, 0.15f);
            cylindre(pal, b, 0.04f, 0.72f, palette_.uv(0x2a2c30), 6);
            cylindre(pal, b + vec3{0, 0.72f, 0}, 0.38f, 0.04f, palette_.uv(0xe9e4da), 12);
            for (int sd : {-1, 1}) {
                const vec3 cp = b + pa.f * (sd * 0.62f);
                boite(pal, cp, pa.r, pa.f, 0.42f, 0.45f, 0.42f, palette_.uv(0x3a3d44));
                boite(pal, cp + pa.f * (sd * 0.19f) + vec3{0, 0.45f, 0}, pa.r, pa.f, 0.42f, 0.45f, 0.05f, palette_.uv(0x3a3d44));
            }
            // jour : parasol ; nuit : bougie sur la table
            cylindre(jour, b, 0.03f, 2.3f, palette_.uv(0xd8d8d8), 6);
            cone(jour, b + vec3{0, 2.0f, 0}, 1.35f, 0.45f, std::fmod(d, 5.f) < 2.5f ? aw0 : aw1, 10);
            boite(nuit, b + vec3{0, 0.76f, 0}, pa.r, pa.f, 0.07f, 0.1f, 0.07f, {0, 0});
        }
        for (float d : {0.3f, L - 0.3f}) {   // pots de fleurs aux deux bouts
            const vec3 b = lateral(repere(t, (debut + d / SEG_M) * SEG), tLat, 0.15f);
            cylindre(pal, b, 0.25f, 0.5f, palette_.uv(0x5a4a3a), 8);
            boite(pal, b + vec3{0, 0.5f, 0}, {1, 0, 0}, {0, 0, 1}, 0.6f, 0.5f, 0.6f, palette_.uv(0x3f6f2a));
        }
    }
    void construire(Renderer& r, Track& t, int64_t c) {
        Geo pal, vitre, jour, nuit; std::map<unsigned, Geo> textes;
        const int64_t a0 = c * GROUPE, a1 = a0 + GROUPE;
        const int64_t lap = (int64_t)std::floor((double)a0 / len_) * len_;
        auto dans = [&](int64_t seg, int64_t& abs) {
            for (int64_t L : {lap, lap + len_}) { const int64_t a = seg + L; if (a >= a0 && a < a1) { abs = a; return true; } }
            return false;
        };
        if (ville_) for (const Commerce& co : commerces(*map_, len_)) { int64_t a; if (dans(co.seg, a)) commerce(t, co, a, pal, vitre, jour, nuit, textes); }
        if (plage_) for (const Serviette& sv : serviettes(*map_, len_)) {   // DECOR.towel : 0,9 × 1,9 m, tournée de rot
            int64_t a; if (!dans((int64_t)sv.s, a)) continue;
            const Repere p = repere(t, a * SEG);
            const float cr = std::cos(sv.rot), sr = std::sin(sv.rot);
            const vec3 r2 = p.r * cr + p.f * sr, f2 = p.f * cr - p.r * sr;
            boite(pal, lateral(p, sv.lat, hauteurSable(sv.lat) + 0.01f), r2, f2, 0.9f, 0.02f, 1.9f, palette_.uv(SERVIETTES[sv.couleur]));
        }
        Morceau& m = morceaux_[c];
        auto mk = [&](Geo& gg, Mesh& me) { if (!gg.i.empty()) me = r.makeMesh(gg.v, gg.i); };
        mk(pal, m.pal); mk(vitre, m.vitre); mk(jour, m.jour); mk(nuit, m.nuit);
        for (auto& [coul, gg] : textes) mk(gg, m.textes[coul]);
    }

    bool actif_ = false, ville_ = false, plage_ = false;
    const MapDef* map_ = nullptr;
    int64_t len_ = 0;
    AtlasTextes atlas_; Palette palette_;
    Material vitreJour_, vitreNuit_, bougie_;
    std::map<int64_t, Morceau> morceaux_;
    std::vector<std::pair<uint64_t, Mesh>> graveyard_;
    uint64_t frame_ = 0;
};

} // namespace

WFS_SYSTEM(Commerces, 109)   // décor (100-199)

// ---------------------------------------------------------------- banc d'essai (WFS-71)
void benchVie(Game& g, Renderer& r, const std::string& out) {
    auto capture = [&](const std::string& nom, double& t) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : %.0f km/h, %.0f m", nom.c_str(), g.player().kmh(), g.player().dist * M);
    };
    auto rouler = [&](int n, double& t) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t); } };
    // ville (jour) : terrasse avec clients, puis portique du premier checkpoint (règles de course actives)
    courseBenchRules(true);
    if (g.init(r, "golf", "ville")) {
        double t = 1200;
        g.setAutopilot(true); g.camMode = 1;
        rouler(60 * 3 + 30, t);   // décompte
        const double cible = g_premiereTerrasse;   // 18 segments avant la première terrasse (côté droit), 40 s au plus
        for (int i = 0; i < 60 * 40 && cible > 0 && g.player().dist / SEG < cible - 18; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 30 == 0) g.render(t); }
        capture("vie-terrasse", t);
        const Course* c = activeCourse();
        for (int i = 0; i < 60 * 60 && c && (c->nextCp - g.player().dist) * M > 25; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 30 == 0) g.render(t); courseBenchSetTime(30); }
        capture("vie-portique", t);   // WFS-94 (RG-A1, ANO-40) : à 25 m du portique « CHECKPOINT 1 »
        g.shutdown();
    }
    courseBenchRules(false);
    // Corniche de jour : serviettes, personnes assises, baigneurs ; de nuit : panneau néon
    for (const char* mo : {"jour", "nuit"}) {
        setenv("WFS_MOMENT", mo, 1);
        const bool jour = !std::strcmp(mo, "jour");
        // WFS-94 (RG-A1, ANO-40) : de jour, départ à côté de la première serviette proche de la route, caméra posée côté
        // terre et tournée vers la mer : la plage (serviettes, baigneurs) à ≈ 15 m de la caméra
        Track tr; tr.build(mapById("plage"));
        if (jour) {
            const int64_t L = (int64_t)tr.segmentCount();
            for (const Serviette& sv : serviettes(*tr.map(), L)) if (sv.lat < (float)RW + 16) { setenv("WFS_DEPART", std::to_string((int64_t)sv.s - 20).c_str(), 1); break; }
        }
        if (g.init(r, "golf", "plage")) {
            double t = 1300;
            g.setAutopilot(true); g.camMode = 1;
            rouler(jour ? 60 : 20, t);
            if (jour) {
                const TrackPoint q = tr.point(g.player().dist, 0);
                g.camMode = 2; g.orbit = std::atan2(-(float)std::cos(q.th), -(float)std::sin(q.th));
            }
            capture(jour ? "vie-plage" : "vie-enseigne-nuit", t);
            g.camMode = 1; unsetenv("WFS_DEPART");
            g.shutdown();
        }
    }
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
}

} // namespace wfs
