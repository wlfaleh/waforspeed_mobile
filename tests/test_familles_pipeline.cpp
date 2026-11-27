// WaForSpeed — moteur 40 img/s : familles de pipelines de la scène (src/render/familles_pipeline.h).
// Vérifie : (1) chaque objet va dans une famille qui sait dessiner tous ses drapeaux (même image que le shader complet) ;
// (2) les cas réels (route, trottoir, sol, façade, végétation, voiture, eau…) vont dans la famille attendue ;
// (3) les drapeaux F_* des shaders ont les mêmes valeurs que ceux du C++ ; (4) scene.frag masque bien ses drapeaux.
#include <cstdio>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include "../src/render/familles_pipeline.h"

using namespace famille_pipeline;
static int echecs = 0;
#define VERIF(c, ...) do { if (!(c)) { std::printf("ÉCHEC l.%d : ", __LINE__); std::printf(__VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static std::string lire(const std::string& chemin) {
    std::ifstream f(chemin); std::stringstream s; s << f.rdbuf(); return s.str();
}

int main() {
    // (1) toutes les combinaisons de 24 drapeaux seraient 16 M : on prend chaque drapeau seul, chaque paire, et 200 000 tirages
    uint32_t graine = 12345;
    auto tirer = [&]() { graine ^= graine << 13; graine ^= graine >> 17; graine ^= graine << 5; return graine & 0xFFFFFFu; };
    for (int i = 0; i < 200000 + 24 * 24; i++) {
        uint32_t d;
        if (i < 24 * 24) d = (1u << (i / 24)) | (1u << (i % 24)); else d = tirer() & (i % 3 ? 0x7FFFFFu : 0xFFFFFFu);
        for (bool voiture : {false, true}) {
            const int f = choisir(d, voiture);
            VERIF(f >= 0 && f < NOMBRE, "famille hors bornes");
            VERIF((d & ~MASQUES[f]) == 0u, "drapeaux %u voiture=%d : famille %d ne sait pas tout dessiner", d, (int)voiture, f);
            VERIF(!(voiture && f == FAM_DECOR), "voiture dans la famille décor (peinture 2 du toit perdue)");
        }
    }
    // (2) cas réels
    VERIF(choisir(ROUTE, false) == FAM_ROUTE, "route");
    VERIF(choisir(ROUTE | NORMALES | METAL_RUGOSITE, false) == FAM_ROUTE, "route texturée");
    VERIF(choisir(TROTTOIR, false) == FAM_TROTTOIR, "trottoir");
    VERIF(choisir(SOL | OCCLUSION, false) == FAM_SOL, "sol");
    VERIF(choisir(FACADE | OCCLUSION | NORMALES | TANGENTE, false) == FAM_DECOR, "façade");
    VERIF(choisir(INTERIEUR, false) == FAM_DECOR, "vitre d'immeuble");
    VERIF(choisir(VEGETATION | DECOUPE | DOUBLE, false) == FAM_DECOR, "feuillage");
    VERIF(choisir(0, false) == FAM_DECOR, "objet simple");
    VERIF(choisir(0, true) == FAM_VOITURE, "pièce de voiture simple (toit d'origine)");
    VERIF(choisir(PEINTURE, true) == FAM_VOITURE, "carrosserie");
    VERIF(choisir(ROUE | NORMALES, true) == FAM_VOITURE, "roue");
    VERIF(choisir(FEU_ARRIERE | EMISSION_TEXTURE, true) == FAM_VOITURE, "feu");
    VERIF(choisir(PEINTURE, false) == FAM_VOITURE, "voiture garée en maillage");
    VERIF(choisir(EAU, false) == COMPLET, "eau : shader complet");
    VERIF(choisir(VITRE | MELANGE, true) == FAM_VITRE, "vitre de voiture");
    VERIF(choisir(PHARE | MELANGE, true) == FAM_VOITURE, "glace de phare transparente");
    VERIF(choisir(VEGETATION | MELANGE, false) == FAM_DECOR, "décor transparent");
    VERIF(choisir(ROUTE | TROTTOIR, false) == COMPLET, "route + trottoir : aucune famille n'a les deux");
    // le shader complet garde tous les drapeaux
    VERIF(MASQUES[COMPLET] == 0xFFFFFFFFu, "complet");

    // (3) drapeaux F_* des shaders = C++
    const std::string dir = WFS_SOURCE_DIR "/shaders/";
    const std::map<std::string, uint32_t> attendus = {
        {"F_PAINT", PEINTURE}, {"F_BLEND", MELANGE}, {"F_TAIL", FEU_ARRIERE}, {"F_HEAD", PHARE}, {"F_MASK", DECOUPE}, {"F_GLASS", VITRE},
        {"F_WHEEL", ROUE}, {"F_ROAD", ROUTE}, {"F_WALK", TROTTOIR}, {"F_GROUND", SOL}, {"F_SHADOW", OMBRE_TACHE}, {"F_HASNORMAL", NORMALES},
        {"F_HASMR", METAL_RUGOSITE}, {"F_VTINT", VEGETATION}, {"F_POOL", TACHE_PHARES}, {"F_ADD", ADDITIF}, {"F_FANTOME", FANTOME},
        {"F_EMTEX", EMISSION_TEXTURE}, {"F_TANGENTE", TANGENTE}, {"F_INTERIEUR", INTERIEUR}, {"F_FACADE", FACADE}, {"F_AO", OCCLUSION}, {"F_EAU", EAU}};
    std::map<std::string, uint32_t> lus;
    const std::regex re(R"((F_[A-Z]+) = ([0-9]+)u)");
    for (const char* nom : {"scene.frag", "decor.glsl", "eau.glsl"}) {
        const std::string t = lire(dir + nom);
        VERIF(!t.empty(), "shader %s illisible", nom);
        for (std::sregex_iterator it(t.begin(), t.end(), re), fin; it != fin; ++it) lus[(*it)[1]] = (uint32_t)std::stoul((*it)[2]);
    }
    for (const auto& [nom, v] : attendus) {
        VERIF(lus.count(nom), "%s absent des shaders", nom.c_str());
        if (lus.count(nom)) VERIF(lus[nom] == v, "%s = %u dans les shaders, %u en C++", nom.c_str(), lus[nom], v);
    }
    // (4) scene.frag masque ses drapeaux, common.glsl déclare la constante 2
    VERIF(lire(dir + "scene.frag").find("uint(pc.params.w + 0.5) & MASQUE_FAMILLE") != std::string::npos, "scene.frag : drapeaux non masqués");
    VERIF(lire(dir + "common.glsl").find("layout(constant_id = 2) const uint MASQUE_FAMILLE") != std::string::npos, "common.glsl : constante 2 absente");

    if (echecs) { std::printf("test_familles_pipeline : %d échec(s)\n", echecs); return 1; }
    std::printf("test_familles_pipeline : OK (%d familles)\n", (int)NOMBRE);
    return 0;
}
