Agent : Claude, agent dev3d (01UY1Vv5Cs)

## Demande de l'utilisateur
Vidéo du 2026-11-12 (Corniche, Poursuite, vue Cockpit) : la bande « POLICE » de la voiture de police est un rectangle plat qui dépasse au-dessus de la roue avant, il n'y en a pas du tout côté droit (ANO-250), et la voiture passe du gris argent au blanc en 0,2 s (ANO-260). Ticket WFS-460 (P1), règles `regles-de-gestion/WFS-460.md`.

## Ce qui change
**ANO-250 : les bandes sont posées sur la vraie tôle, des deux côtés** (`src/game/police_bandes.h`, nouveau, calcul seul)
- Au chargement du modèle de la police, les triangles de sa peinture sont relus dans le `.wfm` (`vehicule::lireWfmOctets`, même lecture que `lireWfm`, depuis les octets de `readAsset` : marche aussi sur Android).
- Une grille de 28 × 5 points est projetée sur ces triangles, de l'extérieur vers l'axe de la voiture (comme un décalque collé à la main), puis posée 5 mm au-dessus de la tôle, le long de sa normale. La bande suit donc le galbe des portières et la rentrée de caisse.
- Elle va de 12 cm après le pneu avant à 12 cm avant le pneu arrière (passages de roue mesurés sur le modèle, `Model::roues`) : rien ne dépasse au-dessus des roues.
- Hauteur 18 cm (RG-02 : 15 à 20 cm), centrée à 0,44 × la hauteur de la voiture, soit 0,63 m sur la 208, au milieu des portières.
- Un point tombé dans le jour d'une porte (plus de 2,5 cm en retrait de ses voisins) est remplacé par l'interpolation de ses voisins.
- Si la tôle n'est pas trouvée, aucune bande n'est posée : jamais un morceau qui flotte.
- « POLICE » se lit à l'endroit des deux côtés. Les triangles sont enroulés vers l'extérieur.
- **Cause du côté droit absent** : l'ancien quadrilatère avait ses deux faces enroulées dans le même sens, donc la face droite était une face arrière et le rendu la retirait.
- Texture refaite en 960 × 96 (au lieu de 320 × 32) : lettres bâton à traits pleins de 13 cm au lieu de carrés de 5 × 7. Même bleu, même liseré blanc et rouge.
- Toujours **un seul dessin par voiture de police**, comme avant (RG-06), et 432 triangles. L'ancien quadrilatère n'existe plus en jeu : il n'est recréé que par le banc, pour l'image « avant ».

**ANO-260 : une seule peinture blanche, à toutes les distances et dans toutes les qualités.** Voici la cause exacte :
- Au chargement, `model_loader` classe « très fine » (WFS-42) toute pièce dont les triangles ont une hauteur médiane sous 4 mm. La peinture de la 208 est un maillage très dense (3,5 mm), elle est donc classée ainsi (journal : `cars/p208.wfm : matériau 7 très fin (3.5 mm) → préfiltré`).
- En qualité **Basse**, `Renderer::draw` ne dessine plus les pièces très fines au-delà de 15 m de la caméra (règle WFS-134 prévue pour les logos, grilles et fils). **Toute la carrosserie disparaissait** : on voyait à sa place les surfaces intérieures gris foncé (le lot « reste opaque », couleur 0,01), qui paraissent gris argent ou noires selon le reflet.
- La police est dessinée en « lots » au-delà de 15 m. Dans la vidéo, en vue Cockpit, elle roule tout près de cette distance : elle passe d'une image à l'autre de « sans carrosserie » (gris) à « carrosserie » (blanc).
- Correction : **accroche d'une ligne** dans `src/render/renderer.cpp`. La peinture n'est jamais retirée par cette règle, comme la règle suivante (pièces de moins de 30 cm) l'excluait déjà.
- `renderer.h` reçoit en plus un drapeau de banc (`bancPeintureFineMain`), qui ne sert qu'aux images « avant ».

## ⚠️ À regarder : effet hors de la police (même cause)
Quatre voitures ont une peinture classée « très fine » : **golf (3,3 mm), mustang (2,4 mm), 208 (3,5 mm) et 911 (2,6 mm)**. En Basse, au-delà de 15 m, celles du trafic perdaient aussi leur carrosserie. Avec cette correction, elles la retrouvent.
- Coût en Basse : environ 1 à 2 appels de dessin par image de plus (la carrosserie qui manquait).
- Je le signale au PO. Si le lead dev préfère limiter la correction à la police, il suffit de remettre `mat.detail = 0` sur la seule copie `modele_` de la police, mais le trafic resterait sans carrosserie.

## Comment c'est vérifié
- **Test CPU** `test_police_bandes` (nouveau, dans `CMakeLists.txt`) :
  - sur une caisse d'essai galbée avec un jour de porte : deux côtés, chaque point à 5 mm (± 1,5 mm) de la tôle, normales et enroulement vers l'extérieur, texte à l'endroit des deux côtés, hauteur de 15 à 20 cm, aucune bande sans tôle ni entre des passages de roue trop proches, tôle d'un seul côté → bande de ce côté seulement ;
  - sur la **vraie 208** (`p208.wfm`) : 1,67 m de bande, 0 point sans tôle, écart maximal à la tôle de 0,2 mm.
- **Banc d'essai** `WFS_BANC=police-bandes` (nouveau ; accroche d'une ligne dans `desktop_main.cpp`) : Corniche, joueur à 30 km/h. Chaque image est rendue deux fois dans la même scène : « -avant » (état de `main`) puis « après ».
- **Coût en Basse** (voir le journal du banc et le tableau ci-dessous), **bandes seules : +0 appel de dessin** par image.
- `bash tools/qa/tests_cpu.sh build` : voir le résultat dans le commentaire de la PR.

## Limites et risques
- Rendu vérifié sur le banc (lavapipe), pas sur un téléphone. Après la fusion, **🟡 À valider (utilisateur)**.
- Le `.wfm` de la police est relu une fois de plus à son chargement (6,7 Mo, ≈ 40 ms au départ de la poursuite, aucune allocation par image).
- Hors périmètre, laissés tels quels : gyrophares (cubes de la rampe), roues (WFS-452), conduite (WFS-461).
- Accroches hors zone (signalées au journal) : `renderer.cpp` (1 ligne), `renderer.h` (1 ligne, banc), `desktop_main.cpp` (1 ligne, banc), `vehicule_mesure.h` (lecture depuis la mémoire, sans changement de comportement).

## Coût (banc `police-bandes`, moyenne de 16 images par approche, même image rendue trois fois)
| Approche | Appels de dessin : main → bandes sur la tôle → branche | Triangles : main → branche | Rendu processeur (lavapipe) : main → branche |
|---|---|---|---|
| Cockpit (Haute) | 387,1 → 387,1 (+0) → 387,1 (+0) | 449 917 → 450 345 | 595,0 → 595,1 ms |
| Poursuite (Haute) | 384,6 → 384,6 (+0) → 384,6 (+0) | 499 112 → 499 540 | 542,1 → 529,3 ms |
| Poursuite **Basse** | 284,1 → 284,1 (+0) → 285,9 (+1,8 : carrosserie retrouvée) | 338 882 → 344 056 | 241,3 → 242,1 ms |
| Cockpit **Basse** | 281,1 → 281,1 (+0) → 282,9 (+1,8 : carrosserie retrouvée) | 266 458 → 272 539 | 251,4 → 253,4 ms |

RG-06 : les bandes posées sur la tôle ne coûtent **aucun appel de dessin de plus** (+428 triangles par voiture de police).

## Comment lire les captures
- Toutes viennent de la branche (`SANS_AVANT=1` : le banc n'existe pas sur `main`). **« …-avant » = état de `main`, rendu dans la même scène et la même image** : bandes plates de `main`, peinture très fine retirée en Basse. Le nom sans suffixe montre le résultat de la branche.
- `police-bandes-gauche-jour` / `police-bandes-droite-jour` : profils de jour. Sur `main`, le côté droit n'a **aucune bande**.
- `police-bandes-3-4-nuit` : 3/4 avant, de nuit.
- `police-bandes-approche-cockpit-basse-00` à `15` : la police, de 80 m à 5 m devant, une image toutes les 0,1 s, en vue Cockpit et en **Basse** (cas de la vidéo). Sur `main`, elle est **noire ou grise au-delà de 15 m, puis blanche** tout près (ANO-260). Après : blanche sur toutes les images.
- `…-approche-basse-10` à `15` : la même approche en vue Poursuite, en Basse.
- `…-approche-cockpit-12` à `15` et `…-approche-poursuite-12` à `15` : la même approche en qualité Haute (aucun changement de couleur sur `main` non plus).

## Captures pour le lead dev (66 après)

Dossier : [`qa/pr/WFS-460`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `police-bandes-3-4-nuit-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-3-4-nuit-avant.jpg?raw=true) |
| `police-bandes-3-4-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-3-4-nuit.jpg?raw=true) |
| `police-bandes-approche-basse-10-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-basse-10-avant.jpg?raw=true) |
| `police-bandes-approche-basse-10` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-basse-10.jpg?raw=true) |
| `police-bandes-approche-basse-11-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-basse-11-avant.jpg?raw=true) |
| `police-bandes-approche-basse-11` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-basse-11.jpg?raw=true) |
| `police-bandes-approche-basse-12-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-basse-12-avant.jpg?raw=true) |
| `police-bandes-approche-basse-12` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-basse-12.jpg?raw=true) |
| `police-bandes-approche-basse-13-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-basse-13-avant.jpg?raw=true) |
| `police-bandes-approche-basse-13` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-basse-13.jpg?raw=true) |
| `police-bandes-approche-basse-14-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-basse-14-avant.jpg?raw=true) |
| `police-bandes-approche-basse-14` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-basse-14.jpg?raw=true) |
| `police-bandes-approche-basse-15-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-basse-15-avant.jpg?raw=true) |
| `police-bandes-approche-basse-15` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-basse-15.jpg?raw=true) |
| `police-bandes-approche-cockpit-12-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-12-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-12` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-12.jpg?raw=true) |
| `police-bandes-approche-cockpit-13-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-13-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-13` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-13.jpg?raw=true) |
| `police-bandes-approche-cockpit-14-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-14-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-14` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-14.jpg?raw=true) |
| `police-bandes-approche-cockpit-15-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-15-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-15` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-15.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-00-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-00-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-00` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-00.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-01-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-01-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-01` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-01.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-02-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-02-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-02` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-02.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-03-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-03-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-03` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-03.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-04-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-04-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-04` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-04.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-05-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-05-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-05` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-05.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-06-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-06-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-06` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-06.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-07-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-07-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-07` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-07.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-08-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-08-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-08` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-08.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-09-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-09-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-09` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-09.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-10-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-10-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-10` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-10.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-11-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-11-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-11` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-11.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-12-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-12-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-12` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-12.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-13-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-13-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-13` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-13.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-14-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-14-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-14` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-14.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-15-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-15-avant.jpg?raw=true) |
| `police-bandes-approche-cockpit-basse-15` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-cockpit-basse-15.jpg?raw=true) |
| `police-bandes-approche-poursuite-12-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-poursuite-12-avant.jpg?raw=true) |
| `police-bandes-approche-poursuite-12` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-poursuite-12.jpg?raw=true) |
| `police-bandes-approche-poursuite-13-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-poursuite-13-avant.jpg?raw=true) |
| `police-bandes-approche-poursuite-13` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-poursuite-13.jpg?raw=true) |
| `police-bandes-approche-poursuite-14-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-poursuite-14-avant.jpg?raw=true) |
| `police-bandes-approche-poursuite-14` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-poursuite-14.jpg?raw=true) |
| `police-bandes-approche-poursuite-15-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-poursuite-15-avant.jpg?raw=true) |
| `police-bandes-approche-poursuite-15` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-approche-poursuite-15.jpg?raw=true) |
| `police-bandes-droite-jour-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-droite-jour-avant.jpg?raw=true) |
| `police-bandes-droite-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-droite-jour.jpg?raw=true) |
| `police-bandes-gauche-jour-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-gauche-jour-avant.jpg?raw=true) |
| `police-bandes-gauche-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-460/qa/pr/WFS-460/apres-police-bandes-gauche-jour.jpg?raw=true) |
