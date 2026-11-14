// WaForSpeed — garage : colonne d'action du panneau (WFS-534, sortie de garage.cpp ; WFS-489, épic E18 : achat des couleurs ;
// WFS-499 : achat des voitures et essai gratuit).
//   - « Choisir » (« Chargement… » tant que le modèle 3D n'est pas prêt) et « Retour » ;
//   - couleur montrée non possédée (principale, droit « Toit et rétros » ou couleur secondaire) : « Acheter N ★ » à la place de
//     « Choisir », même taille (N = total de ce qui manque, perso::prix) ;
//   - appui sur « Acheter » : solde suffisant → tout ce qui manque est acheté (boutique::acheterObjet, débit une seule fois
//     chacun), le choix est gardé (paint-, paint2-) et la sauvegarde écrite ; sinon bandeau « Il te manque N ★», rien n'est
//     débité (le bandeau reste tant que la même couleur est montrée).
// WFS-491 : la finition montrée non possédée s'ajoute à ce qui manque (même bouton « Acheter N ★ », même refus).
//   - WFS-499 : solde d'étoiles toujours écrit en haut à droite du garage (RG-01) ;
//   - WFS-499 : voiture bloquée (sauvegarde active, vendue, ni gratuite ni achetée) : cadenas sur son point (vignette) et carte
//     « cadenas · 2 200 ★ · ≈ 3 or + 1 argent + 1 bronze · ≈ 21,78 € » au-dessus du panneau ; « Acheter 2 200 ★ » ouvre la
//     fenêtre de confirmation (Annuler / Confirmer ; solde insuffisant : « Il te manque N ★ », pas de Confirmer) ;
//     « Essai gratuit » (une course solo, jamais en duel) puis « Essai déjà fait » ; la voiture ne part pas en course autrement.
//   - WFS-502 : voiture prêtée par le cadeau du jour : en attente, « Activer le prêt » à la place de l'essai (temps restant pour
//     l'activer sur la carte) ; prêt actif, carte « Voiture prêtée · Prêtée : 2 h 41 min · Garde-la pour toujours : N ★ ».
// Emplacements suivants : skins (WFS-494), skin offerte à l'achat d'une voiture (WFS-494, RG-04 de WFS-499).
// Outils partagés avec la rangée des couleurs : étoile dessinée (la police n'a pas le caractère ★), texte avec ★, équivalents, cadenas.
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

// WFS-499 (RG-01) : voiture bloquée = sauvegarde active, voiture vendue, ni gratuite ni achetée. Sans sauvegarde (bancs d'essai
// des autres tickets, parcours de la CI), aucune voiture n'est bloquée : garage et courses comme avant.
bool voitureBloquee(const Sauvegarde& s, const char* carId);
// WFS-499 (RG-03) : la voiture affichée peut partir en essai gratuit (bloquée, essai jamais fait, pas en duel)
bool essaiPossible(const Sauvegarde& s, const char* carId);

// étoile pleine à 5 branches (interface 2D : icône de la monnaie), centrée en (cx, cy), rayon extérieur R
void etoile(Renderer& r, float cx, float cy, float R, const vec4& c);
// cadenas : garage_commun.h (WFS-491) ; WFS-499 : aussi les voitures bloquées
// ui::text où chaque « ★ » est une étoile dessinée ; rend la largeur
float texteEtoiles(Renderer& r, float x, float y, float px, const std::string& s, const vec4& c, int align, float espacement = 0);
float largeurEtoiles(float px, const std::string& s, float espacement = 0);
// WFS-499 (RG-05) : nombre avec le séparateur des milliers de la langue active : « 2 200 » (espace fine) / « 2,200 »
std::string nombreLangue(long long v);
// WFS-499 (RG-05) : « 3 or + 1 argent + 1 bronze » / « 3 gold + 1 silver + 1 bronze » ; « 21,78 € » / « €21.78 »
std::string texteMedailles(long long prix);
std::string texteEuros(long long prix);
// « 75 ★ = 1 bronze = 0,74 € » dans la langue active (RC-E18-11 : équivalent en médailles et en euros)
std::string texteEquivalent(long long prix);
// WFS-499 (RG-02) : « Acheter la Draven Vantor GT pour 2 200 ★ ? », « Acheter l'Orion Corsaire pour 250 ★ ? » / « Buy the … for … ★? »
std::string questionAchat(const std::string& nom, long long prix);
// WFS-499 (RG-01) : « ≈ 3 or + 1 argent + 1 bronze · ≈ 21,78 € »
std::string texteEquivalentVoiture(long long prix);
// WFS-502 (RG-03) : durée d'un prêt dans la langue active : « 2 h 41 min », « 41 min » (minutes arrondies au-dessus)
std::string texteDuree(long long secondes);

// WFS-499 (RG-02) : fenêtre de confirmation de l'achat d'une voiture, commune au garage et à l'écran de fin de l'essai.
// « Acheter la Draven Vantor GT pour 2 200 ★ ? » · Annuler / Confirmer ; solde insuffisant : « Il te manque N ★ », Annuler seul.
// Aucun achat sans Confirmer. Mêmes tailles de boutons que le garage (Confirmer 200 × 52, Annuler 200 × 46, en 720 p).
class FenetreAchatVoiture {
public:
    enum Choix { RIEN = -1, DEHORS = 0, ANNULER = 1, CONFIRMER = 2 };
    void ouvrir(int index);   // indice dans CARS[]
    void fermer() { ouverte_ = false; zAnnuler_ = zConfirmer_ = Zone{}; }
    bool ouverte() const { return ouverte_; }
    int voiture() const { return index_; }
    // fond assombri plein écran et fenêtre centrée dans la zone sûre (pixels de l'interface, k = H / 720)
    void dessiner(Renderer& r, float W, float H, float k, float gl, float gr, float gt, float gb);
    int appui(float x, float y) const;   // Choix touché (RIEN si la fenêtre est fermée)
    // Confirmer : débit une seule fois (boutique::acheterVoiture), voiture gardée dans la sauvegarde (voitures-possedees, car) ;
    // vrai si achetée ; la fenêtre se ferme ; solde insuffisant : rien de débité, fenêtre ouverte
    bool confirmer();
    const Zone& zoneAnnuler() const { return zAnnuler_; }       // banc d'essai
    const Zone& zoneConfirmer() const { return zConfirmer_; }   // banc d'essai (vide si solde insuffisant)
private:
    bool ouverte_ = false;
    int index_ = 0;
    Zone zAnnuler_, zConfirmer_;
};

class AchatGarage : public PanneauGarage {
public:
    enum Bouton { AUCUN = 0, CHOISIR = 1, RETOUR = 2, ESSAI = 3 };   // WFS-115 : mêmes numéros que le bouton appuyé de garage.cpp
    void dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) override;   // zones g_z.choisir, g_z.essai et g_z.retour
    int appui(float x, float y) const override;   // Bouton touché (CHOISIR, RETOUR, ESSAI), −1 si aucun
    void viderZones() override;
    // relâcher sur CHOISIR : vrai si le bouton était « Acheter » (achat de couleur fait ou refusé, ou fenêtre d'achat de la
    // voiture ouverte : le garage ne choisit pas)
    bool acheter();
    // relâcher sur ESSAI : vrai si l'essai gratuit de la voiture affichée peut partir (sinon « Essai déjà fait » : rien)
    bool essaiDemande() const;
    // WFS-502 (RG-01) : relâcher sur ESSAI quand la voiture affichée est la voiture prêtée en attente : « Activer le prêt » ;
    // vrai si le bouton était celui-là (prêt activé et sauvegarde écrite : la voiture se choisit ensuite comme une voiture possédée)
    bool activerPret();
    long long manqueAffiche() const { return manque_; }   // banc : N du bandeau « Il te manque N ★ » (0 : pas de bandeau)
    FenetreAchatVoiture& fenetre() { return fenetre_; }
    // WFS-499 : fenêtre de confirmation, dessinée après tous les panneaux (au-dessus)
    void dessinerFenetre(Renderer& r, const MiseEnPage& mp);
    bool boutonFenetre(const char* nom, float& x, float& y) const;   // banc : « confirmer », « annuler » (centre de la zone)

private:
    EtatGarage e_;                  // état montré à la dernière image (l'achat porte sur ce que le joueur voit)
    long long manque_ = 0;          // N du bandeau, 0 sans bandeau
    EtatGarage eManque_;            // état au moment du refus : le bandeau disparaît quand la couleur montrée change
    FenetreAchatVoiture fenetre_;   // WFS-499
};

// WFS-499 (banc d'essai) : bouton « Acheter N ★ » de l'écran de fin d'un essai gratuit et fenêtre de confirmation de cet écran
bool finEssaiBouton(const char* nom, float& x, float& y);   // nom : « acheter », « confirmer », « annuler »
bool finEssaiVisible();

} // namespace wfs::ecrans::garage
