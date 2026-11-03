// WaForSpeed — bruitages (WFS-28) : chocs, frottements, quasi-contacts, bips de course, crissements, souffle de nitro.
// Port de js/audio/sfx.js (noiseHit, beep, setMuted) et des appels de js/game/update.js / js/ui/screens.js.
// Le jeu (thread principal) demande un son par une file sans verrou ; le thread audio (AAudio, WFS-10) le mélange
// au moteur. Sur le banc d'essai PC (pas de carte son) chaque son demandé écrit « SFX <nom> » au journal (RG-11).
#pragma once
#include <atomic>
#include <cstdint>
#include <vector>

namespace wfs {

struct GameView;
struct Course;

enum class SfxId : uint8_t {
    Accident, Choc, Frottement, ContactDuel, QuasiContact,
    BipDecompte, BipGo, BipCheckpoint, BipFin, BipDuel, Count
};
const char* sfxNom(SfxId id);

class SfxMixer {
public:
    void build(float rate);                      // calcule tous les sons (démarrage, hors du thread audio)
    void play(SfxId id, float gain = 1);         // thread du jeu : un son, une fois
    void setBoucles(float crissement, float nitro);   // thread du jeu : volume visé des sons continus (0..1)
    void setMuted(bool m) { muted_ = m; }        // future option « Son » (setMuted du jeu web)
    // thread audio : ajoute les bruitages dans out ; now : horloge (sans nouvelles du jeu → silence, pause)
    void mix(float* out, int n, double now);
    std::atomic<double> lastUpdate{0};
    float master = 0.5f;                         // sfxGain du jeu web
    const std::vector<float>& tampon(SfxId id) const { return sons_[(int)id]; }
private:
    std::vector<float> sons_[(int)SfxId::Count], crissement_, nitro_;
    struct Demande { uint8_t id; float gain; };
    static constexpr int FILE = 64, VOIX = 16;
    Demande file_[FILE];
    std::atomic<uint32_t> tete_{0}, queue_{0};
    struct Voix { const std::vector<float>* buf = nullptr; size_t pos = 0; float gain = 0; };
    Voix voix_[VOIX];
    std::atomic<float> cibleCrissement_{0}, cibleNitro_{0};
    std::atomic<bool> muted_{false};
    float gCrissement_ = 0, gNitro_ = 0, fade_ = 0, k_ = 0.001f, kFade_ = 0.001f;
    size_t posCrissement_ = 0, posNitro_ = 0;
    std::atomic<bool> pret_{false};
};

// suit la partie (lecture seule : activeCourse(), joueur) et demande un son à chaque changement d'état (RG-08)
class SfxEvenements {
public:
    void update(double dt, const GameView& g, SfxMixer& m);
private:
    void resync(const Course& c);
    const Course* c_ = nullptr;
    int count_ = -1, level_ = 0, accidents_ = 0, near_ = 0, bumps_ = 0, scrapes_ = 0, state_ = 0;
    bool crisse_ = false, nitro_ = false;
};

} // namespace wfs
