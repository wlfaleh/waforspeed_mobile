## Agent : Claude, agent dev conduite

**Ticket** : WFS-295 (P1, ANO-168) — règles : `regles-de-gestion/WFS-295.md`

## Description

- **Demande de l'utilisateur** : dans les virages, à gauche comme à droite, la voiture ne tourne pas assez ; il faut pouvoir tourner les roues plus que ça (vidéo du 2026-11-08 sur la Corniche, flèches, aide désactivée, après un choc).
- **Ce qui change** :
  - `src/game/player.cpp` (ancienne conduite, celle du jeu depuis WFS-258) : **RG-01**, l'avance le long de la route et la rotation de la route sous la voiture sont divisées par max(0,2 ; 1 − k·n) (k = courbure, n = écart à l'axe, repère de Frenet). Avant, à l'extérieur d'un virage, la voiture tournait comme sur un rayon (1 + n/R) fois trop grand : **+28 % au bord extérieur d'une épingle de 22 m, +45 % sur le bas-côté** ; à l'intérieur, elle pivotait presque sur place (−31 %, −46 %). L'ordinateur (`Aide::Web`, règles du jeu web) n'est pas touché.
  - `src/game/player.h` / `player.cpp` : **RG-03**, après un accident, la voiture du joueur a ses commandes bloquées **0,2 + 0,3 × impact (0,5 s au plus**, avant 0,6 + 1,2 × impact, soit 1,14 s pour un choc de décor) et **garde son braquage** (avant, il était remis à zéro). L'ordinateur (`Aide::Web`) garde les règles du jeu web (`test_course` : 0 écart avec le jeu web).
  - **Non touchés** : `braquage_max`, `braquage_vitesse` (angle des roues), les aides, le modèle de WFS-241 (coupé).
  - Banc `WFS_BANC=virages` (`src/game/virages_banc.*`, accroche de 2 lignes dans `desktop_main.cpp`) et test `tests/test_virages_fleches.cpp`. Le banc produit lui-même les images **« avant »** en rejouant le comportement de `main` (interrupteur `g_avantWfs295`, banc seulement, toujours faux dans le jeu) ; le test vérifie que cet interrupteur reproduit exactement `main` (rayon au bord extérieur 5,35 m, comme mesuré sur `main`). C'est pourquoi la colonne « Avant (main) » du script est vide : `main` n'a pas ce banc (`SANS_AVANT=1`).
- **Comment c'est vérifié** :
  - `test_virages_fleches` (nouveau) : rayon volant à fond à 10 km/h au centre, aux bords et sur les bas-côtés de l'épingle : **−0,7 à −1,1 %** de la ligne droite (sur `main` : de −46 % à +45 %, le test échoue) ; rayon Golf 4,17 m pour 4,20 m à la fiche (−1 %), M4 4,74 m pour 4,60 m (+3 %) ; 6 épingles des lacets de la Corniche à 20, 25 et 30 km/h, aux flèches et en « Glisser », aide désactivée et activée : **0 image sur l'herbe** ; après un accident à 60 km/h : la voiture répond à la flèche en **0,34 s** et le braquage reste à 1,00 (sur `main` : 1,18 s, braquage remis à 0,01).
  - 66 tests CPU OK (dont `test_course`, `test_conduite`, `test_aide_direction`, `test_chocs*`).
  - Bancs de RG-04 lancés en local (rendu logiciel, comme la CI) : **parcours 0 écart** (et parcours en vue Cockpit 0 écart) ; **fluidité** : pire à-coup 3,18 mm (objectif < 5), saut d'angle 0,030° (≤ 0,2), aller-retour 1,85 cm ; **Course à 5** (fin → Rejouer → Menu) **0 écart**.
  - Poursuite, critère « appels de dessin en Basse ≤ +60 » : **déjà en échec sur `main`** (d937129 : +104) ; sur la branche : +158 (au « camion en travers », où la référence « Checkpoint » passe elle-même de 248 à 199 appels) : la mesure suit la scène autour de la voiture, qui se déplace autrement ; rien dans ce ticket ne touche au rendu.
- **Limites et risques** :
  - Dans l'épingle prise sur la file extérieure, mon joueur simulé passe aussi sur `main` (il corrige à chaque instant) : la différence se voit sur le rayon (vue de dessus) et surtout **après un choc** (suite « choc »), qui est le cas de la vidéo (choc à 116 km/h, puis la voiture ne répond plus).
  - La marche arrière au frein (le « R » de la vidéo) est le sujet de WFS-296 ; l'adhérence trop forte de l'ancien modèle (≈ 2 g), celui de WFS-259.
  - Le ressenti reste à confirmer sur le téléphone de l'utilisateur (🟡 après fusion).

## Captures pour le lead dev (47 images, banc `virages`, 1280 × 720, rendu de la CI)

Dossier : [`qa/pr/WFS-295`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295) — produit par `tools/qa/captures_pr.sh` (`SANS_AVANT=1`). Rayons de chaque trace : `qa/pr/WFS-295/virages.txt`.

### 1. Épingle à gauche des lacets de la Corniche, vue de dessus : Golf au bord extérieur, flèche gauche tenue 2 s (bleu 10, vert 20, orange 30, rouge 50 km/h)
Rayons : avant 5,29 / 5,90 / 6,45 / 10,55 m → après 4,12 / 4,81 / 5,43 / 8,47 m.

| Avant (comportement de main) | Après |
|---|---|
| ![dessus-avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-dessus-avant.jpg?raw=true) | ![dessus-apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-dessus-apres.jpg?raw=true) |

### 2. La même épingle dans la partie, vue Proche, aux flèches, 20 km/h, aide désactivée (8 images)
| Avant | Après |
|---|---|
| ![proche-avant-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-avant-00.jpg?raw=true) | ![proche-apres-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-apres-00.jpg?raw=true) |
| ![proche-avant-01](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-avant-01.jpg?raw=true) | ![proche-apres-01](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-apres-01.jpg?raw=true) |
| ![proche-avant-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-avant-02.jpg?raw=true) | ![proche-apres-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-apres-02.jpg?raw=true) |
| ![proche-avant-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-avant-03.jpg?raw=true) | ![proche-apres-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-apres-03.jpg?raw=true) |
| ![proche-avant-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-avant-04.jpg?raw=true) | ![proche-apres-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-apres-04.jpg?raw=true) |
| ![proche-avant-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-avant-05.jpg?raw=true) | ![proche-apres-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-apres-05.jpg?raw=true) |
| ![proche-avant-06](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-avant-06.jpg?raw=true) | ![proche-apres-06](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-apres-06.jpg?raw=true) |
| ![proche-avant-07](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-avant-07.jpg?raw=true) | ![proche-apres-07](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-proche-apres-07.jpg?raw=true) |

### 3. Choc puis flèche : accident à 60 km/h (impact 0,45), flèche droite tenue ; une image toutes les 0,4 s pendant 2 s
Avant : commandes bloquées 1,14 s, volant retombé à 0,04, la voiture file tout droit. Après : blocage 0,34 s, volant gardé à 1,00, la voiture tourne tout de suite (puis touche le trottoir, choc de WFS-256).

| Avant | Après |
|---|---|
| ![choc-avant-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-choc-avant-00.jpg?raw=true) | ![choc-apres-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-choc-apres-00.jpg?raw=true) |
| ![choc-avant-01](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-choc-avant-01.jpg?raw=true) | ![choc-apres-01](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-choc-apres-01.jpg?raw=true) |
| ![choc-avant-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-choc-avant-02.jpg?raw=true) | ![choc-apres-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-choc-apres-02.jpg?raw=true) |
| ![choc-avant-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-choc-avant-03.jpg?raw=true) | ![choc-apres-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-choc-apres-03.jpg?raw=true) |
| ![choc-avant-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-choc-avant-04.jpg?raw=true) | ![choc-apres-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-choc-apres-04.jpg?raw=true) |
| ![choc-avant-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-choc-avant-05.jpg?raw=true) | ![choc-apres-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-choc-apres-05.jpg?raw=true) |

### 4. Rond-point de la Ville aux flèches, 30 km/h : aide désactivée / activée (après)
| Aide désactivée | Aide activée |
|---|---|
| ![rond-point-sans-aide-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-sans-aide-00.jpg?raw=true) | ![rond-point-aide-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-aide-00.jpg?raw=true) |
| ![rond-point-sans-aide-01](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-sans-aide-01.jpg?raw=true) | ![rond-point-aide-01](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-aide-01.jpg?raw=true) |
| ![rond-point-sans-aide-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-sans-aide-02.jpg?raw=true) | ![rond-point-aide-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-aide-02.jpg?raw=true) |
| ![rond-point-sans-aide-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-sans-aide-03.jpg?raw=true) | ![rond-point-aide-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-aide-03.jpg?raw=true) |
| ![rond-point-sans-aide-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-sans-aide-04.jpg?raw=true) | ![rond-point-aide-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-aide-04.jpg?raw=true) |
| ![rond-point-sans-aide-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-sans-aide-05.jpg?raw=true) | ![rond-point-aide-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-aide-05.jpg?raw=true) |
| ![rond-point-sans-aide-06](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-sans-aide-06.jpg?raw=true) | ![rond-point-aide-06](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-aide-06.jpg?raw=true) |
| ![rond-point-sans-aide-07](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-sans-aide-07.jpg?raw=true) | ![rond-point-aide-07](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-rond-point-aide-07.jpg?raw=true) |

### 5. Cercles volant à fond à 10 km/h : Golf (bleu) 4,17 m, M4 (rouge) 4,74 m ; cercle de la fiche en blanc
![cercle-10kmh](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-295/qa/pr/WFS-295/apres-virages-cercle-10kmh.jpg?raw=true)
