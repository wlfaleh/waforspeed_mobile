// WFS-541 (ANO-269, ANO-270) : trafic de la Poursuite après les chocs.
//  A. RG-01 : une voiture du trafic heurtée, joueur arrêté à côté d'elle (avant : elle restait en travers tant qu'il était à
//     moins de 60 m) : arrêtée 3 s au plus, puis elle se redresse en roulant (≤ 15 km/h) et reprend sa file, en 8 s pour un petit
//     choc ; tournée de plus de 50° (90°, 143°, 180°), demi-tour en plusieurs temps sans s'arrêter, 11 s au plus ; jamais retirée
//     sous les yeux du joueur (A bis : retirée dès qu'elle est hors de la vue, passé 8 s).
//  B. Banc CPU (critère du ticket) : Corniche, trafic dense de la Poursuite, 200 chocs autour du joueur (qui roule et s'arrête) :
//     aucune voiture du trafic arrêtée plus de 10 s (RG-02, hors feu rouge ; les deux-roues couchés au sol sont mesurés à part :
//     sans animation du pilote qui se relève, ils ne sont relevés que hors de la vue, comme avant), aucune pénétration de plus de 5 cm entre deux
//     voitures du trafic ni avec le joueur, à chaque pas (RG-03) ; coût CPU par image avec / sans les contacts (RG-05).
//  C. RG-03 : le trafic s'écarte d'une voiture qu'on ne déplace pas (police) : separerDe ne bouge que le trafic.
// usage : test_trafic_chocs541
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include "../src/game/player.h"
#include "../src/game/trace_cartes.h"
#include "../src/game/traffic.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// pénétration la plus forte (m) entre deux voitures du trafic (emprises dessinées)
static double pireTrafic(const Traffic& tr, const Track& t, int* ia = nullptr, int* ib = nullptr) {
    double pire = 0;
    for (size_t a = 0; a < tr.cars.size(); a++) {
        const TrafficCar& A = tr.cars[a];
        if (A.cache) continue;
        for (size_t b = a + 1; b < tr.cars.size(); b++) {
            const TrafficCar& B = tr.cars[b];
            if (B.cache) continue;
            const double dz = t.relZ(B.z - A.z);
            if (std::abs(dz) * M > 12) continue;
            const double o = recouvrement(empriseTrafic(A, 0), empriseTrafic(B, dz));
            if (o > pire) { pire = o; if (ia) *ia = (int)a; if (ib) *ib = (int)b; }
        }
    }
    return pire;
}
// voiture arrêtée devant un feu rouge ou orange (attente normale : RG-02 ne la compte pas)
static bool auFeu(const Track& t, const TrafficCar& c, double sim) {
    for (const trace::Section& s : t.sections()) {
        if (s.feu < 0 || s.depart) continue;
        const double dz = t.relZ(s.ligneArret * SEG - c.z) * M;
        if (dz > -2 && dz < 200 && trace::feuCarrefour(s.feu, sim) != trace::Feu::Vert) return true;
    }
    return false;
}

int main() {
    static Track t; t.build(mapById("plage"), trace::versionCarte("plage"));
    // ---------- A. RG-01 : voiture heurtée, joueur arrêté à côté ----------
    struct Cas { double impact; int side; double cap; const char* nom; };
    const Cas cas[] = {{0.2, 1, 99, "choc léger à droite"}, {0.5, -1, 99, "choc moyen à gauche"}, {0.9, 1, 99, "gros choc à droite"},
                       {0.9, -1, 99, "gros choc à gauche"}, {0.5, 1, 1.5708, "tournée de 90°"}, {0.5, -1, 3.1416, "nez vers l'arrière"},
                       {0.5, 1, -2.5, "tournée de 143°"}};
    for (const Cas& k : cas) {
        Traffic tr; tr.reset(t, 541, 8, 0); tr.clock = tr.sim = 0;
        TrafficView pv; pv.position = t.wrap(200 / M); pv.playerX = -2.0 / 3; pv.speed = 0; pv.myLen = 4.3 / M;
        for (size_t i = 1; i < tr.cars.size(); i++) tr.cars[i].z = t.wrap(pv.position + (600 + 40 * i) / M);   // les autres loin devant
        TrafficCar& c = tr.cars[0];
        c.lane = c.target = 1; c.offset = c.prevOff = 0; c.z = t.wrap(pv.position + 12 / M); c.speed = 50 / 3.6 / M; c.want = c.speed;
        Traffic::crash(c, k.impact, k.side);
        if (k.cap != 99) { c.crashT = 0.5; c.crashYaw = k.cap; c.crashSpin = 0; c.crashVx = 0; c.speed = 0; }
        double arretMax = 0, remis = -1, vMax = 0, dzMin = 1e9, arretManoeuvre = 0;
        for (int n = 0; n < 60 * 12; n++) {
            tr.step(T_STEP, pv); tr.sim += T_STEP;
            const double dz = t.relZ(c.z - pv.position) * M;
            if (c.crashed) { arretMax = std::max(arretMax, c.arretT); dzMin = std::min(dzMin, dz);
                if (c.redresse) { vMax = std::max(vMax, std::abs(c.vRed)); arretManoeuvre = std::max(arretManoeuvre, c.arretT); } }
            else if (remis < 0) remis = (n + 1) * T_STEP;
        }
        const double ecartFile = std::abs(c.offset - LANE_X[c.lane] * tr.facteurVoie(c.z)) * RW;
        std::printf("A. %-20s : arrêt max %.2f s (pendant la manœuvre %.2f s), remise en route à %.2f s, vitesse max %.1f km/h, file %d (écart %.2f m), vitesse finale %.0f km/h\n",
                    k.nom, arretMax, arretManoeuvre, remis, vMax * 3.6, c.lane, ecartFile, c.speed * M * 3.6);
        VERIFIE(arretMax <= 3.0, "%s : arrêtée %.2f s (> 3 s)", k.nom, arretMax);
        // RG-01 : remise dans sa file en 8 s ; une voiture tournée de plus de 50° fait un demi-tour en plusieurs temps, qui peut
        // dépasser 8 s : elle roule tout le temps (jamais arrêtée plus de 1,5 s, le temps d'un changement de sens) et, si elle
        // est encore en manœuvre, elle est retirée dès qu'elle sort de la vue (cas A bis) ; jamais plus de 11 s
        const bool tournee = k.cap != 99 || k.impact >= 0.5;
        VERIFIE(remis > 0 && remis <= (tournee ? 11.0 : RETRAIT_APRES_S), "%s : pas remise en route (%.2f s)", k.nom, remis);
        VERIFIE(arretManoeuvre <= 1.5, "%s : arrêtée %.2f s pendant la manœuvre", k.nom, arretManoeuvre);
        VERIFIE(vMax <= REDRESSE_V + 1e-6, "%s : redressement à %.1f km/h (> 15 km/h)", k.nom, vMax * 3.6);
        VERIFIE(ecartFile < 0.5 && !c.crashed && c.speed * M > 5, "%s : pas repartie dans sa file", k.nom);
        VERIFIE(dzMin > 2, "%s : la voiture est passée sur le joueur (%.1f m)", k.nom, dzMin);
    }
    // A bis : coincée hors de la route, joueur à côté : jamais retirée sous ses yeux ; retirée dès qu'elle est hors de la vue
    {
        Traffic tr; tr.reset(t, 77, 4, 0); tr.clock = tr.sim = 0;
        TrafficView pv; pv.position = t.wrap(300 / M); pv.speed = 0; pv.myLen = 4.3 / M;
        TrafficCar& c = tr.cars[0];
        c.z = t.wrap(pv.position + 20 / M); Traffic::crash(c, 0.6, 1); c.crashT = -30;   // accident posé (surprise) : ne se redresse pas
        double saut = 0;
        for (int n = 0; n < 60 * 20; n++) { const double z0 = c.z; tr.step(T_STEP, pv); tr.sim += T_STEP; saut = std::max(saut, std::abs(t.relZ(c.z - z0)) * M); }
        const double dz0 = t.relZ(c.z - pv.position) * M;
        VERIFIE(c.crashed && saut < 1, "accident posé : déplacé d'un coup sous les yeux du joueur (%.1f m en un pas)", saut);
        pv.position = t.wrap(pv.position + 400 / M);   // le joueur est passé : elle est 380 m derrière lui
        c.crashT = RETRAIT_APRES_S + 1;
        tr.step(T_STEP, pv);
        std::printf("A. accident posé : reste à %.0f m du joueur arrêté (pas retiré sous ses yeux) ; hors de la vue : %s\n", dz0, c.crashed ? "toujours là" : "retiré");
        VERIFIE(!c.crashed, "accident hors de la vue depuis plus de 8 s : pas retiré");
    }
    // ---------- B. banc CPU : 200 chocs dans le trafic dense ----------
    // mode 0 : sans les contacts du trafic (comme main), pour le coût ; 1 : un choc toutes les 1,5 s (critère du ticket) ;
    // 2 : carambolage, un choc toutes les 0,4 s (bien au-delà d'une partie) : pénétration seulement
    for (int mode = 0; mode < 3; mode++) {
        const bool avecContacts = mode > 0; const double intervalle = mode == 2 ? 0.4 : 1.5;
        Traffic tr; tr.contactsTrafic = avecContacts; tr.totalFixe = true;
        tr.reset(t, 2026, (int)std::lround(mapById("plage").trafficN * 2.5), 0);   // trafic dense de la Poursuite (poursuite.densite = 2,5)
        Player p; p.reset(carById("golf")); p.aide = Aide::Desactivee; p.speed = 60 / 3.6 / M;
        Mulberry32 R(541);
        int chocs = 0, chocsJeu = 0; double pireTT = 0, pireJ = 0, arretMax = 0, arretDR = 0, tmax = 0, coutTotal = 0; int images = 0, ia = -1, ib = -1; char quiArret[160] = "", quiPen[240] = "";
        double prochain = 2;
        for (int n = 0; chocs < 200 || n < 120 * 30; n++) {
            const double tps = n / 120.0;
            if (n > 120 * 900) break;
            const bool arret = std::fmod(tps, 9.0) > 6.0;   // roule 6 s, s'arrête 3 s (police, bouchon)
            DriveInput in; in.gas = !arret; in.brake = arret && p.speed > 0;
            in.steer = (float)std::clamp((0 - p.playerX) * 3 - p.heading * 4, -1.0, 1.0);
            p.update(1.0 / 120, in, t, true);
            if (p.speed < 0) p.speed = 0;
            TrafficView pv; pv.position = p.position; pv.playerX = p.playerX; pv.speed = p.speed; pv.myLen = p.car->d.L / M;
            const auto t0 = std::chrono::steady_clock::now();
            tr.advance(1.0 / 120, pv);
            const HitEvents e = tr.collide(p, 1.0 / 120);
            const auto t1 = std::chrono::steady_clock::now();
            if (n % 2 == 1) { coutTotal += std::chrono::duration<double, std::milli>(t1 - t0).count() * 2; images++; }   // 60 images/s
            chocsJeu += e.bumps + e.accidents;
            // chocs provoqués : toutes les 1,5 s, une voiture du trafic à moins de 120 m du joueur part en glissade
            if (tps >= prochain && chocs < 200) {
                prochain = tps + intervalle;
                // mise en place du banc (comme les embouteillages de la Poursuite, poursuite_system « deplacer ») : une voiture
                // lointaine est amenée près du joueur, à une place libre de sa file, pour que les chocs se suivent autour de lui
                int proches = 0;
                for (const TrafficCar& c : tr.cars) proches += !c.crashed && !c.cache && std::abs(t.relZ(c.z - p.position)) * M < 60;
                if (proches < 6)
                    for (TrafficCar& c : tr.cars) {
                        const double rel = t.relZ(c.z - p.position) * M;
                        if (c.crashed || c.cache || c.traverse >= 0 || (rel > -200 && rel < 400)) continue;
                        const int f = (int)(R.next() * 3) % 3; const double z = t.wrap(p.position + (R.next() < 0.7 ? 20 + 50 * R.next() : -12 - 20 * R.next()) / M);
                        // place libre : à plus d'1 m de toute carrosserie (emprises dessinées), joueur compris
                        TrafficCar essai = c; essai.z = z; essai.offset = LANE_X[f]; essai.laneYaw = 0;
                        bool libre = recouvrement(empriseJoueur(p), empriseTrafic(essai, t.relZ(z - p.position))) < -1;
                        for (const TrafficCar& o : tr.cars)
                            if (&o != &c && !o.cache && std::abs(t.relZ(o.z - z)) * M < 15 && recouvrement(empriseTrafic(o, 0), empriseTrafic(essai, t.relZ(z - o.z))) > -1) libre = false;
                        if (!libre) break;
                        c.z = c.zPrec = z; c.lane = c.target = f; c.offset = c.prevOff = c.offPrec = LANE_X[f]; c.speed = p.speed; c.pending = false; c.signal = 0;
                        c.prevDz = t.relZ(z - p.position);
                        break;
                    }
                int best = -1; double bd = 120;
                for (size_t i = 0; i < tr.cars.size(); i++) {
                    const TrafficCar& c = tr.cars[i];
                    if (c.crashed || c.cache || c.traverse >= 0) continue;
                    const double d = std::abs(t.relZ(c.z - p.position)) * M;
                    if (d < bd && d > 6) { bd = d; best = (int)i; }
                }
                if (best >= 0) { Traffic::crash(tr.cars[(size_t)best], 0.2 + 0.8 * R.next(), R.next() < 0.5 ? -1 : 1); chocs++; }
            }
            if (!avecContacts) continue;
            int a = -1, b = -1;
            const double o = pireTrafic(tr, t, &a, &b);
            if (o > pireTT) { pireTT = o; ia = a; ib = b; tmax = tps;
                const TrafficCar &A = tr.cars[(size_t)a], &B = tr.cars[(size_t)b];
                std::snprintf(quiPen, sizeof quiPen, "A : %s%s, %.1f m du joueur, x %.2f m ; B : %s%s, %.1f m, x %.2f m", A.crashed ? "accidentée" : "roule", A.redresse ? " (se redresse)" : "",
                              t.relZ(A.z - p.position) * M, A.offset * RW, B.crashed ? "accidentée" : "roule", B.redresse ? " (se redresse)" : "", t.relZ(B.z - p.position) * M, B.offset * RW); }
            for (const TrafficCar& c : tr.cars)
                if (!c.cache) pireJ = std::max(pireJ, recouvrement(empriseJoueur(p), empriseTrafic(c, t.relZ(c.z - p.position))));
            for (const TrafficCar& c : tr.cars)
                if (!c.cache && c.traverse < 0 && c.deuxRoues) arretDR = std::max(arretDR, c.arretT);   // deux-roues : à part (voir l'en-tête)
                else if (!c.cache && c.traverse < 0 && !auFeu(t, c, tr.sim) && c.arretT > arretMax) {
                    arretMax = c.arretT;
                    std::snprintf(quiArret, sizeof quiArret, "voiture %d%s%s, %.0f m du joueur, à %.1f s", (int)(&c - tr.cars.data()), c.deuxRoues ? " (deux-roues)" : "",
                                  c.crashed ? (c.redresse ? " (se redresse)" : " (accidentée)") : "", t.relZ(c.z - p.position) * M, tps);
                }
        }
        const double cout = coutTotal / std::max(1, images);
        { int ncr = 0; for (const TrafficCar& c : tr.cars) ncr += c.crashed; std::printf("   (joueur à %.0f m, %.0f km/h, %zu voitures, %d accidentées)\n", p.dist * M, p.kmh(), tr.cars.size(), ncr); }
        if (!avecContacts) { std::printf("B. sans contacts du trafic (main) : %.3f ms CPU par image (trafic + chocs)\n", cout); continue; }
        std::printf("B (%s). %d chocs provoqués, %d chocs du jeu, %d images : pénétration max trafic / trafic %.3f m (voitures %d et %d, %.1f s), "
                    "trafic / joueur %.3f m, arrêt max %.1f s (hors feu : %s), %.3f ms CPU par image\n",
                    mode == 1 ? "un choc toutes les 1,5 s" : "carambolage, un choc toutes les 0,4 s", chocs, chocsJeu, images, pireTT, ia, ib, tmax, pireJ, arretMax, quiArret, cout);
        VERIFIE(chocs >= 200, "banc : %d chocs seulement", chocs);
        if (pireTT > 0) std::printf("   pire pénétration : %s\n", quiPen);
        std::printf("   deux-roues couché au sol le plus longtemps : %.1f s (relevé seulement hors de la vue, comme avant WFS-541)\n", arretDR);
        VERIFIE(pireTT <= 0.05, "banc : deux voitures du trafic l'une dans l'autre (%.3f m > 5 cm)", pireTT);
        VERIFIE(pireJ <= 0.05, "banc : une voiture du trafic dans celle du joueur (%.3f m > 5 cm)", pireJ);
        if (mode == 1) VERIFIE(arretMax <= 10, "banc : une voiture du trafic arrêtée %.1f s (> 10 s)", arretMax);
    }
    // ---------- C. trafic écarté d'une voiture qu'on ne déplace pas (police) ----------
    {
        Traffic tr; tr.reset(t, 9, 2, 0);
        TrafficCar& c = tr.cars[0]; c.z = t.wrap(500 / M); c.offset = 0.1; c.lane = c.target = 1;
        const Emprise police{0.0, 0.0, 0.3, 4.06 / 2, 1.75 / 2};   // 208 de travers, posée à z = 500 m + 1 m
        const double zP = t.wrap((500 + 1.0) / M);
        const int k = tr.separerDe(police, zP);
        const double o = recouvrement(police, empriseTrafic(c, t.relZ(c.z - zP)));
        std::printf("C. police : %d voiture(s) du trafic écartée(s), recouvrement restant %.3f m (police non déplacée)\n", k, std::max(0.0, o));
        VERIFIE(k == 1 && o <= 0.0, "police : trafic pas écarté (%.3f m)", o);
    }
    std::printf(echecs ? "test_trafic_chocs541 : %d ÉCHEC(S)\n" : "test_trafic_chocs541 : tout est bon\n", echecs);
    return echecs ? 1 : 0;
}
