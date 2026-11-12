## Agent : Claude, agent dev3d (01UY1Vv5g8)

**Ticket** : WFS-453 — [TECHNIQUE] voiture du trafic posée sous 120 m non dessinée en Basse (ANO-247, régression de WFS-239) — règles : `regles-de-gestion/WFS-453.md`

## Description

- **Demande** (recette QA du 2026-11-12, ANO-247 bloquante) : en qualité Basse, toute voiture du trafic posée à moins de 120 m du joueur doit être dessinée ; au banc `qualite`, la voiture à 107 m ne l'était pas (« ERREUR placement (WFS-239 RG-03) »).
- **Cause (RG-01)** : commit **dcd173c9** (WFS-265, #268, 2026-11-08 18:46 +0100, après la fusion de WFS-239 à 04:07). `masquage::distanceMax` coupe en Basse les petits objets (rayon < 3 m) à **110 m de la caméra** (avant : 150 m). Le trafic choisit pourtant ses voitures jusqu'à **120 m du joueur** (`Traffic::dessines`). En vue Poursuite la caméra est 8 m derrière le joueur : une citadine (rayon ≈ 2,4 m) à 107 m du joueur est à ≈ 110,6 m de la caméra (rayon déduit) → coupée. Celle à 118 m restait dessinée parce que c'est un grand véhicule (rayon ≥ 3 m, autre règle). Preuve : la même branche avec la correction neutralisée redonne exactement l'erreur (1 « ERREUR placement »).
- **Le défaut existait aussi en jeu** : au banc `perf`, Ville Basse et Campagne Basse, le trafic choisissait 1 voiture mais en dessinait **0** (médiane : 0 appel, 0 triangle) ; après : 6 appels. Et la capture `voiture-distance-120-basse` de `main` ne montre **aucune voiture** à 120 m (le contrôle la comptait dessinée : voir « Limites »).
- **Ce qui change** :
  - `src/render/masquage.h/.cpp` : `distanceMaxTrafic()` et `TRAFIC_BASSE_M = 130 m` (120 m + 8 m de recul de la caméra + marge) : en Basse, une voiture **du trafic** n'est plus coupée avant 130 m de la caméra ; c'est toujours la sélection du trafic (120 m du joueur) qui borne. Haute, voitures garées, décor, personnes : `distanceMax` inchangée (la fonction de WFS-265 est gardée, RG-05).
  - `src/render/renderer.h/.cpp` : `traficProchainDessin()` (même principe que `adversaireProchaineVoiture`, WFS-243) : le prochain `draw` (voiture ou son ombre) prend la distance du trafic ; ombre en fondu sur les 15 derniers mètres comme avant.
  - Accroches annoncées au journal : `src/game/traffic_system.cpp` (6 appels avant les dessins des voitures et deux-roues du trafic) ; `src/render/qualite_banc.cpp` (capture seule `voiture-basse-poursuite-107-seule` ; **le contrôle n'est pas touché**, RG-03).
  - `tests/test_masquage.cpp` : cas 107 / 118 / 120 m en vue Poursuite, Basse ; le cas d'ANO-247 avec l'ancienne limite ; Haute et autres objets inchangés (RG-04).
- **Comment c'est vérifié** : banc `qualite` en local (lavapipe, même rendu que la CI) : **0 « ERREUR placement »** (avant : 1), 11 voitures dessinées une à une au lieu de 10 ; 114 tests CPU verts ; banc `perf` avant / après (`perf-avant-apres.txt`).
- **Performance** : Basse, +1 voiture quand l'une est entre 110 et 130 m de la caméra : +1 à +6 appels de dessin, +0,2 à +2,5 % de triangles. Temps d'image lavapipe dans le bruit (Haute, code identique et mêmes triangles, varie déjà de −1 à +10 % d'un passage à l'autre).
- **Limites et risques** :
  - Haute : la limite de 200 m de la caméra reste en deçà des 300 m du trafic (ANO-170, hors périmètre).
  - Le contrôle du banc compte une voiture « dessinée » dès que l'image a plus d'appels de dessin ; sur `main` il a compté la voiture à 120 m de la scène `voiture-distance-120-basse` alors qu'elle n'était pas à l'écran. Je ne touche pas au contrôle (RG-03) ; signalé au PO.
  - Les pilotes des deux-roues (personnes) restent effacés à 90 m en Basse (WFS-265), inchangé.

## Captures pour le lead dev

Dossier : [`qa/pr/WFS-453`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453). Avant = `main` (d8de393) ; pour la capture 1, qui n'existe pas sur `main`, avant = la branche avec la correction neutralisée (rendu de `main`).

| # | Capture | Avant | Après |
|---|---|---|---|
| 1 | Basse, Poursuite : voiture à **107 m** seule devant le joueur | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/avant-voiture-basse-poursuite-107-seule.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/apres-voiture-basse-poursuite-107-seule.jpg?raw=true) |
| 1 bis | Zoom ×4 du fond de la rue (même image) : absente avant, voiture sombre file de gauche après | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/avant-zoom4-voiture-107.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/apres-zoom4-voiture-107.jpg?raw=true) |
| 2 | Basse, voiture à **120 m** (`voiture-distance-120-basse`) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/avant-voiture-distance-120-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/apres-voiture-distance-120-basse.jpg?raw=true) |
| 2 bis | Zoom ×8 : absente avant, voiture rouge après | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/avant-zoom8-voiture-distance-120-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/apres-zoom8-voiture-distance-120-basse.jpg?raw=true) |
| 3 | Basse, Poursuite, trafic dense (`voiture-basse-poursuite`, 3 voitures dessinées ensemble : plafond voulu) : identique (écart ≤ 2 niveaux, bruit) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/avant-voiture-basse-poursuite.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/apres-voiture-basse-poursuite.jpg?raw=true) |
| 4 | **Haute**, trafic (`voiture-course-trafic`) : rien ne change (écart ≤ 3 niveaux, bruit) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/avant-voiture-course-trafic.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/apres-voiture-course-trafic.jpg?raw=true) |
| 5 | **Haute**, voiture à 120 m (`voiture-distance-120`) : identique au pixel près | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/avant-voiture-distance-120.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/apres-voiture-distance-120.jpg?raw=true) |
| 6 | Journal du banc `qualite` : 1 « ERREUR placement » avant, **0** après | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/avant-journal-qualite.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-453/qa/pr/WFS-453/apres-journal-qualite.jpg?raw=true) |

Pas de nuit : le sujet est la coupe à distance, la même de jour et de nuit.
Journal complet après : `qa/pr/WFS-453/qualite-journal-apres.txt` ; performance : `perf-avant-apres.txt`.

## Tests CPU

Commit testé : 72af8ae6 (branche synchronisée avec `main` b187fe57). Journal complet : `qa/pr/WFS-453/tests-cpu.log`.

```
[OK] test_ville_lieux (0 s)
[OK] test_ville_sol (4 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 114 test(s) réussi(s) en 436 s
code de sortie : 0
```
`test_masquage` : « trafic en Basse, vue Poursuite : 107 / 118 / 120 m gardées (WFS-453) : OK ».
