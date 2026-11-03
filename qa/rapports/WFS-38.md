# QA WFS-38 : menu tactile (appuis, départ de course, fin → Rejouer / Menu)

**Verdict final : PASSABLE AVEC ANOMALIES MINEURES** (banc d'essai PC), **tour 2/2**. Aucune anomalie bloquante.
ANO-01 (accroche trop longue) est **corrigée**. La correction crée un petit défaut de mise en page : la paire « Rejouer » / « Menu »
n'est plus centrée. C'est une anomalie mineure, et c'était le dernier tour : elle est inscrite au tableau des anomalies comme
**ANO-14** (`anno.md`). **La fusion est autorisée.**

Historique : 1er passage `e7e73a0`, **À CORRIGER (tour 1/2)** pour ANO-01. Correction `7a047b3` (fusion de `main` : `60cde90`),
**PASSABLE AVEC ANOMALIES MINEURES (tour 2/2)**, avec ANO-14.

> **Remarque de procédure** : la PR #27 a été **fusionnée sur `main` par l'utilisateur** (`27e22c6`, le 2026-11-03 à 17:53 UTC),
> avant ce verdict. Le verdict rendu ici autorise cette fusion. Il n'y a rien à défaire.

## Tour 2/2 : ce qui a été regardé

- Construction : commit `60cde90` (`feature/waforspeed-38`, PR #27). CI au vert (run 41680051281 : `banc-essai`, `android`,
  `publication`). Release `apercu-waforspeed-38` (« commit 60cde90 »), 61 captures et `WaForSpeed.apk`.
- Comparaison au pixel avec les captures du tour 1 (`e7e73a0`) et avec `apercu-waforspeed-55` (code identique à `main` avant WFS-38).

## ANO-01 (numéro propre à ce rapport, tour 1) : accroche de `src/game/course_system.cpp`

| Point | Constat | Résultat |
|---|---|---|
| Nombre de lignes | `git diff --numstat` avant WFS-38 (`d69aa0f`) → `60cde90` : **9 ajoutées, 4 supprimées**. Lignes nouvelles : `onEcran`, garde `collide`, toucher « Menu », dessin et texte de « Menu » (5). Lignes modifiées : signatures de `collide` et `onTouch`, garde d'`onTouch`, garde de `drawUi` (4). Total : **9 lignes touchées, ≤ 10**. | **Corrigé** |
| Contenu | Un seul garde en tête de `drawUi` (`g.ecran != Ecran::Course \|\| restartReq_`) remplace les deux gardes du tour 1. « Rejouer » est revenu exactement au code de `main` (`btn_[0] = cx - bw / 2`, `btn_[4]` supprimé). La zone de toucher de « Menu » (`btn_[0] + 1,05 bw` à `+ 1,75 bw`) est exactement celle du dessin. | OK |
| Effet de bord du garde | Avec `restartReq_` levé, `drawUi` ne dessine plus le décompte pendant l'image de la remise à zéro. Cela dure une seule image, et le décompte n'a pas encore commencé. Les captures `course-decompte-*` et `menu-depart-*` sont identiques au tour 1. | OK |
| Journal | Entrée du 2026-11-03 sur `main` (`d69aa0f`) : elle donne le compte réel (9 / 4 : 5 nouvelles, 4 modifiées) et dit que l'annonce précédente (« environ 10, dont 4 modifiées ») était fausse, avec 15 lignes. Le décalage de la paire y est annoncé. | **Corrigé** |
| Banc d'essai `menu.cpp` (E8) | Le point d'appui de « Menu » suit la nouvelle position (`W/2 - bw/2 + 1,05 bw`). | OK |

## Captures

| Capture | Constat |
|---|---|
| `fin-menu` (changée, voulu) | « REJOUER » (biseau orange) **à sa place de `main`**, de x = 496 à 784, milieu 640. « MENU » (biseau `#1E2130`, texte clair) à droite, de x = 799 à 1000 : même hauteur, même biseau, 0,7 fois la largeur. Image nette, rien de cassé. La paire va de 496 à 1000 : son milieu est à **748**, soit **108 px à droite** du centre du panneau (208 à 1072). Il reste 288 px de vide à gauche contre 72 à droite. Voir **ANO-14**. |
| `course-fin` (changée, voulu) | Comparée à `main` (`apercu-waforspeed-55`), seule la zone x = 799 à 1000, y = 504 à 576 change : c'est le bouton « Menu » ajouté. « Rejouer » et le reste de l'écran sont **identiques au pixel**, ce qui respecte RG-06 (« Rejouer » inchangé). Même décalage de la paire que `fin-menu`. |
| `fin-retour-menu` | **Identique au pixel au tour 1** : menu affiché après l'appui sur « Menu », record **390** (le score de `fin-menu`), aucune commande de conduite. L'appui sur la nouvelle position de « Menu » fonctionne. |
| `menu-appui`, `menu-depart-3`, `menu-depart-go`, `menu-bientot` | **Identiques au pixel au tour 1** (voir le tour 1 ci-dessous). |
| Les 55 autres captures | **Identiques au pixel au tour 1.** Comparées à `main` (`apercu-waforspeed-55`), les seuls écarts sont ceux voulus et déjà acceptés au tour 1 : la ligne « Duel en ligne » grisée de `menu-0`, `menu-1`, `menu-anim` et `ecran-menu-vide` (RG-04). Les vitrines, `course-cam*`, `course-decompte-*`, `hud-*`, `toast-*`, `trafic-*`, `ombres-*`, `chargement-*`, `carte-*`, `encoche-course` et `texte-essai-*` sont identiques au pixel. Aucune régression. |
| Journal du banc | `mode exact`, `plage`, `campagne`, `circuit`, `course automatique` : **0 écart(s)**. La ligne d'annulation du glissé (RG-01) est présente, ainsi que les lignes `capture fin-menu : écran course` et `capture fin-retour-menu : écran menu`. Les seules `[ERREUR]` concernent `cars/golf.wfm` et viennent de la scène voulue `chargement-erreur`. |

## Critères d'acceptation et règles (tour 2/2)

| Critère / règle | Résultat |
|---|---|
| `menu-appui`, `menu-depart-3` / `menu-depart-go`, `menu-bientot` | OK (inchangés depuis le tour 1) |
| `fin-menu` (« Rejouer » et « Menu ») puis `fin-retour-menu` | OK |
| Aucune commande de conduite sur le menu ; autres captures inchangées ; banc `0 écart(s)` | OK |
| RG-01 à RG-05, RG-07, RG-08 | OK (voir le tour 1) |
| RG-06 : « Menu » `.btn--ghost` à droite de « Rejouer », même biseau et même hauteur, « Rejouer » inchangé, retour au menu avec record à jour | OK. Le centrage de la paire n'est pas demandé par RG-06 : il était seulement constaté au tour 1. Le décalage est donc une finition (ANO-14), pas un critère manqué. |
| Accroche `course_system.cpp` ≤ 10 lignes, annoncée au journal | OK (9 lignes, journal exact) |

## Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-01 | ~~Mineure~~ **Corrigée** | — (code : `src/game/course_system.cpp`) | Accroche à 15 lignes (tour 1). Elle est passée à 9 lignes et le journal est corrigé. |
| ANO-14 | Mineure | `fin-menu`, `course-fin` | La paire « Rejouer » / « Menu » est décentrée sur l'écran de fin. « Rejouer » reste centré seul, et « Menu » déborde à droite : le milieu de la paire est à x = 748 au lieu de 640 (+108 px), avec 288 px de marge à gauche contre 72 à droite. Tout le reste du panneau est centré. Cela ne gêne pas le joueur : les deux boutons sont lisibles et fonctionnent. **À faire** (ticket E3, sans limite d'accroche) : centrer la paire dans le panneau, dessin et zones de toucher. |

ANO-14 reste après le tour 2/2. Elle est inscrite au tableau des anomalies (`anno.md`, statut 🆕 Nouvelle). Le PO la priorise et
crée l'US de correction.

---

# Tour 1/2 (pour mémoire) : verdict À CORRIGER, commit `e7e73a0`

### Captures

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

### Critères d'acceptation et règles

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

### Choix du développeur

1. **Libellé « Bientôt » de « Duel en ligne » à 80 % d'opacité** (le reste de la ligne à 45 %) : accepté, pour qu'il reste lisible.
   Le jeu web n'a pas ce libellé, il vient de RG-04.
2. **Bouton « Menu » à 0,7 fois la largeur de « Rejouer »** : accepté. RG-06 impose la même hauteur et le même biseau, pas la même largeur.
3. **Course figée et écran de fin masqué quand on est sur le menu** (`collide`, `drawUi` de `course_system.cpp`) : accepté.
   C'est ce qui empêche une partie de tourner derrière le menu (RG-05) ; mais ces lignes comptent dans l'accroche (ANO-01).

### Hors ticket (déjà présent avant WFS-38)

- Le tiret « — » du record vide s'affiche « ? » sur le menu (`menu-0`, `menu-appui`, `menu-bientot` : « RECORD ? »).
  Le glyphe manque dans la police ; le défaut vient de WFS-37 et doit être noté à sa recette (`qa/Check.md`). Il n'est pas compté ici.

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-01 | Mineure | — (code : `src/game/course_system.cpp`, E3) | L'accroche dépasse la limite de 10 lignes de `regles-de-gestion/WFS-38.md` : 15 lignes touchées (7 nouvelles, 8 modifiées), et le journal en annonce environ 10, dont 4 modifiées. Pas d'effet pour le joueur, et E3 n'a aucun ticket en cours. **À faire** : ramener l'accroche à 10 lignes au plus, par exemple en regroupant les gardes `g.ecran` ; sinon, corriger l'annonce au journal avec le compte réel et obtenir l'accord du PO pour ce dépassement. |

Tour suivant (2/2) : je regarde de nouveau le diff de `course_system.cpp`, le journal et les captures de la nouvelle Release
`apercu-waforspeed-38`. Toutes les captures doivent rester celles de ce passage.
