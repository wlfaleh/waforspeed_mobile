// Feux de croisement du joueur (WFS-218) : flux de la paire = flux sourcé (±5 %, PB-LUM), coupure à −0,57° et remontée de
// 15° côté droit (RG-01), presque plus de lumière au-dessus (pas de mur éclairé jusqu'au toit), route et pieds de façade
// proches éclairés, portée utile 60-80 m (RG-02), teintes 3500 K / 5700 K de luminance 1.
// usage : test_phares
#include <cmath>
#include <cstdio>
#include "../src/render/feux_modele.h"
#include "../src/render/phares.h"
using namespace wfs;
using namespace wfs::phares;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

constexpr double RAD = 3.14159265358979 / 180;
// ∫ intensiteRelative cos(el) dΩ, pas de 0,05°
static double integrale() {
    double s = 0; const double pas = 0.05;
    for (double el = -90 + pas / 2; el < 90; el += pas) {
        double ligne = 0;
        for (double az = -180 + pas / 2; az < 180; az += pas) ligne += intensiteRelative((float)el, (float)az);
        s += ligne * std::cos(el * RAD);
    }
    return s * (pas * RAD) * (pas * RAD);
}
// éclairement (lux) d'un point (x à droite, y en haut, z devant) par un phare en (0, h, 0), surface de normale n
static double lux(bool estLed, double x, double y, double z, double h, double nx, double ny, double nz) {
    const double dx = x, dy = y - h, dz = z, d = std::sqrt(dx * dx + dy * dy + dz * dz);
    const double el = std::asin(dy / d) / RAD, az = std::atan2(dx, dz) / RAD;
    const double cosI = std::max(0.0, -(dx * nx + dy * ny + dz * nz) / d);
    return creteCd(estLed) * intensiteRelative((float)el, (float)az) * cosI / (d * d);
}

int main() {
    // flux
    const double I = integrale();
    std::printf("intégrale %.5f sr (constante %.5f)\n", I, INTEGRALE_SR);
    VERIFIE(std::abs(I / INTEGRALE_SR - 1) < 0.01, "INTEGRALE_SR à recalculer : %.5f", I);
    for (bool l : {false, true}) {
        const double flux = 2 * creteCd(l) * I, cible = l ? FLUX_LED_LM : FLUX_HALOGENE_LM;
        std::printf("%s : crête %.0f cd par phare, flux de la paire %.0f lm (sourcé %.0f lm)\n", l ? "LED" : "halogène", creteCd(l), flux, cible);
        VERIFIE(std::abs(flux / cible - 1) <= 0.05, "flux hors ±5 %%");
    }
    // RG-01 : coupure
    VERIFIE(intensiteRelative(-1.0f, 0) > 0.95f, "cœur sous la coupure");
    VERIFIE(intensiteRelative(0.2f, -5) < 0.02f, "au-dessus de la coupure à gauche");
    VERIFIE(std::abs(intensiteRelative(COUPURE_DEG, 0) - 0.5f * (1 + RESIDU)) < 0.02f, "coupure à −0,57°");
    VERIFIE(intensiteRelative(0.5f, 10) > 0.5f, "côté droit relevé (15°)");
    VERIFIE(intensiteRelative(0.5f, -10) < 0.02f, "côté gauche non relevé");
    VERIFIE(intensiteRelative(3.f, 10) < 0.02f, "au-dessus de l'épaule");
    // mur clair à 6 m à droite (bas-côté), à 12 m devant, phare à 0,65 m : pied éclairé, haut presque noir
    const double h = 0.65, pied = lux(false, 6, 0.2, 12, h, -1, 0, 0), toit = lux(false, 6, 6, 12, h, -1, 0, 0), etage = lux(false, 6, 3, 12, h, -1, 0, 0);
    std::printf("mur à 6 m à droite, 12 m devant : pied %.2f lx, 3 m %.2f lx, 6 m %.2f lx\n", pied, etage, toit);
    VERIFIE(pied > 3, "pied de façade proche éclairé"); VERIFIE(etage < pied * 0.03 && toit < pied * 0.03, "mur éclairé jusqu'au toit");
    // RG-02 : route (deux phares à ±0,65 m) : éclairement à 10, 30, 60, 80, 120 m
    double route[5]; const double dist[5] = {10, 30, 60, 80, 120};
    for (int i = 0; i < 5; i++) route[i] = lux(false, -0.65, 0, dist[i], h, 0, 1, 0) + lux(false, 0.65, 0, dist[i], h, 0, 1, 0);
    std::printf("route (paire halogène) : 10 m %.2f lx, 30 m %.3f, 60 m %.3f, 80 m %.3f, 120 m %.4f lx\n", route[0], route[1], route[2], route[3], route[4]);
    VERIFIE(route[0] > 3 && route[0] < 30, "route à 10 m");
    VERIFIE(route[4] < route[2] * 0.25, "au-delà de la portée");
    // portée géométrique : la coupure (−1 %) touche la chaussée à h / 0,01 m ; bas-côté droit (cible verticale de 0,5 m à
    // 70 m, 3,5 m à droite) éclairé ; œil d'un conducteur venant en face (1,1 m, 50 m, 3,5 m à gauche) ≤ 0,4 lx (B50L, R112)
    const double portee = h / std::tan(-COUPURE_DEG * RAD);
    const double cible = lux(false, 3.5 - 0.65, 0.5, 70, h, 0, 0, -1) + lux(false, 3.5 + 0.65, 0.5, 70, h, 0, 0, -1);
    const double oeil = lux(false, -3.5 - 0.65, 1.1, 50, h, 0, 0, -1) + lux(false, -3.5 + 0.65, 1.1, 50, h, 0, 0, -1);
    std::printf("portée %.0f m ; bas-côté droit à 70 m %.2f lx ; œil en face à 50 m %.3f lx\n", portee, cible, oeil);
    VERIFIE(portee >= 60 && portee <= 80, "portée utile 60-80 m"); VERIFIE(cible > 1, "bas-côté droit à 70 m"); VERIFIE(oeil < 0.4, "éblouissement B50L");
    // teintes de luminance 1
    for (const vec3& c : {TEINTE_HALOGENE, TEINTE_LED}) VERIFIE(std::abs(0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z - 1) < 0.01f, "luminance de la teinte");
    VERIFIE(!led("abarth") && led("golf") && !led(nullptr), "LED selon la voiture");
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("test_phares : OK\n");
    return 0;
}
