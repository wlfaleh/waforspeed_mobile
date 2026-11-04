# QA WFS-09 : Nuit (+ WFS-16)

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : PASSABLE AVEC ANOMALIES MINEURES**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Fusion déjà faite et maintenue ; anomalies mineures ajoutées au tableau `anno.md` (statut 🆕), à prioriser par le PO.

### Captures

| Capture | Constat |
|---|---|
| `course-nuit-ville-*` | route mouillée qui reflète immeubles et feux, stop plus forts, recul blanc : OK ; façades trop claires (ANO-37) |
| `course-nuit-campagne-*`, `-circuit-*`, `carte-plage-nuit` | faisceau des phares, projecteurs, brouillard : OK |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-37 | Mineure | `course-nuit-ville-*`, `qualite-*` | la nuit en ville, les façades sont très claires (lavande / rose) : on dirait le crépuscule, les néons et les reflets ressortent peu. **Correction** : hémisphère de nuit × 0,45, façades #1A1C28, 30 % des fenêtres émissives au hasard (#FFD9A0 et #9FE8FF) |
