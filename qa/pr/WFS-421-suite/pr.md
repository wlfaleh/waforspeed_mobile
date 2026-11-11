Agent : lead-dev (session 01UY1Vv29v), codé sur ordre de l'utilisateur

**Ticket** : WFS-421 (suite) — plus de disque au sol des lampadaires, à toutes les qualités. Règles : `regles-de-gestion/WFS-421.md`.

## Description
- **Demande de l'utilisateur** (2026-11-11) : « pourquoi tu as accepté le 421 alors qu'il y a un grand cercle comme ça ; c'est dans le périmètre de la tâche, prends tes responsabilités, corrige et fusionne ». La PR #346 n'avait coupé le disque orange qu'en Basse. En Moyenne et en Haute, il restait au pied de chaque lampadaire un grand cercle orange à bord net : c'est une forme nue, interdite par la règle du 2026-11-09.
- **Ce qui change** :
  - `src/render/nuit.cpp` : la « flaque » additive au sol (`r.haloSol`, héritée du `poolQuad` du jeu web) est **supprimée à toutes les qualités**. La boucle des lampadaires ne dessine plus que le petit halo de la tête (style Ville et « têtes seules », comme avant, RG-02). La fonction `lin()`, devenue inutile, est retirée.
  - `src/render/nuit_qualite.h` : `haloSolLampadaire` est retirée, il ne reste que `haloTeteLampadaire`.
  - `tests/test_nuit_qualite.cpp` (+ `WFS_SOURCE_DIR` dans `CMakeLists.txt`) : le test vérifie que `nuit.cpp` n'appelle plus `r.haloSol`, et que la tête est gardée à toutes les qualités.
  - `src/render/nuit_halo_sol_banc.cpp` : commentaire mis à jour.
  - La vraie lumière des lampadaires (projecteurs du shader) ne change pas. `Renderer::haloSol` reste dans le noyau, mais n'est plus appelé.
- **Comment c'est vérifié** :
  - Construction sans erreur ni avertissement ; tests CPU : **tous verts** (105), dont `test_nuit_qualite`.
  - Banc `halos-sol` (1280 × 720, nuit) : en Basse, avant et après **identiques à l'octet près** (déjà sans disque depuis #346). En Moyenne et en Haute (Ville, Corniche), le disque disparaît.
  - Appels de dessin, Corniche : Moyenne 294, Haute 619 ; Basse 233 (inchangé).
- **Limites et risques** :
  - En Moyenne et en Haute, la chaussée juste derrière la voiture est plus sombre (plus de disque). Elle reste éclairée près des mâts et par les phares. Aucune forme n'est ajoutée pour compenser (RG-03).
  - Le banc du matin `luminance_route.py` compare « avec reflet / sans reflet » : le disque était présent dans les deux images, le rapport ne devrait presque pas bouger. À surveiller sur les bancs du matin.
  - Statut 🟡 À valider (utilisateur), avec le gain mesuré sur le S21 FE (RG-04).

## Captures (avant = `main` après #346, après = cette branche)

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `halosol-ville-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421-suite/qa/pr/WFS-421-suite/avant-halosol-ville-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421-suite/qa/pr/WFS-421-suite/apres-halosol-ville-basse.jpg?raw=true) |
| `halosol-ville-moyenne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421-suite/qa/pr/WFS-421-suite/avant-halosol-ville-moyenne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421-suite/qa/pr/WFS-421-suite/apres-halosol-ville-moyenne.jpg?raw=true) |
| `halosol-ville-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421-suite/qa/pr/WFS-421-suite/avant-halosol-ville-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421-suite/qa/pr/WFS-421-suite/apres-halosol-ville-haute.jpg?raw=true) |
| `halosol-plage-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421-suite/qa/pr/WFS-421-suite/avant-halosol-plage-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421-suite/qa/pr/WFS-421-suite/apres-halosol-plage-basse.jpg?raw=true) |
| `halosol-plage-moyenne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421-suite/qa/pr/WFS-421-suite/avant-halosol-plage-moyenne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421-suite/qa/pr/WFS-421-suite/apres-halosol-plage-moyenne.jpg?raw=true) |
| `halosol-plage-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421-suite/qa/pr/WFS-421-suite/avant-halosol-plage-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421-suite/qa/pr/WFS-421-suite/apres-halosol-plage-haute.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv29v9kzSBx4vyVKcD8
