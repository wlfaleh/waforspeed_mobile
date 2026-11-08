## Agent : Claude, agent dev conduite

**Ticket** : WFS-327 (P1, ANO-176) — règles : `regles-de-gestion/WFS-327.md` (Suivi : décision du PO, option (A) gardée car RG-02 tenue).

## Description

- **Demande de l'utilisateur** (captures APK 916) : en Course à 5, il finit toujours dernier ; les 4 adversaires sont « Arrivé » quand il est encore à 0,14 km, ou 1,1 à 1,6 km derrière.
- **Cause** : les ordinateurs suivaient une table « arcade » (`course5::vitesseVirage`) : épingle de 20 m du Grand Prix à ≈ 127 km/h (≈ 6 g) quand le joueur (et une vraie voiture) la passe à ≈ 50 km/h.
- **Ce qui change** (adversaires seulement ; **rien dans la conduite du joueur**, RG-03) :
  - `src/game/course5.cpp` : **`vitesseVirageOrdinateur(piste, s, voiture)`** (RG-01) : jamais au-dessus de la vitesse de virage physiquement possible pour la voiture de l'ordinateur : **même règle que l'aide du joueur** (`vitesseVirageAide`, virages des 600 m qui viennent), avec l'adhérence du jeu (`Phys::grip`) à l'échelle de la fiche de la voiture (μ de la voiture / μ de la Golf) et le freinage du jeu ; au plus l'ancienne table. **`vitesseVirageOrdinateurDevant`** : regarde aussi 4 et 8 m plus loin (la vitesse permise est calculée par paliers de 4 m : sans cela, un palier de retard, jusqu'à + 20 % mesuré).
  - `src/core/configgame.h` : **vitesse visée en ligne droite** : Normale 0,90 → **0,95**, Difficile 0,97 → **1,00** (Facile inchangé 0,80) : décision du PO (option A, « ≈ + 3 % ») pour garder les seuils de `test_course5` (protocole B : un bon pilote ne gagne pas toujours) : + 3 % ne suffisait pas pour la Mustang (place moyenne 1,4 en Normale) ; **+ 5,5 % en Normale** (à signaler au PO), + 3 % en Difficile ; RG-02 reste tenue.
  - `tests/test_course5.cpp` : le pilote du protocole B freine à la **même vitesse permise que les ordinateurs** (`vitesseVirageOrdinateurDevant`) ; seuils inchangés.
  - `tests/test_course5_ordinateurs.cpp` (nouveau) : RG-01 et RG-02.
- **Comment c'est vérifié** :
  - **RG-02** (`test_course5_ordinateurs`, course simulée sans GPU, conduite du jeu, Golf, Normale, 2 km, 8 tirages par carte) :

| Carte | Aide activée, volant lâché : dans les 3 premiers | Sans aide (joueur simulé fruste) : victoires | Ordinateurs au-dessus de leur vitesse permise |
|---|---|---|---|
| Ville | **7 / 8** (`main` : 0 / 8, dernier 8 fois) | 1 / 8 | + 1,6 % au plus |
| Campagne | **7 / 8** (`main` : 1 / 8) | 1 / 8 | 0 % |
| Corniche | **8 / 8** (`main` : 0 / 8) | 1 / 8 | 0 % |
| Grand Prix | **6 / 8** (`main` : 0 / 8) | 1 / 8 | 0 % |

  - `test_course5` (protocole B, 7 voitures × 3 difficultés × 5 graines) : tous les seuils tenus (M4 1,8 / 1,8 ; Mustang 1,6 / 2,4 ; Golf 2,6 / 3,0 ; Compass 4,4 / 4,8) ; RG-02 bis : ordinateur à 100 % = pilote qui freine à la même vitesse permise, + 1,9 à + 2,4 % (≤ 3 %).
  - 73 tests CPU OK.
  - **Dans la partie** (banc `course5`, Ville, 2 km, joueur au pilote automatique, mêmes captures sur `main`) : **avant 5e place** (vainqueur en 40,0 s, joueur + 14,1 s) ; **après 3e place** (vainqueur en 51,2 s, joueur + 2,9 s, deux adversaires encore derrière).
- **Limites et risques** :
  - Les ordinateurs sont plus lents dans les virages serrés et un peu plus rapides en ligne droite : la course se joue davantage dans les virages ; le classement réel est à juger par l'utilisateur sur son téléphone (🟡).
  - Le banc `course5` du jeu ne fait qu'une course (Ville) ; les 4 cartes sont mesurées par la course simulée du test.
  - La mesure Course à 5 de WFS-258 (doigt tenu) sera refaite après la fusion (demande du PO).
  - Duel et Poursuite : non touchés (`avancer` de la Course à 5 seulement).

## Captures pour le lead dev (40 images : 20 de `main`, 20 de la branche ; banc `course5`)

`tools/qa/captures_pr.sh` (`FILTRE='course5-*'`), vraies images « avant » de `main`. Tableau complet : `qa/pr/WFS-327/README.md`.

### Grille, course (4 s), arrivée et classement (Ville, 2 km)
| Avant (main) | Après |
|---|---|
| ![avant-course5-grille](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-grille.jpg?raw=true) | ![apres-course5-grille](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-grille.jpg?raw=true) |
| ![avant-course5-course](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-course.jpg?raw=true) | ![apres-course5-course](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-course.jpg?raw=true) |
| ![avant-course5-fin](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-fin.jpg?raw=true) | ![apres-course5-fin](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-fin.jpg?raw=true) |
| ![avant-course5-rejouer](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-rejouer.jpg?raw=true) | ![apres-course5-rejouer](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-rejouer.jpg?raw=true) |

### Départ sur les 4 cartes (vue de dessus, au « 3 »)
| Avant (main) | Après |
|---|---|
| ![avant-course5-depart-ville-dessus](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-depart-ville-dessus.jpg?raw=true) | ![apres-course5-depart-ville-dessus](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-depart-ville-dessus.jpg?raw=true) |
| ![avant-course5-depart-campagne-dessus](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-depart-campagne-dessus.jpg?raw=true) | ![apres-course5-depart-campagne-dessus](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-depart-campagne-dessus.jpg?raw=true) |
| ![avant-course5-depart-plage-dessus](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-depart-plage-dessus.jpg?raw=true) | ![apres-course5-depart-plage-dessus](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-depart-plage-dessus.jpg?raw=true) |
| ![avant-course5-depart-circuit-dessus](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/avant-course5-depart-circuit-dessus.jpg?raw=true) | ![apres-course5-depart-circuit-dessus](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-327/qa/pr/WFS-327/apres-course5-depart-circuit-dessus.jpg?raw=true) |
