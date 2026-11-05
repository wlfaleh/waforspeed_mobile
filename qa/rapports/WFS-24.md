# QA WFS-24 : Sauvegarde et record

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : PASSABLE AVEC ANOMALIES MINEURES**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Fusion déjà faite et maintenue ; anomalies mineures ajoutées au tableau `anno.md` (statut 🆕), à prioriser par le PO.

### Captures

| Capture | Constat |
|---|---|
| `fin-record` | badge « NOUVEAU RECORD », ligne record égale au score : OK ; mise en page serrée (ANO-36) |
| `sauvegarde-relue` | BMW M4 rouge relue : OK |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-36 | Mineure | `fin-record` | badge « NOUVEAU RECORD » à ≈ 4 px des libellés des statistiques, ligne « RECORD 2 130 » serrée entre les stats et les boutons. **Correction** : 16 px sous le badge, ligne record 12 px au-dessus des boutons (panneau agrandi de 28 px) |
