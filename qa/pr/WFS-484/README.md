# WFS-484 — captures avant / après

Banc `WFS_BANC=plaques-autres` (src/render/vehicule_plaques_autres_banc.cpp), Ville, Golf du joueur, rendu logiciel lavapipe (même rendu que la CI),
2400 × 1080 réduites à 1280 px ; `-x2` : recadrage 1200 × 540 de l'image 2400 × 1080 (zoom ×2) ; `-x4` : recadrage 600 × 270 agrandi ×2.
« Avant » : le même banc avec `WFS_PLAQUES_AUTRES_AVANT=1` (plaques des autres voitures masquées : rendu de `main`, où ce banc n'existe pas).
Vue Proche (caméra derrière le joueur) ; gros plans : la voiture visée est entre la caméra (4 m d'elle, à 35 cm du sol) et le joueur.

Tests CPU : [`tests-cpu.log`](tests-cpu.log) (sortie complète de `bash tools/qa/tests_cpu.sh build`).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `plaques-course5-jour` | ![avant](avant-plaques-course5-jour.jpg) | ![après](apres-plaques-course5-jour.jpg) |
| `plaques-course5-jour-x2` | ![avant](avant-plaques-course5-jour-x2.jpg) | ![après](apres-plaques-course5-jour-x2.jpg) |
| `plaques-course5-arriere-jour` | ![avant](avant-plaques-course5-arriere-jour.jpg) | ![après](apres-plaques-course5-arriere-jour.jpg) |
| `plaques-course5-arriere-jour-x2` | ![avant](avant-plaques-course5-arriere-jour-x2.jpg) | ![après](apres-plaques-course5-arriere-jour-x2.jpg) |
| `plaques-trafic-jour` | ![avant](avant-plaques-trafic-jour.jpg) | ![après](apres-plaques-trafic-jour.jpg) |
| `plaques-trafic-nuit` | ![avant](avant-plaques-trafic-nuit.jpg) | ![après](apres-plaques-trafic-nuit.jpg) |
| `plaques-trafic-arriere-jour` | ![avant](avant-plaques-trafic-arriere-jour.jpg) | ![après](apres-plaques-trafic-arriere-jour.jpg) |
| `plaques-trafic-arriere-jour-x2` | ![avant](avant-plaques-trafic-arriere-jour-x2.jpg) | ![après](apres-plaques-trafic-arriere-jour-x2.jpg) |
| `plaques-trafic-arriere-nuit` | ![avant](avant-plaques-trafic-arriere-nuit.jpg) | ![après](apres-plaques-trafic-arriere-nuit.jpg) |
| `plaques-trafic-avant-jour` | ![avant](avant-plaques-trafic-avant-jour.jpg) | ![après](apres-plaques-trafic-avant-jour.jpg) |
| `plaques-trafic-avant-nuit` | ![avant](avant-plaques-trafic-avant-nuit.jpg) | ![après](apres-plaques-trafic-avant-nuit.jpg) |
| `plaques-trafic-avant-nuit-x2` | ![avant](avant-plaques-trafic-avant-nuit-x2.jpg) | ![après](apres-plaques-trafic-avant-nuit-x2.jpg) |
| `plaques-police-jour` | ![avant](avant-plaques-police-jour.jpg) | ![après](apres-plaques-police-jour.jpg) |
| `plaques-police-nuit` | ![avant](avant-plaques-police-nuit.jpg) | ![après](apres-plaques-police-nuit.jpg) |
| `plaques-police-arriere-jour` | ![avant](avant-plaques-police-arriere-jour.jpg) | ![après](apres-plaques-police-arriere-jour.jpg) |
| `plaques-police-arriere-nuit` | ![avant](avant-plaques-police-arriere-nuit.jpg) | ![après](apres-plaques-police-arriere-nuit.jpg) |
| `plaques-police-arriere-nuit-x2` | ![avant](avant-plaques-police-arriere-nuit-x2.jpg) | ![après](apres-plaques-police-arriere-nuit-x2.jpg) |
| `plaques-police-avant-jour` | ![avant](avant-plaques-police-avant-jour.jpg) | ![après](apres-plaques-police-avant-jour.jpg) |
| `plaques-police-avant-nuit` | ![avant](avant-plaques-police-avant-nuit.jpg) | ![après](apres-plaques-police-avant-nuit.jpg) |
| `plaques-fantome-jour` | ![avant](avant-plaques-fantome-jour.jpg) | ![après](apres-plaques-fantome-jour.jpg) |
| `plaques-fantome-jour-x4` | ![avant](avant-plaques-fantome-jour-x4.jpg) | ![après](apres-plaques-fantome-jour-x4.jpg) |
