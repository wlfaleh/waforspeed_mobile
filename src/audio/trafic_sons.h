// WaForSpeed — sons du trafic (WFS-121, épic E6) : « vrouuum » d'une voiture qu'on dépasse (effet Doppler), klaxon d'un
// automobiliste qu'on frôle ou qu'on colle, rumeur des moteurs du trafic proche.
// Le jeu web n'a rien de tout cela (seul noiseHit est réutilisé, déjà porté dans sfx_synth.h) : valeurs fixées par le PO
// (regles-de-gestion/WFS-121.md), ajustables dans le bloc de constantes ci-dessous.
// Calcul seul, sans dépendance au jeu : le module son (audio_system.cpp) lit le trafic (activeTraffic(), lecture seule),
// remplit une VoitureSon par voiture et demande les sons à SfxMixer. Testé par tests/test_audio.cpp.
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace wfs {

namespace trafic {
// RG-02 : passage
constexpr double PASSAGE_M = 25;            // distance longitudinale (devant ou derrière)
constexpr double PASSAGE_LAT = 0.8;         // écart latéral maximal (demi-largeurs de route)
constexpr double PASSAGE_KMH = 40;          // vitesse relative minimale
constexpr double PASSAGE_KMH_MAX = 150;     // vitesse relative du volume maximal
constexpr float PASSAGE_VOL_MIN = 0.20f, PASSAGE_VOL_MAX = 0.45f;
constexpr double PASSAGE_INTERVALLE = 0.25; // au plus un passage toutes les 0,25 s
constexpr float PASSAGE_DUREE = 0.45f, PASSAGE_F0 = 900, PASSAGE_F1 = 350, PASSAGE_Q = 0.9f;
// RG-03 : klaxon
constexpr double KLAXON_DELAI = 0.15;       // après le quasi-contact
constexpr double COLLE_M = 6;               // écart pare-chocs à pare-chocs derrière la voiture
constexpr double COLLE_S = 1.5;             // pendant plus de 1,5 s
constexpr double COLLE_KMH = 30;            // à plus de 30 km/h
constexpr double KLAXON_INTERVALLE = 3;     // au plus un klaxon toutes les 3 s
constexpr float KLAXON_DUREE = 0.35f;
// RG-04 : rumeur
constexpr double RUMEUR_M = 40;             // voitures à moins de 40 m
constexpr float RUMEUR_PAR_VOITURE = 0.04f, RUMEUR_MAX = 0.25f;
constexpr float RUMEUR_FC = 250;            // passe-bas
constexpr float RUMEUR_TAU = 0.5f / 3;      // montée / descente en 0,5 s (95 %)
constexpr double RUMEUR_LOG = 0.05;         // RG-07 : une ligne par seconde si le gain change de plus de 0,05
} // namespace trafic

// une voiture du trafic vue du joueur, même indice que Traffic::cars (le signe de dz est suivi d'un pas à l'autre)
struct VoitureSon {
    double dzM = 0;          // position de la voiture − position du joueur (m) : > 0 devant
    double lat = 0;          // |écart latéral| (demi-largeurs de route)
    double vRelKmh = 0;      // |vitesse du joueur − vitesse de la voiture| (km/h)
    double longueurM = 4.5;  // longueur de la voiture (m)
    bool accidentee = false;
};

struct SonsDuPas {
    int passages = 0; float gainPassage = 0;   // 0 ou 1 passage par pas (intervalle de 0,25 s)
    bool klaxon = false;
    float rumeur = 0;                          // volume visé de la boucle (0..0,25)
    bool journalRumeur = false;                // RG-07 : écrire « rumeur <gain> »
};

class TraficSons {
public:
    // actif : écran Course, course en jeu (RG-06) et trafic présent (RG-01) ; sinon : tout est remis à zéro, rien ne sonne.
    // quasiContacts : compteur des quasi-contacts du trafic (TrafficStats::nearMisses, jamais la police)
    SonsDuPas update(double dt, bool actif, const std::vector<VoitureSon>& voitures, double kmhJoueur, double longueurJoueurM,
                     int quasiContacts);
    void reset();
private:
    std::vector<double> prevDz_;
    double depuisPassage_ = 1e9, depuisKlaxon_ = 1e9, colle_ = 0, attente_ = -1, horlogeLog_ = 0;
    int colleIdx_ = -1, quasi_ = -1;
    float rumeurLog_ = 0;
    uint32_t hasard_ = 0x6A09E667u;   // tirage reproductible « une fois sur deux »
};

// sons fabriqués une fois dans SfxMixer::build (RG-05)
std::vector<float> sonPassage(float rate);       // souffle passe-bande 900 → 350 Hz en 0,45 s
std::vector<float> sonKlaxon(float rate);        // klaxon de musique.cpp (sfx::klaxon) sans le filtre de distance
std::vector<float> boucleRumeur(float rate);     // bruit grave passe-bas 250 Hz, en boucle sans clic

} // namespace wfs
