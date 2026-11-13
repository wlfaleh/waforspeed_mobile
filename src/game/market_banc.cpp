// WaForSpeed — banc du market (WFS-510, RG-09) : WFS_BANC=market, sans rendu ; voir market.h.
// Fichier à part (moteur et banc seulement) : il lit configgame.json (chargerConfig), que les tests CPU ne lient pas tous.
// Sauvegarde du banc en mémoire seulement (jamais écrite sur le disque) ; boutique simulée.
#include <filesystem>
#include <string>
#include <vector>
#include "../core/configgame.h"
#include "../core/log.h"
#include "boutique.h"
#include "market.h"

namespace wfs::market {

int benchMarket(const std::string& out) {
    const BilanConfig bc = chargerConfig();   // PC : WFS_CONFIG, sinon ./configgame.json (un pack changé compte sans recompiler)
    std::vector<std::string> lignes;
    int faux = 0;
    auto ligne = [&](const std::string& l) { lignes.push_back(l); };
    auto verifier = [&](bool ok, const std::string& quoi) { if (!ok) { faux++; ligne("MARKET FAUX : " + quoi); } };
    ligne("MARKET : configuration " + (bc.fichier.empty() ? std::string("du code (aucun fichier)") : bc.fichier) + " (" + std::to_string(bc.lues) + " clés lues, " +
          std::to_string(bc.erreurs) + " erreur(s))");

    // 1) catalogue (RG-01)
    for (const Pack& p : catalogue()) {
        std::string t = "MARKET catalogue : " + p.id + " — " + p.nomFr + " / " + p.nomEn + " : ";
        if (p.consommable) t += std::to_string(p.etoilesBase) + " ★ + " + std::to_string(p.offert) + " % = " + std::to_string(p.total) + " ★ crédités";
        else if (p.total > 0 || p.bonsSkin > 0) t += std::to_string(p.total) + " ★ + " + std::to_string(p.bonsSkin) + " bon(s) de skin, une seule fois";
        else t += "plus de proposition de vidéo, ×2 direct, une seule fois";
        t += " (prix de secours PC : " + texteEurosSecours(p.prixSecours) + ")";
        ligne(t);
    }

    Sauvegarde s; s.defauts();   // inactive : rien n'est écrit sur le disque
    BoutiqueSimulee b;
    std::string dernierMessage;
    brancher(b, s, [&](const std::string& produit, const Traitement& t) {
        ligne("MARKET   " + produit + " : " + (t.credit == Credit::Inconnu && t.message.empty() && !t.confirmer ? std::string("rien de crédité") : std::string(texte(t.credit))) +
              (t.confirmer ? ", confirmé auprès de la boutique" : "") + (t.message.empty() ? "" : " ; message « " + t.message + " »"));
        dernierMessage = t.message;
    });
    auto solde = [&](const std::string& etape) { ligne("MARKET solde après « " + etape + " » : " + std::to_string(s.etoiles) + " ★"); };

    // 2) achat de chaque pack d'étoiles (RG-03)
    long long attendu = 0;
    for (int i = 0; i < MARKET_ETOILES; i++) {
        b.acheter(MARKET_PRODUITS_IDS[i]); attendu += pack(i).total;
        solde(std::string("achat ") + MARKET_PRODUITS_IDS[i]);
    }
    verifier(s.etoiles == attendu, "4 packs : " + std::to_string(s.etoiles) + " ★ au lieu de " + std::to_string(attendu));

    // 3) même jeton deux fois
    const long long avantDouble = s.etoiles;
    b.doubler(true); b.acheter("etoiles_argent");
    solde("argent, même jeton reçu deux fois");
    verifier(s.etoiles == avantDouble + pack(1).total, "même jeton deux fois : crédité deux fois");

    // 4) en attente, annulé
    const long long avantAttente = s.etoiles;
    b.repondre(BoutiqueSimulee::Reponse::EnAttente); b.acheter("etoiles_or"); solde("or en attente de paiement");
    verifier(s.etoiles == avantAttente && dernierMessage == "Achat en attente de paiement", "achat en attente : crédité ou sans message");
    b.repondre(BoutiqueSimulee::Reponse::Annule); b.acheter("etoiles_coffre"); solde("coffre annulé");
    verifier(s.etoiles == avantAttente, "achat annulé : crédité");

    // 5) achat interrompu (crédit écrit, confirmation pas faite) puis relance
    b.repondre(BoutiqueSimulee::Reponse::Interrompu); b.acheter("etoiles_bronze");
    const long long apresCoupure = s.etoiles;
    solde("bronze, application coupée avant la confirmation");
    ligne("MARKET   achats pas encore confirmés : " + std::to_string(b.nonConfirmes()));
    b.repondre(BoutiqueSimulee::Reponse::Achete);
    b.chargerProduits();   // lancement suivant : rattrapage
    solde("relance du jeu (rattrapage)");
    ligne("MARKET   achats pas encore confirmés : " + std::to_string(b.nonConfirmes()));
    verifier(apresCoupure == avantAttente + pack(0).total && s.etoiles == apresCoupure && b.nonConfirmes() == 0, "achat interrompu : pas crédité une seule fois ou pas confirmé à la relance");

    // 6) pack débutant (RG-02) et sans pub (RG-05)
    s.profil.courses = 2;
    const bool apres2 = packDebutantPropose(s, true);
    s.profil.courses = 3;
    const bool apres3 = packDebutantPropose(s, true);
    ligne(std::string("MARKET pack débutant après 2 courses : ") + (apres2 ? "proposé" : "pas proposé") + " ; après 3 courses : " + (apres3 ? "proposé" : "pas proposé") +
          " (si des skins sont en vente ; aujourd'hui : " + (SKINS_EN_VENTE ? "oui" : "non, pack caché") + ")");
    verifier(!apres2 && apres3, "pack débutant : pas proposé à partir de la 3e course");
    b.acheter("pack_debutant"); solde("pack débutant");
    ligne("MARKET   bons de skin : " + std::to_string(s.market.bonsSkin) + " ; pack débutant encore proposé : " + (packDebutantPropose(s, true) ? "oui" : "non"));
    verifier(!packDebutantPropose(s, true) && s.market.bonsSkin == pack(MARKET_ETOILES).bonsSkin, "pack débutant encore proposé après l'achat");
    b.acheter("sans_pub"); solde("sans pub");
    const long long avantRestaurer = s.etoiles;
    b.restaurer(); solde("restaurer mes achats");
    verifier(sansPub(s) && s.etoiles == avantRestaurer, "restaurer : étoiles créditées une 2e fois");

    // 7) ×2 de fin de course (RG-06) : 7 demandes le même jour, minuit, heure qui recule
    Sauvegarde x; x.defauts();
    const int gain = config().boutique.gainCourse, jour = 20261113, lendemain = 20261114;
    long long course = 0;
    int accordes = 0;
    for (int k = 1; k <= 7; k++) {
        const int n = recompenseVideo(x, gain, ++course, jour);
        accordes += n > 0;
        ligne("MARKET ×2 n° " + std::to_string(k) + " le 13/11 (course " + std::to_string(course) + ") : " + (n > 0 ? "accordé, +" + std::to_string(n) + " ★" : std::string("refusé")) +
              " ; solde " + std::to_string(x.etoiles) + " ★, " + std::to_string(x2Restants(x, jour)) + " restant(s)");
    }
    verifier(accordes == config().market.x2ParJour, "×2 accordés le même jour : " + std::to_string(accordes));
    const int apresMinuit = recompenseVideo(x, gain, ++course, lendemain);
    ligne("MARKET ×2 après minuit (14/11, course " + std::to_string(course) + ") : " + (apresMinuit > 0 ? "accordé" : "refusé") + ", " + std::to_string(x2Restants(x, lendemain)) + " restant(s)");
    const int deuxFois = recompenseVideo(x, gain, course, lendemain);
    ligne(std::string("MARKET ×2 une 2e fois pour la même course : ") + (deuxFois > 0 ? "accordé" : "refusé"));
    for (int k = 0; k < 4; k++) recompenseVideo(x, gain, ++course, lendemain);   // 5 ×2 le 14/11
    const int recul = recompenseVideo(x, gain, ++course, jour);   // l'heure du téléphone recule au 13/11
    ligne("MARKET ×2 heure du téléphone reculée au 13/11 après 5 ×2 le 14/11 : " + std::string(recul > 0 ? "accordé" : "refusé") + " (compteur pas remis à zéro)");
    verifier(apresMinuit > 0 && deuxFois == 0 && recul == 0, "×2 : minuit, même course ou heure reculée");
    Sauvegarde y; y.defauts(); y.market.sansPub = true;
    int direct = 0; long long c2 = 0;
    for (int k = 0; k < 7; k++) direct += x2SansPub(y, gain, ++c2, jour) > 0;
    ligne("MARKET ×2 direct avec « sans pub » : " + std::to_string(direct) + " accordés sur 7 courses le même jour");
    verifier(direct == config().market.x2ParJour, "×2 avec sans pub");

    ligne(faux == 0 ? "MARKET : OK (chaque achat crédité une seule fois)" : "MARKET : ÉCHEC (" + std::to_string(faux) + " résultat(s) faux)");
    std::string texteFichier;
    for (const std::string& l : lignes) { WFS_LOG("%s", l.c_str()); texteFichier += l + "\n"; }
    std::error_code ec; std::filesystem::create_directories(out, ec);
    const std::string chemin = out + "/market.txt";
    if (!ecrireAtomique(chemin, texteFichier)) WFS_ERR("market : %s non écrit", chemin.c_str());
    return faux;
}

} // namespace wfs::market
