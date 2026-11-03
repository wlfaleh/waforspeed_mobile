# WFS-38 — Menu tactile : appuis et départ de course, fin → Rejouer / Menu
Épic : E8 (Écrans et menus) · Priorité : 0.4 · Dépend de : WFS-37 ✅, WFS-12 ✅

## Besoin (qui, quoi, pourquoi)
Le joueur ouvre l'application, arrive sur le menu (WFS-37), touche « Course solo » et la course démarre avec le décompte
3-2-1-GO de WFS-12 ; en fin de course il choisit « Rejouer » ou « Menu ». C'est le parcours complet du MVP (vérifié par WFS-50).

## Zone de code autorisée
- **E8** : `src/ui/ecrans/**` (surtout `menu.cpp` / `menu.h` de WFS-37 : zones touchables des 3 choix, état pressé, bandeau).
- **Accroches ≤ 10 lignes, annoncées au Journal** (rien d'autre hors zone) :
  1. `src/game/course_system.cpp` (E3) : bouton « Menu » sur l'écran de fin (dessin + toucher, à côté de « Rejouer ») et
     remise à zéro de la course à l'entrée en `Ecran::Course` depuis le menu (`onEcran` → même chemin que `restartReq_`).
  2. `src/platform/android_main.cpp` (E7) : après `ecrans::chargement().fin()`, `app.game.allerA(Ecran::Menu)` (1 ligne).
  3. `src/platform/desktop_main.cpp` (E7) : appel de la nouvelle scène de banc (1 ligne, à côté de `benchMenu`).
- Interdit : `src/core/**`, `src/game/game.*`, `src/ui/hud.*` (E5 actif : WFS-47), CMake. Besoin du noyau → ticket E0.

## Règles de gestion
- RG-01 : **appui = état pressé** (web `.mitem:hover/:active`) : choix décalé de **10 px** à droite (`translateX(10px)`),
  fond `rgba(30,33,48,.9)`, barre gauche de 3 px **cyan** `--accent-2` #00E5FF avec halo (`0 0 12px rgba(0,229,255,.7)`).
  L'action part au **relâcher** sur le même choix ; glisser hors du choix annule (rien ne se passe).
- RG-02 : zone touchable = tout le parallélogramme du choix (mêmes rectangles que le dessin de WFS-37, calculés dans `drawUi`).
- RG-03 : « Course solo » → `allerA(Ecran::Course)` ; à l'entrée la course repart à zéro (voiture, trafic, `Course::reset`,
  donc décompte 3-2-1-GO de WFS-12). Garage (WFS-11/40) et terrain (WFS-41) absents : on part **directement** en course
  (le web fait menu → garage → terrain → course ; ils s'inséreront plus tard). Graine du trafic : celle du « Rejouer » de WFS-12
  (graine aléatoire `Math.random() * 2^32` de `startRace` hors périmètre, à reprendre avec WFS-41).
- RG-04 : « Showroom » → bandeau « Bientôt » 1,5 s sur le menu (on reste au menu) jusqu'à WFS-11 ;
  « Duel en ligne » grisé (opacité 0,45, web `.mitem:disabled`) avec « Bientôt », sans action (WFS-21).
- RG-05 : sur le menu, `onTouch` renvoie **vrai** pour tout toucher : aucune commande de conduite ne s'active, aucun HUD.
- RG-06 : écran de fin : bouton **« Menu »** à droite de « Rejouer », style `.btn--ghost` (fond `--surface-2` #1E2130, texte
  `--text`, même biseau de 14 px et même hauteur que « Rejouer ») ; appui → `allerA(Ecran::Menu)` (web `#menuBtn` → `toMenu()`),
  record du menu mis à jour (déjà fait par WFS-37, RG-09). « Rejouer » inchangé. Mêmes délais que WFS-12 (boutons actifs après 0,9 s).
- RG-07 : sur Android, l'application s'ouvre sur le **menu** après l'écran de chargement (accroche 2). Le banc PC garde son
  départ actuel en `Course` : **toutes les captures existantes restent identiques**.
- RG-08 : banc : nouvelle scène `ecrans::benchMenuTactile` (dans `src/ui/ecrans/`) qui simule les touchers avec `Game::onTouch`
  (appui, maintien, relâcher) et écrit les captures ci-dessous, avec une ligne de journal par capture (écran affiché).

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- `js/ui/screens.js` : `startRace()` (l. 19), `toMenu()` (l. 70), écouteurs `#startBtn`, `#garageBtn`, `#againBtn`, `#menuBtn` (l. 446-449).
- `css/style.css` : `.mitem` (l. 556-572 : `:hover`, `::before`, `:disabled`), `.btn` / `.btn--ghost` (l. 293-304).
- `index.html` l. 228-229 : ordre et textes des boutons de fin (« Rejouer », « Menu »).
- Natif, ne rien refaire : `CourseSystem` (`restartReq_`, `drawOver`, `biseau`) de WFS-12, `Menu` et `CHOIX` de WFS-37,
  `GameView::allerA` / `onEcran` de WFS-36, `ui::text` / `uiRect`. Aucun modèle 3D ni son nécessaire.

## Critères d'acceptation (captures du banc PC)
- [ ] `menu-appui` : « Course solo » à l'état pressé (décalé, barre cyan), les 2 autres choix normaux
- [ ] `menu-depart-3` puis `menu-depart-go` : décompte « 3 » puis « GO » de WFS-12 après le relâcher simulé
- [ ] `menu-bientot` : bandeau « Bientôt » après appui sur « Showroom » ; « Duel en ligne » grisé
- [ ] `fin-menu` : écran de fin avec « Rejouer » et « Menu » ; puis `fin-retour-menu` : menu affiché après appui sur « Menu »
- [ ] aucune commande de conduite sur les captures du menu ; autres captures inchangées, banc `0 écart(s)`

## Hors périmètre
Garage (WFS-11/40), terrain et graine aléatoire (WFS-41), pause (WFS-39), duel (WFS-21), sauvegarde du record (WFS-24),
animation d'attente du menu (voiture qui roule seule, `resetRace('menu', 7)` du web).
