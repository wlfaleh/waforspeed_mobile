// WFS-510 (RG-10) : market — chaque pack crédite son total une seule fois ; même jeton deux fois → une seule fois ; en attente /
// annulé → 0 ; interrompu puis relance → une seule fois ; pack débutant absent avant la 3e course et après l'achat ; ×2 limité à 5
// par jour (avec et sans « sans pub »), minuit, heure qui recule, une fois par course ; livre des achats gardé dans la sauvegarde
// (200 jetons) ; configgame.json mal écrit → valeurs par défaut + message, jamais d'arrêt.
#include <cstdio>
#include <string>
#include "../src/core/configgame.h"
#include "../src/game/market.h"
using namespace wfs;
using namespace wfs::market;

static int g_echecs = 0;
static void cas(const std::string& nom, bool ok) {
    std::printf("%s %s\n", ok ? "OK   " : "ÉCHEC", nom.c_str());
    if (!ok) g_echecs++;
}

int main() {
    // ---- RG-01 : catalogue de l'utilisateur
    {
        const long long totaux[MARKET_PRODUITS] = {100, 330, 720, 4500, 500, 0};
        bool ok = (int)catalogue().size() == MARKET_PRODUITS;
        for (int i = 0; i < MARKET_PRODUITS; i++) ok = ok && pack(i).total == totaux[i] && pack(i).id == MARKET_PRODUITS_IDS[i];
        cas("catalogue : 100, 330, 720, 4 500, 500 ★ (+ bon de skin), sans pub", ok && pack(4).bonsSkin == 1 && !pack(4).consommable && !pack(5).consommable && pack(3).consommable);
        cas("total = base + arrondi(base × offert) : 333 + 10 % = 366 ; 5 + 10 % = 6 (moitié vers le haut)", totalEtoiles(333, 10) == 366 && totalEtoiles(5, 10) == 6 && totalEtoiles(100, 0) == 100);
        cas("produit inconnu : −1", indexProduit("etoiles_platine") == -1 && indexProduit("sans_pub") == 5);
        cas("prix de secours : 2,49 € et 19,99 €", texteEurosSecours(pack(1).prixSecours) == "2,49 €" && texteEurosSecours(pack(3).prixSecours) == "19,99 €");
    }
    // ---- RG-03, RG-07 : chaque pack crédite son total une seule fois ; même jeton deux fois
    {
        Sauvegarde s; s.defauts();
        BoutiqueSimulee b; brancher(b, s);
        bool ok = true; long long avant = 0;
        for (int i = 0; i < MARKET_ETOILES; i++) { avant = s.etoiles; b.acheter(MARKET_PRODUITS_IDS[i]); ok = ok && s.etoiles == avant + pack(i).total; }
        cas("chaque pack d'étoiles crédite son total (5 650 ★ en tout)", ok && s.etoiles == 5650 && b.confirmes() == 4 && b.nonConfirmes() == 0);
        avant = s.etoiles; b.doubler(true); b.acheter("etoiles_or");
        cas("même jeton reçu deux fois : crédité une seule fois, confirmé", s.etoiles == avant + 720 && b.nonConfirmes() == 0);
        avant = s.etoiles;
        cas("crediterAchat direct, jeton déjà au livre : « déjà crédité », 0 ★", crediterAchat(s, "etoiles_or", s.market.achats.back()) == Credit::DejaCredite && s.etoiles == avant);
        cas("jeton mal formé ou produit inconnu : « inconnu », rien de changé",
            crediterAchat(s, "etoiles_or", "a|b") == Credit::Inconnu && crediterAchat(s, "etoiles_or", "") == Credit::Inconnu &&
            crediterAchat(s, "gemmes", "jeton-1") == Credit::Inconnu && s.etoiles == avant);
        const Traitement t = traiterAchat(s, "etoiles_or", s.market.achats.back(), Etat::Achete);
        cas("déjà crédité : l'appelant confirme quand même", t.credit == Credit::DejaCredite && t.confirmer);
    }
    // ---- RG-04 : en attente, annulé, erreur → 0
    {
        Sauvegarde s; s.defauts();
        BoutiqueSimulee b; std::string message; bool confirme = false;
        brancher(b, s, [&](const std::string&, const Traitement& t) { message = t.message; confirme = t.confirmer; });
        b.repondre(BoutiqueSimulee::Reponse::EnAttente); b.acheter("etoiles_coffre");
        cas("en attente : 0 ★, message « Achat en attente de paiement », pas de confirmation", s.etoiles == 0 && message == "Achat en attente de paiement" && !confirme);
        b.repondre(BoutiqueSimulee::Reponse::Annule); b.acheter("etoiles_coffre");
        cas("annulé : 0 ★, aucun message", s.etoiles == 0 && message.empty() && !confirme);
        b.acheter("gemmes");
        cas("erreur (produit inconnu) : 0 ★, aucun message", s.etoiles == 0 && message.empty());
    }
    // ---- RC-E21-06 : achat interrompu (crédit écrit, confirmation pas faite) puis relance
    {
        Sauvegarde s; s.defauts();
        BoutiqueSimulee b; brancher(b, s);
        b.repondre(BoutiqueSimulee::Reponse::Interrompu); b.acheter("etoiles_argent");
        const bool credite = s.etoiles == 330 && b.nonConfirmes() == 1;
        // lancement suivant : la sauvegarde relue (même fichier), la boutique renvoie l'achat pas encore confirmé
        Sauvegarde relue; relue.lire(s.texte());
        BoutiqueSimulee b2 = b; brancher(b2, relue);
        b2.repondre(BoutiqueSimulee::Reponse::Achete); b2.chargerProduits();
        cas("interrompu : crédité une fois, puis relance : pas recrédité et confirmé", credite && relue.etoiles == 330 && b2.nonConfirmes() == 0);
    }
    // ---- RG-02 : pack débutant
    {
        Sauvegarde s; s.defauts();
        s.profil.courses = 2;
        const bool avant3 = packDebutantPropose(s, true);
        s.profil.courses = 3;
        const bool a3 = packDebutantPropose(s, true);
        cas("pack débutant : absent avant la 3e course, proposé après", !avant3 && a3);
        cas("pack débutant : absent tant qu'aucune skin n'est en vente", !packDebutantPropose(s, false) && packDebutantPropose(s) == SKINS_EN_VENTE);
        BoutiqueSimulee b; brancher(b, s);
        b.acheter("pack_debutant");
        cas("pack débutant acheté : 500 ★ + 1 bon de skin, plus jamais proposé", s.etoiles == 500 && s.market.bonsSkin == 1 && s.market.packDebutant && !packDebutantPropose(s, true));
        b.acheter("pack_debutant");   // autre jeton (ne devrait pas arriver) : une seule fois par joueur
        cas("pack débutant une 2e fois (autre jeton) : rien de plus", s.etoiles == 500 && s.market.bonsSkin == 1);
    }
    // ---- RG-05 : sans pub, restauration
    {
        Sauvegarde s; s.defauts();
        BoutiqueSimulee b; brancher(b, s);
        cas("sans pub : faux au départ", !sansPub(s));
        b.acheter("sans_pub");
        cas("sans pub acheté : vrai, 0 ★", sansPub(s) && s.etoiles == 0);
        Sauvegarde neuve; neuve.defauts();   // réinstallation : sauvegarde vide, la boutique rend l'achat
        BoutiqueSimulee b2 = b; brancher(b2, neuve); b2.restaurer();
        cas("restaurer après réinstallation : sans pub revient", sansPub(neuve));
        Sauvegarde relue; relue.lire(s.texte());
        cas("sans pub gardé dans la sauvegarde (sans réseau)", sansPub(relue));
    }
    // ---- RG-06 : ×2 de fin de course
    {
        Sauvegarde s; s.defauts();
        int accordes = 0; long long c = 0;
        for (int k = 0; k < 7; k++) accordes += recompenseVideo(s, 10, ++c, 20261113) > 0;
        cas("×2 (vidéo) : 5 accordés sur 7 le même jour, +50 ★", accordes == 5 && s.etoiles == 50 && x2Restants(s, 20261113) == 0);
        cas("×2 : une seule fois par course", recompenseVideo(s, 10, c + 1, 20261114) == 10 && recompenseVideo(s, 10, c + 1, 20261114) == 0);
        cas("×2 : après minuit, compteur remis à zéro (4 restants)", x2Restants(s, 20261114) == 4);
        for (int k = 0; k < 4; k++) recompenseVideo(s, 10, c + 2 + k, 20261114);
        cas("×2 : heure du téléphone qui recule : pas de remise à zéro", x2Restants(s, 20261113) == 0 && recompenseVideo(s, 10, c + 10, 20261113) == 0 && s.market.x2Jour == 20261114);
        Sauvegarde relue; relue.lire(s.texte());
        cas("×2 : compteur gardé dans la sauvegarde", x2Restants(relue, 20261114) == 0 && relue.market.x2Jour == 20261114 && relue.market.x2Nombre == 5);
        Sauvegarde p; p.defauts();
        cas("×2 direct sans « sans pub » : refusé", x2SansPub(p, 10, 1, 20261113) == 0 && p.etoiles == 0);
        p.market.sansPub = true; accordes = 0;
        for (int k = 1; k <= 7; k++) accordes += x2SansPub(p, 10, k, 20261113) > 0;
        cas("×2 direct avec « sans pub » : 5 par jour aussi", accordes == 5 && p.etoiles == 50);
        cas("×2 : gain nul ou course 0 refusés", recompenseVideo(p, 0, 9, 20261120) == 0 && recompenseVideo(p, 10, 0, 20261120) == 0);
        cas("jour local : AAAAMMJJ plausible", aujourdhui() > 20200101 && aujourdhui() < 99991231);
    }
    // ---- RG-03 : livre gardé dans la sauvegarde, 200 derniers jetons
    {
        Sauvegarde s; s.defauts();
        for (int i = 0; i < 205; i++) crediterAchat(s, "etoiles_bronze", "jeton-" + std::to_string(i));
        Sauvegarde relue; relue.lire(s.texte());
        cas("livre : 200 derniers jetons gardés et relus", (int)relue.market.achats.size() == 200 && relue.market.achats.front() == "jeton-5" && relue.etoiles == 20500);
        cas("jeton relu : jamais recrédité", crediterAchat(relue, "etoiles_bronze", "jeton-204") == Credit::DejaCredite && relue.etoiles == 20500);
        Sauvegarde abimee; abimee.lire("version=1\netoiles=0\nachats-credites=ok-1|mal formé|ok-2\nbons-skin=abc\nx2=2026;1\nsans-pub=oui\n");
        cas("clés du market mal écrites : ignorées sans erreur", abimee.market.achats.size() == 2 && abimee.market.bonsSkin == 0 && abimee.market.x2Jour == 0 && !abimee.market.sansPub);
    }
    // ---- configgame.json : valeurs lues, mal écrites → défauts + message
    {
        ConfigJeu c;
        BilanConfig b = lireConfig("{\"market\": {\"packs\": {\"etoiles_argent\": {\"etoiles\": 350}}, \"x2_par_jour\": 3}}", "essai", c);
        cas("configgame : argent à 350 ★ et 3 ×2 par jour lus", b.jsonValide && c.market.pack[1].etoiles == 350 && c.market.x2ParJour == 3 && totalEtoiles(c.market.pack[1].etoiles, c.market.pack[1].offert) == 385);
        ConfigJeu d;
        b = lireConfig("{\"market\": {\"packs\": {\"etoiles_or\": {\"etoiles\": -5, \"offert\": \"vingt\"}}, \"x2_par_jour\": 500}}", "essai", d);
        bool messages = false; for (const std::string& m : b.messages) messages = messages || m.find("market.") != std::string::npos;
        cas("configgame mal écrit : valeurs du code (600 ★, 20 %, 5 par jour) et messages", d.market.pack[2].etoiles == 600 && d.market.pack[2].offert == 20 && d.market.x2ParJour == 5 && messages && b.erreurs >= 3);
        ConfigJeu e;
        b = lireConfig("{\"market\": {", "essai", e);
        cas("configgame cassé : valeurs du code, pas d'arrêt", !b.jsonValide && e.market.pack[3].etoiles == 3600);
    }
    // ---- RC-E21-03 : aucun interdit dans le catalogue (seulement des packs d'étoiles, le pack débutant et le sans pub)
    {
        bool ok = true;
        for (const Pack& p : catalogue()) ok = ok && (p.consommable || p.id == "pack_debutant" || p.id == "sans_pub");
        cas("catalogue : rien d'autre que des étoiles, le pack débutant et le sans pub", ok);
    }
    std::printf("%s : %d échec(s)\n", g_echecs ? "test_market ÉCHEC" : "test_market OK", g_echecs);
    return g_echecs ? 1 : 0;
}
