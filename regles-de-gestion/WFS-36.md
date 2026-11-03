# WFS-36 — Point d'extension : écrans de l'application (chargement, menu, garage, course, pause, fin)
Épic : E0 (Noyau partagé) · Priorité : 0.2 · Dépend de : —

## Besoin (qui, quoi, pourquoi)
Aujourd'hui l'application démarre directement en course. Pour brancher le menu (WFS-37/38), le garage (WFS-11), la pause
(WFS-39) et le choix du terrain (WFS-41) **sans que chaque ticket touche au noyau**, `Game` doit connaître l'écran affiché
et laisser un système en demander un autre. Catalyseur : débloque E8 et E5 en parallèle.

## Règles de gestion
- RG-01 : état unique `Ecran { Chargement, Menu, Garage, Terrain, Course, Pause, Fin }` dans `src/core/` (ex. `ecran.h`),
  lu par les systèmes dans `GameView::ecran` (comme `state` du web : `'menu' | 'garage' | 'count' | 'play' | 'pause' | 'over'`).
- RG-02 : un système demande un changement par un appel fourni dans `GameView` (ex. `allerA(Ecran)`), appliqué **entre deux
  images** ; tous les systèmes sont prévenus par une nouvelle méthode virtuelle facultative `System::onEcran(Ecran ancien, Ecran nouveau)`.
- RG-03 : hors `Course`, la conduite ne reçoit **aucune commande** (web : `for (const k in input) input[k] = false`), les commandes
  tactiles de conduite (`drawControls`) ne sont pas dessinées et la voiture reste immobile ; la scène continue d'être dessinée
  derrière les menus (web : `resetRace('menu', 7)` garde la ville en fond). `Pause` fige la simulation (pas de `update`).
- RG-04 : écran de départ = `Menu` dès que le menu existe ; **en attendant, `Course`** (aucun changement visible tant que WFS-37
  n'est pas fusionné). Le banc d'essai peut forcer un écran (`g.allerA(...)`) pour ses captures.
- RG-05 : en passant à `Course`, la course repart du début (web `startRace()` → `resetRace('play', graine)`) : `CourseSystem`
  (WFS-12) écoutera `onEcran` — ligne d'accroche dans E3 au moment de WFS-38.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- `js/ui/screens.js` → `show(name)`, `startRace`, `toMenu`, `pauseGame`, `resumeGame` (transitions et remise à zéro des commandes).
- Natif : `src/core/system.h` (WFS-30), `Game::systemsInit` / `systemsUi`. Rien d'autre à réutiliser.

## Critères d'acceptation
- [ ] Toutes les captures du banc **identiques** (écran `Course` par défaut) ; test de trafic et de course à 0 écart
- [ ] Nouvelle capture `ecran-menu-vide` : la scène sans commandes tactiles, voiture immobile (forcée par le banc)
- [ ] Doc du point d'extension dans le skill `waforspeed-natif` (comme WFS-30)

## Hors périmètre
Tout dessin de menu (WFS-37), pause (WFS-39).
