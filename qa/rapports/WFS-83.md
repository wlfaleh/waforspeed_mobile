# QA WFS-83 : Salon du duel

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : PASSABLE AVEC ANOMALIES MINEURES**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Fusion déjà faite et maintenue ; anomalies mineures ajoutées au tableau `anno.md` (statut 🆕), à prioriser par le PO.

### Captures

| Capture | Constat |
|---|---|
| `menu-duel`, `salon-accueil`, `salon-hote`, `salon-pret`, `duel-reseau-course` | « Duel en ligne » actif, pseudo, salon de Nadia, code, 2 pilotes, Prêt, course à 2 : OK ; écran vide et petits textes (ANO-38) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-38 | Mineure | `salon-accueil`, `salon-hote`, `salon-pret` (et `-2400`) | écran presque vide (deux tiers sans contenu), informations en 11 px, liste des pilotes isolée en haut à droite. **Correction** : carte de salon centrale (max 720 px) avec le code en 72 px, voiture, terrain et distance dessous, pilotes en liste sous le code ; libellés 12 px, valeurs 18 px |
