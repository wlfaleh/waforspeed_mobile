# WFS-27 — [MVP] HUD de course : vitesse, temps, score, checkpoint, nitro (texte)
Épic : E5 · Priorité : 0.15 (MVP du 2026-11-03) · Dépend de : WFS-26 ✅, WFS-12 ✅ · Taille : ≈ 2 à 3 h

## Besoin (qui, quoi, pourquoi)
Pendant la course, le joueur lit d'un coup d'œil sa vitesse, son temps restant, son score, son checkpoint et sa nitro.
Aujourd'hui il n'a que deux barres sans chiffres (`Game::drawUi`) : sans ce HUD, ce n'est pas encore un jeu.
**Version réduite pour le MVP** : texte + rectangles seulement. Le compteur en arc part en WFS-46, les notifications en WFS-47,
l'italique en WFS-48 (ANO-04). Le décompte 3-2-1-GO existe déjà (WFS-12) : ne pas y toucher.

## Règles de gestion
- RG-01 : nouveau système `src/ui/hud.cpp` (+ `.h` si besoin), dessiné **seulement** quand `vue.ecran == Ecran::Course` et
  qu'une course est active (`activeCourse()` de `src/game/course_system.h`, lecture seule). Rien pendant l'écran de fin
  (l'écran de fin de WFS-12 reste seul à l'écran), rien dans les menus.
- RG-02 : **Temps** (en haut à gauche) : libellé « TEMPS » (11 px, capitales, espacement 0,16, gris `#8A90A6`) puis
  `timeLeft` à **1 décimale** avec virgule (`ui::decimal(t, 1)`), grand (≈ 7 % de la hauteur), blanc `#F2F4F8` avec halo
  cyan ; **rouge `#FF1F4B`** quand `timeLeft < 8` (classe `.low` du web).
- RG-03 : **Checkpoint** sous le temps : « CHECKPOINT » + `level + 1` ; barre de progression 6 px de haut, fond `#1E2130`,
  remplissage cyan `#00E5FF` → orange `#FF6A00`, largeur = `clamp(1 − (course.nextCp − player.dist) / CHECK_DIST, 0, 1)`.
- RG-04 : **Score** (en haut à droite, aligné à droite, **sous le bouton Nitro interdit** : rester au-dessus de y = 22 % de la
  hauteur) : libellé « SCORE » puis `course.score(*vue.player)` au format français `ui::nombre()` (12 345) ; dessous « Combo ×N » en cyan
  seulement si `combo > 1`.
- RG-05 : **Vitesse** (en bas au centre, entre le volant à gauche et le frein à droite, zones des pouces libres) :
  `|kmh|` entier (`Course::kmh`, même formule que le web : `round(speed / MAX_SPEED × 260)`), grand, puis l'unité
  « KM/H » (« R · KM/H » en marche arrière, `speed < −1`) ; chiffres en rouge `#FF1F4B` au-delà de 300.
- RG-06 : **Nitro** sous la vitesse : libellé « NITRO » + 10 cases (`round(player.nitro × 10)` allumées) en vert `#B6FF00`, éteintes
  `#1E2130` ; quand `player.nitroOn`, libellé et cases plus lumineux (halo). Aucune donnée inventée : tout vient de `Course`
  et du joueur (`GameView`).
- RG-07 : les deux barres provisoires de `Game::drawUi` (vitesse et nitro, `game.cpp` l. 184-189) sont **retirées** :
  accroche ≤ 10 lignes dans `src/game/game.cpp`, à annoncer au Journal. Les commandes tactiles ne bougent pas.
- RG-08 : marges de sécurité : 16 px logiques à gauche / droite, 14 px en haut / bas (gouttières `--gutter-*` du web) ;
  lisible en 1280x720 et en 2400x1080 (tailles en fraction de la hauteur).
- RG-09 : style `design-street-racing` : fond vitré `rgba(20,22,31,.72)` derrière chaque bloc, un seul accent fort (orange) ;
  texte en polices de WFS-26 (`ui::text`), pas de nouvelle police.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/ui/hud.js` → `updateHud` (libellés, formats, `low`, `cells`, `R · KM/H`, `red` au-delà de 300) ;
  `js/game/state.js` → `score()`, `kmh()` ; `css/style.css` l. 53-88 (`.hud-top`, `.time-value`, `.time-value.low`, `.cp-bar`,
  `.score-value`, `.combo`, `.nitro-bar`) et variables l. 5-23 (couleurs, gouttières).
- Natif (ne pas recopier) : `src/ui/text.h` (`ui::text`, `ui::width`, `ui::nombre`, `ui::decimal`, `ui::majuscules`,
  couleurs `ui::CYAN/ORANGE/ROUGE`), le halo de `src/game/course_system.cpp` (`halo`) comme modèle, `activeCourse()`.
- Pas de modèle 3D ni de son.

## Critères d'acceptation (captures du banc PC)
- [ ] `course-*` (1280x720) et une capture `hud-2400` (2400x1080) : temps, checkpoint + barre, score, vitesse, nitro lisibles,
  rien sous les commandes tactiles, rien de coupé
- [ ] `hud-temps-bas` : temps < 8 s en rouge ; `hud-nitro` : nitro active, cases allumées ; `hud-combo` : « Combo ×2 » visible
- [ ] les deux anciennes barres ont disparu ; `course-decompte-*` et `course-fin` inchangées ; banc `0 écart(s)`

## Hors périmètre
Compteur en arc (WFS-46), notifications (WFS-47), italique (WFS-48), classement du duel (WFS-21), bouton pause (WFS-39).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| 0.15 | WFS-27 | [MVP] HUD de course : vitesse, temps, score, checkpoint, nitro (texte, version réduite) | E5 | ✅ Terminé |  | WFS-26, WFS-12 | `feature/waforspeed-27` | PR #16 | ✅ VALIDÉ par qa-lead au tour 2/2 (main 6e672a9, ANO-11 et ANO-12 corrigées) [rapport](qa/rapports/WFS-27.md) |

Critères et notes de suivi :

**WFS-27 — [MVP] HUD de course (version réduite)** · E5 · priorité 0.15 · ✅ Terminé, PR #16 (règles : `regles-de-gestion/WFS-27.md`)
- [ ] `course-*` et `hud-2400` : temps, checkpoint + barre, score, vitesse (bas centre), nitro 10 cases lisibles, rien sous les commandes
- [ ] `hud-temps-bas` (rouge sous 8 s), `hud-nitro` (nitro active), `hud-combo` (« Combo ×2 »)
- [ ] anciennes barres de `Game::drawUi` retirées ; `course-decompte-*`, `course-fin` inchangées ; banc `0 écart(s)`
