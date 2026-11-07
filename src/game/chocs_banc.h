// WaForSpeed — mesures des chocs avec le décor (WFS-256, RG-05, RG-06), sans GPU : partagées par le banc WFS_BANC=chocs
// (chocs_banc.cpp : registre rempli par le vrai chargement des cartes) et tests/test_chocs_cartes.cpp (Ville, îlots).
//  - RG-05 : tours complets en pilote automatique (Golf, SF90) : aucun choc (aucun mur invisible, rien sur la chaussée) ;
//  - RG-02 / RG-06 : pour chaque genre d'objet déclaré à portée (le bord de la carte borne la voiture), la voiture lancée droit
//    dessus à 30, 80 et 150 km/h le touche et ne le traverse pas.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include "aide_direction_banc.h"
#include "cars.h"
#include "chocs_decor.h"
#include "player.h"
#include "track.h"

namespace wfs::banc_chocs {

inline int tourPilote(Track& t, const char* car, std::string& rapport) {
    Player p; p.reset(carById(car)); p.aide = Aide::Activee;
    int chocs = 0;
    for (int n = 0; n < 120 * 600 && p.dist < t.length(); n++) {
        DriveInput in; in.gas = true;
        in.steer = (float)std::clamp((0.0 - p.playerX) * 3.0 - p.heading * 4.0, -1.0, 1.0);
        limiterPilote(p, t, in);
        p.update(1.0 / 120, in, t, true);
        if (p.chocsDecor.n) {
            if (chocs < 3) { char l[160]; std::snprintf(l, sizeof l, "    choc au tour du pilote : %.0f m, écart %+.2f m, %s\n", p.dist * M, p.playerX * RW, chocs::nomGenre(p.chocsDecor.genre)); rapport += l; }
            chocs += p.chocsDecor.n; p.chocsDecor = {};
        }
    }
    return chocs;
}

// registre déjà rempli pour la carte de la piste t ; rend le nombre d'écarts, rapport : tableau
inline int mesurer(Track& t, const MapDef& map, std::string& rapport) {
    int ecarts = 0; char l[260];
    std::snprintf(l, sizeof l, "## %s : %zu obstacle(s) déclarés\n", map.id, chocs::registre().taille()); rapport += l;
    for (const char* car : {"golf", "sf90"}) {
        const int c = tourPilote(t, car, rapport);
        std::snprintf(l, sizeof l, "- tour complet au pilote, %s : %d choc(s) → %s\n", car, c, c ? "ÉCART (mur invisible ou obstacle sur la chaussée)" : "OK"); rapport += l;
        if (c) ecarts++;
    }
    const auto& ps = chocs::registre().parSegment();
    const double bord = map.edge * RW;
    bool fait[8] = {};
    rapport += "| genre | latéral | taille | 30 km/h | 80 km/h | 150 km/h |\n|---|---|---|---|---|---|\n";
    for (size_t a = 0; a < ps.size(); a++)
        for (const chocs::ObstacleRoute& o : ps[a]) {
            const int g = (int)o.genre;
            if (fait[g]) continue;
            const double portee = o.forme == chocs::Forme::Cylindre ? o.r : o.hx;
            if (std::abs(o.lat) - portee > bord + 0.85 || a < 80) continue;   // hors de portée (bord de la carte) ; élan
            fait[g] = true;
            const double s0 = a * SEG + o.along / M;
            std::snprintf(l, sizeof l, "| %s | %+.1f m | %.2f m |", chocs::nomGenre(o.genre), o.lat, portee); rapport += l;
            for (double kmh : {30.0, 80.0, 150.0}) {
                Player p; p.reset(carById("golf")); p.aide = Aide::Desactivee;
                const double cote = o.lat >= 0 ? 1.0 : -1.0;   // départ côté route : bord de l'objet + 2,5 m
                const double depart = s0 - 15 / M, latD = std::clamp((double)o.lat - cote * (portee + 2.5), -bord + 1, bord - 1);
                p.dist = depart; p.position = t.wrap(depart); p.playerX = latD / RW; p.speed = kmh / 3.6 / M;
                const TrackPoint A = t.pointLisse(depart, latD), B = t.pointLisse(s0, o.lat);
                p.heading = std::remainder(std::atan2(B.x - A.x, -(B.z - A.z)) - A.th, 2 * 3.14159265358979);   // visée droit sur l'objet
                int chocs = 0; double plusLoin = -1e9;
                for (int n = 0; n < 120 * 5; n++) {
                    p.update(1.0 / 120, DriveInput{}, t, true);
                    chocs += p.chocsDecor.n; p.chocsDecor = {};
                    const double dx = p.monde.x - B.x, dz = p.monde.z - B.z;
                    plusLoin = std::max(plusLoin, dx * std::sin(p.monde.cap) - dz * std::cos(p.monde.cap));   // > 0 : centre de l'objet dépassé
                }
                const bool bon = chocs >= 1 && plusLoin < 0;
                if (!bon) ecarts++;
                std::snprintf(l, sizeof l, " %s (%d choc(s)) |", bon ? "arrêtée" : chocs ? "TRAVERSÉ" : "AUCUN CHOC", chocs); rapport += l;
            }
            rapport += "\n";
        }
    return ecarts;
}

} // namespace wfs::banc_chocs

namespace wfs {
class Game; class Renderer;
int benchChocs(Game& g, Renderer& r, const std::string& out);   // banc WFS_BANC=chocs (chocs_banc.cpp) : rend le nombre d'écarts
} // namespace wfs
