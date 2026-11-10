// WaForSpeed — WFS-372 (épic E15, décor de la Campagne) : objets agricoles modélisés au niveau du vrai objet (RG-03), à la
// place des formes nues de campagne.cpp (ANO-65, ANO-219) : rang de vigne palissé (piquets, fils, ceps), clôture rustique
// (poteaux à pointe, lisses en demi-rondin), botte ronde de foin, corps de ferme (murs crépis, soubassement de pierre,
// fenêtres en retrait avec appuis et volets, toit de tuiles canal, génoise, cheminée), hangar agricole (portiques en acier
// IPE, bardage de planches verticales, toit en plaques ondulées, gouttières, bottes rangées dedans), poteau de ligne
// électrique en bois (fût conique, traverse, jambes de force, isolateurs à jupes).
//
// Même méthode que les bateaux de la marina (WFS-376, marina_bateaux.h) : géométrie calculée sur le processeur seulement,
// avec les outils loft / eventail / ligne de marina_bateaux.h (réutilisés, pas recopiés) ; la carte pose ces mailles dans son
// décor (palette de la carte, même matériau, aucun appel de dessin en plus). Chaque modèle est construit UNE fois à
// l'initialisation de la carte (un maillage par sorte d'objet, plusieurs variantes), puis recopié à chaque pose.
// Vérifié par tests/test_decor_campagne.cpp (mesures ±1 %, budget de triangles, normales, aucun triangle dégénéré).
//
// Repère local d'un modèle : mètres, y vers le haut (0 = sol), x en travers, z en long (le long de la route).
// Mesures (sources : fiches techniques et guides publics, valeurs courantes en France) :
//   - rang de vigne palissé (taille Guyot, guides de la viticulture) : piquets intermédiaires de bois de 1,80 m hors sol,
//     Ø 7 à 8 cm, tous les 5 à 7 m ; fil porteur à 0,60 m, fils releveurs à 1,0 et 1,4 m ; ceps tous les 1,0 m, tronc
//     de 0,55 à 0,60 m, Ø 5 à 7 cm ; feuillage palissé de 0,60 à 1,65 m, 0,35 à 0,50 m d'épaisseur ;
//   - clôture rustique de pâture : poteaux ronds de châtaignier Ø 10 à 12 cm, 1,20 m hors sol, pointe taillée ; lisses en
//     demi-rondin Ø 10 cm ;
//   - botte ronde (presses à balles rondes à chambre variable) : 1,20 m de large, Ø 1,50 m ;
//   - corps de ferme : 8 × 11 m au sol (inchangé, chocs et écho), murs de 4,5 m (rez-de-chaussée + étage), soubassement de
//     0,6 m, fenêtres de 0,80 × 1,20 m en retrait de 0,20 m, porte de 1,0 × 2,1 m, toit à deux pans de tuiles canal à 30 %,
//     débord de 0,45 m (génoise à deux rangs), cheminée de 0,60 × 0,45 m ;
//   - hangar agricole : 10 × 14 m (inchangé), portiques en acier tous les 3,5 m (HEA 200 : 0,20 × 0,20 m), sablière à 5,5 m,
//     pente 15° (27 %), plaques ondulées (onde de 177 mm, 51 mm de creux : plaques de fibres-ciment ou d'acier courantes),
//     bardage de planches verticales de 145 mm, muret de béton de 0,6 m ;
//   - poteau de ligne électrique en bois (réseau HTA aérien) : 8,5 m hors sol, Ø 0,28 m au pied, 0,20 m en tête, traverse de
//     1,6 m, isolateurs rigides de 0,2 m.
#pragma once
#include <cstdint>
#include <vector>
#include "marina_bateaux.h"   // Maille, Sommet, loft, eventail, ligne (WFS-376)

namespace wfs {
namespace campagne {

using Maille = marina::Maille;

// cases de la palette de la carte utilisées par les modèles (données par la carte)
struct Couleurs {
    int piquet = 0, cep = 0, fil = 0;                                  // vigne
    int foin = 0, foinBout = 0;                                         // botte (côté, bout en spirale)
    int crepi = 0, pierre = 0, tuile = 0, volet = 0, menuiserie = 0, vitre = 0, porte = 0, embrasure = 0;   // ferme
    int bardage = 0, bardage2 = 0, tole = 0, acier = 0, beton = 0, zinc = 0;                         // hangar
    int poteau = 0, ferrure = 0, isolateur = 0, cable = 0;            // ligne électrique
};

// ---------- mesures (m), vérifiées par le test
constexpr float PIQUET_HAUT = 1.80f, PIQUET_DIAM = 0.075f;
constexpr float FILS_Y[3] = {0.60f, 1.00f, 1.40f};
constexpr float CEP_ECART = 1.0f, CEP_HAUT = 0.58f;
constexpr float VIGNE_FEUILLE_BAS = 0.60f, VIGNE_FEUILLE_HAUT = 1.65f, VIGNE_EPAISSEUR = 0.42f;
constexpr float CLOTURE_HAUT = 1.20f, CLOTURE_DIAM = 0.12f, LISSE_DIAM = 0.10f, LISSES_Y[2] = {0.45f, 0.95f};
constexpr float BOTTE_LARGEUR = 1.20f, BOTTE_DIAM = 1.50f;
constexpr float FERME_LARGEUR = 8.0f, FERME_LONGUEUR = 11.0f, FERME_MUR = 4.5f, FERME_PENTE = 0.30f, FERME_DEBORD = 0.45f;
constexpr float HANGAR_LARGEUR = 10.0f, HANGAR_LONGUEUR = 14.0f, HANGAR_SABLIERE = 5.5f, HANGAR_PENTE_DEG = 15.f;
constexpr float POTEAU_HAUT = 8.5f, POTEAU_DIAM_PIED = 0.28f, POTEAU_DIAM_TETE = 0.20f, TRAVERSE = 1.6f, TRAVERSE_Y = 7.8f;

// ---------- modèles (construits une fois ; loin = version allégée pour les morceaux au-delà de 200 m)
Maille piquetVigne(const Couleurs& c, int variante);            // base au sol en (0, 0, 0), un peu enfoncé
Maille cep(const Couleurs& c, int variante, bool loin);         // tronc tortueux et cordon (bras le long de z)
Maille poteauCloture(const Couleurs& c, int variante);          // poteau rond, pointe taillée en diamant
Maille botte(const Couleurs& c, bool loin);                     // axe selon x, posée au sol (bas aplati), centre en x = z = 0
Maille ferme(const Couleurs& c, bool loin);                     // façade principale (porte) vers +x ; centre au sol
Maille hangar(const Couleurs& c, const Maille& botteRangee, bool loin);   // côté ouvert vers +x ; centre au sol
Maille poteauLigne(const Couleurs& c, bool loin);               // fût, traverse le long de x
// points d'accroche des 3 conducteurs en tête des isolateurs du poteau de ligne (repère du poteau)
void accrochesLigne(vec3 out[3]);

// ---------- pièces posées le long de la route (coordonnées du monde, ajoutées à une maille du monde)
// lisse de clôture en demi-rondin de a à b (face plane vers « plat »), légère flèche au milieu
void lisse(Maille& m, vec3 a, vec3 b, vec3 plat, int couleur, bool loin);
// câble pesant de a à b (chaînette approchée par une parabole de flèche f au milieu), en n tronçons
void cable(Maille& m, vec3 a, vec3 b, float fleche, float diam, int n, int couleur);

// ---------- outils de modélisation (aussi utilisés par le test)
// pièce prismatique de section rectangulaire w × h, arêtes chanfreinées de ch, de a à b ; « haut » oriente la hauteur h
void poutre(Maille& m, vec3 a, vec3 b, vec3 haut, float w, float h, float ch, int couleur);
// profilé en I (HEA / IPE) de a à b : hauteur h selon « haut », largeur des ailes w, épaisseurs ame et aile
void profileI(Maille& m, vec3 a, vec3 b, vec3 haut, float h, float w, float ame, float aile, int couleur);

// ---------- banc d'essai (gros plans) : premier objet de chaque sorte placé sur le tour (rempli par campagne.cpp au chargement)
enum ObjetSorte { OB_VIGNE, OB_CLOTURE, OB_BOTTES, OB_FERME, OB_POTEAU, NB_OBJETS_BANC };
struct ObjetBanc { int64_t seg = -1; int side = 1; float lat = 0; };
ObjetBanc objetBanc(int sorte);

} // namespace campagne
} // namespace wfs
