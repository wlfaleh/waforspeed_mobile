// WaForSpeed — WFS-416 : banc « batiments » (WFS_BANC=batiments) : bâtiments du kit cuits dans le paquet de la Ville.
// Le banc cuit le paquet (comme la CI : cartes_v2::cuire), puis joue les mêmes scènes deux fois :
//   « secours » : sans paquet, le kit construit par le code (état de main) ;
//   « paquet »  : bloc « batiments » lu au chargement, plus aucune construction en course.
// Captures (règles de gestion de WFS-416) : rue d'immeubles de nuit et de jour en qualité Basse avec la ligne de mesure,
// façade de près (menuiserie 3D) et à 50 m (image avec relief), suite d'images à 160 km/h avec le temps de chaque image.
// Contrôles (écarts) : avec le paquet, aucune construction (archi::constructions) et aucune création de maillage
// (Renderer::creationsMaillages) pendant la course ; sans paquet et avec un bloc refusé, le kit de secours joue sans plantage.
// WFS-426 : WFS_CARTE=plage → même banc sur la Corniche (villas méditerranéennes, captures batiments-plage-*) ; sans : la Ville.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include "paquet_carte.h"
#include "../archi_carte.h"
#include "../ville_quartiers.h"
#include "../plage_villas.h"   // WFS-426 : banc de la Corniche (WFS_CARTE=plage)
#include "../../core/log.h"
#include "../../core/memoire.h"
#include "../../core/platform.h"
#include "../../core/system.h"
#include "../../game/camera_vue.h"
#include "../../game/course_system.h"
#include "../../game/game.h"
#include "../../game/trace_cartes.h"
#include "../../game/track.h"
#include "../../render/environnement.h"
#include "../../render/renderer.h"
#include "../../ui/ecrans/reglages.h"
#include "../../ui/text.h"

namespace wfs {
namespace cartes_v2 {
namespace {
namespace fs = std::filesystem;

std::string g_ligne[2];   // lignes de mesure dessinées sur les captures du banc (vides hors du banc)
struct CameraBanc { bool active = false; vec3 oeil{}, oeilB{}, cible{}; bool auPlusPres = false; float champ = 0; } g_camera;   // caméra posée devant une façade (banc seulement)
class LigneBanc : public System {
public:
    const char* name() const override { return "batiments-banc"; }
    void ambiance(FrameParams& fp, const GameView&) override {
        if (!g_camera.active) return;
        // auPlusPres : des deux points de vue possibles (d'un bout ou de l'autre de la rue), celui du côté de la voiture
        const vec3 oeil = g_camera.auPlusPres && length(g_camera.oeilB - fp.camPos) < length(g_camera.oeil - fp.camPos) ? g_camera.oeilB : g_camera.oeil;
        fp.view = lookAt(oeil, g_camera.cible, {0, 1, 0}); fp.camPos = oeil;
        if (g_camera.champ > 0) fp.proj = perspectiveVk(g_camera.champ * PI / 180.f, std::fabs(fp.proj.at(1, 1) / fp.proj.at(0, 0)), 0.1f);   // même rapport d'image
    }
    void drawUi(Renderer& r, const GameView& g) override {
        for (int k = 0; k < 2; k++) if (!g_ligne[k].empty()) ui::text(r, g.marges.gauche + 24, g.marges.haut + 18 + 34.f * k, 24, g_ligne[k], k ? ui::CYAN : ui::BLANC);
    }
};

bool ecrireFichier(const std::string& chemin, const std::vector<uint8_t>& o) {
    const std::string tmp = chemin + ".tmp";
    { std::ofstream f(tmp, std::ios::binary); if (!f) return false; f.write((const char*)o.data(), (std::streamsize)o.size()); if (!f) return false; }
    std::error_code ec; fs::rename(tmp, chemin, ec);
    return !ec;
}

struct Mesure { double mediane = 0, p99 = 0, pire = 0, chargementMs = 0, gpuMo = 0, kmh = 0, batMs = 0, batPire = 0; size_t constructions = 0; uint64_t creations = 0, recopies = 0; bool ok = false; };

}  // namespace

int benchBatiments(Game& g, Renderer& r, const std::string& out, const std::string& racine) {
    int ecarts = 0;
    auto verifier = [&](bool ok, const std::string& quoi) { WFS_LOG("batiments (WFS-416) banc : %s : %s", quoi.c_str(), ok ? "OK" : "ÉCART"); if (!ok) ecarts++; };
    const char* ce = std::getenv("WFS_CARTE");   // WFS-426 : carte du banc (cartes cuites seulement), la Ville par défaut
    const std::string c = ce && cuiteEnPaquet(ce) ? ce : "ville", dossier = carte::dossierPaquets();
    const bool corniche = c == "plage";
    const std::string nomCarte = mapById(c.c_str()).name, prefixe = corniche ? "batiments-plage" : "batiments";
    const archi::Style styleFacade = corniche ? archi::Style::Mediterraneen : archi::Style::Classique;
    const carte::FormatTexture F = carte::FormatTexture::RGBA8;
    const std::string chemin = dossier + "/" + carte::nomFichier(c, F);
    std::error_code ec;
    fs::create_directories(dossier, ec);
    for (carte::FormatTexture f : carte::FORMATS) fs::remove(dossier + "/" + carte::nomFichier(c, f), ec);

    // rue la plus bâtie de la Ville : deux morceaux de suite avec le plus d'immeubles, sur une partie droite
    int64_t depart = 0;
    {
        Track t; t.build(mapById(c.c_str()), trace::versionCarte(c.c_str()));
        if (corniche) plage::poserVillasKit(t); else ville::poserImmeublesKit(mapById(c.c_str()), t);
        std::map<int64_t, int> n;
        for (const archi::cuit::Bati& b : archi::carte::poses()) n[b.seg / archi::cuit::MORCEAU]++;
        archi::carte::vider();
        int meilleur = -1;
        for (const auto& [m, k] : n) {
            double courbe = 0;
            for (int64_t a = (m - 1) * archi::cuit::MORCEAU; a < (m + 3) * archi::cuit::MORCEAU; a++) if (a >= 0 && a < (int64_t)t.segmentCount()) courbe = std::max(courbe, (double)std::fabs(t.segAt(a).curve));
            const int total = k + (n.count(m + 1) ? n[m + 1] : 0) + (n.count(m + 2) ? n[m + 2] : 0);
            if (m >= 2 && courbe < 0.5 && total > meilleur) { meilleur = total; depart = m * archi::cuit::MORCEAU; }
        }
        WFS_LOG("batiments (WFS-416) banc : rue d'essai au segment %lld (%d immeubles sur 3 morceaux)", (long long)depart, meilleur);
    }

    // 1) cuisson (RG-02) : deux cuissons identiques à l'octet ; taille du paquet (RG-08)
    const double tc0 = nowSeconds();
    const carte::Paquet cuit = cuire(racine, c, F);
    const std::vector<uint8_t> octets = carte::ecrire(cuit);
    const double tc = nowSeconds() - tc0;
    verifier(carte::ecrire(cuire(racine, c, F)) == octets, "deux cuissons identiques à l'octet (bloc des bâtiments compris)");
    const carte::Bloc* bloc = cuit.bloc(archi::cuit::NOM_BLOC);
    verifier(bloc && !bloc->donnees.empty(), "bloc « batiments » présent dans le paquet de la carte (" + nomCarte + ")");
    WFS_LOG("batiments (WFS-416) mesure RG-08 : %s : %.1f Mo, dont bloc des bâtiments %.1f Mo ; cuit en %d ms",
            carte::nomFichier(c, F).c_str(), octets.size() / 1048576.0, bloc ? bloc->donnees.size() / 1048576.0 : 0.0, (int)std::lround(tc * 1000));

    ecrans::Reglages& R = ecrans::reglages();
    const int q0 = R.qualite;
    ui::chargerPolice(r);
    courseBenchRules(false);

    // une scène : Ville, moment, qualité Basse, départ dans la rue d'essai ; conduite tenue à « kmh » ; captures et mesures
    //   vue : 0 poursuite en course ; 1 façade de près (caméra tournée vers l'immeuble, voiture à l'arrêt) ; 2 la même à 50 m
    auto scene = [&](const char* etat, const char* moment, int vue, double kmh, const std::string& nom, int images) {
        Mesure m;
        setenv("WFS_MOMENT", moment, 1);
        setenv("WFS_DEPART", std::to_string(vue == 2 ? depart - (int64_t)(50 / SEG_M) : depart).c_str(), 1);
        environnement::invalider();
        R.qualite = ecrans::QUALITE_BASSE; ecrans::appliquerQualiteNiveau(r);
        const double t0 = nowSeconds();
        const bool ok = g.init(r, "golf", c.c_str());
        m.chargementMs = (nowSeconds() - t0) * 1000;
        unsetenv("WFS_DEPART");
        if (!ok) { WFS_ERR("batiments (WFS-416) banc : chargement de %s impossible (%s)", nomCarte.c_str(), etat); ecarts++; return m; }
        g.allerA(Ecran::Course);
        g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{};
        choisirVue(VueCamera::Poursuite);
        choisirVue(vue == 0 ? VueCamera::Proche : VueCamera::Poursuite);
        if (vue != 0) {   // façade d'essai : l'immeuble classique le plus large du morceau de la rue d'essai, côté droit d'abord
            const archi::cuit::Bati* f = nullptr;
            for (const archi::cuit::Bati& b : archi::carte::poses())
                if (b.seg / archi::cuit::MORCEAU == depart / archi::cuit::MORCEAU && b.plan.style == styleFacade && (!f || b.plan.largeur > f->plan.largeur)) f = &b;
            if (f) {
                g_camera = CameraBanc{};
                g_camera.active = true;
                const float L = f->plan.largeur;
                if (vue == 1) {   // de près : caméra à 4 m de la façade (entre les arbres et le mur), à 5 m du sol, regard le long de la façade (6 à 12 m)
                    g_camera.oeil = f->o + f->x * (L / 2 - 6.f) + f->z * 4.f + vec3{0, 5.f, 0};
                    g_camera.cible = f->o + f->x * (L / 2 + 3.f) + vec3{0, 7.f, 0};
                } else {          // à 50 m : du bout de la rue où est la voiture, de biais, vue resserrée (champ de 22°)
                    g_camera.cible = f->o + f->x * (L / 2) + vec3{0, 8.f, 0};
                    g_camera.oeil = f->o + f->x * (L / 2 + 48.f) + f->z * 11.f + vec3{0, 3.5f, 0};
                    g_camera.oeilB = f->o + f->x * (L / 2 - 48.f) + f->z * 11.f + vec3{0, 3.5f, 0};
                    g_camera.auPlusPres = true; g_camera.champ = 22.f;
                }
            }
        }
        double t = 5000, braquage = 0;
        auto pas = [&](double dt) {
            const Player& pj = g.player();
            g.manette.gas = kmh > 0 && pj.kmh() < kmh; g.manette.brake = kmh <= 0 || pj.kmh() > kmh + 12;
            const double vise = std::clamp(-pj.playerX * 0.8 - pj.heading * 1.2, -0.6, 0.6);
            braquage += (vise - braquage) * (1 - std::exp(-dt / 0.3));
            g.manette.steer = (float)braquage;
            g.update(dt); t += dt;
        };
        // mise en vitesse : 6 s de conduite (une image dessinée sur 6 : la lumière du rendu se pose, les morceaux suivent)
        for (int i = 0; i < 60 * (vue == 0 ? 6 : 2); i++) { pas(1.0 / 60); if (i % 6 == 0) g.render(t); }
        for (int i = 0; i < 12; i++) { pas(1.0 / 60); g.render(t); }
        const size_t c0 = archi::constructions();
        const uint64_t m0 = r.creationsMaillages();
        archi::carte::chrono() = archi::carte::Chrono{};
        std::vector<double> ms;
        for (int i = 0; i < images; i++) {
            const double a = nowSeconds();
            pas(1.0 / 60); g.render(t);
            ms.push_back((nowSeconds() - a) * 1000);
            if (vue == 0 && images >= 120 && (i == images / 4 || i == images / 2 || i == 3 * images / 4)) {   // suite d'images : temps de chaque image
                char l[160]; std::snprintf(l, sizeof l, "image %d / %d : %.1f ms   %.0f km/h", i + 1, images, ms.back(), g.player().kmh());
                g_ligne[0] = nomCarte + ", " + moment + ", Basse, " + etat; g_ligne[1] = l;
                g.render(t); r.capturePng(out + "/" + nom + "-suite-" + std::to_string(i * 4 / images) + ".png");
                g_ligne[0].clear(); g_ligne[1].clear();
            }
        }
        m.constructions = archi::constructions() - c0; m.creations = r.creationsMaillages() - m0;
        m.kmh = g.player().kmh(); m.gpuMo = memoire::gpuMo();
        { const archi::carte::Chrono& k = archi::carte::chrono(); m.batMs = k.images ? k.cumulMs / (double)k.images : 0; m.batPire = k.pireMs; m.recopies = k.sommetsRecopies; }
        std::vector<double> tri = ms; std::sort(tri.begin(), tri.end());
        m.mediane = tri[tri.size() / 2]; m.p99 = tri[std::min(tri.size() - 1, (size_t)(tri.size() * 0.99))]; m.pire = tri.back();
        char l[200];
        std::snprintf(l, sizeof l, "%.1f img/s (médiane %.0f ms, pire %.0f ms)   bâtiments : %.2f ms par image, pire %.1f ms, %zu constructions", 1000 / m.mediane, m.mediane, m.pire, m.batMs, m.batPire, m.constructions);
        g_ligne[0] = nomCarte + ", " + moment + ", Basse, " + etat + (vue == 1 ? ", façade de près (6 à 12 m)" : vue == 2 ? ", façade à 50 m (vue resserrée)" : ""); g_ligne[1] = l;
        g.render(t); g.render(t); r.capturePng(out + "/" + nom + ".png");
        g_ligne[0].clear(); g_ligne[1].clear();
        WFS_LOG("batiments (WFS-416) mesure RG-07 : %s : %s %s, Basse, %s : %s ; système des bâtiments seul : %.3f ms par image en moyenne, pire image %.2f ms, %llu sommets recopiés ; créations de maillages (tout le jeu) %llu ; %.0f km/h ; chargement %d ms ; mémoire graphique %.0f Mo (banc PC, rendu logiciel)",
                nom.c_str(), nomCarte.c_str(), moment, etat, l, m.batMs, m.batPire, (unsigned long long)m.recopies, (unsigned long long)m.creations, m.kmh, (int)std::lround(m.chargementMs), m.gpuMo);
        g.shutdown();
        g_camera.active = false;
        m.ok = true;
        return m;
    };
    const bool rapide = std::getenv("WFS_BAT_RAPIDE") != nullptr;   // essai local : façades seulement
    auto serie = [&](const char* etat, const std::string& pre, Mesure* nuit) {
        scene(etat, "jour", 1, 0, pre + "-facade-pres", 6);
        scene(etat, "jour", 2, 0, pre + "-facade-50m", 6);
        scene(etat, "nuit", 1, 0, pre + "-facade-pres-nuit", 6);
        scene(etat, "nuit", 2, 0, pre + "-facade-50m-nuit", 6);
        if (rapide) { Mesure m = scene(etat, "jour", 0, 50, pre + "-rue-jour", 30); *nuit = m; return m; }
        *nuit = scene(etat, "nuit", 0, 50, pre + "-rue-nuit", 150);
        scene(etat, "jour", 0, 50, pre + "-rue-jour", 60);
        return scene(etat, "nuit", 0, 160, pre + "-160kmh", 240);
    };

    // 2) sans paquet : kit de secours (RG-06) = état de main
    verifier(!preparerCarte(c), "paquet absent → " + nomCarte + " du code");
    Mesure sn, s160 = serie("kit de secours (sans paquet)", prefixe + "-secours", &sn);
    verifier(s160.ok && (rapide || s160.constructions > 0), "sans paquet : le kit de secours construit les bâtiments en roulant (comme avant)");

    // 3) avec le paquet : lu au chargement, aucune construction en course (RG-03)
    verifier(ecrireFichier(chemin, octets), "écriture de " + chemin);
    verifier(preparerCarte(c) && carte::carteActive() == c, "paquet valide → activé");
    Mesure pn, p160 = serie("paquet cuit", prefixe + "-paquet", &pn);
    verifier(pn.ok && pn.constructions == 0, "paquet : aucune construction de bâtiment en course (rue, 50 km/h)");
    verifier(p160.ok && p160.constructions == 0, "paquet : aucune construction de bâtiment en course (160 km/h)");
    verifier(p160.creations <= s160.creations, "paquet : pas plus de maillages créés en course que le kit (les autres systèmes en créent aussi)");
    // 3 bis) repère : le kit avec les fenêtres réelles de WFS-402 construites en course (état d'avant WFS-415), même trajet
    Mesure w160;
    if (!rapide) {
        carte::desactiver();
        archi::choisirOuverturesReelles(true);
        w160 = scene("kit + fenêtres WFS-402 construites en course", "nuit", 0, 160, prefixe + "-kit402-160kmh", 240);
        archi::choisirOuverturesReelles(false);
        verifier(preparerCarte(c), "paquet réactivé");
    }
    WFS_LOG("batiments (WFS-416) VERDICT système des bâtiments seul, 160 km/h de nuit, Basse (ms par image : moyenne / pire) : kit actuel %.3f / %.2f (%zu constructions) ; kit + fenêtres WFS-402 %.3f / %.2f (%zu constructions) ; paquet cuit %.3f / %.2f (0 construction, %llu sommets recopiés)",
            s160.batMs, s160.batPire, s160.constructions, w160.batMs, w160.batPire, w160.constructions, p160.batMs, p160.batPire, (unsigned long long)p160.recopies);
    WFS_LOG("batiments (WFS-416) mesure RG-07 : 160 km/h de nuit, Basse : secours médiane %.1f ms, 99 %% %.1f, pire %.1f ; paquet médiane %.1f ms, 99 %% %.1f, pire %.1f ; chargement %+d ms ; mémoire graphique %+.0f Mo",
            s160.mediane, s160.p99, s160.pire, p160.mediane, p160.p99, p160.pire, (int)std::lround(p160.chargementMs - s160.chargementMs), p160.gpuMo - s160.gpuMo);
    verifier(p160.chargementMs <= s160.chargementMs + 3000, "chargement au plus 3 s plus long avec le paquet (RG-07)");

    // 4) bloc des bâtiments refusé (cuit pour un autre plan) : le paquet reste lu, le kit de secours reprend la main, sans plantage
    {
        carte::Paquet faux = cuit;
        for (carte::Bloc& b : faux.blocs) if (b.nom == archi::cuit::NOM_BLOC && b.donnees.size() > 16) b.donnees[8] ^= 0x5A;   // signature
        verifier(ecrireFichier(chemin, carte::ecrire(faux)) && preparerCarte(c), "paquet au bloc de bâtiments faux : activé (le reste est bon)");
        const Mesure f = scene("bloc refusé → kit de secours", "jour", 0, 50, prefixe + "-bloc-refuse", 60);
        verifier(f.ok, "bloc des bâtiments refusé → kit de secours, aucun plantage");
    }
    carte::desactiver();
    fs::remove(chemin, ec);   // le banc ne laisse aucun paquet
    R.qualite = q0; ecrans::appliquerQualiteNiveau(r);
    WFS_LOG("batiments (WFS-416) banc : %d écart(s)%s", ecarts, ecarts ? " ÉCHEC" : " OK");
    return ecarts;
}

}  // namespace cartes_v2
using LigneBancBatiments = cartes_v2::LigneBanc;
WFS_SYSTEM(LigneBancBatiments, 499)   // interface (400-499) : ne dessine que pendant le banc « batiments »
}  // namespace wfs
