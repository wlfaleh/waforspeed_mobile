// WaForSpeed — garage : rangée des couleurs (WFS-534, sortie de garage.cpp ; WFS-489, épic E18 : 28 couleurs, couleur secondaire,
// cadenas et prix). Voir regles-de-gestion/WFS-489.md.
//   - deux onglets « Principale » et « Toit et rétros » (RG-02), puis les pastilles (taille inchangée : 30 × 30, skewX(-10deg), RG-01) ;
//     en « Toit et rétros », une pastille « Comme la carrosserie » en premier ;
//   - quand la place manque, la rangée défile au doigt (glisser) ou par les chevrons à ses bords ; seules des pastilles entières
//     sont dessinées (pas de découpe) ;
//   - pastille non possédée : petit cadenas et prix en ★ dessous (prix de perso::prix : base × coefficient de la classe, RG-03) ;
//     un appui la montre tout de suite sur la voiture (aperçu, rien n'est débité) ; l'achat est fait par AchatGarage ;
//   - choix possédé : gardé tout de suite dans la sauvegarde (paint-<voiture>, paint2-<voiture>, RG-04) ;
//   - à chaque ouverture du garage, chaque voiture reprend sa dernière couleur possédée (aperçu non acheté oublié, RG-03).
#pragma once
#include "garage_commun.h"

namespace wfs {
struct Touch;
namespace ecrans::garage {

class CouleurGarage : public PanneauGarage {
public:
    CouleurGarage() { for (int& c : second_) c = -1; }
    static float hauteur(float k);   // hauteur de la rangée (pastille + ligne des prix), pixels de l'interface
    // ouverture du garage : peinture[i] = dernière couleur possédée de chaque voiture (sauvegarde active seulement),
    // couleurs secondaires lues dans la sauvegarde, onglet « Principale »
    void ouvrir(int (&peinture)[CAR_COUNT]);
    // toucher de la rangée : 0 pas pour elle ; 1 appui pris (onglet, chevron : le garage retient le doigt) ; 2 doigt suivi
    // (glisser = défiler, relâcher sans glisser = choisir la pastille). libre : aucun autre doigt n'agit dans le garage.
    int toucher(const Touch& t, bool libre, int index, int& peinture);
    int peinture2(int index) const;   // couleur secondaire montrée (indice dans PAINTS), −1 = comme la carrosserie
    float code2(int index) const;     // code de la couleur 2 pour Renderer::peinture2ProchaineVoiture (vehicule_peinture2.h)
    bool toit() const { return toit_; }
    void dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) override;   // zones g_z.peintures, toit, onglets, chevrons
    int appui(float x, float y) const override;   // indice dans PAINTS de la pastille principale touchée, −1 si aucune
    void viderZones() override;

private:
    int nbItems() const { return toit_ ? NB_PEINTURES + 1 : NB_PEINTURES; }
    int itemSous(float x, float y) const;   // pastille visible sous le doigt, −1 sinon
    void choisirItem(int item, int index, int& peinture);
    bool toit_ = false;
    int second_[CAR_COUNT] = {};   // couleur secondaire de chaque voiture (indice dans PAINTS), −1 = comme la carrosserie
    int premier_ = 0, visibles_ = 0;   // 1re pastille visible, nombre de pastilles visibles (dernière image)
    int selVue_ = -1, indexVu_ = -1; bool toitVu_ = false;   // sélection rendue visible à la dernière image
    float pas_ = 1;                // pas entre deux pastilles (dernière image)
    Zone rangee_, items_[NB_PEINTURES + 1];   // rangée visible, pastilles de l'onglet ouvert
    int doigt_ = -1, itemAppui_ = -1, premier0_ = 0; float x0_ = 0; bool glisse_ = false;
};

} // namespace ecrans::garage
} // namespace wfs
