## Agent : Claude, agent dev (01UY1Vv3Vj)

**Ticket** : WFS-429 — règles : `regles-de-gestion/WFS-429.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : dans la course, mettre les boutons « Menu » et « Caméra » à côté du score, en haut à droite, dans l'ordre [MENU] [CAMÉRA] [SCORE], et les agrandir de 5 %.
- **Ce qui change** :
  - `src/ui/hud.h` / `hud.cpp` : constantes du panneau de score (`SCORE_L`, `SCORE_H`, gouttières) et des boutons (`BOUTON_ECHELLE = 1.05` : seul réglage du +5 %, RG-02) ; `hud::rangeeBoutons` calcule les deux rectangles : rangée centrée sur la hauteur du panneau de score (70 px), écarts de 8 px, à gauche du score, dans la zone sûre (RG-01). En vue Cockpit, le panneau de vitesse passe sous le panneau du temps (sa place à côté du bouton Menu n'existe plus, RG-05).
  - `src/ui/ecrans/pause.cpp` (bouton Menu) et `src/game/camera_vue.cpp` (bouton Caméra) dessinent et reçoivent le toucher sur ces rectangles (RG-03) ; même style (biseau, verre, icône, texte), tailles × 1,05 (hauteur 40 → 42 px à 720 p).
  - Poursuite : même placement (RG-06). Gaucher : seules les commandes de conduite sont inversées, le score reste à droite, donc la rangée aussi (RG-07).
  - `src/platform/desktop_main.cpp` : accroche d'1 ligne `WFS_BANC=pause` (banc du bouton Menu et de la pause seul, annoncé au journal).
  - `tests/test_hud_boutons.cpp` (nouveau, déclaré dans `CMakeLists.txt`).
- **Comment c'est vérifié** :
  - `test_hud_boutons` : mesures × 1,05 (RG-02), ordre, écarts de 8 px et centrage sur le score (RG-01), zone sûre (encoche 90 px), aucun chevauchement avec le GPS / la mini-carte (RG-04), en 2400×1080, 1600×720, 2340×1080, gaucher, Poursuite.
  - Tests CPU : 109 réussis après synchronisation avec `main`.
  - Captures : bancs `hud`, `pause`, `course5` en 1600×720, avant / après (ci-dessous) : boutons à gauche du score, rien ne se chevauche (combo, notifications, GPS, Nitro), Cockpit jour et nuit, Poursuite ; le toucher du bouton Menu ouvre la pause, puis Reprendre, Recommencer, Quitter.
- **Limites et risques connus** :
  - Banc `pause` neuf : pas de capture « avant » (le banc n'existe pas sur `main`, « groupe inconnu »).
  - Lancé seul, le banc `pause` écrit `[ERREUR] pipelines (WFS-262 RG-02) : 2 pipeline(s) créé(s) pendant la course` (aucune course chauffée avant, banc isolé) ; rien dans les bancs `hud` et `course5`. L'erreur `éclairage (WFS-213)` de `course5` est la même avant et après.
  - Captures « après » faites juste avant la dernière synchronisation avec `main` (arrivées : rétroviseurs WFS-324, mémoire WFS-418 ; aucune ne touche le HUD) ; tests CPU relancés après.
  - Pas d'essai sur téléphone : à vérifier par l'utilisateur sur l'APK de `main`.

## Captures pour le lead dev (choix ; les 107 « après » et leurs « avant » sont dans le dossier)

Dossier : [`qa/pr/WFS-429`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429) — produit par `tools/qa/captures_pr.sh WFS-429 hud:1600x720 pause:1600x720 course5:1600x720` (liste complète : `README.md`).

| Capture | Quoi regarder | Avant (main) | Après (branche) |
|---|---|---|---|
| `hud-course` | course, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/avant-hud-course.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-hud-course.jpg?raw=true) |
| `hud-combo` | combo + notifications (RG-04) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/avant-hud-combo.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-hud-combo.jpg?raw=true) |
| `hud-solo-nuit` | course solo, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/avant-hud-solo-nuit.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-hud-solo-nuit.jpg?raw=true) |
| `hud-cockpit-jour` | Cockpit, jour (RG-05 : vitesse sous le temps) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/avant-hud-cockpit-jour.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-hud-cockpit-jour.jpg?raw=true) |
| `hud-cockpit-nuit` | Cockpit, nuit (RG-05) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/avant-hud-cockpit-nuit.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-hud-cockpit-nuit.jpg?raw=true) |
| `hud-nitro` | Nitro (RG-04) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/avant-hud-nitro.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-hud-nitro.jpg?raw=true) |
| `encoche-hud` | encoche 90 px (zone sûre) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/avant-encoche-hud.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-encoche-hud.jpg?raw=true) |
| `toast-pile` | pile de notifications (RG-04) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/avant-toast-pile.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-toast-pile.jpg?raw=true) |
| `minicarte-ville-jour` | GPS sous le score (RG-04) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/avant-minicarte-ville-jour.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-minicarte-ville-jour.jpg?raw=true) |
| `poursuite-hud` | Poursuite (RG-06) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/avant-poursuite-hud.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-poursuite-hud.jpg?raw=true) |
| `poursuite-police` | Poursuite, police | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/avant-poursuite-police.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-poursuite-police.jpg?raw=true) |
| `pause-bouton` | toucher du bouton Menu à sa nouvelle place | — (banc `pause` absent de main) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-pause-bouton.jpg?raw=true) |
| `pause-0` | pause ouverte | — (banc `pause` absent de main) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-pause-0.jpg?raw=true) |
| `pause-reprise` | Reprendre | — (banc `pause` absent de main) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-pause-reprise.jpg?raw=true) |
| `pause-recommencer` | Recommencer | — (banc `pause` absent de main) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-pause-recommencer.jpg?raw=true) |
| `pause-quitter` | Quitter | — (banc `pause` absent de main) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-429/qa/pr/WFS-429/apres-pause-quitter.jpg?raw=true) |
