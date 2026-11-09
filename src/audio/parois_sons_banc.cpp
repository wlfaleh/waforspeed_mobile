// WaForSpeed — acoustique des lieux (WFS-275) : banc d'essai PC (RG-09), écouté et regardé par la QA. Scènes de 10 s calculées comme
// en jeu (parois::Suivi et parois::Echos au pas de 1/120 s, blocs de 400 échantillons), le son du moteur du banc (moteur.wav) en entrée :
//   parois-ville   : 50 km/h ; rue bordée d'immeubles des deux côtés (façades à RW + 6,5 m), carrefour de 4 à 5 s, rue de 5 à 7 s, parc ensuite ;
//   parois-falaise : montée de la Corniche (WFS-379, ex-lacets) : paroi de roche à gauche seulement (RW + 6,35 m), une trouée de 4,5 à 5,2 s ;
//   parois-stands  : 200 km/h dans la ligne droite des stands : mur vitré à gauche (RW + 1,4 m), tribune à droite de 4 à 6 s ;
//   masque-ville   : un adversaire passe derrière un immeuble (virage à angle droit) : masque de diffraction ;
// WFS-278 :
//   parois-casino   : 50 km/h, Ville (tracé 2) : rue, façade du casino à droite (RW + 12, pierre) de 1 à 5,3 s, carrefour, puis
//                     centre commercial à gauche (RW + 31, verre) de 6 à 10 s ;
//   masque-corniche : un adversaire dans le lacet suivant (branche parallèle à 45 m à gauche) passe derrière la roche ;
//   masque-campagne : une voiture passe derrière les maisons du village (une maison tous les 14,4 m, ruelles entre elles) ;
//   masque-stands   : une voiture passe derrière les garages du Grand Prix (60 m de garages accolés) ;
//   obstacles déclarés comme en jeu (parois::declarer avec une profondeur, accroches des cartes), route droite ;
// .wav : stéréo (son direct + écho) ; .png : niveaux gauche (cyan) et droite (orange) en dB en haut ; en bas : délai de la 1re réflexion
// commandé (blanc, 0 à 150 ms, 0 à 250 ms pour parois-casino) ou masque (magenta, 0 à 20 dB ; 0 à 25 dB pour WFS-278) ; journal : délais mesurés par intercorrélation, niveaux, coût.
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include "../../third_party/stb/stb_image_write.h"
#include "../core/log.h"
#include "../game/cars.h"
#include "../game/track.h"
#include "parois_sons.h"
#include "voitures_sons.h"

namespace wfs::parois {
namespace {

constexpr int R = 48000, PAS = R / 120;
constexpr size_t N = 10 * (size_t)R;

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

// image : niveaux en haut ; en bas, une courbe (valeurs 0..echelle) par pas du jeu
void image(const std::string& path, const std::vector<float>& G, const std::vector<float>& D, const std::vector<float>& bas, float echelle,
           std::array<uint8_t, 3> coulBas) {
    const int W = 1000, H = 400, HN = 250;
    std::vector<uint8_t> img((size_t)W * H * 3, 18);
    auto px = [&](int x, int y, std::array<uint8_t, 3> k) {
        if (x < 0 || y < 0 || x >= W || y >= H) return;
        uint8_t* p = &img[((size_t)y * W + x) * 3]; p[0] = k[0]; p[1] = k[1]; p[2] = k[2];
    };
    for (int d = 0; d <= 60; d += 10) for (int x = 0; x < W; x += 2) px(x, (int)(d / 60.f * (HN - 1)), {60, 60, 70});
    for (int s = 0; s <= 10; s++) for (int y = 0; y < H; y += 3) px(s * (W - 1) / 10, y, {60, 60, 70});
    for (int x = 0; x < W; x++) px(x, HN + 2, {90, 90, 100});
    const size_t w = R / 20;
    for (size_t a = 0; a + w <= G.size(); a += w) {
        double eg = 0, ed = 0; for (size_t i = a; i < a + w; i++) { eg += (double)G[i] * G[i]; ed += (double)D[i] * D[i]; }
        const int x = (int)(a * (W - 1) / G.size());
        for (const auto& [l, k] : {std::pair{dB((float)std::sqrt(eg / (double)w)), std::array<uint8_t, 3>{0, 229, 255}},
                                   std::pair{dB((float)std::sqrt(ed / (double)w)), std::array<uint8_t, 3>{255, 140, 0}}})
            if (l > -60) for (int dx = 0; dx < 5; dx++) for (int dy = -1; dy <= 1; dy++) px(x + dx, (int)(-l / 60.f * (HN - 1)) + dy, k);
    }
    for (size_t k = 0; k < bas.size(); k++) {
        const int x = (int)(k * (W - 1) / bas.size()), y = H - 2 - (int)(std::clamp(bas[k] / echelle, 0.f, 1.f) * (H - HN - 8));
        px(x, y, coulBas); px(x, y + 1, coulBas);
    }
    stbi_write_png(path.c_str(), W, H, 3, img.data(), W * 3);
}

// distances de la tête aux parois (m, 0 : aucune) à un instant ; WFS-278 : absorption de chaque paroi
struct Profil { float a, b; bool tunnel; float alG = ALPHA, alD = ALPHA; };

// délai (ms) de la réflexion la plus forte d'un côté (−1 gauche, +1 droite), mesuré par intercorrélation : bruit blanc dans le bus
// (signal sans période, contrairement au moteur), parois du profil tenues fixes
float delaiMesure(Profil p, int cote) {
    Echos e; e.build((float)R); Suivi su; Reflexion r[6]; float gG[6], gD[6];
    int n = 0; for (int k = 0; k < 40; k++) n = su.pas(1.f / 120, p.a, p.b, p.tunnel, r, gG, gD, p.alG, p.alD);
    for (int i = 0; i < PRISES; i++) e.cible(i, i < n ? r[i].delai : 0.01f, i < n ? gG[i] : 0.f, i < n ? gD[i] : 0.f);
    const size_t M = (size_t)(0.6f * R);
    std::vector<float> b(M), G(M, 0.f), D(M, 0.f);
    sfx::Bruit br; for (float& x : b) x = br.next() * 0.3f;
    for (size_t a = 0; a + PAS <= M; a += PAS) e.process(b.data() + a, G.data() + a, D.data() + a, PAS);
    const std::vector<float>& y = cote < 0 ? G : D;
    size_t best = 0; double vmax = 0;
    for (size_t lag = 140; lag < (size_t)(0.25f * R); lag++) {   // WFS-278 : jusqu'à 250 ms (centre commercial : 224 ms)
        double s = 0; for (size_t i = (size_t)(0.35f * R); i < M; i++) s += (double)y[i] * b[i - lag];
        if (s > vmax) { vmax = s; best = lag; }
    }
    return (float)best / R * 1000;
}

void scene(const std::string& out, const char* nom, const std::vector<float>& m, Profil (*profil)(float), std::initializer_list<float> instants = {2.f},
           float echelleDelai = 150.f, const char* ticket = "WFS-275") {
    Echos e; e.build((float)R); Suivi su;
    std::vector<float> G(N, 0.f), D(N, 0.f), delais;
    Reflexion r[6]; float gG[6], gD[6];
    for (size_t a = 0; a + PAS <= N; a += PAS) {
        const Profil p = profil((float)a / R);
        const int n = su.pas(1.f / 120, p.a, p.b, p.tunnel, r, gG, gD, p.alG, p.alD);
        for (int i = 0; i < PRISES; i++) e.cible(i, i < n ? r[i].delai : 0.01f, i < n ? gG[i] : 0.f, i < n ? gD[i] : 0.f);
        e.process(m.data() + a, G.data() + a, D.data() + a, PAS);
        delais.push_back(n ? r[0].delai * 1000 : 0.f);
    }
    std::vector<float> sG(N), sD(N);
    for (size_t i = 0; i < N; i++) { sG[i] = m[i] + G[i]; sD[i] = m[i] + D[i]; }
    const std::string base = out + "/" + nom;
    wavStereo(base + ".wav", sG, sD);
    image(base + ".png", sG, sD, delais, echelleDelai, {240, 240, 240});
    for (float t : instants) {   // énergie de l'écho sur ± 1 s autour de l'instant (WFS-275 : 1 à 3 s autour de 2 s)
        double em = 0, eg = 0, ed = 0;
        for (size_t i = (size_t)((t - 1) * R); i < (size_t)((t + 1) * R) && i < N; i++) { em += (double)m[i] * m[i]; eg += (double)G[i] * G[i]; ed += (double)D[i] * D[i]; }
        const Profil p2 = profil(t);
        Reflexion rr[6]; const int nr = reflexions(p2.a, p2.b, rr, p2.alG, p2.alD);
        WFS_LOG("son (%s) : %s.wav : à %.0f s, parois %.1f m / %.1f m, %d réflexion(s), 1re attendue %.1f ms, mesurée %.1f ms (gauche) / %.1f ms (droite) ; "
                "écho %.1f dB (gauche) / %.1f dB (droite) sous le son direct", ticket, nom, t, p2.a, p2.b, nr, nr ? rr[0].delai * 1000 : 0.f,
                p2.a > 0 ? delaiMesure(p2, -1) : 0.f, p2.b > 0 ? delaiMesure(p2, 1) : 0.f, 10 * std::log10(std::max(eg, 1e-12) / em), 10 * std::log10(std::max(ed, 1e-12) / em));
    }
}

// masque : l'auditeur à l'arrêt en (0, 1,2, 0), regard vers −z ; une voiture (m4) en p0 + v·t passe derrière les obstacles im
void sceneMasque(const std::string& out, const char* nom, const std::vector<ImmeubleMonde>& im, vec3 p0, vec3 v, float kmh, float echelle,
                 const char* ticket, const char* quoi) {
    voitures::SonsVoitures sv; sv.build((float)R);
    voitures::Attribution at;
    std::vector<float> G(N, 0.f), D(N, 0.f), bus(PAS), masques;
    float etat = 0, mMax = 0, cMax = 0; double tCache = -1, tVu = -1; int retours = 0; bool cache = false;
    for (size_t a = 0; a + PAS <= N; a += PAS) {
        const float t = (float)a / R;
        std::vector<voitures::Candidat> c(1);
        c[0].id = 1; c[0].p = p0 + v * t; c[0].v = v; c[0].kmh = kmh; c[0].car = &carById("m4"); c[0].genre = voitures::ADVERSAIRE;
        const vec3 ici{0, 1.2f, 0};
        at.update(1.f / 120, c, ici);
        voitures::Cible cb = voitures::calculerCible(at.voix(0), 1.f / 120, ici, {}, {0, 0, -1}, {1, 0, 0});
        const float cible = masqueDb(ici, at.voix(0).c.p, im);
        appliquerMasque(cb.gG, cb.gD, cb.coupure, etat, cible, 1.f / 120);
        if (cible > 0 && tCache < 0) tCache = t;
        if (cible <= 0 && tCache >= 0 && tVu < 0) tVu = t;
        if (cache && cible <= 0) retours++;
        cache = cible > 0;
        mMax = std::max(mMax, etat); cMax = std::max(cMax, cible);
        sv.cible(0, cb);
        std::fill(bus.begin(), bus.end(), 0.f);
        sv.mix(G.data() + a, D.data() + a, bus.data(), PAS, 0.5f, true);
        masques.push_back(etat);
    }
    wavStereo(out + "/" + nom + ".wav", G, D);
    image(out + "/" + nom + ".png", G, D, masques, echelle, {255, 60, 200});
    if (std::string(ticket) == "WFS-275")
        WFS_LOG("son (WFS-275) : %s.wav : l'adversaire passe derrière l'immeuble à %.1f s ; masque le plus fort %.1f dB (plafond 20), "
                "son plus sourd (passe-bas 1 kHz) tant qu'il est caché", nom, tCache, mMax);
    else
        WFS_LOG("son (%s) : %s.wav : %s ; cachée à %.1f s, de nouveau entendue à %.1f s (%d retour(s) à l'air libre en 10 s) ; masque le plus fort %.1f dB "
                "(cible %.1f ; plafonds 20 et 25), son plus sourd (passe-bas 1 kHz) tant qu'elle est cachée ; %zu obstacles", ticket, nom, quoi, tCache, tVu, retours,
                mMax, cMax, im.size());
}

// WFS-278 : obstacles d'une carte d'essai (registre, comme en jeu) placés le long d'une route droite vers −z (cap 0 : droite = +x),
// segment 0 en z0
std::vector<ImmeubleMonde> monde(const char* carte, float z0) {
    std::vector<ImmeubleMonde> out;
    const std::string nom = carte;
    for (const Immeuble& im : obstacles(nom))
        out.push_back({{im.centre.lat, 0, z0 - (float)(im.centre.seg * SEG_M + im.centre.avant)}, {1, 0, 0}, {0, 0, -1}, im.demiProf, im.demiLong, im.h});
    return out;
}

} // namespace

void benchParois(const std::string& out, const std::vector<float>& moteurBanc) {
    std::vector<float> m(N);
    double ph = 0;
    for (size_t i = 0; i < N; i++) {
        if (!moteurBanc.empty()) m[i] = moteurBanc[(i + moteurBanc.size() / 3) % moteurBanc.size()];
        else { m[i] = 0.3f * (float)(2 * (ph - std::floor(ph + 0.5))); ph += 100.0 / R; }
    }
    scene(out, "parois-ville", m, [](float t) -> Profil {
        const float F = (float)RW + 6.5f;   // façades des immeubles de la Ville
        if (t < 4) return {F, F, false};
        if (t < 5) return {0, 0, false};   // carrefour
        if (t < 7) return {F, F, false};
        return {0, 0, false};              // parc
    });
    scene(out, "parois-falaise", m, [](float t) -> Profil { return {t >= 4.5f && t < 5.2f ? 0.f : (float)RW + 6.35f + 1.9f, 0, false}; });
    scene(out, "parois-stands", m, [](float t) -> Profil { return {(float)RW + 1.4f + 2.f, t >= 4 && t < 6 ? (float)RW + 14.f - 2.f : 0.f, false}; });
    // masque : l'auditeur à l'arrêt dans le virage, un adversaire passe derrière un immeuble de 10 × 10 m, haut de 15 m
    sceneMasque(out, "masque-ville", {{{12, 0, -22}, {1, 0, 0}, {0, 0, 1}, 5, 5, 15}}, {60.f, 0.6f, -42.f}, {-12.f, 0, 0}, 43, 20.f, "WFS-275", "");

    // ---------------------------------------------------------------- WFS-278
    const float RWf = (float)RW;
    scene(out, "parois-casino", m, [](float t) -> Profil {
        const float F = (float)RW + 6.5f;   // façades des immeubles de la Ville
        if (t < 1) return {F, F, false};
        if (t < 5.3f) return {F, (float)RW + 12.f, false, ALPHA, ALPHA_PIERRE};      // casino à droite (60 m de façade à 50 km/h)
        if (t < 6) return {0, 0, false};                                             // carrefour
        return {(float)RW + 31.f, F, false, ALPHA_VERRE, ALPHA};                     // centre commercial à gauche
    }, {3.f, 8.f}, 250.f, "WFS-278");
    {   // Corniche : roche côté montagne (accroche de plage.cpp : face à RW + 4,6 + 1,75 m, hauteurParoi), sur 60 m, profondeur de
        // 17 m ; l'adversaire roule à 108 km/h de l'autre côté de la roche, à 45 m. WFS-379 : la Corniche n'a plus de lacets (la
        // scène reste un massif de roche entre l'auditeur et la voiture, mêmes mesures qu'avant pour comparer les journaux)
        effacer("banc-corniche");
        const float x0 = RWf + 4.6f, prof = (45.f + RWf + 1) / 2 - x0;
        for (int n = 0; n < 50; n++)
            declarer("banc-corniche", 1000, n, -1, x0 + 1.75f, 13.f + 3.f * std::sin(n * 0.013f) + 3.f * std::sin(n * 0.051f + 1.3f), (float)SEG_M, prof, ALPHA_PIERRE);
        sceneMasque(out, "masque-corniche", monde("banc-corniche", 30.f), {-45.f, 0.6f, -150.f}, {0, 0, 30.f}, 108, 25.f, "WFS-278",
                    "adversaire derrière la roche de la montée (60 m de roche, 13 m de haut environ)");
    }
    {   // Campagne : maisons à 12 m à droite (10 m de façade, 8 m de profondeur, 6 m de haut), une tous les 14,4 m ; voiture à 35 m, 108 km/h
        effacer("banc-campagne");
        for (int k = 0; k < 7; k++) declarer("banc-campagne", 1000, 12 * k, 1, 12.f, 6.f, 10.f, 8.f, ALPHA_DUR);
        sceneMasque(out, "masque-campagne", monde("banc-campagne", 43.f), {35.f, 0.6f, 150.f}, {0, 0, -30.f}, 108, 25.f, "WFS-278",
                    "voiture derrière les maisons du village, entendue dans les ruelles entre elles");
    }
    {   // Grand Prix : garages à 22 m à gauche (6 m de large, 12 m de profondeur, 7 m de haut), 10 côte à côte ; voiture à 60 m, 198 km/h
        effacer("banc-stands");
        for (int g = 0; g < 10; g++) declarer("banc-stands", 1000, 5 + 5 * g, -1, 22.f, 7.f, 6.f, 12.f, ALPHA_DUR);
        sceneMasque(out, "masque-stands", monde("banc-stands", 33.f), {-60.f, 0.6f, -250.f}, {0, 0, 55.f}, 198, 25.f, "WFS-278",
                    "voiture derrière les garages du Grand Prix (60 m de garages accolés)");
    }
    {   // coût des masques avec la roche de la Corniche (le cas le plus chargé : beaucoup de morceaux accolés)
        effacer("banc-cout");
        for (int n = 0; n < 250; n++) declarer("banc-cout", 1000, n, -1, RWf + 6.35f, 13.f + 3.f * std::sin(n * 0.051f), (float)SEG_M, 20.f, ALPHA_PIERRE);
        for (int n = 0; n < 250; n += 9) declarer("banc-cout", 1000, n, 1, 14.f, 7.f, 10.f, 8.f, ALPHA_DUR);
        const std::vector<ImmeubleMonde> ob = monde("banc-cout", 150.f);
        const auto t0 = std::chrono::steady_clock::now();
        float somme = 0;
        for (int b = 0; b < 400; b++) for (int k = 0; k < 13; k++) somme += masqueDb({0, 1.2f, 0}, {-45.f + (k % 2) * 90.f, 0.6f, 120.f - k * 20.f - b * 0.1f}, ob);
        WFS_LOG("son (WFS-278) : coût des masques : %.3f ms par pas (13 sources, %zu obstacles de roche et de villas sur 300 m ; ≤ 0,2 ms)%s",
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / 400, ob.size(), somme < 0 ? " " : "");
    }
    {   // RG-08 : coût de l'écho, 6 prises (rue bordée des deux côtés)
        Echos e; e.build((float)R); Suivi su; Reflexion r[6]; float gG[6], gD[6];
        int n = 0; for (int k = 0; k < 30; k++) n = su.pas(1.f / 120, 6, 8, false, r, gG, gD);
        for (int i = 0; i < n; i++) e.cible(i, r[i].delai, gG[i], gD[i]);
        std::vector<float> bus(480, 0.1f), G(480), D(480);
        const auto t0 = std::chrono::steady_clock::now();
        for (int b = 0; b < 2000; b++) e.process(bus.data(), G.data(), D.data(), 480);
        WFS_LOG("son (WFS-275) : coût de l'écho des murs : %.3f ms par bloc de 10 ms (%d prises ; ≤ 0,3 ms)",
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / 2000, n);
    }
}

} // namespace wfs::parois
