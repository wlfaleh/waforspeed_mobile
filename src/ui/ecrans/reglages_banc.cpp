// WaForSpeed — WFS-420 : banc de performance automatique sur le téléphone, côté écrans. Voir reglages_banc.h et
// src/render/banc_telephone.h.
//
// Déroulé (RG-01, RG-02) : pour chaque essai de banc_tel::deroule(), la Ville est chargée au bon moment (nuit : pluie ; jour),
// la course repart du départ (même graine de trafic, banc_tel::GRAINE), caméra Arrière, pilote automatique (Game::piloteBanc) ;
// un seul interrupteur de WFS-419 allumé par le menu caché (les autres éteints) ; dureeChauffe s de chauffe, puis dureeMesure s
// de mesure image par image. La qualité automatique ne bouge pas pendant le banc (banc_tel::actif, qualite_systeme.cpp) : les
// essais se comparent à résolution égale. Quitter la course (pause, appel, application en arrière-plan) ou « Arrêter »
// interrompt le banc : les essais finis sont gardés et écrits.
// État du banc dans des variables du fichier : Game::init recrée les systèmes à chaque changement de carte (nuit → jour).
#include "reglages_banc.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include "reglages.h"
#include "../text.h"
#include "../petit_ecran.h"
#include "../../core/log.h"
#include "../../core/memoire.h"
#include "../../core/platform.h"
#include "../../core/system.h"
#include "../../game/camera_vue.h"
#include "../../game/course_system.h"
#include "../../game/game.h"
#include "../../game/reseau_relais.h"
#include "../../game/sauvegarde.h"
#include "../../game/traffic_system.h"
#include "../../platform/chauffe.h"
#include "../../render/banc_telephone.h"
#include "../../render/diagnostic_perf.h"
#include "../../render/memoire_gpu.h"
#include "../../render/passes.h"
#include "../../render/qualite_auto.h"
#include "../../render/renderer.h"

namespace wfs::ecrans::banc_telephone {
namespace {

static_assert(banc_tel::N_INTERRUPTEURS == diag::N_INTERRUPTEURS, "un essai par interrupteur de WFS-419");

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SURFACE2 = 0x1E2130;   // jetons des Réglages
struct Zone {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};
// bouton du style des Réglages (biseau, 44 dp de haut au moins) ; plein = orange (action principale)
Zone bouton(Renderer& r, float x, float y, const std::string& texte, float k, bool plein) {
    const float h = 44 * k, tp = 14 * k; const std::string t = ui::majuscules(texte);
    const float w = ui::width(tp, t, 0.1f) + 32 * k;
    dessin2d::biseau(r, x, y, w, h, 10 * k, dessin2d::uni(plein ? hex(ACCENT) : hex(SURFACE2)));
    ui::text(r, x + w / 2, y + (h - tp) / 2, tp, t, plein ? hex(BG) : hex(TEXTE), ui::CENTRE, 0.1f, false);
    return {x, y, w, h};
}
float largeurBouton(const std::string& texte, float k) { return ui::width(14 * k, ui::majuscules(texte), 0.1f) + 32 * k; }
// ligne réduite si elle ne tient pas dans la largeur l
void ligne(Renderer& r, float x, float& y, float l, float taille, const std::string& t, const vec4& c, float pas) {
    const float tp = std::min(taille, taille * l / std::max(ui::width(taille, t, 0.f), 1.f));
    ui::text(r, x, y, tp, t, c, ui::GAUCHE, 0.f, false); y += pas;
}

enum class Etat { Inactif, Chargement, Chauffe, Mesure };
Etat g_etat = Etat::Inactif;
bool g_protocole = false, g_resultats = false;
std::vector<banc_tel::Essai> g_deroule;
size_t g_index = 0;
double g_tPhase = 0, g_tImage = -1;
std::vector<banc_tel::Resultat> g_res;
banc_tel::Entete g_entete;
std::string g_texte, g_cheminFichier;
// accumulateurs de la mesure (tampon réservé au départ : aucune allocation par image)
std::vector<float> g_ms;
double g_cpuLogique = 0, g_cpuRendu = 0, g_attente = 0, g_envoi = 0, g_triangles = 0, g_appels = 0, g_materiaux = 0;
int g_images = 0, g_chauffeDebut = -1; double g_tempDebut = -1000;
// à rétablir à la fin
VueCamera g_vueAvant = VueCamera::Poursuite; std::string g_carteAvant, g_momentAvant;
bool g_arreter = false;
Zone g_commencer, g_annuler, g_partager, g_fermer, g_arret;

bool nuitChargee() { const char* m = std::getenv("WFS_MOMENT"); return m && std::strcmp(m, "nuit") == 0; }
const char* const* libelles() {
    static const char* l[diag::N_INTERRUPTEURS];
    for (int i = 0; i < diag::N_INTERRUPTEURS; i++) l[i] = diag::INFOS[i].libelle;
    return l;
}
std::string interrupteursJson() {   // allumés par configgame.json : le banc ne peut pas les éteindre
    std::string s;
    for (int i = 0; i < diag::N_INTERRUPTEURS; i++)
        if (diag::actif((diag::Interrupteur)i) && !diag::menu((diag::Interrupteur)i)) s += (s.empty() ? "" : ", ") + std::string(diag::INFOS[i].libelle);
    return s;
}
std::string dateLocale(bool pourFichier) {
    const std::time_t t = std::time(nullptr); std::tm l{};
#if defined(_WIN32)
    localtime_s(&l, &t);
#else
    localtime_r(&t, &l);
#endif
    if (pourFichier) return banc_tel::nomFichier(l.tm_year + 1900, l.tm_mon + 1, l.tm_mday, l.tm_hour, l.tm_min);
    char b[64]; std::snprintf(b, sizeof b, "%04d-%02d-%02d %02d:%02d:%02d", l.tm_year + 1900, l.tm_mon + 1, l.tm_mday, l.tm_hour, l.tm_min, l.tm_sec);
    return b;
}
const char* PROTOCOLE = "Ville de nuit (pluie) puis de jour ; caméra Arrière ; trafic de graine fixe ; pilote automatique ; par moment : tout éteint, "
                        "puis un seul interrupteur de WFS-419 à la fois, puis tout éteint (contrôle de la chauffe) ; 30 s de chauffe et 60 s de mesure par essai";

// interrupteurs du menu caché : seulement celui de l'essai (−1 : aucun) ; cibles recréées si MSAA ou résolution changent
void appliquerInterrupteur(Renderer* r, int i) {
    bool recreer = false;   // un interrupteur qui recrée les cibles change d'état
    for (int k = 0; k < diag::N_INTERRUPTEURS; k++) if (diag::INFOS[k].recreeCibles && diag::menu((diag::Interrupteur)k) != (k == i)) recreer = true;
    diag::eteindreMenu();
    if (i >= 0) diag::basculerMenu((diag::Interrupteur)i);
    if (r && recreer) r->resize();   // hors de l'image (mise à jour, changement d'écran), comme le menu caché de WFS-419
}

void viderMesure() {
    g_ms.clear(); g_cpuLogique = g_cpuRendu = g_attente = g_envoi = g_triangles = g_appels = g_materiaux = 0; g_images = 0; g_tImage = -1;
}

void ecrireFichier() {
    g_texte = banc_tel::texteFichier(g_entete, g_res);
    const std::string dossier = !dossierBanc.empty() ? dossierBanc : qualite::dossierMesure;
    g_cheminFichier.clear();
    if (dossier.empty()) { WFS_LOG("banc du téléphone (WFS-420) : aucun dossier d'écriture, résultats gardés pour « Partager » seulement"); return; }
    const std::string chemin = dossier + "/" + dateLocale(true);
    if (FILE* f = std::fopen(chemin.c_str(), "w")) {
        std::fputs(g_texte.c_str(), f); std::fclose(f); g_cheminFichier = chemin;
        WFS_LOG("banc du téléphone (WFS-420) : résultats écrits dans %s", chemin.c_str());
    } else WFS_ERR("banc du téléphone (WFS-420) : écriture impossible de %s", chemin.c_str());
}

// essai fini : moyennes de la fenêtre de mesure
void finirEssai(Renderer& r, bool interrompu) {
    banc_tel::Resultat res;
    res.essai = g_deroule[g_index]; res.nom = banc_tel::nomEssai(res.essai, libelles()); res.interrompu = interrompu;
    std::vector<float> ms = g_ms;   // copie triée (fin de l'essai seulement)
    res.images = banc_tel::stats(ms);
    const Renderer::GpuCumul& c = r.gpuCumul;
    res.gpuMesure = passes::horodatagesDisponibles() && c.images > 0;
    if (res.gpuMesure) {
        res.gpuImages = (int)c.images;
        res.gpuOmbre = c.ombre / c.images; res.gpuReflet = c.reflet / c.images; res.gpuScene = c.scene / c.images;
        res.gpuPost = c.post / c.images; res.gpuTotal = c.total / c.images;
        res.passesEnfichables = passes::texteCouts(passes::coutsCumules());
    }
    if (g_images > 0) {
        const double n = g_images;
        res.cpuLogique = g_cpuLogique / n; res.cpuAttente = g_attente / n; res.cpuEnvoi = g_envoi / n;
        res.cpuPreparation = std::max(0.0, (g_cpuRendu - g_attente - g_envoi) / n);
        res.triangles = g_triangles / n; res.appels = g_appels / n; res.materiaux = g_materiaux / n;
    }
    res.memoireGpuMo = memoire::gpuMo(); res.memoireParType = memoire_gpu::texte();
    res.chauffeDebut = g_chauffeDebut; res.chauffeFin = chauffe::statut();
    res.temperatureDebut = g_tempDebut; res.temperatureFin = banc_tel::temperatureBatterie();
    res.rendu = qualite::ligneRendu();
    WFS_LOG("banc du téléphone (WFS-420) : essai %zu/%zu %s, %s : %s", g_index + 1, g_deroule.size(), res.essai.nuit ? "nuit" : "jour", res.nom.c_str(),
            res.images.images ? (std::to_string(res.images.images) + " images, médiane " + banc_tel::virgule(res.images.mediane) + " ms, 95 % " +
                                 banc_tel::virgule(res.images.p95) + " ms, GPU " + (res.gpuMesure ? banc_tel::virgule(res.gpuTotal, 2) : std::string("—")) + " ms").c_str() : "aucune image");
    g_res.push_back(std::move(res));
}

// fin du banc (fini, arrêté ou interrompu) : tout est rétabli, tableau des résultats par-dessus la pause
void finirBanc(Renderer* r, const GameView& g, const std::string& raison) {
    const bool interrompu = !raison.empty();
    if (interrompu && r && g_etat == Etat::Mesure && g_index < g_deroule.size()) finirEssai(*r, true);
    g_etat = Etat::Inactif;
    Game::piloteBanc = false; banc_tel::actif = false;
    if (r) r->compterMateriaux = false;
    appliquerInterrupteur(r, -1);
    choisirVue(g_vueAvant);
    if (sauvegarde().actif() && !g_carteAvant.empty()) { sauvegarde().map = g_carteAvant; sauvegarde().tod = g_momentAvant; sauvegarde().enregistrer(); }
    g_entete.interrompu = interrompu; g_entete.raison = raison;
    ecrireFichier();
    WFS_LOG("banc du téléphone (WFS-420) : %s, %zu essais mesurés sur %zu", interrompu ? ("interrompu (" + raison + ")").c_str() : "fini", g_res.size(), g_deroule.size());
    g_resultats = true;
    if (g.allerA) g.allerA(Ecran::Pause);
    ouvrirReglagesBanc(true);
}

void demarrerEssai(Renderer* r, const GameView& g) {
    const banc_tel::Essai& e = g_deroule[g_index];
    appliquerInterrupteur(r, e.interrupteur);
    choisirVue(VueCamera::Poursuite);   // RG-01 : caméra Arrière
    trafficRestartSeed(banc_tel::GRAINE, g.player ? g.player->position : 0);
    courseRejouer();
    Game::piloteBanc = true;
    g_chauffeDebut = chauffe::statut(); g_tempDebut = banc_tel::temperatureBatterie();
    g_tPhase = nowSeconds(); g_etat = Etat::Chauffe;
    WFS_LOG("banc du téléphone (WFS-420) : essai %zu/%zu, %s, %s : chauffe %.0f s", g_index + 1, g_deroule.size(), e.nuit ? "nuit" : "jour",
            banc_tel::nomEssai(e, libelles()).c_str(), banc_tel::dureeChauffe);
}

class BancTelephone : public System {
public:
    const char* name() const override { return "banc-telephone"; }
    void init(Renderer& r, const GameView&) override { r_ = &r; }
    void shutdown(Renderer&) override { r_ = nullptr; }

    void update(double, const GameView& g) override {
        if (g_etat == Etat::Inactif) return;
        if (g_arreter) { g_arreter = false; finirBanc(r_, g, "arrêté avec « Arrêter »"); return; }
        if (Course* c = activeCourseMut(); c && c->timeLeft < 300) c->timeLeft = 600;   // course sans fin de chrono pendant le banc (chrono rechargé à 10 min)
        const double t = nowSeconds();
        if (g_etat == Etat::Chargement) {
            const bool nuit = g_deroule[g_index].nuit;
            if (!Game::demandes().carte.empty()) return;   // chargement demandé, pas encore fait
            if (!g.map || std::strcmp(g.map->id, "ville") != 0 || nuitChargee() != nuit) {
                Game::demandes().carte = "ville"; Game::demandes().nuit = nuit;
                WFS_LOG("banc du téléphone (WFS-420) : chargement de la Ville de %s", nuit ? "nuit" : "jour");
                return;
            }
            if (g.ecran != Ecran::Course) { if (g.allerA) g.allerA(Ecran::Course); return; }
            demarrerEssai(r_, g);
        } else if (g_etat == Etat::Chauffe && t - g_tPhase >= banc_tel::dureeChauffe) {
            viderMesure(); passes::remettreAZero();
            if (r_) { r_->gpuCumul = {}; r_->compterMateriaux = true; }
            g_tPhase = t; g_etat = Etat::Mesure;
        } else if (g_etat == Etat::Mesure && t - g_tPhase >= banc_tel::dureeMesure && r_) {
            r_->compterMateriaux = false;
            finirEssai(*r_, false);
            if (++g_index >= g_deroule.size()) { finirBanc(r_, g, ""); return; }
            g_etat = Etat::Chargement;   // même moment : l'essai suivant démarre à la mise à jour suivante ; sinon la Ville est rechargée
        }
    }

    // RG-03 : une mesure par image affichée (durée réelle entre deux images), seulement pendant la mesure (RG-06)
    void render(Renderer& r, const GameView&) override {
        if (g_etat != Etat::Mesure) return;
        const double t = nowSeconds();
        const double dt = g_tImage < 0 ? 0 : t - g_tImage;
        g_tImage = t;
        if (dt <= 0 || dt > 1) return;
        if (g_ms.size() < g_ms.capacity()) g_ms.push_back((float)(dt * 1000));
        g_cpuLogique += Game::msMiseAJour; g_cpuRendu += Game::msRendu;
        g_attente += r.chronoImage().attenteGpuMs; g_envoi += r.chronoImage().finImageMs;
        g_triangles += r.chronoImage().triangles; g_appels += r.appelsDessin(); g_materiaux += r.materiauxImage();
        g_images++;
    }

    // quitter la course (pause, appel, jeu en arrière-plan) interrompt le banc ; appelé entre deux images (cibles recréables)
    void onEcran(Ecran, Ecran n, const GameView& g) override {
        if ((g_etat == Etat::Chauffe || g_etat == Etat::Mesure) && n != Ecran::Course)
            finirBanc(r_, g, std::string("écran ") + nomEcran(n) + " pendant l'essai (pause, appel ou jeu en arrière-plan)");
        // les Réglages (460) se ferment à chaque changement d'écran : le panneau des résultats est rouvert après eux (465)
        if (g_resultats && n == Ecran::Pause) ouvrirReglagesBanc(true);
    }

    // bandeau du banc en course : essai, moment, interrupteur, temps restant, « Arrêter »
    void drawUi(Renderer& r, const GameView& g) override {
        if (g_etat == Etat::Inactif || g.ecran != Ecran::Course || !ui::fontReady()) return;
        const float W = g.uiWidth, H = g.uiHeight, k = ui::unitePetitEcran(W, H);
        const banc_tel::Essai& e = g_deroule[std::min(g_index, g_deroule.size() - 1)];
        const double reste = g_etat == Etat::Chauffe ? banc_tel::dureeChauffe - (nowSeconds() - g_tPhase) : g_etat == Etat::Mesure ? banc_tel::dureeMesure - (nowSeconds() - g_tPhase) : 0;
        const std::string l1 = "BANC AUTO · ESSAI " + std::to_string(g_index + 1) + "/" + std::to_string(g_deroule.size()) + " · " + (e.nuit ? "NUIT" : "JOUR");
        const std::string l2 = banc_tel::nomEssai(e, libelles()) + " · " + (g_etat == Etat::Chauffe ? "chauffe " : g_etat == Etat::Mesure ? "mesure " : "chargement") +
                               (g_etat == Etat::Chargement ? std::string() : std::to_string((int)std::max(0.0, reste + 0.999)) + " s");
        const float tp1 = 13 * k, tp2 = 12 * k, pad = 10 * k;
        const float wt = std::max(ui::width(tp1, l1, 0.08f), ui::width(tp2, l2, 0.f)), wb = largeurBouton("Arrêter", k);
        const float h = 44 * k + 2 * pad, w = wt + wb + 3 * pad, x = g.marges.gauche + 16 * k, y = H - g.marges.bas - h - 12 * k - 150 * k;
        r.uiRect(x, y, w, h, hex(BG, 0.82f));
        r.uiRect(x, y, 3 * k, h, hex(ACCENT2));
        ui::text(r, x + pad + 3 * k, y + pad + 2 * k, tp1, l1, hex(ACCENT2), ui::GAUCHE, 0.08f, false);
        ui::text(r, x + pad + 3 * k, y + pad + 24 * k, tp2, l2, hex(TEXTE), ui::GAUCHE, 0.f, false);
        g_arret = bouton(r, x + w - pad - wb, y + pad, "Arrêter", k, false);
    }
    bool onTouch(const Touch& t, const GameView& g) override {
        if (g_etat == Etat::Inactif || g.ecran != Ecran::Course || !t.down || !g_arret.contient(t.x, t.y)) return false;
        g_arreter = true;
        return true;
    }

private:
    Renderer* r_ = nullptr;
};
WFS_SYSTEM(BancTelephone, 465)   // interface (400-499) : par-dessus le HUD (420) ; état du banc dans les variables du fichier

} // namespace

void ouvrirProtocole() { g_protocole = true; g_resultats = false; WFS_LOG("banc du téléphone (WFS-420) : protocole affiché"); }
bool protocoleOuvert() { return g_protocole; }
bool resultatsOuverts() { return g_resultats; }
bool enCours() { return g_etat != Etat::Inactif; }

void commencer() {
    g_protocole = false; g_resultats = false; g_arreter = false;
    g_deroule = banc_tel::deroule(); g_index = 0; g_res.clear();
    g_ms.clear(); g_ms.reserve((size_t)(std::max(banc_tel::dureeMesure, 1.0) * 150));   // 150 img/s au plus
    g_entete = {};
    g_entete.date = dateLocale(false); g_entete.protocole = PROTOCOLE; g_entete.interrupteursJson = interrupteursJson();
    g_entete.batterie = banc_tel::batteriePourcent(); g_entete.enCharge = banc_tel::batterieEnCharge();
    g_vueAvant = vueCamera();
    g_carteAvant = sauvegarde().actif() ? sauvegarde().map : ""; g_momentAvant = sauvegarde().actif() ? sauvegarde().tod : "";
    banc_tel::actif = true;
    g_etat = Etat::Chargement;
    // demande lue au début de Game::update, même en pause (simulation figée) : la Ville de nuit est chargée, ou la course reprend
    Game::demandes().carte = "ville"; Game::demandes().nuit = g_deroule[0].nuit;
    ouvrirReglagesBanc(false);
    WFS_LOG("banc du téléphone (WFS-420) : départ, %zu essais (%.0f s de chauffe + %.0f s de mesure chacun), batterie %d %%, en charge %d",
            g_deroule.size(), banc_tel::dureeChauffe, banc_tel::dureeMesure, g_entete.batterie, g_entete.enCharge);
}

// RG-05 : protocole affiché avant le banc, avec les relevés du téléphone quand il les donne
void dessinerProtocole(Renderer& r, float W, float H, float k) {
    r.uiRect(0, 0, W, H, hex(BG, 0.94f));
    const float m = 16 * k, x0 = m, lw = W - 2 * m;
    float y = m;
    ui::text(r, x0, y, 18 * k, ui::majuscules("Banc de performance automatique (WFS-420)"), hex(ACCENT2), ui::GAUCHE, 0.08f, false);
    y += 30 * k;
    ligne(r, x0, y, lw, 13 * k, "Avant de commencer (protocole) :", hex(TEXTE), 20 * k);
    const int bat = banc_tel::batteriePourcent(), ch = banc_tel::batterieEnCharge();
    const double temp = banc_tel::temperatureBatterie();
    auto point = [&](const std::string& t, const std::string& releve, bool ok) {
        ligne(r, x0 + 12 * k, y, lw - 12 * k, 12 * k, "• " + t + (releve.empty() ? "" : "  —  " + releve), ok ? hex(TEXTE) : hex(ACCENT), 17 * k);
    };
    point("Batterie à plus de 50 %", bat >= 0 ? "relevé : " + std::to_string(bat) + " %" : "relevé non disponible", bat < 0 || bat > 50);
    point("Téléphone pas en charge (câble débranché)", ch == 1 ? "relevé : EN CHARGE" : ch == 0 ? "relevé : pas en charge" : "relevé non disponible", ch != 1);
    point("Mode avion activé", "", true);
    point("Luminosité fixe (luminosité automatique coupée)", "", true);
    point("Téléphone froid au départ (posé 10 minutes, hors charge)", temp > -999 ? "batterie à " + banc_tel::virgule(temp) + " °C" : "", true);
    y += 6 * k;
    ligne(r, x0, y, lw, 12 * k, "Déroulé : Ville de nuit (pluie), puis de jour ; caméra Arrière ; trafic toujours le même ; la voiture roule seule.", hex(MUTED), 16 * k);
    ligne(r, x0, y, lw, 12 * k, "Par moment : tout éteint, puis un seul interrupteur à la fois (12), puis tout éteint (contrôle de la chauffe).", hex(MUTED), 16 * k);
    {
        const double minutes = banc_tel::deroule().size() * (banc_tel::dureeChauffe + banc_tel::dureeMesure) / 60.0;
        ligne(r, x0, y, lw, 12 * k, "Chaque essai : " + banc_tel::virgule(banc_tel::dureeChauffe, 0) + " s de chauffe puis " + banc_tel::virgule(banc_tel::dureeMesure, 0) +
                                    " s de mesure ; 28 essais, environ " + banc_tel::virgule(minutes, 0) + " minutes. Ne touche pas l'écran ; « Arrêter » garde les essais finis.", hex(MUTED), 16 * k);
    }
    ligne(r, x0, y, lw, 12 * k, "Résultats : tableau à l'écran et fichier diagnostic-AAAA-MM-JJ-hhmm.txt, avec un bouton « Partager ».", hex(MUTED), 16 * k);
    const std::string js = interrupteursJson();
    if (!js.empty()) ligne(r, x0, y, lw, 12 * k, "Attention : interrupteurs allumés dans configgame.json (le banc ne peut pas les éteindre) : " + js, hex(ACCENT), 16 * k);
    const float yb = H - m - 44 * k;
    g_commencer = bouton(r, x0, yb, "Commencer", k, true);
    g_annuler = bouton(r, g_commencer.x + g_commencer.w + 12 * k, yb, "Annuler", k, false);
}

bool toucherProtocole(float x, float y) {
    if (g_commencer.contient(x, y)) commencer();
    else if (g_annuler.contient(x, y)) { g_protocole = false; WFS_LOG("banc du téléphone (WFS-420) : protocole annulé"); }
    return true;
}

// RG-04 : tableau des résultats (nuit à gauche, jour à droite), remarques, fichier, « Partager » et « Fermer »
void dessinerResultats(Renderer& r, float W, float H, float k) {
    r.uiRect(0, 0, W, H, hex(BG, 0.96f));
    const float m = 12 * k, lw = W - 2 * m;
    float y = m;
    ui::text(r, m, y, 16 * k, ui::majuscules(g_entete.interrompu ? "Banc auto : interrompu (résultats partiels)" : "Banc auto : résultats"), hex(ACCENT2), ui::GAUCHE, 0.08f, false);
    {   // boutons en haut à droite
        const float wf = largeurBouton("Fermer", k), wp = largeurBouton("Partager", k);
        g_fermer = bouton(r, W - m - wf, y - 6 * k, "Fermer", k, true);
        g_partager = bouton(r, W - m - wf - 8 * k - wp, y - 6 * k, "Partager", k, false);
    }
    y += 26 * k;
    if (g_entete.interrompu) ligne(r, m, y, lw - 200 * k, 11 * k, g_entete.raison, hex(ACCENT), 15 * k);
    ligne(r, m, y, lw - 200 * k, 11 * k, "ms par image : médiane · 95 % · 99 % · img/s · GPU total · écart à la référence (+ = plus lent)", hex(MUTED), 16 * k);
    const float colW = (lw - 12 * k) / 2, rowH = 15.5f * k, tp = 10.5f * k;
    for (int moment = 0; moment < 2; moment++) {
        const float cx = m + moment * (colW + 12 * k);
        float yc = y;
        ui::text(r, cx, yc, 12 * k, moment == 0 ? "NUIT (pluie)" : "JOUR", hex(ACCENT2), ui::GAUCHE, 0.06f, false);
        yc += 17 * k;
        int rang = 0;
        for (size_t i = 0; i < g_res.size(); i++) {
            if (g_res[i].essai.nuit != (moment == 0)) continue;
            if (rang++ % 2 == 0) r.uiRect(cx, yc - 2 * k, colW, rowH, hex(SURFACE2, 0.7f));
            const bool ref = g_res[i].essai.interrupteur < 0;
            const float wn = colW * 0.42f;
            const float tn = std::min(tp, tp * (wn - 6 * k) / std::max(ui::width(tp, g_res[i].nom, 0.f), 1.f));
            ui::text(r, cx + 4 * k, yc, tn, g_res[i].nom, ref ? hex(ACCENT2) : hex(TEXTE), ui::GAUCHE, 0.f, false);
            const std::string v = banc_tel::ligneTableau(g_res, i);
            const float tv = std::min(tp, tp * (colW - wn - 6 * k) / std::max(ui::width(tp, v, 0.f), 1.f));
            ui::text(r, cx + wn, yc, tv, v, hex(TEXTE), ui::GAUCHE, 0.f, false);
            yc += rowH;
        }
        if (!rang) ui::text(r, cx + 4 * k, yc, tp, "aucun essai mesuré", hex(MUTED), ui::GAUCHE, 0.f, false);
    }
    y += 17 * k + 14 * rowH + 6 * k;
    for (const std::string& rq : banc_tel::remarques(g_res)) { if (y > H - 2 * rowH) break; ligne(r, m, y, lw, 10 * k, rq, hex(ACCENT), 13 * k); }
    ligne(r, m, y, lw, 10 * k, g_cheminFichier.empty() ? "Fichier : non écrit (pas de dossier) ; « Partager » envoie le texte complet." : "Fichier : " + g_cheminFichier, hex(MUTED), 13 * k);
}

bool toucherResultats(float x, float y) {
    if (g_fermer.contient(x, y)) { g_resultats = false; WFS_LOG("banc du téléphone (WFS-420) : résultats fermés"); }
    else if (g_partager.contient(x, y) && !g_texte.empty()) { WFS_LOG("banc du téléphone (WFS-420) : résultats partagés"); reseau::partager(g_texte); }
    return true;
}

// ---------------------------------------------------------------- banc PC (captures de la PR)
int benchBancTelephone(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    {   // durées courtes : « chauffe,mesure » en secondes réelles (le rendu logiciel est lent)
        const char* d = std::getenv("WFS_BANC_TEL_DUREES");
        banc_tel::dureeChauffe = 0.6; banc_tel::dureeMesure = 1.2;
        if (d) std::sscanf(d, "%lf,%lf", &banc_tel::dureeChauffe, &banc_tel::dureeMesure);
    }
    dossierBanc = out;
    setenv("WFS_MOMENT", "jour", 1);   // départ sur la Ville de jour : le banc recharge la Ville de nuit (vrai chemin du téléphone)
    if (!g.init(r, "golf", "ville")) { WFS_ERR("banc du téléphone : chargement impossible"); return 1; }
    double t = 7000;
    auto image = [&](bool dessiner = true) { g.update(1.0 / 60); if (dessiner) g.render(t); t += 1.0 / 60; };
    auto capture = [&](const char* nom) { g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png"); WFS_LOG("capture %s", nom); };
    g.allerA(Ecran::Course); for (int i = 0; i < 30; i++) image(i % 10 == 9);
    g.allerA(Ecran::Pause); image();
    ouvrirReglagesBanc(true); ouvrirMenuCacheBanc(true); image();
    capture("banc-tel-1-menu-cache");
    ouvrirMenuCacheBanc(false); ouvrirProtocole(); image();
    capture("banc-tel-2-protocole");
    commencer();
    bool vuNuit = false, vuJour = false;
    for (int i = 0; i < 20000 && enCours(); i++) {
        image();
        const bool mesure = g_etat == Etat::Mesure;
        if (mesure && !vuNuit && g_index == 4) { capture("banc-tel-3-nuit-en-cours"); vuNuit = true; }
        if (mesure && !vuJour && g_index == 14 + 9) { capture("banc-tel-4-jour-en-cours"); vuJour = true; }
    }
    if (enCours()) { WFS_ERR("banc du téléphone : pas fini après 20 000 images"); ecarts++; }
    for (int i = 0; i < 3; i++) image();
    capture("banc-tel-5-resultats");
    WFS_LOG("banc du téléphone : %zu essais, fichier %s, écran %s", g_res.size(), g_cheminFichier.c_str(), nomEcran(g.ecran()));
    if (g_res.size() != banc_tel::deroule().size()) { WFS_ERR("banc du téléphone : %zu essais au lieu de %zu", g_res.size(), banc_tel::deroule().size()); ecarts++; }
    for (const banc_tel::Resultat& rr : g_res) if (!rr.images.images) { WFS_ERR("banc du téléphone : essai sans image : %s", rr.nom.c_str()); ecarts++; }
    if (g_cheminFichier.empty()) { WFS_ERR("banc du téléphone : fichier non écrit"); ecarts++; }
    if (!diag::aucun()) { WFS_ERR("banc du téléphone : interrupteurs encore allumés après le banc"); ecarts++; }
    if (Game::piloteBanc || banc_tel::actif) { WFS_ERR("banc du téléphone : pilote ou banc encore actif"); ecarts++; }
    {   // l'arrêt garde les essais finis (« Arrêter »)
        g_resultats = false; ouvrirReglagesBanc(false);
        banc_tel::dureeChauffe = 0.3; banc_tel::dureeMesure = 0.6;
        commencer();
        for (int i = 0; i < 20000 && enCours() && g_res.size() < 2; i++) image();
        g_arreter = true; image(); image();
        capture("banc-tel-6-arrete");
        if (enCours() || !g_entete.interrompu || g_res.size() < 2) { WFS_ERR("banc du téléphone : « Arrêter » mal pris en compte"); ecarts++; }
        g_resultats = false; ouvrirReglagesBanc(false);
    }
    WFS_LOG("banc du téléphone (WFS-420) : %d écart(s)", ecarts);
    for (size_t p = 0; p < g_texte.size();) { const size_t f = g_texte.find('\n', p); WFS_LOG("fichier | %s", g_texte.substr(p, f - p).c_str()); if (f == std::string::npos) break; p = f + 1; }
    dossierBanc.clear(); banc_tel::dureeChauffe = banc_tel::CHAUFFE_S; banc_tel::dureeMesure = banc_tel::MESURE_S;
    g.shutdown();
    return ecarts;
}

} // namespace wfs::ecrans::banc_telephone
