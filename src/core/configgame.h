// WaForSpeed — configuration du jeu par le fichier configgame.json (WFS-128, épic E0).
// Régler le jeu sans toucher au code : scores, chrono et checkpoints, nitro, course à 5, poursuite police, caméras,
// trafic, conduite, qualité. Les valeurs par défaut sont ici (ce sont celles du jeu d'aujourd'hui) ; le fichier
// configgame.json (racine du dépôt, copié dans l'APK) les reprend à l'identique : sans modification, rien ne change.
//   - Android : configgame.json de l'APK, remplacé par celui posé dans Android/data/com.waforspeed.game/files/ (RG-03) ;
//   - PC (banc d'essai) : WFS_CONFIG, sinon ./configgame.json ; relu à chaque partie (Game::init).
// Une clé absente, d'un mauvais type ou hors bornes garde la valeur par défaut (avertissement au journal), un JSON cassé
// garde toutes les valeurs par défaut : jamais de plantage (RG-02 / RG-04). Lecteur JSON maison, nombres lus sans la
// langue du téléphone (RG-05, RG-16). Les valeurs sont lues une fois puis gardées en mémoire : config() ne lit jamais
// de fichier (RG-09). Une ligne par clé dans configgame.README.md (RG-08), fabriqué avec le fichier par
// test_configgame --ecrire.
// Les constantes du jeu qui en dépendent sont des références vers ces valeurs (ex. CHECK_DIST, Phys::grip) : le code
// qui les lit ne change pas.
#pragma once
#include <string>
#include <vector>

namespace wfs {

struct CamPoursuite { float dist, h, ahead, lookH; };
// WFS-191 : noms affichés des 20 voitures (section « voitures » : marque et modèle par id de src/game/cars.h, même ordre).
// Textes UTF-8 de 1 à 16 caractères (marque) et de 1 à 24 (modèle) ; les tampons gardent la place des accents (4 octets).
constexpr int VOITURES_NB = 20, MARQUE_MAX = 16, MODELE_MAX = 24, ADRESSE_MAX = 200;   // ADRESSE_MAX : WFS-306
inline constexpr const char* VOITURES_IDS[VOITURES_NB] = {"golf", "m4", "p911", "gallardo", "p208", "abarth", "tcross", "compass", "s3", "x5",
                                                         "giulia", "a45", "model3", "corsa", "gt86", "elantra", "v60", "rrsport", "mustang", "sf90"};
struct NomVoiture { char marque[MARQUE_MAX * 4 + 1]; char modele[MODELE_MAX * 4 + 1]; };
// WFS-487 (RG-04) : identifiants du catalogue de personnalisation (section « boutique ») ; noms, couleurs et règles dans
// src/game/personnalisation.* ; les 16 premières couleurs sont celles du garage actuel (PEINTURES de src/game/sauvegarde.h, même ordre)
constexpr int BOUTIQUE_COULEURS = 28, BOUTIQUE_FINITIONS = 8, BOUTIQUE_SKINS = 4, BOUTIQUE_DESSINS = 5, BOUTIQUE_CLASSES = 6, SKIN_VOITURES_MAX = 160;
constexpr int BOUTIQUE_MEDAILLES = 3;   // WFS-498 (RG-02) : bronze, argent, or
// WFS-510 (RG-01) : produits du market, dans l'ordre du catalogue ; ce sont aussi les identifiants des produits de la Play Console
// (WFS-511, 515) : ils ne changent jamais (seuls les étoiles, le % offert et les prix de secours se règlent dans configgame.json)
constexpr int MARKET_PRODUITS = 6, MARKET_ETOILES = 4;   // les 4 premiers : packs d'étoiles consommables
inline constexpr const char* MARKET_PRODUITS_IDS[MARKET_PRODUITS] = {"etoiles_bronze", "etoiles_argent", "etoiles_or", "etoiles_coffre", "pack_debutant", "sans_pub"};
inline constexpr const char* BOUTIQUE_MEDAILLES_IDS[BOUTIQUE_MEDAILLES] = {"bronze", "argent", "or"};
inline constexpr const char* BOUTIQUE_COULEURS_IDS[BOUTIQUE_COULEURS] = {
    "blanc", "noir", "gris", "rouge", "bleu", "orange", "jaune", "vert", "nacre", "bleunuit", "anglais", "bordeaux", "sable", "violet", "turquoise", "taxi",
    "bleuelectrique", "vertacide", "orangefluo", "rose", "jaunecitron", "violetneon", "rougevif", "cyan", "vertmenthe", "magenta", "corail", "bleuciel"};
inline constexpr const char* BOUTIQUE_FINITIONS_IDS[BOUTIQUE_FINITIONS] = {"brillant", "metallise", "nacree", "mat", "satine", "candy", "chrome", "cameleon"};
inline constexpr const char* BOUTIQUE_SKINS_IDS[BOUTIQUE_SKINS] = {"rallye", "drifttokyo", "neonnuit", "camouflage"};
inline constexpr const char* BOUTIQUE_DESSINS_IDS[BOUTIQUE_DESSINS] = {"essentiels", "numeros", "bandes", "flammes", "etoiles"};
constexpr int SCORE_POURSUITE_MAX = 1000;   // borne de poursuite.score_depart, reprise par la relecture des records (sauvegarde)   // = Poursuite de src/game/camera_vue.h

struct ConfigJeu {
    struct Score {
        int pointsParMetre = 5;        // score : 5 points par mètre parcouru (floor(dist × 0,006) × 5)
        int quasiContact = 250;        // quasi-contact : 250 × combo points
        double comboDuree = 3;         // s : le combo continue si le quasi-contact suivant arrive avant
        double chocPerdCombo = 0.08;   // impact au-delà duquel un petit choc fait perdre le combo
    } score;
    struct Checkpoint {
        double depart = 60, bonus = 30;   // s au départ, s ajoutées à chaque checkpoint (WFS-110 RG-08)
        double distanceM = 600;           // m entre deux checkpoints
        double distanceU = 100000;        // même distance en unités du jeu (calculée : distanceM / 0,006)
    } checkpoint;
    struct Nitro {
        double depart = 0.4;    // réserve au départ (0 à 1)
        double poussee = 1.6;   // accélération multipliée pendant la nitro
        double rechargeTemps = 0.015, rechargeDepassement = 0.10;   // WFS-189 (RG-04) : sans trafic, par seconde, par dépassement
    } nitro;
    struct Course5 {
        int ordinateurs = 4;                             // voitures pilotées par l'ordinateur
        double facteur[3] = {0.80, 0.95, 1.00};          // part de la vitesse de pointe visée : facile, normale, difficile (WFS-327 : 0,90 → 0,95 et 0,97 → 1,00 : les virages arcade ne compensent plus)
        double ecartMinM = 1.2;                          // m : jamais plus près de la voiture de devant
        double rattrapageDerriereM = 150, rattrapageDevantM = 250;   // WFS-133 (RG-04) : rattrapage, écart au joueur (m)
        double rattrapageBonus = 0.08, rattrapageMalus = 0.06, rattrapageMalusFacile = 0.10;
        double aspirationM = 25, aspirationBonus = 0.04;             // WFS-133 (RG-05) : aspiration
        double nitroBonus = 0.15, nitroDuree = 2.5, nitroFinM = 500; // WFS-133 (RG-06) : nitro des ordinateurs
    } course5;
    struct Poursuite {
        int scoreDepart = 1000; double kmArrivee = 10;   // score de départ : valeur de l'utilisateur (configgame.json, e23fa51)
        int chocTrafic = 5, chocGlissiere = 3, chocPolice = 10;
        double colleM = 12, colleS = 1;                  // police à moins de colleM pendant colleS : −1 par seconde
        int policeDepart = 2, policeMax = 6; double renfortKm = 2;
        double densite = 2.5;                            // trafic multiplié pendant la poursuite
    } poursuite;
    struct Camera {
        CamPoursuite loin{9.f, 6.3f, 10.f, -4.5f}, proche{6.f, 2.67f, 7.f, -2.1f};   // m : recul, hauteur, regard devant, regard en hauteur
        float fov[4] = {64.f, 72.f, 62.f, 60.f};          // degrés (zoom) : poursuite, capot, cockpit, proche (ordre de VueCamera)
        float elargVitesse = 7.f, elargNitro = 6.f;       // degrés ajoutés à pleine vitesse, en nitro
        float suiviOeil = 14.f, suiviRegard = 16.f;       // lissage du suivi (/s)
    } camera;
    struct Trafic {
        double densite = 1.0;   // nombre de voitures de chaque carte multiplié par ce facteur
    } trafic;
    struct Conduite {   // = Phys de src/game/player.h
        double brake = 12.5, roll = 0.35, drag = 0.00042, engineBrake = 0.9, grip = 18, maxSteer = 0.6, steerV = 14,
               steerIn = 4, steerOut = 6, align = 3, maxHeading = 0.5, offRoad = 7, vLibre = 10, vRecul = 3;
    } conduite;
    struct Voitures {   // WFS-191 : noms inventés par défaut (aucune marque ni aucun modèle réel affiché)
        NomVoiture nom[VOITURES_NB] = {
            {"Norvik", "Lume"}, {"Draven", "Vantor GT"}, {"Pfeiler", "9F S"}, {"Toro", "Ventura V10 Spyder"},
            {"Lionne", "L2"}, {"Scorpio", "Cinquina"}, {"Vollmer", "Trek-X"}, {"Ranger", "Bussola"}, {"Aurel", "R3 berline"},
            {"Bavaro", "XR5"}, {"Alvar", "Selene TS"}, {"Merkur", "A-Sport 45"}, {"Volta", "E3"}, {"Orion", "Corsaire"},
            {"Tamura", "Hachi"}, {"Hanseo", "Elanta R"}, {"Valka", "V6 Nordstern"}, {"Highland", "Sport"}, {"Stallion", "GT5"},
            {"Cavallo", "SF-X"}};
    } voitures;
    struct Qualite {
        float plancherMoyenne = 0.9f;   // échelle de rendu la plus basse en qualité Moyenne (WFS-104)
        float refletRouteMouillee = 1.f;   // WFS-142 (RG-04) : force du reflet de la scène sur la route mouillée (0 à 1,5)
    } qualite;
    struct Diagnostic {   // WFS-419 (RG-02) : interrupteurs de diagnostic de performance (0 éteint, 1 coupe l'effet) ; ordre de src/render/diagnostic_perf.h
        int coupe[12] = {};   // tous à 0 par défaut : jeu identique
    } diagnostic;
    struct Paquets {   // WFS-306 : liste des paquets de cartes (cartes.json de la Release « paquets », WFS-302) ; adresse https seulement
        char adresseListe[ADRESSE_MAX * 4 + 1] = "https://github.com/wlfaleh/waforspeed_mobile/releases/download/paquets/cartes.json";
    } paquets;
    // WFS-487 (RG-03, RG-04 ; prix de l'utilisateur, E18 et WFS-498 RG-04 à RG-06) : catalogue de personnalisation. Prix de BASE en étoiles,
    // multipliés par le coefficient de la classe de la voiture (packs de dessins : sans coefficient) ; gratuit = 1 : offert sur toutes les voitures ;
    // cadeau = 1 : seulement cadeau du jour ou succès, jamais en vente. Réglables sans recompiler (configgame.json).
    struct Boutique {
        struct Objet { int prix, gratuit, cadeau; };
        double coefficient[BOUTIQUE_CLASSES] = {1, 1, 1.5, 2, 3, 4};   // gratuites, classes 1 à 5
        // classe de chaque voiture (ordre de VOITURES_IDS) : 0 = gratuite (p208, abarth, gt86) ; x5, a45, elantra retirées (jamais vendues)
        int classe[VOITURES_NB] = {2, 3, 4, 4, 0, 0, 1, 1, 2, 3, 3, 3, 2, 1, 0, 2, 2, 2, 3, 5};
        Objet couleur[BOUTIQUE_COULEURS] = {
            {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0},   // les 8 gratuites (RG-03)
            {50, 0, 0}, {50, 0, 0}, {50, 0, 0}, {50, 0, 0}, {50, 0, 0}, {50, 0, 0}, {50, 0, 0}, {50, 0, 0},
            {50, 0, 0}, {50, 0, 0}, {50, 0, 0}, {50, 0, 0}, {50, 0, 0}, {50, 0, 0}, {50, 0, 0}, {50, 0, 0}, {50, 0, 0}, {50, 0, 0}, {50, 0, 0}, {50, 0, 0}};
        int couleurSecondaire = 50;   // couleur secondaire (toit, rétroviseurs : WFS-488, 489)
        Objet finition[BOUTIQUE_FINITIONS] = {{0, 1, 0}, {100, 0, 0}, {150, 0, 0}, {150, 0, 0}, {150, 0, 0}, {250, 0, 0}, {400, 0, 0}, {400, 0, 0}};
        Objet skin[BOUTIQUE_SKINS] = {{300, 0, 0}, {600, 0, 0}, {800, 0, 1}, {450, 0, 0}};   // Néon nuit : cadeau du jour / succès seulement
        char skinVoitures[BOUTIQUE_SKINS][SKIN_VOITURES_MAX * 4 + 1] = {
            "p208 abarth gt86 golf mustang giulia", "p208 abarth gt86 golf mustang giulia", "p208 abarth gt86 golf mustang giulia", "p208 abarth gt86 golf mustang giulia"};
        Objet dessins[BOUTIQUE_DESSINS] = {{0, 1, 0}, {150, 0, 0}, {200, 0, 0}, {250, 0, 0}, {300, 0, 0}};   // « essentiels » : les 10 autocollants gratuits
        // WFS-498 (RG-03, RG-04) : prix des voitures en étoiles (ordre de VOITURES_IDS) ; 0 = gratuite (p208, abarth, gt86) ;
        // x5, a45, elantra : retirées (VOITURES_RETIREES de src/game/cars.h), jamais vendues quel que soit leur prix. Total vendu : 24 350 ★
        int prixVoiture[VOITURES_NB] = {600, 2200, 3500, 3000, 0, 0, 300, 300, 1000, 0, 2400, 0, 1200, 250, 0, 0, 1000, 800, 1800, 6000};
        int gainCourse = 10;              // WFS-498 (RG-01) : ★ créditées à la fin d'une course terminée (en attendant l'étape « score »)
        double minutesParCourse = 2.96;   // WFS-498 (RG-11) : durée moyenne d'une course pour le banc d'équilibrage (120 h pour 24 350 ★)
        // WFS-498 (RG-02) : packs de médailles du market (pas une monnaie) : étoiles du pack et prix en euros ; bronze, argent, or
        struct Medaille { int etoiles; double euros; };
        Medaille medaille[BOUTIQUE_MEDAILLES] = {{100, 0.99}, {300, 2.49}, {600, 4.49}};
    } boutique;
    // WFS-510 (RG-01, RG-02, RG-06 ; valeurs de l'utilisateur, épic E21) : market. Étoiles de base et % offert de chaque pack d'étoiles
    // (total crédité = base + arrondi(base × offert / 100)), pack débutant (étoiles, bons de skin, course après laquelle il est proposé),
    // ×2 de fin de course par jour. Prix de secours en euros : PC et bancs seulement, jamais montrés sur Android (prix de Google, RC-E21-05).
    struct Market {
        struct Pack { int etoiles, offert; double prixSecours; };   // offert en % ; pack_debutant : étoiles seulement ; sans_pub : 0 étoile
        Pack pack[MARKET_PRODUITS] = {{100, 0, 0.99}, {300, 10, 2.49}, {600, 20, 4.49}, {3600, 25, 19.99}, {500, 0, 1.99}, {0, 0, 2.99}};
        int bonsSkinDebutant = 1;   // bons de skin du pack débutant
        int debutantApres = 3;      // pack débutant proposé à partir de la fin de cette course terminée
        int x2ParJour = 5;          // ×2 de fin de course par jour (date locale du téléphone)
    } market;
    // WFS-406 (RG-06) : adresse https de la politique de confidentialité montrée dans l'écran Crédits ; vide (défaut) : ligne cachée
    char confidentialiteUrl[ADRESSE_MAX * 4 + 1] = "";
};

// valeurs en cours (une seule instance, lue une fois par chargement ; jamais de fichier lu ici)
// initialisée à la compilation (constinit) : les constantes du jeu qui y renvoient (références) sont sûres dès le démarrage
constinit inline ConfigJeu g_configJeu{};
inline ConfigJeu& config() { return g_configJeu; }

// bilan d'un chargement (RG-10)
struct BilanConfig {
    std::string fichier;            // fichier utilisé (vide : aucun, valeurs du code)
    int lues = 0, defauts = 0;      // clés lues du fichier, clés restées à la valeur du code
    int erreurs = 0;                // clés refusées (type faux, hors bornes) + inconnues
    bool jsonValide = false;
    std::vector<std::string> messages;   // une ligne par problème (journal)
};

// lit un texte JSON dans c (pur : testé par tests/test_configgame.cpp) ; c doit déjà contenir les valeurs par défaut
BilanConfig lireConfig(const std::string& texte, const std::string& nom, ConfigJeu& c);
// charge config() depuis le premier fichier trouvé (Android : fichier du téléphone, puis l'APK ; PC : WFS_CONFIG,
// puis ./configgame.json) et écrit le bilan au journal ; sans fichier : valeurs du code
BilanConfig chargerConfig();
void configCheminTelephone(const std::string& dossier);   // Android : dossier de données (externalDataPath), avant chargerConfig
// fichier configgame.json et aide configgame.README.md fabriqués depuis les valeurs par défaut (test_configgame --ecrire)
std::string configJson(const ConfigJeu& c);
std::string configReadme();
int configNombreCles();

} // namespace wfs
