# QA WFS-32 : Carrosseries, supprimer les points blancs en trame (reflets du vernis)

**Verdict : VALIDÉ** (banc d'essai PC, règle WFS-31)

- Construction regardée : commit `84b489b` (branche `feature/waforspeed-32`, PR #8). Texte de la Release
  `apercu-waforspeed-32` : « Branche feature/waforspeed-32, commit 84b489b » (cible `84b489bda249…`).
  Run Actions 41537333197 (push) : jobs `banc-essai`, `android`, `publication` verts ; run 41537472452 (pull_request) vert.
- Référence AVANT : captures de la construction 83 de `main` (antérieure à la correction).

## Captures regardées

APRÈS (Release `apercu-waforspeed-32`, les 22 PNG) : `vitrine-golf-0/1/2`, `vitrine-p911-0/1/2`, `vitrine-gallardo-0/1/2`,
`vitrine-m4-0/1/2`, `course-cam0-0/1/2`, `course-cam1-0/1/2`, `trafic-cam0`, `trafic-cam1`, `trafic-choc`, `moteur-courbe`.
AVANT (construction 83) : `vitrine-golf-0`, `vitrine-p911-1`, `course-cam0-*`, `course-cam1-*`, `trafic-*`.
Zones des carrosseries agrandies ×3 à ×8 (pixel par pixel), comparaison avant / après côte à côte, et comptage automatique
des pixels brillants isolés (luminance > 170, les 4 voisins directs plus sombres de 60) et de ceux qui forment une trame
(un autre point isolé exactement à 3 px à droite ou en dessous).

| Capture | Points isolés avant → après | Dont en trame 3 px avant → après |
|---|---|---|
| `vitrine-golf-0` | 66 → 25 | 45 → 2 |
| `vitrine-p911-1` | 43 → 8 | 22 → 0 |
| `course-cam0-0` | 19 → 2 | 11 → 0 |
| `course-cam0-1` | 24 → 8 | 14 → 0 |
| `trafic-choc` | 15 → 5 | 0 → 0 |
| toutes les autres captures APRÈS | 0 à 29 | 0 |

(Les 2 « trames » restantes de `vitrine-golf-0` sont deux points voisins sur une jointure, pas une plaque.)

## Constats par critère

| Critère | Preuve | Résultat |
|---|---|---|
| (a) Cause identifiée et notée au Journal avec capture | Journal 2026-11-02 « WFS-32 : cause trouvée » + PR #8 | OK. Cause : halo de `post.frag` en 5 × 5 lectures espacées de 3 px, qui transforme un pixel HDR très brillant en 25 points isolés en grille ; capture citée `vitrine-golf-0` (construction 83). Cohérent avec ce qu'on voit : plaques de points espacés de 3 px exactement, en 5 × 5, sur la carrosserie et sur la route sous la voiture. |
| (b) Plus de points blancs isolés en trame sur les carrosseries | `vitrine-golf-0` (près du feu arrière, portière, bas de caisse), `vitrine-p911-1` (capot avant, bouclier, plaque), `vitrine-gallardo-0/1/2`, `course-cam0-1`, `trafic-choc`, `trafic-cam0` | OK. Toutes les plaques de points en grille ont disparu (y compris la trame rose près du feu arrière et la trame blanche sur le bouclier arrière de `trafic-cam0`). Gallardo : aucune trame, carrosserie propre. Les points restants sont sur les jantes et le long des jointures (voir ci-dessous). |
| (c) Vernis toujours brillant, aucune autre régression | mêmes captures + comparaison pixel avant/après | OK. Les reflets restent nets et forts (pavillon et hayon de la Golf, capot de la 911 au rendu chromé, reflet du soleil sur le capot moteur de la Gallardo, M4 noire) : pas d'aspect mat. Différences avant/après limitées aux voitures et au halo du soleil (moyenne 0,03 à 0,17 niveau, luminance moyenne identique) : route, ville, ciel, arbres, HUD inchangés. Halo du soleil (`course-cam1-0`, ciel entre les immeubles) toujours présent, bord un peu plus serré qu’avant (halo sur 8 × 8 px au lieu de 13 × 13), sans trou ni trame. Image non déformée, voitures visibles. |
| (d) Coût GPU ±0,5 ms | journal du banc + PR | Accepté par raisonnement, non mesuré. Le banc ne journalise aucun temps GPU et lavapipe (CPU) ne permet pas de mesure fiable (durée totale du banc : 124 s ici, 99 s et 118 s sur les deux dernières constructions de `main`, bruit du CPU). La correction réduit le halo de 25 à 16 lectures et ajoute 2 dérivées (`dFdx`/`dFdy`) + quelques calculs par pixel : coût attendu égal ou inférieur, très en dessous de 0,5 ms. À confirmer sur téléphone en fin d'épic E4. |

## Banc d'essai et correction hors zone (`src/world/ville.cpp`)

Journal du job `banc-essai` 125592398390 : le banc va jusqu'au bout (Golf, M4, 911, Gallardo chargées, Ville
« 899 immeubles, 222 lampadaires, 439 arbres », passages « Système : AudioSystem » sans arrêt, captures `course-*`,
`trafic-cam0/1`, `trafic-choc`, son 28 s, listing des 22 PNG). Aucun « corrupted size » ni abandon. La correction de la
course dans `blob()` est annoncée au Journal (message à toutes les sessions) ; une seule construction ne prouve pas
l'absence d'un défaut aléatoire, mais le développeur indique 3 bancs sur 3 et ThreadSanitizer propre dans notre code.

## Problèmes

Aucun problème bloquant.

## Remarques (non bloquantes)

1. **Fines lignes pointillées sur les jointures** (`vitrine-golf-0` : trappe d'essence, passage de roue arrière, ligne de
   portière, bas de caisse ; `trafic-choc` : bas de caisse). Acceptées au regard du critère (b) : ce ne sont pas des points
   « en trame » (ils suivent les arêtes, sans espacement régulier de grille), ils existaient déjà avant (même aspect sur
   les captures de la construction 83), et ils viennent de triangles plus fins qu'un pixel (crénelage de géométrie que le
   MSAA ×4 ne lisse pas, pas du vernis ni du halo). Ils sont discrets à taille réelle. Le PO devrait l'ajouter
   explicitement aux critères de **WFS-17** (TAA) pour qu'ils ne soient pas oubliés.
2. **Lunette arrière de la Golf** (`vitrine-golf-0`, `course-cam0-*`) : trame régulière de petits points clairs sur la
   vitre, identique avant et après (donc hors de cette correction, et sur le vitrage, pas la carrosserie). Probablement la
   texture du modèle (sérigraphie / dégivrage) ou la transparence ; à vérifier par le PO (ticket E4 ou E3 voitures si c'est
   un défaut).
3. Points restants sur les jantes (`vitrine-p911-2`, 29 points sur les rayons) : reflets sur rayons fins, hors carrosserie.
4. Le Journal (entrée « cause trouvée ») renvoie encore à la construction `4a20e7c` ; la construction jugée ici est `84b489b`.

## Non vérifiable sur le banc PC (pas un motif de refus, WFS-31)

- Coût GPU réel (critère d) et rendu du halo / du vernis sur GPU mobile (précision `mediump`, autre MSAA) : à contrôler à
  la vérification simulateur / appareil de fin d'épic E4.
