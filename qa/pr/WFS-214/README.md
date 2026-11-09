# WFS-214 — captures avant / après

Branche `feature/waforspeed-214` comparée à `origin/main` (64c14e06) ; banc d'essai PC (lavapipe, 1280x720, même rendu que la CI) : `realisme` (`WFS_LUMIERE_SEUL=1`, `WFS_PASSES_SEUL=1`) et `depart` (`WFS_DEPART_SEUL=departs`). Même méthode que `tools/qa/captures_pr.sh` (main compilé dans un arbre à part, mêmes bancs des deux côtés, JPG 1280 px qualité 85), bancs lancés en parallèle pour gagner 1 h 30.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `realisme-ciel-campagne-h70` — RG-02 : ciel de midi (soleil à 70°) : gris-bleu sombre → bleu clair (zénith 2 667 → 7 034 cd/m²) | ![avant](avant-realisme-ciel-campagne-h70.jpg) | ![apres](apres-realisme-ciel-campagne-h70.jpg) |
| `realisme-ciel-circuit-h10` — RG-01 / RG-02 : ciel du soir (soleil à 10°) autour du soleil : dégradé continu, plus clair | ![avant](avant-realisme-ciel-circuit-h10.jpg) | ![apres](apres-realisme-ciel-circuit-h10.jpg) |
| `zoom-tramage-ciel-h10` — RG-01 : zoom × 2 du ciel autour du soleil, contraste renforcé (niveaux 25-75 %) : plus de marches | ![avant](avant-zoom-tramage-ciel-h10.jpg) | ![apres](apres-zoom-tramage-ciel-h10.jpg) |
| `realisme-ciel-matin-campagne` — RG-02 : matin (soleil à 25°, vue « h 30 » demandée par le PO) | ![avant](avant-realisme-ciel-matin-campagne.jpg) | ![apres](apres-realisme-ciel-matin-campagne.jpg) |
| `realisme-ciel-aube-campagne` — condition 3 du PO : aube (soleil sous l'horizon) inchangée | ![avant](avant-realisme-ciel-aube-campagne.jpg) | ![apres](apres-realisme-ciel-aube-campagne.jpg) |
| `depart-jour-ville-4` — RG-02 : Ville de jour, ombres (dessous de la Golf, façades) : un peu plus claires, pas délavées ; EV 15 avant et après | ![avant](avant-depart-jour-ville-4.jpg) | ![apres](apres-depart-jour-ville-4.jpg) |
| `parcours-ultra-course` — RG-03 : capture d'origine d'ANO-106 (chaussée grise ; sur main, ce banc n'atteint plus Ultra, voir limites) | ![avant](avant-parcours-ultra-course.jpg) | ![apres](apres-parcours-ultra-course.jpg) |
| `chaussee-ultra-course` — RG-03 : vrai Ultra (sonde 256 px), même instant que la suivante | — | ![apres](apres-chaussee-ultra-course.jpg) |
| `chaussee-haute-course` — RG-03 : Haute (sonde 128 px) : chaussée ΔE2000 0,02 à 0,03, trottoir 0,92 avec Ultra | — | ![apres](apres-chaussee-haute-course.jpg) |
| `depart-nuit-ville-4` — RG-05 : nuit, Golf sous les lampadaires de la Ville : garde le bleu de sa peinture | ![avant](avant-depart-nuit-ville-4.jpg) | ![apres](apres-depart-nuit-ville-4.jpg) |
| `depart-nuit-campagne-4` — RG-05 / condition 3 : nuit en Campagne loin des lumières : vision de nuit gardée | ![avant](avant-depart-nuit-campagne-4.jpg) | ![apres](apres-depart-nuit-campagne-4.jpg) |
| `realisme-ciel-campagne-nuit` — condition 3 : ciel de nuit inchangé | ![avant](avant-realisme-ciel-campagne-nuit.jpg) | ![apres](apres-realisme-ciel-campagne-nuit.jpg) |
