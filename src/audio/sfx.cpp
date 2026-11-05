// WaForSpeed — bruitages (WFS-28) : voir sfx.h.
#include "sfx.h"
#include <algorithm>
#include <cmath>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/course_system.h"
#include "../game/player.h"
#include "sfx_synth.h"
#include "trafic_sons.h"   // WFS-121

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
        case SfxId::Sirene: return "sirene";   // WFS-111
        case SfxId::Petarade: return "petarade"; case SfxId::Turbo: return "turbo";   // WFS-120
        case SfxId::Passage: return "passage"; case SfxId::Klaxon: return "klaxon";   // WFS-121
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
    for (int i = 0; i < 4; i++) beep(sons_[(int)SfxId::Sirene], rate, i % 2 ? 960.f : 720.f, i * 0.25f, 0.25f);   // WFS-111 : sirène deux tons (1 s)
    // WFS-120 (RG-01 / RG-02) : un claquement d'échappement (grave, sec) ; souffle du turbo (aigu qui descend)
    noiseHit(sons_[(int)SfxId::Petarade], 0, rate, 0.05f, 380, 1.2f, 0.55f, 140);
    noiseHit(sons_[(int)SfxId::Turbo], 0, rate, 0.22f, 2800, 0.8f, 0.25f, 1200);
    // WFS-121 (RG-02 à RG-05) : passage d'une voiture (Doppler), klaxon du trafic, rumeur du trafic proche
    sons_[(int)SfxId::Passage] = sonPassage(rate); sons_[(int)SfxId::Klaxon] = sonKlaxon(rate);
    rumeur_ = boucleRumeur(rate); kRumeur_ = 1 - std::exp(-1 / (trafic::RUMEUR_TAU * rate));
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
        out[i] = std::clamp(out[i] + s * master * fade_, -1.f, 1.f);
    }
}

// ---------------------------------------------------------------- événements de la partie (thread du jeu)
void SfxEvenements::resync(const Course& c) {
    c_ = &c; count_ = c.lastCount; level_ = c.level; accidents_ = c.accidents; near_ = c.nearCount;
    bumps_ = c.bumps; scrapes_ = c.scrapes; state_ = (int)c.state;
}

static std::atomic<uint32_t> g_interface{0};   // WFS-96 : sons demandés par les écrans (un bit par son)
void sfxInterface(SfxId id) { g_interface.fetch_or(1u << (int)id); }
void SfxEvenements::update(double, const GameView& g, SfxMixer& m) {
    for (uint32_t d = g_interface.exchange(0); d; d &= d - 1) m.play((SfxId)__builtin_ctz(d));
    const Course* c = activeCourse();
    const bool enCourse = g.ecran == Ecran::Course && !g.sceneDediee;
    if (!c || !enCourse) {
        if (c) resync(*c);
        if (crisse_ || nitro_) m.setBoucles(0, 0);
        crisse_ = nitro_ = false;
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
    if (c->state == RaceState::Over && state_ != (int)RaceState::Over) m.play(SfxId::BipFin);
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
}

} // namespace wfs
