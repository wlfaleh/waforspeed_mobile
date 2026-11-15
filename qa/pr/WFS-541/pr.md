Agent : Claude, agent conduite (01UY1Vv1WU)

**Ticket** : WFS-541 (P1, E3) — règles : `regles-de-gestion/WFS-541.md` — anomalies ANO-269, ANO-270 (vidéos de l'utilisateur du 2026-11-13, Poursuite, Corniche, nuit).

## Description

- **Demande de l'utilisateur** : dans la Poursuite, une voiture du trafic heurtée ne doit plus rester en travers et bloquer la route, et les voitures du trafic ne doivent plus se traverser ni passer à travers la caméra.
- **Ce qui change** :
  - `src/game/traffic.{h,cpp}` :
    - **RG-01** : une voiture heurtée glisse, s'arrête (≈ 1 s), puis **se redresse en roulant** (modèle bicyclette : roues braquées à 35° au plus, 1 rad/s, ≤ 15 km/h, 2,5 m/s²). Si elle regarde de travers ou vers l'arrière, elle fait un demi-tour en plusieurs temps (avant / arrière, roues à fond) jusqu'à l'axe de sa file, puis reprend sa route. Bloquée 1,5 s : elle se dégage dans l'autre sens.
    - **Retrait** : pas remise en route en 8 s → retirée **seulement hors de la vue** (> 150 m devant ou > 15 m derrière, donc derrière la caméra) et replacée loin devant (`placerLoin`, mis en commun avec la régulation WFS-281). Avant : relevée d'un coup sur place dès 60 m, sous les yeux du joueur, et **jamais** tant que le joueur restait à moins de 60 m (cause d'ANO-269).
    - **RG-03** : nouveaux **contacts entre voitures du trafic** (`contacts`), à chaque pas et après les chocs du joueur. Emprises dessinées WFS-333, balayage le long de la route, 4 passes. Un choc par l'arrière est mou. Une voiture qui roule n'est pas sortie de ses files (anneaux WFS-243 compris). La voiture du joueur est fixe et résolue en dernier. Avant : aucun contact entre voitures du trafic (cause d'ANO-270).
    - `separerDe` : écarte le trafic d'une voiture qu'on ne déplace jamais (la police, ordre du 2026-11-14).
    - **RG-02** :
      - une voiture accidentée ne retient que les voitures dont elle occupe vraiment le couloir : sinon elles passent à côté ;
      - au pas, un changement de file ne fait plus glisser la voiture de côté à 3,75 m/s avec la caisse tournée de 90° : au plus v · tan 17° + 0,6 m/s, caisse ≤ 20°. Rien ne change au-delà de 38 km/h.
  - `src/game/traffic_system.cpp` :
    - une voiture du trafic qui contient la caméra n'est pas dessinée (ANO-270, « rien sous la caméra ») ;
    - nouveau banc `trafic-541` (3 scènes).
  - **Accroches annoncées au journal** :
    - `src/game/poursuite_system.cpp` : 2 lignes, la police écarte le trafic (`separerDe`) sans jamais être déplacée ;
    - `src/platform/desktop_main.cpp` : 1 ligne, groupe de banc `trafic-541`.
  - **Tests** :
    - nouveau `tests/test_trafic_chocs541.cpp` ;
    - `tests/test_penetration_voitures.cpp` passe de 6 à 12 graines. Les voitures heurtées repartent au lieu de rester devant le joueur, qui les heurtait encore et encore : 6 graines ne donnaient plus que 44 contacts (Ville, aide désactivée) pour la borne de 50. Aucune borne n'est affaiblie.
- **Comment c'est vérifié** :
  - **Tests CPU** : 145 / 145 sur la branche synchronisée avec `main`. Détail dans la capture `tests-cpu`.
  - **Banc CPU du critère** : Corniche, trafic dense de la Poursuite (× 2,5), 200 chocs provoqués autour du joueur, qui roule et s'arrête :
    - **pénétration max trafic / trafic 0,000 m**, trafic / joueur 0,000 m ;
    - **aucune voiture arrêtée plus de 7,9 s** (hors feu rouge).
  - **Carambolage** (un choc toutes les 0,4 s, bien au-delà d'une partie) : pénétration ≤ 0,032 m.
  - **Voiture heurtée, joueur arrêté à côté** : arrêt ≤ 1,2 s, puis remise dans sa file :
    - 5,7 s pour un petit choc ;
    - 8,2 à 10 s pour une voiture tournée de 90° à 180° (demi-tour en plusieurs temps, toujours en mouvement) ;
    - vitesse ≤ 15 km/h.
  - **RG-05, coût CPU** (trafic + chocs, par image) :

    | | main | branche |
    |---|---|---|
    | Banc ci-dessus | 0,074 ms | 0,090 ms |
    | Carambolage | — | 0,105 ms |

    Soit **+0,02 à +0,03 ms**, sous la borne de 0,5 ms.
  - **Trafic hors chocs** : références du jeu web (`test_traffic`, `test_course`), anneaux des ronds-points (`test_trace`), `test_trafic_vivant` et `test_trafic_duree` inchangés et verts.
  - **Captures** : banc `trafic-541`, Poursuite, Corniche, nuit, 1280x720, rendu lavapipe identique à la CI. **Avant** = même banc avec `WFS_AVANT_541=1`, qui remet le trafic de `main` : `main` n'a pas ce banc, d'où `captures_pr.sh` lancé avec `SANS_AVANT=1`.
    1. `trafic541-suv-00…13`, vue Proche, 0,75 s entre deux :
       - avant : le Range Rover Sport heurté reste en travers 10,5 s devant le joueur arrêté ;
       - après : il recule, braque, se remet dans sa file et repart à 35 km/h.
    2. `trafic541-bouchon-00…09`, caméra haute, 0,8 s entre deux : 2 voitures arrivent derrière une voiture accidentée en travers, s'arrêtent, puis changent de file sans glisser de travers.
    3. `trafic541-pres-00…07`, vue Proche, 12 km/h, bouchon serré :
       - avant : la voiture blanche en glissade entre dans la grise (−5 « CHOC AVEC LE TRAFIC », pénétration 0,81 m au journal) ;
       - après : elle s'arrête contre elle, puis se redresse.
- **Limites et risques connus** :
  - Ticket de conduite né d'une vidéo de l'utilisateur : **🟡 À valider (utilisateur)** sur l'APK de `main`, même parcours que les vidéos du 2026-11-13.
  - **Deux-roues couchés** : ils ne se redressent pas (aucune animation du pilote qui se relève). Ils restent relevés hors de la vue, comme avant. Les voitures passent maintenant à côté d'eux au lieu d'attendre derrière.
  - **Demi-tour au-delà de 8 s** : une voiture tournée de plus de 90° peut mettre jusqu'à 10 s, en roulant tout le temps. RG-01 prévoit ce cas : si elle n'a pas fini, elle est retirée dès qu'elle sort de la vue.
  - **Surprise « accident » de la Poursuite** (2 voitures posées) : inchangée, obstacle annoncé. Elle se redresse au bout de 13,5 s au lieu d'être relevée sur place.
  - **Police** : le trafic s'écarte d'elle. La police elle-même relève de WFS-540 (`poursuite.cpp`, non touché).
  - **Caméra** : une voiture du trafic qui la contiendrait n'est pas dessinée (pas de fondu).

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv1WUtoyPMiBqhjFrvU

## Captures pour le lead dev (33 après : 3 suites de 32 images + tests CPU)

Dossier : [`qa/pr/WFS-541`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541)

| Capture | Avant (comportement de main) | Après (branche) |
|---|---|---|
| `trafic541-bouchon-00` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-bouchon-00.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-bouchon-00.jpg?raw=true) |
| `trafic541-bouchon-01` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-bouchon-01.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-bouchon-01.jpg?raw=true) |
| `trafic541-bouchon-02` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-bouchon-02.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-bouchon-02.jpg?raw=true) |
| `trafic541-bouchon-03` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-bouchon-03.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-bouchon-03.jpg?raw=true) |
| `trafic541-bouchon-04` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-bouchon-04.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-bouchon-04.jpg?raw=true) |
| `trafic541-bouchon-05` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-bouchon-05.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-bouchon-05.jpg?raw=true) |
| `trafic541-bouchon-06` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-bouchon-06.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-bouchon-06.jpg?raw=true) |
| `trafic541-bouchon-07` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-bouchon-07.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-bouchon-07.jpg?raw=true) |
| `trafic541-bouchon-08` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-bouchon-08.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-bouchon-08.jpg?raw=true) |
| `trafic541-bouchon-09` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-bouchon-09.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-bouchon-09.jpg?raw=true) |
| `trafic541-pres-00` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-pres-00.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-pres-00.jpg?raw=true) |
| `trafic541-pres-01` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-pres-01.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-pres-01.jpg?raw=true) |
| `trafic541-pres-02` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-pres-02.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-pres-02.jpg?raw=true) |
| `trafic541-pres-03` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-pres-03.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-pres-03.jpg?raw=true) |
| `trafic541-pres-04` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-pres-04.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-pres-04.jpg?raw=true) |
| `trafic541-pres-05` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-pres-05.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-pres-05.jpg?raw=true) |
| `trafic541-pres-06` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-pres-06.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-pres-06.jpg?raw=true) |
| `trafic541-pres-07` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-pres-07.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-pres-07.jpg?raw=true) |
| `trafic541-suv-00` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-suv-00.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-suv-00.jpg?raw=true) |
| `trafic541-suv-01` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-suv-01.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-suv-01.jpg?raw=true) |
| `trafic541-suv-02` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-suv-02.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-suv-02.jpg?raw=true) |
| `trafic541-suv-03` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-suv-03.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-suv-03.jpg?raw=true) |
| `trafic541-suv-04` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-suv-04.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-suv-04.jpg?raw=true) |
| `trafic541-suv-05` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-suv-05.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-suv-05.jpg?raw=true) |
| `trafic541-suv-06` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-suv-06.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-suv-06.jpg?raw=true) |
| `trafic541-suv-07` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-suv-07.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-suv-07.jpg?raw=true) |
| `trafic541-suv-08` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-suv-08.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-suv-08.jpg?raw=true) |
| `trafic541-suv-09` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-suv-09.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-suv-09.jpg?raw=true) |
| `trafic541-suv-10` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-suv-10.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-suv-10.jpg?raw=true) |
| `trafic541-suv-11` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-suv-11.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-suv-11.jpg?raw=true) |
| `trafic541-suv-12` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-suv-12.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-suv-12.jpg?raw=true) |
| `trafic541-suv-13` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/avant-trafic541-suv-13.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-trafic541-suv-13.jpg?raw=true) |
| `tests-cpu` | — | ![tests](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/apres-tests-cpu.jpg?raw=true) |
