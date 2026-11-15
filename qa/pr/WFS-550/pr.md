## Agent : Claude, agent dev3d (01UY1Vv1yg) (session_01UY1Vv1ygW1sN5qWWTdaUJc)

**Ticket** : WFS-550 — règles : `regles-de-gestion/WFS-550.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : interface R19 (3/5) : passer les indicateurs du jeu au style des maquettes R19 : cadrans fins, jauges et barres fines cyan → orange, anneaux du temps restant, 17 segments de la position dans le garage, compteur d'étoiles animé après un gain, bandeaux inclinés en course (`regles-de-gestion/WFS-550.md`).
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - nouveau `src/ui/jauges.{h,cpp}` : cadran de 240° ouvert en bas, anneau, barre fine graduée, barre d'étape, segments inclinés (−10°) ; tous les calculs (fractions bornées, angles, place des segments, étapes du chargement) sont des fonctions pures testées par `tests/test_jauges.cpp` ;
  - **RG-01 garage** (`garage_fiche.*`, `garage_cadre.cpp`) : Puissance, 0 à 100, Vitesse max dans 3 cadrans fins (remplissage = mêmes échelles que les notes sur 10, sans arrondi : la SF-X remplit les 3 cadrans) ; 4 notes en barres fines graduées (10 cases) ; panneau du bas en verre dépoli de WFS-548 (même place, même taille). Chiffres et libellés inchangés ;
  - **RG-04** : sous le plateau, un segment par voiture permise (blanc possédée, gris bloquée, orange allongé affichée) et « 2 / 17 » ; le nombre suit `voituresPermises()` ; remplace les points de l'en-tête (mêmes zones d'appui `g_z.points`) ;
  - **RG-02 jauges** : chargement (`chargement.cpp`) : étape et grand pourcentage au-dessus de la barre, 4 étapes Moteur / Modèles 3D / Voitures / Ville (coche cyan si finie, orange en cours), 5 nouveaux textes FR / EN ; **chaque segment suit le texte annoncé** (revue #424) : Moteur = « Préparation du moteur » et « Construction de la route » (0 à 20 %), Modèles 3D = « Chargement du modèle 3D… » (20 %), Voitures = « Chargement des voitures » (70 %, juste avant le trafic, système 50), Ville = « Construction de la ville » (85 %, annoncée par un petit système d'ordre 99 de `chargement.cpp`, juste avant la ville) ; accroche d'1 ligne dans `src/game/game.cpp` (texte de l'annonce à 70 %) ; l'écran d'échec ne change pas ; combo en course : jauge fine du temps restant du combo à côté de « Combo ×N » (5 lignes dans `hud.cpp`, dont 2 inclusions) ;
  - **RG-03 menu** : compte à rebours du Cadeau du jour dans une pilule de verre avec anneau fin (rempli à mesure que le cadeau approche) ; bandeau « Voiture prêtée » en pilule de verre bordée de cyan (rouge si le prêt est expiré) avec anneau du temps restant (délai d'activation ou durée du prêt de `configgame.json`) ; boutons inchangés (taille, place) ;
  - **RG-05 compteur d'étoiles** (`compteur_etoiles.cpp`) : quand le solde monte, 7 étoiles volent de la voiture vers la pilule, « +N » orange (gain réel), la pilule s'allume et compte jusqu'au total (≈ 1 s) ; une dépense s'affiche tout de suite ; aucune écriture dans la sauvegarde ;
  - **RG-06 bandeaux** (`toasts.cpp`) : pile centrée sous la rangée du haut, bandeaux inclinés pleins : checkpoint cyan, points orange (quasi-contact, combo, feu grillé), accident / combo perdu rouge ; glissent du haut, s'effacent ; ombre sombre sans flou (aucun coût de rendu ajouté en course).
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) : construction complète sans avertissement nouveau ; **149 tests CPU verts** (`bash tools/qa/tests_cpu.sh build`), dont `test_jauges` (17 cas) et `test_langue` (nouvelles clés FR / EN) ; bancs `garage-voitures`, `cadeau` (FR puis EN : menu, cadeau, gain d'étoiles en 3 images, voiture prêtée : parcours RG-08), `chargement`, `hud` (bandeaux de jour, **nouvelle capture de nuit** `toast-pile-nuit` avec combo ×2) ; le banc `cadeau` vérifie que l'animation ne change pas le solde enregistré.
- **Limites et risques connus** :
  - **jauge « 1 250 / 6 000 » de l'achat (maquette 28) non faite** : elle est dans `garage_achat*`, que RG-07 donne à WFS-551 ; `jauges::barreFine` est prête pour lui (1 ligne) ;
  - les **cadenas** au-dessus des segments gris viennent de `garage_achat.cpp` (WFS-499, fichier de WFS-551, qui traite le « cadenas-jauge ») : non touchés ;
  - le jeu n'a **aucun message « police en approche »** : le rouge s'applique aux bandeaux d'accident et de combo perdu ; aucun nouvel événement (hors périmètre : nouvelles règles) ;
  - l'animation des étoiles part de la voiture affichée au menu (point fixe à 62 % × 52 % de l'écran), pas d'un point calculé sur le modèle 3D ;
  - `CMakeLists.txt` : une ligne (`test_jauges`).

## Tests CPU

`bash tools/qa/tests_cpu.sh build` sur la branche synchronisée avec `main` : sortie complète dans [`qa/pr/WFS-550/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/tests-cpu.log) — **149 test(s) réussi(s) en 436 s, 0 échec** (dont `test_jauges` : un cas par palier du chargement, et `test_langue` : nouvelles clés FR / EN).

## Maquettes (règle de l'utilisateur du 2026-11-15 23:14 UTC : maquette ↔ fichier:fonction)

| Maquette | Fichier : fonction | État |
|---|---|---|
| **05** Cadeau du jour (anneau du temps restant) | `src/ui/ecrans/cadeau_jour.cpp` : `dessinerBouton` (pilule `verre::panneau`, `jauges::anneau`, `iconeCadeau` au centre) | conforme (3e revue : icône de cadeau ajoutée dans l'anneau) |
| **08** Chargement (barre + 4 étapes) | `src/ui/ecrans/chargement.cpp` : `dessinerChargement` (`verre::barreMarque`, `jauges::barreEtape`, `jauges::avancementEtape`) ; annonce de la ville : système `AnnonceVille` (99) | conforme ; écart justifié : le segment en cours reste gris au moment de son annonce (un seul palier par étape, il se remplit d'un coup au palier suivant) |
| **13** Garage, onglet Voiture (cadrans, barres fines) | `src/ui/garage_fiche.cpp` : `FicheGarage::specs` (`jauges::cadran`), `FicheGarage::stats` (`jauges::barreFine`, 10 graduations) ; `src/ui/garage_cadre.cpp` : `cadre::fondPanneau` (`verre::panneau`) | conforme |
| **15** Voiture bloquée (17 segments) | `src/ui/garage_fiche.cpp` : `FicheGarage::position` (`jauges::segments`, « N / 17 ») | conforme ; écart justifié : les cadenas au-dessus des segments gris viennent de `garage_achat.cpp` (WFS-551, RG-07) |
| **17** HUD : jauge de combo | `src/ui/hud.cpp` : bloc « score + combo » du `drawUi` du HUD (`jauges::barreFine` du temps restant du combo, texte « Combo ×N » en orange) | conforme (3e revue : texte passé du cyan à l'orange) |
| **26** Voiture prêtée (pilule bordée de cyan + anneau) | `src/ui/ecrans/pret_bandeau.cpp` : `dessiner` (`verre::panneau` bord cyan / rouge, `jauges::anneau`, `ui::boutons::iconeVoitureTrait` au centre) ; cadeau : voir 05 | conforme (3e revue : icône de voiture ajoutée dans l'anneau) |
| **29** Gain d'étoiles (vol, « +N », comptage) | `src/ui/ecrans/compteur_etoiles.cpp` : `soldeMontre` (comptage), `dessiner` (étoiles en vol, liseré orange, « +N ») | conforme |
| **30** Messages en course (bandeaux inclinés) | `src/ui/toasts.cpp` : `Toasts::drawUi` (`dessin2d::biseau`, couleurs `couleur` / `couleurTexte`) | conforme ; écart justifié (accepté en 1re revue) : pas de message « police en approche » dans le jeu, le rouge sert à l'accident et au combo perdu |
| **28** Achat (jauge 1 250 / 6 000) | `src/ui/garage_achat*` (WFS-551) | hors de cette PR (tranché en 1re revue : WFS-551, RG-07) |

## Captures pour le lead dev (26 après)

Dossier : [`qa/pr/WFS-550`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `cadeau-etoiles-gain-1-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-cadeau-etoiles-gain-1-en.jpg?raw=true) |
| `cadeau-etoiles-gain-1-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-cadeau-etoiles-gain-1-fr.jpg?raw=true) |
| `cadeau-etoiles-gain-2-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-cadeau-etoiles-gain-2-en.jpg?raw=true) |
| `cadeau-etoiles-gain-2-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-cadeau-etoiles-gain-2-fr.jpg?raw=true) |
| `cadeau-etoiles-gain-3-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-cadeau-etoiles-gain-3-en.jpg?raw=true) |
| `cadeau-etoiles-gain-3-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-cadeau-etoiles-gain-3-fr.jpg?raw=true) |
| `cadeau-menu-attente-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-cadeau-menu-attente-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-cadeau-menu-attente-en.jpg?raw=true) |
| `cadeau-menu-attente-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-cadeau-menu-attente-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-cadeau-menu-attente-fr.jpg?raw=true) |
| `cadeau-menu-nuit-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-cadeau-menu-nuit-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-cadeau-menu-nuit-fr.jpg?raw=true) |
| `pret-menu-actif-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-pret-menu-actif-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-pret-menu-actif-en.jpg?raw=true) |
| `pret-menu-actif-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-pret-menu-actif-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-pret-menu-actif-fr.jpg?raw=true) |
| `pret-menu-attente-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-pret-menu-attente-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-pret-menu-attente-fr.jpg?raw=true) |
| `pret-menu-expire-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-pret-menu-expire-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-pret-menu-expire-fr.jpg?raw=true) |
| `chargement-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-chargement-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-chargement-1.jpg?raw=true) |
| `chargement-route` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-chargement-route.jpg?raw=true) |
| `chargement-voitures` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-chargement-voitures.jpg?raw=true) |
| `chargement-ville` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-chargement-ville.jpg?raw=true) |
| `chargement-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-chargement-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-chargement-2.jpg?raw=true) |
| `chargement-erreur` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-chargement-erreur.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-chargement-erreur.jpg?raw=true) |
| `garage-voitures-en-golf-bloquee` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-garage-voitures-en-golf-bloquee.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-garage-voitures-en-golf-bloquee.jpg?raw=true) |
| `garage-voitures-m4-bloquee` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-garage-voitures-m4-bloquee.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-garage-voitures-m4-bloquee.jpg?raw=true) |
| `garage-voitures-p208` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-garage-voitures-p208.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-garage-voitures-p208.jpg?raw=true) |
| `hud-combo` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-hud-combo.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-hud-combo.jpg?raw=true) |
| `toast-checkpoint` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-toast-checkpoint.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-toast-checkpoint.jpg?raw=true) |
| `toast-pile-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-toast-pile-nuit.jpg?raw=true) |
| `toast-pile` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/avant-toast-pile.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-550/qa/pr/WFS-550/apres-toast-pile.jpg?raw=true) |
