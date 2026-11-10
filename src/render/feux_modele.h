// WaForSpeed — feux arrière des modèles 3D (WFS-109, épic E4).
//   - RG-13 : découpe des feux arrière au chargement. Certains modèles (Golf, 911…) n'ont qu'un seul matériau de feux,
//     émissif blanc, pour l'avant et l'arrière : leurs triangles situés au-delà de z = 0,4 L (arrière vers +z) forment une
//     pièce à part, marquée feu arrière, dessinée comme un feu MAT_TAIL (rouge, allumé au freinage, de jour comme de nuit).
//   - RG-03 : 3e feu stop (petite pièce MAT_TAIL centrée, en haut de la lunette) : allumé au freinage seulement.
//   - RG-02 : transition douce (≈ 80 ms) entre feu éteint / de position et feu stop.
// Calcul CPU seul, sans Vulkan (testé par tests/test_feux_modele.cpp) ; le banc d'essai des feux est dans feux_modele.cpp.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include "../core/math.h"

namespace wfs {
class Game; class Renderer;
// WFS-109 (RG-15) : captures feux-frein-jour, feux-frein-nuit, feux-recul (voiture du joueur, caméra arrière)
void benchFeuxFrein(Game& g, Renderer& r, const std::string& out);
// WFS-170 (RG-04, RG-08) : vitrine-<id>-feux des 20 voitures : vue arrière de trois quarts, frein tenu (feux stop allumés),
// de jour. WFS_VOITURES="id id …" (essai local) : ces voitures seulement, avec aussi leur vitrine-<id>-0.
void benchVitrinesFeux(Game& g, Renderer& r, const std::string& out);
// WFS-413 : optiques avant de nuit : phares-planche (voitures de face), phares-gros-plan-<id>, phares-joueur-face, phares-trafic-face, phares-jour-golf
void benchPharesOptiques(Game& g, Renderer& r, const std::string& out);
}

namespace wfs::feux_modele {

// réglages (RG-06)
constexpr float SEUIL_Z = 0.4f;          // RG-13 : triangles au-delà de 0,4 L (derrière l'essieu arrière, ≈ 0,3 L)
constexpr float HAUTEUR_MAX = 0.5f;      // RG-13 : pas de découpe si la pièce fait plus de 0,5 m de haut (lampAt du web)
constexpr float TRANSITION_S = 0.08f;    // RG-02 : 80 ms entre feu de position et feu stop
constexpr float TROISIEME_LARGEUR = 0.6f, TROISIEME_CENTRE = 0.1f;   // RG-03 : 3e feu stop = pièce étroite et centrée

constexpr uint32_t DRAPEAU_TAIL = 4, DRAPEAU_HEAD = 8;   // MAT_TAIL / MAT_HEAD (renderer.h, format .wfm)
enum : int8_t { PIECE_NORMALE = 0, PIECE_FEU_ARRIERE = 1, PIECE_TROISIEME_FEU = 2, PIECE_PHARE = 3 };   // WFS-413 : 3 = optique avant
inline bool pieceFeuArriere(int8_t p) { return p == PIECE_FEU_ARRIERE || p == PIECE_TROISIEME_FEU; }

// ---------- WFS-413 : optiques avant mesurées sur le modèle (ANO-240) ----------
// Une optique = les pièces de la lampe (réflecteur, projecteur, DEL, bandeau) et sa glace, rangées dans une boîte par côté
// (|x| entre xMin et xMax : la même boîte en miroir à gauche et à droite). Zones données voiture par voiture
// (correctifs_voitures.h, RG-04), repérées sur le modèle vu de face : on ne devine plus le phare à partir de la boîte de la caisse.
struct ZoneOptique { uint32_t materiau; bool glace; float xMin, xMax, yMin, yMax, zMax; };
// centre : centre de la glace (moyenne de la surface vue de face, pondérée par l'aire projetée, sur la glace ; sur la lampe si
// le modèle n'a pas de glace), au ras de sa surface extérieure (rayon venu de l'avant) ; normale : normale moyenne de la glace,
// tournée vers l'extérieur ; largeur, hauteur : boîte de la lampe. Repère du modèle : avant vers −z, x à droite.
struct Optique { vec3 centre{}, normale{0, 0, -1}; float largeur = 0, hauteur = 0; uint32_t triangles = 0; bool trouvee = false; };
constexpr float DERRIERE_GLACE = 0.01f;   // RG-02 : la lumière part 1 cm derrière la glace (dans l'optique)

// le triangle de centre c est-il dans la zone, du côté cote (−1 gauche, +1 droite) ?
inline bool dansZone(const ZoneOptique& z, const vec3& c, int cote) {
    const float x = c.x * (float)cote;
    return x >= z.xMin && x <= z.xMax && c.y >= z.yMin && c.y <= z.yMax && c.z <= z.zMax;
}

// RG-01 : les deux optiques avant (0 : gauche, x < 0 ; 1 : droite). Part : material, firstIndex, indexCount, vertexOffset ;
// pos(v) : position du sommet v. Toutes les pièces d'un matériau sont lues (découpées ou non).
template <class Part, class Pos>
void mesurerOptiques(const std::vector<uint32_t>& idx, const std::vector<Part>& parts, Pos pos, const std::vector<ZoneOptique>& zones,
                     Optique out[2]) {
    for (int s = 0; s < 2; s++) {
        const int cote = s ? 1 : -1;
        Optique o;
        std::vector<vec3> lampe, glace;   // triangles (3 sommets à la suite)
        for (const Part& p : parts)
            for (const ZoneOptique& z : zones) {
                if (p.material != z.materiau) continue;
                for (uint32_t t = 0; t + 2 < p.indexCount; t += 3) {
                    const vec3 a = pos((size_t)p.vertexOffset + idx[p.firstIndex + t]), b = pos((size_t)p.vertexOffset + idx[p.firstIndex + t + 1]),
                               c = pos((size_t)p.vertexOffset + idx[p.firstIndex + t + 2]);
                    if (!dansZone(z, (a + b + c) * (1.f / 3), cote)) continue;
                    std::vector<vec3>& v = z.glace ? glace : lampe;
                    v.push_back(a); v.push_back(b); v.push_back(c);
                }
            }
        if (lampe.empty()) { out[s] = o; continue; }
        vec3 mn{1e9f, 1e9f, 1e9f}, mx{-1e9f, -1e9f, -1e9f};
        for (const vec3& q : lampe) { mn = {std::min(mn.x, q.x), std::min(mn.y, q.y), std::min(mn.z, q.z)}; mx = {std::max(mx.x, q.x), std::max(mx.y, q.y), std::max(mx.z, q.z)}; }
        const std::vector<vec3>& face = glace.empty() ? lampe : glace;
        double A = 0, X = 0, Y = 0; vec3 N{};
        for (size_t k = 0; k + 2 < face.size(); k += 3) {
            vec3 n = cross(face[k + 1] - face[k], face[k + 2] - face[k]) * 0.5f;
            const double a = std::fabs(n.z);   // aire vue de face
            A += a; X += a * (face[k].x + face[k + 1].x + face[k + 2].x) / 3; Y += a * (face[k].y + face[k + 1].y + face[k + 2].y) / 3;
            if (n.z > 0) n = n * -1.f;          // maillages doubles faces : normale vers l'avant (−z)
            N = N + n;
        }
        if (A <= 0) { out[s] = o; continue; }
        const float x = (float)(X / A), y = (float)(Y / A);
        // z de la glace en (x, y) : 1re surface touchée par un rayon venu de l'avant (glace, sinon lampe, sinon avant de la lampe)
        auto rayon = [&](const std::vector<vec3>& T) {
            float zb = 1e9f;
            for (size_t k = 0; k + 2 < T.size(); k += 3) {
                const vec3 &a = T[k], &b = T[k + 1], &c = T[k + 2];
                const float d = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
                if (std::fabs(d) < 1e-12f) continue;
                const float w0 = ((b.x - x) * (c.y - y) - (b.y - y) * (c.x - x)) / d, w1 = ((c.x - x) * (a.y - y) - (c.y - y) * (a.x - x)) / d, w2 = 1 - w0 - w1;
                if (w0 < 0 || w1 < 0 || w2 < 0) continue;
                zb = std::min(zb, w0 * a.z + w1 * b.z + w2 * c.z);
            }
            return zb;
        };
        float z = rayon(face);
        if (z > 1e8f) z = rayon(lampe);
        if (z > 1e8f) z = mn.z;
        const float nl = length(N);
        o.centre = {x, y, z}; o.normale = nl > 0 ? N * (1.f / nl) : vec3{0, 0, -1};
        o.largeur = mx.x - mn.x; o.hauteur = mx.y - mn.y; o.triangles = (uint32_t)(lampe.size() / 3); o.trouvee = true;
        out[s] = o;
    }
}

// RG-03 : triangles de la lampe (zones non « glace ») sortis dans des pièces « phare » (PIECE_PHARE), allumées la nuit par le
// moteur ; même découpe que correctifs::marquerFeux (indices de la pièce réordonnés, une pièce de plus). Rend les triangles marqués.
template <class Part, class Pos>
uint32_t marquerPhares(std::vector<uint32_t>& idx, std::vector<Part>& parts, Pos pos, const std::vector<ZoneOptique>& zones) {
    uint32_t n = 0;
    const size_t nParts = parts.size();
    for (size_t k = 0; k < nParts; k++) {
        if (parts[k].feuArriere) continue;
        std::vector<uint32_t> garde, phare;
        for (uint32_t t = 0; t + 2 < parts[k].indexCount; t += 3) {
            const uint32_t* T = &idx[parts[k].firstIndex + t];
            const vec3 c = (pos((size_t)parts[k].vertexOffset + T[0]) + pos((size_t)parts[k].vertexOffset + T[1]) + pos((size_t)parts[k].vertexOffset + T[2])) * (1.f / 3);
            bool dedans = false;
            for (const ZoneOptique& z : zones)
                if (!z.glace && parts[k].material == z.materiau && (dansZone(z, c, -1) || dansZone(z, c, 1))) { dedans = true; break; }
            for (int j = 0; j < 3; j++) (dedans ? phare : garde).push_back(T[j]);
        }
        if (phare.empty()) continue;
        n += (uint32_t)phare.size() / 3;
        if (garde.empty()) { parts[k].feuArriere = PIECE_PHARE; continue; }   // pièce entière dans l'optique
        std::copy(garde.begin(), garde.end(), idx.begin() + parts[k].firstIndex);
        std::copy(phare.begin(), phare.end(), idx.begin() + parts[k].firstIndex + garde.size());
        Part q = parts[k];
        q.firstIndex = parts[k].firstIndex + (uint32_t)garde.size(); q.indexCount = (uint32_t)phare.size(); q.feuArriere = PIECE_PHARE;
        parts[k].indexCount = (uint32_t)garde.size();
        parts.push_back(q);
    }
    return n;
}

// un matériau du .wfm : drapeaux d'origine, émission, boîte de toutes ses pièces (repère du modèle)
struct Materiau { uint32_t flags = 0; vec3 emissif{}; vec3 bmin{1e9f, 1e9f, 1e9f}, bmax{-1e9f, -1e9f, -1e9f}; };
// WFS-171 (RG-03, ANO-78) : recul = centre du feu de recul dans le bloc découpé (côté +x ; l'autre côté en miroir)
struct Resultat { int materiau = -1; uint32_t triangles = 0; vec3 bmin{1e9f, 1e9f, 1e9f}, bmax{-1e9f, -1e9f, -1e9f}; int troisieme = -1;
                  vec3 recul{}; bool reculTrouve = false; };

// ---------- WFS-171 : valeurs photométriques réelles des feux (sources : règlements ONU-CEE, textes consolidés) ----------
// R7 (feux de position et stop) : feu de position R1 4 à 17 cd dans l'axe, feu stop S1 60 à 260 cd ; un feu stop
// combiné au feu de position doit être au moins 5 fois plus intense (R7 § 6). R23 (feux de recul) : 80 cd au moins
// dans l'axe, 300 cd au plus au-dessus de l'horizontale, 600 cd au plus en dessous. Ces règlements ne mesurent que
// ±10 à 15° autour de l'axe ; vers la chaussée (30 à 80° sous l'horizontale), on prend 30 % de la valeur typique de
// l'axe (estimation, diffusion du cabochon) : position 10 cd → 3 cd, stop 100 cd → 30 cd (rapport 10 ≥ 5 de R7).
constexpr float CD_POSITION_SOL = 3.f, CD_STOP_SOL = 30.f;   // intensité d'un feu vers la chaussée (cd)
constexpr float CD_RECUL_SOL = 40.f;                          // feu de recul vers la chaussée (cd, R23 : 80-600)
// Étalon de la scène : lampadaire de ville 1,1 (portée 26 m, décroissance 2) à 7,1 m → 0,58 sous le mât ≡ 20 lux
// (DA-realisme § 3 : 10 à 30 lux sur la chaussée) : 1 unité d'éclairement du moteur ≈ 35 lux
constexpr float LUX_PAR_UNITE = 35.f;
constexpr float ALBEDO_ASPHALTE = 0.10f;                      // DA-realisme § 3 : asphalte usé 0,10-0,18
constexpr float PI_F = 3.14159265f;

// RG-01 : éclairement (lux par cd) d'un point de la chaussée à s m derrière la glace du feu et u m de côté, feu à h m
// de haut : E = I(α) cos β / r², cos β = h / r (chaussée horizontale) ; I(α) = I (0,5 + 0,5 cos α), α depuis l'axe du
// feu (horizontal, vers l'arrière) : moitié diffusée par le cabochon, moitié dans l'axe. Maximum au pied du pare-chocs,
// décroissance continue (aucun bord net) ; nul devant la glace (s < 0).
inline float eclairementSol(float s, float u, float h) {
    if (s < 0) return 0;
    const float r2 = s * s + u * u + h * h, r = std::sqrt(r2);
    return (0.5f + 0.5f * s / r) * h / (r2 * r);
}
// luminance ajoutée à la chaussée (unités HDR du moteur) pour un éclairement en lux : ρ / π × E / LUX_PAR_UNITE
inline float luminanceSol(float lux) { return ALBEDO_ASPHALTE / PI_F * lux / LUX_PAR_UNITE; }

// feu MAT_TAIL accepté par le bloc WFS-09 de model_loader.cpp : bloc bas (< 0,5 m) dans le dernier tiers (z > 0,3 L)
inline bool feuArriereValide(const Materiau& m, float L) {
    return (m.flags & DRAPEAU_TAIL) && m.bmax.x >= m.bmin.x && m.bmax.y - m.bmin.y < HAUTEUR_MAX && m.bmin.z > L * 0.3f;
}
// RG-03 : 3e feu stop = feu arrière étroit et centré (giulia : matériau 40, 0,3 m de large, à 1,09 m de haut)
inline bool troisiemeFeu(const Materiau& m, float L) {
    return feuArriereValide(m, L) && m.bmax.x - m.bmin.x < TROISIEME_LARGEUR && std::abs(m.bmin.x + m.bmax.x) < TROISIEME_CENTRE * 2;
}

// RG-02 : niveau du feu stop (0 = repos, 1 = stop) après dt secondes ; montée et descente linéaires en 80 ms ;
// dt < 0 (première image) ou trop grand (voiture revue après une pause) : niveau atteint tout de suite
inline float transition(float niveau, bool stop, float dt) {
    const float cible = stop ? 1.f : 0.f;
    if (dt < 0 || dt > 0.25f) return cible;
    const float pas = dt / TRANSITION_S;
    return niveau < cible ? std::min(cible, niveau + pas) : std::max(cible, niveau - pas);
}

// WFS-171 (signalé par WFS-170) : boîte des pièces « feu arrière » marquées au chargement (correctifs de WFS-170 : V60,
// GT86, Abarth, A45) : position mesurée du feu, comme la découpe ; sans elle, le halo de repli flottait hors de la caisse
template <class Part, class Pos>
void boiteFeuxMarques(const std::vector<uint32_t>& idx, const std::vector<Part>& parts, Pos pos, vec3& bmin, vec3& bmax) {
    for (const Part& p : parts) {
        if (p.feuArriere != PIECE_FEU_ARRIERE) continue;
        for (uint32_t k = 0; k < p.indexCount; k++) {
            const vec3 q = pos((size_t)p.vertexOffset + idx[p.firstIndex + k]);
            bmin = {std::min(bmin.x, q.x), std::min(bmin.y, q.y), std::min(bmin.z, q.z)};
            bmax = {std::max(bmax.x, q.x), std::max(bmax.y, q.y), std::max(bmax.z, q.z)};
        }
    }
}
// RG-13 / RG-03 : découpe d'un modèle. Part : firstIndex, indexCount, vertexOffset, material, feuArriere (ModelPart) ;
// pos(v) : position du sommet v (indice absolu). Au plus une pièce de plus par pièce du matériau choisi (même matériau,
// mêmes tampons) ; un modèle qui a déjà un feu MAT_TAIL valide n'est pas découpé (son 3e feu stop est seulement marqué).
template <class Part, class Pos>
Resultat decouper(std::vector<uint32_t>& idx, std::vector<Part>& parts, Pos pos, const std::vector<Materiau>& mats, float L) {
    Resultat res;
    bool tail = false;
    for (size_t mi = 0; mi < mats.size(); mi++) {
        tail = tail || feuArriereValide(mats[mi], L);
        if (troisiemeFeu(mats[mi], L)) res.troisieme = (int)mi;
    }
    if (res.troisieme >= 0) for (Part& p : parts) if ((int)p.material == res.troisieme) p.feuArriere = PIECE_TROISIEME_FEU;
    if (tail) return res;
    auto arriere = [&](const Part& p, uint32_t t) {
        float z = 0;
        for (int j = 0; j < 3; j++) z += pos((size_t)p.vertexOffset + idx[p.firstIndex + t * 3 + j]).z;
        return z / 3 > SEUIL_Z * L;
    };
    // candidat : matériau de feux non MAT_TAIL (drapeau MAT_HEAD d'origine ou émissif) ; on garde celui qui a le plus de
    // triangles à l'arrière, si sa pièce arrière reste basse
    for (size_t mi = 0; mi < mats.size(); mi++) {
        const Materiau& m = mats[mi];
        if ((m.flags & DRAPEAU_TAIL) || !((m.flags & DRAPEAU_HEAD) || std::max({m.emissif.x, m.emissif.y, m.emissif.z}) > 0)) continue;
        Resultat r; r.materiau = (int)mi;
        for (const Part& p : parts) {
            if (p.material != mi) continue;
            for (uint32_t t = 0; t < p.indexCount / 3; t++) {
                if (!arriere(p, t)) continue;
                r.triangles++;
                for (int j = 0; j < 3; j++) {
                    const vec3 q = pos((size_t)p.vertexOffset + idx[p.firstIndex + t * 3 + j]);
                    r.bmin = {std::min(r.bmin.x, q.x), std::min(r.bmin.y, q.y), std::min(r.bmin.z, q.z)};
                    r.bmax = {std::max(r.bmax.x, q.x), std::max(r.bmax.y, q.y), std::max(r.bmax.z, q.z)};
                }
            }
        }
        if (r.triangles && r.bmax.y - r.bmin.y <= HAUTEUR_MAX && r.triangles > res.triangles) { r.troisieme = res.troisieme; res = r; }
    }
    if (res.materiau < 0) return res;
    // WFS-171 (RG-03) : feu de recul dans la partie intérieure basse du bloc (côté +x) : 30 % intérieurs de la largeur du
    // bloc, au tiers bas de leur hauteur, au ras de la glace (Golf VII : le recul est dans la partie du feu posée sur le hayon)
    float xin = 1e9f, xout = -1e9f;
    auto chaqueSommet = [&](auto&& f) {
        for (const Part& p : parts) {
            if ((int)p.material != res.materiau) continue;
            for (uint32_t t = 0; t < p.indexCount / 3; t++)
                if (arriere(p, t)) for (int j = 0; j < 3; j++) f(pos((size_t)p.vertexOffset + idx[p.firstIndex + t * 3 + j]));
        }
    };
    chaqueSommet([&](const vec3& q) { if (q.x > 0) { xin = std::min(xin, q.x); xout = std::max(xout, q.x); } });
    if (xout > xin) {
        const float lim = xin + 0.3f * (xout - xin);
        float ylo = 1e9f, yhi = -1e9f, zmax = -1e9f;
        chaqueSommet([&](const vec3& q) { if (q.x > 0 && q.x <= lim) { ylo = std::min(ylo, q.y); yhi = std::max(yhi, q.y); zmax = std::max(zmax, q.z); } });
        res.recul = {xin + 0.15f * (xout - xin), ylo + 0.3f * (yhi - ylo), zmax};
        res.reculTrouve = true;
    }
    // pièces du matériau choisi : triangles de l'avant d'un côté, de l'arrière de l'autre (indices regroupés)
    std::vector<uint32_t> idx2; idx2.reserve(idx.size());
    std::vector<Part> parts2; parts2.reserve(parts.size() + 4);
    for (const Part& p : parts) {
        const bool coupe = (int)p.material == res.materiau;
        for (int cote = 0; cote < (coupe ? 2 : 1); cote++) {
            Part q = p; q.firstIndex = (uint32_t)idx2.size();
            for (uint32_t t = 0; t < p.indexCount / 3; t++)
                if (!coupe || arriere(p, t) == (cote == 1)) for (int j = 0; j < 3; j++) idx2.push_back(idx[p.firstIndex + t * 3 + j]);
            q.indexCount = (uint32_t)idx2.size() - q.firstIndex;
            if (cote == 1) q.feuArriere = PIECE_FEU_ARRIERE;
            if (q.indexCount) parts2.push_back(q);
        }
    }
    idx.swap(idx2); parts.swap(parts2);
    return res;
}

} // namespace wfs::feux_modele
