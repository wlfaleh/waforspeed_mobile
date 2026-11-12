// WFS-461 : police de la Poursuite (ANO-261, ANO-262), calcul seul.
//  (A) RG-03 : la police roule avec la fiche de sa voiture (208 : 208 km/h ; Golf GTI quand le joueur est en 208 : 250 km/h) :
//      vitesse maxi ≤ fiche + 1 % en ligne droite, en rattrapage, après une perte (replacement à 220 m) et à la naissance d'un
//      renfort ; 0-100 km/h et 100-0 de la fiche ; RG-04 : elle rattrape encore un joueur plus lent qu'elle.
//  (B) RG-01 / RG-02 : contacts police / joueur (Corniche, comme dans la Poursuite : poursuite::avancer puis duel::contacter) :
//      la voiture du joueur n'est plus poussée de côté sans effort (glisse due aux contacts), ne tourne plus à chaque pas du
//      contact, et un contact ne la fait jamais passer en marche arrière.
// usage : test_police_461   ;   compilé avec -DAVANT contre le code de main : mesures « avant » (même scénario, même graine)
#include <algorithm>
#include <cmath>
#include <cstdio>
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
static double kmh(double u) { return u * M * 3.6; }

#ifdef AVANT   // code de main : avancer sans fiche (MAX_SPEED × 1,45)
struct FicheBanc { double vMax = MAX_SPEED * 1.45; double contactL = CONTACT_L_M, contactW = CONTACT_W_M; };
static FicheBanc fiche(const char*, const char*) { return {}; }
static void pas(std::vector<Police>& ps, double t, double sJ, double xJ, double vJ, const std::vector<Obstacle>& obs, const FicheBanc&, const Track*) {
    avancer(ps, DT, t, sJ, xJ, vJ, obs);
}
#else
using FicheBanc = FichePolice;
static FicheBanc fiche(const char* p, const char* j) { return fichePolice(p, j); }
static void pas(std::vector<Police>& ps, double t, double sJ, double xJ, double vJ, const std::vector<Obstacle>& obs, const FicheBanc& f, const Track* tr) {
    avancer(ps, DT, t, sJ, xJ, vJ, obs, f, tr);
}
#endif

// (A) un joueur qui file à vJ km/h en ligne droite pendant `duree` s ; police partie à v0 km/h, `ecart0` m derrière ;
// rend la vitesse maxi de la police (km/h) et, dans *proche, la plus petite distance au joueur (m)
static double vitesseMaxPolice(const char* police, const char* joueur, double vJkmh, double v0kmh, double ecart0, double duree,
                               const Track* tr, double* proche = nullptr, int* replacements = nullptr) {
    const FicheBanc f = fiche(police, joueur);
    const double vJ = vJkmh / 3.6 / M;
    double sJ = 2000 / M, vmax = 0, pr = 1e9; int rep = 0;
    std::vector<Police> ps = {renfort(0, sJ, v0kmh / 3.6 / M, 5)};
    ps[0].rd = sJ - ecart0 / M; ps[0].v = v0kmh / 3.6 / M;
    std::vector<Obstacle> obs;
    for (int i = 0; i < (int)(duree * 120); i++) {
        sJ += vJ * DT;
        const double avant = (sJ - ps[0].rd) * M;
        pas(ps, i * DT, sJ, 0, vJ, obs, f, tr);
        const double rel = (sJ - ps[0].rd) * M;
        if (avant > PERDUE_M - 1 && rel < avant - 50) rep++;   // replacée à 220 m après une perte
        vmax = std::max(vmax, kmh(ps[0].v)); pr = std::min(pr, rel);
    }
    if (proche) *proche = pr;
    if (replacements) *replacements = rep;
    return vmax;
}

// (B) une Poursuite réduite : le joueur roule à vJ km/h (gaz si plus lent, rien sinon) dans sa file, volant `volant`, frein
// `frein` ; 2 voitures de police (derrière, à gauche) qui attaquent à tour de rôle ; mesure ce que les contacts font à la
// voiture du joueur (déplacement latéral et rotation dans duel::contacter), et si la marche arrière s'enclenche sans frein
struct MesureContact { int contacts = 0; double glisseM = 0, vLatMax = 0, rotation = 0, rotPasMax = 0, pousseFrein = 0; bool reculSansFrein = false; double vFin = 0; };
static MesureContact contactsPoursuite(Track& t, const char* voiture, double vJkmh, float volant, bool frein, double duree) {
    MesureContact m;
    const char* pol = std::string(voiture) == "p208" ? "golf" : "p208";
    const FicheBanc f = fiche(pol, voiture);
    Player pl; pl.reset(carById(voiture)); pl.aide = Aide::Activee;
    pl.dist = 3000 / M; pl.position = t.wrap(pl.dist); pl.speed = vJkmh / 3.6 / M; pl.playerX = 0;
    std::vector<Police> ps = {renfort(0, pl.dist, pl.speed, 3), renfort(1, pl.dist, pl.speed, 3)};
    ps[0].role = 0; ps[1].role = 1;
    ps[0].rd = pl.dist - 12 / M; ps[1].rd = pl.dist - 2 / M; ps[0].x = ps[0].cibleX = 0; ps[1].x = ps[1].cibleX = -0.62;
    std::vector<Obstacle> obs;
    for (int i = 0; i < (int)(duree * 120); i++) {
        DriveInput in; in.steer = volant; in.brake = frein; in.gas = !frein && kmh(pl.speed) < vJkmh;
        pl.update(DT, in, t, true);
        pas(ps, 6.0 + i * DT, pl.dist, pl.playerX, pl.speed, obs, f, &t);
        for (Police& p : ps) {
            duel::Ghost g; g.rd = p.rd; g.x = p.x; g.v = p.v; g.car = pol; g.hitCd = p.hitCd;
            const double x0 = pl.playerX, h0 = pl.heading, v0 = pl.speed;
            const duel::ContactEvents e = duel::contacter(DT, pl, t, carById(voiture), g);
            p.hitCd = g.hitCd;
            if (e.chocs || e.frottements) m.contacts++;
            // rotation qui ÉCARTE la voiture de l'axe de la route (celle qui la ramène parallèle à l'autre voiture, le long de son
            // flanc, est la réaction normale du contact et n'est pas comptée)
            const auto ecartAxe = [](double h) { const double a = std::abs(std::remainder(h, 2 * 3.14159265358979323846)); return std::min(a, 3.14159265358979323846 - a); };
            const double dx = std::abs(pl.playerX - x0) * RW, dh = std::max(0.0, ecartAxe(pl.heading) - ecartAxe(h0));
            m.glisseM += dx; m.vLatMax = std::max(m.vLatMax, dx / DT); m.rotation += dh; m.rotPasMax = std::max(m.rotPasMax, dh);
            if (v0 >= 0 && pl.speed < 0 && !frein) m.reculSansFrein = true;   // RG-02 : un contact ne met jamais en marche arrière
            // frein serré : vitesse gagnée par la poussée de la police (km/h, cumulée) ; une voiture de même poids qui pousse ne
            // peut pas entraîner une voiture qui freine (sa poussée est bornée par l'adhérence de ses roues motrices)
            if (frein && v0 >= 0) m.pousseFrein += std::max(0.0, kmh(pl.speed - v0));   // (en marche arrière voulue : choc de recul, à part)
        }
        if (pl.speed < -1 && !frein) m.reculSansFrein = true;
    }
    m.vFin = kmh(pl.speed);
    return m;
}

int main() {
    std::printf("== (A) vitesse de la police (RG-03, ANO-262)\n");
    static Track corniche; corniche.build(mapById("plage"), trace::versionCarte("plage"));
    struct Cas { const char* nom; const char* police; const char* joueur; double vJ, v0, ecart, duree; const Track* tr; };
    const Cas cas[] = {
        {"ligne droite, joueur à 300 km/h (208)", "p208", "golf", 300, 150, 20, 120, nullptr},
        {"rattrapage d'un joueur à 300 km/h, Corniche", "p208", "golf", 300, 0, 15, 120, &corniche},
        {"replacement après perte (joueur à 300 km/h)", "p208", "rrsport", 300, 200, 290, 60, nullptr},
        {"renfort né à 300 km/h", "p208", "golf", 300, 300, 220, 30, nullptr},
        {"ligne droite, Golf GTI (joueur en 208)", "golf", "p208", 300, 150, 20, 120, nullptr},
    };
    for (const Cas& c : cas) {
        int rep = 0;
        const double v = vitesseMaxPolice(c.police, c.joueur, c.vJ, c.v0, c.ecart, c.duree, c.tr, nullptr, &rep);
        const double fichev = carById(c.police).spec.vmax;
        std::printf("%-46s : police %s, vitesse maxi %.1f km/h (fiche %.0f km/h)%s\n", c.nom, c.police, v, fichev,
                    rep ? " — replacée après perte" : "");
#ifndef AVANT
        VERIFIE(v <= fichev * 1.01, "%s : police à %.1f km/h > fiche %.0f km/h", c.nom, v, fichev);
#endif
    }
#ifndef AVANT
    {   // la police atteint bien sa vitesse de fiche (≥ 97 %) : elle n'est pas bridée plus bas que le réel
        const double v = vitesseMaxPolice("p208", "golf", 300, 150, 20, 180, nullptr);
        VERIFIE(v >= 208 * 0.97, "208 : vitesse maxi atteinte %.1f km/h (< 97 %% de 208)", v);
        // 0-100 km/h et 100-0 de la fiche (WFS-188 : 0-100 ±10 %, 100-0 de 33 à 41 m)
        const FichePolice f = fichePolice("p208", "golf");
        double v0 = 0, t0 = 0; while (kmh(v0) < 100 && t0 < 60) { v0 = vitessePolice(f, v0, f.vMax, DT); t0 += DT; }
        double v1 = 100 / 3.6 / M, d1 = 0; for (int i = 0; i < 6000 && v1 > 0; i++) { v1 = vitessePolice(f, v1, 0, DT); d1 += v1 * M * DT; }
        std::printf("208 de police : 0-100 km/h en %.1f s (fiche %.1f s), 100-0 en %.1f m\n", t0, carById("p208").spec.s100, d1);
        VERIFIE(std::abs(t0 - carById("p208").spec.s100) <= carById("p208").spec.s100 * 0.10, "0-100 : %.1f s", t0);
        VERIFIE(d1 >= 33 && d1 <= 41, "100-0 : %.1f m", d1);
        // RG-04 : jouable : elle rattrape un joueur plus lent qu'elle (160 km/h, 200 m devant) sans dépasser sa fiche
        double proche = 1e9;
        const double vr = vitesseMaxPolice("p208", "golf", 160, 160, 200, 60, &corniche, &proche);
        std::printf("rattrapage d'un joueur à 160 km/h parti 200 m devant (Corniche) : au plus près %.1f m, vitesse maxi %.1f km/h\n", proche, vr);
        VERIFIE(proche < 20, "RG-04 : la police ne rattrape pas un joueur à 160 km/h (%.0f m)", proche);
    }
#endif

    std::printf("== (B) contacts police / joueur (RG-01, RG-02, ANO-261)\n");
    struct CasC { const char* nom; const char* voiture; double vJ; float volant; bool frein; };
    const CasC cc[] = {
        {"30 km/h, file tenue, Range Rover Sport", "rrsport", 30, 0.f, false},
        {"30 km/h, braque vers la police, Golf", "golf", 30, -0.5f, false},
        {"30 km/h, braque vers la police, Range Rover Sport", "rrsport", 30, -0.5f, false},
        {"90 km/h, file tenue, Golf", "golf", 90, 0.f, false},
        {"10 km/h, frein tenu (poussé par l'arrière), Golf", "golf", 10, 0.f, true},
        {"marche arrière vers la police (vidéo 12:04), Golf", "golf", 0, 0.3f, true},
        {"marche arrière vers la police, Range Rover Sport", "rrsport", 0, -0.3f, true},
    };
    for (const CasC& c : cc) {
        const MesureContact m = contactsPoursuite(corniche, c.voiture, c.vJ, c.volant, c.frein, 40);
        std::printf("%-50s : %4d pas de contact, glisse due aux contacts %.2f m (au plus %.2f m/s), rotation qui écarte de l'axe %.2f rad (%.3f rad/pas au plus), "
                    "marche arrière sans frein : %s, vitesse donnée frein serré %.0f km/h, vitesse finale %.0f km/h\n",
                    c.nom, m.contacts, m.glisseM, m.vLatMax, m.rotation, m.rotPasMax, m.reculSansFrein ? "OUI" : "non", m.pousseFrein, m.vFin);
#ifndef AVANT
        // RG-01 : sans effort au-delà de l'adhérence, pas de glisse : un contact ne fait que sortir la voiture du recouvrement
        // (quelques cm) ; vitesse latérale due au contact < 1 m/s ; rotation seulement sur un vrai choc (≤ 0,06 rad), jamais à chaque pas
        VERIFIE(m.vLatMax < 1.0, "%s : voiture poussée de côté à %.2f m/s", c.nom, m.vLatMax);
        VERIFIE(m.glisseM < 0.5, "%s : %.2f m de glisse due aux contacts", c.nom, m.glisseM);
        VERIFIE(m.rotPasMax < 0.065, "%s : rotation de %.3f rad en un pas de contact", c.nom, m.rotPasMax);
        VERIFIE(m.rotation < 0.25, "%s : %.2f rad de rotation due aux contacts en 40 s (la voiture « danse »)", c.nom, m.rotation);
        VERIFIE(!m.reculSansFrein, "%s : marche arrière sans frein (RG-02)", c.nom);
        VERIFIE(m.pousseFrein < 15, "%s : frein serré, la police donne %.0f km/h à la voiture (glisse « sur la glace »)", c.nom, m.pousseFrein);
#endif
    }
    std::printf("police de la Poursuite (WFS-461) : %s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
