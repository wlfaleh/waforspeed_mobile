Agent : Claude, agent 1 (01UY1Vv2)

**Ticket** : WFS-193 — règles : `regles-de-gestion/WFS-193.md` (RG-05 à RG-07 du PO, 2026-11-09, après la 2e revue de #311 : dernier essai de la X5)

## Description

- **Demande de l'utilisateur** (en une phrase) : voitures sans marque (file A) : petites retouches de forme pour qu'on ne reconnaisse plus les marques. Cette 2e PR traite la **X5**, retirée de la PR pilote #308 (Compass, 208 et l'outil, déjà fusionnés).
- **Méthode tranchée par le PO après la 2e revue** :
  - RG-05 : grille d'un seul tenant, cadre et barres au même galbe ;
  - RG-06 : grille posée dans l'ouverture d'origine, 1 à 2 cm derrière le bord de la carrosserie, qui cache la jonction ;
  - RG-07 : bord du pare-chocs rentré seulement s'il reste un vide visible.
- **Ce qui change** :
  - **`tools/formes.json`**, entrée `x5` (4 retouches, empreintes avant `4fc2afea68dc4d96`, après `8a664fb46210b69c`) :
    1. **calandre de marque retirée** (matériau 15 : les deux « haricots », leur cadre chromé, leurs barres et les arcs du bas, 2 640 triangles comptés) ;
    2. **fond des haricots retiré** (matériau 10, 240 triangles comptés) ;
    3. **capot entier, bord net** :
       - aucun triangle de carrosserie n'est retiré ;
       - la pointe de carrosserie entre les deux haricots est repliée dans le bord du capot, en hauteur et en profondeur (× 0,15, 47 sommets comptés) ;
    4. **grille générique d'un seul tenant** :
       - 6 barres, cadre de 1,5 cm, gris foncé métallisé ;
       - **cadre et barres au même galbe (4,5 cm)** : plus de cadre séparé devant les barres, donc plus de « pare-buffle » ;
       - bords 1 à 2 cm **derrière** le bord de l'ouverture : la carrosserie cache la jonction, comme sur une calandre de série ;
       - bas un peu plus large (trapèze 1,05), à 0,63 m, derrière le haut du pare-chocs ;
       - haut à 0,915 m (rien par la fente capot-phares).
  - **RG-07** : un essai de retrait de la lèvre du pare-chocs (33 triangles) faisait des dents blanches aux coins bas, de face. Il n'est pas gardé : le bord du pare-chocs n'est pas touché.
  - **`tools/retoucher_formes.mjs`** : **inchangé** par rapport à `main`. L'option `galbeCadre` de la version précédente est retirée, elle ne sert plus.
  - **`assets/cars/x5.wfm`** et **`x5.lod`** régénérés par l'outil (tangentes, boîte et niveaux de détail recalculés).
  - **Banc** (pour la vue basse) :
    - accroche de 2 lignes dans le noyau : `Game::orbitHauteur` / `orbitRecul` (1,7 m et 6,5 m par défaut, la caméra de vitrine en jeu ne change pas) ;
    - `WFS_VITRINE_BAS="<angle> <hauteur> <recul>"` dans `feux_modele.cpp`. Les valeurs d'avant sont gardées dans des variables puis remises (remarque de la 2e revue).
- **Comment c'est vérifié** :
  - `retoucher_formes.mjs --verifier` : « déjà retouchée » pour la Compass, la 208 et la X5 ;
  - `test_fidelite_voitures` OK (20 voitures, dimensions ±1 % ou écarts connus) ;
  - **90 tests CPU verts**, branche synchronisée avec `main` ;
  - captures rendues en 2560×1440 puis recadrées, de jour et de nuit :
    - face ;
    - trois quarts droit et gauche ;
    - trois quarts haut ;
    - vue basse ;
    - **zooms × 4** : coin bas droit et gauche, centre du bord du capot, montant et bas de la grille en vue basse.

    Plus les vitrines de jour et les feux de nuit. Les « avant » sont pris avec le même binaire et les `.wfm` / `.lod` de `main`.
- **Limites et risques connus** :
  - Au coin bas droit (trois quarts droit), le bord du pare-chocs d'origine reste un peu dentelé au zoom × 4. Il était caché par le chrome des haricots. Le retirer faisait des dents aux coins de face : il est laissé tel quel (point 4 de la 2e revue, jugé non bloquant seul).
  - La grille garde des angles droits, sans la double forme des haricots, qui est la signature de la marque.
  - De nuit, la grille reste sombre : les phares éclairent devant la voiture, pas sa calandre.
  - Les feux, les jantes et les autres signatures viendront au lot suivant ; la phrase de licence WFS-193 sera ajoutée au lot final.

## Captures pour le lead dev (avant = `main`)

| Capture | Avant | Après |
|---|---|---|
| `nez-x5-face-nuit` — X5, De face, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-face-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-face-nuit.jpg?raw=true) |
| `nez-x5-face-jour` — X5, De face, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-face-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-face-jour.jpg?raw=true) |
| `nez-x5-bas-jour` — X5, Vue basse au niveau du pare-chocs, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-bas-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-bas-jour.jpg?raw=true) |
| `nez-x5-bas-nuit` — X5, Vue basse, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-bas-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-bas-nuit.jpg?raw=true) |
| `zoom-x5-bas-montant-jour` — X5, Zoom × 4, vue basse : montant et haut de la grille, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-zoom-x5-bas-montant-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-zoom-x5-bas-montant-jour.jpg?raw=true) |
| `zoom-x5-bas-montant-nuit` — X5, Zoom × 4, vue basse : montant, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-zoom-x5-bas-montant-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-zoom-x5-bas-montant-nuit.jpg?raw=true) |
| `zoom-x5-bas-grille-jour` — X5, Zoom × 4, vue basse : bas de la grille et haut du pare-chocs, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-zoom-x5-bas-grille-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-zoom-x5-bas-grille-jour.jpg?raw=true) |
| `zoom-x5-bas-grille-nuit` — X5, Zoom × 4, vue basse : bas de la grille, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-zoom-x5-bas-grille-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-zoom-x5-bas-grille-nuit.jpg?raw=true) |
| `nez-x5-34-jour` — X5, Trois quarts droit, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-34-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-34-jour.jpg?raw=true) |
| `nez-x5-34-nuit` — X5, Trois quarts droit, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-34-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-34-nuit.jpg?raw=true) |
| `zoom-x5-coin-droit-jour` — X5, Zoom × 4 : coin bas droit, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-zoom-x5-coin-droit-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-zoom-x5-coin-droit-jour.jpg?raw=true) |
| `zoom-x5-coin-droit-nuit` — X5, Zoom × 4 : coin bas droit, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-zoom-x5-coin-droit-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-zoom-x5-coin-droit-nuit.jpg?raw=true) |
| `nez-x5-34g-jour` — X5, Trois quarts gauche, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-34g-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-34g-jour.jpg?raw=true) |
| `nez-x5-34g-nuit` — X5, Trois quarts gauche, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-34g-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-34g-nuit.jpg?raw=true) |
| `zoom-x5-coin-gauche-jour` — X5, Zoom × 4 : coin bas gauche, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-zoom-x5-coin-gauche-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-zoom-x5-coin-gauche-jour.jpg?raw=true) |
| `zoom-x5-coin-gauche-nuit` — X5, Zoom × 4 : coin bas gauche, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-zoom-x5-coin-gauche-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-zoom-x5-coin-gauche-nuit.jpg?raw=true) |
| `nez-x5-capot-jour` — X5, Trois quarts haut, au-dessus du capot, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-capot-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-capot-jour.jpg?raw=true) |
| `nez-x5-capot-nuit` — X5, Trois quarts haut, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-capot-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-capot-nuit.jpg?raw=true) |
| `zoom-x5-centre-jour` — X5, Zoom × 4 : centre du bord du capot, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-zoom-x5-centre-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-zoom-x5-centre-jour.jpg?raw=true) |
| `zoom-x5-centre-nuit` — X5, Zoom × 4 : centre du bord du capot, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-zoom-x5-centre-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-zoom-x5-centre-nuit.jpg?raw=true) |
| `vitrine-x5-a1` — X5, Vitrine de trois quarts, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-a1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-a1.jpg?raw=true) |
| `vitrine-x5-a2` — X5, De dos, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-a2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-a2.jpg?raw=true) |
| `vitrine-x5-feux-nuit` — X5, Feux de nuit (frein) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-feux-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-feux-nuit.jpg?raw=true) |

Les vitrines `vitrine-x5-0`, `-a0`, `-feux` sont aussi dans `qa/pr/WFS-193/`, avec les captures de la PR pilote (Compass et 208).

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv2MVfXZF3cPf1r8qRq
