// WaForSpeed — banc « textes » de la course (WFS-506, épic E20) : une course de chaque mode dans la langue de WFS_LANGUE
// (téléphone simulé, réglage Automatique), en Ville.
//   - Checkpoint : HUD, carte de navigation (consigne), bandeau de quasi-contact, fin, « Rejouer », fin, « Menu » ;
//   - Course à 5 (2 km) : HUD, classement, carte de navigation, de jour puis de nuit, fin, « Rejouer », fin, « Menu » ;
//   - Poursuite : encart de la police, annonce « RENFORT DE POLICE », perte de points, victoire, « Rejouer », fin, « Menu » ;
//   - Duel en mémoire : classement, résultat, « Revanche », résultat, « Retour » (benchDuelTextes, duel_system.cpp).
// Chaque capture textes-<fr|en>-<nom> note les textes dessinés (journal de WFS-504) ; à la fin, le garde-fou vérifie qu'aucune
// phrase française ni aucun texte qui dépasse de son bouton n'apparaît en anglais (ui::langue::verifierJournal ; rien n'est
// vérifié en français). Les noms inventés (rues, pseudos, adversaires) sont notés à part (textes::NomPropre).
#include <cstdlib>
#include <string>
#include "../core/log.h"
#include "../render/renderer.h"
#include "../ui/langue.h"
#include "course.h"
#include "course5.h"
#include "course_system.h"
#include "duel_system.h"
#include "game.h"
#include "player.h"
#include "poursuite.h"
#include "textes_course.h"

namespace wfs {

int benchCourseTextes(Game& g, Renderer& r, const std::string& out) {
    const std::string lg = ui::langue::code();
    int ecarts = 0;
    double t = 3000;
    auto verifier = [&](bool ok, const std::string& quoi) {
        if (!ok) ecarts++;
        WFS_LOG("textes (WFS-506) : %s : %s : %s", lg.c_str(), quoi.c_str(), ok ? "OK" : "ÉCHEC");
    };
    auto images = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 9) g.render(t); } };
    auto capture = [&](const std::string& nom) {   // textes des deux dernières images notés au journal, puis l'image
        ui::langue::journalActif(true, nom.c_str());
        g.render(t += 0.01); g.render(t += 0.01);
        ui::langue::journalActif(false);
        r.capturePng(out + "/textes-" + lg + "-" + nom + ".png");
        WFS_LOG("capture textes-%s-%s : écran %s, mode %s", lg.c_str(), nom.c_str(), nomEcran(g.ecran()), nomMode(modeCourse()));
    };
    auto toucher = [&](bool (*bouton)(const std::string&, float&, float&), const char* nom) {
        float x = 0, y = 0;
        if (!bouton(nom, x, y)) { verifier(false, std::string("bouton « ") + nom + " » absent"); return; }
        g.onTouch({21, x, y, true}); images(2); g.onTouch({21, x, y, false}); images(4);
    };
    auto finie = [&]() { const Course* c = activeCourse(); return c && c->state == RaceState::Over; };
    auto jusquaFin = [&](int maxImages) { for (int i = 0; i < maxImages && !finie(); i++) { g.update(1.0 / 60); t += 1.0 / 60; } images(90); };
    auto enCourse = [&]() { const Course* c = activeCourse(); return g.ecran() == Ecran::Course && c && c->state != RaceState::Over; };
    auto demarrer = [&](ModeCourse m, const char* moment) {
        modeCourse() = m;
        setenv("WFS_MOMENT", moment, 1);
        if (!g.init(r, "golf", "ville")) { verifier(false, std::string("chargement ") + nomMode(m)); return false; }
        g.setAutopilot(true); g.camMode = 0;
        g.allerA(Ecran::Course);
        images(3 * 60 + 5 * 60);   // décompte (3 s) puis 5 s de course
        return true;
    };
    courseBenchRules(true);
    ui::langue::viderJournal();

    // ---------- Checkpoint ----------
    if (demarrer(ModeCourse::Checkpoint, "jour")) {
        if (Course* c = activeCourseMut()) { c->nearCount += 1; c->combo = 2; }   // bandeau « Quasi-contact ×2 » (même chemin qu'en course)
        images(20);
        capture("course-checkpoint");
        courseBenchSetTime(0.05); jusquaFin(5 * 60);
        verifier(finie(), "Checkpoint : course finie");
        capture("fin-checkpoint");
        toucher(courseBouton, "rejouer");
        images(3 * 60 + 60);
        verifier(enCourse(), "Checkpoint : « Rejouer » → nouvelle course");
        courseBenchSetTime(0.05); jusquaFin(5 * 60);
        verifier(finie(), "Checkpoint : 2e course finie");
        toucher(courseBouton, "menu");
        images(10);
        verifier(g.ecran() == Ecran::Menu, "Checkpoint : fin puis « Menu » → menu principal");
        g.shutdown();
    }
    // ---------- Course à 5 (2 km) : jour, puis nuit ----------
    kmCourse5() = 2;
    if (demarrer(ModeCourse::Course5, "nuit")) {
        capture("course-course5-nuit");
        g.shutdown();
    }
    if (demarrer(ModeCourse::Course5, "jour")) {
        capture("course-course5");
        jusquaFin(150 * 60);
        verifier(finie(), "Course à 5 : course finie");
        capture("fin-course5");
        toucher(course5Bouton, "rejouer");
        images(3 * 60 + 60);
        verifier(enCourse(), "Course à 5 : « Rejouer » → nouvelle course");
        jusquaFin(150 * 60);
        verifier(finie(), "Course à 5 : 2e course finie");
        toucher(course5Bouton, "menu");
        images(10);
        verifier(g.ecran() == Ecran::Menu, "Course à 5 : fin puis « Menu » → menu principal");
        g.shutdown();
    }
    // ---------- Poursuite : annonce de renfort, perte de points, victoire ----------
    poursuiteBancFixe(true);
    if (demarrer(ModeCourse::Poursuite, "jour")) {
        poursuiteBancMessages();
        images(20);
        capture("course-poursuite");
        poursuiteBancFin(2); jusquaFin(60 * 60);
        verifier(finie(), "Poursuite : course finie (victoire)");
        capture("fin-poursuite");
        toucher(poursuiteBouton, "rejouer");
        images(3 * 60 + 60);
        verifier(enCourse(), "Poursuite : « Rejouer » → nouvelle poursuite");
        poursuiteBancFin(1); jusquaFin(10 * 60);
        verifier(finie(), "Poursuite : 2e poursuite finie (arrêté)");
        capture("fin-poursuite-arrete");
        toucher(poursuiteBouton, "menu");
        images(10);
        verifier(g.ecran() == Ecran::Menu, "Poursuite : fin puis « Menu » → menu principal");
        g.shutdown();
    }
    poursuiteBancFixe(false);
    modeCourse() = ModeCourse::Checkpoint;
    unsetenv("WFS_MOMENT");
    courseBenchRules(false);

    // ---------- Duel en mémoire ----------
    ecarts += benchDuelTextes(g, r, capture);

    const int phrases = ui::langue::verifierJournal();   // RC-E20-06 : aucune phrase française, aucun texte qui dépasse (en anglais)
    WFS_LOG("textes (WFS-506) : banc de la course %s : %d écart(s) de parcours, %d échec(s) du garde-fou → %s", lg.c_str(), ecarts, phrases,
            ecarts + phrases ? "ROUGE" : "VERT");
    return ecarts + phrases;
}

} // namespace wfs
