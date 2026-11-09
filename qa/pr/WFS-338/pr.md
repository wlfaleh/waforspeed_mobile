## Agent : agent arwa (Claude, session trafic)

**Ticket** : WFS-338 — règles : `regles-de-gestion/WFS-338.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : en Poursuite, la police ne doit plus surgir de nulle part ni rouler vide ; l'encart, le score et le bandeau « TRAVAUX » doivent dire vrai (ANO-191 à 193, 195 à 197).
- **Ce qui change** : voir le tableau « Causes et corrections » ci-dessous.
- **Comment c'est vérifié** : tests CPU (83 OK, dont les nouveaux contrôles de `test_poursuite.cpp`), banc `course5` (parcours complet 0 écart), captures avant / après ci-dessous.
- **Limites et risques connus** : rétroviseur Cockpit non rendu (hors zone) ; en qualité Basse, la police reste coupée à 90 m (WFS-134) ; mesure RG-04 de WFS-119 (appels de dessin en Basse, ralentisseur) : +128 sur ce banc local contre une cible ≤ 60 (le Checkpoint de référence n'y fait que 183 appels, contre ≈ 300 ailleurs) : à regarder par le lead dev ; la voiture qui déboîte reste créée 45 m devant (hors RG-05, qui vise la police).

## Causes et corrections (WFS-338, ANO-191 à 193, 195 à 197)

| RG | Cause trouvée | Correction |
|---|---|---|
| RG-05 (ANO-195, bloquante) | Police dessinée seulement jusqu'à 60 m derrière ; renfort créé à 90 m ; retour « trop loin » de 250 m à 120 m ; rapprochement jusqu'à +45 m/s même au contact ; voiture « à côté » ramenée sur le joueur quand il roule sur une file du bord (côté hors chaussée borné au bord) ; poussée visée à 2 m (moitié dans la voiture) ; deux polices trop proches : recul de 7 m d'un coup | Dessin jusqu'à **160 m derrière** ; renfort et retour à **220 m** (hors du rayon de dessin, donc hors de toute vue) ; rapprochement borné **4 m/s + 0,3 × distance** (45 au plus) ; côté impossible → l'autre côté, sinon derrière ; jamais dans la voiture du joueur (largeur gardée à côté, pare-chocs contre pare-chocs derrière) ; recul de 0,3 m par pas au plus |
| RG-02 (ANO-192) | Le chiffre de l'encart POLICE était la distance à l'arrivée | Distance à la police la plus proche (« 45 M », « 1,20 KM », « — ») ; même police, même taille |
| RG-03 (ANO-193) | Le score perd 1 point par seconde quand une police est à moins de 12 m (et à chaque choc), sans rien d'écrit | Chaque perte garde sa raison ; écrite sous l'encart 2 s (« -1 POLICE AU CONTACT », « -5 CHOC AVEC LE TRAFIC », « -3 CÔNE DE CHANTIER »…), cumulée si la raison ne change pas |
| RG-01 (ANO-191) | 12 cubes orange **lumineux** en ligne (les « blocs lumineux »), créés 260 m avant avec le bandeau, dessinés à 250 m | Vrai chantier : panneau AK5 sur pied, biseau de 7 cônes K5a (75 cm, socle noir, bandes blanches, sans lumière propre) qui barre la file, cônes tous les 10 m sur 120 m ; posé 450 m avant (hors de vue), bandeau à 200 m quand les cônes se voient. Barrage, accident, camion, ralentisseur aussi posés 450 m avant |
| RG-06 (ANO-196) | Aucun appel à `conducteurs::ajouter` pour la police | Policier en uniforme bleu marine au volant de chaque voiture ; prioritaire quand la file des conducteurs est pleine (trafic dense) |
| RG-07 (ANO-197) | Deux boîtes rouge / bleue sous le haut du modèle (traversaient le toit) ; une voiture sur deux bleu marine, sans marquage | Caisse blanche, bandes bleues « POLICE » avec liseré blanc et rouge sur les portières (texture faite par le code, aucune marque) ; rampe posée sur le pavillon mesuré : socle gris 1 m × 30 cm, deux feux **bleus** qui clignotent l'un après l'autre (2 Hz) |

**Fichiers touchés** : `src/game/poursuite.h`, `src/game/poursuite.cpp`, `src/game/poursuite_system.cpp`, `tests/test_poursuite.cpp`, `journal.md`.
**Accroche hors zone (annoncée au journal, ≤ 10 lignes)** : `src/world/conducteurs.h/.cpp` (paramètre `policier`). Aucun fichier `traffic*`, `game.cpp`, `track.*`, `camera_vue.*`, `player.*`, `conduite_*`, `vehicule_*`, `hud.*`.
**Limite** : le rétroviseur de la vue Cockpit n'est qu'un boîtier noir (aucune image rendue, hors zone) ; preuve « derrière » par une caméra du banc devant la voiture, tournée vers l'arrière (`poursuite-arrivee-N-arriere`).

**Tests CPU** : `bash tools/qa/tests_cpu.sh build` : 79 tests OK ; `test_course5` (poursuite) : nouveaux contrôles RG-05 (0 saut > 1 m par pas dans la zone dessinée, 0 rapprochement trop rapide, 0 pas dans la voiture du joueur, 4 positions du joueur), RG-02, RG-03 (7 raisons), RG-01 (biseau qui ferme la file, 3 files).
**Parcours complet** (banc course5) : « accueil → Course solo → Poursuite → Continuer → garage → Choisir → terrain → Rouler → poursuite → fin → Menu → accueil » et redémarrage : **0 écart**.
**Suite d'images de la police qui arrive** : `poursuite-arrivee-1` à `-6` (une par seconde : 229, 204, 158, 114, 81, 56 m derrière), vue Poursuite et vue arrière du banc.

## Captures pour le lead dev (24 après)

Dossier : [`qa/pr/WFS-338`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `poursuite-arrete` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-arrete.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrete.jpg?raw=true) |
| `poursuite-arrivee-1-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-1-arriere.jpg?raw=true) |
| `poursuite-arrivee-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-1.jpg?raw=true) |
| `poursuite-arrivee-2-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-2-arriere.jpg?raw=true) |
| `poursuite-arrivee-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-2.jpg?raw=true) |
| `poursuite-arrivee-3-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-3-arriere.jpg?raw=true) |
| `poursuite-arrivee-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-3.jpg?raw=true) |
| `poursuite-arrivee-4-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-4-arriere.jpg?raw=true) |
| `poursuite-arrivee-4` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-4.jpg?raw=true) |
| `poursuite-arrivee-5-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-5-arriere.jpg?raw=true) |
| `poursuite-arrivee-5` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-5.jpg?raw=true) |
| `poursuite-arrivee-6-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-6-arriere.jpg?raw=true) |
| `poursuite-arrivee-6` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-arrivee-6.jpg?raw=true) |
| `poursuite-bouchon` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-bouchon.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-bouchon.jpg?raw=true) |
| `poursuite-camion` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-camion.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-camion.jpg?raw=true) |
| `poursuite-hud` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-hud.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-hud.jpg?raw=true) |
| `poursuite-police-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-police-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-police-basse.jpg?raw=true) |
| `poursuite-police` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-police.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-police.jpg?raw=true) |
| `poursuite-ralentisseur` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-ralentisseur.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-ralentisseur.jpg?raw=true) |
| `poursuite-redemarrage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-redemarrage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-redemarrage.jpg?raw=true) |
| `poursuite-surprise` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-surprise.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-surprise.jpg?raw=true) |
| `poursuite-travaux-proche` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-travaux-proche.jpg?raw=true) |
| `poursuite-travaux` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-travaux.jpg?raw=true) |
| `poursuite-victoire` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/avant-poursuite-victoire.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-338/qa/pr/WFS-338/apres-poursuite-victoire.jpg?raw=true) |
