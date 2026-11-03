# WFS-55 — Correction : PI en double dans le tracé de la route (fidélité au jeu web)
Épic : E3 · Priorité : 12.05 · Dépend de : WFS-14 (apporte `tests/test_cartes.cpp`) · Taille : ≈ 1 h · Origine : Journal « WFS-14 : code écrit »

## Besoin (qui, quoi, pourquoi)
`easeInOut` de `src/game/track.cpp` multiplie par `PI` en float alors que le jeu web utilise `Math.PI` (double) : les
altitudes de la route dérivent de 3 µm au plus. Invisible pour le joueur, mais le test de fidélité du tracé doit
tolérer 1e-3 au lieu de 1e-9, ce qui masquerait de vrais écarts sur les cartes à venir (Campagne WFS-53, Grand Prix WFS-54).

## Règles de gestion
- RG-01 : dans `src/game/track.cpp`, `easeIn`, `easeOut`, `easeInOut` calculent en double avec un PI double
  (`std::numbers::pi`), comme `Util.easeInOut` du jeu web ; aucun autre changement de formule.
- RG-02 : le `PI` float du noyau (`src/core`) ne change pas (hors zone E3).
- RG-03 : `tests/test_cartes.cpp` passe sa tolérance de 1e-3 à **1e-9** ; tous les tests (`test_*`) à 0 écart.
- RG-04 : les captures de référence du banc qui changent (≈ 27 d'après la mesure de WFS-14, ville comprise) sont
  renouvelées dans le même ticket ; le compte rendu liste chaque capture renouvelée et son écart (attendu : quelques
  pixels, aucune différence visible à l'œil).
- RG-05 : à fusionner hors d'une période où un autre ticket compare ses captures à `main` sans resynchroniser :
  annoncer au Journal la fusion pour que les tickets en cours se resynchronisent.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/util.js` → `Util.easeIn` / `easeOut` / `easeInOut` (référence exacte, `Math.PI`) ; `js/road.js` / `maps.js`
  (tracés déjà portés par WFS-14 dans `tests/test_cartes.cpp`). Rien d'autre à porter.

## Critères d'acceptation (vérifiables sur les captures du banc PC)
- [ ] `test_cartes` OK en tolérance 1e-9 (ville, plage, campagne, circuit) ; autres tests OK
- [ ] captures renouvelées : comparaison avant / après sans différence visible (route, horizon, voiture à la même place)
- [ ] aucune autre capture modifiée hors de la liste du compte rendu

## Hors périmètre
- Tout changement du `PI` du noyau, de la physique ou du décor ; nouvelles cartes (WFS-53, WFS-54).
