Agent : Claude, agent dev N (01UY1Vv5K1)

**Ticket** : WFS-382 — règles : `regles-de-gestion/WFS-382.md` (ANO-222, ANO-228) · **2e version**, après le refus du lead dev (revue du commit 3c89a192)

## Description

- **Demande de l'utilisateur** : la nuit, les phares (police, voitures en face) font des cônes blancs énormes et brûlés qui mangent l'image ; il faut des phares qui ressemblent aux vrais.
- **Causes trouvées au banc** :
  1. l'ancienne « tache des phares » du jeu web (grille de 110 × 26 m en mélange multiplicatif, dessinée **deux fois** en Campagne) s'ajoutait aux deux vrais feux de croisement de WFS-218 : c'est le cône blanc devant la voiture (Campagne, neige) ;
  2. répartition verticale du feu de croisement trop forte vers le bas : sol devant les optiques ≈ 30 fois plus éclairé qu'à 30 m, crête trop basse (21 000 cd) ;
  3. reflet des feux de croisement sur la chaussée borné à 4 (comme le soleil) : vu de face, deux taches presque blanches au sol ;
  4. lueur de chaque phare d'en face = disque de 60 cm (forme interdite) ;
  5. halo et bloom de la passe finale sans limite la nuit (sur le téléphone : grosse tache blanche autour de la police) ;
  6. chaussée en Lambert pur : pour le conducteur, la zone de 30 à 60 m restait sombre (défaut 2 du lead dev).
- **Ce qui change** :
  - `src/render/nuit.cpp` : tache du jeu web retirée (RG-04 : seuls les deux feux réels éclairent, rien de recréé ; un appel de dessin en moins) ; marque des deux feux du joueur (pénombre 1) quand la caméra est derrière leurs optiques ; ligne `fx.stop / fx.recul / fx.frein` du joueur recollée (elle était au bout d'un commentaire, donc jamais exécutée : feux stop et recul du joueur, WFS-109) ;
  - `src/render/phares.h` + `shaders/eclairage.glsl` : plateau jusqu'à −1,5°, puis chute en 1 / (1 + t^1,6), t = (−el − 1,5) / 0,85, identique des deux côtés ; **flux sourcé inchangé** (LED 3 000 lm, halogène 1 500 lm) → crête LED 30 000 cd (réel 30 000 à 45 000 cd, R112 / R149, avis `expert-realisme-3d`) ; reflet des seuls feux de croisement borné à 0,8 ;
  - `shaders/eclairage.glsl`, **rétroréflexion de l'asphalte** (`retroChaussee`, accord du PO du 2026-11-10, `regles-de-gestion/WFS-382.md` § Suivi, ≈ 15 lignes au total dans ce fichier) : k = 0,12 / sin α, de 1 à 7 (EN 1436, R_L), seulement pour les feux du joueur avec la caméra derrière eux (pas en vue de face, pas pour les autres voitures), sur un sol horizontal et sombre ; il s'efface sur la neige et les surfaces claires (RG-07). Le chemin Basse à 2 lumières par pixel (WFS-353) l'applique aussi. La marque passe par `spotCone.y` (pénombre), parce que `spotCone.w` est déjà écrit à 0 par `renderer.cpp` et que le PO interdit de toucher au renderer et à `frame_ubo` ; la pénombre n'est pas lue pour un feu de croisement ;
  - `src/render/renderer.cpp` (accroche, 2 lignes) : lueur des phares 0,30 m → 0,12 m, intensité 0,35 → 0,12 (vu au-dessus de la coupure, un feu de croisement n'envoie que 350 à 625 cd, R112) ;
  - `shaders/post.frag` (accroche E13) : **correction du défaut 1** : la nuit, halo et bloom compressés en douceur sur leur luminance, e / (1 + Y / 0,35), au lieu du `min()` canal par canal (fin du disque gris, du carré et de l'anneau irisé) ; les 16 prises du halo local sont pondérées en gaussienne (halo rond) ; **le jour, rien ne change** (poids égaux, aucune compression) ;
  - banc `WFS_BANC=phares` (`bench_nuit.*`, accroche d'1 ligne dans `desktop_main.cpp`) : captures 1 à 4, police visible derrière le joueur, vue haute de la coupure, position à l'écran de la chaussée à 10, 30 et 50 m, deux témoins de jour ;
  - `tests/test_phares.cpp` : intensité à −3°, −5°, −10°, −15°, rapport sol à 5 m / 30 m (6 à 15), crête LED.
- **Comment c'est vérifié** :
  - `bash tools/qa/tests_cpu.sh build` : **96 tests réussis** ; `test_phares` : paire LED 5 m 12,6 lx, 30 m 1,44 lx, 50 m 0,29 lx, portée 65 m, œil d'en face à 50 m 0,18 lx (≤ 0,4 lx B50L) ;
  - pixels saturés (≥ 250 sur les 3 canaux), image entière, avant → après : poursuite police 0,82 → **0 %**, police Basse 0,99 → **0 %**, poursuite face 0,35 → 0 %, Basse face 0,60 → 0 %, Ville face sous la pluie 0,80 → 0 % (RG-03 : ≤ 10 %) ; **toutes les captures « après » à 0 %** ;
  - **RG-01, vue Capot en Campagne** (clarté moyenne 0-255 de la chaussée au milieu de la voie, points projetés par le banc) : main 10 m **225** (blanc de l'ancienne tache) / 30 m 123 / 50 m 96 → branche 10 m **130** / 30 m **112** / 50 m **72** : premier plan non brûlé, tache texturée jusqu'à 40-50 m, puis la coupure (≈ 65 m, h / tan 0,57°) ; vue haute au même endroit ;
  - neige de nuit, tache des phares : 7,56 → **0 %** saturés (RG-07 : ≤ 2 %) ; grain et traces de roues visibles ;
  - témoins de jour : **0 pixel changé** ;
  - RG-05, banc `perf` (`WFS_PERF_SCENE=1`, Ville de nuit sous la pluie, **Basse**), 4 mesures alternées (main, branche, main, branche) : départ 243,8 / 245,4 / 252,3 / 264,1 ms, Mimosas 155,6 / 160,9 / 167,3 / 171,0 ms. La machine ralentit à chaque mesure (+4 à +6 ms, visible entre les deux mesures de `main`). Une fois cette dérive retirée, l'écart dû au ticket est d'environ **+1 % au départ et −0,8 % aux Mimosas : coût égal, aux erreurs de mesure près** ; 1 appel de dessin en moins ;
  - RG-02 : aucun cône dans l'air, ni au sec ni sous la pluie ; RG-06 : l'exposition du départ de nuit n'est pas touchée.
- **Limites et risques connus** :
  - la compression de l'éclat ne joue que la nuit (`params.w ≠ 0`), pas à l'heure bleue ni au crépuscule ;
  - le banc (lavapipe) coupe la chaîne du bloom (`gpuLogiciel`) : la compression du bloom ne se voit que sur le téléphone (Moyenne / Haute) ; celle du halo local se voit au banc (zooms) ;
  - la mesure de perf a été faite avant la pondération gaussienne du halo (16 poids constants par pixel dans la passe finale, la nuit), dont le coût est négligeable ;
  - sur les zooms des lampadaires, le petit rectangle clair qui reste est la **glace lumineuse du modèle de la tête de lampe** (`ville.cpp`, 0,90 × 0,36 m), jusqu'ici noyée sous l'éblouissement ; ce n'est pas un halo. Le modèle relève de l'épic E15 (Ville) / WFS-378 ;
  - vue de face, sous la pluie, le reflet allongé des phares sur la chaussée mouillée reste visible : c'est le reflet réel d'une route mouillée, borné et non saturé ;
  - « avant » = `origin/main` (8fda3f17) + le seul banc `phares` (il n'existe pas sur `main`), compilé à part ; captures `neige-*` « avant » : `main` du 2026-11-09 23:40 UTC, sans aucun changement de ce ticket ;
  - non fait : cœur du phare en vraie glace modélisée (le cœur reste un point de 9 cm) ;
  - après la fusion : **🟡 À valider (utilisateur)**.

## Captures pour le lead dev

Dossier : [`qa/pr/WFS-382`](https://github.com/wlfaleh/waforspeed_mobile/tree/feature/waforspeed-382/qa/pr/WFS-382). Nuit et jour ; captures 1 à 4 et 6 des règles ; capture 5 (perf) : chiffres ci-dessus.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| 1. Poursuite de nuit, police derrière le joueur, vue de biais (phares de la police vers la caméra), Haute | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-poursuite-police.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-poursuite-police.jpg?raw=true) |
| 1. Même scène en qualité Basse | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-poursuite-basse-police.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-poursuite-basse-police.jpg?raw=true) |
| 1. Poursuite, vue de face du joueur, Haute | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-poursuite-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-poursuite-face.jpg?raw=true) |
| 1. Même scène en qualité Basse | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-poursuite-basse-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-poursuite-basse-face.jpg?raw=true) |
| 1. Poursuite, vue Proche (jeu) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-poursuite-proche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-poursuite-proche.jpg?raw=true) |
| 2. Voiture vue de face, Ville, chaussée sèche | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-ville-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-ville-face.jpg?raw=true) |
| 3. Vue Capot, Campagne (autoroute, aucun lampadaire) : tache de 30 à 60 m | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-campagne-capot.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-campagne-capot.jpg?raw=true) |
| 3. Même endroit, vue haute : forme de la tache et bord de la coupure | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-campagne-capot-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-campagne-capot-haut.jpg?raw=true) |
| 4. Vue Capot sous la pluie | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-campagne-capot-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-campagne-capot-pluie.jpg?raw=true) |
| 4. Même endroit sous la pluie, vue haute | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-campagne-capot-pluie-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-campagne-capot-pluie-haut.jpg?raw=true) |
| 4. Voiture vue de face, Ville, sous la pluie | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-ville-face-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-ville-face-pluie.jpg?raw=true) |
| 6. Neige de nuit, vue Arrière (banc neige) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-neige-nuit-arriere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-neige-nuit-arriere.jpg?raw=true) |
| 6. Neige de nuit, vue Cockpit (banc neige) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-neige-cockpit-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-neige-cockpit-nuit.jpg?raw=true) |
| Témoin de jour, vue Proche (identique) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-ville-jour-proche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-ville-jour-proche.jpg?raw=true) |
| Témoin de jour, vue de face (identique) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-ville-jour-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-ville-jour-face.jpg?raw=true) |

**Zooms ×3 demandés par le lead dev** (de gauche à droite : `main` · 1re version de la PR, `min()` par canal · correction) :

| Zoom | Image |
|---|---|
| Lampadaire (478, 68) de la poursuite, Haute | ![zoom](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/zoom-phares-poursuite-face.jpg?raw=true) |
| Même lampadaire, Basse | ![zoom](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/zoom-phares-poursuite-basse-face.jpg?raw=true) |
| Lampadaire (478, 62), Ville au sec | ![zoom](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/zoom-phares-ville-face.jpg?raw=true) |
| Même lampadaire, sous la pluie | ![zoom](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/zoom-phares-ville-face-pluie.jpg?raw=true) |
| Écran du Cockpit, neige de nuit (gauche `main`, droite correction) | ![zoom](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/zoom-neige-cockpit-nuit.jpg?raw=true) |
