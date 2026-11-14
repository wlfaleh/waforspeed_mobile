// WaForSpeed — écran du cadeau du jour (WFS-501) : voir cadeau_jour.h.
#include "cadeau_jour.h"
#include <algorithm>
#include <cmath>
#include <optional>
#include "reglages_appui.h"   // appui::dansCible : cible de 48 dp (comme les Réglages, les CGU et les Crédits)
#include "textes_banc.h"      // banc « textes » (journal des textes de cet écran)
#include "../garage_achat.h"   // étoile dessinée (la police n'a pas le caractère ★) et texte avec ★ (WFS-489)
#include "../langue.h"
#include "../petit_ecran.h"
#include "../text.h"
#include "../../core/log.h"
#include "../../game/cars.h"
#include "../../game/commandes_tactiles.h"   // dessin2d
#include "../../game/personnalisation.h"
#include "../../game/pret.h"   // WFS-502 : « Activer » la voiture prêtée
#include "../../game/sauvegarde.h"
#include "../../platform/manette.h"
#include "../../render/renderer.h"

namespace wfs {
namespace ecrans::cadeau_jour {

// calculs purs (secondesAvantProchain, texteHeures, lignesChances, restantsGarantie) : cadeau_jour_calculs.cpp

namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, ACCENT_PRESSE = 0xFF9A4D,
               SURFACE2 = 0x1E2130, INTRO = 0xC9CDDA;
const vec4 SURFACE{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.97f}, GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.16f};
constexpr double DUREE = 1.5;   // RG-02 : animation d'ouverture ≈ 1,5 s

struct Zone { float x = 0, y = 0, w = 0, h = 0; };
enum class Vue { Animation, Gain, Attente, Chances };
enum Cible { RIEN = 0, RETOUR, CHANCES, ACTIVER, FOND };

struct Etat {
    bool ouvert = false;
    Vue vue = Vue::Attente, avantChances = Vue::Attente;
    double t0 = -1, temps = 0;               // début de l'animation (posé à la 1re image), temps de la dernière image
    std::optional<cadeau::Resultat> res;     // gain montré (tiré, ou dernier gain réaffiché)
    Zone retour, chances, activer, bouton;
    int doigt = -1, cible = RIEN;
    float dp = 1, dpBouton = 1;
    std::optional<cadeau::Instant> heureBanc;          // banc : heure du téléphone forcée
    std::optional<cadeau::Resultat> resultatBanc;      // banc : gain montré à la place du gain tiré (la sauvegarde suit le vrai tirage)
} g;

cadeau::Instant heure() { return g.heureBanc ? *g.heureBanc : cadeau::maintenant(); }

float ease(float t) { t = std::clamp(t, 0.f, 1.f); return 1 - (1 - t) * (1 - t) * (1 - t); }   // sortie douce (ease-out cubique)

// nom affiché d'une voiture (configgame.json, section « voitures ») : jamais une marque
std::string nomVoiture(const std::string& id) {
    const CarDef& c = carById(id.c_str());
    return id == c.id ? std::string(modele(c)) : id;
}
// nom d'un coloris ou d'une skin dans la langue active (couleurs : couleur.<id> ; finitions : finition.<id> ; skins : skin.<id>)
std::string nomObjet(const std::string& id) {
    const perso::Objet* o = perso::objet(id);
    if (!o) return id;
    const char* prefixe = o->type == perso::Type::Couleur ? "couleur." : o->type == perso::Type::Finition ? "finition." : "skin.";
    const std::string& t = ui::tr((prefixe + id).c_str());
    return t.empty() ? std::string(o->nom) : t;   // finition.<id> arrive avec WFS-491 : nom du catalogue en attendant
}
// teinte de la pastille d'un coloris (couleur : sa peinture ; finition : gris argent)
unsigned teinteColoris(const std::string& id) {
    for (int i = 0; i < NB_PEINTURES; i++) if (id == PEINTURES[i].id) return PEINTURES[i].hex;
    return 0xB8BCC6;
}
// « golf:rose » → voiture, objet
void separer(const cadeau::Resultat& r, std::string& voiture, std::string& objet) { voiture = nomVoiture(r.voiture); objet = nomObjet(r.objet); }

std::string pourcent(double v) {
    char b[32];
    if (std::fabs(v - std::round(v)) < 1e-9) std::snprintf(b, sizeof b, "%lld", (long long)std::llround(v));
    else std::snprintf(b, sizeof b, "%.1f", v);
    std::string s = b;
    if (ui::langue::active() == ui::langue::Langue::Francais) std::replace(s.begin(), s.end(), '.', ',');
    return ui::tr("cadeau.chances.pourcent", s);
}

// phrase du gain (RG-03)
std::string phraseGain(const cadeau::Resultat& r) {
    std::string v, o;
    switch (r.gain) {
        case cadeau::Gain::Etoiles: return ui::tr("cadeau.gain.etoiles", ui::nombre(r.etoiles));
        case cadeau::Gain::Coloris: separer(r, v, o); return ui::tr("cadeau.gain.coloris", o, v);
        case cadeau::Gain::Skin: separer(r, v, o); return ui::tr("cadeau.gain.skin", o, v);
        case cadeau::Gain::Voiture: {
            char h[16]; std::snprintf(h, sizeof h, "%g", config().cadeau.pretHeures);
            return ui::tr("cadeau.gain.voiture", h, nomVoiture(r.voiture));
        }
    }
    return "";
}

// polygone convexe dans le sens horaire de l'écran, quel que soit l'ordre donné
void poly(Renderer& r, vec2* p, int n, const dessin2d::Teinte& t) {
    float aire = 0;
    for (int i = 0; i < n; i++) { const vec2 &a = p[i], &b = p[(i + 1) % n]; aire += a.x * b.y - b.x * a.y; }
    if (aire < 0) std::reverse(p, p + n);
    dessin2d::polygone(r, p, n, t);
}
void polyContour(Renderer& r, vec2* p, int n, float ep, const vec4& c) {
    float aire = 0;
    for (int i = 0; i < n; i++) { const vec2 &a = p[i], &b = p[(i + 1) % n]; aire += a.x * b.y - b.x * a.y; }
    if (aire < 0) std::reverse(p, p + n);
    dessin2d::contour(r, p, n, ep, c);
}
// rectangle (x, y, w, h) tourné de a autour de (px, py)
void rectTourne(vec2* q, float x, float y, float w, float h, float px, float py, float a) {
    const float c = std::cos(a), s = std::sin(a);
    const vec2 coins[4] = {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}};
    for (int i = 0; i < 4; i++) {
        const float dx = coins[i].x - px, dy = coins[i].y - py;
        q[i] = {px + dx * c - dy * s, py + dx * s + dy * c};
    }
}
void pieceNeon(Renderer& r, vec2* q, float k, float a, bool halo) {
    if (halo) dessin2d::halo(r, q, 4, 16 * k, hex(ACCENT, 0.45f * a));
    dessin2d::Teinte t{hex(SURFACE2, 0.97f * a), hex(0x12141C, 0.97f * a), std::min({q[0].y, q[1].y, q[2].y, q[3].y}), std::max({q[0].y, q[1].y, q[2].y, q[3].y})};
    poly(r, q, 4, t);
    polyContour(r, q, 4, 2.5f * k, hex(ACCENT, a));
}

// boîte cadeau en 2D (néons) : corps, ruban, couvercle, nœud ; t : temps depuis le début de l'animation (s)
void dessinerBoite(Renderer& r, float cx, float bas, float k, float t, float a) {
    k *= 1.45f;   // boîte grande au centre du panneau
    const float bw = 150 * k, bh = 104 * k, lh = 28 * k, lw = bw + 18 * k, rib = 20 * k;
    // tremblement (0 → 0,9 s), de plus en plus fort ; ouverture du couvercle (0,9 → 1,3 s)
    const float tr = std::clamp(t / 0.9f, 0.f, 1.f), tremble = t < 0.9f ? std::sin(t * 2 * PI * 9) * 0.07f * tr : 0.f;
    const float ouv = ease((t - 0.9f) / 0.4f);
    const float haut = bas - bh;
    // lumière qui sort de la boîte (dès 0,95 s) : rayons et lueur, derrière le couvercle
    if (t > 0.95f) {
        const float l = std::clamp((t - 0.95f) / 0.25f, 0.f, 1.f) * (t < 1.25f ? 1.f : std::max(0.25f, 1 - (t - 1.25f) / 0.25f * 0.75f));
        dessin2d::couronne(r, cx, haut, 0, 200 * k, hex(0xFFB066, 0.75f * l * a), hex(ACCENT, 0));
        for (int i = 0; i < 11; i++) {
            const float ang = PI + PI * (i + 0.5f) / 11, lg = (150 + 70 * (i % 2)) * k * l, ep = 0.06f;
            vec2 ray[3] = {{cx, haut}, {cx + std::cos(ang - ep) * lg, haut + std::sin(ang - ep) * lg}, {cx + std::cos(ang + ep) * lg, haut + std::sin(ang + ep) * lg}};
            poly(r, ray, 3, dessin2d::uni(hex(i % 2 ? ACCENT2 : 0xFFB066, 0.6f * l * a)));
        }
    }
    vec2 q[4];
    // corps
    rectTourne(q, cx - bw / 2, haut, bw, bh, cx, bas, tremble);
    pieceNeon(r, q, k, a, true);
    rectTourne(q, cx - rib / 2, haut, rib, bh, cx, bas, tremble);
    poly(r, q, 4, dessin2d::uni(hex(ACCENT2, 0.85f * a)));
    // couvercle : il monte, part à gauche et bascule
    const float ly = haut - lh - ouv * 74 * k, lx = cx - lw / 2 - ouv * 26 * k, la = tremble - ouv * 0.42f;
    const float pcx = lx + lw / 2, pcy = ly + lh;
    rectTourne(q, lx, ly, lw, lh, pcx, pcy, la);
    pieceNeon(r, q, k, a, ouv < 0.5f);
    rectTourne(q, pcx - rib / 2, ly, rib, lh, pcx, pcy, la);
    poly(r, q, 4, dessin2d::uni(hex(ACCENT2, 0.85f * a)));
    // nœud : deux boucles en losange sur le couvercle
    const float c = std::cos(la), s = std::sin(la);
    auto pt = [&](float dx, float dy) { return vec2{pcx + dx * c - (dy - lh) * s, pcy + dx * s + (dy - lh) * c}; };
    for (int cote = -1; cote <= 1; cote += 2) {
        vec2 b[4] = {pt(0, 0), pt(cote * 14 * k, -22 * k), pt(cote * 40 * k, -26 * k), pt(cote * 34 * k, -4 * k)};
        poly(r, b, 4, dessin2d::uni(hex(SURFACE2, a)));
        polyContour(r, b, 4, 2.5f * k, hex(ACCENT2, a));
    }
}

// bouton du panneau : principal (orange en biseau, comme « Retour » des Crédits) ou secondaire (vitré à liseré cyan)
void bouton(Renderer& r, const Zone& z, float kb, float tp, const char* cle, bool principal, bool presse, float largeurTexte) {
    const std::string t = ui::majuscules(ui::tr(cle));
    if (principal) dessin2d::biseau(r, z.x, z.y, z.w, z.h, 14 * kb, dessin2d::uni(hex(presse ? ACCENT_PRESSE : ACCENT)));
    else {
        dessin2d::biseau(r, z.x - kb, z.y - kb, z.w + 2 * kb, z.h + 2 * kb, 14 * kb, dessin2d::uni(BORD));
        dessin2d::biseau(r, z.x, z.y, z.w, z.h, 14 * kb, dessin2d::uni(GLASS));
        if (presse) dessin2d::biseau(r, z.x, z.y, z.w, z.h, 14 * kb, dessin2d::uni(hex(ACCENT2, 0.22f)));
    }
    ui::textBoite(r, z.x + z.w / 2, z.y, largeurTexte, z.h, tp, t, principal ? hex(BG) : hex(ACCENT2), ui::CENTRE, 0.12f, false);
}
// RC-E20-04 : largeur d'un bouton mesurée sur le plus long de ses deux textes (même bouton dans les deux langues)
float largeurTexte(float tp, const char* cle) {
    return std::max(ui::width(tp, ui::majuscules(ui::trLangue(cle, ui::langue::Langue::Francais)), 0.12f),
                    ui::width(tp, ui::majuscules(ui::trLangue(cle, ui::langue::Langue::Anglais)), 0.12f));
}

// ui::textBoite où chaque « ★ » est une étoile dessinée (garage::texteEtoiles) ; trop large : police réduite de 20 % au plus
void texteCase(Renderer& r, float x, float y, float largeur, float hauteur, float px, const std::string& s, const vec4& c, ui::Align align,
               float esp = 0, bool italique = false) {
    if (s.find("\xE2\x98\x85") == std::string::npos) { ui::textBoite(r, x, y, largeur, hauteur, px, s, c, align, esp, false, italique); return; }
    const float w = garage::largeurEtoiles(px, s, esp);
    if (w > largeur) px *= std::max(1 - ui::REDUCTION_MAX, largeur / w);
    garage::texteEtoiles(r, x, y + (hauteur - px) / 2, px, s, c, align, esp);
}

int cibleSous(float x, float y) {
    auto dans = [&](const Zone& z) { return ecrans::appui::dansCible(z.x, z.y, z.w, z.h, g.dp, x, y); };
    if (dans(g.retour)) return RETOUR;
    if (dans(g.chances)) return CHANCES;
    if (dans(g.activer)) return ACTIVER;
    return FOND;
}

// ---------------------------------------------------------------- contenus du panneau
// gain (RG-03) ; a : opacité, e : agrandissement d'entrée (0,8 → 1)
void dessinerGain(Renderer& r, const cadeau::Resultat& res, float cx, float cy, float largeur, float k, float a, float e) {
    dessin2d::couronne(r, cx, cy, 0, 260 * k, hex(ACCENT, 0.30f * a), hex(ACCENT, 0));
    const float gros = 76 * k * e, phrase = 24 * k;
    std::string titre, voiture, objet;
    const char* type = "cadeau.type.etoiles";
    switch (res.gain) {
        case cadeau::Gain::Etoiles: titre = "+" + ui::nombre(res.etoiles) + " ★"; break;
        case cadeau::Gain::Coloris: separer(res, voiture, objet); titre = objet; type = "cadeau.type.coloris"; break;
        case cadeau::Gain::Skin: separer(res, voiture, objet); titre = objet; type = "cadeau.type.skin"; break;
        case cadeau::Gain::Voiture: titre = nomVoiture(res.voiture); type = "cadeau.type.voiture"; break;
    }
    float y = cy - 104 * k;
    ui::text(r, cx, y, 14 * k, ui::majuscules(ui::tr(type)), hex(ACCENT2, a), ui::CENTRE, 0.22f, false);
    y += 30 * k;
    if (res.gain == cadeau::Gain::Coloris) {   // pastille de la couleur gagnée, à gauche du nom
        const float tw = std::min(ui::width(gros, ui::majuscules(titre), 0.02f), largeur - 80 * k), rp = 22 * k;
        const float px = cx - tw / 2 - 18 * k - rp, py = y + gros * 0.5f;
        dessin2d::couronne(r, px, py, rp, rp + 12 * k, hex(ACCENT2, 0.45f * a), hex(ACCENT2, 0));
        dessin2d::disqueRadial(r, px, py, rp, px - rp * 0.35f, py - rp * 0.35f, rp * 1.4f, hex(0xFFFFFF, 0.55f * a), hex(teinteColoris(res.objet), a));
        dessin2d::anneau(r, px, py, rp, rp + 2 * k, hex(TEXTE, 0.8f * a));
    }
    {
        std::optional<textes::NomPropre> np;   // nom d'une voiture : inchangé en anglais (RC-E20-03)
        if (res.gain == cadeau::Gain::Voiture) np.emplace();
        texteCase(r, cx, y, largeur, gros * 1.1f, gros, ui::majuscules(titre), res.gain == cadeau::Gain::Etoiles ? hex(ACCENT, a) : hex(TEXTE, a),
                  ui::CENTRE, 0.02f, true);
    }
    y += gros * 1.1f + 18 * k;
    texteCase(r, cx, y, largeur, phrase * 1.3f, phrase, phraseGain(res), hex(TEXTE, 0.92f * a), ui::CENTRE);
    if (res.gain == cadeau::Gain::Voiture) {   // la voiture se garde 24 h avant d'être activée (WFS-502)
        char h1[16], h2[16];
        std::snprintf(h1, sizeof h1, "%g", config().cadeau.activationHeures);
        std::snprintf(h2, sizeof h2, "%g", config().cadeau.pretHeures);
        ui::textBoite(r, cx, y + phrase * 1.6f, largeur, 20 * k, 15 * k, ui::tr("cadeau.voiture.aide", h1, h2), hex(MUTED, a), ui::CENTRE, 0, false);
    }
}

void dessinerAttente(Renderer& r, float cx, float cy, float largeur, float k) {
    const EtatCadeau& c = sauvegarde().cadeau;
    const long long s = secondesAvantProchain(c, heure());
    float y = cy - 80 * k;
    ui::text(r, cx, y, 15 * k, ui::majuscules(ui::tr("cadeau.attente.titre")), hex(MUTED), ui::CENTRE, 0.16f, false);
    y += 30 * k;
    const std::string prochain = s >= 0 ? ui::tr("cadeau.prochain", texteHeures(s)) : ui::tr("cadeau.bientot");
    ui::textBoite(r, cx, y, largeur, 40 * k, 32 * k, prochain, hex(ACCENT2), ui::CENTRE, 0.02f, false);
    y += 72 * k;
    if (g.res) {   // RG-02 : dernier gain réaffiché (cadeau-dernier de la sauvegarde)
        ui::text(r, cx, y, 13 * k, ui::majuscules(ui::tr("cadeau.dernier")), hex(MUTED), ui::CENTRE, 0.16f, false);
        texteCase(r, cx, y + 22 * k, largeur, 30 * k, 22 * k, phraseGain(*g.res), hex(TEXTE), ui::CENTRE);
    }
}

void dessinerChances(Renderer& r, float x0, float y0, float largeur, float k) {
    const ConfigJeu::Cadeau& c = config().cadeau;
    const float lh = 33 * k, tp = 18 * k;
    float y = y0;
    for (const LigneChance& l : lignesChances(c)) {
        r.uiRect(x0, y + lh - 1 * k, largeur, 1 * k, hex(TEXTE, 0.08f));
        texteCase(r, x0, y, largeur - 110 * k, lh, tp, ui::tr(l.cle.c_str(), l.v1, l.v2), hex(TEXTE), ui::GAUCHE);
        ui::text(r, x0 + largeur, y + (lh - tp) / 2, tp, pourcent(l.chance), hex(ACCENT), ui::DROITE, 0, false);
        y += lh;
    }
    y += 12 * k;
    texteCase(r, x0, y, largeur, 24 * k, 16 * k, ui::tr("cadeau.chances.minimum", std::to_string(c.etoilesMin)), hex(INTRO), ui::GAUCHE);
    y += 26 * k;
    ui::textBoite(r, x0, y, largeur, 24 * k, 16 * k, ui::tr("cadeau.chances.garantie", std::to_string(c.garantie)), hex(INTRO), ui::GAUCHE, 0, false);
    y += 26 * k;
    const int n = restantsGarantie(c, sauvegarde().cadeau);
    ui::textBoite(r, x0, y, largeur, 24 * k, 16 * k, n > 1 ? ui::tr("cadeau.chances.restants", std::to_string(n)) : ui::tr("cadeau.chances.restant1"),
                  hex(ACCENT2), ui::GAUCHE, 0, false);
}

} // namespace

// ---------------------------------------------------------------- écran
void ouvrir() {
    Sauvegarde& s = sauvegarde();
    const cadeau::Instant t = heure();
    g.ouvert = true; g.doigt = -1; g.cible = RIEN; g.t0 = -1;
    g.retour = g.chances = g.activer = {};
    if (cadeau::disponible(s.cadeau, t)) {
        // RG-02 : tirage AVANT l'animation, sauvegarde écrite par cadeau::ouvrir (WFS-500, RG-08)
        const std::optional<cadeau::Resultat> r = g.heureBanc ? cadeau::ouvrir(s, t, cadeau::entreeDepuis(s), 501u) : cadeau::ouvrir(s);
        if (r) {
            g.res = g.resultatBanc ? *g.resultatBanc : *r;
            g.vue = Vue::Animation;
            WFS_LOG("cadeau du jour (WFS-501) : écran ouvert, animation d'ouverture, gain %s", g.res->texte().c_str());
            return;
        }
        WFS_ERR("cadeau du jour (WFS-501) : ouverture impossible (sauvegarde), écran d'attente");
    }
    cadeau::Resultat dernier;
    if (cadeau::lireResultat(s.cadeau.dernier, dernier)) g.res = dernier; else g.res.reset();
    if (g.resultatBanc && g.res) g.res = *g.resultatBanc;
    g.vue = Vue::Attente;
    WFS_LOG("cadeau du jour (WFS-501) : écran ouvert sans cadeau (%s), dernier gain %s", cadeau::texteEtat(cadeau::etat(s.cadeau, t)),
            g.res ? g.res->texte().c_str() : "—");
}
void fermer() { if (g.ouvert) WFS_LOG("cadeau du jour (WFS-501) : écran fermé"); g.ouvert = false; g.doigt = -1; g.cible = RIEN; }
bool ouvert() { return g.ouvert; }

void dessiner(Renderer& r, float W, float H, const Marges& m, double temps) {
    if (!g.ouvert || !ui::fontReady()) return;
    const textes::Journal journal;
    g.temps = temps;
    if (g.vue == Vue::Animation && g.t0 < 0) g.t0 = temps;
    const float ta = g.vue == Vue::Animation ? (float)(temps - g.t0) : (float)DUREE;
    if (g.vue == Vue::Animation && ta >= DUREE) g.vue = Vue::Gain;
    const bool cp = ui::petitEcran(W, H);
    const float k = cp ? ui::unitePetitEcran(W, H) : H / 720.f, kb = H / 720.f;
    g.dp = ui::pixelsParDp(W);
    r.uiRect(0, 0, W, H, hex(BG, 0.86f));   // voile : la scène du menu reste devinée derrière
    const float zl = m.gauche + 16 * k, zr = W - m.droite - 16 * k;
    const float pw = std::min(zr - zl, 860 * k), px = zl + (zr - zl - pw) / 2;
    const float py = m.haut + (cp ? 8 : 24) * k, ph = H - py - m.bas - (cp ? 8 : 24) * k;
    r.uiRect(px, py, pw, ph, SURFACE);
    r.uiRect(px, py, pw, 3 * k, hex(ACCENT2));   // liseré néon
    const float x0 = px + 24 * k;
    ui::text(r, x0, py + 14 * k, 12 * k, ui::majuscules(ui::tr("cadeau.surtitre")), hex(ACCENT2), ui::GAUCHE, 0.22f, false);
    const float tt = (cp ? 26 : 34) * k;
    ui::text(r, x0, py + 32 * k, tt, ui::majuscules(ui::tr(g.vue == Vue::Chances ? "cadeau.chances.titre" : "cadeau.titre")), hex(TEXTE), ui::GAUCHE,
             0.02f, false, true);
    // boutons du bas (RC-E19-01 : jamais d'offre payante) ; largeur fixe dans les deux langues (RC-E20-04)
    const float bh = 50 * kb, tp = (cp ? 18 : 20) * k, by = py + ph - 14 * k - bh;
    g.retour = g.chances = g.activer = {};
    if (g.vue != Vue::Animation) {
        // WFS-502 (RG-01) : « Activer » seulement tant que le prêt attend (réaffichage après activation ou perte : « Retour » seul)
        const bool voiture = g.vue == Vue::Gain && g.res && g.res->gain == cadeau::Gain::Voiture && pret::etat(sauvegarde().cadeau) == pret::Etat::EnAttente;
        float xd = px + pw - 24 * k;   // bord droit du prochain bouton
        auto poser = [&](Zone& z, const char* cle) {
            const float w = largeurTexte(tp, cle) + 56 * kb;
            z = {xd - w, by, w, bh}; xd -= w + 14 * k;
        };
        if (voiture) poser(g.activer, "cadeau.activer");
        poser(g.retour, "cadeau.retour");
        if (g.vue != Vue::Chances) poser(g.chances, "cadeau.chances");
        const bool appui = g.doigt >= 0;
        if (g.activer.w > 0) { bouton(r, g.activer, kb, tp, "cadeau.activer", true, appui && g.cible == ACTIVER, largeurTexte(tp, "cadeau.activer")); manette::focus(g.activer); }
        bouton(r, g.retour, kb, tp, "cadeau.retour", !voiture, appui && g.cible == RETOUR, largeurTexte(tp, "cadeau.retour"));
        manette::focus(g.retour, true);
        if (g.chances.w > 0) { bouton(r, g.chances, kb, tp, "cadeau.chances", false, appui && g.cible == CHANCES, largeurTexte(tp, "cadeau.chances")); manette::focus(g.chances); }
    } else ui::text(r, x0, by + (bh - 14 * k) / 2, 14 * k, ui::tr("cadeau.aide.passer"), hex(MUTED), ui::GAUCHE, 0.02f, false);
    // contenu, entre le titre et les boutons
    const float haut = py + 32 * k + tt + 12 * k, bas = by - 12 * k, cx = px + pw / 2, cy = (haut + bas) / 2, largeur = pw - 48 * k;
    switch (g.vue) {
        case Vue::Animation: {
            const float fin = std::clamp((ta - 1.15f) / 0.25f, 0.f, 1.f), aBoite = 1 - fin;   // la boîte descend et s'efface, le gain monte
            if (aBoite > 0) dessinerBoite(r, cx, cy + 110 * k + fin * 90 * k, k, ta, aBoite);
            if (ta > 1.2f && g.res) { const float e = ease((ta - 1.2f) / 0.3f); dessinerGain(r, *g.res, cx, cy, largeur, k, e, 0.8f + 0.2f * e); }
            break;
        }
        case Vue::Gain: if (g.res) dessinerGain(r, *g.res, cx, cy, largeur, k, 1, 1); break;
        case Vue::Attente: dessinerAttente(r, cx, cy, largeur, k); break;
        case Vue::Chances: dessinerChances(r, x0, haut + 6 * k, pw - 48 * k, k); break;
    }
}

bool toucher(int id, float x, float y, bool down) {
    if (!g.ouvert) return false;
    const int sous = cibleSous(x, y);
    if (down) {
        if (g.doigt < 0) {
            g.doigt = id; g.cible = sous;
            if (g.vue == Vue::Animation) {   // RG-02 : un appui passe l'animation (le cadeau est déjà tiré : rien n'est redonné)
                g.vue = Vue::Gain; g.cible = RIEN;
                WFS_LOG("cadeau du jour (WFS-501) : animation passée");
            }
        } else if (id == g.doigt && sous != g.cible) g.cible = RIEN;   // glissé hors du bouton : annulé
        return true;
    }
    if (id != g.doigt) return true;
    const int c = g.cible;
    g.doigt = -1; g.cible = RIEN;
    if (c != sous) return true;
    if (c == RETOUR) {
        if (g.vue == Vue::Chances) { g.vue = g.avantChances; WFS_LOG("cadeau du jour (WFS-501) : « Retour » → %s", g.vue == Vue::Gain ? "gain" : "attente"); }
        else { WFS_LOG("cadeau du jour (WFS-501) : « Retour » → menu"); fermer(); }
    } else if (c == CHANCES) { g.avantChances = g.vue; g.vue = Vue::Chances; WFS_LOG("cadeau du jour (WFS-501) : « Chances »"); }
    else if (c == ACTIVER) {   // WFS-502 (RG-02) : le prêt de 3 h commence, la sauvegarde est écrite
        Sauvegarde& s = sauvegarde();
        if (pret::activer(s, pret::maintenant())) s.enregistrer();
        WFS_LOG("cadeau du jour : « Activer » : voiture prêtée %s → menu", pret::etat(s.cadeau) == pret::Etat::Actif ? "activée" : "pas activée (aucun prêt en attente)");
        fermer();
    }
    return true;
}

// .cam-btn de « Crédits » (credits.cpp) : vitré, bordure 1 px, biais 8 px, 44 px de haut ; texte cyan 15 px ; posé au-dessus de « Crédits »
void dessinerBouton(Renderer& r, float W, float H, const Marges& m, bool presse) {
    if (!ui::fontReady()) return;
    const textes::Journal journal;
    const bool cp = ui::petitEcran(W, H);
    const float k = cp ? ui::unitePetitEcran(W, H) : H / 720.f, kb = H / 720.f;
    const float h = 44 * kb, tp = 15 * k, pad = 18 * kb;
    const std::string t = ui::majuscules(ui::tr("cadeau.bouton"));
    const float tw = largeurTexte(tp, "cadeau.bouton");
    const float w = pad + tw + pad, x = W - m.droite - 16 * k - w, y = H - m.bas - 14 * k - h - 10 * k - h;   // « Crédits » : H − bas − 14 − h
    g.bouton = {x, y, w, h};
    g.dpBouton = ui::pixelsParDp(W);
    const cadeau::Instant ti = heure();
    const EtatCadeau& c = sauvegarde().cadeau;
    const bool dispo = cadeau::disponible(c, ti);
    dessin2d::biseau(r, x - kb, y - kb, w + 2 * kb, h + 2 * kb, 8 * kb, dessin2d::uni(dispo ? hex(ACCENT, 0.7f) : BORD));
    dessin2d::biseau(r, x, y, w, h, 8 * kb, dessin2d::uni(GLASS));
    if (presse) dessin2d::biseau(r, x, y, w, h, 8 * kb, dessin2d::uni(hex(ACCENT2, 0.22f)));
    ui::textBoite(r, x + pad + tw / 2, y, tw, h, tp, t, hex(ACCENT2), ui::CENTRE, 0.12f, false);
    if (dispo) {   // pastille : un cadeau attend
        const float rp = 7 * kb, pxp = x + w - 2 * kb, pyp = y + 2 * kb;
        dessin2d::couronne(r, pxp, pyp, rp, rp + 9 * kb, hex(ACCENT, 0.55f), hex(ACCENT, 0));
        dessin2d::disqueRadial(r, pxp, pyp, rp, pxp - rp * 0.3f, pyp - rp * 0.3f, rp * 1.3f, hex(0xFFC08A), hex(ACCENT));
    } else {   // RG-01 : « Prochain cadeau dans hh:mm » (ou « bientôt » si l'heure a reculé), à gauche du bouton
        const long long s = secondesAvantProchain(c, ti);
        const std::string p = s >= 0 ? ui::tr("cadeau.prochain", texteHeures(s)) : ui::tr("cadeau.bientot");
        ui::text(r, x - 12 * k, y + (h - 14 * k) / 2, 14 * k, p, hex(MUTED), ui::DROITE, 0.02f, true);
    }
    manette::focus(g.bouton);
}
bool dansBouton(float x, float y) { return g.bouton.w > 0 && ecrans::appui::dansCible(g.bouton.x, g.bouton.y, g.bouton.w, g.bouton.h, g.dpBouton, x, y); }

namespace detail {   // accès du banc (cadeau_jour_banc.cpp)
void forcerHeure(const cadeau::Instant* t) { if (t) g.heureBanc = *t; else g.heureBanc.reset(); }
void forcerResultat(const cadeau::Resultat* r) {
    if (r) g.resultatBanc = *r; else g.resultatBanc.reset();
    if (r && g.ouvert && g.res) g.res = *r;
}
void forcerFinAnimation() { if (g.vue == Vue::Animation) g.vue = Vue::Gain; }
int vue() { return (int)g.vue; }   // 0 animation, 1 gain, 2 attente, 3 chances
bool centre(const char* nom, float& x, float& y) {
    const std::string n = nom;
    const Zone& z = n == "retour" ? g.retour : n == "chances" ? g.chances : n == "activer" ? g.activer : g.bouton;
    if (z.w <= 0) return false;
    x = z.x + z.w / 2; y = z.y + z.h / 2; return true;
}
} // namespace detail

namespace {
// système : panneau par-dessus le menu (450), le bandeau des paquets (458) et les Réglages (460), sous les Crédits (470, jamais ouverts ensemble)
class CadeauJour : public System {
public:
    const char* name() const override { return "cadeau-jour"; }
    void onEcran(Ecran, Ecran, const GameView&) override { fermer(); }
    void drawUi(Renderer& r, const GameView& v) override {
        if (v.ecran == Ecran::Menu && !v.sceneDediee) dessiner(r, v.uiWidth, v.uiHeight, v.marges, v.time);
    }
    void shutdown(Renderer&) override { fermer(); }
};
} // namespace

} // namespace ecrans::cadeau_jour

WFS_SYSTEM(ecrans::cadeau_jour::CadeauJour, 466)

} // namespace wfs
