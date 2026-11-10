# WFS-382 — captures avant / après (2e version, après la revue du lead dev)

Bancs `phares` et `neige`, 1280×720, lavapipe. « Avant » = `origin/main` (7a2e01d7, brume WFS-381 comprise) + le seul banc `phares` ; « après » = la branche.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| 1. Poursuite de nuit, police derrière le joueur, vue de biais (phares de la police vers la caméra), Haute | ![avant](avant-phares-poursuite-police.jpg) | ![après](apres-phares-poursuite-police.jpg) |
| 1. Même scène en qualité Basse | ![avant](avant-phares-poursuite-basse-police.jpg) | ![après](apres-phares-poursuite-basse-police.jpg) |
| 1. Poursuite, vue de face du joueur, Haute | ![avant](avant-phares-poursuite-face.jpg) | ![après](apres-phares-poursuite-face.jpg) |
| 1. Même scène en qualité Basse | ![avant](avant-phares-poursuite-basse-face.jpg) | ![après](apres-phares-poursuite-basse-face.jpg) |
| 1. Poursuite, vue Proche (jeu) | ![avant](avant-phares-poursuite-proche.jpg) | ![après](apres-phares-poursuite-proche.jpg) |
| 2. Voiture vue de face, Ville, chaussée sèche | ![avant](avant-phares-ville-face.jpg) | ![après](apres-phares-ville-face.jpg) |
| 3. Vue Capot, Campagne (autoroute, aucun lampadaire) : tache de 30 à 60 m | ![avant](avant-phares-campagne-capot.jpg) | ![après](apres-phares-campagne-capot.jpg) |
| 3. Même endroit, vue haute : forme de la tache et bord de la coupure | ![avant](avant-phares-campagne-capot-haut.jpg) | ![après](apres-phares-campagne-capot-haut.jpg) |
| 4. Vue Capot sous la pluie | ![avant](avant-phares-campagne-capot-pluie.jpg) | ![après](apres-phares-campagne-capot-pluie.jpg) |
| 4. Même endroit sous la pluie, vue haute | ![avant](avant-phares-campagne-capot-pluie-haut.jpg) | ![après](apres-phares-campagne-capot-pluie-haut.jpg) |
| 4. Voiture vue de face, Ville, sous la pluie | ![avant](avant-phares-ville-face-pluie.jpg) | ![après](apres-phares-ville-face-pluie.jpg) |
| 6. Neige de nuit, vue Arrière (banc neige) | ![avant](avant-neige-nuit-arriere.jpg) | ![après](apres-neige-nuit-arriere.jpg) |
| 6. Neige de nuit, vue Cockpit (banc neige) | ![avant](avant-neige-cockpit-nuit.jpg) | ![après](apres-neige-cockpit-nuit.jpg) |
| Témoin de jour, vue Proche (identique) | ![avant](avant-phares-ville-jour-proche.jpg) | ![après](apres-phares-ville-jour-proche.jpg) |
| Témoin de jour, vue de face (identique) | ![avant](avant-phares-ville-jour-face.jpg) | ![après](apres-phares-ville-jour-face.jpg) |

**Zooms ×3 demandés par le lead dev** (de gauche à droite : `main` · 1re version de la PR, `min()` par canal · correction) :

| Zoom | Image |
|---|---|
| Lampadaire (478, 68) de la poursuite, Haute | ![zoom](zoom-phares-poursuite-face.jpg) |
| Même lampadaire, Basse | ![zoom](zoom-phares-poursuite-basse-face.jpg) |
| Lampadaire (478, 62), Ville au sec | ![zoom](zoom-phares-ville-face.jpg) |
| Même lampadaire, sous la pluie | ![zoom](zoom-phares-ville-face-pluie.jpg) |
| Écran du Cockpit, neige de nuit (gauche `main`, droite correction) | ![zoom](zoom-neige-cockpit-nuit.jpg) |

