// WaForSpeed — garage : rangée « Skin » (WFS-534 : emplacement ; WFS-494, épic E18 : skins, skin offerte, prix, skins du cadeau).
// Voir regles-de-gestion/WFS-494.md et E18-personnalisation.md ; règles de la skin offerte, de donnerSkin et des bons : src/game/skin_offerte.h.
//   - RG-01 : sous les finitions, 5 boutons dans l'ordre Aucune, Rallye, Camouflage, Drift Tokyo, Néon nuit, seulement pour les voitures
//     qui ont des skins (6 voitures) ; chacun porte un échantillon des deux couleurs de la skin (interface 2D) ; non possédée : cadenas
//     et prix en ★ dessous (perso::prix : base × coefficient de la classe, configgame.json) ; à droite, le nom de la skin montrée et
//     son équivalent en médailles et en euros (RC-E18-11) ;
//   - RG-04 : Néon nuit n'est jamais en vente : cadenas, « Cadeau » sous le bouton et « Bientôt dans le cadeau du jour » à droite,
//     tant qu'elle n'est pas possédée (donnerSkin) ;
//   - RG-05 : un appui montre la skin tout de suite sur la voiture qui tourne (vehicule_skin::poser sur le modèle du garage, une seule
//     texture d'habillage en mémoire) ; l'achat passe par « Acheter N ★ » (AchatGarage : la skin s'ajoute à ce qui manque) ;
//     une skin possédée est gardée tout de suite (skin-<voiture>) ; à chaque ouverture du garage, chaque voiture reprend sa dernière
//     skin possédée ; en sortant, la voiture de la course reprend sa skin possédée (aspectCourse) ;
//   - RG-02 : fenêtre « Choisis ta skin offerte » (skins en vente, valeur en ★, aperçu sur la voiture, « Plus tard » / « Prendre ») :
//     ouverte d'elle-même juste après l'achat d'une voiture qui a des skins ; pour une voiture déjà possédée qui attend sa skin offerte
//     (208, 500, GT86 dès le départ ; « Plus tard »), bouton « Choisis ta skin offerte » à droite de la rangée (jamais de fenêtre
//     surprise à l'ouverture du garage : le joueur qui vient choisir sa voiture n'est pas arrêté) ;
//   - bon de skin du market (pack débutant) : « Utiliser un bon » à droite, à la place du prix, sur une skin en vente non possédée.
// Boutons de taille fixe (104 × 30 px, comme les finitions), style design-street-racing ; écran étroit : la rangée défile par des chevrons.
#pragma once
#include "garage_commun.h"
#include "../core/configgame.h"   // BOUTIQUE_SKINS_IDS

namespace wfs {
struct Model;
namespace ecrans::garage {

class SkinGarage : public PanneauGarage {
public:
    static constexpr int NB = BOUTIQUE_SKINS + 1;   // « Aucune » puis les 5 skins (WFS-552)
    SkinGarage() { for (int& s : skin_) s = -1; }
    // ordre affiché → indice dans BOUTIQUE_SKINS_IDS (rallye, drifttokyo, neonnuit, camouflage, ciel) ; −1 = aucune
    static constexpr int ORDRE[NB] = {-1, 0, 3, 1, 2, 4};   // Aucune, Rallye, Camouflage, Drift Tokyo, Néon nuit, Bleu ciel (WFS-552)
    static int catalogue(int ordre) { return ordre >= 0 && ordre < NB ? ORDRE[ordre] : -1; }
    static float hauteur(float k);       // hauteur de la rangée (boutons + ligne des prix + écart), pixels de l'interface
    static bool visible(int index);      // la voiture (indice dans CARS[]) a des skins : la rangée est montrée (RG-01)
    static float largeurBouton(float k);
    static bool police(float k, float& px);   // 12 px × k réduite de 20 % au plus ; faux si le plus long nom ne tient pas

    void ouvrir();                       // ouverture du garage : dernière skin possédée de chaque voiture, fenêtre pas encore proposée
    int skin(int index) const;           // skin choisie au garage pour cette voiture (indice dans BOUTIQUE_SKINS_IDS), −1 = aucune
    static constexpr int CHEVRON_GAUCHE = NB, CHEVRON_DROIT = NB + 1, ACTION = NB + 2;   // zones rendues par appui()
    // appui sur la zone rendue par appui() : bouton = aperçu, gardé si possédée ; chevron = défilement ; ACTION = « Choisis ta skin
    // offerte » (fenêtre ouverte) ou « Utiliser un bon » (bon utilisé sur la skin montrée)
    void appuyer(int zone, int index);

    // RG-05 : aperçu de la skin montrée sur le modèle du garage (avant de le dessiner) ; ne recharge que si la skin, la voiture ou le
    // modèle changent (une seule texture d'habillage en mémoire)
    void apercu(Renderer& r, const Model& m, int index);
    // sortie du garage : si l'aperçu a posé une texture, la voiture de la course reprend sa skin possédée (reposer = vrai), sinon
    // la texture est seulement rendue (« Choisir » : Game::setCar la pose ensuite)
    void quitter(Renderer& r, const GameView& g, bool reposer);

    // RG-02 : ouvre la fenêtre de la skin offerte si la voiture attend sa skin offerte (achat d'une voiture, bouton de la rangée)
    void proposer(int index);
    bool fenetreOuverte() const { return fenetre_; }
    bool toucherFenetre(float x, float y, int index);   // appui (doigt posé) quand la fenêtre est ouverte ; vrai si un bouton est touché
    void dessinerFenetre(Renderer& r, const MiseEnPage& mp, int index);
    // banc : centre d'un bouton de la fenêtre (« plustard », « prendre ») ou d'une skin de la fenêtre (« skin0 », « skin1 »…)
    bool boutonFenetre(const char* nom, float& x, float& y) const;

    void dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) override;   // zones g_z.skins, g_z.chevronsSkin, g_z.actionSkin
    int appui(float x, float y) const override;   // ordre (0..4) du bouton touché, CHEVRON_GAUCHE / CHEVRON_DROIT, ACTION, −1 si aucun
    void viderZones() override;

private:
    int skin_[CAR_COUNT];
    int premier_ = 0, visibles_ = NB;        // 1er bouton visible, nombre de boutons visibles (dernière image)
    int selVue_ = -1, indexVu_ = -1;         // sélection rendue visible à la dernière image
    bool pose_ = false;                      // l'aperçu a posé une texture d'habillage (rendue en sortant du garage)
    // fenêtre de la skin offerte
    bool fenetre_ = false; int fenetreIndex_ = -1, choixFenetre_ = 0, skinAvant_ = -1;
    Zone zSkinsFenetre_[BOUTIQUE_SKINS], zPlusTard_, zPrendre_;
};

} // namespace ecrans::garage
} // namespace wfs
