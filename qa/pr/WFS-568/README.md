# WFS-568 — captures avant / après

Branche `feature/waforspeed-568` comparée à `origin/main` ; bancs : `ville:1280x720` (WFS_VILLE_LIEU=carte-ville-enseigne,carte-ville-lieu-centre,carte-ville-lieu-garage,carte-ville-ao-3-g), `corniche:1280x720` (WFS_CORNICHE=enseignes).

Captures « avant » : `origin/main` = `fa5b065045491d70d8da9c454fc70767bf675595` compilé avec **seulement les vues de banc** de cette PR (`cartes_banc.cpp` : lieux `carte-ville-enseigne-*` ; `plage.cpp` : `WFS_CORNICHE=enseignes` ; `vie::panneauxRoute()` qui place les panneaux comme le système) : aucun changement visuel côté « avant ». Produit à la main (même rendu lavapipe, 1280 × 720, mêmes JPG 1280 px qualité 85) car `captures_pr.sh` prend `main` tel quel et ces vues n'y existent pas encore.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `carte-ville-enseigne-1` | ![avant](avant-carte-ville-enseigne-1.jpg) | ![après](apres-carte-ville-enseigne-1.jpg) |
| `carte-ville-enseigne-1-nuit` | ![avant](avant-carte-ville-enseigne-1-nuit.jpg) | ![après](apres-carte-ville-enseigne-1-nuit.jpg) |
| `carte-ville-enseigne-2` | ![avant](avant-carte-ville-enseigne-2.jpg) | ![après](apres-carte-ville-enseigne-2.jpg) |
| `carte-ville-enseigne-2-nuit` | ![avant](avant-carte-ville-enseigne-2-nuit.jpg) | ![après](apres-carte-ville-enseigne-2-nuit.jpg) |
| `carte-ville-enseigne-1-route` | ![avant](avant-carte-ville-enseigne-1-route.jpg) | ![après](apres-carte-ville-enseigne-1-route.jpg) |
| `carte-ville-enseigne-1-route-nuit` | ![avant](avant-carte-ville-enseigne-1-route-nuit.jpg) | ![après](apres-carte-ville-enseigne-1-route-nuit.jpg) |
| `carte-ville-enseigne-2-route` | ![avant](avant-carte-ville-enseigne-2-route.jpg) | ![après](apres-carte-ville-enseigne-2-route.jpg) |
| `carte-ville-enseigne-2-route-nuit` | ![avant](avant-carte-ville-enseigne-2-route-nuit.jpg) | ![après](apres-carte-ville-enseigne-2-route-nuit.jpg) |
| `carte-ville-ao-3-g` | ![avant](avant-carte-ville-ao-3-g.jpg) | ![après](apres-carte-ville-ao-3-g.jpg) |
| `carte-ville-lieu-centre` | ![avant](avant-carte-ville-lieu-centre.jpg) | ![après](apres-carte-ville-lieu-centre.jpg) |
| `carte-ville-lieu-centre-nuit` | ![avant](avant-carte-ville-lieu-centre-nuit.jpg) | ![après](apres-carte-ville-lieu-centre-nuit.jpg) |
| `carte-ville-lieu-garage` | ![avant](avant-carte-ville-lieu-garage.jpg) | ![après](apres-carte-ville-lieu-garage.jpg) |
| `carte-ville-lieu-garage-nuit` | ![avant](avant-carte-ville-lieu-garage-nuit.jpg) | ![après](apres-carte-ville-lieu-garage-nuit.jpg) |
| `carte-plage-enseigne-1` | ![avant](avant-carte-plage-enseigne-1.jpg) | ![après](apres-carte-plage-enseigne-1.jpg) |
| `carte-plage-enseigne-1-nuit` | ![avant](avant-carte-plage-enseigne-1-nuit.jpg) | ![après](apres-carte-plage-enseigne-1-nuit.jpg) |
| `carte-plage-enseigne-2` | ![avant](avant-carte-plage-enseigne-2.jpg) | ![après](apres-carte-plage-enseigne-2.jpg) |
| `carte-plage-enseigne-2-nuit` | ![avant](avant-carte-plage-enseigne-2-nuit.jpg) | ![après](apres-carte-plage-enseigne-2-nuit.jpg) |
