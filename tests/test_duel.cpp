// Duel en ligne (WFS-21) : graine identique au jeu web, deux joueurs dans un salon en mémoire avec le même trafic
// image par image, messages hostiles sans plantage, fantômes lissés et bornés, contacts, classement.
// usage : test_duel
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "../src/game/duel.h"
#include "../src/game/reseau_udp.h"   // WFS-83
#include "../src/game/reseau_peerjs.h"   // WFS-89
#include "../src/game/binarypack.h"
#include "../src/game/course5.h"   // WFS-189 : rechargerNitro (Duel sans trafic)
#include <chrono>
#include <thread>
#include "../src/game/player.h"
#include "../src/game/traffic.h"
using namespace wfs;
using namespace wfs::duel;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// WFS-83 : deux transports UDP en boucle locale (127.0.0.1) dans le même processus ; horloges d'appareils différentes
static double msReel() { using namespace std::chrono; return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count(); }
static void attendre(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
template <class F> static bool jusqua(F ok, int msMax) { for (int t = 0; t < msMax; t += 5) { if (ok()) return true; attendre(5); } return ok(); }
// WFS-89 : duel par Internet sur un réseau PeerJS simulé (mêmes messages BinaryPack que le jeu web)
static void testInternet() {
    using namespace wfs::reseau;
    auto net = std::make_shared<ReseauMemoire>();
    Session a, b;
    auto ta = std::make_unique<TransportPeerJs>(net->fabrique(), "K7QX"); TransportPeerJs* pa = ta.get();
    VERIFIE(a.join(std::move(ta), "k7qx", true, "Alice", "golf", "bleu", "plage/nuit"), "Internet : création");
    VERIFIE(pa->etat() == TransportPeerJs::Etat::Pret && pa->relais() && pa->monId() == "waforspeed-wfs-k7qx-v1", "Internet : le créateur devient le relais");
    auto tb = std::make_unique<TransportPeerJs>(net->fabrique(), "k7qx"); TransportPeerJs* pb = tb.get();
    VERIFIE(b.join(std::move(tb), "k7qx", false, "Bob", "m4", "", ""), "Internet : rejoindre");
    VERIFIE(pb->etat() == TransportPeerJs::Etat::Pret && !pb->relais(), "Internet : ID-TAKEN → invité connecté au relais");
    std::this_thread::sleep_for(std::chrono::milliseconds(40));   // groupage de 30 ms
    b.others(); a.others();
    VERIFIE(a.others().size() == 1 && b.others().size() == 1, "Internet : les deux joueurs ne se voient pas (%zu / %zu)", a.others().size(), b.others().size());
    if (b.others().size() == 1) {
        const auto vus = b.others(); const auto& h = vus[0].f;
        VERIFIE(str(h, "n") == "Alice" && str(h, "host") == "1" && str(h, "map") == "plage/nuit" && str(h, "car") == "golf", "Internet : présence de l'hôte vue par l'invité");
    }
    b.setReady(true); a.setReady(true);
    bool partiA = false, partiB = false;
    for (int i = 0; i < 100 && !(partiA && partiB); i++) {   // comme le jeu : les deux appareils lisent leur transport à chaque image
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        partiA = partiA || a.checkStart(); partiB = partiB || b.checkStart(); a.others(); b.others();
    }
    VERIFIE(partiA && partiB && a.seed() == b.seed(), "Internet : départ commun, même graine (%d %d)", partiA, partiB);
    VERIFIE(std::abs(a.trafficClock() - b.trafficClock()) < 0.05, "Internet : horloge commune (%.3f / %.3f)", a.trafficClock(), b.trafficClock());
    std::printf("duel Internet (PeerJS simulé) : relais, ID-TAKEN → invité, présences, départ commun, graine %u : OK\n", a.seed());

    // un « navigateur » (jeu web) rejoint le relais : présence au format du jeu web (nombres, host null, ft null)
    std::vector<std::vector<uint8_t>> recu;
    auto web = net->fabrique()();
    EvenementsPair ev; std::string webId;
    ev.ouvert = [&](const std::string& id) { webId = id; };
    ev.donnees = [&](const std::string&, std::vector<uint8_t> o) { recu.push_back(std::move(o)); };
    web->ouvrir("", ev); web->connecter(idRelais("k7qx"));
    wfs::bp::Valeur pr = wfs::bp::Valeur::objet();
    pr.ajoute("n", wfs::bp::Valeur::texte("Web")); pr.ajoute("st", wfs::bp::Valeur::texte("lobby")); pr.ajoute("r", wfs::bp::Valeur::nombre(0));
    pr.ajoute("d", wfs::bp::Valeur::nombre(1234)); pr.ajoute("x", wfs::bp::Valeur::nombre(-0.45)); pr.ajoute("ft", wfs::bp::Valeur::nul());
    pr.ajoute("host", wfs::bp::Valeur::nul()); pr.ajoute("car", wfs::bp::Valeur::texte("p911"));
    wfs::bp::Valeur m = wfs::bp::Valeur::objet(); m.ajoute("t", wfs::bp::Valeur::texte("p")); m.ajoute("presence", pr);
    web->envoyer(idRelais("k7qx"), wfs::bp::pack(m));
    bool vuWeb = false;
    for (const auto& p : a.others()) if (str(p.f, "n") == "Web") vuWeb = num(p.f, "d") == 1234 && num(p.f, "x") == -0.45 && str(p.f, "car") == "p911" && str(p.f, "host") == "";
    VERIFIE(vuWeb, "Internet : présence du jeu web lue par le relais natif");
    bool listeOk = false;   // ce que le navigateur reçoit : {t:'all', list} avec host: true (booléen), map et tod séparés, nombres
    for (const auto& o : recu) {
        wfs::bp::Valeur v; if (!wfs::bp::unpack(o, v) || !v.champ("list")) continue;
        for (const auto& l : v.champ("list")->tab)
            if (l.tab.size() == 2 && l.tab[0].s == "waforspeed-wfs-k7qx-v1") {
                const auto* h = l.tab[1].champ("host"); const auto* mp = l.tab[1].champ("map"); const auto* td = l.tab[1].champ("tod"); const auto* r = l.tab[1].champ("r");
                listeOk = h && h->type == wfs::bp::Valeur::Booleen && h->b && mp && mp->s == "plage" && td && td->s == "nuit" && r && r->type == wfs::bp::Valeur::Nombre;
            }
    }
    VERIFIE(listeOk, "Internet : liste du relais au format du jeu web");
    // messages hostiles du « navigateur » : ignorés
    web->envoyer(idRelais("k7qx"), {0xdd, 0x7f, 0xff, 0xff, 0xff}); web->envoyer(idRelais("k7qx"), {0x82, 0xc4}); web->envoyer(idRelais("k7qx"), wfs::bp::pack(wfs::bp::Valeur::texte("p")));
    VERIFIE(a.others().size() == 2, "Internet : messages hostiles");
    web->fermer();
    VERIFIE(a.others().size() == 1, "Internet : départ du navigateur");
    // l'invité part : le relais le retire ; le relais part : l'invité perd la connexion
    b.leave();
    VERIFIE(a.others().empty(), "Internet : départ de l'invité");
    Session c; auto tc = std::make_unique<TransportPeerJs>(net->fabrique(), "k7qx"); TransportPeerJs* pc = tc.get();
    c.join(std::move(tc), "k7qx", false, "Cleo", "golf", "", "");
    VERIFIE(pc->etat() == TransportPeerJs::Etat::Pret, "Internet : nouvel invité");
    a.leave();
    VERIFIE(pc->etat() == TransportPeerJs::Etat::Perdu && c.others().empty(), "Internet : relais parti → connexion perdue");
    c.leave();
    // hors ligne : échec propre (note orange du salon), aucun plantage
    net->horsLigne = true;
    TransportPeerJs hors(net->fabrique(), "zz99");
    VERIFIE(hors.etat() == TransportPeerJs::Etat::Echec, "Internet : hors ligne");
    TransportPeerJs sansReseau(nullptr, "zz99");
    VERIFIE(sansReseau.etat() == TransportPeerJs::Etat::Echec && sansReseau.peers().empty(), "Internet : version sans réseau PeerJS");
    net->horsLigne = false;
    // deux transports à la fois : l'adversaire arrive par Internet → Internet est gardé, le Wi-Fi local est quitté
    auto salon = std::make_shared<SalonMemoire>();
    Session d, e;
    d.join(std::make_unique<TransportDouble>(std::make_unique<TransportMemoire>(salon, "D"), std::make_unique<TransportPeerJs>(net->fabrique(), "ab23")), "ab23", true, "Dan", "golf", "", "ville/jour");
    e.join(std::make_unique<TransportPeerJs>(net->fabrique(), "ab23"), "ab23", false, "Eve", "golf", "", "");
    std::this_thread::sleep_for(std::chrono::milliseconds(40)); e.others();
    const auto vusD = d.others();
    VERIFIE(vusD.size() == 1 && str(vusD[0].f, "n") == "Eve" && salon->joueurs.empty(), "Internet : double transport, Internet choisi");
    d.leave(); e.leave();
    std::printf("duel Internet : présence du jeu web lue, liste au format web, messages hostiles ignorés, départs, hors ligne, double transport : OK\n");
}

static void testReseau() {
    using namespace wfs::reseau;
    double avanceB = 0;   // l'horloge de B avance de 777 777 ms sur celle de A (appareils différents), + avanceB (test des 3 s)
    auto pa = std::make_shared<Prise>(Config{47401, {{"127.0.0.1", 47402}}, [] { return msReel(); }});
    auto pb = std::make_shared<Prise>(Config{47402, {{"127.0.0.1", 47401}}, [&] { return msReel() + 777777 + avanceB; }});
    if (!pa->ouverte() || !pb->ouverte()) { std::printf("réseau local indisponible : test UDP sauté\n"); return; }
    // RG-05 : messages hostiles refusés par l'analyse (aucun plantage)
    Recu r;
    VERIFIE(!Prise::lire(std::string(600, 'x'), r) && !Prise::lire("WFS2|k7qx|abcdefgh|12|n=a", r) && !Prise::lire("WFS1|k7qx|BAD|12|n=a", r) &&
            !Prise::lire("WFS1|k7qx|abcdefgh|-1|n=a", r) && !Prise::lire("WFS1|k7qx|abcdefgh|12|n=%ZZ", r) && !Prise::lire("WFS1|k7|abcdefgh|12|n=a", r) &&
            !Prise::lire("WFS1|k7qx|abcdefgh", r), "message hostile accepté");
    VERIFIE(Prise::lire("WFS1|K7QX|abcdefgh|12|n=Al|ice;st=lobby", r) && r.code == "k7qx" && duel::str(r.f, "n") == "Al|ice", "message valide refusé");
    // RG-08 / RG-02 : l'hôte annonce son salon, l'appareil de B (sur l'accueil, sans salon) le voit
    Session a, b;
    VERIFIE(a.join(std::make_unique<TransportUdp>(pa, "k7qx"), "k7qx", true, "Alice", "golf", "", "plage/nuit"), "UDP : création du salon");
    bool vu = jusqua([&] { a.others(); pb->pomper(); auto l = salonsOuverts(*pb); return l.size() == 1 && l[0].code == "k7qx" && l[0].hote == "Alice" && l[0].carte == "plage/nuit"; }, 2000);
    VERIFIE(vu, "UDP : le salon annoncé n'est pas vu par l'autre appareil");
    // rejoindre, puis le salon n'est plus annoncé (2 pilotes au plus)
    VERIFIE(b.join(std::make_unique<TransportUdp>(pb, "K7QX"), "K7QX", false, "Bob", "m4", "", ""), "UDP : rejoindre");
    VERIFIE(jusqua([&] { return a.others().size() == 1 && b.others().size() == 1; }, 2000), "UDP : les deux joueurs ne se voient pas");
    VERIFIE(jusqua([&] { a.others(); b.others(); return salonsOuverts(*pb).empty(); }, 2000), "UDP : salon plein encore annoncé");
    // prêts → départ commun, même graine, horloge commune (trafic identique) à ± 50 ms
    b.setReady(true); a.setReady(true);
    bool partiA = false, partiB = false;
    jusqua([&] { partiA = partiA || a.checkStart(); partiB = partiB || b.checkStart(); return partiA && partiB; }, 3000);
    VERIFIE(partiA && partiB, "UDP : départ non commun (A %d, B %d)", partiA, partiB);
    VERIFIE(a.seed() == b.seed() && a.round() == 1 && b.round() == 1, "UDP : graines %u / %u", a.seed(), b.seed());
    jusqua([&] { a.others(); b.others(); return false; }, 400);   // quelques messages de l'hôte : moyenne du décalage
    const double ecart = std::abs(a.trafficClock() - b.trafficClock()) * 1000;
    VERIFIE(ecart <= 50, "UDP : trafficClock A %.3f s, B %.3f s (écart %.0f ms)", a.trafficClock(), b.trafficClock(), ecart);
    std::printf("duel UDP : salon vu puis plein, départ commun, graine %u, horloges d'appareils à 777 777 ms d'écart → trafficClock à %.0f ms près : OK\n", a.seed(), ecart);
    // message hostile reçu par la prise : ignoré, le duel continue
    const int avant = pb->ignores();
    pa->envoyer("WFS2|k7qx|abcdefgh|12|n=x"); pa->envoyer("WFS1|k7qx|abc|12|n=x"); pa->envoyer("WFS1|k7qx|abcdefgh|12|n=%Z");
    jusqua([&] { b.others(); return pb->ignores() >= avant + 3; }, 1000);
    VERIFIE(pb->ignores() >= avant + 3 && b.others().size() == 1, "UDP : messages hostiles (%d ignorés, %zu joueur(s))", pb->ignores() - avant, b.others().size());
    // RG-06 : bye → retrait immédiat
    a.leave();
    VERIFIE(jusqua([&] { return b.others().empty(); }, 500), "UDP : bye non pris en compte");
    // RG-06 : plus rien depuis 3 s → retiré (l'horloge de B avance de 3,1 s ; C ne parle plus)
    Session c, d;
    c.join(std::make_unique<TransportUdp>(pa, "zz99"), "zz99", true, "Cleo", "golf", "", "ville/jour");
    d.join(std::make_unique<TransportUdp>(pb, "zz99"), "zz99", false, "Dan", "golf", "", "");
    VERIFIE(jusqua([&] { c.others(); return d.others().size() == 1; }, 2000), "UDP : second salon");
    avanceB += 2900; VERIFIE(d.others().size() == 1, "UDP : retiré avant 3 s");
    avanceB += 200; VERIFIE(d.others().empty(), "UDP : pas retiré après 3 s sans message");
    std::printf("duel UDP : messages hostiles ignorés, bye immédiat, retrait après 3 s de silence : OK\n");
    b.leave(); c.leave(); d.leave();
}

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
    // WFS-242 (RG-05) : grille du duel : chaque joueur à sa place (créateur devant), la même vue de tous les téléphones ;
    // la distance d'un adversaire sur la grille (derrière la ligne, négative) est lue telle quelle (avant : ramenée à 0)
    {
        VERIFIE(a.place() == 0 && b.place() == 1, "WFS-242 RG-05 : places : créateur %d, invité %d", a.place(), b.place());
        auto s2 = std::make_shared<SalonMemoire>();
        Session h, i1, i2;
        h.join(std::make_unique<TransportMemoire>(s2, "H"), "m3qx", true, "Hugo", "golf", "", "ville");
        i1.join(std::make_unique<TransportMemoire>(s2, "I"), "m3qx", false, "Ines", "m4", "", "");
        i2.join(std::make_unique<TransportMemoire>(s2, "J"), "m3qx", false, "Jade", "p208", "", "");
        h.setReady(true); i1.setReady(true); i2.setReady(true);
        const bool d1 = i1.checkStart(), d2 = i2.checkStart(), d0 = h.checkStart();
        const int p0 = h.place(), p1 = i1.place(), p2 = i2.place();
        VERIFIE(d0 && d1 && d2 && p0 == 0 && ((p1 == 1 && p2 == 2) || (p1 == 2 && p2 == 1)), "WFS-242 RG-05 : 3 joueurs : places %d, %d, %d", p0, p1, p2);
        Player ph; ph.reset(carById("golf")); ph.dist = ph.position = -11.5 / M;   // 2e place de la grille
        h.raceTick(1.0 / 60, ph, 0);
        s2->now += 50; i1.updateGhosts(1.0 / 60);
        const auto it = i1.ghosts().begin();
        const double vu = i1.ghosts().empty() ? 0 : it->second.rd * M;
        VERIFIE(i1.ghosts().size() >= 1 && std::abs(vu + 11.5) < 0.01, "WFS-242 RG-05 : adversaire sur la grille vu à %.2f m (attendu −11,50)", vu);
        std::printf("WFS-242 RG-05 : grille du duel : places %d / %d / %d (créateur devant), adversaire vu à %.2f m derrière la ligne : OK\n", p0, p1, p2, -vu);
    }

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
    testReseau();   // WFS-83
    testInternet(); // WFS-89
    {   // WFS-189 (RG-04, RG-05) : en Duel, nitro rechargée au temps seulement (pas de bonus de dépassement, équité réseau) ;
        // les deux pilotes ont la même règle : même jauge après la même course, chacun la calcule chez lui
        double j1 = config().nitro.depart, j2 = config().nitro.depart; const double dt = 1.0 / 120;
        for (int i = 0; i < 120 * 30; i++) { j1 = course5::rechargerNitro(j1, false, dt, 0); j2 = course5::rechargerNitro(j2, false, dt, 0); }
        double j3 = config().nitro.depart; for (int i = 0; i < 120 * 30; i++) j3 = course5::rechargerNitro(j3, i < 120 * 2, dt, 0);   // 2 s de nitro
        std::printf("duel : nitro après 30 s : %.3f et %.3f (attendu %.3f) ; avec 2 s de nitro tenue : %.3f (rien pendant la nitro)\n",
                    j1, j2, config().nitro.depart + 30 * config().nitro.rechargeTemps, j3);
        VERIFIE(j1 == j2 && std::abs(j1 - (config().nitro.depart + 30 * config().nitro.rechargeTemps)) < 1e-6, "duel : recharge au temps");
        VERIFIE(std::abs(j3 - (config().nitro.depart + 28 * config().nitro.rechargeTemps)) < 1e-6, "duel : pas de recharge pendant la nitro");
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("duel : tout est OK\n");
    return 0;
}
