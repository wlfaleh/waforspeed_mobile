// WaForSpeed — intérieurs par modèle de la vue cockpit (WFS-197) : voir vehicule_cockpit.h.
#include "vehicule_cockpit.h"
#include <algorithm>
#include <cstring>
#include "../core/log.h"
#include "vehicule_cockpit_geo.h"
#include "vk.h"

namespace wfs::cockpit {

// WFS-282 : pièces construites par code, vues de l'intérieur : jamais de retrait des faces arrière (WFS-246 retire les faces
// arrière des matériaux opaques sans MAT_DOUBLE : le dessus de la planche de bord disparaissait, on voyait la route au travers)
static Material deuxFaces(Material m) { m.flags |= MAT_DOUBLE; return m; }
// WFS-282 (RG-04) : lumière d'ambiance de nuit de l'habitacle, en part de l'albédo de chaque matière (émission, mêmes unités que les écrans)
constexpr float AMBIANCE_NUIT = 0.25f;

bool g_interieurCle = false;   // banc « cockpit » (WFS-235, RG-05) : voir vehicule_cockpit.h
const Material& Interieur::mat(const Material& m) const { return g_interieurCle && cleMat_.set ? cleMat_ : m; }

// définitions par modèle (DEFS, interieurPour) : vehicule_cockpit_defs.cpp (sans rendu : testées par tests/test_cockpit.cpp)

// ---------------------------------------------------------------- géométrie
namespace {

constexpr float CX = 0.36f;   // centre de la voiture (repère de l'œil)
constexpr float GRAIN = 12.f;   // répétitions du grain par mètre (motif de 8 cm, cellules de 2,6 mm : grain réel du cuir et du plastique)

vec3 lin(unsigned h) {
    auto l = [](float c) { return std::pow(c, 2.2f); };
    return {l(((h >> 16) & 255) / 255.f), l(((h >> 8) & 255) / 255.f), l((h & 255) / 255.f)};
}
void ajouterUV(Geo& g, const mat4& M, vec3 p, vec3 n, float u, float v) {
    g.sommet(M, p, n); g.v.back().u = u; g.v.back().v = v;
}

// profil (y, z) extrudé le long de x, de x0 à x1 : une bande par segment du profil ; normale dans le plan (y, z)
void extruder(Geo& g, const mat4& M, const std::vector<vec2>& profil, float x0, float x1) {
    float s = 0;
    for (size_t k = 0; k + 1 < profil.size(); k++) {
        const vec2 a = profil[k], b = profil[k + 1];
        const vec2 d{b.x - a.x, b.y - a.y};
        const float l = std::sqrt(d.x * d.x + d.y * d.y);
        if (l < 1e-5f) continue;
        const vec3 n = normalize(vec3{0, -d.y, d.x});   // profil parcouru du pare-brise vers le conducteur : normale vers le haut / l'œil
        const uint32_t b0 = (uint32_t)g.v.size();
        ajouterUV(g, M, {x0, a.x, a.y}, n, x0 * GRAIN, s * GRAIN); ajouterUV(g, M, {x1, a.x, a.y}, n, x1 * GRAIN, s * GRAIN);
        ajouterUV(g, M, {x1, b.x, b.y}, n, x1 * GRAIN, (s + l) * GRAIN); ajouterUV(g, M, {x0, b.x, b.y}, n, x0 * GRAIN, (s + l) * GRAIN);
        g.quad(b0, b0 + 1, b0 + 2, b0 + 3);
        s += l;
    }
    // joues aux deux bouts (le profil fermé par le bas)
    for (float x : {x0, x1}) {
        const uint32_t c0 = (uint32_t)g.v.size();
        const vec3 n{x == x0 ? -1.f : 1.f, 0, 0};
        float yc = 0, zc = 0; for (const vec2& p : profil) { yc += p.x; zc += p.y; }
        yc /= (float)profil.size(); zc /= (float)profil.size();
        ajouterUV(g, M, {x, yc, zc}, n, 0, 0);
        for (const vec2& p : profil) ajouterUV(g, M, {x, p.x, p.y}, n, p.y, p.x);
        for (size_t k = 0; k + 1 < profil.size(); k++) g.i.insert(g.i.end(), {c0, c0 + 1 + (uint32_t)k, c0 + 2 + (uint32_t)k});
    }
}

// barre de section w × t entre deux points (montants, bras d'essuie-glace)
void barre(Geo& g, const mat4& M, vec3 a, vec3 b, float w, float t) {
    const vec3 d = b - a; const float l = length(d);
    if (l < 1e-5f) return;
    const vec3 y = d * (1.f / l), ref = std::fabs(y.x) < 0.9f ? vec3{1, 0, 0} : vec3{0, 0, 1};
    const vec3 x = normalize(cross(y, ref)), z = cross(x, y);
    mat4 B = mat4::identity();
    B.at(0, 0) = x.x; B.at(1, 0) = x.y; B.at(2, 0) = x.z;
    B.at(0, 1) = y.x; B.at(1, 1) = y.y; B.at(2, 1) = y.z;
    B.at(0, 2) = z.x; B.at(1, 2) = z.y; B.at(2, 2) = z.z;
    const vec3 c = (a + b) * 0.5f;
    boite(g, M * translate(c) * B, w, l, t);
}

// jante du volant (plan xy, face +z vers le conducteur) : tube de rayon r autour d'un cercle de rayon R, aplati en bas
// (méplat : la ligne médiane ne descend pas sous −R × (1 − meplat), raccord arrondi) ; uv pour le grain du cuir
vec2 ligneJante(float a, float R, float meplat, float meplatHaut = 0) {
    vec2 p{std::cos(a) * R, std::sin(a) * R};
    if (meplatHaut > 0) {   // WFS-201 : même raccord en haut (208)
        const float haut = R * (1 - meplatHaut), raccord = R * 0.08f;
        if (p.y > haut - raccord) {
            const float t = std::clamp((p.y - (haut - raccord)) / (2 * raccord), 0.f, 1.f);
            p.y = p.y + (haut - p.y) * t * t * (3 - 2 * t) * (p.y > haut ? 1.f : 0.5f);
            if (p.y > haut) p.y = haut;
        }
    }
    if (meplat > 0) {
        const float bas = -R * (1 - meplat), raccord = R * 0.08f;
        if (p.y < bas + raccord) {   // fonction douce (min lissé) entre le cercle et la droite du méplat
            const float t = std::clamp((bas + raccord - p.y) / (2 * raccord), 0.f, 1.f);
            p.y = p.y + (bas - p.y) * t * t * (3 - 2 * t) * (p.y < bas ? 1.f : 0.5f);
            if (p.y < bas) p.y = bas;
        }
    }
    return p;
}
void jante(Geo& g, const mat4& M, float R, float r, float meplat, float a0, float a1, int n, int m, float meplatHaut = 0) {
    const uint32_t b = (uint32_t)g.v.size();
    for (int k = 0; k <= n; k++) {
        const float a = a0 + (a1 - a0) * k / n;
        const vec2 c = ligneJante(a, R, meplat, meplatHaut), c2 = ligneJante(a + 0.01f, R, meplat, meplatHaut);
        const vec2 t{c2.x - c.x, c2.y - c.y};
        const float tl = std::max(1e-6f, std::sqrt(t.x * t.x + t.y * t.y));
        const vec3 nr{t.y / tl, -t.x / tl, 0};   // normale dans le plan, vers l'extérieur
        for (int j = 0; j <= m; j++) {
            const float w = 2 * PI * j / m;
            const vec3 n3 = nr * std::cos(w) + vec3{0, 0, 1} * std::sin(w);
            ajouterUV(g, M, vec3{c.x, c.y, 0} + n3 * r, n3, (a - a0) * R * GRAIN, (float)j / m * 2 * PI * r * GRAIN);
        }
    }
    for (int k = 0; k < n; k++) for (int j = 0; j < m; j++) {
        const uint32_t a = b + k * (m + 1) + j, c = a + m + 1;
        g.quad(a, c, c + 1, a + 1);
    }
}

// aérateur : cadre (accent) et lames sombres, face +z, centré en c, taille w × h
void aerateur(Geo& cadre, Geo& lames, const mat4& M, float w, float h, bool rond) {
    if (rond) {
        anneau(cadre, M, std::min(w, h) * 0.40f, std::min(w, h) * 0.5f, 28);
        for (int k = -2; k <= 2; k++) boite(lames, M * translate({0, k * h * 0.15f, -0.004f}), w * 0.78f, h * 0.035f, 0.006f);
        return;
    }
    const float e = std::min(w, h) * 0.10f;
    boite(cadre, M * translate({0, h / 2 - e / 2, 0}), w, e, 0.012f); boite(cadre, M * translate({0, -h / 2 + e / 2, 0}), w, e, 0.012f);
    boite(cadre, M * translate({-w / 2 + e / 2, 0, 0}), e, h, 0.012f); boite(cadre, M * translate({w / 2 - e / 2, 0, 0}), e, h, 0.012f);
    const int n = std::max(2, (int)(h / 0.016f));
    for (int k = 0; k < n; k++) {
        const float y = -h / 2 + e + (h - 2 * e) * (k + 0.5f) / n;
        boite(lames, M * translate({0, y, -0.006f}) * rotateX(0.35f), w - 2 * e, (h - 2 * e) / n * 0.55f, 0.004f);
    }
}

// orientation d'un écran tourné vers l'œil (normale +z vers le conducteur), avec un lacet donné
mat4 versOeil(vec3 p, float lacet) { return translate(p) * rotateY(lacet) * rotateX(std::atan2(p.y, -p.z)); }

// normal map du grain du cuir (256 × 256, répétable) : bosses de cellules irrégulières (Worley simplifié) ; RGBA linéaire
std::vector<uint8_t> grainCuir(int N) {
    std::vector<float> h((size_t)N * N);
    uint32_t s = 0x2545F491u;
    auto rnd = [&]() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s & 0xFFFFFF) / 16777216.f; };
    const int C = 32; std::vector<vec2> pts((size_t)C * C);
    for (auto& p : pts) p = {rnd(), rnd()};
    for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
        const float fx = (float)x / N * C, fy = (float)y / N * C; const int cx = (int)fx, cy = (int)fy;
        float d1 = 9, d2 = 9;
        for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
            const int gx = (cx + i + C) % C, gy = (cy + j + C) % C; const vec2 p = pts[(size_t)gy * C + gx];
            const float dx = cx + i + p.x - fx, dy = cy + j + p.y - fy, d = dx * dx + dy * dy;
            if (d < d1) { d2 = d1; d1 = d; } else if (d < d2) d2 = d;
        }
        h[(size_t)y * N + x] = std::sqrt(d2) - std::sqrt(d1);   // creux le long des bords des cellules
    }
    std::vector<uint8_t> o((size_t)N * N * 4);
    for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
        auto H = [&](int i, int j) { return h[(size_t)((j + N) % N) * N + (i + N) % N]; };
        const float dx = (H(x + 1, y) - H(x - 1, y)) * 1.2f, dy = (H(x, y + 1) - H(x, y - 1)) * 1.2f;
        const vec3 n = normalize(vec3{-dx, -dy, 1});
        uint8_t* q = &o[((size_t)y * N + x) * 4];
        q[0] = (uint8_t)std::lround((n.x * 0.5f + 0.5f) * 255); q[1] = (uint8_t)std::lround((n.y * 0.5f + 0.5f) * 255);
        q[2] = (uint8_t)std::lround((n.z * 0.5f + 0.5f) * 255); q[3] = 255;
    }
    return o;
}

// ---------- matériaux de l'intérieur (indices de Interieur::mats_)
enum : int { M_CUIR, M_PLANCHE_HAUT, M_PLANCHE_BAS, M_ACCENT, M_LAQUE, M_BRANCHES, M_COUTURE0, M_COUTURE1, M_COUTURE2, M_REPERE,
             M_FOND_COMBINE, M_FOND_ECRAN, M_FOND_ECRAN_NUIT, M_LAMES, M_PAVILLON, M_MOYEU, M_CHROME, M_GLACE, M_N };
// écrans dynamiques
enum : int { D_ACTIF, D_FOND, D_ALERTE, D_ROUTE, D_FLECHE, D_N };
constexpr size_t DYN_SOMMETS = 4096;

// ---------- dessin 2D dans le plan d'un écran (repère local : x à droite, y en haut, m ; z = décollement)
struct Plan2D {
    std::vector<Vertex>* v; std::vector<uint32_t>* i; float z;
    void quad(vec2 a, vec2 b, vec2 c, vec2 d) {
        if (v->size() + 4 > DYN_SOMMETS) return;
        const uint32_t k = (uint32_t)v->size();
        for (vec2 p : {a, b, c, d}) v->push_back({p.x, p.y, z, 0, 0, 1, 0, 0});
        i->insert(i->end(), {k, k + 1, k + 2, k, k + 2, k + 3});
    }
    void rect(float x0, float y0, float x1, float y1) { quad({x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}); }
    // trait de c, de longueur l et largeur w, orienté par a (rad, 0 = vers la droite)
    void trait(vec2 c, float a, float l0, float l1, float w) {
        const vec2 d{std::cos(a), std::sin(a)}, n{-d.y * w / 2, d.x * w / 2};
        const vec2 p0{c.x + d.x * l0, c.y + d.y * l0}, p1{c.x + d.x * l1, c.y + d.y * l1};
        quad({p0.x - n.x, p0.y - n.y}, {p1.x - n.x, p1.y - n.y}, {p1.x + n.x, p1.y + n.y}, {p0.x + n.x, p0.y + n.y});
    }
    void segment(vec2 a, vec2 b, float w) {
        const float dx = b.x - a.x, dy = b.y - a.y, l = std::sqrt(dx * dx + dy * dy);
        if (l < 1e-6f) return;
        trait(a, std::atan2(dy, dx), -w / 2, l + w / 2, w);
    }
};
// chiffre à 7 segments de hauteur h, coin bas gauche (x, y)
void chiffre(Plan2D& p, float x, float y, float h, uint8_t seg) {
    const float w = h * 0.52f, e = h * 0.13f, m = y + h / 2;
    if (seg & 1) p.rect(x + e, y + h - e, x + w - e, y + h);
    if (seg & 2) p.rect(x + w - e, m, x + w, y + h - e);
    if (seg & 4) p.rect(x + w - e, y + e, x + w, m);
    if (seg & 8) p.rect(x + e, y, x + w - e, y + e);
    if (seg & 16) p.rect(x, y + e, x + e, m);
    if (seg & 32) p.rect(x, m, x + e, y + h - e);
    if (seg & 64) p.rect(x + e, m - e / 2, x + w - e, m + e / 2);
}
// nombre aligné à droite sur xd ; zéros de tête omis
void nombre(Plan2D& p, float xd, float y, float h, int n, int chiffresMax) {
    const float pas = h * 0.66f;
    int k = 0;
    do { chiffre(p, xd - pas * (k + 1), y, h, segments7(n % 10)); n /= 10; k++; } while (n > 0 && k < chiffresMax);
}
void rapport(Plan2D& p, float x, float y, float h, int r) {
    chiffre(p, x, y, h, r > 0 ? segments7(std::min(r, 9)) : r < 0 ? SEG_R : SEG_N);
}
// arc de n barres de rayon ri → ro, de l'angle a0 à a1 ; les barres sous `part` vont dans `allume`, les autres dans
// `eteint` ; au-delà de `rouge` (part de l'arc), les barres allumées vont dans `alerte`
void arcBarres(Plan2D& allume, Plan2D& eteint, Plan2D& alerte, vec2 c, float ri, float ro, float a0, float a1, int n, float part,
               float rouge, float largeur) {
    for (int k = 0; k < n; k++) {
        const float f = (k + 0.5f) / n, a = a0 + (a1 - a0) * f;
        Plan2D& cible = f <= part ? (f >= rouge ? alerte : allume) : eteint;
        cible.trait(c, a, ri, ro, largeur);
    }
}

// WFS-201 : cadran à aiguille (arc de 270°, de 225° à −45°) : graduations (fond), zone rouge au-delà de `rouge` (alerte), aiguille
// à la part `part` (actif), pivot au centre ; partagé par les combinés analogiques du lot 2 (Audi, Alfa, Lamborghini)
void cadran(Plan2D& F, Plan2D& A, Plan2D& R, vec2 c, float r, float part, float rouge, int graduations, float epaisseur) {
    arcBarres(F, F, R, c, r * 0.84f, r, PI * 1.25f, -PI * 0.25f, graduations, 1.f, rouge, epaisseur);
    A.trait(c, PI * 1.25f - PI * 1.5f * std::clamp(part, 0.f, 1.f), -r * 0.12f, r * 0.90f, epaisseur * 1.3f);
}
// lettre « d » (marche avant) des voitures à boîte automatique sans rapports affichés (Model 3)
constexpr uint8_t SEG_D = 0x5E;

} // namespace

// ---------------------------------------------------------------- construction
void Interieur::ajouter(Renderer& r, std::vector<Vertex>& v, std::vector<uint32_t>& i, int mat, int attache) {
    if (v.empty()) return;
    octets_ += v.size() * sizeof(Vertex) + i.size() * 4;
    pieces_.push_back({r.makeMesh(v, i), mat, attache});
    v.clear(); i.clear();
}

bool Interieur::construire(Renderer& r, const InterieurDef& d) {
    detruire(r);
    def_ = &d; octets_ = 0;
    // matériaux (valeurs physiques : charte du réel, DA-realisme.md ; cuir albédo 0,04-0,12, rugosité 0,55-0,75)
    {
        const std::vector<uint8_t> px = grainCuir(256);
        grain_ = r.vk().uploadTexture(px.data(), 256, 256, false);
        octets_ += px.size();
    }
    mats_.assign(M_N, Material{});
    auto mat = [&](int k, unsigned c, float rough, float metal, bool grain = false) {
        Material m; const vec3 l = lin(c); m.color = {l.x, l.y, l.z, 1}; m.rough = rough; m.metal = metal;
        mats_[(size_t)k] = r.makeMaterial(deuxFaces(m), nullptr, grain && grain_.view ? &grain_ : nullptr, nullptr);
    };
    auto ecran = [&](int k, unsigned c, float force) {
        Material m; m.color = {0, 0, 0, 1}; m.rough = 0.2f; m.metal = 0; m.emissive = lin(c) * force;
        mats_[(size_t)k] = r.makeMaterial(deuxFaces(m), nullptr, nullptr, nullptr);
    };
    // cuir du volant : couleur du cuir ramenée dans la plage de la charte (albédo linéaire ≥ 0,04 : cuir noir réel ≈ 0,04-0,05)
    {
        Material m; vec3 l = lin(d.cuir); const float y = 0.2126f * l.x + 0.7152f * l.y + 0.0722f * l.z;
        if (y < 0.04f) l = y > 1e-5f ? l * (0.045f / y) : vec3{0.045f, 0.045f, 0.045f};
        m.color = {l.x, l.y, l.z, 1}; m.rough = 0.65f; m.metal = 0;
        mats_[M_CUIR] = r.makeMaterial(deuxFaces(m), nullptr, grain_.view ? &grain_ : nullptr, nullptr);
    }
    mat(M_PLANCHE_HAUT, d.plancheHaut, 0.72f, 0, true);    // haut de planche : cuir ou matière moussée grainée
    mat(M_PLANCHE_BAS, d.plancheBas, 0.85f, 0, true);      // plastique grainé (aucun reflet métallique)
    mat(M_ACCENT, d.accent, d.accentRugo, d.accentMetal);   // alu brossé (métal 1), carbone verni (0,3), noir laqué
    mat(M_LAQUE, 0x0a0a0b, 0.12f, 0);                       // noir laqué des cadres d'écran, rétroviseur
    mat(M_BRANCHES, d.branchesCol, d.branchesCol > 0x505050 ? 0.35f : 0.3f, d.branchesCol > 0x505050 ? 1.f : 0.f);
    for (int k = 0; k < 3; k++) mat(M_COUTURE0 + k, d.couture[k] ? d.couture[k] : 0x202020, 0.7f, 0);
    // repères et commandes rouges (manettino, démarreur, boutons M, agrafe GTI) : plastique rouge satiné, teinte assombrie de
    // moitié (la couleur vive de la marque, vue sous le ciel sans toit au-dessus, sortait rose)
    {
        const unsigned c = d.repereCol ? d.repereCol : 0x202020;
        mat(M_REPERE, ((((c >> 16) & 255) / 2) << 16) | ((((c >> 8) & 255) / 2) << 8) | ((c & 255) / 2), 0.7f, 0);
    }
    mat(M_MOYEU, 0x1c1c1e, 0.85f, 0);                       // coussin du sac gonflable : plastique moussé (pièce sans uv : pas de grain)
    // enjoliveurs des aérateurs : chrome satiné (mat.chrome, albédo 0,65-0,70) ; alu brossé sur la 911
    if (d.accentMetal > 0.5f) mat(M_CHROME, d.accent, 0.35f, 1.f); else mat(M_CHROME, 0xd2d6da, 0.18f, 1.f);
    ecran(M_FOND_COMBINE, d.combineFond, 1.0f);
    ecran(M_FOND_ECRAN, 0x1b2430, 1.0f);
    ecran(M_FOND_ECRAN_NUIT, 0x0b0f15, 1.0f);
    mat(M_LAMES, 0x0a0a0a, 0.8f, 0);
    mat(M_PAVILLON, 0x26272a, 0.95f, 0);                    // ciel de toit (tissu, Alcantara)
    // WFS-282 (RG-03) : glace du rétroviseur : surface claire et lisse, éclairée par le ciel (claire le jour) ; en métal, elle ne
    // reflétait presque rien dans l'habitacle (bloc presque noir, refus du lead dev sur la PR #251)
    mat(M_GLACE, 0xa4aab0, 0.12f, 0);
    // WFS-282 (RG-04) : la nuit, l'habitacle reçoit un peu de lumière (compteurs, écran, rue, ≈ 0,5 à 1 lux) : chaque matière
    // garde sa couleur, plus une petite émission proportionnelle à son albédo (métaux : un tiers) ; sans elle, planche,
    // volant et montants sortaient en aplats noirs
    // ; ni les écrans (déjà lumineux), ni la glace du rétroviseur (la nuit, elle reste sombre comme le ciel qu'elle reflète)
    matsNuit_ = mats_;
    for (int k = 0; k < M_N; k++) {
        Material& m = matsNuit_[(size_t)k];
        const bool lumineux = m.emissive.x + m.emissive.y + m.emissive.z > 0.f;
        if (k == M_GLACE) {   // glace de nuit : position nuit d'un rétroviseur (réflexion 0,5 %, sans diffus, glace lisse : les lampes ne laissent qu'un petit reflet) : jamais plus claire que le ciel de nuit
            m.color = {0.005f, 0.005f, 0.005f, m.color.w}; m.metal = 1.f; m.rough = 0.15f;   // métal, rugosité : constantes du dessin
            continue;
        }
        if (lumineux) continue;
        m.emissive = vec3{m.color.x, m.color.y, m.color.z} * (AMBIANCE_NUIT * (m.metal > 0.5f ? 0.33f : 1.f));
    }
    std::vector<Vertex> v; std::vector<uint32_t> i;
    auto prend = [&](Geo& g, int m, int attache = 0) { v = std::move(g.v); i = std::move(g.i); ajouter(r, v, i, m, attache); g = Geo{}; };
    const mat4 I = mat4::identity();
    const float Y = d.plancheY, Z = d.plancheZ, Yb = d.pareBriseY, Zb = d.pareBriseZ, x0 = CX - 0.78f, x1 = CX + 0.78f;
    // planche (profil y, z du pied du pare-brise vers le conducteur) : dessus, lèvre, face en retrait derrière le combiné et
    // l'écran central (z ≈ −0,86), puis la partie basse qui revient vers les genoux
    const bool combine = d.combine != Combine::Aucun && d.combineW > 0;   // WFS-201 : Model 3 : aucun combiné derrière le volant
    const float zFace = std::min(combine ? d.combinePos.z : d.ecranPos.z, d.ecranW > 0 ? d.ecranPos.z : d.combinePos.z) - 0.05f;
    {
        Geo haut, face, creux;
        // devant le conducteur, la planche descend sous le combiné (creux) : l'écran, posé au-dessus, se voit dans l'ouverture
        // haute du volant, sous sa visière ; ailleurs, le dessus de planche va du pare-brise à la lèvre
        const float xa = d.combinePos.x - d.combineW * 0.5f - 0.03f, xb = d.combinePos.x + d.combineW * 0.5f + 0.03f;
        const float Yc = std::min(Y, d.combinePos.y - d.combineH * 0.5f - 0.012f);
        const std::vector<vec2> pHaut{{Yb, Zb}, {Y + 0.005f, (Zb + Z) * 0.5f}, {Y, Z - 0.02f}, {Y - 0.035f, Z}};
        const std::vector<vec2> pCreux{{Yb, Zb}, {Yc, d.combinePos.z - 0.03f}, {Yc, Z - 0.02f}, {Yc - 0.03f, Z}};
        if (combine) { extruder(haut, I, pHaut, x0, xa); extruder(haut, I, pHaut, xb, x1); extruder(creux, I, pCreux, xa, xb); }
        else extruder(haut, I, pHaut, x0, x1);
        extruder(face, I, {{Y - 0.035f, Z}, {Y - 0.09f, zFace}, {Y - 0.42f, zFace}, {Y - 0.58f, zFace + 0.20f}}, x0, x1);
        prend(haut, M_PLANCHE_HAUT); prend(creux, M_PLANCHE_HAUT); prend(face, M_PLANCHE_BAS);
        // casquette (visière) du combiné : avance de la lèvre vers le conducteur au-dessus de l'écran
        Geo cas;
        const float cw = d.combineW * 0.5f + 0.03f, top = d.combinePos.y + d.combineH * 0.5f + 0.02f;
        extruder(cas, I, {{Yb, d.combinePos.z - 0.10f}, {top + 0.015f, d.combinePos.z - 0.03f}, {top + 0.012f, d.combinePos.z + 0.07f},
                          {top - 0.012f, d.combinePos.z + 0.075f}}, d.combinePos.x - cw, d.combinePos.x + cw);
        if (combine) prend(cas, M_PLANCHE_HAUT); else cas = Geo{};
        // jonc décoratif sur toute la largeur, sous la lèvre (alu brossé, carbone, ou liseré rouge de la Golf)
        Geo jonc;
        boite(jonc, translate({CX, Y - 0.10f, zFace + 0.004f}), x1 - x0 - 0.04f, d.carbone ? 0.05f : 0.010f, 0.006f);
        prend(jonc, M_ACCENT);
    }
    // aérateurs : deux aux extrémités, deux au centre (sous l'écran central quand il est sur la planche)
    {
        Geo cadre, lames;
        const bool rond = d.combine == Combine::Porsche5 || d.combine == Combine::FerrariIncurve;
        const float ay = Y - 0.17f, az = zFace + 0.012f;
        for (float x : {x0 + 0.11f, x1 - 0.11f}) aerateur(cadre, lames, translate({x, ay, az}) * rotateX(-0.2f), 0.13f, rond ? 0.09f : 0.055f, rond);
        for (float x : {CX - 0.09f, CX + 0.09f}) aerateur(cadre, lames, translate({x, ay - 0.04f, az + 0.01f}) * rotateX(-0.2f), 0.11f, 0.045f, false);
        prend(cadre, M_CHROME); prend(lames, M_LAMES);
    }
    // console centrale et sélecteur
    {
        Geo con, sel;
        boite(con, translate({CX, Y - 0.50f, zFace + 0.32f}) * rotateX(-0.40f), 0.26f, 0.12f, 0.60f);
        cylindre(sel, translate({CX, Y - 0.42f, zFace + 0.40f}) * rotateX(-0.2f), 0.020f, 0.024f, 0.07f, 16);
        boite(sel, translate({CX, Y - 0.375f, zFace + 0.40f}), 0.05f, 0.025f, 0.075f);
        prend(con, M_PLANCHE_BAS); prend(sel, M_ACCENT);
    }
    // montants de pare-brise, ciel de toit, rétroviseur intérieur
    {
        Geo mont, pav, ret, porte, glace;
        const vec3 bas[2] = {{CX - 0.72f, Yb, Zb}, {CX + 0.72f, Yb, Zb}};
        const vec3 haut[2] = {{CX - 0.62f, 0.40f, -0.42f}, {CX + 0.62f, 0.40f, -0.42f}};
        for (int k = 0; k < 2; k++) barre(mont, I, bas[k], haut[k], 0.075f, 0.05f);
        boite(pav, translate({CX, 0.43f, -0.28f}), 1.40f, 0.04f, 0.32f);
        // rétroviseur intérieur : 240 × 65 mm, en haut du pare-brise, devant le conducteur à droite (≈ 0,62 m de l'œil)
        const mat4 R = translate({CX - 0.04f, 0.20f, -0.62f}) * rotateY(0.22f) * rotateX(-0.10f);
        boite(ret, R, 0.24f, 0.065f, 0.025f);
        plan(glace, R * translate({0, 0, 0.0135f}), 0.226f, 0.052f);   // WFS-282 (RG-03) : glace côté conducteur, cadre noir de 7 mm
        boite(ret, translate({CX - 0.04f, 0.27f, -0.60f}), 0.022f, 0.08f, 0.02f);
        // contre-portes : panneau intérieur sous la vitre, accoudoir et poignée (place du conducteur et du passager)
        for (float sx : {-1.f, 1.f}) {
            const float x = CX + sx * 0.80f;
            boite(porte, translate({x, Y - 0.32f, -0.30f}), 0.06f, 0.62f, 1.30f);
            boite(porte, translate({x - sx * 0.04f, Y - 0.24f, -0.20f}), 0.06f, 0.05f, 0.45f);
        }
        prend(mont, M_PAVILLON); prend(pav, M_PAVILLON); prend(ret, M_LAQUE); prend(glace, M_GLACE); prend(porte, M_PLANCHE_BAS);
    }
    // écrans : fond du combiné, cadre et fond de l'écran central
    {
        Geo fc, cc, cadre, fe;
        plan(fc, versOeil(d.combinePos, 0), d.combineW, d.combineH);
        boite(cc, versOeil(d.combinePos, 0) * translate({0, 0, -0.008f}), d.combineW + 0.012f, d.combineH + 0.012f, 0.012f);
        if (combine) { prend(fc, M_FOND_COMBINE); prend(cc, M_LAQUE); } else { fc = Geo{}; cc = Geo{}; }
        if (d.ecranW > 0) {
            const mat4 E = versOeil(d.ecranPos, d.ecranLacet);
            boite(cadre, E * translate({0, 0, -0.010f}), d.ecranW + 0.016f, d.ecranH + 0.016f, 0.018f);
            plan(fe, E * translate({0, 0, 0.0005f}), d.ecranW, d.ecranH);
            prend(cadre, M_LAQUE);
            prend(fe, M_FOND_ECRAN);   // deux matériaux (jour, nuit) : même maillage dessiné avec l'un ou l'autre
        }
    }
    // volant (repère du volant : plan xy, +z vers le conducteur)
    {
        const float R = d.volantD * 0.5f - d.janteEp * 0.5f, rt = d.janteEp * 0.5f;
        Geo ja, br, co[3], rep, pal, man, mol;
        jante(ja, I, R, rt, d.meplat, 0, 2 * PI, 96, 14, d.meplatHaut);
        Geo mo;
        cylindre(mo, rotateX(PI / 2) * translate({0, -0.012f, 0}), 0.072f, 0.078f, 0.05f, 28);   // moyeu (coussin du sac gonflable)
        // branches : 3 (à 9 h, 3 h et 6 h) ou 2 (à 9 h et 3 h, SF90 : branches larges qui portent les commandes)
        const float hub = 0.07f;
        // WFS-202 : 4 branches (Range Rover Sport) : deux presque horizontales, deux en V vers le bas
        std::vector<float> angles = d.branches == 2 ? std::vector<float>{0.f, PI}
                                  : d.branches == 4 ? std::vector<float>{0.10f, PI - 0.10f, -PI / 2 + 0.45f, -PI / 2 - 0.45f}
                                                    : std::vector<float>{0.f, PI, -PI / 2};
        for (float a : angles) {
            const vec2 bout = ligneJante(a, R, d.meplat, d.meplatHaut);
            const float l = std::sqrt(bout.x * bout.x + bout.y * bout.y) - hub;
            const float large = d.branches == 2 ? 0.060f : d.branches == 4 ? (a < 0 ? 0.026f : 0.036f) : (a == -PI / 2 ? 0.032f : 0.040f);
            boite(br, rotateZ(a) * translate({hub + l * 0.5f, 0, -0.008f}), l, large, 0.016f);
        }
        // coutures colorées en bas de la jante (M4 : bleu clair, bleu foncé, rouge), de part et d'autre de la branche basse
        const bool coutures = d.couture[0] || d.couture[1] || d.couture[2];
        for (int k = 0; k < 3; k++) if (d.couture[k])
            for (float cote : {-1.f, 1.f}) {
                const float a = -PI / 2 + cote * (0.11f + 0.05f * k);
                jante(co[k], translate({0, 0, 0.0005f}), R, rt * 1.03f, d.meplat, a - 0.024f, a + 0.024f, 3, 10);
            }
        // repère : M4 (boutons M1 / M2 rouges sur les branches), Golf (agrafe GTI rouge sur la branche basse), SF90 (rouge à midi)
        if (d.repereCol) {
            if (coutures || d.boutonsBranches) for (float sx : {-1.f, 1.f}) boite(rep, translate({sx * 0.105f, 0.018f, 0.012f}), 0.022f, 0.014f, 0.008f);
            else if (d.manettino || d.repereHaut) boite(rep, translate({0, R + rt * 0.2f, 0.004f}), 0.012f, rt * 2.1f, rt * 2.1f);
            else if (!d.demarreur) boite(rep, translate({0, -R * 0.55f, 0.010f}), 0.045f, 0.012f, 0.004f);   // Giulia : le rouge est le démarreur
        }
        if (d.palettes) for (float sx : {-1.f, 1.f}) boite(pal, translate({sx * (R * 0.80f), 0.02f, -0.045f}) * rotateZ(sx * 0.15f), 0.055f, 0.13f, 0.006f);
        // manettino en bas à droite, démarreur en bas à gauche (sur les branches) ; WFS-201 : démarreur seul (Giulia)
        if (d.manettino) cylindre(man, translate({0.085f, -0.055f, 0.012f}) * rotateX(PI / 2), 0.022f, 0.022f, 0.016f, 20);
        if (d.demarreur) cylindre(man, translate({-0.085f, -0.055f, 0.010f}) * rotateX(PI / 2), 0.016f, 0.016f, 0.012f, 20);
        // WFS-201 : molettes (Model 3) : deux roulettes de commande sur les branches horizontales, caoutchouc sombre
        if (d.molettes) for (float sx : {-1.f, 1.f}) cylindre(mol, translate({sx * 0.105f, 0.f, 0.014f}) * rotateZ(PI / 2), 0.011f, 0.011f, 0.010f, 16);
        prend(ja, M_CUIR, true); prend(mo, M_MOYEU, true); prend(br, M_BRANCHES, true);
        for (int k = 0; k < 3; k++) prend(co[k], M_COUTURE0 + k, true);
        prend(rep, M_REPERE, true); prend(pal, d.carbone ? M_ACCENT : M_BRANCHES, d.palettesColonne ? 2 : 1); prend(man, M_REPERE, true);
        prend(mol, M_LAMES, true);
    }
    // essuie-glaces : bras + balai, dans le repère du balai (x le long du bras, y vers le haut du pare-brise)
    {
        Material m; m.color = {0.012f, 0.012f, 0.012f, 1}; m.rough = 0.55f; m.metal = 0; brasMat_ = r.makeMaterial(deuxFaces(m), nullptr, nullptr, nullptr);
        brasNuit_ = brasMat_; brasNuit_.emissive = vec3{m.color.x, m.color.y, m.color.z} * AMBIANCE_NUIT;   // WFS-282 (RG-04)
        for (int k = 0; k < 2; k++) {
            const float L = k == 0 ? d.balaiG : d.balaiD;
            if (L <= 0) continue;
            Geo g;
            boite(g, translate({L * 0.45f, 0, 0.012f}), L * 0.9f, 0.014f, 0.010f);   // bras
            boite(g, translate({L * 0.5f, 0, 0.004f}), L, 0.010f, 0.012f);           // balai (caoutchouc)
            bras_[k] = r.makeMesh(g.v, g.i); octets_ += g.v.size() * sizeof(Vertex) + g.i.size() * 4;
        }
    }
    // écrans dynamiques (capacité fixe : aucune allocation par image)
    for (int k = 0; k < D_N; k++) {
        dv_[k].assign(DYN_SOMMETS, Vertex{}); di_[k].assign(DYN_SOMMETS / 4 * 6, 0);
        dyn_[k] = r.makeMesh(dv_[k], di_[k], true);
        octets_ += DYN_SOMMETS * sizeof(Vertex) + DYN_SOMMETS / 4 * 6 * 4;
        dv_[k].clear(); di_[k].clear();
    }
    auto emis = [&](unsigned c, float f) { Material m; m.color = {0, 0, 0, 1}; m.rough = 0.2f; m.emissive = lin(c) * f; return r.makeMaterial(deuxFaces(m), nullptr, nullptr, nullptr); };
    const unsigned cols[D_N] = {d.combineActif, 0x2c3440, d.combineAlerte, 0xc8d0da, 0xff6a00};
    for (int k = 0; k < D_N; k++) { dynMat_[k] = emis(cols[k], 1.0f); dynNuit_[k] = emis(cols[k], 0.45f); }   // nuit : moins lumineux, jamais éblouissant
    { Material c; c.color = {0, 0, 0, 1}; c.rough = 1; c.emissive = {16, 0, 16}; cleMat_ = r.makeMaterial(deuxFaces(c), nullptr, nullptr, nullptr); }   // banc (WFS-235, RG-05)
    WFS_LOG("cockpit (WFS-197) : intérieur %s construit : %zu pièces, %.2f Mo", d.car, pieces_.size(), octets_ / 1048576.0);
    return true;
}

void Interieur::detruire(Renderer& r) {
    if (!def_ && pieces_.empty()) return;
    r.vk().waitIdle();
    for (Piece& p : pieces_) r.destroy(p.m);
    pieces_.clear();
    for (Material& m : mats_) r.destroy(m);
    mats_.clear(); matsNuit_.clear();   // copies de nuit : même jeu de descripteurs (rien à détruire)
    for (Mesh& m : bras_) if (m.indexCount) r.destroy(m);
    if (brasMat_.set) r.destroy(brasMat_);
    for (int k = 0; k < D_N; k++) { r.destroy(dyn_[k]); r.destroy(dynMat_[k]); r.destroy(dynNuit_[k]); dyn_[k] = {}; dynMat_[k] = dynNuit_[k] = {}; }
    for (Mesh& m : bras_) m = {};
    brasMat_ = {}; brasNuit_ = {};
    if (cleMat_.set) r.destroy(cleMat_);
    cleMat_ = {};
    if (grain_.view) r.vk().destroy(grain_);
    grain_ = {};
    def_ = nullptr; octets_ = 0;
}

// ---------------------------------------------------------------- dessin
void Interieur::dessinerCombine(Renderer& r, const mat4& cam, const Valeurs& val) {
    const InterieurDef& d = *def_;
    for (int k : {D_ACTIF, D_FOND, D_ALERTE}) { dv_[k].clear(); di_[k].clear(); }
    Plan2D A{&dv_[D_ACTIF], &di_[D_ACTIF], 0.001f}, F{&dv_[D_FOND], &di_[D_FOND], 0.0008f}, R{&dv_[D_ALERTE], &di_[D_ALERTE], 0.001f};
    const bool surEcran = d.combine == Combine::Aucun;   // WFS-201 : Model 3 : bande de gauche (34 %) de l'écran central
    const float W = surEcran ? d.ecranW * 0.34f : d.combineW, H = surEcran ? d.ecranH * 0.90f : d.combineH, kmh = (float)val.kmh;
    const float vit = std::clamp(kmh / 320.f, 0.f, 1.f), reg = std::clamp(val.regime, 0.f, 1.f), nit = std::clamp(val.nitro, 0.f, 1.f);
    auto barreNitro = [&](float x0, float y0, float x1, float y1) {   // jauge de nitro horizontale
        F.rect(x0, y0, x1, y1); A.rect(x0, y0, x0 + (x1 - x0) * nit, y1);
    };
    switch (d.combine) {
    case Combine::VwDigital: {   // deux cadrans ronds (vitesse à gauche, régime à droite), vitesse et rapport au centre
        const float rc = H * 0.44f;
        arcBarres(A, F, R, {-W * 0.30f, 0}, rc * 0.78f, rc, PI * 1.25f, -PI * 0.25f, 40, vit, 2.f, 0.0028f);
        arcBarres(A, F, R, {W * 0.30f, 0}, rc * 0.78f, rc, PI * 1.25f, -PI * 0.25f, 40, reg, 0.85f, 0.0028f);
        nombre(A, 0.018f, -H * 0.05f, H * 0.30f, (int)kmh, 3);
        rapport(A, W * 0.30f - H * 0.08f, -H * 0.12f, H * 0.26f, val.rapport);
        barreNitro(-W * 0.10f, -H * 0.38f, W * 0.10f, -H * 0.33f);
        break;
    }
    case Combine::BmwLive: {   // vitesse en chiffres à gauche, régime en barres qui montent vers la droite, rapport au centre
        nombre(A, -W * 0.18f, -H * 0.10f, H * 0.34f, (int)kmh, 3);
        const int n = 34;
        for (int k = 0; k < n; k++) {
            const float f = (k + 0.5f) / n, x = W * (0.02f + 0.44f * f), h = H * (0.12f + 0.55f * f * f);
            Plan2D& c = f <= reg ? (f >= 0.82f ? R : A) : F;
            c.rect(x, -H * 0.30f, x + W * 0.44f / n * 0.6f, -H * 0.30f + h);
        }
        rapport(A, -W * 0.07f, -H * 0.12f, H * 0.30f, val.rapport);
        barreNitro(-W * 0.44f, -H * 0.38f, -W * 0.22f, -H * 0.33f);
        break;
    }
    case Combine::Porsche5: {   // 5 cadrans : compte-tours analogique au centre (vitesse numérique dedans), 4 autour
        const float rc = H * 0.47f, rs = H * 0.33f;
        F.trait({0, 0}, 0, rc * 0.96f, rc, 0.001f);
        arcBarres(F, F, F, {0, 0}, rc * 0.86f, rc, PI * 1.25f, -PI * 0.25f, 9, 1.f, 2.f, 0.0022f);   // graduations
        arcBarres(R, R, R, {0, 0}, rc * 0.86f, rc, PI * 1.25f - PI * 1.5f * 0.84f, -PI * 0.25f, 3, 1.f, 2.f, 0.0022f);   // zone rouge
        const float aig = PI * 1.25f - PI * 1.5f * reg;
        A.trait({0, 0}, aig, -rc * 0.12f, rc * 0.92f, 0.0026f);
        nombre(A, H * 0.17f, -rc * 0.60f, H * 0.17f, (int)kmh, 3);
        rapport(A, -H * 0.04f, rc * 0.18f, H * 0.16f, val.rapport);
        for (int k : {-2, -1, 1, 2}) {   // cadrans latéraux : vitesse (à gauche du centre), nitro (à droite), deux écrans
            const vec2 c{k * W * 0.205f, -H * 0.04f};
            arcBarres(F, F, F, c, rs * 0.88f, rs, PI * 1.25f, -PI * 0.25f, 24, 1.f, 2.f, 0.0018f);
            if (k == -1) A.trait(c, PI * 1.25f - PI * 1.5f * vit, -rs * 0.1f, rs * 0.85f, 0.0020f);
            if (k == 1) arcBarres(A, F, R, c, rs * 0.70f, rs * 0.86f, PI * 1.25f, -PI * 0.25f, 24, nit, 2.f, 0.0018f);
        }
        break;
    }
    case Combine::FerrariIncurve: {   // écran large : compte-tours circulaire au centre (rouge en haut), vitesse et rapport dedans
        const float rc = H * 0.46f;
        arcBarres(A, F, R, {0, 0}, rc * 0.80f, rc, PI * 1.30f, -PI * 0.30f, 52, reg, 0.80f, 0.0026f);
        rapport(A, -H * 0.10f, -H * 0.05f, H * 0.34f, val.rapport);
        nombre(A, H * 0.22f, -rc * 0.80f, H * 0.14f, (int)kmh, 3);
        barreNitro(-W * 0.44f, -H * 0.30f, -W * 0.24f, -H * 0.24f);   // panneau gauche (véhicule) ; la carte est à droite
        break;
    }
    case Combine::AudiVirtual: {   // Audi virtual cockpit, vue classique : vitesse à gauche, régime à droite (aiguilles), rapport au centre
        const float rc = H * 0.46f;
        cadran(F, A, R, {-W * 0.29f, 0}, rc, vit, 2.f, 26, 0.0022f);
        cadran(F, A, R, {W * 0.29f, 0}, rc, reg, 0.85f, 26, 0.0022f);
        rapport(A, -H * 0.07f, H * 0.02f, H * 0.26f, val.rapport);
        nombre(A, H * 0.30f, -H * 0.36f, H * 0.16f, (int)kmh, 3);
        barreNitro(-W * 0.10f, -H * 0.44f, W * 0.10f, -H * 0.40f);
        break;
    }
    case Combine::AlfaCadrans: {   // Giulia QV : deux cadrans à aiguille dans leurs tunnels, petit écran au centre (rapport, vitesse)
        const float rc = H * 0.46f;
        cadran(F, A, R, {-W * 0.30f, 0}, rc, vit, 2.f, 24, 0.0024f);
        cadran(F, A, R, {W * 0.30f, 0}, rc, reg, 0.82f, 24, 0.0024f);
        F.rect(-W * 0.12f, -H * 0.40f, W * 0.12f, H * 0.40f);   // écran central du combiné
        rapport(A, -H * 0.08f, -H * 0.02f, H * 0.28f, val.rapport);
        nombre(A, W * 0.10f, -H * 0.34f, H * 0.14f, (int)kmh, 3);
        break;
    }
    case Combine::PeugeotICockpit: {   // i-Cockpit 3D (208), vu au-dessus du volant : régime en barre en haut, vitesse et rapport au centre
        const int n = 30;
        for (int k = 0; k < n; k++) {
            const float f = (k + 0.5f) / n, x = -W * 0.42f + W * 0.84f * f;
            Plan2D& c = f <= reg ? (f >= 0.84f ? R : A) : F;
            c.rect(x, H * 0.22f, x + W * 0.84f / n * 0.62f, H * 0.40f);
        }
        nombre(A, H * 0.30f, -H * 0.30f, H * 0.42f, (int)kmh, 3);
        rapport(A, W * 0.28f, -H * 0.22f, H * 0.30f, val.rapport);
        barreNitro(-W * 0.42f, -H * 0.36f, -W * 0.20f, -H * 0.30f);
        break;
    }
    case Combine::LamboCompteTours: {   // Gallardo : grand compte-tours au centre (rapport numérique dedans), vitesse à droite, jauges à gauche
        const float rc = H * 0.48f, rs = H * 0.30f;
        cadran(F, A, R, {0, 0}, rc, reg, 0.86f, 30, 0.0026f);
        rapport(A, -H * 0.06f, -rc * 0.62f, H * 0.20f, val.rapport);
        cadran(F, A, R, {W * 0.33f, -H * 0.06f}, rs, vit, 2.f, 18, 0.0018f);
        nombre(A, W * 0.33f + H * 0.14f, -H * 0.42f, H * 0.11f, (int)kmh, 3);
        cadran(F, A, R, {-W * 0.33f, -H * 0.06f}, rs, nit, 2.f, 18, 0.0018f);   // jauge : réserve de nitro
        break;
    }
    case Combine::FordDigital: {   // Shelby GT500 : compte-tours en arc sur toute la largeur, vitesse au centre, rapport à droite
        const vec2 c{0, -H * 0.62f};
        arcBarres(A, F, R, c, H * 0.90f, H * 1.05f, PI * 0.88f, PI * 0.12f, 46, reg, 0.86f, 0.0030f);
        nombre(A, H * 0.32f, -H * 0.30f, H * 0.40f, (int)kmh, 3);
        rapport(A, W * 0.30f, -H * 0.24f, H * 0.30f, val.rapport);
        barreNitro(-W * 0.44f, -H * 0.40f, -W * 0.24f, -H * 0.34f);
        break;
    }
    case Combine::Aucun: {   // Model 3 : pas de combiné ; bande de gauche de l'écran central : vitesse, « D » / « R », nitro
        nombre(A, W * 0.30f, H * 0.02f, H * 0.32f, (int)kmh, 3);
        chiffre(A, -W * 0.38f, H * 0.06f, H * 0.22f, val.rapport < 0 ? SEG_R : val.kmh < 1 ? SEG_N : SEG_D);
        barreNitro(-W * 0.38f, -H * 0.30f, W * 0.30f, -H * 0.25f);
        break;
    }
    }
    const mat4 M = surEcran ? cam * versOeil(d.ecranPos, d.ecranLacet) * translate({-d.ecranW * 0.5f + W * 0.5f + 0.008f, 0, 0.0004f})
                            : cam * versOeil(d.combinePos, 0);
    for (int k : {D_FOND, D_ACTIF, D_ALERTE}) {
        if (di_[k].empty()) continue;
        r.updateMesh(dyn_[k], dv_[k], di_[k]);
        r.draw(dyn_[k], mat(val.nuit ? dynNuit_[k] : dynMat_[k]), M); appels_++;
    }
}

void Interieur::tailleCarte(float& w, float& h) const {
    if (!def_) { w = h = 0; return; }
    if (def_->combine == Combine::Aucun) { w = def_->ecranW * 0.62f; h = def_->ecranH * 0.90f; return; }   // WFS-201 : Model 3
    if (def_->ecranW > 0) { w = def_->ecranW; h = def_->ecranH; return; }
    w = def_->combineW / 3 * 0.92f; h = def_->combineH * 0.86f;   // SF90 : tiers droit du combiné
}

void Interieur::dessinerCarte(Renderer& r, const mat4& cam, const std::vector<vec2>& route, vec2, double, bool nuit) {
    const InterieurDef& d = *def_;
    for (int k : {D_ROUTE, D_FLECHE}) { dv_[k].clear(); di_[k].clear(); }
    Plan2D Ro{&dv_[D_ROUTE], &di_[D_ROUTE], 0.0012f}, Fl{&dv_[D_FLECHE], &di_[D_FLECHE], 0.0016f};
    float W, H; tailleCarte(W, H);
    // `route` : segments déjà dans le repère de l'écran (paires de points, de −W/2 à W/2 et de −H/2 à H/2, découpés)
    for (size_t k = 0; k + 1 < route.size(); k += 2) Ro.segment(route[k], route[k + 1], H * 0.035f);
    // flèche du joueur : pointe vers le haut (devant), à CARTE_JOUEUR_Y de la hauteur
    const float y = -H / 2 + H * CARTE_JOUEUR_Y, s = H * 0.07f;
    Fl.quad({-s * 0.6f, y - s * 0.55f}, {0, y - s * 0.25f}, {s * 0.6f, y - s * 0.55f}, {0, y + s * 0.65f});
    (void)W;
    const mat4 M = d.combine == Combine::Aucun   // WFS-201 : Model 3 : partie droite de l'écran central
                       ? cam * versOeil(d.ecranPos, d.ecranLacet) * translate({d.ecranW * 0.5f - W * 0.5f - 0.008f, 0, 0.0004f})
                 : d.ecranW > 0 ? cam * versOeil(d.ecranPos, d.ecranLacet)
                                : cam * versOeil(d.combinePos, 0) * translate({d.combineW / 3, 0, 0.0004f});   // SF90 : tiers droit du combiné
    for (int k : {D_ROUTE, D_FLECHE}) {
        if (di_[k].empty()) continue;
        r.updateMesh(dyn_[k], dv_[k], di_[k]);
        r.draw(dyn_[k], mat(nuit ? dynNuit_[k] : dynMat_[k]), M); appels_++;
    }
}

void Interieur::dessiner(Renderer& r, const mat4& cam, const EtatInterieur& e, const std::vector<vec2>& route, vec2 joueur, double cap) {
    if (!def_) return;
    const InterieurDef& d = *def_;
    appels_ = 0;
    const mat4 colonne = cam * translate(d.volantPos) * rotateX(d.volantIncl), vol = colonne * rotateZ(e.volant);
    for (const Piece& p : pieces_) {
        int m = p.mat;
        if (m == M_FOND_ECRAN && e.v.nuit) m = M_FOND_ECRAN_NUIT;
        r.draw(p.m, mat((e.v.nuit && m < (int)matsNuit_.size() ? matsNuit_ : mats_)[(size_t)m]), p.attache == 1 ? vol : p.attache == 2 ? colonne : cam); appels_++;
    }
    // essuie-glaces : pivots au bas du pare-brise ; plan du pare-brise (axe x, axe « haut » le long des montants)
    {
        const vec3 base{CX, d.pareBriseY - 0.01f, d.pareBriseZ}, haut{CX, 0.40f, -0.42f};
        const vec3 ex{1, 0, 0}, eu = normalize(haut - base), en = cross(ex, eu);
        auto repere = [&](vec3 o, float sens) {   // sens +1 : repos vers la droite, tourne vers le haut ; −1 : en miroir
            mat4 B = mat4::identity();
            const vec3 x = ex * sens;
            B.at(0, 0) = x.x; B.at(1, 0) = x.y; B.at(2, 0) = x.z;
            B.at(0, 1) = eu.x; B.at(1, 1) = eu.y; B.at(2, 1) = eu.z;
            B.at(0, 2) = en.x * sens; B.at(1, 2) = en.y * sens; B.at(2, 2) = en.z * sens;
            return translate(o) * B;
        };
        switch (d.essuie) {
        case Essuie::Paralleles:
            if (bras_[0].indexCount) { r.draw(bras_[0], mat(e.v.nuit ? brasNuit_ : brasMat_), cam * repere(base + vec3{-0.62f, 0, 0}, 1) * rotateZ(e.essuie)); appels_++; }
            if (bras_[1].indexCount) { r.draw(bras_[1], mat(e.v.nuit ? brasNuit_ : brasMat_), cam * repere(base + vec3{0.08f, 0, 0}, 1) * rotateZ(e.essuie)); appels_++; }
            break;
        case Essuie::Opposition:
            if (bras_[0].indexCount) { r.draw(bras_[0], mat(e.v.nuit ? brasNuit_ : brasMat_), cam * repere(base + vec3{-0.62f, 0, 0}, 1) * rotateZ(e.essuie)); appels_++; }
            if (bras_[1].indexCount) { r.draw(bras_[1], mat(e.v.nuit ? brasNuit_ : brasMat_), cam * repere(base + vec3{0.62f, 0, 0}, -1) * rotateZ(e.essuie)); appels_++; }
            break;
        case Essuie::Unique:
            if (bras_[0].indexCount) { r.draw(bras_[0], mat(e.v.nuit ? brasNuit_ : brasMat_), cam * repere(base + vec3{-0.02f, 0, 0}, -1) * rotateZ(e.essuie)); appels_++; }
            break;
        }
    }
    dessinerCombine(r, cam, e.v);
    dessinerCarte(r, cam, route, joueur, cap, e.v.nuit);
}

} // namespace wfs::cockpit
