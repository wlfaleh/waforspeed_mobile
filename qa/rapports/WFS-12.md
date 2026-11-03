# QA WFS-12 : Règles de course (chrono, checkpoints, score, nitro, quasi-contacts)

**Verdict : À CORRIGER — tour 1/2** (banc d'essai PC), 1er passage. Anomalies **mineures** seulement : pas encore de fusion.

- Construction regardée : commit `12662d9` (branche `feature/waforspeed-12`, synchronisée avec `main` 3278fa4), PR #10.
  Release `apercu-waforspeed-12` (cible `12662d97c207…`, texte « Branche feature/waforspeed-12, commit 12662d9 », captures du
  2026-11-03 09:05 UTC). Run Actions 41605341761 : réussi (banc-essai, android, publication).

## Critères d'acceptation

| Critère | Résultat |
|---|---|
| Journal : temps, checkpoints et score d'une course automatique identiques au JS (tolérance nulle) | **OK.** `test_course` : `course automatique : 22 lignes comparées au jeu web, 0 écart(s)`. Les 22 lignes « course de référence : … » de l'étape « Captures du rendu » (3 répétitions) sont identiques à `tests/course_web_ref.txt`. Contre-vérification QA : `tests/course_web.cjs` relancé sur le `waforspeed/` actuel de `wlfaleh/waaps` → sortie identique au fichier de référence. |
| Capture : décompte « 3 » puis « GO » | **OK sur le fond** (`course-decompte-3`, `course-decompte-go`), journal `decompte 3,2,1,GO` ; couleurs inversées par rapport au jeu web : ANO-04. |
| Test unitaire : checkpoint 1 → +11 s, checkpoint 10 → +7 s, quasi-contact ×3 → 750 points | **OK.** `RG-02 checkpoint 1 : +11 s`, `RG-05 quasi-contact ×3 : 250, 500 puis 750 points`. Checkpoint 10 : le test donne **+7,4 s** et checkpoint 11 **+7 s**, conforme à RG-02 et au jeu web (`update.js` l. 255 : `max(5, 11 − level×0.4)` avec `level` = 9 avant le 10e checkpoint). **Le critère « checkpoint 10 → +7 s » est faux, pas le code** : au PO de corriger le critère (+7,4 s). |
| Règles RG-01 à RG-07 (tests unitaires du journal) | OK : départ 30 s, décompte voiture immobile, plancher 5 s, combo remis à 1 après 3 s, combo perdu à 0,09 et gardé à 0,05, décor (recul 300, MAX/8), ralenti ×0,35, fin à 0 s, score, km/h, nitro 0,26/s et seuil 0,01. |

## Captures regardées

- `course-decompte-3` : grand « 3 » centré au-dessus de la Golf, image nette, voiture visible, commandes tactiles en place.
- `course-decompte-go` : « GO » centré, même scène, voiture toujours immobile.
- `course-fin` : voile sombre, panneau « TEMPS ÉCOULÉ », score 5 985, distance 1,20 km, vitesse max 234 km/h, quasi-contacts 0,
  checkpoints 1, bouton « REJOUER ». Valeurs cohérentes entre elles : floor(1 197 m)×5 = 5 985, 1 197 m < 1 200 m → 1 checkpoint.
  Le bouton « Menu » et « Nouveau record » du jeu web sont hors périmètre (WFS-38, WFS-24).
- Non-régression : `vitrine-*` (12), `course-cam0/1-*` (6), `ombres-a/b/coupees`, `texte-essai-1280/2400`, `moteur-courbe`
  **identiques au pixel près** à la Release `apercu` de `main` (3278fa4).
  `trafic-cam0/1`, `trafic-choc`, `ecran-menu-vide` **changent** : les règles de course tournent maintenant dans la scène du
  trafic (checkpoint 2 à 25 s, +6 voitures), donc le trafic n'est plus au même endroit. Attendu (RG-02). Regardées : trafic
  visible devant la Golf (2 à 3 voitures), choc avec tête-à-queue (`trafic-choc`, journal : 1 accident, -1,53 rad),
  route éclairée, pas d'écran noir ni de morceau flottant. `test_traffic` toujours à 0 écart. Pas de régression.

## Anomalies

| ANO | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-04 | Mineure | `course-decompte-3`, `course-decompte-go` | Couleurs du décompte inversées par rapport au jeu web (`.countdown` de `css`) : les chiffres doivent être **blancs** (`--text` #F2F4F8) avec halo cyan, et « GO » **orange** (`--accent` #FF6A00) avec halo orange. Ici « 3 » est orange et « GO » cyan (`course_system.cpp` : `go ? ui::CYAN : ui::ORANGE`). L'ombre portée sombre décalée se lit comme un 2e chiffre gris derrière le premier ; le jeu web met un halo (glow) et du texte en italique. |
| ANO-05 | Mineure | `course-fin` | Écran de fin moins fidèle au jeu web (`#over`) : bouton « REJOUER » rectangulaire, alors que le jeu web et le skill design demandent un **parallélogramme** (`clip-path` biseauté, le commentaire du code parle d'un biseau qui ne se voit pas) ; sur-titre « TEMPS ÉCOULÉ » gris au lieu de cyan (`.eyebrow` = `--accent-2`) ; chiffres du score et des stats droits au lieu d'italiques (`.score-big`, `.stats dd`). Le score blanc est accepté (un seul accent : le liseré et le bouton orange). |

Numéros ANO provisoires : ils ne sont inscrits au tableau des anomalies de `TACHES.md` que s'ils restent après le tour 2/2.

## Suite

Le développeur corrige ANO-04 et ANO-05 (tour 1/2), relance la CI, puis nouveau passage QA sur les captures
`course-decompte-3`, `course-decompte-go`, `course-fin`. Pour le PO : remplacer le critère « checkpoint 10 → +7 s » par
« checkpoint 10 → +7,4 s (checkpoint 11 → +7 s) ».
