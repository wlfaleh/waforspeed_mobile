// WaForSpeed — WFS-416 : banc « batiments » (WFS_BANC=batiments) : bâtiments du kit cuits dans le paquet de la Ville.
// Le banc cuit le paquet (comme la CI : cartes_v2::cuire), puis joue les mêmes scènes deux fois :
//   « secours » : sans paquet, le kit construit par le code (état de main) ;
//   « paquet »  : bloc « batiments » lu au chargement, plus aucune construction en course.
// Captures (règles de gestion de WFS-416) : rue d'immeubles de nuit et de jour en qualité Basse avec la ligne de mesure,
// façade de près (menuiserie 3D) et à 50 m (image avec relief), suite d'images à 160 km/h avec le temps de chaque image.
// Contrôles (écarts) : avec le paquet, aucune construction (archi::constructions) et aucune création de maillage
// (Renderer::creationsMaillages) pendant la course ; sans paquet et avec un bloc refusé, le kit de secours joue sans plantage.
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
class LigneBanc : public System {
public:
    const char* name() const override { return "batiments-banc"; }
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

struct Mesure { double mediane = 0, p99 = 0, pire = 0, chargementMs = 0, gpuMo = 0, kmh = 0; size_t constructions = 0; uint64_t creations = 0; bool ok = false; };

}  // namespace

int benchBatiments(Game& g, Renderer& r, const std::string& out, const std::string& racine) {
    int ecarts = 0;
    auto verifier = [&](bool ok, const std::string& quoi) { WFS_LOG("batiments (WFS-416) banc : %s : %s", quoi.c_str(), ok ? "OK" : "ÉCART"); if (!ok) ecarts++; };
    const std::string c = "ville", dossier = carte::dossierPaquets();
    const carte::FormatTexture F = carte::FormatTexture::RGBA8;
    const std::string chemin = dossier + "/" + carte::nomFichier(c, F);
    std::error_code ec;
    fs::create_directories(dossier, ec);
    for (carte::FormatTexture f : carte::FORMATS) fs::remove(dossier + "/" + carte::nomFichier(c, f), ec);

    // rue la plus bâtie de la Ville : deux morceaux de suite avec le plus d'immeubles, sur une partie droite
    int64_t depart = 0;
    {
        Track t; t.build(mapById(c.c_str()), trace::versionCarte(c.c_str()));
        ville::poserImmeublesKit(mapById(c.c_str()), t);
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
    verifier(bloc && !bloc->donnees.empty(), "bloc « batiments » présent dans le paquet de la Ville");
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
        if (!ok) { WFS_ERR("batiments (WFS-416) banc : chargement de la Ville impossible (%s)", etat); ecarts++; return m; }
        g.allerA(Ecran::Course);
        g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{};
        choisirVue(VueCamera::Poursuite);
        g.camMode = vue == 1 ? 2 : 0;
        if (vue == 1) { g.orbit = 1.5708f; g.orbitRecul = 3.f; g.orbitHauteur = 1.6f; }   // caméra à 3 m de la voiture, tournée vers la façade
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
        for (int i = 0; i < 60 * 6; i++) { pas(1.0 / 60); if (i % 6 == 0) g.render(t); }
        for (int i = 0; i < 12; i++) { pas(1.0 / 60); g.render(t); }
        const size_t c0 = archi::constructions();
        const uint64_t m0 = r.creationsMaillages();
        std::vector<double> ms;
        for (int i = 0; i < images; i++) {
            const double a = nowSeconds();
            pas(1.0 / 60); g.render(t);
            ms.push_back((nowSeconds() - a) * 1000);
            if (vue == 0 && images >= 120 && (i == images / 4 || i == images / 2 || i == 3 * images / 4)) {   // suite d'images : temps de chaque image
                char l[160]; std::snprintf(l, sizeof l, "image %d / %d : %.1f ms   %.0f km/h", i + 1, images, ms.back(), g.player().kmh());
                g_ligne[0] = std::string("Ville, ") + moment + ", Basse, " + etat; g_ligne[1] = l;
                g.render(t); r.capturePng(out + "/" + nom + "-suite-" + std::to_string(i * 4 / images) + ".png");
                g_ligne[0].clear(); g_ligne[1].clear();
            }
        }
        m.constructions = archi::constructions() - c0; m.creations = r.creationsMaillages() - m0;
        m.kmh = g.player().kmh(); m.gpuMo = memoire::gpuMo();
        std::vector<double> tri = ms; std::sort(tri.begin(), tri.end());
        m.mediane = tri[tri.size() / 2]; m.p99 = tri[std::min(tri.size() - 1, (size_t)(tri.size() * 0.99))]; m.pire = tri.back();
        char l[200];
        std::snprintf(l, sizeof l, "%.1f img/s (médiane %.1f ms, 99 %% < %.1f ms, pire %.1f ms)   constructions en course : %zu", 1000 / m.mediane, m.mediane, m.p99, m.pire, m.constructions);
        g_ligne[0] = std::string("Ville, ") + moment + ", Basse, " + etat + (vue == 1 ? ", façade de près" : vue == 2 ? ", façade à 50 m" : ""); g_ligne[1] = l;
        g.render(t); g.render(t); r.capturePng(out + "/" + nom + ".png");
        g_ligne[0].clear(); g_ligne[1].clear();
        WFS_LOG("batiments (WFS-416) mesure RG-07 : %s : Ville %s, Basse, %s : %s ; créations de maillages en course %llu ; %.0f km/h ; chargement %d ms ; mémoire graphique %.0f Mo (banc PC, rendu logiciel)",
                nom.c_str(), moment, etat, l, (unsigned long long)m.creations, m.kmh, (int)std::lround(m.chargementMs), m.gpuMo);
        g.shutdown();
        m.ok = true;
        return m;
    };
    auto serie = [&](const char* etat, const std::string& pre, Mesure* nuit) {
        scene(etat, "nuit", 0, 50, pre + "-tmp", 30);   // 1er passage après un changement : la lumière du rendu se construit (non comparé)
        fs::remove(out + "/" + pre + "-tmp.png", ec);
        *nuit = scene(etat, "nuit", 0, 50, pre + "-rue-nuit", 240);
        scene(etat, "jour", 0, 50, pre + "-rue-jour", 120);
        scene(etat, "jour", 1, 0, pre + "-facade-pres", 20);
        scene(etat, "jour", 2, 0, pre + "-facade-50m", 20);
        return scene(etat, "nuit", 0, 160, pre + "-160kmh", 360);
    };

    // 2) sans paquet : kit de secours (RG-06) = état de main
    verifier(!preparerCarte(c), "paquet absent → Ville du code");
    Mesure sn, s160 = serie("kit de secours (sans paquet)", "batiments-secours", &sn);
    verifier(s160.ok && s160.constructions > 0, "sans paquet : le kit de secours construit les bâtiments en roulant (comme avant)");

    // 3) avec le paquet : lu au chargement, aucune construction en course (RG-03)
    verifier(ecrireFichier(chemin, octets), "écriture de " + chemin);
    verifier(preparerCarte(c) && carte::carteActive() == c, "paquet valide → activé");
    Mesure pn, p160 = serie("paquet cuit", "batiments-paquet", &pn);
    verifier(pn.ok && pn.constructions == 0 && pn.creations == 0, "paquet : aucune construction ni création de maillage en course (rue, 50 km/h)");
    verifier(p160.ok && p160.constructions == 0 && p160.creations == 0, "paquet : aucune construction ni création de maillage en course (160 km/h)");
    WFS_LOG("batiments (WFS-416) mesure RG-07 : 160 km/h de nuit, Basse : secours médiane %.1f ms, 99 %% %.1f, pire %.1f ; paquet médiane %.1f ms, 99 %% %.1f, pire %.1f ; chargement %+d ms ; mémoire graphique %+.0f Mo",
            s160.mediane, s160.p99, s160.pire, p160.mediane, p160.p99, p160.pire, (int)std::lround(p160.chargementMs - s160.chargementMs), p160.gpuMo - s160.gpuMo);
    verifier(p160.chargementMs <= s160.chargementMs + 3000, "chargement au plus 3 s plus long avec le paquet (RG-07)");

    // 4) bloc des bâtiments refusé (cuit pour un autre plan) : le paquet reste lu, le kit de secours reprend la main, sans plantage
    {
        carte::Paquet faux = cuit;
        for (carte::Bloc& b : faux.blocs) if (b.nom == archi::cuit::NOM_BLOC && b.donnees.size() > 16) b.donnees[8] ^= 0x5A;   // signature
        verifier(ecrireFichier(chemin, carte::ecrire(faux)) && preparerCarte(c), "paquet au bloc de bâtiments faux : activé (le reste est bon)");
        const Mesure f = scene("bloc refusé → kit de secours", "jour", 0, 50, "batiments-bloc-refuse", 60);
        verifier(f.ok && f.constructions > 0, "bloc des bâtiments refusé → kit de secours, aucun plantage");
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
