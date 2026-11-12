// WFS-383 : sols des cartes (src/world/sols.*, shaders/sols.glsl) — code du sol lu par le shader, lissage des normales du terrain
// (plus de facettes : un pli en V donne une normale moyenne au faîte, les sommets soudés sont fusionnés, deux bandes de couleur
// différente gardent leurs sommets mais la même normale), climat des cartes (cartes du code ; carte de données par sa ligne « sol » ;
// carte sans ligne : bandes unies comme avant, carte de Noël de WFS-346 comprise) et refus d'une ligne « sol » fausse.
#include <cmath>
#include <cstdio>
#include <string>
#include "../src/core/carte_cuite.h"
#include "../src/world/cartes_v2/carte_donnees.h"
#include "../src/world/sols.h"
using namespace wfs;

static int g_echecs = 0;
#define VERIFIER(c) do { if (!(c)) { std::printf("ÉCHEC ligne %d : %s\n", __LINE__, #c); g_echecs++; } } while (0)

// quad (deux triangles séparés, 6 sommets) de a, b, c, d, couleur u
static void quad(std::vector<Vertex>& v, std::vector<uint32_t>& i, vec3 a, vec3 b, vec3 c, vec3 d, float u) {
    for (vec3 p : {a, b, c, a, c, d}) { i.push_back((uint32_t)v.size()); v.push_back({p.x, p.y, p.z, 0, 0, 0, u, 0}); }
}

int main() {
    // ---- code du sol (pc.params.z de sols.glsl) : climat + 4 × teinte + 8 × rocheux ; matériau MAT_GROUND
    VERIFIER(sols::code(sols::Climat::Mediterraneen, false, false) == 1.f);
    VERIFIER(sols::code(sols::Climat::Tempere, true, false) == 6.f);
    VERIFIER(sols::code(sols::Climat::Automne, false, true) == 11.f);
    const Material m = sols::materiau(sols::Climat::Mediterraneen, false, true);
    VERIFIER(m.flags == MAT_GROUND && m.cutoff == 9.f && m.rough > 0.9f);

    // ---- pli en V (talus de 45° de chaque côté) : 2 quads, 12 sommets → 6 ; normale du faîte verticale, des bords à 45°
    {
        std::vector<Vertex> v; std::vector<uint32_t> i;
        quad(v, i, {-1, 0, 0}, {0, 1, 0}, {0, 1, 1}, {-1, 0, 1}, 0);
        quad(v, i, {0, 1, 0}, {1, 0, 0}, {1, 0, 1}, {0, 1, 1}, 0);
        sols::lisserNormales(v, i);
        VERIFIER(v.size() == 6 && i.size() == 12);
        for (const Vertex& x : v) {
            VERIFIER(std::fabs(x.nx * x.nx + x.ny * x.ny + x.nz * x.nz - 1) < 1e-4f && x.ny > 0);
            if (std::fabs(x.px) < 1e-4f) VERIFIER(std::fabs(x.ny - 1) < 1e-4f);                 // faîte : verticale
            else VERIFIER(std::fabs(x.ny - std::sqrt(0.5f)) < 1e-3f && std::fabs(x.nz) < 1e-4f);   // bords : 45°
        }
        for (uint32_t k : i) VERIFIER(k < v.size());
    }
    // ---- deux bandes de couleur différente : sommets gardés (8), même normale au point commun ; triangle à l'envers : normale vers le haut
    {
        std::vector<Vertex> v; std::vector<uint32_t> i;
        quad(v, i, {-1, 0, 0}, {0, 1, 0}, {0, 1, 1}, {-1, 0, 1}, 0.1f);
        quad(v, i, {0, 1, 1}, {1, 0, 1}, {1, 0, 0}, {0, 1, 0}, 0.2f);   // enroulé dans l'autre sens
        sols::lisserNormales(v, i);
        VERIFIER(v.size() == 8);
        int faite = 0;
        for (const Vertex& x : v) if (std::fabs(x.px) < 1e-4f) { faite++; VERIFIER(std::fabs(x.ny - 1) < 1e-4f); }
        VERIFIER(faite == 4);
        for (const Vertex& x : v) VERIFIER(x.ny > 0.7f);
    }
    // ---- rien à lisser : pas de plantage
    { std::vector<Vertex> v; std::vector<uint32_t> i; sols::lisserNormales(v, i); VERIFIER(v.empty()); }

    // ---- climat des cartes du code
    sols::Climat c = sols::Climat::Automne;
    VERIFIER(sols::climatCarte("plage", c) && c == sols::Climat::Mediterraneen);
    VERIFIER(sols::climatCarte("campagne", c) && c == sols::Climat::Tempere);
    VERIFIER(sols::climatCarte("ville", c) && sols::climatCarte("circuit", c) && c == sols::Climat::Tempere);
    VERIFIER(!sols::climatCarte(nullptr, c) && !sols::climatCarte("inconnue", c));

    // ---- ligne « sol » du bloc decor (cartes de données) : lue, refusée si fausse ; absente : 0 (bandes unies, comme avant)
    std::string e;
    cartes_v2::DonneesCarte d; d.id = "x";
    VERIFIER(cartes_v2::lireTrace("format 1\nnom Essai\nroute 50 200 50 0 0\n", d, e));
    VERIFIER(cartes_v2::lireDecor("format 2\nsol automne\nbande 7.5 9 0 0 0x7a6a3a\n", d, e) && d.climatSol == 3);
    VERIFIER(cartes_v2::lireDecor("format 2\nsol mediterraneen\n", d, e) && d.climatSol == 1);
    VERIFIER(cartes_v2::lireDecor("format 2\nbande 7.5 9 0 0 0x7a6a3a\n", d, e) && d.climatSol == 0);   // relu : remis à 0
    VERIFIER(!cartes_v2::lireDecor("format 2\nsol desert\n", d, e) && e.find("sol") != std::string::npos);
    VERIFIER(!cartes_v2::lireDecor("format 2\nsol\n", d, e));
    // les deux cartes d'essai : l'automne a son sol, la carte de Noël (WFS-346) n'en a pas (inchangée)
    carte::Paquet pn, pa;
    const std::string src = std::string(WFS_SOURCE_DIR) + "/assets";
    VERIFIER(cartes_v2::sourcesCarte(src, "noel-test", carte::FormatTexture::RGBA8, pn, e) && cartes_v2::sourcesCarte(src, "automne-test", carte::FormatTexture::RGBA8, pa, e));
    if (pn.bloc("decor") && pa.bloc("decor") && pn.bloc("trace") && pa.bloc("trace")) {
        cartes_v2::DonneesCarte n, a; n.id = "noel-test"; a.id = "automne-test";
        VERIFIER(cartes_v2::lireTrace(std::string(pn.bloc("trace")->donnees.begin(), pn.bloc("trace")->donnees.end()), n, e));
        VERIFIER(cartes_v2::lireTrace(std::string(pa.bloc("trace")->donnees.begin(), pa.bloc("trace")->donnees.end()), a, e));
        VERIFIER(cartes_v2::lireDecor(std::string(pn.bloc("decor")->donnees.begin(), pn.bloc("decor")->donnees.end()), n, e) && n.climatSol == 0);
        VERIFIER(cartes_v2::lireDecor(std::string(pa.bloc("decor")->donnees.begin(), pa.bloc("decor")->donnees.end()), a, e) && a.climatSol == 3);
    } else VERIFIER(false);

    if (g_echecs) { std::printf("test_sols : %d échec(s)\n", g_echecs); return 1; }
    std::printf("test_sols : OK\n");
    return 0;
}
