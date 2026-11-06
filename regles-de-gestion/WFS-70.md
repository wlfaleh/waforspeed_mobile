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

## Précisions du PO (2026-11-04 00:17:41, à l'attribution)
- RG-07 (zone) : code dans de nouveaux fichiers `src/game/ressenti.{h,cpp}` (secousse, flash, ralenti, vibration, réduire les mouvements) et `src/game/direction.{h,cpp}` (modes de direction). **Exception accordée par le PO** : dans `src/game/game.cpp`, la partie « volant » de `Game::onTouch` et de `Game::drawControls` peut être **déplacée** vers `direction.cpp` (lignes retirées + appels ≤ 10 lignes ajoutées) ; pédales, nitro et marges (WFS-51) inchangées. À annoncer au Journal.
- RG-08 (mode « flèches ») : deux zones à gauche (flèche gauche / flèche droite), braquage progressif vers ±1 tant qu'on appuie, retour à 0 au relâchement (comme `keys` du jeu web). Mode par défaut : « zone de glissement » (comportement actuel, inchangé).
- RG-09 (réglages) : le mode de direction et « réduire les mouvements » sont deux champs publics lisibles et modifiables par les écrans (WFS-41, E8) ; si WFS-41 n'est pas fusionné, ne pas toucher `src/ui/ecrans/` : le banc les règle (`WFS_BANC=ressenti`). Mémorisation : WFS-24, hors périmètre.
- RG-10 (vibration Android) : nouveau fichier `src/platform/vibration_android.cpp` (JNI `Vibrator`, motif `[90, 40, 120]` / 40 ms) + permission `VIBRATE` dans le manifeste (1 ligne) + accroche dans `android_main.cpp` ≤ 10 lignes ; sur PC, la vibration est seulement écrite au journal (`vibration 90-40-120`). Pas de conflit : WFS-50 (E7) ne touche que `tools/qa/**` et `simulateur.yml`.
- RG-11 (existant à reprendre, ne pas refaire) : `traffic.cpp` calcule déjà `ev.shake` (0,9 / `0,25 + impact × 1,5` / 0,15 au frottement → porter à 0,3 selon RG-03) et `course.cpp` a déjà `timeScale = 0,35`, `slowmoT = 0,5` : brancher dessus (secousse appliquée à la caméra de `camera_vue.*`, flash par le HUD existant si disponible, sinon un rectangle blanc translucide `uiRect`).
- Banc : scène `ressenti` avec captures `ressenti-choc` (secousse + flash visibles), `ressenti-fleches` (deux flèches à l'écran), `ressenti-reduit` (même choc, pas de secousse) ; tests dans `tests/` : chrono non allongé par le ralenti (RG-05), secousse nulle avec « réduire les mouvements ».

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P2 Haute | WFS-70 | Ressenti de conduite : modes de direction, chocs (secousse, ralenti, vibration), réduire les mouvements | E3 | ✅ Fusionné (par confiance, QA de nuit à faire) | Claude (session trafic), 2026-11-04 | — | `feature/waforspeed-70` | [#51](https://github.com/wlfaleh/waforspeed_mobile/pull/51) |  |

Critères et notes de suivi :

**WFS-70 — Ressenti de conduite** · E3 · priorité P2 Haute · ✅ Fusionné (par confiance, QA de nuit à faire), PR [#51](https://github.com/wlfaleh/waforspeed_mobile/pull/51) (règles : `regles-de-gestion/WFS-70.md`)
- [x] choc = secousse + ralenti ; 2 modes de direction ; réduire les mouvements
