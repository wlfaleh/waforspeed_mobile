// WFS-24 : sauvegarde. Écrire puis relire toutes les clés ; fichier corrompu → défauts ; identifiant inconnu → défaut ;
// record écrit seulement s'il est battu ; écriture atomique sur le thread de travail.
// usage : test_sauvegarde [dossier temporaire] (par défaut : dossier courant)
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include "../src/game/sauvegarde.h"
using namespace wfs;

static int g_echecs = 0;
#define VERIFIER(c) do { if (!(c)) { std::printf("ÉCHEC ligne %d : %s\n", __LINE__, #c); g_echecs++; } } while (0)

static bool estDefaut(const Sauvegarde& s) {
    return s.best == 0 && s.car == "golf" && s.paint.empty() && s.map == "ville" && s.tod == "nuit" && s.quality == "high" && !s.qualityChoisie && s.music &&
           s.cam == "poursuite" && !s.muted && s.nick == "Pilote" && s.km == 4 && s.steer == "glisser" && s.diff5 == "normale" && s.aideDirection;   // WFS-217 : aide à la direction activée
}
static std::string lireFichier(const std::string& p) {
    std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str();
}

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? std::string(argv[1]) + "/" : "";
    const std::string chemin = dir + "test-sauvegarde.txt";
    std::remove(chemin.c_str());

    // 1) défauts du jeu web, fichier absent
    { Sauvegarde s; VERIFIER(estDefaut(s)); VERIFIER(!s.actif()); VERIFIER(!s.charger(chemin)); VERIFIER(s.actif()); VERIFIER(estDefaut(s)); }

    // 2) écrire toutes les clés (thread de travail, fichier temporaire puis renommage) puis relire
    {
        Sauvegarde s; s.charger(chemin);
        s.best = 48210; s.car = "m4"; s.paint["m4"] = "rouge"; s.paint["golf"] = "vert"; s.map = "circuit"; s.tod = "jour";
        s.setQualiteIndex(0); s.setCamIndex(2); s.muted = true; s.music = false; s.nick = "Zoé"; s.km = 10; s.steer = "fleches"; s.diff5 = "difficile";
        s.aideDirection = false;   // WFS-217
        s.enregistrer(); s.attendre();
        std::FILE* t = std::fopen((chemin + ".tmp").c_str(), "rb");
        VERIFIER(t == nullptr);   // le temporaire a été renommé
        if (t) std::fclose(t);
        const std::string c = lireFichier(chemin);
        VERIFIER(c.rfind("version=1\n", 0) == 0);
        VERIFIER(c.find("waforspeed-best=48210\n") != std::string::npos);
        VERIFIER(c.find("aide=desactivee\n") != std::string::npos);   // WFS-217
        std::printf("fichier écrit :\n%s", c.c_str());
    }
    {
        Sauvegarde s; VERIFIER(s.charger(chemin));
        VERIFIER(s.best == 48210); VERIFIER(s.car == "m4"); VERIFIER(s.peinture("m4") == "rouge"); VERIFIER(s.peinture("golf") == "vert");
        VERIFIER(s.peinture("p911") == "gris");   // non choisie : peinture d'origine (myPaint)
        VERIFIER(s.map == "circuit"); VERIFIER(s.tod == "jour"); VERIFIER(s.quality == "low"); VERIFIER(s.cam == "cockpit");
        VERIFIER(s.camIndex() == 2); VERIFIER(s.muted); VERIFIER(!s.music); VERIFIER(s.qualityChoisie); VERIFIER(s.nick == "Zoé"); VERIFIER(s.km == 10); VERIFIER(s.steer == "fleches"); VERIFIER(s.diff5 == "difficile");   // WFS-133
        VERIFIER(!s.aideDirection);   // WFS-217 : « Aide à la direction : Désactivée » reprise au lancement suivant
        // une seconde écriture remplace la première (dernière valeur gagnante)
        s.best = 50000; s.enregistrer(); s.car = "p911"; s.enregistrer(); s.attendre();
        Sauvegarde r; r.charger(chemin); VERIFIER(r.best == 50000); VERIFIER(r.car == "p911");
    }

    // 3) fichier corrompu → défauts, sans plantage
    for (const char* bad : {"", "n'importe quoi\ncar=m4\n", "version=2\ncar=m4\n", "\x01\x02\xff\xfe", "car=m4\nversion=1\n"}) {
        { std::ofstream f(chemin, std::ios::binary); f << bad; }
        Sauvegarde s; VERIFIER(!s.charger(chemin)); VERIFIER(estDefaut(s));
    }

    // 4) identifiant ou valeur inconnue → défaut de cette clé seule
    {
        Sauvegarde s;
        VERIFIER(s.lire("version=1\r\ncar=fusee\npaint-golf=fuchsia\npaint-fusee=rouge\nmap=lune\ntod=aube\nquality=ultra\ncam=drone\n"
                        "muted=oui\nnick=   \nkm=5\nsteer=volant\ndiff5=extreme\naide=peut-etre\nwaforspeed-best=-3\nligne sans egal\ninconnue=1\n"));
        VERIFIER(estDefaut(s));
        VERIFIER(s.lire("version=1\ntrace=2\nwaforspeed-car=gallardo\nbest=12\nkm=2\nnick=\tUn pseudo beaucoup trop long\n"));
        VERIFIER(s.car == "gallardo"); VERIFIER(s.best == 12); VERIFIER(s.km == 2);
        VERIFIER(s.nick == "Un pseudo beau");   // 14 caractères (cleanName du jeu web)
        VERIFIER(s.peinture("gallardo") == "jaune");
        VERIFIER(s.peinture("sf90") == "rouge"); VERIFIER(s.peinture("x5") == "blanc");   // voitures de WFS-13
        VERIFIER(s.lire("version=1\ncar=sf90\npaint-sf90=turquoise\nquality=medium\ncam=proche\n"));
        VERIFIER(s.car == "sf90"); VERIFIER(s.peinture("sf90") == "turquoise"); VERIFIER(s.qualiteIndex() == 1); VERIFIER(s.camIndex() == 3);   // WFS-85 : vue Proche
    }

    // 5) record (endRace) : écrit seulement s'il est battu
    {
        Sauvegarde s; s.best = 1000;
        VERIFIER(!s.battreRecord(999)); VERIFIER(!s.battreRecord(1000)); VERIFIER(s.best == 1000);
        VERIFIER(s.battreRecord(1001)); VERIFIER(s.best == 1001);
    }
    // 6) sauvegarde inactive (jamais chargée) : enregistrer() n'écrit rien
    {
        std::remove(chemin.c_str());
        Sauvegarde s; s.best = 7; s.enregistrer(); s.attendre();
        std::FILE* f = std::fopen(chemin.c_str(), "rb"); VERIFIER(f == nullptr); if (f) std::fclose(f);
    }
    std::remove(chemin.c_str());
    // WFS-122 (RG-01) : derniers adversaires du duel : 3 au plus, le plus récent en tête, sans doublon, relus tels quels
    {
        Sauvegarde s;
        VERIFIER(s.adversaires.empty() && s.texte().find("adversaires=") == std::string::npos);   // aucun duel : rien
        s.noterAdversaires({"Nadia"}); s.noterAdversaires({"Sam", "Lou"}); s.noterAdversaires({"Nadia", "Ka|rim"});
        // WFS-183 : une manche passe en tête dans l'ordre reçu (classement) : Sam, Lou puis Nadia, Ka rim en haut
        VERIFIER(s.adversaires.size() == 3 && s.adversaires[0] == "Nadia" && s.adversaires[1] == "Ka rim" && s.adversaires[2] == "Sam");
        Sauvegarde r; VERIFIER(r.lire(s.texte()));
        VERIFIER(r.adversaires == s.adversaires);
        Sauvegarde h; VERIFIER(h.lire("version=1\ntrace=2\nadversaires=" + std::string(500, 'x') + "|||Bob\n"));
        VERIFIER(h.adversaires.size() == 2 && h.adversaires[0].size() <= 14 && h.adversaires[1] == "Bob");   // pseudos bornés, vides ignorés
    }
    // WFS-183 (RG-01, ANO-93) : même manche → ordre du classement (mieux classé en haut, « Parti » en dernier), quel que soit
    // l'ordre du départ (identifiant du relais) ; deux tirages du relais différents donnent la même liste
    {
        Sauvegarde a, b;
        a.noterAdversaires({"Lou", "Nadia", "Sam"});   // départ, ordre de l'identifiant du relais (tirage 1)
        b.noterAdversaires({"Sam", "Lou", "Nadia"});   // départ, autre tirage
        a.noterAdversaires({"Nadia", "Sam", "Lou"}); b.noterAdversaires({"Nadia", "Sam", "Lou"});   // arrivée : classement
        VERIFIER(a.adversaires == b.adversaires);
        VERIFIER(a.adversaires.size() == 3 && a.adversaires[0] == "Nadia" && a.adversaires[1] == "Sam" && a.adversaires[2] == "Lou");
        Sauvegarde c; c.noterAdversaires({"Sam", "Nadia", "Lou"});   // Lou « Parti » : dernier du classement
        VERIFIER(c.adversaires.size() == 3 && c.adversaires[0] == "Sam" && c.adversaires[2] == "Lou");
        Sauvegarde r; VERIFIER(r.lire(c.texte())); VERIFIER(r.adversaires == c.adversaires);   // ordre relu tel quel
    }
    std::printf(g_echecs ? "ÉCHEC (%d)\n" : "OK\n", g_echecs);
    return g_echecs ? 1 : 0;
}
