Agent : agent expert ci cd

**Demande de l'utilisateur** : « Je valide A B D C » (2026-11-09 ~00:35 UTC, avis de Smart Brain : A, B, D, C) : le 2026-11-08, GitHub a lancé les réveils de `matin.yml` avec ~6 h de retard (08:06 et 09:43 UTC au lieu de 02:00 / 03:00), le job `heure` a tout sauté, et le run #3, vert mais vide, a fait passer 23 tickets en faux ✅.

## Ce qui change
- **RG-A, `.github/workflows/statuts.yml`** : pour un run des bancs du matin vert, le robot lit le job `compilation` ; s'il n'a pas tourné (`skipped`, ou illisible), **aucun statut ne change** et rien n'est écrit au journal (ni vert ni rouge, donc pas d'US technique pour rien). Les runs de fusion (`build.yml`) ne changent pas.
- **RG-C, `.github/workflows/matin.yml`** :
  - quatre réveils de secours (04:00, 05:00, 06:00, 07:00 UTC) en plus de 02:00 et 03:00 ;
  - job `heure` : lancement à la main → toujours ; réveil programmé → les bancs partent seulement entre **04:00 et 10:59 heure de Paris** et seulement si **aucun run du même jour (date de Paris)** n'a déjà une compilation qui a tourné ou qui tourne (lancement à la main compris) : **jamais deux fois le même jour** ;
  - `permissions: actions: read` (lecture des runs du jour).
- **RG-B** (déjà fait sur `main`, commit 4e7fa716) : les 23 faux ✅ remis en 🟣 avec leur statut d'avant. **RG-D** : bancs du matin lancés à la main cette nuit (2026-11-09 ~00:40 UTC) ; ce run compte pour le 9 novembre, donc les réveils de ce matin ne relanceront rien.

## Comment c'est vérifié
- YAML des deux fichiers relu par `yaml.safe_load`.
- Le script du job `heure` extrait du YAML et joué avec un faux `gh` (heure et date données) :
```
manuel 01h                                   → lancer=oui
réveil 03h                                   → lancer=non (hors de la fenêtre)
réveil 04h, rien avant                       → lancer=oui
réveil 08h, run du jour à compilation verte  → lancer=non (déjà lancés : run 111 (success))
réveil 09h, run du jour sauté                → lancer=oui
réveil 05h, run d'hier                       → lancer=oui
réveil 06h, compilation en cours             → lancer=non (déjà lancés : run 111 (in_progress))
réveil 11h                                   → lancer=non (hors de la fenêtre)
```
- RG-A : le nom du job `compilation` est celui des runs #3 et #4 (lus sur GitHub : `compilation` = `skipped`).

**Captures** : ticket de CI sans changement à l'écran ; preuves ci-dessus.

## Limites et risques
- Les bancs lancés en retard (jusqu'à 10:59) finissent après 07:00 : la QA ne part que sur le message « Recette prête » du jour (règle déjà en place).
- Un réveil qui arrive après 10:59 heure de Paris ne lance rien : ce jour-là il faut un lancement à la main.
- La concurrence du groupe `bancs-matin-main` met les réveils en file derrière un run en cours : chaque réveil voit donc le run précédent terminé ou en cours.
- Aucun seuil, banc ni test affaibli (RG-E).
