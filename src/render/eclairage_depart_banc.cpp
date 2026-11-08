// WaForSpeed — WFS-328 (épic E13, ANO-177) : banc du départ de course (WFS_BANC=depart).
// Reproduit le parcours du joueur : menu et garage (jour), écran Terrain, « Rouler » sur une carte de nuit (rechargement),
// décompte puis course. Chaque image est rendue (pas de 1/60 s, une image sur deux comme un téléphone à 30 i/s) et
// l'exposition est écrite au journal ; captures depart-<moment>-<carte>-<instant> au premier décompte (« d3 »), à 0:00,
// 0:01, 0:02 et 0:04 du chrono. Puis « Recommencer » de nuit (Pause → Menu → Course) et une reprise après pause.
// Contrôle chiffré (RG-02) : (1) à chaque capture, l'EV appliqué n'est pas plus de 1 IL sous l'EV visé (image au plus deux fois
// trop claire : aucun flash ; plus sombre que visé, c'est l'adaptation douce vers le sombre, RG-03) ; (2) l'EV de la première
// image est à moins de 1 IL de l'EV mesuré sur la première sonde locale de la scène (la mesure juste du lieu).
#include <cmath>
#include <cstdlib>
#include <string>
#include "../core/log.h"
#include "../game/course.h"
#include "../game/course_system.h"
#include "../game/camera_vue.h"
#include "../game/game.h"
#include "../ui/ecrans/reglages.h"
#include "../ui/text.h"
#include "atmosphere_physique.h"
#include "eclairage_physique.h"
#include "renderer.h"

namespace wfs {

namespace {
constexpr double ECART_MAX_IL = 1.0;   // RG-02 : écart toléré entre l'exposition appliquée et celle du lieu (1 IL)

struct Depart {
    Game& g; Renderer& r; std::string out; double t = 2000; int erreurs = 0;
    // une image : simulation 1/60 s, rendu une fois sur deux
    void image(int i) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 2 == 0) g.render(t); }
    void journal(const char* nom) {
        const eclairage::Etat& e = eclairage::etat();
        const Course* k = activeCourse();
        WFS_LOG("depart %s : chrono %+.2f s, EV %.2f (visé %.2f), eRecu %.2f lx, L mesurée %.3f cd/m², sonde %d, écran %s", nom,
                k ? k->clock : 0.0, e.ev, e.evCible, e.eRecu, e.luminanceMesuree, (int)e.sondeCaptureActive, nomEcran(g.ecran()));
    }
    void capture(const std::string& nom) {
        r.capturePng(out + "/" + nom + ".png");
        const eclairage::Etat& e = eclairage::etat();
        const double ecart = e.evCible - e.ev;   // > 0 : image plus claire que visé
        const bool ok = !e.actif || ecart <= ECART_MAX_IL;
        if (!ok) erreurs++;
        WFS_LOG("capture %s : EV %.2f, visé %.2f, surexposition %.2f IL (≤ %.1f) %s", nom.c_str(), e.ev, e.evCible, ecart, ECART_MAX_IL, ok ? "OK" : "ÉCART");
    }
    // course de la première image jusqu'à 0:04 du chrono, captures aux instants demandés (RG-04)
    void course(const std::string& base) {
        const double instants[] = {0, 1, 2, 4};
        int prochain = 0, i = 0; bool premiere = true, sonde = false; double ev0 = 0;
        for (; i < 60 * 9 && (prochain < 4 || !sonde); i++) {
            image(i);
            if (i % 2) continue;
            const Course* k = activeCourse();
            const double c = k ? k->clock : 0;
            journal(base.c_str());
            const eclairage::Etat& e = eclairage::etat();
            if (premiere) { capture(base + "-d3"); premiere = false; ev0 = e.ev; }
            if (!sonde && e.sondeCaptureActive) {   // première sonde locale : la mesure juste du lieu
                sonde = true;
                const bool ok = std::fabs(ev0 - e.evCible) <= ECART_MAX_IL;
                if (!ok) erreurs++;
                WFS_LOG("%s : 1re image EV %.2f, 1re sonde locale (chrono %+.2f s) EV visé %.2f, écart %.2f IL (≤ %.1f) %s", base.c_str(), ev0,
                        c, e.evCible, std::fabs(ev0 - e.evCible), ECART_MAX_IL, ok ? "OK" : "ÉCART");
            }
            if (prochain < 4 && c >= instants[prochain] - 1e-6) { capture(base + "-" + std::to_string((int)instants[prochain])); prochain++; }
        }
    }
    // parcours du joueur : menu et garage de jour, écran Terrain, « Rouler » sur la carte et le moment choisis
    bool rouler(const char* carte, bool nuit) {
        unsetenv("WFS_MOMENT");
        if (!g.init(r, "golf", "ville")) { WFS_ERR("depart : chargement impossible"); erreurs++; return false; }
        g.allerA(Ecran::Menu); for (int i = 0; i < 40; i++) image(i);
        g.allerA(Ecran::Garage); for (int i = 0; i < 40; i++) image(i);
        g.allerA(Ecran::Terrain); for (int i = 0; i < 20; i++) image(i);
        Game::demandes().carte = carte; Game::demandes().nuit = nuit;
        eclairage::choisirMoment((int)(nuit ? atmosphere::Moment::Nuit : atmosphere::Moment::Midi));
        g.update(1.0 / 60);   // changerCarte (rechargement), puis la course
        // décompte depuis le début : la course repart en passant à Course (comme après « Rouler » dans le jeu)
        if (g.ecran() == Ecran::Course) { g.allerA(Ecran::Menu); g.update(1.0 / 60); }
        g.allerA(Ecran::Course);
        g.setAutopilot(true); g.camMode = 0;
        return true;
    }
};
} // namespace

// WFS-328 (RG-05, ANO-179) : 4 min de course de nuit en Ville (Checkpoint, vue Arrière, Graphismes Basse comme le téléphone de la
// recette) : une image tous les 1/4 s, EV au journal ; captures autour de 4:00 ; compte les images surexposées de plus de 1 IL par
// rapport à la rue éclairée (EV < 2 en Ville : le paysage n'y est jamais exposé comme la rase campagne)
static int courseLongue(Depart& d, const std::string& base, double duree) {
    int i = 0, n = 0, claires = 0; double evMin = 99, tMin = 0;
    const double t0 = d.t;
    for (; d.t - t0 < duree; i++) {
        d.g.update(1.0 / 60); d.t += 1.0 / 60;
        if (i % 15) continue;
        d.g.render(d.t); n++;
        const eclairage::Etat& e = eclairage::etat();
        if (e.ev < evMin) { evMin = e.ev; tMin = d.t - t0; }
        if (e.ev < 2.0) claires++;
        if (i % 240 == 0) d.journal(base.c_str());
        const double ecoule = d.t - t0;
        for (double c : {duree - 20.0, duree - 10.0, duree - 0.02})
            if (ecoule >= c && ecoule - 0.25 < c) { d.journal(base.c_str()); d.capture(base + "-" + std::to_string((int)std::lround(c))); }
    }
    WFS_LOG("%s : %d images, EV mini %.2f à %.1f s, %d image(s) sous EV 2 (surexposées) %s", base.c_str(), n, evMin, tMin, claires, claires ? "ÉCART" : "OK");
    return claires ? 1 : 0;
}

int benchDepart(Game& g, Renderer& r, const std::string& out) {
    ui::chargerPolice(r);
    ecrans::reglages().qualite = ecrans::QUALITE_HAUTE;
    Depart d{g, r, out};
    // la sonde locale arrive ≈ 3 s après le chargement sur le téléphone (APK 918 : image juste vers 0:03), en 0,3 s sur le banc :
    // le banc la retarde de 3 s pour reproduire le téléphone
    eclairage::Forcage f; f.actif = true; f.retardCapture = 3.0; eclairage::forcer(f);
    const char* seul = std::getenv("WFS_DEPART_SEUL");   // essai local : departs, recommencer, longue ou cockpit seulement
    const std::string quoi = seul ? seul : "";
    const bool tout = quoi.empty();
    if (tout || quoi == "longue") {   // RG-05 : 4 min de course de nuit, Basse, vue Arrière (Poursuite)
        ecrans::reglages().qualite = ecrans::QUALITE_BASSE; ecrans::appliquerQualiteNiveau(r);
        if (d.rouler("ville", true)) d.erreurs += courseLongue(d, "longue-nuit-ville-basse", 245);
        ecrans::reglages().qualite = ecrans::QUALITE_HAUTE; ecrans::appliquerQualiteNiveau(r);
    }
    if (tout || quoi == "cockpit") {   // RG-05 : départ de nuit en vue Cockpit (Ville)
        const VueCamera v = vueCamera(); choisirVue(VueCamera::Cockpit);
        if (d.rouler("ville", true)) d.course("depart-cockpit-nuit-ville");
        choisirVue(v);
    }
    if (tout || quoi == "departs") for (const char* carte : {"ville", "plage", "campagne"})
        if (d.rouler(carte, true)) d.course(std::string("depart-nuit-") + carte);
    if ((tout || quoi == "departs") && d.rouler("ville", false)) d.course("depart-jour-ville");
    if ((tout || quoi == "recommencer") && d.rouler("ville", true)) {   // pause, reprise, puis « Recommencer » (Pause → Menu → Course, le menu n'est pas dessiné)
        for (int i = 0; i < 60 * 6; i++) d.image(i);
        g.allerA(Ecran::Pause); for (int i = 0; i < 60; i++) d.image(i);
        g.allerA(Ecran::Course); g.update(1.0 / 60); g.render(d.t); d.journal("reprise"); d.capture("depart-reprise-nuit-ville");
        for (int i = 0; i < 60; i++) d.image(i);
        g.allerA(Ecran::Pause); for (int i = 0; i < 30; i++) d.image(i);
        g.allerA(Ecran::Menu); g.update(1.0 / 60); g.allerA(Ecran::Course);
        d.course("depart-recommencer-nuit-ville");
    }
    g.setAutopilot(false); unsetenv("WFS_MOMENT"); eclairage::choisirMoment(-1); eclairage::forcer({});
    WFS_LOG("depart : %d écart(s) d'exposition%s", d.erreurs, d.erreurs ? " ÉCHEC" : " OK");
    return d.erreurs;
}

} // namespace wfs
