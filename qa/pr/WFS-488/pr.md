## Agent : Claude, agent dev3d (01Us6uCwUX)

**Ticket** : WFS-488 — règles : `regles-de-gestion/WFS-488.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : la couleur secondaire peint le toit et les coques de rétroviseurs (vrai « toit contrasté ») sur les 17 voitures, la carrosserie garde la couleur principale (épic E18, `regles-de-gestion/WFS-488.md`).
- **Ce qui change** :
  - **Données (chaîne de conversion)** : nouvel outil `tools/peinture2_voitures.mjs` + réglages `tools/peinture2.json` → un petit fichier `assets/cars/<id>.pt2` (0,5 à 10 Ko) à côté de chaque `.wfm` : un bit par triangle du pavillon ou d'une coque de rétroviseur. **Le `.wfm` n'est pas touché** (sommets, TAN1, `.lod`, empreintes de `formes.json` / `logos.json` inchangés). On choisit des **îlots entiers** du modèle (chaque panneau de carrosserie est un îlot séparé par un vrai joint ou une arête vive : RG-02) ; pour les 4 voitures au toit noir d'origine fait de triangles séparés, des triangles dans une boîte tournés vers le ciel. Empreinte FNV du maillage dans le `.pt2` : modèle changé sans relancer l'outil → fichier ignoré, voiture d'une seule couleur (jamais de mauvais toit).
  - **Moteur** : `src/render/vehicule_peinture2.{h,cpp}` lit le `.pt2` au chargement : normales des sommets du toit et des rétros mises à la longueur 4 (le shader normalise toujours la normale : éclairage inchangé), sommets du bord **dédoublés** (même position, même normale) → limite nette sur l'arête, sans dégradé ni trou. **Même maillage, même matériau, même tampon : aucun appel de dessin en plus** (RG-05 : 201 appels en Basse, 587 en Haute, comme `main`). Versions allégées (`.lod`, Basse) traitées aussi.
  - **Couleur** : la couleur 2 voyage dans le 4e nombre de la couleur des pièces peintes (`pc.color.w` = −(0xRRGGBB + 1), entier exact ; ≥ 0 = une seule couleur) : aucune constante de poussée en plus. `shaders/vehicule.glsl` (`couleurPeinture`) choisit la couleur au pixel ; **même vernis, même métal, même rugosité** que la carrosserie (RG-04).
  - **Toits noirs ou carbone d'origine** (Corsa, M4, Brezza, SF90 ; coques noires de la V60, de la Corsa, du Range Rover…) : sans couleur 2 ils gardent **exactement** leur aspect d'origine ; avec une couleur 2, leurs seuls pixels marqués deviennent de la peinture (couleur 2, vernis, finition de la carrosserie) ; le reste des garnitures noires ne change pas.
  - **Partie** : `Game` lit `perso::aspectCourse(...).peinture2` (couleur possédée seulement, WFS-487) et la pose sur la seule voiture du joueur (`Renderer::peinture2ProchaineVoiture`, remis à « une seule couleur » à chaque voiture) → **trafic, adversaires, police : couleur 2 = couleur 1** (RG-06). Le choix au garage arrive avec WFS-489 (hors périmètre).
  - **Accroches hors zone E10** (annoncées au journal) : `model_loader.cpp` (2 lignes), `renderer.h/.cpp` (5 lignes), `scene.frag` (5 lignes), `scene_skin.frag` (1), `game.h/.cpp` (6), `desktop_main.cpp` (2), `CMakeLists.txt` (2).
  - **Par voiture (RG-03)** : toit + rétros : Golf, M4 (toit carbone), 911, 208 (becquet compris), 500, Brezza, Compass (panneau peint entre les barres ; le cadre noir d'origine reste noir), Giulia (antenne comprise), Corsa (toit noir et becquet), GT86, V60, Corvette (panneau targa), SF90 (toit noir d'origine). **Rétroviseurs seuls** : Gallardo **Spyder** (cabriolet), **Model 3** (toit en verre), **S3** (toit ouvrant en verre noir), **Range Rover Sport** (toit panoramique en verre).
- **Comment c'est vérifié** :
  - nouveau test CPU `test_peinture2` : codes des 28 couleurs et du noir pur relus à l'identique (un défaut d'arrondi au-delà de 2²³ a été trouvé et corrigé), finition, maillage d'essai (bord dédoublé, aucune pièce ni triangle en plus, triangle mêlé du niveau allégé en couleur 1), `.pt2` tronqué / faux refusé, et les 17 voitures (empreintes = `.wfm`, toit et rétros repérés, aucun triangle à moitié marqué) ; **121 tests CPU OK** après synchronisation avec `main` ;
  - banc `WFS_BANC=peinture2` (nouveau) : 208, GT86, Corvette en toit Noir sur Rouge de 3/4 avant, de dessus, gros plans du rétroviseur et du joint avant du pavillon ; planche des 17 en toit **Blanc** (visible aussi sur les toits noirs d'origine) ; 208 en course de jour et de nuit ; gros plans Corsa, Compass, M4, Brezza regardés en local ;
  - **sans couleur 2 = aspect inchangé** (RG-04) : banc `vitrines-feux` (208, GT86, Corvette) branche contre `main` : 4 images **identiques au pixel**, 2 images `-feux` (après 2 s de conduite simulée) à **1 pixel de 1/255** près (bruit du calcul) ;
  - performances (banc `perf`, Ville, lavapipe) : Basse 358,4 ms (`main`) / 357,0 ms (branche), Haute 807,6 / 789,8 ms : même nombre d'appels de dessin et de triangles ;
  - **RC-E18-03, images par seconde en Basse** (retour du lead dev) : voir la section « Mesure en Basse » ci-dessous.
- **Limites et risques connus** :
  - rendu et images par seconde mesurés sur le banc PC (lavapipe), pas sur un vrai GPU mobile ;
  - le choix de la couleur 2 au garage n'existe pas encore (WFS-489) : aujourd'hui elle ne s'applique qu'à une couleur 2 achetée et posée dans la sauvegarde ;
  - Compass : seul le panneau peint du pavillon prend la couleur 2 (le cadre noir d'origine est mêlé à des pièces cachées du modèle) ; GT86 : en noir, le vernis montre un peu plus les ondulations d'origine du pavillon ;
  - un modèle reconverti ou retouché demande de relancer `node tools/peinture2_voitures.mjs` (sinon toit d'une seule couleur, test `test_peinture2` en échec) ;
  - RC-E18-05 (captures au garage) : la vitrine du banc tient lieu de garage jusqu'à WFS-489 ; RC-E18-11 (captures FR / EN) : ticket sans texte.

## Tests CPU

Journal complet : [`qa/pr/WFS-488/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/tests-cpu.log) — `bash tools/qa/tests_cpu.sh build`, branche `0f6f59d7` synchronisée avec `main` `d8166341` (depuis, `main` n'a changé que des `.md` et `tools/qa/verifier_licences.py`, relancé : « 160 fichiers d'assets vérifiés, 0 manque(s) ») :

```
tests CPU : 121 test(s) réussi(s) en 387 s
```

## Mesure en Basse (RC-E18-03 : écart ≤ 1 img/s, moyenne et 1 % les plus lents)

Banc `WFS_BANC=peinture2-mesure` (`src/render/vehicule_peinture2_mesure.cpp`, le même fichier compilé sur `main` pour la mesure « avant ») : Basse, Ville, 208 en course au pilote automatique, 180 images de chauffe, 600 images chronométrées ; couleur 2 posée sur la voiture du joueur par `WFS_PEINTURE2=noir`. Détail : [`qa/pr/WFS-488/mesure-basse.txt`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/mesure-basse.txt).

| Mesure (lavapipe) | Moyenne | 1 % les plus lents |
|---|---|---|
| `main` d8166341, série 1 / série 2 | 3,441 / 3,401 img/s | 2,181 / 2,314 img/s |
| branche, sans couleur 2, série 1 / 2 | 3,308 / 3,486 img/s | 2,313 / 2,480 img/s |
| **branche, couleur 2 noire posée**, série 1 / 2 | **3,317 / 3,326 img/s** | **2,369 / 2,321 img/s** |

Écart `main` → branche avec couleur 2 : −0,12 et −0,08 img/s en moyenne, +0,19 et +0,01 img/s pour le 1 % le plus lent : **critère tenu**, écarts du même ordre que le bruit entre deux mesures de `main`.

## Captures pour le lead dev (20 après)

Dossier : [`qa/pr/WFS-488`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `peinture2-course-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-peinture2-course-jour.jpg?raw=true) |
| `peinture2-course-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-peinture2-course-nuit.jpg?raw=true) |
| `peinture2-gt86-34` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-peinture2-gt86-34.jpg?raw=true) |
| `peinture2-gt86-dessus` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-peinture2-gt86-dessus.jpg?raw=true) |
| `peinture2-gt86-joint` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-peinture2-gt86-joint.jpg?raw=true) |
| `peinture2-gt86-retro` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-peinture2-gt86-retro.jpg?raw=true) |
| `peinture2-mustang-34` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-peinture2-mustang-34.jpg?raw=true) |
| `peinture2-mustang-dessus` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-peinture2-mustang-dessus.jpg?raw=true) |
| `peinture2-mustang-joint` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-peinture2-mustang-joint.jpg?raw=true) |
| `peinture2-p208-34` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-peinture2-p208-34.jpg?raw=true) |
| `peinture2-p208-dessus` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-peinture2-p208-dessus.jpg?raw=true) |
| `peinture2-p208-joint` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-peinture2-p208-joint.jpg?raw=true) |
| `peinture2-p208-retro` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-peinture2-p208-retro.jpg?raw=true) |
| `peinture2-planche` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-peinture2-planche.jpg?raw=true) |
| `vitrine-gt86-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/avant-vitrine-gt86-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-vitrine-gt86-0.jpg?raw=true) |
| `vitrine-gt86-feux` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/avant-vitrine-gt86-feux.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-vitrine-gt86-feux.jpg?raw=true) |
| `vitrine-mustang-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/avant-vitrine-mustang-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-vitrine-mustang-0.jpg?raw=true) |
| `vitrine-mustang-feux` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/avant-vitrine-mustang-feux.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-vitrine-mustang-feux.jpg?raw=true) |
| `vitrine-p208-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/avant-vitrine-p208-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-vitrine-p208-0.jpg?raw=true) |
| `vitrine-p208-feux` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/avant-vitrine-p208-feux.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-488/qa/pr/WFS-488/apres-vitrine-p208-feux.jpg?raw=true) |
