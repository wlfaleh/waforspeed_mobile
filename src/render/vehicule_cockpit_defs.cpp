// WaForSpeed — intérieurs par modèle de la vue cockpit (WFS-197) : définitions chiffrées et choix de l'intérieur, sans rendu
// (tests/test_cockpit.cpp). Sources et statut de chaque valeur : journal de WFS-197 (expert-realisme-3d, 2026-11-06) ;
// V = vérifié, AC = à confirmer (presse, revendeur : pages des constructeurs bloquées par le réseau des agents), E = estimé
// (méthode SAE J1100, proportions sur photo), NM = non mesurable. Profondeur (z) des combinés : reculée de 6 à 8 cm par rapport
// à l'estimation de l'expert pour que le haut de la jante (z ≈ −0,82) reste devant l'écran (estimations E toutes les deux).
#include <algorithm>
#include <cstring>
#include "vehicule_cockpit.h"

namespace wfs::cockpit {

// ---------------------------------------------------------------- définitions par modèle
// Repère de l'œil (m). Le centre de la voiture est à x = +0,36 (œil du conducteur à gauche, comme le cockpit commun).
// Valeurs : WFS-197, journal (sources et statut de chaque mesure ; « à confirmer » = page constructeur non ouverte par le réseau
// des agents, « estimé » = proportion sur photo, méthode au journal).
static const InterieurDef DEFS[] = {
    // Golf 8 GTI (avant restylage 2020-2024) : volant 3 branches à méplat (≈ 0,18 R), cuir noir à coutures rouges et agrafe
    // GTI rouge sur la branche basse, palettes DSG ; Digital Cockpit Pro 10,25 po (244 × 91 mm) et écran central 10 po
    // (232 × 104 mm) dans le même îlot, tourné de 9° vers le conducteur ; liseré rouge sur toute la largeur ; balais 650 / 450 mm
    {"golf", 0.370f, 0.034f, 3, 0.18f, true, false, false, 0x3d3d3f, {0, 0, 0}, 0x1a1a1c, 0xbe141e,
     {0.f, -0.31f, -0.76f}, -0.38f,
     Combine::VwDigital, 0.244f, 0.091f, {0.f, -0.24f, -0.86f}, 0xdfe6ee, 0x0e1116, 0xff2a2a,
     0.232f, 0.104f, {0.33f, -0.24f, -0.74f}, -0.16f,
     -0.17f, -0.78f, -0.22f, -1.10f, 0x1c1d20, 0x232428, 0xbe141e, 0.f, 0.40f, false,
     Essuie::Paralleles, 0.650f, 0.450f, 1.66f, 0.80f},
    // M4 Competition G82 (avant restylage) : volant M rond, épais, coutures M en bas (bleu clair, bleu foncé, rouge),
    // boutons M1 / M2 rouges, palettes ; Live Cockpit Professional 12,3 po (292 × 110 mm), écran central 10,25 po posé sur
    // la planche (244 × 91 mm), carbone et alu ; balais 600 / 475 mm
    {"m4", 0.370f, 0.038f, 3, 0.f, true, false, false, 0x3d3d3f, {0x0066b1, 0x1c2c5c, 0xe7222e}, 0x2b2c30, 0xe7222e,
     {0.f, -0.30f, -0.76f}, -0.35f,
     Combine::BmwLive, 0.292f, 0.110f, {0.f, -0.24f, -0.88f}, 0xe9eef4, 0x0d0f12, 0xe7222e,
     0.244f, 0.091f, {0.38f, -0.20f, -0.80f}, -0.15f,
     -0.18f, -0.80f, -0.22f, -1.15f, 0x141415, 0x1b1c1e, 0x101113, 0.f, 0.15f, true,
     Essuie::Paralleles, 0.600f, 0.475f, 1.66f, 0.79f},
    // 911 Carrera 4S (992.1) : volant rond 3 branches fines (375 mm), 5 cadrans sous une visière (compte-tours analogique
    // au centre), écran PCM 10,9 po encastré à mi-hauteur (245 × 130 mm), planche plate et basse, alu brossé ; balai 525 mm
    {"p911", 0.375f, 0.036f, 3, 0.f, true, false, false, 0x3d3d3f, {0, 0, 0}, 0x6e7177, 0,
     {0.f, -0.28f, -0.72f}, -0.31f,
     Combine::Porsche5, 0.300f, 0.100f, {0.f, -0.22f, -0.83f}, 0xf2f2f0, 0x0a0b0c, 0xe0201a,
     0.245f, 0.130f, {0.37f, -0.33f, -0.82f}, 0.f,
     -0.16f, -0.72f, -0.20f, -1.00f, 0x151516, 0x1c1c1e, 0xe9ebec, 1.f, 0.35f, false,
     Essuie::Paralleles, 0.525f, 0.450f, 1.57f, 0.785f},
    // SF90 Stradale : volant à méplat, 2 branches larges qui portent les commandes (manettino rouge à droite, démarreur
    // en bas), très grandes palettes carbone ; un seul écran courbe 16 po derrière le volant (≈ 380 × 143 mm), pas d'écran
    // central : la carte est dans le tiers droit du combiné ; carbone et Alcantara ; essuie-glaces non mesurables (2 balais
    // en tandem supposés, comme les 3 autres)
    {"sf90", 0.355f, 0.035f, 2, 0.18f, true, true, true, 0x3d3d3f, {0, 0, 0}, 0x1d1d20, 0xe00b0b,
     {0.f, -0.27f, -0.70f}, -0.30f,
     Combine::FerrariIncurve, 0.380f, 0.143f, {0.f, -0.22f, -0.82f}, 0xf4f4f4, 0x090a0c, 0xff2b14,
     0.f, 0.f, {0.f, 0.f, 0.f}, 0.f,
     -0.15f, -0.70f, -0.25f, -1.30f, 0x121213, 0x18181a, 0x101113, 0.f, 0.15f, true,
     Essuie::Paralleles, 0.600f, 0.500f, 1.57f, 0.775f},
};

const InterieurDef* interieurPour(const char* carId) {
    if (!carId) return nullptr;
    for (const InterieurDef& d : DEFS) if (std::strcmp(d.car, carId) == 0) return &d;
    return nullptr;
}
int nombreInterieurs() { return (int)(sizeof(DEFS) / sizeof(DEFS[0])); }
const InterieurDef& interieur(int i) { return DEFS[std::clamp(i, 0, nombreInterieurs() - 1)]; }

} // namespace wfs::cockpit
