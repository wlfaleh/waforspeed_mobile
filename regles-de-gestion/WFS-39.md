# WFS-39 — Pause en course et retour au menu
Épic : E8 (Écrans et menus) · Priorité : 0.6 · Dépend de : WFS-38

## Besoin
En course, un bouton « Menu » met en pause et propose Reprendre / Recommencer / Quitter, comme le web.

## Règles de gestion
- RG-01 : bouton « Menu » (icône pause ‖ + « Menu », orange `--accent`) en haut à gauche, dans la zone sûre (web `#menuBtnHud`).
- RG-02 : appui → écran `Pause` : simulation figée, chrono arrêté ; sur-titre « Course en pause », titre « Réglages » (web `#pause`).
- RG-03 : « Reprendre » → `Course` sans perte (les commandes repartent à zéro, web `resumeGame`) ; « Recommencer » → nouvelle course
  (`startRace`) ; « Quitter la course » → `Menu` (`toMenu`).
- RG-04 : Android : passage en arrière-plan ou perte du focus pendant la course → `Pause` automatique (web `blur` / `visibilitychange`).

## À réutiliser
`index.html` `#pause`, `#menuBtnHud` ; `js/ui/screens.js` → `pauseGame`, `resumeGame`, `toMenu` ; `css/style.css` `.cam-btn`, `.btn`.

## Critères d'acceptation
- [ ] `pause-bouton` : bouton Menu visible en course, hors encoche
- [ ] `pause-0` : écran de pause, 3 boutons ; `pause-reprise` : même chrono qu'avant la pause (journal)
- [ ] `pause-quitter` : retour au menu

## Hors périmètre
Réglages caméra / graphismes / son de l'écran de pause (ticket ultérieur), HUD (WFS-27).
