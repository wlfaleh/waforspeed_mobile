// WFS-498 (RG-11, RG-12) : portefeuille d'étoiles, prix des voitures et des personnalisations, équivalents en médailles et en euros,
// achats (suffisant, insuffisant, déjà possédé), peinture achetée pour une seule voiture, pack de dessins pour toutes, essai gratuit
// une seule fois, sauvegarde d'avant (voiture et couleurs gardées), partie neuve sur la p208, configgame.json mal écrit, et
// banc d'équilibrage (1re voiture 22 à 28 courses, tout débloquer 108 à 132 h ; un prix changé change le résultat).
#include <cstdio>
#include <cstring>
#include <string>
#include "../src/core/configgame.h"
#include "../src/game/boutique.h"
#include "../src/game/cars.h"
using namespace wfs;
using namespace wfs::boutique;

static int g_echecs = 0;
static void cas(const std::string& nom, bool ok) {
    std::printf("%s %s\n", ok ? "OK   " : "ÉCHEC", nom.c_str());
    if (!ok) g_echecs++;
}
static const perso::Objet& O(const char* id) {
    const perso::Objet* o = perso::objet(id);
    if (!o) { std::printf("ÉCHEC objet inconnu : %s\n", id); std::fflush(stdout); std::abort(); }
    return *o;
}
static std::string remplacer(std::string s, const std::string& a, const std::string& b) {
    const size_t i = s.find(a); if (i != std::string::npos) s.replace(i, a.size(), b); return s;
}

int main() {
    // ---- RG-03, RG-04 : voitures gratuites, prix, classes, coefficients
    {
        Sauvegarde s; s.defauts();
        cas("3 voitures gratuites possédées dès le départ (p208, abarth, gt86)", possedeVoiture(s, "p208") && possedeVoiture(s, "abarth") && possedeVoiture(s, "gt86"));
        int gratuites = 0, payantes = 0; long long total = 0;
        for (const char* id : VOITURES_IDS) { if (voitureGratuite(id)) gratuites++; else if (prixVoiture(id) > 0) { payantes++; total += prixVoiture(id); } }
        cas("3 gratuites, 14 à acheter, total 24 350 ★", gratuites == 3 && payantes == 14 && total == 24350);
        struct { const char* id; int prix, classe; } t[] = {
            {"p208", 0, 0}, {"abarth", 0, 0}, {"gt86", 0, 0}, {"corsa", 250, 1}, {"tcross", 300, 1}, {"compass", 300, 1},
            {"golf", 600, 2}, {"rrsport", 800, 2}, {"s3", 1000, 2}, {"v60", 1000, 2}, {"model3", 1200, 2},
            {"mustang", 1800, 3}, {"m4", 2200, 3}, {"giulia", 2400, 3}, {"gallardo", 3000, 4}, {"p911", 3500, 4}, {"sf90", 6000, 5}};
        bool ok = true;
        for (const auto& v : t) {
            const bool b = prixVoiture(v.id) == v.prix && perso::classeVoiture(v.id) == v.classe;
            if (!b) std::printf("  %s : %d ★ classe %d (attendu %d ★ classe %d)\n", v.id, prixVoiture(v.id), perso::classeVoiture(v.id), v.prix, v.classe);
            ok = ok && b;
        }
        cas("prix et classes du tableau RG-04 (17 voitures)", ok);
        const double k[6] = {1, 1, 1.5, 2, 3, 4};
        bool kok = true; for (int i = 0; i < 6; i++) kok = kok && config().boutique.coefficient[i] == k[i];
        cas("coefficients des classes ×1, ×1, ×1,5, ×2, ×3, ×4", kok);
        cas("x5, a45, elantra ni vendues ni possédées", !voitureVendue("x5") && !voitureVendue("a45") && !voitureVendue("elantra") &&
            prixVoiture("x5") == -1 && !possedeVoiture(s, "a45") && !essaiDisponible(s, "elantra"));
        cas("voiture inconnue : pas vendue", !voitureVendue("dauphine") && prixVoiture("dauphine") == -1);
    }
    // ---- RG-05 : prix de base × coefficient
    {
        cas("métallisé sur la SF90 = 400 ★", perso::prix(O("metallise"), "sf90") == 400);
        cas("candy sur la Golf = 375 ★", perso::prix(O("candy"), "golf") == 375);
        cas("pack Flammes 250 ★ sans coefficient (SF90 comme Corsa)", perso::prix(O("flammes"), "sf90") == 250 && perso::prix(O("flammes"), "corsa") == 250);
    }
    // ---- RG-02 : équivalents en médailles et en euros
    {
        cas("2 200 ★ = 3 or + 1 argent + 1 bronze", equivalentMedailles(2200) == "3 or + 1 argent + 1 bronze");
        cas("250 ★ = 1 argent", equivalentMedailles(250) == "1 argent");
        cas("600 ★ = 1 or ; 50 ★ = 1 bronze ; 0 ★ = gratuit", equivalentMedailles(600) == "1 or" && equivalentMedailles(50) == "1 bronze" && equivalentMedailles(0) == "gratuit");
        cas("2 200 ★ = 21,78 €", equivalentCentimes(2200) == 2178 && texteEuros(2178) == "21,78 €");
        cas("250 ★ = 2,48 € ; 24 350 ★ = 241,07 €", texteEuros(equivalentCentimes(250)) == "2,48 €" && texteEuros(equivalentCentimes(24350)) == "241,07 €");
    }
    // ---- RG-01 : portefeuille
    {
        Sauvegarde s; s.defauts();
        cas("solde de départ 0 ★", solde(s) == 0);
        crediter(s, 120, "test");
        cas("crédit 120 ★", solde(s) == 120);
        cas("débit 200 ★ refusé, solde inchangé", !debiter(s, 200, "test") && solde(s) == 120);
        cas("débit 20 ★ accepté", debiter(s, 20, "test") && solde(s) == 100);
        cas("débit négatif refusé ; crédit négatif sans effet", !debiter(s, -5, "test") && (crediter(s, -50, "test"), solde(s) == 100));
        crediter(s, SOLDE_MAX, "test");
        cas("solde borné à 10^12 ★ (aucun débordement)", solde(s) == SOLDE_MAX);
        Sauvegarde t; t.defauts();
        cas("WFS-536 (RG-02) : calcul impossible : secours gain_course 10 ★", crediterCourse(t, -1) == 10 && solde(t) == 10);
    }
    // ---- RG-07 : achats
    {
        Sauvegarde s; s.defauts(); crediter(s, 300, "test");
        cas("Corsa (250 ★) avec 300 ★ : achetée, solde 50 ★", acheterVoiture(s, "corsa") == AchatVoiture::Achetee && solde(s) == 50 && possedeVoiture(s, "corsa"));
        cas("Corsa déjà possédée : aucun débit", acheterVoiture(s, "corsa") == AchatVoiture::DejaPossedee && solde(s) == 50);
        cas("Golf (600 ★) avec 50 ★ : refusée, rien de débité, pas possédée", acheterVoiture(s, "golf") == AchatVoiture::SoldeInsuffisant && solde(s) == 50 && !possedeVoiture(s, "golf"));
        cas("voiture gratuite : déjà possédée, aucun débit", acheterVoiture(s, "p208") == AchatVoiture::DejaPossedee && solde(s) == 50);
        cas("X5 (retirée) : pas en vente", acheterVoiture(s, "x5") == AchatVoiture::NonVendue && solde(s) == 50);
        crediter(s, 1000, "test");
        cas("métallisé sur la Golf (150 ★) payé avec le portefeuille", acheterObjet(s, O("metallise"), "golf") == perso::Achat::Achete && solde(s) == 900);
        cas("peinture achetée sur la Golf absente sur la M4", perso::possede(s, O("metallise"), "golf") && !perso::possede(s, O("metallise"), "m4"));
        cas("pack Bandes (200 ★) présent sur toutes les voitures", acheterObjet(s, O("bandes"), "golf") == perso::Achat::Achete && solde(s) == 700 &&
            perso::possede(s, O("bandes"), "m4") && perso::possede(s, O("bandes"), "sf90") && acheterObjet(s, O("bandes"), "p911") == perso::Achat::DejaPossede && solde(s) == 700);
        cas("caméléon sur la SF90 (1 600 ★) avec 700 ★ : refusé, rien de débité",
            acheterObjet(s, O("cameleon"), "sf90") == perso::Achat::SoldeInsuffisant && solde(s) == 700 && !perso::possede(s, O("cameleon"), "sf90"));
        // relu tel quel
        Sauvegarde r; const bool lu = r.lire(s.texte());
        cas("solde, voitures et objets relus depuis la sauvegarde", lu && r.etoiles == 700 && possedeVoiture(r, "corsa") && !possedeVoiture(r, "golf") &&
            perso::possede(r, O("metallise"), "golf") && perso::possede(r, O("bandes"), "m4"));
    }
    // ---- RG-08 : essai gratuit
    {
        Sauvegarde s; s.defauts();
        cas("essai disponible pour la M4 (bloquée)", essaiDisponible(s, "m4"));
        cas("essai noté une fois, puis plus disponible", marquerEssai(s, "m4") && !essaiDisponible(s, "m4") && !marquerEssai(s, "m4"));
        cas("après l'essai, la M4 reste bloquée", !possedeVoiture(s, "m4"));
        cas("pas d'essai pour une voiture possédée ou retirée", !essaiDisponible(s, "p208") && !marquerEssai(s, "gt86") && !marquerEssai(s, "x5"));
        Sauvegarde r; r.lire(s.texte());
        cas("essai gardé dans la sauvegarde (essai-m4=1)", s.texte().find("essai-m4=1\n") != std::string::npos && !essaiDisponible(r, "m4") && essaiDisponible(r, "golf"));
    }
    // ---- RG-09 : sauvegarde d'avant ce ticket, partie neuve, valeurs mal écrites
    {
        Sauvegarde s;
        const bool lu = s.lire("version=1\nwaforspeed-best=48210\ntrace=2\ncar=m4\npaint-m4=bordeaux\nperso=1\npossede-m4=bordeaux\nmap=ville\n");
        cas("sauvegarde d'avant : la M4 choisie devient possédée, sa couleur reste", lu && s.car == "m4" && possedeVoiture(s, "m4") &&
            perso::possede(s, O("bordeaux"), "m4") && s.peinture("m4") == "bordeaux" && s.etoiles == 0 && s.best == 48210);
        Sauvegarde v;
        v.lire("version=1\ncar=giulia\npaint-giulia=turquoise\nmap=ville\n");   // d'avant WFS-487 aussi
        cas("sauvegarde d'avant le catalogue : Giulia et turquoise gardées", possedeVoiture(v, "giulia") && perso::possede(v, O("turquoise"), "giulia"));
        Sauvegarde w; w.lire("version=1\ncar=m4\nperso=1\netoiles=40\nmap=ville\n");
        cas("sauvegarde à jour (clé etoiles) : pas de migration, la M4 reste à acheter", !possedeVoiture(w, "m4") && w.etoiles == 40);
        Sauvegarde x; x.lire("version=1\ncar=m4\netoiles=-3\nvoitures-possedees=golf|dauphine|\nessai-zz=1\nessai-s3=oui\nmap=ville\n");
        cas("valeurs mal écrites ignorées sans erreur (solde 0, voiture inconnue, essai mal écrit)", x.etoiles == 0 && x.voituresPossedees.size() == 1 &&
            possedeVoiture(x, "golf") && x.essais.empty());
        const std::string chemin = "test_boutique_partie_neuve.txt";
        std::remove(chemin.c_str());
        Sauvegarde n; n.charger(chemin); n.attendre();
        cas("partie neuve (aucun fichier) : voiture de départ p208, gratuite", n.car == "p208" && possedeVoiture(n, "p208"));
        n.desactiver();
        Sauvegarde g; g.defauts();
        cas("défauts du banc d'essai inchangés (Golf)", g.car == "golf");
    }
    // ---- RG-10 : configgame.json mal écrit, prix changé
    {
        const std::string defauts = configJson(ConfigJeu{});
        ConfigJeu c0; const BilanConfig b0 = lireConfig(defauts, "défauts", c0);
        cas("clés de WFS-498 écrites et relues sans erreur", b0.erreurs == 0 && defauts.find("\"gain_course\": 10") != std::string::npos &&
            defauts.find("\"minutes_par_course\": 2.96") != std::string::npos && defauts.find("\"bronze\": {") != std::string::npos);
        const std::string corsa = "\"corsa\": {\n        \"classe\": 1,\n        \"prix\": 250";
        cas("prix de la Corsa dans configgame.json", defauts.find(corsa) != std::string::npos);
        std::string t = remplacer(defauts, corsa, "\"corsa\": {\n        \"classe\": 1,\n        \"prix\": -7");
        t = remplacer(t, "\"gain_course\": 10", "\"gain_course\": \"dix\"");
        ConfigJeu c; const BilanConfig b = lireConfig(t, "boutique mal écrite", c);
        int msg = 0; for (const std::string& m : b.messages) { msg += m.find("boutique.") == 0; std::printf("  %s\n", m.c_str()); }
        cas("prix négatif et gain mal écrit refusés : valeur du code + message, jamais d'arrêt", b.erreurs == 2 && msg == 2 &&
            c.boutique.prixVoiture[13] == 250 && c.boutique.gainCourse == 10);
        // RG-11 : banc d'équilibrage avec les valeurs du code
        const Equilibrage e = equilibrage();
        for (const std::string& l : bilanEquilibrage(e)) std::printf("  %s\n", l.c_str());
        for (const std::string& l : verifierEquilibrage(e)) std::printf("  écart : %s\n", l.c_str());
        cas("1re voiture (Corsa) : 25 courses (22 à 28)", e.premiereVoiture == "corsa" && e.coursesPremiere == 25);
        cas("les 14 voitures : 24 350 ★, 2 435 courses, ≈ 120 h (108 à 132 h)", e.nbVoitures == 14 && e.totalVoitures == 24350 && e.coursesVoitures == 2435 &&
            e.heuresVoitures > 120.0 && e.heuresVoitures < 120.3);
        cas("équilibrage dans les bornes de RG-11", verifierEquilibrage(e).empty());
        cas("total avec peintures et skins plus grand que les voitures seules", e.totalPersonnalisation > 0 && e.heuresTout > e.heuresVoitures);
        // un prix changé dans configgame.json change le résultat, sans recompiler
        ConfigJeu d; lireConfig(remplacer(defauts, corsa, "\"corsa\": {\n        \"classe\": 1,\n        \"prix\": 400"), "prix changé", d);
        const ConfigJeu sauve = config(); config() = d;
        const Equilibrage f = equilibrage();
        for (const std::string& l : bilanEquilibrage(f)) std::printf("  %s\n", l.c_str());
        cas("Corsa à 400 ★ : la 1re voiture devient le T-Cross (30 courses), hors bornes signalé", f.premiereVoiture == "tcross" && f.coursesPremiere == 30 &&
            f.totalVoitures == 24500 && !verifierEquilibrage(f).empty());
        config() = sauve;
    }
    // ---- WFS-536 : les étoiles du score remplacent les 10 ★ fixes
    {
        Sauvegarde s; s.defauts();
        cas("RG-01 : course terminée : les ★ du score (23 ★) sont créditées", crediterCourse(s, 23) == 23 && solde(s) == 23);
        cas("RG-01 : score à 0 ★ (duel arrangé) : rien de crédité, pas de secours", crediterCourse(s, 0) == 0 && solde(s) == 23);
        GainCourse g;
        cas("RG-05 : course jamais partie : rien", g.finir(s, false, 12) == 0 && solde(s) == 23);
        g.depart();
        cas("RG-01 : course quittée : 0 ★, solde inchangé", g.finir(s, true, 12) == 0 && solde(s) == 23 && !g.ouverte());
        g.depart();
        cas("RG-05 : 1er crédit de la course : 12 ★", g.finir(s, false, 12) == 12 && solde(s) == 35);
        cas("RG-05 : écran de fin rouvert / retour d'arrière-plan : aucun 2e crédit", g.finir(s, false, 12) == 0 && g.finir(s, true, 12) == 0 && solde(s) == 35);
        g.depart();
        cas("RG-02 : calcul impossible : secours 10 ★ (gain_course)", g.finir(s, false, -1) == 10 && solde(s) == 45);
        // entrée du score d'après la course du profil
        CourseJouee c; c.mode = ModeJoue::Course5; c.place = 2; c.tempsMs = 101000;
        RecordsDepart r; r.tempsCourse5Ms = 100000;
        score::ResultatCourse e = resultatScore(c, r, 0, -1);
        cas("course à 5 : place, temps contre le record (101 s pour 100 s : proche), chocs", e.terminee && e.place == 2 && e.temps == 101.0 && e.record == 100.0 && e.chocs == 0);
        CourseJouee q = c; q.quittee = true;
        cas("course quittée : pas terminée (0 ★ au calcul)", !resultatScore(q, r, 0, -1).terminee && score::calculer(resultatScore(q, r, 0, -1)).etoiles == 0);
        CourseJouee so; so.mode = ModeJoue::Checkpoint; so.score = 5000;
        RecordsDepart rs; rs.pointsSolo = 4000;
        e = resultatScore(so, rs, 3, -1);
        cas("course solo : 5 000 points pour un record de 4 000 : record battu", score::calculer(e).nouveauRecord && e.pointsDistance == 5000);
        cas("course solo sans record : 1re course", resultatScore(so, RecordsDepart{}, 3, -1).record <= 0);
        CourseJouee du; du.mode = ModeJoue::Duel; du.victoire = true;
        cas("duel : adversaire parti à 12 s : 0 ★ (RC-E22-04)", score::calculer(resultatScore(du, RecordsDepart{}, 0, 12)).etoiles == 0);
        cas("duel gagné : aucun point de temps (pas de record de duel)", score::calculer(resultatScore(du, RecordsDepart{}, 0, -1)).etoiles > 0 && resultatScore(du, RecordsDepart{}, 0, -1).record > 0);
        CourseJouee po; po.mode = ModeJoue::Poursuite; po.score = 400;
        e = resultatScore(po, RecordsDepart{}, 1, -1);
        cas("Poursuite perdue : score restant, score de départ de configgame, 1re course", e.scorePoursuite == 400 && e.scorePoursuiteDepart == config().poursuite.scoreDepart && e.record <= 0);
        // RG-04 : banc d'équilibrage avec le vrai calcul (valeurs du code = configgame.json)
        const Equilibrage eq = equilibrage();
        int ecarts = 0;
        SimulationJoueur sim[3];
        for (int j = 0; j < 3; j++) {
            sim[j] = simulerJoueur((Joueur)j, eq);
            for (const std::string& l : bilanJoueur(sim[j])) std::printf("  %s\n", l.c_str());
            for (const std::string& l : verifierJoueur(sim[j])) { std::printf("  écart : %s\n", l.c_str()); ecarts++; }
        }
        cas("RG-04 : joueur moyen dans les bornes (22 à 28 courses, 108 à 132 h), débutant ≤ 40 courses", ecarts == 0);
        cas("RG-04 : bon joueur plus rapide que le moyen, débutant plus lent", sim[1].coursesPremiere <= sim[0].coursesPremiere && sim[2].coursesPremiere >= sim[0].coursesPremiere &&
            sim[1].heuresVoitures < sim[0].heuresVoitures && sim[2].heuresVoitures > sim[0].heuresVoitures);
        cas("RG-04 : étoiles bornées entre 5 et 40 pour chaque course simulée", sim[0].etoilesMin >= 5 && sim[1].etoilesMax <= 40 && sim[2].etoilesMin >= 5);
        const SimulationJoueur bis = simulerJoueur(Joueur::Moyen, eq);
        cas("RG-04 : simulation déterministe (même graine, même résultat)", bis.coursesPremiere == sim[0].coursesPremiere && bis.coursesVoitures == sim[0].coursesVoitures);
        // un barème changé dans configgame.json change le résultat, sans recompiler
        const ConfigJeu sauve = config(); config().scoreEtoiles.pointsParEtoile = 40;
        const SimulationJoueur plus = simulerJoueur(Joueur::Moyen, eq);
        std::printf("  points_par_etoile = 40 : %s\n", bilanJoueur(plus)[0].c_str());
        cas("RG-04 : points_par_etoile 40 : plus d'étoiles, hors bornes signalé", plus.etoilesMoyennes > sim[0].etoilesMoyennes && !verifierJoueur(plus).empty());
        config() = sauve;
    }
    std::printf("%s : %d échec(s)\n", g_echecs ? "ÉCHEC" : "OK", g_echecs);
    return g_echecs ? 1 : 0;
}
