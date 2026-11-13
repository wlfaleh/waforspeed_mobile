// WaForSpeed — banc d'équilibrage des prix (WFS-498, RG-11 ; WFS-536, RG-04) : WFS_BANC=equilibrage, sans rendu ; voir boutique.h.
// Fichier à part (moteur, banc et test_boutique) : il lit configgame.json (chargerConfig) et lie le calcul du score (score_course.cpp),
// que les autres tests CPU ne lient pas.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
#include "../core/configgame.h"
#include "../core/log.h"
#include "../core/math.h"
#include "boutique.h"

namespace wfs::boutique {

namespace {

// profil d'un joueur simulé (WFS-536, RG-04 : « profils de place, chocs et défis dans le banc »). Parts de 0 à 1.
struct ProfilJoueur {
    double place[5];                 // course à 5 : part des courses finies à la 1re … 5e place
    double chocs0, chocs12;          // part des courses sans choc, avec 1 ou 2 chocs (le reste : 3 à 6 chocs)
    double duelGagne, poursuiteGagnee;
    int poursuiteRestantMax;         // Poursuite perdue : score restant tiré entre 0 et cette valeur
    int soloMin, soloMax;            // course solo : points de distance de la course
    double tempsBattu, tempsProche;  // part des courses (avec record) qui battent le record, qui l'approchent à moins de 5 %
    double tempsBase;                // course à 5 : temps de la 1re course (s)
};

constexpr ProfilJoueur PROFILS[3] = {
    // moyen : milieu de classement, 1 ou 2 chocs le plus souvent, un duel sur deux, une Poursuite sur quatre
    {{0.10, 0.20, 0.30, 0.25, 0.15}, 0.25, 0.45, 0.50, 0.25, 600, 3000, 15000, 0.10, 0.30, 190},
    // bon : souvent sur le podium, conduite propre, gagne la plupart des duels et des Poursuites
    {{0.40, 0.30, 0.20, 0.07, 0.03}, 0.50, 0.40, 0.75, 0.60, 900, 8000, 25000, 0.25, 0.45, 175},
    // débutant : fin de classement, beaucoup de chocs, perd presque toujours
    {{0.02, 0.08, 0.20, 0.30, 0.40}, 0.05, 0.30, 0.25, 0.05, 300, 1000, 6000, 0.02, 0.13, 220},
};

constexpr long long COURSES_MAX = 200000;   // garde-fou : jamais de boucle sans fin (gain nul, prix énormes)

double tirage(Mulberry32& h) { return h.next(); }
int entre(Mulberry32& h, int a, int b) { return a + (int)std::min<double>(b - a, std::floor(tirage(h) * (b - a + 1))); }

std::string heures(double h) { char b[32]; std::snprintf(b, sizeof b, "%.1f h", h); return b; }

} // namespace

const char* nomJoueur(Joueur j) {
    switch (j) {
        case Joueur::Moyen: return "moyen";
        case Joueur::Bon: return "bon";
        case Joueur::Debutant: return "débutant";
    }
    return "?";
}

SimulationJoueur simulerJoueur(Joueur j, const Equilibrage& e) {
    const ProfilJoueur& p = PROFILS[(int)j];
    Mulberry32 h(536u + (uint32_t)j);   // graine fixe : même résultat à chaque lancement
    SimulationJoueur sj; sj.joueur = j;
    sj.etoilesMin = 1 << 30;
    RecordsDepart rec;                  // records simulés du joueur (aucun au départ)
    const long long cibleVoitures = e.totalVoitures, cibleTout = e.totalVoitures + e.totalPersonnalisation;
    long long cumul = 0, n = 0;
    while (n < COURSES_MAX && (cumul < cibleTout || cumul < e.prixPremiere)) {
        CourseJouee c;
        c.mode = (ModeJoue)(n % 4);     // à tour de rôle : course solo, course à 5, Poursuite, duel
        const double uc = tirage(h);
        const int chocs = uc < p.chocs0 ? 0 : uc < p.chocs0 + p.chocs12 ? entre(h, 1, 2) : entre(h, 3, 6);
        const double ut = tirage(h);
        const int temps = ut < p.tempsBattu ? 0 : ut < p.tempsBattu + p.tempsProche ? 1 : 2;   // battu, proche, loin
        switch (c.mode) {
            case ModeJoue::Course5: {
                double u = tirage(h); c.place = 5;
                for (int i = 0; i < 5; i++) { if (u < p.place[i]) { c.place = i + 1; break; } u -= p.place[i]; }
                c.victoire = c.place == 1;
                const double rs = rec.tempsCourse5Ms / 1000.0;
                const double s = rs <= 0 ? p.tempsBase : temps == 0 ? rs * 0.99 : temps == 1 ? rs * 1.03 : rs * 1.10;
                c.tempsMs = std::llround(s * 1000);
                break;
            }
            case ModeJoue::Checkpoint: {
                const long long r = rec.pointsSolo;
                c.score = r <= 0 ? entre(h, p.soloMin, p.soloMax) : temps == 0 ? r + r / 100 + 1 : temps == 1 ? r * 98 / 100 : r * 85 / 100;
                break;
            }
            case ModeJoue::Poursuite:
                c.victoire = tirage(h) < p.poursuiteGagnee;
                c.score = c.victoire ? entre(h, config().poursuite.scoreDepart / 2, config().poursuite.scoreDepart) : entre(h, 0, p.poursuiteRestantMax);
                break;
            case ModeJoue::Duel:
                c.victoire = tirage(h) < p.duelGagne;
                break;
        }
        const score::Score s = score::calculer(resultatScore(c, rec, chocs, -1));
        // records du joueur simulé, comme le jeu à l'arrivée
        if (c.mode == ModeJoue::Course5 && (rec.tempsCourse5Ms <= 0 || c.tempsMs < rec.tempsCourse5Ms)) rec.tempsCourse5Ms = c.tempsMs;
        if (c.mode == ModeJoue::Checkpoint && c.score > rec.pointsSolo) rec.pointsSolo = c.score;
        if (c.mode == ModeJoue::Poursuite) rec.poursuite = true;
        cumul += s.etoiles; n++;
        sj.etoilesMin = std::min(sj.etoilesMin, s.etoiles); sj.etoilesMax = std::max(sj.etoilesMax, s.etoiles);
        if (!sj.coursesPremiere && cumul >= e.prixPremiere) sj.coursesPremiere = n;
        if (!sj.coursesVoitures && cumul >= cibleVoitures) sj.coursesVoitures = n;
        if (!sj.coursesTout && cumul >= cibleTout) sj.coursesTout = n;
    }
    if (!n) sj.etoilesMin = 0;
    sj.etoilesMoyennes = n ? (double)cumul / n : 0;
    sj.heuresVoitures = sj.coursesVoitures * e.minutesParCourse / 60.0;
    sj.heuresTout = sj.coursesTout * e.minutesParCourse / 60.0;
    return sj;
}

std::vector<std::string> bilanJoueur(const SimulationJoueur& sj) {
    char b[320];
    std::snprintf(b, sizeof b, "ÉQUILIBRAGE (score) joueur %s : %.2f ★ par course en moyenne (%d à %d ★) ; 1re voiture : %lld courses ; "
                  "toutes les voitures : %lld courses, %s ; tout : %lld courses, %s",
                  nomJoueur(sj.joueur), sj.etoilesMoyennes, sj.etoilesMin, sj.etoilesMax, sj.coursesPremiere,
                  sj.coursesVoitures, heures(sj.heuresVoitures).c_str(), sj.coursesTout, heures(sj.heuresTout).c_str());
    return {b};
}

std::vector<std::string> verifierJoueur(const SimulationJoueur& sj) {
    std::vector<std::string> v;
    const std::string qui = std::string("joueur ") + nomJoueur(sj.joueur) + " : ";
    if (sj.joueur == Joueur::Moyen) {
        if (sj.coursesPremiere < 22 || sj.coursesPremiere > 28) v.push_back(qui + "1re voiture en " + std::to_string(sj.coursesPremiere) + " courses, hors de 22 à 28");
        if (sj.heuresVoitures < 108 || sj.heuresVoitures > 132) v.push_back(qui + "toutes les voitures en " + heures(sj.heuresVoitures) + ", hors de 108 à 132 h");
    }
    if (sj.joueur == Joueur::Debutant && (sj.coursesPremiere <= 0 || sj.coursesPremiere > 40))
        v.push_back(qui + "1re voiture en " + std::to_string(sj.coursesPremiere) + " courses, plus de 40");
    return v;
}

int benchEquilibrage(const std::string& out) {
    const BilanConfig bc = chargerConfig();   // PC : WFS_CONFIG, sinon ./configgame.json (un prix changé compte sans recompiler)
    WFS_LOG("ÉQUILIBRAGE : configuration %s (%d clés lues, %d erreur(s))", bc.fichier.empty() ? "du code (aucun fichier)" : bc.fichier.c_str(), bc.lues, bc.erreurs);
    const Equilibrage e = equilibrage();
    std::vector<std::string> lignes = bilanEquilibrage(e);
    // WFS-536 (RG-02) : le gain fixe n'est plus qu'un secours ; ses bornes sont données pour information
    for (const std::string& l : verifierEquilibrage(e)) lignes.push_back("ÉQUILIBRAGE (secours gain_course, information) : " + l);
    // WFS-536 (RG-04) : les étoiles du score, joueurs moyen, bon et débutant
    std::vector<std::string> ecarts;
    for (Joueur j : {Joueur::Moyen, Joueur::Bon, Joueur::Debutant}) {
        const SimulationJoueur sj = simulerJoueur(j, e);
        for (const std::string& l : bilanJoueur(sj)) lignes.push_back(l);
        for (const std::string& l : verifierJoueur(sj)) ecarts.push_back(l);
    }
    for (const std::string& l : ecarts) lignes.push_back("ÉQUILIBRAGE ÉCART : " + l);
    lignes.push_back(ecarts.empty() ? "ÉQUILIBRAGE : OK (bornes de RG-04 de WFS-536 tenues)" : "ÉQUILIBRAGE : ÉCHEC (" + std::to_string(ecarts.size()) + " écart(s))");
    std::string texte;
    for (const std::string& l : lignes) { WFS_LOG("%s", l.c_str()); texte += l + "\n"; }
    std::error_code ec; std::filesystem::create_directories(out, ec);   // dossier des captures du banc (absent au 1er lancement)
    const std::string chemin = out + "/equilibrage.txt";
    if (!ecrireAtomique(chemin, texte)) WFS_ERR("équilibrage : %s non écrit", chemin.c_str());
    return (int)ecarts.size();
}

} // namespace wfs::boutique
