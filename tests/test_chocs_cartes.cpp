// WFS-256 : obstacles déclarés de la Ville et des îlots de rond-point (chocs_declaration.cpp), sur la vraie piste :
//  - RG-05 (aucun mur invisible) : un tour complet en pilote automatique (Golf, SF90) et les 6 virages du banc de conduite
//    sur la ligne idéale ne touchent aucun obstacle ;
//  - RG-02 : pour chaque genre d'objet déclaré (poteau, arbre, mobilier, mur, bâtiment, îlot), la voiture lancée droit dessus
//    à 30, 80 et 150 km/h le touche et ne le traverse pas (objets hors de portée du bord de la carte : signalés, non testés).
// usage : test_chocs_cartes
#include <cmath>
#include <cstdio>
#include <cstring>
#include "../src/game/aide_direction_banc.h"
#include "../src/game/conduite_banc.h"
#include "../src/game/cars.h"
#include "../src/game/chocs_decor.h"
#include "../src/game/player.h"
#include "../src/game/trace_cartes.h"
#include "../src/game/track.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static int tourPilote(Track& t, const char* car) {
    Player p; p.reset(carById(car)); p.aide = Aide::Activee;
    int chocs = 0;
    for (int n = 0; n < 120 * 600 && p.dist < t.length(); n++) {
        DriveInput in; in.gas = true;
        in.steer = (float)std::clamp((0.0 - p.playerX) * 3.0 - p.heading * 4.0, -1.0, 1.0);
        limiterPilote(p, t, in);
        p.update(1.0 / 120, in, t, true);
        if (p.chocsDecor.n) {
            if (chocs < 3) std::printf("    choc au tour du pilote : %.0f m, écart %+.2f m, %s\n", p.dist * M, p.playerX * RW, chocs::nomGenre(p.chocsDecor.genre));
            chocs += p.chocsDecor.n; p.chocsDecor = {};
        }
    }
    return chocs;
}

int main() {
    for (const char* id : {"ville", "campagne", "plage", "circuit"}) {
        static Track t; t.build(mapById(id), trace::versionCarte(id));
        const chocs::Declares d = chocs::declarerCarte(t, mapById(id));
        std::printf("%s : %zu obstacle(s) déclarés (Ville %d, îlots %d)\n", id, chocs::registre().taille(), d.ville, d.ilots);
        // RG-05 : tours du pilote automatique
        for (const char* car : {"golf", "sf90"}) {
            const int c = tourPilote(t, car);
            std::printf("  tour complet au pilote, %s : %d choc(s)\n", car, c);
            VERIFIE(c == 0, "%s, %s : le pilote automatique touche un obstacle (mur invisible ou obstacle sur la chaussée)", id, car);
        }
        // RG-02 : chaque genre, droit dessus, 30 / 80 / 150 km/h
        const auto& ps = chocs::registre().parSegment();
        const double bord = (mapById(id).edge) * RW;
        bool fait[8] = {};
        for (size_t a = 0; a < ps.size(); a++)
            for (const chocs::ObstacleRoute& o : ps[a]) {
                const int g = (int)o.genre;
                if (fait[g]) continue;
                const double portee = o.forme == chocs::Forme::Cylindre ? o.r : o.hx;
                if (std::abs(o.lat) - portee > bord + 0.85) continue;   // hors de portée : la voiture ne dépasse pas le bord de la carte
                if (a < 80) continue;                                    // assez de piste avant pour l'élan
                fait[g] = true;
                const double s0 = a * SEG + o.along / M;
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
                        const double dx = p.monde.x - B.x, dz = p.monde.z - B.z, avance = dx * std::sin(p.monde.cap) - dz * std::cos(p.monde.cap);
                        plusLoin = std::max(plusLoin, avance);   // > 0 : le centre de la voiture a dépassé le centre de l'objet
                    }
                    std::printf("  %-9s (latéral %+5.1f m, rayon / demi-largeur %.2f m) à %3.0f km/h : %d choc(s), dépassement %.2f m\n",
                                chocs::nomGenre(o.genre), o.lat, portee, kmh, chocs, plusLoin);
                    VERIFIE(chocs >= 1, "%s : %s à %.0f km/h : aucun choc", id, chocs::nomGenre(o.genre), kmh);
                    VERIFIE(plusLoin < 0, "%s : %s à %.0f km/h : traversé (%.2f m)", id, chocs::nomGenre(o.genre), kmh, plusLoin);
                }
            }
    }
    {   // RG-05 : virages du banc de conduite (ligne idéale, aide activée) : aucun choc
        int c = 0;
        for (const banc_conduite::Virage& v : banc_conduite::virages()) {
            Track& t = banc_conduite::piste(v.carte);
            chocs::declarerCarte(t, mapById(v.carte));
            Player p; p.reset(carById("golf")); p.aide = Aide::Activee;
            p.dist = v.debutU - 80 / M; p.position = t.wrap(p.dist); p.speed = vitesseVirageNormale(t, p.dist);
            for (int n = 0; n < 120 * 60 && p.dist < v.finU + 40 / M; n++) { p.update(1.0 / 120, joueurLigneIdeale(p, t), t, true); c += p.chocsDecor.n; p.chocsDecor = {}; }
        }
        std::printf("virages du banc (ligne idéale) : %d choc(s)\n", c);
        VERIFIE(c == 0, "ligne idéale dans les virages : %d choc(s) (mur invisible ?)", c);
    }
    std::printf("obstacles des cartes (WFS-256) : %s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
