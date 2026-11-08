// WFS-308 : conditions d'utilisation. Version de store/cgu.md = VERSION_CGU ; mention « relecture juridique conseillée » ;
// écran montré seulement si la version acceptée est plus ancienne (RG-04) ; clé cgu= écrite puis relue, invalide → 0 ;
// lecture du Markdown simple (titres, puces, gras, liens).
// usage (depuis la racine du dépôt) : test_cgu [dossier temporaire]
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include "../src/game/sauvegarde.h"
#include "../src/ui/ecrans/cgu.h"
using namespace wfs;
using namespace wfs::ecrans;

static int g_echecs = 0;
#define VERIFIER(c) do { if (!(c)) { std::printf("ÉCHEC ligne %d : %s\n", __LINE__, #c); g_echecs++; } } while (0)

static std::string lireFichier(const std::string& p) {
    std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str();
}

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? std::string(argv[1]) + "/" : "";

    // 1) texte des CGU : version, date, mention de relecture, faits exigés par RG-01
    {
        const std::string md = lireFichier("store/cgu.md");
        VERIFIER(!md.empty());
        VERIFIER(versionDansTexte(md) == VERSION_CGU);
        VERIFIER(md.find("relecture juridique conseillée") != std::string::npos);
        VERIFIER(md.find("Releases GitHub") != std::string::npos);
        VERIFIER(md.find("adresse IP") != std::string::npos);
        VERIFIER(md.find("Wi-Fi") != std::string::npos);
        VERIFIER(md.find("configgame.json") != std::string::npos);
        const std::vector<ParagrapheCgu> p = paragraphesMarkdown(md);
        VERIFIER(!p.empty() && p[0].style == ParagrapheCgu::TITRE);
        bool sansMarque = true;
        for (const ParagrapheCgu& q : p) if (q.texte.find("**") != std::string::npos || q.texte.find('`') != std::string::npos) sansMarque = false;
        VERIFIER(sansMarque);
        VERIFIER(!lireFichier("store/confidentialite.md").empty());   // affichée à la suite des CGU
    }

    // 2) Markdown simple
    {
        const std::vector<ParagrapheCgu> p = paragraphesMarkdown("# Titre\n\nUne **ligne**\nsuite.\n\n## Partie\n- puce [lien](https://a.b)\n  suite de puce\n\n\n*fin*\n");
        VERIFIER(p.size() == 8);
        if (p.size() == 8) {
            VERIFIER(p[0].style == ParagrapheCgu::TITRE && p[0].texte == "Titre");
            VERIFIER(p[1].style == ParagrapheCgu::VIDE);
            VERIFIER(p[2].style == ParagrapheCgu::TEXTE && p[2].texte == "Une ligne suite.");
            VERIFIER(p[4].style == ParagrapheCgu::SOUS_TITRE && p[4].texte == "Partie");
            VERIFIER(p[5].style == ParagrapheCgu::PUCE && p[5].texte == "puce lien (https://a.b) suite de puce");
            VERIFIER(p[6].style == ParagrapheCgu::VIDE);
            VERIFIER(p[7].style == ParagrapheCgu::TEXTE && p[7].texte == "fin");
        }
        VERIFIER(versionDansTexte("# T\n\nVersion 12 — x\n") == 12);
        VERIFIER(versionDansTexte("# T\n") == 0);
    }

    // 3) RG-04 : écran montré seulement si la version acceptée est plus ancienne ; jamais sans sauvegarde (banc d'essai)
    VERIFIER(cguAMontrer(true, 0));
    VERIFIER(cguAMontrer(true, VERSION_CGU - 1));
    VERIFIER(!cguAMontrer(true, VERSION_CGU));
    VERIFIER(!cguAMontrer(false, 0));

    // 4) clé cgu= de la sauvegarde : absente → 0, écrite puis relue, invalide → 0
    {
        const std::string chemin = dir + "test-cgu-sauvegarde.txt";
        std::remove(chemin.c_str());
        { Sauvegarde s; s.charger(chemin); VERIFIER(s.cgu == 0); s.cgu = VERSION_CGU; s.enregistrer(); s.attendre(); }
        VERIFIER(lireFichier(chemin).find("\ncgu=" + std::to_string(VERSION_CGU) + "\n") != std::string::npos);
        { Sauvegarde s; VERIFIER(s.charger(chemin)); VERIFIER(s.cgu == VERSION_CGU); }
        { Sauvegarde s; VERIFIER(s.lire("version=1\ncgu=abc\n")); VERIFIER(s.cgu == 0); }
        { Sauvegarde s; VERIFIER(s.lire("version=1\ncgu=-3\n")); VERIFIER(s.cgu == 0); }
        { Sauvegarde s; VERIFIER(s.lire("version=1\ncar=m4\n")); VERIFIER(s.cgu == 0); }   // ancienne sauvegarde : écran montré
        std::remove(chemin.c_str());
    }

    if (g_echecs) { std::printf("test_cgu : %d échec(s)\n", g_echecs); return 1; }
    std::printf("test_cgu : OK\n");
    return 0;
}
