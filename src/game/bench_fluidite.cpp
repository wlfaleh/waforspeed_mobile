// WaForSpeed — WFS-221 (RG-01, RG-03, RG-08) : banc « fluidite » (WFS_BANC=fluidite) : à-coups de la voiture du joueur vue
// par la caméra, avec un temps d'image IRRÉGULIER comme sur le téléphone, sans dessin (calcul de la vue seul : rapide).
//
// Scénarios : ligne droite (la plus longue section droite des cartes), virage serré, épingle, montée (première section de ce type sur les cartes
// Ville, Campagne, Corniche, Grand Prix) ; vues Proche et Poursuite du jeu (camera_vue.cpp : choisirVue) ; 10, 15, 20, 26 (téléphone, WFS-247), 30, 60 et 90 images/s, temps d'image
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
#include "fluidite_reference.h"   // WFS-293 (RG-07)

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
    // WFS-293 (RG-02) : carrefour de la Ville (virage de rue de WFS-283, à la place du rond-point), traversé à 50 km/h
    struct Scenario { const char* nom; trace::Type type; double kmh; bool carrefour = false; };
    const Scenario SCEN[] = {{"ligne-droite", trace::Type::Ligne, 150}, {"virage-serre", trace::Type::Virage, 90}, {"epingle", trace::Type::Epingle, 55}, {"montee", trace::Type::Montee, 150},
                             {"carrefour", trace::Type::Virage, 50, true}};
    const char* CARTES[] = {"ville", "campagne", "plage", "circuit"};
    std::ostringstream rap;
    rap << "WaForSpeed — banc fluidite (WFS-221) : voiture du joueur vue par la caméra, temps d'image irrégulier (± 30 %)\n"
           "colonnes : passe | scénario | carte | vue | cadence | img/s | images | pas par image (0/1/2/3) | à-coup max (mm, WFS-247) | écart à la référence 120 Hz (mm, juge, WFS-293) | saut d'angle max (°) | aller-retour max (cm) | allers-retours > 2 cm par s\n";
    int manques = 0;
    double pireApres[4] = {0, 0, 0, 0};
    // WFS-293 (captures du lead dev) : suites de 5 images successives (la voiture doit rester à la même place dans l'image) ;
    // WFS_FLUIDITE_IMAGES=1 : seulement ces suites (fluidite-seq-<nom>-0 à 4.png), sans les mesures
    if (std::getenv("WFS_FLUIDITE_IMAGES")) {
        struct Seq { const char* nom; const char* carte; trace::Type type; bool carrefour; double kmh; int fps; bool poursuite; };
        const Seq SEQ[] = {{"virage-serre-ville-15ips-poursuite", "ville", trace::Type::Virage, false, 90, 15, true},
                           // WFS-379 : la Corniche n'a plus de lacets ; la suite « epingle-corniche » est remplacée par la nouvelle montée
                           // (1re section en montée de la carte, 15 m après son début), même vue Proche et même cadence
                           {"montee-corniche-30ips-proche", "plage", trace::Type::Montee, false, 55, 30, false},
                           {"carrefour-ville-30ips-poursuite", "ville", trace::Type::Virage, true, 50, 30, true}};
        for (const Seq& q : SEQ) {
            Track tq; tq.build(mapById(q.carte), trace::versionCarte(q.carte));
            int64_t sq = -1, dedans = 0;
            for (const trace::Section& x : trace::sections(tq))
                if (x.type == q.type && x.exRondPoint == q.carrefour) {
                    // revue du lead dev (PR #310) : capture au segment de plus forte courbure de la section (les lacets de la Corniche
                    // commencent par une partie droite : 15 m après le début, la voiture n'était pas encore dans l'épingle)
                    int64_t pic = x.debut; for (int64_t a = x.debut; a < x.fin; a++) if (std::abs(tq.segAt(a).curve) > std::abs(tq.segAt(pic).curve)) pic = a;
                    dedans = std::max(x.debut + (int64_t)(15 / SEG_M), pic - (int64_t)(6 / SEG_M));
                    sq = std::max<int64_t>(0, std::min(x.debut, dedans - (int64_t)(60 / SEG_M)) - (int64_t)(120 / SEG_M)); break;
                }
            if (sq < 0) { WFS_ERR("fluidite : %s : section introuvable", q.nom); manques++; continue; }
            Game::interpolation = true; Game::cameraTempsSimule = true; Track::penteLissee = true; g_cameraFiltreExact = true;
            setenv("WFS_DEPART", std::to_string(sq).c_str(), 1);
            courseBenchRules(true);
            const bool ok = g.init(r, "golf", q.carte);
            unsetenv("WFS_DEPART");
            if (!ok) { manques++; continue; }
            g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{}; g.camMode = 0; choisirVue(q.poursuite ? VueCamera::Poursuite : VueCamera::Proche); g.allerA(Ecran::Course);
            Mulberry32 R(293u + (uint32_t)q.fps);
            double t = 3000, braquage = 0, duree = 0;
            for (int i = 0; i < 60 * 4; i++) { g.manette = DriveInput{}; g.update(1.0 / 60); t += 1.0 / 60; g.calculerVue(t); }
            int n = 0;
            while (n < 5 && duree < 40) {   // conduite du banc (douce) ; 5 images rendues de suite une fois 15 m dans la courbe
                const double dt = (1.0 / q.fps) * (0.7 + 0.6 * R.next());
                duree += dt; t += dt;
                const Player& pj = g.player();
                g.manette.gas = pj.kmh() < q.kmh; g.manette.brake = pj.kmh() > q.kmh + 15;
                const double vise = std::clamp(-pj.playerX * 0.8 - pj.heading * 1.2, -0.6, 0.6);
                braquage += (vise - braquage) * (1 - std::exp(-dt / 0.3));
                g.manette.steer = (float)braquage;
                g.update(dt);
                if ((int64_t)std::floor(g.player().dist / SEG) < dedans) { g.calculerVue(t); continue; }
                g.render(t);
                r.capturePng(out + "/fluidite-seq-" + q.nom + "-" + std::to_string(n++) + ".png");
            }
            g.shutdown();
        }
        g.modeManette = false; g.manette = DriveInput{}; choisirVue(VueCamera::Poursuite);
        return manques;
    }
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
            for (const trace::Section& s : trace::sections(t))
                if (s.type == sc.type && s.exRondPoint == sc.carrefour) { carte = c; seg = std::max<int64_t>(0, s.debut - (int64_t)(120 / SEG_M)); break; }
            if (carte) break;
        }
        if (sc.type == trace::Type::Ligne && carte) seg = ligneDebut;
        if (!carte) { rap << "scénario " << sc.nom << " : aucune section de ce type\n"; continue; }
        if (const char* seul = std::getenv("WFS_FLUIDITE_SEUL"); seul && std::string(seul) != sc.nom) continue;   // essai local
        if (std::getenv("WFS_FLUIDITE_PISTE")) {   // WFS-293 : diagnostic : à-coup dû au seul tracé (point à vitesse constante, point / pointLisse, axe et 2 m)
            Track t; t.build(mapById(carte), trace::versionCarte(carte));
            for (int lisse = 0; lisse < 2; lisse++) for (double lat : {0.0, -2.0}) for (int fps : {15, 30, 60}) {
                Mulberry32 R(221u * 31u + (uint32_t)fps);
                std::vector<Echantillon> e; double tt = 0; const double v = sc.kmh / 3.6;
                for (double du = 0; du < 14;) { const double dt = (1.0 / fps) * (0.7 + 0.6 * R.next()); du += dt; tt += dt;
                    const double sm = (double)(seg + (int64_t)(120 / SEG_M)) * SEG_M + v * tt;
                    const TrackPoint q = lisse ? t.pointLisse(sm / M, lat) : t.point(sm / M, lat);
                    Echantillon x{}; x.t = tt; x.p = {(float)q.x, (float)q.y, (float)q.z}; e.push_back(x); }
                int pas[4] = {}; const Resultat rr = analyser(e, pas);
                WFS_LOG("PISTE %s %s lat %.0f %d img/s : à-coup %.2f mm", sc.nom, lisse ? "pointLisse" : "point", lat, fps, rr.acoupMm);
            }
            continue;
        }
        for (int passe = 0; passe < 2; passe++)
            for (int cam = 0; cam < 2; cam++)
                for (int regulier = 0; regulier < 2; regulier++)   // WFS-293 (RG-03) : cadences irrégulières (± 30 %) puis régulières
                for (int fps : {10, 15, 20, 26, 30, 60, 90}) {   // WFS-247 (RG-01) : cadences réelles du téléphone (10 à 26 img/s) ajoutées
                    Game::interpolation = passe == 1; Game::cameraTempsSimule = passe == 1; Track::penteLissee = passe == 1; g_cameraFiltreExact = passe == 1;
                    setenv("WFS_DEPART", std::to_string(seg).c_str(), 1);
                    courseBenchRules(true);
                    const bool ok = g.init(r, "golf", carte);
                    unsetenv("WFS_DEPART");
                    if (!ok) { WFS_ERR("fluidite : chargement de %s impossible", carte); manques++; continue; }
                    g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{}; g.camMode = 0; choisirVue(cam ? VueCamera::Poursuite : VueCamera::Proche); g.allerA(Ecran::Course);
                    Mulberry32 R(221u * 31u + (uint32_t)fps);
                    double t = 1000;
                    int64_t nPas = 0;   // WFS-293 (RG-07) : pas de simulation depuis le chargement (instant simulé de chaque image)
                    for (int i = 0; i < 60 * 4; i++) { g.manette = DriveInput{}; g.update(1.0 / 60); nPas += g.pasDerniereImage(); t += 1.0 / 60; g.calculerVue(t); }   // décompte (4 s)
                    std::vector<Echantillon> ech; int pas[4] = {}; double braquage = 0;
                    // WFS-293 (RG-07) : commandes de chaque image (pas [debut, fin)) pour rejouer la course à 120 Hz, et poses rendues
                    struct Commande { int64_t debut, fin; DriveInput in; double dist, x; };
                    std::vector<Commande> cmds; std::vector<fluidite::Pose> rendues;
                    const double moy = 1.0 / fps;
                    for (double duree = 0; duree < 14; ) {
                        const double tire = R.next(), dt = regulier ? moy : moy * (0.7 + 0.6 * tire);
                        duree += dt; t += dt;
                        // conduite douce (gain faible, comme un joueur) : le pilote automatique du jeu braque fort à chaque image
                        // (gain 4 sur le cap) et rend la trajectoire elle-même irrégulière au rythme des images : il mesurerait
                        // sa conduite, pas le rendu
                        const Player& pj = g.player();
                        g.manette.gas = pj.kmh() < sc.kmh; g.manette.brake = pj.kmh() > sc.kmh + 15;
                        const double vise = std::clamp(-pj.playerX * 0.8 - pj.heading * 1.2, -0.6, 0.6);
                        braquage += (vise - braquage) * (1 - std::exp(-dt / 0.3));   // pouce : braquage lissé (0,3 s)
                        g.manette.steer = (float)braquage;
                        const int64_t pas0 = nPas;
                        g.update(dt);
                        nPas += g.pasDerniereImage();
                        cmds.push_back({pas0, nPas, g.manette, g.player().dist, g.player().playerX});
                        const double tSim = ((double)nPas - 1 + g.alphaRendu()) / 120.0;   // état rendu : entre les pas n−1 et n
                        const Game::Vue v = g.calculerVue(t);
                        if (duree < 4) continue;   // mise en vitesse (≈ 150 km/h) et caméra posée
                        pas[std::min(g.pasDerniereImage(), 3)]++;
                        const mat4 V = lookAt(v.eye, v.look, {0, 1, 0});
                        const vec3 pc = V.transformPoint(v.carPos), fc = normalize(V.transformDir(v.fwd));
                        if (!(g.player().controlLock > 0 || g.player().playerSpin != 0)) rendues.push_back({tSim, pc});   // chocs exclus
                        const vec3 h = normalize(vec3{v.fwd.x, 0, v.fwd.z});
                        ech.push_back({t, pc, std::atan2(fc.x, -fc.z), std::asin(std::clamp(fc.y, -1.f, 1.f)), (double)dot(v.carPos - v.eye, h), g.player().kmh(), g.player().controlLock > 0 || g.player().playerSpin != 0, v.carPos, v.eye, g.player().playerX, g.player().heading});
                    }
                    g.shutdown();
                    // WFS-293 (RG-07, RG-12) : à toutes les cadences, la même course rejouée à 120 Hz (mêmes commandes à chaque pas, caméra calculée
                    // à chaque pas) : écart de chaque image rendue à cette référence au même instant simulé
                    double ecartRef = -1;
                    {   // WFS-293 (RG-12) : référence à toutes les cadences
                        setenv("WFS_DEPART", std::to_string(seg).c_str(), 1);
                        courseBenchRules(true);
                        const bool ok2 = g.init(r, "golf", carte);
                        unsetenv("WFS_DEPART");
                        if (ok2) {
                            g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{}; g.camMode = 0; choisirVue(cam ? VueCamera::Poursuite : VueCamera::Proche); g.allerA(Ecran::Course);
                            double t2 = 1000; int64_t n2 = 0;
                            for (int i = 0; i < 60 * 4; i++) { g.manette = DriveInput{}; g.update(1.0 / 60); n2 += g.pasDerniereImage(); t2 += 1.0 / 60; g.calculerVue(t2); }
                            std::vector<fluidite::Pose> ref; size_t k = 0; int rejeuLogs = 0;
                            while (!cmds.empty() && n2 < cmds.back().fin) {
                                while (k + 1 < cmds.size() && n2 >= cmds[k].fin) k++;
                                g.manette = cmds[k].in;
                                g.update(1.0 / 120); n2 += g.pasDerniereImage(); t2 += 1.0 / 120;
                                if (n2 == cmds[k].fin && std::getenv("WFS_FLUIDITE_TRACE")) {   // essai local : la simulation rejouée est-elle la même ?
                                    const double dd = (g.player().dist - cmds[k].dist) * M, dx = (g.player().playerX - cmds[k].x) * RW;
                                    if (std::abs(dd) + std::abs(dx) > 1e-4 && rejeuLogs++ < 4) { WFS_LOG("REJEU %d img/s pas %lld : écart dist %.5f m, latéral %.5f m", fps, (long long)n2, dd, dx); }
                                }
                                const double tr = ((double)n2 - 1 + g.alphaRendu()) / 120.0;
                                const Game::Vue v = g.calculerVue(t2);
                                const mat4 V = lookAt(v.eye, v.look, {0, 1, 0});
                                ref.push_back({tr, V.transformPoint(v.carPos)});
                            }
                            g.shutdown();
                            if (std::getenv("WFS_FLUIDITE_TRACE")) {   // essai local : séries rendue et de référence
                                std::ofstream fr(out + "/ref-" + (passe ? "apres-" : "avant-") + sc.nom + "-" + (cam ? "poursuite-" : "proche-") + std::to_string(fps) + ".csv"); fr.precision(10);
                                for (const fluidite::Pose& x : rendues) fr << "R " << x.t << ' ' << x.p.x << ' ' << x.p.y << ' ' << x.p.z << '\n';
                                for (const fluidite::Pose& x : ref) fr << "F " << x.t << ' ' << x.p.x << ' ' << x.p.y << ' ' << x.p.z << '\n';
                            }
                            int nc = 0;
                            ecartRef = fluidite::ecartMaxMm(rendues, ref, &nc);
                            // WFS-293 (RG-14) : la référence voit-elle un saut dans l'épingle à 30 img/s ? saut de 5 mm injecté sur une image
                            if (passe == 1 && sc.type == trace::Type::Epingle && fps == 30 && !regulier && rendues.size() > 10) {
                                std::vector<fluidite::Pose> saut = rendues;
                                saut[saut.size() / 2].p = saut[saut.size() / 2].p + vec3{0.005f, 0, 0};
                                const double vu = fluidite::ecartMaxMm(saut, ref);
                                char ls[200]; std::snprintf(ls, sizeof ls, "RG-14 : épingle, %s, 30 img/s : saut de 5 mm injecté, mesuré %.2f mm par la référence (≥ 4)\n", cam ? "Poursuite" : "Proche", vu);
                                rap << ls; WFS_LOG("fluidite : %s", ls);
                                if (vu < 4) manques++;
                            }
                            if (nc < (int)rendues.size() * 9 / 10) WFS_ERR("fluidite : référence 120 Hz incomplète (%d / %zu images)", nc, rendues.size());
                        }
                    }
                    if (const char* tr = std::getenv("WFS_FLUIDITE_TRACE")) {   // essai local : série image par image
                        std::ofstream f(out + "/trace-" + (passe ? "apres-" : "avant-") + sc.nom + "-" + (cam ? "poursuite-" : "proche-") + (regulier ? "reg-" : "") + std::to_string(fps) + ".csv");
                        f.precision(10);
                        for (const Echantillon& x : ech) f << x.t << ' ' << x.p.x << ' ' << x.p.y << ' ' << x.p.z << ' ' << x.cap << ' ' << x.tangage << ' ' << x.distCam << ' ' << x.vitesse << ' ' << x.choc << ' ' << x.voiture.x << ' ' << x.voiture.y << ' ' << x.voiture.z << ' ' << x.oeil.x << ' ' << x.oeil.y << ' ' << x.oeil.z << ' ' << x.x << ' ' << x.cap0 << '\n';
                        (void)tr;
                    }
                    const Resultat res = analyser(ech, pas);
                    const double parS = res.allersRetours / 10.0;
                    char l[320];
                    char refTxt[24]; if (ecartRef >= 0) std::snprintf(refTxt, sizeof refTxt, "%.2f", ecartRef); else std::snprintf(refTxt, sizeof refTxt, "-");
                    std::snprintf(l, sizeof l, "%s | %s | %s | %s | %s | %d | %d | %d/%d/%d/%d | %.2f | %s | %.3f | %.2f | %.1f\n", passe ? "après" : "avant", sc.nom, carte,
                                  cam ? "Poursuite" : "Proche", regulier ? "régulière" : "irrégulière", fps, res.images, res.pas[0], res.pas[1], res.pas[2], res.pas[3], res.acoupMm, refTxt, res.sautDeg, res.allerRetourCm, parS);
                    rap << l;
                    WFS_LOG("fluidite : %s", l);
                    if (passe == 1) {
                        // objectifs (RG-03, RG-08) : à-coup (30-90 img/s) et saut d'angle en ligne droite et en montée ; en courbe, la
                        // trajectoire PHYSIQUE elle-même n'est pas lisse (cap qui suit la courbure) : à-coup et angle donnés pour information.
                        // WFS-247 (ANO-139) : l'aller-retour voiture ↔ caméra (≤ 2 cm) est compté dans TOUS les scénarios, virages compris,
                        // et à toutes les cadences (10 à 90 img/s)
                        // WFS-293 (RG-01, RG-08) : à-coup compté dans TOUS les scénarios, virages et carrefour compris : à ≥ 30 img/s,
                        // indicateur de WFS-247 < 5 mm ; à 10-20 img/s, écart à la même course rejouée à 120 Hz ≤ 10 mm (RG-07) ; 26 img/s
                        // (téléphone, entre les deux) : pour information. Saut d'angle (≤ 0,2°) : ligne droite et montée, comme avant.
                        // WFS-293 (RG-12, RG-13, décision du PO du 2026-11-08 23:53 UTC) : la référence 120 Hz juge tous les scénarios à
                        // toutes les cadences (< 5 mm à ≥ 30 img/s, ≤ 10 mm à 10-20) ; l'indicateur de WFS-247 juge EN PLUS à ≥ 30 img/s en
                        // ligne droite, en montée et en virage serré (sans plancher là : il voit une irrégularité de la simulation elle-même) ;
                        // dans l'épingle et au carrefour, il a un plancher mesuré (baisse en h² à h³ avec la cadence) : pour information ;
                        // 26 img/s (téléphone, entre 20 et 30) : aucun seuil, pour information (RG-03 : seuils à 10-20 et ≥ 30 img/s)
                        const bool droite = sc.type == trace::Type::Ligne || sc.type == trace::Type::Montee;
                        const bool sansPlancher = droite || (sc.type == trace::Type::Virage && !sc.carrefour);
                        if (fps >= 30 && sansPlancher) pireApres[0] = std::max(pireApres[0], res.acoupMm);
                        if (fps >= 30 || fps <= 20) pireApres[3] = std::max(pireApres[3], ecartRef);
                        if (droite) pireApres[1] = std::max(pireApres[1], res.sautDeg);
                        pireApres[2] = std::max(pireApres[2], res.allerRetourCm);
                        const bool refManque = ecartRef < 0 || (fps >= 30 && ecartRef >= 5) || (fps <= 20 && ecartRef > 10);
                        if (refManque || (fps >= 30 && sansPlancher && res.acoupMm >= 5) || (droite && res.sautDeg > 0.2) || res.allerRetourCm > 2) manques++;
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
        for (int ips : {60, 20})   // WFS-247 (RG-04) : aussi à 20 img/s, la cadence du téléphone : fluidite-…-20ips-NN.png
        for (int passe = 0; passe < 2 && carte; passe++)
            for (int cam = 0; cam < 2; cam++) {
                Game::interpolation = passe == 1; Game::cameraTempsSimule = passe == 1; Track::penteLissee = passe == 1; g_cameraFiltreExact = passe == 1;
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
                for (int i = 0; i < ips * 5 + 12; i++) {
                    const double dt = (1.0 / ips) * (0.7 + 0.6 * R.next());
                    t += dt;
                    g.manette.gas = g.player().kmh() < 150; g.manette.steer = 0;
                    g.update(dt);
                    if (i < ips * 5) { g.calculerVue(t); continue; }
                    g.render(t);
                    char nom[96]; std::snprintf(nom, sizeof nom, "/fluidite-%s-%s%s-%02d.png", passe ? "apres" : "avant", cam ? "poursuite" : "proche", ips == 20 ? "-20ips" : "", i - ips * 5);
                    r.capturePng(out + nom);
                }
                g.shutdown();
            }
    }
    g.modeManette = false; g.manette = DriveInput{};
    Game::interpolation = true; Game::cameraTempsSimule = true; Track::penteLissee = true; g_cameraFiltreExact = true; choisirVue(VueCamera::Poursuite);
    char f[400];
    std::snprintf(f, sizeof f, "après, pire cas : à-coup WFS-247 %.2f mm (ligne droite, montée, virage serré, 30-90 img/s ; < 5), écart à la référence 120 Hz %.2f mm (tous scénarios ; < 5 à ≥ 30 img/s, ≤ 10 à 10-20), saut d'angle %.3f° (ligne droite et montée ; ≤ 0,2), aller-retour %.2f cm (tous scénarios ; ≤ 2) ; objectifs manqués : %d\n",
                  pireApres[0], pireApres[3], pireApres[1], pireApres[2], manques);
    rap << f;
    std::ofstream(out + "/fluidite.txt") << rap.str();
    WFS_LOG("fluidite : %s", f);
    return manques;
}

} // namespace wfs
