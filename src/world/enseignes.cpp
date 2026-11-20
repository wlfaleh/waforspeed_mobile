// WaForSpeed — ville vivante (WFS-71) : outils partagés (enseignes.h) et panneaux néon de la route (DECOR.board du jeu web).
#include "enseignes.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "../../third_party/stb/stb_truetype.h"   // implémentation dans src/ui/text.cpp
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/trace_cartes.h"   // WFS-139 (accroche)
#include "../game/track.h"
#include "ville_quartiers.h"
#include "zones.h"
#include "mobilier_ville.h"   // WFS-568 : pavés chanfreinés, pièces tournées
#include "../render/eclairage_unites.h"   // WFS-568 : couleur du rétroéclairage

namespace wfs {
namespace vie {

vec3 lin(unsigned h) {
    auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); };
    return {c(16), c(8), c(0)};
}

Repere repere(Track& t, double s) {
    TrackPoint p = t.point(s, 0);
    return {{(float)p.x, (float)p.y, (float)p.z}, {(float)std::cos(p.th), 0, (float)std::sin(p.th)}, {(float)std::sin(p.th), 0, (float)-std::cos(p.th)}};
}

// ---------------------------------------------------------------- géométrie
static void quad(Geo& g, vec3 a, vec3 b, vec3 c, vec3 d, vec3 n, vec2 ua, vec2 ub, vec2 uc, vec2 ud) {
    const uint32_t k = (uint32_t)g.v.size();
    for (auto [p, u] : {std::pair{a, ua}, {b, ub}, {c, uc}, {d, ud}}) g.v.push_back({p.x, p.y, p.z, n.x, n.y, n.z, u.x, u.y});
    g.i.insert(g.i.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
}
void boite(Geo& g, vec3 c, vec3 r, vec3 f, float w, float h, float d, vec2 uv) {
    const vec3 up{0, 1, 0};
    const float hx = w / 2, hz = d / 2;
    auto P = [&](float x, float y, float z) { return c + r * x + up * y + f * z; };
    struct S { float x0, z0, x1, z1; vec3 n; } cotes[4] = {{-hx, -hz, hx, -hz, -f}, {hx, -hz, hx, hz, r}, {hx, hz, -hx, hz, f}, {-hx, hz, -hx, -hz, -r}};
    for (auto& s : cotes) quad(g, P(s.x0, 0, s.z0), P(s.x1, 0, s.z1), P(s.x1, h, s.z1), P(s.x0, h, s.z0), s.n, uv, uv, uv, uv);
    quad(g, P(-hx, h, -hz), P(hx, h, -hz), P(hx, h, hz), P(-hx, h, hz), up, uv, uv, uv, uv);
}
void cylindre(Geo& g, vec3 c, float rayon, float h, vec2 uv, int n) {
    for (int k = 0; k < n; k++) {
        const float a0 = 2 * PI * k / n, a1 = 2 * PI * (k + 1) / n;
        const vec3 d0{std::cos(a0), 0, std::sin(a0)}, d1{std::cos(a1), 0, std::sin(a1)};
        quad(g, c + d0 * rayon, c + d1 * rayon, c + d1 * rayon + vec3{0, h, 0}, c + d0 * rayon + vec3{0, h, 0}, normalize(d0 + d1), uv, uv, uv, uv);
        const uint32_t b = (uint32_t)g.v.size();   // dessus
        for (vec3 p : {c + vec3{0, h, 0}, c + d1 * rayon + vec3{0, h, 0}, c + d0 * rayon + vec3{0, h, 0}}) g.v.push_back({p.x, p.y, p.z, 0, 1, 0, uv.x, uv.y});
        g.i.insert(g.i.end(), {b, b + 1, b + 2});
    }
}
void cone(Geo& g, vec3 c, float rayon, float h, vec2 uv, int n) {
    const vec3 sommet = c + vec3{0, h, 0};
    for (int k = 0; k < n; k++) {
        const float a0 = 2 * PI * k / n, a1 = 2 * PI * (k + 1) / n;
        const vec3 d0{std::cos(a0), 0, std::sin(a0)}, d1{std::cos(a1), 0, std::sin(a1)};
        const vec3 nm = normalize(normalize(d0 + d1) * h + vec3{0, rayon, 0});
        const uint32_t b = (uint32_t)g.v.size();
        for (vec3 p : {c + d0 * rayon, sommet, c + d1 * rayon}) g.v.push_back({p.x, p.y, p.z, nm.x, nm.y, nm.z, uv.x, uv.y});
        g.i.insert(g.i.end(), {b, b + 1, b + 2});
        const uint32_t d = (uint32_t)g.v.size();   // dessous (ombre du parasol)
        for (vec3 p : {c + d0 * rayon, c + d1 * rayon, c}) g.v.push_back({p.x, p.y, p.z, 0, -1, 0, uv.x, uv.y});
        g.i.insert(g.i.end(), {d, d + 1, d + 2});
    }
}
void plan(Geo& g, vec3 c, vec3 x, vec3 y, vec3 n, float w, float h, float u0, float v0, float u1, float v1) {
    const vec3 hx = x * (w / 2), hy = y * (h / 2);
    quad(g, c - hx - hy, c + hx - hy, c + hx + hy, c - hx + hy, n, {u0, v1}, {u1, v1}, {u1, v0}, {u0, v0});
}

// ---------------------------------------------------------------- palette (cases de 8 × 8 texels, 8 × 8 cases)
vec2 Palette::uv(unsigned hex) {
    auto it = std::find(couleurs_.begin(), couleurs_.end(), hex);
    int i = (int)(it - couleurs_.begin());
    if (it == couleurs_.end()) { if (couleurs_.size() >= 64) i = 0; else { couleurs_.push_back(hex); i = (int)couleurs_.size() - 1; } }
    return {((i % 8) + 0.5f) / 8.f, ((i / 8) + 0.5f) / 8.f};
}
bool Palette::construire(Renderer& r) {
    std::vector<uint8_t> px(64 * 64 * 4, 255);
    for (int i = 0; i < 64; i++) {
        const unsigned h = i < (int)couleurs_.size() ? couleurs_[(size_t)i] : 0x808080;
        for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) {
            uint8_t* p = &px[((size_t)((i / 8) * 8 + y) * 64 + (i % 8) * 8 + x) * 4];
            p[0] = (h >> 16) & 255; p[1] = (h >> 8) & 255; p[2] = h & 255; p[3] = 255;
        }
    }
    img_ = r.vk().uploadTexture(px.data(), 64, 64, true);
    Material m; m.rough = 0.7f; m.metal = 0.f; m.flags = MAT_DOUBLE;
    materiau = r.makeMaterial(m, &img_, nullptr, nullptr);
    return img_.img != VK_NULL_HANDLE;
}
// WFS-568 : autre matériau sur la même texture de palette (métal, plexiglas, rétroéclairage) : la texture reste unique
Material Palette::variante(Renderer& r, const Material& m) { return r.makeMaterial(m, &img_, nullptr, nullptr); }
void Palette::detruire(Renderer& r) { r.destroy(materiau); if (img_.img) r.vk().destroy(img_); img_ = Image{}; couleurs_.clear(); }   // WFS-187 (accroche) : jeux de descripteurs rendus

Material neon(Renderer& r, unsigned hex, float emission) {
    Material m; const vec3 c = lin(hex);
    m.color = {c.x, c.y, c.z, 1}; m.rough = 0.4f; m.emissive = c * emission; m.flags = MAT_DOUBLE;
    return r.makeMaterial(m, nullptr, nullptr, nullptr);
}

// ---------------------------------------------------------------- atlas de textes
namespace {
constexpr int AW = 2048, LIGNE = 96, PX = 72;   // 2 colonnes de 1024 px ; hauteur selon le nombre de textes
std::vector<uint32_t> codepoints(const std::string& s) {
    std::vector<uint32_t> out;
    for (size_t i = 0; i < s.size();) {
        const unsigned char c = (unsigned char)s[i];
        uint32_t cp = c; int n = 1;
        if (c >= 0xF0) { cp = c & 7; n = 4; } else if (c >= 0xE0) { cp = c & 15; n = 3; } else if (c >= 0xC0) { cp = c & 31; n = 2; }
        for (int k = 1; k < n && i + k < s.size(); k++) cp = (cp << 6) | ((unsigned char)s[i + k] & 63);
        out.push_back(cp); i += (size_t)n;
    }
    return out;
}
} // namespace

void AtlasTextes::ajouter(const std::string& t, unsigned couleur) {
    if (std::find(textes_.begin(), textes_.end(), t) == textes_.end()) textes_.push_back(t);
    couleurs_[t] = couleur;   // WFS-568 : couleur des lettres (blanc par défaut : rendu d'avant inchangé)
}

bool AtlasTextes::construire(Renderer& r) {
    std::vector<uint8_t> ttf;
    stbtt_fontinfo info;
    if (!readAsset("fonts/Rajdhani-Bold.ttf", ttf) || !stbtt_InitFont(&info, ttf.data(), 0)) { WFS_ERR("vie : police introuvable"); return false; }
    const int lignes = std::max(1, (int)(textes_.size() + 1) / 2), AH = lignes * LIGNE;
    std::vector<uint8_t> px((size_t)AW * AH * 4, 0);
    for (size_t i = 0; i < (size_t)AW * AH; i++) { px[i * 4] = px[i * 4 + 1] = px[i * 4 + 2] = 255; }
    const float sc = stbtt_ScaleForPixelHeight(&info, (float)PX);
    int asc, desc, gap; stbtt_GetFontVMetrics(&info, &asc, &desc, &gap);
    for (size_t t = 0; t < textes_.size() && t < (size_t)(2 * lignes); t++) {
        const int col = (int)t / lignes, row = (int)t % lignes, x0 = col * (AW / 2) + 8, y0 = row * LIGNE;
        const auto cps = codepoints(textes_[t]);
        float x = (float)x0;
        const int base = y0 + (LIGNE - PX) / 2 + (int)(asc * sc);
        for (size_t k = 0; k < cps.size(); k++) {
            int adv, lsb; stbtt_GetCodepointHMetrics(&info, (int)cps[k], &adv, &lsb);
            int bx0, by0, bx1, by1; stbtt_GetCodepointBitmapBox(&info, (int)cps[k], sc, sc, &bx0, &by0, &bx1, &by1);
            const int w = bx1 - bx0, h = by1 - by0;
            if (w > 0 && h > 0 && x + bx0 + w < col * (AW / 2) + AW / 2 - 8) {
                std::vector<uint8_t> bm((size_t)w * h);
                stbtt_MakeCodepointBitmap(&info, bm.data(), w, h, w, sc, sc, (int)cps[k]);
                for (int yy = 0; yy < h; yy++) for (int xx = 0; xx < w; xx++) {
                    const int X = (int)x + bx0 + xx, Y = base + by0 + yy;
                    if (X < 0 || Y < 0 || X >= AW || Y >= AH) continue;
                    uint8_t& a = px[((size_t)Y * AW + X) * 4 + 3];
                    a = std::max(a, bm[(size_t)yy * w + xx]);
                }
            }
            x += adv * sc + 1.5f;   // léger espacement des lettres (enseignes)
            if (k + 1 < cps.size()) x += stbtt_GetCodepointKernAdvance(&info, (int)cps[k], (int)cps[k + 1]) * sc;
        }
        Texte T;
        T.u0 = (x0 - 4.f) / AW; T.u1 = (x + 4.f) / AW; T.v0 = (float)y0 / AH; T.v1 = (float)(y0 + LIGNE) / AH;
        T.aspect = (x - x0 + 8.f) / LIGNE; T.ok = true;
        places_[textes_[t]] = T;
    }
    // WFS-568 : lettres de couleur (sRGB, comme la palette) ; le blanc d'avant reste 1,0 (rendu inchangé pour les textes blancs).
    // Autour des lettres, la couleur du texte déborde dans les texels transparents (sinon le filtrage foncerait les bords).
    for (size_t t = 0; t < textes_.size() && t < (size_t)(2 * lignes); t++) {
        const int col = (int)t / lignes, row = (int)t % lignes;
        const unsigned coul = couleurs_.count(textes_[t]) ? couleurs_[textes_[t]] : 0xFFFFFFu;
        for (int Y = row * LIGNE; Y < (row + 1) * LIGNE && Y < AH; Y++) for (int X = col * (AW / 2); X < (col + 1) * (AW / 2); X++) {
            uint8_t* q = &px[((size_t)Y * AW + X) * 4];
            q[0] = (uint8_t)(coul >> 16); q[1] = (uint8_t)(coul >> 8); q[2] = (uint8_t)coul;
        }
    }
    img_ = r.vk().essayerUploadTexture(px.data(), AW, AH, true);   // WFS-446 (RG-04) : voie de repli de WFS-418
    if (!img_.img) { WFS_WARN("vie : mémoire graphique insuffisante : atlas des enseignes refusé, enseignes sans texte (WFS-446)"); places_.clear(); return false; }
    WFS_LOG("vie : %zu textes d'enseignes dans l'atlas", places_.size());
    return true;
}
const Texte& AtlasTextes::texte(const std::string& t) const {
    static const Texte vide;
    auto it = places_.find(t);
    return it == places_.end() ? vide : it->second;
}
const Material& AtlasTextes::materiau(Renderer& r, unsigned hex, float emission, float seuil) {
    const Cle cle{hex, emission, seuil, 0.5f, false};   // WFS-568 : une couleur peut servir avec deux émissions (jour, nuit)
    auto it = mats_.find(cle);
    if (it != mats_.end()) return it->second;
    Material m; const vec3 c = lin(hex);
    m.color = {c.x, c.y, c.z, 1}; m.rough = 0.5f; m.emissive = c * emission; m.cutoff = seuil; m.flags = MAT_MASK | MAT_DOUBLE;   // WFS-94 : seuil réglable (circuit)
    return mats_[cle] = r.makeMaterial(m, &img_, nullptr, nullptr);
}
const Material& AtlasTextes::materiauCouleurs(Renderer& r, float emission, float seuil, float rugosite) {
    const Cle cle{0xFFFFFFu, emission, seuil, rugosite, true};
    auto it = mats_.find(cle);
    if (it != mats_.end()) return it->second;
    // WFS-568 : lettres de la couleur de l'atlas (couleur blanche du matériau) ; émission × couleur de la texture (MAT_EMTEX)
    Material m; m.rough = rugosite; m.emissive = vec3{1, 1, 1} * emission; m.cutoff = seuil; m.flags = MAT_MASK | MAT_DOUBLE | MAT_EMTEX;
    return mats_[cle] = r.makeMaterial(m, &img_, nullptr, nullptr);
}
void AtlasTextes::detruire(Renderer& r) { for (auto& [h, m] : mats_) r.destroy(m); if (img_.img) r.vk().destroy(img_); img_ = Image{}; mats_.clear(); places_.clear(); textes_.clear(); couleurs_.clear(); }   // WFS-187 (accroche) : jeux de descripteurs rendus

} // namespace vie

// ---------------------------------------------------------------- enseignes sur mâts le long de la route (DECOR.board)
// WFS-568 (ANO-316, demande de l'utilisateur du 2026-11-18) : plus de panneau néon cyan sur deux poteaux carrés. Chaque
// panneau est un vrai caisson d'enseigne double face sur deux mâts, modélisé pièce par pièce (aucune forme nue, règle du
// 2026-11-09) aux mesures courantes de ces ouvrages :
//   - massif de béton affleurant (0,70 × 0,70 m, 10 cm hors sol, arêtes chanfreinées), platine d'acier galvanisé
//     450 × 450 × 25 mm et 4 tiges d'ancrage M24 (rondelle, écrou hexagonal de 36 mm sur plats, bout fileté) ;
//   - mât en tube d'acier Ø 273,0 mm (série EN 10219 ; avis de l'agent 3D, PR #437), thermolaqué gris anthracite RAL 7016, fourreau soudé au pied ;
//     selle de fixation (plat de 30 mm, 4 boulons) sous le caisson ;
//   - caisson en profilé d'aluminium anodisé de 8,40 × 3,20 × 0,40 m (bas à 4,6 m, haut à 7,8 m : totem de 4 à 8 m, RG-03), couvertine débordante, faces en
//     plexiglas diffusant en retrait de 3 cm dans le cadre, impression de la face propre à chaque commerce ;
//   - la nuit, caisson rétroéclairé (tubes fluorescents ou LED derrière le plexiglas) : seule la partie translucide de
//     l'impression s'allume (le fond OU les lettres, selon le commerce), luminance bornée, sans halo ; de jour, rien
//     n'est allumé : les couleurs se lisent à la peinture.
// WFS-568 : place des enseignes sur mâts d'une carte (Ville : plan des quartiers ; Corniche : côté terre tous les 180 segments) ;
// partagé avec le banc d'essai (captures ciblées). Même code et mêmes tirages qu'avant : les panneaux ne bougent pas.
constexpr int NB_BOARDS = 5;   // textes de BOARD_TEXTS
std::vector<vie::PanneauRoute> vie::panneauxRoute(const MapDef& map, Track& t) {
    std::vector<PanneauRoute> out;
    const bool ville = !std::strcmp(map.id, "ville");
    const int64_t len = (int64_t)t.segmentCount();
    if (ville) {   // panneaux du plan des quartiers (city.centre : tous les 130 à 180 segments, côté tiré, pas sur un lampadaire)
        for (const wfs::ville::Panneau& b : wfs::ville::plan(map, len).panneaux) out.push_back({b.seg, b.side, b.texte});
    } else {   // Corniche : côté terre tous les 180 segments
        Mulberry32 D(map.seed * 31 + 23);
        // WFS-139 (accroche) : tracé 2 : aucun panneau (8,6 m au-dessus du bas-côté) dans un virage serré, un tunnel, un
        // rond-point ou un carrefour, ni dans les 60 m qui précèdent (il cacherait la route ou tomberait dessus)
        auto gene = [&](int64_t n) { for (int64_t k = n - 50; k <= n + 10; k += 5) { const trace::Section* s = trace::sectionA(t, (double)k * SEG);
            if (trace::rayonInterieur(t, (double)k * SEG).m < 100 || (s && (s->type == trace::Type::Tunnel || s->type == trace::Type::RondPoint || s->type == trace::Type::Feux || s->type == trace::Type::Lacets))) return true; } return false; };
        for (int64_t n = 60; n < len - 40; n += 180) {
            if (n % 24 >= 10 && n % 24 <= 14) n += 6;   // pas sur un lampadaire
            const int texte = (int)(D.next() * NB_BOARDS);   // tirage gardé : les autres panneaux ne changent pas
            if (!gene(n)) out.push_back({n, -1, texte});
        }
    }
    return out;
}

namespace {
using namespace vie;
// textes de BOARD_TEXTS (js/world/decor.js) : l'ordre et les tirages ne changent pas (indices du plan de la Ville).
// WFS-568 (RG-01, RG-02) : chaque commerce a sa propre impression, aux couleurs d'enseignes réelles, sans cyan.
struct Impression {
    const char* texte;
    unsigned fond, lettres;    // sRGB : fond de la face, lettres
    bool fondLumineux;         // true : le fond est translucide (il s'allume) et les lettres opaques ; false : l'inverse
};
const Impression BOARD_TEXTS[] = {
    {"WAFORSPEED", 0x15161a, 0xff6a00, false},   // lettres orange sur fond noir mat
    {"GARAGE 88", 0x1c3766, 0xf4f4f0, false},    // garage : lettres blanches sur caisson bleu marine
    {"TURBO CAFÉ", 0x4a2f1c, 0xf0e2c4, false},   // café : lettres crème sur fond brun bois
    {"NUIT BLANCHE", 0x121014, 0xff5c9e, false}, // boîte de nuit : lettres rose néon sur fond noir
    {"PNEUS 24/7", 0xf2c40c, 0x141414, true},    // pneus : lettres noires sur fond jaune (le jaune s'allume)
};
static_assert(sizeof(BOARD_TEXTS) / sizeof(BOARD_TEXTS[0]) == NB_BOARDS);
struct Panneau { int64_t seg; int side, texte; };

// mesures du modèle (m)
constexpr float CAISSON_L = 8.4f, CAISSON_H = 3.2f, CAISSON_E = 0.40f, CAISSON_BAS = 4.6f, PROFIL = 0.12f;
constexpr float MAT_R = 0.2730f / 2, MAT_ECART = 2.0f;   // mâts à ±2 m du milieu du caisson (le 1er reste à RW + 1 : collision)

// un caisson double face sur deux mâts ; R : origine au sol sous le milieu du caisson, x latéral (loin de la route), z le
// long de la route (les deux faces regardent ±z) ; tampons : peint (mât, béton), metal (aluminium, acier galvanisé),
// faces (fond imprimé), lettres (texte de l'atlas, 1 mm devant la face)
void caissonSurMats(mobilier::Sortie peint, mobilier::Sortie metal, mobilier::Sortie faces, Geo& lettres, const mobilier::Repere& R,
                    const Texte& T, vec2 uvMat, vec2 uvBeton, vec2 uvAlu, vec2 uvGalva, vec2 uvFond) {
    using mobilier::pave; using mobilier::tour;
    const int nm = 20;   // côtés du mât (vu de près au bord de la route)
    for (float xm : {-MAT_ECART, MAT_ECART}) {
        pave(peint, R, {xm, 0.0f, 0}, {0.35f, 0.10f, 0.35f}, 0.02f, uvBeton);                       // massif (10 cm hors sol)
        pave(metal, R, {xm, 0.1125f, 0}, {0.225f, 0.0125f, 0.225f}, 0.006f, uvGalva);                // platine 450 × 25
        for (float a : {-1.f, 1.f}) for (float b : {-1.f, 1.f}) {                                   // 4 tiges M24 : rondelle, écrou, bout fileté
            const vec3 c{xm + a * 0.17f, 0.125f, b * 0.17f};
            tour(metal, R, c, {{0.022f, 0}, {0.022f, 0.004f}, {0, 0.004f}}, 12, uvGalva);
            tour(metal, R, c + vec3{0, 0.004f, 0}, {{0.0208f, 0}, {0.0208f, 0.019f}, {0.018f, 0.022f}, {0, 0.022f}}, 6, uvGalva);
            tour(metal, R, c + vec3{0, 0.026f, 0}, {{0.012f, 0}, {0.012f, 0.03f}, {0.009f, 0.034f}, {0, 0.034f}}, 8, uvGalva);
        }
        // fourreau soudé au pied puis fût (le haut entre de 5 cm dans le caisson)
        tour(peint, R, {xm, 0.125f, 0}, {{MAT_R + 0.02f, 0}, {MAT_R + 0.02f, 0.012f}, {MAT_R + 0.006f, 0.03f}, {MAT_R + 0.006f, 0.30f},
                                         {MAT_R, 0.315f}, {MAT_R, CAISSON_BAS - 0.07f}, {0, CAISSON_BAS - 0.07f}}, nm, uvMat);
        pave(metal, R, {xm, CAISSON_BAS - 0.045f, 0}, {0.20f, 0.015f, 0.17f}, 0.004f, uvGalva);      // selle de fixation
        for (float a : {-1.f, 1.f}) for (float b : {-1.f, 1.f})                                      // et ses 4 boulons (têtes vers le bas)
            tour(metal, R, {xm + a * 0.15f, CAISSON_BAS - 0.075f, b * 0.12f}, {{0.015f, 0}, {0.015f, 0.014f}, {0, 0.014f}}, 6, uvGalva);
        pave(peint, R, {xm, CAISSON_BAS - 0.06f, 0}, {MAT_R * 0.95f, 0.004f, MAT_R * 0.95f}, 0.002f, uvMat);   // cordon de soudure
    }
    // caisson : cadre en profilé d'aluminium (traverses et montants chanfreinés), couvertine débordante, faces en retrait
    const float hl = CAISSON_L / 2, hh = CAISSON_H / 2, he = CAISSON_E / 2, yc = CAISSON_BAS + hh;
    pave(metal, R, {0, CAISSON_BAS + PROFIL / 2, 0}, {hl, PROFIL / 2, he}, 0.015f, uvAlu);
    pave(metal, R, {0, CAISSON_BAS + CAISSON_H - PROFIL / 2, 0}, {hl, PROFIL / 2, he}, 0.015f, uvAlu);
    for (float sx : {-1.f, 1.f}) pave(metal, R, {sx * (hl - PROFIL / 2), yc, 0}, {PROFIL / 2, hh - PROFIL, he}, 0.015f, uvAlu);
    pave(metal, R, {0, CAISSON_BAS + CAISSON_H + 0.015f, 0}, {hl + 0.02f, 0.015f, he + 0.025f}, 0.008f, uvAlu);   // couvertine
    for (float sx : {-1.f, 1.f}) for (float sz : {-1.f, 1.f})                                                   // équerres d'angle (vis)
        pave(metal, R, {sx * (hl - 0.03f), CAISSON_BAS + CAISSON_H - 0.03f, sz * (he + 0.002f)}, {0.022f, 0.022f, 0.002f}, 0.003f, uvGalva);
    // faces : plexiglas de 8 mm en retrait de 3 cm du nu du cadre (le volume entre les deux faces ferme le caisson)
    const float zf = he - 0.03f;
    pave(faces, R, {0, yc, 0}, {hl - PROFIL, hh - PROFIL, zf}, 0.004f, uvFond);
    // lettres imprimées sur chaque face (1 mm devant), centrées, 2,0 m de haut au plus, marges de 30 cm
    if (!T.ok) return;
    const float h = 2.0f, w = std::min(CAISSON_L - 2 * PROFIL - 0.6f, h * T.aspect), hs = h * w / (h * T.aspect);
    for (int face : {-1, 1}) {
        // sens de lecture tiré de la normale de la face (droite du lecteur = haut × normale) : jamais de texte en miroir, quel
        // que soit le côté de la route (le repère est indirect à gauche)
        const vec3 c = R.p({0, yc, face * (zf + 0.001f)}), n = R.z * (float)face;
        plan(lettres, c, normalize(cross(vec3{0, 1, 0}, n)), {0, 1, 0}, n, w, hs, T.u0, T.v0, T.u1, T.v1);
    }
}

class Enseignes : public System {
public:
    const char* name() const override { return "enseignes"; }
    void init(Renderer& r, const GameView& g) override {
        actif_ = false;
        panneaux_.clear();   // WFS-568 : une nouvelle course ne rajoute pas ses panneaux à ceux de la précédente
        if (!g.map || !g.track) return;
        if (std::strcmp(g.map->id, "ville") && std::strcmp(g.map->id, "plage")) return;   // RG-09 : Ville et Corniche (Grand Prix : WFS-53)
        for (const PanneauRoute& b : panneauxRoute(*g.map, *g.track)) panneaux_.push_back({b.seg, b.side, b.texte});
        for (auto& b : BOARD_TEXTS) atlas_.ajouter(b.texte, b.lettres);   // WFS-568 : lettres de la couleur du commerce
        if (!atlas_.construire(r)) { panneaux_.clear(); return; }
        // couleurs (sRGB) : RAL 7016 thermolaqué, béton, aluminium anodisé naturel, acier galvanisé, fonds des impressions
        const vec2 uvMat = palette_.uv(0x383e42), uvBeton = palette_.uv(0x8d8a84), uvAlu = palette_.uv(0xa9acb0), uvGalva = palette_.uv(0x8c9094);
        vec2 uvFond[NB_BOARDS];
        for (int i = 0; i < NB_BOARDS; i++) uvFond[i] = palette_.uv(BOARD_TEXTS[i].fond);
        palette_.construire(r);
        Material mm; mm.metal = 1; mm.rough = 0.42f; mm.flags = MAT_DOUBLE;                            // aluminium et acier
        metal_ = palette_.variante(r, mm);
        Material mp; mp.rough = 0.22f; mp.flags = MAT_DOUBLE;                                             // plexiglas imprimé (lisse)
        plexi_ = palette_.variante(r, mp);
        // WFS-568 (RG-04) : rétroéclairage 4 000 K derrière le plexiglas, la nuit seulement ; × la couleur de l'impression
        double k4000[3]; lumiere::couleurTemperature(4000, k4000);
        const vec3 retro = vec3{(float)k4000[0], (float)k4000[1], (float)k4000[2]} * FACE_NUIT;
        mp.emissive = retro; mp.flags |= MAT_EMTEX;
        plexiNuit_ = palette_.variante(r, mp);
        lettresJour_ = &atlas_.materiauCouleurs(r, 0.f);
        lettresNuit_ = &atlas_.materiauCouleurs(r, LETTRES_NUIT);
        Track& t = *g.track;
        for (const Panneau& b : panneaux_) {   // géométrie construite une fois (≈ 1 panneau / 200 m) ; un jeu de maillages par
            const Repere p = repere(t, b.seg * SEG);   // panneau : le moteur écarte ceux hors de la vue (sphère englobante, WFS-82)
            const float s = (float)b.side, R = (float)RW;
            const mobilier::Repere M = mobilier::repere(lateral(p, s * (R + 1 + MAT_ECART), 0.f), p.r * s, p.f);
            Geo peint, metal, faces, lettres;
            caissonSurMats(peint, metal, faces, lettres, M, atlas_.texte(BOARD_TEXTS[b.texte].texte), uvMat, uvBeton, uvAlu, uvGalva, uvFond[b.texte]);
            Caisson c; c.fondLumineux = BOARD_TEXTS[b.texte].fondLumineux;
            for (auto [geo, mesh] : {std::pair{&peint, &c.peint}, {&metal, &c.metal}, {&faces, &c.faces}, {&lettres, &c.lettres}})
                if (!geo->i.empty()) *mesh = r.makeMesh(geo->v, geo->i);
            caissons_.push_back(c);
        }
        actif_ = true;
        WFS_LOG("vie : %zu enseignes sur mâts (%s)", panneaux_.size(), g.map->id);
    }
    void render(Renderer& r, const GameView& g) override {
        if (!actif_ || g.sceneDediee) return;
        const mat4 I = mat4::identity();
        for (const Caisson& c : caissons_) {   // la nuit, seule la partie translucide de l'impression s'allume (fond OU lettres)
            const bool fondAllume = g.night && c.fondLumineux, lettresAllumees = g.night && !c.fondLumineux;
            if (c.peint.indexCount) r.draw(c.peint, palette_.materiau, I);
            if (c.metal.indexCount) r.draw(c.metal, metal_, I);
            if (c.faces.indexCount) r.draw(c.faces, fondAllume ? plexiNuit_ : plexi_, I);
            if (c.lettres.indexCount) r.draw(c.lettres, lettresAllumees ? *lettresNuit_ : *lettresJour_, I);
        }
    }
    void shutdown(Renderer& r) override {
        r.vk().waitIdle();
        for (Caisson& c : caissons_) for (Mesh* m : {&c.peint, &c.metal, &c.faces, &c.lettres}) if (m->indexCount) r.destroy(*m);
        caissons_.clear(); panneaux_.clear();
        atlas_.detruire(r); palette_.detruire(r);
        for (Material* m : {&metal_, &plexi_, &plexiNuit_}) r.destroy(*m);   // WFS-187 (accroche) : jeux de descripteurs rendus
        lettresJour_ = lettresNuit_ = nullptr;
        actif_ = false;
    }
private:
    // luminances de nuit (unités d'émission du moteur) : bornées pour qu'aucune enseigne ne fasse de halo (RG-04)
    static constexpr float FACE_NUIT = 0.45f, LETTRES_NUIT = 1.25f;   // grande face jaune : moitié des lettres (pas de halo)
    bool actif_ = false;
    std::vector<Panneau> panneaux_;
    AtlasTextes atlas_; Palette palette_;
    struct Caisson { Mesh peint, metal, faces, lettres; bool fondLumineux = false; };
    std::vector<Caisson> caissons_;
    Material metal_, plexi_, plexiNuit_;
    const Material *lettresJour_ = nullptr, *lettresNuit_ = nullptr;
};

} // namespace

const char* vie::textePanneauRoute(int texte) { return texte >= 0 && texte < NB_BOARDS ? BOARD_TEXTS[texte].texte : ""; }

WFS_SYSTEM(Enseignes, 107)   // décor (100-199) : après les piétons (106)

} // namespace wfs
