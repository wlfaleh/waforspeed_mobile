// WFS-298 : son des chocs selon ce qu'on touche. Vérifie hors ligne, sans carte son, avec la même synthèse que le jeu :
//   RG-01 : genre touché → famille ; RG-02 : T60 mesuré ± 10 % de la valeur retenue (T60 = 2,2 / (f · η)) ;
//   RG-03 : +6 dB ± 1 dB par doublement de la vitesse, coupure de l'excitation ∝ v^(1/5) ± 5 %, plus clair plus vite, jamais deux
//           fois la même variante de suite ; RG-04 : obstacle trouvé (distances), côté (≥ 6 dB du bon côté) ;
//   RG-05 : crête à 30 km/h entre −6 et 0 dB du coup sourd, accident ≤ +3 dB, bus < 1 ; RG-06 : sans choc, rien n'est ajouté ;
//   RG-07 : coût, toutes les voix actives.
// usage : test_audio_chocs [racine des fichiers du jeu, « assets/ » par défaut]
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>
#include "../src/audio/chocs_sons.h"
#include "../src/audio/engine_synth.h"
#include "../src/audio/voitures_sons.h"
#include "../src/game/cars.h"
#include "../src/game/player.h"
#include "../src/game/track.h"
using namespace wfs;
using namespace wfs::sonchocs;

static int echecs = 0;
static void verifier(bool ok, const char* quoi) { std::printf("[%s] %s\n", ok ? "OK" : "ÉCHEC", quoi); if (!ok) echecs++; }
static float dB(double x) { return 20.f * (float)std::log10(std::max(x, 1e-12)); }
constexpr int R = 48000, PAS = 400;
constexpr float MASTER = 0.5f;

// un choc rendu seul : son de l'obstacle (G, D) pendant `duree` s
static void rendre(SonsChocs& s, int f, float v, float az, bool accident, float duree, std::vector<float>& G, std::vector<float>& D) {
    const size_t N = (size_t)(duree * R);
    G.assign(N, 0.f); D.assign(N, 0.f); std::vector<float> bus(PAS);
    const voitures::Pan p = voitures::panExistant(az);
    s.choc(f, v, p.g, p.d, accident);
    for (size_t a = 0; a + PAS <= N; a += PAS) { std::fill(bus.begin(), bus.end(), 0.f); s.mix(G.data() + a, D.data() + a, bus.data(), PAS, MASTER); }
}
static double rms(const std::vector<float>& a, const std::vector<float>& b) { double e = 0; for (size_t i = 0; i < a.size(); i++) e += (double)a[i] * a[i] + (double)b[i] * b[i]; return std::sqrt(e / (double)a.size()); }
static float crete(const std::vector<float>& a, const std::vector<float>& b) { float p = 0; for (size_t i = 0; i < a.size(); i++) p = std::max(p, std::max(std::fabs(a[i]), std::fabs(b[i]))); return p; }

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : "assets/";
    char msg[1200];

    // ---------- RG-01 ----------
    {
        using G = chocs::Genre;
        verifier(famillePour(G::Poteau) == POTEAU && famillePour(G::Arbre) == ARBRE && famillePour(G::Mobilier) == MOBILIER && famillePour(G::Mur) == BETON &&
                 famillePour(G::Batiment) == BETON && famillePour(G::Ilot) == BETON && famillePour(G::Voiture) == VOITURE && famillePour(G::Autre) == AUCUNE,
                 "RG-01 : poteau → métal sonore, arbre → bois et feuillage, mobilier → mobilier léger, mur / bâtiment / îlot → béton, voiture → voiture, autre → rien");
    }

    // ---------- RG-02 ----------
    {
        bool ok = true; char l[800]; int k = 0;
        for (int f = 0; f < NB_FAMILLES; f++) {
            float pire = 0; bool vue[VARIANTES] = {};
            SonsChocs s; s.build(R);
            for (int c = 0; c < 8; c++) {   // 8 chocs l'un après l'autre : les 4 variantes passent
                std::vector<float> G, D;
                rendre(s, f, V_REF, 0, false, matiere(f).duree, G, D);
                const int var = s.derniere(f); vue[var] = true;
                std::vector<float> o(G.size()); for (size_t i = 0; i < o.size(); i++) o[i] = G[i] + D[i];
                const float t60 = mesurerT60(o, R, f == BETON ? 0.f : s.mode(f, var, 0).f);
                const float attendu = f == BETON ? T60_BETON : s.mode(f, var, 0).t60;
                pire = std::max(pire, std::fabs(t60 / attendu - 1));
                ok = ok && std::fabs(t60 / attendu - 1) <= 0.10f && std::fabs(attendu / t60Retenu(f) - 1) <= 0.06f;
            }
            ok = ok && vue[0] && vue[1] && vue[2] && vue[3];
            k += std::snprintf(l + k, sizeof l - (size_t)k, "%s%s %.3f s (±%.0f %%)", f ? ", " : "", nomFamille(f), t60Retenu(f), 100 * pire);
        }
        std::snprintf(msg, sizeof msg, "RG-02 : T60 mesuré / retenu (4 variantes) : %s (≤ 10 %%) ; poteau 2,2 / (188 Hz × 0,01) = %.2f s", l, t60Mode(188, ETA_ACIER_MAT));
        verifier(ok, msg);
        verifier(t60Retenu(POTEAU) >= 0.8f && t60Retenu(POTEAU) <= 1.5f && t60Retenu(MOBILIER) >= 0.2f && t60Retenu(MOBILIER) <= 0.5f && t60Retenu(ARBRE) <= 0.15f &&
                 t60Retenu(BETON) <= 0.1f && t60Retenu(VOITURE) >= 0.2f && t60Retenu(VOITURE) <= 0.4f,
                 "RG-02 : T60 dans les plages du PO (métal 0,8-1,5 s, mobilier 0,2-0,5 s, tronc ≤ 0,15 s, béton ≤ 0,1 s, tôle 0,2-0,4 s)");
    }

    // ---------- RG-03 ----------
    {
        bool ok = true; char l[800]; int k = 0;
        for (int f = 0; f < NB_FAMILLES; f++) {
            float d[2];
            for (int p = 0; p < 2; p++) {   // 15 → 30 km/h, puis 30 → 60 km/h (toujours la variante 0 : deux moteurs neufs)
                const float v1 = (p ? 30.f : 15.f) / 3.6f, v2 = 2 * v1;
                SonsChocs a, b; a.build(R); b.build(R);
                std::vector<float> G1, D1, G2, D2;
                rendre(a, f, v1, 0, false, matiere(f).duree, G1, D1); rendre(b, f, v2, 0, false, matiere(f).duree, G2, D2);
                d[p] = dB(rms(G2, D2)) - dB(rms(G1, D1));
                ok = ok && a.derniere(f) == b.derniere(f) && std::fabs(d[p] - 6.02f) <= 1;
            }
            k += std::snprintf(l + k, sizeof l - (size_t)k, "%s%s +%.1f / +%.1f", f ? ", " : "", nomFamille(f), d[0], d[1]);
        }
        std::snprintf(msg, sizeof msg, "RG-03 : niveau par doublement (15→30, 30→60 km/h) : %s dB (+6 ± 1)", l);
        verifier(ok, msg);
        bool okC = true;
        for (int f = 0; f < NB_FAMILLES; f++) for (float v : {7.f, 15.f, 30.f, 50.f})
            okC = okC && std::fabs(coupureExcitation(f, 2 * v / 3.6f) / coupureExcitation(f, v / 3.6f) / std::pow(2.f, 0.2f) - 1) <= 0.05f;
        std::snprintf(msg, sizeof msg, "RG-03 : coupure de l'excitation : poteau %.0f / %.0f / %.0f Hz à 15 / 30 / 80 km/h, rapport par doublement %.4f (2^(1/5) = %.4f ± 5 %%)",
                      coupureExcitation(POTEAU, 15 / 3.6f), coupureExcitation(POTEAU, 30 / 3.6f), coupureExcitation(POTEAU, 80 / 3.6f),
                      coupureExcitation(POTEAU, 60 / 3.6f) / coupureExcitation(POTEAU, 30 / 3.6f), std::pow(2.f, 0.2f));
        verifier(okC, msg);
        // plus clair plus vite : à 4 kHz, le niveau monte plus que celui du mode grave
        SonsChocs a, b; a.build(R); b.build(R);
        std::vector<float> G1, D1, G2, D2;
        rendre(a, POTEAU, 15 / 3.6f, 0, false, 0.4f, G1, D1); rendre(b, POTEAU, 60 / 3.6f, 0, false, 0.4f, G2, D2);
        std::vector<float> o1(G1.size()), o2(G2.size()); for (size_t i = 0; i < o1.size(); i++) { o1[i] = G1[i] + D1[i]; o2[i] = G2[i] + D2[i]; }
        const float hautes = niveauTiers(o2, 0, o2.size(), R, 3507) - niveauTiers(o1, 0, o1.size(), R, 3507), basses = niveauTiers(o2, 0, o2.size(), R, 188) - niveauTiers(o1, 0, o1.size(), R, 188);
        std::snprintf(msg, sizeof msg, "RG-03 : poteau de 15 à 60 km/h : aigus (3,5 kHz) +%.1f dB, grave (188 Hz) +%.1f dB : plus clair plus vite", hautes, basses);
        verifier(hautes > basses + 0.5f, msg);
        SonsChocs s; s.build(R);
        bool jamais = true; int vu[VARIANTES] = {};
        for (int f = 0; f < NB_FAMILLES; f++) { int avant = -1; for (int i = 0; i < 200; i++) { const int v = s.choc(f, V_REF, 1, 1, false); jamais = jamais && v != avant && v >= 0; avant = v; if (f == 0) vu[v]++; } }
        std::snprintf(msg, sizeof msg, "RG-03 : 200 chocs par famille : jamais deux fois la même variante de suite ; poteau : variantes jouées %d / %d / %d / %d fois", vu[0], vu[1], vu[2], vu[3]);
        verifier(jamais && vu[0] > 20 && vu[1] > 20 && vu[2] > 20 && vu[3] > 20, msg);
    }

    // ---------- RG-04 ----------
    {
        Rect moi; moi.x = 0; moi.z = 0; moi.cap = 0; moi.hl = 2.145; moi.hw = 0.895;   // Golf, avant vers −z, droite vers +x
        chocs::Obstacle p; p.x = 0.895 + 0.3 + 0.15; p.z = -1; p.r = 0.15;               // poteau à 30 cm du flanc droit
        double qx, qz; const double dp = distance(moi, p, qx, qz); const float azp = azimut(moi, qx, qz);
        chocs::Obstacle m; m.forme = chocs::Forme::Boite; m.x = -0.895 - 0.2 - 0.5; m.z = 0; m.hx = 0.5; m.hz = 10; m.angle = 0;   // mur à 20 cm à gauche
        const double dm = distance(moi, m, qx, qz); const float azm = azimut(moi, qx, qz);
        Rect autre; autre.x = 0; autre.z = -2.145 - 0.4 - 2.145; autre.cap = 0; autre.hl = 2.145; autre.hw = 0.895;   // voiture 40 cm devant
        const double dv = distance(moi, autre, qx, qz); const float azv = azimut(moi, qx, qz);
        std::snprintf(msg, sizeof msg, "RG-04 : distances : poteau %.3f m (0,30) à %.0f°, mur %.3f m (0,20) à %.0f°, voiture %.3f m (0,40) à %.0f°",
                      dp, azp * 57.2958f, dm, azm * 57.2958f, dv, azv * 57.2958f);
        verifier(std::fabs(dp - 0.30) < 1e-6 && azp > 0.5f && std::fabs(dm - 0.20) < 1e-6 && azm < -0.5f && std::fabs(dv - 0.40) < 1e-6 && std::fabs(azv) < 0.6f, msg);
        SonsChocs a, b; a.build(R); b.build(R);
        std::vector<float> G1, D1, G2, D2;
        rendre(a, POTEAU, V_REF, -1.5707963f, false, 1, G1, D1); rendre(b, POTEAU, V_REF, 1.5707963f, false, 1, G2, D2);
        const float gauche = dB(rms(G1, G1)) - dB(rms(D1, D1)), droite = dB(rms(D2, D2)) - dB(rms(G2, G2));
        std::snprintf(msg, sizeof msg, "RG-04 : obstacle à gauche : gauche %.1f dB au-dessus de la droite ; à droite : droite %.1f dB au-dessus (≥ 6)", gauche, droite);
        verifier(gauche >= 6 && droite >= 6, msg);
    }

    // ---------- RG-05 ----------
    {
        const std::vector<float> petit = coupSourd(R, false), acc = coupSourd(R, true);
        float pP = 0, pA = 0; for (float x : petit) pP = std::max(pP, std::fabs(x)); for (float x : acc) pA = std::max(pA, std::fabs(x));
        const float coup30 = pP * (0.4f + V_REF / (float)(MAX_SPEED * M)) * MASTER, coupAcc = pA * MASTER;
        bool ok = true; char l[800]; int k = 0; float pire80 = 0;
        for (int f = 0; f < NB_FAMILLES; f++) {
            float e30 = -99, e30min = 99;
            SonsChocs s; s.build(R);
            for (int c = 0; c < 8; c++) {   // 8 chocs de suite : les 4 variantes ; panoramique au centre (gain 1 de chaque côté)
                std::vector<float> G, D;
                rendre(s, f, V_REF, 0, false, matiere(f).duree, G, D);
                const float e = dB(crete(G, D)) - dB(coup30);
                e30 = std::max(e30, e); e30min = std::min(e30min, e);
            }
            {   // obstacle sur le côté : le côté proche reçoit le panoramique de WFS-272 (× 1,4) ; la crête reste sous celle du coup sourd
                std::vector<float> G, D; rendre(s, f, V_REF, 1.5707963f, false, matiere(f).duree, G, D);
                const float e = dB(crete(G, D)) - dB(coup30);
                e30 = std::max(e30, e); e30min = std::min(e30min, e);
            }
            SonsChocs s80; s80.build(R); std::vector<float> G, D; rendre(s80, f, 80 / 3.6f, 0, true, matiere(f).duree, G, D);
            const float e80 = dB(crete(G, D)) - dB(coupAcc);
            pire80 = std::max(pire80, e80);
            ok = ok && e30 <= 0.01f && e30min >= -6 && e80 <= 3;
            k += std::snprintf(l + k, sizeof l - (size_t)k, "%s%s %.1f à %.1f dB (accident %+.1f)", f ? ", " : "", nomFamille(f), e30min, e30, e80);
        }
        std::snprintf(msg, sizeof msg, "RG-05 : crête du son de l'obstacle / coup sourd à 30 km/h, au centre et sur le côté : %s (règle : −6 à 0 dB ; accident ≤ +3 dB)", l);
        verifier(ok, msg);
        // bus : moteur de la Golf au rupteur + accident + poteau à 80 km/h
        float creteMoteur = 0;
        std::ifstream fw(root + "/" + engineVoiceFor("golf").sample, std::ios::binary);
        std::vector<uint8_t> wav((std::istreambuf_iterator<char>(fw)), {});
        EngineSynth es; es.setOutputRate(R);
        if (es.loadWav(wav)) {
            Track t; t.build(mapById("ville")); Player pl; pl.reset(carById("golf")); EngineModel em; std::vector<float> pcm(PAS);
            for (int i = 0; i < 20 * 120; i++) {
                DriveInput in; in.gas = true; pl.update(1.0 / 120, in, t, true);
                em.update(1.0 / 120, pl.speed, MAX_SPEED * pl.perf.top, MAX_SPEED, true, false, true, engineVoiceFor("golf").pitch, false, es.targets); es.targets.lastUpdate = 1e9;
                es.render(pcm.data(), PAS, 0);
                for (float x : pcm) creteMoteur = std::max(creteMoteur, std::fabs(x));
            }
        }
        SonsChocs s; s.build(R); std::vector<float> G, D; rendre(s, POTEAU, 80 / 3.6f, 1.5707963f, true, 1, G, D);
        const float somme = creteMoteur + coupAcc + crete(G, D);
        std::snprintf(msg, sizeof msg, "RG-05 : pire cas : moteur au rupteur %.3f + accident %.3f + poteau à 80 km/h %.3f = %.3f (< 1, pas de saturation)",
                      creteMoteur, coupAcc, crete(G, D), somme);
        verifier(creteMoteur > 0 && somme < 1, msg);
    }

    // ---------- RG-06 ----------
    {
        SonsChocs s; s.build(R);
        std::vector<float> G(48000), D(48000), B(48000);
        for (size_t i = 0; i < G.size(); i++) { G[i] = 0.2f * std::sin(0.01f * (float)i); D[i] = 0.1f * std::cos(0.013f * (float)i); B[i] = 0.05f; }
        const std::vector<float> G0 = G, D0 = D, B0 = B;
        for (size_t a = 0; a + PAS <= G.size(); a += PAS) s.mix(G.data() + a, D.data() + a, B.data() + a, PAS, MASTER);
        verifier(G == G0 && D == D0 && B == B0, "RG-06 : sans choc, la sortie et le bus de l'écho sont identiques, à l'échantillon près");
    }

    // ---------- RG-07 ----------
    {
        SonsChocs s; s.build(R);
        std::vector<float> G(480), D(480), B(480);
        for (int k = 0; k < VOIX; k++) s.choc(k % 2 ? VOITURE : POTEAU, 80 / 3.6f, 1, 1, true);
        s.mix(G.data(), D.data(), B.data(), 1, MASTER);
        const int actives = s.actives();
        const auto t0 = std::chrono::steady_clock::now();
        int b = 0; for (; b < 40 && s.actives() == VOIX; b++) s.mix(G.data(), D.data(), B.data(), 480, MASTER);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / std::max(1, b);
        std::snprintf(msg, sizeof msg, "RG-07 : coût avec les %d voix actives (poteaux et voitures à 80 km/h, accident) : %.4f ms par bloc de 10 ms (≤ 0,05) ; aucun tampon de son",
                      actives, ms);
        verifier(actives == VOIX && b == 40 && ms <= 0.05, msg);
    }

    std::printf(echecs ? "test_audio_chocs : %d ÉCHEC(S)\n" : "test_audio_chocs : tout est conforme\n", echecs);
    return echecs ? 1 : 0;
}
