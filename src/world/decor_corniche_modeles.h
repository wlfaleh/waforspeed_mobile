// WaForSpeed — WFS-371 (épic E15, décor de la Corniche) : objets de la plage modélisés au niveau du vrai objet (RG-03), à la
// place des cônes, boîtes, cylindres et boules de plage.cpp (inventaire : qa/pr/WFS-371/inventaire.md).
//
// Même méthode que la Campagne (WFS-372), le Grand Prix (WFS-373) et la marina (WFS-376) : géométrie calculée sur le
// processeur avec loft / eventail / ligne de marina_bateaux.h et poutre / profileI de decor_campagne_modeles.h (réutilisés).
// Chaque modèle est construit UNE fois à l'initialisation de la carte (version proche et version allégée), puis recopié à
// chaque pose par plage.cpp dans les blocs du décor préparés au chargement (WFS-268) : rien n'est créé en course.
// Vérifié par tests/test_decor_corniche.cpp (mesures ±1 %, budget de triangles, normales, aucun triangle dégénéré).
//
// Un modèle a plusieurs « parts » (matériaux de plage.cpp) : uni (couleurs de la palette), acier (métal brillant), paille
// (chaume texturé), neon (verre éclairé). Repère local : mètres, y vers le haut (0 = sol), z le long de la route.
// Mesures (sources : fiches techniques du commerce, normes, relevés publics ; vérifiées par le test) :
//   - parasol de plage : toile Ø 1,80 m à 8 baleines, mât d'aluminium Ø 28 mm en deux parties, 2,20 m hors sol, vrille
//     d'ancrage dans le sable, lambrequin de 0,12 m (fiches des parasols de plage : Ø 1,60 à 2,00 m, mât 22 à 32 mm) ;
//   - bain de soleil (transat) : 1,90 × 0,65 m, assise à 0,32 m, dossier relevé à 40°, cadre en tube d'aluminium Ø 30 mm,
//     toile textilène tendue, 2 roues Ø 0,16 m à la tête (fiches : 190 à 200 × 60 à 70 cm, assise 30 à 38 cm) ;
//   - parasol de paille (paillote) : Ø 2,40 m, mât de bois Ø 80 mm, 2,60 m hors sol, chaume en 3 rangs à franges ;
//   - tabouret de bar : assise Ø 0,36 m à 0,76 m (bar debout de 1,05 à 1,15 m), embase Ø 0,42 m, repose-pied Ø 0,40 m à 0,30 m ;
//   - table basse de terrasse en teck : 0,80 × 0,80 × 0,45 m, plateau à lattes ; pouf rond Ø 0,60 × 0,40 m, passepoil ;
//   - bouteille de 75 cl (bordelaise) : Ø 76 mm × 300 mm ;
//   - poste de surveillance de plage (sauveteurs) : plancher à 2,40 m, cabine 2,2 × 2,2 m et 2,0 m sous plafond, toit à une
//     pente débordant de 0,3 m, échelle de meunier, garde-corps de 1,0 m ;
//   - glissière métallique à double onde (glissière simple GS, NF EN 1317) : lisse de 310 mm de haut, 83 mm de profondeur,
//     3 mm d'acier galvanisé, bord supérieur à 0,75 m, poteaux en C 100 × 50 mm tous les 4 m, écarteur de 0,3 m ;
//   - scène d'été (podium) : 10 × 5 m, plancher à 1,0 m (praticables de 2 × 1 m), portique en poutres carrées de 29 cm
//     (membrures Ø 48 mm, diagonales Ø 20 mm), 6,2 m de haut, enceintes en caissons, table du DJ ;
//   - candélabre de promenade (lanterne de style) : fût cannelé de fonte de 5,3 m (Ø 160 mm au pied, 100 mm en tête), socle
//     mouluré, crosse de 0,6 m à volute, lanterne vitrée à 4 faces de 0,42 × 0,42 × 0,75 m sous chapeau ;
//   - palmier (Phoenix / Washingtonia) : gaines des palmes en 10 pétioles de 0,6 m (hors palme), noix de coco Ø 0,20 × 0,26 m ;
//   - guirlande guinguette : mât de pin de 4,1 m (Ø 140 → 100 mm), ampoules G60 (Ø 60 × 90 mm) ; bûche de bois de chauffage de 50 cm × Ø 0,11 m.
#pragma once
#include <cstdint>
#include <vector>
#include "marina_bateaux.h"   // Maille, Sommet, loft, eventail, ligne (WFS-376)

namespace wfs {
namespace corniche {

using Maille = marina::Maille;
constexpr int COUL_VAR = -1;   // couleur choisie à la pose (toile du parasol, coussin, pouf, bouteille)

// cases de la palette de plage.cpp utilisées par les modèles (données par plage.cpp)
struct Couleurs {
    int blanc = 0, alu = 0, toile = 0, bois = 0, boisSombre = 0, planche = 0, rouge = 0, vitre = 0, noir = 0, hp = 0;
    int fonte = 0, gaine = 0, noix = 0, ecorce = 0, sable = 0, pneu = 0, rotin = 0;
};

// modèle en parts (une maille par matériau de plage.cpp)
struct Modele {
    Maille uni, acier, paille, neon;
    int triangles() const { return uni.triangles() + acier.triangles() + paille.triangles() + neon.triangles(); }
};

// ---------- mesures (m), vérifiées par le test
constexpr float PARASOL_DIAM = 1.80f, PARASOL_HAUT = 2.20f, PARASOL_MAT = 0.028f, PARASOL_LAMBREQUIN = 0.12f;
constexpr int PARASOL_BALEINES = 8;
constexpr float TRANSAT_ARTIC = 1.10f;   // longueur de l'assise, du pied à l'articulation du dossier
constexpr float TRANSAT_L = 1.90f, TRANSAT_W = 0.65f, TRANSAT_ASSISE = 0.32f, TRANSAT_DOSSIER_DEG = 40.f, TRANSAT_TUBE = 0.030f;
constexpr float PAILLE_DIAM = 2.40f, PAILLE_HAUT = 2.60f, PAILLE_MAT = 0.080f;
constexpr float COMPTOIR_HAUT = 1.10f, ETAGERE_HAUT = 1.70f;
constexpr float TABOURET_ASSISE = 0.76f, TABOURET_DIAM = 0.36f, TABOURET_EMBASE = 0.42f;
constexpr float TABLE_COTE = 0.80f, TABLE_HAUT = 0.45f, POUF_DIAM = 0.60f, POUF_HAUT = 0.40f;
constexpr float BOUTEILLE_H = 0.300f, BOUTEILLE_D = 0.076f;
constexpr float POSTE_PLANCHER = 2.40f, POSTE_COTE = 2.20f, POSTE_CABINE = 2.00f, POSTE_DEBORD = 0.30f;
constexpr float GLISS_HAUT = 0.75f, GLISS_LISSE = 0.310f, GLISS_PROF = 0.083f, GLISS_EP = 0.003f, GLISS_PAS = 4.0f, GLISS_ECARTEUR = 0.30f;
constexpr float LYRE_Y = 5.6f, LYRE_X = -2.35f;   // centre de la tête des 4 lyres sous la poutre de façade (plage.cpp : z = −3,6 à 3,6)
constexpr float SCENE_L = 10.f, SCENE_P = 5.f, SCENE_PLANCHER = 1.0f, SCENE_PORTIQUE = 6.2f, TRUSS = 0.29f;
constexpr float CANDELABRE_FUT = 5.3f, CANDELABRE_CROSSE = 0.6f, LANTERNE_COTE = 0.42f, LANTERNE_H = 0.75f;
constexpr float LANTERNE_Y = 5.6f;   // centre du verre de la lanterne (lumière de nuit.cpp : 5,6 m, inchangée)
constexpr float NOIX_DIAM = 0.20f, NOIX_LONG = 0.26f, GAINE_PETIOLE = 0.6f;
constexpr float MAT_GUIRLANDE = 4.1f, AMPOULE_D = 0.060f, AMPOULE_H = 0.090f, BUCHE_L = 0.50f, BUCHE_D = 0.11f;

// profil de la lisse à double onde (section en x = saillie vers la route, y = hauteur au-dessus du sol), du bas vers le haut ;
// x = 0 : fond des ondes (contre l'écarteur). Utilisé par plage.cpp pour la lisse continue le long de la route.
std::vector<vec2> profilLisse();

// ---------- modèles (loin = version allégée : au-delà de 200 m ou qualité Basse pour les objets très nombreux)
// parasol de plage, mât en (0, 0, 0) ; toile : COUL_VAR
Modele parasol(const Couleurs& c, bool loin);
// bain de soleil, axe le long de z (tête vers +z), centre au sol ; toile : couleur c.toile
Modele transat(const Couleurs& c, bool loin);
// parasol de paille, mât en (0, 0, 0)
Modele parasolPaille(const Couleurs& c, bool loin);
// comptoir de bar de longueur L (le long de z), face aux clients vers −x : caisson à lames verticales, plateau de teck
// débordant chanfreiné (dessus à COMPTOIR_HAUT), socle en retrait ; centre au sol
Modele comptoir(const Couleurs& c, float L);
// étagère de bar (L le long de z, 0,40 m de profondeur, ETAGERE_HAUT), dos contre x = +0,2 ; centre au sol
Modele etagere(const Couleurs& c, float L);
// tabouret de bar, pied en (0, 0, 0)
Modele tabouret(const Couleurs& c);
// table basse carrée centrée en (0, 0, 0)
Modele tableBasse(const Couleurs& c);
// pouf rond posé au sol, centre en (0, 0, 0) ; tissu : COUL_VAR
Modele pouf(const Couleurs& c);
// bouteille debout, fond en (0, 0, 0) ; verre : COUL_VAR
Modele bouteille(const Couleurs& c);
// poste de surveillance, centre au sol ; échelle et porte côté −x (dos à la mer quand x pointe vers la mer)
Modele posteSecours(const Couleurs& c, bool loin);
// poteau de glissière (C 100 et écarteur) : pied en (0, 0, 0), lisse côté −x (vers la route)
Modele poteauGlissiere(const Couleurs& c);
// scène d'été : plancher de SCENE_P (x, profondeur, face au public vers −x) × SCENE_L (z) ; centre au sol
Modele scene(const Couleurs& c, bool loin);
// enceinte de façade (2 caissons de basses et une enceinte à pavillon), face vers −x, pied en (0, 0, 0)
Modele enceinte(const Couleurs& c, bool loin);
// lentille bombée d'une lyre (Ø 0,15 m), tournée vers le bas, centre de la face en (0, 0, 0) : maillage éclairé de la fête
Modele lentilleLyre();
// candélabre de promenade : pied en (0, 0, 0), crosse et lanterne vers −x
Modele candelabre(const Couleurs& c, bool loin);
// couronne d'un palmier : gaines (pétioles) autour de (0, 0, 0), le haut du stipe ; noix de coco en grappe sous la couronne
Modele gainesPalmier(const Couleurs& c, uint32_t graine, bool loin);
// mât de guirlande, pied en (0, 0, 0)
Modele matGuirlande(const Couleurs& c);
// ampoule G60 (verre éclairé : part neon), culot en (0, 0, 0), ampoule vers le bas
Modele ampoule();
// bûche couchée le long de x, centrée en (0, 0, 0)
Modele buche(const Couleurs& c, uint32_t graine);
// rondin vertical (poteau de pin écorcé) de 1 m de haut et Ø 1 m au pied, à mettre à l'échelle (rayon, hauteur) à la pose
Modele rondin(const Couleurs& c, uint32_t graine);

} // namespace corniche
} // namespace wfs
