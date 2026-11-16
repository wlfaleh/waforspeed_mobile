// WaForSpeed — choix du terrain et du moment (WFS-41) : voir terrain.h.
#include "terrain.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "../../core/log.h"
#include "../../core/system.h"
#include "../../game/course_system.h"
#include "../../game/game.h"
#include "../../game/sauvegarde.h"   // WFS-24 : moment gardé
#include "../../render/atmosphere_physique.h"   // WFS-208 : moments de la journée
#include "../../render/eclairage_physique.h"     // WFS-208 : eclairage::choisirMoment
#include "../../game/track.h"
#include "../../render/renderer.h"
#include "../text.h"
#include "../boutons.h"   // WFS-549 : boutons au modèle R19
#include "../langue.h"   // WFS-505 : textes du choix du terrain en français ou en anglais
#include "textes_banc.h"   // WFS-505 : banc « textes » (journal des textes de cet écran)
#include "../petit_ecran.h"   // WFS-147 : téléphone couché (dp)
#include "../../game/commandes_tactiles.h"   // WFS-146 : dessin2d::biseau (un seul polygone)
#include "reglages.h"
#include "../../platform/manette.h"   // WFS-22
#include "../../audio/sfx.h"          // WFS-96 : son du changement de carte
#include "../../world/zones.h"        // WFS-96 : noms des quartiers
#include "../../world/cartes_v2/paquet_carte.h"   // WFS-307 : empreinte attendue de chaque carte
#include "paquets.h"                  // WFS-307 : paquets des cartes (télécharger, supprimer)
#include "../../game/boutique.h"   // WFS-557 : solde d'étoiles (maquette 18)
#include "../garage_achat.h"      // WFS-557 : étoile dessinée (garage::etoile)
#include "../../world/cartes_v2/carte_donnees.h"   // WFS-343 : cartes lues des paquets

namespace wfs {
namespace ecrans {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
vec4 melange(const vec4& a, const vec4& b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t}; }
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SURFACE2 = 0x1E2130;
const vec4 BORD{1, 1, 1, 0.10f};

// js/world/maps.js : ordre de MAPS, textes et couleurs des illustrations (art.ground [nuit, jour], art.accent)
// WFS-505 (RG-01) : desc = clé de la description dans les tables de textes (assets/textes/fr.txt, en.txt)
struct Carte { const char* id; const char* desc; unsigned solNuit, solJour, accent; };
const Carte CARTES[4] = {
    {"ville", "terrain.desc.ville", 0x14151C, 0x55565C, 0x00E5FF},
    {"plage", "terrain.desc.plage", 0x2A2A22, 0xE6D3A1, 0x1D8FB8},
    {"campagne", "terrain.desc.campagne", 0x18200F, 0x8FAE4C, 0xCDB259},
    {"circuit", "terrain.desc.circuit", 0x14200F, 0x4F7F32, 0xE2262B},
};
// WFS-505 (RG-01) : noms des quartiers (src/world/zones.cpp, en français) → clé de leur traduction ; les noms des cartes, eux,
// restent tels quels (noms propres, RC-E20-03). Quartier inconnu (carte de données) : affiché tel quel.
const char* const QUARTIERS[][2] = {
    {"Centre-ville", "quartier.centre_ville"}, {"Quartier résidentiel", "quartier.residentiel"}, {"Parc urbain", "quartier.parc"},
    {"Zone portuaire", "quartier.port"}, {"Quartier d'affaires", "quartier.affaires"}, {"Vieux quartier", "quartier.vieux"},
    {"Corniche", "quartier.corniche"}, {"Port de plaisance", "quartier.marina"}, {"Côte rocheuse", "quartier.rochers"},
    {"Village de pêcheurs", "quartier.pecheurs"}, {"Champs", "quartier.champs"}, {"Forêt", "quartier.foret"},
    {"Village", "quartier.village"}, {"Vignobles", "quartier.vignes"}, {"Plaine", "quartier.plaine"}, {"Bois de pins", "quartier.pins"},
    {"Ligne droite des tribunes", "quartier.tribunes"}, {"Paddock", "quartier.paddock"}, {"Virages rapides", "quartier.virages"},
    {"Section boisée", "quartier.bois"},
};
std::string nomQuartier(const std::string& fr) {
    for (const auto& q : QUARTIERS) if (fr == q[0]) return ui::tr(q[1]);
    return fr;
}

// WFS-343 (RG-03) : cartes montrées : les 4 du code, puis les cartes de données enregistrées (cartes_v2::decouvrir) ; art : dessin de
// la vignette (0 à 3 : cartes du code ; 4 : vignette générique aux couleurs de la carte). Cartes de test cachées : WFS_CARTES_TEST=1.
struct CarteVue { std::string id, desc; unsigned solNuit, solJour, accent; int art; };
std::vector<CarteVue> g_liste;
void listeCartes() {
    g_liste.clear();
    for (int i = 0; i < 4; i++) g_liste.push_back({CARTES[i].id, CARTES[i].desc, CARTES[i].solNuit, CARTES[i].solJour, CARTES[i].accent, i});
    for (const std::string& id : cartes_v2::cartesDonnees(std::getenv("WFS_CARTES_TEST") != nullptr))
        if (const auto d = cartes_v2::donnees(id)) g_liste.push_back({id, d->description, d->vignetteNuit, d->vignetteJour, d->accent, 4});   // description du paquet (cartes de test, cachées)
}
int nbCartes() { if (g_liste.empty()) listeCartes(); return (int)g_liste.size(); }

struct Zone {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};
Zone g_carte, g_prec, g_suiv, g_jour, g_nuit, g_retour, g_rouler;
int g_index = 0;          // carte choisie (MAPS)
bool g_nuitChoisie = true; // TOD du web : nuit par défaut
// WFS-208 : moment de la course (WFS-375 : seulement midi pour « Jour », nuit pour « Nuit »)
using atmosphere::Moment;
Moment g_moment = Moment::Nuit;
// WFS-375 (RG-01, RG-02, ordre de l'utilisateur) : deux modes seulement, JOUR = midi (soleil haut, quelle que soit l'heure du
// téléphone) et NUIT = nuit ; plus de défilement des moments de WFS-208 (aube, dorée, réelle… restent pour les bancs de lumière)
Moment modeSimple(bool nuit) { return nuit ? Moment::Nuit : Moment::Midi; }

// WFS-147 (ANO-42) : formes d'un seul tenant (plus de bandes), reprises de WFS-102 (1ddd3b6) : dégradé vertical,
// polygone coupé à [x0, x1], ellipse
void degradeV(Renderer& r, float x, float y, float w, float h, const vec4& a, const vec4& b) {
    const UiVertex q[4] = {{x, y, -1, -1, a.x, a.y, a.z, a.w}, {x + w, y, -1, -1, a.x, a.y, a.z, a.w},
                           {x + w, y + h, -1, -1, b.x, b.y, b.z, b.w}, {x, y + h, -1, -1, b.x, b.y, b.z, b.w}};
    r.uiQuad(q);
}
// polygone convexe (sens horaire) coupé aux verticales x0 et x1 (Sutherland-Hodgman), teinte verticale
void polygoneCoupe(Renderer& r, std::vector<vec2> p, float x0, float x1, const dessin2d::Teinte& t) {
    for (int cote = 0; cote < 2; cote++) {
        std::vector<vec2> o;
        auto dedans = [&](const vec2& v) { return cote == 0 ? v.x >= x0 : v.x <= x1; };
        const float xc = cote == 0 ? x0 : x1;
        for (size_t i = 0; i < p.size(); i++) {
            const vec2 a = p[i], b = p[(i + 1) % p.size()];
            const bool da = dedans(a), db = dedans(b);
            if (da) o.push_back(a);
            if (da != db) { const float f = (xc - a.x) / (b.x - a.x); o.push_back({xc, a.y + (b.y - a.y) * f}); }
        }
        p = o;
        if (p.size() < 3) return;
    }
    dessin2d::polygone(r, p.data(), (int)std::min<size_t>(p.size(), 64), t);
}
void ellipse(Renderer& r, float cx, float cy, float rx, float ry, const vec4& c) {
    vec2 p[32];
    for (int i = 0; i < 32; i++) { const float a = 6.2831853f * i / 32; p[i] = {cx + rx * std::cos(a), cy + ry * std::sin(a)}; }
    dessin2d::polygone(r, p, 32, dessin2d::uni(c));
}

// ---------------------------------------------------------------- vignette (mapArt du web, en rectangles et disques)
// toutes les formes restent dans le cadre (x, y, w, h) ; repère du web : 400 x 250, horizon à 150
void vignette(Renderer& r, float X, float Y, float W, float H, const CarteVue& c, bool nuit) {
    const int idx = c.art;
    const MapDef& m = mapById(c.id.c_str());
    Mulberry32 R(m.seed * 7 + (nuit ? 3 : 1));
    auto rnd = [&]() { return (float)R.next(); };
    const float sx = W / 400.f, sy = H / 250.f, HZ = Y + 150 * sy;
    const float sr = std::min(sx, sy);   // WFS-147 : rayon de la lune et du soleil (halo gardé dans la vignette élargie)
    auto px = [&](float v) { return X + v * sx; };
    auto py = [&](float v) { return Y + v * sy; };
    // ciel : dégradé à 3 arrêts (0, 62 %, 100 %, screens.js l. 209) en deux quadrilatères (WFS-147, ANO-42 : plus de bandes)
    unsigned ciel[3] = {nuit ? 0x05050Cu : 0x2F6CC0u, nuit ? 0x1A0F2Eu : 0x79B1E2u, nuit ? 0x43203Fu : 0xDCEBF5u};
    if (!nuit && idx == 0) ciel[2] = 0xE6E3DF;
    if (nuit && idx == 2) ciel[2] = 0x1D2640;
    if (nuit && idx == 1) ciel[2] = 0x3A2448;
    const float y62 = Y + (HZ - Y) * 0.62f;
    degradeV(r, X, Y, W, y62 - Y, hex(ciel[0]), hex(ciel[1]));
    degradeV(r, X, y62, W, HZ - y62, hex(ciel[1]), hex(ciel[2]));
    auto cielEn = [&](float yy) { const float t = (yy - Y) / (HZ - Y); return t < 0.62f ? melange(hex(ciel[0]), hex(ciel[1]), t / 0.62f) : melange(hex(ciel[1]), hex(ciel[2]), (t - 0.62f) / 0.38f); };
    // astres et nuages
    if (nuit) {
        if (idx != 0) for (int i = 0; i < 40; i++) { const float x = rnd() * 400, y = rnd() * 112; r.uiCircle(px(x), py(y), (0.5f + rnd()) * sx, hex(0xFFFFFF, 0.4f + rnd() * 0.6f)); }
        const float mx = idx == 1 ? 92 : 318;
        for (int k = 3; k >= 1; k--) r.uiCircle(px(mx), py(46), 12 * k * sr, hex(0xFFF6D8, 0.10f));
        r.uiCircle(px(mx), py(46), 13 * sr, hex(0xF4F1FF));
        r.uiCircle(px(mx + 6), py(42), 11 * sr, hex(ciel[0], 0.35f));
    } else {
        const float sy0 = idx == 1 ? 108 : 52;
        for (int k = 3; k >= 1; k--) r.uiCircle(px(300), py(sy0), 18 * k * sr, hex(0xFFF3C4, 0.13f));
        r.uiCircle(px(300), py(sy0), 17 * sr, hex(0xFFF8E0));
        for (int i = 0; i < 4; i++) {   // WFS-147 (ANO-42) : nuage = 3 ellipses opaques (blanc × 0,85 + ciel × 0,15, <g opacity=".85">)
            const float cx = 40 + rnd() * 320, cy = 30 + rnd() * 46, w = 40 + rnd() * 50;
            const vec4 cc = melange(hex(0xFFFFFF), cielEn(py(cy)), 0.15f);
            ellipse(r, px(cx), py(cy), w / 2 * sx, 9 * sy, cc);
            ellipse(r, px(cx - w * 0.12f), py(cy - 6), w / 4 * sx, 9 * sy, cc);
            ellipse(r, px(cx + w * 0.16f), py(cy - 4), w / 5 * sx, 8 * sy, cc);
        }
    }
    // décor de fond
    if (idx == 0) {   // ville : deux rangées d'immeubles, fenêtres allumées
        const unsigned tons[3] = {nuit ? 0x11121Cu : 0x4A5F7Cu, nuit ? 0x171826u : 0xA3B3C7u, nuit ? 0x0D0E17u : 0x8B9DB4u};
        for (int couche = 0; couche < 2; couche++) {
            float x = 0;
            while (x < 400) {
                const float w = std::min(18 + rnd() * 30, 400 - x), h = (couche ? 40 : 70) + rnd() * (couche ? 50 : 70), y = 150 - h;
                r.uiRect(px(x), py(y), w * sx, h * sy, hex(couche ? tons[0] : tons[1 + (int)(rnd() * 2)]));
                if (couche) for (float wy = y + 6; wy < 144; wy += 7) for (float wx = x + 4; wx < x + w - 4; wx += 6)
                    if (rnd() < (nuit ? 0.42f : 0.25f)) r.uiRect(px(wx), py(wy), 3 * sx, 3.4f * sy, hex(nuit ? (rnd() < 0.75f ? 0xFFCF7A : 0x7FF0FF) : 0xC9DEF2, 0.85f));
                x += w + 2;
            }
        }
        if (nuit) { r.uiRect(px(54), py(96), 44 * sx, 12 * sy, hex(0x00E5FF, 0.55f)); r.uiRect(px(292), py(84), 10 * sx, 40 * sy, hex(0xFF6A00, 0.55f)); }
    } else if (idx == 1) {   // corniche : la mer jusqu'à l'horizon, vaguelettes
        r.uiRect(X, HZ - 2 * sy, W, Y + H - HZ + 2 * sy, hex(nuit ? 0x0C1A2E : 0x1F86B8));
        for (int i = 0; i < 9; i++) r.uiRect(px(rnd() * 340), py(153 + i * 3), (20 + rnd() * 40) * sx, 1.2f * sy, hex(0xFFFFFF, nuit ? 0.18f : 0.5f));
    } else if (idx == 4) {   // WFS-343 : carte de données : collines aux couleurs de son sol, maisons et sapins en silhouette, lumières la nuit
        const vec4 coll = melange(hex(nuit ? c.solNuit : c.solJour), hex(nuit ? 0x05050C : 0xFFFFFF), 0.25f);
        for (int i = 0; i < 40; i++) { const float h0 = 26 + 16 * std::sin(i * 0.29f + 0.7f); r.uiRect(px(i * 10.f), py(150 - h0), 10.5f * sx, h0 * sy, coll); }
        for (int i = 0; i < 7; i++) {
            const float x = 18 + i * 55 + rnd() * 18, w = 22 + rnd() * 10, h = 14 + rnd() * 8;
            r.uiRect(px(x), py(150 - h), w * sx, h * sy, hex(nuit ? 0x1A1C26 : 0x8A6A58));
            r.uiRect(px(x - 2), py(150 - h - 6), (w + 4) * sx, 6 * sy, hex(nuit ? 0x2A2D3A : 0xF2F4F8));
            if (nuit) r.uiRect(px(x + w / 2 - 3), py(150 - h + 4), 6 * sx, 5 * sy, hex(0xFFCF7A, 0.9f));
            const float tx = std::min(x + w + 10, 390.f);
            for (int k = 0; k < 3; k++) r.uiRect(px(tx - 6 + k * 2), py(150 - 8 - k * 8), (12 - k * 4) * sx, 8 * sy, hex(nuit ? 0x0E1A14 : 0x2E5A3A));
        }
        if (nuit) for (int i = 0; i < 26; i++) r.uiCircle(px(10 + i * 15.f), py(128 + 4 * std::sin(i * 0.9f)), 1.6f * sx, hex(i % 3 == 0 ? c.accent : i % 3 == 1 ? 0xFFCF7Au : 0xFF3B30u));
    } else {   // campagne et circuit : collines (bosses en bandes)
        const unsigned c0 = idx == 2 ? (nuit ? 0x141C2C : 0x9AB7A0) : (nuit ? 0x10161C : 0x5F8A52), c1 = nuit ? 0x101622 : 0x7FA45A;
        for (int i = 0; i < 40; i++) {
            const float x = i * 10.f, h0 = 30 + 18 * std::sin(i * 0.31f + 1.f), h1 = 12 + 12 * std::sin(i * 0.23f + 2.5f);
            r.uiRect(px(x), py(150 - h0), 10.5f * sx, h0 * sy, hex(c0));
            if (idx == 2) r.uiRect(px(x), py(150 - h1), 10.5f * sx, h1 * sy, hex(c1));
        }
        if (idx == 2) for (float x : {70.f, 118.f, 338.f}) {   // éoliennes
            r.uiRect(px(x - 0.7f), py(104), 1.4f * sx, 34 * sy, hex(nuit ? 0x2A3350 : 0xF4F6F8));
            r.uiCircle(px(x), py(104), 3 * sx, hex(nuit ? 0xFF1F4B : 0xF4F6F8));
        } else {   // circuit : tribune et spectateurs
            r.uiRect(px(14), py(108), 120 * sx, 42 * sy, hex(nuit ? 0x1B1E2C : 0xC4C9D2));
            const unsigned pub[4] = {0xE2262B, 0xFFD23A, 0x2A6FDB, 0xF2F4F8};
            for (int l = 0; l < 4; l++) for (int i = 0; i < 22; i++) r.uiRect(px(20 + i * 5.2f), py(114 + l * 7), 3.2f * sx, 3 * sy, hex(pub[(int)(rnd() * 4)], nuit ? 0.55f : 0.85f));
            r.uiRect(px(148), py(98), 104 * sx, 11 * sy, hex(nuit ? 0x202434 : 0x2D3038));   // portique de départ
            for (int i = 0; i < 5; i++) r.uiCircle(px(172 + i * 14), py(103.5f), 3 * sx, hex(nuit ? 0xFF1F4B : 0x5B1020));
        }
    }
    // sol et route en perspective : un dégradé et un trapèze chacun (WFS-147, ANO-42 : plus de bandes) ; bords en trapèzes
    const vec4 sol = hex(nuit ? c.solNuit : c.solJour);
    if (idx != 1) r.uiRect(X, HZ, W, Y + H - HZ, sol);
    const float cx = idx == 1 ? 230 : 200, top = 6, bot = 190;
    auto pt = [&](float x, float y) { return vec2{px(x), py(y)}; };
    if (idx == 1)   // sable à droite de la route, à la plage
        polygoneCoupe(r, {pt(cx - top - 10, 150), pt(400, 150), pt(400, 250), pt(cx - bot - 50, 250)}, X, X + W, dessin2d::uni(sol));
    {
        dessin2d::Teinte route{hex(nuit ? 0x1A1B24 : 0x5D6068), hex(nuit ? 0x0B0C12 : 0x3A3C42), py(150), py(250)};
        polygoneCoupe(r, {pt(cx - top, 150), pt(cx + top, 150), pt(cx + bot, 250), pt(cx - bot, 250)}, X, X + W, route);
    }
    const int NR = 40;
    for (int i = 0; i < NR; i++) {
        const float t = (i + 0.5f) / NR, y0 = 150 + 100.f * i / NR, demi = top + t * (bot - top);
        const float bord = (1.2f + 2 * t) * sx;
        if (idx == 3) {   // vibreurs rouge et blanc
            const vec4 v = ((int)(std::sqrt(t) * 9)) % 2 ? hex(0xF2F4F8) : hex(0xE2262B);
            const float e = (4 + demi * 0.12f) * sx;
            const float g0 = std::max(X, px(cx - demi) - e), d1 = std::min(X + W, px(cx + demi) + e);   // vibreurs gardés dans la vignette
            if (px(cx - demi) > g0) r.uiRect(g0, py(y0), px(cx - demi) - g0, 100.f / NR * sy + 0.5f, v);
            if (d1 > px(cx + demi)) r.uiRect(px(cx + demi), py(y0), d1 - px(cx + demi), 100.f / NR * sy + 0.5f, v);
        }
        if (((int)(std::pow(t, 1 / 2.2f) * 12)) % 2 == 0)   // tirets des voies (± 1/3)
            for (float l : {-1 / 3.f, 1 / 3.f}) r.uiRect(px(cx + l * demi) - bord / 2, py(y0), bord, 100.f / NR * sy + 0.5f, hex(0xF2F4F8, nuit ? 0.6f : 0.85f));
    }
    if (idx != 3) {   // lignes de bord : un trapèze fin de chaque côté
        const vec4 v = hex(nuit ? 0xC9CDDA : 0xF2F4F8, 0.8f);
        const float b0 = 1.2f, b1 = 3.2f;
        polygoneCoupe(r, {pt(cx - top, 150), pt(cx - top + b0, 150), pt(cx - bot + b1, 250), pt(cx - bot, 250)}, X, X + W, dessin2d::uni(v));
        polygoneCoupe(r, {pt(cx + top - b0, 150), pt(cx + top, 150), pt(cx + bot, 250), pt(cx + bot - b1, 250)}, X, X + W, dessin2d::uni(v));
    }
    if (nuit) {   // feux d'une voiture au loin et reflet de l'accent sur la chaussée mouillée
        r.uiCircle(px(cx - 3), py(174), 14 * sx, hex(0xFF1F4B, 0.16f));
        r.uiCircle(px(cx - 9), py(172), 2 * sx, hex(0xFF1F4B)); r.uiCircle(px(cx + 3), py(172), 2 * sx, hex(0xFF1F4B));
        polygoneCoupe(r, {pt(cx - 3, 158), pt(cx + 3, 158), pt(cx + 29, 250), pt(cx - 26, 250)}, X, X + W, dessin2d::uni(hex(c.accent, 0.14f)));   // ANO-18 : reflet en un trapèze
    }
}

// zoneNames du web : noms distincts des quartiers de la carte, dans l'ordre (un tour de ZONES[ground])
std::vector<std::string> quartiers(const MapDef& m) {
    if (const auto d = cartes_v2::donnees(m.id)) return d->quartiers;   // WFS-343 : quartiers lus du paquet
    std::vector<std::string> v;
    for (int64_t i = 0; i < 12; i++) {
        const std::string n = nomQuartier(zoneName(m, i * ZONE_LEN, (int64_t)1 << 40));   // WFS-505 : dans la langue active
        if (std::find(v.begin(), v.end(), n) == v.end()) v.push_back(n);
    }
    return v;
}

class Terrain : public System {
public:
    const char* name() const override { return "terrain"; }
    void onEcran(Ecran, Ecran nouveau, const GameView& g) override {
        doigt_ = -1;
        if (nouveau != Ecran::Terrain) { g_carte = g_prec = g_suiv = g_jour = g_nuit = g_retour = g_rouler = {}; return; }
        // carte en cours présélectionnée (pickMap garde MAP)
        cartes_v2::decouvrir(); listeCartes();   // WFS-343 (RG-03) : cartes des paquets du téléphone
        g_index = std::min(g_index, nbCartes() - 1);
        for (int i = 0; i < nbCartes(); i++) if (g.map && g.map->id == g_liste[(size_t)i].id) g_index = i;
        if (g.time - tListe_ >= 60 || !cartesPaquets().cherche()) {   // WFS-307 : liste des paquets lue en entrant (une fois par minute au plus)
            std::vector<std::pair<std::string, uint64_t>> v;
            for (const Carte& c : CARTES) v.push_back({c.id, cartes_v2::empreinteAttendue(c.id)});
            cartesPaquets().chercher(v); tListe_ = g.time;
        }
        if (sauvegarde().actif()) {   // WFS-24 : moment de la dernière course (WFS-208 : moment fin, sinon déduit de tod)
            // WFS-375 (RG-03) : un ancien moment (aube, matin, dorée, réelle → jour ; bleue → nuit) se range dans l'un des deux modes
            g_nuitChoisie = atmosphere::momentDeNuit(atmosphere::momentDepuisNom(sauvegarde().moment.empty() ? sauvegarde().tod.c_str() : sauvegarde().moment.c_str()));
            g_moment = modeSimple(g_nuitChoisie);
        }
    }
    // RG-02 : glisser sur la vignette = carte suivante / précédente ; appuis sur les flèches, Jour / Nuit, Retour, Rouler
    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran != Ecran::Terrain) return false;
        if (toucherPaquetCarte(g_liste[(size_t)g_index].id.c_str(), t.id, t.x, t.y, t.down, g.time)) return true;   // WFS-307 : boutons du paquet de la carte
        const float k = ui::petitEcran(g.uiWidth, g.uiHeight) ? ui::unitePetitEcran(g.uiWidth, g.uiHeight) : g.uiHeight / 720.f;   // WFS-147
        if (t.down) {
            if (doigt_ < 0) { doigt_ = t.id; x0_ = t.x; y0_ = t.y; }
            return true;
        }
        if (t.id != doigt_) return true;
        doigt_ = -1;
        const float dx = t.x - x0_;
        if (g_carte.contient(x0_, y0_) && std::fabs(dx) > 50 * k) { pas(dx < 0 ? 1 : -1); return true; }   // glissé
        if (std::fabs(dx) > 30 * k) return true;
        if (g_prec.contient(t.x, t.y)) pas(-1);
        else if (g_suiv.contient(t.x, t.y)) pas(1);
        else if (g_jour.contient(t.x, t.y)) { g_nuitChoisie = false; g_moment = modeSimple(false); WFS_LOG("terrain : Jour (%s)", atmosphere::nomMoment(g_moment)); }   // WFS-375 : sans défilement
        else if (g_nuit.contient(t.x, t.y)) { g_nuitChoisie = true; g_moment = modeSimple(true); WFS_LOG("terrain : Nuit (%s)", atmosphere::nomMoment(g_moment)); }   // WFS-375 : sans défilement
        else if (g_retour.contient(t.x, t.y)) { WFS_LOG("terrain : « Retour »"); g.allerA(Ecran::Menu); }
        else if (g_rouler.contient(t.x, t.y) && carteATelecharger(g_liste[(size_t)g_index].id)) {   // WFS-551 (RG-07) : « Rouler » éteint
            WFS_LOG("terrain : « Rouler » éteint : %s pas encore téléchargée", g_liste[(size_t)g_index].id.c_str());
        } else if (g_rouler.contient(t.x, t.y)) {   // RG-03 : la partie est reconstruite sur la carte choisie, puis la course
            WFS_LOG("terrain : « Rouler » sur %s (%s)", g_liste[(size_t)g_index].id.c_str(), g_nuitChoisie ? "nuit" : "jour");
            Game::demandes().carte = g_liste[(size_t)g_index].id.c_str(); Game::demandes().nuit = g_nuitChoisie;
            if (cartesPaquets().changee(g_liste[(size_t)g_index].id.c_str())) { Game::demandes().recharger = true; cartesPaquets().prise(g_liste[(size_t)g_index].id.c_str()); WFS_LOG("terrain : paquet de %s changé, carte relue", g_liste[(size_t)g_index].id.c_str()); }   // WFS-307 (RG-08)
            eclairage::choisirMoment((int)g_moment); if (sauvegarde().actif()) sauvegarde().moment = atmosphere::nomMoment(g_moment);   // WFS-208 (enregistré par Game::changerCarte)
        }
        return true;   // l'écran garde tous les touchers
    }
    void drawUi(Renderer& r, const GameView& g) override {
        if (g.ecran != Ecran::Terrain || g.sceneDediee || !ui::fontReady()) return;
        const textes::Journal journal;   // WFS-505 : textes notés au banc « textes »
        // WFS-557 (maquettes 18a à 18d, 03 ; RG-23, ordre de l'utilisateur : traité dans WFS-557). Unité k = H / 720 (maquette
        // 1600 × 720), réduite en 16:9 pour tenir en largeur. « CHOISIS TA CARTE » et « Étape 3 sur 3 · carte n / N » en haut à
        // gauche, solde d'étoiles en haut à droite ; grande carte arrondie au centre (halo orange quand elle est installée) avec,
        // à gauche et à droite, les cartes voisines (toucher = carte précédente / suivante) ; nom, description, longueur,
        // quartiers en pastilles ; points sous la carte ; « Retour / Choix de la voiture » en autocollant et « Midi / Nuit » en bas
        // à gauche ; « Rouler » rond en bas à droite. Image de la carte : illustration du jeu, provisoire (vue aérienne : RG-23).
        const float W = g.uiWidth, H = g.uiHeight, k = std::min(H / 720.f, (W - g.marges.gauche - g.marges.droite) / 1600.f);
        const Marges& mg = g.marges;
        const float ox = mg.gauche + (W - mg.gauche - mg.droite - 1600 * k) / 2, oy = mg.haut + (H - mg.haut - mg.bas - 720 * k) / 2;
        r.uiRect(0, 0, W, H, hex(BG, 0.96f));   // fond nuit
        // ---------- en-tête ----------
        ui::text(r, ox + 46 * k, oy + 26 * k, 64 * k, ui::majuscules(ui::tr("terrain.titre")), hex(TEXTE), ui::GAUCHE, 0.01f, false, true);
        {
            char sous[64]; std::snprintf(sous, sizeof sous, "%d / %d", g_index + 1, nbCartes());
            ui::AvecPolice inter(ui::Police::Inter);
            ui::text(r, ox + 44 * k, oy + 86 * k, 15 * k, ui::tr("terrain.etape", sous), hex(0xD7DAE2), ui::GAUCHE, 0, false);
        }
        {   // solde d'étoiles : pilule de verre inclinée
            const std::string nb = ui::nombre(boutique::solde(sauvegarde()));
            const float tp = 24 * k, h = 40 * k, w = ui::width(tp, nb, 0) + tp + 40 * k, x = ox + 1568 * k - w, y = oy + 26 * k, b = 8 * k;
            const vec2 q[4] = {{x + b, y}, {x + w, y}, {x + w - b, y + h}, {x, y + h}};
            dessin2d::polygone(r, q, 4, dessin2d::uni(hex(0x15171D, 0.85f)));
            dessin2d::contour(r, q, 4, 1 * k, hex(TEXTE, 0.18f));
            garage::etoile(r, x + 20 * k + tp * 0.45f, y + h / 2, tp * 0.48f, hex(ACCENT));
            ui::text(r, x + 20 * k + tp + 4 * k, y + (h - tp) / 2, tp, nb, hex(TEXTE), ui::GAUCHE, 0, false, true);
        }
        // ---------- cartes voisines (150 × 340), grisées ----------
        g_prec = g_suiv = {};
        for (int s = 0; s < 2; s++) {
            const int i = g_index + (s == 0 ? -1 : 1);
            if (i < 0 || i >= nbCartes()) continue;
            const float x = ox + (s == 0 ? 30 : 1420) * k, y = oy + 160 * k, w = 150 * k, h = 340 * k;
            (s == 0 ? g_prec : g_suiv) = {x, y, w, h};
            vignette(r, x, y, w, h, g_liste[(size_t)i], g_nuitChoisie);
            r.uiRect(x, y, w, h, hex(BG, 0.55f));
            arrondirCoins(r, x, y, w, h, 14 * k);
        }
        // ---------- grande carte (1180 × 440) ----------
        const float cx0 = ox + 210 * k, cy0 = oy + 110 * k, cw = 1180 * k, ch = 440 * k;
        g_carte = {cx0, cy0, cw, ch};
        const CarteVue& c = g_liste[(size_t)g_index];
        const MapDef& m = mapById(c.id.c_str());
        const bool aTelecharger = carteATelecharger(c.id);
        if (!aTelecharger) {   // carte installée : halo orange autour (maquette 18c)
            vec2 q[ui::boutons::MAX_SOMMETS];
            const int n = ui::boutons::contourArrondi(q, cx0, cy0, cw, ch, 18 * k);
            dessin2d::halo(r, q, n, 18 * k, hex(ACCENT, 0.35f));
        }
        vignette(r, cx0, cy0, cw, ch, c, g_nuitChoisie);
        assombrirCarte(r, c.id.c_str(), cx0, cy0, cw, ch);   // WFS-307 (RG-01) : paquet publié pas encore téléchargé : image grise
        // ombre du bas pour le texte (transparent → fond)
        degradeV(r, cx0, cy0 + ch * 0.45f, cw, ch * 0.25f, hex(BG, 0), hex(BG, 0.55f));
        degradeV(r, cx0, cy0 + ch * 0.70f, cw, ch * 0.30f, hex(BG, 0.55f), hex(BG, 0.90f));
        arrondirCoins(r, cx0, cy0, cw, ch, 18 * k);
        if (!aTelecharger) { vec2 q[ui::boutons::MAX_SOMMETS]; const int n = ui::boutons::contourArrondi(q, cx0, cy0, cw, ch, 18 * k);
                             dessin2d::contour(r, q, n, 1.5f * k, hex(ACCENT, 0.85f)); }
        {   // nom, description, longueur, quartiers
            const float ix = cx0 + 32 * k;
            { const textes::NomPropre np;   // WFS-505 : nom de la carte, nom propre (RC-E20-03)
              ui::text(r, ix + 2 * k, cy0 + 300 * k, 64 * k, ui::majuscules(m.name), hex(TEXTE), ui::GAUCHE, 0.01f, false, true); }
            { ui::AvecPolice inter(ui::Police::Inter);
              ui::textBoite(r, ix, cy0 + 365 * k, cw - 64 * k, 20 * k, 17 * k, c.art < 4 ? ui::tr(c.desc.c_str()) : c.desc, hex(0xE6E8EE), ui::GAUCHE, 0); }
            std::string km = ui::decimal(std::round(longueurKm(m) * 10) / 10, 1);
            if (km.size() > 2 && km.compare(km.size() - 2, 2, ",0") == 0) km.resize(km.size() - 2);
            const std::vector<std::string> qs = quartiers(m);
            const float fy = cy0 + 397 * k, fp = 26 * k;
            float x = ix;
            const std::string t1 = km + " km", t2 = ui::tr(qs.size() > 1 ? "terrain.n_quartiers" : "terrain.n_quartier", std::to_string(qs.size()));
            ui::text(r, x, fy, fp, t1, hex(TEXTE), ui::GAUCHE, 0, false, true); x += ui::width(fp, t1, 0) + 14 * k;
            r.uiRect(x, fy + fp * 0.5f, 3 * k, 3 * k, hex(TEXTE, 0.5f)); x += 17 * k;
            ui::text(r, x, fy, fp, t2, hex(TEXTE), ui::GAUCHE, 0, false, true); x += ui::width(fp, t2, 0) + 18 * k;
            ui::AvecPolice inter(ui::Police::InterMedium);
            for (const std::string& q : qs) {   // pastilles des quartiers : verre arrondi, bord clair
                const float pw = ui::width(13 * k, q, 0) + 24 * k, ph = 22 * k, py = fy + (fp - ph) / 2 + 2 * k;
                if (x + pw > cx0 + cw - 24 * k) break;
                vec2 qq[ui::boutons::MAX_SOMMETS];
                const int n = ui::boutons::contourArrondi(qq, x, py, pw, ph, 6 * k);
                dessin2d::polygone(r, qq, n, dessin2d::uni(hex(0x15171D, 0.75f)));
                dessin2d::contour(r, qq, n, 1 * k, hex(TEXTE, 0.30f));
                ui::text(r, x + pw / 2, py + (ph - 13 * k) / 2, 13 * k, q, hex(0xE6E8EE), ui::CENTRE, 0, false);
                x += pw + 16 * k;
            }
        }
        dessinerPaquetCarte(r, c.id.c_str(), cx0, cy0, cw, ch, k, g.time);   // WFS-307 / WFS-551 : badge, Télécharger, progression, Supprimer
        // ---------- points sous la carte ----------
        {
            const int n = nbCartes();
            float tot = 0; for (int i = 0; i < n; i++) tot += (i == g_index ? 35 : 16) * k + (i ? 6 * k : 0);
            float x = ox + 800 * k - tot / 2;
            for (int i = 0; i < n; i++) {
                const float w = (i == g_index ? 35 : 16) * k;
                r.uiRect(x, oy + 567 * k, w, 4 * k, i == g_index ? hex(ACCENT) : i < g_index ? hex(TEXTE, 0.8f) : hex(TEXTE, 0.25f));
                x += w + 6 * k;
            }
        }
        // ---------- Midi / Nuit (sélecteur de verre 170 × 36) ----------
        {
            const float sx = ox + 240 * k, sy = oy + 654 * k, sh = 36 * k, in = 3 * k, tw = 82 * k;
            ui::boutons::selecteurFond(r, sx, sy, 2 * tw + 2 * in, sh);
            for (int s2 = 0; s2 < 2; s2++) {
                const bool nuit = s2 == 1, on = nuit == g_nuitChoisie;
                const float x = sx + in + s2 * tw;
                (nuit ? g_nuit : g_jour) = {x, sy - 4 * k, tw, sh + 8 * k};   // zone ≥ 44 px
                ui::boutons::selecteurChoix(r, x, sy + in, tw, sh - 2 * in, "", on);
                const vec4 ci = on ? hex(BG) : hex(TEXTE);
                const float icx = x + 20 * k, icy = sy + sh / 2;
                if (!nuit) { dessin2d::anneau(r, icx, icy, 0, 3.2f * k, ci); for (int a2 = 0; a2 < 8; a2++) { const vec2 l[2] = {{icx + std::cos(a2 * 0.785f) * 5 * k, icy + std::sin(a2 * 0.785f) * 5 * k}, {icx + std::cos(a2 * 0.785f) * 7 * k, icy + std::sin(a2 * 0.785f) * 7 * k}}; dessin2d::trait(r, l, 2, 1.2f * k, ci); } }
                else dessin2d::arc(r, icx + 1 * k, icy, 5 * k, 1.4f * k, 1.15f * PI, 1.85f * PI + PI * 0.15f, ci);
                ui::AvecPolice inter(ui::Police::InterMedium);
                ui::text(r, x + 31 * k, sy + (sh - 14 * k) / 2, 14 * k, ui::majuscules(ui::tr(nuit ? "terrain.nuit" : "terrain.midi")), ci, ui::GAUCHE, 0.08f, false);
            }
        }
        // ---------- « Retour / Choix de la voiture » (autocollant) et « Rouler » (rond) ----------
        g_retour = {ox + 70 * k, oy + 616 * k, 142 * k, 58 * k};
        ui::boutons::autocollant(r, g_retour.x, g_retour.y, g_retour.w, g_retour.h, ui::majuscules(ui::tr("terrain.retour")), 1, false, 26 * k, ui::tr("terrain.retour.sous"));
        const float d = 120 * k;
        g_rouler = {ox + 1460 * k - d / 2, oy + 604 * k - d / 2, d, d};
        // WFS-551 (RG-07, maquette 18a) : carte pas encore téléchargée : « Rouler » éteint, « Carte à télécharger » dessous
        ui::boutons::startStop(r, g_rouler.x, g_rouler.y, d, d, ui::majuscules(ui::tr("terrain.rouler")), 1, false, aTelecharger ? ui::tr("terrain.a_telecharger") : "", 18 * k, aTelecharger);
        for (const Zone* z : {&g_rouler, &g_retour, &g_prec, &g_suiv, &g_jour, &g_nuit}) manette::focus(*z, z == &g_retour);   // WFS-22
    }
    // coins arrondis d'un rectangle dessiné en carré : quatre coins remplis du fond (l'illustration reste à l'intérieur)
    static void arrondirCoins(Renderer& r, float x, float y, float w, float h, float R) {
        for (int c2 = 0; c2 < 4; c2++) {
            const float cx = c2 % 3 == 0 ? x + R : x + w - R, cy = c2 < 2 ? y + R : y + h - R;
            const float a0 = c2 == 0 ? 1.5f * PI : c2 == 1 ? 0.f : c2 == 2 ? 0.5f * PI : PI;
            vec2 q[12]; int n = 0;
            q[n++] = {c2 % 3 == 0 ? x : x + w, c2 < 2 ? y : y + h};
            for (int i = 0; i <= 8; i++) { const float a = a0 + 0.5f * PI * i / 8; q[n++] = {cx + std::sin(a) * R, cy - std::cos(a) * R}; }
            dessin2d::polygone(r, q, n, dessin2d::uni(hex(BG)));
        }
    }
private:
    void pas(int dir) {   // stepMap du web : bornée aux extrémités
        const int i = std::clamp(g_index + dir, 0, nbCartes() - 1);
        if (i != g_index) { g_index = i; sfxInterface(SfxId::Carte); WFS_LOG("terrain : carte %s (%02d / %02d)", g_liste[(size_t)i].id.c_str(), i + 1, nbCartes()); }
    }
    int doigt_ = -1;
    float x0_ = 0, y0_ = 0;
    double tListe_ = -1e9;
};

} // namespace

void choisirCarte(const std::string& id, bool nuit) { listeCartes(); for (int i = 0; i < nbCartes(); i++) if (id == g_liste[(size_t)i].id) g_index = i; g_nuitChoisie = nuit; g_moment = modeSimple(nuit); }

bool centreRouler(float& x, float& y) {
    if (g_rouler.w <= 0) return false;
    x = g_rouler.x + g_rouler.w / 2; y = g_rouler.y + g_rouler.h / 2;
    return true;
}

double longueurKm(const MapDef& m) {   // mapLengthKm (screens.js)
    if (const auto d = cartes_v2::donnees(m.id)) return d->segments() * SEG * M / 1000;   // WFS-343 : routes du paquet
    Mulberry32 R(m.seed);
    int n = 120;
    for (int i = 0; i < m.sections; i++) {
        const double t = R.next();
        if (R.next() < 0.5) R.next();
        if (t < 0.22) n += 50 + (int)(40 + R.next() * 40);
        else if (t < 0.7) { R.next(); R.next(); n += 60 + (int)(30 + R.next() * 50); }
        else { R.next(); n += 195; }
    }
    n += 180 + 70;
    return n * SEG * M / 1000;
}

// banc d'essai (WFS-41) : touchers simulés comme ceux d'un doigt
void benchTerrain(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("terrain : chargement impossible"); courseBenchRules(false); return; }
    g.setAutopilot(false); g.camMode = 0;
    double t = 1100;
    int n = 0;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; if (++n % 10 == 0) g.render(t); };
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, carte %s, %s", nom, nomEcran(g.ecran()), g_liste[(size_t)g_index].id.c_str(), quoi);
    };
    auto toucher = [&](int id, const Zone& z) {
        const float x = z.x + z.w / 2, y = z.y + z.h / 2;
        g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); image();
    };
    for (int i = 0; i < 30; i++) image();
    for (int i = 0; i < 4; i++) WFS_LOG("terrain : %s ≈ %.2f km, %zu quartiers", CARTES[i].id, longueurKm(mapById(CARTES[i].id)), quartiers(mapById(CARTES[i].id)).size());
    g.allerA(Ecran::Terrain); image(); g.render(t);
    capture("terrain-0", "ville choisie, nuit par défaut");
    // glissé vers la gauche sur la vignette (carte suivante), puis deux appuis sur la flèche droite → circuit ; Jour
    const float y = g_carte.y + g_carte.h / 2;
    g.onTouch({1, g_carte.x + g_carte.w * 0.7f, y, true}); image(); g.onTouch({1, g_carte.x + g_carte.w * 0.3f, y, true}); image();
    g.onTouch({1, g_carte.x + g_carte.w * 0.3f, y, false}); image();
    g.render(t); toucher(2, g_suiv); g.render(t); toucher(3, g_suiv); g.render(t); toucher(4, g_jour);
    capture("terrain-1", "circuit choisi, jour");
    // Rouler : partie reconstruite sur le circuit, décompte puis course en pilote automatique
    g.render(t); toucher(5, g_rouler);
    g.setAutopilot(true);
    for (int i = 0; i < 60 * 6; i++) image();
    WFS_LOG("terrain-rouler : écran %s, %.0f km/h, %.0f m", nomEcran(g.ecran()), g.player().kmh(), g.player().dist * M);
    capture("terrain-rouler", "course lancée après « Rouler »");
    // WFS-375 (RG-01, RG-05, ANO-232) : deux appuis sur JOUR restent « JOUR » (midi, avant : « Dorée ») ; Campagne en plein jour
    // (horloge du banc indifférente : faketime 22:15 dans la PR), puis deux appuis sur NUIT restent « NUIT » ; Corniche de nuit
    auto essai = [&](const char* carte, bool nuit, const char* ecran, const char* course) {
        g.allerA(Ecran::Terrain); image(); g.render(t);
        choisirCarte(carte, !nuit); g.render(t);
        toucher(6, nuit ? g_nuit : g_jour); g.render(t); toucher(7, nuit ? g_nuit : g_jour);
        capture(ecran, nuit ? "deux appuis sur Nuit" : "deux appuis sur Jour");
        WFS_LOG("terrain : moment choisi %s", atmosphere::nomMoment(g_moment));
        g.render(t); toucher(8, g_rouler); g.setAutopilot(true);
        for (int i = 0; i < 60 * 6; i++) image();
        capture(course, nuit ? "course de nuit" : "course de jour");
    };
    essai("campagne", false, "terrain-jour-2appuis", "terrain-campagne-jour");
    essai("plage", true, "terrain-nuit-2appuis", "terrain-corniche-nuit");
    eclairage::choisirMoment(-1);   // les bancs suivants gardent jour / nuit
    g.shutdown();
    unsetenv("WFS_MOMENT");
    courseBenchRules(false);
}

} // namespace ecrans

WFS_SYSTEM(ecrans::Terrain, 452)   // interface (400-499) : après le menu (450), avant la pause (455)

} // namespace wfs
