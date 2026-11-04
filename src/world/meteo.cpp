// WaForSpeed — météo (ticket WFS-18, épic E2) : voir meteo.h.
#include "meteo.h"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/player.h"
#include "../game/ressenti.h"   // WFS-70 : « réduire les mouvements » (reduceMotion du jeu web)
#include "../game/track.h"
#include "../render/renderer.h"

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
        if (!actif_ || g.sceneDediee || g.ecran != Ecran::Course) return;
        preparer(g);
        if (gouttes_.empty()) return;
        const float p = vitesseRelative(g);
        meteo::avancer(gouttes_, (float)dt, w_, h_, p, meteo::inclinaison((float)g.player->steer, p), rnd_);
    }

    void drawUi(Renderer& r, const GameView& g) override {
        if (!actif_ || g.sceneDediee || g.ecran != Ecran::Course) return;
        const auto t0 = std::chrono::steady_clock::now();
        preparer(g);
        if (gouttes_.empty()) return;
        const float p = vitesseRelative(g), slant = meteo::inclinaison((float)g.player->steer, p);
        // trait rgba(170,190,255,.2), 1 px « CSS » de large (lineWidth = w / innerWidth)
        const float e = 0.5f * k_;
        const float cr = 170 / 255.f, cg = 190 / 255.f, cb = 1.f, ca = 0.2f;
        for (const meteo::Goutte& d : gouttes_) {
            const float x0 = d.x, y0 = d.y, x1 = d.x + slant * d.l, y1 = d.y + d.l * (1 + p);
            const float dx = x1 - x0, dy = y1 - y0, l = std::max(1e-3f, std::sqrt(dx * dx + dy * dy));
            const float nx = -dy / l * e, ny = dx / l * e;
            const UiVertex q[4] = {{x0 + nx, y0 + ny, -1, -1, cr, cg, cb, ca}, {x1 + nx, y1 + ny, -1, -1, cr, cg, cb, ca},
                                   {x1 - nx, y1 - ny, -1, -1, cr, cg, cb, ca}, {x0 - nx, y0 - ny, -1, -1, cr, cg, cb, ca}};
            r.uiQuad(q);
        }
        // RG-05 : coût mesuré (CPU, préparation des traits), écrit au journal à la fin de la course
        tempsUs_ += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        images_++;
    }

    void shutdown(Renderer&) override {
        if (images_ > 0)
            WFS_LOG("Pluie : %zu gouttes, coût moyen %.3f ms par image (%d images)", gouttes_.size(), tempsUs_ / images_ / 1000.0, images_);
        actif_ = false; gouttes_.clear(); images_ = 0; tempsUs_ = 0;
    }

private:
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
