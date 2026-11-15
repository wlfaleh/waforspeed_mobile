// WaForSpeed — WFS-496 : atelier d'autocollants (voir atelier.h).
#include "atelier.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include "../../core/configgame.h"
#include "../../core/log.h"
#include "../../core/platform.h"             // readAsset (police des lettres et des chiffres)
#include "../../game/cars.h"
#include "../../game/commandes_tactiles.h"   // dessin2d (biseau, trait)
#include "../../game/game.h"                 // Touch
#include "../../game/personnalisation.h"
#include "../../game/sauvegarde.h"
#include "../../platform/manette.h"          // focus de la manette ; zone retour = touche Retour d'Android
#include "../../render/renderer.h"
#include "../../render/vehicule_skin.h"
#include "../boutons.h"         // WFS-549 : boutons R19
#include "../garage_achat.h"    // étoile dessinée (prix des packs)
#include "../garage_commun.h"   // cadenas (WFS-489)
#include "../langue.h"
#include "../petit_ecran.h"
#include "../text.h"
#include "../verre.h"           // WFS-548 : panneaux de verre, sur-titres
#include "atelier_calques.h"
#include "reglages_appui.h"     // appui::dansCible : cibles de 48 dp
#include "textes_banc.h"        // banc « textes » : journal des textes de cet écran

namespace wfs {
namespace ecrans::atelier {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
// design-street-racing : textes, gris, accents orange (action) et cyan (information, sélection)
const unsigned TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF;
constexpr double DUREE_MESSAGE = 2.4;

struct Zone {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};

// ---- vues (RG-01) : angle autour de la voiture (θ, 0 = derrière, voiture vers −Z) et hauteur (φ)
enum Vue { V_GAUCHE, V_DROITE, V_AVANT, V_ARRIERE, V_DESSUS };
constexpr int NB_VUES = 5;
constexpr float THETA[NB_VUES] = {-PI / 2, PI / 2, PI, 0, -PI / 2};
constexpr float PHI[NB_VUES] = {0.16f, 0.16f, 0.16f, 0.16f, 1.40f};
constexpr float PHI_MIN = 0.02f, PHI_MAX = 1.42f;
const char* const CLES_VUES[NB_VUES] = {"atelier.vue.gauche", "atelier.vue.droite", "atelier.vue.avant", "atelier.vue.arriere", "atelier.vue.dessus"};
const char* const CLES_FAMILLES[NB_FAMILLES] = {"atelier.famille.bandes", "atelier.famille.numeros", "atelier.famille.flammes",
                                                "atelier.famille.eclairs", "atelier.famille.etoiles", "atelier.famille.lettres"};
const char* const CLES_MOTIFS[NB_FAMILLES] = {"atelier.motif.bande", "atelier.motif.numero", "atelier.motif.flamme",
                                              "atelier.motif.eclair", "atelier.motif.etoile", "atelier.motif.lettre"};

// ---- cibles (action au relâcher, sur la cible appuyée : WFS-115)
enum Cible {
    RIEN = -1, VUE0 = 0, FAMILLE0 = 10, MOTIF0 = 20, PAGE_PREC = 40, PAGE_SUIV, CALQUE0 = 50, OEIL0 = 80, LISTE_HAUT = 110, LISTE_BAS,
    MONTER, DESCENDRE, ROT_MOINS, ROT_PLUS, ECH_MOINS, ECH_PLUS, MIROIR, COULEUR, SUPPRIMER, ANNULER, REFAIRE, VALIDER, QUITTER,
    PALETTE0 = 140, CONF_QUITTER = 180, CONF_RESTER, FOND_PALETTE
};
constexpr int MOTIFS_PAGE = 12, LIGNES = 8;   // tuiles de la bibliothèque par page ; lignes visibles de la liste des calques
constexpr int NB_OUTILS = 7;                  // −5°, +5°, −5 %, +5 %, miroir, couleur, supprimer
enum Mode { M_AUCUN, M_CAMERA, M_PLACER, M_DEUX, M_FINI };

struct Doigt { int id = -1; float x = 0, y = 0; };

struct EtatEcran {
    bool ouvert = false;
    std::string car;
    int skin = -1;
    Etat e;
    int vue = V_GAUCHE;                       // −1 : vue libre (rotation au doigt)
    float theta = THETA[V_GAUCHE], phi = PHI[V_GAUCHE];
    int famille = 0, page = 0, premier = 0;   // onglet, page de la bibliothèque, 1re ligne de la liste
    bool palette = false, confirmation = false;
    std::string message; double tMessage = -100, maintenant = 0;
    double tOuvert = -1;
    // dernière image : caméra et taille
    mat4 vp = mat4::identity(); vec3 oeil{0, 2, 6}; float W = 1280, H = 720, k = 1; bool vpOk = false;
    vec3 bmin{-0.9f, 0, -2.2f}, bmax{0.9f, 1.45f, 2.2f};   // boîte du modèle montré (m)
    // touchers
    Doigt doigts[2]; int nDoigts = 0; Mode mode = M_AUCUN;
    int cible = RIEN, doigtCible = -1;
    float x0 = 0, y0 = 0, theta0 = 0, phi0 = 0, offA = 0, offB = 0, d0 = 1, a0 = 0; int rot0 = 0, ech0 = 100;
    // zones de la dernière image
    Zone bouton, vues[NB_VUES], familles[NB_FAMILLES], motifs[MOTIFS_PAGE], pagePrec, pageSuiv, calques[LIGNES], yeux[LIGNES],
         listeHaut, listeBas, monter, descendre, outils[NB_OUTILS], annuler, refaire, valider, quitter, palette28[NB_PEINTURES],
         fondPalette, confQuitter, confRester, panneauG, panneauD, barre;
    std::string motifTuile[MOTIFS_PAGE];
    float dp = 1, dpBouton = 1;
    // aperçu cuit (WFS-495) : texte posé, pour ne cuire qu'à chaque changement
    std::string cuit;                         // texte cuit de l'atelier (figé pendant un geste)
    std::string poseCle; bool poseEchec = false;
    std::string garde, gardeCuisson;          // cache de calquesCourse (texte gardé → texte à cuire)
    // police (lettres, chiffres) et contours des motifs, lus une fois
    std::vector<uint8_t> police; bool policeLue = false;
    std::map<std::string, std::vector<va::Contour>> contours;
};
EtatEcran g;

const std::vector<va::Contour>& contoursDe(const std::string& id) {
    auto it = g.contours.find(id);
    if (it != g.contours.end()) return it->second;
    if (!g.policeLue) { g.policeLue = true; if (!readAsset("fonts/Rajdhani-Bold.ttf", g.police)) WFS_ERR("atelier : police Rajdhani absente (WFS-496)"); }
    return g.contours[id] = va::contours(id, g.police);
}

void montrerMessage(const std::string& m) { g.message = m; g.tMessage = g.maintenant; WFS_LOG("atelier : « %s »", m.c_str()); }

// nom d'un calque dans la liste : « Bande 2 », « Numéro 7 », « Lettre A »
std::string nomMotif(const std::string& id) {
    const va::Motif* m = va::motif(id);
    if (!m) return id;
    const int f = (int)m->famille;
    std::string suffixe;
    if (m->famille == va::Famille::Chiffre) suffixe = id.substr(id.size() - 1);
    else if (m->famille == va::Famille::Lettre) suffixe = std::string(1, (char)std::toupper((unsigned char)id.back()));
    else {   // rang dans sa famille (bandes : large, double, filets, triple, flèche)
        int n = 0;
        for (const va::Motif& x : va::bibliotheque()) if (x.famille == m->famille) { n++; if (id == x.id) break; }
        suffixe = std::to_string(n);
    }
    return ui::tr(CLES_MOTIFS[f]) + " " + suffixe;
}

// ---------------------------------------------------------------- géométrie : plan de chaque côté de la voiture
const float VOITURE_Y = 0.08f;   // la voiture du garage est posée 8 cm au-dessus du sol (garage.cpp : plateau)

vec4 mul(const mat4& m, const vec4& v) {
    return {m.at(0, 0) * v.x + m.at(0, 1) * v.y + m.at(0, 2) * v.z + m.at(0, 3) * v.w, m.at(1, 0) * v.x + m.at(1, 1) * v.y + m.at(1, 2) * v.z + m.at(1, 3) * v.w,
            m.at(2, 0) * v.x + m.at(2, 1) * v.y + m.at(2, 2) * v.z + m.at(2, 3) * v.w, m.at(3, 0) * v.x + m.at(3, 1) * v.y + m.at(3, 2) * v.z + m.at(3, 3) * v.w};
}
float hauteurDessus(va::Direction d) {   // hauteur du plan des faces du dessus (capot, toit, coffre), approchée
    const float H = g.bmax.y;
    return d == va::Direction::Toit ? H : d == va::Direction::Capot ? 0.70f * H : 0.75f * H;
}
void ancre(va::Direction d, float an[3]) {
    const float mn[3] = {g.bmin.x, g.bmin.y, g.bmin.z}, mx[3] = {g.bmax.x, g.bmax.y, g.bmax.z};
    va::ancrage(d, mn, mx, an);
}
// point (a, b) du plan (m, vu de dehors, autour de l'ancrage ; repères de vehicule_autocollants.h) → repère de la voiture
vec3 pointPlan(va::Direction d, float a, float b) {
    float an[3]; ancre(d, an);
    switch (d) {
    case va::Direction::Gauche: return {g.bmin.x, an[1] + b, an[2] + a};
    case va::Direction::Droite: return {g.bmax.x, an[1] + b, an[2] - a};
    case va::Direction::Avant: return {-a, an[1] + b, g.bmin.z};
    case va::Direction::Arriere: return {a, an[1] + b, g.bmax.z};
    default: return {b, hauteurDessus(d), an[2] + a};
    }
}
vec3 normalePlan(va::Direction d) {
    switch (d) {
    case va::Direction::Gauche: return {-1, 0, 0};
    case va::Direction::Droite: return {1, 0, 0};
    case va::Direction::Avant: return {0, 0, -1};
    case va::Direction::Arriere: return {0, 0, 1};
    default: return {0, 1, 0};
    }
}
bool projeter(const vec3& pVoiture, float& x, float& y) {
    const vec4 c = mul(g.vp, {pVoiture.x, pVoiture.y + VOITURE_Y, pVoiture.z, 1});
    if (c.w <= 1e-4f) return false;
    x = (c.x / c.w * 0.5f + 0.5f) * g.W; y = (c.y / c.w * 0.5f + 0.5f) * g.H;
    return true;
}
bool faceVisible(va::Direction d) {
    const vec3 n = normalePlan(d), p = pointPlan(d, 0, 0);
    return dot(n, g.oeil - vec3{p.x, p.y + VOITURE_Y, p.z}) > 0;
}
// toucher (écran) → point (a, b) du plan de la direction d (m) : rayon de la caméra coupé par le plan
bool versPlan(va::Direction d, float sx, float sy, float& a, float& b) {
    if (!g.vpOk) return false;
    const mat4 inv = inverse(g.vp);
    const float nx = sx / g.W * 2 - 1, ny = sy / g.H * 2 - 1;
    const vec4 q1 = mul(inv, {nx, ny, 1, 1}), q2 = mul(inv, {nx, ny, 0.05f, 1});   // profondeur inversée : 1 = près
    if (std::fabs(q1.w) < 1e-6f || std::fabs(q2.w) < 1e-6f) return false;
    const vec3 p1{q1.x / q1.w, q1.y / q1.w - VOITURE_Y, q1.z / q1.w}, p2{q2.x / q2.w, q2.y / q2.w - VOITURE_Y, q2.z / q2.w};
    const vec3 dir = p2 - p1;
    const vec3 n = normalePlan(d), o = pointPlan(d, 0, 0);
    const float den = dot(n, dir);
    if (std::fabs(den) < 1e-5f) return false;
    const float t = dot(n, o - p1) / den;
    if (t < 0) return false;
    const vec3 P = p1 + dir * t;
    float an[3]; ancre(d, an);
    switch (d) {
    case va::Direction::Gauche: a = P.z - an[2]; b = P.y - an[1]; break;
    case va::Direction::Droite: a = an[2] - P.z; b = P.y - an[1]; break;
    case va::Direction::Avant: a = -P.x; b = P.y - an[1]; break;
    case va::Direction::Arriere: a = P.x; b = P.y - an[1]; break;
    default: a = P.z - an[2]; b = P.x; break;
    }
    return true;
}
// côté de la voiture que montre la caméra : direction d'un nouveau calque
va::Direction directionVue() {
    if (g.phi > 0.9f) return va::Direction::Capot;
    const float s = std::sin(g.theta), c = std::cos(g.theta);
    if (std::fabs(s) >= std::fabs(c)) return s < 0 ? va::Direction::Gauche : va::Direction::Droite;
    return c < 0 ? va::Direction::Avant : va::Direction::Arriere;
}
// contour à l'écran du calque (motif tourné, à l'échelle, posé) ; vide si son côté ne fait pas face à la caméra
std::vector<std::vector<vec2>> contourEcran(const va::Calque& c) {
    std::vector<std::vector<vec2>> sortie;
    if (!faceVisible(c.dir)) return sortie;
    const float s = c.echelle / 100.f, ang = c.rotation * PI / 180.f, co = std::cos(ang), si = std::sin(ang);
    for (const va::Contour& ct : contoursDe(c.motif)) {
        std::vector<vec2> v;
        const size_t pas = std::max<size_t>(1, ct.size() / 96);
        for (size_t i = 0; i < ct.size(); i += pas) {
            const float px = ct[i].first * s, py = ct[i].second * s;
            const float a = px * co - py * si + c.x / 100.f, b = px * si + py * co + c.y / 100.f;
            float x, y;
            if (projeter(pointPlan(c.dir, a, b), x, y)) v.push_back({x, y});
        }
        if (v.size() >= 2) sortie.push_back(std::move(v));
    }
    return sortie;
}
// le doigt est-il sur le calque choisi (boîte de son contour à l'écran, élargie) ?
bool surCalque(float x, float y) {
    const CalqueAtelier* ca = g.e.calqueChoisi();
    if (!ca || ca->masque) return false;
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    for (const auto& c : contourEcran(ca->c)) for (const vec2& p : c) { x0 = std::min(x0, p.x); y0 = std::min(y0, p.y); x1 = std::max(x1, p.x); y1 = std::max(y1, p.y); }
    if (x1 < x0) return false;
    const float marge = 36 * g.k;
    return x >= x0 - marge && x <= x1 + marge && y >= y0 - marge && y <= y1 + marge;
}

// ---------------------------------------------------------------- dessin
// icône d'un motif (interface 2D) : remplissage des contours par lignes, règle « non nulle » (comme la cuisson de WFS-495 : les
// langues d'une flamme qui se chevauchent restent pleines), dans la case (x, y, w, h)
void icone(Renderer& r, const std::string& id, float x, float y, float w, float h, const vec4& c) {
    const auto& cs = contoursDe(id);
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    for (const auto& ct : cs) for (const auto& p : ct) { x0 = std::min(x0, p.first); x1 = std::max(x1, p.first); y0 = std::min(y0, p.second); y1 = std::max(y1, p.second); }
    if (x1 <= x0 || y1 <= y0) return;
    const float e = std::min(w / (x1 - x0), h / (y1 - y0));
    const float ox = x + (w - (x1 - x0) * e) / 2, oy = y + (h - (y1 - y0) * e) / 2;   // motif centré, y vers le haut → vers le bas
    const float hh = (y1 - y0) * e;
    const int lignes = std::clamp((int)hh, 1, 64);
    const float pas = hh / lignes;
    std::vector<std::pair<float, int>> xs;   // abscisse du croisement, sens de l'arête (+1 montante, −1 descendante)
    for (int l = 0; l < lignes; l++) {
        const float sy = oy + (l + 0.5f) * pas;          // centre de la ligne (écran)
        const float my = y1 - (sy - oy) / e;             // même hauteur dans le motif
        xs.clear();
        for (const auto& ct : cs)
            for (size_t i = 0, n = ct.size(); i < n; i++) {
                const auto& p = ct[i]; const auto& q = ct[(i + 1) % n];
                if ((p.second > my) != (q.second > my))
                    xs.push_back({p.first + (my - p.second) / (q.second - p.second) * (q.first - p.first), q.second > p.second ? 1 : -1});
            }
        std::sort(xs.begin(), xs.end());
        int enroulement = 0;
        for (size_t i = 0; i + 1 < xs.size(); i++) {
            enroulement += xs[i].second;
            if (enroulement != 0) r.uiRect(ox + (xs[i].first - x0) * e, sy - pas / 2, (xs[i + 1].first - xs[i].first) * e, pas + 0.35f, c);
        }
    }
}

void bouton(Renderer& r, const Zone& z, const std::string& t, int cible, float px, bool actif = true) {
    const bool appuye = g.cible == cible;
    const vec4 gris = hex(ui::boutons::INACTIF_TEXTE);
    ui::boutons::verre(r, z.x, z.y, z.w, z.h, t, 1, appuye, px, actif ? nullptr : &gris);
}

// chevron de défilement de la liste (haut ou bas), dessiné au trait
void chevron(Renderer& r, const Zone& z, bool haut) {
    const float cx = z.x + z.w / 2, cy = z.y + z.h / 2, a = z.h * 0.22f, s = haut ? -1.f : 1.f;
    const vec2 p[3] = {{cx - a * 1.6f, cy - s * a * 0.8f}, {cx, cy + s * a * 0.8f}, {cx + a * 1.6f, cy - s * a * 0.8f}};
    dessin2d::trait(r, p, 3, 2.2f * g.k, hex(TEXTE));
}

void dessinerBibliotheque(Renderer& r, float x, float y, float w, float h) {
    const float k = g.k;
    g.panneauG = {x, y, w, h};
    verre::panneau(r, x, y, w, h, {14 * k});
    verre::surTitre(r, x + 16 * k, y + 14 * k, 13 * k, ui::tr("atelier.bibliotheque"));
    // onglets des familles : 2 rangées de 3
    const float tx = x + 12 * k, tw = (w - 24 * k - 2 * 6 * k) / 3, th = 30 * k;
    for (int f = 0; f < NB_FAMILLES; f++) {
        const float zx = tx + (f % 3) * (tw + 6 * k), zy = y + 36 * k + (f / 3) * (th + 6 * k);
        g.familles[f] = {zx, zy, tw, th};
        ui::boutons::selecteurFond(r, zx, zy, tw, th);
        ui::boutons::selecteurChoix(r, zx, zy, tw, th, ui::tr(CLES_FAMILLES[f]), f == g.famille, 1, g.cible == FAMILLE0 + f, 12 * k);
    }
    // tuiles : 3 colonnes × 4 rangées
    const auto motifs = motifsDe((va::Famille)g.famille);
    const int pages = std::max(1, ((int)motifs.size() + MOTIFS_PAGE - 1) / MOTIFS_PAGE);
    g.page = std::clamp(g.page, 0, pages - 1);
    const float gy = y + 36 * k + 2 * th + 18 * k, gap = 8 * k, cw = (w - 24 * k - 2 * gap) / 3;
    const float ch = std::min(cw * 0.78f, (y + h - 52 * k - gy - 3 * gap) / 4);
    for (int i = 0; i < MOTIFS_PAGE; i++) {
        g.motifs[i] = Zone{}; g.motifTuile[i].clear();
        const int m = g.page * MOTIFS_PAGE + i;
        if (m >= (int)motifs.size()) continue;
        const float zx = x + 12 * k + (i % 3) * (cw + gap), zy = gy + (i / 3) * (ch + gap);
        g.motifs[i] = {zx, zy, cw, ch}; g.motifTuile[i] = motifs[m]->id;
        const bool permis = motifPermis(motifs[m]->id);
        verre::panneau(r, zx, zy, cw, ch, {8 * k, 1, 1, false, g.cible == MOTIF0 + i ? ACCENT2 : 0u, 0.6f});
        icone(r, motifs[m]->id, zx + 10 * k, zy + 8 * k, cw - 20 * k, ch - (permis ? 16 : 30) * k, hex(permis ? TEXTE : MUTED, permis ? 1 : 0.55f));
        if (!permis) {   // RG-02 : cadenas et prix du pack
            garage::cadenas(r, zx + cw - 13 * k, zy + 13 * k, 9 * k, 1);
            const char* pack = packDuMotif(motifs[m]->id);
            const perso::Objet* o = pack ? perso::objet(pack) : nullptr;
            const std::string prix = o ? garage::nombreLangue(perso::prix(*o, "")) + " ★" : "";
            garage::texteEtoiles(r, zx + cw / 2, zy + ch - 18 * k, 11 * k, prix, hex(ACCENT), ui::CENTRE);
        }
    }
    // pages
    const float py = y + h - 42 * k, pw = 46 * k;
    g.pagePrec = g.pageSuiv = Zone{};
    if (pages > 1) {
        g.pagePrec = {x + 12 * k, py, pw, 32 * k}; g.pageSuiv = {x + w - 12 * k - pw, py, pw, 32 * k};
        bouton(r, g.pagePrec, "\u2190", PAGE_PREC, 15 * k, g.page > 0);
        bouton(r, g.pageSuiv, "\u2192", PAGE_SUIV, 15 * k, g.page + 1 < pages);
        ui::text(r, x + w / 2, py + 8 * k, 13 * k, std::to_string(g.page + 1) + " / " + std::to_string(pages), hex(MUTED), ui::CENTRE);
    }
}

void dessinerCalques(Renderer& r, float x, float y, float w, float h) {
    const float k = g.k;
    g.panneauD = {x, y, w, h};
    verre::panneau(r, x, y, w, h, {14 * k});
    const auto& cs = g.e.calques();
    const int n = (int)cs.size();
    verre::surTitre(r, x + 16 * k, y + 14 * k, 13 * k, ui::tr("atelier.calques", std::to_string(n), std::to_string(MAX_CALQUES)));
    // liste : le dessus (dernier calque) en haut
    const int sel = g.e.choisi() < 0 ? -1 : n - 1 - g.e.choisi();   // rang dans la liste
    if (sel >= 0) { if (sel < g.premier) g.premier = sel; if (sel >= g.premier + LIGNES) g.premier = sel - LIGNES + 1; }
    g.premier = std::clamp(g.premier, 0, std::max(0, n - LIGNES));
    const float lh = 34 * k, ly = y + 64 * k, ow = 92 * k;
    g.listeHaut = g.listeBas = Zone{};
    if (g.premier > 0) { g.listeHaut = {x + w / 2 - 30 * k, y + 34 * k, 60 * k, 26 * k}; bouton(r, g.listeHaut, "", LISTE_HAUT, 13 * k); chevron(r, g.listeHaut, true); }
    for (int l = 0; l < LIGNES; l++) {
        g.calques[l] = g.yeux[l] = Zone{};
        const int rang = g.premier + l;
        if (rang >= n) continue;
        const int i = n - 1 - rang;
        const CalqueAtelier& ca = cs[i];
        const float zy = ly + l * (lh + 4 * k);
        g.calques[l] = {x + 10 * k, zy, w - 20 * k - ow - 6 * k, lh};
        g.yeux[l] = {x + w - 10 * k - ow, zy, ow, lh};
        const bool choisi = i == g.e.choisi();
        verre::panneau(r, g.calques[l].x, zy, g.calques[l].w, lh, {7 * k, 1, 1, false, choisi ? ACCENT2 : 0u, choisi ? 1.f : 0.5f});
        const PeintureDef* p = peintureDef(ca.c.couleur);
        r.uiRect(g.calques[l].x + 10 * k, zy + 9 * k, 16 * k, 16 * k, hex(p ? p->hex : 0xFFFFFF));
        ui::textBoite(r, g.calques[l].x + 34 * k, zy, g.calques[l].w - 44 * k, lh, 14 * k, std::to_string(i + 1) + ". " + nomMotif(ca.c.motif),
                      hex(ca.masque ? MUTED : TEXTE), ui::GAUCHE);
        bouton(r, g.yeux[l], ui::tr(ca.masque ? "atelier.afficher" : "atelier.masquer"), OEIL0 + l, 12 * k);
    }
    if (n == 0) ui::textBoite(r, x + 16 * k, ly, w - 32 * k, 3 * lh, 13 * k, ui::tr("atelier.vide"), hex(MUTED), ui::GAUCHE);
    if (g.premier + LIGNES < n) { g.listeBas = {x + w / 2 - 30 * k, ly + LIGNES * (lh + 4 * k), 60 * k, 26 * k}; bouton(r, g.listeBas, "", LISTE_BAS, 13 * k); chevron(r, g.listeBas, false); }
    // monter / descendre (RG-04)
    const float by = y + h - 44 * k, bw = (w - 30 * k) / 2;
    g.monter = {x + 10 * k, by, bw, 34 * k}; g.descendre = {x + 20 * k + bw, by, bw, 34 * k};
    const int c = g.e.choisi();
    bouton(r, g.monter, ui::tr("atelier.monter"), MONTER, 13 * k, c >= 0 && c + 1 < n);
    bouton(r, g.descendre, ui::tr("atelier.descendre"), DESCENDRE, 13 * k, c > 0);
}

void dessinerOutils(Renderer& r, float x0, float x1, float y) {
    const float k = g.k, h = 40 * k, gap = 6 * k;
    const char* const CLES[NB_OUTILS] = {"atelier.rotation.moins", "atelier.rotation.plus", "atelier.taille.moins", "atelier.taille.plus",
                                         "atelier.miroir", "atelier.couleur", "atelier.supprimer"};
    const float poids[NB_OUTILS] = {1, 1, 1, 1, 1.25f, 1.25f, 1.35f};
    float total = 0; for (float p : poids) total += p;
    const float u = (x1 - x0 - gap * (NB_OUTILS - 1)) / total;
    const bool actif = g.e.calqueChoisi() != nullptr;
    float x = x0;
    for (int i = 0; i < NB_OUTILS; i++) {
        g.outils[i] = {x, y, u * poids[i], h};
        bouton(r, g.outils[i], ui::tr(CLES[i]), ROT_MOINS + i, 13 * k, actif);
        x += u * poids[i] + gap;
    }
    // valeurs du calque choisi, au-dessus des outils
    if (const CalqueAtelier* ca = g.e.calqueChoisi()) {
        const std::string v = ui::tr("atelier.valeurs", std::to_string(ca->c.rotation), std::to_string(ca->c.echelle));
        ui::text(r, (x0 + x1) / 2, y - 22 * k, 13 * k, v, hex(ACCENT2), ui::CENTRE);
    } else ui::text(r, (x0 + x1) / 2, y - 22 * k, 13 * k, ui::tr("atelier.astuce"), hex(MUTED), ui::CENTRE);
}

void dessinerPalette(Renderer& r) {
    const float k = g.k, s = 30 * k, gap = 6 * k;
    const int col = 14;
    const float w = col * s + (col - 1) * gap + 24 * k, h = 2 * s + gap + 48 * k;
    const float x = (g.W - w) / 2, y = g.outils[0].y - 34 * k - h;
    g.fondPalette = {x, y, w, h};
    verre::panneau(r, x, y, w, h, {12 * k});
    verre::surTitre(r, x + 12 * k, y + 10 * k, 11 * k, ui::tr("atelier.couleur"));
    const CalqueAtelier* ca = g.e.calqueChoisi();
    for (int i = 0; i < NB_PEINTURES; i++) {
        const float zx = x + 12 * k + (i % col) * (s + gap), zy = y + 32 * k + (i / col) * (s + gap);
        g.palette28[i] = {zx, zy, s, s};
        const bool choisie = ca && ca->c.couleur == PEINTURES[i].id;
        if (choisie) r.uiRect(zx - 3 * k, zy - 3 * k, s + 6 * k, s + 6 * k, hex(ACCENT2));
        r.uiRect(zx, zy, s, s, hex(PEINTURES[i].hex));
    }
}

void dessinerConfirmation(Renderer& r) {
    const float k = g.k, w = 520 * k, h = 200 * k, x = (g.W - w) / 2, y = (g.H - h) / 2;
    verre::assombrir(r, g.W, g.H, 0.45f);
    verre::panneau(r, x, y, w, h, {16 * k});
    ui::text(r, x + w / 2, y + 26 * k, 24 * k, ui::tr("atelier.confirmer.titre"), hex(TEXTE), ui::CENTRE);
    ui::textBoite(r, x + 24 * k, y + 62 * k, w - 48 * k, 40 * k, 15 * k, ui::tr("atelier.confirmer.texte"), hex(MUTED), ui::CENTRE);
    const float bw = 200 * k, bh = 50 * k, by = y + h - bh - 22 * k;
    g.confQuitter = {x + 30 * k, by, bw, bh}; g.confRester = {x + w - 30 * k - bw, by, bw, bh};
    ui::boutons::autocollant(r, g.confQuitter.x, by, bw, bh, ui::majuscules(ui::tr("atelier.quitter")), 1, g.cible == CONF_QUITTER, 17 * k);
    ui::boutons::startStop(r, g.confRester.x, by, bw, bh, ui::majuscules(ui::tr("atelier.rester")), 1, g.cible == CONF_RESTER);
}

void dessiner(Renderer& r, const GameView& v) {
    const textes::Journal journal;
    const float k = g.k = v.uiHeight / 720.f, W = g.W = v.uiWidth, H = g.H = v.uiHeight;
    g.dp = ui::pixelsParDp(W);
    const Marges& m = v.marges;
    const float gl = m.gauche + 16 * k, gr = W - m.droite - 16 * k, gt = m.haut + 14 * k, gb = H - m.bas - 14 * k;
    // haut : titre, vues, annuler / refaire
    verre::surTitre(r, gl, gt, 12 * k, ui::tr("atelier.surtitre"));
    ui::text(r, gl, gt + 18 * k, 26 * k, ui::majuscules(ui::tr("atelier.titre")), hex(TEXTE), ui::GAUCHE, 0.06f);
    const float vw = 96 * k, vh = 34 * k, vx = W / 2 - NB_VUES * vw / 2, vy = gt + 4 * k;
    ui::boutons::selecteurFond(r, vx, vy, NB_VUES * vw, vh);
    for (int i = 0; i < NB_VUES; i++) {
        g.vues[i] = {vx + i * vw, vy, vw, vh};
        ui::boutons::selecteurChoix(r, g.vues[i].x, vy, vw, vh, ui::tr(CLES_VUES[i]), g.vue == i, 1, g.cible == VUE0 + i, 13 * k);
    }
    const float aw = 112 * k;
    g.refaire = {gr - aw, vy, aw, vh}; g.annuler = {gr - 2 * aw - 8 * k, vy, aw, vh};
    bouton(r, g.annuler, ui::tr("atelier.annuler"), ANNULER, 13 * k, g.e.pasAnnuler() > 0);
    bouton(r, g.refaire, ui::tr("atelier.refaire"), REFAIRE, 13 * k, g.e.pasRefaire() > 0);
    // panneaux de côté
    const float py = gt + 60 * k, ph = gb - 66 * k - py;
    dessinerBibliotheque(r, gl, py, 300 * k, ph);
    dessinerCalques(r, gr - 330 * k, py, 330 * k, ph);
    // contour de l'autocollant choisi (suit le doigt pendant un geste)
    if (const CalqueAtelier* ca = g.e.calqueChoisi(); ca && !ca->masque && g.vpOk && !g.confirmation)
        for (const auto& c : contourEcran(ca->c)) {
            std::vector<vec2> ferme = c; ferme.push_back(c.front());
            for (size_t i = 0; i + 1 < ferme.size(); i += 63) {
                const int nb = (int)std::min<size_t>(64, ferme.size() - i);
                dessin2d::trait(r, ferme.data() + i, nb, 4.5f * k, hex(0x05060A, 0.7f));
                dessin2d::trait(r, ferme.data() + i, nb, 2 * k, hex(ACCENT2));
            }
        }
    // bas : Quitter, outils, Valider
    const float bh = 50 * k, by = gb - bh, qw = 170 * k;
    g.quitter = {gl, by, qw, bh}; g.valider = {gr - 200 * k, by, 200 * k, bh};
    ui::boutons::autocollant(r, gl, by, qw, bh, ui::majuscules(ui::tr("atelier.quitter")), 1, g.cible == QUITTER, 17 * k);
    ui::boutons::startStop(r, g.valider.x, by, g.valider.w, bh, ui::majuscules(ui::tr("atelier.valider")), 1, g.cible == VALIDER);
    dessinerOutils(r, gl + qw + 24 * k, g.valider.x - 24 * k, by + 5 * k);
    if (g.palette) dessinerPalette(r); else g.fondPalette = Zone{};
    // message (RG-04 : « 20 calques au maximum »)
    if (!g.message.empty() && g.maintenant - g.tMessage < DUREE_MESSAGE) {
        const float tw = ui::width(16 * k, g.message) + 48 * k, tx = (W - tw) / 2, ty = gt + 52 * k;
        verre::panneau(r, tx, ty, tw, 40 * k, {10 * k, 1.5f, 1, true, ACCENT});
        ui::text(r, W / 2, ty + 11 * k, 16 * k, g.message, hex(TEXTE), ui::CENTRE);
    } else if (g.maintenant - g.tMessage >= DUREE_MESSAGE) g.message.clear();
    if (g.confirmation) dessinerConfirmation(r); else g.confQuitter = g.confRester = Zone{};
    // manette et touche Retour d'Android (WFS-116 : la zone « retour » reçoit l'appui) : « Quitter » (confirmation si changé) ;
    // dans la confirmation : « Rester »
    if (g.confirmation) { manette::focus(g.confQuitter); manette::focus(g.confRester, true); return; }
    for (const Zone& z : g.vues) manette::focus(z);
    for (const Zone& z : g.familles) manette::focus(z);
    for (const Zone& z : g.motifs) manette::focus(z);
    for (const Zone& z : g.calques) manette::focus(z);
    for (const Zone& z : g.outils) manette::focus(z);
    if (g.palette) for (const Zone& z : g.palette28) manette::focus(z);
    for (const Zone* z : {&g.annuler, &g.refaire, &g.monter, &g.descendre, &g.valider}) manette::focus(*z);
    manette::focus(g.quitter, true);
}

// ---------------------------------------------------------------- touchers
int cibleSous(float x, float y) {
    auto dans = [&](const Zone& z) { return z.w > 0 && appui::dansCible(z.x, z.y, z.w, z.h, g.dp, x, y); };
    if (g.confirmation) return dans(g.confQuitter) ? CONF_QUITTER : dans(g.confRester) ? CONF_RESTER : FOND_PALETTE;   // tout le reste est couvert
    if (g.palette) {
        for (int i = 0; i < NB_PEINTURES; i++) if (g.palette28[i].contient(x, y)) return PALETTE0 + i;
        if (g.fondPalette.contient(x, y)) return FOND_PALETTE;
    }
    for (int i = 0; i < NB_VUES; i++) if (g.vues[i].contient(x, y)) return VUE0 + i;
    for (int i = 0; i < NB_FAMILLES; i++) if (g.familles[i].contient(x, y)) return FAMILLE0 + i;
    for (int i = 0; i < MOTIFS_PAGE; i++) if (g.motifs[i].contient(x, y)) return MOTIF0 + i;
    for (int i = 0; i < LIGNES; i++) { if (g.yeux[i].contient(x, y)) return OEIL0 + i; if (g.calques[i].contient(x, y)) return CALQUE0 + i; }
    for (int i = 0; i < NB_OUTILS; i++) if (dans(g.outils[i])) return ROT_MOINS + i;
    const std::pair<const Zone*, int> autres[] = {{&g.pagePrec, PAGE_PREC}, {&g.pageSuiv, PAGE_SUIV}, {&g.listeHaut, LISTE_HAUT}, {&g.listeBas, LISTE_BAS},
        {&g.monter, MONTER}, {&g.descendre, DESCENDRE}, {&g.annuler, ANNULER}, {&g.refaire, REFAIRE}, {&g.valider, VALIDER}, {&g.quitter, QUITTER}};
    for (const auto& [z, c] : autres) if (dans(*z)) return c;
    if (g.panneauG.contient(x, y) || g.panneauD.contient(x, y)) return FOND_PALETTE;   // fond des panneaux : rien
    return RIEN;
}

void choisirVueInterne(int v) {
    if (v < 0 || v >= NB_VUES) return;
    g.vue = v; g.theta = THETA[v]; g.phi = PHI[v];
    WFS_LOG("atelier : vue %s", ui::tr(CLES_VUES[v]).c_str());
}

void valider() {
    const std::string t = g.e.texteGarde();
    garder(g.car, t);
    WFS_LOG("atelier : « Valider » %s : %d calque(s), cuisson pour le garage et la course", g.car.c_str(), (int)g.e.calques().size());
    g.ouvert = false;   // retour au garage : l'aperçu reprend l'habillage gardé (apercu, calquesCourse)
}

void agir(int c) {
    const auto rangCalque = [](int ligne) { const int n = (int)g.e.calques().size(); return n - 1 - (g.premier + ligne); };
    if (c >= VUE0 && c < VUE0 + NB_VUES) { choisirVueInterne(c - VUE0); return; }
    if (c >= FAMILLE0 && c < FAMILLE0 + NB_FAMILLES) { g.famille = c - FAMILLE0; g.page = 0; return; }
    if (c >= MOTIF0 && c < MOTIF0 + MOTIFS_PAGE) {
        const std::string id = g.motifTuile[c - MOTIF0];
        if (id.empty()) return;
        if (!motifPermis(id)) {   // RG-02 : cadenas
            const char* pack = packDuMotif(id);
            montrerMessage(ui::tr("atelier.verrou", ui::tr((std::string("atelier.pack.") + (pack ? pack : "")).c_str())));
            return;
        }
        if ((int)g.e.calques().size() >= MAX_CALQUES) { montrerMessage(ui::tr("atelier.max")); return; }   // RG-04
        g.e.ajouter(id, directionVue());
        WFS_LOG("atelier : calque %s ajouté (%s), %d calque(s)", id.c_str(), va::nomDirection(directionVue()), (int)g.e.calques().size());
        return;
    }
    if (c >= CALQUE0 && c < CALQUE0 + LIGNES) { g.e.choisir(rangCalque(c - CALQUE0)); return; }
    if (c >= OEIL0 && c < OEIL0 + LIGNES) { g.e.choisir(rangCalque(c - OEIL0)); g.e.masquer(); return; }
    if (c >= PALETTE0 && c < PALETTE0 + NB_PEINTURES) { g.e.colorer(PEINTURES[c - PALETTE0].id); g.palette = false; return; }
    switch (c) {
    case PAGE_PREC: g.page = std::max(0, g.page - 1); break;
    case PAGE_SUIV: g.page++; break;
    case LISTE_HAUT: g.premier = std::max(0, g.premier - 1); break;
    case LISTE_BAS: g.premier++; break;
    case MONTER: g.e.monter(); break;
    case DESCENDRE: g.e.descendre(); break;
    case ROT_MOINS: g.e.tourner(-PAS_ROTATION); break;
    case ROT_PLUS: g.e.tourner(PAS_ROTATION); break;
    case ECH_MOINS: g.e.agrandir(-PAS_ECHELLE); break;
    case ECH_PLUS: g.e.agrandir(PAS_ECHELLE); break;
    case MIROIR: g.e.miroir(); break;
    case COULEUR: if (g.e.calqueChoisi()) g.palette = !g.palette; break;
    case SUPPRIMER: g.e.supprimer(); break;
    case ANNULER: g.e.annuler(); break;
    case REFAIRE: g.e.refaire(); break;
    case VALIDER: valider(); break;
    case QUITTER: if (g.e.modifie()) g.confirmation = true; else { WFS_LOG("atelier : « Quitter » (rien de changé)"); g.ouvert = false; } break;
    case CONF_QUITTER: WFS_LOG("atelier : « Quitter » sans valider : changements oubliés"); g.confirmation = false; g.ouvert = false; break;
    case CONF_RESTER: g.confirmation = false; break;
    default: break;
    }
}

float angle(const Doigt& a, const Doigt& b) { return std::atan2(-(b.y - a.y), b.x - a.x); }   // sens trigonométrique à l'écran
float ecart(const Doigt& a, const Doigt& b) { return std::hypot(b.x - a.x, b.y - a.y); }

} // namespace

// ---------------------------------------------------------------- interface publique
void ouvrir(const char* carId, int skin) {
    g.ouvert = true; g.car = carId ? carId : ""; g.skin = skin;
    g.e.charger(texteGarde(g.car));
    g.cuit = g.e.texteCuisson();
    g.famille = 0; g.page = 0; g.premier = 0; g.palette = g.confirmation = false; g.message.clear();
    g.nDoigts = 0; g.mode = M_AUCUN; g.cible = RIEN; g.doigtCible = -1;
    choisirVueInterne(V_GAUCHE);
    WFS_LOG("atelier : ouvert sur %s (%d calque(s) gardé(s)) (WFS-496)", g.car.c_str(), (int)g.e.calques().size());
}
void fermer() { if (g.ouvert) WFS_LOG("atelier : fermé"); g.ouvert = false; g.palette = g.confirmation = false; g.nDoigts = 0; g.mode = M_AUCUN; }
bool ouvert() { return g.ouvert; }

void camera(FrameParams& fp, SceneDediee& s, const GameView& v) {
    if (!g.ouvert) return;
    const float L = g.bmax.z - g.bmin.z, Wd = g.bmax.x - g.bmin.x, Hc = std::max(0.5f, g.bmax.y);
    const float aspect = v.uiWidth / std::max(1.f, v.uiHeight);
    const float tv = std::tan(18.f * PI / 180.f);   // demi-angle vertical (36° : décor du garage)
    // distance qui garde la voiture entre les deux panneaux (≈ 40 % de la largeur) et sous la barre du haut
    const float dCote = std::max(L / (0.80f * tv * aspect), Hc / (0.62f * tv));
    const float dFace = std::max(Wd / (0.80f * tv * aspect) * 1.25f, Hc / (0.62f * tv));
    const float f = std::fabs(std::sin(g.theta)), haut = std::clamp((g.phi - 0.5f) / 0.9f, 0.f, 1.f);
    const float d = lerpf(lerpf(dFace, dCote, f), std::max(L / (0.8f * tv * aspect), Wd / (0.62f * tv)), haut);
    const vec3 c{0, VOITURE_Y + 0.45f * Hc * (1 - haut), 0};
    const vec3 eye = c + vec3{std::sin(g.theta) * std::cos(g.phi), std::sin(g.phi), std::cos(g.theta) * std::cos(g.phi)} * d;
    fp.view = lookAt(eye, c, {0, 1, 0});
    fp.camPos = eye;
    s.cible = c;
    g.oeil = eye;
}

bool apercu(Renderer& r, const Model& m, const char* carId, int skin) {
    if (!m.ok || !carId) return false;
    if (g.ouvert && g.car == carId) { g.bmin = m.bmin; g.bmax = m.bmax; }
    const char* skinId = skin >= 0 && skin < BOUTIQUE_SKINS ? BOUTIQUE_SKINS_IDS[skin] : "";
    std::string texte;
    if (g.ouvert && g.car == carId) {
        if (g.mode != M_PLACER && g.mode != M_DEUX) g.cuit = g.e.texteCuisson();   // pendant un geste : la cuisson attend le doigt levé
        texte = g.cuit;
    } else {
        const std::string garde = texteGarde(carId);
        if (garde != g.garde) { g.garde = garde; g.gardeCuisson = calquesCourse(carId); }
        texte = g.gardeCuisson;
    }
    bool surCeModele = false;
    for (uint32_t i = 0; i < m.mats.size() && !surCeModele; i++) surCeModele = vehicule_skin::materiau(m, i) != nullptr;
    const vehicule_skin::Etat e = vehicule_skin::etat();
    if (texte.empty()) {   // rien à montrer : une cuisson restée sur ce modèle est rendue, le garage montre sa skin
        if (surCeModele && e.skin == vehicule_skin::CUIT) { r.vk().waitIdle(); vehicule_skin::liberer(r); }
        g.poseCle.clear();
        return false;
    }
    const std::string cle = std::string(carId) + "|" + skinId + "|" + texte;
    if (g.poseCle == cle && (g.poseEchec || (surCeModele && e.skin == vehicule_skin::CUIT && e.voiture == carId))) return true;
    r.vk().waitIdle();   // l'ancienne texture peut être lue par une image en vol
    g.poseEchec = !vehicule_skin::poser(r, m, carId, skinId, texte);
    g.poseCle = cle;
    return true;
}

bool toucher(const Touch& t, const GameView& v) {
    if (!g.ouvert) return false;
    g.k = v.uiHeight / 720.f;
    int i = -1;
    for (int j = 0; j < g.nDoigts; j++) if (g.doigts[j].id == t.id) i = j;
    if (t.id == g.doigtCible) {   // bouton : action au relâcher, sur la cible appuyée
        if (t.down) return true;
        const int c = cibleSous(t.x, t.y), b = g.cible;
        g.cible = RIEN; g.doigtCible = -1;
        if (c == b) agir(c);
        return true;
    }
    if (t.down && i < 0) {   // nouveau doigt
        if (g.nDoigts == 0) {
            const int c = cibleSous(t.x, t.y);
            if (c == FOND_PALETTE) { if (!g.confirmation && !g.fondPalette.contient(t.x, t.y)) g.palette = false; return true; }
            if (c != RIEN) { g.cible = c; g.doigtCible = t.id; return true; }
            if (g.palette) { g.palette = false; return true; }
            g.doigts[0] = {t.id, t.x, t.y}; g.nDoigts = 1;
            float a, b;
            const CalqueAtelier* ca = g.e.calqueChoisi();
            if (ca && surCalque(t.x, t.y) && versPlan(ca->c.dir, t.x, t.y, a, b)) {   // RG-03 : glisser = placer
                g.mode = M_PLACER; g.e.debutGeste();
                g.offA = ca->c.x / 100.f - a; g.offB = ca->c.y / 100.f - b;
            } else {   // RG-01 : rotation au doigt autour de la voiture
                g.mode = M_CAMERA; g.x0 = t.x; g.y0 = t.y; g.theta0 = g.theta; g.phi0 = g.phi;
            }
            return true;
        }
        if (g.nDoigts == 1 && g.mode != M_FINI) {   // 2e doigt : tourner et agrandir le calque choisi (RG-03)
            g.doigts[1] = {t.id, t.x, t.y}; g.nDoigts = 2;
            const CalqueAtelier* ca = g.e.calqueChoisi();
            if (!ca) { g.mode = M_FINI; return true; }
            g.e.debutGeste();
            g.mode = M_DEUX; g.d0 = std::max(1.f, ecart(g.doigts[0], g.doigts[1])); g.a0 = angle(g.doigts[0], g.doigts[1]);
            g.rot0 = ca->c.rotation; g.ech0 = ca->c.echelle;
        }
        return true;
    }
    if (i < 0) return true;
    if (t.down) {   // mouvement
        g.doigts[i].x = t.x; g.doigts[i].y = t.y;
        if (g.mode == M_PLACER) {
            float a, b;
            if (const CalqueAtelier* ca = g.e.calqueChoisi(); ca && versPlan(ca->c.dir, t.x, t.y, a, b))
                g.e.placer((int)std::lround((a + g.offA) * 100), (int)std::lround((b + g.offB) * 100));
        } else if (g.mode == M_CAMERA) {
            const float u = 720.f / std::max(1.f, v.uiHeight);
            g.theta = g.theta0 - (t.x - g.x0) * 0.01f * u;
            g.phi = std::clamp(g.phi0 + (t.y - g.y0) * 0.006f * u, PHI_MIN, PHI_MAX);
            if (std::fabs(t.x - g.x0) + std::fabs(t.y - g.y0) > 6 * g.k) g.vue = -1;   // vue libre
        } else if (g.mode == M_DEUX && g.nDoigts == 2) {
            const float da = (angle(g.doigts[0], g.doigts[1]) - g.a0) * 180.f / PI;
            g.e.transformer(g.rot0 + (int)std::lround(da), (int)std::lround(g.ech0 * ecart(g.doigts[0], g.doigts[1]) / g.d0));
        }
        return true;
    }
    // doigt levé
    if (g.mode == M_PLACER || g.mode == M_DEUX) g.e.finGeste();
    if (g.mode == M_DEUX || g.mode == M_FINI) g.mode = g.nDoigts > 1 ? M_FINI : M_AUCUN;   // l'autre doigt ne fait plus rien jusqu'au relâcher
    else g.mode = M_AUCUN;
    g.doigts[i] = g.doigts[g.nDoigts - 1]; g.nDoigts--;
    if (g.nDoigts == 0) g.mode = M_AUCUN;
    return true;
}

void dessinerBoutonGarage(Renderer& r, float W, float H, const Marges& m, float op) {
    if (!ui::fontReady() || g.ouvert) { g.bouton = Zone{}; return; }
    const textes::Journal journal;
    const float k = H / 720.f;
    // sous « MARKET » (market_ecran.cpp, dessinerBoutonGarage : y = haut + 46 × k, hauteur 40 × k), même taille et même style
    const float h = 40 * k, tp = 14 * k, pad = 16 * k;
    const std::string t = ui::majuscules(ui::tr("atelier.bouton"));
    const float tw = ui::width(tp, t, 0.12f);
    const float w = pad + tw + pad, x = W - m.droite - 16 * k - w, y = m.haut + 14 * k + 6 * k + 14 * k + 12 * k + h + 8 * k;
    g.bouton = {x, y, w, h};
    g.dpBouton = ui::pixelsParDp(W);
    auto a = [op](vec4 c) { c.w *= op; return c; };
    dessin2d::biseau(r, x - k, y - k, w + 2 * k, h + 2 * k, 8 * k, dessin2d::uni(a(vec4{1, 1, 1, 0.16f})));
    dessin2d::biseau(r, x, y, w, h, 8 * k, dessin2d::uni(a(vec4{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f})));
    ui::textBoite(r, x + w / 2, y, tw, h, tp, t, a(hex(ACCENT)), ui::CENTRE, 0.12f, false);
}
bool dansBoutonGarage(float x, float y) {
    return !g.ouvert && g.bouton.w > 0 && appui::dansCible(g.bouton.x, g.bouton.y, g.bouton.w, g.bouton.h, g.dpBouton, x, y);
}

// ---------------------------------------------------------------- banc
bool centre(const char* nom, float& x, float& y) {
    const std::string n = nom;
    const Zone* z = n == "bouton" ? &g.bouton : n == "valider" ? &g.valider : n == "quitter" ? &g.quitter : n == "conf-quitter" ? &g.confQuitter
                  : n == "conf-rester" ? &g.confRester : n == "annuler" ? &g.annuler : n == "refaire" ? &g.refaire : n == "monter" ? &g.monter
                  : n == "descendre" ? &g.descendre : n == "page-suiv" ? &g.pageSuiv : nullptr;
    auto indice = [&](const char* p) { return n.rfind(p, 0) == 0 ? std::atoi(n.c_str() + std::strlen(p)) : -1; };
    if (!z) {
        if (int i = indice("motif-"); i >= 0 && i < MOTIFS_PAGE) z = &g.motifs[i];
        else if (int i = indice("famille-"); i >= 0 && i < NB_FAMILLES) z = &g.familles[i];
        else if (int i = indice("vue-"); i >= 0 && i < NB_VUES) z = &g.vues[i];
        else if (int i = indice("outil-"); i >= 0 && i < NB_OUTILS) z = &g.outils[i];
        else if (int i = indice("couleur-"); i >= 0 && i < NB_PEINTURES) z = &g.palette28[i];
        else if (int i = indice("ligne-"); i >= 0 && i < LIGNES) z = &g.calques[i];
        else if (int i = indice("oeil-"); i >= 0 && i < LIGNES) z = &g.yeux[i];
    }
    if (!z || z->w <= 0) return false;
    x = z->x + z->w / 2; y = z->y + z->h / 2;
    return true;
}
const std::string& message() { return g.message; }
std::string texteAtelier() { return g.e.texteGarde(); }
void choisirVue(int v) { choisirVueInterne(v); }
bool versEcran(float a, float b, float& x, float& y) {
    const CalqueAtelier* ca = g.e.calqueChoisi();
    return ca && projeter(pointPlan(ca->c.dir, a / 100.f, b / 100.f), x, y);
}

namespace {
// système : panneau par-dessus le garage (440), sous le Market (469) ; touchers reçus par le garage (accroche)
class AtelierEcran : public System {
public:
    const char* name() const override { return "atelier"; }
    void onEcran(Ecran ancien, Ecran, const GameView&) override { if (ancien == Ecran::Garage) fermer(); }
    void drawUi(Renderer& r, const GameView& v) override {
        g.maintenant = v.time;
        if (v.ecran != Ecran::Garage || !g.ouvert || !ui::fontReady()) { g.vpOk = false; return; }
        g.vp = v.proj * v.view; g.vpOk = true;
        dessiner(r, v);
    }
    void shutdown(Renderer&) override { fermer(); g.poseCle.clear(); g.garde.clear(); g.gardeCuisson.clear(); }
};
} // namespace

} // namespace ecrans::atelier
} // namespace wfs

WFS_SYSTEM(wfs::ecrans::atelier::AtelierEcran, 467)   // interface : par-dessus le garage (440), sous le Profil (468) et le Market (469)
