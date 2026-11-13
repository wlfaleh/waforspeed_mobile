// WFS-490 : finitions de la peinture (src/render/vehicule_finitions.h, shaders/vehicule.glsl).
// Vérifie : (1) le codage de la finition dans le métal (pc.params.x) : aller-retour exact pour les 8 finitions et tout métal de
// [0, 1], brillant = métal d'origine au bit près (RG-01), donnée inconnue → brillant ; (2) les valeurs du shader contre la charte
// (regles-de-gestion/DA-realisme.md § 3) et references/reel/voitures/peintures.md : vernis brillant 0,03 inchangé, chrome
// réflectance 0,55-0,65 et rugosité 0,05-0,15 (RG-06), mat plus rugueux que satiné, satiné plus rugueux que le brillant (RG-04,
// RG-05) ; (3) le shader garde SPEC_MAX sur les paillettes et le vernis (RG-09) et un seul shader (RG-10 : aucun nouvel échantillonneur).
// usage : test_fidelite_voitures_finitions [dossier du dépôt]
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include "../src/render/vehicule_finitions.h"
using namespace wfs;

#ifndef WFS_SOURCE_DIR
#define WFS_SOURCE_DIR "."
#endif

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static std::string lire(const std::string& chemin) { std::ifstream f(chemin); std::stringstream s; s << f.rdbuf(); return s.str(); }
// valeur de « const float NOM = x; » dans le texte du shader ; NAN si absente
static double constante(const std::string& src, const char* nom) {
    const std::string cle = std::string(nom) + " = ";
    const size_t p = src.find(cle);
    return p == std::string::npos ? NAN : std::atof(src.c_str() + p + cle.size());
}

int main(int argc, char** argv) {
    namespace vf = vehicule_finitions;
    const std::string racine = argc > 1 ? argv[1] : WFS_SOURCE_DIR;
    // (1) codage
    for (int f = 0; f < vf::NOMBRE; f++)
        for (int i = 0; i <= 1000; i++) {
            const float m = i / 1000.f;
            float x = vf::coder(m, f);
            const int g = vf::decoder(x);
            VERIFIE(g == f, "finition %d, métal %.3f : relue %d", f, m, g);
            VERIFIE(std::fabs(x - m) < 1e-5f, "finition %d, métal %.3f : métal relu %.6f", f, m, x);
        }
    for (float m : {0.f, 0.3f, 0.77f, 1.f}) { const float x = vf::coder(m, vf::BRILLANT); VERIFIE(std::memcmp(&x, &m, sizeof m) == 0, "brillant : métal %.2f modifié", m); }
    VERIFIE(vf::coder(0.5f, 9) == 0.5f && vf::coder(0.5f, -3) == 0.5f, "finition hors bornes : métal modifié");
    for (int f = 0; f < vf::NOMBRE; f++) VERIFIE(vf::depuisId(vf::IDS[f]) == f, "identifiant %s", vf::IDS[f]);
    VERIFIE(vf::depuisId("doré") == -1 && vf::depuisId(nullptr) == -1, "identifiant inconnu accepté");
    // (2) valeurs du shader
    const std::string glsl = lire(racine + "/shaders/vehicule.glsl"), frag = lire(racine + "/shaders/scene.frag");
    VERIFIE(!glsl.empty() && !frag.empty(), "shaders illisibles depuis %s", racine.c_str());
    const double vb = constante(glsl, "FIN_VERNIS_BRILLANT"), vs = constante(glsl, "FIN_VERNIS_SATINE"), rm = constante(glsl, "FIN_MAT_RUGOSITE"),
                 rs = constante(glsl, "FIN_SATINE_RUGOSITE"), cf = constante(glsl, "FIN_CHROME_F0"), cr = constante(glsl, "FIN_CHROME_RUGOSITE");
    std::printf("vernis brillant %.3f, satiné %.3f ; base mat %.2f, satinée %.2f ; chrome F0 %.2f, rugosité %.2f\n", vb, vs, rm, rs, cf, cr);
    VERIFIE(vb == 0.03, "vernis brillant %.3f : doit rester 0,03 (aspect d'avant, RG-01)", vb);
    VERIFIE(vb >= 0.02 && vb <= 0.08, "vernis brillant hors de la charte (0,02-0,08)");
    VERIFIE(cf >= 0.55 && cf <= 0.65, "chrome : réflectance %.2f hors 0,55-0,65 (RG-06)", cf);
    VERIFIE(cr >= 0.05 && cr <= 0.15, "chrome : rugosité %.2f hors 0,05-0,15 (charte § 3)", cr);
    const double ct = constante(glsl, "FIN_CHROME_TEINTE");
    VERIFIE(ct >= 0.0 && ct <= 0.15, "chrome : teinte %.2f au-delà de 15 %% (miroir lisible, RG-06)", ct);
    VERIFIE(glsl.find("FIN_CHROME ? vec3(0.0) : albedo") != std::string::npos, "chrome : lumière de nuit diffuse revenue (métal pur, RG-06)");
    VERIFIE(frag.find("FIN_CHROME) env = vec3(0.0)") == std::string::npos && frag.find("FIN_CHROME) env = mix(env, chausseeReflechie(sun)") != std::string::npos
            && glsl.find("const float RHO = 0.14;") != std::string::npos,
            "chrome : sonde de reflet gardée au-dessus de l'horizon, chaussée réelle (asphalte 0,14) en dessous (RG-06)");
    VERIFIE(rm > rs && rs > 0.3 && vs > vb && rm >= 0.5, "ordre des rugosités mat > satiné > brillant faux (RG-04, RG-05)");
    VERIFIE(glsl.find("vernisCouche(col, R, sun, NoV, NoH, NoL, aaKernel, specK, 0.03, 1.0, couleurPeinture().rgb)") != std::string::npos,
            "vernisPeinture ne garde plus le vernis d'avant (RG-01)");
    VERIFIE(glsl.find("vec3(SPEC_MAX * 0.5)") != std::string::npos && glsl.find("min(vernis, vec3(SPEC_MAX))") != std::string::npos, "SPEC_MAX perdu (RG-09)");
    // (3) un seul shader : la finition est lue dans le métal, aucun échantillonneur ajouté à scene.frag
    VERIFIE(frag.find("lireFinition(metal)") != std::string::npos && frag.find("vernisFinition(") != std::string::npos, "accroches de scene.frag absentes");
    size_t n = 0; for (size_t p = 0; (p = frag.find("uniform sampler2D", p)) != std::string::npos; p++) n++;
    VERIFIE(n == 3, "scene.frag : %zu échantillonneurs de matériau (3 attendus : aucune texture en plus)", n);
    std::printf("%s : %d échec(s)\n", echecs ? "ROUGE" : "VERT", echecs);
    return echecs ? 1 : 0;
}
