# QA WFS-45 : Projecteurs (spots)

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : REFUSÉ**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Ticket déjà fusionné (fusion par confiance / rapide) : chaque anomalie **bloquante** devient un **ticket de correction prioritaire** à créer par le PO ; les mineures sont au tableau `anno.md`.

### Captures

| Capture | Constat |
|---|---|
| `spots-sol` | sol sombre, brouillard noir, Golf éclairée sur ses bords : OK ; **aucune** tache orange, cyan ni blanche au sol (critère) (ANO-23) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-23 | Bloquante | `spots-sol`, `garage-m4`, `garage-2` (911 noire) | aucune des 3 taches de lumière (orange, cyan, blanche) sur le sol (critère du ticket) ; voiture noire presque invisible sur le fond noir du garage. **Correction** : sol métal 0,7 → 0,2, rugosité 0,6, albedo #1A1C24 pour que le diffus des projecteurs se voie ; ajouter un liseré de contre-jour cyan (#00E5FF, force 0,25) sur la carrosserie ; voiture vue de 3/4 avant (cap 0,6 + π) |
