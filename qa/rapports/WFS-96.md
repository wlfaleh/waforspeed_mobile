# QA WFS-96 : Showroom, quartiers du terrain, focus manette, voile sans bandes, sons d'interface

## QA après fusion, recette de nuit (2026-11-04 22:36:48 UTC, qa-lead, accent qualité et design)

**Verdict : VALIDÉ**

- **Captures regardées** : Release de la branche `apercu-waforspeed-96` (commit `d50a282`, publiée à 19:07 UTC). Le run de `main` du commit de fusion `6863763` (run 41924411860) était encore en cours à 22:36 UTC, et la Release `apercu` en était restée à `f542093`, sans WFS-96. La revue sur `main` sera à refaire quand la Release `apercu` sera à jour. Entre `d50a282` et la fusion, il n'y a eu qu'une synchronisation avec `main`, sans conflit et sans changement dans `src/ui/ecrans/**`.
- **Références** : `regles-de-gestion/WFS-96.md`, skill `design-street-racing`, jeu web `screens.js` (`openGarage`, `mapCard`, `zoneNames`).
- **Anomalies vérifiées** : **ANO-15** (voile en bandes) et **ANO-34** (titre « RÉGLAGES » sur la pause) passent en ✅ Vérifiée.

### Critères d'acceptation

| Critère | Capture | Constat |
|---|---|---|
| Showroom → garage en mode regarder, « Choisir » ramène au menu avec la nouvelle voiture | `menu-showroom`, `showroom-choisir` | OK. Garage ouvert sur la M4, bouton « CHOISIR » et « RETOUR » ; retour au menu avec « TA VOITURE : M4 COMPETITION ». `menu-bientot` n'existe plus |
| Parcours Course solo inchangé | `menu-depart-3`, `manette-garage-focus`, `manette-terrain-focus` | OK |
| « Quartiers » et noms des quartiers sur la carte | `terrain-0`, `terrain-1`, `-2400` | OK. Ville : « QUARTIERS 6 » et 6 pastilles (Centre-ville … Vieux quartier) ; Grand Prix : 4. Rien ne déborde, ni en 1280 ni en 2400. Les libellés 8-9 px sont à peine lisibles (« QUART ERS », « Quartier d'offoires ») : c'est ANO-17 (WFS-95) |
| Voile du menu sans rayures | `menu-0`, `manette-menu-focus` | OK. Sur la ligne y = 700, luminance continue 12 → 67, par pas de 0 à 3 niveaux, sans paliers |
| Titre « PAUSE » | `pause-0` | OK. « COURSE EN PAUSE » / « PAUSE » au-dessus des 3 boutons |
| Focus manette sur le 1er choix | `manette-menu-focus`, `manette-garage-focus`, `manette-terrain-focus`, `manette-journal.txt` | OK. Focus 0 sur « Course solo », « Choisir » et « Rouler ». `manette-reglages` est pris après un déplacement (cadre sur « Flèches ») : l'ouverture n'y est pas visible. Journal identique à celui de `main` avant la fusion |
| Journal « SFX garage » / « SFX carte » | — | Pas vérifiable : le journal complet du banc n'est pas publié dans la Release, seulement `manette-journal.txt`. Le code est en place : `sfxInterface(SfxId::Garage / Carte)` est appelé, et `SfxMixer::play` écrit `SFX %s` dans le journal |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-42 | Mineure (origine WFS-41, pas une régression de WFS-96) | `terrain-1`, `terrain-1-2400`, `terrain-0` | Illustration de la carte dessinée en bandes. Ciel en paliers de 4 niveaux tous les 14 px ; dans l'herbe et sur la route, des raccords sombres (−17 niveaux) tous les ≈ 100 px, qui font des lignes horizontales sur toute la vignette. Nuages faits de disques translucides superposés, plus clairs là où ils se chevauchent. **Correction** (`terrain.cpp`) : ciel en deux `uiQuad` à couleurs par sommet (WFS-65), arrêts 0 → 0,62 → 1 du `linearGradient` du web (`screens.js` l. 209) ; sol et route en un `uiQuad` dégradé chacun. Nuages comme au web : 3 ellipses (rx w/2, ry 9 ; rx w/4 ; rx w/5) **opaques** de la couleur déjà mélangée (#FFFFFF × 0,85 + ciel × 0,15), pour reproduire l'opacité de groupe `<g opacity=".85">` |
| (ANO-17) | Bloquante, déjà planifiée | `terrain-*`, `menu-0` | Petits libellés illisibles → WFS-95 |
| (ANO-22) | Bloquante, déjà planifiée | `menu-showroom` | Néons sur le titre du garage → WFS-99 (fusionné à 22:32 UTC, recette quand il y aura des captures) |
| (ANO-32) | Mineure, déjà planifiée | `menu-0`, `showroom-choisir` | L'aide « Flèches à gauche… » est toujours là → WFS-102 |

### À noter (hors ticket)
`showroom-choisir` : le fond du menu est la scène de nuit. On y voit les fantômes du bloom autour des lampadaires (ANO-27, WFS-100) et des reflets en pointillés sous la M4 sur la chaussée mouillée. C'est à revoir avec WFS-100.
