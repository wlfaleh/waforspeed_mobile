# WFS-71 — Ville vivante : commerces, portique, enseignes
Épic : E2 · Priorité : P3 Moyenne · Dépend de : WFS-06 ✅ · Taille : 2 à 3 jours

## Besoin
Donner de la vie au décor : cafés, restaurants et bars avec terrasses, portique 3D des checkpoints / kilomètres / arrivée, panneaux et enseignes de chaque terrain.

## Règles de gestion
- RG-01 : commerces (`SHOPS`) posés devant les bâtiments en zone ville, avec terrasse (tables, chaises, parasols) ; placement par la graine de la carte, identique à chaque partie.
- RG-02 : clients attablés = personnages de WFS-19 (si disponibles, sinon tables vides) ; pas de silhouettes géométriques.
- RG-03 : portique au-dessus de la route : panneau lumineux « CHECKPOINT », « n km », « ARRIVÉE » avec les couleurs du jeu web, visible de loin.
- RG-04 : panneaux publicitaires et enseignes néon (`BOARD_TEXTS`, `AD_TEXTS` : WAFORSPEED, GARAGE 88, TURBO CAFÉ, NUIT BLANCHE, PNEUS 24/7, GRAND PRIX…) sur la ville et le circuit.
- RG-06 : plage : serviettes (`decor.js` l. 51) avec personnes allongées, et baigneurs ; personnages et apparence de WFS-19 (`populatePeople` : plage 50 % côté mer, 10 % rochers), poses `Sitting_*` et allongée du jeu web.
- RG-05 : coût maîtrisé : objets fusionnés par tronçon (comme le décor existant) ; ≤ 300 appels de dessin.

## À réutiliser du jeu web
`js/world/life.js` (`SHOPS`, `getShopMats`, `placeShop`, terrasses) ; `js/world/scene.js` (`setGantry`) ; `js/world/decor.js` (`BOARD_TEXTS`, `AD_TEXTS`, panneaux).

## Critères d'acceptation
- [ ] captures : terrasse de café avec clients assis, plage avec serviettes et allongés, portique de checkpoint, enseigne néon la nuit ; banc `0 écart(s)` hors zones modifiées

## Hors périmètre
Piétons qui marchent (WFS-19), conducteurs (WFS-73), nouvelles cartes (WFS-53).
