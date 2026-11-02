// WaForSpeed — catalogue des voitures (repris de js/cars/catalog.js) et performances dérivées de la fiche réelle.
#pragma once
#include <cstring>

namespace wfs {

struct CarDims { float L, W, H, rw, wb; };
struct CarSpec { int ch; float s100; int vmax; int kg; };
struct CarDef {
    const char* id; const char* brand; const char* model; const char* file;
    CarSpec spec; int grip; CarDims d; unsigned paint;   // couleur de peinture par défaut (0xRRGGBB)
    const char* credit;
};

inline const CarDef CARS[] = {
    {"golf", "Volkswagen", "Golf 8 GTI", "cars/golf.wfm", {245, 6.2f, 250, 1430}, 8, {4.29f, 1.79f, 1.46f, 0.33f, 2.62f}, 0x1d4fd8,
     "« Volkswagen Golf 8. GTI (A8) » par GT Cars: Hyperspeed (Sketchfab), CC BY 4.0"},
    {"m4", "BMW", "M4 Competition", "cars/m4.wfm", {510, 3.9f, 290, 1725}, 7, {4.79f, 1.89f, 1.39f, 0.34f, 2.85f}, 0x101114,
     "« 2021 BMW M4 Competition » par Ricy (Sketchfab), CC BY 4.0"},
    {"p911", "Porsche", "911 Carrera 4S", "cars/p911.wfm", {450, 3.6f, 306, 1565}, 9, {4.52f, 1.85f, 1.30f, 0.34f, 2.45f}, 0x8a8f96,
     "« (FREE) Porsche 911 Carrera 4S » par Karol Miklas (Sketchfab), CC BY-SA 4.0"},
    {"gallardo", "Lamborghini", "Gallardo LP560-4 Spyder", "cars/gallardo.wfm", {560, 4.0f, 324, 1550}, 9, {4.35f, 1.90f, 1.18f, 0.34f, 2.56f}, 0xf2c200,
     "« 2009 Lamborghini Gallardo LP560-4 Spyder » par Ddiaz Design (Sketchfab), CC BY 4.0"},
};
constexpr int CAR_COUNT = sizeof(CARS) / sizeof(CARS[0]);

inline const CarDef& carById(const char* id) {
    for (const auto& c : CARS) if (std::strcmp(c.id, id) == 0) return c;
    return CARS[0];
}

// performances (identiques à carPerf() du jeu web)
struct CarPerf { float top, a0, wb, acc, handling, mass, nitroMult, nitroGain, nitroBurn; };
inline float clampN(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
inline CarPerf carPerf(const CarDef& c) {
    const CarSpec& s = c.spec;
    return {s.vmax / 260.f, (27.8f / s.s100) * 1.45f, c.d.wb, clampN((27.8f / s.s100) / 4.6f, 0.55f, 1.8f),
            0.75f + c.grip * 0.05f, s.kg / 1450.f, 1.3f, 0.12f, 0.26f};
}

} // namespace wfs
