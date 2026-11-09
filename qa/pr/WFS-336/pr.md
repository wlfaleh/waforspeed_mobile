Agent : lead-dev

## Demande de l'utilisateur
« Après environ 2 minutes de course, le jeu rame et saccade » (ANO-151, essai sur son téléphone) : le trafic s'accumulait autour du joueur. WFS-336 remplace la PR #249 (WFS-281, fermée sans fusion) : petite PR propre, **seulement le correctif du jeu**.

## Ce qui change
- `src/game/traffic.*`, `src/game/traffic_system.*` (version finale d'agent arwa, 71e540e, reprise telle quelle) :
  - **nombre total de voitures constant** (`totalFixe`) : plus aucune voiture créée en course ;
  - **montée de difficulté gardée** (décision du PO) : +6 voitures par checkpoint en Course solo, prises dans une réserve préparée au chargement, plafonnée à +15 % ;
  - `regulerDensite` (1 fois par seconde) : hors de la vue, une zone trop dense renvoie ses voitures lentes à plus de 800 m ; rien de visible à l'écran ;
  - `keepTrafficInView` identique à `main` ; la Poursuite garde son trafic dense.
- `tests/test_trafic_duree.cpp` (nouveau) : 10 min, 4 cartes, 10 graines, Haute et Basse, **chaque fenêtre de 30 s** (P90 des véhicules dessinés ≤ max(1,3 R ; R + 2), ≤ 8), total constant, aucune voiture immobile plus de 20 s.
- Accroches de banc seulement : `bench_perf.cpp` (médianes du trafic, `WFS_PERF_AVANCE_RENDU`, `WFS_PERF_CAPTURE`), `desktop_main.cpp` (1 ligne), `CMakeLists.txt` (2 lignes). Rien d'autre à l'écran.

## Vérification
- `test_trafic_duree` : **0 fenêtre hors règle sur 1600** ; avec `--avant` (comportement de `main`) : 771 sur 1600 → le test attrape bien l'accumulation.
- Tests CPU : **80 sur 80 OK**.
- Banc perf, Ville (tableau ci-dessous) : à 10 min, le trafic coûte 2 fois moins d'appels et 3 fois moins de triangles qu'avant ; le total de voitures reste à 54 (sur main il monte jusqu’à 114, relevé du test `--avant`).

| Mesure : TrafficSystem (banc perf, Ville, médiane de 90 images) | Haute avant (main) | Haute après | Basse avant | Basse après |
|---|---|---|---|---|
| départ (0 image) | 12 appels / 9 547 tri. | 12 / 9 547 (2 véhicules) | 0 / 0 | 0 / 0 |
| ≈ 2 min 50 (10000 images) | 76 / 86 119 | 25 / 11 331 (2 véhicules) | 31 / 59 603 | 2 / 24 291 (2 véhicules) |
| 10 min, jour (36000 images) | **201 / 170 554** | **94 / 58 020** (3 véhicules) | 56 / 138 966 | 30 / 32 680 (2 véhicules) |
| 10 min, nuit | **222 / 170 596** | **100 / 58 032** (3 véhicules) | 70 / 139 207 | 35 / 32 784 (2 véhicules) |
| voitures au total | monte (54 → 114 au relevé du test, `--avant`) | **54 constant** | | |

## Limites et risques
- À 10 min (Haute), le trafic fait encore 94 appels contre 12 au départ, pour **3 véhicules dessinés au lieu de 2** : c'est le coût d'une voiture proche, pas l'accumulation ; décision du PO (22:51:10) : jugé sur les véhicules dessinés, le coût d'une voiture proche est traité par **WFS-335**.
- La commande de reproduction du ticket (54 → 644 appels à 2 min 50) ne se reproduit plus telle quelle sur le `main` actuel : 76 appels à 2 min 50 et 201 à 10 min sur `main` (le reste du jeu a changé depuis) ; l'accumulation se voit toujours sur `main` et plus sur la branche.
- À 2 min 50, la Golf du **pilote automatique du banc** sort de la route sur `main` comme sur la branche (hors ticket).
- Banc PC (lavapipe) : temps d'image non représentatif du téléphone. **À valider par l'utilisateur sur l'APK** (🟡) : 10 min de course en Ville sans ralentissement.

## Captures (18 : 9 paires)

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `ville-haute-depart` — Ville, Haute, départ (0 image) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/avant-ville-haute-depart.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/apres-ville-haute-depart.jpg?raw=true) |
| `ville-haute-2min50` — Ville, Haute, ≈ 2 min 50 (10000 images) : la Golf du pilote automatique du banc sort de la route, **sur main comme sur la branche** (pilote du banc, hors ticket) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/avant-ville-haute-2min50.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/apres-ville-haute-2min50.jpg?raw=true) |
| `ville-haute-10min` — Ville, Haute, 10 min (36000 images) : Golf sur la chaussée | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/avant-ville-haute-10min.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/apres-ville-haute-10min.jpg?raw=true) |
| `ville-basse-10min` — Ville, Basse, 10 min : Golf sur la chaussée | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/avant-ville-basse-10min.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/apres-ville-basse-10min.jpg?raw=true) |
| `ville-nuit-haute-10min` — Ville de nuit, Haute, 10 min : Golf sur la chaussée, phares du trafic | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/avant-ville-nuit-haute-10min.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/apres-ville-nuit-haute-10min.jpg?raw=true) |
| `ville-nuit-basse-10min` — Ville de nuit, Basse, 10 min | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/avant-ville-nuit-basse-10min.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/apres-ville-nuit-basse-10min.jpg?raw=true) |
| `poursuite-police` — Poursuite (trafic dense, garde son propre trafic) : aucune voiture qui disparaît | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/avant-poursuite-police.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/apres-poursuite-police.jpg?raw=true) |
| `poursuite-police-basse` — Poursuite, Basse | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/avant-poursuite-police-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/apres-poursuite-police-basse.jpg?raw=true) |
| `poursuite-bouchon` — Poursuite, bouchon | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/avant-poursuite-bouchon.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-336/qa/pr/WFS-336/apres-poursuite-bouchon.jpg?raw=true) |
