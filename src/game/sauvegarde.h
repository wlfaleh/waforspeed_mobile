// WaForSpeed — sauvegarde et progression (WFS-24, épic E3).
// Magasin unique des données gardées d'un lancement à l'autre : record, voiture, peinture par voiture, réglages.
// Mêmes clés et mêmes défauts que le jeu web (store() de js/core/utils.js, préfixe « waforspeed- » retiré sauf pour le
// record) : js/game/state.js (best), js/cars/catalog.js (myCar, myPaint), js/world/maps.js (setMap), js/ui/screens.js
// (setQuality, steerMode), js/world/scene.js (camMode), js/audio/sfx.js (muted), js/net/duel.js (nick, duelKm).
//
// Fichier texte « cle=valeur », une clé par ligne, première ligne « version=1 » :
//   version=1
//   waforspeed-best=48210
//   car=m4
//   paint-m4=rouge
//   ...
// Fichier absent, illisible ou corrompu (première ligne différente) : défauts. Valeur inconnue : défaut de cette clé seule.
//
// Utilisation (thread principal) :
//   sauvegarde().charger(chemin);              // au démarrage (Android : internalDataPath/sauvegarde.txt)
//   sauvegarde().car = "m4"; sauvegarde().enregistrer();   // écriture atomique sur un thread de travail
// Tant que charger() n'a pas été appelé, actif() est faux : le banc d'essai garde ses captures (aucun record).
#pragma once
#include <condition_variable>
#include <deque>
#include <map>
#include <set>
#include <vector>
#include <mutex>
#include <string>
#include <thread>
#include "trace_cartes.h"   // WFS-136 : version du tracé

namespace wfs {

// les 16 peintures du jeu web (PAINTS de js/cars/catalog.js, mêmes identifiants et hex que le garage, src/ui/garage.cpp)
struct PeintureDef { const char* id; const char* nom; unsigned hex; };
inline const PeintureDef PEINTURES[] = {
    {"blanc", "Blanc", 0xc9ccd2}, {"noir", "Noir", 0x0f1014}, {"gris", "Gris", 0x7b808a}, {"rouge", "Rouge", 0xb3122b},
    {"bleu", "Bleu", 0x1d3f8f}, {"orange", "Orange", 0xff6a00}, {"jaune", "Jaune", 0xc8940a}, {"vert", "Vert", 0x1f5a3c},
    {"nacre", "Blanc nacré", 0xe6e2d6}, {"bleunuit", "Bleu nuit", 0x141c3a}, {"anglais", "Vert anglais", 0x0f3324},
    {"bordeaux", "Bordeaux", 0x5a0f1e}, {"sable", "Sable", 0xb39a72}, {"violet", "Violet", 0x4b2380},
    {"turquoise", "Turquoise", 0x0f8f8c}, {"taxi", "Jaune taxi", 0xe8b400},
};
const PeintureDef* peintureDef(const std::string& id);   // nullptr si inconnue
// peinture d'origine d'une voiture (champ paint de CARS[] du jeu web)
const char* peintureOrigine(const std::string& carId);

class Sauvegarde {
public:
    static constexpr int VERSION = 1;
    // ---- données (RG-01) et défauts du jeu web (RG-02)
    long long best = 0;                        // waforspeed-best : meilleur score
    std::string car = "golf";                  // car : voiture choisie (id de CARS[])
    std::map<std::string, std::string> paint;  // paint-<voiture> : peinture choisie (absente = peinture d'origine)
    std::string map = "ville";                 // map : carte
    std::string tod = "nuit";                  // tod : moment (jour / nuit)
    std::string moment;                        // WFS-375 : midi (JOUR) ou nuit (NUIT) au chargement ; vide : déduit de tod
    std::string quality = "high";              // quality : qualité graphique (low / medium / high : 3 niveaux, WFS-68)
    bool qualityChoisie = false;               // clé quality présente (qualityChosen du web) : pas de qualité auto (WFS-15)
    std::string cam = "poursuite";             // cam : vue (poursuite / capot / cockpit = 0, 1, 2 : VUES de WFS-66)
    bool muted = false;                        // muted : son coupé
    bool music = true;                         // music : musique (WFS-76, propre au jeu natif)
    std::string nick = "Pilote";               // nick : pseudo (14 caractères au plus)
    int km = 4;                                // km : distance de duel (2, 4, 6 ou 10)
    std::string steer = "glisser";             // steer : mode de braquage (glisser / fleches), stocké seulement
    bool gaucher = false, vibrations = true;   // WFS-75 : commandes en miroir, vibrations (propres au jeu natif)
    bool grandsBoutons = false;                // WFS-108 : boutons : normaux / grands (× 1,25)
    int gyro = 0, gyroZone = 1;                // WFS-108 : gyroscope 0 coupé, 1 doux, 2 moyen, 3 fort ; zone morte 0..2
    bool aideDirection = true;                 // WFS-217 : aide à la direction (clé aide : activee / desactivee)
    int cgu = 0;                               // WFS-308 (RG-04) : version des CGU acceptée (0 : jamais acceptées)
    std::string mode = "checkpoint";           // WFS-110 : mode de course (checkpoint / course5 / poursuite : WFS-119)
    int km5 = 4;                               // WFS-110 : distance de la course à 5 (2, 4, 6 ou 10 km)
    std::string diff5 = "normale";             // WFS-133 (RG-02) : difficulté de la course à 5 (facile / normale / difficile)
    // WFS-125 (RG-02) : records par mode ; place 0 / distance −1 = vide (« — », ancienne sauvegarde : RG-03).
    // Checkpoint = best (clé waforspeed-best inchangée, RG-01) ; duel : aucun record.
    int rec5Place = 0; long long rec5Ms = 0;            // record-course5=place;temps_ms
    bool recPVictoire = false; int recPScore = 0;       // record-poursuite=victoire;score;distance_m
    long long recPDistM = -1;
    // WFS-136 (RG-07, A7.4) : version du tracé des records. Au premier lancement où une carte au moins est au tracé 2
    // (trace::CARTES_ACTIVES), records mis de côté une seule fois sous des clés *_v1 (jamais effacés) et trace=2 écrit ;
    // par carte, clé trace-<carte>=2 : à son passage au tracé 2, son fantôme est renommé fantome-<carte>_v1.bin et un
    // toast « Nouveaux tracés » est montré au départ suivant (une fois par carte activée)
    int trace = 1;
    bool nouveauxTraces = false;                        // toast à montrer (pas écrit dans le fichier)
    std::map<std::string, std::string> anciens;         // best_v1, record-course5_v1, record-poursuite_v1 (gardés tels quels)
    std::set<std::string> cartesV2;                     // cartes déjà passées au tracé 2 (clés trace-<carte>=2)
    std::vector<std::string> cartesAActiver;            // cartes actives pas encore passées (remplie par lire)
    void activerCartes();                               // fantômes renommés, cartes notées (appelée par charger)
    // WFS-122 (RG-01) : 3 derniers adversaires du duel, le plus récent en premier (clé adversaires=a|b|c)
    std::vector<std::string> adversaires;
    void noterAdversaires(const std::vector<std::string>& noms);   // en tête dans l'ordre reçu (classement), sans doublon, 3 au plus

    ~Sauvegarde();
    // remet les défauts, puis lit le fichier ; vrai si un fichier valide a été lu. Rend la sauvegarde active.
    bool charger(const std::string& chemin);
    // écrit l'état actuel (copié tout de suite) sur le thread de travail : fichier temporaire puis renommage (RG-05/06)
    void enregistrer();
    // attend la fin des écritures en cours (banc d'essai, fermeture)
    void attendre();
    bool actif() const { return actif_; }
    const std::string& chemin() const { return chemin_; }

    // ---- outils
    void defauts();                              // toutes les clés aux défauts du jeu web
    std::string texte() const;                   // contenu du fichier
    bool lire(const std::string& contenu);       // défauts puis lecture ; faux (défauts) si le format est invalide
    // endRace du jeu web : record écrit seulement s'il est battu (s > best) ; vrai si battu
    bool battreRecord(long long score);
    // WFS-125 (RG-02) : records Course à 5 (meilleure place, puis meilleur temps) et Poursuite (victoire > « ARRÊTÉ » ;
    // entre victoires le plus haut score ; entre « ARRÊTÉ » la plus grande distance) ; écrits seulement s'ils sont battus
    bool battreRecordCourse5(int place, long long tempsMs);
    bool battreRecordPoursuite(bool victoire, int score, long long distanceM);
    bool aRecordCourse5() const { return rec5Place > 0; }
    bool aRecordPoursuite() const { return recPDistM >= 0; }
    // WFS-125 (RG-05) : fichier binaire séparé (fantôme), écrit sur le même thread de travail (atomique) ; chemin à côté
    // du fichier de la sauvegarde
    void enregistrerFichier(const std::string& chemin, std::string contenu);
    std::string cheminVoisin(const std::string& nom) const;
    // WFS-125 : banc d'essai : sauvegarde de nouveau inactive (défauts), pour que les scènes suivantes gardent leurs captures
    void desactiver();
    // peinture de la voiture : choisie si valide, sinon celle d'origine (myPaint du jeu web)
    std::string peinture(const std::string& carId) const;
    // peinture choisie par le joueur pour cette voiture (nullptr : jamais choisie, la voiture garde sa couleur d'origine)
    const PeintureDef* peintureChoisie(const std::string& carId) const;
    int camIndex() const;                        // 0 poursuite, 1 capot, 2 cockpit (= VueCamera, reglages().camera)
    void setCamIndex(int i);
    int qualiteIndex() const;                    // 0 basse, 1 moyenne, 2 haute (= reglages().qualite)
    void setQualiteIndex(int i);

private:
    bool actif_ = false;
    std::string chemin_;
    // thread d'écriture : une seule écriture en attente (la plus récente), écrite par le thread de travail
    std::thread ecrivain_;
    std::mutex m_;
    std::condition_variable cv_;
    std::string aEcrire_, cheminAEcrire_;
    bool enAttente_ = false, enCours_ = false, quitter_ = false;
    std::deque<std::pair<std::string, std::string>> fichiers_;   // WFS-125 : fichiers en attente (chemin, contenu)
    void boucleEcriture();
};

// WFS-125 (RG-02) : comparaisons des records (vrai si le nouveau résultat bat l'ancien ; ancien vide = toujours battu)
namespace records {
bool course5Bat(int place, long long ms, int ancPlace, long long ancMs);
bool poursuiteBat(bool victoire, int score, long long distM, bool ancVictoire, int ancScore, long long ancDistM);
}

class Game;
class Renderer;
// banc d'essai (WFS_BANC=sauvegarde, lancé deux fois) : 1er lancement = voiture m4, peinture rouge, course jusqu'au record
// (capture fin-record), écriture ; 2e = « SAUVEGARDE relue : … » au journal (capture sauvegarde-relue), puis fichier effacé.
// Défini dans course_system.cpp.
void benchSauvegardeScene(Game& g, Renderer& r, const std::string& outDir, const std::string& chemin);

// sauvegarde du jeu, lisible et modifiable par tous les modules (thread principal)
Sauvegarde& sauvegarde();
// écriture atomique d'un fichier texte (temporaire « .tmp » puis renommage) ; vrai si réussi
bool ecrireAtomique(const std::string& chemin, const std::string& contenu);

} // namespace wfs
