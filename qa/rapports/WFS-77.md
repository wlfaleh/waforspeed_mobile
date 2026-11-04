# QA WFS-77 : Planche des 20 voitures

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : REFUSÉ**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Ticket déjà fusionné (fusion par confiance / rapide) : chaque anomalie **bloquante** devient un **ticket de correction prioritaire** à créer par le PO ; les mineures sont au tableau `anno.md`.

### Captures

| Capture | Constat |
|---|---|
| `vitrines-planche` | 20 voitures en Turquoise, noms dessous, GT86 à 4 roues, A45 feux rouges : OK |
| `vitrine-giulia-0`, `vitrine-x5-0`, `vitrine-model3-0` | taches claires sur la peinture (ANO-28) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-28 | Bloquante | `vitrines-planche`, `vitrine-giulia-0`, `vitrine-x5-0`, `vitrine-model3-0` | taches claires sur la peinture (critère « aucune tache ni camouflage ») : grande tache blanche sur la porte arrière de la Giulia, plaque beige sur le hayon et taches grises sur la porte arrière du X5, trait blanc sur la porte de la Model 3. **Correction** : pour les matériaux de carrosserie, ignorer la texture de couleur de base (peinture unie = couleur choisie × 1) ou ne garder la texture que hors masque « peinture » |
