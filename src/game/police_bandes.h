// WaForSpeed — WFS-460 (RG-01, RG-02, ANO-250) : bandes « POLICE » posées sur la vraie tôle des portières de la voiture de police.
// Avant : un quadrilatère plat de chaque côté d'un cube unité, mis à l'échelle de la boîte de la voiture : il ne suivait pas
// la caisse, dépassait au-dessus de la roue avant, et le côté droit (+x) n'était jamais dessiné (ses deux triangles étaient
// enroulés vers l'intérieur : face arrière retirée par le rendu).
// Ici : une grille de points (le long de la voiture × la hauteur de la bande) est projetée sur les triangles de la peinture
// du modèle, de l'extérieur vers l'axe de la voiture (comme un décalque appliqué à la main), puis décalée de quelques
// millimètres le long de la normale de la tôle. La bande épouse le galbe des portières (rentrée de caisse, nervures), va de
// l'arrière du passage de roue avant au début du passage de roue arrière, et « POLICE » se lit à l'endroit des deux côtés.
// Calcul seul, sur le processeur, sans GPU ni allocation par image (une fois au chargement du modèle) : testé par
// tests/test_police_bandes.cpp. Branchement : poursuite_system.cpp (un seul dessin par voiture de police, comme avant).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>
#include "../core/math.h"

namespace wfs::police_bandes {

// ---- réglages (RG-02 : mesures réelles de la livrée, sans marque ni logo officiel)
constexpr float HAUTEUR_M = 0.18f;          // hauteur de la bande : 15 à 20 cm (RG-02)
constexpr float CENTRE_HAUTEUR = 0.44f;     // centre de la bande au-dessus du sol, en part de la hauteur de la voiture (≈ 0,63 m
                                            // sur la 208 de 1,43 m : milieu des portières, entre le bas de caisse et la ceinture de caisse)
constexpr float MARGE_PASSAGE_M = 0.12f;    // tôle de l'aile autour du pneu : la bande commence 12 cm après le pneu avant,
                                            // finit 12 cm avant le pneu arrière (bord du passage de roue, ± 2 cm sur la 208)
constexpr float DECALAGE_M = 0.005f;        // 5 mm au-dessus de la tôle (épaisseur d'un film adhésif posé, aucun z-fighting)
constexpr int COLONNES = 28;                // points le long de la bande (≈ 6 cm entre deux sur 1,7 m : galbe suivi)
constexpr int RANGEES = 5;                  // points dans la hauteur (≈ 4,5 cm entre deux : rentrée de caisse suivie)
constexpr float CREUX_MAX_M = 0.025f;       // un point plus de 2,5 cm en retrait de ses voisins est tombé dans un jour entre
                                            // deux panneaux (ligne de porte) : remplacé par l'interpolation de ses voisins

struct Triangle { vec3 a, b, c; };
struct Sommet { vec3 p, n; float u, v; };
struct Bandes {
    std::vector<Sommet> sommets;            // deux côtés : d'abord −x (gauche), puis +x (droite)
    std::vector<uint32_t> indices;
    float zAvant = 0, zArriere = 0, yBas = 0, yHaut = 0;   // place de la bande dans le repère du modèle (avant vers −z)
    int manques = 0;                        // points sans tôle trouvée (remplis par leurs voisins) ; −1 : pas de bande
    int creux = 0;                          // points tombés dans un jour entre deux panneaux (corrigés)
    bool ok() const { return !indices.empty(); }
};

// rayon horizontal (y, z fixés) qui part de x = depart vers l'axe de la voiture (sens −cote) : premier triangle touché.
// Rend vrai et la position x et la normale géométrique (tournée vers l'extérieur) du point de tôle le plus extérieur.
inline bool toucher(const std::vector<Triangle>& tris, float y, float z, int cote, float& x, vec3& n) {
    float meilleur = -1e9f; bool trouve = false;
    for (const Triangle& t : tris) {
        // coordonnées barycentriques dans le plan (z, y) : le rayon est parallèle à x
        const float d = (t.b.z - t.a.z) * (t.c.y - t.a.y) - (t.c.z - t.a.z) * (t.b.y - t.a.y);
        if (std::fabs(d) < 1e-10f) continue;   // triangle vu par la tranche (parallèle au rayon)
        const float l1 = ((z - t.a.z) * (t.c.y - t.a.y) - (t.c.z - t.a.z) * (y - t.a.y)) / d;
        const float l2 = ((t.b.z - t.a.z) * (y - t.a.y) - (z - t.a.z) * (t.b.y - t.a.y)) / d;
        if (l1 < -1e-5f || l2 < -1e-5f || l1 + l2 > 1 + 1e-5f) continue;
        const float xi = t.a.x + l1 * (t.b.x - t.a.x) + l2 * (t.c.x - t.a.x);
        if (xi * cote <= meilleur) continue;
        vec3 g = cross(t.b - t.a, t.c - t.a);
        const float lg = length(g);
        if (lg < 1e-12f) continue;
        g = g * (1.f / lg);
        if (g.x * cote < 0) g = g * -1.f;   // normale vers l'extérieur du côté visé (faces doubles de la peinture)
        meilleur = xi * cote; x = xi; n = g; trouve = true;
    }
    return trouve;
}

// Construit les deux bandes. tris : triangles de la peinture de la caisse (repère du modèle, m, avant vers −z) ;
// zPneuAvant : bord arrière du pneu avant (centre + rayon) ; zPneuArriere : bord avant du pneu arrière (centre − rayon) ;
// sol, hauteur : bas des pneus et hauteur de la voiture (m).
inline Bandes construire(const std::vector<Triangle>& tous, float zPneuAvant, float zPneuArriere, float sol, float hauteur) {
    Bandes b;
    b.zAvant = zPneuAvant + MARGE_PASSAGE_M;
    b.zArriere = zPneuArriere - MARGE_PASSAGE_M;
    const float yc = sol + CENTRE_HAUTEUR * hauteur;
    b.yBas = yc - HAUTEUR_M / 2; b.yHaut = yc + HAUTEUR_M / 2;
    if (b.zArriere - b.zAvant < 0.5f || tous.empty()) { b.manques = -1; return b; }   // voiture sans portière mesurable : rien
    // seuls les triangles qui croisent la boîte de la bande comptent (≈ 2 % de la caisse : chargement rapide)
    std::vector<Triangle> tris;
    for (const Triangle& t : tous) {
        const float z0 = std::min({t.a.z, t.b.z, t.c.z}), z1 = std::max({t.a.z, t.b.z, t.c.z});
        const float y0 = std::min({t.a.y, t.b.y, t.c.y}), y1 = std::max({t.a.y, t.b.y, t.c.y});
        if (z1 < b.zAvant - 0.01f || z0 > b.zArriere + 0.01f || y1 < b.yBas - 0.01f || y0 > b.yHaut + 0.01f) continue;
        tris.push_back(t);
    }
    for (int cote = -1; cote <= 1; cote += 2) {
        float X[RANGEES][COLONNES]; vec3 N[RANGEES][COLONNES]; bool ok[RANGEES][COLONNES];
        for (int j = 0; j < RANGEES; j++) for (int i = 0; i < COLONNES; i++) {
            const float z = b.zAvant + (b.zArriere - b.zAvant) * (float)i / (COLONNES - 1);
            const float y = b.yBas + (b.yHaut - b.yBas) * (float)j / (RANGEES - 1);
            ok[j][i] = toucher(tris, y, z, cote, X[j][i], N[j][i]);
        }
        // points sans tôle (trou du modèle) ou tombés dans le jour d'une porte (plus de 2,5 cm en retrait de la médiane de leurs
        // 4 voisins de rangée) : remplacés par l'interpolation des points justes les plus proches de la rangée, de chaque côté
        for (int j = 0; j < RANGEES; j++) {
            bool juste[COLONNES];
            for (int i = 0; i < COLONNES; i++) {
                juste[i] = ok[j][i];
                if (!ok[j][i]) { b.manques++; continue; }
                float vois[4]; int nv = 0;
                for (int k = i - 2; k <= i + 2; k++) if (k != i && k >= 0 && k < COLONNES && ok[j][k]) vois[nv++] = X[j][k] * cote;
                if (nv >= 2) {
                    for (int a = 1; a < nv; a++) for (int c = a; c > 0 && vois[c - 1] > vois[c]; c--) std::swap(vois[c - 1], vois[c]);   // 4 valeurs au plus
                    const float med = nv % 2 ? vois[nv / 2] : 0.5f * (vois[nv / 2 - 1] + vois[nv / 2]);
                    if (X[j][i] * cote < med - CREUX_MAX_M) { juste[i] = false; b.creux++; }
                }
            }
            for (int i = 0; i < COLONNES; i++) {
                if (juste[i]) continue;
                int g = i - 1, d = i + 1;
                while (g >= 0 && !juste[g]) g--;
                while (d < COLONNES && !juste[d]) d++;
                if (g < 0 && d >= COLONNES) { ok[j][i] = false; continue; }   // rangée sans aucun point juste
                if (g < 0) { X[j][i] = X[j][d]; N[j][i] = N[j][d]; }
                else if (d >= COLONNES) { X[j][i] = X[j][g]; N[j][i] = N[j][g]; }
                else {
                    const float a = (float)(i - g) / (float)(d - g);
                    X[j][i] = X[j][g] + (X[j][d] - X[j][g]) * a; N[j][i] = normalize(N[j][g] * (1 - a) + N[j][d] * a);
                }
                ok[j][i] = true;
            }
        }
        // rangée entière sans tôle : la bande de ce côté n'est pas posée (jamais un morceau qui flotte)
        bool complet = true;
        for (int j = 0; j < RANGEES; j++) for (int i = 0; i < COLONNES; i++) complet = complet && ok[j][i];
        if (!complet) continue;
        const uint32_t base = (uint32_t)b.sommets.size();
        for (int j = 0; j < RANGEES; j++) for (int i = 0; i < COLONNES; i++) {
            const float z = b.zAvant + (b.zArriere - b.zAvant) * (float)i / (COLONNES - 1);
            const float y = b.yBas + (b.yHaut - b.yBas) * (float)j / (RANGEES - 1);
            // normale lissée (moyenne avec les voisins) : la lumière glisse sur la bande comme sur la tôle, sans facettes
            vec3 n = N[j][i];
            for (int dj = -1; dj <= 1; dj++) for (int di = -1; di <= 1; di++) {
                const int jj = j + dj, ii = i + di;
                if ((dj || di) && jj >= 0 && jj < RANGEES && ii >= 0 && ii < COLONNES) n = n + N[jj][ii] * 0.5f;
            }
            n = normalize(n);
            const float a = (float)i / (COLONNES - 1);   // 0 à l'avant, 1 à l'arrière
            // à l'endroit des deux côtés : côté droit (+x), on lit de l'arrière vers l'avant ; côté gauche, de l'avant vers l'arrière
            const float u = cote > 0 ? 1.f - a : a;
            const float v = 1.f - (float)j / (RANGEES - 1);   // haut de la texture en haut de la bande
            b.sommets.push_back({vec3{X[j][i], y, z} + n * DECALAGE_M, n, u, v});
        }
        for (int j = 0; j + 1 < RANGEES; j++) for (int i = 0; i + 1 < COLONNES; i++) {
            const uint32_t p00 = base + (uint32_t)(j * COLONNES + i), p01 = p00 + 1, p10 = p00 + COLONNES, p11 = p10 + 1;
            // enroulement : normale géométrique vers l'extérieur (sens des faces avant du moteur, comme la caisse)
            if (cote > 0) b.indices.insert(b.indices.end(), {p00, p10, p11, p00, p11, p01});
            else b.indices.insert(b.indices.end(), {p00, p01, p11, p00, p11, p10});
        }
    }
    if (b.indices.empty()) b.manques = -1;
    return b;
}

} // namespace wfs::police_bandes
