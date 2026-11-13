Agent : Claude, agent dev3d (01Us6uCvEv)

**Ticket** : WFS-506 (P1, épic E20) — règles : `regles-de-gestion/WFS-506.md` et `regles-de-gestion/E20-langue-anglaise.md` (RC-E20-01 à 07)

## Demande de l'utilisateur
Sur un téléphone qui n'est pas en français, tout le jeu doit être en anglais. Ce ticket traduit la **course** : HUD, messages, consignes de la carte de navigation et fin de course des 4 modes.

## Ce qui change
- **Textes** : environ 120 clés ajoutées en français et en anglais (`assets/textes/fr.txt`, `en.txt`). Les deux tables ont les mêmes clés, sans doublon, et le bloc de WFS-505 est gardé.
- **HUD** (`src/ui/hud.cpp`) : Temps / Time, Arrivée / Finish, Checkpoint n, Score, Combo, km/h, Nitro.
- **Commandes** (`commandes_tactiles.cpp`) : GAZ / GAS, FREIN / BRAKE, NITRO.
- **Bouton de vue** (`camera_vue.cpp`) : ARRIÈRE / CHASE…, clés `vue.*` de WFS-504.
- **Étiquette du fantôme** (`fantome_etiquette.cpp`) : VOTRE RECORD / YOUR BEST.
- **Bandeaux** (`toasts.cpp`, `trace_cartes_systeme.cpp`) : quasi-contact / near miss, accident, combo perdu, checkpoint, feu grillé, nouveaux tracés.
- **Carte de navigation** (`minicarte.cpp`, RG-02) :
  - consignes « Sharp right », « Hairpin left », « At the roundabout, take the 3rd exit », « Finish », « Now » ;
  - distances en mètres, comme avant ;
  - les noms de rues inventés sont inchangés (« Rue du Belvédère »).
- **Fin de course** (RG-03) :
  - Checkpoint (`course_system.cpp`) : TIME'S UP, statistiques, RESTART / MENU, NEW RECORD, BEST ;
  - Course à 5 (`course5_system.cpp`) : 5-CAR RACE · 2 KM, 2nd PLACE, Finished, Driver, record « 1st place · … » ;
  - Poursuite (`poursuite_system.cpp`) : POLICE PURSUIT, VICTORY / BUSTED, lignes du résultat, annonces (POLICE BACKUP, POLICE ROADBLOCK, TRAFFIC JAM…), pertes de points « -5 HIT TRAFFIC » ;
  - Duel (`duel_system.cpp`) : DUEL OVER, 2nd PLACE, « Alex (you) », REMATCH / BACK, POSITION · RECONNECTING… / MS.
- **Allocations (RG-04)** : `src/game/textes_course.h` (nouveau) ajoute des petites aides :
  - `remplir()` : valeurs écrites dans un tampon réutilisé ;
  - `Capitales` : capitales gardées tant que la langue ne change pas ;
  - `ordinal()` et `titrePlace()`.
  Aucune allocation n'est ajoutée par image ; plusieurs concaténations par image disparaissent (HUD, Position n/N).
- **Taille des boutons (RC-E20-04)** : elle ne change pas.
  - Les boutons « Menu » et de vue sont mesurés sur le texte français (comme la pause de WFS-505).
  - Le texte des boutons de fin passe par `ui::textBoite` : réduit de 20 % au plus s'il dépassait. Aucun ne dépasse dans le banc.
- **Logique inchangée** : `poursuite.cpp`, `navigation.h` et `duel.cpp` gardent leurs textes français, qui servent de référence à `test_poursuite`, `test_navigation` et `test_duel` et au journal. Seul l'affichage passe par `ui::tr`.
- **Banc `textes` étendu (RG-05)** : `src/game/textes_course_banc.cpp` (nouveau), avec deux accroches de 3 lignes dans `src/platform/desktop_main.cpp`. Le groupe `textes` lance maintenant CGU, Réglages, menus, puis course ; le groupe `textes-course` lance la course seule.
  - Checkpoint, Course à 5 et Poursuite : course → fin → Rejouer → course → fin → Menu → menu principal.
  - Duel en mémoire : course → résultat → Revanche → 2e manche → résultat → Retour → menu principal.
  - Les noms inventés (rues, pseudos, adversaires) sont notés à part avec `NomPropre` de WFS-505, réutilisé.
- **Petites fonctions de banc ajoutées** : `courseBouton()` (`course_system.*`), `poursuiteBancMessages()` (`poursuite*`), `benchDuelTextes()` (`duel_system.*`).

## Comment c'est vérifié
- **Construction** complète et **122 tests CPU verts** sur `ce8cb4fa`, branche synchronisée avec `main` après les fusions de WFS-505 et WFS-508 (voir « Tests CPU » ci-dessous et `qa/pr/WFS-506/tests-cpu.log`).
- **Banc `textes` complet, téléphone en anglais** (`WFS_LANGUE=en`) : VERT.
  - CGU (WFS-508), Réglages (WFS-504), menus (WFS-505) et course (WFS-506) : 0 phrase française, 0 texte qui dépasse.
  - Parcours : 0 écart dans les 4 modes.
- **Même banc en français** (avant la fusion de WFS-508) : VERT, et le français est inchangé (voir les captures `textes-fr-*`).
- **Captures avant / après** (bancs `hud` et `course5` en 1280x720, `WFS_LANGUE=en`) : seule la langue des textes change ; décor, voitures, HUD et places sont identiques au pixel près.
- **Performance** : banc de chauffe sur `main` et sur la branche (`chauffe-main.txt`, `chauffe-branche.txt`) : vert des deux côtés, mêmes appels de dessin et mêmes triangles (545 appels, 874 k ; 330 appels, 518 k), durée 145 s contre 146 s.

## Limites et risques
- **Images par seconde réelles non mesurées** : le PC de test fait le rendu en logiciel (lavapipe). Le code par image ne fait que remplacer des textes écrits en dur par des références aux tables. L'endurance et la chauffe de la CI de `main` le confirmeront après la fusion.
- **Captures avant / après prises juste avant la synchronisation avec WFS-505 et WFS-508**. Ces deux tickets ne touchent pas aux écrans de course, et les captures du banc `textes` ont été refaites après la synchronisation.
- Le banc `textes` ne tourne pas dans la CI aujourd'hui (comme pour WFS-504 et 505) : il se lance à la main.
- **Hors périmètre** : textes de mesure technique (ligne « Rendu : … » du menu caché), garage (WFS-507), CGU (WFS-508). Les nombres gardent la virgule française (« 0,07 km ») : formats des nombres hors de l'épic E20.
- **Accroche hors zone** : 3 lignes dans `desktop_main.cpp` (groupes de banc), annoncées au journal.

## Tests CPU

![Tests CPU](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-tests-cpu.jpg?raw=true)

## Captures pour le lead dev

Dossier : [`qa/pr/WFS-506`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506). Le banc `hud` et `course5` est en anglais (avant = `main`, textes français ; après = branche).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `hud-combo` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/avant-hud-combo.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-hud-combo.jpg?raw=true) |
| `hud-course5-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/avant-hud-course5-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-hud-course5-jour.jpg?raw=true) |
| `hud-course5-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/avant-hud-course5-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-hud-course5-nuit.jpg?raw=true) |
| `hud-cockpit-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/avant-hud-cockpit-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-hud-cockpit-nuit.jpg?raw=true) |
| `minicarte-ville-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/avant-minicarte-ville-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-minicarte-ville-jour.jpg?raw=true) |
| `minicarte-ville-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/avant-minicarte-ville-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-minicarte-ville-nuit.jpg?raw=true) |
| `minicarte-campagne-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/avant-minicarte-campagne-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-minicarte-campagne-jour.jpg?raw=true) |
| `toast-pile` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/avant-toast-pile.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-toast-pile.jpg?raw=true) |
| `course5-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/avant-course5-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-course5-course.jpg?raw=true) |
| `course5-fin` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/avant-course5-fin.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-course5-fin.jpg?raw=true) |
| `course5-rejouer` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/avant-course5-rejouer.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-course5-rejouer.jpg?raw=true) |
| `poursuite-hud` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/avant-poursuite-hud.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-poursuite-hud.jpg?raw=true) |
| `poursuite-surprise` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/avant-poursuite-surprise.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-poursuite-surprise.jpg?raw=true) |
| `poursuite-arrete` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/avant-poursuite-arrete.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-poursuite-arrete.jpg?raw=true) |
| `poursuite-victoire` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/avant-poursuite-victoire.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-poursuite-victoire.jpg?raw=true) |

**Banc `textes-course`** (nouveau groupe, branche seulement : français inchangé, anglais) :

| Capture | Après (branche) |
|---|---|
| `textes-en-course-checkpoint` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-en-course-checkpoint.jpg?raw=true) |
| `textes-en-course-course5` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-en-course-course5.jpg?raw=true) |
| `textes-en-course-course5-nuit` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-en-course-course5-nuit.jpg?raw=true) |
| `textes-en-course-poursuite` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-en-course-poursuite.jpg?raw=true) |
| `textes-en-duel-course` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-en-duel-course.jpg?raw=true) |
| `textes-en-duel-fin` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-en-duel-fin.jpg?raw=true) |
| `textes-en-fin-checkpoint` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-en-fin-checkpoint.jpg?raw=true) |
| `textes-en-fin-course5` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-en-fin-course5.jpg?raw=true) |
| `textes-en-fin-poursuite` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-en-fin-poursuite.jpg?raw=true) |
| `textes-en-fin-poursuite-arrete` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-en-fin-poursuite-arrete.jpg?raw=true) |
| `textes-fr-course-checkpoint` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-fr-course-checkpoint.jpg?raw=true) |
| `textes-fr-course-course5` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-fr-course-course5.jpg?raw=true) |
| `textes-fr-course-course5-nuit` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-fr-course-course5-nuit.jpg?raw=true) |
| `textes-fr-course-poursuite` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-fr-course-poursuite.jpg?raw=true) |
| `textes-fr-duel-course` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-fr-duel-course.jpg?raw=true) |
| `textes-fr-duel-fin` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-fr-duel-fin.jpg?raw=true) |
| `textes-fr-fin-checkpoint` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-fr-fin-checkpoint.jpg?raw=true) |
| `textes-fr-fin-course5` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-fr-fin-course5.jpg?raw=true) |
| `textes-fr-fin-poursuite` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-fr-fin-poursuite.jpg?raw=true) |
| `textes-fr-fin-poursuite-arrete` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-506/qa/pr/WFS-506/apres-textes-fr-fin-poursuite-arrete.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01Us6uCvEvngLZWcCkMPKtFB
