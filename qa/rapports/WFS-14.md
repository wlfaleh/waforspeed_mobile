# QA WFS-14 : Cartes, socle multi-cartes et Corniche (plage)

**Verdict : À CORRIGER (tour 1/2)**, banc d'essai PC, 1er passage. **Pas de fusion** pour l'instant.
Aucune anomalie bloquante. Une anomalie mineure (ANO-13) est à corriger par le développeur.

## 1er passage : commit `915d1e6`

- Construction regardée : commit `915d1e6` (`feature/waforspeed-14`), PR #19. Run Actions 41627083861 réussi (android,
  banc-essai, publication). Release `apercu-waforspeed-14` (« commit 915d1e6 »), 44 captures.
- Référence de non-régression : Release `apercu-waforspeed-29` (commit `3a87933`). `git diff --stat 3a87933 2b0b4ff -- src shaders assets`
  est vide : le code est identique à la base de la branche (`git merge-base` = `2b0b4ff`).
  Comparaison pixel par pixel (PIL) : **les 42 captures communes sont identiques** (vitrines, course, trafic, ombres,
  chargement, texte, scène dédiée, décompte, fin). Les seules nouvelles captures sont `carte-plage-jour` et `carte-plage-nuit`.
- Diff relu : `src/world/plage.cpp`, `zones.*`, `vegetation.*` (palmier) sont dans la zone E2. Accroches annoncées au journal :
  `system.h` (+4, `System::ambiance`, `System::remplaceSol`) et `game.cpp` (+3/−1), soit moins de 10 lignes ; `desktop_main.cpp`,
  `CMakeLists.txt` et `build.yml` (test). Placement par `Mulberry32(seed · 31 + 7)`, comme `generateDecor`, avec les pas
  du jeu web pour les palmiers et les parasols. Parasols et transats omis dans les morceaux à plus de 200 m (`!lod`).

| Critère / règle | Résultat |
|---|---|
| Critère 1 : `carte-plage-jour` | OK. On voit l'océan à droite (côté mer, bleu clair qui se perd dans la brume), la promenade `#b8ab8c`, le sable `#cdb683`, le sable mouillé et des parasols de 5 couleurs avec leurs transats. Des palmiers bordent les deux côtés (stipe courbé et annelé, palmes en V qui retombent, gaines et noix de coco). Côté terre, on voit la dune `#9aa65e` puis la colline `#7a8a4c`. Le brouillard lointain `#cfe3f0` est visible : les palmiers et la route s'effacent au loin. Les ombres des palmiers tombent sur le sable. Rien ne flotte, l'image n'est pas déformée et la voiture est visible. Voir ANO-13 pour le stipe. |
| Critère 1 : `carte-plage-nuit` | OK. Le ciel est violet sombre, le brouillard bleuté (route au loin : (29, 33, 65), même teinte que `#141428`, un peu éclaircie comme l'annonce `plage.cpp`, k = 2,2). La mer sombre reste distincte du sable. Les palmiers se voient en silhouette, sans ombres. La nuit n'a pas d'éclairage : c'est le périmètre de WFS-09. |
| RG-01 : table `MAPS` | OK. `src/game/track.cpp` contient les mêmes valeurs pour plage, campagne et circuit (seed, curves, hills, sections, edge, trafic). |
| RG-02 : zones | OK. `ZONE_LEN = 448`, avec les 4 thèmes beach dans l'ordre du jeu web. La dernière zone prolonge la précédente. `zoneName` est exposé. |
| Critère 2 : fidélité du tracé | OK. Journal banc-essai : `plage : 4369 segments comparés au jeu web, 0 écart(s) (écart max 7.6e-08)`, `campagne : 5479 …, 0 écart(s) (écart max 0.00046)`, `circuit : 5061 …, 0 écart(s) (écart max 2.6e-05)`, `OK`. Tous les segments sont comparés et leur nombre est identique. La tolérance est de 1e-3 unité (6 µm) au lieu de 1e-9. Le développeur a mesuré l'écart et en a trouvé la cause : `PI` en float dans `easeInOut`, dans le code E3 hors zone. La correction changerait 27 captures, donc elle est reprise par **WFS-55** (E3, test à 1e-9). L'écart est invisible et il est suivi : il n'est pas classé comme anomalie de WFS-14. |
| Critère 3 : appels de dessin | OK. Journal : `Plage : 108 appels de dessin (25 morceaux)` de jour et de nuit (≤ 300) ; `Plage : 469 palmiers, 268 parasols`. |
| Critère 3 : autres captures | OK. Les 42 captures sont identiques au pixel près. |
| RG-06 : ville par défaut | OK. `WFS_CARTE` est lu par le banc et la ville reste la valeur par défaut. |

Remarques (non classées en anomalie, hors règles de gestion) : le côté terre est vide. Villas, tours de sauveteurs, bateaux,
lampadaires, paillotes et baigneurs de `generateDecor` ne sont pas portés, ce que la PR annonce comme limite. Les pentes de dune
et de colline forment donc un grand pan vert uni. Le PO peut en faire une US de décor. Les noix de coco sont placées avec un
tirage séparé (`Rn`) et non avec le `R` du jeu web : l'écart n'est pas visible.

## Anomalies

| ANO | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-13 | Mineure | `carte-plage-jour` (palmier au premier plan à droite, x ≈ 1000-1030, et tous les stipes) | Les stipes des palmiers sont rendus **presque noirs, gris anthracite** : (43, 44, 45) au milieu du stipe et (72, 64, 55) au plus clair. Le jeu web utilise un brun moyen, `BARK.palm #7d6a52` et `palm2 #5e5040`. Pourtant, sur la même image, le sable a sa couleur d'origine ((195, 189, 165) pour `#cdb683`). Le stipe est donc environ deux fois trop sombre et il perd sa teinte brune. Les palmiers ressemblent à des poteaux noirs, ce qui nuit au réalisme de la plage de jour. À corriger dans `plage.cpp` / `veg::palmier` : la couleur du stipe (texture 2 × 1 et teinte de sommet `MAT_VTINT`) doit donner le brun de `BARK.palm` au soleil, comme le sable. |

Tour de correction : **1/2**. Prochaine étape : le développeur corrige ANO-13 et relance la CI. Le QA regarde ensuite les nouvelles
captures (`carte-plage-*`, et les 42 autres captures, qui doivent rester identiques à `main`).
