## Agent : Claude, agent dev3d (01UY1Vv1yg) (session_01UY1Vv1ygW1sN5qWWTdaUJc)

**Ticket** : WFS-550 — règles : `regles-de-gestion/WFS-550.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : interface R19 (3/5) : indicateurs du jeu au style des maquettes R19 (cadrans fins, jauges et barres fines, anneaux du temps restant, 17 segments, compteur d'étoiles animé, bandeaux en course), puis, sur ses ordres dans la session (2026-11-15 / 16) : **« la maquette appliquée à 100 %, zéro erreur »**, le **menu de la maquette 01** (arbitrage du PO, RG-11), et **« la voiture du menu toujours la même, c'est l'image de marque du jeu »**.
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - `src/ui/jauges.{h,cpp}` (nouveau) : cadran 240°, anneau, anneau gradué des cartes, barre fine graduée, barre unie, barre d'étape, segments inclinés ; calculs purs testés par `tests/test_jauges.cpp` (un cas par palier du chargement) ;
  - **garage (13, 15)** `garage_fiche.*` : 3 cadrans fins et 4 barres fines graduées dans le panneau de l'onglet Voiture de WFS-551 ; 17 segments sous le plateau (blanc possédée, gris bloquée, orange allongé affichée, « 2 / 17 ») à la place des points (conflit de synchronisation résolu : en-tête et onglets de WFS-551 gardés) ;
  - **menu (01, 05, 26, 29)** `ecrans/menu.cpp` : logo sur une ligne, panneau sombre à coin coupé, « COURSE SOLO » / « COURSE EN LIGNE » en boutons ronds, pilule « GARAGE », « RECORD » (toucher → Profil), compteur d'étoiles (toucher → Market) et bouton rond à engrenage (`reglages.cpp`, menu seulement), carte « Cadeau du jour » en bas à droite (`cadeau_jour.cpp` : compte à rebours 07:42:13, secondes orange, anneau gradué, « avant le prochain · 30e garanti dans N j »), carte « Voiture prêtée » au-dessus (`pret_bandeau.cpp` : « 2 h 41 au volant », voiture vue de face) ; Crédits, Market, Profil, sous-titres, « Ta voiture » et l'aide retirés de l'écran (fonctions gardées) ; plus de voile sombre sur la scène ;
  - **voiture du menu** : SF-X de vitrine toujours affichée (orange, finition caméléon, skin bleu ciel, plaques retirées, détails jaunes d'origine en bleu ciel), quelle que soit la voiture du joueur (course, garage, sauvegarde inchangés) ; caméra rapprochée au cadrage de la maquette (`menu_parking_cadrage.cpp`, test mis à jour) ; éclairage du parking réglé sur les mesures de la maquette (`menu_parking.cpp`) ;
  - **compteur d'étoiles (29)** `compteur_etoiles.cpp` : pilule arrondie, étoiles en vol, « +N », comptage ≈ 1 s, lueur orange ; solde enregistré jamais touché ;
  - **bandeaux (30)** `toasts.cpp` : bandes droites fondues aux bords, titre sombre, valeur blanche, plus récent en bas, 4 au plus ; « +30 s » ; « COMBO ×N » séparé du quasi-contact (même total) ; « POLICE EN APPROCHE » (rouge) dans la Poursuite quand une police roulante passe à moins de 60 m (au plus toutes les 8 s) ;
  - **combo (17)** `hud.cpp` (accroche 5 lignes) : « COMBO ×N » orange en capitales et barre orange du temps restant ;
  - **chargement (08)** `ecrans/chargement.cpp` : grand pourcentage, barre arrondie, 4 étapes alignées sur les textes annoncés (accroche 1 ligne dans `game.cpp`, système `AnnonceVille` d'ordre 99) ; le dessin complet de la maquette 08 vient de WFS-557 (accord entre les deux sessions : le 2e fusionné garde son dessin et ces paliers) ;
  - textes FR / EN ajoutés : `chargement.segment.*`, `chargement.etape.voitures`, `cadeau.carte.*`, `pret.carte.*`, `menu.r19.*`, `toast.combo`, `toast.police_approche`.
- **Comment c'est vérifié** : construction complète ; **151 tests CPU verts** après synchronisation avec `main` (dont `test_jauges`, `test_menu_parking`, `test_langue`) ; bancs `garage-voitures`, `cadeau` (FR / EN, gain d'étoiles en 3 images, voiture prêtée), `chargement` (un palier par étape), `hud` (bandeaux de jour et de nuit, capture `toast-maquette`) ; mesures de luminosité de la voiture du menu contre la maquette 01 (voiture 89 contre 98 sur 255).
- **Limites et risques connus** :
  - voiture du menu : bandes du capot de la skin « Bleu ciel » plus marquées que sur la maquette, reflets orange plus faibles ; sol encore plus clair que la maquette (motif procédural du béton) ;
  - petits textes : police Inter attendue de WFS-557 ; bague des boutons ronds redessinée par WFS-557 ;
  - la carte « Cadeau du jour » occupe la place des anciens boutons de droite (arbitrage RG-11).

## Tests CPU

`bash tools/qa/tests_cpu.sh build` sur la branche synchronisée avec `main` : [`qa/pr/WFS-550/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/tests-cpu.log) — **151 test(s) réussi(s), 0 échec**.

## Maquettes (règle de l'utilisateur du 2026-11-15 23:14 UTC)

| Maquette | Fichier : fonction | État |
|---|---|---|
| **01** Menu | `src/ui/ecrans/menu.cpp` : `Menu::drawUi`, `Menu::scene`, `Menu::render` (SF-X de vitrine) ; `src/ui/menu_parking*.cpp` (cadrage, éclairage) ; `src/ui/ecrans/chargement.cpp` : `dessinerLogoLigne` ; `reglages.cpp` : `dessinerEngrenage` | conforme pour la mise en page ; écarts : bandes de la skin, reflets, sol un peu clair (voir limites) |
| **05 / 26** Cadeau du jour, voiture prêtée | `cadeau_jour.cpp` : `dessinerBouton`, `placeCarte` ; `pret_bandeau.cpp` : `dessiner`, `voitureDeFace` ; `jauges::anneauGradue` | conforme |
| **08** Chargement | `chargement.cpp` : `dessinerChargement`, `AnnonceVille` ; `jauges::barreEtape`, `avancementEtape` | conforme pour la barre et les étapes ; logo et fenêtre : WFS-557 |
| **13 / 15** Garage, onglet Voiture | `garage_fiche.cpp` : `specs` (`jauges::cadran`), `stats` (`jauges::barreFine`), `position` (`jauges::segments`) | conforme ; onglets et panneau : WFS-551 (fusionné) |
| **17** Combo | `hud.cpp` (bloc score + combo) : `jauges::barreUnie` | conforme |
| **29** Gain d'étoiles | `compteur_etoiles.cpp` : `soldeMontre`, `dessiner` | conforme |
| **30** Messages en course | `toasts.cpp` : `Toasts::update` (combo, police en approche), `Toasts::drawUi` | conforme |
| **28** Jauge d'achat | WFS-551 (tranché en 1re revue) | hors de cette PR |

## Captures pour le lead dev (20 après)

Dossier : [`qa/pr/WFS-550`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550) — produit par `tools/qa/captures_pr.sh`.

Captures « avant » : `origin/main` = `68a71e26edc4f336bd583795aece05af9386c094` ; cache : 0 banc(s) relu(s), 4 banc(s) rejoué(s) ; main compilé : oui.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `cadeau-etoiles-gain-1-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-cadeau-etoiles-gain-1-fr.jpg?raw=true) |
| `cadeau-etoiles-gain-2-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-cadeau-etoiles-gain-2-fr.jpg?raw=true) |
| `cadeau-etoiles-gain-3-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-cadeau-etoiles-gain-3-fr.jpg?raw=true) |
| `cadeau-menu-attente-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-cadeau-menu-attente-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-cadeau-menu-attente-en.jpg?raw=true) |
| `cadeau-menu-attente-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-cadeau-menu-attente-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-cadeau-menu-attente-fr.jpg?raw=true) |
| `cadeau-menu-nuit-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-cadeau-menu-nuit-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-cadeau-menu-nuit-fr.jpg?raw=true) |
| `pret-menu-actif-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-pret-menu-actif-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-pret-menu-actif-fr.jpg?raw=true) |
| `pret-menu-attente-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-pret-menu-attente-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-pret-menu-attente-fr.jpg?raw=true) |
| `chargement-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-chargement-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-chargement-1.jpg?raw=true) |
| `chargement-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-chargement-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-chargement-2.jpg?raw=true) |
| `chargement-erreur` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-chargement-erreur.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-chargement-erreur.jpg?raw=true) |
| `chargement-route` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-chargement-route.jpg?raw=true) |
| `chargement-ville` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-chargement-ville.jpg?raw=true) |
| `chargement-voitures` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-chargement-voitures.jpg?raw=true) |
| `garage-voitures-m4-bloquee` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-garage-voitures-m4-bloquee.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-garage-voitures-m4-bloquee.jpg?raw=true) |
| `garage-voitures-p208` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-garage-voitures-p208.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-garage-voitures-p208.jpg?raw=true) |
| `hud-combo` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-hud-combo.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-hud-combo.jpg?raw=true) |
| `toast-maquette` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-toast-maquette.jpg?raw=true) |
| `toast-pile-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-toast-pile-nuit.jpg?raw=true) |
| `toast-pile` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-toast-pile.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-toast-pile.jpg?raw=true) |
