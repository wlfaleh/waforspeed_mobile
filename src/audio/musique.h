// WaForSpeed — musique et ambiances sonores par carte (WFS-76, épic E6).
// Tout est synthétisé par le code, hors du thread audio (aucun fichier musical, aucune licence) :
//   - musique : boucle synthwave à 110 BPM, 4 mesures (La m – Fa – Do – Sol, ≈ 8,7 s) écrite « en anneau » (les fins de
//     notes retombent au début : boucle sans clic) ; variante « menu » (basse + nappe) et « course » (+ batterie), jouées
//     en même temps pour un fondu croisé de 0,8 s ; volume 0,25 du master ;
//   - ambiance de la carte (en course seulement) : boucle de 8 s de bruit filtré (ville : rumeur grave ; plage : vagues avec
//     une houle de 8 s ; campagne : vent ; circuit : foule lointaine) + événements rares tirés au hasard sur le thread audio
//     (klaxon, mouette, oiseaux) ; la nuit : moitié du volume, ni oiseaux ni mouettes ; volume 0,15 du master.
// Le thread du jeu choisit ce qui joue (cibles atomiques) ; le thread audio lit les tampons et mélange, sans allocation ni verrou.
#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace wfs {

enum class Variante : int { Silence = 0, Menu = 1, Course = 2 };

class Musique {
public:
    void build(float rate);                   // une fois par fréquence de sortie (thread du jeu)
    void cible(Variante v) { cible_ = (int)v; }
    void mix(float* out, int n, float master);   // thread audio
    float volume = 0.25f;                     // RG-02 : part du master
    const std::vector<float>& tampon(Variante v) const { return v == Variante::Course ? course_ : menu_; }
    float rate() const { return rate_; }
private:
    std::vector<float> menu_, course_;
    float rate_ = 0, gMenu_ = 0, gCourse_ = 0, k_ = 0.001f;
    size_t pos_ = 0;
    std::atomic<int> cible_{0};
    std::atomic<bool> pret_{false};
};
Musique& musique();   // une seule musique pour toute l'application (gardée d'une partie à l'autre)

class Ambiance {
public:
    // carte : ville, plage, campagne, circuit (autre : ville) ; nuit : moitié du volume, sans oiseaux ni mouettes
    void build(float rate, const std::string& carte, bool nuit);
    void active(bool on) { actif_ = on; }
    void mix(float* out, int n, float master);   // thread audio
    float volume = 0.15f;                     // RG-03 : part du master
    // banc d'essai : premier événement rare avancé à `secondes` (pour qu'il s'entende dans les 10 s enregistrées)
    void premierEvenement(float secondes) { attente_ = (int64_t)(secondes * rate_); }
    const std::string& carte() const { return carte_; }
    bool nuit() const { return nuit_; }
private:
    std::vector<float> fond_, evenements_[3];
    int nbEvenements_ = 0;
    float rate_ = 0, g_ = 0, k_ = 0.001f, evMin_ = 12, evMax_ = 25, gainNuit_ = 1;
    size_t pos_ = 0;
    const std::vector<float>* evCourant_ = nullptr; size_t evPos_ = 0;
    int64_t attente_ = 0;
    uint32_t hasard_ = 0x2545F491u;
    std::string carte_; bool nuit_ = false;
    std::atomic<bool> actif_{false}, pret_{false};
};

// banc d'essai PC (RG-08) : musique-menu.wav, musique-course.wav, ambiance-<carte>.wav (10 s) + forme d'onde en PNG
void benchMusique(const std::string& out);

} // namespace wfs
