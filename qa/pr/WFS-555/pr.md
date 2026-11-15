## Agent : lead-dev (session_01UY1Vv2AmFHmyurYWvyN3oV)

**Ticket** : WFS-555 — règles : `regles-de-gestion/WFS-555.md`

## Description

- **Demande de l'utilisateur** (2026-11-15 ≈ 22:50 UTC) : les développeurs passent trop de temps à faire les captures de PR ; garder les captures « avant » de main et sa compilation au lieu de tout refaire à chaque lancement (« on l'applique toutes de suite »).
- **Ce qui change** : seul `tools/qa/captures_pr.sh` (+ preuves dans `qa/pr/WFS-555/`).
  - RG-01 / RG-02 : captures « avant » rangées dans un cache hors du dépôt (`${XDG_CACHE_HOME:-~/.cache}/wfs-captures/<SHA complet de origin/main>/<banc>[_<LxH>]/`), relues si le témoin `.complet` existe pour **ce** SHA ; écriture dans `.en-cours.*` puis renommage ; banc en erreur sur main jamais mis en cache ; 3 derniers commits gardés ; verrou `flock` autour de l'arbre de main et du cache.
  - RG-03 : arbre de travail de main gardé (`main-src-<empreinte du clone>`), `checkout --detach <sha>` puis compilation incrémentale Ninja ; recréé s'il est cassé ou lié à un autre clone ; `build/` recréé une fois si la compilation incrémentale échoue.
  - RG-05 : `SANS_CACHE=1` = exactement l'ancien chemin (arbre temporaire, compilation depuis zéro, cache ni lu ni écrit) ; `README.md` et `pr.md` disent le SHA de main comparé et d'où viennent les « avant ».
  - RG-06 / RG-07 : compilation et bancs de la branche inchangés ; aucun workflow CI touché.
- **Comment c'est vérifié** (banc `chargement` 1280x720, même conteneur, lavapipe ; journaux dans `qa/pr/WFS-555/*.log`) :

  | Mesure | Durée totale |
  |---|---|
  | (a) ancien script (main) | **242 s** |
  | (b) nouveau, cache vide (arbre de main créé, compilation complète) | 203 s |
  | (c) nouveau, cache plein (main ni compilé ni rejoué) | **29 s** |
  | (d) nouveau, `SANS_CACHE=1` | 196 s |
  | (e) nouveau, main a bougé (42bdd896 ≠ 56cdb81f) : nouveau dossier, compilation incrémentale | **54 s** |

  - Cache rendu octet pour octet : les PNG « avant » de (b) (qui a rempli le cache) et de (c) (qui l'a lu) sont identiques au fichier du cache (`cmp`).
  - Avec / sans cache au pixel : écart 596 à 857 pixels sur 921 600 ; mais **deux lancements identiques sans cache diffèrent déjà de 804 à 967 pixels** (la ligne « Mémoire : jeu … Mo » de l'écran de chargement affiche une mesure réelle) ; l'image fixe `chargement-erreur` : 0 pixel d'écart partout. Le cache ne change donc rien au rendu ; il rend la capture d'un vrai passage du banc sur ce commit.
  - Main a changé → nouveau dossier `42bdd896…`, l'ancien `56cdb81f…` n'est pas lu (journal (e) : « 0 banc relu, 1 rejoué »).
  - `bash -n` sans erreur (`shellcheck` absent du conteneur). Tests CPU ci-dessous.
- **Limites et risques connus** :
  - Les sessions cloud repartent d'une machine neuve : le cache sert dans la même session (refus du lead dev, synchronisations, plusieurs lancements) ; une nouvelle session paie une fois le coût (b).
  - Critère « identique au pixel » : impossible au sens strict même sans cache (banc non déterministe, voir ci-dessus) ; prouvé à la place : fichiers identiques octet pour octet au passage réel mis en cache.
  - Deux lancements en même temps attendent le verrou pour la partie « avant » seulement.
  - Les 5 captures sont celles de l'écran de chargement (US technique sans écran : RG « Captures demandées »).

## Tests CPU

Commit testé : `1a058cbd` (branche synchronisée avec `main` ; le commit suivant n'ajoute que `qa/pr/WFS-555/`). Journal complet : `qa/pr/WFS-555/tests-cpu.log`.

```
[OK] test_ville_sol (3 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 148 test(s) réussi(s) en 381 s
code de sortie : 0
```

## Captures pour le lead dev (5 après)

Dossier : [`qa/pr/WFS-555`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-555/qa/pr/WFS-555) — produit par `tools/qa/captures_pr.sh`.

Captures « avant » : `origin/main` = `42bdd896d14fd07b6b3fb6b165aa51c4f2d0c277` ; cache : 0 banc(s) relu(s), 1 banc(s) rejoué(s) ; main compilé : oui.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `chargement-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-555/qa/pr/WFS-555/avant-chargement-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-555/qa/pr/WFS-555/apres-chargement-0.jpg?raw=true) |
| `chargement-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-555/qa/pr/WFS-555/avant-chargement-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-555/qa/pr/WFS-555/apres-chargement-1.jpg?raw=true) |
| `chargement-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-555/qa/pr/WFS-555/avant-chargement-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-555/qa/pr/WFS-555/apres-chargement-2.jpg?raw=true) |
| `chargement-erreur` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-555/qa/pr/WFS-555/avant-chargement-erreur.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-555/qa/pr/WFS-555/apres-chargement-erreur.jpg?raw=true) |
| `chargement-fondu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-555/qa/pr/WFS-555/avant-chargement-fondu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-555/qa/pr/WFS-555/apres-chargement-fondu.jpg?raw=true) |
