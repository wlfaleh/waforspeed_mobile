Agent : Claude, agent dev Z (01UY1Vv3EZ)

## Demande
L'utilisateur : « la dernière version est niquer » (2026-11-09 19:35 UTC). Ticket WFS-374 (P1) : trouver le commit fautif et corriger, sans rien retirer.

## Diagnostic
- APK de l'utilisateur à 19:35 : run #966 (WFS-338), puis vidéos de l'APK v0.1.969. Bancs `menu1`, `parcours`, `paquets` de main : 0 écart. AddressSanitizer + UBSan sur les bancs `carte` (4 cartes jour / nuit) et `course5` (Course à 5 + poursuite) : aucune erreur mémoire.
- Vidéos v0.1.969 : la « Campagne bloquée » n'est **pas confirmée** (QA, `qa/rapports/recette-manuelle-video-2026-11-09-2213.md`, et PO : aucun doigt sur GAZ après le GO). Défaut visible = lumière JOUR → ANO-232 / WFS-375 (autre ticket). Reste pour WFS-374 (PO, 21:35 UTC) : le banc « départ Campagne, gaz tenu dès le GO » pour chaque moment de JOUR, et **ANO-233** (appui sur la case SCORE → nitro).
- Cause d'ANO-233 : `commandeSous()` (`src/game/commandes_tactiles.h`) donnait le nitro à tout appui dans le coin haut droit (x > 0,80 W, y < 0,45 H), zone héritée de l'ancien bouton nitro, là où est la case SCORE.

## Ce qui change
- `src/game/commandes_tactiles.h` : au-dessus du haut de la zone de toucher du nitro (disque + 8 dp), un appui ne pilote plus rien (ni nitro ni gaz). Boutons, tailles et zones des pédales inchangés ; nitro « Grand » compris.
- `src/game/depart_campagne_banc.cpp` (nouveau) + accroche de 2 lignes dans `src/platform/desktop_main.cpp` : banc `WFS_BANC=depart-campagne` (Réelle, Aube, Matin, Midi, Dorée, Nuit) : décompte, GO, ≥ 25 km/h et ≥ 8 m 3 s après le GO avec le doigt sur GAZ, pas de nitro sur SCORE.
- `src/render/bench_effets.cpp` (accroche) : la capture `nitro-lignes` appuie au centre du vrai bouton nitro.
- Tests : `tests/test_commandes.cpp` (HUD haut droit = aucune commande, 6 écrans × normaux / grands × droitier / gaucher) ; `tests/test_course.cpp` (coin haut droit : rien au lieu de nitro).

## Vérification
- 94 tests CPU verts (`bash tools/qa/tests_cpu.sh build`) sur la branche synchronisée avec main.
- Banc `depart-campagne` : branche 0 écart ; main 6 écarts (nitro sur SCORE dans les 6 moments). 57,9 km/h 3 s après le GO, avant comme après : la Campagne démarre.
- Banc `effets` : `nitro-lignes`, nitro actif (réserve 0,21, 46 km/h) avec le vrai bouton.

## Limites et risques
- Le banc PC ne reproduit aucune panne de conduite en Campagne ; le ticket reste 🟡 jusqu'à l'essai de l'utilisateur sur son téléphone.
- Un joueur qui lançait le nitro en touchant le haut droit de l'écran doit maintenant toucher le bouton NITRO (zone élargie de 8 dp gardée).
- Pas de captures « avant » produites par `captures_pr.sh` : le banc n'existe pas sur main ; « avant » = origin/main compilé avec ce seul banc ajouté (sans le correctif).

## Captures
Voir `qa/pr/WFS-374/README.md` (14 captures, 6 moments avant / après + nitro-lignes).
