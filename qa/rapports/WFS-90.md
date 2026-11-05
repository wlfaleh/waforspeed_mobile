# QA WFS-90 : Conduite libre

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : REFUSÉ**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Ticket déjà fusionné (fusion par confiance / rapide) : chaque anomalie **bloquante** devient un **ticket de correction prioritaire** à créer par le PO ; les mineures sont au tableau `anno.md`.

### Captures

| Capture | Constat |
|---|---|
| `conduite-contresens` | Golf retournée, caméra derrière, roues dans le bon sens : OK |
| `conduite-demitour` | voiture presque hors cadre (ANO-30) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-30 | Bloquante | `conduite-demitour` | pendant le demi-tour, la caméra fixe le trottoir et la Golf est presque hors cadre (seul le haut du toit en bas de l'image) : on ne voit pas la voiture « en travers de la route » (critère). **Correction** : caméra poursuite qui suit le cap avec un lissage de 0,35 s et garde la voiture dans le tiers bas, recul minimal 6 m et hauteur 2,5 m pendant le demi-tour |
