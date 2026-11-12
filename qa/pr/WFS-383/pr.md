## Agent : Claude, agent dev3d (01UY1Vv1zt)

**Ticket** : WFS-383 — règles : `regles-de-gestion/WFS-383.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : ANO-224 — les sols des cartes sont des aplats unis en facettes ; il faut de l'herbe sèche, de la terre et de la roche selon la pente, et des pins d'Alep sur les pentes de la Corniche (RG de `regles-de-gestion/WFS-383.md`).
- **Ce qui change** :
  - **Nouveau shader de sol** `shaders/sols.glsl` (`surfaceTerrain`) : mélange de 3 matériaux selon la pente lue sur la normale lissée — pré / herbe sèche (< 25°), terre et maquis (25 à 40°), roche (> 40°, calcaire clair en Corniche) — limites irrégulières (± 4°, tache de 25 m), fondus de 8°, taches d'herbe et de terre (25 m et 3,5 m), grain de 40 cm effacé au loin (pas de scintillement), fissures dans la roche. Couleurs mesurées (RG-02 : pré 0x6B7F3A, garrigue, calcaire 0,50 de luminance, sources dans `src/world/sols.h`). Automne : feuilles mortes de 4 teintes en taches de 10 à 30 cm (jamais un aplat orange). Coût : 2 bruits de 4 octaves (2 en Basse) + 3 bruits simples, aucune texture.
  - **`src/world/sols.{h,cpp}`** (nouveaux) : matériau de sol par climat (méditerranéen, tempéré, automne), **lissage des normales** des terrains (sommets soudés, normales pondérées par l'angle : plus de facettes, RG-01), climat de chaque carte.
  - **Corniche** (`plage.cpp`) : collines, dunes, herbe et garrigue passent au sol naturel (falaises `ROC` texturées inchangées) ; les anciens pins en cônes deviennent des **pins d'Alep** (tronc gris / rouge-brun, couronne en parasol, 2 niveaux de détail) **plus des bosquets aléatoires sur les pentes** (densité lissée sur 40 segments, jamais en grille, jamais dans la mer ni au-delà de 40°) ; tout est préparé au chargement (lots WFS-268, occlusion comprise).
  - **Campagne, Grand Prix, cartes de données** (`decor_carte.*`, `circuit.cpp`, `carte_donnees_decor.cpp`) : verges, fossés, collines, champs et prés passent au sol naturel ; les champs gardent leur couleur (blé, labour) ramenée à mi-chemin du pré mesuré.
  - **Cartes de données** : nouvelle ligne `sol mediterraneen|tempere|automne` du bloc `decor` (`carte_donnees.*`) ; la carte d'automne (`paquets/sources/automne-test/decor.txt`) a `sol automne` ; la carte de Noël (WFS-346) n'a pas la ligne et reste **exactement comme avant** (aucun fichier neige touché).
  - **Accroches annoncées (≤ 10 lignes)** : `shaders/scene.frag` (2 lignes : include + choix du sol), `src/game/game.cpp` (2 lignes : sol de la carte), `decor_carte.h`, `circuit.cpp`. `nuit.cpp` (WFS-421) et les fichiers de WFS-425 non touchés.
  - **Test** `tests/test_sols.cpp` (dans `CMakeLists.txt`) : code du sol, matériau, lissage (pli en V : faîte vertical, flancs à 45°, 12 → 6 sommets ; deux bandes de couleur gardent leurs sommets avec une normale commune), climat des cartes, lecture de la ligne `sol`, Noël sans sol / automne = 3.
  - **Corrections après l'avis de l'agent 3D (À REPRENDRE sur f53fa15d)** :
    1. **Pins d'Alep** : albédo des aiguilles relevé (`ALEP_AIGUILLES` = 4,5 / 2,2 / 7,7 sur l'atlas, linéaire) : moyenne du houppier mesurée à l'écran **(71, 86, 61)** au lieu de (46, 56, 39), gris-vert visé #5E6A48 à #6F7A55.
    2. **RG-01 montrée** : 4 nouvelles vues du banc `corniche` (`carte-plage-talus`, `-talus-haut`, `-rochers`, `-rochers-haut`, côté colline). Pentes mesurées sur la Corniche : **19° au plus** sur les bandes de sol naturel (colline ≈ 11,6°, zone « rochers » 15,5°) ; les vraies pentes > 40° sont les falaises `ROC`, qui gardent leur texture (WFS-139). La zone « rochers » (`rocheux`) lit donc la pente **+ 25°** : `carte-plage-rochers-haut` montre **pré, terre / maquis et calcaire fissuré avec leurs fondus**. `carte-plage-talus` (colline ≈ 12°) reste en garrigue : c'est juste pour sa pente.
    3. **Motif « camouflage »** : grandes taches à contraste 0,4 (au lieu de 0,7) avec fondu plus large, pointillé de touffes de maquis (30 à 80 cm) sur le plat méditerranéen, taches de terre plus douces et ramenées vers le pré (`carte-plage-montee-dessus`, `-montee-courbe-dessus`).
    - Réserves : bruit des fissures calculé **seulement sur la roche** (`tRoche > 0.001`) ; captures perf en double (`-basse` / `-haute`) retirées.
  - **Synchronisation avec `main` (WFS-392, tunnels)** : conflits dans `carte_donnees.{h,cpp}` et `carte_donnees_decor.cpp` résolus en gardant les deux côtés ; le **trottoir de service en béton des tunnels** reste un sol uni (exclu du sol naturel). Construction et **112 tests CPU verts** après synchronisation.
- **Comment c'est vérifié** :
  - Captures avant / après (bancs `corniche`, `campagne`, `neige`, `perf`), jour et nuit, regardées une à une ; Noël (`neige-jour-arriere`) inchangé, automne avec feuilles mortes.
  - **Banc perf (PC, lavapipe = rendu sur le processeur)**, temps moyen par image, série 3 (commit 8753feb2) :
    | Carte | Basse (avant → après) | Haute (avant → après) |
    |---|---|---|
    | Corniche | 217,0 → 231,7 ms (+7 %) | 453,3 → 477,7 ms (+5 %) |
    | Campagne | 219,4 → 235,3 ms (+7 %) | 432,0 → 442,1 ms (+2 %) |
    | Grand Prix | 243,5 → 264,6 ms (+9 %) | 599,1 → 613,1 ms (+2 %) |
    | Ville (témoin, sol non touché) | 297,1 → 303,5 ms (+2 %) | 673,2 → 676,1 ms (0 %) |

    Séries précédentes (passe de scène) : Corniche Basse +1 % puis +12 %, Campagne Basse +2 % puis +15 %. Lecture honnête : sur lavapipe, le bruit d'une série à l'autre est du même ordre que l'écart ; le coût ajouté est du calcul de pixel sur le sol seulement (aucune texture lue). Triangles égaux ou en baisse.
  - **Corniche en paquet (WFS-426)** : banc `batiments` avec `WFS_CARTE=plage`, jour et nuit : le paquet ne contient que les villas, le sol et les pins sont construits par le code et visibles (`paquet-plage-*`).
- **Limites et risques connus** :
  - **Performance du S21 FE non mesurée** (banc PC seulement) : le critère « Corniche ≤ 33 ms sur S21 FE » est à confirmer par l'utilisateur sur l'APK de `main`.
  - **Pin d'Alep procédural** (RG-03 demande « 1 modèle CC0 / CC BY ») : aucun modèle CC0 / CC BY accessible depuis les sites autorisés ; générateur d'arbres du dépôt (`veg::pin`, couronne en parasol), 2 niveaux de détail, placement irrégulier.
  - **Pins fusionnés dans la géométrie des morceaux de décor, pas en instances GPU** (RG-03) : même chemin que les autres arbres du dépôt ; les triangles baissent.
  - **Épingle de la Campagne** : talus en terrasses de vignes, pentes douces : pas de roche visible ; la démonstration des 3 matériaux est faite sur la zone « rochers » de la Corniche.
  - Captures prises sur 8753feb2 ; la synchronisation suivante avec `main` ne touche que les cartes de données (tunnels), pas la Corniche ni la Campagne.
  - Le sol ne porte plus d'ombre propre (`MAT_GROUND`, comme les autres sols).
  - Hors périmètre, vus sur les captures : montagnes en pyramides et éoliennes de la Campagne (WFS-369 à 373), pelouse de la Ville (WFS-385), gros feuillus en boules facettées de la Corniche (forme existante), crête droite de la colline de la Corniche.

## Captures pour le lead dev

Dossier : [`qa/pr/WFS-383`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `carte-campagne-col-haut` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-carte-campagne-col-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-carte-campagne-col-haut.jpg?raw=true) |
| `carte-campagne-epingle` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-carte-campagne-epingle.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-carte-campagne-epingle.jpg?raw=true) |
| `carte-campagne-vaches` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-carte-campagne-vaches.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-carte-campagne-vaches.jpg?raw=true) |
| `carte-campagne-vignes-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-carte-campagne-vignes-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-carte-campagne-vignes-nuit.jpg?raw=true) |
| `carte-plage-bateaux` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-carte-plage-bateaux.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-carte-plage-bateaux.jpg?raw=true) |
| `carte-plage-falaise-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-carte-plage-falaise-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-carte-plage-falaise-nuit.jpg?raw=true) |
| `carte-plage-jeux` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-carte-plage-jeux.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-carte-plage-jeux.jpg?raw=true) |
| `carte-plage-montee-courbe-dessus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-carte-plage-montee-courbe-dessus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-carte-plage-montee-courbe-dessus.jpg?raw=true) |
| `carte-plage-montee-dessus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-carte-plage-montee-dessus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-carte-plage-montee-dessus.jpg?raw=true) |
| `carte-plage-rochers-haut` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-carte-plage-rochers-haut.jpg?raw=true) |
| `carte-plage-rochers` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-carte-plage-rochers.jpg?raw=true) |
| `carte-plage-rond-point-dessus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-carte-plage-rond-point-dessus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-carte-plage-rond-point-dessus.jpg?raw=true) |
| `carte-plage-talus-haut` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-carte-plage-talus-haut.jpg?raw=true) |
| `carte-plage-talus` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-carte-plage-talus.jpg?raw=true) |
| `neige-automne-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-neige-automne-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-neige-automne-jour.jpg?raw=true) |
| `neige-jour-arriere` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-neige-jour-arriere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-neige-jour-arriere.jpg?raw=true) |
| `perf-campagne-Basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-perf-campagne-Basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-perf-campagne-Basse.jpg?raw=true) |
| `perf-campagne-Haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-perf-campagne-Haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-perf-campagne-Haute.jpg?raw=true) |
| `perf-circuit-Basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-perf-circuit-Basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-perf-circuit-Basse.jpg?raw=true) |
| `perf-circuit-Haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-perf-circuit-Haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-perf-circuit-Haute.jpg?raw=true) |
| `perf-plage-Basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-perf-plage-Basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-perf-plage-Basse.jpg?raw=true) |
| `perf-plage-Haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-perf-plage-Haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-perf-plage-Haute.jpg?raw=true) |
| `perf-ville-Basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-perf-ville-Basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-perf-ville-Basse.jpg?raw=true) |
| `perf-ville-Haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/avant-perf-ville-Haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-perf-ville-Haute.jpg?raw=true) |
| `paquet-plage-rue-jour` (banc `batiments`, `WFS_CARTE=plage`) | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-paquet-plage-rue-jour.jpg?raw=true) |
| `paquet-plage-rue-nuit` (banc `batiments`, `WFS_CARTE=plage`) | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-paquet-plage-rue-nuit.jpg?raw=true) |
| `paquet-plage-160kmh` (banc `batiments`, `WFS_CARTE=plage`) | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-383/qa/pr/WFS-383/apres-paquet-plage-160kmh.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv1ztBP5V8pNZenM1Vd
