Agent : agent arwa (Claude, session trafic)

**Ticket** : WFS-353 [TECHNIQUE] — règles : `regles-de-gestion/WFS-353.md`. Partie de `feature/waforspeed-348` (PR #300, fusionnée depuis) puis synchronisée avec `main` (4d8ba694).

## Demande (en une phrase)
Ville de nuit sous la pluie en Basse plus fluide (objectif ≥ 30 img/s sur le téléphone) : 2 lumières par pixel en Basse (accord E13 du PO) et résolution plancher de la Basse à 45 % (accord de l'utilisateur, 09:14:26 UTC), Moyenne et Haute inchangées.

## Ce qui change
- **RG-03 (fait)** — `src/render/qualite_auto.h/.cpp` : plancher de la Basse 45 % (`ECHELLE_MIN_BASSE`), atteint seulement quand la scène est déjà à 60 %, au palier 2 (effets coupés) et toujours sous 30 img/s ; retour à 60 % d'un coup dès que la marge revient (règle de marge de WFS-246, ou ≥ 50 img/s à 45 %), avant tout retour des effets. Basse par rétrogradation (netteté retenue de la Moyenne), Moyenne et Haute : 60 %, inchangé.
- **RG-01 (variante du banc, décision du PO attendue)** — `shaders/eclairage.glsl` : `spotLights2` (spotN.y = 2) : les 2 lumières les plus fortes au point (éclairement reçu × luminance) en calcul complet, les autres approchées (diffus, coupure du feu de croisement approchée), aucune ne disparaît. **Le jeu garde 4 lumières en Basse** (`LUMIERES_PAR_PIXEL_BASSE = 4`) : au banc la variante coûte **plus** (+ 4 à + 5 %) et retire le reflet de la 3e lumière sur la chaussée mouillée dans le faisceau des phares (capture `variante-2-lumieres-zoom-mimosas.jpg`). Lancée seulement par `WFS_PERF_SCENE_LEVIERS=1`.
- `src/render/renderer.cpp` (accroche 2 lignes) : valeur de la Basse lue dans `qualite_auto.h`.
- `src/render/bench_perf.cpp` : leviers WFS-353, `WFS_PERF_SCENE_CAPTURES=1` (capture de chaque variante), `WFS_PERF_SCENE_SUITE=1` (3 captures successives en roulant).
- `tests/test_qualite.cpp` : plancher 45 % (seulement à 60 %, palier 2, sous 30 img/s ; gardé à 35 et 45 img/s ; retour à 60 % à 52 img/s ou GPU 9 ms ; Moyenne / Haute / rétrogradation 60 %), choix des 2 lumières (les plus fortes, tout ordre de liste, égalités), atan2 approché (≤ 0,0015 rad), coupure approchée du feu de croisement (écart 0,0008 de la crête).

## Vérifications (résultats)
- Tests CPU : **87 réussis** (`tools/qa/tests_cpu.sh`, après synchronisation). `test_audio_chocs` (mesure de temps ≤ 0,05 ms) a échoué une fois pendant qu'un banc tournait, réussi ensuite.
- Banc `WFS_BANC=perf WFS_PERF_SCENE=1 WFS_PERF_SCENE_LEVIERS=1 WFS_STATS_PIPELINE=1`, Basse 2340 × 1080, 20 images, temps processeur du banc (ms par image) :

| variante | Rue des Mimosas | écart | Départ | écart |
|---|---|---|---|---|
| Basse du jeu, scène 60 % (4 lumières) | 1 208 | — | 1 738 | — |
| **scène à 45 % (RG-03)** | **984** | **− 19 %** | **1 250** | **− 28 %** |
| 2 lumières approchées (RG-01) | 1 262 | + 4 % | 1 825 | + 5 % |
| 2 lumières approchées, 45 % | 963 | − 20 % | 1 355 | − 22 % |
| scène à 50 % | 1 032 | − 15 % | 1 447 | − 17 % |
| sans lumières de nuit (repère) | 1 076 | − 11 % | 1 515 | − 13 % |

  GPU scène (lavapipe) Rue des Mimosas : 310-316 ms → 224 ms à 45 % (− 28 %), pixels calculés 2 470 k → 1 434 k (− 42 %).
- Captures : **Haute et Basse 60 % identiques au pixel près** à la base (#300) : 0 pixel différent sur les 4 PNG 2340 × 1080 (Mimosas et départ). 3 captures successives en roulant : lampadaires et flaques de lumière présents, aucun saut.
- Parcours complet (`WFS_BANC=parcours`) : **0 écart**, vue Cockpit 0 écart.
- CI de branche (US technique) : run **955** (`workflow_dispatch`, 4d8ba694).

## Limites et risques
- **RG-04 non atteint au calcul** : 45 % donne au banc − 19 à − 28 % ; estimation téléphone (part par pixel ≈ 72 %, pixels − 42 %) : 55,8 → ≈ 39 ms, **≈ 26 img/s**, pas 30. Validation sur téléphone par l'utilisateur. Poste suivant : sol et trottoirs (− 32 % au banc de WFS-348), ticket proposé au PO.
- RG-01 non activé dans le jeu (aucun gain mesuré, image moins fidèle) : décision du PO demandée au journal.
- Netteté de la voiture en Basse à 45 % (dérogation à WFS-134 acceptée par l'utilisateur), seulement sous 30 img/s.

## Captures pour le lead dev
Dossier : [`qa/pr/WFS-353`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-353/qa/pr/WFS-353) (README.md).

| Capture | Avant (#300) | Après (branche) |
|---|---|---|
| `scene-mimosas-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-353/qa/pr/WFS-353/avant-scene-mimosas-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-353/qa/pr/WFS-353/apres-scene-mimosas-basse.jpg?raw=true) |
| `scene-mimosas-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-353/qa/pr/WFS-353/avant-scene-mimosas-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-353/qa/pr/WFS-353/apres-scene-mimosas-haute.jpg?raw=true) |
| `scene-depart-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-353/qa/pr/WFS-353/avant-scene-depart-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-353/qa/pr/WFS-353/apres-scene-depart-basse.jpg?raw=true) |
| `scene-depart-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-353/qa/pr/WFS-353/avant-scene-depart-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-353/qa/pr/WFS-353/apres-scene-depart-haute.jpg?raw=true) |
| `scene-mimosas-basse-45` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-353/qa/pr/WFS-353/apres-scene-mimosas-basse-45.jpg?raw=true) |
| `scene-depart-basse-45` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-353/qa/pr/WFS-353/apres-scene-depart-basse-45.jpg?raw=true) |
| `scene-mimosas-basse-suite-0` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-353/qa/pr/WFS-353/apres-scene-mimosas-basse-suite-0.jpg?raw=true) |
| `scene-mimosas-basse-suite-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-353/qa/pr/WFS-353/apres-scene-mimosas-basse-suite-1.jpg?raw=true) |
| `scene-mimosas-basse-suite-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-353/qa/pr/WFS-353/apres-scene-mimosas-basse-suite-2.jpg?raw=true) |
| `variante 2 lumières (banc)` | — | ![variante](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-353/qa/pr/WFS-353/variante-2-lumieres-zoom-mimosas.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01Us6uCwMdPUDf6HbGK9u3Gx
