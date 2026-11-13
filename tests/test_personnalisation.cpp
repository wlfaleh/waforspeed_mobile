// WFS-487 (RG-08) : catalogue de personnalisation. Sauvegarde d'avant (couleur gardée, gratuite) ; 8 couleurs gratuites, 20 à 50 ★ ×
// coefficient (Golf 75 ★, SF90 200 ★) ; couleur achetée sur la Golf absente sur la M4 ; achat avec solde suffisant, insuffisant, déjà
// possédé ; couleur d'origine gratuite ; aspect en course jamais non possédé ; catalogue mal écrit (texte vide, prix négatif) refusé
// avec un message ; don provisoire du garage actuel (option A du PO, jusqu'à WFS-489).
#include <cstdio>
#include <cstring>
#include <string>
#include "../src/core/configgame.h"
#include "../src/game/personnalisation.h"
#include "../src/game/cars.h"
using namespace wfs;
using namespace wfs::perso;

static int g_echecs = 0;
static void cas(const char* nom, bool ok) {
    std::printf("%s %s\n", ok ? "OK   " : "ÉCHEC", nom);
    if (!ok) g_echecs++;
}
static const Objet& O(const char* id) {
    const Objet* o = objet(id);
    if (!o) { std::printf("ÉCHEC objet inconnu : %s\n", id); std::fflush(stdout); std::abort(); }
    return *o;
}
static std::string remplacer(std::string s, const std::string& a, const std::string& b) {
    const size_t i = s.find(a); if (i != std::string::npos) s.replace(i, a.size(), b); return s;
}

int main() {
    // ---- catalogue (RG-01, RG-02, RG-04)
    {
        int n[5] = {};
        for (const Objet& o : catalogue()) n[(int)o.type]++;
        cas("28 couleurs, 1 couleur secondaire, 8 finitions, 4 skins, 5 packs d'autocollants", n[0] == 28 && n[1] == 1 && n[2] == 8 && n[3] == 4 && n[4] == 5);
        bool ordre = true;
        for (int i = 0; i < BOUTIQUE_COULEURS; i++) ordre = ordre && !std::strcmp(PEINTURES[i].id, BOUTIQUE_COULEURS_IDS[i]);
        cas("PEINTURES et boutique.couleurs dans le même ordre", ordre);
        cas("16 couleurs historiques inchangées (turquoise 0F8F8C, taxi E8B400, blanc C9CCD2)",
            peintureDef("turquoise")->hex == 0x0f8f8c && peintureDef("taxi")->hex == 0xe8b400 && peintureDef("blanc")->hex == 0xc9ccd2 && NB_PEINTURES_GARAGE == 16 &&
            !std::strcmp(PEINTURES[15].id, "taxi"));
        cas("12 couleurs vives de RG-02 (bleu électrique 0050FF … bleu ciel 4FA8FF)",
            peintureDef("bleuelectrique")->hex == 0x0050ff && peintureDef("vertacide")->hex == 0x7fff00 && peintureDef("violetneon")->hex == 0x9d00ff &&
            peintureDef("bleuciel")->hex == 0x4fa8ff && peintureDef("corail")->hex == 0xff6f61);
        cas("identifiants uniques, catalogue lu sans problème", verifierCatalogue().empty());
    }
    // ---- prix (RG-03, WFS-498 RG-04 / RG-05)
    {
        int gratuites = 0, a50 = 0;
        for (const Objet& o : catalogue())
            if (o.type == Type::Couleur) { const auto& r = config().boutique.couleur[o.index]; gratuites += r.gratuit == 1; a50 += r.gratuit == 0 && r.prix == 50; }
        cas("8 couleurs gratuites, 20 à 50 ★ de base", gratuites == 8 && a50 == 20);
        bool huit = true;
        for (const char* id : {"blanc", "noir", "gris", "rouge", "bleu", "jaune", "vert", "orange"}) huit = huit && prix(O(id), "sf90") == 0 && gratuit(O(id), "sf90");
        cas("blanc, noir, gris, rouge, bleu, jaune, vert, orange gratuits même sur la SF90", huit);
        cas("couleur vive : Golf 75 ★, SF90 200 ★, GT86 50 ★", prix(O("rose"), "golf") == 75 && prix(O("rose"), "sf90") == 200 && prix(O("rose"), "gt86") == 50);
        cas("finitions : métallisé SF90 400 ★, candy Golf 375 ★, brillant gratuit", prix(O("metallise"), "sf90") == 400 && prix(O("candy"), "golf") == 375 && prix(O("brillant"), "m4") == 0);
        cas("packs d'autocollants sans coefficient : bandes 200 ★ sur la Golf et la SF90", prix(O("bandes"), "golf") == 200 && prix(O("bandes"), "sf90") == 200 && prix(O("essentiels"), "sf90") == 0);
        cas("skins : Rallye sur la 208 300 ★, sur la Golf 450 ★ ; pas sur la M4 ; Néon nuit cadeau", prix(O("rallye"), "p208") == 300 && prix(O("rallye"), "golf") == 450 &&
            !disponible(O("rallye"), "m4") && disponible(O("rallye"), "mustang") && cadeau(O("neonnuit")) && !cadeau(O("rallye")));
        cas("couleur d'origine de la Golf gratuite pour elle", gratuit(O(peintureOrigine("golf")), "golf"));
        // une voiture dont la couleur d'origine est payante ailleurs
        std::string origPayante;
        for (const auto& c : CARS) { const Objet* o = couleur(peintureOrigine(c.id)); if (o && !config().boutique.couleur[o->index].gratuit && !voitureRetiree(c.id)) { origPayante = c.id; break; } }
        if (!origPayante.empty()) {
            const Objet& oc = *couleur(peintureOrigine(origPayante));
            std::string autre = origPayante == "sf90" ? "m4" : "sf90";
            cas(("couleur d'origine payante ailleurs : " + std::string(oc.id) + " gratuite sur " + origPayante + ", payante sur " + autre).c_str(),
                prix(oc, origPayante) == 0 && prix(oc, autre) > 0);
        } else cas("couleur d'origine payante ailleurs : aucune voiture concernée (cas sans objet)", true);
    }
    // ---- possession et achat (RG-05)
    {
        Sauvegarde s; s.defauts();
        long long solde = 100;
        cas("achat couleur rose sur la Golf (75 ★) : acheté, solde 25", acheter(s, solde, O("rose"), "golf") == Achat::Achete && solde == 25);
        cas("rose possédée sur la Golf, absente sur la M4", possede(s, O("rose"), "golf") && !possede(s, O("rose"), "m4"));
        cas("déjà possédé : aucun débit", acheter(s, solde, O("rose"), "golf") == Achat::DejaPossede && solde == 25);
        cas("solde insuffisant (cyan Golf 75 ★, 25 au solde) : refus, rien de débité, rien de possédé",
            acheter(s, solde, O("cyan"), "golf") == Achat::SoldeInsuffisant && solde == 25 && !possede(s, O("cyan"), "golf"));
        cas("couleur gratuite : déjà possédée, aucun débit", acheter(s, solde, O("rouge"), "m4") == Achat::DejaPossede && solde == 25);
        cas("Néon nuit pas en vente ; skin sur une voiture sans skin : pas en vente", acheter(s, solde = 5000, O("neonnuit"), "p208") == Achat::NonVendu &&
            acheter(s, solde, O("rallye"), "m4") == Achat::NonVendu && solde == 5000);
        cas("pack Bandes acheté une fois, présent sur toutes les voitures", acheter(s, solde, O("bandes"), "golf") == Achat::Achete && solde == 4800 &&
            possede(s, O("bandes"), "m4") && possede(s, O("bandes"), "sf90") && acheter(s, solde, O("bandes"), "sf90") == Achat::DejaPossede);
        // relue : possession gardée
        Sauvegarde r; cas("possession écrite puis relue (possede-golf, possede-dessins)", r.lire(s.texte()) && possede(r, O("rose"), "golf") && !possede(r, O("rose"), "m4") &&
                          possede(r, O("bandes"), "v60") && r.texte().find("possede-golf=rose") != std::string::npos);
    }
    // ---- aspect garage / course (RG-06, RG-07)
    {
        Sauvegarde s; s.defauts();
        s.paint["m4"] = "magenta"; s.finition["m4"] = "chrome"; s.skin["p208"] = "rallye"; s.paint2["m4"] = "rouge"; s.skin["m4"] = "rallye";
        const Aspect g = aspectGarage(s, "m4"), c = aspectCourse(s, "m4");
        cas("garage : aperçu d'objets non possédés (magenta, chromé)", g.peinture && !std::strcmp(g.peinture->id, "magenta") && !std::strcmp(g.finition, "chrome"));
        cas("course : jamais un objet non possédé (couleur d'origine, brillant, sans secondaire, sans skin)", !c.peinture && !std::strcmp(c.finition, "brillant") && !c.peinture2 && !*c.skin &&
            s.peinture("m4") == peintureOrigine("m4") && !s.peintureCourse("m4"));
        cas("skin d'une voiture qui ne l'a pas : ignorée même au garage", !*g.skin);
        long long solde = 10000;
        acheter(s, solde, O("magenta"), "m4"); acheter(s, solde, O("chrome"), "m4"); acheter(s, solde, O("rallye"), "p208");
        const Aspect c2 = aspectCourse(s, "m4"), c3 = aspectCourse(s, "p208");
        cas("course après achat : magenta chromé sur la M4, Rallye sur la 208", c2.peinture && !std::strcmp(c2.peinture->id, "magenta") && !std::strcmp(c2.finition, "chrome") &&
            !std::strcmp(c3.skin, "rallye") && s.peinture("m4") == "magenta");
        cas("couleur secondaire sans le droit « secondaire » : ignorée en course", !c2.peinture2);
        acheter(s, solde, O(ID_SECONDAIRE), "m4");
        cas("couleur secondaire achetée (M4 100 ★) : rouge sur le toit en course", aspectCourse(s, "m4").peinture2 && !std::strcmp(aspectCourse(s, "m4").peinture2->id, "rouge"));
        Sauvegarde r; r.lire(s.texte());
        cas("choix relus : paint2, finition, skin", r.finition["m4"] == "chrome" && r.skin["p208"] == "rallye" && r.paint2["m4"] == "rouge");
        Sauvegarde x; x.lire("version=1\nperso=1\nfinition-m4=rose\nskin-golf=chrome\npossede-m4=inconnu|magenta\ncreation-golf=../x\npaint2-m4=nimporte\n");
        cas("valeurs inconnues → défaut sans erreur", x.finition.empty() && x.skin.empty() && x.possede["m4"].size() == 1 && x.creation.empty() && x.paint2.empty());
    }
    // ---- sauvegarde d'avant ce ticket (RG-03)
    {
        Sauvegarde s;
        cas("sauvegarde d'avant : lue sans erreur", s.lire("version=1\ncar=golf\npaint-golf=turquoise\npaint-m4=rouge\n"));
        cas("turquoise choisie avant : gardée, gratuite, en course", possede(s, O("turquoise"), "golf") && s.peinture("golf") == "turquoise" && s.peintureCourse("golf") &&
            !possede(s, O("turquoise"), "m4") && s.perso == 1);
        Sauvegarde r; r.lire(s.texte());
        cas("migration faite une fois (perso=1 écrit, possession gardée)", r.texte().find("perso=1") != std::string::npos && possede(r, O("turquoise"), "golf"));
        Sauvegarde n; n.lire("version=1\nperso=1\npaint-golf=turquoise\n");
        cas("sauvegarde du nouveau format : pas de migration (turquoise non achetée → origine en course)", !possede(n, O("turquoise"), "golf") && !n.peintureCourse("golf"));
    }
    // ---- garage actuel : don provisoire jusqu'à WFS-489 (option A du PO)
    {
        Sauvegarde s; s.defauts();
        cas("« Choisir » turquoise sur la M4 dans le garage actuel : donnée à la M4 seulement", donGarageProvisoire(s, "m4", "turquoise") && possede(s, O("turquoise"), "m4") &&
            !possede(s, O("turquoise"), "golf"));
        s.paint["m4"] = "turquoise";
        cas("la course a la couleur choisie au garage", s.peintureCourse("m4") && !std::strcmp(s.peintureCourse("m4")->id, "turquoise"));
        cas("nouvelle couleur (hors des 16 du garage actuel) : jamais donnée", !donGarageProvisoire(s, "m4", "rose") && !possede(s, O("rose"), "m4"));
        cas("couleur gratuite ou déjà possédée : rien à donner", !donGarageProvisoire(s, "m4", "rouge") && !donGarageProvisoire(s, "m4", "turquoise"));
    }
    // ---- catalogue mal écrit dans configgame.json (RG-08) : refusé, valeur du code, message
    {
        const std::string defauts = configJson(ConfigJeu{});
        ConfigJeu c0; const BilanConfig b0 = lireConfig(defauts, "défauts", c0);
        cas("section boutique lue sans erreur", b0.erreurs == 0 && defauts.find("\"boutique\"") != std::string::npos);
        std::string t = remplacer(defauts, "\"rose\": {\n        \"prix\": 50", "\"rose\": {\n        \"prix\": -5");
        t = remplacer(t, "\"voitures\": \"p208 abarth gt86 golf mustang giulia\"", "\"voitures\": \"\"");
        ConfigJeu c; const BilanConfig b = lireConfig(t, "boutique mal écrite", c);
        int msg = 0; for (const std::string& m : b.messages) { msg += m.find("boutique.") == 0; std::printf("  %s\n", m.c_str()); }
        cas("prix négatif et liste vide refusés : valeur du code + message", b.erreurs == 2 && msg == 2 && c.boutique.couleur[19].prix == 50 &&
            !std::strcmp(c.boutique.skinVoitures[0], "p208 abarth gt86 golf mustang giulia"));
        ConfigJeu d; lireConfig(remplacer(defauts, "\"rose\": {\n        \"prix\": 50", "\"rose\": {\n        \"prix\": 80"), "prix changé", d);
        const ConfigJeu sauve = config(); config() = d;
        cas("prix changé dans configgame.json sans recompiler : rose sur la Golf 120 ★", prix(O("rose"), "golf") == 120);
        config() = sauve;
    }
    std::printf("%s : %d échec(s)\n", g_echecs ? "ÉCHEC" : "OK", g_echecs);
    return g_echecs ? 1 : 0;
}
