# QA WFS-11 : Garage

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : REFUSÉ**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Ticket déjà fusionné (fusion par confiance / rapide) : chaque anomalie **bloquante** devient un **ticket de correction prioritaire** à créer par le PO ; les mineures sont au tableau `anno.md`.

### Captures

| Capture | Constat |
|---|---|
| `garage-0/1/2`, `garage-peinture-0/1`, `garage-m4`, `garage-suivant`, `garage-peintures`, `garage-course` | plateau, anneau néon, fiche, 16 peintures, nom et index à jour, compteur en arc (`hud-arc-120`, `hud-arc-320` zone rouge) : OK |
| `garage-*` | néons qui traversent le titre (ANO-22), reflets parasites sur la carrosserie (ANO-21), voiture noire invisible (ANO-23), panneau en escalier et crédit 8 px (ANO-33) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-22 | Bloquante | `garage-0`, `garage-1`, `garage-2`, `garage-sf90`, `garage-m4` | néons verticaux cyan et orange du décor qui traversent le titre (« GOLF 8 | GTI », « SF90 XX »), la description et la barre d'index : chevauchement texte / décor, aucun voile à gauche. **Correction** : voile `.screen--garage` du web (style.css l. 366 : 100deg, α 0,92 à 0 %, 0,7 à 34 %, 0 à 62 %) en `uiQuad`, et néons placés à x ≥ 45 % de l'écran (ou derrière le plateau) |
| ANO-33 | Mineure | `garage-0`, `garage-0-2400` | coin haut droit du panneau des caractéristiques en escalier (deux rectangles décalés), crédit du modèle en 8 px à peine lisible, première vue de la voiture de l'arrière. **Correction** : biseau du panneau en `uiQuad` (coin coupé 14 px), crédit 10 px #8A90A6, cap de départ 0,6 + π (3/4 avant comme le web) |
