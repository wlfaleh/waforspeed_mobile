// WFS-288 : son de la pluie. Vérifie hors ligne, sans carte son :
//   RG-01 : pluie de référence (Marshall et Palmer, Atlas) recalculée ici d'une autre façon (± 1 %) ;
//   RG-02 : silence hors pluie et hors écran Course, fondus de 1 s et 0,4 s ;
//   RG-03 : averse gauche / droite décorrélée (|ρ| ≤ 0,2), aucun motif qui revient (autocorrélation ≤ 0,2 de 0,05 à 2 s),
//           maximum entre 1 et 8 kHz, gouttes isolées et bulles à leur cadence ;
//   RG-04 : cadence des impacts du toit et du pare-brise (rapport 0 / 50 / 130 km/h ± 2 %), averse étouffée ;
//   RG-05 : phase des essuie-glaces du son = phase du modèle de l'image (≤ 0,01 après 10 min), 2 chocs par cycle ;
//   RG-06 : tunnel : pluie coupée en 0,3 s, reprise en 0,5 s, essuie-glaces continus ;
//   RG-07 : crête de la pluie ≥ 6 dB sous la crête du moteur à mi-régime, sans saturation, chuintement des pneus audible ;
//   RG-08 : hors pluie, sortie identique à l'échantillon près ; RG-09 : coût.
// usage : test_audio_pluie [racine des fichiers du jeu, « assets/ » par défaut]
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>
#include "../src/audio/engine_synth.h"
#include "../src/audio/pluie_sons.h"
#include "../src/audio/route_sons.h"
#include "../src/audio/sfx_synth.h"
#include "../src/game/cars.h"
#include "../src/game/player.h"
#include "../src/game/track.h"
using namespace wfs;
using namespace wfs::pluie;

static int echecs = 0;
static void verifier(bool ok, const char* quoi) { std::printf("[%s] %s\n", ok ? "OK" : "ÉCHEC", quoi); if (!ok) echecs++; }
static float dB(double x) { return 20.f * (float)std::log10(std::max(x, 1e-12)); }
constexpr int R = 48000, PAS = R / 120;
constexpr float DT = 1.f / 120, MASTER = 0.5f;

static double rms(const std::vector<float>& b, size_t a, size_t e) { double s = 0; for (size_t i = a; i < e; i++) s += (double)b[i] * b[i]; return std::sqrt(s / (double)(e - a)); }
static double correlation(const std::vector<float>& x, const std::vector<float>& y, size_t a, size_t n, size_t lag) {
    double sxy = 0, sxx = 0, syy = 0;
    for (size_t i = a; i < a + n; i++) { sxy += (double)x[i] * y[i + lag]; sxx += (double)x[i] * x[i]; syy += (double)y[i + lag] * y[i + lag]; }
    return sxy / std::sqrt(std::max(1e-30, sxx * syy));
}
// énergie dans une bande (passe-bande RBJ d'ordre 2, Q = f / largeur)
static double bande(const std::vector<float>& x, size_t a, size_t e, float f, float q) {
    const float w = 6.28318530718f * f / R, c = std::cos(w), al = std::sin(w) / (2 * q), a0 = 1 + al;
    const float b0 = al / a0, b2 = -al / a0, a1 = -2 * c / a0, a2 = (1 - al) / a0;
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0; double s = 0;
    for (size_t i = a; i < e; i++) { const float y = b0 * x[i] + b2 * x2 - a1 * y1 - a2 * y2; x2 = x1; x1 = x[i]; y2 = y1; y1 = y; s += (double)y * y; }
    return std::sqrt(s / (double)(e - a));
}
// scène : n secondes au pas du jeu ; état fixe
struct Etat { float kmh = 0; bool actif = true, pluie = true, cockpit = false, balais = false, tunnel = false; };
static void rendre(SonsPluie& p, const Surfaces& s, const Etat& e, float secondes, std::vector<float>& G, std::vector<float>& D) {
    const size_t N = (size_t)(secondes * R), a0 = G.size();
    G.resize(a0 + N, 0.f); D.resize(a0 + N, 0.f);
    p.active(e.actif);
    for (size_t a = 0; a + PAS <= N; a += PAS) { p.pas(DT, e.kmh, e.pluie, e.cockpit, e.balais, e.tunnel, s); p.mix(G.data() + a0 + a, D.data() + a0 + a, PAS, MASTER); }
}

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : "assets/";
    char msg[400];
    const CarDef& golf = carById("golf");
    const Surfaces S = surfaces(golf.d.L, golf.d.W, golf.d.H, golf.d.wb);

    // ---------- RG-01 ----------
    const Reference ref = reference();
    {
        // autre calcul : trapèzes à pas fin, intégrales jusqu'à 12 mm
        const double L = 4.1 * std::pow(5.0, -0.21);
        double F = 0, Fg = 0, NT = 0, Fb = 0; const double h = 1e-4;
        for (double D = 0.5; D < 12; D += h) {
            const double Dm = D + h / 2, N = 8000 * std::exp(-L * Dm), v = std::max(0.0, 9.65 - 10.3 * std::exp(-0.6 * Dm));
            F += N * v * h; NT += N * h; if (Dm >= 2) Fg += N * v * h; if (Dm >= 0.8 && Dm <= 1.1) Fb += N * v * h;
        }
        auto ecart = [](double a, double b) { return std::fabs(a - b) / std::max(1e-9, std::fabs(b)); };
        std::snprintf(msg, sizeof msg, "RG-01 : 5 mm/h : Λ = %.4f mm⁻¹ (%.4f), %.0f gouttes/m³ (%.0f), %.0f impacts/m²/s (%.0f), grosses %.1f/m²/s (%.1f) soit %.2f %%, "
                      "bulles %.0f/m²/s (%.0f) ; contrôle : pluie recalculée %.2f mm/h", ref.lambda, L, ref.concentration, NT, ref.flux, F, ref.fluxGrosses, Fg,
                      100 * ref.partGrosses, ref.fluxBulles, Fb, ref.tauxMmh);
        verifier(ecart(ref.lambda, L) < 0.01 && ecart(ref.concentration, NT) < 0.01 && ecart(ref.flux, F) < 0.01 && ecart(ref.fluxGrosses, Fg) < 0.01 &&
                 ecart(ref.fluxBulles, Fb) < 0.01 && ref.tauxMmh > 2.5f && ref.tauxMmh < 7.6f, msg);
        std::snprintf(msg, sizeof msg, "RG-01 : vitesse de chute (Atlas) %.2f / %.2f / %.2f m/s à 1 / 2 / 5 mm (Gunn et Kinzer, d'après le PO : 4,0 / 6,5 / 9,1 ± 2 %%)",
                      vitesseChute(1), vitesseChute(2), vitesseChute(5));
        verifier(ecart(vitesseChute(1), 4.0) < 0.02 && ecart(vitesseChute(2), 6.5) < 0.02 && ecart(vitesseChute(5), 9.1) < 0.02, msg);
    }

    // ---------- RG-02 ----------
    {
        SonsPluie p; p.build(R);
        std::vector<float> G, D;
        Etat e; e.pluie = false; rendre(p, S, e, 2, G, D);            // il ne pleut pas (jour, autre carte)
        bool zero = true; for (size_t i = 0; i < G.size(); i++) zero = zero && G[i] == 0 && D[i] == 0;
        std::vector<float> G2, D2; Etat m; m.actif = false; rendre(p, S, m, 2, G2, D2);   // il pleut, mais menus / pause / scène dédiée
        bool zero2 = true; for (size_t i = 0; i < G2.size(); i++) zero2 = zero2 && G2[i] == 0 && D2[i] == 0;
        verifier(zero && zero2, "RG-02 : silence s'il ne pleut pas et hors de l'écran Course (rien n'est ajouté)");
        // fondus : entrée en 1 s, sortie en 0,4 s
        SonsPluie q; q.build(R); std::vector<float> g, d;
        float tPlein = -1, tMuet = -1;
        q.active(true);
        for (int k = 0; k < 240; k++) { q.pas(DT, 0, true, false, false, false, S); std::vector<float> a(PAS), b(PAS); q.mix(a.data(), b.data(), PAS, MASTER); if (tPlein < 0 && q.presence() >= 1) tPlein = (k + 1) * DT; }
        q.active(false);
        for (int k = 0; k < 120; k++) { q.pas(DT, 0, true, false, false, false, S); std::vector<float> a(PAS), b(PAS); q.mix(a.data(), b.data(), PAS, MASTER); if (tMuet < 0 && q.presence() <= 0) tMuet = (k + 1) * DT; }
        std::snprintf(msg, sizeof msg, "RG-02 : fondu d'entrée %.2f s (1), de sortie %.2f s (0,4) ; « Son : Coupé » : audio_system remplit la sortie de zéros après la pluie (WFS-68)", tPlein, tMuet);
        verifier(std::fabs(tPlein - 1) < 0.02f && std::fabs(tMuet - 0.4f) < 0.02f, msg);
    }

    // ---------- RG-03 ----------
    {
        SonsPluie p; p.build(R);
        std::vector<float> G, D; Etat e; rendre(p, S, e, 8, G, D);
        const size_t a = (size_t)(1.5 * R), n = (size_t)(2 * R);
        const double rho = correlation(G, D, a, n, 0);
        double pire = 0; size_t pireLag = 0;
        for (size_t lag = (size_t)(0.05 * R); lag <= (size_t)(2 * R); lag += (size_t)(0.01 * R)) {
            const double c = std::fabs(correlation(G, G, a, (size_t)(2 * R), lag));
            if (c > pire) { pire = c; pireLag = lag; }
        }
        std::snprintf(msg, sizeof msg, "RG-03 : averse : ρ gauche / droite %.3f (|ρ| ≤ 0,2) ; autocorrélation la plus forte de 0,05 à 2 s : %.3f à %.2f s (≤ 0,2)",
                      rho, pire, (double)pireLag / R);
        verifier(std::fabs(rho) <= 0.2 && pire <= 0.2, msg);
        const double b250 = bande(G, a, a + n, 250, 1.4f), b500 = bande(G, a, a + n, 500, 1.4f), b2k = bande(G, a, a + n, 2000, 1.4f), b4k = bande(G, a, a + n, 4000, 1.4f),
                     b12k = bande(G, a, a + n, 12000, 1.4f);
        std::snprintf(msg, sizeof msg, "RG-03 : spectre (bandes d'une octave) : 250 Hz %.1f, 500 Hz %.1f, 2 kHz %.1f, 4 kHz %.1f, 12 kHz %.1f dB : maximum entre 1 et 8 kHz",
                      dB(b250), dB(b500), dB(b2k), dB(b4k), dB(b12k));
        verifier(std::max(b2k, b4k) > b500 && std::max(b2k, b4k) > b250 && std::max(b2k, b4k) > b12k, msg);
        const float tG = (float)p.gouttes() / 8, tB = (float)p.bulles() / 8;
        std::snprintf(msg, sizeof msg, "RG-03 : gouttes isolées %.1f/s (grosses gouttes sur %.0f m² : %.1f/s ± 10 %%), bulles %.1f/s (flaque de %.1f m² : %.1f/s ± 10 %%)",
                      tG, A_ECOUTE, ref.fluxGrosses * A_ECOUTE, tB, A_FLAQUE, ref.fluxBulles * A_FLAQUE);
        verifier(std::fabs(tG / (ref.fluxGrosses * A_ECOUTE) - 1) < 0.1f && std::fabs(tB / (ref.fluxBulles * A_FLAQUE) - 1) < 0.1f, msg);
    }

    // ---------- RG-04 ----------
    {
        float taux[3] = {}; const float vitesses[3] = {0, 50, 130}; float tauxToit = 0;
        for (int k = 0; k < 3; k++) {
            SonsPluie p; p.build(R);
            std::vector<float> G, D; Etat e; e.cockpit = true; e.kmh = vitesses[k]; rendre(p, S, e, 20, G, D);
            taux[k] = (float)p.impactsPareBrise() / 20; if (k == 0) tauxToit = (float)p.impactsToit() / 20;
        }
        const float t0 = fluxPareBrise(ref, 0) * S.pareBrise, t50 = fluxPareBrise(ref, 50) * S.pareBrise, t130 = fluxPareBrise(ref, 130) * S.pareBrise;
        std::snprintf(msg, sizeof msg, "RG-04 : Golf : toit %.2f m² (%.0f impacts/s, attendu %.0f), pare-brise %.2f m² à %.1f° : %.0f / %.0f / %.0f impacts/s à 0 / 50 / 130 km/h, "
                      "rapport 1 / %.2f / %.2f (attendu 1 / %.2f / %.2f ± 2 %%)", S.toit, tauxToit, ref.flux * S.toit, S.pareBrise, std::atan(S.pente) * 57.2958f,
                      taux[0], taux[1], taux[2], taux[1] / taux[0], taux[2] / taux[0], t50 / t0, t130 / t0);
        verifier(std::fabs(tauxToit / (ref.flux * S.toit) - 1) < 0.02f && std::fabs((taux[1] / taux[0]) / (t50 / t0) - 1) < 0.02f &&
                 std::fabs((taux[2] / taux[0]) / (t130 / t0) - 1) < 0.02f && std::fabs(taux[0] / t0 - 1) < 0.02f, msg);
        // averse étouffée : aigus (8 kHz) de la vue Cockpit bien sous ceux de la vue extérieure ; ni gouttes isolées ni bulles
        SonsPluie pe, pc; pe.build(R); pc.build(R);
        std::vector<float> Ge, De, Gc, Dc; Etat ee, ec; ec.cockpit = true;
        rendre(pe, S, ee, 4, Ge, De); rendre(pc, S, ec, 4, Gc, Dc);
        const size_t a = (size_t)(1.5 * R), b = (size_t)(4 * R) - PAS;
        const double hE = bande(Ge, a, b, 6000, 1.4f), hC = bande(Gc, a, b, 6000, 1.4f);
        std::snprintf(msg, sizeof msg, "RG-04 : vue Cockpit : aigus (6 kHz) %.1f dB sous la vue extérieure (≥ 15), %llu goutte(s) isolée(s) et %llu bulle(s) entendue(s)",
                      dB(hE) - dB(hC), (unsigned long long)pc.gouttes(), (unsigned long long)pc.bulles());
        verifier(dB(hE) - dB(hC) >= 15 && pc.gouttes() == 0 && pc.bulles() == 0, msg);
    }

    // ---------- RG-05 ----------
    {
        SonsPluie p; p.build(R); p.active(true);
        cockpit::EtatEssuie image;   // modèle de l'image (camera_vue.cpp), mêmes entrées
        std::vector<float> G(PAS), D(PAS);
        float pire = 0, pireDebut = 0; double t = 0;
        for (int k = 0; k < 10 * 60 * 120; k++, t += DT) {
            const double kmh = 75 + 70 * std::sin(t * 0.21);   // 5 à 145 km/h : 45 et 65 balayages par minute
            image.avancer(DT, kmh, true);
            p.pas(DT, kmh, true, true, true, false, S);
            std::fill(G.begin(), G.end(), 0.f); std::fill(D.begin(), D.end(), 0.f);
            p.mix(G.data(), D.data(), PAS, MASTER);
            auto ecart = [](float a, float b) { float d = std::fabs(a - b); return std::min(d, 1 - d); };
            pire = std::max(pire, ecart(p.essuie().phase, image.phase));
            pireDebut = std::max(pireDebut, ecart(p.phaseDebutBloc(), image.phase));
        }
        std::snprintf(msg, sizeof msg, "RG-05 : 10 min de pas : phase du son (modèle) – phase de l'image %.6f, phase du thread audio en début de bloc – image %.6f (≤ 0,01)",
                      pire, pireDebut);
        verifier(pire <= 0.01f && pireDebut <= 0.01f, msg);
        // chocs : 2 par cycle (butée et haut de la course) ; frottement nul aux deux bouts
        SonsPluie q; q.build(R); std::vector<float> g, d;
        Etat e; e.cockpit = true; e.balais = true; e.kmh = 50; rendre(q, S, e, 20, g, d);
        const float cycles = 20 * 45.f / 60;
        std::snprintf(msg, sizeof msg, "RG-05 : 20 s à 45 balayages par minute : %llu chocs pour %.0f cycles (2 par cycle ± 1)", (unsigned long long)q.chocs(), cycles);
        verifier(std::fabs((float)q.chocs() - 2 * cycles) <= 2, msg);
    }

    // ---------- RG-06 ----------
    {
        SonsPluie p; p.build(R); p.active(true);
        std::vector<float> G(PAS), D(PAS);
        for (int k = 0; k < 240; k++) { p.pas(DT, 50, true, true, true, false, S); p.mix(G.data(), D.data(), PAS, MASTER); }
        const uint64_t chocs0 = p.chocs(); float tCoupe = -1, tRepris = -1;
        for (int k = 0; k < 360; k++) { p.pas(DT, 50, true, true, true, true, S); p.mix(G.data(), D.data(), PAS, MASTER); if (tCoupe < 0 && p.horsTunnel() <= 0) tCoupe = (k + 1) * DT; }
        const uint64_t chocsTunnel = p.chocs() - chocs0, toitTunnel = p.impactsToit();
        for (int k = 0; k < 120; k++) { p.pas(DT, 50, true, true, true, false, S); p.mix(G.data(), D.data(), PAS, MASTER); if (tRepris < 0 && p.horsTunnel() >= 1) tRepris = (k + 1) * DT; }
        std::snprintf(msg, sizeof msg, "RG-06 : tunnel : pluie coupée en %.2f s (0,3), reprise en %.2f s (0,5) ; %llu chocs des essuie-glaces pendant les 3 s du tunnel",
                      tCoupe, tRepris, (unsigned long long)chocsTunnel);
        verifier(std::fabs(tCoupe - 0.3f) < 0.02f && std::fabs(tRepris - 0.5f) < 0.02f && chocsTunnel >= 3 && toitTunnel > 0, msg);
    }

    // ---------- RG-07 ----------
    {
        float creteMoteur = 0;   // moteur du joueur (Golf) à mi-régime, en pleine accélération
        std::ifstream f(root + "/" + engineVoiceFor("golf").sample, std::ios::binary);
        std::vector<uint8_t> wav((std::istreambuf_iterator<char>(f)), {});
        EngineSynth es; es.setOutputRate(R);
        if (!es.loadWav(wav)) verifier(false, "RG-07 : son du moteur de la Golf illisible");
        else {
            Track t; t.build(mapById("ville")); Player pl; pl.reset(golf); EngineModel em;
            std::vector<float> pcm(PAS);
            for (int i = 0; i < 12 * 120; i++) {
                DriveInput in; in.gas = true; pl.update(DT, in, t, true);
                em.update(DT, pl.speed, MAX_SPEED * pl.perf.top, MAX_SPEED, true, false, true, engineVoiceFor("golf").pitch, false, es.targets); es.targets.lastUpdate = 1e9;
                es.render(pcm.data(), PAS, 0);
                if (em.rpm >= 0.45f && em.rpm <= 0.55f) for (float x : pcm) creteMoteur = std::max(creteMoteur, std::fabs(x));
            }
        }
        float pire = 0; const char* vuePire = "";
        for (int v = 0; v < 2; v++) for (float kmh : {0.f, 50.f, 130.f}) {
            SonsPluie p; p.build(R); std::vector<float> G, D; Etat e; e.cockpit = v == 1; e.balais = v == 1; e.kmh = kmh; rendre(p, S, e, 20, G, D);
            for (size_t i = 0; i < G.size(); i++) { const float c = std::max(std::fabs(G[i]), std::fabs(D[i])); if (c > pire) { pire = c; vuePire = v ? "Cockpit" : "extérieure"; } }
        }
        std::snprintf(msg, sizeof msg, "RG-07 : crête de la pluie %.3f (vue %s, pire de 0 / 50 / 130 km/h), crête du moteur à mi-régime %.3f : %.1f dB d'écart (≥ 6) ; "
                      "somme des crêtes %.3f (< 1, pas de saturation)", pire, vuePire, creteMoteur, dB(creteMoteur) - dB(pire), creteMoteur + pire);
        verifier(creteMoteur > 0 && dB(creteMoteur) - dB(pire) >= 6 && creteMoteur + pire < 1, msg);
        // chuintement des pneus (WFS-174, route_sons.h) à 50 km/h sous la pluie, comparé à l'averse dans sa bande (3-6 kHz)
        Player pl; pl.reset(golf);
        route::Entree en; en.kmh = 50; en.kmhMax = (float)(pl.perf.top * 260.0); en.pluie = true; en.enCourse = true;
        const route::Cibles c = route::cibles(en);
        std::vector<float> eau = sfx::boucle((float)R, 0.37f, 4500, 0.7f, 1);
        float pk = 0; for (float x : eau) pk = std::max(pk, std::fabs(x));
        for (float& x : eau) x = x / pk * c.eau * MASTER;
        std::vector<float> eauLong; for (int k = 0; k < 6; k++) eauLong.insert(eauLong.end(), eau.begin(), eau.end());
        SonsPluie p; p.build(R); std::vector<float> G, D; Etat e; e.kmh = 50; rendre(p, S, e, 4, G, D);
        const double bPneus = bande(eauLong, 0, eauLong.size(), 4500, 1.5f), bPluie = bande(G, (size_t)(1.5 * R), (size_t)(3.9 * R), 4500, 1.5f);
        std::snprintf(msg, sizeof msg, "RG-07 : à 50 km/h, chuintement des pneus %.1f dB, averse %.1f dB dans la même bande (4,5 kHz) : %.1f dB au-dessus (≥ 3, audible)",
                      dB(bPneus), dB(bPluie), dB(bPneus) - dB(bPluie));
        verifier(dB(bPneus) - dB(bPluie) >= 3, msg);
    }

    // ---------- RG-08 ----------
    {
        SonsPluie p; p.build(R);
        std::vector<float> G(48000), D(48000);
        for (size_t i = 0; i < G.size(); i++) { G[i] = 0.2f * std::sin(0.01f * (float)i); D[i] = 0.1f * std::cos(0.013f * (float)i); }
        const std::vector<float> G0 = G, D0 = D;
        p.active(true);
        for (size_t a = 0; a + PAS <= G.size(); a += PAS) { p.pas(DT, 80, false, true, true, false, S); p.mix(G.data() + a, D.data() + a, PAS, MASTER); }
        verifier(G == G0 && D == D0, "RG-08 : sans pluie (vue Cockpit, 80 km/h) : la sortie est identique, à l'échantillon près");
    }

    // ---------- RG-09 ----------
    {
        SonsPluie p; p.build(R); p.active(true);
        std::vector<float> G(480), D(480);
        for (int k = 0; k < 60; k++) { p.pas(DT, 130, true, true, true, false, S); p.mix(G.data(), D.data(), 400, MASTER); }
        const int B = 3000;
        const auto t0 = std::chrono::steady_clock::now();
        for (int b = 0; b < B; b++) { p.pas(DT, 130, true, true, true, false, S); p.mix(G.data(), D.data(), 480, MASTER); }
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / B;
        SonsPluie e; e.build(R); e.active(true);
        for (int k = 0; k < 60; k++) { e.pas(DT, 130, true, false, false, false, S); e.mix(G.data(), D.data(), 400, MASTER); }
        const auto t1 = std::chrono::steady_clock::now();
        for (int b = 0; b < B; b++) { e.pas(DT, 130, true, false, false, false, S); e.mix(G.data(), D.data(), 480, MASTER); }
        const double msE = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t1).count() / B;
        std::snprintf(msg, sizeof msg, "RG-09 : coût par bloc de 10 ms : vue Cockpit %.4f ms, vue extérieure %.4f ms (≤ 0,1) ; aucun tampon de son (sons calculés en direct)", ms, msE);
        verifier(ms <= 0.1 && msE <= 0.1, msg);
    }

    std::printf(echecs ? "test_audio_pluie : %d ÉCHEC(S)\n" : "test_audio_pluie : tout est conforme\n", echecs);
    return echecs ? 1 : 0;
}
