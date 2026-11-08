Agent : Claude, agent 2 (01UY1Vv5)

**Ticket** : WFS-296 — règles : `regles-de-gestion/WFS-296.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : vidéo du 2026-11-08 (`qa/retours/2026-11-08-video-volant-planche.jpg`) : le frein enclenche la marche arrière avec seulement un petit « R » au compteur, et en marche arrière le volant agit à l'envers, ce qui donne l'impression que « ça ne tourne pas » ; le PO demande un signal clair de marche arrière (WFS-296).
- **Ce qui change** : `src/ui/hud.cpp` seulement (zone E5, HUD).
  - **RG-01** : en marche arrière, une **pastille rouge inclinée** (`--danger` #FF1F4B, biais de 10° comme les boutons du jeu, halo néon) avec un **grand « R » noir** (#0A0B10 sur le rouge : contraste 5,4 : 1). Vue normale : 64 px (référence 720) à gauche du compteur, à mi-hauteur (au-dessus, elle cachait la voiture du joueur). Vue Cockpit : à droite du panneau de vitesse, même hauteur (40 px). Le « R · km/h » existant reste. Rien en marche avant.
  - **RG-02** : aucun changement de la conduite (même condition qu'avant pour le « R » : `speed < −1` ou `reversing`).
  - **RG-04 du projet** : aucun bouton ne change de taille ni de place (comparer FREIN, GAZ, NITRO et le volant sur les paires).
  - Banc `hud` : 4 captures de marche arrière (frein maintenu à l'arrêt après le décompte, comme le joueur) : vue normale et Cockpit, jour et nuit, avec contrôle « la voiture recule » (sinon erreur au journal).
- **Comment c'est vérifié** : 71 tests CPU verts ; parcours complet 0 écart (et vue Cockpit 0 écart) ; banc `hud` en **2340 × 1080** avant (main) / après (branche) : 14 captures, dont marche avant (solo, Course à 5, Cockpit, jour et nuit) et marche arrière (4) ; les 4 captures de marche arrière ont un « avant » fait avec main + la seule partie banc de cette PR (compilée à part, sans la pastille). Les 4 reculent bien (−31 km/h).
- **Limites et risques connus** :
  - Le volant reste inversé en marche arrière (comportement de conduite, RG-02 : hors de ce ticket) ; la pastille dit seulement clairement qu'on recule.
  - Rouge = couleur réservée au danger (charte) : choisi exprès, la marche arrière en course est une situation à signaler ; l'info ne passe pas que par la couleur (grand « R »).

## Captures pour le lead dev (14 après)

Dossier : [`qa/pr/WFS-296`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `hud-arc-120` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/avant-hud-arc-120.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/apres-hud-arc-120.jpg?raw=true) |
| `hud-arc-320` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/avant-hud-arc-320.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/apres-hud-arc-320.jpg?raw=true) |
| `hud-arriere-cockpit-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/avant-hud-arriere-cockpit-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/apres-hud-arriere-cockpit-jour.jpg?raw=true) |
| `hud-arriere-cockpit-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/avant-hud-arriere-cockpit-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/apres-hud-arriere-cockpit-nuit.jpg?raw=true) |
| `hud-arriere-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/avant-hud-arriere-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/apres-hud-arriere-jour.jpg?raw=true) |
| `hud-arriere-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/avant-hud-arriere-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/apres-hud-arriere-nuit.jpg?raw=true) |
| `hud-cockpit-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/avant-hud-cockpit-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/apres-hud-cockpit-jour.jpg?raw=true) |
| `hud-cockpit-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/avant-hud-cockpit-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/apres-hud-cockpit-nuit.jpg?raw=true) |
| `hud-combo` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/avant-hud-combo.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/apres-hud-combo.jpg?raw=true) |
| `hud-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/avant-hud-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/apres-hud-course.jpg?raw=true) |
| `hud-course5-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/avant-hud-course5-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/apres-hud-course5-jour.jpg?raw=true) |
| `hud-course5-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/avant-hud-course5-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/apres-hud-course5-nuit.jpg?raw=true) |
| `hud-solo-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/avant-hud-solo-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/apres-hud-solo-jour.jpg?raw=true) |
| `hud-solo-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/avant-hud-solo-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-296/qa/pr/WFS-296/apres-hud-solo-nuit.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv5pG5V2148UwYk53fJ
