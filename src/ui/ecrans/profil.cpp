// WaForSpeed — écran « Profil » (WFS-482) : voir profil.h.
#include "profil.h"
#include "menu.h"   // WFS-557 : bloc RECORD du menu (banc)
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include "../../core/log.h"
#include "../../core/system.h"
#include "../../game/cars.h"
#include "../../game/course.h"
#include "../../game/course_system.h"   // banc : courseBenchRules, activeCourse
#include "../../game/game.h"
#include "../../game/commandes_tactiles.h"   // dessin2d (biseau)
#include "../verre.h"   // WFS-548 (R19) : panneaux de verre, sur-titres gris, couleurs d'état, fond flouté
#include "../../game/sauvegarde.h"
#include "../../game/textes_course.h"         // ordinal (« 1re » / « 1st »), comme l'écran de fin
#include "../../platform/manette.h"           // focus de la manette ; zone retour = touche Retour d'Android
#include "../../platform/retour_systeme.h"    // banc : touche Retour d'Android
#include "../../render/renderer.h"
#include "../langue.h"
#include "../petit_ecran.h"
#include "../text.h"
#include "../boutons.h"   // WFS-549 : boutons au modèle R19
#include "creation_profil.h"   // formulaire de WFS-481 (« Modifier »), aperçu de la plaque
#include "reglages_appui.h"    // appui::dansCible : cibles de 48 dp
#include "textes_banc.h"       // banc « textes » : journal des textes de cet écran, noms propres

namespace wfs {
namespace ecrans::profil_ecran {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
// design-street-racing : fond, surface, accent orange (action : « Modifier »), cyan (information), textes
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF;
const unsigned ACCENT_PRESSE = 0xFF9A4D;   // bouton orange sous le doigt (même teinte que les Crédits et les CGU)
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.16f};

struct Zone {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};
enum Cible { RIEN = 0, LISTE, RETOUR, MODIFIER };

struct Etat {
    bool ouvert = false;
    double tOuvert = -1;           // fondu d'entrée (250 ms) à la 1re image
    Zone bouton, retour, modifier, liste;
    float dp = 1, dpBouton = 1;
    int doigt = -1, cible = RIEN;
    float yDoigt = 0, yDepart = 0, defil = 0, defilMax = 0;
    int lignes = 0;
    bool pseudoEntier = true;      // RG-08 : pseudo dessiné en entier (dernière image)
} g;

// RG-08 : cibles de 48 dp (Android) autour des deux boutons
int cibleSous(float x, float y) {
    if (appui::dansCible(g.retour.x, g.retour.y, g.retour.w, g.retour.h, g.dp, x, y)) return RETOUR;
    if (appui::dansCible(g.modifier.x, g.modifier.y, g.modifier.w, g.modifier.h, g.dp, x, y)) return MODIFIER;
    return g.liste.contient(x, y) ? LISTE : RIEN;
}

std::string remplir(const char* cle, const std::string& v1, const std::string& v2 = "", const std::string& v3 = "") {
    std::string t = ui::tr(cle), out;
    for (size_t i = 0; i < t.size(); i++) {
        if (t[i] == '{' && i + 2 < t.size() && t[i + 1] >= '1' && t[i + 1] <= '3' && t[i + 2] == '}') {
            out += t[i + 1] == '1' ? v1 : t[i + 1] == '2' ? v2 : v3;
            i += 2;
        } else out += t[i];
    }
    return out;
}

// RG-04 : records, mêmes textes que les écrans de fin (course5_system, poursuite_system) ; vide : « — »
std::string recordCheckpoint() { const Sauvegarde& s = sauvegarde(); return s.best > 0 ? ui::tr("profil.res.points", ui::nombre(s.best)) : "—"; }
std::string recordCourse5() {
    const Sauvegarde& s = sauvegarde();
    if (!s.aRecordCourse5()) return "—";
    std::string o;
    return remplir("c5.record", wfs::textes::ordinal(o, s.rec5Place), ui::chrono(s.rec5Ms / 1000.0));
}
std::string recordPoursuite() {
    const Sauvegarde& s = sauvegarde();
    if (!s.aRecordPoursuite()) return "—";
    return s.recPVictoire ? ui::tr("police.record_victoire", std::to_string(s.recPScore)) : ui::tr("police.record_arrete", ui::decimal(s.recPDistM / 1000.0, 2));
}

// ligne de l'historique : 2 lignes de texte (mode · voiture, date) et le résultat à droite
constexpr float LIGNE_DP = 46;

} // namespace

// ---------------------------------------------------------------- textes
std::string date(long long secondes) {
    if (secondes <= 0) return "—";
    const std::time_t t = (std::time_t)secondes;
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    static const char* const MOIS[12] = {"profil.mois.1", "profil.mois.2", "profil.mois.3", "profil.mois.4", "profil.mois.5", "profil.mois.6",
                                         "profil.mois.7", "profil.mois.8", "profil.mois.9", "profil.mois.10", "profil.mois.11", "profil.mois.12"};
    return remplir("profil.date", std::to_string(tm.tm_mday), ui::tr(MOIS[std::clamp(tm.tm_mon, 0, 11)]), std::to_string(tm.tm_year + 1900));
}

std::string nomMode(ModeJoue m) {
    switch (m) {
        case ModeJoue::Course5: return ui::tr("mode.course5");
        case ModeJoue::Poursuite: return ui::tr("mode.poursuite");
        case ModeJoue::Duel: return ui::tr("profil.mode.duel");
        default: return ui::tr("mode.checkpoint");
    }
}

std::string resultat(const CourseJouee& c) {
    if (c.quittee) return ui::tr("profil.res.quittee");
    switch (c.mode) {
        case ModeJoue::Course5: {   // place et temps (même chrono que le record de la Course à 5)
            std::string o;
            return remplir("profil.res.place", wfs::textes::ordinal(o, std::max(1, c.place)), ui::chrono(c.tempsMs / 1000.0));
        }
        case ModeJoue::Poursuite: return ui::tr(c.victoire ? "profil.res.echappe" : "profil.res.arrete", ui::nombre(c.score));
        case ModeJoue::Duel: return ui::tr(c.victoire ? "profil.res.gagne" : "profil.res.perdu");
        default: return ui::tr("profil.res.points", ui::nombre(c.score));
    }
}

std::string nomVoiture(const std::string& id) {
    for (const CarDef& c : CARS)
        if (id == c.id) { const char* n = modele(c); return n && *n ? n : id; }
    return id;
}

// ---------------------------------------------------------------- écran
void ouvrir() {
    g.ouvert = true; g.tOuvert = -1; g.defil = 0; g.doigt = -1; g.cible = RIEN;
    const Sauvegarde& s = sauvegarde();
    WFS_LOG("profil (WFS-482) : écran ouvert : %s", s.profil.texte(s.nick).c_str());
}
void fermer() { if (g.ouvert) WFS_LOG("profil (WFS-482) : écran fermé"); g.ouvert = false; g.doigt = -1; g.cible = RIEN; }
bool ouvert() { return g.ouvert; }

// WFS-557 (maquette 21 ; arbitrage du PO du 2026-11-16 : maquette à 100 %, boutons compris). Unité k = H / 720 (maquette
// 1600 × 720) ; positions relevées sur la maquette, depuis la zone sûre. Pas de grande fenêtre : en-tête (sur-titre, pseudo,
// « Pilote depuis », plaque à côté du nom), « ✎ Modifier » en verre en haut à droite, 6 tuiles de chiffres (3 × 2), panneau
// « Records », panneau « Dernières courses » (cartes), autocollant « Retour / Menu principal » en bas à droite.
void dessiner(Renderer& r, float W, float H, const Marges& m, double temps) {
    if (!g.ouvert || !ui::fontReady()) return;
    const textes::Journal journal;   // banc « textes » : textes de cet écran
    if (g.tOuvert < 0) g.tOuvert = temps;
    const float op = (float)std::clamp((temps - g.tOuvert) / 0.25, 0.0, 1.0);   // fondu d'entrée de 250 ms
    const float k = H / 720.f;
    g.dp = ui::pixelsParDp(W);
    const Sauvegarde& s = sauvegarde();
    const Profil& p = s.profil;
    verre::fenetreSurEcran(r);   // menu dessous : son interface n'est plus dessinée (verre::couvert)
    verre::assombrir(r, W, H, 0.62f * op);
    const float x0 = m.gauche + 56 * k, x1 = W - m.droite - 31 * k, y0 = m.haut;
    // ---- en-tête : sur-titre, pseudo (Rajdhani italique 70), date d'arrivée (Inter), plaque à côté du nom
    verre::surTitre(r, x0, y0 + 37 * k, 16 * k, ui::tr("profil.surtitre"));
    const float tPseudo = 70 * k, placePseudo = 400 * k;
    float finPseudo;
    {
        const textes::NomPropre np;   // pseudo du joueur : rien à traduire
        const std::string nom = ui::majuscules(s.nick);
        const float pris = ui::textBoite(r, x0 + 2 * k, y0 + 46 * k, placePseudo, tPseudo, tPseudo, nom, hex(TEXTE), ui::GAUCHE, 0.01f, false, true);
        g.pseudoEntier = ui::width(pris, nom, 0.01f) <= placePseudo + 0.5f;   // RG-08 : jamais coupé
        finPseudo = x0 + 2 * k + std::min(placePseudo, ui::width(pris, nom, 0.01f));
    }
    { ui::AvecPolice police(ui::Police::Inter);
      ui::text(r, x0, y0 + 103 * k, 15 * k, ui::tr("profil.depuis", date(p.arrivee)), hex(0xC9CDDA), ui::GAUCHE, 0, false); }
    dessinerApercuPlaque(r, std::max(x0 + 201 * k, finPseudo + 44 * k), y0 + 53 * k, 230 * k, p.plaque, k);
    // ---- « ✎ Modifier » : 158 × 51, en haut à droite
    const bool pRet = g.doigt >= 0 && g.cible == RETOUR, pMod = g.doigt >= 0 && g.cible == MODIFIER;
    {
        const float tp = 21 * k, w = 158 * k, h = 51 * k;
        g.modifier = {x1 - w, y0 + 40 * k, w, h};
        ui::boutons::verreIcone(r, g.modifier.x, g.modifier.y, w, h, ui::boutons::Icone::Crayon, ui::majuscules(ui::tr("profil.modifier")), op, pMod, tp,
                                {14 * k, 0, nullptr, 0.08f});
    }
    // ---- 6 tuiles (203 × 82 puis 203 × 98, écarts de 14) : courses, km, km/h max, frôlements, victoires, voiture préférée
    {
        const std::string id = p.voiturePreferee();
        std::string kmVoiture = "—", libVoiture = ui::majuscules(ui::tr("profil.voiture"));
        if (id != "—") {
            const auto it = p.parVoiture.find(id);
            kmVoiture = ui::tr("profil.km_valeur", ui::decimal(it != p.parVoiture.end() ? it->second.metres / 1000.0 : 0, 0));
            libVoiture += " · " + ui::majuscules(nomVoiture(id));
        }
        struct Tuile { std::string valeur, libelle; };
        const Tuile t[6] = {{ui::nombre(p.courses), ui::tr("profil.courses")}, {ui::decimal(p.km(), 1), ui::tr("profil.km")},
                            {std::to_string(p.vmax), ui::tr("profil.vmax")}, {ui::nombre(p.frolements), ui::tr("profil.frolements")},
                            {ui::nombre(p.victoires), ui::tr("profil.victoires")}, {kmVoiture, libVoiture}};
        const float tw = 203 * k, ecart = 14 * k;
        for (int i = 0; i < 6; i++) {
            const float tx = x0 + (i % 3) * (tw + ecart) + 1 * k, ty = y0 + (i < 3 ? 171 : 267) * k, th = (i < 3 ? 82 : 98) * k;
            verre::Style st; st.rayon = 12 * k; st.lisere = 1.2f * k; st.ombre = false; st.force = 0.7f; st.opacite = op;
            verre::panneau(r, tx, ty, tw, th, st);
            { const textes::NomPropre np;   // chiffres et nom de voiture
              ui::textBoite(r, tx + 16 * k, ty + 16 * k, tw - 32 * k, 40 * k, 48 * k, t[i].valeur, hex(TEXTE, op), ui::GAUCHE, 0, false, true); }
            // libellé en capitales espacées (Inter Medium), sur deux lignes s'il ne tient pas (voiture préférée)
            ui::AvecPolice police(ui::Police::InterMedium);
            const std::string lib = ui::majuscules(t[i].libelle);
            const float lp = 13 * k, esp = 0.18f, place = tw - 32 * k;
            if (ui::width(lp, lib, esp) <= place) ui::text(r, tx + 16 * k, ty + 57 * k, lp, lib, hex(0xA3A8B5, op), ui::GAUCHE, esp, false);
            else {
                size_t coupe = std::string::npos;
                for (size_t c = lib.find(' '); c != std::string::npos; c = lib.find(' ', c + 1))
                    if (ui::width(lp, lib.substr(0, c), esp) <= place) coupe = c; else break;
                const std::string l1 = coupe == std::string::npos ? lib : lib.substr(0, coupe), l2 = coupe == std::string::npos ? "" : lib.substr(coupe + 1);
                ui::textBoite(r, tx + 16 * k, ty + 57 * k, place, lp, lp, l1, hex(0xA3A8B5, op), ui::GAUCHE, esp);
                if (!l2.empty()) ui::textBoite(r, tx + 16 * k, ty + 71 * k, place, lp, lp, l2, hex(0xA3A8B5, op), ui::GAUCHE, esp);
            }
        }
    }
    // ---- panneau « Records » : 638 × 196, rangs de 48
    {
        const float rx = x0 + 1 * k, ry = y0 + 431 * k, rw = 638 * k, rh = 196 * k;
        verre::Style st; st.rayon = 16 * k; st.lisere = 1.2f * k; st.ombre = false; st.force = 0.7f; st.opacite = op;
        verre::panneau(r, rx, ry, rw, rh, st);
        verre::surTitre(r, rx + 17 * k, ry + 16 * k, 14 * k, ui::tr("profil.records"), ui::GAUCHE, op);
        struct Rec { std::string mode, valeur; };
        const Rec rec[3] = {{ui::tr("mode.checkpoint"), recordCheckpoint()}, {ui::tr("mode.course5"), recordCourse5()}, {ui::tr("mode.poursuite"), recordPoursuite()}};
        for (int i = 0; i < 3; i++) {
            const float base = ry + (66 + 48 * i) * k;
            { ui::AvecPolice police(ui::Police::InterMedium);
              ui::textBoite(r, rx + 33 * k, base - 12 * k, rw * 0.40f, 16 * k, 16 * k, rec[i].mode, hex(TEXTE, op), ui::GAUCHE, 0); }
            ui::textBoite(r, rx + rw - 32 * k, base - 13 * k, rw * 0.52f, 22 * k, 22 * k, rec[i].valeur, hex(TEXTE, op), ui::DROITE, 0, false, true);
        }
    }
    // ---- panneau « Dernières courses » : 798 × 428, cartes de 55 tous les 63
    const float lx0 = x0 + 685 * k, lyP = y0 + 171 * k, lwP = std::min(798 * k, x1 - 31 * k - lx0), lhP = 428 * k;
    {
        verre::Style st; st.rayon = 16 * k; st.lisere = 1.2f * k; st.ombre = false; st.force = 0.7f; st.opacite = op;
        verre::panneau(r, lx0, lyP, lwP, lhP, st);
        verre::surTitre(r, lx0 + 17 * k, lyP + 18 * k, 14 * k, ui::tr("profil.historique"), ui::GAUCHE, op);
        const std::string n = std::to_string(p.dernieres.size()) + " / " + std::to_string(profil::HISTORIQUE_MAX);
        const textes::NomPropre np;   // compteur : chiffres seulement
        ui::AvecPolice police(ui::Police::InterMedium);
        ui::text(r, lx0 + lwP - 18 * k, lyP + 19 * k, 13 * k, n, hex(0xA3A8B5, op), ui::DROITE, 0.18f, false);
    }
    const float ly = lyP + 43 * k, lh = lhP - 43 * k - 16 * k, cx0 = lx0 + 16 * k, cw = lwP - 34 * k, hl = 63 * k, hc = 55 * k;
    g.liste = {lx0, ly, lwP, lh};
    g.lignes = (int)p.dernieres.size();
    g.defilMax = std::max(0.f, g.lignes * hl - (lh + (hl - hc)));
    g.defil = std::clamp(g.defil, 0.f, g.defilMax);
    if (p.dernieres.empty()) {
        ui::AvecPolice police(ui::Police::Inter);
        ui::textBoite(r, lx0 + lwP / 2, ly, lwP - 48 * k, lh, 16 * k, ui::tr("profil.vide"), hex(0xA3A8B5, op), ui::CENTRE, 0);
    } else {
        for (int i = 0; i < g.lignes; i++) {
            const float yl = ly + i * hl - g.defil;
            if (yl < ly - 0.5f || yl + hc > ly + lh + 0.5f) continue;   // cartes entièrement visibles seulement
            const CourseJouee& c = p.dernieres[(size_t)i];
            verre::Style st; st.rayon = 8 * k; st.lisere = 0; st.ombre = false; st.force = 0.45f; st.opacite = op;
            verre::panneau(r, cx0, yl, cw, hc, st);
            // WFS-557 (RG-15 a) : « · record » quand la course égale le record actuel de son mode (calcul à l'affichage)
            const bool record = !c.quittee && ((c.mode == ModeJoue::Checkpoint && s.best > 0 && c.score == s.best) ||
                                               (c.mode == ModeJoue::Course5 && s.aRecordCourse5() && c.tempsMs == s.rec5Ms && c.place == s.rec5Place) ||
                                               (c.mode == ModeJoue::Poursuite && s.aRecordPoursuite() && s.recPVictoire && c.victoire && c.score == s.recPScore));
            const std::string res = record ? resultat(c) + " · " + ui::tr("profil.record") : resultat(c);
            // WFS-548 (RG-05) : rouge = perte (duel perdu, arrêté) ; cyan = gagné ou échappé ; quittée : gris clair
            const bool bon = !c.quittee && (c.victoire || (c.mode == ModeJoue::Course5 && c.place == 1));
            const bool perte = !c.quittee && !c.victoire && (c.mode == ModeJoue::Duel || c.mode == ModeJoue::Poursuite);
            const vec4 cr = record ? verre::hexa(verre::TOI, op) : bon ? hex(0x5FD6E8, op) : perte ? verre::hexa(verre::PERTE, op) : hex(c.quittee ? 0xD7DAE2 : TEXTE, op);
            ui::AvecPolice police(ui::Police::InterMedium);
            const float wr = std::min(cw * 0.45f, ui::width(16 * k, res) + 4 * k);
            ui::textBoite(r, cx0 + 17 * k, yl + 10 * k, cw - wr - 50 * k, 18 * k, 16 * k, nomMode(c.mode), hex(TEXTE, op), ui::GAUCHE, 0);
            {
                const textes::NomPropre np;   // nom de la voiture (inchangé en anglais) et date
                ui::AvecPolice inter(ui::Police::Inter);
                ui::textBoite(r, cx0 + 17 * k, yl + 31 * k, cw - wr - 50 * k, 14 * k, 12 * k, ui::majuscules(nomVoiture(c.voiture)) + " · " + date(c.date), hex(0xA3A8B5, op), ui::GAUCHE, 0);
            }
            ui::textBoite(r, cx0 + cw - 16 * k, yl, wr, hc, 16 * k, res, cr, ui::DROITE, 0);
        }
        // maquette 21 : pas d'ascenseur dessiné (la liste se fait glisser au doigt)
    }
    // ---- autocollant « Retour / Menu principal » : 206 × 88, en bas à droite (au-dessus du bord de la zone sûre)
    {
        const float w = 206 * k, h = 88 * k;
        g.retour = {x1 - 32 * k - w, H - m.bas - 8 * k - h, w, h};
        ui::boutons::autocollant(r, g.retour.x, g.retour.y, w, h, ui::majuscules(ui::tr("profil.retour")), op, pRet, 40 * k, ui::tr("reglages.surtitre.menu"));
    }
    // manette : seules les zones de cet écran ; « Retour » est la zone retour (touche Retour d'Android, RG-01)
    manette::zonesImage().clear();
    manette::focus(g.modifier);
    manette::focus(g.retour, true);
}

bool toucher(int id, float x, float y, bool down) {
    if (!g.ouvert) return false;
    const int sous = cibleSous(x, y);
    if (down) {
        if (g.doigt < 0) { g.doigt = id; g.yDoigt = g.yDepart = y; g.cible = sous; }
        else if (id == g.doigt && g.cible == LISTE) {   // glisser : la liste suit le doigt
            g.defil = std::clamp(g.defil - (y - g.yDoigt), 0.f, g.defilMax);
            g.yDoigt = y;
        }
        return true;
    }
    if (id != g.doigt) return true;   // lever d'un autre doigt (ou du doigt qui a ouvert l'écran)
    const int c = g.cible;
    g.doigt = -1; g.cible = RIEN;
    if (c == RETOUR && sous == RETOUR) { WFS_LOG("profil (WFS-482) : « Retour » → menu"); fermer(); }
    else if (c == MODIFIER && sous == MODIFIER) ouvrirModificationProfil();
    return true;
}

// même bouton que « Cadeau du jour » et « Crédits » (vitré, biseau, 44 px de haut, texte cyan), au-dessus du cadeau
void dessinerBouton(Renderer& r, float W, float H, const Marges& m, bool presse) {
    if (!ui::fontReady()) return;
    const textes::Journal journal;
    const bool cp = ui::petitEcran(W, H);
    const float k = cp ? ui::unitePetitEcran(W, H) : H / 720.f, kb = H / 720.f;
    const float h = 44 * kb, tp = 15 * k, pad = 18 * kb;
    const std::string t = ui::majuscules(ui::tr("profil.bouton"));
    const float tw = ui::width(tp, ui::majuscules(ui::trLangue("profil.bouton", ui::langue::Langue::Francais)), 0.12f);
    // « Crédits » : H − bas − 14 − h ; « Cadeau du jour » : 10 au-dessus ; « Profil » : encore 10 au-dessus
    const float w = pad + tw + pad, x = W - m.droite - 16 * k - w, y = H - m.bas - 14 * k - 3 * h - 20 * k;
    g.bouton = {x, y, w, h};
    g.dpBouton = ui::pixelsParDp(W);
    dessin2d::biseau(r, x - kb, y - kb, w + 2 * kb, h + 2 * kb, 8 * kb, dessin2d::uni(BORD));
    dessin2d::biseau(r, x, y, w, h, 8 * kb, dessin2d::uni(GLASS));
    if (presse) dessin2d::biseau(r, x, y, w, h, 8 * kb, dessin2d::uni(hex(ACCENT2, 0.22f)));   // appuyé : voile cyan
    ui::textBoite(r, x + pad + tw / 2, y, tw, h, tp, t, hex(ACCENT2), ui::CENTRE, 0.12f, false);
    manette::focus(g.bouton);
}
bool dansBouton(float x, float y) { return g.bouton.w > 0 && appui::dansCible(g.bouton.x, g.bouton.y, g.bouton.w, g.bouton.h, g.dpBouton, x, y); }

bool centre(const char* nom, float& x, float& y) {
    const std::string n = nom;
    const Zone& z = n == "retour" ? g.retour : n == "modifier" ? g.modifier : n == "liste" ? g.liste : g.bouton;
    if (z.w <= 0) return false;
    x = z.x + z.w / 2; y = z.y + z.h / 2; return true;
}
float defilement() { return g.defil; }
float defilementMax() { return g.defilMax; }
int lignesHistorique() { return g.lignes; }
bool pseudoEntier() { return g.pseudoEntier; }

// ---------------------------------------------------------------- banc d'essai (WFS_BANC=profil)
namespace {
// une langue : parcours complet, captures profil-<langue>-<nom>.png
int benchUneLangue(Game& gm, Renderer& r, const std::string& out, ui::langue::Choix choix) {
    int ecarts = 0;
    const std::string chemin = out + "/profil-sauvegarde-banc.txt";
    std::remove(chemin.c_str());
    sauvegarde().charger(chemin);
    ui::langue::choisir(choix);   // après la lecture de la sauvegarde (langue=auto du fichier neuf)
    const std::string lg = ui::langue::code();
    auto verifier = [&](bool ok, const char* quoi) { WFS_LOG("profil (WFS-482) banc [%s] : %s : %s", lg.c_str(), quoi, ok ? "OK" : "ÉCART"); if (!ok) ecarts++; };
    // joueur qui vient de créer son profil (WFS-481), arrivé le 12 nov. 2026 à 10:00 UTC (date fixe : captures stables)
    Sauvegarde& sv = sauvegarde();
    sv.cgu = 1;
    const char* pseudo0 = choix == ui::langue::Choix::Francais ? "Karima" : "Ryder";
    std::string nick;
    verifier(sv.profil.creer(pseudo0, choix == ui::langue::Choix::Francais ? "KARIMA" : "RYDER", nick), "profil créé (WFS-481)");
    sv.nick = nick;
    sv.profil.arrivee = 1794481200;
    sv.enregistrer();
    if (!gm.init(r, "golf", "ville")) { WFS_ERR("profil (WFS-482) banc : chargement impossible"); sauvegarde().desactiver(); return ecarts + 1; }
    gm.setAutopilot(true); gm.camMode = 0;
    double t = 4000;
    auto image = [&]() { gm.update(1.0 / 60); t += 1.0 / 60; gm.render(t); };
    auto images = [&](int n) { for (int i = 0; i < n; i++) image(); };
    auto pas = [&](int n) { for (int i = 0; i < n; i++) { gm.update(1.0 / 60); t += 1.0 / 60; if (i % 8 == 7) gm.render(t); } };
    auto capture = [&](const char* nom, const char* quoi) {
        gm.render(t); gm.render(t + 0.01);
        const std::string fichier = std::string("profil-") + lg + "-" + nom;
        r.capturePng(out + "/" + fichier + ".png");
        WFS_LOG("capture %s : écran %s, %s", fichier.c_str(), nomEcran(gm.ecran()), quoi);
    };
    int doigt = 70;
    auto appui = [&](float x, float y) { t += 0.2; gm.onTouch({doigt, x, y, true}); image(); gm.onTouch({doigt, x, y, false}); image(); doigt++; };
    // WFS-557 : depuis WFS-550 (maquette 01), le Profil s'ouvre par le bloc « RECORD » du menu (plus de bouton « Profil »)
    auto appuyer = [&](const char* nom) {
        float x, y;
        if (std::string(nom) == "bouton" ? !menuBouton("record", x, y) : !centre(nom, x, y)) return false;
        appui(x, y); return true;
    };
    auto touche = [&](const std::string& nom) { float x, y; if (!centreCreationProfil(nom, x, y)) return false; appui(x, y); return true; };
    auto retourTelephone = [&]() { retourSysteme(gm, true, 0); image(); retourSysteme(gm, false, 0); images(3); };
    gm.allerA(Ecran::Menu);
    images(60);
    capture("menu", "menu principal : bouton « Profil » au-dessus de « Cadeau du jour »");
    // RG-01 : menu → Profil
    verifier(appuyer("bouton"), "bloc « RECORD » du menu → Profil (WFS-550)");
    images(40);
    verifier(ouvert(), "RG-01 : menu → Profil");
    capture("neuf", "profil d'un joueur neuf : 0 course, records « — », liste vide");
    verifier(lignesHistorique() == 0, "RG-06 : profil neuf : liste vide");
    // RG-01 : touche Retour d'Android → menu ; on revient
    retourTelephone();
    verifier(!ouvert() && gm.ecran() == Ecran::Menu, "RG-01 : touche Retour d'Android → menu");
    appuyer("bouton"); images(20);
    // RG-07 : « Modifier » → formulaire de WFS-481 ; nouveau pseudo ; « Valider »
    verifier(appuyer("modifier") && (images(3), creationProfilOuverte()) && modificationProfilOuverte(), "RG-07 : « Modifier » → formulaire de WFS-481");
    for (int i = 0; i < 16; i++) touche("<");
    const std::string pseudo1 = "Nadia";
    for (char c : std::string("nadia")) touche(std::string(1, c));
    capture("modification", "« Modifier » : formulaire de WFS-481, nouveau pseudo en cours, « Valider » et « Annuler »");
    verifier(touche("valider"), "« Valider » dessiné");
    images(3);
    verifier(!creationProfilOuverte() && ouvert() && sauvegarde().nick == pseudo1 && sauvegarde().profil.plaque == (choix == ui::langue::Choix::Francais ? "KARIMA" : "RYDER"),
             "RG-07 : « Valider » : pseudo écrit tout de suite, plaque gardée, retour au Profil");
    capture("apres-modification", "retour au Profil : nouveau pseudo en tête");
    // RG-07 : « Annuler » ne change rien ; Retour d'Android pendant la modification = « Annuler »
    appuyer("modifier"); images(3);
    touche("x");
    touche("annuler"); images(3);
    verifier(!creationProfilOuverte() && ouvert() && sauvegarde().nick == pseudo1, "RG-07 : « Annuler » : rien ne change");
    appuyer("modifier"); images(3);
    touche("x");
    retourTelephone();
    verifier(!creationProfilOuverte() && ouvert() && sauvegarde().nick == pseudo1, "RG-01 / RG-07 : Retour d'Android pendant la modification = « Annuler »");
    // « Retour » → menu → course → fin → Profil : la course est en tête de liste, chiffres augmentés
    verifier(appuyer("retour") && !ouvert(), "« Retour » → menu");
    images(10);
    courseBenchRules(true);
    gm.allerA(Ecran::Course); pas(60 * 8);
    verifier(gm.ecran() == Ecran::Course && activeCourse() && activeCourse()->state == RaceState::Play, "course en cours");
    gm.setAutopilot(false); courseBenchSetTime(0.05); pas(90);
    verifier(activeCourse() && activeCourse()->state == RaceState::Over, "course → écran de fin");
    gm.allerA(Ecran::Menu); images(30);
    courseBenchRules(false);
    appuyer("bouton"); images(30);
    const Profil& p = sauvegarde().profil;
    verifier(ouvert() && p.courses == 1 && lignesHistorique() == 1 && !p.dernieres.empty() && p.dernieres[0].voiture == "golf",
             "parcours : après la course, 1 course, la course en tête de liste");
    verifier(p.metres > 0 && p.vmax > 0, "parcours : kilomètres et vitesse maximale augmentés");
    capture("apres-course", "après une course : la course en tête de liste, chiffres augmentés, voiture préférée");
    WFS_LOG("profil (WFS-482) banc [%s] : %s", lg.c_str(), p.texte(sauvegarde().nick).c_str());
    // profil plein : 21 courses de plus (modes, voitures et résultats variés) → 20 lignes, la plus ancienne effacée
    {
        Profil& pm = sauvegarde().profil;
        const char* voitures[] = {"m4", "giulia", "golf", "m4", "p208", "gt86"};
        for (int i = 0; i < 21; i++) {
            CourseJouee c;
            c.mode = (ModeJoue)(i % 4);
            c.voiture = voitures[i % 6];
            c.date = 1794481200 + 3600LL * (i + 1) * 5;
            c.metres = 1500 + 230 * i; c.vmax = 180 + 3 * i; c.frolements = i % 5;
            c.quittee = i % 7 == 6;
            c.victoire = i % 3 == 0;
            c.place = 1 + i % 5; c.tempsMs = 95000 + 1730 * i; c.score = 900 + 137 * i;
            pm.noterCourse(c);
        }
        sauvegarde().best = 48210; sauvegarde().rec5Place = 2; sauvegarde().rec5Ms = 154320;
        sauvegarde().recPVictoire = true; sauvegarde().recPScore = 87; sauvegarde().recPDistM = 4200;
    }
    fermer(); ouvrir(); images(20);
    verifier(p.courses == 22 && lignesHistorique() == 20, "RG-06 : 22 courses jouées → 20 lignes");
    capture("plein", "profil plein : 20 dernières courses, records des 3 modes, voiture préférée");
    {   // RG-06 : la liste défile au doigt
        float x, y;
        if (centre("liste", x, y)) {
            gm.onTouch({90, x, y, true}); image();
            for (int i = 1; i <= 12; i++) { gm.onTouch({90, x, y - 40.f * i, true}); image(); }
            gm.onTouch({90, x, y - 480, false}); image();
        }
        verifier(defilementMax() > 0 && defilement() > 100 && ouvert(), "RG-06 : la liste défile au doigt, écran gardé");
        capture("plein-defile", "liste défilée au doigt vers les plus anciennes courses");
    }
    // RG-08 : pseudo de 16 « W » et plaque de 8 « W » : rien de coupé
    sauvegarde().nick = "WWWWWWWWWWWWWWWW"; sauvegarde().profil.plaque = "WWWWWWWW";
    fermer(); ouvrir(); images(20);
    verifier(pseudoEntier(), "RG-08 : pseudo de 16 « W » dessiné en entier");
    capture("16w", "pseudo de 16 « W » et plaque « WWWWWWWW » sans coupure");
    // « Modifier » avec le pseudo de 16 « W » (formulaire de WFS-481)
    appuyer("modifier"); images(5);
    verifier(creationProfilOuverte() && champsCreationEntiers(), "RG-08 : « Modifier » : pseudo de 16 « W » et curseur dans le champ");
    capture("16w-modification", "« Modifier » avec le pseudo de 16 « W »");
    touche("annuler"); images(3);
    appuyer("retour"); images(10);
    verifier(!ouvert() && gm.ecran() == Ecran::Menu, "« Retour » → menu");
    gm.shutdown();
    sauvegarde().desactiver();
    std::remove(chemin.c_str());
    return ecarts;
}
} // namespace

int benchProfil(Game& gm, Renderer& r, const std::string& out) {
    ui::chargerPolice(r);
    ui::langue::charger();
    int ecarts = 0;
    for (const ui::langue::Choix c : {ui::langue::Choix::Francais, ui::langue::Choix::Anglais}) {
        if (c == ui::langue::Choix::Anglais) { ui::langue::viderJournal(); ui::langue::journalActif(true, "profil"); }
        ecarts += benchUneLangue(gm, r, out, c);
        if (c == ui::langue::Choix::Anglais) {   // RC-E20-06 : aucune phrase française ni texte qui dépasse en anglais
            ui::langue::journalActif(false);
            const int e = ui::langue::verifierJournal();
            WFS_LOG("profil (WFS-482) banc [en] : RC-E20-06 : %d texte(s) français ou qui dépasse(nt) : %s", e, e ? "ÉCART" : "OK");
            ecarts += e;
        }
    }
    ui::langue::choisir(ui::langue::Choix::Automatique);
    WFS_LOG("profil (WFS-482) banc : parcours complet FR + EN : %d écart(s)%s", ecarts, ecarts ? " ÉCHEC" : " OK");
    return ecarts;
}

namespace {
// système : panneau par-dessus le menu (450), le bandeau des paquets (458), les Réglages (460) et le cadeau (466) ;
// touchers reçus par le menu. Le formulaire « Modifier » (WFS-481, 459) se dessine seul : l'écran Profil s'efface dessous.
class ProfilEcran : public System {
public:
    const char* name() const override { return "profil-ecran"; }
    void onEcran(Ecran, Ecran, const GameView&) override { fermer(); }
    void drawUi(Renderer& r, const GameView& v) override {
        if (v.ecran == Ecran::Menu && !v.sceneDediee && !creationProfilOuverte()) dessiner(r, v.uiWidth, v.uiHeight, v.marges, v.time);
    }
    void shutdown(Renderer&) override { fermer(); }
};
} // namespace

} // namespace ecrans::profil_ecran
} // namespace wfs

WFS_SYSTEM(wfs::ecrans::profil_ecran::ProfilEcran, 468)   // interface : par-dessus le cadeau du jour (466), sous les Crédits (470)
