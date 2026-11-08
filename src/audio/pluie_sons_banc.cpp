// WaForSpeed — son de la pluie (WFS-288) : banc d'essai PC (RG-10), écouté et regardé par la QA. Scènes calculées comme en jeu
// (SonsPluie::pas au pas de 1/120 s, blocs de 400 échantillons), le son du moteur du banc (moteur.wav) dessous :
//   pluie-arriere : vue extérieure ; arrêt 5 s, puis 50 km/h (5 à 8,5 s) et 130 km/h (8,5 à 12 s) ;
//   pluie-cockpit : même trajet en vue Cockpit (Golf) ; essuie-glaces à 45 puis 65 balayages par minute (au-delà de 100 km/h) ;
//   pluie-tunnel  : 50 km/h, vue extérieure ; tunnel de la Ville de 3 à 7 s (pluie coupée en 0,3 s, reprise en 0,5 s) ;
// .wav : stéréo (moteur + pluie) ; .png : en haut, niveaux de la pluie seule à gauche (cyan) et à droite (orange) et du moteur
// (gris), en dB (0 à −70) ; en bas, spectre de la pluie (gauche, tiers d'octave, −90 à −40 dB) de 100 Hz à 16 kHz (repères 100 Hz,
// 1 kHz, 10 kHz), une courbe par partie de la scène (blanc, jaune, rouge).
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include "../../third_party/stb/stb_image_write.h"
#include "../core/log.h"
#include "../game/cars.h"
#include "pluie_sons.h"

namespace wfs::pluie {
namespace {

constexpr int R = 48000, PAS = R / 120;
constexpr float MASTER = 0.5f;   // sfx_.master du jeu

void wavStereo(const std::string& path, const std::vector<float>& G, const std::vector<float>& D) {
    std::ofstream f(path, std::ios::binary);
    auto u32 = [&](uint32_t x) { f.write((const char*)&x, 4); };
    auto u16 = [&](uint16_t x) { f.write((const char*)&x, 2); };
    const uint32_t bytes = (uint32_t)G.size() * 4;
    f.write("RIFF", 4); u32(36 + bytes); f.write("WAVEfmt ", 8); u32(16); u16(1); u16(2); u32(R); u32(R * 4); u16(4); u16(16);
    f.write("data", 4); u32(bytes);
    for (size_t i = 0; i < G.size(); i++)
        for (float x : {G[i], D[i]}) { const int16_t s = (int16_t)std::lround(std::clamp(x, -1.f, 1.f) * 32767); f.write((const char*)&s, 2); }
}
float dB(double x) { return 20.f * (float)std::log10(std::max(x, 1e-9)); }

// niveau RMS d'une bande d'un tiers d'octave autour de f (passe-bande d'ordre 2, Q = 4,3) sur [a, a + n)
float spectre(const std::vector<float>& x, size_t a, size_t n, float f) {
    const float w = 6.28318530718f * f / R, c = std::cos(w), al = std::sin(w) / (2 * 4.3f), a0 = 1 + al;
    const float b0 = al / a0, b2 = -al / a0, a1 = -2 * c / a0, a2 = (1 - al) / a0;
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0; double s = 0;
    for (size_t i = a; i < a + n; i++) { const float y = b0 * x[i] + b2 * x2 - a1 * y1 - a2 * y2; x2 = x1; x1 = x[i]; y2 = y1; y1 = y; s += (double)y * y; }
    return (float)std::sqrt(s / (double)n);
}

struct Instant { float kmh; bool cockpit, tunnel; };

void image(const std::string& path, const std::vector<float>& G, const std::vector<float>& D, const std::vector<float>& m,
           const std::vector<std::pair<float, float>>& parties) {
    const int W = 1000, H = 400, HN = 230;
    std::vector<uint8_t> img((size_t)W * H * 3, 18);
    auto px = [&](int x, int y, std::array<uint8_t, 3> k) {
        if (x < 0 || y < 0 || x >= W || y >= H) return;
        uint8_t* p = &img[((size_t)y * W + x) * 3]; p[0] = k[0]; p[1] = k[1]; p[2] = k[2];
    };
    for (int d = 0; d <= 70; d += 10) for (int x = 0; x < W; x += 2) px(x, (int)(d / 70.f * (HN - 1)), {60, 60, 70});
    for (int s = 0; s <= 12; s++) for (int y = 0; y < HN; y += 3) px(s * (W - 1) / 12, y, {60, 60, 70});
    for (int x = 0; x < W; x++) px(x, HN + 2, {90, 90, 100});
    const size_t w = R / 20, N = G.size();
    for (size_t a = 0; a + w <= N; a += w) {   // niveaux (0 à −70 dB)
        double eg = 0, ed = 0, em = 0;
        for (size_t i = a; i < a + w; i++) { eg += (double)G[i] * G[i]; ed += (double)D[i] * D[i]; em += (double)m[i] * m[i]; }
        const int x = (int)(a * (W - 1) / N);
        for (const auto& [l, k] : {std::pair{dB(std::sqrt(em / (double)w)), std::array<uint8_t, 3>{130, 130, 130}},
                                   std::pair{dB(std::sqrt(eg / (double)w)), std::array<uint8_t, 3>{0, 229, 255}},
                                   std::pair{dB(std::sqrt(ed / (double)w)), std::array<uint8_t, 3>{255, 140, 0}}})
            if (l > -70) for (int dx = 0; dx < 4; dx++) for (int dy = -1; dy <= 1; dy++) px(x + dx, (int)(-l / 70.f * (HN - 1)) + dy, k);
    }
    // spectre (bas) : tiers d'octave de 100 Hz à 16 kHz en échelle logarithmique, −90 à −40 dB
    const std::array<std::array<uint8_t, 3>, 3> couleurs = {{{240, 240, 240}, {255, 220, 0}, {255, 60, 60}}};
    for (int dec : {100, 1000, 10000}) { const int x = (int)(std::log10(dec / 100.f) / std::log10(160.f) * (W - 1)); for (int y = HN + 4; y < H; y += 3) px(x, y, {70, 70, 80}); }
    for (size_t p = 0; p < parties.size() && p < couleurs.size(); p++) {
        const size_t a = (size_t)(parties[p].first * R), n = std::min((size_t)(parties[p].second * R) - a, (size_t)(R / 2));
        int yAvant = -1;
        for (int x = 0; x < W; x += 10) {
            const float f = 100.f * std::pow(160.f, (float)x / (W - 1));
            const float l = dB(spectre(G, a, n, f));
            const int y = H - 2 - (int)(std::clamp((l + 90) / 50.f, 0.f, 1.f) * (H - HN - 8));
            if (yAvant >= 0) for (int dx = -10; dx <= 0; dx++) { const int yy = yAvant + (y - yAvant) * (dx + 10) / 10; px(x + dx, yy, couleurs[p]); px(x + dx, yy + 1, couleurs[p]); }
            yAvant = y;
        }
    }
    stbi_write_png(path.c_str(), W, H, 3, img.data(), W * 3);
}

void scene(const std::string& out, const char* nom, const std::vector<float>& moteur, float duree, Instant (*instant)(float),
           const std::vector<std::pair<float, float>>& parties) {
    const CarDef& car = carById("golf");
    const Surfaces s = surfaces(car.d.L, car.d.W, car.d.H, car.d.wb);
    SonsPluie p; p.build((float)R); p.active(true);
    const size_t N = (size_t)(duree * R);
    std::vector<float> G(N, 0.f), D(N, 0.f), m(N);
    for (size_t i = 0; i < N; i++) m[i] = moteur.empty() ? 0.f : moteur[(i + moteur.size() / 3) % moteur.size()];
    uint64_t toit0 = 0, pb0 = 0; float tPart = 0; std::vector<float> cadences;
    for (size_t a = 0; a + PAS <= N; a += PAS) {
        const Instant it = instant((float)a / R);
        p.pas(1.f / 120, it.kmh, true, it.cockpit, it.cockpit, it.tunnel, s);
        p.mix(G.data() + a, D.data() + a, PAS, MASTER);
        tPart += 1.f / 120;
        if (tPart >= 1) { cadences.push_back((float)((p.impactsToit() - toit0) + (p.impactsPareBrise() - pb0))); toit0 = p.impactsToit(); pb0 = p.impactsPareBrise(); tPart = 0; }
    }
    std::vector<float> sG(N), sD(N);
    for (size_t i = 0; i < N; i++) { sG[i] = m[i] + G[i]; sD[i] = m[i] + D[i]; }
    wavStereo(out + "/" + nom + ".wav", sG, sD);
    image(out + "/" + nom + ".png", G, D, m, parties);
    float pk = 0, pkM = 0; double e2 = 0, e2m = 0;
    for (size_t i = 0; i < N; i++) { pk = std::max(pk, std::max(std::fabs(G[i]), std::fabs(D[i]))); pkM = std::max(pkM, std::fabs(m[i])); e2 += (double)G[i] * G[i]; e2m += (double)m[i] * m[i]; }
    char parts[200]; int k = 0;
    for (const auto& [a, b] : parties) {
        double e = 0; const size_t i0 = (size_t)(a * R), i1 = std::min(N, (size_t)(b * R));
        for (size_t i = i0; i < i1; i++) e += (double)G[i] * G[i];
        k += std::snprintf(parts + k, sizeof parts - (size_t)k, "%s%.1f dB (%.1f-%.1f s)", k ? ", " : "", dB(std::sqrt(e / (double)std::max<size_t>(1, i1 - i0))), a, b);
        if (k >= (int)sizeof parts) break;
    }
    WFS_LOG("son (WFS-288) : %s.wav : pluie seule %s (RMS gauche) ; crête %.1f dB, moteur du banc %.1f dB de crête (%.1f dB d'écart) ; "
            "%llu impacts sur le toit, %llu sur le pare-brise, %llu gouttes isolées, %llu bulles, %llu chocs des balais",
            nom, parts, dB(pk), dB(pkM), dB(pkM) - dB(pk), (unsigned long long)p.impactsToit(), (unsigned long long)p.impactsPareBrise(),
            (unsigned long long)p.gouttes(), (unsigned long long)p.bulles(), (unsigned long long)p.chocs());
}

} // namespace

void benchPluie(const std::string& out, const std::vector<float>& moteur) {
    const Reference r = reference();
    const CarDef& car = carById("golf");
    const Surfaces s = surfaces(car.d.L, car.d.W, car.d.H, car.d.wb);
    WFS_LOG("son (WFS-288) : pluie de référence %.0f mm/h : Λ = %.3f mm⁻¹, %.0f gouttes/m³ (D ≥ 0,5 mm), %.0f impacts/m²/s dont %.1f %% de grosses "
            "(D ≥ 2 mm, %.1f/m²/s), bulles %.0f/m²/s ; pluie recalculée %.2f mm/h", PLUIE_MMH, r.lambda, r.concentration, r.flux, 100 * r.partGrosses,
            r.fluxGrosses, r.fluxBulles, r.tauxMmh);
    const float f0 = fluxPareBrise(r, 0, s.pente), f50 = fluxPareBrise(r, 50, s.pente), f130 = fluxPareBrise(r, 130, s.pente);
    WFS_LOG("son (WFS-288) : Golf : toit %.2f m², pare-brise %.2f m² incliné de %.1f° ; pare-brise %.0f / %.0f / %.0f impacts/m²/s à 0 / 50 / 130 km/h "
            "(rapport 1 / %.2f / %.2f) ; toit %.0f impacts/s à toute vitesse", s.toit, s.pareBrise, std::atan(s.pente) * 57.2958f, f0, f50, f130,
            f50 / f0, f130 / f0, r.flux * s.toit);
    scene(out, "pluie-arriere", moteur, 12, [](float t) -> Instant { return {t < 5 ? 0.f : t < 8.5f ? 50.f : 130.f, false, false}; },
          {{1, 5}, {5.5f, 8.5f}, {9, 12}});
    scene(out, "pluie-cockpit", moteur, 12, [](float t) -> Instant { return {t < 5 ? 0.f : t < 8.5f ? 50.f : 130.f, true, false}; },
          {{1, 5}, {5.5f, 8.5f}, {9, 12}});
    scene(out, "pluie-tunnel", moteur, 10, [](float t) -> Instant { return {50.f, false, t >= 3 && t < 7}; }, {{0.5f, 3}, {3.5f, 7}, {7.5f, 10}});
    {   // RG-09 : coût, vue Cockpit (la plus chère : averse étouffée, tambourinage, essuie-glaces), 130 km/h
        SonsPluie p; p.build((float)R); p.active(true);
        std::vector<float> G(480), D(480);
        for (int k = 0; k < 60; k++) { p.pas(1.f / 120, 130, true, true, true, false, s); p.mix(G.data(), D.data(), 400, MASTER); }
        const auto t0 = std::chrono::steady_clock::now();
        for (int b = 0; b < 2000; b++) { p.pas(1.f / 120, 130, true, true, true, false, s); p.mix(G.data(), D.data(), 480, MASTER); }
        WFS_LOG("son (WFS-288) : coût de la pluie : %.3f ms par bloc de 10 ms (vue Cockpit, 130 km/h ; ≤ 0,1 ms), aucun tampon de son",
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / 2000);
    }
}

} // namespace wfs::pluie
