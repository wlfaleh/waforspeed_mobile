// Niveaux de détail des voitures (WFS-158, RG-04) : choix du niveau par la taille à l'écran (seuils en pixels, fondu tramé
// sans saut ni retour en arrière), cas Basse / fantôme / police en lots, lecture des fichiers .lod WFL1 et WFL2 (indices
// u16 et u32), refus des fichiers faux, répartition des pièces par niveau après une découpe ; fichiers .lod du dépôt :
// 20 voitures, 3 niveaux, de moins en moins de triangles (≥ 25 % au niveau 1, RG-12 de WFS-134).
// usage : test_vehicule_lod
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include "../src/render/vehicule_lod.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

struct Piece { uint32_t firstIndex, indexCount; int32_t vertexOffset; uint32_t material; int32_t roue = -1; int8_t feuArriere = 0; };
struct V3 { float x, y, z; };
struct Modele {
    std::vector<Piece> parts, partsLod, partsBasse; std::vector<std::vector<Piece>> partsNiveaux;
    V3 bmin{-0.9f, 0, -2.13f}, bmax{0.9f, 1.45f, 2.13f};   // Golf : 4,26 m
};

static void ecrire32(std::vector<uint8_t>& b, uint32_t v) { for (int k = 0; k < 4; k++) b.push_back((uint8_t)(v >> (8 * k))); }

int main() {
    const float F = 864.f;   // champ vertical 64°, scène de 1080 px de haut
    {   // seuils (WFS-246) : 200 px ≈ 18 m pour une Golf (60 px ≈ 64 m avant), niveaux de plus en plus hauts avec la distance
        const float L = 4.26f;
        VERIFIE(std::fabs(L * F / vehicule_lod::SEUILS_PX[0] - 18.4f) < 1.f, "1er seuil à %.1f m (≈ 18 m attendus, WFS-246)", L * F / vehicule_lod::SEUILS_PX[0]);
        int avant = 0; float tAvant = 0;
        bool monotone = true, fondus[3] = {false, false, false};
        for (float d = 1; d < 400; d += 0.25f) {
            const vehicule_lod::Choix c = vehicule_lod::choisir(vehicule_lod::pixels(L, F, d), 4);
            const float rang = (float)c.a + c.t;   // 0 → 3 sans saut en arrière
            if (rang + 1e-4f < (float)avant + tAvant) monotone = false;
            if (c.t > 0) { fondus[c.a] = true; VERIFIE(c.b == c.a + 1, "fondu de %d vers %d", c.a, c.b); }
            VERIFIE(c.t >= 0 && c.t < 1, "t = %.3f hors de [0, 1)", c.t);
            avant = c.a; tAvant = c.t;
        }
        VERIFIE(monotone, "le niveau ne doit jamais redescendre quand la voiture s'éloigne");
        VERIFIE(fondus[0] && fondus[1] && fondus[2], "fondu tramé à chacun des 3 passages");
        VERIFIE(vehicule_lod::choisir(1000.f, 4).a == 0 && vehicule_lod::choisir(5.f, 4).a == 3, "près : complet ; très loin : niveau 3");
        VERIFIE(vehicule_lod::choisir(5.f, 2).a == 1 && vehicule_lod::choisir(5.f, 1).a == 0, "niveaux absents : le plus haut disponible");
        // fondu sur 1/9 du seuil : de 64 à 72 m dans l'ancien banc, au même rapport
        const vehicule_lod::Choix m = vehicule_lod::choisir(vehicule_lod::SEUILS_PX[0] * (1 - vehicule_lod::BANDE / 2), 4);
        VERIFIE(m.a == 0 && m.b == 1 && std::fabs(m.t - 0.5f) < 0.01f, "milieu du fondu : t = %.3f", m.t);
    }
    {   // listes dessinées : Haute (4 niveaux), Basse (complète puis partsBasse), fantôme (complète), police en lots (partsBasse)
        Modele mo;
        mo.parts = {{0, 30, 0, 0}}; mo.partsLod = {{30, 9, 0, 0}}; mo.partsBasse = {{39, 9, 0, 0}};
        mo.partsNiveaux = {{{48, 6, 0, 0}}, {{54, 3, 0, 0}}};
        auto d = [&](float dist, bool basse, bool lots, bool fantome) { return vehicule_lod::dessin(mo, dist, F, basse, lots, fantome); };
        VERIFIE(d(10, false, false, false).a == &mo.parts, "Haute, 10 m : complète");
        VERIFIE(d(40, false, false, false).a == &mo.partsLod, "Haute, 40 m : niveau 1 (WFS-246)");
        VERIFIE(d(100, false, false, false).a == &mo.partsNiveaux[0], "Haute, 100 m : niveau 2");
        VERIFIE(d(200, false, false, false).a == &mo.partsNiveaux[1], "Haute, 200 m : niveau 3");
        VERIFIE(d(190, true, false, false).a == &mo.partsBasse, "Basse, 190 m : version en 4 pièces");
        VERIFIE(d(10, true, true, false).a == &mo.partsBasse && d(10, true, true, false).t == 0, "police en lots : partsBasse seule, même de près");
        VERIFIE(d(190, false, false, true).a == &mo.parts, "fantôme : complet à toute distance");
        const auto f = d(19.5f, false, false, false);   // WFS-246 : fondu de 18,4 à 20,7 m (64 à 72 m avant)
        VERIFIE(f.a == &mo.parts && f.b == &mo.partsLod && f.t > 0, "19,5 m : fondu complète → niveau 1");
        Modele seul; seul.parts = mo.parts;
        VERIFIE(d(10, false, false, false).niveau == 0 && vehicule_lod::dessin(seul, 300, F, false, false, false).a == &seul.parts, "sans .lod : complète partout");
        Modele trou = mo; trou.partsLod.clear();
        VERIFIE(vehicule_lod::dessin(trou, 300, F, false, false, false).a == &trou.parts, "sans niveau 1, les niveaux 2 et 3 ne sont pas pris");
    }
    {   // lecture WFL1 / WFL2
        const std::vector<Piece> complet = {{0, 6, 0, 0}, {6, 3, 100, 1}};
        std::vector<uint8_t> b1 = {'W', 'F', 'L', '1'};
        ecrire32(b1, 2); ecrire32(b1, 3); for (uint32_t i : {0u, 1u, 2u}) ecrire32(b1, i); ecrire32(b1, 3); for (uint32_t i : {2u, 1u, 0u}) ecrire32(b1, i);
        std::vector<uint32_t> idx, fins; std::vector<Piece> parts;
        VERIFIE(vehicule_lod::lire(b1.data(), b1.size(), complet, idx, parts, fins) && fins.size() == 1 && idx.size() == 6 && parts.size() == 2 &&
                parts[1].vertexOffset == 100 && parts[1].material == 1 && parts[1].firstIndex == 3 && idx[3] == 2, "WFL1 : 1 niveau lu");
        std::vector<uint8_t> b2 = {'W', 'F', 'L', '2'};
        ecrire32(b2, 2); ecrire32(b2, 2);
        for (int niv = 0; niv < 2; niv++) {
            ecrire32(b2, 3); ecrire32(b2, 2); for (uint16_t i : {7, 8, 9}) { b2.push_back((uint8_t)i); b2.push_back(0); } b2.push_back(0); b2.push_back(0);
            ecrire32(b2, 3); ecrire32(b2, 4); for (uint32_t i : {70000u, 1u, 2u}) ecrire32(b2, i);
        }
        VERIFIE(vehicule_lod::lire(b2.data(), b2.size(), complet, idx, parts, fins) && fins.size() == 2 && fins[0] == 6 && fins[1] == 12 &&
                idx[0] == 7 && idx[3] == 70000 && idx[6] == 7 && parts.size() == 4, "WFL2 : 2 niveaux, indices u16 et u32");
        // pièce découpée dans sa plage (comme les tranches des portes) : elle reste dans son niveau
        parts.push_back(parts[2]); parts.back().firstIndex = 8; parts.back().indexCount = 0; parts[2].indexCount = 3;
        std::vector<Piece> n1; std::vector<std::vector<Piece>> autres;
        vehicule_lod::repartir(parts, fins, 1000, n1, autres);
        VERIFIE(n1.size() == 2 && autres.size() == 1 && autres[0].size() == 3 && n1[0].firstIndex == 1000 && autres[0][0].firstIndex == 1006,
                "répartition par niveau (%zu + %zu pièces)", n1.size(), autres.empty() ? 0 : autres[0].size());
        std::vector<uint8_t> court(b2.begin(), b2.end() - 5);
        VERIFIE(!vehicule_lod::lire(court.data(), court.size(), complet, idx, parts, fins) && idx.empty() && parts.empty(), "fichier tronqué refusé");
        const std::vector<Piece> autre = {{0, 6, 0, 0}};
        VERIFIE(!vehicule_lod::lire(b2.data(), b2.size(), autre, idx, parts, fins), "nombre de maillages différent du .wfm : refusé");
        std::vector<uint8_t> faux = b2; faux[3] = '9';
        VERIFIE(!vehicule_lod::lire(faux.data(), faux.size(), complet, idx, parts, fins), "en-tête inconnu refusé");
    }
#ifdef WFS_SOURCE_DIR
    {   // fichiers du dépôt : les 20 voitures ont leurs 3 niveaux allégés, chacun plus léger que le précédent
        const char* ids[] = {"a45", "abarth", "compass", "corsa", "elantra", "gallardo", "giulia", "golf", "gt86", "m4",
                             "model3", "mustang", "p208", "p911", "rrsport", "s3", "sf90", "tcross", "v60", "x5"};
        for (const char* id : ids) {
            auto lireFichier = [](const std::string& c) { std::ifstream f(c, std::ios::binary); return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), {}); };
            const std::string base = std::string(WFS_SOURCE_DIR) + "/assets/cars/" + id;
            const std::vector<uint8_t> w = lireFichier(base + ".wfm"), l = lireFichier(base + ".lod");
            if (w.size() < 44 || l.size() < 12) { VERIFIE(false, "%s : .wfm ou .lod absent", id); continue; }
            // maillages du .wfm : nombre et triangles (en-tête, textures, matériaux de 56 octets, maillages)
            auto u32 = [&](size_t p) { uint32_t v; std::memcpy(&v, &w[p], 4); return v; };
            const uint32_t nTex = u32(8), nMat = u32(12), nMesh = u32(16);
            size_t p = 44;
            for (uint32_t i = 0; i < nTex; i++) p += 4 + u32(p);
            p += (size_t)nMat * 56;
            std::vector<Piece> complet; uint64_t triComplet = 0;
            for (uint32_t i = 0; i < nMesh && p + 12 <= w.size(); i++) {
                const uint32_t nv = u32(p + 4), ni = u32(p + 8);
                complet.push_back({0, ni, 0, u32(p)}); triComplet += ni / 3;
                p += 12 + (size_t)nv * 32 + (size_t)ni * 4;
            }
            std::vector<uint32_t> idx, fins; std::vector<Piece> parts;
            const bool ok = vehicule_lod::lire(l.data(), l.size(), complet, idx, parts, fins);
            VERIFIE(ok && fins.size() == 3, "%s : .lod illisible ou sans 3 niveaux (%zu)", id, fins.size());
            if (!ok || fins.size() != 3) continue;
            const double t1 = fins[0] / 3.0, t2 = (fins[1] - fins[0]) / 3.0, t3 = (fins[2] - fins[1]) / 3.0;
            VERIFIE(t1 >= 0.25 * (double)triComplet && t1 < (double)triComplet && t2 < t1 && t3 < t2 && t3 > 0,
                    "%s : triangles %llu → %.0f → %.0f → %.0f (niveaux de plus en plus légers, ≥ 25 %% au niveau 1)", id,
                    (unsigned long long)triComplet, t1, t2, t3);
            std::printf("%-9s %7llu → %6.0f (%4.1f %%) → %6.0f (%4.1f %%) → %6.0f (%4.1f %%)\n", id, (unsigned long long)triComplet,
                        t1, 100 * t1 / (double)triComplet, t2, 100 * t2 / (double)triComplet, t3, 100 * t3 / (double)triComplet);
        }
    }
#endif
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("niveaux de détail des voitures : OK\n");
    return 0;
}
