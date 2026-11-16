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
#include "../boutons.h"   // WFS-549 : boutons au modèle R19
#include "../verre.h"   // WFS-548 (R19) : panneaux de verre, sur-titres gris, fond flouté

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
// champ de saisie (verre teinté, liseré bas, contour cyan quand il est choisi) ; texte du joueur, curseur clignotant
bool g_champsEntiers = true;   // WFS-482 (RG-08) : textes saisis entiers dans leur champ à la dernière image
// WFS-557 (maquette 20) : champ de verre aux coins arrondis (12 px), bordé d'orange quand il est choisi ; texte du joueur en
// Rajdhani italique ; curseur orange clignotant ; indice en Inter gris
void champ(Renderer& r, const Zone& z, float u, const std::string& valeur, const char* indice, bool choisi, bool clignote, float espacement) {
    vec2 q[ui::boutons::MAX_SOMMETS];
    const int n = ui::boutons::contourArrondi(q, z.x, z.y, z.w, z.h, 12 * u);
    dessin2d::polygone(r, q, n, {hex(0x2A2C34, 0.92f), hex(0x1C1E25, 0.92f), z.y, z.y + z.h});
    dessin2d::contour(r, q, n, (choisi ? 2.f : 1.f) * u, choisi ? hex(ACCENT) : hex(TEXTE, 0.14f));
    // WFS-482 (RG-08) : texte saisi réduit pour tenir dans le champ (16 « W » sans rien qui déborde), curseur compris
    const float px0 = 30 * u, tx = z.x + 17 * u, place = z.w - 34 * u - 6 * u;
    const float large = valeur.empty() ? 0.f : ui::width(px0, valeur, espacement);
    const float px = large > place && large > 0 ? px0 * place / large : px0;
    const float ty = z.y + (z.h - px) / 2;
    if (valeur.empty()) {
        if (indice) { ui::AvecPolice inter(ui::Police::Inter); ui::text(r, tx, z.y + (z.h - 17 * u) / 2, 17 * u, ui::tr(indice), hex(MUTED), ui::GAUCHE, 0, false); }
    } else {
        // texte saisi par le joueur : ce n'est pas un texte du jeu à traduire (journal des textes : ligne technique)
        ui::langue::journalTechnique(true);
        ui::text(r, tx, ty, px, valeur, hex(TEXTE), ui::GAUCHE, espacement, false, true);
        ui::langue::journalTechnique(false);
    }
    // WFS-482 (RG-08) : fin du texte et curseur dans le cadre (vérifié par le banc « profil »)
    if (!valeur.empty() && tx + ui::width(px, valeur, espacement) + 4 * u > z.x + z.w - 2 * u) g_champsEntiers = false;
    if (choisi && clignote) {
        const float cx = valeur.empty() ? tx : tx + ui::width(px, valeur, espacement) + 3 * u;
        r.uiRect(cx, z.y + 12 * u, 2 * u, z.h - 24 * u, hex(ACCENT));
    }
}

} // namespace

// RG-02 : aperçu 2D de la plaque (gabarit du jeu, en attendant la plaque 3D de WFS-483) : plaque claire 520 x 110 mm,
// bord sombre, bande « WFS » à gauche (marque inventée du jeu, aucune marque ni aucun pays réel), texte du joueur en noir ;
// WFS-482 : aussi dessiné par l'écran Profil (déclaré dans creation_profil.h)
// WFS-557 (maquettes 20 et 21) : plaque crème aux coins arrondis, bande orange à gauche avec « WFS » en petit en bas,
// cadre sombre fin ; texte du joueur en capitales espacées, sombre. Hauteur = largeur × 50 / 230 (maquette 21 : 230 × 50).
void dessinerApercuPlaque(Renderer& r, float x, float y, float w, const std::string& texte, float u) {
    const float h = w * 50.f / 230.f, bande = w * 0.12f, b = std::max(1.f, 2 * u), R = 3 * u;
    vec2 q[ui::boutons::MAX_SOMMETS];
    int n = ui::boutons::contourArrondi(q, x - b, y - b, w + 2 * b, h + 2 * b, R + b);
    dessin2d::polygone(r, q, n, dessin2d::uni(hex(0x15171D)));
    n = ui::boutons::contourArrondi(q, x, y, w, h, R);
    dessin2d::polygone(r, q, n, dessin2d::uni(hex(0xF2F0EA)));
    n = ui::boutons::contourArrondi(q, x, y, bande + R, h, R);
    dessin2d::polygone(r, q, n, dessin2d::uni(hex(0xFF6A00)));
    r.uiRect(x + bande, y, R, h, hex(0xF2F0EA));   // bord droit de la bande net (le coin arrondi ne vaut qu'à gauche)
    ui::langue::journalTechnique(true);   // marque du jeu et texte du joueur : rien à traduire
    ui::textBoite(r, x + bande / 2, y + h * 0.66f, bande, h * 0.30f, h * 0.22f, "WFS", hex(0x15171D), ui::CENTRE, 0.02f);
    if (!texte.empty()) ui::textBoite(r, x + bande + (w - bande) / 2, y, w - bande - 12 * u, h, h * 0.66f, texte, hex(0x15171D), ui::CENTRE, 0.12f);
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

// WFS-557 (maquette 20 ; arbitrages du PO du 2026-11-16 : maquette à 100 %, RG-14 : plus de champ « Plaque » à part, la plaque
// suit le pseudo ; toucher l'aperçu « Ta plaque » la rend modifiable avec le même clavier). Unité k = H / 720 (maquette
// 1600 × 720). Sur-titre « Première course », titre ; panneau de gauche : « Pseudo » (champ bordé d'orange quand il est
// choisi), « Ta plaque » (aperçu, bordé d'orange quand il est choisi), phrase d'aide ; « C'est parti » rond (Start/Stop) ;
// clavier de verre : les 39 premières touches de la table en 3 rangs de 13 (⇧ plus large), ⌫ seule, puis « Espace ».
void dessiner(Renderer& r, const GameView& g) {
    // unité de la maquette ; écran moins large que 20:9 (16:9) : tout est réduit pour tenir en largeur, et centré
    const float W = g.uiWidth, H = g.uiHeight, k = std::min(H / 720.f, (W - g.marges.gauche - g.marges.droite) / 1600.f);
    const float op = (float)std::clamp((g.time - g_e.tOuvert) / 0.25, 0.0, 1.0);   // fondu d'entrée de 250 ms
    const Saisie& s = g_e.saisie;
    const bool clignote = std::fmod(g.time - g_e.tOuvert, 1.0) < 0.6;
    verre::fenetreSurEcran(r);   // menu dessous : rien de son interface (verre::couvert)
    verre::assombrir(r, W, H, 0.72f * op);
    const float ox = g.marges.gauche + (W - g.marges.gauche - g.marges.droite - 1600 * k) / 2, oy = g.marges.haut + (H - g.marges.haut - g.marges.bas - 720 * k) / 2;
    const float x = ox + 56 * k;
    // ---- titre
    verre::surTitre(r, x, oy + 41 * k, 16 * k, ui::tr(g_e.modification ? "profil.surtitre" : "creation.surtitre"));
    const char* titre = g_e.modification ? "profil.modifier_titre" : "creation.titre";   // WFS-482 (RG-07)
    ui::textBoite(r, x + 2 * k, oy + 53 * k, 470 * k, 54 * k, 54 * k, ui::majuscules(ui::tr(titre)), hex(TEXTE), ui::GAUCHE, 0.01f, false, true);
    // ---- panneau de gauche : 462 × 258
    verre::Style st; st.rayon = 16 * k; st.lisere = 1.2f * k; st.opacite = op;
    verre::panneau(r, x + 1 * k, oy + 131 * k, 462 * k, 258 * k, st);
    const float xi = x + 22 * k, li = 419 * k;
    verre::surTitre(r, xi, oy + 154 * k, 14 * k, ui::tr("creation.pseudo"));
    g_champsEntiers = true;
    g_e.zPseudo = {xi, oy + 175 * k, li, 52 * k};
    champ(r, g_e.zPseudo, k, s.pseudo(), "creation.indice", s.champ() == Champ::Pseudo, clignote, 0.01f);
    if (const char* e = s.erreur(Champ::Pseudo)) { ui::AvecPolice inter(ui::Police::InterMedium); ui::textBoite(r, xi, oy + 230 * k, li, 16 * k, 13 * k, ui::tr(e), hex(DANGER), ui::GAUCHE, 0, false); }
    verre::surTitre(r, xi, oy + 250 * k, 14 * k, ui::tr("creation.ta_plaque"));
    const float wp = 297 * k, hp = wp * 50.f / 230.f, yp = oy + 274 * k;
    dessinerApercuPlaque(r, xi, yp, wp, s.plaque(), k);
    g_e.zPlaque = {xi - 4 * k, yp - 4 * k, wp + 8 * k, hp + 8 * k};
    if (s.champ() == Champ::Plaque) {   // RG-14 : plaque choisie : contour orange (comme le champ du pseudo), curseur après le texte
        vec2 q[ui::boutons::MAX_SOMMETS];
        const int n = ui::boutons::contourArrondi(q, xi - 4 * k, yp - 4 * k, wp + 8 * k, hp + 8 * k, 6 * k);
        dessin2d::contour(r, q, n, 2 * k, hex(ACCENT));
        if (const char* e = s.erreur(Champ::Plaque)) { ui::AvecPolice inter(ui::Police::InterMedium); ui::textBoite(r, xi + wp + 12 * k, yp, li - wp - 12 * k, hp, 12 * k, ui::tr(e), hex(DANGER), ui::GAUCHE, 0, false); }
    }
    { ui::AvecPolice inter(ui::Police::Inter);
      ui::textBoite(r, xi, oy + 348 * k, li, 16 * k, 14 * k, ui::tr("creation.aide_plaque"), hex(0xC9CDDA, op), ui::GAUCHE, 0, false); }
    // ---- « C'est parti » rond (146 px, centre 265, 545) ; en modification « Valider » au même endroit, « Annuler » (autocollant) à droite
    const bool ok = s.valide();
    const float d = 146 * k;
    g_e.zValider = {x + 209 * k - d / 2, oy + 545 * k - d / 2, d, d};
    {
        const std::string t = ui::majuscules(ui::tr(g_e.modification ? "profil.valider" : "creation.valider"));
        const size_t sp = t.find(' ');   // « C'EST » en grand, « PARTI » en sous-titre (maquette 20)
        ui::boutons::startStop(r, g_e.zValider.x, g_e.zValider.y, d, d, sp == std::string::npos ? t : t.substr(0, sp), 1, false,
                               sp == std::string::npos ? "" : t.substr(sp + 1), 24 * k, !ok);
    }
    g_e.zAnnuler = {};
    if (g_e.modification) {
        g_e.zAnnuler = {g_e.zValider.x + d + 30 * k, oy + 545 * k - 36 * k, 190 * k, 72 * k};
        ui::boutons::autocollant(r, g_e.zAnnuler.x, g_e.zAnnuler.y, g_e.zAnnuler.w, g_e.zAnnuler.h, ui::majuscules(ui::tr("profil.annuler")), 1, false, 30 * k);
    }
    // ---- clavier : panneau 1018 × 348 ; touches 62 × 52, pas de 74 × 64 ; ⇧ et ⌫ de 84 ; « Espace » 439
    const float kx0 = ox + 521 * k, ky0 = oy + 131 * k;
    verre::panneau(r, kx0, ky0, 1018 * k, 348 * k, st);
    const Touche* cl = creation::clavier(ui::langue::active());
    const bool appuiRecent = g_e.presse >= 0 && g_e.maintenant - g_e.tPresse < 0.12;
    const float tw0 = 62 * k, th = 52 * k, ec = 12 * k, large = 84 * k, centre = kx0 + 509 * k;
    float rangX[3], xCur[3];
    for (int rg = 0; rg < 3; rg++) {   // largeur de chaque rang (⇧ plus large) : rangs centrés sur le panneau
        float lw = 0;
        for (int i = rg * 13; i < rg * 13 + 13; i++) lw += (cl[i].genre == Touche::Majuscules ? large : tw0) + (i % 13 ? ec : 0);
        rangX[rg] = xCur[rg] = centre - lw / 2;
    }
    (void)rangX;
    for (int i = 0; i < creation::NB_TOUCHES; i++) {
        const Touche& t = cl[i];
        float tx, ty, tw;
        if (i < 39) { const int rg = i / 13; tw = t.genre == Touche::Majuscules ? large : tw0; tx = xCur[rg]; ty = ky0 + 19 * k + rg * (th + ec); xCur[rg] += tw + ec; }
        else if (t.genre == Touche::Effacer) { tw = large; tx = centre - tw / 2; ty = ky0 + 211 * k; }
        else { tw = 439 * k; tx = centre - tw / 2; ty = ky0 + 275 * k; }
        const bool active = creation::toucheActive(t, s.champ());
        const bool majArmee = t.genre == Touche::Majuscules && s.majuscules() && s.champ() == Champ::Pseudo;
        const bool presse = appuiRecent && g_e.presse == i;
        // touche de verre sombre aux coins arrondis, liseré clair ; grisée : plus sombre, texte atténué
        vec2 q[ui::boutons::MAX_SOMMETS];
        const int n = ui::boutons::contourArrondi(q, tx, ty, tw, th, 9 * k);
        dessin2d::halo(r, q, n, 3 * k, {0, 0, 0, 0.35f});
        dessin2d::polygone(r, q, n, {presse ? hex(0x3A3F4E) : hex(active ? 0x262933 : 0x14161C), hex(presse ? 0x2C303C : active ? 0x17191F : 0x101217), ty, ty + th});
        dessin2d::contour(r, q, n, 1 * k, majArmee ? hex(ACCENT2) : hex(TEXTE, active ? 0.14f : 0.06f));
        // zone touchable : la touche et la moitié des écarts autour (aucun trou entre deux touches d'un rang)
        g_e.zTouches[i] = {tx - ec / 2, ty - ec / 2, tw + ec, th + ec};
        const vec4 c = hex(TEXTE, active ? 1.f : 0.35f);
        const float cx = tx + tw / 2, cy = ty + th / 2;
        switch (t.genre) {
            case Touche::Caractere: {
                const bool capitale = s.champ() == Champ::Plaque || s.majuscules();
                const char ch = t.c >= 'a' && t.c <= 'z' && capitale ? (char)(t.c - 'a' + 'A') : t.c;
                ui::langue::journalTechnique(true);   // touche d'un caractère : rien à traduire
                { ui::AvecPolice inter(ui::Police::InterMedium); ui::textBoite(r, cx, ty, tw, th, 21 * k, std::string(1, ch), c, ui::CENTRE); }
                ui::langue::journalTechnique(false);
                break;
            }
            case Touche::Majuscules: iconeMajuscules(r, cx, cy, 8 * k, majArmee, majArmee ? hex(ACCENT2) : c); break;
            case Touche::Effacer: iconeEffacer(r, cx, cy, 0.75f * k, c); break;
            case Touche::Espace: { ui::AvecPolice inter(ui::Police::InterMedium);
                                   ui::textBoite(r, cx, ty, tw, th, 14 * k, ui::majuscules(ui::tr("creation.espace")), hex(0xC9CDDA, active ? 1.f : 0.35f), ui::CENTRE, 0.20f); break; }
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
bool champsCreationEntiers() { return g_champsEntiers; }
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
        // WFS-557 (arbitrage du PO du 2026-11-16, RG-06 / RG-07) : touches de la maquette 20 (62 × 52 px) ; zone touchable (touche et
        // demi-écarts, 74 × 64 px de la maquette) jamais sous 44 px de la maquette (la règle des 9 mm de WFS-481 est remplacée)
        const float k = r.height() / 720.f;
        verifier(g_e.zTouches[0].h >= 44 * k - 0.5f && g_e.zTouches[0].w >= 44 * k - 0.5f, "WFS-557 RG-07 : zone touchable ≥ 44 px de la maquette");
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
