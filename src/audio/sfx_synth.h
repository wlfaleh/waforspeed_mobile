// WaForSpeed — fabrication des bruitages (WFS-28), port de js/audio/sfx.js du jeu web (noiseHit, beep).
// Les sons sont calculés une fois au démarrage (thread du jeu) dans des tampons ; le thread audio ne fait que
// les lire et les mélanger (aucune allocation dans le rappel). En-tête seul : utilisé par sfx.cpp et tests/test_audio.cpp.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace wfs::sfx {

constexpr float PI_F = 3.14159265358979f;
constexpr float FONDU = 0.005f;   // attaque et relâche de 5 ms : aucun craquement au début ni à la fin (RG-10)

// bruit blanc reproductible (même son à chaque lancement) : Math.random() * 2 − 1 du jeu web
struct Bruit {
    uint32_t s = 0x9E3779B9u;
    float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float)((double)s / 4294967295.0 * 2.0 - 1.0); }
};

// passe-bande du Web Audio (BiquadFilterNode « bandpass », gain 0 dB au sommet, Audio EQ Cookbook)
struct PasseBande {
    float b0 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    void regler(float freq, float q, float rate) {
        freq = std::clamp(freq, 10.f, rate * 0.45f);
        const float w = 2 * PI_F * freq / rate, alpha = std::sin(w) / (2 * std::max(q, 0.0001f)), a0 = 1 + alpha;
        b0 = alpha / a0; b2 = -alpha / a0; a1 = -2 * std::cos(w) / a0; a2 = (1 - alpha) / a0;
    }
    float run(float x) { const float y = b0 * x + b2 * x2 - a1 * y1 - a2 * y2; x2 = x1; x1 = x; y2 = y1; y1 = y; return y; }
};

// enveloppe de 5 ms aux deux bouts
inline float fondu(size_t i, size_t n, float rate) {
    const float f = FONDU * rate, a = (float)i / f, r = (float)(n - 1 - i) / f;
    return std::clamp(std::min(a, r), 0.f, 1.f);
}

// noiseHit(dur, freq, q, vol, sweep) : bruit filtré passe-bande, fréquence en rampe exponentielle vers sweep,
// volume en rampe exponentielle de vol à 0,001 ; ajouté dans `out` à partir de l'échantillon `at`
inline void noiseHit(std::vector<float>& out, size_t at, float rate, float dur, float freq, float q, float vol, float sweep) {
    const size_t n = (size_t)(dur * rate);
    if (out.size() < at + n) out.resize(at + n, 0.f);
    Bruit br; PasseBande f;
    for (size_t i = 0; i < n; i++) {
        const float x = (float)i / (float)n;
        if (i % 16 == 0) f.regler(sweep > 0 ? freq * std::pow(sweep / freq, x) : freq, q, rate);
        const float g = vol * std::pow(0.001f / vol, x);
        out[at + i] += f.run(br.next()) * g * fondu(i, n, rate);
    }
}

// beep(freq, at, dur) : onde triangle, volume 0,12 en rampe exponentielle vers 0,001
inline void beep(std::vector<float>& out, float rate, float freq, float at, float dur) {
    const size_t a = (size_t)(at * rate), n = (size_t)(dur * rate);
    if (out.size() < a + n) out.resize(a + n, 0.f);
    double ph = 0;
    for (size_t i = 0; i < n; i++) {
        const float tri = (float)(4 * std::abs(ph - std::floor(ph + 0.5)) - 1);   // triangle −1..1
        const float g = 0.12f * std::pow(0.001f / 0.12f, (float)i / (float)n);
        out[a + i] += tri * g * fondu(i, n, rate);
        ph += freq / rate;
    }
}

// boucle continue (crissement, souffle de nitro) : bruit filtré, fin fondue dans le début (raccord sans clic)
inline std::vector<float> boucle(float rate, float dur, float freq, float q, float vol, float freq2 = 0, float q2 = 0) {
    const size_t n = (size_t)(dur * rate), x = (size_t)(0.05f * rate);
    std::vector<float> tmp(n + x);
    Bruit br; br.s = 0x1234567u; PasseBande f, f2;
    f.regler(freq, q, rate); if (freq2 > 0) f2.regler(freq2, q2, rate);
    for (auto& v : tmp) { const float b = br.next(); v = (f.run(b) + (freq2 > 0 ? f2.run(b) : 0.f)) * vol; }
    std::vector<float> out(tmp.begin(), tmp.begin() + (long)n);
    for (size_t i = 0; i < x; i++) { const float k = (float)i / (float)x; out[i] = out[i] * k + tmp[n + i] * (1 - k); }
    return out;
}

// WFS-184 (RG-04, ANO-96) : lecture d'une boucle de bruit sans motif qui revient. À chaque tour, la lecture repart d'un point
// tiré au hasard dans la boucle ; la fin du tour se fond dans le nouveau départ sur X échantillons (fondu à puissance
// constante : deux bruits indépendants, cos² + sin² = 1, pas de creux de volume au raccord). f : table du fondu,
// f[i] = sin(π/2 · i / X), i = 0..X (fonduBoucle, calculée une fois). Thread audio : aucune allocation, aucun verrou.
inline std::vector<float> fonduBoucle(float rate, float duree) {
    const size_t x = std::max<size_t>(1, (size_t)std::lround(duree * rate));
    std::vector<float> f(x + 1);
    for (size_t i = 0; i <= x; i++) f[i] = std::sin(1.57079632679f * (float)i / (float)x);
    return f;
}
struct LecteurAleatoire {
    size_t pos = 0, ancien = 0, k = (size_t)-1;   // k : pas du fondu en cours (-1 : aucun fondu)
    Bruit br;
    explicit LecteurAleatoire(uint32_t graine = 0x9E3779B9u) { br.s = graine ? graine : 1; }
    void reset() { pos = 0; k = (size_t)-1; }
    float lire(const std::vector<float>& b, const std::vector<float>& f) {
        const size_t n = b.size(), X = f.size() - 1;
        if (n < 2 * X + 2) { const float y = b[pos]; if (++pos >= n) pos = 0; return y; }   // boucle trop courte : lecture simple
        float y;
        if (k != (size_t)-1) {   // fin de l'ancien tour (qui s'éteint) + début du nouveau (qui monte)
            y = b[ancien] * f[X - k] + b[pos] * f[k];
            ancien++; pos++;
            if (++k >= X) k = (size_t)-1;
        } else {
            y = b[pos++];
        }
        if (k == (size_t)-1 && pos + X >= n) {   // fin du tour : nouveau départ au hasard dans [0, n − 2X]
            ancien = pos; k = 0;
            pos = (size_t)((double)(br.next() * 0.5f + 0.5f) * (double)(n - 2 * X));
            if (pos > n - 2 * X) pos = n - 2 * X;
        }
        return y;
    }
};

// klaxon (WFS-76, rendu visible par WFS-121) : deux notes carrées adoucies (sol, si), volume 0,5 ; `distance` : passe-bande
// 900 Hz (klaxon lointain de l'ambiance de la ville) ; sans : klaxon d'une voiture du trafic toute proche
inline void klaxon(std::vector<float>& b, float rate, float debut, float dur, bool distance = true) {
    constexpr float PI2 = 6.28318530718f;
    const size_t a = (size_t)(debut * rate), n = (size_t)(dur * rate);
    if (b.size() < a + n) b.resize(a + n, 0.f);
    PasseBande f; f.regler(900, 0.9f, rate);
    for (size_t i = 0; i < n; i++) {
        const float t = (float)i / rate, sq = std::tanh(3 * std::sin(PI2 * 392 * t)) + std::tanh(3 * std::sin(PI2 * 494 * t));
        b[a + i] += (distance ? f.run(sq) : sq) * 0.5f * fondu(i, n, rate) * std::min(1.f, t / 0.02f) * std::min(1.f, (dur - t) / 0.04f);
    }
}

} // namespace wfs::sfx
