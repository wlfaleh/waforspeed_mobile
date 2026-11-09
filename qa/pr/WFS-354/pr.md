Agent : Claude, agent dev Y

**Ticket** : WFS-354 [TECHNIQUE]. Suite de WFS-334, d'après les remarques non bloquantes du lead dev sur la PR #293. Règles : `regles-de-gestion/WFS-354.md`.

## Description

- **Demande** : finir la pluie en vue Cockpit. Plus de goutte sur le montant ; gouttes effacées au passage du balai dessiné ; plus de pluie dans la bouche du tunnel.
- **Ce qui change**
  - **RG-01** : `cockpit::coinsVitre` (`vehicule_cockpit.h`), appelée par `pareBriseVu()`.
    - La vitre s'arrête au bord intérieur des montants dessinés (axe ± demi-section de 7,5 cm), plus 1 cm de joint, en bas comme en haut.
    - Avant : en bas, le bord était sur l'axe du montant ; en haut, un retrait de 12 %.
    - Le dessin des montants n'est pas touché. Le test reprend ses cotes : `LARGEUR_MONTANT`, `RETRAIT_MONTANT_HAUT`.
  - **RG-02** (`meteo.cpp`) : la pluie n'a plus sa propre copie des balais (`essuie_` retiré).
    - À chaque image dessinée, `balayerVitre` lit l'angle des balais **dessinés** (`pareBriseVu().angle`, écrit par l'intérieur à la même image).
    - Le balayage ne se fait donc qu'en vue Cockpit, avec 0 image d'écart entre le balai vu et l'effacement.
  - **RG-03** (`meteo.*`) : la coupure du tunnel suit la **position de la caméra** le long de la route (voiture + écart caméra-voiture projeté sur la route).
    - Le fondu de 8 m est gardé : il va de 7 m avant la bouche à 1 m sous la voûte.
    - Avant, il suivait la voiture : en vue Cockpit, la pluie tombait encore 6 m sous la voûte.
  - Vitres latérales en vue Cockpit : la marge tient compte de la largeur du montant à l'écran (`montantPx_`).
  - **Banc `WFS_BANC=pluie`** (`benchPluie` dans `meteo.cpp`), branché par une accroche E7 de 2 lignes dans `desktop_main.cpp`, annoncée au journal.
- **Comment c'est vérifié**
  - `test_pluie` vert :
    - la vitre reste hors des montants pour les 20 intérieurs (10 mm de jeu au plus près pour la Golf, le X5 et l'Abarth ; avant, 37,5 mm d'empiètement en bas) ;
    - il ne pleut plus 1 m sous la voûte ;
    - le fondu reste de 8 m.
  - Banc `pluie` : 133 et 157 gouttes effacées par les balais dessinés en Cockpit, 0 en vue Arrière (avant : 137 « effacées » par la copie des balais, même en vue Arrière).
  - Coût de la pluie : 0,030 à 0,033 ms par image en Cockpit, 0,007 ms en vue Arrière (seuil RG-04 : 0,05 ms).
  - Tests CPU verts et parcours complet 0 écart après synchronisation avec `main` (chiffres ci-dessous).
- **Limites et risques connus**
  - La suite des balais est de nuit : la pluie n'existe que la nuit en Ville, et le ticket demandait le jour.
  - Les gouttes déjà posées sur le pare-brise restent dans le tunnel jusqu'au passage du balai, comme dans une vraie voiture : seule la pluie qui tombe est coupée.
  - RG-02 suppose que l'intérieur est dessiné à chaque image ; c'est le cas en jeu. Une image non dessinée ne balaie rien.
  - Après la fusion, le ticket passe en 🟡 À valider (utilisateur), car il est né d'ANO-182.

## Captures pour le lead dev

Dossier : [`qa/pr/WFS-354`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354)

| Capture | Ce qu'on voit | Avant (main) | Après (branche) |
|---|---|---|---|
| `pluie-cockpit-105` | Cockpit Golf, nuit, 102 km/h (RG-01) : vue entière | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-cockpit-105.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-cockpit-105.jpg?raw=true) |
| `pluie-cockpit-105-zoom-montant` | **agrandissement du montant gauche** : avant, des gouttes posées sur le bord du montant ; après, aucune, la vitre commence au bord intérieur du montant (1 cm de joint) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-cockpit-105-zoom-montant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-cockpit-105-zoom-montant.jpg?raw=true) |
| `pluie-balais-1` | **balayage 1/5** (RG-02), balais à 10 % de leur course montante, 107 km/h | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-balais-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-balais-1.jpg?raw=true) |
| `pluie-balais-2` | balayage 2/5 (30 %) : sous le balai dessiné, la vitre est nette | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-balais-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-balais-2.jpg?raw=true) |
| `pluie-balais-3` | balayage 3/5 (50 %) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-balais-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-balais-3.jpg?raw=true) |
| `pluie-balais-4` | balayage 4/5 (70 %) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-balais-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-balais-4.jpg?raw=true) |
| `pluie-balais-5` | balayage 5/5 (90 %) : la zone balayée est vide, les gouttes restent au-dessus du balai | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-balais-5.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-balais-5.jpg?raw=true) |
| `pluie-tunnel-avant` | **tunnel en Cockpit** (RG-03) : 10 m avant la bouche, il pleut | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-tunnel-avant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-tunnel-avant.jpg?raw=true) |
| `pluie-tunnel-bouche` | caméra à la bouche : le fondu finit (avant : la pluie tombait encore 6 m) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-tunnel-bouche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-tunnel-bouche.jpg?raw=true) |
| `pluie-tunnel-3m` | caméra 3 m sous la voûte : avant, des traits de pluie ; après, aucun (les gouttes déjà posées sur le pare-brise restent jusqu'au balai) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-tunnel-3m.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-tunnel-3m.jpg?raw=true) |
| `pluie-tunnel-6m` | caméra 6 m sous la voûte : aucun trait | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-tunnel-6m.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-tunnel-6m.jpg?raw=true) |
| `pluie-tunnel-milieu` | milieu du tunnel : aucun trait | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-tunnel-milieu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-tunnel-milieu.jpg?raw=true) |
| `pluie-arriere-vitesse` | **témoin** : vue Arrière sous la pluie à 102 km/h : traits inclinés depuis le point de fuite (WFS-334), image comme sur main | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-arriere-vitesse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-arriere-vitesse.jpg?raw=true) |
