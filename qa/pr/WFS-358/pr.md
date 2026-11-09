Agent : Claude, agent dev3d (01UY1Vv41d)

**Ticket** : WFS-358 — règles : `regles-de-gestion/WFS-358.md` (suite de WFS-193, PR #311 acceptée)

## Description

- **Demande de l'utilisateur** (en une phrase) : voitures sans marque ; ici, la calandre de la X5 doit avoir une jonction propre avec la carrosserie (les 4 défauts laissés par #311) et le léger contour de l'ancien logo rond du capot doit disparaître (revue du lead dev).
- **Causes trouvées** :
  - **dents de scie et éclats blancs** : #311 retirait la calandre « par centre de triangle » ; les triangles à cheval sur la limite restaient en escalier, et des trous traversants restaient aux coins bas (on voyait la lumière à travers) ;
  - **coin haut droit devant la carrosserie** : la grille était un trapèze posé à une profondeur fixe ; sous le phare, la carrosserie recule et la grille passait devant ;
  - **bord en peigne** : derrière le bord de l'ouverture il n'y a pas de paroi ; on voyait les bouts des barres sur l'intérieur clair (boîtier du phare) ;
  - **ailettes noires** entre les barres (vues de biais) : restes des barres verticales des haricots, vues « par la tranche » ;
  - **contour du logo** : le logo avait été retiré en laissant son biseau et un trou dans le capot (le disque blanc de jour, l'ovale de nuit).
- **Méthode (RG-03, a)** : grille découpée sur le contour exact de l'ouverture, toujours derrière la carrosserie. Aucun cadre en haricots, aucun « pare-buffle ».
- **Ce qui change** :
  - `tools/retoucher_formes.mjs` : 3 nouvelles retouches, rejouables (empreintes avant / après) :
    1. **découpe exacte** : la partie d'une pièce dans un contour vu de face (rectangle à coins bas arrondis) est retirée ; les triangles à cheval sont **coupés** le long du contour (bord net), les pièces vues par la tranche sont retirées en entier ;
    2. **grille ajustée** : sa profondeur **suit la carrosserie**, mesurée sur une carte de profondeur du modèle. Elle est partout 1,5 cm derrière ce qui est devant elle. Dans l'ouverture, la forme est interpolée, ce qui donne le galbe du nez. L'outil **refuse d'écrire** si un seul sommet de la grille est devant la carrosserie (RG-05). La grille comprend 9 lames chanfreinées et inclinées (profil à 6 faces, chanfreins de 3 mm), un fond noir mat 5 cm plus loin et un **conduit noir** tout autour, comme le conduit d'air en plastique d'une vraie calandre ;
    3. **bouchon** : les triangles du logo (biseau, trou) sont retirés, et la peau du capot est **recousue** à partir des sommets du bord du trou. C'est étanche, sans pièce rapportée, et les sommets du bord gardent leur position et leur normale.
  - `tools/formes.json`, entrée `x5` : la chaîne repart du modèle d'avant #311 (empreinte `4fc2afea68dc4d96`). Elle fait :
    - les découpes : calandre, fond, panneau intérieur, traverse ;
    - la pointe repliée de #311, inchangée ;
    - le bouchon du logo ;
    - la grille ajustée.

    Le nombre de triangles touchés est contrôlé ; empreinte après : `87570eb9ad0fa1c8`.
  - `assets/cars/x5.wfm` / `x5.lod` : régénérés par l'outil. Le `.wfm` pèse **7,54 Mo** (limite de 8 Mo). Les niveaux de détail sont refaits.
  - `src/render/feux_modele.cpp` (banc `WFS_VITRINE_BAS`, accroche annoncée au journal) : plusieurs vues par passage, séparées par « ; » (`vitrine-<id>-bas`, `-bas1`…). Sans « ; », rien ne change.
- **Comment c'est vérifié** :
  - `node tools/retoucher_formes.mjs --verifier` : « déjà retouchée » pour la Compass, la 208 et la X5. L'outil rejoué depuis le modèle d'origine donne un fichier identique, octet pour octet ;
  - contrôle RG-05 dans l'outil : 0 sommet de la grille devant la carrosserie ;
  - vue de face projetée (hors moteur) : plus aucun trou dans l'ouverture ni dans le capot ;
  - **91 tests CPU verts** (branche synchronisée avec `main`) (dont `test_fidelite_voitures` : dimensions ±1 %, la boîte de la X5 ne change pas) ;
  - captures : banc PC, même rendu que la CI (lavapipe). Elles comprennent :
    - le script `captures_pr.sh` (1920×1080, jour) ;
    - des vues rapprochées 2560×1440 de jour et de nuit : face, vue basse, trois quarts des deux côtés, capot ;
    - des **zooms ×4** : coins bas gauche et droit, coin haut droit sous le phare, bas de la grille en vue basse, emplacement du logo.

    Les « avant » des vues rapprochées sont faites avec le même binaire et le `x5.wfm` / `.lod` de `main` (le banc de `main` ne fait qu'une vue basse).
- **Limites et risques connus** :
  - **Logo du capot** : le disque blanc et l'ovale ont disparu. Au zoom ×4, en plein reflet du soleil, on devine encore un très léger losange à l'endroit recousu. Il vient du vernis, très sensible aux normales, qui sont interpolées sur une petite zone. Les essais écartés sont au journal : pièce posée devant, pièce posée derrière, zone recousue plus large.
  - Grille : +7 012 triangles (fond, conduit, lames). La X5 passe de 153 k à environ 158 k triangles au niveau 0 ; les niveaux 1 à 3 sont refaits par `alleger_voiture.mjs` (33,5 %, 16,6 %, 11,2 %).
  - La lame du bas passe en partie derrière le bord du pare-chocs (vue de face) : c'est voulu, la grille continue derrière la carrosserie.
  - La X5 sera remplacée plus tard (WFS-361, licence) ; l'outil et la méthode resservent pour les autres voitures.

## Captures pour le lead dev

Dossier : [`qa/pr/WFS-358`](https://github.com/wlfaleh/waforspeed_mobile/tree/feature/waforspeed-358/qa/pr/WFS-358).

### Zooms ×4 (défauts de RG-04 et logo), jour et nuit

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `zoom-x5-coin-bas-gauche-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-zoom-x5-coin-bas-gauche-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-zoom-x5-coin-bas-gauche-jour.jpg?raw=true) |
| `zoom-x5-coin-bas-gauche-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-zoom-x5-coin-bas-gauche-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-zoom-x5-coin-bas-gauche-nuit.jpg?raw=true) |
| `zoom-x5-coin-bas-droit-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-zoom-x5-coin-bas-droit-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-zoom-x5-coin-bas-droit-jour.jpg?raw=true) |
| `zoom-x5-coin-bas-droit-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-zoom-x5-coin-bas-droit-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-zoom-x5-coin-bas-droit-nuit.jpg?raw=true) |
| `zoom-x5-coin-haut-droit-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-zoom-x5-coin-haut-droit-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-zoom-x5-coin-haut-droit-jour.jpg?raw=true) |
| `zoom-x5-coin-haut-droit-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-zoom-x5-coin-haut-droit-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-zoom-x5-coin-haut-droit-nuit.jpg?raw=true) |
| `zoom-x5-bas-grille-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-zoom-x5-bas-grille-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-zoom-x5-bas-grille-jour.jpg?raw=true) |
| `zoom-x5-bas-grille-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-zoom-x5-bas-grille-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-zoom-x5-bas-grille-nuit.jpg?raw=true) |
| `zoom-x5-capot-logo-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-zoom-x5-capot-logo-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-zoom-x5-capot-logo-jour.jpg?raw=true) |
| `zoom-x5-capot-logo-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-zoom-x5-capot-logo-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-zoom-x5-capot-logo-nuit.jpg?raw=true) |

### Vues rapprochées (2560×1440 réduites), jour et nuit

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `nez-x5-face-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-nez-x5-face-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-nez-x5-face-jour.jpg?raw=true) |
| `nez-x5-face-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-nez-x5-face-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-nez-x5-face-nuit.jpg?raw=true) |
| `nez-x5-bas-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-nez-x5-bas-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-nez-x5-bas-jour.jpg?raw=true) |
| `nez-x5-bas-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-nez-x5-bas-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-nez-x5-bas-nuit.jpg?raw=true) |
| `nez-x5-34a-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-nez-x5-34a-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-nez-x5-34a-jour.jpg?raw=true) |
| `nez-x5-34a-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-nez-x5-34a-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-nez-x5-34a-nuit.jpg?raw=true) |
| `nez-x5-34b-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-nez-x5-34b-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-nez-x5-34b-jour.jpg?raw=true) |
| `nez-x5-34b-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-nez-x5-34b-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-nez-x5-34b-nuit.jpg?raw=true) |
| `nez-x5-capot-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-nez-x5-capot-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-nez-x5-capot-jour.jpg?raw=true) |
| `nez-x5-capot-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-nez-x5-capot-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-nez-x5-capot-nuit.jpg?raw=true) |

### Script `tools/qa/captures_pr.sh` (vitrine, 1920×1080, jour)

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `vitrine-x5-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-vitrine-x5-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-vitrine-x5-0.jpg?raw=true) |
| `vitrine-x5-a0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-vitrine-x5-a0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-vitrine-x5-a0.jpg?raw=true) |
| `vitrine-x5-a1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-vitrine-x5-a1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-vitrine-x5-a1.jpg?raw=true) |
| `vitrine-x5-a2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-vitrine-x5-a2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-vitrine-x5-a2.jpg?raw=true) |
| `vitrine-x5-bas` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-vitrine-x5-bas.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-vitrine-x5-bas.jpg?raw=true) |
| `vitrine-x5-bas1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-vitrine-x5-bas1.jpg?raw=true) |
| `vitrine-x5-bas2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-vitrine-x5-bas2.jpg?raw=true) |
| `vitrine-x5-bas3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-vitrine-x5-bas3.jpg?raw=true) |
| `vitrine-x5-bas4` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-vitrine-x5-bas4.jpg?raw=true) |
| `vitrine-x5-feux` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/avant-vitrine-x5-feux.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-358/qa/pr/WFS-358/apres-vitrine-x5-feux.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv41dPBrq3vbTpDyTvg
