# WFS-14 — Cartes campagne, plage, circuit
Épic : E2 · Priorité : 12 · Dépend de : WFS-06 (✅)

## Besoin (qui, quoi, pourquoi)
Le joueur peut choisir entre quatre cartes. La ville existe (WFS-06) ; il faut ajouter Corniche (plage), Campagne et
Grand Prix (circuit), avec leur décor, leur sol et leur ambiance.

## Règles de gestion
- RG-01 : valeurs des cartes (`MAPS`) : plage `seed 301, curves [1,2,3], hills 0, sections 32, edge 1.6, trafic n 34 (0,3-0,55)` ; campagne `seed 512, curves [2,3,4], hills 120, sections 36, edge 1.38, trafic 26 (0,28-0,5)` ; circuit `seed 777, curves [3,5,6], hills 12, sections 34, edge 1.66, trafic 16 (0,5-0,78)` ; ville : `seed 88`, trafic 42.
- RG-02 : zones tous les `ZONE_LEN = 448` tronçons (≈ 540 m) : plage = Corniche, Port de plaisance, Côte rocheuse, Village de pêcheurs ; campagne = Champs, Forêt, Village, Vignobles, Plaine, Bois de pins ; circuit = Tribunes, Paddock, Virages rapides, Section boisée. Le nom s'affiche en entrant dans une zone.
- RG-03 : décor propre à chaque carte : palmiers, parasols, océan (plan d'eau `#1e7fa8` jour) ; haies, champs, fermes, éoliennes (rotors animés) ; vibreurs, tribunes, murs de pneus, gravier. Sol et route par carte (`ground`, `road`, textures route `roadCircuit`, etc.).
- RG-04 : ambiance jour/nuit par carte (`envFor`) : brouillard plage jour 950 m, campagne 820 m, circuit 820 m ; nuit plage `#141428` 640 m, campagne `#0d1218`, circuit `#0e1020` 640 m.
- RG-05 : décor généré par la graine de la carte (`mulberry32`), ≤ 300 appels de dessin, instanciation GPU comme WFS-06.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/world/maps.js` (`MAPS`, `envFor`) ; `js/world/decor.js` → `generateDecor`, `ZONES`, `DECOR`, `GROUND`, `GCOL` ; `js/world/track.js` → `buildTrack` ; `js/world/life.js` (végétation `vegBroadleaf`, `vegPalm`, `vegPine`…) ; `js/world/renderer.js` → `buildChunk`.
- Retours connus : décors statiques fusionnés par matériau ; petits objets non dessinés au-delà de 200 m ; arbres en cartes à `alphaTest`, pas de transparence.

## Critères d'acceptation (captures du banc PC)
- [ ] une capture par carte (3 nouvelles) jour et nuit : sol, bordure, décor et ciel reconnaissables
- [ ] test de fidélité du tracé de chaque carte (même graine = mêmes segments que le JS)
- [ ] appels de dessin ≤ 300 relevés au journal

## Hors périmètre
Piétons (WFS-19), pluie (WFS-18), sélecteur de carte (WFS-11/13).
