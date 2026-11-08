Agent : Claude, agent 1 (01UY1Vv2)

**Ticket** : WFS-317 — règles : `regles-de-gestion/WFS-317.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : toujours 21 img/s en Basse ; la mesure du téléphone (APK 916, I9) désigne les postes, puis, décision de l'utilisateur à 17:27 UTC : « pour la Ville, il faut encore −70 % des personnes, c'est juste un décor de jeu ».
- **Mesure du téléphone (RG-01)**, Ville de nuit, Basse 60 % :
  - à 10 img/s : 372 appels, 439 k triangles (décor 32 %, voitures 41 %, personnes 27 %), GPU scène 88,7 ms ;
  - à 20 img/s : 225 appels, 212 k triangles, GPU scène 43,4 ms.
  - Le temps suit les triangles de la passe de scène. Voitures → WFS-265 ; à moi : personnes et décor (PO) ; décor au loin → WFS-265 (PO).
- **Ce qui change** (partie 2, RG-02 et RG-03 ; accords du PO dans le Suivi de WFS-317 : zone `pietons.cpp` / `conducteurs.*`, promeneurs avec chien, répartition des marcheurs) :
  - **`src/world/pietons.cpp`**, défaut corrigé : le niveau de détail d'un piéton se choisissait sur la distance le long de la route depuis la voiture, sans l'écart du trottoir ni le recul de la caméra. Des adultes et des enfants étaient donc en niveau 0 (15 000 à 27 000 triangles) à 14-17 m de la caméra.
    - Correction : distance réelle à la caméra, toutes qualités.
    - Seuils, fondu et apparition (WFS-257) inchangés.
  - **`src/world/conducteurs.*`** : en Basse, seuils 8 / 20 / 30 m et niveau 3 « très simple » au-delà de 30 m (avant : niveau 2 jusqu'à 50 m). Moyenne et Haute inchangées ; conducteurs proches identiques.
  - **RG-03, Ville, toutes qualités : 814 → 248 personnes (−70 %)** :
    - clients des terrasses et passants : 3 sur 10 gardés dans chaque liste, réguliers (`apparition::garderVille`, jamais plus de 3 omis de suite) ;
    - **marcheurs** : sur `main`, le plafond de life.js (220) les serre tous dans le **1er km** du tour de 6,8 km (106 sur 110), et les km 2 à 7 n'en ont aucun. Désormais 3 sur 10 de ce nombre (32) sont placés au plus près de places à pas réguliers sur **tout le tour**, côtés alternés (`apparition::repartirSurTour`, testé) : 4 à 7 marcheurs par km, écart médian de 141 m, plus grand écart de 661 m, dans les quartiers où la liste d'origine n'a aucun marcheur ;
    - familles et enfants : même part ; les 5 promeneurs avec chien restent tous (leur chien est dessiné par la Ville) ;
    - autres cartes inchangées.
  - **`qualite_auto`** : `texteComptes`, la mise en forme de la ligne « appels · triangles » en fonction pure testée (remarque du lead dev sur #270).
  - **`bench_perf.cpp`** : le banc des parties prend aussi la Ville de jour à 0, 30 et 60 s (avant : la nuit seulement).
- **Comment c'est vérifié** :
  - **Une seule base pour l'avant et l'après** : branche synchronisée avec `main` (WFS-265, 316, 326 compris), puis tests, parcours et `captures_pr.sh`, sans fusion de `main` pendant les captures. La part « voitures » ne bouge pas.
  - **Tests** : 74 tests CPU verts, dont `test_apparition_pietons` (3 sur 10, répartition sur le tour, listes courtes), `test_conducteurs` (seuils de la Basse, hystérésis à 30 m, retour en Haute) et `test_qualite` (texte des comptes) ; parcours complet 0 écart (vue Arrière et Cockpit).
  - **Ligne « Rendu » des captures** (Basse, départ, vue Proche, 2340×1080) :

    | | Total | Voitures | Décor | Personnes |
    |---|---|---|---|---|
    | Ville de nuit, avant | 380 appels, 550 k | 34 % ≈ 187 k | 22 % ≈ 121 k | 44 % ≈ 242 k |
    | Ville de nuit, après | 307 appels, **344 k (−37 %)** | 54 % ≈ 186 k | 35 % ≈ 120 k | 11 % ≈ **38 k (−84 %)** |
    | Ville de jour, avant | 335 appels, 569 k | 32 % ≈ 182 k | 22 % ≈ 125 k | 45 % ≈ 256 k |
    | Ville de jour, après | 260 appels, **347 k (−39 %)** | 53 % ≈ 184 k | 36 % ≈ 125 k | 11 % ≈ **38 k (−85 %)** |

  - **Rue au-delà du 1er km** : `parties-1-ville-nuit-30s` et `parties-2-ville-jour-30s` (1,66 km). Avant, aucun marcheur près de la route à gauche ; après, des marcheurs sur le trottoir du parc. Captures à 60 s (3,73 km, port) : la voiture du banc est arrêtée sur le trottoir, avant comme après (pilote automatique du banc, sans lien avec cette PR) ; personne au port, ni avant ni après.
- **Limites et risques connus** :
  - Banc en rendu logiciel : les img/s du téléphone diront le gain. L'objectif ≥ 30 img/s stables en Basse est à confirmer par l'utilisateur, 🟡 après la fusion.
  - Les voitures restent le 1er poste (WFS-265). Le décor au loin est confié à WFS-265 par le PO.
  - Familles de triangles approchées : conducteurs et pilotes comptés en personnes, poussettes en décor.
  - La Ville est volontairement moins peuplée : moins de monde aux terrasses, sur décision de l'utilisateur. La capture `parties-2-ville-jour-*` n'a pas d'« avant » : le banc ne prenait pas ces images sur `main`.

## Captures pour le lead dev (35 après)

Dossier : [`qa/pr/WFS-317`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `parties-1-chargement` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-1-chargement.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-1-chargement.jpg?raw=true) |
| `parties-1-reglages` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-1-reglages.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-1-reglages.jpg?raw=true) |
| `parties-1-suite-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-1-suite-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-1-suite-0.jpg?raw=true) |
| `parties-1-suite-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-1-suite-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-1-suite-1.jpg?raw=true) |
| `parties-1-suite-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-1-suite-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-1-suite-2.jpg?raw=true) |
| `parties-1-suite-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-1-suite-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-1-suite-3.jpg?raw=true) |
| `parties-1-suite-4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-1-suite-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-1-suite-4.jpg?raw=true) |
| `parties-1-suite-5` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-1-suite-5.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-1-suite-5.jpg?raw=true) |
| `parties-1-ville-nuit-0s` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-1-ville-nuit-0s.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-1-ville-nuit-0s.jpg?raw=true) |
| `parties-1-ville-nuit-30s` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-1-ville-nuit-30s.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-1-ville-nuit-30s.jpg?raw=true) |
| `parties-1-ville-nuit-60s` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-1-ville-nuit-60s.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-1-ville-nuit-60s.jpg?raw=true) |
| `parties-1-ville-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-1-ville-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-1-ville-nuit.jpg?raw=true) |
| `parties-2-chargement` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-2-chargement.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-2-chargement.jpg?raw=true) |
| `parties-2-reglages` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-2-reglages.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-2-reglages.jpg?raw=true) |
| `parties-2-ville-jour-0s` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-2-ville-jour-0s.jpg?raw=true) |
| `parties-2-ville-jour-30s` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-2-ville-jour-30s.jpg?raw=true) |
| `parties-2-ville-jour-60s` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-2-ville-jour-60s.jpg?raw=true) |
| `parties-2-ville-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-2-ville-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-2-ville-jour.jpg?raw=true) |
| `conducteurs-tenue-campagne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-conducteurs-tenue-campagne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-conducteurs-tenue-campagne.jpg?raw=true) |
| `conducteurs-tenue-circuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-conducteurs-tenue-circuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-conducteurs-tenue-circuit.jpg?raw=true) |
| `conducteurs-tenue-plage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-conducteurs-tenue-plage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-conducteurs-tenue-plage.jpg?raw=true) |
| `conducteurs-tenue-ville-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-conducteurs-tenue-ville-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-conducteurs-tenue-ville-nuit.jpg?raw=true) |
| `conducteurs-tenue-ville` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-conducteurs-tenue-ville.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-conducteurs-tenue-ville.jpg?raw=true) |
| `enfants-campagne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-enfants-campagne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-enfants-campagne.jpg?raw=true) |
| `enfants-cheveux-longs` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-enfants-cheveux-longs.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-enfants-cheveux-longs.jpg?raw=true) |
| `enfants-plage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-enfants-plage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-enfants-plage.jpg?raw=true) |
| `enfants-ville-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-enfants-ville-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-enfants-ville-1.jpg?raw=true) |
| `enfants-ville` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-enfants-ville.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-enfants-ville.jpg?raw=true) |
| `pietons-plage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-pietons-plage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-pietons-plage.jpg?raw=true) |
| `pietons-ville-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-pietons-ville-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-pietons-ville-0.jpg?raw=true) |
| `pietons-ville-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-pietons-ville-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-pietons-ville-1.jpg?raw=true) |
| `vie-enseigne-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-vie-enseigne-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-vie-enseigne-nuit.jpg?raw=true) |
| `vie-plage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-vie-plage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-vie-plage.jpg?raw=true) |
| `vie-portique` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-vie-portique.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-vie-portique.jpg?raw=true) |
| `vie-terrasse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-vie-terrasse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-vie-terrasse.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv2MVfXZF3cPf1r8qRq
