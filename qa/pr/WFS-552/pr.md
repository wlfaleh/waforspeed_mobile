Agent : Claude, agent dev3d (01Us6uCwZ7)

## Demande de l'utilisateur
R19, maquette 01 : derrière le menu, la voiture du joueur en gros plan dans le parking souterrain ; exemple : Cavallo SF-X caméléon orange de face / bleu sur les côtés, skin bleu ciel, caméra rapprochée (WFS-552, épic E23, `regles-de-gestion/WFS-552.md`).

## Ce qui change
- **Parking souterrain** (`src/ui/menu_parking.{h,cpp}`, RG-01, RG-02) : aucun modèle libre de parking aux vraies mesures n'est atteignable par les sources permises (GitHub public ; Sketchfab réservé aux voitures, `regles-reseau.md`). Tout est donc **modélisé par le code au niveau du vrai objet** avec le kit du mobilier (WFS-369 : pavés chanfreinés, tubes, pièces tournées), aux mesures d'un parc réel. Aucune boîte, aucun disque ni cylindre nus :
  - poteaux 50 × 50 cm, angles chanfreinés de 2 cm, bande de danger jaune / noire peinte (bandes de 10 cm, de 0,30 à 1,50 m) ;
  - poutres retombées 45 × 40 cm (2,20 m libres), prédalles de 2,40 m à joints en V, voiles banchés en panneaux de 2,50 m ;
  - dalle sciée en panneaux de 5 m (époxy gris), bordures chasse-roues au nez jaune, marquage blanc de 12 cm, butées de roue 550 × 150 × 100 mm à bandes jaunes ;
  - réglettes LED IP65 de 1 500 mm (boîtier, diffuseur opale bombé, embouts, clips inox) ;
  - sprinklers : collecteur DN 80 et antennes DN 25 rouges RAL 3000, suspentes M10 avec colliers, têtes pendantes en laiton à déflecteur denté.
  
  Sources : NF P 91-100 (place 2,50 × 5,00 m, allée 6 m, hauteur libre ≥ 2,20 m), EN 12464-1 (75 lux dans l'allée, 50 lux sur les places), NF EN 12845 (sprinklers), DA-realisme § 3 (albédos). 37 692 triangles (butées profilées comprises), 11 appels de dessin, tout construit au chargement (RG-05). Si la mémoire graphique manque, le menu garde le fond d'avant, sans planter (`essayerMesh`).
- **Scène du menu** (`src/ui/ecrans/menu.cpp`) : le menu prend la scène dédiée. La voiture du joueur (sa vraie peinture, sa finition et sa skin, dessinées par Game) est garée en marche arrière dans le parking. La lumière est la même de jour et de nuit (RG-06), et le fond se fige après 1 s comme avant (WFS-329).
- **Caméra rapprochée** (`src/ui/menu_parking_cadrage.*`, RG-04) : trois quarts avant gauche, caméra horizontale à 1 m. La distance est cherchée pour que la voiture ait la taille de la maquette (49 % de la hauteur en 2400 × 1080), sans dépasser la largeur permise. L'objectif est décentré pour placer la voiture à droite de la console, sans pencher la caméra : les poteaux restent verticaux.
- **Caméléon** (`shaders/vehicule.glsl`, RG-03) : la peinture passe à deux teintes, la couleur choisie de face et sa teinte opposée (orange → bleu) sur les panneaux verticaux vus de biais. Le dessus (capot) garde la couleur de face, comme sur la maquette. Le mélange se fait sans passer par les couleurs intermédiaires, donc sans arc-en-ciel sur les galbes.
- **Skin Bleu ciel** (catalogue E18, RG-03) : 5e skin, identifiant `ciel` (« bleuciel » est déjà l'identifiant d'une couleur), 500 ★. Elle a deux bandes de capot, une bande de bas de caisse et la prise d'air du flanc en bleu ciel, avec des liserés blancs de 12 mm (`tools/skins/generer.py`). Elle va sur les 7 voitures à skins, et la SF-X reçoit aussi les 4 skins d'avant : 11 textures EAC de 1,33 Mo. Le code est mis à jour en conséquence : `configgame.*`, `vehicule_skin*`, l'ordre du garage dans `garage_skin.h` et les textes FR / EN.
- **Accroches hors zone, annoncées au journal** :
  - `src/core/system.h` (1 ligne) : `SceneDediee::interface` ;
  - `src/game/game.cpp` (5 lignes) : fond du menu figé aussi, `sceneDediee` faux pendant `drawUi`, et suivi du fond figé remis à zéro à chaque partie et à chaque voiture ;
  - `src/platform/desktop_main.cpp` (2 lignes) : banc `menu3d`.

## Vérification
- Compilation sans erreur. Tests CPU : nouveau `test_menu_parking` (toutes les voitures entières, à droite de la console, en gros plan, en 2400 × 1080, 1280 × 720 et 4:3). `test_vehicule_skin` contrôle les 11 nouvelles textures. `test_garage_skin` et `test_personnalisation` sont passés à 5 skins et 7 voitures.
- Banc `WFS_BANC=menu3d` (0 écart) :
  - SF-X de la maquette, gros plan sous 3 angles, décor seul, nuit, Golf du joueur ;
  - parcours menu → garage → menu → course → menu ;
  - repli si la mémoire graphique manque ;
  - 37 à 42 i/s en 1280 × 720 une fois le fond posé (rendu logiciel).

## Tests CPU
Commit testé : `25107961` (branche synchronisée avec `main`), `bash tools/qa/tests_cpu.sh build` ; journal complet : [`qa/pr/WFS-552/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/tests-cpu.log).
```
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 145 test(s) réussi(s) en 343 s
code de sortie : 0
```
Taille du décor (RG-05, journal du banc `menu3d`) : « menu (WFS-552) : parking souterrain préparé : 37692 triangles, 11 appels de dessin ».

## Revue du lead dev (refus sur eb05307b)
Corrigé dans 25107961 : nom « Bleu ciel » ajouté à `NOMS_SKINS` (`src/game/personnalisation.cpp`, contrôle dans `test_personnalisation`) ; voiture toujours à droite de la console (bord gauche à x NDC ≥ −0,26, seuil du test −0,28) ; journal des tests CPU joint.

## Limites et risques
- Le changement du caméléon vaut pour **toutes** les voitures caméléon (ex. Golf rouge : rouge de face, cyan sur les flancs vus de biais ; avant : rouge → jaune-vert). Un flanc vu bien en face reste de la couleur choisie (angle 3) : c'est l'angle de vue qui fait le bleu.
- Avis de l'agent 3D sur 5aa1b1b (À REPRENDRE) traité dans 7e6bef09 : la skin ne prend plus la rotation de teinte du caméléon (adhésif, métal 0,10) ; le caméléon garde sa luminosité d'une teinte à l'autre, métal 0,45 au lieu de 0,75 ; caméra 15 % plus près ; butées de roue profilées (trapèze arrondi, bandes jaunes dans le profil, tire-fond) ; halo des réglettes resserré. Reste mineur : plus de variation du béton du voile du fond (trous de banche), tâche séparée à proposer au PO si besoin.
- Les prises d'air jaunes du capot et la lame avant sont des pièces du modèle d'origine, pas de la peinture : elles ne prennent pas la skin.
- En 2400 × 1080, le rendu logiciel du PC fait 13 i/s même figé (passe finale de 2,6 Mpx sur le processeur, comme sur `main`) ; la mesure de cadence se lit en 1280 × 720.
- WFS-548 et WFS-549 passent sur le même écran : la console du menu de la maquette viendra avec WFS-549.


## Captures pour le lead dev (24 après)

Dossier : [`qa/pr/WFS-552`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `menu-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/avant-menu-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu-0.jpg?raw=true) |
| `menu-anim` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/avant-menu-anim.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu-anim.jpg?raw=true) |
| `menu-appui` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/avant-menu-appui.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu-appui.jpg?raw=true) |
| `menu-depart-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/avant-menu-depart-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu-depart-3.jpg?raw=true) |
| `menu-depart-go` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/avant-menu-depart-go.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu-depart-go.jpg?raw=true) |
| `menu-showroom` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/avant-menu-showroom.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu-showroom.jpg?raw=true) |
| `menu3d-1280x720-angle-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-1280x720-angle-1.jpg?raw=true) |
| `menu3d-1280x720-angle-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-1280x720-angle-2.jpg?raw=true) |
| `menu3d-1280x720-angle-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-1280x720-angle-3.jpg?raw=true) |
| `menu3d-1280x720-decor` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-1280x720-decor.jpg?raw=true) |
| `menu3d-1280x720-golf` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-1280x720-golf.jpg?raw=true) |
| `menu3d-1280x720-sfx` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-1280x720-sfx.jpg?raw=true) |
| `menu3d-2400x1080-angle-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-2400x1080-angle-1.jpg?raw=true) |
| `menu3d-2400x1080-angle-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-2400x1080-angle-2.jpg?raw=true) |
| `menu3d-2400x1080-angle-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-2400x1080-angle-3.jpg?raw=true) |
| `menu3d-2400x1080-decor` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-2400x1080-decor.jpg?raw=true) |
| `menu3d-2400x1080-golf` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-2400x1080-golf.jpg?raw=true) |
| `menu3d-2400x1080-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-2400x1080-nuit.jpg?raw=true) |
| `menu3d-2400x1080-parcours-course` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-2400x1080-parcours-course.jpg?raw=true) |
| `menu3d-2400x1080-parcours-fin` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-2400x1080-parcours-fin.jpg?raw=true) |
| `menu3d-2400x1080-parcours-garage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-2400x1080-parcours-garage.jpg?raw=true) |
| `menu3d-2400x1080-parcours-retour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-2400x1080-parcours-retour.jpg?raw=true) |
| `menu3d-2400x1080-repli` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-2400x1080-repli.jpg?raw=true) |
| `menu3d-2400x1080-sfx` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-552/qa/pr/WFS-552/apres-menu3d-2400x1080-sfx.jpg?raw=true) |
