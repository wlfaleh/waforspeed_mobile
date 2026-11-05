# WFS-97 — Lot anomalies NUIT : taches de lumière du circuit + façades de la ville trop claires (ANO-37)
Épic : E4 (exception : en parallèle de WFS-100, fichier séparé `nuit.cpp`) · Priorité : P1 Urgent (lot élargi, voir plus bas) · Dépend de : — (WFS-09, WFS-81 ✅) · Taille : 1 à 2 jours

## Besoin
Sur `carte-circuit-nuit` et `course-nuit-circuit-0` de `main`, les 135 mâts de projecteurs brillent mais la piste reste
uniformément grise : on ne voit pas les grandes taches de lumière froide au sol que le jeu web dessine sous chaque
projecteur (`poolQuad(ctx.pool, p, s * (RW - 3), 14, C.poolCool)`). Le code existe (`src/render/nuit.cpp`, RG-05 de
WFS-09, style `Stade`) mais la tache est presque invisible. Cause probable : la couleur du web `poolCool [0.35, 0.38,
0.45]` est passée en linéaire (`lin()` → ≈ 0,1) alors que three r128 utilise les couleurs de sommet telles quelles (sans
gestion des couleurs), et `MAT.pool` est additif à l'opacité `poolK` = 1 la nuit.

## Règles de gestion
- RG-01 : sous chaque projecteur du circuit, à moins de 500 m, une tache additive de demi-taille 14 m centrée à
  `side × (RW − 3)`, texture radiale, couleur et intensité équivalentes au web (bleu-blanc froid, nettement visible sur
  l'asphalte sombre, bords doux).
- RG-02 : même correction pour les flaques des lampadaires de ville et de plage si elles souffrent du même passage en
  linéaire, sans rendre la ville (`course-nuit-ville-*`) éblouissante : comparer aux captures actuelles.
- RG-03 : coût négligeable (mêmes quads qu'aujourd'hui), aucun appel de dessin en plus par tache.

## À réutiliser du jeu web
`js/world/decor.js` : `DECOR.lamp` (style `flood`, l. 335-342), `poolQuad` (l. 701), palette `C.pool`, `C.poolCool` (l. 305) ;
`js/world/renderer.js` l. 311 (`MAT.pool` additif) et l. 467 (`opacity = poolK`) ; `js/world/maps.js` (`poolK` 1 la nuit).

## Critères d'acceptation
- [ ] `carte-circuit-nuit`, `course-nuit-circuit-0` : taches de lumière froides visibles sur la piste sous les mâts
- [ ] `course-nuit-ville-*`, `nuit-plage-corniche` : flaques visibles, pas de surexposition ; captures de jour inchangées

## Hors périmètre
Vraies lumières dynamiques par projecteur, ombres des projecteurs.

## Zone de code
`src/render/nuit.cpp` (E4). Interdit : `src/world/**` (WFS-93), `src/platform/**` (WFS-91), fichiers iOS (gel iOS).


## Lot élargi (décision de l'utilisateur, 2026-11-04 : anomalies prioritaires, gros lots)
- Ce ticket devient le **lot « nuit »** : il corrige aussi **ANO-37** (nuit en ville : façades trop claires, on dirait le crépuscule). Correction proposée par le QA : hémisphère de nuit × 0,45, façades #1A1C28, 30 % des fenêtres émissives au hasard (#FFD9A0 et #9F…), néons et reflets qui ressortent. Priorité passée en **P1 Urgent**. Une seule branche, une seule CI.
- Coordonner avec WFS-100 (bloom, en cours) et WFS-84 / WFS-109 (feux).

## Précisions du PO à l'attribution (2026-11-05, Copilot session dc0cdd3f)
- RG-04 (ANO-37, nuit de ville et de plage) : corriger dans `ambiance()` de `nuit.cpp` uniquement. Proposition du QA :
  hémisphère de nuit × 0,45 (ville ; plage si même défaut), teinte moins violette si les façades restent lavande / rose.
  Valeurs du web pour comparer : `envFor` nuit de `js/world/maps.js` (l. 23-29 : `hemiSky #46327a`, `hemiGround #08080e`,
  `hemiI 0.75` ; étalonnage nuit `exposure 0.9`, l. 45). Le natif met `fp.exposure = 1.1`, `fp.ambient = 0.7` et éclaircit
  ciel et brouillard × 2,2 (`k`) : vérifier ces trois leviers **en premier** au banc. Résultat attendu : façades sombres ;
  voiture, néons, flaques et reflets qui ressortent ; captures de jour inchangées. Noter les valeurs choisies au Journal.
- RG-05 (conversion des couleurs du web) : three r128 utilise les couleurs de sommet et de sprite **telles quelles** (sans
  gestion des couleurs). Pour retrouver l'éclat du web (taches `poolCool`, flaques), **réutiliser `effets::versHdr` /
  `acesInverse` de `src/render/effets_particules.h`** (WFS-107 : inverse exact de l'ACES du web) : inclure ce fichier sans
  le modifier. Ne pas écrire une autre conversion.
- RG-06 (hors périmètre) : fenêtres éclairées la nuit. Au web : `ENV.windowsLit`, `MAT.bld` (`MeshBasicMaterial` + `TEXS.facade`
  de nuit, `js/world/renderer.js` l. 304 et 741), vitrines `shopLit #7a5a34` sur 2 immeubles sur 3 (`js/world/decor.js` l. 678).
  Les façades sont générées par `src/world/ville.cpp`, zone de WFS-93 en cours : **ne pas y toucher**. Si la ville reste trop
  claire après RG-04, le QA le note dans ANO-37 et le PO range les fenêtres dans le prochain lot E2, après WFS-93.
- RG-07 (coordination) : le bloc « RG-04 : feux du joueur » à la fin de `render()` (`FeuxVoiture`, `feuxProchaineVoiture`) est
  réservé à l'accroche de WFS-109 : ne pas le modifier. `renderer.*` (dont `haloSol`, `poolMat`) et `shaders/**` sont à
  WFS-100 : si une flaque l'exige, accroche ≤ 10 lignes annoncée au Journal. Se synchroniser avec `main` avant chaque
  poussée ; si WFS-100 (bloom) fusionne avant, refaire les captures de nuit après la synchronisation.
- Critère ajouté : `course-nuit-ville-*` : façades sombres (plus de lavande / rose), néons, flaques et reflets qui
  ressortent (ANO-37) ; valeurs notées au Journal ; tests CPU verts.
- Zone précisée : `src/render/nuit.cpp` (sauf le bloc des feux du joueur) ; `#include "effets_particules.h"` permis
  (sans modifier ce fichier) ; accroche ≤ 10 lignes dans `src/platform/desktop_main.cpp` seulement si une capture de nuit
  manque. Interdit : `src/world/**` (WFS-93), `src/render/renderer.*` et `shaders/**` (WFS-100), le reste de
  `src/platform/**` (WFS-106), `src/game/**`, iOS.
