Agent : Claude, agent dev3d (01Us6uCv9g)

**Ticket** : WFS-502 (épic E19 « Cadeau du jour ») — règles : `regles-de-gestion/WFS-502.md` et `regles-de-gestion/E19-cadeau-du-jour.md`

## Demande de l'utilisateur
Quand le cadeau du jour donne une voiture que le joueur n'a pas, il a 24 h pour l'activer. Il la conduit ensuite 3 h, dans tous les modes, avec un compte à rebours au garage et au menu. À la fin, le jeu lui propose : « Garde-la pour toujours : <prix> ★ ».

## Ce qui change
- **Logique du prêt** (E3), nouveaux fichiers `src/game/pret.h` et `src/game/pret.cpp` :
  - états du prêt : aucun, en attente, actif ;
  - activation avant la fin du délai (`cadeau.activation_heures`), puis prêt de `cadeau.pret_heures` (valeurs de `configgame.json`, rien en dur) ;
  - **RG-04** : le temps restant ne peut jamais augmenter. Le jeu garde le dernier temps restant vu ; si l'heure du téléphone recule, la fin du prêt est recalée ;
  - **RG-05** : à la fin, la voiture redevient non possédée et le choix revient à la voiture d'avant. Le message de fin est montré une fois.
- **Système `pret`** (`src/game/pret_system.cpp`, ordre 206) : il applique la perte et la fin du prêt **hors course seulement**. Une course commencée se termine normalement et ses gains sont comptés. À la fin du prêt, la partie reprend la voiture d'avant.
- **Tirage** (`cadeau.cpp`) :
  - le prêt est écrit dans la même sauvegarde que le cadeau ;
  - **un seul prêt à la fois** (RG-01) : pendant un prêt en attente ou actif, le tirage ne donne aucune voiture. Le test de WFS-500 qui attendait 13 voitures à prêter est mis à jour en conséquence.
- **Sauvegarde** : nouvelles clés `pret-restant-vu`, `pret-avant` et `pret-message`. Une sauvegarde d'avant se lit sans erreur, et une valeur invalide ne remet à zéro que sa propre clé.
- **Garage** (E5) :
  - une voiture en prêt actif n'est plus bloquée : on peut la choisir et rouler avec dans tous les modes ;
  - carte « Voiture prêtée · Prêtée : 2 h 41 min · Garde-la pour toujours : 2 400 ★ » ;
  - pour une voiture prêtée en attente, le bouton « Activer le prêt » remplace « Essai gratuit », et la carte affiche « À activer dans 23 h 13 min » ;
  - accroche `garageOuvrirSur` : le garage s'ouvre sur la voiture prêtée.
- **Menu** (E8) : nouveau bandeau `src/ui/ecrans/pret_bandeau.*` au-dessus du bouton « Profil » de WFS-482 (après le retour du lead dev ; aucun bouton existant ne bouge). Il a 4 cas :
  - en attente : « Activer » ;
  - prêt actif : compte à rebours, et un appui ouvre le garage ;
  - prêt expiré : message montré une fois ;
  - fin du prêt : « Garde-la pour toujours : <prix> ★ » et un bouton « Garage ».
- **Écran du cadeau** : « Activer » lance vraiment le prêt. Le bouton ne s'affiche que tant que le prêt attend.
- **Essai gratuit** (accroche dans `profil_system.cpp`) : rouler avec la voiture prêtée n'utilise pas son essai gratuit de WFS-498. Ce défaut a été vu sur les captures, puis corrigé.
- **Textes** FR et EN (`pret.*`, 14 clés dans chaque table).
- **Accroches** (≤ 10 lignes chacune, annoncées au journal) : `menu.cpp` (4 lignes, `PRET = 8` après `PROFIL = 7`), `garage.cpp` (4), `garage.h` (2), `profil_system.cpp` (2), `cadeau_jour.cpp`.

## Comment c'est vérifié
- **Nouveau test `test_pret`** (RG-08, 39 cas), qui couvre :
  - 24 h + 1 s sans activer : le prêt est perdu ;
  - à 3 h − 1 min la voiture est utilisable ; à 3 h + 1 s elle ne l'est plus et le choix revient à la voiture d'avant ;
  - heure reculée de 1 h : le temps restant n'augmente pas ;
  - course commencée à 3 h − 10 s : les gains sont comptés ;
  - relance du jeu : l'état est gardé ;
  - sauvegarde ancienne ou invalide ;
  - un seul prêt à la fois.
- **`test_garage_voitures`** : voiture prêtée sans cadenas pendant le prêt, de nouveau bloquée à la fin, et durée affichée en FR et en EN.
- **Banc `WFS_BANC=cadeau`** (`WFS_CADEAU=pret-attente|pret-actif|pret-fin`) : parcours complet RG-10 au doigt, en FR et en EN, sur une sauvegarde du banc. Il va du menu au cadeau, au garage (« Activer le prêt »), à « Choisir », au menu, à la course de jour et de nuit, puis à la fin du prêt pendant la course (voiture gardée jusqu'au retour), au menu « Garde-la », au garage, au prêt expiré et à la relance. Résultat : **0 erreur**, garde-fou des textes de WFS-504 VERT dans les deux langues.
- Les captures de WFS-501 (banc `cadeau`) sont inchangées par rapport à `main` : voir le tableau.

## Limites et risques
- Le compte à rebours suit l'heure du téléphone, protégée contre le recul (RG-04). Si l'heure **avance**, le prêt raccourcit, comme pour le cadeau du jour : c'est le comportement voulu, le joueur ne gagne jamais de temps.
- « Rejouer » depuis l'écran de fin, quand le prêt est fini pendant la course : la course suivante repart avec la voiture prêtée, jusqu'au prochain passage par le menu. C'est le même enchaînement que la course commencée (RG-05), à faire confirmer par l'utilisateur sur téléphone.
- Le bouton « Market » de la RG-05 arrivera avec le market. Pour l'instant, c'est le bouton « Garage ».
- Testé avec le rendu logiciel (lavapipe), comme la CI. Sur téléphone : à valider par l'utilisateur (🟡 après la fusion).

## Tests CPU

Commit testé : `ca13d7cb` (branche synchronisée avec `main`, WFS-482 compris) ; journal complet : `qa/pr/WFS-502/tests-cpu.log`.

```
[OK] test_vehicule_skin (0 s)
[OK] test_version_chargement (0 s)
[OK] test_ville_lieux (0 s)
[OK] test_ville_sol (4 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 141 test(s) réussi(s) en 497 s
code de sortie : 0
```

## Lecture des captures
- `pret-*` : nouvelles captures (aucun prêt n'existe sur `main`), FR et EN : gain « voiture prêtée » avec « Activer » ; menu en attente ; garage « Activer le prêt » ; garage actif « Prêtée : 2 h 41 min » ; menu actif ; course jour et nuit avec la voiture prêtée ; menu « Garde-la pour toujours : 2 400 ★ » + « Garage » ; garage après la fin (voiture de nouveau bloquée, prix) ; menu « Prêt expiré ». Sur les 4 menus, le bandeau est **au-dessus de « Profil »**, qui reste visible et libre.
- Menu sans prêt, avant / après (même `main` avec « Profil ») : `cadeau-menu-*` (identiques).
- `cadeau-*` (WFS-501) : 28 paires avant / après **identiques au pixel près** (écart RMSE 0, `compare` d'ImageMagick).

## Captures pour le lead dev (48 après)

Dossier : [`qa/pr/WFS-502`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `cadeau-attente-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-attente-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-attente-en.jpg?raw=true) |
| `cadeau-attente-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-attente-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-attente-fr.jpg?raw=true) |
| `cadeau-chances-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-chances-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-chances-en.jpg?raw=true) |
| `cadeau-chances-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-chances-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-chances-fr.jpg?raw=true) |
| `cadeau-coloris-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-coloris-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-coloris-en.jpg?raw=true) |
| `cadeau-coloris-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-coloris-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-coloris-fr.jpg?raw=true) |
| `cadeau-etoiles-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-etoiles-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-etoiles-en.jpg?raw=true) |
| `cadeau-etoiles-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-etoiles-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-etoiles-fr.jpg?raw=true) |
| `cadeau-etoiles-nuit-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-etoiles-nuit-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-etoiles-nuit-en.jpg?raw=true) |
| `cadeau-etoiles-nuit-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-etoiles-nuit-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-etoiles-nuit-fr.jpg?raw=true) |
| `cadeau-menu-attente-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-menu-attente-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-menu-attente-en.jpg?raw=true) |
| `cadeau-menu-attente-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-menu-attente-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-menu-attente-fr.jpg?raw=true) |
| `cadeau-menu-bientot-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-menu-bientot-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-menu-bientot-en.jpg?raw=true) |
| `cadeau-menu-bientot-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-menu-bientot-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-menu-bientot-fr.jpg?raw=true) |
| `cadeau-menu-nuit-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-menu-nuit-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-menu-nuit-en.jpg?raw=true) |
| `cadeau-menu-nuit-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-menu-nuit-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-menu-nuit-fr.jpg?raw=true) |
| `cadeau-menu-pastille-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-menu-pastille-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-menu-pastille-en.jpg?raw=true) |
| `cadeau-menu-pastille-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-menu-pastille-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-menu-pastille-fr.jpg?raw=true) |
| `cadeau-ouverture-1-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-ouverture-1-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-ouverture-1-en.jpg?raw=true) |
| `cadeau-ouverture-1-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-ouverture-1-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-ouverture-1-fr.jpg?raw=true) |
| `cadeau-ouverture-2-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-ouverture-2-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-ouverture-2-en.jpg?raw=true) |
| `cadeau-ouverture-2-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-ouverture-2-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-ouverture-2-fr.jpg?raw=true) |
| `cadeau-ouverture-3-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-ouverture-3-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-ouverture-3-en.jpg?raw=true) |
| `cadeau-ouverture-3-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-ouverture-3-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-ouverture-3-fr.jpg?raw=true) |
| `cadeau-skin-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-skin-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-skin-en.jpg?raw=true) |
| `cadeau-skin-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-skin-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-skin-fr.jpg?raw=true) |
| `cadeau-voiture-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-voiture-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-voiture-en.jpg?raw=true) |
| `cadeau-voiture-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/avant-cadeau-voiture-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-cadeau-voiture-fr.jpg?raw=true) |
| `pret-course-jour-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-course-jour-en.jpg?raw=true) |
| `pret-course-jour-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-course-jour-fr.jpg?raw=true) |
| `pret-course-nuit-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-course-nuit-en.jpg?raw=true) |
| `pret-course-nuit-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-course-nuit-fr.jpg?raw=true) |
| `pret-gain-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-gain-en.jpg?raw=true) |
| `pret-gain-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-gain-fr.jpg?raw=true) |
| `pret-garage-actif-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-garage-actif-en.jpg?raw=true) |
| `pret-garage-actif-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-garage-actif-fr.jpg?raw=true) |
| `pret-garage-attente-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-garage-attente-en.jpg?raw=true) |
| `pret-garage-attente-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-garage-attente-fr.jpg?raw=true) |
| `pret-garage-fin-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-garage-fin-en.jpg?raw=true) |
| `pret-garage-fin-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-garage-fin-fr.jpg?raw=true) |
| `pret-menu-actif-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-menu-actif-en.jpg?raw=true) |
| `pret-menu-actif-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-menu-actif-fr.jpg?raw=true) |
| `pret-menu-attente-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-menu-attente-en.jpg?raw=true) |
| `pret-menu-attente-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-menu-attente-fr.jpg?raw=true) |
| `pret-menu-expire-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-menu-expire-en.jpg?raw=true) |
| `pret-menu-expire-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-menu-expire-fr.jpg?raw=true) |
| `pret-menu-fin-en` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-menu-fin-en.jpg?raw=true) |
| `pret-menu-fin-fr` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-502/qa/pr/WFS-502/apres-pret-menu-fin-fr.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01Us6uCv9gWscPZKVN4ymQn9
