Agent : Claude, agent dev N (01UY1Vv5K1)

**Ticket** : WFS-382 — règles : `regles-de-gestion/WFS-382.md` (ANO-222, ANO-228)

## Description

- **Demande de l'utilisateur** : la nuit, les phares (police, voitures en face) font des cônes blancs énormes et brûlés qui mangent l'image ; il faut des phares qui ressemblent aux vrais.
- **Causes trouvées au banc** :
  1. l'ancienne « tache des phares » du jeu web (grille de 110 × 26 m en mélange multiplicatif, dessinée **deux fois** en Campagne) s'ajoutait aux deux vrais feux de croisement de WFS-218 : c'est le cône blanc devant la voiture (Campagne, neige) ;
  2. répartition verticale du feu de croisement trop forte vers le bas : sol devant les optiques ≈ 30 fois plus éclairé qu'à 30 m, crête trop basse (21 000 cd) ;
  3. reflet spéculaire des feux de croisement sur la chaussée borné à 4 (comme le soleil) : vu de face, deux taches presque blanches au sol ;
  4. lueur de chaque phare d'en face = disque de 60 cm (forme interdite) ;
  5. bloom et halo de la passe finale non plafonnés la nuit (sur le téléphone : grosse tache blanche autour de la police).
- **Ce qui change** :
  - `src/render/nuit.cpp` : tache du jeu web retirée (RG-04 : seuls les deux feux réels éclairent, rien de recréé) ; un appel de dessin en moins ;
  - `src/render/phares.h` + `shaders/eclairage.glsl` (accroche de 7 lignes : `feuCroisement`, `feuCroisementRapide`, plafond du reflet) : plateau jusqu'à −1,5°, puis chute en 1 / (1 + t^1,6), t = (−el − 1,5) / 0,85 ; **flux sourcé inchangé** (LED 3 000 lm, halogène 1 500 lm), donc crête LED 30 000 cd (fourchette réelle 30 000 à 45 000 cd, R112 / R149, avis `expert-realisme-3d` du 2026-11-10) ; reflet des seuls feux de croisement borné à 0,8 (soleil et lampadaires inchangés) ;
  - `src/render/renderer.cpp` (accroche, 2 lignes) : lueur des phares 0,30 m → 0,12 m (≈ 2 fois la glace), intensité 0,35 → 0,12 (5 % du cœur) : vu au-dessus de la coupure, un feu de croisement n'envoie que 350 à 625 cd (R112, B50L et zone III) ;
  - `shaders/post.frag` (accroche E13, 5 lignes) : la nuit seulement, halo et bloom plafonnés à +0,35 avant ACES ; le jour, rien ne change ;
  - `src/render/nuit.cpp` : la ligne `fx.stop / fx.recul / fx.frein` du joueur était collée au bout d'un commentaire (jamais exécutée) : recollée (feux stop et recul du joueur, WFS-109) ;
  - banc `WFS_BANC=phares` (`bench_nuit.*`, accroche d'1 ligne dans `desktop_main.cpp`) : les captures 1 à 4 du ticket et deux témoins de jour, en 5 min au lieu de plus de 10 pour le banc `nuit` complet ;
  - `tests/test_phares.cpp` : intensité à −3°, −5°, −10°, −15°, rapport sol à 5 m / 30 m (6 à 15), crête LED 28 000 à 45 000 cd.
- **Comment c'est vérifié** :
  - `bash tools/qa/tests_cpu.sh build` : 95 tests réussis ; `test_phares` : paire LED 5 m 12,6 lx, 30 m 1,44 lx, 50 m 0,29 lx (rapport 8,7 ; réel ≈ 12), portée 65 m, œil d'en face à 50 m 0,18 lx (≤ 0,4 lx B50L) ;
  - pixels saturés (≥ 250 sur les 3 canaux), image entière, avant → après : poursuite face 0,37 → 0 %, poursuite Basse face 0,62 → 0 %, Ville face sous la pluie 0,80 → 0 % (RG-03 : ≤ 10 %) ; **neige de nuit, tache des phares : 7,56 → 0 %** (RG-07 : ≤ 2 %), grain et traces de roues visibles ;
  - témoins de jour : **0 % de pixels changés** (identiques) ;
  - RG-05, banc `perf` (`WFS_PERF_SCENE=1`, Ville de nuit sous la pluie, **Basse**), 2 mesures chacun, moyenne GPU : départ 176,4 → 179,1 ms (+1,6 %), Mimosas 112,4 → 111,2 ms (−1,1 %) ; 260 → 259 appels ; deux mesures du même code varient de ±3 % : **coût égal, aux erreurs de mesure près** ;
  - RG-02 : aucun cône visible dans l'air, ni par temps sec ni sous la pluie (aucun voile ajouté) ; RG-06 : l'exposition du départ de nuit n'est pas touchée (aucune modification de `eclairage_exposition` ni de WFS-356).
- **Limites et risques connus** :
  - le banc (lavapipe) coupe la chaîne du bloom (`gpuLogiciel`) : le plafond du bloom de `post.frag` ne se voit pas sur les captures, seulement sur le téléphone (Moyenne / Haute) ; le plafond du halo local, lui, est actif au banc ;
  - vue de face, le sol juste devant les optiques reste clair (≈ 28 lx à 3 m, valeur réelle 30 à 45 lx), mais n'est plus saturé ;
  - non fait, proposé à l'expert / au PO pour plus tard : rétroréflexion de l'asphalte pour le conducteur (zone 30 à 60 m plus lisible), cœur du phare en vraie glace modélisée (le cœur reste un point de 9 cm) ;
  - « avant » = `origin/main` (5e9f9f95) + le seul banc `phares` (il n'existe pas sur `main`), compilé à part : `tools/qa/captures_pr.sh` ne peut pas lancer sur `main` un banc ajouté par la branche ;
  - ticket de lumière vue sur téléphone : **🟡 À valider (utilisateur)** après la fusion.

## Captures pour le lead dev (11 après, 11 avant)

Dossier : [`qa/pr/WFS-382`](https://github.com/wlfaleh/waforspeed_mobile/tree/feature/waforspeed-382/qa/pr/WFS-382). Nuit et jour ; captures 1 à 4 et 6 des règles du ticket ; capture 5 (perf) : chiffres ci-dessus.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| 1. Poursuite de nuit, Ville, vue de face (police derrière le joueur), Haute | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-poursuite-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-poursuite-face.jpg?raw=true) |
| 1. Même scène en qualité Basse | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-poursuite-basse-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-poursuite-basse-face.jpg?raw=true) |
| 1. Poursuite de nuit, vue Proche (jeu) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-poursuite-proche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-poursuite-proche.jpg?raw=true) |
| 2. Voiture vue de face, Ville, chaussée sèche | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-ville-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-ville-face.jpg?raw=true) |
| 3. Vue Capot, Campagne (autoroute, aucun lampadaire) : tache au sol et coupure | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-campagne-capot.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-campagne-capot.jpg?raw=true) |
| 4. Même scène sous la pluie | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-campagne-capot-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-campagne-capot-pluie.jpg?raw=true) |
| 4. Voiture vue de face, Ville, sous la pluie | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-ville-face-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-ville-face-pluie.jpg?raw=true) |
| 6. Neige de nuit, vue Arrière (banc neige) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-neige-nuit-arriere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-neige-nuit-arriere.jpg?raw=true) |
| 6. Neige de nuit, vue Cockpit (banc neige) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-neige-cockpit-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-neige-cockpit-nuit.jpg?raw=true) |
| Témoin de jour, vue Proche (rien ne doit changer) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-ville-jour-proche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-ville-jour-proche.jpg?raw=true) |
| Témoin de jour, vue de face (rien ne doit changer) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/avant-phares-ville-jour-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-382/qa/pr/WFS-382/apres-phares-ville-jour-face.jpg?raw=true) |
