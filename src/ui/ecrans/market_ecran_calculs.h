// WaForSpeed — écran « Market » (WFS-513) : calculs seuls, sans rendu ni fichier, testés par tests/test_market_ecran.cpp.
//   - RG-02 / RG-03 / RG-05 : état du bouton d'achat d'un produit (actif, grisé hors ligne, « Patiente… » pendant l'achat, acheté) ;
//   - RG-02 : ordre des cartes (pack débutant en tête quand il est proposé, puis les 4 packs d'étoiles) ; base et bonus offert ;
//   - RG-03 : animation du solde qui monte (1,2 s, ralentie à la fin ; une baisse ne s'anime pas) ;
//   - RG-04 : fin de la restauration (« Achats restaurés » dès qu'un achat revient, « Aucun achat à restaurer » après 2 s) ;
//   - sécurité : un achat sans réponse rend son bouton après 60 s (Google qui ne répond jamais : le joueur n'est pas bloqué).
#pragma once
#include <algorithm>
#include <cmath>

namespace wfs::ecrans::market_ecran::calcul {

constexpr double DUREE_SOLDE = 1.2, DELAI_RESTAURATION = 2.0, DELAI_ACHAT = 60.0;

// ---- RG-02, RG-03, RG-05 : bouton d'achat
enum class Bouton { Actif, Grise, Patiente, Achete };
// possede : non consommable déjà à soi ; achatEnCours : une réponse est attendue (tous les boutons attendent) ;
// ceProduit : c'est ce produit qui attend ; prixConnu : prix donné par la boutique (vide : hors ligne ou produit introuvable)
inline Bouton etatBouton(bool possede, bool achatEnCours, bool ceProduit, bool prixConnu) {
    if (possede) return Bouton::Achete;
    if (achatEnCours && ceProduit) return Bouton::Patiente;
    if (achatEnCours || !prixConnu) return Bouton::Grise;
    return Bouton::Actif;
}

// ---- RG-02 : ordre des cartes ; ordre[] reçoit les indices du catalogue (pack débutant = nbEtoiles) ; rend leur nombre
inline int ordreCartes(bool debutantPropose, int nbEtoiles, int* ordre) {
    int n = 0;
    if (debutantPropose) ordre[n++] = nbEtoiles;
    for (int i = 0; i < nbEtoiles; i++) ordre[n++] = i;
    return n;
}
// étoiles offertes à montrer en petit (« 600 ★ + 120 offertes ») ; 0 : pas de ligne (pack sans bonus)
inline long long etoilesOffertes(long long base, long long total) { return total > base && base >= 0 ? total - base : 0; }

// ---- RG-03 : solde affiché ; animation de deb à fin, commencée à t0 (secondes)
inline float ralenti(float t) { t = std::clamp(t, 0.f, 1.f); return 1 - (1 - t) * (1 - t) * (1 - t); }
struct Solde {
    bool pret = false;
    long long deb = 0, fin = 0;
    double t0 = 0;
    long long valeur(double t) const {
        return deb + (long long)std::llround((double)(fin - deb) * ralenti((float)((t - t0) / DUREE_SOLDE)));
    }
    // nouvelle valeur du portefeuille à l'instant t ; rend le solde à dessiner
    long long suivre(long long cible, double t) {
        if (!pret) { pret = true; deb = fin = cible; t0 = t; }
        if (cible != fin) {
            deb = cible > fin ? valeur(t) : cible;   // une hausse repart du solde affiché ; une baisse (achat au garage) est immédiate
            fin = cible; t0 = t;
        }
        return valeur(t);
    }
};

// ---- RG-04 : restauration
enum class Restauration { EnCours, Restaures, Aucun };
inline Restauration finRestauration(int rendus, double ecoule) {
    if (rendus > 0) return Restauration::Restaures;
    return ecoule >= DELAI_RESTAURATION ? Restauration::Aucun : Restauration::EnCours;
}

// ---- achat sans réponse : bouton rendu après DELAI_ACHAT
inline bool achatOublie(double ecoule) { return ecoule >= DELAI_ACHAT; }

} // namespace wfs::ecrans::market_ecran::calcul
