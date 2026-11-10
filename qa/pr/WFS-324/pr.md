## Agent : Claude, agent dev3d (01UY1Vv5YT)

**Ticket** : WFS-324 — règles : `regles-de-gestion/WFS-324.md`

## Description (à remplir par le développeur avant d'ouvrir la PR)

- **Demande de l'utilisateur** (en une phrase) : en vue Cockpit, des rétroviseurs visibles (intérieur, gauche, droit) qui montrent vraiment la route et les voitures derrière (WFS-324, ANO-207).
- **Ce qui change** :
  - **Vrai reflet, miroir plan exact** (`src/render/retroviseurs.{h,cpp}`, `retroviseurs_geo.cpp`) : la glace est la fenêtre d'une projection décentrée vue depuis l'œil, sur le monde réfléchi par le plan de la glace. Chaque point de la glace montre le bon texel, même quand la tête bouge. Nouvelle passe enfichable « rétroviseurs » (étape Écran, avant la scène), avec les pipelines de la passe de reflet.
  - **Coût (RG-03)** : une glace redessinée par image (intérieur, gauche, intérieur, droite ; en Basse, une image sur deux). Images de 352 × 88 et 192 × 112 texels. Seuls les objets vus par la glace de l'image sont redessinés (nouveau bit `VIS_RETRO` du `Renderer`). Sans ombres. Personnes jusqu'à 25 m. Rien hors de la vue Cockpit (RG-04).
  - **Voiture du joueur** : vue **seulement dans les glaces** (`Renderer::retroSeul`), en version allégée (4 pièces en Basse), sans ses vitres (elles voilaient la glace de bleu). Dans la glace intérieure, son habitacle est assombri (albédo × 0,25, occlusion d'un habitacle fermé) : on y voit la lunette arrière, les appuie-têtes et la voiture qui suit. L'habitacle du cockpit n'est jamais dans les glaces (`horsRetro`).
  - **Glaces extérieures** : dessinées dans l'habitacle du cockpit, à l'angle avant de la vitre, hors de la carrosserie (largeur de la fiche). La place du rétroviseur du modèle 3D est **mesurée au chargement** (`mesurerRetroviseurs`, une ligne dans `model_loader.cpp` ; trouvée sur 18 voitures sur 20). Le reflet est calculé depuis cette place : le flanc de la voiture est au bon endroit, et la coque du modèle est coupée par le plan proche. Avant la mesure, elle remplissait 70 % de la glace. Réglage du conducteur : la carrosserie occupe le cinquième intérieur de la glace.
  - **Pièces modelées, aucune forme nue** (`vehicule_cockpit.cpp`) :
    - coque extérieure galbée (surface balayée : lèvre arrondie, logement de la glace en retrait, nez qui se resserre vers la portière) ;
    - bras profilé et aile moulée à l'angle de la vitre ;
    - glace à coins arrondis (en « goutte » côté extérieur), chant de 2 mm.
  - **Rétroviseur intérieur** : coque de WFS-323 (≈ 25 × 7 cm, collé en haut du pare-brise, ANO-207), en plastique satiné. En noir verni, un trait blanc crénelé apparaissait au-dessus de la glace.
  - **Shader** (`scene.frag`) : une glace à reflet (`F_EMTEX` + `params.z = −2`) affiche l'image du reflet × la réflexion (intérieur 80 %, extérieurs 50 % ; ONU R46 : ≥ 40 %), sans diffus ni reflet du ciel.
  - **Accroches annoncées au journal** :
    - `renderer.{h,cpp}` : `vis` sur 16 bits et `VIS_RETRO`, `FrameParams::retro`, `retroSeul` / `horsRetro`, halos dans la glace, voiture du joueur allégée dans la glace ;
    - `game.cpp` (7 lignes), `model_loader.cpp` (1 ligne), `camera_vue.cpp` (`ambiance()` choisit la glace de l'image), `passes.h` (2 accesseurs), `desktop_main.cpp` (banc `retroviseurs`) ;
    - `vehicule_cockpit_banc.cpp` : glaces figées pendant le banc WFS-349.
- **Comment c'est vérifié** :
  - `tests_cpu.sh` : 100 tests verts. Nouveau `test_retroviseurs` (21 cas) :
    - réflexion ;
    - coins de la glace aux coins de l'image ;
    - point vu par la loi de la réflexion au bon texel ;
    - côté non réfléchissant coupé ;
    - 20 intérieurs : place, réglage vers l'arrière, tailles ONU R46 ;
    - mesure des rétroviseurs sur un modèle.
  - Nouveau banc `WFS_BANC=retroviseurs` (Golf, Ville, 2400 × 1080, jour et nuit) : **0 écart**. Voiture 10 m derrière dans la même voie, puis 9 m derrière et 3,3 m à gauche. Pixels de la voiture vus dans chaque glace (image avec / sans la voiture, `retroviseurs-mesures.txt`) :
    - même voie : glace intérieure 16 063 px, gauche 4 832 px, droite 5 880 px ;
    - voie de gauche : glace gauche 540 px ; glace intérieure 0 px (hors de son champ, comme en vrai) ;
    - de nuit : on voit les phares de la voiture derrière.
  - Zooms sur trois autres voitures : 911, M4, Range Rover Sport.
  - **Coût mesuré en Basse et en Haute** (sur le banc PC lavapipe : 4 × 40 images avec et sans rétroviseurs, en alternance, médianes) :
    - Basse : écart de 2,9 %, soit **0,60 img/s perdue à 21 img/s** (estimé, ≤ 2 visé) ;
    - Haute : écart de 2,7 %, soit **0,56 img/s perdue à 21 img/s** (estimé, ≤ 2 visé) ;
    - quand la passe tourne : ≈ 105 appels de dessin et 300 k triangles.
  - Capture 3 du ticket : Réglages en vue Cockpit, Basse, sans puis avec rétroviseurs (`retro-reglages-rendu-basse-*`) : ligne « Rendu », appels et triangles.
  - Banc `cockpit-voisine` (WFS-349, Golf) : 0 écart.
  - Banc `cockpit` (Golf), avant / après : le reste du cockpit est inchangé.
- **Limites et risques connus** :
  - Coût mesuré sur le banc PC, pas sur le téléphone : la perte en img/s est une estimation (même part du temps de l'image). Le ticket passe 🟡 À valider : c'est l'utilisateur qui confirme sur l'APK.
  - Les glaces sont redessinées à tour de rôle : une glace extérieure a jusqu'à 3 images de retard (7 en Basse).
  - Pas d'ombres dans les glaces (RG-03).
  - Selon la voiture, la glace droite est au bord de l'écran, en partie derrière le montant droit, comme dans une vraie voiture.
  - Place et taille des glaces extérieures : estimées (E). M4 et Corvette : rétroviseur du modèle non trouvé (place estimée, plan proche à 80 cm).
  - Le modèle 3D a son rétroviseur 50 à 75 cm plus en arrière que l'habitacle dessiné : l'œil du cockpit est placé trop en avant dans le modèle. À signaler au PO, hors de ce ticket.
  - Capture des Réglages : la ligne « Caméra » montre le choix enregistré (Arrière), pas la vue Cockpit forcée par le banc.

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

### Autres voitures (après seulement, banc `retroviseurs` rapide)

| Capture | Après |
|---|---|
| `retro-m4-derriere-zoom-droite` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-m4-derriere-zoom-droite.jpg?raw=true) |
| `retro-m4-derriere-zoom-gauche` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-m4-derriere-zoom-gauche.jpg?raw=true) |
| `retro-m4-derriere-zoom-interieur` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-m4-derriere-zoom-interieur.jpg?raw=true) |
| `retro-m4-derriere` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-m4-derriere.jpg?raw=true) |
| `retro-p911-derriere-zoom-gauche` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-p911-derriere-zoom-gauche.jpg?raw=true) |
| `retro-p911-derriere-zoom-interieur` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-p911-derriere-zoom-interieur.jpg?raw=true) |
| `retro-p911-derriere` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-p911-derriere.jpg?raw=true) |
| `retro-rrsport-derriere-zoom-droite` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-rrsport-derriere-zoom-droite.jpg?raw=true) |
| `retro-rrsport-derriere-zoom-gauche` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-rrsport-derriere-zoom-gauche.jpg?raw=true) |
| `retro-rrsport-derriere-zoom-interieur` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-rrsport-derriere-zoom-interieur.jpg?raw=true) |
| `retro-rrsport-derriere` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-324/qa/pr/WFS-324/apres-retro-rrsport-derriere.jpg?raw=true) |
