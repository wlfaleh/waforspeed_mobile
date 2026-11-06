// WaForSpeed — mode « Poursuite police » (WFS-111) : règles, police et programme de la course. Voir poursuite.h.
#include "poursuite.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "../core/math.h"

namespace wfs {
namespace poursuite {

void perdre(Etat& e, int points) {
    if (e.arrete || e.gagne) return;
    e.score = std::max(0, e.score - points);
    if (e.score == 0) e.arrete = true;   // RG-01 : défaite immédiate, écran « ARRÊTÉ »
}
void chocTrafic(Etat& e) { if (e.cdTrafic > 0) return; e.cdTrafic = DELAI_CHOC; e.chocsTrafic++; perdre(e, CHOC_TRAFIC); }
void chocGlissiere(Etat& e) { if (e.cdGlissiere > 0) return; e.cdGlissiere = DELAI_CHOC; e.chocsGlissiere++; perdre(e, CHOC_GLISSIERE); }
void chocPolice(Etat& e) { e.chocsPolice++; perdre(e, CHOC_POLICE); }
void quasiContact(Etat& e) { if (e.arrete || e.gagne) return; e.quasi++; e.score = std::min(SCORE_DEPART, e.score + 1); }

void pas(Etat& e, double dt, double dPoliceM) {
    e.cdTrafic = std::max(0.0, e.cdTrafic - dt); e.cdGlissiere = std::max(0.0, e.cdGlissiere - dt);
    if (e.arrete || e.gagne) return;
    if (dPoliceM >= COLLE_M) { e.colle = 0; return; }
    e.colle += dt;
    while (e.colle >= COLLE_S && !e.arrete) { e.colle -= COLLE_S; perdre(e, 1); }   // −1 par seconde collée
}
void arrivee(Etat& e, double distKm) { if (!e.arrete && !e.gagne && distKm >= KM_ARRIVEE && e.score >= 1) e.gagne = true; }
int ralentisseur(Etat& e, double kmh) {
    if (e.arrete || e.gagne || kmh <= RALENTISSEUR_KMH) return 0;
    e.ralentisseurs++; const int avant = e.score; perdre(e, CHOC_RALENTISSEUR); return avant - e.score;
}
int policeVoulue(double distKm) { return std::min(POLICE_MAX, POLICE_DEPART + (int)std::floor(std::max(0.0, distKm) / RENFORT_KM)); }

// ---------------------------------------------------------------- police
bool attaque(const Police& p, int indice, double t) {
    if (p.fixe || t < 4) return false;
    return std::fmod(t + indice * (CYCLE_S / 2.7), CYCLE_S) < ATTAQUE_S;
}

void avancer(std::vector<Police>& ps, double dt, double t, double sJ, double xJ, double vJ, const std::vector<Obstacle>& obs) {
    const double vMax = MAX_SPEED * 1.45;
    for (size_t i = 0; i < ps.size(); i++) {
        Police& p = ps[i];
        p.hitCd = std::max(0.0, p.hitCd - dt);
        if (p.fixe) { p.v = 0; continue; }
        double rel = (sJ - p.rd) * M;   // > 0 : le joueur est devant (m)
        if (rel > 250) { p.rd = sJ - 120 / M; p.v = std::max(p.v, vJ); rel = 120; }   // la poursuite ne se perd pas
        // place visée : derrière à 15 m, à côté à 18 / 21 m (une file à gauche / à droite) ; en attaque : contre le joueur
        const bool att = attaque(p, (int)i, t);
        const double viseM = att ? (p.role == 0 ? 2.0 : 0.0) : 15.0 + p.role * 3.0;
        const double ecart = rel - viseM;
        double vCible = vJ + std::clamp(ecart * 1.6, -12.0, 45.0) / M;   // rattrape (≤ +45 m/s), recule s'il dépasse
        vCible = std::clamp(vCible, 0.0, vMax);
        double cx = p.role == 0 ? xJ : xJ + (p.role == 1 ? -0.62 : 0.62);
        if (rel > 40 && !att) cx = p.cibleX;   // loin : reste dans sa file (moins de zigzags)
        // trafic devant dans sa trajectoire (25 m) : change de file vers la plus dégagée, ou ralentit
        const Obstacle* gene = nullptr; double geneD = 1e9;
        for (const Obstacle& o : obs) {
            const double d = (o.s - p.rd) * M;
            if (d > 0 && d < 25 && std::abs(o.x - p.x) < 0.36 && d < geneD) { gene = &o; geneD = d; }
        }
        if (gene) {
            double mieux = p.x, mieuxD = -1;
            for (double f : FILES) {
                double libre = 200;
                for (const Obstacle& o : obs) {
                    const double d = (o.s - p.rd) * M;
                    if (d > -8 && d < libre && std::abs(o.x - f) < 0.36) libre = d;
                }
                if (libre > mieuxD + 1) { mieuxD = libre; mieux = f; }
            }
            cx = mieux;
            if (mieuxD < 12) vCible = std::min(vCible, gene->v);
        }
        if (rel <= 40 || gene) p.cibleX = cx;
        const double dx = std::clamp(p.cibleX - p.x, -0.9 * dt, 0.9 * dt);
        p.x = std::clamp(p.x + dx, -0.92, 0.92);
        const double acc = (vCible > p.v ? MAX_SPEED * 0.45 : MAX_SPEED * 0.9) * dt;
        p.v = std::clamp(vCible, p.v - acc, p.v + acc);
        p.rd += p.v * dt;
    }
    // deux voitures de police ne se chevauchent pas : celle de derrière reste 7 m derrière celle de devant
    for (size_t i = 0; i < ps.size(); i++)
        for (size_t j = 0; j < ps.size(); j++) {
            if (i == j || ps[i].fixe || ps[j].fixe) continue;
            const double d = (ps[j].rd - ps[i].rd) * M;
            if (d >= 0 && d < 7 && std::abs(ps[j].x - ps[i].x) < 0.3 && (d > 0 || i > j)) { ps[i].rd = ps[j].rd - 7 / M; ps[i].v = std::min(ps[i].v, ps[j].v); }
        }
}

Police renfort(int indice, double sJ, double vJ, uint32_t graine) {
    Mulberry32 R(graine * 2654435761u + (uint32_t)indice * 40503u + 11u);
    Police p;
    p.role = indice % 3;
    p.x = p.cibleX = FILES[(int)(R.next() * 3) % 3];
    p.rd = sJ - (90 + indice * 9) / M;
    p.v = vJ;
    p.peinture = indice % 2 ? 0x14224a : 0xf2f4f8;   // blanc / bleu marine
    return p;
}

double plusProcheM(const std::vector<Police>& ps, double sJ, double xJ) {
    double best = 1e9;
    for (const Police& p : ps) {
        if (p.fixe) continue;
        const double dz = (p.rd - sJ) * M, dx = (p.x - xJ) * RW;
        best = std::min(best, std::sqrt(dz * dz + dx * dx));
    }
    return best;
}

// ---------------------------------------------------------------- programme (graine)
const char* bandeau(Surprise s) {
    switch (s) {
        case Surprise::Barrage: return "BARRAGE DE POLICE";
        case Surprise::Deboite: return "ATTENTION : VOITURE QUI DÉBOÎTE";
        case Surprise::Travaux: return "TRAVAUX : FILE FERMÉE";
        case Surprise::Accident: return "ACCIDENT DEVANT";
        case Surprise::Camion: return "CAMION EN TRAVERS";
        case Surprise::Ralentisseur: return "RALENTISSEUR";
        default: return "";
    }
}

Programme programme(uint32_t graine, bool banc) {
    Mulberry32 R(graine ? graine : 1);
    Programme p;
    // 3 embouteillages : vers 2,5, 5,5 et 8 km (± 0,6 km), 300 à 600 m, une file libre (passage étroit)
    for (int i = 0; i < 3; i++) {
        Bouchon b;
        b.km = 2.5 + i * 2.8 + (R.next() - 0.5) * 1.2;
        b.longueurM = 300 + R.next() * 300;
        b.passage = (int)(R.next() * 3) % 3;
        b.kmhMin = 20; b.kmhMax = 40;
        p.bouchons.push_back(b);
    }
    if (banc) { p.bouchons[0].km = 0.9; p.bouchons[0].longueurM = 400; p.bouchons[0].passage = 2; }   // captures : embouteillage à 0,9 km
    // 6 surprises : les 6 types mélangés (une fois chacun, WFS-119) ; une tous les 0,9 à 1,3 km dès 0,9 km
    std::vector<Surprise> types;
    for (int i = 0; i < TYPES_SURPRISE; i++) types.push_back((Surprise)i);
    for (int i = TYPES_SURPRISE - 1; i > 0; i--) std::swap(types[(size_t)i], types[(size_t)((int)(R.next() * (i + 1)) % (i + 1))]);
    double km = 0.9;
    for (Surprise t : types) {
        Evenement e; e.km = km; e.type = t; e.file = (int)(R.next() * 3) % 3;
        if (t == Surprise::Camion) e.file = fileLibreCamion(e.file);
        // pas de surprise dans un embouteillage
        for (int passe = 0; passe < 2; passe++)
            for (const Bouchon& b : p.bouchons) if (e.km > b.km - 0.25 && e.km < b.km + b.longueurM / 1000 + 0.1) e.km = b.km + b.longueurM / 1000 + 0.2;
        if (banc && e.km < 2.4) e.km = 2.4;   // banc : la place des captures (barrage, ralentisseur, camion) reste libre
        if (e.km < KM_ARRIVEE - 0.3) p.surprises.push_back(e);
        km = e.km + 0.9 + R.next() * 0.4;
    }
    if (banc) {
        p.surprises.insert(p.surprises.begin(), Evenement{BANC_CAMION_KM, Surprise::Camion, 2, false});
        p.surprises.insert(p.surprises.begin(), Evenement{BANC_RALENTISSEUR_KM, Surprise::Ralentisseur, 0, false});
    }
    if (banc) p.surprises.insert(p.surprises.begin(), Evenement{0.45, Surprise::Barrage, 0, false});   // captures : barrage à 0,45 km
    return p;
}

// ---------------------------------------------------------------- camion en travers (WFS-119)
const char* camionCar(const char* carJoueur) { return carJoueur && !std::strcmp(carJoueur, "x5") ? "rrsport" : "x5"; }
int fileLibreCamion(int file) { return file == 1 ? 0 : file; }
Travers camionTravers(int fileLibre, double longueurM, double largeurM) {
    // à cheval sur la limite des 2 files couvertes (±RW/3) : la file libre (0 : gauche, 2 : droite) n'est pas touchée
    Travers tv;
    tv.xM = (fileLibre == 0 ? 1 : -1) * RW / 3;
    tv.demiX = longueurM * CAMION_ECHELLE / 2;
    tv.demiZ = largeurM * CAMION_ECHELLE / 2;
    return tv;
}

// ---------------------------------------------------------------- mode gardé (WFS-119 RG-03)
std::string cleMode(ModeCourse m) { return m == ModeCourse::Course5 ? "course5" : m == ModeCourse::Poursuite ? "poursuite" : "checkpoint"; }
ModeCourse modeDeCle(const std::string& cle) { return cle == "course5" ? ModeCourse::Course5 : cle == "poursuite" ? ModeCourse::Poursuite : ModeCourse::Checkpoint; }

// WFS-141 (RG-01) : 2 Hz = période de 0,5 s : rouge la 1re moitié, bleu la 2e (une voiture sur deux : l'inverse)
Gyrophare gyrophare(double t, int voiture) {
    const double ph = t - std::floor(t / 0.5) * 0.5;   // 0 ≤ ph < 0,5 (t négatif compris)
    const bool premier = (ph < 0.25) != (voiture % 2 == 1);
    Gyrophare g; g.rouge = premier; g.bleu = !premier;
    return g;
}
// WFS-141 (RG-03) : « 2e PLACE » : le « e » reste minuscule (ui::majuscules le mettait en capitale, comme ANO-35)
std::string titrePlace(int rang) { return rang <= 1 ? "VICTOIRE" : std::to_string(rang) + "e PLACE"; }

} // namespace poursuite
} // namespace wfs
