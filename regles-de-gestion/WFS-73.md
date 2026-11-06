# WFS-73 — Conducteurs dans les voitures
Épic : E0 (point d'extension, avec accroches) · Priorité : P3 Moyenne · Dépend de : WFS-19 (tenues et dessin des personnages) · Taille : 1 à 2 jours

## Besoin
Chaque voiture (joueur, trafic, adversaire du duel) a un conducteur visible face au volant, comme dans le jeu web. Issu du découpage de WFS-19 (ancienne RG-05).

## Règles de gestion
- RG-01 : mesure du siège au chargement du `.wfm` (`src/render/model_loader.cpp`) : même méthode que `measureSeat` (rayons verticaux sur la carrosserie pour trouver le toit) ; profil gardé dans `Model` (quelques flottants, pas la géométrie). Rien de mesuré par image.
- RG-02 : buste figé = pose `Sitting_Idle_Loop` figée, haut du corps seul (0,85 m), comme `bakeBust` ; dessiné par `drawSkinned` avec une palette d'os calculée une fois (pas d'animation).
- RG-03 : posé à la place mesurée, tourné vers le volant (`rotation.y = π ± 0,06`), 3 coiffures par sexe, apparence tirée de `pedLook` (WFS-19) stable par voiture ; réduit jusqu'à 0,86 sur les voitures très basses.
- RG-04 : accroches : joueur (`src/game/game.cpp`), trafic (`traffic_system`) et adversaire du duel ; chaque accroche ≤ 10 lignes, annoncée au Journal. Le code du conducteur vit dans un fichier dédié (`src/world/conducteurs.{h,cpp}` ou `src/game/conducteurs.{h,cpp}`).
- RG-05 : coût : un buste par voiture visible à moins de 50 m (au-delà, rien) ; pas d'allocation par image.

## À réutiliser
- Jeu web (`wlfaleh/waaps`, `waforspeed/`) : `js/world/drivers.js` (`bakeBust`, `measureSeat`, `addDriver`) — mêmes valeurs ; `js/world/people.js` (`pedLook`).
- Natif : `drawSkinned`, `src/render/squelette.h`, `assets/people/*.wfp`, `ApparencePersonnage` (WFS-72) et l'apparence de WFS-19. Ne créer aucun personnage.

## Critères d'acceptation (captures du banc PC)
- [ ] capture poursuite : conducteur visible à travers la vitre du joueur, face au volant (pas de dos), ni dans le toit ni flottant
- [ ] capture trafic : conducteurs dans les voitures du trafic ; deux modèles de hauteurs différentes bien placés
- [ ] autres captures inchangées hors voitures, banc `0 écart(s)`

## Hors périmètre
Mains sur le volant, animation du conducteur, conducteur en vue cockpit (WFS-66 dessine l'habitacle).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P3 Moyenne | WFS-73 | Conducteurs dans les voitures (siège mesuré au chargement, buste figé, joueur / trafic / duel) | E0 | ✅ Fusionné (QA de nuit à faire) |  | WFS-19 | `feature/waforspeed-73` | #42 (fusionnée, 710c9fc) |  |

Critères et notes de suivi :

**WFS-73 — Conducteurs dans les voitures** · E0 · priorité P3 Moyenne · ⏳ En attente de WFS-19 (règles : `regles-de-gestion/WFS-73.md`)
- [ ] conducteur face au volant chez le joueur et dans le trafic, bien placé sur deux modèles de hauteurs différentes ; banc `0 écart(s)`
