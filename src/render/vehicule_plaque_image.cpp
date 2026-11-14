// WaForSpeed — WFS-483 (RG-02, RG-03, RG-05, RG-06) : image de la plaque du joueur, sur le processeur (sans Vulkan, testée).
// 1024 × 216 px pour 520 × 110 mm (≈ 2 px par mm). Deux images :
//   - couleur (sRGB) : fond blanc cassé (film rétroréfléchissant), liseré noir de 1,5 mm à 5 mm du bord, bande verticale gris
//     anthracite à droite (40 mm) avec un double chevron et « WFS » gris clair (motif inventé), caractères noirs ;
//   - normales (repère u vers la droite, v vers le bas, comme Renderer : dérivées de l'écran) tirées d'une carte de hauteur :
//     caractères en relief de 1 mm aux bords arrondis, bourrelet embouti de 0,6 mm le long du bord, grain très fin du film.
// Texte : Rajdhani Bold (OFL, police du jeu), caractères de 75 mm de haut, chasse resserrée à 80 % (constante), centrés dans la
// zone entre la vis de gauche et la bande. Texte trop large pour la zone : réduit en entier (hauteur et largeur), jamais étiré.
#include <algorithm>
#include <cmath>
#include <cstring>
#include "../../third_party/stb/stb_truetype.h"
#include "vehicule_plaque.h"

namespace wfs::plaque_joueur {
namespace {


// couleurs (sRGB 8 bits) : blanc cassé d'un film rétroréfléchissant neuf, noir d'encre sérigraphiée, anthracite RAL 7016
constexpr uint8_t FOND[3] = {236, 234, 226}, ENCRE[3] = {22, 22, 24}, BANDE[3] = {56, 62, 66}, MOTIF[3] = {196, 200, 204};

float lisse(float a, float b, float x) { const float t = std::clamp((x - a) / (b - a), 0.f, 1.f); return t * t * (3 - 2 * t); }

// distance signée au rectangle à coins arrondis de la plaque (mm, négative dedans), point en mm depuis le centre
float distanceBord(float x, float y) {
    const float a = PLAQUE_L_MM / 2 - PLAQUE_RAYON_MM, b = PLAQUE_H_MM / 2 - PLAQUE_RAYON_MM;
    const float qx = std::fabs(x) - a, qy = std::fabs(y) - b;
    return std::sqrt(std::max(qx, 0.f) * std::max(qx, 0.f) + std::max(qy, 0.f) * std::max(qy, 0.f)) + std::min(std::max(qx, qy), 0.f) - PLAQUE_RAYON_MM;
}

// petit bruit déterministe (grain du film, ± 1)
float grain(int x, int y) { uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u; h = (h ^ (h >> 13)) * 1274126177u; return ((h >> 8) & 0xffff) / 32767.5f - 1; }

// pose les glyphes d'une chaîne dans un masque (couverture 0..1), à l'échelle sx, sy (px par unité de la police), ligne de base
// en (x0, base) ; rend la largeur posée (px)
// L × Hi : taille du masque (px) ; WFS-484 : résolution réglable
float poser(const stbtt_fontinfo& f, const std::string& s, float sx, float sy, float x0, float base, float ecart, int L, int Hi, std::vector<float>& masque) {
    float x = x0;
    std::vector<unsigned char> tampon;
    for (size_t i = 0; i < s.size(); i++) {
        int adv, lsb; stbtt_GetCodepointHMetrics(&f, s[i], &adv, &lsb);
        int bx0, by0, bx1, by1;
        const float fx = x - std::floor(x);
        stbtt_GetCodepointBitmapBoxSubpixel(&f, s[i], sx, sy, fx, 0, &bx0, &by0, &bx1, &by1);
        const int w = bx1 - bx0, h = by1 - by0;
        if (w > 0 && h > 0) {
            tampon.assign((size_t)w * h, 0);
            stbtt_MakeCodepointBitmapSubpixel(&f, tampon.data(), w, h, w, sx, sy, fx, 0, s[i]);
            for (int j = 0; j < h; j++)
                for (int k = 0; k < w; k++) {
                    const int px = (int)std::floor(x) + bx0 + k, py = (int)std::lround(base) + by0 + j;
                    if (px < 0 || py < 0 || px >= L || py >= Hi) continue;
                    float& m = masque[(size_t)py * L + px];
                    m = std::max(m, tampon[(size_t)j * w + k] / 255.f);
                }
        }
        x += adv * sx + (i + 1 < s.size() ? ecart : 0);
    }
    return x - x0;
}

float largeurTexte(const stbtt_fontinfo& f, const std::string& s, float sx, float ecart) {
    float w = 0;
    for (size_t i = 0; i < s.size(); i++) {
        int adv, lsb; stbtt_GetCodepointHMetrics(&f, s[i], &adv, &lsb);
        // dernier caractère : sa boîte (pas son avance), pour un centrage juste
        if (i + 1 == s.size()) { int x0, y0, x1, y1; stbtt_GetCodepointBox(&f, s[i], &x0, &y0, &x1, &y1); w += x1 * sx; }
        else w += adv * sx + ecart;
    }
    return w;
}

} // namespace

bool dessinerImage(const std::string& brut, const uint8_t* ttf, ImagePlaque& im, int largeur) {
    im = ImagePlaque{};
    // WFS-484 : résolution demandée (atlas des autres voitures : 256 × 54 px) ; plaque du joueur : 1024 × 216 px (≈ 1,97 px par mm)
    const int IMAGE_L = std::clamp(largeur, 64, wfs::plaque_joueur::IMAGE_L);
    const int IMAGE_H = (int)std::lround(IMAGE_L * (float)wfs::plaque_joueur::IMAGE_H / wfs::plaque_joueur::IMAGE_L);
    const float PX_MM = IMAGE_L / PLAQUE_L_MM;
    im.largeur = IMAGE_L; im.hauteur = IMAGE_H;
    stbtt_fontinfo f;
    if (!ttf || !stbtt_InitFont(&f, ttf, stbtt_GetFontOffsetForIndex(ttf, 0))) return false;
    std::string texte;
    for (char c : brut) {
        const char C = (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
        if (((C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9') || C == '-') && (int)texte.size() < TEXTE_MAX) texte += C;
    }
    const size_t N = (size_t)IMAGE_L * IMAGE_H;
    std::vector<float> texteM(N, 0.f), motifM(N, 0.f);
    // ---- texte (RG-03) : hauteur de capitale = 75 mm
    int hx0, hy0, hx1, hy1; stbtt_GetCodepointBox(&f, 'H', &hx0, &hy0, &hx1, &hy1);
    const float capitale = (float)std::max(1, hy1);
    const float zoneG = -PLAQUE_L_MM / 2 + ZONE_TEXTE_G_MM, zoneD = PLAQUE_L_MM / 2 - 6.5f - BANDE_L_MM - ZONE_TEXTE_D_MM;
    float hauteur = CARACTERE_H_MM;
    float sy = hauteur / capitale * PX_MM, sx = sy * CONDENSE, ecart = ECART_MIN_MM * PX_MM;
    float w = largeurTexte(f, texte, sx, ecart);
    const float zone = (zoneD - zoneG) * PX_MM;
    if (w > zone) {   // trop large : tout réduit dans la même proportion (jamais étiré ni écrasé)
        const float k = zone / w;
        hauteur *= k; sy *= k; sx *= k; ecart *= k; w = zone;
    }
    if (!texte.empty()) {
        int x0b, y0b, x1b, y1b; stbtt_GetCodepointBox(&f, texte[0], &x0b, &y0b, &x1b, &y1b);
        const float centre = ((zoneG + zoneD) / 2 + PLAQUE_L_MM / 2) * PX_MM;
        const float base = IMAGE_H / 2.f + hauteur * PX_MM / 2;   // capitales centrées en hauteur
        poser(f, texte, sx, sy, centre - w / 2 - x0b * sx, base, ecart, IMAGE_L, IMAGE_H, texteM);
    }
    im.hauteurTexteMm = texte.empty() ? 0 : hauteur;
    im.largeurTexteMm = w / PX_MM;
    im.caracteres = (int)texte.size();
    // ---- motif de la bande (RG-02) : double chevron au-dessus de la vis, « WFS » en dessous (gris clair, sans relief)
    const float bandeX0 = PLAQUE_L_MM - 6.5f - BANDE_L_MM, bandeX1 = PLAQUE_L_MM - 6.5f, bandeCx = (bandeX0 + bandeX1) / 2;
    {
        const float sm = 10.f / capitale * PX_MM;   // capitales de 10 mm
        const float lw = largeurTexte(f, "WFS", sm * CONDENSE, 0.8f * PX_MM);
        poser(f, "WFS", sm * CONDENSE, sm, bandeCx * PX_MM - lw / 2, (PLAQUE_H_MM - 17.f) * PX_MM, 0.8f * PX_MM, IMAGE_L, IMAGE_H, motifM);
    }
    // ---- assemblage : couleur et hauteur (mm)
    std::vector<float> H(N, 0.f);
    im.couleur.resize(N * 4); im.normales.resize(N * 4);
    for (int y = 0; y < IMAGE_H; y++)
        for (int x = 0; x < IMAGE_L; x++) {
            const size_t i = (size_t)y * IMAGE_L + x;
            const float xm = (x + 0.5f) / PX_MM, ym = (y + 0.5f) / PX_MM;           // mm depuis le coin haut gauche
            const float d = -distanceBord(xm - PLAQUE_L_MM / 2, PLAQUE_H_MM / 2 - ym);  // distance au bord, positive dedans
            float c[3] = {(float)FOND[0], (float)FOND[1], (float)FOND[2]};
            auto melange = [&](const uint8_t* k, float t) { for (int q = 0; q < 3; q++) c[q] += (k[q] - c[q]) * std::clamp(t, 0.f, 1.f); };
            // bande de droite (dans le bourrelet : de 6,5 mm du haut au bas)
            const float aa = 0.5f / PX_MM;   // demi-pixel : bords nets mais sans escalier
            const float dansBande = lisse(-aa, aa, xm - bandeX0) * lisse(-aa, aa, d - 6.5f);
            melange(BANDE, dansBande);
            // double chevron (pointe vers le haut) : deux « ^ » de 18 mm, trait de 3 mm
            for (int k = 0; k < 2; k++) {
                const float cy = 22.f + k * 8.f, dx = std::fabs(xm - bandeCx), dy = ym - cy;
                const float ligne = std::fabs(dy - dx * 0.62f);                    // branches à ≈ 32°
                const float t = (1 - lisse(1.5f - aa, 1.5f + aa, ligne * 0.85f)) * (1 - lisse(9.f - aa, 9.f + aa, dx));
                melange(MOTIF, t * dansBande);
            }
            melange(MOTIF, motifM[i] * dansBande);
            // liseré noir de 1,5 mm centré à 5 mm du bord ; il fait le tour de la bande
            const float lis = 1 - lisse(0.75f - aa, 0.75f + aa, std::fabs(d - 5.f));
            melange(ENCRE, lis);
            melange(ENCRE, texteM[i]);
            for (int q = 0; q < 3; q++) im.couleur[i * 4 + q] = (uint8_t)std::lround(std::clamp(c[q] + grain(x, y) * 1.2f, 0.f, 255.f));
            im.couleur[i * 4 + 3] = 255;
            // hauteur : bourrelet embouti (0,6 mm, de 2,5 à 7 mm du bord), caractères (1 mm), grain du film (2 µm)
            const float bourrelet = 0.6f * lisse(2.5f, 3.5f, d) * (1 - lisse(6.0f, 7.0f, d));
            H[i] = bourrelet + grain(x * 7, y * 13) * 0.002f;
        }
    // relief des caractères : masque adouci (flou 3 × 3 deux fois : bords arrondis de l'emboutissage, ≈ 1 mm)
    std::vector<float> m = texteM, tmp(N);
    // WFS-484 : flou de ≈ 1 mm : 2 passes à 2 px / mm, 1 à 1 px / mm, aucune en dessous
    const int passes = IMAGE_L >= 1024 ? 2 : IMAGE_L >= 512 ? 1 : 0;
    for (int passe = 0; passe < passes; passe++) {
        for (int y = 0; y < IMAGE_H; y++)
            for (int x = 0; x < IMAGE_L; x++) {
                float s = 0; int n = 0;
                for (int j = -1; j <= 1; j++) for (int k = -1; k <= 1; k++) {
                    const int xx = x + k, yy = y + j;
                    if (xx < 0 || yy < 0 || xx >= IMAGE_L || yy >= IMAGE_H) continue;
                    s += m[(size_t)yy * IMAGE_L + xx]; n++;
                }
                tmp[(size_t)y * IMAGE_L + x] = s / n;
            }
        m.swap(tmp);
    }
    for (size_t i = 0; i < N; i++) H[i] += RELIEF_MM * m[i];
    // normales : n = (−∂H/∂u, −∂H/∂v, 1) en mm, u vers la droite, v vers le bas (repère de Renderer : T = +u, B = +v)
    const float pas = 1.f / PX_MM;
    for (int y = 0; y < IMAGE_H; y++)
        for (int x = 0; x < IMAGE_L; x++) {
            auto h = [&](int xx, int yy) { xx = std::clamp(xx, 0, IMAGE_L - 1); yy = std::clamp(yy, 0, IMAGE_H - 1); return H[(size_t)yy * IMAGE_L + xx]; };
            const float du = (h(x + 1, y) - h(x - 1, y)) / (2 * pas), dv = (h(x, y + 1) - h(x, y - 1)) / (2 * pas);
            vec3 n = normalize(vec3{-du, -dv, 1});
            const size_t i = ((size_t)y * IMAGE_L + x) * 4;
            im.normales[i] = (uint8_t)std::lround((n.x * 0.5f + 0.5f) * 255);
            im.normales[i + 1] = (uint8_t)std::lround((n.y * 0.5f + 0.5f) * 255);
            im.normales[i + 2] = (uint8_t)std::lround((n.z * 0.5f + 0.5f) * 255);
            im.normales[i + 3] = 255;
        }
    return true;
}

} // namespace wfs::plaque_joueur
