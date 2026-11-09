Agent : Claude, agent dev Y

**Ticket** : WFS-334 (ANO-182, avec ANO-186 et ANO-194). Règles : `regles-de-gestion/WFS-334.md`.

## Description

- **Demande de l'utilisateur** : « la pluie rentre dans la voiture en vue Cockpit, et dans le tunnel » ; en plus, la pluie doit s'incliner avec la vitesse (ANO-194).
- **Cause (RG-01)** : la pluie est un voile de traits 2D dessiné par-dessus toute l'image (`meteo.cpp`, `drawUi`). Elle ignore la vue (planche, volant, toit), les tunnels et la vitesse : les traits restent verticaux, comme à l'arrêt.
- **Ce qui change**
  - **`src/world/meteo.h`** : nouvelles règles pures, testées par `tests/test_pluie.cpp`.
    - `pluieSousToit` : pluie coupée sous un toit, avec un fondu de 8 m à chaque bouche.
    - `sensGoutte` et `avancerVers` : à vitesse, les gouttes partent du point de fuite et viennent vers la caméra.
    - `dansQuad` : point dans le pare-brise.
    - `balaye` : passage d'un balai.
  - **`src/world/meteo.cpp`** (système « pluie ») :
    - **RG-03** : sous un tunnel du tracé (`trace::Type::Tunnel`), la pluie s'efface. La chaussée reste mouillée.
    - **RG-02**, en vue Cockpit (`etatCockpit()`, lecture seule) :
      - les traits ne sont dessinés que par le pare-brise, projeté à l'écran, et par les vitres latérales au-dessus de la planche, hors montants ;
      - 140 gouttes de 2 à 6 mm sont posées sur la vitre : elles apparaissent peu à peu, les grosses coulent, et à vitesse le vent les pousse vers le haut ;
      - les balais effacent ces gouttes, avec le même modèle et les mêmes entrées que la caméra (`cockpit::EtatEssuie`).
    - **ANO-194** : traits inclinés depuis le point de fuite, plus longs à vitesse.
  - **`src/render/vehicule_cockpit.*`** (E10) : `pareBriseVu()` donne, pour l'image en cours, le pare-brise (4 coins) et les balais (pivots, sens, longueurs), relevés là où l'intérieur les dessine.
  - **`src/world/cartes_banc.cpp`** : 3 vues de plus au banc Ville, `carte-ville-tunnel-bouche`, `-milieu` et `-sortie` (de jour et de nuit).
  - **Non touchés** : `camera_vue.cpp` (lu seulement), `nuit.cpp`, `vehicule_lod*`, `effets_*`, `post*`, `eclairage_*`, `traffic.*`.
- **Comment c'est vérifié**
  - `test_pluie` est vert : fondu du tunnel, sens des gouttes, quadrilatère, balayage.
  - Banc `cockpit` (Golf, Ville de nuit) : en fin de course, `vitre : 182 gouttes effacées par les balais`.
  - Coût CPU de la pluie : 0,029 ms par image en cockpit (0,005 avant), 0,006 ms en vue Arrière en Haute, 0,013 ms en Basse.
  - 81 tests CPU verts, parcours complet 0 écart (synchronisé avec `main`).
- **Limites et risques connus**
  - Les vitres latérales sont approchées à l'écran : au-dessus de la ligne de la planche, à plus de 7 % de la hauteur de l'écran du montant.
  - Les 20 voitures ont un intérieur propre (`vehicule_cockpit_defs.cpp`). Une future voiture sans intérieur propre (cockpit commun) n'aurait pas de pare-brise relevé et garderait l'ancienne pluie plein écran.
  - En sortie de tunnel, la pluie ne revient qu'à la bouche : on ne la voit pas tomber dehors, au bout du tunnel.
  - La chaussée mouillée de `nuit.cpp` n'est pas touchée : elle reste mouillée aussi au milieu du tunnel.
  - Ticket né d'un essai de l'utilisateur : il passera en 🟡 À valider (utilisateur) après la fusion.

## Captures pour le lead dev (12 paires : cockpit nuit et jour, vue Arrière Haute et Basse, tunnel en 4 images, 3 agrandissements)

Dossier : [`qa/pr/WFS-334`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334)

| Capture | Ce qu'on voit | Avant (main) | Après (branche) |
|---|---|---|---|
| `cockpit-golf-pluie` | **Vue Cockpit sous la pluie** (Ville de nuit, Golf, 26 km/h, balais en haut). Avant : des traits de pluie tombent sur la planche, le volant, les aérateurs et l'écran. Après : la pluie ne se voit que par le pare-brise, et des gouttes sont posées sur la vitre | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/avant-cockpit-golf-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/apres-cockpit-golf-pluie.jpg?raw=true) |
| `cockpit-golf-pluie-zoom-planche` | agrandissement de la planche : avant, des traits sur le volant, l'écran et les aérateurs ; après, aucun | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/avant-cockpit-golf-pluie-zoom-planche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/apres-cockpit-golf-pluie-zoom-planche.jpg?raw=true) |
| `cockpit-golf-nuit-virage` | Cockpit à **105 km/h** de nuit. Avant : traits verticaux partout, planche comprise. Après : gouttes inclinées depuis le point de fuite (ANO-194), seulement derrière la vitre, et gouttes sur le pare-brise | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/avant-cockpit-golf-nuit-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/apres-cockpit-golf-nuit-virage.jpg?raw=true) |
| `cockpit-golf-nuit-virage-zoom-planche` | agrandissement de la planche à 105 km/h : avant, des traits ; après, aucun | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/avant-cockpit-golf-nuit-virage-zoom-planche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/apres-cockpit-golf-nuit-virage-zoom-planche.jpg?raw=true) |
| `cockpit-golf-jour` | Cockpit de jour (sans pluie, témoin) : image identique | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/avant-cockpit-golf-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/apres-cockpit-golf-jour.jpg?raw=true) |
| `carte-ville-feux-nuit` | **même rue en vue Arrière**, nuit, Haute : la pluie reste (verticale à l'arrêt), rien d'autre ne change | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/avant-carte-ville-feux-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/apres-carte-ville-feux-nuit.jpg?raw=true) |
| `carte-ville-feux-nuit-basse` | même vue en **Basse** (qualité du téléphone) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/avant-carte-ville-feux-nuit-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/apres-carte-ville-feux-nuit-basse.jpg?raw=true) |
| `carte-ville-tunnel-nuit` | **tunnel 1/4** : 40 m avant l'entrée, sous la pluie : la pluie tombe (inchangé) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/avant-carte-ville-tunnel-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/apres-carte-ville-tunnel-nuit.jpg?raw=true) |
| `carte-ville-tunnel-bouche-nuit` | **tunnel 2/4** : 6 m dans la bouche : fondu, la pluie s'arrête | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/avant-carte-ville-tunnel-bouche-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/apres-carte-ville-tunnel-bouche-nuit.jpg?raw=true) |
| `carte-ville-tunnel-milieu-nuit` | **tunnel 3/4** : milieu. Avant : la pluie tombait sous la voûte. Après : plus aucune goutte ; la chaussée reste mouillée | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/avant-carte-ville-tunnel-milieu-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/apres-carte-ville-tunnel-milieu-nuit.jpg?raw=true) |
| `carte-ville-tunnel-milieu-nuit-zoom` | agrandissement du milieu du tunnel : avant, des traits sous la voûte ; après, aucun | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/avant-carte-ville-tunnel-milieu-nuit-zoom.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/apres-carte-ville-tunnel-milieu-nuit-zoom.jpg?raw=true) |
| `carte-ville-tunnel-sortie-nuit` | **tunnel 4/4** : 12 m avant la sortie, toujours sous la voûte : pas de pluie | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/avant-carte-ville-tunnel-sortie-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-334/qa/pr/WFS-334/apres-carte-ville-tunnel-sortie-nuit.jpg?raw=true) |
