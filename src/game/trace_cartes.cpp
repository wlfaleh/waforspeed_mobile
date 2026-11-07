// WaForSpeed — tracé des cartes, 2e version (ticket WFS-136, épic E3) : voir trace_cartes.h.
// Le jeu web n'a ni rond-point, ni feu, ni autoroute : seule la construction de la route (addRoad, CURVE_K, ELEV de
// js/world/track.js) est reprise. Plans des 4 cartes : RG-06 de regles-de-gestion/WFS-136.md.
#include "trace_cartes.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <tuple>
#include <vector>
#include <numbers>
#include <string>
#include "track.h"
#include "../core/log.h"

namespace wfs::trace {

namespace {
constexpr double PI = std::numbers::pi;
constexpr double M_PAR_Y = SEG * M * ELEV;   // altitude (m) d'une unité « y » de addRoad : 0,6 m
constexpr double KMH = 1 / 3.6 / M;          // km/h → unités/s

// somme des courbures relatives d'une entrée (easeIn 0 → 1) et d'une sortie (easeInOut 1 → 0) de addRoad : le cap tourné
// par addRoad(e, h, l, c) vaut CURVE_K × c × (entree(e) + h + sortie(l))
double entree(int e) { double s = 0; for (int n = 0; n < e; n++) { const double p = (double)n / e; s += p * p; } return s; }
double sortie(int l) { double s = 0; for (int n = 0; n < l; n++) s += 0.5 + 0.5 * std::cos(PI * n / l); return s; }
int segs(double m) { return std::max(1, (int)std::lround(m / SEG_M)); }

// plan d'une carte : chaque élément ajoute ses morceaux de route (addRoad) et sa section
struct Plan {
    Track& t;
    std::vector<Section> out;
    int feux = 0;
    explicit Plan(Track& tr) : t(tr) {}
    int64_t ici() const { return (int64_t)t.segmentCount(); }
    Section& ouvrir(Type ty, int64_t debut) { out.push_back({}); Section& s = out.back(); s.type = ty; s.debut = debut; return s; }
    void fermer(Section& s, double dh) { s.fin = ici(); s.denivele = dh; }

    // morceau de route droit (dh : montée en m, + vers le haut)
    void droit(double m, double dh) { t.ajouterRoute(0, segs(m), 0, 0, dh / M_PAR_Y); }
    // morceau courbe : angle en degrés (+ à droite), rayon en m ; transitions e / l en segments
    static int tenue(double angle, double rayon, int e, int l) {   // segments à courbure pleine
        const double cible = std::abs(angle) * PI / 180 * rayon / SEG_M;   // entree(e) + h + sortie(l) (en segments)
        return std::max(0, (int)std::lround(cible - entree(e) - sortie(l)));
    }
    static double longueur(double angle, double rayon, int e, int l) { return (e + tenue(angle, rayon, e, l) + l) * SEG_M; }
    void courbe(double angle, double rayon, double dh, int e, int l) {
        const double c = (angle >= 0 ? 1 : -1) * SEG_M / (rayon * CURVE_K);
        t.ajouterRoute(e, tenue(angle, rayon, e, l), l, c, dh / M_PAR_Y);
    }
    static int transition(double rayon) { return rayon < 30 ? 4 : rayon < 60 ? 6 : 20; }
    // cap tourné (rad) par courbe(angle, rayon, ·, e, l), au même arrondi que Track (somme des courbures × CURVE_K)
    static double capCourbe(double angle, double rayon, int e, int l) { return (angle >= 0 ? 1 : -1) * SEG_M / rayon * (entree(e) + tenue(angle, rayon, e, l) + sortie(l)); }

    void ligne(double m, double dh = 0, Type ty = Type::Ligne, bool autoroute = false) {
        if (dh != 0 && ty == Type::Ligne) ty = dh > 0 ? Type::Montee : Type::Descente;
        Section& s = ouvrir(ty, ici()); s.autoroute = autoroute;
        droit(m, dh); fermer(out.back(), dh);
    }
    void virage(double angle, double rayon, double dh = 0, Type ty = Type::Virage, bool autoroute = false) {
        const int64_t d = ici();
        courbe(angle, rayon, dh, transition(rayon), transition(rayon));
        Section& s = ouvrir(ty, d); s.cote = angle >= 0 ? 1 : -1; s.rayon = rayon; s.angle = angle; s.autoroute = autoroute;
        fermer(s, dh);
    }
    // épingle (RG-01) : ≥ 150° sur ≤ 80 m (rayon 20 à 24 m)
    void epingle(double angle, double rayon, double dh = 0) { virage(angle, rayon, dh, Type::Epingle); }
    // lacets (RG-05) : n épingles alternées (165°, 22 m), séparées de branches de « jambe » m, montée totale dh répartie
    // à la longueur (pente moyenne constante)
    void lacets(int n, double jambe, double dh, int premierCote) {
        const double longEp = longueur(165, 22, 4, 4), total = n * (segs(jambe) * SEG_M + longEp);
        const int64_t d = ici();
        for (int i = 0; i < n; i++) {
            const int cote = (i % 2 == 0) ? premierCote : -premierCote;
            droit(jambe, dh * segs(jambe) * SEG_M / total);
            courbe(cote * 165.0, 22, dh * longEp / total, 4, 4);
        }
        Section& s = ouvrir(Type::Lacets, d); s.cote = premierCote; s.rayon = 22; s.angle = 165; s.branches = n;
        fermer(s, dh);
    }
    // rond-point (RG-02) : entrée à droite (90°, 25 m), ¾ de tour à gauche autour d'un îlot de 14 m, sortie à droite :
    // cap de sortie = cap d'entrée − 90° ; 2 amorces de branches pour le décor
    void rondPoint() {
        if (!rondsPoints()) { carrefourSimple(); return; }   // WFS-283
        const int64_t d = ici();
        courbe(90, 25, 0, 6, 6);
        courbe(-270, 14, 0, 4, 4);
        courbe(90, 25, 0, 6, 6);
        Section& s = ouvrir(Type::RondPoint, d); s.cote = 1; s.rayon = 14; s.angle = -90; s.branches = 2;
        fermer(s, 0);
    }
    // WFS-283 (RG-02) : carrefour simple à la place du rond-point : virage de rue à gauche de RAYON_CARREFOUR_M (comme les virages
    // serrés de la Ville), au cap net EXACT du rond-point (−90,28°, mêmes courbes) ; le rond-point tourne presque sur place
    // (déplacement net < 0,1 m), ce virage avance et décale : les droites voisines en sont raccourcies (raccourcis(), en
    // segments), la suite de la carte garde sa place à moins d'un demi-segment près (test_trace)
    static constexpr double RAYON_CARREFOUR_M = 22;
    static double capRondPoint() { return capCourbe(90, 25, 6, 6) + capCourbe(-270, 14, 4, 4) + capCourbe(90, 25, 6, 6); }
    static int tenueCarrefour() { return tenue(capRondPoint() * 180 / PI, RAYON_CARREFOUR_M, 4, 4); }
    static double courbureCarrefour() { return capRondPoint() / (CURVE_K * (entree(4) + tenueCarrefour() + sortie(4))); }
    void carrefourSimple() {
        const int64_t d = ici();
        t.ajouterRoute(4, tenueCarrefour(), 4, courbureCarrefour(), 0);
        Section& s = ouvrir(Type::Virage, d); s.cote = -1; s.rayon = RAYON_CARREFOUR_M; s.angle = capRondPoint() * 180 / PI; s.exRondPoint = true;
        fermer(s, 0);
    }
    // segments à retirer à la droite d'avant (avant) et à celle d'après (apres) le carrefour simple : déplacement du virage
    // (même intégration que Track::pose) moins celui du rond-point, dans le repère de l'entrée (avant) et de la sortie (après)
    struct Raccourcis { int avant = 0, apres = 0; };
    static Raccourcis raccourcis() {
        if (rondsPoints()) return {};
        auto deplacement = [](const std::vector<double>& courbes, double& x, double& z, double& th) {
            x = 0; z = 0; th = 0;
            for (double c : courbes) { x += std::sin(th) * SEG_M; z -= std::cos(th) * SEG_M; th += c * CURVE_K; }
        };
        auto courbes = [](int e, int h, int l, double c, std::vector<double>& v) {   // Track::addRoad (easeIn, puis easeInOut)
            for (int n = 0; n < e; n++) { const double p = (double)n / e; v.push_back(c * p * p); }
            for (int n = 0; n < h; n++) v.push_back(c);
            for (int n = 0; n < l; n++) v.push_back(c + (0 - c) * (0.5 - 0.5 * std::cos(PI * n / l)));
        };
        std::vector<double> rp, cs;
        for (const auto& [a, r, e] : {std::tuple{90.0, 25.0, 6}, {-270.0, 14.0, 4}, {90.0, 25.0, 6}})
            courbes(e, tenue(a, r, e, e), e, (a >= 0 ? 1 : -1) * SEG_M / (r * CURVE_K), rp);
        courbes(4, tenueCarrefour(), 4, courbureCarrefour(), cs);
        double xr, zr, thr, xc, zc, thc; deplacement(rp, xr, zr, thr); deplacement(cs, xc, zc, thc);
        // écart (x, z) = A · f + B · h, f = direction d'entrée (0, −1), h = direction de sortie (sin thc, −cos thc) : A segments
        // à retirer avant, B après (règle de Cramer)
        const double dx = xc - xr, dz = zc - zr, hx = std::sin(thc), hz = -std::cos(thc), det = 0 * hz - (-1) * hx;
        const double A = (dx * hz - dz * hx) / det, B = (0 * dz - (-1) * dx) / det;
        return {(int)std::lround(A / SEG_M), (int)std::lround(B / SEG_M)};
    }
    double avantCarrefour() const { return raccourcis().avant * SEG_M; }   // m retirés à la droite qui précède le rond-point
    double apresCarrefour() const { return raccourcis().apres * SEG_M; }   // m retirés à celle qui le suit
    // carrefour à feux (RG-03) : ligne droite, ligne d'arrêt au milieu, passage des voitures 8 m plus loin
    void carrefour(double m = 120) {
        const int64_t d = ici();
        droit(m, 0);
        Section& s = ouvrir(Type::Feux, d); s.feu = feux++; s.ligneArret = d + segs(m / 2 - CARREFOUR_M / 2);
        fermer(s, 0);
    }
    void feuxDepart(double m) {
        const int64_t d = ici();
        droit(m, 0);
        Section& s = ouvrir(Type::Feux, d); s.feu = feux++; s.depart = true; s.ligneArret = d;
        fermer(s, 0);
    }
    // chicane : deux virages opposés rapprochés (40°, 35 m)
    void chicane(int cote) {
        const int64_t d = ici();
        courbe(cote * 40.0, 35, 0, 6, 6); courbe(-cote * 40.0, 35, 0, 6, 6);
        Section& s = ouvrir(Type::Chicane, d); s.cote = cote; s.rayon = 35; s.angle = 0;
        fermer(s, 0);
    }
    // pont : légère bosse (+2 m puis −2 m) sur la ligne droite
    void pont(double m, bool autoroute = false) {
        const int64_t d = ici();
        droit(m / 2, 2); droit(m / 2, -2);
        Section& s = ouvrir(Type::Pont, d); s.autoroute = autoroute;
        fermer(s, 0);
    }
};

// ---------- plans des 4 cartes (RG-06) ----------
void ville(Plan& p) {   // grande ville côtière en damier
    p.ligne(200);
    p.virage(88, 22);                    // 1er virage serré presque à angle droit
    p.ligne(350);
    p.carrefour();                       // feu n° 0
    p.ligne(300);
    p.virage(-88, 22);                   // 2e virage serré
    p.ligne(250 - p.avantCarrefour());   // WFS-283 : raccourcie pour le carrefour simple
    p.rondPoint();                       // rond-point à fontaine (WFS-283 : carrefour simple)
    p.ligne(300 - p.apresCarrefour());
    p.virage(30, 150);
    p.ligne(200);
    p.pont(250);
    p.ligne(150);
    // autoroute urbaine de 1,5 km, grandes courbes (rayon ≥ 600 m) et tunnel de 300 m
    p.virage(20, 800, 0, Type::Autoroute, true);
    p.ligne(300, 0, Type::Autoroute, true);
    p.ligne(300, 0, Type::Tunnel, true);
    p.ligne(200, 0, Type::Autoroute, true);
    p.virage(-25, 700, 0, Type::Autoroute, true);
    p.ligne(120, 0, Type::Autoroute, true);
    p.ligne(250);
    p.carrefour();                       // feu n° 1
    p.ligne(300);
    p.virage(88, 24);                    // 3e virage serré
    p.ligne(400);
    p.virage(-45, 120);
    p.ligne(300);
    p.virage(40, 200);
    p.ligne(300);
    p.virage(-60, 150);
    p.ligne(300);
    p.virage(35, 250);
    p.ligne(250);
}

void corniche(Plan& p) {   // côte méditerranéenne : falaise, lacets, tunnel dans la roche
    p.ligne(200);
    p.virage(-30, 200);
    p.ligne(250 - p.avantCarrefour());   // WFS-283 : raccourcie pour le carrefour simple
    p.rondPoint();                       // entrée de la station (WFS-283 : carrefour simple)
    p.ligne(200 - p.apresCarrefour());
    p.carrefour();                       // front de mer
    p.ligne(300);
    p.virage(45, 150);
    p.ligne(150);
    p.lacets(6, 120, 60, 1);             // 6 lacets au-dessus de la mer, +60 m
    p.ligne(150);
    p.ligne(200, 0, Type::Tunnel);       // tunnel de 200 m dans la roche
    p.virage(60, 180, -11);              // descente de 60 m à pente moyenne constante (≈ 5,3 %)
    p.ligne(400, -21);
    p.virage(-50, 200, -10);
    p.ligne(350, -18);
    p.virage(-40, 250);
    p.ligne(400);
    p.virage(30, 300);
    p.ligne(270);
}

void campagne(Plan& p) {   // vallée et col de montagne
    p.ligne(200);
    p.virage(35, 200);
    p.ligne(200);
    p.pont(150);                         // pont sur la rivière
    p.ligne(200 - p.avantCarrefour());   // WFS-283 : raccourcie pour le carrefour simple
    p.rondPoint();                       // rond-point du village (WFS-283 : carrefour simple)
    p.ligne(150 - p.apresCarrefour());
    p.carrefour();                       // feu du village
    p.ligne(300);
    p.lacets(4, 470, 120, -1);           // col de 120 m, 4 épingles
    p.ligne(150);
    p.virage(-50, 250, -12);
    p.ligne(300, -13);
    // autoroute de 2 km (aire de péage au début, pour le décor), en descente douce (−95 m)
    p.ligne(400, -19, Type::Autoroute, true);
    p.virage(25, 900, -19, Type::Autoroute, true);
    p.ligne(500, -24, Type::Autoroute, true);
    p.virage(-20, 800, -13, Type::Autoroute, true);
    p.ligne(430, -20, Type::Autoroute, true);
    p.ligne(200);
}

void circuit(Plan& p) {   // Grand Prix : circuit fermé, sans feu de circulation ni rond-point
    p.feuxDepart(60);                    // feux de départ : rouges au décompte, éteints au « GO »
    p.ligne(940);                        // ligne droite de 1 km avec la grille
    p.epingle(170, 20);
    p.ligne(300);
    p.chicane(1);
    p.ligne(400);
    p.virage(-90, 60);
    p.ligne(300);
    p.virage(60, 420, 25, Type::Montee); // raidillon : +25 m en courbe
    p.ligne(300);
    p.ligne(450, -25);
    p.chicane(-1);
    p.ligne(400);
    p.virage(120, 80);
    p.ligne(300);
    p.virage(-70, 100);
    p.ligne(400);
    p.virage(100, 70);
    p.ligne(350);
    p.virage(-50, 150);
    p.ligne(300);
    p.virage(60, 120);
    p.ligne(100);
}

// carte en cours (système des feux) : piste et horloge du trafic
const Track* g_piste = nullptr;
double g_horloge = -3;
} // namespace

namespace { bool g_forcer = false; int g_rondsPoints = -1; }   // WFS-283 : −1 = selon l'environnement
bool rondsPoints() {
    if (g_rondsPoints >= 0) return g_rondsPoints == 1;
    const char* e = std::getenv("WFS_RONDS_POINTS"); const char* b = std::getenv("WFS_BANC");
    return (e && !std::strcmp(e, "1")) || (b && !std::strcmp(b, "conduite"));
}
void forcerRondsPoints(bool oui) { g_rondsPoints = oui ? 1 : 0; }
bool carteActive(const char* id) {
    for (const char* c : CARTES_ACTIVES) if (id && std::strcmp(c, id) == 0) return true;
    return false;
}
int versionCarte(const char* id) { return g_forcer || carteActive(id) ? 2 : 1; }
void forcerTrace2(bool oui) { g_forcer = oui; }
std::string signature() {
    std::string s;
    for (const char* c : CARTES_ACTIVES) { if (!s.empty()) s += ','; s += c; }
    if (!s.empty() && !rondsPoints()) s += ",sans-rond-point";   // WFS-283 : tracé sans rond-point (un APK d'avant n'a pas la même route) ; jamais de « ; » (séparateur des trames)
    return s.empty() ? "1" : s;
}

const char* nomType(Type t) {
    static const char* const N[] = {"ligne", "virage", "epingle", "lacets", "montee", "descente", "rond-point", "feux",
                                    "autoroute", "tunnel", "pont", "chicane"};
    return N[(int)t];
}
double Section::debutU() const { return debut * SEG; }
double Section::finU() const { return fin * SEG; }

std::vector<Section> construire(Track& t, const MapDef& map) {
    Plan p(t);
    if (std::strcmp(map.id, "plage") == 0) corniche(p);
    else if (std::strcmp(map.id, "campagne") == 0) campagne(p);
    else if (std::strcmp(map.id, "circuit") == 0) circuit(p);
    else ville(p);
    // retour à l'altitude 0 (la route reboucle sur elle-même au tour suivant) : addRoad s'arrête un peu avant l'altitude
    // visée (dernier pas de l'interpolation) ; quelques mètres de correction, jusqu'à moins d'1 mm
    for (int i = 0; i < 4 && std::abs(t.altitudeFin() * M * ELEV) > 1e-3; i++) {
        const double dh = -t.altitudeFin() * M * ELEV;
        if (!p.out.empty() && p.out.back().type == Type::Ligne && p.out.back().denivele == 0) { p.droit(std::max(40.0, std::abs(dh) / 0.04), dh); p.out.back().fin = p.ici(); }
        else p.ligne(std::max(40.0, std::abs(dh) / 0.04), dh);
    }
    WFS_LOG("tracé 2 : %s, %zu sections, %zu segments (%.0f m), %d carrefour(s) à feux", map.id, p.out.size(),
            t.segmentCount(), t.segmentCount() * SEG_M, p.feux);
    return std::move(p.out);
}

const std::vector<Section>& sections(const Track& t) { return t.sections(); }

const Section* sectionA(const Track& t, double z) {
    const auto& S = t.sections();
    if (S.empty() || t.length() <= 0) return nullptr;
    const int64_t a = (int64_t)std::floor(t.wrap(z) / SEG);
    // recherche dichotomique : sections triées et contiguës
    auto it = std::upper_bound(S.begin(), S.end(), a, [](int64_t v, const Section& s) { return v < s.debut; });
    if (it == S.begin()) return &S.front();
    --it;
    return a < it->fin ? &*it : &S.back();
}

Rayon rayonInterieur(const Track& t, double s) {
    Rayon r;
    if (t.sections().empty() || t.segmentCount() == 0) return r;   // tracé 1 : rien ne change (décor du jeu web)
    // le décor à « lat » mètres du côté intérieur retombe sur la route quand lat dépasse le rayon : on regarde les
    // courbes à ±6 m autour de s (un morceau du bas-côté est tiré entre deux lignes de segments) ; plus large, le côté
    // extérieur d'un anneau serait coupé à cause des courbes d'entrée et de sortie, de l'autre sens
    const int64_t a = (int64_t)std::floor(s / SEG), w = (int64_t)std::ceil(6 / SEG_M);
    double cD = 0, cG = 0;   // courbure > 0 : la route tourne à droite, intérieur à droite
    for (int64_t i = a - w; i <= a + w; i++) {
        const double c = t.segAt(i).curve;
        if (c > cD) cD = c;
        if (-c > cG) cG = -c;
    }
    if (cD > 1e-9) r.droite = SEG_M / (cD * CURVE_K);
    if (cG > 1e-9) r.gauche = SEG_M / (cG * CURVE_K);
    r.m = std::min(r.droite, r.gauche); r.cote = r.m == std::numeric_limits<double>::infinity() ? 0 : r.droite <= r.gauche ? 1 : -1;
    return r;
}

Feu feuCarrefour(int i, double horloge, double* tempsDansPhase) {
    double ph = std::fmod(horloge + i * FEU_DECALAGE, FEU_CYCLE);
    if (ph < 0) ph += FEU_CYCLE;
    Feu f = Feu::Vert;
    if (ph >= FEU_VERT + FEU_ORANGE) { f = Feu::Rouge; ph -= FEU_VERT + FEU_ORANGE; }
    else if (ph >= FEU_VERT) { f = Feu::Orange; ph -= FEU_VERT; }
    if (tempsDansPhase) *tempsDansPhase = ph;
    return f;
}

int nombreFeux(const Track& t) {
    int n = 0;
    for (const Section& s : t.sections()) if (s.feu >= 0) n++;
    return n;
}

Feu feuA(int i) {
    if (!g_piste) return Feu::Eteint;
    for (const Section& s : g_piste->sections()) {
        if (s.feu != i) continue;
        if (s.depart) return g_horloge < 0 ? Feu::Rouge : Feu::Eteint;   // feux de départ du Grand Prix
        return feuCarrefour(i, g_horloge);
    }
    return Feu::Eteint;
}

double vitesseTrafic(const Track& t, double z) {
    const Section* s = sectionA(t, z);
    if (!s) return std::numeric_limits<double>::infinity();
    if (s->type == Type::RondPoint) return 40 * KMH;
    if (s->type == Type::Epingle || s->type == Type::Lacets) {
        if (std::abs(t.findSegment(t.wrap(z)).curve) * CURVE_K > SEG_M / 45) return 60 * KMH;   // courbe de moins de 45 m
    }
    return std::numeric_limits<double>::infinity();
}

double facteurTrafic(const Track& t, double z) {
    const Section* s = sectionA(t, z);
    return s && s->autoroute ? 1.15 : 1.0;
}

double longueurAutoroute(const Track& t) {
    double L = 0;
    for (const Section& s : t.sections()) if (s.autoroute) L += s.finU() - s.debutU();
    return L;
}

// carte en cours (posée par le système des feux, trace_cartes_systeme.cpp)
void poserCarte(const Track* piste, double horloge) { g_piste = piste; g_horloge = horloge; }

} // namespace wfs::trace
