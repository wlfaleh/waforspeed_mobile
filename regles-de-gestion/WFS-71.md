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

## Zone et partage avec WFS-53 (PO, 2026-11-04, attribution à agent 3)
WFS-53 (cartes Campagne et Grand Prix, même épic E2) est en cours en parallèle. Pour ne jamais modifier les mêmes fichiers :
- RG-07 : code nouveau **uniquement dans de nouveaux fichiers** : `src/world/commerces.*` (RG-01, RG-02), `src/world/portique.*` (RG-03), `src/world/enseignes.*` (RG-04), assets sous `assets/world/vie/**`. `plage.cpp` (RG-06) est modifiable : WFS-53 n'y touche pas.
- RG-08 : `ville.cpp`, `zones.*`, `vegetation.*` et tout fichier de carte de WFS-53 : **accroches de ≤ 10 lignes au total**, annoncées au Journal ; aucune refonte.
- RG-09 : enseignes et portique sur **Ville et Corniche** ; le circuit Grand Prix n'existe pas encore sur `main` : l'API de `portique.*` / `enseignes.*` doit pouvoir être appelée par WFS-53 plus tard (une fonction par carte), sans l'appeler toi-même sur des fichiers de WFS-53.
- RG-10 : la position des checkpoints vient de la course (E3, `src/game/course.h`) en **lecture seule** ; si un accès manque, l'ajouter par une accroche ≤ 10 lignes annoncée au Journal.
- RG-11 : « du jeu qui fonctionne » : terrasses, portique et enseignes visibles sur les captures ; pas de fidélité au pixel.
