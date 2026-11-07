// WaForSpeed — son des autres voitures (WFS-272) : banc d'essai PC (RG-09), écouté et regardé par la QA. Trois scènes de 10 s
// sur une ligne droite (avant = −z, droite = +x), calculées comme en jeu (Attribution, calculerCible, SonsVoitures au pas de
// 1/120 s, blocs de 400 échantillons) :
//   voitures-course5 : départ arrêté, un adversaire double par la gauche (≈ 3,5 s), un autre par la droite (≈ 7 s) ;
//   voitures-trafic  : le joueur roule à 70 km/h, croise une voiture à 90 km/h (file de gauche, en face, ≈ 4 s), puis double une
//                      voiture à 40 km/h (file de droite, ≈ 7,5 s) ;
//   voitures-police  : le joueur roule à 90 km/h, la police arrive de 200 m derrière à 180 km/h et double par la gauche (≈ 8 s).
// .wav : stéréo ; .png : niveaux gauche (cyan) et droite (orange) en dB en haut, hauteur de combustion de chaque voix en bas
// (échelle logarithmique 40 à 640 Hz) ; journal : écart gauche / droite au passage, hauteur jouée et hauteur de la formule.
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include "../../third_party/stb/stb_image_write.h"
#include "../core/log.h"
#include "../game/cars.h"
#include "voitures_sons.h"

namespace wfs::voitures {
namespace {

constexpr int R = 48000, PAS = R / 120;   // un pas du jeu = 400 échantillons

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
float dB(float x) { return 20.f * std::log10(std::max(x, 1e-6f)); }

struct Mobile { uint64_t id; float x, z0, vz, kmh; const char* car; uint8_t genre; };   // z(t) = z0 − vz·t (vz > 0 : vers l'avant)
struct Scene { const char* nom; float vJoueur0, aJoueur, vJoueurMax; std::vector<Mobile> m; };

void jouer(const std::string& out, const Scene& sc) {
    const size_t N = 10 * (size_t)R;
    SonsVoitures s; s.build((float)R);
    Attribution at;
    std::vector<float> G(N, 0.f), D(N, 0.f), bus(PAS);
    std::vector<std::array<float, VOIX>> hauteur;   // par pas
    std::vector<Candidat> cs;
    float zJ = 0, vJ = sc.vJoueur0;
    const vec3 avant{0, 0, -1}, droite{1, 0, 0};
    float pireEcart = 0, fJ = 0, fF = 0; double tPire = 0;
    for (size_t a = 0; a + PAS <= N; a += PAS) {
        const float t = (float)a / R, dt = 1.f / 120;
        vJ = std::min(sc.vJoueurMax, vJ + sc.aJoueur * dt); zJ -= vJ * dt;
        const vec3 ici{0, 1.2f, zJ}, vIci = avant * vJ;
        cs.clear();
        for (const Mobile& m : sc.m) {
            Candidat c; c.id = m.id; c.p = {m.x, 0.6f, m.z0 - m.vz * t}; c.v = avant * m.vz; c.kmh = m.kmh; c.car = &carById(m.car); c.genre = m.genre;
            cs.push_back(c);
        }
        at.update(dt, cs, ici);
        std::array<float, VOIX> h{};
        for (int v = 0; v < VOIX; v++) {
            const Cible c = calculerCible(at.voix(v), dt, ici, vIci, avant, droite);
            s.cible(v, c);
            if (at.voix(v).occupee) {
                // RG-05 : hauteur de la formule, recalculée ici sans lissage (vitesses projetées sur la droite voiture → joueur)
                const vec3 u = ici - at.voix(v).c.p; const float d = length(u);
                const float vs = dot(at.voix(v).c.v, u) / d, vl = dot(vIci, u) / d;
                const float formule = c.fCycle * (c.timbre == 2 ? 1.f : (float)CYLINDRES) * (VITESSE_SON - vl) / (VITESSE_SON - vs);
                const float jouee = c.fCycle * (c.timbre == 2 ? 1.f : (float)CYLINDRES) * c.doppler;
                const float ecart = std::abs(jouee / formule - 1);
                if (d > 4 && ecart > pireEcart) { pireEcart = ecart; tPire = t; fJ = jouee; fF = formule; }
            }
        }
        std::fill(bus.begin(), bus.end(), 0.f);
        s.mix(G.data() + a, D.data() + a, bus.data(), PAS, 0.5f, true);
        for (int v = 0; v < VOIX; v++) h[(size_t)v] = s.frequenceJouee(v);
        hauteur.push_back(h);
    }
    const std::string nom = std::string("voitures-") + sc.nom;
    wavStereo(out + "/" + nom + ".wav", G, D);
    // image : niveaux (50 ms) en haut, hauteurs en bas
    const int W = 1000, H = 400, HN = 250;
    std::vector<uint8_t> img((size_t)W * H * 3, 18);
    auto px = [&](int x, int y, std::array<uint8_t, 3> k) {
        if (x < 0 || y < 0 || x >= W || y >= H) return;
        uint8_t* p = &img[((size_t)y * W + x) * 3]; p[0] = k[0]; p[1] = k[1]; p[2] = k[2];
    };
    for (int d = 0; d <= 60; d += 10) for (int x = 0; x < W; x += 2) px(x, (int)(d / 60.f * (HN - 1)), {60, 60, 70});
    for (int sec = 0; sec <= 10; sec++) for (int y = 0; y < H; y += 3) px(sec * (W - 1) / 10, y, {60, 60, 70});
    for (int x = 0; x < W; x++) px(x, HN + 2, {90, 90, 100});
    const size_t w = R / 20;
    float maxEcartLR = 0; double tLR = 0;
    for (size_t a = 0; a + w <= N; a += w) {
        double eg = 0, ed = 0; for (size_t i = a; i < a + w; i++) { eg += (double)G[i] * G[i]; ed += (double)D[i] * D[i]; }
        const float lg = dB((float)std::sqrt(eg / (double)w)), ld = dB((float)std::sqrt(ed / (double)w));
        const int x = (int)(a * (W - 1) / N);
        for (const auto& [l, k] : {std::pair{lg, std::array<uint8_t, 3>{0, 229, 255}}, std::pair{ld, std::array<uint8_t, 3>{255, 140, 0}}})
            if (l > -60) for (int dx = 0; dx < 5; dx++) for (int dy = -1; dy <= 1; dy++) px(x + dx, (int)(-l / 60.f * (HN - 1)) + dy, k);
        if (lg > -50 || ld > -50) if (std::abs(lg - ld) > std::abs(maxEcartLR)) { maxEcartLR = lg - ld; tLR = (double)a / R; }
    }
    static const std::array<uint8_t, 3> COUL[VOIX] = {{240, 240, 240}, {255, 60, 200}, {120, 255, 120}, {255, 230, 60},
                                                      {120, 160, 255}, {255, 120, 120}, {180, 120, 255}, {120, 255, 230}};
    for (size_t k = 0; k < hauteur.size(); k++)
        for (int v = 0; v < VOIX; v++) {
            const float f = hauteur[k][(size_t)v];
            if (f < 40) continue;
            const int x = (int)(k * (W - 1) / hauteur.size()), y = H - 1 - (int)(std::log2(std::min(f, 640.f) / 40.f) / 4.f * (H - HN - 6));
            px(x, y, COUL[v]); px(x, y + 1, COUL[v]);
        }
    stbi_write_png((out + "/" + nom + ".png").c_str(), W, H, 3, img.data(), W * 3);
    WFS_LOG("son (WFS-272) : %s.wav : plus grand écart gauche − droite %.1f dB (t = %.1f s) ; Doppler : pire écart à la formule %.2f %% "
            "(t = %.2f s, %.1f Hz joués, %.1f Hz attendus ; lissage de 12 ms compris)", nom.c_str(), maxEcartLR, tLR, pireEcart * 100, tPire, fJ, fF);
}

} // namespace

void benchVoitures(const std::string& out) {
    // adversaires : 130 → 180 km/h derrière le joueur qui part arrêté (4 m/s²)
    jouer(out, {"course5", 0.f, 4.f, 45.f, {
        {1, -3.5f, 100.f, 130 / 3.6f, 130, "m4", ADVERSAIRE},     // à gauche, 100 m derrière : double vers 3,5 s
        {2, 3.5f, 190.f, 150 / 3.6f, 150, "giulia", ADVERSAIRE},  // à droite, 190 m derrière : double vers 7 s
        {3, 0.f, -60.f, 120 / 3.6f, 120, "golf", ADVERSAIRE},    // devant
        {4, -3.5f, -120.f, 125 / 3.6f, 125, "s3", ADVERSAIRE}}});
    // trafic : le joueur à 70 km/h ; une voiture en face à 90 km/h (vz < 0) ; une voiture lente devant, à droite
    jouer(out, {"trafic", 70 / 3.6f, 0.f, 70 / 3.6f, {
        {11, -3.5f, -180.f, -90 / 3.6f, 90, "golf", TRAFIC},
        {12, 3.5f, -62.f, 40 / 3.6f, 40, "p208", TRAFIC}}});   // doublée vers 7,5 s
    // police : le joueur à 90 km/h ; la police à 180 km/h, 200 m derrière, file de gauche
    jouer(out, {"police", 90 / 3.6f, 0.f, 90 / 3.6f, {{21, -3.5f, 200.f, 180 / 3.6f, 180, "p208", POLICE}}});
    {   // RG-08 : coût de 8 voix à portée, au pas d'un rappel de 10 ms
        SonsVoitures s; s.build((float)R);
        Attribution at; std::vector<Candidat> cs;
        for (int i = 0; i < VOIX; i++) { Candidat c; c.id = (uint64_t)i + 1; c.p = {(float)(i - 4) * 3, 0.6f, -10.f * (float)i}; c.v = {0, 0, -30}; c.kmh = 108; c.car = &CARS[i]; c.genre = i % 2 ? TRAFIC : ADVERSAIRE; cs.push_back(c); }
        for (int k = 0; k < 40; k++) at.update(1.f / 120, cs, {0, 1.2f, 0});
        for (int v = 0; v < VOIX; v++) s.cible(v, calculerCible(at.voix(v), 1.f / 120, {0, 1.2f, 0}, {0, 0, -20}, {0, 0, -1}, {1, 0, 0}));
        std::vector<float> G(480), D(480), bus(480);
        const int B = 2000;
        const auto t0 = std::chrono::steady_clock::now();
        for (int b = 0; b < B; b++) { std::fill(G.begin(), G.end(), 0.f); std::fill(D.begin(), D.end(), 0.f); std::fill(bus.begin(), bus.end(), 0.f); s.mix(G.data(), D.data(), bus.data(), 480, 0.5f, true); }
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / B;
        WFS_LOG("son (WFS-272) : coût du son des voitures : %.3f ms par bloc de 10 ms (%d voix ; ≤ 0,3 ms)", ms, at.actives());
    }
}

} // namespace wfs::voitures
