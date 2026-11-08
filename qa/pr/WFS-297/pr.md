## Agent : Claude, agent 1 (01UY1Vv2)

**Ticket** : WFS-297 — règles : `regles-de-gestion/WFS-297.md`

## Description (à remplir par le développeur avant d'ouvrir la PR)

- **Demande de l'utilisateur** (en une phrase) : « le jeu rame de plus en plus, la 2e partie est une catastrophe » : mesurer 3 parties de suite, nommer ce qui grossit, afficher les chiffres de mémoire au chargement et dans les Réglages (RG-06).
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - **Banc** `WFS_BANC=perf WFS_PERF_PARTIES=…` (`src/render/bench_perf.cpp`) : parties enchaînées sans quitter le jeu, par le chemin de « Rouler » (`Game::demandes`), qualité Basse, règles de la course (décompte et « GO »).
    - Mesures au départ, à 1 min et à 2 min : GPU par passe, appels et triangles par système, trafic, lampadaires, RSS, mémoire de la carte graphique.
    - Captures du chargement, des Réglages et de la scène à chaque partie, plus la Ville de nuit à 0, 30 et 60 s.
  - **Compteurs mesurés** (`src/core/memoire.h`) :
    - mémoire de la carte graphique : `vk.cpp`, octets réellement alloués, mémoire paresseuse exclue ;
    - Mo lus pendant le chargement de la partie : `readAsset` et `Game::init` ;
    - objets 3D vivants : `Renderer` ;
    - personnes : `pietons.cpp` ;
    - **numéro de la partie** : chaque départ de course (« GO ») dans `course_system.cpp`, qu'il vienne de « Rouler », de « Rejouer », d'un rechargement de carte ou d'une manche de duel ;
    - mémoire du jeu : `/proc/self/statm`, taille de page du système.
  - **Textes** : construits avec `ui::nombre` dans `chargement.cpp` (`texteMemoire`), utilisés aussi par `reglages.cpp`.
    - Écran de chargement : « Lus pour cette partie : … Mo · … objets 3D · … voitures · … personnes », puis « Mémoire : jeu … Mo · carte graphique … Mo · partie N », N étant le numéro de la partie qui se charge.
    - Réglages, deux mises en page : la ligne « Rendu » se poursuit par la mémoire et « partie N », réduite si elle ne tient pas. Aucun bouton ne bouge.
  - **Test CPU** : `tests/test_memoire.cpp`.
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) :
  - 66 tests CPU verts, dont `test_memoire` ;
  - parcours complet 0 écart, dont le parcours réel : « course : départ, partie 1 / 2 / 3 / 4 depuis le lancement » ;
  - **tableau RG-01 / RG-02** (banc PC lavapipe, 2340×1080, Basse, règles de la course) :

    | Partie | Moment | GPU (ms) | Appels | Triangles | Trafic | RSS (Mo) | Carte graphique (Mo) |
    |---|---|---|---|---|---|---|---|
    | 1. plage jour | départ | 362.01 | 248 | 405 k | 36 | 1094 | 478 |
    | 1. plage jour | 1 min | 386.48 | 158 | 373 k | 42 | 1092 | 478 |
    | 1. plage jour | 2 min | 274.14 | 83 | 245 k | 42 | 1120 | 478 |
    | 2. ville nuit | départ | 686.28 | 423 | 773 k | 47 | 1462 | 665 |
    | 2. ville nuit | 1 min | 370.67 | 227 | 304 k | 65 | 1458 | 657 |
    | 2. ville nuit | 2 min | 411.96 | 186 | 263 k | 89 | 1458 | 657 |
    | 3. ville jour | départ | 594.98 | 376 | 777 k | 47 | 1477 | 664 |
    | 3. ville jour | 1 min | 318.18 | 172 | 305 k | 65 | 1472 | 657 |
    | 3. ville jour | 2 min | 318.27 | 143 | 262 k | 89 | 1472 | 657 |

  - **Comparaisons au même moment** : Ville jour en 3e partie contre Ville nuit en 2e, mémoire stable.
  - **Contre-épreuve** (1re version du banc) : la Ville de nuit jouée en 1re partie donne les mêmes chiffres qu'en 2e et 4e partie.
  - **Rien ne s'accumule** : mêmes appels, triangles et mémoire de la carte graphique (≈ 660 Mo) d'une partie de Ville à l'autre.
  - **Cause** : la Ville de nuit est la scène la plus lourde. Les piétons y font environ 258 k triangles au départ, et la nuit ajoute jusqu'à 8 lumières par pixel. « Chauffe 0 » sur les captures de l'utilisateur.
- **Limites et risques connus** :
  - RG-05 (même fluidité en 2e partie) passe au ticket **WFS-299**, sur décision du PO (allègement de la Ville de nuit).
  - Sur les captures, la ligne « Images (20 s) » affiche les **valeurs d'essai** du banc (`WFS_MESURE_ESSAI`, WFS-246). Le banc ne mesure pas les vraies images par seconde ; ces valeurs servent seulement à vérifier que les deux lignes du bas tiennent sans se chevaucher.
  - La mémoire réservée par le pilote graphique lui-même n'est pas comptée.

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

## Banc des parties enchaînées, 2340×1080 (après seulement : mode nouveau)

| Partie | Chargement | Réglages (pause) | Scène au départ |
|---|---|---|---|
| 1 | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-1-chargement.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-1-reglages.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-1-plage-jour.jpg?raw=true) |
| 2 | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-2-chargement.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-2-reglages.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-2-ville-nuit.jpg?raw=true) |
| 3 | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-3-chargement.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-3-reglages.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-3-ville-jour.jpg?raw=true) |

### Ville de nuit, 2e partie, vue Proche, à 30 s d'écart

| 0 s | 30 s | 60 s |
|---|---|---|
| ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-2-ville-nuit-0s.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-2-ville-nuit-30s.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-297/qa/pr/WFS-297/apres-parties-2-ville-nuit-60s.jpg?raw=true) |
