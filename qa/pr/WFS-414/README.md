# WFS-414 — captures avant / après (volants de série : Mustang, Golf 8, Abarth 500, T-Cross, S3 berline, Giulia)

Branche `feature/waforspeed-414` comparée à `origin/main` (19be73b, après WFS-415). Bancs : `WFS_BANC=cockpit-volant WFS_TAILLE=2340x1080` (rendu lavapipe, comme la CI). « Avant » = `origin/main` + le banc seul (fichiers `vehicule_cockpit_volant_banc.*` et accroche de `desktop_main.cpp`, sans aucun changement du cockpit), comme pour WFS-323 : le banc n'existe pas sur `main`. Gros plans : zone de 4:3 autour de la jante projetée par la caméra de l'image, agrandie 2 fois, sans autre rendu. Copies « éclaircies » : niveaux 0..56 étirés sur 0..255 (l'habitacle est sombre sous le toit), aide à la revue seulement ; même traitement avant et après.

## Planches des 6 volants (gros plans éclaircis : Mustang, Golf, Abarth / T-Cross, S3, Giulia)

| Avant (main) | Après (branche) |
|---|---|
| ![](planche-6-volants-avant.jpg) | ![](planche-6-volants-apres.jpg) |

Témoins qui ne doivent pas changer : **M4 et 911 identiques au pixel près** (comparaison des PNG : 0 pixel différent, vue entière et gros plan).

## Toutes les captures

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `volant-mustang-jour-gros-plan` | ![avant](avant-volant-mustang-jour-gros-plan.jpg) | ![après](apres-volant-mustang-jour-gros-plan.jpg) |
| `volant-mustang-jour-gros-plan-eclairci` | ![avant](avant-volant-mustang-jour-gros-plan-eclairci.jpg) | ![après](apres-volant-mustang-jour-gros-plan-eclairci.jpg) |
| `volant-golf-jour-gros-plan` | ![avant](avant-volant-golf-jour-gros-plan.jpg) | ![après](apres-volant-golf-jour-gros-plan.jpg) |
| `volant-golf-jour-gros-plan-eclairci` | ![avant](avant-volant-golf-jour-gros-plan-eclairci.jpg) | ![après](apres-volant-golf-jour-gros-plan-eclairci.jpg) |
| `volant-abarth-jour-gros-plan` | ![avant](avant-volant-abarth-jour-gros-plan.jpg) | ![après](apres-volant-abarth-jour-gros-plan.jpg) |
| `volant-abarth-jour-gros-plan-eclairci` | ![avant](avant-volant-abarth-jour-gros-plan-eclairci.jpg) | ![après](apres-volant-abarth-jour-gros-plan-eclairci.jpg) |
| `volant-tcross-jour-gros-plan` | ![avant](avant-volant-tcross-jour-gros-plan.jpg) | ![après](apres-volant-tcross-jour-gros-plan.jpg) |
| `volant-tcross-jour-gros-plan-eclairci` | ![avant](avant-volant-tcross-jour-gros-plan-eclairci.jpg) | ![après](apres-volant-tcross-jour-gros-plan-eclairci.jpg) |
| `volant-s3-jour-gros-plan` | ![avant](avant-volant-s3-jour-gros-plan.jpg) | ![après](apres-volant-s3-jour-gros-plan.jpg) |
| `volant-s3-jour-gros-plan-eclairci` | ![avant](avant-volant-s3-jour-gros-plan-eclairci.jpg) | ![après](apres-volant-s3-jour-gros-plan-eclairci.jpg) |
| `volant-giulia-jour-gros-plan` | ![avant](avant-volant-giulia-jour-gros-plan.jpg) | ![après](apres-volant-giulia-jour-gros-plan.jpg) |
| `volant-giulia-jour-gros-plan-eclairci` | ![avant](avant-volant-giulia-jour-gros-plan-eclairci.jpg) | ![après](apres-volant-giulia-jour-gros-plan-eclairci.jpg) |
| `volant-m4-jour-gros-plan` | ![avant](avant-volant-m4-jour-gros-plan.jpg) | ![après](apres-volant-m4-jour-gros-plan.jpg) |
| `volant-m4-jour-gros-plan-eclairci` | ![avant](avant-volant-m4-jour-gros-plan-eclairci.jpg) | ![après](apres-volant-m4-jour-gros-plan-eclairci.jpg) |
| `volant-p911-jour-gros-plan` | ![avant](avant-volant-p911-jour-gros-plan.jpg) | ![après](apres-volant-p911-jour-gros-plan.jpg) |
| `volant-p911-jour-gros-plan-eclairci` | ![avant](avant-volant-p911-jour-gros-plan-eclairci.jpg) | ![après](apres-volant-p911-jour-gros-plan-eclairci.jpg) |
| `volant-mustang-nuit-gros-plan` | ![avant](avant-volant-mustang-nuit-gros-plan.jpg) | ![après](apres-volant-mustang-nuit-gros-plan.jpg) |
| `volant-mustang-nuit-gros-plan-eclairci` | ![avant](avant-volant-mustang-nuit-gros-plan-eclairci.jpg) | ![après](apres-volant-mustang-nuit-gros-plan-eclairci.jpg) |
| `volant-mustang-braquage-1-gros-plan` | ![avant](avant-volant-mustang-braquage-1-gros-plan.jpg) | ![après](apres-volant-mustang-braquage-1-gros-plan.jpg) |
| `volant-mustang-braquage-1-gros-plan-eclairci` | ![avant](avant-volant-mustang-braquage-1-gros-plan-eclairci.jpg) | ![après](apres-volant-mustang-braquage-1-gros-plan-eclairci.jpg) |
| `volant-mustang-braquage-2-gros-plan` | ![avant](avant-volant-mustang-braquage-2-gros-plan.jpg) | ![après](apres-volant-mustang-braquage-2-gros-plan.jpg) |
| `volant-mustang-braquage-2-gros-plan-eclairci` | ![avant](avant-volant-mustang-braquage-2-gros-plan-eclairci.jpg) | ![après](apres-volant-mustang-braquage-2-gros-plan-eclairci.jpg) |
| `volant-mustang-braquage-3-gros-plan` | ![avant](avant-volant-mustang-braquage-3-gros-plan.jpg) | ![après](apres-volant-mustang-braquage-3-gros-plan.jpg) |
| `volant-mustang-braquage-3-gros-plan-eclairci` | ![avant](avant-volant-mustang-braquage-3-gros-plan-eclairci.jpg) | ![après](apres-volant-mustang-braquage-3-gros-plan-eclairci.jpg) |
| `volant-mustang-braquage-4-gros-plan` | ![avant](avant-volant-mustang-braquage-4-gros-plan.jpg) | ![après](apres-volant-mustang-braquage-4-gros-plan.jpg) |
| `volant-mustang-braquage-4-gros-plan-eclairci` | ![avant](avant-volant-mustang-braquage-4-gros-plan-eclairci.jpg) | ![après](apres-volant-mustang-braquage-4-gros-plan-eclairci.jpg) |
| `volant-golf-braquage-1-gros-plan` | ![avant](avant-volant-golf-braquage-1-gros-plan.jpg) | ![après](apres-volant-golf-braquage-1-gros-plan.jpg) |
| `volant-golf-braquage-1-gros-plan-eclairci` | ![avant](avant-volant-golf-braquage-1-gros-plan-eclairci.jpg) | ![après](apres-volant-golf-braquage-1-gros-plan-eclairci.jpg) |
| `volant-golf-braquage-2-gros-plan` | ![avant](avant-volant-golf-braquage-2-gros-plan.jpg) | ![après](apres-volant-golf-braquage-2-gros-plan.jpg) |
| `volant-golf-braquage-2-gros-plan-eclairci` | ![avant](avant-volant-golf-braquage-2-gros-plan-eclairci.jpg) | ![après](apres-volant-golf-braquage-2-gros-plan-eclairci.jpg) |
| `volant-golf-braquage-3-gros-plan` | ![avant](avant-volant-golf-braquage-3-gros-plan.jpg) | ![après](apres-volant-golf-braquage-3-gros-plan.jpg) |
| `volant-golf-braquage-3-gros-plan-eclairci` | ![avant](avant-volant-golf-braquage-3-gros-plan-eclairci.jpg) | ![après](apres-volant-golf-braquage-3-gros-plan-eclairci.jpg) |
| `volant-golf-braquage-4-gros-plan` | ![avant](avant-volant-golf-braquage-4-gros-plan.jpg) | ![après](apres-volant-golf-braquage-4-gros-plan.jpg) |
| `volant-golf-braquage-4-gros-plan-eclairci` | ![avant](avant-volant-golf-braquage-4-gros-plan-eclairci.jpg) | ![après](apres-volant-golf-braquage-4-gros-plan-eclairci.jpg) |
| `volant-mustang-jour` | ![avant](avant-volant-mustang-jour.jpg) | ![après](apres-volant-mustang-jour.jpg) |
| `volant-mustang-nuit` | ![avant](avant-volant-mustang-nuit.jpg) | ![après](apres-volant-mustang-nuit.jpg) |
| `volant-golf-jour` | ![avant](avant-volant-golf-jour.jpg) | ![après](apres-volant-golf-jour.jpg) |
