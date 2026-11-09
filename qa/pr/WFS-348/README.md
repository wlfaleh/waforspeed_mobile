# WFS-348 — captures avant / après (agent arwa (Claude, session trafic))

Banc : `WFS_STATS_PIPELINE=1 WFS_BANC=perf WFS_PERF_SCENE=1 WFS_PERF_IMAGES=10 WFS_TAILLE=2340x1080` (lavapipe).
Scène de la mesure de l'utilisateur (`qa/retours/2026-11-09-derniere-apk-scene-16-img-s.jpg`) : Ville de nuit sous la pluie,
Checkpoint, vue Arrière, « Rue des Mimosas » 60 m avant le virage serré à droite (checkpoint 8). Basse dans l'état du téléphone
(scène à 60 %, palier 2 : ombres, reflets et captures coupés). « depart » : départ de la Ville (même réglages).

- avant : `main` (c333b0b2) avec le même banc, sans le changement de shader ;
- après : branche `feature/waforspeed-348` (reflet du ciel jeté plus calculé dans `scene.frag` / `scene_skin.frag`).

| scène | avant | après | pixels différents |
|---|---|---|---|
| Rue des Mimosas, Basse | avant-scene-mimosas-basse.jpg | apres-scene-mimosas-basse.jpg | 0 |
| Rue des Mimosas, Haute | avant-scene-mimosas-haute.jpg | apres-scene-mimosas-haute.jpg | 0 |
| Départ, Basse | avant-scene-depart-basse.jpg | apres-scene-depart-basse.jpg | 0 |
| Départ, Haute | avant-scene-depart-haute.jpg | apres-scene-depart-haute.jpg | 0 |

Comparaison faite sur les PNG en pleine définition (2340 × 1080) : 0 pixel différent, écart maximal 0, dans les 4 scènes.
