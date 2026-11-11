## Agent : Claude, agent dev (01UY1Vv5Nm)

**Ticket** : WFS-428 — règles : `regles-de-gestion/WFS-428.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : « Répare les bancs du matin » : le groupe `realisme-2` est rouge depuis le run #12 (ANO-245 : M-01 de nuit, face +Z à −6,34 % ; ANO-246 : « capture abandonnée » 2 fois en Campagne).
- **Ce qui change** :
  - `src/render/environnement.h` / `.cpp` : `demander(point, taille, sansJoueur = false)`. Depuis WFS-322 (#289, commit **52d42271**, fusionné 7 min après le run #5 vert), **toute** capture d'environnement excluait la voiture du joueur. La caméra du banc M-01, posée 6 m devant la Golf, la voit derrière elle dans la face +Z (de nuit, avec ses phares allumés) ; la capture non. Sur la planche `realisme-cubemap-essai` de `main`, on voit l'ombre de la Golf sans la Golf. Désormais, l'exclusion ne vaut que pour les captures des **sondes d'éclairage** (`eclairage_physique.cpp` : `demander(p, 0, true)`, seul appel du jeu). **L'image du jeu ne change pas** (ANO-174 / WFS-322 gardé).
  - `src/render/eclairage_physique.cpp` (1 ligne + commentaire) : la passe des sondes ne comptait les captures finies que lorsqu'elle tournait. Les captures 5 à 11 du banc des passes, faites éclairage désactivé, laissaient le compteur en retard. À la 1re image en Campagne, la capture 12, **encore en cours**, était prise pour une capture finie : relue trop tôt, faces sans exposition, abandonnée ; puis la 12 elle-même (une face non notée). Désormais, quand une capture est en cours et que le compteur est en retard, il est remis à jour et la capture suivie face par face. Le même cas existe en jeu au retour d'une scène dédiée (menus, garage) : la sonde n'est plus jetée. **Garde `kFacesValides` inchangée** (RG-04).
  - Commits à l'origine de (c) : **00bd5807** (WFS-151, compteur) et **941e4756** (WFS-213, garde qui rend l'abandon visible). Ce n'est **pas** une entrée récente : reproduit en local sur 9c4f58a (run #12) **et** sur 4e7fa716 (run #5 vert). Ces lignes `[ERREUR]` ne comptent pas dans le nombre d'erreurs du banc, d'où l'hypothèse « entrée entre 9c4f58a et ed6f745 » du ticket.
- **Comment c'est vérifié** :
  - `tools/qa/banc_groupe.sh realisme-2` (lavapipe, comme la CI), sur `main` puis sur la branche :

| | `main` (9a6789c0) | branche |
|---|---|---|
| résultat | « partie 2, **3** erreur(s) » | « partie 2, **2** erreur(s) » : seulement les 2 lignes de WFS-377 (« Ultra » introuvable, parcours 2 / 5) |
| M-01 nuit +X | 0,0282 / 0,0283 (−0,37 %) | idem |
| M-01 nuit −X | 0,0371 / 0,0371 (+0,07 %) | idem |
| M-01 nuit +Y | 0,0303 / 0,0303 (−0,03 %) | idem |
| M-01 nuit −Y | 0,0541 / 0,0541 (−0,02 %) | idem |
| M-01 nuit **+Z** | 0,0345 / 0,0368 (**−6,37 %, NON CONFORME**) | 0,0368 / 0,0368 (**−0,06 %**, conforme) |
| M-01 nuit −Z | 0,0698 / 0,0693 (+0,64 %) | idem |
| M-01 nuit | **5 / 6** | **6 / 6** |
| M-01 jour +Z | −0,72 % | −0,47 % (6 / 6 des deux côtés) |
| éclairage (WFS-213) | `capture abandonnée (face sans exposition connue)` **×2** (Campagne, captures 11 et 12), sonde 3 prise sur la capture 13 | **aucune** ; sonde 3 prise sur la capture 12 |

  - Rapports `realisme-fidelite-2.txt` et `realisme-mesures-2.txt` : identiques à `main` hors M-01 et durées. Étalonnage identique (sonde 14 244 lx, M-02 ΔE 0,00). Tolérance ± 1 %, contrôles et valeurs attendues **inchangés** ; `bench_passes.cpp` **non touché**.
  - Captures ci-dessous (bancs `phares` et `realisme` en passes seules, `WFS_PASSES_SEUL=1`) : **0 pixel différent** sur toutes les vues du jeu en Haute (Ville de nuit vue Proche, Campagne de nuit vue Capot, avec ou sans pluie, Ville de jour, chaussée Haute / Ultra, parcours Ultra). En **Basse**, les 3 vues `poursuite-basse*` diffèrent (jusqu'à 111/255 sur 53 pixels) : **deux lancements de la même branche donnent exactement le même écart** (301 462 px, max 111) ; c'est le non-déterminisme du banc en Basse, pas ce changement. `reglages-*` : seule la ligne des durées change. `realisme-cubemap-essai` (planche du cube, banc) : la Golf apparaît dans la face +Z, ce qui corrige le défaut.
  - Tests CPU (`tools/qa/tests_cpu.sh build`) : 106 / 107 pendant un autre essai en parallèle ; `test_audio_chocs` (RG-07, chronométrage 0,0504 ms contre ≤ 0,05) vert relancé seul (0,0424 ms) : sans lien.
- **Limites et risques connus** :
  - Les captures d'environnement **hors sondes** (bancs) contiennent maintenant la voiture du joueur. Le jeu n'en demande pas d'autres aujourd'hui ; un futur consommateur (`suivre()`, reflets) qui la veut exclue passe `sansJoueur = true`.
  - Coût : aucun en jeu ; dans le banc seulement, la face qui voit la Golf la dessine (+0,7 ms par face sur lavapipe, bruit compris).
  - `realisme-2` ne sera vert qu'avec **WFS-377** fusionné (choix « Ultra »).
  - Vue Ville de nuit en Ultra demandée par le ticket : le banc `phares` ne propose que la Haute ; l'Ultra de nuit est couverte par la mesure M-01 (planche du cube : jour seulement, voir le tableau).

## Captures pour le lead dev (22 après)

Dossier : [`qa/pr/WFS-428`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `phares-campagne-capot-haut` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-phares-campagne-capot-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-phares-campagne-capot-haut.jpg?raw=true) |
| `phares-campagne-capot-pluie-haut` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-phares-campagne-capot-pluie-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-phares-campagne-capot-pluie-haut.jpg?raw=true) |
| `phares-campagne-capot-pluie` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-phares-campagne-capot-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-phares-campagne-capot-pluie.jpg?raw=true) |
| `phares-campagne-capot` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-phares-campagne-capot.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-phares-campagne-capot.jpg?raw=true) |
| `phares-poursuite-basse-face` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-phares-poursuite-basse-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-phares-poursuite-basse-face.jpg?raw=true) |
| `phares-poursuite-basse-police` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-phares-poursuite-basse-police.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-phares-poursuite-basse-police.jpg?raw=true) |
| `phares-poursuite-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-phares-poursuite-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-phares-poursuite-basse.jpg?raw=true) |
| `phares-poursuite-face` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-phares-poursuite-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-phares-poursuite-face.jpg?raw=true) |
| `phares-poursuite-police` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-phares-poursuite-police.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-phares-poursuite-police.jpg?raw=true) |
| `phares-poursuite-proche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-phares-poursuite-proche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-phares-poursuite-proche.jpg?raw=true) |
| `phares-ville-face-pluie` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-phares-ville-face-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-phares-ville-face-pluie.jpg?raw=true) |
| `phares-ville-face` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-phares-ville-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-phares-ville-face.jpg?raw=true) |
| `phares-ville-jour-face` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-phares-ville-jour-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-phares-ville-jour-face.jpg?raw=true) |
| `phares-ville-jour-proche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-phares-ville-jour-proche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-phares-ville-jour-proche.jpg?raw=true) |
| `chaussee-haute-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-chaussee-haute-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-chaussee-haute-course.jpg?raw=true) |
| `chaussee-ultra-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-chaussee-ultra-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-chaussee-ultra-course.jpg?raw=true) |
| `parcours-ultra-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-parcours-ultra-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-parcours-ultra-course.jpg?raw=true) |
| `parcours-ultra-retour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-parcours-ultra-retour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-parcours-ultra-retour.jpg?raw=true) |
| `realisme-cubemap-essai` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-realisme-cubemap-essai.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-realisme-cubemap-essai.jpg?raw=true) |
| `realisme-prepasse-essai` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-realisme-prepasse-essai.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-realisme-prepasse-essai.jpg?raw=true) |
| `reglages-diagnostic` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-reglages-diagnostic.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-reglages-diagnostic.jpg?raw=true) |
| `reglages-qualite-ultra` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/avant-reglages-qualite-ultra.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-428/qa/pr/WFS-428/apres-reglages-qualite-ultra.jpg?raw=true) |
