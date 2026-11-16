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

// bague chrome + centre sombre : base du Start/Stop et de la pilule chrome ; rend l'épaisseur de la bague.
// WFS-557 (maquettes 01, 19, 20, 22 : bouton de démarrage comparé au zoom) : bague en métal sombre (gris acier, plus clair en
// haut), mince (7 % du diamètre), fins reflets clairs sur le bord (haut gauche marqué, bas droit léger), ombre portée sombre ;
// centre noir profond ; liseré fin (≈ 1,5 % du diamètre) orange doré, collé au bord du centre, sans halo.
float bagueChrome(Renderer& r, float x, float y, float w, float h, float op, bool appuye, const vec4& lisere, float lueurLisere,
                  bool inactif = false) {
    const float R = std::min(w, h) / 2;
    const bool rond = std::fabs(w - h) < 1.f;
    const float t = std::clamp(h * 0.056f, 3.f, 11.f);   // épaisseur moyenne de la bague (maquette 19 : 7 px pour 128)
    // WFS-557 (maquette 19 mesurée pixel par pixel) : bouton vu un peu d'en haut : le centre noir et le liseré descendent de 3 % du
    // diamètre ; la bague est épaisse en haut et fine en bas
    const float dy = rond ? h * 0.035f : 0.f;
    lueur(r, x, y + h * 0.02f, w, h, R, std::max(4.f, h * 0.08f), {0, 0, 0, 0.45f * op});   // ombre portée
    plein(r, x, y, w, h, R, hex(0x55585E, op), hex(0x2A2E33, op));                          // métal sombre : #303236 sur les côtés
    bord(r, x, y, w, h, R, 1, hex(0x0E0F12, 0.9f * op));                                    // arête extérieure
    if (rond) {   // reflet du plafond : bande blanche presque pure au milieu du haut de la bague, fondue sur les côtés (0 = haut)
        const float cx = x + w / 2, cy = y + h / 2, rb = R - (t + dy) * 0.42f, eb = (t + dy) * 0.32f;
        for (int i = 0; i < 10; i++) {   // 10 couches de plus en plus courtes : fondu doux aux deux bouts, blanc pur au milieu
            const float demi = (0.46f - 0.035f * i) * PI;
            dessin2d::arc(r, cx, cy, rb, eb * 1.3f, 2 * PI - demi, 2 * PI + demi, hex(0xFEFFFD, 0.17f * op));
        }
        dessin2d::arc(r, cx, cy, R - 1.5f, 1.2f, 0.55f * PI, 0.95f * PI, hex(0x6A7072, 0.5f * op));   // éclat discret en bas à droite
    } else {
        plein(r, x + R * 0.5f, y + 1, w - R, std::max(1.f, h * 0.012f), 0, hex(0xF2F4F7, 0.6f * op), hex(0xF2F4F7, 0.6f * op));
    }
    // centre noir pur (enfoncé à l'appui : un peu plus clair), descendu de dy
    const float xi = x + t, yi = y + t + dy, wi = w - 2 * t, hi = h - 2 * t, Ri = std::max(0.f, R - t);
    bord(r, xi - 2, yi - 2, wi + 4, hi + 4, Ri + 2, 2.5f, hex(0x2A2522, op));   // gorge sombre entre la bague et le liseré
    if (inactif) plein(r, xi, yi, wi, hi, Ri, hex(0x0C0D11, op), hex(INACTIF_CENTRE, op));
    else if (appuye) plein(r, xi, yi, wi, hi, Ri, hex(0x1A1B1F, op), hex(0x101114, op));
    else plein(r, xi, yi, wi, hi, Ri, hex(0x000000, op), hex(0x030303, op));
    // liseré fin, collé au bord du centre ; plus pâle en haut (reflet), comme la maquette
    const float g = std::max(1.f, h * 0.008f), ep = std::clamp(h * 0.018f, 1.2f, 3.f);
    const float xl = xi + g, yl = yi + g, wl = wi - 2 * g, hl = hi - 2 * g, Rl = std::max(0.f, Ri - g);
    vec2 p[MAX_SOMMETS];
    const int n = contourArrondi(p, xl, yl, wl, hl, Rl);
    const vec4 c{lisere.x, lisere.y, lisere.z, lisere.w * op};
    if (lueurLisere > 0) dessin2d::halo(r, p, n, std::max(2.f, h * 0.02f), {c.x, c.y, c.z, 0.20f * lueurLisere * (appuye ? 1.6f : 1.f) * op});
    dessin2d::contour(r, p, n, ep, c);
    if (rond && !inactif) dessin2d::arc(r, xl + wl / 2, yl + hl / 2, Rl - ep / 2, ep, 1.82f * PI, 2.18f * PI, hex(0xE6C48A, 0.8f * op));
    return t + g;   // (le centre est en plus descendu de dy : startStop en tient compte)
}

} // namespace

void startStop(Renderer& r, float x, float y, float w, float h, const std::string& titre, float op, bool appuye,
               const std::string& sousTitre, float px, bool grise) {
    // inactif : liseré éteint, sans lueur, texte et icône gris clair pleins (lisibles, contraste ≥ 4,5 : 1)
    const float m = grise ? bagueChrome(r, x, y, w, h, op, false, hex(INACTIF_LISERE), 0, true)
                          : bagueChrome(r, x, y, w, h, op, appuye, hex(appuye ? 0xFFC070 : 0xE09B4A), 0.45f);   // WFS-557 : liseré orange doré (#E09B4A mesuré)
    const vec4 blanc = hex(grise ? INACTIF_TEXTE : TEXTE, op), orange = grise ? hex(INACTIF_LISERE, op) : hex(appuye ? 0xFFB088 : 0xF28C5A, op);   // icône pêche (maquettes 01, 19)
    const float xi = x + m, wi = w - 2 * m, hi = h - 2 * m;
    if (px <= 0) px = std::min(hi * (sousTitre.empty() ? 0.42f : 0.34f), 40.f * h / 60.f);
    const bool rond = h >= w * 0.75f;   // bouton presque rond (maquettes 19, 27) : icône au-dessus du texte
    const float rayonInt = std::min(wi, hi) / 2;
    if (rond) {
        const float s = px * 0.62f, total = s + px * 0.30f + px + (sousTitre.empty() ? 0 : px * 0.8f);
        float yy = y + h / 2 - total / 2 - h * 0.05f;   // icône et texte un peu au-dessus du centre (maquette 19 mesurée)
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
                          : bagueChrome(r, x, y, w, h, op, appuye, hex(appuye ? 0xFFC070 : 0xE09B4A), 0.45f);
    const float xi = x + m, wi = w - 2 * m, hi = h - 2 * m, rayonInt = std::min(wi, hi) / 2;
    const float s = hi * 0.36f, xs = xi + rayonInt * 0.55f + s / 2;
    iconeMarche(r, xs, y + h / 2, s, grise ? hex(INACTIF_LISERE, op) : hex(appuye ? 0xFFB088 : 0xF28C5A, op));
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

// WFS-557 : voiture à hayon vue de profil, au trait (maquette 01 : 117 × 47 dans la pilule « GARAGE »), centrée en (cx, cy)
void iconeHayon(Renderer& r, float cx, float cy, float w, float ep, const vec4& c) {
    // WFS-557 (maquette 01 au zoom) : voiture tournée vers la gauche : bouclier avant droit, capot et pare-brise en une longue
    // pente, toit, lunette arrière inclinée, petit coffre plat, arrière droit ; roues entières (pneu et moyeu) sous la caisse
    const float u = w / 117.f, x0 = cx - w / 2, y0 = cy - 20 * u;   // repère de la maquette : 0..117 en x, 0..42 en y
    auto P = [&](float x, float y) { return vec2{x0 + x * u, y0 + y * u}; };
    const float rr = 8.5f * u, yr = 35 * u;
    const vec2 caisse[] = {P(16, 33), P(1, 33), P(0, 25), P(3, 22), P(23, 5), P(31, 1), P(72, 1), P(97, 12), P(114, 15),
                           P(117, 22), P(116, 33), P(98, 33)};
    dessin2d::trait(r, caisse, (int)(sizeof caisse / sizeof caisse[0]), ep, c);
    const vec2 bas[2] = {P(34, 33), P(79, 33)};
    dessin2d::trait(r, bas, 2, ep, c);
    for (float xr : {25.f, 88.f}) {   // roues entières : pneu et moyeu
        const vec2 q = P(xr, 0);
        dessin2d::anneau(r, q.x, y0 + yr, rr - ep / 2, rr + ep / 2, c);
        dessin2d::anneau(r, q.x, y0 + yr, rr * 0.42f - ep / 2, rr * 0.42f + ep / 2, c);
    }
}

void rondMenu(Renderer& r, float cx, float cy, float k, const std::string& titre, const std::string& sousTitre, float op, bool appuye, bool grise) {
    const float R = 117 * k, Ri = 105 * k, Ra = 98 * k, D = PI / 180;   // bague (maquette 01 : 117 / 105), anneau orange (98)
    // ombre portée, puis halo blanc au-dessus et en haut à gauche (lumière du plafond sur le chrome)
    dessin2d::couronne(r, cx, cy + 6 * k, R - 6 * k, R + 16 * k, {0, 0, 0, 0.55f * op}, {0, 0, 0, 0});
    // halo du plafond seulement au-dessus du bouton : anneau décentré vers le haut (le reste est caché par la bague et le centre)
    dessin2d::couronne(r, cx - 3 * k, cy - 30 * k, R - 30 * k, R - 6 * k, {1, 1, 1, 0.55f * op}, {1, 1, 1, 0});
    // bague chrome : gris acier ; reflets fondus (haut blanc pur, droite, bas : arête blanche fine), côtés plus sombres
    dessin2d::anneau(r, cx, cy, Ri, R, hex(0x55565A, op));
    dessin2d::anneau(r, cx, cy, Ri, R, hex(0x2E2F33, 0.35f * op), 215 * D, 300 * D);
    for (int i = 0; i < 8; i++) {   // reflet du haut : 8 couches de plus en plus courtes (bouts fondus)
        const float demi = (78 - 8 * i) * D;
        dessin2d::anneau(r, cx, cy, Ri + 1.5f * k, R - 1.5f * k, {1, 1, 1, 0.20f * op}, -demi - 10 * D, demi - 10 * D);
    }
    for (int i = 0; i < 4; i++) {   // éclat à droite et liseré blanc du bas
        dessin2d::anneau(r, cx, cy, Ri + 3 * k, R - 3 * k, {0.95f, 0.95f, 0.97f, 0.20f * op}, (64 + 5 * i) * D, (112 - 5 * i) * D);
        dessin2d::anneau(r, cx, cy, R - 4.5f * k, R - 1.5f * k, {1, 1, 1, 0.16f * op}, (140 + 8 * i) * D, (230 - 8 * i) * D);
    }
    dessin2d::arc(r, cx, cy, R - 0.6f * k, 1.2f * k, 0, 2 * PI, hex(0xA2A3A7, 0.8f * op));    // arête extérieure
    dessin2d::arc(r, cx, cy, R - 1 * k, 2.2f * k, -62 * D, 62 * D, hex(0xFFFFFF, 0.9f * op));     // liseré blanc vif du haut (maquette 01)
    dessin2d::arc(r, cx, cy, Ri + 0.6f * k, 1.4f * k, 0, 2 * PI, hex(0x151618, 0.95f * op));  // arête intérieure sombre
    // centre gris foncé (#2A2B2D), un peu plus clair en bas, plus clair à l'appui
    dessin2d::disqueRadial(r, cx, cy, Ri, cx, cy - 30 * k, Ri * 1.15f, hex(appuye ? 0x3A3B3F : 0x2D2E31, op), hex(appuye ? 0x2E2F33 : 0x1F2023, op));
    dessin2d::couronne(r, cx, cy + Ri * 0.65f, 0, Ri * 0.45f, hex(0x4A4B4E, 0.30f * op), hex(0x4A4B4E, 0));
    // anneau orange fin (#F59A45), plus pâle en haut
    dessin2d::arc(r, cx, cy, Ra, 2.6f * k, 0, 2 * PI, grise ? hex(INACTIF_LISERE, op) : hex(appuye ? 0xFFB070 : 0xF59A45, op));
    if (!grise) dessin2d::arc(r, cx, cy, Ra, 2.6f * k, -30 * D, 30 * D, hex(0xF2C9B3, 0.8f * op));
    // icône (−39), titre crème centré à −6, sous-titre gris centré à +22
    iconeMarche(r, cx, cy - 39 * k, 24 * k, grise ? hex(INACTIF_LISERE, op) : hex(appuye ? 0xFFB088 : 0xF28C5A, op));
    ui::textBoite(r, cx, cy - 6 * k - 18 * k, 2 * Ra - 16 * k, 36 * k, 36 * k, titre, grise ? hex(INACTIF_TEXTE, op) : hex(0xEAD8D0, op), ui::CENTRE, 0.02f, false, true);
    if (!sousTitre.empty()) ui::textBoite(r, cx, cy + 22 * k - 12 * k, 2 * Ra - 30 * k, 24 * k, 24 * k, sousTitre, hex(0xC6C7CC, op), ui::CENTRE, 0.03f, false, true);
}

void piluleMenu(Renderer& r, float x, float y, float w, float h, float k, const std::string& texte, float op, bool appuye) {
    const float R = h / 2;
    lueur(r, x, y + 6 * k, w, h, R, 16 * k, {0, 0, 0, 0.55f * op});                     // ombre portée
    plein(r, x, y, w, h, R, hex(0x9A9DA1, op), hex(0x3C3F44, op));                        // bague chrome
    plein(r, x + 4 * k, y + 4 * k, w - 8 * k, h - 8 * k, R - 4 * k, hex(0x6E7176, op), hex(0x2F3236, op));
    bord(r, x, y, w, h, R, 1.2f * k, hex(0x1A1B1D, 0.85f * op));
    bord(r, x + 1.5f * k, y + 1.5f * k, w - 3 * k, h - 3 * k, R - 1.5f * k, 1.6f * k, hex(0xC9CBCE, 0.55f * op));               // arête claire
    plein(r, x + R * 0.85f, y + 2.5f * k, w - 1.7f * R, 3 * k, 1.5f * k, hex(0xFFFFFF, 0.9f * op), hex(0xFFFFFF, 0.9f * op));     // reflet blanc du haut
    for (int i = 0; i < 4; i++)   // WFS-557 (maquette 01 au zoom) : reflet blanc sur le bout gauche de la bague (de 10 h à 8 h)
        dessin2d::arc(r, x + R, y + R, R - 6 * k, 3 * k, (222 + 8 * i) * PI / 180, (318 - 8 * i) * PI / 180, {1, 1, 1, 0.18f * op});
    // fond noir bleuté et liseré cyan néon (maquette : 33 à gauche, 22 en haut, 26 à droite, 27 en bas)
    const float xi = x + 33 * k, yi = y + 22 * k, wi = w - 59 * k, hi = h - 49 * k, Rt = hi / 2;
    plein(r, xi - 3 * k, yi - 3 * k, wi + 6 * k, hi + 6 * k, Rt + 3 * k, hex(0x15171A, op), hex(0x0B0C0E, op));   // gorge
    plein(r, xi, yi, wi, hi, Rt, hex(appuye ? 0x24303A : 0x1B2023, op), hex(appuye ? 0x14191E : 0x080C0F, op));
    lueur(r, xi, yi, wi, hi, Rt, 8 * k, hex(ACCENT2, (appuye ? 0.50f : 0.32f) * op));
    bord(r, xi, yi, wi, hi, Rt, 1.8f * k, hex(0xADE6ED, op));
    // voiture à hayon au trait cyan (lueur puis trait clair), « GARAGE » gris clair en italique (maquette : x 253, centre y 520)
    const float vx = x + 141 * k, vy = y + 84 * k;
    iconeHayon(r, vx, vy, 117 * k, 7 * k, hex(0x00B8CC, 0.28f * op));   // lueur cyan
    iconeHayon(r, vx, vy, 117 * k, 3 * k, hex(0x5FE0EC, op));
    iconeHayon(r, vx, vy, 117 * k, 1.3f * k, hex(0xC9F4F8, op));   // cœur clair du tube
    ui::textBoite(r, x + 223 * k, y + 77 * k - 31 * k, w - 250 * k, 62 * k, 62 * k, texte, hex(0xD2D3D5, op), ui::GAUCHE, 0.03f, false, true);
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

void icone(Renderer& r, Icone i, float cx, float cy, float s, const vec4& c) {
    const float ep = std::max(1.2f, s * 0.09f), h = s / 2;
    switch (i) {
        case Icone::Aucune: return;
        case Icone::Recommencer: {   // flèche circulaire ↺ : arc ouvert en haut à gauche, pointe en haut
            const float R = s * 0.40f;
            dessin2d::arc(r, cx, cy, R, ep, -0.30f * PI, 1.25f * PI, c);   // 0 = haut, sens horaire
            const float ax = cx + R * std::sin(-0.30f * PI), ay = cy - R * std::cos(-0.30f * PI), d = s * 0.20f;
            const vec2 pointe[3] = {{ax - d, ay - d * 0.6f}, {ax + d * 0.15f, ay + d * 0.05f}, {ax - d * 0.2f, ay + d}};
            dessin2d::trait(r, pointe, 3, ep, c);
            return;
        }
        case Icone::Sortie: {   // porte ⇥ : crochet ouvert à droite, flèche vers la droite
            const vec2 porte[4] = {{cx + h * 0.05f, cy - h * 0.85f}, {cx - h * 0.85f, cy - h * 0.85f}, {cx - h * 0.85f, cy + h * 0.85f},
                                   {cx + h * 0.05f, cy + h * 0.85f}};
            dessin2d::trait(r, porte, 4, ep, c);
            const vec2 tige[2] = {{cx - h * 0.25f, cy}, {cx + h * 0.95f, cy}};
            dessin2d::trait(r, tige, 2, ep, c);
            const vec2 pointe[3] = {{cx + h * 0.55f, cy - h * 0.4f}, {cx + h * 0.95f, cy}, {cx + h * 0.55f, cy + h * 0.4f}};
            dessin2d::trait(r, pointe, 3, ep, c);
            return;
        }
        case Icone::Engrenage: {   // engrenage au trait (maquettes 01 et 19) : anneau fin, 8 dents courtes, moyeu
            const float R0 = s * 0.27f, R1 = s * 0.44f;
            for (int k = 0; k < 8; k++) {
                const float a = k * PI / 4, ca = std::cos(a), sa = std::sin(a);
                const vec2 d[2] = {{cx + ca * (R0 + ep * 0.5f), cy + sa * (R0 + ep * 0.5f)}, {cx + ca * R1, cy + sa * R1}};
                dessin2d::trait(r, d, 2, ep * 1.3f, c);
            }
            dessin2d::anneau(r, cx, cy, R0 - ep * 0.5f, R0 + ep * 0.5f, c);
            dessin2d::anneau(r, cx, cy, s * 0.09f - ep * 0.5f, s * 0.09f + ep * 0.5f, c);
            return;
        }
        case Icone::Crayon: {   // crayon ✎ incliné à 45°
            const float d = s * 0.42f, l = s * 0.12f;
            const vec2 corps[5] = {{cx - d + l * 0.4f, cy + d}, {cx - d, cy + d - l * 0.4f}, {cx + d - l, cy - d}, {cx + d, cy - d + l}, {cx - d + l * 0.4f, cy + d}};
            dessin2d::trait(r, corps, 5, ep, c);
            return;
        }
        case Icone::Etoile: {   // étoile à 5 branches au trait
            vec2 p[11];
            for (int k = 0; k < 10; k++) {
                const float a = -0.5f * PI + k * PI / 5, R = (k % 2 ? 0.21f : 0.48f) * s;
                p[k] = {cx + R * std::cos(a), cy + 0.06f * s + R * std::sin(a)};
            }
            p[10] = p[0];
            dessin2d::trait(r, p, 11, ep, c);
            return;
        }
        case Icone::Document: {   // feuille au coin plié et deux lignes de texte
            const float w = s * 0.36f, hh = s * 0.46f, pli = s * 0.16f;
            const vec2 f[6] = {{cx - w, cy - hh}, {cx + w - pli, cy - hh}, {cx + w, cy - hh + pli}, {cx + w, cy + hh}, {cx - w, cy + hh}, {cx - w, cy - hh}};
            dessin2d::trait(r, f, 6, ep, c);
            const vec2 l1[2] = {{cx - w * 0.5f, cy + hh * 0.05f}, {cx + w * 0.5f, cy + hh * 0.05f}}, l2[2] = {{cx - w * 0.5f, cy + hh * 0.45f}, {cx + w * 0.5f, cy + hh * 0.45f}};
            dessin2d::trait(r, l1, 2, ep, c);
            dessin2d::trait(r, l2, 2, ep, c);
            return;
        }
        case Icone::Croix: iconeCroix(r, cx, cy, s, c); return;
        case Icone::Ecran: {   // moniteur : écran arrondi et pied
            vec2 p[MAX_SOMMETS];
            const int n = contourArrondi(p, cx - s * 0.46f, cy - s * 0.38f, s * 0.92f, s * 0.60f, s * 0.08f);
            dessin2d::contour(r, p, n, ep, c);
            const vec2 pied[2] = {{cx, cy + s * 0.22f}, {cx, cy + s * 0.40f}}, socle[2] = {{cx - s * 0.22f, cy + s * 0.42f}, {cx + s * 0.22f, cy + s * 0.42f}};
            dessin2d::trait(r, pied, 2, ep, c);
            dessin2d::trait(r, socle, 2, ep, c);
            return;
        }
        case Icone::Volant: {   // volant : anneau, moyeu et trois branches
            dessin2d::anneau(r, cx, cy, s * 0.46f - ep, s * 0.46f, c);
            dessin2d::anneau(r, cx, cy, s * 0.10f, s * 0.10f + ep, c);
            const vec2 b1[2] = {{cx - s * 0.44f, cy - s * 0.02f}, {cx - s * 0.11f, cy}}, b2[2] = {{cx + s * 0.11f, cy}, {cx + s * 0.44f, cy - s * 0.02f}},
                       b3[2] = {{cx, cy + s * 0.11f}, {cx, cy + s * 0.44f}};
            dessin2d::trait(r, b1, 2, ep, c); dessin2d::trait(r, b2, 2, ep, c); dessin2d::trait(r, b3, 2, ep, c);
            return;
        }
        case Icone::HautParleur: {   // haut-parleur et deux ondes
            const vec2 hp[7] = {{cx - s * 0.45f, cy - s * 0.14f}, {cx - s * 0.25f, cy - s * 0.14f}, {cx, cy - s * 0.38f}, {cx, cy + s * 0.38f},
                                {cx - s * 0.25f, cy + s * 0.14f}, {cx - s * 0.45f, cy + s * 0.14f}, {cx - s * 0.45f, cy - s * 0.14f}};
            dessin2d::trait(r, hp, 7, ep, c);
            dessin2d::arc(r, cx + s * 0.02f, cy, s * 0.20f, ep, 0.2f * PI, 0.8f * PI, c);
            dessin2d::arc(r, cx + s * 0.02f, cy, s * 0.38f, ep, 0.22f * PI, 0.78f * PI, c);
            return;
        }
    }
}

void plaque(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op, bool appuye) {
    const float b = std::max(1.f, h * 0.05f), R = h * 0.08f, bande = h * 0.30f, d = appuye ? 1.5f : 0;
    lueur(r, x, y, w, h, R, std::max(4.f, h * 0.30f), {1, 1, 1, 0.12f * op});
    plein(r, x - b, y - b + d, w + 2 * b, h + 2 * b, R + b, hex(0x2A2D35, op), hex(0x15171C, op));
    plein(r, x, y + d, w, h, R, hex(appuye ? 0xD8D6D0 : 0xF4F3EF, op), hex(appuye ? 0xBDBBB5 : 0xDCDAD4, op));
    plein(r, x, y + d, bande + R, h, R, hex(0xFF7A1A, op), hex(ACCENT, op));
    r.uiRect(x + bande, y + d, R, h, hex(appuye ? 0xD8D6D0 : 0xF0EFEB, op));
    ui::textBoite(r, x + bande + (w - bande) / 2, y + d, w - bande - h * 0.4f, h, h * 0.62f, texte, hex(0x15171C, op), ui::CENTRE, 0.10f);
}

void verreIcone(Renderer& r, float x, float y, float w, float h, Icone i, const std::string& texte, float op, bool appuye, float px,
                const OptionsVerre& o) {
    const float R = o.rayon >= 0 ? o.rayon : std::min(h * 0.24f, w / 2);
    lueur(r, x, y, w, h, R, std::max(3.f, h * 0.18f), {0, 0, 0, 0.35f * op});   // ombre douce sous le verre
    plein(r, x, y, w, h, R, {38 / 255.f, 41 / 255.f, 52 / 255.f, (appuye ? 0.95f : 0.80f) * op}, {18 / 255.f, 20 / 255.f, 27 / 255.f, 0.86f * op});
    if (o.bord) bord(r, x, y, w, h, R, std::max(1.5f, h * 0.03f), hex(o.bord, (appuye ? 1.f : 0.85f) * op));
    else bord(r, x, y, w, h, R, std::max(1.f, h * 0.02f), {1, 1, 1, (appuye ? 0.38f : 0.20f) * op});
    const vec4 c = o.couleurTexte ? vec4{o.couleurTexte->x, o.couleurTexte->y, o.couleurTexte->z, o.couleurTexte->w * op} : hex(TEXTE, op);
    if (px <= 0) px = h * 0.40f;
    const float s = px * 0.86f;
    if (texte.empty()) { icone(r, i, x + w / 2, y + h / 2, s, c); return; }
    const float ecart = i == Icone::Aucune ? 0 : px * 0.65f, si = i == Icone::Aucune ? 0 : s;
    const float tw = std::min(ui::width(px, texte, o.espacement), w - h * 0.6f - si - ecart);
    const float x0 = x + w / 2 - (si + ecart + tw) / 2;
    icone(r, i, x0 + si / 2, y + h / 2, s, c);
    ui::textBoite(r, x0 + si + ecart, y + (h - px) / 2, tw, px, px, texte, c, ui::GAUCHE, o.espacement, false, true);
}

} // namespace wfs::ui::boutons
