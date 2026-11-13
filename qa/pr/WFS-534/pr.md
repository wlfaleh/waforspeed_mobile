Agent : lead-dev (renfort)

**Ticket** : WFS-534 [TECHNIQUE] — règles : `regles-de-gestion/WFS-534.md`. Codé par le lead dev en renfort (sujet bloquant confié par le PO) ; **fusionnée par son auteur sur ordre de l'utilisateur (2026-11-13)**, relayé par le PO (« il le prend rapidement, c'est pas grand chose, et il merge »), exception valable pour WFS-534 seulement.

## Description

- **Demande de l'utilisateur** : « pourquoi toutes ces liaisons, le code n'est pas découpé en sous-composants ? » : découper `src/ui/garage.cpp` (678 lignes) en sous-panneaux pour que les tickets du garage (489, 491, 494, 499, 502, 507, 513) avancent en parallèle, **sans rien changer à l'écran ni au comportement**.
- **Ce qui change** (code seulement, aucun pixel) :
  - `garage.cpp` : **678 → 249 lignes** ; il ne garde que le système « garage » (ordre 440 inchangé, nom `ecrans::GarageEnregistre` inchangé dans le journal), la mise en page (`MiseEnPage`), les touchers et l'enchaînement des panneaux, dans le même ordre de dessin qu'avant.
  - Nouveaux fichiers (RG-01) : `garage_commun.*` (jetons de couleur, fiches, zones d'appui, `MiseEnPage`, `EtatGarage`, interface `PanneauGarage` : dessiner / appui / viderZones) ; `garage_couleur.*` (rangée des 16 couleurs → WFS-489) ; `garage_finition.h` et `garage_skin.h` (emplacements **vides** → WFS-491, WFS-494 : rien n'est dessiné, aucune zone) ; `garage_achat.*` (Choisir / Retour, emplacement de l'achat → WFS-489, 499, 502) ; `garage_fiche.*` (en-tête, caractéristiques, barres) ; `garage_cadre.*` (voiles, flèches, fond vitré, crédit, chargement) ; `garage_decor.*` (décor 3D, code repris tel quel) ; `garage_bancs.cpp` (`benchGarage`, `benchGarageTour`).
  - RG-05 : guide « quel fichier pour quel ticket » en tête de `garage.h`.
  - Zones d'appui : une seule variable globale `g_z`, comme avant (elles survivent à `Game::init`, ce que lisent `garageBouton` et les parcours) ; chaque panneau n'écrit que ses champs. API publique de `garage.h` inchangée.
  - `CMakeLists.txt` inchangé (les `src/ui/*.cpp` sont pris par le GLOB).
- **Comment c'est vérifié** :
  - Construction complète sans avertissement dans les fichiers du garage ; tests CPU : voir « Tests CPU » ci-dessous.
  - **Comparaison pixel à pixel (ImageMagick `compare -metric AE`, PNG d'origine) : 0 pixel différent sur 59 paires** : banc `garage` 1280x720 (14 captures, dont `garage-peintures`, `garage-course`, `course-p911-gris`) ; tour du garage `garage-tour-01…17` (banc `voitures-retirees`, 17 captures) ; banc `garage` en anglais (`WFS_LANGUE=en`, 14) ; banc `garage` en 2400x1080 (14).
  - Journal du banc `garage` (en anglais) identique ligne à ligne entre main et la branche (hors chemins de sortie) : mêmes systèmes, même ordre, mêmes voitures, peintures et boutons.
- **Limites et risques** :
  - Le garage est une scène intérieure sans jour ni nuit : pas de capture de nuit (sans objet). La GT86 (« Hachi ») est dans le tour (`garage-tour-13-gt86`).
  - En anglais, le garage reste en français : c'est l'état de `main` (traduction = WFS-507) ; identique avant / après.
  - Les formes nues du décor 3D (disque, tore, boîte) sont **déplacées sans changement** dans `garage_decor.cpp` : hors périmètre (RG-03), tickets « formes géométriques ».
  - Pas de nouveau test CPU : aucune logique nouvelle ; la preuve est la comparaison pixel à pixel et le journal identique.
  - Captures : `tools/qa/captures_pr.sh` a produit les PNG avant / après puis a été arrêté par la limite de temps de la session pendant la fin des captures « avant » du banc `voitures-retirees` (après le tour du garage) ; la réduction en JPG et ce tableau ont été faits ensuite avec les mêmes commandes que l'étape 4 du script. Anglais et 2400x1080 : bancs lancés à la main (branche 23d54111 contre main 311ada52, avant la fusion de WFS-508 qui ne touche pas le garage).

## Tests CPU

Commit testé : voir la ligne ci-dessous (journal complet : `qa/pr/WFS-534/tests-cpu.log`).

```
commit ab6f7cac
[OK] test_ville_sol (4 s)
[OK] test_virages_fleches (1 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 122 test(s) réussi(s) en 463 s
(1er passage : test_audio_chocs RG-07 en échec de chronométrage, 0,0526 ms pour 0,05, machine chargée par les bancs ; relancé seul puis toute la suite : vert. Test sans lien avec le garage.)
```

## Captures pour le lead dev (31 après, toutes identiques à main)

Dossier : [`qa/pr/WFS-534`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `garage-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-0.jpg?raw=true) |
| `garage-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-1.jpg?raw=true) |
| `garage-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-2.jpg?raw=true) |
| `garage-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-course.jpg?raw=true) |
| `garage-m4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-m4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-m4.jpg?raw=true) |
| `garage-peinture-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-peinture-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-peinture-0.jpg?raw=true) |
| `garage-peinture-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-peinture-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-peinture-1.jpg?raw=true) |
| `garage-peintures-p911` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-peintures-p911.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-peintures-p911.jpg?raw=true) |
| `garage-peintures` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-peintures.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-peintures.jpg?raw=true) |
| `garage-suivant` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-suivant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-suivant.jpg?raw=true) |
| `garage-tour-01-golf` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-01-golf.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-01-golf.jpg?raw=true) |
| `garage-tour-02-m4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-02-m4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-02-m4.jpg?raw=true) |
| `garage-tour-03-p911` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-03-p911.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-03-p911.jpg?raw=true) |
| `garage-tour-04-gallardo` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-04-gallardo.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-04-gallardo.jpg?raw=true) |
| `garage-tour-05-p208` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-05-p208.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-05-p208.jpg?raw=true) |
| `garage-tour-06-abarth` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-06-abarth.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-06-abarth.jpg?raw=true) |
| `garage-tour-07-tcross` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-07-tcross.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-07-tcross.jpg?raw=true) |
| `garage-tour-08-compass` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-08-compass.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-08-compass.jpg?raw=true) |
| `garage-tour-09-s3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-09-s3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-09-s3.jpg?raw=true) |
| `garage-tour-10-giulia` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-10-giulia.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-10-giulia.jpg?raw=true) |
| `garage-tour-11-model3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-11-model3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-11-model3.jpg?raw=true) |
| `garage-tour-12-corsa` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-12-corsa.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-12-corsa.jpg?raw=true) |
| `garage-tour-13-gt86` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-13-gt86.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-13-gt86.jpg?raw=true) |
| `garage-tour-14-v60` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-14-v60.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-14-v60.jpg?raw=true) |
| `garage-tour-15-rrsport` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-15-rrsport.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-15-rrsport.jpg?raw=true) |
| `garage-tour-16-mustang` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-16-mustang.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-16-mustang.jpg?raw=true) |
| `garage-tour-17-sf90` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-tour-17-sf90.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-tour-17-sf90.jpg?raw=true) |
| `garage-0-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-0-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-0-en.jpg?raw=true) |
| `garage-m4-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-m4-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-m4-en.jpg?raw=true) |
| `garage-0-g24` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-0-g24.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-0-g24.jpg?raw=true) |
| `garage-m4-g24` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/avant-garage-m4-g24.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-534/qa/pr/WFS-534/apres-garage-m4-g24.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv5DBBy9VB8jyoPkxZa
