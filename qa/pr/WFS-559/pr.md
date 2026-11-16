Agent : Claude, agent dev3d (01Us6uCwnA)

**Ticket** : WFS-559 — règles : `regles-de-gestion/WFS-559.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : chaque écran respecte sa maquette R19 à 100 % ; ici la maquette 09 « départ de la course » : un portique en treillis d'aluminium au-dessus de la ligne de départ, 5 feux rouges qui s'allument un par un, la consigne « Garde le doigt sur GAZ » dessous (WFS-559, arbitrages du PO RG-05 à RG-13).
- **Ce qui change** :
  - `src/world/portique_depart_modele.{h,cpp}` (nouveau) : modèle 3D calculé, aux vraies mesures d'un treillis carré d'événementiel de 290 mm (membrures Ø 50 mm, diagonales Ø 20 mm en zigzag sur les 4 faces, traverses, tronçons de 3 m / 2 m avec manchons coniques et broches), platines d'acier galvanisé chanfreinées 600 × 600 × 10 mm avec 4 boulons M24, caisson d'aluminium noir à arêtes arrondies 4,6 × 0,62 × 0,22 m pendu par 3 colliers, 5 colonnes de 2 lanternes Ø 200 mm (lentille bombée, bague, visière de 160 mm). Ouverture = chaussée 15 m + 1 m de chaque côté, hauteur libre 5,5 m (RG-02). `construire(…, avecFeux = false)` donne le même portique sans feux pour l'ARRIVÉE de WFS-560. Aucune forme nue : ≈ 20 800 triangles, 0 boîte ni disque.
  - `src/world/portique_depart.cpp` (nouveau système, ordre 112) : portique posé par le code à la ligne de départ (damier, s = 0) de chaque carte, dans tous les modes (RG-03, RG-09) ; pas dans les paquets `.wfcarte` ; pas sur le Grand Prix (il a déjà son portique de feux, WFS-373). Construit une fois au premier dessin, rien créé pendant la course (RG-C5). Matériaux PBR : aluminium brut (F0 0,91, rugosité 0,38), acier galvanisé, laque noire, polycarbonate ; LED rouges / vertes émissives. La nuit, une lumière ponctuelle rouge (verte au GO) éclaire le treillis (RG-04). Consigne FR / EN sous le caisson projeté à l'écran (RG-08), « GO » orange à sa place pendant 0,7 s (RG-05).
  - `src/game/course.{h,cpp}` : `Course::feuxRouges()` / `feuxVerts()` : le décompte existant pilote les feux (0 avant le 1er « 3 », un toutes les 0,6 s, le 5e à −0,6 s, verts pendant le GO) ; durée du décompte et `Course::hold` inchangés (RG-01, RG-06).
  - `src/game/course_system.cpp` : les gros chiffres 3 / 2 / 1 / GO du centre sont retirés (RG-05) ; bips de `sfx.cpp` inchangés.
  - accroches annoncées au journal (1 ligne chacune, RG-10 / RG-11) : `src/ui/ecrans/pause.cpp` et `src/game/camera_vue.cpp` (`boutonVisible`) : MENU et ARRIÈRE cachés pendant le décompte seulement ; `src/platform/desktop_main.cpp` (banc `portique-depart`).
  - textes `depart.consigne.*` dans `assets/textes/fr.txt` et `en.txt` ; test `tests/test_portique_depart.cpp` ; banc `src/world/portique_depart_banc.cpp`.
- **Comment c'est vérifié** : `test_portique_depart` (section 290 mm, Ø 50 / 20 mm, largeur 17,58 m, hauteur libre 5,49 m, pieds à 8,49 m de l'axe, lentilles Ø 200 mm, caisson 4,6 m au-dessus de 4,5 m, visières 160 mm, tous à ±1 % ; 0 triangle dégénéré ou inversé, normales unitaires ; budget ≤ 25 000 ; cadence des feux) ; les 151 tests CPU passent (`tools/qa/tests_cpu.sh`) ; banc `portique-depart` en 2400 × 1080 : Ville et Corniche, jour et nuit, Checkpoint et Course à 5 : décompte de 3 s, 3 puis 5 rouges, verts au départ, éteints 1 s après, bouton de vue caché pendant le décompte et revenu au départ : 0 écart ; banc `depart` (avant / après sur main).
- **Limites et risques connus** :
  - la caméra n'a pas changé (RG-12) : en vue « Proche », le caisson est coupé par le haut de l'image ; en vue « Arrière » (par défaut) il est entier. La maquette 09 a une caméra plus basse que celle du jeu.
  - le Grand Prix (en pause) garde ses propres feux (`circuit.cpp`, formule recopiée à l'époque) ; il n'appelle pas encore `Course::feuxRouges` (hors zone : à faire à sa reprise).
  - `RG-07` : pas de mécanique de « départ parfait » dans le code : la consigne reste le texte de la maquette, sans bonus.
  - « GO » au départ : les boutons MENU / ARRIÈRE reviennent en même temps (RG-10) et couvrent une partie des feux verts.

## Maquette ↔ code (règle du 2026-11-15 23:14 UTC)

| Élément de la maquette 09 (`references/maquettes-R19/09-depart-course.jpg`) | Fichier : fonction |
|---|---|
| portique en treillis aluminium au-dessus de la ligne | `src/world/portique_depart_modele.cpp` : `treillis`, `construire` ; posé par `src/world/portique_depart.cpp` : `PortiqueDepart::construire` / `render` |
| 5 feux rouges allumés un par un | `src/game/course.cpp` : `Course::feuxRouges` ; `src/world/portique_depart.cpp` : `PortiqueDepart::render` (matériau rouge / éteint par colonne) ; `portique_depart_modele.cpp` : `lanterne` |
| lueur rouge des feux la nuit | `PortiqueDepart::ambiance` (lumière ponctuelle) + émission des lentilles (bloom) |
| consigne « Garde le doigt sur GAZ : lâche les freins au vert pour un départ parfait » (GAZ en orange, bandeau sombre) | `src/world/portique_depart.cpp` : `PortiqueDepart::drawUi` ; textes `depart.consigne.*` |
| aucun chiffre de décompte au centre | `src/game/course_system.cpp` : `CourseSystem::drawUi` (bloc retiré) |
| pas de boutons au centre en haut (seulement TEMPS et SCORE) | `src/ui/ecrans/pause.cpp` et `src/game/camera_vue.cpp` : `boutonVisible` (pendant le décompte) |
| voiture du joueur, adversaires, chaussée, TEMPS / SCORE | inchangés (WFS-550 / 551, voitures `.wfm`) |

## Captures pour le lead dev (18 après)

Dossier : [`qa/pr/WFS-559`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559) — produit par `tools/qa/captures_pr.sh`.

Captures « avant » : `origin/main` = `68a71e26edc4f336bd583795aece05af9386c094` ; cache : 0 banc(s) relu(s), 2 banc(s) rejoué(s) ; main compilé : oui.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `depart-jour-ville-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/avant-depart-jour-ville-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-depart-jour-ville-0.jpg?raw=true) |
| `depart-jour-ville-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/avant-depart-jour-ville-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-depart-jour-ville-1.jpg?raw=true) |
| `depart-jour-ville-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/avant-depart-jour-ville-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-depart-jour-ville-d3.jpg?raw=true) |
| `depart-nuit-plage-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/avant-depart-nuit-plage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-depart-nuit-plage-0.jpg?raw=true) |
| `depart-nuit-plage-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/avant-depart-nuit-plage-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-depart-nuit-plage-d3.jpg?raw=true) |
| `depart-nuit-ville-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/avant-depart-nuit-ville-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-depart-nuit-ville-0.jpg?raw=true) |
| `depart-nuit-ville-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/avant-depart-nuit-ville-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-depart-nuit-ville-d3.jpg?raw=true) |
| `portique-checkpoint-plage-nuit-feux3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-portique-checkpoint-plage-nuit-feux3.jpg?raw=true) |
| `portique-checkpoint-plage-nuit-go` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-portique-checkpoint-plage-nuit-go.jpg?raw=true) |
| `portique-checkpoint-ville-jour-feux3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-portique-checkpoint-ville-jour-feux3.jpg?raw=true) |
| `portique-checkpoint-ville-jour-feux5` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-portique-checkpoint-ville-jour-feux5.jpg?raw=true) |
| `portique-checkpoint-ville-jour-go` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-portique-checkpoint-ville-jour-go.jpg?raw=true) |
| `portique-course5-plage-jour-feux3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-portique-course5-plage-jour-feux3.jpg?raw=true) |
| `portique-course5-proche-ville-jour-feux5` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-portique-course5-proche-ville-jour-feux5.jpg?raw=true) |
| `portique-course5-proche-ville-nuit-feux3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-portique-course5-proche-ville-nuit-feux3.jpg?raw=true) |
| `portique-course5-ville-jour-feux5` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-portique-course5-ville-jour-feux5.jpg?raw=true) |
| `portique-course5-ville-nuit-feux3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-portique-course5-ville-nuit-feux3.jpg?raw=true) |
| `portique-course5-ville-nuit-go` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/apres-portique-course5-ville-nuit-go.jpg?raw=true) |

## Zooms (pour l'agent 3D, banc `portique-depart` en 2400 × 1080, agrandis)

| Zoom | Image |
|---|---|
| caisson et 5 colonnes de feux (3 allumées), jour, ×4 | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/zoom4-feux-jour.jpg?raw=true) |
| caisson et feux (5 allumés), nuit, ×4 | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/zoom4-feux-nuit.jpg?raw=true) |
| pied gauche en treillis et platine, jour, ×2 | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/zoom2-pied-gauche-jour.jpg?raw=true) |
| portique entier vu de la grille (Course à 5), jour, ×3 | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-559/qa/pr/WFS-559/zoom3-portique-course5-jour.jpg?raw=true) |
