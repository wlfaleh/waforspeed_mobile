// WFS-151 (ST-1, RC-05) : unités photométriques, exposition physique, soleil, ciel de référence, harmoniques sphériques,
// préfiltrage GGX et table BRDF (src/render/eclairage_unites.h), et cohérence des constantes avec le référentiel du réel
// (references/reel/photometrie.json). Un écart rend le test rouge.
// usage : test_fidelite_lumiere [dossier du dépôt]   (par défaut WFS_SOURCE_DIR, fixé par CMakeLists.txt)
#include <cmath>
#include <cstdio>
#include <string>
#include "../src/render/bench_realisme.h"
#include "../src/render/eclairage_unites.h"
#include "../src/render/atmosphere_physique.h"   // WFS-207
#include "../src/render/eclairage_exposition.h"   // WFS-213
using namespace wfs::lumiere;

#ifndef WFS_SOURCE_DIR
#define WFS_SOURCE_DIR "."
#endif

static int echecs = 0, verifs = 0;
#define VERIFIE(c, ...) do { verifs++; if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)
static bool proche(double a, double b, double relatif) { return std::fabs(a - b) <= relatif * std::fabs(b); }

// demi-flottant → flottant (IEEE 754 binaire16), pour vérifier versDemi
static float demi(uint16_t h) {
    const int s = (h >> 15) & 1, e = (h >> 10) & 31, m = h & 1023;
    const float v = e == 0 ? std::ldexp((float)m, -24) : e == 31 ? INFINITY : std::ldexp((float)(m + 1024), e - 25);
    return s ? -v : v;
}
// température de couleur proximale (McCamy 1992), comme le banc du réalisme
static double mccamy(const double rgb[3]) {
    const double X = 0.4124564 * rgb[0] + 0.3575761 * rgb[1] + 0.1804375 * rgb[2], Y = 0.2126729 * rgb[0] + 0.7151522 * rgb[1] + 0.0721750 * rgb[2],
                 Z = 0.0193339 * rgb[0] + 0.1191920 * rgb[1] + 0.9503041 * rgb[2];
    const double x = X / (X + Y + Z), y = Y / (X + Y + Z), n = (x - 0.3320) / (0.1858 - y);
    return 449 * n * n * n + 3525 * n * n + 6823.3 * n + 5520.33;
}

int main(int argc, char** argv) {
    const std::string racine = argc > 1 ? argv[1] : WFS_SOURCE_DIR;
    // ---- exposition (RG-02)
    VERIFIE(std::fabs(ev100(16, 1.0 / 125, 100) - 14.966) < 1e-3, "EV100 de f/16, 1/125 s, 100 ISO : 14,97 attendu (%.4f)", ev100(16, 1.0 / 125, 100));
    VERIFIE(std::fabs(ev100(16, 1.0 / 125, 200) - 13.966) < 1e-3, "200 ISO : une valeur de moins");
    VERIFIE(proche(exposition(15), 1.0 / (1.2 * 32768), 1e-12), "exposition(15) = 1 / (1,2 × 2^15)");
    VERIFIE(proche(exposition(14) / exposition(15), 2, 1e-12), "un EV de moins : deux fois plus de lumière");
    for (double ev : {-3.0, 4.0, 15.0}) VERIFIE(std::fabs(ev100Luminance(luminanceEv100(ev)) - ev) < 1e-9, "posemètre réciproque à EV %.0f", ev);
    VERIFIE(proche(luminanceEv100(15), 4096, 1e-9), "EV100 15 ↔ 4 096 cd/m² (K = 12,5)");
    // charte grise 18 % au soleil de 100 klx : L = 0,18 × 100 000 / π ≈ 5 730 cd/m² → EV100 ≈ 15,5 (règle du « f/16 ensoleillé »)
    VERIFIE(std::fabs(ev100Luminance(0.18 * 100000 / PI_L) - 15.48) < 0.01, "charte grise au soleil : EV100 15,48 (%.3f)", ev100Luminance(0.18 * 100000 / PI_L));
    // adaptation : constante de temps 0,5 s vers le clair, 2 s vers le sombre
    VERIFIE(std::fabs(adapter(10, 15, 0.5) - (10 + 5 * (1 - std::exp(-1.0)))) < 1e-9, "adaptation vers le clair : τ = 0,5 s");
    VERIFIE(std::fabs(adapter(15, 10, 2.0) - (15 - 5 * (1 - std::exp(-1.0)))) < 1e-9, "adaptation vers le sombre : τ = 2 s");
    VERIFIE(adapter(10, 15, 0) == 10, "dt nul : aucune adaptation");
    VERIFIE(EV_JOUR_MAX == 15 && EV_NUIT_MIN == 3 && EV_NUIT_MAX == 5, "bornes de l'exposition par moment");
    {   // histogramme : 100 cellules de poids égal, log2 L = 0 … 99 ; bande 80 à 98,3 % : cellules 80 à 97 et 0,3 de la 98
        std::vector<Cellule> c; for (int i = 0; i < 100; i++) c.push_back({(double)i, 1});
        const double m = std::log2(moyenneHistogramme(c)), att = (1593 + 0.3 * 98) / 18.3;
        VERIFIE(std::fabs(m - att) < 1e-6, "histogramme : bande 80-98,3 %% (%.3f)", m);
        std::vector<Cellule> u(10, Cellule{3, 2});
        VERIFIE(proche(moyenneHistogramme(u), 8, 1e-9), "histogramme uniforme : la luminance de toutes les cellules");
        VERIFIE(moyenneHistogramme({}) == 0, "histogramme vide : 0");
    }
    // ---- soleil (RG-01)
    VERIFIE(std::fabs(masseAir(90) - 1) < 1e-3, "masse d'air au zénith : 1 (%.5f)", masseAir(90));
    VERIFIE(proche(masseAir(0), 38.0868, 0.005), "masse d'air à l'horizon : 38,09 ± 0,5 %% (table de Kasten et Young 1989 ; formule approchée) (%.3f)", masseAir(0));
    VERIFIE(eclairementSoleilNormal(-1) == 0, "soleil sous l'horizon : 0 lx");
    double prec = 0; bool croissant = true;
    for (double h = 0; h <= 90; h += 5) { const double e = eclairementSoleilNormal(h); croissant = croissant && e > prec; prec = e; }
    VERIFIE(croissant, "éclairement du soleil croissant avec la hauteur");
    VERIFIE(eclairementSoleilNormal(90) < E_SOLAIRE_HORS_ATMOSPHERE, "atmosphère : moins que hors atmosphère");
    VERIFIE(std::fabs(angleSolideSoleil() - 6.79e-5) < 0.05e-5, "angle solide du soleil ≈ 6,8 × 10⁻⁵ sr (%.4g)", angleSolideSoleil());
    // ---- ciel de référence CIE type 12 (M-04)
    for (double h : {20.0, 45.0, 70.0}) {
        const double K = cieFacteur(h), hs = h * PI_L / 180, s[3] = {std::cos(hs), std::sin(hs), 0};
        // contrôle indépendant de l'intégrale (cellules 3 fois plus fines, en cos Z) : E = ∫ L cos Z dω
        double E = 0; const int NZ = 192, NA = 384;
        for (int i = 0; i < NZ; i++) for (int j = 0; j < NA; j++) {
            const double cz = (i + 0.5) / NZ, sz = std::sqrt(1 - cz * cz), A = (j + 0.5) / NA * 2 * PI_L;
            const double d[3] = {sz * std::cos(A), cz, sz * std::sin(A)};
            E += K * cieRelative(d, s) * cz * (1.0 / NZ) * (2 * PI_L / NA);
        }
        VERIFIE(proche(E, eclairementCielDiffus(h), 0.005), "ciel h %.0f° : éclairement horizontal %.1f lx contre %.1f lx (± 0,5 %%)", h, E, eclairementCielDiffus(h));
        const double z[3] = {0, 1, 0};
        VERIFIE(proche(K * cieRelative(z, s), cieLuminanceZenith(h), 1e-6), "ciel h %.0f° : luminance du zénith", h);
    }
    VERIFIE(eclairementCielDiffus(H_MIDI_DEG) == E_CIEL_CLAIR_MIDI, "ciel de midi : 6 000 lx (référentiel)");
    VERIFIE(std::fabs(cieF(PI_L / 2) - 1) < 1e-12, "CIE : f(π/2) = 1");
    VERIFIE(std::fabs(ciePhi(0) - (1 - std::exp(-0.32))) < 1e-12 && ciePhi(PI_L / 2) == 1, "CIE : φ(0) = 1 − e^−0,32, φ(π/2) = 1");
    // ---- couleur des lumières
    for (double T : {2600.0, 4100.0, 5778.0, 6500.0}) {
        double c[3]; couleurTemperature(T, c);
        VERIFIE(std::fabs(luminanceRgb(c) - 1) < 1e-9, "T %.0f K : luminance relative 1", T);
        VERIFIE(std::fabs(mccamy(c) - T) < 50, "T %.0f K : McCamy %.0f K (± 50 K)", T, mccamy(c));
    }
    // ---- cubes, harmoniques sphériques, préfiltrage, table BRDF (RG-03)
    double tot = 0; for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) tot += 6 * angleSolideTexel(x, y, 8);
    VERIFIE(std::fabs(tot - 4 * PI_L) < 1e-9, "angles solides du cube : 4π (%.12f)", tot);
    bool aller = true;
    for (int f = 0; f < 6; f++) for (int y = 0; y < 4; y++) for (int x = 0; x < 4; x++) {
        double d[3], s, t; directionTexel(f, (x + 0.5) / 4, (y + 0.5) / 4, d);
        const int g = faceDirection(d, s, t);
        aller = aller && g == f && std::fabs(s - (x + 0.5) / 4) < 1e-9 && std::fabs(t - (y + 0.5) / 4) < 1e-9;
    }
    VERIFIE(aller, "texel → direction → texel (convention des cubemaps Vulkan)");
    {   // environnement uniforme L = 1 : irradiance π partout ; préfiltré = 1 à toute rugosité
        CubeF c; c.allouer(16, 5); for (auto& f : c.faces) std::fill(f.begin(), f.end(), 1.f); c.reduire();
        const Sh9 sh = projeterSh9(c); double E[3]; irradianceSh9(sh, 0.3, 0.8, -0.52, E);
        VERIFIE(proche(E[0], PI_L, 0.002), "ciel uniforme : irradiance π (%.5f)", E[0]);
        double R[3] = {0.6, 0.0, 0.8}, v[3];
        for (double r : {0.0, 0.4, 1.0}) { prefiltrerTexel(c, R, r, 64, v); VERIFIE(proche(v[1], 1, 0.001), "préfiltrage d'un cube uniforme, rugosité %.1f (%.5f)", r, v[1]); }
    }
    {   // L = max(y, 0) : irradiance vers le haut 2π/3 (SH9 : ≈ 2 % d'erreur attendue sur une demi-sphère)
        CubeF c; c.allouer(32, 1);
        for (int f = 0; f < 6; f++) for (int y = 0; y < 32; y++) for (int x = 0; x < 32; x++) {
            double d[3]; directionTexel(f, (x + 0.5) / 32, (y + 0.5) / 32, d);
            for (int q = 0; q < 3; q++) c.face(0, f)[((size_t)y * 32 + x) * 3 + q] = (float)std::max(d[1], 0.0);
        }
        const Sh9 sh = projeterSh9(c); double E[3]; irradianceSh9(sh, 0, 1, 0, E);
        VERIFIE(proche(E[0], 2 * PI_L / 3, 0.03), "ciel en cos : irradiance 2π/3 vers le haut (%.4f)", E[0]);
        irradianceSh9(sh, 0, -1, 0, E);
        VERIFIE(E[0] < 0.1, "ciel en cos : presque rien vers le bas (%.4f)", E[0]);
    }
    {   // table BRDF : peu de perte d'énergie pour une surface lisse vue de face, F0 seul quand B → 0
        double A, B; tableBrdf(1.0, 0.05, 256, A, B);
        VERIFIE(A + B > 0.95 && A + B <= 1.01, "table BRDF lisse, de face : A + B ≈ 1 (%.4f)", A + B);
        VERIFIE(B < 0.01, "table BRDF de face : B ≈ 0 (%.4f)", B);
        double A2, B2; tableBrdf(0.1, 0.05, 256, A2, B2);
        VERIFIE(B2 > B, "table BRDF rasante : plus de Fresnel (%.4f > %.4f)", B2, B);
    }
    for (float f : {0.f, 1.f, 0.18f, 5730.f, 1e-5f, 70000.f}) { const float g = demi(versDemi(f)); VERIFIE(std::fabs(g - std::min(f, 65504.f)) <= std::max(1e-7f, std::min(f, 65504.f) * 1e-3f), "demi-flottant %.6g → %.6g", f, g); }
    // ---- cohérence avec le référentiel du réel (photometrie.json)
    wfs::reel::Json j; std::string e;
    if (!wfs::reel::lireJson(racine + "/references/reel/photometrie.json", j, e)) { std::printf("ÉCHEC : %s\n", e.c_str()); return 1; }
    auto val = [&](const char* id, const char* cle) { if (const wfs::reel::Json* ms = j.get("mesures")) for (const auto& m : ms->a) if (m.texte("id") == id) return m.nombre(cle, -1); return -1.0; };
    VERIFIE(val("lum.ciel_clair.intensite", "valeur") == E_CIEL_CLAIR_MIDI, "ciel clair : 6 000 lx du référentiel");
    VERIFIE(val("lum.pleine_lune.intensite", "valeur") == E_PLEINE_LUNE, "pleine lune : 0,32 lx du référentiel");
    VERIFIE(val("lum.nuit_sans_lune.intensite", "valeur") == E_NUIT_SANS_LUNE, "nuit sans lune : 0,002 lx du référentiel");
    VERIFIE(val("lum.pleine_lune.temperature", "valeur") == T_PLEINE_LUNE && val("lum.soleil_midi.temperature", "valeur") == T_SOLEIL_MIDI, "températures du référentiel");
    const double eMidi = val("lum.soleil_midi.intensite", "typique");
    const double eh70 = eclairementSoleilNormal(H_MIDI_DEG) * std::sin(H_MIDI_DEG * PI_L / 180);
    VERIFIE(proche(eh70, eMidi, 0.05), "soleil à 70° : %.0f lx au sol contre %.0f lx (± 5 %%, photométrie)", eh70, eMidi);
    // ---- WFS-207 : astres calculés et atmosphère (atmosphere_physique.h)
    {
        namespace A = wfs::atmosphere;
        const wfs::vec3 est = A::directionMonde(0, 90), nord = A::directionMonde(0, 0);
        VERIFIE(std::fabs(est.x - 1) < 1e-5 && std::fabs(nord.z + 1) < 1e-5, "repère du monde : est = +X, nord = −Z");
        const A::Astre so = A::soleil(A::lieu().heureJourUtc);
        VERIFIE(so.hauteur > 68 && so.hauteur < 71 && so.azimut > 170 && so.azimut < 190, "soleil de midi solaire (29 juin, 43,55° N) : h %.2f°, az %.2f°", so.hauteur, so.azimut);
        // pleine lune du 29 juin 2026 à 23 h 57 UTC (USNO, « Phases of the Moon ») : fraction éclairée ≈ 1 à 22 h 30 UTC
        const A::Astre lu = A::lune(A::lieu().heureNuitUtc);
        VERIFIE(lu.fraction > 0.98 && lu.hauteur > 5 && lu.hauteur < 30, "lune de 0 h 30 : fraction %.3f, h %.2f° (pleine lune basse au sud en été)", lu.fraction, lu.hauteur);
        const A::Ciel c45 = A::ciel(A::directionMonde(45, 180)), c10 = A::ciel(A::directionMonde(10, 180));
        double xz, yz, xh, yh; A::chromaticite(c45.zenith, xz, yz); A::chromaticite(c45.horizon, xh, yh);
        VERIFIE(xz > 0.20 && xz < 0.28 && yz > 0.20 && yz < 0.30 && xh > xz, "ciel, soleil à 45° : zénith bleu (x %.3f y %.3f), horizon plus pâle (x %.3f)", xz, yz, xh);
        VERIFIE(c10.soleil[0] > c10.soleil[1] && c10.soleil[1] > c10.soleil[2], "soleil à 10° rougi par la traversée de l'atmosphère");
        const double V = 3.912 / c45.extY / 1000;
        VERIFIE(V > 30 && V < 60, "visibilité météorologique %.1f km (aérosols τ = 0,09, trouble de Linke ≈ 2)", V);
    }
    // ---- WFS-230 (RG-01) : borne basse de la nuit selon l'éclairement reçu (posemètre incident C = 250, ISO 2720)
    VERIFIE(std::fabs(evIncidentNuit(20) - 3) < 1e-9, "rue éclairée (20 lx) : EV 3 (%.3f)", evIncidentNuit(20));
    VERIFIE(std::fabs(evIncidentNuit(1) - std::log2(0.4)) < 1e-9, "1 lx : EV −1,32 (%.3f)", evIncidentNuit(1));
    VERIFIE(evIncidentNuit(0.094) == EV_LUNE_MIN && evIncidentNuit(0) == EV_LUNE_MIN && evIncidentNuit(std::nan("")) == EV_REF_NUIT, "clair de lune : −2 ; NaN : EV de référence 4");
    VERIFIE(std::fabs(evIncidentNuit(0.625) - EV_LUNE_MIN) < 1e-9, "borne −2 atteinte à 0,625 lx (%.3f)", evIncidentNuit(0.625));
    VERIFIE(evIncidentNuit(1000) == EV_NUIT_MAX && evIncidentNuit(0.01) == EV_LUNE_MIN && evIncidentNuit(INFINITY) == EV_REF_NUIT && std::fabs(evIncidentNuit(20) - 3) < 1e-9,
            "exposition incidente de nuit : −2 à 5, 20 lx → 3");
    VERIFIE(proche(eclairementLampe(15, 0, 8), 15, 1e-12) && proche(eclairementLampe(15, 8, 8), 15 / std::pow(2.0, 1.5), 1e-12), "lampe : 15 lx au pied, ÷ 2^1,5 à d = h");
    {   // passage continu en s'éloignant d'une lampe de 8 m (+ 0,094 lx de lune) : borne continue (au plus 0,3 EV par mètre ; l'adaptation de l'œil la lisse dans le temps)
        double prec = evIncidentNuit(eclairementLampe(15, 0, 8) + 0.094), sautMax = 0, d2 = -1;
        for (double d = 0.5; d <= 80; d += 0.5) {
            const double e = evIncidentNuit(eclairementLampe(15, d, 8) + 0.094);
            sautMax = std::max(sautMax, std::fabs(e - prec) / 0.5); prec = e;
            if (d2 < 0 && e <= EV_LUNE_MIN + 1e-9) d2 = d;
        }
        VERIFIE(sautMax < 0.3 && d2 > 15 && d2 < 30, "passage lampe → rase campagne : pente max %.3f EV/m, borne −2 à %.1f m", sautMax, d2);
        std::printf("WFS-230 : borne de nuit −2 atteinte à %.1f m d'une lampe de 15 lx (8 m), pente max %.3f EV/m\n", d2, sautMax);
    }

    // ---- WFS-213 (RG-03) : 3 enchaînements init / shutdown, mesures valides puis invalides (NaN, infini, sonde absurde de
    // 1,36·10¹¹ cd/m² du banc vitrines) : l'EV reste dans les bornes du jour (10 à 15), jamais non fini
    {
        Exposition x;
        const double nan = std::nan(""), inf = INFINITY;
        for (int cycle = 0; cycle < 3; cycle++) {
            x.nouvelleScene(false);   // init (voiture suivante)
            VERIFIE(x.ev == EV_REF_JOUR && !x.init, "cycle %d : EV remis à la valeur du jour (%.2f)", cycle, x.ev);
            for (double L : {nan, inf, 1.36e11, -1.0, 0.0}) {
                x.image(L, EV_JOUR_MIN, EV_JOUR_MAX, 1.0 / 60);
                VERIFIE(std::isfinite(x.ev) && x.ev >= EV_JOUR_MIN && x.ev <= EV_JOUR_MAX, "cycle %d : mesure %g : EV %.3f dans [10 ; 15]", cycle, L, x.ev);
            }
            x.image(3458, EV_JOUR_MIN, EV_JOUR_MAX, 1.0 / 60);   // première mesure valide : recalage direct
            VERIFIE(std::fabs(x.ev - ev100Luminance(3458)) < 1e-9, "cycle %d : recalé sur la 1re mesure valide (EV %.3f)", cycle, x.ev);
            for (int i = 0; i < 120; i++) x.image(i % 2 ? nan : 3000, EV_JOUR_MIN, EV_JOUR_MAX, 1.0 / 60);
            VERIFIE(std::isfinite(x.ev) && x.ev >= EV_JOUR_MIN && x.ev <= EV_JOUR_MAX, "cycle %d : course mêlée de mesures invalides : EV %.3f", cycle, x.ev);
        }
        const double kBon[6] = {1e-4, 1e-4, 1e-4, 1e-4, 1e-4, 1e-4}, kTrou[6] = {1e-4, 0, 1e-4, -1, 1e-4, 1e-4};
        VERIFIE(kFacesValides(kBon) && !kFacesValides(kTrou), "capture : face sans exposition connue refusée");
        VERIFIE(!luminanceValide(1.36e11) && !luminanceValide(nan) && luminanceValide(3458) && luminanceValide(0.0912), "sonde : luminance absurde ou non finie refusée");
    }
    std::printf("fidelite_lumiere : %d vérifications, %d échec(s)\n", verifs, echecs);
    return echecs ? 1 : 0;
}
