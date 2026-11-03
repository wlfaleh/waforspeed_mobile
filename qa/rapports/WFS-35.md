# QA WFS-35 : Écran de chargement de l'application (logo, barre de progression, fondu)

**Verdict : À CORRIGER, tour 1/2** (banc d'essai PC), 1er passage. Anomalies **mineures** seulement : pas encore de fusion.

- Construction regardée : commit `cfd7088` (branche `feature/waforspeed-35`, synchronisée avec `main` 412c4ce), PR #12.
  Release `apercu-waforspeed-35` (cible `cfd70888…`, texte « Branche feature/waforspeed-35, commit cfd7088 »,
  captures du 2026-11-03 09:01 UTC). Run Actions 41605344614 : réussi (banc-essai, android, publication).
- Captures en 2400x1080 : le banc les fait (3e lancement, `WFS_TAILLE=2400x1080`, dossier `texte/`), mais la CI ne les
  copie ni dans l'artefact `captures` ni dans la Release. **Le QA a construit le banc de `cfd7088` lui-même** (Linux,
  lavapipe, comme la CI) et l'a lancé avec `WFS_TAILLE=2400x1080` pour les regarder (ANO-09).

## Critères d'acceptation

| Critère | Résultat |
|---|---|
| `chargement-0/1/2` : fond `#0A0B10`, sur-titre cyan, logo « WAFOR / SPEED », barre cyan → orange à 0 %, 20-70 %, 100 %, étape + « n % » | **OK.** Couleurs mesurées au pixel : fond (10, 11, 16) = `#0A0B10`, sur-titre `#00E5FF`, « WA » `#F2F4F8`, « FOR » en contour blanc de 2 px, « SPEED » `#FF6A00` avec halo orange léger, fond de barre `#1E2130`, libellé `#8A90A6`. Barre : vide à 0 %, 20 % (cyan → orange sur la partie remplie, comme `.cp-bar i` du web), pleine à 100 % (cyan à gauche, orange à droite). Libellés « Préparation du moteur 0 % », « Chargement du modèle 3D… 20 % » (début de l'étape, limite notée au Journal : pas d'avancement intermédiaire du chargeur du noyau), « Construction de la ville 100 % ». Accents corrects. Mise en page à 1280x720 : bloc à x = 93 px (6 % de 1280 + 16), barre de 600 px, bloc centré verticalement (203 → 517 px), logo de 140 px et interligne 0,82. Lisibilité de « 3D » : ANO-07. |
| Mêmes captures en 2400x1080 : bloc à gauche, lisible, rien de coupé ni de chevauché | **OK, vérifié par le QA sur sa propre construction** (non publié par la CI : ANO-09). Bloc à gauche (x = 168 px), barre de 900 px, logo à l'échelle, rien de coupé ni de chevauché, message d'échec lisible. |
| `chargement-fondu` : la ville se voit à travers l'écran à moitié transparent | **OK sur le fond** : Golf, route, immeubles, arbres et commandes tactiles visibles sous l'écran de chargement à 50 % (journal : « écran de chargement à 50 % d'opacité »). Défauts de superposition pendant le fondu : ANO-06. |
| `chargement-erreur` : message rouge, pas de plantage | **OK.** « Le moteur 3D n'a pas pu démarrer sur cet appareil. » en `#FF1F4B`, logo et barre affichés, le banc continue ensuite (vitrines, course…). Étape d'échec mal nommée au journal : ANO-08. |
| Journal : durée de chaque étape + total ; première image < 0,5 s après l'initialisation Vulkan | **OK.** Une ligne par étape et le total (1280x720 : 0,84 / 0,06 / 0,37 / 0,50 s, total 1,88 s ; 2400x1080 : total 2,26 s). Première image (fond + barre) : **0,071 s**, 0,046 s et 0,046 s après l'initialisation Vulkan (les 3 lancements de la CI), 0,092 s sur la construction du QA. |
| Autres captures inchangées ; CI verte | **OK.** Les 28 captures communes (`vitrine-*` x12, `course-cam0/1-*` x6, `trafic-*` x3, `ombres-*` x3, `ecran-menu-vide`, `texte-essai-1280/2400`, `moteur-courbe`) sont **identiques au pixel près** à la Release `apercu` de `main` (3278fa4). Run 41605344614 vert. |

## Captures regardées

- `chargement-0` : écran noir-bleu uni, sur-titre « SPRINT URBAIN · VRAIES VOITURES » cyan, logo sur deux lignes, barre vide, « Préparation du moteur 0 % ». Rien de déformé.
- `chargement-1` : idem, barre remplie à 20 % en dégradé, « Chargement du modèle 3D… 20 % » (le « 3D » se lit « 30 » : ANO-07).
- `chargement-2` : barre pleine, « Construction de la ville 100 % ».
- `chargement-fondu` : scène de la ville à travers l'écran à 50 % ; traits plus clairs dans la barre, « FOR » qui paraît plein (ANO-06).
- `chargement-erreur` : barre à 70 %, message rouge à la place du libellé.
- 2400x1080 (construction du QA) : `chargement-0/1/2`, `-fondu`, `-erreur` : même aspect, à l'échelle, bloc à gauche, rien de coupé.

## Anomalies

| ANO | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-06 | Mineure | `chargement-fondu` (1280x720 et 2400x1080) | Pendant le fondu, les couches de l'écran se voient en transparence. **Barre** : des traits verticaux plus clairs d'un pixel tous les 75 px environ (pixel (205, 489) : (45, 162, 166) contre (35, 122, 132) à côté). Les 16 tranches se chevauchent (`plein / N + 0.5f` dans `chargement.cpp`) et la zone commune est dessinée deux fois à 50 %. **Logo** : « FOR » paraît plein et gris, plus en contour (8 copies blanches + remplissage couleur du fond, tous à 50 %), et le halo de « SPEED » s'additionne. À corriger : tranches bord à bord sans chevauchement (bords arrondis au pixel, largeur = bord suivant − bord courant) ; pour le logo, ne pas dessiner le contour par superposition quand l'opacité est < 1 (par exemple « FOR » en `--muted`, l'adaptation permise par RG-03, pendant le fondu seulement), ou toute autre solution qui garde le contour. |
| ANO-07 | Mineure | `chargement-1` (1280x720) | « Chargement du modèle 3D… » se lit « Chargement du modèle **30**… » : à 16 px, le « D » de Rajdhani ressemble au « 0 » de « 20 % » juste à côté. À 2400x1080, il se lit bien. À corriger : libellé un peu plus grand à 720 p (RG-07 : « ≈ 2,2 % de la hauteur »), ou autre réglage du rendu qui sépare « D » et « 0 ». |
| ANO-08 | Mineure | `chargement-erreur` + journal du banc | L'étape de l'échec est mal nommée. Le journal dit `échec à l'étape « Construction de la ville »` et `Chargement du modèle 3D… 0,00 s` comme si le modèle avait été chargé, alors que la cause est `Fichier introuvable : cars/golf.wfm`. La barre s'arrête à 70 % au lieu de 20 %. `Game::init` ne rend `false` qu'après `systemsInit`. À corriger : arrêter le chargement dès que la voiture manque (ou garder l'étape où l'erreur s'est produite), pour que le journal (RG-10, WFS-20) et la barre montrent la bonne étape. |
| ANO-09 | Mineure | captures 2400x1080 (absentes) | La CI fait les captures `chargement-*` en 2400x1080 (`texte/`) mais ne les publie pas : le critère « 2400x1080 » n'est pas vérifiable sur la Release (le QA a dû construire le banc). À corriger : les copier dans `captures/` sous un autre nom (ex. `chargement-0-2400.png` … `chargement-erreur-2400.png`) dans `.github/workflows/build.yml` (accroche hors zone E8, à annoncer au Journal). |

Numéros ANO provisoires : ils ne sont inscrits au tableau des anomalies de `TACHES.md` que s'ils restent après le tour 2/2.

## Remarques (pas des anomalies)

- RG-01 : la première image (fond + barre) part à 0,05 à 0,09 s, mais **l'écran complet** (logo, texte) ne paraît qu'après
  la construction de la police : 0,773 s au premier lancement de la CI (0,26 à 0,63 s ensuite, lavapipe). Le critère porte
  sur la première image : il est rempli. À surveiller avec WFS-20 sur téléphone.
- Accroches : 13 lignes au lieu des 10 prévues, annoncées au Journal par le développeur (affaire du PO, pas de la QA).

## Suite

Le développeur corrige ANO-06 à ANO-09 (tour 1/2), la CI repasse, puis 2e passage QA sur les nouvelles captures.
Pas de fusion avant.
