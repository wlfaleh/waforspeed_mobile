// WaForSpeed — banc « textes » des menus (WFS-505, épic E20) : voir textes_banc.h.
// Parcours de l'utilisateur, au doigt, dans la langue de WFS_LANGUE : accueil → « Course solo » → choix du mode (Course à 5 :
// distance et difficulté) → « Continuer » → garage « Choisir » → choix du terrain → « Rouler » → course → bouton « Menu » →
// pause → « Quitter la course » → accueil ; puis « Duel en ligne » → salon → « Créer un duel » (salon ouvert) → « Quitter le
// salon » → « Retour » → accueil ; puis « Crédits » ; enfin l'écran de chargement (étapes, puis échec et « Réessayer »).
// Chaque écran note ses textes (textes::Journal) ; le garde-fou de WFS-504 les vérifie à la fin. Rend le nombre d'échecs.
#include "textes_banc.h"
#include <cmath>
#include "chargement.h"
#include "menu.h"
#include "pause.h"
#include "salon.h"
#include "terrain.h"
#include "../credits.h"
#include "../garage.h"
#include "../../core/log.h"
#include "../../core/platform.h"
#include "../../game/course5.h"
#include "../../game/course_system.h"
#include "../../game/duel.h"
#include "../../game/duel_system.h"
#include "../../game/game.h"
#include "../../render/renderer.h"

namespace wfs::ecrans::textes {

std::string& ecranNote() { static std::string e; return e; }

int benchMenusTextes(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("textes (WFS-505) : chargement impossible"); courseBenchRules(false); return 1; }
    g.setAutopilot(false); g.camMode = 0;
    choixModeBanc(true);   // panneau des modes ouvert par « Course solo » même sans sauvegarde (banc)
    const std::string lg = ui::langue::code();
    int echecs = 0;
    double t = 1500;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    auto attendre = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 9) g.render(t); } };
    auto etape = [&](bool ok, const char* quoi) {
        if (!ok) echecs++;
        WFS_LOG("textes (WFS-505) : parcours %s : %s : %s", lg.c_str(), quoi, ok ? "OK" : "ÉCHEC");
    };
    // textes de l'écran notés sur deux images, puis capture textes-<langue>-<nom>
    auto capture = [&](const std::string& nom, const char* quoi) {
        ecranNote() = nom; g.render(t); g.render(t + 0.01); ecranNote().clear();
        r.capturePng(out + "/textes-" + lg + "-" + nom + ".png");
        WFS_LOG("capture textes-%s-%s : écran %s, %s", lg.c_str(), nom.c_str(), nomEcran(g.ecran()), quoi);
    };
    auto toucher = [&](int id, float x, float y) { attendre(24); g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); image(); };
    float x = 0, y = 0;
    ui::langue::viderJournal();

    // ---------- accueil ----------
    g.allerA(Ecran::Menu); for (int i = 0; i < 60; i++) image();
    capture("menu-principal", "accueil : choix, record, ta voiture, aide des commandes, Crédits");
    // ---------- « Course solo » → choix du mode ----------
    etape(menuBouton("solo", x, y), "accueil : « Course solo » dessiné");
    toucher(1, x, y);
    etape(choixModeOuvert(), "« Course solo » → choix du mode");
    if (choixModeCentre("carte1", x, y)) toucher(2, x, y);   // Course à 5 : distance et difficulté visibles
    attendre(10);
    capture("modes", "choix du mode, Course à 5 (distance, difficulté)");
    if (choixModeCentre("carte0", x, y)) toucher(3, x, y);   // retour au Checkpoint (mode par défaut des autres bancs)
    etape(choixModeCentre("continuer", x, y), "choix du mode : « Continuer » dessiné");
    toucher(4, x, y);
    etape(g.ecran() == Ecran::Garage, "« Continuer » → garage");
    attendre(30);
    if (garageBouton("choisir", x, y)) toucher(5, x, y);
    etape(g.ecran() == Ecran::Terrain, "garage « Choisir » → choix du terrain");
    attendre(30);
    capture("terrain", "choix du terrain : description, longueur, quartiers, Jour / Nuit, Retour, Rouler");
    // ---------- « Rouler » → course (la partie est reconstruite) ----------
    etape(centreRouler(x, y), "choix du terrain : « Rouler » dessiné");
    toucher(6, x, y);
    for (int i = 0; i < 60 * 5 && g.ecran() != Ecran::Course; i++) image();
    etape(g.ecran() == Ecran::Course, "« Rouler » → course");
    attendre(60 * 3); image();
    capture("course-bouton-menu", "course : bouton « Menu » (le reste de la course : WFS-506)");
    // ---------- pause ----------
    etape(pauseBouton("menu", x, y), "course : bouton « Menu » dessiné");
    g.onTouch({7, x, y, true}); image(); g.onTouch({7, x, y, false}); image();
    etape(g.ecran() == Ecran::Pause, "bouton « Menu » → pause");
    capture("pause", "pause : sur-titre, titre, Reprendre, Recommencer, Quitter la course");
    etape(pauseBouton("quitter", x, y), "pause : « Quitter la course » dessiné");
    toucher(8, x, y);
    for (int i = 0; i < 30; i++) image();
    etape(g.ecran() == Ecran::Menu, "« Quitter la course » → accueil");
    // ---------- « Duel en ligne » → salon ----------
    for (int i = 0; i < 40; i++) image();
    etape(menuBouton("duel", x, y), "accueil : « Duel en ligne » dessiné");
    toucher(9, x, y);
    etape(g.ecran() == Ecran::Salon, "« Duel en ligne » → salon");
    attendre(20);
    capture("salon-accueil", "salon : pseudo, aide, « Rejoindre avec un code », Créer un duel, Retour");
    etape(salonBouton("creer", x, y), "salon : « Créer un duel » dessiné");
    toucher(10, x, y);
    attendre(20);
    etape(duelSession().inRoom(), "« Créer un duel » → salon ouvert");
    capture("salon-ouvert", "salon ouvert : code, Inviter, voiture, terrain, distance, pilotes, Prêt, Quitter le salon");
    if (salonBouton("quitter", x, y)) toucher(11, x, y);
    etape(!duelSession().inRoom(), "« Quitter le salon »");
    attendre(10);
    if (salonBouton("retour", x, y)) toucher(12, x, y);
    for (int i = 0; i < 40; i++) image();
    etape(g.ecran() == Ecran::Menu, "salon « Retour » → accueil");
    duelSession().leave();
    // ---------- Crédits ----------
    etape(credits::centreBouton(x, y), "accueil : bouton « Crédits » dessiné");
    toucher(13, x, y);
    etape(credits::ouvert(), "« Crédits » → écran des crédits");
    capture("credits", "crédits : titres, phrases, auteurs et licences");
    credits::defilerJusqua("Quaternius");
    capture("credits-milieu", "crédits : personnes, animaux, routes");
    if (credits::centreRetour(x, y)) toucher(14, x, y);
    etape(!credits::ouvert() && g.ecran() == Ecran::Menu, "crédits « Retour » → accueil");
    g.shutdown();

    // ---------- écran de chargement : étapes de Game::init, puis échec et bouton ----------
    Chargement& ch = chargement();
    auto noterChargement = [&](const std::string& nom) {   // image redessinée telle quelle, textes notés, capture
        ecranNote() = nom; ch.dessiner(); ecranNote().clear();
        r.capturePng(out + "/textes-" + lg + "-" + nom + ".png");
        WFS_LOG("capture textes-%s-%s : %s %d %%%s", lg.c_str(), nom.c_str(), ch.libelle().c_str(), (int)std::lround(ch.pourcent() * 100), ch.enErreur() ? " (échec)" : "");
    };
    ch.debut(r, nowSeconds());
    for (const char* e : {"Construction de la route", "Chargement du modèle 3D…", "Construction de la ville"}) ch.etape(ch.pourcent() + 0.25f, e);
    noterChargement("chargement");
    ch.erreur(false);
    noterChargement("chargement-erreur");
    if (chargementBouton(x, y)) { ch.toucher(15, x, y, true); ch.toucher(15, x, y, false); }
    etape(ch.reessaiDemande() && !ch.enErreur(), "chargement : « Réessayer » → relance demandée");
    ch.apresImage = nullptr;

    echecs += ui::langue::verifierJournal();
    WFS_LOG("textes (WFS-505) : menus %s : %d échec(s) → %s", lg.c_str(), echecs, echecs ? "ROUGE" : "VERT");
    choixModeBanc(false);
    courseBenchRules(false);
    return echecs;
}

} // namespace wfs::ecrans::textes
