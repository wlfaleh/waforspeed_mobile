# WFS-94 — Corniche : quartiers Port de plaisance, Côte rocheuse et Village de pêcheurs + ville de nuit : façades sombres et fenêtres allumées (fin d'ANO-37)
Épic : E2 · Priorité : P2 Haute · Dépend de : WFS-93 ✅ (2026-11-05) · Taille : 3 jours (2 jours de corniche + ANO-40 + 1 jour de façades de nuit)

## Besoin
La Corniche (`src/world/plage.cpp`) porte les bas-côtés de chaque zone mais pas leurs objets : pas de villas côté terre,
pas de bateaux ni de bornes d'amarrage dans la marina, pas de rochers, de pins ni de glissières sur la côte rocheuse, pas
de maisons ni de barques au village, pas de tour de sauveteur. La carte paraît vide comparée au jeu web.

## Règles de gestion
- RG-01 : `beach.corniche` : villas côté terre (`villa`, RW + 12-18, une fois sur 0,75), palmiers côté terre, tours de
  sauveteur (`tower`, tous les 160-240 tronçons, RW + 22) ; palmiers et parasols côté mer déjà faits.
- RG-02 : `beach.marina` : bornes (`bollard`) tous les 3 tronçons à RW + 6,5, bateaux (`boat`, 7-14 m, voile 6 fois sur 10,
  4 couleurs) à RW + 13-31, villas colorées côté terre à RW + 7 avec commerces (`placeShop`, déjà porté par WFS-71).
- RG-03 : `beach.rochers` : rochers (`rock`, taille 1-4,5) côté mer, pins et arbres ronds côté terre (`vegetation.*`),
  glissière (`guard`) tous les 3 tronçons à RW + 1,2.
- RG-04 : `beach.village` : maisons (`villa`) des deux côtés, barques (`boat` sans voile) à RW + 30-40.
- RG-05 : formes et couleurs = `DECOR.villa`, `tower`, `bollard`, `boat`, `rock`, `guard` de `decor.js` (primitives de
  `decor_carte.*`) ; ≤ 300 appels de dessin.

## À réutiliser du jeu web
`js/world/decor.js` : `beach.corniche`, `marina`, `rochers`, `village` (l. 99-148), `DECOR.villa`, `tower`, `bollard`,
`boat`, `rock`, `guard`. Natif : `decor_carte.*` (villa déjà portée pour la campagne), `vegetation.*`, `commerces.cpp`.

## Critères d'acceptation
- [ ] `carte-plage-marina`, `carte-plage-rochers`, `carte-plage-village` : objets de la zone visibles ; `carte-plage-jour`
  avec villas côté terre ; ≤ 300 appels de dessin ; tests CPU verts

## Ajout du PO (2026-11-04) — ANO-40 (mineure) : captures de la vie de la plage lisibles
- RG-A1 : banc `vie-portique` pris à **25 m** du portique « CHECKPOINT 1 » (texte lisible) et `vie-plage` à **15 m** de la plage
  (serviettes et baigneurs visibles) ; seul le cadrage du banc change (banc de `src/world/commerces.cpp`, ajouté à la zone de ce ticket).
- Critère : - [ ] `vie-portique` : « CHECKPOINT 1 » lisible ; `vie-plage` : serviettes et baigneurs visibles.

## Ajout du PO (2026-11-05) — ANO-37 (fin) : ville de nuit, façades sombres et fenêtres allumées
Constat de WFS-97 (Copilot dc0cdd3f, banc) : l'ambiance de nuit est corrigée. Mais les immeubles du fond de rue restent clairs
dans l'axe des phares et des lampadaires : `ville.cpp` les éclaire comme de jour. Au web, la nuit, les façades sont **non
éclairées** (`MAT.bld` = `MeshBasicMaterial` + texture de nuit `TEXS.facade`, `js/world/renderer.js` l. 304 et 741).
- RG-A2 : la nuit (`WFS_MOMENT` = `nuit`, lu comme dans `traffic_system.cpp` l. 27 ; la carte est reconstruite quand le
  moment change, `game.h` l. 39), les façades de la ville prennent les couleurs de la texture de nuit du web
  (`makeTextures`, `js/world/renderer.js` l. 85-99) : fond `#0d0f17`, fenêtres éteintes `#161a26`, coin des toits
  `#07080c`. Elles restent sombres même dans l'axe des phares et des lampadaires.
- RG-A3 : fenêtres allumées comme au web, au hasard mais toujours les mêmes (graine fixe, comme `mulberry32(31)`) :
  - 24 % chaudes, rgb(95-115, 68-82, 40-50) ;
  - 8 % froides, rgb(55-70, 78-92, 100-115) ;
  - 1,5 % sarcelle `#007f8c` ou orange `#8c3b00`.
  Elles se placent sur la grille de fenêtres de la façade native (`facadeTexture` : 2 fenêtres par tuile de 4 m), ou
  sur celle du web (une case = 3,2 m × 3,4 m). Elles brillent **sans dépendre des phares**.
- RG-A4 (moyen imposé, sans toucher au moteur) : le moteur n'a pas de carte émissive. `makeMaterial(m, color, normal, mr)`,
  et `scene.frag` ajoute une émission **constante** par matériau (`col += pc.emissive.rgb`). Les fenêtres allumées sont donc
  de **petits quads** posés sur les façades. Ils utilisent 2 ou 3 matériaux émissifs (chaud, froid, rare), avec une couleur
  convertie par `effets::versHdr` (`src/render/effets_particules.h`, à inclure sans le modifier). Ils sont regroupés avec
  la géométrie de chaque morceau de carte : 3 appels de dessin de plus par morceau au plus. Interdit : `renderer.*` et
  `shaders/**` (WFS-100).
- RG-A5 : de jour, rien ne change (`facadeTexture` actuelle) ; le critère des 300 appels de dessin tient toujours ; la
  vitesse d'affichage ne baisse pas de façon visible au banc `qualite-*`. Vitrines du rez-de-chaussée éclairées 2 immeubles
  sur 3 la nuit (`js/world/decor.js` l. 678, `BLD_C.shopLit #7a5a34`), seulement si les vitrines existent déjà dans le
  natif.
- Critère : - [ ] `course-nuit-ville-*` : façades sombres jusqu'au fond de rue, même dans l'axe des phares, et environ une
  fenêtre sur trois allumée (surtout chaudes, quelques froides). Les néons ressortent. Captures de jour inchangées.
- Zone ajoutée : `src/world/ville.cpp` (façades et fenêtres de nuit seulement) ; `#include "effets_particules.h"` permis.

## Hors périmètre
Bateaux animés, mer animée.

## Zone de code
`src/world/plage.cpp`, `src/world/cartes_banc.cpp`, nouveaux fichiers `src/world/plage_*.cpp`, `src/world/commerces.cpp` (banc, ANO-40), `src/world/ville.cpp` (façades et fenêtres de nuit, ANO-37). Interdit : `src/render/**` (sauf l'inclusion de `effets_particules.h`), `shaders/**`, `src/game/**`, `src/platform/**`, iOS.
