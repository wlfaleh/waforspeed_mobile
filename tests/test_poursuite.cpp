// Poursuite police (WFS-111) : score (−5 au choc, arrêt à 0, victoire à 10 km), police (2 au départ, +1 tous les 2 km,
// rattrape le joueur), programme tiré par la graine (embouteillages, surprises). Lancé par test_course5 (même exécutable).
#include <cmath>
#include <cstdio>
#include <set>
#include <string>
#include "../src/game/poursuite.h"
#include "../src/game/sauvegarde.h"   // WFS-119 : mode gardé
using namespace wfs;
using namespace wfs::poursuite;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int testPoursuite() {
    // RG-01 / RG-02 : score de départ 100, −5 au choc avec le trafic (une fois par seconde au plus), −3 glissière, −10 police
    Etat e; const int D = e.score;   // score de départ de configgame.json (1000 depuis e23fa51), les écarts restent ceux de RG-02
    VERIFIE(e.score == SCORE_DEPART && D > 25, "score de départ %d", e.score);
    chocTrafic(e); VERIFIE(e.score == D - 5, "choc trafic : %d (D − 5 attendu)", e.score);
    chocTrafic(e); VERIFIE(e.score == D - 5, "deuxième choc dans la même seconde : %d", e.score);
    pas(e, 1.01, 100); chocTrafic(e); VERIFIE(e.score == D - 10, "choc trafic après 1 s : %d", e.score);
    chocGlissiere(e); VERIFIE(e.score == D - 13, "glissière : %d", e.score);
    chocPolice(e); VERIFIE(e.score == D - 23, "police : %d", e.score);
    quasiContact(e); VERIFIE(e.score == D - 22, "quasi-contact : %d", e.score);
    // police collée à moins de 12 m : −1 par seconde ; éloignée : rien
    for (int i = 0; i < 120 * 3 + 2; i++) pas(e, 1.0 / 120, 8);   // 3 s (+ 2 pas : arrondis)
    VERIFIE(e.score == D - 25, "3 s collée : %d (D − 25 attendu)", e.score);
    for (int i = 0; i < 120 * 3; i++) pas(e, 1.0 / 120, 30);
    VERIFIE(e.score == D - 25, "3 s à 30 m : %d", e.score);
    std::printf("score : 100, −5 trafic, −3 glissière, −10 police, +1 quasi-contact, −1/s collée : OK\n");
    // arrêt à 0 : défaite immédiate, plus aucune victoire possible
    for (int i = 0; i < D && !e.arrete; i++) { pas(e, 1.01, 100); chocTrafic(e); }
    VERIFIE(e.arrete && e.score == 0, "arrêt à 0 (score %d)", e.score);
    arrivee(e, 10.5); VERIFIE(!e.gagne, "arrêté puis 10 km : pas de victoire");
    std::printf("score à 0 : ARRÊTÉ après %d chocs : OK\n", e.chocsTrafic);
    // victoire à 10 km avec au moins 1 point
    Etat v; perdre(v, v.score - 1);
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
        VERIFIE(p.surprises.size() >= 6 && types.size() == 6, "graine %u : %zu surprises, %zu types", g, p.surprises.size(), types.size());
        VERIFIE(memes, "graine %u : même programme", g);
        std::printf("graine %u : 3 embouteillages, %zu surprises (%zu types) : %s\n", g, p.surprises.size(), types.size(), bouchonsOk && memes && types.size() >= 3 ? "OK" : "ÉCHEC");
    }
    // WFS-119 RG-01 : les 6 surprises (une fois chacune) sur 10 000 graines, hors embouteillage, avant l'arrivée
    int sixTypes = 0, camionLibre = 0;
    for (uint32_t g = 1; g <= 10000; g++) {
        const Programme p = programme(g);
        std::set<int> types; bool ok = true;
        for (const Evenement& s : p.surprises) {
            types.insert((int)s.type);
            ok = ok && s.km < KM_ARRIVEE - 0.3;
            if (s.type == Surprise::Camion) camionLibre += s.file == 0 || s.file == 2;
        }
        sixTypes += ok && p.surprises.size() == 6 && types.size() == 6;
    }
    VERIFIE(sixTypes == 10000, "6 surprises de 6 types : %d graines sur 10 000", sixTypes);
    VERIFIE(camionLibre == 10000, "camion : file libre au bord (%d sur 10 000)", camionLibre);
    VERIFIE(std::string(bandeau(Surprise::Camion)) == "CAMION EN TRAVERS" && std::string(bandeau(Surprise::Ralentisseur)) == "RALENTISSEUR", "bandeaux");
    {   // programme du banc : ralentisseur et camion aux places des captures
        const Programme b = programme(20261105u, true);
        bool r = false, c = false;
        for (const Evenement& s : b.surprises) { r = r || (s.type == Surprise::Ralentisseur && s.km == BANC_RALENTISSEUR_KM); c = c || (s.type == Surprise::Camion && s.km == BANC_CAMION_KM); }
        VERIFIE(r && c, "banc : ralentisseur %d, camion %d", r, c);
    }
    std::printf("6 surprises (barrage, déboîte, travaux, accident, camion, ralentisseur) sur 10 000 graines : OK\n");
    // camion en travers : x5 (rrsport si le joueur roule en x5), à cheval sur 2 files, la file libre n'est pas touchée
    VERIFIE(std::string(camionCar("golf")) == "x5" && std::string(camionCar("x5")) == "rrsport", "modèle du camion");
    VERIFIE(fileLibreCamion(1) == 0 && fileLibreCamion(0) == 0 && fileLibreCamion(2) == 2, "file libre du camion");
    for (int libre : {0, 2}) {
        const Travers tv = camionTravers(libre, 4.92, 2.0);
        const double a = tv.xM - tv.demiX, b = tv.xM + tv.demiX;   // largeur couverte (m)
        const double fl0 = FILES[libre] * RW - RW / 3, fl1 = FILES[libre] * RW + RW / 3;   // la file libre (5 m)
        bool couvre = true;
        for (int f = 0; f < 3; f++) if (f != libre) couvre = couvre && FILES[f] * RW > a && FILES[f] * RW < b;
        VERIFIE(couvre, "camion (file libre %d) : les 2 autres files couvertes [%.1f ; %.1f] m", libre, a, b);
        VERIFIE(b <= fl0 || a >= fl1, "camion (file libre %d) : la file libre [%.1f ; %.1f] m est touchée [%.1f ; %.1f]", libre, fl0, fl1, a, b);
    }
    std::printf("camion en travers : 2 files couvertes, une file libre : OK\n");
    // ralentisseur : −3 au-dessus de 90 km/h, rien dessous
    {
        Etat r;
        VERIFIE(ralentisseur(r, 89) == 0 && ralentisseur(r, 90) == 0 && r.score == D, "ralentisseur à 89 / 90 km/h : %d", r.score);
        VERIFIE(ralentisseur(r, 91) == 3 && r.score == D - 3 && r.ralentisseurs == 1, "ralentisseur à 91 km/h : %d", r.score);
        VERIFIE(ralentisseur(r, 180) == 3 && r.score == D - 6, "ralentisseur à 180 km/h : %d", r.score);
        std::printf("ralentisseur : −3 au-dessus de 90 km/h, rien dessous : OK\n");
    }
    // RG-03 : mode gardé : clé « poursuite » écrite puis relue ; ancienne sauvegarde → Checkpoint (aucun plantage)
    {
        VERIFIE(cleMode(ModeCourse::Poursuite) == "poursuite" && cleMode(ModeCourse::Course5) == "course5" && cleMode(ModeCourse::Checkpoint) == "checkpoint", "cleMode");
        VERIFIE(modeDeCle("poursuite") == ModeCourse::Poursuite && modeDeCle("course5") == ModeCourse::Course5 && modeDeCle("checkpoint") == ModeCourse::Checkpoint, "modeDeCle");
        Sauvegarde a; a.mode = cleMode(ModeCourse::Poursuite);
        Sauvegarde b; const bool lu = b.lire(a.texte());
        VERIFIE(lu && b.mode == "poursuite" && modeDeCle(b.mode) == ModeCourse::Poursuite, "sauvegarde relue : %s", b.mode.c_str());
        Sauvegarde vieille; vieille.lire("version=1\ncar=m4\nkm5=6\n");
        VERIFIE(modeDeCle(vieille.mode) == ModeCourse::Checkpoint, "ancienne sauvegarde sans mode : %s", vieille.mode.c_str());
        Sauvegarde bizarre; bizarre.lire("version=1\nmode=helicoptere\n");
        VERIFIE(modeDeCle(bizarre.mode) == ModeCourse::Checkpoint && bizarre.mode == "checkpoint", "mode inconnu : %s", bizarre.mode.c_str());
        VERIFIE(modeDeCle("") == ModeCourse::Checkpoint, "clé vide");
        std::printf("mode gardé : « poursuite » écrit et relu, ancienne sauvegarde → Checkpoint : OK\n");
    }
    // WFS-141 (RG-01, ANO-56) : gyrophares : rouge puis bleu à 2 Hz, jamais ensemble ni éteints ensemble ; voitures voisines
    // en opposition ; halo de 1,5 × la largeur de la lampe au plus ; couleurs #FF2D55 et #2D7CFF
    {
        int rouges = 0, bleus = 0, changements = 0; bool avant = gyrophare(0.001, 0).rouge;
        for (int i = 0; i < 1200; i++) {   // 10 s au pas de 1/120 s
            const double t = (i + 0.5) / 120;
            const Gyrophare a = gyrophare(t, 0), b = gyrophare(t, 1);
            VERIFIE(a.rouge != a.bleu, "gyrophare : rouge et bleu %s à t = %.3f s", a.rouge ? "ensemble" : "éteints", t);
            VERIFIE(a.rouge == b.bleu, "gyrophare : 2e voiture pas en opposition à t = %.3f s", t);
            rouges += a.rouge; bleus += a.bleu;
            if (a.rouge != avant) { changements++; avant = a.rouge; }
        }
        VERIFIE(rouges == 600 && bleus == 600, "gyrophare : %d images rouges, %d bleues sur 1200", rouges, bleus);
        VERIFIE(changements == 39 || changements == 40, "gyrophare : %d changements en 10 s (2 Hz : 40)", changements);
        VERIFIE(gyrophare(-0.1, 0).bleu && gyrophare(0.1, 0).rouge, "gyrophare : temps négatif");
        VERIFIE(HALO_GYRO <= 1.5f && GYRO_ROUGE == 0xFF2D55u && GYRO_BLEU == 0x2D7CFFu, "gyrophare : halo %.2f, couleurs %06X / %06X", HALO_GYRO, GYRO_ROUGE, GYRO_BLEU);
        std::printf("gyrophares : rouge puis bleu à 2 Hz, jamais ensemble, halo ≤ 1,5 × la lampe : OK\n");
    }
    // WFS-141 (RG-02, ANO-57) : bandeau d'annonce effacé après 1,5 s
    VERIFIE(BANDEAU_S == 1.5, "bandeau : %.2f s", BANDEAU_S);
    // WFS-141 (RG-03, ANO-59) : « 2e PLACE » (jamais « 2E »)
    VERIFIE(titrePlace(1) == "VICTOIRE" && titrePlace(2) == "2e PLACE" && titrePlace(5) == "5e PLACE", "titre : %s / %s", titrePlace(1).c_str(), titrePlace(2).c_str());
    std::printf("bandeau 1,5 s ; titres « VICTOIRE », « 2e PLACE » : OK\n");
    if (echecs) std::printf("poursuite : %d échec(s)\n", echecs);
    else std::printf("poursuite police : tout est OK\n");
    return echecs;
}
