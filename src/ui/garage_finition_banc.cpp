// WaForSpeed — banc d'essai de WFS-491 (WFS_BANC=garage-finitions) : parcours du garage avec une vraie sauvegarde (fichier du banc,
// effacé au début et à la fin), par de vrais touchers sur les zones dessinées (comme garage_couleur_banc.cpp) :
//   1. Golf : rangée « Finition » (8 boutons, cadenas et prix), Caméléon en aperçu sous 3 angles, refus faute d'étoiles
//      (« Il te manque N ★ », rien de débité), achat ;
//   2. Giulia Mat et 500 (abarth) Chromé achetées ; Golf Candy en aperçu puis « Retour » : la Golf reste Caméléon ;
//   3. parcours RC-E18-09 : 500 « Choisir » → terrain → « Rouler » → course en Chromé, retour au garage ;
//   4. relance du jeu (sauvegarde relue) : finitions gardées, une finition achetée pour une seule voiture ;
//   5. course de jour et de nuit avec les 3 voitures (caméra de poursuite, Ville) ;
//   6. en anglais : garage de la Golf, aperçu Nacré, refus.
// Hors 1280x720 (WFS_TAILLE=1024x768, 1280x800, 2400x1080) : étapes 1 (sans l'achat) et 6 seulement, rangée qui défile, captures
// garage-finitions-<L>x<H>-*.png ; dans chaque langue, contrôle « les noms tiennent dans leurs boutons » (police réduite de 20 % au plus).
// Captures garage-finitions-*.png ; chaque contrôle au journal (« garage-finitions (WFS-491) : OK / ÉCART ») ; rend le nombre d'écarts.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../core/log.h"
#include "../game/boutique.h"
#include "../game/game.h"
#include "../game/personnalisation.h"
#include "../render/renderer.h"
#include "../render/vehicule_finitions.h"
#include "ecrans/terrain.h"   // « Rouler »
#include "garage.h"
#include "garage_commun.h"
#include "garage_finition.h"
#include "langue.h"
#include "text.h"

namespace wfs::ecrans {
using namespace garage;

int benchGarageFinitions(Game& g, Renderer& r, const std::string& out) {
    namespace vf = vehicule_finitions;
    ui::chargerPolice(r);
    int ecarts = 0;
    auto verifier = [&](bool ok, const std::string& quoi) { WFS_LOG("garage-finitions (WFS-491) : %s : %s", ok ? "OK" : "ÉCART", quoi.c_str()); if (!ok) ecarts++; };
    const std::string chemin = out + "/garage-finitions-sauvegarde-banc.txt";
    std::remove(chemin.c_str());
    Sauvegarde& s = sauvegarde();
    s.charger(chemin);
    // WFS-499 : avec la sauvegarde active, la Golf est à acheter ; ce banc ne teste que les finitions : il possède ses voitures
    for (int i = 0; i < CAR_COUNT; i++) s.voituresPossedees.insert(CARS[i].id);
    ui::langue::choisir(ui::langue::Choix::Francais);
    // écran de référence 1280x720 : parcours complet ; autre taille (4:3, 16:10, téléphone 2400x1080) : écran du garage seulement
    // (rangée, défilement, aperçu, refus, FR / EN), captures suffixées par la taille ; les courses sont vérifiées en 1280x720
    const bool complet = (r.width() == 1280 && r.height() == 720) || std::getenv("WFS_GARAGE_FINITIONS_COMPLET");
    const std::string taille = complet ? "" : "-" + std::to_string(r.width()) + "x" + std::to_string(r.height());
    double t = 4000;
    bool ouvert = false;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    auto images = [&](int n) { for (int i = 0; i < n; i++) image(); };
    auto capture = [&](const std::string& nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/garage-finitions" + taille + "-" + nom + ".png");
        WFS_LOG("capture garage-finitions-%s : %s (solde %lld ★, finition du rendu %d)", nom.c_str(), quoi, boutique::solde(s), r.finitionJoueur());
    };
    int doigt = 50;
    auto toucher = [&](float x, float y) { const int id = doigt++; g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); images(3); };
    auto fermer = [&]() { if (ouvert) g.shutdown(); ouvert = false; };
    auto ouvrirGarage = [&](const char* id) {
        fermer();
        if (!g.init(r, id, "ville")) { WFS_ERR("garage-finitions : chargement impossible (%s)", id); return false; }
        ouvert = true; g.setAutopilot(false); g.camMode = 0;
        for (int i = 0; i < 30; i++) g.update(1.0 / 60);
        g.allerA(Ecran::Garage); images(45);
        return garageEnCours() != nullptr;
    };
    // ordre affiché (0..7) d'une finition du catalogue
    auto ordre = [](const char* id) { for (int o = 0; o < FinitionGarage::NB; o++) if (std::strcmp(BOUTIQUE_FINITIONS_IDS[FinitionGarage::catalogue(o)], id) == 0) return o; return -1; };
    auto toucherZone = [&](Zone z) { if (z.w > 0) toucher(z.x + z.w / 2, z.y + z.h / 2); return z.w > 0; };   // copie : zones réécrites
    // touche le bouton de la finition, après l'avoir amené par les chevrons si la rangée défile (écran étroit)
    auto montrer = [&](const char* id) {
        const int o = std::max(0, ordre(id));
        for (int i = 0; i < 8 && g_z.finitions[o].w <= 0 && g_z.chevronsFinition[0].w > 0; i++) toucherZone(g_z.chevronsFinition[0]);
        for (int i = 0; i < 8 && g_z.finitions[o].w <= 0 && g_z.chevronsFinition[1].w > 0; i++) toucherZone(g_z.chevronsFinition[1]);
        if (!toucherZone(g_z.finitions[o])) { WFS_ERR("garage-finitions : bouton %s introuvable", id); ecarts++; return false; }
        images(6);
        return true;
    };
    // RG-04 / E20 : le plus long nom tient dans son bouton, police réduite de 20 % au plus ; boutons visibles entiers, sans chevauchement
    auto texteTient = [&](const char* langue) {
        float px = 0;
        const float k = r.height() / 720.f;
        const bool tient = FinitionGarage::police(k, px);
        int n = 0; bool chevauche = false; float fin = -1e9f;
        for (const Zone& z : g_z.finitions) if (z.w > 0) { n++; if (z.x < fin - 0.5f) chevauche = true; fin = z.x + z.w; }
        verifier(tient && px >= 12 * k * 0.8f - 0.01f && n >= 1 && !chevauche,
                 std::string(langue) + " (" + std::to_string(r.width()) + "x" + std::to_string(r.height()) + ") : noms des finitions dans leurs boutons (police " +
                 std::to_string(px / k).substr(0, 4) + " px sur 12), " + std::to_string(n) + " bouton(s) visible(s)" + (g_z.chevronsFinition[1].w > 0 ? ", chevron" : ""));
    };
    auto bouton = [&](const char* nom) { float x, y; if (!garageBouton(nom, x, y)) return false; toucher(x, y); images(4); return true; };
    auto voiture = [&](const char* id) {
        for (int i = 0; i < CAR_COUNT && garageVoitureAffichee() && std::strcmp(garageVoitureAffichee(), id) != 0; i++) { bouton("suivant"); images(45); }
        return garageVoitureAffichee() && std::strcmp(garageVoitureAffichee(), id) == 0;
    };
    auto possede = [&](const char* car, const char* id) { const perso::Objet* o = perso::objet(id); return o && perso::possede(s, *o, car); };
    auto prix = [&](const char* car, const char* id) { const perso::Objet* o = perso::objet(id); return o ? (long long)perso::prix(*o, car) : 0LL; };
    // la voiture du garage tourne seule : quelques secondes entre deux angles du caméléon
    auto tourner = [&](int n) { images(n); };

    // 1. Golf : la rangée, Caméléon en aperçu, refus, achat
    if (!ouvrirGarage("golf")) { s.desactiver(); std::remove(chemin.c_str()); return ecarts + 1; }
    s.etoiles = 100; images(4);
    texteTient("français");
    capture("golf-debut", "Golf : rangée « Finition » (8 boutons), cadenas et prix des 7 finitions payantes, solde 100 ★");
    verifier(r.finitionJoueur() == vf::BRILLANT, "ouverture : la Golf est en Brillant (gratuit)");
    montrer("cameleon");
    verifier(!possede("golf", "cameleon") && boutique::solde(s) == 100, "aperçu Caméléon : rien de débité, finition non possédée");
    capture("golf-cameleon-apercu-1", "aperçu Caméléon sur la Golf (angle 1) : « Acheter N ★ », équivalents");
    if (complet) tourner(200), capture("golf-cameleon-apercu-2", "aperçu Caméléon : la voiture a tourné (angle 2), la couleur change");
    if (complet) tourner(200), capture("golf-cameleon-apercu-3", "aperçu Caméléon (angle 3)");
    bouton("choisir");
    verifier(boutique::solde(s) == 100 && !possede("golf", "cameleon") && s.finition.count("golf") == 0, "« Acheter » avec 100 ★ : refus, rien de débité");
    capture("golf-il-manque", "refus : bandeau « Il te manque N ★ »");
    if (complet) {
    boutique::crediter(s, 2000, "banc WFS-491");
    const long long avant = boutique::solde(s), pCam = prix("golf", "cameleon");
    bouton("choisir");
    verifier(possede("golf", "cameleon") && s.finition["golf"] == "cameleon" && boutique::solde(s) == avant - pCam,
             "achat Caméléon : " + std::to_string(pCam) + " ★ débitées une fois, finition-golf=cameleon");
    capture("golf-cameleon", "Golf Caméléon achetée : « Choisir »");
    // 2. Giulia Mat, 500 Chromé ; Golf Candy en aperçu puis « Retour »
    verifier(voiture("giulia"), "flèche « suivante » jusqu'à la Giulia");
    montrer("mat"); capture("giulia-mat-apercu", "Giulia Mat en aperçu");
    bouton("choisir");
    verifier(possede("giulia", "mat") && s.finition["giulia"] == "mat", "Giulia Mat achetée");
    verifier(!possede("giulia", "cameleon"), "le Caméléon de la Golf ne vaut pas pour la Giulia (RG-03)");
    capture("giulia-mat", "Giulia Mat achetée");
    verifier(voiture("abarth"), "flèche « suivante » jusqu'à la 500");
    montrer("chrome"); bouton("choisir");
    verifier(possede("abarth", "chrome") && s.finition["abarth"] == "chrome", "500 Chromé achetée");
    capture("abarth-chrome", "500 Chromé achetée");
    verifier(voiture("golf"), "retour à la Golf");
    montrer("candy"); capture("golf-candy-apercu", "Golf Candy en aperçu (non achetée)");
    bouton("retour"); images(10);
    verifier(s.finition["golf"] == "cameleon" && !possede("golf", "candy"), "« Retour » sans acheter : la Golf garde Caméléon (aperçu oublié)");
    // 3. parcours RC-E18-09 : la 500 Chromé part en course par « Choisir » → terrain → « Rouler »
    if (ouvrirGarage("abarth")) {
        verifier(r.finitionJoueur() == vf::CHROME, "course préparée : la 500 est en Chromé (sauvegarde)");
        float x, y;
        if (garageBouton("choisir", x, y)) toucher(x, y);
        images(20); g.render(t);
        if (centreRouler(x, y)) toucher(x, y); else { WFS_ERR("garage-finitions : « Rouler » absent (écran %s)", nomEcran(g.ecran())); ecarts++; }
        for (int i = 0; i < 120 && g.ecran() != Ecran::Course; i++) image();
        g.setAutopilot(true); images(300); g.setAutopilot(false);
        verifier(g.ecran() == Ecran::Course && r.finitionJoueur() == vf::CHROME, "« Choisir » → « Rouler » : la 500 roule en Chromé");
        capture("parcours-abarth-course", "parcours : la 500 Chromé en course après « Choisir » et « Rouler »");
        g.allerA(Ecran::Garage); images(30);
        verifier(garageVoitureAffichee() && std::strcmp(garageVoitureAffichee(), "abarth") == 0, "retour au garage : la 500, toujours Chromé");
        capture("parcours-abarth-retour-garage", "retour au garage après la course : 500 Chromé");
    }
    // 4. relance du jeu : la sauvegarde est relue
    fermer();
    const long long solde = boutique::solde(s);
    s.attendre(); s.charger(chemin);
    verifier(s.finition["golf"] == "cameleon" && s.finition["giulia"] == "mat" && s.finition["abarth"] == "chrome" && boutique::solde(s) == solde &&
             possede("golf", "cameleon") && !possede("abarth", "cameleon"), "relance : finitions, achats et solde gardés");
    if (ouvrirGarage("golf")) {
        verifier(r.finitionJoueur() == vf::CAMELEON, "relance : la Golf repart en Caméléon");
        capture("relance-golf", "après relance : la Golf est Caméléon (pas Candy)");
    }
    fermer();
    // 5. course de jour et de nuit
    const struct { const char* id; int fin; } COURSES[] = {{"golf", vf::CAMELEON}, {"giulia", vf::MAT}, {"abarth", vf::CHROME}};
    for (const char* moment : {"jour", "nuit"}) {
        setenv("WFS_MOMENT", moment, 1);
        for (const auto& c : COURSES) {
            if (!g.init(r, c.id, "ville")) { WFS_ERR("garage-finitions : course %s impossible", c.id); ecarts++; continue; }
            ouvert = true; g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
            for (int i = 0; i < 300; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 6 == 0) g.render(t); }
            verifier(r.finitionJoueur() == c.fin, std::string("course ") + c.id + " (" + moment + ") : finition achetée " + vf::IDS[c.fin]);
            capture(std::string("course-") + c.id + "-" + moment, "course avec la finition achetée (caméra de poursuite, Ville)");
            g.setAutopilot(false); fermer();
        }
    }
    unsetenv("WFS_MOMENT");
    }
    // 6. en anglais
    ui::langue::choisir(ui::langue::Choix::Anglais);
    if (ouvrirGarage("golf")) {
        s.etoiles = 40; images(4);
        texteTient("English");
        capture("en-golf", "English: Golf, « Finish » row, locks and prices");
        montrer("nacree");
        capture("en-golf-nacre-apercu", "English: Pearl preview, « Buy N ★ »");
        bouton("choisir");
        verifier(boutique::solde(s) == 40 && !possede("golf", "nacree"), "English: not enough stars, nothing charged");
        capture("en-il-manque", "English: « You need N more ★ »");
    }
    fermer();
    ui::langue::choisir(ui::langue::Choix::Automatique);
    s.attendre(); s.desactiver(); std::remove(chemin.c_str());
    WFS_LOG("garage-finitions (WFS-491) : %d écart(s)", ecarts);
    return ecarts;
}

} // namespace wfs::ecrans
