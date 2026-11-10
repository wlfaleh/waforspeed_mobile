Agent : Claude, agent dev3d 6 (01UY1Vv3MV)

**Ticket** : WFS-363 — règles : `regles-de-gestion/WFS-363.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : les voitures doivent être sans marque, y compris par leur forme (« et les autres voitures ? », 2026-11-09) ; ce ticket retire les signatures de forme de la Giulia (écusson en V au centre, prises d'air en trèfle) et du Range Rover Sport (calandre en nid d'abeille, phares en fente très fine ; le lettrage du capot était déjà effacé par WFS-192).
- **Ce qui change à l'écran** :
  - **Giulia** (3 retouches) : (1) l'**écusson central en V est refermé par une peau de carrosserie** (même peinture), **cousue au bord découpé** : ses sommets de bord sont ceux de la peinture voisine, au 1/10 de mm. Il n'y a donc ni joint ni saut d'éclairage qui redessinerait le V (défaut n° 2 de la M4 en revue #323). Le haut rejoint en douceur l'arrondi vers le capot. (2) Les deux **prises d'air en trèfle** et la pointe du V forment maintenant **une seule grille basse et large** à 8 lames chanfreinées, en plastique noir satiné, 1,5 cm derrière la carrosserie et toujours derrière la plaque. (3) Le **socle rond de l'ancien logo**, en haut de l'écusson, est retiré.
  - **Range Rover Sport** (2 retouches) : (1) dans le cadre de la calandre (gardé), le **nid d'abeille allongé est remplacé par 4 lames horizontales épaisses** (grille ajustée, fond noir mat). (2) **Phares plus hauts** : le bandeau avant entier (phares et cadre de calandre, x ±0,905 m) est étiré ×1,3 vers le bas, pivot en haut des phares. C'est la même transformation pour les 8 matériaux de la zone (peinture, cadre, verre, feux de jour, pièces internes). Elle est exactement linéaire dans la boîte, qui contient tout le bloc optique, avec un fondu de 12 cm dans la peinture autour : jonctions fermées, aucun pli, et la fente très fine de la marque disparaît. Deux essais écartés : une boîte par phare (le montant du cadre se tordait en crochet) et un fondu en profondeur (de grands triangles traversaient la barre du feu de jour).
  - Rien d'autre ne change : arrière, habitacle, jantes, plaques, autres voitures.
- **Ce qui change dans les fichiers** :
  - `tools/retoucher_formes.mjs` :
    - nouvelle retouche **`couture`**. Elle reprend tels quels les sommets du bord découpé qui sont sur le contour, et ajoute des points libres là où aucune carrosserie ne longe le contour (au-dessus de la grille). L'intérieur est une **triangulation de Delaunay** (bord + grille de 1 cm). La surface est une base de degré 2 ajustée sur la **face** du bouclier (option `baseFace`), plus l'écart interpolé par Laplace puis **biharmonique** (hauteur et pente raccordées), en **mode « exact »**. Un contrôle vérifie qu'aucun sommet n'est devant la carrosserie ;
    - ajustement de la base **factorisé** (`baseAjustee`), sans changer le panneau ;
    - `surfaceOuverture` : options `exact` et `biharmonique`, inactives par défaut.
  - `tools/formes.json` : entrées `giulia` (13 découpes, 1 couture, 1 grille) et `rrsport` (8 déplacements, 2 découpes, 1 grille), avec empreintes avant / après et nombres de contrôle.
  - `assets/cars/giulia.*`, `assets/cars/rrsport.*` : modèles retouchés, LOD régénérés.
- **Comment c'est vérifié** :
  - `node tools/retoucher_formes.mjs --verifier` : vert pour **compass, p208, x5, m4, s3, giulia, rrsport**. Giulia et Range Rover sont rejoués depuis leur modèle d'origine. **M4 et S3, rejouées depuis leur origine (historique git) avec le nouvel outil, donnent exactement leurs empreintes** (`7c66e923c1debae9`, `826c25570405adee`). Le journal de l'outil dit « aucun devant la carrosserie » (peau et grilles) et « derrière la plaque » (Giulia).
  - Construction complète et `bash tools/qa/tests_cpu.sh build` sur la branche synchronisée avec `main` : **98 tests réussis**. Dans `test_fidelite_voitures`, toutes les mesures des deux voitures restent **« conforme » (±1 %)**.
  - Captures avant / après regardées une par une (vitrines jour, feux), puis gros plans ×4 jour et nuit des zones retouchées.
- **Limites et risques connus** :
  - **Giulia** : le nez refermé avance d'environ 1,2 cm au centre (galbe prolongé de la face voisine). La longueur brute fait +0,25 % et l'échelle globale passe de ×1,000 à ×0,999 ; toutes les mesures restent conformes. La pointe du capot au-dessus de l'ancien écusson et les petites lèvres au bout de la grille appartiennent au modèle d'origine (gardées).
  - **Range Rover** : le **motif en nid d'abeille des prises d'air basses** (ouverture en U, non convexe) et les 4 petits pavés de LED des phares sont gardés : ils ne sont pas dans RG-01 et ne suffisent pas à reconnaître la marque. Je peux les proposer au PO en tâche séparée (RG-06).
  - Triangles : Giulia 134,1 k → **133,2 k** ; Range Rover → **110,6 k**. Poids : 6,9 Mo et 6,8 Mo (≤ 8 Mo).
  - Pas d'APK ni de CI de branche (règle) ; rendu vérifié au banc PC (lavapipe, même rendu que la CI). Les gros plans avant / après sont faits avec le même binaire, seuls les modèles changent.

## Captures pour le lead dev (16 après)

Dossier : [`qa/pr/WFS-363`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `vitrine-giulia-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-giulia-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-giulia-0.jpg?raw=true) |
| `vitrine-giulia-a0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-giulia-a0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-giulia-a0.jpg?raw=true) |
| `vitrine-giulia-a1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-giulia-a1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-giulia-a1.jpg?raw=true) |
| `vitrine-giulia-a2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-giulia-a2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-giulia-a2.jpg?raw=true) |
| `vitrine-giulia-bas` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-giulia-bas.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-giulia-bas.jpg?raw=true) |
| `vitrine-giulia-bas1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-giulia-bas1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-giulia-bas1.jpg?raw=true) |
| `vitrine-giulia-bas2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-giulia-bas2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-giulia-bas2.jpg?raw=true) |
| `vitrine-giulia-feux` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-giulia-feux.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-giulia-feux.jpg?raw=true) |
| `vitrine-rrsport-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-rrsport-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-rrsport-0.jpg?raw=true) |
| `vitrine-rrsport-a0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-rrsport-a0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-rrsport-a0.jpg?raw=true) |
| `vitrine-rrsport-a1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-rrsport-a1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-rrsport-a1.jpg?raw=true) |
| `vitrine-rrsport-a2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-rrsport-a2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-rrsport-a2.jpg?raw=true) |
| `vitrine-rrsport-bas` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-rrsport-bas.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-rrsport-bas.jpg?raw=true) |
| `vitrine-rrsport-bas1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-rrsport-bas1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-rrsport-bas1.jpg?raw=true) |
| `vitrine-rrsport-bas2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-rrsport-bas2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-rrsport-bas2.jpg?raw=true) |
| `vitrine-rrsport-feux` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-vitrine-rrsport-feux.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-vitrine-rrsport-feux.jpg?raw=true) |

### Zooms ×4 des zones retouchées, jour et nuit (même binaire, seuls les modèles changent)

Giulia : centre du nez de face (ancien écusson), bas de la grille au centre (ancienne pointe du V), bout de la grille de 3/4 ; Range Rover : calandre de face, phare droit de face, phare et cadre de 3/4.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `zoom-giulia-nez-face-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-zoom-giulia-nez-face-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-zoom-giulia-nez-face-jour.jpg?raw=true) |
| `zoom-giulia-nez-face-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-zoom-giulia-nez-face-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-zoom-giulia-nez-face-nuit.jpg?raw=true) |
| `zoom-giulia-grille-bas-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-zoom-giulia-grille-bas-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-zoom-giulia-grille-bas-jour.jpg?raw=true) |
| `zoom-giulia-grille-bas-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-zoom-giulia-grille-bas-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-zoom-giulia-grille-bas-nuit.jpg?raw=true) |
| `zoom-giulia-34-grille-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-zoom-giulia-34-grille-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-zoom-giulia-34-grille-jour.jpg?raw=true) |
| `zoom-giulia-34-grille-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-zoom-giulia-34-grille-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-zoom-giulia-34-grille-nuit.jpg?raw=true) |
| `zoom-rrsport-calandre-face-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-zoom-rrsport-calandre-face-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-zoom-rrsport-calandre-face-jour.jpg?raw=true) |
| `zoom-rrsport-calandre-face-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-zoom-rrsport-calandre-face-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-zoom-rrsport-calandre-face-nuit.jpg?raw=true) |
| `zoom-rrsport-phare-face-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-zoom-rrsport-phare-face-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-zoom-rrsport-phare-face-jour.jpg?raw=true) |
| `zoom-rrsport-phare-face-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-zoom-rrsport-phare-face-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-zoom-rrsport-phare-face-nuit.jpg?raw=true) |
| `zoom-rrsport-34-phare-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-zoom-rrsport-34-phare-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-zoom-rrsport-34-phare-jour.jpg?raw=true) |
| `zoom-rrsport-34-phare-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/avant-zoom-rrsport-34-phare-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-363/qa/pr/WFS-363/apres-zoom-rrsport-34-phare-nuit.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv3MVhKQapjuowdFE2c
