// WaForSpeed — WFS-214 (épic E13) : passe finale (shaders/post.frag).
//   RG-01 (ANO-109) : tramage avant la conversion 8 bits : aucun palier dans un dégradé, à toute clarté, sortie sRGB matérielle ou non ;
//   RG-05 : vision de nuit (WFS-230) seulement loin des lumières : la Golf bleue sous un lampadaire garde sa peinture (ΔE2000 ≤ 10 avec la
//           même image sans vision de nuit) ; en rase campagne, la vision de nuit ne change pas.
// Copie C++ des calculs du shader : src/render/eclairage_exposition.h (visionNuit) et la fonction tramer() ci-dessous ; le texte du
// shader est relu pour vérifier que ce sont les mêmes constantes.
#include <cmath>
#include <cstdio>
#include <string>
#include "../src/render/bench_realisme.h"        // Lab, deltaE2000
#include "../src/render/eclairage_exposition.h"  // visionNuit, poidsScotopique, exposition

using namespace wfs::lumiere;
using wfs::reel::Lab; using wfs::reel::deltaE2000; using wfs::reel::lineaireVersLab;

static int g_echecs = 0;
#define VERIFIE(c, ...) do { if (c) { std::printf("  ok   : "); } else { std::printf("  ÉCHEC : "); g_echecs++; } std::printf(__VA_ARGS__); std::printf("\n"); } while (0)

// transferts sRGB exacts (IEC 61966-2-1), comme srgbCode / srgbLineaire de post.frag
static double code(double l) { l = std::clamp(l, 0.0, 1.0); return l <= 0.0031308 ? l * 12.92 : 1.055 * std::pow(l, 1 / 2.4) - 0.055; }
static double lineaire(double s) { return s <= 0.04045 ? s / 12.92 : std::pow((s + 0.055) / 1.055, 2.4); }
// conversion 8 bits de l'écran (sortie sRGB matérielle : l'écran encode la valeur linéaire écrite par le shader)
static int encodeEcran(double lin) { return (int)std::lround(std::clamp(code(lin), 0.0, 1.0) * 255.0); }

// moyenne, sur toutes les valeurs du bruit, du code 8 bits affiché pour une valeur linéaire v : un tramage juste donne en moyenne
// exactement le code continu (aucun palier) ; l'écart maximal sur un dégradé est la hauteur des marches qui restent
static double residuMax(bool nouveau, double vMin, double vMax) {
    const int N = 64;   // valeurs du bruit (grille régulière : moyenne exacte, résultat reproductible)
    double pire = 0;
    for (int i = 0; i <= 4000; i++) {
        const double v = vMin + (vMax - vMin) * i / 4000.0, attendu = code(v) * 255.0;
        double somme = 0;
        for (int a = 0; a < N; a++) for (int b = 0; b < (nouveau ? N : 1); b++) {
            const double ua = (a + 0.5) / N, ub = (b + 0.5) / N;
            int q;
            if (nouveau) q = encodeEcran(lineaire(std::clamp(code(v) + (ua + ub - 1.0) / 255.0, 0.0, 1.0)));   // post.frag (WFS-214)
            else q = encodeEcran(v + (ua - 0.5) / 255.0);                                                      // post.frag avant
            somme += q;
        }
        pire = std::max(pire, std::fabs(somme / (nouveau ? N * N : N) - attendu));
    }
    return pire;
}

static std::string lire(const char* chemin) {
    std::string t; FILE* f = std::fopen(chemin, "rb"); char b[4096]; size_t n;
    while (f && (n = std::fread(b, 1, sizeof b, f)) > 0) t.append(b, n);
    if (f) std::fclose(f);
    return t;
}

// couleur affichée (ACES + sRGB, comme post.frag) d'une couleur pré-exposée, en L*a*b*
static double aces(double x) { return std::clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0); }
static Lab affichee(const double c[3]) { const double a[3] = {aces(c[0]), aces(c[1]), aces(c[2])}; return lineaireVersLab(a); }

// ancienne vision de nuit (WFS-230) : poids selon la luminance du pixel seule
static void visionNuitAvant(const double c[3], double k, double o[3]) { visionNuit(c, k, 0.0, o); }

int main() {
    std::printf("== WFS-214 RG-01 : tramage avant la conversion 8 bits (ANO-109) ==\n");
    // ciel clair : codes sRGB 0,55 à 0,85 (linéaire 0,26 à 0,69), où ANO-109 montrait les anneaux ; puis toute la plage
    const double avantCiel = residuMax(false, 0.26, 0.69), apresCiel = residuMax(true, 0.26, 0.69);
    const double apresTout = residuMax(true, 0.002, 0.98);
    VERIFIE(avantCiel > 0.2, "avant : marches de %.2f code dans le ciel clair (le défaut d'ANO-109 est reproduit)", avantCiel);
    VERIFIE(apresCiel < 0.02, "après : marches de %.3f code dans le ciel clair (≤ 0,02)", apresCiel);
    VERIFIE(apresTout < 0.02, "après : marches de %.3f code sur toute la plage 0,002..0,98 (≤ 0,02)", apresTout);
    const std::string post = lire(WFS_SOURCE_DIR "/shaders/post.frag");
    VERIFIE(post.find("c += tramageTriangulaire(gl_FragCoord.xy) / 255.0;") != std::string::npos &&
            post.find("srgbCode(c)") != std::string::npos && post.find("c = srgbLineaire(clamp(c, 0.0, 1.0));") != std::string::npos,
            "post.frag : tramage triangulaire ajouté au code sRGB, puis ramené en linéaire pour l'écran sRGB");
    std::printf("WFS-214 : marches du dégradé de ciel %.2f code avant, %.3f après\n", avantCiel, apresCiel);

    std::printf("== WFS-214 RG-05 : vision de nuit loin des lumières seulement ==\n");
    VERIFIE(post.find("const float SCOTO_LOG_BAS = -2.3, SCOTO_LOG_PLAGE = 3.0, SCOTO_MAX = 0.9;") != std::string::npos &&
            post.find("log(max(Y / k, La))") != std::string::npos, "post.frag : mêmes constantes et même adaptation que eclairage_exposition.h");
    // Golf du joueur : peinture 0x1d4fd8 (src/game/cars.h), part diffuse (albédo = couleur sRGB ramenée en linéaire)
    const double golf[3] = {lineaire(0x1d / 255.0), lineaire(0x4f / 255.0), lineaire(0xd8 / 255.0)};
    // sous un lampadaire : 15 lx (E_LAMPE_SOL), exposition de la rue éclairée : pire cas EV 3 (borne basse de la nuit éclairée)
    double pire = 0, pireAvant = 0;
    for (double ev : {EV_NUIT_MIN, 4.0, EV_NUIT_MAX}) {
        const double k = exposition(ev), La = luminanceAdaptationNuit(ev);
        double c[3], av[3], ap[3];
        for (int i = 0; i < 3; i++) c[i] = golf[i] * E_LAMPE_SOL / PI_L * k;
        visionNuitAvant(c, k, av); visionNuit(c, k, La, ap);
        const Lab ref = affichee(c);
        const double dAp = deltaE2000(ref, affichee(ap)), dAv = deltaE2000(ref, affichee(av));
        pire = std::max(pire, dAp); pireAvant = std::max(pireAvant, dAv);
        std::printf("  Golf sous lampadaire (15 lx), EV %.1f : ΔE2000 avec / sans vision de nuit : avant %.1f, après %.2f\n", ev, dAv, dAp);
    }
    VERIFIE(pire <= 10, "Golf sous lampadaire : ΔE2000 ≤ 10 avec la même carrosserie sans vision de nuit (pire %.2f ; avant %.1f)", pire, pireAvant);
    // rase campagne : lune 0,094 lx, posemètre incident à la borne −3,5 (WFS-249) : la vision de nuit reste presque entière
    // (adaptation 0,011 cd/m², au lieu de la luminance du pixel seule : poids 0,87 au lieu de 0,90 sur la Golf, écart invisible)
    {
        const double ev = evIncidentNuit(0.094), k = exposition(ev), La = luminanceAdaptationNuit(ev);
        double pireC = 0, poidsMin = 1;
        const double surfaces[3][3] = {{golf[0], golf[1], golf[2]}, {0.05, 0.10, 0.03}, {0.08, 0.08, 0.08}};   // Golf, herbe, asphalte
        for (const auto& alb : surfaces) {
            double c[3], av[3], ap[3];
            for (int i = 0; i < 3; i++) c[i] = alb[i] * 0.094 / PI_L * k;
            // relèvement des basses lumières (WFS-276) avant l'affichage, comme dans post.frag, pour comparer ce qui se voit
            visionNuitAvant(c, k, av); visionNuit(c, k, La, ap);
            auto vu = [&](const double x[3]) { const double Y = 0.2126 * x[0] + 0.7152 * x[1] + 0.0722 * x[2], f = Y > 0 ? basseLumiereNuit(Y) / Y : 1;
                                               const double y[3] = {x[0] * f, x[1] * f, x[2] * f}; return affichee(y); };
            pireC = std::max(pireC, deltaE2000(vu(av), vu(ap)));
            poidsMin = std::min(poidsMin, poidsScotopique((0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2]) / k, La));
        }
        VERIFIE(ev <= EV_LUNE_MIN + 1e-9 && poidsMin > 0.85 && pireC <= 2.0,   // ≤ 2 : sous le seuil de perception côte à côte (≈ 2,3 ; Mahy, Van Eycken et Oosterlinck, Color Res. Appl. 19(2), 1994)

                "rase campagne (EV %.1f) : vision de nuit gardée (poids ≥ %.2f ; Golf, herbe, asphalte à ΔE2000 ≤ %.2f de WFS-230)", ev, poidsMin, pireC);
    }
    // de jour : rien (k = 0)
    { const double c[3] = {0.3, 0.2, 0.1}; double o[3]; visionNuit(c, 0.0, 100.0, o); VERIFIE(o[0] == c[0] && o[1] == c[1] && o[2] == c[2], "jour : image inchangée"); }
    // poids continu et décroissant avec l'adaptation (aucun saut quand le joueur s'approche d'une lampe)
    { bool mono = true; double prec = 1; for (double L = 1e-4; L < 100; L *= 1.05) { const double s = poidsScotopique(0.0, L); mono = mono && s <= prec + 1e-12; prec = s; }
      VERIFIE(mono, "poids scotopique décroissant avec la luminance d'adaptation (continu)"); }
    std::printf("WFS-214 : Golf sous lampadaire ΔE2000 %.1f avant, %.2f après (seuil 10)\n", pireAvant, pire);

    std::printf(g_echecs ? "ÉCHEC : %d vérification(s)\n" : "tout est bon\n", g_echecs);
    return g_echecs ? 1 : 0;
}
