// Vérifie le son du moteur (WFS-10) hors ligne : la Golf accélère à fond 14 s puis lève le pied.
// Critères : le régime monte puis chute à chaque passage (rapports 1 → 5), le son n'est jamais vide ni saturé,
// pas de craquement (pas de saut brutal d'un échantillon au suivant), silence quand le jeu ne donne plus de nouvelles.
// usage : test_audio <dossier assets> [sortie.wav]
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>
#include "../src/audio/engine_synth.h"
#include "../src/audio/sfx_synth.h"   // WFS-28
#include "../src/audio/sfx_course.h"  // WFS-124
#include "../src/audio/trafic_sons.h"  // WFS-121
#include "../src/audio/garage_son.h"  // WFS-127
#include "../src/audio/route_sons.h"  // WFS-174
#include "../src/audio/sirene_son.h"  // WFS-131
#include <cstring>
#include "../src/game/player.h"
using namespace wfs;

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : "assets/";
    std::ifstream f(root + "/audio/sport.wav", std::ios::binary);
    std::vector<uint8_t> wav((std::istreambuf_iterator<char>(f)), {});
    EngineSynth s; s.setOutputRate(48000);
    if (!s.loadWav(wav)) { std::printf("ÉCHEC : sport.wav illisible\n"); return 1; }
    Track t; t.build(mapById("ville")); Player pl; pl.reset(carById("golf"));
    EngineModel m;
    std::vector<float> pcm; double acc = 0;
    int gearsSeen = 0, shifts = 0, lastGear = 1; float rpmMax = 0, jump = 0, peak = 0; double rms = 0;
    for (int i = 0; i < 20 * 120; i++) {
        const double dt = 1.0 / 120, tt = i * dt;
        DriveInput in; in.gas = tt < 14;
        pl.update(dt, in, t, true);
        m.update(dt, pl.speed, MAX_SPEED * pl.perf.top, MAX_SPEED, in.gas, false, true, 1.0f, false, s.targets);
        s.targets.lastUpdate = 1e9;
        acc += dt * 48000; const int n = (int)acc; acc -= n;
        const size_t at = pcm.size(); pcm.resize(at + n); s.render(pcm.data() + at, n, 0);
        if (m.gear > lastGear) shifts++;
        lastGear = m.gear; gearsSeen = std::max(gearsSeen, m.gear); rpmMax = std::max(rpmMax, m.rpm);
    }
    for (size_t i = 1; i < pcm.size(); i++) { jump = std::max(jump, std::abs(pcm[i] - pcm[i - 1])); peak = std::max(peak, std::abs(pcm[i])); rms += pcm[i] * pcm[i]; }
    rms = std::sqrt(rms / pcm.size());
    // pause : plus de nouvelles du jeu depuis 1 s → le son doit s'éteindre
    s.targets.lastUpdate = 0; std::vector<float> quiet(9600); s.render(quiet.data(), 9600, 1.0);
    float tail = 0; for (int i = 4800; i < 9600; i++) tail = std::max(tail, std::abs(quiet[i]));
    if (argc > 2) {   // écoute : WAV 16 bits
        std::ofstream o(argv[2], std::ios::binary); auto u32 = [&](uint32_t x) { o.write((char*)&x, 4); }; auto u16 = [&](uint16_t x) { o.write((char*)&x, 2); };
        o.write("RIFF", 4); u32(36 + pcm.size() * 2); o.write("WAVEfmt ", 8); u32(16); u16(1); u16(1); u32(48000); u32(96000); u16(2); u16(16); o.write("data", 4); u32(pcm.size() * 2);
        for (float x : pcm) { int16_t v = (int16_t)std::lround(x * 32767); o.write((char*)&v, 2); }
    }
    const bool ok = gearsSeen == 5 && shifts >= 4 && rpmMax > 0.85f && peak < 0.99f && rms > 0.01 && jump < 0.25f && tail < 0.005f;   // après 0,1 s de pause : < 0,005 (environ −40 dB sous la crête), inaudible
    std::printf("son : %zu échantillons, rapports vus %d (%d passages), régime max %.2f, crête %.2f, RMS %.3f, plus grand saut %.3f, après pause %.5f → %s\n",
                pcm.size(), gearsSeen, shifts, rpmMax, peak, rms, jump, tail, ok ? "OK" : "ÉCHEC");
    // WFS-28 : bruitages (noiseHit / beep du jeu web) — pas de craquement : départ et fin à zéro (fondu de 5 ms),
    // raccord des boucles sans saut plus grand que dans le son lui-même, aucun son vide ni saturé
    bool okSfx = true;
    auto verifie = [&](const char* nom, const std::vector<float>& b, bool boucle) {
        float pk = 0, sautMax = 0; for (size_t i = 1; i < b.size(); i++) { pk = std::max(pk, std::abs(b[i])); sautMax = std::max(sautMax, std::abs(b[i] - b[i - 1])); }
        const float bout = std::max(std::abs(b.front()), std::abs(b.back())), raccord = std::abs(b.front() - b.back());
        const bool o = !b.empty() && pk > 0.02f && pk < 1.5f && (boucle ? raccord <= sautMax * 1.01f : bout < 0.002f);
        std::printf("bruitage %-14s : %5.2f s, crête %.2f, %s %.4f → %s\n", nom, b.size() / 48000.0, pk, boucle ? "raccord" : "bouts", boucle ? raccord : bout, o ? "OK" : "ÉCHEC");
        okSfx = okSfx && o;
    };
    {
        using namespace sfx; const float R = 48000;
        std::vector<float> acc; noiseHit(acc, 0, R, 0.6f, 140, 0.7f, 1, 45); noiseHit(acc, 0, R, 0.4f, 2600, 1.4f, 0.55f, 7000); noiseHit(acc, 0, R, 0.25f, 900, 2, 0.4f, 300);
        verifie("accident", acc, false);
        std::vector<float> fr; noiseHit(fr, 0, R, 0.22f, 3200, 3, 0.35f, 1600); verifie("frottement", fr, false);
        std::vector<float> qc; noiseHit(qc, 0, R, 0.28f, 900, 1.2f, 0.5f, 2600); verifie("quasi-contact", qc, false);
        std::vector<float> fin; beep(fin, R, 660, 0, 0.2f); beep(fin, R, 440, 0.2f, 0.2f); beep(fin, R, 330, 0.4f, 0.4f); verifie("bip-fin", fin, false);
        verifie("crissement", boucle(R, 1.0f, 2100, 7, 0.9f, 3400, 9), true);
        verifie("nitro", boucle(R, 1.2f, 650, 0.7f, 0.6f), true);
        std::vector<float> pe; noiseHit(pe, 0, R, 0.05f, 380, 1.2f, 0.55f, 140); verifie("petarade", pe, false);   // WFS-120
        std::vector<float> tu; noiseHit(tu, 0, R, 0.22f, 2800, 0.8f, 0.25f, 1200); verifie("turbo", tu, false);
    }
    // WFS-120 : pétarades (RG-01), turbo (RG-02), rupteur (RG-03), voiture électrique (RG-04), hors course (RG-06)
    bool okEff = true;
    auto effet = [&](const char* quoi, bool c) { std::printf("effets moteur : %-52s → %s\n", quoi, c ? "OK" : "ÉCHEC"); okEff = okEff && c; };
    auto salve = [&](float rpm, float kmh, bool enCourse, bool elec, float v = 0.4f) {   // plein gaz 1 s puis lever de pied 0,6 s
        EffetsMoteur e; int n = 0;
        for (int i = 0; i < 60; i++) e.update(1.0 / 60, rpm, 3, v, true, kmh, enCourse, elec);
        for (int i = 0; i < 36; i++) { e.update(1.0 / 60, rpm, 3, v, false, kmh, enCourse, elec); n += e.claquements; }
        return n;
    };
    const int n7 = salve(0.7f, 120, true, false);
    effet("lever de pied à régime 0,70 : 2 à 4 claquements", n7 >= 2 && n7 <= 4);
    effet("lever de pied à régime 0,40 : aucun", salve(0.4f, 120, true, false) == 0);
    effet("lever de pied sous 30 km/h : aucun", salve(0.7f, 20, true, false) == 0);
    const int n5 = salve(0.42f, 210, true, false, 0.8f);
    effet("lever de pied en 5e à 210 km/h (régime 0,42) : 2 à 4", n5 >= 2 && n5 <= 4);
    effet("voiture électrique : aucun", salve(0.7f, 120, true, true) == 0);
    effet("hors course : aucun", salve(0.7f, 120, false, false) == 0);
    {   // deux levers de pied à 0,5 s d'écart : une seule salve (pause de 1,5 s)
        EffetsMoteur e; int n = 0;
        for (int k = 0; k < 2; k++) {
            for (int i = 0; i < 15; i++) e.update(1.0 / 60, 0.7f, 4, 0.6f, true, 120, true, false);
            for (int i = 0; i < 15; i++) { e.update(1.0 / 60, 0.7f, 4, 0.6f, false, 120, true, false); n += e.claquements; }
        }
        effet("deux levers de pied en 0,5 s : une seule salve (2 à 4)", n >= 2 && n <= 4);
    }
    {   // turbo : 2 → 3 en charge ; pas au rétrogradage ni pied levé ; pas deux fois en 0,4 s
        EffetsMoteur e; int t = 0;
        for (int i = 0; i < 60; i++) e.update(1.0 / 60, 0.9f, 2, 0.3f, true, 80, true, false);   // déjà en 2e depuis 1 s
        e.update(1.0 / 60, 0.5f, 3, 0.35f, true, 80, true, false); t += e.turbo;
        effet("passage 2 → 3 en charge : un souffle", t == 1);
        e.update(1.0 / 60, 0.9f, 4, 0.55f, true, 120, true, false);
        effet("passage 3 → 4 trop tôt (< 0,4 s) : rien", !e.turbo);
        EffetsMoteur f; f.update(1.0 / 60, 0.9f, 3, 0.4f, false, 90, true, false); f.update(1.0 / 60, 0.5f, 4, 0.55f, false, 90, true, false);
        effet("passage pied levé : rien", !f.turbo);
        EffetsMoteur h; h.update(1.0 / 60, 0.9f, 2, 0.3f, true, 80, true, true); h.update(1.0 / 60, 0.5f, 3, 0.35f, true, 80, true, true);
        effet("voiture électrique : pas de turbo", !h.turbo);
    }
    {   // rupteur : 5e, plein gaz à la vitesse de pointe ; coupures un pas de 70 ms sur deux ; fin au lever de pied
        EffetsMoteur e; int debut = 0, coupes = 0, pas = 0;
        for (int i = 0; i < 120; i++) { e.update(1.0 / 120, 0.63f, 5, 0.99f, true, 250, true, false); debut += e.rupteurDebut; if (e.rupteur) { pas++; coupes += e.couper; } }
        effet("5e à fond à la vitesse de pointe : un épisode de rupteur", debut == 1 && e.rupteur);
        effet("coupures environ un pas sur deux", pas > 0 && coupes > pas / 3 && coupes < pas * 2 / 3);
        e.update(1.0 / 120, 0.6f, 5, 0.99f, false, 250, true, false);
        effet("lever de pied : rupteur fini", !e.rupteur && !e.couper);
        EffetsMoteur f; for (int i = 0; i < 120; i++) f.update(1.0 / 120, 0.6f, 5, 0.85f, true, 220, true, false);
        effet("5e à fond sous la vitesse de pointe : pas de rupteur", !f.rupteur);
    }
    // WFS-121 : sons du trafic — passages (RG-02), klaxon (RG-03), rumeur (RG-04), trafic nul (RG-01), tampons (RG-05)
    bool okTrafic = true;
    auto trafic = [&](const char* quoi, bool c) { std::printf("sons du trafic : %-56s → %s\n", quoi, c ? "OK" : "ÉCHEC"); okTrafic = okTrafic && c; };
    {
        const double dt = 1.0 / 120;
        // une voiture de +5 m à −5 m, vitesse relative v, décalée de lat ; renvoie le nombre de passages et le dernier gain
        auto passe = [&](double v, double lat, float* gain = nullptr) {
            TraficSons ts; std::vector<VoitureSon> cars(1); int n = 0;
            cars[0].lat = lat; cars[0].vRelKmh = v; cars[0].longueurM = 4.5;
            for (double t = 0, dz = 5; dz > -5; t += dt, dz -= v / 3.6 * dt) {
                cars[0].dzM = dz;
                const SonsDuPas s = ts.update(dt, true, cars, 150, 4.3, 0);
                n += s.passages; if (s.passages && gain) *gain = s.gainPassage;
            }
            return n;
        };
        float g80 = 0, g160 = 0, gLat = 0;
        trafic("voiture de +5 m à −5 m à 80 km/h relatifs : 1 passage", passe(80, 0, &g80) == 1);
        trafic("volume à 80 km/h relatifs : 0,29 (0,20 à 40, 0,45 à 150)", std::abs(g80 - 0.2909f) < 0.002f);
        passe(160, 0, &g160); passe(160, 0.8 * 0.999, &gLat);
        trafic("volume à 160 km/h : 0,45 ; à 0,8 demi-largeur : moitié", std::abs(g160 - 0.45f) < 1e-4f && std::abs(gLat - 0.225f) < 0.002f);
        trafic("vitesse relative 20 km/h : aucun passage", passe(20, 0) == 0);
        trafic("voiture à 0,9 demi-largeur de côté : aucun passage", passe(80, 0.9) == 0);
        {   // deux voitures qui passent à 0,1 s d'écart : un seul souffle (intervalle de 0,25 s)
            TraficSons ts; std::vector<VoitureSon> cars(2); int n = 0;
            for (auto& c : cars) c.vRelKmh = 100;
            for (int i = 0; i < 120; i++) {
                const double t = i * dt;
                cars[0].dzM = 5 - 100 / 3.6 * t; cars[1].dzM = 5 - 100 / 3.6 * (t - 0.1);
                n += ts.update(dt, true, cars, 150, 4.3, 0).passages;
            }
            trafic("2 voitures dans les 0,25 s : 1 seul passage", n == 1);
        }
        {   // voiture téléportée (réapparition du trafic loin devant) : pas de passage
            TraficSons ts; std::vector<VoitureSon> cars(1); cars[0].vRelKmh = 100; int n = 0;
            cars[0].dzM = 300; n += ts.update(dt, true, cars, 150, 4.3, 0).passages;
            cars[0].dzM = -300; n += ts.update(dt, true, cars, 150, 4.3, 0).passages;
            trafic("changement de signe loin du joueur (> 25 m) : aucun passage", n == 0);
        }
        // collage : voiture 3 m devant pare-chocs à pare-chocs, même file, joueur à 100 km/h, pendant `duree` secondes
        auto colle = [&](double duree, double kmh = 100, double lat = 0) {
            TraficSons ts; std::vector<VoitureSon> cars(1); int n = 0;
            cars[0].dzM = 3 + (4.5 + 4.3) / 2; cars[0].lat = lat; cars[0].vRelKmh = 0;
            for (double t = 0; t < duree; t += dt) n += ts.update(dt, true, cars, kmh, 4.3, 0).klaxon;
            return n;
        };
        trafic("collage 1,6 s : 1 klaxon", colle(1.6) == 1);
        trafic("collage 1 s : aucun klaxon", colle(1.0) == 0);
        trafic("collage 5 s : 2 klaxons (au plus un toutes les 3 s)", colle(5.0) == 2);
        trafic("collage à 20 km/h : aucun klaxon", colle(2.0, 20) == 0);
        trafic("voiture dans la file voisine : aucun klaxon", colle(2.0, 100, 0.5) == 0);
        {   // quasi-contacts : klaxon 0,15 s après, une fois sur deux environ (tirage reproductible), au plus un toutes les 3 s
            TraficSons ts; std::vector<VoitureSon> none; int k = 0, q = 0; double premier = -1, quasiT = -1;
            for (int i = 0; i < 120 * 60; i++) {
                const double t = i * dt;
                if (i % (120 * 4) == 60) { q++; if (quasiT < 0) quasiT = t; }   // un quasi-contact toutes les 4 s
                const SonsDuPas s = ts.update(dt, true, none, 150, 4.3, q);
                if (s.klaxon) { k++; if (premier < 0) premier = t; }
            }
            trafic("15 quasi-contacts : 4 à 11 klaxons (une fois sur deux)", k >= 4 && k <= 11);
            TraficSons a, b; int ka = 0, kb = 0;
            for (int i = 0; i < 1200; i++) { const int qq = i / 60; ka += a.update(dt, true, none, 150, 4.3, qq).klaxon; kb += b.update(dt, true, none, 150, 4.3, qq).klaxon; }
            trafic("tirage reproductible (même suite à chaque partie)", ka == kb);
            (void)premier; (void)quasiT;
        }
        {   // rumeur : 0,04 par voiture à moins de 40 m, plafonnée à 0,25
            TraficSons ts; std::vector<VoitureSon> cars(10);
            for (size_t i = 0; i < cars.size(); i++) cars[i].dzM = i < 3 ? 10.0 + i : 200.0;
            const float r3 = ts.update(dt, true, cars, 150, 4.3, 0).rumeur;
            for (auto& c : cars) c.dzM = -20;
            const float r10 = ts.update(dt, true, cars, 150, 4.3, 0).rumeur;
            trafic("rumeur : 3 voitures à moins de 40 m → 0,12 ; 10 → 0,25", std::abs(r3 - 0.12f) < 1e-5f && std::abs(r10 - 0.25f) < 1e-5f);
            for (int i = 0; i < 130; i++) ts.update(dt, true, cars, 150, 4.3, 0);   // plus d'une seconde : rumeur 0,25 écrite au journal
            // RG-01 / RG-06 : trafic nul ou hors course : aucun son, rumeur 0
            int n = 0; float r = 1; bool log0 = false;
            for (int i = 0; i < 240; i++) {
                for (auto& c : cars) { c.dzM = 5 - i * 0.1; c.vRelKmh = 100; }
                const SonsDuPas s = ts.update(dt, false, cars, 150, 4.3, i);
                n += s.passages + s.klaxon; r = s.rumeur; log0 = log0 || (s.journalRumeur && s.rumeur == 0);
            }
            trafic("trafic nul / hors course : aucun son, rumeur 0 (écrite au journal)", n == 0 && r == 0 && log0);
        }
        const float R = 48000;
        auto borne = [](const std::vector<float>& b) { float p = 0; for (float x : b) p = std::max(p, std::abs(x)); return p; };
        const std::vector<float> pa = sonPassage(R), kl = sonKlaxon(R), ru = boucleRumeur(R);
        verifie("passage", pa, false); verifie("klaxon", kl, false); verifie("rumeur", ru, true);
        trafic("tampons Passage, Klaxon, rumeur non vides, |x| ≤ 1", !pa.empty() && !kl.empty() && !ru.empty() && borne(pa) <= 1 && borne(kl) <= 1 && borne(ru) <= 1);
        {   // Doppler : la fréquence descend — rapport d'énergie 900 Hz / 350 Hz (Goertzel) plus grand au début qu'à la fin
            auto energie = [&](size_t a, size_t e, float f) {
                const double w = 2 * 3.14159265358979 * f / R, c = 2 * std::cos(w); double s1 = 0, s2 = 0;
                for (size_t i = a; i < e; i++) { const double s0 = pa[i] + c * s1 - s2; s2 = s1; s1 = s0; }
                return s1 * s1 + s2 * s2 - c * s1 * s2;
            };
            const size_t n = pa.size();
            const double debut = energie(n / 10, n * 4 / 10, 900) / energie(n / 10, n * 4 / 10, 350);
            const double fin = energie(n * 6 / 10, n, 900) / energie(n * 6 / 10, n, 350);
            std::printf("passage : énergie 900 Hz / 350 Hz : %.2f au début, %.2f à la fin\n", debut, fin);
            trafic("passage : fréquence qui descend (effet Doppler)", debut > fin * 2);
        }
    }
    // WFS-124 : tic du chrono (RG-01 / RG-02), combo (RG-03), combo perdu (RG-04), tampons (RG-05)
    bool okCourse = true;
    {
        using namespace sfxcourse;
        auto cas = [&](const char* quoi, bool c) { std::printf("sons de course : %-56s → %s\n", quoi, c ? "OK" : "ÉCHEC"); okCourse = okCourse && c; };
        cas("chrono : actif en Checkpoint en jeu", chronoActif(true, 0, 30, true));
        cas("chrono : inactif en duel / course à 5 / poursuite", !chronoActif(true, 2000, 30, true));
        cas("chrono : inactif sans règles (1e9) et hors écran Course", !chronoActif(true, 0, 1e9, true) && !chronoActif(true, 0, 5, false));
        cas("chrono : inactif hors jeu (décompte, fin)", !chronoActif(false, 0, 5, true));
        cas("affiché : 6,0 → 6 ; 5,4 → 6 ; 0,2 → 1 ; −1 → 0", chronoAffiche(6.0) == 6 && chronoAffiche(5.4) == 6 && chronoAffiche(0.2) == 1 && chronoAffiche(-1) == 0);
        int tics = 0, urgents = 0, a = chronoAffiche(10);
        for (double t = 10; t > 0; t -= 1.0 / 60) { const Tic x = ticChrono(a, chronoAffiche(t)); tics += x == TIC; urgents += x == URGENT; a = chronoAffiche(t); }
        cas("10 s → 0 : 5 tics (8 à 4) et 3 urgents (3, 2, 1)", tics == 5 && urgents == 3);
        cas("saut de 60 à 6 : un seul tic", ticChrono(60, 6) == TIC);
        cas("remontée (checkpoint 5 → 35) : aucun tic", ticChrono(5, 35) == AUCUN);
        cas("au-dessus de 8 s (12 → 11) : aucun tic", ticChrono(12, 11) == AUCUN);
        cas("combo 1 → 2 : son 2 ; 4 → 5 : son 5 ; 7 → 8 : son 6", sonCombo(1, 2) == 2 && sonCombo(4, 5) == 5 && sonCombo(7, 8) == 6);
        cas("combo 0 → 1 et combo qui baisse : aucun son", sonCombo(0, 1) == 0 && sonCombo(3, 0) == 0);
        cas("fréquences du combo : 660 Hz puis un ton de plus", std::abs(frequenceCombo(2) - 660) < 0.5f && std::abs(frequenceCombo(3) - 740.8f) < 1 && frequenceCombo(6) > frequenceCombo(5));
        cas("combo 3 → 0 sur un choc : combo perdu", comboPerdu(3, 0, 4, 5));
        cas("combo 3 → 0 sans choc (fin après 3 s) : rien", !comboPerdu(3, 0, 4, 4));
        cas("combo 1 → 0 sur un choc : rien", !comboPerdu(1, 0, 4, 5));
        using namespace sfx; const float R = 48000;
        std::vector<float> tic; beep(tic, R, 1000, 0, 0.06f); verifie("chrono-tic", tic, false);
        std::vector<float> urg; beep(urg, R, 1500, 0, 0.09f); verifie("chrono-urgent", urg, false);
        for (int n = 2; n <= COMBO_MAX_SON; n++) { std::vector<float> b; beep(b, R, frequenceCombo(n), 0, 0.08f); verifie("combo", b, false); }
        std::vector<float> pe; beep(pe, R, 440, 0, 0.1f); beep(pe, R, 330, 0.1f, 0.1f); verifie("combo-perdu", pe, false);
    }
    // WFS-127 : showroom sonore (RG-04, RG-09)
    bool okGarage = true;
    {
        using namespace garageson;
        auto cas = [&](const char* quoi, bool c) { std::printf("showroom : %-60s → %s\n", quoi, c ? "OK" : "ÉCHEC"); okGarage = okGarage && c; };
        cas("régime : 0,12 à 0 s, 0,65 à 0,30 s, 0,12 à 1,20 s",
            std::abs(regimeGarage(0) - 0.12f) < 1e-4f && std::abs(regimeGarage(0.30f) - 0.65f) < 1e-3f && std::abs(regimeGarage(1.20f) - 0.12f) < 1e-3f);
        bool monte = true, descend = true;
        for (float t = 0.01f; t < 0.30f; t += 0.01f) monte = monte && regimeGarage(t) > regimeGarage(t - 0.01f);
        for (float t = 0.31f; t <= 1.20f; t += 0.01f) descend = descend && regimeGarage(t) <= regimeGarage(t - 0.01f) + 1e-6f;
        cas("régime : monte de 0 à 0,30 s, décroît de 0,30 à 1,20 s", monte && descend);
        cas("avant le coup de gaz et après : ralenti", regimeGarage(-0.2f) == RALENTI && regimeGarage(3) == RALENTI);
        cas("charge : pédale pendant la montée seulement", chargeGarage(0.1f) == 1 && chargeGarage(0.5f) == 0 && chargeGarage(-1) == 0);
        const Cibles lo = cibles(RALENTI, 0, 1), hi = cibles(POINTE, 1, 1);
        cas("cibles : le coup de gaz monte le son (vitesse de lecture, aigus)", hi.rate > lo.rate && hi.hiGain > lo.hiGain && lo.hiGain == 0);
        int connues = 0;
        for (int i = 0; i < CAR_COUNT; i++) connues += std::strcmp(engineVoiceFor(CARS[i].id).carId, CARS[i].id) == 0;
        std::printf("showroom : %d voitures sur %d ont leur voix\n", connues, CAR_COUNT);
        cas("les 20 voitures ont une voix connue", connues == CAR_COUNT);
        using namespace sfx; const float R = 48000;
        std::vector<float> d;   // même fabrication que SfxMixer::build (sfx.cpp)
        for (int i = 0; i < 6; i++) noiseHit(d, (size_t)(i * 0.065f * R), R, 0.06f, 190, 1.4f, 0.45f, 120);
        noiseHit(d, (size_t)(0.38f * R), R, 0.07f, 320, 1.0f, 0.7f, 140);
        float pk = 0; for (float x : d) pk = std::max(pk, std::abs(x));
        cas("démarreur : 0,45 s, |x| ≤ 1", std::abs(d.size() / R - 0.45f) < 0.005f && pk <= 1);
        verifie("demarreur", d, false);
    }
    // WFS-131 : sirène de la police (RG-01 à RG-06, RG-10)
    bool okSirene = true;
    {
        using namespace sirene;
        auto cas = [&](const char* quoi, bool c) { std::printf("sirène : %-62s → %s\n", quoi, c ? "OK" : "ÉCHEC"); okSirene = okSirene && c; };
        // RG-04 : Doppler
        std::printf("sirène : Doppler 0 → %.4f, +30 m/s → %.4f, −30 m/s → %.4f, ±200 m/s → %.4f / %.4f\n",
                    dopplerSirene(0), dopplerSirene(30), dopplerSirene(-30), dopplerSirene(200), dopplerSirene(-200));
        cas("Doppler : 1 à l'arrêt, > 1 qui approche, < 1 qui s'éloigne", dopplerSirene(0) == 1 && dopplerSirene(30) > 1 && dopplerSirene(-30) < 1);
        cas("Doppler : borné à 0,88 et 1,12 (±200 m/s, ≥ c, NaN → 1)",
            std::abs(dopplerSirene(200) - 1.12f) < 1e-6f && std::abs(dopplerSirene(-200) - 0.88f) < 1e-6f && dopplerSirene(400) == dopplerSirene(200)
            && dopplerSirene(NAN) == 1);
        cas("Doppler : c / (c − v) entre les bornes (+30 m/s : 343 / 313)", std::abs(dopplerSirene(30) - 343.f / 313.f) < 1e-5f);
        // RG-03 : volume selon la distance
        std::printf("sirène : gain 10 m → %.3f, 82,5 m → %.3f, 150 m → %.3f, 200 m → %.3f\n", gainSirene(10), gainSirene(82.5), gainSirene(150), gainSirene(200));
        cas("gain : 0,45 à 10 m, 0 à 200 m, ≈ 0,225 à 82,5 m (± 0,01)",
            std::abs(gainSirene(10) - 0.45f) < 1e-6f && gainSirene(200) == 0 && std::abs(gainSirene(82.5) - 0.225f) <= 0.01f);
        bool decroit = true; for (double d = 15; d < 150; d += 0.5) decroit = decroit && gainSirene(d + 0.5) <= gainSirene(d);
        cas("gain : continu et décroissant de 15 à 150 m, 0 sans police (1e9, NaN)", decroit && gainSirene(AUCUNE) == 0 && gainSirene(NAN) == 0);
        // RG-02 : hystérésis wail / yelp
        cas("yelp à 19 m, wail à 26 m", yelpSirene(false, 19) && !yelpSirene(true, 26));
        cas("22 m : yelp si on venait de 19 m, wail si on venait de 30 m", yelpSirene(yelpSirene(false, 19), 22) && !yelpSirene(yelpSirene(false, 30), 22));
        // RG-01 / RG-02 : boucles
        const float R = 48000;
        for (const float cycle : {CYCLE_WAIL, CYCLE_YELP}) {
            const std::vector<float> b = boucleSirene(R, cycle);
            float pk = 0, sautMax = 0; for (size_t i = 1; i < b.size(); i++) { pk = std::max(pk, std::abs(b[i])); sautMax = std::max(sautMax, std::abs(b[i] - b[i - 1])); }
            // raccord : le 1er échantillon prolonge la fin (extrapolation linéaire des 2 derniers), comme un échantillon ordinaire
            const size_t n = b.size();
            const float prolonge = 2 * b[n - 1] - b[n - 2], ecart = std::abs(b[0] - prolonge), saut = std::abs(b[0] - b[n - 1]);
            std::printf("sirène : boucle %.2f s : %zu échantillons, crête %.2f, raccord : écart au prolongement %.4f, saut %.3f (saut max %.3f)\n",
                        cycle, n, pk, ecart, saut, sautMax);
            cas(cycle > 1 ? "boucle wail : 3,2 s, |x| ≤ 1, raccord sans saut (écart < 0,05)" : "boucle yelp : 0,30 s, |x| ≤ 1, raccord sans saut (écart < 0,05)",
                std::abs(n / R - cycle) < 1e-3f && pk > 0.5f && pk <= 1 && ecart < 0.05f && saut <= sautMax * 1.01f);
            // fréquence : passages par zéro montants sur 50 ms au début (≈ 650 Hz) et au milieu du cycle (≈ 1 450 Hz)
            auto frequence = [&](size_t debut) {
                const size_t m = (size_t)(0.05f * R) * (cycle > 1 ? 1 : 0) + (size_t)(0.004f * R) * (cycle > 1 ? 0 : 1);
                int z = 0; for (size_t i = debut + 1; i < debut + m && i < n; i++) z += b[i - 1] < 0 && b[i] >= 0;
                return z / (m / R);
            };
            if (cycle > 1) {
                const float f0 = frequence(0), f1 = frequence(n / 2 - (size_t)(0.025f * R));
                std::printf("sirène : wail : %.0f Hz au début, %.0f Hz à mi-cycle\n", f0, f1);
                cas("wail : ≈ 650 Hz au début, ≈ 1 450 Hz à mi-cycle (± 5 %)", std::abs(f0 - 650) < 33 && std::abs(f1 - 1450) < 73);
            }
        }
        // RG-06 : jingles
        const std::vector<float> ja = jingleArrete(R), je = jingleEchappe(R);
        verifie("poursuite-arrete", ja, false); verifie("poursuite-echappe", je, false);
        cas("jingles : durées 0,50 s et 0,59 s (± 0,02)", std::abs(ja.size() / R - 0.50f) <= 0.02f && std::abs(je.size() / R - 0.59f) <= 0.02f);
        // RG-05 : extinction en fin de course : de 0,45 à 0 (seuil de silence du mixeur) en 0,8 s au plus, sans saut
        {
            const float pasFin = pasFinSirene(R), k = 1 - std::exp(-1 / (LISSAGE_S / 3 * R));
            float g = GAIN_PRES, pasMax = 0; int i = 0;
            for (; g > 1e-4f && i < (int)(2 * R); i++) { const float avant = g; g = suivreGainSirene(g, 0, true, k, pasFin); pasMax = std::max(pasMax, avant - g); }
            std::printf("sirène : extinction de fin : %.4f s (%d échantillons), plus grand pas %.7f\n", i / R, i, pasMax);
            cas("fin de course : 0,45 → 0 en ≤ 0,8 s, linéaire (pas ≤ 0,45 / 0,8 s)", i <= (int)(FIN_S * R) + 1 && pasMax <= GAIN_PRES / (FIN_S * R) * 1.03f);
            // RG-03 : lissage 0,3 s : 0 → 0,45 atteint 95 % en 0,3 s, aucun saut
            g = 0; for (int j = 0; j < (int)(LISSAGE_S * R); j++) g = suivreGainSirene(g, GAIN_PRES, false, k, pasFin);
            cas("montée lissée : 95 % du gain en 0,3 s", g >= 0.94f * GAIN_PRES && g <= GAIN_PRES);
        }
        // décisions par pas (RG-02, RG-03, RG-05, RG-09) : poursuite simulée à 120 pas par seconde
        {
            SireneSon s; const double dt = 1.0 / 120;
            CiblesSirene c = s.update(dt, false, AUCUNE, 0, true, false);
            cas("sans appel de la poursuite (autre mode, menus) : gain 0", c.gain == 0 && !c.fin);
            c = s.update(dt, true, 82.5, 30, true, false);
            cas("police à 82,5 m qui approche : gain 0,225, wail, Doppler > 1", std::abs(c.gain - 0.225f) < 0.01f && !c.yelp && c.doppler > 1);
            c = s.update(dt, true, 82.5, 30, false, false);
            cas("pas en jeu (décompte, son coupé) : gain 0", c.gain == 0);
            c = s.update(dt, true, 19, 0, true, false); const bool y1 = c.yelp;
            c = s.update(dt, true, 22, 0, true, false); const bool y2 = c.yelp;
            c = s.update(dt, true, 26, 0, true, false);
            cas("19 m → yelp, 22 m → yelp, 26 m → wail (pas après pas)", y1 && y2 && !c.yelp);
            for (int i = 0; i < 130; i++) s.update(dt, true, 10, 0, true, false);   // plus d'une seconde : le gain est écrit au journal
            c = s.update(dt, true, 10, -20, false, true);
            cas("course finie : extinction (fin, gain visé 0), mode gardé, journal du retour à 0", c.fin && c.gain == 0 && c.yelp && c.journal);
            int lignes = 0; for (int i = 0; i < 240; i++) lignes += s.update(dt, true, 150 - i * 0.5, 10, true, false).journal;
            cas("journal : une ligne par seconde au plus (2 s → ≤ 3 lignes)", lignes >= 1 && lignes <= 3);
        }
    }
    // WFS-174 : sons de la route (RG-01 à RG-08)
    bool okRoute = true;
    {
        using namespace route;
        auto cas = [&](const char* quoi, bool c) { std::printf("route : %-62s → %s\n", quoi, c ? "OK" : "ÉCHEC"); okRoute = okRoute && c; };
        auto en = [](float kmh, float x = 0, bool pluie = false, bool nitro = false, bool course = true) {
            Entree e; e.kmh = kmh; e.kmhMax = 250; e.playerX = x; e.edge = 1.6f; e.pluie = pluie; e.nitro = nitro; e.enCourse = course; return e;
        };
        cas("vent nul sous 40 km/h (0, 20, 40)", cibles(en(0)).vent == 0 && cibles(en(20)).vent == 0 && cibles(en(40)).vent == 0);
        cas("vent 0,30 à la vitesse de pointe (avant le plafond de RG-08)", std::abs(ciblesBrutes(en(250)).vent - VENT_MAX) < 1e-5f);
        cas("à la pointe sur route sèche : 0,30 + 0,18 ramenés à 0,45 ensemble", std::abs(cibles(en(250)).somme() - SOMME_MAX) < 1e-5f && std::abs(cibles(en(250)).vent / cibles(en(250)).roulement - VENT_MAX / ROULEMENT_MAX) < 1e-4f);
        cas("vent qui croît avec le carré de la vitesse (100 < 150 < 200)", ciblesBrutes(en(100)).vent < ciblesBrutes(en(150)).vent && ciblesBrutes(en(150)).vent < ciblesBrutes(en(200)).vent
            && ciblesBrutes(en(150)).vent - ciblesBrutes(en(100)).vent < ciblesBrutes(en(200)).vent - ciblesBrutes(en(150)).vent);
        cas("nitro : vent + 20 %", std::abs(ciblesBrutes(en(200, 0, false, true)).vent - ciblesBrutes(en(200)).vent * VENT_NITRO) < 1e-5f);
        cas("filtre du vent : fermé à 50 km/h, ouvert à la pointe", cibles(en(50)).ouverture == 0 && cibles(en(250)).ouverture == 1);
        cas("roulement nul à l'arrêt, 0,18 à la pointe (avant le plafond)", cibles(en(0)).roulement == 0 && std::abs(ciblesBrutes(en(250)).roulement - ROULEMENT_MAX) < 1e-5f);
        const Cibles b = cibles(en(120, 1.6f)), b2 = cibles(en(120, -1.7f)), h = cibles(en(120, 1.2f)), h2 = cibles(en(120, -1.01f));
        cas("bord à |x| ≥ edge (1,6 et −1,7) : bande rugueuse, plus de roulement", b.bord > 0 && b.roulement == 0 && b2.bord > 0 && b.surface == Surface::Bord);
        cas("bord : cadence de 15 à 60 Hz selon la vitesse", cibles(en(0.1f, 1.6f)).cadenceBord < 16 && std::abs(cibles(en(250, 1.6f)).cadenceBord - BORD_HZ_MAX) < 1e-3f);
        cas("bord à l'arrêt : pas de bande rugueuse", cibles(en(1, 1.6f)).bord == 0);
        cas("hors route à |x| > 1 (1,2 et −1,01) : herbe / gravier, plus de roulement", h.herbe > 0 && h.roulement == 0 && h2.herbe > 0 && h.surface == Surface::HorsRoute);
        cas("sur la route (|x| = 1) : roulement, ni bord ni herbe", cibles(en(120, 1.0f)).roulement > 0 && cibles(en(120, 1.0f)).herbe == 0 && cibles(en(120, 1.0f)).bord == 0);
        cas("eau seulement sous la pluie (0,20 à la pointe)", cibles(en(250)).eau == 0 && cibles(en(250, 0, true)).eau > 0 && cibles(en(0, 0, true)).eau == 0);
        const Cibles off = cibles(en(200, 1.2f, true, true, false));
        cas("silence hors course (pause, menus, garage, fin)", off.somme() == 0);
        float pire = 0;
        for (float kmh : {60.f, 150.f, 250.f, 320.f}) for (float x : {0.f, 1.2f, 1.6f}) for (bool p : {false, true}) for (bool n : {false, true})
            pire = std::max(pire, cibles(en(kmh, x, p, n)).somme());
        cas("somme des sons de la route ≤ 0,45 (vitesse, nitro, bord, herbe, pluie)", pire <= SOMME_MAX + 1e-5f);
        // RG-08 : crête du mélange de la route seule (boucles de sfx.cpp, crête 1) au pire cas, battement de +15 % compris
        cas("crête des sons de la route < 1,0 (≤ 0,45 × 1,15 sur des boucles de crête 1)", SOMME_MAX * (1 + BATTEMENT) < 1.0f);
        cas("lissage : 95 % de la cible en 0,15 s", [] { float g = 0; const float k = lissage(48000); for (int i = 0; i < (int)(0.15f * 48000); i++) g += (1 - g) * k; return g > 0.94f && g < 0.96f; }());
        using namespace sfx; const float R = 48000;
        std::vector<float> vg = boucle(R, 0.62f, VENT_GRAVE_HZ, 0.5f, 1), ro = boucle(R, 0.41f, 140, 0.8f, 1, 90, 0.9f), ea = boucle(R, 0.37f, 4500, 0.7f, 1);
        verifie("route-vent", vg, true); verifie("route-roulement", ro, true); verifie("route-eau", ea, true);
        const size_t octets = (size_t)((0.62f + 0.57f + 0.41f + 0.53f + 0.37f + 0.012f) * R) * sizeof(float);
        std::printf("route : mémoire des boucles %.0f Ko à 48 kHz\n", octets / 1024.0);
        cas("mémoire des boucles ≤ 0,5 Mo (RG-09)", octets <= 512 * 1024);
    }
    return ok && okSfx && okEff && okTrafic && okCourse && okGarage && okSirene && okRoute ? 0 : 1;
}
