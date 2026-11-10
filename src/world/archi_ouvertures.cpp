// WaForSpeed — WFS-402 (épic E15) : fenêtres et portes au niveau du vrai objet (voir archi_ouvertures.h).
//
// Mesures (valeurs courantes des menuiseries françaises, conception ; à confirmer au référentiel references/reel/, M-02) :
//   - dormant de fenêtre bois 58 × 68 mm : vu de face 60 mm, épaisseur 70 mm ; ouvrant 55 mm vu de face, 55 mm d'épaisseur ;
//     petits bois 26 mm ; jet d'eau (rejet d'eau) 30 mm ; jeu entre ouvrants 3 mm ;
//   - huisserie de porte d'immeuble 70 mm ; bloc-porte de maison 0,90 × 2,15 m, huisserie 35 mm (passage utile 0,83 m :
//     arrêté du 24 décembre 2015, référentiel archi.porte_entree.passage_utile) ; vantail 50 mm d'épaisseur ;
//     béquille à 1,00 m du sol fini, plaque de 45 × 240 mm ;
//   - porte de garage sectionnelle : sections de 0,50 m ; lame de volet roulant aluminium 40 mm, coulisses 30 mm ;
//   - persienne : cadre 60 mm, lames de 65 mm inclinées à 45° au pas de 50 mm, épaisseur 35 mm ; volet plein : planches de
//     27 mm à barres et écharpe ;
//   - appui de pierre : saillie 50 mm (RG-02 : 3 à 5 cm), glacis 12 mm, nez 50 mm, larmier 6 mm ; linteau de pierre 20 cm.
#include "archi_ouvertures.h"
#include <algorithm>
#include <cmath>

namespace wfs::archi::ouvertures {
namespace {

// ---------------------------------------------------------------- géométrie élémentaire (mêmes conventions que archi_details.cpp)
vec2 uvPlan(vec3 p, vec3 n) {   // uv en mètres, projetés sur le plan le plus proche
    if (std::fabs(n.y) > 0.7f) return {p.x, p.z};
    if (std::fabs(n.x) > std::fabs(n.z)) return {n.x > 0 ? -p.z : p.z, p.y};
    return {n.z > 0 ? p.x : -p.x, p.y};
}
// quadrilatère plan a b c d ; la normale est prise du côté de « dehors » (l'ordre est corrigé si besoin)
void face(Maillage& m, vec3 a, vec3 b, vec3 c, vec3 d, vec3 dehors) {
    vec3 n = cross(b - a, c - a);
    if (length(n) < 1e-10f) n = cross(c - a, d - a);
    if (length(n) < 1e-10f) return;
    n = normalize(n);
    if (dot(n, dehors) < 0) { std::swap(b, d); n = n * -1.f; }
    const uint32_t k = (uint32_t)m.v.size();
    for (vec3 p : {a, b, c, d}) m.v.push_back({p, n, uvPlan(p, n)});
    m.i.insert(m.i.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
}
// pavé aligné : bits de faces (1 avant +z, 2 arrière −z, 4 gauche −x, 8 droite +x, 16 dessus, 32 dessous)
enum : int { AV = 1, AR = 2, GA = 4, DR = 8, HA = 16, BA = 32, TOUT = 63, VISIBLE = AV | GA | DR | HA | BA };
void bloc(Maillage& m, float x0, float x1, float y0, float y1, float z0, float z1, int faces = VISIBLE) {
    if (x1 - x0 < 1e-4f || y1 - y0 < 1e-4f || std::fabs(z1 - z0) < 1e-4f) return;
    if (z0 > z1) std::swap(z0, z1);
    if (faces & AV) face(m, {x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}, {0, 0, 1});
    if (faces & AR) face(m, {x1, y0, z0}, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0}, {0, 0, -1});
    if (faces & GA) face(m, {x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0}, {-1, 0, 0});
    if (faces & DR) face(m, {x1, y0, z1}, {x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1}, {1, 0, 0});
    if (faces & HA) face(m, {x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0}, {x0, y1, z0}, {0, 1, 0});
    if (faces & BA) face(m, {x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1}, {0, -1, 0});
}

struct Rect {
    float x0, y0, x1, y1;
    Rect retreci(float e) const { return {x0 + e, y0 + e, x1 - e, y1 - e}; }
    float l() const { return x1 - x0; }
    float h() const { return y1 - y0; }
    bool valide() const { return x1 - x0 > 1e-3f && y1 - y0 > 1e-3f; }
};

// anneau qui relie le rectangle A (à la profondeur zA) au rectangle B (à zB), B intérieur à A ou égal : 4 quadrilatères à
// coupe d'onglet. Côté visible : vers l'avant (+z) et, quand l'anneau s'enfonce (zB < zA), vers le centre ; quand il monte
// (zB > zA), vers l'extérieur. bas = false : pas de côté bas (huisserie de porte, sans traverse basse).
void anneau(Maillage& m, Rect A, float zA, Rect B, float zB, bool bas = true) {
    const vec3 a[4] = {{A.x0, A.y0, zA}, {A.x1, A.y0, zA}, {A.x1, A.y1, zA}, {A.x0, A.y1, zA}};
    const vec3 b[4] = {{B.x0, B.y0, zB}, {B.x1, B.y0, zB}, {B.x1, B.y1, zB}, {B.x0, B.y1, zB}};
    const vec3 centre[4] = {{0, 1, 0}, {-1, 0, 0}, {0, -1, 0}, {1, 0, 0}};   // bas, droite, haut, gauche : direction du centre
    const float s = zA > zB + 1e-6f ? 1.f : (zA < zB - 1e-6f ? -1.f : 0.f);
    for (int k = 0; k < 4; k++) {
        if (k == 0 && !bas) continue;
        const int j = (k + 1) % 4;
        face(m, a[k], a[j], b[j], b[k], vec3{0, 0, 1} + centre[k] * s);
    }
}
// côtés extérieurs d'un rectangle, de z0 à z1 (vus de l'extérieur : jeu entre un ouvrant et son dormant, tranche d'un vantail)
void cotes(Maillage& m, Rect R, float z0, float z1, bool bas = true) {
    if (bas) face(m, {R.x0, R.y0, z0}, {R.x1, R.y0, z0}, {R.x1, R.y0, z1}, {R.x0, R.y0, z1}, {0, -1, 0});
    face(m, {R.x1, R.y0, z0}, {R.x1, R.y1, z0}, {R.x1, R.y1, z1}, {R.x1, R.y0, z1}, {1, 0, 0});
    face(m, {R.x1, R.y1, z0}, {R.x0, R.y1, z0}, {R.x0, R.y1, z1}, {R.x1, R.y1, z1}, {0, 1, 0});
    face(m, {R.x0, R.y1, z0}, {R.x0, R.y0, z0}, {R.x0, R.y0, z1}, {R.x0, R.y1, z1}, {-1, 0, 0});
}
// cadre profilé : extérieur R, largeur vue de face e, de zArr (dos) à zAv (face), chanfrein c sur l'arête intérieure ; côtés
// extérieurs seulement si ext (ouvrant détaché de son dormant)
void cadre(Maillage& m, Rect R, float e, float zArr, float zAv, float c, bool ext = false, bool bas = true) {
    Rect I = R.retreci(e);
    if (!bas) I.y0 = R.y0;
    Rect Ic = I.retreci(-c);
    if (!bas) Ic.y0 = R.y0;
    anneau(m, R, zAv, Ic, zAv, bas);         // face vue
    anneau(m, Ic, zAv, I, zAv - c, bas);     // chanfrein
    anneau(m, I, zAv - c, I, zArr, bas);     // tranche intérieure (vers la vitre)
    if (ext) cotes(m, R, zArr, zAv, bas);
}
// face plate d'un vantail percée de « trous » (panneaux, vitrages) : découpe en grille, seules les cases hors trous sont posées
void facePercee(Maillage& m, Rect R, float z, const std::vector<Rect>& trous) {
    std::vector<float> xs{R.x0, R.x1}, ys{R.y0, R.y1};
    for (const Rect& t : trous) { xs.push_back(t.x0); xs.push_back(t.x1); ys.push_back(t.y0); ys.push_back(t.y1); }
    std::sort(xs.begin(), xs.end()); xs.erase(std::unique(xs.begin(), xs.end(), [](float a, float b) { return std::fabs(a - b) < 1e-5f; }), xs.end());
    std::sort(ys.begin(), ys.end()); ys.erase(std::unique(ys.begin(), ys.end(), [](float a, float b) { return std::fabs(a - b) < 1e-5f; }), ys.end());
    for (size_t i = 0; i + 1 < xs.size(); i++) for (size_t j = 0; j + 1 < ys.size(); j++) {
        const float cx = (xs[i] + xs[i + 1]) / 2, cy = (ys[j] + ys[j + 1]) / 2;
        bool dedans = false;
        for (const Rect& t : trous) dedans |= cx > t.x0 && cx < t.x1 && cy > t.y0 && cy < t.y1;
        if (!dedans) face(m, {xs[i], ys[j], z}, {xs[i + 1], ys[j], z}, {xs[i + 1], ys[j + 1], z}, {xs[i], ys[j + 1], z}, {0, 0, 1});
    }
}
// panneau à plate-bande (vantail de porte, soubassement) : moulure creuse puis champ relevé, de la face z du bâti
void panneau(Maillage& m, Rect P, float z) {
    const Rect a = P.retreci(0.012f), b = P.retreci(0.06f);
    if (!b.valide()) { anneau(m, P, z, P, z - 0.015f); face(m, {P.x0, P.y0, z - 0.015f}, {P.x1, P.y0, z - 0.015f}, {P.x1, P.y1, z - 0.015f}, {P.x0, P.y1, z - 0.015f}, {0, 0, 1}); return; }
    anneau(m, P, z, P, z - 0.02f);            // tranche du bâti (profondeur de la moulure)
    anneau(m, P, z - 0.02f, a, z - 0.02f);    // fond de la moulure
    anneau(m, a, z - 0.02f, b, z - 0.008f);   // plate-bande qui remonte vers le champ
    face(m, {b.x0, b.y0, z - 0.008f}, {b.x1, b.y0, z - 0.008f}, {b.x1, b.y1, z - 0.008f}, {b.x0, b.y1, z - 0.008f}, {0, 0, 1});
}
// trou dans un vantail jusqu'à la vitre (z0) : tranche intérieure, avec petit chanfrein de parclose
void jour(Maillage& m, Rect G, float zFace, float z0) {
    const Rect Gc = G.retreci(-0.006f);
    anneau(m, Gc, zFace, G, zFace - 0.006f);
    anneau(m, G, zFace - 0.006f, G, z0);
}

// rotation d'un maillage local (x le long du vantail depuis ses gonds, y vers le haut, z = face) autour d'un gond vertical
// placé en (gx, gz) de la façade, d'un angle a (0 : fermé dans le plan de la façade, π : rabattu contre le mur) ; miroir
// pour un vantail de droite (ses gonds à droite) : l'ordre des triangles est inversé pour garder la normale du bon côté
void poserVantail(const Maillage& loc, float gx, float gz, float a, bool droite, Maillage& out) {
    const float c = std::cos(a), s = std::sin(a);
    const vec3 U = droite ? vec3{-c, 0, s} : vec3{c, 0, s};
    const vec3 W = droite ? vec3{s, 0, c} : vec3{-s, 0, c};
    const uint32_t base = (uint32_t)out.v.size();
    for (const Sommet& q : loc.v) {
        const vec3 p = vec3{gx, 0, gz} + U * q.p.x + vec3{0, q.p.y, 0} + W * q.p.z;
        const vec3 n = normalize(U * q.n.x + vec3{0, q.n.y, 0} + W * q.n.z);
        out.v.push_back({p, n, uvPlan(p, n)});
    }
    for (size_t t = 0; t + 2 < loc.i.size(); t += 3) {
        if (droite) out.i.insert(out.i.end(), {base + loc.i[t], base + loc.i[t + 2], base + loc.i[t + 1]});
        else out.i.insert(out.i.end(), {base + loc.i[t], base + loc.i[t + 1], base + loc.i[t + 2]});
    }
}

uint32_t melanger(uint32_t x) { x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16; return x; }

// ---------------------------------------------------------------- RG-05 : 4 modèles de fenêtre et 4 de porte par style
constexpr int NS = (int)Style::Nombre;
const ModeleFenetre FENETRES[NS][4] = {
    {{"à la française, 2 vantaux, 3 carreaux", 2, 3, 1, false}, {"à la française, 2 vantaux, 4 carreaux", 2, 4, 1, false},
     {"2 vantaux et imposte", 2, 2, 1, true}, {"2 vantaux sans petits bois et imposte", 2, 1, 1, true}},                        // classique
    {{"oscillo-battant, 1 vantail", 1, 1, 1, false}, {"2 vantaux", 2, 1, 1, false},
     {"1 vantail et imposte", 1, 1, 1, true}, {"2 vantaux et imposte", 2, 1, 1, true}},                                          // moderne
    {{"2 vantaux, 3 carreaux", 2, 3, 1, false}, {"2 vantaux, 2 carreaux", 2, 2, 1, false},
     {"2 vantaux sans petits bois", 2, 1, 1, false}, {"2 vantaux, 4 carreaux", 2, 4, 1, false}},                               // méditerranéen
    {{"2 vantaux, 3 carreaux", 2, 3, 1, false}, {"2 vantaux, 2 carreaux", 2, 2, 1, false},
     {"1 vantail, 4 carreaux", 1, 2, 2, false}, {"1 vantail, 6 carreaux", 1, 3, 2, false}},                                   // village
    {{"châssis d'atelier, carreaux de 0,6 m", 0, 0, 6, false}, {"châssis d'atelier, carreaux de 0,5 m", 0, 0, 5, false},
     {"châssis d'atelier et vasistas", 0, 0, 6, true}, {"2 vantaux à petits bois", 2, 3, 2, false}},                          // industriel
    {{"baie coulissante, 2 vantaux", 2, 1, 1, false}, {"baie coulissante, 3 vantaux", 3, 1, 1, false},
     {"baie à galandage, 2 vantaux et imposte", 2, 1, 1, true}, {"baie fixe et vantail", 1, 1, 1, false}},                     // villa
};
const ModelePorte PORTES[NS][4] = {
    {{"porte d'immeuble à panneaux et imposte", Vantail::Panneaux, 2, true, true}, {"porte cochère à panneaux", Vantail::Panneaux, 2, false, true},
     {"porte vitrée en partie haute, grille de fer forgé", Vantail::VitreHaut, 2, true, true}, {"porte à panneaux, 1 vantail", Vantail::Panneaux, 1, true, true}},
    {{"porte vitrée aluminium", Vantail::Vitre, 1, false, false}, {"porte vitrée aluminium et imposte", Vantail::Vitre, 1, true, false},
     {"porte vitrée aluminium, 2 vantaux", Vantail::Vitre, 2, false, false}, {"porte pleine à rainures", Vantail::Rainures, 1, false, false}},
    {{"porte à panneaux", Vantail::Panneaux, 1, false, true}, {"porte à planches", Vantail::Planches, 1, false, true},
     {"porte vitrée en partie haute, grille", Vantail::VitreHaut, 1, false, true}, {"porte à panneaux et imposte", Vantail::Panneaux, 1, true, true}},
    {{"porte à planches et pentures", Vantail::Planches, 1, false, true}, {"porte à panneaux", Vantail::Panneaux, 1, false, true},
     {"porte à planches, jour vitré", Vantail::VitreHaut, 1, false, true}, {"porte à planches larges", Vantail::Planches, 1, false, true}},
    {{"porte métallique à panneaux", Vantail::Panneaux, 1, false, false}, {"porte d'atelier vitrée à petits bois", Vantail::VitreHaut, 1, true, false},
     {"porte métallique à 2 vantaux", Vantail::Panneaux, 2, false, false}, {"porte métallique à rainures", Vantail::Rainures, 1, true, false}},
    {{"porte pleine à rainures", Vantail::Rainures, 1, false, true}, {"porte vitrée aluminium", Vantail::Vitre, 1, false, false},
     {"porte pleine en bois", Vantail::Panneaux, 1, false, true}, {"porte vitrée en partie haute", Vantail::VitreHaut, 1, false, true}},
};

// ---------------------------------------------------------------- mesures (m)
constexpr float DORMANT = 0.06f, DORMANT_EP = 0.07f, OUVRANT = 0.055f, OUVRANT_EP = 0.055f, PETIT_BOIS = 0.026f, JEU = 0.003f;
constexpr float SCELLEMENT = 0.01f;   // dormants et huisseries enfoncés de 1 cm dans la maçonnerie (pas de fente de lumière au bord du tableau)
constexpr float CHANFREIN = 0.01f, JET_D_EAU = 0.03f, TRAVERSE_IMPOSTE = 0.07f;
constexpr float HUISSERIE_IMMEUBLE = 0.07f, HUISSERIE_MAISON = 0.035f, VANTAIL_EP = 0.05f, BEQUILLE_Y = 1.0f;
constexpr float SECTION_GARAGE = 0.5f, LAME_ROULANT = 0.04f, COULISSE = 0.03f;
constexpr float PERSIENNE_CADRE = 0.06f, PERSIENNE_PAS = 0.05f, PERSIENNE_LAME = 0.065f, VOLET_EP = 0.035f, VOLET_JEU = 0.02f;
constexpr float APPUI_SAILLIE = 0.05f, APPUI_DEBORD = 0.05f, APPUI_EP = 0.06f, GLACIS = 0.012f;
constexpr float CADRE_SAILLIE = 0.03f;   // encadrement en saillie du classique et du méditerranéen (archi_geo.cpp)
constexpr int FERMES = 12, ENTROUVERTS = 10;   // RG-03 : sur 100 baies, 12 volets fermés et 10 entrouverts (22 %)

bool maison(Style s) { return s == Style::Village || s == Style::Villa; }

// poignée de fenêtre (crémone) sur le montant d'un ouvrant : tringle, plaque et olive
void cremone(Maillage& m, float x, float y0, float y1, float yp, float z) {
    bloc(m, x - 0.006f, x + 0.006f, y0 + 0.08f, y1 - 0.08f, z, z + 0.008f);         // tringle
    bloc(m, x - 0.016f, x + 0.016f, yp - 0.06f, yp + 0.06f, z, z + 0.01f);          // boîtier
    bloc(m, x - 0.009f, x + 0.009f, yp - 0.1f, yp + 0.01f, z + 0.01f, z + 0.028f);  // olive tournée vers le bas
}
// béquille de porte sur plaque, avec serrure (x : axe de la plaque, côté : +1 béquille vers +x, −1 vers −x)
void bequille(Maillage& m, float x, float y, float z, float cote) {
    bloc(m, x - 0.0225f, x + 0.0225f, y - 0.16f, y + 0.08f, z, z + 0.008f);           // plaque de 45 × 240 mm
    bloc(m, x - 0.009f, x + 0.009f, y - 0.009f, y + 0.009f, z + 0.008f, z + 0.055f);   // carré de la béquille
    const float xa = cote > 0 ? x - 0.009f : x - 0.13f, xb = cote > 0 ? x + 0.13f : x + 0.009f;
    bloc(m, xa, xb, y - 0.01f, y + 0.01f, z + 0.045f, z + 0.065f);                    // béquille de 130 mm
    bloc(m, x - 0.008f, x + 0.008f, y - 0.12f, y - 0.088f, z + 0.008f, z + 0.016f);    // rosace du cylindre de serrure
}

// ---------------------------------------------------------------- fenêtre et porte-fenêtre
void fenetre(const Plan& p, const Baie& b, float d, int lod, Batiment& out) {
    Maillage& m = out.part[partChassis(p.style)];
    const ModeleFenetre& M = modeleFenetre(p);
    const float zf = -d;
    const Rect R{b.x - b.l / 2, b.y, b.x + b.l / 2, b.y + b.h};
    cadre(m, R.retreci(-SCELLEMENT), DORMANT + SCELLEMENT, zf, zf + DORMANT_EP, CHANFREIN);   // dormant scellé au fond du tableau (1 cm dans le mur : aucun jour au bord)
    Rect O = R.retreci(DORMANT);
    if (M.imposte && O.h() > 1.2f) {   // imposte fixe : traverse et vitrage au-dessus des ouvrants
        const float hI = std::clamp(0.22f * O.h(), 0.3f, 0.5f), yt = O.y1 - hI;
        bloc(m, O.x0, O.x1, yt - TRAVERSE_IMPOSTE, yt, zf, zf + DORMANT_EP, AV | HA | BA);
        if (lod == 0 && M.vantaux >= 2) {   // petit bois vertical de l'imposte, dans l'axe des ouvrants
            for (int k = 1; k < M.vantaux; k++) { const float x = O.x0 + O.l() * k / M.vantaux; bloc(m, x - PETIT_BOIS / 2, x + PETIT_BOIS / 2, yt, O.y1, zf, zf + 0.035f); }
        }
        O.y1 = yt - TRAVERSE_IMPOSTE;
    }
    if (M.vantaux == 0) {   // châssis d'atelier fixe : grille de petits bois métalliques
        const float pas = M.colonnes * 0.1f;
        const int nx = std::max(2, (int)std::lround(O.l() / pas)), ny = std::max(2, (int)std::lround(O.h() / pas));
        if (lod == 0) {
            for (int i = 1; i < nx; i++) { const float x = O.x0 + O.l() * i / nx; bloc(m, x - 0.018f, x + 0.018f, O.y0, O.y1, zf, zf + 0.04f); }
            for (int j = 1; j < ny; j++) { const float y = O.y0 + O.h() * j / ny; bloc(m, O.x0, O.x1, y - 0.018f, y + 0.018f, zf, zf + 0.04f); }
        }
        return;
    }
    const int n = M.vantaux;
    const float w = O.l() / n;
    for (int k = 0; k < n; k++) {
        const Rect V{O.x0 + k * w + JEU, O.y0 + JEU, O.x0 + (k + 1) * w - JEU, O.y1 - JEU};
        if (lod >= 1) {   // de loin : seulement les montants qui se voient (rencontre des ouvrants)
            if (k > 0) bloc(m, V.x0 - JEU - OUVRANT, V.x0 - JEU + OUVRANT, V.y0, V.y1, zf, zf + OUVRANT_EP);
            continue;
        }
        cadre(m, V, OUVRANT, zf, zf + OUVRANT_EP, 0.008f, true);
        const Rect G = V.retreci(OUVRANT);
        bloc(m, G.x0, G.x1, G.y0, G.y0 + JET_D_EAU, zf + 0.035f, zf + OUVRANT_EP + 0.02f, AV | HA | GA | DR);   // jet d'eau
        for (int c = 1; c < M.colonnes; c++) { const float x = G.x0 + G.l() * c / M.colonnes; bloc(m, x - PETIT_BOIS / 2, x + PETIT_BOIS / 2, G.y0, G.y1, zf, zf + 0.035f); }
        for (int r = 1; r < M.rangs; r++) { const float y = G.y0 + G.h() * r / M.rangs; bloc(m, G.x0, G.x1, y - PETIT_BOIS / 2, y + PETIT_BOIS / 2, zf, zf + 0.035f); }
    }
    if (lod == 0) {   // crémone sur le montant de l'ouvrant de service (à droite de la rencontre, ou côté libre d'un seul vantail)
        const float yp = std::clamp(p.niveauY(b.niveau) + 1.1f, O.y0 + 0.25f, O.y1 - 0.15f);
        const float x = n >= 2 ? O.x0 + w * (n / 2) + JEU + OUVRANT / 2 : O.x1 - JEU - OUVRANT / 2;
        cremone(m, x, O.y0, O.y1, yp, zf + OUVRANT_EP);
    }
}

// ---------------------------------------------------------------- vantail de porte (repère du vantail : face à zv)
void vantailPorte(const ModelePorte& M, Rect L, float zv, float ys, bool service, bool grille, int lod, Batiment& out) {
    Maillage& v = out.part[M.bois ? MENUISERIE : METAL];
    Maillage& met = out.part[METAL];
    const float zb = zv - VANTAIL_EP;
    cotes(v, L, zb, zv);   // tranche du vantail (jeu avec l'huisserie)
    std::vector<Rect> trous;
    switch (M.vantail) {
    case Vantail::Panneaux: {   // bâti : montants 12 cm, traverse haute 12 cm, basse 20 cm, intermédiaire à 1/3
        const float mo = std::min(0.12f, L.l() * 0.18f), th = 0.12f, tb = 0.2f, ti = 0.12f;
        const float yi = L.y0 + tb + (L.h() - tb - th - ti) * 0.38f;
        const int cols = L.l() > 0.75f ? 2 : 1;
        const float cw = (L.l() - 2 * mo - (cols - 1) * mo) / cols;
        for (int c = 0; c < cols; c++) {
            const float x0 = L.x0 + mo + c * (cw + mo);
            trous.push_back({x0, L.y0 + tb, x0 + cw, yi});
            trous.push_back({x0, yi + ti, x0 + cw, L.y1 - th});
        }
        facePercee(v, L, zv, trous);
        for (const Rect& t : trous) panneau(v, t, zv);
        break;
    }
    case Vantail::Planches: case Vantail::Rainures: {   // planches verticales (bois) ou lames horizontales (rainures), joints en V
        const bool vert = M.vantail == Vantail::Planches;
        const float larg = vert ? (L.l() > 1.f ? 0.18f : 0.13f) : 0.15f, tot = vert ? L.l() : L.h();
        const int nb = std::max(2, (int)std::lround(tot / larg));
        for (int k = 0; k < nb; k++) {
            const Rect P = vert ? Rect{L.x0 + tot * k / nb, L.y0, L.x0 + tot * (k + 1) / nb, L.y1} : Rect{L.x0, L.y0 + tot * k / nb, L.x1, L.y0 + tot * (k + 1) / nb};
            const Rect F = P.retreci(0.004f);
            anneau(v, P, zv - 0.005f, F, zv);   // joint en V (chanfreins de 4 mm)
            face(v, {F.x0, F.y0, zv}, {F.x1, F.y0, zv}, {F.x1, F.y1, zv}, {F.x0, F.y1, zv}, {0, 0, 1});
        }
        if (vert && lod == 0) {   // pentures forgées (côté des gonds) : deux bandes de fer, comme les vraies portes de pays
            const bool gondsGauche = !service;
            for (float yy : {L.y0 + 0.3f, L.y1 - 0.3f}) {
                const float xa = gondsGauche ? L.x0 - 0.01f : L.x1 - L.l() * 0.7f, xb = gondsGauche ? L.x0 + L.l() * 0.7f : L.x1 + 0.01f;
                bloc(met, xa, xb, yy - 0.022f, yy + 0.022f, zv, zv + 0.006f);
            }
        }
        break;
    }
    case Vantail::VitreHaut: {   // soubassement à panneau, vitrage en partie haute (grille de fer forgé ou petits bois)
        const float mo = std::min(0.12f, L.l() * 0.18f), th = 0.12f, tb = 0.2f, ti = 0.14f;
        const float yi = std::min(L.y0 + 1.0f, L.y1 - th - 0.5f);
        const Rect bas{L.x0 + mo, L.y0 + tb, L.x1 - mo, yi};
        const Rect G{L.x0 + mo, yi + ti, L.x1 - mo, L.y1 - th};
        trous = {bas, G};
        facePercee(v, L, zv, trous);
        panneau(v, bas, zv);
        jour(v, G, zv, zb + 0.002f);
        if (lod == 0) {
            if (grille) {   // barreaux de fer forgé (16 mm) devant le vitrage, au pas de 11 cm (NF P01-012 : jour ≤ 11 cm)
                const int nb = std::max(2, (int)std::ceil(G.l() / 0.11f));
                for (int k = 1; k < nb; k++) { const float x = G.x0 + G.l() * k / nb; bloc(met, x - 0.008f, x + 0.008f, G.y0, G.y1, zv - 0.03f, zv - 0.014f); }
                bloc(met, G.x0, G.x1, (G.y0 + G.y1) / 2 - 0.01f, (G.y0 + G.y1) / 2 + 0.01f, zv - 0.032f, zv - 0.012f);
            } else {   // petits bois : 2 × 2 carreaux
                const float x = (G.x0 + G.x1) / 2, y = (G.y0 + G.y1) / 2;
                bloc(v, x - PETIT_BOIS / 2, x + PETIT_BOIS / 2, G.y0, G.y1, zb, zv - 0.015f);
                bloc(v, G.x0, G.x1, y - PETIT_BOIS / 2, y + PETIT_BOIS / 2, zb, zv - 0.015f);
            }
        }
        break;
    }
    case Vantail::Vitre: {   // vantail aluminium vitré : montants 8 cm, traverse basse 12 cm
        const Rect G{L.x0 + 0.08f, L.y0 + 0.12f, L.x1 - 0.08f, L.y1 - 0.08f};
        facePercee(v, L, zv, {G});
        jour(v, G, zv, zb + 0.002f);
        if (lod == 0) {   // barre de tirage verticale (inox, 30 mm) sur entretoises
            const float x = service ? G.x0 + 0.06f : G.x1 - 0.06f, y0 = ys + 0.6f, y1 = ys + 1.6f;
            bloc(met, x - 0.015f, x + 0.015f, y0, y1, zv + 0.05f, zv + 0.075f);
            for (float yy : {y0 + 0.1f, y1 - 0.1f}) bloc(met, x - 0.008f, x + 0.008f, yy - 0.008f, yy + 0.008f, zv, zv + 0.05f);
        }
        return;   // pas de béquille sur une porte vitrée à barre
    }
    }
    if (service && lod == 0)   // béquille et serrure du vantail de service, côté libre (droite), à 1,00 m du seuil
        bequille(met, L.x1 - std::min(0.07f, L.l() * 0.12f), ys + BEQUILLE_Y, zv, -1);
}

void porte(const Plan& p, const Baie& b, float d, int lod, Batiment& out) {
    const ModelePorte& M = modelePorte(p);
    const float zf = -d, x0 = b.x - b.l / 2, x1 = b.x + b.l / 2, y1 = b.y + b.h;
    const float hui = maison(p.style) ? HUISSERIE_MAISON : HUISSERIE_IMMEUBLE;
    Maillage& hm = out.part[M.bois ? MENUISERIE : METAL];
    // seuil de pierre (2 cm au-dessus du trottoir, saillie de 4 cm)
    const float ys = 0.02f;
    bloc(out.part[ENCADREMENT], x0 - 0.03f, x1 + 0.03f, -0.05f, ys, zf, 0.04f, AV | HA | GA | DR);
    cadre(hm, {x0 - SCELLEMENT, ys, x1 + SCELLEMENT, y1 + SCELLEMENT}, hui + SCELLEMENT, zf - 0.01f, zf + VANTAIL_EP + 0.02f, CHANFREIN, false, false);   // huisserie (3 côtés)
    Rect L{x0 + hui, ys, x1 - hui, y1 - hui};
    if (M.imposte && L.h() > 2.25f) {   // imposte vitrée : traverse et petits bois
        const float hI = std::min(0.6f, L.h() - 2.1f), yt = L.y1 - hI;
        bloc(hm, L.x0, L.x1, yt - TRAVERSE_IMPOSTE, yt, zf - 0.01f, zf + VANTAIL_EP + 0.02f, AV | HA | BA);
        if (lod == 0) {
            const int nb = std::max(2, (int)std::lround(L.l() / 0.45f));
            for (int k = 1; k < nb; k++) { const float x = L.x0 + L.l() * k / nb; bloc(hm, x - PETIT_BOIS / 2, x + PETIT_BOIS / 2, yt, L.y1, zf, zf + 0.035f); }
        }
        L.y1 = yt - TRAVERSE_IMPOSTE;
    }
    const int n = (M.vantaux >= 2 && L.l() >= 1.05f) ? 2 : 1;
    const bool grille = p.style == Style::Classique || p.style == Style::Mediterraneen;
    const float w = L.l() / n, zv = zf + VANTAIL_EP;
    for (int k = 0; k < n; k++) {
        const Rect V{L.x0 + k * w + 0.004f, L.y0 + 0.008f, L.x0 + (k + 1) * w - 0.004f, L.y1 - 0.004f};
        vantailPorte(M, V, zv, ys, k == 0, grille, lod, out);
    }
    if (n == 2 && lod == 0 && M.vantail != Vantail::Vitre)   // béquille du vantail de service au milieu (côté de la rencontre) ; l'autre est dormant
        bequille(out.part[METAL], L.x0 + w + 0.004f + 0.07f, ys + BEQUILLE_Y, zv, 1);
}

// ---------------------------------------------------------------- porte de garage sectionnelle
void garage(const Plan& p, const Baie& b, float d, int lod, Batiment& out) {
    Maillage& m = out.part[partChassis(p.style)];
    const float zf = -d, x0 = b.x - b.l / 2, x1 = b.x + b.l / 2, y1 = b.y + b.h;
    bloc(out.part[ENCADREMENT], x0 - 0.02f, x1 + 0.02f, -0.05f, 0.01f, zf, 0.02f, AV | HA | GA | DR);   // seuil
    cadre(m, {x0 - SCELLEMENT, 0.01f, x1 + SCELLEMENT, y1 + SCELLEMENT}, 0.06f + SCELLEMENT, zf - 0.01f, zf + 0.07f, CHANFREIN, false, false);   // encadrement (3 côtés)
    const Rect L{x0 + 0.06f, 0.01f, x1 - 0.06f, y1 - 0.06f};
    const int ns = std::max(3, (int)std::lround(L.h() / SECTION_GARAGE));
    const float z = zf + 0.045f;
    for (int k = 0; k < ns; k++) {   // sections : face plate, arêtes chanfreinées (joint en V entre deux sections)
        const Rect S{L.x0, L.y0 + L.h() * k / ns, L.x1, L.y0 + L.h() * (k + 1) / ns};
        const Rect F{S.x0, S.y0 + 0.012f, S.x1, S.y1 - 0.012f};
        anneau(m, {S.x0, S.y0, S.x1, S.y1}, z - 0.015f, {F.x0, F.y0, F.x1, F.y1}, z);
        face(m, {F.x0, F.y0, z}, {F.x1, F.y0, z}, {F.x1, F.y1, z}, {F.x0, F.y1, z}, {0, 0, 1});
        if (lod == 0) {   // nervures de la tôle : deux rainures par section
            for (int r = 1; r <= 2; r++) { const float y = S.y0 + S.h() * r / 3; bloc(m, F.x0, F.x1, y - 0.006f, y + 0.006f, z - 0.004f, z, AV | HA | BA); }
        }
    }
    if (lod == 0) bloc(out.part[METAL], b.x - 0.08f, b.x + 0.08f, L.y0 + 0.1f, L.y0 + 0.13f, z, z + 0.035f);   // poignée
}

// ---------------------------------------------------------------- vitrine : porte vitrée, barre de tirage, soubassements
void vitrine(const Plan& p, const Baie& b, float d, int lod, Batiment& out) {
    Maillage& met = out.part[METAL];
    const float z0 = -d, x0 = b.x - b.l / 2, x1 = b.x + b.l / 2;
    const int n = std::max(1, (int)std::ceil(b.l / 2.f));   // mêmes sections que le dormant (archi_details.cpp)
    const float w = b.l / n;
    // porte dans la première section (0,93 m de passage), un meneau de plus si la section est plus large
    const float pd0 = x0 + 0.06f, pd1 = std::min(x0 + w - 0.03f, pd0 + 0.97f);
    if (x0 + w - 0.03f - pd1 > 0.25f) bloc(met, pd1, pd1 + 0.06f, b.y + 0.12f, b.y + std::min(b.h, 2.36f) - 0.06f, z0, z0 + 0.06f, AV | GA | DR);
    bloc(out.part[ENCADREMENT], pd0, pd1, -0.05f, 0.02f, z0, 0.04f, AV | HA | GA | DR);   // seuil devant la porte
    const float ph = std::min(b.h, 2.36f) - 0.06f;
    const Rect L{pd0 + 0.004f, 0.02f, pd1 - 0.004f, b.y + ph};
    cadre(met, L, 0.07f, z0 + 0.005f, z0 + 0.055f, 0.006f, true, true);   // vantail aluminium
    if (lod == 0) {   // barre de tirage verticale
        const float x = L.x1 - 0.12f;
        bloc(met, x - 0.015f, x + 0.015f, 0.65f, 1.75f, z0 + 0.1f, z0 + 0.125f);
        for (float yy : {0.75f, 1.65f}) bloc(met, x - 0.008f, x + 0.008f, yy - 0.008f, yy + 0.008f, z0 + 0.055f, z0 + 0.1f);
    }
    // soubassement plein (0,45 m) sous le vitrage des autres sections : panneau à plate-bande
    const float xs = x0 + w - 0.03f - pd1 > 0.25f ? pd1 + 0.06f : x0 + w + 0.03f;
    if (x1 - 0.06f - xs > 0.3f) {
        const Rect S{xs, b.y + 0.12f, x1 - 0.06f, b.y + 0.45f};
        face(met, {S.x0, S.y0, z0 + 0.04f}, {S.x1, S.y0, z0 + 0.04f}, {S.x1, S.y1, z0 + 0.04f}, {S.x0, S.y1, z0 + 0.04f}, {0, 0, 1});
        bloc(met, S.x0, S.x1, S.y1, S.y1 + 0.03f, z0, z0 + 0.06f, AV | HA);   // traverse sous le vitrage
    }
}

// ---------------------------------------------------------------- volets
// persienne ou volet plein dans son repère local : x de 0 (gonds) à lv, y de y0 à y1, face extérieure (fermé) à z = 0,
// dos à z = −VOLET_EP
void vantailVolet(Volet t, float lv, float y0, float y1, int lod, Maillage& m) {
    const Rect R{0, y0, lv, y1};
    if (lod >= 1) { bloc(m, 0, lv, y0, y1, -VOLET_EP, 0, TOUT); return; }   // de loin : épaisseur du vantail, sans lames
    if (t == Volet::Persiennes) {
        cadre(m, R, PERSIENNE_CADRE, -VOLET_EP, 0, 0.006f, true);
        anneau(m, R.retreci(PERSIENNE_CADRE), -VOLET_EP, R, -VOLET_EP);   // dos du cadre
        const Rect I = R.retreci(PERSIENNE_CADRE);
        const float dz = PERSIENNE_LAME * 0.7071f;
        for (float y = I.y0; y + 0.001f < I.y1; y += PERSIENNE_PAS) {   // lames à 45°, nez vers l'extérieur et vers le bas
            const float ya = y, yb = std::min(I.y1, y + dz);
            face(m, {I.x0, ya, -0.004f}, {I.x1, ya, -0.004f}, {I.x1, yb, -0.004f - dz}, {I.x0, yb, -0.004f - dz}, {0, 1, 1});
        }
        return;
    }
    // volet plein : planches verticales (joints en V), barres et écharpe sur le dos
    const int nb = std::max(2, (int)std::lround(lv / 0.12f));
    for (int k = 0; k < nb; k++) {
        const Rect P{lv * k / nb, y0, lv * (k + 1) / nb, y1}, F = P.retreci(0.003f);
        anneau(m, P, -0.004f, F, 0);
        face(m, {F.x0, F.y0, 0}, {F.x1, F.y0, 0}, {F.x1, F.y1, 0}, {F.x0, F.y1, 0}, {0, 0, 1});
    }
    cotes(m, R, -VOLET_EP + 0.012f, 0);
    bloc(m, 0, lv, y0, y1, -VOLET_EP + 0.012f, -0.004f, AR);   // dos des planches
    const float h = y1 - y0;
    for (float yy : {y0 + 0.12f * h, y1 - 0.12f * h}) bloc(m, 0.02f, lv - 0.02f, yy - 0.05f, yy + 0.05f, -VOLET_EP, -VOLET_EP + 0.012f, TOUT & ~AV);   // barres
    const float ya = y0 + 0.12f * h + 0.05f, yb = y1 - 0.12f * h - 0.05f;   // écharpe : de la barre basse côté gonds à la barre haute côté libre
    face(m, {0.03f, ya, -VOLET_EP}, {0.13f, ya, -VOLET_EP}, {lv - 0.03f, yb, -VOLET_EP}, {lv - 0.13f, yb, -VOLET_EP}, {0, 0, -1});
}

}  // namespace

// ---------------------------------------------------------------- interface
int indiceModeleFenetre(const Plan& p) { return (int)(melanger(p.graine * 2654435761u + 0x51u + (uint32_t)p.style) % 4u); }
int indiceModelePorte(const Plan& p) { return (int)(melanger(p.graine * 40503u + 0x9e37u + (uint32_t)p.style * 7u) % 4u); }
const ModeleFenetre& modeleFenetre(const Plan& p) { return FENETRES[std::min((int)p.style, NS - 1)][indiceModeleFenetre(p)]; }
const ModelePorte& modelePorte(const Plan& p) { return PORTES[std::min((int)p.style, NS - 1)][indiceModelePorte(p)]; }

bool porteVitree(const Plan& p) { const Vantail v = modelePorte(p).vantail; return v == Vantail::VitreHaut || v == Vantail::Vitre; }
int partChassis(Style s) { return s == Style::Classique || s == Style::Mediterraneen || s == Style::Village ? CHASSIS : METAL; }

Volet voletDuStyle(Style s) {
    switch (s) {
    case Style::Mediterraneen: return Volet::Persiennes;
    case Style::Village: return Volet::Pleins;
    case Style::Moderne: case Style::Villa: return Volet::Roulant;
    default: return Volet::Aucun;   // classique (persiennes métalliques repliées dans le tableau, invisibles), industriel
    }
}
EtatVolet etatVolet(const Plan& p, const Baie& b) {
    const int g = (int)(melanger((uint32_t)graineBaie(p, b.niveau, b.travee) * 977u + p.graine * 131u + 7u) % 100u);
    return g < FERMES ? EtatVolet::Ferme : g < FERMES + ENTROUVERTS ? EtatVolet::Entrouvert : EtatVolet::Ouvert;
}
bool voletsBattants(const Plan& p, const Baie& b) {
    const Volet t = voletDuStyle(p.style);
    if ((t != Volet::Persiennes && t != Volet::Pleins) || b.type != Ouverture::Fenetre) return false;
    const float lv = b.l / 2, place = (p.travee - b.l) / 2 - VOLET_JEU;   // ouverts, ils se rabattent jusqu'au milieu du trumeau
    return lv <= place && b.x - b.l / 2 - lv > 0.05f && b.x + b.l / 2 + lv < p.largeur - 0.05f;
}

void menuiserie(const Plan& p, const Baie& b, float d, int lod, Batiment& out) {
    if (lod >= 2) return;
    switch (b.type) {
    case Ouverture::Fenetre: case Ouverture::PorteFenetre: fenetre(p, b, d, lod, out); break;
    case Ouverture::Porte: porte(p, b, d, lod, out); break;
    case Ouverture::PorteGarage: garage(p, b, d, lod, out); break;
    case Ouverture::Vitrine: vitrine(p, b, d, lod, out); break;
    }
}

void volets(const Plan& p, const Baie& b, float d, int lod, Batiment& out) {
    if (lod >= 2 || (b.type != Ouverture::Fenetre && b.type != Ouverture::PorteFenetre)) return;
    const Volet t = voletDuStyle(p.style);
    const EtatVolet e = etatVolet(p, b);
    const float x0 = b.x - b.l / 2, x1 = b.x + b.l / 2, y0 = b.y, y1 = b.y + b.h;
    if (t == Volet::Roulant) {   // coulisses dans le tableau, lame finale sous le coffre, tablier descendu selon l'état
        Maillage& m = out.part[METAL];
        const float za = -0.075f, zb = -0.045f;
        if (d < 0.1f) return;
        bloc(m, x0, x0 + COULISSE, y0, y1, za, zb, AV | DR);
        bloc(m, x1 - COULISSE, x1, y0, y1, za, zb, AV | GA);
        const float frac = e == EtatVolet::Ferme ? 1.f : e == EtatVolet::Entrouvert ? 0.55f : 0.f;
        const float yb = y1 - std::max(0.05f, frac * b.h);
        const float xa = x0 + COULISSE * 0.5f, xb = x1 - COULISSE * 0.5f, zm = (za + zb) / 2;
        if (lod == 0) {
            for (float y = y1; y - LAME_ROULANT > yb - 1e-4f; y -= LAME_ROULANT) {   // lames bombées : deux pans par lame
                face(m, {xa, y - LAME_ROULANT / 2, zm + 0.006f}, {xb, y - LAME_ROULANT / 2, zm + 0.006f}, {xb, y, zm}, {xa, y, zm}, {0, 0.3f, 1});
                face(m, {xa, y - LAME_ROULANT, zm}, {xb, y - LAME_ROULANT, zm}, {xb, y - LAME_ROULANT / 2, zm + 0.006f}, {xa, y - LAME_ROULANT / 2, zm + 0.006f}, {0, -0.3f, 1});
            }
        } else if (frac > 0) face(m, {xa, yb, zm}, {xb, yb, zm}, {xb, y1, zm}, {xa, y1, zm}, {0, 0, 1});
        bloc(m, xa, xb, yb - 0.03f, yb, za + 0.005f, zb - 0.002f, AV | BA | HA);   // lame finale
        return;
    }
    if (!voletsBattants(p, b)) return;
    Maillage& m = out.part[VOLET];
    const float lv = b.l / 2 - 0.004f;
    const float zh = p.style == Style::Mediterraneen ? CADRE_SAILLIE : 0.f;   // gonds au nu de l'encadrement
    // angle : fermé (0), entrouvert (35° et 50°, jamais deux pareils), ouvert rabattu contre le mur (π)
    const float a = e == EtatVolet::Ferme ? 0.f : e == EtatVolet::Entrouvert ? 35.f * PI / 180.f : PI;
    Maillage loc;
    vantailVolet(t, lv, y0, y1, lod, loc);
    poserVantail(loc, x0, zh, a, false, m);
    poserVantail(loc, x1, zh, e == EtatVolet::Entrouvert ? 50.f * PI / 180.f : a, true, m);
}

void encadrement(Maillage& m, float x0, float y0, float x1, float y1, float e, float s) {
    const Rect E{x0 - e, y0, x1 + e, y1 + e}, I{x0, y0, x1, y1}, Ic{x0 - 0.015f, y0, x1 + 0.015f, y1 + 0.015f};
    anneau(m, E, s, Ic, s, false);            // face vue
    anneau(m, Ic, s, I, s - 0.015f, false);   // chanfrein de 15 mm vers la baie
    anneau(m, I, s - 0.015f, I, 0, false);    // tranche intérieure
    cotes(m, E, 0, s, false);                 // tranches extérieures
    for (float xa : {E.x0, x1}) face(m, {xa, y0, 0}, {xa + e, y0, 0}, {xa + e, y0, s}, {xa, y0, s}, {0, -1, 0});   // pieds des jambages
}

void appuiEtLinteau(const Plan& p, const Baie& b, Batiment& out) {
    Maillage& m = out.part[ENCADREMENT];
    const float x0 = b.x - b.l / 2, x1 = b.x + b.l / 2, y0 = b.y, y1 = b.y + b.h;
    if (b.type == Ouverture::Fenetre) {   // appui de pierre : glacis vers l'extérieur, nez, larmier sous le nez
        const float a = x0 - APPUI_DEBORD, c = x1 + APPUI_DEBORD, S = APPUI_SAILLIE, yb = y0 - APPUI_EP;
        face(m, {a, y0, 0}, {c, y0, 0}, {c, y0 - GLACIS, S}, {a, y0 - GLACIS, S}, {0, 1, 0.2f});         // glacis
        face(m, {a, yb, S}, {c, yb, S}, {c, y0 - GLACIS, S}, {a, y0 - GLACIS, S}, {0, 0, 1});              // nez
        face(m, {a, yb, S}, {c, yb, S}, {c, yb, S - 0.015f}, {a, yb, S - 0.015f}, {0, -1, 0});             // sous-face avant
        face(m, {a, yb, S - 0.015f}, {c, yb, S - 0.015f}, {c, yb + 0.006f, S - 0.015f}, {a, yb + 0.006f, S - 0.015f}, {0, 0, -1});   // larmier
        face(m, {a, yb + 0.006f, S - 0.015f}, {c, yb + 0.006f, S - 0.015f}, {c, yb + 0.006f, S - 0.025f}, {a, yb + 0.006f, S - 0.025f}, {0, -1, 0});
        face(m, {a, yb + 0.006f, S - 0.025f}, {c, yb + 0.006f, S - 0.025f}, {c, yb, S - 0.025f}, {a, yb, S - 0.025f}, {0, 0, 1});
        face(m, {a, yb, S - 0.025f}, {c, yb, S - 0.025f}, {c, yb, 0}, {a, yb, 0}, {0, -1, 0});
        for (float x : {a, c}) {   // abouts (profil du glacis)
            const vec3 dir{x == a ? -1.f : 1.f, 0, 0};
            face(m, {x, yb, 0}, {x, yb, S}, {x, y0 - GLACIS, S}, {x, y0, 0}, dir);
        }
    }
    // linteau visible : pierre (village), béton (industriel), au-dessus des fenêtres et des portes
    if ((p.style == Style::Village || p.style == Style::Industriel) && b.type != Ouverture::Vitrine) {
        const float e = p.style == Style::Village ? 0.18f : 0.2f, deb = p.style == Style::Village ? 0.12f : 0.1f;
        bloc(m, x0 - deb, x1 + deb, y1, y1 + e, 0, 0.015f, AV | HA | BA | GA | DR);
    }
}

}  // namespace wfs::archi::ouvertures
