# WFS-94 — Corniche : quartiers Port de plaisance, Côte rocheuse et Village de pêcheurs
Épic : E2 · Priorité : P3 Moyenne · Dépend de : WFS-93 (même épic, à prendre après) · Taille : 2 jours

## Besoin
La Corniche (`src/world/plage.cpp`) porte les bas-côtés de chaque zone mais pas leurs objets : pas de villas côté terre,
pas de bateaux ni de bornes d'amarrage dans la marina, pas de rochers, de pins ni de glissières sur la côte rocheuse, pas
de maisons ni de barques au village, pas de tour de sauveteur. La carte paraît vide comparée au jeu web.

## Règles de gestion
- RG-01 : `beach.corniche` : villas côté terre (`villa`, RW + 12-18, une fois sur 0,75), palmiers côté terre, tours de
  sauveteur (`tower`, tous les 160-240 tronçons, RW + 22) ; palmiers et parasols côté mer déjà faits.
- RG-02 : `beach.marina` : bornes (`bollard`) tous les 3 tronçons à RW + 6,5, bateaux (`boat`, 7-14 m, voile 6 fois sur 10,
  4 couleurs) à RW + 13-31, villas colorées côté terre à RW + 7 avec commerces (`placeShop`, déjà porté par WFS-71).
- RG-03 : `beach.rochers` : rochers (`rock`, taille 1-4,5) côté mer, pins et arbres ronds côté terre (`vegetation.*`),
  glissière (`guard`) tous les 3 tronçons à RW + 1,2.
- RG-04 : `beach.village` : maisons (`villa`) des deux côtés, barques (`boat` sans voile) à RW + 30-40.
- RG-05 : formes et couleurs = `DECOR.villa`, `tower`, `bollard`, `boat`, `rock`, `guard` de `decor.js` (primitives de
  `decor_carte.*`) ; ≤ 300 appels de dessin.

## À réutiliser du jeu web
`js/world/decor.js` : `beach.corniche`, `marina`, `rochers`, `village` (l. 99-148), `DECOR.villa`, `tower`, `bollard`,
`boat`, `rock`, `guard`. Natif : `decor_carte.*` (villa déjà portée pour la campagne), `vegetation.*`, `commerces.cpp`.

## Critères d'acceptation
- [ ] `carte-plage-marina`, `carte-plage-rochers`, `carte-plage-village` : objets de la zone visibles ; `carte-plage-jour`
  avec villas côté terre ; ≤ 300 appels de dessin ; tests CPU verts

## Hors périmètre
Bateaux animés, mer animée.

## Zone de code
`src/world/plage.cpp`, `src/world/cartes_banc.cpp`, nouveaux fichiers `src/world/plage_*.cpp`.
