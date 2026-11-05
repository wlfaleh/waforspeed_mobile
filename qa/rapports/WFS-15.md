# QA WFS-15 : Qualité graphique et bloom

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : REFUSÉ**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Ticket déjà fusionné (fusion par confiance / rapide) : chaque anomalie **bloquante** devient un **ticket de correction prioritaire** à créer par le PO ; les mineures sont au tableau `anno.md`.

### Captures

| Capture | Constat |
|---|---|
| `qualite-basse/moyenne/haute` | Basse plus floue sans ombres, halo en Moyenne / Haute : OK |
| nuit | images fantômes, traînées et halos en fleur (ANO-27) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-27 | Bloquante | `qualite-haute`, `qualite-moyenne`, `course-nuit-ville-*`, `nuit-plage-corniche`, `carte-plage-nuit`, `carte-circuit-nuit`, `course-nuit-circuit-*` | bloom qui dédouble l'image : copies fantômes du panneau « TURBO CAFÉ » au-dessus et en dessous, traînées horizontales sur les tribunes du circuit, halo en « fleur » à 6 pétales autour des têtes de lampadaire. **Correction** : flou gaussien séparable (9 prises) à chaque niveau au lieu de prises espacées, échantillonneur `CLAMP_TO_EDGE`, chaîne de 5 niveaux descendants puis montants (« dual filter »), seuil 1,0 avec genou doux 0,5 |
