// WaForSpeed — banc d'essai de la qualité (WFS-15) : voir qualite_banc.h.
#include "qualite_banc.h"
#include <cmath>
#include <cstdlib>
#include <vector>
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/game.h"
#include "../game/course5.h"          // WFS-134 : poursuite en Basse (aucune voiture en cubes)
#include "../game/course_system.h"
#include "../game/traffic_system.h"   // WFS-134 : voiture-course-trafic (3 voitures à 10, 25 et 50 m)
#include "../ui/ecrans/reglages.h"
#include "qualite_auto.h"
#include "renderer.h"

namespace wfs {
namespace {

// WFS-239 (RG-02) : position (unités de la piste) du joueur tel qu'il est dessiné. Depuis WFS-221, l'image montre l'état
// interpolé entre les deux derniers pas de 1/120 s (Game::alphaRendu : part du pas suivant déjà écoulée) ; le dernier pas a
// avancé de pas × vitesse × cos(cap) (Player::update). À 150 km/h, le joueur dessiné est jusqu'à 35 cm derrière le joueur simulé.
double positionDessinee(const Game& g) {
    const Player& p = g.player();
    const double a = Game::interpolation ? g.alphaRendu() : 1.0;
    return p.position - (1.0 - a) * (1.0 / 120.0) * p.speed * std::cos(p.heading);
}
// WFS-239 (RG-02) : voiture du trafic posée à d m devant le joueur dessiné, au décalage x (demi-largeurs de route), à la vitesse
// du joueur ; état d'avant le pas remis (WFS-221 : sinon, après un saut de moins de 20 m, elle est dessinée entre son ancienne
// place et la nouvelle)
void poser(TrafficCar& c, double posDessinee, double d, double x, double vitesse) {
    c.crashed = false; c.speed = c.want = vitesse;
    c.offset = c.prevOff = c.offPrec = x;
    c.z = c.zPrec = posDessinee + d / M;
    c.yawPrec = c.laneYaw;
}
void ranger(TrafficCar& c, double posDessinee, size_t i, double vitesse) {   // loin derrière le joueur, hors champ
    c.crashed = false; c.speed = c.want = vitesse;
    c.z = c.zPrec = posDessinee - (200 + 5.0 * (double)i) / M;
}
// WFS-239 (RG-03) : contrôle des voitures posées pour la capture `nom` : chacune est dessinée seule (les autres posées rangées
// loin derrière), et ses appels de dessin sont comparés à l'image sans aucune ; distance dessinée depuis le joueur dessiné, avec
// l'interpolation du trafic (traffic_system.cpp, WFS-221 : zPrec + (z − zPrec) × alpha, sauf saut de plus de 20 m). Erreur
// (« ERREUR placement ») si une voiture posée en deçà de la limite de dessin du niveau (Basse : 120 m inclus ; Haute : 300 m)
// n'est pas dessinée, ou si elle est dessinée à plus de 0,5 m de sa place. L'état des voitures est remis à la fin.
struct Posee { size_t i; double d; };
int controler(Game& g, Renderer& r, const std::string& nom, const std::vector<Posee>& posees, bool basse, double t) {
    Traffic* tr = activeTrafficMut();
    if (!tr || posees.empty()) return 0;
    const double limite = basse ? 120 : 300, pos = positionDessinee(g), a = Game::interpolation ? g.alphaRendu() : 1.0;
    std::vector<TrafficCar> garde;
    for (const Posee& p : posees) garde.push_back(tr->cars[p.i]);
    auto appels = [&]() { g.render(t); g.render(t); return r.appelsDessin(); };
    for (const Posee& p : posees) ranger(tr->cars[p.i], pos, p.i, tr->cars[p.i].speed);
    const unsigned sans = appels();
    int dessinees = 0, erreurs = 0;
    std::string distances;
    for (size_t k = 0; k < posees.size(); k++) {
        TrafficCar& c = tr->cars[posees[k].i];
        const TrafficCar rangee = c;
        c = garde[k];
        const unsigned avec = appels();
        const double dzc = c.z - c.zPrec, al = std::abs(dzc) * M > 20 ? 1.0 : a;
        const double dessinee = (c.zPrec + dzc * al - pos) * M;
        const bool vue = avec > sans;
        dessinees += vue;
        char b[48]; std::snprintf(b, sizeof b, "%s%.2f%s", distances.empty() ? "" : ", ", dessinee, vue ? "" : " (non dessinée)");
        distances += b;
        if (posees[k].d <= limite && !vue) {
            WFS_ERR("ERREUR placement %s (WFS-239 RG-03) : voiture posée à %.1f m, en deçà de la limite de %.0f m, non dessinée", nom.c_str(), posees[k].d, limite);
            erreurs++;
        }
        if (vue && std::abs(dessinee - posees[k].d) > 0.5) {
            WFS_ERR("ERREUR placement %s (WFS-239 RG-03) : voiture posée à %.1f m, dessinée à %.2f m (écart de plus de 0,5 m)", nom.c_str(), posees[k].d, dessinee);
            erreurs++;
        }
        c = rangee;
    }
    for (size_t k = 0; k < posees.size(); k++) tr->cars[posees[k].i] = garde[k];
    WFS_LOG("contrôle %s (WFS-239 RG-03) : %zu voiture(s) posée(s), %d dessinée(s) (chacune seule ; image sans : %u appels de dessin), "
            "distances dessinées depuis le joueur dessiné : %s m ; limite de dessin %.0f m%s", nom.c_str(), posees.size(), dessinees, sans,
            distances.c_str(), limite, erreurs ? "" : " ; placement juste");
    return erreurs;
}

} // namespace

void benchQualite(Game& g, Renderer& r, const std::string& out) {
    // images/s simulées (watchPerf) : 25 images/s pendant 10 s, puis 60 images/s
    qualite::Auto a;
    for (int s = 0; s < 6; s++) {
        int e = 0; for (int i = 0; i < 50; i++) e |= a.image(1.0 / 25, true);
        WFS_LOG("qualité simulée : 25 images/s, mesure %d → échelle %.2f, niveau %d%s", s + 1, a.echelle, a.niveau,
                (e & qualite::Auto::RETRO) ? " — rétrogradation : « Graphismes rapides pour plus de fluidité »" : "");
    }
    for (int s = 0; s < 3; s++) { for (int i = 0; i < 120; i++) a.image(1.0 / 60, true); }
    WFS_LOG("qualité simulée : 60 images/s pendant 6 s → échelle %.2f, niveau %d (pas de remontée de niveau)", a.echelle, a.niveau);
    // WFS-86 : échelle réelle de la scène sur un téléphone lent (résolution dynamique au plancher 0,6) : avant le
    // correctif, échelle du niveau × 0,6 (Haute 60 %, Moyenne 45 %, Basse 30 %) ; après : plancher 60 %, Basse fixe 50 %
    WFS_LOG("qualité (WFS-86) : échelle réelle à 0,6 de résolution dynamique : Haute %.0f %%, Moyenne %.0f %%, Basse %.0f %% (avant : 60 / 45 / 30 %%)",
            qualite::echelleRendu(qualite::HAUTE, 0.6f) * 100, qualite::echelleRendu(qualite::MOYENNE, 0.6f) * 100,
            qualite::echelleRendu(qualite::BASSE, 0.6f) * 100);

    // les trois niveaux, de nuit (bloom des feux, néons et phares), même scène
    ecrans::Reglages& R = ecrans::reglages();
    const int q0 = R.qualite;
    const char* noms[3] = {"basse", "moyenne", "haute"};
    setenv("WFS_MOMENT", "nuit", 1);
    for (int q = 0; q < 3; q++) {
        R.qualite = q; ecrans::appliquerQualiteNiveau(r);
        if (!g.init(r, "golf", "ville")) { WFS_ERR("qualité : chargement impossible"); break; }
        g.setAutopilot(true); g.camMode = 1;
        for (int i = 0; i < 240; i++) { g.update(1.0 / 60); if (i % 8 == 0) g.render(900 + i / 60.0); }
        const double t0 = nowSeconds();
        for (int i = 0; i < 20; i++) { g.update(1.0 / 60); g.render(904 + i / 60.0); }
        const double ms = (nowSeconds() - t0) / 20 * 1000;
        r.capturePng(out + "/qualite-" + noms[q] + ".png");
        WFS_LOG("qualité %s (WFS-134 RG-09) : %s", noms[q], qualite::ligneRendu().c_str());
        if (q >= 1) {   // WFS-104 : la Golf en course, vue de près (comme garage-golf), qualités Moyenne et Haute
            g.camMode = 2; g.orbit = 2.4f;
            for (int i = 0; i < 4; i++) g.render(905 + i / 60.0);
            r.capturePng(out + "/voiture-course-" + noms[q] + ".png");
            WFS_LOG("qualité (WFS-104) : voiture-course-%s : scène à %.0f %%, MSAA ×%d, niveau %s (garage : même échelle, même MSAA)",
                    noms[q], r.vk().renderScale * 100, (int)r.vk().msaa, noms[q]);
            g.camMode = 1;
        }
        WFS_LOG("qualité %s : scène à %.0f %%, ombres %s, bloom %.1f, %u appels de dessin, %u objets masqués (WFS-82), %.1f ms par image (banc, CPU)",
                noms[q], r.vk().renderScale * 100, r.forceNoShadows ? "coupées" : "actives", r.bloomForce, r.appelsDessin(), r.objetsMasques(), ms);
        g.shutdown();
    }
    unsetenv("WFS_MOMENT");

    // WFS-134 (RG-09) : la voiture en course, de jour, vue par le joueur (caméra de poursuite) : Haute de près, Haute avec
    // 3 voitures du trafic à 10, 25 et 50 m devant, Basse (75 %) ; à comparer avec garage-golf
    struct Prise { const char* nom; int niveau; bool trafic; };
    for (const Prise& pr : {Prise{"voiture-course-proche", qualite::HAUTE, false}, Prise{"voiture-course-trafic", qualite::HAUTE, true},
                            Prise{"voiture-course-basse", qualite::BASSE, false}}) {
        R.qualite = pr.niveau; ecrans::appliquerQualiteNiveau(r);
        if (!g.init(r, "golf", "ville")) { WFS_ERR("qualité : chargement impossible"); break; }
        g.setAutopilot(true); g.camMode = 0;
        for (int i = 0; i < 240; i++) { g.update(1.0 / 60); if (i % 8 == 0) g.render(910 + i / 60.0); }
        std::vector<Posee> posees;
        if (pr.trafic) if (Traffic* tr = activeTrafficMut()) {
            const double pos = positionDessinee(g), D[3] = {10, 25, 50};   // WFS-239 (RG-02) : depuis le joueur dessiné
            for (size_t i = 0; i < tr->cars.size(); i++) {   // 3 voitures placées devant, les autres loin derrière (hors champ)
                TrafficCar& c = tr->cars[i];
                if (i < 3) { c.lane = c.target = (int)(i % LANES); poser(c, pos, D[i], LANE_X[(i + 2) % LANES], g.player().speed); posees.push_back({i, D[i]}); }
                else ranger(c, pos, i, g.player().speed);
            }
        }
        // de près (orbite 2,4 m comme voiture-course-haute) ; trafic et Basse : caméra de poursuite haute du joueur
        if (!pr.trafic && pr.niveau == qualite::HAUTE) { g.camMode = 2; g.orbit = 2.4f; } else g.camMode = 1;
        for (int i = 0; i < 4; i++) g.render(914 + i / 60.0);
        r.capturePng(out + "/" + pr.nom + ".png");
        WFS_LOG("capture %s (WFS-134) : %s, %.0f km/h", pr.nom, qualite::ligneRendu().c_str(), g.player().kmh());
        controler(g, r, pr.nom, posees, pr.niveau == qualite::BASSE, 914 + 3 / 60.0);   // WFS-239 (RG-03)
        g.shutdown();
    }
    // WFS-134 (RG-13) : la même voiture du trafic devant le joueur, à 10, 25, 50, 60, 80, 120 et 180 m, même caméra, Haute et
    // Basse (voiture-distance-<m>[-basse]) ; passage complète → allégée à 56, 60 et 64 m (voiture-lod-passage-<m>)
    {
        // WFS-239 (RG-02) : distance comptée depuis le joueur dessiné (WFS-221), état d'avant le pas remis
        auto placer = [&](double d) {
            Traffic* tr = activeTrafficMut(); if (!tr) return;
            const double pos = positionDessinee(g);
            for (size_t i = 0; i < tr->cars.size(); i++) {
                if (i == 0) poser(tr->cars[i], pos, d, 0, g.player().speed); else ranger(tr->cars[i], pos, i, g.player().speed);
            }
        };
        for (int niveau : {qualite::HAUTE, qualite::BASSE}) {
            R.qualite = niveau; ecrans::appliquerQualiteNiveau(r);
            if (!g.init(r, "golf", "ville")) break;
            g.setAutopilot(true); g.camMode = 1;
            for (int i = 0; i < 240; i++) { g.update(1.0 / 60); if (i % 8 == 0) g.render(940 + i / 60.0); }
            g.setAutopilot(false);
            const double D[] = {10, 25, 50, 60, 80, 120, 180};
            for (double d : D) {
                placer(d);
                for (int i = 0; i < 3; i++) g.render(944 + i / 60.0);
                const std::string nom = "voiture-distance-" + std::to_string((int)d) + (niveau == qualite::BASSE ? "-basse" : "");
                r.capturePng(out + "/" + nom + ".png");
                WFS_LOG("capture %s (WFS-134 RG-13) : voiture du trafic à %.0f m, %u appels de dessin", nom.c_str(), d, r.appelsDessin());
                controler(g, r, nom, {{0, d}}, niveau == qualite::BASSE, 944 + 2 / 60.0);   // WFS-239 (RG-03)
            }
            if (niveau == qualite::HAUTE) for (double d : {56.0, 60.0, 64.0}) {
                placer(d);
                for (int i = 0; i < 3; i++) g.render(946 + i / 60.0);
                r.capturePng(out + "/voiture-lod-passage-" + std::to_string((int)d) + ".png");
                WFS_LOG("capture voiture-lod-passage-%d (WFS-134 RG-12) : complète → allégée, fondu de 56 à 64 m du joueur", (int)d);
                controler(g, r, "voiture-lod-passage-" + std::to_string((int)d), {{0, d}}, false, 946 + 2 / 60.0);   // WFS-239 (RG-03)
            }
            g.shutdown();
        }
    }
    // WFS-134 (demande de l'utilisateur : « jamais de cubes ») : poursuite en qualité Basse, trafic dense placé de 8 à 150 m
    // devant, police et trafic : vrais modèles à toute distance ; appels de dessin au journal
    {
        R.qualite = qualite::BASSE; ecrans::appliquerQualiteNiveau(r);
        courseBenchRules(true); modeCourse() = ModeCourse::Poursuite;
        if (g.init(r, "golf", "ville")) {
            g.setAutopilot(true); g.camMode = 1;
            for (int i = 0; i < 8 * 60; i++) { g.update(1.0 / 60); if (i % 8 == 0) g.render(920 + i / 60.0); }
            size_t places = 0;
            std::vector<Posee> posees;
            if (Traffic* tr = activeTrafficMut()) {
                const double pos = positionDessinee(g);   // WFS-239 (RG-02)
                for (size_t i = 0; i < tr->cars.size(); i++) {
                    TrafficCar& c = tr->cars[i];
                    if (i < 14) { poser(c, pos, 8 + 11.0 * (double)i, LANE_X[i % LANES], g.player().speed); posees.push_back({i, 8 + 11.0 * (double)i}); places++; }
                    else ranger(c, pos, i, g.player().speed);
                }
            }
            unsigned appels = 0;
            for (int i = 0; i < 4; i++) { g.render(930 + i / 60.0); appels += r.appelsDessin(); }
            r.capturePng(out + "/voiture-basse-poursuite.png");
            WFS_LOG("capture voiture-basse-poursuite (WFS-134) : %s, %zu voitures du trafic de 8 à 151 m, vrais modèles (aucun cube), "
                    "%u appels de dessin par image", qualite::ligneRendu().c_str(), places, appels / 4);
            controler(g, r, "voiture-basse-poursuite", posees, true, 930 + 3 / 60.0);   // WFS-239 (RG-03)
            g.shutdown();
        }
        courseBenchRules(false); modeCourse() = ModeCourse::Checkpoint;
    }
    R.qualite = q0; ecrans::appliquerQualiteNiveau(r);
}

} // namespace wfs
