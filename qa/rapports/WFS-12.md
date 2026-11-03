# QA WFS-12 : Règles de course (chrono, checkpoints, score, nitro, quasi-contacts)

**Verdict final : PASSABLE AVEC ANOMALIES MINEURES** (banc d'essai PC), 2e passage. **Fusion autorisée.**
Anomalie mineure restante inscrite au tableau des anomalies de `TACHES.md` : **ANO-04** (italique absent).

## 2e passage — commit `f228e46` (tour 2/2)

- Construction regardée : commit `f228e46` (`feature/waforspeed-12`), run Actions 41609393349 réussi (banc-essai, android,
  publication). Release `apercu-waforspeed-12` mise à jour (cible `f228e466…`, texte « commit f228e46 », captures du
  2026-11-03 09:36 UTC). Seul `src/game/course_system.cpp` change dans le code depuis `12662d9`.
- Journal : `course automatique : 22 lignes comparées au jeu web, 0 écart(s)` ; les 66 lignes « course de référence : … »
  (3 × 22) sont identiques à `tests/course_web_ref.txt` ; 16 lignes RG-0x, toutes OK.
- Non-régression : seules `course-decompte-3`, `course-decompte-go` et `course-fin` changent par rapport au 1er passage. Les 28
  autres captures sont identiques au pixel près (vitrine, course, ombres, texte et moteur aussi identiques à `main`).

| Anomalie du 1er passage | Résultat |
|---|---|
| ANO-04 (décompte) — couleurs | **CORRIGÉ.** `course-decompte-3` : « 3 » blanc avec un halo cyan régulier (agrandi ×4 : dégradé propre, sans marches ni copies visibles). `course-decompte-go` : « GO » orange avec un halo orange. L'ombre sombre décalée a disparu. |
| ANO-05 (écran de fin) — bouton, sur-titre, score | **CORRIGÉ.** `course-fin` : bouton « REJOUER » en parallélogramme (biseau des deux côtés, comme `.btn`), « TEMPS ÉCOULÉ » en cyan bien espacé, score 5 985 orange avec halo (`.score-big`). Stats et valeurs inchangées et cohérentes. |
| Italique (décompte, score, chiffres des stats) | **Reste.** Les textes sont droits ; le jeu web les met en italique (`.countdown span`, `.score-big`, `.stats dd`). |

**Décision sur l'italique** : c'est une **anomalie mineure** (rendu un peu différent du jeu web, sans gêne pour le joueur ni
pour les autres tickets). La corriger demande un texte incliné dans `src/ui/text.*`, hors de la zone E3 de ce ticket, donc
un 2e tour de correction dans WFS-12 ne pourrait rien changer. Ce passage compte comme le tour 2/2 ; l'anomalie reste et passe au
tableau des anomalies (**ANO-04**, nouvelle) pour que le PO crée l'US de correction dans l'épic du texte (E5/E8).

| ANO | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-04 | Mineure | `course-decompte-3`, `course-decompte-go`, `course-fin` | Texte droit au lieu d'italique : chiffres du décompte, score et valeurs des stats (jeu web : `font-style: italic`, le décompte est aussi incliné de -8°). Le texte natif ne sait pas incliner les glyphes : à corriger dans `src/ui/text.*`. |

Rappel pour le PO : le critère « checkpoint 10 → +7 s » est à remplacer par « checkpoint 10 → +7,4 s (checkpoint 11 → +7 s) ».

---

## 1er passage — commit `12662d9` : À CORRIGER, tour 1/2
**Verdict : À CORRIGER — tour 1/2** (banc d'essai PC), 1er passage. Anomalies **mineures** seulement : pas encore de fusion.

- Construction regardée : commit `12662d9` (branche `feature/waforspeed-12`, synchronisée avec `main` 3278fa4), PR #10.
  Release `apercu-waforspeed-12` (cible `12662d97c207…`, texte « Branche feature/waforspeed-12, commit 12662d9 », captures du
  2026-11-03 09:05 UTC). Run Actions 41605341761 : réussi (banc-essai, android, publication).

### Critères d'acceptation

| Critère | Résultat |
|---|---|
| Journal : temps, checkpoints et score d'une course automatique identiques au JS (tolérance nulle) | **OK.** `test_course` : `course automatique : 22 lignes comparées au jeu web, 0 écart(s)`. Les 22 lignes « course de référence : … » de l'étape « Captures du rendu » (3 répétitions) sont identiques à `tests/course_web_ref.txt`. Contre-vérification QA : `tests/course_web.cjs` relancé sur le `waforspeed/` actuel de `wlfaleh/waaps` → sortie identique au fichier de référence. |
| Capture : décompte « 3 » puis « GO » | **OK sur le fond** (`course-decompte-3`, `course-decompte-go`), journal `decompte 3,2,1,GO` ; couleurs inversées par rapport au jeu web : ANO-04. |
| Test unitaire : checkpoint 1 → +11 s, checkpoint 10 → +7 s, quasi-contact ×3 → 750 points | **OK.** `RG-02 checkpoint 1 : +11 s`, `RG-05 quasi-contact ×3 : 250, 500 puis 750 points`. Checkpoint 10 : le test donne **+7,4 s** et checkpoint 11 **+7 s**, conforme à RG-02 et au jeu web (`update.js` l. 255 : `max(5, 11 − level×0.4)` avec `level` = 9 avant le 10e checkpoint). **Le critère « checkpoint 10 → +7 s » est faux, pas le code** : au PO de corriger le critère (+7,4 s). |
| Règles RG-01 à RG-07 (tests unitaires du journal) | OK : départ 30 s, décompte voiture immobile, plancher 5 s, combo remis à 1 après 3 s, combo perdu à 0,09 et gardé à 0,05, décor (recul 300, MAX/8), ralenti ×0,35, fin à 0 s, score, km/h, nitro 0,26/s et seuil 0,01. |

### Captures regardées

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

### Anomalies

| ANO | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-04 | Mineure | `course-decompte-3`, `course-decompte-go` | Couleurs du décompte inversées par rapport au jeu web (`.countdown` de `css`) : les chiffres doivent être **blancs** (`--text` #F2F4F8) avec halo cyan, et « GO » **orange** (`--accent` #FF6A00) avec halo orange. Ici « 3 » est orange et « GO » cyan (`course_system.cpp` : `go ? ui::CYAN : ui::ORANGE`). L'ombre portée sombre décalée se lit comme un 2e chiffre gris derrière le premier ; le jeu web met un halo (glow) et du texte en italique. |
| ANO-05 | Mineure | `course-fin` | Écran de fin moins fidèle au jeu web (`#over`) : bouton « REJOUER » rectangulaire, alors que le jeu web et le skill design demandent un **parallélogramme** (`clip-path` biseauté, le commentaire du code parle d'un biseau qui ne se voit pas) ; sur-titre « TEMPS ÉCOULÉ » gris au lieu de cyan (`.eyebrow` = `--accent-2`) ; chiffres du score et des stats droits au lieu d'italiques (`.score-big`, `.stats dd`). Le score blanc est accepté (un seul accent : le liseré et le bouton orange). |

Numéros ANO provisoires au 1er passage. ANO-05 corrigée au 2e passage ; seul le reste d'ANO-04 (italique) est inscrit au tableau, sous le numéro ANO-04.

### Suite

Le développeur corrige ANO-04 et ANO-05 (tour 1/2), relance la CI, puis nouveau passage QA sur les captures
`course-decompte-3`, `course-decompte-go`, `course-fin`. Pour le PO : remplacer le critère « checkpoint 10 → +7 s » par
« checkpoint 10 → +7,4 s (checkpoint 11 → +7 s) ».
