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
