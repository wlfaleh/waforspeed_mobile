// WaForSpeed — WFS-550 (interface R19 3/5) : jauges communes ; voir jauges.h.
#include "jauges.h"
#include <cmath>
#include "../game/commandes_tactiles.h"   // dessin2d : anneau et arc à bords lissés, biseau
#include "../render/renderer.h"
#include "verre.h"                         // dégradé de la marque cyan → orange (WFS-548)

namespace wfs::jauges {
namespace {

constexpr int MORCEAUX = 32;                    // morceaux d'arc d'un dégradé sur 360° (≈ 11° chacun : aucune marche visible)
const vec4 PISTE{1, 1, 1, 0.13f};               // piste grise (blanc ≈ 13 % sur le verre, maquettes 13, 26 et 28)
const vec4 BLANC{242 / 255.f, 244 / 255.f, 248 / 255.f, 1};   // --text
constexpr unsigned ORANGE = 0xFF6A00;           // --accent
constexpr unsigned GRIS_BLOQUEE = 0x5A5F6E;     // segment d'une voiture bloquée (maquette 15)

vec4 alpha(vec4 c, float a) { c.w *= a; return c; }

// arc rempli du dégradé de la marque, de a0 à a1 (angles de dessin2d) ; couleur t0 au début, t1 à la fin ; bouts ronds
void arcDegrade(Renderer& r, float cx, float cy, float rayon, float ep, float a0, float a1, float t0, float t1, float a) {
    if (a1 <= a0 + 1e-4f) return;
    const int n = std::max(1, (int)std::ceil(MORCEAUX * (a1 - a0) / (2 * PI_J)));
    const float hw = ep / 2;
    for (int i = 0; i < n; i++) {   // morceaux jointifs : le liseré de lissage n'est que radial, aucune couture visible
        const float u0 = (float)i / n, u1 = (float)(i + 1) / n;
        const vec4 c = verre::degradeMarque(t0 + (t1 - t0) * (u0 + u1) / 2, a);
        dessin2d::anneau(r, cx, cy, rayon - hw, rayon + hw, c, a0 + (a1 - a0) * u0, a0 + (a1 - a0) * u1);
    }
    // bouts ronds (stroke-linecap: round), couleur du bout
    const float sa0 = std::sin(a0), ca0 = std::cos(a0), sa1 = std::sin(a1), ca1 = std::cos(a1);
    r.uiCircle(cx + rayon * sa0, cy - rayon * ca0, hw + 0.5f, verre::degradeMarque(t0, a));
    r.uiCircle(cx + rayon * sa1, cy - rayon * ca1, hw + 0.5f, verre::degradeMarque(t1, a));
}

} // namespace

void cadran(Renderer& r, float cx, float cy, float rayon, float ep, float f, float a) {
    f = std::clamp(f, 0.f, 1.f);
    dessin2d::arc(r, cx, cy, rayon, ep, CADRAN_DEBUT, CADRAN_FIN, alpha(PISTE, a));
    // maquette 13 : l'arc part en cyan et finit en orange à son bout, quelle que soit sa longueur
    if (f > 0) arcDegrade(r, cx, cy, rayon, ep, CADRAN_DEBUT, angleCadran(f), 0, 1, a);
}

void anneau(Renderer& r, float cx, float cy, float rayon, float ep, float f, float a) {
    f = std::clamp(f, 0.f, 1.f);
    const float hw = ep / 2;
    dessin2d::anneau(r, cx, cy, rayon - hw, rayon + hw, alpha(PISTE, a));
    if (f > 0) arcDegrade(r, cx, cy, rayon, ep, 0, 2 * PI_J * f, 0, 1, a);
}

void barreFine(Renderer& r, float x, float y, float w, float h, float f, float a, int graduations) {
    if (w <= 0 || h <= 0) return;
    f = std::clamp(f, 0.f, 1.f);
    r.uiRect(x, y, w, h, alpha(PISTE, a));
    if (f > 0) {   // dégradé pris sur toute la largeur : un seul quadrilatère à couleurs par sommet, de cyan à la couleur de f
        const vec4 c0 = verre::degradeMarque(0, a), c1 = verre::degradeMarque(f, a);
        const float xf = x + w * f;
        const UiVertex q[4] = {{x, y, -1, -1, c0.x, c0.y, c0.z, c0.w}, {xf, y, -1, -1, c1.x, c1.y, c1.z, c1.w},
                               {xf, y + h, -1, -1, c1.x, c1.y, c1.z, c1.w}, {x, y + h, -1, -1, c0.x, c0.y, c0.z, c0.w}};
        r.uiQuad(q);
    }
    if (graduations > 1) {   // traits fins de graduation (maquette 13 : 10 cases), un pixel, au-dessus et au-dessous de la barre
        const float t = std::max(1.f, std::round(h * 0.25f)), dh = std::max(2.f, h * 0.6f);
        for (int i = 1; i < graduations; i++) {
            const float gx = std::round(x + w * i / graduations);
            r.uiRect(gx, y - dh, t, h + 2 * dh, vec4{1, 1, 1, 0.16f * a});
        }
    }
}

void barreEtape(Renderer& r, float x, float y, float w, float h, float avancement, bool enCours, float a) {
    if (w <= 0 || h <= 0) return;
    avancement = std::clamp(avancement, 0.f, 1.f);
    r.uiRect(x, y, w, h, alpha(PISTE, a));
    if (avancement >= 1) r.uiRect(x, y, w, h, alpha(BLANC, a));                       // finie : blanche (maquette 08)
    else if (enCours && avancement > 0) r.uiRect(x, y, w * avancement, h, verre::hexa(ORANGE, a));   // en cours : orange
}

float segments(Renderer& r, float cx, float y, float h, const Segment* etats, int n, float a, Rect* zones) {
    if (n <= 0 || h <= 0) return 0;
    const float l = h * 3.2f, e = h * 0.9f, biais = h * 0.176f;   // tan(10°) ≈ 0,176 : skewX(-10deg)
    const float total = largeurSegments(etats, n, l, e), x0 = cx - total / 2;
    for (int i = 0; i < n; i++) {
        const PlaceSegment p = placeSegment(etats, n, i, l, e);
        const vec4 c = etats[i] == Segment::Affichee ? verre::hexa(ORANGE, a)
                     : etats[i] == Segment::Possedee ? alpha(BLANC, 0.92f * a) : verre::hexa(GRIS_BLOQUEE, 0.75f * a);
        if (etats[i] == Segment::Affichee)   // lueur néon du segment affiché (un seul accent dominant)
            dessin2d::biseau(r, x0 + p.x - h * 0.6f, y - h * 0.6f, p.l + h * 1.2f, h * 2.2f, biais * 2.2f, dessin2d::uni(verre::hexa(ORANGE, 0.22f * a)));
        dessin2d::biseau(r, x0 + p.x, y, p.l, h, biais, dessin2d::uni(c));
        if (zones) zones[i] = {x0 + p.x, y, p.l, h};
    }
    return total;
}

} // namespace wfs::jauges
