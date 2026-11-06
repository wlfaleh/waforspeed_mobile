// WFS-150 : socle des passes, calculs seuls. usage : test_passes
//   - RG-03 : faces de la cubemap rendues dans la convention des cubemaps Vulkan (un texel (s, t) de la face f, projeté par
//     vueProjFace, retombe sur la direction que la spécification lui donne) ; taille 256 / 128 / 64 selon le niveau ;
//   - RG-04 : Ultra proposé seulement après 5 mesures de 2 s de suite à 57 images/s ou plus, en Haute, sans chauffe ;
//     niveau affiché, ligne « Rendu » ; coûts de la partie inchangés (Ultra = Haute pour reglages().qualite).
#include <cmath>
#include <cstdio>
#include <string>
#include "../src/render/cube_faces.h"
#include "../src/render/qualite_auto.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// direction de la spécification pour le texel (s, t) de la face f (s, t ∈ [0, 1])
static vec3 directionSpec(int f, float s, float t) {
    const float sc = 2 * s - 1, tc = 2 * t - 1;
    switch (f) {
    case 0: return normalize(vec3{1, -tc, -sc});
    case 1: return normalize(vec3{-1, -tc, sc});
    case 2: return normalize(vec3{sc, 1, tc});
    case 3: return normalize(vec3{sc, -1, -tc});
    case 4: return normalize(vec3{sc, -tc, 1});
    default: return normalize(vec3{-sc, -tc, -1});
    }
}

int main() {
    // RG-03 : orientation des 6 faces (point quelconque, 25 texels par face)
    const vec3 p{12.5f, 1.7f, -40.f};
    double pire = 0;
    for (int f = 0; f < 6; f++) {
        const mat4 vp = environnement::vueProjFace(f, p);
        for (float s : {0.1f, 0.3f, 0.5f, 0.7f, 0.9f}) for (float t : {0.1f, 0.3f, 0.5f, 0.7f, 0.9f}) {
            const vec3 d = directionSpec(f, s, t), q = p + d * 10.f;
            const float x = vp.at(0, 0) * q.x + vp.at(0, 1) * q.y + vp.at(0, 2) * q.z + vp.at(0, 3);
            const float y = vp.at(1, 0) * q.x + vp.at(1, 1) * q.y + vp.at(1, 2) * q.z + vp.at(1, 3);
            const float w = vp.at(3, 0) * q.x + vp.at(3, 1) * q.y + vp.at(3, 2) * q.z + vp.at(3, 3);
            // NDC Vulkan : x à droite, y vers le bas (rangée 0 en haut) ; texel = (ndc + 1) / 2
            const double es = std::fabs((x / w + 1) / 2 - s), et = std::fabs((y / w + 1) / 2 - t);
            pire = std::max(pire, std::max(es, et));
            VERIFIE(w > 0 && es < 1e-4 && et < 1e-4, "face %d, texel (%.1f, %.1f) : rendu en (%.4f, %.4f)", f, s, t, (x / w + 1) / 2, (y / w + 1) / 2);
        }
    }
    std::printf("cubemap : 6 faces × 25 texels dans la convention Vulkan (écart max %.2e) : %s\n", pire, pire < 1e-4 ? "OK" : "ÉCHEC");
    VERIFIE(qualite::tailleCubemap(qualite::ULTRA) == 256 && qualite::tailleCubemap(qualite::HAUTE) == 128 &&
            qualite::tailleCubemap(qualite::MOYENNE) == 128 && qualite::tailleCubemap(qualite::BASSE) == 64, "tailles de la cubemap (RG-03)");

    // RG-04 : tenue de la Haute
    qualite::TenueUltra t;
    for (int i = 0; i < 4; i++) t.mesure(60, qualite::HAUTE, 0);
    VERIFIE(!t.tenue, "4 mesures : pas encore tenue");
    t.mesure(50, qualite::HAUTE, 0);
    for (int i = 0; i < 4; i++) t.mesure(60, qualite::HAUTE, 0);
    VERIFIE(!t.tenue && t.suite == 4, "une mesure à 50 images/s remet la série à zéro (suite %d)", t.suite);
    t.mesure(58, qualite::HAUTE, 1);
    VERIFIE(!t.tenue && t.suite == 0, "chauffe : série remise à zéro");
    for (int i = 0; i < 5; i++) t.mesure(60, qualite::MOYENNE, 0);
    VERIFIE(!t.tenue, "en Moyenne : rien ne prouve la Haute");
    for (int i = 0; i < 5; i++) t.mesure(57, qualite::HAUTE, 0);
    VERIFIE(t.tenue, "5 mesures à 57 images/s en Haute sans chauffe : tenue");
    std::printf("Ultra proposé après 5 mesures de 2 s à ≥ 57 images/s en Haute, chauffe 0 (série coupée par une mesure lente ou la chauffe) : %s\n",
                t.tenue ? "OK" : "ÉCHEC");

    qualite::ultraChoisi = false; qualite::tenueUltra = {};
    VERIFIE(!qualite::ultraPropose() && qualite::niveauAffiche(qualite::HAUTE) == qualite::HAUTE, "sans tenue : Ultra non proposé");
    qualite::ultraChoisi = true;
    VERIFIE(qualite::ultraPropose() && qualite::niveauAffiche(qualite::HAUTE) == qualite::ULTRA, "Ultra choisi : proposé et affiché");
    VERIFIE(qualite::niveauAffiche(qualite::MOYENNE) == qualite::MOYENNE, "rétrogradation (Moyenne) : Ultra ne s'affiche pas");
    const std::string l = qualite::texteRendu(qualite::niveauAffiche(qualite::HAUTE), 1.f, 4, 60, 0);
    VERIFIE(l.rfind("Rendu : Ultra · 100 %", 0) == 0, "ligne Rendu : %s", l.c_str());
    // Ultra = coûts de la Haute : échelle, plancher, bloom, allègement de la valeur 2
    VERIFIE(qualite::echelleScene(qualite::HAUTE, 1.f) == 1.f && qualite::bloomNiveau(qualite::HAUTE) == 1.f, "Ultra : coûts de la Haute");
    qualite::ultraChoisi = false;
    std::printf("%s\n", echecs ? "test_passes : ÉCHEC" : "test_passes : OK");
    return echecs ? 1 : 0;
}
