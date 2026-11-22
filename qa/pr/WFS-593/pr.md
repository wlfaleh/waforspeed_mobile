## Agent : Claude, agent dev3d (01UY1Vv4Di)

**Ticket** : WFS-593 — règles : `regles-de-gestion/WFS-593.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : « passer à seulement 10 % des personnes dans les cartes » (2026-11-22 12:00 UTC).
- **Ce qui change** :
  - `src/world/foule_apparition.h` : **un seul réglage** `PERSONNES_GARDEES_SUR_100 = 10` et le tirage régulier `garderPart(n)` (la 1re personne de chaque liste, puis une sur 10 ; total = ⌈10 % de main⌉, RG-01, RG-02) ; `promeneurGarde(i)` pour les promeneurs avec chien.
  - `src/world/pietons.cpp` : le tirage s'applique **après** les tirages existants (`garder()` WFS-222, `garderVille()` WFS-317), donc sur ce que `main` posait, catégorie par catégorie : marcheurs, promeneurs, passants, clients des terrasses, serviettes, baigneurs, fêtes de la plage. Les marcheurs de **toutes** les cartes passent par la répartition sur tout le tour de la Ville (`repartirSurTour`, WFS-317) : avant, hors Ville, ils étaient serrés dans le 1er km (plafond de life.js). Les familles (enfants, poussettes) se forment parmi les marcheurs gardés, comme avant. Journal du chargement : comptes avant / après par catégorie et par carte (RG-06).
  - Accroches annoncées au journal : `src/world/circuit.cpp` (+3 lignes : 10 % des spectateurs des tribunes, une place occupée sur 10) et `src/world/ville.cpp` (+2 lignes : le chien n'est dessiné que si son promeneur est gardé ; sinon un chien marcherait seul en laisse).
  - `tests/apparition_pietons_test.cpp` : 5 cas WFS-593 (RG-07).
  - Rien n'est créé en course (RG-03) ; aucun modèle, animation ni code des personnes supprimé (RG-04) ; conducteurs et deux-roues inchangés.
- **Comptes (journal du chargement des bancs, `main` → branche)** :

| Carte | Total | Détail |
|---|---|---|
| Ville | 245 → 27 | marcheurs 32 → 4, promeneurs 5 → 1, passants 1 → 1, terrasses 207 → 21 |
| Corniche | 252 → 27 | marcheurs 110 → 11, serviettes 67 → 7, baigneurs 43 → 5, fêtes 32 → 4 |
| Campagne (en pause) | 37 → 4 | marcheurs 37 → 4 |
| Grand Prix (en pause) | 72 → 8 marcheurs ; 381 → 39 spectateurs | (qualité Haute) |

- **Comment c'est vérifié** : 157 tests CPU verts (`tools/qa/tests_cpu.sh build`), dont `test_apparition_pietons` ; bancs `pietons`, `vie`, `cartes` en 1280×720 (lavapipe) avant / après ; parcours complet (banc `parcours`, 1280×720) : accueil → choix → course → fin passent ; l'étape « fin puis Menu → accueil » échoue **de la même façon sur `main` (699b39e9)** : 2 erreurs identiques sur `main` et sur la branche, aucune autre. C'est le rouge déjà suivi par WFS-589 (`@cicd`, « parcours fin → Menu »), sans lien avec ce ticket.
- **Paquets `.wfcarte` (RG-08)** : aucune personne n'est cuite dans un paquet (les personnes sont posées au chargement par `pietons.cpp` et `circuit.cpp`) : rien à publier.
- **Limites et risques connus** : la ligne « Ville (WFS-176) : … 10 chien(s) » du journal compte encore les promeneurs du plan (seul le chien des promeneurs gardés est dessiné). Le placement des marcheurs hors Ville change (répartis sur tout le tour au lieu du 1er km) : c'est voulu par RG-02. Performance : 10 fois moins de personnes à animer et à dessiner, donc égale ou meilleure (non mesuré sur téléphone).

## Captures pour le lead dev (16 paires ciblées, jour ; Grand Prix et Ville de nuit)

Dossier : [`qa/pr/WFS-593`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593) — produit par `tools/qa/captures_pr.sh`.

Captures « avant » : `origin/main` = `699b39e965f6c1fd3c8e0b1b5ac34a7f3d313b81` ; cache : 0 banc(s) relu(s), 3 banc(s) rejoué(s) ; main compilé : oui.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `carte-circuit-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-carte-circuit-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-carte-circuit-jour.jpg?raw=true) |
| `carte-plage-corniche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-carte-plage-corniche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-carte-plage-corniche.jpg?raw=true) |
| `carte-plage-rochers` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-carte-plage-rochers.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-carte-plage-rochers.jpg?raw=true) |
| `carte-ville-casino-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-carte-ville-casino-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-carte-ville-casino-nuit.jpg?raw=true) |
| `carte-ville-chien` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-carte-ville-chien.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-carte-ville-chien.jpg?raw=true) |
| `carte-ville-mobilier-rue` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-carte-ville-mobilier-rue.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-carte-ville-mobilier-rue.jpg?raw=true) |
| `carte-ville-mobilier-terrasse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-carte-ville-mobilier-terrasse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-carte-ville-mobilier-terrasse.jpg?raw=true) |
| `carte-ville-residentiel` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-carte-ville-residentiel.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-carte-ville-residentiel.jpg?raw=true) |
| `enfants-campagne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-enfants-campagne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-enfants-campagne.jpg?raw=true) |
| `enfants-plage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-enfants-plage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-enfants-plage.jpg?raw=true) |
| `enfants-ville` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-enfants-ville.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-enfants-ville.jpg?raw=true) |
| `pietons-plage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-pietons-plage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-pietons-plage.jpg?raw=true) |
| `pietons-ville-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-pietons-ville-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-pietons-ville-0.jpg?raw=true) |
| `pietons-ville-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-pietons-ville-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-pietons-ville-1.jpg?raw=true) |
| `vie-plage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-vie-plage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-vie-plage.jpg?raw=true) |
| `vie-terrasse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/avant-vie-terrasse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-593/qa/pr/WFS-593/apres-vie-terrasse.jpg?raw=true) |
