# WFS-93 — Ville : les 6 quartiers du jeu web + finitions des cartes (panneaux du circuit, vignes)
Épic : E2 · Priorité : P2 Haute · Dépend de : — (WFS-06, WFS-53, WFS-71 ✅) · Taille : 2 à 3 jours

## Besoin (qui, quoi, pourquoi)
La Ville est la carte par défaut, mais le natif ne porte que le quartier « centre » (`src/world/ville.cpp`, en-tête :
« Port simplifié du quartier centre ») : toute la course traverse les mêmes tours. Dans le jeu web, la ville traverse
6 quartiers (un par checkpoint environ, `ZONES.city` de `js/world/decor.js`) : Centre-ville, Quartier résidentiel, Parc
urbain, Zone portuaire, Quartier d'affaires, Vieux quartier. C'est l'écart le plus visible restant avec le jeu web.
Dans la même zone de code, deux finitions vues sur les captures de `main` (Release `apercu`) : les panneaux
publicitaires du circuit sont des bandes vides (`carte-circuit-jour`, `-nuit`) et les rangs de vignes de la campagne
forment des marches (`carte-campagne-vignes`).

## Règles de gestion (du jeu qui marche, pas la perfection)
- RG-01 : la ville suit `ZONES.city` et `zoneId()` (`src/world/zones.*`, déjà porté) : chaque zone de 448 tronçons
  appelle le générateur de son thème (`city.centre`, `city.residentiel`, `city.parc`, `city.port` de `decor.js`, mêmes pas,
  mêmes tirages `D()`, mêmes dimensions). « Quartier d'affaires » = `centre`, « Vieux quartier » = `residentiel`, comme le web.
- RG-02 : **résidentiel** : immeubles bas (11-23 m, teintes 2-3) tous les 9 tronçons, commerces (`placeShop`, déjà porté
  par WFS-71 dans `commerces.cpp`) une fois sur 0,4, lampadaires tous les 26, arbres d'alignement tous les 10, abribus
  (`busstop`) tous les 160, passages piétons tous les 170.
- RG-03 : **parc** : un côté tiré (`parkSide`) : grille (`railing`) tous les 4 tronçons à RW + 6, arbres ronds / pins
  (`tree`, essences de `vegetation.*`) à RW + 9 … + 54, bancs (`bench`) tous les 22, fontaine (`fountain`) tous les 200 ;
  l'autre côté : immeubles moyens (14-36 m). Les piétons du parc (WFS-19) gardent leur placement actuel.
- RG-04 : **port** : entrepôts (`warehouse`), conteneurs empilés (`containers`, 2-4 de haut, couleurs du web), grillage
  (`mesh`) à RW + 5, grues (`crane`, 34-46 m) tous les 110-150 tronçons, lampadaires tous les 30.
- RG-05 : formes et couleurs des objets = `DECOR.busstop`, `railing`, `bench`, `fountain`, `warehouse`, `containers`,
  `mesh`, `crane` de `decor.js` (boîtes et cylindres, palette `C`), construits avec les primitives existantes
  (`decor_carte.*` : `box`, `cyl`, palette) ; un maillage par matériau et par morceau ; ≤ 300 appels de dessin (journal).
- RG-06 : les lampadaires des nouveaux quartiers s'inscrivent dans `lampadaires()` (comme WFS-81) pour que la nuit
  (WFS-09) les allume ; collisions (`collide`) aux mêmes endroits que le web.
- RG-07 : **panneaux du circuit** : le texte d'`AD_TEXTS` (« WAFORSPEED », « GRAND PRIX », « PNEUS 24/7 », « TURBO »,
  couleurs fond / texte du web) est écrit sur chaque panneau `T_ADBOARD` de `circuit.cpp`, lisible des deux côtés, avec
  l'atlas de textes déjà fait par WFS-71 (`AtlasTextes` de `src/world/enseignes.h`). Police italique 700 comme le web.
- RG-08 : **vignes** : les rangs (`T_VINES`, `campagne.cpp`) suivent la pente du coteau sans former de marches ni de prismes
  visibles : base posée sur `groundY` à chaque extrémité (ou rangs plus courts), effilement 0,6 comme `DECOR.vines`.
  Comparer au rendu web (`vines(ctx…)` l. 629) : même allure de rangs fins et réguliers.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- `js/world/decor.js` : `ZONES`, `generateDecor` → `city.residentiel`, `city.parc`, `city.port` (l. 75-125) ; `DECOR.busstop`,
  `railing`, `bench`, `fountain`, `warehouse`, `containers`, `mesh`, `crane` ; `AD_TEXTS` (l. 7) et `adboard` (l. 502) ;
  `vines` (l. 629). `js/world/renderer.js` l. 150 (`TEXS.ads` : canvas du texte des panneaux).
- Natif déjà fait, à réutiliser, rien à recréer : `src/world/zones.*`, `decor_carte.*` (primitives, palette, morceaux),
  `vegetation.*` (arbres), `commerces.cpp` (`placeShop`), `enseignes.*` (`AtlasTextes`), `lampadaires()` (WFS-81).
- Aucun modèle 3D : tout est procédural dans le web aussi.

## Critères d'acceptation (captures du banc PC, groupe `cartes` existant)
- [ ] nouvelles captures `carte-ville-residentiel`, `carte-ville-parc`, `carte-ville-port` (départ dans la zone, comme
  `cartes_banc.cpp` le fait pour la campagne) : immeubles bas + abribus ; grille, pelouse, arbres, bancs, fontaine ;
  entrepôts, conteneurs colorés, grues ; `course-*` de la ville (départ au centre) inchangées
- [ ] `carte-circuit-jour` / `-nuit` : texte lisible sur les panneaux publicitaires
- [ ] `carte-campagne-vignes` : rangs de vignes réguliers, plus de marches
- [ ] journal : appels de dessin ≤ 300 sur la ville, ≥ 30 im/s inchangé en qualité Basse ; tests CPU verts

## Hors périmètre
Quartiers de la Corniche (WFS-94), voitures garées (no-op dans le web), nouveaux piétons, lumière des projecteurs (WFS-97).
Pluie RG-05 de WFS-18 : abandonnée par le PO (coût mesuré 0,008 ms, le web n'a pas cette règle).

## Zone de code
`src/world/**` (nouveau fichier conseillé `src/world/ville_quartiers.cpp`, plus `ville.cpp`, `circuit.cpp`,
`campagne.cpp`, `cartes_banc.cpp`). Interdit : `src/render/**` (WFS-97), `src/game/duel*`, `src/ui/ecrans/salon.*`
(WFS-89), `src/platform/**` et `.github/**` (WFS-91 : ne pas toucher `desktop_main.cpp`, utiliser le groupe `cartes`).
