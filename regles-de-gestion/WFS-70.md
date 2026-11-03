# WFS-70 — Ressenti de conduite : direction, chocs, vibrations, accessibilité
Épic : E3 · Priorité : P2 Haute · Dépend de : — · Taille : 2 jours

## Besoin
Rendre la conduite agréable sur téléphone : choix de la façon de tourner, chocs qui se sentent (secousse, ralenti, vibration), et option pour réduire les mouvements.

## Règles de gestion
- RG-01 : modes de direction repris du jeu web (`setSteer`, `steerModeBtn`) : au minimum « zone de glissement » et « flèches » ; le choix se fait dans les réglages (WFS-41) et est mémorisé (WFS-24).
- RG-02 : au choc fort : secousse de caméra 0,9, flash, **ralenti à 35 % du temps pendant 0,5 s**, vibration du téléphone `[90, 40, 120] ms` ; au choc léger : secousse `0,25 + impact × 1,5`, flash proportionnel.
- RG-03 : frottement (glissière) : secousse 0,3, flash 0,25, vibration 40 ms ; voiture accidentée au ralenti : secousse 0,12.
- RG-04 : option « réduire les mouvements » : coupe secousses et ralenti (valeurs à 0), garde les vibrations ; la pluie (WFS-18) et les lignes de vitesse (WFS-65) sont réduites.
- RG-05 : le ralenti n'allonge pas le chrono de course ; la physique et les temps restent corrects.
- RG-06 : le code a son propre fichier ; accroche ≤ 10 lignes annoncée au Journal dans `src/game/game.cpp` ou la caméra.

## À réutiliser du jeu web
`js/game/update.js` (≈ lignes 36-45, 68, 112-113, 214 : `shake`, `flash`, `timeScale`, `slowmoT`, `navigator.vibrate`) ; `js/ui/screens.js` (`setSteer`, `setSteerRow`) ; `js/core/config.js` (`reduceMotion`).

## Critères d'acceptation
- [ ] En course : un choc donne une secousse visible et un ralenti (journal du jeu) ; deux modes de direction utilisables ; « réduire les mouvements » supprime la secousse ; captures du banc sans régression.

## Hors périmètre
Manettes de jeu (WFS-22), effets visuels (WFS-65).
