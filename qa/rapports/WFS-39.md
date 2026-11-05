# QA WFS-39 : Pause en course

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : REFUSÉ**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Ticket déjà fusionné (fusion par confiance / rapide) : chaque anomalie **bloquante** devient un **ticket de correction prioritaire** à créer par le PO ; les mineures sont au tableau `anno.md`.

### Captures

| Capture | Constat |
|---|---|
| `pause-0`, `pause-reprise`, `pause-recommencer`, `pause-quitter` | écran de pause, reprise, recommencer, quitter : OK |
| `hud-course`, `pause-bouton` | bouton « MENU » rayé de lignes sombres (bandes qui se chevauchent, ANO-20) |
| `pause-0` | titre « RÉGLAGES » trompeur (ANO-34) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-20 | Bloquante | `hud-course`, `encoche-hud-2400`, toutes les captures de course | boutons « MENU » et « POURSUITE » (et « RÉGLAGES ») rayés de fines lignes sombres horizontales : parallélogramme fait de 16 bandes translucides de h/N + 0,5 px qui se chevauchent, l'alpha 0,72 est doublé à chaque raccord. **Correction** : `biseau()` en un seul `r.uiQuad` à 4 sommets (x + biais, y) (x + w, y) (x + w − biais, y + h) (x, y + h) dans `camera_vue.cpp`, `pause.cpp`, `reglages.cpp`, `salon.cpp`, `course_system.cpp`, `garage.cpp`, `hud.cpp`, `terrain.cpp`, `chargement.cpp` |
| ANO-34 | Mineure | `pause-0`, `manette-pause` | titre « RÉGLAGES » au-dessus de trois boutons sans aucun réglage (les réglages sont dans le panneau du bouton « Réglages ») : titre trompeur. **Correction** : titre « PAUSE » (le panneau garde « RÉGLAGES ») |
