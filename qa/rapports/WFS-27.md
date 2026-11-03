# QA WFS-27 : [MVP] HUD de course (vitesse, temps, score, checkpoint, nitro — texte, version réduite)

**Verdict : À CORRIGER (tour 1/2)** — banc d'essai PC, 1er passage. **Pas de fusion** pour l'instant.
Aucune anomalie bloquante ; deux anomalies mineures à corriger par le développeur.

## 1er passage — commit `29139a4`

- Construction regardée : commit `29139a4` (`feature/waforspeed-27`), PR #16, run Actions 41617208849 réussi
  (android, banc-essai, publication). Release `apercu-waforspeed-27` (cible `29139a4…`, 37 captures du 2026-11-03 10:25 UTC).
- Banc : `mode exact : 39 voitures comparées au jeu web, 0 écart(s)` et `course automatique : 22 lignes comparées au jeu web, 0 écart(s)`.
- Référence de non-régression : Release `apercu-waforspeed-44` (`240ca10`, même base de code que la branche, `b68012f`).
  Comparaison pixel par pixel des 32 captures communes : `ecran-menu-vide`, `scene-dediee-vide`, `moteur-courbe` **identiques** ;
  `course-fin` ne change que dans la zone des anciennes barres (x 22-310, y 29-64) ; toutes les autres ne changent **que dans
  les trois panneaux du HUD**, plus le texte du temps qui déborde de son panneau (ANO-11). Rendu 3D, voitures, ombres, trafic : inchangés.

| Critère / règle | Résultat |
|---|---|
| RG-01 : HUD seulement en course, rien à l'écran de fin ni dans les menus | OK : `course-fin` sans HUD (écran de WFS-12 seul), `ecran-menu-vide` et `scene-dediee-vide` identiques. Mais voir ANO-11 (HUD dans les vitrines du banc). |
| RG-02 : temps à 1 décimale avec virgule, halo cyan ; rouge sous 8 s | OK : `hud-course` « 24,0 » blanc avec halo cyan ; `hud-temps-bas` « 5,5 » en rouge (journal : temps 5.5 s). |
| RG-03 : « CHECKPOINT n » + barre cyan → orange | OK : `hud-course` « CHECKPOINT 1 », barre dégradée ; `course-cam1-0` « CHECKPOINT 2 », barre presque vide après le passage. |
| RG-04 : score en haut à droite, format français, au-dessus de 22 % de H ; « Combo ×N » | OK : panneau y 14-106 (< 158 px) au-dessus du bouton Nitro ; « 3 090 » (`course-cam1-0`) avec espace fine ; `hud-combo` « Combo ×2 » en cyan (journal : combo 2). |
| RG-05 : vitesse en bas au centre, entre volant et frein ; unité KM/H | OK : panneau x 525-755, volant finit vers x 295, frein commence vers x 835 ; valeurs = journal (126, 134, 147, 150 km/h). |
| RG-06 : NITRO + 10 cases vertes, halo en nitro active | OK : `hud-course` 4 cases (nitro 0,40) ; `hud-nitro` 3 cases (0,30, active) plus lumineuses avec halo vert, libellé « NITRO » vert ; `hud-combo` 3 cases sans halo. |
| RG-07 : anciennes barres retirées, commandes inchangées | OK : plus de barres en haut à gauche (`course-fin`, `course-cam*`) ; volant, frein, gaz, Nitro identiques. |
| RG-08 : marges 16 / 14 px, 1280x720 et 2400x1080 | OK : panneaux à x 16 / 1264, y 14 / 706 ; `hud-2400` à l'échelle, sans planche de texte, rien de coupé ni sous les commandes. |
| RG-09 : fond vitré, polices de WFS-26 | OK. |
| `course-decompte-3` / `-go` | HUD affiché par-dessus la course pendant le décompte (« 30,0 », 0 km/h) ; décompte de WFS-12 inchangé (aucun écart hors panneaux). |

## Anomalies

| ANO | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-11 | Mineure | `course-cam0-*`, `course-cam1-*`, `trafic-*`, `ombres-*`, `texte-essai-*`, `vitrine-*` (les 12) | Dans les scènes du banc sans règles de course (`courseBenchRules(false)`, `timeLeft = 1e9`), le HUD affiche un temps « 1000000000,0 » / « 999999995,0 » qui **déborde à droite du panneau TEMPS** (texte jusqu'à x ≈ 250, panneau jusqu'à 216) ; et le HUD de course s'affiche **dans les 12 vitrines** des voitures, qui ne sont pas une course. Le joueur ne le verra pas (temps réel ≤ quelques dizaines de s), mais les captures de preuve de `course-*` montrent une donnée fausse et les vitrines (référence des autres tickets) sont encombrées. À corriger : ne pas dessiner le HUD quand les règles de course sont coupées (scènes de banc) ou y donner un temps réaliste, et ne pas dessiner le HUD dans les vitrines. |
| ANO-12 | Mineure | `hud-course`, `hud-nitro`, `course-*` (1280x720) | Les libellés à 11 px sont flous et mal formés à 720 p : « TEMPS » se lit « IEMPS », « NITRO » a des lettres déformées, « CHECKPOINT 1 » est à peine lisible (le numéro du checkpoint est une information de jeu). En 2400x1080 (`hud-2400`) ils sont nets. À corriger : caler les libellés sur le pixel entier (position et taille) ou fixer une taille minimale lisible à 720 p (ex. 12-13 px), sans changer le reste du style. |

Tour de correction : **1/2**. Prochaine étape : le développeur corrige ANO-11 et ANO-12, relance la CI, puis le QA regarde les nouvelles captures.
