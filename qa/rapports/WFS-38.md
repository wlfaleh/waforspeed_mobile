# QA WFS-38 : menu tactile (appuis, départ de course, fin → Rejouer / Menu)

**Verdict : À CORRIGER** (banc d'essai PC), **tour 1/2**. Une anomalie mineure, aucune bloquante. **Pas de fusion pour l'instant.**

- Construction regardée : commit `e7e73a0` (`feature/waforspeed-38`, PR #27). C'est une fusion de `main` ; le code du ticket est
  celui de `9707a70`, plus `main` (WFS-55). CI au vert (run 41669884877 : `banc-essai`, `android`, `publication`).
  Release `apercu-waforspeed-38` (« commit e7e73a0 »), 61 captures et `WaForSpeed.apk`.
- La branche se fusionne sur `main` (`a310199`) sans conflit (`git merge-tree`).
- Règles : `regles-de-gestion/WFS-38.md` (RG-01 à RG-08). Référence web : `css/style.css` (`.mitem`, `.btn--ghost`), `index.html`.
- Comparaison : `apercu-waforspeed-55` (commit `47d7b85`), dont le code est **identique à `main`** hors fichiers `.md`.

## Captures

| Capture | Constat |
|---|---|
| `menu-appui` (nouvelle) | « Course solo » à l'état pressé : choix décalé de **10 px** à droite (barre de x = 93 à x = 103), barre gauche de 3 px **cyan #00E5FF** avec halo, fond `(28,32,45)`, soit `rgba(30,33,48,.9)` sur fond sombre. « Duel en ligne » grisé avec « Bientôt », « Showroom » normal. Aucune commande de conduite ni HUD. |
| `menu-depart-3` (nouvelle) | Écran de course après le relâcher : décompte « 3 » blanc à halo cyan, chrono 30,0, score 0, 0 km/h. Identique à `course-decompte-3` de WFS-12 : la course est bien repartie à zéro. |
| `menu-depart-go` (nouvelle) | « GO » orange, chrono 29,9 : départ de la course. Golf visible, image non déformée. |
| `menu-bientot` (nouvelle) | Bandeau « BIENTÔT » « Showroom » (vitré, bordure gauche cyan, style des bandeaux de course) au centre ; on reste sur le menu. « Duel en ligne » grisé. |
| `fin-menu` (nouvelle) | Écran de fin : « REJOUER » (biseau orange) puis « MENU » (biseau `#1E2130`, texte clair), même hauteur et même biseau. Paire centrée : x = 388 à 892, milieu 640. |
| `fin-retour-menu` (nouvelle) | Menu affiché après « Menu », record mis à jour à **390** (score de `fin-menu`). Aucune commande de conduite. |
| `course-fin` (changée, voulu) | Seule la zone des boutons change (x 388 à 892, y 504 à 576) : « Menu » ajouté, paire centrée (RG-06). Le reste est identique au pixel. |
| `menu-0`, `menu-1`, `menu-anim`, `ecran-menu-vide` (changées, voulu) | Seule la ligne « Duel en ligne » change : grisée (opacité 0,45), « Bientôt » à droite (RG-04). Le reste est identique au pixel. |
| Les 50 autres captures | **Identiques au pixel** à `apercu-waforspeed-55` (vitrines, `course-cam*`, `course-decompte-*`, `hud-*`, `toast-*`, `trafic-*`, `ombres-*`, `chargement-*`, `carte-*`, `encoche-course`, `texte-essai-*`). |
| Journal du banc | `mode exact`, `plage`, `campagne`, `circuit`, `course automatique` : **0 écart(s)**. Une ligne par capture, et la ligne d'annulation : « menu tactile : appui puis glissé hors de « Course solo » → écran menu (annulé) » (RG-01). Les `[ERREUR]` de `cars/golf.wfm` viennent de la scène voulue `chargement-erreur`. |

## Critères d'acceptation et règles

| Critère / règle | Constat | Résultat |
|---|---|---|
| `menu-appui` : « Course solo » pressé, les 2 autres normaux | voir Captures (« Duel » grisé, c'est RG-04) | OK |
| `menu-depart-3` puis `menu-depart-go` | « 3 » puis « GO » de WFS-12 | OK |
| `menu-bientot` : bandeau « Bientôt », « Duel » grisé | voir Captures | OK |
| `fin-menu` puis `fin-retour-menu` | « Rejouer » et « Menu », puis menu affiché | OK |
| Aucune commande de conduite sur le menu, autres captures inchangées, banc `0 écart(s)` | voir Captures | OK |
| RG-01 état pressé, action au relâcher, glissé hors du choix = annulé | `menu-appui` et journal du banc | OK |
| RG-03 « Course solo » → course remise à zéro, décompte 3-2-1-GO | `menu-depart-*` | OK |
| RG-04 « Showroom » → bandeau 1,5 s ; « Duel » grisé avec « Bientôt » | `menu-bientot`, `menu-0` | OK |
| RG-06 bouton « Menu » `.btn--ghost` à droite de « Rejouer », paire centrée, record mis à jour | `fin-menu`, `course-fin`, `fin-retour-menu` | OK |
| RG-07 Android s'ouvre sur le menu ; banc PC inchangé | `android_main.cpp` : 1 ligne ; captures existantes inchangées | OK (vérification sur appareil : WFS-50) |
| Zone de code : accroche `course_system.cpp` (E3) ≤ 10 lignes | 15 lignes ajoutées et 8 supprimées (`git diff --numstat`) : **7 lignes nouvelles + 8 lignes existantes modifiées**, soit 15 lignes touchées. Le journal annonce « environ 10 lignes, dont 4 modifiées ». | **ANO-01 (mineure)** |

## Choix du développeur

1. **Libellé « Bientôt » de « Duel en ligne » à 80 % d'opacité** (le reste de la ligne à 45 %) : accepté, pour qu'il reste lisible.
   Le jeu web n'a pas ce libellé, il vient de RG-04.
2. **Bouton « Menu » à 0,7 fois la largeur de « Rejouer »** : accepté. RG-06 impose la même hauteur et le même biseau, pas la même largeur.
3. **Course figée et écran de fin masqué quand on est sur le menu** (`collide`, `drawUi` de `course_system.cpp`) : accepté.
   C'est ce qui empêche une partie de tourner derrière le menu (RG-05) ; mais ces lignes comptent dans l'accroche (ANO-01).

## Hors ticket (déjà présent avant WFS-38)

- Le tiret « — » du record vide s'affiche « ? » sur le menu (`menu-0`, `menu-appui`, `menu-bientot` : « RECORD ? »).
  Le glyphe manque dans la police ; le défaut vient de WFS-37 et doit être noté à sa recette (`qa/Check.md`). Il n'est pas compté ici.

## Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-01 | Mineure | — (code : `src/game/course_system.cpp`, E3) | L'accroche dépasse la limite de 10 lignes de `regles-de-gestion/WFS-38.md` : 15 lignes touchées (7 nouvelles, 8 modifiées), et le journal en annonce environ 10, dont 4 modifiées. Pas d'effet pour le joueur, et E3 n'a aucun ticket en cours. **À faire** : ramener l'accroche à 10 lignes au plus, par exemple en regroupant les gardes `g.ecran` ; sinon, corriger l'annonce au journal avec le compte réel et obtenir l'accord du PO pour ce dépassement. |

Tour suivant (2/2) : je regarde de nouveau le diff de `course_system.cpp`, le journal et les captures de la nouvelle Release
`apercu-waforspeed-38`. Toutes les captures doivent rester celles de ce passage.
