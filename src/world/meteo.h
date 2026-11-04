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
#include <cstdint>
#include <cstring>
#include <vector>

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

} // namespace wfs::meteo
