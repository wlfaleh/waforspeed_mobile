Agent : Claude, agent dev3d (01Us6uCwfT) (session_01Us6uCwfTwaie75MYviBP8W)

**Ticket** : WFS-491 — règles : `regles-de-gestion/WFS-491.md` (épic E18, RC-E18-01 à 11 : `regles-de-gestion/E18-personnalisation.md`)

## Description

- **Demande de l'utilisateur** (en une phrase) : au garage, choisir la finition de la peinture (brillant, satiné, mat, métallisé, nacré, candy, chromé, caméléon), voir son prix en ★ (100 à 400 × coefficient de la classe), la voir tout de suite sur la voiture qui tourne, l'acheter pour cette voiture seulement, et la retrouver en course et après une relance.
- **Base** : WFS-489 (PR #395, fusionnée sur `main` pendant ce ticket) : son bouton « Acheter N ★ » est repris tel quel (consigne du PO : pas de copie). Branche resynchronisée avec `main` après sa fusion : le code de WFS-489 sur `main` est identique à celui sur lequel les captures ont été faites (`git diff 07099d25 origin/main -- src tests` vide).
- **Ce qui change** :
  - `src/ui/garage_finition.*` (panneau « Finition », zone du ticket) : rangée **« Finition »** sous les couleurs, **8 boutons de même taille** (30 px de haut, comme les pastilles) dans l'ordre de RG-01 : Brillant, Satiné, Mat, Métallisé, Nacré, Candy, Chromé, Caméléon. Chaque bouton porte un petit **échantillon 2D** de la finition sur la couleur montrée ; non possédée : **cadenas + prix en ★** dessous (`perso::prix` : base de `configgame.json` × coefficient de la classe : 400 ★ sur la 500, 600 ★ sur la Golf, 800 ★ sur la Giulia pour le Caméléon). À droite : nom de la finition et, si elle est payante, **« 600 ★ = 1 or = 5,94 € »** (RC-E18-11). Un appui la montre **tout de suite sur la voiture** (aperçu, RG-02) ; choix possédé gardé tout de suite (`finition-<voiture>`, RG-03) ; à chaque ouverture du garage, chaque voiture reprend sa **dernière finition possédée** (aperçu non acheté oublié). Le texte des boutons se réduit seul (12 → 8 px) s'il ne tient pas : aucun texte ne dépasse (RG-04).
  - **Achat** (reprise de WFS-489, sans copie) : `garage_achat_prix.cpp` ajoute la finition montrée à « ce qui manque » (4 objets au plus au lieu de 3) ; `garage_achat.cpp` garde la finition achetée (`finition-<voiture>`) avec la couleur. Même bouton « Acheter N ★ », même refus « Il te manque N ★ », **rien n'est débité** sans assez d'étoiles.
  - **En course** : `Game::setCar` pose la finition **possédée** de la voiture du joueur (`perso::aspectCourse` : une finition non achetée ne part jamais en course) sur `Renderer::finitionJoueur` (rendu de WFS-490) ; au garage, la finition montrée est posée seulement le temps du dessin de la voiture, puis celle de la course est remise.
  - Textes FR / EN : `garage.finition` (Finition / Finish), `finition.<id>` (Gloss, Satin, Matte, Metallic, Pearl, Candy, Chrome, Chameleon).
  - **Accroches annoncées au journal** : `garage.cpp` 7 lignes (ouverture, dessin de la voiture avec la finition, toucher, état, hauteur du panneau) ; `garage_commun.h` 4 lignes (`EtatGarage::finition`, `g_z.finitions`, déclaration du cadenas) ; `garage_couleur.cpp` 2 lignes (le cadenas de WFS-489 sort de l'espace anonyme pour être partagé, pas de copie) ; `game.cpp` 3 lignes ; `vehicule_finitions.h` 1 ligne (identifiant `nacree` du catalogue = `NACRE`) ; `desktop_main.cpp`, `garage.h` 1 ligne chacun (banc) ; `CMakeLists.txt` 1 ligne (test).
- **Comment c'est vérifié** :
  - nouveau test CPU **`test_garage_finition`** (16 cas : ordre de RG-01, lien avec le rendu, prix par classe, brillant gratuit, ce qui manque avec la finition, refus sans débit, achat débité une fois, finition achetée pour la Golf toujours payante sur la Giulia) ; `test_garage_achat` et `test_langue` toujours verts ;
  - nouveau banc **`WFS_BANC=garage-finitions`** (`src/ui/garage_finition_banc.cpp`), vraie sauvegarde du banc, vrais touchers : Golf Caméléon en aperçu sous 3 angles, **refus (100 ★ pour 600 ★ : rien de débité)**, achat ; Giulia **Mat**, 500 **Chromé** ; Golf Candy en aperçu puis « Retour » (la Golf reste Caméléon) ; **parcours RC-E18-09** : 500 « Choisir » → terrain → « Rouler » → course en Chromé → retour au garage ; **relance du jeu** (sauvegarde relue : tout gardé) ; courses de jour et de nuit des 3 voitures avec la finition achetée ; anglais : **0 écart**.
- **Limites et risques connus** :
  - le rendu des finitions lui-même (aspect du caméléon, du chromé…) est celui de WFS-490, hors de ce ticket ;
  - RC-E18-03 (images/s en Basse) : la course ne fait qu'un appel de plus au chargement de la voiture (`finitionJoueur`), rien par image ; pas de mesure d'endurance faite ;
  - aucune confirmation avant l'achat (comme les couleurs de WFS-489) ;
  - les captures « avant » viennent de `main` **avant** la fusion de WFS-489 (garage aux 16 couleurs) : l'écart avant / après de `garage-*` montre donc WFS-489 **et** WFS-491 ; seule la rangée « Finition » est de ce ticket ;
  - le panneau du garage est plus haut d'une rangée (54 px à 720 px de haut) : la voiture reste entièrement visible au-dessus (captures).

## Tests CPU

Journal complet : `qa/pr/WFS-491/tests-cpu.log` (branche synchronisée avec `main`, avant l'arrivée de WFS-489 sur `main` ; code du garage identique).

```
[OK] test_garage_achat
[OK] test_garage_finition
[OK] test_langue
tests CPU : 131 test(s) réussi(s) en 466 s
code de sortie : 0
```

Banc `WFS_BANC=garage-finitions` : `garage-finitions (WFS-491) : 0 écart(s)` (aperçu, refus, achat, une voiture seulement, « Retour », parcours Choisir → Rouler → course → garage, relance, 6 courses jour / nuit, anglais).

## Captures pour le lead dev (36 après)

Dossier : [`qa/pr/WFS-491`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `garage-finitions-abarth-chrome` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-abarth-chrome.jpg?raw=true) |
| `garage-finitions-course-abarth-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-course-abarth-jour.jpg?raw=true) |
| `garage-finitions-course-abarth-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-course-abarth-nuit.jpg?raw=true) |
| `garage-finitions-course-giulia-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-course-giulia-jour.jpg?raw=true) |
| `garage-finitions-course-giulia-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-course-giulia-nuit.jpg?raw=true) |
| `garage-finitions-course-golf-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-course-golf-jour.jpg?raw=true) |
| `garage-finitions-course-golf-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-course-golf-nuit.jpg?raw=true) |
| `garage-finitions-en-golf-nacre-apercu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-en-golf-nacre-apercu.jpg?raw=true) |
| `garage-finitions-en-golf` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-en-golf.jpg?raw=true) |
| `garage-finitions-en-il-manque` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-en-il-manque.jpg?raw=true) |
| `garage-finitions-giulia-mat-apercu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-giulia-mat-apercu.jpg?raw=true) |
| `garage-finitions-giulia-mat` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-giulia-mat.jpg?raw=true) |
| `garage-finitions-golf-cameleon-apercu-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-golf-cameleon-apercu-1.jpg?raw=true) |
| `garage-finitions-golf-cameleon-apercu-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-golf-cameleon-apercu-2.jpg?raw=true) |
| `garage-finitions-golf-cameleon-apercu-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-golf-cameleon-apercu-3.jpg?raw=true) |
| `garage-finitions-golf-cameleon` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-golf-cameleon.jpg?raw=true) |
| `garage-finitions-golf-candy-apercu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-golf-candy-apercu.jpg?raw=true) |
| `garage-finitions-golf-debut` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-golf-debut.jpg?raw=true) |
| `garage-finitions-golf-il-manque` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-golf-il-manque.jpg?raw=true) |
| `garage-finitions-parcours-abarth-course` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-parcours-abarth-course.jpg?raw=true) |
| `garage-finitions-parcours-abarth-retour-garage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-parcours-abarth-retour-garage.jpg?raw=true) |
| `garage-finitions-relance-golf` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-finitions-relance-golf.jpg?raw=true) |
| `course-p911-gris` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/avant-course-p911-gris.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-course-p911-gris.jpg?raw=true) |
| `garage-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/avant-garage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-0.jpg?raw=true) |
| `garage-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/avant-garage-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-1.jpg?raw=true) |
| `garage-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/avant-garage-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-2.jpg?raw=true) |
| `garage-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/avant-garage-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-course.jpg?raw=true) |
| `garage-m4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/avant-garage-m4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-m4.jpg?raw=true) |
| `garage-peinture-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/avant-garage-peinture-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-peinture-0.jpg?raw=true) |
| `garage-peinture-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/avant-garage-peinture-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-peinture-1.jpg?raw=true) |
| `garage-peintures-p911` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/avant-garage-peintures-p911.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-peintures-p911.jpg?raw=true) |
| `garage-peintures` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/avant-garage-peintures.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-peintures.jpg?raw=true) |
| `garage-suivant` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/avant-garage-suivant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-garage-suivant.jpg?raw=true) |
| `showroom-suivante-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/avant-showroom-suivante-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-showroom-suivante-1.jpg?raw=true) |
| `showroom-suivante-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/avant-showroom-suivante-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-showroom-suivante-2.jpg?raw=true) |
| `showroom-suivante-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/avant-showroom-suivante-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-491/qa/pr/WFS-491/apres-showroom-suivante-3.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01Us6uCwfTwaie75MYviBP8W
