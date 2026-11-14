// WaForSpeed — tests de l'écran « Market » (WFS-513) : état des boutons d'achat (actif, grisé hors ligne, « Patiente… », acheté),
// ordre des cartes (pack débutant en tête), étoiles offertes, solde qui monte (1,2 s, baisse immédiate), fin de la
// restauration (« Achats restaurés » / « Aucun achat à restaurer » après 2 s), achat sans réponse rendu après 60 s.
#include <cstdio>
#include "../src/ui/ecrans/market_ecran_calculs.h"

using namespace wfs::ecrans::market_ecran::calcul;

static int g_echecs = 0;
#define VERIFIE(c) do { if (!(c)) { std::printf("ÉCHEC %s:%d : %s\n", __FILE__, __LINE__, #c); g_echecs++; } } while (0)

int main() {
    // ---- RG-02, RG-03, RG-05 : boutons
    VERIFIE(etatBouton(false, false, false, true) == Bouton::Actif);      // prix de Google connu : achat possible
    VERIFIE(etatBouton(false, false, false, false) == Bouton::Grise);     // RG-05 : hors ligne : grisé
    VERIFIE(etatBouton(false, true, true, true) == Bouton::Patiente);     // RG-03 : ce produit attend sa réponse
    VERIFIE(etatBouton(false, true, false, true) == Bouton::Grise);       // RG-03 : un autre achat attend : pas de 2e achat
    VERIFIE(etatBouton(true, false, false, true) == Bouton::Achete);      // RG-02 : « Sans pub » possédé
    VERIFIE(etatBouton(true, false, false, false) == Bouton::Achete);     // possédé reste « Acheté » même hors ligne
    // ---- RG-02 : ordre des cartes
    int o[5] = {-1, -1, -1, -1, -1};
    VERIFIE(ordreCartes(false, 4, o) == 4 && o[0] == 0 && o[3] == 3 && o[4] == -1);
    VERIFIE(ordreCartes(true, 4, o) == 5 && o[0] == 4 && o[1] == 0 && o[4] == 3);   // pack débutant en tête
    // étoiles offertes (« 600 ★ + 120 offertes ») ; bronze sans bonus : pas de ligne
    VERIFIE(etoilesOffertes(600, 720) == 120);
    VERIFIE(etoilesOffertes(3600, 4500) == 900);
    VERIFIE(etoilesOffertes(100, 100) == 0);
    VERIFIE(etoilesOffertes(100, 90) == 0);
    // ---- RG-03 : solde qui monte
    Solde s;
    VERIFIE(s.suivre(1250, 10.0) == 1250);                                // 1re image : valeur exacte, pas d'animation
    VERIFIE(s.suivre(1250, 10.5) == 1250);
    VERIFIE(s.suivre(1970, 11.0) == 1250);                                // achat : part du solde affiché
    const long long mi = s.suivre(1970, 11.0 + DUREE_SOLDE / 2);
    VERIFIE(mi > 1250 && mi < 1970);                                      // monte
    VERIFIE(mi > 1250 + (1970 - 1250) / 2);                               // ralentie à la fin (plus de la moitié à mi-temps)
    long long avant = 1250; bool monte = true;
    for (int i = 0; i <= 80; i++) { const long long v = s.suivre(1970, 11.0 + i / 60.0); monte = monte && v >= avant; avant = v; }
    VERIFIE(monte);                                                       // jamais de recul
    VERIFIE(s.suivre(1970, 11.0 + DUREE_SOLDE) == 1970);                  // arrivé au bout en 1,2 s
    VERIFIE(s.suivre(1970, 30.0) == 1970);
    // 2e achat pendant l'animation : repart du solde affiché, sans saut en arrière
    Solde d;
    d.suivre(100, 0.0); d.suivre(200, 1.0);
    const long long v1 = d.suivre(200, 1.3);
    VERIFIE(d.suivre(300, 1.3) == v1);
    VERIFIE(d.suivre(300, 1.3 + DUREE_SOLDE) == 300);
    // baisse (achat au garage) : immédiate
    VERIFIE(d.suivre(50, 5.0) == 50);
    // ---- RG-04 : restauration
    VERIFIE(finRestauration(0, 0.0) == Restauration::EnCours);
    VERIFIE(finRestauration(0, 1.99) == Restauration::EnCours);
    VERIFIE(finRestauration(0, DELAI_RESTAURATION) == Restauration::Aucun);
    VERIFIE(finRestauration(1, 0.1) == Restauration::Restaures);         // dès qu'un achat revient
    VERIFIE(finRestauration(2, 5.0) == Restauration::Restaures);
    // ---- achat sans réponse
    VERIFIE(!achatOublie(59.9));
    VERIFIE(achatOublie(DELAI_ACHAT));
    if (g_echecs == 0) std::printf("test_market_ecran : tout est bon\n");
    return g_echecs == 0 ? 0 : 1;
}
