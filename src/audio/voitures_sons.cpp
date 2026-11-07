// WaForSpeed — son des autres voitures (WFS-272) : voir voitures_sons.h.
#include "voitures_sons.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <map>
#include "../core/log.h"
#include "../game/cars.h"

namespace wfs::voitures {
namespace {

constexpr float PI2 = 6.28318530718f;
constexpr int TABLE = 4096;   // échantillons d'un cycle moteur (720° de vilebrequin)

// table d'un cycle : harmoniques (rang, amplitude) aux phases tirées une fois ; RMS 1
std::vector<float> table(const std::vector<std::pair<int, float>>& h, uint32_t graine) {
    std::vector<float> t(TABLE, 0.f);
    uint32_t s = graine;
    for (const auto& [k, a] : h) {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        const float ph = (float)s / 4294967295.f * PI2;
        for (int i = 0; i < TABLE; i++) t[(size_t)i] += a * std::sin(PI2 * (float)k * (float)i / TABLE + ph);
    }
    double e2 = 0; for (float x : t) e2 += (double)x * x;
    const float g = (float)(1 / std::sqrt(e2 / TABLE));
    for (float& x : t) x *= g;
    return t;
}

// bruit de roulement (3 s, en boucle sans raccord) : bande large centrée sur 1 à 2 kHz (spectre du roulement de CNOSSOS-EU)
std::vector<float> roulement(float R) {
    const size_t n = (size_t)(3 * R), x = (size_t)(0.1f * R);
    std::vector<float> b(n + x);
    sfx::Bruit br; br.s = 0x0B5E55EDu;
    sfx::PasseBande f1, f2, f3; f1.regler(1000, 0.6f, R); f2.regler(2000, 0.8f, R); f3.regler(500, 0.7f, R);
    for (float& v : b) { const float w = br.next(); v = f1.run(w) + 0.7f * f2.run(w) + 0.3f * f3.run(w); }
    std::vector<float> out(b.begin(), b.begin() + (long)n);
    for (size_t i = 0; i < x; i++) { const float k = (float)i / (float)x; out[i] = out[i] * k + b[n + i] * (1 - k); }
    double e2 = 0; for (float v : out) e2 += (double)v * v;
    const float g = (float)(1 / std::sqrt(e2 / (double)n));
    for (float& v : out) v *= g;
    return out;
}

struct Tampons { std::vector<float> tables[3], pneus; };
const Tampons& tampons(float rate) {
    static std::map<int, Tampons> parFrequence;   // jamais libérés : le thread audio les lit sans verrou
    const int cle = (int)std::lround(rate);
    auto it = parFrequence.find(cle);
    if (it != parFrequence.end()) return it->second;
    Tampons& T = parFrequence[cle];
    // 4 cylindres : combustion au rang 4 du cycle (2 tours), ses harmoniques 8, 12, 16 ; rangs 2 et 6 (un tour et demi-ordres :
    // inégalités d'un cylindre à l'autre) ; en charge, harmoniques hautes plus fortes (son plus « dur »)
    T.tables[0] = table({{1, 0.10f}, {2, 0.35f}, {3, 0.12f}, {4, 1.f}, {5, 0.08f}, {6, 0.30f}, {8, 0.50f}, {10, 0.15f}, {12, 0.25f}, {16, 0.10f}}, 0x1C0FFEEu);
    T.tables[1] = table({{1, 0.12f}, {2, 0.40f}, {3, 0.15f}, {4, 1.f}, {6, 0.40f}, {8, 0.75f}, {10, 0.30f}, {12, 0.55f}, {14, 0.20f},
                         {16, 0.40f}, {20, 0.25f}, {24, 0.18f}, {28, 0.10f}}, 0x2C0FFEEu);
    T.tables[2] = table({{1, 1.f}, {2, 0.3f}, {3, 0.1f}}, 0x3C0FFEEu);   // moteur électrique : sifflement (fCycle = sa fréquence)
    T.pneus = roulement(rate);
    return T;
}

float lireTable(const std::vector<float>& t, double ph) {   // ph dans [0, 1)
    const double x = ph * TABLE; const size_t i = (size_t)x; const float f = (float)(x - (double)i);
    return t[i % TABLE] * (1 - f) + t[(i + 1) % TABLE] * f;
}

float sommeDb(const float* l, int n) { double e = 0; for (int i = 0; i < n; i++) e += std::pow(10.0, l[i] / 10.0); return (float)(10 * std::log10(e)); }

} // namespace

// ---------------------------------------------------------------- RG-02 : niveaux
float roulementDbA(float kmh) {
    const float v = std::max(kmh, 20.f);   // CNOSSOS-EU : de 20 à 130 km/h (au-delà, prolongé)
    float l[8]; for (int b = 0; b < 8; b++) l[b] = A_R[b] + B_R[b] * std::log10(v / 70.f) + POND_A[b];
    const float L = sommeDb(l, 8);
    return kmh >= 20 ? L : L + 40.f * std::log10(std::max(kmh, 1.f) / 20.f);   // sous 20 km/h : les pneus s'éteignent
}
float propulsionDbA(float kmh) {
    float l[8]; for (int b = 0; b < 8; b++) l[b] = A_P[b] + B_P[b] * (std::max(kmh, 0.f) - 70.f) / 70.f + POND_A[b];
    return sommeDb(l, 8);
}
Parts parts(float kmh, bool charge, bool electrique) {
    static const float REF = [] { const float l[2] = {roulementDbA(50), propulsionDbA(50)}; return sommeDb(l, 2); }();   // croisière, 50 km/h
    Parts p;
    p.pneus = std::pow(10.f, (roulementDbA(kmh) - REF) / 20.f);
    const float prop = propulsionDbA(kmh) + (charge ? CHARGE_DB : 0.f) - (electrique ? 12.f : 0.f);   // électrique : sifflement discret
    p.moteur = std::pow(10.f, (prop - REF) / 20.f);
    return p;
}

// ---------------------------------------------------------------- moteur
// Boîte tirée de la fiche avec les formules de physique::etalonner (conduite_monde.h) : puissance maxi à 6 500 tr/min, coupure
// à 1,12 × ce régime, 5 rapports sous 160 ch, 7 à partir de 280 km/h, sinon 6, étagés géométriquement de la coupure à v1 à la
// puissance maxi à la vitesse de pointe. Sans l'étalonnage du 0-100 (≈ 30 ms par voiture sur PC : trop long pour le pas du
// jeu à la première rencontre d'une voiture), qui ne change pas les rapports.
float regime(const CarDef& car, float vMs, int& rapport, bool charge) {
    constexpr double TRMIN = 60 / 6.283185307179586;   // rad/s → tr/min
    const double wP = 6500 / TRMIN, wMax = 1.12 * wP, r = car.d.rw > 0.2 ? car.d.rw : 0.33, vmax = car.spec.vmax / 3.6;
    const int n = car.spec.vmax >= 280 ? 7 : car.spec.ch < 160 ? 5 : 6;
    const double gHaut = wP * r / vmax, v1 = std::max(50 / 3.6, 0.24 * vmax), gBas = wMax * r / v1;
    auto G = [&](int g) { return gBas * std::pow(gHaut / gBas, (double)(g - 1) / (n - 1)); };
    const double wRoue = std::abs((double)vMs) / r;
    const double haut = charge ? 0.95 * wMax : 2400 / TRMIN, bas = charge ? 0.55 * wMax : 1300 / TRMIN;
    rapport = std::clamp(rapport, 1, n);
    for (int k = 0; k < 8; k++) {   // hystérésis : on ne change de rapport que hors de [bas, haut]
        if (rapport < n && wRoue * G(rapport) > haut) rapport++;
        else if (rapport > 1 && wRoue * G(rapport) < bas && wRoue * G(rapport - 1) < haut) rapport--;
        else break;
    }
    const double w = std::clamp(wRoue * G(rapport), 800 / TRMIN, wMax);   // ralenti 800 tr/min
    return (float)(w * TRMIN);
}
bool electrique(const CarDef& car) { return car.id && std::strcmp(car.id, "model3") == 0; }

// ---------------------------------------------------------------- RG-05, RG-06
float doppler(vec3 ps, vec3 vs, vec3 pl, vec3 vl) {
    vec3 u = pl - ps;
    const float d = length(u);
    if (d < 0.5f) return 1.f;   // voitures confondues : pas de direction
    u = u * (1.f / d);          // de la source vers l'auditeur
    const float vS = dot(vs, u), vL = dot(vl, u);   // vS > 0 : la source s'approche ; vL > 0 : l'auditeur s'éloigne
    return std::clamp((VITESSE_SON - vL) / (VITESSE_SON - vS), 0.5f, 2.f);   // f' = f (c + v_auditeur) / (c − v_source)
}
Pan panExistant(float az) {
    const float phi = (lieux::PAN_LARGEUR * std::sin(az) + 1) * 0.25f * PI;
    return {1.41421356f * std::max(0.f, std::cos(phi)), 1.41421356f * std::max(0.f, std::sin(phi))};
}

// ---------------------------------------------------------------- RG-01 : choix des voix
void Attribution::update(float dt, const std::vector<Candidat>& c, vec3 ecoute) {
    const int n = (int)c.size();
    ordre_.clear(); d2_.resize((size_t)n);
    for (int i = 0; i < n; i++) {
        const vec3 d = c[(size_t)i].p - ecoute;
        d2_[(size_t)i] = dot(d, d);
        const float portee = c[(size_t)i].genre == POLICE ? PORTEE_POLICE : PORTEE;
        if (d2_[(size_t)i] < portee * portee) ordre_.push_back(i);
    }
    std::sort(ordre_.begin(), ordre_.end(), [&](int a, int b) { return d2_[(size_t)a] < d2_[(size_t)b]; });
    if ((int)ordre_.size() > VOIX) ordre_.resize(VOIX);   // les VOIX plus proches gagnent
    auto trouver = [&](uint64_t id) { for (int i = 0; i < n; i++) if (c[(size_t)i].id == id) return i; return -1; };
    // voix en place : la même voiture garde sa voix ; perdue (plus parmi les gagnantes) : sortie en fondu
    for (EtatVoix& v : v_) {
        if (!v.occupee) continue;
        const int i = trouver(v.id);
        const bool saut = i >= 0 && length(c[(size_t)i].p - v.c.p) > 30.f;   // voiture du trafic recyclée plus loin : autre voiture
        const bool gagne = i >= 0 && !saut && std::find(ordre_.begin(), ordre_.end(), i) != ordre_.end();
        if (i >= 0 && !saut) v.c = c[(size_t)i]; else v.c.v = {};   // disparue : immobile à sa dernière place, le temps du fondu
        if (saut) v.id = 0;   // ne sera plus reconnue
        v.sens = gagne ? 1 : -1;
    }
    // gagnantes sans voix : une voix libre (jamais une voix en cours de sortie : le fondu va au bout, sans clic)
    for (int i : ordre_) {
        bool a = false;
        for (const EtatVoix& v : v_) a = a || (v.occupee && v.id == c[(size_t)i].id && v.sens > 0);
        if (a) continue;
        for (EtatVoix& v : v_) if (!v.occupee) { v = EtatVoix{}; v.occupee = true; v.id = c[(size_t)i].id; v.c = c[(size_t)i]; v.sens = 1; break; }
    }
    for (EtatVoix& v : v_) {
        if (!v.occupee) continue;
        v.fondu = std::clamp(v.fondu + (float)v.sens * dt / FONDU, 0.f, 1.f);
        if (v.sens < 0 && v.fondu <= 0.f) v = EtatVoix{};
    }
}

Cible calculerCible(EtatVoix& e, float dt, vec3 ecoute, vec3 vEcoute, vec3 avant, vec3 droite) {
    Cible c;
    if (!e.occupee || !e.c.car) return c;
    const vec3 d = e.c.p - ecoute;
    const float dist = length(d), az = std::atan2(dot(d, droite), dot(d, avant));
    const lieux::Fiche f{e.c.genre == POLICE ? PORTEE_POLICE : PORTEE, D_PLEIN, 1.f};
    const lieux::Gains g = lieux::spatialiser(dist, az, f);
    const bool charge = e.c.genre != TRAFIC, elec = electrique(*e.c.car);
    const Parts p = parts(e.c.kmh, charge, elec);
    // niveau : NIVEAU_7_5 pour une voiture en croisière à 50 km/h à 7,5 m ; plafond doux à PLAFOND (somme quadratique)
    const float g75 = lieux::gainDistance(D_REF, D_PLEIN, f.portee);
    const float a = NIVEAU_7_5 * std::sqrt(p.moteur * p.moteur + p.pneus * p.pneus) * g.g / g75;
    const float k = a > 1e-9f ? 1.f / std::sqrt(1.f + (a / PLAFOND) * (a / PLAFOND)) : 1.f;
    const float base = NIVEAU_7_5 / g75 * k * e.fondu;
    c.gG = g.gG * base; c.gD = g.gD * base; c.coupure = g.coupure;
    c.moteur = p.moteur; c.pneus = p.pneus;
    c.timbre = elec ? 2 : charge ? 1 : 0;
    const float vMs = e.c.kmh / 3.6f;
    c.fCycle = elec ? 120.f + 18.f * vMs                                         // sifflement : monte avec la vitesse
                    : frequenceCombustion(regime(*e.c.car, vMs, e.rapport, charge)) / CYLINDRES;   // un cycle = 2 tours
    const float brut = doppler(e.c.p, e.c.v, ecoute, vEcoute);
    e.dop = e.neuve ? brut : e.dop + (brut - e.dop) * (1.f - std::exp(-dt / LISSAGE_DOPPLER));   // le mélangeur glisse ensuite sur chaque bloc
    e.neuve = false;
    c.doppler = e.dop;
    return c;
}

// ---------------------------------------------------------------- mélange
void SonsVoitures::build(float rate) {
    pret_ = false;
    const Tampons& T = tampons(rate);
    for (int t = 0; t < 3; t++) tables_[t] = &T.tables[t];
    pneus_ = &T.pneus;
    rate_ = rate; fade_ = 0; kFade_ = 1 - std::exp(-1 / (0.03f * rate));
    for (int v = 0; v < VOIX; v++) {
        Voix& x = voix_[v];
        x.cG = 0; x.cD = 0; x.cMoteur = 0; x.cPneus = 0; x.cF = 30; x.cDop = 1; x.cFc = lieux::COUPURE_PROCHE; x.cTimbre = 0;
        x.gG = x.gD = x.aM = x.aP = x.lp = 0; x.inc = 0; x.incPneu = 1;
        x.phase = 0.137 * v; x.posPneu = (double)((size_t)v * pneus_->size() / VOIX);   // voix décorrélées
    }
    for (Place& p : places_) p = Place{};
    pret_.store(true, std::memory_order_release);
}

void SonsVoitures::cible(int v, const Cible& c) {
    if (v < 0 || v >= VOIX) return;
    Voix& x = voix_[v];
    x.cG.store(c.gG, std::memory_order_relaxed); x.cD.store(c.gD, std::memory_order_relaxed);
    x.cMoteur.store(c.moteur, std::memory_order_relaxed); x.cPneus.store(c.pneus, std::memory_order_relaxed);
    x.cF.store(c.fCycle, std::memory_order_relaxed); x.cDop.store(c.doppler, std::memory_order_relaxed);
    x.cFc.store(c.coupure, std::memory_order_relaxed); x.cTimbre.store(c.timbre, std::memory_order_relaxed);
}

void SonsVoitures::jouer(const std::vector<float>* tampon, float gain, Pan pan) {
    if (!tampon || tampon->empty()) return;
    const uint32_t t = tete_.load(std::memory_order_relaxed);
    if (t - queue_.load(std::memory_order_acquire) >= (uint32_t)FILE) return;   // file pleine (pas de carte son) : ignoré
    file_[t % FILE] = {tampon, gain, pan.g, pan.d};
    tete_.store(t + 1, std::memory_order_release);
}

void SonsVoitures::mix(float* G, float* D, float* bus, int n, float master, bool actif) {
    if (!pret_.load(std::memory_order_acquire) || n <= 0) return;
    uint32_t q = queue_.load(std::memory_order_relaxed);   // nouvelles demandes → places libres (la plus avancée est reprise)
    const uint32_t t = tete_.load(std::memory_order_acquire);
    for (; q != t; q++) {
        const Demande& d = file_[q % FILE];
        int libre = 0; size_t loin = 0;
        for (int i = 0; i < PLACES; i++) { if (!places_[i].buf) { libre = i; break; } if (places_[i].pos > loin) { loin = places_[i].pos; libre = i; } }
        places_[libre] = {d.buf, 0, d.gain, d.g, d.d};
    }
    queue_.store(q, std::memory_order_release);
    // fondu de pause (30 ms) : rampe linéaire sur le bloc, de la valeur d'entrée à celle de sortie
    const float f0 = fade_, f1 = (actif ? 1.f : 0.f) + (fade_ - (actif ? 1.f : 0.f)) * std::pow(1 - kFade_, (float)n);
    fade_ = f1;
    const float pas = 1.f / (float)n, dF = (f1 - f0) * pas;
    const size_t NP = pneus_->size();
    for (int v = 0; v < VOIX; v++) {
        Voix& x = voix_[v];
        const float tG = x.cG.load(std::memory_order_relaxed) * master, tD = x.cD.load(std::memory_order_relaxed) * master;
        if (tG + tD < 1e-7f && x.gG + x.gD < 1e-7f) { x.gG = x.gD = 0; fJouee_[v] = 0; continue; }   // voix libre : rien à calculer
        const float tM = x.cMoteur.load(std::memory_order_relaxed), tP = x.cPneus.load(std::memory_order_relaxed);
        const float dop = x.cDop.load(std::memory_order_relaxed), fC = x.cF.load(std::memory_order_relaxed);
        const float tInc = fC * dop / rate_, tIncP = dop;   // RG-05 : moteur et pneus au même facteur Doppler
        const std::vector<float>& tab = *tables_[std::clamp(x.cTimbre.load(std::memory_order_relaxed), 0, 2)];
        const float a = lieux::coefPasseBas(x.cFc.load(std::memory_order_relaxed), rate_);
        if (x.inc <= 0) x.inc = tInc;   // première image de la voix : pas de glissando depuis 0
        const float dG = (tG - x.gG) * pas, dD = (tD - x.gD) * pas, dM = (tM - x.aM) * pas, dP = (tP - x.aP) * pas;
        const float dI = (tInc - x.inc) * pas, dIP = (tIncP - x.incPneu) * pas;
        float f = f0;
        for (int i = 0; i < n; i++) {
            x.gG += dG; x.gD += dD; x.aM += dM; x.aP += dP; x.inc += dI; x.incPneu += dIP; f += dF;
            x.phase += x.inc; if (x.phase >= 1) x.phase -= 1;
            x.posPneu += x.incPneu; if (x.posPneu >= (double)NP) x.posPneu -= (double)NP;
            const size_t ip = (size_t)x.posPneu; const float fp = (float)(x.posPneu - (double)ip);
            const float pneu = (*pneus_)[ip] * (1 - fp) + (*pneus_)[(ip + 1) % NP] * fp;
            x.lp += a * (x.aM * lireTable(tab, x.phase) + x.aP * pneu - x.lp);   // RG-03 / RG-04 : aigus atténués
            const float y = x.lp * f;
            G[i] += y * x.gG; D[i] += y * x.gD; bus[i] += y * 0.5f * (x.gG + x.gD);   // RG-07 : écho du tunnel
        }
        x.gG = tG; x.gD = tD; x.aM = tM; x.aP = tP; x.inc = tInc; x.incPneu = tIncP;
        fJouee_[v] = tInc * rate_ * (x.cTimbre.load(std::memory_order_relaxed) == 2 ? 1.f : (float)CYLINDRES);
    }
    for (Place& p : places_) {   // RG-06 : passage et klaxon, gauche / droite de leur voiture
        if (!p.buf) continue;
        float f = f0;
        for (int i = 0; i < n && p.buf; i++) {
            f += dF;
            const float y = (*p.buf)[p.pos] * p.gain * master * f;
            G[i] += y * p.g; D[i] += y * p.d; bus[i] += y * 0.5f * (p.g + p.d);
            if (++p.pos >= p.buf->size()) p.buf = nullptr;
        }
    }
}

} // namespace wfs::voitures
