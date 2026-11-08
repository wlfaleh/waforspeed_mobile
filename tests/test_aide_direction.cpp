// WFS-217 : aide à la direction (« guide auto ») de la voiture du joueur, dans les épingles, les lacets et les ronds-points
// des 4 cartes (tracé 2 de WFS-136). Un joueur simulé suit la ligne idéale comme un humain : il vise un point de la route
// devant lui (poursuite pure, en coordonnées du monde) et tient la vitesse de virage normale (celle du trafic : 60 km/h
// dans les épingles, 40 km/h dans l'anneau du rond-point, 70 km/h ailleurs), au glisser ou aux flèches, avec ou sans
// temps de réaction.
//  - aide Activée puis Désactivée : 0 sortie de route (|écart| ≤ 1 demi-largeur), aucun saut de cap > 0,05 rad par image ;
//  - aide Activée, volant lâché à la vitesse normale : la voiture reste sur la route (pas de sortie forcée) ;
//  - aide Désactivée, volant lâché : la voiture ne suit plus le virage (pas de guide caché) ;
//  - aides du jeu web (Aide::Web, ordinateur) : mesurées pour comparaison, comportement inchangé ;
//  - demi-tour depuis l'arrêt (WFS-90) possible avec l'aide activée comme désactivée.
// usage : test_aide_direction
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "../src/game/aide_direction_banc.h"
#include "../src/game/cars.h"
#include "../src/game/player.h"
#include "../src/game/trace_cartes.h"
#include "../src/game/track.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static double angle(double a) { return std::atan2(std::sin(a), std::cos(a)); }

// joueur simulé en cours : temps de réaction (pas de 1/120 s), flèches (−1 / 0 / +1) ou glisser, facteur de vitesse
static int g_retard = 0;
static bool g_fleches = false;
static double g_vitesse = 1;
struct Joueur { const char* nom; int retard; bool fleches; double vitesse; };
static const Joueur JOUEURS[] = {
    {"glisser, sans retard", 0, false, 1.0},
    {"flèches, 0,2 s de réaction", 24, true, 1.0},
    {"glisser, 0,3 s de réaction", 36, false, 1.0},
    // WFS-241 : vitesse normale = 0,7 g au plus ; 1,1 × = 0,85 g, 95 % de la limite des pneus de la Golf (0,89 g) (au-delà : voir « trop vite »)
    {"glisser, 1,1 × la vitesse normale", 0, false, 1.1},  // aide activée seulement : marge au-dessus de la vitesse normale
};
struct Mesure { double ecartMax = 0, sautMax = 0; int sorties = 0; double vMin = 1e9, vMax = 0; };

// joueur simulé (aide_direction_banc.h) : la vitesse visée est multipliée par g_vitesse
static DriveInput joueur(const Player& p, Track& t, bool volant) { return joueurLigneIdeale(p, t, volant, g_vitesse); }

// une section : départ 60 m avant, arrivée 40 m après ; pas de 1/120 s, cap du monde relevé toutes les 2 étapes (une image)
static Mesure rouler(Track& t, const trace::Section& s, Aide aide, bool volant, bool journal = false) {
    float file[256] = {};
    Player p; p.reset(carById("golf")); p.aide = aide;
    const double debut = s.debutU() - 60 / M, fin = s.finU() + 40 / M;
    p.dist = debut; p.position = t.wrap(debut); p.speed = vitesseVirageNormale(t, debut, g_vitesse);
    Mesure m;
    double psi0 = t.point(p.dist, 0).th + p.heading;
    for (int n = 0; p.dist < fin && n < 120 * 120; n++) {
        DriveInput in = joueur(p, t, volant);
        if (g_fleches) in.steer = in.steer > 0.3f ? 1.f : in.steer < -0.3f ? -1.f : 0.f;
        file[n % 256] = in.steer; if (g_retard) in.steer = n >= g_retard ? file[(n - g_retard) % 256] : 0.f;
        p.update(1.0 / 120, in, t, true);
        m.ecartMax = std::max(m.ecartMax, std::abs(p.playerX));
        if (std::abs(p.playerX) > 1) m.sorties++;
        if (p.dist > s.debutU() && p.dist < s.finU()) { m.vMin = std::min(m.vMin, p.kmh()); m.vMax = std::max(m.vMax, p.kmh()); }
        if (n % 2 == 1) {
            const double psi = t.point(p.dist, p.playerX * RW).th + p.heading;
            m.sautMax = std::max(m.sautMax, std::abs(angle(psi - psi0)));
            psi0 = psi;
        }
        if (journal && n % 60 == 0)
            std::printf("    t %5.2f s : %6.1f m, écart %+5.2f, cap %+5.2f rad, volant %+5.2f, %3.0f km/h\n", n / 120.0,
                        (p.dist - s.debutU()) * M, p.playerX, p.heading, p.steer, p.kmh());
    }
    return m;
}

static const char* nomAide(Aide a) { return a == Aide::Web ? "jeu web" : a == Aide::Activee ? "activée" : "désactivée"; }

int main() {
    wfs::trace::forcerRondsPoints(true);   // WFS-283 RG-06 : mesures de conduite sur les anciens ronds-points (interrupteur, code gardé)
    const char* cartes[] = {"circuit", "campagne", "ville", "plage"};
    int nSections = 0;
    for (const char* id : cartes) {
        static Track t; t.build(mapById(id), trace::versionCarte(id));
        for (const trace::Section& s : trace::sections(t)) {
            if (s.type != trace::Type::Epingle && s.type != trace::Type::Lacets && s.type != trace::Type::RondPoint) continue;
            nSections++;
            std::printf("%s : %s (rayon %.0f m, %.0f m de long)\n", id, trace::nomType(s.type), s.rayon, (s.finU() - s.debutU()) * M);
            for (const Joueur& j : JOUEURS) {
                g_retard = j.retard; g_fleches = j.fleches; g_vitesse = j.vitesse;
                for (Aide a : {Aide::Web, Aide::Activee, Aide::Desactivee}) {
                    if (j.vitesse > 1 && a == Aide::Desactivee) continue;   // au-delà de la vitesse normale, sans aide : physique seule
                    const Mesure m = rouler(t, s, a, true);
                    std::printf("  %-34s aide %-10s : écart max %.2f, %3d pas hors de la route, saut de cap max %.3f rad/image, %.0f à %.0f km/h\n",
                                j.nom, nomAide(a), m.ecartMax, m.sorties, m.sautMax, m.vMin, m.vMax);
                    if (a == Aide::Web) continue;   // ordinateur : inchangé (mesure de comparaison seulement)
                    VERIFIE(m.sorties == 0, "%s %s, %s, aide %s : %d pas hors de la route (écart max %.2f)", id, trace::nomType(s.type), j.nom, nomAide(a), m.sorties, m.ecartMax);
                    VERIFIE(m.sautMax <= 0.05, "%s %s, %s, aide %s : saut de cap %.3f rad par image", id, trace::nomType(s.type), j.nom, nomAide(a), m.sautMax);
                }
            }
            g_retard = 0; g_fleches = false; g_vitesse = 1;
            {   // WFS-241 (RG-02 c) : 1,4 × la vitesse normale (1,4 g : au-delà de toute adhérence), aide activée, ligne idéale :
                // la voiture élargit (physique), sans saut de cap ni mur invisible
                g_vitesse = 1.4;
                const Mesure vite = rouler(t, s, Aide::Activee, true);
                g_vitesse = 1;
                std::printf("  trop vite (1,4 × la vitesse normale), aide activée : écart max %.2f, saut de cap max %.3f rad/image\n", vite.ecartMax, vite.sautMax);
                VERIFIE(vite.sautMax <= 0.08, "%s %s : trop vite, saut de cap %.3f rad par image", id, trace::nomType(s.type), vite.sautMax);
            }
            const Mesure lache = rouler(t, s, Aide::Activee, false), libre = rouler(t, s, Aide::Desactivee, false);
            std::printf("  volant lâché : aide activée écart max %.2f (%d pas dehors) ; aide désactivée écart max %.2f\n", lache.ecartMax,
                        lache.sorties, libre.ecartMax);
            VERIFIE(lache.sorties == 0, "%s %s : volant lâché, aide activée, sortie forcée (écart %.2f)", id, trace::nomType(s.type), lache.ecartMax);
            VERIFIE(lache.sautMax <= 0.05, "%s %s : volant lâché, aide activée, saut de cap %.3f", id, trace::nomType(s.type), lache.sautMax);
            VERIFIE(libre.ecartMax > 1, "%s %s : volant lâché, aide désactivée, la voiture suit encore le virage (écart %.2f)", id,
                    trace::nomType(s.type), libre.ecartMax);
        }
    }
    VERIFIE(nSections >= 4, "%d sections serrées seulement", nSections);
    {   // trace de la trajectoire : épingle du Grand Prix et rond-point de la Ville, aide activée
        for (const char* id : {"circuit", "ville"}) {
            static Track t; t.build(mapById(id), trace::versionCarte(id));
            for (const trace::Section& s : trace::sections(t))
                if (s.type == trace::Type::Epingle || s.type == trace::Type::RondPoint) {
                    std::printf("trajectoire %s, %s, aide activée :\n", id, trace::nomType(s.type));
                    rouler(t, s, Aide::Activee, true, true);
                    break;
                }
        }
    }
    {   // WFS-90 : demi-tour depuis l'arrêt, braquage à fond, gaz sous 4 m/s
        static Track t; t.build(mapById("ville"));
        for (Aide a : {Aide::Activee, Aide::Desactivee}) {
            Player p; p.reset(carById("golf")); p.aide = a;
            int pas = 0;
            for (; pas < 120 * 30 && std::abs(p.heading) < 3.0; pas++) { DriveInput in; in.steer = 1; in.gas = p.speed * M < 4; p.update(1.0 / 120, in, t, true); }
            VERIFIE(std::abs(p.heading) >= 3.0, "demi-tour, aide %s : cap %.2f rad", nomAide(a), p.heading);
            std::printf("demi-tour depuis l'arrêt, aide %s : cap %.2f rad en %.1f s\n", nomAide(a), p.heading, pas / 120.0);
        }
    }
    {   // réglage : Activée par défaut ; Player neuf = aides du jeu web (ordinateur)
        VERIFIE(reglagesConduite().aideDirection, "aide à la direction désactivée par défaut");
        Player p; VERIFIE(p.aide == Aide::Web, "Player neuf : aide %d (Web attendu)", (int)p.aide);
        p.aide = Aide::Activee; p.reset(carById("golf")); VERIFIE(p.aide == Aide::Activee, "reset() a changé l'aide");
    }
    std::printf("aide à la direction (WFS-217) : %s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
