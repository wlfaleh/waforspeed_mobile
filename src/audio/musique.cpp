// WaForSpeed — musique et ambiances sonores par carte (WFS-76) : voir musique.h.
#include "musique.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include "../../third_party/stb/stb_image_write.h"
#include "../core/log.h"
#include "sfx_synth.h"

namespace wfs {
namespace {

using sfx::Bruit;
using sfx::PasseBande;
constexpr float PI2 = 6.28318530718f;

float hz(int midi) { return 440.f * std::pow(2.f, (midi - 69) / 12.f); }

// passe-bas à un pôle (pente douce) ; deux à la suite pour la basse et la nappe
struct PasseBas {
    float a = 0, y = 0;
    void regler(float fc, float rate) { a = 1 - std::exp(-PI2 * fc / rate); }
    float run(float x) { y += a * (x - y); return y; }
};

// tampon « en anneau » : ce qui dépasse la fin retombe au début (boucle parfaite, aucune coupure)
struct Anneau {
    std::vector<float>& b;
    float& operator[](int64_t i) { const int64_t n = (int64_t)b.size(); return b[(size_t)(((i % n) + n) % n)]; }
};

void normaliser(std::vector<float>& b, float crete) {
    float p = 0;
    for (float x : b) p = std::max(p, std::abs(x));
    if (p > 1e-6f) for (float& x : b) x *= crete / p;
}

// ---------------------------------------------------------------- grille musicale (4 mesures, une par accord)
struct Accord { int basse; int notes[3]; };
const Accord GRILLE[4] = {
    {45, {57, 60, 64}},   // La mineur
    {41, {57, 60, 65}},   // Fa majeur (renversement proche)
    {48, {55, 60, 64}},   // Do majeur
    {43, {55, 59, 62}},   // Sol majeur
};
const int ARPEGE[8] = {0, 12, 7, 12, 0, 12, 7, 19};   // basse en croches : fondamentale, octave, quinte…

// basse : dents de scie filtrée passe-bas, enveloppe courte par croche
void basse(Anneau b, float rate, double debut, float freq, float dur, float coupure, float vol) {
    const int64_t a = (int64_t)(debut * rate), n = (int64_t)((dur + 0.03f) * rate);
    PasseBas f1, f2; double ph = 0;
    for (int64_t i = 0; i < n; i++) {
        const float t = (float)i / rate;
        const float env = std::min(1.f, t / 0.004f) * std::exp(-t * 5.f) * std::clamp((dur + 0.03f - t) / 0.03f, 0.f, 1.f);
        f1.regler(coupure * (0.6f + 1.4f * std::exp(-t * 12.f)), rate); f2.a = f1.a;
        const float saw = (float)(2 * (ph - std::floor(ph + 0.5)));
        b[a + i] += f2.run(f1.run(saw)) * env * vol;
        ph += freq / rate;
    }
}

// nappe : deux dents de scie désaccordées par note, attaque lente, relâche qui déborde sur la mesure suivante
void nappe(Anneau b, float rate, double debut, const int* notes, float dur, float vol) {
    const float rel = 0.8f;
    const int64_t a = (int64_t)(debut * rate), n = (int64_t)((dur + rel) * rate);
    for (int k = 0; k < 3; k++) for (float d : {-0.004f, 0.004f}) {
        const float f = hz(notes[k]) * (1 + d);
        PasseBas f1, f2; f1.regler(1100, rate); f2.regler(1100, rate);
        double ph = 0.37 * k;
        for (int64_t i = 0; i < n; i++) {
            const float t = (float)i / rate;
            const float env = std::min(1.f, t / 0.6f) * (t > dur ? std::max(0.f, 1 - (t - dur) / rel) : 1.f);
            b[a + i] += f2.run(f1.run((float)(2 * (ph - std::floor(ph + 0.5))))) * env * vol;
            ph += f / rate;
        }
    }
}

void grosseCaisse(Anneau b, float rate, double debut) {
    const int64_t a = (int64_t)(debut * rate), n = (int64_t)(0.35f * rate);
    double ph = 0;
    for (int64_t i = 0; i < n; i++) {
        const float t = (float)i / rate, f = 45 + 75 * std::exp(-t * 30);
        b[a + i] += std::sin((float)(PI2 * ph)) * std::exp(-t * 9) * std::min(1.f, t / 0.002f) * 0.9f;
        ph += f / rate;
    }
}

void caisseClaire(Anneau b, float rate, double debut, Bruit& br) {
    const int64_t a = (int64_t)(debut * rate), n = (int64_t)(0.22f * rate);
    PasseBande f; f.regler(1800, 0.7f, rate);
    for (int64_t i = 0; i < n; i++) {
        const float t = (float)i / rate;
        b[a + i] += (f.run(br.next()) * 1.4f * std::exp(-t * 18) + std::sin(PI2 * 190 * t) * 0.3f * std::exp(-t * 25)) * std::min(1.f, t / 0.001f);
    }
}

void charleston(Anneau b, float rate, double debut, Bruit& br) {
    const int64_t a = (int64_t)(debut * rate), n = (int64_t)(0.05f * rate);
    PasseBande f; f.regler(8000, 1.2f, rate);
    for (int64_t i = 0; i < n; i++) { const float t = (float)i / rate; b[a + i] += f.run(br.next()) * 0.35f * std::exp(-t * 70); }
}

// ---------------------------------------------------------------- ambiances
// bruit filtré de `secondes`, fin fondue dans le début (boucle sans coupure) ; filtre(i, bruit) donne l'échantillon
template <class F>
std::vector<float> fondBoucle(float rate, float secondes, F filtre) {
    const size_t n = (size_t)(secondes * rate), x = (size_t)(0.25f * rate);
    std::vector<float> tmp(n + x);
    Bruit br; br.s = 0x7F4A7C15u;
    for (size_t i = 0; i < n + x; i++) tmp[i] = filtre(i % n, br.next());
    std::vector<float> out(tmp.begin(), tmp.begin() + (long)n);
    for (size_t i = 0; i < x; i++) { const float k = (float)i / (float)x; out[i] = out[i] * k + tmp[n + i] * (1 - k); }
    return out;
}

// glissando en onde triangle (mouette, oiseau) de f0 à f1, enveloppe en cloche
void glisse(std::vector<float>& b, float rate, float debut, float dur, float f0, float f1, float vol) {
    const size_t a = (size_t)(debut * rate), n = (size_t)(dur * rate);
    if (b.size() < a + n) b.resize(a + n, 0.f);
    double ph = 0;
    for (size_t i = 0; i < n; i++) {
        const float x = (float)i / (float)n, f = f0 * std::pow(f1 / f0, x);
        const float tri = (float)(4 * std::abs(ph - std::floor(ph + 0.5)) - 1);
        b[a + i] += tri * vol * std::sin(3.14159265f * x);
        ph += f / rate;
    }
}

// klaxon lointain : sfx::klaxon (sfx_synth.h, WFS-121 : partagé avec le klaxon du trafic), avec le filtre de distance
using sfx::klaxon;

void forme(const std::string& path, const std::vector<float>& pcm, size_t boucle) {
    const int W = 1200, H = 240, cy = H / 2;
    std::vector<uint8_t> img((size_t)W * H * 3, 18);
    auto px = [&](int x, int y, uint8_t r, uint8_t g, uint8_t bb) {
        if (x < 0 || y < 0 || x >= W || y >= H) return;
        uint8_t* p = &img[((size_t)y * W + x) * 3]; p[0] = r; p[1] = g; p[2] = bb;
    };
    for (int x = 0; x < W; x++) { px(x, cy - 100, 90, 30, 40); px(x, cy + 100, 90, 30, 40); }   // ±1,0 (saturation)
    for (int x = 0; x < W; x++) {
        const size_t a = pcm.size() * x / W, e = std::max(a + 1, pcm.size() * (x + 1) / W);
        float lo = 0, hi = 0;
        for (size_t i = a; i < e && i < pcm.size(); i++) { lo = std::min(lo, pcm[i]); hi = std::max(hi, pcm[i]); }
        for (int y = (int)(cy - hi * 100); y <= (int)(cy - lo * 100); y++) px(x, y, 0, 229, 255);
    }
    if (boucle) for (size_t k = boucle; k < pcm.size(); k += boucle) {   // raccord de la boucle (orange)
        const int x = (int)(k * W / pcm.size());
        for (int y = 0; y < H; y += 2) px(x, y, 255, 106, 0);
    }
    stbi_write_png(path.c_str(), W, H, 3, img.data(), W * 3);
}

void wav(const std::string& path, const std::vector<float>& pcm, int rate) {
    std::ofstream f(path, std::ios::binary);
    auto u32 = [&](uint32_t x) { f.write((const char*)&x, 4); };
    auto u16 = [&](uint16_t x) { f.write((const char*)&x, 2); };
    const uint32_t bytes = (uint32_t)pcm.size() * 2;
    f.write("RIFF", 4); u32(36 + bytes); f.write("WAVEfmt ", 8); u32(16); u16(1); u16(1); u32(rate); u32(rate * 2); u16(2); u16(16);
    f.write("data", 4); u32(bytes);
    for (float x : pcm) { const int16_t s = (int16_t)std::lround(std::clamp(x, -1.f, 1.f) * 32767); f.write((const char*)&s, 2); }
}

void mesurer(const char* nom, const std::vector<float>& pcm, size_t boucle) {
    float crete = 0, saut = 0;
    for (size_t i = 0; i < pcm.size(); i++) {
        crete = std::max(crete, std::abs(pcm[i]));
        if (i) saut = std::max(saut, std::abs(pcm[i] - pcm[i - 1]));
    }
    const float raccord = boucle && boucle < pcm.size() ? std::abs(pcm[boucle] - pcm[boucle - 1]) : 0.f;
    WFS_LOG("son %s : %.1f s, crête %.2f (%s 1,0), plus grand saut %.3f, saut au raccord de la boucle %.3f", nom,
            pcm.size() / 48000.0, crete, crete < 1 ? "sous" : "ATTEINT", saut, raccord);
}

} // namespace

// ---------------------------------------------------------------- musique
void Musique::build(float rate) {
    if (pret_.load() && rate_ == rate) return;   // déjà calculée pour cette fréquence (gardée d'une partie à l'autre)
    pret_ = false;
    const double temps = 60.0 / 110.0, mesure = 4 * temps;   // RG-01 : 110 BPM, 4 temps par mesure, 4 mesures
    const size_t n = (size_t)std::lround(4 * mesure * rate);
    std::vector<float> basses(n, 0.f), nappes(n, 0.f), batterie(n, 0.f);
    Bruit br; br.s = 0x51ED270Bu;
    for (int m = 0; m < 4; m++) {
        const Accord& a = GRILLE[m];
        const double t0 = m * mesure;
        for (int c = 0; c < 8; c++) basse({basses}, rate, t0 + c * temps / 2, hz(a.basse + ARPEGE[c]), (float)(temps / 2) * 0.9f, 700, 0.5f);
        nappe({nappes}, rate, t0, a.notes, (float)mesure, 0.08f);
        for (int tp = 0; tp < 4; tp++) {
            grosseCaisse({batterie}, rate, t0 + tp * temps);
            if (tp == 1 || tp == 3) caisseClaire({batterie}, rate, t0 + tp * temps, br);
            charleston({batterie}, rate, t0 + tp * temps, br); charleston({batterie}, rate, t0 + (tp + 0.5) * temps, br);
        }
    }
    menu_.assign(n, 0.f); course_.assign(n, 0.f);
    for (size_t i = 0; i < n; i++) {
        menu_[i] = basses[i] * 0.7f + nappes[i];                        // « menu » : sans batterie, plus douce
        course_[i] = basses[i] + nappes[i] * 0.8f + batterie[i] * 0.55f;   // « course » : avec batterie
    }
    normaliser(menu_, 0.6f); normaliser(course_, 0.9f);
    rate_ = rate; pos_ = 0;
    k_ = 1 - std::exp(-1 / (0.2f * rate));   // RG-02 : fondu de 0,8 s (≈ 4 constantes de temps)
    WFS_LOG("musique : boucle de %.2f s calculée (%zu échantillons par variante, %.1f Mo)", n / rate, n, 2.0 * n * 4 / 1e6);
    pret_.store(true, std::memory_order_release);
}

void Musique::mix(float* out, int n, float master) {
    if (!pret_.load(std::memory_order_acquire)) return;
    const int c = cible_.load();
    const float cm = c == 1 ? 1.f : 0.f, cc = c == 2 ? 1.f : 0.f, g = volume * master;
    const size_t len = menu_.size();
    for (int i = 0; i < n; i++) {
        gMenu_ += (cm - gMenu_) * k_; gCourse_ += (cc - gCourse_) * k_;
        if (gMenu_ > 1e-5f || gCourse_ > 1e-5f) out[i] = std::clamp(out[i] + (menu_[pos_] * gMenu_ + course_[pos_] * gCourse_) * g, -1.f, 1.f);
        if (++pos_ >= len) pos_ = 0;   // les deux variantes avancent ensemble : le fondu croisé reste en mesure
    }
}

Musique& musique() { static Musique m; return m; }

// ---------------------------------------------------------------- ambiance
void Ambiance::build(float rate, const std::string& carte, bool nuit) {
    pret_ = false;
    rate_ = rate; carte_ = carte; nuit_ = nuit; gainNuit_ = nuit ? 0.5f : 1.f;   // RG-04
    for (auto& e : evenements_) e.clear();
    nbEvenements_ = 0;
    const float N = 8 * rate;   // boucle de 8 s
    if (carte == "plage") {   // vagues : passe-bande 800 Hz, houle de 8 s
        PasseBande f; f.regler(800, 0.6f, rate); PasseBande f2; f2.regler(300, 0.5f, rate);
        fond_ = fondBoucle(rate, 8, [&](size_t i, float b) {
            const float h = std::sin(3.14159265f * (float)i / N);
            return (f.run(b) + f2.run(b) * 0.5f) * (0.2f + 0.8f * h * h);
        });
        if (!nuit) { glisse(evenements_[0], rate, 0, 0.28f, 1900, 1200, 0.5f); glisse(evenements_[0], rate, 0.38f, 0.3f, 1800, 1100, 0.45f); nbEvenements_ = 1; }
        evMin_ = 10; evMax_ = 22;
    } else if (carte == "campagne") {   // vent : passe-bande lent 300 Hz, rafales
        PasseBande f; f.regler(300, 0.6f, rate);
        fond_ = fondBoucle(rate, 8, [&](size_t i, float b) { return f.run(b) * (0.45f + 0.35f * std::sin(PI2 * (float)i / N) + 0.2f * std::sin(PI2 * 3 * (float)i / N)); });
        if (!nuit) {
            for (int k = 0; k < 3; k++) glisse(evenements_[0], rate, k * 0.12f, 0.07f, 3200, 4200, 0.35f);
            for (int k = 0; k < 2; k++) glisse(evenements_[1], rate, k * 0.2f, 0.11f, 4100, 2900, 0.3f);
            nbEvenements_ = 2;
        }
        evMin_ = 8; evMax_ = 18;
    } else if (carte == "circuit") {   // foule lointaine : passe-bande 1 kHz faible
        PasseBande f; f.regler(1000, 0.5f, rate); PasseBande f2; f2.regler(450, 0.7f, rate);
        fond_ = fondBoucle(rate, 8, [&](size_t i, float b) { return (f.run(b) + f2.run(b) * 0.6f) * (0.8f + 0.2f * std::sin(PI2 * 2 * (float)i / N)); });
    } else {   // ville (et carte inconnue) : rumeur grave, passe-bas 400 Hz, klaxon lointain rare
        PasseBas f1, f2; f1.regler(400, rate); f2.regler(400, rate); PasseBande f3; f3.regler(120, 0.5f, rate);
        fond_ = fondBoucle(rate, 8, [&](size_t i, float b) { return (f2.run(f1.run(b)) * 3 + f3.run(b) * 0.8f) * (0.85f + 0.15f * std::sin(PI2 * 2 * (float)i / N)); });
        klaxon(evenements_[0], rate, 0, 0.35f);
        klaxon(evenements_[1], rate, 0, 0.16f); klaxon(evenements_[1], rate, 0.24f, 0.3f);
        nbEvenements_ = 2;
        evMin_ = 12; evMax_ = 25;
        carte_ = "ville";
    }
    normaliser(fond_, 0.55f);   // fond + événement rare < 1,0 même au volume plein (banc)
    for (int e = 0; e < nbEvenements_; e++) normaliser(evenements_[e], 0.4f);
    pos_ = 0; evCourant_ = nullptr; evPos_ = 0;
    attente_ = (int64_t)(rate * evMin_);
    k_ = 1 - std::exp(-1 / (0.075f * rate));   // RG-05 : fondu de 0,3 s
    pret_.store(true, std::memory_order_release);
}

void Ambiance::mix(float* out, int n, float master) {
    if (!pret_.load(std::memory_order_acquire) || fond_.empty()) return;
    const bool on = actif_.load();
    const float cible = on ? 1.f : 0.f, gv = volume * master * gainNuit_;
    for (int i = 0; i < n; i++) {
        g_ += (cible - g_) * k_;
        float s = fond_[pos_];
        if (++pos_ >= fond_.size()) pos_ = 0;
        if (nbEvenements_ && on && !evCourant_ && --attente_ <= 0) {   // événement rare : tiré au hasard (xorshift, sans allocation)
            hasard_ ^= hasard_ << 13; hasard_ ^= hasard_ >> 17; hasard_ ^= hasard_ << 5;
            evCourant_ = &evenements_[hasard_ % (uint32_t)nbEvenements_]; evPos_ = 0;
            const float u = (float)(hasard_ >> 8) / 16777216.f;
            attente_ = (int64_t)(rate_ * (evMin_ + u * (evMax_ - evMin_)));
        }
        if (evCourant_) { s += (*evCourant_)[evPos_]; if (++evPos_ >= evCourant_->size()) evCourant_ = nullptr; }
        if (g_ > 1e-5f) out[i] = std::clamp(out[i] + s * g_ * gv, -1.f, 1.f);
    }
}

// ---------------------------------------------------------------- banc d'essai (RG-08)
void benchMusique(const std::string& out) {
    const int R = 48000;
    const size_t dix = 10 * (size_t)R;
    Musique& m = musique();
    m.build((float)R);
    for (Variante v : {Variante::Menu, Variante::Course}) {   // la boucle jouée telle quelle sur 10 s (raccord vers 8,73 s)
        const std::vector<float>& b = m.tampon(v);
        std::vector<float> pcm(dix);
        for (size_t i = 0; i < dix; i++) pcm[i] = b[i % b.size()];
        const std::string nom = std::string("musique-") + (v == Variante::Menu ? "menu" : "course");
        wav(out + "/" + nom + ".wav", pcm, R); forme(out + "/" + nom + ".png", pcm, b.size()); mesurer(nom.c_str(), pcm, b.size());
    }
    for (const char* carte : {"ville", "plage", "campagne", "circuit"}) {   // ambiance mélangée comme en course (événement à 3 s)
        Ambiance a; a.build((float)R, carte, false); a.volume = 1;
        a.active(true); a.premierEvenement(3);
        std::vector<float> pcm(dix, 0.f);
        a.mix(pcm.data(), (int)dix, 1);
        const std::string nom = std::string("ambiance-") + carte;
        wav(out + "/" + nom + ".wav", pcm, R); forme(out + "/" + nom + ".png", pcm, (size_t)(8 * R)); mesurer(nom.c_str(), pcm, (size_t)(8 * R));
    }
}

} // namespace wfs
