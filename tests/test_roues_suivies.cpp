// WFS-452 : roues des voitures conduites par le jeu (trafic, Course à 5, duel, police, fantôme) : roulage et braquage.
// RG-02 : l'angle de roulage change quand la voiture avance, reste fixe à l'arrêt ;
// RG-03 : braquage selon le virage et le changement de file, 0 après un accident ;
// RG-08 : la voiture qui glisse de côté braque aussi (contre-braquage) ;
// RG-09 (ANO-265) : en ligne droite sans changement de file, braquage = 0 sur 300 images ; sans saut d'une image à l'autre,
// ±35° au plus ; mesuré sur le vrai trafic de la Ville (tracé 2) : avant WFS-452, roues avant jusqu'à 2,5 rad et en sauts.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
#include "../src/core/jobs.h"
#include "../src/game/roues.h"
#include "../src/game/traffic.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

constexpr double H = 1.0 / 120;    // pas fixe du jeu
constexpr int PAS_IMAGE = 4;       // une image à 30 img/s = 4 pas
constexpr float EMP = 2.65f;       // empattement (m) d'une berline compacte
constexpr float ECART_IMAGE_MAX = roues::BRAQUAGE_VITESSE / 30.f + 1e-4f;   // écart maxi entre deux images à 30 img/s

int main() {
    // 1. ligne droite à 50 km/h, sans changement de file : braquage exactement 0 sur 300 images
    {
        roues::SuiviRoues sv; double s = 0; float maxi = 0;
        for (int i = 0; i < 300 * PAS_IMAGE; i++) { s += 13.9 * H; sv.pas(s, 1.875, 0, 0, EMP, H); maxi = std::max(maxi, std::abs(sv.braquage)); }
        VERIFIE(maxi == 0.f, "ligne droite : braquage %.6f au lieu de 0", maxi);
        std::printf("ligne droite, 300 images : braquage maxi %.6f rad : OK\n", maxi);
    }
    // 2. virage de 20 m de rayon à 36 km/h (à droite puis à gauche) : roues avant à atan(empattement / rayon)
    for (int sens : {1, -1}) {
        roues::SuiviRoues sv; double s = 0;
        for (int i = 0; i < 240; i++) { s += 10 * H; sv.pas(s, 0, 0, sens / 20.0, EMP, H); }
        const double attendu = sens * std::atan(EMP / 20.0);
        VERIFIE(std::abs(sv.braquage - attendu) < 0.01 * std::abs(attendu), "virage %+d : braquage %.4f au lieu de %.4f", sens, sv.braquage, attendu);
        std::printf("virage de 20 m (%s) : braquage %.4f rad (attendu %.4f) : OK\n", sens > 0 ? "droite" : "gauche", sv.braquage, attendu);
    }
    // 3. changement de file comme le trafic (glissement latéral de 3,75 m/s, départ et arrêt francs, caisse tournée vers sa
    //    direction, laneYaw) : braquage visible, aucun saut entre deux images, revient à 0, ±35° au plus
    for (double v : {14.0, 4.5}) {   // 50 km/h ; 16 km/h (voiture lente : avant, roues à 1,7 rad)
        roues::SuiviRoues sv; double s = 0, lat = 0; float maxi = 0, sautMax = 0, precImage = 0;
        for (int i = 0; i < 6 * 120; i++) {
            const bool glisse = i >= 120 && lat < 3.75;
            const double dl = glisse ? std::min(3.75 * H, 3.75 - lat) : 0;
            lat += dl; s += v * H;
            sv.pas(s, lat, std::atan2(dl, v * H), 0, EMP, H);
            maxi = std::max(maxi, std::abs(sv.braquage));
            if (i % PAS_IMAGE == 0) { sautMax = std::max(sautMax, std::abs(sv.braquage - precImage)); precImage = sv.braquage; }
        }
        VERIFIE(maxi > 0.02f, "changement de file à %.1f m/s : braquage invisible (%.4f rad)", v, maxi);
        VERIFIE(maxi <= roues::BRAQUAGE_MAX, "changement de file à %.1f m/s : %.3f rad > 35°", v, maxi);
        VERIFIE(sautMax <= ECART_IMAGE_MAX, "changement de file à %.1f m/s : saut de %.4f rad entre deux images", v, sautMax);
        VERIFIE(std::abs(sv.braquage) < 1e-3f, "changement de file à %.1f m/s : roues pas revenues droites (%.4f rad)", v, sv.braquage);
        std::printf("changement de file à %.1f m/s : braquage maxi %.3f rad, saut maxi %.4f rad par image, final %.5f : OK\n", v, maxi, sautMax, sv.braquage);
    }
    // 4. glisse de côté (police poussée, caisse dans l'axe de la route) : les roues avant braquent vers où va la voiture
    {
        roues::SuiviRoues sv; double s = 0, lat = 0;
        for (int i = 0; i < 60; i++) { s += 20 * H; lat += 3 * H; sv.pas(s, lat, 0, 0, EMP, H); }
        VERIFIE(sv.braquage > 0.05f, "glisse vers la droite : braquage %.4f (attendu > 0,05)", sv.braquage);
        std::printf("glisse de côté vers la droite : braquage %.3f rad : OK\n", sv.braquage);
    }
    // 5. arrêt : angle gardé ; accident ou voiture qui traverse : roues droites ; roulage qui change quand la voiture avance
    {
        roues::SuiviRoues sv; double s = 0;
        for (int i = 0; i < 240; i++) { s += 10 * H; sv.pas(s, 0, 0, 1 / 20.0, EMP, H); }
        const float b = sv.braquage;
        for (int i = 0; i < 240; i++) sv.pas(s, 0, 0, 1 / 20.0, EMP, H);
        VERIFIE(sv.braquage == b, "arrêt : braquage %.4f ≠ %.4f", sv.braquage, b);
        const RouesPose p0 = roues::poseSuivi(sv, 0.5, s / M, 0.32f), p1 = roues::poseSuivi(sv, 0.5, s / M, 0.32f);
        VERIFIE(p0.roulage == p1.roulage, "arrêt : roulage qui change");
        const RouesPose p2 = roues::poseSuivi(sv, 0.5, (s + 0.10) / M, 0.32f);
        VERIFIE(std::abs((p2.roulage - p0.roulage) + 0.10f / 0.32f) < 1e-3f, "roulage : %.4f rad pour 10 cm (attendu %.4f)", p2.roulage - p0.roulage, -0.10f / 0.32f);
        for (int i = 0; i < 120; i++) { s += 10 * H; sv.pas(s, 0, 0, 1 / 20.0, EMP, H, true); }
        VERIFIE(sv.braquage == 0.f, "accident : braquage %.4f au lieu de 0", sv.braquage);
        std::printf("arrêt (angle et roulage gardés), roulage = −distance / rayon, accident (roues droites) : OK\n");
    }
    // 6. sens de la courbure : courbureRoute + à droite = cap de la route (TrackPoint::th) qui augmente, comme laneYaw et le braquage
    jobs().start();
    Track t; t.build(mapById("ville"), 2);
    {
        int verifies = 0, faux = 0;
        for (double s = 0; s < t.length() && verifies < 2000; s += SEG * 3) {
            const double k = roues::courbureRoute(t, s + SEG / 2);
            if (std::abs(k) < 0.01) continue;
            const double dth = t.point(s + SEG, 0).th - t.point(s, 0).th;
            verifies++; faux += (dth > 0) != (k > 0);
        }
        VERIFIE(verifies > 20 && faux == 0, "sens de la courbure : %d faux sur %d", faux, verifies);
        std::printf("sens de la courbure (%d segments en courbe) : OK\n", verifies);
    }
    // 7. vrai trafic de la Ville, 2 min : hors changement de file et en ligne droite, roues droites ; jamais plus de 35° ;
    //    aucun saut entre deux images (ANO-265)
    {
        Traffic tr; tr.parallel = false;
        TrafficView pv; pv.position = 0; pv.playerX = 0; pv.speed = MAX_SPEED * 0.4; pv.myLen = webCarLength("golf", carById("golf").d.L);
        tr.reset(t, 12345u, mapById("ville").trafficN, pv.position);
        std::vector<roues::SuiviRoues> sv(tr.cars.size());
        std::vector<float> precImage(tr.cars.size(), 0);
        float maxi = 0, sautMax = 0, droitMax = 0; long droits = 0, mesures = 0;
        std::vector<double> calme(tr.cars.size(), 0), calmeT(tr.cars.size(), 0);   // m et s (en roulant) depuis le dernier changement de file ou virage
        for (int i = 1; i <= 120 * 120; i++) {
            tr.step(H, pv); pv.position = t.wrap(pv.position + pv.speed * H);
            if (sv.size() != tr.cars.size()) { sv.resize(tr.cars.size()); precImage.resize(tr.cars.size(), 0); calme.resize(tr.cars.size(), 0.0); calmeT.resize(tr.cars.size(), 0.0); }
            for (size_t k = 0; k < tr.cars.size(); k++) {
                const TrafficCar& c = tr.cars[k];
                if (c.deuxRoues) continue;
                const double kap = roues::courbureRoute(t, c.z);
                sv[k].pas(c.z * M, c.offset * RW, c.laneYaw, kap, EMP, H, c.crashed || c.traverse >= 0);
                const float b = sv[k].braquage;
                maxi = std::max(maxi, std::abs(b)); mesures++;
                const bool tranquille = c.target == c.lane && !c.pending && std::abs(c.laneYaw) < 1e-9 && kap == 0 && !c.crashed && c.traverse < 0;
                calme[k] = tranquille ? calme[k] + c.speed * M * H : 0;
                calmeT[k] = tranquille ? calmeT[k] + (c.speed * M * H > 1e-4 ? H : 0) : 0;
                if (calme[k] > 5 && calmeT[k] > 1) { droits++; droitMax = std::max(droitMax, std::abs(b)); }   // 5 m et 1 s de roulage après la fin du virage ou de la file (à l'arrêt, une vraie voiture garde ses roues)
                if (i % PAS_IMAGE == 0) {
                    if (i > PAS_IMAGE) sautMax = std::max(sautMax, std::abs(b - precImage[k]));
                    precImage[k] = b;
                }
            }
        }
        VERIFIE(maxi <= roues::BRAQUAGE_MAX + 1e-6f, "trafic : braquage %.3f rad > 35°", maxi);
        VERIFIE(sautMax <= ECART_IMAGE_MAX, "trafic : saut de %.4f rad entre deux images", sautMax);
        VERIFIE(droits > 10000 && droitMax < 2e-3f, "trafic en ligne droite : braquage %.4f rad (%ld mesures)", droitMax, droits);
        std::printf("trafic de la Ville, 2 min (%ld mesures) : braquage maxi %.3f rad, saut maxi %.4f rad par image, en ligne droite %.5f rad (%ld mesures) : OK\n",
                    mesures, maxi, sautMax, droitMax, droits);
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("roues suivies : tout est OK\n");
    return 0;
}
