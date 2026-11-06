// WaForSpeed — tracé des cartes, 2e version (ticket WFS-136, épic E3).
// Chaque carte est une liste ordonnée de sections typées (ligne, virage, épingle, lacets, montée, rond-point, feux,
// autoroute, tunnel, pont, chicane) qui remplace le tirage au hasard des courbes de Track::build (tracé 1 = jeu web).
// La route est toujours construite par addRoad (port de js/world/track.js) : aucun nouveau moteur de route.
//
// API en lecture pour le décor (E2, WFS-137 à 140) et le son (E6) :
//   trace::sections(piste)          liste des sections (vide pour le tracé 1 du jeu web)
//   trace::sectionA(piste, z)       section sous l'abscisse z (unités, ramenée sur un tour), nullptr sans tracé 2
//   trace::rayonInterieur(piste, s) rayon (m) de la courbe la plus serrée à ±6 m de s et son côté (le décor ne doit pas
//                                   être posé au-delà de ce rayon du côté intérieur : il retomberait sur la chaussée)
//   trace::feuA(i)                  état du i-ème carrefour à feux de la carte en cours (vert, orange, rouge, éteint)
#pragma once
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace wfs {
class Track;
struct MapDef;
struct Segment;

namespace trace {

constexpr int VERSION = 2;   // version du tracé : sauvegarde (trace=2, trace-<carte>=2)

// A7 (décision du PO, 2026-11-06) : activation carte par carte. Une carte absente garde exactement le tracé 1 du jeu web.
// Chaque ticket de décor (WFS-137 Ville, WFS-138 Campagne, WFS-139 Corniche) ajoute sa carte sur CETTE ligne, dans le même
// commit que son décor adapté aux sections serrées.
inline constexpr const char* CARTES_ACTIVES[] = {"circuit", "ville"};   // WFS-137 : ville (décor adapté aux sections serrées)
bool carteActive(const char* id);              // dans CARTES_ACTIVES
int versionCarte(const char* id);              // 2 si active (ou forcée au banc), sinon 1 : Track::build(map, versionCarte(id))
void forcerTrace2(bool oui);                   // banc WFS_BANC=trace : tracé 2 sur les 4 cartes
std::string signature();                       // champ « trace » du salon du duel : cartes actives (« circuit »), « 1 » sans carte

enum class Type : uint8_t { Ligne, Virage, Epingle, Lacets, Montee, Descente, RondPoint, Feux, Autoroute, Tunnel, Pont, Chicane };
const char* nomType(Type t);

struct Section {
    Type type = Type::Ligne;
    int64_t debut = 0, fin = 0;   // segments [debut, fin) du tour
    int cote = 0;                 // −1 gauche, +1 droite (virages, épingles, rond-point : côté de l'entrée), 0 sinon
    double rayon = 0;             // m : rayon du virage, de l'épingle, de l'anneau du rond-point (0 : ligne droite)
    double angle = 0;             // degrés tournés (signés : + à droite)
    double denivele = 0;          // m : montée (+) ou descente (−) sur la section
    bool autoroute = false;       // tronçon d'autoroute (aussi vrai pour un tunnel ou un pont sur l'autoroute)
    int feu = -1;                 // FEUX : indice du carrefour (feuA) ; −1 sinon
    bool depart = false;          // FEUX de départ du Grand Prix (rouges au décompte, éteints au « GO »)
    int64_t ligneArret = 0;       // FEUX : segment de la ligne d'arrêt ; le carrefour (passage des voitures) est 8 m après
    int branches = 0;             // ROND_POINT : amorces de branches pour le décor (30 m chacune)
    double debutU() const;        // début et fin en unités
    double finU() const;
};

// construit le tracé 2 de la carte (appelée par Track::build(map, 2)) ; rend les sections
std::vector<Section> construire(Track& t, const MapDef& map);

const std::vector<Section>& sections(const Track& t);
const Section* sectionA(const Track& t, double z);
// m, cote : courbe la plus serrée (cote +1 : intérieur à droite) ; gauche, droite : rayon le plus petit de chaque côté
struct Rayon { double m = std::numeric_limits<double>::infinity(); int cote = 0;
               double gauche = std::numeric_limits<double>::infinity(), droite = std::numeric_limits<double>::infinity(); };
Rayon rayonInterieur(const Track& t, double s);

// ---------- carrefours à feux (RG-03) : vert 14 s, orange 3 s, rouge 10 s, cycle décalé de 9 s par carrefour ----------
enum class Feu : uint8_t { Vert, Orange, Rouge, Eteint };
constexpr double FEU_VERT = 14, FEU_ORANGE = 3, FEU_ROUGE = 10, FEU_CYCLE = FEU_VERT + FEU_ORANGE + FEU_ROUGE, FEU_DECALAGE = 9;
constexpr double CARREFOUR_M = 8;   // distance de la ligne d'arrêt au passage des voitures qui traversent (m)
// état d'un carrefour à l'horloge du trafic (s, < 0 pendant le décompte) ; tempsDansPhase : secondes écoulées dans la phase
Feu feuCarrefour(int i, double horloge, double* tempsDansPhase = nullptr);
// carte en cours (mis à jour par le système des feux, ordre 205) : état du i-ème carrefour (Eteint s'il n'existe pas)
Feu feuA(int i);
void poserCarte(const Track* piste, double horloge);   // système des feux (trace_cartes_systeme.cpp)
int nombreFeux(const Track& t);

// ---------- trafic (RG-02, RG-04) ----------
// vitesse maximale conseillée au trafic à l'abscisse z (unités/s ; infini = pas de limite) : 40 km/h dans l'anneau,
// 60 km/h dans les épingles ; facteur de vitesse de croisière (1,15 sur l'autoroute)
double vitesseTrafic(const Track& t, double z);
double facteurTrafic(const Track& t, double z);
// longueur d'autoroute du tour (unités), pour les voitures en plus (+30 %)
double longueurAutoroute(const Track& t);

} // namespace trace
class Game;
class Renderer;
namespace trace {
// banc d'essai (WFS_BANC=trace) : captures trace-*.png dans outDir ; rend le nombre d'écarts
int benchTrace(Game& g, Renderer& r, const std::string& outDir);
} // namespace trace
} // namespace wfs
