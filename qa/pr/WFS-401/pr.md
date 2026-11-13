Agent : Claude, agent dev3d (01Us6uCxKQ) (session_01Us6uCxKQDpfjkTaDzFG4mi)

**Ticket** : WFS-401 — règles : `regles-de-gestion/WFS-401.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : finir la vraie mer de WFS-380 (réserves R3, R7, R8, R10 de l'agent 3D) : écume et rides moins « filées » de près, plus de motif régulier au loin, traînée du soleil bas faite de reflets séparés, plus de trait blanc plat sous le dauphin, et les captures de l'écume contre les blocs et au pied de la falaise.
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - `shaders/eau.glsl` (hors Basse sauf mention) :
    - **RG-01 écume** : nouvelle mousse en réseau de bulles (`eauCellules`, `eauMousse` : cellules de Voronoï de 42 et 15 cm aux bords ondulés, trous qui grandissent quand l'écume se défait) appliquée à moins de 45 m à l'écume du bord, à la nappe du ressac sur le sable et à l'écume de la gerbe. Taches du bruit de l'écume : 2,2 × 0,7 m au lieu de 6 × 0,4 m (rapport 3 au lieu de 15). Liseré au pied de la roche de largeur variable (le long du bord et avec la vague) au lieu d'une bande droite.
    - **RG-01 / RG-02 vagues** : « groupes » de vagues (amplitude qui varie le long de la crête sur 1,5 longueur d'onde, crêtes courtes qui se croisent ; moyenne inchangée), crêtes courbées et espacement variable (déphasage lent ± 1,5 rad) ; chaque vague est filtrée selon la taille du pixel **dans sa direction** (à angle rasant, les vagues en travers ne sont plus effacées avec les autres) et un peu plus tôt près de la limite du pixel (fines rayures du loin).
    - **RG-03 soleil bas** : reflets séparés (« glitter ») : là où le reflet du soleil peut tomber (demi-vecteur à moins de ≈ 3 écarts types des pentes non résolues), chaque carré de 2 × 2 pixels tire une facette (pente gaussienne de la variance perdue, tirage changé 12 fois par seconde) avec la rugosité d'une facette lisse ; la colonne reste étroite sous le soleil et s'élargit au loin.
    - **RG-04 gerbe du dauphin** (mode 2) : couronne d'eau soulevée (nappe laiteuse en bas, ≈ 15 jets inégaux, gouttes de 1 à 4 cm au-dessus) + nappe d'écume au bord déchiré ; aucune paroi pleine, aucun disque, aucun plan blanc.
    - **Basse** : chemin inchangé (4 vagues, écume de main, pas de mousse ni de reflets séparés).
  - `src/render/eau_parametres.h` : `eclaboussure()` fait la nappe (20 triangles) + la couronne évasée (24 × 4 quads), **un seul maillage**, un seul appel de dessin ; `hauteurGerbe(âge)` (balistique, sommet 0,9 m à 0,43 s).
  - `src/world/plage.cpp` (7 lignes ajoutées, 1 changée, annoncées au journal) : hauteur de la gerbe dans l'échelle du dessin ; vues de banc `mer-eclaboussure-pres` (gros plan depuis l'eau, 0,2 et 0,45 s), `mer-rochers-pres` (écume contre les blocs), `mer-falaise` (pied de la falaise depuis l'eau).
  - `tests/test_eau.cpp` : 6 contrôles de la gerbe (un maillage, nappe dans le rayon 1, couronne marquée et évasée, hauteur balistique).
- **Comment c'est vérifié** :
  - construction complète, `tools/qa/tests_cpu.sh build` : 124 tests réussis (dont `test_eau`) sur la branche synchronisée avec main ;
  - banc `mer` avant (main) / après (branche) : 21 captures, toutes regardées ;
  - zooms ajoutés : `zoom-soleil-avant/apres.jpg` (niveaux étirés 60–100 % pour voir sous le voile du soleil : tache floue sur main, colonne de reflets séparés sur la branche), `zoom-eclaboussure-loin-avant/apres.jpg` (trait blanc plat → gerbe), `zoom-eclaboussure-apres-1/2.jpg` (gros plan ×4) ;
  - **banc perf de la Corniche** (`WFS_BANC=perf WFS_PERF_CARTE=plage`, main et branche l'un après l'autre sur la même machine, lavapipe) :

    | | main (2 passes) | branche (3 passes) |
    |---|---|---|
    | **Basse**, image | 195,6 / 197,4 ms | 198,3 / 195,7 / 197,5 ms |
    | Basse, appels de dessin / triangles | 255 / 311 526 | 255 / 311 526 |
    | Haute, image | 417,6 / 420,7 ms | 435,4 / 424,0 / 413,9 ms |

    Basse : dans le bruit de mesure (même chemin du shader que main), mêmes appels de dessin ; Haute : ≈ +1 à +2 %.
- **Limites et risques connus** :
  - **soleil bas** : la vue reste très claire (voile du soleil et de la brume, exposition, bloom : hors de ce ticket) ; les reflets séparés se voient bien sur le zoom aux niveaux étirés, moins sur l'image brute ;
  - **au loin au téléobjectif** (`mer-sillage`, champ de 14°) : il reste de fines bandes horizontales (compression de la perspective) moins régulières qu'avant, mais encore visibles ;
  - les reflets séparés sont tirés par pixel à l'écran : en course, ils scintillent (12 tirages par seconde) ;
  - mesures sur le banc PC (lavapipe), pas sur le téléphone ;
  - hors périmètre, visibles sur les captures : formes nues des bateaux, du jeu gonflable et des pédalos (WFS-378 / 386), texture de la falaise (WFS-383), reflets des façades (WFS-400) ;
  - aucun actif ajouté, aucun paquet `.wfcarte` changé (RG-E15-01 sans objet).

## Captures pour le lead dev (21 après)

Dossier : [`qa/pr/WFS-401`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `mer-bord-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-bord-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-bord-nuit.jpg?raw=true) |
| `mer-bord` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-bord.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-bord.jpg?raw=true) |
| `mer-eclaboussure-pres-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-eclaboussure-pres-1.jpg?raw=true) |
| `mer-eclaboussure-pres-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-eclaboussure-pres-2.jpg?raw=true) |
| `mer-eclaboussure` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-eclaboussure.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-eclaboussure.jpg?raw=true) |
| `mer-falaise` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-falaise.jpg?raw=true) |
| `mer-marina-bassin` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-marina-bassin.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-marina-bassin.jpg?raw=true) |
| `mer-marina` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-marina.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-marina.jpg?raw=true) |
| `mer-promenade-dos` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-promenade-dos.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-promenade-dos.jpg?raw=true) |
| `mer-promenade-soleil` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-promenade-soleil.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-promenade-soleil.jpg?raw=true) |
| `mer-reflets-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-reflets-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-reflets-nuit.jpg?raw=true) |
| `mer-rochers-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-rochers-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-rochers-nuit.jpg?raw=true) |
| `mer-rochers-pres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-rochers-pres.jpg?raw=true) |
| `mer-rochers` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-rochers.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-rochers.jpg?raw=true) |
| `mer-sillage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-sillage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-sillage.jpg?raw=true) |
| `mer-suite-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-suite-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-suite-1.jpg?raw=true) |
| `mer-suite-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-suite-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-suite-2.jpg?raw=true) |
| `mer-suite-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-suite-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-suite-3.jpg?raw=true) |
| `mer-suite-4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-suite-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-suite-4.jpg?raw=true) |
| `mer-ville-riviere-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-ville-riviere-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-ville-riviere-nuit.jpg?raw=true) |
| `mer-ville-riviere` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/avant-mer-ville-riviere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/apres-mer-ville-riviere.jpg?raw=true) |

## Zooms

| Zoom | Avant (main) | Après (branche) |
|---|---|---|
| Soleil bas, niveaux étirés 60–100 % (RG-03) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/zoom-soleil-avant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/zoom-soleil-apres.jpg?raw=true) |
| Dauphin vu de la promenade, ×5 (RG-04) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/zoom-eclaboussure-loin-avant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/zoom-eclaboussure-loin-apres.jpg?raw=true) |
| Gerbe en gros plan, 0,2 s puis 0,45 s, ×4 (RG-04) | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/zoom-eclaboussure-apres-1.jpg?raw=true) ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-401/qa/pr/WFS-401/zoom-eclaboussure-apres-2.jpg?raw=true) |
