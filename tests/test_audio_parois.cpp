// WFS-275 : acoustique des lieux. Vérifie hors ligne, sans carte son :
//   RG-02 : registre des parois (la plus proche gagne) et façades de la Ville lues dans le plan ;
//   RG-03 : délai 2·d / c (± 1 échantillon), niveau 20·lg(r0 / 2d) (± 0,5 dB), ordres 2 et 3 d'une rue bordée des deux côtés ;
//   RG-04 : paroi à droite seule → écho ≥ 9 dB plus fort à droite ;
//   RG-05 : trouée en fondu de 50 à 100 ms, tunnel en fondu ≥ 0,3 s, changement de délai sans clic ;
//   RG-06 : D = 10·lg(3 + 20·N), plafond 20 dB, détour géométrique, ligne au-dessus du toit ;
//   RG-07 : sans paroi, rien n'est ajouté (à l'échantillon près) ; RG-08 : coût.
// usage : test_audio_parois
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>
#include "../src/audio/parois_sons.h"
#include "../src/game/track.h"
#include "../src/world/ville_quartiers.h"
using namespace wfs;
using namespace wfs::parois;

static int echecs = 0;
static void verifier(bool ok, const char* quoi) { std::printf("[%s] %s\n", ok ? "OK" : "ÉCHEC", quoi); if (!ok) echecs++; }
static float dB(float x) { return 20.f * std::log10(std::max(x, 1e-12f)); }

int main() {
    char msg[300];
    constexpr float R = 48000, DT = 1.f / 120;

    // ---------- RG-02 ----------
    {
        effacer("essai");
        declarer("essai", 100, 10, 1, 20.f, 8.f, 12.f);
        declarer("essai", 100, 12, 1, 14.f, 20.f, 4.f);   // plus proche : gagne là où elle est
        declarer("essai", 100, 98, -1, 9.f, 3.f, 6.f);    // à cheval sur la fin du tour
        const std::string essai = "essai";
        const std::vector<Cotes>& p = lire(essai);   // 6 m autour du segment 98 : segments 95 à 101, soit 95…99 puis 0 et 1
        verifier(p.size() == 100 && p[5][1].d == 20.f && p[12][1].d == 14.f && p[14][1].d == 14.f && p[15][1].d == 20.f && p[0][0].d == 9.f && p[1][0].d == 9.f && p[94][0].d == 0.f && p[2][0].d == 0.f,
                 "RG-02 : parois par segment et par côté ; la plus proche de l'axe gagne ; un mur qui passe la fin du tour continue au début");
        ville::Plan pl; pl.objets.resize(200);
        ville::Objet im; im.t = ville::IMMEUBLE; im.side = -1; im.h = 18; im.len = 24; im.d = 12; pl.objets[50].push_back(im);
        ville::Objet en; en.t = ville::ENTREPOT; en.side = 1; en.lat = 22; en.h = 9; en.len = 36; en.d = 20; pl.objets[120].push_back(en);
        depuisVille(pl, 200);
        const std::string nomVille = "ville";
        const std::vector<Cotes>& v = lire(nomVille);
        verifier(v[60][0].d == (float)RW + 6.5f && v[60][0].h == 18.f && v[140][1].d == 22.f && v[49][0].d == 0.f && v[60][1].d == 0.f,
                 "RG-02 : Ville : façade des immeubles à RW + 6,5 m (SET_BACK), entrepôts à leur latéral, sur leur longueur");
        const auto ims = immeublesVille(pl, 200);
        verifier(ims.size() == 2 && ims[0].centre.seg == 60 && std::abs(ims[0].centre.lat + ((float)RW + 6.5f + 6.f)) < 1e-4f && ims[0].demiLong == 12.f,
                 "RG-06 : immeubles de la Ville (centre, profondeur, longueur) pour les masques");
    }

    // ---------- RG-03 ----------
    {
        Reflexion r[6];
        int n = reflexions(10.f, 0, r);
        const float att = 20 * std::log10(1.5f / 20.f) + 20 * std::log10(std::sqrt(1 - ALPHA));
        std::snprintf(msg, sizeof msg, "RG-03 : paroi à 10 m : délai %.1f ms (2·d / c = %.1f), niveau %.2f dB (formule %.2f dont %.2f dB de réflexion)",
                      r[0].delai * 1000, 20 / 343.f * 1000, dB(r[0].gain), att, 20 * std::log10(std::sqrt(1 - ALPHA)));
        verifier(n == 1 && std::abs(r[0].delai - 20 / 343.f) < 1e-6f && std::abs(dB(r[0].gain) - att) < 0.5f && r[0].cote == -1, msg);
        reflexions(2.f, 0, r); const float n2 = dB(r[0].gain);
        reflexions(40.f, 0, r); const float n40 = dB(r[0].gain);
        std::snprintf(msg, sizeof msg, "RG-03 : paroi à 2 m : %.1f dB (−8,5) ; à 40 m : %.1f dB (−34,5) ; perte à la réflexion %.2f dB (≤ 0,3)", n2, n40, 20 * std::log10(std::sqrt(1 - ALPHA)));
        verifier(std::abs(n2 + 8.5f) < 0.5f && std::abs(n40 + 34.5f) < 0.5f && -20 * std::log10(std::sqrt(1 - ALPHA)) <= 0.3f, msg);
        n = reflexions(4.f, 6.f, r);   // rue de 10 m : 8, 12, 20, 20, 28, 32 m
        const float attendu[6] = {8, 12, 20, 20, 28, 32};
        bool ok = n == 6;
        for (int i = 0; i < n && ok; i++) ok = std::abs(r[i].delai * 343.f - attendu[i]) < 1e-3f;
        verifier(ok && r[2].ordre == 2 && r[4].ordre == 3, "RG-03 : rue bordée des deux côtés (4 m et 6 m) : ordres 1 à 3, chemins 8, 12, 20, 20, 28 et 32 m");
        verifier(reflexions(30.f, 25.f, r) == 2, "RG-03 : rue de plus de 40 m de large : ordre 1 seulement");
        // écho réel : impulsion dans le bus, paroi de gauche à 10 m
        Echos e; e.build(R);
        Suivi su; float gG[6], gD[6];
        for (int k = 0; k < 30; k++) su.pas(DT, 10.f, 0, false, r, gG, gD);   // présence à plein
        e.cible(0, r[0].delai, gG[0], gD[0]);
        std::vector<float> bus(9600, 0.f), G(9600, 0.f), D(9600, 0.f), z(480, 0.f), zg(480), zd(480);
        for (int b = 0; b < 5; b++) e.process(z.data(), zg.data(), zd.data(), 480);   // gains à plein
        bus[0] = 1;
        for (size_t a = 0; a < bus.size(); a += 480) e.process(bus.data() + a, G.data() + a, D.data() + a, 480);
        size_t pic = 1; for (size_t i = 1; i + 1 < G.size(); i++) if (std::abs(G[i]) > std::abs(G[pic])) pic = i;
        const float attenduEch = r[0].delai * R;
        const float amp = std::abs(G[pic - 1]) + std::abs(G[pic]) + std::abs(G[pic + 1]);   // délai fractionnaire : l'impulsion tombe sur deux échantillons
        std::snprintf(msg, sizeof msg, "RG-03 : écho mesuré à %zu échantillons (attendu %.1f ± 1), %.2f dB à gauche (attendu %.2f ± 0,5)", pic, attenduEch, dB(amp), dB(gG[0]));
        verifier(std::abs((float)pic - attenduEch) <= 1.f && std::abs(dB(amp) - dB(gG[0])) < 0.5f, msg);
        // RG-04
        std::snprintf(msg, sizeof msg, "RG-04 : paroi de gauche : écho %.1f dB plus fort à gauche qu'à droite (≥ 9)", dB(gG[0]) - dB(gD[0]));
        verifier(dB(gG[0]) - dB(gD[0]) >= 9.f, msg);
        Suivi s2; for (int k = 0; k < 30; k++) s2.pas(DT, 0, 7.f, false, r, gG, gD);
        std::snprintf(msg, sizeof msg, "RG-04 : paroi de droite seule : écho %.1f dB plus fort à droite (≥ 9)", dB(gD[0]) - dB(gG[0]));
        verifier(r[0].cote == 1 && dB(gD[0]) - dB(gG[0]) >= 9.f, msg);
    }

    // ---------- RG-05 ----------
    {
        Suivi s; Reflexion r[6]; float gG[6], gD[6];
        for (int k = 0; k < 30; k++) s.pas(DT, 12.f, 0, false, r, gG, gD);
        int pas = 0; while (s.presenceG() > 0 && pas < 100) { s.pas(DT, 0, 0, false, r, gG, gD); pas++; }
        std::snprintf(msg, sizeof msg, "RG-05 : trouée : l'écho s'éteint en %.0f ms (50 à 100)", pas * DT * 1000);
        verifier(pas * DT >= 0.05f && pas * DT <= 0.1f, msg);
        pas = 0; do { s.pas(DT, 12.f, 0, false, r, gG, gD); pas++; } while (s.presenceG() < 1 && pas < 100);
        std::snprintf(msg, sizeof msg, "RG-05 : fin de la trouée : l'écho revient en %.0f ms (50 à 100)", pas * DT * 1000);
        verifier(pas * DT >= 0.05f && pas * DT <= 0.1f, msg);
        const float avant = gG[0];
        pas = 0; float g = 1; while (g > 1e-6f && pas < 200) { const int n = s.pas(DT, 12.f, 0, true, r, gG, gD); g = n ? gG[0] : 0; pas++; }
        std::snprintf(msg, sizeof msg, "RG-05 : entrée dans un tunnel : l'écho des murs laisse la place à celui du tunnel en %.2f s (≥ 0,3)", pas * DT);
        verifier(avant > 0 && pas * DT >= 0.3f, msg);
        // changement de délai (mur à 10 m puis à 3 m) : fondu entre deux prises, aucun saut d'échantillon
        Echos e; e.build(R);
        std::vector<float> sin(48000), G(48000, 0.f), D(48000, 0.f);
        for (size_t i = 0; i < sin.size(); i++) sin[i] = 0.5f * std::sin(6.2831853f * 220 * (float)i / R);
        float sautAvant = 0, sautApres = 0;
        for (size_t a = 0; a < sin.size(); a += 480) {
            e.cible(0, a < 24000 ? 20 / 343.f : 6 / 343.f, 0.5f, 0.5f);
            e.process(sin.data() + a, G.data() + a, D.data() + a, 480);
        }
        for (size_t i = 4801; i < 24000; i++) sautAvant = std::max(sautAvant, std::abs(G[i] - G[i - 1]));
        for (size_t i = 24000; i < 30000; i++) sautApres = std::max(sautApres, std::abs(G[i] - G[i - 1]));
        std::snprintf(msg, sizeof msg, "RG-05 : délai qui passe de 58 à 17 ms : plus grand saut %.4f, contre %.4f avant (fondu entre deux prises, sans clic)", sautApres, sautAvant);
        verifier(sautApres <= sautAvant * 1.05f, msg);
    }

    // ---------- RG-06 ----------
    {
        std::snprintf(msg, sizeof msg, "RG-06 : D(δ = 0,1 m) = %.2f dB (11,7), D(0) = %.2f dB (4,8), D(1 m) = %.1f dB (plafond 20)", diffractionDb(0.1f), diffractionDb(0), diffractionDb(1));
        verifier(std::abs(diffractionDb(0.1f) - 10 * std::log10(3 + 20 * 2 * 0.1f / LAMBDA_1K)) < 0.01f && std::abs(diffractionDb(0.1f) - 11.7f) < 0.5f &&
                 std::abs(diffractionDb(0) - 4.77f) < 0.05f && diffractionDb(1) == 20.f && diffractionDb(-1) == 0.f, msg);
        const vec3 c{0, 0, 0}, rr{1, 0, 0}, ff{0, 0, 1};
        // ligne qui frôle le bord : détour par les deux coins du même côté
        const float d1 = detour({-20, 1.2f, 4.8f}, {20, 1.2f, 4.8f}, c, rr, ff, 5, 5, 12);   // r : x (profondeur), f : z (longueur)
        const float d1a = 2 * std::hypot(15.f, 0.2f) + 10 - 40;
        std::snprintf(msg, sizeof msg, "RG-06 : détour par les coins : %.4f m (géométrie %.4f m)", d1, d1a);
        verifier(std::abs(d1 - d1a) < 1e-3f, msg);
        const float d2 = detour({-20, 10.f, 0}, {20, 10.f, 0}, c, rr, ff, 5, 5, 10.5f);   // toit à 10,5 m, ligne à 10 m
        const float d2a = 2 * std::hypot(15.f, 0.5f) + 10 - 40;
        std::snprintf(msg, sizeof msg, "RG-06 : détour par le toit : %.4f m (géométrie %.4f m)", d2, d2a);
        verifier(std::abs(d2 - d2a) < 1e-3f, msg);
        verifier(detour({-20, 1.2f, 6}, {20, 1.2f, 6}, c, rr, ff, 5, 5, 12) < 0, "RG-06 : ligne qui passe à côté de l'immeuble : pas de masque");
        verifier(detour({-20, 14, 0}, {20, 14, 0}, c, rr, ff, 5, 5, 12) < 0, "RG-06 : ligne au-dessus du toit : pas de masque");
        const std::vector<ImmeubleMonde> im = {{c, rr, ff, 5, 5, 12}};
        std::snprintf(msg, sizeof msg, "RG-06 : voiture cachée derrière un immeuble de 10 m : %.1f dB (plafond 20)", masqueDb({-20, 1.2f, 0}, {20, 0.6f, 0}, im));
        verifier(masqueDb({-20, 1.2f, 0}, {20, 0.6f, 0}, im) == 20.f && masqueDb({-20, 1.2f, 8}, {20, 0.6f, 8}, im) == 0.f, msg);
    }

    // ---------- RG-07 ----------
    {
        Suivi s; Reflexion r[6]; float gG[6], gD[6];
        int n = 0; for (int k = 0; k < 60; k++) n = s.pas(DT, 0, 0, false, r, gG, gD);
        Echos e; e.build(R);
        std::vector<float> bus(4800), G(4800), D(4800), G0, D0;
        for (size_t i = 0; i < bus.size(); i++) bus[i] = 0.3f * std::sin(0.05f * (float)i);
        for (size_t i = 0; i < G.size(); i++) G[i] = D[i] = 0.1f * std::cos(0.01f * (float)i);
        G0 = G; D0 = D;
        for (int i = 0; i < PRISES; i++) e.cible(i, 0.01f, 0, 0);
        for (size_t a = 0; a < bus.size(); a += 480) e.process(bus.data() + a, G.data() + a, D.data() + a, 480);
        verifier(n == 0 && G == G0 && D == D0, "RG-07 : champ libre (aucune paroi) : rien n'est ajouté, à l'échantillon près");
    }

    // ---------- RG-08 ----------
    {
        Echos e; e.build(R);
        Reflexion r[6]; float gG[6], gD[6]; Suivi s;
        int n = 0; for (int k = 0; k < 30; k++) n = s.pas(DT, 6, 8, false, r, gG, gD);
        for (int i = 0; i < n; i++) e.cible(i, r[i].delai, gG[i], gD[i]);
        std::vector<float> bus(480, 0.1f), G(480), D(480);
        const int B = 3000;
        auto t0 = std::chrono::steady_clock::now();
        for (int b = 0; b < B; b++) e.process(bus.data(), G.data(), D.data(), 480);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / B;
        std::vector<ImmeubleMonde> im;
        for (int i = 0; i < 120; i++) im.push_back({{(float)(i % 12) * 25 - 150, 0, (float)(i / 12) * 30 - 150}, {1, 0, 0}, {0, 0, 1}, 6, 10, 15});
        t0 = std::chrono::steady_clock::now();
        float somme = 0;
        for (int b = 0; b < 200; b++) for (int k = 0; k < 13; k++) somme += masqueDb({0, 1.2f, 0}, {(float)k * 9 - 60, 0.6f, 80.f - (float)b * 0.1f}, im);
        const double msM = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / 200;
        std::snprintf(msg, sizeof msg, "RG-08 : écho, %d prises : %.3f ms par bloc de 10 ms (≤ 0,3) ; masques, 13 sources × 120 immeubles : %.3f ms par pas (≤ 0,2)%s", n, ms, msM, somme < 0 ? "" : "");
        verifier(n == 6 && ms <= 0.3 && msM <= 0.2, msg);
    }

    std::printf(echecs ? "test_audio_parois : %d ÉCHEC(S)\n" : "test_audio_parois : tout est conforme\n", echecs);
    return echecs ? 1 : 0;
}
