// WaForSpeed — garage : rangée « Finition » (WFS-534 : emplacement ; WFS-491, épic E18 : choix de la finition, prix, aperçu).
// Voir regles-de-gestion/WFS-491.md et E18-personnalisation.md.
//   - RG-01 : sous les couleurs, 8 boutons dans l'ordre Brillant, Satiné, Mat, Métallisé, Nacré, Candy, Chromé, Caméléon ; chacun
//     porte un petit échantillon de la finition (interface 2D) ; non possédée : cadenas et prix en ★ dessous (perso::prix : base
//     × coefficient de la classe de la voiture, valeurs de configgame.json, boutique.finitions) ; à droite, le nom de la finition
//     montrée et, si elle est payante, son équivalent en médailles et en euros (RC-E18-11) ;
//   - RG-02 : un appui la montre tout de suite sur la voiture qui tourne (Renderer::finitionJoueur, rendu de WFS-490) ; l'achat
//     passe par le bouton « Acheter N ★ » de WFS-489 (AchatGarage : la finition s'ajoute à ce qui manque) ;
//   - RG-03 : une finition achetée vaut pour une seule voiture ; un choix possédé est gardé tout de suite (finition-<voiture>) ;
//     à chaque ouverture du garage, chaque voiture reprend sa dernière finition possédée (aperçu non acheté oublié) ;
//   - RG-04 : boutons de taille fixe (104 × 30 px, hauteur des pastilles des couleurs), style design-street-racing ; écran étroit (4:3) :
//     la rangée défile par des chevrons, comme celle des couleurs ; texte à 12 px, réduit de 20 % au plus (E20), jamais coupé.
// La course lit la finition dans la sauvegarde (perso::aspectCourse, Game::setCar) : une finition non possédée ne part jamais en course.
#pragma once
#include "garage_commun.h"
#include "../core/configgame.h"          // BOUTIQUE_FINITIONS_IDS
#include "../render/vehicule_finitions.h"

namespace wfs::ecrans::garage {

class FinitionGarage : public PanneauGarage {
public:
    static constexpr int NB = BOUTIQUE_FINITIONS;
    // RG-01 : ordre affiché → indice dans BOUTIQUE_FINITIONS_IDS (brillant, metallise, nacree, mat, satine, candy, chrome, cameleon)
    static constexpr int ORDRE[NB] = {0, 4, 3, 1, 2, 5, 6, 7};   // Brillant, Satiné, Mat, Métallisé, Nacré, Candy, Chromé, Caméléon
    static float hauteur(float k);   // hauteur de la rangée (boutons + ligne des prix), pixels de l'interface
    static int catalogue(int ordre) { return ordre >= 0 && ordre < NB ? ORDRE[ordre] : 0; }   // ordre affiché → catalogue
    // indice dans BOUTIQUE_FINITIONS_IDS → finition du rendu (vehicule_finitions.h) ; hors bornes : brillant
    static int rendu(int finition) {
        const int f = finition >= 0 && finition < NB ? vehicule_finitions::depuisId(BOUTIQUE_FINITIONS_IDS[finition]) : -1;
        return f < 0 ? vehicule_finitions::BRILLANT : f;
    }
    // ouverture du garage : dernière finition possédée de chaque voiture (sauvegarde active seulement), sinon brillant
    void ouvrir();
    int finition(int index) const;   // finition montrée pour cette voiture (indice dans BOUTIQUE_FINITIONS_IDS)
    static constexpr int CHEVRON_GAUCHE = NB, CHEVRON_DROIT = NB + 1;   // zones des chevrons rendues par appui()
    // appui sur la zone rendue par appui() : bouton = aperçu, gardé dans la sauvegarde si possédée ; chevron = défilement
    void appuyer(int zone, int index);
    static float largeurBouton(float k);
    // taille du texte des boutons (langue active) : 12 px × k, réduite de 20 % au plus ; faux si le plus long nom ne tient pas
    static bool police(float k, float& px);
    void dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) override;   // zones g_z.finitions, g_z.chevronsFinition
    int appui(float x, float y) const override;   // ordre (0..7) du bouton touché, CHEVRON_GAUCHE / CHEVRON_DROIT, −1 si aucun
    void viderZones() override;

private:
    int fin_[CAR_COUNT] = {};
    int premier_ = 0, visibles_ = NB;        // 1er bouton visible, nombre de boutons visibles (dernière image)
    int selVue_ = -1, indexVu_ = -1;         // sélection rendue visible à la dernière image
};

} // namespace wfs::ecrans::garage
