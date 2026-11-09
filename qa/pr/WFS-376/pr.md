## Agent : Claude, agent dev3d (01UY1Vv3u9)

**Ticket** : WFS-376 — règles : `regles-de-gestion/WFS-376.md`

## Description

- **Demande de l'utilisateur** : sur la vidéo du 2026-11-09 (Corniche, vue Cockpit), « pour le réalisme c'est quoi ces bateaux » : les « bateaux » de la marina étaient de grands triangles blancs plats derrière le mur du quai (ANO-206). Il fallait une vraie marina de la Côte d'Azur.
- **Ce qui change** :
  - `src/world/marina_bateaux.{h,cpp}` (nouveau, processeur seul, testé) : bateaux **modélisés**, surfaces lissées (aucune boîte, aucun triangle de voile) : voilier de croisière au port (9,6 m, bau 3,4 m, coque à tonture et étrave inclinée, rouf vitré, cockpit à hiloires et fond en teck, filières, mât ovale de 13,25 m, barres de flèche, haubans, pataras, bôme à 2,0 m du pont, **sac de grand-voile fermé**, **génois enroulé** sur l'étai) ; yacht de 18 m à flybridge (plateforme de bain, salon vitré à pare-brise incliné, hublots, arceau radar, main courante) ; vedette (même modèle sans flybridge, échelle 0,6). Trois versions par modèle : détaillée, moyenne, allégée. Amarres croisées au quai, pendille, passerelle de poupe. Pontons flottants (platelage chanfreiné, lisses de bois, flotteurs, bornes eau / électricité, pieu de guidage, passerelle articulée depuis le quai). Bitte d'amarrage en fonte (remplace les cylindres noirs).
  - `src/world/plage.cpp` (zone « marina » seulement) : quai d'amarrage abaissé à **1,45 m au-dessus de l'eau** (1,85 m avant, RG-01) avec deux marches de 0,20 m depuis la promenade et un bord chanfreiné ; placement de la marina selon la décision du PO : rangée cul au quai (poupe à 1-2 m, ±2,3°, places de bau + 0,6 à 1,0 m), occupée à ≈ 87 % (jamais deux places vides de suite), 8 pontons tous les 33-35 m avec bateaux des deux côtés au-delà d'un chenal de 29 m, yachts en bout de quai ; les 37 bateaux mis au mouillage au pied de la falaise sont de vrais bateaux voiles rangées. Deux couleurs ajoutées à la palette (gelcoat blanc cassé 0xe0ded6, bleu marine 0x283a5c). Barques du village et bateaux du large inchangés (RG-11). Banc d'essai `marina` (même fichier).
  - Accroche annoncée : `src/platform/desktop_main.cpp` (2 lignes : groupe de banc `marina`) ; `CMakeLists.txt` (1 ligne : `test_marina_bateaux`).
- **Comment c'est vérifié** :
  - `test_marina_bateaux` (nouveau) : voilier 9,60 m / bau 3,40 m (±1 %), mât 13,25 m = 1,38 × L, bôme 3,28 m (pont + 2,0 m), **aucune voile hissée** (1,8 m² au-dessus de 4 m : mât, génois enroulé, câbles) ; yacht 18,00 × 4,90 m ; budgets ; aucun triangle dégénéré ni inversé, normales unitaires ; versions moyennes aux mêmes dimensions. `bash tools/qa/tests_cpu.sh build` : voir le journal.
  - Banc `marina` (rendu de la CI, lavapipe), « avant » = main 4be37594 + le seul banc ajouté (mêmes caméras) :

    | Mesure | Avant | Après |
    |---|---|---|
    | Bateaux du port | ≈ 56 boîtes + triangle de voile | 125 à quai (55 % de voiliers, 41 % de vedettes, 4 % de yachts), 12 places vides, 37 au mouillage |
    | Coques (RG-07) | — | 82 % blanc cassé, 10 % marine, 8 % gris / crème |
    | Triangles par bateau, détaillé / moyen / allégé | ≈ 45 | voilier 284 / 118 / 15, vedette 172 / 70 / 21, yacht 207 / 95 / 21 (coque allégée : 11) |
    | Fenêtre détaillée la plus chargée (315 m, passe de la scène) | ≈ 2 600 | **17 812** (budget du PO : ≤ 18 000) |
    | Appels de dessin, Cockpit / Poursuite 120 / Basse | 361 / 298 / 199 | **361 / 298 / 199 (inchangés)** |
    | Image entière, Cockpit 80 km/h | 305 413 | 370 237 (+21 %) |
    | Image entière, Poursuite 120 km/h | 630 756 | 695 972 (+10,3 %) |
    | Image entière, qualité Basse | 293 303 | 325 911 (+11,1 %) |
- **Limites et risques connus** :
  - Qualité Basse : +11,1 % de triangles pour l'image entière (le PO demandait sous +10 % si possible). Ombres comprises ; aucun appel de dessin en plus. À mesurer sur le téléphone de l'utilisateur.
  - RG-09 : le lampadaire de la promenade mesure ≈ 6,4 m (et non 7 à 8 m) ; mât / lampadaire = 2,07 pour le voilier de 9,6 m (1,86 à 2,38 selon la taille). Ramené à un lampadaire de 7,5 m : 1,77 (dans 1,6-2,0). Le lampadaire n'est pas dans la zone de ce ticket.
  - Seuil de détail unique à 200 m par bloc de 115 m (WFS-268), décision du PO : pas de seuils 150 m / 60 m. La suite `marina-suite-1` à `-4` passe ce seuil sans saut de silhouette (voir aussi `marina-loin` / `marina-loin-detail`, même cadrage, allégé / détaillé).
  - Le ciel sombre de l'heure dorée et du midi au banc vient de E13 (WFS-214, ANO-208), pas de ce ticket.
  - Les deux erreurs « éclairage (WFS-213) : capture abandonnée » du banc existent aussi sur main (vues libres), elles n'empêchent pas les captures.

## Captures pour le lead dev (18 après)

Dossier : [`qa/pr/WFS-376`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `marina-bassin` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-bassin.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-bassin.jpg?raw=true) |
| `marina-cockpit-120-doree` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-cockpit-120-doree.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-cockpit-120-doree.jpg?raw=true) |
| `marina-cockpit-120-midi` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-cockpit-120-midi.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-cockpit-120-midi.jpg?raw=true) |
| `marina-cockpit-80-doree` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-cockpit-80-doree.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-cockpit-80-doree.jpg?raw=true) |
| `marina-cockpit-80-midi` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-cockpit-80-midi.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-cockpit-80-midi.jpg?raw=true) |
| `marina-gros-plan-doree` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-gros-plan-doree.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-gros-plan-doree.jpg?raw=true) |
| `marina-gros-plan-midi` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-gros-plan-midi.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-gros-plan-midi.jpg?raw=true) |
| `marina-loin-detail` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-loin-detail.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-loin-detail.jpg?raw=true) |
| `marina-loin` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-loin.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-loin.jpg?raw=true) |
| `marina-poursuite-120-doree` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-poursuite-120-doree.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-poursuite-120-doree.jpg?raw=true) |
| `marina-poursuite-120-midi-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-poursuite-120-midi-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-poursuite-120-midi-basse.jpg?raw=true) |
| `marina-poursuite-120-midi` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-poursuite-120-midi.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-poursuite-120-midi.jpg?raw=true) |
| `marina-poursuite-80-doree` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-poursuite-80-doree.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-poursuite-80-doree.jpg?raw=true) |
| `marina-poursuite-80-midi` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-poursuite-80-midi.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-poursuite-80-midi.jpg?raw=true) |
| `marina-suite-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-suite-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-suite-1.jpg?raw=true) |
| `marina-suite-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-suite-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-suite-2.jpg?raw=true) |
| `marina-suite-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-suite-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-suite-3.jpg?raw=true) |
| `marina-suite-4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/avant-marina-suite-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-376/qa/pr/WFS-376/apres-marina-suite-4.jpg?raw=true) |
