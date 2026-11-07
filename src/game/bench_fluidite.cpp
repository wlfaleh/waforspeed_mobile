// WaForSpeed — WFS-221 (RG-01, RG-03, RG-08) : banc « fluidite » (WFS_BANC=fluidite) : à-coups de la voiture du joueur vue
// par la caméra, avec un temps d'image IRRÉGULIER comme sur le téléphone, sans dessin (calcul de la vue seul : rapide).
//
// Scénarios : ligne droite (la plus longue section droite des cartes), virage serré, épingle, montée (première section de ce type sur les cartes
// Ville, Campagne, Corniche, Grand Prix) ; vues Proche et Poursuite du jeu (camera_vue.cpp : choisirVue) ; 30, 60 et 90 images/s, temps d'image
// tiré à ± 30 % autour de la moyenne (graine fixe : 60 img/s → 11,7 à 21,7 ms, comme le téléphone) ; 14 s chacun, les 4
// premières secondes ignorées (mise en vitesse) ; conduite à la manette, douce (gaz, braquage à faible gain), à la vitesse où la
// voiture tient la courbe (150 km/h en ligne droite et en montée, 90 en virage serré, 55 en épingle). Deux passes : « avant » (rendu du dernier pas, pente par segment : ce
// que faisait le jeu) et « après » (rendu interpolé, pente lissée : WFS-221 RG-02).
// Par image : position et cap / tangage de la voiture dans le repère de la caméra, nombre de pas de simulation.
// Indicateurs (écart au mouvement régulier : cubique passant par les deux images d'avant et les deux d'après, dans le temps ;
// nul pour un mouvement régulier, virage compris, quel que soit le rythme des images ; images de choc exclues : secousse voulue) :
//   - à-coup de position (mm) ;
//   - saut d'angle (°) : même calcul pour le cap et le tangage vus par la caméra (RG-03 : ≤ 0,2°) ;
//   - RG-08 : distance voiture ↔ caméra le long de la voiture : écart au mouvement régulier (un aller-retour « en une fraction
//     de seconde ») ; objectif : aucun > 2 cm. Un vrai ralentissement (freinage) est régulier et ne compte pas.
// Rapport : <sortie>/fluidite.txt ; code de retour = nombre d'objectifs manqués par la passe « après ».
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "game.h"
#include "trace_cartes.h"
#include "../core/log.h"
#include "camera_vue.h"
#include "course_system.h"

namespace wfs {

namespace {

struct Echantillon { double t; vec3 p; double cap, tangage, distCam, vitesse = 0; bool choc = false; vec3 voiture{}, oeil{}; double x = 0, cap0 = 0; };

struct Resultat { double acoupMm = 0, sautDeg = 0, allerRetourCm = 0; int allersRetours = 0; int pas[4] = {}; int images = 0; };

double angleWrap(double a) { while (a > PI) a -= 2 * PI; while (a < -PI) a += 2 * PI; return a; }

// valeur attendue à l'image i d'un mouvement régulier : cubique (Lagrange) passant par les images i−2, i−1, i+1, i+2 ; nulle
// pour tout mouvement à accélération régulière (virage, freinage) ; un saut d'un pas de simulation (0, 1 ou 2 pas par image)
// laisse un écart de l'ordre de v × 1/120 s
template <class F> double attendu(const std::vector<Echantillon>& e, size_t i, F val) {
    const size_t k[4] = {i - 2, i - 1, i + 1, i + 2};
    double r = 0;
    for (int a = 0; a < 4; a++) {
        double l = 1;
        for (int b = 0; b < 4; b++) if (b != a) l *= (e[i].t - e[k[b]].t) / (e[k[a]].t - e[k[b]].t);
        r += l * val(e[k[a]]);
    }
    return r;
}

Resultat analyser(const std::vector<Echantillon>& e, const int pas[4]) {
    Resultat r;
    for (int k = 0; k < 4; k++) r.pas[k] = pas[k];
    r.images = (int)e.size();
    for (size_t i = 2; i + 2 < e.size(); i++) {
        if (e[i].choc) continue;   // choc : secousse voulue de la caméra (WFS-70)
        const vec3 lisse{(float)attendu(e, i, [](const Echantillon& x) { return (double)x.p.x; }), (float)attendu(e, i, [](const Echantillon& x) { return (double)x.p.y; }),
                         (float)attendu(e, i, [](const Echantillon& x) { return (double)x.p.z; })};
        r.acoupMm = std::max(r.acoupMm, (double)length(e[i].p - lisse) * 1000);
        const double c0 = e[i].cap;   // cap déroulé autour de l'image i
        const double cap = attendu(e, i, [c0](const Echantillon& x) { return c0 + angleWrap(x.cap - c0); });
        const double tg = attendu(e, i, [](const Echantillon& x) { return x.tangage; });
        r.sautDeg = std::max({r.sautDeg, std::abs(c0 - cap) * 180 / PI, std::abs(e[i].tangage - tg) * 180 / PI});
        // RG-08 : « la voiture avance, recule, avance en une fraction de seconde » : écart de la distance voiture ↔ caméra (le
        // long de la voiture) au mouvement régulier ; un écart > 2 cm = un aller-retour visible
        const double ecart = std::abs(e[i].distCam - attendu(e, i, [](const Echantillon& x) { return x.distCam; })) * 100;
        r.allerRetourCm = std::max(r.allerRetourCm, ecart);
        if (ecart > 2) r.allersRetours++;
    }
    return r;
}

} // namespace

int benchFluidite(Game& g, Renderer& r, const std::string& out) {
    // vitesse visée : celle où la voiture tient la courbe (au-delà, elle tape la barrière : un vrai choc du jeu, pas un à-coup du rendu)
    struct Scenario { const char* nom; trace::Type type; double kmh; };
    const Scenario SCEN[] = {{"ligne-droite", trace::Type::Ligne, 150}, {"virage-serre", trace::Type::Virage, 90}, {"epingle", trace::Type::Epingle, 55}, {"montee", trace::Type::Montee, 150}};
    const char* CARTES[] = {"ville", "campagne", "plage", "circuit"};
    std::ostringstream rap;
    rap << "WaForSpeed — banc fluidite (WFS-221) : voiture du joueur vue par la caméra, temps d'image irrégulier (± 30 %)\n"
           "colonnes : passe | scénario | carte | vue | img/s | images | pas par image (0/1/2/3) | à-coup max (mm) | saut d'angle max (°) | aller-retour max (cm) | allers-retours > 2 cm par s\n";
    int manques = 0;
    double pireApres[3] = {0, 0, 0};
    for (const Scenario& sc : SCEN) {
        // première section du type sur les cartes, dans l'ordre (la ligne droite : départ de la Ville)
        const char* carte = nullptr; int64_t seg = 0, ligneDebut = 0;
        for (const char* c : CARTES) {
            Track t; t.build(mapById(c), trace::versionCarte(c));
            if (sc.type == trace::Type::Ligne) {   // la plus longue section droite (ligne, autoroute) des 4 cartes
                for (const trace::Section& s : trace::sections(t))
                    if ((s.type == trace::Type::Ligne || s.type == trace::Type::Autoroute) && (!carte || s.fin - s.debut > seg)) { carte = c; seg = s.fin - s.debut; ligneDebut = s.debut; }
                continue;
            }
            for (const trace::Section& s : trace::sections(t)) if (s.type == sc.type) { carte = c; seg = std::max<int64_t>(0, s.debut - (int64_t)(120 / SEG_M)); break; }
            if (carte) break;
        }
        if (sc.type == trace::Type::Ligne && carte) seg = ligneDebut;
        if (!carte) { rap << "scénario " << sc.nom << " : aucune section de ce type\n"; continue; }
        if (const char* seul = std::getenv("WFS_FLUIDITE_SEUL"); seul && std::string(seul) != sc.nom) continue;   // essai local
        for (int passe = 0; passe < 2; passe++)
            for (int cam = 0; cam < 2; cam++)
                for (int fps : {30, 60, 90}) {
                    Game::interpolation = passe == 1; Track::penteLissee = passe == 1; g_cameraFiltreExact = passe == 1;
                    setenv("WFS_DEPART", std::to_string(seg).c_str(), 1);
                    courseBenchRules(true);
                    const bool ok = g.init(r, "golf", carte);
                    unsetenv("WFS_DEPART");
                    if (!ok) { WFS_ERR("fluidite : chargement de %s impossible", carte); manques++; continue; }
                    g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{}; g.camMode = 0; choisirVue(cam ? VueCamera::Poursuite : VueCamera::Proche); g.allerA(Ecran::Course);
                    Mulberry32 R(221u * 31u + (uint32_t)fps);
                    double t = 1000;
                    for (int i = 0; i < 60 * 4; i++) { g.manette = DriveInput{}; g.update(1.0 / 60); t += 1.0 / 60; g.calculerVue(t); }   // décompte (4 s)
                    std::vector<Echantillon> ech; int pas[4] = {}; double braquage = 0;
                    const double moy = 1.0 / fps;
                    for (double duree = 0; duree < 14; ) {
                        const double dt = moy * (0.7 + 0.6 * R.next());
                        duree += dt; t += dt;
                        // conduite douce (gain faible, comme un joueur) : le pilote automatique du jeu braque fort à chaque image
                        // (gain 4 sur le cap) et rend la trajectoire elle-même irrégulière au rythme des images : il mesurerait
                        // sa conduite, pas le rendu
                        const Player& pj = g.player();
                        g.manette.gas = pj.kmh() < sc.kmh; g.manette.brake = pj.kmh() > sc.kmh + 15;
                        const double vise = std::clamp(-pj.playerX * 0.8 - pj.heading * 1.2, -0.6, 0.6);
                        braquage += (vise - braquage) * (1 - std::exp(-dt / 0.3));   // pouce : braquage lissé (0,3 s)
                        g.manette.steer = (float)braquage;
                        g.update(dt);
                        const Game::Vue v = g.calculerVue(t);
                        if (duree < 4) continue;   // mise en vitesse (≈ 150 km/h) et caméra posée
                        pas[std::min(g.pasDerniereImage(), 3)]++;
                        const mat4 V = lookAt(v.eye, v.look, {0, 1, 0});
                        const vec3 pc = V.transformPoint(v.carPos), fc = normalize(V.transformDir(v.fwd));
                        const vec3 h = normalize(vec3{v.fwd.x, 0, v.fwd.z});
                        ech.push_back({t, pc, std::atan2(fc.x, -fc.z), std::asin(std::clamp(fc.y, -1.f, 1.f)), (double)dot(v.carPos - v.eye, h), g.player().kmh(), g.player().controlLock > 0 || g.player().playerSpin != 0, v.carPos, v.eye, g.player().playerX, g.player().heading});
                    }
                    g.shutdown();
                    if (const char* tr = std::getenv("WFS_FLUIDITE_TRACE")) {   // essai local : série image par image
                        std::ofstream f(out + "/trace-" + (passe ? "apres-" : "avant-") + sc.nom + "-" + (cam ? "poursuite-" : "proche-") + std::to_string(fps) + ".csv");
                        f.precision(10);
                        for (const Echantillon& x : ech) f << x.t << ' ' << x.p.x << ' ' << x.p.y << ' ' << x.p.z << ' ' << x.cap << ' ' << x.tangage << ' ' << x.distCam << ' ' << x.vitesse << ' ' << x.choc << ' ' << x.voiture.x << ' ' << x.voiture.y << ' ' << x.voiture.z << ' ' << x.oeil.x << ' ' << x.oeil.y << ' ' << x.oeil.z << ' ' << x.x << ' ' << x.cap0 << '\n';
                        (void)tr;
                    }
                    const Resultat res = analyser(ech, pas);
                    const double parS = res.allersRetours / 10.0;
                    char l[320];
                    std::snprintf(l, sizeof l, "%s | %s | %s | %s | %d | %d | %d/%d/%d/%d | %.2f | %.3f | %.2f | %.1f\n", passe ? "après" : "avant", sc.nom, carte,
                                  cam ? "Poursuite" : "Proche", fps, res.images, res.pas[0], res.pas[1], res.pas[2], res.pas[3], res.acoupMm, res.sautDeg, res.allerRetourCm, parS);
                    rap << l;
                    WFS_LOG("fluidite : %s", l);
                    if (passe == 1 && (sc.type == trace::Type::Ligne || sc.type == trace::Type::Montee)) {
                        pireApres[0] = std::max(pireApres[0], res.acoupMm); pireApres[1] = std::max(pireApres[1], res.sautDeg); pireApres[2] = std::max(pireApres[2], res.allerRetourCm);
                        // objectifs (RG-03, RG-08) comptés en ligne droite et en montée : en courbe, la trajectoire PHYSIQUE elle-même
                        // n'est pas lisse (cap qui suit la courbure de chaque segment de 1,2 m, consigne de vitesse en tout ou rien) ;
                        // la physique ne change pas (RG-02) : valeurs données pour information
                        const bool droite = sc.type == trace::Type::Ligne || sc.type == trace::Type::Montee;
                        if (droite && (res.acoupMm >= 5 || res.sautDeg > 0.2 || res.allerRetourCm > 2)) manques++;
                    }
                }
    }
    // RG-04 : 12 images de suite (60 img/s irrégulières), ligne droite, vues Proche et Poursuite, avant puis après :
    // fluidite-<avant|apres>-<proche|poursuite>-NN.png (la voiture doit rester à la même place d'une image à l'autre)
    {
        const char* carte = nullptr; int64_t seg = 0, lg = 0;
        for (const char* c : CARTES) {
            Track t; t.build(mapById(c), trace::versionCarte(c));
            for (const trace::Section& x : trace::sections(t))
                if ((x.type == trace::Type::Ligne || x.type == trace::Type::Autoroute) && x.fin - x.debut > lg) { carte = c; seg = x.debut; lg = x.fin - x.debut; }
        }
        for (int passe = 0; passe < 2 && carte; passe++)
            for (int cam = 0; cam < 2; cam++) {
                Game::interpolation = passe == 1; Track::penteLissee = passe == 1; g_cameraFiltreExact = passe == 1;
                setenv("WFS_DEPART", std::to_string(seg).c_str(), 1);
                courseBenchRules(true);
                const bool ok = g.init(r, "golf", carte);
                unsetenv("WFS_DEPART");
                if (!ok) continue;
                g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{}; g.camMode = 0;
                choisirVue(cam ? VueCamera::Poursuite : VueCamera::Proche); g.allerA(Ecran::Course);
                Mulberry32 R(4221u);
                double t = 2000;
                for (int i = 0; i < 60 * 4; i++) { g.manette = DriveInput{}; g.update(1.0 / 60); t += 1.0 / 60; g.calculerVue(t); }
                for (int i = 0; i < 60 * 5 + 12; i++) {
                    const double dt = (1.0 / 60) * (0.7 + 0.6 * R.next());
                    t += dt;
                    g.manette.gas = g.player().kmh() < 150; g.manette.steer = 0;
                    g.update(dt);
                    if (i < 60 * 5) { g.calculerVue(t); continue; }
                    g.render(t);
                    char nom[96]; std::snprintf(nom, sizeof nom, "/fluidite-%s-%s-%02d.png", passe ? "apres" : "avant", cam ? "poursuite" : "proche", i - 60 * 5);
                    r.capturePng(out + nom);
                }
                g.shutdown();
            }
    }
    g.modeManette = false; g.manette = DriveInput{};
    Game::interpolation = true; Track::penteLissee = true; g_cameraFiltreExact = true; choisirVue(VueCamera::Poursuite);
    char f[200];
    std::snprintf(f, sizeof f, "après, ligne droite et montée, pire cas : à-coup %.2f mm (objectif < 5), saut d'angle %.3f° (≤ 0,2), aller-retour %.2f cm (≤ 2) ; objectifs manqués : %d\n",
                  pireApres[0], pireApres[1], pireApres[2], manques);
    rap << f;
    std::ofstream(out + "/fluidite.txt") << rap.str();
    WFS_LOG("fluidite : %s", f);
    return manques;
}

} // namespace wfs
