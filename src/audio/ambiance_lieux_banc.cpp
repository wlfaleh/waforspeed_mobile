// WaForSpeed — son des lieux (WFS-261) : banc d'essai PC (RG-08), écouté et regardé par la QA.
//   lieu-<source>.wav : 10 s en stéréo, la voiture passe à 50 km/h en ligne droite devant la source (à sa distance de passage
//                       réelle, à droite de la route), au plus près à 5 s ; lieu-<source>.png : niveaux gauche (cyan) et droite
//                       (orange) en dB, toutes les 50 ms ;
//   lieu-tunnel.wav   : le son du moteur du banc (moteur.wav) 3 s dehors, 4 s dans un tunnel, 3 s dehors ; lieu-tunnel.png :
//                       niveau du moteur (blanc) et de la réverbération (magenta), tunnel en gris ;
//   journal : niveaux et écarts gauche / droite à 2,5 s, 5 s et 7,5 s ; écart moteur / réverbération ; coût par bloc de 10 ms.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include "../../third_party/stb/stb_image_write.h"
#include "../core/log.h"
#include "ambiance_lieux.h"

namespace wfs::lieux {
namespace {

constexpr int R = 48000, BLOC = 480;   // blocs de 10 ms, comme un rappel audio
constexpr float VITESSE = 50.f / 3.6f; // 50 km/h

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

// niveau efficace (dB) par fenêtre de 50 ms
std::vector<float> niveaux(const std::vector<float>& b) {
    const size_t w = R / 20;
    std::vector<float> out;
    for (size_t a = 0; a + w <= b.size(); a += w) {
        double s = 0; for (size_t i = a; i < a + w; i++) s += (double)b[i] * b[i];
        out.push_back(dB((float)std::sqrt(s / (double)w)));
    }
    return out;
}

// courbes de niveau de −60 à 0 dB (lignes tous les 10 dB), une ligne verticale par seconde ; zone grisée [z0, z1) en s
void courbes(const std::string& path, const std::vector<std::vector<float>>& c, const std::vector<std::array<uint8_t, 3>>& coul,
             float z0 = -1, float z1 = -1) {
    const int W = 1000, H = 300;
    std::vector<uint8_t> img((size_t)W * H * 3, 18);
    auto px = [&](int x, int y, std::array<uint8_t, 3> k) {
        if (x < 0 || y < 0 || x >= W || y >= H) return;
        uint8_t* p = &img[((size_t)y * W + x) * 3]; p[0] = k[0]; p[1] = k[1]; p[2] = k[2];
    };
    const size_t n = c.empty() ? 1 : std::max<size_t>(1, c[0].size());
    const float T = n / 20.f;
    if (z0 >= 0) for (int x = (int)(z0 / T * W); x < (int)(z1 / T * W) && x < W; x++) for (int y = 0; y < H; y++) px(x, y, {45, 45, 52});
    for (int d = 0; d <= 60; d += 10) { const int y = (int)(d / 60.f * (H - 1)); for (int x = 0; x < W; x += 2) px(x, y, {60, 60, 70}); }
    for (int s = 0; s <= (int)T; s++) { const int x = (int)(s / T * (W - 1)); for (int y = 0; y < H; y += 3) px(x, y, {60, 60, 70}); }
    for (size_t k = 0; k < c.size(); k++)
        for (size_t i = 0; i < c[k].size(); i++) {
            if (c[k][i] <= -60.f) continue;   // silence : rien de tracé
            const int x = (int)(i * (W - 1) / n), y = (int)(std::clamp(-c[k][i], 0.f, 60.f) / 60.f * (H - 1));
            for (int dx = 0; dx < std::max(1, W / (int)n); dx++) for (int dy = -1; dy <= 1; dy++) px(x + dx, y + dy, coul[k]);
        }
    stbi_write_png(path.c_str(), W, H, 3, img.data(), W * 3);
}

} // namespace

void benchLieux(const std::string& out, const std::vector<float>& moteur) {
    const size_t N = 10 * (size_t)R;
    // distances de passage réelles (m) : anneau du giratoire, rivage de la plage, tribune de la grille, troupeau, terrasse
    const float ECART[NB_TYPES] = {14, 53, 22, 30, 12};
    for (int t = 0; t < NB_TYPES; t++) {
        SonsDesLieux s; s.build((float)R, false); s.active(true);
        if (t == FERME) s.premierAppel(4.f);   // vache (variante 0) au plus près
        std::vector<float> G(N, 0.f), D(N, 0.f);
        for (size_t a = 0; a < N; a += BLOC) {
            // la voiture roule vers l'avant ; la source est à ECART m à droite de la route, au point atteint à 5 s
            const float avance = VITESSE * ((float)a / R - 5.f), d = std::hypot(ECART[t], avance);
            s.cible(t, spatialiser(d, std::atan2(ECART[t], -avance), FICHES[t]), 0);
            s.mix(G.data() + a, D.data() + a, BLOC, 1.f);
        }
        const std::string nom = std::string("lieu-") + nomType(t);
        wavStereo(out + "/" + nom + ".wav", G, D);
        const std::vector<float> nG = niveaux(G), nD = niveaux(D);
        courbes(out + "/" + nom + ".png", {nG, nD}, {{{0, 229, 255}}, {{255, 140, 0}}});
        auto a = [&](float sec) {   // niveau efficace sur 1 s autour de sec (le son lui-même varie : houle, clameurs, appels)
            const size_t i0 = (size_t)((sec - 0.5f) * R), i1 = (size_t)((sec + 0.5f) * R);
            double eG = 0, eD = 0; for (size_t i = i0; i < i1; i++) { eG += (double)G[i] * G[i]; eD += (double)D[i] * D[i]; }
            return std::array<float, 2>{dB((float)std::sqrt(eG / (double)(i1 - i0))), dB((float)std::sqrt(eD / (double)(i1 - i0)))};
        };
        const auto x1 = a(2.5f), x2 = a(5.f), x3 = a(7.5f);
        WFS_LOG("son (WFS-261) : %s.wav : passage à %.0f m, à 50 km/h : G/D %.1f/%.1f dB (2,5 s) %.1f/%.1f dB (5 s) %.1f/%.1f dB (7,5 s)",
                nom.c_str(), ECART[t], x1[0], x1[1], x2[0], x2[1], x3[0], x3[1]);
    }
    {   // tunnel : moteur (moteur.wav du banc, ou dents de scie à 100 Hz s'il est vide), 3 s dehors, 4 s dedans, 3 s dehors
        std::vector<float> m(N);
        double ph = 0;
        for (size_t i = 0; i < N; i++) {
            if (!moteur.empty()) m[i] = moteur[(i + moteur.size() / 4) % moteur.size()];
            else { m[i] = 0.3f * (float)(2 * (ph - std::floor(ph + 0.5))); ph += 100.0 / R; }
        }
        Tunnel tu; tu.build((float)R);
        std::vector<float> wG(N, 0.f), wD(N, 0.f), G(N), D(N);
        for (size_t a = 0; a < N; a += BLOC) {
            const float t = (float)a / R;
            tu.dedans(t >= 3.f && t < 7.f);
            tu.process(m.data() + a, wG.data() + a, wD.data() + a, BLOC);
        }
        for (size_t i = 0; i < N; i++) { G[i] = m[i] + wG[i]; D[i] = m[i] + wD[i]; }
        wavStereo(out + "/lieu-tunnel.wav", G, D);
        const std::vector<float> nm = niveaux(m), nw = niveaux(wG);
        courbes(out + "/lieu-tunnel.png", {nm, nw}, {{{240, 240, 240}}, {{255, 60, 200}}}, 3.f, 7.f);
        double em = 0, ew = 0;
        for (size_t i = 4 * (size_t)R; i < 6 * (size_t)R; i++) { em += (double)m[i] * m[i]; ew += (double)wG[i] * wG[i] + (double)wD[i] * wD[i]; }
        const float ecart = 10.f * std::log10((float)(em / std::max(ew / 2, 1e-12)));
        float queue = -120; for (size_t i = (size_t)(7.5f * R); i < 8 * (size_t)R; i++) queue = std::max(queue, dB(std::abs(wG[i])));
        WFS_LOG("son (WFS-261) : lieu-tunnel.wav : 1re réflexion %.1f ms (tube de %.0f m), moteur %.1f dB au-dessus de la réverbération "
                "(≥ 6), fondus de %.1f s ; 0,5 s après la sortie : crête de l'écho %.0f dB", tu.delai() * 1000, LARGEUR_TUNNEL, ecart, FONDU_TUNNEL, queue);
    }
    {   // RG-07 : coût du thread audio, 5 sources à portée et tunnel
        SonsDesLieux s; s.build((float)R, false); s.active(true);
        for (int t = 0; t < NB_TYPES; t++) s.cible(t, spatialiser(10, 0.7f, FICHES[t]), 1);
        Tunnel tu; tu.build((float)R); tu.dedans(true);
        std::vector<float> in(BLOC, 0.1f), G(BLOC), D(BLOC);
        const int B = 2000;
        const auto t0 = std::chrono::steady_clock::now();
        for (int b = 0; b < B; b++) {
            std::fill(G.begin(), G.end(), 0.f); std::fill(D.begin(), D.end(), 0.f);
            s.mix(G.data(), D.data(), BLOC, 1.f); tu.process(in.data(), G.data(), D.data(), BLOC);
        }
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / B;
        WFS_LOG("son (WFS-261) : coût du son des lieux : %.3f ms par bloc de 10 ms (5 sources + tunnel ; ≤ 0,3 ms)", ms);
    }
}

} // namespace wfs::lieux
