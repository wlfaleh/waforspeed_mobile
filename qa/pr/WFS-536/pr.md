Agent : Claude, agent dev3d (01UY1Vv3A5)

**Ticket** : WFS-536 (P1, épic E22 « Score ») — règles : `regles-de-gestion/WFS-536.md`, `regles-de-gestion/E22-score.md`

## Demande
Demande de l'utilisateur (épic E22) : chaque course terminée rapporte des étoiles selon son score (5 à 40 ★) au lieu des 10 ★ fixes ; ce ticket **branche le calcul de WFS-535 en fin de course** et adapte le banc d'équilibrage (écran de fin : WFS-537 ; records : WFS-538).

## Ce qui change
- **`src/game/profil_system.cpp`** (fin de course) : à l'arrivée, le score de la course est calculé (`score::calculer`, WFS-535) et ses étoiles sont créditées (RG-01). Course quittée : 0 ★. Résultat du mode jamais reçu (calcul impossible) : **secours `boutique.gain_course`** = 10 ★, avec une ligne au journal du jeu (RG-02). Le jeu donne au score : place ou issue, score du mode, **chocs** de la voiture du joueur (`bumps + accidents` de la course), records du mode **lus au départ** (le mode écrit son nouveau record à l'arrivée) et, en duel, le moment où un adversaire quitte la manche (RC-E22-04 : avant 30 s, 0 ★).
- **`src/game/boutique.{h,cpp}`** :
  - `crediterCourse(s, etoilesScore)` : les ★ du score, ou le secours si `etoilesScore < 0`.
  - `GainCourse` : **un seul crédit par course** (RG-05). Ni l'écran de fin rouvert, ni le retour au menu, ni le retour d'arrière-plan ne créditent une 2e fois.
  - `resultatScore()` : passage « course notée au profil → entrée du score », le même pour le jeu et le banc. La course solo n'a pas de chrono : c'est son score contre le record qui compte comme temps. La Poursuite et le duel n'ont pas de temps : seulement « 1re course » pour la Poursuite.
- **`src/game/profil.{h,cpp}`** (RG-03) : l'historique (20 dernières courses) garde les ★ gagnées, en **9e champ à la fin de la ligne**. Les lignes d'avant (8 champs) sont lues sans erreur (★ inconnues) et réécrites telles quelles.
- **`src/game/boutique_banc.cpp`** (RG-04) : le banc `WFS_BANC=equilibrage` simule 3 joueurs (moyen, bon, débutant : profils de place, chocs, victoires et temps à graine fixe) avec **le vrai calcul**, sur les 4 modes à tour de rôle. Il échoue si le joueur moyen sort de 22 à 28 courses pour la 1re voiture ou de 108 à 132 h pour toutes les voitures, ou si le débutant dépasse 40 courses.
- **Nouveau `src/game/score_banc.cpp`** + accroche d'une ligne dans `src/platform/desktop_main.cpp` : banc `WFS_BANC=score-etoiles`. Il joue une vraie course à 5 avec la sauvegarde active (fichier du banc), puis le retour au menu, puis une course quittée.
- **`configgame.README.md`**, et la description de la clé dans `src/core/configgame.{h,cpp}` (2 lignes) : `gain_course` = secours (RG-02). `configgame.json` ne change pas (aucune valeur touchée).
- **Tests** : `tests/test_boutique.cpp` (crédit, quittée, double crédit, secours, entrée du score des 4 modes, banc à 3 joueurs, déterminisme, barème changé) ; `tests/test_profil.cpp` (historique avec les ★, anciennes lignes) ; `CMakeLists.txt` (`test_boutique` lie `boutique_banc.cpp` et `score_course.cpp`).
- **À l'écran** : rien ne change encore (le détail du score et « +N ★ » sont WFS-537) ; le solde du garage et du market suit les nouvelles étoiles.

## Vérification
- Branche synchronisée avec `main` (f797b278, cadeau du jour WFS-500 compris). Construction complète locale, puis `bash tools/qa/tests_cpu.sh build` : **126 tests réussis**, aucun échec (`qa/pr/WFS-536/tests-cpu.log`).
- **Banc `equilibrage`** (barème de départ, sans rien changer) :
  - **joueur moyen** : 9,99 ★ par course, 1re voiture en **26 courses**, toutes les voitures en **120,4 h** → OK ;
  - **bon** : 12,68 ★, 19 courses, 94,9 h ;
  - **débutant** : 7,89 ★, **29 courses** (≤ 40), 151,1 h.
- **Banc `score-etoiles`** (vraie partie, Ville, course à 5 de 2 km) :
  - course à 5 terminée en 2e, 0 choc, 1re course = 1 050 points → **12 ★**, solde 0 → **12 ★** ;
  - retour au menu : toujours 12 ★ (aucun 2e crédit) ;
  - course quittée après 8 s : 0 ★, solde inchangé, notée « Quittée ;…;0 » dans l'historique.

## Limites et risques
- **Défis pas encore comptés** : les modes ne donnent pas encore leurs compteurs (dépassements, contacts avec la police, nitro, checkpoints sans retard, « jamais derrière »). Ces défis valent 0 point, comme le prévoit RG-04 de WFS-535. Le banc simule exactement ce que le jeu donne aujourd'hui. À proposer au PO : un ticket pour brancher ces compteurs. Il faudra ensuite revoir `points_par_etoile` au banc, car le joueur moyen gagnerait plus.
- **40 ★ jamais atteintes** : avec le barème de départ, le maximum est **18 ★** au banc (21 avec les défis). C'est le risque déjà signalé par WFS-535. Le PO doit choisir : élargir le barème ou passer à une conversion non linéaire. Ce ticket ne change aucune valeur, car l'équilibrage du joueur moyen tient.
- **Records par mode, pas encore par carte** : ils deviennent par carte avec WFS-538. Pour la course solo, le score tient lieu de temps.
- **Duel** : un adversaire est compté comme parti quand il disparaît des fantômes pendant la manche sans être arrivé. Non essayé en réseau réel ici, seulement par le calcul (test CPU).
- **Pas de capture « avant » pour les bancs `score-etoiles` et course quittée** : ce banc n'existe pas sur `main`. Sur `main`, une course terminée donne toujours 10 ★.

## Captures pour le lead dev (4 planches + 2 images du jeu)
Dossier : [`qa/pr/WFS-536`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-536/qa/pr/WFS-536). Produit par `tools/qa/captures_pr.sh WFS-536 score-etoiles:1280x720 equilibrage` ; le texte des bancs est ajouté sous les images.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| 1. banc `equilibrage` (3 joueurs) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-536/qa/pr/WFS-536/avant-1-equilibrage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-536/qa/pr/WFS-536/apres-1-equilibrage.jpg?raw=true) |
| 2. course à 5 : solde avant / après | — (10 ★ fixes sur `main`) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-536/qa/pr/WFS-536/apres-2-course5-solde.jpg?raw=true) |
| 3. course quittée : solde inchangé | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-536/qa/pr/WFS-536/apres-3-course-quittee.jpg?raw=true) |
| écran de fin (image seule) | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-536/qa/pr/WFS-536/apres-score-course5-fin.jpg?raw=true) |
| menu après la course quittée (image seule) | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-536/qa/pr/WFS-536/apres-score-quittee.jpg?raw=true) |

Textes complets : `apres-equilibrage.txt`, `avant-equilibrage.txt`, `apres-score-etoiles.txt`.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv3A5c1kqBucUa5xkdy
