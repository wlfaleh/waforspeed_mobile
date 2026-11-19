Agent : Claude, agent dev3d (01UY1Vv3kc)

**Ticket** : WFS-562 (P1, épic E23) — règles : `regles-de-gestion/WFS-562.md`

## Description

- **Demande de l'utilisateur** : « mettre des photos parlantes genre checkpoint voiture avec checkpoint, poursuite voiture avec police » sur l'écran « Choisis ton mode » (mise en page de la maquette 02 gardée), écran pas noir sur le téléphone (ANO-314), et « il faut évoluer les checkpoint » : portique de checkpoint en vrai modèle, sans cyan ni néon (RG-05 bis, RG-05 ter, ANO-279 partie checkpoint, ANO-315).
- **Ce qui change** :
  - **Portique de checkpoint refait** (`src/world/portique_checkpoint_modele.*`, `src/world/portique.cpp`) : portique d'événementiel en treillis carré d'aluminium de 290 mm (treillis, manchons et platines du portique de départ WFS-559, réutilisés), deux poutres, hauteur libre 5,2 m, **bâche PVC imprimée** tendue entre les poutres (ourlet, œillets tous les 0,5 m, tendeurs à boule), « CHECKPOINT N » imprimé **noir sur orange**, damier aux deux bouts, **non lumineux** ; la nuit deux projecteurs LED (boîtier à ailettes, lyre, bras) éclairent la bâche (une lumière en cône). **Ligne de chronométrage peinte en blanc** (0,30 m, 3 mm) à la place de la bande cyan au sol. **Plus aucun cyan ni néon** dans le portique. Courbe serrée ou tunnel : ligne peinte seule (comme avant). Œillets et tendeurs dessinés seulement à moins de 40 m. 31 000 triangles de loin, 47 000 de près.
  - **3 photos des modes** (`assets/ui/modes/*.jpg`, 720 × 546, 55 à 85 Ko) faites **une fois** dans le jeu par le banc `modes-photos` (`src/game/modes_photos_banc.cpp`, outil `tools/ui/photos_modes.sh`) : Ville de jour, la Golf du joueur arrive sous le nouveau portique ; Ville de jour, la Golf au milieu de 4 concurrents ; Ville de nuit, la Golf poursuivie par la police aux gyrophares (reflet bleu sur la carrosserie). Aucune image venue d'ailleurs, aucune retouche.
  - **Affichage** (`src/ui/modes_photos.*`, `src/game/modes_ecran.cpp`) : la photo remplit la carte inclinée (découpée par le parallélogramme, sans déformation), voile sombre sous le sujet pour le nom et la phrase ; distance et difficulté inchangées. Chargées à la 1re ouverture de l'écran (planche de 4,5 Mo en mémoire graphique), libérées à l'arrêt du moteur ; **repli** sur la scène floutée si une photo manque (RG-06, capture `modes-ecran-sans-photos`).
  - **Fond (RG-07)** : voile du fond flouté de 62 % à **22 %** ; luminosité moyenne du fond hors cartes mesurée sur la capture 2400 × 1080 : **25,7 %** (≥ 25 %), ≈ 13 % avant (calcul : même scène, voile 0,62).
  - **Accroches hors zone, annoncées** : `src/render/renderer.*` + `shaders/ui.frag` (≈ 30 lignes : l'interface sait afficher une photo, binding 1 du jeu de la police, inutilisé jusqu'ici) ; `src/game/game.*` (2 lignes, banc seulement : image sans interface, point visé de la caméra de vitrine) ; `portique_depart_modele.*` (`platine` devient publique) ; `desktop_main.cpp` (3 bancs).
- **Comment c'est vérifié** : compilation complète ; **155 tests CPU réussis** dont le nouveau `test_portique_checkpoint` (mesures ±1 % : largeur, hauteur libre 5,2 m, bâche 1,29 m, œillets Ø 24 mm au pas ≤ 0,5 m, ligne 0,30 m ; aucun triangle dégénéré ni inversé ; budget) ; bancs `portique-checkpoint` (Ville et Corniche, jour et nuit, 16 captures), `modes-ecran` (3 cartes FR / EN + repli), `parcours` et `vie` avant / après.
- **Accord du PO** pour l'accroche du rendu hors zone (`renderer.*`, `ui.frag`) : section Suivi de `regles-de-gestion/WFS-562.md` (commit 97672604 sur `main`).
- **Limites et risques** : la Corniche a son 1er checkpoint en courbe serrée (ligne peinte seule, règle WFS-139 inchangée) ; textes 2D de course (« CHECKPOINT N +30 s ») : WFS-567 ; le portique de départ n'a pas changé (hors périmètre). Validation 🟡 par l'utilisateur sur son téléphone. Avis de l'agent 3D demandé (RG-08).

## Tests CPU

`bash tools/qa/tests_cpu.sh build` sur la branche synchronisée avec `main` (14634dad) : **155 test(s) réussi(s) en 437 s**, 0 échec, dont le nouveau `test_portique_checkpoint` (mesures ±1 %, qualité des maillages, budget, ligne seule en courbe serrée). Journal complet : [`qa/pr/WFS-562/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/tests-cpu.log).

## Captures pour le lead dev

Dossier : [`qa/pr/WFS-562`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562) — produit par `tools/qa/captures_pr.sh` (avant : `origin/main`).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `parcours-mode-3-cartes` — écran « Choisis ton mode » (2400 × 1080) : cartes noires → photos qui disent le mode, fond moins sombre | ![avant-parcours-mode-3-cartes](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/avant-parcours-mode-3-cartes.jpg?raw=true) | ![apres-parcours-mode-3-cartes](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/apres-parcours-mode-3-cartes.jpg?raw=true) |
| `vie-portique` — portique de checkpoint en course, 25 m avant : poteaux en boîtes et néons cyan → treillis, bâche imprimée, ligne peinte | ![avant-vie-portique](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/avant-vie-portique.jpg?raw=true) | ![apres-vie-portique](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/apres-vie-portique.jpg?raw=true) |

**Nouvelles captures (après seulement : banc nouveau)** — toutes les autres (`pc-*` de jour et de nuit, `modes-ecran-en-*`) sont dans le dossier.

| Capture | Après |
|---|---|
| `modes-ecran-fr-checkpoint` | ![apres-modes-ecran-fr-checkpoint](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/apres-modes-ecran-fr-checkpoint.jpg?raw=true) |
| `modes-ecran-fr-course5` | ![apres-modes-ecran-fr-course5](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/apres-modes-ecran-fr-course5.jpg?raw=true) |
| `modes-ecran-fr-poursuite` | ![apres-modes-ecran-fr-poursuite](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/apres-modes-ecran-fr-poursuite.jpg?raw=true) |
| `modes-ecran-en-course5` | ![apres-modes-ecran-en-course5](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/apres-modes-ecran-en-course5.jpg?raw=true) |
| `modes-ecran-sans-photos` | ![apres-modes-ecran-sans-photos](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/apres-modes-ecran-sans-photos.jpg?raw=true) |
| `pc-ville-jour-34` | ![apres-pc-ville-jour-34](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/apres-pc-ville-jour-34.jpg?raw=true) |
| `pc-ville-jour-dessous` | ![apres-pc-ville-jour-dessous](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/apres-pc-ville-jour-dessous.jpg?raw=true) |
| `pc-ville-jour-ligne` | ![apres-pc-ville-jour-ligne](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/apres-pc-ville-jour-ligne.jpg?raw=true) |
| `pc-ville-nuit-34` | ![apres-pc-ville-nuit-34](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/apres-pc-ville-nuit-34.jpg?raw=true) |
| `pc-ville-nuit-dessous` | ![apres-pc-ville-nuit-dessous](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/apres-pc-ville-nuit-dessous.jpg?raw=true) |
| `pc-plage-jour-34` | ![apres-pc-plage-jour-34](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/apres-pc-plage-jour-34.jpg?raw=true) |
| `pc-plage-nuit-approche` | ![apres-pc-plage-nuit-approche](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-562/qa/pr/WFS-562/apres-pc-plage-nuit-approche.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv3kcVfmbaBKf14LPpJ
