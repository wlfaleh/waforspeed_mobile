// Poursuite police (WFS-111) : score (−5 au choc, arrêt à 0, victoire à 10 km), police (2 au départ, +1 tous les 2 km,
// rattrape le joueur), programme tiré par la graine (embouteillages, surprises). Lancé par test_course5 (même exécutable).
#include <cmath>
#include <cstdio>
#include <set>
#include "../src/game/poursuite.h"
using namespace wfs;
using namespace wfs::poursuite;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int testPoursuite() {
    // RG-01 / RG-02 : score de départ 100, −5 au choc avec le trafic (une fois par seconde au plus), −3 glissière, −10 police
    Etat e;
    VERIFIE(e.score == 100, "score de départ %d", e.score);
    chocTrafic(e); VERIFIE(e.score == 95, "choc trafic : %d (95 attendu)", e.score);
    chocTrafic(e); VERIFIE(e.score == 95, "deuxième choc dans la même seconde : %d", e.score);
    pas(e, 1.01, 100); chocTrafic(e); VERIFIE(e.score == 90, "choc trafic après 1 s : %d", e.score);
    chocGlissiere(e); VERIFIE(e.score == 87, "glissière : %d", e.score);
    chocPolice(e); VERIFIE(e.score == 77, "police : %d", e.score);
    quasiContact(e); VERIFIE(e.score == 78, "quasi-contact : %d", e.score);
    // police collée à moins de 12 m : −1 par seconde ; éloignée : rien
    for (int i = 0; i < 120 * 3 + 2; i++) pas(e, 1.0 / 120, 8);   // 3 s (+ 2 pas : arrondis)
    VERIFIE(e.score == 75, "3 s collée : %d (75 attendu)", e.score);
    for (int i = 0; i < 120 * 3; i++) pas(e, 1.0 / 120, 30);
    VERIFIE(e.score == 75, "3 s à 30 m : %d", e.score);
    std::printf("score : 100, −5 trafic, −3 glissière, −10 police, +1 quasi-contact, −1/s collée : OK\n");
    // arrêt à 0 : défaite immédiate, plus aucune victoire possible
    for (int i = 0; i < 40 && !e.arrete; i++) { pas(e, 1.01, 100); chocTrafic(e); }
    VERIFIE(e.arrete && e.score == 0, "arrêt à 0 (score %d)", e.score);
    arrivee(e, 10.5); VERIFIE(!e.gagne, "arrêté puis 10 km : pas de victoire");
    std::printf("score à 0 : ARRÊTÉ après %d chocs : OK\n", e.chocsTrafic);
    // victoire à 10 km avec au moins 1 point
    Etat v; perdre(v, 99);
    arrivee(v, 9.99); VERIFIE(!v.gagne, "9,99 km : pas encore");
    arrivee(v, 10.0); VERIFIE(v.gagne && v.score == 1, "10 km avec 1 point : victoire");
    std::printf("victoire à 10 km avec 1 point : OK\n");

    // RG-03 : 2 voitures au départ, une de plus tous les 2 km, 6 au plus
    VERIFIE(policeVoulue(0) == 2 && policeVoulue(1.99) == 2 && policeVoulue(2) == 3 && policeVoulue(8) == 6 && policeVoulue(12) == 6, "renforts");
    // la police rattrape un joueur à 160 km/h parti 40 m devant, sans traverser le trafic
    std::vector<Police> ps = {renfort(0, 0, 0, 7), renfort(1, 0, 0, 7)};
    ps[0].rd = -25 / M; ps[1].rd = -40 / M;
    const double vJ = 160 / 3.6 / M; double sJ = 0, tProche = -1;
    std::vector<Obstacle> obs;
    for (int i = 0; i < 120 * 30; i++) {
        sJ += vJ / 120;
        obs = {{sJ + 30 / M, ps[0].x, 80 / 3.6 / M}};   // une voiture lente devant la police
        avancer(ps, 1.0 / 120, i / 120.0, sJ, 0, vJ, obs);
        if (tProche < 0 && plusProcheM(ps, sJ, 0) < 12) tProche = i / 120.0;
    }
    VERIFIE(tProche >= 0 && tProche < 15, "la police rattrape le joueur (%.1f s)", tProche);
    VERIFIE(std::abs(ps[0].rd - sJ) * M < 20 && std::abs(ps[1].rd - sJ) * M < 20, "la police reste au contact (%.0f m, %.0f m)", (sJ - ps[0].rd) * M, (sJ - ps[1].rd) * M);
    std::printf("police : 2 au départ, renfort tous les 2 km (6 au plus), au contact en %.1f s : OK\n", tProche);

    // RG-04 / RG-05 : programme : 3 embouteillages de 300 à 600 m, 6 surprises dont au moins 3 types ; même graine = même programme
    for (uint32_t g : {1u, 1234u, 20261105u, 99u, 7u}) {
        const Programme p = programme(g, g == 20261105u), q = programme(g, g == 20261105u);   // la graine du banc avec son programme
        bool bouchonsOk = p.bouchons.size() == 3;
        for (const Bouchon& b : p.bouchons) bouchonsOk = bouchonsOk && b.longueurM >= 300 && b.longueurM <= 600 && b.km > 0.5 && b.km < 10 && b.kmhMin >= 20 && b.kmhMax <= 40;
        std::set<int> types; for (const Evenement& s : p.surprises) types.insert((int)s.type);
        bool memes = p.surprises.size() == q.surprises.size();
        for (size_t i = 0; memes && i < p.surprises.size(); i++) memes = p.surprises[i].km == q.surprises[i].km && p.surprises[i].type == q.surprises[i].type;
        bool horsBouchon = true;   // aucune surprise dans un embouteillage
        for (const Evenement& s : p.surprises) for (const Bouchon& b : p.bouchons) horsBouchon = horsBouchon && !(s.km > b.km - 0.25 && s.km < b.km + b.longueurM / 1000 + 0.1);
        VERIFIE(horsBouchon, "graine %u : surprise dans un embouteillage", g);
        VERIFIE(bouchonsOk, "graine %u : embouteillages", g);
        VERIFIE(p.surprises.size() >= 4 && types.size() >= 3, "graine %u : %zu surprises, %zu types", g, p.surprises.size(), types.size());
        VERIFIE(memes, "graine %u : même programme", g);
        std::printf("graine %u : 3 embouteillages, %zu surprises (%zu types) : %s\n", g, p.surprises.size(), types.size(), bouchonsOk && memes && types.size() >= 3 ? "OK" : "ÉCHEC");
    }
    if (echecs) std::printf("poursuite : %d échec(s)\n", echecs);
    else std::printf("poursuite police : tout est OK\n");
    return echecs;
}
