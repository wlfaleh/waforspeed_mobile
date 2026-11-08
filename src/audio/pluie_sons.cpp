// WaForSpeed — son de la pluie (WFS-288) : voir pluie_sons.h.
#include "pluie_sons.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace wfs::pluie {
namespace {

constexpr float PI_F = 3.14159265358979f, PI2 = 6.28318530718f;
// essuie-glaces [à vérifier, aucune mesure publiée lisible] : frottement du balai mouillé (bruit, bande de 2,2 kHz), choc sourd du
// balai qui revient en butée (85 Hz amorti en 30 ms + claquement de 2 ms), bourdonnement du moteur à engrenages (110 et 220 Hz)
constexpr float FROT_HZ = 2200.f, CHOC_HZ = 85.f, CHOC_TAU = 0.03f, CLAC_TAU = 0.002f, BOURDON_HZ = 110.f;
constexpr float CHOC_HAUT = 0.4f;          // choc en haut de la course : plus léger qu'au retour en butée
constexpr float CHOC_GARDE = 0.15f;        // s : jamais deux chocs plus rapprochés (recalage du thread audio sur le modèle)
// résonances des impacts [à vérifier] : tôle du toit (450 Hz) et vitrage du pare-brise (1,5 kHz), dans la bande 200 Hz - 2 kHz de RG-04
constexpr float TOIT_HZ = 450.f, TOIT_Q = 2.5f, PB_HZ = 1500.f, PB_Q = 3.f, TAMBOUR_HAUT = 2000.f;
// averse : bruit large, maximum entre 1 et 8 kHz (RG-03 [à vérifier]) ; grains des gouttes isolées autour de 3 kHz, 2 à 10 ms
constexpr float AVERSE_BAS = 1000.f, AVERSE_HAUT = 8000.f, GOUTTE_HZ = 3000.f, GOUTTE_MIN = 0.002f, GOUTTE_MAX = 0.010f;
// bulles de Minnaert : f · r = √(3γP0 / ρ) / 2π = 3,29 m/s (air dans l'eau, 1 atm, 20 °C) ; pic de 13 à 16 kHz des petites gouttes
// (Pumphrey et Crum 1989 [à vérifier]) : rayon de 0,21 à 0,25 mm ; amortie en 1 ms, montée de 10 % de la fréquence
constexpr float BULLE_MIN = 13000.f, BULLE_MAX = 16000.f, BULLE_DUREE = 0.003f, BULLE_TAU = 0.001f, BULLE_CHIRP = 0.10f;

// Simpson sur [a, b], n pair
template <class F> double integrer(F f, double a, double b, int n = 2000) {
    const double h = (b - a) / n;
    double s = f(a) + f(b);
    for (int i = 1; i < n; i++) s += f(a + i * h) * (i % 2 ? 4 : 2);
    return s * h / 3;
}

} // namespace

// ---------------------------------------------------------------- RG-01
float lambdaMP(float R) { return 4.1f * std::pow(std::max(R, 1e-3f), -0.21f); }
float vitesseChute(float D) { return std::max(0.f, 9.65f - 10.3f * std::exp(-0.6f * D)); }

Reference reference(float R) {
    Reference r;
    const double L = lambdaMP(R);
    auto N = [&](double D) { return MP_N0 * std::exp(-L * D); };   // m⁻³·mm⁻¹
    auto Nv = [&](double D) { return N(D) * vitesseChute((float)D); };
    r.lambda = (float)L;
    r.concentration = (float)integrer(N, D_MIN, D_MAX);
    r.flux = (float)integrer(Nv, D_MIN, D_MAX);
    r.fluxGrosses = (float)integrer(Nv, D_GROSSE, D_MAX);
    r.partGrosses = r.flux > 0 ? r.fluxGrosses / r.flux : 0.f;
    r.fluxBulles = (float)integrer(Nv, D_BULLE_MIN, D_BULLE_MAX);
    // contrôle : eau tombée (mm/h) = 3,6·10⁶ × ∫ N · v · π/6 · D³ (D en m), toutes tailles
    r.tauxMmh = (float)(3.6e6 * integrer([&](double D) { const double d = D * 1e-3; return Nv(D) * PI_F / 6 * d * d * d; }, 0, D_MAX));
    return r;
}

float fluxPareBrise(const Reference& r, float kmh, float pente) {
    const float a = std::atan(pente), V = std::fabs(kmh) / 3.6f;
    return r.flux * std::cos(a) + V * std::sin(a) * r.concentration;
}

Surfaces surfaces(float L, float W, float H, float wb) {
    (void)L;
    Surfaces s;
    s.pente = PENTE_PARE_BRISE;
    s.toit = (0.60f * wb) * (0.70f * W);
    const float hauteurPb = 0.26f * H, longueurPb = hauteurPb / std::sin(std::atan(s.pente));
    s.pareBrise = longueurPb * 0.80f * W;
    return s;
}

// ---------------------------------------------------------------- filtres (RBJ)
SonsPluie::Biquad SonsPluie::passeBas(float fc, float q, float rate) {
    Biquad b; const float w = PI2 * std::min(fc, 0.45f * rate) / rate, c = std::cos(w), al = std::sin(w) / (2 * q), a0 = 1 + al;
    b.b0 = (1 - c) / 2 / a0; b.b1 = (1 - c) / a0; b.b2 = b.b0; b.a1 = -2 * c / a0; b.a2 = (1 - al) / a0; return b;
}
SonsPluie::Biquad SonsPluie::passeHaut(float fc, float q, float rate) {
    Biquad b; const float w = PI2 * std::min(fc, 0.45f * rate) / rate, c = std::cos(w), al = std::sin(w) / (2 * q), a0 = 1 + al;
    b.b0 = (1 + c) / 2 / a0; b.b1 = -(1 + c) / a0; b.b2 = b.b0; b.a1 = -2 * c / a0; b.a2 = (1 - al) / a0; return b;
}
SonsPluie::Biquad SonsPluie::passeBande(float fc, float q, float rate) {
    Biquad b; const float w = PI2 * std::min(fc, 0.45f * rate) / rate, c = std::cos(w), al = std::sin(w) / (2 * q), a0 = 1 + al;
    b.b0 = al / a0; b.b1 = 0; b.b2 = -al / a0; b.a1 = -2 * c / a0; b.a2 = (1 - al) / a0; return b;
}

uint32_t SonsPluie::hasard() { etat_ ^= etat_ << 13; etat_ ^= etat_ >> 17; etat_ ^= etat_ << 5; return etat_; }

// ---------------------------------------------------------------- build (thread du jeu)
void SonsPluie::build(float rate) {
    pret_.store(false, std::memory_order_release);
    rate_ = rate;
    ref_ = reference(PLUIE_MMH);
    // quantiles des tailles : tirage d'une goutte qui frappe une surface (poids N·v) ou balayée par le pare-brise (poids N)
    const double L = ref_.lambda;
    auto quantiles = [&](std::array<float, TABLE>& t, bool chute) {
        constexpr int M = 4000;
        std::vector<double> cum(M + 1, 0.0);
        const double h = (D_MAX - D_MIN) / M;
        for (int i = 1; i <= M; i++) {
            const double D = D_MIN + (i - 0.5) * h;
            cum[i] = cum[i - 1] + std::exp(-L * D) * (chute ? vitesseChute((float)D) : 1.0) * h;
        }
        for (int k = 0; k < TABLE; k++) {
            const double cible = (k + 0.5) / TABLE * cum[M];
            const int i = (int)(std::lower_bound(cum.begin(), cum.end(), cible) - cum.begin());
            t[(size_t)k] = (float)(D_MIN + std::clamp(i, 1, M) * h);
        }
    };
    quantiles(tailleChute_, true); quantiles(tailleConcentration_, false);
    for (int c = 0; c < 2; c++) { extG_[c] = extD_[c] = c == 0 ? passeHaut(AVERSE_BAS, 0.707f, rate) : passeBas(AVERSE_HAUT, 0.707f, rate); }
    for (int c = 0; c < 2; c++) intG_[c] = intD_[c] = passeBas(COUPURE_HABITACLE, 0.707f, rate);
    goutteBp_ = passeBande(GOUTTE_HZ, 1.2f, rate);
    toitG_ = toitD_ = passeBande(TOIT_HZ, TOIT_Q, rate); pbG_ = pbD_ = passeBande(PB_HZ, PB_Q, rate);
    tambG_ = tambD_ = passeBas(TAMBOUR_HAUT, 0.707f, rate);   // impacts entendus à travers la carrosserie : rien au-dessus de 2 kHz
    frot_ = passeBande(FROT_HZ, 0.9f, rate);
    // normalisations : RMS 1 du bruit blanc filtré (1 s), crête 1 de la réponse à une impulsion
    auto rms = [&](auto chaine) {
        uint32_t e = 0x1234567u; double s2 = 0; const int n = (int)rate;
        for (int i = 0; i < n; i++) { e ^= e << 13; e ^= e >> 17; e ^= e << 5; const float y = chaine((float)(e >> 8) * (2.f / 16777216.f) - 1); s2 += (double)y * y; }
        return (float)std::sqrt(s2 / n);
    };
    { Biquad a = extG_[0], b = extG_[1]; kExt_ = 1 / std::max(1e-6f, rms([&](float x) { return b.run(a.run(x)); })); }
    { Biquad a = intG_[0], b = intG_[1]; kInt_ = 1 / std::max(1e-6f, rms([&](float x) { return b.run(a.run(x)); })); }
    { Biquad a = goutteBp_; kGoutte_ = 1 / std::max(1e-6f, rms([&](float x) { return a.run(x); })); }
    { Biquad a = frot_; kFrot_ = 1 / std::max(1e-6f, rms([&](float x) { return a.run(x); })); }
    auto crete = [&](Biquad b) { float p = 0; for (int i = 0; i < (int)(0.05f * rate); i++) p = std::max(p, std::fabs(b.run(i == 0 ? 1.f : 0.f))); return p; };
    kToit_ = 1 / std::max(1e-6f, crete(toitG_)); kPb_ = 1 / std::max(1e-6f, crete(pbG_));
    pGoutte_ = ref_.fluxGrosses * A_ECOUTE / rate;
    pBulle_ = ref_.fluxBulles * A_FLAQUE / rate;
    gPresence_ = 0; gTunnel_ = 1; gVue_ = 0; gBalais_ = 0;
    for (Grain& g : gouttes_) g = Grain{};
    for (Grain& g : bulles_) g = Grain{};
    phaseA_ = phaseDebut_ = 0; depuisChoc_ = 1; chocT_ = 1e9f; chocAmp_ = 0; phBourdon_ = 0; marcheVue_ = false;
    nToit_ = nPb_ = nGouttes_ = nBulles_ = nChocs_ = 0;
    etat_ = 0x9E3779B9u; seqVu_ = seq_.load();
    essuie_ = cockpit::EtatEssuie{};
    pret_.store(true, std::memory_order_release);
}

// ---------------------------------------------------------------- pas (thread du jeu)
void SonsPluie::pas(float dt, double kmh, bool pluie, bool cockpit, bool balais, bool tunnel, const Surfaces& s) {
    essuie_.avancer(dt, kmh, pluie);   // RG-05 : même modèle et mêmes entrées que l'image (camera_vue.cpp)
    pluie_.store(pluie, std::memory_order_relaxed); cockpit_.store(cockpit, std::memory_order_relaxed);
    balais_.store(balais, std::memory_order_relaxed); tunnel_.store(tunnel, std::memory_order_relaxed);
    const float a = std::atan(s.pente), V = (float)std::fabs(kmh) / 3.6f;
    const float fChute = ref_.flux * std::cos(a), fTotal = fluxPareBrise(ref_, (float)kmh, s.pente);
    pToit_.store(std::min(0.9f, ref_.flux * s.toit / rate_), std::memory_order_relaxed);
    pPb_.store(std::min(0.9f, fTotal * s.pareBrise / rate_), std::memory_order_relaxed);
    partChute_.store(fTotal > 0 ? fChute / fTotal : 1.f, std::memory_order_relaxed);
    vnPb_.store(V * std::sin(a), std::memory_order_relaxed); cosPb_.store(std::cos(a), std::memory_order_relaxed);
    const float c = cockpit::cadenceEssuie(kmh, pluie);
    phase_.store(essuie_.phase, std::memory_order_relaxed);
    cadence_.store(essuie_.enMarche ? (c > 0 ? c : 45.f) / 60.f : 0.f, std::memory_order_relaxed);
    marche_.store(essuie_.enMarche, std::memory_order_relaxed);
    seq_.fetch_add(1, std::memory_order_release);
}

// ---------------------------------------------------------------- mix (thread audio)
void SonsPluie::mix(float* G, float* D, int n, float master) {
    if (!pret_.load(std::memory_order_acquire) || n <= 0) return;
    const bool actif = actif_.load(std::memory_order_relaxed), pluie = pluie_.load(std::memory_order_relaxed);
    const bool cockpit = cockpit_.load(std::memory_order_relaxed), tunnel = tunnel_.load(std::memory_order_relaxed);
    const bool balais = actif && cockpit && balais_.load(std::memory_order_relaxed);
    const float cPresence = actif && pluie ? 1.f : 0.f, cTunnel = tunnel ? 0.f : 1.f, cVue = cockpit ? 1.f : 0.f;
    // RG-08 : muette (hors pluie, hors écran Course, aucun choc en cours) : rien n'est touché, le son sort comme avant
    if (cPresence == 0 && gPresence_ == 0 && !balais && gBalais_ == 0 && chocT_ > 0.2f) {
        gVue_ = cVue; gTunnel_ = cTunnel;
        const uint32_t sq = seq_.load(std::memory_order_acquire);
        if (sq != seqVu_) { seqVu_ = sq; phaseA_ = phase_.load(std::memory_order_relaxed); marcheVue_ = marche_.load(std::memory_order_relaxed); }
        phaseDebut_ = phaseA_;
        return;
    }
    const float r = rate_;
    const float dIn = 1 / (FONDU_ENTREE * r), dOut = 1 / (FONDU_SORTIE * r);
    const float dTunIn = 1 / (FONDU_TUNNEL_ENTREE * r), dTunOut = 1 / (FONDU_TUNNEL_SORTIE * r), dVue = 1 / (FONDU_VUE * r);
    const float pToit = pToit_.load(std::memory_order_relaxed), pPb = pPb_.load(std::memory_order_relaxed);
    const float partChute = partChute_.load(std::memory_order_relaxed), vnPb = vnPb_.load(std::memory_order_relaxed);
    const float cosPb = cosPb_.load(std::memory_order_relaxed), v2 = vitesseChute(D_GROSSE);
    const float isolation = std::pow(10.f, ISOLATION_DB / 20.f);
    auto choc = [&](float amp) {   // départ d'un choc des balais (jamais deux à moins de CHOC_GARDE)
        if (depuisChoc_ < CHOC_GARDE) return;
        chocT_ = 0; chocAmp_ = amp; depuisChoc_ = 0; nChocs_++;
    };
    // RG-05 : recalage de la phase sur le modèle de l'image au début du bloc (le thread audio avance ensuite à la même cadence)
    const uint32_t sq = seq_.load(std::memory_order_acquire);
    float cad = cadence_.load(std::memory_order_relaxed);
    if (sq != seqVu_) {
        seqVu_ = sq;
        const bool marche = marche_.load(std::memory_order_relaxed);
        const float ph = phase_.load(std::memory_order_relaxed);
        if (marche) {
            if (ph < phaseA_ - 0.5f) { if (balais) choc(1.f); }                         // repassé par la butée (fin d'un cycle)
            else if (phaseA_ < 0.5f && ph >= 0.5f && ph - phaseA_ < 0.5f) { if (balais) choc(CHOC_HAUT); }   // haut de la course
            phaseA_ = ph;
        } else {
            if (marcheVue_ && phaseA_ > 0.5f && balais) choc(1.f);   // dernier cycle fini : retour en butée
            phaseA_ = 0;
        }
        marcheVue_ = marche;
    }
    phaseDebut_ = phaseA_;
    if (gPresence_ == 0 && gBalais_ == 0) gVue_ = cVue;   // départ dans la vue choisie (aucun fondu de vue à partir du silence)
    const bool marche = marcheVue_;
    const float kAverse = AVERSE_EXT * master, kTambour = TAMBOUR * master;
    for (int i = 0; i < n; i++) {
        // rampes (RG-02, RG-06, vue)
        gPresence_ = cPresence > gPresence_ ? std::min(cPresence, gPresence_ + dIn) : std::max(cPresence, gPresence_ - dOut);
        gTunnel_ = cTunnel > gTunnel_ ? std::min(cTunnel, gTunnel_ + dTunOut) : std::max(cTunnel, gTunnel_ - dTunIn);
        gVue_ = cVue > gVue_ ? std::min(cVue, gVue_ + dVue) : std::max(cVue, gVue_ - dVue);
        const float cB = balais ? 1.f : 0.f;
        gBalais_ = cB > gBalais_ ? std::min(cB, gBalais_ + dVue) : std::max(cB, gBalais_ - dVue);
        const float pluieIci = gPresence_ * gTunnel_, wExt = pluieIci * (1 - gVue_), wInt = pluieIci * gVue_;
        float sG = 0, sD = 0;
        if (pluieIci > 0) {
            const float bG = bruit(), bD = bruit();   // deux bruits indépendants : gauche et droite décorrélés (RG-03)
            if (wExt > 0) {   // RG-03 : averse, gouttes isolées, bulles des flaques
                sG += kAverse * wExt * kExt_ * extG_[1].run(extG_[0].run(bG));
                sD += kAverse * wExt * kExt_ * extD_[1].run(extD_[0].run(bD));
                if (aleatoire() < pGoutte_) {
                    nGouttes_++;
                    for (Grain& g : gouttes_) if (g.t >= g.duree) {
                        const float p = aleatoire() * 1.5707963f;
                        g = Grain{0, GOUTTE_MIN + (GOUTTE_MAX - GOUTTE_MIN) * aleatoire(), std::cos(p), std::sin(p), GOUTTE * (0.3f + 0.7f * aleatoire())};
                        break;
                    }
                }
                if (aleatoire() < pBulle_) {
                    nBulles_++;
                    for (Grain& g : bulles_) if (g.t >= g.duree) {
                        const float p = aleatoire() * 1.5707963f, f = std::min(0.45f * r, BULLE_MIN + (BULLE_MAX - BULLE_MIN) * aleatoire());
                        g = Grain{0, BULLE_DUREE, std::cos(p), std::sin(p), BULLE * (0.4f + 0.6f * aleatoire()), f, 0, f * BULLE_CHIRP / BULLE_DUREE};
                        break;
                    }
                }
                const float bg = kGoutte_ * goutteBp_.run(bruit());
                for (Grain& g : gouttes_) if (g.t < g.duree) {
                    const float env = std::min(1.f, g.t / 0.0005f) * std::exp(-g.t / (0.25f * g.duree)) * g.amp * wExt * master;
                    sG += bg * env * g.gG; sD += bg * env * g.gD; g.t += 1 / r;
                }
                for (Grain& g : bulles_) if (g.t < g.duree) {
                    const float y = std::sin(PI2 * g.ph) * std::exp(-g.t / BULLE_TAU) * g.amp * wExt * master;
                    sG += y * g.gG; sD += y * g.gD;
                    g.ph += (g.f + g.df * g.t) / r; if (g.ph >= 1) g.ph -= 1; g.t += 1 / r;
                }
            }
            if (wInt > 0) {   // RG-04 : averse derrière la tôle, tambourinage du toit et du pare-brise
                sG += kAverse * isolation * wInt * kInt_ * intG_[1].run(intG_[0].run(bG));
                sD += kAverse * isolation * wInt * kInt_ * intD_[1].run(intD_[0].run(bD));
                float tG = 0, tD = 0, pG = 0, pD = 0;
                if (aleatoire() < pToit) {   // toit horizontal : gouttes à leur vitesse de chute
                    nToit_++;
                    const float d = tailleChute_[(size_t)(aleatoire() * TABLE) % TABLE], dd = d / D_GROSSE;
                    const float a = std::min(TAMBOUR_MAX, dd * dd * dd * vitesseChute(d) / v2), p = (0.2f + 0.6f * aleatoire()) * 1.5707963f;
                    tG = a * std::cos(p); tD = a * std::sin(p);
                }
                if (aleatoire() < pPb) {   // pare-brise : chute + vitesse de la voiture projetées sur sa normale
                    nPb_++;
                    const bool chute = aleatoire() < partChute;
                    const float d = (chute ? tailleChute_ : tailleConcentration_)[(size_t)(aleatoire() * TABLE) % TABLE], dd = d / D_GROSSE;
                    const float vn = vitesseChute(d) * cosPb + vnPb;
                    const float a = std::min(TAMBOUR_MAX, dd * dd * dd * vn / v2), p = (0.3f + 0.4f * aleatoire()) * 1.5707963f;
                    pG = a * std::cos(p); pD = a * std::sin(p);
                }
                sG += kTambour * wInt * tambG_.run(kToit_ * toitG_.run(tG) + kPb_ * pbG_.run(pG));
                sD += kTambour * wInt * tambD_.run(kToit_ * toitD_.run(tD) + kPb_ * pbD_.run(pD));
            }
        }
        // RG-05 : essuie-glaces (vue Cockpit), calés sur le modèle de l'image ; RG-06 : ils continuent sous le tunnel
        depuisChoc_ += 1 / r;
        if (marche) {
            const float avant = phaseA_;
            phaseA_ += cad / r;
            if (phaseA_ >= 1) { phaseA_ -= 1; if (balais) choc(1.f); }
            else if (avant < 0.5f && phaseA_ >= 0.5f) { if (balais) choc(CHOC_HAUT); }
        }
        if (gBalais_ > 0) {
            float e = 0;
            if (marche) {
                e += FROTTEMENT * kFrot_ * frot_.run(bruit()) * std::fabs(std::sin(PI2 * phaseA_));   // frottement selon la vitesse du balai
                phBourdon_ += BOURDON_HZ / r; if (phBourdon_ >= 1) phBourdon_ -= 1;
                e += BOURDON * (std::sin(PI2 * phBourdon_) + 0.5f * std::sin(2 * PI2 * phBourdon_));
            }
            if (chocT_ < 0.2f) {
                e += CHOC * chocAmp_ * (std::sin(PI2 * CHOC_HZ * chocT_) * std::exp(-chocT_ / CHOC_TAU) + 0.5f * bruit() * std::exp(-chocT_ / CLAC_TAU));
                chocT_ += 1 / r;
            }
            sG += e * gBalais_ * master; sD += e * gBalais_ * master;
        } else if (chocT_ < 0.2f) chocT_ = 1e9f;
        G[i] += sG; D[i] += sD;
    }
    (void)cad;
}

} // namespace wfs::pluie
