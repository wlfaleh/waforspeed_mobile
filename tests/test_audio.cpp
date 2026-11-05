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
    return ok && okSfx && okEff ? 0 : 1;
}
