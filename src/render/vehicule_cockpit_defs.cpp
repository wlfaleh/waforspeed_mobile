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
     Essuie::Paralleles, 0.650f, 0.450f, 1.66f, 0.442f},
    // M4 Competition G82 (avant restylage) : volant M rond, épais, coutures M en bas (bleu clair, bleu foncé, rouge),
    // boutons M1 / M2 rouges, palettes ; Live Cockpit Professional 12,3 po (292 × 110 mm), écran central 10,25 po posé sur
    // la planche (244 × 91 mm), carbone et alu ; balais 600 / 475 mm
    {"m4", 0.370f, 0.038f, 3, 0.f, true, false, false, 0x3d3d3f, {0x0066b1, 0x1c2c5c, 0xe7222e}, 0x2b2c30, 0xe7222e,
     {0.f, -0.30f, -0.76f}, -0.35f,
     Combine::BmwLive, 0.292f, 0.110f, {0.f, -0.24f, -0.88f}, 0xe9eef4, 0x0d0f12, 0xe7222e,
     0.244f, 0.091f, {0.38f, -0.20f, -0.80f}, -0.15f,
     -0.18f, -0.80f, -0.22f, -1.15f, 0x141415, 0x1b1c1e, 0x101113, 0.f, 0.15f, true,
     Essuie::Paralleles, 0.600f, 0.475f, 1.66f, 0.335f},
    // 911 Carrera 4S (992.1) : volant rond 3 branches fines (375 mm), 5 cadrans sous une visière (compte-tours analogique
    // au centre), écran PCM 10,9 po encastré à mi-hauteur (245 × 130 mm), planche plate et basse, alu brossé ; balai 525 mm
    {"p911", 0.375f, 0.036f, 3, 0.f, true, false, false, 0x3d3d3f, {0, 0, 0}, 0x6e7177, 0,
     {0.f, -0.28f, -0.72f}, -0.31f,
     Combine::Porsche5, 0.300f, 0.100f, {0.f, -0.22f, -0.83f}, 0xf2f2f0, 0x0a0b0c, 0xe0201a,
     0.245f, 0.130f, {0.37f, -0.33f, -0.82f}, 0.f,
     -0.16f, -0.72f, -0.20f, -1.00f, 0x151516, 0x1c1c1e, 0xe9ebec, 1.f, 0.35f, false,
     Essuie::Paralleles, 0.525f, 0.450f, 1.57f, 0.297f},
    // SF90 Stradale : volant à méplat, 2 branches larges qui portent les commandes (manettino rouge à droite, démarreur
    // en bas), très grandes palettes carbone ; un seul écran courbe 16 po derrière le volant (≈ 380 × 143 mm), pas d'écran
    // central : la carte est dans le tiers droit du combiné ; carbone et Alcantara ; essuie-glaces non mesurables (2 balais
    // en tandem supposés, comme les 3 autres)
    {"sf90", 0.355f, 0.035f, 2, 0.18f, true, true, true, 0x3d3d3f, {0, 0, 0}, 0x1d1d20, 0xe00b0b,
     {0.f, -0.27f, -0.70f}, -0.30f,
     Combine::FerrariIncurve, 0.380f, 0.143f, {0.f, -0.22f, -0.82f}, 0xf4f4f4, 0x090a0c, 0xff2b14,
     0.f, 0.f, {0.f, 0.f, 0.f}, 0.f,
     -0.15f, -0.70f, -0.25f, -1.30f, 0x121213, 0x18181a, 0x101113, 0.f, 0.15f, true,
     Essuie::Paralleles, 0.600f, 0.500f, 1.57f, 0.250f},
    // ---------- WFS-201 : lot 2 (génération du modèle .wfm du jeu, décision du PO ; cotes : expert-realisme-3d, 2026-11-07, journal)
    // A 45 (W176 restylée, le .wfm « A45 AMG 2018 ») : volant AMG à méplat, repère rouge à midi, palettes ; 2 cadrans dans des tubes et
    // écran couleur au milieu (pas de MBUX : W177 seulement) ; écran de 8 po posé sur la planche ; bagues rouges ; balais 600 / 475 (AC)
    {"a45", 0.370f, 0.036f, 3, 0.15f, true, false, false, 0x3d3d3f, {0, 0, 0}, 0x8f9398, 0xc8102e,
     {0.f, -0.31f, -0.74f}, -0.36f,
     Combine::AudiVirtual, 0.300f, 0.120f, {0.f, -0.23f, -0.87f}, 0xeef2f6, 0x0b0c0e, 0xe2231a,
     0.177f, 0.100f, {0.36f, -0.11f, -0.80f}, 0.f,
     -0.17f, -0.78f, -0.22f, -1.10f, 0x1c1d20, 0x232428, 0xc8102e, 0.f, 0.40f, false,
     Essuie::Paralleles, 0.600f, 0.475f, 1.60f, 0.357f, 0, false, false, true},
    // S3 berline (8V, le .wfm « 2014 Audi S3 Limousine ») : volant S à méplat, palettes ; 2 cadrans gris à aiguille (pas de virtual
    // cockpit : 8V restylée seulement) ; écran MMI de 7 po qui sort du haut de la planche ; alu brossé ; balais 650 / 475 (AC)
    {"s3", 0.359f, 0.035f, 3, 0.15f, true, false, false, 0x3d3d3f, {0, 0, 0}, 0x8f9398, 0,
     {0.f, -0.30f, -0.75f}, -0.37f,
     Combine::AudiVirtual, 0.290f, 0.120f, {0.f, -0.23f, -0.87f}, 0xf0f2f4, 0x1e2023, 0xd00a0a,
     0.152f, 0.091f, {0.36f, -0.13f, -0.92f}, 0.f,
     -0.17f, -0.80f, -0.22f, -1.12f, 0x1c1d20, 0x232428, 0xe9ebec, 1.f, 0.35f, false,
     Essuie::Paralleles, 0.650f, 0.475f, 1.60f, 0.352f},
    // Giulia Quadrifoglio : volant à méplat, bouton de démarrage rouge, grandes palettes alu fixées sur la colonne ; 2 cadrans dans
    // des tubes (régime à gauche, vitesse à droite) et TFT au milieu ; écran 8,8 po intégré, tourné vers le conducteur ; carbone ;
    // balais 600 / 450 (AC)
    {"giulia", 0.362f, 0.038f, 3, 0.12f, true, false, true, 0x3d3d3f, {0, 0, 0}, 0x2b2c30, 0xb3122b,
     {0.f, -0.30f, -0.74f}, -0.35f,
     Combine::AlfaCadrans, 0.300f, 0.120f, {0.f, -0.22f, -0.86f}, 0xf2f2f0, 0x0a0b0c, 0xe0201a,
     0.209f, 0.079f, {0.33f, -0.20f, -0.80f}, -0.17f,
     -0.18f, -0.80f, -0.23f, -1.20f, 0x151516, 0x1c1c1e, 0x101113, 0.f, 0.15f, true,
     Essuie::Paralleles, 0.600f, 0.450f, 1.60f, 0.412f, 0, false, true},
    // 208 (II, i-Cockpit 3D) : petit volant bas, plat en haut et en bas, palettes ; combiné 10 po lu au-dessus du volant ; écran
    // central 10 po tourné vers le conducteur, touches chromées ; balais 650 / 400 (AC)
    {"p208", 0.350f, 0.034f, 3, 0.20f, true, false, false, 0x3d3d3f, {0, 0, 0}, 0x2b2c30, 0,
     {0.f, -0.36f, -0.66f}, -0.45f,
     Combine::PeugeotICockpit, 0.236f, 0.094f, {0.f, -0.16f, -0.88f}, 0xe6ecf2, 0x0a0d10, 0xff3b30,
     0.221f, 0.125f, {0.36f, -0.24f, -0.72f}, -0.12f,
     -0.11f, -0.85f, -0.20f, -1.05f, 0x1c1d20, 0x2a2b2e, 0xd2d6da, 1.f, 0.20f, false,
     Essuie::Paralleles, 0.650f, 0.400f, 1.62f, 0.46f, 0.20f},
    // Gallardo LP560-4 Spyder : volant rond 3 branches, palettes e-gear fixées sur la colonne ; grand compte-tours au centre, vitesse et
    // jauges autour, sous une visière ; petit écran de navigation bas ; alu ; balais 650 / 650 (AC, disposition non mesurable)
    {"gallardo", 0.360f, 0.036f, 3, 0.f, true, false, false, 0x3d3d3f, {0, 0, 0}, 0x8f9398, 0,
     {0.f, -0.28f, -0.70f}, -0.30f,
     Combine::LamboCompteTours, 0.300f, 0.130f, {0.f, -0.20f, -0.82f}, 0xf2f2f2, 0x0a0a0b, 0xff7a00,
     0.144f, 0.081f, {0.36f, -0.30f, -0.70f}, -0.10f,
     -0.14f, -0.72f, -0.24f, -1.25f, 0x1a1a1c, 0x1c1c1e, 0xe9ebec, 1.f, 0.35f, false,
     Essuie::Paralleles, 0.650f, 0.650f, 1.50f, 0.235f, 0, false, true},
    // Mustang Shelby GT500 (S550) : volant rond, palettes ; combiné numérique 12 po (compte-tours en arc) ; écran SYNC 3 de 8 po ;
    // double casquette, carbone ; balais 560 / 510 (AC)
    {"mustang", 0.370f, 0.038f, 3, 0.f, true, false, false, 0x3d3d3f, {0, 0, 0}, 0x2b2c30, 0,
     {0.f, -0.31f, -0.76f}, -0.36f,
     Combine::FordDigital, 0.285f, 0.107f, {0.f, -0.23f, -0.87f}, 0xf4f6f8, 0x08090b, 0xe31b23,
     0.174f, 0.105f, {0.36f, -0.30f, -0.78f}, 0.f,
     -0.16f, -0.82f, -0.24f, -1.25f, 0x141415, 0x1b1c1e, 0x101113, 0.f, 0.15f, true,
     Essuie::Paralleles, 0.560f, 0.510f, 1.57f, 0.389f},
    // Model 3 (2018) : volant 2 branches avec 2 molettes, sans palettes ; aucun combiné : écran 15 po horizontal (vitesse, « D », carte),
    // planche très basse, bandeau bois ; balais 650 / 475 (V : guide d'entretien Tesla)
    {"model3", 0.350f, 0.035f, 2, 0.f, false, false, false, 0x3d3d3f, {0, 0, 0}, 0x1d1d20, 0,
     {0.f, -0.32f, -0.74f}, -0.38f,
     Combine::Aucun, 0.f, 0.f, {0.f, 0.f, 0.f}, 0xf4f6f8, 0x16191d, 0xe31937,
     0.323f, 0.202f, {0.36f, -0.22f, -0.68f}, 0.f,
     -0.24f, -0.80f, -0.26f, -1.15f, 0x1a1a1c, 0x232428, 0x6b5444, 0.f, 0.45f, false,
     Essuie::Paralleles, 0.650f, 0.475f, 1.60f, 0.385f, 0, true},
    // X5 (G05 LCI, le .wfm « 2024 X5 xDrive40i ») : volant M Sport (coutures M), palettes ; Curved Display : combiné 12,3 po et écran
    // 14,9 po côte à côte sur une même vitre courbe tournée vers le conducteur (hauts alignés) ; combiné placé pour être vu en entier
    // entre le moyeu et le haut de la jante (œil haut du SUV : y −0,237, E) ; position haute ; alu ; balais 650 / 500 (AC)
    {"x5", 0.370f, 0.038f, 3, 0.f, true, false, false, 0x3d3d3f, {0x0066b1, 0x1c2c5c, 0xe7222e}, 0x2b2c30, 0,
     {0.f, -0.33f, -0.70f}, -0.45f,
     Combine::BmwLive, 0.292f, 0.110f, {0.f, -0.237f, -0.86f}, 0xe9eef4, 0x0d0f12, 0xe7222e,
     0.354f, 0.133f, {0.33f, -0.249f, -0.82f}, -0.30f,
     -0.22f, -0.82f, -0.28f, -1.20f, 0x1c1d20, 0x2a2b2e, 0xe9ebec, 1.f, 0.35f, false,
     Essuie::Paralleles, 0.650f, 0.500f, 1.62f, 0.677f},
};

const InterieurDef* interieurPour(const char* carId) {
    if (!carId) return nullptr;
    for (const InterieurDef& d : DEFS) if (std::strcmp(d.car, carId) == 0) return &d;
    return nullptr;
}
int nombreInterieurs() { return (int)(sizeof(DEFS) / sizeof(DEFS[0])); }
const InterieurDef& interieur(int i) { return DEFS[std::clamp(i, 0, nombreInterieurs() - 1)]; }

} // namespace wfs::cockpit
