// WaForSpeed — WFS-492 : skins, partie sans Vulkan (vehicule_skin.h) : identifiants, chemins et décodeur EAC ; testée par
// tests/test_vehicule_skin.cpp.
#include <cstring>
#include "../core/configgame.h"
#include "vehicule_skin.h"

namespace wfs::vehicule_skin {

namespace {
// modificateurs des 16 tables (Khronos Data Format Specification 1.3, tableau 21.12, communs à EAC R11 et à l'alpha d'ETC2)
constexpr int TABLE[16][8] = {
    {-3, -6, -9, -15, 2, 5, 8, 14}, {-3, -7, -10, -13, 2, 6, 9, 12}, {-2, -5, -8, -13, 1, 4, 7, 12}, {-2, -4, -6, -13, 1, 3, 5, 12},
    {-3, -6, -8, -12, 2, 5, 7, 11}, {-3, -7, -9, -11, 2, 6, 8, 10}, {-4, -7, -8, -11, 3, 6, 7, 10}, {-3, -5, -8, -11, 2, 4, 7, 10},
    {-2, -6, -8, -10, 1, 5, 7, 9},  {-2, -5, -8, -10, 1, 4, 7, 9},  {-2, -4, -8, -10, 1, 3, 7, 9},  {-2, -5, -7, -10, 1, 4, 6, 9},
    {-3, -4, -7, -10, 2, 3, 6, 9},  {-1, -2, -3, -10, 0, 1, 2, 9},  {-4, -6, -8, -9, 3, 5, 7, 8},   {-3, -5, -7, -9, 2, 4, 6, 8}};

} // namespace

void decoderEacR11(const uint8_t b[8], uint16_t sortie[16]) {
    const int base = b[0], mult = b[1] >> 4;
    const int* t = TABLE[b[1] & 0xF];
    uint64_t bits = 0;
    for (int k = 2; k < 8; k++) bits = (bits << 8) | b[k];
    for (int i = 0; i < 16; i++) {   // i = x × 4 + y (colonnes d'abord), indice du texel (0,0) dans les bits de poids fort
        const int x = i / 4, y = i % 4;
        const int m = t[(bits >> (45 - 3 * i)) & 7];
        // multiplicateur nul : modificateur × 1/8 avant le × 8 (spécification § 21.3) → base × 8 + 4 + modificateur
        int v = base * 8 + 4 + (mult ? m * mult * 8 : m);
        v = v < 0 ? 0 : (v > 2047 ? 2047 : v);
        sortie[x + 4 * y] = (uint16_t)v;
    }
}

void decoderEacRG11(const uint8_t bloc[16], uint8_t sortie[16][2]) {
    uint16_t r[16], g[16];
    decoderEacR11(bloc, r); decoderEacR11(bloc + 8, g);
    for (int i = 0; i < 16; i++) {   // UNORM 11 bits → 8 bits arrondis : v / 2047 × 255
        sortie[i][0] = (uint8_t)((r[i] * 255u + 1023u) / 2047u);
        sortie[i][1] = (uint8_t)((g[i] * 255u + 1023u) / 2047u);
    }
}

int depuisId(const char* id) {
    if (!id || !*id) return AUCUNE;
    for (int i = 0; i < BOUTIQUE_SKINS && i < NOMBRE; i++) if (std::strcmp(id, BOUTIQUE_SKINS_IDS[i]) == 0) return i + 1;
    return AUCUNE;
}

bool couleurs(int skin, float a[3], float b[3]) {
    // codes sRGB 0..1, mêmes valeurs que SKIN_COULEURS (shaders/vehicule.glsl) : 2 couleurs par skin, numéros 0 à NOMBRE
    static const float C[(NOMBRE + 1) * 2][3] = {{0, 0, 0}, {0, 0, 0},
        {0.91f, 0.91f, 0.89f}, {0.10f, 0.11f, 0.125f},   // 1 Rallye
        {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};   // 2 à 4 : WFS-493
    if (skin <= AUCUNE || skin > NOMBRE) return false;
    for (int c = 0; c < 3; c++) { a[c] = C[skin * 2][c]; b[c] = C[skin * 2 + 1][c]; }
    return true;
}

std::string chemin(const char* idVoiture, const char* idSkin) {
    return std::string("cars/skins/") + (idVoiture ? idVoiture : "") + "-" + (idSkin ? idSkin : "");
}

} // namespace wfs::vehicule_skin
