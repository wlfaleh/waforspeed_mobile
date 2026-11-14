## Agent : Claude, agent dev3d (01Us6uCwV6)

**Ticket** : WFS-494 — règles : `regles-de-gestion/WFS-494.md`

## Description

- **Demande de l'utilisateur** : au garage, choisir et acheter les skins (Rallye, Camouflage, Drift Tokyo ; Néon nuit réservée au cadeau du jour et aux succès), avec une skin offerte par voiture débloquée et un prix en étoiles × la classe de la voiture (épic E18, `regles-de-gestion/WFS-494.md`).
- **Ce qui change à l'écran** :
  - **Rangée « Skin »** sous les finitions (RG-01), seulement pour les 6 voitures qui ont des skins (208, 500, GT86, Golf, Corvette C7, Giulia) : Aucune, Rallye, Camouflage, Drift Tokyo, Néon nuit ; échantillon des 2 couleurs de la skin, cadenas et prix en ★ (base × coefficient de la classe, `configgame.json`), nom et équivalent en médailles / euros à droite (RC-E18-11). Mêmes boutons que les finitions (104 × 30 px) ; écran étroit : chevrons.
  - **Aperçu (RG-05)** : un appui pose la skin sur la voiture qui tourne (une seule texture d'habillage en mémoire, RC-E18-02). Non possédée : « Acheter N ★ » (la skin s'ajoute à la couleur et à la finition), refus « Il te manque N ★ » sans débit. Possédée : gardée tout de suite (`skin-<voiture>`). En sortant du garage, la voiture de la course reprend sa skin possédée.
  - **Néon nuit (RG-04)** : cadenas, « Cadeau » sous le bouton, « Bientôt dans le cadeau du jour » à droite ; jamais en vente, jamais en course tant qu'elle n'est pas donnée.
  - **Skin offerte (RG-02)** : fenêtre « Choisis ta skin offerte » (3 skins en vente avec leur valeur, aperçu sur la voiture, « Plus tard » / « Prendre »), une par voiture, gardée dans la sauvegarde (nouvelle clé `skins-offertes`). Elle s'ouvre **d'elle-même juste après l'achat d'une voiture** (RG-04 de WFS-499) ; pour une voiture déjà possédée (208, 500, GT86 dès le départ, ou « Plus tard »), bouton orange **« Choisis ta skin offerte »** à droite de la rangée.
  - **Bon de skin du market** (pack débutant, WFS-510 RG-02) : « Utiliser un bon (N) » à la place du prix, sur une skin en vente non possédée d'une voiture possédée.
- **Choix de ma part (à confirmer par le lead dev / PO)** : pas de fenêtre surprise à l'ouverture du garage. Ouverte d'office, elle arrêtait le joueur qui vient juste choisir sa voiture, et elle mangeait les touchers de 5 bancs existants (finitions, couleurs, voitures, mode testeur, prêt) qui ouvrent le garage avec une sauvegarde active.
- **Fichiers** :
  - nouveaux : `src/ui/garage_skin.cpp` (panneau + fenêtre), `src/game/skin_offerte.*` (règles : skin offerte, `donnerSkin(voiture, skin)` pour WFS-503, bons), `src/ui/garage_skin_banc.cpp` (banc `garage-skins`), `tests/test_garage_skin.cpp` ;
  - `src/ui/garage_skin.h` (emplacement vide de WFS-534 rempli) ; textes `assets/textes/fr.txt` / `en.txt` (12 clés) ;
  - accroches : `garage.cpp` (ouverture, aperçu, touchers, hauteur du panneau, fenêtre), `garage_commun.h` (zones, `EtatGarage::skin`), `garage_achat*` (la skin dans « ce qui manque » et gardée à l'achat), `garage.h` / `desktop_main.cpp` (banc) ;
  - E3 (≤ 10 lignes) : `sauvegarde.*` (clé `skins-offertes`, 4 lignes) ; `market.h` : `SKINS_EN_VENTE = true` (ligne prévue pour WFS-494 : le pack débutant et son bon de skin sont maintenant proposés).
- **Limites et risques** : images/s mesurées sur lavapipe seulement (à confirmer sur l'APK) ; changer de skin au garage recharge une texture (~1,4 Mo, attente du GPU) : un petit à-coup possible sur le téléphone au moment de l'appui, jamais en course ; le contenu du cadeau du jour et des succès reste à WFS-503 (`donnerSkin` est prête).

## Tests CPU

- `test_garage_skin` (nouveau, 41 cas) : ordre affiché, 6 voitures, prix × classe, Néon nuit jamais en vente ni en course sans être possédée, `donnerSkin`, ce qui manque (5 objets), refus sans débit, achat pour une seule voiture, skin offerte (gratuites dès le départ, voiture achetée, une seule, pas Néon nuit), bon de skin, relecture de la sauvegarde (clé inconnue ignorée, ancienne sauvegarde lue sans erreur), pack débutant proposé.
- `bash tools/qa/tests_cpu.sh build` : **143 tests réussis**.

## Parcours RC-E18-09 et banc `garage-skins` (0 écart, 1280x720 et 1024x768)

208 : rangée, « Choisis ta skin offerte » → Camouflage prise (0 ★) → Drift Tokyo en aperçu, refus à 100 ★, achat 600 ★ → Néon nuit (« Choisir » n'achète rien) ; Golf achetée → fenêtre ouverte d'elle-même → Rallye ; bon → Drift Tokyo ; Giulia « Plus tard » puis Rallye ; 208 « Choisir » → terrain → « Rouler » → **course en Camouflage** → garage → aperçu Néon nuit → « Retour » : la voiture garde le Camouflage ; **relance du jeu** : tout est gardé ; courses de jour et de nuit (208 Camouflage, Golf Drift Tokyo, Giulia Rallye) ; en anglais (GT86). Banc `garage-finitions` toujours à 0 écart.

## Images/s en qualité Basse (RC-E18-03)

MESURE

## Captures pour le lead dev (37 après)

Dossier : [`qa/pr/WFS-494`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `garage-finitions-course-golf-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/avant-garage-finitions-course-golf-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-finitions-course-golf-nuit.jpg?raw=true) |
| `garage-finitions-en-golf` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/avant-garage-finitions-en-golf.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-finitions-en-golf.jpg?raw=true) |
| `garage-finitions-golf-debut` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/avant-garage-finitions-golf-debut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-finitions-golf-debut.jpg?raw=true) |
| `garage-skins-1024x768-en-gt86-fenetre` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-1024x768-en-gt86-fenetre.jpg?raw=true) |
| `garage-skins-1024x768-en-gt86-rangee` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-1024x768-en-gt86-rangee.jpg?raw=true) |
| `garage-skins-1024x768-en-il-manque` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-1024x768-en-il-manque.jpg?raw=true) |
| `garage-skins-1024x768-en-neon` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-1024x768-en-neon.jpg?raw=true) |
| `garage-skins-1024x768-p208-camouflage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-1024x768-p208-camouflage.jpg?raw=true) |
| `garage-skins-1024x768-p208-drift-apercu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-1024x768-p208-drift-apercu.jpg?raw=true) |
| `garage-skins-1024x768-p208-fenetre-camouflage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-1024x768-p208-fenetre-camouflage.jpg?raw=true) |
| `garage-skins-1024x768-p208-fenetre-offerte` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-1024x768-p208-fenetre-offerte.jpg?raw=true) |
| `garage-skins-1024x768-p208-neon-cadeau` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-1024x768-p208-neon-cadeau.jpg?raw=true) |
| `garage-skins-1024x768-p208-rangee` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-1024x768-p208-rangee.jpg?raw=true) |
| `garage-skins-course-giulia-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-course-giulia-jour.jpg?raw=true) |
| `garage-skins-course-giulia-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-course-giulia-nuit.jpg?raw=true) |
| `garage-skins-course-golf-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-course-golf-jour.jpg?raw=true) |
| `garage-skins-course-golf-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-course-golf-nuit.jpg?raw=true) |
| `garage-skins-course-p208-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-course-p208-jour.jpg?raw=true) |
| `garage-skins-course-p208-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-course-p208-nuit.jpg?raw=true) |
| `garage-skins-en-gt86-fenetre` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-en-gt86-fenetre.jpg?raw=true) |
| `garage-skins-en-gt86-rangee` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-en-gt86-rangee.jpg?raw=true) |
| `garage-skins-en-il-manque` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-en-il-manque.jpg?raw=true) |
| `garage-skins-en-neon` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-en-neon.jpg?raw=true) |
| `garage-skins-giulia-rallye` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-giulia-rallye.jpg?raw=true) |
| `garage-skins-golf-achat-fenetre-offerte` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-golf-achat-fenetre-offerte.jpg?raw=true) |
| `garage-skins-golf-bon` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-golf-bon.jpg?raw=true) |
| `garage-skins-golf-rallye` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-golf-rallye.jpg?raw=true) |
| `garage-skins-p208-camouflage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-p208-camouflage.jpg?raw=true) |
| `garage-skins-p208-drift-apercu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-p208-drift-apercu.jpg?raw=true) |
| `garage-skins-p208-fenetre-camouflage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-p208-fenetre-camouflage.jpg?raw=true) |
| `garage-skins-p208-fenetre-offerte` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-p208-fenetre-offerte.jpg?raw=true) |
| `garage-skins-p208-il-manque` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-p208-il-manque.jpg?raw=true) |
| `garage-skins-p208-neon-cadeau` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-p208-neon-cadeau.jpg?raw=true) |
| `garage-skins-p208-rangee` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-p208-rangee.jpg?raw=true) |
| `garage-skins-parcours-p208-course` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-parcours-p208-course.jpg?raw=true) |
| `garage-skins-parcours-p208-retour-menu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-parcours-p208-retour-menu.jpg?raw=true) |
| `garage-skins-relance-golf` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-494/qa/pr/WFS-494/apres-garage-skins-relance-golf.jpg?raw=true) |
