Agent : Claude, agent dev3d (01UY1Vv2rp) (session_01UY1Vv2rpnG6QPHQ9jg9MnC)

**Ticket** : WFS-371 — règles : `regles-de-gestion/WFS-371.md` (épic E15 « Décors » : **avis de l'agent 3D obligatoire avant le lead dev**)

## Description

- **Demande de l'utilisateur** (2026-11-09) : sur la Corniche, plus aucun objet en forme géométrique (cônes, boîtes, cylindres, boules) ; les parasols, transats, le bar de plage, les cabines (postes de secours), les glissières et les rochers deviennent de vrais objets, fidèles au réel à 99 % ; avec les ajouts de l'agent 3D : scène de concert en vrai volume et sable 0xD8C8A0 avec laisse de mer (D4), lampadaires du quai en lanternes de promenade (R4), glissière à double onde en volume (R5).
- **Ce qui change** :
  - nouveau `src/world/decor_corniche_modeles.{h,cpp}` : 21 modèles construits par le code au niveau du vrai objet (RG-03), aux vraies mesures avec leurs sources, en version proche et version allégée (loin / Basse) ; même méthode que WFS-372, 373 et 376 (outils `loft`, `eventail`, `ligne`, `poutre` réutilisés) ;
  - `src/world/plage.cpp` : chaque forme nue de l'inventaire remplacée par son modèle (`poserModele`) ; modèles construits une fois au chargement (`initModeles`), recopiés dans les blocs du décor préparés au chargement (WFS-268) : **rien n'est créé en course** (0 maillage, 0 matériau, journal du banc) ;
  - à l'écran : parasols Ø 1,80 m à 8 baleines et lambrequin, bains de soleil 1,90 × 0,65 m sur roues, bar de plage (pieux en rondins, comptoir à lames, étagère, tabourets, tables en teck, poufs, bouteilles, parasols de paille à franges, ampoules), postes de secours à clin sur pilotis, glissière à double onde en volume (poteaux C 100 tous les 4 m), rochers en blocs de calcaire, arbres ronds devenus pins d'Alep, scène de concert (praticables, portique treillis, fond tendu, DJ, enceintes, lyres), guirlandes et feux de camp, candélabres de style (fût cannelé, crosse, lanterne vitrée), couronnes de palmiers (gaines, noix de coco), sable 0xD8C8A0 et laisse de mer ;
  - inventaire complet (RG-01) : `qa/pr/WFS-371/inventaire.md` (aussi au journal de `main`) ;
  - banc des gros plans : `WFS_BANC=corniche WFS_CORNICHE=objets` (14 vues `corniche-objets-*`, jour et nuit) ;
  - nouveau test `tests/test_decor_corniche.cpp` (mesures ±1 %, budget de triangles, version loin plus légère, aucun triangle dégénéré ni inversé, normales unitaires).
- **Taille ajoutée** : 0 Mo de fichiers (aucun actif téléchargé : tout est calculé au chargement) ; code source + 0,05 Mo.
- **Paquet `.wfcarte`** : le paquet de la Corniche (WFS-426) ne cuit que les villas du kit ; ces objets sont construits par le code au chargement, avec ou sans paquet. WFS-463 (`@cicd`) peut publier après la fusion sans autre changement.
- **Comment c'est vérifié** :
  - `test_decor_corniche` (nouveau) : mesures ±1 % de chaque modèle (parasol Ø 1,800 m et 2,200 m, transat 0,650 m de large, poste : plancher 2,400 m, lisse 0,310 × 0,083 m à 0,750 m, scène 5 × 10 m à 1,0 m et portique 6,2 m, verres du candélabre centrés à 5,600 m, etc.), budgets de triangles, version loin plus légère, 0 triangle dégénéré ou inversé ;
  - tests CPU : **113 réussis** sur `345cd5d5` (journal complet : `qa/pr/WFS-371/tests-cpu.log`) ;
  - banc `corniche` avant / après (15 vues de course et de promenade, jour et nuit) et banc des gros plans `corniche-objets` (14 vues), toutes regardées ;
  - **performance** (banc `perf`, Corniche, **nuit**, 90 images, rendu logiciel lavapipe : un indice, pas la mesure du S21 FE) : **Basse 318,6 → 312,9 ms** par image (pas de baisse, bruit), Haute 508,7 → 481,0 ms ; triangles dessinés Basse 252 k → 319 k (+27 %), Haute 331 k → 424 k ; appels de dessin 290 → 291 (Basse), 325 → 327 (Haute) ; 0 maillage et 0 matériau créés en course. La mesure sur le S21 FE de l'utilisateur (≤ ≈ 33 ms, Basse 45 %, nuit) reste à faire sur l'APK de `main`.
- **Limites et risques connus** :
  - D4 : les **traces dans le sable** demandent une texture du sol : non faites ici (petit reste, tâche séparée proposée au PO) ; couleur du sable et laisse de mer faites ;
  - formes nues encore visibles sur la Corniche, **hors de ce ticket** (liste remise au PO dans `inventaire.md`) : bateaux au large et barques du village, jeux de plage (poteaux de volley, pédalos, toboggan, château de sable), scooters, façades du casino / café / galerie (WFS-410, WFS-386), feu tricolore, ancre, luminaires du tunnel, faisceaux et flamme (effets de lumière, E13) ;
  - la nuit, le bar reste sombre (son éclairage n'a pas changé : lumière, E13) ;
  - le banc des gros plans écrit « pipelines créés pendant la course » : propre à sa vue libre (même chose que `campagne-objets` et `grandprix-objets`) ;
  - +27 % de triangles sur la Corniche : à confirmer sur le téléphone.

## Tests CPU (commit `345cd5d5`)

```
[OK] test_vehicule_lod (0 s)
[OK] test_version_chargement (0 s)
[OK] test_ville_lieux (0 s)
[OK] test_ville_sol (3 s)
[OK] test_virages_fleches (1 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 113 test(s) réussi(s) en 403 s
code 0
```

## Captures pour le lead dev et l'agent 3D

Dossier : [`qa/pr/WFS-371`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371). Gros plans (`corniche-objets-*`) : nouveaux, sans « avant » (le banc n'existe pas sur `main`) ; les vues `carte-plage-*` et `perf-*` montrent les mêmes lieux avant / après.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `corniche-objets-bar-comptoir-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-corniche-objets-bar-comptoir-nuit.jpg?raw=true) |
| `corniche-objets-bar-comptoir` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-corniche-objets-bar-comptoir.jpg?raw=true) |
| `corniche-objets-bar-paille` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-corniche-objets-bar-paille.jpg?raw=true) |
| `corniche-objets-feu-de-camp` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-corniche-objets-feu-de-camp.jpg?raw=true) |
| `corniche-objets-glissiere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-corniche-objets-glissiere.jpg?raw=true) |
| `corniche-objets-lampadaire-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-corniche-objets-lampadaire-nuit.jpg?raw=true) |
| `corniche-objets-lampadaire` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-corniche-objets-lampadaire.jpg?raw=true) |
| `corniche-objets-palmier` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-corniche-objets-palmier.jpg?raw=true) |
| `corniche-objets-parasol` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-corniche-objets-parasol.jpg?raw=true) |
| `corniche-objets-poste` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-corniche-objets-poste.jpg?raw=true) |
| `corniche-objets-rocher` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-corniche-objets-rocher.jpg?raw=true) |
| `corniche-objets-scene-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-corniche-objets-scene-nuit.jpg?raw=true) |
| `corniche-objets-scene` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-corniche-objets-scene.jpg?raw=true) |
| `corniche-objets-transat` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-corniche-objets-transat.jpg?raw=true) |
| `carte-plage-bar-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-carte-plage-bar-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-carte-plage-bar-nuit.jpg?raw=true) |
| `carte-plage-bar-vue-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-carte-plage-bar-vue-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-carte-plage-bar-vue-nuit.jpg?raw=true) |
| `carte-plage-bar-vue` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-carte-plage-bar-vue.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-carte-plage-bar-vue.jpg?raw=true) |
| `carte-plage-bar` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-carte-plage-bar.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-carte-plage-bar.jpg?raw=true) |
| `carte-plage-cafe-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-carte-plage-cafe-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-carte-plage-cafe-nuit.jpg?raw=true) |
| `carte-plage-falaise-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-carte-plage-falaise-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-carte-plage-falaise-nuit.jpg?raw=true) |
| `carte-plage-falaise` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-carte-plage-falaise.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-carte-plage-falaise.jpg?raw=true) |
| `carte-plage-fete-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-carte-plage-fete-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-carte-plage-fete-nuit.jpg?raw=true) |
| `carte-plage-fete-vue-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-carte-plage-fete-vue-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-carte-plage-fete-vue-nuit.jpg?raw=true) |
| `carte-plage-fete-vue` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-carte-plage-fete-vue.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-carte-plage-fete-vue.jpg?raw=true) |
| `carte-plage-fete` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-carte-plage-fete.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-carte-plage-fete.jpg?raw=true) |
| `carte-plage-jeux-vue` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-carte-plage-jeux-vue.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-carte-plage-jeux-vue.jpg?raw=true) |
| `carte-plage-rochers-haut` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-carte-plage-rochers-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-carte-plage-rochers-haut.jpg?raw=true) |
| `carte-plage-rochers` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-carte-plage-rochers.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-carte-plage-rochers.jpg?raw=true) |
| `carte-plage-village` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-carte-plage-village.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-carte-plage-village.jpg?raw=true) |
| `perf-plage-nuit-Basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-perf-plage-nuit-Basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-perf-plage-nuit-Basse.jpg?raw=true) |
| `perf-plage-nuit-Haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/avant-perf-plage-nuit-Haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-371/qa/pr/WFS-371/apres-perf-plage-nuit-Haute.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv2rpnG6QPHQ9jg9MnC
