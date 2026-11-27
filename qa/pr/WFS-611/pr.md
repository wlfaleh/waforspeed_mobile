## Agent : Claude, lead dev (01UY1Vv1NH)

**Ticket** : WFS-611 — règles : `regles-de-gestion/WFS-611.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : « minimum 40 images/s avec une qualité top d'image et de résolution, sans coupure, sans lag » (aujourd'hui 8 à 30 img/s), l'objectif de 60 img/s restant pour toujours.
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - **Shader de scène spécialisé par famille d'objets** : `scene.frag` (≈ 20 000 instructions SPIR-V) faisait payer à chaque pixel les registres du pire cas. Nouvelle constante de spécialisation `MASQUE_FAMILLE` (`shaders/common.glsl`, fonction `aF`) : chaque famille (route, trottoir, sol, décor, voiture, vitre) est le même shader dont le pilote retire au chargement tout ce que la famille ne dessine pas. Instructions après spécialisation (`tools/qa/taille_shaders.sh`) : complet 19 959 → route 7 409, trottoir 4 769, sol 8 049, décor 4 030, voiture 5 330, vitre 5 391.
  - `src/render/familles_pipeline.h` (nouveau, pur) : masques et choix de la famille (la plus petite qui sait dessiner TOUS les drapeaux de l'objet, sinon le shader complet d'avant) ; `renderer.cpp` : pipelines par famille créés au chargement (opaques, découpe, deux faces, léger, transparents, passe de reflet), choisis à chaque appel de dessin ; `WFS_FAMILLES=0` les coupe (banc).
  - **Écran en 120 Hz** : `JeuActivity.java` lit la fréquence maximale de l'écran et la passe au moteur (`nativeFrequenceEcran`, `android_main.cpp`) ; `chauffe_android.cpp` demande 120 Hz si l'écran le permet (sinon 60). Avec la synchro FIFO en 60 Hz, seules 60 / 30 / 20 img/s sont possibles ; en 120 Hz, une image de 20 à 25 ms s'affiche à 40 img/s au lieu de tomber à 30. L'objectif de course reste 60 img/s (`cadenceCourse` et budget inchangés) : 40 est un plancher, jamais un plafond.
  - À l'écran : rien ne change (même image au pixel près).
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) :
  - Nouveau test `test_familles_pipeline` : 200 000 objets tirés au hasard, la famille choisie sait toujours dessiner tous leurs drapeaux ; cas réels ; valeurs `F_*` des shaders = drapeaux du C++ ; masque et constante 2 présents dans les shaders.
  - Tests CPU : **160 test(s) réussi(s)** (commit 5d9fe3c), journal complet `qa/pr/WFS-611/tests-cpu.log`.
  - Captures avant (main) / après (branche) : **19 / 20 identiques à l'octet près** (départ de jour Ville et Campagne, de nuit Ville, plage et Campagne ; halos de nuit). `halos-poursuite` diffère seulement parce que la simulation de la Poursuite n'est pas rejouée à l'identique (police à 37 m contre 24 m, autre moment de la course) : le rendu est normal. Banc nuit complet de l'étape précédente : 82 / 84 identiques (les 2 autres : même cause).
  - APK d'essai construit par la CI (run #1167, branche `feature/waforspeed-613` qui réunit 611, 612 et 613) : compilation, tests et APK verts ; donné à l'utilisateur avant la fusion.
- **Limites et risques connus** :
  - Le gain de temps n'est mesurable que sur le téléphone (le GPU logiciel du banc ne réserve pas de registres) : ticket 🟡 À valider (utilisateur) après fusion.
  - Plus de pipelines créés au chargement (6 familles × 6 sortes + passe de reflet) : un peu plus de temps au premier lancement (le cache des pipelines le garde ensuite) ; aucun pipeline créé pendant la course (RG-04).
  - Un objet dont les drapeaux ne tiennent dans aucune famille garde le shader complet d'avant (aucun risque d'image fausse, seulement pas de gain pour lui).

## Captures pour le lead dev (20 paires ciblées, jour et nuit)

Dossier : [`qa/pr/WFS-611`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611) — produit par `tools/qa/captures_pr.sh`.

Captures « avant » : `origin/main` = `d088626744d31ebcafabca989a22943b0b804b99` ; cache : 0 banc(s) relu(s), 2 banc(s) rejoué(s) ; main compilé : oui.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `depart-jour-campagne-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-depart-jour-campagne-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-depart-jour-campagne-0.jpg?raw=true) |
| `depart-jour-campagne-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-depart-jour-campagne-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-depart-jour-campagne-d3.jpg?raw=true) |
| `depart-jour-ville-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-depart-jour-ville-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-depart-jour-ville-0.jpg?raw=true) |
| `depart-jour-ville-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-depart-jour-ville-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-depart-jour-ville-d3.jpg?raw=true) |
| `depart-nuit-campagne-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-depart-nuit-campagne-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-depart-nuit-campagne-0.jpg?raw=true) |
| `depart-nuit-campagne-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-depart-nuit-campagne-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-depart-nuit-campagne-d3.jpg?raw=true) |
| `depart-nuit-plage-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-depart-nuit-plage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-depart-nuit-plage-0.jpg?raw=true) |
| `depart-nuit-plage-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-depart-nuit-plage-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-depart-nuit-plage-d3.jpg?raw=true) |
| `depart-nuit-ville-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-depart-nuit-ville-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-depart-nuit-ville-0.jpg?raw=true) |
| `depart-nuit-ville-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-depart-nuit-ville-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-depart-nuit-ville-d3.jpg?raw=true) |
| `halos-160-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-halos-160-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-halos-160-0.jpg?raw=true) |
| `halos-160-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-halos-160-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-halos-160-1.jpg?raw=true) |
| `halos-160-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-halos-160-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-halos-160-2.jpg?raw=true) |
| `halos-160-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-halos-160-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-halos-160-3.jpg?raw=true) |
| `halos-160-4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-halos-160-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-halos-160-4.jpg?raw=true) |
| `halos-160-5` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-halos-160-5.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-halos-160-5.jpg?raw=true) |
| `halos-50m` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-halos-50m.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-halos-50m.jpg?raw=true) |
| `halos-poursuite` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-halos-poursuite.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-halos-poursuite.jpg?raw=true) |
| `halos-pres` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-halos-pres.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-halos-pres.jpg?raw=true) |
| `halos-rue` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/avant-halos-rue.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-611/qa/pr/WFS-611/apres-halos-rue.jpg?raw=true) |

## Tests CPU

```
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 160 test(s) réussi(s) en 495 s
commit testé : 5d9fe3c
```
