// WFS-125 (RG-08) : records par mode et fantôme de ton record. Comparaisons RG-02 (les 6 cas Poursuite, place / temps
// Course à 5), relecture d'une ancienne sauvegarde (RG-03), écriture / relecture d'une trace et interpolation (RG-05 /
// RG-06), limite de 6 000 échantillons, record Checkpoint inchangé (RG-01).
// usage : test_records [dossier temporaire] (par défaut : dossier courant)
#include <cmath>
#include <cstdio>
#include <string>
#include "../src/game/fantome.h"
#include "../src/game/sauvegarde.h"
using namespace wfs;

static int g_echecs = 0;
#define VERIFIER(c) do { if (!(c)) { std::printf("ÉCHEC ligne %d : %s\n", __LINE__, #c); g_echecs++; } } while (0)
static bool proche(double a, double b) { return std::abs(a - b) < 1e-9; }

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? std::string(argv[1]) + "/" : "";

    // 1) RG-02 Poursuite : les 6 cas
    using records::poursuiteBat;
    VERIFIER(poursuiteBat(true, 10, 10000, false, 0, 9000));     // victoire bat un « ARRÊTÉ » (même à petit score)
    VERIFIER(!poursuiteBat(false, 100, 9999, true, 1, 10000));   // « ARRÊTÉ » ne bat jamais une victoire
    VERIFIER(poursuiteBat(true, 80, 10000, true, 70, 10000));    // deux victoires : plus haut score
    VERIFIER(!poursuiteBat(true, 70, 10000, true, 70, 10000));   // score égal (ou plus bas) : non battu
    VERIFIER(poursuiteBat(false, 0, 4200, false, 0, 3100));      // deux « ARRÊTÉ » : plus grande distance
    VERIFIER(!poursuiteBat(false, 50, 3100, false, 0, 3100));    // distance égale : non battu (le score ne compte pas)
    VERIFIER(poursuiteBat(false, 0, 10, false, 0, -1));          // record vide : toujours battu

    // 2) RG-02 Course à 5 : place, puis temps à place égale
    using records::course5Bat;
    VERIFIER(course5Bat(3, 99000, 0, 0));          // vide
    VERIFIER(course5Bat(1, 99000, 2, 50000));      // meilleure place, même plus lente
    VERIFIER(!course5Bat(3, 10000, 2, 50000));     // moins bonne place
    VERIFIER(course5Bat(2, 49000, 2, 50000));      // même place, plus rapide
    VERIFIER(!course5Bat(2, 50000, 2, 50000));     // même place, même temps

    // 3) Sauvegarde : écriture / relecture des records, ancienne sauvegarde (RG-03), clés mal formées, Checkpoint inchangé
    {
        Sauvegarde s; s.defauts(); s.best = 48210;
        VERIFIER(s.texte().find("record-") == std::string::npos);   // records vides : aucune clé écrite
        VERIFIER(s.battreRecordCourse5(2, 61500)); VERIFIER(!s.battreRecordCourse5(3, 1000)); VERIFIER(s.battreRecordCourse5(2, 60000));
        VERIFIER(s.battreRecordPoursuite(false, 0, 3200)); VERIFIER(s.battreRecordPoursuite(true, 64, 10000)); VERIFIER(!s.battreRecordPoursuite(false, 90, 9000));
        VERIFIER(s.best == 48210);   // RG-01 : les autres modes ne touchent pas waforspeed-best
        Sauvegarde r; VERIFIER(r.lire(s.texte()));
        VERIFIER(r.best == 48210 && r.rec5Place == 2 && r.rec5Ms == 60000 && r.recPVictoire && r.recPScore == 64 && r.recPDistM == 10000);
        VERIFIER(s.texte().find("waforspeed-best=48210\n") != std::string::npos);
        VERIFIER(s.texte().find("record-course5=2;60000\n") != std::string::npos && s.texte().find("record-poursuite=1;64;10000\n") != std::string::npos);
    }
    {
        Sauvegarde a;   // ancienne sauvegarde (WFS-24) : sans les nouvelles clés
        VERIFIER(a.lire("version=1\nwaforspeed-best=955\ncar=m4\npaint-m4=rouge\nmap=circuit\nmode=poursuite\n"));
        VERIFIER(a.best == 955 && a.car == "m4" && a.map == "circuit" && a.mode == "poursuite" && !a.aRecordCourse5() && !a.aRecordPoursuite());
        Sauvegarde m;   // clés mal formées : ignorées, pas de plantage
        VERIFIER(m.lire("version=1\nrecord-course5=0;100\nrecord-poursuite=2;5;100\nrecord-course5=;\nrecord-poursuite=1;101;5\nrecord-course5=3\nrecord-poursuite=a;b;c\nwaforspeed-best=12\n"));
        VERIFIER(!m.aRecordCourse5() && !m.aRecordPoursuite() && m.best == 12);
        Sauvegarde k; VERIFIER(k.lire("version=1\nrecord-course5=6;100\nrecord-course5=4;123456789\nrecord-poursuite=0;0;4520\n"));
        VERIFIER(k.rec5Place == 4 && k.rec5Ms == 123456789 && !k.recPVictoire && k.recPDistM == 4520);
    }

    // 4) RG-05 : enregistrement toutes les 0,1 s, limite de 6 000 échantillons
    fantome::Enregistreur en;
    en.demarrer("ville", "m4", 0xb3122b);
    for (int i = 0; i <= 120 * 25; i++) {   // 25 s à 120 images/s
        const double t = i / 120.0;
        en.pas(t, {t * 1000, 0.3 * std::sin(t), 0.01 * t, 20 + t});
    }
    VERIFIER(en.trace.e.size() == 251 && !en.plein);
    VERIFIER(proche(en.trace.e[10].d, 1000.0));   // échantillon 10 = 1,0 s
    {
        fantome::Enregistreur g; g.demarrer("ville", "golf", 0);
        bool ok = true;
        for (int i = 0; i < 7000 && ok; i++) ok = g.pas(i * 0.1, {(double)i, 0, 0, 0});
        VERIFIER(!ok && g.plein && g.trace.e.size() == fantome::MAX_ECHANTILLONS);
        VERIFIER(!g.pas(800, {}) && g.trace.e.size() == fantome::MAX_ECHANTILLONS);   // au-delà : on arrête d'enregistrer
        en.trace.score = 0;
        const std::string b = fantome::encoder(g.trace);
        fantome::Trace r; VERIFIER(fantome::decoder(b, r) && r.e.size() == fantome::MAX_ECHANTILLONS && proche(r.e.back().d, 5999));
    }

    // 5) écriture / relecture d'un fichier (thread de travail de la sauvegarde), interpolation (RG-06)
    en.trace.score = 51234;
    const std::string chemin = dir + "test-records.txt";
    std::remove(chemin.c_str());
    Sauvegarde s; s.charger(chemin);
    const std::string cf = s.cheminVoisin(fantome::nomFichier("ville"));
    VERIFIER(cf == dir + "fantome-ville.bin");
    s.enregistrerFichier(cf, fantome::encoder(en.trace)); s.enregistrer(); s.attendre();
    fantome::Trace lu;
    VERIFIER(fantome::lireFichier(cf, lu));
    VERIFIER(lu.terrain == "ville" && lu.car == "m4" && lu.peinture == 0xb3122b && lu.score == 51234 && lu.e.size() == en.trace.e.size());
    bool pareil = true;
    for (size_t i = 0; i < lu.e.size(); i++) pareil = pareil && lu.e[i].d == en.trace.e[i].d && lu.e[i].x == en.trace.e[i].x && lu.e[i].h == en.trace.e[i].h && lu.e[i].v == en.trace.e[i].v;
    VERIFIER(pareil);
    fantome::Echantillon e;
    VERIFIER(fantome::interpoler(lu, 1.05, e) && std::abs(e.d - 0.5 * (lu.e[10].d + lu.e[11].d)) < 1e-6 && std::abs(e.v - 0.5 * (lu.e[10].v + lu.e[11].v)) < 1e-6);
    VERIFIER(fantome::interpoler(lu, 0, e) && proche(e.d, lu.e[0].d));
    VERIFIER(fantome::interpoler(lu, lu.duree(), e) && proche(e.d, lu.e.back().d));
    VERIFIER(!fantome::interpoler(lu, lu.duree() + 0.2, e));   // trace finie : plus de fantôme
    VERIFIER(!fantome::interpoler(fantome::Trace{}, 0, e));
    {   // cap : plus court chemin entre −π et π
        fantome::Trace c; c.e = {{0, 0, 3.1, 0}, {1, 0, -3.1, 0}};
        VERIFIER(fantome::interpoler(c, 0.05, e) && std::abs(std::abs(e.h) - 3.1 - (2 * 3.14159265358979 - 6.2) / 2) < 1e-6);
    }
    // fichier abîmé : refusé, sans plantage
    std::string b = fantome::encoder(en.trace);
    fantome::Trace x;
    VERIFIER(!fantome::decoder(b.substr(0, b.size() - 7), x));
    VERIFIER(!fantome::decoder("WFSF1", x) && !fantome::decoder("", x) && !fantome::decoder("XXXX1" + b.substr(5), x));
    b[12] ^= 0x55; fantome::decoder(b, x);
    VERIFIER(!fantome::lireFichier(dir + "absent-fantome.bin", x));
    std::remove(cf.c_str()); std::remove(chemin.c_str());

    if (g_echecs) { std::printf("test_records : %d échec(s)\n", g_echecs); return 1; }
    std::printf("test_records : OK (6 cas Poursuite, Course à 5, ancienne sauvegarde, trace 0,1 s, 6 000 échantillons, fichier, interpolation)\n");
    return 0;
}
