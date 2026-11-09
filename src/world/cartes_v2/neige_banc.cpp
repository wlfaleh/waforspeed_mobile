// WaForSpeed — WFS-346 : banc « neige » (WFS_BANC=neige) : saison lue du paquet (neige au sol, flocons, adhérence), sur les deux cartes
// d'essai (paquets/sources) cuites en paquet dans le dossier des paquets puis découvertes comme sur le téléphone.
// Captures (dossier de sortie) :
//   neige-jour-arriere, neige-nuit-arriere      : carte de Noël, vue Arrière : chaussée enneigée, traces de roues, flocons (jour / nuit)
//   neige-cockpit-jour, neige-cockpit-nuit       : vue Cockpit sous la neige : flocons seulement à travers les vitres
//   neige-tunnel-avant / -bouche / -dedans       : vue Cockpit à l'entrée de l'abri (tunnel) : aucun flocon dessous
//   neige-tunnel-arriere-dedans                  : témoin en vue Arrière, voiture sous l'abri
//   neige-automne-jour                           : autre saison par les données seules (sans neige, sol et couleurs du paquet)
//   neige-freinage-0..4, neige-virage-0..4       : suite d'images d'un freinage à fond (ABS) et d'un virage volant à fond sur la neige
// Mesures au journal : distance de freinage 100 → 0 (ou de la vitesse atteinte) sur la neige et sur la Ville (sec), accélération
// latérale du virage, coût des flocons (fin de course). Renvoie le nombre d'écarts.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include "carte_donnees.h"
#include "../../core/carte_cuite.h"
#include "../../core/log.h"
#include "../../game/camera_vue.h"
#include "../../game/course_system.h"
#include "../../game/game.h"
#include "../../render/renderer.h"

namespace wfs {
namespace cartes_v2 {

namespace {
struct Banc {
    Game& g; Renderer& r; std::string out; double t = 1000; int ecarts = 0;
    void verifier(bool ok, const char* quoi) { if (!ok) { ecarts++; WFS_ERR("banc neige (WFS-346) : ÉCART : %s", quoi); } else WFS_LOG("banc neige (WFS-346) : ok : %s", quoi); }
    Player& joueur() { return const_cast<Player&>(g.player()); }   // banc seulement : départ placé, vitesse lue
    // pilote : tient la file du milieu et la vitesse demandée (gaz, frein doux), volant libre ou imposé
    DriveInput piloter(double kmh, float volant = 2.f) {
        const Player& p = g.player();
        DriveInput in;
        in.steer = volant <= 1.f ? volant : (float)std::clamp(-p.playerX * 3.0 - p.heading * 4.0, -1.0, 1.0);
        in.gas = p.kmh() < kmh; in.brake = p.kmh() > kmh + 12;
        return in;
    }
    void pas(int n, double kmh, float volant = 2.f, int rendu = 6) {
        for (int i = 0; i < n; i++) { g.manette = piloter(kmh, volant); g.update(1.0 / 60); t += 1.0 / 60; if (rendu > 0 && i % rendu == 0) g.render(t); }
    }
    void capture(const std::string& nom) {
        g.render(t += 1.0 / 60); r.capturePng(out + "/" + nom + ".png");
        const Player& p = g.player();
        WFS_LOG("capture %s : %.0f km/h, %.1f m, écart %+.2f m, μ sol %.2f", nom.c_str(), p.kmh(), p.dist * M, p.playerX * RW, saisonPartie().muChaussee);
    }
    // course sur une carte, de jour ou de nuit, vue donnée, départ au mètre `depart` (voiture placée sur le tracé, puis décompte)
    bool partir(const char* carte, bool nuit, VueCamera v, double depart, const char* voiture = "golf") {
        setenv("WFS_MOMENT", nuit ? "nuit" : "jour", 1);
        courseBenchRules(true);
        if (!g.init(r, voiture, carte)) { verifier(false, "chargement de la carte"); return false; }
        choisirVue(v); g.camMode = 0; g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{};
        if (depart > 0) { Player& p = joueur(); p.dist = p.position = depart / M; }
        g.allerA(Ecran::Course);
        for (int i = 0; i < 60 * 4; i++) { g.manette = DriveInput{}; g.update(1.0 / 60); t += 1.0 / 60; if (i % 20 == 0) g.render(t); }   // décompte
        return true;
    }
    void fin() { g.manette = DriveInput{}; g.modeManette = false; g.shutdown(); }
    // freinage à fond depuis la vitesse atteinte (au plus 100 km/h) sur la ligne droite ; 5 images ; renvoie la distance (m)
    // élan sur la ligne droite du départ (au plus `elan` m) ; la vitesse atteinte est notée (sur la neige, l'antipatinage limite l'élan)
    double freiner(const char* nom, double& v0, double elan) {
        Player& p = joueur();
        for (int i = 0; i < 60 * 40 && p.kmh() < 100 && p.dist * M < elan; i++) pas(1, 105);
        v0 = p.kmh();
        const double d0 = p.dist * M;
        int pris = 0; double dernier = -1;
        for (int i = 0; i < 60 * 30 && p.kmh() > 0.5; i++) {
            DriveInput in; in.brake = true; in.steer = (float)std::clamp(-p.playerX * 3.0 - p.heading * 4.0, -1.0, 1.0);
            const double fait = (p.dist * M - d0);
            if (nom && pris < 5 && (dernier < 0 || fait - dernier >= std::max(8.0, v0 * v0 / 3000))) { capture(std::string(nom) + "-" + std::to_string(pris++)); dernier = fait; }
            g.manette = in; g.update(1.0 / 60); t += 1.0 / 60; if (i % 6 == 0) g.render(t);
            if (p.kmh() < 1 && in.brake) break;   // arrêt (le frein enchaînerait sur la marche arrière)
        }
        if (nom) while (pris < 5) capture(std::string(nom) + "-" + std::to_string(pris++));
        return p.dist * M - d0;
    }
};

void cuireEssais(const std::string& racine) {
    for (const char* id : {"noel-test", "automne-test"}) {
        carte::Paquet p; std::string err;
        if (!sourcesCarte(racine, id, carte::FormatTexture::RGBA8, p, err)) { WFS_ERR("banc neige : %s : %s", id, err.c_str()); continue; }
        const std::vector<uint8_t> o = carte::ecrire(p);
        std::ofstream(carte::dossierPaquets() + "/" + carte::nomFichier(id, carte::FormatTexture::RGBA8), std::ios::binary).write((const char*)o.data(), (std::streamsize)o.size());
    }
}
} // namespace

int benchNeige(Game& g, Renderer& r, const std::string& out, const std::string& racine) {
    std::error_code ec;
    std::filesystem::create_directories(carte::dossierPaquets(), ec);
    cuireEssais(racine);
    setenv("WFS_CARTES_TEST", "1", 1);
    Banc b{g, r, out};
    b.verifier(decouvrir() >= 2 && donnees("noel-test") && donnees("automne-test"), "les deux cartes d'essai lues de leur paquet");
    const auto noel = donnees("noel-test");
    if (!noel) { unsetenv("WFS_CARTES_TEST"); return b.ecarts + 1; }
    double tunnel0 = -1, tunnel1 = -1;
    for (const trace::Section& s : noel->sections) if (s.type == trace::Type::Tunnel) { tunnel0 = s.debut * SEG_M; tunnel1 = s.fin * SEG_M; break; }

    // 1. vue Arrière, jour puis nuit : place du marché (chalets, bancs), à 50 km/h
    for (const bool nuit : {false, true}) {
        if (!b.partir("noel-test", nuit, VueCamera::Poursuite, 30)) continue;
        b.verifier(saisonPartie().neigeChaussee > 0.9f && saisonPartie().flocons > 0 && saisonPartie().muChaussee < 0.3f, "saison de la carte de Noël lue du paquet");
        b.pas(60 * 9, 50);
        b.capture(nuit ? "neige-nuit-arriere" : "neige-jour-arriere");
        b.fin();
    }
    // 2. vue Cockpit sous la neige, jour et nuit : flocons à travers le pare-brise et les vitres seulement
    for (const bool nuit : {false, true}) {
        if (!b.partir("noel-test", nuit, VueCamera::Cockpit, 30)) continue;
        b.pas(60 * 9, 50);
        b.capture(nuit ? "neige-cockpit-nuit" : "neige-cockpit-jour");
        b.fin();
    }
    // 3. abri (tunnel) en vue Cockpit : 15 m avant la bouche, à la bouche, au milieu ; témoin vue Arrière au milieu
    if (tunnel0 > 0) {
        if (b.partir("noel-test", false, VueCamera::Cockpit, tunnel0 - 60)) {
            const std::pair<double, const char*> etapes[] = {{tunnel0 - 15, "neige-tunnel-avant"}, {tunnel0 + 1, "neige-tunnel-bouche"}, {(tunnel0 + tunnel1) / 2, "neige-tunnel-dedans"}};
            size_t e = 0;
            for (int k = 0; k < 60 * 60 && e < 3; k++) { b.pas(1, 40, 2.f, 3); if (g.player().dist * M >= etapes[e].first) b.capture(etapes[e++].second); }
            b.verifier(e == 3, "entrée de l'abri en vue Cockpit (3 images)");
            b.fin();
        }
        if (b.partir("noel-test", false, VueCamera::Poursuite, tunnel0 - 40)) {
            for (int k = 0; k < 60 * 60 && g.player().dist * M < (tunnel0 + tunnel1) / 2; k++) b.pas(1, 40, 2.f, 3);
            b.capture("neige-tunnel-arriere-dedans");
            b.fin();
        }
    }
    // 4. autre saison par les données seules
    if (b.partir("automne-test", false, VueCamera::Poursuite, 30)) {
        b.verifier(saisonPartie().neigeChaussee == 0 && saisonPartie().flocons == 0, "automne : ni neige au sol ni flocons");
        b.pas(60 * 9, 50);
        b.capture("neige-automne-jour");
        b.fin();
    }
    // 5. conduite sur la neige : freinage à fond (ABS) puis virage volant à fond ; comparaison avec la Ville (sec), même voiture
    double vNeige = 0, vSec = 0, dNeige = 0, dSec = 0;
    if (b.partir("noel-test", false, VueCamera::Poursuite, 0)) { dNeige = b.freiner("neige-freinage", vNeige, 125); b.fin(); }
    if (b.partir("ville", false, VueCamera::Poursuite, 0)) { dSec = b.freiner(nullptr, vSec, 400); b.fin(); }
    // distance ramenée à 100 km/h (d ∝ v²) pour comparer
    const double d100Neige = vNeige > 20 ? dNeige * (100 / vNeige) * (100 / vNeige) : 0, d100Sec = vSec > 20 ? dSec * (100 / vSec) * (100 / vSec) : 0;
    WFS_LOG("banc neige (WFS-346) : freinage à fond (ABS) : neige %.0f → 0 km/h en %.1f m (≈ %.0f m depuis 100) ; Ville sèche %.0f → 0 en %.1f m (≈ %.0f m depuis 100) ; rapport %.2f",
            vNeige, dNeige, d100Neige, vSec, dSec, d100Sec, d100Sec > 0 ? d100Neige / d100Sec : 0);
    b.verifier(d100Neige > 2.5 * d100Sec && d100Sec > 25, "sur la neige, la voiture freine 3 à 4 fois plus long qu'au sec");
    if (b.partir("noel-test", false, VueCamera::Poursuite, 30)) {
        Player& p = b.joueur();
        for (int i = 0; i < 60 * 30 && p.kmh() < 55; i++) b.pas(1, 60);
        double cap0 = p.monde.cap, latMax = 0;
        for (int k = 0; k < 5; k++) {
            b.capture("neige-virage-" + std::to_string(k));
            for (int i = 0; i < 24; i++) { b.pas(1, 60, 1.f, 6); latMax = std::max(latMax, std::abs(p.monde.u() * p.monde.r)); }
        }
        WFS_LOG("banc neige (WFS-346) : virage volant à fond vers 55 km/h : accélération latérale maxi %.2f g (μ sol %.2f), cap gagné %.2f rad en 2 s",
                latMax / 9.81, saisonPartie().muChaussee, std::abs(p.monde.cap - cap0));
        b.verifier(latMax / 9.81 < saisonPartie().muChaussee * 1.3 + 0.05, "virage sur la neige : accélération latérale limitée par μ (la voiture glisse)");
        b.fin();
    }
    // retour sur une carte du code : sec, sans neige
    if (b.partir("ville", false, VueCamera::Poursuite, 0)) {
        b.verifier(saisonPartie().muChaussee == 1.0f && saisonPartie().neigeChaussee == 0 && saisonPartie().flocons == 0, "retour sur la Ville : sec, sans neige");
        b.fin();
    }
    choisirVue(VueCamera::Poursuite);
    unsetenv("WFS_MOMENT"); unsetenv("WFS_CARTES_TEST");
    for (const char* id : {"noel-test", "automne-test"}) std::filesystem::remove(carte::dossierPaquets() + "/" + carte::nomFichier(id, carte::FormatTexture::RGBA8), ec);
    WFS_LOG("banc neige (WFS-346) : %d écart(s)%s", b.ecarts, b.ecarts ? " ÉCHEC" : " OK");
    return b.ecarts;
}

} // namespace cartes_v2
} // namespace wfs
