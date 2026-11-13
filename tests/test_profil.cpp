// WFS-480 (RG-12) : profil du joueur. Pseudo et plaque (limites, caractères, mots interdits avec majuscules, accents
// et « leet », liste blanche), plaque par défaut, statistiques après 3 courses, 21 courses → 20 gardées, voiture préférée,
// écriture / relecture, lecture d'une sauvegarde d'avant ce ticket (records gardés, profil=0).
// usage : test_profil [liste des mots interdits] [dossier temporaire]   (à lancer depuis la racine du dépôt)
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include "../src/game/profil.h"
#include "../src/game/sauvegarde.h"
using namespace wfs;
using profil::Refus;

static int g_echecs = 0;
#define VERIFIER(c) do { if (!(c)) { std::printf("ÉCHEC ligne %d : %s\n", __LINE__, #c); g_echecs++; } } while (0)

static std::string lireFichier(const std::string& p) {
    std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str();
}
static uint32_t g_graine = 7;
static uint32_t hasard() { g_graine = g_graine * 1664525u + 1013904223u; return g_graine >> 8; }

static CourseJouee course(ModeJoue m, const char* voiture, long long metres, int vmax, int frol, long long date) {
    CourseJouee c; c.mode = m; c.voiture = voiture; c.metres = metres; c.vmax = vmax; c.frolements = frol; c.date = date;
    return c;
}

int main(int argc, char** argv) {
    const std::string liste = argc > 1 ? argv[1] : "assets/profil/mots_interdits.txt";
    const std::string dir = argc > 2 ? std::string(argv[2]) + "/" : "";

    // ---- RG-04 : liste chargée (≥ 150 mots)
    const std::string contenu = lireFichier(liste);
    VERIFIER(!contenu.empty());
    const int n = profil::chargerMotsInterdits(contenu);
    std::printf("mots interdits : %d\n", n);
    VERIFIER(n >= 150);

    // ---- RG-01 : pseudo
    std::string p;
    VERIFIER(profil::verifierPseudo("ab") == Refus::TropCourt);                         // 2
    VERIFIER(profil::verifierPseudo("abc") == Refus::Aucun);                            // 3
    VERIFIER(profil::verifierPseudo("Abcdefghij_-1234") == Refus::Aucun);               // 16
    VERIFIER(profil::verifierPseudo("Abcdefghij_-12345") == Refus::TropLong);           // 17
    VERIFIER(profil::verifierPseudo("   ab   ") == Refus::TropCourt);                   // espaces de bord retirés avant de compter
    VERIFIER(profil::verifierPseudo("  Max Speed  ", &p) == Refus::Aucun && p == "Max Speed");
    VERIFIER(profil::verifierPseudo("Max  Speed") == Refus::CaractereInterdit);         // deux espaces de suite
    VERIFIER(profil::verifierPseudo("Max!") == Refus::CaractereInterdit);
    VERIFIER(profil::verifierPseudo("Zoé") == Refus::CaractereInterdit);                // lettres A-Z / a-z seulement
    VERIFIER(profil::verifierPseudo("") == Refus::TropCourt);
    VERIFIER(std::string(profil::motif(Refus::TropCourt)) == "trop court");
    VERIFIER(std::string(profil::motif(Refus::MotInterdit)) == "mot interdit");

    // ---- RG-02 : plaque
    VERIFIER(profil::verifierPlaque("") == Refus::TropCourt);                           // 0
    VERIFIER(profil::verifierPlaque("a", &p) == Refus::Aucun && p == "A");              // 1, minuscule → majuscule
    VERIFIER(profil::verifierPlaque("ab-123cd", &p) == Refus::Aucun && p == "AB-123CD");   // 8
    VERIFIER(profil::verifierPlaque("AB-123CDE") == Refus::TropLong);                   // 9
    VERIFIER(profil::verifierPlaque("-AB") == Refus::CaractereInterdit);
    VERIFIER(profil::verifierPlaque("AB-") == Refus::CaractereInterdit);
    VERIFIER(profil::verifierPlaque("A--B") == Refus::CaractereInterdit);
    VERIFIER(profil::verifierPlaque("AB 12") == Refus::CaractereInterdit);
    VERIFIER(profil::verifierPlaque("AB_12") == Refus::CaractereInterdit);

    // ---- RG-04 : mots interdits (majuscules, accents, leet, séparateurs), liste blanche
    VERIFIER(profil::normaliser("C0nN4rD_É-1") == "connardei");
    VERIFIER(profil::contientMotInterdit("connard"));
    VERIFIER(profil::contientMotInterdit("CONNARD"));
    VERIFIER(profil::contientMotInterdit("Enculé"));                 // accent
    VERIFIER(profil::contientMotInterdit("ENCULÉ"));
    VERIFIER(profil::contientMotInterdit("c0nn4rd"));                // leet
    VERIFIER(profil::contientMotInterdit("N4Z1"));
    VERIFIER(profil::contientMotInterdit("$h1t"));
    VERIFIER(profil::contientMotInterdit("f_u-c k"));                // séparateurs retirés
    VERIFIER(profil::contientMotInterdit("xXHitlerXx"));             // contenu dans le texte
    VERIFIER(!profil::contientMotInterdit("Scunthorpe"));            // liste blanche
    VERIFIER(!profil::contientMotInterdit("Classe"));
    VERIFIER(!profil::contientMotInterdit("Bassin"));
    VERIFIER(!profil::contientMotInterdit("Cockpit"));
    VERIFIER(!profil::contientMotInterdit("Max Speed"));
    VERIFIER(!profil::contientMotInterdit("Conor"));                 // mots courts non listés : pas de faux refus
    VERIFIER(!profil::contientMotInterdit("Technique"));
    VERIFIER(profil::contientMotInterdit("Scunthorpe cunt"));        // la liste blanche ne couvre que son propre mot
    VERIFIER(profil::verifierPseudo("Connard42") == Refus::MotInterdit);
    VERIFIER(profil::verifierPseudo("N4zi_Racer") == Refus::MotInterdit);
    VERIFIER(profil::verifierPlaque("KKK-1") == Refus::MotInterdit);
    VERIFIER(profil::verifierPlaque("FDP") == Refus::MotInterdit);

    // ---- RG-03 : plaque proposée d'après le pseudo
    VERIFIER(profil::plaqueProposee("Max Speed", hasard) == "MAXSPEED");
    VERIFIER(profil::plaqueProposee("Le Grand Pilote 2026", hasard) == "LEGRANDP");
    VERIFIER(profil::plaqueProposee("Zoé_Racer", hasard) == "ZORACER");
    VERIFIER(profil::plaqueProposee("a", hasard) == "A");
    for (const char* vide : {"", "___", "Connard"}) {   // vide ou interdite : WFS- + 3 chiffres
        const std::string q = profil::plaqueProposee(vide, hasard);
        VERIFIER(q.size() == 7 && q.rfind("WFS-", 0) == 0 && q[4] >= '0' && q[4] <= '9' && q[6] >= '0' && q[6] <= '9');
        VERIFIER(profil::verifierPlaque(q) == Refus::Aucun);
    }

    // ---- RG-06 : création
    {
        Profil pr; std::string nick = "Pilote";
        VERIFIER(!pr.creer("ab", "AB-1", nick) && !pr.cree && nick == "Pilote");
        VERIFIER(!pr.creer("Max", "--", nick) && !pr.cree);
        VERIFIER(pr.creer("  Max Speed ", "max-1", nick) && pr.cree && nick == "Max Speed" && pr.plaque == "MAX-1");
        VERIFIER(Profil::pseudoPropose("Pilote").empty());   // RG-05
        VERIFIER(Profil::pseudoPropose("Zoé") == "Zoé");
    }

    // ---- RG-07, RG-09 : statistiques après 3 courses
    {
        Profil pr;
        VERIFIER(pr.voiturePreferee() == "—");
        CourseJouee a = course(ModeJoue::Checkpoint, "golf", 3450, 212, 4, 1000); a.score = 17250; a.victoire = true;   // Checkpoint : jamais une victoire
        CourseJouee b = course(ModeJoue::Course5, "m4", 4000, 251, 0, 2000); b.place = 1; b.tempsMs = 83450; b.victoire = true;
        CourseJouee c = course(ModeJoue::Poursuite, "m4", 1200, 198, 2, 3000); c.quittee = true; c.victoire = true;   // quittée : pas une victoire
        pr.noterCourse(a); pr.noterCourse(b); pr.noterCourse(c);
        VERIFIER(pr.courses == 3);
        VERIFIER(pr.metres == 8650);
        char km[16]; std::snprintf(km, sizeof km, "%.1f", pr.km()); VERIFIER(std::string(km) == "8.7");
        VERIFIER(pr.vmax == 251);
        VERIFIER(pr.frolements == 6);
        VERIFIER(pr.victoires == 1);
        VERIFIER(pr.voiturePreferee() == "m4");   // 5 200 m contre 3 450 m
        VERIFIER(pr.parVoiture["m4"].courses == 2 && pr.parVoiture["golf"].courses == 1);
        VERIFIER(pr.dernieres.size() == 3 && pr.dernieres[0].mode == ModeJoue::Poursuite && pr.dernieres[2].mode == ModeJoue::Checkpoint);
        VERIFIER(pr.dernieres[1].resultat() == "1re en 1:23,45");
        VERIFIER(pr.dernieres[0].resultat() == "Quittée");
        VERIFIER(pr.dernieres[2].resultat() == "17250 pts");
        // égalité de km : la voiture qui a le plus de courses
        Profil e; e.noterCourse(course(ModeJoue::Duel, "p911", 2000, 0, 0, 1)); e.noterCourse(course(ModeJoue::Duel, "golf", 1000, 0, 0, 2));
        e.noterCourse(course(ModeJoue::Duel, "golf", 1000, 0, 0, 3));
        VERIFIER(e.voiturePreferee() == "golf");
        CourseJouee d = course(ModeJoue::Duel, "golf", 0, 0, 0, 4); d.victoire = true; VERIFIER(d.resultat() == "Gagné");
        d.victoire = false; VERIFIER(d.resultat() == "Perdu");
        CourseJouee q = course(ModeJoue::Poursuite, "golf", 0, 0, 0, 4); q.victoire = true; q.score = 900; VERIFIER(q.resultat() == "Échappé · 900 pts");
    }

    // ---- RG-10 : 21 courses → 20 gardées, la plus récente en premier
    {
        Profil pr;
        for (int i = 1; i <= 21; i++) { CourseJouee c = course(ModeJoue::Checkpoint, "golf", 100, 100, 0, i); c.score = i; pr.noterCourse(c); }
        VERIFIER(pr.courses == 21);
        VERIFIER(pr.dernieres.size() == 20);
        VERIFIER(pr.dernieres.front().date == 21 && pr.dernieres.back().date == 2);   // la 1re a été effacée
    }

    // ---- RG-11 : écriture puis relecture dans la sauvegarde (fichier temporaire puis renommage)
    const std::string chemin = dir + "test-profil.txt";
    std::remove(chemin.c_str());
    {
        Sauvegarde s; s.charger(chemin);
        VERIFIER(!s.profil.cree && s.profil.arrivee > 0);   // RG-05 : date du 1er lancement
        VERIFIER(s.profil.creer("Max Speed", "MAX-1", s.nick));
        s.profil.arrivee = 1794530000;
        for (int i = 1; i <= 21; i++) {
            CourseJouee c = course(i % 2 ? ModeJoue::Course5 : ModeJoue::Poursuite, i % 3 ? "m4" : "golf", 1000 + i, 200 + i, i, 1794530000 + i);
            c.place = 2; c.tempsMs = 90000 + i; c.score = 10 * i; c.victoire = i % 2 == 0; s.profil.noterCourse(c);
        }
        s.best = 48210; s.nick = "Abcdefghij_-1234";   // 16 caractères gardés (RG-01)
        s.enregistrer(); s.attendre();
        const std::string c = lireFichier(chemin);
        std::printf("fichier écrit :\n%s", c.c_str());
        VERIFIER(c.find("profil=1\n") != std::string::npos);
        VERIFIER(c.find("plaque=MAX-1\n") != std::string::npos);
        VERIFIER(c.find("arrivee=1794530000\n") != std::string::npos);
        VERIFIER(c.find("historique-20=") != std::string::npos && c.find("historique-21=") == std::string::npos);
        VERIFIER(c.find("waforspeed-best=48210\n") != std::string::npos);   // aucune ancienne clé retirée
    }
    {
        Sauvegarde s; VERIFIER(s.charger(chemin));
        VERIFIER(s.profil.cree && s.profil.plaque == "MAX-1" && s.profil.arrivee == 1794530000);
        VERIFIER(s.nick == "Abcdefghij_-1234");
        VERIFIER(s.profil.courses == 21 && s.profil.vmax == 221 && s.profil.frolements == 231);
        VERIFIER(s.profil.metres == 21 * 1000 + 231);
        VERIFIER(s.profil.dernieres.size() == 20 && s.profil.dernieres[0].date == 1794530021 && s.profil.dernieres[19].date == 1794530002);
        VERIFIER(s.profil.dernieres[0].mode == ModeJoue::Course5 && s.profil.dernieres[0].voiture == "golf" && s.profil.dernieres[0].tempsMs == 90021);
        VERIFIER(s.profil.dernieres[1].mode == ModeJoue::Poursuite && s.profil.dernieres[1].victoire && s.profil.dernieres[1].score == 200);
        VERIFIER(s.profil.voiturePreferee() == "m4");
        VERIFIER(s.best == 48210);
        std::printf("profil relu : %s\n", s.profil.texte(s.nick).c_str());
    }
    // pseudo de 17 caractères dans le fichier : coupé à 16 (nettoyerPseudo)
    {
        { std::ofstream f(chemin, std::ios::binary); f << "version=1\nnick=Abcdefghij_-12345\n"; }
        Sauvegarde s; s.charger(chemin);
        VERIFIER(s.nick == "Abcdefghij_-1234");
    }

    // ---- RG-12 : sauvegarde d'avant ce ticket : records gardés, profil=0, rien d'illisible
    {
        { std::ofstream f(chemin, std::ios::binary);
          f << "version=1\nwaforspeed-best=31337\ncar=m4\npaint-m4=rouge\nmap=ville\ntod=nuit\ncam=poursuite\nmuted=0\nmusic=1\nnick=Zoé\nkm=4\n"
               "steer=glisser\nmode=course5\nrecord-course5=2;83450\nrecord-poursuite=1;900;5400\ntrace=2\ntrace-ville=2\nadversaires=Alex|Sam\n"; }
        Sauvegarde s; VERIFIER(s.charger(chemin));
        VERIFIER(s.best == 31337 && s.rec5Place == 2 && s.rec5Ms == 83450 && s.recPVictoire && s.recPScore == 900 && s.recPDistM == 5400);
        VERIFIER(s.car == "m4" && s.nick == "Zoé" && s.adversaires.size() == 2);
        VERIFIER(!s.profil.cree && s.profil.plaque.empty() && s.profil.courses == 0 && s.profil.dernieres.empty());
        VERIFIER(s.profil.arrivee > 0);                          // RG-05 : date du 1er lancement de cette version
        VERIFIER(Profil::pseudoPropose(s.nick) == "Zoé");        // pseudo actuel proposé
        VERIFIER(s.texte().find("profil=0\n") != std::string::npos);
    }
    // ---- lignes du profil mal formées : ignorées, sans erreur ; profil=1 sans plaque → 0
    {
        { std::ofstream f(chemin, std::ios::binary);
          f << "version=1\nprofil=1\nplaque=--\nstats=1;2\nvoiture-../x=1;1\nhistorique-05=checkpoint;golf;10;0;0;0;0;5\nhistorique-99=checkpoint;golf;1;0;0;0;0;5\n"
               "historique-02=inconnu;golf;1;0;0;0;0;5\narrivee=abc\n"; }
        Sauvegarde s; VERIFIER(s.charger(chemin));
        VERIFIER(!s.profil.cree && s.profil.plaque.empty() && s.profil.courses == 0 && s.profil.parVoiture.empty());
        VERIFIER(s.profil.dernieres.size() == 1 && s.profil.dernieres[0].date == 10);   // trous retirés
    }
    std::remove(chemin.c_str());

    if (g_echecs) { std::printf("test_profil : %d échec(s)\n", g_echecs); return 1; }
    std::printf("test_profil : OK\n");
    return 0;
}
