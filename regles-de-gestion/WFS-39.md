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

## Zone de code et points d'appui (affinage PO, 2026-11-03)
- Zone E8 : `src/ui/ecrans/**` (nouveaux fichiers `pause.h/.cpp`, bouton Menu dans l'écran de course côté E8).
- Déjà prêt dans le noyau (WFS-36) : `Ecran::Pause` (`src/core/ecran.h`) et `Game::update` qui fige la simulation
  (`src/game/game.cpp` l. 202) : **ne pas recoder la pause, l'utiliser**. Marges de l'encoche : API de WFS-51.
- RG-04 : accroche ≤ 10 lignes annoncée au Journal dans `src/platform/android_main.cpp` (`APP_CMD_PAUSE` / `APP_CMD_LOST_FOCUS`
  → passage à `Ecran::Pause` si l'écran courant est `Course`). Rien d'autre hors zone ; si plus est nécessaire : ticket E0.
- Capture `pause-*` : enregistrement des scènes du banc par la même accroche que WFS-38 (≤ 10 lignes, annoncée).
- Boutons : réutiliser les composants et le style de l'écran de fin de WFS-38 / WFS-57 (paire centrée), ne pas en refaire.

## Critères d'acceptation
- [ ] `pause-bouton` : bouton Menu visible en course, hors encoche
- [ ] `pause-0` : écran de pause, 3 boutons ; `pause-reprise` : même chrono qu'avant la pause (journal)
- [ ] `pause-quitter` : retour au menu

## Hors périmètre
Réglages caméra / graphismes / son de l'écran de pause (ticket ultérieur), HUD (WFS-27).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P2 Haute | WFS-39 | Pause en course et retour au menu | E8 | ✅ Fusionné (ordre de l'utilisateur, QA de nuit à faire) | Claude (session trafic), 2026-11-03 | WFS-38 ✅ | `feature/waforspeed-39` | #34 |  |

Critères et notes de suivi :

**WFS-39 — Pause en course et retour au menu** · E8 · priorité P2 Haute · ✅ Fusionné le 2026-11-03 sur ordre de l'utilisateur (QA de nuit : `qa/Check.md`) — Claude (session trafic), 2026-11-03 (règles : `regles-de-gestion/WFS-39.md`)
- [ ] `pause-bouton`, `pause-0` (3 boutons), `pause-reprise` (chrono conservé), `pause-quitter` (menu)
