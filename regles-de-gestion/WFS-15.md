# WFS-15 — Réglages de qualité selon le téléphone
Épic : E4 · Priorité : 14 · Dépend de : WFS-07

## Besoin (qui, quoi, pourquoi)
Le jeu doit rester fluide du téléphone d'entrée de gamme au haut de gamme : trois niveaux (bas, moyen, haut), choisis
automatiquement au premier lancement, modifiables dans la pause.

## Règles de gestion
- RG-01 : le web a 2 niveaux (`high` / `low`) ; le natif en ajoute un « moyen » : haut = tout activé, bas = comportement du web `low`.
- RG-02 : effets du niveau bas (web) : portée de vue `VIEW` 330 au lieu de 470 tronçons, brouillard lointain ≤ 420 m, pas de bloom ni d'étalonnage, pas d'ombres temps réel, pas de reflets temps réel de la route, façades sans PBR, `DPR` ≤ 1,25 (haut : 1,75 ordinateur, 1,5 mobile).
- RG-03 : résolution dynamique : toutes les 2 s de course, `fps < 40` → échelle −0,1 (plancher 0,6) ; `fps > 57` → +0,1 (plafond 1).
- RG-04 : si l'échelle est à 0,6 et `fps < 30` : passage automatique au niveau inférieur une seule fois, message « Graphismes rapides pour plus de fluidité ».
- RG-05 : choix de l'utilisateur mémorisé et prioritaire ; tout nouvel effet (ombres, bloom, route mouillée) dépend du niveau.
- RG-06 : mesures au journal pour chaque niveau : ms CPU/GPU, appels de dessin, triangles.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/ui/screens.js` → `setQuality`, `watchPerf`, `qualityChosen`, `setQualityLabel` ; `js/world/renderer.js` → `onResize` (DPR, `resScale`), `VIEW`, `setupComposer` ; `js/world/maps.js` (`fogFar`).
- Retours connus (skill `waforspeed-3d`, §8) : `pixelRatio ≤ 1.5` sur mobile ; mesurer les premières secondes de course ; ne jamais changer ombres/résolution à chaque image, seulement au changement de niveau.

## Critères d'acceptation (captures du banc PC)
- [ ] trois captures de la même scène (bas / moyen / haut) avec des différences visibles et documentées
- [ ] journal : le niveau bas coûte nettement moins (ms) que le haut
- [ ] test CPU de la logique d'ajustement (fps simulés → échelle et niveau attendus)

## Hors périmètre
Contenu des effets eux-mêmes (WFS-07, 16, 17), détection fine du modèle de téléphone.
