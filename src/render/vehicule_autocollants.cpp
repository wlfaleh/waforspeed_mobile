// WaForSpeed — WFS-495 : autocollants (voir vehicule_autocollants.h) : texte des calques, bibliothèque de motifs vectoriels
// (dessins originaux), rastérisation anticrénelée dans l'atlas de la texture d'habillage, cuisson et compression ETC2.
// Sans Vulkan ; testé par tests/test_vehicule_autocollants.cpp.
#include "vehicule_autocollants.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include "../../third_party/stb/stb_truetype.h"
#include "../game/sauvegarde.h"   // PEINTURES : les 28 couleurs du catalogue (WFS-487), données seulement
#include "vehicule_etc2.h"
#include "vehicule_skin.h"

namespace wfs::vehicule_autocollants {

namespace {

constexpr float PI = 3.14159265358979f;
constexpr float CAPITALE = 0.30f;   // hauteur des capitales et des chiffres à 100 % (m)

using P = std::pair<float, float>;

// ---------------------------------------------------------------- tracé de contours (courbes aplaties)
struct Trace {
    std::vector<Contour> c;
    void moveTo(float x, float y) { c.push_back({{x, y}}); }
    void lineTo(float x, float y) { c.back().push_back({x, y}); }
    void quadTo(float cx, float cy, float x, float y, int n = 10) {
        const P p0 = c.back().back();
        for (int i = 1; i <= n; i++) {
            const float t = (float)i / n, u = 1 - t;
            c.back().push_back({u * u * p0.first + 2 * u * t * cx + t * t * x, u * u * p0.second + 2 * u * t * cy + t * t * y});
        }
    }
    void cubicTo(float ax, float ay, float bx, float by, float x, float y, int n = 16) {
        const P p0 = c.back().back();
        for (int i = 1; i <= n; i++) {
            const float t = (float)i / n, u = 1 - t;
            const float k0 = u * u * u, k1 = 3 * u * u * t, k2 = 3 * u * t * t, k3 = t * t * t;
            c.back().push_back({k0 * p0.first + k1 * ax + k2 * bx + k3 * x, k0 * p0.second + k1 * ay + k2 * by + k3 * y});
        }
    }
    void rect(float x0, float y0, float x1, float y1) { moveTo(x0, y0); lineTo(x1, y0); lineTo(x1, y1); lineTo(x0, y1); }
    void poly(std::initializer_list<P> pts) { bool premier = true; for (const P& p : pts) { if (premier) moveTo(p.first, p.second); else lineTo(p.first, p.second); premier = false; } }
};

// ---------------------------------------------------------------- bandes (2,5 m de long à 100 %, le long de x ; à 180 % : 4,5 m,
// du capot au coffre d'une compacte)
void bande(Trace& t, int n) {
    const float L = 1.25f;
    switch (n) {
    case 1: t.rect(-L, -0.10f, L, 0.10f); break;                                    // large bande, 20 cm
    case 2: t.rect(-L, 0.04f, L, 0.18f); t.rect(-L, -0.18f, L, -0.04f); break;       // double bande 2 × 14 cm, 8 cm d'écart
    case 3:                                                                          // bande de 15 cm bordée de deux filets de 2 cm
        t.rect(-L, -0.075f, L, 0.075f);
        t.rect(-L, 0.105f, L, 0.125f); t.rect(-L, -0.125f, L, -0.105f);
        break;
    case 4: for (int k = -1; k <= 1; k++) t.rect(-L, k * 0.08f - 0.02f, L, k * 0.08f + 0.02f); break;   // trois filets de 4 cm
    default:                                                                         // flèche de vitesse : queue d'aronde, pointe
        t.poly({{-L, -0.10f}, {0.85f, -0.10f}, {L, 0.0f}, {0.85f, 0.10f}, {-L, 0.10f}, {-1.10f, 0.0f}});
        break;
    }
}

// aire signée d'un contour (> 0 : sens trigonométrique) ; les pièces d'un même motif réunies doivent tourner dans le même sens
float aire(const Contour& c) {
    float a = 0;
    for (size_t i = 0; i < c.size(); i++) { const P& p = c[i]; const P& q = c[(i + 1) % c.size()]; a += p.first * q.second - q.first * p.second; }
    return 0.5f * a;
}
void orienter(Contour& c, bool trigo) { if ((aire(c) > 0) != trigo) std::reverse(c.begin(), c.end()); }

// ---------------------------------------------------------------- flammes (1,2 m × 0,5 m à 100 %) : base arrondie à gauche, langues
// vers la droite (sur le flanc gauche : de l'avant vers l'arrière). Chaque langue suit une ligne médiane en S (elle part à plat,
// se creuse, puis sa pointe se relève) et s'affine jusqu'à la pointe ; base et langues réunies (même sens de rotation).
struct Langue { float ys, xt, yt, largeur, relevee; };   // départ (y) dans la base, pointe, largeur au départ, relevé de la pointe
void flamme(Trace& t, float rx, float ry, const std::vector<Langue>& L) {
    const float xb = -0.60f + rx;   // centre de la base
    t.moveTo(xb + rx, 0.f);         // base : ellipse
    for (int i = 1; i < 48; i++) { const float a = 2 * PI * i / 48; t.lineTo(xb + rx * std::cos(a), ry * std::sin(a)); }
    orienter(t.c.back(), true);
    for (const Langue& l : L) {
        const float sx = xb, sy = l.ys, lg = l.xt - sx;
        const P c1{sx + 0.40f * lg, sy - 0.02f}, c2{l.xt - 0.35f * lg, l.yt - l.relevee};
        const int N = 28;
        std::vector<P> haut, bas;
        for (int i = 0; i <= N; i++) {
            const float u = (float)i / N, v = 1 - u;
            const float x = v * v * v * sx + 3 * v * v * u * c1.first + 3 * v * u * u * c2.first + u * u * u * l.xt;
            const float y = v * v * v * sy + 3 * v * v * u * c1.second + 3 * v * u * u * c2.second + u * u * u * l.yt;
            const float dx = 3 * v * v * (c1.first - sx) + 6 * v * u * (c2.first - c1.first) + 3 * u * u * (l.xt - c2.first);
            const float dy = 3 * v * v * (c1.second - sy) + 6 * v * u * (c2.second - c1.second) + 3 * u * u * (l.yt - c2.second);
            const float n = std::sqrt(dx * dx + dy * dy) + 1e-6f;
            const float w = 0.5f * l.largeur * std::pow(v, 0.85f);   // s'affine jusqu'à la pointe
            haut.push_back({x - dy / n * w, y + dx / n * w}); bas.push_back({x + dy / n * w, y - dx / n * w});
        }
        t.c.push_back(haut);
        for (int i = N - 1; i >= 0; i--) t.c.back().push_back(bas[i]);
        orienter(t.c.back(), true);
    }
}
void flammeN(Trace& t, int n) {
    if (n == 1)   // classique : cinq langues
        flamme(t, 0.17f, 0.19f, {{0.10f, 0.20f, 0.225f, 0.15f, 0.06f}, {0.04f, 0.60f, 0.10f, 0.17f, 0.07f}, {-0.03f, 0.42f, -0.03f, 0.15f, 0.06f},
                                 {-0.09f, 0.53f, -0.15f, 0.13f, 0.06f}, {-0.13f, 0.12f, -0.225f, 0.11f, 0.05f}});
    else if (n == 2)   // trois longues langues larges
        flamme(t, 0.16f, 0.18f, {{0.08f, 0.40f, 0.19f, 0.20f, 0.08f}, {-0.01f, 0.60f, 0.02f, 0.21f, 0.08f}, {-0.10f, 0.36f, -0.20f, 0.17f, 0.07f}});
    else   // feu vif : six langues fines d'inégales longueurs
        flamme(t, 0.15f, 0.20f, {{0.13f, 0.05f, 0.235f, 0.09f, 0.05f}, {0.07f, 0.38f, 0.17f, 0.11f, 0.06f}, {0.02f, 0.60f, 0.08f, 0.12f, 0.06f},
                                 {-0.03f, 0.24f, -0.01f, 0.10f, 0.05f}, {-0.08f, 0.50f, -0.11f, 0.11f, 0.06f}, {-0.13f, 0.18f, -0.225f, 0.09f, 0.05f}});
}

// ---------------------------------------------------------------- éclairs (0,9 m × 0,35 m à 100 %, pointe à droite)
void eclair(Trace& t, int n) {
    if (n == 1) {   // éclair classique (marche au milieu)
        t.poly({{0.45f, 0.044f}, {-0.0225f, -0.175f}, {-0.0225f, 0.0f}, {-0.45f, -0.044f}, {0.0225f, 0.175f}, {0.0225f, 0.0f}});
    } else if (n == 2) {   // zigzag à trois marches, épaisseur régulière (joints en onglet), pointe fine
        const P axe[] = {{-0.43f, 0.07f}, {-0.16f, 0.14f}, {-0.10f, -0.03f}, {0.14f, 0.07f}, {0.20f, -0.09f}, {0.45f, -0.155f}};
        const int N = 6;
        const float e = 0.034f;   // demi-épaisseur (≈ 7 cm)
        std::vector<P> g, d;
        for (int i = 0; i < N; i++) {
            const P a = axe[i > 0 ? i - 1 : 0], b = axe[i < N - 1 ? i + 1 : N - 1];
            float tx = b.first - a.first, ty = b.second - a.second; const float l = std::sqrt(tx * tx + ty * ty); tx /= l; ty /= l;
            float k = e;
            if (i > 0 && i < N - 1) {   // onglet : demi-épaisseur / cos(demi-angle), borné
                const P p = axe[i - 1], q = axe[i], r = axe[i + 1];
                float ux = q.first - p.first, uy = q.second - p.second, vx = r.first - q.first, vy = r.second - q.second;
                const float lu = std::sqrt(ux * ux + uy * uy), lv = std::sqrt(vx * vx + vy * vy);
                ux /= lu; uy /= lu; vx /= lv; vy /= lv;
                k = e / std::max(0.35f, std::sqrt(0.5f * (1 + ux * vx + uy * vy)));
            }
            if (i == N - 1) k = 0.002f;   // pointe
            g.push_back({axe[i].first - ty * k, axe[i].second + tx * k}); d.push_back({axe[i].first + ty * k, axe[i].second - tx * k});
        }
        t.c.push_back(g);
        for (int i = N - 1; i >= 0; i--) t.c.back().push_back(d[i]);
    } else {   // double éclair : un grand et son écho plus petit, décalé vers l'arrière
        t.poly({{0.45f, 0.06f}, {0.0f, -0.175f}, {0.0f, -0.01f}, {-0.30f, -0.06f}, {0.06f, 0.175f}, {0.05f, 0.03f}});
        t.poly({{-0.05f, 0.075f}, {-0.30f, -0.04f}, {-0.30f, 0.03f}, {-0.45f, 0.0f}, {-0.27f, 0.12f}, {-0.27f, 0.06f}});
    }
}

// ---------------------------------------------------------------- étoiles (0,4 m à 100 %)
void etoile(Trace& t, int n) {
    auto branches = [&](int k, float R, float r, float phase, bool inverse) {
        std::vector<P> pts;
        for (int i = 0; i < 2 * k; i++) {
            const float a = phase + PI * i / k, rr = i % 2 ? r : R;
            pts.push_back({rr * std::cos(a), rr * std::sin(a)});
        }
        if (inverse) std::reverse(pts.begin(), pts.end());
        t.moveTo(pts[0].first, pts[0].second);
        for (size_t i = 1; i < pts.size(); i++) t.lineTo(pts[i].first, pts[i].second);
    };
    if (n == 1) branches(5, 0.20f, 0.0764f, PI / 2, false);   // étoile à cinq branches (rayon intérieur = R × 0,382)
    else if (n == 2) {   // éclat à quatre branches, côtés creusés
        t.moveTo(0.0f, 0.20f);
        const P pointes[4] = {{0.20f, 0.0f}, {0.0f, -0.20f}, {-0.20f, 0.0f}, {0.0f, 0.20f}};
        for (const P& p : pointes) t.quadTo(0.0f, 0.0f, p.first, p.second, 12);
    } else {   // étoile à cinq branches évidée (filet de 3 cm environ)
        branches(5, 0.20f, 0.0764f, PI / 2, false);
        branches(5, 0.135f, 0.0516f, PI / 2, true);   // trou : sens inverse
    }
}

// ---------------------------------------------------------------- lettres et chiffres (Rajdhani Bold)
bool glyphe(Trace& t, const std::vector<uint8_t>& police, int code) {
    if (police.empty()) return false;
    stbtt_fontinfo f;
    if (!stbtt_InitFont(&f, police.data(), stbtt_GetFontOffsetForIndex(police.data(), 0))) return false;
    int hx0, hy0, hx1, hy1;
    if (!stbtt_GetCodepointBox(&f, 'H', &hx0, &hy0, &hx1, &hy1) || hy1 <= hy0) return false;
    const float k = CAPITALE / (float)(hy1 - hy0);   // capitales de 30 cm à 100 %
    int gx0, gy0, gx1, gy1;
    if (!stbtt_GetCodepointBox(&f, code, &gx0, &gy0, &gx1, &gy1)) return false;
    const float cx = (gx0 + gx1) * 0.5f, cy = (gy0 + gy1) * 0.5f;
    stbtt_vertex* v = nullptr;
    const int n = stbtt_GetCodepointShape(&f, code, &v);
    auto X = [&](float x) { return (x - cx) * k; };
    auto Y = [&](float y) { return (y - cy) * k; };
    for (int i = 0; i < n; i++) {
        switch (v[i].type) {
        case STBTT_vmove: t.moveTo(X(v[i].x), Y(v[i].y)); break;
        case STBTT_vline: t.lineTo(X(v[i].x), Y(v[i].y)); break;
        case STBTT_vcurve: t.quadTo(X(v[i].cx), Y(v[i].cy), X(v[i].x), Y(v[i].y), 8); break;
        case STBTT_vcubic: t.cubicTo(X(v[i].cx), Y(v[i].cy), X(v[i].cx1), Y(v[i].cy1), X(v[i].x), Y(v[i].y), 10); break;
        default: break;
        }
    }
    stbtt_FreeShape(&f, v);
    return n > 0;
}

// ---------------------------------------------------------------- bibliothèque
std::vector<Motif> construire() {
    std::vector<Motif> b;
    static const char* BANDES[] = {"bande-large", "bande-double", "bande-filets", "bande-triple", "bande-fleche"};
    const float hb[] = {0.20f, 0.36f, 0.25f, 0.20f, 0.20f};
    for (int i = 0; i < 5; i++) b.push_back({BANDES[i], Famille::Bande, 2.5f, hb[i]});
    static const char* CHIFFRES[] = {"chiffre-0", "chiffre-1", "chiffre-2", "chiffre-3", "chiffre-4", "chiffre-5", "chiffre-6", "chiffre-7", "chiffre-8", "chiffre-9"};
    for (const char* c : CHIFFRES) b.push_back({c, Famille::Chiffre, 0.18f, CAPITALE});   // largeur indicative (la police donne la vraie)
    for (const char* f : {"flamme-1", "flamme-2", "flamme-3"}) b.push_back({f, Famille::Flamme, 1.2f, 0.5f});
    for (const char* e : {"eclair-1", "eclair-2", "eclair-3"}) b.push_back({e, Famille::Eclair, 0.9f, 0.35f});
    for (const char* e : {"etoile-1", "etoile-2", "etoile-3"}) b.push_back({e, Famille::Etoile, 0.4f, 0.4f});
    static const char* LETTRES[] = {"lettre-a", "lettre-b", "lettre-c", "lettre-d", "lettre-e", "lettre-f", "lettre-g", "lettre-h", "lettre-i",
        "lettre-j", "lettre-k", "lettre-l", "lettre-m", "lettre-n", "lettre-o", "lettre-p", "lettre-q", "lettre-r", "lettre-s", "lettre-t",
        "lettre-u", "lettre-v", "lettre-w", "lettre-x", "lettre-y", "lettre-z"};
    for (const char* l : LETTRES) b.push_back({l, Famille::Lettre, 0.22f, CAPITALE});
    return b;
}

const PeintureDef* couleur(const std::string& id) {
    for (const PeintureDef& p : PEINTURES) if (id == p.id) return &p;
    return nullptr;
}

bool directionDepuis(const std::string& s, Direction& d) {
    static const char CODES[] = "gdctkar";
    if (s.size() != 1) return false;
    const char* p = std::strchr(CODES, s[0]);
    if (!p || !*p) return false;
    d = (Direction)(p - CODES);
    return true;
}

bool entier(const std::string& s, int& v) {
    if (s.empty() || s.size() > 6) return false;
    char* fin = nullptr;
    const long l = std::strtol(s.c_str(), &fin, 10);
    if (!fin || *fin) return false;
    v = (int)l;
    return true;
}

// ---------------------------------------------------------------- rastérisation : couverture exacte (aires signées accumulées par
// ligne, puis somme courante ; méthode des rastériseurs de polices « à accumulation »). Le tampon couvre la boîte du motif.
struct Tampon {
    int x0 = 0, y0 = 0, w = 0, h = 0;   // origine (pixels de l'atlas) et taille
    std::vector<float> acc;
    void ligne(float ax, float ay, float bx, float by) {
        ax -= x0; bx -= x0; ay -= y0; by -= y0;
        if (ay == by) return;
        float dir = 1.f;
        if (ay > by) { std::swap(ax, bx); std::swap(ay, by); dir = -1.f; }
        const float dxdy = (bx - ax) / (by - ay);
        float x = ax;
        if (ay < 0) x -= ay * dxdy;
        const int ydeb = std::max(0, (int)ay), yfin = std::min(h, (int)std::ceil(by));
        for (int y = ydeb; y < yfin; y++) {
            float* L = &acc[(size_t)y * (w + 2)];
            const float dy = std::min((float)(y + 1), by) - std::max((float)y, ay);
            const float xn = x + dxdy * dy, d = dy * dir;
            const float xa = std::min(x, xn), xb = std::max(x, xn);
            const float xaf = std::floor(xa);
            const int xai = std::clamp((int)xaf, 0, w), xbi = std::clamp((int)std::ceil(xb), 0, w + 1);
            if (xbi <= xai + 1) {
                const float xm = 0.5f * (x + xn) - xaf;
                L[xai] += d - d * xm;
                L[xai + 1] += d * xm;
            } else {
                const float s = 1.f / (xb - xa), xa0 = xa - xaf;
                const float a0 = 0.5f * s * (1 - xa0) * (1 - xa0);
                const float xb1 = xb - (float)xbi + 1, am = 0.5f * s * xb1 * xb1;
                L[xai] += d * a0;
                if (xbi == xai + 2) L[xai + 1] += d * (1 - a0 - am);
                else {
                    const float a1 = s * (1.5f - xa0);
                    L[xai + 1] += d * (a1 - a0);
                    for (int xi = xai + 2; xi < xbi - 1; xi++) L[xi] += d * s;
                    const float a2 = a1 + (xbi - xai - 3) * s;
                    L[xbi - 1] += d * (1 - a2 - am);
                }
                L[std::min(xbi, w + 1)] += d * am;
            }
            x = xn;
        }
    }
};

// zone de l'atlas d'une direction : colonnes [c0, c1), lignes [l0, l1)
void zone(Direction d, int& c0, int& c1, int& l0, int& l1) {
    namespace A = vehicule_skin::atlas;
    c0 = 0; c1 = A::TAILLE;
    switch (d) {
    case Direction::Gauche: l0 = A::GAUCHE0; l1 = A::GAUCHE1; break;
    case Direction::Droite: l0 = A::DROITE0; l1 = A::DROITE1; break;
    case Direction::Avant: l0 = A::FACES0; l1 = A::FACES1; c1 = A::TAILLE / 2; break;
    case Direction::Arriere: l0 = A::FACES0; l1 = A::FACES1; c0 = A::TAILLE / 2; break;
    default: l0 = A::DESSUS0; l1 = A::DESSUS1; break;
    }
}

inline float versLin(float s) { return std::pow(s, 2.2f); }    // même conversion que le shader (pow 2,2)
inline float versSrgb(float l) { return std::pow(std::max(l, 0.f), 1.f / 2.2f); }

// tampon RGBA prémultiplié en linéaire → RGBA8 (RGB en codes sRGB non prémultipliés), puis couleurs étendues sous les texels
// transparents (4 passes) : le filtrage du GPU ne mêle jamais une couleur noire au bord d'un motif
void versRgba8(const std::vector<float>& f, uint32_t T, std::vector<uint8_t>& o) {
    o.assign((size_t)T * T * 4, 0);
    for (size_t i = 0; i < (size_t)T * T; i++) {
        const float a = std::clamp(f[i * 4 + 3], 0.f, 1.f);
        o[i * 4 + 3] = (uint8_t)std::lround(a * 255.f);
        if (a <= 0.f) continue;
        for (int c = 0; c < 3; c++) o[i * 4 + c] = (uint8_t)std::lround(std::clamp(versSrgb(f[i * 4 + c] / a), 0.f, 1.f) * 255.f);
    }
    std::vector<uint8_t> plein((size_t)T * T);
    for (size_t i = 0; i < plein.size(); i++) plein[i] = o[i * 4 + 3] > 0;
    for (int passe = 0; passe < 4; passe++) {
        std::vector<uint8_t> suivant = plein;
        for (uint32_t y = 0; y < T; y++) for (uint32_t x = 0; x < T; x++) {
            const size_t i = (size_t)y * T + x;
            if (plein[i]) continue;
            int s[3] = {0, 0, 0}, n = 0;
            const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
            for (int k = 0; k < 4; k++) {
                const int X = (int)x + dx[k], Y = (int)y + dy[k];
                if (X < 0 || Y < 0 || X >= (int)T || Y >= (int)T) continue;
                const size_t j = (size_t)Y * T + X;
                if (!plein[j]) continue;
                for (int c = 0; c < 3; c++) s[c] += o[j * 4 + c];
                n++;
            }
            if (!n) continue;
            for (int c = 0; c < 3; c++) o[i * 4 + c] = (uint8_t)((s[c] + n / 2) / n);
            suivant[i] = 1;
        }
        plein.swap(suivant);
    }
}

double ms(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

} // namespace

// ---------------------------------------------------------------- texte des calques
char codeDirection(Direction d) { return "gdctkar"[(int)d]; }
const char* nomDirection(Direction d) {
    static const char* N[] = {"gauche", "droite", "capot", "toit", "coffre", "avant", "arrière"};
    return N[(int)d];
}

bool lire(const std::string& t, std::vector<Calque>& sortie, int* rejetes) {
    sortie.clear();
    int rej = 0;
    std::istringstream is(t.size() > 4096 ? std::string() : t);   // texte anormalement long : rien (sauvegarde abîmée)
    if (t.size() > 4096) rej++;
    std::string bloc;
    while (std::getline(is, bloc, ';')) {
        if (bloc.empty()) continue;
        std::vector<std::string> ch;
        std::istringstream bs(bloc);
        for (std::string c; std::getline(bs, c, ',');) ch.push_back(c);
        Calque c;
        int miroir = 0;
        const bool ok = ch.size() == 8 && motif(ch[0]) && directionDepuis(ch[1], c.dir) && entier(ch[2], c.x) && entier(ch[3], c.y)
                        && entier(ch[4], c.rotation) && entier(ch[5], c.echelle) && couleur(ch[6]) && entier(ch[7], miroir)
                        && (miroir == 0 || miroir == 1);
        if (!ok || (int)sortie.size() >= MAX_CALQUES) { rej++; continue; }
        c.motif = ch[0]; c.couleur = ch[6]; c.miroir = miroir == 1;
        c.x = std::clamp(c.x, -600, 600); c.y = std::clamp(c.y, -300, 300);   // au-delà : hors de la voiture
        c.rotation = ((c.rotation % 360) + 540) % 360 - 180;
        c.echelle = std::clamp(c.echelle, ECHELLE_MIN, ECHELLE_MAX);
        sortie.push_back(c);
    }
    if (rejetes) *rejetes = rej;
    return rej == 0;
}

std::string texte(const std::vector<Calque>& calques) {
    std::string s;
    for (const Calque& c : calques) {
        if (!s.empty()) s += ';';
        s += c.motif + ',' + codeDirection(c.dir) + ',' + std::to_string(c.x) + ',' + std::to_string(c.y) + ',' + std::to_string(c.rotation) + ','
             + std::to_string(c.echelle) + ',' + c.couleur + ',' + (c.miroir ? '1' : '0');
    }
    return s;
}

// ---------------------------------------------------------------- bibliothèque
const std::vector<Motif>& bibliotheque() { static const std::vector<Motif> b = construire(); return b; }
const Motif* motif(const std::string& id) {
    for (const Motif& m : bibliotheque()) if (id == m.id) return &m;
    return nullptr;
}
bool lisible(Famille f) { return f == Famille::Chiffre || f == Famille::Lettre; }

std::vector<Contour> contours(const std::string& id, const std::vector<uint8_t>& police) {
    const Motif* m = motif(id);
    if (!m) return {};
    Trace t;
    const int n = id.back() - '0';
    switch (m->famille) {
    case Famille::Bande: bande(t, (int)(m - &bibliotheque()[0]) + 1); break;
    case Famille::Flamme: flammeN(t, n); break;
    case Famille::Eclair: eclair(t, n); break;
    case Famille::Etoile: etoile(t, n); break;
    case Famille::Chiffre: glyphe(t, police, '0' + n); break;
    case Famille::Lettre: glyphe(t, police, 'A' + (id.back() - 'a')); break;
    }
    return t.c;
}

// ---------------------------------------------------------------- repères
void ancrage(Direction d, const float bmin[3], const float bmax[3], float o[3]) {
    const float L = bmax[2] - bmin[2], zc = (bmin[2] + bmax[2]) * 0.5f, H = bmax[1];
    o[0] = 0; o[1] = 0; o[2] = zc;
    switch (d) {
    case Direction::Gauche: case Direction::Droite: o[1] = 0.42f * H; break;
    case Direction::Capot: o[2] = bmin[2] + 0.20f * L; break;
    case Direction::Toit: o[2] = zc + 0.05f * L; break;
    case Direction::Coffre: o[2] = bmax[2] - 0.10f * L; break;
    case Direction::Avant: o[1] = 0.40f * H; o[2] = bmin[2]; break;
    case Direction::Arriere: o[1] = 0.40f * H; o[2] = bmax[2]; break;
    }
}

void versAtlas(Direction d, const float an[3], float a, float b, float& u, float& v) {
    namespace A = vehicule_skin::atlas;
    const float T = (float)A::TAILLE, DX = A::DEMI_LARGEUR, DZ = A::DEMI_LONGUEUR, H = A::HAUTEUR;
    auto ligne = [&](int l0, int l1, float t) { return l0 + t * (l1 - l0); };
    switch (d) {
    case Direction::Gauche: { const float z = an[2] + a, y = an[1] + b; u = (z + DZ) / (2 * DZ) * T; v = ligne(A::GAUCHE0, A::GAUCHE1, (H - y) / H); break; }
    case Direction::Droite: { const float z = an[2] - a, y = an[1] + b; u = (DZ - z) / (2 * DZ) * T; v = ligne(A::DROITE0, A::DROITE1, (H - y) / H); break; }
    case Direction::Avant: { const float x = -a, y = an[1] + b; u = (DX - x) * 0.25f / DX * T; v = ligne(A::FACES0, A::FACES1, (H - y) / H); break; }
    case Direction::Arriere: { const float x = a, y = an[1] + b; u = (0.5f + (x + DX) * 0.25f / DX) * T; v = ligne(A::FACES0, A::FACES1, (H - y) / H); break; }
    default: { const float z = an[2] + a, x = b; u = (z + DZ) / (2 * DZ) * T; v = ligne(A::DESSUS0, A::DESSUS1, (x + DX) / (2 * DX)); break; }
    }
}

// ---------------------------------------------------------------- cuisson
bool cuire(const Entree& e, Resultat& r) {
    namespace A = vehicule_skin::atlas;
    const auto t0 = std::chrono::steady_clock::now();
    const uint32_t T = A::TAILLE;
    r = Resultat{};
    r.taille = T;
    const bool calques = e.calques && !e.calques->empty();
    if (!e.masques && !calques) return false;
    std::vector<float> f((size_t)T * T * 4, 0.f);   // RGBA prémultiplié, linéaire
    // 1) skin (RG-05) : A puis B par-dessus, comme le shader : mix(mix(peinture, A, r), B, g)
    if (e.masques) {
        float la[3], lb[3];
        for (int c = 0; c < 3; c++) { la[c] = versLin(e.couleurA[c]); lb[c] = versLin(e.couleurB[c]); }
        for (size_t i = 0; i < (size_t)T * T; i++) {
            const float ra = e.masques[i * 2] / 255.f, gb = e.masques[i * 2 + 1] / 255.f;
            for (int c = 0; c < 3; c++) f[i * 4 + c] = la[c] * ra * (1 - gb) + lb[c] * gb;
            f[i * 4 + 3] = 1 - (1 - ra) * (1 - gb);
        }
    }
    // 2) calques, dans l'ordre (le dernier par-dessus) ; copie du miroir juste après son calque
    if (calques) {
        Tampon tp;
        for (const Calque& c : *e.calques) {
            const Motif* m = motif(c.motif);
            const PeintureDef* p = couleur(c.couleur);
            if (!m || !p) continue;
            const std::vector<Contour> cs = contours(c.motif, e.police ? *e.police : std::vector<uint8_t>{});
            if (cs.empty()) { r.motifsVides++; continue; }
            const float lin[3] = {versLin(((p->hex >> 16) & 255) / 255.f), versLin(((p->hex >> 8) & 255) / 255.f), versLin((p->hex & 255) / 255.f)};
            for (int copie = 0; copie < (c.miroir ? 2 : 1); copie++) {
                Direction d = c.dir;
                float px = c.x * 0.01f, py = c.y * 0.01f, rot = c.rotation * PI / 180.f;
                bool retourneX = false, retourneY = false;
                if (copie == 1) {   // symétrique par rapport au plan médian de la voiture (x → −x), vue de dehors
                    const bool dessus = d == Direction::Capot || d == Direction::Toit || d == Direction::Coffre;
                    if (d == Direction::Gauche) d = Direction::Droite;
                    else if (d == Direction::Droite) d = Direction::Gauche;
                    else if (std::abs(dessus ? c.y : c.x) < 1) continue;   // sur l'axe : la copie se confondrait avec l'original
                    // flancs et faces : l'axe a du plan s'inverse ; dessus : l'axe b (b = x de la voiture)
                    if (dessus) py = -py; else px = -px;
                    rot = -rot;
                    const bool retourne = !lisible(m->famille);   // lettres et chiffres restent lisibles
                    retourneX = retourne && !dessus; retourneY = retourne && dessus;
                }
                float an[3];
                ancrage(d, e.bmin, e.bmax, an);
                const float s = c.echelle * 0.01f, cr = std::cos(rot), sr = std::sin(rot);
                // motif (m, à 100 %) → plan de la direction → pixels de l'atlas
                std::vector<Contour> pix = cs;
                float umin = 1e9f, umax = -1e9f, vmin = 1e9f, vmax = -1e9f;
                for (Contour& k : pix) for (P& q : k) {
                    const float mx = (retourneX ? -q.first : q.first) * s, my = (retourneY ? -q.second : q.second) * s;
                    const float a = px + cr * mx - sr * my, b = py + sr * mx + cr * my;
                    versAtlas(d, an, a, b, q.first, q.second);
                    umin = std::min(umin, q.first); umax = std::max(umax, q.first); vmin = std::min(vmin, q.second); vmax = std::max(vmax, q.second);
                }
                int c0, c1, l0, l1;
                zone(d, c0, c1, l0, l1);
                const int X0 = std::max(c0, (int)std::floor(umin)), X1 = std::min(c1, (int)std::ceil(umax) + 1);
                const int Y0 = std::max(l0, (int)std::floor(vmin)), Y1 = std::min(l1, (int)std::ceil(vmax) + 1);
                if (X0 >= X1 || Y0 >= Y1) continue;   // hors de la zone : rien de visible
                // le tampon couvre toute la boîte du motif (les segments hors zone comptent pour la couverture des texels de la zone)
                tp.x0 = (int)std::floor(umin) - 1; tp.y0 = (int)std::floor(vmin) - 1;
                tp.w = (int)std::ceil(umax) - tp.x0 + 2; tp.h = (int)std::ceil(vmax) - tp.y0 + 2;
                tp.acc.assign((size_t)(tp.w + 2) * tp.h, 0.f);
                for (const Contour& k : pix)
                    for (size_t i = 0; i < k.size(); i++) {
                        const P& a = k[i]; const P& b = k[(i + 1) % k.size()];
                        tp.ligne(a.first, a.second, b.first, b.second);
                    }
                for (int y = Y0; y < Y1; y++) {
                    const float* L = &tp.acc[(size_t)(y - tp.y0) * (tp.w + 2)];
                    float somme = 0;
                    for (int x = tp.x0; x < X1; x++) {
                        somme += L[x - tp.x0];
                        if (x < X0) continue;
                        const float cov = std::min(1.f, std::abs(somme));
                        if (cov <= 0.f) continue;
                        float* o = &f[((size_t)y * T + x) * 4];
                        for (int k = 0; k < 3; k++) o[k] = lin[k] * cov + o[k] * (1 - cov);
                        o[3] = cov + o[3] * (1 - cov);
                    }
                }
                r.calques++;
            }
        }
    }
    // 3) marges entre les zones (8 lignes) : copie de la ligne de bord la plus proche, comme tools/skins/generer.py
    {
        const int zones[4][2] = {{A::GAUCHE0, A::GAUCHE1}, {A::DROITE0, A::DROITE1}, {A::DESSUS0, A::DESSUS1}, {A::FACES0, A::FACES1}};
        for (int y = 0; y < (int)T; y++) {
            bool dedans = false;
            for (auto& z : zones) dedans |= y >= z[0] && y < z[1];
            if (dedans) continue;
            int meilleure = -1, dist = 1 << 20;
            for (auto& z : zones) {
                const int bords[2] = {z[0], z[1] - 1};
                for (int b : bords) { const int dd = std::abs(b - y) * 2 + (b > y ? 1 : 0); if (dd < dist) { dist = dd; meilleure = b; } }
            }
            std::memcpy(&f[(size_t)y * T * 4], &f[(size_t)meilleure * T * 4], T * 4 * sizeof(float));
        }
    }
    r.msDessin = ms(t0);
    // 4) mipmaps (moyenne 2 × 2 du prémultiplié : grandeurs linéaires), chaque niveau en RGBA8
    const auto t1 = std::chrono::steady_clock::now();
    std::vector<std::vector<uint8_t>> rgba;
    std::vector<uint32_t> tailles;
    {
        std::vector<float> niv = f;
        uint32_t t = T;
        for (;;) {
            rgba.emplace_back();
            versRgba8(niv, t, rgba.back());
            tailles.push_back(t);
            if (t == 1) break;
            const uint32_t m = t / 2;
            std::vector<float> s((size_t)m * m * 4);
            for (uint32_t y = 0; y < m; y++) for (uint32_t x = 0; x < m; x++) for (int c = 0; c < 4; c++)
                s[((size_t)y * m + x) * 4 + c] = 0.25f * (niv[((size_t)(2 * y) * t + 2 * x) * 4 + c] + niv[((size_t)(2 * y) * t + 2 * x + 1) * 4 + c]
                                                         + niv[((size_t)(2 * y + 1) * t + 2 * x) * 4 + c] + niv[((size_t)(2 * y + 1) * t + 2 * x + 1) * 4 + c]);
            niv.swap(s);
            t = m;
        }
    }
    r.msMip = ms(t1);
    // 5) compression ETC2 RGBA8 de chaque niveau (lignes de blocs réparties sur les cœurs)
    const auto t2 = std::chrono::steady_clock::now();
    r.niveaux.resize(rgba.size());
    for (size_t k = 0; k < rgba.size(); k++) {
        const uint32_t t = tailles[k];
        r.niveaux[k].resize(vehicule_etc2::octets(t, t));
        const int n = (int)vehicule_etc2::lignesDeBlocs(t);
        uint8_t* sortie = r.niveaux[k].data();
        const uint8_t* src = rgba[k].data();
        auto corps = [&, t, sortie, src](int a, int b) { vehicule_etc2::encoderLignes(src, t, t, sortie, (uint32_t)a, (uint32_t)b); };
        if (e.repartir && n > 4) e.repartir(n, corps);
        else corps(0, n);
    }
    r.msCompression = ms(t2);
    if (e.garderRgba) r.rgba = std::move(rgba[0]);
    return true;
}

} // namespace wfs::vehicule_autocollants
