// WaForSpeed — banc de l'écran « Market » (WFS-513, RG-08) : WFS_BANC=market (après le banc logique de WFS-510).
// Boutique simulée de WFS-510 (WFS_BOUTIQUE=simulee, prix de secours) ; sauvegarde du banc, effacée à la fin.
// Captures market-<langue>-<nom>.png : menu, market, achat, attente, sans-pub, restauration, garage, garage-market, hors-ligne.
#include "market_ecran.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include "../../core/configgame.h"
#include "../../core/log.h"
#include "../../game/boutique.h"
#include "../../game/cars.h"
#include "../../game/game.h"
#include "../../game/market.h"
#include "../../game/sauvegarde.h"
#include "../../platform/retour_systeme.h"   // touche Retour d'Android
#include "../../render/renderer.h"
#include "../garage.h"
#include "../garage_achat.h"   // voitureBloquee
#include "../langue.h"
#include "../text.h"
#include "compteur_etoiles.h"

namespace wfs::ecrans::market_ecran {
namespace {

int benchUneLangue(Game& gm, Renderer& r, const std::string& out, ui::langue::Choix choix) {
    int ecarts = 0;
    const std::string chemin = out + "/market-sauvegarde-banc.txt";
    std::remove(chemin.c_str());
    sauvegarde().charger(chemin);
    ui::langue::choisir(choix);
    const std::string lg = ui::langue::code();
    auto verifier = [&](bool ok, const char* quoi) { WFS_LOG("market (WFS-513) banc [%s] : %s : %s", lg.c_str(), quoi, ok ? "OK" : "ÉCART"); if (!ok) ecarts++; };
    // joueur avec un profil (pas d'écran « Crée ton pilote ») et 1 250 ★
    Sauvegarde& sv = sauvegarde();
    sv.cgu = 1;
    std::string nick;
    sv.profil.creer("Karima", "KARIMA", nick);
    sv.nick = nick;
    boutique::crediter(sv, 1250, "banc market");
    sv.enregistrer();
    horsLigneBanc(false);
    nouvelleBoutiqueBanc();
    market::BoutiqueSimulee* sim = dynamic_cast<market::BoutiqueSimulee*>(boutique());
    verifier(sim != nullptr, "RG-08 : boutique simulée de WFS-510 (WFS_BOUTIQUE=simulee)");
    if (!sim) { sauvegarde().desactiver(); return ecarts; }
    sim->repondre(market::BoutiqueSimulee::Reponse::Achete);
    if (!gm.init(r, "golf", "ville")) { WFS_ERR("market (WFS-513) banc : chargement impossible"); sauvegarde().desactiver(); return ecarts + 1; }
    gm.setAutopilot(true); gm.camMode = 0;
    double t = 5000;
    auto image = [&]() { gm.update(1.0 / 60); t += 1.0 / 60; gm.render(t); };
    auto images = [&](int n) { for (int i = 0; i < n; i++) image(); };
    auto capture = [&](const char* nom, const char* quoi) {
        gm.render(t); gm.render(t + 0.01);
        const std::string fichier = std::string("market-") + lg + "-" + nom;
        r.capturePng(out + "/" + fichier + ".png");
        WFS_LOG("capture %s : écran %s, %s", fichier.c_str(), nomEcran(gm.ecran()), quoi);
    };
    int doigt = 120;
    auto appui = [&](float x, float y) { t += 0.2; gm.onTouch({doigt, x, y, true}); image(); gm.onTouch({doigt, x, y, false}); image(); doigt++; };
    // WFS-557 : depuis WFS-550 (maquette 01), le Market s'ouvre au menu par le compteur d'étoiles (plus de bouton « Market »)
    auto appuyer = [&](const char* nom) {
        float x, y;
        if (std::string(nom) == "bouton" ? !compteur_etoiles::centre(x, y) : !centre(nom, x, y)) return false;
        appui(x, y); return true;
    };
    auto retourTelephone = [&]() { retourSysteme(gm, true, 0); image(); retourSysteme(gm, false, 0); images(3); };
    auto solde = [] { return boutique::solde(sauvegarde()); };
    const int OR = 2;
    // ---- menu → Market (RG-01)
    gm.allerA(Ecran::Menu);
    images(60);
    capture("menu", "menu principal : bouton « Market » au-dessus de « Profil »");
    verifier(appuyer("bouton"), "RG-01 : compteur d'étoiles du menu → Market (WFS-550)");
    images(30);
    verifier(ouvert() && gm.ecran() == Ecran::Menu, "RG-01 : menu → Market");
    verifier(soldeAffiche() == 1250, "RG-02 : solde en haut (1 250 ★)");
    capture("market", "Market : solde, 4 packs (bonus, base, prix), « Sans pub », mentions, Restaurer, Retour");
    {   // WFS-557 (revue de la PR #432, RG-18) : pack débutant à l'état « proposé » (bouton 1,99 €), le temps d'une capture
        const int c0 = sauvegarde().profil.courses;
        sauvegarde().profil.courses = std::max(c0, config().market.debutantApres);
        images(2);
        verifier(market::packDebutantPropose(sauvegarde()) == market::SKINS_EN_VENTE, "WFS-557 RG-18 : pack débutant proposé après la 3e course");
        capture("debutant-propose", "Market : pack débutant proposé (prix en grand bouton orange)");
        sauvegarde().profil.courses = c0;
        images(2);
    }
    // ---- achat réussi (RG-03) : solde qui monte, « +720 ★ »
    const long long avant = solde();
    verifier(appuyer("pack-2"), "RG-02 : bouton d'achat de la « Médaille d'or »");
    verifier(solde() == avant + market::pack(OR).total, "RG-03 : achat réussi : +720 ★ crédités (livre des achats de WFS-510)");
    verifier(message() == ui::tr("market.gain", garage::nombreLangue(market::pack(OR).total)), "RG-03 : message « +720 ★ »");
    images(20);
    verifier(soldeAffiche() > avant && soldeAffiche() < solde(), "RG-03 : solde affiché en train de monter");
    images(70);
    verifier(soldeAffiche() == solde(), "RG-03 : solde monté jusqu'au bout");
    capture("achat", "achat réussi : « +720 ★ », solde monté de 1 250 à 1 970");
    verifier(sim->nonConfirmes() == 0 && sim->confirmes() >= 1, "RC-E21-06 : achat consommé après le crédit");
    images(120);   // fin du message
    // ---- en attente (RG-03) : rien de crédité, message
    sim->repondre(market::BoutiqueSimulee::Reponse::EnAttente);
    const long long s1 = solde();
    appuyer("pack-0");
    verifier(solde() == s1 && message() == ui::tr("market.attente"), "RG-03 : en attente : rien de crédité, « Achat en attente de paiement »");
    images(10);
    capture("attente", "achat en attente de paiement : rien de crédité");
    images(160);
    // ---- annulé (RG-03) : rien, aucun message
    sim->repondre(market::BoutiqueSimulee::Reponse::Annule);
    appuyer("pack-0");
    verifier(solde() == s1 && message().empty() && ouvert(), "RG-03 : annulé : rien de crédité, aucun message, écran gardé");
    sim->repondre(market::BoutiqueSimulee::Reponse::Achete);
    // ---- restaurer sans rien de possédé (RG-04) : « Aucun achat à restaurer » après 2 s
    verifier(appuyer("restaurer"), "RG-04 : bouton « Restaurer mes achats »");
    images(140);
    verifier(message() == ui::tr("market.rien_a_restaurer"), "RG-04 : « Aucun achat à restaurer »");
    images(170);
    // ---- « Sans pub » (RG-02) : acheté, puis « Acheté » ; restaurer → « Achats restaurés »
    verifier(appuyer("sans_pub") && market::sansPub(sauvegarde()) && solde() == s1, "RG-02 : « Sans pub » acheté (aucune étoile)");
    images(20);
    float x, y;
    verifier(!centre("sans_pub", x, y), "RG-02 : « Sans pub » possédé : plus de bouton d'achat");
    capture("sans-pub", "« Sans pub » acheté : « Acheté » avec la coche");
    images(170);
    appuyer("restaurer");
    images(5);
    verifier(message() == ui::tr("market.restaures") && market::sansPub(sauvegarde()) && solde() == s1, "RG-04 : « Achats restaurés », rien de crédité deux fois");
    capture("restauration", "« Restaurer mes achats » : « Achats restaurés »");
    images(170);
    // ---- « Retour » → menu (RG-01)
    verifier(appuyer("retour") && !ouvert() && gm.ecran() == Ecran::Menu, "RG-01 : « Retour » → menu");
    images(20);
    // ---- touche Retour d'Android (RG-01)
    appuyer("bouton"); images(20);
    retourTelephone();
    verifier(!ouvert() && gm.ecran() == Ecran::Menu, "RG-01 : touche Retour d'Android → menu");
    images(20);
    // ---- compteur d'étoiles du menu → Market (WFS-537 RG-09)
    if (compteur_etoiles::centre(x, y)) appui(x, y);
    images(10);
    verifier(ouvert(), "WFS-537 RG-09 : le compteur d'étoiles ouvre le Market");
    appuyer("retour"); images(20);
    // ---- garage → Market → retour garage (RG-01), sur une voiture bloquée (prix ★ et ≈ €)
    const char* bloquee = nullptr;
    for (const CarDef& c : CARS) if (garage::voitureBloquee(sauvegarde(), c.id)) { bloquee = c.id; break; }
    verifier(bloquee != nullptr, "garage : une voiture bloquée à montrer");
    if (bloquee) garageOuvrirSur(bloquee);
    garageRegarder();
    gm.allerA(Ecran::Garage);
    images(60);
    capture("garage", "garage : bouton « Market » sous le solde, voiture bloquée (prix ★ et ≈ €)");
    verifier(appuyer("garage"), "RG-01 : bouton « Market » dessiné au garage");
    images(30);
    verifier(ouvert() && gm.ecran() == Ecran::Garage, "RG-01 : garage → Market");
    capture("garage-market", "Market ouvert depuis le garage");
    appuyer("retour"); images(20);
    verifier(!ouvert() && gm.ecran() == Ecran::Garage, "RG-01 : « Retour » → garage");
    gm.allerA(Ecran::Menu); images(30);
    // ---- hors ligne (RG-05) : packs visibles, boutons grisés, pas de plantage
    horsLigneBanc(true);
    appuyer("bouton"); images(30);
    const long long s2 = solde();
    verifier(ouvert() && appuyer("pack-3") && solde() == s2 && ouvert(), "RG-05 : hors ligne : appui sans effet, pas de plantage");
    capture("hors-ligne", "hors ligne : boutons grisés, « Connexion à Google Play nécessaire »");
    appuyer("retour"); images(10);
    horsLigneBanc(false);
    verifier(!ouvert(), "RG-05 : hors ligne : « Retour » → menu");
    gm.shutdown();
    sauvegarde().desactiver();
    std::remove(chemin.c_str());
    return ecarts;
}

} // namespace

int benchMarketEcran(Game& g, Renderer& r, const std::string& out) {
    setenv("WFS_BOUTIQUE", "simulee", 1);   // RG-08 : boutique simulée (prix de secours), avant le 1er appel à boutique()
    ui::chargerPolice(r);
    ui::langue::charger();
    int ecarts = 0;
    for (const ui::langue::Choix c : {ui::langue::Choix::Francais, ui::langue::Choix::Anglais}) {
        if (c == ui::langue::Choix::Anglais) { ui::langue::viderJournal(); ui::langue::journalActif(true, "market"); }
        ecarts += benchUneLangue(g, r, out, c);
        if (c == ui::langue::Choix::Anglais) {   // RC-E20-06 : aucune phrase française ni texte qui dépasse en anglais
            ui::langue::journalActif(false);
            const int e = ui::langue::verifierJournal();
            WFS_LOG("market (WFS-513) banc [en] : RC-E20-06 : %d texte(s) français ou qui dépasse(nt) : %s", e, e ? "ÉCART" : "OK");
            ecarts += e;
        }
    }
    ui::langue::choisir(ui::langue::Choix::Automatique);
    WFS_LOG("market (WFS-513) banc : parcours complet FR + EN : %d écart(s)%s", ecarts, ecarts ? " ÉCHEC" : " OK");
    return ecarts;
}

} // namespace wfs::ecrans::market_ecran
