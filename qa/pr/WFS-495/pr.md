## Agent : Claude, agent dev3d (01Us6uCv3C)

**Ticket** : WFS-495 — règles : `regles-de-gestion/WFS-495.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : poser des autocollants comme de vrais adhésifs, qui suivent les courbes de la carrosserie, sans bande plate qui flotte (ANO-250), 20 calques au plus cuits en une seule texture par voiture (épic E18, WFS-495).
- **Ce qui change** :
  - **Aucune géométrie ajoutée** (règle « pas de forme nue ») : chaque autocollant est **cuit dans la texture d'habillage** de WFS-492, celle que le shader lit par projection sur les **pièces peintes seulement**. Il épouse donc chaque bosse, garde sa taille réelle et s'arrête où la peinture s'arrête : rien sur les vitres, les phares, la calandre, les jantes ni les pneus (RG-01). Il passe sous la finition et le vernis, et reçoit la même lumière que la peinture de nuit (RG-04).
  - `src/render/vehicule_autocollants.*` (nouveau) :
    - calques (RG-02) : 20 au plus, chacun avec motif, direction (gauche, droite, capot, toit, coffre, avant, arrière), position, rotation, échelle de 10 à 300 %, couleur parmi les 28 de `PEINTURES` (WFS-487) et miroir ;
    - **texte court** pour la sauvegarde (RG-07) : `motif,direction,x,y,rotation,échelle,couleur,miroir;…`, lecture bornée, calque illisible ignoré ;
    - **bibliothèque de 50 motifs vectoriels originaux** (RG-06) : 5 bandes, chiffres 0-9, 3 flammes, 3 éclairs, 3 étoiles, lettres A-Z en Rajdhani Bold (SIL OFL, déjà dans `assets/fonts`). Aucun logo ni sponsor réel ;
    - **rastérisation à couverture exacte**, pour des bords nets sans halo à toute échelle ;
    - **cuisson** : skin éventuelle (RG-05, autocollants par-dessus) + calques, puis RGBA 1024², mipmaps et compression **ETC2 RGBA8**.
  - `src/render/vehicule_etc2.*` (nouveau) : encodeur et décodeur ETC2 RGBA8 (modes individuel et différentiel, alpha EAC). Chemin rapide pour les blocs unis.
  - `vehicule_skin.*` : `poser(…, calques)`. Sans calque, c'est exactement l'ancien `poser` (skin seule). Avec des calques, on obtient **une seule texture** (RC-E18-02), numéro de skin 5 = « cuit ». Le GPU sans ETC2 (lavapipe) reçoit les niveaux décodés sur le processeur, donc la même image que le téléphone. `couleurs()` est une copie C++ de `SKIN_COULEURS`.
  - `shaders/vehicule.glsl` : `habillageCuit` (mêmes projections et mêmes poids que `habillage`, lecture RGBA prémultipliée). Le chemin des skins est inchangé.
  - Accroches annoncées au journal : `game.cpp` (1 ligne : variable `WFS_AUTOCOLLANTS` des bancs ; l'atelier WFS-496 y branchera la sauvegarde), `desktop_main.cpp` (banc `autocollants`), `CMakeLists.txt` (test).
- **Comment c'est vérifié** :
  - `test_vehicule_autocollants` (nouveau) : blocs ETC2 unis, PSNR après compression 45,8 dB (couleur) et 60 dB (alpha) ; aller-retour du texte, bornes et rejets (21 calques donnent 20 gardés) ; 50 motifs dessinés dans leur taille ; bande du capot à la bonne place et de la bonne couleur, rien sur les faces ; miroir symétrique, lettres jamais retournées ; skin seule reproduite ; couleurs C++ égales à celles du shader.
  - **141 tests CPU réussis** (`qa/pr/WFS-495/tests-cpu.log`).
  - Banc `WFS_BANC=autocollants` sur la 208, la GT86 et la Corvette C7 : double bande capot-toit-coffre, flamme de l'aile avant coupée par le passage de roue, numéro « 27 » sur les deux portières, étoiles sur le capot. Jour, nuit, course, gros plans roue / porte / dessus, et version sur la skin Rallye.
  - **Cuisson : 48 à 76 ms** sur PC (4 cœurs) ; le S21 FE, à 8 cœurs, est sous 1 s avec une large marge (RG-03).
  - En course : 1 texture d'habillage en mémoire et rien de cuit.
  - **Sans autocollant : captures `skins` identiques au pixel près à `main`** (7 images, écart max 0) ; le banc confirme le chemin « skin seule », sans cuisson.
  - **Qualité Basse** (`qa/pr/WFS-495/mesure-basse.txt`, 3 + 3 passages) : 3,891 img/s sans autocollant, 3,843 img/s avec, soit **0,05 img/s d'écart** ; 1 % le plus lent : 2,715 contre 2,860 img/s.
- **Limites et risques connus** :
  1. L'écran de l'atelier et la clé de sauvegarde des calques ne sont pas dans ce ticket (WFS-496 et WFS-497). La clé `creation-<voiture>` actuelle n'accepte qu'un identifiant de 32 caractères. Aujourd'hui, les calques arrivent par `WFS_AUTOCOLLANTS` (bancs) ; l'atelier appellera `vehicule_skin::poser(…, texte)` et rangera le texte dans la sauvegarde.
  2. Le **miroir** garde chaque lettre et chaque chiffre lisible, mais inverse l'ordre d'un mot (« 27 » devient « 72 » de l'autre côté). Un numéro des deux côtés se pose avec un calque de chaque côté, comme dans le banc. L'atelier pourra le proposer.
  3. Un adhésif continue sur la jonction de deux panneaux (porte / aile) au lieu d'y être coupé au cutter : c'est la même projection continue que les skins (RG-04 de WFS-492).
  4. **Conflit prévisible avec WFS-493** (même bloc `SKIN_COULEURS` du shader) : la PR fusionnée en second garde les deux. Si WFS-493 passe après, `test_vehicule_autocollants` lui demandera de recopier ses couleurs dans `vehicule_skin::couleurs`. La **lueur de nuit de la skin Néon** (WFS-493) ne passe pas dans la texture cuite : sous des autocollants, le Néon ne brillerait plus. Signalé au PO.
  5. Le temps de cuisson sur le S21 FE n'est pas mesuré sur l'appareil (pas de téléphone ici), seulement estimé d'après le PC. Le journal du jeu l'écrit à chaque cuisson.

## Tests CPU

Commit testé : **`b39bc7e0`** (branche synchronisée avec `main`), 2026-11-14 15:06:52 UTC, `bash tools/qa/tests_cpu.sh build`. Journal complet : [`qa/pr/WFS-495/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/tests-cpu.log).

```
[OK] test_vehicule_autocollants (0 s)
[OK] test_vehicule_skin (0 s)
[OK] test_ville_sol (3 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 141 test(s) réussi(s) en 338 s
```

## Captures pour le lead dev (31 après)

Dossier : [`qa/pr/WFS-495`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `autocollants-bibliotheque` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-bibliotheque.jpg?raw=true) |
| `autocollants-gt86-course-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-gt86-course-jour.jpg?raw=true) |
| `autocollants-gt86-course-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-gt86-course-nuit.jpg?raw=true) |
| `autocollants-gt86-dessus` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-gt86-dessus.jpg?raw=true) |
| `autocollants-gt86-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-gt86-jour.jpg?raw=true) |
| `autocollants-gt86-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-gt86-nuit.jpg?raw=true) |
| `autocollants-gt86-porte` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-gt86-porte.jpg?raw=true) |
| `autocollants-gt86-roue` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-gt86-roue.jpg?raw=true) |
| `autocollants-mustang-course-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-mustang-course-jour.jpg?raw=true) |
| `autocollants-mustang-course-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-mustang-course-nuit.jpg?raw=true) |
| `autocollants-mustang-dessus` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-mustang-dessus.jpg?raw=true) |
| `autocollants-mustang-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-mustang-jour.jpg?raw=true) |
| `autocollants-mustang-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-mustang-nuit.jpg?raw=true) |
| `autocollants-mustang-porte` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-mustang-porte.jpg?raw=true) |
| `autocollants-mustang-roue` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-mustang-roue.jpg?raw=true) |
| `autocollants-p208-course-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-p208-course-jour.jpg?raw=true) |
| `autocollants-p208-course-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-p208-course-nuit.jpg?raw=true) |
| `autocollants-p208-dessus` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-p208-dessus.jpg?raw=true) |
| `autocollants-p208-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-p208-jour.jpg?raw=true) |
| `autocollants-p208-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-p208-nuit.jpg?raw=true) |
| `autocollants-p208-porte` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-p208-porte.jpg?raw=true) |
| `autocollants-p208-roue` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-p208-roue.jpg?raw=true) |
| `autocollants-p208-sans` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-p208-sans.jpg?raw=true) |
| `autocollants-p208-skin` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-autocollants-p208-skin.jpg?raw=true) |
| `skins-gt86-course-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/avant-skins-gt86-course-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-skins-gt86-course-jour.jpg?raw=true) |
| `skins-gt86-course-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/avant-skins-gt86-course-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-skins-gt86-course-nuit.jpg?raw=true) |
| `skins-gt86-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/avant-skins-gt86-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-skins-gt86-jour.jpg?raw=true) |
| `skins-gt86-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/avant-skins-gt86-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-skins-gt86-nuit.jpg?raw=true) |
| `skins-mustang-droit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/avant-skins-mustang-droit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-skins-mustang-droit.jpg?raw=true) |
| `skins-mustang-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/avant-skins-mustang-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-skins-mustang-jour.jpg?raw=true) |
| `skins-p208-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/avant-skins-p208-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-495/qa/pr/WFS-495/apres-skins-p208-jour.jpg?raw=true) |
