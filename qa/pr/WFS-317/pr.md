Agent : Claude, agent 1 (01UY1Vv2)

**Ticket** : WFS-317 — règles : `regles-de-gestion/WFS-317.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : toujours 21 img/s en Basse ; la mesure du téléphone (APK 916, I9) désigne les postes, puis, décision de l'utilisateur à 17:27 UTC : « pour la Ville, il faut encore −70 % des personnes, c'est juste un décor de jeu ».
- **Mesure du téléphone (RG-01)**, Ville de nuit, Basse 60 % :
  - à 10 img/s : 372 appels, 439 k triangles (décor 32 %, voitures 41 %, personnes 27 %), GPU scène 88,7 ms ;
  - à 20 img/s : 225 appels, 212 k triangles, GPU scène 43,4 ms.
  - Le temps suit les triangles de la passe de scène. Voitures → WFS-265 (agent dev X) ; à moi : personnes et décor (PO).
- **Ce qui change** (partie 2, RG-02 et RG-03 ; accord de zone du PO pour `pietons.cpp` et `conducteurs.*`) :
  - **`src/world/pietons.cpp`**, défaut corrigé : le niveau de détail d'un piéton se choisissait sur la distance le long de la route depuis la voiture, sans l'écart du trottoir ni le recul de la caméra. Des adultes et des enfants étaient donc en niveau 0 (15 000 à 27 000 triangles) à 14-17 m de la caméra.
    - Correction : distance réelle à la caméra, toutes qualités.
    - Seuils, fondu et apparition (WFS-257) inchangés.
  - **`src/world/conducteurs.*`** : en Basse, seuils 8 / 20 / 30 m et niveau 3 « très simple » au-delà de 30 m (avant : niveau 2 jusqu'à 50 m). Moyenne et Haute inchangées ; conducteurs proches identiques.
  - **RG-03, Ville, toutes qualités** :
    - marcheurs, clients des terrasses et passants : 3 sur 10 gardés dans chaque liste, répartis régulièrement (`apparition::garderVille`, jamais plus de 3 omis de suite) ;
    - familles et enfants : même part ;
    - les rares promeneurs avec chien restent tous (leur chien est dessiné par la Ville ; exception acceptée par le PO) ;
    - résultat : 882 → 263 personnes ;
    - autres cartes inchangées.
  - **`qualite_auto`** : `texteComptes`, la mise en forme de la ligne « appels · triangles » en fonction pure testée (remarque du lead dev sur #270).
- **Comment c'est vérifié** :
  - tests CPU : `test_conducteurs` (seuils de la Basse, hystérésis à 30 m, retour en Haute), `test_apparition_pietons` (3 sur 10, régularité), `test_qualite` (texte des comptes) ; toute la construction passe ;
  - **banc** (lavapipe, Ville de nuit, Basse, 2340×1080, relevé objet par objet, image 301) :

    | | Personnes | Triangles des personnes | Scène |
    |---|---|---|---|
    | Avant | 882 | 171 k | 413 k |
    | Après | 263 | **40 k (−77 %)** | **280 k (−32 %)** |

  - au départ (vue Proche), la ligne « Rendu » des captures passe de 383 appels / 554 k triangles (personnes 44 %) à 319 appels / 450 k (personnes 10 %).
- **Limites et risques connus** :
  - Banc en rendu logiciel : les img/s du téléphone diront le gain. RG-03 (≥ 30 img/s stables) à confirmer par l'utilisateur, 🟡 après la fusion.
  - Les voitures (41 à 58 % des triangles sur le téléphone) restent le 1er poste : WFS-265. Le décor au loin (≈ 42 k triangles en gros morceaux à plus de 140 m) est confié à WFS-265 par le PO.
  - Familles de triangles approchées : conducteurs et pilotes comptés en personnes, poussettes en décor.
  - La ville est volontairement moins peuplée : moins de monde sur les trottoirs et aux terrasses, sur décision de l'utilisateur.

## Captures pour le lead dev (23 après)

Dossier : [`qa/pr/WFS-317`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `parties-1-chargement` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-1-chargement.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-1-chargement.jpg?raw=true) |
| `parties-1-reglages` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-1-reglages.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-1-reglages.jpg?raw=true) |
| `parties-1-ville-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-1-ville-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-1-ville-nuit.jpg?raw=true) |
| `parties-2-chargement` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-2-chargement.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-2-chargement.jpg?raw=true) |
| `parties-2-reglages` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-parties-2-reglages.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-parties-2-reglages.jpg?raw=true) |
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
