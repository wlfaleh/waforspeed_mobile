# WFS-381 — captures avant / après (banc WFS_BANC=brume, 1280x720, lavapipe)

Avant = brume de `main` reproduite dans la même construction (forçage 4 du banc : extinction du ciel, sans hauteur, même calcul du shader que main ; écart avec la vraie capture de main : RMSE 0,15 %) ; nuit : capture de `origin/main`. Après = branche `feature/waforspeed-381`. Écart = |après − avant| × 8. Mesures : `brume-mesures.txt`.

| Capture | Avant (main) | Après (branche) | Écart ×8 |
|---|---|---|---|
| `brume-campagne-haut-jour` | ![avant](avant-brume-campagne-haut-jour.jpg) | ![après](apres-brume-campagne-haut-jour.jpg) | ![écart ×8](ecart-brume-campagne-haut-jour.jpg) |
| `brume-campagne-jour` | ![avant](avant-brume-campagne-jour.jpg) | ![après](apres-brume-campagne-jour.jpg) | ![écart ×8](ecart-brume-campagne-jour.jpg) |
| `brume-corniche-haut-jour` | ![avant](avant-brume-corniche-haut-jour.jpg) | ![après](apres-brume-corniche-haut-jour.jpg) | ![écart ×8](ecart-brume-corniche-haut-jour.jpg) |
| `brume-corniche-jour` | ![avant](avant-brume-corniche-jour.jpg) | ![après](apres-brume-corniche-jour.jpg) | ![écart ×8](ecart-brume-corniche-jour.jpg) |
| `brume-corniche-nuit` | ![avant](avant-brume-corniche-nuit.jpg) | ![après](apres-brume-corniche-nuit.jpg) | ![écart ×8](ecart-brume-corniche-nuit.jpg) |
| `brume-ville-haut-jour` | ![avant](avant-brume-ville-haut-jour.jpg) | ![après](apres-brume-ville-haut-jour.jpg) | ![écart ×8](ecart-brume-ville-haut-jour.jpg) |
| `brume-ville-jour` | ![avant](avant-brume-ville-jour.jpg) | ![après](apres-brume-ville-jour.jpg) | ![écart ×8](ecart-brume-ville-jour.jpg) |
