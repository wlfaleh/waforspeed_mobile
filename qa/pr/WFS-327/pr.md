## Agent : Claude, agent dev conduite

**Ticket** : WFS-327 (P1, ANO-176) — règles : `regles-de-gestion/WFS-327.md` (Suivi : décision du PO, option (A)).

## Description

- **Demande de l'utilisateur** (captures APK 916) : en Course à 5, il finit toujours dernier ; les 4 adversaires sont « Arrivé » quand il est encore loin derrière.
- **Cause** : les ordinateurs suivaient une table « arcade » (`course5::vitesseVirage`) : dans l'épingle de la Corniche, jusqu'à 130 km/h et 6 g latéral (banc, ci-dessous).
- **Ce qui change** (adversaires seulement ; **rien dans la conduite du joueur**, RG-03) :
  - `src/game/course5.cpp` : **`vitesseVirageOrdinateur(piste, s, voiture)`** (RG-01) : jamais au-dessus de la vitesse de virage physiquement possible pour la voiture : **même règle que l'aide du joueur** (`vitesseVirageAide`), adhérence du jeu (`Phys::grip`) à l'échelle de la fiche (μ de la voiture / μ de la Golf), freinage du jeu ; au plus l'ancienne table. **`vitesseVirageOrdinateurDevant`** : regarde aussi 4 et 8 m plus loin (paliers de 4 m).
  - **μ lu directement sur la fiche** : `physique::adherencePneu(c) = 0,88 + 0,022 × grip` (`src/game/conduite_monde.h`), seule fonction, partagée avec l'étalonnage (`etalonner`). Plus aucun étalonnage de voiture pendant la course ni au chargement (retour du lead dev, point 2).
  - `src/core/configgame.h` : vitesse visée en ligne droite : Normale 0,90 → **0,95**, Difficile 0,97 → **1,00** (Facile 0,80) : décision du PO (option A) pour garder les seuils de `test_course5`.
  - Banc `course5` (`src/game/course5_system.cpp`) : nouvelle séquence de l'épingle de la Corniche (avant / après, journal des vitesses) ; interrupteur `course5::g_avantWfs327` (banc seulement, faux en jeu).
  - `tests/test_course5.cpp` : le pilote du protocole B freine à la même vitesse permise que les ordinateurs ; seuils inchangés. `tests/test_course5_ordinateurs.cpp` (nouveau) : RG-01 et RG-02.
- **Comment c'est vérifié** (tous les chiffres viennent du dernier lancement, branche synchronisée avec `main`) :
  - **75 tests CPU OK** (`build/test_*`, lancés depuis la racine).
  - **RG-02** (`test_course5_ordinateurs`, course simulée sans GPU, Golf, Normale, 2 km, 8 tirages par carte) :

| Carte | Aide activée, volant lâché : dans les 3 premiers | Sans aide (joueur simulé fruste) : victoires | Ordinateurs au-dessus de leur vitesse permise |
|---|---|---|---|
| Ville | **7 / 8** | 1 / 8 | + 0,0 % |
| Campagne | **7 / 8** | 1 / 8 | + 0,0 % |
| Corniche | **7 / 8** | 1 / 8 | + 0,0 % |
| Grand Prix | **6 / 8** | 1 / 8 | + 0,0 % |

  - `test_course5` (protocole B) : seuils tenus (place moyenne Normale / Difficile : M4 1,8 / 1,8 ; Mustang 1,6 / 2,4 ; Golf 2,6 / 3,0 ; Compass 4,4 / 4,8) ; RG-02 bis : ordinateur à 100 % = pilote qui freine à la même vitesse permise, + 1,9 à + 2,4 %.
  - **Épingle de la Corniche** (banc `course5`, journal) : **avant jusqu'à + 87,5 %** au-dessus de la vitesse possible (ex. R3 berline 130 km/h pour 72 km/h possibles, 6,05 g) ; **après + 0,0 %** (70 à 72 km/h, ≈ 1,8 g, l'adhérence du jeu, celle de l'aide du joueur).
  - **À-coup au départ** : premier pas de `course5::avancer` après le « GO » (processus froid, Corniche, 4 ordinateurs) : **84 à 86 ms avant** (étalonnage de 4 voitures), **0,22 ms après** ; pas suivants 0,03 ms.
  - **Dans la partie** (banc `course5`, Ville, 2 km, joueur au pilote automatique, mêmes captures sur `main`) : **avant 5e place, après 3e place** (joueur arrivé en 54,1 s dans les deux cas) ; après « Rejouer » : avant 5e, après 4e.
- **Limites et risques** :
  - Les ordinateurs sont plus lents dans les virages serrés et un peu plus rapides en ligne droite ; le classement réel est à juger par l'utilisateur sur son téléphone (🟡).
  - Le banc de la partie ne fait qu'une course complète (Ville) ; les 4 cartes sont mesurées par la course simulée du test.
  - « [ERREUR] éclairage (WFS-213) : capture abandonnée » apparaît aussi sur `main` (même banc) : sans lien avec ce ticket.
  - Duel et Poursuite : non touchés. La mesure Course à 5 de WFS-258 (doigt tenu) sera refaite après la fusion.

## Captures pour le lead dev (44 après)

### Épingle de la Corniche (1re des lacets, R ≈ 22 m) : vue de dessus toutes les 0,75 s, 4 ordinateurs lancés à 90 km/h

Avant = comportement de `main` rejoué au banc par `course5::g_avantWfs327` (table arcade seule, niveaux 0,90 / 0,97) : `main` n'a pas cette séquence. Les vitesses sont au journal du banc (une image ne montre pas la vitesse).

| n° | Avant (table arcade) | Après (vitesse possible) |
|---|---|---|
| 1 | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-avant-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-apres-1.jpg?raw=true) |
| 2 | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-avant-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-apres-2.jpg?raw=true) |
| 3 | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-avant-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-apres-3.jpg?raw=true) |
| 4 | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-avant-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-apres-4.jpg?raw=true) |
| 5 | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-avant-5.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-apres-5.jpg?raw=true) |
| 6 | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-avant-6.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-apres-6.jpg?raw=true) |

### Toutes les captures du banc `course5`

Dossier : [`qa/pr/WFS-327`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `course5-adversaires-190m-basse-avant` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-adversaires-190m-basse-avant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-adversaires-190m-basse-avant.jpg?raw=true) |
| `course5-adversaires-190m-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-adversaires-190m-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-adversaires-190m-basse.jpg?raw=true) |
| `course5-adversaires-190m-haute-avant` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-adversaires-190m-haute-avant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-adversaires-190m-haute-avant.jpg?raw=true) |
| `course5-adversaires-190m-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-adversaires-190m-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-adversaires-190m-haute.jpg?raw=true) |
| `course5-adversaires-400m-basse-avant` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-adversaires-400m-basse-avant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-adversaires-400m-basse-avant.jpg?raw=true) |
| `course5-adversaires-400m-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-adversaires-400m-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-adversaires-400m-basse.jpg?raw=true) |
| `course5-adversaires-400m-haute-avant` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-adversaires-400m-haute-avant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-adversaires-400m-haute-avant.jpg?raw=true) |
| `course5-adversaires-400m-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-adversaires-400m-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-adversaires-400m-haute.jpg?raw=true) |
| `course5-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-course.jpg?raw=true) |
| `course5-depart-campagne-arriere` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-depart-campagne-arriere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-depart-campagne-arriere.jpg?raw=true) |
| `course5-depart-campagne-dessus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-depart-campagne-dessus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-depart-campagne-dessus.jpg?raw=true) |
| `course5-depart-circuit-arriere` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-depart-circuit-arriere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-depart-circuit-arriere.jpg?raw=true) |
| `course5-depart-circuit-dessus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-depart-circuit-dessus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-depart-circuit-dessus.jpg?raw=true) |
| `course5-depart-plage-arriere` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-depart-plage-arriere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-depart-plage-arriere.jpg?raw=true) |
| `course5-depart-plage-dessus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-depart-plage-dessus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-depart-plage-dessus.jpg?raw=true) |
| `course5-depart-ville-arriere` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-depart-ville-arriere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-depart-ville-arriere.jpg?raw=true) |
| `course5-depart-ville-dessus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-depart-ville-dessus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-depart-ville-dessus.jpg?raw=true) |
| `course5-epingle-apres-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-apres-1.jpg?raw=true) |
| `course5-epingle-apres-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-apres-2.jpg?raw=true) |
| `course5-epingle-apres-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-apres-3.jpg?raw=true) |
| `course5-epingle-apres-4` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-apres-4.jpg?raw=true) |
| `course5-epingle-apres-5` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-apres-5.jpg?raw=true) |
| `course5-epingle-apres-6` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-apres-6.jpg?raw=true) |
| `course5-epingle-avant-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-avant-1.jpg?raw=true) |
| `course5-epingle-avant-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-avant-2.jpg?raw=true) |
| `course5-epingle-avant-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-avant-3.jpg?raw=true) |
| `course5-epingle-avant-4` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-avant-4.jpg?raw=true) |
| `course5-epingle-avant-5` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-avant-5.jpg?raw=true) |
| `course5-epingle-avant-6` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-epingle-avant-6.jpg?raw=true) |
| `course5-fin` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-fin.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-fin.jpg?raw=true) |
| `course5-grille` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-grille.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-grille.jpg?raw=true) |
| `course5-rejouer` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-rejouer.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-rejouer.jpg?raw=true) |
| `modes-choix` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-modes-choix.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-modes-choix.jpg?raw=true) |
| `parcours5-difficile` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-parcours5-difficile.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-parcours5-difficile.jpg?raw=true) |
| `poursuite-arrete` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-poursuite-arrete.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-poursuite-arrete.jpg?raw=true) |
| `poursuite-bouchon` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-poursuite-bouchon.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-poursuite-bouchon.jpg?raw=true) |
| `poursuite-camion` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-poursuite-camion.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-poursuite-camion.jpg?raw=true) |
| `poursuite-hud` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-poursuite-hud.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-poursuite-hud.jpg?raw=true) |
| `poursuite-police-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-poursuite-police-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-poursuite-police-basse.jpg?raw=true) |
| `poursuite-police` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-poursuite-police.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-poursuite-police.jpg?raw=true) |
| `poursuite-ralentisseur` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-poursuite-ralentisseur.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-poursuite-ralentisseur.jpg?raw=true) |
| `poursuite-redemarrage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-poursuite-redemarrage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-poursuite-redemarrage.jpg?raw=true) |
| `poursuite-surprise` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-poursuite-surprise.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-poursuite-surprise.jpg?raw=true) |
| `poursuite-victoire` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-poursuite-victoire.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-poursuite-victoire.jpg?raw=true) |
