// WaForSpeed — acoustique des lieux (WFS-275) : voir parois_sons.h.
#include "parois_sons.h"
#include <algorithm>
#include <cmath>
#include <map>
#include "../game/track.h"
#include "../world/ville_quartiers.h"

namespace wfs::parois {
namespace {

std::map<std::string, std::vector<Cotes>>& registre() { static std::map<std::string, std::vector<Cotes>> r; return r; }

constexpr float SET_BACK_VILLE = 6.5f;   // retrait des façades des immeubles par rapport au bord de la chaussée (ville.cpp, SET_BACK)

struct P2 { float x, y; };
float lon(P2 a, P2 b) { return std::hypot(b.x - a.x, b.y - a.y); }
float croix(P2 o, P2 a, P2 b) { return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x); }

// détour par les côtés : enveloppe convexe de a, b et des 4 coins ; a et b en sont des sommets (le segment traverse la boîte) ;
// le plus court des deux chemins de a à b le long de l'enveloppe, moins |ab|
float detourCotes(P2 a, P2 b, const P2 coins[4]) {
    struct Pt { P2 p; int id; };   // id : 0 = a, 1 = b, 2.. = coins
    Pt pts[6] = {{a, 0}, {b, 1}, {coins[0], 2}, {coins[1], 3}, {coins[2], 4}, {coins[3], 5}};
    std::sort(pts, pts + 6, [](const Pt& u, const Pt& v) { return u.p.x < v.p.x || (u.p.x == v.p.x && u.p.y < v.p.y); });
    Pt h[12]; int k = 0;
    for (int i = 0; i < 6; i++) { while (k >= 2 && croix(h[k - 2].p, h[k - 1].p, pts[i].p) <= 0) k--; h[k++] = pts[i]; }
    for (int i = 4, t = k + 1; i >= 0; i--) { while (k >= t && croix(h[k - 2].p, h[k - 1].p, pts[i].p) <= 0) k--; h[k++] = pts[i]; }
    const int n = k - 1;   // sommets de l'enveloppe, sens direct (le dernier répète le premier)
    int ia = -1, ib = -1;
    for (int i = 0; i < n; i++) { if (h[i].id == 0) ia = i; if (h[i].id == 1) ib = i; }
    if (ia < 0 || ib < 0) return -1;
    float c1 = 0, c2 = 0;
    for (int i = ia; i != ib; i = (i + 1) % n) c1 += lon(h[i].p, h[(i + 1) % n].p);
    for (int i = ib; i != ia; i = (i + 1) % n) c2 += lon(h[i].p, h[(i + 1) % n].p);
    return std::min(c1, c2) - lon(a, b);
}

} // namespace

// ---------------------------------------------------------------- registre
void effacer(const char* carte) { registre()[carte ? carte : ""].clear(); }

void declarer(const char* carte, int64_t L, int64_t centre, int cote, float d, float h, float longueurM) {
    if (L <= 0 || d <= 0 || d > PORTEE + (float)RW) return;
    std::vector<Cotes>& v = registre()[carte ? carte : ""];
    if ((int64_t)v.size() != L) v.assign((size_t)L, Cotes{});
    const int64_t demi = std::max<int64_t>(0, (int64_t)std::lround(longueurM / 2 / SEG_M));
    const int c = cote < 0 ? 0 : 1;
    for (int64_t n = centre - demi; n <= centre + demi; n++) {
        Paroi& p = v[(size_t)(((n % L) + L) % L)][(size_t)c];
        if (p.d <= 0 || d < p.d) p = {d, h};   // la plus proche de l'axe gagne (elle cache les autres)
    }
}

const std::vector<Cotes>& lire(const std::string& carte) {
    static const std::vector<Cotes> vide;
    auto& r = registre();
    auto it = r.find(carte);
    return it == r.end() ? vide : it->second;
}

void depuisVille(const ville::Plan& p, int64_t L) {
    effacer("ville");
    for (int64_t a = 0; a < (int64_t)p.objets.size(); a++)
        for (const ville::Objet& o : p.objets[(size_t)a]) {
            if (o.t != ville::IMMEUBLE && o.t != ville::ENTREPOT) continue;
            const float d = o.t == ville::IMMEUBLE ? (float)RW + SET_BACK_VILLE : o.lat;   // façade côté route
            declarer("ville", L, a + (int64_t)std::lround(o.len / SEG_M / 2), o.side, d, o.h, o.len);
        }
}

// ---------------------------------------------------------------- RG-03
int reflexions(float a, float b, Reflexion out[6]) {
    int n = 0;
    const float rho = std::sqrt(1.f - ALPHA);   // amplitude réfléchie par une paroi dure
    auto ajouter = [&](float chemin, int cote, int ordre) {
        if (chemin <= 0 || n >= 6) return;
        out[n++] = {chemin / lieux::VITESSE_SON, std::min(1.f, R0 / chemin) * std::pow(rho, (float)ordre), cote, ordre};
    };
    if (a > 0) ajouter(2 * a, -1, 1);
    if (b > 0) ajouter(2 * b, 1, 1);
    if (a > 0 && b > 0 && a + b <= PORTEE) {   // rue bordée des deux côtés : sources images d'ordre 2 et 3
        const float W = a + b;
        ajouter(2 * W, 1, 2);            // gauche puis droite : arrive de droite
        ajouter(2 * W, -1, 2);           // droite puis gauche : arrive de gauche
        ajouter(2 * W + 2 * a, -1, 3);
        ajouter(2 * W + 2 * b, 1, 3);
    }
    return n;
}

int Suivi::pas(float dt, float a, float b, bool tunnel, Reflexion r[6], float gG[6], float gD[6]) {
    const float k = dt / FONDU_TROUEE;   // RG-05 : un côté s'ouvre ou se ferme en 75 ms
    pG_ = std::clamp(pG_ + (a > 0 ? k : -k), 0.f, 1.f); pD_ = std::clamp(pD_ + (b > 0 ? k : -k), 0.f, 1.f);
    if (a > 0) aG_ = a;
    if (b > 0) aD_ = b;   // trouée : l'écho s'éteint à la dernière distance connue
    fT_ = std::clamp(fT_ + (tunnel ? -1.f : 1.f) * dt / lieux::FONDU_TUNNEL, 0.f, 1.f);
    const int n = reflexions(pG_ > 0 ? aG_ : 0, pD_ > 0 ? aD_ : 0, r);
    static const lieux::Fiche SANS_DISTANCE{1e9f, 1e9f, 1.f};   // panoramique de WFS-261 seul, à ±90°
    const lieux::Gains gauche = lieux::spatialiser(1, -1.5707963f, SANS_DISTANCE), droite = lieux::spatialiser(1, 1.5707963f, SANS_DISTANCE);
    for (int i = 0; i < n; i++) {
        const float pres = (r[i].ordre == 1 ? (r[i].cote < 0 ? pG_ : pD_) : pG_ * pD_) * fT_;
        const lieux::Gains& p = r[i].cote < 0 ? gauche : droite;
        gG[i] = r[i].gain * pres * p.gG; gD[i] = r[i].gain * pres * p.gD;
    }
    return n;
}

// ---------------------------------------------------------------- écho
void Echos::build(float rate) {
    pret_ = false;
    rate_ = rate;
    ligne_.assign((size_t)(0.5f * rate) + 8, 0.f);
    ecr_ = 0;
    pasXf_ = 1.f / (FONDU_PRISE * rate);
    for (Prise& p : p_) { p.cDelai = 0; p.cG = 0; p.cD = 0; p.tete = p.suivante = 0; p.xf = -1; p.gG = p.gD = 0; }
    pret_.store(true, std::memory_order_release);
}

void Echos::cible(int i, float delaiS, float gG, float gD) {
    if (i < 0 || i >= PRISES) return;
    p_[i].cDelai.store(delaiS, std::memory_order_relaxed);
    p_[i].cG.store(gG, std::memory_order_relaxed); p_[i].cD.store(gD, std::memory_order_relaxed);
}

float Echos::lire(float pos) const {   // pos : position (échantillons) dans la ligne, déjà ramenée sur [0, N)
    const size_t N = ligne_.size();
    const size_t i = (size_t)pos; const float f = pos - (float)i;
    return ligne_[i % N] * (1 - f) + ligne_[(i + 1) % N] * f;
}

void Echos::process(const float* bus, float* G, float* D, int n) {
    if (!pret_.load(std::memory_order_acquire) || n <= 0) return;
    const size_t N = ligne_.size(), debut = ecr_;
    for (int i = 0; i < n; i++) ligne_[(debut + (size_t)i) % N] = bus[i];   // tout le bloc d'abord : délai ≥ 2,9 ms
    ecr_ = (debut + (size_t)n) % N;
    const float maxD = (float)N - 4;
    for (Prise& p : p_) {
        const float tG = p.cG.load(std::memory_order_relaxed), tD = p.cD.load(std::memory_order_relaxed);
        const float tDel = std::clamp(p.cDelai.load(std::memory_order_relaxed) * rate_, 1.f, maxD);
        if (tG + tD < 1e-7f && p.gG + p.gD < 1e-7f) { p.gG = p.gD = 0; p.tete = tDel; p.xf = -1; continue; }   // muette : prête au bon délai
        if (p.xf < 0 && std::abs(tDel - p.tete) > 1.5f) {   // RG-05 : nouveau délai → fondu entre les deux prises (aucun glissement)
            if (p.gG + p.gD < 1e-6f) p.tete = tDel; else { p.suivante = tDel; p.xf = 0; }
        }
        const float dG = (tG - p.gG) / (float)n, dD = (tD - p.gD) / (float)n;
        for (int i = 0; i < n; i++) {
            p.gG += dG; p.gD += dD;
            const float ici = (float)((debut + (size_t)i) % N) + (float)N;   // + N : jamais négatif après le délai
            float y;
            if (p.xf >= 0) {
                y = (1 - p.xf) * lire(std::fmod(ici - p.tete, (float)N)) + p.xf * lire(std::fmod(ici - p.suivante, (float)N));
                if ((p.xf += pasXf_) >= 1.f) { p.tete = p.suivante; p.xf = -1; }
            } else {
                y = lire(std::fmod(ici - p.tete, (float)N));
            }
            G[i] += y * p.gG; D[i] += y * p.gD;
        }
        p.gG = tG; p.gD = tD;
    }
}

// ---------------------------------------------------------------- RG-06
float diffractionDb(float delta) {
    if (delta < 0) return 0.f;
    const float N = 2 * delta / LAMBDA_1K;   // nombre de Fresnel
    return std::min(MASQUE_MAX, 10.f * std::log10(3.f + 20.f * N));
}

std::vector<Immeuble> immeublesVille(const ville::Plan& p, int64_t L) {
    std::vector<Immeuble> out;
    for (int64_t a = 0; a < (int64_t)p.objets.size(); a++)
        for (const ville::Objet& o : p.objets[(size_t)a]) {
            if (o.t != ville::IMMEUBLE && o.t != ville::ENTREPOT) continue;
            const float pres = o.t == ville::IMMEUBLE ? (float)RW + SET_BACK_VILLE : o.lat;
            // centre : au milieu de la longueur (ville.cpp : poseS(a + len / 2) pour l'immeuble, poseAt(a + arrondi) pour l'entrepôt)
            const double sc = o.t == ville::IMMEUBLE ? (double)a + o.len / SEG_M / 2 : (double)(a + (int64_t)std::floor(o.len / SEG_M / 2 + 0.5));
            Immeuble im;
            const double sf = std::floor(sc);
            im.centre.seg = L > 0 ? (((int64_t)sf % L) + L) % L : (int64_t)sf;
            im.centre.avant = (float)((sc - sf) * SEG_M);
            im.centre.lat = (float)o.side * (pres + o.d / 2);
            im.demiProf = o.d / 2; im.demiLong = o.len / 2; im.h = o.h;
            out.push_back(im);
        }
    return out;
}

float detour(vec3 a, vec3 b, vec3 c, vec3 r, vec3 f, float hp, float hl, float h) {
    auto loc = [&](vec3 p) { const vec3 d = p - c; return P2{dot(d, r), dot(d, f)}; };
    const P2 A = loc(a), B = loc(b);
    // Liang–Barsky : partie du segment dans le rectangle |x| ≤ hp, |y| ≤ hl
    float t0 = 0, t1 = 1;
    const float dx = B.x - A.x, dy = B.y - A.y;
    auto couper = [&](float p, float q) {   // p · t ≤ q
        if (std::abs(p) < 1e-9f) return q >= 0;
        const float t = q / p;
        if (p < 0) { if (t > t1) return false; t0 = std::max(t0, t); }
        else { if (t < t0) return false; t1 = std::min(t1, t); }
        return true;
    };
    if (!couper(-dx, A.x + hp) || !couper(dx, hp - A.x) || !couper(-dy, A.y + hl) || !couper(dy, hl - A.y) || t0 >= t1) return -1;
    if (t0 <= 0 || t1 >= 1) return -1;   // une extrémité dans la boîte : pas de masque (cas hors du décor)
    const float L2 = lon(A, B), L3 = length(b - a), H = c.y + h;
    const float yE = a.y + (b.y - a.y) * t0, yX = a.y + (b.y - a.y) * t1;
    if (yE >= H && yX >= H) return -1;   // la ligne passe au-dessus du toit
    const float toit = std::hypot(t0 * L2, H - a.y) + (t1 - t0) * L2 + std::hypot((1 - t1) * L2, H - b.y) - L3;
    const P2 coins[4] = {{-hp, -hl}, {hp, -hl}, {hp, hl}, {-hp, hl}};
    const float cotes = detourCotes(A, B, coins);
    const float d = cotes < 0 ? toit : std::min(toit, cotes);
    return std::max(0.f, d);
}

void appliquerMasque(float& gG, float& gD, float& coupure, float& m, float cible, float dt) {
    m += (cible - m) * (1.f - std::exp(-dt / MASQUE_TAU));
    if (m < 0.01f) return;
    const float k = std::pow(10.f, -m / 20.f), w = std::min(1.f, m / 4.8f);   // 4,8 dB : masque le plus faible (δ = 0)
    gG *= k; gD *= k; coupure += (std::min(coupure, MASQUE_COUPURE) - coupure) * w;
}

float masqueDb(vec3 a, vec3 s, const std::vector<ImmeubleMonde>& proches) {
    float m = 0;
    for (const ImmeubleMonde& im : proches) {
        const float dl = detour(a, s, im.c, im.r, im.f, im.demiProf, im.demiLong, im.h);
        if (dl >= 0) m = std::max(m, diffractionDb(dl));
    }
    return m;
}

} // namespace wfs::parois
