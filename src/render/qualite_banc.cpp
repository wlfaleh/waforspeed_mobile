// WaForSpeed — banc d'essai de la qualité (WFS-15) : voir qualite_banc.h.
#include "qualite_banc.h"
#include <algorithm>
#include <cmath>
#include <cstdio>   // WFS-300 (RG-04) : snprintf
#include <cstdlib>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/game.h"
#include "../game/course5.h"          // WFS-134 : poursuite en Basse (aucune voiture en cubes)
#include "../game/course_system.h"
#include "../game/traffic_system.h"   // WFS-134 : voiture-course-trafic (3 voitures à 10, 25 et 50 m)
#include "../ui/ecrans/reglages.h"
#include "../../third_party/stb/stb_image.h"   // WFS-300 (RG-05) : relecture des captures (couleur d'une façade)
#include "bench_realisme.h"     // WFS-300 (RG-02, RG-05) : srgb8VersLab, deltaE2000 (lecture seule)
#include "eclairage_banc.h"     // WFS-300 (RG-02) : attendreSondes
#include "eclairage_physique.h" // WFS-300 (RG-01) : nombre de sondes traitées (lecture seule)
#include "qualite_auto.h"
#include "renderer.h"

namespace wfs {
namespace {

// WFS-239 (RG-02) : position (unités de la piste) du joueur tel qu'il est dessiné. Depuis WFS-221, l'image montre l'état
// interpolé entre les deux derniers pas de 1/120 s : avant + (après − avant) × Game::alphaRendu (part du pas suivant déjà
// écoulée ; au banc, presque 0 : le joueur dessiné est un pas derrière le joueur simulé, 15 cm à 67 km/h). Le banc fait
// exactement un pas (Game::pasDerniereImage) en notant la position d'avant : position dessinée exacte, quelle que soit la conduite.
double unPasPuisPositionDessinee(Game& g) {
    const double avant = g.player().position;
    g.update(1.0 / 120 + 1e-9);   // un pas : le reste du pas précédent (< 1/120 s) plus 1/120 s
    if (g.pasDerniereImage() != 1) WFS_ERR("ERREUR placement (WFS-239 RG-02) : %d pas au lieu d'un avant le placement", g.pasDerniereImage());
    const double apres = g.player().position, a = Game::interpolation ? g.alphaRendu() : 1.0;
    return avant + (apres - avant) * a;
}
// WFS-300 (RG-01, RG-02) : avant chaque capture, sondes d'environnement prêtes et stables : attendreSondes (au moins une capture
// traitée), puis des images au même temps jusqu'à ce qu'aucune nouvelle sonde (rebond compris) n'arrive pendant 6 images de suite
// (60 au plus). Cause des façades bleues de voiture-lod-passage-56 : les images de contrôle de WFS-239 (RG-03) font avancer les
// captures d'environnement, et une sonde (rebond) arrivait pendant les 3 images d'avant la capture.
void sondesStables(Game& g, double t, const std::string& nom) {
    const int n0 = eclairage::etat().sondesCapturees;
    attendreSondes(g, t);
    int n = eclairage::etat().sondesCapturees, calme = 0, images = 0;
    for (; images < 60 && calme < 6; images++) {
        g.render(t);
        const int m = eclairage::etat().sondesCapturees;
        calme = m == n ? calme + 1 : 0; n = m;
    }
    WFS_LOG("sondes %s (WFS-300 RG-02) : %d sonde(s) traitée(s) pendant l'attente (%d → %d), %d image(s) d'attente, sonde B %s",
            nom.c_str(), n - n0, n0, n, images, eclairage::etat().sondeCaptureActive ? "capturée" : "du ciel");
}
void capturer(Game& g, Renderer& r, const std::string& dossier, const std::string& nom, double t) {
    sondesStables(g, t, nom);
    r.capturePng(dossier + "/" + nom + ".png");
}
// WFS-300 (RG-05) : couleur moyenne (L*a*b*) d'une zone de la capture (x0, y0, x1, y1 en fractions de l'image)
bool couleurZone(const std::string& png, double x0, double y0, double x1, double y1, reel::Lab& lab) {
    int w = 0, h = 0, n = 0;
    unsigned char* px = stbi_load(png.c_str(), &w, &h, &n, 3);
    if (!px) return false;
    double acc[3] = {0, 0, 0}; long c = 0;
    for (int y = (int)(y0 * h); y < (int)(y1 * h); y++) for (int x = (int)(x0 * w); x < (int)(x1 * w); x++) {
        const unsigned char* q = px + ((size_t)y * w + x) * 3;
        for (int k = 0; k < 3; k++) acc[k] += q[k];
        c++;
    }
    stbi_image_free(px);
    if (!c) return false;
    const double m[3] = {acc[0] / c, acc[1] / c, acc[2] / c};
    lab = reel::srgb8VersLab(m);
    return true;
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
int controler(Game& g, Renderer& r, const std::string& nom, const std::vector<Posee>& posees, bool basse, double t, double pos) {
    Traffic* tr = activeTrafficMut();
    if (!tr || posees.empty()) return 0;
    const double limite = basse ? 120 : 300, a = Game::interpolation ? g.alphaRendu() : 1.0;
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
    // WFS-300 (RG-03) : voitures dessinées ENSEMBLE dans l'image réelle (toutes posées) : chacune est retirée à son tour ; elle était
    // dessinée si l'image perd des appels de dessin sans elle. Moins qu'une à une : plafond voulu (WFS-111 / WFS-134), pas une erreur.
    int ensemble = 0;
    if (posees.size() > 1) {
        const unsigned tous = appels();
        for (size_t k = 0; k < posees.size(); k++) {
            TrafficCar& c = tr->cars[posees[k].i];
            ranger(c, pos, posees[k].i, c.speed);
            ensemble += appels() < tous;
            c = garde[k];
        }
    } else ensemble = dessinees;
    WFS_LOG("contrôle %s (WFS-239 RG-03) : %zu voiture(s) posée(s), %d dessinée(s) (chacune seule ; image sans : %u appels de dessin), "
            "distances dessinées depuis le joueur dessiné : %s m ; limite de dessin %.0f m%s", nom.c_str(), posees.size(), dessinees, sans,
            distances.c_str(), limite, erreurs ? "" : " ; placement juste");
    WFS_LOG("voitures %s (WFS-300 RG-03) : placées %zu, dessinées une à une %d, dessinées ensemble %d%s", nom.c_str(), posees.size(), dessinees,
            ensemble, ensemble < dessinees ? " (plafond de dessin voulu, WFS-111 / WFS-134 : pas une erreur)" : "");
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
        capturer(g, r, out, std::string("qualite-") + noms[q], 904 + 19 / 60.0);   // WFS-300 (RG-02)
        WFS_LOG("qualité %s (WFS-134 RG-09) : %s", noms[q], qualite::ligneRendu().c_str());
        if (q >= 1) {   // WFS-104 : la Golf en course, vue de près (comme garage-golf), qualités Moyenne et Haute
            g.camMode = 2; g.orbit = 2.4f;
            for (int i = 0; i < 4; i++) g.render(905 + i / 60.0);
            capturer(g, r, out, std::string("voiture-course-") + noms[q], 905 + 3 / 60.0);   // WFS-300 (RG-02)
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
        const double posT = pr.trafic ? unPasPuisPositionDessinee(g) : 0;   // WFS-239 (RG-02) : distances depuis le joueur dessiné
        if (pr.trafic) if (Traffic* tr = activeTrafficMut()) {
            const double pos = posT, D[3] = {10, 25, 50};
            for (size_t i = 0; i < tr->cars.size(); i++) {   // 3 voitures placées devant, les autres loin derrière (hors champ)
                TrafficCar& c = tr->cars[i];
                if (i < 3) { c.lane = c.target = (int)(i % LANES); poser(c, pos, D[i], LANE_X[(i + 2) % LANES], g.player().speed); posees.push_back({i, D[i]}); }
                else ranger(c, pos, i, g.player().speed);
            }
        }
        // de près (orbite 2,4 m comme voiture-course-haute) ; trafic et Basse : caméra de poursuite haute du joueur
        if (!pr.trafic && pr.niveau == qualite::HAUTE) { g.camMode = 2; g.orbit = 2.4f; } else g.camMode = 1;
        for (int i = 0; i < 4; i++) g.render(914 + i / 60.0);
        capturer(g, r, out, pr.nom, 914 + 3 / 60.0);   // WFS-300 (RG-02)
        WFS_LOG("capture %s (WFS-134) : %s, %.0f km/h", pr.nom, qualite::ligneRendu().c_str(), g.player().kmh());
        controler(g, r, pr.nom, posees, pr.niveau == qualite::BASSE, 914 + 3 / 60.0, posT);   // WFS-239 (RG-03)
        g.shutdown();
    }
    // WFS-134 (RG-13) : la même voiture du trafic devant le joueur, à 10, 25, 50, 60, 80, 120 et 180 m, même caméra, Haute et
    // Basse (voiture-distance-<m>[-basse]) ; passage complète → allégée à 56, 60 et 64 m (voiture-lod-passage-<m>)
    {
        // WFS-239 (RG-02) : distance comptée depuis le joueur dessiné (WFS-221), état d'avant le pas remis
        double posD = 0;
        auto placer = [&](double d) {
            Traffic* tr = activeTrafficMut(); if (!tr) return;
            const double pos = posD = unPasPuisPositionDessinee(g);
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
                capturer(g, r, out, nom, 944 + 2 / 60.0);   // WFS-300 (RG-02)
                WFS_LOG("capture %s (WFS-134 RG-13) : voiture du trafic à %.0f m, %u appels de dessin", nom.c_str(), d, r.appelsDessin());
                controler(g, r, nom, {{0, d}}, niveau == qualite::BASSE, 944 + 2 / 60.0, posD);   // WFS-239 (RG-03)
            }
            if (niveau == qualite::HAUTE) for (double d : {56.0, 60.0, 64.0}) {
                placer(d);
                for (int i = 0; i < 3; i++) g.render(946 + i / 60.0);
                capturer(g, r, out, "voiture-lod-passage-" + std::to_string((int)d), 946 + 2 / 60.0);   // WFS-300 (RG-02) : plus de façades bleues
                WFS_LOG("capture voiture-lod-passage-%d (WFS-134 RG-12) : complète → allégée, fondu de 56 à 64 m du joueur", (int)d);
                controler(g, r, "voiture-lod-passage-" + std::to_string((int)d), {{0, d}}, false, 946 + 2 / 60.0, posD);   // WFS-239 (RG-03)
            }
            if (niveau == qualite::HAUTE) {
                // WFS-300 (RG-05) : mesurer sans corriger : la voiture avance de ≈ 110 m sans image (nouvelle sonde à capturer), puis
                // images une à une au même temps, chacune capturée ; on garde les 5 d'avant et les 5 d'après l'arrivée de la sonde
                // (voiture-sonde-0 à -9) et on mesure la couleur d'une zone de façades (quart haut droit) sur chacune : écart d'une
                // image à l'autre au journal ; plus de ΔE2000 2 → pour le PO (anomalie et ticket E13 à part)
                g.setAutopilot(true);
                for (int i = 0; i < 6 * 60; i++) g.update(1.0 / 60);
                g.setAutopilot(false);
                std::vector<std::string> f; std::vector<int> nb;
                int arrivee = -1;
                for (int i = 0; i < 40 && (arrivee < 0 || i < arrivee + 5); i++) {
                    g.render(948);
                    const std::string png = out + "/voiture-sonde-tmp-" + std::to_string(i) + ".png";
                    r.capturePng(png);
                    f.push_back(png); nb.push_back(eclairage::etat().sondesCapturees);
                    if (arrivee < 0 && i > 0 && nb[(size_t)i] > nb[(size_t)i - 1]) arrivee = i;
                }
                if (arrivee < 0) WFS_LOG("sonde (WFS-300 RG-05) : aucune sonde arrivée en %zu images après 110 m : mesure impossible", f.size());
                else {
                    const int d0 = std::max(0, arrivee - 5);
                    reel::Lab prec{}; double pire = 0; std::string ecarts;
                    for (int k = 0; k < 10 && d0 + k < (int)f.size(); k++) {
                        const std::string nom = out + "/voiture-sonde-" + std::to_string(k) + ".png";
                        std::rename(f[(size_t)(d0 + k)].c_str(), nom.c_str());
                        f[(size_t)(d0 + k)].clear();
                        reel::Lab lab{};
                        if (!couleurZone(nom, 0.70, 0.04, 0.98, 0.40, lab)) continue;
                        if (k > 0) { const double e = reel::deltaE2000(prec, lab); pire = std::max(pire, e); char b[32]; std::snprintf(b, sizeof b, "%s%.2f", ecarts.empty() ? "" : ", ", e); ecarts += b; }
                        prec = lab;
                        WFS_LOG("sonde (WFS-300 RG-05) : voiture-sonde-%d%s : façades (quart haut droit) L*a*b* %.1f %.1f %.1f, %d sonde(s) traitée(s)", k,
                                d0 + k == arrivee ? " (arrivée de la sonde)" : "", lab.L, lab.a, lab.b, nb[(size_t)(d0 + k)]);
                    }
                    WFS_LOG("sonde (WFS-300 RG-05) : écarts ΔE2000 d'une image à l'autre : %s ; pire %.2f%s", ecarts.c_str(), pire,
                            pire > 2 ? " → POUR LE PO : changement de couleur visible à l'arrivée d'une sonde en jeu (anomalie et ticket E13 à part)" : " (≤ 2 : pas de saut visible)");
                }
                for (const std::string& x : f) if (!x.empty()) std::remove(x.c_str());
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
            const double posP = unPasPuisPositionDessinee(g);   // WFS-239 (RG-02)
            if (Traffic* tr = activeTrafficMut()) {
                const double pos = posP;
                for (size_t i = 0; i < tr->cars.size(); i++) {
                    TrafficCar& c = tr->cars[i];
                    if (i < 14) { poser(c, pos, 8 + 11.0 * (double)i, LANE_X[i % LANES], g.player().speed); posees.push_back({i, 8 + 11.0 * (double)i}); places++; }
                    else ranger(c, pos, i, g.player().speed);
                }
            }
            unsigned appels = 0;
            for (int i = 0; i < 4; i++) { g.render(930 + i / 60.0); appels += r.appelsDessin(); }
            capturer(g, r, out, "voiture-basse-poursuite", 930 + 3 / 60.0);   // WFS-300 (RG-02)
            WFS_LOG("capture voiture-basse-poursuite (WFS-134) : %s, %zu voitures du trafic de 8 à 151 m, vrais modèles (aucun cube), "
                    "%u appels de dessin par image", qualite::ligneRendu().c_str(), places, appels / 4);
            controler(g, r, "voiture-basse-poursuite", posees, true, 930 + 3 / 60.0, posP);   // WFS-239 (RG-03)
            g.shutdown();
        }
        courseBenchRules(false); modeCourse() = ModeCourse::Checkpoint;
    }
    R.qualite = q0; ecrans::appliquerQualiteNiveau(r);
}

} // namespace wfs
