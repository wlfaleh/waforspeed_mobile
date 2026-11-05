// WaForSpeed — banc d'essai sur ordinateur (sans fenêtre) : rend des scènes de test hors écran
// et les enregistre en PNG (utilisé par l'intégration continue pour vérifier le rendu).
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../audio/audio_system.h"
#include "../core/scene_essai.h"
#include "../game/camera_banc.h"
#include "../game/game.h"
#include "../game/course5.h"   // WFS-110
#include "../game/bench_changement_carte.h"
#include "../game/course_system.h"
#include "../game/duel_system.h"
#include "../game/conduite_libre_banc.h"
#include "../game/ressenti.h"   // WFS-70 : ressenti de conduite (captures ressenti-*.png)
#include "../game/sauvegarde.h"   // WFS-24
#include "../game/traffic_system.h"
#include "../ui/hud.h"
#include "../ui/planche_voitures.h"
#include "../render/bench_ombres.h"
#include "../render/bench_squelette.h"
#include "../world/pietons.h"
#include "../world/decor_carte.h"
#include "../render/bench_spots.h"
#include "../render/qualite_banc.h"
#include "../game/roues_banc.h"
#include "../render/bench_effets.h"
#include "../render/bench_nuit.h"
#include "../render/renderer.h"
#include "../render/vk.h"
#include "../ui/ecrans/chargement.h"
#include "manette_jeu.h"   // WFS-22
#include "../ui/ecrans/menu.h"
#include "../ui/ecrans/pause.h"
#include "../ui/ecrans/salon.h"
#include "../ui/ecrans/reglages.h"
#include "../ui/ecrans/terrain.h"
#include "../ui/garage.h"
#include "../world/enseignes.h"
#include "../ui/text.h"
#include "../game/cars.h"
#include <cstring>


namespace wfs {
static std::string g_root = "assets/";
bool readAsset(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream f(g_root + path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), {});
    return true;
}
double nowSeconds() {
    using namespace std::chrono;
    static auto t0 = steady_clock::now();
    return duration<double>(steady_clock::now() - t0).count();
}
}

using namespace wfs;

int main(int argc, char** argv) {
    std::string out = argc > 1 ? argv[1] : "captures";
    if (argc > 2) g_root = argv[2];
    jobs().start();
    VkCtx vk;
    // taille des captures (WFS_TAILLE=2400x1080 pour tester un grand écran), 1280x720 par défaut
    unsigned cw = 1280, chh = 720;
    if (const char* t = std::getenv("WFS_TAILLE")) std::sscanf(t, "%ux%u", &cw, &chh);
    if (!vk.init(nullptr, cw, chh)) return 1;
    const double tVk = nowSeconds();
    Renderer r; r.init(vk);
    Game g;
    courseBenchRules(false);   // WFS-12 : scènes existantes sans décompte ni fin (benchCourseScene les remet)
    // WFS-69 : groupes de scènes lancés en parallèle par la CI (WFS_BANC) ; sans WFS_BANC, tout le banc comme avant.
    // WFS-106 : jobs de CI ≤ 11 min (durées mesurées) : partie1 = partie1a + partie1b, partie2 = partie2a + partie2b + partie2c,
    //   menu = menu1 + menu2 ; même ordre des scènes que dans le banc complet.
    //   partie1a : chargement, vitrines, course, ombres, trafic, écran vide, scène dédiée
    //   partie1b : plage, cartes, effets (WFS-65), trafic vivant (WFS-67)
    //   partie2a : son, course (décompte / fin), HUD, menus, pause, salon, garage
    //   partie2b : duel, conduite libre, roues (WFS-60), terrain, réglages, changement de carte, qualité (WFS-15)
    //   partie2c : projecteurs, personnages (WFS-63), caméras, piétons, ressenti, encoche
    //   chargement : écran de chargement seul ; texte : planche de texte (course-cam0-0 seule) ; hud ; menu (menu1, menu2)
    const std::string banc = std::getenv("WFS_BANC") ? std::getenv("WFS_BANC") : "";
    auto finir = [&]() { r.shutdown(); vk.shutdown(); jobs().stop(); return 0; };
    if (banc == "hud") { benchHud(g, r, out); return finir(); }   // WFS-27 : HUD seul (2400x1080)
    if (banc == "carte") { benchChangementCarte(g, r, out); return finir(); }   // WFS-74 : changement de carte seul (essai local)
    if (banc == "vitrines") {   // WFS-13 : vitrine des 16 voitures ajoutées, garage de 2 d'entre elles, course avec la SF90
        ui::chargerPolice(r);
        for (int i = TRAFIC_MODELES; i < CAR_COUNT; i++) {
            const char* id = CARS[i].id;
            if (!g.init(r, id, "ville")) { WFS_ERR("chargement %s impossible", id); continue; }
            g.camMode = 2; g.orbit = 0.7f; g.render(0); g.render(0); r.capturePng(out + "/vitrine-" + id + "-0.png");
            const bool garage = !std::strcmp(id, "p208") || !std::strcmp(id, "sf90"), course = !std::strcmp(id, "sf90");
            if (garage) { g.allerA(Ecran::Garage); for (int k = 0; k < 40; k++) { g.update(1.0 / 60); if (k % 10 == 0) g.render(950 + k / 60.0); } g.render(951); g.render(951.01); r.capturePng(out + "/garage-" + id + ".png"); }
            if (course) { g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true); for (int k = 0; k < 240; k++) { g.update(1.0 / 60); if (k % 8 == 0) g.render(960 + k / 60.0); } g.render(964); g.render(964.02); r.capturePng(out + "/course-" + std::string(id) + ".png"); g.setAutopilot(false); }
            WFS_LOG("capture vitrine-%s-0%s%s", id, garage ? ", garage" : "", course ? ", course" : "");
            g.shutdown();
        }
        benchPlancheVoitures(g, r, out);   // WFS-77 : les 20 voitures en Turquoise, roues et peinture au journal
        return finir();
    }
    if (banc == "nuit") { benchNuit(g, r, out); return finir(); }   // WFS-09 / WFS-16 : nuit, phares, lampadaires, route mouillée (course-nuit-*, job nuit de la CI)
    if (banc == "qualite") { ui::chargerPolice(r); benchQualite(g, r, out); ecrans::benchGarage(g, r, out); return finir(); }   // WFS-104 : course vs garage (essai local)
    if (banc == "course5") { ui::chargerPolice(r); benchCourse5(g, r, out); return finir(); }   // WFS-110 : modes, course à 5
    if (banc == "vie") { ui::chargerPolice(r); benchVie(g, r, out); return finir(); }   // WFS-71 : terrasse, enseignes, portique, plage
    if (banc == "garage") { ecrans::benchGarage(g, r, out); return finir(); }   // WFS-11 : garage seul (essai local)
    if (banc == "camera") { benchCameras(g, r, out); return finir(); }   // WFS-66 : caméras seules (2400x1080 : job ecrans-2400)
    if (banc == "squelette") { benchSquelette(g, r, out); return finir(); }   // WFS-63 : personnages animés seuls
    if (banc == "cartes") { benchCartes(g, r, out); return finir(); }         // WFS-53 : cartes Campagne et Grand Prix seules
    if (banc == "ressenti") { benchRessenti(g, r, out); return finir(); }   // WFS-70 : ressenti de conduite seul
    if (banc == "sauvegarde") {   // WFS-24 : à lancer deux fois ; chemin propre au banc (WFS_SAUVEGARDE pour le forcer)
        benchSauvegardeScene(g, r, out, std::getenv("WFS_SAUVEGARDE") ? std::getenv("WFS_SAUVEGARDE") : out + "/sauvegarde-banc.txt"); return finir();
    }
    if (banc == "manette") { manette::benchManette(g, r, out); return finir(); }   // WFS-22 : manettes (job texte)
    if (banc == "pietons") { benchPietons(g, r, out); return finir(); }       // WFS-19 : piétons seuls
    const bool menu1 = banc == "menu" || banc == "menu1", menu2 = banc == "menu" || banc == "menu2";   // WFS-106 : 2400x1080 en deux jobs
    if (menu1) { ecrans::benchMenu(g, r, out); ecrans::benchMenuTactile(g, r, out); ecrans::benchPause(g, r, out); ecrans::benchGarage(g, r, out);
        ecrans::benchTerrain(g, r, out); }
    // menu2 seul (job à part) : record de session que la course « fin-menu » de menu1 lui laissait (950 à 955 selon la
    // machine), sinon reglages-menu et menu-duel montreraient le menu sans record (RG-08 : mêmes captures)
    if (menu2 && !menu1) sauvegarde().best = 955;
    if (menu2) { ecrans::benchReglages(g, r, out); ecrans::benchSalon(g, r, out); }
    if (menu1 || menu2) return finir();   // WFS-37, WFS-11, WFS-41, WFS-83
    // WFS-106 : un morceau tourne sans WFS_BANC, sous son nom ou sous celui de sa partie (partie1 / partie2)
    auto morceau = [&](const char* nom, const char* partie) { return banc.empty() || banc == nom || banc == partie; };
    const bool texte = banc == "texte", partie1a = morceau("partie1a", "partie1"), partie1b = morceau("partie1b", "partie1"),
               partie2a = morceau("partie2a", "partie2"), partie2b = morceau("partie2b", "partie2"), partie2c = morceau("partie2c", "partie2");
    if (!texte && banc != "chargement" && !partie1a && !partie1b && !partie2a && !partie2b && !partie2c) {   // aucune capture en silence
        WFS_ERR("WFS_BANC=%s : groupe inconnu", banc.c_str()); finir(); return 2;
    }
    if (partie1a || banc == "chargement")
        ecrans::benchChargement(g, r, out, tVk, [](bool ok) { static const std::string keep = g_root; g_root = ok ? keep : "absent/"; });   // WFS-35
    if (banc == "chargement") return finir();
    if (partie1a) for (const char* id : {"golf", "m4", "p911", "gallardo"}) {
        if (!g.init(r, id, "ville")) { WFS_ERR("chargement %s impossible", id); return 2; }
        // vitrine : 3 angles
        g.camMode = 2;
        for (int k = 0; k < 3; k++) {
            g.orbit = 0.7f + k * 2.1f;
            g.render(0); g.render(0);
            r.capturePng(out + "/vitrine-" + id + "-" + std::to_string(k) + ".png");
        }
        g.shutdown();
    }
    if (partie1a || texte) {
        // course : la Golf roule en pilote automatique, captures à différents moments
        g.init(r, "golf", std::getenv("WFS_CARTE") ? std::getenv("WFS_CARTE") : "ville");   // WFS-14 : WFS_CARTE=plage pour essayer une autre carte
        if (const char* e = std::getenv("WFS_ENCOCHE")) g.setMarges({(float)std::atoi(e), 0, 0, 0});   // WFS-51 : encoche simulée à gauche
        g.setAutopilot(true);
        const int cams = texte ? 1 : 2, shots = texte ? 1 : 3;   // planche de texte (WFS-26) : course-cam0-0 seule
        for (int cam = 0; cam < cams; cam++) {
            g.camMode = cam;
            for (int shot = 0; shot < shots; shot++) {
                for (int i = 0; i < 240; i++) { g.update(1.0 / 60); if (i % 8 == 0) g.render(shot * 10 + i / 60.0); }
                g.render(shot * 10 + 4.0); g.render(shot * 10 + 4.02);
                char name[128]; std::snprintf(name, sizeof name, "%s/course-cam%d-%d.png", out.c_str(), cam, shot);
                r.capturePng(name);
                WFS_LOG("capture %s : %.0f km/h, %.0f m", name, g.player().kmh(), g.player().dist * M);
            }
        }
        if (texte) { g.shutdown(); return finir(); }
    }
    if (partie1a) {
        benchShadowScenes(g, r, out);    // WFS-07 : ombres du soleil (captures ombres-*.png)
        benchTrafficScenes(g, r, out);   // WFS-08 : trafic de près et choc (captures trafic-*.png)
        // WFS-36 : écran Menu forcé (sans menu dessiné) : scène sans commandes tactiles, voiture immobile
        g.allerA(Ecran::Menu); g.setAutopilot(false);
        for (int i = 0; i < 90; i++) { g.update(1.0 / 60); if (i % 15 == 0) g.render(300 + i / 60.0); }
        g.render(301.5); r.capturePng(out + "/ecran-menu-vide.png");
        WFS_LOG("capture ecran-menu-vide : écran %s, %.1f km/h", nomEcran(g.ecran()), g.player().kmh());
        // WFS-44 : un système de test prend la scène (fond uni #07070c, cube, caméra FOV 36°)
        activerSceneEssai(true); g.render(310); g.render(310.02); r.capturePng(out + "/scene-dediee-vide.png"); activerSceneEssai(false);
        g.shutdown();
    }
    if (partie1b) {
        // WFS-14 : carte Corniche (plage) de jour et de nuit, en pilote automatique, caméra haute
        for (const char* mo : {"jour", "nuit"}) {
            setenv("WFS_MOMENT", mo, 1); g.init(r, "golf", "plage"); g.setAutopilot(true); g.camMode = 1;
            for (int i = 0; i < 240; i++) { g.update(1.0 / 60); if (i % 8 == 0) g.render(400 + i / 60.0); }
            g.render(404); g.render(404.02); r.capturePng(out + "/carte-plage-" + mo + ".png"); g.shutdown();
        }
        unsetenv("WFS_MOMENT");
        benchCartes(g, r, out);   // WFS-53 : cartes Campagne et Grand Prix (carte-campagne-*, carte-circuit-*)
        benchEffets(g, r, out);   // WFS-65 : étincelles, fumée, lignes de vitesse (choc-etincelles, accident-fumee, nitro-lignes)
        benchTraficVivant(g, r, out);    // WFS-67 : clignotant, feux stop, accident (trafic-clignotant / -freinage / -accident)
    }
    if (partie2a) {
        benchAudioScene(g, r, out);   // WFS-10 : son du moteur (moteur.wav, moteur-courbe.png)
        benchCourseScene(g, r, out);  // WFS-12 : décompte, checkpoints, fin de partie (course-*.png)
        benchHud(g, r, out);          // WFS-27 : HUD (hud-course, hud-temps-bas, hud-nitro, hud-combo)
        ecrans::benchMenu(g, r, out); // WFS-37 : menu principal (menu-anim, menu-0)
        ecrans::benchMenuTactile(g, r, out);   // WFS-38 : appuis du menu, départ, « Bientôt », fin → Menu
        ecrans::benchPause(g, r, out);         // WFS-39 : bouton Menu, pause, reprise, recommencer, quitter
        ecrans::benchSalon(g, r, out);         // WFS-83 : salon du duel, second joueur en UDP (boucle locale)
        ecrans::benchGarage(g, r, out);        // WFS-11 : garage (plateau, fiche, peintures, voiture suivante, « Choisir »)
    }
    if (partie2b) {
        benchDuelScene(g, r, out);    // WFS-21 : duel, adversaire simulé (duel-course.png, duel-resultat.png)
        benchConduiteLibre(g, r, out);   // WFS-90 : demi-tour et contresens (conduite-demitour.png, conduite-contresens.png)
        benchRoues(r, out);           // WFS-60 : roues animées (roues-arret-1/2, roues-roule, roues-virage)
        ecrans::benchTerrain(g, r, out);       // WFS-41 : choix du terrain et du moment, « Rouler » sur le circuit
        ecrans::benchReglages(g, r, out);      // WFS-68 : réglages depuis le menu et la pause, qualité basse
        benchChangementCarte(g, r, out);       // WFS-74 : écran de chargement au changement de carte, course figée hors course
        benchQualite(g, r, out);      // WFS-15 : qualité Basse / Moyenne / Haute de nuit, bloom, images/s simulées
    }
    if (partie2c) {
        benchSpots(r, out);           // WFS-45 : projecteurs, hémisphère, brouillard linéaire (spots-sol.png)
        benchSquelette(g, r, out);    // WFS-63 : personnages animés par squelette (squelette-*.png)
        benchCameras(g, r, out);      // WFS-66 : vues poursuite, capot, cockpit (camera-*.png)
        benchPietons(g, r, out);      // WFS-19 : piétons de la ville et de la plage (pietons-*.png)
        benchRessenti(g, r, out);     // WFS-70 : choc, flèches, mouvements réduits (ressenti-*.png)
    }
    r.shutdown();
    vk.shutdown();
    if (partie2c) {   // WFS-51 : encoche de 90 px à gauche en 2400x1080 : bande marquée, volant et jauges décalés à droite de la bande
        VkCtx vk2; Renderer r2; Game g2;
        if (vk2.init(nullptr, 2400, 1080) && r2.init(vk2) && g2.init(r2, "golf", "ville")) {
            g2.setMarges({90, 0, 0, 0}); g2.montrerMarges = true; g2.setAutopilot(true);
            for (int i = 0; i < 120; i++) { g2.update(1.0 / 60); if (i % 10 == 0) g2.render(500 + i / 60.0); }
            g2.render(502); g2.render(502.02); r2.capturePng(out + "/encoche-course.png");
            WFS_LOG("capture encoche-course : marge gauche %.0f px, écran %ux%u", g2.marges().gauche, r2.width(), r2.height());
            g2.shutdown(); r2.shutdown(); vk2.shutdown();
        }
    }
    jobs().stop();
    return 0;
}
