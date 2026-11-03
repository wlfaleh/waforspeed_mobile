# WFS-51 — Point d'extension : marges de sécurité de l'écran (encoche)
Épic : E0 · Priorité : 0.43 · Dépend de : WFS-29 · Origine : RG-02 de WFS-29 (remarque du QA, `qa/rapports/WFS-29.md`)

## Besoin (qui, quoi, pourquoi)
Depuis WFS-29, l'image passe sous l'encoche (`shortEdges`). Le joueur ne doit jamais avoir une commande tactile ou une
information cachée par l'encoche ou les coins arrondis. Il faut lire les marges de sécurité sur Android et les donner
au jeu ; ce ticket décale aussi les commandes tactiles (dessinées par `Game`).

## Règles de gestion
- RG-01 : `GameView` expose `marges` (gauche, haut, droite, bas, en pixels de l'image) ; 0 partout par défaut.
- RG-02 : Android : marges lues dans `JeuActivity` (`WindowInsets.getDisplayCutout()` / `getSafeInsetLeft…`, Android 9+ ; 0 avant), transmises au natif (JNI, une fonction) à chaque changement d'insets et de rotation.
- RG-03 : commandes tactiles (volant, gaz, frein, nitro) décalées de la marge du côté concerné ; zones de toucher décalées de la même valeur.
- RG-04 : banc PC : variable `WFS_ENCOCHE=<px>` simule une encoche à gauche (ex. 90 px en 2400x1080) ; sans variable, captures identiques.
- RG-05 : les systèmes (HUD E5, écrans E8) lisent `view.marges` ; aucun autre changement d'API.

## Zone de code autorisée
E0 (`src/core/**`, `src/game/game.*`) **plus, exceptionnellement**, `src/platform/android_main.cpp` et `android/**/JeuActivity.java` (E7 libre après WFS-29), annoncé au Journal.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Rien à porter : le jeu web utilise `env(safe-area-inset-*)` en CSS (`css/style.css`), sans équivalent natif. Reprendre seulement le principe : tout élément d'interface reste dans la zone sûre.

## Critères d'acceptation (captures du banc PC)
- [ ] `encoche-course` (2400x1080, `WFS_ENCOCHE=90`) : bande de 90 px à gauche marquée, volant et gaz entièrement à droite de la bande
- [ ] toutes les autres captures identiques à `main`, banc `0 écart(s)`
- [ ] revue du code : lecture des insets Android et appel JNI (vérification à l'écran au simulateur, WFS-50)

## Hors périmètre
HUD dans les marges (WFS-52), écrans du menu (E8, à leur ticket).
