// WaForSpeed — banc d'essai de WFS-496 (WFS_BANC=atelier) : l'atelier d'autocollants, par de vrais touchers sur les zones
// dessinées, avec une vraie sauvegarde (fichier du banc, effacé au début et à la fin) :
//   1. garage de la 208 : bouton « Atelier » sous « Market » ; atelier ouvert (bibliothèque, 10 gratuits, cadenas) ;
//   2. 5 calques (RC-E18-09) : bande, numéro déplacé au doigt, flamme tournée et agrandie à deux doigts (capture pendant le geste),
//      éclair sur le flanc droit, bande sur le capot ; ± 5°, ± 5 %, miroir, couleur, masquer, monter, annuler / refaire ;
//      motif d'un pack non possédé : message ; « Valider » : gardé et cuit ;
//   3. 20 calques au plus (« 20 calques au maximum »), « Quitter » (ou la touche Retour du téléphone) → confirmation → rien de gardé ;
//   4. parcours : « Choisir » → terrain → « Rouler » → course de jour avec les autocollants → retour au garage ;
//   5. 500 et GT86 habillées dans l'atelier (3 voitures) ; relance du jeu (sauvegarde relue) : habillages gardés ;
//   6. course de jour et de nuit avec les 3 voitures ;
//   7. en anglais : atelier, confirmation.
// Hors 1280x720 : étapes 1, 2 (sans le parcours) et 7, captures atelier-<L>x<H>-*. Chaque contrôle au journal
// (« atelier (WFS-496) : OK / ÉCART ») ; rend le nombre d'écarts.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../../core/log.h"
#include "../../game/game.h"
#include "../../game/sauvegarde.h"
#include "../../platform/retour_systeme.h"   // touche Retour du téléphone
#include "../../render/renderer.h"
#include "../../render/vehicule_skin.h"
#include "../garage.h"
#include "../langue.h"
#include "../text.h"
#include "atelier.h"
#include "atelier_calques.h"
#include "terrain.h"   // « Rouler »

namespace wfs::ecrans::atelier {

int benchAtelier(Game& g, Renderer& r, const std::string& out) {
    ui::chargerPolice(r);
    int ecarts = 0;
    auto verifier = [&](bool ok, const std::string& quoi) { WFS_LOG("atelier (WFS-496) : %s : %s", ok ? "OK" : "ÉCART", quoi.c_str()); if (!ok) ecarts++; };
    const std::string chemin = out + "/atelier-sauvegarde-banc.txt";
    std::remove(chemin.c_str());
    Sauvegarde& s = sauvegarde();
    s.charger(chemin);   // partie neuve : 208, 500, GT86 gratuites, aucun pack d'autocollants
    s.profil.cree = true;   // pilote déjà créé (WFS-481) : le menu n'est pas caché par « Crée ton pilote »
    ui::langue::choisir(ui::langue::Choix::Francais);
    const bool complet = (r.width() == 1280 && r.height() == 720) || std::getenv("WFS_ATELIER_COMPLET");
    const std::string taille = complet ? "" : "-" + std::to_string(r.width()) + "x" + std::to_string(r.height());
    double t = 5000;
    bool partie = false;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    auto images = [&](int n) { for (int i = 0; i < n; i++) image(); };
    auto capture = [&](const std::string& nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/atelier" + taille + "-" + nom + ".png");
        const vehicule_skin::Etat e = vehicule_skin::etat();
        WFS_LOG("capture atelier%s-%s : %s (habillage posé : %s, %d calque(s), cuisson %.0f ms)", taille.c_str(), nom.c_str(), quoi,
                e.voiture.c_str(), e.calques, e.msCuisson);
    };
    int doigt = 300;
    auto toucher = [&](float x, float y) { const int id = doigt++; g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); images(3); };
    auto zone = [&](const char* nom) { float x, y; if (!centre(nom, x, y)) { WFS_ERR("atelier : zone %s absente", nom); ecarts++; return false; } toucher(x, y); return true; };
    auto fermerPartie = [&]() { if (partie) g.shutdown(); partie = false; };
    auto ouvrirGarage = [&](const char* id) {
        fermerPartie();
        if (!g.init(r, id, "ville")) { WFS_ERR("atelier : chargement impossible (%s)", id); return false; }
        partie = true; g.setAutopilot(false); g.camMode = 0;
        for (int i = 0; i < 30; i++) g.update(1.0 / 60);
        g.allerA(Ecran::Garage); images(45);
        return garageVoitureAffichee() && std::strcmp(garageVoitureAffichee(), id) == 0;
    };
    auto cuit = [&](const char* car, int n) { const vehicule_skin::Etat e = vehicule_skin::etat(); return e.skin == vehicule_skin::CUIT && e.voiture == car && e.calques >= n; };
    auto nCalques = [&]() { Etat e; e.charger(texteAtelier()); return (int)e.calques().size(); };
    // motif de la famille f, par son identifiant (page suivante si besoin)
    auto poserMotif = [&](int famille, const char* id) {
        zone(("famille-" + std::to_string(famille)).c_str());
        for (int page = 0; page < 3; page++) {
            const auto motifs = motifsDe((va::Famille)famille);
            for (int i = 0; i < 12; i++) {
                const int m = page * 12 + i;
                if (m < (int)motifs.size() && std::strcmp(motifs[m]->id, id) == 0) { zone(("motif-" + std::to_string(i)).c_str()); images(4); return true; }
            }
            if (!zone("page-suiv")) break;
        }
        WFS_ERR("atelier : motif %s introuvable", id); ecarts++; return false;
    };
    // glisser d'un point du plan du calque choisi (cm) à un autre, doigt posé sur le calque
    auto glisser = [&](float a0, float b0, float a1, float b1, bool capturer) {
        float x0, y0, x1, y1;
        if (!versEcran(a0, b0, x0, y0) || !versEcran(a1, b1, x1, y1)) { ecarts++; return; }
        const int id = doigt++;
        g.onTouch({id, x0, y0, true}); image();
        for (int i = 1; i <= 12; i++) { g.onTouch({id, x0 + (x1 - x0) * i / 12, y0 + (y1 - y0) * i / 12, true}); image(); }
        if (capturer) capture("glisser", "numéro en cours de déplacement au doigt : seul son contour cyan suit le doigt");
        g.onTouch({id, x1, y1, false}); images(4);
    };
    // deux doigts autour du centre du calque : rotation (degrés) et facteur d'échelle
    auto deuxDoigts = [&](float degres, float facteur, bool capturer) {
        float cx, cy;
        if (!versEcran(0, 0, cx, cy)) { ecarts++; return; }
        Etat e; e.charger(texteAtelier());
        const CalqueAtelier* ca = e.calques().empty() ? nullptr : &e.calques().back();
        if (ca && !versEcran(ca->c.x, ca->c.y, cx, cy)) { ecarts++; return; }
        const float R0 = 60 * r.height() / 720.f;
        const int a = doigt++, b = doigt++;
        auto poser = [&](float ang, float R, bool down) {
            const float c = std::cos(ang), s = std::sin(ang);
            g.onTouch({a, cx - R * c, cy + R * s, down}); g.onTouch({b, cx + R * c, cy - R * s, down});
        };
        g.onTouch({a, cx - R0, cy, true}); image(); g.onTouch({b, cx + R0, cy, true}); image();
        for (int i = 1; i <= 12; i++) { poser(degres * PI / 180.f * i / 12, R0 * (1 + (facteur - 1) * i / 12), true); image(); }
        if (capturer) capture("deux-doigts", "flamme en cours de rotation et d'agrandissement à deux doigts (contour cyan)");
        poser(degres * PI / 180.f, R0 * facteur, false); images(4);
    };

    // 1. garage de la 208, bouton « Atelier »
    if (!ouvrirGarage("p208")) { fermerPartie(); s.desactiver(); std::remove(chemin.c_str()); return ecarts + 1; }
    float bx, by;
    verifier(centre("bouton", bx, by), "garage : bouton « Atelier » dessiné (sous « Market »)");
    capture("garage-bouton", "garage de la 208 : bouton « ATELIER » sous « MARKET », tailles des boutons inchangées");
    zone("bouton"); images(10);
    verifier(ouvert() && nCalques() == 0, "« Atelier » : écran ouvert, aucun calque");
    capture("ouvert", "atelier ouvert : vue gauche, bibliothèque (Bandes : 2 gratuites, 3 avec cadenas et prix), liste vide");
    // 2. cinq calques
    poserMotif(0, "bande-large"); images(10);
    zone("outil-3"); zone("outil-3"); zone("outil-3");   // +15 %
    poserMotif(1, "chiffre-1"); images(10);
    verifier(nCalques() == 2, "bande et numéro posés sur le flanc gauche");
    glisser(0, 0, 60, 25, true); images(10);
    { Etat e; e.charger(texteAtelier()); const auto& c = e.calques().back().c;
      verifier(std::abs(c.x - 60) <= 4 && std::abs(c.y - 25) <= 4, "glisser : numéro posé à (" + std::to_string(c.x) + ", " + std::to_string(c.y) + ") cm (attendu 60, 25)"); }
    zone("outil-5"); images(4); zone("couleur-5"); images(6);   // orange
    poserMotif(2, "flamme-1"); images(10);
    glisser(0, 0, -95, -18, false); images(6);
    deuxDoigts(20, 1.4f, true); images(10);
    { Etat e; e.charger(texteAtelier()); const auto& c = e.calques().back().c;
      verifier(std::abs(c.rotation - 20) <= 3 && std::abs(c.echelle - 140) <= 6, "deux doigts : flamme tournée de " + std::to_string(c.rotation) + "° et à " + std::to_string(c.echelle) + " % (attendu 20°, 140 %)"); }
    zone("outil-1"); zone("outil-1");   // +10°
    capture("calques", "3 calques sur le flanc gauche : bande, numéro orange déplacé, flamme tournée et agrandie (choisie, contour cyan)");
    zone("vue-1"); images(10);
    poserMotif(3, "eclair-1"); images(10);
    zone("outil-4"); images(10);   // miroir
    capture("vue-droite", "vue droite : éclair posé sur le flanc droit, en miroir (copie sur le flanc gauche)");
    zone("vue-4"); images(10);
    poserMotif(0, "bande-double"); images(10);
    zone("outil-5"); images(4); zone("couleur-0"); images(6);   // blanc
    verifier(nCalques() == 5, "5 calques (RC-E18-09)");
    capture("vue-dessus", "vue de dessus : bande double blanche sur le capot ; liste des 5 calques");
    // verrou (RG-02) : motif d'un pack non possédé
    zone("famille-1"); images(2);
    { const auto motifs = motifsDe(va::Famille::Chiffre); int i = 0; while (i < (int)motifs.size() && motifGratuit(motifs[i]->id)) i++;
      zone(("motif-" + std::to_string(i)).c_str()); images(2); }
    verifier(nCalques() == 5 && message().find("Numéros et lettres") != std::string::npos, "motif du pack « Numéros et lettres » non possédé : message, rien posé");
    capture("verrou", "numéro d'un pack non possédé touché : message « Autocollant du pack « Numéros et lettres » »");
    // liste : masquer, monter, annuler / refaire
    zone("oeil-1"); images(6);
    capture("liste-masque", "liste des calques : 2e calque masqué (« Afficher »), contour seul, rien cuit pour lui");
    zone("oeil-1"); images(6);
    zone("ligne-0"); zone("descendre"); images(4);
    const std::string avantAnnuler = texteAtelier();
    zone("annuler"); images(4);
    verifier(texteAtelier() != avantAnnuler, "« Annuler » : retour en arrière d'un pas");
    zone("refaire"); images(4);
    verifier(texteAtelier() == avantAnnuler, "« Refaire » : le pas annulé revient");
    zone("vue-0"); images(6);
    const std::string cinq = texteAtelier();
    zone("valider"); images(20);
    verifier(!ouvert() && texteGarde("p208") == cinq && cuit("p208", 5), "« Valider » : 5 calques gardés (sauvegarde) et cuits sur la 208 du garage");
    capture("garage-apres", "retour au garage : la 208 porte ses 5 autocollants");

    // 3. 20 calques au plus, « Quitter » sans valider
    zone("bouton"); images(10);
    for (int i = 0; i < 15; i++) { zone("famille-4"); zone("motif-0"); }
    verifier(nCalques() == 20, "20 calques");
    zone("famille-4"); zone("motif-0"); images(2);
    verifier(nCalques() == 20 && message() == ui::tr("atelier.max"), "21e calque refusé : « 20 calques au maximum »");
    capture("max", "21e autocollant : message « 20 calques au maximum », liste de 20");
    zone("quitter"); images(4);
    capture("confirmation", "« Quitter » après des changements : « Quitter sans valider ? »");
    zone("conf-rester"); images(2);
    verifier(ouvert() && nCalques() == 20, "« Rester » : l'atelier reste ouvert, rien perdu");
    retourSysteme(g, true, 0); image(); retourSysteme(g, false, 0); images(4);   // touche Retour d'Android = « Quitter »
    float cx, cy;
    verifier(ouvert() && g.ecran() == Ecran::Garage && centre("conf-rester", cx, cy), "touche Retour du téléphone : confirmation « Quitter sans valider ? » (rien perdu)");
    retourSysteme(g, true, 0); image(); retourSysteme(g, false, 0); images(4);   // dans la confirmation : « Rester »
    verifier(ouvert() && nCalques() == 20 && !centre("conf-rester", cx, cy), "touche Retour dans la confirmation : « Rester »");
    zone("quitter"); zone("conf-quitter"); images(10);
    verifier(!ouvert() && texteGarde("p208") == cinq, "« Quitter » sans valider : les 5 calques gardés ne changent pas");
    // garage → « Retour » → menu : la voiture de la course reprend son habillage (texture de l'aperçu rendue)
    {
        float x, y;
        if (garageBouton("retour", x, y)) toucher(x, y);
        images(40);
        const vehicule_skin::Etat e = vehicule_skin::etat();
        verifier(g.ecran() == Ecran::Menu && reposeCourse() == "p208" && e.skin == vehicule_skin::CUIT && e.voiture == "p208" && e.calques >= 5 && e.textures == 1,
                 "garage → « Retour » → menu : aperçu rendu, habillage de la course reposé (5 calques, 1 texture en mémoire)");
        capture("menu-retour", "« Retour » au menu : la 208 du parking garde ses autocollants");
        g.allerA(Ecran::Garage); images(45);
    }

    if (complet) {
        // 4. parcours : « Choisir » → terrain → « Rouler » → course
        s.tod = "jour"; s.moment = "midi";   // le terrain s'ouvre sur « Jour » (moment de la sauvegarde)
        float x, y;
        if (garageBouton("choisir", x, y)) toucher(x, y);
        images(20); g.render(t);
        if (ecrans::centreRouler(x, y)) toucher(x, y); else { WFS_ERR("atelier : « Rouler » absent (écran %s)", nomEcran(g.ecran())); ecarts++; }
        for (int i = 0; i < 120 && g.ecran() != Ecran::Course; i++) image();
        g.setAutopilot(true); images(300); g.setAutopilot(false);
        verifier(s.tod == "jour", "parcours : course de jour (moment demandé par « Rouler », écrit dans la sauvegarde : " + s.tod + ")");   // Game : sauvegarde().tod = nuit ? « nuit » : « jour »
        verifier(g.ecran() == Ecran::Course && cuit("p208", 5), "« Choisir » → « Rouler » : la 208 roule avec ses 5 autocollants");
        capture("parcours-course", "parcours : la 208 en course de jour avec ses autocollants");
        g.allerA(Ecran::Garage); images(30);
        verifier(cuit("p208", 5), "retour au garage : autocollants toujours là");
        // 5. 500 et GT86 habillées
        const struct { const char* id; } AUTRES[] = {{"abarth"}, {"gt86"}};
        for (const auto& v : AUTRES) {
            if (!ouvrirGarage(v.id)) { ecarts++; continue; }
            zone("bouton"); images(10);
            poserMotif(4, "etoile-1"); zone("outil-3"); zone("outil-3");
            poserMotif(5, "lettre-a"); glisser(0, 0, -40, 10, false);
            zone("vue-4"); images(6); poserMotif(0, "bande-double"); zone("outil-5"); zone("couleur-0");   // bande double blanche sur le toit (vue en course)
            zone("vue-3"); images(6); poserMotif(1, "chiffre-2"); zone("outil-3"); zone("outil-3"); zone("outil-5"); zone("couleur-5");   // numéro orange à l'arrière (vu en course)
            zone("vue-0"); images(6);
            zone("valider"); images(20);
            verifier(cuit(v.id, 4) && texteGarde(v.id).find("bande-double,t,") != std::string::npos, std::string(v.id) + " : 4 calques gardés et cuits, bande double sur le toit");
            capture(std::string("garage-") + v.id, "garage : étoile, lettre A, bande double blanche sur le toit, numéro 2 orange à l'arrière");
        }
        // relance
        fermerPartie();
        s.attendre(); s.charger(chemin);
        verifier(texteGarde("p208") == cinq && !texteGarde("abarth").empty() && !texteGarde("gt86").empty(), "relance : habillages des 3 voitures relus");
        if (ouvrirGarage("p208")) {
            verifier(cuit("p208", 5), "relance : la 208 du garage porte ses autocollants");
            capture("relance-garage", "après relance : la 208 au garage avec ses autocollants");
            zone("bouton"); images(10);
            verifier(nCalques() == 5, "relance : l'atelier rouvre avec les 5 calques");
            zone("quitter"); images(6);
        }
        fermerPartie();
        // 6. course de jour et de nuit, 3 voitures
        for (const char* moment : {"jour", "nuit"}) {
            setenv("WFS_MOMENT", moment, 1);
            for (const char* id : {"p208", "abarth", "gt86"}) {
                if (!g.init(r, id, "ville")) { WFS_ERR("atelier : course %s impossible", id); ecarts++; continue; }
                partie = true; g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
                for (int i = 0; i < 300; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 6 == 0) g.render(t); }
                verifier(cuit(id, 3), std::string("course ") + id + " (" + moment + ") : autocollants cuits");
                capture(std::string("course-") + id + "-" + moment, "course avec les autocollants (caméra de poursuite, Ville)");
                g.setAutopilot(false); fermerPartie();
            }
        }
        unsetenv("WFS_MOMENT");
    }
    // 7. en anglais
    ui::langue::choisir(ui::langue::Choix::Anglais);
    if (ouvrirGarage("p208")) {
        capture("en-garage-bouton", "English: « WORKSHOP » button under « MARKET »");
        zone("bouton"); images(10);
        zone("ligne-0"); images(4);
        capture("en-atelier", "English: decal workshop, library, layers, tools");
        zone("outil-6"); images(6);   // supprimer : changement
        zone("quitter"); images(4);
        capture("en-confirmation", "English: « Leave without applying? »");
        zone("conf-quitter"); images(6);
        verifier(texteGarde("p208") == cinq, "English: « Exit » without applying keeps the saved decals");
    }
    fermerPartie();
    ui::langue::choisir(ui::langue::Choix::Automatique);
    s.attendre(); s.desactiver(); std::remove(chemin.c_str());
    WFS_LOG("atelier (WFS-496) : %d écart(s)", ecarts);
    return ecarts;
}

} // namespace wfs::ecrans::atelier
