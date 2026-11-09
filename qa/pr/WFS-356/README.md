# WFS-356 — captures avant / après (banc PC, rendu de la CI : lavapipe, 1280x720)

Avant = `origin/main` (avec le seul banc `depart` de la branche, pour la variante Basse 60 %) ; après = `feature/waforspeed-356`.
Bancs : `WFS_BANC=depart` avec `WFS_DEPART_SEUL=basse | departs | recommencer | cockpit`, et `WFS_BANC=ville WFS_VILLE_LIEU=carte-ville-autoroute WFS_QUALITE_BASSE=1`.
Instants : `d3` = 1re image du décompte (« 3 »), `0`/`1`/`2`/`4` = secondes du chrono.

## Mesures

| Capture | EV avant | EV après | saturés R,G,B ≥ 250 avant → après | très clairs Y ≥ 230 avant → après (HUD compris) | luminance moyenne avant → après |
|---|---|---|---|---|---|
| `depart-basse60-nuit-ville-d3` | 2.27 | 4.00 | 0.00 % → 0.00 % | 5.53 % → 1.00 % | 51 → 38 |
| `depart-basse60-nuit-ville-2` | 3.82 | 4.08 | 0.00 % → 0.00 % | 0.65 % → 0.39 % | 60 → 51 |
| `depart-basse60-nuit-ville-4` | 4.06 | 4.08 | 0.00 % → 0.00 % | 0.41 % → 0.40 % | 64 → 60 |
| `depart-basse60-cockpit-nuit-ville-d3` | 2.27 | 4.00 | 0.00 % → 0.00 % | 4.71 % → 0.98 % | 48 → 36 |
| `depart-nuit-ville-d3` | 2.27 | 4.00 | 0.00 % → 0.00 % | 6.75 % → 1.02 % | 81 → 42 |
| `depart-nuit-ville-4` | 4.11 | 4.18 | 0.00 % → 0.00 % | 2.06 % → 0.42 % | 73 → 62 |
| `depart-cockpit-nuit-ville-d3` | 2.27 | 4.00 | 0.00 % → 0.00 % | 5.86 % → 1.04 % | 77 → 40 |
| `depart-recommencer-nuit-ville-d3` | 2.27 | 4.00 | 0.01 % → 0.00 % | 6.81 % → 1.09 % | 103 → 60 |
| `depart-reprise-nuit-ville` | 4.12 | 4.18 | 0.00 % → 0.00 % | 1.62 % → 0.43 % | 61 → 53 |
| `carte-ville-autoroute-nuit` | — | — | 0.00 % → 0.00 % | 1.43 % → 1.00 % | 30 → 24 |
| `depart-jour-ville-d3` | 14.76 | 14.76 | 0.13 % → 0.13 % | 1.66 % → 1.66 % | 93 → 93 |

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `depart-basse60-nuit-ville-d3` | ![avant](avant-depart-basse60-nuit-ville-d3.jpg) | ![après](apres-depart-basse60-nuit-ville-d3.jpg) |
| `depart-basse60-nuit-ville-2` | ![avant](avant-depart-basse60-nuit-ville-2.jpg) | ![après](apres-depart-basse60-nuit-ville-2.jpg) |
| `depart-basse60-nuit-ville-4` | ![avant](avant-depart-basse60-nuit-ville-4.jpg) | ![après](apres-depart-basse60-nuit-ville-4.jpg) |
| `depart-basse60-cockpit-nuit-ville-d3` | ![avant](avant-depart-basse60-cockpit-nuit-ville-d3.jpg) | ![après](apres-depart-basse60-cockpit-nuit-ville-d3.jpg) |
| `depart-basse60-cockpit-nuit-ville-2` | ![avant](avant-depart-basse60-cockpit-nuit-ville-2.jpg) | ![après](apres-depart-basse60-cockpit-nuit-ville-2.jpg) |
| `depart-nuit-ville-d3` | ![avant](avant-depart-nuit-ville-d3.jpg) | ![après](apres-depart-nuit-ville-d3.jpg) |
| `depart-nuit-ville-1` | ![avant](avant-depart-nuit-ville-1.jpg) | ![après](apres-depart-nuit-ville-1.jpg) |
| `depart-nuit-ville-4` | ![avant](avant-depart-nuit-ville-4.jpg) | ![après](apres-depart-nuit-ville-4.jpg) |
| `depart-cockpit-nuit-ville-d3` | ![avant](avant-depart-cockpit-nuit-ville-d3.jpg) | ![après](apres-depart-cockpit-nuit-ville-d3.jpg) |
| `depart-recommencer-nuit-ville-d3` | ![avant](avant-depart-recommencer-nuit-ville-d3.jpg) | ![après](apres-depart-recommencer-nuit-ville-d3.jpg) |
| `depart-reprise-nuit-ville` | ![avant](avant-depart-reprise-nuit-ville.jpg) | ![après](apres-depart-reprise-nuit-ville.jpg) |
| `carte-ville-autoroute-nuit` | ![avant](avant-carte-ville-autoroute-nuit.jpg) | ![après](apres-carte-ville-autoroute-nuit.jpg) |
| `depart-nuit-plage-d3` | ![avant](avant-depart-nuit-plage-d3.jpg) | ![après](apres-depart-nuit-plage-d3.jpg) |
| `depart-nuit-campagne-d3` | ![avant](avant-depart-nuit-campagne-d3.jpg) | ![après](apres-depart-nuit-campagne-d3.jpg) |
| `depart-jour-ville-d3` | ![avant](avant-depart-jour-ville-d3.jpg) | ![après](apres-depart-jour-ville-d3.jpg) |
| `depart-jour-ville-4` | ![avant](avant-depart-jour-ville-4.jpg) | ![après](apres-depart-jour-ville-4.jpg) |
