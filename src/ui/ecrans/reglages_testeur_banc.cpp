// WaForSpeed — banc d'essai de WFS-546 (WFS_BANC=testeur) : mode testeur, bouton étoile (+2500 ★) du menu caché de diagnostic.
// Vraie sauvegarde (fichier du banc, effacé au début et à la fin), solde de départ 0 ★, vrais touchers comme sur Android :
//   1. menu caché ouvert en français : bouton étoile (icône seule, RG-05) dans la rangée du haut, solde 0 ★ ;
//   2. 2 appuis (chacun : posé, 2 mouvements du même doigt, levé) : bouton éclairé, solde 5000 ★, sauvegarde relue = 5000 ★ ;
//   3. menu principal : le compteur d'étoiles (WFS-537) montre 5 000 ;
//   4. menu caché en anglais : même bouton étoile (aucun texte) ;
//   5. garage de la 208 (coefficient 1 : prix du catalogue) : Chromé (400 ★) en aperçu, « Acheter », puis achetée (cadenas tombé, 400 ★ débitées).
// Captures testeur-*.png ; chaque contrôle au journal (« testeur (WFS-546) : OK / ÉCART ») ; rend le nombre d'écarts.
#include <cstdio>
#include <cstring>
#include "../../core/log.h"
#include "../../game/boutique.h"
#include "../../game/game.h"
#include "../../game/personnalisation.h"
#include "../../game/sauvegarde.h"
#include "../../render/renderer.h"
#include "../garage.h"
#include "../garage_commun.h"
#include "../garage_finition.h"
#include "../langue.h"
#include "../text.h"
#include "reglages.h"

namespace wfs::ecrans {

int benchTesteur(Game& g, Renderer& r, const std::string& out) {
    ui::chargerPolice(r);
    int ecarts = 0;
    auto verifier = [&](bool ok, const std::string& quoi) { WFS_LOG("testeur (WFS-546) : %s : %s", ok ? "OK" : "ÉCART", quoi.c_str()); if (!ok) ecarts++; };
    const std::string chemin = out + "/testeur-sauvegarde-banc.txt";
    std::remove(chemin.c_str());
    Sauvegarde& s = sauvegarde();
    s.charger(chemin);
    for (int i = 0; i < CAR_COUNT; i++) s.voituresPossedees.insert(CARS[i].id);   // le garage n'est pas le sujet : voitures possédées
    s.etoiles = 0;
    s.profil.cree = true;   // pas d'écran « Crée ton pilote » (WFS-481) : partie déjà commencée
    ui::langue::choisir(ui::langue::Choix::Francais);
    if (!g.init(r, "p208", "ville")) { WFS_ERR("testeur : chargement impossible"); s.desactiver(); std::remove(chemin.c_str()); return 1; }
    g.setAutopilot(false); g.camMode = 0;
    double t = 4000;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    auto images = [&](int n) { for (int i = 0; i < n; i++) image(); };
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/testeur-" + nom + ".png");
        WFS_LOG("capture testeur-%s : %s (solde %lld ★)", nom, quoi, boutique::solde(s));
    };
    int doigt = 70;
    // appui comme sur Android : posé, 2 mouvements du même doigt (down = vrai), levé
    auto appui = [&](float x, float y) {
        const int id = doigt++;
        g.onTouch({id, x, y, true}); image();
        g.onTouch({id, x + 3, y + 2, true}); g.onTouch({id, x + 6, y + 1, true}); image();
        g.onTouch({id, x + 6, y + 1, false}); image();
    };
    auto ouvrirDiag = [&]() { g.allerA(Ecran::Menu); images(20); ouvrirReglagesBanc(true); ouvrirMenuCacheBanc(true); images(3); };
    auto fermerDiag = [&]() { ouvrirMenuCacheBanc(false); ouvrirReglagesBanc(false); images(3); };

    // 1. menu caché, français
    ouvrirDiag();
    float bx = 0, by = 0;
    verifier(boutonEtoilesDiag(bx, by), "RG-01 : bouton étoile (+2500 ★) dessiné dans le menu caché");
    capture("1-diagnostic-fr", "menu caché : bouton étoile à gauche de « Banc auto »");
    // 2. deux appuis
    appui(bx, by);
    verifier(boutique::solde(s) == 2500, "RG-02 : 1er appui (avec mouvements du doigt) : 2500 ★, une seule fois");
    appui(bx, by);
    capture("2-etoile-eclairee", "juste après le 2e appui : bouton étoile éclairé");
    verifier(boutique::solde(s) == 5000, "RG-02 : 2 appuis : 5000 ★");
    s.attendre();
    { Sauvegarde relue; const bool lu = relue.lire([&] { std::string c; if (FILE* f = std::fopen(chemin.c_str(), "rb")) { char b[4096]; size_t n; while ((n = std::fread(b, 1, sizeof b, f)) > 0) c.append(b, n); std::fclose(f); } return c; }());
      verifier(lu && relue.etoiles == 5000, "RG-03 : fichier de sauvegarde relu : etoiles=" + std::to_string(relue.etoiles)); }
    // 3. compteur du menu
    fermerDiag();
    images(20);
    capture("3-menu-compteur", "menu principal : compteur d'étoiles à 5 000");
    // 4. anglais
    ui::langue::choisir(ui::langue::Choix::Anglais);
    ouvrirDiag();
    capture("4-diagnostic-en", "menu caché en anglais : bouton étoile, aucun texte");
    fermerDiag();
    ui::langue::choisir(ui::langue::Choix::Francais);
    // 5. garage : Chromé à 400 ★
    using namespace garage;
    g.allerA(Ecran::Garage); images(45);
    int o = -1;
    for (int i = 0; i < FinitionGarage::NB; i++) if (std::strcmp(BOUTIQUE_FINITIONS_IDS[FinitionGarage::catalogue(i)], "chrome") == 0) o = i;
    const perso::Objet* chrome = perso::objet("chrome");
    const long long prix = chrome ? perso::prix(*chrome, "p208") : -1;
    // rangée qui défile : chevron « suivant » jusqu'à Chromé (comme garage_finition_banc.cpp)
    for (int i = 0; i < 8 && o >= 0 && g_z.finitions[o].w <= 0 && g_z.chevronsFinition[1].w > 0; i++) {
        const Zone c = g_z.chevronsFinition[1]; appui(c.x + c.w / 2, c.y + c.h / 2); images(3);
    }
    verifier(o >= 0 && g_z.finitions[o].w > 0 && prix == 400, "garage : bouton Chromé visible, prix " + std::to_string(prix) + " ★");
    if (o >= 0 && g_z.finitions[o].w > 0) {
        const Zone z = g_z.finitions[o];
        appui(z.x + z.w / 2, z.y + z.h / 2); images(6);
        capture("5-garage-chrome-apercu", "garage : Chromé (400 ★) en aperçu, « Acheter 400 ★ », solde 5 000 ★");
        float cx, cy;
        if (garageBouton("choisir", cx, cy)) { appui(cx, cy); images(6); }
        verifier(chrome && perso::possede(s, *chrome, "p208") && boutique::solde(s) == 5000 - prix, "Chromé acheté avec les étoiles du mode testeur : solde " + std::to_string(boutique::solde(s)) + " ★");
        capture("6-garage-chrome-achete", "garage : Chromé acheté (cadenas tombé), 400 ★ débitées");
    }
    g.allerA(Ecran::Menu); images(3);
    g.shutdown();
    s.desactiver(); std::remove(chemin.c_str());
    WFS_LOG("testeur (WFS-546) : %d écart(s)", ecarts);
    return ecarts;
}

} // namespace wfs::ecrans
