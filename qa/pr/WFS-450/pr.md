Agent : Claude, lead dev (01UY1Vv5Nu)

**Ticket** : WFS-450 — règles : `regles-de-gestion/WFS-450.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : la nuit, en qualité Basse, la chaussée éclairée par les lampadaires ne doit plus devenir noire d'un coup puis se rallumer d'un coup en roulant (Corniche, vers 91-100 km/h), et les phares des autres voitures ne doivent plus apparaître d'un coup (Ville).
- **Cause trouvée au banc en mouvement (elle complète celle du ticket)** : le jeu ne gardait comme lumières que les 4 lampadaires situés « de 4 segments derrière le joueur à 160 devant ». Un lampadaire sortait donc de la liste **5 m derrière la voiture**, alors qu'il éclaire 26 m autour de lui : toute la chaussée autour de la voiture s'éteignait en une image, puis se rallumait à l'approche du lampadaire suivant. Les disques orange au sol (retirés en Basse par WFS-421) cachaient ce défaut. La cause écrite dans le ticket (quota de 4 lumières par pixel pris par les voitures de derrière) est réelle aussi et corrigée, mais c'est le choix des lampadaires qui donne les −45 % et −85 % mesurés.
- **Ce qui change** (Basse seulement ; Moyenne et Haute gardent le code d'avant) :
  - `src/render/nuit.cpp`, `src/render/phares.h` : en Basse, un lampadaire reste candidat jusqu'à 36 m derrière le joueur (portée 26 m + recul de la caméra) ; les 4 retenus sont les plus proches, la route devant gardant la priorité (un lampadaire derrière compte 3 fois sa distance) ; chaque entrée ou sortie de la liste se fait en **fondu de 0,3 s** (`FonduLampes`, 6 lumières au plus un court instant).
  - **RG-01** `src/render/renderer.cpp`, `renderer.h` (champ `rang` d'une lumière, accroche de 2 lignes) : en Basse, ordre phares du joueur, puis lampadaires, puis phares des autres voitures ; à rang égal, la plus proche de la caméra.
  - **RG-02, RG-04** `shaders/eclairage.glsl` : toujours 4 lumières par pixel en Basse, mais une lumière ne prend une place que si elle éclaire vraiment le pixel (place de 0 à 1 entre 0,6 % et 3 % de l'éclairement sous un lampadaire ; sous 0,1 %, elle n'est pas calculée), et la dernière lumière admise est pesée par la place qui reste : plus de bord net au bord du quota. Coupure à 80° du feu de croisement adoucie (0,17 → 0,30).
  - **RG-03** `nuit.cpp`, `phares.h` (`FonduAutres`) : les phares d'une autre voiture montent et descendent en 0,3 s (changement de rang, avec un écart de 6 m pour ne pas clignoter) et avec la distance (pleins à 55 m, nuls à 70 m) ; la lumière au sol calculée leur rend la main en fondu croisé.
  - **RG-06** nouveau banc `src/render/lumieres_mouvement_banc.cpp` (`WFS_BANC=lumieres-mouvement`) ; accroches annoncées au journal : `src/game/course5.h` et `course5_system.cpp` (+13 lignes : adversaires tenus derrière le joueur pour le banc), `src/platform/desktop_main.cpp` (+2).
  - **RG-07** `tests/test_phares.cpp` : ordre des lumières, fondu des phares (montée ≈ 0,3 s, saut ≤ 20 % par image à 60 et 30 img/s, changement de rang sans aller-retour, 55-70 m), choix et fondu des lampadaires.
- **Comment c'est vérifié** : banc en mouvement, 2 scènes × 45 images à 10 img/s, avant / après sur le même trajet (tableau ci-dessous) : 0 écart ; 110 tests CPU verts (`bash tools/qa/tests_cpu.sh build`, branche synchronisée avec `main`) ; images ouvertes une à une.
- **Limites et risques connus** :
  - **RG-05 (pas de perte d'images sur le S21 FE) n'est pas mesuré** : je n'ai pas de téléphone. Le shader fait le même travail par lumière ; en Basse, il peut y avoir un court instant 6 lampadaires au lieu de 4 dans la liste (le quota par pixel reste 4). À mesurer par l'utilisateur avec le banc de WFS-420 (ticket 🟡).
  - Le même retrait du lampadaire 5 m derrière la voiture existe en **Moyenne et en Haute** (code d'avant gardé, comme le demande RG-01) : à décider par le PO.
  - La suite « avant » est rejouée par un interrupteur du banc (`phares::avant450`, comme `Game::interpolation` pour le banc « fluidite »), pas par une construction de `main` : le banc n'existe pas sur `main`.
  - En Ville, la lumière monte de 24 % en une image (59,6 → 73,8) quand la voiture voisine arrive à 10 m (fondu de 0,3 s = 3 images à 10 img/s) : c'est une montée, pas une chute.

## Mesures (RG-06)

| Scène | Plus forte chute d'une image à la suivante, avant | après | RG-06 |
|---|---|---|---|
| Corniche, 85 → 105 km/h, 2 adversaires derrière | **45 %** (86 → 49) | **9 %** | ≤ 20 % : tenu |
| Ville, ≈ 90 km/h, voiture voisine | **85 %** (54 → 9) | **17 %** | ≤ 20 % : tenu |

Mesure : luminosité moyenne (luma des pixels, 0 à 255) du rectangle « chaussée autour de la voiture », sur chaque image ; tableaux complets : [`mouv-corniche.txt`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/mouv-corniche.txt), [`mouv-ville.txt`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/mouv-ville.txt).

## Captures pour le lead dev et l'agent 3D

Dossier : [`qa/pr/WFS-450`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450).

**Avant en haut, après en bas, une image sur trois**

Corniche : ![compare-mouv-corniche.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/compare-mouv-corniche.jpg?raw=true)

Ville : ![compare-mouv-ville.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/compare-mouv-ville.jpg?raw=true)

| Capture | Avant (même trajet, `phares::avant450`) | Après (branche) |
|---|---|---|
| `mouv-corniche-planche`<br>Corniche, 45 images à 10 img/s (4,5 s), 85 → 105 km/h, 2 adversaires collés derrière | ![avant-mouv-corniche-planche.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-corniche-planche.jpg?raw=true) | ![apres-mouv-corniche-planche.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-corniche-planche.jpg?raw=true) |
| `mouv-corniche-04`<br>Corniche, image 04 : juste avant la chute | ![avant-mouv-corniche-04.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-corniche-04.jpg?raw=true) | ![apres-mouv-corniche-04.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-corniche-04.jpg?raw=true) |
| `mouv-corniche-05`<br>Corniche, image 05 (0,1 s plus tard) : avant, la chaussée est noire ; après, elle reste éclairée | ![avant-mouv-corniche-05.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-corniche-05.jpg?raw=true) | ![apres-mouv-corniche-05.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-corniche-05.jpg?raw=true) |
| `mouv-corniche-07`<br>Corniche, image 07 | ![avant-mouv-corniche-07.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-corniche-07.jpg?raw=true) | ![apres-mouv-corniche-07.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-corniche-07.jpg?raw=true) |
| `mouv-ville-planche`<br>Ville, 45 images à 10 img/s, ≈ 90 km/h, une voiture remonte sur la file voisine (85 m → 4 m) | ![avant-mouv-ville-planche.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-ville-planche.jpg?raw=true) | ![apres-mouv-ville-planche.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-ville-planche.jpg?raw=true) |
| `mouv-ville-08`<br>Ville, image 08 : juste avant la chute | ![avant-mouv-ville-08.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-ville-08.jpg?raw=true) | ![apres-mouv-ville-08.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-ville-08.jpg?raw=true) |
| `mouv-ville-09`<br>Ville, image 09 (0,1 s plus tard) : avant, rue noire ; après, rue éclairée | ![avant-mouv-ville-09.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-ville-09.jpg?raw=true) | ![apres-mouv-ville-09.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-ville-09.jpg?raw=true) |
| `mouv-ville-11`<br>Ville, image 11 | ![avant-mouv-ville-11.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-ville-11.jpg?raw=true) | ![apres-mouv-ville-11.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-ville-11.jpg?raw=true) |
| `mouv-ville-42`<br>Ville, image 42 : la voiture voisine arrive, ses phares montent en fondu | ![avant-mouv-ville-42.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-ville-42.jpg?raw=true) | ![apres-mouv-ville-42.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-ville-42.jpg?raw=true) |
| `mouv-ville-44`<br>Ville, image 44 : avant, la rue s'éteint encore ; après, non | ![avant-mouv-ville-44.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-ville-44.jpg?raw=true) | ![apres-mouv-ville-44.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-ville-44.jpg?raw=true) |

