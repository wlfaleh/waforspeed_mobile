# WFS-281 — captures avant / après (2e passage, après la 2e revue du lead dev)

Agent : agent arwa (Claude, session trafic). Banc `perf` (pilote automatique, vue du joueur), `WFS_PERF_AVANCE` = 0 / 10000 / 36000,
`WFS_PERF_AVANCE_RENDU=0`, `WFS_PERF_CHAUFFE=5`, `WFS_PERF_IMAGES=90` (médianes du trafic sur ces 90 images, décision du PO du 2026-11-08) ;
nuit : `WFS_MOMENT=nuit`. Dernière image de la mesure, réduite en JPG (1280 px, qualité 85), même format que `tools/qa/captures_pr.sh`.
**Avant** = branche synchronisée, avec `traffic.*` et `traffic_system.*` de `origin/main` (et seulement la mesure des médianes ajoutée) ;
**après** = la branche (158cdee). `keepTrafficInView` est celui de `main` dans les deux binaires.
Golf sur la chaussée : sur toutes les captures « après » en Ville (2 min 50, 10 min, Basse, nuit), en Corniche et au Grand Prix. En Campagne, à 10 min, elle est dans l'herbe.
Sur `main` aussi, le pilote automatique monte sur le trottoir (capture « avant » de Ville à 2 min 50).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `ville-haute-depart` — Ville, Haute, départ | ![avant](avant-ville-haute-depart.jpg) | ![après](apres-ville-haute-depart.jpg) |
| `ville-haute-2min50` — Ville, Haute, ≈ 2 min 50 (10000 images) | ![avant](avant-ville-haute-2min50.jpg) | ![après](apres-ville-haute-2min50.jpg) |
| `ville-haute-10min` — Ville, Haute, 10 min (36000 images) | ![avant](avant-ville-haute-10min.jpg) | ![après](apres-ville-haute-10min.jpg) |
| `ville-basse-10min` — Ville, Basse, 10 min | ![avant](avant-ville-basse-10min.jpg) | ![après](apres-ville-basse-10min.jpg) |
| `ville-nuit-haute-10min` — Ville, nuit, Haute, 10 min (phares WFS-226) | ![avant](avant-ville-nuit-haute-10min.jpg) | ![après](apres-ville-nuit-haute-10min.jpg) |
| `corniche-haute-10min` — Corniche, Haute, 10 min | ![avant](avant-corniche-haute-10min.jpg) | ![après](apres-corniche-haute-10min.jpg) |
| `campagne-haute-10min` — Campagne, Haute, 10 min | ![avant](avant-campagne-haute-10min.jpg) | ![après](apres-campagne-haute-10min.jpg) |
| `grandprix-haute-10min` — Grand Prix, Haute, 10 min | ![avant](avant-grandprix-haute-10min.jpg) | ![après](apres-grandprix-haute-10min.jpg) |
