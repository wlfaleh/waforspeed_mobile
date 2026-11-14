// WaForSpeed — écran « Crée ton pilote » (WFS-481) : voir creation_profil.h. Saisie : creation_profil_saisie.h.
#include "creation_profil.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "creation_profil_saisie.h"
#include "../../core/log.h"
#include "../../core/system.h"
#include "../../game/course.h"
#include "../../game/course_system.h"
#include "../../game/commandes_tactiles.h"   // dessin2d (biseau, contour, trait)
#include "../../game/game.h"
#include "../../game/sauvegarde.h"
#include "../../platform/manette.h"   // manette : seules les zones de cet écran ont le focus
#include "../../render/renderer.h"
#include "../langue.h"
#include "../petit_ecran.h"
#include "../text.h"

namespace wfs {
namespace ecrans {
namespace {

using creation::Champ;
using creation::Saisie;
using creation::Touche;

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
// design-street-racing : fond, surfaces, accent orange (action), cyan (sélection), rouge (erreur), textes
const unsigned BG = 0x0A0B10, SURFACE = 0x14161F, SURFACE2 = 0x1E2130, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, DANGER = 0xFF1F4B,
               TEXTE = 0xF2F4F8, MUTED = 0x8A90A6;
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f};

// RG-03 : touche de 57 dp (9,05 mm à 160 dp par pouce ; S21 FE, 2,625 px/dp à 401 ppp : 150 px = 9,5 mm), écart 4 dp
constexpr float TOUCHE_DP = 57, ECART_DP = 4, MARGE_DP = 16, COLONNE_MIN_DP = 200;

struct Zone {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};

struct Etat {
    bool ouvert = false;
    Saisie saisie;
    Zone zPseudo, zPlaque, zValider, zAnnuler, zTouches[creation::NB_TOUCHES];
    bool modification = false;   // WFS-482 (RG-07) : ouvert par « Modifier » de l'écran Profil (profil déjà créé)
    int presse = -1;           // touche sous le doigt (retour visuel)
    double tPresse = -1;       // heure de l'appui (la touche s'éclaire 120 ms)
    double tOuvert = 0;        // heure d'ouverture (fondu d'entrée de 250 ms)
    double maintenant = 0;
    bool valideNote = false;   // RG-07 : profil écrit (l'écran se ferme à l'image suivante)
    bool pointsNotes = false;  // points d'appui écrits au journal (une fois par ouverture)
    creation::FiltreAppuis filtre;   // WFS-545 : un appui = un caractère, 150 ms après chaque caractère
};
Etat g_e;

// RG-01 : écran à montrer : sauvegarde active (le banc d'essai sans WFS_BANC=creation ne la charge pas) et profil=0 ;
// WFS-482 : ou modification demandée par l'écran Profil
bool g_modifDemandee = false;
bool aMontrer(const GameView& g) {
    if (g.ecran != Ecran::Menu) g_modifDemandee = false;   // menu quitté : la modification est abandonnée (rien d'écrit)
    return g.ecran == Ecran::Menu && sauvegarde().actif() && (!sauvegarde().profil.cree || g_modifDemandee);
}

// ouverture / fermeture selon la sauvegarde, appelée par les deux systèmes (touchers et dessin)
void synchroniser(const GameView& g) {
    const bool montrer = aMontrer(g);
    if (montrer && !g_e.ouvert) {
        const Sauvegarde& s = sauvegarde();
        const std::string propose = Profil::pseudoPropose(s.nick);
        const uint32_t graine = (uint32_t)std::chrono::steady_clock::now().time_since_epoch().count() | 1u;
        g_e.modification = s.profil.cree;   // WFS-482 : profil déjà créé → modification (pseudo et plaque actuels)
        if (g_e.modification) g_e.saisie.modifier(s.nick, s.profil.plaque);
        else g_e.saisie.ouvrir(propose, graine);
        g_e.ouvert = true; g_e.presse = -1; g_e.tOuvert = g.time; g_e.valideNote = g_e.pointsNotes = false;
        g_e.filtre.oublier();
        WFS_LOG("création du profil (WFS-481) : écran ouvert (%s, pseudo proposé « %s », plaque proposée « %s »)", ui::langue::code(),
                propose.c_str(), g_e.saisie.plaque().c_str());
    } else if (!montrer && g_e.ouvert) {
        g_e.ouvert = false;
        WFS_LOG("création du profil (WFS-481) : écran fermé (écran %s, profil=%d)", nomEcran(g.ecran), sauvegarde().profil.cree ? 1 : 0);
    }
}

// RG-07 : « C'est parti » : profil écrit (pseudo dans nick, plaque, profil=1) et enregistré ; le menu revient à l'image suivante
void valider() {
    Sauvegarde& s = sauvegarde();
    std::string nick;
    if (!s.profil.creer(g_e.saisie.pseudo(), g_e.saisie.plaque(), nick)) return;
    s.nick = nick;
    s.enregistrer();
    g_e.valideNote = true;
    g_modifDemandee = false;   // WFS-482 : modification écrite, retour à l'écran Profil
    WFS_LOG("création du profil (WFS-481) : « %s » : pseudo « %s », plaque « %s » enregistrés → menu", ui::tr("creation.valider").c_str(),
            s.nick.c_str(), s.profil.plaque.c_str());
}

// RG-03 : appui (au poser du doigt, comme le clavier du salon du duel) ; WFS-545 RG-01 : les mouvements d'un doigt posé
// (down = true répété par Android) ne sont pas de nouveaux appuis
bool toucher(const Touch& t) {
    if (!g_e.filtre.poser(t.id, t.down)) return true;
    g_e.presse = -1;
    if (g_e.zPseudo.contient(t.x, t.y)) { g_e.saisie.choisirChamp(Champ::Pseudo); return true; }
    if (g_e.zPlaque.contient(t.x, t.y)) { g_e.saisie.choisirChamp(Champ::Plaque); return true; }
    if (g_e.zValider.contient(t.x, t.y)) { if (g_e.saisie.valide()) valider(); return true; }
    if (g_e.modification && g_e.zAnnuler.contient(t.x, t.y)) {   // WFS-482 (RG-07) : « Annuler » : rien ne change
        g_modifDemandee = false;
        WFS_LOG("profil (WFS-482) : modification annulée, pseudo « %s » et plaque « %s » gardés", sauvegarde().nick.c_str(), sauvegarde().profil.plaque.c_str());
        return true;
    }
    const Touche* cl = creation::clavier(ui::langue::active());
    for (int i = 0; i < creation::NB_TOUCHES; i++)
        if (g_e.zTouches[i].contient(t.x, t.y)) {
            // WFS-545 RG-02 : 150 ms après chaque caractère accepté (heure de la dernière image, horloge réelle sur le téléphone)
            if (creation::toucheActive(cl[i], g_e.saisie.champ()) && g_e.filtre.accepter(g_e.maintenant)) {
                g_e.saisie.appuyer(cl[i]); g_e.presse = i; g_e.tPresse = g_e.maintenant;
            }
            return true;
        }
    return true;   // RG-02 : tout toucher est gardé (rien du menu dessous)
}

// ---------------------------------------------------------------- dessin
struct Page { float u, x0, y0, colonne, kx, ky, touche, ecart; };
Page miseEnPage(float W, float H, const Marges& m) {
    const float ppd = ui::pixelsParDp(W), Wd = W - m.gauche - m.droite, Hd = H - m.haut - m.bas;
    const float largeurClavier = creation::COLONNES * TOUCHE_DP + (creation::COLONNES - 1) * ECART_DP;   // 606 dp
    const float hauteurClavier = creation::LIGNES * TOUCHE_DP + (creation::LIGNES - 1) * ECART_DP;      // 301 dp
    // unité : la densité du téléphone (touches de 9 mm), agrandie sur un écran de bureau, réduite si la page ne tient pas
    float u = std::max(ppd, std::min(Hd / 400.f, Wd / 900.f));
    u = std::min({u, Hd / (hauteurClavier + 2 * MARGE_DP), Wd / (largeurClavier + 3 * MARGE_DP + COLONNE_MIN_DP)});
    Page p{};
    p.u = u; p.touche = TOUCHE_DP * u; p.ecart = ECART_DP * u;
    p.x0 = m.gauche + MARGE_DP * u; p.y0 = m.haut + MARGE_DP * u;
    p.kx = W - m.droite - MARGE_DP * u - largeurClavier * u;
    p.ky = m.haut + (Hd - hauteurClavier * u) / 2;
    p.colonne = p.kx - MARGE_DP * u - p.x0;
    return p;
}

// champ de saisie (verre teinté, liseré bas, contour cyan quand il est choisi) ; texte du joueur, curseur clignotant
void champ(Renderer& r, const Zone& z, float u, const std::string& valeur, const char* indice, bool choisi, bool clignote, float espacement) {
    r.uiRect(z.x, z.y, z.w, z.h, GLASS);
    r.uiRect(z.x, z.y + z.h - 3 * u, z.w, 3 * u, choisi ? hex(ACCENT2) : hex(SURFACE2));
    if (choisi) {
        const vec2 c[4] = {{z.x, z.y}, {z.x + z.w, z.y}, {z.x + z.w, z.y + z.h}, {z.x, z.y + z.h}};
        dessin2d::contour(r, c, 4, 2 * u, hex(ACCENT2, 0.9f));
    }
    const float px = 24 * u, tx = z.x + 12 * u, ty = z.y + (z.h - px) / 2 - 1.5f * u;
    if (valeur.empty()) {
        if (indice) {   // indice en gris : texte de la table (journal des textes du banc)
            ui::text(r, tx, ty + 3 * u, 18 * u, ui::tr(indice), hex(MUTED), ui::GAUCHE, 0, false);
        }
    } else {
        // texte saisi par le joueur : ce n'est pas un texte du jeu à traduire (journal des textes : ligne technique)
        ui::langue::journalTechnique(true);
        ui::text(r, tx, ty, px, valeur, hex(TEXTE), ui::GAUCHE, espacement, false);
        ui::langue::journalTechnique(false);
    }
    if (choisi && clignote) {
        const float cx = valeur.empty() ? tx : tx + ui::width(px, valeur, espacement) + 2 * u;
        r.uiRect(cx, z.y + 10 * u, 2 * u, z.h - 20 * u, hex(ACCENT2));
    }
}

// RG-02 : aperçu 2D de la plaque (gabarit du jeu, en attendant la plaque 3D de WFS-483) : plaque claire 520 x 110 mm,
// bord sombre, bande « WFS » à gauche (marque inventée du jeu, aucune marque ni aucun pays réel), texte du joueur en noir
} // namespace
void dessinerApercuPlaque(Renderer& r, float x, float y, float w, const std::string& texte, float u) {
    const float h = w * 110.f / 520.f, bande = w * 0.11f, b = std::max(1.f, 2 * u);
    r.uiRect(x - b, y - b, w + 2 * b, h + 2 * b, hex(BG));
    r.uiRect(x, y, w, h, hex(TEXTE));
    r.uiRect(x, y, bande, h, hex(SURFACE2));
    r.uiRect(x + bande, y, std::max(1.f, u), h, hex(MUTED));
    ui::langue::journalTechnique(true);   // marque du jeu et texte du joueur : rien à traduire
    ui::textBoite(r, x + bande / 2, y, bande, h, h * 0.30f, "WFS", hex(TEXTE), ui::CENTRE, 0.05f);
    if (!texte.empty()) ui::textBoite(r, x + bande + (w - bande) / 2, y, w - bande - 8 * u, h, h * 0.70f, texte, hex(BG), ui::CENTRE, 0.10f);
    ui::langue::journalTechnique(false);
}
namespace {

// icône majuscules : flèche vers le haut (pentagone), pleine quand la majuscule est armée
void iconeMajuscules(Renderer& r, float cx, float cy, float s, bool armee, const vec4& c) {
    const vec2 p[7] = {{cx, cy - s}, {cx + s, cy}, {cx + s * 0.45f, cy}, {cx + s * 0.45f, cy + s}, {cx - s * 0.45f, cy + s},
                       {cx - s * 0.45f, cy}, {cx - s, cy}};
    // polygone non convexe : tête (triangle) et tige (rectangle) remplis à part
    if (armee) {
        const vec2 tete[3] = {p[0], p[1], p[6]}, tige[4] = {p[2], p[3], p[4], p[5]};
        dessin2d::polygone(r, tete, 3, dessin2d::uni(c));
        dessin2d::polygone(r, tige, 4, dessin2d::uni(c));
    } else {
        const vec2 contourFleche[8] = {p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[0]};
        dessin2d::trait(r, contourFleche, 8, std::max(1.5f, s * 0.16f), c);
    }
}
// icône effacer « ⌫ » (même dessin que le clavier du salon du duel, WFS-146)
void iconeEffacer(Renderer& r, float cx, float cy, float u, const vec4& c) {
    const float iw = 26 * u, ih = 18 * u, ix = cx - iw / 2, iy = cy - ih / 2, p = 7 * u, ep = 2 * u;
    const vec2 penta[5] = {{ix, iy + ih / 2}, {ix + p, iy}, {ix + iw, iy}, {ix + iw, iy + ih}, {ix + p, iy + ih}};
    dessin2d::contour(r, penta, 5, ep, c);
    const float mx = ix + p + (iw - p) / 2, my = iy + ih / 2, k = 3.5f * u;
    const vec2 d1[2] = {{mx - k, my - k}, {mx + k, my + k}}, d2[2] = {{mx - k, my + k}, {mx + k, my - k}};
    dessin2d::trait(r, d1, 2, ep, c); dessin2d::trait(r, d2, 2, ep, c);
}

void dessiner(Renderer& r, const GameView& g) {
    const float W = g.uiWidth, H = g.uiHeight;
    const Page p = miseEnPage(W, H, g.marges);
    const float u = p.u, op = (float)std::clamp((g.time - g_e.tOuvert) / 0.25, 0.0, 1.0);   // fondu d'entrée de 250 ms
    const Saisie& s = g_e.saisie;
    const bool clignote = std::fmod(g.time - g_e.tOuvert, 1.0) < 0.6;
    // fond : asphalte de nuit, presque opaque (le menu n'est pas dessiné dessous), lueur orange en bas à gauche
    r.uiRect(0, 0, W, H, hex(BG, 0.80f + 0.14f * op));
    {
        const vec4 a = hex(ACCENT, 0.10f * op), z = hex(ACCENT, 0);
        const UiVertex q[4] = {{0, H * 0.55f, -1, -1, z.x, z.y, z.z, 0}, {W * 0.5f, H * 0.55f, -1, -1, z.x, z.y, z.z, 0},
                               {W * 0.5f, H, -1, -1, a.x, a.y, a.z, 0}, {0, H, -1, -1, a.x, a.y, a.z, a.w}};
        r.uiQuad(q);
    }
    // ---- colonne de gauche : titre, champs, erreurs, aperçu de la plaque, « C'est parti »
    const float x = p.x0, L = p.colonne;
    float y = p.y0;
    r.uiRect(x, y + 4 * u, 4 * u, 26 * u, hex(ACCENT));   // barre néon du titre
    const char* titre = g_e.modification ? "profil.modifier_titre" : "creation.titre";   // WFS-482 (RG-07)
    ui::textBoite(r, x + 14 * u, y, L - 14 * u, 34 * u, 30 * u, ui::majuscules(ui::tr(titre)), hex(TEXTE), ui::GAUCHE, 0.02f, true, true);
    y += 34 * u + 16 * u;
    auto bloc = [&](Champ c, const char* libelle, Zone& z, const std::string& valeur, const char* indice, float esp) {
        ui::textBoite(r, x, y, L, 14 * u, 12 * u, ui::majuscules(ui::tr(libelle)), hex(MUTED), ui::GAUCHE, 0.15f);
        y += 14 * u + 4 * u;
        z = {x, y, L, 46 * u};
        champ(r, z, u, valeur, indice, s.champ() == c, clignote, esp);
        y += 46 * u + 4 * u;
        if (const char* e = s.erreur(c)) ui::textBoite(r, x, y, L, 16 * u, 14 * u, ui::tr(e), hex(DANGER), ui::GAUCHE, 0, false);
        y += 16 * u + 10 * u;
    };
    bloc(Champ::Pseudo, "creation.pseudo", g_e.zPseudo, s.pseudo(), "creation.indice", 0.02f);
    bloc(Champ::Plaque, "creation.plaque", g_e.zPlaque, s.plaque(), nullptr, 0.10f);
    const float wp = std::min(L, 200 * u);
    dessinerApercuPlaque(r, x, y, wp, s.plaque(), u);
    // « C'est parti » en bas de la colonne (bouton orange incliné ; grisé tant que le pseudo ou la plaque ne sont pas valides)
    const float bh = 50 * u, by = std::max(y + wp * 110.f / 520.f + 14 * u, p.ky + creation::LIGNES * p.touche + (creation::LIGNES - 1) * p.ecart - bh);
    // WFS-482 (RG-07) : en modification, « Valider » (orange) et « Annuler » (vitré, texte cyan) côte à côte
    const float bw = g_e.modification ? std::min((L - 10 * u) / 2, 200 * u) : std::min(L, 260 * u);
    const bool ok = s.valide();
    dessin2d::biseau(r, x, by, bw, bh, 14 * u, dessin2d::uni(ok ? hex(ACCENT) : hex(SURFACE2)));
    const char* valider = g_e.modification ? "profil.valider" : "creation.valider";
    ui::textBoite(r, x + bw / 2, by, bw - 36 * u, bh, 22 * u, ui::majuscules(ui::tr(valider)), ok ? hex(BG) : hex(MUTED, 0.8f), ui::CENTRE, 0.12f, false, true);
    g_e.zValider = {x, by, bw, bh};
    g_e.zAnnuler = {};
    if (g_e.modification) {
        const float ax = x + bw + 10 * u;
        dessin2d::biseau(r, ax - u, by - u, bw + 2 * u, bh + 2 * u, 14 * u, dessin2d::uni(vec4{1, 1, 1, 0.16f}));
        dessin2d::biseau(r, ax, by, bw, bh, 14 * u, dessin2d::uni(GLASS));
        ui::textBoite(r, ax + bw / 2, by, bw - 36 * u, bh, 20 * u, ui::majuscules(ui::tr("profil.annuler")), hex(ACCENT2), ui::CENTRE, 0.12f, false);
        g_e.zAnnuler = {ax, by, bw, bh};
    }
    // ---- clavier (RG-03)
    const Touche* cl = creation::clavier(ui::langue::active());
    const bool appuiRecent = g_e.presse >= 0 && g_e.maintenant - g_e.tPresse < 0.12;
    for (int i = 0; i < creation::NB_TOUCHES; i++) {
        const Touche& t = cl[i];
        const float tx = p.kx + t.colonne * (p.touche + p.ecart), ty = p.ky + t.ligne * (p.touche + p.ecart);
        const float tw = t.largeur * p.touche + (t.largeur - 1) * p.ecart;
        const bool active = creation::toucheActive(t, s.champ());
        const bool majArmee = t.genre == Touche::Majuscules && s.majuscules() && s.champ() == Champ::Pseudo;
        const float a = active ? 1.f : 0.35f;
        // touche grisée : fond opaque plus sombre (rien de la scène ne passe à travers), texte atténué
        const unsigned fond = !active ? 0x111219 : t.genre == Touche::Caractere ? SURFACE2 : SURFACE;
        r.uiRect(tx, ty, tw, p.touche, appuiRecent && g_e.presse == i ? hex(0x3A3F55) : hex(fond));
        r.uiRect(tx, ty + p.touche - 2 * u, tw, 2 * u, majArmee ? hex(ACCENT2) : hex(BG, 0.6f));   // bord bas (relief) ; cyan : majuscule armée
        // zone touchable : la touche et la moitié des écarts autour (aucun trou entre deux touches)
        g_e.zTouches[i] = {tx - p.ecart / 2, ty - p.ecart / 2, tw + p.ecart, p.touche + p.ecart};
        const vec4 c = hex(TEXTE, a);
        const float cx = tx + tw / 2, cy = ty + p.touche / 2;
        switch (t.genre) {
            case Touche::Caractere: {
                const bool capitale = s.champ() == Champ::Plaque || s.majuscules();
                const char ch = t.c >= 'a' && t.c <= 'z' && capitale ? (char)(t.c - 'a' + 'A') : t.c;
                ui::langue::journalTechnique(true);   // touche d'un caractère : rien à traduire
                ui::textBoite(r, cx, ty, tw, p.touche, 26 * u, std::string(1, ch), c, ui::CENTRE);
                ui::langue::journalTechnique(false);
                break;
            }
            case Touche::Majuscules: iconeMajuscules(r, cx, cy, 11 * u, majArmee, majArmee ? hex(ACCENT2) : c); break;
            case Touche::Effacer: iconeEffacer(r, cx, cy, u, c); break;
            case Touche::Espace: ui::textBoite(r, cx, ty, tw, p.touche, 15 * u, ui::majuscules(ui::tr("creation.espace")), hex(MUTED, a), ui::CENTRE, 0.15f); break;
        }
    }
}

// points d'appui (pixels de l'interface) écrits au journal à la 1re image : l'émulateur du matin (tools/qa/emulateur.sh) tape
// « Petit » (touches p, e, t, i : même place en AZERTY et en QWERTY) puis « C'est parti » sans recopier la mise en page
void noterPoints() {
    g_e.pointsNotes = true;
    const Touche* cl = creation::clavier(ui::langue::active());
    auto centre = [](const Zone& z) { char b[32]; std::snprintf(b, sizeof b, "%d,%d", (int)(z.x + z.w / 2), (int)(z.y + z.h / 2)); return std::string(b); };
    std::string l;
    for (char c : {'p', 'e', 't', 'i'})
        for (int i = 0; i < creation::NB_TOUCHES; i++)
            if (cl[i].genre == Touche::Caractere && cl[i].c == c) l += std::string(" ") + c + "=" + centre(g_e.zTouches[i]);
    WFS_LOG("création du profil (WFS-481) : points d'appui :%s valider=%s", l.c_str(), centre(g_e.zValider).c_str());
}

class CreationProfilToucher final : public System {
public:
    const char* name() const override { return "creation-profil-touchers"; }
    bool onTouch(const Touch& t, const GameView& g) override {
        synchroniser(g);
        return g_e.ouvert && toucher(t);
    }
};

class CreationProfil final : public System {
public:
    const char* name() const override { return "creation-profil"; }
    void drawUi(Renderer& r, const GameView& g) override {
        g_e.maintenant = g.time;
        synchroniser(g);
        if (!g_e.ouvert || g.sceneDediee || !ui::fontReady()) return;
        ui::langue::journalActif(ui::langue::journalOuvert(), "creation-profil");
        dessiner(r, g);
        // manette : seules les zones de cet écran ont le focus (rien du menu caché dessous, comme le panneau des modes)
        manette::zonesImage().clear();
        manette::focus(g_e.zPseudo); manette::focus(g_e.zPlaque);
        for (const Zone& z : g_e.zTouches) manette::focus(z);
        manette::focus(g_e.zValider);
        manette::focus(g_e.zAnnuler, true);   // WFS-482 (RG-01) : Retour d'Android = « Annuler » (zone vide hors modification)
        if (!g_e.pointsNotes) noterPoints();
    }
    void onEcran(Ecran, Ecran, const GameView& g) override { synchroniser(g); }
};

} // namespace

bool creationProfilOuverte() { return g_e.ouvert; }

void ouvrirModificationProfil() {
    if (!sauvegarde().actif() || !sauvegarde().profil.cree) return;
    g_modifDemandee = true;
    WFS_LOG("profil (WFS-482) : « Modifier » → formulaire de WFS-481 (pseudo « %s », plaque « %s »)", sauvegarde().nick.c_str(), sauvegarde().profil.plaque.c_str());
}
bool modificationProfilOuverte() { return g_modifDemandee || (g_e.ouvert && g_e.modification); }

bool centreCreationProfil(const std::string& nom, float& x, float& y) {
    const Zone* z = nom == "pseudo" ? &g_e.zPseudo : nom == "plaque" ? &g_e.zPlaque : nom == "valider" ? &g_e.zValider : nom == "annuler" ? &g_e.zAnnuler : nullptr;
    if (!z && nom.size() == 1) {
        const Touche* cl = creation::clavier(ui::langue::active());
        for (int i = 0; i < creation::NB_TOUCHES; i++)
            if ((cl[i].genre == Touche::Caractere && cl[i].c == nom[0]) || (nom[0] == '<' && cl[i].genre == Touche::Effacer)) { z = &g_e.zTouches[i]; break; }
    }
    if (!z || z->w <= 0) return false;
    x = z->x + z->w / 2; y = z->y + z->h / 2; return true;
}

// ---------------------------------------------------------------- banc d'essai (WFS_BANC=creation)
int benchCreationProfil(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    const std::string lg = ui::langue::code();
    auto verifier = [&](bool ok, const char* quoi) { WFS_LOG("création du profil (WFS-481) banc [%s] : %s : %s", lg.c_str(), quoi, ok ? "OK" : "ÉCART"); if (!ok) ecarts++; };
    ui::chargerPolice(r);
    ui::langue::charger();
    // 1er lancement : installation neuve (sauvegarde du banc effacée), CGU acceptées (WFS-308), puis chargement
    const std::string chemin = out + "/creation-sauvegarde-banc.txt";
    std::remove(chemin.c_str());
    sauvegarde().charger(chemin);
    verifier(!sauvegarde().profil.cree, "installation neuve : profil=0");
    sauvegarde().cgu = 1;   // CGU acceptées (écran de WFS-308, parcours vérifié par WFS_BANC=cgu)
    sauvegarde().enregistrer();
    if (!g.init(r, "golf", "ville")) { WFS_ERR("création du profil (WFS-481) banc : chargement impossible"); sauvegarde().desactiver(); return ecarts + 1; }
    g.setAutopilot(true); g.camMode = 0;
    double t = 3000;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    auto images = [&](int n) { for (int i = 0; i < n; i++) image(); };
    auto pas = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 8 == 7) g.render(t); } };   // course : une image sur 8 (banc plus court)
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t); g.render(t + 0.01);
        const std::string fichier = std::string("creation-") + lg + "-" + nom;
        r.capturePng(out + "/" + fichier + ".png");
        WFS_LOG("capture %s : écran %s, %s", fichier.c_str(), nomEcran(g.ecran()), quoi);
    };
    // WFS-545 RG-02 : chaque appui du banc vient 150 ms après le précédent (délai du clavier), comme un pouce rapide
    auto appui = [&](int id, const Zone& z) {
        const float x = z.x + z.w / 2, y = z.y + z.h / 2;
        t += creation::FiltreAppuis::DELAI;
        g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); image();
    };
    const Touche* cl = creation::clavier(ui::langue::active());
    auto indice = [&](char c) { for (int i = 0; i < creation::NB_TOUCHES; i++) if (cl[i].genre == Touche::Caractere && cl[i].c == c) return i; return -1; };
    auto genre = [&](Touche::Genre gn) { for (int i = 0; i < creation::NB_TOUCHES; i++) if (cl[i].genre == gn) return i; return -1; };
    double tActions = 0;   // RG-05 : temps d'actions du scénario, 1,2 s par appui (rythme d'un pouce qui cherche ses lettres)
    auto taper = [&](const char* texte) {
        for (const char* c = texte; *c; c++) {
            char b = *c; bool maj = false;
            if (b >= 'A' && b <= 'Z') { b = (char)(b - 'A' + 'a'); maj = true; }
            if (maj != g_e.saisie.majuscules() && g_e.saisie.champ() == Champ::Pseudo) { appui(50, g_e.zTouches[genre(Touche::Majuscules)]); tActions += 1.2; }
            const int i = b == ' ' ? genre(Touche::Espace) : indice(b);
            if (i < 0) { verifier(false, "touche absente du clavier"); continue; }
            appui(51, g_e.zTouches[i]); tActions += 1.2;
        }
    };
    auto effacerTout = [&]() { const int e = genre(Touche::Effacer); for (int k = 0; k < 20; k++) appui(52, g_e.zTouches[e]); };
    g.allerA(Ecran::Menu);   // comme sur le téléphone : le chargement mène au menu (aucune course jouée avant)
    images(30);
    verifier(creationProfilOuverte(), "RG-01 : 1er lancement → après le chargement, écran « Crée ton pilote »");
    // RG-03 : touches de 9 mm au moins sur le téléphone de référence (2,625 px par dp, 401 ppp)
    {
        const float dp = ui::pixelsParDp((float)r.width());
        WFS_LOG("création du profil (WFS-481) banc : touche %.0f px = %.1f dp (%.1f mm à 160 dp par pouce)", g_e.zTouches[0].h, g_e.zTouches[0].h / dp, g_e.zTouches[0].h / dp * 25.4f / 160.f);
        verifier(r.width() < 2000 || g_e.zTouches[0].h / dp * 25.4f / 160.f >= 9.f, "RG-03 : touches de 9 mm au moins (zone touchable)");
    }
    images(20);
    capture("vide", "écran vide, « C'est parti » grisé, majuscule armée");
    verifier(!g_e.saisie.valide(), "RG-02 : « C'est parti » grisé sans pseudo");
    appui(53, g_e.zValider);
    verifier(creationProfilOuverte() && !sauvegarde().profil.cree, "RG-02 : « C'est parti » grisé : rien ne se passe");
    // WFS-545 RG-01 : doigt posé sur « r » (majuscule armée), 10 mouvements du même doigt (down répété par Android), lever → « R »
    {
        const Zone& z = g_e.zTouches[indice('r')];
        const float x = z.x + z.w / 2, y = z.y + z.h / 2;
        t += creation::FiltreAppuis::DELAI;
        g.onTouch({60, x, y, true}); image();
        for (int k = 0; k < 10; k++) { g.onTouch({60, x + (k % 3) * 0.7f, y + (k % 2) * 0.7f, true}); image(); }
        WFS_LOG("création du profil (WFS-545) banc : appui sur « r » + 10 mouvements du doigt → pseudo « %s »", g_e.saisie.pseudo().c_str());
        verifier(g_e.saisie.pseudo() == "R", "WFS-545 RG-01 : un appui + 10 mouvements du même doigt → un seul « R »");
        capture("repetition", "WFS-545 : doigt posé sur « r » qui bouge (10 mouvements) : un seul « R »");
        g.onTouch({60, x, y, false}); image();
    }
    effacerTout();
    // WFS-545 RG-02 : « Gh » puis 3 appuis sur « j » à 50 ms d'écart (rebond, 3 doigts différents) → un seul « j » ; à 200 ms → deux
    taper("Gh");
    {
        const Zone& z = g_e.zTouches[indice('j')];
        const float x = z.x + z.w / 2, y = z.y + z.h / 2;
        t += creation::FiltreAppuis::DELAI;
        for (int k = 0; k < 3; k++) { t += 0.05; g.render(t); g.onTouch({61 + k, x, y, true}); g.onTouch({61 + k, x, y, false}); }
        image();
        WFS_LOG("création du profil (WFS-545) banc : « Gh » + 3 appuis sur « j » à 50 ms → pseudo « %s »", g_e.saisie.pseudo().c_str());
        verifier(g_e.saisie.pseudo() == "Ghj", "WFS-545 RG-02 : 3 appuis à 50 ms → un seul « j »");
        capture("rapide", "WFS-545 : « Gh » puis 3 appuis sur « j » à 50 ms : « Ghj »");
        t += 0.2; g.render(t); g.onTouch({64, x, y, true}); g.onTouch({64, x, y, false}); image();
        verifier(g_e.saisie.pseudo() == "Ghjj", "WFS-545 RG-02 : appui suivant à 200 ms → accepté");
    }
    effacerTout();
    // RG-04 : pseudo trop court
    taper("Ma");
    verifier(g_e.saisie.erreur(Champ::Pseudo) && std::strcmp(g_e.saisie.erreur(Champ::Pseudo), "creation.erreur.pseudo_court") == 0, "RG-04 : « 3 caractères au moins » à la frappe");
    capture("court", "pseudo de 2 lettres : « 3 caractères au moins » en rouge");
    effacerTout();
    // RG-04 : mot interdit (en français, un mot de la liste ; le message ne répète pas le mot)
    if (ui::langue::active() == ui::langue::Langue::Francais) {
        taper("Abruti");
        const char* e = g_e.saisie.erreur(Champ::Pseudo);
        verifier(e && std::strcmp(e, "creation.erreur.mot") == 0, "RG-04 : « mot non autorisé » à la frappe");
        verifier(ui::tr("creation.erreur.mot").find("bruti") == std::string::npos, "RG-04 : le message ne répète pas le mot");
        verifier(!g_e.saisie.valide(), "RG-04 : mot interdit → « C'est parti » grisé");
        capture("interdit", "mot interdit : « Mot non autorisé », « C'est parti » grisé");
        effacerTout();
    }
    // RG-05 : scénario chronométré : 6 lettres (majuscule armée au départ), plaque proposée gardée, « C'est parti »
    tActions = 0;
    const char* pseudo = ui::langue::active() == ui::langue::Langue::Francais ? "Karima" : "Ryderx";
    taper(pseudo);
    verifier(g_e.saisie.pseudo() == pseudo, "RG-05 : pseudo tapé au clavier du jeu");
    const std::string plaqueProposee = g_e.saisie.plaque();
    verifier(!plaqueProposee.empty() && g_e.saisie.refusPlaque() == profil::Refus::Aucun, "RG-02 : plaque proposée d'après le pseudo");
    WFS_LOG("création du profil (WFS-481) banc : pseudo « %s » → plaque proposée « %s »", pseudo, plaqueProposee.c_str());
    capture("rempli", "pseudo de 6 lettres, plaque proposée, aperçu de la plaque, « C'est parti » actif");
    // RG-02 : plaque touchée par le joueur → elle ne suit plus le pseudo ; RG-03 : sur la plaque, l'espace est sans effet
    {
        Saisie essai = g_e.saisie;
        essai.choisirChamp(Champ::Plaque);
        essai.appuyer(cl[genre(Touche::Espace)]);
        verifier(essai.plaque() == plaqueProposee, "RG-03 : espace sans effet sur la plaque");
    }
    if (ui::langue::active() == ui::langue::Langue::Anglais) {   // capture du champ Plaque choisi (touches grisées), puis retour au pseudo
        appui(54, g_e.zPlaque);
        verifier(g_e.saisie.champ() == Champ::Plaque, "champ Plaque choisi au doigt");
        capture("plaque", "champ Plaque choisi : espace, « _ » et majuscules grisés");
        appui(55, g_e.zPseudo);
    }
    appui(56, g_e.zValider); tActions += 1.2;
    WFS_LOG("création du profil (WFS-481) banc : RG-05 : %d appuis, %.1f s d'actions (1,2 s par appui), limite 20 s", (int)std::lround(tActions / 1.2), tActions);
    verifier(tActions < 20.0, "RG-05 : moins de 20 s d'actions");
    images(2);
    verifier(!creationProfilOuverte() && sauvegarde().profil.cree && sauvegarde().nick == pseudo && sauvegarde().profil.plaque == plaqueProposee,
             "RG-07 : « C'est parti » → profil écrit (pseudo, plaque, profil=1), écran fermé");
    verifier(g.ecran() == Ecran::Menu, "RG-07 : menu principal après la création");
    images(40);
    capture("menu", "menu principal après la validation");
    // course → fin → menu (le profil note la course : WFS-480)
    courseBenchRules(true);
    g.allerA(Ecran::Course); pas(60 * 6);
    verifier(g.ecran() == Ecran::Course && activeCourse() && activeCourse()->state == RaceState::Play, "course en cours");
    g.setAutopilot(false); courseBenchSetTime(0.05); pas(90);
    verifier(activeCourse() && activeCourse()->state == RaceState::Over, "course → écran de fin");
    g.allerA(Ecran::Menu); images(30);
    courseBenchRules(false);
    verifier(g.ecran() == Ecran::Menu && !creationProfilOuverte(), "fin → menu, sans écran de création");
    verifier(sauvegarde().profil.courses == 1, "la course est notée dans le profil (WFS-480) : 1 course");
    WFS_LOG("création du profil (WFS-481) banc : %s", sauvegarde().profil.texte(sauvegarde().nick).c_str());
    g.shutdown();
    // 2e lancement : sauvegarde relue, plus d'écran de création (RG-01)
    sauvegarde().attendre(); sauvegarde().charger(chemin);
    verifier(sauvegarde().profil.cree && sauvegarde().nick == pseudo, "2e lancement : profil relu (profil=1)");
    if (g.init(r, "golf", "ville")) {
        g.setAutopilot(true);
        g.allerA(Ecran::Menu); images(30);
        verifier(!creationProfilOuverte(), "RG-01 : 2e lancement → menu, plus d'écran de création");
        g.shutdown();
    } else verifier(false, "2e lancement : chargement");
    // RG-01 / WFS-480 RG-05 : joueur déjà installé avant le profil (pseudo choisi, profil=0) : écran une fois, son pseudo proposé
    sauvegarde().profil.cree = false; sauvegarde().nick = "Lina_77"; sauvegarde().enregistrer();
    if (g.init(r, "golf", "ville")) {
        g.setAutopilot(true);
        g.allerA(Ecran::Menu); images(30);
        verifier(creationProfilOuverte() && g_e.saisie.pseudo() == "Lina_77", "RG-01 : joueur déjà installé → écran une fois, pseudo actuel proposé");
        capture("installe", "joueur déjà installé : pseudo actuel proposé, plaque d'après le pseudo");
        g.shutdown();
    } else verifier(false, "joueur déjà installé : chargement");
    // RG-06 / RC-E20-06 : textes de l'écran dans la langue du banc (aucune phrase française en anglais, aucun texte qui dépasse)
    if (ui::langue::active() == ui::langue::Langue::Anglais) {
        ui::langue::viderJournal(); ui::langue::journalActif(true, "creation-profil");
        if (g.init(r, "golf", "ville")) {
            g.allerA(Ecran::Menu); images(10);
            g.shutdown();
        }
        ui::langue::journalActif(false);
        const int e = ui::langue::verifierJournal();
        verifier(e == 0, "RC-E20-06 : aucun texte français ni texte qui dépasse en anglais");
    }
    sauvegarde().desactiver();
    std::remove(chemin.c_str());
    WFS_LOG("création du profil (WFS-481) banc [%s] : parcours complet : %d écart(s)%s", lg.c_str(), ecarts, ecarts ? " ÉCHEC" : " OK");
    return ecarts;
}

} // namespace ecrans
} // namespace wfs

WFS_SYSTEM(wfs::ecrans::CreationProfilToucher, 401)   // interface : avant le menu (450) et ses bandeaux (458), qui ne voient plus les touchers
WFS_SYSTEM(wfs::ecrans::CreationProfil, 459)          // interface : par-dessus le menu (450), les bandeaux et le panneau des modes (458), sous les Réglages (460)
