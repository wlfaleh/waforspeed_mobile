# WFS-14 — Cartes : socle multi-cartes et Corniche (plage)
Épic : E2 · Priorité : 12 · Dépend de : WFS-06 (✅) · Redécoupé le 2026-11-03 (PO) : Campagne → WFS-53, Grand Prix → WFS-54.

## Besoin (qui, quoi, pourquoi)
Le joueur pourra choisir entre quatre cartes. La ville existe (WFS-06). Ce ticket pose le **socle commun** (table des
cartes, zones, sol et ambiance par carte) et livre la **première nouvelle carte : Corniche (plage)**. Campagne (WFS-53) et
Grand Prix (WFS-54) ne feront ensuite qu'ajouter leur ligne de table et leur décor.

## Règles de gestion
- RG-01 : table des cartes portée de `MAPS` (mêmes valeurs) : ville `seed 88`, trafic 42 ; plage `seed 301, curves [1,2,3], hills 0, sections 32, edge 1.6, trafic n 34 (0,3-0,55)` ; campagne `seed 512, curves [2,3,4], hills 120, sections 36, edge 1.38, trafic 26 (0,28-0,5)` ; circuit `seed 777, curves [3,5,6], hills 12, sections 34, edge 1.66, trafic 16 (0,5-0,78)`. Les lignes campagne et circuit sont saisies dès ce ticket (tracé seulement), leur décor vient en WFS-53 / WFS-54.
- RG-02 : zones tous les `ZONE_LEN = 448` tronçons (≈ 540 m) ; plage = Corniche, Port de plaisance, Côte rocheuse, Village de pêcheurs. Le nom de zone est **exposé** (lecture par l'interface) ; son affichage à l'écran relève de E5 (hors périmètre).
- RG-03 : décor de la plage : palmiers (`vegPalm`), parasols, océan (plan d'eau `#1e7fa8` de jour) côté mer ; sol et route de la carte (`GROUND`, `GCOL`).
- RG-04 : ambiance par carte (`envFor`) : plage jour brouillard 950 m ; plage nuit `#141428`, 640 m. Ville inchangée.
- RG-05 : décor généré par la graine de la carte (`mulberry32`), ≤ 300 appels de dessin, instanciation GPU comme WFS-06 ; petits objets non dessinés au-delà de 200 m.
- RG-06 : carte par défaut = ville : toutes les captures existantes restent identiques. Choix de la carte au banc par variable d'environnement (ex. `WFS_CARTE=plage`) ; le sélecteur à l'écran est WFS-41.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/) — obligatoire, ne rien réinventer
- Code : `js/world/maps.js` (`MAPS`, `envFor`) ; `js/world/decor.js` → `generateDecor`, `ZONES`, `DECOR`, `GROUND`, `GCOL` ; `js/world/track.js` → `buildTrack` ; `js/world/life.js` → `vegPalm` (déjà en partie porté dans `src/world/vegetation.cpp` par WFS-34 : réutiliser ce générateur) ; `js/world/renderer.js` → `buildChunk`.
- Natif existant : `src/world/ville.cpp` (instanciation, graine), `src/world/vegetation.*` (arbres WFS-34).
- Retours connus : décors statiques fusionnés par matériau ; arbres en cartes à `alphaTest`, pas de transparence.

## Critères d'acceptation (captures du banc PC)
- [ ] `carte-plage-jour` et `carte-plage-nuit` : océan, palmiers, parasols, sol sableux, brouillard / ciel de la plage reconnaissables
- [ ] test de fidélité du tracé des 3 nouvelles cartes (même graine = mêmes segments que le JS), 0 écart
- [ ] appels de dessin ≤ 300 relevés au journal ; toutes les autres captures identiques à `main`

## Hors périmètre
Décor campagne (WFS-53) et circuit (WFS-54), affichage du nom de zone (E5), sélecteur de carte (WFS-41), piétons (WFS-19), pluie (WFS-18).
