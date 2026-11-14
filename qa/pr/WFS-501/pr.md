## Agent : Claude, agent dev3d (01Us6uCviA)

**Ticket** : WFS-501 — règles : `regles-de-gestion/WFS-501.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : un cadeau gratuit par jour, ouvert depuis le menu avec une animation, puis le gain, et un bouton « Chances » qui montre le tableau des probabilités, en français et en anglais (épic E19, `regles-de-gestion/WFS-501.md`, RG-01 à 08).
- **Ce qui change** :
  - **Nouveau** `src/ui/ecrans/cadeau_jour.{h,cpp}` : système « CadeauJour » (ordre 466, dessiné seulement en `Ecran::Menu`, par-dessus le menu), panneau dans le style des Crédits (voile, panneau à liseré cyan, sur-titre, titre, boutons en biseau). Quatre vues :
    - **animation** ≈ 1,5 s : boîte cadeau dessinée en 2D, néons orange et ruban cyan, qui tremble (0 à 0,9 s), puis le couvercle se soulève (0,9 à 1,3 s), lumière et rayons, puis le gain monte ; un appui passe l'animation ;
    - **gain** (RG-03) : « +23 ★ » ; « Nouveau coloris : <nom> pour <voiture> » avec la pastille de la couleur ; « Nouvelle skin : <nom> pour <voiture> » ; « Voiture prêtée 3 h : <voiture> » avec « Activer », qui ferme l'écran (branché par WFS-502). Les noms des voitures sont ceux de `configgame.json`, jamais une marque ;
    - **Chances** (RG-04) : les 7 lignes lues dans `config().cadeau` (rien dans le code), plus « Minimum 10 ★ par cadeau », la garantie au 30e cadeau et le compteur du joueur ;
    - **attente** : cadeau déjà ouvert → « Prochain cadeau dans hh:mm » et le dernier gain réaffiché (`cadeau-dernier`, RG-02).
  - **Tirage** : il est fait **avant** l'animation, par `cadeau::ouvrir()` de WFS-500, qui écrit la sauvegarde d'abord. Quitter pendant l'animation ne perd ni ne double rien. **Aucun bouton payant** (RC-E19-01).
  - **Nouveau** `src/ui/ecrans/cadeau_jour_calculs.cpp` contient les calculs purs : temps jusqu'au prochain minuit local, format hh:mm, lignes des chances, compteur de la garantie. **Nouveau** `tests/test_cadeau_ecran.cpp` les teste (1 ligne dans `CMakeLists.txt`).
  - **Nouveau** `src/ui/ecrans/cadeau_jour_banc.cpp` : banc `WFS_BANC=cadeau` (RG-07), en FR puis EN. Son heure est forcée et la vraie sauvegarde n'est pas touchée. `WFS_CADEAU=etoiles|coloris|skin|voiture|chances|attente` ne garde que ce type de captures.
  - **Accroche `menu.cpp`** (7 lignes, annoncée au journal) : include, `CADEAU = 5`, touchers vers l'écran ouvert, zone du bouton, action « Cadeau du jour », menu non dessiné sous l'écran, appel `dessinerBouton`.
    - Le bouton « CADEAU DU JOUR » a **la taille et le style de « Crédits »**. Il est placé **au-dessus de « Crédits »**, en bas à droite. Le compteur d'étoiles de WFS-537 (en haut à droite, près de « Réglages ») n'est pas touché.
    - Une **pastille orange** apparaît quand un cadeau attend. Sinon, « Prochain cadeau dans hh:mm », ou « Prochain cadeau bientôt » si l'heure a reculé, est écrit à gauche du bouton.
  - **Accroche `desktop_main.cpp`** (2 lignes) : banc `cadeau`.
  - **Textes** : 30 clés `cadeau.*` et 4 clés `skin.*` dans `assets/textes/fr.txt` et `en.txt`, à la suite des Crédits. La police n'a pas le ★ : il est dessiné par `garage::texteEtoiles` (WFS-489), réutilisé. Les noms des finitions viennent de `finition.<id>` de WFS-491 quand il sera fusionné ; d'ici là, le nom du catalogue est affiché.
- **Comment c'est vérifié** :
  - 133 tests CPU verts, dont le nouveau `test_cadeau_ecran` : minuit local à Paris et à New York, heure reculée, arrondi hh:mm, chances suivies quand `configgame.json` change, somme 100 %, garantie.
  - Banc `cadeau` : **0 erreur**. Parcours RG-08 au doigt en FR et en EN, 0 écart (ci-dessous).
  - Garde-fou des textes de WFS-504 en anglais : 0 phrase française, 0 texte qui dépasse.
  - 30 captures regardées une à une.
  - Menu avant / après : banc `credits` (`credits-menu`, `credits-retour-menu`).
- **Limites et risques connus** :
  - Le gain n'est pas encore donné au joueur (WFS-503), et « Activer » ne démarre pas encore le prêt (WFS-502) : hors périmètre.
  - Rendu vérifié sur lavapipe seulement, en 1280x720.
  - Le banc `cadeau` n'existe pas sur `main` : il n'y a donc pas de capture « avant » pour l'écran du cadeau, qui est nouveau. Le menu a son « avant » par le banc `credits`.
  - Si WFS-491 et cette PR se suivent, `fr.txt` et `en.txt` auront un petit conflit de fusion (clés différentes) : il suffira de garder les deux.

## Tests CPU

Commit testé : `ff166e29` (branche synchronisée avec main), `bash tools/qa/tests_cpu.sh build` ; fin du journal (complet : `qa/pr/WFS-501/tests-cpu.log`) :

```
[OK] test_vehicule_skin (0 s)
[OK] test_version_chargement (0 s)
[OK] test_ville_lieux (0 s)
[OK] test_ville_sol (3 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 133 test(s) réussi(s) en 401 s
code de sortie : 0
```

## Parcours RG-08 (banc `cadeau`, journal)

```
parcours fr : « Cadeau du jour » → animation d'ouverture : OK
parcours fr : tirage fait avant l'animation (1 cadeau ouvert) : OK
parcours fr : fin de l'animation → gain : OK
parcours fr : « Chances » → tableau des chances : OK
parcours fr : « Retour » des chances → gain : OK
parcours fr : « Retour » → menu : OK
parcours fr : menu : plus de cadeau (pastille partie) : OK
parcours fr : 2e appui le même jour : écran d'attente, aucun 2e cadeau : OK
parcours fr : après la relance : écran d'attente, aucun 2e cadeau : OK
parcours fr : « Activer » ferme l'écran : OK
parcours en : « Cadeau du jour » → animation d'ouverture : OK
parcours en : tirage fait avant l'animation (1 cadeau ouvert) : OK
parcours en : fin de l'animation → gain : OK
parcours en : « Chances » → tableau des chances : OK
parcours en : « Retour » des chances → gain : OK
parcours en : « Retour » → menu : OK
parcours en : menu : plus de cadeau (pastille partie) : OK
parcours en : 2e appui le même jour : écran d'attente, aucun 2e cadeau : OK
parcours en : après la relance : écran d'attente, aucun 2e cadeau : OK
parcours en : « Activer » ferme l'écran : OK
cadeau (WFS-501) : banc terminé, 0 erreur(s)
```

## Captures pour le lead dev (30 après)

Dossier : [`qa/pr/WFS-501`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `cadeau-attente-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-attente-en.jpg?raw=true) |
| `cadeau-attente-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-attente-fr.jpg?raw=true) |
| `cadeau-chances-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-chances-en.jpg?raw=true) |
| `cadeau-chances-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-chances-fr.jpg?raw=true) |
| `cadeau-coloris-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-coloris-en.jpg?raw=true) |
| `cadeau-coloris-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-coloris-fr.jpg?raw=true) |
| `cadeau-etoiles-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-etoiles-en.jpg?raw=true) |
| `cadeau-etoiles-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-etoiles-fr.jpg?raw=true) |
| `cadeau-etoiles-nuit-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-etoiles-nuit-en.jpg?raw=true) |
| `cadeau-etoiles-nuit-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-etoiles-nuit-fr.jpg?raw=true) |
| `cadeau-menu-attente-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-menu-attente-en.jpg?raw=true) |
| `cadeau-menu-attente-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-menu-attente-fr.jpg?raw=true) |
| `cadeau-menu-bientot-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-menu-bientot-en.jpg?raw=true) |
| `cadeau-menu-bientot-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-menu-bientot-fr.jpg?raw=true) |
| `cadeau-menu-nuit-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-menu-nuit-en.jpg?raw=true) |
| `cadeau-menu-nuit-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-menu-nuit-fr.jpg?raw=true) |
| `cadeau-menu-pastille-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-menu-pastille-en.jpg?raw=true) |
| `cadeau-menu-pastille-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-menu-pastille-fr.jpg?raw=true) |
| `cadeau-ouverture-1-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-ouverture-1-en.jpg?raw=true) |
| `cadeau-ouverture-1-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-ouverture-1-fr.jpg?raw=true) |
| `cadeau-ouverture-2-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-ouverture-2-en.jpg?raw=true) |
| `cadeau-ouverture-2-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-ouverture-2-fr.jpg?raw=true) |
| `cadeau-ouverture-3-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-ouverture-3-en.jpg?raw=true) |
| `cadeau-ouverture-3-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-ouverture-3-fr.jpg?raw=true) |
| `cadeau-skin-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-skin-en.jpg?raw=true) |
| `cadeau-skin-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-skin-fr.jpg?raw=true) |
| `cadeau-voiture-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-voiture-en.jpg?raw=true) |
| `cadeau-voiture-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-cadeau-voiture-fr.jpg?raw=true) |
| `credits-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/avant-credits-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-credits-menu.jpg?raw=true) |
| `credits-retour-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/avant-credits-retour-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-501/qa/pr/WFS-501/apres-credits-retour-menu.jpg?raw=true) |
