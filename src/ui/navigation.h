// WaForSpeed — carte de navigation façon GPS (WFS-234, épic E5) : calculs purs, sans rendu, utilisés par la carte du HUD
// (minicarte.cpp) et par tests/test_navigation.cpp.
//   RG-01 : zoom qui s'éloigne avec la vitesse ; nature du terrain autour de la route (bâtiments, verdure, eau) par zone ;
//   RG-02 : prochaine manœuvre (virage, épingle, rond-point et sa sortie, carrefour à feux, arrivée) et sa distance arrondie
//           comme un GPS (« dans 250 m ») ; noms de rues INVENTÉS (tirés de listes de mots communs, aucun nom de ville) ;
//   RG-03 : signalements fictifs façon Waze (radar, travaux) posés une fois pour toutes sur le tracé, sans effet sur le jeu.
// Le tracé 2 (trace_cartes.h) est la seule source : sections typées d'un tour, répétées à chaque tour (la route ne reboucle
// jamais en 3D, mais son plan se répète tous les Track::length()).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "../game/trace_cartes.h"

namespace wfs::nav {

// ---- RG-01 : zoom selon la vitesse : distance montrée devant le joueur (m) : 160 m à l'arrêt et en ville, 300 m à 150 km/h,
// 400 m au-delà de 236 km/h (un rond-point de 14 m de rayon reste lisible sur une carte de 110 px de haut)
constexpr double AVANT_MIN_M = 160, AVANT_MAX_M = 400, AVANT_PAR_KMH = 1.1, AVANT_ARRET_M = 140;
inline double avantM(double kmh) { return std::clamp(AVANT_ARRET_M + AVANT_PAR_KMH * std::max(kmh, 0.0), AVANT_MIN_M, AVANT_MAX_M); }
// lissage du zoom et du cap (constante de temps, s) : un GPS ne saute jamais d'une image à l'autre
constexpr double TAU_ZOOM = 1.0, TAU_CAP = 0.25;
inline double lisser(double actuel, double cible, double dt, double tau) {
    if (!(dt > 0) || !std::isfinite(actuel)) return cible;
    return actuel + (cible - actuel) * (1 - std::exp(-dt / tau));
}

// ---- RG-02 : manœuvres
enum class Manoeuvre : uint8_t { Aucune, LegerGauche, LegerDroite, Gauche, Droite, SerreGauche, SerreDroite, EpingleGauche,
                                 EpingleDroite, Lacets, Chicane, RondPoint, Feux, Arrivee };
// virage montré : au moins 35°, ou plus de 20° si le rayon est sous 100 m (une grande courbe d'autoroute n'est pas une manœuvre)
inline Manoeuvre manoeuvreDe(const trace::Section& s) {
    using trace::Type;
    const bool d = s.cote > 0 || (s.cote == 0 && s.angle > 0);
    switch (s.type) {
        case Type::Epingle: return d ? Manoeuvre::EpingleDroite : Manoeuvre::EpingleGauche;
        case Type::Lacets: return Manoeuvre::Lacets;
        case Type::Chicane: return Manoeuvre::Chicane;
        case Type::RondPoint: return Manoeuvre::RondPoint;
        case Type::Feux: return s.depart ? Manoeuvre::Aucune : Manoeuvre::Feux;
        case Type::Virage: case Type::Autoroute: case Type::Montee: case Type::Descente: {
            const double a = std::abs(s.angle);
            if (s.rayon <= 0 || a < 20 || (a < 35 && s.rayon >= 100)) return Manoeuvre::Aucune;
            if (a < 60) return d ? Manoeuvre::LegerDroite : Manoeuvre::LegerGauche;
            if (s.rayon < 30) return d ? Manoeuvre::SerreDroite : Manoeuvre::SerreGauche;
            return d ? Manoeuvre::Droite : Manoeuvre::Gauche;
        }
        default: return Manoeuvre::Aucune;
    }
}
// rond-point du tracé 2 (trace_cartes.cpp) : 4 branches à 90° (entrée, sortie, 2 amorces) ; virage net s.angle (−90 : ¾ de tour)
// → sortie n° 2 − angle / 90 (−90 → 3e sortie, 0 → 2e, +90 → 1re), comptée dans le sens de l'anneau
inline int sortieRondPoint(const trace::Section& s) { return std::clamp(2 - (int)std::lround(s.angle / 90.0), 1, 4); }

struct Prochaine {
    Manoeuvre m = Manoeuvre::Aucune;
    double distanceM = 0;    // du joueur au début de la manœuvre (0 : on y est)
    double sAbs = 0;         // abscisse absolue (unités) du début de la manœuvre
    int section = -1;        // indice dans les sections du tour (−1 : arrivée ou rien)
    int sortie = 0;          // rond-point : numéro de la sortie
};
// prochaine manœuvre à partir de l'abscisse absolue s (unités) : la section qui contient s compte (distance 0) tant qu'on
// n'en est pas sorti ; tour de longueur L (unités) ; arrivee : abscisse absolue de l'arrivée (≤ 0 : aucune) ; M : m par unité
inline Prochaine prochaine(const std::vector<trace::Section>& secs, double s, double L, double arrivee, double M, double horizonM = 5000) {
    Prochaine p;
    double best = horizonM;
    if (L > 0) {
        const double tour = std::floor(s / L) * L, sT = s - tour;
        for (int i = 0; i < (int)secs.size(); i++) {
            const Manoeuvre m = manoeuvreDe(secs[i]);
            if (m == Manoeuvre::Aucune) continue;
            const double d0 = secs[i].debutU(), d1 = secs[i].finU();
            for (int k = 0; k < 2; k++) {   // ce tour et le suivant
                const double a = d0 + k * L, b = d1 + k * L;
                if (b <= sT) continue;
                const double dist = std::max(0.0, (a - sT) * M);
                if (dist < best) { best = dist; p = {m, dist, tour + a, i, m == Manoeuvre::RondPoint ? sortieRondPoint(secs[i]) : 0}; }
                break;
            }
        }
    }
    if (arrivee > 0 && arrivee >= s) {
        const double dist = (arrivee - s) * M;
        if (dist <= best) p = {Manoeuvre::Arrivee, dist, arrivee, -1, 0};
    }
    return p;
}

// distance arrondie comme un GPS : « maintenant » sous 15 m, au 10 m sous 100 m, au 50 m sous 1 km, puis au 0,1 km
inline std::string texteDistance(double m) {
    char b[32];
    if (!(m >= 15)) return "Maintenant";
    if (m < 100) std::snprintf(b, sizeof b, "%d m", (int)std::lround(m / 10) * 10);
    else if (m < 975) std::snprintf(b, sizeof b, "%d m", (int)std::lround(m / 50) * 50);
    else {
        const long d = std::lround(m / 100);   // dixièmes de km, virgule française
        if (d % 10 == 0) std::snprintf(b, sizeof b, "%ld km", d / 10); else std::snprintf(b, sizeof b, "%ld,%ld km", d / 10, d % 10);
    }
    return b;
}
inline const char* texteManoeuvre(Manoeuvre m) {
    switch (m) {
        case Manoeuvre::LegerGauche: return "Serrer à gauche";
        case Manoeuvre::LegerDroite: return "Serrer à droite";
        case Manoeuvre::Gauche: return "Tourner à gauche";
        case Manoeuvre::Droite: return "Tourner à droite";
        case Manoeuvre::SerreGauche: return "Virage serré à gauche";
        case Manoeuvre::SerreDroite: return "Virage serré à droite";
        case Manoeuvre::EpingleGauche: return "Épingle à gauche";
        case Manoeuvre::EpingleDroite: return "Épingle à droite";
        case Manoeuvre::Lacets: return "Lacets";
        case Manoeuvre::Chicane: return "Chicane";
        case Manoeuvre::RondPoint: return "Au rond-point";
        case Manoeuvre::Feux: return "Carrefour à feux, tout droit";
        case Manoeuvre::Arrivee: return "Arrivée";
        default: return "Continuer tout droit";
    }
}
inline std::string texteConsigne(const Prochaine& p) {
    if (p.m == Manoeuvre::RondPoint) return std::string("Au rond-point, ") + std::to_string(p.sortie) + (p.sortie == 1 ? "re sortie" : "e sortie");
    return texteManoeuvre(p.m);
}
// sens de la flèche : −1 gauche, +1 droite, 0 tout droit
inline int sensDe(Manoeuvre m) {
    switch (m) {
        case Manoeuvre::LegerGauche: case Manoeuvre::Gauche: case Manoeuvre::SerreGauche: case Manoeuvre::EpingleGauche: return -1;
        case Manoeuvre::LegerDroite: case Manoeuvre::Droite: case Manoeuvre::SerreDroite: case Manoeuvre::EpingleDroite: return 1;
        default: return 0;
    }
}

// ---- RG-02 : noms de rues inventés : genre de voie selon la carte et la section, complément tiré (graine de la carte + indice)
// Listes de mots communs (arbres, fleurs, métiers, lieux-dits) : aucune ville, aucune personne, aucun lieu réel désigné.
inline uint32_t melange(uint32_t x) { x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16; return x; }
inline std::string nomRue(const char* carte, uint32_t graine, int section, trace::Type type, bool autoroute) {
    static const char* const COMPL[] = {"des Glycines", "des Tilleuls", "du Moulin", "des Mimosas", "de la Source", "des Peupliers",
        "du Vieux Puits", "des Lavandières", "des Oliviers", "du Belvédère", "des Acacias", "de la Fontaine", "des Tisserands",
        "du Grand Pré", "des Hirondelles", "de l'Horloge", "des Cerisiers", "du Pont Neuf", "des Marronniers", "de la Halle",
        "des Coquelicots", "du Lavoir", "des Platanes", "des Vergers"};
    constexpr int NC = sizeof COMPL / sizeof COMPL[0];
    const bool ville = carte && !std::strcmp(carte, "ville"), plage = carte && !std::strcmp(carte, "plage");
    const bool circuit = carte && !std::strcmp(carte, "circuit");
    const uint32_t h = melange(graine * 2654435761U + (uint32_t)section * 40503U + 17U);
    if (autoroute || type == trace::Type::Autoroute) return std::string("Voie rapide ") + (h % 2 ? "du Littoral" : "des Collines");
    if (type == trace::Type::Tunnel) return "Tunnel du Rocher";
    if (circuit) {
        static const char* const V[] = {"Virage des Pins", "Courbe du Paddock", "Épingle des Tribunes", "Ligne droite des Stands", "Virage du Bois"};
        return V[h % 5];
    }
    const char* genre;
    if (type == trace::Type::RondPoint) genre = "Place";
    else if (plage) { static const char* const G[] = {"Promenade", "Quai", "Corniche", "Boulevard"}; genre = G[(h >> 8) % 4]; }
    else if (ville) { static const char* const G[] = {"Rue", "Avenue", "Boulevard", "Rue", "Allée"}; genre = G[(h >> 8) % 5]; }
    else { static const char* const G[] = {"Route", "Chemin", "Route", "Rue"}; genre = G[(h >> 8) % 4]; }
    return std::string(genre) + " " + COMPL[h % NC];
}
// section du tour qui contient l'abscisse absolue s (unités), −1 sinon
inline int sectionDe(const std::vector<trace::Section>& secs, double s, double L) {
    if (L <= 0) return -1;
    const double sT = s - std::floor(s / L) * L;
    for (int i = 0; i < (int)secs.size(); i++) if (sT >= secs[i].debutU() && sT < secs[i].finU()) return i;
    return -1;
}

// ---- RG-01 : terrain autour de la route, d'après la zone (zones.h) : aplats doux de la carte
enum class Terrain : uint8_t { Batiments, Verdure, Foret, Champs, Eau, Gravier };
// eauDroite : la mer est à droite de la route (Corniche : plage.cpp, lat > 0 côté mer)
struct Abords { Terrain gauche = Terrain::Verdure, droite = Terrain::Verdure; };
inline Abords abordsDe(const char* carte, const char* zone) {
    auto est = [&](const char* z) { return zone && !std::strcmp(zone, z); };
    const bool plage = carte && !std::strcmp(carte, "plage");
    if (plage) {
        if (est("rochers")) return {Terrain::Verdure, Terrain::Eau};
        return {Terrain::Batiments, Terrain::Eau};   // corniche, marina, village de pêcheurs : façades côté terre, mer côté droit
    }
    if (est("centre") || est("residentiel") || est("port") || est("village")) return {Terrain::Batiments, Terrain::Batiments};
    if (est("parc")) return {Terrain::Verdure, Terrain::Verdure};
    if (est("foret")) return {Terrain::Foret, Terrain::Foret};
    if (est("champs") || est("vignes")) return {Terrain::Champs, Terrain::Champs};
    if (est("gravier")) return {Terrain::Gravier, Terrain::Verdure};
    if (est("tribunes") || est("paddock")) return {Terrain::Batiments, Terrain::Verdure};
    return {};
}

// ---- RG-03 : signalements fictifs (sans effet sur le jeu) : un radar au milieu de chaque ligne droite d'au moins 400 m (une
// sur deux), des travaux au tiers de chaque 5e ligne droite d'au moins 200 m hors autoroute ; abscisses dans le tour (unités)
enum class Signal : uint8_t { Police, Barrage, Adversaire, Accident, Bouchon, Radar, Travaux };
struct SignalFixe { Signal type; double sTour; int id; };
inline std::vector<SignalFixe> signauxFixes(const std::vector<trace::Section>& secs, double M) {
    std::vector<SignalFixe> v;
    int lignes = 0, longues = 0;
    for (int i = 0; i < (int)secs.size(); i++) {
        const trace::Section& s = secs[i];
        const bool droite = s.type == trace::Type::Ligne || (s.type == trace::Type::Autoroute && s.rayon <= 0);
        if (!droite) continue;
        const double l = (s.finU() - s.debutU()) * M;
        if (l >= 400 && (longues++ % 2) == 0) v.push_back({Signal::Radar, (s.debutU() + s.finU()) / 2, i});
        if (l >= 200 && !s.autoroute && (lignes++ % 5) == 2) v.push_back({Signal::Travaux, s.debutU() + (s.finU() - s.debutU()) / 3, i});
    }
    return v;
}

// bouchon (RG-03) : au moins BOUCHON_N voitures du trafic à moins de BOUCHON_M l'une de la suivante, toutes sous BOUCHON_KMH ;
// v : vitesses (km/h) et abscisses (m), triées par abscisse ; rend l'indice de la voiture de queue de chaque bouchon (ou −1)
constexpr int BOUCHON_N = 3; constexpr double BOUCHON_M = 25, BOUCHON_KMH = 25;
inline int bouchons(const double* sM, const double* kmh, int n, int* queues, int max) {
    int nb = 0, debut = -1, compte = 0;
    for (int i = 0; i <= n; i++) {
        const bool lent = i < n && kmh[i] < BOUCHON_KMH;
        const bool suite = lent && compte > 0 && sM[i] - sM[i - 1] <= BOUCHON_M;
        if (suite) { compte++; continue; }
        if (compte >= BOUCHON_N && nb < max) queues[nb++] = debut;
        if (lent) { debut = i; compte = 1; } else { debut = -1; compte = 0; }
    }
    return nb;
}

} // namespace wfs::nav
