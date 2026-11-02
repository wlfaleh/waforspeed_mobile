# WFS-28 — Bruitages : chocs, nitro, crissements, ambiance
Épic : E6 · Priorité : 15 · Dépend de : WFS-10 (✅), WFS-08 (✅)

## Besoin (qui, quoi, pourquoi)
Les événements du jeu (chocs, frottements, nitro, quasi-contacts, bips du décompte et des checkpoints) doivent s'entendre,
en plus du moteur déjà porté.

## Règles de gestion
Le web synthétise ces sons par bruit filtré, `noiseHit(durée s, fréquence Hz, Q, volume, fréquence finale)` :
- RG-01 : accident joueur : `(0.6, 140, 0.7, 1, 45)` + `(0.4, 2600, 1.4, 0.55, 7000)` + `(0.25, 900, 2, 0.4, 300)`.
- RG-02 : petit choc (`bump`) : `(0.3, 200, 0.8, 0.4 + impact, 70)` ; frottement latéral : `(0.22, 3200, 3, 0.35, 1600)`, au plus un toutes les 0,18 s.
- RG-03 : contact avec un adversaire de duel : `(0.5, 160, 0.8, 0.7 + impact, 50)` + `(0.3, 2400, 1.5, 0.5, 6000)`, rythme 0,35 s.
- RG-04 : quasi-contact : `(0.28, 900, 1.2, 0.5, 2600)`.
- RG-05 : bips (`beep`, triangle, volume 0,12) : décompte 440 Hz 0,15 s ; GO 880 Hz 0,35 s ; checkpoint 880 puis 1320 Hz ; fin de course 660, 440, 330 ; arrivée duel 880, 1320, 1760.
- RG-06 : nitro : flammes visibles, régime moteur +6 % (`rate × 1,06`), ajouter un souffle de nitro (nouveau, absent du web) ; crissements de pneus au freinage fort / dérapage (mêmes conditions que la fumée : `braking` et > 18 m/s, ou `|latAcc| > 0.92×PHYS.grip`).
- RG-07 : coupure en pause, volume maître 0,5, coupure par l'option « Son » ; pas de craquement ; un son joué au plus une fois par événement.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/audio/sfx.js` → `noiseHit`, `beep`, `setMuted` ; `js/game/update.js` (appels `noiseHit`/`beep` dans `playerAccident`, `bump`, `hitCar`, `hitGhosts`) ; `js/world/scene.js` (condition `skid`).
- Sons enregistrés : `audio/engines/*.wav` (moteur seulement, déjà dans `assets/audio/`). Le natif reproduit les bruits synthétisés par du bruit filtré (ou de courts échantillons générés au démarrage).
- Retour connu : la vibration du téléphone accompagne les gros chocs (`vibrate([90,40,120])`).

## Critères d'acceptation
- [ ] journal du banc : chaque événement du scénario (choc, frottement, quasi-contact, nitro, bips) déclenche son son une fois
- [ ] enregistrement du banc : pas de craquement au début/fin des sons
- [ ] capture : aucun changement visible

## Hors périmètre
Sons de moteur (WFS-10), musique, ambiance de ville/mer (si demandée, nouveau ticket).
