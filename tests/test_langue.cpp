// Système de langue (WFS-504, épic E20, RG-06) : tables françaises et anglaises cohérentes, langue du téléphone (RG-01),
// clé langue= de la sauvegarde (RG-04), garde-fou « phrase française » (RG-07).
// usage : test_langue [dossier des actifs, « assets » par défaut]
#include <cstdio>
#include <fstream>
#include <sstream>
#include "../src/game/sauvegarde.h"
#include "../src/ui/langue_regles.h"
using namespace wfs;
using namespace wfs::ui::langue;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static bool lireTable(const std::string& chemin, std::map<std::string, std::string>& t) {
    std::ifstream f(chemin, std::ios::binary);
    if (!f) return false;
    std::stringstream ss; ss << f.rdbuf();
    const int illisibles = analyserTable(ss.str(), t);
    VERIFIE(illisibles == 0, "%s : %d ligne(s) illisible(s) ou clé(s) en double", chemin.c_str(), illisibles);
    return true;
}

int main(int argc, char** argv) {
    const std::string actifs = argc > 1 ? argv[1] : "assets";
    std::map<std::string, std::string> fr, en;
    VERIFIE(lireTable(actifs + "/textes/fr.txt", fr), "table française introuvable");
    VERIFIE(lireTable(actifs + "/textes/en.txt", en), "table anglaise introuvable");
    VERIFIE(fr.size() >= 60, "table française : %zu textes (au moins 60 : Réglages)", fr.size());
    // RG-06 : mêmes clés, aucun texte vide, mêmes {n}, aucune phrase française dans la table anglaise
    for (const auto& [k, v] : fr) {
        VERIFIE(en.count(k), "clé « %s » absente de en.txt", k.c_str());
        VERIFIE(!v.empty(), "fr.txt : texte vide pour « %s »", k.c_str());
        if (en.count(k)) VERIFIE(marqueurs(v) == marqueurs(en[k]), "clé « %s » : valeurs {n} différentes", k.c_str());
    }
    for (const auto& [k, v] : en) {
        VERIFIE(fr.count(k), "clé « %s » absente de fr.txt", k.c_str());
        VERIFIE(!v.empty(), "en.txt : texte vide pour « %s »", k.c_str());
        const std::string raison = phraseFrancaise(v);
        VERIFIE(raison.empty(), "en.txt : « %s » = « %s » : %s", k.c_str(), v.c_str(), raison.c_str());
    }
    VERIFIE(fr["reglages.langue.fr"] == "Français" && en["reglages.langue.fr"] == "Français" && fr["reglages.langue.en"] == "English" &&
            en["reglages.langue.en"] == "English", "RG-02 : « Français » et « English » écrits dans leur propre langue");
    // RG-01 : langue du téléphone
    VERIFIE(langueDuCode("fr") == Langue::Francais, "fr");
    VERIFIE(langueDuCode("fr-CA") == Langue::Francais, "fr-CA");
    VERIFIE(langueDuCode("fr-MA") == Langue::Francais, "fr-MA");
    VERIFIE(langueDuCode("fr_BE") == Langue::Francais, "fr_BE");
    VERIFIE(langueDuCode("FR") == Langue::Francais, "FR");
    for (const char* c : {"en", "en-GB", "ar", "es", "de", "", "f", "fra-x", "frr"}) VERIFIE(langueDuCode(c) == Langue::Anglais, "« %s » → anglais", c);
    VERIFIE(langueActive(Choix::Automatique, "fr-FR") == Langue::Francais && langueActive(Choix::Automatique, "de") == Langue::Anglais, "Automatique");
    VERIFIE(langueActive(Choix::Anglais, "fr") == Langue::Anglais && langueActive(Choix::Francais, "en") == Langue::Francais, "choix forcé");
    // RG-04 : clé langue de la sauvegarde
    {
        Sauvegarde s;
        VERIFIE(s.lire("version=1\ncar=golf\n") && s.langue == "auto", "sauvegarde sans langue → auto");
        VERIFIE(s.lire("version=1\nlangue=en\n") && s.langue == "en", "langue=en relue");
        VERIFIE(s.lire("version=1\nlangue=fr\n") && s.langue == "fr", "langue=fr relue");
        VERIFIE(s.lire("version=1\nlangue=xx\n") && s.langue == "auto", "langue inconnue → auto");
        s.langue = "en";
        Sauvegarde t; VERIFIE(t.lire(s.texte()) && t.langue == "en", "langue écrite puis relue");
        VERIFIE(s.texte().find("\ncar=") != std::string::npos, "aucune clé ancienne supprimée");
        VERIFIE(choixDeCle("auto") == Choix::Automatique && choixDeCle("en") == Choix::Anglais && choixDeCle("") == Choix::Automatique, "choixDeCle");
    }
    // RG-05 : valeurs {1} {2}, retour à la ligne
    VERIFIE(remplir("Lap {1}/{2}", {"2", "3"}) == "Lap 2/3", "remplir");
    VERIFIE(remplir("{1} km/h {3}", {"182"}) == "182 km/h ", "marqueur sans valeur : vide");
    { std::map<std::string, std::string> t; analyserTable("a=x\\ny\n# note\n\nb = c\n", t); VERIFIE(t["a"] == "x\ny" && t["b"] == " c", "analyserTable"); }
    // RG-07 : garde-fou « phrase française »
    VERIFIE(!phraseFrancaise("RÉGLAGES").empty() && !phraseFrancaise("Course en pause").empty() && !phraseFrancaise("POUR LA VILLE").empty(), "français détecté");
    VERIFIE(phraseFrancaise("SETTINGS").empty() && phraseFrancaise("FRANÇAIS").empty() && phraseFrancaise("Race paused").empty(), "anglais accepté");
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("langue : OK (%zu textes dans chaque table)\n", fr.size());
    return 0;
}
