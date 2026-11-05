// Configuration du jeu par configgame.json (WFS-128) : lecteur JSON maison, clés absentes / mauvais type / hors bornes /
// JSON cassé / clé inconnue (RG-02, RG-04, RG-05), nombres lus sans la langue (RG-16), et le fichier configgame.json du
// dépôt = valeurs du code à l'identique (RG-01, RG-16 : sans modification, rien ne change dans le jeu).
// usage : test_configgame [configgame.json]            → vérifications
//         test_configgame --ecrire <dossier>            → écrit configgame.json et configgame.README.md depuis le code
#include <cctype>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include "../src/core/configgame.h"
using namespace wfs;

static int echecs = 0;
static void cas(const char* quoi, bool ok) { std::printf("configuration : %-66s → %s\n", quoi, ok ? "OK" : "ÉCHEC"); echecs += !ok; }
static std::string remplacer(std::string s, const std::string& a, const std::string& b) {
    const size_t i = s.find(a); if (i != std::string::npos) s.replace(i, a.size(), b); return s;
}

int main(int argc, char** argv) {
    if (argc > 2 && !std::strcmp(argv[1], "--ecrire")) {
        const std::string d = argv[2];
        std::ofstream(d + "/configgame.json") << configJson(ConfigJeu{});
        std::ofstream(d + "/configgame.README.md") << configReadme();
        std::printf("écrit : %s/configgame.json (%d clés), %s/configgame.README.md\n", d.c_str(), configNombreCles(), d.c_str());
        return 0;
    }
    const int N = configNombreCles();
    const std::string defauts = configJson(ConfigJeu{});
    // fichier fabriqué depuis le code : tout est lu, rien ne change
    {
        ConfigJeu c; const BilanConfig b = lireConfig(defauts, "défauts", c);
        cas("fichier des défauts : JSON valide, toutes les clés lues, 0 erreur", b.jsonValide && b.lues == N && b.defauts == 0 && b.erreurs == 0);
        cas("fichier des défauts : valeurs identiques au code", configJson(c) == defauts);
        cas("distance des checkpoints : 600 m = 100 000 unités", c.checkpoint.distanceU == 100000.0);
    }
    // une clé modifiée est prise ; les autres gardent le code
    {
        ConfigJeu c; std::string t = remplacer(defauts, "\"depart\": 60.0", "\"depart\": 90.0");
        t = remplacer(t, "\"zoom\": 66.0", "\"zoom\": 75.5");
        const BilanConfig b = lireConfig(t, "modifié", c);
        cas("checkpoint.depart = 90 et camera.poursuite.zoom = 75,5 pris", c.checkpoint.depart == 90 && c.camera.fov[0] == 75.5f && b.erreurs == 0);
        cas("les autres clés gardent la valeur du code", c.checkpoint.bonus == 30 && c.camera.fov[3] == 60.f && c.conduite.grip == 18);
    }
    // RG-02 : clé absente → valeur du code, compté et signalé
    {
        ConfigJeu c; const std::string t = remplacer(defauts, "\"quasi_contact\": 250,", "");
        const BilanConfig b = lireConfig(t, "absente", c);
        cas("clé absente : valeur du code (250), 1 défaut", b.jsonValide && c.score.quasiContact == 250 && b.defauts == 1 && b.lues == N - 1);
    }
    // mauvais type : texte, booléen, réel pour un entier
    {
        ConfigJeu c; std::string t = remplacer(defauts, "\"quasi_contact\": 250", "\"quasi_contact\": \"beaucoup\"");
        t = remplacer(t, "\"adherence\": 18.0", "\"adherence\": true");
        t = remplacer(t, "\"ordinateurs\": 4", "\"ordinateurs\": 2.5");
        const BilanConfig b = lireConfig(t, "types", c);
        cas("mauvais type (texte, booléen, réel pour un entier) : valeurs du code", c.score.quasiContact == 250 && c.conduite.grip == 18 && c.course5.ordinateurs == 4 && b.erreurs == 3);
    }
    // RG-04 : hors bornes
    {
        ConfigJeu c; std::string t = remplacer(defauts, "\"depart\": 60.0", "\"depart\": 5000");
        t = remplacer(t, "\"densite\": 1.0", "\"densite\": -2");
        const BilanConfig b = lireConfig(t, "bornes", c);
        cas("hors bornes (5000 s, −2) : valeurs du code, 2 erreurs", c.checkpoint.depart == 60 && c.trafic.densite == 1 && b.erreurs == 2);
    }
    // JSON cassé : toutes les valeurs du code, jamais de plantage
    for (const char* casse : {"", "{", "{\"score\": {\"quasi_contact\": 250", "[1, 2]", "{\"score\": 1,}", "{\"a\": 0,5}", "{} {}", "{\"a\": tru}", "\x01\x02"}) {
        ConfigJeu c; c.score.quasiContact = 250; const BilanConfig b = lireConfig(casse, "cassé", c);
        if (b.jsonValide || c.score.quasiContact != 250 || b.defauts != N) { cas(("JSON cassé refusé : " + std::string(casse)).c_str(), false); }
    }
    cas("JSON cassé (9 textes, dont « 0,5 ») : refusé, valeurs du code", true);
    // clé inconnue : signalée, sans effet ; commentaires « _aide » ignorés
    {
        ConfigJeu c; const std::string t = remplacer(defauts, "\"score\": {", "\"score\": {\n    \"quasi_contacte\": 999,\n    \"_note\": \"x\",");
        const BilanConfig b = lireConfig(t, "inconnue", c);
        cas("clé inconnue signalée (1 erreur), commentaire « _note » ignoré", b.erreurs == 1 && b.lues == N && c.score.quasiContact == 250);
    }
    // RG-16 : nombres lus sans la langue (virgule décimale en français), exacts comme les littéraux du code
    {
        std::setlocale(LC_ALL, "fr_FR.UTF-8"); std::setlocale(LC_NUMERIC, "de_DE.UTF-8");
        ConfigJeu c; std::string t = remplacer(defauts, "\"trainee\": 0.00042", "\"trainee\": 4.2e-4");
        lireConfig(t, "langue", c);
        cas("langue française / allemande : 0.00042 et 4.2e-4 lus exactement", c.conduite.drag == 0.00042 && configJson(c) == defauts);
        std::setlocale(LC_ALL, "C");
        bool virgule = false;   // virgule entre deux chiffres = virgule décimale (la virgule JSON suit un espace, un chiffre puis un saut de ligne)
        for (size_t i = 1; i + 1 < defauts.size(); i++) virgule = virgule || (defauts[i] == ',' && std::isdigit((unsigned char)defauts[i - 1]) && std::isdigit((unsigned char)defauts[i + 1]));
        cas("fichier écrit avec des points décimaux, jamais de virgule", !virgule && defauts.find("e+") == std::string::npos);
    }
    // police au départ > police au plus : corrigé
    {
        ConfigJeu c; std::string t = remplacer(defauts, "\"police_depart\": 2", "\"police_depart\": 6");
        t = remplacer(t, "\"police_max\": 6", "\"police_max\": 3");
        lireConfig(t, "police", c);
        cas("poursuite.police_depart > police_max : ramené à police_max", c.poursuite.policeDepart == 3 && c.poursuite.policeMax == 3);
    }
    // RG-01 / RG-16 : le fichier du dépôt reprend le code à l'identique
    const char* chemin = argc > 1 ? argv[1] : "configgame.json";   // sans argument : ./configgame.json (racine du dépôt, CI)
    if (std::ifstream(chemin).good()) {
        std::ifstream f(chemin); std::stringstream ss; ss << f.rdbuf();
        ConfigJeu c; const BilanConfig b = lireConfig(ss.str(), chemin, c);
        for (const std::string& m : b.messages) std::printf("  %s\n", m.c_str());
        cas("configgame.json du dépôt : toutes les clés lues, 0 erreur", b.jsonValide && b.lues == N && b.erreurs == 0);
        cas("configgame.json du dépôt = valeurs du code (sans modification, rien ne change)", configJson(c) == defauts);
        cas("configgame.json du dépôt à jour (relancer test_configgame --ecrire . sinon)", ss.str() == defauts);
    } else std::printf("configuration : %s absent : comparaison avec le fichier du dépôt sautée\n", chemin);
    std::printf("configuration : %d clés, %s\n", N, echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
