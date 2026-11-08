## Agent : Claude, agent dev conduite

**Ticket** : WFS-258 (P1, ANO-131 ; ANO-138), **point 3 de RG-00b** : « en conduite normale, la direction ne doit pas être trop rapide : sensibilité qui baisse avec la vitesse » — règles : `regles-de-gestion/WFS-258.md` (Suivi : accord écrit du PO, seulement au-dessus de 100 km/h, fonction séparée, rien sous 100 km/h (zone de WFS-316) ; puis précision accordée après le refus du lead dev : au-dessus de 100 km/h, la limite de l'aide ne cède qu'au frein). **Version corrigée après le refus du lead dev** (commit relu à relire : voir le dernier commit de la branche).

## Description

- **Demande de l'utilisateur** : « quand la voiture roule normalement, le changement de direction est trop rapide selon la vitesse ».
- **Mesure sur `main`** (aide désactivée, volant tenu 0,5 s) : dès 100 km/h, un quart de volant demande presque toute l'adhérence (Golf 1,7 g à 100 km/h ; 2,1 g dès 150 km/h, autant que le volant à fond) ; à 200 km/h dans la partie, un quart de volant 0,5 s envoie la voiture 11 m sur le côté, contre le bord.
- **Ce qui change** :
  - `src/game/player.cpp` : fonction **`reductionBraquageHauteVitesse(v, empattement)`**, appelée en une ligne : au-dessus de 100 km/h, volant à fond = 1,2 × l'adhérence du jeu (RG-06), un quart de volant ≈ un quart ; progressive de 100 à 130 km/h ; **rien sous 100 km/h** ; `maxSteer`, `steerV`, `grip` inchangés ; ordinateur (`Aide::Web`) inchangé.
  - **Correction du refus du lead dev** : au-dessus de 100 km/h, **la limite de vitesse de l'aide ne cède plus qu'au frein** (le doigt à fond y veut dire « tourner au maximum ») ; sous 100 km/h, elle cède comme avant au doigt ≥ 0,35 ou au frein (option A). Sans cela, le joueur aide activée qui braque plus pour le même virage passait le seuil et la limite lui cédait (régression du point 1 validé par l'utilisateur).
  - Banc `virages` : sections 8 (vue de dessus), 9 (changement de file à 200 km/h), **10 (virage rapide du Grand Prix, aide activée, doigt tenu)** et classement en Course à 5 doigt tenu (`WFS_VIRAGES=vif`) ; interrupteur `g_avantWfs258Vif` (banc seulement) pour rejouer `main` ; blocs `if (tout)` réindentés.
- **Comment c'est vérifié** :
  - `test_direction_vitesse` : sous 100 km/h identique à `main` au bit près ; au-dessus de 130 km/h, quart de volant ≤ 40 % du volant à fond (Golf 0,63 g pour 2,11 g ; `main` 2,11 / 2,11) ; volant à fond inchangé ; aucun à-coup de 98 à 140 km/h (0,115 g au plus par 2 km/h).
  - `test_aide_ralentit` **compte maintenant aussi les tours aide activée, doigt tenu (« Glisser »)** et exige 0 sortie : tableau demandé par le lead dev :

| Tour, aide activée, doigt tenu (images hors route) | `main` | branche refusée (b87d1ee) | **branche corrigée** |
|---|---|---|---|
| Grand Prix, Golf / Gallardo | 0 / 35 | 1316 / 1366 | **0 / 0** |
| Campagne, Golf / Gallardo | 0 / 13 | 19 / 185 | **0 / 0** |
| Ville, Golf / Gallardo | 0 / 0 | 326 / 485 | **0 / 0** |
| Corniche, Golf / Gallardo | 0 / 21 | 291 / 845 | **0 / 0** |

  Aide activée, volant lâché : 0 sortie sur les 4 cartes (inchangé, point 1). 72 tests CPU OK.
  - **Course à 5, aide activée, doigt tenu** (banc `virages`, `virages-vif.txt`) : direction de `main` → direction réduite : Ville 4/5 (87 images hors route) → **5/5 (0)** ; Campagne 2/5 → **2/5** ; Corniche 2/5 → **3/5** ; Grand Prix 4/5 → **5/5** (0 sortie partout). **Décision du PO** (Suivi de WFS-258.md) : accepté tel quel, la place recule parce que les adversaires prennent les virages trop vite (≈ 6 g), traité par **WFS-327** ; la mesure sera refaite après sa fusion.
  - Ligne droite de la Ville, quart de volant 0,5 s : 0,74 m de décalage à 150, 190 et 230 km/h (`main` 2,55 à 2,65 m) ; dans la partie à 200 km/h : changement de file de 4 m (`main` 11 m, contre le bord).
- **Limites et risques** :
  - **Au-dessus de 100 km/h, la direction demande plus de doigt** : dans le virage rapide du Grand Prix (rayon 60 m, 127 km/h), le joueur simulé braque à fond (doigt −1,00, `main` −0,08) et passe près du bord extérieur, sans sortir. C'est voulu (direction progressive), mais c'est le ressenti à juger par l'utilisateur.
  - Au quart de volant, la voiture tourne plus fort à 90 km/h (1,50 g) qu'à 130 km/h (0,63 g) : la direction sous 100 km/h n'est pas touchée (zone de WFS-316).
  - **WFS-316 n'est pas fusionné** : fusion d'essai avec `feature/waforspeed-316` (e6ce403) : aucun conflit dans `player.cpp` ; conflits d'ajouts dans `CMakeLists.txt` et `virages_banc.cpp` (garder les deux).
  - À confirmer par l'utilisateur sur son téléphone (🟡, nouvel essai après la fusion).

## Captures pour le lead dev (30 images, banc `virages`, `WFS_VIRAGES=vif`, 1280 × 720, rendu de la CI)

Dossier : [`qa/pr/WFS-258`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258) — `tools/qa/captures_pr.sh` (`SANS_AVANT=1` : `main` n'a pas ces sections ; « avant » rejoué par `g_avantWfs258Vif`). Mesures : `virages-vif.txt`.

### 1. Ligne droite de la Ville vue de dessus : Golf à 150 (vert), 190 (orange) et 230 km/h (rouge), quart de volant à droite 0,5 s
| Avant (main) | Après |
|---|---|
| ![vif-dessus-avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-dessus-avant.jpg?raw=true) | ![vif-dessus-apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-dessus-apres.jpg?raw=true) |

### 2. Dans la partie : Grand Prix, Gallardo à 200 km/h, aide désactivée, quart de volant à droite 0,5 s (une image toutes les 0,3 s)
Avant : 0,95 → 3,8 → 7,3 → 10,8 m, la voiture tape le bord ; après : 0,26 → 1,1 → 2,1 → 3,1 → 4,1 m.

| Avant (main) | Après |
|---|---|
| ![vif-avant-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-avant-00.jpg?raw=true) | ![vif-apres-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-apres-00.jpg?raw=true) |
| ![vif-avant-01](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-avant-01.jpg?raw=true) | ![vif-apres-01](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-apres-01.jpg?raw=true) |
| ![vif-avant-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-avant-02.jpg?raw=true) | ![vif-apres-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-apres-02.jpg?raw=true) |
| ![vif-avant-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-avant-03.jpg?raw=true) | ![vif-apres-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-apres-03.jpg?raw=true) |
| ![vif-avant-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-avant-04.jpg?raw=true) | ![vif-apres-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-apres-04.jpg?raw=true) |
| ![vif-avant-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-avant-05.jpg?raw=true) | ![vif-apres-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-vif-apres-05.jpg?raw=true) |

### 3. Virage rapide du Grand Prix (rayon 60 m), Golf, **aide activée, doigt tenu** (« Glisser »), gaz tenu ; 8 images de 200 m avant à 120 m après
Les deux restent sur la route ; après, l'aide freine avant le virage (174 → 127 km/h) puis le joueur braque à fond (doigt −1,00) et passe près du bord extérieur ; avant, doigt −0,08.

| Avant (main) | Après |
|---|---|
| ![rapide-tenu-avant-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-avant-00.jpg?raw=true) | ![rapide-tenu-apres-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-apres-00.jpg?raw=true) |
| ![rapide-tenu-avant-01](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-avant-01.jpg?raw=true) | ![rapide-tenu-apres-01](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-apres-01.jpg?raw=true) |
| ![rapide-tenu-avant-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-avant-02.jpg?raw=true) | ![rapide-tenu-apres-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-apres-02.jpg?raw=true) |
| ![rapide-tenu-avant-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-avant-03.jpg?raw=true) | ![rapide-tenu-apres-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-apres-03.jpg?raw=true) |
| ![rapide-tenu-avant-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-avant-04.jpg?raw=true) | ![rapide-tenu-apres-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-apres-04.jpg?raw=true) |
| ![rapide-tenu-avant-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-avant-05.jpg?raw=true) | ![rapide-tenu-apres-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-apres-05.jpg?raw=true) |
| ![rapide-tenu-avant-06](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-avant-06.jpg?raw=true) | ![rapide-tenu-apres-06](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-apres-06.jpg?raw=true) |
| ![rapide-tenu-avant-07](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-avant-07.jpg?raw=true) | ![rapide-tenu-apres-07](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-apres-07.jpg?raw=true) |
