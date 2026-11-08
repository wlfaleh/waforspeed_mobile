# WFS-281 — captures avant / après (3e passage, binaire 71e540e)

Agent : agent arwa (Claude, session trafic). Banc `perf` (pilote automatique, vue du joueur), `WFS_PERF_AVANCE` = 0 / 10000 / 10600 / 36000,
`WFS_PERF_AVANCE_RENDU=0`, `WFS_PERF_CHAUFFE=5`, `WFS_PERF_IMAGES=90` ; nuit : `WFS_MOMENT=nuit`. Dernière image de la mesure, réduite en JPG
(1280 px, qualité 85), même format que `tools/qa/captures_pr.sh`.
**Avant** = `traffic.*` et `traffic_system.*` de `origin/main` (avec seulement la mesure des médianes en plus) ; **après** = la branche, 71e540e.
`keepTrafficInView` est celui de `main` dans les deux binaires.
**Golf sur la chaussée** dans toutes les captures « après », sauf à 10000 images en Ville (≈ 2 min 50). À cet instant, la Golf de `main` sort aussi de la route,
et à 10600 images (≈ 2 min 57) c'est encore `main` qui est sur le trottoir : c'est le pilote automatique du banc (hors zone), à trafic égal, pas ce ticket.
Les paires à 10600 montrent l'après sur la chaussée.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `ville-haute-depart` — Ville, Haute, départ | ![avant](avant-ville-haute-depart.jpg) | ![après](apres-ville-haute-depart.jpg) |
| `ville-haute-2min50` — Ville, Haute, ≈ 2 min 50 (10000 images) : la Golf sort de la route, avant comme après | ![avant](avant-ville-haute-2min50.jpg) | ![après](apres-ville-haute-2min50.jpg) |
| `ville-basse-2min50` — Ville, Basse, ≈ 2 min 50 : la Golf sort de la route, avant comme après | ![avant](avant-ville-basse-2min50.jpg) | ![après](apres-ville-basse-2min50.jpg) |
| `ville-haute-2min57` — Ville, Haute, ≈ 2 min 57 (10600 images) | ![avant](avant-ville-haute-2min57.jpg) | ![après](apres-ville-haute-2min57.jpg) |
| `ville-basse-2min57` — Ville, Basse, ≈ 2 min 57 | ![avant](avant-ville-basse-2min57.jpg) | ![après](apres-ville-basse-2min57.jpg) |
| `ville-haute-10min` — Ville, Haute, 10 min (36000 images) | ![avant](avant-ville-haute-10min.jpg) | ![après](apres-ville-haute-10min.jpg) |
| `ville-basse-10min` — Ville, Basse, 10 min | ![avant](avant-ville-basse-10min.jpg) | ![après](apres-ville-basse-10min.jpg) |
| `ville-nuit-haute-10min` — Ville, nuit, Haute, 10 min (phares WFS-226) | ![avant](avant-ville-nuit-haute-10min.jpg) | ![après](apres-ville-nuit-haute-10min.jpg) |
| `ville-nuit-basse-10min` — Ville, nuit, Basse, 10 min | ![avant](avant-ville-nuit-basse-10min.jpg) | ![après](apres-ville-nuit-basse-10min.jpg) |
| `corniche-haute-10min` — Corniche, Haute, 10 min | ![avant](avant-corniche-haute-10min.jpg) | ![après](apres-corniche-haute-10min.jpg) |
| `campagne-haute-10min` — Campagne, Haute, 10 min | ![avant](avant-campagne-haute-10min.jpg) | ![après](apres-campagne-haute-10min.jpg) |
| `grandprix-haute-10min` — Grand Prix, Haute, 10 min | ![avant](avant-grandprix-haute-10min.jpg) | ![après](apres-grandprix-haute-10min.jpg) |
