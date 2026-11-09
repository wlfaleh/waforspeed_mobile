Agent : Claude, agent cartes (01UY1Vv2dx)

**Ticket** : WFS-346 — Carte de Noël (1 bis/3) : neige lue du paquet (sol enneigé, flocons dehors seulement, adhérence de la neige). Règles : `regles-de-gestion/WFS-346.md`.

## Description

- **Demande de l'utilisateur** (en une phrase) : une carte de Noël enneigée dont tout (neige au sol, flocons, adhérence, couleurs) vient des données du paquet, avec des flocons seulement dehors et une voiture qui glisse vraiment sur la neige (demande du 2026-11-09 01:02 UTC, WFS-342, découpée par le PO).
- **Ce qui change** :
  - **Saison lue du paquet** (`src/world/cartes_v2/carte_donnees.*`, format 2 des blocs) : 3 lignes nouvelles du bloc `decor` : `adherence chaussee bas_cote` (μ 0,05 à 1,2), `neige_chaussee couverture` (0 à 1), `flocons intensite chute taille` ; bornées, une valeur fausse refuse la carte avec une raison (jamais de plantage) ; **valeurs par défaut = sec, sans neige** : les 4 cartes du code ne changent pas. `preparerSaison(carte)` au début de `Game::init` (accroche de 2 lignes). Valeurs réelles et sources : `references/reel/cartes_v2/neige.md` (μ neige tassée 0,2 à 0,3 ; flocons 2 à 5 mm, 1 à 1,5 m/s).
  - **Chaussée enneigée** (`shaders/route.glsl`, `src/render/route_revetements*`) : neige tassée gris-blanc (albédo ≈ 0,45, pas de blanc pur), traces de roues sombres et lisses de chaque voie (glace grise, asphalte qui perce), bourrelets plus clairs en rive, lignes peintes couvertes ; trottoir enneigé ; **chaussée sèche sous le toit d'un tunnel** (neige apportée par les roues sur 8 m). La couverture passe au shader par `cutoff` (mode + 4 × carte + 16 × neige) et l'abri par la couleur du matériau, inutilisée par la chaussée. Bas-côtés blancs : bandes du sol du paquet (déjà en données).
  - **Flocons** (`src/world/meteo.*`) : particules du monde autour de la caméra (intensité × 2 000), chute et balancement, traînée d'une exposition de 1/60 s ; **vue Cockpit : seulement à travers les vitres** (mêmes vitres que la pluie de WFS-334 / 354) ; **aucun flocon sous un tunnel** (position de chaque flocon le long de la route) ; la nuit, visibles surtout dans le faisceau des phares. 0 avec « réduire les animations » (comme la pluie). Coût mesuré : 0,07 ms CPU par image pour 1 200 flocons.
  - **Tunnel des cartes de données** (`carte_donnees_decor.cpp`) : section `tunnel` du tracé dessinée (piédroits et dalle de béton) ; la carte d'essai de Noël en a un (abri des captures).
  - **Adhérence dans la conduite** (`player.*`, accroches de 8 lignes, voiture du joueur seulement) : conduite du jeu (ancien modèle, `g_conduiteMonde` faux) : limite de lacet, freinage (ABS : tout le freinage × μ) et élan (antipatinage : ≤ μ g × 55 %) selon le sol ; **l'aide à la direction** ralentit pour le virage avec l'adhérence de la neige ; nouveau modèle : `sol.mu` de la carte (ABS, antipatinage, ESP le prennent déjà). Ordinateurs et cartes du code : inchangés (test).
  - **Données** : `paquets/sources/noel-test` (neige, μ 0,25 / 0,2, flocons 0,6 · 1,2 m/s · 3,5 mm, abri de 880 à 990, guirlandes coupées sous l'abri), `paquets/sources/automne-test` (μ 0,9 / 0,45, sans neige) : **tout change par les données seules** (RG-03).
  - Banc `WFS_BANC=neige` (`neige_banc.cpp`, accroche d'1 ligne dans `desktop_main.cpp`).
- **Comment c'est vérifié** :
  - `bash tools/qa/tests_cpu.sh build` : 91 tests verts, dont le nouveau `test_cartes_v2_saison` (lecture et refus des lignes de saison, valeurs par défaut = μ d'avant, cartes de Noël / automne, code du matériau de route, freinage 100-0 des 20 voitures sur neige ≈ v² / 2μg (écart maxi 6,5 %), virage ≤ μ g, conduite du jeu : 100-0 **36,8 m au sec → 145,2 m sur neige**, 0-50 km/h 3,0 s → 11,4 s, ordinateurs inchangés ; flocons : nombre, boîte qui suit la caméra, toits) et `test_cartes_v2_donnees` (format 2, 6 sections).
  - Banc `neige` : 0 écart ; freinage à fond dans le jeu : **≈ 144 m depuis 100 km/h sur neige contre 37 m sur la Ville sèche (rapport 3,9 ; réel ≈ 4)**.
  - Banc `paquets` (WFS-343) : parcours accueil → choix du terrain → course sur les cartes d'essai (jour, nuit, automne) → retour au menu → Ville.
- **Limites et risques connus** :
  - **Aurore (RG-03)** : non faite, le ciel est en E13 (`sky*`, interdit à ce ticket, WFS-214 en cours) : question posée au PO au journal (proposition : WFS-344 ou une US E13).
  - Chaussée sèche sous le **1er** tunnel de la carte seulement (une seule place dans le matériau) ; ligne au journal du jeu s'il y en a plusieurs. Les flocons, eux, s'arrêtent sous tous les tunnels.
  - Rendu vérifié en rendu logiciel (lavapipe) seulement ; coût GPU du shader de neige non mesuré sur téléphone (carte de données seulement : les cartes du code passent par une branche à 0).
  - Ticket de conduite : **🟡 À valider par l'utilisateur** sur son téléphone après la fusion.

## Captures pour le lead dev (25 après)

Dossier : [`qa/pr/WFS-346`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `neige-automne-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-automne-jour.jpg?raw=true) |
| `neige-cockpit-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-cockpit-jour.jpg?raw=true) |
| `neige-cockpit-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-cockpit-nuit.jpg?raw=true) |
| `neige-freinage-0` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-freinage-0.jpg?raw=true) |
| `neige-freinage-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-freinage-1.jpg?raw=true) |
| `neige-freinage-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-freinage-2.jpg?raw=true) |
| `neige-freinage-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-freinage-3.jpg?raw=true) |
| `neige-freinage-4` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-freinage-4.jpg?raw=true) |
| `neige-jour-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-jour-arriere.jpg?raw=true) |
| `neige-nuit-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-nuit-arriere.jpg?raw=true) |
| `neige-tunnel-arriere-dedans` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-tunnel-arriere-dedans.jpg?raw=true) |
| `neige-tunnel-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-tunnel-avant.jpg?raw=true) |
| `neige-tunnel-bouche` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-tunnel-bouche.jpg?raw=true) |
| `neige-tunnel-dedans` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-tunnel-dedans.jpg?raw=true) |
| `neige-virage-0` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-virage-0.jpg?raw=true) |
| `neige-virage-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-virage-1.jpg?raw=true) |
| `neige-virage-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-virage-2.jpg?raw=true) |
| `neige-virage-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-virage-3.jpg?raw=true) |
| `neige-virage-4` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-neige-virage-4.jpg?raw=true) |
| `donnees-automne-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/avant-donnees-automne-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-donnees-automne-jour.jpg?raw=true) |
| `donnees-noel-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/avant-donnees-noel-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-donnees-noel-jour.jpg?raw=true) |
| `donnees-noel-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/avant-donnees-noel-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-donnees-noel-nuit.jpg?raw=true) |
| `donnees-retour-ville` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/avant-donnees-retour-ville.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-donnees-retour-ville.jpg?raw=true) |
| `donnees-terrain-noel-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/avant-donnees-terrain-noel-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-donnees-terrain-noel-nuit.jpg?raw=true) |
| `donnees-terrain-noel` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/avant-donnees-terrain-noel.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-346/qa/pr/WFS-346/apres-donnees-terrain-noel.jpg?raw=true) |

Les captures « neige-* » n'ont pas d'« avant » : le banc `neige` n'existe pas sur `main` ; les « avant » de la carte de Noël sont les `donnees-*` du banc `paquets` (WFS-343).

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv2dxNQxhJG6xe4ofua
