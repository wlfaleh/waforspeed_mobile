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

    // ======================= WFS-278 (cas ajoutés ; les cas de WFS-275 ci-dessus sont gardés tels quels) =======================
    // route droite vers −z (cap 0 : droite = +x, avant = −z, comme audio_system::preparerParois), départ du tour en z0
    auto monde = [](const std::vector<Immeuble>& v, float z0) {
        std::vector<ImmeubleMonde> out;
        for (const Immeuble& im : v)
            out.push_back({{im.centre.lat, 0, z0 - (float)(im.centre.seg * SEG_M + im.centre.avant)}, {1, 0, 0}, {0, 0, -1}, im.demiProf, im.demiLong, im.h});
        return out;
    };
    // écho d'une impulsion (parois a / b tenues fixes) : position du pic et amplitude du côté demandé, niveau de l'autre côté
    auto impulsion = [&](float a, float b, float alG, float alD, int cote, size_t& pic, float& amp, float& autre, Reflexion& r0, float& g0) {
        Echos e; e.build(R); Suivi su; Reflexion r[6]; float gG[6], gD[6];
        int n = 0; for (int k = 0; k < 30; k++) n = su.pas(DT, a, b, false, r, gG, gD, alG, alD);
        for (int i = 0; i < PRISES; i++) e.cible(i, i < n ? r[i].delai : 0.01f, i < n ? gG[i] : 0.f, i < n ? gD[i] : 0.f);
        const size_t M = 24000;
        std::vector<float> bus(M, 0.f), G(M, 0.f), D(M, 0.f), z(480, 0.f), zg(480), zd(480);
        for (int k = 0; k < 5; k++) e.process(z.data(), zg.data(), zd.data(), 480);
        bus[0] = 1;
        for (size_t s = 0; s < M; s += 480) e.process(bus.data() + s, G.data() + s, D.data() + s, 480);
        const std::vector<float>& y = cote < 0 ? G : D;
        const std::vector<float>& x = cote < 0 ? D : G;
        pic = 1; for (size_t i = 1; i + 1 < M; i++) if (std::abs(y[i]) > std::abs(y[pic])) pic = i;
        amp = std::abs(y[pic - 1]) + std::abs(y[pic]) + std::abs(y[pic + 1]);
        autre = std::abs(x[pic - 1]) + std::abs(x[pic]) + std::abs(x[pic + 1]);
        int i0 = 0; for (int i = 0; i < n; i++) if (r[i].cote == cote && r[i].ordre == 1) i0 = i;
        r0 = r[i0]; g0 = cote < 0 ? gG[i0] : gD[i0];
    };

    // ---------- RG-01 : absorption sourcée (pyroomacoustics, annexe de Vorländer 2008, bande de 1 kHz) ----------
    {
        auto perte = [](float al) { return 10 * std::log10(1 - al); };
        std::snprintf(msg, sizeof msg, "WFS-278 RG-01 : α (1 kHz) dur %.2f, béton %.2f, verre %.2f, pierre %.2f ; perte %.2f / %.2f dB par réflexion (≤ 0,3) ; avant : 0,05 (%.2f dB)",
                      ALPHA_DUR, ALPHA_BETON, ALPHA_VERRE, ALPHA_PIERRE, perte(ALPHA_DUR), perte(ALPHA_PIERRE), perte(0.05f));
        verifier(ALPHA_DUR == 0.03f && ALPHA_BETON == 0.03f && ALPHA_VERRE == 0.03f && ALPHA_PIERRE == 0.04f && ALPHA == ALPHA_DUR && -perte(ALPHA_PIERRE) <= 0.3f, msg);
        Reflexion r[6];
        const int n = reflexions(4.f, 6.f, r, ALPHA_PIERRE, ALPHA_VERRE);
        const float rG = std::sqrt(1 - ALPHA_PIERRE), rD = std::sqrt(1 - ALPHA_VERRE);
        const float att[6] = {1.5f / 8 * rG, 1.5f / 12 * rD, 1.5f / 20 * rG * rD, 1.5f / 20 * rG * rD, 1.5f / 28 * rG * rD * rG, 1.5f / 32 * rD * rG * rD};
        bool ok = n == 6; for (int i = 0; i < n && ok; i++) ok = std::abs(dB(r[i].gain) - dB(att[i])) < 0.01f;
        verifier(ok, "WFS-278 RG-01 : pierre à gauche, verre à droite : chaque réflexion perd 10·lg(1 − α) de chaque paroi touchée (ordres 1 à 3)");
    }

    // ---------- RG-02 : façades du casino, du centre commercial et des garages (Ville, tracé 2) ----------
    {
        ville::Plan pl; pl.objets.resize(400);
        pl.lieux.push_back({ville::CASINO, 20, 1, 60.f});
        pl.lieux.push_back({ville::CENTRE_COMMERCIAL, 150, -1, 84.f});
        pl.lieux.push_back({ville::GARAGE, 300, 1, 18.f});
        depuisVille(pl, 400);
        const std::string nomVille = "ville";
        const std::vector<Cotes>& v = lire(nomVille);
        const float Rw = (float)RW;
        verifier(v.size() == 400 && v[20][1].d == Rw + 12 && v[70][1].d == Rw + 12 && v[45][1].h == 14.5f && v[45][1].alpha == ALPHA_PIERRE && v[19][1].d == 0 && v[71][1].d == 0 &&
                 v[45][0].d == 0 && v[185][0].d == Rw + 31 && v[185][0].alpha == ALPHA_VERRE && v[185][1].d == 0 && v[307][1].d == Rw + 6.5f && v[307][1].alpha == ALPHA_DUR,
                 "WFS-278 RG-02 : casino (RW + 12, pierre), centre commercial (RW + 31, verre), garage (RW + 6,5) : paroi de leur côté, sur la longueur de la façade");
        const auto ims = immeublesVille(pl, 400);
        verifier(ims.size() == 3 && ims[0].centre.seg == 45 && std::abs(ims[0].centre.lat - (Rw + 12 + 12)) < 1e-4f && ims[0].demiLong == 30 && ims[0].h == 14.5f &&
                 std::abs(ims[1].centre.lat + (Rw + 31 + 13)) < 1e-4f && ims[2].h == 6.f,
                 "WFS-278 RG-02 : ces lieux sont aussi des obstacles des masques de la Ville (centre, profondeur, longueur, hauteur)");
        // écho réel : la voiture sur l'axe, casino à droite
        size_t pic; float amp, autre, g0; Reflexion r0;
        impulsion(0, Rw + 12, ALPHA, ALPHA_PIERRE, 1, pic, amp, autre, r0, g0);
        const float attD = 20 * std::log10(1.5f / (2 * (Rw + 12))) + 10 * std::log10(1 - ALPHA_PIERRE);
        std::snprintf(msg, sizeof msg, "WFS-278 RG-02 : casino à droite : écho à %zu échantillons (2·d / c = %.1f ± 1), %.2f dB (formule %.2f ± 0,5), %.1f dB plus fort à droite (≥ 9)",
                      pic, 2 * (Rw + 12) / 343.f * R, dB(amp), attD + dB(g0 / r0.gain), dB(amp) - dB(autre));
        verifier(std::abs((float)pic - 2 * (Rw + 12) / 343.f * R) <= 1.f && std::abs(dB(amp) - (attD + dB(g0 / r0.gain))) < 0.5f && std::abs(dB(r0.gain) - attD) < 0.01f &&
                 dB(amp) - dB(autre) >= 9, msg);
        impulsion(Rw + 31, 0, ALPHA_VERRE, ALPHA, -1, pic, amp, autre, r0, g0);
        const float attG = 20 * std::log10(1.5f / (2 * (Rw + 31))) + 10 * std::log10(1 - ALPHA_VERRE);
        std::snprintf(msg, sizeof msg, "WFS-278 RG-02 : centre commercial à gauche : écho à %zu échantillons (%.1f ± 1), %.2f dB (formule %.2f ± 0,5), %.1f dB plus fort à gauche",
                      pic, 2 * (Rw + 31) / 343.f * R, dB(amp), attG + dB(g0 / r0.gain), dB(amp) - dB(autre));
        verifier(std::abs((float)pic - 2 * (Rw + 31) / 343.f * R) <= 1.f && std::abs(dB(amp) - (attG + dB(g0 / r0.gain))) < 0.5f && std::abs(dB(r0.gain) - attG) < 0.01f &&
                 dB(amp) - dB(autre) >= 9, msg);
    }

    // ---------- RG-03 : obstacles déclarés par les cartes (Corniche, Campagne, Grand Prix) ----------
    {
        effacer("essai-obstacle");
        declarer("essai-obstacle", 1000, 100, -1, 10.f, 8.f, 12.f, 6.f, ALPHA_PIERRE);
        const std::string k1 = "essai-obstacle";
        const std::vector<Immeuble>& o1 = obstacles(k1);
        verifier(o1.size() == 1 && o1[0].centre.seg == 100 && std::abs(o1[0].centre.lat + 13.f) < 1e-5f && o1[0].demiProf == 3.f && o1[0].demiLong == 6.f && o1[0].h == 8.f &&
                 lire(k1)[100][0].d == 10.f && lire(k1)[100][0].alpha == ALPHA_PIERRE,
                 "WFS-278 RG-03 : paroi déclarée avec une profondeur : écho (comme WFS-275) et obstacle (boîte de la façade à d + profondeur, haute de h)");
        effacer("essai-roche");   // roche de la Corniche : un appel par segment, morceaux voisins réunis jusqu'à 15 m
        for (int n = 0; n < 30; n++) declarer("essai-roche", 1000, 200 + n, -1, 13.85f, 20.f - 0.1f * (float)n, (float)SEG_M, 25.f, ALPHA_PIERRE);
        const std::string k2 = "essai-roche";
        const std::vector<Immeuble>& o2 = obstacles(k2);
        std::snprintf(msg, sizeof msg, "WFS-278 RG-03 : roche : 30 segments → %zu obstacles (12, 12 et 6 segments ; 14,4 m au plus), hauteur la plus basse du morceau (%.2f m)",
                      o2.size(), o2.empty() ? 0.f : o2[0].h);
        verifier(o2.size() == 3 && std::abs(o2[0].demiLong - 7.2f) < 1e-3f && o2[0].centre.seg == 205 && std::abs(o2[0].centre.avant - 0.6f) < 1e-3f &&
                 std::abs(o2[0].h - 18.9f) < 1e-3f && std::abs(o2[2].demiLong - 3.6f) < 1e-3f && std::abs(o2[1].centre.lat + (13.85f + 12.5f)) < 1e-4f, msg);
        effacer("essai-loin"); effacer("essai-sans");
        declarer("essai-loin", 1000, 50, 1, 80.f, 10.f, 20.f, 10.f);   // au-delà de la portée de l'écho : obstacle seulement
        declarer("essai-sans", 1000, 50, 1, 12.f, 10.f, 20.f);         // accroche de WFS-275 sans profondeur : écho seulement
        const std::string k3 = "essai-loin", k4 = "essai-sans";
        verifier(obstacles(k3).size() == 1 && lire(k3).empty() && obstacles(k4).empty() && lire(k4)[50][1].d == 12.f,
                 "WFS-278 RG-03 : paroi au-delà de 40 m : obstacle sans écho ; paroi sans profondeur (WFS-275) : écho sans obstacle");
        // Corniche : adversaire dans le lacet suivant (branche parallèle à 45 m à gauche), derrière la roche haute de 13 m
        effacer("essai-corniche");
        for (int n = 0; n < 70; n++) declarer("essai-corniche", 1000, n, -1, (float)RW + 4.6f + 1.75f, 13.f, (float)SEG_M, (45.f + (float)RW + 1) / 2 - ((float)RW + 4.6f), ALPHA_PIERRE);
        const std::string k5 = "essai-corniche";
        const std::vector<ImmeubleMonde> roche = monde(obstacles(k5), 40.f);   // roche de z = 40 à z = −44
        const float mC = masqueDb({0, 1.2f, 0}, {-45, 0.6f, -10}, roche), mVisible = masqueDb({0, 1.2f, 0}, {-45, 0.6f, -200}, roche);
        std::snprintf(msg, sizeof msg, "WFS-278 RG-03 : Corniche : adversaire derrière la roche %.1f dB (≥ 20, plafond 25) ; au-delà du bout de la roche %.1f dB (0)", mC, mVisible);
        verifier(mC >= 20.f && mC <= 25.f && mVisible == 0.f, msg);
        // Campagne : maison (villa à 12 m à droite, 10 m de façade, 8 m de profondeur, 6 m de haut) ; une voiture derrière, une qui frôle le coin
        effacer("essai-campagne");
        declarer("essai-campagne", 1000, 50, 1, 12.f, 6.f, 10.f, 8.f, ALPHA_DUR);
        const std::string k6 = "essai-campagne";
        const std::vector<ImmeubleMonde> maison = monde(obstacles(k6), 60.f);   // centre en z = 0, de x = 12 à 20
        const vec3 ici{0, 1.2f, 0};
        const float mDerriere = masqueDb(ici, {40, 0.6f, 0}, maison);
        const vec3 frole{40, 0.6f, 9.6f};   // la ligne passe à 2 cm du coin (x = 20, z = 5) : détour de quelques millimètres
        const float dl = detour(ici, frole, maison[0].c, maison[0].r, maison[0].f, maison[0].demiProf, maison[0].demiLong, maison[0].h);
        const float mFrole = masqueDb(ici, frole, maison), formule = 10 * std::log10(3 + 20 * 2 * dl / LAMBDA_1K);
        std::snprintf(msg, sizeof msg, "WFS-278 RG-03 : Campagne : voiture derrière une maison %.1f dB (plafond 20) ; qui frôle le coin : détour %.4f m, D = %.2f dB (formule %.2f ± 0,5)",
                      mDerriere, dl, mFrole, formule);
        verifier(mDerriere == 20.f && dl > 0 && std::abs(mFrole - formule) < 0.5f && mFrole < 20.f, msg);
        // Grand Prix : garages (à 20 m à gauche, 12 m de profondeur, 7 m de haut, 60 m de long), voiture sur la ligne opposée à 60 m
        effacer("essai-circuit");
        for (int g = 0; g < 10; g++) declarer("essai-circuit", 1000, 5 + g * 5, -1, 20.f, 7.f, 6.f, 12.f, ALPHA_DUR);
        const std::string k7 = "essai-circuit";
        const std::vector<ImmeubleMonde> garages = monde(obstacles(k7), 36.f);
        const float mG = masqueDb(ici, {-60, 0.6f, 0}, garages), mLibre = masqueDb(ici, {0, 0.6f, -100}, garages);
        std::snprintf(msg, sizeof msg, "WFS-278 RG-03 : Grand Prix : voiture derrière les garages %.1f dB (≥ 20, plafond 25), voiture devant sur la piste %.1f dB (0) ; %zu obstacles",
                      mG, mLibre, garages.size());
        verifier(mG >= 20.f && mG <= 25.f && mLibre == 0.f && garages.size() <= 5, msg);
    }

    // ---------- RG-04 : plusieurs obstacles sur la ligne (ISO 9613-2 éq. 14, CNOSSOS-EU) ----------
    {
        const float x = 5 * LAMBDA_1K / 30.f, C3 = (1 + x * x) / (1.f / 3 + x * x);
        const float f1 = 10 * std::log10(3 + 20 * (2 * 0.002f / LAMBDA_1K) * C3);
        std::snprintf(msg, sizeof msg, "WFS-278 RG-04 : D(z = 2 mm, e = 30 m) = %.2f dB (formule %.2f, C3 = %.3f) ; e = 0 : %.2f dB (= simple %.2f) ; z = 1 m : %.1f dB (plafond 25)",
                      diffractionMultipleDb(0.002f, 30), f1, C3, diffractionMultipleDb(0.05f, 0), diffractionDb(0.05f), diffractionMultipleDb(1, 10));
        verifier(std::abs(diffractionMultipleDb(0.002f, 30) - f1) < 0.01f && std::abs(diffractionMultipleDb(0.05f, 0) - diffractionDb(0.05f)) < 1e-4f &&
                 diffractionMultipleDb(1, 10) == 25.f && diffractionMultipleDb(-1, 5) == 0.f && C3 > 2.9f && C3 < 3.f, msg);
        // deux immeubles de même hauteur, la ligne passe 20 cm sous les toits : chemin tendu par-dessus les deux
        const vec3 rr{1, 0, 0}, ff{0, 0, 1};
        std::vector<ImmeubleMonde> deux = {{{-10, 0, 0}, rr, ff, 5, 20, 10.2f}, {{10, 0, 0}, rr, ff, 5, 20, 10.2f}};
        const vec3 a{-30, 10, 0}, s{30, 10, 0};
        const float z2 = 2 * std::hypot(15.f, 0.2f) + 30 - 60, d2 = diffractionMultipleDb(z2, 30);
        const float seul = std::max(diffractionDb(detour(a, s, deux[0].c, rr, ff, 5, 20, 10.2f)), diffractionDb(detour(a, s, deux[1].c, rr, ff, 5, 20, 10.2f)));
        std::snprintf(msg, sizeof msg, "WFS-278 RG-04 : deux toits à 10,2 m : %.2f dB (chemin tendu : z = %.4f m, e = 30 m → %.2f ± 0,5) ; le plus fort seul : %.2f dB",
                      masqueDb(a, s, deux), z2, d2, seul);
        verifier(std::abs(masqueDb(a, s, deux) - d2) < 0.5f && masqueDb(a, s, deux) > seul, msg);
        // le second plus haut (11 m) : le premier toit est sous le chemin tendu (une seule arête efficace : épaisseur e = 10 m)
        deux[1].h = 11.f;
        const float z3 = std::hypot(35.f, 1.f) + 10 + std::hypot(15.f, 1.f) - 60, d3 = diffractionMultipleDb(z3, 10);
        std::snprintf(msg, sizeof msg, "WFS-278 RG-04 : toits à 10,2 et 11 m : %.2f dB (z = %.4f m, e = 10 m → %.2f ± 0,5)", masqueDb(a, s, deux), z3, d3);
        verifier(std::abs(masqueDb(a, s, deux) - d3) < 0.5f, msg);
        // autour : deux immeubles bas et longs côte à côte, la ligne frôle leurs coins du même côté
        const std::vector<ImmeubleMonde> cote2 = {{{-10, 0, 0}, rr, ff, 5, 5, 40}, {{10, 0, 0}, rr, ff, 5, 5, 40}};
        const vec3 a2{-30, 1.2f, 4.9f}, s2{30, 1.2f, 4.9f};
        const float z4 = 2 * std::hypot(15.f, 0.1f) + 30 - 60, d4 = diffractionMultipleDb(z4, 30);
        std::snprintf(msg, sizeof msg, "WFS-278 RG-04 : autour de deux immeubles : %.2f dB (z = %.4f m, e = 30 m → %.2f ± 0,5)", masqueDb(a2, s2, cote2), z4, d4);
        verifier(std::abs(masqueDb(a2, s2, cote2) - d4) < 0.5f, msg);
        const std::vector<ImmeubleMonde> hauts = {{{-10, 0, 0}, rr, ff, 5, 30, 20}, {{10, 0, 0}, rr, ff, 5, 30, 20}};
        std::snprintf(msg, sizeof msg, "WFS-278 RG-04 : voiture cachée derrière deux immeubles hauts : %.1f dB (plafond 25)", masqueDb({-30, 1.2f, 0}, {30, 0.6f, 0}, hauts));
        verifier(masqueDb({-30, 1.2f, 0}, {30, 0.6f, 0}, hauts) == 25.f, msg);
        // un seul obstacle : comportement de WFS-275 inchangé (diffraction simple, plafond 20)
        const std::vector<ImmeubleMonde> un = {{{0, 0, 0}, rr, ff, 5, 5, 12}};
        const vec3 a5{-20, 1.2f, 4.8f}, s5{20, 1.2f, 4.8f};
        verifier(masqueDb(a5, s5, un) == diffractionDb(detour(a5, s5, un[0].c, rr, ff, 5, 5, 12)) && masqueDb({-20, 1.2f, 0}, {20, 0.6f, 0}, un) == 20.f,
                 "WFS-278 RG-04 : un seul obstacle sur la ligne : diffraction simple de WFS-275, à l'identique (plafond 20)");
    }

    // ---------- RG-05 : rien changé ailleurs ----------
    {
        Reflexion r1[6], r2[6];
        const int n1 = reflexions(5.f, 9.f, r1), n2 = reflexions(5.f, 9.f, r2, ALPHA, ALPHA);
        bool ok = n1 == n2; for (int i = 0; i < n1 && ok; i++) ok = r1[i].delai == r2[i].delai && r1[i].gain == r2[i].gain && r1[i].cote == r2[i].cote;
        const vec3 rr{1, 0, 0}, ff{0, 0, 1};
        const std::vector<ImmeubleMonde> loin = {{{0, 0, 30}, rr, ff, 5, 5, 12}, {{0, 0, -30}, rr, ff, 5, 5, 12}};
        verifier(ok && masqueDb({-20, 1.2f, 0}, {20, 0.6f, 0}, loin) == 0.f && masqueDb({-20, 1.2f, 0}, {20, 0.6f, 0}, {}) == 0.f,
                 "WFS-278 RG-05 : parois sans matériau précisé : réflexions identiques ; aucun obstacle sur la ligne : aucun masque (0 dB)");
    }

    // ---------- RG-06 : coût des masques avec la roche de la Corniche (le cas le plus chargé) ----------
    {
        effacer("essai-cout");
        for (int n = 0; n < 250; n++) declarer("essai-cout", 1000, n, -1, (float)RW + 6.35f, 13.f + 3.f * std::sin((float)n * 0.051f), (float)SEG_M, 20.f, ALPHA_PIERRE);
        for (int n = 0; n < 250; n += 9) declarer("essai-cout", 1000, n, 1, 14.f, 7.f, 10.f, 8.f, ALPHA_DUR);   // villas de l'autre côté
        const std::string kc = "essai-cout";
        const std::vector<ImmeubleMonde> ob = monde(obstacles(kc), 150.f);
        const auto t0 = std::chrono::steady_clock::now();
        float somme = 0;
        for (int b = 0; b < 200; b++) for (int k = 0; k < 13; k++) somme += masqueDb({0, 1.2f, 0}, {-45.f + (k % 2) * 90.f, 0.6f, 120.f - (float)k * 20 - (float)b * 0.1f}, ob);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / 200;
        std::snprintf(msg, sizeof msg, "WFS-278 RG-06 : masques, 13 sources × %zu obstacles (roche et villas sur 300 m) : %.3f ms par pas (≤ 0,2)%s", ob.size(), ms, somme < 0 ? "" : "");
        verifier(ms <= 0.2, msg);
    }

    std::printf(echecs ? "test_audio_parois : %d ÉCHEC(S)\n" : "test_audio_parois : tout est conforme\n", echecs);
    return echecs ? 1 : 0;
}
