// Duel entre amis par Internet (WFS-112) : liens d'invitation, WebSocket maison face à des trames hostiles, présences
// hostiles du relais sans plantage (valeurs bornées), puis bout en bout avec le vrai relais lancé en local
// (node serveur/local.js) : deux clients simulés, code et lien, prêts, même course, positions et arrivée cohérentes,
// ping, reconnexion, revanche, code inconnu, code expiré ; enfin le test JS du relais (serveur/test/bout_en_bout.js).
// usage : test_relais   (Node absent : la partie de bout en bout est sautée, avec un message)
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <thread>
#include "../src/game/duel.h"
#include "../src/game/player.h"
#include "../src/game/trace_cartes.h"
#include "../src/game/reseau_relais.h"
#include "../src/game/trace_cartes.h"
using namespace wfs;
using namespace wfs::duel;
using namespace wfs::reseau;

#ifndef WFS_SOURCE_DIR
#define WFS_SOURCE_DIR "."
#endif

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)
static void attendre(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
template <class F> static bool jusqua(F ok, int msMax) { for (int t = 0; t < msMax; t += 10) { if (ok()) return true; attendre(10); } return ok(); }

// ---------------------------------------------------------------- flux en mémoire : le test joue le serveur
struct FluxMemoire : Flux {
    std::string envoye, aLire; Etat e = Etat::Ouvert;
    Etat etat() const override { return e; }
    void envoyer(const std::string& o) override { envoye += o; }
    std::string lire() override { std::string r; r.swap(aLire); return r; }
    void fermer() override { e = Etat::Ferme; }
    void couper() override { e = Etat::Ferme; }
};
static std::string trameServeur(int op, const std::string& p) {
    std::string t; t += (char)(0x80 | op);
    if (p.size() < 126) t += (char)p.size();
    else { t += (char)126; t += (char)(p.size() >> 8); t += (char)(p.size() & 255); }
    return t + p;
}
// réponse 101 correcte à la poignée de main envoyée par le client
static std::string reponse101(const std::string& requete) {
    const size_t k = requete.find("Sec-WebSocket-Key: ");
    const std::string cle = requete.substr(k + 19, requete.find("\r\n", k) - k - 19);
    return "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " +
           base64(sha1(cle + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")) + "\r\n\r\n";
}

static void testLiens() {
    AdresseRelais a; a.hote = "relais.exemple.dev";
    VERIFIE(lienInvitation("K7QX", a) == "https://relais.exemple.dev/d/k7qx", "lien : %s", lienInvitation("K7QX", a).c_str());
    AdresseRelais l{"127.0.0.1", 8787, false};
    VERIFIE(lienInvitation("ab23", l) == "http://127.0.0.1:8787/d/ab23", "lien local");
    std::string c;
    VERIFIE(lireLienInvitation("waforspeed://duel/K7QX", c) && c == "k7qx", "lien profond waforspeed://duel/K7QX");
    VERIFIE(lireLienInvitation("Viens ! code K7QX : https://x.workers.dev/d/k7qx", c) && c == "k7qx", "texte partagé");
    VERIFIE(lireLienInvitation("intent://duel/ab23#Intent;scheme=waforspeed;end", c) && c == "ab23", "lien intent://");
    VERIFIE(!lireLienInvitation("waforspeed://duel/k7q", c) && !lireLienInvitation("https://x/d/k7qxy", c) && !lireLienInvitation("https://x/d/k7-x", c) &&
            !lireLienInvitation(std::string(600, 'a') + "/d/k7qx", c) && !lireLienInvitation("", c), "liens invalides refusés");
    const std::string t = texteInvitation("k7qx");
    VERIFIE(t.find("K7QX") != std::string::npos && lireLienInvitation(t, c) && c == "k7qx", "texte d'invitation : %s", t.c_str());
    VERIFIE(base64(sha1("abc")) == "qZk+NkcGgWq6PiVxeFDCbJzQ2J0=", "SHA-1 (vecteur « abc »)");
    std::printf("liens d'invitation, SHA-1, base64 : OK\n");
}

static void testTramesHostiles() {
    // poignée refusée (mauvaise clé)
    { auto f = std::make_unique<FluxMemoire>(); FluxMemoire* fm = f.get();
      ClientWs ws(std::move(f), "h", "/salon/k7qx?id=abcdef");
      fm->aLire = "HTTP/1.1 101 OK\r\nSec-WebSocket-Accept: faux\r\n\r\n";
      VERIFIE(ws.etat() == ClientWs::Etat::Ferme, "poignée de main avec une fausse clé refusée"); }
    // réponse sans fin, trop longue
    { auto f = std::make_unique<FluxMemoire>(); FluxMemoire* fm = f.get();
      ClientWs ws(std::move(f), "h", "/");
      fm->aLire = std::string(5000, 'x');
      VERIFIE(ws.etat() == ClientWs::Etat::Ferme, "réponse trop longue refusée"); }
    const std::string hostiles[] = {
        std::string("\x81\x85\x01\x02\x03\x04" "abcde", 11),                  // trame masquée venant du serveur
        std::string("\x81\x7f\x00\x00\x00\x00\x7f\xff\xff\xff", 10),       // longueur énorme
        std::string("\x82\x03\x01\x02\x03", 5),                            // binaire
        std::string("\xf1\x01" "x", 3),                                       // bits réservés
        std::string("\x01\x02" "ab" "\x01\x02" "cd", 8),                            // texte coupé puis nouveau texte
    };
    int i = 0;
    for (const std::string& h : hostiles) {
        auto f = std::make_unique<FluxMemoire>(); FluxMemoire* fm = f.get();
        ClientWs ws(std::move(f), "h", "/");
        fm->aLire = reponse101(fm->envoye) + trameServeur(1, "P|abcdef|12|n=Ok");
        VERIFIE(ws.etat() == ClientWs::Etat::Ouvert && ws.recevoir().size() == 1, "trame %d : poignée et premier message", i);
        ws.injecter(h);
        VERIFIE(ws.etat() == ClientWs::Etat::Ferme && ws.erreurs() > 0, "trame hostile %d : connexion fermée sans plantage", i);
        i++;
    }
    // ping du serveur → pong ; message en deux morceaux recollé
    { auto f = std::make_unique<FluxMemoire>(); FluxMemoire* fm = f.get();
      ClientWs ws(std::move(f), "h", "/");
      fm->aLire = reponse101(fm->envoye); ws.etat(); fm->envoye.clear();
      ws.injecter(std::string("\x89\x02hi", 4) + std::string("\x01\x03P|a", 5) + std::string("\x80\x02" "bc", 4));
      const auto r = ws.recevoir();
      VERIFIE(r.size() == 1 && r[0] == "P|abc" && (uint8_t)fm->envoye[0] == 0x8A, "ping → pong, morceaux recollés"); }
    std::printf("WebSocket maison : %d trames hostiles refusées sans plantage : OK\n", i);

    // présences hostiles venant du « relais » : ignorées ou bornées, jamais de plantage
    FluxMemoire* fm = nullptr;
    auto fab = [&](const std::string&, int, bool) { auto f = std::make_unique<FluxMemoire>(); fm = f.get(); return std::unique_ptr<Flux>(std::move(f)); };
    double horloge = 1000;
    Session s;
    auto t = std::make_unique<TransportRelais>(fab, AdresseRelais{"h", 80, false}, "k7qx", true, [&] { return horloge; });
    TransportRelais* tr = t.get();
    VERIFIE(s.join(std::move(t), "k7qx", true, "Moi", "golf", "", "ville/jour"), "transport en mémoire");
    fm->aLire = reponse101(fm->envoye) + trameServeur(1, "OK|5000|905000");
    s.others();
    VERIFIE(tr->lien() == Lien::Pret && std::abs(tr->expireDansMs() - 900000) < 1, "accueil du relais, code valable 15 min (%.0f)", tr->expireDansMs());
    const std::string msgs[] = {
        "P|ABCDEF|10|n=Maj", "P|abc def|10|n=x", "P|abcdef|NaN|n=x", "P|abcdef|-5|n=x", "P|abcdef|1e400|n=x", "P|abcdef|10|n=%ZZ",
        "P|abcdef|10|" + std::string(600, 'a'), "P|abcdef", "P|", "B|A B", "pong|x|y", "pong|999999|5", "E|n'importe", "chat|salut",
        std::string("\xff\xfe", 2), "P|" + tr->monId() + "|10|n=Moi",
    };
    for (const std::string& m : msgs) if (m.rfind("E|", 0) != 0) fm->aLire += trameServeur(1, m);
    // valeurs énormes : bornées par la session (RG-06)
    fm->aLire += trameServeur(1, "P|rival1|6000|n=Rival;st=race;r=1;d=1e300;x=-999;v=1e9;h=77;b=9;ft=-3;car=inconnue;trace=" + trace::signature());
    for (int k = 0; k < 20; k++) fm->aLire += trameServeur(1, "P|j" + std::to_string(100000 + k) + "|6000|n=J;st=lobby;trace=" + trace::signature());
    s.others();
    VERIFIE(s.others().size() == JOUEURS_MAX, "au plus %zu autres joueurs retenus (%zu)", JOUEURS_MAX, s.others().size());
    VERIFIE(tr->ignores() >= 13, "messages hostiles ignorés (%d)", tr->ignores());
    VERIFIE(tr->lien() == Lien::Pret, "toujours connecté après les messages hostiles");
    fm->aLire += trameServeur(1, "E|plein");
    VERIFIE(tr->lien() == Lien::Perdu && tr->raison() == "plein", "refus du relais après l'accueil → perdu (%s)", tr->raison().c_str());
    std::printf("présences hostiles du relais : ignorées ou bornées, %d message(s) ignoré(s) : OK\n", tr->ignores());
    s.leave();

    // session : fantôme d'un adversaire aux valeurs énormes, borné
    Session a;
    auto ta = std::make_unique<TransportRelais>(fab, AdresseRelais{"h", 80, false}, "ab23", false, [&] { return horloge; });
    a.join(std::move(ta), "ab23", false, "A", "golf", "", "");
    fm->aLire = reponse101(fm->envoye) + trameServeur(1, "OK|1000|2000");
    a.others();
    fm->aLire += trameServeur(1, "P|rival1|1000|n=R;st=ready;r=0;host=1;km=2;trace=" + trace::signature());
    a.setReady(true);
    VERIFIE(a.checkStart(), "départ face à un adversaire du relais");
    fm->aLire += trameServeur(1, "P|rival1|1000|n=R;st=race;r=1;d=1e300;x=-999;v=1e9;h=77;b=9;ft=-3;car=inconnue;trace=" + trace::signature());
    a.updateGhosts(0.1);
    bool borne = false;
    for (const auto& [id, g] : a.ghosts()) borne = g.rd <= a.finish() + 1e-6 && g.x >= -2 && g.h <= 3.2 && g.b <= 2 && g.car == "golf";
    VERIFIE(borne, "fantôme borné (distance, écart, cap, voiture)");
    a.leave();

    // WFS-136 (RG-07) : adversaire sur un autre tracé (ancienne version, sans champ « trace ») : « version », pas de course
    Session v;
    auto tv = std::make_unique<TransportRelais>(fab, AdresseRelais{"h", 80, false}, "cd45", false, [&] { return horloge; });
    TransportRelais* trv = tv.get();
    v.join(std::move(tv), "cd45", false, "V", "golf", "", "");
    fm->aLire = reponse101(fm->envoye) + trameServeur(1, "OK|1000|2000");
    v.others();
    fm->aLire += trameServeur(1, "P|vieux1|1000|n=Vieux;st=ready;r=0;host=1;km=2");
    v.setReady(true);
    VERIFIE(!v.checkStart() && v.others().empty(), "autre tracé : pas de course");
    VERIFIE(trv->lien() == Lien::Echec && trv->raison() == "version", "autre tracé : refus « version » (%s)", trv->raison().c_str());
    std::printf("adversaire sur un autre tracé : refusé (« Mets ton jeu à jour ») : OK\n");
    v.leave();
}

// ---------------------------------------------------------------- bout en bout avec le relais local (Node)
struct Joueur {
    Session s; TransportRelais* t = nullptr; Player pl;
    bool join(const AdresseRelais& adr, const std::string& code, bool hote, const char* nom, const char* car) {
        auto tr = std::make_unique<TransportRelais>(fluxDefaut, adr, code, hote); t = tr.get();
        return s.join(std::move(tr), code, hote, nom, car, "", hote ? "plage/nuit" : "");
    }
};

static void testBoutEnBout() {
    RelaisLocal relais;
    if (!relais.demarrer(std::string(WFS_SOURCE_DIR) + "/serveur/local.js", {"--expiration", "6000", "--grace", "5000"})) {
        std::printf("node absent ou relais local impossible : bout en bout sauté\n");
        return;
    }
    const AdresseRelais adr{"127.0.0.1", relais.port(), false};
    const auto debut = std::chrono::steady_clock::now();
    adresseRelais() = adr;
    // 1) l'hôte crée le duel : code à 4 caractères ; l'ami reçoit le lien et rejoint par le code du lien
    Joueur a, b;
    const std::string code = makeCode([] { return (uint32_t)std::rand(); });
    VERIFIE(a.join(adr, code, true, "Alex", "golf"), "création du duel %s", code.c_str());
    a.s.setKm(2);
    VERIFIE(jusqua([&] { return a.t->lien() == Lien::Pret; }, 3000), "l'hôte est connecté au relais");
    std::string recu;
    VERIFIE(lireLienInvitation(texteInvitation(code), recu) && recu == code, "lien d'invitation → code");
    VERIFIE(b.join(adr, recu, false, "Nadia", "m4"), "l'ami rejoint par le lien");
    VERIFIE(jusqua([&] { const size_t na = a.s.others().size(), nb = b.s.others().size(); return na == 1 && nb == 1; }, 3000), "les deux joueurs se voient (%zu / %zu, ignorés %d / %d)", a.s.others().size(), b.s.others().size(), a.t->ignores(), b.t->ignores());
    VERIFIE(jusqua([&] { b.s.checkStart(); return b.s.km() == 2; }, 2000), "l'ami suit la distance de l'hôte (%d km)", b.s.km());
    VERIFIE(jusqua([&] { const int x = a.t->pingMs(), y = b.t->pingMs(); return x >= 0 && y >= 0; }, 3000), "ping affiché (%d / %d ms)", a.t->pingMs(), b.t->pingMs());
    VERIFIE(std::abs(a.s.transport()->nowMs() - b.s.transport()->nowMs()) < 50, "horloge commune (écart %.1f ms)", a.s.transport()->nowMs() - b.s.transport()->nowMs());
    std::printf("salon %s : 2 joueurs par le code et le lien, ping %d ms : OK\n", code.c_str(), a.t->pingMs());

    // 2) prêts → départ commun, même graine (même course)
    a.s.setReady(true); b.s.setReady(true);
    bool pa = false, pb = false;
    jusqua([&] { pa = pa || a.s.checkStart(); pb = pb || b.s.checkStart(); return pa && pb; }, 10000);   // WFS-135 (ANO-61) : 10 s (CI chargée)
    VERIFIE(pa && pb && a.s.seed() == b.s.seed() && a.s.round() == 1 && b.s.round() == 1, "départ commun, même graine (%u / %u ; partis %d / %d, manche %d / %d, horloge %.3f / %.3f)",
            a.s.seed(), b.s.seed(), pa, pb, a.s.round(), b.s.round(), a.s.trafficClock(), b.s.trafficClock());
    VERIFIE(std::abs(a.s.trafficClock() - b.s.trafficClock()) < 0.1, "même horloge de course (%.3f / %.3f, manche %d / %d)", a.s.trafficClock(), b.s.trafficClock(), a.s.round(), b.s.round());

    // 3) course : positions envoyées 10 fois par seconde, vues lissées chez l'autre
    a.pl.reset(carById("golf")); b.pl.reset(carById("m4"));
    const double vA = 40, vB = 30;   // unités / s
    for (int i = 0; i < 90; i++) {   // 1,5 s de course
        const double dt = 1.0 / 60;
        a.pl.dist += vA * dt; a.pl.speed = vA; a.pl.playerX = 0.3;
        b.pl.dist += vB * dt; b.pl.speed = vB; b.pl.playerX = -0.45;
        a.s.raceTick(dt, a.pl, i * dt); b.s.raceTick(dt, b.pl, i * dt);
        a.s.updateGhosts(dt); b.s.updateGhosts(dt);
        attendre(16);
    }
    const Ghost* gA = b.s.ghosts().count(a.t->monId()) ? &b.s.ghosts().at(a.t->monId()) : nullptr;
    const Ghost* gB = a.s.ghosts().count(b.t->monId()) ? &a.s.ghosts().at(b.t->monId()) : nullptr;
    VERIFIE(gA && gB, "chacun voit la voiture de l'autre");
    if (gA && gB) {
        VERIFIE(std::abs(gA->rd - a.pl.dist) < 12 && std::abs(gB->rd - b.pl.dist) < 12, "positions cohérentes (A %.1f vu %.1f ; B %.1f vu %.1f)", a.pl.dist, gA->rd, b.pl.dist, gB->rd);
        VERIFIE(std::abs(gA->x - 0.3) < 0.05 && std::abs(gB->x + 0.45) < 0.05 && gA->name == "Alex" && gB->name == "Nadia" && gB->car == "m4", "écart latéral, nom et voiture");
        VERIFIE(a.s.position(a.pl.dist) == 1 && b.s.position(b.pl.dist) == 2, "classement en direct (A 1er, B 2e)");
        std::printf("course : A %.1f (vu par B %.1f), B %.1f (vu par A %.1f) : OK\n", a.pl.dist, gA->rd, b.pl.dist, gB->rd);
    }

    // 4) messages hostiles envoyés au relais par un intrus : le relais et la course continuent
    {
        auto intrus = fluxTcp("127.0.0.1", relais.port());
        intrus->envoyer(std::string("GET /salon/") + code + "?id=pirate1&hote=0 HTTP/1.1\r\nHost: x\r\n\r\n\x00\x01\xff garbage\r\n\r\n");
        auto intrus2 = fluxTcp("127.0.0.1", relais.port());
        intrus2->envoyer("\x16\x03\x01 pas du HTTP");
        attendre(200);
    }
    const double avant = gA ? gA->rd : 0;
    for (int i = 0; i < 20; i++) { a.pl.dist += vA / 60; a.s.raceTick(1.0 / 60, a.pl, 2); b.s.updateGhosts(1.0 / 60); attendre(16); }
    VERIFIE(b.s.ghosts().count(a.t->monId()) && b.s.ghosts().at(a.t->monId()).rd > avant, "après l'intrus, la course continue");

    // 5) reconnexion (RG-05) : coupure nette chez A ; A revient seul, B le voit toujours
    a.t->couper();
    VERIFIE(jusqua([&] { b.s.others(); return a.t->lien() == Lien::Reconnexion; }, 9000), "coupure détectée (reconnexion)");
    VERIFIE(jusqua([&] { b.s.others(); return a.t->lien() == Lien::Pret && a.t->reconnexions() == 1; }, 5000), "reconnecté automatiquement (%d)", a.t->reconnexions());
    VERIFIE(a.s.others().size() == 1 && b.s.others().size() == 1, "après la reconnexion, les deux joueurs se voient encore");
    std::printf("reconnexion automatique : OK\n");

    // 6) arrivée : A passe la ligne le premier, puis B ; les deux écrans de fin donnent le même classement
    a.pl.dist = a.s.finish(); VERIFIE(a.s.raceTick(0.016, a.pl, 41.5), "arrivée de A");
    attendre(100);
    b.pl.dist = b.s.finish(); VERIFIE(b.s.raceTick(0.016, b.pl, 43.25), "arrivée de B");
    jusqua([&] { a.s.updateGhosts(0.016); b.s.updateGhosts(0.016);
                 const auto ra = a.s.results(), rb = b.s.results();
                 return ra.size() == 2 && rb.size() == 2 && ra[1].done && rb[0].done && rb[0].ft > 0; }, 2000);
    const auto ra = a.s.results(), rb = b.s.results();
    VERIFIE(ra.size() == 2 && rb.size() == 2 && ra[0].me && !rb[0].me && rb[1].me, "même classement des deux côtés (A 1er)");
    if (rb.size() == 2) VERIFIE(std::abs(rb[0].ft - 41500) < 1 && std::abs(ra[1].ft - 43250) < 1, "temps d'arrivée reçus (%.0f / %.0f ms)", rb[0].ft, ra[1].ft);
    std::printf("arrivée : A 41,50 s, B 43,25 s, même classement chez les deux : OK\n");

    // 7) revanche : les deux « prêts » → manche 2, nouvelle graine commune
    const uint32_t g1 = a.s.seed();
    a.s.setReady(true); b.s.setReady(true);
    pa = pb = false;
    jusqua([&] { pa = pa || a.s.checkStart(); pb = pb || b.s.checkStart(); return pa && pb; }, 3000);
    VERIFIE(pa && pb && a.s.round() == 2 && b.s.round() == 2 && a.s.seed() == b.s.seed() && a.s.seed() != g1, "revanche : manche 2, même graine (%u)", a.s.seed());

    // 8) code inconnu ; départ de B annoncé tout de suite ; code expiré pour un nouveau venu
    Joueur c;
    c.join(adr, code == "zz99" ? "zz98" : "zz99", false, "Perdu", "golf");
    VERIFIE(jusqua([&] { a.s.others(); return c.t->lien() == Lien::Echec; }, 3000) && c.t->raison() == "inconnu", "code inconnu → « %s »", c.t->raison().c_str());
    b.s.leave();
    VERIFIE(jusqua([&] { return a.s.others().empty(); }, 2000), "départ de l'ami vu tout de suite");
    const double ecoule = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - debut).count();
    if (ecoule < 6300) attendre((int)(6300 - ecoule));
    Joueur d;
    d.join(adr, code, false, "Tard", "golf");
    VERIFIE(jusqua([&] { a.s.others(); return d.t->lien() == Lien::Echec; }, 3000) && d.t->raison() == "expire", "code expiré → « %s »", d.t->raison().c_str());
    VERIFIE(a.t->lien() == Lien::Pret, "l'hôte reste dans son salon après l'expiration du code");
    std::printf("code inconnu, départ, code expiré (6 s ici, 15 min en vrai) : OK\n");
    a.s.leave(); c.s.leave(); d.s.leave();
    adresseRelais() = AdresseRelais{};

    // 9) test JS du relais (deux clients simulés, messages hostiles, salon plein, reconnexion, lien d'invitation)
    const int r = std::system("node " WFS_SOURCE_DIR "/serveur/test/bout_en_bout.js > /dev/null 2>&1");
    VERIFIE(r == 0, "serveur/test/bout_en_bout.js (code %d)", r);
    if (r == 0) std::printf("serveur/test/bout_en_bout.js : OK\n");
}

int main() {
    std::srand((unsigned)std::chrono::steady_clock::now().time_since_epoch().count());
    testLiens();
    testTramesHostiles();
    testBoutEnBout();
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("duel entre amis (relais) : tout est OK\n");
    return 0;
}
