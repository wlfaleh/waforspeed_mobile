## Agent : Claude, agent dev3d (01Us6uCwQU)

**Ticket** : WFS-544 — règles : `regles-de-gestion/WFS-544.md`

## Description

- **Demande** (US technique P1 du PO, bancs du matin #32 rouges) : rendre vert le job « banc (vitrines) » : plus aucun pipeline créé pendant la course (WFS-262 RG-02) et parcours `records` qui va de nouveau de l'accueil au garage, au terrain et à la course.
- **Ce qui change** :
  - `src/render/environnement.cpp` (1 ligne de code) : les ressources fixes de la capture d'environnement (dont **les 2 pipelines** des personnages animés et de la tache des phares) sont créées **au chargement** (`creer()` de la passe, appelé par `passes::creer` après `createPipelines`, avec le cache de WFS-262) au lieu de la 1re capture, qui tombait pendant la course (RG-01, RG-02). Trouvé par un journal de chaque création de pipeline (retiré) : n° 39 et 40, passe de reflet, juste après « course affichée ».
  - `src/game/course_system.cpp` (banc `benchRecords` seulement, RG-05) : la sauvegarde neuve du banc reçoit un profil (comme « C'est parti » de WFS-481, sinon « Crée ton pilote » couvre le menu) et le banc part sur **la voiture de la sauvegarde** (partie neuve : 208, gratuite, WFS-498 RG-09) au lieu de la Golf, qui coûte 600 ★ depuis WFS-499 et ouvrait la fenêtre d'achat au lieu du terrain. Nouveaux contrôles : profil créé, voiture possédée. Captures ajoutées `records-modes` et `records-garage`. **Pas un défaut du jeu** : un vrai joueur crée son profil et arrive au garage sur sa voiture gratuite.
  - À l'écran du jeu : rien ne change (vitrines, garages et course identiques au pixel près, voir plus bas).
- **Comment c'est vérifié** :
  - Banc `vitrines` complet en local (lavapipe, même rendu que la CI), sortie du banc :
    - **avant (main)** : 39 `[ERREUR]`, dont `pipelines (WFS-262 RG-02) : 2 pipeline(s) créé(s) pendant la course (0 attendu)` et `records (parcours) : « Course solo » : panneau des modes ouvert (ÉCHEC)` ;
    - **après (branche)** : **0 `[ERREUR]`**, 5 × `pipelines (WFS-262 RG-02) : 0 pipeline créé pendant la course : OK`, parcours `records` complet (accueil → Course solo → Checkpoint → Continuer → garage → Choisir → terrain → Rouler → course → NOUVEAU RECORD → Rejouer avec fantôme → Poursuite → Course à 5), code de sortie 0.
  - Comparaison au pixel des captures `vitrine-p208-0`, `vitrine-sf90-0`, `garage-p208`, `garage-sf90`, `course-sf90` avant / après : **0 pixel différent** (écart max 0).
  - Run de branche n° 1068 (build.yml, US technique, branche synchronisée avec main, [lien](https://github.com/wlfaleh/waforspeed_mobile/actions/runs/43588935668)) : compilation ✅, APK et AAB ✅, **138 tests CPU réussis** ; le job « texte » est rouge **seulement** au contrôle des licences (`cars/remplacants/virtus`, `w212` absents de `CARS[]`) : c'est le rouge de main confié à WFS-543, hors de ce ticket. Endurance + chauffe et menu1-2400 : verts.
- **Limites et risques connus** :
  - Les ressources fixes de la capture d'environnement (2 pipelines repris du cache, quelques tampons et descripteurs, < 1 Mo) sont créées au chargement même si la passe ne tourne pas : coût négligeable.
  - RG-01 : la création différée existe depuis WFS-150 ; je n'ai pas pu établir avec certitude quel ticket récent a fait tomber la 1re capture pendant la course (historique local limité) : les pistes du PO (WFS-484, 488, 490, 491, 492) ne sont pas confirmées.
  - Les captures `records-*` « avant » montrent le parcours bloqué de main (attendu).

## Tests CPU

Commande : `bash tools/qa/tests_cpu.sh build` sur la branche synchronisée avec `main`, commit testé **ac88d39a** ; journal complet : [`qa/pr/WFS-544/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/tests-cpu.log).

```
[OK] test_ville_sol (3 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 138 test(s) réussi(s) en 401 s
```

## Captures pour le lead dev (11 après)

Dossier : [`qa/pr/WFS-544`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `course-sf90` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/avant-course-sf90.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/apres-course-sf90.jpg?raw=true) |
| `garage-p208` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/avant-garage-p208.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/apres-garage-p208.jpg?raw=true) |
| `garage-sf90` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/avant-garage-sf90.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/apres-garage-sf90.jpg?raw=true) |
| `records-checkpoint` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/avant-records-checkpoint.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/apres-records-checkpoint.jpg?raw=true) |
| `records-course5` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/avant-records-course5.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/apres-records-course5.jpg?raw=true) |
| `records-fantome` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/avant-records-fantome.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/apres-records-fantome.jpg?raw=true) |
| `records-garage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/apres-records-garage.jpg?raw=true) |
| `records-modes` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/apres-records-modes.jpg?raw=true) |
| `records-poursuite` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/avant-records-poursuite.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/apres-records-poursuite.jpg?raw=true) |
| `vitrine-p208-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/avant-vitrine-p208-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/apres-vitrine-p208-0.jpg?raw=true) |
| `vitrine-sf90-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/avant-vitrine-sf90-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-544/qa/pr/WFS-544/apres-vitrine-sf90-0.jpg?raw=true) |
