## Agent : Claude, agent dev conduite

**Ticket** : WFS-258 — règles : `regles-de-gestion/WFS-258.md`

## Description (à remplir par le développeur avant d'ouvrir la PR)

- **Demande de l'utilisateur** (en une phrase) : À REMPLIR
- **Ce qui change** (conduite du jeu, l'ancienne depuis RG-00 ; aide **activée** seulement) :
  - `src/game/player.cpp` : **l'aide ralentit à temps, volant lâché ou presque** : au-dessus de la vitesse permise pour les virages des 600 m qui viennent, gaz et nitro coupés, puis frein. Vitesse permise = la plus basse de (a) l'adhérence et le freinage de cette conduite (`Phys::grip` 18 m/s², `Phys::brake` 12,5 m/s² : si WFS-259 change l'adhérence, l'aide suit) et (b) 90 % de la vitesse où un ordinateur de la Course à 5 reste sur la chaussée (`course5::vitesseVirage`, branchée par un pointeur de fonction, accroche de 2 lignes dans `course5.cpp`).
  - **La limite cède quand le joueur conduit lui-même** (décision écrite du PO, option A) : **seuils** : doigt ou flèche à **0,35 et plus** (`VOLANT_PLEIN`, le même seuil où l'aide lui laisse déjà tout le volant depuis WFS-217 ; une flèche vaut 1) **ou frein appuyé**. En dessous (volant lâché ou presque), l'aide ralentit ; au-dessus, le joueur garde le gaz et peut sortir s'il exagère.
  - `guider` : **maintien de voie** : volant lâché, l'aide ramène la voiture vers la file où le joueur l'a laissée (avant, la dérive vers l'extérieur des virages n'était jamais corrigée) ; seulement la part de l'aide (elle cède dès que le joueur braque, RG-03 de WFS-217). **La file est reprise à la position de la voiture** au `reset()`, sous 5 m/s (grille, arrêt) et quand un autre système a déplacé la voiture depuis le pas précédent (placement sur la grille, remise en piste, choc) : correction du refus du lead dev.
  - **Aide désactivée : rien ne change** (RG-07 de WFS-241). `Phys::grip` n'est pas touché (décision du PO : WFS-259). Pilote automatique : même règle qu'avant (`vitessePilote`, 0,6 g / 0,7 g, déplacée dans `player.h` sous le nom `vitesseVirageAide`).
  - Banc `virages` : suite `gp-aide-avant/-apres` et classement en Course à 5 sur les 4 cartes, volant lâché ou presque (pilote automatique du jeu) et volant tenu (« Glisser ») ; interrupteur `g_avantWfs258` (banc seulement, faux dans le jeu) pour rejouer `main`, vérifié par le test (945 images hors route, comme `main`).
- **Comment c'est vérifié** :
  - `test_aide_ralentit` (nouveau) : 1er virage du Grand Prix lancé à 228 km/h, Gallardo et Golf, volant lâché, gaz tenu ou lâché : **0 image hors route** (`main` : 945) ; aide désactivée : l'aide ne freine jamais ; **tours des 4 cartes volant lâché : 0 image hors route** ; **départ de la grille** (voiture posée à x = 0,5, aide activée, volant lâché, comme dans l'essai du lead dev) : cap 0,000 rad et x immobile, aussi après une course finie au bord gauche (sur 9833d7d : cap 0,30 rad, x de 0,50 à 0,05).
  - 70 tests CPU OK (dont `test_aide_direction`, `test_conduite`, `test_course`, `test_virages_fleches`).
- **Temps au tour, gaz tenu, pour l'utilisateur** (simulation, Golf / Gallardo ; « hors route » = images de 1/120 s) :

| Carte | `main`, « Glisser » | Après, volant lâché (aide) | Après, volant tenu (« Glisser ») |
|---|---|---|---|
| Grand Prix | 115 s / 98 s (7 s / 10 s hors route) | 137 s / 123 s, 0 sortie | 134 s / 120 s (Gallardo 0,3 s hors route) |
| Campagne | 133 s / 110 s (8 s / 9 s) | 152 s / 131 s, 0 sortie | 139 s / 119 s, 0 sortie |
| Ville | 122 s / 97 s (5 s / 6 s) | 144 s / 127 s, 0 sortie | 137 s / 119 s (Gallardo 1,8 s hors route) |
| Corniche | 109 s / 95 s (10 s / 12 s) | 135 s / 124 s, 0 sortie | 122 s / 111 s (Gallardo 0,2 s hors route) |

- **Course à 5, 2 km, joueur gaz tenu avec l'aide (banc `virages`, tirage fixe)** — condition du PO : volant lâché → 0 sortie ; volant tenu → le joueur peut suivre les adversaires :

| Carte | Volant tenu (« Glisser ») : `main` → après | Pilote automatique du jeu (volant presque lâché) : `main` → après |
|---|---|---|
| Ville | 2/5, 49,2 s, 176 images hors route → **4/5**, 54,3 s, 87 | 5/5, 66,0 s, 854 → **5/5**, 59,0 s, 0 |
| Campagne | 2/5, 46,7 s, 96 → **2/5**, 46,9 s, 0 | 2/5, 47,1 s, 102 → **2/5**, 52,0 s, 0 |
| Corniche | 2/5, 51,1 s, 260 → **2/5**, 50,9 s, 0 | 2/5, 51,7 s, 269 → **4/5**, 56,3 s, 24 |
| Grand Prix | 2/5, 46,7 s, 256 → **4/5**, 50,7 s, 0 | 2/5, 47,0 s, 260 → **4/5**, 51,1 s, 0 |

- **Limites et risques** :
  - Le pilote automatique du jeu braque parfois au-delà de 0,35 : la limite lui cède alors (24 images hors route à la Corniche). Un joueur qui lâche vraiment le volant reste à 0 sortie (test CPU).
  - Volant tenu, le joueur va plus vite que l'aide et peut sortir s'il exagère (Gallardo en Ville : 1,8 s hors route par tour avec mon joueur simulé).
  - Les ordinateurs prennent l'épingle du Grand Prix à ≈ 6 g (table du jeu arcade) : étape 4 « même physique pour les adversaires » (décision du PO), pas WFS-258.
  - Point 3 (direction moins vive à haute vitesse, sans toucher l'adhérence) et suite de RG-00b : prochaine PR. RG-05/06/07 (adhérence) : WFS-259.
  - À confirmer par l'utilisateur sur son téléphone (🟡).

## Captures pour le lead dev (16 après)

Dossier : [`qa/pr/WFS-258`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `virages-gp-aide-apres-00` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-00.jpg?raw=true) |
| `virages-gp-aide-apres-01` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-01.jpg?raw=true) |
| `virages-gp-aide-apres-02` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-02.jpg?raw=true) |
| `virages-gp-aide-apres-03` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-03.jpg?raw=true) |
| `virages-gp-aide-apres-04` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-04.jpg?raw=true) |
| `virages-gp-aide-apres-05` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-05.jpg?raw=true) |
| `virages-gp-aide-apres-06` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-06.jpg?raw=true) |
| `virages-gp-aide-apres-07` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-apres-07.jpg?raw=true) |
| `virages-gp-aide-avant-00` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-00.jpg?raw=true) |
| `virages-gp-aide-avant-01` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-01.jpg?raw=true) |
| `virages-gp-aide-avant-02` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-02.jpg?raw=true) |
| `virages-gp-aide-avant-03` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-03.jpg?raw=true) |
| `virages-gp-aide-avant-04` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-04.jpg?raw=true) |
| `virages-gp-aide-avant-05` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-05.jpg?raw=true) |
| `virages-gp-aide-avant-06` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-06.jpg?raw=true) |
| `virages-gp-aide-avant-07` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-gp-aide-avant-07.jpg?raw=true) |
