// WaForSpeed — banc « textes » du garage et de l'écran des paquets de cartes (WFS-507, épic E20), dans la langue de WFS_LANGUE
// (téléphone simulé, réglage Automatique) :
//   - garage : fiche de la Golf (type, accroche, Puissance / 0 à 100 / Vitesse max, notes, Couleur, Choisir, Retour, crédit), puis
//     fiche de la voiture la plus longue à écrire (SF90), puis parcours accueil → garage → « Choisir » → terrain → « Rouler » →
//     course → « Menu » → pause → « Quitter la course » → accueil ;
//   - paquets : le banc complet de WFS-306 / 307 (proposition, téléchargement, coupure, place, échec, bandeau du menu, vignettes du
//     choix du terrain : Télécharger, sans Internet, données mobiles, progression, Supprimer), textes notés, captures renommées
//     textes-<fr|en>-paquets-*.
// Chaque capture note ses textes au journal de WFS-504 ; à la fin, le garde-fou vérifie qu'aucune phrase française ni aucun
// texte qui dépasse de son bouton n'apparaît en anglais (rien n'est vérifié en français). Rend le nombre d'échecs.
#include <cstring>
#include <filesystem>
#include "../core/log.h"
#include "../game/course_system.h"
#include "../game/game.h"
#include "../render/renderer.h"
#include "ecrans/pause.h"
#include "ecrans/paquets.h"
#include "ecrans/terrain.h"
#include "ecrans/textes_banc.h"
#include "garage.h"
#include "garage_commun.h"
#include "langue.h"
#include "text.h"

namespace wfs::ecrans {
using namespace garage;

int benchGarageTextes(Game& g, Renderer& r, const std::string& out) {
    const std::string lg = ui::langue::code();
    int echecs = 0;
    auto etape = [&](bool ok, const char* quoi) {
        if (!ok) echecs++;
        WFS_LOG("textes (WFS-507) : garage %s : %s : %s", lg.c_str(), quoi, ok ? "OK" : "ÉCHEC");
    };
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("textes (WFS-507) : chargement impossible"); courseBenchRules(false); return 1; }
    g.setAutopilot(false); g.camMode = 0;
    double t = 2500;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    auto attendre = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 9) g.render(t); } };
    auto capture = [&](const std::string& nom, const char* quoi) {   // textes des deux dernières images notés, puis l'image
        ui::langue::journalActif(true, nom.c_str());
        g.render(t += 0.01); g.render(t += 0.01);
        ui::langue::journalActif(false);
        r.capturePng(out + "/textes-" + lg + "-" + nom + ".png");
        WFS_LOG("capture textes-%s-%s : écran %s, %s", lg.c_str(), nom.c_str(), nomEcran(g.ecran()), quoi);
    };
    auto toucher = [&](int id, float x, float y) { attendre(24); g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); image(); };
    float x = 0, y = 0;
    ui::langue::viderJournal();

    // ---------- accueil → garage : fiche de la Golf ----------
    for (int i = 0; i < 30; i++) g.update(1.0 / 60);
    g.allerA(Ecran::Menu); for (int i = 0; i < 30; i++) image();
    g.allerA(Ecran::Garage);
    image();
    capture("garage-chargement", "garage à l'ouverture (« Chargement… » tant que le modèle n'est pas prêt)");
    for (int i = 0; i < 90; i++) image();
    etape(garageEnCours() != nullptr, "accueil → garage");
    capture("garage-fiche", "fiche de la Golf : type, accroche, caractéristiques, notes, couleur, Choisir, Retour, crédit");
    // ---------- voiture au texte le plus long (Hypercar hybride, accroche longue) ----------
    for (int k = 0; k < CAR_COUNT && garageEnCours() && std::strcmp(CARS[garageEnCours()->index()].id, "sf90") != 0 && garageBouton("suivant", x, y); k++) {
        toucher(30 + k, x, y);
        for (int i = 0; i < 45; i++) image();
    }
    if (garageEnCours() && !std::strcmp(CARS[garageEnCours()->index()].id, "sf90")) {
        garageEnCours()->choisirPeinture(peintureParId("rouge")); for (int i = 0; i < 12; i++) image();
        capture("garage-fiche-sf90", "fiche de la SF90 (texte le plus long), peinture Rouge");
    } else WFS_LOG("textes (WFS-507) : SF90 absente du garage (voiture retirée) : capture de la dernière voiture vue");
    // ---------- « Choisir » → terrain → « Rouler » → course → pause → accueil ----------
    etape(garageBouton("choisir", x, y), "garage : « Choisir » dessiné");
    toucher(60, x, y);
    attendre(30);
    etape(g.ecran() == Ecran::Terrain, "garage « Choisir » → choix du terrain");
    etape(centreRouler(x, y), "choix du terrain : « Rouler » dessiné");
    toucher(61, x, y);
    for (int i = 0; i < 60 * 5 && g.ecran() != Ecran::Course; i++) image();
    etape(g.ecran() == Ecran::Course, "« Rouler » → course avec la voiture choisie");
    attendre(60 * 3); image();
    capture("garage-course", "course avec la voiture choisie au garage");
    etape(pauseBouton("menu", x, y), "course : bouton « Menu » dessiné");
    g.onTouch({62, x, y, true}); image(); g.onTouch({62, x, y, false}); image();
    etape(g.ecran() == Ecran::Pause && pauseBouton("quitter", x, y), "« Menu » → pause, « Quitter la course » dessiné");
    toucher(63, x, y);
    for (int i = 0; i < 30; i++) image();
    etape(g.ecran() == Ecran::Menu, "« Quitter la course » → accueil (retour)");
    g.shutdown();
    echecs += ui::langue::verifierJournal();
    WFS_LOG("textes (WFS-507) : garage %s : %d échec(s) → %s", lg.c_str(), echecs, echecs ? "ROUGE" : "VERT");
    courseBenchRules(false);
    return echecs;
}

int benchPaquetsTextes(Game& g, Renderer& r, const std::string& out, const std::string& racine) {
    namespace fs = std::filesystem;
    const std::string lg = ui::langue::code();
    ui::langue::viderJournal();
    textes::ecranNote() = "paquets";   // chaque écran dessiné (étape, bandeau, vignettes du terrain, menu) note ses textes
    const int ecarts = benchPaquets(g, r, out, racine);
    textes::ecranNote().clear();
    const int phrases = ui::langue::verifierJournal();
    std::error_code ec;
    for (const auto& f : fs::directory_iterator(out, ec)) {   // paquets-*.png → textes-<langue>-paquets-*.png
        const std::string n = f.path().filename().string();
        if (n.rfind("paquets-", 0) == 0 && f.path().extension() == ".png") fs::rename(f.path(), f.path().parent_path() / ("textes-" + lg + "-" + n), ec);
    }
    WFS_LOG("textes (WFS-507) : paquets %s : %d écart(s) du banc, %d échec(s) des textes → %s", lg.c_str(), ecarts, phrases, ecarts + phrases ? "ROUGE" : "VERT");
    return ecarts + phrases;
}

} // namespace wfs::ecrans
