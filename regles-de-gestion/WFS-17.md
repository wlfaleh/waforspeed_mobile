# WFS-17 — Post-traitement : bloom, TAA, flou de mouvement
Épic : E4 · Priorité : 20 · Dépend de : WFS-15

## Besoin (qui, quoi, pourquoi)
Les néons, phares et feux rayonnent (bloom), l'image est stable (anti-crénelage temporel) et la vitesse se ressent
(flou de mouvement), activés selon la qualité.

## Règles de gestion
- RG-01 : bloom (qualité haute) : rayon 0,45 ; nuit force 0,8, seuil 0,46 ; jour force 0,3, seuil 0,9 ; ce qui doit briller (feux, néons, phares) dépasse le seuil, le reste reste dessous.
- RG-02 : étalonnage final : nuit `exposure 0.9, white 1.4, sat 1.1`, jour `exposure 0.72, white 1.65, sat 1.06`, lift/gain par ambiance ; vignette 0,3 (jour) à 0,55 (nuit), plus sombre et cyan avec la nitro.
- RG-03 : HDR en virgule flottante demi-précision ; MSAA 4 échantillons restent la base (déjà WFS-03) ; TAA et flou de mouvement sont des ajouts natifs absents du web.
- RG-04 : TAA : jitter sous-pixel par image, historique réutilisé, rejet sur mouvement rapide, pas de traînées sur le trafic ; nette sur les bords fins (câbles, lampadaires).
- RG-05 : flou de mouvement : proportionnel à la vitesse (nul < 100 km/h), désactivé en « réduire les animations » ; lignes de vitesse en cercle quand la nitro est active (28 traits, `rgba(230,240,255,.14)`).
- RG-06 : niveaux : bas = rien ; moyen = bloom ; haut = bloom + TAA + flou ; coût mesuré au journal ; aucun point blanc en trame (WFS-32).

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/world/renderer.js` → `setupComposer`, `makeGradeShader`, `updateGrade` ; `js/world/maps.js` (`bloom`, `grade`, `vignette`) ; `js/world/fx.js` → `drawFx` (vignette, lignes de nitro) ; `js/world/scene.js` (réglage du bloom par image).
- Retour connu : bloom faible de jour (seuil haut), glow néon la nuit ; tout nouvel effet dépend de la qualité.

## Critères d'acceptation (captures du banc PC)
- [ ] nuit : halo des feux arrière et des néons visible, pas de voile sur la route ; jour : image nette, peu de halo
- [ ] captures TAA : bords sans escalier, pas de fantôme derrière la voiture
- [ ] lignes pointillées sur les jointures de carrosserie (reste du rapport WFS-32) lissées par le TAA
- [ ] haute vitesse : flou radial léger visible ; qualité basse : aucun effet ; coûts au journal

## Hors périmètre
Profondeur de champ, lens flare (le web a un `flare` du soleil, hors ticket sauf demande).
