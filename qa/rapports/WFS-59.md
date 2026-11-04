# QA WFS-59 : Mode GPU logiciel

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : VALIDÉ**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Aucune anomalie : rien à corriger.

### Captures

| Capture | Constat |
|---|---|
| banc PC | ombres présentes, MSAA, pleine définition sur toutes les captures de main |
| `simulateur-MVP` (commit `2ac6ed5`) | logcat : `mode GPU logiciel : actif (dt max 0,5 s, rendu 25 %, sans ombres)`, `QA ips=1.0` puis 0,6 ; `qa-05-go` 108 km/h, `qa-06` 135 km/h : la course avance |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — | — | — | Aucune anomalie. |
