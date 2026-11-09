// WaForSpeed — météo (ticket WFS-18, épic E2) : pluie la nuit en ville et ciel couvert.
// Port de js/world/fx.js (initRain, drawFx : la pluie) et de js/world/maps.js (envFor : rain, nightClouds, cloudCover).
// La route mouillée, les flaques et les reflets (WFS-16) sont dans src/render/nuit.cpp et ne sont pas retouchés ici.
//
// Deux systèmes, dans src/world/meteo.cpp (aucune accroche dans le noyau : WFS_SYSTEM suffit) :
// - « pluie » (ordre 199) : avance les gouttes (update) et les dessine en 2D (drawUi), avant la vignette (350) et le HUD
//   (420), comme le calque fx du jeu web, qui est sous le HUD HTML ;
// - « ciel-couvert » (ordre 960) : assombrit le ciel et rapproche le brouillard, après l'ambiance de nuit (950).
// Partie pure ci-dessous (en-tête seul) : règles RG-01 à RG-03, vérifiables sans GPU.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include "../core/math.h"   // WFS-354 : vitre vue de l'œil (vec3)

#include <string>

namespace wfs {
class Game; class Renderer;
void benchPluie(Game& g, Renderer& r, const std::string& out);   // WFS-354 : banc « pluie » (meteo.cpp)
}

namespace wfs::meteo {

// RG-01 : ENV.rain = night, sur la carte ville seulement (maps.js : `if (map.id === 'ville') E.rain = night`)
inline bool pluieActive(const char* carte, bool nuit) { return nuit && carte && std::strcmp(carte, "ville") == 0; }

// RG-04 : ciel nocturne couvert en ville (maps.js : nightClouds 0,35 ; cloudCover 0,52)
constexpr float NIGHT_CLOUDS = 0.35f, CLOUD_COVER = 0.52f;

// RG-02 : nombre de gouttes = min(160, L × H / 9000), en pixels « CSS » ; 0 si « réduire les animations »
inline int nombreGouttes(float largeurCss, float hauteurCss, bool reduit) {
    if (reduit) return 0;
    const float n = std::min(160.f, largeurCss * hauteurCss / 9000.f);
    return std::max(0, (int)(n + 0.5f));   // Math.round
}

struct Goutte { float x, y, l, s; };   // position, longueur (10-24 px) et vitesse (900-1500 px/s), en pixels de l'écran

// tirage pseudo-aléatoire léger (xorshift32), sans allocation
struct Hasard {
    uint32_t etat = 0x2545F491u;
    float operator()() { etat ^= etat << 13; etat ^= etat >> 17; etat ^= etat << 5; return (etat >> 8) * (1.f / 16777216.f); }
    float entre(float a, float b) { return a + (b - a) * (*this)(); }   // rnd(a, b) du jeu web
};

// initRain : n gouttes réparties sur l'écran ; k = échelle de l'écran (pixels de l'écran par pixel « CSS »)
inline void initRain(std::vector<Goutte>& g, int n, float w, float h, float k, Hasard& rnd) {
    g.resize((size_t)std::max(0, n));
    for (Goutte& d : g) { d.x = rnd() * w; d.y = rnd() * h; d.l = rnd.entre(10, 24) * k; d.s = rnd.entre(900, 1500) * k; }
}

// RG-03 : inclinaison (−0,12 − braquage × 0,25) × (0,4 + vitesse relative)
inline float inclinaison(float braquage, float p) { return (-0.12f - braquage * 0.25f) * (0.4f + p); }

// drawFx (partie mouvement) : chute à s × (0,7 + 0,6 p), dérive selon l'inclinaison ; sortie en bas → retour en haut à une
// abscisse aléatoire ; bords gauche et droit raccordés
inline void avancer(std::vector<Goutte>& g, float dt, float w, float h, float p, float slant, Hasard& rnd) {
    for (Goutte& d : g) {
        d.y += d.s * dt * (0.7f + p * 0.6f); d.x += d.s * dt * slant * 0.3f;
        if (d.y > h) { d.y = -d.l; d.x = rnd() * w; }
        if (d.x < 0) d.x += w; else if (d.x > w) d.x -= w;
    }
}

// ---------- WFS-334 (ANO-182, ANO-186, ANO-194) : pluie sous un toit, vue Cockpit, gouttes inclinées par la vitesse ----------

// RG-03 : part de la pluie visible (0..1) pour la caméra à `d` m sur la route, sous un toit de `debut` à `fin` m (tunnel) :
// 1 dehors, 0 à plus de TOIT_FONDU − marge m sous le toit ; fondu de TOIT_FONDU m à l'entrée et à la sortie.
// WFS-354 (RG-03, 2e retour du lead dev) : deux calculs selon la vue.
// - vue Arrière (et toute vue hors Cockpit) : celui de main, inchangé : `d` = position de la VOITURE, marge TOIT_MARGE = 2 m
//   (la caméra de poursuite, derrière la voiture, passe sous la voûte quand la voiture est 6 m dedans : pluie à 0) ;
// - vue Cockpit : `d` = position de la CAMÉRA (l'œil), marge TOIT_MARGE_COCKPIT = 8 m : fondu de 8 m avant la bouche jusqu'à la
//   bouche, plus aucun trait dès que la caméra est sous la voûte (avant : la pluie tombait encore sur les 6 premiers mètres)
constexpr float TOIT_FONDU = 8.f, TOIT_MARGE = 2.f, TOIT_MARGE_COCKPIT = 8.f;
inline float pluieSousToit(double d, double debut, double fin, float marge = TOIT_MARGE) {
    const double dedans = std::min(d - debut, fin - d);   // m sous le toit (négatif : dehors)
    return std::clamp((float)(1.0 - (dedans + marge) / TOIT_FONDU), 0.f, 1.f);
}
// marge selon la vue : `d` est la position de la caméra en Cockpit (caméra connue), celle de la voiture sinon (comme sur main)
inline float pluieTunnel(bool cockpit, double d, double debut, double fin) {
    return pluieSousToit(d, debut, fin, cockpit ? TOIT_MARGE_COCKPIT : TOIT_MARGE);
}

// RG-02 (ANO-194) : direction d'une goutte à l'écran (unitaire). À l'arrêt : verticale, penchée par `slant` (braquage, jeu
// web). En roulant, les gouttes viennent vers la caméra : elles partent du point de fuite (fx, fy) et s'en écartent, d'autant
// plus que la vitesse relative p est grande (part radiale au plus 0,85 à pleine vitesse).
inline void sensGoutte(float x, float y, float fx, float fy, float p, float slant, float& dx, float& dy) {
    float vx = slant * 0.3f, vy = 1.f;
    const float rl = std::sqrt(vx * vx + vy * vy); vx /= rl; vy /= rl;
    float rx = x - fx, ry = y - fy;
    const float rr = std::sqrt(rx * rx + ry * ry);
    if (rr > 1e-3f) { rx /= rr; ry /= rr; } else { rx = 0; ry = 1; }
    const float a = std::clamp(p * 1.2f, 0.f, 0.85f);
    dx = vx * (1 - a) + rx * a; dy = vy * (1 - a) + ry * a;
    const float l = std::sqrt(dx * dx + dy * dy);
    if (l > 1e-4f) { dx /= l; dy /= l; } else { dx = 0; dy = 1; }
}

// avancer (WFS-334) : chute le long de sensGoutte, à s × (0,7 + 0,6 p) plus un élan radial s × 1,6 p ; une goutte sortie de
// l'écran repart d'un point tiré au hasard (à vitesse, plutôt près du point de fuite, d'où partent les gouttes)
inline void avancerVers(std::vector<Goutte>& g, float dt, float w, float h, float p, float slant, float fx, float fy, Hasard& rnd) {
    for (Goutte& d : g) {
        float dx, dy; sensGoutte(d.x, d.y, fx, fy, p, slant, dx, dy);
        const float v = d.s * dt * (0.7f + p * 0.6f + p * 1.6f);
        d.x += dx * v; d.y += dy * v;
        if (d.y > h || d.y < -d.l * 3 || d.x < -d.l || d.x > w + d.l) {
            const float r = 1 - std::clamp(p, 0.f, 1.f) * 0.6f;   // à vitesse : départ resserré autour du point de fuite
            d.x = std::clamp(fx + (rnd() - 0.5f) * w * r * 1.6f, 0.f, w);
            d.y = p > 0.15f ? std::clamp(fy + (rnd() - 0.5f) * h * r, 0.f, h) : -d.l;
        }
    }
}

// RG-02 : point (x, y) à l'intérieur du quadrilatère convexe q (4 sommets x, y dans l'ordre, sens quelconque)
inline bool dansQuad(const float q[8], float x, float y) {
    int pos = 0, neg = 0;
    for (int i = 0; i < 4; i++) {
        const float ax = q[2 * i], ay = q[2 * i + 1], bx = q[(2 * i + 2) % 8], by = q[(2 * i + 3) % 8];
        const float c = (bx - ax) * (y - ay) - (by - ay) * (x - ax);
        if (c > 0) pos++; else if (c < 0) neg++;
    }
    return pos == 0 || neg == 0;
}

// WFS-354 (RG-01) : vitre visible de l'œil (origine du repère), sans ce que cachent les montants. axe : axes des montants
// (bas gauche, bas droit, haut droit, haut gauche) ; chaque montant est une barre de largeur × épaisseur (mêmes axes que
// barre() de l'intérieur : y le long du montant, x = y × (1, 0, 0), z = x × y). Ses 4 arêtes, projetées depuis l'œil sur le plan
// de la vitre (qui contient les deux axes), donnent le bord le plus rentré qu'elles cachent, en bas et en haut ; on retire en
// plus `marge` (joint, rayon d'une goutte). out : mêmes coins que axe, sur la vitre visible
inline void vitreVue(const vec3 axe[4], float largeur, float epaisseur, float marge, vec3 out[4]) {
    const vec3 n = normalize(cross(vec3{1, 0, 0}, axe[3] - axe[0]));   // normale au plan de la vitre
    for (int s = -1; s <= 1; s += 2) {
        const vec3 a = axe[s < 0 ? 0 : 1], b = axe[s < 0 ? 3 : 2];
        const vec3 y = normalize(b - a), ref = std::fabs(y.x) < 0.9f ? vec3{1, 0, 0} : vec3{0, 0, 1};
        const vec3 x = normalize(cross(y, ref)), z = cross(x, y);
        for (int h = 0; h < 2; h++) {
            const vec3 A = h ? b : a;
            float bord = A.x;
            for (int k = 0; k < 4; k++) {
                const vec3 C = A + x * ((k & 1 ? 0.5f : -0.5f) * largeur) + z * ((k & 2 ? 0.5f : -0.5f) * epaisseur);
                const float dc = dot(n, C);
                if (std::fabs(dc) < 1e-6f) continue;
                const vec3 P = C * (dot(n, A) / dc);   // rayon œil → arête, sur le plan de la vitre
                bord = s > 0 ? std::min(bord, P.x) : std::max(bord, P.x);
            }
            const int i = s < 0 ? (h ? 3 : 0) : (h ? 2 : 1);
            out[i] = {bord - s * marge, A.y, A.z};
        }
    }
}

// RG-02 : le balai d'angle a0 → a1 (rad, autour de son pivot) est passé sur l'angle phi pendant l'image
inline bool balaye(float phi, float a0, float a1) { return std::min(a0, a1) <= phi && phi <= std::max(a0, a1) && a0 != a1; }

} // namespace wfs::meteo
