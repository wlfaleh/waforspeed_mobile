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
constexpr double FIN_AFFICHEE_S = 0.6;       // « Ville installée » avant la suite du chargement

std::string hexa(uint64_t v) { char b[17]; std::snprintf(b, sizeof b, "%016llx", (unsigned long long)v); return b; }

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
std::string texteFormats(const std::vector<carte::FormatTexture>& f) {
    std::string s;
    for (carte::FormatTexture x : f) s += std::string(s.empty() ? "" : ",") + carte::nomFormat(x);
    return s;
}
int versionApk() {
    const std::string v = versionJeu();   // « 0.1.N » sur l'APK de la CI, « dev » ailleurs
    const size_t p = v.rfind('.');
    if (p == std::string::npos) return 0;
    int n = 0;
    for (size_t i = p + 1; i < v.size() && v[i] >= '0' && v[i] <= '9' && n < 100000000; i++) n = n * 10 + (v[i] - '0');
    return n;
}
std::string texteMo(long long o) { return ui::nombre(std::max(1LL, (o + 524288) / 1048576)) + " Mo"; }

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
    r_ = &r; source_ = &s; carte_ = c; fenetre_ = true; installe_ = false; secours_ = false;
    const uint64_t e = cartes_v2::empreinteAttendue(c);
    empreinte_ = hexa(e); formats_ = texteFormats(formats);
    for (carte::FormatTexture f : formats)   // RG-01 : paquet juste et à jour déjà là → rien de plus
        if (carte::verifier_carte(cheminPaquet(c, f), {c, f, e}) == carte::Verdict::Bon) {
            WFS_LOG("paquets (WFS-306) : %s présent et vérifié, pas de téléchargement", carte::nomFichier(c, f).c_str());
            etat_ = Etat::Ferme;
            return false;
        }
    etat_ = Etat::Recherche; t0_ = nowSeconds(); recus_ = taille_ = 0; tDebutTelechargement_ = tFini_ = -1; message_.clear(); doigt_ = -1; essaisRotation_ = 0;
    s.chercher(c, formats_, empreinte_, versionApk());
    WFS_LOG("paquets (WFS-306) : %s absent ou ancien, recherche de la liste (formats %s, empreinte %s, APK %d)", c.c_str(), formats_.c_str(), empreinte_.c_str(), versionApk());
    return true;
}

void EtapePaquets::finir(bool avecPaquet, const char* raison) {
    if (!avecPaquet) secours_ = true;
    tFini_ = maintenant_;
    etat_ = Etat::Fini;
    WFS_LOG("paquets (WFS-306) : fin de l'étape, %s (%s)", avecPaquet ? "paquet installé" : "carte du code (secours)", raison);
}

void EtapePaquets::avancer(double t) {
    maintenant_ = t;
    if (!source_ || !ouvert()) return;
    long long recus = 0, taille = 0;
    const SourcePaquets::Phase ph = source_->phase(recus, taille);
    switch (etat_) {
    case Etat::Recherche:
        if (ph == SourcePaquets::ABSENTE) { finir(false, "pas de liste ou rien pour ce téléphone"); break; }
        if (ph == SourcePaquets::PRETE) {
            source_->proposition(nom_, formatPropose_);
            taille_ = taille;
            reseau_ = source_->reseau();
            const long long libre = source_->placeLibre(), besoin = taille + taille / 10;   // RG-07 (le reste exact est revu par Java)
            if (libre >= 0 && libre < besoin) {
                message_ = "Place insuffisante : " + texteMo(besoin) + " nécessaires, " + texteMo(libre) + " libres";
                etat_ = Etat::Place;
                WFS_LOG("paquets (WFS-306) : %s", message_.c_str());
                break;
            }
            etat_ = Etat::Proposition;
            WFS_LOG("paquets (WFS-306) : proposition %s (%s, %s), réseau %s", nom_.c_str(), formatPropose_.c_str(), texteMo(taille_).c_str(),
                    reseau_ == 1 ? "Wi-Fi" : reseau_ == 2 ? "données mobiles" : "aucun");
        }
        break;
    case Etat::Proposition:
        reseau_ = source_->reseau();
        break;
    case Etat::Telechargement:
        recus_ = recus; if (taille > 0) taille_ = taille;
        coupure_ = ph == SourcePaquets::COUPURE;
        if (ph == SourcePaquets::FINI) {
            if (tFini_ < 0) { tFini_ = t; installe_ = true; WFS_LOG("paquets (WFS-306) : %s installé (%s)", nom_.c_str(), texteMo(taille_).c_str()); }
            if (t - tFini_ >= FIN_AFFICHEE_S) finir(true, "téléchargé et vérifié");
        } else if (ph == SourcePaquets::SHA_FAUX || ph == SourcePaquets::ERREUR) {
            message_ = ph == SourcePaquets::SHA_FAUX ? "Le paquet téléchargé est abîmé (deux essais)." : "Le téléchargement a échoué.";
            etat_ = Etat::Echec;
            WFS_LOG("paquets (WFS-306) : échec : %s", message_.c_str());
        } else if (ph == SourcePaquets::PLACE) {
            message_ = "Place insuffisante : " + texteMo(taille_ + taille_ / 10) + " nécessaires, " + texteMo(source_->placeLibre()) + " libres";
            etat_ = Etat::Place;
        }
        break;
    default: break;
    }
}

void EtapePaquets::dessiner(Renderer& r, float W, float H, const Marges& m, double t) {
    if (!ui::fontReady()) return;
    const bool cp = ui::petitEcran(W, H);
    const float k = cp ? ui::unitePetitEcran(W, H) : H / 720.f, kb = H / 720.f;
    dessinerChargement(r, W, H, 1.f, "Paquets de cartes", false, 1.f);   // fond de l'écran de chargement (logo, barre pleine)
    r.uiRect(0, 0, W, H, hex(BG, 0.82f));
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
    if (c == 1 && zInstaller_.contient(x, y) && (etat_ == Etat::Proposition || etat_ == Etat::Echec)) {
        WFS_LOG("paquets (WFS-306) : « %s » (%s)", etat_ == Etat::Echec ? "Réessayer" : "Installer", reseau_ == 1 ? "Wi-Fi" : reseau_ == 2 ? "données mobiles" : "sans réseau");
        etat_ = Etat::Telechargement; tDebutTelechargement_ = maintenant_; recusDebut_ = recus_; tFini_ = -1; coupure_ = false;
        source_->installer();
    } else if (c == 2 && zPlusTard_.contient(x, y)) {
        if (etat_ == Etat::Telechargement) source_->arreter();   // le .part est gardé : reprise à la prochaine installation
        finir(false, "« Plus tard »");
    } else if (c == 3 && zContinuer_.contient(x, y)) finir(false, "place insuffisante");
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

// ---------------------------------------------------------------- bandeau du menu (RG-06)
// Après une étape finie sans paquet : « Paquet de la Ville disponible — Installer » si la liste le propose ; sans liste (hors ligne),
// la plateforme est interrogée au menu au plus une fois par minute et la liste relue quand la connexion revient. « Installer » ouvre la
// même proposition (taille, Wi-Fi) par-dessus le menu ; le téléchargement continue au menu, la carte installée sert au chargement suivant.
class BandeauPaquets : public System {
public:
    const char* name() const override { return "bandeau des paquets"; }
    void update(double, const GameView& g) override {
        EtapePaquets& e = paquets();
        SourcePaquets* s = e.source();
        if (!s || !e.secours_ || g.ecran != Ecran::Menu) return;
        long long recus = 0, taille = 0;
        const SourcePaquets::Phase ph = s->phase(recus, taille);
        if (ph == SourcePaquets::ABSENTE && g.time - dernier_ >= 60) {   // connexion revenue ? (une fois par minute au plus)
            dernier_ = g.time;
            if (s->reseau() != 0) { s->chercher(e.carte(), e.formats_, e.empreinte_, versionApk()); WFS_LOG("paquets (WFS-306) : menu, connexion présente : liste relue"); }
        }
    }
    void drawUi(Renderer& r, const GameView& g) override {
        z_ = {};
        EtapePaquets& e = paquets();
        SourcePaquets* s = e.source();
        if (!s || !e.secours_ || g.ecran != Ecran::Menu || g.sceneDediee || !ui::fontReady()) return;
        long long recus = 0, taille = 0;
        const SourcePaquets::Phase ph = s->phase(recus, taille);
        std::string nom, fmt, t;
        if (!s->proposition(nom, fmt)) return;
        if (ph == SourcePaquets::PRETE) t = "Paquet de la " + nom + " disponible — Installer";
        else if (ph == SourcePaquets::TELECHARGE || ph == SourcePaquets::COUPURE) t = nom + " — " + texteMo(recus) + " / " + texteMo(taille);
        else if (ph == SourcePaquets::FINI) t = nom + " installée : utilisée à la prochaine partie";
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
        if (g.ecran != Ecran::Menu || !z_.contient(t.x, t.y)) return t.id == doigt_ && !t.down ? (doigt_ = -1, true) : false;
        if (t.down) { doigt_ = t.id; return true; }
        if (t.id == doigt_) {
            doigt_ = -1;
            SourcePaquets* s = paquets().source();
            if (s) { WFS_LOG("paquets (WFS-306) : bandeau du menu, « Installer » (%s)", s->reseau() == 2 ? "données mobiles" : "Wi-Fi"); s->installer(); }
        }
        return true;
    }
private:
    double dernier_ = -1e9;
    EtapePaquets::Zone z_;
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
    // 6) bandeau du menu (RG-06) : liste présente, paquet non installé
    s = SourceFactice(); s.racine = racine;
    e.demarrer(r, s, c, formats); image(3); appui(e.zonePlusTard());
    verifier(e.secours_, "« Plus tard » sur la proposition → secours");
    if (!g.init(r, "golf", "ville")) { WFS_ERR("paquets (WFS-306) banc : chargement impossible"); retirer(); return ecarts + 1; }
    g.setAutopilot(true); g.camMode = 0;
    double tg = 1000;
    for (int i = 0; i < 30; i++) g.update(1.0 / 60);
    g.allerA(Ecran::Menu);
    for (int i = 0; i < 60; i++) { g.update(1.0 / 60); tg += 1.0 / 60; g.render(tg); }
    g.render(tg); g.render(tg + 0.01); r.capturePng(out + "/paquets-bandeau-menu.png");
    WFS_LOG("capture paquets-bandeau-menu : menu, bandeau « Paquet de la Ville disponible — Installer »");
    // hors ligne au menu puis connexion revenue : la liste est relue (une fois par minute au plus)
    s.liste = false; s.ph = SourcePaquets::ABSENTE; s.reseauFactice = 1; const int avant = s.appelsListe;
    for (int i = 0; i < 70 * 60; i++) g.update(1.0 / 60);
    verifier(s.appelsListe > avant, "connexion présente au menu → liste relue");
    g.shutdown();
    retirer();
    WFS_LOG("paquets (WFS-306) banc : %d écart(s)%s", ecarts, ecarts ? " ÉCHEC" : " OK");
    return ecarts;
}

} // namespace ecrans

WFS_SYSTEM(ecrans::BandeauPaquets, 458)   // interface (400-499) : par-dessus le menu (450), sous les réglages (460)

} // namespace wfs
