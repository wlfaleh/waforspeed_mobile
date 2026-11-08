## Agent : Claude, agent dev conduite

**Ticket** : WFS-258 (P1, ANO-131), **point 1 de RG-00b** : « aide activée, la voiture doit passer virages et ronds-points » — règles : `regles-de-gestion/WFS-258.md`. Livré seul, comme le repli RG-00 (#217).

## Description

- **Demande de l'utilisateur** : avec l'aide à la direction activée, la voiture doit passer les virages ; vidéo : Grand Prix, aide activée, 228 km/h, la voiture part tout droit dans le bac à sable au 1er virage (l'aide doit freiner / ralentir à temps).
- **Ce qui change** (conduite du jeu, l'ancienne depuis RG-00 ; aide **activée** seulement) :
  - `src/game/player.cpp` : **l'aide ralentit à temps** : au-dessus de la vitesse permise pour les virages des 600 m qui viennent, gaz et nitro coupés, puis frein. Vitesse permise = la plus basse de (a) l'adhérence et le freinage de cette conduite (`Phys::grip` 18 m/s², `Phys::brake` 12,5 m/s² : si WFS-259 change l'adhérence, l'aide suit) et (b) 90 % de la vitesse où un ordinateur de la Course à 5 reste sur la chaussée (`course5::vitesseVirage`, déjà calculée par le jeu ; branchée par un pointeur de fonction, accroche d'une ligne dans `course5.cpp`).
  - `guider` : **maintien de voie** : volant lâché, l'aide ramène la voiture vers la file où le joueur l'a laissée (avant, elle ne visait que la direction de la route : la dérive vers l'extérieur des virages n'était jamais corrigée). Seulement la part de l'aide : elle cède dès que le joueur braque (RG-03 de WFS-217).
  - **Aide désactivée : rien ne change** (aucun gaz coupé, aucun frein, aucune correction : RG-07 de WFS-241). `Phys::grip` n'est pas touché (décision du PO : WFS-259). Pilote automatique : même règle qu'avant (`vitessePilote`, 0,6 g / 0,7 g, déplacée dans `player.h` sous le nom `vitesseVirageAide`).
  - Banc `virages` : suite `gp-aide-avant/-apres` (Gallardo lancée à pleine vitesse, aide activée, gaz tenu, volant lâché) et classement en Course à 5 sur les 4 cartes ; interrupteur `g_avantWfs258` (banc seulement, faux dans le jeu) pour rejouer `main`, vérifié par le test (945 images hors route, comme `main`).
- **Comment c'est vérifié** :
  - `test_aide_ralentit` (nouveau) : 1er virage du Grand Prix lancé à 228 km/h, Gallardo et Golf, volant lâché ou tenu, gaz tenu ou lâché : **0 image hors route** (`main` : 945) ; aide désactivée : l'aide ne freine jamais ; **tours des 4 cartes, gaz tenu, volant lâché ou tenu (« Glisser ») : 0 image hors route**.
  - 70 tests CPU OK (dont `test_aide_direction`, `test_conduite`, `test_course`, `test_virages_fleches`).
- **Temps au tour, gaz tenu, pour l'utilisateur** (simulation, Golf / Gallardo) :

| Carte | `main` en « Glisser » (sorties de route par tour) | Après, aide activée (0 sortie) |
|---|---|---|
| Grand Prix | 115 s / 98 s (7 s / 10 s hors route) | 138 s / 123 s |
| Campagne | 133 s / 110 s (8 s / 9 s) | 153 s / 131 s |
| Ville | 122 s / 97 s (5 s / 6 s) | 144 s / 127 s |
| Corniche | 109 s / 95 s (10 s / 12 s) | 136 s / 125 s |

  Avec l'aide de `main`, volant lâché, la voiture sortait de la route dans chaque grand virage (tour de 260 à 580 s). Une aide réglée sur l'adhérence d'un vrai pneu (0,85 g) passait aussi sans sortie, mais en 153 à 175 s par tour (dernier partout en Course à 5).
- **Course à 5, 2 km, joueur gaz tenu avec l'aide (banc `virages`, tirage fixe)** — condition du PO : jamais systématiquement dernier :

| Carte | `main` | Après |
|---|---|---|
| Ville | 5/5, 66,0 s, 854 images hors route | **5/5, 60,5 s**, 0 hors route |
| Campagne | 2/5, 47,1 s, 103 hors route | **2/5, 52,0 s**, 0 hors route |
| Corniche | 2/5, 51,7 s, 269 hors route | **5/5, 58,6 s**, 0 hors route |
| Grand Prix | 2/5, 47,0 s, 260 hors route | **5/5, 53,7 s**, 0 hors route |

- **Limites et risques** :
  - En Course à 5, le joueur qui ne fait que tenir le gaz avec l'aide finit dernier sur 3 cartes sur 4 (sur `main` : 1 sur 4, mais en sortant de la route) : les ordinateurs prennent les virages à la vitesse de leur table (l'épingle de 20 m du Grand Prix à 127 km/h) ; l'aide reste à 90 % pour qu'un joueur qui braque ne sorte pas dans les chicanes. Un joueur qui freine et braque lui-même peut aller plus vite que l'aide.
  - Point 3 (direction moins vive à haute vitesse, sans toucher l'adhérence) et la suite de RG-00b : prochaine PR. RG-05/06/07 (adhérence) : WFS-259 (décision du PO).
  - À confirmer par l'utilisateur sur son téléphone (🟡).

## Captures pour le lead dev (16 images, banc `virages`, 1280 × 720, rendu de la CI)

Dossier : [`qa/pr/WFS-258`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258) — `tools/qa/captures_pr.sh` (`FILTRE='virages-gp-aide-*'`, `SANS_AVANT=1` : les images « avant » sont produites par le banc avec `g_avantWfs258`). Classement et mesures : `qa/pr/WFS-258/virages.txt`.

### 1er virage du Grand Prix (épingle de 20 m), Gallardo lancée à pleine vitesse, aide activée, gaz tenu, volant lâché ; 8 images de 150 m avant à 60 m après
Avant : 296 km/h à l'entrée, la voiture arrive en travers dans le virage (93 km/h, fumée). Après : l'aide freine de 236 à 97 km/h avant le virage (« aide qui freine : oui » au journal), la voiture passe dans sa file.

| Avant (main) | Après |
|---|---|
| ![avant-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-00.jpg?raw=true) | ![apres-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-00.jpg?raw=true) |
| ![avant-01](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-01.jpg?raw=true) | ![apres-01](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-01.jpg?raw=true) |
| ![avant-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-02.jpg?raw=true) | ![apres-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-02.jpg?raw=true) |
| ![avant-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-03.jpg?raw=true) | ![apres-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-03.jpg?raw=true) |
| ![avant-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-04.jpg?raw=true) | ![apres-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-04.jpg?raw=true) |
| ![avant-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-05.jpg?raw=true) | ![apres-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-05.jpg?raw=true) |
| ![avant-06](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-06.jpg?raw=true) | ![apres-06](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-06.jpg?raw=true) |
| ![avant-07](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-07.jpg?raw=true) | ![apres-07](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-07.jpg?raw=true) |
