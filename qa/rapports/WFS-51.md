# QA WFS-51 : point d'extension, marges de sécurité de l'écran (encoche), commandes tactiles

**Verdict : VALIDÉ** (banc d'essai PC et revue du code), 1er passage, aucune anomalie. **Fusion autorisée.**

- Construction regardée : commit `66c13bc` (`feature/waforspeed-51`, PR #20), run Actions 41629230983 réussi
  (`banc-essai`, `android`, `publication` au vert). Release `apercu-waforspeed-51` (« commit 66c13bc »), 42 captures et `WaForSpeed.apk`.
- Synchronisation `778085f` : n'ajoute que `qa/rapports/WFS-14.md` et de la documentation, aucun code. La branche se fusionne sur `main` sans conflit (`git merge-tree`).
- Règles : `regles-de-gestion/WFS-51.md` (RG-01 à RG-05).
- Code changé : `src/core/system.h`, `src/game/game.{h,cpp}` (E0), plus les accroches prévues par les règles et annoncées au
  Journal : `src/platform/android_main.cpp`, `JeuActivity.java` et `src/platform/desktop_main.cpp` (banc, `WFS_ENCOCHE`).

## Captures

| Capture | Constat |
|---|---|
| `encoche-course` (2400x1080, nouvelle) | Bande rouge translucide de 90 px à gauche, sur toute la hauteur. Volant au repos centré à x ≈ 522 (0,18 W + 90), rayon ≈ 97 px : bord gauche à x ≈ 425, bien à droite de la bande. Jauges vitesse et nitro décalées à x ≈ 122 (3u + 90), hors de la bande. À droite, gaz (x ≈ 2136), frein (x ≈ 1680) et nitro (x ≈ 2160) aux mêmes positions qu'avant (0,89 W, 0,70 W, 0,90 W) : inchangés, aucune marge à droite. Golf visible, décompte « 2 » lisible, image non déformée, pas d'écran noir. |
| Les 41 autres captures | Comparaison pixel par pixel avec `apercu-waforspeed-29` (commit `3a87933`, dont le code est identique à la base `e7d68bc` de cette branche, seuls des `.md` diffèrent) : mêmes tailles, 12 captures identiques au pixel, les autres avec un écart **maximal de 1/255** par canal dans le ciel (bruit de rendu d'un lancement à l'autre), invisible. Aucun changement sans `WFS_ENCOCHE`. |
| Tests du banc | `mode exact : 39 voitures comparées au jeu web, 0 écart(s)` ; `course automatique : 22 lignes comparées au jeu web, 0 écart(s)` ; journal : `capture encoche-course : marge gauche 90 px, écran 2400x1080`. |

## Critères d'acceptation et règles

| Critère / règle | Constat | Résultat |
|---|---|---|
| `encoche-course` : bande de 90 px marquée, volant et gaz à droite de la bande | voir Captures | OK |
| Autres captures identiques à `main`, banc `0 écart(s)` | voir Captures | OK |
| RG-01 `GameView::marges` (gauche, haut, droite, bas), 0 par défaut | `struct Marges` dans `src/core/system.h`, initialisée à 0 ; copiée dans `view_` à `systemsInit` (après `view_ = GameView{}`) et à chaque `setMarges` | OK |
| RG-02 lecture Android et JNI | `JeuActivity` : `OnApplyWindowInsetsListener` sur la vue de décor, `getDisplayCutout()` et `getSafeInset*` seulement si `SDK_INT >= P`, 0 sinon ; appelé à chaque envoi d'insets (rotation, barres), `requestApplyInsets()` relancé dans `pleinEcran()`. Une seule fonction JNI `nativeMarges` (statique, `jclass`, nom `Java_com_waforspeed_game_JeuActivity_nativeMarges` conforme au paquet). Côté natif : valeurs en `std::atomic` écrites par le thread de l'interface, lues dans la boucle avant `update` (drapeau `exchange`) : pas de course entre threads ; des marges reçues avant que la fenêtre soit prête sont gardées et appliquées à la première image. Unités cohérentes : insets en pixels de la fenêtre, interface en pixels de la fenêtre (`setViewSize`). CI `android` verte (Java compilé). | OK (revue du code) |
| RG-03 commandes et zones de toucher décalées | Volant + `mg`, pédales et nitro − `md`, tout le monde − `mb` en bas, jauges + `mg` / + `mh` ; zones de toucher décalées des mêmes valeurs (`mg + 0,45 W`, `0,80 W − md`…), sans chevauchement entre zones avec 90 px | OK |
| RG-04 `WFS_ENCOCHE` au banc, sans variable rien ne change | Variable lue dans `desktop_main.cpp` (encoche à gauche) ; la CI ne la pose pas : captures inchangées | OK |
| RG-05 aucun autre changement d'API | seuls `GameView::marges`, `Game::setMarges/marges/montrerMarges` ajoutés | OK |

## Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — | — | — | aucune |

## Remarques (sans gravité, pas d'anomalie)
- La capture `encoche-course` est prise avec une marge de 90 px fixée dans le code du banc ; `WFS_ENCOCHE` sert au lancement manuel. Conforme au critère.
- Volant en cours d'utilisation : son cercle est centré sous le doigt (`steerX0_`), il n'est pas borné à la marge. Le doigt ne peut pas toucher l'encoche elle-même, donc sans effet pour le joueur ; à garder en tête pour WFS-52.
- À vérifier à l'écran au simulateur (WFS-50, fin d'épic) : valeurs réelles des insets dans logcat (« Marges de l'écran : … »), changement de côté en paysage inversé.
