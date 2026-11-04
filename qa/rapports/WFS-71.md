# QA WFS-71 : Vie de la ville

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : PASSABLE AVEC ANOMALIES MINEURES**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Fusion déjà faite et maintenue ; anomalies mineures ajoutées au tableau `anno.md` (statut 🆕), à prioriser par le PO.

### Captures

| Capture | Constat |
|---|---|
| `vie-terrasse` | vitrines, stores, tables, parasols, clients assis : OK |
| `vie-enseigne-nuit` | panneau néon lumineux : OK |
| `vie-portique`, `vie-plage` | portique illisible, plage sans détail (ANO-40) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-40 | Mineure | `vie-portique`, `vie-plage` | portique « CHECKPOINT 1 » trop loin pour être lu, serviettes et baigneurs de la plage invisibles à cette distance. **Correction** : captures prises à 25 m du portique et à 15 m de la plage (cadrage du banc) |
