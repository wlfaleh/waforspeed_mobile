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
    {"m4", 0.370f, 0.038f, 3, 0.f, true, false, false, 0x3d3d3f, {0, 0, 0}, 0x2b2c30, 0xe7222e,   // WFS-404 (ANO-209) : bandes de couleur de la marque retirées
     {0.f, -0.30f, -0.76f}, -0.35f,
     Combine::BmwLive, 0.292f, 0.110f, {0.f, -0.24f, -0.88f}, 0xe9eef4, 0x0d0f12, 0xe7222e,
     0.244f, 0.091f, {0.38f, -0.20f, -0.80f}, -0.15f,
     -0.18f, -0.80f, -0.22f, -1.15f, 0x141415, 0x1b1c1e, 0x101113, 0.f, 0.15f, true,
     Essuie::Paralleles, 0.600f, 0.475f, 1.66f, 0.335f, 0, false, false, false, true},   // WFS-404 : boutons rouges des branches gardés sans les bandes
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
    // ---------- WFS-202 : lot 3 (génération du modèle .wfm du jeu, décision du PO ; cotes : expert-realisme-3d, 2026-11-07, journal ;
    // assise = toit − 0,05 − (garde au toit − 0,102)·cos 8° − 0,095, même méthode que le lot 2)
    // Abarth 500 (2008-2015, texture du .wfm : combiné rond concentrique) : volant épais à méplat ; un seul cadran rond (régime en
    // anneau intérieur, vitesse en anneau extérieur, LCD au centre) ; pas d'écran central d'origine : carte dans le cadran (comme la
    // SF90) ; façade verticale, alu ; balais 600 / 350 (AC)
    {"abarth", 0.365f, 0.038f, 3, 0.15f, false, false, false, 0x3d3d3f, {0, 0, 0}, 0x2b2c30, 0,
     {0.f, -0.30f, -0.72f}, -0.42f,
     Combine::FerrariIncurve, 0.170f, 0.170f, {0.f, -0.21f, -0.82f}, 0xff4a1c, 0x0a0a0a, 0xe8101a,
     0.f, 0.f, {0.f, 0.f, 0.f}, 0.f,
     -0.16f, -0.72f, -0.20f, -1.05f, 0x1b1b1d, 0x232428, 0x8f9398, 1.f, 0.35f, false,
     Essuie::Paralleles, 0.600f, 0.350f, 1.60f, 0.463f},
    // Compass II restylée (2021) : gros volant rond ; combiné numérique 10,25 po ; écran Uconnect 5 de 10,1 po flottant au-dessus
    // d'aérateurs fins ; bandeau cousu et filet métal ; position haute ; balais 600 / 450 (AC)
    {"compass", 0.380f, 0.036f, 3, 0.f, false, false, false, 0x3d3d3f, {0, 0, 0}, 0x1d1d20, 0,
     {0.f, -0.32f, -0.72f}, -0.43f,
     Combine::VwDigital, 0.244f, 0.091f, {0.f, -0.24f, -0.87f}, 0xe6ecf2, 0x0b0d10, 0xe31b23,
     0.218f, 0.136f, {0.36f, -0.13f, -0.80f}, 0.f,
     -0.20f, -0.82f, -0.26f, -1.18f, 0x1c1d20, 0x2a2b2e, 0x8f9398, 1.f, 0.35f, false,
     Essuie::Paralleles, 0.600f, 0.450f, 1.60f, 0.594f},
    // Corsa F (2020) : combiné numérique 7 po sous une petite visière ; écran 7 po intégré haut entre les aérateurs ; bandeau noir
    // laqué ; balais 600 / 400 (AC)
    {"corsa", 0.365f, 0.034f, 3, 0.f, false, false, false, 0x3d3d3f, {0, 0, 0}, 0x2b2c30, 0,
     {0.f, -0.31f, -0.74f}, -0.38f,
     Combine::VwDigital, 0.155f, 0.087f, {0.f, -0.23f, -0.86f}, 0xe6ecf2, 0x0a0d10, 0xff3b30,
     0.155f, 0.087f, {0.36f, -0.17f, -0.78f}, -0.10f,
     -0.17f, -0.80f, -0.22f, -1.08f, 0x1c1d20, 0x232428, 0x101113, 0.f, 0.15f, false,
     Essuie::Paralleles, 0.600f, 0.400f, 1.62f, 0.448f},
    // Elantra N (CN7 restylée, 2024) : volant à palettes, 2 boutons N bleus en bas des branches ; combiné 10,25 po (compte-tours
    // rond au centre, rapport dedans) et écran 10,25 po sur une même vitre tournée vers le conducteur ; fils bleus ; balais 600 / 450 (AC)
    {"elantra", 0.370f, 0.037f, 3, 0.f, true, false, false, 0x3d3d3f, {0, 0, 0}, 0x2b2c30, 0x0072ce,
     {0.f, -0.30f, -0.73f}, -0.36f,
     Combine::FerrariIncurve, 0.244f, 0.091f, {0.f, -0.23f, -0.87f}, 0x9fd3ff, 0x0a0e16, 0xe0201a,
     0.244f, 0.091f, {0.28f, -0.23f, -0.84f}, -0.17f,
     -0.15f, -0.80f, -0.21f, -1.20f, 0x141415, 0x1b1c1e, 0x0072ce, 0.f, 0.40f, false,
     Essuie::Paralleles, 0.600f, 0.450f, 1.60f, 0.350f, 0, false, false, false, true},
    // GT86 (génération non mesurable, branches alu d'avant 2017) : très petit volant rond (363,5 mm, ± 1 % des deux générations) ;
    // grand compte-tours au centre (le socle met la vitesse à droite, la vraie est à gauche : écart au journal) ; écran 6,1 po bas
    // dans la console ; planche basse et plate ; balais 560 / 510 (AC)
    {"gt86", 0.3635f, 0.034f, 3, 0.f, false, false, false, 0x3d3d3f, {0, 0, 0}, 0x8f9398, 0,
     {0.f, -0.28f, -0.70f}, -0.30f,
     Combine::LamboCompteTours, 0.300f, 0.120f, {0.f, -0.20f, -0.82f}, 0xf2f2f2, 0x0a0a0b, 0xe0201a,
     0.135f, 0.076f, {0.36f, -0.26f, -0.74f}, 0.f,
     -0.13f, -0.74f, -0.20f, -1.10f, 0x151516, 0x1c1c1e, 0x101113, 0.f, 0.15f, false,
     Essuie::Paralleles, 0.560f, 0.510f, 1.55f, 0.306f},
    // Range Rover Sport (L461, 2023) : volant 3 branches horizontales (RG-02 dit 4 : faux, journal), palettes ; combiné 13,7 po ;
    // écran courbe flottant 13,1 po ; planche longue, alu ; position très haute ; balais 600 / 500 (AC)
    {"rrsport", 0.380f, 0.038f, 3, 0.f, true, false, false, 0x3d3d3f, {0, 0, 0}, 0x2b2c30, 0,
     {0.f, -0.33f, -0.72f}, -0.45f,
     Combine::AudiVirtual, 0.326f, 0.122f, {0.f, -0.25f, -0.88f}, 0xe9eef4, 0x0d0f12, 0xd0021b,
     0.290f, 0.163f, {0.36f, -0.26f, -0.80f}, 0.f,
     -0.24f, -0.86f, -0.30f, -1.25f, 0x1c1d20, 0x2a2b2e, 0xe9ebec, 1.f, 0.35f, false,
     Essuie::Paralleles, 0.600f, 0.500f, 1.62f, 0.794f},
    // T-Cross (SAIC-VW, Chine, 2021) : volant VW famille Polo ; Active Info Display 10,25 po ; écran 8 po dans la planche ; plastique
    // dur grainé ; balais 600 / 450 (AC, version européenne)
    {"tcross", 0.370f, 0.034f, 3, 0.f, false, false, false, 0x3d3d3f, {0, 0, 0}, 0x2b2c30, 0,
     {0.f, -0.31f, -0.74f}, -0.40f,
     Combine::VwDigital, 0.244f, 0.091f, {0.f, -0.24f, -0.86f}, 0xdfe6ee, 0x0e1116, 0xff2a2a,
     0.174f, 0.105f, {0.36f, -0.19f, -0.78f}, -0.10f,
     -0.19f, -0.80f, -0.26f, -1.12f, 0x1c1d20, 0x232428, 0x101113, 0.f, 0.15f, false,
     Essuie::Paralleles, 0.600f, 0.450f, 1.60f, 0.531f},
    // V60 Polestar (P3, 2013) : volant à palettes ; Adaptive Digital Display TFT 8 po (thème Performance : compte-tours rouge au
    // centre) ; écran 7 po sous une visière en haut de la console flottante (RG-04 « 9 po vertical » : V60 de 2018, journal) ;
    // alu ; balais 650 / 500 (AC)
    {"v60", 0.375f, 0.036f, 3, 0.f, true, false, false, 0x3d3d3f, {0, 0, 0}, 0x8f9398, 0,
     {0.f, -0.31f, -0.75f}, -0.38f,
     Combine::FerrariIncurve, 0.190f, 0.071f, {0.f, -0.23f, -0.87f}, 0xff3a2a, 0x0a0b0c, 0xff2b14,
     0.152f, 0.091f, {0.36f, -0.12f, -0.86f}, 0.f,
     -0.17f, -0.80f, -0.24f, -1.20f, 0x1a1a1c, 0x1c1c1e, 0xe9ebec, 1.f, 0.35f, false,
     Essuie::Paralleles, 0.650f, 0.500f, 1.62f, 0.477f},
};

const InterieurDef* interieurPour(const char* carId) {
    if (!carId) return nullptr;
    for (const InterieurDef& d : DEFS) if (std::strcmp(d.car, carId) == 0) return &d;
    return nullptr;
}
// WFS-315 (RG-01) : largeur aux coudes à l'avant et écart œil-axe par modèle (sources et statut : journal de WFS-315)
namespace {
struct LargeurDef { const char* car; Largeur l; };
// Statut (expert-realisme-3d, 2026-11-08) : aucune cote vérifiée (sites des constructeurs bloqués par le réseau des agents) ;
// coudes = largeur aux épaules (fiches US, de mémoire, AC) + 45 mm, ou largeur hors tout − 2 × épaisseur de portière (E) ;
// œil ≈ 0,25 × coudes (0,26 × pour les supercars, E) ; montants = k × largeur hors tout (k = 0,74 à 0,81 selon la voiture, E)
const LargeurDef LARGEURS[] = {
    {"golf", {1.470f, 0.370f, 1.450f}},
    {"m4", {1.460f, 0.365f, 1.470f}},
    {"p911", {1.400f, 0.350f, 1.410f}},
    {"sf90", {1.420f, 0.370f, 1.460f}},
    {"a45", {1.420f, 0.355f, 1.440f}},
    {"s3", {1.420f, 0.355f, 1.450f}},
    {"giulia", {1.450f, 0.365f, 1.470f}},
    {"p208", {1.400f, 0.350f, 1.430f}},
    {"gallardo", {1.400f, 0.365f, 1.410f}},
    {"mustang", {1.480f, 0.370f, 1.500f}},
    {"model3", {1.480f, 0.370f, 1.500f}},
    {"x5", {1.550f, 0.390f, 1.620f}},
    {"abarth", {1.310f, 0.325f, 1.310f}},
    {"compass", {1.455f, 0.365f, 1.470f}},
    {"corsa", {1.400f, 0.350f, 1.430f}},
    {"elantra", {1.465f, 0.365f, 1.480f}},
    {"gt86", {1.400f, 0.350f, 1.400f}},
    {"rrsport", {1.580f, 0.395f, 1.660f}},
    {"tcross", {1.420f, 0.355f, 1.430f}},
    {"v60", {1.470f, 0.370f, 1.510f}},
};
}
Largeur largeurPour(const InterieurDef& d) {
    for (const LargeurDef& l : LARGEURS) if (std::strcmp(l.car, d.car) == 0) return l.l;
    return {};
}

// WFS-414 : volant de série (vehicule_cockpit.h) : 6 voitures seulement (ordre de l'utilisateur, 2026-11-10) ; formes estimées
// sur photos des volants de série (journal de WFS-414) ; toutes les autres voitures : nullptr (volant commun, inchangé)
namespace {
struct SerieDef { const char* car; VolantSerie s; };
const SerieDef SERIE[] = {
    {"aucune", {0.200f, 0.145f, 4.f, 0.f, 0.068f}},   // étape 1 du ticket : aucune voiture (chemin commun pour toutes)
};
}
const VolantSerie* volantSeriePour(const InterieurDef& d) {
    for (const SerieDef& s : SERIE) if (std::strcmp(s.car, d.car) == 0) return &s.s;
    return nullptr;
}

int nombreInterieurs() { return (int)(sizeof(DEFS) / sizeof(DEFS[0])); }
const InterieurDef& interieur(int i) { return DEFS[std::clamp(i, 0, nombreInterieurs() - 1)]; }

} // namespace wfs::cockpit
