// WaForSpeed — « Mise à jour et installation des paquets » (WFS-306) : voir paquets.h.
// Style design-street-racing des écrans : fond de l'écran de chargement (WFS-35), panneau --surface à liseré cyan, sur-titre cyan, titre
// en capitales, bouton principal orange en biseau, bouton secondaire vitré, barres dégradées cyan → orange comme celle du chargement.
#include "paquets.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include "chargement.h"
#include "terrain.h"   // WFS-307 : banc (choix de la carte, « Rouler »)
#include "../../core/log.h"
#include "../../core/platform.h"
#include "../../game/commandes_tactiles.h"   // dessin2d::biseau
#include "../../game/game.h"
#include "../../render/renderer.h"
#include "../../render/vk.h"
#include "../../world/cartes_v2/paquet_carte.h"   // empreinte attendue, cuisson (banc)
#include "../petit_ecran.h"
#include "../text.h"

namespace wfs {
namespace ecrans {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SURFACE2 = 0x1E2130, DANGER = 0xFF1F4B;
const vec4 SURFACE{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.97f}, GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.16f};
constexpr double ASTUCE_S = 7.0;            // RG-04 : 6 à 8 s
constexpr double TEMPS_RESTANT_APRES_S = 5.0;


void bouton(Renderer& r, const EtapePaquets::Zone& z, float k, bool principal, const std::string& t, float tp) {
    if (!principal) dessin2d::biseau(r, z.x - 1, z.y - 1, z.w + 2, z.h + 2, 14 * k, dessin2d::uni(BORD));
    dessin2d::biseau(r, z.x, z.y, z.w, z.h, 14 * k, dessin2d::uni(principal ? hex(ACCENT) : GLASS));
    ui::text(r, z.x + z.w / 2, z.y + (z.h - tp) / 2, tp, ui::majuscules(t), principal ? hex(BG) : hex(TEXTE), ui::CENTRE, 0.12f, false);
}
// barre dégradée cyan → orange (même dessin que la barre de l'écran de chargement, chargement.cpp)
void barre(Renderer& r, float x, float y, float w, float h, float p) {
    r.uiRect(x, y, w, h, hex(SURFACE2));
    const float plein = w * std::clamp(p, 0.f, 1.f);
    const int N = 16;
    for (int i = 0; i < N && plein > 0; i++) {
        const float t = (i + 0.5f) / N;
        const vec4 a = hex(ACCENT2), b = hex(ACCENT);
        const float x0 = std::floor(x + plein * i / N), x1 = i == N - 1 ? x + plein : std::floor(x + plein * (i + 1) / N);
        if (x1 > x0) r.uiRect(x0, y, x1 - x0, h, {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, 1});
    }
}
std::string duree(double s) {
    const int n = (int)std::lround(std::max(0.0, s));
    if (n < 60) return std::to_string(n) + " s";
    return std::to_string(n / 60) + " min " + (n % 60 < 10 ? "0" : "") + std::to_string(n % 60) + " s";
}
std::string cheminPaquet(const std::string& c, carte::FormatTexture f) { return carte::dossierPaquets() + "/" + carte::nomFichier(c, f); }

} // namespace

// ---------------------------------------------------------------- outils
std::vector<carte::FormatTexture> formatsAcceptes(const VkCtx& vk) {
    std::vector<carte::FormatTexture> f;
    if (vk.formatEchantillonnable(VK_FORMAT_ASTC_4x4_SRGB_BLOCK) && vk.formatEchantillonnable(VK_FORMAT_ASTC_4x4_UNORM_BLOCK) &&
        vk.formatEchantillonnable(VK_FORMAT_ASTC_6x6_SRGB_BLOCK) && vk.formatEchantillonnable(VK_FORMAT_ASTC_6x6_UNORM_BLOCK)) f.push_back(carte::FormatTexture::ASTC);
    if (vk.formatEchantillonnable(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK) && vk.formatEchantillonnable(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK)) f.push_back(carte::FormatTexture::ETC2);
    f.push_back(carte::FormatTexture::RGBA8);   // toujours lu
    return f;
}
int versionApk() { return versionApk(versionJeu()); }   // « 0.1.N » sur l'APK de la CI, « dev » ailleurs

const std::vector<std::string>& astuces() {
    static std::vector<std::string> a = [] {
        std::vector<std::string> v;
        std::vector<uint8_t> o;
        if (readAsset("textes/astuces-paquets.txt", o)) {
            std::string l;
            for (size_t i = 0; i <= o.size(); i++) {
                if (i == o.size() || o[i] == '\n') {
                    while (!l.empty() && (l.back() == '\r' || l.back() == ' ')) l.pop_back();
                    if (!l.empty() && l[0] != '#') v.push_back(l);
                    l.clear();
                } else l += (char)o[i];
            }
        }
        if (v.empty()) v.push_back("Une fois installée, la carte reste sur ton téléphone.");
        return v;
    }();
    return a;
}

// ---------------------------------------------------------------- étape
bool EtapePaquets::demarrer(Renderer& r, SourcePaquets& s, const std::string& c, const std::vector<carte::FormatTexture>& formats) {
    r_ = &r;
    cartesPaquets().lier(&s, formats, versionApk());   // WFS-307 : même source pour le choix du terrain
    cartesPaquets().suivre(c, cartes_v2::empreinteAttendue(c));   // RG-08 : paquet installé plus tard au menu → relu à la course suivante
    return demarrerAvec(s, c, formats, cartes_v2::empreinteAttendue(c), versionApk(), nowSeconds());
}

void EtapePaquets::dessiner(Renderer& r, float W, float H, const Marges& m, double t) {
    if (!ui::fontReady()) return;
    const bool cp = ui::petitEcran(W, H);
    const float k = cp ? ui::unitePetitEcran(W, H) : H / 720.f, kb = H / 720.f;
    if (!menu_) dessinerChargement(r, W, H, 1.f, "Paquets de cartes", false, 1.f);   // fond de l'écran de chargement (logo, barre pleine)
    r.uiRect(0, 0, W, H, hex(BG, menu_ ? 0.72f : 0.82f));   // au menu (RG-06) : voile sur le menu
    const float pw = std::min(W - m.gauche - m.droite - 32 * k, 760 * k), px = m.gauche + (W - m.gauche - m.droite - pw) / 2;
    const float ph = (cp ? 300 : 340) * k, py = std::max(m.haut + 8 * k, (H - ph) / 2);
    r.uiRect(px, py, pw, ph, SURFACE);
    r.uiRect(px, py, pw, 3 * k, hex(ACCENT2));
    const float x0 = px + 28 * k, lw = pw - 56 * k;
    ui::text(r, x0, py + 18 * k, 12 * k, ui::majuscules("Paquets de cartes"), hex(ACCENT2), ui::GAUCHE, 0.22f, false);
    const float tt = (cp ? 24 : 30) * k;
    ui::text(r, x0, py + 36 * k, tt, ui::majuscules("Mise à jour et installation"), hex(TEXTE), ui::GAUCHE, 0.02f, false, true);
    float y = py + 36 * k + tt + 22 * k;
    const float tc = (cp ? 16 : 18) * k, ts = (cp ? 14 : 15) * k, bh = 50 * kb, tb = (cp ? 18 : 20) * k;
    const float by = py + ph - 22 * k - bh;
    auto largeur = [&](const std::string& s) { return ui::width(tb, ui::majuscules(s), 0.12f) + 56 * kb; };
    zInstaller_ = zPlusTard_ = zContinuer_ = {};
    switch (etat_) {
    case Etat::Recherche: {
        ui::text(r, x0, y, tc, "Recherche des paquets de cartes…", hex(TEXTE), ui::GAUCHE, 0.f, false);
        const float p = (float)std::fmod((t - t0_) * 0.6, 1.0);   // barre d'attente qui défile
        r.uiRect(x0, y + tc + 14 * k, lw, 6 * k, hex(SURFACE2));
        r.uiRect(x0 + lw * std::max(0.f, p - 0.25f), y + tc + 14 * k, lw * (std::min(p, 0.75f) - std::max(0.f, p - 0.25f) + 0.25f) * 0.999f, 6 * k, hex(ACCENT2, 0.85f));
        break;
    }
    case Etat::Proposition: {
        ui::text(r, x0, y, tc + 4 * k, nom_ + " — " + texteMo(taille_), hex(TEXTE), ui::GAUCHE, 0.02f, false);
        y += tc + 14 * k;
        ui::text(r, x0, y, ts, "Carte téléchargée une seule fois : elle reste ensuite sur le téléphone.", hex(MUTED), ui::GAUCHE, 0.f, false);
        y += ts + 12 * k;
        ui::text(r, x0, y, ts, "Wi-Fi conseillé.", hex(ACCENT2), ui::GAUCHE, 0.f, false);
        if (reseau_ == 2) ui::text(r, x0, y + ts + 8 * k, ts, "Vous êtes sur les données mobiles (" + texteMo(taille_) + ").", hex(ACCENT), ui::GAUCHE, 0.f, false);
        else if (reseau_ == 0) ui::text(r, x0, y + ts + 8 * k, ts, "Pas de connexion pour le moment.", hex(MUTED), ui::GAUCHE, 0.f, false);
        const float wi = largeur("Installer"), wp = largeur("Plus tard");
        zInstaller_ = {px + pw - 28 * k - wi, by, wi, bh};
        zPlusTard_ = {zInstaller_.x - 16 * k - wp, by, wp, bh};
        bouton(r, zPlusTard_, kb, false, "Plus tard", tb);
        bouton(r, zInstaller_, kb, reseau_ == 1, "Installer", tb);   // RG-02 : mis en avant sur Wi-Fi seulement
        break;
    }
    case Etat::Telechargement: {
        const float p = taille_ > 0 ? (float)recus_ / (float)taille_ : 0.f;
        ui::text(r, x0, y, tc, nom_ + " — " + texteMo(recus_) + " / " + texteMo(taille_), hex(TEXTE), ui::GAUCHE, 0.f, false);
        barre(r, x0, y + tc + 8 * k, lw, 6 * k, p);
        y += tc + 26 * k;
        ui::text(r, x0, y, ts, "Total — " + texteMo(recus_) + " / " + texteMo(taille_), hex(MUTED), ui::GAUCHE, 0.f, false);   // une seule carte pour l'étape 1
        barre(r, x0, y + ts + 8 * k, lw, 4 * k, p);
        y += ts + 26 * k;
        std::string etat;
        if (tFini_ >= 0) etat = nom_ + " installée";
        else if (coupure_) etat = "Connexion perdue, reprise automatique";
        else if (tDebutTelechargement_ >= 0 && t - tDebutTelechargement_ >= TEMPS_RESTANT_APRES_S && recus_ > recusDebut_) {
            const double v = (recus_ - recusDebut_) / std::max(0.1, t - tDebutTelechargement_);
            etat = "Temps restant : " + duree((taille_ - recus_) / std::max(1.0, v));
        }
        if (!etat.empty()) ui::text(r, x0, y, ts, etat, coupure_ ? hex(ACCENT) : hex(ACCENT2), ui::GAUCHE, 0.f, false);
        const auto& a = astuces();
        const size_t i = (size_t)std::max(0.0, (t - t0_) / ASTUCE_S) % a.size();
        ui::text(r, x0, by - ts - 18 * k, ts, "Astuce : " + a[i], hex(TEXTE, 0.85f), ui::GAUCHE, 0.f, false);
        if (tFini_ < 0) { const float wp = largeur("Plus tard"); zPlusTard_ = {px + pw - 28 * k - wp, by, wp, bh}; bouton(r, zPlusTard_, kb, false, "Plus tard", tb); }
        break;
    }
    case Etat::Place: case Etat::Echec: {
        ui::text(r, x0, y, tc, message_, hex(DANGER), ui::GAUCHE, 0.f, false);
        ui::text(r, x0, y + tc + 12 * k, ts, "Le jeu démarre avec la carte livrée dans l'application.", hex(MUTED), ui::GAUCHE, 0.f, false);
        if (etat_ == Etat::Echec) {
            const float wi = largeur("Réessayer"), wp = largeur("Plus tard");
            zInstaller_ = {px + pw - 28 * k - wi, by, wi, bh};
            zPlusTard_ = {zInstaller_.x - 16 * k - wp, by, wp, bh};
            bouton(r, zPlusTard_, kb, false, "Plus tard", tb);
            bouton(r, zInstaller_, kb, true, "Réessayer", tb);
        } else {
            const float wc = largeur("Continuer");
            zContinuer_ = {px + pw - 28 * k - wc, by, wc, bh};
            bouton(r, zContinuer_, kb, true, "Continuer", tb);
        }
        break;
    }
    default: break;
    }
}

bool EtapePaquets::toucher(int id, float x, float y, bool down) {
    if (!ouvert()) return false;
    if (down) {
        if (doigt_ < 0) { doigt_ = id; cible_ = zInstaller_.contient(x, y) ? 1 : zPlusTard_.contient(x, y) ? 2 : zContinuer_.contient(x, y) ? 3 : 0; }
        return true;
    }
    if (id != doigt_) return true;
    const int c = cible_;
    doigt_ = -1; cible_ = 0;
    if (c == 1 && zInstaller_.contient(x, y)) installer();
    else if (c == 2 && zPlusTard_.contient(x, y)) plusTard();
    else if (c == 3 && zContinuer_.contient(x, y)) continuer();
    return true;
}

bool EtapePaquets::image(VkCtx& vk, uint32_t lf, uint32_t hf, double t) {
    if (etat_ == Etat::Fini) { etat_ = Etat::Ferme; return true; }
    if (!tientLaFenetre() || !r_) return false;
    avancer(t);
    if (etat_ == Etat::Fini) { etat_ = Etat::Ferme; return true; }
    const bool tourne = (lf >= hf) != (r_->width() >= r_->height());   // même contrôle que les CGU (WFS-308)
    if (vk.surfaceChanged() || (tourne && essaisRotation_++ < 8)) { vk.waitIdle(); if (vk.createSwapchain()) r_->resize(); }
    r_->setViewSize(lf, hf);
    FrameParams fp;
    fp.view = mat4::identity(); fp.proj = mat4::identity(); fp.camPos = {0, 0, 0};
    if (!r_->beginFrame(fp)) return false;
    dessiner(*r_, (float)r_->width(), (float)r_->height(), Marges{}, t);
    r_->endFrame();
    return false;
}

void EtapePaquets::perdreFenetre(Renderer& r) {
    ui::libererPolice(r);   // comme les CGU : l'atlas suit le renderer
    fenetre_ = false; r_ = nullptr; doigt_ = -1;
    WFS_LOG("paquets (WFS-306) : fenêtre perdue, l'étape attend (le téléchargement continue)");
}
bool EtapePaquets::reprendre(Renderer& r) {
    if (!ouvert()) return false;
    r_ = &r; fenetre_ = true; essaisRotation_ = 0;
    ui::chargerPolice(r);
    WFS_LOG("paquets (WFS-306) : fenêtre retrouvée, l'étape reprend");
    return true;
}

EtapePaquets& paquets() { static EtapePaquets e; return e; }

// ---------------------------------------------------------------- WFS-307 : paquet de la carte sur la vignette du choix du terrain
// RG-01 : carte à télécharger assombrie, « Télécharger — X Mo » ou « Mise à jour — X Mo » ; RG-02 : file, barre, astuce, coupure,
// « Annuler », données mobiles confirmées ; RG-04 : « Supprimer — libère X Mo » avec confirmation ; RG-05 : « Connexion nécessaire ».
namespace {
struct UiTerrain {
    enum Dialogue { Aucun, Supprimer, Mobile } dialogue = Aucun;
    std::string carteDialogue, message;
    double tMessage = -1e9;
    EtapePaquets::Zone z[4];   // 0 principal, 1 Supprimer / Annuler, 2 confirmer, 3 refuser
    int doigt = -1, zoneDoigt = -1;
};
UiTerrain& uiTerrain() { static UiTerrain u; return u; }
constexpr double MESSAGE_S = 3.0;

void bloc(Renderer& r, float x, float y, float w, float h, float k) {
    dessin2d::biseau(r, x - 1, y - 1, w + 2, h + 2, 10 * k, dessin2d::uni(BORD));
    dessin2d::biseau(r, x, y, w, h, 10 * k, dessin2d::uni(SURFACE));
    r.uiRect(x + 10 * k, y, w - 20 * k, 2 * k, hex(ACCENT2));
}
} // namespace

void assombrirCarte(Renderer& r, const std::string& c, float x, float y, float w, float h) {
    const EtatCarte e = cartesPaquets().etat(c);
    if (e.etat == EtatCarte::Disponible || e.etat == EtatCarte::MiseAJour || e.etat == EtatCarte::Attente ||
        e.etat == EtatCarte::Telechargement || e.etat == EtatCarte::Coupure || e.etat == EtatCarte::Echec)
        r.uiRect(x, y, w, h, hex(BG, 0.48f));
}

void dessinerPaquetCarte(Renderer& r, const std::string& c, float x, float y, float w, float h, float k, double t) {
    UiTerrain& u = uiTerrain();
    for (auto& z : u.z) z = {};
    CartesPaquets& cp = cartesPaquets();
    const EtatCarte e = cp.etat(c);
    if (u.dialogue != UiTerrain::Aucun && u.carteDialogue != c) u.dialogue = UiTerrain::Aucun;   // carte changée : confirmation refermée
    const float tp = 15 * k, ts = 14 * k, bh = std::max(44.f, ui::TOUCHER_MIN_DP) * k, m = 16 * k;
    auto largeur = [&](const std::string& s) { return ui::width(tp, ui::majuscules(s), 0.12f) + 40 * k; };
    const float pwMax = std::min(w - 2 * m, 460 * k);
    // ---- confirmations (RG-04 suppression, RG-02 données mobiles) : panneau au milieu de la vignette
    if (u.dialogue != UiTerrain::Aucun) {
        const bool sup = u.dialogue == UiTerrain::Supprimer;
        const std::string l1 = sup ? "Supprimer le paquet de la " + e.nom + " ?" : "Vous êtes sur les données mobiles (" + texteMo(e.taille) + ")",
                          l2 = sup ? "Libère " + texteMo(e.surDisque) + ". La carte reste jouable, construite par le jeu." : "Wi-Fi conseillé. Télécharger quand même ?",
                          oui = sup ? "Supprimer" : "Télécharger", non = sup ? "Annuler" : "Plus tard";
        const float pw = std::min(w - 2 * m, std::max({ui::width(tp + 2 * k, l1), ui::width(ts, l2), largeur(oui) + largeur(non) + 12 * k}) + 48 * k);
        const float ph = 24 * k + tp + 2 * k + 10 * k + ts + 18 * k + bh + 20 * k, px = x + (w - pw) / 2, py = y + (h - ph) / 2;
        r.uiRect(x, y, w, h, hex(BG, 0.55f));
        bloc(r, px, py, pw, ph, k);
        ui::text(r, px + 24 * k, py + 24 * k, tp + 2 * k, l1, hex(TEXTE), ui::GAUCHE, 0.f, false);
        ui::text(r, px + 24 * k, py + 24 * k + tp + 12 * k, ts, l2, hex(MUTED), ui::GAUCHE, 0.f, false);
        const float by = py + ph - 20 * k - bh, wo = largeur(oui), wn = largeur(non);
        u.z[2] = {px + pw - 24 * k - wo, by, wo, bh};
        u.z[3] = {u.z[2].x - 12 * k - wn, by, wn, bh};
        bouton(r, u.z[3], k, false, non, tp);
        if (sup) {   // bouton de suppression rouge (action destructrice)
            dessin2d::biseau(r, u.z[2].x, u.z[2].y, u.z[2].w, u.z[2].h, 14 * k, dessin2d::uni(hex(DANGER)));
            ui::text(r, u.z[2].x + wo / 2, by + (bh - tp) / 2, tp, ui::majuscules(oui), hex(TEXTE), ui::CENTRE, 0.12f, false);
        } else bouton(r, u.z[2], k, false, oui, tp);   // RG-02 de WFS-306 : sur données mobiles, pas mis en avant
        return;
    }
    const float px0 = x + w - m;   // panneaux calés en haut à droite (le badge « Choisi » est en haut à gauche)
    switch (e.etat) {
    case EtatCarte::Disponible: case EtatCarte::MiseAJour: case EtatCarte::Echec: {
        const std::string l = (e.etat == EtatCarte::MiseAJour ? "Mise à jour — " : e.etat == EtatCarte::Echec ? "Réessayer — " : "Télécharger — ") + texteMo(e.taille);
        const float bw = largeur(l);
        u.z[0] = {px0 - bw, y + m, bw, bh};
        bouton(r, u.z[0], k, cp.reseau() != 2, l, tp);
        if (e.etat == EtatCarte::Echec) ui::text(r, px0, y + m + bh + 8 * k, ts, "Paquet abîmé ou erreur : la carte reste jouable", hex(DANGER), ui::DROITE, 0.f, true);
        break;
    }
    case EtatCarte::Attente: case EtatCarte::Telechargement: case EtatCarte::Coupure: {
        const float pw = pwMax, px = px0 - pw, py = y + m, ph = 20 * k + tp + 12 * k + 6 * k + 14 * k + ts + 12 * k + bh + 16 * k;
        bloc(r, px, py, pw, ph, k);
        const float x0 = px + 20 * k, lw = pw - 40 * k;
        std::string l1;
        if (e.etat == EtatCarte::Attente) l1 = e.nom + " — en file d'attente (" + texteMo(e.taille) + ")";
        else l1 = e.nom + " — " + texteMo(e.recus) + " / " + texteMo(e.taille);
        ui::text(r, x0, py + 20 * k, tp, l1, hex(TEXTE), ui::GAUCHE, 0.f, false);
        barre(r, x0, py + 20 * k + tp + 12 * k, lw, 6 * k, e.taille > 0 ? (float)e.recus / (float)e.taille : 0.f);
        const float y2 = py + 20 * k + tp + 12 * k + 6 * k + 14 * k;
        if (e.etat == EtatCarte::Coupure) ui::text(r, x0, y2, ts, "Connexion perdue, reprise automatique", hex(ACCENT), ui::GAUCHE, 0.f, false);
        else {
            const auto& a = astuces();
            const size_t i = (size_t)std::max(0.0, t / ASTUCE_S) % a.size();
            std::string s = a[i];
            while (s.size() > 4 && ui::width(ts, s) > lw) { s.resize(s.size() - 4); while (!s.empty() && ((unsigned char)s.back() & 0xC0) == 0x80) s.pop_back(); if (!s.empty()) s.pop_back(); s += "…"; }
            ui::text(r, x0, y2, ts, s, hex(TEXTE, 0.85f), ui::GAUCHE, 0.f, false);
        }
        const float wa = largeur("Annuler");
        u.z[1] = {px + pw - 20 * k - wa, py + ph - 16 * k - bh, wa, bh};
        bouton(r, u.z[1], k, false, "Annuler", tp);
        ui::text(r, x0, u.z[1].y + (bh - ts) / 2, ts, "Tu peux rouler pendant ce temps", hex(ACCENT2), ui::GAUCHE, 0.f, false);
        break;
    }
    case EtatCarte::Installee: {
        const std::string l = "Supprimer — libère " + texteMo(e.surDisque);
        const float bw = largeur(l);
        u.z[1] = {px0 - bw, y + m, bw, bh};
        bouton(r, u.z[1], k, false, l, tp);
        break;
    }
    default: break;
    }
    if (t - u.tMessage < MESSAGE_S && !u.message.empty()) {   // message passager (RG-05, place insuffisante, suppression faite)
        const float mw = std::min(w - 2 * m, ui::width(tp, u.message) + 40 * k), mh = 40 * k, mx = x + (w - mw) / 2, my = y + h * 0.42f;
        bloc(r, mx, my, mw, mh, k);
        ui::text(r, mx + mw / 2, my + (mh - tp) / 2, tp, u.message, hex(TEXTE), ui::CENTRE, 0.f, false);
    }
}

bool toucherPaquetCarte(const std::string& c, int id, float x, float y, bool down, double t) {
    UiTerrain& u = uiTerrain();
    auto zoneSous = [&](float px, float py) { for (int i = 0; i < 4; i++) if (u.z[i].contient(px, py)) return i; return -1; };
    if (down) {
        if (u.doigt >= 0) return u.dialogue != UiTerrain::Aucun;
        const int zi = zoneSous(x, y);
        if (zi < 0 && u.dialogue == UiTerrain::Aucun) return false;   // le terrain garde ses touchers
        u.doigt = id; u.zoneDoigt = zi;
        return true;
    }
    if (id != u.doigt) return false;
    u.doigt = -1;
    const int zi = zoneSous(x, y);
    if (zi != u.zoneDoigt) { if (u.dialogue != UiTerrain::Aucun && zi < 0 && u.zoneDoigt < 0) u.dialogue = UiTerrain::Aucun; return true; }   // appui à côté : confirmation refermée
    CartesPaquets& cp = cartesPaquets();
    u.message.clear();   // un nouvel appui efface le message passager précédent
    auto message = [&](const std::string& m) { u.message = m; u.tMessage = t; };
    auto lancer = [&]() { std::string m; if (!cp.telecharger(c, m)) message(m); };
    const EtatCarte e = cp.etat(c);
    if (u.dialogue != UiTerrain::Aucun) {
        const UiTerrain::Dialogue d = u.dialogue;
        u.dialogue = UiTerrain::Aucun;
        if (zi == 2 && d == UiTerrain::Supprimer) { long long l = 0; if (cp.supprimer(c, l)) message(e.nom + " supprimée : " + texteMo(l) + " libérés"); }
        else if (zi == 2 && d == UiTerrain::Mobile) lancer();
        else WFS_LOG("paquets (WFS-307) : %s : confirmation refusée", c.c_str());
        return true;
    }
    if (zi == 0) {   // Télécharger / Mise à jour / Réessayer
        if (cp.reseau() == 2) { u.dialogue = UiTerrain::Mobile; u.carteDialogue = c; WFS_LOG("paquets (WFS-307) : %s : données mobiles, confirmation demandée", c.c_str()); }
        else lancer();
    } else if (zi == 1) {
        if (e.etat == EtatCarte::Installee) { u.dialogue = UiTerrain::Supprimer; u.carteDialogue = c; }
        else cp.annuler(c);
    }
    return true;
}

bool centrePaquetCarte(int quelle, float& x, float& y) {
    const EtapePaquets::Zone& z = uiTerrain().z[std::clamp(quelle, 0, 3)];
    x = z.x + z.w / 2; y = z.y + z.h / 2;
    return z.w > 0;
}

// ---------------------------------------------------------------- bandeau du menu (RG-06)
// Après une étape finie sans paquet : « Paquet de la Ville disponible — Installer » si la liste le propose ; sans liste (hors ligne),
// la plateforme est interrogée au menu au plus une fois par minute et la liste relue quand la connexion revient. « Installer » ouvre la
// même proposition (taille, Wi-Fi) par-dessus le menu ; le téléchargement continue au menu, la carte installée sert au chargement suivant.
class BandeauPaquets : public System {
public:
    const char* name() const override { return "bandeau des paquets"; }
    void update(double dt, const GameView& g) override {
        // WFS-307 : cartes suivies (fin d'un téléchargement → « prête », relue à la course suivante) ; RG-03 : débit réduit en course
        // sous 30 images par seconde (moyenne glissante d'environ 1 s), pleine vitesse ailleurs
        CartesPaquets& cp = cartesPaquets();
        cp.avancer();
        if (g.ecran == Ecran::Course && dt > 0) dtMoyen_ += (std::min(dt, 0.5) - dtMoyen_) * std::min(1.0, dt);
        else dtMoyen_ = 1.0 / 60;
        cp.ralentir(g.ecran == Ecran::Course && dtMoyen_ > 1.0 / 30);
        if (g.ecran == Ecran::Menu) {   // RG-03 : « Corniche prête » au retour au menu
            for (const std::string& c : cp.pretes()) {
                const EtatCarte ec = cp.etat(c);
                annonces_.push_back(ec.nom + " prête");
                WFS_LOG("paquets (WFS-307) : menu, annonce « %s prête »", ec.nom.c_str());
            }
            if (!annonces_.empty() && tAnnonce_ < 0) tAnnonce_ = g.time;
            if (tAnnonce_ >= 0 && g.time - tAnnonce_ > ANNONCE_S) { annonces_.erase(annonces_.begin()); tAnnonce_ = annonces_.empty() ? -1 : g.time; }
        }
        EtapePaquets& e = paquets();
        SourcePaquets* s = e.source();
        if (!s || !e.secours_ || g.ecran != Ecran::Menu) return;
        if (e.enMenu()) { e.avancer(g.time); return; }   // proposition ouverte : réseau (Wi-Fi / données mobiles) suivi
        long long recus = 0, taille = 0;
        const SourcePaquets::Phase ph = s->phase(recus, taille);
        if (ph == SourcePaquets::ABSENTE && g.time - dernier_ >= 60) {   // connexion revenue ? (une fois par minute au plus)
            dernier_ = g.time;
            if (s->reseau() != 0) { s->chercher(e.carte(), e.formats_, e.empreinte_, e.apk_); WFS_LOG("paquets (WFS-306) : menu, connexion présente : liste relue"); }
        }
    }
    void onEcran(Ecran, Ecran, const GameView&) override { paquets().fermerMenu(); }
    void shutdown(Renderer&) override { paquets().fermerMenu(); }   // fenêtre perdue en jeu (termGraphics) : la proposition du menu se referme
    void drawUi(Renderer& r, const GameView& g) override {
        z_ = {};
        if (g.ecran == Ecran::Menu && !g.sceneDediee && ui::fontReady() && !annonces_.empty() && !paquets().enMenu()) {   // WFS-307 (RG-03)
            const bool cp = ui::petitEcran(g.uiWidth, g.uiHeight);
            const float k = cp ? ui::unitePetitEcran(g.uiWidth, g.uiHeight) : g.uiHeight / 720.f, tp = 16 * k;
            const std::string t = annonces_.front() + " pour la prochaine course";
            const float w = ui::width(tp, t) + 64 * k, h = 48 * k, x = (g.uiWidth - w) / 2, y = g.marges.haut + 20 * k;
            dessin2d::biseau(r, x - 1, y - 1, w + 2, h + 2, 10 * k, dessin2d::uni(hex(ACCENT2, 0.6f)));
            dessin2d::biseau(r, x, y, w, h, 10 * k, dessin2d::uni(SURFACE));
            r.uiCircle(x + 26 * k, y + h / 2, 6 * k, hex(ACCENT2));
            ui::text(r, x + 44 * k, y + (h - tp) / 2, tp, t, hex(TEXTE), ui::GAUCHE, 0.f, false);
        }
        EtapePaquets& e = paquets();
        SourcePaquets* s = e.source();
        if (!s || !e.secours_ || g.ecran != Ecran::Menu || g.sceneDediee || !ui::fontReady()) return;
        if (e.enMenu()) { e.dessiner(r, g.uiWidth, g.uiHeight, g.marges, g.time); return; }   // RG-02 par-dessus le menu
        long long recus = 0, taille = 0;
        const SourcePaquets::Phase ph = s->phase(recus, taille);
        std::string nom, fmt, t;
        if (!s->proposition(nom, fmt)) return;
        if (ph == SourcePaquets::PRETE) t = "Paquet de la " + nom + " disponible — Installer";
        else if (ph == SourcePaquets::TELECHARGE || ph == SourcePaquets::COUPURE) t = nom + " — " + texteMo(recus) + " / " + texteMo(taille);
        else if (ph == SourcePaquets::FINI) t = nom + " installée : prête pour la prochaine course";   // WFS-307 (RG-08)
        else return;
        const bool cp = ui::petitEcran(g.uiWidth, g.uiHeight);
        const float k = cp ? ui::unitePetitEcran(g.uiWidth, g.uiHeight) : g.uiHeight / 720.f, tp = (cp ? 14 : 15) * k;
        const float w = ui::width(tp, t, 0.02f) + 40 * k, h = std::max(44 * k, ui::TOUCHER_MIN_DP * ui::unitePetitEcran(g.uiWidth, g.uiHeight));
        const float x = g.uiWidth - g.marges.droite - 16 * k - w, y = g.uiHeight - g.marges.bas - 16 * k - h;
        dessin2d::biseau(r, x - 1, y - 1, w + 2, h + 2, 10 * k, dessin2d::uni(BORD));
        dessin2d::biseau(r, x, y, w, h, 10 * k, dessin2d::uni(GLASS));
        if (ph == SourcePaquets::TELECHARGE || ph == SourcePaquets::COUPURE) barre(r, x + 20 * k, y + h - 8 * k, w - 40 * k, 3 * k, taille > 0 ? (float)recus / taille : 0.f);
        ui::text(r, x + w / 2, y + (h - tp) / 2, tp, t, ph == SourcePaquets::PRETE ? hex(ACCENT) : hex(ACCENT2), ui::CENTRE, 0.02f, false);
        if (ph == SourcePaquets::PRETE) z_ = {x, y, w, h};
    }
    bool onTouch(const Touch& t, const GameView& g) override {
        EtapePaquets& e = paquets();
        if (g.ecran == Ecran::Menu && e.enMenu()) { e.toucher(t.id, t.x, t.y, t.down); return true; }   // la proposition garde les touchers
        if (g.ecran != Ecran::Menu || !z_.contient(t.x, t.y)) return t.id == doigt_ && !t.down ? (doigt_ = -1, true) : false;
        if (t.down) { doigt_ = t.id; return true; }
        if (t.id == doigt_) { doigt_ = -1; e.ouvrirProposition(g.time); }   // RG-02 : rien ne part sans « Installer » sur la proposition
        return true;
    }
    // banc : centre du bandeau (faux s'il n'est pas affiché)
    static bool centre(float& x, float& y) { x = zone().x + zone().w / 2; y = zone().y + zone().h / 2; return zone().w > 0; }
    static bool contient(float x, float y) { return zone().contient(x, y); }
private:
    static EtapePaquets::Zone& zone() { static EtapePaquets::Zone z; return z; }
    double dernier_ = -1e9, dtMoyen_ = 1.0 / 60, tAnnonce_ = -1;
    static constexpr double ANNONCE_S = 4.0;
    std::vector<std::string> annonces_;
    EtapePaquets::Zone& z_ = zone();
    int doigt_ = -1;
};

// ---------------------------------------------------------------- banc d'essai (WFS_BANC=paquets)
namespace {
// source factice : liste présente ou non, réseau, place ; le téléchargement copie un paquet cuit (WFS-301) par morceaux à chaque appel
struct SourceFactice : SourcePaquets {
    std::string racine, carte, formats;
    bool liste = true, shaFaux = false;
    int reseauFactice = 1;
    long long place = 1LL << 40, coupureA = -1, parImage = 1 << 20;
    Phase ph = RIEN;
    long long recus = 0, taille = 0;
    int appelsListe = 0;
    std::vector<uint8_t> octets;
    void chercher(const std::string& c, const std::string& f, const std::string&, int) override {
        carte = c; formats = f; appelsListe++;
        if (!liste) { ph = ABSENTE; return; }
        octets = carte::ecrire(cartes_v2::cuire(racine, c, carte::FormatTexture::ASTC));
        taille = (long long)octets.size(); recus = 0; ph = PRETE;
    }
    Phase phase(long long& r, long long& t) override {
        if (ph == TELECHARGE) {
            if (coupureA >= 0 && recus >= coupureA) { ph = COUPURE; coupureA = -1; }
            else {
                recus = std::min(taille, recus + parImage);
                if (recus >= taille) {
                    if (shaFaux) ph = SHA_FAUX;
                    else {
                        std::ofstream o(carte::dossierPaquets() + "/" + carte::nomFichier(carte, carte::FormatTexture::ASTC), std::ios::binary);
                        o.write((const char*)octets.data(), (std::streamsize)octets.size());
                        ph = FINI;
                    }
                }
            }
        } else if (ph == COUPURE && reseauFactice != 0) ph = TELECHARGE;   // reprise automatique dès que le réseau revient
        r = recus; t = taille; return ph;
    }
    bool proposition(std::string& n, std::string& f) override { if (taille <= 0) return false; n = "Ville"; f = "astc"; return true; }
    int reseau() override { return reseauFactice; }
    long long placeLibre() override { return place; }
    void installer() override { if (taille > 0) ph = TELECHARGE; }
    void arreter() override { if (ph == TELECHARGE || ph == COUPURE) ph = PRETE; }
};
// source factice de plusieurs cartes (WFS-307) : « plage » (la Corniche) a un paquet publié ; file d'attente une carte à la fois ;
// pas() fait avancer le téléchargement d'un morceau (appelé une fois par image par le banc)
struct SourceMulti : SourcePaquets {
    struct C { Phase ph = RIEN; long long recus = 0, taille = 0; std::vector<uint8_t> octets; std::string nom; };
    std::string racine;
    std::vector<std::string> publiees{"plage"};
    std::vector<std::pair<std::string, C>> cs;
    std::vector<std::string> file;
    int reseauFactice = 1, appelsTout = 0;
    long long place = 1LL << 40, parImage = 256 * 1024;
    bool ralenti = false;
    C& k(const std::string& c) { for (auto& [n, x] : cs) if (n == c) return x; cs.push_back({c, C{}}); return cs.back().second; }
    static bool occupee(const C& x) { return x.ph == TELECHARGE || x.ph == COUPURE || x.ph == ATTENTE; }
    void chercher(const std::string&, const std::string&, const std::string&, int) override {}
    Phase phase(long long& r, long long& t) override { r = t = 0; return ABSENTE; }
    bool proposition(std::string&, std::string&) override { return false; }
    int reseau() override { return reseauFactice; }
    long long placeLibre() override { return place; }
    void installer() override {}
    void arreter() override {}
    void chercherTout(const std::string&, const std::string& empreintes, int) override {
        appelsTout++;
        size_t i = 0;
        while (i < empreintes.size()) {
            const size_t f = std::min(empreintes.find(',', i), empreintes.size()), d = empreintes.find(':', i);
            const std::string c = empreintes.substr(i, std::min(d, f) - i);
            i = f + 1;
            C& x = k(c);
            if (occupee(x)) continue;
            if (std::find(publiees.begin(), publiees.end(), c) == publiees.end()) { x.ph = ABSENTE; continue; }
            if (x.octets.empty()) x.octets = carte::ecrire(cartes_v2::cuire(racine, c, carte::FormatTexture::ASTC));
            x.taille = (long long)x.octets.size(); x.nom = mapById(c.c_str()).name; x.ph = PRETE;
        }
    }
    Phase phaseCarte(const std::string& c, long long& r, long long& t) override { C& x = k(c); r = x.recus; t = x.taille; return x.ph; }
    bool propositionCarte(const std::string& c, std::string& n, std::string& f) override { C& x = k(c); if (x.octets.empty()) return false; n = x.nom; f = "astc"; return true; }
    bool installerCarte(const std::string& c) override {
        C& x = k(c);
        if (x.octets.empty() || occupee(x)) return false;
        x.recus = 0; x.ph = ATTENTE; file.push_back(c);
        return true;
    }
    void arreterCarte(const std::string& c) override {
        const auto it = std::find(file.begin(), file.end(), c);
        if (it != file.end()) { file.erase(it); k(c).ph = PRETE; }
    }
    void supprimee(const std::string& c) override { C& x = k(c); if (!occupee(x)) { x.recus = 0; x.ph = x.octets.empty() ? ABSENTE : PRETE; } }
    void ralentir(bool oui) override { ralenti = oui; }
    void pas() {
        if (file.empty() || reseauFactice == 0) return;
        C& x = k(file.front());
        x.ph = TELECHARGE;
        x.recus = std::min(x.taille, x.recus + (ralenti ? parImage / 4 : parImage));
        if (x.recus < x.taille) return;
        std::ofstream o(carte::dossierPaquets() + "/" + carte::nomFichier(file.front(), carte::FormatTexture::ASTC), std::ios::binary);
        o.write((const char*)x.octets.data(), (std::streamsize)x.octets.size());
        x.ph = FINI;
        file.erase(file.begin());
    }
};
} // namespace

int benchPaquets(Game& g, Renderer& r, const std::string& out, const std::string& racine) {
    int ecarts = 0;
    auto verifier = [&](bool ok, const char* quoi) { WFS_LOG("paquets (WFS-306) banc : %s : %s", quoi, ok ? "OK" : "ÉCART"); if (!ok) ecarts++; };
    namespace fs = std::filesystem;
    std::error_code ec;
    const std::string c = "ville";
    fs::create_directories(carte::dossierPaquets(), ec);
    auto retirer = [&]() { for (carte::FormatTexture f : carte::FORMATS) { fs::remove(cheminPaquet(c, f), ec); fs::remove(cheminPaquet(c, f) + ".part", ec); } };
    retirer();
    ui::chargerPolice(r);
    EtapePaquets& e = paquets();
    double t = 100;
    auto image = [&](int n = 1) {
        for (int i = 0; i < n; i++) {
            t += 1.0 / 30; e.avancer(t);
            if (!e.ouvert()) break;
            FrameParams fp; fp.view = mat4::identity(); fp.proj = mat4::identity(); fp.camPos = {0, 0, 0};
            if (r.beginFrame(fp)) { e.dessiner(r, (float)r.width(), (float)r.height(), Marges{}, t); r.endFrame(); }
        }
    };
    auto capture = [&](const char* nom, const char* quoi) {
        FrameParams fp; fp.view = mat4::identity(); fp.proj = mat4::identity(); fp.camPos = {0, 0, 0};
        if (r.beginFrame(fp)) { e.dessiner(r, (float)r.width(), (float)r.height(), Marges{}, t); r.endFrame(); }
        r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : %s", nom, quoi);
    };
    auto appui = [&](const EtapePaquets::Zone& z) { const float x = z.x + z.w / 2, y = z.y + z.h / 2; e.toucher(1, x, y, true); e.toucher(1, x, y, false); image(); };
    const std::vector<carte::FormatTexture> formats = {carte::FormatTexture::ASTC, carte::FormatTexture::RGBA8};
    SourceFactice s; s.racine = racine;

    // 1) hors ligne (pas de liste) : secours silencieux, aucune image de l'étape
    s.liste = false;
    verifier(e.demarrer(r, s, c, formats), "paquet absent → étape ouverte");
    image(3);
    verifier(!e.ouvert() && e.secours_, "hors ligne : pas de liste → secours silencieux (Ville du code)");
    // 2) proposition sur données mobiles, puis sur Wi-Fi
    s = SourceFactice(); s.racine = racine; s.reseauFactice = 2; s.parImage = 64 * 1024;
    verifier(e.demarrer(r, s, c, formats), "liste présente → étape ouverte");
    image(3);
    verifier(e.etat() == EtapePaquets::Etat::Proposition, "proposition (taille annoncée avant tout envoi)");
    capture("paquets-proposition-mobile", "proposition sur données mobiles : phrase des données mobiles, « Installer » non mis en avant");
    verifier(s.ph == SourcePaquets::PRETE, "rien ne part sans appui sur « Installer »");
    s.reseauFactice = 1; image(2);
    capture("paquets-proposition-wifi", "proposition sur Wi-Fi : « Installer » mis en avant (orange)");
    // 3) « Installer » : barres, astuces, temps restant ; coupure puis reprise automatique
    s.coupureA = 2 << 20;
    appui(e.zoneInstaller());
    verifier(e.etat() == EtapePaquets::Etat::Telechargement, "« Installer » → téléchargement");
    s.reseauFactice = 0;   // Wi-Fi coupé
    image(40);
    capture("paquets-coupure", "connexion perdue : « Connexion perdue, reprise automatique »");
    verifier(s.ph == SourcePaquets::COUPURE, "coupure vue par l'étape");
    s.reseauFactice = 1;   // Wi-Fi revenu : reprise là où il s'était arrêté
    image(20);
    {   // touche Accueil pendant le téléchargement : fenêtre perdue (police libérée, renderer fermé), le téléchargement continue, retour
        long long r0 = 0, tt = 0; s.phase(r0, tt);
        VkCtx& vk = r.vk();
        e.perdreFenetre(r); r.shutdown();
        verifier(e.ouvert() && !e.tientLaFenetre() && !ui::fontReady(), "touche Accueil : étape gardée, police libérée");
        long long r1 = 0; for (int i = 0; i < 20; i++) s.phase(r1, tt);   // le fil Java continue sans fenêtre
        r.init(vk);
        verifier(e.reprendre(r) && e.tientLaFenetre() && ui::fontReady() && r1 > r0, "retour dans le jeu : étape reprise, téléchargement continué");
    }
    image(140);   // > 5 s : temps restant affiché
    capture("paquets-telechargement", "barre de la carte, barre totale, temps restant, astuce");
    image(400);
    verifier(!e.ouvert() && e.installe_ && !e.secours_, "paquet téléchargé et vérifié → fin de l'étape");
    verifier(cartes_v2::preparerCarte(c), "paquet installé → lu au chargement (WFS-301)");
    carte::desactiver();
    verifier(!e.demarrer(r, s, c, formats), "lancement suivant : paquet présent → pas de téléchargement");
    // 4) place insuffisante → message puis secours
    retirer();
    s = SourceFactice(); s.racine = racine; s.place = 1024 * 1024;
    e.demarrer(r, s, c, formats); image(3);
    verifier(e.etat() == EtapePaquets::Etat::Place, "place insuffisante → message");
    capture("paquets-place", "« Place insuffisante : X Mo nécessaires, Y Mo libres »");
    appui(e.zoneContinuer());
    verifier(!e.ouvert() && e.secours_, "place insuffisante, « Continuer » → secours");
    // 5) fichier faux → « Réessayer » / « Plus tard »
    s = SourceFactice(); s.racine = racine; s.shaFaux = true; s.parImage = 8 << 20;
    e.demarrer(r, s, c, formats); image(3); appui(e.zoneInstaller()); image(10);
    verifier(e.etat() == EtapePaquets::Etat::Echec, "paquet abîmé (2 essais) → échec");
    capture("paquets-echec", "paquet abîmé : « Réessayer » / « Plus tard »");
    appui(e.zonePlusTard());
    verifier(!e.ouvert() && e.secours_, "« Plus tard » → secours (Ville du code)");
    // 6) « Plus tard » pendant une coupure (défaut relevé par le lead dev, PR #286) : phase PRETE, le bandeau propose de nouveau l'installation
    s = SourceFactice(); s.racine = racine; s.parImage = 64 * 1024; s.coupureA = 1 << 20;
    e.demarrer(r, s, c, formats); image(3); appui(e.zoneInstaller());
    s.reseauFactice = 0; image(30);
    verifier(s.ph == SourcePaquets::COUPURE, "coupure pendant le téléchargement");
    appui(e.zonePlusTard());
    long long rr = 0, tt = 0;
    verifier(!e.ouvert() && e.secours_ && s.phase(rr, tt) == SourcePaquets::PRETE, "« Plus tard » pendant une coupure → phase PRETE (installation de nouveau possible)");
    s.reseauFactice = 1;
    // 7) menu : bandeau (RG-06) ; l'appui ouvre la proposition de RG-02 par-dessus le menu (rien ne part sans « Installer »)
    if (!g.init(r, "golf", "ville")) { WFS_ERR("paquets (WFS-306) banc : chargement impossible"); retirer(); return ecarts + 1; }
    g.setAutopilot(true); g.camMode = 0;
    double tg = 1000;
    auto imageJeu = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); tg += 1.0 / 60; g.render(tg); } };
    auto captureJeu = [&](const char* nom, const char* quoi) { g.render(tg); g.render(tg + 0.01); r.capturePng(out + "/" + nom + ".png"); WFS_LOG("capture %s : %s", nom, quoi); };
    auto toucherJeu = [&](int id, float x, float y) { g.onTouch({id, x, y, true}); imageJeu(1); g.onTouch({id, x, y, false}); imageJeu(2); };
    for (int i = 0; i < 30; i++) g.update(1.0 / 60);
    g.allerA(Ecran::Menu);
    imageJeu(60);
    captureJeu("paquets-bandeau-menu", "menu, bandeau « Paquet de la Ville disponible — Installer » (après « Plus tard » pendant une coupure)");
    float bx = 0, by = 0;
    verifier(BandeauPaquets::centre(bx, by), "bandeau affiché au menu");
    s.reseauFactice = 2;
    toucherJeu(40, bx, by);
    verifier(e.enMenu() && e.etat() == EtapePaquets::Etat::Proposition && s.ph == SourcePaquets::PRETE, "bandeau → proposition par-dessus le menu, rien ne part");
    captureJeu("paquets-menu-proposition-mobile", "proposition depuis le bandeau, données mobiles : taille et phrase des données mobiles");
    s.reseauFactice = 1; imageJeu(3);
    captureJeu("paquets-menu-proposition-wifi", "proposition depuis le bandeau, Wi-Fi : « Installer » mis en avant");
    { const EtapePaquets::Zone& z = e.zonePlusTard(); toucherJeu(41, z.x + z.w / 2, z.y + z.h / 2); }
    verifier(!e.enMenu() && g.ecran() == Ecran::Menu && s.ph == SourcePaquets::PRETE, "« Plus tard » → proposition refermée, menu inchangé");
    toucherJeu(42, bx, by);
    { const EtapePaquets::Zone& z = e.zoneInstaller(); toucherJeu(43, z.x + z.w / 2, z.y + z.h / 2); }
    verifier(!e.enMenu() && (s.ph == SourcePaquets::TELECHARGE || s.ph == SourcePaquets::FINI), "« Installer » sur la proposition du menu → téléchargement");
    imageJeu(4);
    captureJeu("paquets-bandeau-progression", "bandeau du menu pendant le téléchargement (barre)");
    // hors ligne au menu puis connexion revenue : la liste est relue (une fois par minute au plus)
    s.liste = false; s.ph = SourcePaquets::ABSENTE; s.reseauFactice = 1; const int avant = s.appelsListe;
    for (int i = 0; i < 70 * 60; i++) g.update(1.0 / 60);
    verifier(s.appelsListe > avant, "connexion présente au menu → liste relue");
    // 8) WFS-307 (RG-07) : choix du terrain → Corniche « Télécharger » → course sur la Ville pendant le téléchargement → retour au menu,
    // « Corniche prête » → course sur la Corniche avec son paquet, sans relancer (RG-08) → « Supprimer » → de nouveau « Télécharger »
    {
        const std::string co = "plage";
        auto retirerCo = [&]() { for (carte::FormatTexture f : carte::FORMATS) { fs::remove(cheminPaquet(co, f), ec); fs::remove(cheminPaquet(co, f) + ".part", ec); } };
        retirerCo();
        SourceMulti sm; sm.racine = racine; sm.parImage = 32 * 1024;
        CartesPaquets& cp = cartesPaquets();
        cp.lier(&sm, formats, 0);
        int chargements = 0; bool paquetLu = false; std::string cible;
        Game::demandes().debutChargement = [&] { chargements++; paquetLu = cartes_v2::preparerCarte(cible); };   // comme android_main.cpp
        int nImages = 0;
        auto imageMulti = [&](int n) { for (int i = 0; i < n; i++) { sm.pas(); g.update(1.0 / 60); tg += 1.0 / 60; if (++nImages % 10 == 0) g.render(tg); } };   // rendu une image sur 10 (banc plus court)
        auto rouler = [&](const char* c, bool nuit) {
            choisirCarte(c, nuit); imageMulti(2);
            float rx = 0, ry = 0; centreRouler(rx, ry); cible = c;
            toucherJeu(50, rx, ry); imageMulti(4);
        };
        auto appuiPaquet = [&](int quelle) { float x = 0, y = 0; const bool ok = centrePaquetCarte(quelle, x, y); if (ok) toucherJeu(51 + quelle, x, y); imageMulti(2); return ok; };
        const std::string nomCo = mapById(co.c_str()).name;
        g.allerA(Ecran::Terrain); imageMulti(2);
        choisirCarte(co, false); imageMulti(3);
        verifier(sm.appelsTout == 1 && cp.etat(co).etat == EtatCarte::Disponible, "choix du terrain : liste lue, Corniche « Télécharger »");
        verifier(cp.etat("ville").etat == EtatCarte::SansPaquet, "Ville sans paquet publié : jouable comme avant");
        captureJeu("paquets-terrain-telecharger", "Corniche assombrie, « Télécharger — X Mo » (jour)");
        // RG-05 : sans Internet, « Connexion nécessaire » ; RG-02 : données mobiles confirmées, « Plus tard » ne lance rien
        sm.reseauFactice = 0; appuiPaquet(0);
        verifier(sm.k(co).ph == SourcePaquets::PRETE, "sans Internet : « Connexion nécessaire », rien ne part");
        captureJeu("paquets-terrain-sans-connexion", "sans Internet : message « Connexion nécessaire »");
        sm.reseauFactice = 2; appuiPaquet(0);
        captureJeu("paquets-terrain-mobile", "données mobiles : « Vous êtes sur les données mobiles (X Mo) », « Télécharger » non mis en avant");
        appuiPaquet(3);
        verifier(sm.k(co).ph == SourcePaquets::PRETE, "données mobiles, « Plus tard » : rien ne part");
        sm.reseauFactice = 1; appuiPaquet(0);
        verifier(sm.file.size() == 1, "Wi-Fi, « Télécharger » : Corniche dans la file");
        imageMulti(30);
        verifier(cp.etat(co).etat == EtatCarte::Telechargement, "barre de progression sur la vignette");
        captureJeu("paquets-terrain-progression", "Corniche : barre, octets reçus, astuce, « Annuler » (jour)");
        // RG-03 : course sur la Ville pendant le téléchargement (la Ville est déjà chargée : pas de reconstruction)
        rouler("ville", false);
        verifier(g.ecran() != Ecran::Terrain && chargements == 0, "« Rouler » sur la Ville (déjà chargée, jour) pendant le téléchargement : pas de reconstruction");
        imageMulti(240);
        const long long pendant = sm.k(co).recus;
        verifier(pendant > 0 && pendant < sm.k(co).taille && cp.etat(co).etat == EtatCarte::Telechargement, "course sur la Ville : téléchargement continué");
        captureJeu("paquets-course-pendant", "course sur la Ville pendant le téléchargement de la Corniche");
        imageMulti(1200);
        verifier(sm.k(co).ph == SourcePaquets::FINI && cp.changee(co), "Corniche finie pendant la course, marquée « relue à la prochaine course »");
        g.allerA(Ecran::Menu); imageMulti(20);
        captureJeu("paquets-menu-prete", "retour au menu : « Corniche prête pour la prochaine course »");
        verifier(cp.pretes().empty(), "annonce « Corniche prête » prise par le menu");
        g.allerA(Ecran::Terrain); imageMulti(2); choisirCarte(co, false); imageMulti(2);
        verifier(cp.etat(co).etat == EtatCarte::Installee, "Corniche installée au choix du terrain");
        captureJeu("paquets-terrain-installee", "Corniche installée : « Supprimer — libère X Mo »");
        // RG-08 : course sur la Corniche juste après, sans relancer l'application : paquet vérifié et lu
        const long long lusAvant = carte::lusDansPaquet();
        rouler(co.c_str(), false);
        verifier(chargements == 1 && paquetLu, "« Rouler » sur la Corniche : carte construite avec son paquet (RG-08)");
        WFS_LOG("paquets (WFS-307) banc : fichiers servis par le paquet de la Corniche : %lld (avant : %lld)", carte::lusDansPaquet(), lusAvant);
        verifier(carte::lusDansPaquet() > 0, "fichiers de la Corniche lus dans le paquet");
        imageMulti(240);
        captureJeu("paquets-course-corniche", "course sur la Corniche lue dans son paquet téléchargé");
        // RG-04 : supprimer avec confirmation, la carte revient à « Télécharger »
        g.allerA(Ecran::Terrain); imageMulti(2); choisirCarte(co, false); imageMulti(2);
        appuiPaquet(1);
        captureJeu("paquets-terrain-supprimer", "confirmation « Supprimer le paquet de la Corniche ? Libère X Mo »");
        appuiPaquet(2);
        verifier(!fs::exists(cheminPaquet(co, carte::FormatTexture::ASTC), ec) && cp.etat(co).etat == EtatCarte::Disponible, "Corniche supprimée → de nouveau « Télécharger »");
        imageMulti(4);
        captureJeu("paquets-terrain-apres-suppression", "Corniche supprimée : « X libérés », de nouveau « Télécharger »");
        rouler(co.c_str(), false);
        verifier(chargements == 2 && !paquetLu, "course suivante sur la Corniche : relue, construite par le code (secours)");
        // même carte, paquet inchangé : aucune reconstruction
        g.allerA(Ecran::Terrain); imageMulti(2);
        rouler(co.c_str(), false);
        verifier(chargements == 2, "même carte, paquet inchangé : pas de reconstruction");
        Game::demandes().debutChargement = nullptr;
        cp.lier(nullptr, formats, 0);
        carte::desactiver();
        retirerCo();
    }
    g.shutdown();
    retirer();
    WFS_LOG("paquets (WFS-306) banc : %d écart(s)%s", ecarts, ecarts ? " ÉCHEC" : " OK");
    return ecarts;
}

} // namespace ecrans

namespace ecrans {
bool toucheBandeauPaquets(float x, float y) { float bx, by; return paquets().enMenu() || (BandeauPaquets::centre(bx, by) && BandeauPaquets::contient(x, y)); }
} // namespace ecrans

WFS_SYSTEM(ecrans::BandeauPaquets, 458)   // interface (400-499) : par-dessus le menu (450), sous les réglages (460)

} // namespace wfs
