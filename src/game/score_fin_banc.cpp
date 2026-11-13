// WaForSpeed — banc de l'écran de fin du score et du compteur d'étoiles (WFS-537, épic E22) : WFS_BANC=score-fin, voir score_fin.h.
// Vraie partie au banc d'essai PC, sauvegarde active dans un fichier propre au banc (effacé au début et à la fin), en français
// puis en anglais (réglage « Langue ») :
//   0. 1er lancement : « Crée ton pilote » ouvert, compteur caché ; profil du banc créé ;
//   1. menu principal avec 0, 250 et 12 450 ★ (RG-07, RG-08, RG-10) ;
//   2. course solo terminée → bloc « Score », « +N ★ », nouveau solde ; « Menu » → accueil, compteur = nouveau solde (parcours) ;
//   3. course à 5 (2 km) gagnée : adversaires tenus derrière le joueur (course5BancSuiveurs) ;
//   4. Poursuite perdue (arrêté par la police) ;
//   5. duel en mémoire gagné (l'adversaire arrive après le joueur) ;
//   6. course à 5 quittée après le départ : pas de bloc, solde inchangé (RG-03) ;
//   7. appui sur le compteur du menu → garage (RG-09).
// À chaque écran de fin : le bloc est dans l'écran et ne couvre ni « Rejouer » ni « Menu » (RG-04), le solde a augmenté des
// ★ montrées. Captures score-fin-<fr|en>-<mode>-<L>x<H> et score-menu-<fr|en>-<n>-<L>x<H> ; texte <out>/score-fin.txt ; textes
// notés et garde-fou de WFS-504 (phrase française en anglais, texte qui dépasse) à la fin.
#include "score_fin.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../render/renderer.h"
#include "../ui/ecrans/cgu.h"
#include "../ui/ecrans/compteur_etoiles.h"
#include "../ui/ecrans/creation_profil.h"   // WFS-481 : écran « Crée ton pilote »
#include "../ui/langue.h"
#include "boutique.h"
#include "course.h"
#include "course5.h"
#include "course_system.h"
#include "duel_system.h"
#include "game.h"
#include "poursuite.h"
#include "sauvegarde.h"
#include "track.h"   // M
#include "score_fin_calc.h"

namespace wfs::score_fin {

int benchScoreFin(Game& g, Renderer& r, const std::string& out) {
    std::error_code ec; std::filesystem::create_directories(out, ec);
    const std::string chemin = out + "/score-fin-banc.txt";
    std::filesystem::remove(chemin, ec);
    Sauvegarde& s = sauvegarde();
    s.charger(chemin);                   // partie neuve, sauvegarde active : portefeuille réel
    s.cgu = ecrans::VERSION_CGU;         // CGU déjà acceptées (l'écran des CGU a son banc, WFS-308)
    const std::string taille = std::to_string(r.width()) + "x" + std::to_string(r.height());
    int ecarts = 0;
    std::vector<std::string> lignes;
    auto noter = [&](const std::string& l) { WFS_LOG("%s", l.c_str()); lignes.push_back(l); };
    auto verifier = [&](bool ok, const std::string& quoi) {
        if (!ok) ecarts++;
        noter(std::string("SCORE FIN (") + ui::langue::code() + ") : " + quoi + " : " + (ok ? "OK" : "ÉCHEC"));
    };
    double t = 5000;
    auto images = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 9) g.render(t); } };
    auto capture = [&](const std::string& nom) {   // textes des deux dernières images notés (garde-fou de WFS-504), puis l'image
        ui::langue::journalActif(true, nom.c_str());
        g.render(t += 0.01); g.render(t += 0.01);
        ui::langue::journalActif(false);
        r.capturePng(out + "/" + nom + "-" + taille + ".png");
        WFS_LOG("capture %s-%s : écran %s", nom.c_str(), taille.c_str(), nomEcran(g.ecran()));
    };
    auto fixerSolde = [&](long long n) { boutique::debiter(s, boutique::solde(s), "banc score-fin"); boutique::crediter(s, n, "banc score-fin"); };
    auto toucher = [&](float x, float y) { g.onTouch({41, x, y, true}); images(2); g.onTouch({41, x, y, false}); images(4); };
    auto finie = [&]() { const Course* c = activeCourse(); return c && c->state == RaceState::Over; };
    auto jusquaFin = [&](int maxImages) { for (int i = 0; i < maxImages && !finie(); i++) { g.update(1.0 / 60); t += 1.0 / 60; } images(120); };
    auto demarrer = [&](ModeCourse m) {
        modeCourse() = m;
        s.mode = poursuite::cleMode(m); s.km5 = kmCourse5();   // le mode est relu de la sauvegarde active à chaque partie (modes_ecran.cpp)
        if (!g.init(r, "golf", "ville")) { verifier(false, std::string("chargement ") + nomMode(m)); return false; }
        g.setAutopilot(true); g.camMode = 0;
        g.allerA(Ecran::Course);
        images(3 * 60 + 5 * 60);   // décompte (3 s) puis 5 s de course
        return true;
    };
    // RG-01, RG-02, RG-04 : bloc posé, étoiles créditées, boutons hors du bloc et bloc dans l'écran
    auto verifierFin = [&](const char* mode, long long avant, bool (*bouton)(const std::string&, float&, float&)) {
        const Etat& e = etat();
        float x = 0, y = 0, w = 0, h = 0;
        const bool bloc = rectBloc(x, y, w, h);
        char b[256];
        std::snprintf(b, sizeof b, "%s : %d ligne(s), total %d, +%d ★, solde %lld → %lld%s", mode, (int)e.lignes.size(), e.total, e.etoiles, avant,
                      boutique::solde(s), e.nouveauRecord ? ", nouveau record" : "");
        noter(std::string("SCORE FIN (") + ui::langue::code() + ") : " + b);
        verifier(e.pret && e.detail && !e.lignes.empty(), std::string(mode) + " : détail du score (RG-01)");
        verifier(e.etoiles > 0 && boutique::solde(s) == avant + e.etoiles && e.solde == boutique::solde(s), std::string(mode) + " : +N ★ = étoiles reçues, nouveau solde (RG-02)");
        verifier(bloc && x >= 0 && y >= 0 && x + w <= r.width() + 0.5f && y + h <= r.height() + 0.5f, std::string(mode) + " : bloc dans l'écran");
        for (const char* nom : {"rejouer", "menu"}) {
            float bx = 0, by = 0;
            const bool vu = bouton ? bouton(nom, bx, by) : duelBouton(std::string(nom) == "rejouer" ? "revanche" : "retour", bx, by);
            verifier(vu && !(bx >= x && bx <= x + w && by >= y && by <= y + h), std::string(mode) + " : « " + nom + " » visible, hors du bloc (RG-04)");
        }
    };
    courseBenchRules(true);
    ui::langue::viderJournal();

    // ---------- 0. 1er lancement : « Crée ton pilote » (WFS-481) ouvert : ni « Réglages » ni compteur ; puis profil du banc ----------
    ui::langue::choisir(ui::langue::Choix::Francais);
    if (g.init(r, "golf", "ville")) {
        g.setAutopilot(false); g.allerA(Ecran::Menu); images(90);
        capture("score-menu-creation-fr");
        float cx = 0, cy = 0;
        verifier(ecrans::creationProfilOuverte() && !ecrans::compteur_etoiles::centre(cx, cy), "« Crée ton pilote » ouvert : compteur caché");
        { std::string nick; s.profil.creer("Banc 537", "WFS-537", nick); s.nick = nick; }   // profil du banc, comme WFS-481 (cgu.cpp)
        images(30);
        verifier(!ecrans::creationProfilOuverte() && ecrans::compteur_etoiles::centre(cx, cy), "profil créé : menu et compteur dessinés");
        g.shutdown();
    }
    for (const ui::langue::Choix lg : {ui::langue::Choix::Francais, ui::langue::Choix::Anglais}) {
        ui::langue::choisir(lg);
        const std::string code = ui::langue::code();
        // ---------- 1. menu : 0, 250, 12 450 ★ ----------
        if (g.init(r, "golf", "ville")) {
            g.setAutopilot(false); g.allerA(Ecran::Menu); images(90);
            for (const long long n : {0LL, 250LL, 12450LL}) {
                fixerSolde(n); images(12);
                capture("score-menu-" + code + "-" + std::to_string(n));
                float cx = 0, cy = 0;
                verifier(ecrans::compteur_etoiles::centre(cx, cy), "menu " + std::to_string(n) + " ★ : compteur dessiné (RG-07)");
                verifier(ecrans::compteur_etoiles::texteLecteur() == ui::tr(singulier(n, code == "en") ? "menu.etoiles.lecteur.un" : "menu.etoiles.lecteur", nombreEtoiles(n, code == "en")),
                         "menu : lecteur d'écran « " + ecrans::compteur_etoiles::texteLecteur() + " » (RG-08)");
            }
            fixerSolde(0);
            g.shutdown();
        }
        // ---------- 2. course solo terminée, puis « Menu » : compteur à jour ----------
        if (demarrer(ModeCourse::Checkpoint)) {
            const long long avant = boutique::solde(s);
            courseBenchSetTime(0.05); jusquaFin(5 * 60);
            verifier(finie(), "course solo finie");
            capture("score-fin-" + code + "-solo");
            verifierFin("course solo", avant, courseBouton);
            float x = 0, y = 0;
            if (courseBouton("menu", x, y)) toucher(x, y);
            images(30);
            verifier(g.ecran() == Ecran::Menu && !etat().pret, "fin → « Menu » : accueil, bloc effacé");
            images(30);
            capture("score-parcours-" + code + "-menu");
            verifier(ecrans::compteur_etoiles::texteLecteur() == ui::tr(singulier(boutique::solde(s), code == "en") ? "menu.etoiles.lecteur.un" : "menu.etoiles.lecteur", nombreEtoiles(boutique::solde(s), code == "en")),
                     "parcours accueil → course → fin → menu : compteur = nouveau solde (RG-08)");
            // ---------- 7. appui sur le compteur → garage (RG-09) ----------
            if (ecrans::compteur_etoiles::centre(x, y)) toucher(x, y);
            images(20);
            verifier(g.ecran() == Ecran::Garage, "appui sur le compteur → garage (RG-09)");
            g.shutdown();
        }
        // ---------- 3. course à 5 (2 km) gagnée ----------
        kmCourse5() = 2;
        if (demarrer(ModeCourse::Course5)) {
            const long long avant = boutique::solde(s);
            course5BancSuiveurs({{-25, 0}, {-32, 1}, {-39, 2}, {-46, 1}});   // adversaires tenus derrière : 1re place
            for (int i = 0; i < 150 * 60 && !finie(); i++) {   // lâchés à 40 m de l'arrivée (tenus, ils empêchent la fin de la course à 5)
                const Course* c = activeCourse();
                if (c && c->duelFinish > 0 && g.player().dist > c->duelFinish - 40 / M) course5BancSuiveurs({});
                g.update(1.0 / 60); t += 1.0 / 60;
            }
            course5BancSuiveurs({});
            jusquaFin(60);
            verifier(finie(), "course à 5 finie");
            capture("score-fin-" + code + "-course5");
            verifierFin("course à 5", avant, course5Bouton);
            g.shutdown();
        }
        // ---------- 4. Poursuite perdue ----------
        poursuiteBancFixe(true);
        if (demarrer(ModeCourse::Poursuite)) {
            const long long avant = boutique::solde(s);
            poursuiteBancFin(1); jusquaFin(10 * 60);
            verifier(finie(), "Poursuite finie (arrêté)");
            capture("score-fin-" + code + "-poursuite");
            verifierFin("Poursuite", avant, poursuiteBouton);
            g.shutdown();
        }
        poursuiteBancFixe(false);
        // ---------- 5. duel gagné (salon en mémoire, adversaire derrière) ----------
        {
            const long long avant = boutique::solde(s);
            bool vu = false;
            ecarts += benchDuelTextes(g, r, [&](const std::string& nom) {
                if (nom != "duel-fin") return;
                capture("score-fin-" + code + "-duel");
                verifierFin("duel", avant, nullptr);
                vu = true;
            }, true);
            verifier(vu, "duel : écran de fin vu");
        }
        // ---------- 6. course à 5 quittée après le départ : pas de bloc, rien de crédité (RG-03) ----------
        if (demarrer(ModeCourse::Course5)) {
            const long long avant = boutique::solde(s);
            g.allerA(Ecran::Menu); images(30);
            verifier(!etat().pret && boutique::solde(s) == avant, "course quittée : pas de bloc « Score », solde inchangé (RG-03)");
            g.shutdown();
        }
    }
    modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4;
    courseBenchRules(false);
    ui::langue::choisir(ui::langue::Choix::Automatique);
    const int phrases = ui::langue::verifierJournal();   // RG-05 : garde-fous de WFS-504 (en anglais)
    verifier(phrases == 0, "garde-fous de WFS-504 : " + std::to_string(phrases) + " échec(s)");
    noter(ecarts ? "SCORE FIN : ÉCHEC (" + std::to_string(ecarts) + " écart(s))" : "SCORE FIN : OK");
    std::string texte;
    for (const std::string& l : lignes) texte += l + "\n";
    if (!ecrireAtomique(out + "/score-fin-" + taille + ".txt", texte)) WFS_ERR("score-fin : texte non écrit");
    s.desactiver();
    std::filesystem::remove(chemin, ec);
    return ecarts;
}

} // namespace wfs::score_fin
