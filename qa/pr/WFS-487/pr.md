Agent : Claude, agent dev3d (01Us6uCwB3)

**Ticket** : WFS-487 (épic E18, P1) — règles : `regles-de-gestion/WFS-487.md` (+ RC-E18-01 à 11 de `regles-de-gestion/E18-personnalisation.md`)

## Demande de l'utilisateur (en une phrase)
Les données de toute la personnalisation des voitures, sans écran : 28 couleurs, finitions, skins, autocollants, leurs prix en étoiles (prix de base × coefficient de la classe, dans `configgame.json`), ce que le joueur possède, l'achat et le choix de chaque voiture, gardés dans la sauvegarde.

## Ce qui change
- **Une seule liste de couleurs (RG-01, RG-02)** : `PEINTURES` (`src/game/sauvegarde.h`) a maintenant 28 couleurs : les 16 du jeu (mêmes identifiants, mêmes hex) puis les 12 couleurs vives de RG-02. `PAINTS` du garage (`src/ui/garage.cpp`) lit `PEINTURES` et en montre les 16 premières (`NB_PEINTURES_GARAGE`), comme aujourd'hui. Les 28 pastilles viendront avec WFS-489.
- **Catalogue et prix dans `configgame.json` (RG-03, RG-04)** : nouvelle section `boutique` (+ 128 clés, documentées dans `configgame.README.md`) :
  - les coefficients des classes (gratuites ×1, 1 ×1, 2 ×1,5, 3 ×2, 4 ×3, 5 ×4) et la classe de chaque voiture (WFS-498 RG-04) ;
  - pour chaque couleur, finition, skin et pack d'autocollants : son prix de base, `gratuit` et `cadeau` (skins seulement) ;
  - les voitures de chaque skin : 208, 500, GT86, Golf, Corvette (`mustang`), Giulia.
  - **Le fichier n'est pas régénéré** : les 10 réglages de l'utilisateur (vitesse des ordinateurs, caméras…) restent tels quels, seule la section `boutique` est ajoutée.
- **`src/game/personnalisation.h/.cpp` (nouveau)** :
  - le catalogue : 28 couleurs, la couleur secondaire, 8 finitions, 4 skins, 5 packs ;
  - les prix (`prix(objet, voiture)` = base × coefficient, arrondi à l'étoile ; les packs d'autocollants n'ont pas de coefficient) ;
  - la possession (`possede`) et l'achat : `acheter(sauvegarde, solde, objet, voiture)` débite une seule fois ; un solde insuffisant, un objet déjà possédé ou un objet pas en vente ne débitent rien ;
  - `aspectGarage` (aperçu, objets possédés ou non) et `aspectCourse` (objets possédés seulement, RG-07) ;
  - la migration des sauvegardes d'avant (RG-03) et les bilans pour le journal.
- **Sauvegarde (RG-05, RG-06)** :
  - nouvelles clés : `perso`, `possede-<voiture>`, `possede-dessins`, `paint2-`, `finition-`, `skin-` et `creation-<voiture>` ; aucune clé retirée ;
  - une valeur inconnue revient au défaut, sans erreur ;
  - une sauvegarde sans `perso=1` date d'avant ce ticket : chaque couleur déjà choisie devient possédée par sa voiture (rien de perdu) ;
  - `peinture()` (duel en ligne, fantôme) et le nouveau `peintureCourse()` ne rendent qu'une couleur possédée.
- **Course (RG-07)** : `Game::setCar` lit `peintureCourse` au lieu de `peintureChoisie`. C'est une accroche d'**1 ligne** dans `src/game/game.cpp`, hors de la zone E3, annoncée au journal.
- **Garage actuel : don provisoire jusqu'à WFS-489 (option A tranchée par le PO, `regles-de-gestion/WFS-487.md` § Suivi)** :
  - « Choisir » donne la couleur historique choisie à **cette voiture seulement**, avec une ligne `boutique : PROVISOIRE (jusqu'à WFS-489)` au journal ;
  - tout est dans une seule fonction, `perso::donGarageProvisoire`, appelée à un seul endroit ;
  - le garage et la course restent donc ce que le joueur voit aujourd'hui ;
  - les 12 nouvelles couleurs ne sont jamais données ;
  - WFS-489 retire cet appel quand il pose le cadenas.
- **Banc `WFS_BANC=sauvegarde`** :
  - 1er lancement : achat d'essai avec un solde fictif de 250 ★ (rose sur la M4 acheté pour 100 ★, chromé à 800 ★ refusé) ;
  - 2e lancement : le journal donne le catalogue lu (28 couleurs, 8 finitions, 4 skins, 5 packs, coefficients) et les objets possédés.
- `CMakeLists.txt` : `personnalisation.cpp` est ajouté aux 14 tests qui lient `sauvegarde.cpp`, et il y a un nouveau test, `test_personnalisation`.

## Comment c'est vérifié
- **`test_personnalisation`** (nouveau, 40 cas, tous OK) couvre les cas de RG-08 :
  - sauvegarde d'avant : turquoise gardée, gratuite, en course ;
  - 8 couleurs gratuites et 20 à 50 ★ de base ; Golf 75 ★, SF90 200 ★ ;
  - couleur achetée sur la Golf absente sur la M4 ;
  - achat avec solde suffisant, avec solde insuffisant (rien de débité), déjà possédé (aucun débit) ;
  - couleur d'origine gratuite ;
  - aspect en course jamais non possédé ;
  - `configgame.json` mal écrit (prix négatif, liste vide) : valeur du code + message ;
  - prix changé sans recompiler (rose à 80 → 120 ★ sur la Golf) ;
  - don provisoire du garage (cette voiture seulement, jamais une couleur nouvelle) ;
  - exemples de WFS-498 RG-05 (métallisé SF90 400 ★, candy Golf 375 ★).
- `test_configgame` OK (245 clés, 0 erreur, `configgame.json` du dépôt lu en entier). `test_sauvegarde` OK.
- **Tous les tests CPU** : voir « Tests CPU » ci-dessous.
- **Captures** :
  - garage avant / après sur la Golf, la M4 et la 911 (identiques), et 2 courses avec la couleur choisie ;
  - vitrine du banc `sauvegarde` de jour :
    - (a) sauvegarde d'avant le ticket avec `paint-m4=turquoise` → M4 turquoise, avant comme après ;
    - (b) sauvegarde au nouveau format, turquoise jamais achetée → M4 dans sa couleur d'origine (noir) après : c'est RG-07 ;
  - texte du banc dans `qa/pr/WFS-487/banc-sauvegarde.txt`.
- **Parcours complet** (`WFS_BANC=parcours`, branche) : accueil → mode → garage → « Choisir » → terrain → course → fin → « Menu » → accueil : **0 écart**, en vue Poursuite et en vue Cockpit.

## Limites et risques
- **Pas encore de portefeuille d'étoiles** (WFS-498, ordre de l'utilisateur du 2026-11-12 23:20) : `acheter()` reçoit le solde à débiter, WFS-498 y branchera le sien. Avant cela, aucun écran n'achète : une couleur bloquée reste bloquée.
- **Accroches hors de la zone E3** :
  - `src/game/game.cpp` : 1 ligne ;
  - `src/core/configgame.*` : section `boutique` seulement, même exception que WFS-498 ;
  - `CMakeLists.txt` : lignes de liens.
- `sauvegarde.*` est aussi touché par WFS-480 et WFS-504 : la branche est synchronisée avec `main` juste avant la PR, et seules des clés sont ajoutées.
- Les noms des objets sont en français. L'anglais viendra avec les écrans (WFS-504, 489…).
- La couleur secondaire, les finitions et les skins ne se voient pas encore sur les modèles (WFS-488, 490, 492) : seules les données existent.
- RC-E18-02 et 03 (mémoire, images/s) : rien n'est chargé ni dessiné en plus. `aspectCourse` est appelé au chargement de la voiture (`setCar`), jamais à chaque image.

## Tests CPU

Branche synchronisée avec `main` (synchronisation 66fda6c9, + correction du garage, ligne 190), `bash tools/qa/tests_cpu.sh build` ; journal complet : [`qa/pr/WFS-487/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/tests-cpu.log).

```
[OK] test_ville_lieux (0 s)
[OK] test_ville_sol (3 s)
[OK] test_virages_fleches (1 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 116 test(s) réussi(s) en 442 s
code de sortie : 0
```

## Captures pour le lead dev

Dossier : [`qa/pr/WFS-487`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487) — `tools/qa/captures_pr.sh WFS-487 garage` (14 captures avant / après, **identiques au pixel près** : `compare -metric AE` = 0 pour les 14), plus 2 vitrines du banc `sauvegarde` de jour (avant `main` / après branche). Texte du banc : [`banc-sauvegarde.txt`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/banc-sauvegarde.txt).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `sauvegarde-ancien-jour` — sauvegarde d'avant le ticket, paint-m4=turquoise : gardée (RG-03) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-sauvegarde-ancien-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-sauvegarde-ancien-jour.jpg?raw=true) |
| `sauvegarde-nouveau-jour` — nouveau format, turquoise jamais achetée : couleur d'origine en course (RG-07) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-sauvegarde-nouveau-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-sauvegarde-nouveau-jour.jpg?raw=true) |
| `course-p911-gris` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-course-p911-gris.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-course-p911-gris.jpg?raw=true) |
| `garage-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-garage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-garage-0.jpg?raw=true) |
| `garage-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-garage-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-garage-1.jpg?raw=true) |
| `garage-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-garage-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-garage-2.jpg?raw=true) |
| `garage-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-garage-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-garage-course.jpg?raw=true) |
| `garage-m4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-garage-m4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-garage-m4.jpg?raw=true) |
| `garage-peinture-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-garage-peinture-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-garage-peinture-0.jpg?raw=true) |
| `garage-peinture-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-garage-peinture-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-garage-peinture-1.jpg?raw=true) |
| `garage-peintures-p911` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-garage-peintures-p911.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-garage-peintures-p911.jpg?raw=true) |
| `garage-peintures` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-garage-peintures.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-garage-peintures.jpg?raw=true) |
| `garage-suivant` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-garage-suivant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-garage-suivant.jpg?raw=true) |
| `showroom-suivante-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-showroom-suivante-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-showroom-suivante-1.jpg?raw=true) |
| `showroom-suivante-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-showroom-suivante-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-showroom-suivante-2.jpg?raw=true) |
| `showroom-suivante-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/avant-showroom-suivante-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-487/qa/pr/WFS-487/apres-showroom-suivante-3.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01Us6uCwB3PrvQFZ2ZBBwHSv
