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

## À réutiliser du jeu web
`js/game/traffic.js` (distances de sécurité, changements de file, graine) ; `js/world/scene.js` → `trafficMesh`, `makeCarFar`, `LOD_CAR_M` ; modèles `-lod` de `models/`.

## Critères d'acceptation
- [ ] `trafic-clignotant`, `trafic-freinage`, `trafic-accident` ; banc `0 écart(s)` ; ≤ 300 appels de dessin

## Hors périmètre
Roues animées du trafic (WFS-61), duel (WFS-24).
