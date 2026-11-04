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
#include <map>
#include <mutex>
#include <string>
#include <thread>

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
    std::string quality = "high";              // quality : qualité graphique (low / medium / high : 3 niveaux, WFS-68)
    bool qualityChoisie = false;               // clé quality présente (qualityChosen du web) : pas de qualité auto (WFS-15)
    std::string cam = "poursuite";             // cam : vue (poursuite / capot / cockpit = 0, 1, 2 : VUES de WFS-66)
    bool muted = false;                        // muted : son coupé
    bool music = true;                         // music : musique (WFS-76, propre au jeu natif)
    std::string nick = "Pilote";               // nick : pseudo (14 caractères au plus)
    int km = 4;                                // km : distance de duel (2, 4, 6 ou 10)
    std::string steer = "glisser";             // steer : mode de braquage (glisser / fleches), stocké seulement
    bool gaucher = false, vibrations = true;   // WFS-75 : commandes en miroir, vibrations (propres au jeu natif)

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
    void boucleEcriture();
};

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
