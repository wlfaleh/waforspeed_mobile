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
        t = remplacer(t, "\"zoom\": 64.0", "\"zoom\": 75.5");
        const BilanConfig b = lireConfig(t, "modifié", c);
        cas("checkpoint.depart = 90 et camera.poursuite.zoom = 75,5 pris", c.checkpoint.depart == 90 && c.camera.fov[0] == 75.5f && b.erreurs == 0);
        cas("les autres clés gardent la valeur du code", c.checkpoint.bonus == 30 && c.camera.fov[3] == 60.f && c.conduite.grip == 18);
    }
    // WFS-191 : noms des voitures (section « voitures ») : défaut, nom changé, noms invalides → défaut et message
    {
        ConfigJeu c; const BilanConfig b0 = lireConfig(defauts, "défauts", c);
        cas("WFS-191 : 20 voitures, noms inventés par défaut (golf = Norvik Lume, WFS-406)",
            b0.erreurs == 0 && !std::strcmp(c.voitures.nom[0].marque, "Norvik") && !std::strcmp(c.voitures.nom[0].modele, "Lume") &&
            !std::strcmp(VOITURES_IDS[12], "model3") && !std::strcmp(c.voitures.nom[12].marque, "Volta"));
        ConfigJeu d; std::string t = remplacer(defauts, "\"marque\": \"Volta\"", "\"marque\": \"Électra\"");
        t = remplacer(t, "\"modele\": \"E3\"", "\"modele\": \"Série 3 Très-Long Modèle\"");   // 24 caractères, accents
        const BilanConfig b1 = lireConfig(t, "nom changé", d);
        cas("WFS-191 : nom changé sans reconstruire (Électra Série 3 Très-Long Modèle, 24 car.)",
            b1.erreurs == 0 && !std::strcmp(d.voitures.nom[12].marque, "Électra") && !std::strcmp(d.voitures.nom[12].modele, "Série 3 Très-Long Modèle") &&
            !std::strcmp(d.voitures.nom[0].marque, "Norvik"));
        ConfigJeu e; t = remplacer(defauts, "\"marque\": \"Volta\"", "\"marque\": \"Une marque bien trop longue\"");   // 27 > 16
        t = remplacer(t, "\"modele\": \"E3\"", "\"modele\": 3");                                                  // mauvais type
        t = remplacer(t, "\"marque\": \"Norvik\"", "\"marque\": \"\"");                                       // vide
        t = remplacer(t, "\"modele\": \"Vantor GT\"", "\"modele\": \"R4 <script>\"");                   // caractères non permis
        const BilanConfig b2 = lireConfig(t, "noms invalides", e);
        int msgs = 0; for (const std::string& m : b2.messages) msgs += m.find("voitures.") == 0 && m.find("nom par défaut") != std::string::npos;
        cas("WFS-191 : nom trop long, vide, mauvais type, caractère interdit → nom par défaut + message",
            b2.jsonValide && b2.erreurs == 4 && msgs == 4 && !std::strcmp(e.voitures.nom[12].marque, "Volta") && !std::strcmp(e.voitures.nom[12].modele, "E3") &&
            !std::strcmp(e.voitures.nom[0].marque, "Norvik") && !std::strcmp(e.voitures.nom[1].modele, "Vantor GT"));
        for (const std::string& m : b2.messages) std::printf("  %s\n", m.c_str());
    }
    // WFS-419 (RG-01, RG-02) : interrupteurs de diagnostic tous éteints par défaut, un « 1 » lu, une valeur hors 0 / 1 refusée
    {
        ConfigJeu c; lireConfig(defauts, "défauts", c);
        bool zeros = true; for (int v : c.diagnostic.coupe) zeros = zeros && v == 0;
        cas("WFS-419 : 12 interrupteurs de diagnostic à 0 par défaut", zeros && defauts.find("\"sans_ombres\": 0") != std::string::npos);
        ConfigJeu d; const BilanConfig b = lireConfig(remplacer(defauts, "\"sans_ombres\": 0", "\"sans_ombres\": 1"), "diag", d);
        cas("WFS-419 : diagnostic.sans_ombres = 1 lu", b.erreurs == 0 && d.diagnostic.coupe[3] == 1);
        ConfigJeu e; const BilanConfig b2 = lireConfig(remplacer(defauts, "\"sans_ombres\": 0", "\"sans_ombres\": 2"), "diag hors bornes", e);
        cas("WFS-419 : diagnostic.sans_ombres = 2 refusé (reste 0)", b2.erreurs == 1 && e.diagnostic.coupe[3] == 0);
    }
    // WFS-406 (RG-06) : adresse de la politique de confidentialité : vide par défaut (ligne cachée), https prise, autre refusée
    {
        ConfigJeu c; lireConfig(defauts, "défauts", c);
        cas("WFS-406 : confidentialite_url vide par défaut, lue sans erreur", c.confidentialiteUrl[0] == 0 && defauts.find("\"confidentialite_url\": \"\"") != std::string::npos);
        ConfigJeu d; const BilanConfig b1 = lireConfig(remplacer(defauts, "\"confidentialite_url\": \"\"", "\"confidentialite_url\": \"https://exemple.org/confidentialite\""), "adresse", d);
        cas("WFS-406 : adresse https prise", b1.erreurs == 0 && !std::strcmp(d.confidentialiteUrl, "https://exemple.org/confidentialite"));
        ConfigJeu e; const BilanConfig b2 = lireConfig(remplacer(defauts, "\"confidentialite_url\": \"\"", "\"confidentialite_url\": \"http://exemple.org\""), "adresse http", e);
        cas("WFS-406 : adresse non https refusée → vide (ligne cachée) + message", b2.erreurs == 1 && e.confidentialiteUrl[0] == 0);
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
    // RG-01 / RG-16 : le fichier du dépôt est valide, toutes ses clés sont lues et dans leurs bornes. Il peut différer du code :
    // c'est le fichier de réglage de l'utilisateur (e23fa51) ; les différences sont listées, jamais un échec.
    const char* chemin = argc > 1 ? argv[1] : "configgame.json";   // sans argument : ./configgame.json (racine du dépôt, CI)
    if (std::ifstream(chemin).good()) {
        std::ifstream f(chemin); std::stringstream ss; ss << f.rdbuf();
        ConfigJeu c; const BilanConfig b = lireConfig(ss.str(), chemin, c);
        for (const std::string& m : b.messages) std::printf("  %s\n", m.c_str());
        cas("configgame.json du dépôt : toutes les clés lues, 0 erreur", b.jsonValide && b.lues == N && b.erreurs == 0);
        std::stringstream lu(configJson(c)), code(defauts); std::string a, d; int diff = 0;
        while (std::getline(lu, a) && std::getline(code, d))
            if (a != d) { diff++; std::printf("configuration : réglé dans le fichier : %s (code : %s)\n", a.c_str(), d.c_str()); }
        std::printf("configuration : %d valeur(s) du fichier différente(s) du code (information, pas un échec)\n", diff);
    } else std::printf("configuration : %s absent : comparaison avec le fichier du dépôt sautée\n", chemin);
    std::printf("configuration : %d clés, %s\n", N, echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
