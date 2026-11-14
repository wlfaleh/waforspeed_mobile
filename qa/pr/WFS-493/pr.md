## Agent : Claude, agent dev3d (01Us6uCwTQ)

**Ticket** : WFS-493 — règles : `regles-de-gestion/WFS-493.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : 3 skins de plus, **Drift Tokyo**, **Néon nuit** et **Camouflage**, sur les 6 voitures du 1er lot (208, 500, GT86, Golf, Corvette C7, Giulia), avec le même système et les mêmes limites que la Rallye (WFS-492), sans logo, sponsor, écurie ni motif militaire réels (épic E18, RC-E18-01).
- **Ce qui change** :
  - `tools/skins/generer.py` : 3 nouveaux dessins, en mètres réels et dans le même dépliage que la Rallye :
    - **Drift Tokyo** (RG-02) : dégradé vermillon, plein à l'arrière et fondu vers l'avant le long d'une ligne penchée à 35° (1,10 m) ; trois entailles noires dans le dégradé, deux sur le capot ; trois **signes inventés** de 11 cm sur l'aile arrière, qui ne sont ni des lettres, ni des idéogrammes, ni des chiffres.
    - **Néon nuit** (RG-03) : filets cyan de 3,5 cm et magenta de 2,5 cm sur les flancs (le cyan monte en biais à 25° au-dessus de la roue arrière), sur les faces avant et arrière, et en deux paires sur le dessus.
    - **Camouflage** (RG-04) : habillage complet en blanc, gris et noir, avec des taches organiques allongées de 20 à 60 cm de diamètre. Elles sont posées en 3D sur l'enveloppe de la voiture, donc continues d'un panneau à l'autre.
  - **Taille des taches** : elle est mesurée à chaque génération (journal de l'outil). Les taches noires visibles du flanc mesurent 19 à 36 cm (quartiles), et 72 à 92 % de leur surface est en taches de 20 à 60 cm.
  - `tools/skins/voitures.json` : z des boucliers avant et arrière. Pour la Golf, la boîte relevée au banc va de −2,148 à 2,145 m.
  - **Textures** : 18 nouvelles dans `assets/cars/skins/` (EAC RG11, 1024 × 1024, 11 niveaux, **1,33 Mo** chacune, sous la limite de 1,5 Mo).
  - `shaders/vehicule.glsl` :
    - couleurs A et B des skins 2 à 4 (`SKIN_COULEURS`) ;
    - **lueur des filets du Néon la nuit seulement** (`F.misc.y`), à 0,5 × la couleur. Sa luminance (cyan 0,32, magenta 0,14) reste sous celle d'un feu arrière de position (≈ 0,36). La lumière n'est pas projetée et il n'y a pas de halo : seuls les pixels du filet brillent. De jour, les filets ont leur couleur normale.
  - `shaders/scene.frag` : **1 ligne d'accroche** qui ajoute cette lueur, nulle pour toute autre pièce ou skin.
  - `src/render/vehicule_skin_banc.cpp` : banc `WFS_BANC=skins`, nouvelle variable `WFS_SKINS`. Captures par skin : planche de chaque voiture, planches des 6 voitures de 3/4 avant et de 3/4 arrière, Néon de nuit (vitrine et course), Drift Tokyo et Camouflage en course de jour. Les captures de la Rallye gardent leur nom.
  - Rien d'autre ne change : ni moteur, ni sauvegarde, ni écran.
- **Comment c'est vérifié** :
  - **Tests CPU** : 140 réussis (`qa/pr/WFS-493/tests-cpu.log`). `test_vehicule_skin` lit les 18 nouvelles textures et vérifie :
    - poids ≤ 1,5 Mo, format, 11 niveaux ;
    - Drift : plein à l'arrière, peinture à l'avant ;
    - Néon : filets cyan et magenta, peinture entre eux ;
    - Camouflage : couleur A partout sur le flanc, et au moins 1/8 du flanc dans chaque ton (blanc, gris, noir).
  - **Banc lavapipe** (même rendu que la CI) : 55 captures après, toutes regardées. Les 3 skins sont propres sur les 6 voitures, des 4 côtés. Le Néon se lit la nuit sans tache blanche (vitrine et course). Les motifs sont continus aux jonctions des panneaux.
  - **Non-régression** : les 19 captures Rallye / sans skin d'avant et d'après sont identiques à l'œil (PSNR 37 à 57 dB ; les petits écarts viennent de la scène animée). Une seule texture d'habillage est en mémoire en course (journal du banc, RG-03 de WFS-492).
  - Contrôle des licences : 0 manque. Aucun téléchargement.
- **Limites et risques connus** :
  - **Taches grises du Camouflage** : les noires mordent dessus, donc la partie grise visible est plus petite que la tache dessinée (médiane visible 19 cm). Les taches dessinées font toutes 20 à 60 cm.
  - **Nez très incliné (Corvette)** : la projection de face s'y allonge un peu, comme pour la Rallye.
  - **Prix et règles d'obtention** (Néon nuit seulement en cadeau ou en succès) : ils sont déjà dans `configgame.json` ; l'écran viendra avec WFS-494, hors périmètre.
  - **Mémoire** : inchangée (une seule texture de 1,33 Mo en course). Le banc d'endurance n'a pas été relancé : l'écart de coût du shader se limite à un test d'entier et à une addition par pixel peint habillé.
  - **WFS-495** (autocollants, autre session) touche la même texture d'habillage : synchronisation avec `main` juste avant la fusion.

## Captures pour le lead dev (55 après)

Dossier : [`qa/pr/WFS-493`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `skins-abarth-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-abarth-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-abarth-jour.jpg?raw=true) |
| `skins-camouflage-6-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-camouflage-6-arriere.jpg?raw=true) |
| `skins-camouflage-6-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-camouflage-6-avant.jpg?raw=true) |
| `skins-camouflage-abarth-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-camouflage-abarth-jour.jpg?raw=true) |
| `skins-camouflage-giulia-course-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-camouflage-giulia-course-jour.jpg?raw=true) |
| `skins-camouflage-giulia-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-camouflage-giulia-jour.jpg?raw=true) |
| `skins-camouflage-golf-course-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-camouflage-golf-course-jour.jpg?raw=true) |
| `skins-camouflage-golf-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-camouflage-golf-jour.jpg?raw=true) |
| `skins-camouflage-gt86-course-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-camouflage-gt86-course-jour.jpg?raw=true) |
| `skins-camouflage-gt86-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-camouflage-gt86-jour.jpg?raw=true) |
| `skins-camouflage-mustang-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-camouflage-mustang-jour.jpg?raw=true) |
| `skins-camouflage-p208-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-camouflage-p208-jour.jpg?raw=true) |
| `skins-drifttokyo-6-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-drifttokyo-6-arriere.jpg?raw=true) |
| `skins-drifttokyo-6-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-drifttokyo-6-avant.jpg?raw=true) |
| `skins-drifttokyo-abarth-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-drifttokyo-abarth-jour.jpg?raw=true) |
| `skins-drifttokyo-giulia-course-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-drifttokyo-giulia-course-jour.jpg?raw=true) |
| `skins-drifttokyo-giulia-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-drifttokyo-giulia-jour.jpg?raw=true) |
| `skins-drifttokyo-golf-course-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-drifttokyo-golf-course-jour.jpg?raw=true) |
| `skins-drifttokyo-golf-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-drifttokyo-golf-jour.jpg?raw=true) |
| `skins-drifttokyo-gt86-course-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-drifttokyo-gt86-course-jour.jpg?raw=true) |
| `skins-drifttokyo-gt86-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-drifttokyo-gt86-jour.jpg?raw=true) |
| `skins-drifttokyo-mustang-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-drifttokyo-mustang-jour.jpg?raw=true) |
| `skins-drifttokyo-p208-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-drifttokyo-p208-jour.jpg?raw=true) |
| `skins-giulia-course-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-giulia-course-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-giulia-course-jour.jpg?raw=true) |
| `skins-giulia-course-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-giulia-course-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-giulia-course-nuit.jpg?raw=true) |
| `skins-giulia-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-giulia-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-giulia-jour.jpg?raw=true) |
| `skins-giulia-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-giulia-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-giulia-nuit.jpg?raw=true) |
| `skins-golf-course-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-golf-course-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-golf-course-jour.jpg?raw=true) |
| `skins-golf-course-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-golf-course-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-golf-course-nuit.jpg?raw=true) |
| `skins-golf-droit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-golf-droit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-golf-droit.jpg?raw=true) |
| `skins-golf-jonction` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-golf-jonction.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-golf-jonction.jpg?raw=true) |
| `skins-golf-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-golf-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-golf-jour.jpg?raw=true) |
| `skins-golf-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-golf-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-golf-nuit.jpg?raw=true) |
| `skins-golf-sans` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-golf-sans.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-golf-sans.jpg?raw=true) |
| `skins-gt86-course-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-gt86-course-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-gt86-course-jour.jpg?raw=true) |
| `skins-gt86-course-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-gt86-course-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-gt86-course-nuit.jpg?raw=true) |
| `skins-gt86-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-gt86-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-gt86-jour.jpg?raw=true) |
| `skins-gt86-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-gt86-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-gt86-nuit.jpg?raw=true) |
| `skins-mustang-droit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-mustang-droit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-mustang-droit.jpg?raw=true) |
| `skins-mustang-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-mustang-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-mustang-jour.jpg?raw=true) |
| `skins-neonnuit-6-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-neonnuit-6-arriere.jpg?raw=true) |
| `skins-neonnuit-6-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-neonnuit-6-avant.jpg?raw=true) |
| `skins-neonnuit-abarth-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-neonnuit-abarth-jour.jpg?raw=true) |
| `skins-neonnuit-giulia-course-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-neonnuit-giulia-course-nuit.jpg?raw=true) |
| `skins-neonnuit-giulia-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-neonnuit-giulia-jour.jpg?raw=true) |
| `skins-neonnuit-giulia-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-neonnuit-giulia-nuit.jpg?raw=true) |
| `skins-neonnuit-golf-course-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-neonnuit-golf-course-nuit.jpg?raw=true) |
| `skins-neonnuit-golf-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-neonnuit-golf-jour.jpg?raw=true) |
| `skins-neonnuit-golf-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-neonnuit-golf-nuit.jpg?raw=true) |
| `skins-neonnuit-gt86-course-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-neonnuit-gt86-course-nuit.jpg?raw=true) |
| `skins-neonnuit-gt86-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-neonnuit-gt86-jour.jpg?raw=true) |
| `skins-neonnuit-gt86-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-neonnuit-gt86-nuit.jpg?raw=true) |
| `skins-neonnuit-mustang-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-neonnuit-mustang-jour.jpg?raw=true) |
| `skins-neonnuit-p208-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-neonnuit-p208-jour.jpg?raw=true) |
| `skins-p208-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/avant-skins-p208-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-493/qa/pr/WFS-493/apres-skins-p208-jour.jpg?raw=true) |
