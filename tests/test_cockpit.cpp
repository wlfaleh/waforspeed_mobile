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
#include "../src/render/vehicule_cockpit_details.h"   // WFS-325 : pièces propres du lot 1
#include "../src/render/vehicule_cockpit_formes.h"   // ligneJante
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

// WFS-414 (RG-07), étendu par WFS-325 (RG-05) : points du haut de la jante (40° à 140°) cachés par la visière, vus de l'œil de la
// caméra, et plus petit jour entre la jante et la visière (m) ; sorti du test des volants de série pour servir à la M4, la 911, la SF90
static void mesureVisiere(const InterieurDef& d, int& caches, float& jour) {
    const float R = d.volantD * 0.5f - d.janteEp * 0.5f, rt = d.janteEp * 0.5f;
    const Visiere vis = ligneVisiere(d);
    auto distVisiere = [&](vec3 q) {   // distance (plan y, z) à la ligne de la visière, infinie hors de sa largeur
        if (std::fabs(q.x - vis.pts[0].x) > vis.largeur * 0.5f) return 1e9f;
        float best = 1e9f;
        for (int k = 0; k + 1 < 4; k++) {
            const vec2 a{vis.pts[k].y, vis.pts[k].z}, b{vis.pts[k + 1].y, vis.pts[k + 1].z}, ab{b.x - a.x, b.y - a.y}, aq{q.y - a.x, q.z - a.y};
            const float t = std::clamp((aq.x * ab.x + aq.y * ab.y) / (ab.x * ab.x + ab.y * ab.y), 0.f, 1.f);
            best = std::min(best, std::hypot(aq.x - ab.x * t, aq.y - ab.y * t));
        }
        return best;
    };
    const mat4 C = translate(d.volantPos) * rotateX(d.volantIncl);
    const vec3 oeil{0, OEIL_CAMERA_PLUS_M, 0};
    caches = 0; jour = 1e9f;
    for (int i = 0; i <= 60; i++) {
        const float th = (40.f + 100.f * i / 60) * PI / 180.f;
        const vec2 c = formes::ligneJante(th, R, d.meplat, d.meplatHaut);
        jour = std::min(jour, distVisiere(C.transformPoint({c.x, c.y, 0})) - rt - vis.ev * 0.5f);
        const vec3 nr = normalize(vec3{c.x, c.y, 0});
        for (int j = 0; j < 24; j++) {
            const float w = 2 * PI * j / 24;
            const vec3 nl = nr * std::cos(w) + vec3{0, 0, 1} * std::sin(w);
            const vec3 q = C.transformPoint(vec3{c.x, c.y, 0} + nl * rt), n = C.transformDir(nl);
            if (dot(n, oeil - q) <= 0) continue;   // face de la jante tournée de l'autre côté
            bool cache = false;
            for (int k = 1; k < 200 && !cache; k++) cache = distVisiere(oeil + (q - oeil) * (k / 200.f)) < vis.ev * 0.5f;
            caches += cache;
        }
    }
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
    {   // WFS-414 : volant de série des 6 voitures demandées (RG-01 moyeu, RG-02 branches attachées, RG-03 plaques, RG-07 visière),
        // mesuré sur la géométrie construite (pas sur les constantes)
        const char* six[] = {"mustang", "golf", "abarth", "tcross", "s3", "giulia"};
        int nSerie = 0; for (int i = 0; i < nombreInterieurs(); i++) nSerie += volantSeriePour(interieur(i)) ? 1 : 0;
        bool liste = nSerie == 6; for (const char* c : six) liste = liste && interieurPour(c) && volantSeriePour(*interieurPour(c));
        cas("volant de série : exactement Mustang, Golf, Abarth, T-Cross, S3 et Giulia (ordre de l'utilisateur)", liste);
        bool moyeu = true, depart = true, jante = true, largeurs = true, plaques = true, visiere = true, giulia = true, agrafe = true;
        for (const char* car : six) {
            const InterieurDef* dp = interieurPour(car); const VolantSerie* sp = dp ? volantSeriePour(*dp) : nullptr;
            if (!sp) continue;
            const InterieurDef& d = *dp; const VolantSerie& s = *sp;
            PiecesVolant p; construireVolant(d, p);
            const float R = d.volantD * 0.5f - d.janteEp * 0.5f, rt = d.janteEp * 0.5f;
            // RG-01 : coussin : 200-220 × 140-160 mm, 60-70 mm d'épaisseur, galbe ≥ 15 mm (milieu − face à 4 % du bord)
            const Empreinte m = empreinte(car, 1, p.moyeu);
            const float W = m.mx[0] - m.mn[0], H = m.mx[1] - m.mn[1], E = m.mx[2] - m.mn[2];
            float zBord = -1;
            for (const Vertex& v : p.moyeu.v) if (std::fabs(v.px) >= 0.48f * W && std::fabs(v.py) < 0.02f) zBord = std::max(zBord, v.pz);
            const float galbe = m.mx[2] - zBord;
            const bool okM = W >= 0.200f - 1e-4f && W <= 0.220f && H >= 0.140f && H <= 0.160f && E >= 0.060f && E <= 0.070f && galbe >= 0.015f
                             && !p.joint.v.empty() && !p.coque.v.empty();
            std::printf("  %s : coussin %.0f × %.0f × %.0f mm, galbe %.1f mm\n", car, W * 1000, H * 1000, E * 1000, galbe * 1000);
            moyeu = moyeu && okM;
            // RG-02 : chaque branche naît ≥ 10 mm dans le coussin, dans l'épaisseur de la coque, et finit dans le tube de la jante
            for (const float a : {0.f, PI, -PI / 2}) {
                const vec2 lj = formes::ligneJante(a, R, d.meplat, d.meplatHaut);
                const float Rl = std::sqrt(lj.x * lj.x + lj.y * lj.y), bord = a == -PI / 2 ? s.moyeuH * 0.5f : demiLargeurCoussin(s, 0);
                // sections de la branche (profilé : une section par station, largeur linéaire entre les deux stations du milieu)
                float xmin = 1, xmax = 0, zDep = 0; bool tube = true;
                std::vector<std::pair<float, float>> sections;   // (position de la station le long de l'axe incliné de la branche, largeur)
                const float pente = std::atan2(BRANCHE_Z1 - BRANCHE_Z0, Rl + 0.3f * rt - (bord - BRANCHE_DANS));
                for (const Vertex& v : p.branches.v) {
                    const float xl = v.px * std::cos(a) + v.py * std::sin(a), yl = -v.px * std::sin(a) + v.py * std::cos(a);
                    if (xl < 0.02f || std::fabs(yl) > 0.035f) continue;
                    if (xl < xmin) { xmin = xl; zDep = v.pz; }
                    xmax = std::max(xmax, xl);
                    if (xl > Rl - 0.5f * rt) tube = tube && std::hypot(xl - Rl, v.pz) <= rt + 5e-4f;
                    const float sv = xl * std::cos(pente) + v.pz * std::sin(pente);
                    bool vu = false;
                    for (auto& sct : sections) if (std::fabs(sct.first - sv) < 0.0008f) { sct.second = std::max(sct.second, 2 * std::fabs(yl)); vu = true; }
                    if (!vu) sections.push_back({sv, 2 * std::fabs(yl)});
                }
                const float r0 = bord - BRANCHE_DANS, s0 = r0 * std::cos(pente) + BRANCHE_Z0 * std::sin(pente);
                for (auto& sct : sections) sct.first = r0 + (sct.first - s0) * std::cos(pente);   // ramené à la distance au centre (x)
                std::sort(sections.begin(), sections.end());
                auto largeurEn = [&](float x) {   // entre la 2e et l'avant-dernière station
                    if (sections.size() < 4) return 0.f;
                    const auto a0 = sections[1], a1 = sections[sections.size() - 2];
                    return a0.second + (a1.second - a0.second) * std::clamp((x - a0.first) / (a1.first - a0.first), 0.f, 1.f);
                };
                const float w0 = largeurEn(bord + COQUE_BORD), w1 = largeurEn(Rl - rt);   // sortie de la coque, arrivée sur la jante
                depart = depart && xmin <= bord - 0.010f && zDep >= COQUE_Z_ARRIERE && zDep <= COQUE_Z_AVANT;
                jante = jante && xmax >= Rl && tube;
                const bool okW = std::fabs(w0 - BRANCHE_W0) <= 0.002f && std::fabs(w1 - BRANCHE_W1) <= 0.002f;
                largeurs = largeurs && okW;
                if (!okW) std::printf("  %s : branche %.2f rad : %.1f mm au bord du coussin, %.1f mm à la jante\n", car, a, w0 * 1000, w1 * 1000);
                if (!(xmin <= bord - 0.010f) || !tube || xmax < Rl)
                    std::printf("  %s : branche %.2f rad : départ %.1f mm (bord %.1f), bout %.1f mm (jante %.1f), dans le tube %s\n", car, a, xmin * 1000,
                                bord * 1000, xmax * 1000, Rl * 1000, tube ? "oui" : "non");
            }
            // RG-03 : une plaque de chaque côté, 34 à 42 mm le long de la branche, 30 à 41 mm en travers ; touches en relief dessus
            for (const float sx : {-1.f, 1.f}) {
                float x0 = 1, x1 = -1, y0 = 1, y1 = -1; int touches = 0;
                for (const Vertex& v : p.plaques.v) if (v.px * sx > 0) { x0 = std::min(x0, v.px); x1 = std::max(x1, v.px); y0 = std::min(y0, v.py); y1 = std::max(y1, v.py); }
                for (const Vertex& v : p.touches.v) touches += v.px * sx > 0 ? 1 : 0;
                const bool ok = x1 - x0 >= 0.033f && x1 - x0 <= 0.0425f && y1 - y0 >= 0.030f && y1 - y0 <= 0.041f && touches > 0;
                if (!ok) std::printf("  %s : plaque %s %.1f × %.1f mm\n", car, sx < 0 ? "gauche" : "droite", (x1 - x0) * 1000, (y1 - y0) * 1000);
                plaques = plaques && ok;
            }
            if (d.demarreur) { bool g = !p.manettino.v.empty(); for (const Vertex& v : p.manettino.v) g = g && v.px < 0 && v.py < 0; giulia = giulia && g; }
            if (!std::strcmp(car, "golf")) agrafe = !p.repere.v.empty();
            // RG-07 : la visière ne cache aucun point du haut de la jante vu de l'œil de la caméra, et un jour d'au moins 5 mm les sépare
            int caches = 0; float jour = 0;
            mesureVisiere(d, caches, jour);
            std::printf("  %s : visière : %d points du haut de la jante cachés, jour de %.1f mm\n", car, caches, jour * 1000);
            visiere = visiere && caches == 0 && jour >= 0.005f;
        }
        cas("RG-01 : coussin 200-220 × 140-160 mm, 60-70 mm d'épaisseur, galbe ≥ 15 mm, joint et coque", moyeu);
        cas("RG-02 : chaque branche naît ≥ 10 mm dans le coussin, dans la coque", depart);
        cas("RG-02 : chaque branche finit dans le tube de la jante (aucun jour)", jante);
        cas("RG-02 : branches de 55 mm à la sortie de la coque, 35 mm à la jante (± 2 mm)", largeurs);
        cas("RG-03 : une plaque de 34-42 × 30-41 mm avec ses touches sur chaque branche horizontale", plaques);
        cas("Giulia : démarreur rouge sur la plaque gauche, en bas ; Golf : agrafe rouge sur la branche basse", giulia && agrafe);
        cas("RG-07 : la visière ne cache pas le haut de la jante (jour ≥ 5 mm)", visiere);
    }
    {   // WFS-325 (RG-05, arbitrage du PO du 2026-11-11) : M4, 911 et SF90 : haut de la jante entier devant la visière, jour ≥ 5 mm ;
        // la Golf (volant de série de WFS-414) et les voitures sans avancée propre gardent la visière d'avant
        bool ok = true;
        for (const char* car : {"m4", "p911", "sf90"}) {
            const InterieurDef& d = *interieurPour(car);
            int caches = 0; float jour = 0;
            mesureVisiere(d, caches, jour);
            std::printf("  %s : visière (avancée %.0f mm) : %d points du haut de la jante cachés, jour de %.1f mm\n", car,
                        avanceVisierePropre(d) * 1000, caches, jour * 1000);
            ok = ok && caches == 0 && jour >= 0.005f;
        }
        cas("RG-05 : M4, 911 et SF90 : la visière ne cache pas le haut de la jante (jour ≥ 5 mm)", ok);
        int propres = 0; bool autres = true;
        for (int i = 0; i < nombreInterieurs(); i++) {
            const InterieurDef& d = interieur(i);
            const bool lot = !std::strcmp(d.car, "m4") || !std::strcmp(d.car, "p911") || !std::strcmp(d.car, "sf90");
            if (lot) propres++; else autres = autres && avanceVisierePropre(d) == 0;
        }
        cas("RG-05 / RG-08 : avancée propre pour ces 3 voitures seulement ; les autres visières inchangées", propres == 3 && autres);
        const Visiere g = ligneVisiere(*interieurPour("golf"));
        cas("RG-08 : visière de la Golf inchangée (avancée du volant de série)",
            std::fabs(g.pts[3].z - (interieurPour("golf")->combinePos.z + volantSeriePour(*interieurPour("golf"))->visiereAvance)) < 1e-6f);
    }
    {   // WFS-325 (RG-06, RG-08) : pièces propres du lot 1 (vehicule_cockpit_details.cpp), mesurées sur la géométrie construite
        const char* lot[] = {"golf", "m4", "p911", "sf90"};
        bool liste = true, vides = true;
        for (int i = 0; i < nombreInterieurs(); i++) {
            const InterieurDef& d = interieur(i);
            bool dans = false; for (const char* c : lot) dans = dans || !std::strcmp(c, d.car);
            liste = liste && detailsPropres(d) == dans;
            if (!dans) {   // RG-08 : rien de construit pour une voiture hors du lot (pièces communes de main)
                PiecesDetails p; construireDetails(d, p);
                for (const Geo& g : p.g) vides = vides && g.v.empty() && g.i.empty();
            }
        }
        cas("RG-08 : pièces propres pour Golf, M4, 911 et SF90 seulement ; aucune pour les 16 autres", liste && vides);
        bool pieces = true, indices = true, habitacle = true, rue = true, normales = true, ambiance = true;
        for (const char* car : lot) {
            const InterieurDef& d = *interieurPour(car);
            PiecesDetails p; construireDetails(d, p);
            const Largeur l = largeurPour(d);
            const float cx = l.oeil, demi = l.coudes * 0.5f, zAv = d.pareBriseZ - 0.06f;
            // pièces attendues : aérateurs (lames), lunettes (chrome ou alu), commandes laquées, console (bas de planche), garnitures
            const int nLames = (int)p[MatDetail::Lames].i.size() / 3, nLaque = (int)p[MatDetail::Laque].i.size() / 3;
            const int nCadres = (int)(p[MatDetail::Chrome].i.size() + p[MatDetail::Alu].i.size()) / 3, nCons = (int)p[MatDetail::PlancheBas].i.size() / 3;
            std::printf("  %s : %d triangles de lames, %d de lunettes et poignées, %d laqués, %d de console et garnitures\n", car, nLames, nCadres, nLaque, nCons);
            pieces = pieces && nLames > 2000 && nCadres > 500 && nLaque > 200 && nCons > 200;
            ambiance = ambiance && (couleurAmbiance(d) != 0) == !p[MatDetail::Ambiance].v.empty();
            long hors = 0, devantRue = 0;
            for (const Geo& g : p.g) {
                for (uint32_t k : g.i) indices = indices && k < g.v.size();
                indices = indices && g.i.size() % 3 == 0;
                for (const Vertex& v : g.v) {
                    normales = normales && std::fabs(std::sqrt(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz) - 1) < 1e-3f;
                    // dans l'habitacle : pas au-delà des vitres latérales (une garniture peut entrer dans le bloc de la portière, WFS-349),
                    // du plancher au ciel de toit, du pied du pare-brise à 0,6 m derrière l'œil
                    if (v.px < cx - faceVitre(demi) || v.px > cx + faceVitre(demi) || v.py < PLANCHER_Y - 0.01f || v.py > hauteurCielDeToit(v.pz) + 0.02f
                        || v.pz < zAv - 0.01f || v.pz > 0.61f) hors++;
                    // WFS-235 (RG-05) : rien dans la bande de la rue (de l'horizon à +8° au-dessus, de face : ±60° autour de l'axe)
                    if (v.pz < 0) {
                        const float el = std::atan2(v.py, std::hypot(v.px, v.pz)), az = std::atan2(v.px, -v.pz);
                        if (el >= -0.01f && el <= 9.f * PI / 180.f && std::fabs(az) <= PI / 3) devantRue++;
                    }
                }
            }
            if (hors || devantRue) std::printf("  %s : %ld sommets hors de l'habitacle, %ld dans la bande de la rue\n", car, hors, devantRue);
            habitacle = habitacle && hors == 0; rue = rue && devantRue == 0;
        }
        cas("RG-06 : aérateurs, lunettes, commandes laquées, console et garnitures construits pour les 4 voitures", pieces);
        cas("pièces propres : indices valides, triangles complets, normales unitaires", indices && normales);
        cas("pièces propres dans l'habitacle (portières, plancher, ciel de toit, pied du pare-brise)", habitacle);
        cas("WFS-235 (RG-05) : aucune pièce propre devant la rue (horizon à +9°, ±60°)", rue);
        cas("RG-07 : lot 1 : laqué et carbone vernis 0,02-0,08, chrome 0,05-0,15, alu brossé 0,3-0,5 (rugosités)",
            RUGO_LAQUE_LOT1 >= 0.02f && RUGO_LAQUE_LOT1 <= 0.08f && RUGO_CARBONE_LOT1 >= 0.02f && RUGO_CARBONE_LOT1 <= 0.08f
            && RUGO_CHROME_LOT1 >= 0.05f && RUGO_CHROME_LOT1 <= 0.15f && RUGO_ALU >= 0.3f && RUGO_ALU <= 0.5f);
        cas("lumière d'ambiance : Golf et M4 seulement (couleur du modèle)", ambiance && couleurAmbiance(*interieurPour("golf")) && couleurAmbiance(*interieurPour("m4"))
            && !couleurAmbiance(*interieurPour("p911")) && !couleurAmbiance(*interieurPour("sf90")));
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("cockpit : tout est OK\n");
    return 0;
}
