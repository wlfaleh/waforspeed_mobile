// WaForSpeed — catalogue des voitures (repris de js/cars/catalog.js) et performances dérivées de la fiche réelle.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include "../core/configgame.h"   // WFS-188 : constantes de conduite (frottements, freinage)

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
    // WFS-13 : les 16 autres voitures du jeu web (CARS de catalog.js), modèles convertis par tools/convertir_voitures.sh ;
    // couleur par défaut = peinture par défaut du jeu web (PAINTS). Le trafic n'affiche que les 4 premières (TRAFIC_MODELES).
    {"p208", "Peugeot", "208", "cars/p208.wfm", {130, 8.7f, 208, 1160}, 9, {4.06f, 1.75f, 1.43f, 0.32f, 2.54f}, 0xc8940a,
     "« 2021 Peugeot 208 » par tonielpro520 (Sketchfab), CC BY 4.0"},
    {"abarth", "Abarth", "500", "cars/abarth.wfm", {135, 7.9f, 205, 1035}, 10, {3.66f, 1.63f, 1.49f, 0.3f, 2.3f}, 0xb3122b,
     "« Fiat Abarth 500 » par Luquita (Sketchfab), CC BY 4.0"},
    {"tcross", "Volkswagen", "T-Cross", "cars/tcross.wfm", {150, 8.9f, 200, 1270}, 6, {4.11f, 1.76f, 1.58f, 0.35f, 2.6f}, 0xff6a00,
     "« 2021 Volkswagen T-Cross 280 TSI » par Ddiaz Design (Sketchfab), CC BY-NC-SA 4.0"},
    {"compass", "Jeep", "Compass", "cars/compass.wfm", {150, 9.5f, 196, 1505}, 5, {4.4f, 1.82f, 1.64f, 0.36f, 2.64f}, 0x1f5a3c,
     "« JEEP COMPASS 2021 » par fazt.3ds (Sketchfab), CC BY 4.0"},
    {"s3", "Audi", "S3 berline", "cars/s3.wfm", {300, 4.9f, 250, 1450}, 8, {4.47f, 1.8f, 1.4f, 0.33f, 2.64f}, 0x7b808a,
     "« 2014 Audi S3 Limousine » par tonielpro520 (Sketchfab), CC BY 4.0"},
    {"x5", "BMW", "X5", "cars/x5.wfm", {381, 5.4f, 243, 2135}, 5, {4.92f, 2.0f, 1.75f, 0.37f, 2.69f}, 0xc9ccd2,
     "« 2024 BMW X5 xDrive40i » par Ddiaz Design (Sketchfab), CC BY-NC-SA 4.0"},
    {"giulia", "Alfa Romeo", "Giulia Quadrifoglio", "cars/giulia.wfm", {510, 3.9f, 307, 1580}, 8, {4.64f, 1.87f, 1.43f, 0.34f, 2.82f}, 0xb3122b,
     "« Alfa Romeo Giulia Quadrifoglio » par ijiklvn (Sketchfab), CC BY 4.0"},
    {"a45", "Mercedes-AMG", "A 45", "cars/a45.wfm", {381, 4.2f, 270, 1555}, 8, {4.36f, 1.78f, 1.42f, 0.33f, 2.7f}, 0x7b808a,
     "« Mercedes-Benz A45 AMG 2018 » par Lexyc16 (Sketchfab), CC BY-NC 4.0"},
    {"model3", "Tesla", "Model 3", "cars/model3.wfm", {440, 4.4f, 233, 1844}, 7, {4.69f, 1.85f, 1.44f, 0.34f, 2.88f}, 0xc9ccd2,
     "« Tesla model 3 (free model) » par retro_player (Sketchfab), CC BY 4.0"},
    {"corsa", "Opel", "Corsa", "cars/corsa.wfm", {130, 8.7f, 208, 1165}, 9, {4.06f, 1.77f, 1.43f, 0.32f, 2.54f}, 0xc8940a,
     "« Opel Corsa 2020 » par Nieve5677 (Sketchfab), CC BY 4.0"},
    {"gt86", "Toyota", "GT86", "cars/gt86.wfm", {200, 7.6f, 226, 1240}, 9, {4.24f, 1.78f, 1.29f, 0.32f, 2.57f}, 0xb3122b,
     "« Toyota GT86 3D Model (Free) » par Mpgs.studio3DModels (Sketchfab), CC BY 4.0"},
    {"elantra", "Hyundai", "Elantra N", "cars/elantra.wfm", {280, 5.3f, 250, 1440}, 8, {4.68f, 1.83f, 1.42f, 0.34f, 2.72f}, 0x1d3f8f,
     "« 2024 Hyundai Elantra N » par Ddiaz Design (Sketchfab), CC BY-NC 4.0"},
    {"v60", "Volvo", "V60 Polestar", "cars/v60.wfm", {350, 4.9f, 250, 1796}, 7, {4.63f, 1.87f, 1.48f, 0.34f, 2.78f}, 0x1d3f8f,
     "« Volvo V60 Polestar (2013) » par Myedsu (Sketchfab), CC BY 4.0"},
    {"rrsport", "Land Rover", "Range Rover Sport", "cars/rrsport.wfm", {400, 5.9f, 242, 2330}, 5, {4.95f, 2.05f, 1.82f, 0.38f, 3.0f}, 0x0f1014,
     "« Land Rover Range Rover Sport - 2023 » par Unn Interativa (Sketchfab), CC BY 4.0"},
    {"mustang", "Ford", "Mustang Shelby GT500", "cars/mustang.wfm", {760, 3.6f, 290, 1916}, 6, {4.82f, 1.92f, 1.38f, 0.35f, 2.72f}, 0x1d3f8f,
     "« 2020 Ford Mustang Shelby GT500 » par Ddiaz Design (Sketchfab), CC BY-NC-SA 4.0"},
    {"sf90", "Ferrari", "SF90 XX Stradale", "cars/sf90.wfm", {1030, 2.3f, 320, 1560}, 10, {4.85f, 1.97f, 1.19f, 0.35f, 2.65f}, 0xb3122b,
     "« Ferrari SF90 XX | www.vecarz.com » par vecarz (Sketchfab), CC BY 4.0"},
};
constexpr int TRAFIC_MODELES = 4;   // WFS-13 : modèles chargés par le trafic (les 4 premières voitures), variété : WFS-67
constexpr int CAR_COUNT = sizeof(CARS) / sizeof(CARS[0]);

inline const CarDef& carById(const char* id) {
    for (const auto& c : CARS) if (std::strcmp(c.id, id) == 0) return c;
    return CARS[0];
}

// performances. WFS-188 (RG-01 à RG-04) : recalées sur la fiche (CarSpec, inchangée) : la poussée a0 et le plafond top
// sont calculés pour que le 0-100 mesuré en jeu (plein gaz, sans nitro, pas de 1/60 s) vaille spec.s100 et que la vitesse
// atteinte sans nitro vaille 99,5 % de spec.vmax ; freinage = facteur de Phys::brake pour un 100 → 0 de 33 à 40 m selon
// l'adhérence, la masse et la puissance. Même formule que Player::update (player.cpp), simulée ici une fois par voiture.
// nitroMult (1,3), nitroGain, nitroBurn, handling, mass, acc : inchangés (carPerf() du jeu web).
struct CarPerf { float top, a0, wb, acc, handling, mass, nitroMult, nitroGain, nitroBurn; float freinage = 1; bool referenceWeb = false; };
// WFS-188 : vrai = performances du jeu web (carPerf() de js/cars/catalog.js, bas-côté plein) pour les tests de fidélité au
// jeu web (test_course, course_web_ref.txt) ; le jeu utilise toujours les performances recalées sur la fiche
inline bool g_perfWeb = false;
inline float clampN(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
namespace detail {
constexpr double CP_M = 0.006, CP_MAX = 200.0 * 60.0, CP_DT = 1.0 / 60.0;   // = M, MAX_SPEED de track.h
// 0-100 km/h (s) en plein gaz sans nitro, comme Player::update (accélération : poussée − 0,35 × résistance, sans frein moteur)
inline double tempsCent(double a0, double topU) {
    const auto& c = g_configJeu.conduite;
    double v = 0, t = 0;
    while (v * CP_M * 3.6 < 100 && t < 60) {
        const double av = std::abs(v * CP_M), resist = (c.roll + c.drag * av * av) / CP_M;
        const double push = a0 / CP_M * std::max(0.0, 1 - std::pow(std::max(0.0, v) / topU, 2.2));
        v += (push - resist * 0.35) * CP_DT; t += CP_DT;
    }
    return t;
}
// 100 → 0 km/h (m), frein tenu : freinage puis, sous vRecul, la marche arrière qui enchaîne (comme Player::update)
inline double distanceArret(double facteur) {
    const auto& c = g_configJeu.conduite;
    double v = 100 / 3.6 / CP_M, d = 0;
    for (int i = 0; i < 6000 && v > 0; i++) {
        const double av = std::abs(v * CP_M), resist = (c.roll + c.drag * av * av + c.engineBrake) / CP_M;
        if (v * CP_M < c.vRecul) v -= CP_MAX * 0.1 * CP_DT;                 // REV_ACC
        else v = std::max(0.0, v - (c.brake * facteur / CP_M + resist) * CP_DT);
        d += std::max(0.0, v) * CP_M * CP_DT;
    }
    return d;
}
inline CarPerf etalonner(const CarDef& car) {
    const CarSpec& s = car.spec;
    const auto& c = g_configJeu.conduite;
    CarPerf p{s.vmax / 260.f, (27.8f / s.s100) * 1.45f, car.d.wb, clampN((27.8f / s.s100) / 4.6f, 0.55f, 1.8f),
              0.75f + car.grip * 0.05f, s.kg / 1450.f, 1.3f, 0.12f, 0.26f};
    // vitesse visée (unités/s) : 99,5 % de la fiche ; plafond top tel que poussée = 0,35 × résistance à cette vitesse
    const double ve = 0.995 * s.vmax / 3.6 / CP_M, vms = ve * CP_M;
    double a0 = p.a0, topU = CP_MAX * p.top;
    for (int it = 0; it < 12; it++) {
        const double reste = 1 - 0.35 * (c.roll + c.drag * vms * vms) / a0;
        topU = ve / std::pow(std::max(0.05, reste), 1 / 2.2);
        const double t = tempsCent(a0, topU);
        a0 *= t / s.s100;                                   // le temps varie comme 1 / a0
    }
    p.a0 = (float)a0; p.top = (float)(topU / CP_MAX);
    // RG-04 : 100 → 0 visé de 33,3 à 40,3 m : adhérence (40 %), masse (35 %), puissance (25 %, les sportives freinent mieux)
    const double gr = clampN((car.grip - 5) / 5.f, 0, 1), ma = clampN((s.kg - 1035) / 1295.f, 0, 1), pu = clampN((s.ch - 130) / 470.f, 0, 1);
    const double cible = 33.3 + 7 * (0.4 * (1 - gr) + 0.35 * ma + 0.25 * (1 - pu));
    double f = 1;
    for (int it = 0; it < 12; it++) f *= distanceArret(f) / cible;
    p.freinage = (float)f;
    return p;
}
} // namespace detail

inline CarPerf carPerf(const CarDef& c) {
    if (g_perfWeb) {   // performances du jeu web (référence des tests)
        const CarSpec& s = c.spec;
        CarPerf w{s.vmax / 260.f, (27.8f / s.s100) * 1.45f, c.d.wb, clampN((27.8f / s.s100) / 4.6f, 0.55f, 1.8f), 0.75f + c.grip * 0.05f, s.kg / 1450.f, 1.3f, 0.12f, 0.26f};
        w.referenceWeb = true;
        return w;
    }
    // une fois pour les 20 voitures du catalogue (initialisation statique sûre entre threads) ; autre fiche : calculée
    static const auto table = [] { std::array<CarPerf, sizeof(CARS) / sizeof(CARS[0])> t{}; for (size_t i = 0; i < t.size(); i++) t[i] = detail::etalonner(CARS[i]); return t; }();
    const size_t i = (size_t)(&c - CARS);
    if (&c >= CARS && i < table.size()) return table[i];
    return detail::etalonner(c);
}

} // namespace wfs
