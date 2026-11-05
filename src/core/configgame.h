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

struct CamPoursuite { float dist, h, ahead, lookH; };   // = Poursuite de src/game/camera_vue.h

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
    } nitro;
    struct Course5 {
        int ordinateurs = 4;                             // voitures pilotées par l'ordinateur
        double facteur[3] = {0.80, 0.90, 0.97};          // part de la vitesse de pointe visée : facile, normale, difficile
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
        float fov[4] = {64.f, 72.f, 72.f, 60.f};          // degrés (zoom) : poursuite, capot, cockpit, proche (ordre de VueCamera)
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
    struct Qualite {
        float plancherMoyenne = 0.9f;   // échelle de rendu la plus basse en qualité Moyenne (WFS-104)
    } qualite;
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
