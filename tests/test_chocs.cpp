// WFS-256 : chocs de la voiture du joueur (conduite réaliste, coordonnées du monde) avec les obstacles du décor
// (src/game/chocs_decor.h). Sur une piste réelle (Ville, tracé 2), un obstacle est posé sur la trajectoire :
//  - poteau (cylindre de 15 cm) à 30, 80 et 150 km/h : la voiture s'arrête ou rebondit, ne le traverse jamais (RG-02) ;
//  - poteau de 10 cm à 300 km/h (8 cm parcourus… 0,7 m par pas) : test continu, pas de passage au travers ;
//  - mur (boîte) pris en biais à 30° : la voiture glisse le long, tourne, garde une partie de sa vitesse (RG-03) ;
//  - petit objet (poubelle, cylindre de 30 cm) : fixe comme son image ; la voiture s'arrête ;
//  - sans obstacle : aucune réaction (RG-05, pas de mur invisible) ; déterminisme.
// usage : test_chocs
#include <cmath>
#include <cstdio>
#include "../src/game/cars.h"
#include "../src/game/player.h"
#include "../src/game/trace_cartes.h"
#include "../src/game/track.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

struct Res { double capMax = 0, vMinApresChoc = 1e9, vAvant = 0, vApres = 0, penetration = 0, capFinal = 0, distFinale = 0; int chocs = 0; };

// la Golf part à `kmh` sur la ligne droite de départ de la Ville, sans commande ; obstacle o posé à `devant` m, latéral `lat` m
static Res essai(Track& t, double kmh, chocs::Obstacle o, double devant, double lat, bool poser = true) {
    chocs::registre().vider();
    Player p; p.reset(carById("golf")); p.aide = Aide::Desactivee;
    const double d0 = 50 / M; p.dist = d0; p.position = t.wrap(d0); p.speed = kmh / 3.6 / M;
    const TrackPoint P = t.pointLisse(d0 + devant / M, lat);
    o.x = P.x; o.z = P.z; if (o.forme == chocs::Forme::Boite) o.angle += P.th;
    if (poser) chocs::registre().ajouter(o);
    Res r; r.vAvant = kmh;
    for (int n = 0; n < 120 * 6; n++) {
        const int n0 = p.chocsDecor.n;
        p.update(1.0 / 120, DriveInput{}, t, true);
        if (p.chocsDecor.n > 0) { r.capMax = std::max(r.capMax, std::abs(p.heading)); if (p.chocsDecor.n > n0) r.vMinApresChoc = std::min(r.vMinApresChoc, std::abs(p.kmh())); }
        // pénétration : distance du centre de l'obstacle au rectangle de la voiture (négative si dedans)
        if (poser) {
            const chocs::Contact c = chocs::contact(p.monde.x, p.monde.z, p.monde.cap, p.car->d.L / 2, p.car->d.W / 2, o);
            if (c.touche) r.penetration = std::max(r.penetration, c.prof);
        }
    }
    r.vApres = std::abs(p.kmh()); r.chocs = p.chocsDecor.n; r.capFinal = p.heading; r.distFinale = (p.dist - d0) * M;
    chocs::registre().vider();
    return r;
}

int main() {
    static Track t; t.build(mapById("ville"), trace::versionCarte("ville"));
    chocs::Obstacle poteau; poteau.forme = chocs::Forme::Cylindre; poteau.r = 0.15; poteau.genre = chocs::Genre::Poteau;
    for (double kmh : {30.0, 80.0, 150.0}) {
        const Res r = essai(t, kmh, poteau, 40, 0.3);
        std::printf("poteau à %3.0f km/h : %d choc(s), pénétration max %.3f m, %.0f km/h après, %.1f m parcourus, cap %+.2f rad\n", kmh, r.chocs,
                    r.penetration, r.vApres, r.distFinale, r.capFinal);
        VERIFIE(r.chocs >= 1, "poteau à %.0f km/h : pas de choc", kmh);
        VERIFIE(r.distFinale < 40 && r.distFinale > 30, "poteau à %.0f km/h : passée au travers ou renvoyée trop loin (%.1f m)", kmh, r.distFinale);
        VERIFIE(r.penetration < 0.06, "poteau à %.0f km/h : enfoncé de %.3f m", kmh, r.penetration);
    }
    {   // test continu : poteau fin, 300 km/h
        chocs::Obstacle fin = poteau; fin.r = 0.10;
        const Res r = essai(t, 300, fin, 60, 0);
        std::printf("poteau de 10 cm à 300 km/h : %d choc(s), %.1f m parcourus\n", r.chocs, r.distFinale);
        VERIFIE(r.chocs >= 1 && r.distFinale < 60 && r.distFinale > 50, "poteau fin à 300 km/h traversé ou rebond démesuré (%.1f m)", r.distFinale);
    }
    {   // mur en biais (30°) sur le côté droit de la trajectoire
        chocs::Obstacle mur; mur.forme = chocs::Forme::Boite; mur.hx = 0.25; mur.hz = 15; mur.angle = 0.52; mur.genre = chocs::Genre::Mur;
        const Res r = essai(t, 80, mur, 30, 1.0);
        std::printf("mur à 30° à 80 km/h : %d choc(s), vitesse au plus bas pendant les chocs %.0f km/h, cap max %.2f rad, pénétration %.3f m\n", r.chocs,
                    r.vMinApresChoc, r.capMax, r.penetration);
        VERIFIE(r.chocs >= 1, "mur en biais : pas de choc");
        VERIFIE(r.penetration < 0.06, "mur en biais : enfoncé de %.3f m", r.penetration);
        VERIFIE(r.capMax > 0.3 && r.capMax < 0.8, "mur en biais : cap max %.2f rad (doit s'aligner sur le mur, 0,52 rad, sans tourner sur elle-même)", r.capMax);
        VERIFIE(r.vMinApresChoc > 30, "mur en biais : la voiture s'arrête net au lieu de glisser le long (%.0f km/h)", r.vMinApresChoc);
    }
    {   // poubelle (fixe, comme son image) : la voiture s'arrête contre, ne la traverse pas
        chocs::Obstacle poub = poteau; poub.r = 0.3; poub.genre = chocs::Genre::Mobilier;
        const Res r = essai(t, 50, poub, 30, 0);
        std::printf("poubelle à 50 km/h : %d choc(s), %.0f km/h après, %.1f m parcourus\n", r.chocs, r.vApres, r.distFinale);
        VERIFIE(r.chocs >= 1 && r.distFinale < 30, "poubelle : traversée (%.1f m)", r.distFinale);
    }
    {   // pas de mur invisible ; déterminisme
        const Res r = essai(t, 80, poteau, 40, 0.3, false);
        VERIFIE(r.chocs == 0, "sans obstacle : %d choc(s) (mur invisible)", r.chocs);
        const Res a = essai(t, 80, poteau, 40, 0.3), b = essai(t, 80, poteau, 40, 0.3);
        VERIFIE(a.vApres == b.vApres && a.capFinal == b.capFinal && a.distFinale == b.distFinale, "choc non déterministe");
    }
    std::printf("chocs avec le décor (WFS-256) : %s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
