// WaForSpeed — WFS-161 (ST-1) : géométrie du kit d'architecture (voir archi_geo.h).
#include "archi_geo.h"
#include <algorithm>
#include <cmath>

namespace wfs::archi {
namespace {

constexpr float SOCLE = 1.f;          // murs prolongés sous le sol (pente du terrain, trottoir) : aucun jour en pied de mur
constexpr float APPUI_SAILLIE = 0.05f, APPUI_EP = 0.05f, APPUI_DEBORD = 0.05f;   // appui de fenêtre (m)
constexpr float BALCON_PROF = 0.9f, BALCON_EP = 0.15f, BALCON_DEBORD = 0.3f;      // dalle de balcon (m)
constexpr float GARDE_CORPS = 1.0f, GC_EP = 0.04f;    // garde-corps : 1,00 m (NF P01-012, archi.garde_corps.hauteur)
constexpr float LOGGIA = 1.0f;        // profondeur d'une loggia (m)
constexpr float ACROTERE = 0.5f, ACROTERE_EP = 0.2f;  // muret en rive de toiture-terrasse (m)
constexpr float PENTE_ZINC = 35.f, PENTE_TUILES = 17.f, DEBORD_TUILES = 0.3f;   // degrés ; débord de toit (m)

// quad a b c d (ordre trigonométrique vu du côté de la normale n)
void quad(Maillage& m, vec3 a, vec3 b, vec3 c, vec3 d, vec3 n, vec2 ua, vec2 ub, vec2 uc, vec2 ud) {
    const uint32_t k = (uint32_t)m.v.size();
    m.v.push_back({a, n, ua}); m.v.push_back({b, n, ub}); m.v.push_back({c, n, uc}); m.v.push_back({d, n, ud});
    m.i.insert(m.i.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
}
// rectangle du plan z = z0 (normale ±z), uv en mètres
void rectZ(Maillage& m, float x0, float x1, float y0, float y1, float z0, float sens) {
    if (x1 - x0 < 1e-4f || y1 - y0 < 1e-4f) return;
    const vec3 n{0, 0, sens};
    if (sens > 0) quad(m, {x0, y0, z0}, {x1, y0, z0}, {x1, y1, z0}, {x0, y1, z0}, n, {x0, y0}, {x1, y0}, {x1, y1}, {x0, y1});
    else quad(m, {x1, y0, z0}, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0}, n, {-x1, y0}, {-x0, y0}, {-x0, y1}, {-x1, y1});
}
// rectangle du plan x = x0 (normale ±x), z de z0 à z1
void rectX(Maillage& m, float x0, float z0, float z1, float y0, float y1, float sens) {
    if (std::fabs(z1 - z0) < 1e-4f || y1 - y0 < 1e-4f) return;
    const float za = std::min(z0, z1), zb = std::max(z0, z1);
    const vec3 n{sens, 0, 0};
    if (sens > 0) quad(m, {x0, y0, zb}, {x0, y0, za}, {x0, y1, za}, {x0, y1, zb}, n, {-zb, y0}, {-za, y0}, {-za, y1}, {-zb, y1});
    else quad(m, {x0, y0, za}, {x0, y0, zb}, {x0, y1, zb}, {x0, y1, za}, n, {za, y0}, {zb, y0}, {zb, y1}, {za, y1});
}
// rectangle du plan y = y0 (normale ±y)
void rectY(Maillage& m, float x0, float x1, float z0, float z1, float y0, float sens) {
    if (x1 - x0 < 1e-4f || std::fabs(z1 - z0) < 1e-4f) return;
    const float za = std::min(z0, z1), zb = std::max(z0, z1);
    const vec3 n{0, sens, 0};
    if (sens > 0) quad(m, {x0, y0, zb}, {x1, y0, zb}, {x1, y0, za}, {x0, y0, za}, n, {x0, zb}, {x1, zb}, {x1, za}, {x0, za});
    else quad(m, {x0, y0, za}, {x1, y0, za}, {x1, y0, zb}, {x0, y0, zb}, n, {x0, za}, {x1, za}, {x1, zb}, {x0, zb});
}
// pavé aligné (sans dessous si dessous = false)
void pave(Maillage& m, float x0, float x1, float y0, float y1, float z0, float z1, bool dessous = true) {
    rectZ(m, x0, x1, y0, y1, std::max(z0, z1), 1); rectZ(m, x0, x1, y0, y1, std::min(z0, z1), -1);
    rectX(m, x0, z0, z1, y0, y1, -1); rectX(m, x1, z0, z1, y0, y1, 1);
    rectY(m, x0, x1, z0, z1, y1, 1); if (dessous) rectY(m, x0, x1, z0, z1, y0, -1);
}

float profondeurTableau(const Plan& p, const Baie& b) {
    const ReglesStyle& S = regles(p.style);
    return b.niveau > 0 && p.etage == Etage::Loggias ? LOGGIA : S.retrait;
}

void ouverture(const Plan& p, const Baie& b, int lod, Batiment& out) {
    const float x0 = b.x - b.l / 2, x1 = b.x + b.l / 2, y0 = b.y, y1 = b.y + b.h;
    const bool vitre = b.type == Ouverture::Fenetre || b.type == Ouverture::PorteFenetre || b.type == Ouverture::Vitrine;
    Maillage& fond = vitre ? out.part[VITRE] : out.part[MENUISERIE];
    if (lod >= 2) {   // loin : la baie est peinte à plat, 1 cm devant le mur plein
        const uint32_t k = (uint32_t)fond.v.size();
        rectZ(fond, x0, x1, y0, y1, 0.01f, 1);
        if (vitre) for (uint32_t j = k; j < fond.v.size(); j++) fond.v[j].uv = {(fond.v[j].p.x - x0) / b.l, (fond.v[j].p.y - y0) / b.h};
        return;
    }
    const float d = profondeurTableau(p, b);
    Maillage& tab = out.part[ENCADREMENT];
    rectX(tab, x0, 0, -d, y0, y1, 1);    // tableau gauche (normale vers l'ouverture)
    rectX(tab, x1, 0, -d, y0, y1, -1);   // tableau droit
    rectY(tab, x0, x1, 0, -d, y1, -1);   // sous-face du linteau
    rectY(tab, x0, x1, 0, -d, y0, 1);    // fond d'appui
    const uint32_t k = (uint32_t)fond.v.size();
    rectZ(fond, x0, x1, y0, y1, -d, 1);  // vitre ou porte au fond du tableau
    if (vitre) for (uint32_t j = k; j < fond.v.size(); j++) fond.v[j].uv = {(fond.v[j].p.x - x0) / b.l, (fond.v[j].p.y - y0) / b.h};
    if (lod >= 1) return;
    // appui en saillie sous les fenêtres (pas sous les portes ni les vitrines)
    if (b.type == Ouverture::Fenetre)
        pave(tab, x0 - APPUI_DEBORD, x1 + APPUI_DEBORD, y0 - APPUI_EP, y0, -0.02f, APPUI_SAILLIE);
    // balcon : dalle + garde-corps de 1,00 m (porte-fenêtre ou fenêtre d'étage d'un plan à balcons)
    if (b.balcon) {
        const float bx0 = x0 - BALCON_DEBORD, bx1 = x1 + BALCON_DEBORD, yb = p.niveauY(b.niveau);
        pave(tab, bx0, bx1, yb - BALCON_EP, yb, 0, BALCON_PROF);
        Maillage& gc = out.part[METAL];
        pave(gc, bx0, bx1, yb, yb + GARDE_CORPS, BALCON_PROF - GC_EP, BALCON_PROF, false);
        pave(gc, bx0, bx0 + GC_EP, yb, yb + GARDE_CORPS, 0, BALCON_PROF, false);
        pave(gc, bx1 - GC_EP, bx1, yb, yb + GARDE_CORPS, 0, BALCON_PROF, false);
    } else if (b.type == Ouverture::PorteFenetre && b.niveau > 0) {   // porte-fenêtre sans balcon : garde-corps dans le tableau
        pave(out.part[METAL], x0, x1, y0, y0 + GARDE_CORPS, -0.1f, -0.1f + GC_EP, false);
    }
}

// façade avant percée : par niveau, bandes de mur entre les ouvertures, au-dessus et au-dessous de chacune
void facade(const Plan& p, int lod, Batiment& out) {
    Maillage& mur = out.part[MUR];
    const float haut = p.hRdc + p.etages * p.hEtage;   // haut du dernier niveau (la corniche est au-dessus)
    rectZ(mur, 0, p.largeur, -SOCLE, 0, 0, 1);         // socle
    for (int n = 0; n <= p.etages; n++) {
        const float yb = p.niveauY(n), yt = yb + p.niveauH(n);
        if (lod >= 2) { rectZ(mur, 0, p.largeur, yb, yt, 0, 1); continue; }
        std::vector<const Baie*> b;
        for (const Baie& x : p.baies) if (x.niveau == n) b.push_back(&x);
        std::sort(b.begin(), b.end(), [](const Baie* a, const Baie* c) { return a->x < c->x; });
        float prev = 0;
        for (const Baie* o : b) {
            const float x0 = o->x - o->l / 2, x1 = o->x + o->l / 2;
            rectZ(mur, prev, x0, yb, yt, 0, 1);
            rectZ(mur, x0, x1, yb, o->y, 0, 1);
            rectZ(mur, x0, x1, o->y + o->h, yt, 0, 1);
            prev = x1;
        }
        rectZ(mur, prev, p.largeur, yb, yt, 0, 1);
    }
    rectZ(mur, 0, p.largeur, haut, p.hauteur(), 0, 1);   // bande sous la corniche
    for (const Baie& b : p.baies) ouverture(p, b, lod, out);
}

void toiture(const Plan& p, float D, int lod, Batiment& out) {
    const float H = p.hauteur(), L = p.largeur;
    Maillage& toit = out.part[TOIT];
    Maillage& mur = out.part[MUR];
    if (p.toit == Toit::Terrasse) {
        rectY(toit, 0, L, 0, -D, H, 1);
        // acrotère sur les 4 côtés (dessus seulement visible de loin)
        pave(mur, 0, L, H, H + ACROTERE, -ACROTERE_EP, 0, false);
        pave(mur, 0, L, H, H + ACROTERE, -D, -D + ACROTERE_EP, false);
        pave(mur, 0, ACROTERE_EP, H, H + ACROTERE, -D, 0, false);
        pave(mur, L - ACROTERE_EP, L, H, H + ACROTERE, -D, 0, false);
        if (p.edicules && lod == 0 && L > 6 && D > 6) {   // cage d'escalier et machinerie d'ascenseur
            const float cx = L * 0.5f, cz = -D * 0.6f;
            pave(mur, cx - 1.6f, cx + 1.6f, H, H + 2.6f, cz - 1.4f, cz + 1.4f, false);
            rectY(toit, cx - 1.6f, cx + 1.6f, cz - 1.4f, cz + 1.4f, H + 2.6f, 1);
        }
        return;
    }
    // toit à deux pentes, faîtage parallèle à la façade (zinc 35°, tuiles 17° avec débord)
    const float pente = (p.toit == Toit::Zinc ? PENTE_ZINC : PENTE_TUILES) * PI / 180.f;
    const float deb = p.toit == Toit::Tuiles ? DEBORD_TUILES : 0.f;
    const float demi = D / 2, hf = std::tan(pente) * demi;   // hauteur du faîtage au-dessus de l'égout
    const float zf = -demi, ye = H - std::tan(pente) * deb;  // égout abaissé du débord
    const vec3 a{-deb, ye, deb}, b{L + deb, ye, deb}, c{L + deb, H + hf, zf}, d{-deb, H + hf, zf};
    const vec3 nAv = normalize(cross(b - a, d - a));
    quad(toit, a, b, c, d, nAv, {a.x, 0}, {b.x, 0}, {b.x, length(d - a)}, {a.x, length(d - a)});
    const vec3 e{L + deb, ye, -D - deb}, f{-deb, ye, -D - deb};
    const vec3 nAr = normalize(cross(f - e, c - e));
    quad(toit, e, f, d, c, nAr, {e.x, 0}, {f.x, 0}, {f.x, length(c - e)}, {e.x, length(c - e)});
    // pignons (murs triangulaires) aux deux bouts
    for (int s = 0; s < 2; s++) {
        const float x = s ? L : 0;
        const vec3 n{s ? 1.f : -1.f, 0, 0}, p0{x, H, 0}, p1{x, H, -D}, p2{x, H + hf, zf};
        const uint32_t k = (uint32_t)mur.v.size();
        mur.v.push_back({p0, n, {0, H}}); mur.v.push_back({p1, n, {D, H}}); mur.v.push_back({p2, n, {demi, H + hf}});
        // (p1 − p0) × (p2 − p0) pointe vers +x : ordre direct pour le pignon droit, inversé pour le gauche
        if (s) mur.i.insert(mur.i.end(), {k, k + 1, k + 2}); else mur.i.insert(mur.i.end(), {k, k + 2, k + 1});
    }
    if (deb > 0) {   // sous-face du débord (avant et arrière), vue d'en bas depuis la rue
        rectY(toit, -deb, L + deb, deb, 0, ye, -1);
        rectY(toit, -deb, L + deb, -D, -D - deb, ye, -1);
    }
}

} // namespace

void construire(const Plan& p, float D, int lod, Batiment& out) {
    for (Maillage& m : out.part) { m.v.clear(); m.i.clear(); }
    D = std::max(D, 2.f);
    const float H = p.hauteur(), L = p.largeur;
    facade(p, std::clamp(lod, 0, 2), out);
    Maillage& mur = out.part[MUR];
    rectX(mur, 0, 0, -D, -SOCLE, H, -1);   // côtés et arrière (murs aveugles : mitoyens ou cours)
    rectX(mur, L, 0, -D, -SOCLE, H, 1);
    rectZ(mur, 0, L, -SOCLE, H, -D, -1);
    if (p.corniche > 0) {   // corniche : bandeau en saillie sur la façade et en retour sur les côtés
        const float s = regles(p.style).saillieCorniche;
        pave(out.part[ENCADREMENT], -s, L + s, H - p.corniche, H, -D, s);
    }
    toiture(p, D, std::clamp(lod, 0, 2), out);
}

double aireFacade(const Maillage& mur, float yMax) {
    double a = 0;
    for (size_t t = 0; t + 2 < mur.i.size(); t += 3) {
        const Sommet &A = mur.v[mur.i[t]], &B = mur.v[mur.i[t + 1]], &C = mur.v[mur.i[t + 2]];
        if (A.n.z < 0.99f || std::fabs(A.p.z) > 1e-4f || std::fabs(B.p.z) > 1e-4f || std::fabs(C.p.z) > 1e-4f) continue;
        if (A.p.y < -1e-4f || B.p.y < -1e-4f || C.p.y < -1e-4f) continue;   // socle exclu
        if (A.p.y > yMax + 1e-4f || B.p.y > yMax + 1e-4f || C.p.y > yMax + 1e-4f) continue;   // acrotère exclu
        a += 0.5 * length(cross(B.p - A.p, C.p - A.p));
    }
    return a;
}

} // namespace wfs::archi
