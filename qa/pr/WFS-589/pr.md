## Agent : agent expert ci cd (01UY1Vv43t)

**Ticket** : WFS-589 — règles : `regles-de-gestion/WFS-589.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : « fix les problèmes de CI » : `main` rouge depuis le run #1155 de `build.yml` et bancs du matin #67 rouges sur 4 jobs (ANO-277).
- **Ce qui change** (bancs et outils de contrôle seulement, **aucun changement du jeu**, RG-03) :
  1. `tools/qa/verifier_licences.py` : ligne de couverture pour les 3 photos des modes de WFS-562 (`assets/ui/modes/{checkpoint,course5,poursuite}.jpg`), rendus du jeu lui-même (banc `modes-photos`), sans image extérieure ; ces 3 fichiers seuls (RG-06). Cause des rouges « banc-essai (texte) » (#1155 à #1158) et « tests CPU et licences ».
  2. `src/platform/desktop_main.cpp` (`parcoursFinMenu`) et `src/platform/chauffe_systeme.cpp` (parcours du banc de chauffe seulement) : le bouton « Menu » de l'écran de fin est lu sur l'écran (`courseBouton("menu", x, y)`, déjà utilisé par les bancs `score-fin`, `textes`, garage) au lieu de coordonnées en dur ; le bouton avait bougé avec R19 (WFS-557 / 561). Cause des rouges `complements-c` (cuisson) et chauffe (RG-01).
  3. `src/render/bench_passes.cpp` : le banc cherche « Ultra » dans le **bloc de pastilles** de son réglage. Depuis R19, l'onglet Affichage a 2 colonnes et la ligne « Graphismes » partage sa hauteur avec « Fantôme du record » : le banc comptait 6 choix au lieu de 4 et ne trouvait plus « Ultra » (les 2 erreurs de `realisme-2`). Erreur du banc, pas du soleil (RG-02) : rien à rattacher à WFS-581.
- **Comment c'est vérifié** :
  - local : licences 0 manque ; `WFS_BANC=parcours` 0 écart en 1280x720 et 2400x1080 (vue Cockpit comprise), appui sur « Menu » à (232, 534) → accueil ; `chauffe.sh` code 0 ; `realisme` (passes) : « Ultra trouvé », 5 / 5 étapes.
  - CI de branche (US technique) : bancs du matin #70 sur `feature/waforspeed-589` https://github.com/wlfaleh/waforspeed_mobile/actions/runs/45025551433 : **run entièrement vert** (26 jobs : tests CPU et licences, endurance et chauffe, realisme-1, realisme-2, complements-a/b/c, tous les groupes de captures, APK, émulateur) ; publications sautées (réservées à `main`). Après ce run : synchronisation avec `main` (WFS-597), conflit dans `verifier_licences.py` résolu en gardant les deux lignes, licences 0 manque et compilation refaites en local.
- **Limites et risques** : aucun seuil changé, aucun banc retiré ni passé en avertissement. `chauffe_systeme.cpp` et `bench_passes.cpp` sont hors de la zone écrite dans les RG (desktop_main + tools/qa), mais ne touchent que le code des bancs (annoncé au journal). La capture « avant » du réglage Ultra vient du banc de `main` (Ultra non choisi), celle « après » du banc corrigé (Ultra choisi).

## Tests CPU

`bash tools/qa/tests_cpu.sh build` sur la tête de branche 38e431d (synchronisée avec `main`, WFS-597 compris) : **158 test(s) réussi(s) en 466 s**, 0 échec. Journal complet : [`qa/pr/WFS-589/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589/tests-cpu.log).

## Captures pour le lead dev (7 paires)

Dossier : [`qa/pr/WFS-589`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589) — produit par `tools/qa/captures_pr.sh`.

Captures « avant » : `origin/main` = `f9a63d67bd92d1d11da204acda40bcb09ded2d7e` ; cache : 0 banc(s) relu(s), 2 banc(s) rejoué(s) ; main compilé : oui.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `parcours-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589/avant-parcours-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589/apres-parcours-course.jpg?raw=true) |
| `parcours-fin` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589/avant-parcours-fin.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589/apres-parcours-fin.jpg?raw=true) |
| `parcours-mode-3-cartes` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589/avant-parcours-mode-3-cartes.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589/apres-parcours-mode-3-cartes.jpg?raw=true) |
| `parcours-mode` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589/avant-parcours-mode.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589/apres-parcours-mode.jpg?raw=true) |
| `parcours-retour-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589/avant-parcours-retour-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589/apres-parcours-retour-menu.jpg?raw=true) |
| `parcours-terrain` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589/avant-parcours-terrain.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589/apres-parcours-terrain.jpg?raw=true) |
| `reglages-qualite-ultra` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589/avant-reglages-qualite-ultra.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-589/qa/pr/WFS-589/apres-reglages-qualite-ultra.jpg?raw=true) |
