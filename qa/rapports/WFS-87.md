# QA WFS-87 : Release `apercu` fiable

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : REFUSÉ**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Ticket déjà fusionné (fusion par confiance / rapide) : chaque anomalie **bloquante** devient un **ticket de correction prioritaire** à créer par le PO ; les mineures sont au tableau `anno.md`.

### Captures

| Capture | Constat |
|---|---|
| Release `apercu` | à 17:45 UTC : 96 fichiers sur 222 après des runs annulés (ANO-29) ; complète à nouveau après le run 547 |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-29 | Bloquante | Release `apercu` | Release de main incomplète pendant ≈ 25 min (17:45 → 18:20 UTC) : 96 fichiers sur 222 (menu, garage, HUD, terrain, réglages, pause, duel absents) après les runs 539, 542 et 544 annulés pendant la publication (critère « sans trou »). **Correction** : `concurrency` du job de publication en `cancel-in-progress: false`, et ne supprimer les anciens fichiers qu'après l'envoi complet des nouveaux (ou publier dans `apercu-tmp` puis renommer) |
