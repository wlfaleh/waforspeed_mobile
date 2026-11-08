## Agent : Claude, agent dev conduite

**Ticket** : WFS-258 (P1, ANO-131 ; ANO-138), **point 3 de RG-00b** : « en conduite normale, la direction ne doit pas être trop rapide : sensibilité qui baisse avec la vitesse, comme une vraie voiture » — règles : `regles-de-gestion/WFS-258.md` (accord écrit du PO dans son Suivi : seulement au-dessus de 100 km/h, fonction séparée, rien sous 100 km/h, zone de WFS-316). Points 1 (#263) et RG-00 (#217) déjà fusionnés.

## Description

- **Demande de l'utilisateur** : « quand la voiture roule normalement, le changement de direction est trop rapide selon la vitesse ».
- **Mesure sur `main`** (conduite du jeu, aide désactivée, volant tenu 0,5 s, ligne droite) : dès 100 km/h, **un quart de volant demande presque toute l'adhérence** (Golf : 1,7 g à 100 km/h ; **2,1 g dès 150 km/h, autant que le volant à fond**) : la moindre touche fait virer la voiture au maximum ; à 200 km/h dans la partie, un quart de volant 0,5 s envoie la voiture 11 m sur le côté contre le bord (167 km/h après le choc).
- **Ce qui change** :
  - `src/game/player.cpp` : nouvelle fonction **`reductionBraquageHauteVitesse(v, empattement)`**, appelée en une ligne (calcul de l'angle des roues) : au-dessus de 100 km/h, l'angle des roues est ramené pour que **le volant à fond demande 1,2 × l'adhérence du jeu** (`Phys::grip`, RG-06) et **un quart de volant à peu près un quart** ; réduction **progressive de 100 à 130 km/h** (aucun à-coup). **Rien ne change sous 100 km/h** (zone de WFS-316). `maxSteer`, `steerV`, `grip` inchangés ; l'ordinateur (`Aide::Web`, règles du jeu web) inchangé.
  - Banc `virages` : sections 8 (vue de dessus, ligne droite de la Ville, 150 / 190 / 230 km/h) et 9 (dans la partie, Grand Prix, Gallardo à 200 km/h, quart de volant 0,5 s) ; `WFS_VIRAGES=vif` pour ne lancer que ces sections ; interrupteur `g_avantWfs258Vif` (banc seulement, faux dans le jeu) pour rejouer `main`, vérifié par le test.
- **Comment c'est vérifié** :
  - `test_direction_vitesse` (nouveau, Golf et Gallardo) : **sous 100 km/h, identique à `main`** (50 et 90 km/h, au bit près) ; **au-dessus de 130 km/h, un quart de volant ≤ 40 % du volant à fond** (Golf 0,63 g pour 2,11 g ; `main` : 2,11 g pour 2,11 g) ; **volant à fond inchangé** (toute l'adhérence du jeu) ; **aucun à-coup** de 98 à 140 km/h (0,115 g au plus par palier de 2 km/h).
  - Ligne droite de la Ville, quart de volant 0,5 s : décalage **0,74 m** à 150, 190 et 230 km/h (`main` : 2,55 à 2,65 m). Dans la partie à 200 km/h : **4 m de changement de file en 1,5 s** (`main` : 11 m, contre le bord).
  - 72 tests CPU OK (dont `test_aide_ralentit`, `test_virages_fleches`, `test_aide_direction`, `test_conduite`, `test_course` : 0 écart avec le jeu web).
- **Limites et risques** :
  - **WFS-316 (agent 4) n'est pas encore fusionné** : fusion d'essai faite avec `feature/waforspeed-316` (e6ce403) : **aucun conflit dans `player.cpp`** (sa correction joue jusqu'à 70 km/h, la mienne au-dessus de 100 km/h) ; conflits d'ajouts seulement dans `CMakeLists.txt` (un test chacun) et `src/game/virages_banc.cpp` (sections ajoutées des deux côtés) : à résoudre en gardant les deux pour la 2e fusion.
  - Entre 70 et 100 km/h, la direction reste celle d'avant (ni WFS-316, ni ce point) : à juger par l'utilisateur.
  - L'adhérence elle-même (≈ 2 g, plus qu'un vrai pneu) reste à WFS-259 (décision du PO) ; à fond, le volant donne donc toujours 2,1 g.
  - À confirmer par l'utilisateur sur son téléphone (🟡).

## Captures pour le lead dev (14 images, banc `virages`, `WFS_VIRAGES=vif`, 1280 × 720, rendu de la CI)

Dossier : [`qa/pr/WFS-258`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258) — `tools/qa/captures_pr.sh` (`FILTRE='virages-vif*'`, `SANS_AVANT=1` : `main` n'a pas ces sections ; les images « avant » sont produites par le banc avec `g_avantWfs258Vif`). Mesures : `qa/pr/WFS-258/virages-vif.txt`.

### 1. Ligne droite de la Ville vue de dessus : Golf à 150 (vert), 190 (orange) et 230 km/h (rouge), quart de volant à droite 0,5 s puis lâché
Avant : 2,55 à 2,65 m de décalage, la voiture traverse la route ; après : 0,74 m, un changement de file.

| Avant (main) | Après |
|---|---|
| ![dessus-avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-dessus-avant.jpg?raw=true) | ![dessus-apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-dessus-apres.jpg?raw=true) |

### 2. Dans la partie : Grand Prix, Gallardo à 200 km/h, aide désactivée, quart de volant à droite 0,5 s ; une image toutes les 0,3 s
Avant : 0,95 → 3,8 → 7,3 → 10,8 m, la voiture tape le bord (167 km/h) ; après : 0,26 → 1,1 → 2,1 → 3,1 → 4,1 m, changement de file.

| Avant (main) | Après |
|---|---|
| ![avant-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-avant-00.jpg?raw=true) | ![apres-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-apres-00.jpg?raw=true) |
| ![avant-01](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-avant-01.jpg?raw=true) | ![apres-01](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-apres-01.jpg?raw=true) |
| ![avant-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-avant-02.jpg?raw=true) | ![apres-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-apres-02.jpg?raw=true) |
| ![avant-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-avant-03.jpg?raw=true) | ![apres-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-apres-03.jpg?raw=true) |
| ![avant-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-avant-04.jpg?raw=true) | ![apres-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-apres-04.jpg?raw=true) |
| ![avant-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-avant-05.jpg?raw=true) | ![apres-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-apres-05.jpg?raw=true) |
