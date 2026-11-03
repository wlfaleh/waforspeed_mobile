// WaForSpeed — zones des cartes (WFS-14) : voir zones.h.
#include "zones.h"
#include <cstring>
#include "../game/track.h"

namespace wfs {
namespace {

struct Theme { const char* id; const char* name; };
// ZONES du jeu web : clé = MAPS[].ground (ville = city, plage = beach, campagne = country, circuit = circuit)
const Theme CITY[] = {{"centre", "Centre-ville"}, {"residentiel", "Quartier résidentiel"}, {"parc", "Parc urbain"}, {"port", "Zone portuaire"}, {"centre", "Quartier d'affaires"}, {"residentiel", "Vieux quartier"}};
const Theme BEACH[] = {{"corniche", "Corniche"}, {"marina", "Port de plaisance"}, {"rochers", "Côte rocheuse"}, {"village", "Village de pêcheurs"}};
const Theme COUNTRY[] = {{"champs", "Champs"}, {"foret", "Forêt"}, {"village", "Village"}, {"vignes", "Vignobles"}, {"champs", "Plaine"}, {"foret", "Bois de pins"}};
const Theme CIRCUIT[] = {{"tribunes", "Ligne droite des tribunes"}, {"paddock", "Paddock"}, {"gravier", "Virages rapides"}, {"foret", "Section boisée"}};

const Theme* themes(const MapDef& m, int& n) {
    if (!std::strcmp(m.id, "plage")) { n = 4; return BEACH; }
    if (!std::strcmp(m.id, "campagne")) { n = 6; return COUNTRY; }
    if (!std::strcmp(m.id, "circuit")) { n = 4; return CIRCUIT; }
    n = 6; return CITY;
}

} // namespace

int zoneIndex(const MapDef& map, int64_t a, int64_t len) {
    int n; themes(map, n);
    const int64_t s = ((a % len) + len) % len;
    // même calcul que generateDecor : la dernière zone, trop courte, prolonge la précédente
    const int64_t zi = (s / ZONE_LEN) % n;
    return (int)(s >= (len / ZONE_LEN) * ZONE_LEN && s >= ZONE_LEN ? ((s - ZONE_LEN) / ZONE_LEN) % n : zi);
}
const char* zoneId(const MapDef& map, int64_t a, int64_t len) { int n; return themes(map, n)[zoneIndex(map, a, len)].id; }
const char* zoneName(const MapDef& map, int64_t a, int64_t len) { int n; return themes(map, n)[zoneIndex(map, a, len)].name; }

} // namespace wfs
