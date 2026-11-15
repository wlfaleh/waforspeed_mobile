// WaForSpeed — WFS-548 (interface R19) : panneau de verre dépoli, sur-titres, couleurs d'état, fond flouté : voir verre.h.
#include "verre.h"
#include <algorithm>
#include <cmath>
#include "../game/commandes_tactiles.h"   // dessin2d::contour, halo (bords lissés d'un pixel)
#include "../render/renderer.h"
#include "../core/system.h"

namespace wfs::verre {
namespace {

constexpr int NP = NP_CONTOUR;       // contour complet (≤ 64 points de dessin2d)

vec4 melange(const vec4& a, const vec4& b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}

float distance(vec2 a, vec2 b) { return std::hypot(a.x - b.x, a.y - b.y); }

UiVertex sommet(vec2 p, const vec4& c) { return {p.x, p.y, -1, -1, c.x, c.y, c.z, c.w}; }

} // namespace

void barreMarque(Renderer& r, float x, float y, float w, float h, float a) {
    if (w <= 0 || h <= 0) return;
    // un seul uiQuad à couleurs par sommet : dégradé continu, sans marche entre deux tranches
    const vec4 c0 = degradeMarque(0, a), c1 = degradeMarque(1, a);
    const UiVertex q[4] = {sommet({x, y}, c0), sommet({x + w, y}, c1), sommet({x + w, y + h}, c1), sommet({x, y + h}, c0)};
    r.uiQuad(q);
}

void panneau(Renderer& r, float x, float y, float w, float h, const Style& s) {
    if (w <= 1 || h <= 1) return;
    vec2 p[NP];
    contourArrondi(x, y, w, h, s.rayon, p);
    const float op = std::clamp(s.opacite, 0.f, 1.f);
    // ombre portée douce (maquettes 19 à 33 : les fenêtres se détachent du fond flouté)
    if (s.ombre) dessin2d::halo(r, p, NP, std::max(6.f, s.rayon * 1.2f), {0, 0, 0, 0.30f * op * s.force});
    // remplissage : dégradé radial centré au centre-haut (≈ 30 % de la hauteur), plus sombre vers les bords et les coins.
    // Verre teinté du skill design-street-racing (rgba(20, 22, 31, .72)) éclairci au centre, alpha un peu plus fort au bord.
    const vec4 centre = melange(hexa(0x14161F, 0.50f), hexa(0x2C2F39, 0.74f), s.force);
    const vec4 bord = melange(hexa(0x0A0B10, 0.45f), hexa(0x111218, 0.84f), s.force);
    const vec2 g{x + w * 0.5f, y + h * 0.30f};
    float R = 1;
    for (const vec2& q : p) R = std::max(R, distance(q, g));
    vec4 c[NP];
    for (int i = 0; i < NP; i++) {
        const float t = std::clamp(distance(p[i], g) / R, 0.f, 1.f);
        c[i] = melange(centre, bord, t * t * (3 - 2 * t));
        c[i].w *= op;
    }
    vec4 cg = centre; cg.w *= op;
    for (int i = 0; i < NP; i++) {   // éventail depuis le centre du dégradé : un triangle par segment du contour
        const int j = (i + 1) % NP;
        const UiVertex q[4] = {sommet(g, cg), sommet(p[i], c[i]), sommet(p[j], c[j]), sommet(p[j], c[j])};
        r.uiQuad(q);
    }
    // liseré fin et clair (blanc ≈ 20 %), ou couleur d'état ; il lisse aussi le bord du remplissage
    const vec4 l = s.bord ? hexa(s.bord, 0.85f * op) : vec4{1, 1, 1, (s.force >= 1 ? 0.20f : 0.12f) * op};
    dessin2d::contour(r, p, NP, std::max(s.lisere, 1.f), l);
}

void surTitre(Renderer& r, float x, float y, float px, const std::string& texte, ui::Align align, float opacite) {
    ui::text(r, x, y, px, ui::majuscules(texte), hexa(SUR_TITRE, opacite), align, 0.22f, false);
}

void fondFloute(Renderer& r) { r.flouFond = 1.f; }

void assombrir(Renderer& r, float W, float H, float a) { r.uiRect(0, 0, W, H, hexa(0x0A0B10, a)); }

namespace {
bool g_demande = false, g_couvert = false;   // demande de l'image en cours ; état lu par les écrans du dessous à cette image

// premier système de l'image (ordre 1) : la demande de l'image précédente devient l'état de cette image
class FenetreCouvre : public System {
public:
    const char* name() const override { return "verre-couvre"; }
    void drawUi(Renderer&, const GameView&) override { g_couvert = g_demande; g_demande = false; }
    void onEcran(Ecran, Ecran, const GameView&) override { g_couvert = g_demande = false; }   // changement d'écran : rien de couvert
};
} // namespace

void fenetreSurEcran(Renderer& r) { fondFloute(r); g_demande = true; }
bool couvert() { return g_couvert; }

} // namespace wfs::verre

namespace wfs { WFS_SYSTEM(verre::FenetreCouvre, 1) }   // avant tout dessin de l'interface (systèmes 100 à 499)
