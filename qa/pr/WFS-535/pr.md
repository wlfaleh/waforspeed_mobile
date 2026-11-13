Agent : Claude, agent dev3d (01Us6uCtPt)

## Demande
Épic E22 « Score » (demande de l'utilisateur : chaque course terminée donne un score converti en 5 à 40 ★ au lieu des 10 ★ fixes). Ce ticket fait **seulement le calcul** (fonction pure + barème dans `configgame.json` + tests) ; le crédit des étoiles en fin de course est WFS-536, l'écran WFS-537, les records WFS-538.

## Ce qui change
- **Nouveau** `src/game/score_course.{h,cpp}` : `score::calculer(ResultatCourse, bareme)` → lignes `{clé de texte, points}`, total, étoiles, « nouveau record ». Aucun accès au rendu, au réseau, à la sauvegarde (RG-06, RC-E22-02).
  - RG-01 place / issue des 4 modes (course à 5, duel, Poursuite, course solo) ; RG-02 temps (record battu +300, à 5 % +150, 1re course +150) ; RG-03 conduite propre (0 choc +200, 1-2 chocs +100) ; RG-04 2 défis par mode (compteur inconnu = 0, sans erreur) ; RG-05 conversion arrondie, bornée 5 à 40 ★, course quittée 0 ★ ; RC-E22-04 duel dont l'adversaire part avant 30 s = 0 ★.
- **`src/core/configgame.*`** : section `score_etoiles` (35 clés bornées, valeurs de départ du PO), contrôle `min > max` ; **`configgame.json` et `configgame.README.md`** : section ajoutée **à la main** (le fichier de `main` porte des réglages de l'utilisateur différents du code : régénérer le fichier les aurait écrasés ; aucune ligne existante ne change).
- **`points_par_etoile = 85`** : joueur moyen ≈ 860 points par course sur les 4 modes → ≈ 10 ★ (calcul détaillé au journal du 2026-11-13). Vérifié plus tard par le banc d'équilibrage de WFS-536.
- **Nouveau test** `tests/test_score_course.cpp` (49 cas, RG-07) + une ligne dans `CMakeLists.txt`.
- **Rien ne change dans le jeu** : aucun appel à `score::calculer` pour l'instant ; `boutique.gain_course` (10 ★) reste en place jusqu'à WFS-536.

## Vérification
- Construction complète locale (870 cibles) et `bash tools/qa/tests_cpu.sh build` : **124 tests réussis**, aucun échec, après synchronisation avec `main`.
- `test_configgame` : 308 clés, OK ; `test_boutique` vert.
- Retour du lead dev (remarque non bloquante) : plafonds de la course solo et des dépassements appliqués avant la conversion en entier (aucun débordement), 2 cas de test ajoutés.

## Tests CPU
Commit testé : **7315604f** (branche synchronisée avec `main` 3b89710b), construction complète locale. Journal complet : `qa/pr/WFS-535/tests-cpu.log`.
```
[OK] test_score_course (0 s)
…
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 124 test(s) réussi(s) en 402 s
```

## Limites et risques
- ⚠️ Avec le barème de départ, la meilleure course possible fait ≈ 1 800 points = **21 ★** : la borne 40 ★ n'est atteinte qu'en baissant `points_par_etoile` (le joueur moyen dépasserait alors 10 ★). Signalé au PO (journal) ; à trancher au banc de WFS-536.
- Choix d'interprétation (commentés dans le code) : plafond 400 de la course solo sur la part tirée de la distance ; record égalé = « proche » ; 1re course = pas de record (record ≤ 0) ; « arrivée avec ≥ 50 % » et « écart > 3 s » comptent seulement sur une victoire.
- Les compteurs (chocs, dépassements, contacts police…) seront fournis par WFS-536 ; ici ils sont des entrées.

## Captures (US sans écran : sorties du test, demandées par les RG)
| Avant (`main`) | Après (branche) |
|---|---|
| ![avant](avant-main.png) — 10 ★ fixes par course | ![1](apres-1-course5-premier-propre.png) 1er de course à 5, propre : 1680 points, 20 ★ |
| | ![2](apres-2-poursuite-perdue.png) Poursuite perdue : 420 points, 5 ★ (borne basse) |
| | ![3](apres-3-course-quittee.png) Course quittée : 0 ★ |
| | ![4](apres-4-tests-cpu.png) 49 cas du test, tous OK |

Textes bruts : `qa/pr/WFS-535/*.txt`.
