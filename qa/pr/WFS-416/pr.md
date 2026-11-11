## Agent : Claude, agent dev3d (01UY1Vv4rv)

**Ticket** : WFS-416 — règles : `regles-de-gestion/WFS-416.md` (épics E14 + E15, P1 urgent). Statut après fusion : 🟡 À valider (utilisateur).

## Description

- **Demande de l'utilisateur** (2026-11-10) : cuire les bâtiments dans le paquet de la carte (3 niveaux de détail, fenêtres comprises) pour que le jeu ne construise plus rien pendant la course ; fenêtres en vrai 3D de près, en image avec relief au loin ; le kit construit par le code reste seulement en secours.
- **Ce qui change**
  - `src/world/archi_cuit.*` (nouveau, sans Vulkan) : la cuisson appelle le kit existant (`construire`, `baie`, `ouvertures::menuiserie` : rien n'est réécrit) pour les 625 bâtiments de la Ville, aux 3 niveaux, par morceau de 48 segments et par matériau. Niveau 0 : chaque modèle de baie (tableaux, menuiserie, volets, appui, balcon) est cuit une fois par immeuble puis référencé (2 583 modèles pour 19 347 baies, RG-05). Niveau 1 : la menuiserie de chaque baie est un rectangle au plan du dormant qui porte l'image de son modèle (31 tuiles de 128 × 256 px : couleur, découpe du vitrage, carte de normales tirée des profondeurs de la vraie menuiserie 3D) ; la pièce de nuit existante reste derrière la vitre. Niveau 2 : volume seul, comme avant. Bloc versionné, toutes les tailles et tous les indices vérifiés à la lecture, lu sans copie.
  - `src/world/cartes_v2/paquet_carte.cpp` : `cuire()` ajoute le bloc « batiments » au paquet de la Ville (mode `WFS_BANC=cuire` déjà appelé par `paquets.yml` ; **aucun fichier de `.github/**` modifié**). `src/core/carte_cuite.*` : accès partagé à un bloc du paquet actif (`blocDuPaquet`) ; **la version du format ne change pas** (un bloc inconnu est déjà ignoré).
  - `src/world/archi_carte.cpp` : paquet présent et bloc bon → au chargement, niveaux 1 et 2 de toute la Ville envoyés une fois à la carte graphique, 3 tampons réservés pour le niveau 0 ; en course, le jeu choisit le morceau et le niveau, et recopie le niveau 0 des morceaux proches (sommets déjà cuits, **au plus 24 000 par image**) dans ces tampons : aucune construction, aucun tampon créé. Menuiseries 3D jusqu'à 20 m, image avec relief jusqu'à 300 m. Paquet absent, bloc absent, refusé, ou mémoire insuffisante → kit de secours, comme aujourd'hui, avec une ligne au journal du jeu.
  - `src/world/archi_geo.*` : modes de baie pour la cuisson (`Baies::Reelles`, `Image`, `Aucune`), compteur `archi::constructions()`. **0 ligne de shader** (40 permises).
  - Accroches hors zone, annoncées au journal et acceptées par le PO : `ville.cpp` / `ville_quartiers.h` (les 12 lignes qui posent les immeubles, déplacées telles quelles dans `ville::poserImmeublesKit`, appelée aussi par la cuisson) ; `desktop_main.cpp` (1 ligne : banc) ; `CMakeLists.txt` (1 ligne : test).
  - À l'écran : de près, dormant, ouvrants, petits bois, appuis et garde-corps en volume ; au loin, cadres clairs en image ; de nuit, pièces allumées derrière les cadres. Sans paquet : image de `main`.
- **Comment c'est vérifié**
  - `bash tools/qa/tests_cpu.sh build` : **103 tests réussis** (branche synchronisée avec `main`, WFS-415 compris). Nouveau `tests/test_batiments_cuits.cpp` : deux cuissons identiques à l'octet ; niveau 0 recopié = mêmes triangles et même enveloppe que le bâtiment construit par le code ; lecture et recopie = 0 construction ; une image par baie au niveau 1 ; blocs tronqués, d'une autre version, d'un autre plan, à indice faux → refusés avec une raison ; paquet sans le bloc toujours bon.
  - Banc `WFS_BANC=batiments` : **0 écart** (cuisson déterministe, paquet absent → secours, paquet → 0 construction à 50 et 160 km/h, bloc refusé → secours sans plantage, chargement ≤ +3 s). Journal : `qa/pr/WFS-416/batiments.journal.txt`.
  - **Mesures (banc PC, rendu logiciel, Ville de nuit, Basse, 160 km/h, 240 images)**, temps du seul système des bâtiments par image, moyenne / pire : kit actuel 0,16 / 10,6 ms (58 constructions) ; kit + fenêtres de WFS-402 construites en course 0,20 / 18,3 ms (67 constructions) ; **paquet cuit 0,04 / 0,9 ms (0 construction)**. Image entière : médiane 306 ms (kit) et 291 ms (paquet), pire 357 et 390 ms : le rendu logiciel du banc noie l'écart. **Le gain est sur les à-coups, pas sur la moyenne.**
  - Repère sur le téléphone de l'utilisateur (APK 995, `main` après WFS-415, relayé par la session du lead dev) : Ville de nuit, Basse, 16 à 21 img/s, médiane 55 à 66 ms, scène GPU 43 à 55 ms : la lumière reste le goulot, hors de ce ticket.
- **Limites et risques connus**
  - **Aucune mesure sur téléphone** : le critère 3 (≥ 30 img/s, 99 % < 50 ms) reste à valider par l'utilisateur ; ce ticket retire les constructions en course, il ne traite pas le coût de la lumière.
  - **Poids** : paquet de la Ville 201 Mo (RGBA8), 206 Mo (ASTC), 189 Mo (ETC2), contre 28 à 46 Mo : **téléchargement 4 à 7 fois plus gros** pour le joueur (budget E15 : 200 à 700 Mo, accepté par le PO).
  - **Mémoire** : +80 Mo de mémoire graphique (907 → 987 Mo au banc : 48,3 Mo de niveaux fixes, 36,1 Mo de tampons) et 175 Mo de mémoire vive (le bloc reste chargé) : acceptés par le PO et par l'utilisateur (2026-11-11). Le repli « mémoire insuffisante → kit » est écrit mais n'a pas pu être provoqué au banc.
  - **Chargement** : vérification du paquet 0,8 s sur le PC du banc (CRC de 201 Mo), partie +0,2 à +0,4 s.
  - RG-03 lue par le PO comme « aucune construction, aucun tampon créé » : la recopie bornée de sommets cuits est permise.
  - Image avec relief : une tuile par modèle (style × type × modèle), étirée aux mesures de chaque baie (écart d'épaisseur des cadres de quelques millimètres à plus de 20 m). Les volets simplifiés du niveau 1 restent en volume.
  - Ville seulement (RG-09) ; Corniche et Campagne : tickets à créer par le PO.
  - Les captures n'ont pas de colonne « avant (main) » du script : le banc est nouveau ; il joue lui-même les deux états, « kit de secours » étant l'état de `main`.

## Après la fusion
- a. **Relancer « Paquets de cartes » (`paquets.yml`) sur `main`** : agent expert ci cd, après une CI de `main` verte. Ce n'est pas automatique.
- b. Tant que ce n'est pas fait, le téléphone garde l'ancien paquet, sans bloc « batiments » : le jeu écrit « paquet sans bâtiments cuits : kit de secours » et joue comme aujourd'hui.
- c. `VERSION_MIN_APK` (927) **ne monte pas** : un jeu plus ancien ignore le bloc inconnu et lit le même paquet ; `VERSION_FORMAT` et `VERSION_GENERATION_CARTE` inchangés.
- d. Cuisson complète des 3 formats (deux passes chacun) : **16 s** sur le PC du banc (limite du workflow : 40 min).

## Captures pour le lead dev et l'agent 3D (29, dossier [`qa/pr/WFS-416`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416))

| Capture demandée | Avant : kit construit par le code (état de `main`, = paquet absent) | Après : paquet cuit |
|---|---|---|
| 1. Ville de nuit, Basse, en course (rue d'immeubles, ligne de mesure) | ![secours-rue-nuit](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-secours-rue-nuit.jpg?raw=true) | ![paquet-rue-nuit](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-paquet-rue-nuit.jpg?raw=true) |
| 2. Même rue de jour | ![secours-rue-jour](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-secours-rue-jour.jpg?raw=true) | ![paquet-rue-jour](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-paquet-rue-jour.jpg?raw=true) |
| 3. Façade de près (6 à 12 m), jour : vraie menuiserie 3D | ![secours-facade-pres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-secours-facade-pres.jpg?raw=true) | ![paquet-facade-pres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-paquet-facade-pres.jpg?raw=true) |
| 3 bis. Façade de près, nuit | ![secours-facade-pres-nuit](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-secours-facade-pres-nuit.jpg?raw=true) | ![paquet-facade-pres-nuit](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-paquet-facade-pres-nuit.jpg?raw=true) |
| 4. Même façade à 50 m (vue resserrée), jour : image avec relief | ![secours-facade-50m](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-secours-facade-50m.jpg?raw=true) | ![paquet-facade-50m](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-paquet-facade-50m.jpg?raw=true) |
| 4 bis. Même façade à 50 m, nuit | ![secours-facade-50m-nuit](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-secours-facade-50m-nuit.jpg?raw=true) | ![paquet-facade-50m-nuit](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-paquet-facade-50m-nuit.jpg?raw=true) |
| 5. 160 km/h de nuit, dernière image (ligne : temps des bâtiments) | ![secours-160kmh](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-secours-160kmh.jpg?raw=true) | ![paquet-160kmh](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-paquet-160kmh.jpg?raw=true) |
| 5. Suite à 160 km/h, image 1 (temps de l'image) | ![secours-160kmh-suite-1](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-secours-160kmh-suite-1.jpg?raw=true) | ![paquet-160kmh-suite-1](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-paquet-160kmh-suite-1.jpg?raw=true) |
| 5. Suite à 160 km/h, image 2 | ![secours-160kmh-suite-2](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-secours-160kmh-suite-2.jpg?raw=true) | ![paquet-160kmh-suite-2](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-paquet-160kmh-suite-2.jpg?raw=true) |
| 5. Suite à 160 km/h, image 3 | ![secours-160kmh-suite-3](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-secours-160kmh-suite-3.jpg?raw=true) | ![paquet-160kmh-suite-3](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-paquet-160kmh-suite-3.jpg?raw=true) |

Capture 6 (paquet absent) : la colonne « Avant » ci-dessus (kit de secours). Bloc refusé → kit de secours : ![bloc-refuse](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-bloc-refuse.jpg?raw=true)

Repère, kit avec les fenêtres de WFS-402 construites en course (état d'avant WFS-415), 160 km/h : ![kit402-160kmh](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-416/qa/pr/WFS-416/apres-batiments-kit402-160kmh.jpg?raw=true)

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv4rvXu6si8FEZYAe1H
