// WaForSpeed — son du moteur (WFS-10) : voir engine_synth.h.
#include "engine_synth.h"
#include <algorithm>
#include <cstring>

namespace wfs {

static constexpr float PI_F = 3.14159265358979f;

// ENGINE_SET du jeu web, pour les voitures natives
static const EngineVoiceDef VOICES[] = {
    {"golf", "audio/sport.wav", 1.0f}, {"m4", "audio/v8.wav", 1.25f},
    {"p911", "audio/v8.wav", 1.45f}, {"gallardo", "audio/v8.wav", 1.62f},
    // WFS-13 : ENGINE_SET de js/audio/sfx.js (model3 « électrique » : citadine en attendant le son électrique)
    {"p208", "audio/citadine.wav", 1.0f}, {"abarth", "audio/citadine.wav", 1.14f}, {"tcross", "audio/citadine.wav", 0.9f},
    {"compass", "audio/thermique.wav", 1.0f}, {"s3", "audio/sport.wav", 0.95f}, {"x5", "audio/thermique.wav", 0.88f},
    {"giulia", "audio/sport.wav", 0.86f}, {"a45", "audio/sport.wav", 1.06f}, {"model3", "audio/citadine.wav", 1.0f},
    {"corsa", "audio/citadine.wav", 0.96f}, {"gt86", "audio/sport.wav", 1.12f}, {"elantra", "audio/sport.wav", 1.0f},
    {"v60", "audio/thermique.wav", 1.1f}, {"rrsport", "audio/gros.wav", 1.0f}, {"mustang", "audio/v8.wav", 0.9f},
    {"sf90", "audio/v8.wav", 1.72f},
};
const EngineVoiceDef& engineVoiceFor(const char* carId) {
    for (const auto& v : VOICES) if (std::strcmp(v.carId, carId) == 0) return v;
    return VOICES[0];
}

// ---------------------------------------------------------------- filtres (spécification du Web Audio)
void Biquad::lowpass(float freq, float qDb, float rate) {
    const float w0 = 2 * PI_F * std::min(freq, rate * 0.49f) / rate, c = std::cos(w0);
    const float alpha = std::sin(w0) / (2 * std::pow(10.f, qDb / 20.f));
    const float a0 = 1 + alpha;
    b0 = (1 - c) / 2 / a0; b1 = (1 - c) / a0; b2 = b0; a1 = -2 * c / a0; a2 = (1 - alpha) / a0;
}
void Biquad::peaking(float freq, float q, float gainDb, float rate) {
    const float w0 = 2 * PI_F * std::min(freq, rate * 0.49f) / rate, c = std::cos(w0);
    const float A = std::pow(10.f, gainDb / 40.f), alpha = std::sin(w0) / (2 * q);
    const float a0 = 1 + alpha / A;
    b0 = (1 + alpha * A) / a0; b1 = -2 * c / a0; b2 = (1 - alpha * A) / a0; a1 = -2 * c / a0; a2 = (1 - alpha / A) / a0;
}

// ---------------------------------------------------------------- régime (engineUpdate du jeu web)
static constexpr float GEAR_EDGES[6] = {0, 0.16f, 0.34f, 0.54f, 0.76f, 1.3f};

void EngineModel::update(double dt, double speed, double top, double maxSpeed, bool throttleOn, bool nitro, bool active,
                         float pitch, bool electric, EngineTargets& out) {
    const float fdt = (float)std::min(dt, 0.05);
    const float throttle = active && throttleOn ? 1.f : 0.f;
    load += (throttle - load) * std::min(1.f, fdt * (throttle > 0 ? 10.f : 5.f));
    const float v = (float)std::min(1.25, std::abs(speed) / std::max(top, 1.0));
    const float* E = GEAR_EDGES;
    int g = 1; while (g < 5 && v >= E[g]) g++;
    if (g > gear && speed > 0 && throttle > 0) { shiftT = 0.16f; shiftDip = 1; }
    gear = g;
    const float inGear = std::min(1.f, (v - E[g - 1]) / (E[g] - E[g - 1]));
    float rpmT;
    if (std::abs(speed) < maxSpeed * 0.01) rpmT = throttle > 0 ? 0.85f : 0.1f;          // à l'arrêt : coup d'accélérateur
    else rpmT = 0.22f + 0.7f * inGear + load * 0.1f - (1 - load) * 0.1f;                 // en roulant : la pédale pousse
    if (g == 1 && throttle > 0) rpmT = std::max(rpmT, 0.5f);                             // démarrage
    const bool up = rpmT > rpm;
    rpm += (rpmT - rpm) * std::min(1.f, fdt * (up ? (throttle > 0 ? 9.f : 4.f) : (throttle > 0 ? 6.f : 2.5f)));
    rpm = std::clamp(rpm, 0.06f, 1.05f);
    shiftT = std::max(0.f, shiftT - fdt); shiftDip = std::max(0.f, shiftDip - fdt * 7);
    cibles(v, active, pitch, nitro, electric, out);
}

void EngineModel::updateReel(double dt, double regimeTrMin, double regimeCoupure, int rapport, double vitesse, double top,
                             bool throttleOn, bool nitro, bool active, float pitch, bool electric, EngineTargets& out) {
    const float fdt = (float)std::min(dt, 0.05);
    const float throttle = active && throttleOn ? 1.f : 0.f;
    load += (throttle - load) * std::min(1.f, fdt * (throttle > 0 ? 10.f : 5.f));
    const int g = std::max(1, rapport);   // marche arrière : son du 1er rapport
    if (g > gear && throttle > 0) { shiftT = 0.16f; shiftDip = 1; }
    gear = g;
    const float rpmT = std::clamp((float)(regimeTrMin / std::max(regimeCoupure, 1.0)), 0.06f, 1.05f);
    rpm += (rpmT - rpm) * std::min(1.f, fdt * 25.f);   // suit le régime réel (lissage de 40 ms : pas de saut audible)
    shiftT = std::max(0.f, shiftT - fdt); shiftDip = std::max(0.f, shiftDip - fdt * 7);
    cibles((float)std::min(1.25, std::abs(vitesse) / std::max(top, 1.0)), active, pitch, nitro, electric, out);
}

void EngineModel::cibles(float v, bool active, float pitch, bool nitro, bool electric, EngineTargets& out) {
    out.electric = electric;
    if (electric) {   // pas de moteur thermique : sifflement de l'onduleur qui monte avec la vitesse
        out.elecA = 180 + v * 1400; out.elecB = 90 + v * 700; out.elecFreq = 1800 + v * 2500;
        out.elecGain = active ? 0.01f + v * 0.025f * (0.6f + load * 0.6f) : 0.f;
        out.loGain = 0; out.hiGain = 0;
        return;
    }
    out.elecGain = 0;
    const float r = std::max(0.f, rpm - shiftDip * 0.12f);
    out.rate = std::clamp(pitch * (0.55f + r * 1.15f) * (nitro ? 1.06f : 1.f), 0.4f, 2.6f);
    const float cut = shiftT > 0 ? 0.45f : 1.f;
    const float base = active ? 0.13f + r * 0.1f : 0.f;
    out.loGain = base * (1 - load * 0.55f) * cut;
    out.loFreq = 500 + r * 1600;
    out.hiGain = active ? load * (0.17f + r * 0.2f) * cut : 0.f;
    out.hiTau = shiftT > 0 ? 0.015f : 0.04f;
    out.hiFreq = 2200 + r * 6000;
}

// ---------------------------------------------------------------- pétarades, turbo, rupteur (WFS-120)
void EffetsMoteur::update(double dt, float rpm, int gear, float v, bool throttle, float kmh, bool enCourse, bool electrique) {
    const float fdt = (float)std::min(dt, 0.05);
    claquements = 0; turbo = false; rupteurDebut = false; couper = false;
    depuisSalve_ += fdt; depuisTurbo_ += fdt;
    if (!enCourse || electrique) {   // RG-04 / RG-06 : rien hors course ni en électrique ; salve en cours abandonnée
        attente_ = 0; rupteur = false; aFond_ = 0; pied_ = throttle; rapport_ = gear;
        return;
    }
    // RG-01 : lever de pied à haut régime → salve de 2 à 4 claquements espacés de 60 à 110 ms
    if (pied_ && !throttle && (rpm >= PETARADE_RPM || v >= PETARADE_V) && kmh >= PETARADE_KMH && depuisSalve_ >= PETARADE_PAUSE) {
        const int n = 2 + (int)(hasard() * 3);   // 2, 3 ou 4
        float t = 0;
        for (int i = 0; i < n; i++) { prochains_[i] = t; t += 0.06f + hasard() * 0.05f; }
        attente_ = n; depuisSalve_ = 0;
    }
    if (throttle) attente_ = 0;   // remise des gaz : la salve s'arrête
    int restants = 0;
    for (int i = 0; i < attente_; i++) {
        prochains_[i] -= fdt;
        if (prochains_[i] <= 0) claquements++; else prochains_[restants++] = prochains_[i];
    }
    attente_ = restants;
    // RG-02 : passage au rapport supérieur en charge
    if (gear > rapport_ && throttle && depuisTurbo_ >= TURBO_PAUSE) { turbo = true; depuisTurbo_ = 0; }
    // RG-03 : à fond en 5e à la vitesse de pointe depuis plus de 0,3 s → coupures toutes les 70 ms
    if (gear == 5 && throttle && v >= RUPTEUR_V) aFond_ += fdt; else aFond_ = 0;
    if (!rupteur && aFond_ > RUPTEUR_DELAI) { rupteur = true; rupteurDebut = true; pasRupteur_ = 0; }
    if (rupteur && (!throttle || v < RUPTEUR_V_FIN || gear < 5)) rupteur = false;
    if (rupteur) { pasRupteur_ += fdt; couper = ((int)(pasRupteur_ / RUPTEUR_PAS)) % 2 == 1; }
    pied_ = throttle; rapport_ = gear;
}

// ---------------------------------------------------------------- rendu
bool EngineSynth::loadWav(const std::vector<uint8_t>& w) {
    // WAV PCM 16 bits mono (sons du jeu web) : on cherche les blocs « fmt » et « data »
    if (w.size() < 44 || std::memcmp(w.data(), "RIFF", 4) || std::memcmp(w.data() + 8, "WAVE", 4)) return false;
    size_t p = 12; int channels = 1, bits = 16; uint32_t rate = 22050;
    const uint8_t* data = nullptr; uint32_t bytes = 0;
    auto u32 = [&](size_t o) { uint32_t x; std::memcpy(&x, w.data() + o, 4); return x; };
    auto u16 = [&](size_t o) { uint16_t x; std::memcpy(&x, w.data() + o, 2); return x; };
    while (p + 8 <= w.size()) {
        const uint32_t len = u32(p + 4);
        if (!std::memcmp(w.data() + p, "fmt ", 4) && p + 24 <= w.size()) { channels = u16(p + 10); rate = u32(p + 12); bits = u16(p + 22); }
        if (!std::memcmp(w.data() + p, "data", 4)) { data = w.data() + p + 8; bytes = std::min<uint32_t>(len, (uint32_t)(w.size() - p - 8)); break; }
        p += 8 + len + (len & 1);
    }
    if (!data || bits != 16 || channels < 1) return false;
    const size_t frames = bytes / (2 * channels);
    sample_.resize(frames);
    for (size_t i = 0; i < frames; i++) { int16_t s; std::memcpy(&s, data + i * 2 * channels, 2); sample_[i] = s / 32768.f; }
    srcRate_ = (float)rate;
    // deux voix sur le même enregistrement, décalées d'une demi-boucle (comme le jeu web)
    posLo_ = 0.45 * frames; posHi_ = 0.95 * frames;
    return frames > 16;
}

void EngineSynth::setOutputRate(float rate) {
    outRate_ = rate;
    loFreq_.v = 900; hiFreq_.v = 6000; elecFreq_.v = 1800; rate_.v = 1; elecA_.v = 200; elecB_.v = 100;
    coefCountdown_ = 0;
}

float EngineSynth::sampleAt(double pos) const {
    const size_t n = sample_.size();
    const size_t i = (size_t)pos % n, j = (i + 1) % n;
    const float f = (float)(pos - std::floor(pos));
    return sample_[i] + (sample_[j] - sample_[i]) * f;
}

void EngineSynth::render(float* out, int n, double nowSec) {
    // setTargetAtTime : chaque réglage tend vers sa cible avec sa constante de temps (s)
    auto k = [&](float tau) { return 1.f - std::exp(-1.f / (outRate_ * tau)); };
    const float kRate = k(0.03f), kLoG = k(0.05f), kLoF = k(0.06f), kHiG = k(targets.hiTau.load()), kHiF = k(0.05f);
    const float kEf = k(0.05f), kEg = k(0.12f), kFade = k(0.05f);
    const float tRate = targets.rate, tLoG = targets.loGain, tLoF = targets.loFreq, tHiG = targets.hiGain, tHiF = targets.hiFreq;
    const float tEA = targets.elecA, tEB = targets.elecB, tEF = targets.elecFreq, tEG = targets.elecGain;
    const bool electric = targets.electric;
    // le jeu ne donne plus de nouvelles (pause, appli en arrière-plan) : le son s'éteint en douceur
    const float tFade = nowSec - targets.lastUpdate.load() > 0.25 ? 0.f : 1.f;
    const size_t len = sample_.size();
    const double step = srcRate_ / outRate_;
    for (int i = 0; i < n; i++) {
        if (coefCountdown_-- <= 0) {   // coefficients des filtres recalculés toutes les 32 échantillons
            coefCountdown_ = 32;
            lo_.lowpass(loFreq_.v, 0.7f, outRate_);
            hiPeak_.peaking(1400, 0.8f, 5, outRate_);
            hiLp_.lowpass(hiFreq_.v, 0.5f, outRate_);
            elecLp_.lowpass(elecFreq_.v, 3, outRate_);
        }
        const float fade = fade_.step(tFade, kFade);
        float y = 0;
        if (len && !electric) {
            const float r = rate_.step(tRate, kRate);
            loFreq_.step(tLoF, kLoF); hiFreq_.step(tHiF, kHiF);
            const float sLo = sampleAt(posLo_), sHi = sampleAt(posHi_);
            posLo_ += r * step; posHi_ += r * step;
            if (posLo_ >= len) posLo_ -= len;
            if (posHi_ >= len) posHi_ -= len;
            y += lo_.run(sLo) * loGain_.step(tLoG, kLoG);
            y += hiLp_.run(hiPeak_.run(driveCurve(sHi * 1.8f))) * hiGain_.step(tHiG, kHiG);
        }
        // voiture électrique : onduleur (sinus + triangle), filtré
        const float eg = elecGain_.step(electric ? tEG : 0.f, kEg);
        if (eg > 1e-5f) {
            phA_ += elecA_.step(tEA, kEf) / outRate_; phB_ += elecB_.step(tEB, kEf) / outRate_;
            phA_ -= std::floor(phA_); phB_ -= std::floor(phB_);
            elecFreq_.step(tEF, kEf);
            const float tri = 4 * std::abs((float)phB_ - 0.5f) - 1;
            y += elecLp_.run(std::sin(2 * PI_F * (float)phA_) + 0.8f * tri) * eg;
        }
        out[i] = std::clamp(y * master * fade, -1.f, 1.f);
    }
}

} // namespace wfs
