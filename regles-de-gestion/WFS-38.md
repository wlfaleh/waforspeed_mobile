# WFS-38 — Menu tactile : appuis et départ de course
Épic : E8 (Écrans et menus) · Priorité : 0.4 · Dépend de : WFS-37, WFS-12

## Besoin
Les choix du menu réagissent au doigt : « Course solo » lance la course avec le décompte 3-2-1-GO de WFS-12.

## Règles de gestion
- RG-01 : appui = état pressé visible (barre orange à gauche + léger décalage de 2 px, web `.mitem:hover/active`) ;
  l'action part au **relâcher** sur le même choix (glisser hors du choix annule).
- RG-02 : « Course solo » → `Course` (décompte puis course, graine aléatoire `Math.random() * 2^32` du web `startRace`).
  Tant que le garage (WFS-11/40) et le terrain (WFS-41) n'existent pas, on part **directement** en course ; ils s'inséreront
  ensuite comme dans le web (menu → garage → terrain → course).
- RG-03 : « Showroom » → écran `Garage` (en attendant WFS-11 : bandeau « Bientôt » 1,5 s, on reste au menu) ;
  « Duel en ligne » grisé avec la mention « Bientôt » (WFS-21).
- RG-04 : le menu garde le toucher (`onTouch` renvoie vrai) ; aucune commande de conduite ne s'active.
- RG-05 : fin de course (écran de WFS-12) : bouton « Menu » à côté de « Rejouer » (web `#menuBtn` → `toMenu()`).

## À réutiliser
`js/ui/screens.js` → `startRace`, `toMenu`, écouteurs `#startBtn`, `#garageBtn`, `#menuBtn` ; `css/style.css` `.mitem`, `.btn`, `.btn--ghost`.
Natif : `CourseSystem` (WFS-12, accroche `onEcran` ≤ 10 lignes annoncée), `src/ui/ecrans/menu.*` (WFS-37).

## Critères d'acceptation
- [ ] `menu-appui` : « Course solo » à l'état pressé
- [ ] `menu-depart-3` puis `menu-depart-go` : décompte de WFS-12 après l'appui simulé par le banc
- [ ] `fin-menu` : écran de fin avec « Rejouer » et « Menu » ; appui sur « Menu » → capture du menu

## Hors périmètre
Garage (WFS-11), terrain (WFS-41), pause (WFS-39), duel (WFS-21).
