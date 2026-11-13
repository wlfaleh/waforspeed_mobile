// WaForSpeed — banc d'essai de WFS-489 (WFS_BANC=garage-couleurs) : parcours du garage avec une vraie sauvegarde (fichier du banc,
// effacé au début et à la fin), par de vrais touchers sur les zones dessinées :
//   1. 208 : rangée des 28 couleurs (cadenas et prix), défilement au doigt jusqu'au bout ;
//   2. 208 : Bleu électrique en aperçu (« Acheter »), refus faute d'étoiles (« Il te manque N ★ », rien de débité), achat ;
//   3. 208 : onglet « Toit et rétros », Noir (droit de la couleur secondaire à acheter), achat ;
//   4. GT86 Orange fluo et Golf Rose achetées ; Golf Violet néon en aperçu puis « Retour » : la Golf reste Rose ;
//   5. relance du jeu (sauvegarde relue) : couleurs gardées ; garage de la 208 ;
//   6. course de jour et de nuit avec les 3 voitures (caméra de poursuite, Ville) ;
//   7. en anglais : garage de la Golf, aperçu, refus, onglet « Roof & mirrors ».
// Captures garage-couleurs-*.png ; chaque contrôle au journal (« garage-couleurs (WFS-489) : OK / ÉCART ») ; rend le nombre d'écarts.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../core/log.h"
#include "../game/boutique.h"
#include "../game/game.h"
#include "../game/personnalisation.h"
#include "../render/renderer.h"
#include "garage.h"
#include "garage_commun.h"
#include "langue.h"
#include "text.h"

namespace wfs::ecrans {
using namespace garage;

int benchGarageCouleurs(Game& g, Renderer& r, const std::string& out) {
    ui::chargerPolice(r);
    int ecarts = 0;
    auto verifier = [&](bool ok, const std::string& quoi) { WFS_LOG("garage-couleurs (WFS-489) : %s : %s", ok ? "OK" : "ÉCART", quoi.c_str()); if (!ok) ecarts++; };
    const std::string chemin = out + "/garage-couleurs-sauvegarde-banc.txt";
    std::remove(chemin.c_str());
    Sauvegarde& s = sauvegarde();
    s.charger(chemin);
    ui::langue::choisir(ui::langue::Choix::Francais);
    double t = 4000;
    bool ouvert = false;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    auto images = [&](int n) { for (int i = 0; i < n; i++) image(); };
    auto capture = [&](const std::string& nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/garage-couleurs-" + nom + ".png");
        WFS_LOG("capture garage-couleurs-%s : %s (solde %lld ★)", nom.c_str(), quoi, boutique::solde(s));
    };
    int doigt = 50;
    auto toucher = [&](float x, float y) { const int id = doigt++; g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); images(3); };
    auto toucherZone = [&](Zone z) {   // copie : toucher redessine l'écran (zones réécrites)
        if (z.w > 0) toucher(z.x + z.w / 2, z.y + z.h / 2); return z.w > 0; };
    auto fermer = [&]() { if (ouvert) g.shutdown(); ouvert = false; };
    auto ouvrirGarage = [&](const char* id) {
        fermer();
        if (!g.init(r, id, "ville")) { WFS_ERR("garage-couleurs : chargement impossible (%s)", id); return false; }
        ouvert = true; g.setAutopilot(false); g.camMode = 0;
        for (int i = 0; i < 30; i++) g.update(1.0 / 60);
        g.allerA(Ecran::Garage); images(45);
        return garageEnCours() != nullptr;
    };
    // touche la pastille p (indice dans PAINTS ; toit : onglet « Toit et rétros ») après l'avoir amenée par les chevrons
    auto montrer = [&](bool toit, int p) {
        for (int i = 0; i < 12 && g_z.chevrons[0].w > 0; i++) toucherZone(g_z.chevrons[0]);   // retour au début de la rangée
        for (int i = 0; i < 12; i++) {
            if (toucherZone(toit ? g_z.toit[p + 1] : g_z.peintures[p])) { images(6); return true; }
            if (!toucherZone(g_z.chevrons[1])) break;
        }
        WFS_ERR("garage-couleurs : pastille %s introuvable", PAINTS[p].id); ecarts++;
        return false;
    };
    auto bouton = [&](const char* nom) { float x, y; if (!garageBouton(nom, x, y)) return false; toucher(x, y); images(4); return true; };
    auto voiture = [&](const char* id) {
        for (int i = 0; i < CAR_COUNT && garageVoitureAffichee() && std::strcmp(garageVoitureAffichee(), id) != 0; i++) { bouton("suivant"); images(45); }
        return garageVoitureAffichee() && std::strcmp(garageVoitureAffichee(), id) == 0;
    };
    auto possede = [&](const char* car, const char* id) { const perso::Objet* o = perso::objet(id); return o && perso::possede(s, *o, car); };
    const int BLEU_E = peintureParId("bleuelectrique"), NOIR = peintureParId("noir"), ORANGE_F = peintureParId("orangefluo"),
              ROSE = peintureParId("rose"), VIOLET_N = peintureParId("violetneon"), MAGENTA = peintureParId("magenta");

    // 1. 208 : la rangée, puis défilement au doigt
    if (!ouvrirGarage("p208")) { s.desactiver(); std::remove(chemin.c_str()); return ecarts + 1; }
    s.etoiles = 30;
    images(4);
    capture("p208-debut", "208 : 28 couleurs, cadenas et prix des 20 couleurs payantes, solde 30 ★");
    if (const Zone& z = g_z.peintures[2]; z.w > 0) {   // glisser vers la gauche sur la rangée : défilement
        const float x0 = z.x + z.w / 2, y0 = z.y + z.h / 2, d = r.height() * 0.9f;
        g.onTouch({90, x0, y0, true}); image();
        for (int i = 1; i <= 20; i++) { g.onTouch({90, x0 - d * i / 20, y0, true}); image(); }
        g.onTouch({90, x0 - d, y0, false}); images(4);
    }
    verifier(g_z.peintures[NB_PEINTURES - 1].w > 0 && g_z.peintures[0].w <= 0, "glisser : la rangée défile jusqu'à la 28e couleur (Bleu ciel)");
    capture("p208-defile", "rangée défilée au doigt jusqu'au bout (Bleu ciel)");
    const bool rapide = std::getenv("WFS_GARAGE_COULEURS_RAPIDE") != nullptr;   // essai local : étapes 1 et 7 seulement
    if (!rapide) {
    // 2. Bleu électrique : aperçu, refus faute d'étoiles, achat
    montrer(false, BLEU_E);
    capture("p208-bleuelectrique-apercu", "aperçu Bleu électrique sur la 208 : « Acheter 50 ★ », équivalents");
    verifier(!possede("p208", "bleuelectrique") && boutique::solde(s) == 30, "aperçu : rien de débité, couleur non possédée");
    bouton("choisir");
    verifier(boutique::solde(s) == 30 && !possede("p208", "bleuelectrique"), "« Acheter » avec 30 ★ pour 50 ★ : refus, rien de débité");
    capture("p208-il-manque", "refus : bandeau « Il te manque 20 ★ »");
    boutique::crediter(s, 370, "banc WFS-489");
    bouton("choisir");
    verifier(boutique::solde(s) == 350 && possede("p208", "bleuelectrique") && s.paint["p208"] == "bleuelectrique", "achat Bleu électrique : 50 ★ débitées une fois, couleur gardée (paint-p208)");
    capture("p208-bleuelectrique", "208 Bleu électrique achetée : « Choisir »");
    // 3. toit et rétros Noir
    toucherZone(g_z.onglets[1]); images(4);
    capture("p208-toit", "onglet « Toit et rétros » : « Comme la carrosserie » en premier");
    montrer(true, NOIR);
    capture("p208-toit-noir-apercu", "toit et rétros Noir en aperçu : droit de la couleur secondaire 50 ★");
    bouton("choisir");
    verifier(boutique::solde(s) == 300 && possede("p208", perso::ID_SECONDAIRE) && s.paint2["p208"] == "noir", "achat toit et rétros : 50 ★, paint2-p208=noir");
    capture("p208-toit-noir", "208 Bleu électrique, toit et rétros Noir");
    toucherZone(g_z.onglets[0]); images(4);
    // 4. GT86 Orange fluo, Golf Rose ; aperçu non acheté oublié
    verifier(voiture("gt86"), "flèche « suivante » jusqu'à la GT86");
    montrer(false, ORANGE_F); bouton("choisir");
    verifier(possede("gt86", "orangefluo") && s.paint["gt86"] == "orangefluo", "GT86 Orange fluo achetée");
    capture("gt86-orangefluo", "GT86 Orange fluo achetée");
    verifier(voiture("golf"), "flèche « suivante » jusqu'à la Golf");
    montrer(false, ROSE); bouton("choisir");
    verifier(possede("golf", "rose") && s.paint["golf"] == "rose", "Golf Rose achetée");
    capture("golf-rose", "Golf Rose achetée");
    montrer(false, VIOLET_N);
    capture("golf-violetneon-apercu", "Golf Violet néon en aperçu (non achetée)");
    bouton("retour"); images(10);
    verifier(s.paint["golf"] == "rose" && !possede("golf", "violetneon"), "« Retour » sans acheter : la Golf garde Rose (aperçu oublié)");
    // 5. relance du jeu : la sauvegarde est relue
    fermer();
    const long long solde = boutique::solde(s);
    s.attendre(); s.charger(chemin);
    verifier(s.paint["p208"] == "bleuelectrique" && s.paint2["p208"] == "noir" && s.paint["gt86"] == "orangefluo" && s.paint["golf"] == "rose" &&
             boutique::solde(s) == solde && possede("p208", perso::ID_SECONDAIRE), "relance : couleurs, toit et rétros, achats et solde gardés");
    if (ouvrirGarage("p208")) capture("relance-p208", "après relance : la 208 est Bleu électrique, toit et rétros Noir");
    if (voiture("golf")) { images(4); capture("relance-golf", "après relance : la Golf est Rose (pas Violet néon)"); }
    fermer();
    // 6. course de jour et de nuit
    for (const char* moment : {"jour", "nuit"}) {
        setenv("WFS_MOMENT", moment, 1);
        for (const char* id : {"p208", "gt86", "golf"}) {
            if (!g.init(r, id, "ville")) { WFS_ERR("garage-couleurs : course %s impossible", id); continue; }
            ouvert = true; g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
            for (int i = 0; i < 300; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 6 == 0) g.render(t); }
            capture(std::string("course-") + id + "-" + moment, "course avec les couleurs achetées (caméra de poursuite, Ville)");
            g.setAutopilot(false); fermer();
        }
    }
    unsetenv("WFS_MOMENT");
    }
    // 7. en anglais
    ui::langue::choisir(ui::langue::Choix::Anglais);
    if (ouvrirGarage("golf")) {
        s.etoiles = 40; images(4);
        capture("en-golf", "English: Golf, 28 colors, locks and prices");
        montrer(false, MAGENTA);
        capture("en-golf-magenta-apercu", "English: Magenta preview, « Buy 75 ★ »");
        bouton("choisir");
        verifier(boutique::solde(s) == 40 && !possede("golf", "magenta"), "English: not enough stars, nothing charged");
        capture("en-il-manque", "English: « You need N more ★ »");
        toucherZone(g_z.onglets[1]); images(4);
        capture("en-toit", "English: « Roof & mirrors » tab");
    }
    fermer();
    ui::langue::choisir(ui::langue::Choix::Automatique);
    s.attendre(); s.desactiver(); std::remove(chemin.c_str());
    WFS_LOG("garage-couleurs (WFS-489) : %d écart(s)", ecarts);
    return ecarts;
}

} // namespace wfs::ecrans
