## Agent : Claude, agent 2 (01UY1Vv5)

**Ticket** : WFS-282 — règles : `regles-de-gestion/WFS-282.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : « règle la vue cockpit, remets le tableau de bord et la carcasse de voiture » (WFS-235 refusé, ANO-152) ; 2e partie du ticket (RG-01 à RG-05, après RG-00 + RG-06 fusionnés en #244), avec les corrections demandées par le lead dev au 1er passage de cette PR.
- **Ce qui change** :
  - `src/game/camera_vue.h`, `.cpp` (partie cockpit) — **RG-02 regard plus doux** : la tête suit 40 % de l'angle du virage (avant 60 %), ±7° au plus (avant ±12°), seulement au-dessus de 20 km/h (plein suivi à 30 km/h), point visé à 20-40 m (avant 12-40 m), lissage 1,5 /s (avant 2,5 /s) ; à l'arrêt, regard droit devant ; vitesse toujours passée à `lacetVirage` (plus de valeur par défaut). Tête : 2 cm / 1,5 cm au plus (avant 3 / 2 cm), coupée par « Réduire les mouvements ». **RG-01** gardé : œil sans retard, réglage « Regard cockpit ».
  - **RG-03** : œil de la caméra 4 cm au-dessus de l'œil du siège (maximum permis), intérieur dans le repère du siège ; regard Normal à −3° (avant −3,5°) pour tenir l'arbitrage du PO (horizon 45-47 %, intérieur ≤ 40 %). **Rétroviseur** : glace claire et lisse (albédo 0,35, rugosité 0,12) dans un cadre noir de 7 mm : claire le jour, sombre la nuit (sans lumière d'ambiance). Au 1er passage, la glace en métal ne reflétait presque rien dans l'habitacle (presque noire le jour) et recevait la lumière d'ambiance la nuit (plaque claire).
  - `src/render/vehicule_cockpit.cpp`, `.h` — **RG-04 nuit** : la nuit, chaque matière de l'habitacle reçoit une petite lumière d'ambiance (émission = 25 % de son albédo, un tiers pour les métaux), sauf les écrans (déjà lumineux) et la glace du rétroviseur. Copies de nuit des matériaux : même jeu de descripteurs, aucune ressource en plus. Rien dans `nuit.cpp`.
  - Code mort retiré : `fovHorizontalCockpit`, `fovVerticalDepuisHorizontal`, `COCKPIT_FOV_H`.
  - Banc `cockpit` : mesures « toute l'image » et « horizon » ; capture **départ de nuit sur la Plage** ; gros plans du rétroviseur de la Golf (ci-dessous).
  - `tests/test_regard_cockpit.cpp` : nouvelles valeurs de RG-02.
- **Comment c'est vérifié** : 64 tests CPU verts ; branche synchronisée avec `main` (arbitrage du PO b31ea776 et WFS-245 compris) ; banc `cockpit` en **2340 × 1080**, Golf / SF90 / X5, avant (main) / après (branche), même run. Mesures (arbitrage du PO du 2026-11-08 01:14:55 UTC dans `regles-de-gestion/WFS-282.md`) :
  - **Fenêtre de la rue devant (±15°, jusqu'à l'horizon + 8°) : 0,0 % caché sur toutes les captures, avant et après, jour et nuit** (RG-05).
  - Bande de la rue (toute la largeur, plafond 12 %), regard Normal, avant → après : Golf jour 11,1 → 12,0, virage 9,5 → 11,5 ; SF90 jour 10,6 → 11,4, virage 8,2 → 10,5 ; X5 jour 11,0 → 11,8, virage 9,2 → 11,5 ; Plage de nuit 10,1 à 10,6. Cause de la hausse : RG-02 (montant gauche au bord de l'image) ; preuve : paires `cockpit-x5-virage` et `cockpit-golf-virage`.
  - Intérieur dans toute l'image, regard Normal (≤ 40 %) : Golf 37,6-39,1 %, X5 34,6-36,1 %, SF90 jour / nuit / Plage 39,8-39,9 %. Horizon (45-47 %) : 46,4 à 46,8 %.
- **Limites et risques connus** (écarts écrits honnêtement, au lead dev de juger) :
  - **SF90 en virage, regard Normal : intérieur 40,8-40,9 %** (cible ≤ 40 %) : planche haute de la SF90 (supercar, mesures WFS-197). Regarder plus haut ferait sortir l'horizon de 45-47 % (46,8 % en virage).
  - **Regard Haut** (mesures du banc, pas des captures) : intérieur Golf 34,9 %, X5 32,1 %, **SF90 36,9 %** (cible ≤ 35 %) ; bande de la rue **Golf 12,8 %, X5 12,7 %** (plafond 12 %). En regard Haut, la caméra vise l'horizon et le haut des montants entre dans la bande.
  - Montants : la base du montant gauche est à ≈ 22-25° du centre (géométrie d'avant WFS-235, remise par RG-00), pas au-delà de 30°.
  - La capture du départ sur la Plage n'a pas d'« avant » : référence `qa/retours/2026-11-07-cockpit-nuit-plage.jpg` (capture de l'utilisateur).
  - Cockpit commun (Corsa) : regard plus doux et œil +4 cm aussi ; pas de lumière de nuit.

### Gros plan du rétroviseur (Golf)

| | Avant (main) | Après (branche) |
|---|---|---|
| Jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-gros-plan-retroviseur-golf-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-gros-plan-retroviseur-golf-jour.jpg?raw=true) |
| Nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-gros-plan-retroviseur-golf-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-gros-plan-retroviseur-golf-nuit.jpg?raw=true) |

## Captures pour le lead dev (24 après)

Dossier : [`qa/pr/WFS-282`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `cockpit-golf-braquage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-golf-braquage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-braquage.jpg?raw=true) |
| `cockpit-golf-carte` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-golf-carte.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-carte.jpg?raw=true) |
| `cockpit-golf-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-golf-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-jour.jpg?raw=true) |
| `cockpit-golf-nuit-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-golf-nuit-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-nuit-virage.jpg?raw=true) |
| `cockpit-golf-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-golf-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-nuit.jpg?raw=true) |
| `cockpit-golf-plage-nuit-depart` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-plage-nuit-depart.jpg?raw=true) |
| `cockpit-golf-pluie` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-golf-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-pluie.jpg?raw=true) |
| `cockpit-golf-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-golf-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-virage.jpg?raw=true) |
| `cockpit-sf90-braquage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-sf90-braquage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-sf90-braquage.jpg?raw=true) |
| `cockpit-sf90-carte` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-sf90-carte.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-sf90-carte.jpg?raw=true) |
| `cockpit-sf90-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-sf90-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-sf90-jour.jpg?raw=true) |
| `cockpit-sf90-nuit-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-sf90-nuit-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-sf90-nuit-virage.jpg?raw=true) |
| `cockpit-sf90-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-sf90-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-sf90-nuit.jpg?raw=true) |
| `cockpit-sf90-plage-nuit-depart` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-sf90-plage-nuit-depart.jpg?raw=true) |
| `cockpit-sf90-pluie` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-sf90-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-sf90-pluie.jpg?raw=true) |
| `cockpit-sf90-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-sf90-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-sf90-virage.jpg?raw=true) |
| `cockpit-x5-braquage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-x5-braquage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-x5-braquage.jpg?raw=true) |
| `cockpit-x5-carte` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-x5-carte.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-x5-carte.jpg?raw=true) |
| `cockpit-x5-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-x5-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-x5-jour.jpg?raw=true) |
| `cockpit-x5-nuit-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-x5-nuit-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-x5-nuit-virage.jpg?raw=true) |
| `cockpit-x5-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-x5-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-x5-nuit.jpg?raw=true) |
| `cockpit-x5-plage-nuit-depart` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-x5-plage-nuit-depart.jpg?raw=true) |
| `cockpit-x5-pluie` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-x5-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-x5-pluie.jpg?raw=true) |
| `cockpit-x5-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-x5-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-x5-virage.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv5pG5V2148UwYk53fJ
