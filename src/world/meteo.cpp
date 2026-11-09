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
#include "../game/game.h"                  // WFS-354 : banc « pluie »
#include "cartes_v2/carte_donnees.h"        // WFS-346 : saison de la carte (flocons)
#include <string>

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
        // WFS-346 : la neige de la carte (saison du paquet) prend ce système ; la pluie de la Ville reste prioritaire
        const cartes_v2::Saison& sz = cartes_v2::saisonPartie();
        const bool pluie = pluieDemandee(g);
        neige_ = !pluie && sz.flocons > 0;
        actif_ = pluie || neige_;
        gouttes_.clear(); w_ = h_ = 0; reduit_ = false;
        tempsUs_ = 0; images_ = 0;
        flocons_.clear(); camPrecOk_ = false; toits_.clear(); aAvancer_ = 0;
        intensite_ = sz.flocons; chute_ = sz.chuteMs; tailleMm_ = sz.floconMm;
        if (neige_ && g.track) {   // toits du tracé (tunnels) : aucun flocon dessous
            for (const trace::Section& s : trace::sections(*g.track)) if (s.type == trace::Type::Tunnel) toits_.push_back({s.debut * SEG_M, s.fin * SEG_M});
        }
        if (pluie) WFS_LOG("Pluie : nuit en ville, pluie active");
        if (neige_) WFS_LOG("Neige (WFS-346) : flocons de la carte : intensité %.2f, chute %.1f m/s, %.1f mm, %zu tunnel(s)", intensite_, chute_, tailleMm_, toits_.size());
    }

    void update(double dt, const GameView& g) override {
        if (!actif_ || g.sceneDediee || g.ecran != Ecran::Course) return;
        preparer(g);
        if (neige_) { aAvancer_ += dt; return; }   // WFS-346 : flocons avancés au dessin, avec la caméra de l'image (inconnue ici)
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
        if (gouttes_.empty() && !neige_) return;
        const float p = vitesseRelative(g), slant = meteo::inclinaison((float)g.player->steer, p);
        // WFS-334 (RG-03) : sous un toit (tunnel), la pluie s'efface en un fondu court
        const float ciel = partDuCiel(g);
        // point de fuite : la direction regardée par la caméra, projetée (en vue Cockpit comme dehors)
        { const vec3 f = g.camPos + normalize(g.camTarget - g.camPos) * 500.f; float x, y; if (projeter(g, f, x, y)) { fuiteX_ = x; fuiteY_ = y; } else { fuiteX_ = w_ / 2; fuiteY_ = h_ * 0.45f; } }
        // WFS-334 (RG-02, ANO-182, ANO-186) : vue Cockpit : pluie seulement à travers le pare-brise (aucune goutte sur la planche,
        // le volant ni le toit), et gouttes posées sur la vitre, chassées par les balais
        // WFS-354 (RG-01) : vitre visible de l'œil, sans ce que cachent les montants (barres vues de biais), moins le joint (1 cm)
        // et le rayon de la plus grosse goutte (6 mm)
        if (vitre) meteo::vitreVue(pb.coins, cockpit::LARGEUR_MONTANT, cockpit::EPAISSEUR_MONTANT, 0.016f, vitreCoins_);
        float quad[8]; const bool cockpit = vitre && quadPareBrise(g, pb, quad);
        if (cockpit && !cockpitPrec_) anglePrec_ = pb.angle;   // retour en Cockpit : rien à balayer à cette image (remarque a)
        cockpitPrec_ = cockpit;
        if (neige_) { dessinerFlocons(r, g, cockpit, quad); mesurer(t0); return; }   // WFS-346 : neige (pas de gouttes sur la vitre)
        if (cockpit) { balayerVitre(pb); dessinerVitre(r, g, pb); }   // WFS-354 (RG-02) : balais de l'image dessinée
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
        if (images_ > 0 && neige_) WFS_LOG("Neige (WFS-346) : %zu flocons, coût moyen %.3f ms par image (%d images)", flocons_.size(), tempsUs_ / images_ / 1000.0, images_);
        else if (images_ > 0)
            WFS_LOG("Pluie : %zu gouttes, coût moyen %.3f ms par image (%d images) ; vitre : %ld gouttes effacées par les balais", gouttes_.size(), tempsUs_ / images_ / 1000.0, images_, balayees_);
        actif_ = false; gouttes_.clear(); images_ = 0; tempsUs_ = 0; vitre_.clear(); anglePrec_ = 0; balayees_ = 0;
        neige_ = false; flocons_.clear(); toits_.clear(); camPrecOk_ = false;
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
    // WFS-354 (RG-03) : en vue Cockpit, selon la position de la caméra (l'œil, sous la voûte en même temps que la voiture) ; dans
    // les autres vues, selon la voiture, comme sur main (2e retour du lead dev : vue Arrière inchangée). Distance de la caméra le
    // long de la route = celle de la voiture + l'écart caméra-voiture projeté sur l'axe de la route (avant = (sin th, −cos th))
    static float partDuCiel(const GameView& g) {
        if (!g.track || !g.player) return 1.f;
        // remarque b : avant la 1re image, la caméra est inconnue : on prend la voiture
        const bool cockpit = etatCockpit().actif && length(g.camPos) > 1e-3f;
        double avance = 0;
        if (cockpit) { const TrackPoint q = g.track->point(g.player->dist, 0); avance = (g.camPos.x - q.x) * std::sin(q.th) - (g.camPos.z - q.z) * std::cos(q.th); }
        const double L = (double)g.track->segmentCount() * SEG_M, d = std::fmod(std::fmod(g.player->dist * M + avance, L) + L, L);
        float part = 1.f;
        for (const trace::Section& s : trace::sections(*g.track))
            if (s.type == trace::Type::Tunnel) part = std::min(part, meteo::pluieTunnel(cockpit, d, s.debut * SEG_M, s.fin * SEG_M));
        return part;
    }
    // vue Cockpit : le point de l'écran donne sur la rue (pare-brise, ou vitre latérale au-dessus de la ligne de la planche,
    // à plus d'une largeur de montant du pare-brise) ; ailleurs, c'est l'habitacle (planche, volant, montants, toit)
    bool dehors(const float q[8], float x, float y) const {
        if (meteo::dansQuad(q, x, y)) return true;
        // WFS-354 : la vitre s'arrête au bord intérieur des montants : la vitre latérale commence après le montant (montantPx_) et une marge
        const float seuil = std::min(q[1], q[3]), marge = 0.07f * h_ + montantPx_;   // q : bas gauche, bas droit, haut droit, haut gauche
        if (y >= seuil) return false;
        auto xSur = [&](float ax, float ay, float bx, float by) { return std::abs(by - ay) < 1e-3f ? ax : ax + (bx - ax) * (y - ay) / (by - ay); };
        return x < xSur(q[0], q[1], q[6], q[7]) - marge || x > xSur(q[2], q[3], q[4], q[5]) + marge;
    }
    // pare-brise de l'image, projeté à l'écran (4 sommets) ; faux si un coin est derrière la caméra
    bool quadPareBrise(const GameView& g, const cockpit::PareBriseVu& pb, float q[8]) {
        for (int i = 0; i < 4; i++) if (!projeter(g, pb.repere.transformPoint(vitreCoins_[i]), q[2 * i], q[2 * i + 1])) return false;
        float x, y;   // du bord de la vitre visible au bord extérieur du montant, à l'écran, au pied du montant gauche (WFS-354)
        montantPx_ = projeter(g, pb.repere.transformPoint(pb.coins[0] - pb.ex * cockpit::EPAISSEUR_MONTANT), x, y) ? std::abs(q[0] - x) : 0.f;
        return true;
    }
    float montantPx_ = 0;
    vec3 vitreCoins_[4]{};       // WFS-354 : vitre visible de l'œil à cette image (repère de l'œil)
    bool cockpitPrec_ = false;   // vue Cockpit à l'image précédente
    // gouttes posées sur le pare-brise (u, v dans 0..1 : de bas gauche à haut droit) ; rayon (m), opacité qui monte
    struct GoutteVitre { float u, v, r, a, amax; };
    static constexpr int VITRE_N = 140;
    std::vector<GoutteVitre> vitre_;
    float anglePrec_ = 0;
    long balayees_ = 0;   // gouttes de la vitre effacées par les balais (journal de fin de course)
    vec3 surVitre(float u, float v) const {   // point de la vitre visible (vitreVue), repère de l'œil
        const vec3* c = vitreCoins_;
        const vec3 b = c[0] + (c[1] - c[0]) * u, h = c[3] + (c[2] - c[3]) * u;
        return b + (h - b) * v;
    }
    // gouttes de 2 à 6 mm de rayon (pluie modérée sur un pare-brise), les plus grosses coulent
    // a négatif : la goutte arrive plus tard (0 à 3 s), pour que la vitre balayée ne se recouvre pas d'un coup
    void neuve(GoutteVitre& d) { d.u = rnd_(); d.v = rnd_(); d.r = rnd_.entre(0.002f, 0.006f); d.a = -rnd_() * 0.5f; d.amax = rnd_.entre(0.5f, 0.9f); }
    // gouttes de la vitre : elles apparaissent sous la pluie (pas sous un toit), glissent vers le bas à l'arrêt et remontent
    // à vitesse (vent relatif), et le balai efface celles qu'il croise pendant l'image
    void avancerVitre(float dt, float p, const GameView& g) {
        if (vitre_.empty()) { vitre_.resize(VITRE_N); for (GoutteVitre& d : vitre_) { neuve(d); d.a = d.amax * rnd_(); } }
        const float ciel = partDuCiel(g);
        for (GoutteVitre& d : vitre_) {
            d.a = std::min(d.amax, d.a + dt * 0.16f * ciel);                   // la pluie mouille la vitre peu à peu (≈ 6 s) ; sous le toit, plus rien n'arrive
            d.v += dt * (d.r > 0.0045f ? -0.05f : 0.f) + dt * p * p * 0.25f;    // grosses gouttes qui coulent ; à vitesse, le vent les pousse en haut
            if (d.v < 0 || d.v > 1) neuve(d);
        }
    }
    // WFS-354 (RG-02) : à chaque image dessinée, le balai efface les gouttes qu'il a croisées depuis l'image précédente, avec
    // l'angle des balais dessinés (pb.angle, écrit par l'intérieur à cette image) : effacement et balai vu sont à la même image
    void balayerVitre(const cockpit::PareBriseVu& pb) {
        const float a1 = pb.angle;
        for (GoutteVitre& d : vitre_) {
            const vec3 P = surVitre(d.u, d.v);
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
            const vec3 P = pb.repere.transformPoint(surVitre(d.u, d.v)), Q = pb.repere.transformPoint(surVitre(d.u, d.v) + pb.ex * d.r);
            float x, y, x2, y2;
            if (!projeter(g, P, x, y) || !projeter(g, Q, x2, y2)) continue;
            const float rp = std::max(1.f, std::abs(x2 - x));
            // goutte d'eau : lentille claire (lumières de la rue réfractées), bord plus sombre en bas, reflet net en haut à gauche
            if (d.r > 0.0045f) {   // grosse goutte qui coule : traînée humide au-dessus d'elle
                float xt, yt;
                if (projeter(g, pb.repere.transformPoint(surVitre(d.u, std::min(1.f, d.v + 0.04f))), xt, yt))
                    r.uiQuad(std::array<UiVertex, 4>{UiVertex{x - rp * 0.4f, y, -1, -1, 0.8f, 0.85f, 0.95f, d.a * 0.25f}, UiVertex{x + rp * 0.4f, y, -1, -1, 0.8f, 0.85f, 0.95f, d.a * 0.25f},
                                                     UiVertex{xt + rp * 0.2f, yt, -1, -1, 0.8f, 0.85f, 0.95f, 0}, UiVertex{xt - rp * 0.2f, yt, -1, -1, 0.8f, 0.85f, 0.95f, 0}}.data());
            }
            r.uiCircle(x, y + rp * 0.12f, rp * 1.05f, {0.05f, 0.06f, 0.08f, d.a * 0.35f});
            r.uiCircle(x, y, rp, {0.78f, 0.84f, 0.95f, d.a * (nuit ? 0.6f : 0.45f)});
            r.uiCircle(x - rp * 0.35f, y - rp * 0.35f, std::max(0.8f, rp * 0.3f), {1.f, 1.f, 1.f, d.a * 0.85f});
        }
    }

    float fuiteX_ = 0, fuiteY_ = 0;

    // ---------- WFS-346 : flocons (particules du monde, voir meteo.h) ----------
    // Chaque flocon est projeté à l'écran : taille réelle (rayon × focale / distance, au moins 0,7 px), traînée = déplacement de la
    // caméra pendant l'image (on roule à travers la neige), effacé au bord de la boîte (pas de « mur » de flocons). Rien sous un
    // tunnel (distance du flocon le long de la route), rien sur l'habitacle en vue Cockpit (mêmes vitres que la pluie). Nuit : les
    // flocons ne se voient que dans le faisceau des phares (devant, sous 35 m) et faiblement ailleurs (éclairage de la rue).
    void dessinerFlocons(Renderer& r, const GameView& g, bool cockpit, const float quad[8]) {
        const int n = meteo::nombreFlocons(intensite_, reduit_);
        if (n == 0) { flocons_.clear(); return; }
        vec3 dcam = camPrecOk_ ? g.camPos - camPrec_ : vec3{};
        if ((int)flocons_.size() != n || length(dcam) > 8.f) { meteo::semerFlocons(flocons_, n, g.camPos, tailleMm_, rnd_); dcam = {}; }   // départ, saut de caméra
        camPrec_ = g.camPos; camPrecOk_ = true;
        dcam = dcam * (float)std::min(1.0, (1.0 / 60) / std::max(aAvancer_, 1e-4));   // traînée d'une exposition de 1/60 s (images espacées au banc)
        tempsNeige_ += aAvancer_;
        meteo::avancerFlocons(flocons_, (float)std::min(aAvancer_, 0.25), chute_, (float)tempsNeige_, g.camPos);   // chute du temps écoulé, boîte autour de la caméra
        aAvancer_ = 0;
        const vec3 cf = normalize(g.camTarget - g.camPos);
        const float focale = std::abs(g.proj.at(1, 1)) * g.uiHeight * 0.5f;   // pixels par mètre à 1 m
        double dCar = 0, L = 1, th = 0, qx = 0, qz = 0;
        if (g.track && g.player) {
            const TrackPoint q = g.track->point(g.player->dist, 0);
            dCar = g.player->dist * M; L = std::max(1.0, (double)g.track->segmentCount() * SEG_M); th = q.th; qx = q.x; qz = q.z;
        }
        const double sn = std::sin(th), cs = std::cos(th);
        const bool nuit = g.night;
        for (const meteo::Flocon& k : flocons_) {
            if (!toits_.empty()) {   // distance du flocon le long de la route (avant de la route = (sin th, −cos th), comme partDuCiel)
                const double d = std::fmod(std::fmod(dCar + (k.p.x - qx) * sn - (k.p.z - qz) * cs, L) + L, L);
                if (meteo::sousToit(d, toits_)) continue;
            }
            const vec3 rel = k.p - g.camPos;
            const float z = dot(rel, cf);
            if (z < 0.3f) continue;
            float x1, y1, x0, y0;
            if (!projeter(g, k.p, x1, y1) || x1 < -4 || y1 < -4 || x1 > w_ + 4 || y1 > h_ + 4) continue;
            if (cockpit && !dehors(quad, x1, y1)) continue;
            if (!projeter(g, k.p + dcam, x0, y0)) { x0 = x1; y0 = y1; }
            const float px = std::max(1.6f * k_, k.r * focale / z);   // au moins ≈ 1,5 px « CSS » : flou de mise au point et de bougé d'un flocon proche
            const float horiz = std::sqrt(rel.x * rel.x + rel.z * rel.z);
            float a = 0.9f * (1 - std::clamp((horiz - 0.7f * meteo::FLOCONS_DEMI) / (0.3f * meteo::FLOCONS_DEMI), 0.f, 1.f)) * std::min(1.f, z);
            float lum = 0.95f;
            if (nuit) {   // faisceau des phares : devant, cône de ± 0,35 rad, sous 35 m, sous la hauteur de la caméra + 1 m
                const float lat = length(rel - cf * z) / z;
                const float phare = (1 - std::clamp((lat - 0.25f) / 0.2f, 0.f, 1.f)) * (1 - std::clamp((z - 20.f) / 15.f, 0.f, 1.f)) * (rel.y < 1.f ? 1.f : 0.5f);
                lum = 0.28f + 0.67f * phare;
            }
            if (a <= 0.02f) continue;
            const float cr = 0.93f * lum, cg = 0.95f * lum, cb = 0.98f * lum;
            float dx = x1 - x0, dy = y1 - y0, l = std::sqrt(dx * dx + dy * dy);
            if (l < px) { dx = 0; dy = 1; l = 1; x0 = x1; y0 = y1 - px; }   // presque immobile : petit carré du diamètre du flocon
            const float nx = -dy / l * px * 0.5f, ny = dx / l * px * 0.5f;
            const float at = l > px * 3 ? a * std::max(0.5f, px * 3 / l) : a;   // traînée longue : plus pâle (même lumière étalée)
            const UiVertex q[4] = {{x0 + nx, y0 + ny, -1, -1, cr, cg, cb, at * 0.3f}, {x1 + nx, y1 + ny, -1, -1, cr, cg, cb, at},
                                   {x1 - nx, y1 - ny, -1, -1, cr, cg, cb, at}, {x0 - nx, y0 - ny, -1, -1, cr, cg, cb, at * 0.3f}};
            r.uiQuad(q);
        }
    }
    bool neige_ = false, camPrecOk_ = false;
    float intensite_ = 0, chute_ = 1.2f, tailleMm_ = 3.5f;
    double tempsNeige_ = 0, aAvancer_ = 0;
    vec3 camPrec_{};
    std::vector<meteo::Flocon> flocons_;
    std::vector<std::pair<double, double>> toits_;   // tunnels du tracé (m le long de la route)

    static float vitesseRelative(const GameView& g) {
        return g.player ? (float)std::clamp(std::abs(g.player->speed) / MAX_SPEED, 0.0, 1.0) : 0.f;
    }

    // initRain, refait si la taille de l'écran ou l'option « réduire les animations » change (resize du jeu web)
    void preparer(const GameView& g) {
        const bool reduit = reglagesRessenti().reduireMouvements;
        if (g.uiWidth == w_ && g.uiHeight == h_ && reduit == reduit_) return;
        w_ = g.uiWidth; h_ = g.uiHeight; reduit_ = reduit;
        k_ = std::max(1.f, h_ / 720.f);   // échelle de l'écran : pixels de l'interface par pixel « CSS » (comme effets, WFS-65)
        const int n = neige_ ? 0 : meteo::nombreGouttes(w_ / k_, h_ / k_, reduit);   // WFS-346 : neige : aucune goutte
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

// ---------- WFS-354 : banc « pluie » (WFS_BANC=pluie) : Ville de nuit sous la pluie, Golf ----------
// pluie-cockpit-105 (montants à plus de 100 km/h ; -x5 : 2e intérieur), pluie-balais-1 à 5 (passage d'un balai, à 10, 30, 50, 70 et 90 %
// de sa course montante), pluie-tunnel-avant / -bouche / -3m / -6m / -milieu (vue Cockpit, la caméra à ces distances de la
// bouche), pluie-arriere-vitesse (témoin : vue Arrière à plus de 100 km/h), pluie-tunnel-arriere-voiture-bouche / -arriere-bouche / -6m (témoin)
void benchPluie(Game& g, Renderer& r, const std::string& out) {
    setenv("WFS_MOMENT", "nuit", 1);
    Track t; t.build(mapById("ville"), trace::versionCarte("ville"));
    int64_t tun = -1, tunFin = -1;
    for (const trace::Section& s : trace::sections(t)) if (s.type == trace::Type::Tunnel && tun < 0) { tun = s.debut; tunFin = s.fin; }
    double temps = 1000;
    auto pas = [&](int n, int rendu) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); temps += 1.0 / 60; if (rendu > 0 && i % rendu == 0) g.render(temps); } };
    auto capture = [&](const std::string& nom) {
        g.render(temps += 1.0 / 60); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : %.0f km/h, %.1f m, balais %.2f rad", nom.c_str(), g.player().kmh(), g.player().dist * M, cockpit::pareBriseVu().angle);
    };
    auto partir = [&](int64_t seg, VueCamera v, const char* voiture = "golf") {
        setenv("WFS_DEPART", std::to_string(std::max<int64_t>(0, seg)).c_str(), 1);
        if (!g.init(r, voiture, "ville")) { WFS_ERR("banc pluie : chargement impossible"); return false; }
        choisirVue(v); g.camMode = 0; g.setAutopilot(true); g.allerA(Ecran::Course);
        pas(60 * 3 + 30, 2);   // décompte, départ
        return true;
    };
    auto jusqua100 = [&]() { for (int k = 0; k < 60 * 40 && g.player().kmh() < 102; k++) pas(1, 3); };
    // 1. montant gauche à vitesse, puis passage des balais (rendu à chaque image : les balais dessinés effacent les gouttes)
    if (partir(0, VueCamera::Cockpit)) {
        jusqua100();
        capture("pluie-cockpit-105");
        const cockpit::InterieurDef* d = cockpit::interieurPour("golf");
        const float arc = d ? d->arc : 1.5f;
        float prec = cockpit::pareBriseVu().angle;
        int n = 0;
        for (int k = 0; k < 600 && n < 5; k++) {
            pas(1, 1);
            const float a = cockpit::pareBriseVu().angle, cible = arc * (0.1f + 0.2f * n);
            if (a > prec && prec < cible && a >= cible) capture("pluie-balais-" + std::to_string(++n));
            prec = a;
        }
        g.shutdown();
    }
    // 2. entrée du tunnel en vue Cockpit : la caméra (à l'œil du conducteur) à −10, 0, 3, 6 m de la bouche et au milieu
    if (tun >= 0 && partir(tun - (int64_t)(70 / SEG_M), VueCamera::Cockpit)) {
        const double bouche = tun * SEG_M, milieu = (tun + tunFin) / 2 * SEG_M;
        const std::pair<double, const char*> etapes[] = {{bouche - 10, "pluie-tunnel-avant"}, {bouche, "pluie-tunnel-bouche"}, {bouche + 3, "pluie-tunnel-3m"},
                                                          {bouche + 6, "pluie-tunnel-6m"}, {milieu, "pluie-tunnel-milieu"}};
        size_t e = 0;
        for (int k = 0; k < 60 * 60 && e < 5; k++) { pas(1, 2); if (g.player().dist * M >= etapes[e].first) capture(etapes[e++].second); }
        g.shutdown();
    }
    // 3. témoin : vue Arrière sous la pluie à vitesse
    if (partir(0, VueCamera::Poursuite)) { jusqua100(); capture("pluie-arriere-vitesse"); g.shutdown(); }
    // 4. (retour du lead dev, PR #305) 2e intérieur à vitesse (X5 : montants plus écartés) ; témoin vue Arrière à la bouche du
    // tunnel : la voiture à la bouche (caméra encore dehors : fondu), la voiture 6 m dedans (caméra de poursuite à la bouche), 6 m plus loin
    if (partir(0, VueCamera::Cockpit, "x5")) { jusqua100(); capture("pluie-cockpit-105-x5"); g.shutdown(); }
    if (tun >= 0 && partir(tun - (int64_t)(70 / SEG_M), VueCamera::Poursuite)) {
        const double bouche = tun * SEG_M;
        const std::pair<double, const char*> etapes[] = {{bouche, "pluie-tunnel-arriere-voiture-bouche"}, {bouche + 6, "pluie-tunnel-arriere-bouche"},
                                                          {bouche + 12, "pluie-tunnel-arriere-6m"}};
        size_t e = 0;
        for (int k = 0; k < 60 * 60 && e < 3; k++) { pas(1, 2); if (g.player().dist * M >= etapes[e].first) capture(etapes[e++].second); }
        g.shutdown();
    }
    choisirVue(VueCamera::Poursuite); unsetenv("WFS_DEPART"); unsetenv("WFS_MOMENT");
}

} // namespace wfs

WFS_SYSTEM(wfs::Pluie, 199)          // monde (100-199) : traits 2D avant la vignette (350) et le HUD (420)
WFS_SYSTEM(wfs::CielCouvert, 960)    // après l'ambiance de nuit (950), qui fixe le ciel et le brouillard
