// WaForSpeed — acoustique des lieux (WFS-275) : voir parois_sons.h.
#include "parois_sons.h"
#include <algorithm>
#include <cmath>
#include <map>
#include "../game/track.h"
#include "../world/ville_quartiers.h"

namespace wfs::parois {
namespace {

// par carte : parois par segment (écho) et obstacles (masques, WFS-278) ; fin : dernier segment couvert par chaque obstacle (réunion)
struct Carte { std::vector<Cotes> parois; std::vector<Immeuble> obstacles; std::vector<double> debut, fin; std::vector<int> cote; std::vector<float> d, prof; };
std::map<std::string, Carte>& registre() { static std::map<std::string, Carte> r; return r; }
constexpr float REUNION_MAX = 15.f;   // m : longueur au plus d'un obstacle réuni (la route tourne : la boîte suit son segment central)

constexpr float SET_BACK_VILLE = 6.5f;   // retrait des façades des immeubles par rapport au bord de la chaussée (ville.cpp, SET_BACK)

struct P2 { float x, y; };
float lon(P2 a, P2 b) { const float x = b.x - a.x, y = b.y - a.y; return std::sqrt(x * x + y * y); }   // (hypotf de la libc : lente)
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
void effacer(const char* carte) { registre()[carte ? carte : ""] = Carte{}; }

void declarer(const char* carte, int64_t L, int64_t centre, int cote, float d, float h, float longueurM, float profondeur, float alpha) {
    if (L <= 0 || d <= 0) return;
    Carte& k = registre()[carte ? carte : ""];
    if (profondeur > 0) {   // WFS-278 (RG-03) : obstacle des masques (même au-delà de la portée de l'écho)
        const double deb = (double)centre - longueurM / 2 / SEG_M, fin = (double)centre + longueurM / 2 / SEG_M;
        const size_t n = k.obstacles.size();
        if (n && k.cote[n - 1] == cote && deb - k.fin[n - 1] <= 1.0 && deb >= k.debut[n - 1] && std::abs(k.d[n - 1] - d) < 0.5f &&
            std::abs(k.prof[n - 1] - profondeur) < 0.5f && (fin - k.debut[n - 1]) * SEG_M <= REUNION_MAX) {
            k.fin[n - 1] = fin;   // morceau voisin du même mur : réuni (moins d'obstacles à tester à chaque pas)
            Immeuble& im = k.obstacles[n - 1];
            const double c = (k.debut[n - 1] + fin) / 2, cf = std::floor(c);
            im.centre.seg = (((int64_t)cf % L) + L) % L; im.centre.avant = (float)((c - cf) * SEG_M);
            im.demiLong = (float)((fin - k.debut[n - 1]) * SEG_M / 2); im.h = std::min(im.h, h);
        } else {
            Immeuble im;
            im.centre.seg = ((centre % L) + L) % L; im.centre.lat = (float)(cote < 0 ? -1 : 1) * (d + profondeur / 2);
            im.demiProf = profondeur / 2; im.demiLong = longueurM / 2; im.h = h;
            k.obstacles.push_back(im); k.debut.push_back(deb); k.fin.push_back(fin); k.cote.push_back(cote); k.d.push_back(d); k.prof.push_back(profondeur);
        }
    }
    if (d > PORTEE + (float)RW) return;   // trop loin pour l'écho
    std::vector<Cotes>& v = k.parois;
    if ((int64_t)v.size() != L) v.assign((size_t)L, Cotes{});
    const int64_t demi = std::max<int64_t>(0, (int64_t)std::lround(longueurM / 2 / SEG_M));
    const int c = cote < 0 ? 0 : 1;
    for (int64_t n = centre - demi; n <= centre + demi; n++) {
        Paroi& p = v[(size_t)(((n % L) + L) % L)][(size_t)c];
        if (p.d <= 0 || d < p.d) p = {d, h, alpha};   // la plus proche de l'axe gagne (elle cache les autres)
    }
}

const std::vector<Cotes>& lire(const std::string& carte) {
    static const std::vector<Cotes> vide;
    auto& r = registre();
    auto it = r.find(carte);
    return it == r.end() ? vide : it->second.parois;
}

const std::vector<Immeuble>& obstacles(const std::string& carte) {
    static const std::vector<Immeuble> vide;
    auto& r = registre();
    auto it = r.find(carte);
    return it == r.end() ? vide : it->second.obstacles;
}

// WFS-278 (RG-02) : lieux du tracé 2 (WFS-176), distances et hauteurs de leur dessin (ville.cpp, lieuVille) : casino à RW + 12
// (pierre, 14 m et corniche, 24 m de profondeur), centre commercial à RW + 31 (mur-rideau, 12 m, 26 m), garage à RW + 6,5 (6 m, 12,5 m)
struct FicheLieu { float d, h, prof, alpha; };
FicheLieu ficheLieu(int t) {
    const float R = (float)RW;
    switch (t) {
    case ville::CASINO: return {R + 12.f, 14.5f, 24.f, ALPHA_PIERRE};
    case ville::CENTRE_COMMERCIAL: return {R + 31.f, 12.f, 26.f, ALPHA_VERRE};
    default: return {R + 6.5f, 6.f, 12.5f, ALPHA_DUR};   // garage
    }
}

void depuisVille(const ville::Plan& p, int64_t L) {
    effacer("ville");
    for (int64_t a = 0; a < (int64_t)p.objets.size(); a++)
        for (const ville::Objet& o : p.objets[(size_t)a]) {
            if (o.t != ville::IMMEUBLE && o.t != ville::ENTREPOT) continue;
            const float d = o.t == ville::IMMEUBLE ? (float)RW + SET_BACK_VILLE : o.lat;   // façade côté route
            declarer("ville", L, a + (int64_t)std::lround(o.len / SEG_M / 2), o.side, d, o.h, o.len);
        }
    for (const ville::Lieu& l : p.lieux) {   // WFS-278 (RG-02)
        const FicheLieu f = ficheLieu(l.t);
        declarer("ville", L, l.seg + (int64_t)std::lround(l.len / 2 / SEG_M), l.side, f.d, f.h, l.len, 0.f, f.alpha);
    }
}

// ---------------------------------------------------------------- RG-03
int reflexions(float a, float b, Reflexion out[6], float alphaG, float alphaD) {
    int n = 0;
    const float rG = std::sqrt(1.f - alphaG), rD = std::sqrt(1.f - alphaD);   // amplitude réfléchie par chaque paroi
    auto ajouter = [&](float chemin, int cote, int ordre, float pertes) {
        if (chemin <= 0 || n >= 6) return;
        out[n++] = {chemin / lieux::VITESSE_SON, std::min(1.f, R0 / chemin) * pertes, cote, ordre};
    };
    if (a > 0) ajouter(2 * a, -1, 1, rG);
    if (b > 0) ajouter(2 * b, 1, 1, rD);
    if (a > 0 && b > 0 && a + b <= PORTEE) {   // rue bordée des deux côtés : sources images d'ordre 2 et 3
        const float W = a + b;
        ajouter(2 * W, 1, 2, rG * rD);             // gauche puis droite : arrive de droite
        ajouter(2 * W, -1, 2, rG * rD);            // droite puis gauche : arrive de gauche
        ajouter(2 * W + 2 * a, -1, 3, rG * rD * rG);   // gauche, droite, gauche
        ajouter(2 * W + 2 * b, 1, 3, rD * rG * rD);    // droite, gauche, droite
    }
    return n;
}

int Suivi::pas(float dt, float a, float b, bool tunnel, Reflexion r[6], float gG[6], float gD[6], float alphaG, float alphaD) {
    const float k = dt / FONDU_TROUEE;   // RG-05 : un côté s'ouvre ou se ferme en 75 ms
    pG_ = std::clamp(pG_ + (a > 0 ? k : -k), 0.f, 1.f); pD_ = std::clamp(pD_ + (b > 0 ? k : -k), 0.f, 1.f);
    if (a > 0) { aG_ = a; alG_ = alphaG; }
    if (b > 0) { aD_ = b; alD_ = alphaD; }   // trouée : l'écho s'éteint à la dernière distance connue
    fT_ = std::clamp(fT_ + (tunnel ? -1.f : 1.f) * dt / lieux::FONDU_TUNNEL, 0.f, 1.f);
    const int n = reflexions(pG_ > 0 ? aG_ : 0, pD_ > 0 ? aD_ : 0, r, alG_, alD_);
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
    for (const ville::Lieu& l : p.lieux) {   // WFS-278 (RG-02) : le casino, le centre commercial et les garages masquent aussi
        const FicheLieu f = ficheLieu(l.t);
        const double sc = (double)l.seg + l.len / 2 / SEG_M, sf = std::floor(sc);   // lieuVille : poseS(a + demi / SEG_M)
        Immeuble im;
        im.centre.seg = L > 0 ? (((int64_t)sf % L) + L) % L : (int64_t)sf;
        im.centre.avant = (float)((sc - sf) * SEG_M);
        im.centre.lat = (float)l.side * (f.d + f.prof / 2);
        im.demiProf = f.prof / 2; im.demiLong = l.len / 2; im.h = f.h;
        out.push_back(im);
    }
    return out;
}

// partie [t0, t1] du segment a → b (plan horizontal) dans la boîte de centre c, d'axes r (demi-taille hp) et f (hl) ; Liang–Barsky
bool traverser(vec3 a, vec3 b, vec3 c, vec3 r, vec3 f, float hp, float hl, float& t0, float& t1) {
    auto loc = [&](vec3 p) { const vec3 d = p - c; return P2{dot(d, r), dot(d, f)}; };
    const P2 A = loc(a), B = loc(b);
    t0 = 0; t1 = 1;
    const float dx = B.x - A.x, dy = B.y - A.y;
    auto couper = [&](float p, float q) {   // p · t ≤ q
        if (std::abs(p) < 1e-9f) return q >= 0;
        const float t = q / p;
        if (p < 0) { if (t > t1) return false; t0 = std::max(t0, t); }
        else { if (t < t0) return false; t1 = std::min(t1, t); }
        return true;
    };
    if (!couper(-dx, A.x + hp) || !couper(dx, hp - A.x) || !couper(-dy, A.y + hl) || !couper(dy, hl - A.y) || t0 >= t1) return false;
    return t0 > 0 && t1 < 1;   // une extrémité dans la boîte : pas de masque (cas hors du décor)
}

float detour(vec3 a, vec3 b, vec3 c, vec3 r, vec3 f, float hp, float hl, float h) {
    auto loc = [&](vec3 p) { const vec3 d = p - c; return P2{dot(d, r), dot(d, f)}; };
    const P2 A = loc(a), B = loc(b);
    float t0, t1;
    if (!traverser(a, b, c, r, f, hp, hl, t0, t1)) return -1;
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

float diffractionMultipleDb(float z, float e) {
    if (z < 0) return 0.f;
    float C3 = 1;   // ISO 9613-2 éq. 14 (CNOSSOS-EU, getDz) : C3 = (1 + (5λ/e)²) / (1/3 + (5λ/e)²), 1 pour une seule arête
    if (e > 0) { const float x = 5 * LAMBDA_1K / e; C3 = (1 + x * x) / (1.f / 3 + x * x); }
    const float N = 2 * z / LAMBDA_1K;
    return std::min(MASQUE_MAX_MULTIPLE, 10.f * std::log10(3.f + 20.f * N * C3));
}

namespace {
constexpr int GROUPE_MAX = 32;                 // obstacles accolés suivis au plus (au-delà : on ne peut pas en faire le tour)
constexpr int PTS_MAX = 2 + 4 * GROUPE_MAX + 1;
constexpr float ACCOLE = 0.5f;                 // m : deux boîtes à moins de 2 × 0,5 m l'une de l'autre se touchent (mur continu)
// chemin tendu de p[0] à p[n−1] (enveloppe convexe des points, du côté le plus court) ; z : détour, e : longueur du chemin entre la
// première et la dernière arête (points d'indice 1 à n − 2) ; plan (x, y) quelconque
struct PtE { P2 p; int id; };
bool chemin(PtE* pts, int n, float& z, float& e) {
    std::sort(pts, pts + n, [](const PtE& u, const PtE& v) { return u.p.x < v.p.x || (u.p.x == v.p.x && u.p.y < v.p.y); });
    PtE h[2 * PTS_MAX + 2]; int k = 0;   // enveloppe : au plus 2n points (chaîne monotone d'Andrew) ; n ≤ PTS_MAX
    for (int i = 0; i < n; i++) { while (k >= 2 && croix(h[k - 2].p, h[k - 1].p, pts[i].p) <= 0) k--; h[k++] = pts[i]; }
    for (int i = n - 2, t = k + 1; i >= 0; i--) { while (k >= t && croix(h[k - 2].p, h[k - 1].p, pts[i].p) <= 0) k--; h[k++] = pts[i]; }
    const int m = k - 1;
    int ia = -1, ib = -1;
    for (int i = 0; i < m; i++) { if (h[i].id == 0) ia = i; if (h[i].id == 1) ib = i; }
    if (ia < 0 || ib < 0) return false;
    float best = 1e30f, bestE = 0;
    for (int sens = 0; sens < 2; sens++) {   // de a à b dans un sens puis dans l'autre
        const int i0 = sens ? ib : ia, i1 = sens ? ia : ib;
        float L = 0, premier = -1, dernier = -1;
        for (int i = i0; i != i1; i = (i + 1) % m) {
            if (h[i].id >= 2) { if (premier < 0) premier = L; dernier = L; }
            L += lon(h[i].p, h[(i + 1) % m].p);
        }
        if (L < best) { best = L; bestE = premier >= 0 ? dernier - premier : 0; }
    }
    z = best - lon(h[ia].p, h[ib].p);
    e = bestE;
    return true;
}
} // namespace

namespace {
// boîtes qui se touchent dans le plan horizontal (axes séparateurs des deux rectangles, agrandis de ACCOLE)
bool accoles(const ImmeubleMonde& u, const ImmeubleMonde& v) {
    const vec3 d{v.c.x - u.c.x, 0, v.c.z - u.c.z};
    const float rmax = std::sqrt(u.demiProf * u.demiProf + u.demiLong * u.demiLong) + std::sqrt(v.demiProf * v.demiProf + v.demiLong * v.demiLong) + 2 * ACCOLE;
    if (d.x * d.x + d.z * d.z > rmax * rmax) return false;   // cercles englobants disjoints : test rapide
    auto proj = [](const ImmeubleMonde& b, vec3 ax) { return std::abs(dot(b.r, ax)) * (b.demiProf + ACCOLE) + std::abs(dot(b.f, ax)) * (b.demiLong + ACCOLE); };
    for (const vec3& ax : {u.r, u.f, v.r, v.f})
        if (std::abs(dot(d, ax)) > proj(u, ax) + proj(v, ax)) return false;
    return true;
}
} // namespace

float masqueDb(vec3 a, vec3 s, const std::vector<ImmeubleMonde>& proches) {
    struct Coupe { int i; float t0, t1; };
    Coupe coupes[16]; int nc = 0;
    float m = 0;
    for (size_t i = 0; i < proches.size(); i++) {
        const ImmeubleMonde& im = proches[i];
        float t0, t1;
        if (!traverser(a, s, im.c, im.r, im.f, im.demiProf, im.demiLong, t0, t1)) continue;
        const float dl = detour(a, s, im.c, im.r, im.f, im.demiProf, im.demiLong, im.h);
        if (dl < 0) continue;   // ligne au-dessus du toit
        m = std::max(m, diffractionDb(dl));
        if (nc < 16) coupes[nc++] = {(int)i, t0, t1};
    }
    if (nc == 0) return m;
    // WFS-278 : groupe des obstacles traversés et de ceux qui les touchent, de proche en proche (morceaux d'une même roche, garages
    // côte à côte, immeubles mitoyens) : le chemin « autour » fait le tour du groupe, pas d'un morceau seul (sinon il passerait à
    // travers le voisin)
    int groupe[GROUPE_MAX]; int ng = 0; bool tropGrand = false;
    uint8_t vu[256] = {};
    const bool suivi = proches.size() <= 256;
    for (int i = 0; i < nc; i++) { groupe[ng++] = coupes[i].i; if (suivi) vu[coupes[i].i] = 1; }
    for (int g = 0; suivi && g < ng && !tropGrand; g++)
        for (size_t j = 0; j < proches.size(); j++) {
            if (vu[j] || !accoles(proches[(size_t)groupe[g]], proches[j])) continue;
            if (ng == GROUPE_MAX) { tropGrand = true; break; }
            groupe[ng++] = (int)j; vu[j] = 1;
        }
    if (nc == 1 && ng == 1) return m;   // obstacle seul et isolé : diffraction simple (WFS-275, à l'identique)
    // RG-04 : chemin tendu par-dessus les obstacles traversés (profil : distance le long de a → s, hauteur) …
    const float L2 = std::hypot(s.x - a.x, s.z - a.z);
    PtE pts[PTS_MAX]; int n = 0;
    float zT = 1e30f, eT = 0, zC = 1e30f, eC = 0;
    pts[n++] = {{0, a.y}, 0}; pts[n++] = {{L2, s.y}, 1};
    for (int i = 0; i < nc; i++) {
        const ImmeubleMonde& im = proches[(size_t)coupes[i].i];
        pts[n++] = {{coupes[i].t0 * L2, im.c.y + im.h}, 2}; pts[n++] = {{coupes[i].t1 * L2, im.c.y + im.h}, 2};
    }
    { // enveloppe du dessus seulement : un point très bas, sous la ligne, ferme l'enveloppe par en dessous
        pts[n++] = {{L2 / 2, std::min(a.y, s.y) - 1e4f}, -1};
        float z, e; if (chemin(pts, n, z, e)) { zT = z; eT = e; }
    }
    // … ou autour du groupe (plan horizontal : enveloppe de a, s et des coins des obstacles du groupe) ; groupe trop grand : pas de tour
    if (!tropGrand) {
        n = 0;
        pts[n++] = {{a.x, a.z}, 0}; pts[n++] = {{s.x, s.z}, 1};
        for (int g = 0; g < ng; g++) {
            const ImmeubleMonde& im = proches[(size_t)groupe[g]];
            for (float u : {-1.f, 1.f}) for (float v : {-1.f, 1.f}) {
                const vec3 q = im.c + im.r * (u * im.demiProf) + im.f * (v * im.demiLong);
                pts[n++] = {{q.x, q.z}, 2};
            }
        }
        float z, e; if (chemin(pts, n, z, e)) { zC = z; eC = e; }
    }
    if (zT > 1e29f && zC > 1e29f) return m;   // aucun chemin tendu trouvé : diffraction simple (comme WFS-275)
    // plusieurs arêtes (par-dessus : entrée et sortie de la roche ou des toits, e = épaisseur traversée ; autour : coins du groupe) ;
    // un mur coupé en morceaux donne ainsi le même masque qu'il soit traversé sur un morceau ou sur deux
    const bool dessus = zT <= zC;
    return std::max(m, diffractionMultipleDb(dessus ? zT : zC, dessus ? eT : eC));
}

} // namespace wfs::parois
