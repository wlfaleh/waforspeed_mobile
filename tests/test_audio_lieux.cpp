// WFS-261 : son des lieux. Vérifie hors ligne, sans carte son :
//   RG-01 : sortie stéréo : le son existant recopié à l'égal dans les deux canaux (mono identique, écart 0 dB ≤ 0,5 dB) ;
//   RG-03 : −6 dB par doublement de la distance (±1 dB), 0 au-delà de la portée, passe-bas 8 kHz à 1 m → 2 kHz à la portée ;
//   RG-04 : panoramique à puissance constante (source à 90° à droite : gauche ≤ −9 dB), de dos −3 dB et aigus ≤ 4 kHz ;
//   RG-05 : tunnel : 1re réflexion à largeur / 343 m/s, TR60 de 1,5 à 3 s, réverbération ≥ 6 dB sous le moteur, fondus ≥ 0,3 s ;
//   RG-06 : la nuit, moitié du volume ; RG-07 : coût ≤ 0,3 ms par bloc de 10 ms ;
//   repère de la route : une source garde sa place d'un tour à l'autre ; terrasses regroupées.
// usage : test_audio_lieux
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>
#include "../src/audio/ambiance_lieux.h"
#include "../src/game/track.h"
using namespace wfs;
using namespace wfs::lieux;

static int echecs = 0;
static void verifier(bool ok, const char* quoi) {
    std::printf("[%s] %s\n", ok ? "OK" : "ÉCHEC", quoi);
    if (!ok) echecs++;
}
static float dB(float x) { return 20.f * std::log10(std::max(x, 1e-12f)); }
static float rms(const std::vector<float>& b, size_t a, size_t e) {
    double s = 0; for (size_t i = a; i < e; i++) s += (double)b[i] * b[i];
    return (float)std::sqrt(s / (double)std::max<size_t>(1, e - a));
}

int main() {
    constexpr float R = 48000;
    char msg[256];

    // ---------- RG-01 : mono identique ----------
    {
        std::vector<float> m(4096), z(4096, 0.f), out(8192), out1(4096);
        uint32_t s = 1;
        for (float& x : m) { s ^= s << 13; s ^= s >> 17; s ^= s << 5; x = ((float)s / 4294967295.f * 2 - 1) * 1.3f; }   // jusqu'à ±1,3
        entrelacer(m.data(), z.data(), z.data(), out.data(), 4096, 2);
        bool ident = true;
        for (int i = 0; i < 4096; i++) ident = ident && out[2 * i] == m[i] && out[2 * i + 1] == m[i];
        verifier(ident, "RG-01 : sans source ni tunnel, gauche = droite = son mono d'avant, à l'échantillon près (écart 0 dB)");
        entrelacer(m.data(), z.data(), z.data(), out1.data(), 4096, 1);
        bool mono = true; for (int i = 0; i < 4096; i++) mono = mono && out1[i] == m[i];
        verifier(mono, "RG-01 : sortie mono (iOS, banc) inchangée");
        std::vector<float> G(4096, 0.25f), D(4096, 0.f);
        entrelacer(z.data(), G.data(), D.data(), out.data(), 4096, 2);
        verifier(out[0] == 0.25f && out[1] == 0.f, "RG-01 : une source à gauche ne sort qu'à gauche");
    }

    // ---------- RG-03 : distance ----------
    for (int t = 0; t < NB_TYPES; t++) {
        const Fiche& f = FICHES[t];
        float pire = 0;
        for (float d = f.d0; 2 * d <= 0.9f * f.portee; d *= 1.5f) pire = std::max(pire, std::abs(dB(gainDistance(d, f.d0, f.portee)) - dB(gainDistance(2 * d, f.d0, f.portee)) - 6.02f));
        std::snprintf(msg, sizeof msg, "RG-03 %s : −6 dB par doublement de %g m à %g m (écart max %.2f dB ≤ 1)", nomType(t), f.d0, 0.9 * f.portee, pire);
        verifier(pire <= 1.f, msg);
        std::snprintf(msg, sizeof msg, "RG-03 %s : coupé à la portée (%g m) et au-delà, plein en deçà de %g m", nomType(t), f.portee, f.d0);
        verifier(gainDistance(f.portee, f.d0, f.portee) == 0 && gainDistance(f.portee * 2, f.d0, f.portee) == 0 && gainDistance(f.d0 * 0.5f, f.d0, f.portee) == 1, msg);
        bool decroit = true; float prec = 2;
        for (float d = 0.5f; d < f.portee * 1.1f; d += 0.25f) { const float g = gainDistance(d, f.d0, f.portee); decroit = decroit && g <= prec; prec = g; }
        std::snprintf(msg, sizeof msg, "RG-03 %s : niveau qui ne remonte jamais en s'éloignant (aucune marche)", nomType(t));
        verifier(decroit, msg);
        std::snprintf(msg, sizeof msg, "RG-03 %s : passe-bas 8 kHz à 1 m (%.0f), 2 kHz à la portée (%.0f)", nomType(t), coupureDistance(1, f.portee), coupureDistance(f.portee, f.portee));
        verifier(std::abs(coupureDistance(1, f.portee) - 8000) < 1 && std::abs(coupureDistance(f.portee, f.portee) - 2000) < 1 &&
                 coupureDistance(f.portee / 2, f.portee) < 8000 && coupureDistance(f.portee / 2, f.portee) > 2000, msg);
    }
    {   // le filtre à un pôle atténue de 3 dB à sa coupure (sinus de 2 kHz, coupure 2 kHz) et laisse passer les graves
        auto gainSinus = [&](float f, float fc) {
            const float a = coefPasseBas(fc, R); float y = 0, crete = 0;
            for (int i = 0; i < 48000; i++) { y += a * (std::sin(6.2831853f * f * i / R) - y); if (i > 24000) crete = std::max(crete, std::abs(y)); }
            return dB(crete);
        };
        const float a3 = gainSinus(2000, 2000), a0 = gainSinus(100, 2000);
        std::snprintf(msg, sizeof msg, "RG-03 : passe-bas : −3 dB à la coupure (%.2f dB ± 1), graves intacts (%.2f dB)", a3, a0);
        verifier(std::abs(a3 + 3.f) <= 1.f && a0 > -0.5f, msg);
    }

    // ---------- RG-04 : gauche / droite ----------
    {
        const Fiche& f = FICHES[FONTAINE];
        const Gains d90 = spatialiser(14, 1.5707963f, f), g90 = spatialiser(14, -1.5707963f, f), face = spatialiser(14, 0, f);
        std::snprintf(msg, sizeof msg, "RG-04 : source à 90° à droite : gauche %.1f dB sous la droite (≤ −9)", dB(d90.gG) - dB(d90.gD));
        verifier(dB(d90.gG) - dB(d90.gD) <= -9.f, msg);
        verifier(dB(g90.gD) - dB(g90.gG) <= -9.f, "RG-04 : source à 90° à gauche : droite ≤ −9 dB sous la gauche");
        verifier(std::abs(face.gG - face.gD) < 1e-6f, "RG-04 : source devant : même niveau à gauche et à droite");
        float pire = 0;
        for (float az = -3.1f; az <= 3.1f; az += 0.05f) {
            const Gains x = spatialiser(14, az, f);
            pire = std::max(pire, std::abs(x.gG * x.gG + x.gD * x.gD - x.g * x.g));
        }
        verifier(pire < 1e-5f, "RG-04 : puissance constante (gauche² + droite² = niveau²) sur tout le tour");
        const Gains dos = spatialiser(14, 3.14159265f, f);
        std::snprintf(msg, sizeof msg, "RG-04 : de dos : %.2f dB par rapport à devant (−3 ± 0,1), coupure %.0f Hz (≤ 4 kHz)", dB(dos.g) - dB(face.g), dos.coupure);
        verifier(std::abs(dB(dos.g) - dB(face.g) + 3.f) <= 0.1f && dos.coupure <= 4000.f, msg);
        verifier(std::abs(d90.g - face.g) < 1e-6f && d90.coupure == face.coupure, "RG-04 : sur le côté, ni atténuation ni filtre de dos");
    }

    // ---------- sources : mélange, gauche / droite, nuit ----------
    {
        SonsDesLieux s; s.build(R, false); s.active(true);
        SonsDesLieux n; n.build(R, true); n.active(true);
        bool nonVides = true;
        for (int t = 0; t < NB_TYPES; t++) if (t != FERME) nonVides = nonVides && s.boucle(t) && s.boucle(t)->size() > (size_t)R;
        for (int e = 0; e < ESPECES; e++) nonVides = nonVides && s.appel(e) && !s.appel(e)->empty();
        verifier(nonVides, "RG-02 : 4 boucles (fontaine, vagues, tribune, terrasse) et 3 appels (vache, cheval, mouton) calculés");
        bool raccords = true;   // le saut au raccord de la boucle n'est pas plus grand qu'un saut ordinaire du son lui-même
        for (int t = 0; t < NB_TYPES; t++) if (t != FERME) {
            const auto& b = *s.boucle(t); float interne = 0;
            for (size_t i = 1; i < b.size(); i++) interne = std::max(interne, std::abs(b[i] - b[i - 1]));
            const float raccord = std::abs(b.front() - b.back());
            std::snprintf(msg, sizeof msg, "boucle %s : saut au raccord %.3f ≤ plus grand saut du son %.3f", nomType(t), raccord, interne);
            verifier(raccord <= interne, msg); raccords = raccords && raccord <= interne;
        }
        const Gains g = spatialiser(20, 1.0f, FICHES[TRIBUNE]);   // à droite devant
        s.cible(TRIBUNE, g); n.cible(TRIBUNE, g);
        std::vector<float> G(48000, 0.f), D(48000, 0.f), Gn(48000, 0.f), Dn(48000, 0.f);
        for (int b = 0; b < 100; b++) { s.mix(G.data() + b * 480, D.data() + b * 480, 480, 1); n.mix(Gn.data() + b * 480, Dn.data() + b * 480, 480, 1); }
        const float lr = dB(rms(G, 4800, 48000)) - dB(rms(D, 4800, 48000));
        std::snprintf(msg, sizeof msg, "RG-04 : tribune à droite : gauche − droite mesuré %.2f dB, attendu %.2f dB (± 0,5)", lr, dB(g.gG) - dB(g.gD));
        verifier(std::abs(lr - (dB(g.gG) - dB(g.gD))) <= 0.5f, msg);
        const float nuit = dB(rms(Dn, 4800, 48000)) - dB(rms(D, 4800, 48000));
        std::snprintf(msg, sizeof msg, "RG-06 : la nuit, moitié du volume (%.2f dB, −6 ± 0,2)", nuit);
        verifier(std::abs(nuit + 6.02f) <= 0.2f, msg);
        std::vector<float> Gm(4800, 0.f), Dm(4800, 0.f);
        s.cible(TRIBUNE, Gains{});
        for (int b = 0; b < 5; b++) s.mix(Gm.data() + b * 480, Dm.data() + b * 480, 480, 1);   // 1er bloc : rampe vers 0
        verifier(rms(Gm, 480, 4800) == 0 && rms(Dm, 480, 4800) == 0, "source hors de portée : silence après une rampe d'un bloc");
        s.cible(FERME, spatialiser(15, 0, FICHES[FERME]), 2); s.premierAppel(0.1f);
        std::vector<float> Gf(48000, 0.f), Df(48000, 0.f);
        for (int b = 0; b < 100; b++) s.mix(Gf.data() + b * 480, Df.data() + b * 480, 480, 1);
        verifier(rms(Gf, 0, 48000) > 1e-3f, "RG-02 : ferme : un appel d'animal s'entend quand le troupeau est à portée");
        SonsDesLieux off; off.build(R, false);   // non activée (autre écran que la course)
        off.cible(TRIBUNE, g);
        std::vector<float> Go(4800, 0.f), Do(4800, 0.f);
        for (int b = 0; b < 10; b++) off.mix(Go.data() + b * 480, Do.data() + b * 480, 480, 1);
        verifier(rms(Go, 0, 4800) == 0, "hors de l'écran de course : aucune source");
    }

    // ---------- RG-05 : tunnel ----------
    {
        Tunnel tu; tu.build(R); tu.dedans(true);
        std::vector<float> z(480, 0.f), G(480), D(480);
        for (int b = 0; b < 50; b++) { std::fill(G.begin(), G.end(), 0.f); std::fill(D.begin(), D.end(), 0.f); tu.process(z.data(), G.data(), D.data(), 480); }
        verifier(tu.niveau() == 1.f, "RG-05 : dans le tunnel, réverbération à plein après le fondu");
        // réponse à une impulsion : 4 s
        const size_t N = (size_t)(4 * R);
        std::vector<float> imp(N, 0.f), hG(N, 0.f), hD(N, 0.f); imp[0] = 1;
        for (size_t a = 0; a < N; a += 480) tu.process(imp.data() + a, hG.data() + a, hD.data() + a, 480);
        size_t premier = 0; while (premier < N && hG[premier] == 0 && hD[premier] == 0) premier++;
        const float attendu = LARGEUR_TUNNEL / VITESSE_SON;
        std::snprintf(msg, sizeof msg, "RG-05 : 1re réflexion à %.1f ms, attendu largeur %.0f m / 343 m/s = %.1f ms (± 1 ms)", premier / R * 1000, LARGEUR_TUNNEL, attendu * 1000);
        verifier(std::abs(premier / R - attendu) <= 0.001f, msg);
        // TR60 (intégration à rebours de Schroeder, pente de −5 à −25 dB, extrapolée à −60 dB)
        std::vector<double> e(N + 1, 0.0);
        for (size_t i = N; i-- > 0;) e[i] = e[i + 1] + (double)hG[i] * hG[i] + (double)hD[i] * hD[i];
        size_t i5 = 0, i25 = 0;
        for (size_t i = 0; i < N; i++) {
            const double l = 10 * std::log10(e[i] / e[0]);
            if (!i5 && l <= -5) i5 = i;
            if (!i25 && l <= -25) { i25 = i; break; }
        }
        const float tr = i25 > i5 ? (float)(i25 - i5) / R * 3.f : 0.f;
        std::snprintf(msg, sizeof msg, "RG-05 : queue de réverbération TR60 = %.2f s (1,5 à 3 s)", tr);
        verifier(tr >= 1.5f && tr <= 3.f, msg);
        // moteur (dents de scie à 90 Hz, 2 harmoniques modulées) : la réverbération reste ≥ 6 dB sous le son direct
        Tunnel t2; t2.build(R); t2.dedans(true);
        const size_t M = (size_t)(3 * R);
        std::vector<float> moteur(M), wG(M, 0.f), wD(M, 0.f);
        double ph = 0;
        for (size_t i = 0; i < M; i++) { const float f = 90 + 20 * std::sin(6.2831853f * 0.3f * i / R); moteur[i] = 0.3f * (float)(2 * (ph - std::floor(ph + 0.5))); ph += f / R; }
        for (size_t a = 0; a < M; a += 480) t2.process(moteur.data() + a, wG.data() + a, wD.data() + a, 480);
        const float ecart = dB(rms(moteur, M / 2, M)) - dB(std::max(rms(wG, M / 2, M), rms(wD, M / 2, M)));
        std::snprintf(msg, sizeof msg, "RG-05 : le moteur reste %.1f dB au-dessus de la réverbération (≥ 6 dB)", ecart);
        verifier(ecart >= 6.f, msg);
        // fondus : de 1 à 0 en sortant, de 0 à 1 en entrant, en ≥ 0,3 s
        auto duree = [&](bool dedans) {
            t2.dedans(dedans); int blocs = 0;
            std::vector<float> a(480, 0.f), b(480, 0.f);
            while ((dedans ? t2.niveau() < 1.f : t2.niveau() > 0.f) && blocs < 1000) { t2.process(moteur.data(), a.data(), b.data(), 480); blocs++; }
            return blocs * 480 / R;
        };
        const float sortie = duree(false), entree = duree(true);
        std::snprintf(msg, sizeof msg, "RG-05 : fondu de sortie %.2f s, d'entrée %.2f s (≥ 0,3 s)", sortie, entree);
        verifier(sortie >= 0.3f && entree >= 0.3f && sortie < 1.f && entree < 1.f, msg);
        Tunnel t3; t3.build(R);   // jamais entré : rien n'est ajouté
        std::vector<float> a(480, 0.f), b(480, 0.f);
        t3.process(moteur.data(), a.data(), b.data(), 480);
        verifier(rms(a, 0, 480) == 0 && rms(b, 0, 480) == 0, "RG-05 : hors d'un tunnel, aucun écho");
    }

    // ---------- RG-07 : coût ----------
    {
        SonsDesLieux s; s.build(R, false); s.active(true);
        for (int t = 0; t < NB_TYPES; t++) s.cible(t, spatialiser(10, 0.7f, FICHES[t]), 1);
        Tunnel tu; tu.build(R); tu.dedans(true);
        std::vector<float> in(480, 0.1f), G(480), D(480);
        const int BLOCS = 3000;
        const auto t0 = std::chrono::steady_clock::now();
        for (int b = 0; b < BLOCS; b++) {
            std::fill(G.begin(), G.end(), 0.f); std::fill(D.begin(), D.end(), 0.f);
            s.mix(G.data(), D.data(), 480, 1); tu.process(in.data(), G.data(), D.data(), 480);
        }
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / BLOCS;
        std::snprintf(msg, sizeof msg, "RG-07 : 5 sources + tunnel : %.3f ms par bloc de 10 ms (≤ 0,3 ms)", ms);
        verifier(ms <= 0.3, msg);
    }

    // ---------- repère de la route ----------
    {
        Track t; t.build(mapById("ville"));
        const int64_t L = (int64_t)t.segmentCount();
        const Pose p = t.pose(500);
        const vec3 q{(float)p.x + 12.f, (float)p.y + 3.f, (float)p.z - 7.f};
        const Lieu l = depuisMonde(t, 500, q);
        const vec3 r0 = positionMonde(t, l, 520);
        verifier(length(r0 - q) < 0.01f, "repère de la route : une position du monde revient à sa place (tour 0)");
        // au tour suivant, la source est à la même distance du même point de la route (copie déplacée du tour)
        const Pose p1 = t.pose(510 + L), p0 = t.pose(510);
        const vec3 r1 = positionMonde(t, l, 520 + L);
        const float d0 = length(r0 - vec3{(float)p0.x, (float)p0.y, (float)p0.z}), d1 = length(r1 - vec3{(float)p1.x, (float)p1.y, (float)p1.z});
        std::snprintf(msg, sizeof msg, "repère de la route : au tour suivant, même distance à la route (%.2f m et %.2f m)", d0, d1);
        verifier(std::abs(d0 - d1) < 0.05f, msg);
        const std::vector<Attable> cl = {{100.0, 11.7f}, {100.5, 11.7f}, {102.0, 11.7f}, {103.0, -11.7f}, {140.0, 11.7f}, {141.0, 11.7f}};
        const std::vector<Lieu> ter = regrouperTerrasses(cl, L);
        verifier(ter.size() == 3 && ter[0].lat > 0 && ter[1].lat < 0 && ter[2].seg == 140, "terrasses : clients regroupés par terrasse (même côté, < 6 m)");
    }

    std::printf(echecs ? "test_audio_lieux : %d ÉCHEC(S)\n" : "test_audio_lieux : tout est conforme\n", echecs);
    return echecs ? 1 : 0;
}
