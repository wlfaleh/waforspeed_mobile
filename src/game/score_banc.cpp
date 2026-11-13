// WaForSpeed — banc des étoiles du score en fin de course (WFS-536, épic E22) : WFS_BANC=score-etoiles, voir boutique.h.
// Vraie partie au banc d'essai PC, sauvegarde active dans un fichier propre au banc (effacé au début) :
//   1. course à 5 (Ville, 2 km, pilote automatique) jusqu'à l'arrivée : solde avant / après, ★ du score notées dans l'historique ;
//   2. retour au menu après l'arrivée (écran de fin quitté) : aucun 2e crédit (RG-05) ;
//   3. course à 5 quittée après le départ : 0 ★, solde inchangé (RG-01).
// Captures score-course5-fin (écran de fin) et score-quittee (menu après la course quittée) ; texte <out>/score-etoiles.txt.
// La sauvegarde redevient inactive à la fin (les scènes suivantes gardent leurs captures).
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../render/renderer.h"
#include "boutique.h"
#include "course5.h"
#include "course_system.h"
#include "game.h"
#include "sauvegarde.h"

namespace wfs::boutique {

int benchScoreEtoiles(Game& g, Renderer& r, const std::string& out) {
    std::error_code ec; std::filesystem::create_directories(out, ec);
    const std::string chemin = out + "/score-banc.txt";
    std::filesystem::remove(chemin, ec);
    Sauvegarde& s = sauvegarde();
    s.charger(chemin);   // partie neuve, sauvegarde active : la fin de course est notée et créditée
    s.mode = "course5"; s.km5 = 2;   // le mode est relu de la sauvegarde active (modes_ecran.cpp, WFS-24) : course à 5, 2 km
    courseBenchRules(true);
    modeCourse() = ModeCourse::Course5; kmCourse5() = 2;
    int ecarts = 0;
    std::vector<std::string> lignes;
    auto noter = [&](const std::string& l) { WFS_LOG("%s", l.c_str()); lignes.push_back(l); };
    auto ecart = [&](const std::string& l) { ecarts++; WFS_ERR("SCORE ÉTOILES ÉCART : %s", l.c_str()); lignes.push_back("ÉCART : " + l); };
    if (!g.init(r, "golf", "ville")) {
        courseBenchRules(false); modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4; s.desactiver();
        WFS_ERR("score-etoiles : partie non lancée");
        return 1;
    }
    double t = 900;
    auto images = [&](int n, int tous) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); if (i % tous == 0) g.render(t += tous / 60.0); } };
    auto capture = [&](const char* nom) { g.render(t += 0.02); g.render(t += 0.02); r.capturePng(out + "/" + nom + ".png"); };

    // ---- 1. course à 5 terminée
    g.allerA(Ecran::Menu); images(10, 5);
    const long long avant = boutique::solde(s);
    noter("SCORE ÉTOILES : partie neuve, solde avant la course à 5 : " + std::to_string(avant) + " ★");
    g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
    const Course* c = activeCourse();
    for (int i = 0; i < 150 * 60 && c && c->state != RaceState::Over; i++) { g.update(1.0 / 60); c = activeCourse(); }
    images(90, 10);   // le profil note la course au plus 0,5 s après l'arrivée
    if (!c || c->state != RaceState::Over) ecart("course à 5 pas finie");
    const long long apres = boutique::solde(s);
    const CourseJouee* derniere = s.profil.dernieres.empty() ? nullptr : &s.profil.dernieres[0];
    char b[256];
    if (derniere) {
        std::snprintf(b, sizeof b, "SCORE ÉTOILES : course à 5 terminée : %s, %d ★ gagnées ; solde après : %lld ★ (+%lld ★)",
                      derniere->resultat().c_str(), derniere->etoiles, apres, apres - avant);
        noter(b);
        if (derniere->etoiles < config().scoreEtoiles.etoilesMin || derniere->etoiles > config().scoreEtoiles.etoilesMax)
            ecart("★ de la course hors de " + std::to_string(config().scoreEtoiles.etoilesMin) + " à " + std::to_string(config().scoreEtoiles.etoilesMax));
        if (apres - avant != derniere->etoiles) ecart("solde augmenté de " + std::to_string(apres - avant) + " ★ au lieu des ★ du score");
        if (derniere->etoiles == config().boutique.gainCourse) noter("SCORE ÉTOILES : (information) ★ du score égales au secours gain_course : voir le journal « score : »");
    } else {
        ecart("course non notée au profil");
    }
    capture("score-course5-fin");

    // ---- 2. écran de fin quitté (retour au menu) : aucun 2e crédit
    g.allerA(Ecran::Menu); images(30, 5);
    const long long apresMenu = boutique::solde(s);
    noter("SCORE ÉTOILES : retour au menu après l'arrivée : solde " + std::to_string(apresMenu) + " ★ (aucun 2e crédit attendu)");
    if (apresMenu != apres) ecart("2e crédit au retour au menu");

    // ---- 3. course quittée après le départ : 0 ★
    g.allerA(Ecran::Course); g.setAutopilot(true);
    c = activeCourse();
    for (int i = 0; i < 20 * 60 && c && c->state != RaceState::Play; i++) { g.update(1.0 / 60); c = activeCourse(); }
    images(8 * 60, 30);   // 8 s de course
    const size_t notees = s.profil.dernieres.size();
    g.allerA(Ecran::Menu); images(30, 5);
    const long long apresQuittee = boutique::solde(s);
    const bool quitteeNotee = s.profil.dernieres.size() == notees + 1 && s.profil.dernieres[0].quittee;
    std::snprintf(b, sizeof b, "SCORE ÉTOILES : course à 5 quittée après 8 s : %s, %d ★ ; solde %lld ★ (inchangé attendu : %lld ★)",
                  quitteeNotee ? s.profil.dernieres[0].resultat().c_str() : "non notée", quitteeNotee ? s.profil.dernieres[0].etoiles : -1, apresQuittee, apresMenu);
    noter(b);
    if (!quitteeNotee) ecart("course quittée non notée au profil");
    else if (s.profil.dernieres[0].etoiles != 0) ecart("course quittée avec des ★");
    if (apresQuittee != apresMenu) ecart("solde changé par une course quittée");
    capture("score-quittee");

    s.enregistrer(); s.attendre();
    noter(ecarts ? "SCORE ÉTOILES : ÉCHEC (" + std::to_string(ecarts) + " écart(s))" : "SCORE ÉTOILES : OK (crédit du score, aucun 2e crédit, course quittée à 0 ★)");
    std::string texte;
    for (const std::string& l : lignes) texte += l + "\n";
    if (!ecrireAtomique(out + "/score-etoiles.txt", texte)) WFS_ERR("score-etoiles : texte non écrit");
    g.setAutopilot(false);
    modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4;
    courseBenchRules(false);
    g.shutdown();
    s.desactiver();
    std::filesystem::remove(chemin, ec);
    return ecarts;
}

} // namespace wfs::boutique
