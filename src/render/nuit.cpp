// WaForSpeed — nuit complète (ticket WFS-09, épic E4 ; absorbe WFS-16 « route mouillée avec reflets »).
// Port de l'éclairage de nuit du jeu web (wlfaleh/waaps, waforspeed/) :
//   - ambiance (js/world/maps.js, envFor nuit) : ciel, hémisphère #46327a / #08080e, lune #8a96ff, brouillard linéaire 40-540 m ;
//   - phares du joueur (js/cars/player.js, buildPlayer) : projecteur 0xfff1dd, intensité ENV.headlight, portée 170 m,
//     angle 0,7, pénombre 0,65 ; feu de recul (PointLight 0xe8eeff, 2,2 la nuit, 14 m) ;
//   - tache des phares (buildBeam / updateBeam) : grille 17 × 11 qui épouse la route, deuxième passage sans éclairage public ;
//   - lampadaires (js/world/scene.js, « éclairage des lampadaires proches ») : 4 lumières au plus sur les plus proches,
//     halo de la tête et flaque de lumière au sol (poolQuad de js/world/decor.js) ;
//   - route mouillée (WFS-16) : nuit en ville et sur la corniche ; flaques et reflets temps réel en qualité haute.
// Les feux des voitures (arrière, stop, recul, phares) sont allumés par le moteur (Renderer::feuxVoiture) dès que
// FrameParams::feux est vrai ; ce système donne seulement l'état du joueur (frein, marche arrière).
// Moment : WFS_MOMENT=nuit (écran Terrain, WFS-41, ou banc d'essai), comme la plage (WFS-14).
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/cars.h"
#include "../game/player.h"
#include "../game/track.h"
#include "../ui/ecrans/reglages.h"
#include "eclairage.h"
#include "renderer.h"
#include "feux_modele.h"   // WFS-109
#include "../world/meteo.h"   // WFS-142 (RG-01) : route mouillée seulement quand il pleut (lecture seule)

namespace wfs {
namespace {

vec3 linHex(uint32_t h) {
    auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); };
    return {c(16), c(8), c(0)};
}
vec3 lin(vec3 c) { return {std::pow(c.x, 2.2f), std::pow(c.y, 2.2f), std::pow(c.z, 2.2f)}; }

// BEAM (js/cars/models.js) et rangées de la grille (js/cars/player.js)
constexpr float BEAM_LEN = 110.f, BEAM_WID = 26.f;
constexpr float BEAM_ROWS[] = {0, 1, 2.5f, 4.5f, 7, 10, 14, 19, 25, 32, 40, 50, 61, 73, 86, 100, 110};
constexpr int NR = 17, NC = 11;
constexpr int LAMPES_MAX = 4;   // RG-05 : 4 lumières ponctuelles au plus

class Nuit : public System {
public:
    const char* name() const override { return "nuit"; }

    void init(Renderer&, const GameView& g) override {
        const char* m = std::getenv("WFS_MOMENT");
        nuit_ = m && std::strcmp(m, "nuit") == 0 && g.map;
        if (!nuit_) return;
        const char* id = g.map->id;
        ville_ = !std::strcmp(id, "ville"); plage_ = !std::strcmp(id, "plage");
        campagne_ = !std::strcmp(id, "campagne"); circuit_ = !std::strcmp(id, "circuit");
        // RG-06 : la campagne n'a pas d'éclairage public. La ville inscrit ses lampadaires (src/world/ville.cpp) ;
        // la corniche et le circuit n'ont pas encore de lampadaires en décor natif : têtes de lampe placées ici aux pas du
        // jeu web (decor.js : « beach » tous les 30 segments côté mer, « flood » tous les 34 en alternance)
        auto& L = lampadaires();
        const int64_t len = (int64_t)g.track->segmentCount();
        if (campagne_) L.clear();
        else if (L.empty() && plage_) for (int64_t n = 15; n < len; n += 30) L.push_back({n, (float)RW + 0.6f, 5.6f, StyleLampadaire::Plage});
        else if (L.empty() && circuit_)
            for (int64_t n = 10; n < len; n += 34) L.push_back({n, ((n / 34) % 2 ? 1.f : -1.f) * ((float)RW + 4.f), 16.f, StyleLampadaire::Stade});
        std::sort(L.begin(), L.end(), [](const Lampadaire& a, const Lampadaire& b) { return a.seg < b.seg; });
        WFS_LOG("Nuit : carte %s, %zu lampadaires, phares %.1f%s", id, L.size(), phare(), mouille() ? ", route mouillée" : "");
    }

    float phare() const { return campagne_ ? 3.f : 2.4f; }   // ENV.headlight
    // WFS-142 (RG-01) : la chaussée n'est mouillée que s'il pleut (meteo::pluieActive : la ville la nuit), ou si le banc
    // force la météo (WFS_METEO=pluie / sec). Avant : mouillée à chaque nuit en ville et sur la corniche, même sans pluie.
    static int meteoForcee() {   // 1 pluie, 0 sec, -1 rien de forcé
        const char* m = std::getenv("WFS_METEO");
        if (!m) return -1;
        return !std::strcmp(m, "pluie") ? 1 : !std::strcmp(m, "sec") ? 0 : -1;
    }
    bool mouille() const { const int f = meteoForcee(); return f >= 0 ? f == 1 : meteo::pluieActive(ville_ ? "ville" : "", nuit_); }

    bool actif(const GameView& g) const { return nuit_ && !g.sceneDediee && g.ecran != Ecran::Garage; }

    // matrice de la voiture du joueur (Game::carMatrix)
    static mat4 matriceJoueur(const GameView& g) {
        const Player& p = *g.player;
        const TrackPoint q = g.track->point(p.dist, p.playerX * RW);
        return translate({(float)q.x, (float)q.y, (float)q.z}) * rotateY((float)-(q.th + p.heading + p.spinYaw)) * rotateX((float)q.slope);
    }

    // distance (segments) du joueur au lampadaire, sur la piste qui se répète ; < 0 : derrière
    static int64_t ecart(const Lampadaire& l, int64_t aP, int64_t len) {
        int64_t d = ((l.seg - aP) % len + len) % len;
        return d > len - 40 ? d - len : d;
    }

    void ambiance(FrameParams& fp, const GameView& g) override {
        if (!actif(g)) {   // WFS-142 (RG-09) : le jour, route mouillée seulement si le banc force la pluie (aucune pluie de jour en jeu)
            if (!nuit_ && meteoForcee() == 1 && g.player && !g.sceneDediee && g.ecran != Ecran::Garage) {
                fp.mouille = true; fp.flaques = fp.reflets = ecrans::reglages().qualite == ecrans::QUALITE_HAUTE;
                fp.refletY = matriceJoueur(g).transformPoint({0, 0, 0}).y;
            }
            return;
        }
        const bool haute = ecrans::reglages().qualite == ecrans::QUALITE_HAUTE;
        // ---------- RG-01 : envFor nuit (couleurs éclaircies comme la plage, WFS-14 : l'étalonnage du web n'est pas porté) ----------
        // WFS-97 (RG-04, ANO-37) : en ville et sur la corniche, on aurait dit le crépuscule (façades lavande) → ciel et
        // brouillard moins éclaircis, hémisphère × 0,45 et lune × 0,4 : façades sombres, la voiture, les néons, les flaques
        // et les reflets ressortent. Exposition (1,1) gardée : la baisser éteindrait aussi néons et flaques ; ambiance (0,7)
        // gardée : elle ne règle que les reflets du ciel. Circuit (nuit bleutée voulue) et campagne inchangés.
        const bool rue = ville_ || plage_;
        const float k = rue ? 1.4f : 2.2f;
        uint32_t hor = 0x3a1a36, fog = 0x120d1d, hemiS = 0x46327a; float hemiI = 0.75f, fogFar = 540;
        if (plage_) { hor = 0x2b2240; fog = 0x141428; fogFar = 640; }
        if (campagne_) { hor = 0x1b2236; fog = 0x0d1218; hemiS = 0x3a4670; hemiI = 0.6f; }
        if (circuit_) { hor = 0x1f2440; fog = 0x0e1020; hemiS = 0x5a5c90; hemiI = 0.95f; fogFar = 640; }
        if (ecrans::reglages().qualite == ecrans::QUALITE_BASSE) fogFar = std::min(fogFar, 420.f);
        fp.skyTop = linHex(0x04040a) * k; fp.skyHorizon = linHex(hor) * k; fp.ground = linHex(0x0a0a10) * k;
        fp.fogColor = linHex(fog) * k; fp.fogNear = 40; fp.fogFar = fogFar; fp.fogDensity = 0;
        fp.hemiSky = linHex(hemiS); fp.hemiGround = linHex(0x08080e); fp.hemiIntensity = hemiI * (rue ? 0.45f : 1.f);
        // lune : 0,35 du jeu web (soleil 1,2 → 3,2 ici)
        fp.sunDir = normalize(vec3{-0.4f, 0.8f, 0.3f}); fp.sunColor = linHex(0x8a96ff); fp.sunIntensity = 0.35f * 3.2f / 1.2f * (rue ? 0.4f : 1.f);
        fp.ambient = 0.7f; fp.exposure = 1.1f; fp.night = true; fp.shadows = false;
        // ---------- feux et route mouillée ----------
        fp.feux = true;
        fp.mouille = mouille(); fp.flaques = haute; fp.reflets = haute;
        const mat4 W = matriceJoueur(g);
        fp.refletY = W.transformPoint({0, 0, 0}).y;   // plan du miroir : la chaussée sous la voiture
        // ---------- lumières : phares, recul, 4 lampadaires les plus proches ----------
        fp.spotCount = 0;
        const CarDims& d = g.car->d;
        FrameParams::Spot& h = fp.spots[fp.spotCount++];   // RG-02
        h.pos = W.transformPoint({0, std::max(0.6f, d.H * 0.41f), -d.L / 2 - 0.2f});
        h.target = W.transformPoint({0, -0.5f, -60});
        h.color = linHex(0xfff1dd); h.intensity = phare(); h.distance = 170; h.angle = 0.7f; h.penumbra = 0.65f; h.decay = 1;
        if (g.player->reversing || g.player->speed < -1) {   // feu de recul (revLight)
            // WFS-171 (RG-01, ANO-70) : lumière posée sur les feux (hauteur ≈ 0,6 H, ras du pare-chocs ; elle était 0,8 m
            // derrière et 0,6 m de haut : disque blanc décollé), tournée vers l'arrière et 31° vers le bas, cône de 83°
            // (la caisse n'est pas éclairée), bord doux. Intensité tirée de R23 (feux_modele.h, CD_RECUL_SOL = 40 cd par
            // feu vers la chaussée) : 2 × 40 cd donnent ≈ 30 lux à 1 m derrière, 5 lux à 3 m, < 1 lux à 6 m ; avec
            // l'étalon de 35 lux par unité, portée 8 m et décroissance 2 donnent 28, 6,6 et 0,5 lux (écart ≤ 30 % de 1 à 6 m).
            // Couleur : ampoule à incandescence (≈ 3000 K), même blanc halogène que les phares (DA-realisme § 3).
            FrameParams::Spot& r = fp.spots[fp.spotCount++];
            r.pos = W.transformPoint({0, std::max(0.5f, d.H * 0.6f), d.L / 2 - 0.05f});
            r.target = W.transformPoint({0, std::max(0.5f, d.H * 0.6f) - 0.6f, d.L / 2 + 0.95f});
            r.color = linHex(0xfff1dd); r.intensity = 1.85f; r.distance = 8; r.angle = 1.45f; r.penumbra = 1; r.decay = 2;
        }
        const auto& L = lampadaires();
        const int64_t len = (int64_t)g.track->segmentCount(), aP = (int64_t)std::floor(g.player->dist / SEG);
        proches_.clear();
        for (size_t i = 0; i < L.size(); i++) { const int64_t e = ecart(L[i], aP, len); if (e >= -4 && e < 160) proches_.push_back({e, i}); }
        std::sort(proches_.begin(), proches_.end());
        for (size_t j = 0; j < proches_.size() && j < (size_t)LAMPES_MAX; j++) {
            const Lampadaire& l = L[proches_[j].second];
            const TrackPoint q = g.track->point((double)(aP + proches_[j].first) * SEG, l.lat);
            FrameParams::Spot& s = fp.spots[fp.spotCount++];
            // decay 2 comme PointLight(0xffb46b, 1.1, 26, 2) du jeu web (renderer.js) : lumière moins étalée (WFS-97, RG-10)
            s.pos = {(float)q.x, (float)q.y + l.y, (float)q.z}; s.target = s.pos + vec3{0, -1, 0}; s.angle = PI; s.decay = 2;
            const bool stade = l.style == StyleLampadaire::Stade;
            s.color = linHex(stade ? 0xeaf2ff : 0xffb46b); s.intensity = stade ? 1.8f : 1.1f; s.distance = stade ? 60.f : 26.f;
        }
    }

    void render(Renderer& r, const GameView& g) override {
        if (!actif(g)) { if (!g.sceneDediee && g.ecran != Ecran::Garage) feuxJoueur(r, g); return; }   // WFS-109 : feux du joueur le jour
        Track& t = *g.track;
        const Player& p = *g.player;
        // ---------- RG-03 : tache des phares sur la route ----------
        const double yaw = p.heading + p.spinYaw, cy = std::cos(yaw), sy = std::sin(yaw);
        const double latM = p.playerX * RW, front = g.car->d.L / 2 - 0.3;
        vec3 pt[NR][NC];
        for (int i = 0; i < NR; i++) for (int c = 0; c < NC; c++) {
            const double dd = front + BEAM_ROWS[i], x = ((double)c / (NC - 1) - 0.5) * BEAM_WID;
            const TrackPoint q = t.point(p.dist + (dd * cy - x * sy) / M, latM + x * cy + dd * sy);
            pt[i][c] = {(float)q.x, (float)q.y + 0.07f, (float)q.z};
        }
        Vertex v[(NR - 1) * (NC - 1) * 4];
        int n = 0;
        auto sommet = [&](int i, int c) { const vec3& q = pt[i][c]; v[n++] = {q.x, q.y, q.z, 0, 1, 0, (float)c / (NC - 1), BEAM_ROWS[i] / BEAM_LEN}; };
        for (int i = 1; i < NR; i++) for (int c = 1; c < NC; c++) { sommet(i - 1, c - 1); sommet(i - 1, c); sommet(i, c); sommet(i, c - 1); }
        const int nq = n / 4;
        r.drawQuads(v, nq, r.poolMat);
        if (lampadaires().empty()) r.drawQuads(v, nq, r.poolMat);   // sans éclairage public : deuxième passage (≈ 4× au cœur)
        // ---------- RG-05 : têtes de lampe et flaques de lumière (poolQuad, MAT.pool) ----------
        const auto& L = lampadaires();
        const int64_t len = (int64_t)t.segmentCount(), aP = (int64_t)std::floor(p.dist / SEG);
        for (const Lampadaire& l : L) {
            const int64_t e = ecart(l, aP, len);
            if (e < -20 || e > 420) continue;   // ≈ 500 m : au-delà, le brouillard les efface
            const double s = (double)(aP + e) * SEG;
            const float sg = l.lat > 0 ? 1.f : -1.f;
            float poolLat = sg * ((float)RW - 2.4f), R2 = 8; vec3 col = lin({0.5f, 0.32f, 0.16f}), tete = linHex(0xffd9a8);
            if (l.style == StyleLampadaire::Plage) { poolLat = sg * ((float)RW - 0.5f); R2 = 7; }
            if (l.style == StyleLampadaire::Stade) { poolLat = sg * ((float)RW - 3.f); R2 = 14; col = lin({0.35f, 0.38f, 0.45f}); }
            const TrackPoint a = t.point(s, poolLat), b = t.point(s + 1.0 / M, poolLat);
            const vec3 c{(float)a.x, (float)a.y + 0.04f, (float)a.z};
            const vec3 f = normalize(vec3{(float)(b.x - a.x), (float)(b.y - a.y), (float)(b.z - a.z)});
            const vec3 rr{(float)std::cos(a.th), 0, (float)std::sin(a.th)};
            // × 2,2 : le jeu web ajoute la tache à l'asphalte éclairé avant sa courbe (pow 2,2) ; mesuré au banc (WFS-97), le
            // cœur ressort ainsi comme au web dès que la chaussée reçoit la lumière (≈ × 1 ne laisse qu'un voile)
            r.haloSol(c, rr, f, R2, R2, col * 2.2f);
            // halo de la tête : seulement pour les lampadaires du décor (la corniche et le circuit n'ont pas encore de mât natif)
            if (l.style != StyleLampadaire::Ville) continue;
            const TrackPoint hq = t.point(s, l.lat);
            r.halo({(float)hq.x, (float)hq.y + l.y + 0.25f, (float)hq.z}, 1.1f, tete * 1.6f);
        }
        feuxJoueur(r, g);
    }

    // ---------- RG-04 : feux du joueur (dessiné juste après les systèmes par Game) ----------
    // WFS-109 (RG-07, RG-11, RG-15) : de jour comme de nuit ; feu stop allumé et éteint en 80 ms (RG-02)
    void feuxJoueur(Renderer& r, const GameView& g) {
        const Player& p = *g.player;
        stopK_ = feux_modele::transition(stopK_, p.braking, tFeux_ < 0 ? -1.f : (float)(g.time - tFeux_)); tFeux_ = g.time;
        FeuxVoiture fx; fx.nuit = actif(g); fx.stop = p.braking; fx.recul = p.reversing || p.speed < -1; fx.frein = stopK_;
        r.feuxProchaineVoiture(fx);
    }
    float stopK_ = 0; double tFeux_ = -1;

    void shutdown(Renderer&) override { lampadaires().clear(); nuit_ = false; }

private:
    bool nuit_ = false, ville_ = false, plage_ = false, campagne_ = false, circuit_ = false;
    std::vector<std::pair<int64_t, size_t>> proches_;
};

} // namespace

// après tous les décors et l'interface : la tache et les halos se trient avec les transparents ; feuxProchaineVoiture
// doit être le dernier appel avant le dessin de la voiture du joueur par Game
WFS_SYSTEM(Nuit, 950)

} // namespace wfs
