# QA WFS-58 : scénario simulateur `mvp` piloté par l'état du jeu (SIM-MVP-2, 3, 4)

## Tour 2 (2026-11-03 21:00 UTC)

**Verdict : REFUSÉ** (émulateur Android), 2e passage. **Fusion interdite.** 1 anomalie bloquante (nouvelle, WFS58-5). Les 2 bloquantes et les 2 mineures du tour 1 sont corrigées.

- Constructions regardées (PR #32, `feature/waforspeed-58`) :
  - `scenario=mvp`, run 41714023261, commit `a51bbc8`, Release `simulateur-WFS-58` (20:49:59Z) : `qa-01` à `qa-07` et les fichiers texte. Essai parti avant la fusion de WFS-59 (0,2 à 0,3 image/s).
  - `scenario=mvp`, run 41716390682, commit `3d447b1` (avec WFS-59), Release `simulateur-WFS-58-avec-59` (20:58:26Z) : `qa-01`, `qa-02` et les fichiers texte.
  - `scenario=conduite`, run 41711636444, commit `ffb5474`, Release `simulateur-WFS-58-conduite` (20:25:05Z) : 7 captures.
- Code : `tools/qa/emulateur.sh` et `timeout-minutes` de `simulateur.yml`. Toujours rien dans `src/` ni `android/`.
- WFS-58 est jugé sur son rôle : attendre l'état du jeu, échouer franchement, tracer. Le gel du jeu dans l'essai avec WFS-59 n'est pas compté contre ce ticket (voir « Hors périmètre »).

### Captures

| Capture | Constat |
|---|---|
| `qa-01-chargement` (a51bbc8) | **Écran presque noir** : seul le rail vide de la barre de chargement est visible. Il n'y a ni texte, ni pourcentage, ni titre. La capture est prise 2 s après « Cibles de rendu » (20:39:11.776), alors que l'écran complet, police comprise, ne paraît qu'à 20:39:14.791 (« chargement : première image … écran complet (police) 3,045 s »). Au tour 1, avec 3 s d'attente, la capture montrait « Chargement du modèle 3D… 20 % ». Pas de barre système, pas de bulle. |
| `qa-02-menu` | Menu WFS-37 complet (Course solo, Duel en ligne « bientôt », Showroom, record, Golf 8 GTI) sur la ville, voiture visible. **Plus de bulle de compatibilité** (WFS58-1 corrigée). |
| `qa-03-appui-course-solo` | Menu, prise dès « menu : « Course solo » » : c'est attendu, l'appui vient d'être reçu. |
| `qa-04-decompte` | Encore le menu, alors que la capture est prise après 2 lignes `QA ips=`. À 0,2 image/s, l'écran de course n'est pas encore présenté. Cela vient de la vitesse du jeu (au tour 1, à environ 1 image/s, on voyait le décompte « 3 ») : ce n'est pas compté, mais il faudra le revoir quand le jeu tournera plus vite. |
| `qa-05-go` | Course, « GO », TEMPS 30,0, 1 km/h, HUD complet, voiture visible. Bonne étape. |
| `qa-06-course-hud` | Course, TEMPS 29,9, 2 km/h, HUD complet. Bonne étape. |
| `qa-07-fin` | Preuve d'échec : course à 25,1 s, 0 km/h. Cohérent avec `ÉCHEC étape fin (délai 300 s)`. |
| `qa-01` / `qa-02` (3d447b1, avec WFS-59) | Écran noir puis écran de chargement à 100 %, tous deux couverts par la fenêtre Android « Pixel Launcher isn't responding ». Le jeu est figé (plus aucune ligne de journal après 20:55:25). Le scénario conclut `ÉCHEC étape menu (délai 180 s)`, prend la capture de preuve et écrit `RÉSULTAT TECHNIQUE : ÉCHEC`, ce qui est le comportement voulu. |
| `conduite` `qa-01` à `qa-07` (ffb5474) | `01` : écran de chargement à 70 %, sous la fenêtre « Pixel Launcher isn't responding » et l'avis « Viewing full screen ». `02` à `07` : **même image** (md5 identique, chargement à 100 % sous les mêmes fenêtres). Le scénario affiche pourtant `RÉSULTAT TECHNIQUE : OK`. Le rapport WFS-56 (`simulateur-WFS-56-conduite`) montrait déjà la même chose : `04` à `07` identiques, chargement à 100 % sous « Viewing full screen ». Voir « Hors périmètre ». |

### Suivi des anomalies du tour 1

| Anomalie | Résultat au tour 2 |
|---|---|
| WFS58-1 (bulle de compatibilité, bloquante) | **Corrigée** : plus de `wm size`, écran 1080x2400 (2400x1080 en paysage), aucune bulle sur `qa-01` à `qa-07`. RG-05 (écran réduit) est abandonnée, comme le tour 1 le permettait. |
| WFS58-2 (déroulement sans événement ni heure, `sleep 3`, bloquante) | **Corrigée** : chaque étape indique le motif attendu, puis le numéro de ligne, l'heure de l'appareil et le texte de la ligne du journal trouvée. L'attente fixe passe à 2 s. Mais ce raccourcissement crée WFS58-5. |
| WFS58-3 (03 et 04 identiques, mineure) | **Corrigée** dans le script : `03` est prise à l'appui, `04` après des images. Le contenu de `04` reste à revoir quand le jeu sera plus rapide (voir plus haut). |
| WFS58-4 (retour d'écran non tracé, mineure) | **Corrigée** : `écran en fin de scénario : Physical size: 1080x2400`. |

### Critères d'acceptation

| Critère | Résultat |
|---|---|
| CA 1 : `deroulement.txt` avec l'événement attendu, l'heure et la taille d'écran ; aucune attente fixe > 2 s | OK |
| CA 2 : `01-chargement` montre la barre du jeu sans barres système ; `02-menu` montre le menu | **KO pour `01`** (écran presque noir, WFS58-5). OK pour `02`. |
| CA 3 : échec franc, jamais de faux OK pour `mvp` | OK, sur les deux essais (`ÉCHEC étape fin`, puis `ÉCHEC étape menu`, avec `RÉSULTAT TECHNIQUE : ÉCHEC`). |
| CA 4 : `scenario=conduite` donne les mêmes 7 captures qu'avant | OK au sens du critère : 7 captures et script inchangé. Elles étaient déjà inexploitables avant ce ticket (hors périmètre). |
| RG-07 : durée | OK (essai a51bbc8 : 11 min environ). |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| WFS58-5 | **Bloquante** | `qa-01-chargement` (a51bbc8 et 3d447b1) | Critère 2 non rempli : la capture est presque noire. On ne voit que le rail vide de la barre, sans texte ni pourcentage, et ce n'est pas une preuve croyable de l'écran de chargement du jeu. Il s'agit d'une régression par rapport au tour 1. La cause : l'attente fixe de 2 s après « Cibles de rendu » tombe avant l'affichage de la police (environ 3 s). À faire : attendre la ligne `chargement : première image … écran complet (police)` (ou `chargement : Préparation du moteur`, écrite à la même heure), puis 2 s au plus, comme suggéré au tour 1. |

### Hors périmètre (signalé au PO, non compté contre WFS-58)

- **Gel du jeu après le chargement, avec la fenêtre « Pixel Launcher isn't responding »** : il apparaît dans l'essai avec WFS-59 (3d447b1) **et aussi dans l'essai `conduite` à ffb5474, donc sans WFS-59** (journal arrêté sur la recréation de swapchain de 20:24:35, captures figées). Le rapport WFS-56 montrait déjà des captures `conduite` figées sur le chargement. Imputer ce gel à WFS-59 n'est donc pas prouvé : la piste d'un émulateur saturé, ou d'une fenêtre système qui prend le focus au lancement, est à étudier dans le ticket de correction que crée le PO. Le scénario `mvp` de WFS-58 le détecte correctement.
- **Scénario `conduite` : faux OK** sur des captures figées (`02` à `07` identiques) : il n'a ni contrôle FIGÉE ni attente d'état. Il est resté inchangé à dessein (RG-06), mais c'est à reprendre dans un ticket E7.
- L'avis « Viewing full screen » peut être neutralisé avant le lancement (`settings put secure immersive_mode_confirmations confirmed`) : c'est une piste pour ce même ticket.

### Pour le tour suivant

- Corriger WFS58-5, puis relancer `simulateur.yml` avec `scenario=mvp` et rappeler le QA. Il suffit que `01-chargement` montre le texte et la barre du chargement. L'échec franc d'une étape suivante (gel ou lenteur du jeu) reste accepté pour WFS-58.

---

## Tour 1


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
