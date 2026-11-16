## Agent : Claude, agent dev3d (01Us6uCxBL)

**Ticket** : WFS-561 — règles : `regles-de-gestion/WFS-561.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : l'écran des résultats respecte la maquette R19 11 (parking, tableau LED ambre des pilotes, reflet, voiture du joueur).
- **Ce qui change** (fichiers, comportement, à l'écran) : fins de la course à 5, du duel et de la Poursuite dans la scène 3D (`src/ui/resultats_parking.*`, `tableau_led.*`, accroches des 3 modes) ; fenêtres 2D gardées en repli.
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) : 154 tests CPU (`tests-cpu.log`), bancs `resultats3d` et `resultats3d-modes` (vraies courses, FR / EN, repli) : 0 écart.
- **Limites et risques connus** : WFS-557 non fusionné (mêmes fichiers, « Menu » en autocollant) ; pas de reflet de la voiture au sol ; tableau à 25 % de la largeur (vraies mesures).

## Captures pour le lead dev (9 après)

Dossier : [`qa/pr/WFS-561`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-561/qa/pr/WFS-561) — produit par `tools/qa/captures_pr.sh`.

Captures « avant » : aucune (SANS_AVANT=1).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `resultats3d-modes-en-course5-1er-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-561/qa/pr/WFS-561/apres-resultats3d-modes-en-course5-1er-2400x1080.jpg?raw=true) |
| `resultats3d-modes-en-duel-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-561/qa/pr/WFS-561/apres-resultats3d-modes-en-duel-2400x1080.jpg?raw=true) |
| `resultats3d-modes-en-poursuite-echappe-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-561/qa/pr/WFS-561/apres-resultats3d-modes-en-poursuite-echappe-2400x1080.jpg?raw=true) |
| `resultats3d-modes-fr-course5-1er-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-561/qa/pr/WFS-561/apres-resultats3d-modes-fr-course5-1er-2400x1080.jpg?raw=true) |
| `resultats3d-modes-fr-course5-3e-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-561/qa/pr/WFS-561/apres-resultats3d-modes-fr-course5-3e-2400x1080.jpg?raw=true) |
| `resultats3d-modes-fr-duel-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-561/qa/pr/WFS-561/apres-resultats3d-modes-fr-duel-2400x1080.jpg?raw=true) |
| `resultats3d-modes-fr-poursuite-arrete-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-561/qa/pr/WFS-561/apres-resultats3d-modes-fr-poursuite-arrete-2400x1080.jpg?raw=true) |
| `resultats3d-modes-fr-poursuite-echappe-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-561/qa/pr/WFS-561/apres-resultats3d-modes-fr-poursuite-echappe-2400x1080.jpg?raw=true) |
| `resultats3d-modes-fr-repli-course5-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-561/qa/pr/WFS-561/apres-resultats3d-modes-fr-repli-course5-2400x1080.jpg?raw=true) |
