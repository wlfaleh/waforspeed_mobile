// WaForSpeed — WFS-613 : lumières de nuit limitées par tuile d'écran.
//
// Pourquoi : la nuit, chaque pixel parcourait les 12 lumières de l'image (phares, lampadaires), même celles trop loin pour
// l'éclairer. Ici, une fois par image, le processeur projette la sphère de portée de chaque lumière avec la caméra principale
// et note, pour chaque tuile d'une grille de TX × TY tuiles de l'écran, les lumières dont la sphère peut toucher un point vu
// dans cette tuile (un bit par lumière). Le shader (eclairage.glsl, masqueLumieres) projette le point éclairé avec la même
// matrice, lit le masque de sa tuile et saute les autres lumières.
//
// Règle : le masque est CONSERVATEUR. Un point p à portée de la lumière i est dans sa sphère, donc dans la boîte qui l'entoure ;
// devant la caméra, la projection de cette boîte est l'enveloppe de ses 8 coins projetés : la tuile de p a toujours le bit i.
// Une boîte qui passe derrière la caméra est d'abord coupée au plan w = W_MIN (plus près que le plan proche : aucun point vu n'est
// avant) : sa projection reste l'enveloppe de ses sommets gardés et des points de coupe de ses arêtes.
// Une lumière sautée est donc une lumière que le shader aurait de toute façon écartée (« hors de portée ») : même image. Lumière
// de portée infinie (0) : toutes les tuiles. Point hors de l'écran ou derrière la caméra
// (reflet, rétroviseur, capture d'environnement) : le shader garde toutes les lumières.
// Fichier pur (aucun Vulkan) : testé sur le processeur (test_tuiles_lumieres).
#pragma once
#include <algorithm>
#include <cstdint>
#include "../core/math.h"

namespace tuiles_lumieres {

constexpr int TX = 32, TY = 16;               // grille de l'écran (75 × 68 pixels par tuile en 2400 × 1080)
constexpr int MOTS = TX * TY / 2;              // 2 tuiles de 16 bits par mot de 32 bits (256 mots = 64 uvec4 dans le bloc Frame)
constexpr int LUMIERES_MAX = 12;               // taille des tableaux spotPos… de common.glsl
constexpr uint32_t TOUTES = (1u << LUMIERES_MAX) - 1u;
constexpr float W_MIN = 0.05f;              // plan de coupe de la boîte : sous le plan proche de la caméra (0,1 m)
constexpr float MARGE = 1.0f / 512.0f;         // marge de la boîte projetée (unités NDC) : arrondis du flottant, jamais de bord perdu

// tuile (indice) d'un point déjà projeté en coordonnées normalisées (-1 … 1) ; même calcul que masqueLumieres (eclairage.glsl)
inline int tuile(float nx, float ny) {
    const int tx = std::clamp((int)((nx * 0.5f + 0.5f) * TX), 0, TX - 1), ty = std::clamp((int)((ny * 0.5f + 0.5f) * TY), 0, TY - 1);
    return ty * TX + tx;
}

inline void poser(uint32_t* mots, int t, uint32_t bits) { mots[t >> 1] |= bits << ((t & 1) * 16); }
inline uint32_t lire(const uint32_t* mots, int t) { return (mots[t >> 1] >> ((t & 1) * 16)) & 0xFFFFu; }

// mots : MOTS entiers ; pos[i] = (x, y, z, portée en m, 0 = infinie) comme spotPos ; n lumières (≤ LUMIERES_MAX)
inline void calculer(const wfs::mat4& vp, const wfs::vec4* pos, int n, uint32_t* mots) {
    for (int k = 0; k < MOTS; k++) mots[k] = 0;
    n = std::clamp(n, 0, LUMIERES_MAX);
    for (int i = 0; i < n; i++) {
        const uint32_t bit = 1u << i;
        const float r = pos[i].w;
        const bool partout = !(r > 0.f);
        if (partout) { for (int t = 0; t < TX * TY; t++) poser(mots, t, bit); continue; }
        // coins de la boîte en coordonnées de découpage (x, y, w) ; un point vu est toujours au-delà du plan proche (w ≥ zNear, 0,1 m) :
        // la boîte est coupée au plan w = W_MIN (sous zNear) : sommets gardés (w ≥ W_MIN) et points où ses 12 arêtes traversent ce plan
        float cx[8], cy[8], cw[8];
        for (int c = 0; c < 8; c++) {
            const float px = pos[i].x + ((c & 1) ? r : -r), py = pos[i].y + ((c & 2) ? r : -r), pz = pos[i].z + ((c & 4) ? r : -r);
            cx[c] = vp.at(0, 0) * px + vp.at(0, 1) * py + vp.at(0, 2) * pz + vp.at(0, 3);
            cy[c] = vp.at(1, 0) * px + vp.at(1, 1) * py + vp.at(1, 2) * pz + vp.at(1, 3);
            cw[c] = vp.at(3, 0) * px + vp.at(3, 1) * py + vp.at(3, 2) * pz + vp.at(3, 3);
        }
        float x0 = 1e30f, y0 = 1e30f, x1 = -1e30f, y1 = -1e30f;
        bool vu = false;
        auto ajouter = [&](float x, float y, float w) { vu = true; x0 = std::min(x0, x / w); x1 = std::max(x1, x / w); y0 = std::min(y0, y / w); y1 = std::max(y1, y / w); };
        for (int c = 0; c < 8; c++) if (cw[c] >= W_MIN) ajouter(cx[c], cy[c], cw[c]);
        for (int c = 0; c < 8; c++) for (int axe = 1; axe <= 4; axe <<= 1) {   // arêtes c → c + axe (12 en tout)
            if (c & axe) continue;
            const int d = c | axe;
            if ((cw[c] >= W_MIN) == (cw[d] >= W_MIN)) continue;
            const float t = (W_MIN - cw[c]) / (cw[d] - cw[c]);
            ajouter(cx[c] + (cx[d] - cx[c]) * t, cy[c] + (cy[d] - cy[c]) * t, W_MIN);
        }
        if (!vu) continue;   // boîte entière derrière le plan proche : aucun point vu n'est à portée
        x0 -= MARGE; y0 -= MARGE; x1 += MARGE; y1 += MARGE;
        if (x1 < -1.f || y1 < -1.f || x0 > 1.f || y0 > 1.f) continue;   // sphère hors de l'écran : aucun point vu n'est à portée
        const int a = tuile(x0, y0), b = tuile(x1, y1);
        for (int ty = a / TX; ty <= b / TX; ty++)
            for (int tx = a % TX; tx <= b % TX; tx++) poser(mots, ty * TX + tx, bit);
    }
}

}  // namespace tuiles_lumieres
