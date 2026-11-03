# QA WFS-58 : scénario simulateur `mvp` piloté par l'état du jeu (SIM-MVP-2, 3, 4)

**Verdict : REFUSÉ** (émulateur Android), 1er passage. **Fusion interdite.** 2 anomalies bloquantes, 2 mineures.

- Construction regardée : commit `22cd5fd` (`feature/waforspeed-58`, PR #32). Run `simulateur.yml` 41709020767
  (`epic=WFS-58`, `scenario=mvp`, essai 3), Release `simulateur-WFS-58` publiée le 2026-11-03 à 20:14 UTC (« commit 22cd5fd ») :
  `qa-01` à `qa-07`, `qa-deroulement.txt`, `qa-journal-jeu.txt`, `qa-logcat.txt`, `qa-ecran.txt`.
- Règles : `regles-de-gestion/WFS-58.md` (RG-01 à RG-07).
- Code changé : `tools/qa/emulateur.sh` (+87 / −22) et `timeout-minutes: 35` de l'étape émulateur dans
  `.github/workflows/simulateur.yml` (job à 45 min). Aucun changement de `src/` ni `android/`.
- L'échec à l'étape « fin » (délai 300 s) est le résultat attendu tant que WFS-59 n'est pas fusionné : il n'est **pas**
  compté comme anomalie.

## Captures (essai 3, 1200x540)

| Capture | Étape annoncée | Constat |
|---|---|---|
| `qa-01-chargement` | chargement | Écran de chargement du jeu, barre à 20 % « Chargement du modèle 3D… ». Pas de barre d'état ni de poignée de navigation (SIM-MVP-4 corrigée). **Mais la bulle Android « Tap to restart this app for a better view » et son bouton rond couvrent la moitié droite.** |
| `qa-02-menu` | menu | Menu WFS-37 (Course solo, Duel en ligne « bientôt », Showroom, record, Golf 8 GTI) sur la ville. Bonne étape. Même bulle Android à droite. |
| `qa-03-appui-course-solo` | appui « Course solo » | Écran de course, décompte « 3 », TEMPS 30,0, 0 km/h. L'appui a bien lancé la course. Même bulle, sur le bouton d'accélérateur vert et le cadre SCORE. |
| `qa-04-decompte` | décompte | Décompte « 3 », identique à l'œil à `qa-03` (md5 différents, donc pas noté FIGÉE). Acceptable : à 1 image/s les deux événements tombent dans la même image. Même bulle. |
| `qa-05-go` | GO | Course partie : TEMPS 29,3, 17 km/h, score 5. « GO » n'est pas journalisé : l'étape « voiture qui roule » est le bon substitut. Même bulle sur l'accélérateur. |
| `qa-06-course-hud` | course (HUD) | TEMPS 29,2, 19 km/h, score 10, HUD complet. Même bulle. |
| `qa-07-fin` | fin (preuve d'échec) | Capture de preuve après le délai : TEMPS 11,2, 0 km/h, voiture de trafic visible. Cohérent avec `ÉCHEC étape fin (délai 300 s)`. Même bulle. |

Toutes les images sont nettes, non déformées, voiture visible, ni écran noir ni couleurs fausses : seul l'ajout d'Android cache le jeu.

## Critères d'acceptation et règles

| Critère / règle | Constat | Résultat |
|---|---|---|
| RG-01 : attente des événements du journal | `attendre <nom> <motif> <délai>` lit `journal-jeu.txt` (logcat `-s WaForSpeed:I` en continu, borné par `timeout 2400`) après une marque. Motifs vérifiés dans `src/` : `Cibles de rendu` (renderer.cpp), `Écran : course → menu` (game.cpp, écran initial = course), `menu : « Course solo »` (menu.cpp), `course : nouvelle course` / `course : temps écoulé` (course_system.cpp), `QA ips=… vitesse=` (android_main.cpp). Plus aucun `sleep` de cadence. | OK |
| RG-02 : capture après images présentées, marque FIGÉE | `images()` attend 2 nouvelles lignes `QA ips=` (au plus 30 s, puis 2 s) ; `capture()` compare le md5 à la précédente et écrit `FIGÉE`. | OK |
| RG-03 : délais commentés, échec franc | Délais commentés (120/180/60/60/300/300/60/300/60 s). `ÉCHEC étape fin (délai 300 s)`, capture de preuve, arrêt, `ÉCHEC : étape « fin » non atteinte`, `RÉSULTAT TECHNIQUE : ÉCHEC`, code ≠ 0. Plus de faux OK. | OK |
| RG-04 : `01-chargement` après la 1re image, sans barres système | Capture 3 s après `Cibles de rendu` : écran du jeu, pas de barres système. Bulle Android présente (voir WFS58-1). | OK (barres) |
| RG-05 : écran réduit puis rétabli | `wm size 540x1200` (appareil portrait 1080x2400), noté au déroulement ; W/H lus après (`1200x540`) ; `wm size reset` après le parcours, y compris en échec. Mais la réduction fait apparaître la bulle de compatibilité d'Android (WFS58-1), et le rétablissement n'est pas tracé (WFS58-4). | KO (WFS58-1) |
| RG-06 : extra `qa_simu` | `am start … --ez qa_simu true` ; déroulement : « Intent { … (has extras) } ». | OK |
| RG-06 : scénario `conduite` inchangé | Relu : branche `else` identique à `main` (lancement, coordonnées, 7 captures). Pas d'essai `scenario=conduite` dans la Release. | OK à la lecture (essai demandé au prochain tour) |
| RG-07 : durée | Somme des délais 1 440 s + 9 × 30 s = 28,5 min ≤ 30 min ; étape limitée à 35 min, job à 45 min. Essai 3 : 11 min. | OK |
| CA 1 : `deroulement.txt` avec événement attendu, heure, taille d'écran ; aucune attente fixe > 2 s | Taille d'écran : oui. **Heure : absente** (seulement « après N s »). **Événement attendu : absent** (seul le nom de l'étape). **`sleep 3`** fixe avant `01-chargement`. | KO (WFS58-2) |
| CA 2 : `01-chargement` = barre du jeu sans barres système ; `02-menu` = menu | Oui, à la bulle près. | OK |
| CA 3 : échec propre tant que WFS-59 n'est pas fusionné | Oui. | OK |
| CA 4 : `scenario=conduite` : mêmes 7 captures | Code inchangé ; non rejoué. | À montrer |

## Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| WFS58-1 | **Bloquante** | `qa-01` à `qa-07` | La bulle Android « Tap to restart this app for a better view » et son bouton rond sont **encore sur les 7 captures** de l'essai 3 (annoncée corrigée après l'essai 2) : elle couvre la moitié droite de l'écran, le cadre SCORE et le bouton d'accélérateur. Elle vient de la réduction d'écran (RG-05, `wm size` : mode de compatibilité d'Android), elle n’apparaît pas dans le rapport `simulateur-MVP.md` (WFS-56, sans réduction d’écran). Les captures ne sont donc pas « croyables » pour la vérification WFS-50 (HUD et écran de fin masqués), et un appui sur cette bulle redémarre l'appli. À faire disparaître sans toucher à `src/` ni `android/` (pistes à essayer : appliquer `wm size` avant l'installation ou avant le 1er lancement, densité proportionnelle, désactiver l'interface de compatibilité), ou renoncer à la réduction d'écran si rien ne marche, et le prouver par un nouvel essai. |
| WFS58-2 | **Bloquante** | `qa-deroulement.txt` | Critère 1 non rempli : chaque étape doit montrer l'**événement attendu** et l'**heure** (ex. `20:07:02 étape menu : « Écran : course → menu » atteinte après 2 s`) ; aujourd'hui seuls le nom et la durée relative sont écrits, impossible de recouper avec `qa-journal-jeu.txt`. De plus `sleep 3` fixe avant `01-chargement` dépasse les 2 s permis : attendre plutôt une ligne `chargement : …` (ex. `chargement : Préparation du moteur`) puis 2 s au plus. |
| WFS58-3 | Mineure | `qa-03`, `qa-04` | `03-appui-course-solo` et `04-decompte` montrent la même image (décompte « 3 ») : les deux événements tombent dans la même image à 1 image/s. Sans gêne pour le jugement ; pourrait être signalé au déroulement (« même image que la précédente ») quand la différence md5 est infime. |
| WFS58-4 | Mineure | `qa-deroulement.txt`, `qa-ecran.txt` | `wm size reset` n'est pas tracé : `qa-ecran.txt` est pris **avant** le scénario, rien ne prouve le retour à 1080x2400. Ajouter une ligne `écran rétabli : …` (sortie de `wm size`) en fin de parcours. |

## Pour le tour suivant

- Corriger WFS58-1 et WFS58-2 (bloquantes) ; les mineures WFS58-3 et WFS58-4 peuvent être reprises au même tour.
- Relancer `simulateur.yml` sur la branche avec `scenario=mvp` **et** `scenario=conduite` (critère 4), puis rappeler le QA.
- L'échec à l'étape « fin » reste attendu tant que WFS-59 n'est pas fusionné.
