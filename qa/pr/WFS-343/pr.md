Agent : Claude, agent 3 (01UY1Vv3)

## Demande
Demande de l'utilisateur (2026-11-09, « tout en dynamique ») : une carte est entièrement décrite par les données de son paquet, et rien de propre à une carte n'est écrit dans le code. WFS-343 est le moteur : le tracé, les chocs et le décor sont lus du paquet. La neige est traitée dans WFS-346. Règles : `regles-de-gestion/WFS-343.md` (RG-01, RG-03, RG-04, RG-05).

## Ce qui change
- **Données d'une carte** (`src/world/cartes_v2/carte_donnees.*`, nouveau) : trois blocs texte du paquet `.wfcarte`, au format 1, avec une instruction par ligne.
  - `trace` : nom, description, quartiers, graine, bord, trafic, `checkpoint_m`, couleurs de la vignette, puis les routes. Une route est un `addRoad` du jeu web : entrée, tenue, sortie, courbe, montée. Les sections sont typées (ligne, virage, épingle, autoroute…) ; aucun rond-point (WFS-283).
  - `chocs` : boîtes et cylindres (segment, avance, latéral, taille, couleur, hauteur, genre), refusés s'ils sont sur la chaussée.
  - `decor` : ciel et brouillard, bandes du sol, `herbe 0|1`, maisons, sapins, feuillus, cyprès, lampadaires, lumières, guirlandes, et `repete` pour répéter une instruction.
  - Chaque erreur est refusée avec sa ligne et sa raison. Un bloc faux rend la carte absente du choix du terrain, avec la raison au journal du jeu ; jamais de plantage (RG-01).
- **Registre** : les cartes des paquets du téléphone sont découvertes par `decouvrir()`.
  - Un paquet est vérifié (CRC, empreinte des données) avant d'être lu.
  - `.part`, paquet abîmé ou paquet d'une autre génération : refusés.
  - Les 4 cartes du code ne sont jamais remplacées.
  - L'empreinte d'une carte de données vient de son identifiant et du format des blocs (`empreinteDonnees`) ; `empreinteAttendue` la rend pour ces cartes.
- **Tracé** : les routes et sections du paquet passent par `Track::build` → `MapDef::construire`. Le départ est le début du tracé. Les checkpoints suivent `checkpoint_m` de la carte, puis reviennent à la valeur de `configgame.json` sur les autres cartes (système « réglages de la carte », ordre 1).
- **Décor générique** (`carte_donnees_decor.cpp`) : sous-classe du socle commun `CarteDecor`, le même que la Campagne et le Grand Prix.
  - Il gère les bandes du sol, les arbres, les lampadaires, les morceaux, le LOD et les chocs des objets.
  - Les maisons ont des murs et un toit, et sont solides.
  - Les obstacles du bloc `chocs` sont dessinés et déclarés aux chocs.
  - Les lumières sont émissives à la couleur de la palette (`MAT_EMTEX`, part `P_TEXTE1` inutilisée hors du Grand Prix).
  - Les couleurs de la carte vont dans les cases libres de la palette.
- **Choix du terrain** (`terrain.cpp`, exception PO) : les 4 cartes du code, puis les cartes des paquets (RG-03).
  - Vignette générique aux couleurs de la carte, compteur « 05 / 06 », points.
  - Longueur et quartiers lus du paquet.
  - Les cartes d'essai sont cachées aux joueurs (`cache 1`) et montrées au banc (`WFS_CARTES_TEST=1`).
  - Un message passager n'est plus montré que sur sa carte.
- **Cartes d'essai** : `paquets/sources/noel-test` (village enneigé, chalets, sapins, guirlandes) et `paquets/sources/automne-test` (RG-05 : autre saison par les données seules).
- **Accroches annoncées au journal** :
  - `src/game/track.h` (+3 lignes) et `track.cpp` (+2 lignes) : champ `MapDef::construire`, pointeur `g_cartesDonnees` (cherché par `mapById`), `Track::poserSections`.
  - **`src/world/decor_carte.h` / `.cpp` (accord écrit du PO, 2026-11-09 13:50 UTC)** : `couleursCarte()` (couleurs de la carte dans la palette) et `touffes()` (herbe facultative). Ce sont 4 lignes ; les valeurs par défaut gardent la Campagne et le Grand Prix inchangés.
  - `CMakeLists.txt` : 2 lignes pour le test.

## Comment c'est vérifié
- **`test_cartes_v2_donnees`** (nouveau) :
  - Lecture des deux cartes d'essai.
  - 18 blocs faux refusés : format futur, instruction inconnue, tour trop court, courbe impossible, rond-point, section hors du tour ou qui chevauche, obstacle sur la chaussée ou hors du tour, genre inconnu, maison sur la chaussée, bande mal rangée, `repete` de pas nul…
  - Registre : `mapById`, carte du code jamais remplacée, identifiant invalide.
  - Tracé construit depuis les données (1220 segments, sections, courbe de la 2e route). Ville inchangée.
  - Cuisson déterministe et découverte : paquet juste → enregistré ; CRC faux, autre génération ou `.part` → absente.
- **Tests CPU** : 89/89 OK (`tools/qa/tests_cpu.sh`).
- **Banc `paquets`**, parcours RG-04 : 0 écart.
  1. Accueil, puis choix du terrain : les 2 cartes d'essai sont lues des paquets.
  2. Course sur la carte de Noël : checkpoints tous les 400 m (donnée de la carte), chocs déclarés, course lancée.
  3. Nuit.
  4. Autre saison : checkpoints tous les 700 m.
  5. Retour au menu, puis la Ville : checkpoints revenus à la valeur de `configgame.json`.
  - Les étapes WFS-306 / 307 du même banc restent OK.
- **Campagne et Grand Prix inchangés** (condition du PO) : captures avant / après de jour et de nuit (banc `cartes`), plus les tests CPU des cartes (`test_cartes`, références du jeu web).

## Limites et risques
- **Accroche `decor_carte.*`** (E2, 4 lignes) à relire. Le Grand Prix utilise la part `P_TEXTE1` pour ses lettres ; les cartes de données l'utilisent pour leurs lumières. Les deux sont exclusifs : une seule carte est active à la fois.
- Les arbres sont posés sur le sol des bandes. Les couleurs de la carte sont limitées aux cases libres de la palette ; au-delà, c'est la couleur la plus proche qui est prise.
- **Hors du ticket** :
  - La neige (sol enneigé et traces, flocons, adhérence) est dans WFS-346.
  - La liste `cartes.json` ne propose pas encore les cartes de données au téléchargement : ici, une carte est lue d'un paquet déjà sur le téléphone.
  - Une carte de données gardée comme dernière carte au démarrage n'est pas encore relue avant la sauvegarde : le jeu repart sur la carte par défaut.
- Pas d'essai sur le téléphone : la carte d'essai n'est pas publiée.

**Ticket** : WFS-343 — règles : `regles-de-gestion/WFS-343.md`

## Captures pour le lead dev
La Campagne et le Grand Prix, de jour et de nuit, sont **identiques au pixel près** avant et après (0 pixel différent sur 2400×1080, comparaison faite sur les originaux PNG). Les cartes de données n'existent pas sur `main`, d'où le « — » dans la colonne avant.
 (10 après)

Dossier : [`qa/pr/WFS-343`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-343/qa/pr/WFS-343) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `carte-campagne-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-343/qa/pr/WFS-343/avant-carte-campagne-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-343/qa/pr/WFS-343/apres-carte-campagne-jour.jpg?raw=true) |
| `carte-campagne-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-343/qa/pr/WFS-343/avant-carte-campagne-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-343/qa/pr/WFS-343/apres-carte-campagne-nuit.jpg?raw=true) |
| `carte-circuit-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-343/qa/pr/WFS-343/avant-carte-circuit-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-343/qa/pr/WFS-343/apres-carte-circuit-jour.jpg?raw=true) |
| `carte-circuit-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-343/qa/pr/WFS-343/avant-carte-circuit-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-343/qa/pr/WFS-343/apres-carte-circuit-nuit.jpg?raw=true) |
| `donnees-automne-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-343/qa/pr/WFS-343/apres-donnees-automne-jour.jpg?raw=true) |
| `donnees-noel-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-343/qa/pr/WFS-343/apres-donnees-noel-jour.jpg?raw=true) |
| `donnees-noel-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-343/qa/pr/WFS-343/apres-donnees-noel-nuit.jpg?raw=true) |
| `donnees-retour-ville` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-343/qa/pr/WFS-343/apres-donnees-retour-ville.jpg?raw=true) |
| `donnees-terrain-noel-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-343/qa/pr/WFS-343/apres-donnees-terrain-noel-nuit.jpg?raw=true) |
| `donnees-terrain-noel` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-343/qa/pr/WFS-343/apres-donnees-terrain-noel.jpg?raw=true) |
