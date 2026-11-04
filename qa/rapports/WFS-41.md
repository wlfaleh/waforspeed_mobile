# QA WFS-41 : Terrain et réglages (+ WFS-68)

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : REFUSÉ**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Ticket déjà fusionné (fusion par confiance / rapide) : chaque anomalie **bloquante** devient un **ticket de correction prioritaire** à créer par le PO ; les mineures sont au tableau `anno.md`.

### Captures

| Capture | Constat |
|---|---|
| `terrain-0/1`, `terrain-rouler` | carrousel, 01 / 04 et 04 / 04, Jour / Nuit, flèche grisée, course sur le circuit : OK |
| `terrain-0/1` | texte posé sur l'illustration (ANO-18), « a » lu « o » (ANO-17), sélection rouge (ANO-41) |
| `reglages-*` | panneau Caméra / Graphismes / Son : OK ; cibles de 21 dp et libellés de 5 dp sur téléphone (ANO-19) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-17 | Bloquante | `terrain-0`, `terrain-1`, `menu-0`, `menu-1`, `salon-*` | texte courant de 9 à 12 px illisible : le « a » de Rajdhani Bold se lit « o » et la virgule se lit point (« Avenues entre les grotte-ciel. enseignes ou néon et choussée », « Défie un omi », « Choisis to voiture »). **Correction** : police du texte courant = Inter 500 (OFL, `--font-body` du skill, 15-16 px) pour tout texte < 16 px logiques, Rajdhani gardée pour titres et chiffres ; atlas SDF `BASE_PX` 48 → 64, `PAD` 6 → 8 ; recouvre WFS-95 en cours (« petits libellés lisibles ») |
| ANO-18 | Bloquante | `terrain-0`, `terrain-1`, `terrain-*-2400` | description et longueur de la carte posées directement sur l'illustration (pointillés de la route et reflet cyan en escalier qui passent sous les lettres) : pas d'ombre de bas de vignette. **Correction** : `.map-shade` du web (style.css l. 519) en `uiQuad` sur le bas de la vignette : `linear-gradient(180deg, transparent 30 %, rgba(10,11,16,.6) 52 %, rgba(10,11,16,.97) 68 %, #0A0B10 100 %)` ; reflet central en un seul trapèze (`uiQuad`) au lieu des marches |
| ANO-19 | Bloquante | `reglages-*`, `reglages-menu-2400`, `salon-*-2400`, `hud-2400` | interface trop petite sur téléphone : échelle `k = H / 720` (`chargement.cpp` l. 25) = mise en page d'un écran de bureau de 720 px de haut. Sur un 2400x1080 (≈ 411 dp de haut) : boutons des réglages 54 px ≈ 21 dp (< 44 dp exigés par le skill), libellés 13 px ≈ 5 dp. **Correction** : `k = H / clamp(H / densité, 360, 720)` avec densité = `AConfiguration_getDensity / 160` (1 px CSS = 1 dp, comme le navigateur), puis reprendre les règles `@media (max-height: 500px)` et `(max-height: 520px)` du web (style.css l. 244 et 528) ; cibles tactiles ≥ 44 dp |
| ANO-41 | Mineure | `terrain-1`, `terrain-1-2400` | carte sélectionnée encadrée et marquée « CHOISI » en rouge sur le Grand Prix (rouge = danger dans le skill ; cyan = sélection sur les autres cartes). **Correction** : cadre et badge en #00E5FF pour toutes les cartes, la couleur de la carte reste dans l'illustration |
