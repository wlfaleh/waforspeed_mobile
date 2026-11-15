// WaForSpeed — boutons de l'interface au modèle R19 (WFS-549) : voir boutons.h.
#include "boutons.h"
#include <algorithm>
#include <cmath>
#include "text.h"
#include "../game/commandes_tactiles.h"   // dessin2d : polygones, contours et halos à bords lissés (WFS-146)
#include "../render/renderer.h"

namespace wfs::ui::boutons {
namespace {

// jetons du skill design-street-racing (codes sRGB)
vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
constexpr unsigned TEXTE = 0xF2F4F8, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, ON_ACCENT = 0x0A0B10;

dessin2d::Teinte degradeV(const vec4& haut, const vec4& bas, float y0, float y1) { return {haut, bas, y0, y1}; }

// rectangle arrondi rempli d'un dégradé vertical
void plein(Renderer& r, float x, float y, float w, float h, float rayon, const vec4& haut, const vec4& bas) {
    vec2 p[MAX_SOMMETS];
    const int n = contourArrondi(p, x, y, w, h, rayon);
    dessin2d::polygone(r, p, n, degradeV(haut, bas, y, y + h));
}
void bord(Renderer& r, float x, float y, float w, float h, float rayon, float ep, const vec4& c) {
    vec2 p[MAX_SOMMETS];
    const int n = contourArrondi(p, x, y, w, h, rayon);
    dessin2d::contour(r, p, n, ep, c);
}
void lueur(Renderer& r, float x, float y, float w, float h, float rayon, float flou, const vec4& c) {
    vec2 p[MAX_SOMMETS];
    const int n = contourArrondi(p, x, y, w, h, rayon);
    dessin2d::halo(r, p, n, flou, c);
}

// icône « marche / arrêt » (power) centrée en (cx, cy), diamètre s
void iconeMarche(Renderer& r, float cx, float cy, float s, const vec4& c) {
    const float ep = std::max(1.5f, s * 0.13f), rayon = s * 0.42f;
    dessin2d::arc(r, cx, cy, rayon, ep, 0.22f * PI, 1.78f * PI, c);   // anneau ouvert en haut (0 = haut, sens horaire)
    const vec2 barre[2] = {{cx, cy - s * 0.52f}, {cx, cy - s * 0.04f}};
    dessin2d::trait(r, barre, 2, ep, c);
}
// petite voiture au trait (pilule « Garage » de la maquette 01), largeur s
void iconeVoiture(Renderer& r, float cx, float cy, float s, const vec4& c) {
    const float ep = std::max(1.5f, s * 0.06f), hw = s / 2, hh = s * 0.22f;
    const vec2 caisse[8] = {{cx - hw, cy + hh * 0.5f}, {cx - hw, cy - hh * 0.2f}, {cx - hw * 0.55f, cy - hh * 0.35f},
                            {cx - hw * 0.25f, cy - hh * 1.2f}, {cx + hw * 0.35f, cy - hh * 1.2f}, {cx + hw * 0.7f, cy - hh * 0.35f},
                            {cx + hw, cy - hh * 0.1f}, {cx + hw, cy + hh * 0.5f}};
    dessin2d::trait(r, caisse, 8, ep, c);
    const float rr = s * 0.11f;
    dessin2d::anneau(r, cx - hw * 0.5f, cy + hh * 0.65f, rr - ep / 2, rr + ep / 2, c);
    dessin2d::anneau(r, cx + hw * 0.5f, cy + hh * 0.65f, rr - ep / 2, rr + ep / 2, c);
}
// croix « ✕ » de taille s
void iconeCroix(Renderer& r, float cx, float cy, float s, const vec4& c) {
    const float ep = std::max(1.5f, s * 0.14f), d = s * 0.38f;
    const vec2 a[2] = {{cx - d, cy - d}, {cx + d, cy + d}}, b[2] = {{cx + d, cy - d}, {cx - d, cy + d}};
    dessin2d::trait(r, a, 2, ep, c);
    dessin2d::trait(r, b, 2, ep, c);
}

// bague chrome + centre sombre : base du Start/Stop et de la pilule chrome ; rend l'épaisseur de la bague
float bagueChrome(Renderer& r, float x, float y, float w, float h, float op, bool appuye, const vec4& lisere, float lueurLisere,
                  bool inactif = false) {
    const float R = std::min(w, h) / 2;
    const float t = std::clamp(h * 0.085f, 3.f, 12.f);   // épaisseur de la bague chrome
    // reflet doux autour de la bague (lumière du parking sur le chrome)
    lueur(r, x, y, w, h, R, std::max(4.f, h * 0.12f), {1, 1, 1, 0.10f * op});
    // bague chrome : bord haut très clair, milieu gris, bas clair (reflet), comme un bouton de démarrage poli
    plein(r, x, y, w, h, R, hex(0xF1F3F6, op), hex(0x6E7480, op));
    plein(r, x + t * 0.35f, y + h * 0.5f, w - t * 0.7f, h * 0.5f - t * 0.35f, std::max(0.f, R - t * 0.35f), hex(0x8A909B, 0), hex(0xD5D9E0, 0.55f * op));
    bord(r, x, y, w, h, R, 1, hex(0x2A2D35, 0.9f * op));   // arête extérieure
    // centre sombre (enfoncé à l'appui : plus clair)
    const float xi = x + t, yi = y + t, wi = w - 2 * t, hi = h - 2 * t, Ri = std::max(0.f, R - t);
    bord(r, xi - 1, yi - 1, wi + 2, hi + 2, Ri + 1, 1.5f, hex(0x101114, 0.95f * op));   // gorge entre la bague et le centre
    if (inactif) plein(r, xi, yi, wi, hi, Ri, hex(0x0C0D11, op), hex(INACTIF_CENTRE, op));
    else if (appuye) plein(r, xi, yi, wi, hi, Ri, hex(0x2A2E38, op), hex(0x1A1D25, op));
    else plein(r, xi, yi, wi, hi, Ri, hex(0x05060A, op), hex(0x15171D, op));
    // liseré allumé, un peu en retrait du bord du centre
    const float g = std::max(2.f, h * 0.035f), ep = std::max(1.5f, h * 0.022f);
    const float xl = xi + g, yl = yi + g, wl = wi - 2 * g, hl = hi - 2 * g, Rl = std::max(0.f, Ri - g);
    vec2 p[MAX_SOMMETS];
    const int n = contourArrondi(p, xl, yl, wl, hl, Rl);
    const vec4 c{lisere.x, lisere.y, lisere.z, lisere.w * op};
    if (lueurLisere > 0) dessin2d::halo(r, p, n, std::max(3.f, h * 0.06f), {c.x, c.y, c.z, lueurLisere * (appuye ? 1.4f : 1.f) * op});
    dessin2d::contour(r, p, n, ep, c);
    return t + g;
}

} // namespace

void startStop(Renderer& r, float x, float y, float w, float h, const std::string& titre, float op, bool appuye,
               const std::string& sousTitre, float px, bool grise) {
    // inactif : liseré éteint, sans lueur, texte et icône gris clair pleins (lisibles, contraste ≥ 4,5 : 1)
    const float m = grise ? bagueChrome(r, x, y, w, h, op, false, hex(INACTIF_LISERE), 0, true)
                          : bagueChrome(r, x, y, w, h, op, appuye, hex(appuye ? 0xFF8C33 : ACCENT), 0.45f);
    const vec4 blanc = hex(grise ? INACTIF_TEXTE : TEXTE, op), orange = grise ? hex(INACTIF_LISERE, op) : hex(appuye ? 0xFF8C33 : ACCENT, op);
    const float xi = x + m, wi = w - 2 * m, hi = h - 2 * m;
    if (px <= 0) px = std::min(hi * (sousTitre.empty() ? 0.42f : 0.34f), 40.f * h / 60.f);
    const bool rond = h >= w * 0.75f;   // bouton presque rond (maquettes 19, 27) : icône au-dessus du texte
    const float rayonInt = std::min(wi, hi) / 2;
    if (rond) {
        const float s = px * 0.75f, total = s + px * 0.25f + px + (sousTitre.empty() ? 0 : px * 0.8f);
        float yy = y + h / 2 - total / 2;
        iconeMarche(r, x + w / 2, yy + s / 2, s, orange);
        yy += s + px * 0.25f;
        ui::textBoite(r, x + w / 2, yy, wi - rayonInt * 0.3f, px, px, titre, blanc, ui::CENTRE, 0.04f, false, true);
        if (!sousTitre.empty())
            ui::textBoite(r, x + w / 2, yy + px, wi - rayonInt * 0.4f, px * 0.8f, px * 0.62f, sousTitre, hex(0xC9CDDA, op), ui::CENTRE, 0.04f, false, true);
        return;
    }
    // bouton allongé : icône à gauche du texte, l'ensemble centré dans le centre sombre
    const float s = px * 0.85f, ecart = px * 0.45f, marge = rayonInt * 0.6f;
    const float largeurDispo = wi - 2 * marge - s - ecart;
    const float tw = std::min(largeurDispo, ui::width(px, titre, 0.04f) * 1.02f);
    const float sw = sousTitre.empty() ? 0 : std::min(largeurDispo, ui::width(px * 0.6f, sousTitre, 0.04f));
    const float bloc = s + ecart + std::max(tw, sw);
    const float x0 = xi + wi / 2 - bloc / 2;
    const float hTexte = sousTitre.empty() ? px : px * 1.65f, yt = y + h / 2 - hTexte / 2;
    iconeMarche(r, x0 + s / 2, y + h / 2, s, orange);
    ui::textBoite(r, x0 + s + ecart, yt, largeurDispo, px, px, titre, blanc, ui::GAUCHE, 0.04f, false, true);
    if (!sousTitre.empty())
        ui::textBoite(r, x0 + s + ecart, yt + px * 1.05f, largeurDispo, px * 0.6f, px * 0.6f, sousTitre, hex(0xC9CDDA, op), ui::GAUCHE, 0.04f, false, true);
}

ZoneTexte startStopFond(Renderer& r, float x, float y, float w, float h, float op, bool appuye, bool grise) {
    const float m = grise ? bagueChrome(r, x, y, w, h, op, false, hex(INACTIF_LISERE), 0, true)
                          : bagueChrome(r, x, y, w, h, op, appuye, hex(appuye ? 0xFF8C33 : ACCENT), 0.45f);
    const float xi = x + m, wi = w - 2 * m, hi = h - 2 * m, rayonInt = std::min(wi, hi) / 2;
    const float s = hi * 0.36f, xs = xi + rayonInt * 0.55f + s / 2;
    iconeMarche(r, xs, y + h / 2, s, grise ? hex(INACTIF_LISERE, op) : hex(appuye ? 0xFF8C33 : ACCENT, op));
    const float x0 = xs + s / 2 + s * 0.3f, x1 = xi + wi - rayonInt * 0.55f;
    return {x0, std::max(0.f, x1 - x0)};
}

void autocollant(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op, bool appuye, float px,
                 const std::string& sousTitre) {
    // étiquette collée de travers (maquette 27) : bord haut qui monte vers la droite, coins à peine décalés ; reste dans la zone
    const float d = std::min(h * 0.14f, w * 0.06f), dx = appuye ? 1.5f : 0, dy = appuye ? 1.5f : 0;
    const vec2 p[4] = {{x + d * 0.4f + dx, y + d + dy}, {x + w, y + dy}, {x + w - d * 0.4f, y + h - d * 0.6f + dy},
                       {x + dx, y + h}};
    dessin2d::halo(r, p, 4, std::max(3.f, h * 0.10f), {0, 0, 0, 0.45f * op});   // ombre portée sur le panneau
    dessin2d::polygone(r, p, 4, degradeV(hex(appuye ? 0xD9DCE1 : 0xF7F8FA, op), hex(appuye ? 0xA9AEB7 : 0xC8CCD3, op), y, y + h));
    dessin2d::contour(r, p, 4, std::max(1.f, h * 0.025f), hex(0xFFFFFF, op));   // bord blanc de l'autocollant
    const vec4 encre = hex(ON_ACCENT, op);
    if (px <= 0) px = h * (sousTitre.empty() ? 0.42f : 0.36f);
    const float mx = std::max(d * 2, h * 0.25f);
    if (sousTitre.empty()) {
        ui::textBoite(r, x + w / 2 + dx, y + h / 2 - px / 2 + d * 0.2f + dy, w - 2 * mx, px, px, texte, encre, ui::CENTRE, 0.06f, false, true);
    } else {
        const float yt = y + h / 2 - px * 0.85f + dy;
        ui::textBoite(r, x + mx + dx, yt, w - 2 * mx, px, px, texte, encre, ui::GAUCHE, 0.06f, false, true);
        ui::textBoite(r, x + mx + dx, yt + px * 1.1f, w - 2 * mx, px * 0.5f, px * 0.42f, sousTitre, hex(0x3A3E48, op), ui::GAUCHE, 0, false, false);
    }
}

void piluleChrome(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op, bool appuye, float px,
                  const std::string& sousTitre) {
    const float m = bagueChrome(r, x, y, w, h, op, appuye, hex(ACCENT2), 0.40f);
    const float xi = x + m, wi = w - 2 * m, hi = h - 2 * m;
    if (px <= 0) px = hi * (sousTitre.empty() ? 0.48f : 0.40f);
    const vec4 cyan = hex(ACCENT2, op), blanc = hex(TEXTE, op);
    const float s = std::min(hi * 0.95f, wi * 0.22f), xIcone = xi + hi * 0.35f + s / 2;
    iconeVoiture(r, xIcone, y + h / 2, s, cyan);
    const float xt = xIcone + s / 2 + hi * 0.3f, lt = xi + wi - hi * 0.35f - xt;
    const float hTexte = sousTitre.empty() ? px : px * 1.6f, yt = y + h / 2 - hTexte / 2;
    ui::textBoite(r, xt, yt, lt, px, px, texte, blanc, ui::GAUCHE, 0.04f, false, true);
    if (!sousTitre.empty())
        ui::textBoite(r, xt, yt + px * 1.05f, lt, px * 0.55f, px * 0.5f, sousTitre, hex(0xC9CDDA, op), ui::GAUCHE, 0, false, false);
}

void prix(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op, bool appuye, float px, bool grise) {
    const float R = std::min(h * 0.28f, 14.f * h / 44.f);
    if (px <= 0) px = h * 0.42f;
    if (grise) {   // inactif (hors ligne, achat en cours) : bouton gris neutre, sans lueur, texte clair lisible
        plein(r, x, y, w, h, R, hex(INACTIF_FOND_HAUT, op), hex(INACTIF_FOND_BAS, op));
        bord(r, x, y, w, h, R, 1, {1, 1, 1, 0.12f * op});
        ui::textBoite(r, x + w / 2, y + (h - px) / 2, w - h * 0.5f, px, px, texte, hex(INACTIF_TEXTE, op), ui::CENTRE, 0.06f, false, true);
        return;
    }
    lueur(r, x, y, w, h, R, std::max(4.f, h * 0.22f), hex(ACCENT, (appuye ? 0.55f : 0.35f) * op));
    plein(r, x, y, w, h, R, hex(appuye ? 0xFFA352 : 0xFF8A2E, op), hex(appuye ? 0xFF7A14 : ACCENT, op));
    bord(r, x, y, w, h, R, 1, hex(0xFFC08A, 0.8f * op));
    ui::textBoite(r, x + w / 2, y + (h - px) / 2, w - h * 0.5f, px, px, texte, hex(ON_ACCENT, op), ui::CENTRE, 0.06f, false, true);
}

void selecteurFond(Renderer& r, float x, float y, float w, float h, float op) {
    plein(r, x, y, w, h, h / 2, {1, 1, 1, 0.07f * op}, {1, 1, 1, 0.03f * op});
    plein(r, x, y, w, h, h / 2, {20 / 255.f, 22 / 255.f, 31 / 255.f, 0.55f * op}, {14 / 255.f, 16 / 255.f, 22 / 255.f, 0.55f * op});
    bord(r, x, y, w, h, h / 2, 1, {1, 1, 1, 0.16f * op});
}

void selecteurChoix(Renderer& r, float x, float y, float w, float h, const std::string& texte, bool actif, float op, bool appuye,
                    float px, bool grise) {
    if (px <= 0) px = h * 0.40f;
    if (grise) {   // choix inactif (l'invité du salon) : texte gris clair plein, actif = pilule grise à texte sombre
        if (actif) plein(r, x, y, w, h, h / 2, hex(0xA4A9B6, op), hex(0x9499A6, op));
        ui::textBoite(r, x + w / 2, y + (h - px) / 2, w - h * 0.5f, px, px, texte, actif ? hex(ON_ACCENT, op) : hex(INACTIF_TEXTE, op), ui::CENTRE, 0.10f);
        return;
    }
    if (actif) {
        lueur(r, x, y, w, h, h / 2, std::max(3.f, h * 0.15f), {0, 0, 0, 0.35f * op});
        plein(r, x, y, w, h, h / 2, hex(0xFFFFFF, op), hex(appuye ? 0xD5D9E0 : 0xE9ECF1, op));
    } else if (appuye) {
        plein(r, x, y, w, h, h / 2, {1, 1, 1, 0.16f * op}, {1, 1, 1, 0.10f * op});
    }
    ui::textBoite(r, x + w / 2, y + (h - px) / 2, w - h * 0.5f, px, px, texte, actif ? hex(ON_ACCENT, op) : hex(TEXTE, op), ui::CENTRE, 0.10f);
}

void verreOrange(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op, bool appuye, float px) {
    const float R = h * 0.28f;
    lueur(r, x, y, w, h, R, std::max(4.f, h * 0.25f), hex(ACCENT, (appuye ? 0.45f : 0.28f) * op));
    plein(r, x, y, w, h, R, {30 / 255.f, 26 / 255.f, 24 / 255.f, (appuye ? 0.92f : 0.80f) * op}, {20 / 255.f, 18 / 255.f, 18 / 255.f, 0.80f * op});
    bord(r, x, y, w, h, R, std::max(1.5f, h * 0.04f), hex(appuye ? 0xFF8C33 : ACCENT, op));
    if (px <= 0) px = h * 0.40f;
    const vec4 orange = hex(appuye ? 0xFFA352 : 0xFF8A3D, op);
    const float s = px * 0.75f, ecart = px * 0.5f, tw = std::min(ui::width(px, texte, 0.12f), w - h * 0.6f - s - ecart);
    const float x0 = x + w / 2 - (s + ecart + tw) / 2;
    iconeCroix(r, x0 + s / 2, y + h / 2, s, orange);
    ui::textBoite(r, x0 + s + ecart, y + (h - px) / 2, tw, px, px, texte, orange, ui::GAUCHE, 0.12f, false, true);
}

void verre(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op, bool appuye, float px,
           const vec4* couleurTexte) {
    const float R = h * 0.22f;
    plein(r, x, y, w, h, R, {38 / 255.f, 41 / 255.f, 52 / 255.f, (appuye ? 0.95f : 0.82f) * op}, {22 / 255.f, 24 / 255.f, 32 / 255.f, 0.82f * op});
    bord(r, x, y, w, h, R, 1, {1, 1, 1, (appuye ? 0.35f : 0.18f) * op});
    if (px <= 0) px = h * 0.38f;
    const vec4 c = couleurTexte ? vec4{couleurTexte->x, couleurTexte->y, couleurTexte->z, couleurTexte->w * op} : hex(TEXTE, op);
    ui::textBoite(r, x + w / 2, y + (h - px) / 2, w - h * 0.5f, px, px, texte, c, ui::CENTRE, 0.12f, false, true);
}

} // namespace wfs::ui::boutons
