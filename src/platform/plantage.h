// WaForSpeed — rapport de plantage (WFS-126, épic E7) : Android (android_main.cpp, plantage_android.cpp) et banc PC.
// - Anneau des 200 dernières lignes du journal (RG-02) : chaque WFS_LOG / WFS_WARN / WFS_ERR y est copié par l'accroche
//   de src/core/log.h (pointeur wfsAnneau, nul tant que installer() n'est pas appelé : tests et outils inchangés).
// - Gestionnaire des signaux SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL (RG-01) : sigaction SA_SIGINFO | SA_ONSTACK, pile de
//   secours sigaltstack 64 Ko ; écrit <dossier>/plantage.txt avec des appels sûrs en signal seulement (open / write / close,
//   tampons statiques préparés d'avance, aucun malloc, aucun printf), puis rend la main au gestionnaire précédent
//   (Android garde son tombstone et son logcat).
// - Au lancement suivant (RG-04) : rapportPrecedent() renomme plantage.txt en plantage-precedent.txt (un seul gardé).
//   Sur le téléphone, JeuActivity le fait avant le jeu (boîte « Partager le rapport ») ; ici, c'est le banc PC.
// - Déclencheur de test (RG-05) : plantage natif volontaire 3 s après l'arrivée au menu.
#pragma once
#include <cstddef>
#include <cstdint>

namespace wfs::plantage {

constexpr int LIGNES = 200;        // RG-02 : lignes du journal gardées
constexpr int LONGUEUR = 256;      // RG-06 : une ligne coupée à 255 octets + zéro final (jamais au milieu d'un caractère)
constexpr int PILE_MAX = 32;       // RG-01 : adresses de pile au plus
constexpr size_t RAPPORT_MAX = 96 * 1024;   // tampon fixe du rapport (200 × 256 + en-tête + pile)

// ---------- anneau du journal (RG-02) ----------
// ajoute une ligne (niveau 'I', 'W' ou 'E'), sûre entre threads ; appelée par les macros de log.h une fois installée
void anneauAjouter(char niveau, const char* fmt, ...);
void anneauVider();                // tests
int anneauNombre();                // lignes gardées (≤ LIGNES)
const char* anneauLigne(int i);    // i = 0 : la plus ancienne gardée ; texte sans le niveau
char anneauNiveau(int i);
void brancherAnneau();             // pose le pointeur de log.h (installer() le fait déjà)

// ---------- mise en forme du rapport, sans allocation (RG-01, RG-06) ----------
struct Infos {                     // lues au démarrage, recopiées dans des tampons fixes
    char version[64] = "?";        // versionName (versionCode)
    char appareil[128] = "?";      // modèle du téléphone, version d'Android
};
struct Etat {                      // ce que le gestionnaire de signal connaît du plantage
    int signal = 0, code = 0;
    uintptr_t adresse = 0;         // adresse fautive (si_addr)
    int nPile = 0;
    char pile[PILE_MAX][96] = {};  // « libwaforspeed.so+0x1a2b3c », mis en forme dans le gestionnaire
};
// écrit le rapport complet dans `tampon` (au plus `cap` − 1 octets, zéro final) ; rend la longueur. Aucune allocation,
// aucun appel de bibliothèque hors memcpy / strlen : utilisable dans le gestionnaire de signal.
size_t formaterRapport(char* tampon, size_t cap, const Infos& infos, const Etat& e);
const char* nomSignal(int s);
// « nom+0xdecalage » sans allocation (RG-01) : nom du module sans son dossier
size_t formaterAdresse(char* t, size_t cap, const char* module, uintptr_t decalage);

// ---------- gestionnaire de signaux (RG-01) ----------
// dossier : dossier du fichier (interne à l'appli sur Android, sortie du banc sur PC) ; installe aussi l'anneau et la pile
// de secours du thread appelant (le thread du jeu)
void installer(const char* dossier, const char* version, const char* appareil);
bool installe();
const char* cheminRapport();       // <dossier>/plantage.txt

// ---------- lancement suivant (RG-04, RG-08) ----------
// si <dossier>/plantage.txt existe : journal « rapport précédent trouvé », renommé en plantage-precedent.txt ; rend vrai
bool rapportPrecedent(const char* dossier);

// ---------- déclencheur de test (RG-05) ----------
void armerDeclencheur(bool oui);
bool declencheurArme();
// à chaque image : 3 s après l'arrivée au menu, plantage natif volontaire (SIGSEGV)
void minuterieDeclencheur(double dt, bool auMenu);
[[noreturn]] void plantageVolontaire();

} // namespace wfs::plantage
