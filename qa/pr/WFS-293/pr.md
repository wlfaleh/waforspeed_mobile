Agent : Claude, Agent 0 developer (session 01UY1Vv2SSLK)

**Ticket** : WFS-293 — règles : `regles-de-gestion/WFS-293.md` (RG-01 à RG-14, décisions du PO du 2026-11-08 22:41 et 23:53 UTC).

## Description
- **Demande de l'utilisateur** : la voiture « saute » en virage (ANO-166) : le banc de fluidité doit compter l'à-coup partout (virages, carrefour, cadences régulières et irrégulières) et l'à-coup en virage doit être corrigé, dans la même PR.
- **Ce qui change** :
  - `src/game/track.cpp` (**cause 1**) : `pointLisse` en **B-spline cubique** (position au sol et cap à courbure continue) au lieu de Catmull-Rom, dont la courbure sautait à chaque segment de 1,2 m : tout point écarté de l'axe (voiture, caméra) changeait de vitesse d'un coup tous les 1,2 m (point à vitesse constante, 2 m de l'axe, virage serré : 7,8 mm à 30 img/s ; après : 2,1 mm ; épingle : 0,65 mm). Écart à la route : L²/(6R) au plus (≈ 6 mm pour 40 m de rayon), invisible.
  - `src/game/camera_vue.cpp` (**cause 2**) : la distance de la caméra suivait l'accélération du DERNIER pas (`lonAcc`), qui bascule d'un pas à l'autre au gaz tout ou rien ; lue une fois par image, elle tombait au hasard et la caméra sautait (38 à 54 mm à 15 img/s contre la référence). Accélération tirée maintenant de la vitesse lissée (filtre exact de 0,3 s, indépendant de la cadence) ; même valeur moyenne.
  - `src/game/bench_fluidite.cpp` (banc durci) : à-coup compté dans tous les scénarios (RG-01), **carrefour** de la Ville (RG-02, virage de rue de WFS-283), cadences **régulières et irrégulières** 10 à 90 img/s (RG-03), **indicateur de référence** (RG-07, RG-12) : la même course rejouée à 120 Hz (mêmes commandes, rejeu vérifié exact) ; écart de chaque image rendue à la référence au même instant simulé. Juges : référence < 5 mm à ≥ 30 img/s et ≤ 10 mm à 10-20 img/s ; WFS-247 < 5 mm en plus en ligne droite, montée et virage serré à ≥ 30 img/s (RG-13) ; aller-retour ≤ 2 cm. Suites d'images `WFS_FLUIDITE_IMAGES=1`, diagnostic du tracé `WFS_FLUIDITE_PISTE=1`.
  - `src/game/fluidite_reference.h` + `tests/test_fluidite_reference.cpp` (RG-10) : indicateur testé : 0,003 mm sans saut, saut de 5 mm injecté mesuré 5,00 mm.
  - Rien dans `player.*` ni `conduite_*` ; rien d'iOS ; aucun fichier de la CI.
- **Comment c'est vérifié** :
  - **Banc complet de la branche : 0 manque** dans les 5 scénarios (ligne droite, virage serré, épingle, montée, carrefour), vues Proche et Poursuite, cadences régulières et irrégulières, 10 à 90 img/s.
  - **Même banc sur `main` (base de la branche)** : virage serré **20 manques**, épingle **8**, carrefour **8** (RG-10 : `main` échoue bien).
  - **RG-14** : saut de 5 mm injecté dans l'épingle à 30 img/s, mesuré 4,92 mm (Proche) et 4,84 mm (Poursuite) par la référence.
  - Tests CPU : 89 OK (dont `test_fluidite_reference`) ; banc `parcours` : 0 écart (vue normale et Cockpit).
- **Limites et risques** :
  - Une irrégularité de la simulation elle-même (`player.*`) dans l'épingle et au carrefour ne serait vue par aucun des deux juges (limite acceptée par le PO : relève de WFS-258).
  - L'indicateur de WFS-247 reste au tableau pour information dans l'épingle et au carrefour : il y a un plancher (baisse en h² à h³ avec la cadence : 23 mm à 30 img/s, 4 mm à 90) alors que la référence y donne 1,2 à 1,5 mm.
  - À valider par l'utilisateur sur son téléphone : la voiture ne doit plus sauter en virage (🟡).

## Tableau avant (`main`) / après (branche), cadences irrégulières (RG-05, RG-14)

| Scénario | Vue | img/s | réf. 120 Hz main → branche (mm) | WFS-247 main → branche (mm) | aller-retour main → branche (cm) |
|---|---|---|---|---|---|
| virage-serre | Proche | 10 | 0.73 → **0.42** | 8.28 → 5.54 | 0.86 → 0.56 |
| virage-serre | Proche | 15 | 39.31 → **4.46** | 16.60 → 15.20 | 1.76 → 1.51 |
| virage-serre | Proche | 20 | 65.89 → **4.07** | 11.77 → 11.29 | 1.40 → 1.01 |
| virage-serre | Proche | 26 | 47.28 → **3.69** | 7.31 → 1.67 | 0.63 → 0.20 |
| virage-serre | Proche | 30 | 57.43 → **2.46** | 6.01 → 1.56 | 0.56 → 0.15 |
| virage-serre | Proche | 60 | 29.71 → **0.61** | 3.80 → 0.64 | 0.26 → 0.05 |
| virage-serre | Proche | 90 | 19.98 → **0.23** | 2.13 → 0.23 | 0.15 → 0.02 |
| virage-serre | Poursuite | 10 | 0.71 → **0.41** | 7.91 → 5.50 | 0.86 → 0.56 |
| virage-serre | Poursuite | 15 | 35.16 → **4.74** | 21.08 → 19.51 | 2.16 → 1.89 |
| virage-serre | Poursuite | 20 | 49.68 → **4.51** | 14.62 → 14.07 | 1.54 → 1.26 |
| virage-serre | Poursuite | 26 | 44.78 → **3.97** | 7.45 → 2.01 | 0.90 → 0.27 |
| virage-serre | Poursuite | 30 | 54.05 → **2.48** | 6.07 → 1.54 | 0.81 → 0.19 |
| virage-serre | Poursuite | 60 | 28.04 → **0.58** | 3.77 → 0.59 | 0.36 → 0.06 |
| virage-serre | Poursuite | 90 | 18.88 → **0.22** | 2.11 → 0.24 | 0.21 → 0.02 |
| epingle | Proche | 10 | 0.87 → **0.57** | 7.04 → 5.01 | 0.73 → 0.50 |
| epingle | Proche | 15 | 3.50 → **3.44** | 25.48 → 25.38 | 0.56 → 0.54 |
| epingle | Proche | 20 | 5.45 → **3.30** | 23.32 → 23.17 | 0.52 → 0.45 |
| epingle | Proche | 26 | 5.42 → **2.86** | 22.02 → 21.99 | 0.71 → 0.37 |
| epingle | Proche | 30 | 6.99 → **1.48** | 23.51 → 23.46 | 0.71 → 0.35 |
| epingle | Proche | 60 | 15.39 → **0.73** | 6.63 → 6.63 | 0.43 → 0.09 |
| epingle | Proche | 90 | 13.11 → **0.51** | 4.04 → 4.05 | 0.16 → 0.04 |
| epingle | Poursuite | 10 | 0.70 → **0.48** | 6.76 → 4.95 | 0.73 → 0.50 |
| epingle | Poursuite | 15 | 3.01 → **2.96** | 26.34 → 26.11 | 0.58 → 0.65 |
| epingle | Poursuite | 20 | 4.55 → **2.95** | 24.16 → 23.68 | 0.49 → 0.41 |
| epingle | Poursuite | 26 | 5.77 → **2.68** | 22.17 → 22.13 | 0.96 → 0.42 |
| epingle | Poursuite | 30 | 6.56 → **1.47** | 23.64 → 23.57 | 1.01 → 0.40 |
| epingle | Poursuite | 60 | 10.53 → **0.73** | 6.71 → 6.70 | 0.59 → 0.11 |
| epingle | Poursuite | 90 | 12.41 → **0.59** | 4.13 → 4.14 | 0.24 → 0.05 |
| carrefour | Proche | 10 | 0.75 → **0.50** | 5.85 → 4.47 | 0.61 → 0.46 |
| carrefour | Proche | 15 | 2.34 → **0.54** | 9.77 → 9.57 | 0.57 → 0.28 |
| carrefour | Proche | 20 | 5.70 → **2.42** | 26.65 → 26.55 | 1.35 → 0.33 |
| carrefour | Proche | 26 | 6.99 → **2.31** | 17.30 → 17.20 | 1.21 → 0.24 |
| carrefour | Proche | 30 | 7.75 → **1.15** | 22.89 → 22.85 | 0.55 → 0.43 |
| carrefour | Proche | 60 | 12.70 → **0.53** | 5.88 → 5.86 | 0.39 → 0.06 |
| carrefour | Proche | 90 | 13.10 → **0.28** | 1.86 → 1.72 | 0.24 → 0.03 |
| carrefour | Poursuite | 10 | 0.70 → **0.49** | 5.51 → 4.25 | 0.61 → 0.46 |
| carrefour | Poursuite | 15 | 2.94 → **0.56** | 14.53 → 14.36 | 0.56 → 0.28 |
| carrefour | Poursuite | 20 | 4.71 → **2.45** | 26.84 → 26.75 | 1.15 → 0.37 |
| carrefour | Poursuite | 26 | 6.11 → **1.98** | 17.42 → 17.38 | 1.46 → 0.27 |
| carrefour | Poursuite | 30 | 8.53 → **1.17** | 23.07 → 22.96 | 1.00 → 0.46 |
| carrefour | Poursuite | 60 | 16.41 → **0.56** | 5.85 → 5.68 | 0.51 → 0.07 |
| carrefour | Poursuite | 90 | 12.34 → **0.34** | 1.77 → 1.77 | 0.31 → 0.03 |


## Captures (5 images successives : la voiture reste à la même place dans l'image)

| Suite | Avant (`main`) | Après (branche) |
|---|---|---|
| Virage serré de la Ville, 15 img/s, Poursuite | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-virage-serre-ville-15ips-poursuite.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-virage-serre-ville-15ips-poursuite.jpg?raw=true) |
| Épingle de la Corniche, 30 img/s, Proche (dans la courbe de 165° : capture au segment de plus forte courbure, revue du lead dev) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-epingle-corniche-30ips-proche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-epingle-corniche-30ips-proche.jpg?raw=true) |
| Carrefour de la Ville, 30 img/s, Poursuite | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carrefour-ville-30ips-poursuite.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carrefour-ville-30ips-poursuite.jpg?raw=true) |

Un écart de quelques millimètres ne se voit pas d'une image fixe à l'autre : la preuve est le tableau mesuré. Suites produites par le banc (`WFS_FLUIDITE_IMAGES=1 WFS_BANC=fluidite`) sur la branche et sur `main` (même fichier du banc copié dans un worktree de `main` : `main` n'a pas ce mode).

## Ville en virage (revue du lead dev : `pointLisse` change aussi pour le trafic, le duel, la poursuite, l'audio, le joueur)

Banc `ville` (`FILTRE='carte-ville-virage-*' bash tools/qa/captures_pr.sh WFS-293 ville`), avant (`main`) / après (branche) :

| Capture | écart moyen | Avant | Après |
|---|---|---|---|
| `carte-ville-virage-1` | 0,00 % | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carte-ville-virage-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carte-ville-virage-1.jpg?raw=true) |
| `carte-ville-virage-2-nuit` | 0,00 % | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carte-ville-virage-2-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carte-ville-virage-2-nuit.jpg?raw=true) |
| `carte-ville-virage-3-haut` | 0,81 % | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carte-ville-virage-3-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carte-ville-virage-3-haut.jpg?raw=true) |

Les 9 paires : virages 1 à 3 (jour, nuit) 0,00 % ; vues hautes 0,40 à 0,81 % : seulement les contours (image décalée d'≈ 1 pixel : voiture et caméra déplacées de quelques mm par la B-spline), aucun objet ne change. **Trafic en virage** : la suite du carrefour (ci-dessus, 30 img/s, Poursuite) montre une voiture du trafic dans le virage sur les 5 images, avant comme après, à la même place sur la route.
