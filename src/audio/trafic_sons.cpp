// WaForSpeed — sons du trafic (WFS-121) : voir trafic_sons.h.
#include "trafic_sons.h"
#include <algorithm>
#include <cmath>
#include "sfx_synth.h"
#include "../game/traffic.h"   // LANE_HALF (lecture seule)

namespace wfs {

using namespace trafic;

void TraficSons::reset() {
    prevDz_.clear(); colle_ = 0; colleIdx_ = -1; attente_ = -1; quasi_ = -1;
}

SonsDuPas TraficSons::update(double dt, bool actif, const std::vector<VoitureSon>& v, double kmhJoueur, double longueurJoueurM,
                             int quasiContacts) {
    SonsDuPas s;
    depuisPassage_ += dt; depuisKlaxon_ += dt;
    if (!actif) {   // RG-01 / RG-06 : menus, pause, garage, duel, course à 5, fin : rien, rumeur ramenée à 0
        reset();
    } else {
        if (prevDz_.size() != v.size()) prevDz_.resize(v.size(), NAN);   // voitures ajoutées : pas de passage au premier pas
        // RG-02 : passage = la distance longitudinale change de signe, tout près, assez vite
        float meilleur = 0;
        for (size_t i = 0; i < v.size(); i++) {
            const VoitureSon& c = v[i];
            const double p = prevDz_[i], d = c.dzM;
            prevDz_[i] = d;
            if (c.accidentee || std::isnan(p) || std::abs(p) >= PASSAGE_M || std::abs(d) >= PASSAGE_M || c.lat >= PASSAGE_LAT) continue;
            if (!((p > 0 && d <= 0) || (p < 0 && d >= 0)) || c.vRelKmh < PASSAGE_KMH) continue;
            const double k = std::clamp((c.vRelKmh - PASSAGE_KMH) / (PASSAGE_KMH_MAX - PASSAGE_KMH), 0.0, 1.0);
            const float g = (float)((PASSAGE_VOL_MIN + (PASSAGE_VOL_MAX - PASSAGE_VOL_MIN) * k) * (1 - 0.5 * c.lat / PASSAGE_LAT));
            meilleur = std::max(meilleur, g);
        }
        if (meilleur > 0 && depuisPassage_ >= PASSAGE_INTERVALLE) { s.passages = 1; s.gainPassage = meilleur; depuisPassage_ = 0; }

        // RG-03 (a) : quasi-contact → klaxon 0,15 s après, une fois sur deux (xorshift : même suite à chaque partie)
        if (quasi_ >= 0 && quasiContacts > quasi_) {
            hasard_ ^= hasard_ << 13; hasard_ ^= hasard_ >> 17; hasard_ ^= hasard_ << 5;
            if (hasard_ & 1) attente_ = KLAXON_DELAI;
        }
        quasi_ = quasiContacts;
        bool klaxonne = false;
        if (attente_ >= 0 && (attente_ -= dt) < 0) klaxonne = true;
        // RG-03 (b) : le joueur colle une voiture : même file, moins de 6 m derrière elle, plus de 1,5 s, plus de 30 km/h
        int idx = -1; double ecartMin = 1e9;
        if (kmhJoueur > COLLE_KMH)
            for (size_t i = 0; i < v.size(); i++) {
                const VoitureSon& c = v[i];
                const double ecart = c.dzM - (c.longueurM + longueurJoueurM) / 2;
                if (!c.accidentee && c.dzM > 0 && ecart < COLLE_M && c.lat < LANE_HALF && ecart < ecartMin) { ecartMin = ecart; idx = (int)i; }
            }
        if (idx < 0) { colle_ = 0; colleIdx_ = -1; }
        else {
            colle_ = idx == colleIdx_ ? colle_ + dt : dt; colleIdx_ = idx;
            if (colle_ > COLLE_S) { klaxonne = true; colle_ = 0; }
        }
        if (klaxonne && depuisKlaxon_ >= KLAXON_INTERVALLE) { s.klaxon = true; depuisKlaxon_ = 0; }

        // RG-04 : rumeur = 0,04 par voiture à moins de 40 m, plafonnée à 0,25
        int n = 0;
        for (const VoitureSon& c : v) n += std::abs(c.dzM) < RUMEUR_M;
        s.rumeur = std::min(RUMEUR_MAX, RUMEUR_PAR_VOITURE * (float)n);
    }
    // RG-07 : une ligne par seconde, seulement si le gain a changé de plus de 0,05 ; le retour à 0 est toujours écrit
    horlogeLog_ += dt;
    const bool retourZero = s.rumeur == 0 && rumeurLog_ > 0;
    if ((horlogeLog_ >= 1 && std::abs(s.rumeur - rumeurLog_) > RUMEUR_LOG) || retourZero) { s.journalRumeur = true; rumeurLog_ = s.rumeur; }
    if (horlogeLog_ >= 1) horlogeLog_ = 0;
    return s;
}

// ---------------------------------------------------------------- fabrication (thread du jeu, une fois)
std::vector<float> sonPassage(float rate) {
    using namespace sfx;
    const size_t n = (size_t)(PASSAGE_DUREE * rate);
    std::vector<float> b(n);
    Bruit br; br.s = 0x3C6EF372u; PasseBande f;
    for (size_t i = 0; i < n; i++) {
        const float x = (float)i / (float)n;
        if (i % 16 == 0) f.regler(PASSAGE_F0 * std::pow(PASSAGE_F1 / PASSAGE_F0, x), PASSAGE_Q, rate);   // la descente : effet Doppler
        // « vrouuum » : le souffle monte jusqu'au passage (30 % du son) puis s'éloigne
        const float env = x < 0.3f ? (x / 0.3f) * (x / 0.3f) : std::pow(0.02f, (x - 0.3f) / 0.7f);
        b[i] = f.run(br.next()) * env * fondu(i, n, rate);
    }
    return b;
}

std::vector<float> sonKlaxon(float rate) {
    std::vector<float> b;
    sfx::klaxon(b, rate, 0, KLAXON_DUREE, false);
    return b;
}

std::vector<float> boucleRumeur(float rate) {
    const size_t n = (size_t)(2.0f * rate), x = (size_t)(0.1f * rate);
    std::vector<float> tmp(n + x);
    sfx::Bruit br; br.s = 0x510E527Fu;
    const float a = 1 - std::exp(-6.28318530718f * RUMEUR_FC / rate);
    float y1 = 0, y2 = 0;
    for (size_t i = 0; i < 4096; i++) { y1 += a * (br.next() - y1); y2 += a * (y1 - y2); }   // filtres en régime
    for (auto& s : tmp) { y1 += a * (br.next() - y1); y2 += a * (y1 - y2); s = y2; }
    std::vector<float> out(tmp.begin(), tmp.begin() + (long)n);
    for (size_t i = 0; i < x; i++) { const float k = (float)i / (float)x; out[i] = out[i] * k + tmp[n + i] * (1 - k); }   // raccord sans clic
    float p = 0; for (float s : out) p = std::max(p, std::abs(s));
    if (p > 1e-6f) for (float& s : out) s *= 0.9f / p;
    return out;
}

} // namespace wfs
