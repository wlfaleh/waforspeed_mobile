// WFS-540 : la police de la Poursuite est une vraie voiture (ordre de l'utilisateur du 2026-11-14 : « une voiture de police est une
// voiture comme les autres »). Banc CPU, Corniche (tracé 2), même enchaînement que la Poursuite (poursuite_system.cpp) : conduite
// du joueur (Player::update), police (poursuite::avancer, moteur de conduite du joueur), contacts sur le joueur (duel::contacter),
// score et arrestation (poursuite::pas).
//  (A) RG-00 / RG-08 / RG-10 : 60 s de poursuite : la police ne glisse pas de côté (vitesse latérale dans SON repère, à l'essieu
//      arrière, ≤ 0,5 m/s hors glisse des pneus, c'est-à-dire sous 0,2 g ; en virage appuyé : dérive arrière réelle, ≤ 8°), roule dans
//      l'axe de ses roues (écart cap / déplacement ≤ 3° au-dessus de 5 km/h), aucun saut de position,
//      à l'arrêt elle ne se décale pas, ≤ 208 km/h, reste sur la chaussée.
//  (B) RG-03 : joueur immobile près de la police → ARRESTATION (4 s) → « arrêté par la police ».
//  (C) RG-02 : joueur arrêté, encadré par la police (devant, à côté, derrière), GAZ appuyé → il repart en 2 s au plus.
//  (D) RG-05 : 200 chocs sans frein : jamais de « R », recul dû au choc ≤ 5 km/h, amorti en moins d'1 s.
//  (E) RG-09 : la police derrière un camion en travers garde sa file (pas d'aller-retour) et ne rentre pas dedans.
//  (F) RG-10 : le joueur recule à côté de la police : elle ne recule pas avec lui, reste sur la chaussée.
// usage : test_police_540
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "../src/game/duel.h"
#include "../src/game/player.h"
#include "../src/game/poursuite.h"
#include "../src/game/trace_cartes.h"
using namespace wfs;
using namespace wfs::poursuite;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static constexpr double DT = 1.0 / 120;
static constexpr double PI_T = 3.14159265358979323846;

// une Poursuite réduite (même enchaînement que PoursuiteSystem::collide)
struct Banc {
    Track& t;
    Player pl;
    std::vector<Police> ps;
    std::vector<Obstacle> obs;
    FichePolice f;
    Etat etat;
    const char* police = "p208";
    int chocs = 0;
    double temps = 0;
    Banc(Track& piste, const char* voiture) : t(piste) {
        pl.reset(carById(voiture)); pl.aide = Aide::Activee;
        f = fichePolice(police, voiture);
    }
    void placerJoueur(double distM, double x, double kmh) {
        pl.dist = distM / M; pl.position = t.wrap(pl.dist); pl.playerX = x; pl.speed = kmh / 3.6 / M; pl.heading = 0;
    }
    Police& ajouter(int role, double ecartM, double x, double kmh) {
        Police p; p.role = role; poser(p, pl.dist + ecartM / M, x, kmh / 3.6 / M); p.cibleX = x; p.peinture = 0xe6e8ea;
        ps.push_back(p); return ps.back();
    }
    // un pas de 1/120 s ; in : commandes du joueur
    void pas(const DriveInput& in) {
        pl.update(DT, in, t, true);
        avancer(ps, DT, 6.0 + temps, pl.dist, pl.playerX, pl.speed, obs, f, t, 1.0, pl.heading + pl.spinYaw);
        for (Police& p : ps) {
            duel::Ghost gh; gh.rd = p.rd; gh.x = p.x; gh.v = p.v * std::cos(p.h); gh.car = police; gh.hitCd = p.hitCd;
            const duel::ContactEvents e = duel::contacter(DT, pl, t, *pl.car, gh);
            if ((e.chocs || e.frottements) && p.hitCd <= 0) { p.hitCd = 1.5; chocPolice(etat); }
            if (e.chocs) chocs++;
        }
        poursuite::pas(etat, DT, plusProcheM(ps, pl.dist, pl.playerX), ecartCarrosseriesM(ps, pl.dist, pl.playerX, f), pl.kmh());
        temps += DT;
    }
};

// (A) mesures de mouvement d'une voiture de police
struct Mouvement {
    int pas = 0, latTrop = 0, capTrop = 0, sauts = 0, decaleArret = 0, horsChaussee = 0, deriveTrop = 0, pasVirage = 0;
    double latMax = 0, capMax = 0, sautMax = 0, vMaxKmh = 0, deriveMax = 0, ayMax = 0;
};
// RG-08 mesuré là où une vraie voiture n'a pas de vitesse de travers : à l'essieu arrière (roues non directrices). Au centre de la
// voiture, un virage donne toujours une vitesse latérale b·r (la voiture pivote autour de son essieu arrière), même sans glisse ;
// une voiture « déplacée comme une carte à jouer » a, elle, une vitesse latérale à l'essieu arrière (ANO-272 : jusqu'à 6,75 m/s)
static void mesurer(Track& t, const Police& p, double rdAvant, double xAvant, Mouvement& m, const physique::Params& P) {
    if (p.fixe || !p.mondeOk) return;
    m.pas++;
    const double u = p.monde.u(), vlat = p.monde.v() - P.b * p.monde.r, au = std::abs(u);
    // « hors glisse des pneus » : pneus peu sollicités (moins de la moitié de leur adhérence, freinage compris, et accélération latérale
    // ≤ 0,2 g). En virage appuyé ou en freinant en courbe, la vitesse de travers de
    // l'essieu arrière est la dérive de ses pneus (réel : ≈ 1° par 0,3 g, 3 à 5° vers 0,8 g, Milliken « Race Car Vehicle Dynamics »
    // ch. 2) : elle est mesurée à part, bornée à 8° (pic d'un pneu de série) tant que les pneus n'ont pas dépassé leur pic
    // (accélération latérale lissée du moteur de conduite, ou instantanée u·r si elle est plus forte : début d'un virage)
    const double ay = std::max(std::abs(p.monde.ay), std::abs(u * p.monde.r)) / physique::G;
    const bool glisse = p.monde.sigmaAv > 1 || p.monde.sigmaAr > 1;   // pneus au-delà du pic : glisse réelle, permise
    const double ecart = std::atan2(std::abs(vlat), std::max(au, 0.01)) * 180 / PI_T;   // caisse / déplacement de l'essieu arrière
    const bool peuSollicites = p.monde.sigmaAv <= 0.5 && p.monde.sigmaAr <= 0.5;
    if (!glisse && ay <= 0.2 && peuSollicites) {
        m.latMax = std::max(m.latMax, std::abs(vlat)); if (std::abs(vlat) > 0.5) m.latTrop++;
        if (au > 5 / 3.6) { m.capMax = std::max(m.capMax, ecart); if (ecart > 3) m.capTrop++; }
    } else if (!glisse && au > 5 / 3.6) {
        m.pasVirage++; m.deriveMax = std::max(m.deriveMax, ecart); if (ecart > 8) m.deriveTrop++;
    }
    m.ayMax = std::max(m.ayMax, ay);
    // saut : déplacement (rendu : projection) plus grand que v × dt + 5 cm
    const TrackPoint a = t.pointLisse(rdAvant, xAvant * RW), b = t.pointLisse(p.rd, p.x * RW);
    const double d = std::hypot(b.x - a.x, b.z - a.z), lim = std::hypot(p.monde.vx, p.monde.vz) * DT + 0.05;
    m.sautMax = std::max(m.sautMax, d - lim + 0.05); if (d > lim) m.sauts++;
    // à l'arrêt, aucun décalage de côté
    if (au < 0.05 && std::abs(p.x - xAvant) * RW > 0.002) m.decaleArret++;
    if (std::abs(p.x) > 1.0) m.horsChaussee++;
    m.vMaxKmh = std::max(m.vMaxKmh, au * 3.6);
}

int main() {
    static Track corniche; corniche.build(mapById("plage"), trace::versionCarte("plage"));

    std::printf("== (A) RG-00 / RG-08 : 60 s de poursuite (Corniche), 3 voitures de police, trafic\n");
    {
        Banc b(corniche, "golf");
        b.placerJoueur(2000, 0, 60);
        b.ajouter(0, -60, 0, 60); b.ajouter(1, -80, -0.67, 60); b.ajouter(2, -100, 0.67, 60);
        Mouvement m; int contactsPol = 0;
        double sTrafic[4] = {2150, 2300, 2500, 2800}; const double xTrafic[4] = {0.0, 0.67, -0.67, 0.0};
        for (int i = 0; i < 120 * 60; i++) {
            DriveInput in; in.gas = b.pl.kmh() < 110; in.steer = 0;   // aide à la direction activée : elle suit sa file
            b.obs.clear();
            for (int k = 0; k < 4; k++) { sTrafic[k] += 50 / 3.6 * DT; b.obs.push_back({sTrafic[k] / M, xTrafic[k], 50 / 3.6 / M}); }
            std::vector<double> rd0, x0;
            for (const Police& p : b.ps) { rd0.push_back(p.rd); x0.push_back(p.x); }
            const int c0 = b.chocs;
            b.pas(in);
            contactsPol += b.chocs - c0;
            for (size_t k = 0; k < b.ps.size(); k++) if (b.ps[k].hitCd < 1.0) mesurer(corniche, b.ps[k], rd0[k], x0[k], m, *b.f.P);   // hors contact
            // trafic dépassé : replacé devant (le trafic de la Poursuite fait de même)
            for (int k = 0; k < 4; k++) if (sTrafic[k] < b.pl.dist * M - 150) sTrafic[k] = b.pl.dist * M + 250 + 40 * k;
        }
        std::printf("joueur à %.0f km/h, %.2f km ; police : %d pas mesurés ; hors glisse (pneus à moins de la moitié de leur adhérence, ≤ 0,2 g) : vitesse latérale à l'essieu arrière "
                    "(son repère) au plus %.2f m/s (%d pas > 0,5), écart cap / déplacement au plus %.2f° (%d pas > 3°) ; virages appuyés "
                    "et freinages en courbe (%d pas, jusqu'à %.2f g) : dérive arrière au plus %.2f° (%d pas > 8°) ; %d saut(s) (au plus %.3f m), %d décalage(s) à "
                    "l'arrêt, %d pas hors chaussée, vitesse maxi %.0f km/h, %d choc(s) avec le joueur, plus proche %.1f m\n",
                    b.pl.kmh(), b.pl.dist * M / 1000, m.pas, m.latMax, m.latTrop, m.capMax, m.capTrop, m.pasVirage, m.ayMax, m.deriveMax,
                    m.deriveTrop, m.sauts, m.sautMax, m.decaleArret, m.horsChaussee, m.vMaxKmh, contactsPol, plusProcheM(b.ps, b.pl.dist, b.pl.playerX));
        VERIFIE(m.latTrop == 0, "RG-08 : la police glisse de côté (%d pas à plus de 0,5 m/s, %.2f m/s)", m.latTrop, m.latMax);
        VERIFIE(m.capTrop == 0, "RG-08 : la police ne roule pas dans l'axe de ses roues (%d pas, %.2f°)", m.capTrop, m.capMax);
        VERIFIE(m.deriveTrop == 0, "RG-08 : dérive des pneus arrière irréaliste en virage (%d pas, %.2f°)", m.deriveTrop, m.deriveMax);
        VERIFIE(m.ayMax <= b.f.P->mu * 1.02, "RG-00 : accélération latérale %.2f g au-delà de l'adhérence (%.2f)", m.ayMax, b.f.P->mu);
        VERIFIE(m.sauts == 0, "RG-08 : %d saut(s) de position", m.sauts);
        VERIFIE(m.decaleArret == 0, "RG-08 : la police se décale à l'arrêt (%d pas)", m.decaleArret);
        VERIFIE(m.horsChaussee == 0, "RG-10 : la police sort de la chaussée (%d pas)", m.horsChaussee);
        VERIFIE(m.vMaxKmh <= 208 * 1.01, "RG-01 : police à %.0f km/h (fiche 208)", m.vMaxKmh);
        VERIFIE(plusProcheM(b.ps, b.pl.dist, b.pl.playerX) < 40, "la police ne suit plus le joueur (%.0f m)", plusProcheM(b.ps, b.pl.dist, b.pl.playerX));
    }

    std::printf("== (B) RG-03 : joueur immobile près de la police → arrestation\n");
    {
        Banc b(corniche, "golf");
        b.placerJoueur(3000, 0, 0);
        b.ajouter(0, -40, 0, 30);
        double tCompte = -1, tArret = -1; int compteMax = 0;
        for (int i = 0; i < 120 * 30 && !b.etat.arrete; i++) {
            b.pas(DriveInput{});
            const int n = compteArrestation(b.etat);
            if (n && tCompte < 0) tCompte = b.temps;
            compteMax = std::max(compteMax, n);
            if (b.etat.arrete) tArret = b.temps;
        }
        std::printf("compte à rebours dès %.1f s (police à %.1f m), arrêté à %.1f s (%.1f s après), par la police : %s, compte affiché de %d\n",
                    tCompte, plusProcheM(b.ps, b.pl.dist, b.pl.playerX), tArret, tArret - tCompte, b.etat.parPolice ? "oui" : "non", compteMax);
        VERIFIE(b.etat.arrete && b.etat.parPolice, "RG-03 : joueur immobile près de la police non arrêté");
        VERIFIE(tCompte > 0 && std::abs((tArret - tCompte) - ARRET_S) < 0.05, "RG-03 : arrestation %.2f s après le début du compte (4 s attendues)", tArret - tCompte);
        VERIFIE(compteMax == 4, "RG-03 : compte à rebours de %d (4 attendu)", compteMax);
        // le compte s'annule si le joueur repart au-dessus de 10 km/h
        Etat e; for (int i = 0; i < 120 * 3 + 2; i++) poursuite::pas(e, DT, 6, 2, 2);
        VERIFIE(compteArrestation(e) == 1 && !e.arrete, "compte à 3 s : %d", compteArrestation(e));
        poursuite::pas(e, DT, 6, 2, 12);
        VERIFIE(compteArrestation(e) == 0 && e.arrestation == 0, "compte non annulé au-dessus de 10 km/h");
        for (int i = 0; i < 120 * 5; i++) poursuite::pas(e, DT, 6, 2, 7);   // entre 5 et 10 km/h : il attend
        VERIFIE(!e.arrete && compteArrestation(e) == 0, "entre 5 et 10 km/h : pas d'arrestation");
        // RG-04 : pénalité seulement au contact réel, jamais pendant le compte à rebours
        Etat e2; const int s0 = e2.score;
        for (int i = 0; i < 120 * 3; i++) poursuite::pas(e2, DT, 5, 0.5, 40);   // police à 0,5 m : pas de contact
        VERIFIE(e2.score == s0, "RG-04 : pénalité sans contact (%d)", s0 - e2.score);
        for (int i = 0; i < 120 * 3; i++) poursuite::pas(e2, DT, 4.2, 0.1, 2);  // au contact mais arrestation en cours
        VERIFIE(e2.score == s0, "RG-04 : pénalité pendant le compte à rebours (%d)", s0 - e2.score);
        std::printf("arrestation : compte 4…1, annulé au-dessus de 10 km/h, pénalité seulement au contact et hors compte : OK\n");
    }

    std::printf("== (C) RG-02 : joueur arrêté encadré par la police, GAZ appuyé\n");
    for (const char* voiture : {"golf", "p208", "rrsport"}) {
        Banc b(corniche, voiture);
        b.placerJoueur(4000, 0, 0);
        const double L = b.f.contactL, W = b.f.contactW;
        b.ajouter(0, L + 0.05, 0, 0);            // devant, pare-chocs contre pare-chocs
        b.ajouter(1, -L - 0.05, 0, 0);           // derrière
        b.ajouter(2, 0.3, W / RW + 0.003, 0);    // à côté, flanc contre flanc
        for (int i = 0; i < 120 * 2; i++) b.pas(DriveInput{});   // 2 s d'arrêt (la police est autour)
        double tRepart = -1; const double t0 = b.temps;
        for (int i = 0; i < 120 * 5 && tRepart < 0; i++) {
            DriveInput in; in.gas = true;
            b.pas(in);
            if (b.pl.kmh() > 10) tRepart = b.temps - t0;
        }
        std::printf("%-8s : repart (> 10 km/h) en %.2f s ; police devant à %.1f m\n", voiture, tRepart, (b.ps[0].rd - b.pl.dist) * M);
        VERIFIE(tRepart >= 0 && tRepart <= 2.0, "RG-02 (%s) : le joueur ne repart pas en 2 s (%.2f s)", voiture, tRepart);
        VERIFIE(!b.etat.arrete, "RG-02 (%s) : arrêté alors qu'il repart", voiture);
    }

    std::printf("== (D) RG-05 : chocs sans frein : jamais de « R »\n");
    {
        int chocs = 0, rAffiche = 0, reculTrop = 0, amortiLent = 0; double vMin = 0;
        for (int k = 0; chocs < 200 && k < 400; k++) {
            Banc b(corniche, k % 2 ? "p208" : "golf");
            b.placerJoueur(5000 + 37 * k, (k % 3 - 1) * 0.67, 20 + (k * 7) % 40);
            // voiture de police garée en travers de sa file (barrage), un peu décalée : choc de face ou d'angle, sans frein (capture 3)
            Police& p = b.ajouter(0, 8 + (k % 5), b.pl.playerX + ((k % 7) - 3) * 0.03, 0);
            p.fixe = true;
            double tNeg = -1;
            for (int i = 0; i < 120 * 4; i++) {
                DriveInput in; in.gas = true; in.steer = 0;
                const int c0 = b.chocs;
                b.pas(in);
                chocs += b.chocs - c0;
                const bool r = ((b.pl.speed < -1 && b.pl.reculVoulu) || b.pl.reversing);   // condition du « R » du HUD (hud.cpp)
                if (r) rAffiche++;
                const double kmh = b.pl.speed * M * 3.6;
                vMin = std::min(vMin, kmh);
                if (kmh < -5.05) reculTrop++;
                if (kmh < -0.01) { if (tNeg < 0) tNeg = b.temps; else if (b.temps - tNeg > 1.0) amortiLent++; } else tNeg = -1;
            }
        }
        std::printf("%d chocs : « R » affiché %d pas, recul le plus rapide %.1f km/h, %d pas au-delà de 5 km/h, %d pas de recul de plus d'1 s\n",
                    chocs, rAffiche, -vMin, reculTrop, amortiLent);
        VERIFIE(chocs >= 200, "RG-05 : seulement %d chocs", chocs);
        VERIFIE(rAffiche == 0, "RG-05 : « R » sans frein (%d pas)", rAffiche);
        VERIFIE(reculTrop == 0, "RG-05 : recul dû au choc au-delà de 5 km/h (%.1f km/h)", -vMin);
        VERIFIE(amortiLent == 0, "RG-05 : recul dû au choc non amorti en 1 s (%d pas)", amortiLent);
        // marche arrière voulue (FREIN tenu à l'arrêt) : toujours possible, avec « R »
        Banc b(corniche, "golf"); b.placerJoueur(9000, 0, 0);
        DriveInput in; in.brake = true; bool r = false;
        for (int i = 0; i < 120 * 3; i++) { b.pas(in); r = r || b.pl.reversing; }
        VERIFIE(r && b.pl.kmh() < -10, "marche arrière voulue impossible (%.0f km/h)", b.pl.kmh());
    }

    std::printf("== (E) RG-09 : police derrière un camion en travers\n");
    {
        Banc b(corniche, "golf");
        b.placerJoueur(6000, -0.67, 50);
        const double sCamion = 6000 - 40;   // le joueur est passé ; camion sur les files 1 et 2 (x 0 et 0,67)
        b.ajouter(0, -120, 0.67, 60);
        int changeCible = 0, retours = 0, dedans = 0; double cible0 = b.ps[0].cibleX, penMax = 0;
        double extremum = b.ps[0].x; int sens = 0;   // aller-retour : la voiture repart en sens inverse de plus de 0,1 demi-largeur (0,75 m)
        for (int i = 0; i < 120 * 20; i++) {
            DriveInput in; in.gas = b.pl.kmh() < 50;
            b.obs = {{sCamion / M, 0.0, 0}, {sCamion / M, 0.67, 0}};
            b.pas(in);
            const Police& p = b.ps[0];
            if (std::abs(p.cibleX - cible0) > 0.05) { changeCible++; cible0 = p.cibleX; }
            if (sens >= 0 && p.x > extremum) extremum = p.x;
            else if (sens <= 0 && p.x < extremum) extremum = p.x;
            if (std::abs(p.x - extremum) > 0.1) { const int s2 = p.x > extremum ? 1 : -1; if (sens != 0 && s2 != sens) retours++; sens = s2; extremum = p.x; }
            // recouvrement avec le camion (gabarit de contact d'une file : 4,4 × 1,9 m)
            for (double xo : {0.0, 0.67}) {
                const double ovZ = (b.f.longueur + 4.4) / 2 - std::abs(p.rd * M - sCamion), ovX = (b.f.largeur + 1.9) / 2 - std::abs(p.x - xo) * RW;
                if (ovZ > 0 && ovX > 0) { penMax = std::max(penMax, std::min(ovZ, ovX)); if (std::min(ovZ, ovX) > 0.05) dedans++; }
            }
        }
        std::printf("police : %d changement(s) de file visée, %d aller-retour latéral, recouvrement du camion au plus %.3f m, "
                    "passée de %.0f m (x = %.2f)\n", changeCible, retours, penMax, b.ps[0].rd * M - sCamion, b.ps[0].x);
        VERIFIE(retours == 0, "RG-09 : %d aller-retour de la police à côté du camion", retours);
        VERIFIE(dedans == 0, "RG-09 : la police entre dans le camion (%.3f m)", penMax);
        // toutes les files barrées : elle s'arrête derrière, sans glisser de côté
        Banc c(corniche, "golf");
        c.placerJoueur(7000, 0, 0);
        c.ajouter(0, -150, 0.67, 60);
        int cote = 0; double xArret = 0;
        for (int i = 0; i < 120 * 20; i++) {
            c.obs = {{(7000 - 60) / M, -0.67, 0}, {(7000 - 60) / M, 0.0, 0}, {(7000 - 60) / M, 0.67, 0}};
            const double x0 = c.ps[0].x;
            c.pas(DriveInput{});
            if (std::abs(c.ps[0].monde.u()) < 0.05 && std::abs(c.ps[0].x - x0) * RW > 0.002) cote++;
            xArret = c.ps[0].x;
        }
        const double ecart = (7000 - 60) - c.ps[0].rd * M;
        std::printf("files barrées : police arrêtée %.1f m derrière l'obstacle (x = %.2f), %d décalage(s) à l'arrêt\n", ecart, xArret, cote);
        VERIFIE(ecart > (c.f.longueur + 4.4) / 2 - 0.05, "RG-09 : la police entre dans l'obstacle (%.2f m)", ecart);
        VERIFIE(cote == 0, "RG-09 : la police glisse de côté à l'arrêt (%d)", cote);
        VERIFIE(std::abs(c.ps[0].monde.u()) < 0.3, "RG-09 : la police ne s'arrête pas (%.1f km/h)", c.ps[0].monde.u() * 3.6);
    }

    std::printf("== (F) RG-10 : le joueur recule à côté de la police\n");
    for (double cote : {-1.0, 1.0}) {
        Banc b(corniche, "golf");
        b.placerJoueur(8000, 0, 0);
        b.ajouter(1, 0.5, cote * 0.67, 30);   // à côté, elle roule à 30 km/h
        b.ps[0].cibleX = cote * 0.67;
        double uMin = 0, xMax = 0, colle = 0;
        for (int i = 0; i < 120 * 10; i++) {
            DriveInput in; in.brake = true;   // marche arrière voulue : jusqu'à 31 km/h
            b.pas(in);
            const Police& p = b.ps[0];
            uMin = std::min(uMin, p.monde.u()); xMax = std::max(xMax, std::abs(p.x));
            // « collée à son flanc en reculant avec lui » : à moins de 1 m de son flanc, en recul à plus de 5 km/h
            if (std::abs(p.rd - b.pl.dist) * M < b.f.contactL && std::abs(p.x - b.pl.playerX) * RW < b.f.contactW + 1 && p.monde.u() < -5 / 3.6) colle += DT;
        }
        std::printf("police à %s : recul le plus rapide %.1f km/h, |x| au plus %.2f, %.1f s collée en reculant, joueur à %.0f km/h, police à %.1f m devant\n",
                    cote < 0 ? "gauche" : "droite", -uMin * 3.6, xMax, colle, b.pl.kmh(), (b.ps[0].rd - b.pl.dist) * M);
        VERIFIE(-uMin * 3.6 <= 15, "RG-10 : la police recule à %.1f km/h (15 km/h au plus)", -uMin * 3.6);
        VERIFIE(xMax <= 1.0, "RG-10 : la police sort de la chaussée (|x| = %.2f)", xMax);
        VERIFIE(colle == 0, "RG-10 : la police recule collée au joueur (%.1f s)", colle);
    }

    std::printf("police de la Poursuite, vraie voiture (WFS-540) : %s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
