Agent : Claude, agent 1 (01UY1Vv2)

**Ticket** : WFS-193 — règles : `regles-de-gestion/WFS-193.md` (Suivi : décision du PO du 2026-11-09, option (a) : X5 dans une 2e PR du même ticket)

## Description

- **Demande de l'utilisateur** (en une phrase) : voitures sans marque (file A) : petites retouches de forme pour qu'on ne reconnaisse plus les marques. Cette 2e PR traite la **X5**, retirée de la PR pilote #308 (Compass, 208 et l'outil, déjà fusionnés).
- **Ce que demandait le 3e refus de #308** :
  - capot déchiré derrière la grille de trois quarts ;
  - grille qui fait « plaque rapportée ».

  Le PO a demandé de rendre les triangles du capot et de poser la grille dans l'ouverture d'origine, avec des gros plans de trois quarts de jour et de nuit.
- **Ce qui change** (données seulement : aucun code changé, l'outil `tools/retoucher_formes.mjs` de #308 est réutilisé tel quel) :
  - **`tools/formes.json`**, entrée `x5` (4 retouches, empreintes avant `4fc2afea68dc4d96`, après `4f457585c333e28d`) :
    1. **calandre de marque retirée** (matériau 15 : les deux « haricots », leur cadre chromé, leurs barres et les arcs du bas, 2 640 triangles comptés) ;
    2. **fond des haricots retiré** (matériau 10, 240 triangles comptés) ;
    3. **capot entier** : aucun triangle de carrosserie n'est retiré.
       - La pointe de carrosserie qui descendait entre les deux haricots est remontée jusqu'au bord du capot, par une échelle verticale de 0,15 avec fondu (45 sommets comptés).
       - La carrosserie reste continue, sans trou.
    4. **grille générique en retrait**, posée derrière la carrosserie dans l'ouverture d'origine, qui en fait le tour :
       - 6 barres, cadre de 1,5 cm, galbe du nez de 6 cm ;
       - gris foncé métallisé.

       Le haut est à 0,915 m : plus haut, un coin de la grille se voyait par la fente entre le capot et les phares. Ce défaut a été trouvé par un essai avec la grille en rouge.
  - **`assets/cars/x5.wfm`** et **`x5.lod`** régénérés par l'outil (tangentes, boîte et niveaux de détail recalculés).
- **Comment c'est vérifié** :
  - `retoucher_formes.mjs --verifier` : « déjà retouchée » pour la Compass, la 208 et la X5 ;
  - `effacer_logos.mjs --verifier` : rien à faire ;
  - `test_fidelite_voitures` OK (20 voitures, dimensions ±1 % ou écarts connus) ;
  - **88 tests CPU verts**, branche synchronisée avec `main` ;
  - captures :
    - vitrines de jour (`captures_pr.sh`, avant = `main`) ;
    - **gros plans du nez rendus en 2560×1440 puis recadrés**, de jour et de nuit : de face, de **trois quarts** et **de trois quarts haut, au-dessus du capot** ;
    - feux de nuit.

    Les « avant » sont pris avec le même binaire et les `.wfm` / `.lod` de `main`.
- **Limites et risques connus** :
  - La grille suit le contour de l'ouverture d'origine (sa largeur, son haut, son bas), mais garde des angles droits arrondis par le galbe. Elle n'a pas la double forme des haricots, qui est justement la signature de la marque.
  - De nuit, la grille reste sombre : les phares éclairent devant la voiture, pas sa calandre.
  - Les feux, les jantes et les autres signatures viendront au lot suivant, avec les 17 autres voitures.
  - La phrase de modification WFS-193 sera ajoutée aux licences au lot final.

## Captures pour le lead dev (avant = `main`)

| Capture | Avant | Après |
|---|---|---|
| `nez-x5-34-jour` — X5, gros plan de trois quarts, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-34-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-34-jour.jpg?raw=true) |
| `nez-x5-34-nuit` — X5, gros plan de trois quarts, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-34-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-34-nuit.jpg?raw=true) |
| `nez-x5-capot-jour` — X5, trois quarts haut, au-dessus du capot, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-capot-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-capot-jour.jpg?raw=true) |
| `nez-x5-capot-nuit` — X5, trois quarts haut, au-dessus du capot, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-capot-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-capot-nuit.jpg?raw=true) |
| `nez-x5-face-jour` — X5, gros plan de face, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-face-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-face-jour.jpg?raw=true) |
| `nez-x5-face-nuit` — X5, gros plan de face, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-x5-face-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-x5-face-nuit.jpg?raw=true) |
| `vitrine-x5-a0` — X5, de face, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-a0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-a0.jpg?raw=true) |
| `vitrine-x5-a1` — X5, de trois quarts, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-a1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-a1.jpg?raw=true) |
| `vitrine-x5-a2` — X5, de dos, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-a2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-a2.jpg?raw=true) |
| `vitrine-x5-feux-nuit` — X5, feux de nuit (frein) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-feux-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-feux-nuit.jpg?raw=true) |

Les vues d'ensemble (`vitrine-x5-0`) et les feux de jour (`vitrine-x5-feux`) sont aussi dans `qa/pr/WFS-193/`, avec les captures de la PR pilote (Compass et 208).

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv2MVfXZF3cPf1r8qRq
