## Agent : Claude, agent devartiste 3 (01UY1Vv59N)

**Ticket** : WFS-413 — règles : `regles-de-gestion/WFS-413.md` (anomalie ANO-240, P1 super urgent)

## Description

- **Demande de l'utilisateur** : « Fixer les feux devant des voitures : il faut que ça sorte par les vraies lampes des voitures, c'est trop artificiel. »
- **Ce qui change** :
  - **Optiques repérées voiture par voiture** (RG-01, RG-04) : table `PHARES_AVANT` dans `src/render/correctifs_voitures.h` pour les 17 voitures permises (matériaux « lampe » et « glace », boîte en miroir, nombre de matériaux du .wfm vérifié). Chaque optique a été repérée sur le modèle vu de face et contrôlée à l'œil. `feux_modele::mesurerOptiques` (`feux_modele.h`) calcule le centre de chaque glace, sa normale et la taille de la lampe au chargement (`model_loader.cpp`). Il n'y a plus de point calculé au pare-chocs ni de repli aux proportions du jeu web.
  - **La lumière sort des optiques** (RG-02) : les deux feux de croisement du joueur (`nuit.cpp`) partent du centre de chaque glace, 1 cm derrière elle. Les voitures du trafic, les adversaires et la police (`phares::noterAutre`) partent du milieu des deux glaces mesurées. L'axe, la coupure et les intensités ne changent pas (WFS-218, WFS-382).
  - **La glace du modèle s'allume** (RG-03) : `feux_modele::marquerPhares` sépare les triangles de la lampe en pièces « phare » (`PIECE_PHARE`), que `Renderer::draw` allume la nuit avec la même émission qu'avant. On voit la vraie forme de l'optique (DEL de la M4, bandeau de la Golf, phares ronds de la 911…). Les halos ronds `PHARE_COEUR` / `PHARE_LUEUR` sont **retirés** de `Renderer::feuxVoiture` : plus aucun disque devant ni à côté des voitures. En Basse, au loin, un 5e groupe « optiques » garde les phares allumés.
  - **Masquage « boîtier »** (`shaders/eclairage.glsl`, 2 lignes, accord du PO du 2026-11-10) : un feu de croisement n'éclaire rien à moins de 40 cm devant sa glace. Sans ce masquage, la lampe aurait éclairé le pare-chocs de sa propre voiture.
  - **Feux arrière, stop et recul inchangés** (RG-07) : `pieceFeuArriere` ne garde que les valeurs 1 et 2.
  - **Banc** `WFS_BANC=phares-optiques` (`feux_modele.cpp`, 1 ligne d'accroche dans `desktop_main.cpp`) et **test** `tests/test_phares_optiques.cpp`.
- **Comment c'est vérifié** :
  - `test_phares_optiques` : un cas construit retrouve le centre à 2 mm. Sur les 17 voitures, la lumière part à **1,0 cm au plus** de l'optique (règle : ≤ 2 cm), les deux côtés sont symétriques, et chaque phare est à l'avant, à une hauteur de phare. L'ancien point était à **24 à 58 cm** du vrai phare.
  - Tests CPU : 100 réussis. Seul `test_course5_ordinateurs` échoue, **de la même façon sur `main`** (Course à 5 sur le circuit, sans lien avec ce ticket).
  - Banc perf de nuit, Ville, Basse (`WFS_PERF_SCENE`, lavapipe), avant → après : GPU 449,9 → 448,0 ms (départ) et 282,2 → 282,1 ms (Mimosas) ; triangles identiques ; appels de dessin 273 → 276 et 129 → 130.
  - Captures avant / après ci-dessous, toutes regardées.
- **Limites et risques connus** :
  - `main` n'a pas encore le banc `phares-optiques` : les captures « avant » viennent d'une copie de `main` avec ce banc seul ajouté, sans aucun changement de rendu.
  - Le trafic du jeu roule dans le même sens que le joueur. La capture « trafic de face » est donc prise devant le joueur, en regardant vers l'arrière.
  - S3 : son optique est une face noire opaque dans le modèle d'origine, sans glace transparente. Elle s'allume donc d'un bloc.
  - Les 3 voitures retirées par WFS-404 (A 45, Elantra N, X5) n'ont pas d'optiques repérées (ordre permanent du PO) : leur ancien repli MAT_HEAD reste. Ma branche sera synchronisée dès la fusion de WFS-404.
  - Il y a +1 à +3 appels de dessin par image en Basse (pièces « phare » séparées), pour un GPU égal ou plus bas.
  - Statut après la fusion : **🟡 À valider (utilisateur)** sur l'APK de `main`.

## Captures pour le lead dev (15 après, 15 avant)

Dossier : [`qa/pr/WFS-413`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413). Ordre de la planche : golf, m4, p911, gallardo, p208, abarth, tcross, compass, s3, giulia, model3, corsa, gt86, v60, rrsport, mustang, sf90.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `phares-planche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/avant-phares-planche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/apres-phares-planche.jpg?raw=true) |
| `phares-gros-plan-golf` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/avant-phares-gros-plan-golf.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/apres-phares-gros-plan-golf.jpg?raw=true) |
| `phares-gros-plan-m4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/avant-phares-gros-plan-m4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/apres-phares-gros-plan-m4.jpg?raw=true) |
| `phares-gros-plan-p911` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/avant-phares-gros-plan-p911.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/apres-phares-gros-plan-p911.jpg?raw=true) |
| `phares-joueur-face` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/avant-phares-joueur-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/apres-phares-joueur-face.jpg?raw=true) |
| `phares-trafic-face` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/avant-phares-trafic-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/apres-phares-trafic-face.jpg?raw=true) |
| `phares-poursuite-face` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/avant-phares-poursuite-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/apres-phares-poursuite-face.jpg?raw=true) |
| `phares-poursuite-police` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/avant-phares-poursuite-police.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/apres-phares-poursuite-police.jpg?raw=true) |
| `phares-poursuite-basse-police` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/avant-phares-poursuite-basse-police.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/apres-phares-poursuite-basse-police.jpg?raw=true) |
| `phares-ville-face` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/avant-phares-ville-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/apres-phares-ville-face.jpg?raw=true) |
| `phares-ville-face-pluie` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/avant-phares-ville-face-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/apres-phares-ville-face-pluie.jpg?raw=true) |
| `phares-campagne-capot` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/avant-phares-campagne-capot.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/apres-phares-campagne-capot.jpg?raw=true) |
| `phares-campagne-capot-haut` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/avant-phares-campagne-capot-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/apres-phares-campagne-capot-haut.jpg?raw=true) |
| `phares-jour-golf` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/avant-phares-jour-golf.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/apres-phares-jour-golf.jpg?raw=true) |
| `phares-ville-jour-proche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/avant-phares-ville-jour-proche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-413/qa/pr/WFS-413/apres-phares-ville-jour-proche.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv59Njuxiw5KHWKQ52C
