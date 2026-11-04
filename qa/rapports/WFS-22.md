# QA WFS-22 : Manette (partie Android ; iOS gelé)

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : VALIDÉ**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Aucune anomalie : rien à corriger.

### Captures

| Capture | Constat |
|---|---|
| `manette-course`, `manette-pause`, `manette-menu-focus`, `manette-garage-focus`, `manette-terrain-focus` | HUD sans commandes tactiles, focus cyan sur le bon bouton ; `manette-journal.txt` présent. Partie iOS non recettée (gel) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — | — | — | Aucune anomalie. |
