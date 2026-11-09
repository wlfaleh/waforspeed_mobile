// WaForSpeed — météo (ticket WFS-18, épic E2) : voir meteo.h.
#include "meteo.h"
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/player.h"
#include "../game/ressenti.h"   // WFS-70 : « réduire les mouvements » (reduceMotion du jeu web)
#include "../game/track.h"
#include "../render/renderer.h"
#include "../game/camera_vue.h"            // WFS-334 : etatCockpit() (lecture seule : vue Cockpit active)
#include "../game/trace_cartes.h"          // WFS-334 : sections couvertes du tracé (tunnel)
#include "../render/vehicule_cockpit.h"    // WFS-334 : pare-brise et balais de l'image (pareBriseVu)

namespace wfs {

namespace {

// nuit demandée (même source que l'ambiance de nuit, src/render/nuit.cpp) et pluie active sur la carte (RG-01)
bool pluieDemandee(const GameView& g) {
    const char* m = std::getenv("WFS_MOMENT");
    return g.map && meteo::pluieActive(g.map->id, m && std::strcmp(m, "nuit") == 0);
}

// ---------- pluie : initRain / drawFx du jeu web (RG-02, RG-03, RG-05) ----------
class Pluie : public System {
public:
    const char* name() const override { return "pluie"; }

    void init(Renderer&, const GameView& g) override {
        actif_ = pluieDemandee(g);
        gouttes_.clear(); w_ = h_ = 0; reduit_ = false;
        tempsUs_ = 0; images_ = 0;
        if (actif_) WFS_LOG("Pluie : nuit en ville, pluie active");
    }

    void update(double dt, const GameView& g) override {
        // WFS-334 : balais : même modèle et mêmes entrées que la caméra (camera_vue.cpp, essuie_.avancer à chaque pas) : même
        // angle à chaque pas, même quand l'image n'est pas dessinée
        if (g.player) essuie_.avancer((float)dt, g.player->kmh(), actif_);
        if (!actif_ || g.sceneDediee || g.ecran != Ecran::Course) return;
        preparer(g);
        if (gouttes_.empty()) return;
        const float p = vitesseRelative(g);
        // WFS-334 (ANO-194) : à vitesse, les gouttes partent du point de fuite et viennent vers la caméra
        meteo::avancerVers(gouttes_, (float)dt, w_, h_, p, meteo::inclinaison((float)g.player->steer, p), fuiteX_, fuiteY_, rnd_);
        avancerVitre((float)dt, p, g);
    }

    void drawUi(Renderer& r, const GameView& g) override {
        cockpit::PareBriseVu& pb = cockpit::pareBriseVu();
        const bool vitre = pb.vu && etatCockpit().actif; pb.vu = false;   // WFS-334 : intérieur dessiné à cette image
        if (!actif_ || g.sceneDediee || g.ecran != Ecran::Course) return;
        const auto t0 = std::chrono::steady_clock::now();
        preparer(g);
        if (gouttes_.empty()) return;
        const float p = vitesseRelative(g), slant = meteo::inclinaison((float)g.player->steer, p);
        // WFS-334 (RG-03) : sous un toit (tunnel), la pluie s'efface en un fondu court
        const float ciel = partDuCiel(g);
        // point de fuite : la direction regardée par la caméra, projetée (en vue Cockpit comme dehors)
        { const vec3 f = g.camPos + normalize(g.camTarget - g.camPos) * 500.f; float x, y; if (projeter(g, f, x, y)) { fuiteX_ = x; fuiteY_ = y; } else { fuiteX_ = w_ / 2; fuiteY_ = h_ * 0.45f; } }
        // WFS-334 (RG-02, ANO-182, ANO-186) : vue Cockpit : pluie seulement à travers le pare-brise (aucune goutte sur la planche,
        // le volant ni le toit), et gouttes posées sur la vitre, chassées par les balais
        float quad[8]; const bool cockpit = vitre && quadPareBrise(g, pb, quad);
        if (cockpit) dessinerVitre(r, g, pb);
        if (ciel <= 0.01f) { mesurer(t0); return; }
        // trait rgba(170,190,255,.2), 1 px « CSS » de large (lineWidth = w / innerWidth)
        const float e = 0.5f * k_;
        const float cr = 170 / 255.f, cg = 190 / 255.f, cb = 1.f, ca = 0.2f * ciel;
        for (const meteo::Goutte& d : gouttes_) {
            float sx, sy; meteo::sensGoutte(d.x, d.y, fuiteX_, fuiteY_, p, slant, sx, sy);
            const float L = d.l * (1 + 1.5f * p);   // traînée plus longue à vitesse (mouvement pendant l'exposition)
            const float x0 = d.x, y0 = d.y, x1 = d.x + sx * L, y1 = d.y + sy * L;
            if (cockpit && !(dehors(quad, x0, y0) && dehors(quad, x1, y1))) continue;
            const float dx = x1 - x0, dy = y1 - y0, l = std::max(1e-3f, std::sqrt(dx * dx + dy * dy));
            const float nx = -dy / l * e, ny = dx / l * e;
            const UiVertex q[4] = {{x0 + nx, y0 + ny, -1, -1, cr, cg, cb, ca}, {x1 + nx, y1 + ny, -1, -1, cr, cg, cb, ca},
                                   {x1 - nx, y1 - ny, -1, -1, cr, cg, cb, ca}, {x0 - nx, y0 - ny, -1, -1, cr, cg, cb, ca}};
            r.uiQuad(q);
        }
        mesurer(t0);
    }

    void shutdown(Renderer&) override {
        if (images_ > 0)
            WFS_LOG("Pluie : %zu gouttes, coût moyen %.3f ms par image (%d images) ; vitre : %ld gouttes effacées par les balais", gouttes_.size(), tempsUs_ / images_ / 1000.0, images_, balayees_);
        actif_ = false; gouttes_.clear(); images_ = 0; tempsUs_ = 0; vitre_.clear(); anglePrec_ = 0; essuie_ = {}; balayees_ = 0;
    }

private:
    // RG-05 : coût mesuré (CPU, préparation des traits), écrit au journal à la fin de la course
    void mesurer(std::chrono::steady_clock::time_point t0) {
        tempsUs_ += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        images_++;
    }

    // ---------- WFS-334 ----------
    // point du monde → pixels de l'interface (faux : derrière la caméra)
    static bool projeter(const GameView& g, vec3 p, float& sx, float& sy) {
        const mat4 VP = g.proj * g.view;
        const float x = VP.at(0, 0) * p.x + VP.at(0, 1) * p.y + VP.at(0, 2) * p.z + VP.at(0, 3);
        const float y = VP.at(1, 0) * p.x + VP.at(1, 1) * p.y + VP.at(1, 2) * p.z + VP.at(1, 3);
        const float w = VP.at(3, 0) * p.x + VP.at(3, 1) * p.y + VP.at(3, 2) * p.z + VP.at(3, 3);
        if (w <= 1e-4f) return false;
        sx = (x / w * 0.5f + 0.5f) * g.uiWidth; sy = (y / w * 0.5f + 0.5f) * g.uiHeight;   // perspective Vulkan : y vers le bas
        return true;
    }
    // RG-03 : part de la pluie qui tombe autour de la caméra (0 sous la voûte d'un tunnel, fondu à la bouche)
    static float partDuCiel(const GameView& g) {
        if (!g.track || !g.player) return 1.f;
        const double L = (double)g.track->segmentCount() * SEG_M, d = std::fmod(std::fmod(g.player->dist * M, L) + L, L);
        float part = 1.f;
        for (const trace::Section& s : trace::sections(*g.track))
            if (s.type == trace::Type::Tunnel) part = std::min(part, meteo::pluieSousToit(d, s.debut * SEG_M, s.fin * SEG_M));
        return part;
    }
    // vue Cockpit : le point de l'écran donne sur la rue (pare-brise, ou vitre latérale au-dessus de la ligne de la planche,
    // à plus d'une largeur de montant du pare-brise) ; ailleurs, c'est l'habitacle (planche, volant, montants, toit)
    bool dehors(const float q[8], float x, float y) const {
        if (meteo::dansQuad(q, x, y)) return true;
        const float seuil = std::min(q[1], q[3]), marge = 0.07f * h_;   // q : bas gauche, bas droit, haut droit, haut gauche
        if (y >= seuil) return false;
        auto xSur = [&](float ax, float ay, float bx, float by) { return std::abs(by - ay) < 1e-3f ? ax : ax + (bx - ax) * (y - ay) / (by - ay); };
        return x < xSur(q[0], q[1], q[6], q[7]) - marge || x > xSur(q[2], q[3], q[4], q[5]) + marge;
    }
    // pare-brise de l'image, projeté à l'écran (4 sommets) ; faux si un coin est derrière la caméra
    static bool quadPareBrise(const GameView& g, const cockpit::PareBriseVu& pb, float q[8]) {
        for (int i = 0; i < 4; i++) if (!projeter(g, pb.repere.transformPoint(pb.coins[i]), q[2 * i], q[2 * i + 1])) return false;
        return true;
    }
    // gouttes posées sur le pare-brise (u, v dans 0..1 : de bas gauche à haut droit) ; rayon (m), opacité qui monte
    struct GoutteVitre { float u, v, r, a, amax; };
    static constexpr int VITRE_N = 140;
    std::vector<GoutteVitre> vitre_;
    float anglePrec_ = 0;
    cockpit::EtatEssuie essuie_;
    long balayees_ = 0;   // gouttes de la vitre effacées par les balais (journal de fin de course)
    vec3 surVitre(const cockpit::PareBriseVu& pb, float u, float v) const {   // point de la vitre (repère de l'œil)
        const vec3 b = pb.coins[0] + (pb.coins[1] - pb.coins[0]) * u, h = pb.coins[3] + (pb.coins[2] - pb.coins[3]) * u;
        return b + (h - b) * v;
    }
    // gouttes de 2 à 6 mm de rayon (pluie modérée sur un pare-brise), les plus grosses coulent
    // a négatif : la goutte arrive plus tard (0 à 3 s), pour que la vitre balayée ne se recouvre pas d'un coup
    void neuve(GoutteVitre& d) { d.u = rnd_(); d.v = rnd_(); d.r = rnd_.entre(0.002f, 0.006f); d.a = -rnd_() * 0.5f; d.amax = rnd_.entre(0.5f, 0.9f); }
    // gouttes de la vitre : elles apparaissent sous la pluie (pas sous un toit), glissent vers le bas à l'arrêt et remontent
    // à vitesse (vent relatif), et le balai efface celles qu'il croise pendant l'image
    void avancerVitre(float dt, float p, const GameView& g) {
        if (vitre_.empty()) { vitre_.resize(VITRE_N); for (GoutteVitre& d : vitre_) { neuve(d); d.a = d.amax * rnd_(); } }
        const cockpit::PareBriseVu& pb = cockpit::pareBriseVu();
        const cockpit::InterieurDef* idef = g.car ? cockpit::interieurPour(g.car->id) : nullptr;
        const float ciel = partDuCiel(g), a1 = idef ? essuie_.angle(idef->arc) : pb.angle;
        for (GoutteVitre& d : vitre_) {
            d.a = std::min(d.amax, d.a + dt * 0.16f * ciel);                   // la pluie mouille la vitre peu à peu (≈ 6 s) ; sous le toit, plus rien n'arrive
            d.v += dt * (d.r > 0.0045f ? -0.05f : 0.f) + dt * p * p * 0.25f;    // grosses gouttes qui coulent ; à vitesse, le vent les pousse en haut
            if (d.v < 0 || d.v > 1) { neuve(d); continue; }
            const vec3 P = surVitre(pb, d.u, d.v);
            for (int k = 0; k < pb.balais; k++) {   // balai : de son pivot, le long de ex × sens, tourné vers eu de l'angle a
                const vec3 w = P - pb.pivot[k];
                const float wx = dot(w, pb.ex) * pb.sens[k], wy = dot(w, pb.eu);
                if (wx * wx + wy * wy > pb.longueur[k] * pb.longueur[k]) continue;
                if (meteo::balaye(std::atan2(wy, wx), anglePrec_, a1)) { neuve(d); balayees_++; break; }
            }
        }
        anglePrec_ = a1;
    }
    void dessinerVitre(Renderer& r, const GameView& g, const cockpit::PareBriseVu& pb) {
        const bool nuit = g.night;
        for (const GoutteVitre& d : vitre_) {
            if (d.a <= 0.02f) continue;
            const vec3 P = pb.repere.transformPoint(surVitre(pb, d.u, d.v)), Q = pb.repere.transformPoint(surVitre(pb, d.u, d.v) + pb.ex * d.r);
            float x, y, x2, y2;
            if (!projeter(g, P, x, y) || !projeter(g, Q, x2, y2)) continue;
            const float rp = std::max(1.f, std::abs(x2 - x));
            // goutte d'eau : lentille claire (lumières de la rue réfractées), bord plus sombre en bas, reflet net en haut à gauche
            if (d.r > 0.0045f) {   // grosse goutte qui coule : traînée humide au-dessus d'elle
                float xt, yt;
                if (projeter(g, pb.repere.transformPoint(surVitre(pb, d.u, std::min(1.f, d.v + 0.04f))), xt, yt))
                    r.uiQuad(std::array<UiVertex, 4>{UiVertex{x - rp * 0.4f, y, -1, -1, 0.8f, 0.85f, 0.95f, d.a * 0.25f}, UiVertex{x + rp * 0.4f, y, -1, -1, 0.8f, 0.85f, 0.95f, d.a * 0.25f},
                                                     UiVertex{xt + rp * 0.2f, yt, -1, -1, 0.8f, 0.85f, 0.95f, 0}, UiVertex{xt - rp * 0.2f, yt, -1, -1, 0.8f, 0.85f, 0.95f, 0}}.data());
            }
            r.uiCircle(x, y + rp * 0.12f, rp * 1.05f, {0.05f, 0.06f, 0.08f, d.a * 0.35f});
            r.uiCircle(x, y, rp, {0.78f, 0.84f, 0.95f, d.a * (nuit ? 0.6f : 0.45f)});
            r.uiCircle(x - rp * 0.35f, y - rp * 0.35f, std::max(0.8f, rp * 0.3f), {1.f, 1.f, 1.f, d.a * 0.85f});
        }
    }

    float fuiteX_ = 0, fuiteY_ = 0;

    static float vitesseRelative(const GameView& g) {
        return g.player ? (float)std::clamp(std::abs(g.player->speed) / MAX_SPEED, 0.0, 1.0) : 0.f;
    }

    // initRain, refait si la taille de l'écran ou l'option « réduire les animations » change (resize du jeu web)
    void preparer(const GameView& g) {
        const bool reduit = reglagesRessenti().reduireMouvements;
        if (g.uiWidth == w_ && g.uiHeight == h_ && reduit == reduit_) return;
        w_ = g.uiWidth; h_ = g.uiHeight; reduit_ = reduit;
        k_ = std::max(1.f, h_ / 720.f);   // échelle de l'écran : pixels de l'interface par pixel « CSS » (comme effets, WFS-65)
        const int n = meteo::nombreGouttes(w_ / k_, h_ / k_, reduit);
        meteo::initRain(gouttes_, n, w_, h_, k_, rnd_);
        fuiteX_ = w_ / 2; fuiteY_ = h_ * 0.45f;   // WFS-334 : jusqu'à la première image
        WFS_LOG("Pluie : %d gouttes (écran %.0fx%.0f%s)", n, w_, h_, reduit ? ", animations réduites" : "");
    }

    bool actif_ = false, reduit_ = false;
    float w_ = 0, h_ = 0, k_ = 1;
    std::vector<meteo::Goutte> gouttes_;
    meteo::Hasard rnd_;
    double tempsUs_ = 0; int images_ = 0;
};

// ---------- ciel couvert de la nuit en ville (RG-04) ----------
// Le ciel du jeu web (makeSkyMaterial) mêle des nuages de couleur mix(uMid, uHor, 0,5) × 0,9 la nuit, avec la force
// uCloud = nightClouds (0,35) et la couverture cloudCover (0,52). Le ciel natif n'a pas de nuages : on mêle cette couleur
// au haut et à l'horizon du ciel (couche nuageuse moyenne), puis on rapproche le brouillard en proportion.
class CielCouvert : public System {
public:
    const char* name() const override { return "ciel-couvert"; }
    void init(Renderer&, const GameView& g) override { actif_ = pluieDemandee(g); }
    void ambiance(FrameParams& fp, const GameView& g) override {
        if (!actif_ || g.sceneDediee || g.ecran == Ecran::Garage) return;
        const float c = meteo::NIGHT_CLOUDS;
        const vec3 nuage = lerp(fp.skyTop, fp.skyHorizon, 0.5f) * 0.9f;   // cc = mix(uMid, uHor, 0,5) × 0,9
        fp.skyHorizon = lerp(fp.skyHorizon, nuage, c);                     // horizon assombri vers la couche nuageuse
        fp.skyTop = fp.skyTop * (1.f - c * 0.5f);                          // haut du ciel plus sombre (aucune étoile en ville)
        fp.fogColor = lerp(fp.fogColor, nuage, c * 0.5f);
        if (fp.fogFar > 0) {   // brouillard linéaire de la nuit : plus proche sous la pluie
            fp.fogFar = std::max(fp.fogNear + 60.f, fp.fogFar * (1.f - c * meteo::CLOUD_COVER));
        } else if (fp.fogDensity > 0) {
            fp.fogDensity *= 1.f + c * meteo::CLOUD_COVER;
        }
    }
private:
    bool actif_ = false;
};

} // namespace

} // namespace wfs

WFS_SYSTEM(wfs::Pluie, 199)          // monde (100-199) : traits 2D avant la vignette (350) et le HUD (420)
WFS_SYSTEM(wfs::CielCouvert, 960)    // après l'ambiance de nuit (950), qui fixe le ciel et le brouillard
