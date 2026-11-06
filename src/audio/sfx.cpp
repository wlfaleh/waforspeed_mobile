// WaForSpeed — bruitages (WFS-28) : voir sfx.h.
#include "sfx.h"
#include <algorithm>
#include <cmath>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/course5.h"   // WFS-131 : modeCourse() (lecture seule : jingles de la poursuite à la place de BipFin)
#include "../game/course_system.h"
#include "../game/player.h"
#include "sfx_synth.h"
#include "sfx_course.h"   // WFS-124
#include "trafic_sons.h"   // WFS-121
#include "route_sons.h"   // WFS-174
#include "../world/meteo.h"   // WFS-174 (RG-05) : pluieActive (lecture seule)
#include <cstdlib>
#include <cstring>
#include "../ui/ecrans/reglages.h"   // WFS-131 (RG-05) : « Son : Coupé » (lecture seule)

namespace wfs {

const char* sfxNom(SfxId id) {
    switch (id) {
        case SfxId::Accident: return "accident";
        case SfxId::Choc: return "choc";
        case SfxId::Frottement: return "frottement";
        case SfxId::ContactDuel: return "contact-duel";
        case SfxId::QuasiContact: return "quasi-contact";
        case SfxId::BipDecompte: return "bip-decompte";
        case SfxId::BipGo: return "bip-go";
        case SfxId::BipCheckpoint: return "bip-checkpoint";
        case SfxId::BipFin: return "bip-fin";
        case SfxId::BipDuel: return "bip-duel";
        case SfxId::Garage: return "garage"; case SfxId::Carte: return "carte";   // WFS-96
        case SfxId::Petarade: return "petarade"; case SfxId::Turbo: return "turbo";   // WFS-120
        case SfxId::Passage: return "passage"; case SfxId::Klaxon: return "klaxon";   // WFS-121
        case SfxId::ChronoTic: return "chrono-tic"; case SfxId::ChronoUrgent: return "chrono-urgent";   // WFS-124
        case SfxId::Combo2: return "combo-2"; case SfxId::Combo3: return "combo-3"; case SfxId::Combo4: return "combo-4";
        case SfxId::Combo5: return "combo-5"; case SfxId::Combo6: return "combo-6"; case SfxId::ComboPerdu: return "combo-perdu";
        case SfxId::Demarreur: return "demarreur";   // WFS-127
        case SfxId::PoursuiteArrete: return "poursuite-arrete"; case SfxId::PoursuiteEchappe: return "poursuite-echappe";   // WFS-131
        default: return "?";
    }
}

// ---------------------------------------------------------------- fabrication (valeurs de js/game/update.js, js/ui/screens.js, js/net/duel.js)
void SfxMixer::build(float rate) {
    using namespace sfx;
    for (auto& s : sons_) s.clear();
    auto& acc = sons_[(int)SfxId::Accident];          // playerAccident
    noiseHit(acc, 0, rate, 0.6f, 140, 0.7f, 1, 45); noiseHit(acc, 0, rate, 0.4f, 2600, 1.4f, 0.55f, 7000); noiseHit(acc, 0, rate, 0.25f, 900, 2, 0.4f, 300);
    noiseHit(sons_[(int)SfxId::Choc], 0, rate, 0.3f, 200, 0.8f, 1, 70);               // bump : volume 0,4 + impact (gain à la demande)
    noiseHit(sons_[(int)SfxId::Frottement], 0, rate, 0.22f, 3200, 3, 0.35f, 1600);    // frottement latéral (hitCar)
    auto& duel = sons_[(int)SfxId::ContactDuel];      // hitGhosts : 0,7 + impact pour la première couche
    noiseHit(duel, 0, rate, 0.5f, 160, 0.8f, 1, 50); noiseHit(duel, 0, rate, 0.3f, 2400, 1.5f, 0.5f, 6000);
    noiseHit(sons_[(int)SfxId::QuasiContact], 0, rate, 0.28f, 900, 1.2f, 0.5f, 2600); // quasi-contact
    beep(sons_[(int)SfxId::BipDecompte], rate, 440, 0, 0.15f);                         // décompte 3, 2, 1
    beep(sons_[(int)SfxId::BipGo], rate, 880, 0, 0.35f);                               // GO
    auto& cp = sons_[(int)SfxId::BipCheckpoint]; beep(cp, rate, 880, 0, 0.12f); beep(cp, rate, 1320, 0.12f, 0.18f);
    auto& fin = sons_[(int)SfxId::BipFin]; beep(fin, rate, 660, 0, 0.2f); beep(fin, rate, 440, 0.2f, 0.2f); beep(fin, rate, 330, 0.4f, 0.4f);
    auto& bd = sons_[(int)SfxId::BipDuel]; beep(bd, rate, 880, 0, 0.15f); beep(bd, rate, 1320, 0.15f, 0.15f); beep(bd, rate, 1760, 0.3f, 0.4f);
    noiseHit(sons_[(int)SfxId::Garage], 0, rate, 0.18f, 1400, 1.5f, 0.25f, 500); noiseHit(sons_[(int)SfxId::Carte], 0, rate, 0.12f, 1600, 1.4f, 0.2f, 500);   // WFS-96
    // WFS-120 (RG-01 / RG-02) : un claquement d'échappement (grave, sec) ; souffle du turbo (aigu qui descend)
    noiseHit(sons_[(int)SfxId::Petarade], 0, rate, 0.05f, 380, 1.2f, 0.55f, 140);
    noiseHit(sons_[(int)SfxId::Turbo], 0, rate, 0.22f, 2800, 0.8f, 0.25f, 1200);
    // WFS-121 (RG-02 à RG-05) : passage d'une voiture (Doppler), klaxon du trafic, rumeur du trafic proche
    sons_[(int)SfxId::Passage] = sonPassage(rate); sons_[(int)SfxId::Klaxon] = sonKlaxon(rate);
    // WFS-124 : tic du chrono sous 8 s (plus aigu sur 3, 2, 1), bip du combo (un ton de plus par combo), combo perdu
    beep(sons_[(int)SfxId::ChronoTic], rate, 1000, 0, 0.06f);
    beep(sons_[(int)SfxId::ChronoUrgent], rate, 1500, 0, 0.09f);
    for (int n = 2; n <= sfxcourse::COMBO_MAX_SON; n++) beep(sons_[(int)SfxId::Combo2 + n - 2], rate, sfxcourse::frequenceCombo(n), 0, 0.08f);
    auto& perdu = sons_[(int)SfxId::ComboPerdu]; beep(perdu, rate, 440, 0, 0.1f); beep(perdu, rate, 330, 0.1f, 0.1f);
    {   // WFS-127 (RG-03) : démarreur (0,45 s) : 6 tours de lanceur (grave, réguliers), puis le moteur qui prend
        auto& d = sons_[(int)SfxId::Demarreur];
        for (int i = 0; i < 6; i++) noiseHit(d, (size_t)(i * 0.065f * rate), rate, 0.06f, 190, 1.4f, 0.45f, 120);
        noiseHit(d, (size_t)(0.38f * rate), rate, 0.07f, 320, 1.0f, 0.7f, 140);
    }
    rumeur_ = boucleRumeur(rate); kRumeur_ = 1 - std::exp(-1 / (trafic::RUMEUR_TAU * rate));
    // WFS-131 : sirène (boucles wail et yelp, RG-01 / RG-02), jingles de fin de poursuite (RG-06), lissages (RG-02, RG-03, RG-05)
    boucleWail_ = boucleSirene(rate, sirene::CYCLE_WAIL); boucleYelp_ = boucleSirene(rate, sirene::CYCLE_YELP);
    posWail_ = posYelp_ = 0;
    sons_[(int)SfxId::PoursuiteArrete] = jingleArrete(rate); sons_[(int)SfxId::PoursuiteEchappe] = jingleEchappe(rate);
    kSirene_ = 1 - std::exp(-1 / (sirene::LISSAGE_S / 3 * rate));
    pasFinSirene_ = pasFinSirene(rate); pasYelp_ = 1 / (sirene::FONDU_MODE_S * rate);
    {   // WFS-174 : sons de la route (boucles courtes, durées différentes pour ne pas battre ensemble ; crête ramenée à 1)
        auto normaliser = [](std::vector<float>& b) {
            float pk = 0; for (float x : b) pk = std::max(pk, std::abs(x));
            if (pk > 1e-6f) for (float& x : b) x /= pk;
        };
        // RG-01 : vent = bruit large ; deux boucles (filtre fermé ≈ 400 Hz, ouvert ≈ 2 500 Hz) mélangées selon la vitesse
        // (même générateur que le vent de la campagne, musique.cpp : bruit + passe-bande)
        ventGrave_ = boucle(rate, 0.62f, route::VENT_GRAVE_HZ, 0.5f, 1); normaliser(ventGrave_);
        ventAigu_ = boucle(rate, 0.57f, route::VENT_AIGU_HZ, 0.5f, 1, route::VENT_AIGU_HZ * 0.5f, 0.6f); normaliser(ventAigu_);
        roulementB_ = boucle(rate, 0.41f, 140, 0.8f, 1, 90, 0.9f); normaliser(roulementB_);   // RG-02 : 80-250 Hz
        impulsionBord_.clear(); noiseHit(impulsionBord_, 0, rate, 0.012f, 240, 1.2f, 1, 90); normaliser(impulsionBord_);   // RG-03 : un choc de la bande
        herbeB_ = boucle(rate, 0.53f, 2000, 0.6f, 1, 3500, 0.8f);   // RG-04 : 1-4 kHz, crépitements (grains aléatoires, jamais au raccord)
        {
            Bruit br; br.s = 0xBADC0DEu;
            std::vector<float> env(herbeB_.size(), 0.55f);
            const size_t g = (size_t)(0.006f * rate);
            for (size_t i = g; i + 2 * g < env.size(); i += (size_t)((0.004f + 0.008f * (br.next() * 0.5f + 0.5f)) * rate))
                for (size_t k = 0; k < g; k++) env[i + k] += 0.45f * (1 - (float)k / (float)g);
            for (size_t i = 0; i < herbeB_.size(); i++) herbeB_[i] *= env[i];
            normaliser(herbeB_);
        }
        eauB_ = boucle(rate, 0.37f, 4500, 0.7f, 1); normaliser(eauB_);   // RG-05 : 3-6 kHz
        kRoute_ = route::lissage(rate); rate_ = rate;
        posVentG_ = posVentA_ = posRoulement_ = posHerbe_ = posEau_ = 0; posImpulsion_ = impulsionBord_.size();
    }
    // sons continus (nouveaux, absents du web) : crissement de pneus (bande aiguë étroite) et souffle de nitro (large, grave)
    crissement_ = boucle(rate, 1.0f, 2100, 7, 0.9f, 3400, 9);
    nitro_ = boucle(rate, 1.2f, 650, 0.7f, 0.6f);
    k_ = 1 - std::exp(-1 / (0.04f * rate));       // lissage des boucles (40 ms) : ni clic à l'entrée ni à la sortie
    kFade_ = 1 - std::exp(-1 / (0.03f * rate));   // coupure (pause, muet) en 30 ms
    pret_.store(true, std::memory_order_release);
}

void SfxMixer::play(SfxId id, float gain) {
    WFS_LOG("SFX %s", sfxNom(id));   // RG-11 : journal (banc d'essai et logcat)
    const uint32_t t = tete_.load(std::memory_order_relaxed);
    if (t - queue_.load(std::memory_order_acquire) >= (uint32_t)FILE) return;   // file pleine (pas de carte son) : ignoré
    file_[t % FILE] = {(uint8_t)id, gain};
    tete_.store(t + 1, std::memory_order_release);
}

void SfxMixer::setBoucles(float crissement, float nitro) { cibleCrissement_ = crissement; cibleNitro_ = nitro; }

// ---------------------------------------------------------------- thread audio : aucune allocation, aucun verrou
void SfxMixer::mix(float* out, int n, double now) {
    if (!pret_.load(std::memory_order_acquire)) return;
    // nouvelles demandes → voix libres (la plus ancienne est reprise si toutes sont prises)
    uint32_t q = queue_.load(std::memory_order_relaxed);
    const uint32_t t = tete_.load(std::memory_order_acquire);
    for (; q != t; q++) {
        const Demande d = file_[q % FILE];
        int libre = 0; size_t plusLoin = 0;
        for (int i = 0; i < VOIX; i++) {
            if (!voix_[i].buf) { libre = i; break; }
            if (voix_[i].pos > plusLoin) { plusLoin = voix_[i].pos; libre = i; }
        }
        voix_[libre] = {&sons_[d.id], 0, d.gain};
    }
    queue_.store(q, std::memory_order_release);
    const bool actif = !muted_ && now - lastUpdate.load() < 0.25;   // jeu en pause (plus d'update) ou muet : silence
    const float cC = actif ? cibleCrissement_.load() : 0.f, cN = actif ? cibleNitro_.load() : 0.f, cF = actif ? 1.f : 0.f;
    const float cR = actif ? cibleRumeur_.load() : 0.f;   // WFS-121
    const float cS = actif ? cibleSirene_.load() : 0.f, cY = yelpSirene_.load() ? 1.f : 0.f, cD = dopplerSirene_.load();   // WFS-131
    const bool finS = finSirene_.load();
    // WFS-174 : sons de la route (silence en pause ou muet, comme les autres boucles)
    const float cV = actif ? cVent_.load() : 0.f, cO = cOuverture_.load(), cRo = actif ? cRoulement_.load() : 0.f;
    const float cB = actif ? cBord_.load() : 0.f, cH = actif ? cHerbe_.load() : 0.f, cE = actif ? cEau_.load() : 0.f, cCa = cCadence_.load();
    for (int i = 0; i < n; i++) {
        fade_ += (cF - fade_) * kFade_;
        gCrissement_ += (cC - gCrissement_) * k_; gNitro_ += (cN - gNitro_) * k_;
        float s = 0;
        for (auto& v : voix_) {
            if (!v.buf) continue;
            s += (*v.buf)[v.pos] * v.gain;
            if (++v.pos >= v.buf->size()) v.buf = nullptr;
        }
        if (gCrissement_ > 1e-4f) s += crissement_[posCrissement_] * gCrissement_;
        if (gNitro_ > 1e-4f) s += nitro_[posNitro_] * gNitro_;
        if (++posCrissement_ >= crissement_.size()) posCrissement_ = 0;
        if (++posNitro_ >= nitro_.size()) posNitro_ = 0;
        gRumeur_ += (cR - gRumeur_) * kRumeur_;   // WFS-121 : montée / descente en 0,5 s
        if (gRumeur_ > 1e-4f) s += rumeur_[posRumeur_] * gRumeur_;
        if (++posRumeur_ >= rumeur_.size()) posRumeur_ = 0;
        // WFS-131 (RG-01 à RG-05, RG-08) : deux boucles précalculées, lues au pas du Doppler, fondu linéaire wail ↔ yelp
        gSirene_ = suivreGainSirene(gSirene_, cS, finS, kSirene_, pasFinSirene_);
        mixYelp_ += std::clamp(cY - mixYelp_, -pasYelp_, pasYelp_);
        pasSirene_ += (cD - pasSirene_) * k_;
        if (gSirene_ > 1e-4f) {
            s += gSirene_ * ((1 - mixYelp_) * lireBoucle(boucleWail_, posWail_) + mixYelp_ * lireBoucle(boucleYelp_, posYelp_));
            posWail_ = avancerBoucle(posWail_, pasSirene_, boucleWail_.size()); posYelp_ = avancerBoucle(posYelp_, pasSirene_, boucleYelp_.size());
        }
        {   // WFS-174 (RG-01 à RG-06) : sons de la route, cibles lissées (95 % en 0,15 s)
            gVent_ += (cV - gVent_) * kRoute_; gOuverture_ += (cO - gOuverture_) * kRoute_; gRoulement_ += (cRo - gRoulement_) * kRoute_;
            gBord_ += (cB - gBord_) * kRoute_; gHerbe_ += (cH - gHerbe_) * kRoute_; gEau_ += (cE - gEau_) * kRoute_; gCadence_ += (cCa - gCadence_) * kRoute_;
            if ((phaseBattement_ += route::BATTEMENT_HZ / rate_) >= 1) phaseBattement_ -= 1;
            if (gVent_ > 1e-4f) {
                const float battement = 1 + route::BATTEMENT * std::sin(6.28318530718f * phaseBattement_);
                s += gVent_ * battement * ((1 - gOuverture_) * ventGrave_[posVentG_] + gOuverture_ * ventAigu_[posVentA_]);
            }
            if (gRoulement_ > 1e-4f) s += gRoulement_ * roulementB_[posRoulement_];
            if (gHerbe_ > 1e-4f) s += gHerbe_ * herbeB_[posHerbe_];
            if (gEau_ > 1e-4f) s += gEau_ * eauB_[posEau_];
            if ((phaseBord_ += gCadence_ / rate_) >= 1) { phaseBord_ -= 1; posImpulsion_ = 0; }   // un choc à chaque tour de cadence
            if (posImpulsion_ < impulsionBord_.size()) { if (gBord_ > 1e-4f) s += gBord_ * impulsionBord_[posImpulsion_]; posImpulsion_++; }
            if (++posVentG_ >= ventGrave_.size()) posVentG_ = 0;
            if (++posVentA_ >= ventAigu_.size()) posVentA_ = 0;
            if (++posRoulement_ >= roulementB_.size()) posRoulement_ = 0;
            if (++posHerbe_ >= herbeB_.size()) posHerbe_ = 0;
            if (++posEau_ >= eauB_.size()) posEau_ = 0;
        }
        out[i] = std::clamp(out[i] + s * master * fade_, -1.f, 1.f);
    }
}

// ---------------------------------------------------------------- événements de la partie (thread du jeu)
void SfxEvenements::resync(const Course& c) {
    c_ = &c; count_ = c.lastCount; level_ = c.level; accidents_ = c.accidents; near_ = c.nearCount;
    bumps_ = c.bumps; scrapes_ = c.scrapes; state_ = (int)c.state;
    chrono_ = sfxcourse::chronoAffiche(c.timeLeft); combo_ = c.combo;   // WFS-124 (RG-07) : « Rejouer » sans tic parasite
}

static std::atomic<uint32_t> g_interface{0};   // WFS-96 : sons demandés par les écrans (un bit par son)
static_assert((int)SfxId::Count <= 32, "g_interface : un bit par son, 32 au plus (WFS-124)");
void sfxInterface(SfxId id) { g_interface.fetch_or(1u << (int)id); }
// WFS-131 (RG-07) : police la plus proche donnée par la poursuite. Thread du jeu seulement : à chaque pas, Game appelle
// collide (la poursuite) puis update (SfxEvenements), qui lit puis efface l'appel
static bool g_sireneAppel = false;
static double g_sireneDistance = sirene::AUCUNE, g_sireneV = 0;
void sfxSirene(double distanceM, double vRapprochementMs) { g_sireneAppel = true; g_sireneDistance = distanceM; g_sireneV = vRapprochementMs; }

void SfxEvenements::update(double dt, const GameView& g, SfxMixer& m) {
    for (uint32_t d = g_interface.exchange(0); d; d &= d - 1) m.play((SfxId)__builtin_ctz(d));
    const Course* c = activeCourse();
    const bool enCourse = g.ecran == Ecran::Course && !g.sceneDediee;
    {   // WFS-131 : sirène (RG-02 à RG-05) : en jeu → selon la police ; course finie → extinction ; sinon silence (RG-05)
        const bool enJeu = enCourse && c && c->state == RaceState::Play && ecrans::reglages().son;
        const bool fin = enCourse && c && c->state == RaceState::Over;
        const CiblesSirene s = sirene_.update(dt, g_sireneAppel, g_sireneDistance, g_sireneV, enJeu, fin);
        g_sireneAppel = false;
        m.setSirene(s.gain, s.yelp, s.doppler, s.fin);
        if (s.journal) {   // RG-09
            if (s.distanceM < sirene::AUCUNE) WFS_LOG("sirene %s gain %.2f doppler %.2f police %.0f m", s.yelp ? "yelp" : "wail", s.gain, s.doppler, s.distanceM);
            else WFS_LOG("sirene %s gain %.2f doppler %.2f police aucune", s.yelp ? "yelp" : "wail", s.gain, s.doppler);
        }
    }
    if (!c || !enCourse) {
        if (c) resync(*c);
        if (crisse_ || nitro_) m.setBoucles(0, 0);
        crisse_ = nitro_ = false;
        if (route_) { m.setRoute({}); route_ = false; WFS_LOG("SFX route coupée"); }   // WFS-174 (RG-07) : menus, garage, pause
        return;
    }
    // nouvelle course (Course::reset) : compteurs remis à zéro → on repart sans son
    if (c != c_ || c->accidents < accidents_ || c->level < level_ || c->nearCount < near_ || c->bumps < bumps_ ||
        c->scrapes < scrapes_ || (c->state == RaceState::Count && state_ != (int)RaceState::Count)) {
        resync(*c);
        if (c->state == RaceState::Count) count_ = -1;   // le décompte qui commence est joué
    }
    // un son par changement (front montant), jamais à chaque image
    if (c->lastCount != count_) {   // décompte : 440 Hz à 3, 2, 1 ; 880 Hz au « GO » (lastCount = 0)
        if (c->lastCount >= 0) m.play(c->lastCount > 0 ? SfxId::BipDecompte : SfxId::BipGo);
        count_ = c->lastCount;
    }
    if (c->level > level_) m.play(SfxId::BipCheckpoint);
    if (c->accidents > accidents_) m.play(SfxId::Accident);
    if (c->nearCount > near_) m.play(SfxId::QuasiContact);
    if (c->bumps > bumps_) m.play(SfxId::Choc, (float)(0.4 + c->lastImpact));
    if (c->scrapes > scrapes_) m.play(SfxId::Frottement);
    // WFS-131 (RG-06) : en Poursuite, le jingle « ARRÊTÉ » / « ÉCHAPPÉ » (demandé par la poursuite) remplace BipFin
    if (c->state == RaceState::Over && state_ != (int)RaceState::Over && modeCourse() != ModeCourse::Poursuite) m.play(SfxId::BipFin);
    {   // WFS-124 : tic du chrono (RG-01 / RG-02), bip du combo (RG-03), combo perdu sur un choc (RG-04)
        const int aff = sfxcourse::chronoAffiche(c->timeLeft);
        if (sfxcourse::chronoActif(c->state == RaceState::Play, c->duelFinish, c->timeLeft, true)) {
            const sfxcourse::Tic t = sfxcourse::ticChrono(chrono_, aff);
            if (t != sfxcourse::AUCUN) m.play(t == sfxcourse::URGENT ? SfxId::ChronoUrgent : SfxId::ChronoTic);
        }
        chrono_ = aff;
        if (c->state == RaceState::Play) {
            if (const int n = sfxcourse::sonCombo(combo_, c->combo)) m.play((SfxId)((int)SfxId::Combo2 + n - 2));
            if (sfxcourse::comboPerdu(combo_, c->combo, bumps_, c->bumps)) m.play(SfxId::ComboPerdu);
        }
        combo_ = c->combo;
    }
    level_ = c->level; accidents_ = c->accidents; near_ = c->nearCount; bumps_ = c->bumps; scrapes_ = c->scrapes; state_ = (int)c->state;

    // crissements : mêmes conditions que la fumée de js/world/scene.js (skid) ; souffle de nitro tant qu'elle brûle
    const Player& pl = *g.player;
    const double vms = std::abs(pl.speed) * M;
    const bool joue = c->playing();
    const bool crisse = joue && ((pl.braking && vms > 18) || std::abs(pl.spinYaw) > 0.3 || (std::abs(pl.latAcc) > Phys::grip * 0.92 && vms > 20));
    const bool nitro = joue && pl.nitroOn;
    if (crisse && !crisse_) WFS_LOG("SFX crissement");
    if (nitro && !nitro_) WFS_LOG("SFX nitro");
    if (crisse != crisse_ || nitro != nitro_) m.setBoucles(crisse ? 0.35f : 0.f, nitro ? 0.4f : 0.f);
    crisse_ = crisse; nitro_ = nitro;

    {   // WFS-174 : sons de la route (RG-01 à RG-07) ; en course seulement, toutes les courses ; « Son : Coupé » : setMuted / réglage
        route::Entree e;
        e.kmh = (float)std::abs(pl.kmh()); e.kmhMax = (float)(pl.perf.top * 260.0);
        e.playerX = (float)pl.playerX; e.edge = (float)(g.map ? g.map->edge : 1.6);
        e.nitro = pl.nitroOn;
        const char* mo = std::getenv("WFS_MOMENT");
        e.pluie = meteo::pluieActive(g.map ? g.map->id : nullptr, mo && std::strcmp(mo, "nuit") == 0);
        e.enCourse = joue && ecrans::reglages().son;
        const route::Cibles r = route::cibles(e);
        m.setRoute(r);
        const bool sonne = r.somme() > 0.005f;
        journalRoute_ += dt;
        if (sonne && (!route_ || journalRoute_ >= 1)) {   // RG-10 : une ligne par seconde au plus
            WFS_LOG("SFX route vent %.2f roulement %.2f bord %.2f herbe %.2f eau %.2f (%.0f km/h, x %.2f)", r.vent, r.roulement, r.bord, r.herbe, r.eau, e.kmh, e.playerX);
            journalRoute_ = 0;
        }
        if (!sonne && route_) WFS_LOG("SFX route coupée");
        route_ = sonne;
    }
}

} // namespace wfs
