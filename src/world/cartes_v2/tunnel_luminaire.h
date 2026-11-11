// WaForSpeed — WFS-392 (RG-02) : luminaire de tunnel routier et son chemin de câbles, modélisés pièce par pièce (aucune forme nue).
// Cotes réelles et sources : references/reel/cartes_v2/tunnel.md (avis de l'expert-realisme-3d du 2026-11-11 : valeurs de mémoire,
// à confirmer sur une fiche de fabricant). Repère : `sousDalle` = point de la sous-face de la dalle à l'aplomb du luminaire ;
// r = travers du tunnel, f = le long du tunnel (grand axe du luminaire) ; tout pend sous la dalle :
//   tiges filetées (0 à −0,12 m) → chemin de câbles en U de 0,20 × 0,06 m → étriers → corps → cadre → vitre à −0,35 m
// Sodium haute pression : corps en trapèze 0,90 × 0,34 × 0,16 m, vitre plane 0,78 × 0,24 m ; LED : corps plat 0,80 × 0,30 × 0,10 m à
// 6 ailettes, vitre 0,72 × 0,24 m à 3 traverses (4 modules). Pièces : corps, 2 embouts, cadre de vitre (4 montants), vitre émissive en
// retrait de 5 mm, 2 étriers (plat de 40 × 5 mm), 4 grenouillères, presse-étoupe et câble. Près : ≈ 230 triangles ; loin : 26.
#pragma once
#include "../decor_carte.h"

namespace wfs::carte::tunnel {

constexpr float VITRE_SOUS_DALLE_M = 0.35f;   // vitre à 5,25 m sous une dalle à 5,60 m : gabarit routier de 4,75 m libre
constexpr float CHEMIN_L_M = 0.20f, CHEMIN_H_M = 0.06f, CHEMIN_SOUS_DALLE_M = 0.12f;

struct Cotes { float L, l, h, vitreL, vitreL2, fuite; };   // corps (long, large, haut), vitre (long, large), dessus / dessous
constexpr Cotes SODIUM{0.90f, 0.34f, 0.16f, 0.78f, 0.24f, 0.8f}, LED{0.80f, 0.30f, 0.10f, 0.72f, 0.24f, 1.f};

// pavé à 6 faces : c = centre de la face du dessous, w selon r, h vers le haut, l selon f ; fuite : largeur du dessus / dessous
void pave(Geo& g, vec3 c, vec3 r, vec3 f, float w, float h, float l, vec2 uv, float fuite = 1);
// luminaire entier ; corps et acier dans `uni`, vitre dans `emis` (matériau émissif × couleur de la case uvVitre)
void luminaire(Geo& uni, Geo& emis, vec3 sousDalle, vec3 r, vec3 f, bool led, bool lod, vec2 uvCorps, vec2 uvAcier, vec2 uvVitre);
// chemin de câbles d'une file sur un tronçon : m = milieu du tronçon sur la sous-face de la dalle, longueur L ; tiges tous les 2 m
void cheminCables(Geo& uni, vec3 m, vec3 r, vec3 f, float L, bool lod, vec2 uvAcier);

} // namespace wfs::carte::tunnel
