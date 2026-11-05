# QA WFS-21 : Duel (et suite #44)

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : PASSABLE AVEC ANOMALIES MINEURES**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Fusion déjà faite et maintenue ; anomalies mineures ajoutées au tableau `anno.md` (statut 🆕), à prioriser par le PO.

### Captures

| Capture | Constat |
|---|---|
| `duel-course` | panneau « POSITION 2/2 », « Alex (toi) », étiquette « Nadia » : OK |
| `duel-resultat` | « DUEL TERMINÉ », 2e place, 0:04.5, Nadia 0:04.0 / Alex +0,5 s, panneau derrière en « POSITION 2/2 » : OK ; boutons plats et « 2E » (ANO-35) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-35 | Mineure | `duel-resultat` | boutons « Revanche » / « Menu » rectangulaires plats alors que l'écran de fin solo utilise des parallélogrammes ; « 2E PLACE » (abréviation fautive). **Correction** : même biseau 14 px que `course_system.cpp`, texte « 2e PLACE » |
