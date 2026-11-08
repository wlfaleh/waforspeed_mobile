Agent : Claude, agent 1 (01UY1Vv2)

**Ticket** : WFS-329 — règles : `regles-de-gestion/WFS-329.md` (ANO-178)

## Description

- **Demande de l'utilisateur** (en une phrase) : au menu principal, sans partie, le jeu tourne à 10 img/s (462 appels, 699 k triangles) : le menu et les écrans hors course doivent garder leur fond mais coûter au plus 25 % d'une course et tenir 30 img/s (RG-02).
- **Cause (RG-01)** : au menu et au choix du terrain, aucun système ne prend la scène.
  - `Game::render` redessine donc la course entière derrière l'interface, environ 30 fois par seconde : route, ville, trafic qui roule, foule, conducteurs, voiture du joueur. Ombres, reflet, captures, scène et bloom sont compris, à la même qualité qu'en course.
  - Le menu ne pose qu'un voile en dégradé, presque transparent à droite.
  - Seul le garage a sa propre scène.
- **Ce qui change** :
  - **`src/render/renderer.*`** : `figerScene(bool)` / `sceneFigee()`. Quand le fond est figé et que l'image de scène de l'image précédente est encore valable (cibles non recréées), `endFrame` ne refait que la passe finale : il relit la scène HDR et le bloom gardés, puis dessine l'interface. Ni ombres, ni reflet, ni captures, ni scène, ni bloom. Les horodatages GPU restent écrits (coûts à 0).
  - **`src/game/fond_fige.h`** (règle seule, testée) et **`game.cpp`** :
    - menu et Terrain sans scène dédiée : scène animée pendant 1 s (30 images), puis figée ; aucun dessin 3D tant qu'elle reste valable ;
    - redessinée puis refigée si un réglage du rendu change (qualité, ombres, échelle, MSAA, nuit) ;
    - course, pause, fin, garage et duel ne changent pas.
  - **`qualite_systeme.cpp`** : la ligne « Rendu » affiche « fond figé » au menu.
  - **`bench_perf.cpp`** : banc `WFS_PERF_MENU=ville:nuit,ville:jour` (Basse) : moyennes au menu, captures du menu à 1 s et à 3 s, des Réglages et du Terrain. `WFS_FOND_ANIME=1` rend le fond d'avant avec le même binaire : ce sont les captures « avant ».
  - Aucun bouton changé ; menus, Terrain et garage identiques à l'œil (RG-03).
- **Comment c'est vérifié** :
  - `test_fond_fige` (7 cas : écrans concernés, 1 s avant de figer, retour hors du menu, réglage changé) ; **78 tests CPU verts** ; parcours complet 0 écart (vue Arrière et Cockpit), branche synchronisée avec `main` ;
  - **banc** (lavapipe, Basse, 2340×1080, moyennes au menu après la 1re seconde) :

    | Menu | Appels / image | Triangles / image | Image (banc) |
    |---|---|---|---|
    | Ville de nuit, avant | 306 | 344 k | 626 ms |
    | Ville de nuit, après | **0** | **0** | **88 ms** |
    | Ville de jour, avant | 260 | 347 k | 556 ms |
    | Ville de jour, après | **0** | **0** | **82 ms** |

    Avant, le menu coûtait autant qu'une course (Ville de nuit au départ : 307 appels, 344 k triangles). Après, 0 % au-delà de la 1re seconde ; il ne reste que la passe finale et l'interface.
  - **même image** : la capture du menu figé à 3 s et la dernière image animée (1 s) sont identiques au pixel près (`compare -metric AE` : 0 pixel différent).
- **Limites et risques connus** :
  - Le fond ne bouge plus après 1 s : le trafic et les passants derrière le menu sont arrêtés à l'image (« fond figé rendu une fois », prévu par RG-02).
  - La 1re seconde de chaque arrivée au menu coûte toujours une image de course.
  - Le chiffre de 30 img/s sur le téléphone est à confirmer par l'utilisateur. La cadence hors course reste plafonnée à 30 img/s (WFS-123), et ce qui reste par image, passe finale et interface, ne coûtait que 5 ms sur le téléphone (APK 916).
  - Pas de captures « avant » de `main` pour ce banc, qui est nouveau : les « avant » viennent du même binaire avec `WFS_FOND_ANIME=1`, soit exactement l'ancien rendu.

## Captures pour le lead dev (8 paires ; avant = fond animé d'avant, même binaire)

| Capture | Avant | Après |
|---|---|---|
| `menu-ville-nuit-3s` — Menu, Ville de nuit, 3 s (figé après) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/avant-menu-ville-nuit-3s.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/apres-menu-ville-nuit-3s.jpg?raw=true) |
| `menu-ville-nuit-reglages` — Réglages ouverts sur le menu (ligne « Rendu ») | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/avant-menu-ville-nuit-reglages.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/apres-menu-ville-nuit-reglages.jpg?raw=true) |
| `menu-ville-nuit-terrain` — Choix du terrain | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/avant-menu-ville-nuit-terrain.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/apres-menu-ville-nuit-terrain.jpg?raw=true) |
| `menu-ville-nuit-1s` — Menu, 1re seconde (animé avant et après) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/avant-menu-ville-nuit-1s.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/apres-menu-ville-nuit-1s.jpg?raw=true) |
| `menu-ville-jour-3s` — Menu, Ville de jour, 3 s | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/avant-menu-ville-jour-3s.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/apres-menu-ville-jour-3s.jpg?raw=true) |
| `menu-ville-jour-reglages` — Réglages ouverts sur le menu, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/avant-menu-ville-jour-reglages.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/apres-menu-ville-jour-reglages.jpg?raw=true) |
| `menu-ville-jour-terrain` — Choix du terrain, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/avant-menu-ville-jour-terrain.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/apres-menu-ville-jour-terrain.jpg?raw=true) |
| `menu-ville-jour-1s` — Menu, jour, 1re seconde | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/avant-menu-ville-jour-1s.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-329/qa/pr/WFS-329/apres-menu-ville-jour-1s.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv2MVfXZF3cPf1r8qRq
