// WaForSpeed — banc d'essai de WFS-552 (WFS_BANC=menu3d, toute taille avec WFS_TAILLE) : le menu sur le parking souterrain.
//   1. menu3d-sfx : Cavallo SF-X orange, finition Caméléon, skin Bleu ciel (exemple de la maquette 01), caméra rapprochée ;
//   2. menu3d-angle-1 à -3 : gros plan sous trois angles (de face, trois quarts, flanc) : orange de face, bleu sur les côtés ;
//   3. menu3d-decor : vue du décor seul (poteau, poutres, plafond, réglettes, sprinklers, sol) pour l'avis de l'agent 3D ;
//   4. menu3d-nuit : même menu, partie de nuit (RG-06 : souterrain, même image) ;
//   5. menu3d-golf : une autre voiture du joueur (Golf, peinture d'origine) : le menu montre la voiture choisie (RG-03) ;
//   6. parcours (RG-06) : menu → garage → retour au menu → course → retour au menu, écran de chaque étape au journal ;
//   7. repli (RG-05) : mémoire graphique refusée au chargement → menu sur le fond d'avant, sans plantage ;
//   8. mesures (RG-05) : triangles et appels du décor, durée d'une image du menu (avant et après le fond figé).
// Contrôles écrits au journal « menu3d (WFS-552) : OK / ÉCART … » ; valeur rendue = nombre d'écarts.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "../core/log.h"
#include "../game/game.h"
#include "../game/sauvegarde.h"
#include "../render/renderer.h"
#include "menu_parking.h"
#include "text.h"

namespace wfs::ecrans {

int benchMenuParking(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    auto verifier = [&](bool ok, const std::string& quoi) { WFS_LOG("menu3d (WFS-552) : %s : %s", ok ? "OK" : "ÉCART", quoi.c_str()); if (!ok) ecarts++; };
    const std::string taille = "-" + std::to_string(r.width()) + "x" + std::to_string(r.height());
    const bool reference = r.width() == 2400 && r.height() == 1080;   // parcours, nuit, repli : une seule fois (2400 × 1080)
    ui::chargerPolice(r);
    // sauvegarde du banc : la SF-X orange caméléon bleu ciel possédée (comme le joueur qui l'a achetée au garage)
    const std::string chemin = out + "/menu3d-sauvegarde-banc.txt";
    std::remove(chemin.c_str());
    Sauvegarde& s = sauvegarde();
    s.charger(chemin);
    for (int i = 0; i < CAR_COUNT; i++) s.voituresPossedees.insert(CARS[i].id);
    s.car = "sf90"; s.paint["sf90"] = "orange"; s.finition["sf90"] = "cameleon"; s.skin["sf90"] = "ciel";
    s.possede["sf90"].insert({"orange", "cameleon", "ciel"});
    s.profil.cree = true; s.nick = "Pilote"; s.profil.plaque = "WF-552-SF";   // profil déjà créé : le menu, pas « Crée ton pilote »
    s.enregistrer();
    double t = 6000;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    auto capture = [&](const std::string& nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/menu3d" + taille + "-" + nom + ".png");
        WFS_LOG("capture menu3d%s-%s : %s (écran %s)", taille.c_str(), nom.c_str(), quoi, nomEcran(g.ecran()));
    };
    // le fond du menu se fige après 30 images (WFS-329) : un passage par le garage le fait redessiner (changement d'angle du banc)
    auto redessiner = [&]() { g.allerA(Ecran::Garage); image(); image(); g.allerA(Ecran::Menu); for (int i = 0; i < 6; i++) image(); };
    auto menu = [&](const char* voiture, const char* carte) {
        if (!g.init(r, voiture, carte)) { WFS_ERR("menu3d : chargement impossible"); return false; }
        g.setAutopilot(false); g.camMode = 0;
        for (int i = 0; i < 30; i++) g.update(1.0 / 60);
        g.allerA(Ecran::Menu);
        for (int i = 0; i < 40; i++) image();   // menu posé (fondu d'entrée de 300 ms, lumières)
        return true;
    };
    // ---- 1 à 3 : SF-X de la maquette
    if (!menu("sf90", "ville")) return 1;
    capture("sfx", "SF-X orange Caméléon, skin Bleu ciel, parking souterrain, caméra rapprochée");
    {
        const parking::Cadrage c = parking::cadrage(4.85f, 1.97f, 1.19f, (float)r.width() / (float)r.height());
        const parking::Emprise e = parking::emprise(c, 4.85f, 1.97f, 1.19f, (float)r.width() / (float)r.height());
        verifier(e.xmin > -0.40f && e.xmax < 0.95f && e.ymin > -0.95f && e.ymax < 0.95f,
                 "voiture entière dans l'image, à droite de la console (x NDC " + std::to_string(e.xmin) + " à " + std::to_string(e.xmax) + ", y " +
                 std::to_string(e.ymin) + " à " + std::to_string(e.ymax) + ")");
    }
    const float caps[3] = {0.10f, 0.42f, 1.20f};
    for (int k = 0; k < 3; k++) {
        parking::banc::cap = caps[k];
        redessiner();
        capture("angle-" + std::to_string(k + 1), k == 0 ? "gros plan de face (orange)" : k == 1 ? "trois quarts (orange devant, bleu sur le flanc)" : "flanc (bleu)");
    }
    parking::banc::cap = -1;
    parking::banc::decor = true;
    redessiner();
    capture("decor", "décor seul : poteau à bande de danger, poutres, prédalles, réglettes, sprinklers, marquage, butées");
    parking::banc::decor = false;
    // ---- 8 : mesures (RG-05)
    {
        using H = std::chrono::steady_clock;
        auto mesurer = [&](int n) { const auto a = H::now(); for (int i = 0; i < n; i++) image(); return std::chrono::duration<double, std::milli>(H::now() - a).count() / n; };
        g.allerA(Ecran::Garage); for (int i = 0; i < 5; i++) image();
        g.allerA(Ecran::Menu);
        const double avant = mesurer(20), apres = (mesurer(20), mesurer(30));
        WFS_LOG("menu3d (WFS-552) : image du menu %.1f ms en moyenne avant le fond figé, %.1f ms après (banc PC, rendu logiciel)", avant, apres);
        // rendu logiciel (lavapipe) : en 1280 × 720, la mesure du banc ; en 2400 × 1080, la passe finale seule (5 Mpx sur le
        // processeur) dépasse 33 ms sur main comme ici : journal seulement
        if (r.width() == 1280) verifier(apres < 1000.0 / 30, "menu ≥ 30 i/s une fois posé (" + std::to_string(1000.0 / apres) + " i/s)");
    }
    // ---- 6 : parcours (RG-06)
    if (reference) {
        g.allerA(Ecran::Garage); for (int i = 0; i < 40; i++) image();
        verifier(g.ecran() == Ecran::Garage, std::string("menu → garage : écran ") + nomEcran(g.ecran()));
        capture("parcours-garage", "garage ouvert depuis le menu (décor du garage)");
        g.allerA(Ecran::Menu); for (int i = 0; i < 40; i++) image();
        verifier(g.ecran() == Ecran::Menu, std::string("garage → menu : écran ") + nomEcran(g.ecran()));
        capture("parcours-retour", "retour au menu : parking et SF-X");
        g.allerA(Ecran::Course); for (int i = 0; i < 120; i++) image();
        verifier(g.ecran() == Ecran::Course, std::string("menu → course : écran ") + nomEcran(g.ecran()));
        capture("parcours-course", "course : la ville de nouveau (aucun parking en course)");
        g.allerA(Ecran::Menu); for (int i = 0; i < 40; i++) image();
        verifier(g.ecran() == Ecran::Menu, std::string("course → menu : écran ") + nomEcran(g.ecran()));
        capture("parcours-fin", "retour au menu après la course : parking");
    }
    g.shutdown();
    // ---- 4 : nuit (RG-06)
    if (reference) {
        setenv("WFS_MOMENT", "nuit", 1);
        if (menu("sf90", "ville")) capture("nuit", "partie de nuit : même parking (souterrain)");
        unsetenv("WFS_MOMENT");
        g.shutdown();
    }
    // ---- 5 : autre voiture (RG-03)
    s.car = "golf"; s.enregistrer();
    if (menu("golf", "ville")) capture("golf", "Golf du joueur (peinture d'origine) : le menu montre la voiture choisie");
    g.shutdown();
    // ---- 7 : repli (RG-05)
    if (reference) {
        r.vk().echecReservation = r.vk().reservationsRepli + 1;   // toute réservation de la voie de repli refusée à partir d'ici
        if (menu("golf", "ville")) {
            verifier(g.ecran() == Ecran::Menu, "mémoire graphique refusée : menu affiché sans plantage (fond d'avant)");
            capture("repli", "mémoire graphique refusée : menu sur le fond d'avant");
        }
        r.vk().echecReservation = 0;
        g.shutdown();
    }
    std::remove(chemin.c_str());
    WFS_LOG("menu3d (WFS-552) : %d écart(s)", ecarts);
    return ecarts;
}

} // namespace wfs::ecrans
