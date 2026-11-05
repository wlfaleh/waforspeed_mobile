# QA WFS-37 : Menu principal

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : REFUSÉ**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Ticket déjà fusionné (fusion par confiance / rapide) : chaque anomalie **bloquante** devient un **ticket de correction prioritaire** à créer par le PO ; les mineures sont au tableau `anno.md`.

### Captures

| Capture | Constat |
|---|---|
| `menu-0` / `menu-1` | ville assombrie, logo WA FOR / SPEED, 3 choix, « Golf 8 GTI » : OK ; aucun HUD ni commande sur le menu : OK |
| `menu-0` | voile en 16 bandes verticales visibles (ANO-15) |
| `menu-1` | record « ? » au lieu de « — » (ANO-16) |
| `menu-0`, `menu-1` | petits textes : « a » lu « o » (ANO-17) ; aide en texte brut qui ne suit pas le réglage (ANO-32) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-15 | Bloquante | `menu-0`, `menu-1`, `menu-duel`, `menu-bientot` | voile du menu dessiné en 16 bandes verticales de 80 px (paliers de 5 à 6 niveaux bien visibles sur la route, ligne y = 700 : 15 → 20 → 25 → 28 → 34 → 39 → 44 → 52 → 57 → 64 → 69). **Correction** : remplacer la boucle `N = 16` de `menu.cpp` (l. 96-102) par deux `r.uiQuad` à couleurs par sommet (WFS-65) : 0 → 42 % (α 0,94 → 0,78) et 42 → 100 % (α 0,78 → 0,12), voile `linear-gradient(100deg…)` du web (style.css l. 263) ; recouvre WFS-96 en cours (« voile sans bandes ») |
| ANO-16 | Bloquante | `menu-1` | record affiché « ? » au lieu de « — » (critère « record « — » ») : U+2014 absent de l'atlas de police (`text.cpp` l. 55-58 : ASCII, Latin-1, œ ’ … € seulement). **Correction** : ajouter 0x2013, 0x2014, 0x2022, 0x2192, 0x2248 à la liste `cps` et remplacer tout caractère absent par l'espace (jamais « ? ») ; recouvre WFS-95 en cours |
| ANO-32 | Mineure | `menu-0`, `menu-1` | aide « Flèches à gauche : volant. À droite : Gaz, Frein (à l'arrêt : marche arrière), Nitro. » en texte brut sous le menu, qui ne suit pas le réglage Direction (Glisser par défaut). **Correction** : texte selon le réglage (« Glisse à gauche pour tourner »), libellé 12 px #8A90A6, ou l'enlever (le web ne la montre qu'en `pointer: coarse`) |
