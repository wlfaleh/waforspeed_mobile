// WFS-149 (RG-01) : lecture des textures KTX2 (src/render/ktx2.cpp) : formats du moteur, chaîne complète de mipmaps,
// tailles des niveaux, et refus propre (jamais de plantage) des fichiers corrompus ou hors périmètre.
// Argument facultatif : dossier de fichiers .ktx2 produits par tools/textures/convertir.py (tous doivent être lus).
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "../src/render/ktx2.h"

using namespace wfs;
static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// fichier KTX2 minimal écrit comme tools/textures/ktx2.py (niveaux du plus petit au plus grand, alignés sur 16 octets)
static std::vector<uint8_t> fabriquer(uint32_t fmt, uint32_t w, uint32_t h, uint32_t bw, uint32_t bh, uint32_t ob, int nNiv = -1) {
    std::vector<std::pair<uint32_t, uint32_t>> dims{{w, h}};
    while (dims.back().first > 1 || dims.back().second > 1) dims.push_back({std::max(1u, dims.back().first / 2), std::max(1u, dims.back().second / 2)});
    if (nNiv > 0) dims.resize(nNiv);
    const uint32_t n = (uint32_t)dims.size();
    std::vector<uint8_t> b(80 + 24 * n, 0);
    const uint8_t id[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
    std::memcpy(b.data(), id, 12);
    const uint32_t tete[9] = {fmt, 1, w, h, 0, 0, 1, n, 0};
    std::memcpy(b.data() + 12, tete, sizeof tete);
    for (int i = (int)n - 1; i >= 0; i--) {
        while (b.size() % 16) b.push_back(0);
        const uint64_t taille = (uint64_t)((dims[i].first + bw - 1) / bw) * ((dims[i].second + bh - 1) / bh) * ob, ofs = b.size();
        std::memcpy(b.data() + 80 + 24 * i, &ofs, 8); std::memcpy(b.data() + 88 + 24 * i, &taille, 8); std::memcpy(b.data() + 96 + 24 * i, &taille, 8);
        for (uint64_t k = 0; k < taille; k++) b.push_back((uint8_t)(k * 7 + i));
    }
    return b;
}

int main(int argc, char** argv) {
    // ---- formats du moteur : ASTC 4x4 / 6x6, ETC2 RGBA, RGBA8 ; sRGB et linéaire
    struct Cas { VkFormat f; uint32_t bw, bh, ob; bool srgb; };
    const Cas cas[] = {{VK_FORMAT_ASTC_4x4_SRGB_BLOCK, 4, 4, 16, true}, {VK_FORMAT_ASTC_4x4_UNORM_BLOCK, 4, 4, 16, false},
                       {VK_FORMAT_ASTC_6x6_SRGB_BLOCK, 6, 6, 16, true}, {VK_FORMAT_ASTC_6x6_UNORM_BLOCK, 6, 6, 16, false},
                       {VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, 4, 4, 16, true}, {VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, 4, 4, 16, false},
                       {VK_FORMAT_R8G8B8A8_SRGB, 1, 1, 4, true}, {VK_FORMAT_R8G8B8A8_UNORM, 1, 1, 4, false}};
    for (const Cas& c : cas) for (auto [w, h] : {std::pair{256u, 256u}, std::pair{300u, 100u}, std::pair{1u, 1u}, std::pair{7u, 5u}}) {
        const std::vector<uint8_t> b = fabriquer(c.f, w, h, c.bw, c.bh, c.ob);
        ktx2::Fichier f; std::string e;
        const bool ok = ktx2::lire(b.data(), b.size(), f, e);
        VERIFIE(ok, "%s %ux%u refusé : %s", ktx2::nomFormat(c.f), w, h, e.c_str());
        if (!ok) continue;
        uint32_t chaine = 1; for (uint32_t m = std::max(w, h); m > 1; m >>= 1) chaine++;
        VERIFIE(f.niveaux.size() == chaine, "%s %ux%u : %zu niveaux (chaîne de %u attendue)", ktx2::nomFormat(c.f), w, h, f.niveaux.size(), chaine);
        VERIFIE(f.bloc.srgb == c.srgb && f.bloc.w == c.bw && f.bloc.octets == c.ob, "%s : bloc ou espace colorimétrique faux", ktx2::nomFormat(c.f));
        VERIFIE(f.niveaux.back().w == 1 && f.niveaux.back().h == 1, "%s %ux%u : dernier niveau ≠ 1×1", ktx2::nomFormat(c.f), w, h);
        VERIFIE(f.niveaux.size() == 1 || f.niveaux[0].offset > f.niveaux.back().offset, "%s : niveaux non rangés du plus petit au plus grand", ktx2::nomFormat(c.f));
    }
    std::printf("ktx2 : 8 formats × 4 tailles lus, chaînes complètes\n");

    // ---- refus propres
    const std::vector<uint8_t> bon = fabriquer(VK_FORMAT_ASTC_4x4_SRGB_BLOCK, 64, 64, 4, 4, 16);
    auto refuse = [&](std::vector<uint8_t> b, const char* quoi) {
        ktx2::Fichier f; std::string e;
        VERIFIE(!ktx2::lire(b.data(), b.size(), f, e), "accepté à tort : %s", quoi);
        if (!e.empty()) std::printf("  refus « %s » : %s\n", quoi, e.c_str());
    };
    { auto b = bon; b[3] = 'Y'; refuse(b, "identifiant faux"); }
    { auto b = bon; b.resize(60); refuse(b, "en-tête tronqué"); }
    { auto b = bon; b.resize(b.size() - 1); refuse(b, "dernier niveau tronqué"); }
    { auto b = bon; const uint32_t f = 131; std::memcpy(&b[12], &f, 4); refuse(b, "format BC1 (hors moteur)"); }
    { auto b = bon; const uint32_t s = 1; std::memcpy(&b[44], &s, 4); refuse(b, "supercompression BasisLZ"); }
    { auto b = bon; const uint32_t fc = 6; std::memcpy(&b[36], &fc, 4); refuse(b, "cube"); }
    { auto b = bon; const uint32_t c = 4; std::memcpy(&b[32], &c, 4); refuse(b, "tableau"); }
    { auto b = bon; const uint32_t z = 0; std::memcpy(&b[20], &z, 4); refuse(b, "largeur nulle"); }
    { auto b = bon; const uint64_t t = 15; std::memcpy(&b[88], &t, 8); refuse(b, "taille de niveau fausse"); }
    { auto b = bon; const uint64_t o = (uint64_t)1 << 40; std::memcpy(&b[80], &o, 8); refuse(b, "décalage hors du fichier"); }
    refuse(fabriquer(VK_FORMAT_ASTC_4x4_SRGB_BLOCK, 64, 64, 4, 4, 16, 1), "mipmaps absentes (1 niveau)");
    refuse(fabriquer(VK_FORMAT_R8G8B8A8_SRGB, 64, 64, 1, 1, 4, 4), "chaîne de mipmaps incomplète");
    refuse({}, "fichier vide");

    // ---- fichiers de l'outil (tools/textures/convertir.py), s'ils sont donnés
    int lus = 0;
    if (argc > 1 && std::filesystem::is_directory(argv[1]))
        for (const auto& e : std::filesystem::directory_iterator(argv[1])) {
            if (e.path().extension() != ".ktx2") continue;
            std::ifstream in(e.path(), std::ios::binary);
            const std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
            ktx2::Fichier f; std::string err;
            VERIFIE(ktx2::lire(b.data(), b.size(), f, err), "%s illisible : %s", e.path().filename().c_str(), err.c_str());
            lus++;
        }
    if (argc > 1) std::printf("ktx2 : %d fichier(s) de l'outil lus dans %s\n", lus, argv[1]);
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("test_ktx2 : OK\n");
    return 0;
}
