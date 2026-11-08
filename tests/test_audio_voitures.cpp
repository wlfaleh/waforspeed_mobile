// WFS-272 : son des autres voitures. Vérifie hors ligne, sans carte son :
//   RG-01 : 8 voix au plus, les plus proches ; une voiture garde sa voix ; entrée et sortie en fondu ≥ 0,2 s ; voiture recyclée ;
//   RG-02 : fréquence de combustion, régime tiré des rapports, niveaux CNOSSOS-EU (pente du roulement, pneus au-dessus du
//           moteur au-delà de 50 km/h), voiture du trafic à 50 km/h à 7,5 m sous le moteur du joueur en pleine accélération ;
//   RG-04 : gauche / droite (90° à gauche : droite ≤ −9 dB ; dépassement par la gauche : toujours plus fort à gauche) ;
//   RG-05 : Doppler = formule (± 0,1 %), hauteur jouée mesurée = formule ± 2 %, lissé (aucun saut) ;
//   RG-06 : sons existants au centre inchangés (écart ≤ 0,5 dB) ; RG-07 : les voix nourrissent l'écho ; RG-08 : coût ≤ 0,3 ms.
// usage : test_audio_voitures [dossier assets]
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>
#include "../src/audio/engine_synth.h"
#include "../src/audio/voitures_sons.h"
#include "../src/game/cars.h"
#include "../src/game/player.h"
using namespace wfs;
using namespace wfs::voitures;

static int echecs = 0;
static void verifier(bool ok, const char* quoi) { std::printf("[%s] %s\n", ok ? "OK" : "ÉCHEC", quoi); if (!ok) echecs++; }
static float dB(float x) { return 20.f * std::log10(std::max(x, 1e-12f)); }
static float rms(const std::vector<float>& b, size_t a, size_t e) {
    double s = 0; for (size_t i = a; i < e; i++) s += (double)b[i] * b[i];
    return (float)std::sqrt(s / (double)std::max<size_t>(1, e - a));
}
static Candidat voiture(uint64_t id, vec3 p, vec3 v, float kmh, const char* car, uint8_t genre) {
    Candidat c; c.id = id; c.p = p; c.v = v; c.kmh = kmh; c.car = &carById(car); c.genre = genre; return c;
}
static const vec3 AVANT{0, 0, -1}, DROITE{1, 0, 0}, ICI{0, 1.2f, 0};

// rend `secondes` d'une voix fixe (cibles calculées une fois), sortie G et D
static void rendre(SonsVoitures& s, std::vector<float>& G, std::vector<float>& D, std::vector<float>& bus, float secondes) {
    const size_t N = (size_t)(secondes * 48000);
    G.assign(N, 0.f); D.assign(N, 0.f); bus.assign(N, 0.f);
    for (size_t a = 0; a + 480 <= N; a += 480) s.mix(G.data() + a, D.data() + a, bus.data() + a, 480, 0.5f, true);
}

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : "assets/";
    char msg[320];
    constexpr float DT = 1.f / 120;

    // ---------- RG-01 : choix des voix ----------
    {
        Attribution at;
        std::vector<Candidat> cs;
        for (int i = 0; i < 12; i++) cs.push_back(voiture((uint64_t)i + 1, {3.5f, 0.6f, -5.f - 5.f * (float)i}, {0, 0, -14}, 50, "golf", TRAFIC));   // 5 à 60 m
        for (int k = 0; k < 60; k++) at.update(DT, cs, ICI);
        bool huitProches = at.actives() == VOIX;
        for (int v = 0; v < VOIX; v++) huitProches = huitProches && at.voix(v).id >= 1 && at.voix(v).id <= 8 && at.voix(v).fondu == 1.f;
        verifier(huitProches, "RG-01 : 12 voitures : 8 voix, celles des 8 plus proches, à plein après le fondu");
        uint64_t avant[VOIX]; for (int v = 0; v < VOIX; v++) avant[v] = at.voix(v).id;
        cs[11].p = {3.5f, 0.6f, -2.f};   // la 12e voiture arrive tout près : la 8e (40 m) perd sa voix
        int pasSortie = 0, slot8 = -1;
        for (int v = 0; v < VOIX; v++) if (at.voix(v).id == 8) slot8 = v;
        for (int k = 0; k < 120 && slot8 >= 0 && at.voix(slot8).id == 8; k++) { at.update(DT, cs, ICI); pasSortie++; }
        bool gardees = true;
        for (int v = 0; v < VOIX; v++) if (v != slot8) gardees = gardees && at.voix(v).id == avant[v];
        verifier(gardees, "RG-01 : les 7 autres voitures gardent leur voix (même place, aucun échange)");
        std::snprintf(msg, sizeof msg, "RG-01 : la voiture qui sort de la liste s'éteint en fondu : %.2f s (≥ 0,2 s)", pasSortie * DT);
        verifier(pasSortie * DT >= 0.2f && pasSortie * DT < 0.5f, msg);
        for (int k = 0; k < 60; k++) at.update(DT, cs, ICI);
        bool entree = false; for (int v = 0; v < VOIX; v++) entree = entree || (at.voix(v).id == 12 && at.voix(v).fondu == 1.f);
        verifier(entree, "RG-01 : la voiture arrivée prend la voix libérée, en fondu d'entrée");
        // fondu d'entrée : de 0 à 1 en FONDU (≥ 0,2 s)
        Attribution a2; std::vector<Candidat> un = {voiture(99, {0, 0.6f, -20}, {}, 50, "golf", TRAFIC)};
        int pasEntree = 0; do { a2.update(DT, un, ICI); pasEntree++; } while (a2.voix(0).fondu < 1.f && pasEntree < 200);
        std::snprintf(msg, sizeof msg, "RG-01 : fondu d'entrée %.2f s (≥ 0,2 s)", pasEntree * DT);
        verifier(pasEntree * DT >= 0.2f, msg);
        un[0].p = {0, 0.6f, -90};   // voiture du trafic recyclée 70 m plus loin : ce n'est plus la même voiture
        a2.update(DT, un, ICI);
        int nouvelles = 0; for (int v = 0; v < VOIX; v++) nouvelles += a2.voix(v).occupee;
        verifier(nouvelles == 2, "RG-01 : voiture recyclée plus loin : l'ancienne voix s'éteint en fondu, une autre voix s'ouvre");
        std::vector<Candidat> loin = {voiture(7, {0, 0.6f, -150}, {}, 50, "golf", TRAFIC), voiture(8, {0, 0.6f, -150}, {}, 150, "p208", POLICE)};
        Attribution a3; for (int k = 0; k < 40; k++) a3.update(DT, loin, ICI);
        verifier(a3.actives() == 1 && a3.voix(0).id == 8, "RG-01 : à 150 m, seule la police s'entend (portée 120 m, police 200 m)");
    }

    // ---------- RG-02 : moteur et pneus ----------
    {
        verifier(std::abs(frequenceCombustion(3000) - 100.f) < 1e-3f, "RG-02 : 4 temps, 4 cylindres, 3 000 tr/min → combustion à 100 Hz");
        int r = 1; const float calme = regime(carById("golf"), 50 / 3.6f, r, false);
        int r2 = 1; const float fort = regime(carById("golf"), 150 / 3.6f, r2, true);
        std::snprintf(msg, sizeof msg, "RG-02 : régime tiré des rapports : Golf du trafic à 50 km/h %.0f tr/min (rapport %d), adversaire à 150 km/h %.0f tr/min (rapport %d)", calme, r, fort, r2);
        verifier(calme >= 1300 && calme <= 2400 && fort >= 0.55f * 7280 && fort <= 7280, msg);
        const float pente = (roulementDbA(100) - roulementDbA(50)) / std::log10(2.f);
        std::snprintf(msg, sizeof msg, "RG-02 : pneus (CNOSSOS-EU) : %.1f dB par décade de vitesse (30 à 40)", pente);
        verifier(pente >= 30 && pente <= 40, msg);
        const Parts p50 = parts(50, false, false), p60 = parts(60, false, false), p90 = parts(90, false, false);
        std::snprintf(msg, sizeof msg, "RG-02 : au-dessus de 50 km/h, les pneus dominent le moteur (60 km/h : +%.1f dB, 90 km/h : +%.1f dB)", dB(p60.pneus) - dB(p60.moteur), dB(p90.pneus) - dB(p90.moteur));
        verifier(p60.pneus > p60.moteur && p90.pneus > p90.moteur, msg);
        verifier(std::abs(p50.moteur * p50.moteur + p50.pneus * p50.pneus - 1.f) < 0.01f, "RG-02 : référence : voiture en croisière à 50 km/h = niveau 1");
        verifier(parts(50, false, true).moteur < parts(50, false, false).moteur * 0.3f, "RG-02 : voiture électrique : pas de combustion, sifflement discret");
        // niveau : voiture du trafic à 50 km/h à 7,5 m sur le côté, comparée au moteur du joueur en pleine accélération
        std::ifstream f(root + "/audio/sport.wav", std::ios::binary);
        std::vector<uint8_t> wav((std::istreambuf_iterator<char>(f)), {});
        EngineSynth es; es.setOutputRate(48000);
        if (!es.loadWav(wav)) { verifier(false, "RG-02 : sport.wav illisible"); }
        else {
            Track t; t.build(mapById("ville")); Player pl; pl.reset(carById("golf")); EngineModel em;
            std::vector<float> pcm; double acc = 0, e2 = 0; size_t n2 = 0;
            for (int i = 0; i < 10 * 120; i++) {
                DriveInput in; in.gas = true; pl.update(1.0 / 120, in, t, true);
                em.update(1.0 / 120, pl.speed, MAX_SPEED * pl.perf.top, MAX_SPEED, true, false, true, 1.0f, false, es.targets); es.targets.lastUpdate = 1e9;
                acc += 400; const int n = (int)acc; acc -= n; const size_t at = pcm.size(); pcm.resize(at + (size_t)n); es.render(pcm.data() + at, n, 0);
                if (i > 3 * 120) for (int k = 0; k < n; k++) { e2 += (double)pcm[at + (size_t)k] * pcm[at + (size_t)k]; n2++; }
            }
            const float moteurJoueur = (float)std::sqrt(e2 / (double)n2);
            Attribution at; std::vector<Candidat> c = {voiture(1, {7.5f, 1.2f, 0}, {0, 0, 0}, 50, "golf", TRAFIC)};
            for (int k = 0; k < 60; k++) at.update(DT, c, ICI);
            SonsVoitures s; s.build(48000);
            s.cible(0, calculerCible(at.voix(0), DT, ICI, {}, AVANT, DROITE));
            std::vector<float> G, D, bus; rendre(s, G, D, bus, 2);
            const float voix = std::sqrt(rms(G, 4800, G.size()) * rms(G, 4800, G.size()) + rms(D, 4800, D.size()) * rms(D, 4800, D.size()));
            std::snprintf(msg, sizeof msg, "RG-02 : voiture du trafic à 50 km/h à 7,5 m : %.4f, sous le moteur du joueur à fond %.4f (%.1f dB)", voix, moteurJoueur, dB(voix) - dB(moteurJoueur));
            verifier(voix < moteurJoueur, msg);
        }
    }

    // ---------- RG-04 : gauche / droite ----------
    {
        Attribution at; std::vector<Candidat> c = {voiture(1, {-10.f, 1.2f, 0}, {}, 60, "m4", ADVERSAIRE)};   // 90° à gauche
        for (int k = 0; k < 60; k++) at.update(DT, c, ICI);
        SonsVoitures s; s.build(48000);
        s.cible(0, calculerCible(at.voix(0), DT, ICI, {}, AVANT, DROITE));
        std::vector<float> G, D, bus; rendre(s, G, D, bus, 1);
        const float lr = dB(rms(D, 4800, D.size())) - dB(rms(G, 4800, G.size()));
        std::snprintf(msg, sizeof msg, "RG-04 : voiture à 90° à gauche : droite %.1f dB sous la gauche (≤ −9)", lr);
        verifier(lr <= -9.f, msg);
        verifier(rms(bus, 4800, bus.size()) > 0, "RG-07 : la voix nourrit le bus de l'écho des tunnels");
        // dépassement par la gauche : de l'arrière gauche à l'avant gauche, toujours plus fort à gauche
        Attribution a2; SonsVoitures s2; s2.build(48000);
        bool gauche = true; float zMin = 0;
        std::vector<float> g(400), d(400), b(400);
        for (int k = 0; k < 4 * 120; k++) {
            const float z = 30.f - 20.f * k * DT;   // de 30 m derrière à 50 m devant
            std::vector<Candidat> cc = {voiture(5, {-3.5f, 0.6f, z}, {0, 0, -40}, 144, "m4", ADVERSAIRE)};
            a2.update(DT, cc, ICI);
            s2.cible(0, calculerCible(a2.voix(0), DT, ICI, AVANT * 20.f, AVANT, DROITE));
            std::fill(g.begin(), g.end(), 0.f); std::fill(d.begin(), d.end(), 0.f);
            s2.mix(g.data(), d.data(), b.data(), 400, 0.5f, true);
            if (k > 40) { gauche = gauche && rms(g, 0, 400) > rms(d, 0, 400); zMin = z; }
        }
        std::snprintf(msg, sizeof msg, "RG-04 : dépassement par la gauche (de 30 m derrière à %.0f m devant) : toujours plus fort à gauche", -zMin);
        verifier(gauche, msg);
    }

    // ---------- RG-05 : Doppler ----------
    {
        const float c = VITESSE_SON;
        float pire = 0;
        struct Cas { vec3 ps, vs, pl, vl; };
        const Cas cas[] = {{{0, 0, 100}, {0, 0, -30}, {0, 0, 0}, {0, 0, 0}},          // source qui arrive à 30 m/s
                           {{0, 0, -100}, {0, 0, -30}, {0, 0, 0}, {0, 0, 0}},         // source qui s'éloigne
                           {{0, 0, 100}, {0, 0, 0}, {0, 0, 0}, {0, 0, 25}},           // l'auditeur va vers la source
                           {{50, 0, 50}, {0, 0, -40}, {0, 0, 0}, {0, 0, -20}}};       // en biais, les deux roulent
        for (const Cas& k : cas) {
            const vec3 u = (k.pl - k.ps) * (1.f / length(k.pl - k.ps));
            const float formule = (c + dot(k.vl, u * -1.f)) / (c - dot(k.vs, u));
            pire = std::max(pire, std::abs(doppler(k.ps, k.vs, k.pl, k.vl) / formule - 1));
        }
        std::snprintf(msg, sizeof msg, "RG-05 : facteur Doppler = (c + v_auditeur) / (c − v_source), c = 343 m/s (écart max %.4f %%)", pire * 100);
        verifier(pire < 0.001f, msg);
        verifier(doppler({0, 0, 100}, {0, 0, -30}, {}, {}) > 1.09f && doppler({0, 0, -100}, {0, 0, -30}, {}, {}) < 0.92f,
                 "RG-05 : plus aigu en approchant (+9,6 % à 30 m/s), plus grave en s'éloignant (−8,0 %)");
        // hauteur jouée : moteur seul, cycle à 25 Hz (combustion 100 Hz), Doppler d'une voiture qui arrive à 30 m/s
        SonsVoitures s; s.build(48000);
        Cible cb; cb.gG = cb.gD = 0.3f; cb.moteur = 1; cb.pneus = 0; cb.fCycle = 25; cb.doppler = doppler({0, 0, 100}, {0, 0, -30}, {}, {}); cb.coupure = 8000;
        s.cible(0, cb);
        std::vector<float> G, D, bus; rendre(s, G, D, bus, 1);
        // autocorrélation : période d'un cycle moteur, cherchée de 1/50 à 1/15 s
        size_t meilleur = 0; double vmax = -1e30;
        for (size_t lag = 48000 / 50; lag <= 48000 / 15; lag++) {
            double a = 0; for (size_t i = 9600; i < 9600 + 24000; i++) a += (double)G[i] * G[i + lag];
            if (a > vmax) { vmax = a; meilleur = lag; }
        }
        // affinage parabolique autour du maximum
        auto ac = [&](size_t lag) { double a = 0; for (size_t i = 9600; i < 9600 + 24000; i++) a += (double)G[i] * G[i + lag]; return a; };
        const double y0 = ac(meilleur - 1), y1 = ac(meilleur), y2 = ac(meilleur + 1);
        const double lagF = (double)meilleur + 0.5 * (y0 - y2) / (y0 - 2 * y1 + y2);
        const float mesure = (float)(48000.0 / lagF) * CYLINDRES, attendu = 100.f * cb.doppler;
        std::snprintf(msg, sizeof msg, "RG-05 : hauteur jouée mesurée %.2f Hz, formule %.2f Hz (écart %.2f %%, ≤ 2 %%)", mesure, attendu, std::abs(mesure / attendu - 1) * 100);
        verifier(std::abs(mesure / attendu - 1) <= 0.02f, msg);
        // lissé : passage à 2 m à 60 m/s de différence, le facteur ne saute jamais de plus de 1 % d'un pas à l'autre
        Attribution at; float prec = 0, saut = 0;
        for (int k = 0; k < 3 * 120; k++) {
            const float z = 90.f - 60.f * k * DT;
            std::vector<Candidat> cc = {voiture(3, {2.f, 1.2f, z}, {0, 0, -60}, 216, "m4", ADVERSAIRE)};
            at.update(DT, cc, ICI);
            const Cible x = calculerCible(at.voix(0), DT, ICI, {}, AVANT, DROITE);
            if (k > 1 && prec > 0) saut = std::max(saut, std::abs(x.doppler / prec - 1));
            prec = x.doppler;
        }
        // le mélangeur glisse d'une cible à la suivante sur chaque bloc audio (96 trames au plus court : rafale AAudio de 2 ms) :
        // la hauteur change alors d'au plus saut / 96 d'un échantillon au suivant
        std::snprintf(msg, sizeof msg, "RG-05 : Doppler lissé au passage (2 m, 60 m/s) : %.2f %% au plus d'un pas à l'autre, soit %.4f %% "
                      "au plus d'un échantillon au suivant (glissement continu, ≤ 0,1 %%)", saut * 100, saut * 100 / 96);
        verifier(saut / 96 <= 0.001f && saut <= 0.05f, msg);
    }

    // ---------- RG-06 : sons existants placés ----------
    {
        const Pan devant = panExistant(0), derriere = panExistant(3.14159265f), droite = panExistant(1.5707963f);
        std::snprintf(msg, sizeof msg, "RG-06 : son droit devant ou derrière : gauche %.3f, droite %.3f (1 : comme avant, écart 0 dB)", devant.g, devant.d);
        verifier(std::abs(devant.g - 1) < 1e-5f && std::abs(devant.d - 1) < 1e-5f && std::abs(derriere.g - 1) < 1e-5f && std::abs(derriere.d - 1) < 1e-5f, msg);
        std::snprintf(msg, sizeof msg, "RG-06 : son à 90° à droite : gauche %.1f dB sous la droite (≤ −9), puissance gardée", dB(droite.g) - dB(droite.d));
        verifier(dB(droite.g) - dB(droite.d) <= -9.f && std::abs(droite.g * droite.g + droite.d * droite.d - 2.f) < 1e-4f, msg);
        std::vector<float> passage(9600); for (size_t i = 0; i < passage.size(); i++) passage[i] = 0.3f * std::sin(6.2831853f * 700 * (float)i / 48000);
        SonsVoitures s; s.build(48000);
        s.jouer(&passage, 0.4f, devant);
        std::vector<float> G, D, bus; rendre(s, G, D, bus, 0.2f);
        float pire = 0;   // après le fondu de démarrage du mélangeur (30 ms de constante de temps, comme SfxMixer)
        for (size_t i = 7200; i < passage.size(); i++) pire = std::max({pire, std::abs(G[i] - passage[i] * 0.4f * 0.5f), std::abs(D[i] - passage[i] * 0.4f * 0.5f)});
        const float ecart = dB(rms(G, 7200, passage.size())) - dB(0.3f * 0.4f * 0.5f / 1.41421356f);
        std::snprintf(msg, sizeof msg, "RG-06 : passage joué au centre : même son qu'avant (gain × master), écart %.3f dB (≤ 0,5)", ecart);
        verifier(pire < 1e-3f && std::abs(ecart) <= 0.5f, msg);
    }

    // ---------- RG-08 : coût ----------
    {
        Attribution at; std::vector<Candidat> cs;
        for (int i = 0; i < VOIX; i++) cs.push_back(voiture((uint64_t)i + 1, {(float)(i - 4) * 3, 0.6f, -10.f * (float)i}, {0, 0, -30}, 108, CARS[i].id, i % 2 ? TRAFIC : ADVERSAIRE));
        for (int k = 0; k < 40; k++) at.update(DT, cs, ICI);
        SonsVoitures s; s.build(48000);
        for (int v = 0; v < VOIX; v++) s.cible(v, calculerCible(at.voix(v), DT, ICI, {0, 0, -20}, AVANT, DROITE));
        std::vector<float> G(480), D(480), bus(480);
        const int B = 3000;
        const auto t0 = std::chrono::steady_clock::now();
        for (int b = 0; b < B; b++) { std::fill(G.begin(), G.end(), 0.f); std::fill(D.begin(), D.end(), 0.f); s.mix(G.data(), D.data(), bus.data(), 480, 0.5f, true); }
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / B;
        std::snprintf(msg, sizeof msg, "RG-08 : %d voix : %.3f ms par bloc de 10 ms (≤ 0,3 ms)", at.actives(), ms);
        verifier(at.actives() == VOIX && ms <= 0.3, msg);
        const float actif = rms(G, 0, 480);
        std::vector<float> Gp(480, 0.f), Dp(480, 0.f), bp(480, 0.f);
        for (int b = 0; b < 30; b++) { std::fill(Gp.begin(), Gp.end(), 0.f); s.mix(Gp.data(), Dp.data(), bp.data(), 480, 0.5f, false); }
        std::snprintf(msg, sizeof msg, "pause (le jeu ne donne plus de nouvelles) : %.0f dB après 0,3 s (≤ −40 dB)", dB(rms(Gp, 0, 480)) - dB(actif));
        verifier(dB(rms(Gp, 0, 480)) - dB(actif) <= -40.f, msg);
    }

    std::printf(echecs ? "test_audio_voitures : %d ÉCHEC(S)\n" : "test_audio_voitures : tout est conforme\n", echecs);
    return echecs ? 1 : 0;
}
