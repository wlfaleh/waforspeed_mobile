Agent : Claude, agent 1 (01UY1Vv2)

**Ticket** : WFS-193 — règles : `regles-de-gestion/WFS-193.md` (et Suivi : méthode (A) accordée par le PO le 2026-11-09, en commençant par 3 pilotes)

## Description

- **Demande de l'utilisateur** (en une phrase) : voitures sans marque (objectif de l'utilisateur, file A) : petites retouches de forme pour qu'on ne reconnaisse plus les marques, sur les 20 voitures. Cette PR est la **PR pilote** demandée par le PO : **Compass et 208** (et l'outil), acceptées par le lead dev à la 3e revue. La X5 en est retirée sur décision du PO (2026-11-09, option (a)) : elle sera reprise dans une 2e PR du même ticket.
- **Méthode** :
  - Les sources glTF ne sont pas dans le dépôt : les `assets/cars/<id>.wfm` sont retouchés directement par un outil rejouable (comme WFS-192).
  - Le simple déplacement de sommets a été écarté (`qa/pr/WFS-193/essais/x5-essai-echelle.png`).
  - Méthode (A), accordée par le PO : la calandre de marque est retirée et remplacée par une grille générique faite par l'outil (rien téléchargé).
- **Ce qui change** :
  - **`tools/retoucher_formes.mjs`** (nouveau) et **`tools/formes.json`** :
    - repérage : `--lister`, `--zone` ;
    - **retirer** : triangles vidés, nombre attendu contrôlé (`"triangles"`) ;
    - **grille** : fond, **cadre en relief**, **barres fermées** (faces de bout), **galbe du nez** (recul aux bords), forme en **trapèze** possible. Un matériau est ajouté à la fin (rang des matériaux inchangé) ;
    - déplacement / échelle avec fondu ;
    - empreintes avant / après (sommets, indices, matériaux) : l'outil s'arrête sans rien écrire hors de ces deux états. Le `.lod` est régénéré, les tangentes et la boîte recalculées.
  - **`tools/convertir_voitures.sh`** : lance `retoucher_formes.mjs` juste après `effacer_logos.mjs`. Une voiture absente de `formes.json` est passée sans erreur.
  - **`tools/effacer_logos.mjs`** : accepte les matériaux ajoutés à la fin.
  - **Voitures** :
    - **Compass** : calandre à 7 fentes retirée (705 + 15 621 triangles). Grille à 3 barres, cadre, galbe 4,5 cm ;
    - **208** : treillis en losanges retiré (251 triangles) et **socle du lion** retiré (relief au centre du fond sombre, 748 triangles). Grille **en trapèze** (forme de l'ouverture d'origine) à 5 barres, cadre, galbe 3,5 cm.
  - **`src/render/feux_modele.cpp`** (banc) : `WFS_VITRINE_NUIT=1` (vitrines de nuit).
- **Comment c'est vérifié** :
  - `test_fidelite_voitures` OK (20 voitures, dimensions ±1 % ou écarts connus) ;
  - **88 tests CPU verts** après synchronisation avec `main` ;
  - `retoucher_formes.mjs --verifier` et `effacer_logos.mjs --verifier` : « déjà fait » partout ;
  - captures :
    - vitrines de jour : `captures_pr.sh`, avant = `main` ;
    - **gros plans du nez** (rendus en 2560×1440 puis recadrés), jour et nuit, de face et de trois quarts ;
    - feux de nuit.
    Les « avant » de nuit et des gros plans sont pris avec le même binaire et les `.wfm` / `.lod` de `main`.
- **Limites et risques connus** :
  - **Pilote** : une retouche (la calandre) par voiture. Feux, jantes et autres signatures (crocs lumineux et feux à 3 griffes de la 208) viendront au lot suivant, avec les 17 autres voitures.
  - De nuit, les grilles restent sombres : les phares éclairent devant la voiture, pas sa calandre, comme sur une vraie voiture. 
  - Les licences recevront la phrase de modification WFS-193 au lot final .
  - Paquets npm `meshoptimizer@0.22.0` (MIT) et `sharp@0.33.5` (Apache-2.0), utilisés pour régénérer le `.lod` : notés au journal.

## Captures pour le lead dev (avant = `main`)

| Capture | Avant | Après |
|---|---|---|
| `nez-compass-face-jour` — Compass, gros plan de face, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-compass-face-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-compass-face-jour.jpg?raw=true) |
| `nez-compass-34-jour` — Compass, gros plan de trois quarts, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-compass-34-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-compass-34-jour.jpg?raw=true) |
| `nez-compass-face-nuit` — Compass, gros plan de face, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-compass-face-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-compass-face-nuit.jpg?raw=true) |
| `nez-compass-34-nuit` — Compass, gros plan de trois quarts, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-compass-34-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-compass-34-nuit.jpg?raw=true) |
| `vitrine-compass-a2` — Compass, de dos, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-compass-a2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-compass-a2.jpg?raw=true) |
| `vitrine-compass-feux-nuit` — Compass, feux de nuit (frein) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-compass-feux-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-compass-feux-nuit.jpg?raw=true) |
| `nez-p208-face-jour` — 208, gros plan de face, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-p208-face-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-p208-face-jour.jpg?raw=true) |
| `nez-p208-34-jour` — 208, gros plan de trois quarts, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-p208-34-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-p208-34-jour.jpg?raw=true) |
| `nez-p208-face-nuit` — 208, gros plan de face, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-p208-face-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-p208-face-nuit.jpg?raw=true) |
| `nez-p208-34-nuit` — 208, gros plan de trois quarts, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-nez-p208-34-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-nez-p208-34-nuit.jpg?raw=true) |
| `vitrine-p208-a2` — 208, de dos, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-p208-a2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-p208-a2.jpg?raw=true) |
| `vitrine-p208-feux-nuit` — 208, feux de nuit (frein) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-p208-feux-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-p208-feux-nuit.jpg?raw=true) |

Les vitrines de jour de face et de trois quarts (`vitrine-*-a0`, `-a1`), les vues d'ensemble et les feux de jour sont dans `qa/pr/WFS-193/`.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv2MVfXZF3cPf1r8qRq
