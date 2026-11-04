// WaForSpeed — manettes de jeu (WFS-22 / WFS-23) : partie pure (voir manette.h).
#include "manette.h"
#include <algorithm>
#include <cmath>
#include "../game/player.h"

namespace wfs {
namespace manette {

float zoneMorte(float v) {
    const float a = std::fabs(v);
    if (a < ZONE_MORTE) return 0.f;
    return std::copysign(std::min(1.f, (a - ZONE_MORTE) / (1.f - ZONE_MORTE)), v);
}

// RG-02 : gâchette droite = accélérer, gâchette gauche = freiner, stick X ou croix = braquer, A = nitro.
// Le braquage donné ici est la consigne (−1..1) : Player::update la suit progressivement (steerIn / steerOut).
void conduite(const Etat& e, DriveInput& out) {
    out = DriveInput{};
    float s = zoneMorte(e.stickX);
    const float croix = e.croixX != 0 ? e.croixX : (e.bouton[DROITE] ? 1.f : 0.f) - (e.bouton[GAUCHE] ? 1.f : 0.f);
    if (croix != 0) s = croix > 0 ? 1.f : -1.f;   // croix = touches flèches du web : braquage plein
    out.steer = std::clamp(s, -1.f, 1.f);
    out.gas = zoneMorte(e.gachetteD) > 0 || e.bouton[R2];
    out.brake = zoneMorte(e.gachetteG) > 0 || e.bouton[L2];
    out.nitro = e.bouton[A];
    if (out.nitro) out.gas = true;   // comme le bouton Nitro tactile (input_.gas = gaz || nitro)
}

static void croix(const Etat& e, int& cx, int& cy) {
    cx = e.croixX > 0.5f || e.bouton[DROITE] ? 1 : e.croixX < -0.5f || e.bouton[GAUCHE] ? -1 : 0;
    cy = e.croixY > 0.5f || e.bouton[BAS] ? 1 : e.croixY < -0.5f || e.bouton[HAUT] ? -1 : 0;
}

void directionMenu(const Etat& e, const Etat& precedent, bool& arme, int& dx, int& dy) {
    dx = dy = 0;
    int cx, cy, px, py;
    croix(e, cx, cy); croix(precedent, px, py);
    if (cx || cy) { if (cx != px || cy != py) { dx = cx; dy = cy; } return; }   // croix tenue : un seul pas
    // stick : une direction par poussée, réarmée quand il revient près du centre
    const float ax = std::fabs(e.stickX), ay = std::fabs(e.stickY), m = std::max(ax, ay);
    if (m < REARME_MENU) { arme = true; return; }
    if (!arme || m < SEUIL_MENU) return;
    arme = false;
    if (ax >= ay) dx = e.stickX > 0 ? 1 : -1; else dy = e.stickY > 0 ? 1 : -1;
}

// ---------- focus ----------
static std::vector<Zone> g_image, g_prec;
void zoneFocus(float x, float y, float w, float h, bool retour) { g_image.push_back({x, y, w, h, retour}); }
std::vector<Zone>& zonesImage() { return g_image; }
const std::vector<Zone>& zonesPrecedentes() { return g_prec; }
void finImage() { g_prec.swap(g_image); g_image.clear(); }

int voisin(const std::vector<Zone>& z, int i, int dx, int dy) {
    if (i < 0 || i >= (int)z.size()) return z.empty() ? -1 : 0;
    const float cx = z[i].x + z[i].w / 2, cy = z[i].y + z[i].h / 2;
    int best = i; float bestD = 1e30f;
    for (int j = 0; j < (int)z.size(); j++) {
        if (j == i) continue;
        const float ex = z[j].x + z[j].w / 2 - cx, ey = z[j].y + z[j].h / 2 - cy;
        const float avance = ex * dx + ey * dy, cote = std::fabs(dx ? ey : ex);
        if (avance <= 1.f) continue;   // pas dans la direction demandée
        const float d = avance + 2.f * cote;
        if (d < bestD) { bestD = d; best = j; }
    }
    return best;
}

// ---------- état ----------
static Etat g_etat;
static bool g_actif = false;
void bouton(Bouton b, bool appui) { g_etat.bouton[b] = appui; if (appui) g_actif = true; }
void axes(float sx, float sy, float gg, float gd, float hx, float hy) {
    g_etat.stickX = sx; g_etat.stickY = sy; g_etat.gachetteG = gg; g_etat.gachetteD = gd; g_etat.croixX = hx; g_etat.croixY = hy;
    if (zoneMorte(sx) != 0 || zoneMorte(sy) != 0 || zoneMorte(gg) != 0 || zoneMorte(gd) != 0 || hx != 0 || hy != 0) g_actif = true;
}
void toucherEcran() { g_actif = false; }
static bool g_parti = false;
void deconnexion() { g_parti = g_actif; g_etat = Etat{}; g_actif = false; }
bool deconnexionRecue() { const bool p = g_parti; g_parti = false; return p; }
bool actif() { return g_actif; }
const Etat& etat() { return g_etat; }

} // namespace manette
} // namespace wfs
