// WaForSpeed — garage : colonne d'action du panneau (WFS-534, sortie de garage.cpp ; WFS-489, épic E18 : achat des couleurs).
//   - « Choisir » (« Chargement… » tant que le modèle 3D n'est pas prêt) et « Retour » ;
//   - couleur montrée non possédée (principale, droit « Toit et rétros » ou couleur secondaire) : « Acheter N ★ » à la place de
//     « Choisir », même taille (N = total de ce qui manque, perso::prix) ; le solde d'étoiles est écrit au-dessus ;
//   - appui sur « Acheter » : solde suffisant → tout ce qui manque est acheté (boutique::acheterObjet, débit une seule fois
//     chacun), le choix est gardé (paint-, paint2-) et la sauvegarde écrite ; sinon bandeau « Il te manque N ★ », rien n'est
//     débité (le bandeau reste tant que la même couleur est montrée).
// WFS-491 : la finition montrée non possédée s'ajoute à ce qui manque (même bouton « Acheter N ★ », même refus).
// Emplacement suivant : voitures à acheter (WFS-499), skins (WFS-494).
// Outils partagés avec la rangée des couleurs : étoile dessinée (la police n'a pas le caractère ★), texte avec ★, équivalents.
#pragma once
#include <string>
#include "garage_commun.h"
#include "../core/configgame.h"   // WFS-491 : BOUTIQUE_FINITIONS_IDS
#include "../game/personnalisation.h"

namespace wfs::ecrans::garage {

// ce qui manque pour que l'aspect montré parte en course (WFS-489) : objets à acheter pour cette voiture et leur total en ★
struct Manque {
    const perso::Objet* objets[4] = {};   // WFS-491 : + la finition
    int n = 0;
    long long total = 0;
};
Manque manque(const Sauvegarde& s, const EtatGarage& e);
// prix en ★ d'une pastille de la rangée pour cette voiture : principale (couleur seule) ou secondaire (droit + couleur), 0 si possédée
long long prixPastille(const Sauvegarde& s, const char* carId, int peinture, bool secondaire);

// étoile pleine à 5 branches (interface 2D : icône de la monnaie), centrée en (cx, cy), rayon extérieur R
void etoile(Renderer& r, float cx, float cy, float R, const vec4& c);
// ui::text où chaque « ★ » est une étoile dessinée ; rend la largeur
float texteEtoiles(Renderer& r, float x, float y, float px, const std::string& s, const vec4& c, int align, float espacement = 0);
float largeurEtoiles(float px, const std::string& s, float espacement = 0);
// « 75 ★ = 1 bronze = 0,74 € » dans la langue active (RC-E18-11 : équivalent en médailles et en euros)
std::string texteEquivalent(long long prix);

class AchatGarage : public PanneauGarage {
public:
    enum Bouton { AUCUN = 0, CHOISIR = 1, RETOUR = 2 };   // WFS-115 : mêmes numéros que le bouton appuyé de garage.cpp
    void dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) override;   // zones g_z.choisir et g_z.retour
    int appui(float x, float y) const override;   // Bouton touché (CHOISIR, RETOUR), −1 si aucun
    void viderZones() override;
    // relâcher sur CHOISIR : vrai si le bouton était « Acheter » (achat fait ou refusé : le garage ne choisit pas)
    bool acheter();
    long long manqueAffiche() const { return manque_; }   // banc : N du bandeau « Il te manque N ★ » (0 : pas de bandeau)

private:
    EtatGarage e_;                  // état montré à la dernière image (l'achat porte sur ce que le joueur voit)
    long long manque_ = 0;          // N du bandeau, 0 sans bandeau
    EtatGarage eManque_;            // état au moment du refus : le bandeau disparaît quand la couleur montrée change
};

} // namespace wfs::ecrans::garage
