// WaForSpeed — banc d'essai de WFS-499 (WFS_BANC=garage-voitures) : parcours RG-07 avec une vraie sauvegarde (fichier du banc,
// effacé au début et à la fin), par de vrais touchers sur les zones dessinées :
//   1. garage de la 208 (gratuite), solde 1 000 ★ ; flèche « suivante » ; garage de la M4 (2 200 ★) : cadenas, prix, médailles, euros, solde ;
//   2. « Acheter » : fenêtre, « Il te manque 1 200 ★ », pas de Confirmer, rien de débité ; Annuler ;
//   3. « Essai gratuit » : terrain puis course solo avec la M4 ; essai noté ; écran de fin : « Acheter 2 200 ★ » ;
//   4. « Rejouer » : garage (garde-fou), « Essai déjà fait » ; duel (salon en mémoire) avec la M4 bloquée : pas de course, salon quitté ;
//   5. solde porté à 3 000 ★ : « Acheter » → fenêtre de confirmation → Confirmer : 800 ★, M4 débloquée et choisie ;
//      peinture gratuite Rouge sur la M4, « Choisir » ;
//   6. Corsa (250 ★) : essai, écran de fin, « Acheter » → Confirmer depuis l'écran de fin ;
//   7. course avec la M4 achetée, de jour et de nuit ; relance du jeu (sauvegarde relue) : M4 et Corsa gardées, Corsa choisie ;
//   8. en anglais : Golf bloquée, fenêtre « You need N more ★ », fenêtre « Confirm », écran de fin de l'essai ; « Menu » :
//      la voiture de la partie redevient celle de la sauvegarde, la course suivante part avec elle.
// Format 4:3 (WFS_TAILLE=1024x768) : captures préfixées « 4x3- ». Chaque contrôle au journal (« garage-voitures (WFS-499) : OK / ÉCART ») ;
// rend le nombre d'écarts.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include "../core/log.h"
#include "../game/boutique.h"
#include "../game/course.h"
#include "../game/course_system.h"
#include "../game/duel.h"
#include "../game/duel_system.h"
#include "../game/player.h"
#include "../game/game.h"
#include "../render/renderer.h"
#include "garage.h"
#include "garage_achat.h"
#include "garage_commun.h"
#include "langue.h"
#include "text.h"

namespace wfs::ecrans {
using namespace garage;

int benchGarageVoitures(Game& g, Renderer& r, const std::string& out) {
    ui::chargerPolice(r);
    int ecarts = 0;
    auto verifier = [&](bool ok, const std::string& quoi) { WFS_LOG("garage-voitures (WFS-499) : %s : %s", ok ? "OK" : "ÉCART", quoi.c_str()); if (!ok) ecarts++; };
    const std::string chemin = out + "/garage-voitures-sauvegarde-banc.txt";
    std::remove(chemin.c_str());
    Sauvegarde& s = sauvegarde();
    s.charger(chemin);
    const bool regles = courseBenchRulesOn();
    courseBenchRules(true);   // vraie course : compte à rebours, chrono, écran de fin
    ui::langue::choisir(ui::langue::Choix::Francais);
    const std::string prefixe = r.width() * 3 < r.height() * 4 + 8 ? "4x3-" : "";   // format 4:3 : noms à part
    double t = 5000;
    bool ouvert = false;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    auto images = [&](int n) { for (int i = 0; i < n; i++) image(); };
    auto capture = [&](const std::string& nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + prefixe + "garage-voitures-" + nom + ".png");
        WFS_LOG("capture %sgarage-voitures-%s : %s (solde %lld ★)", prefixe.c_str(), nom.c_str(), quoi, boutique::solde(s));
    };
    int doigt = 70;
    auto toucher = [&](float x, float y) { const int id = doigt++; g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); images(3); };
    auto bouton = [&](const char* nom) { float x, y; if (!garageBouton(nom, x, y)) return false; toucher(x, y); images(4); return true; };
    auto boutonFin = [&](const char* nom) { float x, y; if (!finEssaiBouton(nom, x, y)) return false; toucher(x, y); images(4); return true; };
    auto fermer = [&]() { if (ouvert) g.shutdown(); ouvert = false; };
    auto ouvrirGarage = [&](const char* id) {
        fermer();
        if (!g.init(r, id, "ville")) { WFS_ERR("garage-voitures : chargement impossible (%s)", id); return false; }
        ouvert = true; g.setAutopilot(false); g.camMode = 0;
        for (int i = 0; i < 30; i++) g.update(1.0 / 60);
        g.allerA(Ecran::Garage); images(45);
        return garageEnCours() != nullptr;
    };
    // essai gratuit de la voiture affichée : « Essai gratuit » → Terrain → course solo (pilote automatique) → écran de fin
    auto essai = [&](const char* id) {
        const bool ok = bouton("essai");
        images(6);
        verifier(ok && g.ecran() == Ecran::Terrain, std::string("« Essai gratuit » de la ") + id + " : écran du terrain");
        g.allerA(Ecran::Course); images(2); g.setAutopilot(true);
        for (int i = 0; i < 60 * 8; i++) g.update(1.0 / 60);   // compte à rebours puis départ
        verifier(s.essais.count(id) > 0 && g.ecran() == Ecran::Course, std::string("essai de la ") + id + " noté au départ (essai-" + id + "=1)");
        courseBenchSetTime(0.5);
        for (int i = 0; i < 60 * 3; i++) image();
        g.setAutopilot(false);
        const Course* c = activeCourse();
        verifier(c && c->state == RaceState::Over && finEssaiVisible(), std::string("fin de l'essai de la ") + id + " : « Acheter » sur l'écran de fin");
    };

    // 1. 208 gratuite, puis M4 bloquée
    if (!ouvrirGarage("p208")) { s.desactiver(); std::remove(chemin.c_str()); courseBenchRules(regles); return ecarts + 1; }
    s.etoiles = 1000; images(4);
    verifier(!voitureBloquee(s, "p208") && voitureBloquee(s, "m4"), "208 gratuite (pas de cadenas), M4 bloquée");
    capture("p208", "208 gratuite : « Choisir », cadenas sous les points des voitures bloquées, solde en haut à droite");
    const std::string avant = garageVoitureAffichee() ? garageVoitureAffichee() : "";
    bouton("suivant"); images(30);
    verifier(garageVoitureAffichee() && avant != garageVoitureAffichee(), "flèche « suivante » : voiture suivante");
    verifier(ouvrirGarage("m4") && garageVoitureAffichee() && std::strcmp(garageVoitureAffichee(), "m4") == 0, "garage ouvert sur la M4");
    s.etoiles = 1000; images(4);
    capture("m4-bloquee", "M4 bloquée : cadenas, 2 200 ★, ≈ 3 or + 1 argent + 1 bronze · ≈ 21,78 €, « Essai gratuit »");
    // 2. achat refusé faute d'étoiles
    bouton("choisir");
    float x, y;
    verifier(garageBouton("annuler", x, y) && !garageBouton("confirmer", x, y), "« Acheter » avec 1 000 ★ : fenêtre ouverte, pas de Confirmer");
    capture("m4-il-manque", "fenêtre : « Il te manque 1 200 ★ », Annuler seul");
    bouton("annuler");
    verifier(boutique::solde(s) == 1000 && !boutique::possedeVoiture(s, "m4"), "achat refusé : rien de débité, M4 toujours bloquée");
    // 3. essai gratuit de la M4
    essai("m4");
    capture("fin-essai-m4", "écran de fin de l'essai : « Essai gratuit terminé », « Acheter 2 200 ★ »");
    // 4. « Rejouer » après l'essai (la course repart sur place) : garde-fou → garage sur la M4, « Essai déjà fait »
    courseRejouer(); images(20);
    verifier(g.ecran() == Ecran::Garage, "« Rejouer » avec la M4 bloquée, essai fait : pas de 2e course, retour au garage (garde-fou)");
    images(30);
    verifier(garageVoitureAffichee() && std::strcmp(garageVoitureAffichee(), "m4") == 0 && !essaiPossible(s, "m4"), "garage après l'essai : M4 affichée, essai plus possible");
    capture("m4-essai-fait", "M4 : « Essai déjà fait »");
    bouton("essai");
    verifier(g.ecran() == Ecran::Garage, "« Essai déjà fait » : rien ne se passe");
    // duel avec la M4 bloquée (salon en mémoire, comme le banc du duel) : la course ne part pas et le salon est quitté
    {
        auto salon = std::make_shared<duel::SalonMemoire>();
        duelSession().join(std::make_unique<duel::TransportMemoire>(salon, "moi"), "K7QX", true, "Banc", "m4", "", "ville");
        verifier(duelSession().inRoom(), "duel : salon en mémoire rejoint avec la M4 bloquée");
        g.allerA(Ecran::Menu); images(4); g.allerA(Ecran::Course); images(20);
        verifier(g.ecran() == Ecran::Garage && !duelSession().inRoom(), "duel avec la M4 bloquée : pas de course, salon quitté (leave), retour au garage");
        if (duelSession().inRoom()) duelSession().leave();
        images(30);
    }
    // 5. achat confirmé
    s.etoiles = 3000;
    bouton("choisir");
    verifier(garageBouton("confirmer", x, y), "« Acheter » avec 3 000 ★ : fenêtre avec Confirmer");
    capture("m4-confirmation", "fenêtre : « Acheter la Bavaria Quattro… pour 2 200 ★ ? », Annuler / Confirmer");
    bouton("confirmer");
    verifier(boutique::solde(s) == 800 && boutique::possedeVoiture(s, "m4") && s.car == "m4" && !voitureBloquee(s, "m4"), "Confirmer : 2 200 ★ débitées une fois, M4 débloquée et choisie (car=m4)");
    bouton("choisir");   // une 2e fois : « Choisir » (aucun 2e débit)
    verifier(boutique::solde(s) == 800 && g.ecran() != Ecran::Garage, "M4 possédée : « Choisir » part en course, aucun 2e débit");
    ouvrirGarage("m4");
    const int ROUGE = peintureParId("rouge");
    if (const Zone& z = g_z.peintures[ROUGE]; z.w > 0) toucher(z.x + z.w / 2, z.y + z.h / 2);
    images(6);
    capture("m4-debloquee", "M4 achetée : plus de cadenas, peinture gratuite Rouge, « Choisir »");
    bouton("choisir");
    verifier(s.paint["m4"] == "rouge", "peinture Rouge gardée pour la M4 (paint-m4)");
    // 6. Corsa : achat depuis l'écran de fin de l'essai
    if (ouvrirGarage("corsa")) {
        essai("corsa");
        s.etoiles = 800;   // étoiles de la course d'essai remises à un compte rond
        boutonFin("acheter");
        verifier(finEssaiBouton("confirmer", x, y), "fin de l'essai, « Acheter » avec 800 ★ pour 250 ★ : fenêtre avec Confirmer");
        capture("fin-essai-confirmation", "écran de fin : fenêtre « Acheter la … pour 250 ★ ? »");
        boutonFin("confirmer");
        verifier(boutique::solde(s) == 550 && boutique::possedeVoiture(s, "corsa") && !finEssaiVisible(), "Confirmer à la fin de l'essai : 250 ★ débitées, Corsa débloquée, bouton retiré");
        capture("fin-essai-achetee", "écran de fin après l'achat : plus de bouton « Acheter »");
    }
    // 7. course avec la M4 achetée, de jour et de nuit ; relance du jeu
    for (const char* moment : {"jour", "nuit"}) {
        fermer();
        setenv("WFS_MOMENT", moment, 1);
        if (!g.init(r, "m4", "ville")) { WFS_ERR("garage-voitures : course m4 impossible"); continue; }
        ouvert = true; g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
        for (int i = 0; i < 300; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 6 == 0) g.render(t); }
        verifier(g.ecran() == Ecran::Course, std::string("course avec la M4 achetée (") + moment + ") : pas de retour au garage");
        capture(std::string("course-m4-") + moment, "course avec la M4 achetée, Rouge (caméra de poursuite, Ville)");
        g.setAutopilot(false);
    }
    unsetenv("WFS_MOMENT");
    fermer();
    const long long solde = boutique::solde(s);
    s.attendre(); s.charger(chemin);
    verifier(boutique::possedeVoiture(s, "m4") && boutique::possedeVoiture(s, "corsa") && s.car == "corsa" && boutique::solde(s) == solde && s.essais.count("m4") > 0,
             "relance : M4 et Corsa possédées, Corsa choisie (dernier achat), solde et essais gardés");
    if (ouvrirGarage(s.car.c_str())) capture("relance-corsa", "après relance : garage de la Corsa achetée (sans cadenas)");
    // 8. en anglais
    ui::langue::choisir(ui::langue::Choix::Anglais);
    if (ouvrirGarage("golf")) {
        s.etoiles = 40; images(4);
        capture("en-golf-bloquee", "English: Golf locked, 600 ★, ≈ 1 gold · ≈ €5.94, « Free test drive »");
        bouton("choisir");
        capture("en-il-manque", "English: « You need 560 more ★ », Cancel only");
        bouton("annuler");
        s.etoiles = 5000; images(2);
        bouton("choisir");
        capture("en-confirmation", "English: « Buy the … for 600 ★? », Cancel / Confirm");
        bouton("annuler");
        verifier(boutique::solde(s) == 5000 && !boutique::possedeVoiture(s, "golf"), "English: Cancel, nothing charged");
        essai("golf");
        capture("en-fin-essai", "English: end screen « Free test drive over », « Buy 600 ★ »");
        boutonFin("acheter");
        capture("en-fin-essai-confirmation", "English: end screen, « Buy the … for 600 ★? »");
        boutonFin("annuler");
        // fin de l'essai → « Menu » : la voiture de la partie redevient la voiture de la sauvegarde (Corsa), plus la Golf bloquée
        float mx, my;
        if (courseBouton("menu", mx, my)) toucher(mx, my);
        images(10);
        verifier(g.ecran() == Ecran::Menu && g.player().car && s.car == g.player().car->id,
                 std::string("fin de l'essai → Menu : voiture de la partie remise sur la ") + s.car + " (sauvegarde), plus la Golf bloquée");
        g.allerA(Ecran::Course); images(30);
        verifier(g.ecran() == Ecran::Course && g.player().car && s.car == g.player().car->id, "course suivante depuis le menu : avec la voiture possédée, pas de retour au garage");
        ouvrirGarage("golf");
        capture("en-golf-essai-fait", "English: « Test drive used »");
    }
    fermer();
    ui::langue::choisir(ui::langue::Choix::Automatique);
    courseBenchRules(regles);
    s.attendre(); s.desactiver(); std::remove(chemin.c_str());
    WFS_LOG("garage-voitures (WFS-499) : %d écart(s)", ecarts);
    return ecarts;
}

} // namespace wfs::ecrans
