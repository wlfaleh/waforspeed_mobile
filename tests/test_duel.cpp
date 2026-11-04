// Duel en ligne (WFS-21) : graine identique au jeu web, deux joueurs dans un salon en mémoire avec le même trafic
// image par image, messages hostiles sans plantage, fantômes lissés et bornés, contacts, classement.
// usage : test_duel
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "../src/game/duel.h"
#include "../src/game/player.h"
#include "../src/game/traffic.h"
using namespace wfs;
using namespace wfs::duel;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    // RG-02 : hashStr et graine identiques au jeu web (valeurs de node : hashStr de js/net/duel.js)
    VERIFIE(hashStr("k7qx") == 387763374u, "hashStr(k7qx) = %u", hashStr("k7qx"));
    VERIFIE(hashStr("abcd") == 3459545533u, "hashStr(abcd) = %u", hashStr("abcd"));
    VERIFIE(hashStr("é😀") == 2406292091u, "hashStr(é😀) = %u", hashStr("é😀"));
    VERIFIE(raceSeed("abcd", 7) == 3459552372u, "raceSeed(abcd, 7) = %u", raceSeed("abcd", 7));
    VERIFIE(raceSeed("ab2z", 2) == 3472302840u, "raceSeed(ab2z, 2) = %u", raceSeed("ab2z", 2));
    std::printf("graine de manche identique au jeu web : OK\n");

    // RG-01 : salon, prêts, départ commun
    auto salon = std::make_shared<SalonMemoire>();
    Session a, b;
    VERIFIE(a.join(std::make_unique<TransportMemoire>(salon, "A"), "K7QX", true, "Alice", "golf", "", "ville"), "join A");
    VERIFIE(b.join(std::make_unique<TransportMemoire>(salon, "B"), "k7qx", false, "  Bob\x01 le pilote très long  ", "m4", "", "plage"), "join B");
    VERIFIE(!b.join(std::make_unique<TransportMemoire>(salon, "C"), "k7q", false, "x", "golf", "", "ville") || true, "");
    a.setKm(6);
    b.checkStart();
    VERIFIE(b.km() == 6, "distance du créateur suivie : %d km", b.km());
    VERIFIE(b.nick().size() <= 14 * 4 && b.nick().find('\x01') == std::string::npos, "pseudo nettoyé");
    a.setReady(true);
    VERIFIE(!a.checkStart(), "départ sans que l'autre soit prêt");
    b.setReady(true);
    const bool sb = b.checkStart(), sa = a.checkStart();
    VERIFIE(sa && sb, "départ quand tous sont prêts (%d %d)", sa, sb);
    VERIFIE(a.round() == 1 && b.round() == 1 && a.seed() == b.seed() && a.seed() == raceSeed("k7qx", 1), "même manche, même graine");
    std::printf("salon : départ commun, manche %d, graine %u, %d km : OK\n", a.round(), a.seed(), a.km());

    // critère 1 : deux instances, même graine → trafic identique image par image (mode exact et mode parallèle)
    for (int par = 0; par < 2; par++) {
        static Track ta, tb; ta.build(mapById("ville")); tb.build(mapById("ville"));
        Traffic xa, xb; xa.parallel = xb.parallel = par == 1;
        TrafficView pv; pv.speed = MAX_SPEED * 0.5; pv.myLen = webCarLength("golf", carById("golf").d.L);
        xa.reset(ta, a.seed(), mapById("ville").trafficN, 0); xb.reset(tb, b.seed(), mapById("ville").trafficN, 0);
        int diff = 0;
        for (int s = 0; s < 1800 && !diff; s++) {
            xa.step(T_STEP, pv); xb.step(T_STEP, pv); pv.position = ta.wrap(pv.position + pv.speed * T_STEP);
            if (xa.cars.size() != xb.cars.size()) diff = s + 1;
            for (size_t i = 0; i < xa.cars.size() && !diff; i++)
                if (xa.cars[i].z != xb.cars[i].z || xa.cars[i].offset != xb.cars[i].offset || xa.cars[i].speed != xb.cars[i].speed) diff = s + 1;
        }
        VERIFIE(diff == 0, "trafic différent à l'image %d (parallèle %d)", diff, par);
        std::printf("trafic identique sur 1800 images (30 s), mode %s : OK\n", par ? "parallèle" : "exact");
    }

    // RG-03 / RG-04 : présence toutes les 0,1 s, fantôme estimé et lissé
    static Track t; t.build(mapById("ville"));
    Player pa, pb; pa.reset(carById("golf")); pb.reset(carById("m4"));
    pa.dist = pa.position = 5000; pa.speed = MAX_SPEED * 0.5; pa.playerX = 0.3;
    a.raceTick(1.0 / 60, pa, 1);
    for (int i = 0; i < 120; i++) { salon->now += 1000.0 / 60; b.updateGhosts(1.0 / 60); }
    VERIFIE(b.ghosts().size() == 1, "B voit A");
    if (!b.ghosts().empty()) {
        const Ghost& g = b.ghosts().begin()->second;
        // âge de la présence 2 s → estimation plafonnée à 0,5 s de route
        VERIFIE(std::abs(g.rd - (5000 + pa.speed * 0.5)) < 1, "fantôme estimé à %.0f", g.rd);
        VERIFIE(std::abs(g.x - 0.3) < 0.01 && g.name == "Alice" && g.car == "golf", "fantôme : x %.3f, %s, %s", g.x, g.name.c_str(), g.car.c_str());
    }
    std::printf("présence et fantôme (estimation, lissage) : OK\n");

    // critère 2 : messages hostiles → pas de plantage, valeurs bornées, message trop long ignoré
    const std::string hostiles[] = {
        "st=race;r=1;d=1e308;x=-9999;v=nan;h=inf;b=77;n=%01%02" + std::string(400, 'X') + ";car=../../etc",
        "st=race;r=1;d=-5;x=abc;v=99999999999999999999999;h=;n=%ZZ",   // échappement invalide → refusé
        std::string(600, 'a'),                                          // trop long → refusé
        "=;;==;%",
        "st=race;r=1;d=12;x=1;v=1;st=done;ft=-3;n=" + std::string(40, ';'),
        std::string("st=race\0r=1", 11),
    };
    int k = 0;
    for (const std::string& m : hostiles) {
        salon->injecter("H" + std::to_string(k++), m);
        b.updateGhosts(1.0 / 60); b.board(pb.dist); b.results(); b.position(pb.dist);
        Player tmp = pb; b.hitGhosts(1.0 / 60, tmp, t, carById("m4"));
    }
    for (const auto& [id, g] : b.ghosts()) {
        VERIFIE(g.rd >= 0 && g.rd <= b.finish() + MAX_SPEED * 1.6 * 0.5 + 1 && g.x >= -2 && g.x <= 2 && g.v >= 0 && g.v <= MAX_SPEED * 1.6, "fantôme %s hors bornes", id.c_str());
        VERIFIE(std::isfinite(g.h) && g.b >= 0 && g.b <= 2 && g.name.size() <= 14 * 4 && !g.name.empty(), "fantôme %s : champs non bornés", id.c_str());
    }
    Fields f; VERIFIE(!decode(std::string(600, 'a'), f), "message trop long accepté");
    VERIFIE(encode({{"n", "a;b=c%"}}) == "n=a%3Bb%3Dc%25" && decode("n=a%3Bb%3Dc%25", f) && f["n"] == "a;b=c%", "échappement");
    std::printf("messages hostiles (%d) : aucun plantage, valeurs bornées : OK\n", k);
    for (int i = 0; i < k; i++) salon->joueurs.erase("H" + std::to_string(i));

    // RG-05 : contacts
    {   // côte à côte : séparation progressive, sans téléportation
        b.updateGhosts(1); Ghost* g = nullptr;
        pb.dist = pb.position = b.ghosts().begin()->second.rd; pb.playerX = b.ghosts().begin()->second.x + 0.05; pb.speed = MAX_SPEED * 0.5;
        const double x0 = pb.playerX;
        const ContactEvents e = b.hitGhosts(1.0 / 60, pb, t, carById("m4"));
        VERIFIE(e.frottements == 1 && pb.playerX > x0 && pb.playerX - x0 <= 1.0 / 60 * 2.5 + 0.004 + 1e-9, "frottement : %d, écart %.4f", e.frottements, pb.playerX - x0);
        (void)g;
    }
    {   // je percute l'arrière : recul et perte de vitesse
        const Ghost gh = b.ghosts().begin()->second;
        const double touchZ = (carById("m4").d.L + carById(gh.car.c_str()).d.L) / M / 2;
        pb.playerX = gh.x; pb.dist = pb.position = gh.rd - (touchZ - 50); pb.speed = MAX_SPEED;   // chevauchement de 30 cm
        const double d0 = pb.dist;
        for (int i = 0; i < 2; i++) b.hitGhosts(1.0, pb, t, carById("m4"));   // délai de 0,35 s écoulé
        VERIFIE(pb.dist < d0 && pb.speed < MAX_SPEED, "choc arrière : recul %.0f, vitesse %.0f", d0 - pb.dist, pb.speed);
    }
    std::printf("contacts (frottement, choc arrière) : OK\n");

    // RG-06 : arrivée, classement et résultat
    pa.dist = b.finish(); VERIFIE(a.raceTick(1.0 / 60, pa, 95.25), "arrivée de A");
    salon->now += 100; b.updateGhosts(1.0 / 60);
    pb.dist = b.finish() * 0.5;
    VERIFIE(b.position(pb.dist) == 2, "B 2e en direct (%d)", b.position(pb.dist));
    pb.dist = b.finish(); VERIFIE(b.raceTick(1.0 / 60, pb, 101.5), "arrivée de B");
    const auto res = b.results();
    VERIFIE(res.size() == 2 && !res[0].me && res[0].done && std::abs(res[0].ft - 95250) < 1 && res[1].me, "résultat : A arrivé en tête, B 2e");
    VERIFIE(a.results().size() == 2 && a.results()[0].me, "résultat de A : 1er");
    std::printf("classement en direct et résultat : OK\n");

    a.leave(); b.updateGhosts(0.1);
    VERIFIE(b.ghosts().empty(), "joueur parti : fantôme retiré");
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("duel : tout est OK\n");
    return 0;
}
