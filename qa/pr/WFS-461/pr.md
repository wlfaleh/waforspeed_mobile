Agent : Claude, agent conduite (01UY1Vv1A7)

**Ticket** : WFS-461 — règles : `regles-de-gestion/WFS-461.md` (ANO-261, ANO-262). Après fusion : **🟡 À valider (utilisateur)** (ticket de conduite).

## Description

- **Demande de l'utilisateur** : au contact de la police, la voiture glisse « comme sur de la glace » (vidéo du 12/11, 12:04, Corniche, Poursuite, Cockpit) ; la voiture de police (une 208 de 130 ch) roule vers 300 km/h.
- **Causes trouvées** :
  - ANO-261 (`duel::contacter`, règle du jeu web) :
    - côte à côte, la voiture du joueur était déplacée de **2 fois le recouvrement + 3 mm** à chaque pas et **tournée de 0,06 rad par pas** tant que le contact durait (jusqu'à 7 rad/s) ;
    - poussée par l'arrière, elle prenait **80 % de la vitesse de la police même frein serré**, et son cap était tiré au hasard **120 fois par seconde** ;
    - la police se recollait à 1,90 m forfaitaires, sans les vraies largeurs.
  - ANO-262 (`poursuite::avancer`) : `vMax = MAX_SPEED × 1,45` (**377 km/h**), accélération de **70 m/s²** et freinage de **140 m/s²**.
- **Ce qui change** :
  - `src/game/duel.cpp` (`contacter`) : contact réaliste, l'effort passe par les pneus.
    - Côte à côte : la voiture sort **juste** du recouvrement (aucune poussée en plus) ; le flanc de l'autre bloque la rotation vers lui (cap ramené parallèle, 2,5 rad/s au plus) ; frottement des carrosseries ≤ 0,4 × μ g, vers la vitesse de l'autre (jamais au-delà : pas de « R » par un choc).
    - Par l'arrière : choc mou (restitution 0,2, quantité de mouvement), puis poussée tenue bornée par les roues motrices de la police (55 % × μ g × sa masse) **moins le frein ou la marche arrière du joueur**. Une voiture qui freine ne se laisse plus entraîner.
    - Petit coup de cap seulement sur un vrai choc (> 3 m/s), plus jamais pendant une poussée tenue.
    - Sources : μ pneu-route sèche 0,9-1,0 ; tôle / plastique 0,3-0,5 ; restitution 0,1-0,3 à basse vitesse.
  - `src/game/poursuite.cpp/.h` : `FichePolice` = **fiche de la voiture de police** (`cars.h` : 208 → 208 km/h ; Golf GTI → 250 km/h quand le joueur est en 208).
    - Vitesse maxi, accélération (poussée `CarPerf`, même formule que `Player::update`), freinage (`Phys::brake × freinage`).
    - Adhérence dans les virages : même modèle que la voiture du joueur (`vitesseVirageAide`) ; elle « freine moins tard », pas plus vite que ses pneus.
    - Recul après une attaque à une vitesse qu'elle peut reprendre ; replacement après perte et renfort ≤ sa fiche ; vraies dimensions au contact (+ 2 cm de jour à côté).
  - `src/game/poursuite_system.cpp` : 2 lignes (fiche passée à `avancer`). `src/game/player.h` : accesseur `muSol()`.
  - Banc `WFS_BANC=police` (`src/game/police_banc.*`, nouveau ; accroche de 2 lignes dans `desktop_main.cpp`, annoncée au journal) ; test CPU `test_police_461` (+ `CMakeLists.txt`) ; `test_poursuite` : police lancée à la vitesse du joueur (elle partait de l'arrêt et ne rattrapait qu'à 70 m/s²).
- **Comment c'est vérifié** :
  - Tests CPU verts : `test_police_461`, `test_course5` (`test_poursuite`), `test_chocs_voitures`, `test_penetration_voitures`, `test_duel`, `test_course5_ordinateurs` ; syntaxe de `poursuite_system.cpp` ; banc compilé et lancé (lavapipe).
  - Mesures (`test_police_461`, même fichier compilé avec `-DAVANT` sur main) :

| Vitesse maxi de la police | Avant (main) | Après | Fiche |
|---|---|---|---|
| ligne droite, joueur à 300 km/h (208) | 360,8 km/h | **208,0 km/h** | 208 |
| rattrapage à 300 km/h, Corniche | 375,8 km/h | **208,0 km/h** | 208 |
| replacement après perte | 375,8 km/h | **208,0 km/h** | 208 |
| renfort né à 300 km/h | 375,8 km/h | **208,0 km/h** | 208 |
| Golf GTI de police (joueur en 208) | 360,8 km/h | **250,0 km/h** | 250 |
| 0-100 km/h / 100-0 de la 208 de police | ≈ 0,4 s / ≈ 3 m (70 et 140 m/s²) | **8,7 s / 35,8 m** | 8,7 s / 33-40 m |
| jouable : rattrape un joueur à 160 km/h parti 200 m devant | — | **oui, à 7,6 m** | |

| Contact (Corniche, 40 s, 2 polices qui attaquent) | Avant (main) | Après |
|---|---|---|
| frein serré, poussé par l'arrière (Golf) : vitesse donnée par la police | 46 km/h | **0** |
| marche arrière vers la police (scène de la vidéo, Golf) : vitesse donnée | 48 km/h | **0** |
| idem, Range Rover Sport | 59 km/h | **0** |
| rotation qui écarte de l'axe, frein serré / marche arrière (Golf) | 0,79 / 0,74 rad | **0,01 / 0,03 rad** |
| 30 km/h, file tenue, Range Rover Sport : rotation due aux contacts | 0,43 rad | **0** (plus de contact permanent) |
| marche arrière sans frein causée par un contact | non | non |

  - Banc d'images (ci-dessous) : contact au flanc à 30 km/h, joueur qui braque vers la police : **avant**, la Golf tourne de 5° dans la police (carrosseries l'une dans l'autre, visible aussi en Cockpit) puis rebondit ; **après**, elle s'arrête contre le flanc, cap 0°, et longe la police. Rattrapage : avant, la police est collée à 15 m dès le départ (70 m/s²) ; après, elle part 27 m derrière, prend sa vitesse (137-141 km/h au plus) et rejoint le joueur à 100 km/h en 12 s. Vitesse (joueur au plus vite, jusqu'à 193 km/h, le maximum du pilote automatique sur la Corniche) : avant, la police reste collée à 8-15 m à toutes les vitesses ; après, la 208 (169 km/h au plus) est distancée par la Golf GTI (jusqu'à 144 m), comme dans le réel (RG-04). À l'image 08, le pilote automatique du banc a un accident seul (feu grillé, rien à moins de 1 m), sans lien avec la police.
- **Limites et risques connus** :
  - La conduite du joueur reste l'ancien modèle (`g_conduiteMonde` faux, WFS-258) : la glisse latérale n'a pas d'état de vitesse latérale ; le contact est traité comme une contrainte (pas de poussée sans effort).
  - Un joueur plus rapide que 208 km/h en ligne droite distance la police (voulu, RG-04) ; elle revient par les virages, les bouchons, les renforts et le replacement à 220 m (≤ sa fiche).
  - Frein tenu à l'arrêt = marche arrière (règle WFS-90 inchangée) : dans la vidéo, le « R » vient du frein tenu, pas du choc ; aucun contact ne change le signe de la vitesse (vérifié par le test).
  - Le duel en ligne et la Course à 5 utilisent le même `contacter` : contacts plus doux (tests de non-pénétration verts).
  - À valider sur téléphone par l'utilisateur (🟡).

## Tests CPU

Commit testé : `0a14d368` (branche synchronisée avec `main`), `bash tools/qa/tests_cpu.sh build` ; journal complet : [`qa/pr/WFS-461/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/tests-cpu.log).

```
[OK] test_ville_sol (3 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 115 test(s) réussi(s) en 321 s
```

## Captures pour le lead dev (42 après, 42 avant)

Dossier : [`qa/pr/WFS-461`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461). Suites : `police-flanc-00..09` (vue Poursuite), `police-flanc-cockpit-00..09` (vue Cockpit), `police-rattrapage-00..11` (joueur à 100 km/h, une image toutes les 2 s), `police-vitesse-00..09` (joueur au plus vite, une image toutes les 3 s) ; vitesse de la police écrite sur chaque image.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `police-flanc-00` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-00.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-00.jpg?raw=true) |
| `police-flanc-01` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-01.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-01.jpg?raw=true) |
| `police-flanc-02` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-02.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-02.jpg?raw=true) |
| `police-flanc-03` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-03.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-03.jpg?raw=true) |
| `police-flanc-04` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-04.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-04.jpg?raw=true) |
| `police-flanc-05` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-05.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-05.jpg?raw=true) |
| `police-flanc-06` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-06.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-06.jpg?raw=true) |
| `police-flanc-07` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-07.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-07.jpg?raw=true) |
| `police-flanc-08` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-08.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-08.jpg?raw=true) |
| `police-flanc-09` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-09.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-09.jpg?raw=true) |
| `police-flanc-cockpit-00` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-cockpit-00.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-cockpit-00.jpg?raw=true) |
| `police-flanc-cockpit-01` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-cockpit-01.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-cockpit-01.jpg?raw=true) |
| `police-flanc-cockpit-02` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-cockpit-02.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-cockpit-02.jpg?raw=true) |
| `police-flanc-cockpit-03` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-cockpit-03.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-cockpit-03.jpg?raw=true) |
| `police-flanc-cockpit-04` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-cockpit-04.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-cockpit-04.jpg?raw=true) |
| `police-flanc-cockpit-05` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-cockpit-05.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-cockpit-05.jpg?raw=true) |
| `police-flanc-cockpit-06` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-cockpit-06.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-cockpit-06.jpg?raw=true) |
| `police-flanc-cockpit-07` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-cockpit-07.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-cockpit-07.jpg?raw=true) |
| `police-flanc-cockpit-08` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-cockpit-08.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-cockpit-08.jpg?raw=true) |
| `police-flanc-cockpit-09` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-flanc-cockpit-09.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-flanc-cockpit-09.jpg?raw=true) |
| `police-rattrapage-00` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-rattrapage-00.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-rattrapage-00.jpg?raw=true) |
| `police-rattrapage-01` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-rattrapage-01.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-rattrapage-01.jpg?raw=true) |
| `police-rattrapage-02` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-rattrapage-02.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-rattrapage-02.jpg?raw=true) |
| `police-rattrapage-03` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-rattrapage-03.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-rattrapage-03.jpg?raw=true) |
| `police-rattrapage-04` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-rattrapage-04.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-rattrapage-04.jpg?raw=true) |
| `police-rattrapage-05` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-rattrapage-05.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-rattrapage-05.jpg?raw=true) |
| `police-rattrapage-06` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-rattrapage-06.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-rattrapage-06.jpg?raw=true) |
| `police-rattrapage-07` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-rattrapage-07.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-rattrapage-07.jpg?raw=true) |
| `police-rattrapage-08` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-rattrapage-08.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-rattrapage-08.jpg?raw=true) |
| `police-rattrapage-09` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-rattrapage-09.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-rattrapage-09.jpg?raw=true) |
| `police-rattrapage-10` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-rattrapage-10.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-rattrapage-10.jpg?raw=true) |
| `police-rattrapage-11` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-rattrapage-11.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-rattrapage-11.jpg?raw=true) |
| `police-vitesse-00` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-vitesse-00.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-vitesse-00.jpg?raw=true) |
| `police-vitesse-01` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-vitesse-01.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-vitesse-01.jpg?raw=true) |
| `police-vitesse-02` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-vitesse-02.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-vitesse-02.jpg?raw=true) |
| `police-vitesse-03` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-vitesse-03.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-vitesse-03.jpg?raw=true) |
| `police-vitesse-04` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-vitesse-04.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-vitesse-04.jpg?raw=true) |
| `police-vitesse-05` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-vitesse-05.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-vitesse-05.jpg?raw=true) |
| `police-vitesse-06` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-vitesse-06.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-vitesse-06.jpg?raw=true) |
| `police-vitesse-07` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-vitesse-07.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-vitesse-07.jpg?raw=true) |
| `police-vitesse-08` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-vitesse-08.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-vitesse-08.jpg?raw=true) |
| `police-vitesse-09` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/avant-police-vitesse-09.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-461/qa/pr/WFS-461/apres-police-vitesse-09.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv1A7HLQu3LkH7Q69im
