// WFS-492 : skins de la voiture du joueur (src/render/vehicule_skin.h) — partie vérifiable par le calcul :
//   - décodeur EAC R11 / RG11 (valeurs exactes 0 et 2047, multiplicateur nul, ordre des texels en colonnes) ;
//   - identifiants des skins (BOUTIQUE_SKINS_IDS) et chemin des textures ;
//   - textures livrées (assets/cars/skins/<voiture>-rallye.etc2.ktx2) : les 6 voitures du 1er lot, EAC RG11 1024 × 1024, chaîne
//     complète de mipmaps, ≤ 1,5 Mo sur le disque (RG-02) ; dessin au bon endroit du dépliage (atlas de vehicule_skin.h) :
//     larges bandes du dessus, peinture entre elles, filet, numéro sur les flancs, rien dans les marges entre zones.
// Argument facultatif : dossier des assets (défaut : assets, depuis la racine du dépôt).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "../src/render/ktx2.h"
#include "../src/render/vehicule_skin.h"

using namespace wfs;
namespace vs = wfs::vehicule_skin;
static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// niveau 0 décodé (R, G sur 8 bits)
struct Decode { uint32_t w = 0, h = 0; std::vector<uint8_t> rg; uint8_t r(int x, int y) const { return rg[((size_t)y * w + x) * 2]; } uint8_t g(int x, int y) const { return rg[((size_t)y * w + x) * 2 + 1]; } };
static Decode decoder(const ktx2::Fichier& f) {
    Decode d; d.w = f.w; d.h = f.h; d.rg.assign((size_t)f.w * f.h * 2, 0);
    const uint32_t bx = f.w / 4;
    for (uint32_t j = 0; j < f.h / 4; j++) for (uint32_t i = 0; i < bx; i++) {
        uint8_t px[16][2];
        vs::decoderEacRG11(f.donnees + f.niveaux[0].offset + ((size_t)j * bx + i) * 16, px);
        for (int y = 0; y < 4; y++) for (int x = 0; x < 4; x++) std::memcpy(&d.rg[((size_t)(j * 4 + y) * f.w + i * 4 + x) * 2], px[x + 4 * y], 2);
    }
    return d;
}
// pixel du dessus pour (x, z) en mètres ; d'un flanc pour (z, y)
static void pxDessus(float x, float z, int& u, int& v) {
    namespace a = vs::atlas;
    u = (int)((z + a::DEMI_LONGUEUR) / (2 * a::DEMI_LONGUEUR) * a::TAILLE);
    v = (int)(a::DESSUS0 + (x + a::DEMI_LARGEUR) / (2 * a::DEMI_LARGEUR) * (a::DESSUS1 - a::DESSUS0));
}

int main(int argc, char** argv) {
    // ---- décodeur EAC R11 : base 0, table 13, multiplicateur 1, indices 0 (−1) → 4 − 8 = −4 → 0 (bornée)
    {
        const uint8_t noir[8] = {0, 0x1D, 0, 0, 0, 0, 0, 0};
        uint16_t v[16]; vs::decoderEacR11(noir, v);
        bool ok = true; for (uint16_t x : v) ok &= x == 0;
        VERIFIE(ok, "EAC : bloc noir (base 0, table 13) : %u attendu 0", v[0]);
        // base 255, table 13, multiplicateur 1, indice 5 (+1) partout → 2044 + 8 → 2047 (bornée), soit 255 sur 8 bits
        uint8_t blanc[8] = {255, 0x1D, 0, 0, 0, 0, 0, 0};
        uint64_t bits = 0; for (int i = 0; i < 16; i++) bits = (bits << 3) | 5;
        for (int k = 0; k < 6; k++) blanc[2 + k] = (uint8_t)(bits >> (40 - 8 * k));
        vs::decoderEacR11(blanc, v);
        ok = true; for (uint16_t x : v) ok &= x == 2047;
        VERIFIE(ok, "EAC : bloc blanc : %u attendu 2047", v[0]);
        // multiplicateur nul : base × 8 + 4 + modificateur (sans × 8) ; table 0, indice 7 (+14) : 100 × 8 + 4 + 14 = 818
        uint8_t m0[8] = {100, 0x00, 0, 0, 0, 0, 0, 0};
        bits = 0; for (int i = 0; i < 16; i++) bits = (bits << 3) | 7;
        for (int k = 0; k < 6; k++) m0[2 + k] = (uint8_t)(bits >> (40 - 8 * k));
        vs::decoderEacR11(m0, v);
        VERIFIE(v[0] == 818 && v[15] == 818, "EAC : multiplicateur nul : %u attendu 818", v[0]);
        // ordre des texels : le 2e indice (bits 44..42) est le texel (0, 1) (colonnes d'abord)
        uint8_t ordre[8] = {128, 0x1D, 0, 0, 0, 0, 0, 0};   // indices 0 (−1 × 8) sauf le 2e : 7 (+9 × 8)
        bits = (uint64_t)7 << 42;
        for (int k = 0; k < 6; k++) ordre[2 + k] = (uint8_t)(bits >> (40 - 8 * k));
        vs::decoderEacR11(ordre, v);
        VERIFIE(v[0 + 4 * 1] == 128 * 8 + 4 + 72 && v[1] == 128 * 8 + 4 - 8, "EAC : ordre des texels : (0,1) = %u, (1,0) = %u", v[4], v[1]);
        // RG11 : R puis G, arrondis sur 8 bits
        uint8_t rg[16]; std::memcpy(rg, blanc, 8); std::memcpy(rg + 8, noir, 8);
        uint8_t px[16][2]; vs::decoderEacRG11(rg, px);
        VERIFIE(px[0][0] == 255 && px[0][1] == 0 && px[15][0] == 255, "EAC RG11 : (%u, %u) attendu (255, 0)", px[0][0], px[0][1]);
    }
    // ---- identifiants et chemins
    VERIFIE(vs::depuisId("rallye") == 1 && vs::depuisId("camouflage") == 4, "skins : numéros des identifiants");
    VERIFIE(vs::depuisId("") == vs::AUCUNE && vs::depuisId(nullptr) == vs::AUCUNE && vs::depuisId("inconnue") == vs::AUCUNE, "skins : vide ou inconnue = aucune");
    VERIFIE(vs::chemin("golf", "rallye") == "cars/skins/golf-rallye", "skins : chemin %s", vs::chemin("golf", "rallye").c_str());
    // métal codé : métal + 2 × finition (< 16) + 16 × skin se relit sans perte (même calcul que lireFinition du shader)
    for (int skin = 0; skin <= vs::NOMBRE; skin++) for (int fin = 0; fin < 8; fin++) for (float metal : {0.f, 0.35f, 1.f}) {
        const float code = metal + 2.f * fin + 16.f * skin;
        const float k = std::floor(code / 16.f), reste = code - 16.f * k, f = std::floor(reste * 0.5f);
        VERIFIE((int)k == skin && (int)f == fin && std::fabs(reste - 2.f * f - metal) < 1e-5f, "codage : skin %d finition %d métal %.2f", skin, fin, metal);
    }
    // ---- textures livrées
    const std::string dossier = (argc > 1 ? std::string(argv[1]) : std::string("assets")) + "/cars/skins/";
    int lues = 0;
    for (const char* id : {"p208", "abarth", "gt86", "golf", "mustang", "giulia"}) {
        const std::string chemin = dossier + id + "-rallye.etc2.ktx2";
        std::ifstream in(chemin, std::ios::binary);
        std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        VERIFIE(!b.empty(), "%s absente", chemin.c_str());
        if (b.empty()) continue;
        VERIFIE(b.size() <= 1572864, "%s : %zu octets > 1,5 Mo (RG-02)", chemin.c_str(), b.size());
        ktx2::Fichier f; std::string err;
        const bool ok = ktx2::lire(b.data(), b.size(), f, err);
        VERIFIE(ok, "%s illisible : %s", chemin.c_str(), err.c_str());
        if (!ok) continue;
        VERIFIE(f.format == VK_FORMAT_EAC_R11G11_UNORM_BLOCK && f.w == 1024 && f.h == 1024 && f.niveaux.size() == 11, "%s : EAC RG11 1024 × 1024, 11 niveaux attendus", chemin.c_str());
        if (f.w != 1024 || f.h != 1024) continue;
        const Decode d = decoder(f);
        int u, v;
        pxDessus(0.155f, 0.f, u, v);   // milieu d'une large bande : couleur A
        VERIFIE(d.r(u, v) > 240 && d.g(u, v) < 15, "%s : bande du dessus : (%u, %u)", id, d.r(u, v), d.g(u, v));
        pxDessus(-0.155f, -1.8f, u, v);
        VERIFIE(d.r(u, v) > 240, "%s : bande gauche, à l'avant : %u", id, d.r(u, v));
        pxDessus(0.f, 0.f, u, v);   // entre les bandes : peinture
        VERIFIE(d.r(u, v) < 15 && d.g(u, v) < 15, "%s : peinture entre les bandes : (%u, %u)", id, d.r(u, v), d.g(u, v));
        pxDessus(0.2795f, 0.f, u, v);   // filet : couleur B
        VERIFIE(d.g(u, v) > 200, "%s : filet : %u", id, d.g(u, v));
        pxDessus(0.6f, 0.f, u, v);   // bord du toit : peinture (passage vers le flanc sans motif)
        VERIFIE(d.r(u, v) < 15 && d.g(u, v) < 15, "%s : bord du dessus : (%u, %u)", id, d.r(u, v), d.g(u, v));
        // flancs : la plaque de numéro (A) porte des chiffres (B) ; les deux flancs en ont autant (dessin symétrique)
        long aG = 0, bG = 0, aD = 0, bD = 0;
        for (int y = vs::atlas::GAUCHE0; y < vs::atlas::GAUCHE1; y++) for (int x = 0; x < 1024; x++) { aG += d.r(x, y) > 128; bG += d.g(x, y) > 128; }
        for (int y = vs::atlas::DROITE0; y < vs::atlas::DROITE1; y++) for (int x = 0; x < 1024; x++) { aD += d.r(x, y) > 128; bD += d.g(x, y) > 128; }
        VERIFIE(aG > 2000 && bG > 1000 && std::labs(aG - aD) < aG / 50 && std::labs(bG - bD) < bG / 20, "%s : flancs (A %ld / %ld, B %ld / %ld)", id, aG, aD, bG, bD);
        lues++;
    }
    VERIFIE(lues == 6, "%d textures de skin lues sur 6", lues);
    if (echecs) { std::printf("test_vehicule_skin : %d échec(s)\n", echecs); return 1; }
    std::printf("test_vehicule_skin : OK (décodeur EAC, identifiants, codage, %d textures Rallye ≤ 1,5 Mo)\n", lues);
    return 0;
}
