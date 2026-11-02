// WaForSpeed — son du moteur (WFS-10), port de js/audio/sfx.js du jeu web (engineUpdate, engineVoice).
//
// Deux parties :
//  - EngineModel (thread du jeu) : régime, rapport (boîte à 5 rapports), charge de la pédale → cibles du son ;
//  - EngineSynth (thread audio) : vrais enregistrements en boucle, lus à vitesse variable (hauteur = régime),
//    voix « lever de pied » (filtre passe-bas) + voix « en charge » (saturée, plus claire), lissage de chaque
//    réglage comme AudioParam.setTargetAtTime du Web Audio. Aucune allocation pendant le rendu (pas de craquement).
// Les cibles passent du jeu au son par des std::atomic : pas de verrou dans le rappel audio.
#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace wfs {

// son de chaque voiture native (ENGINE_SET du jeu web) : enregistrement et hauteur de base
struct EngineVoiceDef { const char* carId; const char* sample; float pitch; };
const EngineVoiceDef& engineVoiceFor(const char* carId);

// filtre biquad (formules du Web Audio, « Audio EQ Cookbook ») ; Q des passe-bas en dB comme BiquadFilterNode
struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void lowpass(float freq, float qDb, float rate);
    void peaking(float freq, float q, float gainDb, float rate);
    float run(float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
};

// cibles envoyées au son (une par réglage du jeu web)
struct EngineTargets {
    std::atomic<float> rate{1}, loGain{0}, loFreq{900}, hiGain{0}, hiFreq{6000};
    std::atomic<float> hiTau{0.04f};          // le son « en charge » se coupe vite au passage de rapport
    std::atomic<float> elecA{200}, elecB{100}, elecFreq{1800}, elecGain{0};
    std::atomic<bool> electric{false};
    std::atomic<double> lastUpdate{0};        // horloge du dernier update du jeu : sans nouvelles, le son s'éteint (pause)
};

class EngineModel {
public:
    // une étape (dt en s) : speed et top en unités/s, throttle 0/1, nitro, active = en course
    void update(double dt, double speed, double top, double maxSpeed, bool throttle, bool nitro, bool active,
                float pitch, bool electric, EngineTargets& out);
    float rpm = 0.12f, load = 0; int gear = 1; float shiftT = 0, shiftDip = 0;
};

class EngineSynth {
public:
    // échantillon mono 16 bits (WAV) ; rate de sortie (Hz)
    bool loadWav(const std::vector<uint8_t>& wav);
    void setOutputRate(float rate);
    // remplit `out` (mono, n échantillons) ; nowSec : horloge pour détecter la pause du jeu
    void render(float* out, int n, double nowSec);
    EngineTargets targets;
    float master = 0.5f;                       // sfxGain du jeu web
private:
    struct Smooth { float v = 0; float step(float target, float k) { v += (target - v) * k; return v; } };
    std::vector<float> sample_; float srcRate_ = 22050, outRate_ = 48000;
    double posLo_ = 0, posHi_ = 0;
    Biquad lo_, hiPeak_, hiLp_, elecLp_;
    Smooth rate_, loGain_, loFreq_, hiGain_, hiFreq_, elecA_, elecB_, elecFreq_, elecGain_, fade_;
    double phA_ = 0, phB_ = 0;
    int coefCountdown_ = 0;
    float sampleAt(double pos) const;
};

// courbe « en charge » du jeu web : tanh(x · 2,6) / tanh(2,6), entrée bornée à [-1, 1] (WaveShaperNode)
inline float driveCurve(float x) {
    x = x < -1 ? -1 : (x > 1 ? 1 : x);
    return std::tanh(x * 2.6f) / 0.98903f;   // tanh(2,6) = 0,98903
}

} // namespace wfs
