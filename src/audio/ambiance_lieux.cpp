// WaForSpeed — son des lieux (WFS-261) : voir ambiance_lieux.h.
#include "ambiance_lieux.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include "../core/log.h"
#include "../game/track.h"

namespace wfs::lieux {
namespace {

using sfx::Bruit;
using sfx::PasseBande;
constexpr float PI2 = 6.28318530718f;
constexpr float COMPENSATION_AMORTI = 1.33f;   // TR60 mesuré ≈ 0,75 × consigne des peignes (amorti 0,25)

// ---------------------------------------------------------------- registre
std::map<std::string, std::array<std::vector<Lieu>, NB_TYPES>>& registre() {
    static std::map<std::string, std::array<std::vector<Lieu>, NB_TYPES>> r;
    return r;
}

// ---------------------------------------------------------------- outils de synthèse (thread du jeu)
struct PasseBas {   // un pôle
    float a = 0, y = 0;
    void regler(float fc, float rate) { a = coefPasseBas(fc, rate); }
    float run(float x) { y += a * (x - y); return y; }
};

// bruit lent (modulation), RMS 1 : bruit blanc passé deux fois dans un passe-bas à fc
std::vector<float> bruitLent(size_t n, float fc, float rate, uint32_t graine) {
    std::vector<float> m(n);
    Bruit br; br.s = graine;
    PasseBas a, b; a.regler(fc, rate); b.regler(fc, rate);
    double e2 = 0;
    for (size_t i = 0; i < n; i++) { m[i] = b.run(a.run(br.next())); e2 += (double)m[i] * m[i]; }
    const float k = e2 > 0 ? (float)(1 / std::sqrt(e2 / (double)n)) : 0.f;
    for (float& x : m) x *= k;
    return m;
}

// boucle sans raccord audible : les x derniers échantillons calculés (au-delà de n) se fondent dans le début
std::vector<float> fermer(const std::vector<float>& tmp, size_t n) {
    const size_t x = tmp.size() - n;
    std::vector<float> out(tmp.begin(), tmp.begin() + (long)n);
    for (size_t i = 0; i < x; i++) { const float k = (float)i / (float)x; out[i] = out[i] * k + tmp[n + i] * (1 - k); }
    return out;
}

void normaliserRms(std::vector<float>& b) {
    double e2 = 0;
    for (float x : b) e2 += (double)x * x;
    if (e2 <= 0) return;
    const float k = (float)(1 / std::sqrt(e2 / (double)b.size()));
    for (float& x : b) x *= k;
}

void normaliserCrete(std::vector<float>& b) {
    float p = 0;
    for (float x : b) p = std::max(p, std::abs(x));
    if (p > 1e-9f) for (float& x : b) x /= p;
}

float scie(double ph) { return (float)(2 * (ph - std::floor(ph + 0.5))); }

// fontaine (8 s) : chute d'eau (souffle 2,6 kHz + grondement 650 Hz) modulée par les jets, et bulles (sinus brefs de 1,2 à 4 kHz)
std::vector<float> fontaine(float R) {
    const size_t n = (size_t)(8 * R), N = n + (size_t)(0.25f * R);
    std::vector<float> b(N);
    const std::vector<float> jets = bruitLent(N, 18, R, 0x2F6B1C3Du);
    Bruit br; br.s = 0x51F0A7E1u;
    PasseBande souffle, chute; souffle.regler(2600, 0.5f, R); chute.regler(650, 0.8f, R);
    for (size_t i = 0; i < N; i++) {
        const float w = br.next();
        b[i] = (souffle.run(w) + 0.45f * chute.run(w)) * (0.7f + 0.3f * std::clamp(jets[i], -1.f, 1.f));
    }
    Bruit h; h.s = 0x0B0B1E5u;
    const size_t bulle = (size_t)(0.02f * R);
    for (int k = 0; k < 35 * 8; k++) {   // 35 bulles par seconde
        const size_t at = (size_t)((h.next() * 0.5f + 0.5f) * (float)(N - bulle - 1));
        const float f = 1200 + (h.next() * 0.5f + 0.5f) * 2800, a = 0.15f + 0.15f * (h.next() * 0.5f + 0.5f);
        for (size_t j = 0; j < bulle; j++) b[at + j] += a * std::sin(PI2 * f * (float)j / R) * std::exp(-(float)j / (0.004f * R));
    }
    std::vector<float> out = fermer(b, n);
    normaliserRms(out);
    return out;
}

// vagues (16 s) : deux déferlantes (montée de 0,7 s, puis ressac de 1,6 s de constante de temps), plus brillantes au déferlement,
// sur un ressac de fond ; grondement grave des rouleaux
std::vector<float> vagues(float R) {
    const size_t n = (size_t)(16 * R), N = n + (size_t)(0.5f * R);
    std::vector<float> b(N);
    Bruit br; br.s = 0x7E57A11Du;
    PasseBande ecume, rouleau; ecume.regler(1100, 0.45f, R); rouleau.regler(160, 0.6f, R);
    PasseBas brillance;
    const float cretes[2] = {1.2f, 9.3f};
    auto vague = [](float u) { return u < 0 ? 0.f : u < 0.7f ? (u / 0.7f) * (u / 0.7f) : std::exp(-(u - 0.7f) / 1.6f); };
    for (size_t i = 0; i < N; i++) {
        const float t = std::fmod((float)i / R, 16.f);
        float env = 0.22f;
        for (float c : cretes) env += vague(t - c) + vague(t - c + 16);
        if (i % 32 == 0) brillance.regler(600 + 3400 * std::clamp(env - 0.22f, 0.f, 1.f), R);
        const float w = br.next();
        b[i] = (brillance.run(ecume.run(w) + 0.15f * w) + 0.8f * rouleau.run(w)) * env;
    }
    std::vector<float> out = fermer(b, n);
    normaliserRms(out);
    return out;
}

// voix d'une foule (tribune, terrasse) : bruit filtré à la hauteur d'une voix (formant), haché au rythme des syllabes
void babil(std::vector<float>& b, float R, int voix, float f0, float f1, float s0, float s1, uint32_t graine) {
    Bruit h; h.s = graine;
    auto u = [&]() { return h.next() * 0.5f + 0.5f; };
    for (int v = 0; v < voix; v++) {
        const float f = f0 + u() * (f1 - f0), sr = s0 + u() * (s1 - s0), ph = u() * PI2, g = 0.6f + 0.4f * u();
        const std::vector<float> phrase = bruitLent(b.size(), 0.4f, R, graine * 31 + (uint32_t)v * 7919u + 1);   // pauses entre les phrases
        PasseBande f1b, f2b; f1b.regler(f, 3.f, R); f2b.regler(2 * f + 300, 4.f, R);
        Bruit br; br.s = graine ^ (0x9E3779B9u * (uint32_t)(v + 1));
        for (size_t i = 0; i < b.size(); i++) {
            const float s = std::sin(PI2 * sr * (float)i / R + ph), syl = s > 0 ? s * std::sqrt(s) : 0.f;
            const float w = br.next();
            b[i] += (f1b.run(w) + 0.5f * f2b.run(w)) * syl * g * (phrase[i] > -0.3f ? 1.f : 0.f);
        }
    }
}

// tribune (8 s) : brouhaha de 10 voix et deux clameurs (montée 0,8 s, tenue 1 s, retombée 1,5 s), plus brillantes
std::vector<float> tribune(float R) {
    const size_t n = (size_t)(8 * R), N = n + (size_t)(0.25f * R);
    std::vector<float> b(N, 0.f);
    babil(b, R, 10, 280, 980, 3, 6, 0x7B1B0E5u);
    Bruit br; br.s = 0x0C1A3E07u;
    PasseBande cri, rumeur; cri.regler(1500, 0.7f, R); rumeur.regler(800, 0.7f, R);
    const float clameurs[2] = {1.0f, 5.2f};
    for (size_t i = 0; i < N; i++) {
        const float t = std::fmod((float)i / R, 8.f);
        float e = 0;
        for (float c : clameurs) { const float x = t - c; e += x < 0 ? 0.f : x < 0.8f ? x / 0.8f : x < 1.8f ? 1.f : std::exp(-(x - 1.8f) / 0.5f); }
        const float w = br.next();
        b[i] = b[i] * (1 + 0.6f * e) + (cri.run(w) + rumeur.run(w)) * 1.2f * e;
    }
    std::vector<float> out = fermer(b, n);
    normaliserRms(out);
    return out;
}

// terrasse (8 s) : conversations de 6 voix et tintements de verres et de couverts (deux partiels, 50 ms)
std::vector<float> terrasse(float R) {
    const size_t n = (size_t)(8 * R), N = n + (size_t)(0.25f * R);
    std::vector<float> b(N, 0.f);
    babil(b, R, 6, 350, 1400, 4, 7, 0x7E2A55Eu);
    normaliserRms(b);
    Bruit h; h.s = 0x71A7E5u;
    auto u = [&]() { return h.next() * 0.5f + 0.5f; };
    const size_t L = (size_t)(0.12f * R);
    for (float t = 0.2f; t < 8.f; t += 0.25f + 0.5f * u()) {
        const size_t at = (size_t)(t * R);
        const float f = 2800 + 800 * u(), a = 0.6f + 0.6f * u();
        for (size_t j = 0; j < L && at + j < N; j++) {
            const float x = (float)j / R;
            b[at + j] += a * (std::sin(PI2 * f * x) + 0.5f * std::sin(PI2 * f * 1.62f * x)) * std::exp(-x / 0.05f) * std::min(1.f, x / 0.0005f);
        }
    }
    std::vector<float> out = fermer(b, n);
    normaliserRms(out);
    return out;
}

// appels de la ferme (crête 1) : forme d'onde source dans des formants, enveloppe d'attaque et de relâche
std::vector<float> vache(float R) {   // meuglement : « m » nasal grave qui s'ouvre en « ou », hauteur qui retombe
    const float D = 1.8f; const size_t n = (size_t)(D * R);
    std::vector<float> b(n);
    PasseBande nasal, ou, a2; nasal.regler(220, 4, R); ou.regler(650, 2.5f, R); a2.regler(1100, 3, R);
    Bruit br; br.s = 0x1F00Du; double ph = 0;
    for (size_t i = 0; i < n; i++) {
        const float t = (float)i / R, f = (118 - 18 * t / D) * (1 + 0.006f * std::sin(PI2 * 5 * t));
        const float o = std::clamp((t - 0.15f) / 0.45f, 0.f, 1.f), ouv = o * o * (3 - 2 * o);
        const float x = scie(ph) + 0.05f * br.next();
        const float env = std::min(1.f, (t / 0.3f) * (t / 0.3f)) * std::clamp((D - t) / 0.5f, 0.f, 1.f);
        b[i] = ((1 - ouv) * nasal.run(x) * 2 + ouv * (ou.run(x) + 0.5f * a2.run(x))) * env;
        ph += f / R;
    }
    normaliserCrete(b);
    return b;
}
std::vector<float> cheval(float R) {   // hennissement : glissando de 900 à 520 Hz, chevrotement de 9 Hz, puis ébrouement
    const float D = 1.5f; const size_t n = (size_t)(D * R);
    std::vector<float> b(n);
    PasseBande f1, f2, souffle; f1.regler(1300, 2.5f, R); f2.regler(2600, 3, R); souffle.regler(500, 0.7f, R);
    Bruit br; br.s = 0xC4E7A1u; double ph = 0;
    for (size_t i = 0; i < n; i++) {
        const float t = (float)i / R, g = std::min(t / 1.1f, 1.f);
        const float f = 900 * std::pow(520.f / 900.f, g) * (1 + 0.07f * std::sin(PI2 * 9 * t));
        const float voix = t < 1.15f ? std::min(1.f, t / 0.05f) * std::clamp((1.15f - t) / 0.2f, 0.f, 1.f) : 0.f;
        const float ebrou = t > 1.1f ? std::min(1.f, (t - 1.1f) / 0.02f) * std::exp(-(t - 1.1f) / 0.12f) : 0.f;
        const float x = scie(ph) * (0.8f + 0.2f * br.next());
        b[i] = (f1.run(x) + 0.6f * f2.run(x)) * voix + souffle.run(br.next()) * 3 * ebrou;
        ph += f / R;
    }
    normaliserCrete(b);
    return b;
}
std::vector<float> mouton(float R) {   // bêlement : 320 Hz chevrotant à 22 Hz, formants 850 Hz et 2,3 kHz
    const float D = 1.0f; const size_t n = (size_t)(D * R);
    std::vector<float> b(n);
    PasseBande f1, f2, f3; f1.regler(850, 3, R); f2.regler(2300, 4, R); f3.regler(3200, 5, R);
    double ph = 0;
    for (size_t i = 0; i < n; i++) {
        const float t = (float)i / R, f = 320 * (1 + 0.04f * std::sin(PI2 * 5 * t));
        const float trem = 1 - 0.55f * (0.5f + 0.5f * std::sin(PI2 * 22 * t));
        const float env = std::min(1.f, t / 0.06f) * std::exp(-t / 0.6f) * std::clamp((D - t) / 0.1f, 0.f, 1.f);
        const float x = scie(ph);
        b[i] = (f1.run(x) + 0.7f * f2.run(x) + 0.3f * f3.run(x)) * trem * env;
        ph += f / R;
    }
    normaliserCrete(b);
    return b;
}

// tampons calculés une fois par fréquence de sortie et gardés (une partie suivante ne recalcule rien) ; jamais libérés :
// le thread audio peut les lire sans verrou
struct Tampons { std::vector<float> boucles[NB_TYPES], appels[ESPECES], fondu; };
const Tampons& tampons(float rate) {
    static std::map<int, Tampons> parFrequence;   // nœuds stables : aucun tampon déplacé par un ajout
    const int cle = (int)std::lround(rate);
    auto it = parFrequence.find(cle);
    if (it != parFrequence.end()) return it->second;
    const auto debut = std::chrono::steady_clock::now();
    Tampons& T = parFrequence[cle];
    T.boucles[FONTAINE] = fontaine(rate); T.boucles[VAGUES] = vagues(rate);
    T.boucles[TRIBUNE] = tribune(rate); T.boucles[TERRASSE] = terrasse(rate);
    T.appels[0] = vache(rate); T.appels[1] = cheval(rate); T.appels[2] = mouton(rate);
    T.fondu = sfx::fonduBoucle(rate, 0.25f);
    size_t total = T.fondu.size();
    for (auto& b : T.boucles) total += b.size();
    for (auto& b : T.appels) total += b.size();
    WFS_LOG("son des lieux (WFS-261) : sources calculées à %d Hz en %.0f ms (%.1f Mo, une fois par lancement)", cle,
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - debut).count(), total * 4 / 1e6);
    return T;
}

} // namespace

const char* nomType(int t) {
    static const char* NOMS[NB_TYPES] = {"fontaine", "vagues", "tribune", "ferme", "terrasse"};
    return t >= 0 && t < NB_TYPES ? NOMS[t] : "?";
}

// ---------------------------------------------------------------- registre et repère de la route
void poser(const char* carte, Type t, std::vector<Lieu> l) {
    if (t >= NB_TYPES) return;
    registre()[carte ? carte : ""][t] = std::move(l);
}

const std::vector<Lieu>& lire(const std::string& carte, Type t) {
    static const std::vector<Lieu> vide;
    auto& r = registre();
    auto it = r.find(carte);
    return it == r.end() || t >= NB_TYPES ? vide : it->second[t];
}

Lieu depuisMonde(Track& t, int64_t seg, vec3 p) {
    const Pose q = t.pose(seg);
    const double c = std::cos(q.th), s = std::sin(q.th), dx = p.x - q.x, dz = p.z - q.z;
    Lieu l; l.seg = seg;
    l.lat = (float)(dx * c + dz * s);     // droite du segment : (cos θ, 0, sin θ)
    l.avant = (float)(dx * s - dz * c);   // avant : (sin θ, 0, −cos θ)
    l.dy = (float)(p.y - q.y);
    return l;
}

vec3 positionMonde(Track& t, const Lieu& l, int64_t aP) {
    const int64_t L = (int64_t)t.segmentCount();
    const int64_t k = L > 0 ? (int64_t)std::floor((double)(aP - l.seg) / (double)L + 0.5) : 0;   // tour le plus proche du joueur
    const Pose q = t.pose(l.seg + k * L);
    const double c = std::cos(q.th), s = std::sin(q.th);
    return {(float)(q.x + c * l.lat + s * l.avant), (float)(q.y + l.dy), (float)(q.z + s * l.lat - c * l.avant)};
}

std::vector<Lieu> regrouperTerrasses(const std::vector<Attable>& cl, int64_t longueur) {
    std::vector<Lieu> out;
    for (size_t i = 0; i < cl.size();) {
        size_t j = i + 1;
        double s0 = cl[i].s, s1 = cl[i].s; float lat = cl[i].lat;
        while (j < cl.size() && (cl[j].lat > 0) == (cl[i].lat > 0) && std::abs(cl[j].s - cl[j - 1].s) * SEG_M < 6) {
            s0 = std::min(s0, cl[j].s); s1 = std::max(s1, cl[j].s); lat += cl[j].lat; j++;
        }
        const double s = (s0 + s1) / 2;
        Lieu l;
        const double sf = std::floor(s);
        l.seg = longueur > 0 ? (((int64_t)sf % longueur) + longueur) % longueur : (int64_t)sf;
        l.avant = (float)((s - sf) * SEG_M); l.lat = lat / (float)(j - i); l.dy = 1.2f;   // tête d'un client assis
        out.push_back(l);
        i = j;
    }
    return out;
}

// ---------------------------------------------------------------- calculs purs
float gainDistance(float d, float d0, float portee) {
    if (!(d < portee)) return 0.f;
    float g = d <= d0 ? 1.f : d0 / d;   // source ponctuelle : amplitude en 1/d (−6,02 dB par doublement)
    const float f0 = 0.9f * portee;
    if (d > f0) g *= (portee - d) / (portee - f0);   // aucune marche à la portée
    return g;
}

float coupureDistance(float d, float portee) {
    if (d <= 1.f) return COUPURE_PROCHE;
    if (d >= portee) return COUPURE_PORTEE;
    const float x = std::log(d) / std::log(portee);
    return COUPURE_PROCHE * std::pow(COUPURE_PORTEE / COUPURE_PROCHE, x);
}

Gains spatialiser(float d, float az, const Fiche& f) {
    Gains g;
    const float dos = std::max(0.f, -std::cos(az));   // 0 devant et sur les côtés, 1 juste derrière
    g.g = gainDistance(d, f.d0, f.portee) * std::pow(10.f, DOS_DB * dos / 20.f);
    // largeur PAN_LARGEUR < 1 : une source sur le côté reste un peu dans l'autre oreille (≈ −16 dB à 90°, comme l'ombre de
    // la tête ; règle : ≤ −9 dB), jamais un silence total d'un côté
    const float phi = (PAN_LARGEUR * std::sin(az) + 1) * 0.25f * PI;   // 0 tout à gauche, π/4 au milieu, π/2 tout à droite
    g.gG = g.g * std::max(0.f, std::cos(phi));
    g.gD = g.g * std::max(0.f, std::sin(phi));
    const float fc = coupureDistance(d, f.portee);
    g.coupure = fc + (std::min(fc, COUPURE_DOS) - fc) * dos;
    return g;
}

float coefPasseBas(float fc, float rate) { return 1.f - std::exp(-PI2 * std::clamp(fc, 10.f, rate * 0.45f) / rate); }

void entrelacer(const float* m, const float* G, const float* D, float* out, int n, int canaux) {
    // aucun écrêtage ici : le son existant arrive tel qu'avant (la sortie écrêtait déjà le flux mono)
    if (canaux <= 1) { for (int i = 0; i < n; i++) out[i] = m[i] + (G[i] + D[i]) * 0.70710678f; return; }
    for (int i = 0; i < n; i++) {
        float* o = out + (size_t)i * (size_t)canaux;
        o[0] = m[i] + G[i]; o[1] = m[i] + D[i];
        for (int c = 2; c < canaux; c++) o[c] = 0.f;
    }
}

// ---------------------------------------------------------------- tunnel
void Tunnel::build(float rate, float largeur, float tr60) {
    pret_ = false;
    rate_ = rate; delai_ = largeur / VITESSE_SON;
    r1_ = (size_t)std::lround(delai_ * rate); r2_ = 2 * r1_;
    avant_.b.assign(r2_ + 1, 0.f); avant_.i = 0;
    // délais des peignes (ms) : premiers entre eux, plus longs que ceux d'une salle (tube de 23 m), décalés à droite
    static const float MS[2][4] = {{37.1f, 41.3f, 45.7f, 49.9f}, {37.7f, 41.9f, 46.4f, 50.6f}};
    static const float AP[2][2] = {{5.0f, 1.7f}, {5.3f, 1.9f}};
    for (int c = 0; c < 2; c++) {
        for (int k = 0; k < 4; k++) {
            Peigne& p = peignes_[c][k];
            const size_t n = (size_t)std::lround(MS[c][k] * 1e-3f * rate);
            p.l.b.assign(n, 0.f); p.l.i = 0; p.filtre = 0;
            // −60 dB en tr60 : le filtre d'amorti (aigus) raccourcit la queue mesurée d'un quart (test_audio_lieux, intégration
            // de Schroeder) : la consigne des peignes est allongée d'autant pour que le TR60 MESURÉ soit tr60
            p.fb = std::pow(10.f, -3.f * ((float)n / rate) / (tr60 * COMPENSATION_AMORTI));
        }
        for (int k = 0; k < 2; k++) { PasseTout& a = passeTout_[c][k]; a.l.b.assign((size_t)std::lround(AP[c][k] * 1e-3f * rate), 0.f); a.l.i = 0; }
    }
    pasFondu_ = 1.f / (FONDU_TUNNEL * rate);
    mix_ = 0; vide_ = true;
    pret_.store(true, std::memory_order_release);
}

void Tunnel::process(const float* in, float* G, float* D, int n) {
    if (!pret_.load(std::memory_order_acquire)) return;
    const bool c = cible_.load();
    if (!c && mix_ <= 0.f) {   // dehors, fondu fini : rien à calculer ; tampons remis à zéro une fois (pas de queue fantôme)
        if (!vide_) {
            std::fill(avant_.b.begin(), avant_.b.end(), 0.f);
            for (auto& cote : peignes_) for (Peigne& p : cote) { std::fill(p.l.b.begin(), p.l.b.end(), 0.f); p.filtre = 0; }
            for (auto& cote : passeTout_) for (PasseTout& a : cote) std::fill(a.l.b.begin(), a.l.b.end(), 0.f);
            vide_ = true;
        }
        return;
    }
    vide_ = false;
    const size_t N = avant_.b.size();
    for (int i = 0; i < n; i++) {
        mix_ = c ? std::min(1.f, mix_ + pasFondu_) : std::max(0.f, mix_ - pasFondu_);
        avant_.b[avant_.i] = in[i];
        const float e1 = avant_.b[(avant_.i + N - r1_) % N], e2 = avant_.b[(avant_.i + N - r2_) % N];
        if (++avant_.i >= N) avant_.i = 0;
        float s[2];
        for (int k = 0; k < 2; k++) {
            float acc = 0;
            for (Peigne& p : peignes_[k]) {   // peigne amorti (Schroeder / Moorer) : les aigus meurent plus vite, comme sur le béton
                float& b = p.l.b[p.l.i];
                const float y = b;
                p.filtre = y * (1 - p.amort) + p.filtre * p.amort;
                b = e1 + p.filtre * p.fb;
                if (++p.l.i >= p.l.b.size()) p.l.i = 0;
                acc += y;
            }
            acc *= 0.25f;
            for (PasseTout& a : passeTout_[k]) {   // diffusion : passe-tout de Schroeder
                float& b = a.l.b[a.l.i];
                const float w = acc + a.g * b, y = b - a.g * w;
                b = w;
                if (++a.l.i >= a.l.b.size()) a.l.i = 0;
                acc = y;
            }
            s[k] = acc;
        }
        const float er = 0.6f * e1 + 0.35f * e2;   // réflexions précoces : une, puis deux traversées du tube
        const float g = gainHumide * mix_;
        G[i] += (er + s[0]) * g;
        D[i] += (er + s[1]) * g;
    }
}

// ---------------------------------------------------------------- sources placées
void SonsDesLieux::build(float rate, bool nuit) {
    pret_ = false;
    const Tampons& T = tampons(rate);
    rate_ = rate; gainNuit_ = nuit ? 0.5f : 1.f;   // RG-06 : la nuit, moitié du volume (comme l'ambiance de la carte)
    for (int t = 0; t < NB_TYPES; t++) {
        Voix& v = voix_[t];
        v.boucle = t == FERME ? nullptr : &T.boucles[t];
        v.lecteur = sfx::LecteurAleatoire(0x5EED0000u + (uint32_t)t * 7919u);
        v.cG = 0; v.cD = 0; v.cFc = COUPURE_PROCHE; v.gG = v.gD = v.lp = 0;
    }
    for (int e = 0; e < ESPECES; e++) appels_[e] = &T.appels[e];
    fondu_ = &T.fondu;
    appel_ = nullptr; appelPos_ = 0; attenteAppel_ = (int64_t)(2 * rate);
    pret_.store(true, std::memory_order_release);
}

void SonsDesLieux::cible(int type, const Gains& g, int variante) {
    if (type < 0 || type >= NB_TYPES) return;
    Voix& v = voix_[type];
    v.cG.store(g.gG, std::memory_order_relaxed); v.cD.store(g.gD, std::memory_order_relaxed);
    v.cFc.store(g.coupure, std::memory_order_relaxed);
    if (type == FERME) espece_.store(std::clamp(variante, 0, ESPECES - 1), std::memory_order_relaxed);
}

void SonsDesLieux::mix(float* G, float* D, int n, float master) {
    if (!pret_.load(std::memory_order_acquire) || n <= 0) return;
    const bool on = actif_.load();
    const float m = master * gainNuit_, pas = 1.f / (float)n;
    for (int t = 0; t < NB_TYPES; t++) {
        Voix& v = voix_[t];
        const float k = on ? FICHES[t].volume * m : 0.f;
        const float tG = v.cG.load(std::memory_order_relaxed) * k, tD = v.cD.load(std::memory_order_relaxed) * k;
        if (tG + tD < 1e-7f && v.gG + v.gD < 1e-7f) {   // muette : rien à calculer (un appel en cours s'arrête)
            v.gG = v.gD = 0;
            if (t == FERME) appel_ = nullptr;
            continue;
        }
        const float a = coefPasseBas(v.cFc.load(std::memory_order_relaxed), rate_);
        const float dG = (tG - v.gG) * pas, dD = (tD - v.gD) * pas;   // rampe sur le bloc : aucun « zip » quand la voiture bouge
        for (int i = 0; i < n; i++) {
            float x;
            if (t == FERME) {   // appels espacés de 3 à 8 s, de l'espèce du troupeau le plus proche
                x = 0;
                if (!appel_ && --attenteAppel_ <= 0) {
                    appel_ = appels_[espece_.load(std::memory_order_relaxed)]; appelPos_ = 0;
                    hasard_ ^= hasard_ << 13; hasard_ ^= hasard_ >> 17; hasard_ ^= hasard_ << 5;
                    attenteAppel_ = (int64_t)(rate_ * (3.f + 5.f * (float)(hasard_ >> 8) / 16777216.f));
                }
                if (appel_) { x = (*appel_)[appelPos_]; if (++appelPos_ >= appel_->size()) appel_ = nullptr; }
            } else {
                x = v.lecteur.lire(*v.boucle, *fondu_);
            }
            v.lp += a * (x - v.lp);   // RG-03 / RG-04 : aigus atténués avec la distance et de dos
            v.gG += dG; v.gD += dD;
            G[i] += v.lp * v.gG; D[i] += v.lp * v.gD;
        }
        v.gG = tG; v.gD = tD;
    }
}

} // namespace wfs::lieux
