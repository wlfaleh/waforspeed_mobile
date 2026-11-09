Agent : Claude, agent dev Z (01UY1Vv5sWHo)

**Ticket** : WFS-356 [URGENT] Départ de nuit éblouissant, 2e passage (WFS-328 refusé) (ANO-202, + ANO-179, ANO-184). Règles : `regles-de-gestion/WFS-356.md`.

## Demande de l'utilisateur
Au départ d'une course de nuit dans la Ville (compte à rebours 3-2-1, APK 954), les façades sont des blocs blancs éblouissants avec des halos énormes : il faut une nuit lisible dès la 1re image.

## Cause (RG-01, RG-04)
1. **Exposition** : au départ de la Ville, le posemètre de nuit (WFS-230) ne compte que les lampadaires : **1,57 lx reçus**. La part « rue éclairée » est donc partielle, et l'exposition reste à **EV 2,27** pendant tout le décompte. Or la mesure du lieu donne **EV 4,1** à 0:02 : l'image est 1,8 IL trop claire (3,5 fois). WFS-328 n'a pas vu ce défaut, parce que son contrôle comparait l'EV appliqué au **même** EV visé (2,27 contre 2,27).
2. **Vitrines des commerces** : panneau émissif plat (`neon(0xffcf8a, 1,4)`, `src/world/commerces.cpp`). À l'exposition de la rue, son rouge vaut 1,4 : au-dessus du blanc et du seuil du bloom. Ce sont les grands blocs blancs du rez-de-chaussée et leurs halos.
3. **Fenêtres et vitrines du kit d'immeubles** : la pièce simulée derrière la vitre (murs 31,8 cd/m², plafond ≈ 58) suit l'exposition sans limite : 1,5 à 3 fois le blanc à EV 4, 5 fois à EV 2,27.
4. **Halo des têtes de lampe** : disque de 2,2 m de diamètre pour une tête de 0,90 × 0,36 m.

Le banc `depart` de WFS-328 tournait en Haute à 100 %. Le téléphone était en Basse à 60 % : cette variante est maintenant ajoutée au banc. Les causes 1 à 3 sont les mêmes dans les deux qualités.

## Ce qui change
- `src/render/eclairage_exposition.h`, `src/render/eclairage_physique.cpp` : `partRueNuit`. Dans la Ville, éclairée partout, l'exposition de nuit reste celle d'une rue éclairée (EV 3 à 5). Ailleurs (Campagne, Corniche, Plage, Grand Prix), rien ne change.
- `shaders/eclairage.glsl` : `pieceAdaptee`, une adaptation locale de l'œil aux pièces éclairées vues de la rue (genou 0,2, épaule de Reinhard vers 0,6, teinte gardée ; au plus 212/255, jamais saturé, à peine au-dessus du seuil du bloom). Mêmes constantes en C++, vérifiées par le test.
- **Accroches hors zone, une ligne chacune, annoncées au journal** :
  - `shaders/decor.glsl` : la pièce passe par `pieceAdaptee` ;
  - `src/world/commerces.cpp` : vitrine de nuit à 0,65 au lieu de 1,4 (luminance exposée ≈ 0,44, comme une pièce adaptée).
- `src/render/nuit.cpp` : halo des têtes de lampe de la Ville de 0,6 m de rayon (avant 1,1 m), soit ≤ 3 fois la tête.
- `src/render/eclairage_depart_banc.cpp` :
  - variante `WFS_DEPART_SEUL=basse` (Basse, scène à 60 %, vues Arrière et Cockpit) ;
  - nouveau contrôle : la 1re image doit être à 1 IL au plus de l'EV de 0:04. Sur `main`, il échoue pour les 5 départs de nuit dans la Ville (1,6 à 1,8 IL) ; sur la branche, il passe partout.
- `tests/fidelite_lumiere.cpp` :
  - Ville à 1,57 lx → EV 4,00 (hors Ville inchangé) ;
  - pièces éclairées sous 220/255 de EV 2 à 5 ;
  - courbe continue et croissante ;
  - constantes identiques à celles du shader.
- **À l'écran** : de nuit dans la Ville, dès le « 3 », les façades sont lisibles, les vitrines sont d'un crème chaud sans halo et la rue est exposée comme à 0:04. **Le jour ne change pas.**

## Comment c'est vérifié
- Construction complète. **88 tests CPU réussis** (`bash tools/qa/tests_cpu.sh build`), dont `test_fidelite_lumiere` (119 vérifications).
- Banc `depart` (Basse 60 %, départs Ville / Plage / Campagne / jour, « Recommencer » et reprise, Cockpit) : **0 écart d'exposition**. L'EV vaut 4,00 dès la 1re image dans la Ville, au lieu de 2,27.
- Mesures RG-05 sur chaque capture de nuit (tableau ci-dessous) :
  - **0,00 % de pixels saturés** (R, G et B ≥ 250) ;
  - pixels très clairs (Y ≥ 230, HUD compris, qui en fait déjà 0,3 à 1 %) : de **4,7 à 6,8 % → ≈ 1 %** au « 3 ».
- Jour (RG-06) : `depart-jour-ville` et `carte-ville-autoroute` sont **identiques au pixel près** (écart 0). La Campagne de nuit aussi.

| Capture | EV avant | EV après | saturés R,G,B ≥ 250 avant → après | très clairs Y ≥ 230 avant → après (HUD compris) | luminance moyenne avant → après |
|---|---|---|---|---|---|
| `depart-basse60-nuit-ville-d3` | 2.27 | 4.00 | 0.00 % → 0.00 % | 5.53 % → 1.00 % | 51 → 38 |
| `depart-basse60-nuit-ville-2` | 3.82 | 4.08 | 0.00 % → 0.00 % | 0.65 % → 0.39 % | 60 → 51 |
| `depart-basse60-nuit-ville-4` | 4.06 | 4.08 | 0.00 % → 0.00 % | 0.41 % → 0.40 % | 64 → 60 |
| `depart-basse60-cockpit-nuit-ville-d3` | 2.27 | 4.00 | 0.00 % → 0.00 % | 4.71 % → 0.98 % | 48 → 36 |
| `depart-nuit-ville-d3` | 2.27 | 4.00 | 0.00 % → 0.00 % | 6.75 % → 1.02 % | 81 → 42 |
| `depart-nuit-ville-4` | 4.11 | 4.18 | 0.00 % → 0.00 % | 2.06 % → 0.42 % | 73 → 62 |
| `depart-cockpit-nuit-ville-d3` | 2.27 | 4.00 | 0.00 % → 0.00 % | 5.86 % → 1.04 % | 77 → 40 |
| `depart-recommencer-nuit-ville-d3` | 2.27 | 4.00 | 0.01 % → 0.00 % | 6.81 % → 1.09 % | 103 → 60 |
| `depart-reprise-nuit-ville` | 4.12 | 4.18 | 0.00 % → 0.00 % | 1.62 % → 0.43 % | 61 → 53 |
| `carte-ville-autoroute-nuit` | — | — | 0.00 % → 0.00 % | 1.43 % → 1.00 % | 30 → 24 |
| `depart-jour-ville-d3` | 14.76 | 14.76 | 0.13 % → 0.13 % | 1.66 % → 1.66 % | 93 → 93 |

## Limites et risques
- **ANO-184** (ciel violet sur la voie rapide, Poursuite) : pas reproduit au banc, où le ciel de la voie rapide est déjà noir sur `main`. La cause probable est la même : une exposition de rase campagne (jusqu'à EV −3,5) était possible dans la Ville, et elle ne l'est plus. **À confirmer sur le téléphone.**
- **ANO-179** (image délavée en pleine course) : dans la Ville, l'EV ne descend plus sous 3. Je n'ai pas relancé la course longue de 4 min (`WFS_DEPART_SEUL=longue`).
- **Plage de nuit** : 0,38 % des pixels changent : les halos des lampes de style Ville y sont plus petits (vérifié par différence d'images). Dans la Plage, l'EV passe de 4,0 à 2,7 vers 0:04, comme sur `main` (hors Ville, rien n'a changé).
- **Performance en Basse (RG-06)** : non mesurée sur un vrai GPU (le banc est en rendu logiciel). Le shader ajoute une branche sur les seuls pixels de pièce allumée, et aucun appel de dessin.
- Les vitrines restent des panneaux unis (pas d'intérieur dessiné) : c'est hors périmètre.
- Après la fusion : **🟡 À valider (utilisateur)** sur le téléphone, de nuit dans la Ville, mode Checkpoint, au « 3 », au « 2 » et à 0:04, en Basse et en Haute.

## Captures pour le lead dev (16 paires, avant main / après branche)
Dossier : [`qa/pr/WFS-356`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356). Banc PC, même rendu que la CI.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `depart-basse60-nuit-ville-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-depart-basse60-nuit-ville-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-depart-basse60-nuit-ville-d3.jpg?raw=true) |
| `depart-basse60-nuit-ville-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-depart-basse60-nuit-ville-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-depart-basse60-nuit-ville-2.jpg?raw=true) |
| `depart-basse60-nuit-ville-4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-depart-basse60-nuit-ville-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-depart-basse60-nuit-ville-4.jpg?raw=true) |
| `depart-basse60-cockpit-nuit-ville-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-depart-basse60-cockpit-nuit-ville-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-depart-basse60-cockpit-nuit-ville-d3.jpg?raw=true) |
| `depart-basse60-cockpit-nuit-ville-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-depart-basse60-cockpit-nuit-ville-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-depart-basse60-cockpit-nuit-ville-2.jpg?raw=true) |
| `depart-nuit-ville-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-depart-nuit-ville-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-depart-nuit-ville-d3.jpg?raw=true) |
| `depart-nuit-ville-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-depart-nuit-ville-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-depart-nuit-ville-1.jpg?raw=true) |
| `depart-nuit-ville-4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-depart-nuit-ville-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-depart-nuit-ville-4.jpg?raw=true) |
| `depart-cockpit-nuit-ville-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-depart-cockpit-nuit-ville-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-depart-cockpit-nuit-ville-d3.jpg?raw=true) |
| `depart-recommencer-nuit-ville-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-depart-recommencer-nuit-ville-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-depart-recommencer-nuit-ville-d3.jpg?raw=true) |
| `depart-reprise-nuit-ville` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-depart-reprise-nuit-ville.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-depart-reprise-nuit-ville.jpg?raw=true) |
| `carte-ville-autoroute-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-carte-ville-autoroute-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-carte-ville-autoroute-nuit.jpg?raw=true) |
| `depart-nuit-plage-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-depart-nuit-plage-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-depart-nuit-plage-d3.jpg?raw=true) |
| `depart-nuit-campagne-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-depart-nuit-campagne-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-depart-nuit-campagne-d3.jpg?raw=true) |
| `depart-jour-ville-d3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-depart-jour-ville-d3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-depart-jour-ville-d3.jpg?raw=true) |
| `depart-jour-ville-4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/avant-depart-jour-ville-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-356/qa/pr/WFS-356/apres-depart-jour-ville-4.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv5sWHosB6bDdrEzY21
