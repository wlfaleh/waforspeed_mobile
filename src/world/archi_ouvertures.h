#pragma once
// WaForSpeed — WFS-402 (épic E15, file « Ouvertures communes ») : fenêtres et portes au niveau du vrai objet, pour tous les
// bâtiments du kit d'architecture (WFS-161 ; Ville, Corniche, Campagne).
//
// Avant ce ticket, une baie du kit était un trou en retrait (tableaux, linteau, appui) avec, au fond, une vitre ou une porte
// peinte à plat : aucun cadre, aucun ouvrant, aucune poignée, la même fenêtre partout (règle de l'utilisateur du 2026-11-09
// 19:41 UTC : plus aucune forme géométrique nue). Ici, chaque baie reçoit sa menuiserie :
//   - fenêtre : dormant (6 cm, chanfrein de 1 cm), un ou deux ouvrants (5,5 cm) avec jet d'eau, petits bois selon le style,
//     imposte éventuelle, crémone ; appui en pierre à glacis et larmier ; linteau visible (pierre, béton) selon le style ;
//   - volets selon le lieu (RG-03) : persiennes à lames (méditerranéen), volets pleins à barres et écharpe (village), volets
//     roulants à coulisses (moderne, villa) ; 10 à 30 % fermés ou entrouverts, jamais tous pareils (graine de la baie) ;
//   - porte d'entrée (RG-04) : huisserie, vantail à panneaux, à planches ou vitré, imposte, seuil de pierre (et marche aux
//     maisons), béquille sur plaque avec serrure ; porte de garage sectionnelle à panneaux ; vitrine : porte vitrée, barre
//     de tirage, soubassement ;
//   - variété (RG-05) : 4 modèles de fenêtre et 4 modèles de porte par style, choisis par la graine du bâtiment.
// Aucun Vulkan : testé par tests/archi_geo.cpp (verifierWFS402). Même repère que archi_geo.h (x le long de la
// façade, y vers le haut, z sortant de la façade ; la baie est un trou de z = 0 à z = −d).
#include "archi_geo.h"

namespace wfs::archi::ouvertures {

// RG-05 : modèle de fenêtre (4 par style)
struct ModeleFenetre {
    const char* nom;
    uint8_t vantaux;    // 1 ou 2 (0 : châssis fixe, baie d'atelier)
    uint8_t rangs;      // carreaux en hauteur par vantail (1 : sans petits bois horizontaux)
    uint8_t colonnes;   // carreaux en largeur par vantail
    bool imposte;       // imposte fixe vitrée au-dessus des ouvrants
};
// RG-05 : modèle de porte d'entrée (4 par style)
enum class Vantail : uint8_t { Panneaux, Planches, VitreHaut, Vitre, Rainures };
struct ModelePorte {
    const char* nom;
    Vantail vantail;
    uint8_t vantaux;    // 1 ou 2 (2 seulement si la baie a au moins 1,2 m)
    bool imposte;       // imposte vitrée au-dessus du vantail
    bool bois;          // vantail en bois (sinon métal ou aluminium)
};
const ModeleFenetre& modeleFenetre(const Plan& p);
const ModelePorte& modelePorte(const Plan& p);
int indiceModeleFenetre(const Plan& p);   // 0..3
int indiceModelePorte(const Plan& p);     // 0..3

// RG-03 : volets selon le style et état de chaque fenêtre
enum class Volet : uint8_t { Aucun, Persiennes, Pleins, Roulant };
enum class EtatVolet : uint8_t { Ouvert, Entrouvert, Ferme };
Volet voletDuStyle(Style s);
EtatVolet etatVolet(const Plan& p, const Baie& b);
// vrai si la baie reçoit des volets battants (assez de mur de chaque côté pour les rabattre ouverts)
bool voletsBattants(const Plan& p, const Baie& b);

// vrai si la porte d'entrée du bâtiment a un vitrage (le fond de la baie est alors une vitre, avec la pièce de nuit)
bool porteVitree(const Plan& p);

// pièce du kit qui porte les châssis d'un style : CHASSIS (bois peint) ou METAL (aluminium, acier)
int partChassis(Style s);

// menuiserie complète d'une baie (fenêtre, porte-fenêtre, porte, vitrine, porte de garage) au fond d'un tableau de
// profondeur d, au niveau de détail lod (0 complet, 1 dormant, montant central et volets simplifiés). La vitre (VITRE, uv de
// la pièce de nuit) ou le fond plein de la porte est posé par l'appelant (archi_geo.cpp) au fond du tableau.
void menuiserie(const Plan& p, const Baie& b, float d, int lod, Batiment& out);
// volets d'une fenêtre (RG-03), niveau 0 ou 1
void volets(const Plan& p, const Baie& b, float d, int lod, Batiment& out);
// encadrement (chambranle) en saillie autour d'une baie, sur 3 côtés : face, chanfrein intérieur, tranches (niveau 0)
void encadrement(Maillage& m, float x0, float y0, float x1, float y1, float largeur, float saillie);
// appui de fenêtre en pierre (glacis, nez, larmier) et linteau visible selon le style (niveau 0)
void appuiEtLinteau(const Plan& p, const Baie& b, Batiment& out);

// WFS-402 (décision du PO, pour WFS-410 et WFS-411) : pose une baie complète dans n'importe quelle façade, hors du plan
// d'un bâtiment du kit. Repère de la façade (archi_geo.h) : x le long de la façade, y depuis le sol, z sortant ; la façade
// est le plan z = 0 et l'appelant y a déjà percé le trou [x − l/2, x + l/2] × [y, y + h]. Rend le tableau (retrait du
// style), la vitre (pièce de nuit) ou le fond plein, la menuiserie, les volets, l'appui et l'encadrement du style.
//   plancher : hauteur du plancher de la pièce (poignées à 1,00-1,10 m au-dessus) ; murLibre : mur plein de chaque côté
//   de la baie (les volets battants ne se posent que s'ils peuvent s'y rabattre) ; graine : choix des modèles (RG-05).
void baieComplete(Style style, uint32_t graine, Ouverture type, float x, float y, float l, float h, float plancher,
                  float murLibre, int lod, Batiment& out);

}  // namespace wfs::archi::ouvertures
