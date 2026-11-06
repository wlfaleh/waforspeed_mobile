// Intérieurs par modèle de la vue cockpit (WFS-197) : choix de l'intérieur (RG-01), volants (RG-03), combiné (RG-04),
// carte (RG-05), essuie-glaces (RG-06), œil (RG-07), matières dans la charte du réel (DA-realisme.md).
// usage : test_cockpit
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include "../src/game/cars.h"
#include "../src/render/vehicule_cockpit.h"
using namespace wfs;
using namespace wfs::cockpit;

static int echecs = 0;
static void cas(const char* quoi, bool ok) { std::printf("cockpit : %-78s → %s\n", quoi, ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }

static float lumLin(unsigned h) {
    auto l = [](float c) { return std::pow(c, 2.2f); };
    return 0.2126f * l(((h >> 16) & 255) / 255.f) + 0.7152f * l(((h >> 8) & 255) / 255.f) + 0.0722f * l((h & 255) / 255.f);
}

int main() {
    // RG-01 : 4 intérieurs (lot 1), toutes les autres voitures gardent le cockpit commun
    std::set<std::string> avec;
    for (int i = 0; i < CAR_COUNT; i++) if (interieurPour(CARS[i].id)) avec.insert(CARS[i].id);
    cas("lot 1 : Golf, M4, 911, SF90 ont leur intérieur", avec == std::set<std::string>{"golf", "m4", "p911", "sf90"} && nombreInterieurs() == 4);
    cas("les 16 autres voitures (dont la Corsa) gardent le cockpit commun ; id inconnu ou nul : commun",
        CAR_COUNT - (int)avec.size() == 16 && !interieurPour("corsa") && !interieurPour("inconnue") && !interieurPour(nullptr));
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
    bool janteOk = true; for (int i = 0; i < 4; i++) janteOk = janteOk && interieur(i).janteEp >= 0.033f && interieur(i).janteEp <= 0.040f;
    cas("épaisseur des jantes de 33 à 40 mm", janteOk);
    cas("méplat : Golf et SF90 oui, M4 (avant restylage) et 911 non (volant rond)", golf.meplat > 0 && sf90.meplat > 0 && m4.meplat == 0 && p911.meplat == 0);
    cas("branches : 3 (Golf, M4, 911), 2 larges (SF90)", golf.branches == 3 && m4.branches == 3 && p911.branches == 3 && sf90.branches == 2);
    cas("M4 : coutures M (bleu clair, bleu foncé, rouge) et boutons M1/M2 rouges", m4.couture[0] && m4.couture[1] && m4.couture[2] && m4.repereCol);
    cas("SF90 : manettino et démarreur ; palettes sur les 4", sf90.manettino && sf90.demarreur && golf.palettes && m4.palettes && p911.palettes && sf90.palettes);
    bool incl = true; for (int i = 0; i < 4; i++) incl = incl && interieur(i).volantIncl <= -0.29f && interieur(i).volantIncl >= -0.44f;
    cas("inclinaison de la colonne de 17 à 25° (SAE J1100, angle A18)", incl);
    // charte du réel : cuir noir albédo linéaire 0,04-0,12 (le rendu ramène sous 0,04 à 0,045)
    bool cuir = true; for (int i = 0; i < 4; i++) { const float y = lumLin(interieur(i).cuir); cuir = cuir && y >= 0.04f && y <= 0.12f; }
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
    for (int i = 0; i < 4; i++) { arcs = arcs && interieur(i).arc >= 1.4f && interieur(i).arc <= 1.75f; balais = balais && interieur(i).balaiG >= interieur(i).balaiD && interieur(i).balaiG > 0.5f; }
    cas("arcs de 80 à 100° ; balai conducteur ≥ passager (Golf 650 / 450, M4 600 / 475)", arcs && balais && golf.balaiG == 0.650f && m4.balaiD == 0.475f);
    // RG-07 : œil plus bas dans la 911 et la SF90 que dans la Golf (assise du modèle)
    cas("hauteur de l'œil : Golf > M4 > 911 > SF90", golf.oeilH > m4.oeilH && m4.oeilH > p911.oeilH && p911.oeilH > sf90.oeilH);
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("cockpit : tout est OK\n");
    return 0;
}
