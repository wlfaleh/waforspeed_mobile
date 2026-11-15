// WFS-494 (RG-01 à RG-05) : skins du garage : ordre affiché, voitures qui ont des skins, prix (base × coefficient de la classe),
// Néon nuit jamais en vente, skin offerte (une par voiture possédée, gardée dans la sauvegarde), donnerSkin (cadeau du jour,
// succès), bon de skin du market, ce qui manque pour partir en course (skin comprise), refus sans débit, skin pour une seule voiture.
#include <cstdio>
#include <cstring>
#include <string>
#include "../src/game/boutique.h"
#include "../src/game/cars.h"
#include "../src/game/market.h"
#include "../src/game/skin_offerte.h"
#include "../src/ui/garage_achat.h"
#include "../src/ui/garage_skin.h"
using namespace wfs;
using namespace wfs::ecrans::garage;

static int g_echecs = 0;
static void cas(const char* nom, bool ok) {
    std::printf("%s %s\n", ok ? "OK   " : "ÉCHEC", nom);
    if (!ok) g_echecs++;
}
static int P(const char* id) { for (int i = 0; i < ecrans::garage::NB_PEINTURES; i++) if (!std::strcmp(PAINTS[i].id, id)) return i; return -1; }
static int V(const char* id) { for (int i = 0; i < CAR_COUNT; i++) if (!std::strcmp(CARS[i].id, id)) return i; return -1; }
static int S(const char* id) { for (int i = 0; i < BOUTIQUE_SKINS; i++) if (!std::strcmp(BOUTIQUE_SKINS_IDS[i], id)) return i; return -1; }
static int prix(const char* skin, const char* car) { const perso::Objet* o = perso::objet(skin); return o ? perso::prix(*o, car) : -1; }
static bool possede(const Sauvegarde& s, const char* car, const char* id) { const perso::Objet* o = perso::objet(id); return o && perso::possede(s, *o, car); }

int main() {
    // RG-01 : ordre affiché et voitures
    const char* attendu[] = {"rallye", "camouflage", "drifttokyo", "neonnuit", "ciel"};   // WFS-552 : Bleu ciel en dernier
    bool ordre = SkinGarage::NB == 6 && SkinGarage::catalogue(0) == -1;
    for (int o = 1; o < 6; o++) ordre = ordre && !std::strcmp(BOUTIQUE_SKINS_IDS[SkinGarage::catalogue(o)], attendu[o - 1]);
    cas("ordre : Aucune, Rallye, Camouflage, Drift Tokyo, Néon nuit, Bleu ciel", ordre);
    cas("ordre hors bornes : aucune", SkinGarage::catalogue(-1) == -1 && SkinGarage::catalogue(6) == -1);
    int avec = 0;
    for (int i = 0; i < CAR_COUNT; i++) avec += skins::aDesSkins(CARS[i].id);
    cas("rangée visible pour les 7 voitures qui ont des skins (208, 500, GT86, Golf, Corvette C7, Giulia, SF-X : WFS-552), pas pour la M4",
        avec == 7 && skins::aDesSkins("p208") && skins::aDesSkins("abarth") && skins::aDesSkins("gt86") &&
        skins::aDesSkins("golf") && skins::aDesSkins("mustang") && skins::aDesSkins("giulia") && skins::aDesSkins("sf90") && !skins::aDesSkins("m4"));
    cas("en vente : Rallye, Drift Tokyo, Camouflage, Bleu ciel (jamais Néon nuit)", skins::enVente("golf").size() == 4 && skins::enVente("m4").empty());
    // prix : base × coefficient de la classe (configgame.json)
    cas("208 (gratuite, × 1) : Rallye 300, Camouflage 450, Drift Tokyo 600 ★",
        prix("rallye", "p208") == 300 && prix("camouflage", "p208") == 450 && prix("drifttokyo", "p208") == 600);
    cas("Golf (classe 2, × 1,5) : Rallye 450 ★ ; Giulia (classe 3, × 2) : Drift Tokyo 1 200 ★", prix("rallye", "golf") == 450 && prix("drifttokyo", "giulia") == 1200);
    // RG-04 : Néon nuit jamais en vente
    {
        Sauvegarde s;
        long long solde = 100000;
        const perso::Objet* neon = perso::objet("neonnuit");
        cas("Néon nuit : jamais en vente (rien de débité)", neon && perso::acheter(s, solde, *neon, "golf") == perso::Achat::NonVendu && solde == 100000);
        EtatGarage e; e.index = V("golf"); e.peinture = P("bleu"); e.voiturePrete = true; e.skin = S("neonnuit");
        cas("Néon nuit montrée : rien à acheter (« Choisir », elle ne part pas en course)", manque(s, e).n == 0);
        cas("aspect en course : skin non possédée ignorée", [&] { s.skin["golf"] = "neonnuit"; return std::string(perso::aspectCourse(s, "golf").skin).empty(); }());
        cas("donnerSkin(golf, neonnuit) : possédée, pas choisie d'office", skins::donnerSkin(s, "golf", "neonnuit") && possede(s, "golf", "neonnuit") && !possede(s, "giulia", "neonnuit"));
        cas("donnerSkin : déjà possédée ou voiture sans skin : refus", !skins::donnerSkin(s, "golf", "neonnuit") && !skins::donnerSkin(s, "m4", "rallye") && !skins::donnerSkin(s, "golf", "inconnue"));
        cas("aspect en course : Néon nuit possédée et choisie → en course", std::string(perso::aspectCourse(s, "golf").skin) == "neonnuit");
    }
    // ce qui manque : la skin s'ajoute à la couleur et à la finition ; achat refusé puis fait ; une seule voiture
    {
        Sauvegarde s;   // inactive : rien n'est écrit sur le disque
        EtatGarage e; e.index = V("golf"); e.peinture = P("bleu"); e.voiturePrete = true;
        cas("Golf Bleu sans skin : rien ne manque", manque(s, e).n == 0);
        e.skin = S("rallye");
        cas("Golf Rallye : 450 ★, 1 objet", manque(s, e).n == 1 && manque(s, e).total == 450);
        e.peinture = P("bleuelectrique"); e.peinture2 = P("rose"); e.finition = 7;   // caméléon (600 ★ sur la Golf)
        cas("Golf Bleu électrique + toit Rose + Caméléon + Rallye : 5 objets, 75 + 75 + 75 + 600 + 450 = 1 275 ★", manque(s, e).n == 5 && manque(s, e).total == 1275);
        e.peinture = P("bleu"); e.peinture2 = -1; e.finition = 0;
        const perso::Objet* r = perso::objet("rallye");
        s.etoiles = 100;
        cas("refus faute d'étoiles : rien de débité", r && boutique::acheterObjet(s, *r, "golf") == perso::Achat::SoldeInsuffisant && s.etoiles == 100 && manque(s, e).total == 450);
        s.etoiles = 1000;
        cas("achat Rallye sur la Golf : 450 ★ débitées une fois, plus rien ne manque", r && boutique::acheterObjet(s, *r, "golf") == perso::Achat::Achete && s.etoiles == 550 && manque(s, e).n == 0);
        cas("RG-03 : le Rallye de la Golf ne vaut pas pour la Giulia", !possede(s, "giulia", "rallye"));
        cas("skin hors bornes : ignorée", [&] { EtatGarage x = e; x.skin = 42; return manque(s, x).n == 0; }());
    }
    // RG-02 : skin offerte
    const std::string chemin = "/tmp/wfs-test-garage-skin.txt";
    std::remove(chemin.c_str());
    {
        Sauvegarde s;
        cas("sans sauvegarde active : aucune skin offerte (bancs des autres tickets)", !skins::offerteDue(s, "p208"));
        s.charger(chemin);
        cas("208, 500, GT86 (gratuites) : skin offerte due dès le départ", skins::offerteDue(s, "p208") && skins::offerteDue(s, "abarth") && skins::offerteDue(s, "gt86"));
        cas("Golf pas achetée : pas de skin offerte ; M4 (sans skin) : jamais", !skins::offerteDue(s, "golf") && !skins::offerteDue(s, "m4"));
        cas("Néon nuit ne peut pas être la skin offerte", !skins::offrir(s, "p208", "neonnuit") && skins::offerteDue(s, "p208"));
        cas("208 : Camouflage offerte (possédée, choisie, droit utilisé)", skins::offrir(s, "p208", "camouflage") && possede(s, "p208", "camouflage") &&
            s.skin["p208"] == "camouflage" && !skins::offerteDue(s, "p208") && s.etoiles == 0);
        cas("une seule skin offerte par voiture", !skins::offrir(s, "p208", "rallye") && !possede(s, "p208", "rallye"));
        cas("la skin offerte de la 208 ne vaut pas pour la 500 (qui garde son droit)", !possede(s, "abarth", "camouflage") && skins::offerteDue(s, "abarth"));
        s.voituresPossedees.insert("golf");
        cas("Golf achetée : sa skin offerte devient due", skins::offerteDue(s, "golf"));
        // bon de skin (pack débutant du market)
        cas("sans bon : pas d'« Utiliser un bon »", !skins::bonUtilisable(s, "golf", "drifttokyo"));
        s.market.bonsSkin = 1;
        cas("bon : pas sur Néon nuit, ni sur une voiture pas possédée, ni sur une skin déjà possédée",
            !skins::bonUtilisable(s, "golf", "neonnuit") && !skins::bonUtilisable(s, "giulia", "rallye") && !skins::bonUtilisable(s, "p208", "camouflage"));
        cas("bon utilisé : Drift Tokyo sur la Golf, 0 bon restant, 0 ★ débitée", skins::utiliserBon(s, "golf", "drifttokyo") && possede(s, "golf", "drifttokyo") &&
            s.market.bonsSkin == 0 && s.etoiles == 0 && s.skin["golf"] == "drifttokyo");
        cas("plus de bon : refus", !skins::utiliserBon(s, "golf", "rallye"));
        // relecture de la sauvegarde
        Sauvegarde t;
        const bool lu = t.lire(s.texte());
        cas("relecture : skins-offertes, skins possédées et choisies gardées", lu && t.skinsOffertes.count("p208") && !t.skinsOffertes.count("golf") &&
            possede(t, "p208", "camouflage") && possede(t, "golf", "drifttokyo") && t.skin["p208"] == "camouflage" && t.market.bonsSkin == 0);
        cas("clé skins-offertes : voiture inconnue ignorée, sans erreur", [&] { Sauvegarde u; return u.lire(s.texte() + "skins-offertes=p208|inconnue\n") && u.skinsOffertes.size() == 1; }());
        cas("sauvegarde d'avant ce ticket (sans la clé) : lue sans erreur, droits intacts", [&] { Sauvegarde u; return u.lire("version=1\ncar=p208\netoiles=10\n") && u.skinsOffertes.empty(); }());
        s.desactiver();
    }
    std::remove(chemin.c_str());
    cas("market : le pack débutant (bon de skin) est proposé maintenant que les skins sont en vente", market::SKINS_EN_VENTE);
    std::printf("%s : %d échec(s)\n", g_echecs ? "ÉCHEC" : "OK", g_echecs);
    return g_echecs ? 1 : 0;
}
