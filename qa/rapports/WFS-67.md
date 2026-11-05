# QA WFS-67 : Feux du trafic et silhouettes (et correctif #58)

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : REFUSÉ**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Ticket déjà fusionné (fusion par confiance / rapide) : chaque anomalie **bloquante** devient un **ticket de correction prioritaire** à créer par le PO ; les mineures sont au tableau `anno.md`.

### Captures

| Capture | Constat |
|---|---|
| `course-cam*`, `trafic-accident` | silhouettes au loin, détresse, stop, fumée : OK |
| `trafic-clignotant`, `trafic-freinage` | voiture à ≈ 60 m au lieu de 8-25 m (ANO-26) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-26 | Bloquante | `trafic-clignotant`, `trafic-freinage` | la voiture qui clignote / freine est à ≈ 60 m, petite en haut de l'image (critère du correctif #58 : 8 à 25 m, feux bien visibles). **Correction** : placer la voiture du banc à 15 m devant la Golf, file voisine |
