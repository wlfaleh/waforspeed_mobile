# QA WFS-42 : Vitrages et jantes, trame de points (lunette arrière de la Golf, rayons des jantes)

## QA après fusion, recette de nuit (2026-11-04 18:34:47 UTC, qa-lead, accent qualité et design)

**Verdict : REFUSÉ**

- Captures regardées : Release `apercu` (main, construction 547, commit `dade9be`, publiée après le run 41883566488), comparées à `apercu-waforspeed-80` (construction 537) : 189 captures identiques, 26 qui ne changent que par le trafic, les piétons ou le code de salon tiré au hasard.
- Référence design : skill `design-street-racing` (tokens.md) et jeu web `waaps/waforspeed/` (css/style.css, index.html).
- Ticket déjà fusionné (fusion par confiance / rapide) : chaque anomalie **bloquante** devient un **ticket de correction prioritaire** à créer par le PO ; les mineures sont au tableau `anno.md`.

### Captures

| Capture | Constat |
|---|---|
| `vitrine-golf-0` | lunette arrière sans trame de points visible, feux arrière rouges : OK |
| `vitrine-golf-0`, garage, trafic | traits et points brillants qui flottent sur et au-dessus de la carrosserie (ANO-21) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-21 | Bloquante | `vitrine-golf-0`, `garage-0`, `garage-peinture-1`, `garage-peintures`, `trafic-clignotant`, `trafic-freinage` | traits et points brillants qui flottent sur la carrosserie et **au-dessus du toit** (traits blancs dans le vide au-dessus de la Golf, gerbes cyan et orange sur la lunette et les feux au garage) : reflet du soleil « élargi » des pièces fines opaques. **Correction** : retirer l'élargissement du reflet des opaques fins et leur appliquer le même facteur de couverture du pixel que les transparents ; plafonner le spéculaire à 4,0 avant le bloom |


**Verdict : REFUSÉ** (banc d'essai PC), 1er passage

- Construction regardée : commit `5444e02` (branche `feature/waforspeed-42`). Release `apercu-waforspeed-42`
  (cible `5444e02ed0ea…`, publiée le 2026-11-03 09:03 UTC), run Actions 41605671288 réussi (banc-essai, android, publication).
- Référence « avant » : Release `apercu-waforspeed-34` (mêmes 28 PNG, mêmes cadrages).
- Cause annoncée au Journal (RG-01, critère 1 rempli) : géométrie plus fine qu'un pixel non préfiltrée (Golf pièce 17,
  911 matériau 4). Correction : opacité × couverture du pixel (transparents fins), reflet du soleil élargi (opaques fins).

## Captures regardées

Les 28 PNG, comparées pixel à pixel avec `apercu-waforspeed-34`, puis agrandies ×3 à ×5 :
`vitrine-golf-0` (lunette arrière), `vitrine-p911-0/1/2` (jantes), `course-cam0-0` (lunette vue de derrière),
`vitrine-m4-0`, `vitrine-golf-1`, `trafic-choc`.

Écarts avant / après : uniquement sur les voitures (`moteur-courbe` identique) ; aucune image déformée, cassée ou noire.

## Mesures (agrandissement, même cadre avant / après)

| Zone | Mesure | Avant (34) | Après (42) |
|---|---|---|---|
| `vitrine-golf-0`, lunette arrière (470-555 × 300-335) | points plus clairs que leurs voisins (écart > 4 / > 10 niveaux) | 592 / 367 | 616 / 401 |
| `vitrine-p911-2`, jante avant | points isolés (> 20 niveaux au-dessus des 8 voisins) | 44 | 40 |
| `vitrine-p911-2`, jante arrière | idem | 46 | 40 |
| `vitrine-p911-0`, jante arrière | idem | 37 | 42 |
| `vitrine-p911-0`, jante avant | idem | 17 | 17 |
| `vitrine-p911-1`, jante arrière gauche (dans l'ombre) | idem | 7 | 9 |

## Journal du job `banc-essai` (125814750328)

- `mode exact : 39 voitures comparées au jeu web, 0 écart(s)` : OK. Banc mené jusqu'au bout, aucun abandon.
- Lignes « matériau N très fin (x mm) → préfiltré » : présentes (Golf 17 à 2,0 mm, 911 matériau 4 à 1,9 mm) mais **68
  matériaux** sont marqués (Golf 11, M4 38, 911 8, Gallardo 11, jusqu'à 0,1 mm) : la « hauteur médiane des triangles »
  mesure des triangles étroits, pas des pièces fines ; presque toute la voiture passe dans le chemin « préfiltré ».
- Coût : le journal ne donne que la passe d'ombre (33,7 / 34,0 / 46,8 / 43,6 ms, banc lavapipe non représentatif) ;
  **aucune mesure de la passe scène** : le critère « ±0,5 ms » n'est pas vérifiable.

## Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-01 | Bloquante | `vitrine-golf-0` (lunette arrière), `course-cam0-0` | La trame régulière de points clairs est toujours là, identique à `apercu-waforspeed-34` (592 → 616 points). Les fils de dégivrage vus de derrière sont inchangés. Critère 2 non rempli. Piste : dans `scene.frag`, la branche `F_GLASS` réécrit `alpha = clamp(0.55 + Fg)` après `alpha = base.a * detailK` : si la pièce 17 porte le drapeau vitre, la couverture du pixel est perdue ; son reflet utilise `D_GGX(NoH, 0.0004)` sans `aaKernel`. |
| ANO-02 | Bloquante | `vitrine-p911-0`, `vitrine-p911-1`, `vitrine-p911-2` (jantes) | Les points brillants isolés sur les rayons restent (37 à 46 par jante, comme avant). Jantes toujours métalliques (pas d'aspect mat), mais l'effet de la correction n'est pas visible. Critère 3 non rempli. Dans `vitrine-p911-1` les jantes sont presque entièrement dans l'ombre : regarder aussi `p911-0` et `p911-2`. |
| ANO-03 | Mineure | journal `banc-essai` | Coût GPU de la passe scène absent du journal (seule la passe d'ombre est mesurée). Ajouter une mesure avant / après (banc lavapipe non représentatif, à noter tel quel). |

Effets de bord vus (non classés, à revoir au prochain passage) : feux arrière de la Golf nettement plus rouges
(`vitrine-golf-0`, `course-cam0-0`), tache blanche derrière la roue avant de la 911 devenue grise (`vitrine-p911-1`) :
conséquence du préfiltrage appliqué à 68 matériaux. Pas une dégradation en soi, mais hors du but du ticket.

## Suite

Le développeur corrige ANO-01 et ANO-02 (sans limite de tours tant qu'elles sont bloquantes) et ANO-03 si possible,
puis nouvelle construction et nouveau passage QA. Pas de fusion.
