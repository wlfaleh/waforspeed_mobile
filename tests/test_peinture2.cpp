// WaForSpeed — WFS-488 : couleur secondaire (toit et coques de rétroviseurs), src/render/vehicule_peinture2.h.
// 1) code de la couleur 2 (pc.color.w) : les 28 couleurs du catalogue et le noir pur relus à l'identique, « une seule couleur » ≥ 0 ;
//    finition (pc.params.z) relue comme le shader (vehicule.glsl) ;
// 2) maillage d'essai : 2 triangles qui partagent une arête, un seul en couleur 2 : sommets du bord dédoublés (limite nette,
//    RG-02), normales de longueur 4 sur la zone seulement, aucun triangle ni pièce en plus (RG-05) ; niveau allégé : triangle de
//    la zone en couleur 2, triangle mêlé en couleur 1 ; .pt2 tronqué ou faux refusé ;
// 3) les 17 voitures permises (assets/cars/<id>.pt2, tools/peinture2_voitures.mjs) : fichier lisible, empreintes égales au .wfm,
//    toit repéré (sauf Spyder, Model 3, S3 et Range Rover Sport, toits en verre : RG-03), deux rétroviseurs, triangles inchangés, chaque triangle repéré marqué.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "../src/game/cars.h"
#include "../src/game/sauvegarde.h"
#include "../src/render/vehicule_peinture2.h"

namespace wfs {   // le test lit les fichiers du dépôt (les fonctions de la plateforme ne servent qu'à appliquer(), non testée ici)
bool readAsset(const std::string& chemin, std::vector<uint8_t>& out) {
    std::ifstream f(std::string(WFS_SOURCE_DIR) + "/assets/" + chemin, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), {});
    return true;
}
double nowSeconds() { return 0; }
}

using namespace wfs;
namespace p2 = wfs::vehicule_peinture2;

static int g_echecs = 0;
#define VERIFIE(cond, ...) do { if (!(cond)) { g_echecs++; std::printf("ÉCHEC : "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static float longueur(const Vertex& v) { return std::sqrt(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz); }
static uint32_t u32(const uint8_t* p) { uint32_t x; std::memcpy(&x, p, 4); return x; }

// table des maillages d'un .wfm (même lecture que vehicule_peinture2::appliquer) et sommets en Vertex
static bool lireWfm(const std::vector<uint8_t>& f, std::vector<p2::MaillageWfm>& ms, std::vector<Vertex>& verts, std::vector<uint32_t>& idx,
                    std::vector<ModelPart>& parts) {
    size_t o = 8;
    if (f.size() < 44) return false;
    const uint32_t nT = u32(&f[o]), nM = u32(&f[o + 4]), nMe = u32(&f[o + 8]);
    o += 36;
    for (uint32_t t = 0; t < nT; t++) o += 4 + (size_t)u32(&f[o]);
    std::vector<uint32_t> drapeaux(nM);
    for (uint32_t k = 0; k < nM; k++) drapeaux[k] = u32(&f[o + (size_t)k * 56 + 52]);
    o += (size_t)nM * 56;
    for (uint32_t k = 0; k < nMe; k++) {
        p2::MaillageWfm w; w.materiau = u32(&f[o]); w.nSommets = u32(&f[o + 4]); w.nIndices = u32(&f[o + 8]); o += 12;
        if (o + (size_t)w.nSommets * 32 + (size_t)w.nIndices * 4 > f.size()) return false;
        w.peint = w.materiau < nM && (drapeaux[w.materiau] & MAT_PAINT);
        w.premierSommet = (uint32_t)verts.size(); w.sommets = &f[o]; w.indices = &f[o + (size_t)w.nSommets * 32];
        for (uint32_t s = 0; s < w.nSommets; s++) {
            Vertex v{}; std::memcpy(&v, &f[o + (size_t)s * 32], 32); verts.push_back(v);
        }
        o += (size_t)w.nSommets * 32;
        parts.push_back({(uint32_t)idx.size(), w.nIndices, (int32_t)w.premierSommet, w.materiau});
        for (uint32_t i = 0; i < w.nIndices; i++) idx.push_back(u32(&f[o + (size_t)i * 4]));
        o += (size_t)w.nIndices * 4;
        ms.push_back(w);
    }
    return true;
}

int main() {
    // ---------- 1. codes ----------
    for (const PeintureDef& p : PEINTURES) {
        unsigned h = 0;
        const float c = p2::code(p.hex);
        VERIFIE(p2::decoder(c, h) && h == p.hex, "code de %s (%06x) relu %06x", p.id, p.hex, h);
        VERIFIE(c < -0.5f, "code de %s non négatif", p.id);
    }
    {
        unsigned h = 1;
        VERIFIE(p2::decoder(p2::code(0x000000), h) && h == 0, "noir pur mal relu (%06x)", h);
        VERIFIE(p2::decoder(p2::code(0xFFFFFF), h) && h == 0xFFFFFF, "blanc pur mal relu (%06x)", h);
        VERIFIE(!p2::decoder(p2::UNE_COULEUR, h) && !p2::decoder(0.f, h) && !p2::decoder(0.45f, h), "alpha d'un matériau pris pour une couleur 2");
        // finition : 4 + métal × 100 (entier) + rugosité ; le shader fait metal = floor(v) / 100, rough = fract(v)
        for (float m : {0.f, 0.3f, 1.f}) for (float r : {0.f, 0.2f, 0.45f, 0.9f}) {
            const float v = p2::codeFinition(m, r) - 4.f;
            VERIFIE(std::fabs(std::floor(v) / 100.f - m) < 0.006f && std::fabs((v - std::floor(v)) - r) < 1e-3f, "finition (%.2f, %.2f) mal relue", m, r);
        }
    }
    // ---------- 2. maillage d'essai ----------
    {
        // carré ABCD coupé en ABC (toit) et ACD (carrosserie) : A et C au bord ; B au toit seul ; D à la carrosserie seule
        std::vector<Vertex> v = {{0, 1, 0, 0, 1, 0, 0, 0}, {1, 1, 0, 0, 1, 0, 1, 0}, {1, 1, 1, 0, 1, 0, 1, 1}, {0, 1, 1, 0, 1, 0, 0, 1}};
        std::vector<uint8_t> brut(4 * 32);
        for (int s = 0; s < 4; s++) std::memcpy(&brut[(size_t)s * 32], &v[s], 32);
        std::vector<uint32_t> idx = {0, 1, 2, 0, 2, 3};
        std::vector<uint8_t> brutI(idx.size() * 4); std::memcpy(brutI.data(), idx.data(), brutI.size());
        // niveau allégé (à la suite dans le tampon) : le triangle du toit, et un triangle mêlé B C D
        idx.insert(idx.end(), {0, 1, 2, 1, 2, 3});
        std::vector<p2::MaillageWfm> ms(1);
        ms[0] = {0, 4, 6, 0, true, brut.data(), brutI.data()};
        p2::Bloc b; b.maillage = 0; b.triangles = 2; b.empreinte = p2::empreinte(brut.data(), 4, brutI.data(), 6); b.toit = 1; b.bits = {1};
        std::vector<ModelPart> complet = {{0, 6, 0, 0}}, allege = {{6, 6, 0, 0}};
        const size_t nComplet = complet.size(), nAllege = allege.size();
        const p2::Bilan bl = p2::marquer({b}, ms, v, idx, {&complet, &allege});
        VERIFIE(bl.lu && bl.triangles == 1 && bl.toit == 1, "essai : %u triangle(s) du toit", bl.triangles);
        // A et C (bord du toit) et B (pris aussi par le triangle mêlé du niveau allégé) : copies marquées pour le toit
        VERIFIE(bl.dedoubles == 3 && v.size() == 7, "essai : %u sommets dédoublés (attendu 3 : A, B, C)", bl.dedoubles);
        VERIFIE(complet.size() == nComplet && allege.size() == nAllege && idx.size() == 12, "essai : pièces ou triangles ajoutés (RG-05)");
        const uint32_t orig[3] = {0, 1, 2};
        for (int j = 0; j < 3; j++) {
            VERIFIE(std::fabs(longueur(v[idx[j]]) - p2::MARQUE) < 1e-4f, "essai : sommet %d du toit non marqué", j);
            VERIFIE(std::memcmp(&v[idx[j]].px, &v[orig[j]].px, 12) == 0, "essai : position du sommet %d changée", j);
            VERIFIE(std::fabs(longueur(v[idx[6 + j]]) - p2::MARQUE) < 1e-4f, "essai : allégé : sommet %d du triangle du toit non marqué", j);
        }
        for (int j = 3; j < 6; j++) VERIFIE(std::fabs(longueur(v[idx[j]]) - 1) < 1e-4f, "essai : sommet %d de la carrosserie marqué", j);
        for (int j = 9; j < 12; j++) VERIFIE(std::fabs(longueur(v[idx[j]]) - 1) < 1e-4f, "essai : allégé : triangle mêlé marqué (dégradé)");
        // fichier .pt2 : tronqué, mauvaise signature
        std::vector<uint8_t> f = {'W', 'P', 'T', '2', 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1};
        std::vector<p2::Bloc> lu;
        VERIFIE(p2::lire(f.data(), f.size(), lu) && lu.size() == 1 && lu[0].bits.size() == 1, ".pt2 d'essai refusé");
        VERIFIE(!p2::lire(f.data(), f.size() - 1, lu), ".pt2 tronqué accepté");
        f[0] = 'X';
        VERIFIE(!p2::lire(f.data(), f.size(), lu), ".pt2 de signature fausse accepté");
        // empreinte différente : bloc ignoré
        b.empreinte ^= 1;
        std::vector<Vertex> v2(v.begin(), v.begin() + 4);
        std::vector<uint32_t> i2 = {0, 1, 2, 0, 2, 3};
        std::vector<ModelPart> c2 = {{0, 6, 0, 0}};
        VERIFIE(!p2::marquer({b}, ms, v2, i2, {&c2}).lu, "empreinte fausse acceptée");
    }
    // ---------- 3. les 17 voitures ----------
    int voitures = 0;
    for (const CarDef& c : CARS) {
        if (voitureRetiree(c.id)) continue;
        voitures++;
        std::vector<uint8_t> wfm, pt2;
        const std::string chemin = c.file;   // cars/<id>.wfm
        if (!readAsset(chemin, wfm)) { VERIFIE(false, "%s : %s illisible", c.id, chemin.c_str()); continue; }
        if (!readAsset(chemin.substr(0, chemin.size() - 4) + ".pt2", pt2)) { VERIFIE(false, "%s : .pt2 absent (lancer tools/peinture2_voitures.mjs)", c.id); continue; }
        std::vector<p2::Bloc> blocs;
        VERIFIE(p2::lire(pt2.data(), pt2.size(), blocs) && !blocs.empty(), "%s : .pt2 illisible", c.id);
        std::vector<p2::MaillageWfm> ms; std::vector<Vertex> verts; std::vector<uint32_t> idx; std::vector<ModelPart> parts;
        VERIFIE(lireWfm(wfm, ms, verts, idx, parts), "%s : .wfm illisible", c.id);
        uint32_t toit = 0, retro = 0, attendu = 0;
        for (const p2::Bloc& b : blocs) {
            VERIFIE(b.maillage < ms.size() && ms[b.maillage].nIndices / 3 == b.triangles &&
                    p2::empreinte(ms[b.maillage].sommets, ms[b.maillage].nSommets, ms[b.maillage].indices, ms[b.maillage].nIndices) == b.empreinte,
                    "%s : maillage %u : empreinte différente du .wfm (relancer tools/peinture2_voitures.mjs)", c.id, b.maillage);
            toit += b.toit; retro += b.retro;
            for (uint32_t t = 0; t < b.triangles; t++) attendu += (b.bits[t >> 3] >> (t & 7)) & 1;
        }
        const bool sansToit = !std::strcmp(c.id, "gallardo") || !std::strcmp(c.id, "model3") || !std::strcmp(c.id, "s3") || !std::strcmp(c.id, "rrsport");   // Spyder, toits en verre (RG-03)
        VERIFIE(sansToit ? toit == 0 : toit >= 50, "%s : toit %u triangles", c.id, toit);
        VERIFIE(retro >= 40, "%s : rétroviseurs %u triangles", c.id, retro);
        const size_t nIdx = idx.size();
        const p2::Bilan bl = p2::marquer(blocs, ms, verts, idx, {&parts});
        VERIFIE(bl.triangles == attendu && attendu == toit + retro, "%s : %u triangles marqués sur %u", c.id, bl.triangles, attendu);
        VERIFIE(idx.size() == nIdx, "%s : triangles ajoutés (RG-05)", c.id);
        // chaque triangle repéré : ses 3 sommets marqués ; les autres triangles des maillages peints : aucun sommet marqué
        uint32_t faux = 0;
        for (const ModelPart& p : parts)
            for (uint32_t t = 0; t + 2 < p.indexCount; t += 3) {
                int marques = 0;
                for (int j = 0; j < 3; j++) marques += longueur(verts[(size_t)p.vertexOffset + idx[p.firstIndex + t + j]]) > 2.f;
                faux += marques != 0 && marques != 3;
            }
        VERIFIE(faux == 0, "%s : %u triangles à moitié marqués (dégradé au bord)", c.id, faux);
        std::printf("%-9s toit %5u, rétroviseurs %5u triangles, %u sommets dédoublés, %zu pièce(s) d'origine non peinte(s)\n", c.id, toit, retro,
                    bl.dedoubles, bl.piecesNonPeintes.size());
    }
    VERIFIE(voitures == 17, "%d voitures permises (17 attendues)", voitures);
    if (g_echecs) { std::printf("test_peinture2 : %d échec(s)\n", g_echecs); return 1; }
    std::printf("test_peinture2 : OK (codes, maillage d'essai, %d voitures)\n", voitures);
    return 0;
}
