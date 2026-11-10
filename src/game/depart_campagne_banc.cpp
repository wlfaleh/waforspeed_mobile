// WaForSpeed — banc « depart-campagne » (WFS-374) : départ de la Campagne avec le doigt sur GAZ, comme sur le téléphone.
// Demande du PO (WFS-374, suivi du 2026-11-09 21:35) : vidéos de l'APK v0.1.969 où la XR5 semblait bloquée au départ de la
// Campagne. Le banc refait le parcours du joueur pour chaque moment de JOUR (et la nuit) : retour au menu, Campagne choisie,
// décompte, doigt posé sur GAZ pendant le décompte et tenu après le GO (vrais appuis tactiles, pas le pilote automatique).
// Écart si, 3 s après le GO, la voiture roule à moins de 25 km/h, si le bouton GAZ n'est pas tenu, ou si un appui sur la case
// SCORE (haut à droite) lance le nitro (ANO-233). Une capture par moment : depart-campagne-<moment>.png.
#include <cmath>
#include <string>
#include "../core/log.h"
#include "../render/atmosphere_physique.h"
#include "../render/eclairage_physique.h"
#include "commandes_tactiles.h"
#include "course_system.h"
#include "game.h"

namespace wfs {

int benchDepartCampagne(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    auto verifier = [&](bool ok, const char* quoi, const char* moment) {
        if (!ok) ecarts++;
        WFS_LOG("depart-campagne (WFS-374) : %s : %s : %s", moment, quoi, ok ? "OK" : "ÉCART");
    };
    const float W = (float)r.width(), H = (float)r.height();
    double t = 0;
    auto image = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 9) g.render(t); } };
    using atmosphere::Moment;
    const Moment moments[] = {Moment::Reel, Moment::Aube, Moment::Matin, Moment::Midi, Moment::Doree, Moment::Nuit};
    const bool regles = courseBenchRulesOn();
    courseBenchRules(true);   // règles de course du jeu (décompte, chrono) : le banc PC les coupe par défaut (desktop_main.cpp)
    if (!g.init(r, "rrsport", "campagne")) { courseBenchRules(regles); WFS_ERR("depart-campagne (WFS-374) : Campagne impossible à charger"); return 1; }
    for (const Moment m : moments) {
        const char* nom = atmosphere::nomMoment(m);
        // parcours du joueur : accueil, choix du terrain (Campagne, moment), « Rouler » → course neuve avec décompte
        g.allerA(Ecran::Menu); image(2);
        eclairage::choisirMoment((int)m);
        Game::demandes().carte = "campagne"; Game::demandes().nuit = m == Moment::Nuit;
        image(1);
        g.allerA(Ecran::Course); image(1);
        const DispositionCommandes dc = dispositionCommandes(W, H, g.marges());
        const float gx = dc.gaz.cx(), gy = dc.gaz.cy();
        g.onTouch({0, gx, gy, true});   // doigt posé sur GAZ pendant le décompte (vidéo de l'utilisateur), tenu ensuite
        bool decompte = false;   // course neuve : décompte (après un rechargement de la carte, il commence une image plus tard)
        for (int i = 0; i < 30 && !decompte; i++) { decompte = activeCourse() && activeCourse()->state == RaceState::Count; if (!decompte) image(1); }
        verifier(decompte, "décompte au départ", nom);
        int n = 0;
        for (; n < 60 * 8 && activeCourse() && !activeCourse()->playing(); n++) { g.onTouch({0, gx, gy, true}); image(1); }
        verifier(activeCourse() && activeCourse()->playing(), "GO donné", nom);
        const double d0 = g.player().dist;
        for (int i = 0; i < 60 * 3; i++) { g.onTouch({0, gx, gy, true}); image(1); }   // 3 s après le GO, gaz tenu
        const double kmh = g.player().kmh(), metres = (g.player().dist - d0) * M;
        WFS_LOG("depart-campagne (WFS-374) : %s : 3 s après le GO, %.1f km/h, %.1f m parcourus", nom, kmh, metres);
        verifier(kmh >= 25 && metres >= 8, "la voiture roule (≥ 25 km/h, ≥ 8 m en 3 s)", nom);
        // ANO-233 : un 2e doigt sur la case SCORE (haut à droite) ne lance pas le nitro
        const bool nitroAvant = g.player().nitroOn;
        g.onTouch({1, W * 0.90f, H * 0.08f, true}); image(20);
        verifier(!nitroAvant && !g.player().nitroOn, "appui sur la case SCORE : pas de nitro (ANO-233)", nom);
        g.render(t); g.render(t + 0.01);   // capture doigt encore sur SCORE : avant WFS-374, anneau du nitro allumé et lignes de vitesse
        r.capturePng(out + "/depart-campagne-" + nom + ".png");
        g.onTouch({1, W * 0.90f, H * 0.08f, false}); image(1);
        g.onTouch({0, gx, gy, false}); image(1);
    }
    g.shutdown();
    courseBenchRules(regles);
    WFS_LOG("depart-campagne (WFS-374) : %d écart(s)%s", ecarts, ecarts ? " ÉCHEC" : " OK");
    return ecarts;
}

} // namespace wfs
