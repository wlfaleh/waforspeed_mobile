# WFS-33 — Banc d'essai : scènes de trafic plus probantes
Épic : E3 · Priorité : 7.5 · Dépend de : WFS-08 (✅)

## Besoin (qui, quoi, pourquoi)
Le rapport QA de WFS-08 (remarques 1, 2, 4) demande des captures qui prouvent le trafic : peloton, trafic visible en course,
choc borné. Aucun changement de règle du jeu : seulement les scènes du banc (`src/game/traffic_system.cpp`).

## Règles de gestion
- RG-01 : `trafic-cam0` : joueur derrière 2 à 3 voitures, chacune dans sa file (`LANE_X = [-2/3, 0, 2/3]`), toutes visibles ensemble.
- RG-02 : au moins 3 captures `course-*` sur 6 montrent une voiture du trafic dans le champ, sur la route.
- RG-03 : `trafic-choc` : l'écart latéral de la voiture percutée est borné à ±1,12 comme `crashTraffic` du web ; consigner au Journal si l'arrière sur le trottoir existe aussi dans le web.
- RG-04 : la simulation `traffic.cpp` ne change pas : `mode exact : … 0 écart(s)` inchangé ; aucune voiture du trafic dans le décor.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/game/traffic.js` (`addCars`, `advanceTraffic`, `crashTraffic`, `laneFree`) ; `js/game/update.js` → `hitCar` ; `js/core/config.js` (`LANE_X`, `NEAR_MISS`).
- Voitures : modèles déjà convertis (`assets/cars/*.wfm`, issus de `models/golf-8`, `bmw-m4`, `porsche-911`, `lambo-gallardo`).
- Retour connu : le trafic ne s'interpénètre jamais (`laneFree` + clignotant avant changement de voie).

## Critères d'acceptation (captures du banc PC)
- [ ] `trafic-cam0` : 2 à 3 voitures groupées, une par file
- [ ] ≥ 3 captures `course-*` sur 6 avec voiture du trafic visible
- [ ] journal `trafic-choc` : écart latéral ≤ 1,12 ; constat sur le trottoir noté au Journal
- [ ] `0 écart(s)` du test de fidélité

## Hors périmètre
Toute modification des règles de trafic, rendu des feux stop/clignotants.

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| 7.5 | WFS-33 | Banc d'essai : scènes de trafic plus probantes (peloton, trafic en course, choc borné) | E3 | ✅ Terminé | Claude (session moteur-natif), 2026-11-03 | WFS-08 | `feature/waforspeed-33` | PR #18 | ✅ VALIDÉ par qa-lead |

Critères et notes de suivi :

**WFS-33 — Banc d'essai : scènes de trafic plus probantes (peloton, trafic en course, choc borné)** · E3 · priorité 7.5 · ✅ Terminé (PR #18) (règles : `regles-de-gestion/WFS-33.md`)
Origine : remarques 1, 2 et 4 du [rapport QA WFS-08](qa/rapports/WFS-08.md). Zone : `src/game/traffic_system.cpp`
(scènes du banc uniquement ; la simulation `traffic.cpp` ne change pas, le test de fidélité doit rester à 0 écart). Petite taille.
- [ ] `trafic-cam0` : le joueur roule derrière un **groupe de 2 à 3 voitures** du trafic, visibles ensemble sur la capture,
  chacune dans sa file
- [ ] Captures `course-*` : au moins **3 images sur 6** montrent une voiture du trafic dans le champ, sur la route
  (placement ou densité du trafic près du joueur pour les scènes du banc seulement, pas de changement des règles du jeu)
- [ ] `trafic-choc` (vérification) : le journal donne l'écart latéral de la voiture percutée, borné à ±1,12 comme
  `crashTraffic` du jeu web ; le ticket note au Journal si le dépassement de l'arrière sur le trottoir existe aussi dans
  le jeu web (alors accepté) ou non (alors correction dans la scène ou ticket E3 de correction)
- [ ] Journal du banc : `mode exact : … 0 écart(s)` inchangé ; aucune voiture du trafic dans le décor
