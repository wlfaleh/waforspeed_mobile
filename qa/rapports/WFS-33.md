# QA WFS-33 : Banc d'essai : scènes de trafic plus probantes (peloton, trafic en course, choc borné)

**Verdict : VALIDÉ** (banc d'essai PC), 1er passage. **Fusion autorisée.**

- Construction regardée : commit `3f5b941` (`feature/waforspeed-33`), run Actions 41616586366 réussi, Release
  `apercu-waforspeed-33` (captures du 2026-11-03 ~10:28 UTC, PNG 1280x720).
- Code changé : `src/game/traffic_system.cpp` seulement (scènes du banc) ; `traffic.cpp` inchangé.
- Règles : `regles-de-gestion/WFS-33.md` (RG-01 à RG-04) + remarques 1, 2, 4 de `qa/rapports/WFS-08.md`.

## Critères d'acceptation

| Critère | Capture / journal | Résultat |
|---|---|---|
| RG-01 `trafic-cam0` : 2 à 3 voitures groupées, une par file | `trafic-cam0` : 3 voitures du trafic devant la Golf (blanche à gauche, rouge au centre, blanche/rouge à droite), sur la route, visibles ensemble ; journal « peloton : voiture à 14,5 m file 0 / 21,5 m file -0,67 / 28,5 m file +0,67 » | OK |
| RG-02 ≥ 3 `course-*` sur 6 avec trafic visible sur la route | `course-cam1-0` (break gris, file de droite), `course-cam1-1` (break vert foncé à côté + voiture au loin), `course-cam1-2` (voiture rouge au loin, file de gauche) : 3/6 nettes ; `course-cam0-1` : point minuscule au loin seulement ; `course-cam0-0/2` : rien de lisible | OK (3/6) |
| RG-03 `trafic-choc` : écart latéral ≤ 1,12 + constat trottoir | journal « écart latéral de la voiture percutée +0.905 (borne ±1,12, jeu web identique), 6.8 m du centre » ; capture : voiture grise percutée au bord droit de la chaussée, contre la bordure ; constat au Journal (`stepTraffic` de `js/game/traffic.js` borne à ±1,12, l'arrière sur le trottoir existe aussi dans le web) | OK |
| RG-04 fidélité + décor | journal « mode exact : 39 voitures comparées au jeu web, 0 écart(s) » ; aucune voiture du trafic dans le décor sur les captures regardées | OK |

## Non-régression
`trafic-cam1`, `vitrine-golf-0`, `course-fin` (écran de fin : 1,05 km, 211 km/h, 0 quasi-contact, 1 checkpoint) : rendu
correct, images non déformées, voiture du joueur visible, pas d'écran noir ni de morceau flottant.

## Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — | — | — | Aucune anomalie. |

Observation (pas une anomalie) : en caméra 0 (`course-cam0-*`) la voiture replacée à 90 m est trop petite pour se lire ;
le critère est rempli par la caméra 1.
