// Intérieurs par modèle de la vue cockpit (WFS-197) : choix de l'intérieur (RG-01), volants (RG-03), combiné (RG-04),
// carte (RG-05), essuie-glaces (RG-06), œil (RG-07), matières dans la charte du réel (DA-realisme.md).
// usage : test_cockpit
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>
#include "../src/game/cars.h"
#include "../src/render/vehicule_cockpit.h"
#include "../src/render/vehicule_cockpit_volant.h"
#include "test_cockpit_volants_main.h"   // WFS-414 (RG-05) : empreintes des volants de main (test_cockpit --empreintes)
using namespace wfs;
using namespace wfs::cockpit;

static int echecs = 0;
static void cas(const char* quoi, bool ok) { std::printf("cockpit : %-78s → %s\n", quoi, ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }

static float lumLin(unsigned h) {
    auto l = [](float c) { return std::pow(c, 2.2f); };
    return 0.2126f * l(((h >> 16) & 255) / 255.f) + 0.7152f * l(((h >> 8) & 255) / 255.f) + 0.0722f * l((h & 255) / 255.f);
}

// WFS-414 (RG-05) : empreinte d'une pièce du volant : nombres de sommets et d'indices, somme pondérée des indices (topologie, exacte),
// boîte englobante et centre des sommets (m, comparés à 0,05 mm près : même géométrie, aux arrondis de la bibliothèque près)
static Empreinte empreinte(const char* car, int piece, const Geo& g) {
    Empreinte e{car, piece, (int)g.v.size(), (int)g.i.size(), 0u, {1e9f, 1e9f, 1e9f}, {-1e9f, -1e9f, -1e9f}, {0, 0, 0}};
    for (size_t k = 0; k < g.i.size(); k++) e.topo += g.i[k] * (uint32_t)(k % 97 + 1);
    for (const Vertex& v : g.v) {
        const float p[3] = {v.px, v.py, v.pz};
        for (int a = 0; a < 3; a++) { e.mn[a] = std::min(e.mn[a], p[a]); e.mx[a] = std::max(e.mx[a], p[a]); e.c[a] += p[a] / (float)g.v.size(); }
    }
    return e;
}
static std::vector<const Geo*> piecesDe(const PiecesVolant& p) {
    return {&p.jante, &p.moyeu, &p.branches, &p.coutures[0], &p.coutures[1], &p.coutures[2], &p.repere, &p.palettes, &p.manettino,
            &p.molettes, &p.coque, &p.joint, &p.plaques, &p.touches};
}

int main(int argc, char** argv) {
    if (argc > 1 && !std::strcmp(argv[1], "--empreintes")) {   // WFS-414 : écrit tests/test_cockpit_volants_main.h (à lancer sur le code de main)
        std::printf("// WFS-414 (RG-05) : empreintes des volants des voitures sans volant de série, relevées sur le code de main\n");
        std::printf("// (test_cockpit --empreintes, avant l'ajout du volant de série) : ces volants ne doivent pas changer\n#pragma once\n#include <cstdint>\n");
        std::printf("struct Empreinte { const char* car; int piece, nv, ni; uint32_t topo; float mn[3], mx[3], c[3]; };\n");
        std::printf("static const Empreinte VOLANTS_MAIN[] = {\n");
        for (int i = 0; i < nombreInterieurs(); i++) {
            PiecesVolant p; construireVolant(interieur(i), p);
            const auto ps = piecesDe(p);
            for (int k = 0; k < (int)ps.size(); k++) {
                const Empreinte e = empreinte(interieur(i).car, k, *ps[k]);
                if (!e.nv) continue;
                std::printf("    {\"%s\", %d, %d, %d, %uu, {%.6ff, %.6ff, %.6ff}, {%.6ff, %.6ff, %.6ff}, {%.6ff, %.6ff, %.6ff}},\n", e.car, k, e.nv, e.ni, e.topo,
                            e.mn[0], e.mn[1], e.mn[2], e.mx[0], e.mx[1], e.mx[2], e.c[0], e.c[1], e.c[2]);
            }
        }
        std::printf("};\n");
        return 0;
    }
    // RG-01 : 4 intérieurs (lot 1), toutes les autres voitures gardent le cockpit commun
    std::set<std::string> avec;
    for (int i = 0; i < CAR_COUNT; i++) if (interieurPour(CARS[i].id)) avec.insert(CARS[i].id);
    cas("lots 1 à 3 : les 20 voitures du catalogue ont leur intérieur (WFS-197, 201, 202)",
        (int)avec.size() == CAR_COUNT && CAR_COUNT == 20 && nombreInterieurs() == 20);
    cas("id inconnu ou nul : cockpit commun", !interieurPour("inconnue") && !interieurPour(nullptr));
    for (int i = 0; i < nombreInterieurs(); i++) {
        const InterieurDef& d = interieur(i);
        bool existe = false; for (int k = 0; k < CAR_COUNT; k++) existe = existe || !std::strcmp(CARS[k].id, d.car);
        cas((std::string(d.car) + " : voiture du catalogue").c_str(), existe);
    }
    // RG-03 : volants (cotes de l'expert : Golf 370, M4 370, 911 375, SF90 350-360 mm ; jante 33-40 mm)
    const InterieurDef &golf = *interieurPour("golf"), &m4 = *interieurPour("m4"), &p911 = *interieurPour("p911"), &sf90 = *interieurPour("sf90");
    cas("Ø des volants : Golf 370, M4 370, 911 375, SF90 355 mm (± 1 %)",
        std::abs(golf.volantD - 0.370f) <= 0.0037f && std::abs(m4.volantD - 0.370f) <= 0.0037f && std::abs(p911.volantD - 0.375f) <= 0.00375f
        && sf90.volantD >= 0.350f && sf90.volantD <= 0.360f);
    bool janteOk = true; for (int i = 0; i < nombreInterieurs(); i++) janteOk = janteOk && interieur(i).janteEp >= 0.033f && interieur(i).janteEp <= 0.040f;
    cas("épaisseur des jantes de 33 à 40 mm", janteOk);
    cas("méplat : Golf et SF90 oui, M4 (avant restylage) et 911 non (volant rond)", golf.meplat > 0 && sf90.meplat > 0 && m4.meplat == 0 && p911.meplat == 0);
    cas("branches : 3 (Golf, M4, 911), 2 larges (SF90)", golf.branches == 3 && m4.branches == 3 && p911.branches == 3 && sf90.branches == 2);
    // WFS-404 (ANO-209) : bandes de la marque retirées ; les boutons M1 / M2 rouges des branches restent dessinés (vehicule_cockpit.cpp :
    // boutons si coutures ou boutonsBranches, sinon agrafe de la Golf sur la branche basse : jamais sur la M4)
    cas("M4 : volant sans les bandes de la marque, boutons rouges sur les branches (pas d'agrafe)",
        !m4.couture[0] && !m4.couture[1] && !m4.couture[2] && m4.repereCol && (m4.boutonsBranches || m4.couture[0] || m4.couture[1] || m4.couture[2]));
    cas("SF90 : manettino et démarreur ; palettes sur les 4", sf90.manettino && sf90.demarreur && golf.palettes && m4.palettes && p911.palettes && sf90.palettes);
    bool incl = true; for (int i = 0; i < nombreInterieurs(); i++) incl = incl && interieur(i).volantIncl <= -0.29f && interieur(i).volantIncl >= -0.46f;
    cas("inclinaison de la colonne de 17 à 26° (SAE J1100, angle A18 ; 208 et X5 : colonne plus droite)", incl);
    // charte du réel : cuir noir albédo linéaire 0,04-0,12 (le rendu ramène sous 0,04 à 0,045)
    bool cuir = true; for (int i = 0; i < nombreInterieurs(); i++) { const float y = lumLin(interieur(i).cuir); cuir = cuir && y >= 0.04f && y <= 0.12f; }
    cas("cuir des volants dans la charte (albédo linéaire 0,04-0,12)", cuir);
    // RG-04 : combinés (types et tailles de l'expert : 244 × 91, 292 × 110, 5 cadrans 300 mm, écran courbe 380 mm)
    cas("combinés : VW Digital, BMW Live, Porsche 5 cadrans, Ferrari incurvé",
        golf.combine == Combine::VwDigital && m4.combine == Combine::BmwLive && p911.combine == Combine::Porsche5 && sf90.combine == Combine::FerrariIncurve);
    cas("tailles des combinés (± 1 %) : 244 × 91, 292 × 110, 380 × 143 mm",
        std::abs(golf.combineW - 0.244f) < 0.0025f && std::abs(golf.combineH - 0.091f) < 0.001f && std::abs(m4.combineW - 0.292f) < 0.003f
        && std::abs(sf90.combineW - 0.380f) < 0.004f);
    // chiffres à 7 segments : 0 à 9 distincts, 8 allume tout, hors plage : rien
    std::set<int> vus; for (int k = 0; k <= 9; k++) vus.insert(segments7(k));
    cas("chiffres à 7 segments : 10 formes distinctes, 8 = 7 segments, hors plage = vide", vus.size() == 10 && segments7(8) == 0x7F && segments7(10) == 0 && segments7(-1) == 0);
    cas("vitesse affichée : arrondie, positive, bornée à 999", vitesseAffichee(123.6) == 124 && vitesseAffichee(-30.2) == 30 && vitesseAffichee(1500) == 999);
    // RG-05 : écran central (Golf 232 × 104, M4 244 × 91, 911 245 × 130 mm), SF90 sans écran central (carte dans le combiné)
    cas("écrans centraux : Golf 232 × 104, M4 244 × 91, 911 245 × 130 mm ; SF90 : aucun",
        std::abs(golf.ecranW - 0.232f) < 0.0025f && std::abs(m4.ecranW - 0.244f) < 0.0025f && std::abs(p911.ecranH - 0.130f) < 0.0015f && sf90.ecranW == 0);
    cas("écrans tournés vers le conducteur (lacet ≤ 0) ou droits", golf.ecranLacet < 0 && m4.ecranLacet < 0 && p911.ecranLacet == 0);
    // carte : repère de la voiture, cap 0 = vers −z (avant = (sin cap, −cos cap)), droite = (cos cap, sin cap)
    {
        const vec2 j{10, 20};
        const vec2 devant = versVoiture({10, 0}, j, 0), droite = versVoiture({15, 20}, j, 0);
        const vec2 d90 = versVoiture({30, 20}, j, PI / 2);   // cap π/2 : l'avant est vers +x
        cas("carte : point 20 m devant → (0, 20) ; 5 m à droite → (5, 0) ; cap π/2 : +x devant",
            std::abs(devant.x) < 1e-4f && std::abs(devant.y - 20) < 1e-4f && std::abs(droite.x - 5) < 1e-4f && std::abs(droite.y) < 1e-4f
            && std::abs(d90.y - 20) < 1e-3f && std::abs(d90.x) < 1e-3f);
        cas("carte : 150 m derrière et 350 m devant, flèche à 30 % de la hauteur (route qui arrive au-dessus)",
            CARTE_ARRIERE_M == 150 && CARTE_AVANT_M == 350 && std::abs(CARTE_JOUEUR_Y - CARTE_ARRIERE_M / (CARTE_ARRIERE_M + CARTE_AVANT_M)) < 1e-6);
    }
    // RG-06 : essuie-glaces : 45 balayages/min, 65 au-delà de 100 km/h, à l'arrêt sans pluie ; le cycle en cours finit
    cas("cadence : 0 sans pluie, 45 à 80 km/h, 65 à 130 km/h", cadenceEssuie(80, false) == 0 && cadenceEssuie(80, true) == 45 && cadenceEssuie(130, true) == 65);
    {
        EtatEssuie e; const float dt = 1.f / 120; int cycles = 0; float prec = 0;
        for (int i = 0; i < 120 * 60; i++) { e.avancer(dt, 80, true); if (e.phase < prec) cycles++; prec = e.phase; }
        EtatEssuie v; int cv = 0; prec = 0;
        for (int i = 0; i < 120 * 60; i++) { v.avancer(dt, 130, true); if (v.phase < prec) cv++; prec = v.phase; }
        std::printf("cockpit : essuie-glaces : %d balayages en 60 s à 80 km/h, %d à 130 km/h\n", cycles, cv);
        cas("60 s de pluie : 45 balayages à 80 km/h, 65 à 130 km/h (± 1)", std::abs(cycles - 45) <= 1 && std::abs(cv - 65) <= 1);
        EtatEssuie s; for (int i = 0; i < 120 * 10; i++) s.avancer(dt, 80, false);
        cas("sans pluie : balais au repos (angle 0)", s.angle(1.66f) == 0 && !s.enMarche);
        EtatEssuie f; for (int i = 0; i < 40; i++) f.avancer(dt, 80, true);   // pluie qui s'arrête en cours de balayage
        const float milieu = f.angle(1.66f); int pas = 0;
        while (f.enMarche && pas < 120 * 3) { f.avancer(dt, 80, false); pas++; }
        cas("la pluie cesse en cours de balayage : le balai finit sa course puis reste au repos", milieu > 0 && !f.enMarche && f.angle(1.66f) == 0 && pas < 120 * 2);
        EtatEssuie a; float amax = 0, saut = 0, pa = 0;
        for (int i = 0; i < 120 * 4; i++) { a.avancer(dt, 80, true); const float x = a.angle(1.66f); amax = std::max(amax, x); saut = std::max(saut, std::abs(x - pa)); pa = x; }
        cas("angle de 0 à l'arc (1,66 rad), continu (≤ arc × π × cadence / 60 / 120 par pas)", amax > 1.64f && amax <= 1.66f && saut <= 1.66f * PI * 45 / 60 / 120 * 1.02f);
    }
    bool arcs = true, balais = true;
    for (int i = 0; i < nombreInterieurs(); i++) { arcs = arcs && interieur(i).arc >= 1.4f && interieur(i).arc <= 1.75f; balais = balais && interieur(i).balaiG >= interieur(i).balaiD && interieur(i).balaiG > 0.5f; }
    cas("arcs de 80 à 100° ; balai conducteur ≥ passager (Golf 650 / 450, M4 600 / 475)", arcs && balais && golf.balaiG == 0.650f && m4.balaiD == 0.475f);
    // RG-07 : œil plus bas dans la 911 et la SF90 que dans la Golf (assise du modèle)
    cas("hauteur de l'œil : Golf > M4 > 911 > SF90", hauteurOeil(golf) > hauteurOeil(m4) && hauteurOeil(m4) > hauteurOeil(p911) && hauteurOeil(p911) > hauteurOeil(sf90));
    // ---------- WFS-201 : lot 2 (cotes de l'expert, génération du .wfm)
    {
        const InterieurDef &a45 = *interieurPour("a45"), &s3 = *interieurPour("s3"), &giulia = *interieurPour("giulia"), &p208 = *interieurPour("p208"),
                           &gallardo = *interieurPour("gallardo"), &mustang = *interieurPour("mustang"), &model3 = *interieurPour("model3"), &x5 = *interieurPour("x5");
        cas("Ø des volants du lot 2 : A 45 370, S3 359, Giulia 362, 208 350, Gallardo 360, Mustang 370, Model 3 350, X5 370 mm",
            a45.volantD == 0.370f && s3.volantD == 0.359f && giulia.volantD == 0.362f && p208.volantD == 0.350f && gallardo.volantD == 0.360f
            && mustang.volantD == 0.370f && model3.volantD == 0.350f && x5.volantD == 0.370f);
        cas("208 : volant plat en haut et en bas ; méplat A 45, S3, Giulia ; Mustang, Gallardo, Model 3, X5 ronds",
            p208.meplat > 0 && p208.meplatHaut > 0 && a45.meplat > 0 && s3.meplat > 0 && giulia.meplat > 0 && mustang.meplat == 0 && gallardo.meplat == 0
            && model3.meplat == 0 && x5.meplat == 0 && golf.meplatHaut == 0);
        cas("Model 3 : 2 branches, 2 molettes, sans palettes, sans combiné ; écran central 15 po (323 × 202 mm)",
            model3.branches == 2 && model3.molettes && !model3.palettes && model3.combine == Combine::Aucun && model3.combineW == 0
            && std::abs(model3.ecranW - 0.323f) < 0.0033f && std::abs(model3.ecranH - 0.202f) < 0.0021f);
        cas("Giulia : démarreur rouge sur le volant, palettes fixées sur la colonne (Gallardo aussi) ; A 45 : repère à midi",
            giulia.demarreur && !giulia.manettino && giulia.palettesColonne && gallardo.palettesColonne && !mustang.palettesColonne && a45.repereHaut);
        cas("combinés : 2 cadrans (A 45, S3 : génération du .wfm), Alfa, i-Cockpit, Lamborghini, Ford, BMW (X5)",
            a45.combine == Combine::AudiVirtual && s3.combine == Combine::AudiVirtual && giulia.combine == Combine::AlfaCadrans
            && p208.combine == Combine::PeugeotICockpit && gallardo.combine == Combine::LamboCompteTours && mustang.combine == Combine::FordDigital
            && x5.combine == Combine::BmwLive);
        // 208 : combiné lu AU-DESSUS du volant : la ligne œil → bas du combiné passe au-dessus du haut de la jante
        const float R = p208.volantD / 2, hautY = p208.volantPos.y + R * std::cos(p208.volantIncl), hautZ = p208.volantPos.z + R * std::sin(p208.volantIncl);
        const float basY = p208.combinePos.y - p208.combineH / 2, ligne = basY * (hautZ / p208.combinePos.z);
        std::printf("cockpit : 208 : haut de la jante y %.3f (z %.3f), ligne de vue vers le bas du combiné à cette profondeur y %.3f\n", hautY, hautZ, ligne);
        cas("208 : combiné vu au-dessus de la jante (ligne de vue ≥ 1 cm plus haut que la jante)", ligne > hautY + 0.01f);
        // X5 : combiné et écran côte à côte sur une même vitre, sans se chevaucher
        cas("X5 : écran central à droite du combiné, sans chevauchement", x5.ecranPos.x - x5.ecranW / 2 >= x5.combinePos.x + x5.combineW / 2 - 0.001f);
        // RG-06 / RG-07 : hauteurs d'œil (assise du modèle + yeux assis d'ANSUR-II) : X5 > 208 > Golf … > Gallardo
        std::printf("cockpit : œil au-dessus du sol : X5 %.3f, 208 %.3f, Golf %.3f, Giulia %.3f, Mustang %.3f, Model 3 %.3f, A 45 %.3f, S3 %.3f, M4 %.3f, 911 %.3f, SF90 %.3f, Gallardo %.3f m\n",
                    hauteurOeil(x5), hauteurOeil(p208), hauteurOeil(golf), hauteurOeil(giulia), hauteurOeil(mustang), hauteurOeil(model3), hauteurOeil(a45),
                    hauteurOeil(s3), hauteurOeil(m4), hauteurOeil(p911), hauteurOeil(sf90), hauteurOeil(gallardo));
        bool plusBas = true; for (int i = 0; i < nombreInterieurs(); i++) plusBas = plusBas && hauteurOeil(gallardo) <= hauteurOeil(interieur(i)) + 1e-6f;
        cas("œil : X5 > 208 > Golf ; Gallardo le plus bas", hauteurOeil(x5) > hauteurOeil(p208) && hauteurOeil(p208) > hauteurOeil(golf) && plusBas);
        cas("yeux assis d'ANSUR-II : 0,7859 m (hommes 804,5 et femmes 747,7 mm pondérés par l'effectif 4082 / 1986)",
            std::abs(OEIL_ASSIS_M - (804.5f * 4082 + 747.7f * 1986) / 6068 / 1000) < 0.0002f);
    }
    // ---------- WFS-202 : lot 3 (cotes de l'expert, génération du .wfm)
    {
        const InterieurDef &abarth = *interieurPour("abarth"), &compass = *interieurPour("compass"), &corsa = *interieurPour("corsa"),
                           &elantra = *interieurPour("elantra"), &gt86 = *interieurPour("gt86"), &rrsport = *interieurPour("rrsport"),
                           &tcross = *interieurPour("tcross"), &v60 = *interieurPour("v60");
        cas("Ø des volants du lot 3 : Abarth 365, Compass 380, Corsa 365, Elantra 370, GT86 363,5, RR Sport 380, T-Cross 370, V60 375 mm",
            abarth.volantD == 0.365f && compass.volantD == 0.380f && corsa.volantD == 0.365f && elantra.volantD == 0.370f
            && gt86.volantD == 0.3635f && rrsport.volantD == 0.380f && tcross.volantD == 0.370f && v60.volantD == 0.375f);
        cas("Abarth (2008-2015) : un seul cadran rond (l = h), pas d'écran central (carte dans le cadran)",
            abarth.combine == Combine::FerrariIncurve && abarth.combineW == abarth.combineH && abarth.ecranW == 0 && abarth.meplat > 0);
        cas("Range Rover Sport L461 : volant 3 branches (pas 4), palettes ; 3 branches pour tout le lot",
            rrsport.branches == 3 && rrsport.palettes && compass.branches == 3 && corsa.branches == 3 && elantra.branches == 3
            && gt86.branches == 3 && tcross.branches == 3 && v60.branches == 3 && abarth.branches == 3);
        cas("Elantra N : boutons N bleus sur les branches, palettes ; combiné et écran sur une même vitre sans chevauchement",
            elantra.boutonsBranches && elantra.repereCol == 0x0072ce && elantra.palettes
            && elantra.ecranPos.x - elantra.ecranW / 2 >= elantra.combinePos.x + elantra.combineW / 2 - 0.001f && elantra.ecranLacet < 0);
        cas("V60 P3 : écran 7 po (152 × 91 mm), pas le 9 po vertical ; combiné TFT 8 po",
            std::abs(v60.ecranW - 0.152f) < 0.0016f && v60.ecranW > v60.ecranH && std::abs(v60.combineW - 0.190f) < 0.002f);
        cas("combinés : VW (Compass, Corsa, T-Cross), Lamborghini (GT86), Audi (RR Sport), Ferrari (Elantra, V60)",
            compass.combine == Combine::VwDigital && corsa.combine == Combine::VwDigital && tcross.combine == Combine::VwDigital
            && gt86.combine == Combine::LamboCompteTours && rrsport.combine == Combine::AudiVirtual
            && elantra.combine == Combine::FerrariIncurve && v60.combine == Combine::FerrariIncurve);
        std::printf("cockpit : œil du lot 3 : RR Sport %.3f, Compass %.3f, T-Cross %.3f, V60 %.3f, Abarth %.3f, Corsa %.3f, Elantra %.3f, GT86 %.3f m\n",
                    hauteurOeil(rrsport), hauteurOeil(compass), hauteurOeil(tcross), hauteurOeil(v60), hauteurOeil(abarth), hauteurOeil(corsa),
                    hauteurOeil(elantra), hauteurOeil(gt86));
        bool sousGt = true; for (int i = 0; i < nombreInterieurs(); i++) if (std::strcmp(interieur(i).car, "gallardo") && std::strcmp(interieur(i).car, "sf90") && std::strcmp(interieur(i).car, "p911"))
            sousGt = sousGt && hauteurOeil(gt86) <= hauteurOeil(interieur(i)) + 1e-6f;
        bool plusHaut = true; for (int i = 0; i < nombreInterieurs(); i++) plusHaut = plusHaut && hauteurOeil(rrsport) >= hauteurOeil(interieur(i)) - 1e-6f;
        cas("œil : RR Sport le plus haut, Compass au-dessus des berlines, GT86 la plus basse hors supercars", plusHaut && hauteurOeil(compass) > hauteurOeil(golf) && sousGt);
    }
    {   // WFS-315 (RG-01) : largeur de l'habitacle par modèle : une cote pour chacun ; deux adultes et la console tiennent
        // (≥ 2 × 0,447 m d'épaules du plus large sur 20, ANSUR-II, + 0,20 m) ; œil ≈ 0,25 × coudes (±0,02) ; pieds des montants
        // à moins de 0,10 m de la largeur aux coudes ; montant droit plus loin de l'œil que le gauche ; X5 plus large que la 500
        bool toutes = true, places = true, oeil = true, montants = true, droit = true;
        for (int i = 0; i < nombreInterieurs(); i++) {
            const InterieurDef& d = interieur(i);
            const Largeur l = largeurPour(d), defaut{};
            toutes = toutes && (l.coudes != defaut.coudes || l.oeil != defaut.oeil);
            places = places && l.coudes >= 2 * 0.447f + 0.20f && l.coudes <= 1.70f;
            oeil = oeil && std::abs(l.oeil - 0.25f * l.coudes) <= 0.02f;
            montants = montants && std::abs(l.montants - l.coudes) <= 0.10f;
            droit = droit && l.oeil + l.montants / 2 > 2.5f * (l.montants / 2 - l.oeil);
        }
        cas("largeur : une cote par modèle (WFS-315)", toutes);
        cas("largeur : deux adultes et la console tiennent, rien au-delà de 1,70 m", places);
        cas("œil à ≈ 0,25 × la largeur aux coudes de l'axe", oeil);
        cas("pieds des montants à moins de 0,10 m de la largeur aux coudes", montants);
        cas("montant droit ≥ 2,5 fois plus loin de l'œil que le gauche (place passager)", droit);
        const Largeur x5 = largeurPour(*interieurPour("x5")), ab = largeurPour(*interieurPour("abarth"));
        cas("X5 plus large que la 500 d'au moins 0,20 m", x5.coudes - ab.coudes >= 0.20f);
    }
    {   // WFS-323 (ANO-188) : la vitesse du combiné tient dans la partie vue par l'ouverture haute du volant (tous les styles derrière le
        // volant), chiffres d'au moins 20 % de la hauteur du combiné ; la bande de la X5 commence sous le haut du combiné (jante devant)
        bool ok = true, bmw = true;
        for (int i = 0; i < nombreInterieurs(); i++) {
            const InterieurDef& d = interieur(i);
            if (d.combine == Combine::Aucun || d.combine == Combine::PeugeotICockpit) continue;   // écran central ; lu au-dessus du volant
            const BandeCombine b = bandeLisibleCombine(d);
            float y0 = d.combine == Combine::BmwLive ? -d.combineH * 0.14f : d.combine == Combine::VwDigital ? -d.combineH * 0.08f : -d.combineH * 0.40f;
            float h = d.combineH * (d.combine == Combine::BmwLive ? 0.42f : d.combine == Combine::VwDigital ? 0.40f : 0.22f);
            placerDansBande(b, d.combineH, y0, h);
            const bool dedans = y0 >= b.bas - 1e-4f && y0 + h <= b.haut + 1e-4f && h >= d.combineH * 0.20f - 1e-4f;
            if (!dedans) std::printf("  %s : bande %.3f..%.3f m, chiffres %.3f..%.3f m\n", d.car, b.bas, b.haut, y0, y0 + h);
            ok = ok && dedans;
            if (!std::strcmp(d.car, "x5")) bmw = b.haut < d.combineH * 0.5f - 0.01f;
        }
        cas("combiné : vitesse dans la partie vue par le volant, chiffres ≥ 20 % (WFS-323, ANO-188)", ok);
        cas("combiné de la X5 : le haut de la jante cache le haut du combiné (bande calculée)", bmw);
    }
    {   // WFS-414 (RG-05) : le volant de toutes les voitures sans volant de série est celui de main (empreinte de chaque pièce : nombres,
        // topologie exacte, boîte et centre à 0,05 mm près) ; aucune pièce en plus ni en moins
        int autres = 0; bool identiques = true;
        auto proche = [](const float* a, const float* b) { for (int k = 0; k < 3; k++) if (std::fabs(a[k] - b[k]) > 5e-5f) return false; return true; };
        for (int i = 0; i < nombreInterieurs(); i++) {
            const InterieurDef& d = interieur(i);
            if (volantSeriePour(d)) continue;
            autres++;
            PiecesVolant p; construireVolant(d, p);
            const auto ps = piecesDe(p);
            int attendues = 0, trouvees = 0;
            for (const Empreinte& r : VOLANTS_MAIN) if (!std::strcmp(r.car, d.car)) attendues++;
            for (int k = 0; k < (int)ps.size(); k++) {
                const Empreinte e = empreinte(d.car, k, *ps[k]);
                if (!e.nv) continue;
                trouvees++;
                const Empreinte* r = nullptr;
                for (const Empreinte& x : VOLANTS_MAIN) if (!std::strcmp(x.car, d.car) && x.piece == k) r = &x;
                const bool ok = r && r->nv == e.nv && r->ni == e.ni && r->topo == e.topo && proche(r->mn, e.mn) && proche(r->mx, e.mx) && proche(r->c, e.c);
                if (!ok) std::printf("  %s : pièce %d du volant différente de main\n", d.car, k);
                identiques = identiques && ok;
            }
            if (trouvees != attendues) { std::printf("  %s : %d pièces au lieu de %d\n", d.car, trouvees, attendues); identiques = false; }
        }
        std::printf("cockpit : %d voitures gardent le volant de main\n", autres);
        cas("volants des voitures sans volant de série identiques à main (WFS-414, RG-05)", identiques && autres >= 1);
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("cockpit : tout est OK\n");
    return 0;
}
