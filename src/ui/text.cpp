// WaForSpeed — texte de l'interface (WFS-26) : atlas SDF construit au démarrage avec stb_truetype.
#include "text.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <vector>
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../render/renderer.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../../third_party/stb/stb_truetype.h"

namespace wfs {
namespace ui {
namespace {

constexpr float BASE_PX = 48.f;          // taille de référence de l'atlas
constexpr int PAD = 6, ATLAS = 1024;     // marge du champ de distance, côté de l'atlas
constexpr unsigned char ONEDGE = 128;    // valeur du bord (0,5 dans le shader)

struct Glyph { float u0, v0, u1, v1, w, h, xoff, yoff, adv; };
struct Font {
    bool ready = false;
    std::vector<uint8_t> ttf;
    stbtt_fontinfo info{};
    float scale = 1, ascent = 0, capH = 0;   // capH : hauteur des capitales (pivot de l'italique, WFS-48)
    std::unordered_map<int, Glyph> glyphs;
    Image atlas;
} F;

// décodage UTF-8 → points de code
std::vector<int> codepoints(const std::string& s) {
    std::vector<int> out;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = (unsigned char)s[i];
        int cp = c, n = 0;
        if (c >= 0xF0) { cp = c & 7; n = 3; } else if (c >= 0xE0) { cp = c & 15; n = 2; } else if (c >= 0xC0) { cp = c & 31; n = 1; }
        for (int k = 1; k <= n && i + k < s.size(); k++) cp = (cp << 6) | ((unsigned char)s[i + k] & 63);
        out.push_back(cp); i += 1 + n;
    }
    return out;
}

bool load(Renderer& r) {
    if (F.ready) return true;
    if (!readAsset("fonts/Rajdhani-Bold.ttf", F.ttf) || !stbtt_InitFont(&F.info, F.ttf.data(), 0)) { WFS_ERR("Police introuvable"); return false; }
    F.scale = stbtt_ScaleForPixelHeight(&F.info, BASE_PX);
    int asc, desc, gap; stbtt_GetFontVMetrics(&F.info, &asc, &desc, &gap);
    F.ascent = asc * F.scale;
    { int x0, y0, x1, y1; F.capH = stbtt_GetCodepointBox(&F.info, 'H', &x0, &y0, &x1, &y1) ? y1 * F.scale : F.ascent * 0.7f; }
    // jeu de caractères : ASCII, Latin-1 (accents, « », ×, ·), œ Œ ’ … € et espace fine insécable
    std::vector<int> cps;
    for (int c = 32; c < 127; c++) cps.push_back(c);
    for (int c = 160; c < 256; c++) cps.push_back(c);
    for (int c : {0x152, 0x153, 0x2019, 0x2026, 0x20AC, 0x202F}) cps.push_back(c);
    std::vector<uint8_t> px((size_t)ATLAS * ATLAS, 0);
    int x = 1, y = 1, rowH = 0;
    for (int cp : cps) {
        int adv, lsb; stbtt_GetCodepointHMetrics(&F.info, cp, &adv, &lsb);
        Glyph g{}; g.adv = adv * F.scale;
        if (!stbtt_FindGlyphIndex(&F.info, cp) && cp != 32 && cp != 160 && cp != 0x202F) continue;
        int w = 0, h = 0, xo = 0, yo = 0;
        unsigned char* sdf = stbtt_GetCodepointSDF(&F.info, F.scale, cp, PAD, ONEDGE, (float)ONEDGE / PAD, &w, &h, &xo, &yo);
        if (sdf) {
            if (x + w + 1 > ATLAS) { x = 1; y += rowH + 1; rowH = 0; }
            if (y + h + 1 > ATLAS) { stbtt_FreeSDF(sdf, nullptr); WFS_WARN("Atlas de police plein"); break; }
            for (int j = 0; j < h; j++) std::memcpy(&px[(size_t)(y + j) * ATLAS + x], sdf + (size_t)j * w, (size_t)w);
            g.u0 = (float)x / ATLAS; g.v0 = (float)y / ATLAS; g.u1 = (float)(x + w) / ATLAS; g.v1 = (float)(y + h) / ATLAS;
            g.w = (float)w; g.h = (float)h; g.xoff = (float)xo; g.yoff = (float)yo;
            x += w + 1; if (h > rowH) rowH = h;
            stbtt_FreeSDF(sdf, nullptr);
        }
        F.glyphs[cp] = g;
    }
    // espaces insécables = espace
    if (F.glyphs.count(32)) { F.glyphs[160] = F.glyphs[32]; F.glyphs[0x202F] = F.glyphs[32]; F.glyphs[0x202F].adv *= 0.6f; }
    std::vector<uint8_t> rgba((size_t)ATLAS * ATLAS * 4);
    for (size_t i = 0; i < px.size(); i++) { uint8_t v = px[i]; rgba[i * 4] = rgba[i * 4 + 1] = rgba[i * 4 + 2] = rgba[i * 4 + 3] = v; }
    F.atlas = r.vk().uploadTexture(rgba.data(), ATLAS, ATLAS, false);
    r.uiSetFontAtlas(F.atlas);
    F.ready = true;
    WFS_LOG("Police : %zu caractères dans l'atlas", F.glyphs.size());
    return true;
}

void unload(Renderer& r) {
    if (!F.ready) return;
    r.vk().waitIdle();
    r.vk().destroy(F.atlas);
    r.uiSetFontAtlas(Image{});   // WFS-100 (RG-W2) : l'interface lit la texture blanche, jamais l'atlas détruit
    F.glyphs.clear(); F.ready = false;
}

// pente : tan de l'angle d'italique (0 = droit) ; chaque quad est cisaillé autour du milieu des capitales (pivot)
void run(Renderer* r, float x, float y, float px, const std::vector<int>& cps, const vec4& c, float spacing, float* outW,
         float pente = 0) {
    const float s = px / BASE_PX, pivot = y + (F.ascent - F.capH * 0.5f) * s;
    float pen = 0; int prev = 0;
    for (int cp : cps) {
        auto it = F.glyphs.find(cp);
        if (it == F.glyphs.end()) it = F.glyphs.find('?');
        if (it == F.glyphs.end()) continue;
        const Glyph& g = it->second;
        if (prev) pen += stbtt_GetCodepointKernAdvance(&F.info, prev, cp) * F.scale * s;
        if (r && g.w > 0) {
            const float gy = y + (F.ascent + g.yoff) * s, gh = g.h * s;   // bas du quad décalé de (pivot − bas) × pente
            r->uiGlyph(x + pen + g.xoff * s + (pivot - gy - gh) * pente, gy, g.w * s, gh, g.u0, g.v0, g.u1, g.v1, c, gh * pente);
        }
        pen += g.adv * s + spacing * px;
        prev = cp;
    }
    if (outW) *outW = cps.empty() ? 0 : pen - spacing * px;
}

} // namespace

bool fontReady() { return F.ready; }
bool chargerPolice(Renderer& r) { return load(r); }

float width(float px, const std::string& s, float spacing) {
    if (!F.ready) return 0;
    float w = 0; run(nullptr, 0, 0, px, codepoints(s), {}, spacing, &w);
    return w;
}

void text(Renderer& r, float x, float y, float px, const std::string& s, const vec4& color, Align align, float spacing, bool ombre,
          bool italique) {
    if (!F.ready || s.empty()) return;
    auto cps = codepoints(s);
    float w = 0; run(nullptr, 0, 0, px, cps, {}, spacing, &w);
    if (align == CENTRE) x -= w * 0.5f; else if (align == DROITE) x -= w;
    const float pente = italique ? std::tan(ITALIQUE_DEG * 3.14159265f / 180.f) : 0.f;
    if (ombre) {   // contour sombre décalé : lisible sur la route et le ciel
        const float o = std::max(1.f, px * 0.05f);
        run(&r, x + o, y + o, px, cps, {0, 0, 0, 0.55f * color.w}, spacing, nullptr, pente);
    }
    run(&r, x, y, px, cps, color, spacing, nullptr, pente);
}

std::string nombre(long long v) {
    std::string d = std::to_string(v < 0 ? -v : v), out;
    for (size_t i = 0; i < d.size(); i++) {
        if (i && (d.size() - i) % 3 == 0) out += "\xE2\x80\xAF";   // espace fine insécable
        out += d[i];
    }
    return v < 0 ? "-" + out : out;
}

std::string decimal(double v, int chiffres) {
    char b[64]; std::snprintf(b, sizeof b, "%.*f", chiffres, v);
    std::string s = b; for (char& c : s) if (c == '.') c = ',';
    return s;
}

std::string chrono(double t) {
    if (t < 0) t = 0;
    int m = (int)(t / 60); double sec = t - m * 60;
    char b[32]; std::snprintf(b, sizeof b, "%d:%04.1f", m, sec);
    return b;
}

std::string majuscules(const std::string& s) {
    std::string out;
    for (int cp : codepoints(s)) {
        if (cp >= 'a' && cp <= 'z') cp -= 32;
        else if ((cp >= 0xE0 && cp <= 0xFE && cp != 0xF7)) cp -= 32;   // à→À … þ→Þ
        else if (cp == 0x153) cp = 0x152;                                // œ→Œ
        if (cp < 0x80) out += (char)cp;
        else if (cp < 0x800) { out += (char)(0xC0 | (cp >> 6)); out += (char)(0x80 | (cp & 63)); }
        else { out += (char)(0xE0 | (cp >> 12)); out += (char)(0x80 | ((cp >> 6) & 63)); out += (char)(0x80 | (cp & 63)); }
    }
    return out;
}

// ---------------------------------------------------------------- système : charge la police pour tous les modules
// (ordre 400 : avant le HUD et les menus). Avec WFS_TEXTE_ESSAI=1 (banc d'essai), affiche une planche de test.
class Texte : public System {
public:
    const char* name() const override { return "texte"; }
    void init(Renderer& r, const GameView&) override { load(r); essai_ = std::getenv("WFS_TEXTE_ESSAI") != nullptr; }
    void drawUi(Renderer& r, const GameView& g) override {
        if (!essai_ || !F.ready) return;
        const float W = g.uiWidth, H = g.uiHeight, u = H / 100.f;
        r.uiRect(W * 0.05f, H * 0.14f, W * 0.9f, H * 0.62f, {0.02f, 0.03f, 0.06f, 0.78f});
        text(r, W * 0.08f, H * 0.17f, 9 * u, majuscules("Démo du texte"), CYAN, GAUCHE, 0.16f);
        text(r, W * 0.08f, H * 0.30f, 5.2f * u, "Éléphant à côté d'un garçon : « déjà vu », où ça ? Œuvre, ça va !", BLANC);
        text(r, W * 0.08f, H * 0.39f, 5.2f * u, "àâäéèêëîïôöùûüç ÀÂÉÈÊËÎÔÙÛÇ — 0123456789", BLANC);
        text(r, W * 0.08f, H * 0.50f, 3.2f * u, majuscules("chrono ") + chrono(83.4) + "   " + majuscules("score ") + nombre(12345) +
             "   " + decimal(1.2, 1) + " km   × 3 · 100 %", GRIS, GAUCHE, 0.08f);
        text(r, W * 0.92f, H * 0.58f, 11 * u, "182", ORANGE, DROITE);
        text(r, W * 0.92f, H * 0.70f, 3.2f * u, "KM/H", ROUGE, DROITE, 0.16f);
        text(r, W * 0.5f, H * 0.62f, 4 * u, "Centré : WaForSpeed", CYAN, CENTRE);
        // WFS-48 : même texte droit puis italique (12°), côte à côte
        const std::string ex = "Droit 1:23,4 · 12 345", ei = "Italique 1:23,4 · 12 345";
        text(r, W * 0.08f, H * 0.68f, 5 * u, ex, BLANC);
        text(r, W * 0.08f + width(5 * u, ex) + 5 * u, H * 0.68f, 5 * u, ei, ORANGE, GAUCHE, 0.f, true, true);
    }
    void shutdown(Renderer& r) override { unload(r); }
private:
    bool essai_ = false;
};

} // namespace ui

WFS_SYSTEM(ui::Texte, 400)

} // namespace wfs
