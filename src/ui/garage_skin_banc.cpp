// WaForSpeed — banc d'essai de WFS-494 (WFS_BANC=garage-skins) : parcours du garage avec une vraie sauvegarde (fichier du banc,
// effacé au début et à la fin), par de vrais touchers sur les zones dessinées (comme garage_finition_banc.cpp) :
//   1. 208 (gratuite) : rangée « Skin » (Aucune, Rallye, Camouflage, Drift Tokyo, Néon nuit : cadenas, prix, « Cadeau »), bouton
//      « Choisis ta skin offerte », fenêtre (aperçu sur la voiture), Camouflage prise ; Drift Tokyo en aperçu, refus faute d'étoiles,
//      achat ; Néon nuit : « Bientôt dans le cadeau du jour », rien à acheter ;
//   2. Golf achetée (fenêtre d'achat de WFS-499) : la fenêtre de la skin offerte s'ouvre d'elle-même, Rallye prise ; bon de skin du
//      market utilisé sur Drift Tokyo ; Giulia : « Plus tard » puis Rallye ;
//   3. parcours RC-E18-09 : 208 « Choisir » → terrain → « Rouler » → course en Camouflage, retour au garage ; « Retour » après un
//      aperçu non possédé : la voiture de la course reprend sa skin ;
//   4. relance du jeu (sauvegarde relue) : skins, droits et achats gardés ;
//   5. course de jour et de nuit avec les 3 voitures (caméra de poursuite, Ville) ;
//   6. en anglais : GT86, rangée, fenêtre, Néon nuit, refus.
// Hors 1280x720 (WFS_TAILLE=1024x768, 2400x1080…) : étapes 1 (sans l'achat) et 6 seulement, captures garage-skins-<L>x<H>-* ; dans chaque
// langue, contrôle « les noms tiennent dans leurs boutons ». Captures garage-skins-*.png ; chaque contrôle au journal
// (« garage-skins (WFS-494) : OK / ÉCART ») ; rend le nombre d'écarts.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../core/log.h"
#include "../game/boutique.h"
#include "../game/game.h"
#include "../game/personnalisation.h"
#include "../game/skin_offerte.h"
#include "../render/renderer.h"
#include "../render/vehicule_skin.h"
#include "ecrans/terrain.h"   // « Rouler »
#include "garage.h"
#include "garage_commun.h"
#include "garage_skin.h"
#include "langue.h"
#include "text.h"

namespace wfs::ecrans {
using namespace garage;

int benchGarageSkins(Game& g, Renderer& r, const std::string& out) {
    ui::chargerPolice(r);
    int ecarts = 0;
    auto verifier = [&](bool ok, const std::string& quoi) { WFS_LOG("garage-skins (WFS-494) : %s : %s", ok ? "OK" : "ÉCART", quoi.c_str()); if (!ok) ecarts++; };
    const std::string chemin = out + "/garage-skins-sauvegarde-banc.txt";
    std::remove(chemin.c_str());
    Sauvegarde& s = sauvegarde();
    s.charger(chemin);   // partie neuve : 208, 500, GT86 gratuites ; Golf et Giulia à acheter
    ui::langue::choisir(ui::langue::Choix::Francais);
    const bool complet = (r.width() == 1280 && r.height() == 720) || std::getenv("WFS_GARAGE_SKINS_COMPLET");
    const std::string taille = complet ? "" : "-" + std::to_string(r.width()) + "x" + std::to_string(r.height());
    double t = 4000;
    bool ouvert = false;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    auto images = [&](int n) { for (int i = 0; i < n; i++) image(); };
    auto etatSkin = [&]() { const vehicule_skin::Etat e = vehicule_skin::etat(); return e.voiture + "/" + (e.id.empty() ? "aucune" : e.id); };
    auto capture = [&](const std::string& nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/garage-skins" + taille + "-" + nom + ".png");
        WFS_LOG("capture garage-skins-%s : %s (solde %lld ★, skin posée %s)", nom.c_str(), quoi, boutique::solde(s), etatSkin().c_str());
    };
    int doigt = 50;
    auto toucher = [&](float x, float y) { const int id = doigt++; g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); images(3); };
    auto fermer = [&]() { if (ouvert) g.shutdown(); ouvert = false; };
    auto ouvrirGarage = [&](const char* id) {
        fermer();
        if (!g.init(r, id, "ville")) { WFS_ERR("garage-skins : chargement impossible (%s)", id); return false; }
        ouvert = true; g.setAutopilot(false); g.camMode = 0;
        for (int i = 0; i < 30; i++) g.update(1.0 / 60);
        g.allerA(Ecran::Garage); images(45);
        return garageEnCours() != nullptr;
    };
    auto ordre = [](const char* id) { for (int o = 1; o < SkinGarage::NB; o++) if (std::strcmp(BOUTIQUE_SKINS_IDS[SkinGarage::catalogue(o)], id) == 0) return o; return 0; };
    auto toucherZone = [&](Zone z) { if (z.w > 0) toucher(z.x + z.w / 2, z.y + z.h / 2); return z.w > 0; };   // copie : zones réécrites
    // touche le bouton de la skin (« aucune » ou identifiant), après l'avoir amené par les chevrons si la rangée défile
    auto montrer = [&](const char* id) {
        const int o = std::strcmp(id, "aucune") == 0 ? 0 : ordre(id);
        for (int i = 0; i < 6 && g_z.skins[o].w <= 0 && g_z.chevronsSkin[0].w > 0; i++) toucherZone(g_z.chevronsSkin[0]);
        for (int i = 0; i < 6 && g_z.skins[o].w <= 0 && g_z.chevronsSkin[1].w > 0; i++) toucherZone(g_z.chevronsSkin[1]);
        if (!toucherZone(g_z.skins[o])) { WFS_ERR("garage-skins : bouton %s introuvable", id); ecarts++; return false; }
        images(6);
        return true;
    };
    auto texteTient = [&](const char* langue) {
        float px = 0;
        const float k = r.height() / 720.f;
        const bool tient = SkinGarage::police(k, px);
        int n = 0; bool chevauche = false; float fin = -1e9f;
        for (const Zone& z : g_z.skins) if (z.w > 0) { n++; if (z.x < fin - 0.5f) chevauche = true; fin = z.x + z.w; }
        verifier(tient && px >= 12 * k * 0.8f - 0.01f && n >= 1 && !chevauche,
                 std::string(langue) + " (" + std::to_string(r.width()) + "x" + std::to_string(r.height()) + ") : noms des skins dans leurs boutons (police " +
                 std::to_string(px / k).substr(0, 4) + " px sur 12), " + std::to_string(n) + " bouton(s) visible(s)" + (g_z.chevronsSkin[1].w > 0 ? ", chevron" : ""));
    };
    auto bouton = [&](const char* nom) { float x, y; if (!garageBouton(nom, x, y)) return false; toucher(x, y); images(4); return true; };
    auto voiture = [&](const char* id) {
        for (int i = 0; i < CAR_COUNT && garageVoitureAffichee() && std::strcmp(garageVoitureAffichee(), id) != 0; i++) { bouton("suivant"); images(45); }
        return garageVoitureAffichee() && std::strcmp(garageVoitureAffichee(), id) == 0;
    };
    auto possede = [&](const char* car, const char* id) { const perso::Objet* o = perso::objet(id); return o && perso::possede(s, *o, car); };
    auto fenetre = [&]() { float x, y; return garageBouton("prendre", x, y); };
    auto skinPosee = [&](const char* car, const char* id) { const vehicule_skin::Etat e = vehicule_skin::etat(); return e.voiture == car && e.id == id && e.textures == 1; };

    // 1. 208 : rangée, bouton « Choisis ta skin offerte », fenêtre, Camouflage prise
    if (!ouvrirGarage("p208")) { s.desactiver(); std::remove(chemin.c_str()); return ecarts + 1; }
    s.etoiles = 100; images(4);
    texteTient("français");
    verifier(!fenetre() && g_z.actionSkin.w > 0 && skins::offerteDue(s, "p208"), "208 : pas de fenêtre surprise, bouton « Choisis ta skin offerte » (skin due)");
    verifier(g_z.skins[4].w > 0 || g_z.chevronsSkin[1].w > 0, "rangée « Skin » : 5 boutons (ou chevrons sur écran étroit)");
    capture("p208-rangee", "208 : rangée « Skin » (cadenas, prix, Néon nuit « Cadeau »), bouton orange « Choisis ta skin offerte »");
    toucherZone(g_z.actionSkin); images(10);
    verifier(fenetre() && skinPosee("p208", "rallye"), "fenêtre « Choisis ta skin offerte » ouverte, Rallye en aperçu sur la voiture");
    capture("p208-fenetre-offerte", "fenêtre de la skin offerte : Rallye, Camouflage, Drift Tokyo (valeur en ★), Rallye en aperçu");
    bouton("skin1"); images(10);
    verifier(skinPosee("p208", "camouflage"), "fenêtre : Camouflage touchée, aperçu sur la voiture");
    capture("p208-fenetre-camouflage", "fenêtre : Camouflage choisie, aperçu sur la voiture");
    bouton("prendre"); images(10);
    verifier(!fenetre() && possede("p208", "camouflage") && s.skin["p208"] == "camouflage" && s.skinsOffertes.count("p208") && boutique::solde(s) == 100,
             "« Prendre » : Camouflage offerte (0 ★ débitée), skin-p208=camouflage, droit utilisé");
    verifier(g_z.actionSkin.w <= 0, "plus de bouton « Choisis ta skin offerte » sur la 208");
    capture("p208-camouflage", "208 en Camouflage offerte : plus de cadenas sur Camouflage");
    montrer("drifttokyo");
    verifier(!possede("p208", "drifttokyo") && skinPosee("p208", "drifttokyo") && boutique::solde(s) == 100, "aperçu Drift Tokyo : sur la voiture, rien de débité");
    capture("p208-drift-apercu", "aperçu Drift Tokyo : « Acheter 600 ★ », équivalents en médailles et en euros");
    if (complet) {
        bouton("choisir");
        verifier(boutique::solde(s) == 100 && !possede("p208", "drifttokyo") && s.skin["p208"] == "camouflage", "« Acheter » avec 100 ★ : refus, rien de débité");
        capture("p208-il-manque", "refus : bandeau « Il te manque 500 ★ »");
        boutique::crediter(s, 900, "banc WFS-494");
        bouton("choisir");
        verifier(possede("p208", "drifttokyo") && s.skin["p208"] == "drifttokyo" && boutique::solde(s) == 400, "achat Drift Tokyo : 600 ★ débitées une fois, skin-p208=drifttokyo");
        verifier(!possede("abarth", "drifttokyo"), "le Drift Tokyo de la 208 ne vaut pas pour la 500 (RG-03)");
    }
    montrer("neonnuit");
    verifier(!possede("p208", "neonnuit") && skinPosee("p208", "neonnuit"), "Néon nuit : aperçu, pas possédée");
    capture("p208-neon-cadeau", "Néon nuit : cadenas, « Bientôt dans le cadeau du jour », bouton « Choisir » (rien à acheter)");
    if (complet) {
        const long long avant = boutique::solde(s);
        bouton("choisir"); images(4);
        verifier(boutique::solde(s) == avant && !possede("p208", "neonnuit"), "Néon nuit : « Choisir » n'achète rien");
        if (!ouvrirGarage("p208")) ecarts++;
    }
    montrer("camouflage");
    verifier(s.skin["p208"] == "camouflage" && skinPosee("p208", "camouflage"), "retour sur Camouflage (possédée : gardée tout de suite)");

    if (complet) {
    // 2. Golf achetée : fenêtre de la skin offerte d'elle-même ; bon de skin ; Giulia « Plus tard »
    verifier(voiture("golf"), "flèche « suivante » jusqu'à la Golf (bloquée)");
    verifier(!fenetre() && g_z.actionSkin.w <= 0, "Golf bloquée : pas de skin offerte");
    boutique::crediter(s, boutique::prixVoiture("golf"), "banc WFS-494 : Golf");
    bouton("choisir"); images(4);
    bouton("confirmer"); images(20);
    verifier(boutique::possedeVoiture(s, "golf") && fenetre(), "Golf achetée : la fenêtre « Choisis ta skin offerte » s'ouvre d'elle-même (RG-04 de WFS-499)");
    capture("golf-achat-fenetre-offerte", "Golf achetée : fenêtre de la skin offerte ouverte d'elle-même");
    bouton("skin0"); bouton("prendre"); images(20);
    verifier(possede("golf", "rallye") && s.skin["golf"] == "rallye" && skinPosee("golf", "rallye"), "Golf : Rallye offerte, posée sur la voiture du garage");
    capture("golf-rallye", "Golf en Rallye offerte");
    s.market.bonsSkin = 1;
    montrer("drifttokyo");
    verifier(g_z.actionSkin.w > 0, "bon de skin du market : « Utiliser un bon (1) » sur Drift Tokyo");
    capture("golf-bon", "Golf : Drift Tokyo en aperçu, « Utiliser un bon (1) »");
    toucherZone(g_z.actionSkin); images(6);
    verifier(possede("golf", "drifttokyo") && s.market.bonsSkin == 0 && s.skin["golf"] == "drifttokyo", "bon utilisé : Drift Tokyo à la Golf, 0 bon, 0 ★ débitée");
    s.voituresPossedees.insert("giulia");
    verifier(voiture("giulia"), "flèche « suivante » jusqu'à la Giulia (possédée)");
    verifier(g_z.actionSkin.w > 0 && !fenetre(), "Giulia : bouton « Choisis ta skin offerte »");
    toucherZone(g_z.actionSkin); images(6);
    bouton("plustard"); images(6);
    verifier(!fenetre() && skins::offerteDue(s, "giulia") && !s.skin.count("giulia"), "« Plus tard » : rien de pris, la skin reste due");
    toucherZone(g_z.actionSkin); images(6);
    bouton("skin0"); bouton("prendre"); images(10);
    verifier(possede("giulia", "rallye") && s.skin["giulia"] == "rallye", "Giulia : Rallye offerte");
    capture("giulia-rallye", "Giulia en Rallye offerte");
    // 3. parcours RC-E18-09 : 208 Camouflage → « Choisir » → terrain → « Rouler » → course
    if (ouvrirGarage("p208")) {
        verifier(skinPosee("p208", "camouflage"), "garage : la 208 est en Camouflage (sauvegarde)");
        float x, y;
        if (garageBouton("choisir", x, y)) toucher(x, y);
        images(20); g.render(t);
        if (centreRouler(x, y)) toucher(x, y); else { WFS_ERR("garage-skins : « Rouler » absent (écran %s)", nomEcran(g.ecran())); ecarts++; }
        for (int i = 0; i < 120 && g.ecran() != Ecran::Course; i++) image();
        g.setAutopilot(true); images(300); g.setAutopilot(false);
        verifier(g.ecran() == Ecran::Course && skinPosee("p208", "camouflage"), "« Choisir » → « Rouler » : la 208 roule en Camouflage");
        capture("parcours-p208-course", "parcours : la 208 Camouflage en course après « Choisir » et « Rouler »");
        g.allerA(Ecran::Garage); images(30);
        montrer("neonnuit"); images(6);
        bouton("retour"); images(20);
        verifier(skinPosee("p208", "camouflage") && s.skin["p208"] == "camouflage", "« Retour » après l'aperçu Néon nuit : la voiture de la course reprend le Camouflage");
        capture("parcours-p208-retour-menu", "« Retour » au menu : la 208 garde son Camouflage");
    }
    // 4. relance du jeu : la sauvegarde est relue
    fermer();
    const long long solde = boutique::solde(s);
    s.attendre(); s.charger(chemin);
    verifier(s.skin["p208"] == "camouflage" && s.skin["golf"] == "drifttokyo" && s.skin["giulia"] == "rallye" && s.skinsOffertes.size() == 3 &&
             possede("p208", "drifttokyo") && possede("golf", "rallye") && !possede("abarth", "camouflage") && boutique::solde(s) == solde && s.market.bonsSkin == 0,
             "relance : skins choisies, skins offertes, achats, bon et solde gardés");
    if (ouvrirGarage("golf")) {
        verifier(skinPosee("golf", "drifttokyo"), "relance : la Golf repart en Drift Tokyo");
        capture("relance-golf", "après relance : la Golf en Drift Tokyo");
    }
    fermer();
    // 5. course de jour et de nuit
    const struct { const char* id; const char* skin; } COURSES[] = {{"p208", "camouflage"}, {"golf", "drifttokyo"}, {"giulia", "rallye"}};
    for (const char* moment : {"jour", "nuit"}) {
        setenv("WFS_MOMENT", moment, 1);
        for (const auto& c : COURSES) {
            if (!g.init(r, c.id, "ville")) { WFS_ERR("garage-skins : course %s impossible", c.id); ecarts++; continue; }
            ouvert = true; g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
            for (int i = 0; i < 300; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 6 == 0) g.render(t); }
            verifier(skinPosee(c.id, c.skin), std::string("course ") + c.id + " (" + moment + ") : skin " + c.skin);
            capture(std::string("course-") + c.id + "-" + moment, "course avec la skin possédée (caméra de poursuite, Ville)");
            g.setAutopilot(false); fermer();
        }
    }
    unsetenv("WFS_MOMENT");
    }
    // 6. en anglais : GT86 (skin offerte due)
    ui::langue::choisir(ui::langue::Choix::Anglais);
    if (ouvrirGarage("gt86")) {
        s.etoiles = 40; images(4);
        texteTient("English");
        capture("en-gt86-rangee", "English: GT86, « Skin » row, locks, prices, « Choose your free skin »");
        toucherZone(g_z.actionSkin); images(10);
        verifier(fenetre(), "English: free skin window");
        capture("en-gt86-fenetre", "English: « Choose your free skin » window");
        bouton("plustard"); images(6);
        montrer("neonnuit");
        capture("en-neon", "English: Neon Night « Coming soon in the daily gift »");
        montrer("rallye");
        bouton("choisir");
        verifier(boutique::solde(s) == 40 && !possede("gt86", "rallye"), "English: not enough stars, nothing charged");
        capture("en-il-manque", "English: « You need N more ★ »");
    }
    fermer();
    ui::langue::choisir(ui::langue::Choix::Automatique);
    s.attendre(); s.desactiver(); std::remove(chemin.c_str());
    WFS_LOG("garage-skins (WFS-494) : %d écart(s)", ecarts);
    return ecarts;
}

} // namespace wfs::ecrans
