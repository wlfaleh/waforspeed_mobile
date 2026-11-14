// WaForSpeed — WFS-495 : compression ETC2 RGBA8 de la texture d'habillage cuite (voir vehicule_etc2.h). Sans Vulkan ; testée par
// tests/test_vehicule_autocollants.cpp (codage puis décodage : PSNR, blocs unis exacts).
#include "vehicule_etc2.h"
#include <algorithm>
#include <cstring>

namespace wfs::vehicule_etc2 {

namespace {

// ETC1 / ETC2 : tables des modificateurs de luminance (Khronos Data Format Specification 1.3, tableau 21.4) ; indice de texel
// 0 → +a, 1 → +b, 2 → −a, 3 → −b
constexpr int ETC_TABLE[8][2] = {{2, 8}, {5, 17}, {9, 29}, {13, 42}, {18, 60}, {24, 80}, {33, 106}, {47, 183}};
// EAC (tableau 21.12, commun à R11 et à l'alpha d'ETC2)
constexpr int EAC_TABLE[16][8] = {
    {-3, -6, -9, -15, 2, 5, 8, 14}, {-3, -7, -10, -13, 2, 6, 9, 12}, {-2, -5, -8, -13, 1, 4, 7, 12}, {-2, -4, -6, -13, 1, 3, 5, 12},
    {-3, -6, -8, -12, 2, 5, 7, 11}, {-3, -7, -9, -11, 2, 6, 8, 10}, {-4, -7, -8, -11, 3, 6, 7, 10}, {-3, -5, -8, -11, 2, 4, 7, 10},
    {-2, -6, -8, -10, 1, 5, 7, 9},  {-2, -5, -8, -10, 1, 4, 7, 9},  {-2, -4, -8, -10, 1, 3, 7, 9},  {-2, -5, -7, -10, 1, 4, 6, 9},
    {-3, -4, -7, -10, 2, 3, 6, 9},  {-1, -2, -3, -10, 0, 1, 2, 9},  {-4, -6, -8, -9, 3, 5, 7, 8},   {-3, -5, -7, -9, 2, 4, 6, 8}};

inline int borne(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }
inline int modificateur(int table, int indice) { const int v = ETC_TABLE[table][indice & 1]; return indice & 2 ? -v : v; }
// texel i (= x × 4 + y) dans le sous-bloc 0 ?
inline bool sousBloc0(int i, int flip) { return flip ? (i % 4) < 2 : (i / 4) < 2; }

void ecrire64(uint64_t v, uint8_t* o) { for (int k = 0; k < 8; k++) o[k] = (uint8_t)(v >> (56 - 8 * k)); }
uint64_t lire64(const uint8_t* o) { uint64_t v = 0; for (int k = 0; k < 8; k++) v = (v << 8) | o[k]; return v; }

// ---- alpha EAC 8 bits
void encoderAlpha(const uint8_t t[16][4], uint8_t o[8]) {
    int amin = 255, amax = 0;
    for (int i = 0; i < 16; i++) { amin = std::min(amin, (int)t[i][3]); amax = std::max(amax, (int)t[i][3]); }
    int bBase = amin, bMult = 1, bTable = 13;
    uint8_t bIdx[16];
    for (int i = 0; i < 16; i++) bIdx[i] = 4;   // table 13, indice 4 : modificateur 0 → valeur exacte (bloc uni)
    if (amax > amin) {
        long meilleur = -1;
        for (int tb = 0; tb < 16; tb++) {
            const int* m = EAC_TABLE[tb];
            const int mmin = m[3], mmax = m[7];   // plus petit et plus grand modificateur de chaque table
            const int m0 = std::clamp((int)((amax - amin) / (double)(mmax - mmin) + 0.5), 1, 15);
            for (int mult = std::max(1, m0 - 1); mult <= std::min(15, m0 + 1); mult++) {
                const int base = borne((int)((amax + amin) * 0.5 - mult * (mmax + mmin) * 0.5 + 0.5));
                long err = 0; uint8_t idx[16];
                for (int i = 0; i < 16 && (meilleur < 0 || err < meilleur); i++) {
                    int be = 1 << 30, bi = 0;
                    for (int k = 0; k < 8; k++) {
                        const int d = borne(base + m[k] * mult) - t[i][3];
                        if (d * d < be) { be = d * d; bi = k; }
                    }
                    err += be; idx[i] = (uint8_t)bi;
                }
                if (meilleur < 0 || err < meilleur) { meilleur = err; bBase = base; bMult = mult; bTable = tb; std::memcpy(bIdx, idx, 16); }
            }
        }
    }
    uint64_t bits = 0;
    for (int i = 0; i < 16; i++) bits |= (uint64_t)bIdx[i] << (45 - 3 * i);
    o[0] = (uint8_t)bBase; o[1] = (uint8_t)((bMult << 4) | bTable);
    for (int k = 0; k < 6; k++) o[2 + k] = (uint8_t)(bits >> (40 - 8 * k));
}

// ---- couleur ETC1 (modes individuel et différentiel d'ETC2)
// meilleure table et indices d'un sous-bloc pour la couleur de base c (8 bits) ; renvoie l'erreur quadratique
long choisirTable(const uint8_t t[16][4], int flip, int sb, const int c[3], int& table, uint8_t idx[16]) {
    long meilleur = -1;
    uint8_t essai[16];
    for (int tb = 0; tb < 8; tb++) {
        long err = 0;
        for (int i = 0; i < 16; i++) {
            if (sousBloc0(i, flip) != (sb == 0)) continue;
            long be = -1; int bi = 0;
            for (int k = 0; k < 4; k++) {
                const int m = modificateur(tb, k);
                long e = 0;
                for (int ch = 0; ch < 3; ch++) { const int d = borne(c[ch] + m) - t[i][ch]; e += d * d; }
                if (be < 0 || e < be) { be = e; bi = k; }
            }
            err += be; essai[i] = (uint8_t)bi;
        }
        if (meilleur < 0 || err < meilleur) {
            meilleur = err; table = tb;
            for (int i = 0; i < 16; i++) if (sousBloc0(i, flip) == (sb == 0)) idx[i] = essai[i];
        }
    }
    return meilleur;
}

// bloc d'une seule couleur (la plupart : fond transparent, intérieur d'un motif) : mode différentiel, écart nul, la meilleure
// paire (table, indice) pour cette couleur, sans recherche par texel
bool encoderUni(const uint8_t t[16][4], uint64_t& v) {
    for (int i = 1; i < 16; i++) if (std::memcmp(t[i], t[0], 3) != 0) return false;
    int q[3], c[3];
    for (int ch = 0; ch < 3; ch++) { q[ch] = std::clamp((t[0][ch] * 31 + 127) / 255, 0, 31); c[ch] = (q[ch] << 3) | (q[ch] >> 2); }
    long be = -1; int bt = 0, bk = 0;
    for (int tb = 0; tb < 8; tb++) for (int k = 0; k < 4; k++) {
        long e = 0;
        for (int ch = 0; ch < 3; ch++) { const int d = borne(c[ch] + modificateur(tb, k)) - t[0][ch]; e += d * d; }
        if (be < 0 || e < be) { be = e; bt = tb; bk = k; }
    }
    v = 0;
    for (int ch = 0; ch < 3; ch++) v |= (uint64_t)q[ch] << (59 - 8 * ch);   // écarts nuls
    v |= (uint64_t)bt << 37 | (uint64_t)bt << 34 | 1ull << 33;
    for (int i = 0; i < 16; i++) v |= (uint64_t)((bk >> 1) & 1) << (16 + i) | (uint64_t)(bk & 1) << i;
    return true;
}

uint64_t encoderCouleur(const uint8_t t[16][4]) {
    uint64_t uni;
    if (encoderUni(t, uni)) return uni;
    long meilleur = -1; uint64_t sortie = 0;
    for (int flip = 0; flip < 2; flip++) {
        double moy[2][3] = {};
        for (int i = 0; i < 16; i++) for (int ch = 0; ch < 3; ch++) moy[sousBloc0(i, flip) ? 0 : 1][ch] += t[i][ch] / 8.0;
        for (int diff = 0; diff < 2; diff++) {
            int q[2][3], c[2][3];
            for (int sb = 0; sb < 2; sb++) for (int ch = 0; ch < 3; ch++) {
                if (diff) {
                    q[sb][ch] = std::clamp((int)(moy[sb][ch] * 31.0 / 255.0 + 0.5), 0, 31);
                } else {
                    q[sb][ch] = std::clamp((int)(moy[sb][ch] * 15.0 / 255.0 + 0.5), 0, 15);
                }
            }
            if (diff)   // écart de −4 à +3 (3 bits signés) : la 2e couleur est ramenée dans l'intervalle (jamais de débordement)
                for (int ch = 0; ch < 3; ch++) q[1][ch] = std::clamp(q[1][ch], std::max(0, q[0][ch] - 4), std::min(31, q[0][ch] + 3));
            for (int sb = 0; sb < 2; sb++) for (int ch = 0; ch < 3; ch++)
                c[sb][ch] = diff ? (q[sb][ch] << 3) | (q[sb][ch] >> 2) : q[sb][ch] * 17;
            int tb[2] = {0, 0}; uint8_t idx[16] = {};
            const long err = choisirTable(t, flip, 0, c[0], tb[0], idx) + choisirTable(t, flip, 1, c[1], tb[1], idx);
            if (meilleur >= 0 && err >= meilleur) continue;
            meilleur = err;
            uint64_t v = 0;
            if (diff) {
                for (int ch = 0; ch < 3; ch++) {
                    const int d = q[1][ch] - q[0][ch];
                    v |= (uint64_t)q[0][ch] << (59 - 8 * ch);
                    v |= (uint64_t)(d & 7) << (56 - 8 * ch);
                }
                v |= 1ull << 33;
            } else {
                for (int ch = 0; ch < 3; ch++) {
                    v |= (uint64_t)q[0][ch] << (60 - 8 * ch);
                    v |= (uint64_t)q[1][ch] << (56 - 8 * ch);
                }
            }
            v |= (uint64_t)tb[0] << 37 | (uint64_t)tb[1] << 34 | (uint64_t)flip << 32;
            for (int i = 0; i < 16; i++) {
                // bits de poids fort des indices en 16..31, de poids faible en 0..15 ; indice k : 0 → +a, 1 → +b, 2 → −a, 3 → −b
                v |= (uint64_t)((idx[i] >> 1) & 1) << (16 + i);
                v |= (uint64_t)(idx[i] & 1) << i;
            }
            sortie = v;
        }
    }
    return sortie;
}

} // namespace

void encoderBloc(const uint8_t texels[16][4], uint8_t sortie[16]) {
    encoderAlpha(texels, sortie);
    ecrire64(encoderCouleur(texels), sortie + 8);
}

void decoderBloc(const uint8_t bloc[16], uint8_t texels[16][4]) {
    // alpha
    const int base = bloc[0], mult = bloc[1] >> 4, tb = bloc[1] & 15;
    uint64_t bits = 0;
    for (int k = 2; k < 8; k++) bits = (bits << 8) | bloc[k];
    for (int i = 0; i < 16; i++) texels[i][3] = (uint8_t)borne(base + EAC_TABLE[tb][(bits >> (45 - 3 * i)) & 7] * mult);
    // couleur (modes individuel et différentiel seulement : l'encodeur ne produit jamais T, H ni plan)
    const uint64_t v = lire64(bloc + 8);
    const int flip = (int)(v >> 32) & 1, diff = (int)(v >> 33) & 1;
    const int t0 = (int)(v >> 37) & 7, t1 = (int)(v >> 34) & 7;
    int c[2][3];
    for (int ch = 0; ch < 3; ch++) {
        if (diff) {
            const int a = (int)(v >> (59 - 8 * ch)) & 31;
            int d = (int)(v >> (56 - 8 * ch)) & 7; if (d >= 4) d -= 8;
            const int b = a + d;
            c[0][ch] = (a << 3) | (a >> 2); c[1][ch] = (b << 3) | (b >> 2);
        } else {
            c[0][ch] = ((int)(v >> (60 - 8 * ch)) & 15) * 17; c[1][ch] = ((int)(v >> (56 - 8 * ch)) & 15) * 17;
        }
    }
    for (int i = 0; i < 16; i++) {
        const int sb = sousBloc0(i, flip) ? 0 : 1;
        const int k = (int)(((v >> (16 + i)) & 1) << 1 | ((v >> i) & 1));
        const int m = modificateur(sb ? t1 : t0, k);
        for (int ch = 0; ch < 3; ch++) texels[i][ch] = (uint8_t)borne(c[sb][ch] + m);
    }
}

uint32_t lignesDeBlocs(uint32_t h) { return (h + 3) / 4; }
size_t octets(uint32_t w, uint32_t h) { return (size_t)((w + 3) / 4) * lignesDeBlocs(h) * 16; }

void encoderLignes(const uint8_t* rgba, uint32_t w, uint32_t h, uint8_t* sortie, uint32_t l0, uint32_t l1) {
    const uint32_t bx = (w + 3) / 4;
    uint8_t t[16][4];
    for (uint32_t j = l0; j < l1 && j < lignesDeBlocs(h); j++)
        for (uint32_t i = 0; i < bx; i++) {
            for (int p = 0; p < 16; p++) {   // texel p = x × 4 + y ; bords : dernier texel répété
                const uint32_t X = std::min(w - 1, i * 4 + p / 4), Y = std::min(h - 1, j * 4 + p % 4);
                std::memcpy(t[p], rgba + ((size_t)Y * w + X) * 4, 4);
            }
            encoderBloc(t, sortie + ((size_t)j * bx + i) * 16);
        }
}

void decoderImage(const uint8_t* blocs, uint32_t w, uint32_t h, uint8_t* rgba) {
    const uint32_t bx = (w + 3) / 4, by = lignesDeBlocs(h);
    uint8_t t[16][4];
    for (uint32_t j = 0; j < by; j++)
        for (uint32_t i = 0; i < bx; i++) {
            decoderBloc(blocs + ((size_t)j * bx + i) * 16, t);
            for (int p = 0; p < 16; p++) {
                const uint32_t X = i * 4 + p / 4, Y = j * 4 + p % 4;
                if (X < w && Y < h) std::memcpy(rgba + ((size_t)Y * w + X) * 4, t[p], 4);
            }
        }
}

} // namespace wfs::vehicule_etc2
