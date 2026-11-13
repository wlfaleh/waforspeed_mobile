Agent : Claude, agent dev3d (01Us6uCvzs) (session_01Us6uCvzsxvoKDpDXD7tC4y)

**Ticket** : WFS-498 — règles : `regles-de-gestion/WFS-498.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : étape « voitures et couleurs gratuites » (R16) : une seule monnaie, les étoiles ; médailles = packs du market avec l'équivalent en médailles et en euros ; 208, 500 et GT86 gratuites ; 14 voitures de 250 à 6 000 ★ ; peintures et skins au prix de base × coefficient de la classe ; tout dans `configgame.json` ; essai gratuit ; banc d'équilibrage. **Ce ticket = les données et les calculs, sans écran** (garage : WFS-499).
- **Ce qui change** :
  - **nouveau module `src/game/boutique.h/.cpp`** : portefeuille (`solde`, `crediter`, `debiter` qui refuse sans rien changer, borné à 10^12 ★), `crediterCourse` (gain d'une course terminée), `equivalentMedailles` (centaine supérieure puis or, argent, bronze : 2 200 ★ → « 3 or + 1 argent + 1 bronze » ; 250 ★ → « 1 argent »), `equivalentCentimes` / `texteEuros` (2 200 ★ → 21,78 €), voitures (`voitureVendue`, `prixVoiture`, `voitureGratuite`, `possedeVoiture`, `acheterVoiture`), `acheterObjet` (achat des couleurs, finitions, skins et packs de WFS-487 avec le vrai portefeuille), essai (`essaiDisponible`, `marquerEssai`), migration des sauvegardes d'avant, calcul d'équilibrage ; `src/game/boutique_banc.cpp` : banc `WFS_BANC=equilibrage` (sans rendu, écrit `equilibrage.txt`, code 3 si hors bornes) ;
  - **`configgame.json` / `configgame.README.md`** (fichiers de `main` + **seulement les nouvelles clés de la section `boutique`** ; `git diff origin/main` de ces deux fichiers ne touche que `boutique`, les seules lignes retirées sont les `"classe": N` qui prennent une virgule) : `voitures.<id>.prix` (tableau RG-04, total 24 350 ★ ; x5, a45, elantra jamais vendues), `gain_course` = 10, `minutes_par_course` = 2,96, `medailles.bronze|argent|or` (100 ★ 0,99 € ; 300 ★ 2,49 € ; 600 ★ 4,49 €) ; valeurs par défaut dans `src/core/configgame.*` ;
  - **sauvegarde** (`sauvegarde.*`) : nouvelles clés `etoiles=`, `voitures-possedees=a|b`, `essai-<voiture>=1`, aucune clé ancienne retirée ; **sauvegarde d'avant** (sans clé `etoiles`) : la voiture choisie devient possédée (ses couleurs : migration de WFS-487), solde 0 ; **partie neuve** (aucun fichier) : voiture de départ **p208** ; les défauts du banc d'essai restent la Golf (captures inchangées) ; valeurs mal écrites ignorées sans erreur ;
  - **`profil_system.cpp`** : +10 ★ à la fin de chaque course terminée (pas quittée) ; au départ d'une course solo avec une voiture non possédée, son essai gratuit est noté (une seule fois ; jamais en duel) ;
  - **banc `sauvegarde`** (`course_system.cpp`) : le solde fictif de WFS-487 est remplacé par le vrai portefeuille (achat accepté, finition et voiture refusées faute d'étoiles), lignes `PORTEFEUILLE` aux deux lancements ;
  - **accroches** : 1 ligne + 1 include dans `src/platform/desktop_main.cpp` (banc `equilibrage`) ; `CMakeLists.txt` : `boutique.cpp` ajouté aux 17 tests qui lient déjà `personnalisation.cpp` (la sauvegarde l'appelle) + nouveau test `test_boutique`.
  - **À l'écran : rien ne change** (garage pixel pour pixel identique, voir plus bas).
- **Comment c'est vérifié** :
  - **`test_boutique`** (RG-12 et RG-11, 51 cas) : 3 voitures gratuites ; prix et classes des 17 voitures ; coefficients ; métallisé SF90 = 400 ★, candy Golf = 375 ★, pack sans coefficient ; équivalents médailles et euros ; crédit / débit / refus ; achat suffisant, insuffisant, déjà possédé, voiture retirée ; peinture achetée sur la Golf absente sur la M4 ; pack de dessins sur toutes ; essai une seule fois, voiture encore bloquée après, essai relu ; sauvegarde d'avant (voiture et couleurs gardées) ; partie neuve sur la p208 ; `configgame.json` mal écrit (prix négatif, gain en texte → valeur du code + message) ; banc d'équilibrage dans les bornes, puis Corsa à 400 ★ → 1re voiture T-Cross 30 courses, écart signalé ;
  - `test_sauvegarde` : partie neuve sur la p208 (ajout d'une vérification) ; `test_configgame` : `configgame.json` du dépôt relu, 0 erreur ; **les 122 tests CPU passent** (bloc « Tests CPU » ci-dessous) ;
  - **banc `equilibrage`** : [texte](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/texte-equilibrage.md) — 1re voiture (Corsa 250 ★) **25 courses**, les 14 voitures 24 350 ★ = 2 435 courses = **120,1 h** ; **même banc, prix changé dans `configgame.json` sans recompiler** : [texte](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/texte-equilibrage-prix-change.md) (Corsa 400 ★ → T-Cross 30 courses, code 3) ;
  - **banc `sauvegarde`** lancé deux fois : [texte](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/texte-sauvegarde.md) — 250 ★ ; rose sur la M4 achetée (100 ★) ; chromé (800 ★) et la M4 (2 200 ★ = 3 or + 1 argent + 1 bronze = 21,78 €) refusés faute d'étoiles, rien de débité ; essai de la M4 noté au départ ; +10 ★ à la fin ; relu : 160 ★, voitures p208 abarth gt86, essai m4 ;
  - **garage et course avant / après** : les **16 images** (13 du garage, `garage-course`, `course-p911-gris`, `fin-record`, `sauvegarde-relue`) sont **identiques octet pour octet** (`cmp`) entre `main` et la branche : à l'écran, rien ne change.
- **Limites et risques connus** :
  - pas d'écran : le garage laisse encore choisir et conduire toutes les voitures (cadenas et « Acheter » : WFS-499). D'ici là, conduire une voiture non possédée en solo **utilise son essai gratuit** (donnée seulement, rien ne se bloque) ;
  - le gain de 10 ★ par course est provisoire (étape « score ») ; il compte aussi les duels terminés ;
  - la sauvegarde garde `version=1` (nouvelles clés seulement, comme WFS-480 et 487) ;
  - le total « tout débloquer avec peintures et skins » (≈ 567 h) est donné pour information (RG-11), sans borne.

## Tests CPU

`bash tools/qa/tests_cpu.sh build` sur la branche synchronisée avec `main` (après la fusion de WFS-488), code testé : commit e5669098 (le commit suivant n'ajoute que `qa/pr/WFS-498/`). Journal complet : [`tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/tests-cpu.log). Aucun échec.

```
[OK] test_ville_lieux (0 s)
[OK] test_ville_sol (3 s)
[OK] test_virages_fleches (1 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 122 test(s) réussi(s) en 406 s
code de sortie : 0
```

## Captures pour le lead dev (16 après)

Dossier : [`qa/pr/WFS-498`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `course-p911-gris` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-course-p911-gris.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-course-p911-gris.jpg?raw=true) |
| `garage-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-garage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-garage-0.jpg?raw=true) |
| `garage-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-garage-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-garage-1.jpg?raw=true) |
| `garage-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-garage-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-garage-2.jpg?raw=true) |
| `garage-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-garage-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-garage-course.jpg?raw=true) |
| `garage-m4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-garage-m4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-garage-m4.jpg?raw=true) |
| `garage-peinture-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-garage-peinture-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-garage-peinture-0.jpg?raw=true) |
| `garage-peinture-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-garage-peinture-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-garage-peinture-1.jpg?raw=true) |
| `garage-peintures-p911` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-garage-peintures-p911.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-garage-peintures-p911.jpg?raw=true) |
| `garage-peintures` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-garage-peintures.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-garage-peintures.jpg?raw=true) |
| `garage-suivant` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-garage-suivant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-garage-suivant.jpg?raw=true) |
| `showroom-suivante-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-showroom-suivante-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-showroom-suivante-1.jpg?raw=true) |
| `showroom-suivante-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-showroom-suivante-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-showroom-suivante-2.jpg?raw=true) |
| `showroom-suivante-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-showroom-suivante-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-showroom-suivante-3.jpg?raw=true) |
| `fin-record` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-fin-record.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-fin-record.jpg?raw=true) |
| `sauvegarde-relue` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/avant-sauvegarde-relue.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-498/qa/pr/WFS-498/apres-sauvegarde-relue.jpg?raw=true) |


🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01Us6uCvzsxvoKDpDXD7tC4y
