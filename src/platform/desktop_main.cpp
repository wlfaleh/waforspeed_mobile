// WaForSpeed — banc d'essai sur ordinateur (sans fenêtre) : rend des scènes de test hors écran
// et les enregistre en PNG (utilisé par l'intégration continue pour vérifier le rendu).
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>   // WFS-197 (accroche)
#include <fstream>
#include <string>
#include <vector>
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../audio/audio_system.h"
#include "../core/scene_essai.h"
#include "../game/camera_banc.h"
#include "../game/camera_vue.h"   // WFS-197 (accroche) : benchCockpit
#include "../game/game.h"
#include "../game/course5.h"   // WFS-110
namespace wfs { int benchParcours(Game& g, Renderer& r, const std::string& out); }
namespace wfs { int benchFluidite(Game& g, Renderer& r, const std::string& out); }   // WFS-221 (bench_fluidite.cpp)   // WFS-115 (modes_ecran.cpp)
namespace wfs { void benchCorniche(Game& g, Renderer& r, const std::string& out); }   // WFS-139 (plage.cpp)
#include "../game/poursuite.h"   // WFS-111
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
#include "../render/bench_perf.h"   // WFS-222 (accroche) : WFS_BANC=perf
#include "../game/conduite_banc.h"   // WFS-241 (accroche) : WFS_BANC=conduite
#include "../world/pietons.h"
#include "../world/conducteurs.h"   // WFS-73 (accroche)
#include "../world/decor_carte.h"
#include "../render/bench_spots.h"
#include "../render/qualite_banc.h"
#include "../game/roues_banc.h"
#include "../render/bench_effets.h"
#include "../render/bench_nuit.h"
#include "../render/bench_realisme.h"   // WFS-148 : banc de référence du réalisme
#include "../render/feux_modele.h"   // WFS-109 : banc des feux arrière
#include "../render/renderer.h"
#include "../render/vk.h"
#include "../ui/ecrans/chargement.h"
#include "manette_jeu.h"   // WFS-22
#include "endurance_banc.h"   // WFS-91
#include "chauffe.h"   // WFS-123
#include "plantage.h"   // WFS-126
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
// WFS-123 (RG-08) : état thermique simulé au banc : WFS_CHAUFFE=0|1|2 (aucune, modérée, sévère), à partir de
// WFS_CHAUFFE_APRES=<s> de course (0 par défaut) ; relu à chaque appel (le banc de la chauffe le change en cours de route)
static int statutChauffeSimule() {
    const char* c = std::getenv("WFS_CHAUFFE");
    const char* a = std::getenv("WFS_CHAUFFE_APRES");
    if (!c || chauffe::tempsCourse() < (a ? std::atof(a) : 0.0)) return 0;
    const int n = std::atoi(c);
    return n >= 2 ? 3 : n == 1 ? 2 : 0;   // statut Android simulé : 3 sévère, 2 modéré, 0 aucun
}
}

using namespace wfs;

// WFS-126 (RG-08) : fin du parcours complet de WFS_BANC=parcours, après celui de WFS-115 (qui s'arrête en course) :
// course → fin → « Menu » (appui au doigt) → accueil ; avec WFS_PLANTAGE=1, plantage volontaire 3 s après l'accueil (RG-05).
// Rend le nombre d'écarts.
static int parcoursFinMenu(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    const char* voiture = std::getenv("WFS_VOITURE") && *std::getenv("WFS_VOITURE") ? std::getenv("WFS_VOITURE") : "golf";   // WFS-202 (RG-09)
    if (!g.init(r, voiture, "ville")) { courseBenchRules(false); WFS_ERR("parcours : chargement impossible (fin de course)"); return 1; }
    g.setAutopilot(false); g.camMode = 0;
    double t = 1700; int n = 0, ecarts = 0;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; if (++n % 4 == 0) g.render(t); };
    auto images = [&](int k) { for (int i = 0; i < k; i++) image(); };
    auto attendu = [&](bool ok, const char* etape) {
        if (ok) WFS_LOG("parcours : %s (attendu)", etape); else { ecarts++; WFS_ERR("parcours : %s → écran %s (ÉCHEC)", etape, nomEcran(g.ecran())); }
    };
    g.allerA(Ecran::Course); g.setAutopilot(true); images(60 * 6);   // décompte puis course en pilote automatique
    attendu(g.ecran() == Ecran::Course && activeCourse() && activeCourse()->state == RaceState::Play, "course en cours");
    ecarts += parcoursPauseRetour(g, t); g.setAutopilot(true); images(60);   // WFS-129 (RG-08) : pause par Retour, « Reprendre »
    g.setAutopilot(false);
    courseBenchSetTime(0.05);
    images(80);   // boutons de l'écran de fin actifs 0,9 s après la fin
    attendu(activeCourse() && activeCourse()->state == RaceState::Over, "course → écran de fin");
    g.render(t += 0.02); g.render(t += 0.02); r.capturePng(out + "/parcours-fin.png");
    // « Menu » : à droite de « Rejouer » (CourseSystem::drawOver, WFS-57), avec ou sans la ligne « RECORD » (WFS-101)
    const float W = (float)r.width(), H = (float)r.height(), u = H / 100, bw = 40 * u, bh = 10 * u;
    const float mx = W / 2 - (bw + 2 * u + bw * 0.7f) / 2 + bw + 2 * u + bw * 0.35f;
    for (float plus : {0.f, 28 * H / 720.f}) {
        if (g.ecran() != Ecran::Course) break;
        const float my = 10 * u + 60 * u + plus + bh / 2;
        g.render(t);
        g.onTouch({40, mx, my, true}); image(); g.onTouch({40, mx, my, false}); images(10);
        WFS_LOG("parcours : écran de fin, appui sur « Menu » à (%.0f, %.0f) → écran %s", mx, my, nomEcran(g.ecran()));
    }
    attendu(g.ecran() == Ecran::Menu, "fin puis « Menu » → accueil");
    images(60);
    g.render(t += 0.02); g.render(t += 0.02); r.capturePng(out + "/parcours-retour-menu.png");
    // RG-05 / RG-08 : déclencheur de test, 3 s après l'accueil
    for (int i = 0; i < 60 * 5 && plantage::declencheurArme(); i++) { image(); plantage::minuterieDeclencheur(1.0 / 60, g.ecran() == Ecran::Menu); }
    courseBenchRules(false);
    g.shutdown();
    return ecarts;
}

int main(int argc, char** argv) {
    std::string out = argc > 1 ? argv[1] : "captures";
    if (argc > 2) g_root = argv[2];
    // WFS-126 : gestionnaire de plantage (plantage.txt dans le dossier de sortie) ; rapport d'un lancement précédent renommé
    plantage::installer(out.c_str(), std::getenv("GITHUB_SHA") ? std::getenv("GITHUB_SHA") : "banc PC (wfs_bench)", "PC, banc d'essai");
    plantage::rapportPrecedent(out.c_str());
    if (const char* p = std::getenv("WFS_PLANTAGE"); p && std::atoi(p) == 1) plantage::armerDeclencheur(true);   // RG-05
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
    if (std::getenv("WFS_CHAUFFE")) chauffe::definirSource(statutChauffeSimule);   // WFS-123 : chauffe simulée pour tout le banc
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
    if (plantage::declencheurArme() && banc != "parcours") {   // WFS-126 (RG-05) : autre groupe avec WFS_PLANTAGE=1 : menu, puis plantage 3 s après
        if (!g.init(r, "golf", "ville")) return 2;
        g.allerA(Ecran::Menu);
        for (int i = 0; i < 60 * 5; i++) { g.update(1.0 / 60); plantage::minuterieDeclencheur(1.0 / 60, g.ecran() == Ecran::Menu); }
    }
    if (banc == "perf") { benchPerf(g, r, out); return finir(); }   // WFS-222 (accroche) : profilage des 4 cartes, Haute et Basse
    if (banc == "hud") { benchHud(g, r, out); return finir(); }   // WFS-27 : HUD seul (2400x1080)
    if (banc == "conduite") { const int e = benchConduite(g, r, out); finir(); return e > 0 ? 3 : 0; }   // WFS-241 (accroche) : banc de conduite, captures conduite-*
    if (banc == "fluidite") { const int e = benchFluidite(g, r, out); finir(); return e > 0 ? 3 : 0; }   // WFS-221 (accroche) : à-coups de la voiture vue par la caméra
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
        benchVitrinesFeux(g, r, out);   // WFS-170 (RG-08) : vitrine-<id>-feux des 20 voitures, feux stop allumés
        benchPlancheVoitures(g, r, out); if (benchRecords(g, r, out) > 0) { finir(); return 3; }   // WFS-77 : les 20 voitures en Turquoise, roues et peinture au journal ; WFS-125 : records (job vitrines de la CI)
        return finir();
    }
    if (banc == "vitrines-feux") { benchVitrinesFeux(g, r, out); return finir(); }   // WFS-170 : feux des voitures seuls (WFS_VOITURES : essai local)
    if (banc == "route") { benchRoute(g, r, out); return finir(); }   // WFS-142 : chaussée sèche / mouillée (essai local)
    if (banc == "nuit") { benchNuit(g, r, out); benchFeuxFrein(g, r, out); benchRoute(g, r, out); benchCockpit(g, r, out, COCKPIT_LOT2A); conducteurs::benchConducteurs(g, r, out); return finir(); }   // WFS-73 (accroche) : conducteurs-*   // WFS-201 (accroche) : cockpit lot 2, 1re moitié   // WFS-109 : feux-frein-jour / -nuit, feux-recul   // WFS-09 / WFS-16 : nuit, phares, lampadaires, route mouillée (course-nuit-*, job nuit de la CI)
    if (banc == "qualite") { ui::chargerPolice(r); benchQualite(g, r, out); ecrans::benchGarage(g, r, out); return finir(); }   // WFS-104 : course vs garage (essai local)
    if (banc == "endurance") { const int code = benchEndurance(vk, r, g, out, cw, chh); finir(); return code; }   // WFS-91 : endurance
    if (banc == "parcours") {   // WFS-115 ; WFS-126 (RG-08) : parcours complet jusqu'au retour à l'accueil
        ui::chargerPolice(r);
        int e = benchParcours(g, r, out) + parcoursFinMenu(g, r, out);
        WFS_LOG("parcours complet : %d écart(s)%s", e, e ? " ÉCHEC" : " OK");
        {   // WFS-197 (accroche, RG-09) : même parcours en vue Cockpit (intérieur de la voiture choisie), captures dans cockpit/
            const VueCamera v = vueCamera(); choisirVue(VueCamera::Cockpit); std::filesystem::create_directories(out + "/cockpit");
            const int ec = benchParcours(g, r, out + "/cockpit") + parcoursFinMenu(g, r, out + "/cockpit");
            WFS_LOG("parcours complet en vue Cockpit (WFS-197) : %d écart(s)%s", ec, ec ? " ÉCHEC" : " OK"); choisirVue(v); e += ec;
        }
        finir(); return e > 0 ? 3 : 0;
    }
    if (banc == "trace") { ui::chargerPolice(r); const int e = trace::benchTrace(g, r, out); finir(); return e > 0 ? 3 : 0; }   // WFS-136 (accroche) : tracé 2
    if (banc == "chauffe") { ui::chargerPolice(r); const int e = chauffe::benchChauffe(g, r, out, statutChauffeSimule); finir(); return e > 0 ? 3 : 0; }   // WFS-123
    if (banc == "records") { ui::chargerPolice(r); const int e = benchRecords(g, r, out); finir(); return e > 0 ? 3 : 0; }   // WFS-125 : records par mode, fantôme (aussi à la fin du groupe vitrines)
    if (banc == "course5") { ui::chargerPolice(r); benchCourse5(g, r, out); benchPoursuite(g, r, out); if (benchCourse5Rejouer(g, r, out) > 0) { finir(); return 3; } return finir(); }   // WFS-119 : ANO-53   // WFS-110 : modes, course à 5 ; WFS-111 : poursuite
    if (banc == "vie") { ui::chargerPolice(r); benchVie(g, r, out); return finir(); }   // WFS-71 : terrasse, enseignes, portique, plage
    if (banc == "garage") { ecrans::benchGarage(g, r, out); return finir(); }   // WFS-11 : garage seul (essai local)
    if (banc == "camera") { benchCameras(g, r, out); return finir(); }   // WFS-66 : caméras seules (2400x1080 : job ecrans-2400)
    if (banc == "cockpit") {   // WFS-197 / WFS-201 (accroche) : intérieurs par modèle seuls (les 20)
        std::vector<std::string> v = COCKPIT_LOT1; v.insert(v.end(), COCKPIT_LOT2A.begin(), COCKPIT_LOT2A.end()); v.insert(v.end(), COCKPIT_LOT2B.begin(), COCKPIT_LOT2B.end());
        v.insert(v.end(), COCKPIT_LOT3A.begin(), COCKPIT_LOT3A.end()); v.insert(v.end(), COCKPIT_LOT3B.begin(), COCKPIT_LOT3B.end());   // WFS-202
        if (const char* l = std::getenv("WFS_VOITURES")) { v.clear(); std::string t = l; size_t p0 = 0; for (size_t p; (p = t.find(',', p0)) != std::string::npos; p0 = p + 1) v.push_back(t.substr(p0, p - p0)); v.push_back(t.substr(p0)); }   // essai local
        benchCockpit(g, r, out, v); return finir();
    }
    if (banc == "conducteurs") { conducteurs::benchConducteurs(g, r, out); return finir(); }   // WFS-73 (accroche) : conducteurs vus de côté
    if (banc == "squelette") { benchSquelette(g, r, out); return finir(); }   // WFS-63 : personnages animés seuls
    if (banc == "rondspoints") { const int e = benchRondsPoints(g, r, out); finir(); return e > 0 ? 3 : 0; }   // WFS-223
    if (banc == "campagne") { const int e = benchCampagne(g, r, out); finir(); return e > 0 ? 3 : 0; }   // WFS-138 : col, épingle, vignes
    if (banc == "cartes") { benchCartes(g, r, out); return finir(); }         // WFS-53 : cartes Campagne et Grand Prix seules
    if (banc == "ville") { benchVille(g, r, out); return finir(); }           // WFS-137 : lieux de la Ville seuls (essai local)
    if (banc == "corniche") { benchCorniche(g, r, out); return finir(); }   // WFS-139 : Corniche au tracé 2 seule (essai local)
    if (banc == "ressenti") { benchRessenti(g, r, out); return finir(); }   // WFS-70 : ressenti de conduite seul
    if (banc == "sauvegarde") {   // WFS-24 : à lancer deux fois ; chemin propre au banc (WFS_SAUVEGARDE pour le forcer)
        benchSauvegardeScene(g, r, out, std::getenv("WFS_SAUVEGARDE") ? std::getenv("WFS_SAUVEGARDE") : out + "/sauvegarde-banc.txt"); return finir();
    }
    if (banc == "manette") { manette::benchManette(g, r, out); return finir(); }   // WFS-22 : manettes (job texte)
    if (banc == "pietons") { benchPietons(g, r, out); return finir(); }       // WFS-19 : piétons seuls
    if (banc == "realisme") { const int e = benchRealisme(g, r, out); finir(); return e > 0 ? 3 : 0; }   // WFS-148 : 12 vues, étalonnage, rapports
    const bool menu1 = banc == "menu" || banc == "menu1", menu2 = banc == "menu" || banc == "menu2";   // WFS-106 : 2400x1080 en deux jobs
    if (menu1) { ecrans::benchMenu(g, r, out); ecrans::benchMenuTactile(g, r, out); ecrans::benchPause(g, r, out); ecrans::benchGarage(g, r, out);
        ecrans::benchTerrain(g, r, out); }
    // menu2 seul (job à part) : record de session que la course « fin-menu » de menu1 lui laissait (950 à 955 selon la
    // machine), sinon reglages-menu et menu-duel montreraient le menu sans record (RG-08 : mêmes captures)
    if (menu2 && !menu1) sauvegarde().best = 955;
    if (menu2) { ecrans::benchReglages(g, r, out); ecrans::benchSalon(g, r, out); }
    if (menu1 && benchParcours(g, r, out) > 0) { finir(); return 3; }   // WFS-115 : parcours menu → mode → garage → terrain → course
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
        benchCockpit(g, r, out, COCKPIT_LOT2B);   // WFS-201 (accroche) : cockpit lot 2, 2e moitié (Gallardo, Mustang, Model 3, X5)
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
        benchCockpit(g, r, out, COCKPIT_LOT3A);   // WFS-202 (accroche) : cockpit lot 3, 1re moitié (Abarth, Compass, Elantra, GT86)
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
        benchCockpit(g, r, out, COCKPIT_LOT3B);   // WFS-202 (accroche) : cockpit lot 3, 2e moitié (Range Rover Sport, T-Cross, V60)
    }
    if (partie2c) {
        benchSpots(r, out);           // WFS-45 : projecteurs, hémisphère, brouillard linéaire (spots-sol.png)
        benchSquelette(g, r, out);    // WFS-63 : personnages animés par squelette (squelette-*.png)
        benchCameras(g, r, out);      // WFS-66 : vues poursuite, capot, cockpit (camera-*.png)
        benchCockpit(g, r, out, COCKPIT_LOT1);   // WFS-197 (accroche) : intérieurs Golf, M4, 911, SF90, Corsa (cockpit-*.png)
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
