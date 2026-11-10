## Agent : Claude, agent dev (01UY1Vv1d6)

**Ticket** : WFS-337 — règles : `regles-de-gestion/WFS-337.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : la nuit, sur la voie rapide (vidéo de la Poursuite, APK 931, ANO-189), la tête du lampadaire est une barre jaune et la boule de lumière flotte à côté, dans le vide : la lumière doit être centrée sur la lampe.
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - **Cause (RG-01)** : chaque candélabre de 12 m du terre-plein central de la voie rapide porte **deux lanternes**, au bout de deux crosses, à 5,0 m l'une de l'autre (`ville.cpp`, l. 1058). Seule celle de notre côté était inscrite pour la nuit (`ville.cpp`, l. 783). La lanterne du sens inverse restait donc une barre jaune sans lumière, et le halo se voyait 5 m à côté, sur l'autre lanterne.
  - `src/render/eclairage.h` (accroche de 1 ligne acceptée par le PO) : nouveau style `StyleLampadaire::Tete` (« tête seule » : halo, sans lumière ni flaque au sol).
  - `src/world/ville.cpp` (accroche de 1 ligne acceptée par le PO, à la fin de `initOuvrages`, rien d'autre) : la 2e lanterne est inscrite en `Tete`, à lat −(RW + AR_DBA + 2,5).
  - `src/render/nuit.cpp` : le halo d'une tête est dessiné par une seule fonction (`haloTete`), pour chaque lanterne. Une `Tete` ne prend pas l'une des 4 lumières ponctuelles et n'a pas de flaque au sol : le coût de rendu ne change pas, en Basse comme en Haute.
  - `src/render/eclairage_physique.cpp` : une `Tete` ne compte pas dans l'éclairement qui règle l'exposition de nuit, qui ne change donc pas.
  - Nouveau banc `WFS_BANC=halos` (`src/render/eclairage_halos_banc.cpp`, accroche d'une ligne dans `desktop_main.cpp`) : Ville de nuit, voie rapide. Il vérifie que chaque candélabre double a ses 2 têtes (erreur sinon) et produit les captures ci-dessous.
  - À l'écran : sur la voie rapide, les deux lanternes de chaque candélabre brillent, chacune avec son halo doux (même rayon de 0,6 m qu'avant, même couleur). Rien d'autre ne change (rue, pont, tunnel, Corniche, Campagne, Grand Prix).
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) :
  - 98 tests CPU réussis (`bash tools/qa/tests_cpu.sh build`).
  - Banc `halos` : sur `main`, « 0 candélabre avec ses 2 têtes, 34 avec une seule : ÉCART » ; sur la branche, « 34 avec leurs 2 têtes, 0 avec une seule : OK ».
  - Position mesurée d'après la géométrie du décor (RG-02) : le halo est à l'aplomb de chaque lanterne (même écart latéral). En hauteur, il est à moins de 0,2 m de la vasque : rue + 0,09 m sous la vasque, voie rapide + 0,16 m au-dessus du centre de la lanterne, pont 0,2 m sous elle. C'est 3 fois moins que le rayon du halo.
  - Captures « avant » : `main` avec le même banc ajouté (le banc n'existe pas sur `main`), « après » : la branche ; Ville de nuit, Graphismes Haute, 1280 × 720.
- **Limites et risques connus** :
  - La tête des candélabres reste un pavé (barre au sodium) : le remplacement des formes nues est dans l'inventaire WFS-378, hors de ce ticket. Aucune forme nouvelle n'est ajoutée : seul un 2e halo, en fondu doux.
  - La hauteur inscrite des lampes n'a pas le même sens partout dans `ville.cpp` (bas de la lanterne ou mât). Je ne l'ai pas changée (zone de WFS-370) : l'écart reste ≤ 0,2 m.
  - Capture `halos-poursuite` : la caméra de la Poursuite est basse, et les têtes sont hors du cadre. Le bandeau « TRAVAUX » s'affiche sur l'image « avant » et pas sur « après » au même chrono : sans lien avec les halos (sujet de WFS-338).
  - `ville.cpp` est aussi modifié par WFS-370 (PR #332) : ma ligne est seule, à la fin de `initOuvrages`. En cas de conflit, il faut garder les deux.
  - Ticket 🟡 après la fusion : défaut vu par l'utilisateur, à valider sur son téléphone.

## Captures pour le lead dev (10 après)

Dossier : [`qa/pr/WFS-337`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `halos-160-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/avant-halos-160-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/apres-halos-160-0.jpg?raw=true) |
| `halos-160-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/avant-halos-160-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/apres-halos-160-1.jpg?raw=true) |
| `halos-160-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/avant-halos-160-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/apres-halos-160-2.jpg?raw=true) |
| `halos-160-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/avant-halos-160-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/apres-halos-160-3.jpg?raw=true) |
| `halos-160-4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/avant-halos-160-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/apres-halos-160-4.jpg?raw=true) |
| `halos-160-5` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/avant-halos-160-5.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/apres-halos-160-5.jpg?raw=true) |
| `halos-50m` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/avant-halos-50m.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/apres-halos-50m.jpg?raw=true) |
| `halos-poursuite` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/avant-halos-poursuite.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/apres-halos-poursuite.jpg?raw=true) |
| `halos-pres` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/avant-halos-pres.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/apres-halos-pres.jpg?raw=true) |
| `halos-rue` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/avant-halos-rue.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/apres-halos-rue.jpg?raw=true) |
| `halos-pres-zoom` (×6, têtes du candélabre) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/avant-halos-pres-zoom.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-337/qa/pr/WFS-337/apres-halos-pres-zoom.jpg?raw=true) |
