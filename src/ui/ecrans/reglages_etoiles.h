// WaForSpeed — mode testeur (WFS-546) : bouton étoile (+2500 ★) du menu caché de diagnostic des Réglages.
// Calculs seuls (sans dessin), testés par tests/test_reglages_etoiles.cpp :
//   - RG-02 : chaque appui ajoute exactement 2500 ★ par boutique::crediter (aucun nouveau calcul de solde) ;
//   - un appui = un seul crédit : sur Android, un doigt posé qui bouge envoie d'autres événements « posé » (ACTION_MOVE,
//     down = vrai) avec le même identifiant ; ils ne recréditent pas. Le doigt est libéré quand il se lève.
// Thread principal (comme la sauvegarde). La sauvegarde n'est pas écrite ici : l'appelant appelle enregistrer() (RG-03).
#pragma once
#include "../../game/boutique.h"

namespace wfs::ecrans::testeur {

constexpr long long CREDIT_ETOILES = 2500;   // RG-02 : demande de l'utilisateur, « 2500 étoiles pour chaque clic »

class BoutonEtoiles {
public:
    // doigt « id » posé (ou qui bouge) ; dansBouton : il est sur le bouton. Vrai si 2500 ★ viennent d'être créditées.
    bool poser(int id, bool dansBouton, Sauvegarde& s) {
        if (id == doigt_) return false;   // même doigt encore posé : déjà crédité
        if (!dansBouton) return false;
        doigt_ = id;
        boutique::crediter(s, CREDIT_ETOILES, "mode testeur");
        return true;
    }
    void lever(int id) { if (id == doigt_) doigt_ = -1; }
    void oublier() { doigt_ = -1; }   // menu fermé ou changement d'écran (lever perdu)
private:
    int doigt_ = -1;
};

} // namespace wfs::ecrans::testeur
