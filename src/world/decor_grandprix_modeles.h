// WaForSpeed — WFS-373 (épic E15, décor du Grand Prix) : objets du circuit modélisés au niveau du vrai objet (RG-03), à la
// place des boîtes, cylindres, cônes et triangles de circuit.cpp (inventaire : qa/pr/WFS-373/inventaire.md, ANO-220).
//
// Même méthode que la Campagne (WFS-372, decor_campagne_modeles.h) et la marina (WFS-376, marina_bateaux.h) : géométrie
// calculée sur le processeur, avec les outils loft / eventail / ligne de marina_bateaux.h et poutre / profileI de
// decor_campagne_modeles.h (réutilisés, pas recopiés). Chaque modèle est construit UNE fois à l'initialisation de la carte
// (un maillage par sorte d'objet, version proche et version allégée), puis recopié à chaque pose par circuit.cpp (palette de
// la carte, même matériau : aucun appel de dessin en plus).
// Vérifié par tests/test_decor_grandprix.cpp (mesures ±1 %, budget de triangles, normales, aucun triangle dégénéré).
//
// Repère local d'un modèle : mètres, y vers le haut (0 = sol), x en travers de la route, VERS L'EXTÉRIEUR (dos à la route),
// z le long de la route (sens de la course). x = 0 : face tournée vers la route, sauf mention contraire.
// Mesures (sources : normes, fiches techniques et guides publics) :
//   - pneu de tourisme 205/55 R16 (désignation ETRTO : largeur 205 mm, flanc 55 % = 112,75 mm, jante 16" = 406,4 mm) :
//     Ø extérieur 632 mm ; mur de pneus (FIA, annexe O du Code sportif international et « guidelines for circuit safety
//     installations ») : piles de pneus boulonnées, 4 de haut au moins, face côté piste couverte d'une bande transporteuse de
//     caoutchouc de 10 à 12 mm, sanglée ;
//   - panneau publicitaire de bord de piste : bâche de 11 × 1,2 m sur cadre de tubes d'aluminium 60 × 40 mm, jambes de force ;
//   - siège coque de stade (EN 13200-4) : largeur 0,44 m, assise à 0,45 m, dossier de 0,32 à 0,42 m, entraxe 0,50 m ;
//     garde-corps de tribune 1,0 à 1,1 m (EN 13200-3), tube Ø 48,3 mm ;
//   - mur des stands : muret de béton de 1,1 m surmonté d'un grillage de protection (poteaux Ø 114 mm, câbles, maille) ;
//   - stand (garage d'écurie) : 16 m de façade, rideaux métalliques à lames de 100 mm, porte de 4,2 m de haut, auvent de 3 m ;
//   - mât de drapeau de 9 m en aluminium conique (Ø 100 → 60 mm), drapeau 2,4 × 1,6 m (rapport 3:2) ;
//   - poste de commissaire : plateforme de 2 × 2 m à 1,0 m du sol, garde-corps 1,0 m, abri à pente ;
//   - panneau de freinage 150 / 100 / 50 m : 1,6 × 1,2 m, poteaux Ø 76 mm ;
//   - semi-remorque fourgon : 13,6 m × 2,55 m × 4,0 m hors tout (directive 96/53/CE), tracteur à cabine avancée de 6 m, roues
//     315/80 R22,5 (Ø 1 076 mm) ;
//   - hélicoptère léger monoturbine (5 à 6 places) : fuselage de 10,9 m, rotor Ø 10,7 m, hauteur 3,1 m, patins de 2,3 m d'écart ;
//   - tente dôme 2 places : 2,6 × 2,2 m, 1,3 m de haut ; grillage de clôture : poteaux Ø 48 mm de 1,9 m, maille de 50 mm ;
//   - projecteur de circuit : mât conique de 19 m (Ø 450 → 200 mm), rampe de 6 projecteurs de 17,8 à 19 m.
#pragma once
#include <cstdint>
#include <vector>
#include "marina_bateaux.h"   // Maille, Sommet, loft, eventail, ligne (WFS-376)

namespace wfs {
namespace grandprix {

using Maille = marina::Maille;
// couleur choisie à la pose (fond d'un panneau publicitaire, couleur d'équipe, toile) : remplacée par poser()
constexpr int COUL_VAR = -1;

// cases de la palette de la carte utilisées par les modèles (données par circuit.cpp)
struct Couleurs {
    int pneu = 0, pneuRouge = 0, pneuBlanc = 0, sangle = 0;   // mur de pneus
    int alu = 0, aluSombre = 0, beton = 0, betonNez = 0, acier = 0, tole = 0, bardage = 0;
    int verre = 0, menuiserie = 0, rideau = 0, interieur = 0, dalle = 0, enduit = 0;
    int commissaire = 0, abri = 0, panneau = 0, mat = 0;
    int cabine = 0, chassis = 0, jante = 0, phare = 0, feuArriere = 0;
    int toile = 0, bois = 0, fuselage = 0, pale = 0, plume = 0, plumeSombre = 0;
};

// ---------- mesures (m), vérifiées par le test
constexpr float PNEU_DIAM = 0.632f, PNEU_LARGEUR = 0.205f, PNEU_JANTE = 0.4064f;
constexpr int PNEUS_PILE = 4, PILES_MUR = 7;
constexpr float MUR_PNEUS_L = 4.8f;
constexpr float PUB_L = 11.5f, PUB_BACHE_L = 11.0f, PUB_BACHE_H = 1.2f, PUB_BAS = 0.15f;
constexpr float SIEGE_LARGEUR = 0.44f, SIEGE_ASSISE = 0.45f, SIEGE_DOSSIER = 0.38f;
constexpr float MURET_STANDS = 1.1f, GRILLAGE_STANDS = 2.6f;
constexpr float STAND_HAUT = 7.0f, STAND_PROF = 12.0f, STAND_PORTE_H = 4.2f, AUVENT = 3.0f;
constexpr float MAT_DRAPEAU = 9.0f, DRAPEAU_L = 2.4f, DRAPEAU_H = 1.6f;
constexpr float COMMISSAIRE_COTE = 2.0f, COMMISSAIRE_PLANCHER = 1.0f;
constexpr float FREIN_L = 1.6f, FREIN_H = 1.2f, FREIN_BAS = 1.0f;
constexpr float REMORQUE_L = 13.6f, CAMION_LARGEUR = 2.55f, CAMION_HAUT = 4.0f, ROUE_CAMION = 1.076f, CAMION_L = 16.5f;
constexpr float CAMION_AVANT = CAMION_L - REMORQUE_L / 2 - 0.05f;   // avant du pare-chocs (l'arrière du pare-chocs de la remorque est à −6,85 m)
constexpr float HELICO_L = 10.9f, HELICO_ROTOR = 10.7f, HELICO_HAUT = 3.1f, HELICO_NEZ = 2.65f, HELICO_FIN = HELICO_NEZ - HELICO_L;
constexpr float TENTE_L = 2.6f, TENTE_W = 2.2f, TENTE_H = 1.3f;
constexpr float GRILLAGE_H = 1.9f, GRILLAGE_PAS = 3.6f;
constexpr float PROJ_MAT = 19.f;   // mât jusqu'au haut de la rampe (projecteurs de 17,6 à 19 m, comme avant)

// ---------- modèles (loin = version allégée : morceaux au-delà de 200 m, ou qualité Basse pour les objets très nombreux)
// mur de pneus de 4,8 m (z de 0 à 4,8), face des piles en x = 0 ; pneu du haut peint : rouge / blanc par pile (decalage) ;
// piles ceinturées par deux sangles
Maille murPneus(const Couleurs& c, bool loin, int decalage);
// panneau publicitaire centré en z = 0, bâche en x de 0 (face route) à 0,06 m ; fond de la bâche : COUL_VAR
Maille panneauPub(const Couleurs& c, bool loin);
// siège coque, face vers −x (vers la route), centre de l'assise en (0, 0, 0) au niveau du gradin ; couleur : COUL_VAR
Maille siege(const Couleurs& c);
// rangée de sièges allégée (vue de loin) de longueur l centrée en z = 0 ; couleur : COUL_VAR
Maille rangeeSieges(const Couleurs& c, float l);
// tribune sans sièges ni public : gradins (rangs de profondeur rang, contremarche marche, muret avant muret), allées,
// garde-corps, mur du fond, toit sur charpente ; longueur l centrée en z = 0 ; face de la tribune en x = −rang / 2
Maille tribune(const Couleurs& c, float l, int rangs, float rang, float marche, float muret, float entreAllees, float allee, bool loin);
// élément de mur des stands de longueur l (z de 0 à l) : muret de béton et grillage de protection, face piste en x = −0,2
Maille murStands(const Couleurs& c, float l, bool loin);
// stand (garage d'écurie) vitré de longueur l centré en z = 0, façade en x = 0, bâtiment jusqu'en x = STAND_PROF ;
// bandeau de l'équipe : COUL_VAR
Maille stand(const Couleurs& c, float l, Maille& neon, bool loin);   // néons : réglettes des garages, éclairage sous l'auvent
// mât de drapeau au pied en (0, 0, 0) ; toile vers +z : COUL_VAR
Maille drapeau(const Couleurs& c, bool loin);
// drapeau tenu à la main (hampe de 1,9 m, toile 1,2 × 0,8 m vers −z) : COUL_VAR
Maille drapeauMain(const Couleurs& c);
Maille posteCommissaire(const Couleurs& c, bool loin);   // centré au sol, face à la route vers −x
Maille panneauFrein(const Couleurs& c);                  // centré au sol, face vers −x ; chiffres posés par circuit.cpp
// portique des feux de départ : x en travers (piste centrée en x = 0, largeur entre pylônes 2 · demi), y vers le haut, z
// le long ; caisson des 5 feux à 6,8 m, face vers −z (vers la grille)
Maille portique(const Couleurs& c, float demi);
Maille lentilleFeu();                                    // lentille bombée Ø 0,24 m et sa bague, face vers −z, centre (0, 0, 0)
Maille passerelle(const Couleurs& c, float demi);        // même repère que le portique ; panneaux posés par circuit.cpp
Maille camion(const Couleurs& c, bool loin);             // centré en x = 0, remorque de z = −6,8 à 6,8, tracteur devant ; COUL_VAR
// bar d'accueil : w en travers (x de 0 à w), l le long (centré en z = 0), 2 niveaux et terrasse ; néons dans neon
Maille accueil(const Couleurs& c, float w, float l, int parasol0, int variante, Maille& neon, bool loin);   // parasols : parasol0 + 0..5
Maille tour(const Couleurs& c, Maille& neon, bool loin); // tour de contrôle centrée en x = z = 0
Maille helico(const Couleurs& c, bool loin);             // nez vers +z (vers l'avant de la route), centré au sol
Maille standFans(const Couleurs& c, Maille& neon);       // stand de marché 3 × 4 m centré, comptoir côté −x ; toile COUL_VAR
Maille portail(const Couleurs& c, Maille& neon);         // portail d'entrée (6,8 m), enseigne posée par circuit.cpp
Maille roue(const Couleurs& c, Maille& neon);            // grande roue (tourne autour de l'axe x), centre (0, 0, 0)
Maille nacelle(const Couleurs& c, int couleur);          // nacelle de la grande roue, point d'accroche en (0, 0, 0)
Maille piedsRoue(const Couleurs& c);                     // pieds en A et socle, axe à 14,5 m
Maille tente(const Couleurs& c, int variante);           // tente dôme centrée au sol, porte vers −x ; COUL_VAR
Maille grillage(const Couleurs& c, bool loin);           // élément de 3,6 m (z de 0 à 3,6), poteau en z = 0
Maille oiseau(float battement, int couleur);             // oiseau de 0,3 m, bec vers −z (aile à battement : −1 bas, +1 haut)
Maille vol(int nombre, int couleur);                     // vol en V de nombre oiseaux
Maille projecteur(const Couleurs& c, Maille& neon, bool loin);   // mât en (0, 0, 0), projecteurs tournés vers −x

// pose d'un modèle : repère local x, y, z → X, haut, Z (monde), couleurs de la palette (COUL_VAR → var) ; ajoute à une maille
void poser(Maille& dst, const Maille& m, vec3 o, vec3 X, vec3 Z, int var = 0);
// change une couleur de la palette dans une maille
void colorier(Maille& m, int de, int vers);

// ---------- outils (aussi utilisés par le test et par circuit.cpp)
// tube rond de a à b, rayon r, n côtés, bouts fermés
void tube(Maille& m, vec3 a, vec3 b, float r, int n, int couleur);
// pièce de révolution d'axe ax passant par o : profil (abscisse le long de l'axe, rayon), n côtés, de l'angle a0 à a1 (rad ;
// tour complet par défaut ; angle 0 selon ref, ou une direction quelconque si ref est nul) ; bouts fermés si le rayon n'est pas
// nul, le tour complet et bouts vrai
void revolution(Maille& m, vec3 o, vec3 ax, const std::vector<std::pair<float, float>>& prof, int n, int couleur, float a0 = 0,
                float a1 = 6.2831853f, vec3 ref = {0, 0, 0}, bool bouts = true);
// quadrilatère plan tourné vers « dehors »
void quadPlat(Maille& m, vec3 a, vec3 b, vec3 c, vec3 d, int couleur, vec3 dehors);
// tôle nervurée (bac acier) : bande plane de largeur le long de u, de longueur le long de v, nervures trapézoïdales de pas p
// et de hauteur h vers « dehors », origine au coin (0, 0)
void bacAcier(Maille& m, vec3 o, vec3 u, vec3 v, vec3 dehors, float largeur, float longueur, float p, float h, int couleur);

// ---------- banc d'essai (gros plans) : premier objet de chaque sorte placé sur le tour (rempli par circuit.cpp au chargement)
enum ObjetSorte { OB_PNEUS, OB_PUB, OB_TRIBUNE, OB_MUR_STANDS, OB_STAND, OB_DRAPEAU, OB_COMMISSAIRE, OB_FREIN, OB_PORTIQUE,
                  OB_PASSERELLE, OB_CAMION, OB_ACCUEIL, OB_TOUR, OB_HELICO, OB_FANS, OB_ROUE, OB_TENTE, OB_GRILLAGE, OB_PROJ, NB_OBJETS_BANC };
struct ObjetBanc { int64_t seg = -1; int side = 1; float lat = 0; };
ObjetBanc objetBanc(int sorte);
void noterObjet(int sorte, int64_t seg, int side, float lat);   // premier objet de la sorte seulement
void oublierObjets();

} // namespace grandprix
} // namespace wfs
