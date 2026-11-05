# QA WFS-78 : iOS, surface Metal et démarrage jusqu'au menu

## QA après fusion, recette de nuit (2026-11-04 23:53:28 UTC, qa-lead)

**Verdict : REFUSÉ**

- **Source** : run `ios.yml` de `main` 41877869097 (commit de fusion `2ac79d4`, conclusion **failure**). Le run suivant de `main` (41879945920) a été annulé. Il n'y a aucun run iOS vert sur `main` depuis.
- Ce qui progresse : la surface Metal est créée, la swapchain fait 2622x1206 à la taille de la vue, et `QA pret : moteur initialise` est atteint en 11,6 s, donc on ne bloque plus dans `Renderer::init`.
- Ce qui bloque : le critère « démarrage jusqu'au menu » n'est pas rempli. `QA iOS : menu affiche` est absent après 120 s, le journal répète `VK_ERROR_FEATURE_NOT_PRESENT: vkCmdDrawIndexed(): … non-zero base vertex` et `vkEndCommandBuffer a échoué (-8)` (`renderer.cpp:1094`), et l'app ne produit ni `ios-menu.png` ni `ios-course.png`.
- Banc PC Android : inchangé, rien à signaler.

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — (correction déjà planifiée) | Bloquante | `ios-journal` du run 41877869097 | Pas de menu sur le simulateur iOS : les dessins avec un sommet de base non nul sont refusés par MoltenVK. **La correction existe déjà** : WFS-88, PR #76 (run `ios.yml` 41881649600 de la branche vert), que WFS-103 doit fusionner (ordre de l'utilisateur). Pas de nouveau ticket à créer ; le PO garde WFS-88 / #76 en priorité pour iOS (GEL iOS) |

À revoir au premier run `ios.yml` vert de `main` après la fusion de #76 : `QA iOS : menu affiche`, `ios-menu.png`, `ios-course.png`.
