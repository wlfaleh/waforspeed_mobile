// WaForSpeed — module « son » (WFS-10) : son du moteur de la voiture du joueur, branché sur le moteur du jeu
// par les systèmes enfichables (WFS-30). Le régime est calculé à chaque pas de conduite (thread du jeu) ;
// le son est fabriqué sur le thread audio (AAudio). Sur le banc d'essai PC, il est calculé hors ligne et
// enregistré (moteur.wav, moteur-courbe.png) pour la QA.
#include "audio_system.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include "../../third_party/stb/stb_image_write.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/cars.h"
#include "../game/game.h"
#include "../game/player.h"
#include "audio_out.h"
#include "engine_synth.h"
#include "sfx.h"   // WFS-28 : bruitages

namespace wfs {

static const char* SAMPLES[] = {"audio/sport.wav", "audio/v8.wav", "audio/citadine.wav", "audio/thermique.wav", "audio/gros.wav"};
constexpr int SAMPLE_COUNT = sizeof(SAMPLES) / sizeof(SAMPLES[0]);
constexpr int OUT_RATE = 48000;

struct BenchRec { bool on = false; std::vector<float> pcm; std::vector<EngineFrame> frames; double acc = 0, t = 0; };
static BenchRec g_rec;

class AudioSystem : public System {
public:
    const char* name() const override { return "son"; }

    void init(Renderer&, const GameView&) override {
        for (int i = 0; i < SAMPLE_COUNT; i++) {
            std::vector<uint8_t> w;
            synths_[i] = std::make_unique<EngineSynth>();
            if (!readAsset(SAMPLES[i], w) || !synths_[i]->loadWav(w)) WFS_ERR("son : %s illisible", SAMPLES[i]);
            synths_[i]->setOutputRate(OUT_RATE);
        }
        active_ = -1;
        const int rate = audioOutStart([this](float* out, int n) { const double now = nowSeconds(); renderInto(out, n, now); sfx_.mix(out, n, now); });
        if (rate && rate != OUT_RATE) for (auto& s : synths_) s->setOutputRate((float)rate);
        sfx_.build((float)(rate ? rate : OUT_RATE));   // WFS-28 : bruitages calculés une fois, à la fréquence du flux
    }

    void shutdown(Renderer&) override { audioOutStop(); }

    // à chaque pas de conduite : régime, rapport, charge → cibles du son
    void update(double dt, const GameView& g) override {
        const Player& pl = *g.player;
        const EngineVoiceDef& voice = engineVoiceFor(g.car->id);
        int idx = 0;
        for (int i = 0; i < SAMPLE_COUNT; i++) if (std::strcmp(SAMPLES[i], voice.sample) == 0) idx = i;
        // pédale d'accélérateur : déduite de la conduite (le module ne lit que l'état du joueur) :
        // nitro, ou la voiture ne ralentit pas comme au lever de pied (frottements + frein moteur ≥ 1,2 m/s²)
        const double v = std::abs(pl.speed) * M;
        const bool throttle = pl.nitroOn || (!pl.braking && (pl.lonAcc > 0.4 || (v > 0.5 && pl.lonAcc > -0.6)));
        const double top = MAX_SPEED * pl.perf.top * (pl.nitroOn ? pl.perf.nitroMult : 1);
        EngineSynth& s = *synths_[idx];
        model_.update(dt, pl.speed, top, MAX_SPEED, throttle, pl.nitroOn, true, voice.pitch, false, s.targets);
        s.targets.lastUpdate = nowSeconds();
        for (int i = 0; i < SAMPLE_COUNT; i++) if (i != idx) { synths_[i]->targets.loGain = 0; synths_[i]->targets.hiGain = 0; }
        active_ = idx;
        sfx_.lastUpdate = nowSeconds(); events_.update(dt, g, sfx_);   // WFS-28 : un son par événement de la partie
        if (g_rec.on) {   // banc d'essai : le son de ce pas est calculé tout de suite et gardé
            g_rec.t += dt; g_rec.acc += dt * OUT_RATE;
            const int n = (int)g_rec.acc; g_rec.acc -= n;
            const size_t at = g_rec.pcm.size(); g_rec.pcm.resize(at + (size_t)n);
            for (auto& x : synths_) x->targets.lastUpdate = 1e9;   // hors ligne : pas de « pause » détectée
            renderInto(g_rec.pcm.data() + at, n, 0);
            g_rec.frames.push_back({(float)g_rec.t, model_.rpm, model_.gear, model_.load, (float)pl.kmh(), throttle});
        }
    }

private:
    void renderInto(float* out, int n, double now) {
        const int a = active_;
        if (a < 0) { std::fill(out, out + n, 0.f); return; }
        synths_[a]->render(out, n, now);
    }
    std::unique_ptr<EngineSynth> synths_[SAMPLE_COUNT];
    std::atomic<int> active_{-1};
    EngineModel model_;
    SfxMixer sfx_; SfxEvenements events_;   // WFS-28 : bruitages (mélangés au moteur dans le rappel audio)
};
WFS_SYSTEM(AudioSystem, 300)

// ---------------------------------------------------------------- banc d'essai
static void writeWav(const std::string& path, const std::vector<float>& pcm, int rate) {
    std::ofstream f(path, std::ios::binary);
    auto u32 = [&](uint32_t x) { f.write((const char*)&x, 4); };
    auto u16 = [&](uint16_t x) { f.write((const char*)&x, 2); };
    const uint32_t bytes = (uint32_t)pcm.size() * 2;
    f.write("RIFF", 4); u32(36 + bytes); f.write("WAVEfmt ", 8); u32(16); u16(1); u16(1); u32(rate); u32(rate * 2); u16(2); u16(16);
    f.write("data", 4); u32(bytes);
    for (float x : pcm) { const int16_t s = (int16_t)std::lround(std::clamp(x, -1.f, 1.f) * 32767); f.write((const char*)&s, 2); }
}

// courbe : forme d'onde (gris), régime (orange), rapport (cyan, en marches), vitesse (blanc), pédale (vert, en bas)
static void writeCurve(const std::string& path, const std::vector<float>& pcm, const std::vector<EngineFrame>& fr) {
    const int W = 1200, H = 520;
    std::vector<uint8_t> img((size_t)W * H * 3, 18);
    auto px = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || y < 0 || x >= W || y >= H) return;
        uint8_t* p = &img[((size_t)y * W + x) * 3]; p[0] = r; p[1] = g; p[2] = b;
    };
    // quadrillage : une ligne par seconde
    const float T = fr.empty() ? 1 : fr.back().t;
    for (int s = 0; s <= (int)T; s++) { const int x = (int)(s / T * (W - 1)); for (int y = 0; y < H; y += 3) px(x, y, 50, 50, 60); }
    // forme d'onde (enveloppe) : bande du haut
    const int wy = 110, wh = 90;
    for (int x = 0; x < W; x++) {
        const size_t a = pcm.size() * x / W, b = std::max(a + 1, pcm.size() * (x + 1) / W);
        float lo = 0, hi = 0;
        for (size_t i = a; i < b && i < pcm.size(); i++) { lo = std::min(lo, pcm[i]); hi = std::max(hi, pcm[i]); }
        for (int y = (int)(wy - hi * wh * 2.5f); y <= (int)(wy - lo * wh * 2.5f); y++) px(x, y, 150, 150, 160);
    }
    // courbes
    auto plot = [&](float t, float v01, int y0, int h, uint8_t r, uint8_t g, uint8_t b) {
        const int x = (int)(t / T * (W - 1)), y = y0 - (int)(std::clamp(v01, 0.f, 1.1f) * h);
        for (int d = -1; d <= 1; d++) { px(x, y + d, r, g, b); px(x + d, y, r, g, b); }
    };
    for (const auto& f : fr) {
        plot(f.t, f.rpm / 1.05f, 470, 230, 255, 120, 20);        // régime
        plot(f.t, (f.gear - 1) / 4.f, 470, 230, 0, 229, 255);    // rapport 1 à 5
        plot(f.t, f.kmh / 300.f, 470, 230, 240, 240, 240);       // vitesse (0-300 km/h)
        if (f.throttle) for (int y = 495; y < 510; y++) px((int)(f.t / T * (W - 1)), y, 60, 220, 90);   // pédale
    }
    stbi_write_png(path.c_str(), W, H, 3, img.data(), W * 3);
}

void benchAudioScene(Game& g, Renderer& r, const std::string& out) {
    // scénario : ralenti 2 s, plein gaz 14 s (passages de rapports), lever de pied 4 s, freinage 3 s, gaz 5 s
    g.setAutopilot(false);
    g.init(r, "golf", "ville");
    g_rec = BenchRec{}; g_rec.on = true;
    const float W = (float)r.width(), H = (float)r.height();
    const Touch gasDown{7, W * 0.89f, H * 0.74f, true}, gasUp{7, W * 0.89f, H * 0.74f, false};
    const Touch brDown{8, W * 0.70f, H * 0.80f, true}, brUp{8, W * 0.70f, H * 0.80f, false};
    struct Phase { double until; int what; };   // 0 rien, 1 gaz, 2 frein
    const Phase phases[] = {{2, 0}, {16, 1}, {20, 0}, {23, 2}, {28, 1}};
    int cur = -1;
    g.onTouch({9, W * 0.18f, H * 0.72f, true});   // point de départ du volant
    for (int i = 0; i < 28 * 60; i++) {
        const double t = i / 60.0;
        int p = 0; while (p < 4 && t >= phases[p].until) p++;
        if (phases[p].what != cur) {
            if (cur == 1) g.onTouch(gasUp);
            if (cur == 2) g.onTouch(brUp);
            cur = phases[p].what;
            if (cur == 1) g.onTouch(gasDown);
            if (cur == 2) g.onTouch(brDown);
        }
        // volant : reste au milieu de la route (sinon la voiture dérive dans les virages et sort de la route)
        const Player& pl = g.player();
        const float st = clampf((float)((0.0 - pl.playerX) * 3 - pl.heading * 4), -1.f, 1.f);
        g.onTouch({9, W * 0.18f + st * W * 0.12f, H * 0.72f, true});
        g.update(1.0 / 60);
    }
    g.onTouch({9, W * 0.18f, H * 0.72f, false});
    g_rec.on = false;
    writeWav(out + "/moteur.wav", g_rec.pcm, OUT_RATE);
    writeCurve(out + "/moteur-courbe.png", g_rec.pcm, g_rec.frames);
    // une ligne par seconde (journal de la CI) + crête / craquements : saut d'un échantillon à l'autre
    float peak = 0, jump = 0;
    for (size_t i = 1; i < g_rec.pcm.size(); i++) { peak = std::max(peak, std::abs(g_rec.pcm[i])); jump = std::max(jump, std::abs(g_rec.pcm[i] - g_rec.pcm[i - 1])); }
    int lastS = -1;
    for (const auto& f : g_rec.frames) if ((int)f.t != lastS) {
        lastS = (int)f.t;
        WFS_LOG("son t=%2ds : %3.0f km/h, rapport %d, régime %.2f, charge %.2f, pédale %s", lastS, f.kmh, f.gear, f.rpm, f.load, f.throttle ? "oui" : "non");
    }
    WFS_LOG("son : %.1f s enregistrées (%zu échantillons), crête %.2f, plus grand saut %.3f", g_rec.pcm.size() / (double)OUT_RATE, g_rec.pcm.size(), peak, jump);
    g.shutdown();
}

} // namespace wfs
