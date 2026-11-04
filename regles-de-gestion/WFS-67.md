# WFS-67 — Trafic vivant
Épic : E3 · Priorité : P3 Moyenne · Dépend de : WFS-60 · Taille : 2 à 3 jours

## Besoin
Le trafic doit avoir l'air vrai : clignotants avant un changement de file, feux de freinage, voitures accidentées contournées, et la variété des modèles du jeu web.

## Règles de gestion
- RG-01 : clignotant ≈ 1 s avant un changement de file, puis le changement.
- RG-02 : feux stop allumés quand la voiture freine ; feux de recul en marche arrière.
- RG-03 : les voitures accidentées restent sur place avec la fumée (WFS-65) ; les autres les contournent ou freinent.
- RG-04 : au départ, tous les trafics sont identiques pour une même graine (pour le duel, WFS-24).
- RG-05 : variété des modèles et des couleurs de trafic (`TRAFFIC_PAINTS`) ; version complète à moins de 50 m, silhouette légère au-delà.
- RG-06 : pas de perte de performance : coût de dessin du trafic mesuré avant / après.

### Précisions du PO (2026-11-04, à l'attribution)
- RG-07 : la simulation existe déjà en natif (`src/game/traffic.*` : `signal`, `pending`, `crashed`, contournement) ; ce ticket la **rend visible**. Ne pas réécrire `traffic.cpp` : ajouter seulement l'état manquant (ex. `freine`, `recul`) calculé dans la mise à jour.
- RG-08 : clignotant = feu orange émissif (`emissive` du `DrawItem` / push constant, API existante, aucun changement du renderer) au coin avant et arrière du côté `signal`, clignotement 2 Hz (0,25 s allumé / 0,25 s éteint), visible de jour comme de nuit.
- RG-09 : feux stop = rouge émissif à l'arrière, intensité ×3 pendant le freinage ; WFS-09 (E4) s'occupe des halos et de l'éclairage de nuit : ici seulement l'émissif posé par `traffic_system.cpp`.
- RG-10 : voiture accidentée : feux de détresse (deux clignotants ensemble) tant qu'elle est arrêtée ; fumée = appel à l'API d'effets de WFS-65 déjà fusionnée.
- RG-11 : variété : tirage du modèle et de `TRAFFIC_PAINTS` par la graine ; utiliser les modèles disponibles dans `CARS` (4 aujourd'hui, 20 si WFS-13 est fusionné au moment de la synchronisation : lever alors `TRAFIC_MODELES`). Au-delà de `LOD_CAR_M` = 50 m : version `-lod` si présente, sinon silhouette (boîte peinte + ombre) comme `makeCarFar`.
- RG-12 : roues du trafic : déjà animées par WFS-60 (`roues::poseTrafic`) ; ne pas y toucher.

## À réutiliser du jeu web
`js/game/traffic.js` (distances de sécurité, changements de file, graine) ; `js/world/scene.js` → `trafficMesh`, `makeCarFar`, `LOD_CAR_M` ; modèles `-lod` de `models/`.

## Critères d'acceptation
- [ ] capture `trafic-clignotant` : un feu orange allumé sur le côté d'une voiture qui change de file
- [ ] capture `trafic-freinage` : feux stop rouges vifs derrière une voiture qui freine
- [ ] capture `trafic-accident` : voiture accidentée arrêtée avec feux de détresse et fumée, une autre la contourne
- [ ] capture de course : au moins 3 modèles ou couleurs différents visibles ; voitures lointaines dessinées en version légère
- [ ] banc `0 écart(s)` ; ≤ 300 appels de dessin ; coût de dessin du trafic avant / après noté au Journal ; test CPU : même graine → même trafic

## Hors périmètre
Roues animées du trafic (faites par WFS-60), duel / sauvegarde (WFS-21, WFS-24), éclairage de nuit et halos (WFS-09), ressenti des chocs (WFS-70).
