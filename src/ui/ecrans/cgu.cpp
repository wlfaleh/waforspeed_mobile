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
#include "../text.h"
#include "reglages.h"

namespace wfs {
namespace ecrans {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SURFACE2 = 0x1E2130,
               DANGER = 0xFF1F4B;
const vec4 SURFACE{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.98f}, GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.16f};
const char* MESSAGE_REFUS = "Le jeu ne peut pas démarrer sans votre accord";

// texte d'un fichier : assets de l'APK (cgu/…, copiés par android/app/build.gradle), sinon store/ (PC, depuis la racine du dépôt)
std::string lireTexte(const char* nom) {
    std::vector<uint8_t> a;
    if (readAsset(std::string("cgu/") + nom, a)) return std::string(a.begin(), a.end());
    std::ifstream f(std::string("store/") + nom, std::ios::binary);
    if (f) { std::ostringstream s; s << f.rdbuf(); return s.str(); }
    WFS_ERR("CGU (WFS-308) : %s introuvable (assets cgu/ ni store/)", nom);
    return "";
}

// la politique de confidentialité s'arrête avant les notes réservées aux stores (questionnaires, classification d'âge)
std::string sansNotesStore(const std::string& md) {
    const size_t p = md.find("\n## Réponses aux questionnaires");
    return p == std::string::npos ? md : md.substr(0, p);
}

// trait épais de (ax, ay) à (bx, by)
void trait(Renderer& r, float ax, float ay, float bx, float by, float e, const vec4& c) {
    const float dx = bx - ax, dy = by - ay, l = std::max(std::sqrt(dx * dx + dy * dy), 1e-3f), nx = -dy / l * e / 2, ny = dx / l * e / 2;
    const UiVertex q[4] = {{ax + nx, ay + ny, -1, -1, c.x, c.y, c.z, c.w}, {bx + nx, by + ny, -1, -1, c.x, c.y, c.z, c.w},
                           {bx - nx, by - ny, -1, -1, c.x, c.y, c.z, c.w}, {ax - nx, ay - ny, -1, -1, c.x, c.y, c.z, c.w}};
    r.uiQuad(q);
}

void bouton(Renderer& r, const Cgu::Zone& z, float biais, const vec4& fond, const std::string& t, float tp, const vec4& ct, bool vitre) {
    if (vitre) dessin2d::biseau(r, z.x - 1, z.y - 1, z.w + 2, z.h + 2, biais, dessin2d::uni(BORD));
    dessin2d::biseau(r, z.x, z.y, z.w, z.h, biais, dessin2d::uni(fond));
    ui::text(r, z.x + z.w / 2, z.y + (z.h - tp) / 2, tp, t, ct, ui::CENTRE, 0.12f, false);
}

} // namespace

void Cgu::ouvrir(Renderer& r, Mode m) {
    r_ = &r; mode_ = m; etat_ = Etat::Ouvert; coche_ = false; defil_ = 0; tRefus_ = -1; doigt_ = -1; cible_ = 0;
    paras_ = paragraphesMarkdown(lireTexte("cgu.md"));
    const std::vector<ParagrapheCgu> conf = paragraphesMarkdown(sansNotesStore(lireTexte("confidentialite.md")));
    if (!paras_.empty() && !conf.empty()) paras_.push_back({ParagrapheCgu::VIDE, ""});
    paras_.insert(paras_.end(), conf.begin(), conf.end());
    largeurPage_ = -1;
    WFS_LOG("CGU (WFS-308) : écran ouvert (%s), version %d, %zu paragraphes", m == Mode::Premier ? "premier lancement" : "Réglages, lecture seule",
            VERSION_CGU, paras_.size());
}

// découpe des paragraphes en lignes de la largeur du cadre (mots entiers ; suite d'une puce alignée sur son texte)
void Cgu::mettreEnPage(float largeur, float k) {
    largeurPage_ = largeur; kPage_ = k; lignes_.clear();
    // corps de 16 (dp sur téléphone couché : au-dessus du minimum de 14 dp, petit_ecran.h), titres 22 et 17
    const float corps = 16 * k, titre = 22 * k, sous = 17 * k;
    float y = 0;
    for (const ParagrapheCgu& p : paras_) {
        if (p.style == ParagrapheCgu::VIDE) { y += corps * 0.6f; continue; }
        const float t = p.style == ParagrapheCgu::TITRE ? titre : p.style == ParagrapheCgu::SOUS_TITRE ? sous : corps;
        const float interligne = t * 1.38f;
        const std::string puce = "• ";
        const float retrait = p.style == ParagrapheCgu::PUCE ? ui::width(t, puce) : 0;
        if (p.style == ParagrapheCgu::SOUS_TITRE && y > 0) y += corps * 0.4f;
        std::string ligne = p.style == ParagrapheCgu::PUCE ? puce : "";
        bool premiere = true;
        size_t i = 0;
        auto pousser = [&]() {
            lignes_.push_back({p.style, ligne, premiere ? 0.f : retrait, y, t});
            y += interligne; premiere = false; ligne.clear();
        };
        const std::string& s = p.texte;
        while (i < s.size()) {
            size_t f = s.find(' ', i);
            if (f == std::string::npos) f = s.size();
            const std::string mot = s.substr(i, f - i);
            i = f + 1;
            if (mot.empty()) continue;
            const std::string essai = ligne.empty() || ligne == puce ? ligne + mot : ligne + " " + mot;
            const float dispo = largeur - (premiere ? 0 : retrait);
            if (!ligne.empty() && ligne != puce && ui::width(t, essai) > dispo) { pousser(); ligne = mot; }
            else ligne = essai;
        }
        if (!ligne.empty()) pousser();
    }
    hauteurTexte_ = y;
}

void Cgu::dessiner(Renderer& r, float W, float H, const Marges& m, double maintenant) {
    maintenant_ = maintenant;
    if (etat_ == Etat::Refuse && tRefus_ >= 0 && maintenant - tRefus_ >= CGU_DELAI_FERMETURE) {
        etat_ = Etat::Quitter;
        WFS_LOG("CGU (WFS-308) : refus, fermeture de l'application");
    }
    if (!ouvert() || !ui::fontReady()) return;
    const bool cp = ui::petitEcran(W, H);
    const float k = cp ? ui::unitePetitEcran(W, H) : H / 720.f, kb = H / 720.f;
    r.uiRect(0, 0, W, H, hex(BG));
    const float zl = m.gauche + 16 * k, zr = W - m.droite - 16 * k;
    const float pw = std::min(zr - zl, 1040 * k), px = zl + (zr - zl - pw) / 2;
    const float py = m.haut + (cp ? 8 : 24) * k, ph = H - py - m.bas - (cp ? 8 : 24) * k;
    r.uiRect(px, py, pw, ph, SURFACE);
    r.uiRect(px, py, pw, 3 * k, hex(ACCENT2));   // liseré néon
    const float x0 = px + 24 * k;
    const std::string sur = (mode_ == Mode::Premier ? std::string("Premier lancement") : std::string("Réglages")) + " · version " + std::to_string(VERSION_CGU);
    ui::text(r, x0, py + 14 * k, 12 * k, ui::majuscules(sur), hex(ACCENT2), ui::GAUCHE, 0.22f, false);
    const float tt = (cp ? 26 : 34) * k;
    ui::text(r, x0, py + 32 * k, tt, ui::majuscules("Conditions d'utilisation"), hex(TEXTE), ui::GAUCHE, 0.02f, false, true);
    const float haut = py + 32 * k + tt + 12 * k;

    // barre du bas : case et boutons (premier lancement) ou « Fermer » (lecture)
    const float bh = 50 * kb, tp = (cp ? 18 : 20) * k, by = py + ph - 14 * k - bh;
    auto largeurBouton = [&](const std::string& t) { return ui::width(tp, t, 0.12f) + 56 * kb; };
    float xDroite = px + pw - 24 * k;
    if (mode_ == Mode::Premier) {
        const std::string ta = ui::majuscules("Accepter"), tr = ui::majuscules("Refuser");
        const float wa = largeurBouton(ta), wr = largeurBouton(tr);
        zAccepter_ = {xDroite - wa, by, wa, bh};
        zRefuser_ = {zAccepter_.x - 16 * k - wr, by, wr, bh};
        zFermer_ = {};
        bouton(r, zRefuser_, 14 * kb, GLASS, tr, tp, hex(TEXTE), true);
        // RG-03 : « Accepter » actif seulement après la case (inactif : gris, texte atténué)
        bouton(r, zAccepter_, 14 * kb, coche_ ? hex(ACCENT) : hex(SURFACE2), ta, tp, coche_ ? hex(BG) : hex(MUTED, 0.8f), false);
        const float cs = 28 * k, cy = by + (bh - cs) / 2, tl = (cp ? 16 : 18) * k;
        const std::string lib = "J'ai lu et j'accepte";
        r.uiRect(x0, cy, cs, cs, coche_ ? hex(ACCENT2) : hex(TEXTE, 0.85f));
        r.uiRect(x0 + 2 * k, cy + 2 * k, cs - 4 * k, cs - 4 * k, coche_ ? hex(ACCENT2) : hex(SURFACE2));
        if (coche_) {   // coche en deux traits (la police n'a pas le caractère ✓)
            trait(r, x0 + cs * 0.22f, cy + cs * 0.52f, x0 + cs * 0.42f, cy + cs * 0.72f, 3.5f * k, hex(BG));
            trait(r, x0 + cs * 0.40f, cy + cs * 0.72f, x0 + cs * 0.80f, cy + cs * 0.28f, 3.5f * k, hex(BG));
        }
        ui::text(r, x0 + cs + 12 * k, by + (bh - tl) / 2, tl, lib, coche_ ? hex(ACCENT2) : hex(TEXTE), ui::GAUCHE, 0.02f, false);
        // zone de toucher : case et libellé, 44 dp de haut au moins (petit_ecran.h, TOUCHER_MIN_DP)
        const float zh = std::max(bh, ui::TOUCHER_MIN_DP * k);
        zCase_ = {x0 - 8 * k, by + (bh - zh) / 2, cs + 20 * k + ui::width(tl, lib, 0.02f) + 8 * k, zh};
    } else {
        const std::string tf = ui::majuscules("Fermer");
        const float wf = largeurBouton(tf);
        zFermer_ = {xDroite - wf, by, wf, bh};
        zAccepter_ = zRefuser_ = zCase_ = {};
        bouton(r, zFermer_, 14 * kb, hex(ACCENT), tf, tp, hex(BG), false);
    }

    // cadre du texte défilant
    const float tx = px + 20 * k, ty = haut, tw = pw - 40 * k, th = by - 14 * k - ty, pad = 14 * k;
    zTexte_ = {tx, ty, tw, th};
    r.uiRect(tx, ty, tw, th, hex(BG, 0.65f));
    const float largeur = tw - 2 * pad - 12 * k;
    if (std::fabs(largeur - largeurPage_) > 0.5f || std::fabs(k - kPage_) > 1e-4f) mettreEnPage(largeur, k);
    const float vue = th - 2 * pad;
    defilMax_ = std::max(0.f, hauteurTexte_ - vue);
    defil_ = std::clamp(defil_, 0.f, defilMax_);
    for (const Ligne& l : lignes_) {   // lignes entièrement visibles seulement (pas de découpe du texte par l'interface)
        const float y = l.y - defil_;
        if (y < 0 || y + l.taille * 1.2f > vue) continue;
        const vec4 c = l.style == ParagrapheCgu::TITRE ? hex(ACCENT2) : l.style == ParagrapheCgu::SOUS_TITRE ? hex(ACCENT) : hex(TEXTE, 0.92f);
        const std::string t = l.style == ParagrapheCgu::SOUS_TITRE ? ui::majuscules(l.texte) : l.texte;
        ui::text(r, tx + pad + l.x, ty + pad + y, l.taille, t, c, ui::GAUCHE, l.style == ParagrapheCgu::SOUS_TITRE ? 0.08f : 0.f, false);
    }
    if (defilMax_ > 0) {   // ascenseur : position et part visible du texte
        const float bx = tx + tw - 8 * k, hb = std::max(24 * k, vue * vue / hauteurTexte_), yb = ty + pad + (vue - hb) * (defil_ / defilMax_);
        r.uiRect(bx, ty + pad, 4 * k, vue, hex(TEXTE, 0.08f));
        r.uiRect(bx, yb, 4 * k, hb, hex(ACCENT2, 0.85f));
    }

    if (etat_ == Etat::Refuse) {   // RG-04 : message, puis fermeture après CGU_DELAI_FERMETURE
        r.uiRect(0, 0, W, H, hex(BG, 0.94f));
        const float tm = (cp ? 22 : 30) * k;
        ui::text(r, W / 2, H / 2 - tm, tm, MESSAGE_REFUS, hex(DANGER), ui::CENTRE, 0.02f, false);
        ui::text(r, W / 2, H / 2 + 16 * k, (cp ? 15 : 17) * k, "Fermeture du jeu…", hex(MUTED), ui::CENTRE, 0.02f, false);
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

bool Cgu::toucher(int id, float x, float y, bool down) {
    if (!ouvert()) return false;
    if (etat_ == Etat::Refuse) return true;   // message du refus : plus rien n'est touchable
    if (down) {
        if (doigt_ < 0) {
            doigt_ = id; yDoigt_ = y;
            cible_ = zCase_.contient(x, y) ? 2 : zAccepter_.contient(x, y) ? 3 : zRefuser_.contient(x, y) ? 4 : zFermer_.contient(x, y) ? 5
                   : zTexte_.contient(x, y) ? 1 : 0;
        } else if (id == doigt_ && cible_ == 1) {   // glisser : le texte suit le doigt
            defil_ = std::clamp(defil_ - (y - yDoigt_), 0.f, defilMax_);
            yDoigt_ = y;
        }
        return true;
    }
    if (id != doigt_) return true;
    const int c = cible_;
    doigt_ = -1; cible_ = 0;
    if (c == 2 && zCase_.contient(x, y)) { coche_ = !coche_; WFS_LOG("CGU (WFS-308) : case « J'ai lu et j'accepte » %s", coche_ ? "cochée" : "décochée"); }
    else if (c == 3 && zAccepter_.contient(x, y)) {
        if (!coche_) WFS_LOG("CGU (WFS-308) : « Accepter » inactif (case décochée)");
        else { etat_ = Etat::Accepte; WFS_LOG("CGU (WFS-308) : « Accepter » (version %d)", VERSION_CGU); }
    } else if (c == 4 && zRefuser_.contient(x, y)) { etat_ = Etat::Refuse; tRefus_ = maintenant_; WFS_LOG("CGU (WFS-308) : « Refuser » — %s", MESSAGE_REFUS); }
    else if (c == 5 && zFermer_.contient(x, y)) { etat_ = Etat::Ferme; WFS_LOG("CGU (WFS-308) : lecture fermée"); }
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
    return etat_;
}

Cgu& cgu() { static Cgu c; return c; }

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
    capture("cgu-premier", "premier lancement, case décochée, « Accepter » inactif");
    appui(1, c.zoneAccepter());
    verifier(c.etat() == Cgu::Etat::Ouvert, "« Accepter » sans la case : rien ne se passe");
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
    appui(3, c.zoneCase());
    verifier(c.caseCochee(), "case cochée");
    capture("cgu-coche", "case cochée, « Accepter » actif (orange)");
    appui(4, c.zoneRefuser());
    verifier(c.etat() == Cgu::Etat::Refuse, "« Refuser » → message");
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
        capture("cgu-reprise", "après la perte de la fenêtre : texte lisible, case décochée");
    }
    verifier(!c.caseCochee(), "case décochée à la réouverture");
    appui(5, c.zoneCase()); appui(6, c.zoneAccepter());
    verifier(c.etat() == Cgu::Etat::Accepte, "case puis « Accepter » → accepté");
    {   // fenêtre perdue dans le même lot qu'« Accepter » (avant Cgu::image) : chemin de termGraphics d'android_main.cpp
        verifier(c.tientLaFenetre() && !c.premierLancement(), "« Accepter » pas encore traité : la fenêtre reste tenue par les CGU");
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
