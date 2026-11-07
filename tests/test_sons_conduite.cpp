// WFS-255 : sons et vibrations liés aux vraies forces (étape 3a du chantier « Conduite réaliste »). La physique de la voiture
// (conduite_monde.h) roule ; les calculs purs du son (sons_conduite.h), des vibrations (ressenti.h) et du moteur
// (EngineModel::updateReel) sont vérifiés à chaque pas :
//  - RG-01 : virage à 0,5 g (sous le pic) : aucun crissement ; volant à fond (au-delà du pic) : crissement, montée sans saut ;
//  - RG-02 : freinage ABS : pulsation à 12,5 Hz entendue ; vibration ABS toutes les 0,5 s, plus rien après 2 s (RG-04) ;
//    sans ABS : roues bloquées, crissement plein et continu ;
//  - RG-03 : son du moteur au régime réel (± 5 %), rapport réel, un à-coup de vibration par rapport passé au 0-100 ;
//  - RG-04 : glisse continue de 5 s : vibrations dans les 2 premières secondes seulement ;
//  - déterminisme : deux fois le même essai, mêmes résultats au bit près.
// usage : test_sons_conduite
#include <cmath>
#include <cstdio>
#include "../src/audio/engine_synth.h"
#include "../src/audio/sons_conduite.h"
#include "../src/game/conduite_monde.h"
#include "../src/game/ressenti.h"
using namespace wfs;
using namespace wfs::physique;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)
static constexpr double DT = 1.0 / 120;

struct Bilan { double premiereVib = -1, gainMax = 0, sautMax = 0, somme = 0; int vibAbs = 0, vibGlisse = 0, vibRapport = 0, montees = 0, fronts = 0; double derniereVib = -1; };

// virage à vitesse tenue avec un angle de roues fixe pendant `duree` s
static Bilan virage(const Params& p, double v, double braquage, double duree) {
    Etat e; e.vz = -v; Commande c; c.braquage = braquage; Sol s;
    sonsconduite::Calcul son; VibrationsConduite vib; Bilan b; float g0 = 0;
    for (double t = 0; t < duree; t += DT) {
        c.gaz = e.u() < v ? std::clamp((v - e.u()) * 2, 0.0, 1.0) : 0;
        pas(p, e, c, s, DT);
        sonsconduite::Entree in; in.vms = e.u(); in.sigmaAv = e.sigmaAv; in.sigmaAr = e.sigmaAr; in.abs = e.absActif;
        const float g = son.crissement(DT, in);
        b.gainMax = std::max(b.gainMax, (double)g); b.sautMax = std::max(b.sautMax, (double)std::abs(g - g0)); g0 = g; b.somme += g;
        const auto m = vib.update(DT, e.u(), std::max(e.sigmaAv, e.sigmaAr), e.absActif, e.rapport, c.gaz > 0);
        if (m == VibrationsConduite::Motif::Glisse) { b.vibGlisse++; b.derniereVib = t; if (b.premiereVib < 0) b.premiereVib = t; }
    }
    return b;
}

// freinage à fond de 100 km/h à l'arrêt, avec ou sans ABS
static Bilan freinage(const Params& p, bool abs) {
    Etat e; e.vz = -100 / 3.6; Commande c; c.frein = 1; c.abs = abs; Sol s;
    sonsconduite::Calcul son; VibrationsConduite vib; Bilan b; bool haut = false;
    double t = 0;
    for (; e.u() > 0.05 && t < 10; t += DT) {
        const double u0 = e.u();
        pas(p, e, c, s, DT);
        sonsconduite::Entree in; in.vms = e.u(); in.sigmaAv = e.sigmaAv; in.sigmaAr = e.sigmaAr; in.abs = e.absActif;
        in.freine = true; in.decel = (u0 - e.u()) / DT;
        const float g = son.crissement(DT, in);
        b.gainMax = std::max(b.gainMax, (double)g); b.somme += g;
        const bool h = g > 0.1f && e.u() > 10;   // fronts montants de la pulsation (au-dessus de 36 km/h : volume plein)
        if (h && !haut) b.fronts++;
        haut = h;
        const auto m = vib.update(DT, e.u(), std::max(e.sigmaAv, e.sigmaAr), e.absActif, e.rapport, false);
        if (m == VibrationsConduite::Motif::Abs) { b.vibAbs++; b.derniereVib = t; }
    }
    b.montees = (int)std::lround(t * 1000);   // durée de l'arrêt (ms), pour le journal
    return b;
}

// 0-100 km/h plein gaz : son du moteur au régime réel, à-coups de rapport
static Bilan zeroCent(const Params& p, double& ecartRegime, int& rapportFaux) {
    Etat e; Commande c; c.gaz = 1; Sol s;
    EngineModel mot; EngineTargets cib; VibrationsConduite vib; Bilan b;
    const double coupure = p.wMax * 60 / (2 * 3.14159265358979);
    int r0 = 1; ecartRegime = 0; rapportFaux = 0; double depuis = 0;
    for (double t = 0; e.u() < 100 / 3.6 && t < 30; t += DT) {
        pas(p, e, c, s, DT);
        mot.updateReel(DT, e.regime, coupure, e.rapport, e.u() / 0.006, 72 / 0.006, true, false, true, 1.f, false, cib);
        if (e.rapport != mot.gear) rapportFaux++;
        if (e.rapport > r0) { b.montees++; depuis = 0; } else depuis += DT;
        if (t > 0.5 && depuis > 0.15) ecartRegime = std::max(ecartRegime, std::abs(mot.rpm - e.regime / coupure));   // hors passage
        r0 = e.rapport;
        if (vib.update(DT, e.u(), std::max(e.sigmaAv, e.sigmaAr), e.absActif, e.rapport, true) == VibrationsConduite::Motif::Rapport) b.vibRapport++;
        b.somme += mot.rpm;
    }
    return b;
}

int main() {
    for (const char* id : {"golf", "mustang"}) {
        const Params& p = params(carById(id));
        const double v = 80 / 3.6;
        {   // RG-01 : sous le pic (0,5 g) : silence ; au-delà (volant à fond) : crissement sans saut
            const Bilan sous = virage(p, v, p.L * 0.5 * G / (v * v) * 1.5, 4);
            const Bilan sur = virage(p, v, braquageMax(p, v), 4);
            std::printf("%s, virage à 80 km/h : 0,5 g → crissement max %.3f ; volant à fond → %.3f (saut max %.4f par pas), %d vibration(s) de glisse\n",
                        id, sous.gainMax, sur.gainMax, sur.sautMax, sur.vibGlisse);
            VERIFIE(sous.gainMax == 0, "%s : crissement sous le pic du pneu (%.3f)", id, sous.gainMax);
            VERIFIE(sur.gainMax > 0.1, "%s : pas de crissement au-delà du pic (%.3f)", id, sur.gainMax);
            VERIFIE(sur.sautMax < 0.05, "%s : crissement qui saute (%.3f par pas)", id, sur.sautMax);
            VERIFIE(sur.vibGlisse >= 1 && sur.derniereVib - sur.premiereVib <= 2.0, "%s : vibrations de glisse %d, de %.2f à %.2f s (RG-04 : 2 s au plus)", id,
                    sur.vibGlisse, sur.premiereVib, sur.derniereVib);
            const Bilan longue = virage(p, v, braquageMax(p, v), 6);
            VERIFIE(longue.derniereVib - longue.premiereVib <= 2.0, "%s : glisse de 6 s, vibrations de %.2f à %.2f s (RG-04)", id, longue.premiereVib, longue.derniereVib);
        }
        {   // RG-02 : ABS : pulsation et vibrations limitées à 2 s ; sans ABS : crissement plein
            const Bilan a = freinage(p, true), b = freinage(p, false);
            const double hz = a.fronts / std::max(0.1, std::min(a.montees / 1000.0, 2.0));   // fronts comptés au-dessus de 36 km/h
            std::printf("%s, freinage 100 → 0 : ABS → %d pulsation(s) entendue(s), %d vibration(s) ABS (dernière à %.2f s), arrêt en %.2f s ; sans ABS → crissement max %.3f\n",
                        id, a.fronts, a.vibAbs, a.derniereVib, a.montees / 1000.0, b.gainMax);
            VERIFIE(a.fronts >= 10 && hz > 5, "%s : pulsation de l'ABS non entendue (%d fronts)", id, a.fronts);
            VERIFIE(a.vibAbs >= 3 && a.derniereVib <= 2.05, "%s : vibrations ABS %d, dernière à %.2f s (RG-04 : 2 s au plus)", id, a.vibAbs, a.derniereVib);
            VERIFIE(b.gainMax >= sonsconduite::GAIN_MAX * 0.99, "%s : roues bloquées sans crissement plein (%.3f)", id, b.gainMax);
            VERIFIE(a.gainMax < b.gainMax, "%s : ABS aussi bruyant que les roues bloquées", id);
        }
        {   // RG-03 : moteur au régime réel, à-coup à chaque rapport
            double ecart = 0; int faux = 0;
            const Bilan z = zeroCent(p, ecart, faux);
            std::printf("%s, 0-100 : %d rapport(s) passé(s), %d à-coup(s) de vibration, écart régime son / modèle %.3f, rapport faux %d pas\n",
                        id, z.montees, z.vibRapport, ecart, faux);
            VERIFIE(z.montees >= 1 && z.vibRapport == z.montees, "%s : %d rapports passés, %d à-coups", id, z.montees, z.vibRapport);
            VERIFIE(ecart < 0.05, "%s : son du moteur loin du régime réel (%.3f)", id, ecart);
            VERIFIE(faux == 0, "%s : rapport du son différent du modèle (%d pas)", id, faux);
            double e2 = 0; int f2 = 0;
            VERIFIE(zeroCent(p, e2, f2).somme == z.somme, "%s : son du moteur non déterministe", id);
        }
        VERIFIE(freinage(p, true).somme == freinage(p, true).somme, "%s : sons du freinage non déterministes", id);
    }
    std::printf("sons et vibrations de la conduite (WFS-255) : %s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
