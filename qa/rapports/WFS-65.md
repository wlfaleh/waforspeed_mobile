# QA WFS-65 : Effets de choc et de nitro (et correctifs #58)

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : REFUSÉ**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Ticket déjà fusionné (fusion par confiance / rapide) : chaque anomalie **bloquante** devient un **ticket de correction prioritaire** à créer par le PO ; les mineures sont au tableau `anno.md`.

### Captures

| Capture | Constat |
|---|---|
| `accident-fumee`, `trafic-accident` | fumée en nuages ronds, plus de bande nette : OK |
| `choc-etincelles` | pas d'étincelles visibles (ANO-25) |
| `nitro-lignes` | ni traits en étoile ni vignette cyan (ANO-25) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-25 | Bloquante | `choc-etincelles`, `nitro-lignes` | `choc-etincelles` : aucune gerbe d'étincelles visible, seul le flash rouge plein écran ; `nitro-lignes` : quelques traits derrière la voiture, pas de traits en étoile sur les bords ni de vignette cyan (critères du ticket). **Correction** : étincelles émissives 24 à 40 par choc, 3 px, 0,35 s, #FFB347 → #FFFFFF, au point de contact ; nitro comme `fx.js` : 18 traits radiaux depuis les bords (α 0,35), vignette rgba(0,229,255,.18) |
