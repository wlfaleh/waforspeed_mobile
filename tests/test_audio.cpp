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
    }
    return ok && okSfx ? 0 : 1;
}
