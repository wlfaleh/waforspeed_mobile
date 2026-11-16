// WaForSpeed — banc d'essai de WFS-561 (WFS_BANC=resultats3d, toute taille avec WFS_TAILLE) : scène 3D des résultats avec des
// résultats posés à la main (les vraies courses des 3 modes sont jouées par les bancs des modes une fois les accroches posées) :
//   1. resultats3d-course5-1er : maquette 11 (Golf bleue, le joueur 1er, 5 pilotes, temps et écarts) ;
//   2. resultats3d-course5-3e : le joueur 3e, deux pilotes pas arrivés (distance au lieu du temps) ;
//   3. resultats3d-duel : 2 lignes ; 4. resultats3d-poursuite : toi et les polices (lignes vides cachées) ;
//   5. les mêmes en anglais (-en) ; 6. resultats3d-nuit : partie de nuit (souterrain : même image) ;
//   7. resultats3d-panneau : gros plan du panneau (caisson, visières, points) pour l'avis de l'agent 3D ;
//   8. repli : mémoire graphique refusée au chargement → scène non prête, aucun plantage ;
//   9. mesures : triangles du décor, durée d'une image.
// Contrôles écrits au journal « resultats3d (WFS-561) : OK / ÉCART … » ; valeur rendue = nombre d'écarts.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "../core/log.h"
#include "../game/game.h"
#include "../game/sauvegarde.h"
#include "../game/course.h"
#include "../game/course5.h"
#include "../game/course_system.h"
#include "../game/duel_system.h"
#include "../game/poursuite.h"
#include "../game/boutique.h"
#include "../render/renderer.h"
#include "langue.h"
#include "resultats_parking.h"
#include "text.h"

namespace wfs::resultats3d {

int benchResultats(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    auto verifier = [&](bool ok, const std::string& quoi) { WFS_LOG("resultats3d (WFS-561) : %s : %s", ok ? "OK" : "ÉCART", quoi.c_str()); if (!ok) ecarts++; };
    const std::string taille = "-" + std::to_string(r.width()) + "x" + std::to_string(r.height());
    const bool reference = r.width() == 2400 && r.height() == 1080;
    ui::chargerPolice(r);
    const std::string chemin = out + "/resultats3d-sauvegarde-banc.txt";
    std::remove(chemin.c_str());
    Sauvegarde& s = sauvegarde();
    s.charger(chemin);
    s.car = "golf"; s.paint["golf"] = "bleu";
    s.profil.cree = true; s.nick = "Lume"; s.profil.plaque = "WFS";
    s.enregistrer();
    Tableau tab;
    double t = 6000;
    auto image = [&]() { demander(tab); g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    auto capture = [&](const std::string& nom, const char* quoi) {
        for (int i = 0; i < 3; i++) image();
        r.capturePng(out + "/resultats3d" + taille + "-" + nom + ".png");
        WFS_LOG("capture resultats3d%s-%s : %s (scène %s)", taille.c_str(), nom.c_str(), quoi, affiche() ? "3D" : "absente");
    };
    auto partie = [&](const char* voiture) {
        if (!g.init(r, voiture, "ville")) { WFS_ERR("resultats3d : chargement impossible"); return false; }
        g.setAutopilot(false); g.camMode = 0;
        g.allerA(Ecran::Course);
        for (int i = 0; i < 30; i++) g.update(1.0 / 60);
        for (int i = 0; i < 10; i++) image();
        return true;
    };
    auto toi = [&]() { return ui::tr("resultats.toi", "LUME"); };
    auto titre = [&](const char* ordinalCle, const char* temps) { return ui::tr("resultats.titre", ui::tr(ordinalCle), temps); };
    auto jeux = [&](const std::string& suffixe) {
        const bool fr = suffixe.empty();
        const char* d = fr ? "," : ".";
        auto tm = [&](const char* m) { std::string x = m; for (char& c : x) if (c == ',') c = *d; return x; };
        tab.titre = titre("resultats.rang1", tm("2:05,4").c_str());
        tab.lignes = {{"1", toi(), tm("2:05,4"), "—", true}, {"2", "NADIA", tm("2:06,9"), tm("+1,5")}, {"3", "VANTOR GT", tm("2:08,2"), tm("+2,8")},
                      {"4", "SELENE TS", tm("2:11,0"), tm("+5,6")}, {"5", "SF-X", tm("2:13,7"), tm("+8,3")}};
        capture("course5-1er" + suffixe, "course à 5, le joueur 1er (maquette 11)");
        verifier(affiche(), "course à 5 (1er) : scène 3D affichée" + suffixe);
        tab.titre = titre("ordinal.3", tm("2:09,8").c_str());
        tab.lignes = {{"1", "NADIA", tm("2:06,9"), "—"}, {"2", "VANTOR GT", tm("2:08,2"), tm("+1,3")}, {"3", toi(), tm("2:09,8"), tm("+2,9"), true},
                      {"4", "SELENE TS", tm("1,76 KM"), "—"}, {"5", "SF-X", tm("1,62 KM"), "—"}};
        capture("course5-3e" + suffixe, "course à 5, le joueur 3e, deux pilotes pas arrivés (distance)");
        tab.titre = titre("resultats.rang1", tm("1:12,0").c_str());
        tab.lignes = {{"1", toi(), tm("1:12,0"), "—", true}, {"2", "MARCO", tm("1:13,1"), tm("+1,1")}};
        capture("duel" + suffixe, "duel : 2 lignes");
        tab.titre = ui::tr("resultats.titre", ui::tr("fin.victoire"), tm("0:10,2").c_str());
        tab.lignes = {{"1", toi(), tm("0:10,2"), "—", true}, {"", "", "", ""}, {"2", fr ? "POLICE 1" : "POLICE 1", fr ? "SEMÉE" : "LOST", "—"},
                      {"3", "POLICE 2", fr ? "SEMÉE" : "LOST", "—"}};
        capture("poursuite" + suffixe, "Poursuite : toi et les polices, ligne vide cachée");
    };
    // ---- 1 à 4 : français
    ui::langue::choisir(ui::langue::Choix::Francais);
    if (!partie("golf")) return 1;
    verifier(prete(), "scène des résultats préparée au chargement (RG-C5)");
    const uint64_t m0 = r.creationsMaillages(), mat0 = r.creationsMateriaux();
    jeux("");
    verifier(r.creationsMaillages() == m0 && r.creationsMateriaux() == mat0, "rien n'est créé pendant l'écran des résultats (" +
             std::to_string(r.creationsMaillages() - m0) + " maillage(s), " + std::to_string(r.creationsMateriaux() - mat0) + " matériau(x))");
    // ---- 7 : gros plan du panneau
    banc::panneau = true;
    tab.titre = titre("resultats.rang1", "2:05,4");
    tab.lignes = {{"1", ui::tr("resultats.toi", "LUME"), "2:05,4", "—", true}, {"2", "NADIA", "2:06,9", "+1,5"}, {"3", "VANTOR GT", "2:08,2", "+2,8"},
                  {"4", "SELENE TS", "2:11,0", "+5,6"}, {"5", "SF-X", "2:13,7", "+8,3"}};
    capture("panneau", "gros plan du panneau : caisson, cadre, visières, points ambre");
    banc::panneau = false;
    // ---- 9 : mesures
    {
        using H = std::chrono::steady_clock;
        const auto a = H::now(); for (int i = 0; i < 20; i++) image();
        const double ms = std::chrono::duration<double, std::milli>(H::now() - a).count() / 20;
        WFS_LOG("resultats3d (WFS-561) : image de l'écran des résultats %.1f ms en moyenne (banc PC, rendu logiciel)", ms);
    }
    // ---- effacer : retour à la course normale
    effacer(); g.update(1.0 / 60); g.render(t += 0.02);
    verifier(!affiche(), "résultats effacés : plus de scène 3D");
    g.shutdown();
    // ---- 5 : anglais
    if (reference) {
        ui::langue::choisir(ui::langue::Choix::Anglais);
        if (partie("golf")) jeux("-en");
        ui::langue::choisir(ui::langue::Choix::Francais);
        g.shutdown();
    }
    // ---- 6 : nuit
    if (reference) {
        setenv("WFS_MOMENT", "nuit", 1);
        if (partie("golf")) {
            tab.titre = titre("resultats.rang1", "2:05,4");
            tab.lignes = {{"1", ui::tr("resultats.toi", "LUME"), "2:05,4", "—", true}, {"2", "NADIA", "2:06,9", "+1,5"}, {"3", "VANTOR GT", "2:08,2", "+2,8"},
                          {"4", "SELENE TS", "2:11,0", "+5,6"}, {"5", "SF-X", "2:13,7", "+8,3"}};
            capture("nuit", "partie de nuit : même parking (souterrain)");
        }
        unsetenv("WFS_MOMENT");
        g.shutdown();
    }
    // ---- 8 : repli
    if (reference) {
        r.vk().echecReservation = r.vk().reservationsRepli + 1;
        if (partie("golf")) {
            verifier(!prete(), "mémoire graphique refusée : scène non prête (les modes gardent leur fenêtre 2D)");
            capture("repli", "mémoire graphique refusée : pas de scène 3D, aucun plantage");
            verifier(!affiche(), "repli : la scène ne prend pas l'image");
        }
        r.vk().echecReservation = 0;
        g.shutdown();
    }
    std::remove(chemin.c_str());
    WFS_LOG("resultats3d (WFS-561) : %d écart(s)", ecarts);
    return ecarts;
}

// banc WFS_BANC=resultats3d-modes : vraies courses des 3 modes (pilote automatique, Golf possédée), FR puis EN : course à 5 gagnée
// et finie 3e, Poursuite échappée et arrêtée, duel gagné (salon en mémoire) ; puis le repli (WFS_RESULTATS_SANS_3D : fenêtres 2D
// des maquettes 31 à 33). Captures resultats3d-modes-<fr|en>-<cas>-<L>x<H> ; rend le nombre d'écarts.
int benchResultatsModes(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    auto verifier = [&](bool ok, const std::string& quoi) { WFS_LOG("resultats3d-modes (WFS-561) : %s : %s", ok ? "OK" : "ÉCART", quoi.c_str()); if (!ok) ecarts++; };
    const std::string taille = std::to_string(r.width()) + "x" + std::to_string(r.height());
    ui::chargerPolice(r);
    const std::string chemin = out + "/resultats3d-modes-sauvegarde.txt";
    std::remove(chemin.c_str());
    Sauvegarde& s = sauvegarde();
    s.charger(chemin);
    for (int i = 0; i < CAR_COUNT; i++) s.voituresPossedees.insert(CARS[i].id);   // Golf possédée : pas d'essai gratuit (pas de garage à la fin)
    s.car = "golf";
    { std::string nick; s.profil.creer("Lume", "WFS", nick); s.nick = nick; }
    s.enregistrer();
    double t = 9000;
    auto images = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); } };
    auto finie = [&]() { const Course* c = activeCourse(); return c && c->state == RaceState::Over; };
    auto jusquaFin = [&](int n) { for (int i = 0; i < n && !finie(); i++) { g.update(1.0 / 60); t += 1.0 / 60; } images(90); };
    auto capture = [&](const std::string& nom, const char* quoi, bool scene3d) {
        images(3);
        r.capturePng(out + "/resultats3d-modes-" + nom + "-" + taille + ".png");
        WFS_LOG("capture resultats3d-modes-%s-%s : %s (scène %s)", nom.c_str(), taille.c_str(), quoi, affiche() ? "3D" : "2D");
        verifier(affiche() == scene3d, nom + std::string(" : ") + (scene3d ? "scène 3D des résultats" : "fenêtre 2D (repli)"));
    };
    auto demarrer = [&](ModeCourse m) {
        modeCourse() = m; s.mode = poursuite::cleMode(m); s.km5 = kmCourse5();
        if (!g.init(r, "golf", "ville")) { verifier(false, std::string("chargement ") + nomMode(m)); return false; }
        g.setAutopilot(true); g.camMode = 0; g.allerA(Ecran::Course);
        for (int i = 0; i < 8 * 60; i++) { g.update(1.0 / 60); t += 1.0 / 60; }
        return true;
    };
    auto course5 = [&](const std::string& nom, const std::vector<std::pair<double, int>>& tenus, const char* quoi, bool scene3d) {
        kmCourse5() = 2;
        if (!demarrer(ModeCourse::Course5)) return;
        course5BancSuiveurs(tenus);
        for (int i = 0; i < 150 * 60 && !finie(); i++) {
            const Course* c = activeCourse();
            if (c && c->duelFinish > 0 && g.player().dist > c->duelFinish - 40 / M) course5BancSuiveurs({});
            g.update(1.0 / 60); t += 1.0 / 60;
        }
        course5BancSuiveurs({});
        jusquaFin(60);
        verifier(finie(), nom + " : course à 5 finie");
        capture(nom, quoi, scene3d);
        float x, y;
        verifier(course5Bouton("rejouer", x, y) && course5Bouton("menu", x, y), nom + " : « Rejouer » et « Menu » posés");
        g.shutdown();
    };
    auto poursuiteFin = [&](const std::string& nom, int mode, const char* quoi, bool scene3d) {
        poursuiteBancFixe(true);
        if (demarrer(ModeCourse::Poursuite)) {
            poursuiteBancFin(mode); jusquaFin(60 * 60);
            verifier(finie(), nom + " : Poursuite finie");
            capture(nom, quoi, scene3d);
            g.shutdown();
        }
        poursuiteBancFixe(false);
    };
    auto duel = [&](const std::string& nom, bool scene3d) {
        bool vu = false;
        ecarts += benchDuelTextes(g, r, [&](const std::string& n) {
            if (n != "duel-fin") return;
            r.capturePng(out + "/resultats3d-modes-" + nom + "-" + taille + ".png");
            WFS_LOG("capture resultats3d-modes-%s-%s : duel gagné (scène %s)", nom.c_str(), taille.c_str(), affiche() ? "3D" : "2D");
            verifier(affiche() == scene3d, nom + " : duel");
            vu = true;
        }, true);
        verifier(vu, nom + " : écran de fin du duel vu");
    };
    courseBenchRules(true);
    for (const ui::langue::Choix lg : {ui::langue::Choix::Francais, ui::langue::Choix::Anglais}) {
        ui::langue::choisir(lg);
        const std::string c = ui::langue::code();
        course5(c + "-course5-1er", {{-25, 0}, {-32, 1}, {-39, 2}, {-46, 1}}, "course à 5 gagnée (maquette 11)", true);
        if (c == "fr") course5(c + "-course5-3e", {{30, 0}, {18, 1}, {-30, 2}, {-40, 1}}, "course à 5 finie 3e (deux adversaires derrière)", true);
        poursuiteFin(c + "-poursuite-echappe", 2, "Poursuite échappée (saut à 150 m de l'arrivée)", true);
        if (c == "fr") poursuiteFin(c + "-poursuite-arrete", 1, "Poursuite perdue (arrêté)", true);
        duel(c + "-duel", true);
    }
    // repli (RG-08) : scène refusée → les fenêtres 2D des maquettes 31 à 33, inchangées
    ui::langue::choisir(ui::langue::Choix::Francais);
    setenv("WFS_RESULTATS_SANS_3D", "1", 1);
    course5("fr-repli-course5", {{-25, 0}, {-32, 1}, {-39, 2}, {-46, 1}}, "repli : fenêtre 2D de la course à 5 (maquette 31)", false);
    unsetenv("WFS_RESULTATS_SANS_3D");
    courseBenchRules(false);
    modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4;
    ui::langue::choisir(ui::langue::Choix::Automatique);
    std::remove(chemin.c_str());
    WFS_LOG("resultats3d-modes (WFS-561) : %d écart(s)", ecarts);
    return ecarts;
}

} // namespace wfs::resultats3d
