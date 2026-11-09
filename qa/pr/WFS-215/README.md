# WFS-215 — captures avant / après (tools/qa/captures_pr.sh)

Branche `feature/waforspeed-215` comparée à `origin/main` ; bancs : vitrines conducteurs parcours ; filtre : `@(vitrine-model3-feux|vitrine-model3-0|vitrine-a45-feux|vitrine-a45-0|vitrine-rrsport-0|vitrine-rrsport-feux|conducteurs-rrsport-jour|conducteurs-rrsport-nuit|parcours*)`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `conducteurs-rrsport-jour` | ![avant](avant-conducteurs-rrsport-jour.jpg) | ![après](apres-conducteurs-rrsport-jour.jpg) |
| `conducteurs-rrsport-nuit` | ![avant](avant-conducteurs-rrsport-nuit.jpg) | ![après](apres-conducteurs-rrsport-nuit.jpg) |
| `parcours-course` | ![avant](avant-parcours-course.jpg) | ![après](apres-parcours-course.jpg) |
| `parcours-fin` | ![avant](avant-parcours-fin.jpg) | ![après](apres-parcours-fin.jpg) |
| `parcours-mode` | ![avant](avant-parcours-mode.jpg) | ![après](apres-parcours-mode.jpg) |
| `parcours-retour-menu` | ![avant](avant-parcours-retour-menu.jpg) | ![après](apres-parcours-retour-menu.jpg) |
| `parcours-terrain` | ![avant](avant-parcours-terrain.jpg) | ![après](apres-parcours-terrain.jpg) |
| `parcours-course` | ![avant](avant-parcours-course.jpg) | ![après](apres-parcours-course.jpg) |
| `parcours-fin` | ![avant](avant-parcours-fin.jpg) | ![après](apres-parcours-fin.jpg) |
| `parcours-mode` | ![avant](avant-parcours-mode.jpg) | ![après](apres-parcours-mode.jpg) |
| `parcours-retour-menu` | ![avant](avant-parcours-retour-menu.jpg) | ![après](apres-parcours-retour-menu.jpg) |
| `parcours-terrain` | ![avant](avant-parcours-terrain.jpg) | ![après](apres-parcours-terrain.jpg) |
| `vitrine-a45-0` | ![avant](avant-vitrine-a45-0.jpg) | ![après](apres-vitrine-a45-0.jpg) |
| `vitrine-a45-feux` | ![avant](avant-vitrine-a45-feux.jpg) | ![après](apres-vitrine-a45-feux.jpg) |
| `vitrine-model3-0` | ![avant](avant-vitrine-model3-0.jpg) | ![après](apres-vitrine-model3-0.jpg) |
| `vitrine-model3-feux` | ![avant](avant-vitrine-model3-feux.jpg) | ![après](apres-vitrine-model3-feux.jpg) |
| `vitrine-rrsport-0` | ![avant](avant-vitrine-rrsport-0.jpg) | ![après](apres-vitrine-rrsport-0.jpg) |
| `vitrine-rrsport-feux` | ![avant](avant-vitrine-rrsport-feux.jpg) | ![après](apres-vitrine-rrsport-feux.jpg) |

### Nuit du RR Sport, sans éblouissement (revue du lead dev, point 2)
Même banc `conducteurs`, départ avancé de 300 segments (`WFS_DEPART=300 WFS_BANC=conducteurs`) : la voiture n'est plus face à la lumière du départ. Même binaire, modèles de `main` (avant) et de la branche (après).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `conducteurs-rrsport-nuit` (départ 300) | ![avant](avant-conducteurs-rrsport-nuit-depart300.jpg) | ![après](apres-conducteurs-rrsport-nuit-depart300.jpg) |
| zoom sur le pare-brise | ![avant](avant-conducteurs-rrsport-nuit-depart300-zoom.jpg) | ![après](apres-conducteurs-rrsport-nuit-depart300-zoom.jpg) |
