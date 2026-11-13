// WaForSpeed — WFS-410 (épic E15, RG-04) : église de village de la Campagne modélisée au niveau du vrai objet, à la place de
// la boîte et du cône de campagne.cpp (règle des formes interdites du 2026-11-09 19:41 UTC).
//
// Modèle : petite église romane rurale de Provence et du Languedoc (références de conception : églises de village à nef unique,
// chevet en abside semi-circulaire, clocher-mur à deux baies campanaires sur la façade ouest, portail latéral au midi), en
// pierre calcaire de taille, toit de tuiles canal à génoise.
// Mesures (RG-04 de WFS-410 : nef de 8 à 12 m de large, 20 à 35 m de long ; portail de 1,8 à 2,4 m × 3,5 à 4,5 m) :
//   - nef : 9,0 m de large, 22,0 m de long (avis de l'agent 3D : la nef seule dans 20 à 35 m), murs gouttereaux de 8,0 m, toit à
//     deux pans à 30 % ; abside de 3,4 m de rayon (6,2 m de haut) : 25,75 m hors tout avec l'abside et le débord de son toit ;
//   - portail en plein cintre au midi : 2,2 × 4,4 m, porte à deux vantaux de planches, pentures de fer ; deux marches ;
//   - baies en plein cintre : 0,8 × 2,2 m (3 au nord, 2 au midi de part et d'autre du portail), 0,9 × 2,4 m (ouest), vitraux sous
//     plomb en losanges (plombs de 6 mm, barlotières de fer) ;
//   - clocher-mur : 4,4 m de large, 0,9 m d'épaisseur, de 8,0 à 14,6 m (pignon à 15,35 m), deux baies campanaires de 1,0 ×
//     2,0 m avec leurs cloches de bronze (Ø 0,70 m à la pince) ; croix de fer ;
//   - contreforts de 0,8 × 0,7 m à glacis, trois par gouttereau ; perron de 6 marches de 16 cm chanfreinées jusqu'au sol.
// Repère : mètres, y vers le haut (0 = sol), x vers la route (portail en +x), z le long de la route ; centre de la nef au sol.
// Vérifié par tests/test_archi_lieux.cpp (mesures, aucune forme nue : ouvertures percées, budget de triangles).
#pragma once
#include "decor_campagne_modeles.h"

namespace wfs::campagne {

struct CouleursEglise {
    int pierre = 0, pierre2 = 0, tuile = 0, porte = 0, fer = 0, bronze = 0, bois = 0;
    int vitrail[3] = {0, 0, 0};   // bleu, rubis, ambre (posés dans la part allumée la nuit)
};
// le modèle en deux mailles : la pierre, le bois, le fer et les tuiles (part unie) ; les vitraux (part allumée la nuit)
struct Eglise { Maille m, vitraux; };
Eglise eglise(const CouleursEglise& c, bool loin);

constexpr float EGLISE_LARGEUR = 9.0f, EGLISE_LONGUEUR = 22.0f, EGLISE_MUR = 8.0f, EGLISE_PENTE = 0.30f;
constexpr float EGLISE_ABSIDE_R = 3.4f, EGLISE_ABSIDE_H = 6.2f;
constexpr float EGLISE_PORTAIL_L = 2.2f, EGLISE_PORTAIL_H = 4.4f;
constexpr float EGLISE_CLOCHER_L = 4.4f, EGLISE_CLOCHER_EP = 0.9f, EGLISE_CLOCHER_H = 14.6f;

}  // namespace wfs::campagne
