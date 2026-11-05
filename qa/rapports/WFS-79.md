# QA WFS-79 : son iOS (Core Audio)

## QA après fusion, recette de nuit (2026-11-04 23:53:28 UTC, qa-lead)

**Verdict : VALIDÉ** (partie iOS vérifiée en partie, voir plus bas)

- **Sources** : run `ios.yml` de `main` 41877869097 (commit `2ac79d4`, après la fusion de #63), journal du job ; banc PC Android de `main` (construction 562).

| Point | Source | Constat |
|---|---|---|
| `audio iOS : 48000 Hz` | journal du run 41877869097 | OK : ligne présente, avant « QA pret » |
| Android inchangé | captures du banc de `main` | OK : aucune régression vue (seule une condition de préprocesseur a changé) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — | — | — | Aucune anomalie propre au son |

### Non vérifiable (ce n'est pas un motif de refus)

- `audio iOS : n rappels` avec n > 0 : la ligne n'apparaît pas, parce que l'app iOS ne dépasse pas la première image (`vkCmdDrawIndexed` : « non-zero base vertex » refusé par le GPU du simulateur, voir WFS-78 et WFS-88). À revoir au premier run `ios.yml` vert de `main` après la fusion de #76.
- Sur iPhone (son, bouton silencieux, mélange avec la musique de l'utilisateur) : à faire par l'utilisateur.
