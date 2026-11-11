Agent : Claude, agent dev3d (01UY1Vv51w)

**Ticket** : WFS-377 — règles : `regles-de-gestion/WFS-377.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : « Répare les bancs du matin » : le groupe `realisme-2` est rouge (runs #12 et #16) ; ses 2 dernières lignes `[ERREUR]` viennent du parcours WFS-150 qui ne trouve plus « Ultra » dans les Réglages à onglets (ANO-211).
- **Ce qui change** : un seul fichier, `src/render/bench_passes.cpp`, bloc « RG-04 / P-05 » (banc d'essai seulement ; **le jeu ne change pas**).
  1. **RG-01** : après « Réglages », le banc **touche l'onglet qui contient « Graphismes »** (`ecrans::ongletDe(L_QUALITE)`, table de `reglages_onglets.h` de WFS-291 ; le panneau déclare ses onglets en premier, contrôlé : même rangée, de gauche à droite), comme un joueur. Onglet introuvable → `[ERREUR]` et écart.
  2. **RG-01** : « Ultra » est retrouvé **par son rôle, plus par un rang fixe** (« 8e zone ») : c'est le choix qui apparaît quand la Haute a tenu. Le banc groupe les zones par rangée (même y, même hauteur) ; la seule rangée qui passe de 3 à 4 choix est « Graphismes », Ultra en est le choix le plus à droite. Zéro ou plusieurs rangées candidates → `[ERREUR]` « introuvable », écart.
  3. **RG-02** (logique inchangée) : l'étape « course en Ultra » demandait « 1 capture d'environnement ». Depuis WFS-151, les **sondes d'éclairage** font aussi leurs captures en course (départ, rebond : 2 ici). Le banc compte maintenant **les captures qu'il a demandées** = toutes − celles des sondes (`eclairage::etat().sondesCapturees`) ; il exige toujours 1 capture et 256 px. Aucun seuil affaibli, le journal donne les trois nombres.
  4. Commentaire de l'accroche WFS-214 corrigé (« le banc ne trouve plus Ultra » n'est plus vrai).
- **Comment c'est vérifié** :
  - Banc `realisme` avec `WFS_PASSES_SEUL=1` (lavapipe, même rendu que la CI), `main` à jour (WFS-428 / #357 compris) contre la branche :

| Journal du banc | `main` | branche |
|---|---|---|
| onglet | — | « onglet Affichage, 20 zones sans Ultra, 21 avec (Haute tenue : oui) » |
| choix Ultra | `[ERREUR] réglages (WFS-150) : choix « Ultra » introuvable (21 zones)` | « choix « Ultra » trouvé (zone 11 sur 21, rangée Graphismes de 4 choix) » |
| ligne Rendu | `ULTRA NON CHOISI ; Rendu : Haute` | `Ultra choisi (coûts de la Haute) ; Rendu : Ultra` |
| course | « 1 capture(s) d'environnement de **128 px** » | « 1 capture(s) d'environnement demandée(s) par le banc (3 en tout, dont 2 des sondes WFS-151), **256 px** » |
| parcours | `2 / 5 étapes ÉCHEC` + `[ERREUR] parcours (WFS-150) : 2 / 5 étapes` | **`5 / 5 étapes OK`** |
| fin | « banc terminé, **2** écart(s) » | « banc terminé, **0** écart(s) » |

  - Captures : `reglages-qualite-ultra` montre « Ultra » sélectionné dans l'onglet Affichage (avant : « Haute ») ; `parcours-ultra-course` est la course de 5 s en Ultra (capture d'environnement de 256 px ; avant : Haute, 128 px) : elle redevient une preuve valable pour ANO-106 ; `parcours-ultra-retour` : retour à l'accueil, Ultra gardé.
  - Tests CPU : `tools/qa/tests_cpu.sh build` → **109 / 109 réussis**.
- **Limites et risques connus** :
  - Avant la fusion de #357, la branche montrait aussi `M-01 (nuit) : 5 / 6` (face +Z) : c'était ANO-245, corrigé par WFS-428 ; après synchronisation, 0 écart.
  - Le décompte « captures du banc = toutes − sondes » suppose qu'une capture de sonde finie pendant les 5 s est traitée avant la fin de la fenêtre (c'est le cas : traitée à l'image suivante ; 300 images de marge).
  - Le groupe complet `realisme-2` de la CI n'a pas été relancé ici (passes seules en local) : les bancs du matin de demain le confirmeront ; une CI de branche reste possible (US technique) si le lead dev la veut.

## Captures pour le lead dev (4 après)

Dossier : [`qa/pr/WFS-377`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-377/qa/pr/WFS-377) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `parcours-ultra-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-377/qa/pr/WFS-377/avant-parcours-ultra-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-377/qa/pr/WFS-377/apres-parcours-ultra-course.jpg?raw=true) |
| `parcours-ultra-retour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-377/qa/pr/WFS-377/avant-parcours-ultra-retour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-377/qa/pr/WFS-377/apres-parcours-ultra-retour.jpg?raw=true) |
| `reglages-diagnostic` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-377/qa/pr/WFS-377/avant-reglages-diagnostic.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-377/qa/pr/WFS-377/apres-reglages-diagnostic.jpg?raw=true) |
| `reglages-qualite-ultra` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-377/qa/pr/WFS-377/avant-reglages-qualite-ultra.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-377/qa/pr/WFS-377/apres-reglages-qualite-ultra.jpg?raw=true) |
