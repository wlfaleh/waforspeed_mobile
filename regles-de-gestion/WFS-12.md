# WFS-12 — Règles de course : chrono, checkpoints, score, nitro, quasi-contacts
Épic : E3 · Priorité : 7 · Dépend de : WFS-08 (✅)

## Besoin (qui, quoi, pourquoi)
Rendre la course jouable comme le mode solo web : temps limité, checkpoints qui rallongent le temps, score,
combos de quasi-contacts, nitro, fin de partie. Aujourd'hui `Player` ne porte que la conduite.

## Règles de gestion
- RG-01 : départ de solo : `timeLeft = 30` s, `nitro = 0.4`, compte à rebours 3-2-1-GO (`trafficClock` part de −3), vitesse 0 pendant le compte.
- RG-02 : checkpoint tous les `CHECK_DIST = 100000` unités (= 600 m) : `add = max(5, 11 − level×0.4)` s ajoutées, `level++`, +6 voitures si `cars < 110`.
- RG-03 : fin quand `timeLeft ≤ 0` : écran de fin avec score, distance (`dist×0.006/1000` km), vitesse max, quasi-contacts, checkpoints.
- RG-04 : `score = floor(dist×0.006)×5 + bonus` ; `kmh = round(speed/MAX_SPEED×260)`.
- RG-05 : quasi-contact : voiture dépassée avec écart latéral < `touchLat + 0.24`, `speed > 0.5×MAX_SPEED`, voiture non accidentée ; `combo` +1 si `comboTimer > 0` (3 s) sinon 1 ; points `250×combo` ; nitro `+PERF.nitroGain` (0,12, plafonné à 1).
- RG-06 : choc léger : combo perdu si impact > 0,08 ; accident si `impact/PERF.mass > 0.28` (tête-à-queue, `controlLock`, ralenti ×0,35 pendant 0,5 s) ; décor percuté (`crash`) : recule de 300, vitesse `MAX_SPEED/8`.
- RG-07 : nitro : consommation `PERF.nitroBurn` (0,26/s), vitesse max ×1,3 (`nitroMult`), poussée ×1,6 ; actif seulement si `nitro > 0.01`.
- RG-08 : carte rejouée avec la même graine de trafic ; tests automatiques comparant à des valeurs de référence du JS.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/game/update.js` → `update` (checkpoints, nitro, quasi-contacts), `hitCar`, `playerAccident`, `bump`, `crash` ; `js/game/state.js` → `resetRace`, `score`, `kmh` ; `js/ui/screens.js` → `startRace`, `endRace` ; `js/core/config.js` (constantes) ; `js/cars/catalog.js` → `carPerf`.
- Les collisions voiture/trafic existent déjà (WFS-08) : brancher, ne pas réécrire.
- Retours connus : accidents = tête-à-queue puis remise dans l'axe, caméra qui revient derrière la voiture.

## Critères d'acceptation (captures / journal du banc PC)
- [ ] journal : temps, checkpoints et score d'une course automatique identiques au JS (tolérance nulle sur les entiers)
- [ ] capture : décompte « 3 » puis « GO » (texte de WFS-26 si livré, sinon journal)
- [ ] test unitaire : checkpoint 1 → +11 s, checkpoint 10 → +7 s, quasi-contact ×3 → 750 points

## Hors périmètre
Affichage du HUD (WFS-27), sauvegarde du meilleur score (WFS-24), duel (WFS-21).
