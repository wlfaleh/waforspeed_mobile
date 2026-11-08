Agent : Claude, agent 2 (01UY1Vv5)

**Ticket** : WFS-282 — règles : `regles-de-gestion/WFS-282.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : « règle la vue cockpit, remets le tableau de bord et la carcasse de voiture » (WFS-235 refusé, ANO-152) ; 2e partie du ticket (RG-01 à RG-05, après RG-00 + RG-06 fusionnés en #244), avec les corrections des deux refus du lead dev sur cette PR.
- **Ce qui change** :
  - `src/game/camera_vue.h`, `.cpp` (partie cockpit) — **RG-02 regard plus doux** : 40 % de l'angle du virage (avant 60 %), ±7° au plus (avant ±12°), seulement au-dessus de 20 km/h (plein suivi à 30 km/h), visée à 20-40 m (avant 12-40 m), lissage 1,5 /s (avant 2,5 /s) ; à l'arrêt, regard droit devant ; vitesse toujours passée à `lacetVirage`. Tête : 2 cm / 1,5 cm au plus, coupée par « Réduire les mouvements ». **RG-01** gardé : œil sans retard, réglage « Regard cockpit ».
  - **RG-03** : œil de la caméra 4 cm au-dessus de l'œil du siège (maximum permis), intérieur dans le repère du siège ; regard Normal à −3° (avant −3,5°).
  - **Rétroviseur** (`vehicule_cockpit.cpp`) : de jour, glace claire et lisse (albédo 0,35) dans un cadre noir de 7 mm ; **de nuit, glace en position nuit** : réflexion 0,5 %, lisse, sans diffus ni lumière d'ambiance (métal, rugosité et couleur passent au dessin : aucune ressource en plus). Au 2e refus, elle était encore plus claire que le ciel sur la Plage.
  - **RG-04 nuit** : la nuit, chaque matière de l'habitacle reçoit une petite lumière d'ambiance (émission = 25 % de son albédo, un tiers pour les métaux), sauf les écrans (déjà lumineux) et la glace. Rien dans `nuit.cpp`.
  - Code mort retiré : `fovHorizontalCockpit`, `fovVerticalDepuisHorizontal`, `COCKPIT_FOV_H`.
  - Banc `cockpit` : mesures « toute l'image » et « horizon » ; capture **départ de nuit sur la Plage**.
  - `tests/test_regard_cockpit.cpp` : nouvelles valeurs de RG-02.
- **Comment c'est vérifié** : 66 tests CPU verts ; branche synchronisée avec `main` (98272f6d ; WFS-232, motos du trafic, est arrivé sur main après les captures : il ne touche pas le cockpit, tests revérifiés) ; banc `cockpit` en **2340 × 1080**, Golf / SF90 / X5, avant (main) / après (branche), même run. L'« avant » de la Plage (Golf, SF90) est main + la seule ligne de banc de la capture Plage (aucun autre changement), compilé à part. Mesures, selon les **deux arbitrages du PO** (`regles-de-gestion/WFS-282.md`, 01:14:55 et 02:30:59 UTC) :
  - **Fenêtre de la rue devant (±15°, jusqu'à l'horizon + 8°) : 0,0 % caché sur toutes les captures**, avant et après, jour et nuit (RG-05).
  - **Regard Normal** — intérieur (≤ 40 % en ligne droite ; SF90 en virage ≤ 42 %) : Golf 37,6-39,1 %, X5 34,6-36,1 %, SF90 ligne droite 39,8-39,9 %, SF90 virage 40,8-40,9 %. Horizon (45-47 %) : 46,4-46,8 %. Bande de la rue pleine largeur (≤ 12 %) : 10,1 à 12,0 % (avant → après : Golf jour 11,1 → 12,0, virage 9,5 → 11,5 ; SF90 jour 10,6 → 11,4, virage 8,2 → 10,5 ; X5 jour 11,0 → 11,8, virage 9,2 → 11,5 ; cause : RG-02, montant au bord de l'image, voir les paires `*-virage`).
  - **Regard Haut et Bas** (mesurés et écrits, sans seuil, arbitrage n° 2) : Haut — intérieur Golf 34,9 %, SF90 36,9 %, X5 32,1 % ; bande Golf 12,8 %, SF90 11,4 %, X5 12,7 % ; horizon 50 %. Bas — intérieur 38,8-44,2 %, bande 10,8-11,7 %, horizon 41,7-41,9 %.
  - **Rétroviseur, luminance moyenne de la glace (0-255) contre le ciel à côté**, départ de nuit sur la Plage : Golf avant (bloc noir) 22 / ciel 24 → après **14** / ciel 24 ; SF90 avant 16 / ciel 15 → après **16** / ciel 15. De jour (Golf, Ville) : avant 7 (bloc noir) → après **111** (glace claire).
  - Montants : à leur place réelle (carrosserie de RG-00), comme écrit dans l'arbitrage n° 2.
- **Limites et risques connus** :
  - La nuit, la glace est sombre comme un rétroviseur en position nuit : elle ne montre pas l'image de la route derrière (pas de rendu du rétroviseur dans le moteur).
  - Cockpit commun (Corsa) : regard plus doux et œil +4 cm aussi ; pas de lumière de nuit.
  - Les captures du départ sur la Plage de la X5 n'ont pas d'« avant » (seules la Golf et la SF90 ont été refaites sur main).

### Gros plans du rétroviseur

| | Avant (main) | Après (branche) |
|---|---|---|
| Golf, Ville, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-gros-plan-retroviseur-golf-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-gros-plan-retroviseur-golf-jour.jpg?raw=true) |
| Golf, Ville, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-gros-plan-retroviseur-golf-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-gros-plan-retroviseur-golf-nuit.jpg?raw=true) |
| **Golf, Plage, nuit** | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-gros-plan-retroviseur-golf-plage-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-gros-plan-retroviseur-golf-plage-nuit.jpg?raw=true) |
| **SF90, Plage, nuit** | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-gros-plan-retroviseur-sf90-plage-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-gros-plan-retroviseur-sf90-plage-nuit.jpg?raw=true) |

## Captures pour le lead dev (24 après)

Dossier : [`qa/pr/WFS-282`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `cockpit-golf-braquage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-golf-braquage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-braquage.jpg?raw=true) |
| `cockpit-golf-carte` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-golf-carte.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-carte.jpg?raw=true) |
| `cockpit-golf-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-golf-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-jour.jpg?raw=true) |
| `cockpit-golf-nuit-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-golf-nuit-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-nuit-virage.jpg?raw=true) |
| `cockpit-golf-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-golf-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-nuit.jpg?raw=true) |
| `cockpit-golf-plage-nuit-depart` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-golf-plage-nuit-depart.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-plage-nuit-depart.jpg?raw=true) |
| `cockpit-golf-pluie` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-golf-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-pluie.jpg?raw=true) |
| `cockpit-golf-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-golf-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-golf-virage.jpg?raw=true) |
| `cockpit-sf90-braquage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-sf90-braquage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-sf90-braquage.jpg?raw=true) |
| `cockpit-sf90-carte` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-sf90-carte.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-sf90-carte.jpg?raw=true) |
| `cockpit-sf90-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-sf90-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-sf90-jour.jpg?raw=true) |
| `cockpit-sf90-nuit-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-sf90-nuit-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-sf90-nuit-virage.jpg?raw=true) |
| `cockpit-sf90-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-sf90-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-sf90-nuit.jpg?raw=true) |
| `cockpit-sf90-plage-nuit-depart` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/avant-cockpit-sf90-plage-nuit-depart.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-282/qa/pr/WFS-282/apres-cockpit-sf90-plage-nuit-depart.jpg?raw=true) |
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
