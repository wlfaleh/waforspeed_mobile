// WaForSpeed — WFS-483 : plaque 3D de la voiture du joueur (src/render/vehicule_plaque*.cpp), sans GPU.
// 1) RG-08 / RG-04 : chaque voiture du garage a ses deux places ; sur son vrai .wfm (assets/cars/), la plaque est dans la boîte de
//    la voiture (à l'épaisseur du porte-plaque près), sa normale sort de la caisse, les 64 jours du contour sont relevés de nouveau
//    par rayons et comparés au tableau, et entre deux points le pied du porte-plaque ne flotte pas à plus de 5 mm de la caisse ;
// 2) RG-01 : mesures du modèle (plaque 520 × 110 × 1 mm, porte-plaque 540 × 130 mm, 2 vis de 9 mm), triangles tournés vers
//    l'extérieur, aucun triangle dégénéré, version simple plus légère et sans vis ;
// 3) RG-02, RG-03, RG-06 : image de 1024 × 216 px, caractères de 75 mm, texte centré, 8 caractères à la même taille,
//    texte trop large réduit sans être étiré, aucune bande bleue (pas de bande européenne), relief dans la carte de normales.
#define STB_TRUETYPE_IMPLEMENTATION
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "../third_party/stb/stb_truetype.h"
#include "../src/game/cars.h"
#include "../src/render/vehicule_mesure.h"
#include "../src/render/vehicule_plaque.h"

using namespace wfs;
using namespace wfs::plaque_joueur;

static int g_echecs = 0;
#define VERIFIE(cond, ...) do { if (!(cond)) { g_echecs++; std::printf("ÉCHEC : "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

struct Tri { vec3 a, b, c; };

// rayon o + t d contre un triangle (deux faces) : t ou −1
static float rayon(const vec3& o, const vec3& d, const Tri& T) {
    const vec3 e1 = T.b - T.a, e2 = T.c - T.a, q = cross(d, e2);
    const float det = dot(e1, q);
    if (std::fabs(det) < 1e-12f) return -1;
    const vec3 s = o - T.a; const float u = dot(s, q) / det; if (u < 0 || u > 1) return -1;
    const vec3 r = cross(s, e1); const float v = dot(d, r) / det; if (v < 0 || u + v > 1) return -1;
    return dot(e2, r) / det;
}

static vec3 transforme(const mat4& m, const vec3& p) {
    return {m.at(0, 0) * p.x + m.at(0, 1) * p.y + m.at(0, 2) * p.z + m.at(0, 3), m.at(1, 0) * p.x + m.at(1, 1) * p.y + m.at(1, 2) * p.z + m.at(1, 3),
            m.at(2, 0) * p.x + m.at(2, 1) * p.y + m.at(2, 2) * p.z + m.at(2, 3)};
}

static void boite(const Maillage& m, vec3& mn, vec3& mx) {
    mn = {1e9f, 1e9f, 1e9f}; mx = {-1e9f, -1e9f, -1e9f};
    for (const Sommet& s : m.sommets) { mn = {std::min(mn.x, s.p.x), std::min(mn.y, s.p.y), std::min(mn.z, s.p.z)}; mx = {std::max(mx.x, s.p.x), std::max(mx.y, s.p.y), std::max(mx.z, s.p.z)}; }
}

// triangles tournés comme leurs normales de sommets, aucun triangle dégénéré, indices dans le maillage
static void verifierMaillage(const Maillage& m, const char* nom) {
    int envers = 0, degeneres = 0;
    for (size_t k = 0; k + 2 < m.indices.size(); k += 3) {
        const uint32_t a = m.indices[k], b = m.indices[k + 1], c = m.indices[k + 2];
        if (a >= m.sommets.size() || b >= m.sommets.size() || c >= m.sommets.size()) { VERIFIE(false, "%s : indice hors du maillage", nom); return; }
        const vec3 n = cross(m.sommets[b].p - m.sommets[a].p, m.sommets[c].p - m.sommets[a].p);
        if (length(n) < 1e-12f) { degeneres++; continue; }
        envers += dot(n, m.sommets[a].n + m.sommets[b].n + m.sommets[c].n) < 0;
    }
    VERIFIE(envers == 0, "%s : %d triangle(s) tourné(s) vers l'intérieur", nom, envers);
    VERIFIE(degeneres <= (int)m.indices.size() / 3 / 50, "%s : %d triangle(s) dégénéré(s)", nom, degeneres);
}

int main(int argc, char** argv) {
    const std::string racine = argc > 1 ? argv[1] : WFS_SOURCE_DIR;
    // ---------- 1. places sur les vraies voitures ----------
    int voitures = 0;
    float pireEcart = 0, pireContour = 0;
    int pointsContour = 0, horsContour = 0;
    for (int i = 0; i < CAR_COUNT; i++) {
        const CarDef& c = CARS[i];
        if (voitureRetiree(c.id)) continue;   // voitures du garage (les voitures permises)
        const PlacesVoiture* pv = places(c.id);
        VERIFIE(pv != nullptr, "RG-08 : %s sans place de plaque", c.id);
        if (!pv) continue;
        vehicule::ModeleWfm w; std::string err;
        if (!vehicule::lireWfm(racine + "/assets/" + c.file, w, err)) { VERIFIE(false, "%s : %s", c.file, err.c_str()); continue; }
        std::vector<Tri> opaques;
        for (size_t k = 0; k < w.sommets.size(); k++) {
            if (w.drapeaux[k] & (2 | 64)) continue;   // vitres, pièces transparentes
            const float* v = w.sommets[k].data(); const auto& I = w.indices[k];
            for (size_t t = 0; t + 2 < I.size(); t += 3) {
                if (I[t] == I[t + 1] || I[t + 1] == I[t + 2]) continue;
                opaques.push_back({{v[8 * I[t]], v[8 * I[t] + 1], v[8 * I[t] + 2]}, {v[8 * I[t + 1]], v[8 * I[t + 1] + 1], v[8 * I[t + 1] + 2]},
                                   {v[8 * I[t + 2]], v[8 * I[t + 2] + 1], v[8 * I[t + 2] + 2]}});
            }
        }
        voitures++;
        for (int cote = 0; cote < 2; cote++) {
            const plaque_joueur::Pose& p = cote ? pv->arriere : pv->avant;
            const char* nc = cote ? "arrière" : "avant";
            const vec3 n = normalize(p.normale);
            VERIFIE(cote ? n.z > 0.85f : n.z < -0.85f, "%s %s : normale (%.2f, %.2f, %.2f) ne sort pas de la caisse", c.id, nc, n.x, n.y, n.z);
            // dans la boîte de la voiture, à l'épaisseur du porte-plaque près (8 mm devant la plaque d'origine)
            const float tol = 0.010f;
            const Pieces P = construire(false, &p);
            const mat4 M = repere(p);
            int dehors = 0;
            for (const Sommet& s : P.support.sommets) {
                const vec3 q = transforme(M, s.p);
                dehors += q.x < w.boite[0] - tol || q.y < w.boite[1] - tol || q.z < w.boite[2] - tol || q.x > w.boite[3] + tol || q.y > w.boite[4] + tol || q.z > w.boite[5] + tol;
            }
            VERIFIE(dehors == 0, "RG-08 : %s %s : %d point(s) du porte-plaque hors de la boîte de la voiture", c.id, nc, dehors);
            // pied de la jupe du porte-plaque : (a) aux 64 points du tableau, jours relevés de nouveau (point et 4 voisins à ± 2 mm :
            // une arête vive sous le point, l'arrondi du tableau au dixième de mm suffit à changer la pièce touchée) ; (b) entre deux
            // points du tableau, la jupe ne flotte pas à plus de 5 mm de la caisse (≤ 5 % des points tolérés : trou de calandre)
            auto mesurer = [&](const vec3& qmm) {
                const vec3 o = transforme(M, qmm * 0.001f), d = n * -1.f;
                float t = 1e9f;
                for (const Tri& T : opaques) { const float h = rayon(o, d, T); if (h > -0.002f && h < t) t = h; }
                return std::clamp(t * 1000.f, 0.f, JOUR_MAX_MM);
            };
            for (int k = 0; k < JOURS; k++) {
                const vec3 q = pointContour((float)k / JOURS);
                float bas = 1e9f, haut = -1e9f;
                for (const vec3 dp : {vec3{0, 0, 0}, vec3{2, 0, 0}, vec3{-2, 0, 0}, vec3{0, 2, 0}, vec3{0, -2, 0}}) {
                    const float m = mesurer(q + dp); bas = std::min(bas, m); haut = std::max(haut, m);
                }
                const float table = p.jours[k], ecart = table < bas ? bas - table : table > haut ? table - haut : 0.f;
                pireEcart = std::max(pireEcart, ecart);
                VERIFIE(ecart <= 1.0f, "%s %s : jour au point %d (%.0f, %.0f) mm : %.1f à %.1f mm mesuré, %.1f mm au tableau (relancer tools/plaques_joueur.mjs)",
                        c.id, nc, k, q.x, q.y, bas, haut, table);
            }
            // (b) sommets du pied de la jupe du maillage dessiné (complet) : sur le contour extérieur, en arrière du fond (z ≤ 0)
            {
                const Pieces C = construire(true, &p);
                int horsIci = 0, ici = 0;
                for (const Sommet& sv : C.support.sommets) {
                    const vec3 q = sv.p * 1000.f;
                    const float ax = std::fabs(q.x) - (SUPPORT_L_MM / 2 - SUPPORT_RAYON_MM), ay = std::fabs(q.y) - (SUPPORT_H_MM / 2 - SUPPORT_RAYON_MM);
                    const float dBord = std::sqrt(std::max(ax, 0.f) * std::max(ax, 0.f) + std::max(ay, 0.f) * std::max(ay, 0.f)) + std::min(std::max(ax, ay), 0.f) - SUPPORT_RAYON_MM;
                    if (std::fabs(dBord) > 0.05f || q.z > 0.01f || sv.n.z < -0.5f) continue;   // pied du flanc seulement (pas le dos)
                    if (-q.z >= JOUR_MAX_MM - 0.5f) continue;                                   // calandre ouverte : rien derrière
                    const float flotte = mesurer(vec3{q.x, q.y, 0}) + q.z;   // > 0 : la caisse est plus loin que le pied (jour visible)
                    pointsContour++; horsContour += flotte > 5.f; pireContour = std::max(pireContour, flotte);
                    ici++; horsIci += flotte > 5.f;
                }
                if (horsIci) std::printf("  %s %s : %d sommet(s) du pied sur %d à plus de 5 mm de la caisse (trou de calandre, arête vive)\n", c.id, nc, horsIci, ici);
            }
        }
    }
    std::printf("pied de la jupe (maillage dessiné) : jour ≤ %.1f mm sur %d sommets (%d au-delà de 5 mm)\n", pireContour, pointsContour, horsContour);
    VERIFIE(horsContour * 20 <= pointsContour, "RG-08 : %d point(s) du contour sur %d à plus de 5 mm de la caisse", horsContour, pointsContour);
    VERIFIE(voitures == voituresPermises().n, "voitures du garage contrôlées : %d", voitures);
    std::printf("places : %d voitures × 2 plaques, écart au tableau ≤ %.1f mm\n", voitures, pireEcart);

    // ---------- 2. mesures du modèle ----------
    {
        const Pieces P = construire(true, nullptr), S = construire(false, nullptr);
        vec3 mn, mx;
        boite(P.plaque, mn, mx);
        VERIFIE(std::fabs((mx.x - mn.x) * 1000 - PLAQUE_L_MM) < 0.01f * PLAQUE_L_MM && std::fabs((mx.y - mn.y) * 1000 - PLAQUE_H_MM) < 0.01f * PLAQUE_H_MM,
                "RG-01 : plaque de %.1f × %.1f mm", (mx.x - mn.x) * 1000, (mx.y - mn.y) * 1000);
        VERIFIE(std::fabs((mx.z - mn.z) * 1000 - PLAQUE_EP_MM) < 0.05f, "RG-01 : tôle de %.2f mm", (mx.z - mn.z) * 1000);
        const float avantPlaque = mx.z;
        boite(P.support, mn, mx);
        VERIFIE(std::fabs((mx.x - mn.x) * 1000 - SUPPORT_L_MM) < 0.5f && std::fabs((mx.y - mn.y) * 1000 - SUPPORT_H_MM) < 0.5f,
                "porte-plaque de %.1f × %.1f mm", (mx.x - mn.x) * 1000, (mx.y - mn.y) * 1000);
        VERIFIE(mx.z > avantPlaque + 0.003f, "le cadre du porte-plaque doit dépasser la plaque (%.1f mm)", (mx.z - avantPlaque) * 1000);
        boite(P.vis, mn, mx);
        VERIFIE(P.vis.sommets.size() > 200 && mn.z >= avantPlaque - 1e-5f && std::fabs((mx.y - mn.y) * 1000 - VIS_D_MM) < 0.3f, "vis : %.1f mm de diamètre", (mx.y - mn.y) * 1000);
        VERIFIE(S.vis.sommets.empty() && S.support.sommets.size() < P.support.sommets.size() && S.plaque.indices.size() < P.plaque.indices.size(),
                "version simple : sans vis et plus légère");
        verifierMaillage(P.plaque, "plaque"); verifierMaillage(P.support, "porte-plaque"); verifierMaillage(P.vis, "vis");
        verifierMaillage(S.plaque, "plaque simple"); verifierMaillage(S.support, "porte-plaque simple");
        // uv de la face avant : u de gauche à droite, v de haut en bas (texte lisible, jamais en miroir)
        float uG = 2, uD = -1, vH = 2;
        for (const Sommet& s : P.plaque.sommets) if (s.n.z > 0.99f && s.u > 0) {
            if (s.p.x < -0.25f) uG = std::min(uG, s.u);
            if (s.p.x > 0.25f) uD = std::max(uD, s.u);
            if (s.p.y > 0.05f) vH = std::min(vH, s.v);
        }
        VERIFIE(uG < 0.05f && uD > 0.95f && vH < 0.1f, "uv de la plaque : gauche %.2f, droite %.2f, haut %.2f", uG, uD, vH);
        std::printf("modèle : plaque %zu triangles, porte-plaque %zu, vis %zu ; simple : %zu + %zu\n", P.plaque.indices.size() / 3,
                    P.support.indices.size() / 3, P.vis.indices.size() / 3, S.plaque.indices.size() / 3, S.support.indices.size() / 3);
    }

    // ---------- 3. image ----------
    {
        std::ifstream f(racine + "/assets/fonts/Rajdhani-Bold.ttf", std::ios::binary);
        const std::vector<uint8_t> ttf((std::istreambuf_iterator<char>(f)), {});
        VERIFIE(!ttf.empty(), "police introuvable");
        ImagePlaque a, b, c, d;
        VERIFIE(dessinerImage("ab-123", ttf.data(), a) && a.caracteres == 6, "texte de 6 caractères (minuscules mises en majuscules)");
        VERIFIE(IMAGE_L >= 512 && IMAGE_H >= 108 && a.couleur.size() == (size_t)IMAGE_L * IMAGE_H * 4 && a.normales.size() == a.couleur.size(), "RG-06 : image ≥ 512 × 108");
        VERIFIE(std::fabs(a.hauteurTexteMm - CARACTERE_H_MM) < 0.01f, "RG-03 : caractères de %.1f mm", a.hauteurTexteMm);
        VERIFIE(dessinerImage("WFS-2026", ttf.data(), b) && std::fabs(b.hauteurTexteMm - CARACTERE_H_MM) < 0.01f, "8 caractères à %.1f mm (75 attendus)", b.hauteurTexteMm);
        VERIFIE(dessinerImage("WWWWWWWW", ttf.data(), c) && c.hauteurTexteMm < CARACTERE_H_MM && c.hauteurTexteMm > 40.f, "texte très large : %.1f mm", c.hauteurTexteMm);
        const float zone = PLAQUE_L_MM - ZONE_TEXTE_G_MM - 6.5f - BANDE_L_MM - ZONE_TEXTE_D_MM;
        VERIFIE(c.largeurTexteMm <= zone + 0.5f, "texte très large : %.0f mm pour %.0f mm de zone", c.largeurTexteMm, zone);
        // 1 caractère centré dans la zone du texte (centre des pixels d'encre, hors liseré et bande)
        VERIFIE(dessinerImage("7", ttf.data(), d), "1 caractère");
        const float pxmm = IMAGE_L / PLAQUE_L_MM;
        double sx = 0, n = 0;
        int bleus = 0;
        for (int y = 0; y < IMAGE_H; y++)
            for (int x = 0; x < IMAGE_L; x++) {
                const uint8_t* p = &d.couleur[((size_t)y * IMAGE_L + x) * 4];
                bleus += p[2] > p[0] + 40;   // RG-02 : aucune bande bleue européenne
                const float xm = x / pxmm, ym = y / pxmm;
                if (xm > 12 && xm < PLAQUE_L_MM - 6.5f - BANDE_L_MM - 2 && ym > 12 && ym < PLAQUE_H_MM - 12 && p[0] < 80) { sx += xm; n++; }
            }
        const float centre = ZONE_TEXTE_G_MM + zone / 2;
        VERIFIE(n > 100 && std::fabs(sx / n - centre) < 6.f, "RG-03 : caractère seul à %.1f mm du bord (centre de la zone : %.1f mm)", n ? sx / n : -1, centre);
        VERIFIE(bleus == 0, "RG-02 : %d pixel(s) bleu(s) (bande européenne interdite)", bleus);
        // relief : des normales penchées sur les bords des caractères et du bourrelet (au moins 1 % des pixels)
        int penchees = 0;
        for (size_t i = 0; i < a.normales.size(); i += 4) penchees += a.normales[i + 2] < 240;
        VERIFIE(penchees > (int)(a.normales.size() / 4 / 100), "relief des caractères absent (%d pixels)", penchees);
        std::printf("image : « AB-123 » %.0f × %.0f mm, « WFS-2026 » %.0f mm de large, « WWWWWWWW » réduit à %.1f mm de haut\n",
                    a.largeurTexteMm, a.hauteurTexteMm, b.largeurTexteMm, c.hauteurTexteMm);
    }
    if (g_echecs) { std::printf("test_plaque_joueur : %d échec(s)\n", g_echecs); return 1; }
    std::printf("test_plaque_joueur : OK\n");
    return 0;
}
