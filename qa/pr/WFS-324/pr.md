## Agent : Claude, agent dev3d (01UY1Vv5YT)

**Ticket** : WFS-324 — règles : `regles-de-gestion/WFS-324.md`

## Description (à remplir par le développeur avant d'ouvrir la PR)

- **Demande de l'utilisateur** (en une phrase) : en vue Cockpit, des rétroviseurs visibles (intérieur, gauche, droit) qui montrent vraiment la route et les voitures derrière (WFS-324, ANO-207).
- **Ce qui change** :
  - **Vrai reflet, miroir plan exact** (`src/render/retroviseurs.{h,cpp}`, `retroviseurs_geo.cpp`) : la glace sert de fenêtre à une projection décentrée vue depuis l'œil, sur le monde réfléchi par le plan de la glace. Passe enfichable « rétroviseurs » (étape Écran), une glace par image (une image sur deux en Basse). Images de 352 × 88 et 192 × 112 texels. Seuls les objets vus par la glace sont redessinés (bit `VIS_RETRO`). Sans ombres. Personnes jusqu'à 25 m. Rien hors de la vue Cockpit.
  - **Reflet vu depuis la place réelle du modèle 3D** : l'œil du cockpit est placé 50 à 75 cm trop en avant dans le modèle de la voiture.
    - Glaces extérieures : le rétroviseur du modèle est mesuré au chargement (18 voitures sur 20).
    - Glace intérieure : même décalage en long (60 cm si rien n'est mesuré).
    - Sans ce décalage, les sièges avant du modèle cachaient la voiture qui suit (1er refus).
  - **Réglage du conducteur** :
    - glace intérieure : vise le milieu d'une voiture 12 m derrière, selon la hauteur de l'œil (0,7° vers le bas pour la M4, 2,9° pour le Range Rover Sport) ;
    - glaces extérieures : la carrosserie occupe 1,5° au bord intérieur, le reste montre la voie d'à côté.
  - **Voiture du joueur** : dessinée seulement pour les glaces, en version allégée, sans ses vitres, habitacle assombri dans la glace intérieure. Une voiture vue seulement dans les glaces n'éclaire rien, comme avant (sinon, ses phares éclairaient en blanc le bas de l'habitacle).
  - **Nuit** (`nuit.cpp`, accroche) : en vue Cockpit avec rétroviseurs, les 2 lampadaires les plus proches derrière la voiture (5 à 60 m) sont aussi de vraies lumières. Avant, rien n'était éclairé derrière la voiture et les glaces restaient noires.
  - **Pièces modelées, aucune forme nue** : coque extérieure galbée (surface balayée, lèvre, logement, nez), bras, aile, glaces à coins arrondis. La coque intérieure de WFS-323 (ANO-207) est en plastique satiné.
  - **Shader** : une glace à reflet affiche l'image × la réflexion (intérieur 80 %, extérieurs 50 %, ONU R46).
  - **Accroches annoncées au journal** :
    - `renderer.{h,cpp}` : `VIS_RETRO`, `retroSeul` / `horsRetro`, voiture du joueur allégée, phares des voitures vues seulement dans les glaces ;
    - `game.cpp` (7 lignes), `model_loader.cpp` (1 ligne), `nuit.cpp` (lampadaires derrière), `camera_vue.cpp`, `passes.h`, `desktop_main.cpp` ;
    - `vehicule_cockpit_banc.cpp` (glaces figées pendant le banc WFS-349).
- **Comment c'est vérifié** :
  - **Tests** : 102 tests verts sur 103, dont `test_retroviseurs` (21 cas). Le seul rouge, `test_course5_ordinateurs`, est aussi rouge sur `main` (constaté par le lead dev) : `tests_cpu.sh` s'arrête sur lui, donc les autres tests ont été lancés un par un.
  - **Banc `retroviseurs`** (Golf, 2400 × 1080, jour et nuit) : 0 écart. Pixels de la voiture dans chaque glace (image avec et sans la voiture) :

    | Scène | Glace intérieure | Glace gauche | Glace droite |
    |---|---|---|---|
    | Même voie, jour | 32 874 | 4 175 | 6 022 |
    | Même voie, nuit | 13 713 | 2 416 | 3 798 |
    | Voie de gauche, jour | 0 (hors champ, comme en vrai) | 1 393 | — |
    | Voie de gauche, nuit | — | 789 | — |

  - **Banc `retroviseurs`** (Range Rover Sport, M4, 911, jour et nuit) : 0 écart.
  - **Coût mesuré** (banc PC lavapipe, avec et sans rétroviseurs en alternance, médianes) : Basse 2,0 %, soit **0,41 img/s perdue à 21 img/s** ; Haute 3,2 %, soit **0,64 img/s** (estimé, 2 au plus visé).
  - **Banc `cockpit-voisine`** (WFS-349) : 0 écart.
  - **Banc `cockpit`** (Golf) : comparé avant / après.
  - **Capture 3** : Réglages ouverts en pause, ligne « Rendu » sans puis avec rétroviseurs (155 → 198 appels en Basse).
- **Limites et risques connus** :
  - Coût mesuré sur le banc PC, pas sur le téléphone : ticket 🟡 À valider par l'utilisateur sur l'APK.
  - **Glaces extérieures** :
    - redessinées à tour de rôle, avec jusqu'à 3 images de retard (7 en Basse) ;
    - glace droite au bord de l'écran, en partie derrière le montant ;
    - 911, petite glace de 175 mm : une voiture 9 m derrière dans la voie de gauche n'apparaît qu'au bord.
  - **Nuit, banc cockpit** (voir la réponse au point 4) : la glace gauche reste sombre. La scène n'a que 12 vraies lumières, choisies pour la vue avant : la terrasse éclairée à côté de la voiture l'est par des lumières qui ne sont pas reprises derrière elle, et la glace ne renvoie que 50 % de la lumière.
  - Les phares de la voiture qui suit sont dessinés comme en vue directe (optiques allumées de WFS-413), mais n'éclairent pas la route vers nous (comportement d'avant).
  - Places et tailles des glaces extérieures : estimées. M4 et Corvette : décalage moyen, faute de mesure.

## Captures pour le lead dev (26 après)

Dossier : [`qa/pr/WFS-324`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `cockpit-golf-braquage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/avant-cockpit-golf-braquage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-cockpit-golf-braquage.jpg?raw=true) |
| `cockpit-golf-carte` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/avant-cockpit-golf-carte.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-cockpit-golf-carte.jpg?raw=true) |
| `cockpit-golf-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/avant-cockpit-golf-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-cockpit-golf-jour.jpg?raw=true) |
| `cockpit-golf-nuit-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/avant-cockpit-golf-nuit-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-cockpit-golf-nuit-virage.jpg?raw=true) |
| `cockpit-golf-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/avant-cockpit-golf-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-cockpit-golf-nuit.jpg?raw=true) |
| `cockpit-golf-plage-nuit-depart` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/avant-cockpit-golf-plage-nuit-depart.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-cockpit-golf-plage-nuit-depart.jpg?raw=true) |
| `cockpit-golf-pluie` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/avant-cockpit-golf-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-cockpit-golf-pluie.jpg?raw=true) |
| `cockpit-golf-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/avant-cockpit-golf-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-cockpit-golf-virage.jpg?raw=true) |
| `retro-golf-derriere-nuit-zoom-droite` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-derriere-nuit-zoom-droite.jpg?raw=true) |
| `retro-golf-derriere-nuit-zoom-gauche` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-derriere-nuit-zoom-gauche.jpg?raw=true) |
| `retro-golf-derriere-nuit-zoom-interieur` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-derriere-nuit-zoom-interieur.jpg?raw=true) |
| `retro-golf-derriere-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-derriere-nuit.jpg?raw=true) |
| `retro-golf-derriere-zoom-droite` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-derriere-zoom-droite.jpg?raw=true) |
| `retro-golf-derriere-zoom-gauche` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-derriere-zoom-gauche.jpg?raw=true) |
| `retro-golf-derriere-zoom-interieur` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-derriere-zoom-interieur.jpg?raw=true) |
| `retro-golf-derriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-derriere.jpg?raw=true) |
| `retro-golf-gauche-nuit-zoom-droite` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-gauche-nuit-zoom-droite.jpg?raw=true) |
| `retro-golf-gauche-nuit-zoom-gauche` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-gauche-nuit-zoom-gauche.jpg?raw=true) |
| `retro-golf-gauche-nuit-zoom-interieur` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-gauche-nuit-zoom-interieur.jpg?raw=true) |
| `retro-golf-gauche-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-gauche-nuit.jpg?raw=true) |
| `retro-golf-gauche-zoom-droite` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-gauche-zoom-droite.jpg?raw=true) |
| `retro-golf-gauche-zoom-gauche` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-gauche-zoom-gauche.jpg?raw=true) |
| `retro-golf-gauche-zoom-interieur` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-gauche-zoom-interieur.jpg?raw=true) |
| `retro-golf-gauche` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-golf-gauche.jpg?raw=true) |
| `retro-reglages-rendu-basse-avec` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-reglages-rendu-basse-avec.jpg?raw=true) |
| `retro-reglages-rendu-basse-sans` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-reglages-rendu-basse-sans.jpg?raw=true) |
