Agent : Claude, agent dev Y

**Ticket** : WFS-354 [TECHNIQUE]. Suite de WFS-334, d'après les remarques non bloquantes du lead dev sur la PR #293. Règles : `regles-de-gestion/WFS-354.md`.

## Description

- **Demande** : finir la pluie en vue Cockpit. Plus de goutte sur le montant ; gouttes effacées au passage du balai dessiné ; plus de pluie dans la bouche du tunnel.
- **Ce qui change** (2e version, après le refus de la 1re revue)
  - **RG-01** : la vitre est calculée **telle que l'œil la voit**.
    - `vehicule_cockpit.h` : `coinsVitre` donne les axes des montants. `pareBriseVu()` les porte ; c'est 1 ligne remplacée dans `pareBriseVu()`.
    - `meteo::vitreVue` projette depuis l'œil, sur le plan de la vitre, les 4 arêtes de chaque barre de montant (75 × 50 mm, mêmes axes que `barre()`). Elle garde le bord le plus rentré qu'elles cachent, en bas et en haut, moins 16 mm : 1 cm de joint et 6 mm pour le rayon de la plus grosse goutte.
    - Vu de biais, le montant droit cache environ 10 cm de vitre en bas : 106 mm sur la Golf, 113 mm sur le X5, 99 mm sur l'Abarth.
  - **RG-02** : la pluie n'a plus sa propre copie des balais. À chaque image dessinée, `balayerVitre` lit l'angle des balais dessinés (`pareBriseVu().angle`). Au retour en Cockpit, l'angle de départ est remis à l'angle courant, donc rien n'est balayé d'un coup (remarque a).
  - **RG-03** : la coupure du tunnel suit la **position de la caméra** le long de la route.
    - Le fondu de 8 m s'arrête **à la bouche**.
    - En vue Arrière, c'est le même fondu qu'avant : 8 pixels d'écart à la bouche, 0 à 6 m.
    - En vue Cockpit, plus aucun trait dès que la caméra est sous la voûte.
    - Avant la première image, la caméra n'est pas encore connue : le calcul prend alors la voiture (remarque b).
  - **Banc `WFS_BANC=pluie`** (`benchPluie`, dans `meteo.cpp`), branché par une accroche E7 de 2 lignes dans `desktop_main.cpp`, annoncée au journal. Il prend la Golf et le X5, et les témoins en vue Arrière dehors et à la bouche du tunnel.
- **Comment c'est vérifié**
  - **`test_pluie` vert** :
    - lancer de rayon depuis l'œil : sur les 20 intérieurs, aucun point du bord de la vitre n'est caché par un montant, ni ce même point décalé du rayon d'une goutte ;
    - contrôle du test : l'ancien bord, sur l'axe du montant, est bien vu comme caché ;
    - plus de pluie dès la bouche, fondu de 8 m.
  - Banc `pluie` : effacement par les balais dessinés en Cockpit, 0 effacement en vue Arrière.
  - Coût de la pluie : 0,03 ms par image en Cockpit, au plus 0,05 ms.
  - **87 tests CPU verts, parcours complet 0 écart**, branche synchronisée avec `main` (WFS-318 compris).
- **Limites et risques connus**
  - La suite des balais est de nuit : la pluie n'existe que la nuit en Ville.
  - Les gouttes déjà posées restent sur le pare-brise dans le tunnel jusqu'au passage du balai.
  - Le banc est dans `world/meteo.cpp`, qui dépend donc de `game/game.h` (remarque c). Je peux le sortir dans un fichier à part si le PO l'ajoute à la zone.
  - Les gouttes passent aussi sur le rétroviseur intérieur (remarque d) : c'est déjà le cas sur `main`, hors de ce ticket, et c'est noté pour le PO.
  - Après la fusion, le ticket passe en 🟡 À valider (utilisateur).

## Captures pour le lead dev

Dossier : [`qa/pr/WFS-354`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354)

| Capture | Ce qu'on voit | Avant (main) | Après (branche) |
|---|---|---|---|
| `pluie-cockpit-105-zoom-montant-droit` | **montant droit, Golf, 102 km/h, agrandi** (défaut bloquant de la 1re revue) : avant, des gouttes sur la bande sombre du montant ; après, aucune | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-cockpit-105-zoom-montant-droit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-cockpit-105-zoom-montant-droit.jpg?raw=true) |
| `pluie-cockpit-105-x5-zoom-montant-droit` | **montant droit, X5** (2e intérieur) : avant, des gouttes sur le montant ; après, aucune | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-cockpit-105-x5-zoom-montant-droit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-cockpit-105-x5-zoom-montant-droit.jpg?raw=true) |
| `pluie-cockpit-105-zoom-montant-gauche` | montant gauche, Golf, agrandi : aucune goutte après | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-cockpit-105-zoom-montant-gauche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-cockpit-105-zoom-montant-gauche.jpg?raw=true) |
| `pluie-cockpit-105` | Cockpit Golf, nuit, 102 km/h : vue entière | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-cockpit-105.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-cockpit-105.jpg?raw=true) |
| `pluie-cockpit-105-x5` | Cockpit X5, nuit, 102 km/h : vue entière | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-cockpit-105-x5.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-cockpit-105-x5.jpg?raw=true) |
| `pluie-balais-1` | **balayage 1/5** (RG-02), balais à 10 % de leur course montante | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-balais-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-balais-1.jpg?raw=true) |
| `pluie-balais-2` | balayage 2/5 (30 %) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-balais-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-balais-2.jpg?raw=true) |
| `pluie-balais-3` | balayage 3/5 (50 %) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-balais-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-balais-3.jpg?raw=true) |
| `pluie-balais-4` | balayage 4/5 (70 %) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-balais-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-balais-4.jpg?raw=true) |
| `pluie-balais-5` | balayage 5/5 (90 %) : la zone balayée est vide | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-balais-5.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-balais-5.jpg?raw=true) |
| `pluie-tunnel-avant` | **tunnel en Cockpit** (RG-03) : 10 m avant la bouche, il pleut | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-tunnel-avant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-tunnel-avant.jpg?raw=true) |
| `pluie-tunnel-bouche` | caméra à la bouche : plus de traits (avant : il en restait) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-tunnel-bouche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-tunnel-bouche.jpg?raw=true) |
| `pluie-tunnel-3m` | caméra 3 m sous la voûte : avant, des traits ; après, aucun (les gouttes déjà posées restent jusqu'au balai) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-tunnel-3m.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-tunnel-3m.jpg?raw=true) |
| `pluie-tunnel-6m` | caméra 6 m sous la voûte : aucun trait | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-tunnel-6m.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-tunnel-6m.jpg?raw=true) |
| `pluie-tunnel-milieu` | milieu du tunnel : aucun trait | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-tunnel-milieu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-tunnel-milieu.jpg?raw=true) |
| `pluie-tunnel-arriere-bouche` | **témoin vue Arrière à la bouche** (caméra de poursuite à la bouche) : identique à main (8 pixels d'écart) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-tunnel-arriere-bouche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-tunnel-arriere-bouche.jpg?raw=true) |
| `pluie-tunnel-arriere-6m` | témoin vue Arrière, caméra 6 m dans le tunnel : identique à main (0 pixel) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-tunnel-arriere-6m.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-tunnel-arriere-6m.jpg?raw=true) |
| `pluie-arriere-vitesse` | témoin vue Arrière à 102 km/h, dehors : traits inclinés comme sur main (les traits bougent d'une passe à l'autre) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/avant-pluie-arriere-vitesse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-354/qa/pr/WFS-354/apres-pluie-arriere-vitesse.jpg?raw=true) |
