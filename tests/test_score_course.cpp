// WFS-535 (RG-07) : score d'une course et conversion en étoiles. Un cas par ligne de RG-01 à RG-04, bornes 5 et 40,
// course quittée = 0, valeurs lues dans configgame.json (une valeur changée change le résultat), duel dont l'adversaire part
// avant 30 s = 0 (RC-E22-04). Affiche aussi le détail de 3 courses (captures de la PR : qa/pr/WFS-535/).
#include <cstdio>
#include <string>
#include "../src/core/configgame.h"
#include "../src/game/score_course.h"
using namespace wfs;
using namespace wfs::score;

static int g_echecs = 0;
static void cas(const std::string& nom, bool ok) {
    std::printf("%s %s\n", ok ? "OK   " : "ÉCHEC", nom.c_str());
    if (!ok) g_echecs++;
}
static int points(const Score& s, const char* cle) {   // points d'une ligne (0 : ligne absente)
    for (const Ligne& l : s.lignes) if (std::string(l.cle) == cle) return l.points;
    return 0;
}
static void afficher(const char* titre, const Score& s) {
    std::printf("\n---- %s\n", titre);
    for (const Ligne& l : s.lignes) std::printf("  %-32s %+6d\n", l.cle, l.points);
    std::printf("  %-32s %6d points\n  %-32s %6d ★%s\n\n", "total", s.total, "étoiles", s.etoiles, s.nouveauRecord ? "   (nouveau record)" : "");
}
// course de base : terminée, record existant non approché, 3 chocs (0 point), aucun défi
static ResultatCourse base(ModeJoue m) {
    ResultatCourse r; r.mode = m; r.terminee = true; r.temps = 200; r.record = 100; r.chocs = 3; return r;
}

int main() {
    const ConfigJeu::ScoreEtoiles b{};   // valeurs du code (= configgame.json)
    // ---- RG-01 : place ou issue
    {
        const int attendu[5] = {1000, 700, 500, 350, 250};
        bool ok = true;
        for (int p = 1; p <= 5; p++) { ResultatCourse r = base(ModeJoue::Course5); r.place = p; ok = ok && calculer(r, b).total == attendu[p - 1] && points(calculer(r, b), "score.place") == attendu[p - 1]; }
        cas("course à 5 : 1er 1000, 2e 700, 3e 500, 4e 350, 5e 250", ok);
        ResultatCourse r = base(ModeJoue::Course5); r.place = 6;
        cas("course à 5 : place hors bornes : 0 point de place, sans erreur", calculer(r, b).total == 0);
        r = base(ModeJoue::Duel); r.victoire = true;
        cas("duel gagné : 1000", calculer(r, b).total == 1000 && points(calculer(r, b), "score.duel_gagne") == 1000);
        r.victoire = false;
        cas("duel perdu : 400", calculer(r, b).total == 400 && points(calculer(r, b), "score.duel_perdu") == 400);
        r = base(ModeJoue::Poursuite); r.victoire = true;
        cas("Poursuite gagnée : 1000", calculer(r, b).total == 1000);
        r.victoire = false; r.scorePoursuite = 300;
        cas("Poursuite perdue avec 300 points restants : 300 + 0,5 × 300 = 450", calculer(r, b).total == 450 && points(calculer(r, b), "score.poursuite_score_restant") == 150);
        r = base(ModeJoue::Checkpoint); r.pointsDistance = 2500;
        cas("course solo terminée, 2 500 points de distance : 600 + 250 = 850", calculer(r, b).total == 850);
        r.pointsDistance = 9000;
        cas("course solo : part de la distance plafonnée à 400 (600 + 400)", calculer(r, b).total == 1000 && points(calculer(r, b), "score.solo_distance") == 400);
        r.pointsDistance = -1;
        cas("course solo sans compteur de distance : 600", calculer(r, b).total == 600);
    }
    // ---- RG-02 : temps
    {
        ResultatCourse r = base(ModeJoue::Course5); r.place = 5; r.record = 100; r.temps = 99;
        Score s = calculer(r, b);
        cas("record battu : +300, « nouveau record »", points(s, "score.record_battu") == 300 && s.total == 550 && s.nouveauRecord);
        r.temps = 105;
        s = calculer(r, b);
        cas("à 5 % du record (105 s pour 100 s) : +150, pas de nouveau record", points(s, "score.proche_record") == 150 && s.total == 400 && !s.nouveauRecord);
        r.temps = 105.5;
        cas("à plus de 5 % du record : 0", calculer(r, b).total == 250);
        r.temps = 100;
        cas("record égalé (pas battu) : +150 (proche)", points(calculer(r, b), "score.proche_record") == 150 && !calculer(r, b).nouveauRecord);
        r.record = 0; r.temps = 140;
        s = calculer(r, b);
        cas("1re course sur cette carte : +150", points(s, "score.premiere_course") == 150 && s.total == 400);
    }
    // ---- RG-03 : conduite propre
    {
        ResultatCourse r = base(ModeJoue::Course5); r.place = 5;
        r.chocs = 0; cas("0 choc : +200", points(calculer(r, b), "score.zero_choc") == 200 && calculer(r, b).total == 450);
        r.chocs = 1; cas("1 choc : +100", points(calculer(r, b), "score.peu_chocs") == 100);
        r.chocs = 2; cas("2 chocs : +100", points(calculer(r, b), "score.peu_chocs") == 100);
        r.chocs = 3; cas("3 chocs : 0", calculer(r, b).total == 250);
        r.chocs = -1; cas("chocs inconnus : 0, sans erreur", calculer(r, b).total == 250);
    }
    // ---- RG-04 : défis (2 par mode)
    {
        ResultatCourse r = base(ModeJoue::Course5); r.place = 5;
        r.depassements = 3; cas("course à 5 : 3 dépassements : +60", points(calculer(r, b), "score.defi_depassements") == 60);
        r.depassements = 14; cas("course à 5 : dépassements plafonnés à +200", points(calculer(r, b), "score.defi_depassements") == 200);
        r.depassements = -1; r.enTeteDernierTour = Defi::Reussi; cas("course à 5 : en tête au dernier tour : +100", points(calculer(r, b), "score.defi_en_tete") == 100 && calculer(r, b).total == 350);
        r.enTeteDernierTour = Defi::Rate; cas("course à 5 : pas en tête au dernier tour : 0", calculer(r, b).total == 250);

        r = base(ModeJoue::Poursuite); r.victoire = true; r.scorePoursuiteDepart = 1000;
        r.contactsPolice = 0; cas("Poursuite : aucun contact avec la police : +200", points(calculer(r, b), "score.defi_sans_police") == 200);
        r.contactsPolice = 2; cas("Poursuite : 2 contacts avec la police : 0", points(calculer(r, b), "score.defi_sans_police") == 0);
        r.scorePoursuite = 500; cas("Poursuite : arrivée avec 50 % du score : +100", points(calculer(r, b), "score.defi_score_garde") == 100);
        r.scorePoursuite = 499; cas("Poursuite : arrivée avec 49,9 % du score : 0", points(calculer(r, b), "score.defi_score_garde") == 0);
        r.victoire = false; r.scorePoursuite = 900; cas("Poursuite perdue : pas d'« arrivée » : défi du score à 0", points(calculer(r, b), "score.defi_score_garde") == 0);

        r = base(ModeJoue::Duel); r.victoire = true;
        r.jamaisDerriere = Defi::Reussi; cas("duel : jamais derrière après la mi-course : +150", points(calculer(r, b), "score.defi_jamais_derriere") == 150);
        r.ecartArrivee = 3.5; cas("duel : gagné avec 3,5 s d'avance : +100", points(calculer(r, b), "score.defi_ecart") == 100);
        r.ecartArrivee = 3.0; cas("duel : 3,0 s d'avance (pas plus de 3 s) : 0", points(calculer(r, b), "score.defi_ecart") == 0);

        r = base(ModeJoue::Checkpoint);
        r.checkpointsSansRetard = Defi::Reussi; cas("course solo : tous les checkpoints sans retard : +150", points(calculer(r, b), "score.defi_sans_retard") == 150);
        r.nitroJamaisVide = Defi::Reussi; cas("course solo : nitro jamais vide : +100", points(calculer(r, b), "score.defi_nitro") == 100 && calculer(r, b).total == 850);
        r.nitroJamaisVide = Defi::Inconnu; r.checkpointsSansRetard = Defi::Inconnu;
        cas("course solo : compteurs de défis absents : 0, sans erreur", calculer(r, b).total == 600);
    }
    // ---- RG-05 : conversion, bornes 5 et 40, course quittée
    {
        ResultatCourse r = base(ModeJoue::Course5); r.place = 5; r.chocs = 99;   // 250 points → 250 / 85 = 2,9 → borné à 5
        cas("borne basse : 250 points → 5 ★ (pas 3)", calculer(r, b).etoiles == 5);
        ConfigJeu::ScoreEtoiles g = b; g.pointsParEtoile = 10;   // 1000 + 300 + 200 + 200 + 100 = 1800 → 180 → borné à 40
        r = base(ModeJoue::Course5); r.place = 1; r.temps = 90; r.chocs = 0; r.depassements = 10; r.enTeteDernierTour = Defi::Reussi;
        cas("borne haute : 1800 points à 10 par ★ → 40 ★ (pas 180)", calculer(r, g).total == 1800 && calculer(r, g).etoiles == 40);
        cas("conversion : 1800 points à 85 par ★ → 21 ★ (21,2 arrondi)", calculer(r, b).etoiles == 21);
        r = base(ModeJoue::Course5); r.place = 3; r.chocs = 1; r.depassements = 2;   // 500 + 100 + 40 = 640 → 7,5 → 8 ★ (arrondi du demi vers le haut)
        cas("course à 5 moyenne (3e, 1 choc, 2 dépassements) : 640 points → 8 ★", calculer(r, b).etoiles == 8);
        r = base(ModeJoue::Duel); r.victoire = true; r.chocs = 2; r.jamaisDerriere = Defi::Reussi;   // 1000 + 100 + 150 = 1250 → 14,7 → 15 ★
        cas("duel gagné (2 chocs, jamais derrière) : 1250 points → 15 ★", calculer(r, b).etoiles == 15);
        r.terminee = false;
        const Score q = calculer(r, b);
        cas("course quittée : aucune ligne, 0 point, 0 ★", q.lignes.empty() && q.total == 0 && q.etoiles == 0);
    }
    // ---- RC-E22-04 : duel, adversaire parti avant 30 s
    {
        ResultatCourse r = base(ModeJoue::Duel); r.victoire = true; r.chocs = 0;
        r.adversaireParti = 12; const Score s = calculer(r, b);
        cas("duel : adversaire parti à 12 s : 0 point, 0 ★", s.total == 0 && s.etoiles == 0 && s.lignes.empty());
        r.adversaireParti = 45;
        cas("duel : adversaire parti à 45 s : score normal (1000 + 200)", calculer(r, b).total == 1200);
    }
    // ---- RG-07 : valeurs lues dans configgame.json
    {
        ConfigJeu c;
        const BilanConfig bl = lireConfig("{\"score_etoiles\": {\"course5\": {\"place_1\": 1500}, \"conversion\": {\"points_par_etoile\": 50, \"min\": 2}}}", "essai", c);
        ResultatCourse r = base(ModeJoue::Course5); r.place = 1;
        const Score s = calculer(r, c.scoreEtoiles);
        cas("configgame.json : place_1 = 1500 et 50 points par ★ lus → 1500 points, 30 ★", bl.jsonValide && bl.erreurs == 0 && s.total == 1500 && s.etoiles == 30);
        ConfigJeu d;
        const BilanConfig bm = lireConfig("{\"score_etoiles\": {\"conversion\": {\"points_par_etoile\": 0}}}", "essai", d);
        cas("configgame.json : points_par_etoile = 0 refusé (hors bornes) : valeur du code (85)", bm.erreurs == 1 && d.scoreEtoiles.pointsParEtoile == 85);
        ConfigJeu e;
        lireConfig("{\"score_etoiles\": {\"conversion\": {\"min\": 50, \"max\": 40}}}", "essai", e);
        cas("configgame.json : min > max : min ramené à max", e.scoreEtoiles.etoilesMin == 40 && e.scoreEtoiles.etoilesMax == 40);
        cas("valeurs du code = barème de départ du PO (points par ★ 85, bornes 5 et 40)", b.pointsParEtoile == 85 && b.etoilesMin == 5 && b.etoilesMax == 40);
    }
    // ---- détail de 3 courses (captures de la PR)
    {
        ResultatCourse r = base(ModeJoue::Course5); r.place = 1; r.temps = 182.4; r.record = 185.0; r.chocs = 0; r.depassements = 4; r.enTeteDernierTour = Defi::Reussi;
        afficher("1) Course à 5 : 1er, conduite propre (0 choc), record battu, 4 dépassements, en tête au dernier tour", calculer(r, b));
        r = base(ModeJoue::Poursuite); r.victoire = false; r.scorePoursuite = 240; r.scorePoursuiteDepart = 1000; r.temps = 0; r.record = 410; r.chocs = 5; r.contactsPolice = 3;
        afficher("2) Poursuite perdue : 240 points restants, 5 chocs, 3 contacts avec la police", calculer(r, b));
        r = base(ModeJoue::Course5); r.place = 2; r.chocs = 0; r.terminee = false;
        afficher("3) Course quittée (course à 5, 2e au moment de quitter)", calculer(r, b));
    }
    std::printf("%s : %d échec(s)\n", g_echecs ? "ÉCHEC" : "OK", g_echecs);
    return g_echecs ? 1 : 0;
}
