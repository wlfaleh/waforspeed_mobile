# WFS-353 — captures avant / après (agent arwa (Claude, session trafic))

Banc : `WFS_STATS_PIPELINE=1 WFS_BANC=perf WFS_PERF_SCENE=1 WFS_PERF_IMAGES=3 WFS_TAILLE=2340x1080` (lavapipe), scène de WFS-348 :
Ville de nuit sous la pluie, Checkpoint, vue Arrière, « Rue des Mimosas » 60 m avant le virage serré à droite, et départ de la Ville.
Basse dans l'état du téléphone (scène à 60 %, palier 2).

- avant : `feature/waforspeed-348` (3a6c1e1c, PR #300, base de cette branche) ;
- après : `feature/waforspeed-353`.

| scène | avant | après | pixels différents (PNG 2340 × 1080) |
|---|---|---|---|
| Rue des Mimosas, Basse 60 % | avant-scene-mimosas-basse.jpg | apres-scene-mimosas-basse.jpg | 0 |
| Rue des Mimosas, Haute | avant-scene-mimosas-haute.jpg | apres-scene-mimosas-haute.jpg | 0 |
| Départ, Basse 60 % | avant-scene-depart-basse.jpg | apres-scene-depart-basse.jpg | 0 |
| Départ, Haute | avant-scene-depart-haute.jpg | apres-scene-depart-haute.jpg | 0 |

RG-03 (plancher 45 %, seulement sous 30 img/s au palier 2) : `apres-scene-mimosas-basse-45.jpg`, `apres-scene-depart-basse-45.jpg`
(`WFS_PERF_SCENE_LEVIERS=1 WFS_PERF_SCENE_CAPTURES=1`, même image à 45 %).

RG-02, 3 captures successives en roulant (0,25 s d'écart, `WFS_PERF_SCENE_SUITE=1`, Basse) : `apres-scene-mimosas-basse-suite-0/1/2.jpg`,
`apres-scene-depart-basse-suite-0/1/2.jpg` : lampadaires et flaques de lumière présents sur les 3, aucun saut.

Variante RG-01 « 2 lumières par pixel » (banc seulement, décision du PO attendue) : `variante-scene-*-basse-2-lumieres.jpg` et
`variante-2-lumieres-zoom-mimosas.jpg` (gauche : Basse du jeu, 4 lumières ; milieu : 2 lumières approchées ; droite : écart × 4) :
dans le faisceau des phares, le reflet orangé du lampadaire sur la chaussée mouillée disparaît (3e lumière approchée sans reflet).
