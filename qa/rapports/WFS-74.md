# QA WFS-74 : Chargement des cartes

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : PASSABLE AVEC ANOMALIES MINEURES**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Fusion déjà faite et maintenue ; anomalies mineures ajoutées au tableau `anno.md` (statut 🆕), à prioriser par le PO.

### Captures

| Capture | Constat |
|---|---|
| `terrain-chargement` | logo, barre ≈ 20 %, « Chargement du modèle 3D… 20 % » : OK |
| `terrain-arrivee`, `terrain-fige-1/2` | circuit de jour, décompte, chrono figé : OK |
| `chargement-*` | fond plat, erreur sans action (ANO-39) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-39 | Mineure | `chargement-*`, `terrain-chargement` | fond de chargement plat #0A0B10 sans les reflets d'asphalte du skill ; écran d'erreur sans action (« Le moteur 3D n'a pas pu démarrer… » seul, 10 px rouge). **Correction** : `radial-gradient(120% 60% at 50% 110%, rgba(255,106,0,.14))` + `radial-gradient(80% 50% at 85% 100%, rgba(0,229,255,.10))` ; erreur en 16 px + bouton « Réessayer » |
