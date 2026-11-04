# WFS-97 — Nuit du circuit : taches de lumière visibles sous les projecteurs
Épic : E4 · Priorité : P3 Moyenne · Dépend de : — (WFS-09, WFS-81 ✅) · Taille : 1 jour

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
