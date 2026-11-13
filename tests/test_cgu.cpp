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
#include "../src/ui/langue_regles.h"   // WFS-508 : garde-fou « phrase française »
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

    // 5) WFS-508 : textes anglais (RG-01, RG-02) : présents, même version, phrase « fait foi » dans les 4 fichiers, même plan que le
    //    français (titres, sous-titres, puces : traduction fidèle, rien d'ajouté ni de retiré), aucune phrase française à l'écran
    {
        VERIFIER(std::string(fichierTexteCgu(false, false)) == "cgu.md" && std::string(fichierTexteCgu(false, true)) == "cgu-en.md");
        VERIFIER(std::string(fichierTexteCgu(true, false)) == "confidentialite.md" && std::string(fichierTexteCgu(true, true)) == "confidentialite-en.md");
        for (int conf = 0; conf < 2; conf++) {
            const std::string fr = lireFichier(std::string("store/") + fichierTexteCgu(conf, false));
            const std::string en = lireFichier(std::string("store/") + fichierTexteCgu(conf, true));
            VERIFIER(!fr.empty() && !en.empty());
            if (!conf) VERIFIER(versionDansTexte(en) == VERSION_CGU);
            VERIFIER(fr.find("la version française fait foi") != std::string::npos);
            VERIFIER(en.find("the French version prevails") != std::string::npos);
            const std::vector<ParagrapheCgu> pf = paragraphesMarkdown(fr), pe = paragraphesMarkdown(en);
            VERIFIER(pf.size() == pe.size());
            for (size_t i = 0; i < pf.size() && i < pe.size(); i++)
                if (pf[i].style != pe[i].style) { std::printf("paragraphe %zu (%s) : plan différent du français\n", i, conf ? "confidentialité" : "CGU"); g_echecs++; }
            // partie affichée dans le jeu (notes des stores retirées, titre français ou anglais)
            const std::string vuFr = sansNotesStore(fr), vuEn = sansNotesStore(en);
            if (conf) VERIFIER(vuFr.size() < fr.size() && vuEn.size() < en.size());
            for (const ParagrapheCgu& q : paragraphesMarkdown(vuEn)) {
                const std::string raison = ui::langue::phraseFrancaise(q.texte);
                if (!raison.empty()) { std::printf("ÉCHEC phrase française (%s) : « %s »\n", raison.c_str(), q.texte.c_str()); g_echecs++; }
            }
        }
        VERIFIER(lireFichier("store/cgu-en.md").find("legal review recommended") != std::string::npos);
    }

    // 6) WFS-508 (RG-05) : l'acceptation ne dépend pas de la langue (clé cgu= seule) : acceptée en français, rien à redemander en anglais
    { Sauvegarde s; VERIFIER(s.lire("version=1\ncgu=" + std::to_string(VERSION_CGU) + "\nlangue=en\n")); VERIFIER(!cguAMontrer(true, s.cgu)); }

    if (g_echecs) { std::printf("test_cgu : %d échec(s)\n", g_echecs); return 1; }
    std::printf("test_cgu : OK\n");
    return 0;
}
