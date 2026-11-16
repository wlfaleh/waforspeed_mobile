// WaForSpeed — écran des conditions d'utilisation (WFS-308) : voir cgu.h.
// Style design-street-racing des Réglages : fond --bg, panneau --surface à liseré cyan, sur-titre cyan, titre en capitales,
// bouton principal orange en biseau, bouton secondaire vitré ; même unité que les autres écrans (H / 720, dp sur téléphone couché).
#include "cgu.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include "../../core/log.h"
#include "../../core/platform.h"
#include "../../game/commandes_tactiles.h"   // dessin2d::biseau
#include "../../game/game.h"
#include "../../game/sauvegarde.h"
#include "../../render/renderer.h"
#include "../../render/vk.h"   // rotation de l'écran avant le chargement (Cgu::image)
#include "../petit_ecran.h"
#include "../langue.h"
#include "../text.h"
#include "../boutons.h"   // WFS-549 : boutons au modèle R19
#include "../verre.h"   // WFS-548 (R19) : fenêtre de verre, sur-titre gris, intertitres orange clair, fond flouté
#include "cgu_appui.h"
#include "reglages.h"

namespace wfs {
namespace ecrans {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SURFACE2 = 0x1E2130,
               DANGER = 0xFF1F4B;
const unsigned ACCENT_PRESSE = 0xFF9A4D;   // WFS-355 : « Accepter » sous le doigt (même teinte que les Réglages, WFS-290)
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.16f};

// texte d'un fichier : assets de l'APK (cgu/…, copiés par android/app/build.gradle), sinon store/ (PC, depuis la racine du dépôt)
std::string lireTexte(const char* nom) {
    std::vector<uint8_t> a;
    if (readAsset(std::string("cgu/") + nom, a)) return std::string(a.begin(), a.end());
    std::ifstream f(std::string("store/") + nom, std::ios::binary);
    if (f) { std::ostringstream s; s << f.rdbuf(); return s.str(); }
    WFS_ERR("CGU (WFS-308) : %s introuvable (assets cgu/ ni store/)", nom);
    return "";
}

// WFS-508 (RG-01) : texte dans la langue du jeu ; texte anglais introuvable → texte français (jamais un écran vide)
std::string lireTexteLangue(bool confidentialite) {
    const bool anglais = ui::langue::active() == ui::langue::Langue::Anglais;
    if (anglais) {
        std::string t = lireTexte(fichierTexteCgu(confidentialite, true));
        if (!t.empty()) return t;
        WFS_ERR("CGU (WFS-508) : texte anglais introuvable, texte français affiché");
    }
    return lireTexte(fichierTexteCgu(confidentialite, false));
}


} // namespace

void Cgu::ouvrir(Renderer& r, Mode m) {
    r_ = &r; mode_ = m; etat_ = Etat::Ouvert; coche_ = false; defil_ = 0; tRefus_ = -1; doigt_ = -1; cible_ = 0;
    // WFS-508 : textes relus à chaque ouverture, dans la langue active (Réglages → Langue puis « Terms of use » : anglais)
    // WFS-557 (RG-19, maquette 22) : deux onglets, « Conditions » et « Confidentialité », chacun avec son texte ; « J'accepte »
    // s'allume quand les deux ont été lus jusqu'en bas (remplace la case « J'ai lu et j'accepte »)
    textes_[0] = paragraphesMarkdown(lireTexteLangue(false));
    textes_[1] = paragraphesMarkdown(sansNotesStore(lireTexteLangue(true)));
    onglet_ = 0; lu_[0] = lu_[1] = false; defilOnglet_[0] = defilOnglet_[1] = 0;
    paras_ = textes_[0];
    largeurPage_ = -1;
    WFS_LOG("CGU (WFS-308) : écran ouvert (%s), version %d, langue %s, %zu + %zu paragraphes", m == Mode::Premier ? "premier lancement" : "Réglages, lecture seule",
            VERSION_CGU, ui::langue::code(), textes_[0].size(), textes_[1].size());
}

// WFS-557 (RG-19) : onglet affiché (texte remis en page, défilement de l'onglet gardé)
void Cgu::choisirOnglet(int o) {
    if (o == onglet_ || o < 0 || o > 1) return;
    defilOnglet_[onglet_] = defil_;
    onglet_ = o; paras_ = textes_[o]; defil_ = defilOnglet_[o]; largeurPage_ = -1;
    WFS_LOG("CGU (WFS-557) : onglet « %s »", o == 0 ? "Conditions" : "Confidentialité");
}

// WFS-557 (RG-19) : refus par la touche Retour d'Android au premier lancement : même message et même fermeture qu'avant
void Cgu::refuser() {
    if (!ouvert() || mode_ != Mode::Premier || etat_ == Etat::Refuse) return;
    etat_ = Etat::Refuse; tRefus_ = maintenant_;
    WFS_LOG("CGU (WFS-557) : refus par la touche Retour — %s", ui::tr("cgu.refus").c_str());
}

// découpe des paragraphes en lignes de la largeur du cadre (mots entiers ; suite d'une puce alignée sur son texte)
void Cgu::mettreEnPage(float largeur, float k) {
    largeurPage_ = largeur; kPage_ = k; lignes_.clear(); tropLongues_ = 0;
    // WFS-557 (maquette 22) : titre en Rajdhani italique 22, ligne « Version » en Inter 12 grise, intertitres en Inter Medium 12
    // (capitales orange clair), texte en Inter 14 ; interligne 1,5
    const float corps = 14 * k, titre = 22 * k, sous = 12 * k, version = 12 * k;
    float y = 0;
    for (const ParagrapheCgu& p : paras_) {
        if (p.style == ParagrapheCgu::VIDE) { y += corps * 0.5f; continue; }
        const bool estVersion = p.style == ParagrapheCgu::TEXTE && p.texte.rfind("Version ", 0) == 0;
        const float t = p.style == ParagrapheCgu::TITRE ? titre : p.style == ParagrapheCgu::SOUS_TITRE ? sous : estVersion ? version : corps;
        const ui::Police police = p.style == ParagrapheCgu::TITRE ? ui::Police::Rajdhani : p.style == ParagrapheCgu::SOUS_TITRE ? ui::Police::InterMedium : ui::Police::Inter;
        const ui::AvecPolice av(police);
        const float esp = p.style == ParagrapheCgu::SOUS_TITRE ? 0.14f : 0.f;
        const float interligne = p.style == ParagrapheCgu::TITRE ? t * 1.15f : estVersion ? t * 1.9f : p.style == ParagrapheCgu::SOUS_TITRE ? t * 1.9f : t * 1.5f;
        const std::string puce = "• ";
        const float retrait = p.style == ParagrapheCgu::PUCE ? ui::width(t, puce) : 0;
        if (p.style == ParagrapheCgu::SOUS_TITRE && y > 0) y += corps * 0.5f;
        std::string ligne = p.style == ParagrapheCgu::PUCE ? puce : "";
        bool premiere = true;
        size_t i = 0;
        const std::string s = p.style == ParagrapheCgu::SOUS_TITRE ? ui::majuscules(p.texte) : p.texte;
        auto pousser = [&]() {
            // WFS-508 (RG-06) : un mot plus long que le cadre (adresse) dépasserait : compté pour le banc « textes »
            if (ui::width(t, ligne, esp) > largeur - (premiere ? 0 : retrait) + 0.5f) tropLongues_++;
            lignes_.push_back({p.style, ligne, premiere ? 0.f : retrait, y, t, (int)police, estVersion});
            y += interligne; premiere = false; ligne.clear();
        };
        while (i < s.size()) {
            size_t f = s.find(' ', i);
            if (f == std::string::npos) f = s.size();
            const std::string mot = s.substr(i, f - i);
            i = f + 1;
            if (mot.empty()) continue;
            const std::string essai = ligne.empty() || ligne == puce ? ligne + mot : ligne + " " + mot;
            const float dispo = largeur - (premiere ? 0 : retrait);
            if (!ligne.empty() && ligne != puce && ui::width(t, essai, esp) > dispo) { pousser(); ligne = mot; }
            else ligne = essai;
        }
        if (!ligne.empty()) pousser();
    }
    hauteurTexte_ = y;
}

// WFS-557 (maquette 22 ; arbitrages du PO du 2026-11-16 : maquette à 100 %, RG-19). Unité k = H / 720 (maquette 1600 × 720).
// Fenêtre de verre 1078 × 558 ; sur-titre « Avant de rouler », titre ; onglets « Conditions / Confidentialité » en haut à droite ;
// cadre du texte défilant ; bouton rond « J'accepte » (grisé tant que les deux onglets ne sont pas lus jusqu'en bas) à droite, hors
// de la fenêtre ; phrase « Fais défiler pour tout lire… » sous la fenêtre. Lecture depuis les Réglages : bouton rond « Fermer ».
void Cgu::dessiner(Renderer& r, float W, float H, const Marges& m, double maintenant) {
    maintenant_ = maintenant;
    if (etat_ == Etat::Refuse && tRefus_ >= 0 && maintenant - tRefus_ >= CGU_DELAI_FERMETURE) {
        etat_ = Etat::Quitter;
        WFS_LOG("CGU (WFS-308) : refus, fermeture de l'application");
    }
    if (!ouvert() || !ui::fontReady()) return;
    const float k = std::min(H / 720.f, (W - m.gauche - m.droite) / 1600.f);   // 16:9 : maquette réduite pour tenir en largeur (revue #432)
    dp_ = ui::pixelsParDp(W);
    if (mode_ == Mode::Premier) r.uiRect(0, 0, W, H, hex(BG));   // 1er lancement : aucune scène derrière
    else { verre::fenetreSurEcran(r); verre::assombrir(r, W, H, 0.55f); }   // ouvertes depuis les Réglages : menu ou pause dessous, couverts
    const float ox = m.gauche + (W - m.gauche - m.droite - 1600 * k) / 2, oy = m.haut + std::max(0.f, (H - m.haut - m.bas - 720 * k) / 2);   // maquette centrée
    const float px = ox + 261 * k, py = oy + 41 * k, pw = 1078 * k, ph = 558 * k;
    { verre::Style st; st.rayon = 22 * k; st.lisere = 1.5f * k; verre::panneau(r, px, py, pw, ph, st); }
    verre::surTitre(r, px + 31 * k, py + 27 * k, 15 * k, ui::tr(mode_ == Mode::Premier ? "cgu.surtitre.premier" : "cgu.surtitre.reglages", std::to_string(VERSION_CGU)));
    ui::textBoite(r, px + 33 * k, py + 40 * k, 680 * k, 54 * k, 54 * k, ui::majuscules(ui::tr("cgu.titre")), hex(TEXTE), ui::GAUCHE, 0.01f, false, true);
    {   // onglets
        const float in = 4 * k, tp = 14 * k, pad = 20 * k, h = 44 * k;
        float ow[2], tot = 2 * in;
        { ui::AvecPolice inter(ui::Police::InterMedium);
          for (int o = 0; o < 2; o++) { ow[o] = ui::width(tp, ui::majuscules(ui::tr(o == 0 ? "cgu.onglet.conditions" : "cgu.onglet.confidentialite")), 0.10f) + 2 * pad; tot += ow[o]; } }
        const float x0 = px + pw - 30 * k - tot, y0 = py + 29 * k;
        ui::boutons::selecteurFond(r, x0, y0, tot, h);
        float xo = x0 + in;
        for (int o = 0; o < 2; o++) {
            zOnglets_[o] = {xo, y0, ow[o], h};
            ui::boutons::selecteurChoix(r, xo, y0 + in, ow[o], h - 2 * in, "", o == onglet_);
            ui::AvecPolice inter(ui::Police::InterMedium);
            ui::text(r, xo + ow[o] / 2, y0 + (h - tp) / 2, tp, ui::majuscules(ui::tr(o == 0 ? "cgu.onglet.conditions" : "cgu.onglet.confidentialite")),
                     o == onglet_ ? hex(BG) : hex(0xD7DAE2), ui::CENTRE, 0.10f, false);
            xo += ow[o];
        }
    }
    // bouton rond : « J'accepte » (premier lancement) ou « Fermer » (lecture) ; 146 px, centre (1435, 544)
    const float d = 146 * k, bx = ox + 1435 * k - d / 2, by = oy + 544 * k - d / 2;
    zCase_ = zRefuser_ = {};
    if (mode_ == Mode::Premier) {
        zAccepter_ = {bx, by, d, d}; zFermer_ = {};
        const bool presse = coche_ && doigt_ >= 0 && cible_ == cgu_appui::ACCEPTER;
        ui::boutons::startStop(r, bx, by, d, d, ui::majuscules(ui::tr("cgu.jaccepte")), 1, presse, "", 22 * k, !coche_);
    } else {
        zFermer_ = {bx, by, d, d}; zAccepter_ = {};
        ui::boutons::startStop(r, bx, by, d, d, ui::majuscules(ui::tr("cgu.fermer")), 1, doigt_ >= 0 && cible_ == cgu_appui::FERMER, "", 22 * k);
    }
    { ui::AvecPolice inter(ui::Police::Inter);
      ui::text(r, ox + 286 * k, oy + 644 * k, 14 * k, ui::tr("cgu.aide"), hex(0xA3A8B5), ui::GAUCHE, 0, false); }
    // cadre du texte défilant
    const float tx = px + 31 * k, ty = py + 101 * k, tw = pw - 62 * k, th = 390 * k, pad = 22 * k;
    zTexte_ = {tx, ty, tw, th};
    { vec2 q[ui::boutons::MAX_SOMMETS]; const int n = ui::boutons::contourArrondi(q, tx, ty, tw, th, 12 * k);
      dessin2d::polygone(r, q, n, {hex(0x16181E, 0.85f), hex(0x0E1014, 0.90f), ty, ty + th}); }
    const float largeur = tw - 2 * pad - 16 * k;
    if (std::fabs(largeur - largeurPage_) > 0.5f || std::fabs(k - kPage_) > 1e-4f) mettreEnPage(largeur, k);
    const float vue = th - 2 * pad + 6 * k;
    defilMax_ = std::max(0.f, hauteurTexte_ - vue);
    defil_ = std::clamp(defil_, 0.f, defilMax_);
    // RG-19 : onglet lu quand son bas a été atteint ; « J'accepte » s'allume quand les deux le sont
    if (defil_ >= defilMax_ - 1) lu_[onglet_] = true;
    const bool tout = lu_[0] && lu_[1];
    if (tout && !coche_) WFS_LOG("CGU (WFS-557) : les deux onglets ont été lus jusqu'en bas : « J'accepte » actif");
    coche_ = tout;
    for (const Ligne& l : lignes_) {   // lignes entièrement visibles seulement (pas de découpe du texte par l'interface)
        const float y = l.y - defil_;
        if (y < 0 || y + l.taille * 1.2f > vue) continue;
        const vec4 c = l.style == ParagrapheCgu::TITRE ? hex(TEXTE) : l.style == ParagrapheCgu::SOUS_TITRE ? verre::hexa(verre::INTERTITRE) : l.version ? hex(0xA3A8B5) : hex(0xE6E8EE);
        const ui::AvecPolice av((ui::Police)l.police);
        ui::text(r, tx + pad + l.x, ty + pad + y, l.taille, l.texte, c, ui::GAUCHE, l.style == ParagrapheCgu::SOUS_TITRE ? 0.14f : 0.f, false, l.style == ParagrapheCgu::TITRE);
    }
    if (defilMax_ > 0) {   // ascenseur fin
        const float bxs = tx + tw - 12 * k, hb = std::max(24 * k, vue * vue / hauteurTexte_), yb = ty + pad + (vue - hb) * (defil_ / defilMax_);
        r.uiRect(bxs, yb, 3 * k, hb, hex(TEXTE, 0.30f));
    }
    if (etat_ == Etat::Refuse) {   // RG-04 : message, puis fermeture après CGU_DELAI_FERMETURE
        r.uiRect(0, 0, W, H, hex(BG, 0.94f));
        ui::text(r, W / 2, H / 2 - 30 * k, 30 * k, ui::tr("cgu.refus"), hex(DANGER), ui::CENTRE, 0.02f, false);
        ui::AvecPolice inter(ui::Police::Inter);
        ui::text(r, W / 2, H / 2 + 16 * k, 17 * k, ui::tr("cgu.fermeture"), hex(MUTED), ui::CENTRE, 0, false);
    }
}

void Cgu::dessinerImage(double maintenant) {
    if (!r_) return;
    FrameParams fp;
    fp.view = mat4::identity(); fp.proj = mat4::identity(); fp.camPos = {0, 0, 0};
    if (!r_->beginFrame(fp)) { maintenant_ = maintenant; return; }
    dessiner(*r_, (float)r_->width(), (float)r_->height(), Marges{}, maintenant);
    r_->endFrame();
}

bool Cgu::toucher(int id, float x, float y, bool down, bool premierDoigt) {
    if (!ouvert()) return false;
    if (etat_ == Etat::Refuse) return true;   // message du refus : plus rien n'est touchable
    namespace ca = cgu_appui;
    auto rect = [](const Zone& z) { return ca::Rect{z.x, z.y, z.w, z.h}; };
    const int sous = ca::cibleSous(rect(zCase_), rect(zAccepter_), rect(zRefuser_), rect(zFermer_), rect(zTexte_), dp_, x, y);
    static const char* NOMS[] = {"rien", "texte", "case", "Accepter", "Refuser", "Fermer"};
    if (down) {
        if (premierDoigt && doigt_ >= 0) {   // WFS-355 : le lever de l'appui précédent n'est jamais arrivé ; nouvel appui
            WFS_LOG("CGU (WFS-355) : lever du doigt %d jamais reçu (cible %s), oublié", doigt_, NOMS[cible_]);
            doigt_ = -1; cible_ = 0;
        }
        if (doigt_ < 0) {
            doigt_ = id; yDoigt_ = y; cible_ = sous;
            WFS_LOG("CGU (WFS-355) : doigt posé (%.0f, %.0f) sur %s", x, y, NOMS[sous]);
        } else if (id == doigt_ && cible_ == ca::TEXTE) {   // glisser : le texte suit le doigt
            defil_ = std::clamp(defil_ - (y - yDoigt_), 0.f, defilMax_);
            yDoigt_ = y;
        }
        return true;
    }
    const bool poseConnue = doigt_ >= 0;
    if (poseConnue && id != doigt_) return true;   // autre doigt (le premier posé décide)
    // pose perdue : seulement au premier lancement (appuis du veilleur) ; en lecture des Réglages, le lever du doigt qui a ouvert
    // l'écran arrive sans pose et ne doit rien toucher (« Fermer » est dessiné à cet endroit sur téléphone)
    if (!poseConnue && mode_ != Mode::Premier) return true;
    // WFS-557 (RG-19) : onglet touché (au lever, sur l'onglet où le doigt s'est posé)
    for (int o = 0; o < 2; o++) if (zOnglets_[o].contient(x, y) && cible_ != ca::TEXTE) { doigt_ = -1; cible_ = 0; choisirOnglet(o); return true; }
    const int c = ca::cibleAuLever(poseConnue, cible_, sous);
    WFS_LOG("CGU (WFS-355) : doigt levé (%.0f, %.0f) sur %s%s → %s", x, y, NOMS[sous], poseConnue ? "" : " (pose jamais reçue)", NOMS[c]);
    doigt_ = -1; cible_ = 0;
    if (c == ca::ACCEPTER) {
        if (!coche_) WFS_LOG("CGU (WFS-557) : « J'accepte » inactif (les deux onglets ne sont pas lus jusqu'en bas)");
        else { etat_ = Etat::Accepte; WFS_LOG("CGU (WFS-308) : « Accepter » (version %d)", VERSION_CGU); }
    } else if (c == ca::REFUSER) { etat_ = Etat::Refuse; tRefus_ = maintenant_; WFS_LOG("CGU (WFS-308) : « Refuser » — %s", ui::tr("cgu.refus").c_str()); }
    else if (c == ca::FERMER) { etat_ = Etat::Ferme; WFS_LOG("CGU (WFS-308) : lecture fermée"); }
    return true;
}

void Cgu::noterAcceptation() {
    Sauvegarde& s = sauvegarde();
    s.cgu = VERSION_CGU;
    if (s.actif()) s.enregistrer();
    etat_ = Etat::Ferme;
    WFS_LOG("CGU (WFS-308) : version %d acceptée, gardée dans la sauvegarde", VERSION_CGU);
}

bool Cgu::demarrer(Renderer& r) {
    if (!cguAMontrer(sauvegarde().actif(), sauvegarde().cgu)) return false;
    ui::chargerPolice(r);
    ouvrir(r, Mode::Premier);
    essaisRotation_ = 0;
    return true;
}

void Cgu::perdreFenetre(Renderer& r) {
    if (etat_ == Etat::Accepte && mode_ == Mode::Premier) noterAcceptation();   // « Accepter » dans le même lot : acceptation gardée, chargement au retour
    ui::libererPolice(r);   // sinon l'atlas reste lié à l'ancien appareil Vulkan et chargerPolice ne le refait pas (F.ready)
    etat_ = Etat::Ferme;
    WFS_LOG("CGU (WFS-308) : fenêtre perdue, police libérée (écran rouvert au retour)");
}

Cgu::Etat Cgu::image(VkCtx& vk, uint32_t lf, uint32_t hf, double maintenant) {
    if (etat_ == Etat::Accepte) { noterAcceptation(); return Etat::Accepte; }
    if (etat_ == Etat::Quitter) { etat_ = Etat::Ferme; return Etat::Quitter; }
    if (!premierLancement() || !r_) return etat_;   // lecture des Réglages : dessinée par le jeu, jamais ici
    // l'appli démarre souvent en portrait puis passe en paysage (même contrôle que la boucle de jeu d'android_main.cpp)
    const bool tourne = (lf >= hf) != (r_->width() >= r_->height());
    if (vk.surfaceChanged() || (tourne && essaisRotation_++ < 8)) { vk.waitIdle(); if (vk.createSwapchain()) r_->resize(); }
    r_->setViewSize(lf, hf);
    dessinerImage(maintenant);
    // WFS-355 : une image de plus d'1 s laisse le veilleur lire les appuis (WFS-284) ; noté pour le diagnostic sur téléphone
    const double duree = nowSeconds() - maintenant;
    if (duree > 0.5 && imagesLentes_ < 5) { imagesLentes_++; WFS_LOG("CGU (WFS-355) : image lente, %d ms", (int)std::lround(duree * 1000)); }
    return etat_;
}

Cgu& cgu() { static Cgu c; return c; }

// ---------------------------------------------------------------- banc « textes » (WFS-508, RG-06) : écran des CGU dans la langue
// du banc (WFS_LANGUE) ; captures haut, milieu, bas ; journal des textes (aucune phrase française en anglais), texte long qui
// défile sans dépasser, boutons qui ne se chevauchent pas
int benchCguTextes(Renderer& r, const std::string& out) {
    namespace lg = ui::langue;
    int echecs = 0;
    auto verifier = [&](bool ok, const char* quoi) { WFS_LOG("textes (WFS-508) : CGU : %s : %s", quoi, ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; };
    ui::chargerPolice(r);
    lg::choisir(lg::Choix::Automatique);
    const std::string code = lg::code();
    Cgu& c = cgu();
    double t = 0;
    auto image = [&](const char* ecran) { t += 1.0 / 60; lg::journalActif(true, ecran); c.dessinerImage(t); lg::journalActif(false); };
    auto capture = [&](const char* ecran, const char* quoi) {
        image(ecran); image(ecran);
        const std::string nom = "textes-" + code + "-" + ecran;
        r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : langue %s, défilement %.0f / %.0f, %s", nom.c_str(), code.c_str(), c.defilement(), c.defilementMax(), quoi);
    };
    // glisser dans le texte jusqu'à la part voulue du défilement (0 haut, 1 bas)
    auto defiler = [&](float part) {
        const Cgu::Zone& z = c.zoneTexte();
        const float x = z.x + z.w / 2;
        for (int n = 0; n < 200 && c.defilement() < part * c.defilementMax() - 1; n++) {
            const float y0 = z.y + z.h * 0.8f, pas = std::min(z.h * 0.6f, part * c.defilementMax() - c.defilement());
            c.toucher(7, x, y0, true); c.toucher(7, x, y0 - pas, true); c.toucher(7, x, y0 - pas, false);
        }
    };
    lg::viderJournal();
    c.ouvrir(r, Cgu::Mode::Premier);
    capture("cgu-haut", "premier lancement, haut du texte");
    verifier(c.ouvert() && c.defilementMax() > 0, "texte long : il défile (plus haut que le cadre)");
    verifier(c.lignesTropLongues() == 0, "aucune ligne plus large que le cadre");
    {   // WFS-557 (RG-19) : onglets, cadre du texte et « J'accepte » sans chevauchement
        const Cgu::Zone &o1 = c.zoneOnglet(1), &zt = c.zoneTexte(), &za = c.zoneAccepter();
        verifier(o1.y + o1.h <= zt.y && zt.x + zt.w <= za.x + za.w * 0.2f && o1.x + o1.w <= za.x + za.w, "onglets, texte et « J'accepte » sans chevauchement");
    }
    defiler(0.5f);
    capture("cgu-milieu", "texte défilé à mi-hauteur");
    verifier(c.defilement() > 0 && c.defilement() < c.defilementMax(), "milieu du texte atteint au doigt");
    defiler(1.f); image("cgu-bas");
    {   // WFS-557 (RG-19) : onglet « Confidentialité » lu jusqu'en bas aussi : « J'accepte » actif
        const Cgu::Zone& o = c.zoneOnglet(1);
        c.toucher(8, o.x + o.w / 2, o.y + o.h / 2, true); c.toucher(8, o.x + o.w / 2, o.y + o.h / 2, false);
        image("cgu-bas"); defiler(1.f);
    }
    capture("cgu-bas", "« Confidentialité », bas du texte, les deux onglets lus : « J'accepte » actif");
    verifier(c.defilement() >= c.defilementMax() - 1 && c.caseCochee(), "bas des deux textes atteint : « J'accepte » actif");
    c.ouvrir(r, Cgu::Mode::Lecture);
    capture("cgu-lecture", "Réglages : lecture seule, bouton « Fermer »");
    verifier(c.zoneFermer().w > 0, "lecture : bouton « Fermer » présent");
    c.fermer();
    echecs += lg::verifierJournal();
    lg::viderJournal();
    WFS_LOG("textes (WFS-508) : CGU %s : %d échec(s) → %s", code.c_str(), echecs, echecs ? "ROUGE" : "VERT");
    return echecs;
}

// ---------------------------------------------------------------- banc d'essai (WFS_BANC=cgu)
int benchCgu(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    auto verifier = [&](bool ok, const char* quoi) { WFS_LOG("CGU (WFS-308) banc : %s : %s", quoi, ok ? "OK" : "ÉCART"); if (!ok) ecarts++; };
    ui::chargerPolice(r);
    Cgu& c = cgu();
    double t = 0;
    auto image = [&]() { t += 1.0 / 60; c.dessinerImage(t); };
    auto capture = [&](const char* nom, const char* quoi) { image(); r.capturePng(out + "/" + nom + ".png"); WFS_LOG("capture %s : %s", nom, quoi); };
    auto appui = [&](int id, const Cgu::Zone& z) { const float x = z.x + z.w / 2, y = z.y + z.h / 2; c.toucher(id, x, y, true); image(); c.toucher(id, x, y, false); image(); };
    // RG-06 : installation neuve (sauvegarde du banc effacée)
    const std::string chemin = out + "/cgu-sauvegarde-banc.txt";
    std::remove(chemin.c_str());
    sauvegarde().charger(chemin);
    verifier(cguAMontrer(sauvegarde().actif(), sauvegarde().cgu), "installation neuve → écran des CGU");
    c.ouvrir(r, Cgu::Mode::Premier);
    image();
    capture("cgu-premier", "premier lancement, « J'accepte » grisé (onglets pas encore lus)");
    appui(1, c.zoneAccepter());
    verifier(c.etat() == Cgu::Etat::Ouvert, "« J'accepte » avant la lecture : rien ne se passe");
    {   // défilement : le doigt remonte de 60 % du cadre
        const Cgu::Zone& z = c.zoneTexte();
        const float x = z.x + z.w / 2, y0 = z.y + z.h * 0.8f;
        c.toucher(2, x, y0, true);
        for (int i = 1; i <= 10; i++) { c.toucher(2, x, y0 - z.h * 0.06f * i, true); image(); }
        c.toucher(2, x, y0 - z.h * 0.6f, false); image();
        WFS_LOG("CGU (WFS-308) banc : défilement %.0f / %.0f px", c.defilement(), c.defilementMax());
        verifier(c.defilement() > 0, "le texte défile au doigt");
        capture("cgu-defile", "texte défilé (suite des CGU)");
    }
    // WFS-557 (RG-19) : lire les deux onglets jusqu'en bas (remplace la case « J'ai lu et j'accepte »)
    auto basDuTexte = [&]() {
        const Cgu::Zone& z = c.zoneTexte();
        const float x = z.x + z.w / 2;
        for (int n = 0; n < 300 && c.defilement() < c.defilementMax() - 1; n++) {
            const float y0 = z.y + z.h * 0.85f;
            c.toucher(20, x, y0, true); c.toucher(20, x, y0 - z.h * 0.7f, true); c.toucher(20, x, y0 - z.h * 0.7f, false); image();
        }
    };
    auto lireTout = [&]() {
        basDuTexte();
        verifier(!c.caseCochee(), "RG-19 : un seul onglet lu : « J'accepte » encore grisé");
        appui(21, c.zoneOnglet(1));
        verifier(c.onglet() == 1, "RG-19 : onglet « Confidentialité » ouvert");
        basDuTexte();
    };
    lireTout();
    verifier(c.caseCochee(), "RG-19 : les deux onglets lus jusqu'en bas : « J'accepte » actif");
    capture("cgu-coche", "les deux onglets lus : « J'accepte » allumé");
    c.refuser(); image();
    verifier(c.etat() == Cgu::Etat::Refuse, "RG-19 : touche Retour → message du refus");
    capture("cgu-refus", "refus : « Le jeu ne peut pas démarrer sans votre accord »");
    for (int i = 0; i < (int)(CGU_DELAI_FERMETURE * 60) + 6; i++) image();
    verifier(c.etat() == Cgu::Etat::Quitter, "refus → fermeture de l'application");
    // relance après le refus : rien n'a été accepté, l'écran revient
    sauvegarde().attendre(); sauvegarde().charger(chemin);
    verifier(cguAMontrer(sauvegarde().actif(), sauvegarde().cgu), "relance après refus → écran des CGU à nouveau");
    verifier(c.demarrer(r), "demarrer : écran ouvert (police chargée)"); image();
    {   // fenêtre perdue pendant les CGU (touche Accueil, écran verrouillé) : police libérée, renderer fermé puis rouvert, écran rouvert
        VkCtx& vk = r.vk();
        c.perdreFenetre(r); r.shutdown();
        verifier(!c.ouvert() && !ui::fontReady(), "fenêtre perdue : écran fermé, police libérée");
        r.init(vk);
        verifier(c.demarrer(r) && ui::fontReady(), "fenêtre retrouvée : écran rouvert, police rechargée");
        capture("cgu-reprise", "après la perte de la fenêtre : texte lisible, « J'accepte » grisé");
    }
    verifier(!c.caseCochee(), "« J'accepte » grisé à la réouverture");
    {   // WFS-355 (RG-04) : appuis injectés comme sur le téléphone, débuts ou fins d'appui perdus, bords de la cible de 48 dp
        lireTout();
        const Cgu::Zone& za = c.zoneAccepter();
        const float dp = ui::pixelsParDp((float)r.width()), ax = za.x + za.w / 2, ay = za.y + za.h / 2;
        WFS_LOG("CGU (WFS-355) banc : « J'accepte » %.0f × %.0f px (%.0f × %.0f dp), cible %.0f dp de haut", za.w, za.h, za.w / dp, za.h / dp, std::max(za.h / dp, 48.f));
        c.toucher(6, ax, ay, true, true); image();
        capture("cgu-accepter-appuye", "WFS-355 : doigt posé sur « J'accepte » : teinte plus claire");
        const Cgu::Zone& zt = c.zoneTexte();
        c.toucher(6, zt.x + zt.w / 2, zt.y + zt.h / 2, false); image();
        verifier(c.etat() == Cgu::Etat::Ouvert, "WFS-355 : doigt glissé hors de « J'accepte » → annulé");
        c.toucher(0, zt.x + zt.w / 2, zt.y + zt.h / 2, true, true); image();   // posé sur le texte, lever jamais reçu
        c.toucher(0, ax, za.y + za.h - 2, true, true); image();               // nouvel appui (ACTION_DOWN), bas du bouton
        c.toucher(0, ax, za.y + za.h - 2, false); image();
        verifier(c.etat() == Cgu::Etat::Accepte, "WFS-355 : lever perdu sur le texte puis « J'accepte » au bord bas → accepté");
    }
    {   // fenêtre perdue dans le même lot qu'« Accepter » (avant Cgu::image) : chemin de termGraphics d'android_main.cpp
        verifier(c.tientLaFenetre() && !c.premierLancement(), "« J'accepte » pas encore traité : la fenêtre reste tenue par les CGU");
        // WFS-355 (cause) : la boucle d'android_main.cpp doit encore tourner sans attente pour que Cgu::image traite « Accepter »
        verifier(cgu_appui::boucleSansAttente(false, c.tientLaFenetre(), false), "WFS-355 : « Accepter » touché → boucle Android sans attente (image() suivante)");
        verifier(!cgu_appui::boucleSansAttente(false, c.premierLancement(), false), "WFS-355 : ancienne condition (premierLancement) : boucle bloquée, écran figé — cause du défaut");
        VkCtx& vk = r.vk();
        c.perdreFenetre(r); r.shutdown(); r.init(vk); ui::chargerPolice(r);
        verifier(!c.tientLaFenetre() && sauvegarde().cgu == VERSION_CGU, "fenêtre perdue après « Accepter » : acceptation gardée, renderer refermé");
        verifier(!c.demarrer(r), "fenêtre retrouvée après « Accepter » : pas d'écran des CGU, chargement normal");
    }
    sauvegarde().attendre(); sauvegarde().charger(chemin);
    WFS_LOG("CGU (WFS-308) banc : sauvegarde relue : cgu=%d", sauvegarde().cgu);
    verifier(!cguAMontrer(sauvegarde().actif(), sauvegarde().cgu), "relance après acceptation → pas d'écran des CGU");
    verifier(cguAMontrer(true, VERSION_CGU - 1), "version des CGU changée → écran à nouveau");
    // chargement → menu → Réglages → « Conditions d'utilisation » (lecture seule) → course → retour au menu
    if (!g.init(r, "golf", "ville")) { WFS_ERR("CGU (WFS-308) banc : chargement impossible"); sauvegarde().desactiver(); std::remove(chemin.c_str()); return ecarts + 1; }
    g.setAutopilot(true); g.camMode = 0;
    { std::string nick; sauvegarde().profil.creer("Pilote1", "WFS-001", nick); sauvegarde().nick = nick; }   // WFS-481 : profil créé (écran « Crée ton pilote » : WFS_BANC=creation)
    double tg = 1000;
    auto imageJeu = [&]() { g.update(1.0 / 60); tg += 1.0 / 60; g.render(tg); };
    auto captureJeu = [&](const char* nom, const char* quoi) { g.render(tg); g.render(tg + 0.01); r.capturePng(out + "/" + nom + ".png"); WFS_LOG("capture %s : écran %s, %s", nom, nomEcran(g.ecran()), quoi); };
    auto toucherJeu = [&](int id, float x, float y) { g.onTouch({id, x, y, true}); imageJeu(); g.onTouch({id, x, y, false}); imageJeu(); };
    for (int i = 0; i < 30; i++) g.update(1.0 / 60);
    g.allerA(Ecran::Menu);
    for (int i = 0; i < 60; i++) imageJeu();
    ouvrirReglagesBanc(true); imageJeu();
    float bx = 0, by = 0;
    verifier(boutonCguReglages(bx, by), "Réglages : bouton « Conditions d'utilisation » présent");
    captureJeu("reglages-cgu-bouton", "Réglages ouverts depuis le menu, bouton « Conditions d'utilisation »");
    toucherJeu(7, bx, by);
    verifier(c.ouvert() && c.mode() == Cgu::Mode::Lecture, "Réglages → texte en lecture seule");
    captureJeu("reglages-cgu-lecture", "conditions en lecture seule, bouton « Fermer »");
    { const Cgu::Zone& z = c.zoneFermer(); toucherJeu(8, z.x + z.w / 2, z.y + z.h / 2); }
    verifier(!c.ouvert() && reglagesOuverts(), "« Fermer » → retour aux Réglages");
    {   // fenêtre perdue en jeu pendant la lecture (touche Accueil) : chemin de termGraphics puis initGraphics d'android_main.cpp
        toucherJeu(9, bx, by);
        verifier(c.ouvert() && !c.premierLancement(), "lecture ouverte depuis les Réglages : pas le premier lancement");
        VkCtx& vk = r.vk();
        g.shutdown();
        verifier(!c.ouvert(), "fenêtre perdue en jeu (Game::shutdown) : lecture fermée");
        r.shutdown(); r.init(vk);
        const bool ok = g.init(r, "golf", "ville");
        verifier(ok && !c.ouvert() && !c.premierLancement(), "fenêtre retrouvée : jeu rechargé, lecture fermée, pas d'écran des CGU");
        if (!ok) { sauvegarde().desactiver(); std::remove(chemin.c_str()); return ecarts; }
        g.setAutopilot(true); g.camMode = 0;
        for (int i = 0; i < 30; i++) g.update(1.0 / 60);
        g.allerA(Ecran::Menu);
        for (int i = 0; i < 60; i++) imageJeu();
        captureJeu("cgu-retour-jeu", "après la perte de la fenêtre pendant la lecture : menu revenu, sans écran des CGU");
    }
    ouvrirReglagesBanc(false); imageJeu();
    g.allerA(Ecran::Course);
    for (int i = 0; i < 120; i++) imageJeu();
    verifier(g.ecran() == Ecran::Course, "course après les CGU");
    g.allerA(Ecran::Menu);
    for (int i = 0; i < 30; i++) imageJeu();
    verifier(g.ecran() == Ecran::Menu, "retour au menu");
    g.shutdown();
    sauvegarde().desactiver();
    std::remove(chemin.c_str());
    WFS_LOG("CGU (WFS-308) banc : parcours complet : %d écart(s)%s", ecarts, ecarts ? " ÉCHEC" : " OK");
    return ecarts;
}

} // namespace ecrans
} // namespace wfs
