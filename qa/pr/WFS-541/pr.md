Agent : Claude, agent conduite (01UY1Vv1WU)

**Ticket** : WFS-541 (P1, E3), règles dans `regles-de-gestion/WFS-541.md`. Anomalies ANO-269 et ANO-270, vues sur les vidéos de l'utilisateur du 2026-11-13 (Poursuite, Corniche, nuit).

## Description

**Demande de l'utilisateur** : dans la Poursuite, une voiture du trafic heurtée ne doit plus rester en travers et bloquer la route. Les voitures du trafic ne doivent plus se traverser, ni passer à travers la caméra.

### Ce qui change

**`src/game/traffic.{h,cpp}`**

- **RG-01, voiture heurtée** :
  - Après le choc, elle glisse et s'arrête en ≈ 1 s.
  - Elle se redresse ensuite en roulant, comme une vraie voiture : roues braquées à 35° au plus, 15 km/h au plus.
  - Si elle est de travers ou tournée vers l'arrière, elle fait un demi-tour en plusieurs temps : marche avant, marche arrière, roues braquées à fond.
- **RG-01, retrait** :
  - Pas remise en route en 8 s, elle est retirée, mais seulement hors de la vue : à plus de 150 m devant, ou à plus de 15 m derrière (donc derrière la caméra).
  - Elle est alors replacée loin devant (`placerLoin`).
- **RG-03, contacts** :
  - Contacts entre voitures du trafic (`contacts`), avec la voiture du joueur traitée comme fixe : plus aucune carrosserie dans une autre.
  - `separerDe` écarte le trafic de la police. La police n'est jamais déplacée.
- **RG-02, bouchons** :
  - Une voiture accidentée ne retient que les voitures dont elle occupe vraiment le couloir. Les autres passent à côté.
  - **Revue 1** : une voiture ne se déplace plus de côté sans avancer. Le déplacement de côté est limité à v · tan 35° : à l'arrêt, il est nul ; au-dessus de 19 km/h, rien ne change par rapport à avant.
  - **Revue 1** : derrière une voiture accidentée, une voiture qui a décidé de changer de file déboîte au pas, à 7 km/h, en braquant. Elle garde sa décision : les contacts ne l'annulent plus.

**`src/game/traffic_system.cpp`**

- Une voiture du trafic qui contient la caméra n'est pas dessinée.
- Nouveau banc `trafic-541`.

**Accroches annoncées au journal**

- `src/game/poursuite_system.cpp` : 2 lignes (police et trafic).
- `src/platform/desktop_main.cpp` : 1 ligne (groupe du banc `trafic-541`).

**Tests**

- Nouveau `tests/test_trafic_chocs541.cpp`, avec le cas D ajouté à la revue 1 : changement de file au pas derrière une voiture accidentée.
- `tests/test_penetration_voitures.cpp` passe de 6 à 12 graines. Les voitures heurtées repartent : avec 6 graines, il n'y avait plus que 44 contacts pour la borne de 50. Aucune borne n'est changée.

### Comment c'est vérifié

**Tests CPU** : 146 / 146, sur la branche synchronisée avec `main`. Journal complet dans `qa/pr/WFS-541/tests-cpu.log`, résumé dans la capture « Tests CPU ».

**Banc CPU du critère** : Corniche, trafic de la Poursuite (× 2,5), 200 chocs provoqués autour du joueur.

| Mesure | Résultat |
|---|---|
| Pénétration trafic / trafic | 0,000 m |
| Pénétration trafic / joueur | 0,000 m |
| Plus long arrêt d'une voiture du trafic (hors feu rouge) | 7,6 s |

Carambolage (un choc toutes les 0,4 s) : pénétration max 0,000 m.

**Cas D, changement de file au pas** (voiture accidentée en travers de chacune des 3 files) :

| Mesure | Résultat |
|---|---|
| Déplacement de côté à l'arrêt | 0,000 m |
| Arrêt le plus long de la voiture qui suit | 1,5 s |
| Cap le plus fort pendant le déboîtement | 35° |
| Issue | elle passe à côté, dans les 3 cas |

**Voiture heurtée, joueur arrêté à côté** :

| Mesure | Résultat |
|---|---|
| Temps arrêtée après le choc | 1,2 s au plus |
| Remise en route, petit choc | 5,7 s |
| Remise en route, voiture tournée de 90° à 180° | 8,2 à 10 s |

**RG-05, coût CPU** (trafic + chocs, par image) :

| | Coût |
|---|---|
| `main` | 0,069 ms |
| Branche | 0,101 ms |
| Écart | +0,03 ms |

La borne est de 0,5 ms.

**Trafic sans choc** : les tests suivants sont inchangés et verts :

- références du jeu web : `test_traffic`, `test_course` ;
- anneaux des ronds-points : `test_trace` ;
- feux tricolores : 0 voiture passée au rouge ;
- densité dans la durée : `test_trafic_duree`, RG-02 de WFS-281, 0 fenêtre hors règle sur 1600.

**Captures** : banc `trafic-541`, Poursuite, Corniche, nuit, en 1280x720, avec le rendu lavapipe (le même que la CI).

- **« Avant »** : `main` n'a pas ce banc, donc `captures_pr.sh` est lancé avec `SANS_AVANT=1`. Les images « avant » viennent du même banc avec `WFS_AVANT_541=1`, qui remet le comportement du trafic de `main` (même méthode que WFS-333).
- **`heurtee-00…13`** (vue Proche, 0,75 s entre deux images) :
  - avant : la voiture heurtée reste en travers 10,5 s ;
  - après : elle recule, braque, se remet dans sa file et repart.
  - La voiture est une M4 du trafic, avec son vrai modèle. Il n'y a pas de SUV possible dans le trafic : le trafic ne charge que 4 modèles (golf, m4, p911, gallardo, WFS-13), et un « rrsport » était dessiné avec la M4 étirée. Le SUV des vidéos de l'utilisateur est le camion de la Poursuite, traité par WFS-542.
- **`bouchon-00…09`** (caméra haute, 0,8 s entre deux images) :
  - après : les 2 voitures derrière la voiture accidentée déboîtent au pas, à 7 km/h, caisse tournée de 35°, chacune vers un seul côté ;
  - à 4,8 s, elles roulent à 25 km/h dans les files voisines.
- **`pres-00…07`** (vue Proche, 12 km/h) :
  - avant : la voiture blanche en glissade entre dans la grise (0,81 m) ;
  - après : elle s'arrête contre la grise, puis se redresse.

### Limites et risques connus

- Ticket de conduite : **🟡 À valider (utilisateur)** sur l'APK de `main`.
- Les deux-roues couchés ne se redressent pas : il n'existe pas d'animation du pilote qui se relève. Ils sont relevés hors de la vue, comme avant, et les voitures passent maintenant à côté d'eux.
- Une voiture tournée de plus de 90° peut mettre jusqu'à 10 s à reprendre sa file. Si elle n'a pas fini, RG-01 s'applique : elle est retirée dès qu'elle sort de la vue.
- La police reste du ressort de WFS-540 : `poursuite.cpp` n'est pas touché.
- Une voiture du trafic qui contient la caméra disparaît d'un coup, sans fondu.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv1WUtoyPMiBqhjFrvU

## Captures

Tableau complet (32 paires avant / après et la capture « Tests CPU ») : [`qa/pr/WFS-541/README.md`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-541/qa/pr/WFS-541/README.md).
