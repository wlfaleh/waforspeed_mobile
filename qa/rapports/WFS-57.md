# QA WFS-57 : centrer la paire « Rejouer » / « Menu » de l'écran de fin (correction d'ANO-14)

**Verdict : VALIDÉ** (banc d'essai PC), 1er passage, aucune anomalie. **Fusion autorisée.** ANO-14 est corrigée.

- Construction regardée : commit `cc76b01` (`feature/waforspeed-57`, PR #29), CI au vert (run 41690135163 : `banc-essai`,
  `android`, `publication`). Release `apercu-waforspeed-57` (« commit cc76b01 »), 61 captures et `WaForSpeed.apk`.
- La branche se fusionne sur `main` (`e5bccb1`) sans conflit (`git merge-tree`).
- Règles : `regles-de-gestion/WFS-57.md` (RG-01 à RG-05).
- Code changé : `src/game/course_system.cpp` (`drawOver` pose deux rectangles `rejouer_` / `menu_`, `onTouch` les teste avec
  `Bouton::contient`) et l'accroche de 2 lignes de `benchMenuTactile` (`src/ui/ecrans/menu.cpp`). `tools/qa/**` n'est pas touché.

## Captures

| Capture | Constat |
|---|---|
| `fin-menu` | Paire centrée sur le panneau. Mesures à mi-hauteur du bouton (y = 540) : « Rejouer » orange 395 → 668, « Menu » #1E2130 697 → 884, **milieu de la paire = 639,5** ; boîte des biseaux 388 → 892, milieu 640. Panneau 208 → 1071 : marge gauche 187 px, marge droite 187 px. Écart entre boutons 29 px en largeur visible (14 px de rectangle + biseau), comme sur `main`. Textes « REJOUER » / « MENU » centrés dans leur bouton, couleurs et hauteur inchangées. |
| `course-fin` | Même paire, mêmes mesures (milieu 639,5). Score, stats et panneau identiques à `main`. |
| `fin-retour-menu` | Menu principal affiché, record « 390 » = score de `fin-menu` : l'appui « Menu » du banc (x ≈ 791, y = 540) tombe dans le nouveau rectangle « Menu » (690 → 892). Identique au pixel à `main`. |
| Les 58 autres captures | Comparaison pixel par pixel avec `apercu-waforspeed-38` (`60cde90`) : identiques (aucun pixel au-delà de 8/255). Seules `fin-menu` et `course-fin` changent, dans la zone 388 → 1000 × 504 → 576 (les boutons) uniquement. |
| Tests du banc | `mode exact : 39 voitures… 0 écart(s)`, `plage`, `campagne`, `circuit` : 0 écart(s), `course automatique : 22 lignes… 0 écart(s)`. Les lignes `[ERREUR] cars/golf.wfm` sont celles, voulues, de la capture `chargement-erreur`. |

## Critères d'acceptation et règles

| Critère / règle | Constat | Résultat |
|---|---|---|
| `fin-menu` et `course-fin` : milieu de la paire à x = 640 ± 2, marges égales | 639,5 ; 187 / 187 px | OK |
| `fin-retour-menu` : toujours le menu, record à jour | menu, record 390 | OK |
| Autres captures identiques à `main`, banc `0 écart(s)`, ANO-14 n'est plus visible | voir Captures | OK |
| RG-01 : départ `bx = cx − (1,7·bw + 2·u)/2`, Rejouer 388 → 676, Menu ≈ 690 → 892 | code `bx = cx − (bw + gap + mw)/2` (mw = 0,7·bw, gap = 2·u) ; mesuré 388 → 892 | OK |
| RG-02 : aucune autre valeur ne change | largeurs, hauteur, y, biseau, couleurs, délai 0,9 s inchangés | OK |
| RG-03 : zones de toucher = rectangles dessinés, plus de facteurs 1,05 / 1,75 | `onTouch` teste `rejouer_` / `menu_` posés par `drawOver` | OK |
| RG-04 : accroche ≤ 2 lignes dans `benchMenuTactile`, annoncée au Journal, `tools/qa/emulateur.sh` intact | 2 lignes, annoncée (`e5bccb1`), `tools/qa/**` non modifié ; l'appui x ≈ 791 du scénario simulateur tombe désormais dans « Menu » | OK |
| RG-05 : seules les captures de l'écran de fin changent | `fin-menu`, `course-fin` | OK |

## Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — | — | — | Aucune anomalie. |
