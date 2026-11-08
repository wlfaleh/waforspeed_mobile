// Cache de pipelines (WFS-262, RG-01) : en-tête vérifié (fournisseur, appareil, UUID), écriture atomique, relecture.
// usage : test_cache_pipelines
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "../src/render/cache_pipelines.h"
using namespace wfs::cache_pipelines;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static void ecrire32(std::vector<uint8_t>& b, size_t o, uint32_t v) { for (int k = 0; k < 4; k++) b[o + k] = (uint8_t)(v >> (8 * k)); }

int main() {
    Identite id; id.fournisseur = 0x13B5; id.appareil = 0x92020010; for (int i = 0; i < 16; i++) id.uuid[i] = (uint8_t)(i * 7 + 1);
    std::vector<uint8_t> c(200, 0xAB);
    ecrire32(c, 0, 32); ecrire32(c, 4, 1); ecrire32(c, 8, id.fournisseur); ecrire32(c, 12, id.appareil); std::memcpy(c.data() + 16, id.uuid, 16);
    VERIFIE(enteteValide(c.data(), c.size(), id), "cache du même appareil accepté");
    Identite autre = id; autre.appareil++;
    VERIFIE(!enteteValide(c.data(), c.size(), autre), "autre appareil refusé");
    autre = id; autre.uuid[3] ^= 1;
    VERIFIE(!enteteValide(c.data(), c.size(), autre), "autre pilote (UUID) refusé");
    std::vector<uint8_t> v2 = c; ecrire32(v2, 4, 2);
    VERIFIE(!enteteValide(v2.data(), v2.size(), id), "version inconnue refusée");
    VERIFIE(!enteteValide(c.data(), 20, id) && !enteteValide(nullptr, 0, id), "fichier tronqué ou vide refusé");
    std::vector<uint8_t> v3 = c; ecrire32(v3, 0, 500);
    VERIFIE(!enteteValide(v3.data(), v3.size(), id), "taille d'en-tête plus grande que le fichier refusée");
    const std::string ch = "test_cache_pipelines.tmp.cache";
    VERIFIE(ecrire(ch, c) && lire(ch) == c, "écriture puis relecture identiques");
    VERIFIE(lire("fichier_absent_wfs262.cache").empty() && !ecrire("", c), "fichier absent : vide ; chemin vide : rien écrit");
    std::remove(ch.c_str());
    if (echecs) { std::printf("test_cache_pipelines : %d échec(s)\n", echecs); return 1; }
    std::printf("cache de pipelines (WFS-262) : en-tête vérifié, écriture atomique : OK\n");
    return 0;
}
