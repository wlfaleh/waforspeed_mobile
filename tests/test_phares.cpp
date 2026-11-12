// Feux de croisement du joueur (WFS-218) : flux de la paire = flux sourcé (±5 %, PB-LUM), coupure à −0,57° et remontée de
// 15° côté droit (RG-01), presque plus de lumière au-dessus (pas de mur éclairé jusqu'au toit), route et pieds de façade
// proches éclairés, portée utile 60-80 m (RG-02), teintes 3500 K / 5700 K de luminance 1 ; WFS-226 : lumière au sol des
// autres voitures (même photométrie), étalement en azimut borné.
// usage : test_phares
#include <algorithm>
#include <cmath>
#include <utility>
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
    // WFS-226 : lumière au sol des autres voitures (paire LED) = même photométrie que les vrais projecteurs
    for (double z : {3.0, 10.0, 25.0}) {
        const double ref = lux(true, -0.65, 0, z, h, 0, 1, 0) + lux(true, 0.65, 0, z, h, 0, 1, 0), e = eclairementSol(0, (float)z, 0.65f, (float)h);
        VERIFIE(std::abs(e / ref - 1) < 0.01, "lumière au sol des autres à %.0f m : %.3f au lieu de %.3f lx", z, e, ref);
    }
    // WFS-226 : flanc d'une voiture voisine, à 45° de l'axe : moins de 3 % de la crête
    VERIFIE(intensiteRelative(-0.1f, 45.f) < 0.03f, "étalement à 45°");
    // WFS-382 (RG-01, ANO-222) : intensité limitée vers le bas (premier plan) : part de la crête à −3°, −5°, −10°, −15° dans les
    // fourchettes des feux LED de série (avis expert-realisme-3d du 2026-11-10 : 8 000-12 000, 3 000-5 000, 1 000-2 000, 500-1 000 cd
    // sur ≈ 40 000), et rapport premier plan (5 m) / tache à 30 m de 6 à 15 (réel ≈ 12 ; avant : ≈ 30, sol brûlé devant la voiture)
    {
        const struct { float el, bas, haut; } bornes[] = {{-3, 0.18f, 0.32f}, {-5, 0.07f, 0.13f}, {-10, 0.02f, 0.05f}, {-15, 0.009f, 0.025f}};
        for (const auto& b : bornes) { const float k = intensiteRelative(b.el, 0); VERIFIE(k >= b.bas && k <= b.haut, "intensité à %.0f° : %.3f de la crête", b.el, k); }
        auto paire = [&](double z) { return lux(true, -0.65, 0, z, h, 0, 1, 0) + lux(true, 0.65, 0, z, h, 0, 1, 0); };
        const double e5 = paire(5), e30 = paire(30), e50 = paire(50);
        std::printf("WFS-382 route (paire LED) : 5 m %.1f lx, 30 m %.2f lx, 50 m %.2f lx, rapport 5 m / 30 m %.1f ; crête %.0f cd\n", e5, e30, e50, e5 / e30, creteCd(true));
        VERIFIE(e5 / e30 >= 6 && e5 / e30 <= 15, "rapport premier plan / 30 m : %.1f", e5 / e30);
        VERIFIE(creteCd(true) >= 28000 && creteCd(true) <= 45000, "crête LED %.0f cd (30 000 à 45 000 cd)", creteCd(true));
    }
    // teintes de luminance 1
    for (const vec3& c : {TEINTE_HALOGENE, TEINTE_LED}) VERIFIE(std::abs(0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z - 1) < 0.01f, "luminance de la teinte");
    VERIFIE(!led("abarth") && led("golf") && !led(nullptr), "LED selon la voiture");
    // ---------- WFS-450 (RG-07) : ordre des lumières en Basse et fondu des phares des autres voitures ----------
    {
        // RG-01 : phares du joueur, puis lampadaires, puis phares des autres voitures, même si l'autre voiture est plus près de la caméra
        VERIFIE(avantEnBasse(RANG_FIXE, 30.f, RANG_AUTRES, 2.f), "un lampadaire à 30 m passe avant une voiture à 2 m de la caméra");
        VERIFIE(avantEnBasse(RANG_JOUEUR, 9.f, RANG_FIXE, 3.f) && !avantEnBasse(RANG_AUTRES, 1.f, RANG_JOUEUR, 50.f), "phares du joueur en premier");
        VERIFIE(avantEnBasse(RANG_FIXE, 5.f, RANG_FIXE, 9.f) && !avantEnBasse(RANG_FIXE, 9.f, RANG_FIXE, 5.f), "à rang égal : la plus proche de la caméra");
        avant450 = true;   // banc : ordre d'avant le ticket (distance seule)
        VERIFIE(!avantEnBasse(RANG_FIXE, 30.f, RANG_AUTRES, 2.f), "banc « avant » : la voiture proche passe devant");
        avant450 = false;
        // RG-03 : fondu avec la distance : plein à 55 m, nul à 70 m, sans saut
        VERIFIE(fonduDistance(40.f) == 1.f && fonduDistance(55.f) == 1.f && fonduDistance(70.f) == 0.f && fonduDistance(62.5f) > 0.4f && fonduDistance(62.5f) < 0.6f, "fondu de 55 à 70 m");
        // RG-03 : une voiture arrive à 20 m derrière le joueur : sa lumière monte en ≈ 0,3 s, sans saut de plus de 20 % par image (60 et 30 img/s)
        const vec3 joueur{0, 0, 0};
        auto voiture = [](float z, float x = 0) { PhareAutre p{}; p.centre = {x, 0.7f, z}; p.avant = {0, 0, -1}; p.droite = {1, 0, 0}; return p; };
        for (float dt : {1.f / 60, 1.f / 30}) {
            FonduAutres f; PhareAutre v[3] = {voiture(20)};
            float prec = 0, sautMax = 0, duree = 0; int images = 0;
            while (images < 200) {
                f.pas(v, 1, joueur, AUTRES_ECLAIRANTS_BASSE, dt); images++;
                const float w = FonduAutres::poids(f.places[0], joueur);
                sautMax = std::max(sautMax, w - prec); prec = w;
                if (w >= 1) { duree = images * dt; break; }
                v[0].centre.z -= 25 * dt;   // elle roule (25 m/s) : la place suit la voiture
            }
            VERIFIE(duree > 0.25f && duree < 0.4f, "montée en %.2f s à %.0f img/s (≈ 0,3 s)", duree, 1 / dt);
            VERIFIE(sautMax <= 0.2f, "saut de %.0f %% par image à %.0f img/s (20 %% au plus)", sautMax * 100, 1 / dt);
            // elle n'est plus dessinée : la lumière descend sur place, sans saut, puis la place est rendue
            sautMax = 0;
            for (int i = 0; i < 60 && f.places[0].occupee; i++) { f.pas(v, 0, joueur, AUTRES_ECLAIRANTS_BASSE, dt); const float w = FonduAutres::poids(f.places[0], joueur); sautMax = std::max(sautMax, prec - w); prec = w; }
            VERIFIE(!f.places[0].occupee && sautMax <= 0.2f, "descente : place rendue, saut de %.0f %% par image", sautMax * 100);
        }
        // changement de rang : 2 voitures éclairantes (10 m et 20 m), une 3e passe de 30 m à 8 m : elle monte pendant que celle de 20 m descend,
        // jamais plus de 2 voulues, et pas d'aller-retour quand deux voitures sont à la même distance (écart de 6 m)
        {
            FonduAutres f; PhareAutre v[3] = {voiture(10, -3), voiture(20, 3), voiture(30, 0)};
            const float dt = 1.f / 60; int changements = 0; bool voulue3 = false; float sautMax = 0, prec[FONDU_PLACES] = {};
            for (int i = 0; i < 600; i++) {
                v[2].centre.z = std::max(8.f, 30.f - 22.f * i / 300.f);
                f.pas(v, 3, joueur, AUTRES_ECLAIRANTS_BASSE, dt);
                int voulues = 0; bool v3 = false;
                for (int k = 0; k < FONDU_PLACES; k++) {
                    const FonduAutres::Place& p = f.places[k];
                    voulues += p.occupee && p.voulue; v3 = v3 || (p.occupee && p.voulue && p.indice == 2);
                    const float w = p.occupee ? FonduAutres::poids(p, joueur) : 0.f;
                    sautMax = std::max(sautMax, std::abs(w - prec[k])); prec[k] = w;
                }
                VERIFIE(voulues <= AUTRES_ECLAIRANTS_BASSE, "plus de 2 voitures voulues en Basse");
                if (v3 != voulue3) { changements++; voulue3 = v3; }
            }
            VERIFIE(changements == 1 && voulue3, "la 3e voiture prend la place une seule fois (%d changements)", changements);
            VERIFIE(sautMax <= 0.2f, "changement de rang : saut de %.0f %% par image", sautMax * 100);
            float somme = 0; for (const FonduAutres::Place& p : f.places) somme += p.occupee ? FonduAutres::poids(p, joueur) : 0.f;
            VERIFIE(std::abs(somme - 2.f) < 1e-3f, "à la fin : 2 lumières pleines (%.2f)", somme);
        }
        // entrée dans le rayon : à 69 m, la lumière existe mais presque nulle ; au-delà de 70 m, aucune place
        {
            FonduAutres f; PhareAutre v[1] = {voiture(75)};
            for (int i = 0; i < 30; i++) f.pas(v, 1, joueur, AUTRES_ECLAIRANTS_BASSE, 1.f / 60);
            VERIFIE(!f.places[0].occupee, "à 75 m : pas de vraie lumière");
            v[0] = voiture(69);
            for (int i = 0; i < 30; i++) f.pas(v, 1, joueur, AUTRES_ECLAIRANTS_BASSE, 1.f / 60);
            VERIFIE(f.places[0].occupee && FonduAutres::poids(f.places[0], joueur) < 0.05f, "à 69 m : lumière presque nulle (%.3f)", FonduAutres::poids(f.places[0], joueur));
        }
    }
    // ---------- WFS-450 : lampadaires éclairants en Basse (cause du noir d'un coup : lampadaire retiré 5 m derrière la voiture) ----------
    {
        // un lampadaire reste candidat 36 m derrière le joueur (portée 26 m + recul de la caméra) ; devant d'abord, à distance égale
        VERIFIE(LAMPES_DERRIERE_SEG * 1.2f >= 26.f + 9.f, "candidat derrière jusqu'à la fin de sa lumière visible");
        VERIFIE(cleLampe(10) < cleLampe(-10) && cleLampe(-5) < cleLampe(16) && cleLampe(-5) > cleLampe(14), "clé : derrière compte 3 fois");
        // Corniche : un lampadaire tous les 30 segments ; le joueur avance de 0,35 segment par image (25 m/s à 60 img/s).
        // Le lampadaire qui vient d'être dépassé reste plein tant qu'il est à moins de 26 m (21 segments) : plus de noir d'un coup.
        FonduLampes f; float sautMax = 0, precPoids[64] = {}; bool pleinPres = true;
        for (int im = 0; im < 2000; im++) {
            const double pos = im * 0.35;   // segments
            std::pair<float, int64_t> c[64]; int nc = 0;
            for (int64_t id = 0; id < 40; id++) { const int64_t e = (int64_t)std::floor(15 + 30 * id - pos); if (e >= -LAMPES_DERRIERE_SEG && e < LAMPES_DEVANT_SEG) c[nc++] = {cleLampe(e), id}; }
            std::sort(c, c + nc);
            int64_t voulus[LAMPES_ECLAIRANTES]; int n = 0;
            for (int j = 0; j < nc && n < LAMPES_ECLAIRANTES; j++) voulus[n++] = c[j].second;
            f.pas(voulus, n, 1.f / 60);
            float poidsDe[64] = {};
            for (const FonduLampes::Place& p : f.places) if (p.id >= 0) poidsDe[p.id] = FonduLampes::poids(p);
            for (int id = 0; id < 40; id++) {
                const double e = 15 + 30 * id - pos;
                if (im > 0) sautMax = std::max(sautMax, std::abs(poidsDe[id] - precPoids[id]));
                if (e > -21 && e < 21 && im > 30) pleinPres = pleinPres && poidsDe[id] >= 0.999f;
                precPoids[id] = poidsDe[id];
            }
        }
        VERIFIE(pleinPres, "un lampadaire à moins de 26 m du joueur (devant ou derrière) éclaire toujours à plein");
        VERIFIE(sautMax <= 0.2f, "lampadaires : saut de %.0f %% par image (20 %% au plus)", sautMax * 100);
        FonduLampes g1; const int64_t v1[2] = {7, 9}; g1.pas(v1, 2, 0.f);
        VERIFIE(FonduLampes::poids(g1.places[0]) == 1.f && FonduLampes::poids(g1.places[1]) == 1.f, "première image : lampadaires pleins tout de suite");
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("test_phares : OK\n");
    return 0;
}
