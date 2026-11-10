// WaForSpeed — port de la Ville modélisé (voir decor_ville_port.h).
//
// Mesures (sources) :
// - conteneur 40 pieds, ISO 668 type 1AA : 12,192 × 2,438 × 2,591 m ; pièces de coin ISO 1161 : 178 × 162 × 118 mm ;
//   parois latérales en tôle ondulée trapézoïdale (pas ≈ 0,28 m, profondeur ≈ 36 mm : valeurs courantes des fabricants) ;
//   4 barres de verrouillage par paire de portes (Ø ≈ 34 mm) ;
// - portique de quai : écartement des rails 30,48 m (100 pieds, valeur la plus répandue), portée avant 45 m et arrière 15 m,
//   passage entre pieds ≈ 17 m le long du quai, chevalet ≈ 22 m au-dessus des poutres (ordres de grandeur des fiches de
//   portiques « post-Panamax » publiées par les constructeurs, conception à confirmer au référentiel) ; galets Ø 0,8 m ;
// - clôture à double fil 6/5/6 : panneau de 2,50 m (ici recoupé à la longueur posée), hauteur 2,03 m, maille 200 × 50 mm,
//   fils horizontaux doubles Ø 6 mm, fils verticaux Ø 5 mm ; poteau tube 60 × 40 mm, 2,20 m hors sol, chapeau.
// Les valeurs de conception (pas de mesure publiée accessible, réseau limité) sont dites comme telles.
#include "decor_ville_port.h"
#include <algorithm>
#include <cmath>

namespace wfs::port {
namespace {

// face plane (repère local) : a b c d, normale vers « dehors » (local), ordre corrigé si besoin
void face(Sortie& s, const Repere& R, vec3 a, vec3 b, vec3 c, vec3 d, vec3 dehors, vec2 uv) {
    const vec3 A = R.p(a), B = R.p(b), C = R.p(c), D = R.p(d);
    vec3 n = cross(B - A, C - A);
    if (dot(n, n) < 1e-14f) return;
    n = normalize(n);
    const bool inverse = dot(n, R.d(dehors)) < 0;
    if (inverse) n = n * -1.f;
    const uint32_t k = (uint32_t)s.v.size();
    for (vec3 p : {A, B, C, D}) { Vertex v{}; v.px = p.x; v.py = p.y; v.pz = p.z; v.nx = n.x; v.ny = n.y; v.nz = n.z; v.u = uv.x; v.v = uv.y; s.v.push_back(v); }
    if (inverse) s.i.insert(s.i.end(), {k, k + 2, k + 1, k, k + 3, k + 2});
    else s.i.insert(s.i.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
}
// pavé sans chanfrein (pièces de quelques centimètres : le chanfrein ne se verrait pas)
void bloc(Sortie& s, const Repere& R, vec3 c, vec3 h, vec2 uv) { mobilier::pave(s, R, c, h, 0.f, uv); }

// tôle ondulée trapézoïdale dans le plan u (le long) × y, normale ±n : de u0 à u1, de y0 à y1 ; axe u, axe n (locaux)
void tole(Sortie& s, const Repere& R, vec3 o, vec3 u, vec3 n, float u0, float u1, float y0, float y1, float pas, float creux, vec2 uv) {
    // nervure, pente, creux, pente : 4 quarts du pas
    const vec3 up{0, 1, 0};
    int nb = std::max(1, (int)std::round((u1 - u0) / pas));
    const float p = (u1 - u0) / nb;
    for (int k = 0; k < nb; k++) {
        const float a = u0 + k * p;
        const float x[5] = {a, a + p * 0.25f, a + p * 0.5f, a + p * 0.75f, a + p};
        const float z[5] = {0, 0, -creux, -creux, 0};
        for (int j = 0; j < 4; j++) {
            const vec3 A = o + u * x[j] + n * z[j], B = o + u * x[j + 1] + n * z[j + 1];
            face(s, R, A + up * y0, B + up * y0, B + up * y1, A + up * y1, n, uv);
        }
    }
}

// galet de roulement à boudin (Ø 0,8 m), axe x local, centré en c
void galet(Sortie& s, const Repere& R, vec3 c, vec2 uv) {
    const Repere G{R.p(c), R.y, R.x, R.z * -1.f};   // l'axe de rotation (y de « tour ») suit x
    mobilier::tour(s, G, {0, 0, 0}, {{0.40f, -0.08f}, {0.40f, 0.05f}, {0.45f, 0.06f}, {0.45f, 0.08f}, {0.12f, 0.08f}, {0.12f, 0.12f}, {0.f, 0.12f}}, 14, uv);
    mobilier::tour(s, G, {0, 0, 0}, {{0.f, -0.12f}, {0.12f, -0.12f}, {0.12f, -0.08f}, {0.40f, -0.08f}}, 14, uv);
}

}  // namespace

void conteneur(Sortie s, const Repere& R, vec2 peint, vec2 noir, bool loin) {
    constexpr float hw = CONT_W / 2, hl = CONT_L / 2, H = CONT_H;
    constexpr float PC_X = 0.089f, PC_Y = 0.059f, PC_Z = 0.081f;   // demi-mesures d'une pièce de coin (ISO 1161)
    // poteaux d'angle et pièces de coin
    for (float sx : {-1.f, 1.f}) for (float sz : {-1.f, 1.f}) {
        bloc(s, R, {sx * (hw - 0.075f), H / 2, sz * (hl - 0.075f)}, {0.075f, H / 2 - 2 * PC_Y, 0.075f}, peint);
        for (float y : {PC_Y, H - PC_Y}) bloc(s, R, {sx * (hw - PC_X), y, sz * (hl - PC_Z)}, {PC_X, PC_Y, PC_Z}, peint);
    }
    // longerons bas (0,16 m) et hauts (0,10 m), traverses d'about
    for (float sx : {-1.f, 1.f}) {
        bloc(s, R, {sx * (hw - 0.06f), 0.08f + 0.04f, 0}, {0.06f, 0.08f, hl - 2 * PC_Z}, peint);
        bloc(s, R, {sx * (hw - 0.05f), H - 0.06f, 0}, {0.05f, 0.05f, hl - 2 * PC_Z}, peint);
    }
    for (float sz : {-1.f, 1.f}) {
        bloc(s, R, {0, 0.12f, sz * (hl - 0.07f)}, {hw - 2 * PC_X, 0.08f, 0.07f}, peint);
        bloc(s, R, {0, H - 0.08f, sz * (hl - 0.07f)}, {hw - 2 * PC_X, 0.08f, 0.07f}, peint);
    }
    const float y0 = 0.2f, y1 = H - 0.11f;
    // parois latérales ondulées (nervures verticales), toit, about arrière ondulé
    for (float sx : {-1.f, 1.f}) {
        const vec3 o{sx * (hw - 0.02f), 0, 0}, n{sx, 0, 0};
        if (loin) face(s, R, o + vec3{0, y0, -hl + 0.15f}, o + vec3{0, y0, hl - 0.15f}, o + vec3{0, y1, hl - 0.15f}, o + vec3{0, y1, -hl + 0.15f}, n, peint);
        else tole(s, R, o, {0, 0, 1}, n, -hl + 0.15f, hl - 0.15f, y0, y1, 0.278f, 0.036f, peint);
    }
    face(s, R, {-hw + 0.05f, H - 0.02f, -hl + 0.1f}, {hw - 0.05f, H - 0.02f, -hl + 0.1f}, {hw - 0.05f, H - 0.02f, hl - 0.1f}, {-hw + 0.05f, H - 0.02f, hl - 0.1f}, {0, 1, 0}, peint);
    {
        const vec3 o{0, 0, -hl + 0.02f}, n{0, 0, -1};
        if (loin) face(s, R, o + vec3{-hw + 0.15f, y0, 0}, o + vec3{hw - 0.15f, y0, 0}, o + vec3{hw - 0.15f, y1, 0}, o + vec3{-hw + 0.15f, y1, 0}, n, peint);
        else tole(s, R, o, {1, 0, 0}, n, -hw + 0.15f, hw - 0.15f, y0, y1, 0.278f, 0.036f, peint);
    }
    // portes (+z) : deux vantaux à 4 nervures horizontales, joint central, 4 barres de verrouillage, charnières
    const float zp = hl - 0.03f;
    for (float sx : {-1.f, 1.f}) {
        const float xa = sx < 0 ? -hw + 0.15f : 0.006f, xb = sx < 0 ? -0.006f : hw - 0.15f;
        face(s, R, {xa, y0, zp}, {xb, y0, zp}, {xb, y1, zp}, {xa, y1, zp}, {0, 0, 1}, peint);
        if (!loin) for (int k = 0; k < 4; k++) {
            const float y = y0 + (y1 - y0) * (k + 0.5f) / 4;
            bloc(s, R, {(xa + xb) / 2, y, zp + 0.012f}, {(xb - xa) / 2 - 0.03f, 0.04f, 0.012f}, peint);
        }
    }
    face(s, R, {-0.006f, y0, zp - 0.01f}, {0.006f, y0, zp - 0.01f}, {0.006f, y1, zp - 0.01f}, {-0.006f, y1, zp - 0.01f}, {0, 0, 1}, noir);   // joint
    for (float x : {-0.86f, -0.28f, 0.28f, 0.86f}) {
        mobilier::tube(s, R, {{x, 0.16f, zp + 0.06f}, {x, H - 0.16f, zp + 0.06f}}, 0.017f, loin ? 4 : 6, peint);
        if (loin) continue;
        for (float y : {0.18f, H - 0.18f}) bloc(s, R, {x, y, zp + 0.04f}, {0.045f, 0.03f, 0.04f}, peint);           // gâches de came
        for (float y : {0.8f, 1.6f}) bloc(s, R, {x, y, zp + 0.035f}, {0.035f, 0.02f, 0.035f}, peint);              // paliers
        bloc(s, R, {x + (x > 0 ? -0.12f : 0.12f), 1.25f, zp + 0.09f}, {0.12f, 0.025f, 0.015f}, peint);           // poignée
    }
    if (!loin) for (float sx : {-1.f, 1.f}) for (int k = 0; k < 4; k++)                                         // charnières
        bloc(s, R, {sx * (hw - 0.17f), y0 + 0.25f + k * 0.65f, zp + 0.02f}, {0.04f, 0.05f, 0.02f}, peint);
}

void grue(Sortie s, Sortie verre, const Repere& R, float h, vec2 acier, vec2 blanc, vec2 noir, bool loin) {
    constexpr float G = GRUE_ECARTEMENT, ZP = 8.75f;   // pieds à ± 8,75 m le long du quai (passage ≈ 17 m entre leurs faces)
    const float yg = h + 4.f, yp = yg - 3.f, ya = yg + 22.f;   // dessous des poutres, haut du portique, sommet du chevalet
    // galets, bogies, longrines basses (le long du quai)
    for (float x : {0.f, G}) {
        mobilier::barre(s, R, {x, 2.3f, -ZP - 2.2f}, {x, 2.3f, ZP + 2.2f}, {1, 0, 0}, 1.0f, 1.4f, 0.06f, acier);
        for (float zb : {-ZP, ZP}) for (float d : {-1.4f, 1.4f}) {
            mobilier::pave(s, R, {x, 1.15f, zb + d}, {0.55f, 0.35f, 1.1f}, 0.05f, acier);
            if (!loin) for (float e : {-0.55f, 0.55f}) galet(s, R, {x, 0.4f, zb + d + e}, noir);
        }
    }
    // pieds : caissons de 1,6 m en bas, 1,2 m en haut, du dessus des longrines au portique
    for (float x : {0.f, G}) for (float z : {-ZP, ZP}) {
        mobilier::barre(s, R, {x, 2.8f, z}, {x, yp, z}, {1, 0, 0}, 1.4f, 1.4f, 0.08f, acier);
        mobilier::pave(s, R, {x, yp - 0.6f, z}, {0.9f, 0.6f, 0.9f}, 0.08f, acier);   // nœud du portique
    }
    // portique : traverses le long du quai et entre les rails, croix de contreventement côté terre
    for (float x : {0.f, G}) mobilier::barre(s, R, {x, yp, -ZP - 0.7f}, {x, yp, ZP + 0.7f}, {1, 0, 0}, 1.2f, 1.6f, 0.06f, acier);
    for (float z : {-ZP, ZP}) {
        mobilier::barre(s, R, {-0.7f, yp, z}, {G + 0.7f, yp, z}, {0, 0, 1}, 1.2f, 1.6f, 0.06f, acier);
        mobilier::barre(s, R, {0.6f, 12.f, z}, {G - 0.6f, 12.f, z}, {0, 0, 1}, 0.9f, 1.1f, 0.05f, acier);   // traverse basse
    }
    mobilier::barre(s, R, {0, 3.2f, -ZP}, {0, yp - 1.2f, ZP}, {1, 0, 0}, 0.5f, 0.5f, 0.03f, acier);
    mobilier::barre(s, R, {0, 3.2f, ZP}, {0, yp - 1.2f, -ZP}, {1, 0, 0}, 0.5f, 0.5f, 0.03f, acier);
    // poutres (deux caissons de 2,8 m) : de l'arrière-bec à la pointe de la flèche ; entretoises
    const float x0 = -GRUE_ARRIERE, x1 = G + GRUE_AVANT;
    for (float z : {-3.2f, 3.2f}) {
        mobilier::barre(s, R, {x0, yg + 1.4f, z}, {x1, yg + 1.4f, z}, {0, 0, 1}, 2.8f, 1.2f, 0.06f, acier);
        mobilier::barre(s, R, {x1 - 0.2f, yg + 0.6f, z}, {x1 + 1.2f, yg + 2.2f, z}, {0, 0, 1}, 1.0f, 1.2f, 0.05f, acier);   // tête de flèche
    }
    for (float x = x0 + 2.f; x < x1; x += 10.f) mobilier::barre(s, R, {x, yg + 2.6f, -3.2f}, {x, yg + 2.6f, 3.2f}, {1, 0, 0}, 0.5f, 0.4f, 0.03f, acier);
    for (float x : {0.f, G}) mobilier::barre(s, R, {x, yg - 0.6f, -ZP}, {x, yg - 0.6f, ZP}, {1, 0, 0}, 1.0f, 1.4f, 0.05f, acier);   // sommiers sur les pieds
    for (float x : {0.f, G}) for (float z : {-ZP, ZP})                                                                      // montants pieds → sommier
        mobilier::barre(s, R, {x, yp + 0.8f, z}, {x, yg - 1.1f, z}, {1, 0, 0}, 1.0f, 1.0f, 0.04f, acier);
    // chevalet au-dessus du rail côté eau : deux jambes par côté, chapeau, haubans vers la flèche et l'arrière-bec
    const float xa = G - 1.f;
    for (float z : {-3.2f, 3.2f}) {
        mobilier::barre(s, R, {G - 8.f, yg + 2.8f, z}, {xa, ya, z}, {0, 0, 1}, 0.9f, 0.9f, 0.04f, acier);
        mobilier::barre(s, R, {G + 4.f, yg + 2.8f, z}, {xa, ya, z}, {0, 0, 1}, 0.9f, 0.9f, 0.04f, acier);
        for (float xs : {G + 22.f, x1 - 1.f}) mobilier::tube(s, R, {{xa, ya, z}, {xs, yg + 2.8f, z}}, 0.09f, loin ? 4 : 8, acier, false);
        mobilier::tube(s, R, {{xa, ya, z}, {x0 + 1.f, yg + 2.8f, z}}, 0.11f, loin ? 4 : 8, acier, false);
    }
    mobilier::barre(s, R, {xa, ya + 0.4f, -3.8f}, {xa, ya + 0.4f, 3.8f}, {1, 0, 0}, 1.4f, 1.0f, 0.05f, acier);
    // salle des machines sur l'arrière-bec (12 × 9 × 3,4 m) : caisson à angles cassés, toit en débord, portes, grilles
    const vec3 cm{x0 + 6.5f, yg + 2.8f + 1.7f, 0};
    mobilier::pave(s, R, cm, {6.f, 1.7f, 4.5f}, 0.12f, blanc);
    mobilier::pave(s, R, cm + vec3{0, 1.8f, 0}, {6.3f, 0.12f, 4.8f}, 0.04f, blanc);
    if (!loin) {
        for (float z : {-4.52f, 4.52f}) {
            face(s, R, {cm.x - 4.f, cm.y - 1.7f, z}, {cm.x - 3.f, cm.y - 1.7f, z}, {cm.x - 3.f, cm.y + 0.4f, z}, {cm.x - 4.f, cm.y + 0.4f, z}, {0, 0, z}, noir);   // porte
            for (int k = 0; k < 5; k++) {                                                                                     // lames de ventilation
                const float y = cm.y - 0.3f + k * 0.22f;
                face(s, R, {cm.x + 1.f, y, z}, {cm.x + 4.5f, y, z}, {cm.x + 4.5f, y + 0.14f, z + (z > 0 ? 0.08f : -0.08f)}, {cm.x + 1.f, y + 0.14f, z + (z > 0 ? 0.08f : -0.08f)}, {0, 1, z}, noir);
            }
        }
        // garde-corps des passerelles le long des poutres (main courante et lisse, poteaux tous les 2 m)
        for (float z : {-4.1f, 4.1f}) {
            for (float y : {yg + 2.8f + 1.1f, yg + 2.8f + 0.55f}) mobilier::tube(s, R, {{x0 + 13.f, y, z}, {x1 - 1.f, y, z}}, 0.025f, 4, blanc, false);
            for (float x = x0 + 13.f; x <= x1 - 1.f; x += 2.f) mobilier::tube(s, R, {{x, yg + 2.8f, z}, {x, yg + 2.8f + 1.1f, z}}, 0.02f, 4, blanc, false);
        }
    }
    // chariot sur les poutres (côté eau) et cabine du grutier suspendue dessous, vitrée vers le bas et l'avant
    const float xc = G + 12.f;
    mobilier::pave(s, R, {xc, yg + 3.3f, 0}, {3.2f, 0.5f, 3.9f}, 0.08f, acier);
    mobilier::pave(s, R, {xc, yg - 0.2f, 0}, {2.6f, 0.4f, 2.6f}, 0.08f, acier);
    mobilier::pave(s, R, {xc - 1.6f, yg - 2.2f, -2.2f}, {1.4f, 1.5f, 1.25f}, 0.1f, blanc);                         // cabine
    face(verre, R, {xc - 0.18f, yg - 3.5f, -3.3f}, {xc - 0.18f, yg - 3.5f, -1.1f}, {xc - 0.18f, yg - 0.9f, -1.1f}, {xc - 0.18f, yg - 0.9f, -3.3f}, {1, 0, 0}, {0, 0});
    face(verre, R, {xc - 2.8f, yg - 3.72f, -3.3f}, {xc - 0.4f, yg - 3.72f, -3.3f}, {xc - 0.4f, yg - 3.72f, -1.1f}, {xc - 2.8f, yg - 3.72f, -1.1f}, {0, -1, 0}, {0, 0});
    // câbles de levage (4 brins) et palonnier de 40 pieds (cadre, verrous tournants) à 14 m au-dessus du quai
    const float ys = std::min(14.f, yg - 6.f);
    for (float dx : {-1.0f, 1.0f}) for (float dz : {-1.4f, 1.4f})
        mobilier::tube(s, R, {{xc + dx, yg - 0.6f, dz}, {xc + dx * 0.8f, ys + 0.5f, dz * 2.f}}, 0.03f, 4, noir, false);
    mobilier::pave(s, R, {xc, ys + 0.25f, 0}, {CONT_W / 2, 0.25f, CONT_L / 2}, 0.05f, acier);
    mobilier::pave(s, R, {xc, ys + 0.7f, 0}, {0.8f, 0.25f, 1.6f}, 0.05f, acier);
}

void cloture(Sortie s, vec3 a, vec3 b, vec3 ext, vec2 peint, bool loin) {
    constexpr float HP = 2.03f, HPOT = 2.20f;
    vec3 u = b - a; u.y = 0;
    const float L = length(u);
    if (L < 0.2f) return;
    u = u * (1.f / L);
    const Repere R = mobilier::repere(a, normalize(vec3{ext.x, 0, ext.z}), u);   // x vers la rue, z le long, origine au poteau
    // poteau tube 60 × 40 (40 le long de la clôture), chapeau
    mobilier::pave(s, R, {0, HPOT / 2, 0}, {0.03f, HPOT / 2, 0.02f}, 0.004f, peint);
    mobilier::pave(s, R, {0, HPOT + 0.01f, 0}, {0.034f, 0.012f, 0.024f}, 0.004f, peint);
    // panneau : du poteau (+3 cm) au poteau suivant, fond légèrement au-dessus du sol (5 cm), deux plis en V (h 0,2 m, ±6 cm)
    const float z0 = 0.03f, z1 = L - 0.03f, yb = 0.05f;
    const float plis[2] = {0.35f, 1.45f};
    auto creux = [&](float y) {   // décalage du plan du panneau vers la rue dans les plis
        float d = 0;
        for (float p : plis) d = std::max(d, 0.06f * std::max(0.f, 1.f - std::fabs(y - p) / 0.1f));
        return d;
    };
    if (loin) {   // loin : plan du panneau plein (les fils de 5 mm ne se résolvent plus) — remplacé par les fils doubles horizontaux seuls
        for (float y = yb; y <= yb + HP + 1e-3f; y += 0.2f) mobilier::tube(s, R, {{0.f, y, z0}, {0.f, y, z1}}, 0.006f, 3, peint, false);
        return;
    }
    // fils verticaux Ø 5 tous les 50 mm (rubans orientés vers la rue, matériau double face), pliés dans les V
    const int nv = std::max(1, (int)std::round((z1 - z0) / 0.05f));
    const float ys[8] = {yb, plis[0] - 0.1f, plis[0], plis[0] + 0.1f, plis[1] - 0.1f, plis[1], plis[1] + 0.1f, yb + HP};
    for (int k = 0; k <= nv; k++) {
        const float z = z0 + (z1 - z0) * k / nv;
        for (int j = 0; j < 7; j++) {
            const float ya = ys[j], yc = ys[j + 1], da = creux(ya), dc = creux(yc);
            face(s, R, {da, ya, z - 0.0025f}, {da, ya, z + 0.0025f}, {dc, yc, z + 0.0025f}, {dc, yc, z - 0.0025f}, {1, 0, 0}, peint);
        }
    }
    // fils horizontaux doubles Ø 6 tous les 200 mm, de part et d'autre des fils verticaux
    for (float y = yb + 0.03f; y <= yb + HP; y += 0.2f) {
        const float d = creux(y);
        for (float e : {-0.006f, 0.006f})
            face(s, R, {d + e, y - 0.003f, z0}, {d + e, y - 0.003f, z1}, {d + e, y + 0.003f, z1}, {d + e, y + 0.003f, z0}, {e < 0 ? -1.f : 1.f, 0, 0}, peint);
    }
}

}  // namespace wfs::port
