// WaForSpeed — WFS-391 : banc « sol » (WFS_BANC=sol) : conduite au bas-côté et sur la neige (ANO-229, ANO-231), ancienne conduite
// (celle du jeu, g_conduiteMonde faux). Chaque scène est jouée « avant » (g_avantWfs391 : comportement de main rejoué) puis
// « après » (jeu corrigé), dans la partie, de jour, caméra Poursuite. Captures (5 images chacune) :
//   sol-neige-freinage-avant-NN / -apres-NN : carte de Noël (paquet d'essai), frein à fond depuis 100 km/h (ABS) ;
//   sol-ville-freinage-avant-NN / -apres-NN : Ville au sec, même freinage (pas de régression, RG-03) ;
//   sol-neige-bascote-avant-NN / -apres-NN  : carte de Noël, voiture sur le bas-côté enneigé à 50 km/h, volant à fond vers
//                                             l'extérieur (la voiture tourne moins qu'avant : le bas-côté accroche moins) ;
//   sol-col-bascote-9m-NN, sol-col-glissiere-NN : Campagne, épingle à gauche du col, 10 km/h volant à fond vers l'intérieur,
//                                             depuis le bas-côté intérieur à 9 m (rayon du centre) et depuis 11,5 m (derrière la
//                                             glissière à 10,35 m : la voiture y est ramenée et la longe ; cause de l'ANO-231).
// Chiffres (distances de freinage, lacet, rayons aux 5 positions) : sol.txt dans le dossier de sortie. Renvoie le nombre d'écarts.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include "../core/carte_cuite.h"
#include "../core/log.h"
#include "../render/renderer.h"
#include "../ui/text.h"
#include "../world/cartes_v2/carte_donnees.h"
#include "camera_vue.h"
#include "conduite_banc.h"
#include "course_system.h"
#include "game.h"
#include "virages_banc.h"

namespace wfs {
namespace {
using namespace banc_virages;

struct BancSol {
    Game& g; Renderer& r; std::string out; double t = 1000; int ecarts = 0; std::string rapport;
    void verifier(bool ok, const char* quoi) {
        if (!ok) { ecarts++; WFS_ERR("banc sol (WFS-391) : ÉCART : %s", quoi); } else WFS_LOG("banc sol (WFS-391) : ok : %s", quoi);
        rapport += std::string(ok ? "- ok : " : "- ÉCART : ") + quoi + "\n";
    }
    Player& joueur() { return const_cast<Player&>(g.player()); }   // banc seulement : voiture placée, vitesse imposée au départ
    bool partir(const char* carte) {
        setenv("WFS_MOMENT", "jour", 1);
        reglagesConduite().aideDirection = false;
        courseBenchRules(true);
        if (!g.init(r, "golf", carte)) { verifier(false, "chargement de la carte"); return false; }
        choisirVue(VueCamera::Poursuite); g.camMode = 0; g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{};
        g.allerA(Ecran::Course);
        for (int i = 0; i < 60 * 4; i++) { g.manette = DriveInput{}; g.update(1.0 / 60); t += 1.0 / 60; if (i % 20 == 0) g.render(t); }   // décompte
        // le décompte attend les commandes (WFS-242) et retient la voiture : gaz jusqu'au départ réel, la voiture roule
        for (int i = 0; i < 60 * 10 && g.player().kmh() < 8; i++) { DriveInput in; in.gas = true; pas(in); if (i % 20 == 0) g.render(t); }
        return true;
    }
    void placer(double s, double n, double kmh) {   // banc seulement : pose et vitesse de la voiture
        Player& p = joueur();
        p.dist = s; p.position = s;   // s pris dans le 1er tour (0 ≤ s < longueur) : déjà ramené dans la piste
        p.playerX = n / RW; p.heading = 0; p.speed = kmh / 3.6 / M; p.steer = 0;
        WFS_LOG("banc sol : voiture placée à %.1f m, écart %+.2f m, %.0f km/h", p.dist * M, p.playerX * RW, p.kmh());
    }
    void capture(const std::string& nom, int i) {
        g.render(t += 1.0 / 60);
        char n[64]; std::snprintf(n, sizeof n, "-%02d.png", i);
        r.capturePng(out + "/" + nom + n);
        const Player& p = g.player();
        WFS_LOG("capture %s%s : %.0f km/h, %.1f m, écart %+.2f m, cap %+.2f rad", nom.c_str(), n, p.kmh(), p.dist * M, p.playerX * RW, p.heading);
    }
    void pas(const DriveInput& in) { g.manette = in; g.update(1.0 / 60); t += 1.0 / 60; }
    void fin() { g.manette = DriveInput{}; g.modeManette = false; g.shutdown(); }

    // frein à fond depuis 100 km/h, volant qui garde la file du milieu ; 5 images réparties sur la distance attendue
    double freiner(const char* carte, const std::string& nom) {
        if (!partir(carte)) return 0;
        placer(20 / M, 0, 100);
        Player& p = joueur();
        const double d0 = p.dist * M, attendu = 27.78 * 27.78 / (2 * cartes_v2::saisonPartie().muChaussee * 9.81);
        int pris = 0;
        for (int i = 0; i < 60 * 30 && p.kmh() > 0.5; i++) {
            if (pris < 5 && p.dist * M - d0 >= attendu * pris / 5) capture(nom, pris++);
            DriveInput in; in.brake = true; in.steer = (float)std::clamp(-p.playerX * 3.0 - p.heading * 4.0, -1.0, 1.0);
            pas(in);
            if (i % 6 == 0) g.render(t);
            if (p.kmh() < 1) break;   // arrêt (le frein enchaînerait sur la marche arrière)
        }
        while (pris < 5) capture(nom, pris++);
        const double d = p.dist * M - d0;
        fin();
        return d;
    }
    // bas-côté de la carte de Noël à 8,6 m de l'axe, 50 km/h, volant à fond vers l'extérieur 1,2 s (images) : cap gagné en 0,6 s (rad)
    double basCote(const std::string& nom) {
        if (!partir("noel-test")) return 0;
        placer(20 / M, 8.6, 50);
        Player& p = joueur(); p.steer = 1;
        const double h0 = p.heading;
        double cap = 0;
        int pris = 0;
        for (int i = 0; i < 72; i++) {
            if (i % 15 == 0 && pris < 5) capture(nom, pris++);
            DriveInput in; in.steer = 1; in.gas = p.kmh() < 50;
            pas(in);
            if (i % 6 == 0) g.render(t);
            if (i == 35) cap = p.heading - h0;   // cap gagné en 0,6 s, avant d'atteindre la glissière (12 m)
        }
        while (pris < 5) capture(nom, pris++);
        fin();
        return cap;
    }
    // épingle à gauche du col : 10 km/h, volant à fond vers l'intérieur, depuis n (m, < 0 : à gauche) ; 5 images sur 2 s
    void col(const Epingle& e, double n, const std::string& nom) {
        if (!partir("campagne")) return;
        placer((e.debut + e.fin) / 2, n, 10);
        Player& p = joueur(); p.steer = (float)e.sens;
        int pris = 0;
        for (int i = 0; i < 120; i++) {
            if (i % 24 == 0 && pris < 5) capture(nom, pris++);
            DriveInput in; in.steer = (float)e.sens; in.gas = p.kmh() < 10;
            pas(in);
            if (i % 6 == 0) g.render(t);
        }
        while (pris < 5) capture(nom, pris++);
        fin();
    }
};

// paquets des cartes d'essai (carte de Noël) cuits dans le dossier des paquets, comme le banc « neige » (WFS-346)
void cuireNoel(const std::string& racine) {
    std::error_code ec;
    std::filesystem::create_directories(carte::dossierPaquets(), ec);
    carte::Paquet p; std::string err;
    if (!cartes_v2::sourcesCarte(racine, "noel-test", carte::FormatTexture::RGBA8, p, err)) { WFS_ERR("banc sol : noel-test : %s", err.c_str()); return; }
    const std::vector<uint8_t> o = carte::ecrire(p);
    std::ofstream(carte::dossierPaquets() + "/" + carte::nomFichier("noel-test", carte::FormatTexture::RGBA8), std::ios::binary).write((const char*)o.data(), (std::streamsize)o.size());
}
} // namespace

int benchSol(Game& g, Renderer& r, const std::string& out, const std::string& racine) {
    ui::chargerPolice(r);
    const bool aideAvant = reglagesConduite().aideDirection, reglesAvant = courseBenchRulesOn();
    cuireNoel(racine);
    setenv("WFS_CARTES_TEST", "1", 1);
    cartes_v2::decouvrir();
    BancSol b{g, r, out};
    b.rapport = "# WFS-391 : conduite au bas-côté et sur la neige (ancienne conduite, Golf)\n";
    char l[256];

    // 1 et 3. freinage à fond depuis 100 km/h : neige (RG-01) et Ville au sec (RG-03)
    double dN[2] = {0, 0}, dS[2] = {0, 0};
    for (int apres = 0; apres < 2; apres++) {
        g_avantWfs391 = !apres;
        const std::string q = apres ? "apres" : "avant";
        dN[apres] = b.freiner("noel-test", "sol-neige-freinage-" + q);
        dS[apres] = b.freiner("ville", "sol-ville-freinage-" + q);
    }
    g_avantWfs391 = false;
    std::snprintf(l, sizeof l, "## Freinage 100 → 0 km/h (frein à fond, ABS)\n- neige (μ 0,25) : avant %.1f m, après %.1f m\n- Ville au sec : avant %.1f m, après %.1f m\n",
                  dN[0], dN[1], dS[0], dS[1]);
    b.rapport += l;
    b.verifier(dN[1] >= 120 && dN[1] <= 160 && dN[1] < dN[0], "RG-01 : neige, 120 à 160 m (arbitrage du PO), plus court qu'avant (la traînée ne dépend plus de μ)");
    b.verifier(dS[1] >= 36.6 * 0.97 && dS[1] <= 36.6 * 1.03 && std::abs(dS[1] - dS[0]) < 0.05, "RG-03 : Ville au sec, 36,6 m ± 3 %, inchangé");

    // 2. bas-côté enneigé, volant à fond vers l'extérieur (RG-02)
    double cap[2] = {0, 0};
    for (int apres = 0; apres < 2; apres++) { g_avantWfs391 = !apres; cap[apres] = b.basCote(std::string("sol-neige-bascote-") + (apres ? "apres" : "avant")); }
    g_avantWfs391 = false;
    std::snprintf(l, sizeof l, "## Bas-côté enneigé (8,6 m), 50 km/h, volant à fond\n- cap gagné en 0,6 s : avant %.3f rad, après %.3f rad\n", cap[0], cap[1]);
    b.rapport += l;
    b.verifier(cap[1] < cap[0], "RG-02 : sur le bas-côté enneigé, la voiture tourne moins qu'avant (accroche ≤ chaussée)");

    // 5. épingle du col de la Campagne (RG-06) : rayons aux 5 positions (mêmes calculs que tests/test_virages_fleches.cpp), puis images
    Track& piste = banc_conduite::piste("campagne");
    const std::vector<Epingle> ep = epingles(piste);
    const Epingle* gauche = nullptr;
    for (const Epingle& e : ep) if (e.sens < 0) { gauche = &e; break; }
    if (gauche) {
        const double s = (gauche->debut + gauche->fin) / 2, edge = piste.map()->edge, centre = rayon("golf", piste, s, 0, 10, -1.f);
        std::snprintf(l, sizeof l, "## Épingle à gauche du col de la Campagne (%.0f m), 10 km/h, volant à fond vers l'intérieur (glissière à %.2f m)\n- centre : %.2f m\n",
                      s * M, edge * RW, centre);
        b.rapport += l;
        bool justes = true;
        for (double n : {7.5, -7.5, 9.0, -9.0}) {   // virage à gauche : intérieur à gauche (n < 0)
            const double rr = rayon("golf", piste, s, n, 10, -1.f);
            std::snprintf(l, sizeof l, "- %s à %.1f m : %.2f m (%+.1f %%)\n", std::abs(n) < 8 ? "bord" : "bas-côté", n, rr, 100 * (rr / centre - 1));
            b.rapport += l;
            justes = justes && std::abs(rr / centre - 1) <= 0.03;
        }
        const double r115 = rayon("golf", piste, s, -11.5, 10, -1.f), rG = 1 / std::abs(courbure(piste, s)) - edge * RW;
        std::snprintf(l, sizeof l, "- posée derrière la glissière (−11,5 m) : rayon %.2f m = celui de la glissière intérieure (%.2f m)\n", r115, rG);
        b.rapport += l;
        b.verifier(justes, "RG-06 : rayon aux bords et aux bas-côtés à ± 3 % du centre");
        b.verifier(std::abs(r115 / rG - 1) <= 0.03, "RG-06 : derrière la glissière, la voiture y est ramenée et la longe (cause de l'ANO-231)");
        b.col(*gauche, -9.0, "sol-col-bascote-9m");
        b.col(*gauche, -11.5, "sol-col-glissiere");
    } else b.verifier(false, "épingle à gauche du col introuvable");

    std::ofstream(out + "/sol.txt") << b.rapport;
    WFS_LOG("%s", b.rapport.c_str());
    std::filesystem::path f = std::filesystem::path(carte::dossierPaquets()) / carte::nomFichier("noel-test", carte::FormatTexture::RGBA8);
    std::error_code ec; std::filesystem::remove(f, ec);
    unsetenv("WFS_CARTES_TEST"); unsetenv("WFS_MOMENT");
    reglagesConduite().aideDirection = aideAvant; courseBenchRules(reglesAvant);
    WFS_LOG("banc sol (WFS-391) : %d écart(s)%s", b.ecarts, b.ecarts ? " ÉCHEC" : " OK");
    return b.ecarts;
}

} // namespace wfs
