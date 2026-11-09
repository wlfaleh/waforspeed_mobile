Agent : Claude, agent 1 (01UY1Vv2)

**Ticket** : WFS-193 — règles : `regles-de-gestion/WFS-193.md` (Suivi : décision du PO du 2026-11-09, option (a) : X5 dans une 2e PR du même ticket)

## Description

- **Demande de l'utilisateur** (en une phrase) : voitures sans marque (file A) : petites retouches de forme pour qu'on ne reconnaisse plus les marques. Cette 2e PR traite la **X5**, retirée de la PR pilote #308 (Compass, 208 et l'outil, déjà fusionnés).
- **1re revue de #311, corrigée** :
  1. pli au milieu du nez (pointe écrasée) ;
  2. coin bas de la grille de trois quarts (liseré clair et éclats de l'intérieur du pare-chocs) ;
  3. captures en plus.
- **Ce qui change** :
  - **`tools/formes.json`**, entrée `x5` (4 retouches, empreintes avant `4fc2afea68dc4d96`, après `21eccb31aae32e3c`) :
    1. **calandre de marque retirée** (matériau 15 : les deux « haricots », leur cadre chromé, leurs barres et les arcs du bas, 2 640 triangles comptés) ;
    2. **fond des haricots retiré** (matériau 10, 240 triangles comptés) ;
    3. **capot entier, bord net** :
       - aucun triangle de carrosserie n'est retiré ;
       - la pointe de carrosserie entre les deux haricots est **repliée dans le bord du capot**, en hauteur **et en profondeur** (× 0,15 vers le bord, 47 sommets comptés) ;
       - avant, elle n'était écrasée qu'en hauteur : sa lèvre restait en avant et faisait le pli ;
    4. **grille générique dans l'ouverture d'origine** :
       - barres et fond en retrait (galbe 6 cm), 6 barres, gris foncé métallisé ;
       - **cadre presque plat** (galbe 2 cm, 2 cm de large), devant le bord du pare-chocs : il **couvre la jonction** que cachait le cadre chromé des haricots ;
       - bas un peu plus large (trapèze 1,05) et descendu à 0,62 m ;
       - haut à 0,915 m : plus haut, un coin de la grille se voyait par la fente entre le capot et les phares.
  - **`tools/retoucher_formes.mjs`** : nouvelle option de grille **`galbeCadre`** (galbe de la face avant du cadre ; par défaut celui des barres). La Compass et la 208 sont régénérées à l'identique, avec les mêmes empreintes.
  - **`assets/cars/x5.wfm`** et **`x5.lod`** régénérés par l'outil (tangentes, boîte et niveaux de détail recalculés).
  - **Banc** (pour la vue basse demandée) :
    - accroche de 2 lignes dans le noyau : `Game::orbitHauteur` / `orbitRecul` (1,7 m et 6,5 m par défaut, la caméra de vitrine ne change pas) ;
    - `WFS_VITRINE_BAS="<angle> <hauteur> <recul>"` dans `feux_modele.cpp` (capture `vitrine-<id>-bas`, valeurs remises ensuite).
- **Comment c'est vérifié** :
  - `retoucher_formes.mjs --verifier` : « déjà retouchée » pour la Compass, la 208 et la X5 ;
  - `test_fidelite_voitures` OK (20 voitures, dimensions ±1 % ou écarts connus) ;
  - **88 tests CPU verts**, branche synchronisée avec `main` ;
  - captures rendues en 2560×1440 puis recadrées, de jour et de nuit :
    - de face ;
    - de trois quarts **des deux côtés** ;
    - de trois quarts haut, au-dessus du capot ;
    - **vue basse au niveau du pare-chocs** ;
    - **zooms × 4** du coin bas et du centre du bord du capot.

    Plus les vitrines de jour et les feux de nuit. Les « avant » sont pris avec le même binaire et les `.wfm` / `.lod` de `main`.
- **Limites et risques connus** :
  - Le cadre de la grille passe un peu devant le pare-chocs aux coins bas (c'est ce qui couvre la jonction) : de très près, il se lit comme un entourage de calandre.
  - Au zoom × 4 du coin bas (trois quarts), il reste un éclat clair de 2 à 3 pixels à la jonction entre le cadre et le bas du pare-chocs ; il ne se voit pas sur les gros plans.
  - Dans la bande haute du cadre, au centre, une ombre très légère reste visible sur le zoom × 4, à l'endroit de la pointe repliée.
  - La grille garde des angles droits, sans la double forme des haricots, qui est la signature de la marque.
  - De nuit, la grille reste sombre : les phares éclairent devant la voiture, pas sa calandre.
  - Les feux, les jantes et les autres signatures viendront au lot suivant ; la phrase de licence WFS-193 sera ajoutée au lot final.

## Captures pour le lead dev (avant = `main`)

| Capture | Avant | Après |
|---|---|---|
| `zoom-x5-centre-jour` — Zoom × 4 : centre du bord du capot, trois quarts haut, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-zoom-x5-centre-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-zoom-x5-centre-jour.jpg?raw=true) |
| `zoom-x5-centre-nuit` — Zoom × 4 : centre du bord du capot, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-zoom-x5-centre-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-zoom-x5-centre-nuit.jpg?raw=true) |
| `zoom-x5-coin-jour` — Zoom × 4 : coin bas de la grille, trois quarts, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-zoom-x5-coin-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-zoom-x5-coin-jour.jpg?raw=true) |
| `zoom-x5-coin-nuit` — Zoom × 4 : coin bas de la grille, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-zoom-x5-coin-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-zoom-x5-coin-nuit.jpg?raw=true) |
| `nez-x5-34-jour` — X5, trois quarts (côté droit), jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-34-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-34-jour.jpg?raw=true) |
| `nez-x5-34-nuit` — X5, trois quarts (côté droit), nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-34-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-34-nuit.jpg?raw=true) |
| `nez-x5-34g-jour` — X5, trois quarts côté gauche, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-34g-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-34g-jour.jpg?raw=true) |
| `nez-x5-34g-nuit` — X5, trois quarts côté gauche, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-34g-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-34g-nuit.jpg?raw=true) |
| `nez-x5-capot-jour` — X5, trois quarts haut, au-dessus du capot, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-capot-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-capot-jour.jpg?raw=true) |
| `nez-x5-capot-nuit` — X5, trois quarts haut, au-dessus du capot, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-capot-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-capot-nuit.jpg?raw=true) |
| `nez-x5-bas-jour` — X5, vue basse au niveau du pare-chocs, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-bas-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-bas-jour.jpg?raw=true) |
| `nez-x5-bas-nuit` — X5, vue basse au niveau du pare-chocs, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-bas-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-bas-nuit.jpg?raw=true) |
| `nez-x5-face-jour` — X5, de face, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-face-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-face-jour.jpg?raw=true) |
| `nez-x5-face-nuit` — X5, de face, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-face-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-face-nuit.jpg?raw=true) |
| `vitrine-x5-a1` — X5, vitrine de trois quarts, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-a1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-a1.jpg?raw=true) |
| `vitrine-x5-a2` — X5, de dos, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-a2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-a2.jpg?raw=true) |
| `vitrine-x5-feux-nuit` — X5, feux de nuit (frein) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-feux-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-feux-nuit.jpg?raw=true) |

Les vitrines `vitrine-x5-0`, `-a0`, `-feux` sont aussi dans `qa/pr/WFS-193/`, avec les captures de la PR pilote (Compass et 208).

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv2MVfXZF3cPf1r8qRq
