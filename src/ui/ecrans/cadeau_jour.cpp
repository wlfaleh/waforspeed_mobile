// WaForSpeed — écran du cadeau du jour (WFS-501) : voir cadeau_jour.h.
#include "cadeau_jour.h"
#include "../jauges.h"   // WFS-550 (RG-03) : anneau du temps restant
#include "../verre.h"    // WFS-550 : pilule de verre
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <optional>
#include "../../core/configgame.h"   // WFS-550 : 30e cadeau garanti
#include "reglages_appui.h"   // appui::dansCible : cible de 48 dp (comme les Réglages, les CGU et les Crédits)
#include "textes_banc.h"      // banc « textes » (journal des textes de cet écran)
#include "../garage_achat.h"   // étoile dessinée (la police n'a pas le caractère ★) et texte avec ★ (WFS-489)
#include "../langue.h"
#include "../petit_ecran.h"
#include "../text.h"
#include "../boutons.h"   // WFS-549 : boutons au modèle R19
#include "../../game/boutique.h"   // WFS-557 : solde d'étoiles (maquette 05)
#include <array>
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
// WFS-557 (ordre de l'utilisateur du 2026-11-16 : maquettes R19 à 100 %, maquette 05 mesurée sur l'image 2400 × 1080) ; unités de
// la maquette ramenée à 1600 × 720 (k), origine (ox, oy) du cadre centré
struct Cadre05 { float ox = 0, oy = 0, k = 1; float X(float u) const { return ox + u * k; } float Y(float u) const { return oy + u * k; } };
// étoile orange au liseré chrome (gros « +23 ★ » de la maquette 05)
void etoileChrome(Renderer& r, float cx, float cy, float R, float k, float a) {
    garage::etoile(r, cx + 3 * k, cy + 5 * k, R + 3 * k, hex(0x1A0E06, 0.55f * a));   // ombre portée
    garage::etoile(r, cx, cy, R + 4.5f * k, hex(0xB9BCC2, a));                       // liseré chrome
    garage::etoile(r, cx, cy, R + 2 * k, hex(0x3A3C42, a));
    garage::etoile(r, cx, cy, R, hex(0xF98E2E, a));
}
// gros texte du gain (Rajdhani gras italique 240, orange, ombre brune décalée), centré en x, ligne de base `base`
void grosTexte(Renderer& r, float cx, float base, float px, const std::string& t, const vec4& c, float k, float a) {
    ui::text(r, cx + 4 * k, base + 6 * k - 0.7288f * px, px, t, hex(0x2A1606, 0.6f * a), ui::CENTRE, 0, false, true);
    ui::text(r, cx, base - 0.7288f * px, px, t, c, ui::CENTRE, 0, false, true);
}
// gain (RG-03) au modèle de la maquette 05 : gros « +23 ★ » au-dessus de la scène, panneau « TU AS GAGNÉ » en haut à droite ;
// a : opacité, e : agrandissement d'entrée (0,8 → 1)
void dessinerGain(Renderer& r, const cadeau::Resultat& res, const Cadre05& f, float a, float e) {
    const float k = f.k;
    std::string titre, voiture, objet;
    switch (res.gain) {
        case cadeau::Gain::Etoiles: titre = "+" + ui::nombre(res.etoiles); break;
        case cadeau::Gain::Coloris: separer(res, voiture, objet); titre = objet; break;
        case cadeau::Gain::Skin: separer(res, voiture, objet); titre = objet; break;
        case cadeau::Gain::Voiture: titre = nomVoiture(res.voiture); break;
    }
    // gros gain : « +23 » de 586,7 à 856,7, étoile de rayon 76,7 centrée en (970, 196,7), ligne de base à 256,7
    dessin2d::couronne(r, f.X(815), f.Y(196), 0, 330 * k, hex(0xFF8A3D, 0.22f * a), hex(0xFF8A3D, 0));
    if (res.gain == cadeau::Gain::Etoiles) {
        const float px = 240 * k * e, R = 76.7f * k * e, tw = ui::width(px, titre, 0), gap = 37 * k * e, total = tw + gap + 2 * R;
        const float x0 = f.X(815) - total / 2;
        grosTexte(r, x0 + tw / 2, f.Y(256.7f), px, titre, hex(0xF88F33, a), k, a);
        etoileChrome(r, x0 + tw + gap + R, f.Y(196.7f), R, k, a);
    } else {
        std::optional<textes::NomPropre> np;   // nom d'une voiture : inchangé en anglais (RC-E20-03)
        if (res.gain == cadeau::Gain::Voiture) np.emplace();
        const std::string t = ui::majuscules(titre);
        const float px = std::min(120 * k, 1100 * k / std::max(1.f, ui::width(1, t, 0.02f))) * e;
        grosTexte(r, f.X(815), f.Y(236), px, t, res.gain == cadeau::Gain::Coloris ? hex(teinteColoris(res.objet), a) : hex(0xF88F33, a), k, a);
    }
    // panneau « TU AS GAGNÉ » : (1288, 110,7) 272 × 93,3, verre sombre
    const float px0 = f.X(1288), py0 = f.Y(110.7f), pw = 272 * k, ph = 93.3f * k;
    r.uiRect(px0, py0, pw, ph, hex(0x18191E, 0.86f * a));
    dessin2d::contour(r, std::array<vec2, 4>{vec2{px0, py0}, {px0 + pw, py0}, {px0 + pw, py0 + ph}, {px0, py0 + ph}}.data(), 4, 1 * k, hex(TEXTE, 0.10f * a));
    {
        ui::AvecPolice inter(ui::Police::InterMedium);
        const float p = 12 * k;
        ui::text(r, px0 + 16.7f * k, py0 + 18.7f * k - 0.8006f * p, p, ui::majuscules(ui::tr("cadeau.gagne")), hex(0xB8BCC8, a), ui::GAUCHE, 0.2f, false);
    }
    const float lp = 39.7f * k, base2 = py0 + 52.7f * k, lw = pw - 16.7f * k - 12 * k;
    std::string ligne;
    float xl = px0 + 16.7f * k;
    if (res.gain == cadeau::Gain::Etoiles) {
        garage::etoile(r, f.X(1323.3f), f.Y(151.3f), 15.3f * k, hex(ACCENT, a));
        xl = f.X(1344.7f);
        ligne = ui::majuscules(ui::tr("cadeau.etoiles_n", ui::nombre(res.etoiles)));
    } else ligne = ui::majuscules(ui::tr(res.gain == cadeau::Gain::Voiture ? "cadeau.type.voiture" : res.gain == cadeau::Gain::Skin ? "cadeau.type.skin" : "cadeau.type.coloris"));
    const float lpx = std::min(lp, lp * (px0 + pw - 12 * k - xl) / std::max(1.f, ui::width(lp, ligne, 0)));
    ui::text(r, xl, base2 - 0.7288f * lpx, lpx, ligne, hex(TEXTE, a), ui::GAUCHE, 0, false, true);
    {
        ui::AvecPolice inter(ui::Police::Inter);
        const long long s = secondesAvantProchain(sauvegarde().cadeau, heure());
        const std::string p3 = res.gain == cadeau::Gain::Etoiles ? (s >= 0 ? ui::tr("cadeau.prochain", texteHeures(s)) : ui::tr("cadeau.bientot")) : phraseGain(res);
        const float p = 14.4f * k;
        texteCase(r, px0 + 16.7f * k, py0 + 74 * k - 0.8006f * p, lw, p, p, p3, hex(0xC8CBD2, a), ui::GAUCHE);
    }
}
// autocollant cyan « CHANCES » de la maquette 05 : (78, 595) 170 × 67 incliné de −4°, liseré blanc, texte sombre
void autocollantChances(Renderer& r, const Zone& z, float k, bool presse, int nGains) {
    const float a = -4 * PI / 180, cx = z.x + z.w / 2, cy = z.y + z.h / 2, s = presse ? 0.96f : 1.f;
    vec2 q[4], b[4], o[4];
    rectTourne(q, cx - z.w / 2 * s, cy - z.h / 2 * s, z.w * s, z.h * s, cx, cy, a);
    rectTourne(b, cx - z.w / 2 * s - 3 * k, cy - z.h / 2 * s - 3 * k, z.w * s + 6 * k, z.h * s + 6 * k, cx, cy, a);
    rectTourne(o, cx - z.w / 2 * s + 2 * k, cy - z.h / 2 * s + 6 * k, z.w * s + 4 * k, z.h * s + 4 * k, cx, cy, a);
    poly(r, o, 4, dessin2d::uni({0, 0, 0, 0.35f}));                                 // ombre
    poly(r, b, 4, dessin2d::uni(hex(0xF4F8F9)));                                   // liseré blanc
    poly(r, q, 4, {hex(presse ? 0x5FC2CC : 0x8EE6EE), hex(presse ? 0x2FA8B8 : 0x48C8D8), std::min(q[0].y, q[1].y), std::max(q[2].y, q[3].y)});
    const float c = std::cos(a), sn = std::sin(a);
    auto P = [&](float dx, float dy) { return vec2{cx + dx * c - dy * sn, cy + dx * sn + dy * c}; };
    // texte : on ne tourne pas les glyphes ; ils suivent la pente par leur point de départ (−4° : écart d'un pixel)
    const vec2 t1 = P(-z.w / 2 + 12 * k, -z.h / 2 + 7 * k);
    ui::text(r, t1.x, t1.y, 37 * k, ui::majuscules(ui::tr("cadeau.chances")), hex(0x0F2A30), ui::GAUCHE, 0.02f, false, true);
    const vec2 t2 = P(-z.w / 2 + 12 * k, -z.h / 2 + 46 * k);
    ui::AvecPolice inter(ui::Police::InterMedium);
    ui::text(r, t2.x, t2.y, 11.5f * k, ui::tr("cadeau.gains_possibles", std::to_string(nGains)), hex(0x0F2A30), ui::GAUCHE, 0, false);
}
// pastille du solde de la maquette 05 : parallélogramme sombre (1459,3 ; 26) 110 × 40, biais 5,7 ; étoile orange, chiffres italiques
void soldeIncline(Renderer& r, const Cadre05& f) {
    const float k = f.k, x = f.X(1459.3f), y = f.Y(26), w = 110 * k, h = 40 * k;
    const std::string nb = ui::nombre(boutique::solde(sauvegarde()));
    const float tp = 26.5f * k, wt = ui::width(tp, nb, 0), ww = std::max(w, 48 * k + wt + 14 * k), xx = x + w - ww;
    dessin2d::biseau(r, xx - 1 * k, y - 1 * k, ww + 2 * k, h + 2 * k, 5.7f * k, dessin2d::uni(hex(TEXTE, 0.20f)));
    dessin2d::biseau(r, xx, y, ww, h, 5.7f * k, dessin2d::uni(hex(0x15171D, 0.92f)));
    garage::etoile(r, xx + 33.4f * k, f.Y(45), 11 * k, hex(ACCENT));
    ui::text(r, xx + 48 * k, f.Y(52.7f) - 0.7288f * tp, tp, nb, hex(TEXTE), ui::GAUCHE, 0, false, true);
}
// attente (cadeau déjà ouvert) : compte à rebours en grand au centre, dernier gain dans le panneau de droite
void dessinerAttente(Renderer& r, const Cadre05& f) {
    const float k = f.k;
    const long long s = secondesAvantProchain(sauvegarde().cadeau, heure());
    {
        ui::AvecPolice inter(ui::Police::InterMedium);
        ui::text(r, f.X(815), f.Y(150), 15 * k, ui::majuscules(ui::tr("cadeau.attente.titre")), hex(0xB8BCC8), ui::CENTRE, 0.2f, false);
    }
    const std::string t = s >= 0 ? texteHeures(s) : ui::tr("cadeau.bientot");
    grosTexte(r, f.X(815), f.Y(290), s >= 0 ? 130 * k : 60 * k, t, hex(TEXTE), k, 1);
    if (g.res) {
        const float px0 = f.X(1288), py0 = f.Y(110.7f), pw = 272 * k, ph = 93.3f * k;
        r.uiRect(px0, py0, pw, ph, hex(0x18191E, 0.86f));
        ui::AvecPolice inter(ui::Police::InterMedium);
        ui::text(r, px0 + 16.7f * k, py0 + 18.7f * k - 0.8006f * 12 * k, 12 * k, ui::majuscules(ui::tr("cadeau.dernier")), hex(0xB8BCC8), ui::GAUCHE, 0.2f, false);
        texteCase(r, px0 + 16.7f * k, py0 + 34 * k, pw - 30 * k, 40 * k, 16 * k, phraseGain(*g.res), hex(TEXTE), ui::GAUCHE);
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
    g.dp = ui::pixelsParDp(W);
    // WFS-557 (ordre de l'utilisateur du 2026-11-16 : maquette 05 à 100 %) : la scène 3D du parking reste visible (voile léger),
    // bandeau noir de 86,7 en haut avec le titre (Rajdhani gras italique 53), solde incliné en haut à droite, gain au-dessus de la
    // scène, panneau « TU AS GAGNÉ », bouton rond « PRENDRE » (1436,7 ; 578) de rayon 78, autocollant cyan « CHANCES » en bas à gauche
    Cadre05 f;
    f.k = std::min(H / 720.f, (W - m.gauche - m.droite) / 1600.f);
    f.ox = m.gauche + (W - m.gauche - m.droite - 1600 * f.k) / 2; f.oy = m.haut + (H - m.haut - m.bas - 720 * f.k) / 2;
    const float k = f.k;
    r.uiRect(0, 0, W, H, hex(BG, g.vue == Vue::Chances ? 0.55f : 0.12f));
    r.uiRect(0, 0, W, f.Y(86.7f), {0, 0, 0, 0.92f});
    ui::text(r, f.X(43.3f), f.Y(66.7f) - 0.7288f * 53 * k, 53 * k, ui::majuscules(ui::tr(g.vue == Vue::Chances ? "cadeau.chances.titre" : "cadeau.titre")),
             hex(TEXTE), ui::GAUCHE, 0.02f, false, true);
    soldeIncline(r, f);
    g.retour = g.chances = g.activer = {};
    const bool appui = g.doigt >= 0;
    // bouton rond (« PRENDRE » sur un gain, « RETOUR » ailleurs) ; voiture prêtée en attente : « PRENDRE » active le prêt, et
    // « RETOUR » (autocollant) la garde pour plus tard (WFS-502, RG-01)
    const float rb = 78 * k, bcx = f.X(1436.7f), bcy = f.Y(578);
    if (g.vue != Vue::Animation) {
        const bool voiture = g.vue == Vue::Gain && g.res && g.res->gain == cadeau::Gain::Voiture && pret::etat(sauvegarde().cadeau) == pret::Etat::EnAttente;
        Zone& rond = voiture ? g.activer : g.retour;
        rond = {bcx - rb, bcy - rb, 2 * rb, 2 * rb};
        const char* cle = g.vue == Vue::Gain ? "cadeau.prendre" : "cadeau.retour";
        ui::boutons::rondMenu(r, bcx, bcy, k * 78 / 117, ui::majuscules(ui::tr(cle)), "", 1, appui && g.cible == (voiture ? ACTIVER : RETOUR));
        manette::focus(rond, true);
        if (voiture) {
            const float tp = 20 * k, w = largeurTexte(tp, "cadeau.retour") + 56 * k;
            g.retour = {bcx - rb - 24 * k - w, bcy - 25 * k, w, 50 * k};
            ui::boutons::autocollant(r, g.retour.x, g.retour.y, g.retour.w, g.retour.h, ui::majuscules(ui::tr("cadeau.retour")), 1, appui && g.cible == RETOUR, tp);
            manette::focus(g.retour);
        }
        if (g.vue != Vue::Chances) {
            g.chances = {f.X(78), f.Y(595), 170 * k, 67 * k};
            autocollantChances(r, g.chances, k, appui && g.cible == CHANCES, (int)lignesChances(config().cadeau).size());
            manette::focus(g.chances);
        }
    } else {
        ui::AvecPolice inter(ui::Police::Inter);
        ui::text(r, f.X(815), f.Y(660), 14 * k, ui::tr("cadeau.aide.passer"), hex(MUTED), ui::CENTRE, 0.02f, false);
    }
    switch (g.vue) {
        case Vue::Animation: {
            const float fin = std::clamp((ta - 1.15f) / 0.25f, 0.f, 1.f), aBoite = 1 - fin;   // la boîte descend et s'efface, le gain monte
            if (aBoite > 0) dessinerBoite(r, f.X(815), f.Y(560) + fin * 90 * k, k, ta, aBoite);
            if (ta > 1.2f && g.res) { const float e = ease((ta - 1.2f) / 0.3f); dessinerGain(r, *g.res, f, e, 0.8f + 0.2f * e); }
            break;
        }
        case Vue::Gain: if (g.res) dessinerGain(r, *g.res, f, 1, 1); break;
        case Vue::Attente: dessinerAttente(r, f); break;
        case Vue::Chances: {
            const float px = f.X(300), py = f.Y(130), pw = 1000 * k, ph = 400 * k;
            verre::panneau(r, px, py, pw, ph, {18 * k, 1.5f * k});
            dessinerChances(r, px + 30 * k, py + 26 * k, pw - 60 * k, k);
            break;
        }
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
// WFS-550 (maquettes 05 et 26) : petit cadeau en traits fins (boîte, couvercle, ruban, nœud), taille s, centré en (cx, cy)
static void iconeCadeau(Renderer& r, float cx, float cy, float s, const vec4& c) {
    const float ep = std::max(1.2f, s * 0.08f), hw = s * 0.40f, hb = s * 0.32f, hc = s * 0.16f, y0 = cy - s * 0.10f;
    const vec2 boite[5] = {{cx - hw * 0.88f, y0 + hc}, {cx - hw * 0.88f, y0 + hc + hb}, {cx + hw * 0.88f, y0 + hc + hb},
                           {cx + hw * 0.88f, y0 + hc}, {cx - hw * 0.88f, y0 + hc}};
    dessin2d::trait(r, boite, 5, ep, c);
    const vec2 couvercle[5] = {{cx - hw, y0}, {cx + hw, y0}, {cx + hw, y0 + hc}, {cx - hw, y0 + hc}, {cx - hw, y0}};
    dessin2d::trait(r, couvercle, 5, ep, c);
    const vec2 ruban[2] = {{cx, y0}, {cx, y0 + hc + hb}};
    dessin2d::trait(r, ruban, 2, ep, c);
    // nœud : deux boucles au-dessus du couvercle
    const vec2 g[4] = {{cx, y0}, {cx - s * 0.20f, y0 - s * 0.16f}, {cx - s * 0.06f, y0 - s * 0.20f}, {cx, y0}};
    const vec2 d[4] = {{cx, y0}, {cx + s * 0.20f, y0 - s * 0.16f}, {cx + s * 0.06f, y0 - s * 0.20f}, {cx, y0}};
    dessin2d::trait(r, g, 4, ep, c);
    dessin2d::trait(r, d, 4, ep, c);
}

// WFS-550 (maquettes 01, 05, 26 ; ordre de l'utilisateur du 2026-11-15 : la maquette à 100 %) : texte « 07:42:13 » (heures,
// minutes, secondes) du compte à rebours
static std::string texteHMS(long long s) {
    s = std::clamp(s, 0LL, 86400LL);
    char b[16];
    std::snprintf(b, sizeof b, "%02lld:%02lld:%02lld", s / 3600, (s / 60) % 60, s % 60);
    return b;
}

// carte « Cadeau du jour » de la maquette (verre arrondi en pilule, anneau gradué cyan → orange autour du cadeau, « CADEAU DU
// JOUR », compte à rebours en grand avec les secondes en orange, « avant le prochain · 30e garanti dans 12 j ») ; toute la carte
// est le bouton. Mesures de la maquette 26 (1600 × 720) : 350 × 118, anneau de rayon 37 à 54 du bord gauche, texte à 117.
// Elle remplace le bouton « Cadeau du jour », en bas à droite comme la maquette 01 (arbitrage du PO, WFS-550 RG-11).
void placeCarte(float W, float H, const Marges& m, float& droite, float& haut) {
    const float kb = H / 720.f;
    droite = W - m.droite - 28 * kb;           // maquette 01 : collée au bord droit (28 px), en bas (20 px)
    haut = H - m.bas - 20 * kb - 118 * kb;
}

void dessinerBouton(Renderer& r, float W, float H, const Marges& m, bool presse) {
    if (!ui::fontReady()) return;
    const textes::Journal journal;
    const float kb = H / 720.f;   // carte et textes à l'échelle de la maquette (720 px de haut)
    float droite, haut;
    placeCarte(W, H, m, droite, haut);
    const float w = 350 * kb, h = 118 * kb, x = droite - w, y = haut;
    g.bouton = {x, y, w, h};
    g.dpBouton = ui::pixelsParDp(W);
    const cadeau::Instant ti = heure();
    const EtatCadeau& c = sauvegarde().cadeau;
    const bool dispo = cadeau::disponible(c, ti);
    verre::Style st; st.rayon = h / 2; st.lisere = std::max(1.f, kb); st.bord = dispo ? ACCENT : 0;
    verre::panneau(r, x, y, w, h, st);
    if (presse) {   // appuyé : voile cyan dans la carte
        vec2 q[verre::NP_CONTOUR];
        verre::contourArrondi(x, y, w, h, h / 2, q);
        dessin2d::polygone(r, q, verre::NP_CONTOUR, dessin2d::uni(hex(ACCENT2, 0.18f)));
    }
    // anneau : fond sombre, piste, remplissage de l'attente (point de départ cyan), graduations autour, cadeau au centre
    const float cx = x + 54 * kb, cy = y + h / 2, ra = 37 * kb;
    const long long s = dispo ? 0 : secondesAvantProchain(c, ti);
    dessin2d::disqueRadial(r, cx, cy, ra - 4 * kb, cx, cy - ra * 0.3f, ra, hex(0x1A1C24, 0.95f), hex(0x0E0F14, 0.95f));
    const float f = dispo ? 1.f : s >= 0 ? jauges::fractionAttente(s, 86400) : 1.f;
    jauges::anneauGradue(r, cx, cy, ra, 6 * kb, f, 1, jauges::PI_J * 1.25f, 24);   // départ en bas à gauche (maquette)
    iconeCadeau(r, cx, cy, 26 * kb, hex(TEXTE, 0.95f));
    // textes
    const float tx = x + 117 * kb;
    verre::surTitre(r, tx, y + 21 * kb, 13 * kb, ui::tr("cadeau.bouton"));   // tailles de la maquette 26 (720 px de haut)
    if (dispo) {
        ui::text(r, tx, y + 38 * kb, 36 * kb, ui::majuscules(ui::tr("cadeau.carte.dispo")), hex(ACCENT), ui::GAUCHE, 0, false, true);
        ui::text(r, tx, y + 84 * kb, 13 * kb, ui::tr("cadeau.carte.dispo.aide"), hex(MUTED), ui::GAUCHE, 0, false);
        // pastille : un cadeau attend
        const float rp = 7 * kb, pxp = x + w - h * 0.22f, pyp = y + h * 0.18f;
        dessin2d::couronne(r, pxp, pyp, rp, rp + 9 * kb, hex(ACCENT, 0.55f), hex(ACCENT, 0));
        dessin2d::disqueRadial(r, pxp, pyp, rp, pxp - rp * 0.3f, pyp - rp * 0.3f, rp * 1.3f, hex(0xFFC08A), hex(ACCENT));
    } else if (s >= 0) {
        // « 07:42:13 » : heures et minutes blanches, deux-points gris, secondes orange (maquette)
        const std::string t = texteHMS(s), hh = t.substr(0, 2), mm = t.substr(3, 2), ss = t.substr(6, 2);
        const float tp = 44 * kb, ty = y + 36 * kb;
        float xx = tx;
        auto morceau = [&](const std::string& m2, const vec4& col) { ui::text(r, xx, ty, tp, m2, col, ui::GAUCHE, 0, false, true); xx += ui::width(tp, m2); };
        morceau(hh, hex(TEXTE)); morceau(":", hex(MUTED)); morceau(mm, hex(TEXTE)); morceau(":", hex(MUTED)); morceau(ss, hex(0xFF9A4D));
        // « avant le prochain · 30e garanti dans 12 j » (gris, puis blanc)
        const std::string a1 = ui::tr("cadeau.carte.avant") + " · ";
        const int n = restantsGarantie(config().cadeau, c);
        ui::text(r, tx, y + 84 * kb, 13 * kb, a1, hex(MUTED), ui::GAUCHE, 0, false);
        ui::text(r, tx + ui::width(13 * kb, a1), y + 84 * kb, 13 * kb, ui::tr("cadeau.carte.garanti", std::to_string(config().cadeau.garantie), std::to_string(std::max(1, n))), hex(TEXTE), ui::GAUCHE, 0, false);
    } else {   // l'heure du téléphone a reculé : « Prochain cadeau bientôt »
        ui::text(r, tx, y + 48 * kb, 18 * kb, ui::tr("cadeau.bientot"), hex(TEXTE), ui::GAUCHE, 0, false, true);
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
