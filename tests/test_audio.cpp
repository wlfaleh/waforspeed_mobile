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
#include "../src/audio/trafic_sons.h"  // WFS-121
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
    return ok && okSfx && okEff && okTrafic ? 0 : 1;
}
