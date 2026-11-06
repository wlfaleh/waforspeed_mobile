// WaForSpeed — bruitages (WFS-28) : chocs, frottements, quasi-contacts, bips de course, crissements, souffle de nitro.
// Port de js/audio/sfx.js (noiseHit, beep, setMuted) et des appels de js/game/update.js / js/ui/screens.js.
// Le jeu (thread principal) demande un son par une file sans verrou ; le thread audio (AAudio, WFS-10) le mélange
// au moteur. Sur le banc d'essai PC (pas de carte son) chaque son demandé écrit « SFX <nom> » au journal (RG-11).
#pragma once
#include <atomic>
#include <cstdint>
#include <vector>
#include "sirene_son.h"   // WFS-131
#include "route_sons.h"   // WFS-174

namespace wfs {

struct GameView;
struct Course;

enum class SfxId : uint8_t {
    Accident, Choc, Frottement, ContactDuel, QuasiContact,
    BipDecompte, BipGo, BipCheckpoint, BipFin, BipDuel, Garage, Carte,   // WFS-96 : sons d'interface
    Petarade, Turbo,   // WFS-120 : pétarade (un claquement), souffle du turbo
    Passage, Klaxon,   // WFS-121 : passage d'une voiture du trafic (Doppler), klaxon du trafic
    ChronoTic, ChronoUrgent, Combo2, Combo3, Combo4, Combo5, Combo6, ComboPerdu,   // WFS-124 : chrono sous 8 s, combos
    Demarreur,   // WFS-127 : démarreur de la voiture montrée au garage
    PoursuiteArrete, PoursuiteEchappe,   // WFS-131 : jingles de fin de poursuite (la sirène à bips de WFS-111 est retirée)
    Count
};
const char* sfxNom(SfxId id);
void sfxInterface(SfxId id);   // WFS-96 : son demandé par un écran (joué au pas suivant par SfxEvenements)
// WFS-131 (RG-07) : appelé par la poursuite à chaque pas (thread du jeu) : police mobile la plus proche (distance en m,
// ≥ 1e8 : aucune) et sa vitesse de rapprochement (m/s, > 0 si elle se rapproche) ; sans appel à un pas : pas de sirène
void sfxSirene(double distanceM, double vRapprochementMs);

class SfxMixer {
public:
    void build(float rate);                      // calcule tous les sons (démarrage, hors du thread audio)
    void play(SfxId id, float gain = 1);         // thread du jeu : un son, une fois
    void setBoucles(float crissement, float nitro);   // thread du jeu : volume visé des sons continus (0..1)
    void setRumeur(float g) { cibleRumeur_ = g; }   // WFS-121 (RG-04) : rumeur du trafic proche (0..0,25)
    // WFS-131 : sirène visée (gain 0..0,45, mode « yelp », facteur Doppler, extinction de fin de course)
    void setSirene(float gain, bool yelp, float doppler, bool fin) { cibleSirene_ = gain; yelpSirene_ = yelp; dopplerSirene_ = doppler; finSirene_ = fin; }
    // WFS-174 : sons de la route visés (vent, roulement, bande rugueuse, herbe, eau : route_sons.h)
    void setRoute(const route::Cibles& c) {
        cVent_ = c.vent; cOuverture_ = c.ouverture; cRoulement_ = c.roulement; cBord_ = c.bord; cHerbe_ = c.herbe; cEau_ = c.eau; cCadence_ = c.cadenceBord;
    }
    void setMuted(bool m) { muted_ = m; }        // future option « Son » (setMuted du jeu web)
    // thread audio : ajoute les bruitages dans out ; now : horloge (sans nouvelles du jeu → silence, pause)
    void mix(float* out, int n, double now);
    std::atomic<double> lastUpdate{0};
    float master = 0.5f;                         // sfxGain du jeu web
    const std::vector<float>& tampon(SfxId id) const { return sons_[(int)id]; }
private:
    std::vector<float> sons_[(int)SfxId::Count], crissement_, nitro_, rumeur_, boucleWail_, boucleYelp_;
    struct Demande { uint8_t id; float gain; };
    static constexpr int FILE = 64, VOIX = 16;
    Demande file_[FILE];
    std::atomic<uint32_t> tete_{0}, queue_{0};
    struct Voix { const std::vector<float>* buf = nullptr; size_t pos = 0; float gain = 0; };
    Voix voix_[VOIX];
    std::atomic<float> cibleCrissement_{0}, cibleNitro_{0}, cibleRumeur_{0};
    std::atomic<bool> muted_{false};
    float gCrissement_ = 0, gNitro_ = 0, fade_ = 0, k_ = 0.001f, kFade_ = 0.001f;
    float gRumeur_ = 0, kRumeur_ = 0.001f; size_t posRumeur_ = 0;   // WFS-121
    size_t posCrissement_ = 0, posNitro_ = 0;
    std::atomic<float> cibleSirene_{0}, dopplerSirene_{1};   // WFS-131 : sirène (thread du jeu → thread audio)
    std::atomic<bool> yelpSirene_{false}, finSirene_{false};
    float gSirene_ = 0, mixYelp_ = 0, pasSirene_ = 1, kSirene_ = 0.001f, pasFinSirene_ = 0.001f, pasYelp_ = 0.001f;
    double posWail_ = 0, posYelp_ = 0;
    // WFS-174 : boucles de la route (≈ 2,5 s à 48 kHz, ≤ 0,5 Mo : RG-09) et leur état (thread audio seulement)
    std::vector<float> ventGrave_, ventAigu_, roulementB_, impulsionBord_, herbeB_, eauB_;
    std::atomic<float> cVent_{0}, cOuverture_{0}, cRoulement_{0}, cBord_{0}, cHerbe_{0}, cEau_{0}, cCadence_{15};
    float gVent_ = 0, gOuverture_ = 0, gRoulement_ = 0, gBord_ = 0, gHerbe_ = 0, gEau_ = 0, gCadence_ = 15, kRoute_ = 0.001f, rate_ = 48000;
    float phaseBattement_ = 0, phaseBord_ = 0;
    size_t posVentG_ = 0, posVentA_ = 0, posRoulement_ = 0, posImpulsion_ = (size_t)-1, posHerbe_ = 0, posEau_ = 0;
    std::atomic<bool> pret_{false};
};

// suit la partie (lecture seule : activeCourse(), joueur) et demande un son à chaque changement d'état (RG-08)
class SfxEvenements {
public:
    void update(double dt, const GameView& g, SfxMixer& m);
private:
    void resync(const Course& c);
    SireneSon sirene_;   // WFS-131
    const Course* c_ = nullptr;
    int count_ = -1, level_ = 0, accidents_ = 0, near_ = 0, bumps_ = 0, scrapes_ = 0, state_ = 0;
    int chrono_ = 0, combo_ = 0;   // WFS-124 : valeur entière affichée du chrono, combo au pas précédent
    bool crisse_ = false, nitro_ = false;
    bool route_ = false; double journalRoute_ = 0;   // WFS-174 : sons de la route en cours, dernière ligne de journal
};

} // namespace wfs
