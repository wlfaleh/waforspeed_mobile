# WFS-370 — captures avant / après

Produit comme `tools/qa/captures_pr.sh` (même banc lavapipe, 1280x720, JPG 85) mais à la main : les vues des immeubles et du port sont **nouvelles** dans `cartes_banc.cpp` ; l'« avant » est donc `origin/main` (04508b2b + journal) **plus le seul fichier `src/world/cartes_banc.cpp` de la branche** (vues du banc, aucun changement du décor). Bancs : `WFS_BANC=ville WFS_VILLE_LIEU=carte-ville-immeuble,carte-ville-port,carte-ville-mobilier-rue,carte-ville-ao-1` ; `WFS_BANC=perf WFS_PERF_CARTE=ville WFS_MOMENT=nuit WFS_PERF_CAPTURE=1`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `carte-ville-ao-1-d-nuit` | ![avant](avant-carte-ville-ao-1-d-nuit.jpg) | ![après](apres-carte-ville-ao-1-d-nuit.jpg) |
| `carte-ville-ao-1-d` | ![avant](avant-carte-ville-ao-1-d.jpg) | ![après](apres-carte-ville-ao-1-d.jpg) |
| `carte-ville-ao-1-g-nuit` | ![avant](avant-carte-ville-ao-1-g-nuit.jpg) | ![après](apres-carte-ville-ao-1-g-nuit.jpg) |
| `carte-ville-ao-1-g` | ![avant](avant-carte-ville-ao-1-g.jpg) | ![après](apres-carte-ville-ao-1-g.jpg) |
| `carte-ville-immeuble-classique-nuit` | ![avant](avant-carte-ville-immeuble-classique-nuit.jpg) | ![après](apres-carte-ville-immeuble-classique-nuit.jpg) |
| `carte-ville-immeuble-classique` | ![avant](avant-carte-ville-immeuble-classique.jpg) | ![après](apres-carte-ville-immeuble-classique.jpg) |
| `carte-ville-immeuble-moderne-nuit` | ![avant](avant-carte-ville-immeuble-moderne-nuit.jpg) | ![après](apres-carte-ville-immeuble-moderne-nuit.jpg) |
| `carte-ville-immeuble-moderne` | ![avant](avant-carte-ville-immeuble-moderne.jpg) | ![après](apres-carte-ville-immeuble-moderne.jpg) |
| `carte-ville-mobilier-rue-nuit` | ![avant](avant-carte-ville-mobilier-rue-nuit.jpg) | ![après](apres-carte-ville-mobilier-rue-nuit.jpg) |
| `carte-ville-mobilier-rue` | ![avant](avant-carte-ville-mobilier-rue.jpg) | ![après](apres-carte-ville-mobilier-rue.jpg) |
| `carte-ville-port-cloture-nuit` | ![avant](avant-carte-ville-port-cloture-nuit.jpg) | ![après](apres-carte-ville-port-cloture-nuit.jpg) |
| `carte-ville-port-cloture` | ![avant](avant-carte-ville-port-cloture.jpg) | ![après](apres-carte-ville-port-cloture.jpg) |
| `carte-ville-port-conteneurs-nuit` | ![avant](avant-carte-ville-port-conteneurs-nuit.jpg) | ![après](apres-carte-ville-port-conteneurs-nuit.jpg) |
| `carte-ville-port-conteneurs` | ![avant](avant-carte-ville-port-conteneurs.jpg) | ![après](apres-carte-ville-port-conteneurs.jpg) |
| `carte-ville-port-entrepot-nuit` | ![avant](avant-carte-ville-port-entrepot-nuit.jpg) | ![après](apres-carte-ville-port-entrepot-nuit.jpg) |
| `carte-ville-port-entrepot` | ![avant](avant-carte-ville-port-entrepot.jpg) | ![après](apres-carte-ville-port-entrepot.jpg) |
| `carte-ville-port-grue-nuit` | ![avant](avant-carte-ville-port-grue-nuit.jpg) | ![après](apres-carte-ville-port-grue-nuit.jpg) |
| `carte-ville-port-grue` | ![avant](avant-carte-ville-port-grue.jpg) | ![après](apres-carte-ville-port-grue.jpg) |
| `carte-ville-port-rue-nuit` | ![avant](avant-carte-ville-port-rue-nuit.jpg) | ![après](apres-carte-ville-port-rue-nuit.jpg) |
| `carte-ville-port-rue` | ![avant](avant-carte-ville-port-rue.jpg) | ![après](apres-carte-ville-port-rue.jpg) |
