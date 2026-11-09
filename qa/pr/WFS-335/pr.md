Agent : agent arwa (Claude, session trafic)

**Ticket** : WFS-335 — règles : `regles-de-gestion/WFS-335.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : le jeu tourne à 22-24 img/s en Ville de nuit, Basse, et les voitures font 74 % des triangles : alléger les voitures sans perte visible.
- **Décision du PO du 2026-11-09 02:35 UTC (appliquée)** : en **Basse seulement**, adversaires de **48 à 100 px** à l'écran au **niveau 2**, sans saut de forme ; **voiture du joueur toujours complète** (refus du LOD1, RG-02) ; critère « triangles ÷ 2 » retiré : le ticket se ferme sur le gain mesuré (suite de l'objectif 30 img/s : WFS-348).
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - `src/render/vehicule_lod.h` : chaîne **Basse** = complète (≥ 200 px) → niveau 1 (≥ 100 px) → **niveau 2 avec tous ses matériaux (100 à 48 px, nouveau)** → niveau 2 en 4 pièces (48 à 26 px) → niveau 3 en 4 pièces ; fondu tramé à chaque passage (bande de 1/9 du seuil : 100 → 89 px, 48 → 43 px). Le niveau 2 garde phares, calandre et jantes : une première version (5bee02c9) passait directement à la version en 4 pièces à 100 px, la comparaison des captures montrait les phares de la SF90 perdus en vue Proche, d'où 2fcd39a7. Police en lots (Basse) : 4 pièces de près, niveaux 2 et 3 en 4 pièces au loin, passage à 100 px. Haute / Moyenne, fantôme, **voiture du joueur (`dessinJoueur`) : inchangés**.
  - `src/render/model_loader.cpp` (accroche de 12 lignes, annoncée au journal) : regroupement en 4 pièces aussi pour les niveaux 2 et 3 (`partsBasseNiveaux`), un seul matériau sombre partagé.
  - `src/render/renderer.h/.cpp` : champ `Model::partsBasseNiveaux` ; relevé voiture par voiture (banc seulement, `releverVoitures`).
  - `src/render/bench_perf.cpp` : Course à 5 au banc des parties (`ville:nuit:course5`) et lignes « voitures : … » (RG-01).
  - `src/render/bench_lod.*` : captures WFS-335 (trafic de nuit en Basse de 60 à 170 m ; **passage d'un adversaire de 110 à 40 px** `lod-passage-*` (revue du lead dev), distance réglée par le relevé du renderer ; Course à 5 de nuit en Basse, vues Arrière et Proche).
  - `tests/test_vehicule_lod.cpp` : **seuil Basse testé** (101 px : niveau 1 seul ; 95 px : fondu niveau 1 → 2 ; 48 à 88,8 px : niveau 2 seul, tous ses matériaux ; 45 px : fondu vers les 4 pièces ; niveau 3 sous 26 px ; Haute à 70 px : niveau 1 ; police en lots ; modèle sans niveaux 2 / 3 en 4 pièces : seuil de 48 px gardé), niveaux croissants avec la distance, fondu seulement entre listes différentes.
- **Comment c'est vérifié** (branche synchronisée avec `main` 6aaf41b5, conflit `bench_perf.cpp` résolu en gardant les deux côtés) :
  - **83 tests CPU OK** (`bash tools/qa/tests_cpu.sh build`, 423 s, après la synchronisation), dont `test_vehicule_lod` OK.
  - **Parcours complet** (`WFS_BANC=parcours`) : **0 écart** en vue Arrière et en vue Cockpit.
  - Banc `lod` (1280x720, 54 captures, 0 erreur) avant (main 6aaf41b5) / après, comparé au pixel près (seuil 8 niveaux) : **Haute : 12 captures sur 12 identiques** ; Basse : 18 sur 31 identiques ; les 13 autres ne changent que sur des adversaires de 48 à 100 px ou lointains (Course à 5 de nuit vue Arrière 499 px, vue Proche 884 px, trafic de nuit 277 à 497 px, vues de jour à 50 m 67 à 178 px) ; voiture du joueur identique (au plus 7 pixels isolés sous la ligne d'horizon, gouttes de pluie).
  - Suite `lod-passage` (relevé du renderer) : 106 / 101 px niveau 1 (32 676 triangles) ; 96 / 90 px fondu niveau 1 → 2 ; 81 à 48 px **niveau 2, 16 615 triangles (−49 %)** ; 45 px fondu ; 40 px niveau 2 en 4 pièces (19 569 triangles) : planche ci-dessous, aucun saut de forme à l'œil (fondu tramé visible seulement en zoom ×3).
  - Banc perf avant / après : tableau ci-dessous.
- **Limites et risques connus** :
  - En vue Proche, les adversaires de 48 à 100 px (≈ 37 à 77 m) perdent un peu de finesse de silhouette (arête du toit, flancs) en zoom ×3 ; à la taille réelle, ≈ 10 px de bord. Matériaux (phares, feux, vitres, calandre) gardés. Accepté par le PO (décision 1).
  - Gain sur la Course à 5 au départ : **−26,7 %** de triangles des voitures (−17,9 % sur l'image) ; pas ÷ 2 : la voiture du joueur (109 884 triangles) reste complète (décision 2).
  - Img/s : le banc PC (lavapipe) ne mesure pas le téléphone ; à mesurer sur l'APK (Réglages, Ville de nuit, Basse). La suite de l'objectif ≥ 30 img/s est dans WFS-348.
  - Sous 48 px, la version en 4 pièces (passage déjà présent sur `main`) a un peu plus de triangles que le niveau 2 complet (19 569 contre 16 615 pour cette voiture) mais 4 appels au plus (un par groupe de matériaux) : gardée telle quelle (hors décision du PO).
  - Le relevé dit « niveau » = rang dans la chaîne (Basse : 2 = niveau 2, 3 = niveau 2 en 4 pièces, 4 = niveau 3).
  - Intérieur et dessous de caisse : 88 à 97 % des triangles visibles de l'extérieur (mesure de la 1re version) : non retirés.

## Chiffres avant / après (banc `WFS_BANC=perf WFS_PERF_PARTIES=ville:nuit:course5,ville:nuit WFS_PERF_MINUTE=600 WFS_PERF_IMAGES=20`, Basse, vue Proche)

Avant = `main` (b52e3d6, même banc) ; « 1re version » = cette PR avant la décision du PO (900bb2f9) ; après = cette PR (2fcd39a7 ; la synchronisation suivante avec `main` ne touche pas le dessin des voitures).

| Partie | Moment | Triangles des voitures : avant → 1re version → **après** | Écart après / avant | Triangles de l'image avant → après | Appels voitures | Image du banc (ms) avant → après |
|---|---|---|---|---|---|---|
| Course à 5, Ville de nuit | départ (5 voitures) | 262 100 → 218 839 → **192 065** | **−26,7 %** | 372 692 → 306 102 (−17,9 %) | 107 → 107 | 1 079,7 → 1 051,8 |
| Course à 5, Ville de nuit | 1 min | 162 341 → 132 761 → **132 761** | −18,2 % | 339 791 → 310 211 | 44 → 44 | 1 077,5 → 1 101,2 |
| Course à 5, Ville de nuit | 2 min | 162 341 → 123 921 → **123 921** | −23,7 % | 303 980 → 265 560 | 44 → 44 | 1 055,9 → 1 097,6 |
| Ville de nuit, trafic | départ | 155 428 → 138 575 → **138 575** | −10,8 % | 228 948 → 212 095 | 43 → 46 | 846,6 → 850,4 |
| Ville de nuit, trafic | 1 min / 2 min | 109 884 / 129 216 | 0 | identiques | identiques | 561,4 → 932,5 / 734,8 → 514,1 |

Ce que change la décision du PO (relevé voiture par voiture, RG-01, distance réelle à la caméra) : adversaire à **28,8 m (77 px)** en Course à 5 : niveau 1, **49 013 → 22 239 triangles (−55 %)**, 21 appels (inchangés : niveau 2 avec tous ses matériaux). Joueur : 7 à 11 m, 168 à 276 px, **complet, 109 884 triangles, 40 appels, inchangé** (refus du PO : pas de LOD1 pour le joueur). Adversaire à 16 m (104 px) : niveau 1, inchangé.

**Img/s** : le banc PC (lavapipe, rendu sur processeur, machine partagée à charge 20 sur 4 cœurs) donne ≈ 0,9 à 1,9 img/s avant comme après ; les écarts de temps d'image (−3 % à +4 % sur la Course à 5) sont du bruit de mesure et ne disent pas les img/s du téléphone. La suite de l'objectif ≥ 30 img/s passe à WFS-348 (coût par pixel).

## Passage d'un adversaire de 110 à 40 px, Ville de nuit, Basse (revue du lead dev)

Même voiture du trafic, posée devant à la distance qui donne la taille voulue (relevé du renderer : longueur réelle à l'écran) ; captures `lod-passage-*px-basse` (après seulement : la suite demande le relevé de la branche).

![passage](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/planche-lod-passage-110-40px.jpg?raw=true)

## Course à 5 de nuit, Basse : avant / après en zoom ×3 (en haut : main ; en bas : après)

lod-course5-nuit-arriere-basse : ![zoom](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/zoom-lod-course5-nuit-arriere-basse.jpg?raw=true)

lod-course5-nuit-proche-basse : ![zoom](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/zoom-lod-course5-nuit-proche-basse.jpg?raw=true)

lod-nuit-trafic-loin-60-basse : ![zoom](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/zoom-lod-nuit-trafic-loin-60-basse.jpg?raw=true)

## Captures pour le lead dev (42 après)

Dossier : [`qa/pr/WFS-335`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335) — banc `lod` (1280x720), mis en forme comme `tools/qa/captures_pr.sh` (étapes 1 à 3 faites à la main : le banc `nuit` du script durait plus d'une heure, machine chargée).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `lod-campagne-150-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-campagne-150-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-campagne-150-basse.jpg?raw=true) |
| `lod-campagne-300-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-campagne-300-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-campagne-300-basse.jpg?raw=true) |
| `lod-campagne-50-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-campagne-50-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-campagne-50-basse.jpg?raw=true) |
| `lod-circuit-150-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-circuit-150-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-circuit-150-basse.jpg?raw=true) |
| `lod-circuit-300-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-circuit-300-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-circuit-300-basse.jpg?raw=true) |
| `lod-circuit-50-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-circuit-50-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-circuit-50-basse.jpg?raw=true) |
| `lod-course5-nuit-arriere-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-course5-nuit-arriere-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-course5-nuit-arriere-basse.jpg?raw=true) |
| `lod-course5-nuit-proche-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-course5-nuit-proche-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-course5-nuit-proche-basse.jpg?raw=true) |
| `lod-course5-nuit-proche-loin-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-course5-nuit-proche-loin-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-course5-nuit-proche-loin-basse.jpg?raw=true) |
| `lod-fondu-ville-100-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-fondu-ville-100-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-fondu-ville-100-basse.jpg?raw=true) |
| `lod-fondu-ville-105-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-fondu-ville-105-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-fondu-ville-105-basse.jpg?raw=true) |
| `lod-fondu-ville-110-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-fondu-ville-110-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-fondu-ville-110-basse.jpg?raw=true) |
| `lod-fondu-ville-115-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-fondu-ville-115-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-fondu-ville-115-basse.jpg?raw=true) |
| `lod-fondu-ville-70-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-fondu-ville-70-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-fondu-ville-70-basse.jpg?raw=true) |
| `lod-fondu-ville-80-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-fondu-ville-80-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-fondu-ville-80-basse.jpg?raw=true) |
| `lod-fondu-ville-85-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-fondu-ville-85-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-fondu-ville-85-basse.jpg?raw=true) |
| `lod-fondu-ville-90-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-fondu-ville-90-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-fondu-ville-90-basse.jpg?raw=true) |
| `lod-nuit-trafic-10-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-nuit-trafic-10-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-nuit-trafic-10-basse.jpg?raw=true) |
| `lod-nuit-trafic-25-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-nuit-trafic-25-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-nuit-trafic-25-basse.jpg?raw=true) |
| `lod-nuit-trafic-50-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-nuit-trafic-50-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-nuit-trafic-50-basse.jpg?raw=true) |
| `lod-nuit-trafic-loin-100-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-nuit-trafic-loin-100-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-nuit-trafic-loin-100-basse.jpg?raw=true) |
| `lod-nuit-trafic-loin-130-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-nuit-trafic-loin-130-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-nuit-trafic-loin-130-basse.jpg?raw=true) |
| `lod-nuit-trafic-loin-170-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-nuit-trafic-loin-170-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-nuit-trafic-loin-170-basse.jpg?raw=true) |
| `lod-nuit-trafic-loin-60-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-nuit-trafic-loin-60-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-nuit-trafic-loin-60-basse.jpg?raw=true) |
| `lod-nuit-trafic-loin-80-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-nuit-trafic-loin-80-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-nuit-trafic-loin-80-basse.jpg?raw=true) |
| `lod-passage-100px-basse` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-passage-100px-basse.jpg?raw=true) |
| `lod-passage-110px-basse` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-passage-110px-basse.jpg?raw=true) |
| `lod-passage-40px-basse` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-passage-40px-basse.jpg?raw=true) |
| `lod-passage-45px-basse` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-passage-45px-basse.jpg?raw=true) |
| `lod-passage-48px-basse` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-passage-48px-basse.jpg?raw=true) |
| `lod-passage-50px-basse` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-passage-50px-basse.jpg?raw=true) |
| `lod-passage-60px-basse` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-passage-60px-basse.jpg?raw=true) |
| `lod-passage-70px-basse` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-passage-70px-basse.jpg?raw=true) |
| `lod-passage-80px-basse` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-passage-80px-basse.jpg?raw=true) |
| `lod-passage-90px-basse` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-passage-90px-basse.jpg?raw=true) |
| `lod-passage-95px-basse` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-passage-95px-basse.jpg?raw=true) |
| `lod-plage-150-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-plage-150-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-plage-150-basse.jpg?raw=true) |
| `lod-plage-300-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-plage-300-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-plage-300-basse.jpg?raw=true) |
| `lod-plage-50-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-plage-50-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-plage-50-basse.jpg?raw=true) |
| `lod-ville-150-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-ville-150-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-ville-150-basse.jpg?raw=true) |
| `lod-ville-300-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-ville-300-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-ville-300-basse.jpg?raw=true) |
| `lod-ville-50-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/avant-lod-ville-50-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-335/qa/pr/WFS-335/apres-lod-ville-50-basse.jpg?raw=true) |
