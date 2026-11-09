## Agent : Claude, Agent 0 developer

**Ticket** : WFS-293 — règles : `regles-de-gestion/WFS-293.md`

## Description (à remplir par le développeur avant d'ouvrir la PR)

- **Demande de l'utilisateur** (en une phrase) : À REMPLIR
- **Ce qui change** (fichiers, comportement, à l'écran) : À REMPLIR
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) : À REMPLIR
- **Limites et risques connus** : À REMPLIR

## Captures pour le lead dev (9 après)

Dossier : [`qa/pr/WFS-293`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `carte-ville-virage-1-haut` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carte-ville-virage-1-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carte-ville-virage-1-haut.jpg?raw=true) |
| `carte-ville-virage-1-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carte-ville-virage-1-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carte-ville-virage-1-nuit.jpg?raw=true) |
| `carte-ville-virage-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carte-ville-virage-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carte-ville-virage-1.jpg?raw=true) |
| `carte-ville-virage-2-haut` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carte-ville-virage-2-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carte-ville-virage-2-haut.jpg?raw=true) |
| `carte-ville-virage-2-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carte-ville-virage-2-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carte-ville-virage-2-nuit.jpg?raw=true) |
| `carte-ville-virage-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carte-ville-virage-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carte-ville-virage-2.jpg?raw=true) |
| `carte-ville-virage-3-haut` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carte-ville-virage-3-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carte-ville-virage-3-haut.jpg?raw=true) |
| `carte-ville-virage-3-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carte-ville-virage-3-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carte-ville-virage-3-nuit.jpg?raw=true) |
| `carte-ville-virage-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carte-ville-virage-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carte-ville-virage-3.jpg?raw=true) |

## Ville en virage (revue du lead dev : `pointLisse` change aussi pour le trafic, le duel, la poursuite, l'audio, le joueur)

Banc `ville` (`FILTRE='carte-ville-virage-*' bash tools/qa/captures_pr.sh WFS-293 ville`), avant (`main`) / après (branche) :

| Capture | écart moyen | Avant | Après |
|---|---|---|---|
| `carte-ville-virage-1` | 0,00 % | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carte-ville-virage-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carte-ville-virage-1.jpg?raw=true) |
| `carte-ville-virage-2-nuit` | 0,00 % | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carte-ville-virage-2-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carte-ville-virage-2-nuit.jpg?raw=true) |
| `carte-ville-virage-3-haut` | 0,81 % | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/avant-carte-ville-virage-3-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-293/qa/pr/WFS-293/apres-carte-ville-virage-3-haut.jpg?raw=true) |

Les 9 paires : virages 1 à 3 (jour, nuit) 0,00 % ; vues hautes 0,40 à 0,81 % : seulement les contours (image décalée d'≈ 1 pixel : voiture et caméra déplacées de quelques mm par la B-spline), aucun objet ne change. **Trafic en virage** : la suite du carrefour (ci-dessus, 30 img/s, Poursuite) montre une voiture du trafic dans le virage sur les 5 images, avant comme après, à la même place sur la route.
