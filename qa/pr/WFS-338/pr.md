## Agent : agent arwa (Claude, session trafic)

**Ticket** : WFS-338 — règles : `regles-de-gestion/WFS-338.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : en Poursuite, la police ne doit plus surgir de nulle part ni rouler vide ; l'encart, le score et le bandeau « TRAVAUX » doivent dire vrai (ANO-191 à 193, 195 à 197).
- **Ce qui change** : voir « Causes et corrections » ci-dessous.
- **Comment c'est vérifié** : tests CPU (tous OK), `test_poursuite` (nouveaux contrôles RG-01/02/03/05), banc `course5` (parcours complet 0 écart), captures avant / après, appels de dessin branche / `main` au même endroit.
- **Limites et risques connus** : rétroviseur du cockpit = WFS-324 (décision du PO) ; la voiture qui déboîte reste créée 45 m devant (hors RG-05, qui vise la police) ; le gros plan du policier est un agrandissement de la capture `poursuite-policier-jour-2`.

## Causes et corrections (WFS-338, ANO-191 à 193, 195 à 197)

| RG | Cause trouvée | Correction |
|---|---|---|
| RG-05 (ANO-195, bloquante) | Police dessinée seulement jusqu'à 60 m derrière ; renfort créé à 90 m ; retour « trop loin » de 250 m à 120 m ; rapprochement jusqu'à +45 m/s même au contact ; voiture « à côté » ramenée sur le joueur quand il roule sur une file du bord (côté hors chaussée borné au bord) ; poussée visée à 2 m (moitié dans la voiture) ; deux polices trop proches : recul de 7 m d'un coup | Dessin jusqu'à **160 m derrière** ; renfort et retour à **220 m** (hors du rayon de dessin, donc hors de toute vue) ; rapprochement borné **4 m/s + 0,3 × distance** (45 au plus) ; côté impossible → l'autre côté, sinon derrière ; jamais dans la voiture du joueur (largeur gardée à côté, pare-chocs contre pare-chocs derrière) ; recul de 0,3 m par pas au plus |
| RG-02 (ANO-192) | Le chiffre de l'encart POLICE était la distance à l'arrivée | Distance à la police la plus proche (« 45 M », « 1,20 KM », « — ») ; même police, même taille |
| RG-03 (ANO-193) | Le score perd 1 point par seconde quand une police est à moins de 12 m (et à chaque choc), sans rien d'écrit | Chaque perte garde sa raison ; écrite sous l'encart 2 s (« -1 POLICE AU CONTACT », « -5 CHOC AVEC LE TRAFIC », « -3 CÔNE DE CHANTIER »…), cumulée si la raison ne change pas |
| RG-01 (ANO-191) | 12 cubes orange **lumineux** en ligne (les « blocs lumineux »), créés 260 m avant avec le bandeau, dessinés à 250 m | Vrai chantier : panneau AK5 sur pied, biseau de 7 cônes K5a (75 cm, socle noir, bandes blanches, sans lumière propre, un seul dessin par cône) qui barre la file, cônes tous les 10 m sur 120 m ; posé 450 m avant (hors de vue), bandeau à 80 m (panneau et cônes visibles), tenu jusqu'au biseau. Barrage, accident, camion, ralentisseur aussi posés 450 m avant |
| RG-06 (ANO-196) | Aucun appel à `conducteurs::ajouter` pour la police | Policier en uniforme bleu marine au volant de chaque voiture ; prioritaire quand la file des conducteurs est pleine (trafic dense) |
| RG-07 (ANO-197) | Deux boîtes rouge / bleue sous le haut du modèle (traversaient le toit) ; une voiture sur deux bleu marine, sans marquage | Caisse blanche, bandes bleues « POLICE » avec liseré blanc et rouge sur les portières (texture faite par le code, aucune marque) ; rampe posée sur le pavillon mesuré : socle gris 1 m × 30 cm, deux feux **bleus** qui clignotent l'un après l'autre (2 Hz) |

**Fichiers touchés** : `src/game/poursuite.h`, `src/game/poursuite.cpp`, `src/game/poursuite_system.cpp`, `tests/test_poursuite.cpp`, `journal.md`.
**Accroche hors zone (annoncée au journal, ≤ 10 lignes)** : `src/world/conducteurs.h/.cpp` (paramètre `policier`). Aucun fichier `traffic*`, `game.cpp`, `track.*`, `camera_vue.*`, `player.*`, `conduite_*`, `vehicule_*`, `hud.*`.
**Rétroviseur** : WFS-324 (décision du PO) ; la police se prouve en vue Arrière du jeu (`poursuite-arrivee-NN`) et en vue arrière du banc (`poursuite-arrivee-NN-arriere`).

**Tests CPU** : `bash tools/qa/tests_cpu.sh build` : 85 tests OK ; `test_course5` (poursuite) : nouveaux contrôles RG-05 (0 saut > 1 m par pas dans la zone dessinée, 0 rapprochement trop rapide, 0 pas dans la voiture du joueur, 4 positions du joueur), RG-02, RG-03 (7 raisons), RG-01 (biseau qui ferme la file, 3 files).
**Parcours complet** (banc course5) : « accueil → Course solo → Poursuite → Continuer → garage → Choisir → terrain → Rouler → poursuite → fin → Menu → accueil » et redémarrage : **0 écart**.
**Suite d'images de la police qui arrive (revue 1, point 2)** : toute la police repart à 220 m (aucune à moins de 160 m, rayon de dessin), puis `poursuite-arrivee-01` à `-10`, une par seconde : **220, 197, 152, 110, 78, 54, 36, 23, 13, 6 m** (chiffre de l'encart POLICE sur chaque image) ; vue Arrière du jeu et vue arrière du banc (`-arriere`).
**Chantier (point 3)** : bandeau à 80 m, tenu jusqu'au biseau ; `poursuite-travaux(-proche)` de nuit et `poursuite-travaux-jour(-proche)` de jour : panneau AK5, biseau de cônes et bandeau sur la même image.
**Policier (point 4)** : `poursuite-policier-jour`, `-jour-2` et l'agrandissement ![policier](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-policier-jour-gros-plan.jpg?raw=true)

**Appels de dessin en Basse (point 5, décision du PO 0161316e)**, même banc, même machine, mêmes endroits :

| Endroit | main : poursuite / Checkpoint | branche : poursuite / Checkpoint | branche − main | même image avec / sans WFS-338 |
|---|---|---|---|---|
| police au contact | 187 / 155 (+32) | 187 / 178 (+9) | 0 | 184 / 184 |
| embouteillage | 345 / 312 (+33) | 341 / 304 (+37) | −4 | 341 / 332 |
| HUD | 336 / 312 (+24) | 343 / 304 (+39) | +7 | 342 / 333 |
| ralentisseur | 230 / 224 (+6) | 211 / 183 (+28) | −19 | 195 / 195 |
| camion | 371 / 325 (+46) | 328 / 333 (−5) | −43 | 255 / 255 |
| écart maximal (≤ 60) | +46 OK | **+39 OK** | | |

WFS-338 ajoute au plus +9 appels dans la même image (≤ +10, cas a du PO). Les « +128 » d'avant venaient d'un banc de `main` bloqué au barrage, à d'autres endroits.

## Captures pour le lead dev (36 après)

Dossier : [`qa/pr/WFS-338`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `poursuite-arrete` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-arrete.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrete.jpg?raw=true) |
| `poursuite-arrivee-01-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-01-arriere.jpg?raw=true) |
| `poursuite-arrivee-01` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-01.jpg?raw=true) |
| `poursuite-arrivee-02-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-02-arriere.jpg?raw=true) |
| `poursuite-arrivee-02` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-02.jpg?raw=true) |
| `poursuite-arrivee-03-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-03-arriere.jpg?raw=true) |
| `poursuite-arrivee-03` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-03.jpg?raw=true) |
| `poursuite-arrivee-04-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-04-arriere.jpg?raw=true) |
| `poursuite-arrivee-04` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-04.jpg?raw=true) |
| `poursuite-arrivee-05-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-05-arriere.jpg?raw=true) |
| `poursuite-arrivee-05` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-05.jpg?raw=true) |
| `poursuite-arrivee-06-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-06-arriere.jpg?raw=true) |
| `poursuite-arrivee-06` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-06.jpg?raw=true) |
| `poursuite-arrivee-07-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-07-arriere.jpg?raw=true) |
| `poursuite-arrivee-07` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-07.jpg?raw=true) |
| `poursuite-arrivee-08-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-08-arriere.jpg?raw=true) |
| `poursuite-arrivee-08` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-08.jpg?raw=true) |
| `poursuite-arrivee-09-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-09-arriere.jpg?raw=true) |
| `poursuite-arrivee-09` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-09.jpg?raw=true) |
| `poursuite-arrivee-10-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-10-arriere.jpg?raw=true) |
| `poursuite-arrivee-10` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-10.jpg?raw=true) |
| `poursuite-bouchon` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-bouchon.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-bouchon.jpg?raw=true) |
| `poursuite-camion` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-camion.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-camion.jpg?raw=true) |
| `poursuite-hud` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-hud.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-hud.jpg?raw=true) |
| `poursuite-police-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-police-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-police-basse.jpg?raw=true) |
| `poursuite-police` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-police.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-police.jpg?raw=true) |
| `poursuite-policier-jour-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-policier-jour-2.jpg?raw=true) |
| `poursuite-policier-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-policier-jour.jpg?raw=true) |
| `poursuite-ralentisseur` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-ralentisseur.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-ralentisseur.jpg?raw=true) |
| `poursuite-redemarrage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-redemarrage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-redemarrage.jpg?raw=true) |
| `poursuite-surprise` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-surprise.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-surprise.jpg?raw=true) |
| `poursuite-travaux-jour-proche` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-travaux-jour-proche.jpg?raw=true) |
| `poursuite-travaux-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-travaux-jour.jpg?raw=true) |
| `poursuite-travaux-proche` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-travaux-proche.jpg?raw=true) |
| `poursuite-travaux` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-travaux.jpg?raw=true) |
| `poursuite-victoire` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-victoire.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-victoire.jpg?raw=true) |
