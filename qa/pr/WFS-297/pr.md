## Agent : Claude, agent 1 (01UY1Vv2)

**Ticket** : WFS-297 — règles : `regles-de-gestion/WFS-297.md`

## Description (à remplir par le développeur avant d'ouvrir la PR)

- **Demande de l'utilisateur** (en une phrase) : « le jeu rame de plus en plus, la 2e partie est une catastrophe » : mesurer 3 parties de suite, nommer ce qui grossit, et afficher les chiffres de mémoire au chargement et dans les Réglages (RG-06).
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - **Banc** `WFS_BANC=perf WFS_PERF_PARTIES=1` (`src/render/bench_perf.cpp`) : parties enchaînées sans quitter le jeu, par le même chemin que « Rouler » (`Game::demandes`), en qualité Basse. Mesures au départ, à 1 min et à 2 min : image, GPU par passe, appels et triangles par système, trafic, lampadaires, RSS. Captures du chargement et des Réglages à chaque partie.
  - **Compteurs mesurés** (`src/core/memoire.h`), écrits là où les objets se créent :
    - mémoire de la carte graphique : `vk.cpp`, `createBuffer` / `createImage` / `destroy` (champ `alloue`, mémoire paresseuse exclue) ;
    - octets lus : `readAsset`, PC et Android ;
    - objets 3D vivants : `Renderer::makeMesh` / `destroy` ;
    - personnes : `pietons.cpp`, accroche de 2 lignes ;
    - numéro de la partie : `Game::init` ;
    - mémoire du jeu : `/proc/self/statm`, taille de page du système.
  - **Écran de chargement** (`chargement.cpp`) : deux petites lignes sous l'étape, « Lus : … Mo · … objets 3D · … voitures · … personnes » et « Mémoire : jeu … Mo · carte graphique … Mo · partie N ».
  - **Réglages** (`reglages.cpp`, les deux mises en page) : la ligne « Rendu » se poursuit par « Mémoire : jeu … · carte graphique … · partie N », réduite pour tenir avant « FERMER ». Aucun bouton ne bouge. Ajout de `ouvrirReglagesBanc` (banc seulement).
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) :
  - 65 tests CPU verts ; parcours complet 0 écart ;
  - **diagnostic RG-01 / RG-02** (banc PC lavapipe, Basse, au départ) :

    | Partie | Appels | Triangles | GPU (ms) | RSS (Mo) |
    |---|---|---|---|---|
    | 1. Corniche jour | 230 | 363 k | 215 | 1 018 |
    | 2. Ville nuit | 339 | 523 k | 336 | 1 385 |
    | 3. Ville jour | 291 | 526 k | 289 | 1 411 |
    | 4. Ville nuit | 339 | 523 k | 334 | 1 402 |

  - contre-épreuve : la Ville de nuit jouée en 1re partie donne 339 appels, 525 k triangles et GPU 333 ms. **Rien ne s'accumule** d'une partie à l'autre : la 2e et la 4e partie sont identiques.
  - Cause : la 2e partie de l'utilisateur était la Ville de nuit, la scène la plus lourde (×1,5 en dessin). Les piétons y font 131 appels et 258 k triangles au départ, et la nuit ajoute jusqu'à 8 lumières par pixel. « Chauffe 0 » sur ses deux captures.
- **Limites et risques connus** : l'allègement de la Ville de nuit (RG-05 : même fluidité en 2e partie) passe au ticket **WFS-299**, sur décision du PO. Les chiffres de mémoire sont ceux du jeu : la mémoire réservée par le pilote graphique lui-même n'est pas comptée.

## Captures pour le lead dev (7 après)

Dossier : [`qa/pr/WFS-297`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `chargement-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/avant-chargement-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-chargement-0.jpg?raw=true) |
| `chargement-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/avant-chargement-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-chargement-1.jpg?raw=true) |
| `chargement-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/avant-chargement-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-chargement-2.jpg?raw=true) |
| `chargement-erreur` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/avant-chargement-erreur.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-chargement-erreur.jpg?raw=true) |
| `chargement-fondu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/avant-chargement-fondu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-chargement-fondu.jpg?raw=true) |
| `reglages-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/avant-reglages-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-reglages-menu.jpg?raw=true) |
| `reglages-pause` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/avant-reglages-pause.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-reglages-pause.jpg?raw=true) |

## Banc des parties enchaînées (après seulement : mode nouveau)

| Partie | Chargement | Réglages (pause) | Scène |
|---|---|---|---|
| 1 | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-1-chargement.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-1-reglages.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-1-plage-jour.jpg?raw=true) |
| 2 | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-2-chargement.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-2-reglages.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-2-ville-nuit.jpg?raw=true) |
| 3 | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-3-chargement.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-3-reglages.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-3-ville-jour.jpg?raw=true) |
