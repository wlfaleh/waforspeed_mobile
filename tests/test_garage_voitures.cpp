// WFS-499 : voitures bloquées du garage et prix en texte, français et anglais (src/ui/garage_achat_prix.cpp, garage_achat_texte.cpp).
// Sans sauvegarde active : aucune voiture bloquée (bancs des autres tickets inchangés) ; sauvegarde active : 208, 500 et GT86 gratuites,
// M4 bloquée puis achetée (2 200 ★ débitées une fois) ; achat refusé faute d'étoiles ; essai une seule fois ; voiture retirée jamais
// bloquée ni vendue ; « 2 200 ★ » / « 2,200 ★ » ; « 3 or + 1 argent + 1 bronze » / « 3 gold + 1 silver + 1 bronze » ;
// « ≈ … · ≈ 21,78 € » / « ≈ … · ≈ €21.78 ».
// usage : test_garage_voitures [dossier temporaire]
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include "../src/game/boutique.h"
#include "../src/game/pret.h"
#include "../src/ui/garage_achat.h"
#include "../src/ui/langue.h"

namespace wfs {   // tables de langue (assets/textes) lues dans le dépôt
bool readAsset(const std::string& chemin, std::vector<uint8_t>& out) {
    std::ifstream f(std::string(WFS_SOURCE_DIR) + "/assets/" + chemin, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), {});
    return true;
}
namespace ui {   // langue.cpp ne s'en sert que pour le journal du banc « textes » : capitales ASCII suffisent
std::string majuscules(const std::string& s) { std::string m = s; for (char& c : m) if (c >= 'a' && c <= 'z') c = (char)(c - 32); return m; }
}
}
using namespace wfs;
using namespace wfs::ecrans::garage;

static int g_echecs = 0;
static void cas(const char* nom, bool ok) {
    std::printf("%s %s\n", ok ? "OK   " : "ÉCHEC", nom);
    if (!ok) g_echecs++;
}

int main(int argc, char** argv) {
    const std::string dossier = argc > 1 ? argv[1] : std::filesystem::temp_directory_path().string();
    const std::string chemin = dossier + "/test_garage_voitures.txt";
    std::filesystem::remove(chemin);
    {
        Sauvegarde inactive;
        cas("sans sauvegarde active : aucune voiture bloquée (M4, SF90)", !voitureBloquee(inactive, "m4") && !voitureBloquee(inactive, "sf90"));
        cas("identifiant nul : pas bloquée", !voitureBloquee(inactive, nullptr));
    }
    Sauvegarde s;
    s.charger(chemin);
    cas("sauvegarde du test active", s.actif());
    cas("RG-01 : 208, 500 et GT86 gratuites : pas de cadenas", !voitureBloquee(s, "p208") && !voitureBloquee(s, "abarth") && !voitureBloquee(s, "gt86"));
    cas("RG-01 : M4, Corsa et SF90 bloquées", voitureBloquee(s, "m4") && voitureBloquee(s, "corsa") && voitureBloquee(s, "sf90"));
    cas("voitures retirées (x5, a45, elantra) : jamais bloquées ni vendues", !voitureBloquee(s, "x5") && !voitureBloquee(s, "a45") && !voitureBloquee(s, "elantra"));
    s.etoiles = 1000;
    cas("RG-02 : solde insuffisant : rien de débité, M4 toujours bloquée",
        boutique::acheterVoiture(s, "m4") == boutique::AchatVoiture::SoldeInsuffisant && s.etoiles == 1000 && voitureBloquee(s, "m4"));
    cas("RG-03 : essai gratuit de la M4 une seule fois", boutique::marquerEssai(s, "m4") && !boutique::marquerEssai(s, "m4") && voitureBloquee(s, "m4"));
    s.etoiles = 3000;
    cas("RG-02 : achat de la M4 : 2 200 ★ débitées une fois, cadenas retiré",
        boutique::acheterVoiture(s, "m4") == boutique::AchatVoiture::Achetee && s.etoiles == 800 && !voitureBloquee(s, "m4"));
    cas("RG-02 : 2e achat de la M4 : rien de débité", boutique::acheterVoiture(s, "m4") == boutique::AchatVoiture::DejaPossedee && s.etoiles == 800);
    s.enregistrer(); s.attendre();
    {
        Sauvegarde relue; relue.charger(chemin); relue.attendre();
        cas("RG-07 : relance : M4 achetée gardée, solde gardé", !voitureBloquee(relue, "m4") && relue.etoiles == 800 && relue.essais.count("m4") == 1);
        relue.desactiver();
    }
    // RG-05 : textes en français
    ui::langue::choisir(ui::langue::Choix::Francais);
    cas("FR : 2 200 (espace insécable)", nombreLangue(2200) == "2\xC2\xA0" "200" && nombreLangue(24350) == "24\xC2\xA0" "350" && nombreLangue(250) == "250");
    cas("FR : 3 or + 1 argent + 1 bronze", texteMedailles(2200) == "3 or + 1 argent + 1 bronze");
    cas("FR : 21,78 €", texteEuros(2200) == "21,78 €");
    cas("FR : ≈ 3 or + 1 argent + 1 bronze · ≈ 21,78 €", texteEquivalentVoiture(2200) == "≈ 3 or + 1 argent + 1 bronze · ≈ 21,78 €");
    cas("FR : 250 ★ → 1 argent", texteMedailles(250) == "1 argent");
    cas("FR : question de la fenêtre", questionAchat("Draven Vantor GT", 2200) == "Acheter la Draven Vantor GT pour 2\xC2\xA0" "200 ★ ?");
    cas("FR : élision devant une voyelle (« l'Orion Corsaire »)", questionAchat("Orion Corsaire", 250) == "Acheter l'Orion Corsaire pour 250 ★ ?");
    cas("FR : élision devant une voyelle accentuée (« l'Étoile »)", questionAchat("Étoile X", 600) == "Acheter l'Étoile X pour 600 ★ ?");
    cas("FR : pas d'élision devant une consonne (« la Norvik Lume »)", questionAchat("Norvik Lume", 600) == "Acheter la Norvik Lume pour 600 ★ ?");
    cas("FR : boutons", ui::tr("garage.confirmer") == "Confirmer" && ui::tr("garage.annuler") == "Annuler" && ui::tr("garage.essai") == "Essai gratuit" && ui::tr("garage.essai_fait") == "Essai déjà fait");
    // RG-05 : textes en anglais
    ui::langue::choisir(ui::langue::Choix::Anglais);
    cas("EN : 2,200", nombreLangue(2200) == "2,200" && nombreLangue(1000000) == "1,000,000");
    cas("EN : 3 gold + 1 silver + 1 bronze", texteMedailles(2200) == "3 gold + 1 silver + 1 bronze");
    cas("EN : €21.78", texteEuros(2200) == "€21.78");
    cas("EN : ≈ 3 gold + 1 silver + 1 bronze · ≈ €21.78", texteEquivalentVoiture(2200) == "≈ 3 gold + 1 silver + 1 bronze · ≈ €21.78");
    cas("EN : question de la fenêtre (« the », voyelle ou non)", questionAchat("Orion Corsaire", 250) == "Buy the Orion Corsaire for 250 ★?" && questionAchat("Draven Vantor GT", 2200) == "Buy the Draven Vantor GT for 2,200 ★?");
    cas("EN : You need 1,200 more ★", ui::tr("garage.manque", nombreLangue(1200)) == "You need 1,200 more ★");
    cas("EN : Buy / Confirm / Cancel / Free test drive", ui::tr("garage.acheter", "2,200") == "Buy 2,200 ★" && ui::tr("garage.confirmer") == "Confirm" &&
        ui::tr("garage.annuler") == "Cancel" && ui::tr("garage.essai") == "Free test drive");
    cas("EN : WFS-502 : durée du prêt « 2 h 41 min », « 59 min », « Keep it forever: 2,400 ★ »", texteDuree(2 * 3600 + 40 * 60 + 1) == "2 h 41 min" &&
        texteDuree(59 * 60) == "59 min" && ui::tr("pret.garder", nombreLangue(2400)) == "Keep it forever: 2,400 ★");
    ui::langue::choisir(ui::langue::Choix::Francais);
    cas("FR : WFS-502 : « Prêtée : 2 h 05 min », minutes arrondies au-dessus (1 s → 1 min)", ui::tr("pret.garage.restant", texteDuree(2 * 3600 + 5 * 60)) == "Prêtée : 2 h 05 min" &&
        texteDuree(1) == "1 min" && texteDuree(0) == "0 min");
    // WFS-502 (RG-02) : voiture prêtée, prêt actif : pas de cadenas ; en attente ou finie : bloquée
    pret::recevoir(s.cadeau, "corsa", 1794600000);
    cas("WFS-502 : prêt en attente : Corsa toujours bloquée", voitureBloquee(s, "corsa"));
    pret::activer(s, 1794600000);
    cas("WFS-502 : prêt actif : Corsa choisissable (pas de cadenas), jamais achetée, essai gratuit pas utilisé",
        !voitureBloquee(s, "corsa") && !boutique::possedeVoiture(s, "corsa") && s.essais.count("corsa") == 0);
    pret::mettreAJour(s, 1794600000 + 3 * 3600 + 1);
    cas("WFS-502 : fin du prêt : Corsa de nouveau bloquée", voitureBloquee(s, "corsa"));
    ui::langue::choisir(ui::langue::Choix::Automatique);
    s.desactiver();
    std::filesystem::remove(chemin);
    std::printf("%s : %d échec(s)\n", g_echecs ? "ÉCHEC" : "OK", g_echecs);
    return g_echecs ? 1 : 0;
}
