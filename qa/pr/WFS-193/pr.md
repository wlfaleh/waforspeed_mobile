Agent : Claude, agent 1 (01UY1Vv2)

**Ticket** : WFS-193 — règles : `regles-de-gestion/WFS-193.md` (et Suivi : méthode (A) accordée par le PO le 2026-11-09, en commençant par 3 pilotes)

## Description

- **Demande de l'utilisateur** (en une phrase) : voitures sans marque (objectif de l'utilisateur, file A) : petites retouches de forme pour qu'on ne reconnaisse plus les marques, sur les 20 voitures. Cette PR est la **PR pilote** demandée par le PO : X5, Compass, 208, pour avis du lead dev avant les 17 autres.
- **Méthode** :
  - **Pas de reconversion possible** : les sources glTF ne sont pas dans le dépôt, et 4 voitures ne se reconvertissent pas. Je retouche donc directement les `assets/cars/<id>.wfm`, avec un outil rejouable comme celui de WFS-192.
  - **Premier essai écarté** : avec le seul déplacement de sommets, la X5 garde des haricots reconnaissables, aux barres déformées en vagues (`qa/pr/WFS-193/essais/x5-essai-echelle.png`).
  - **Méthode retenue, (A) accordée par le PO** : la calandre de marque est retirée et remplacée par une grille générique faite par l'outil (rien téléchargé).
- **Ce qui change** :
  - **`tools/retoucher_formes.mjs`** (nouveau) et **`tools/formes.json`** (données), même modèle que `effacer_logos.mjs` :
    - `--lister` (pièces d'une voiture) et `--zone` (sommets de chaque matériau dans une boîte), pour le repérage ;
    - retouches :
      - **retirer** : triangles dont le centre est dans une boîte vidés (indices confondus), aucun sommet retiré ;
      - **grille** : fond aux coins arrondis et barres horizontales en relief, matériau ajouté **à la fin** (rang des matériaux inchangé, `correctifs_voitures.h`) ;
      - **déplacement / échelle** de sommets avec fondu (gardé pour les lots suivants) ;
    - contrôle avant écriture par empreinte (sommets, indices, matériaux) : rejouable, et l'outil s'arrête sans rien écrire si l'état n'est ni « avant » ni « après ». Après un retrait ou une grille, le `.lod` est régénéré (`alleger_voiture.mjs`), les tangentes recalculées et la boîte d'en-tête mise à jour.
  - **`tools/effacer_logos.mjs`** : accepte les matériaux ajoutés à la fin (`nM ≥ attendu`).
  - **Voitures** :
    - **X5** : les deux « haricots » (cadre, barres et fond : 2 289 + 222 triangles) sont remplacés par une grille d'un seul tenant à 6 barres, gris anthracite métallisé ;
    - **Compass** : la calandre à 7 fentes (cadres chromés 705 triangles, fond 15 621) est remplacée par une grille à 3 barres ;
    - **208** : le treillis en losanges (251 triangles) est remplacé par une grille à 5 barres devant le fond sombre d'origine.
  - **`src/render/feux_modele.cpp`** (banc) : `WFS_VITRINE_NUIT=1` pour les vitrines de nuit.
- **Comment c'est vérifié** :
  - **dimensions** : `test_fidelite_voitures` OK (20 voitures, ±1 % ou écarts connus, habitacle et dessous de caisse) ;
  - **87 tests CPU verts** après synchronisation avec `main` ;
  - `retoucher_formes.mjs --verifier` et `effacer_logos.mjs --verifier` : tout est « déjà fait », rien à réécrire ;
  - **captures** :
    - jour : `captures_pr.sh WFS-193 vitrines-feux` (avant = `main`) ;
    - nuit : banc `WFS_VITRINE_NUIT=1`, même binaire, avant = `.wfm` et `.lod` de `main`.
- **Limites et risques connus** :
  - **Pilote** : une retouche (la calandre) par voiture. Les 2 à 4 retouches par voiture (feux, jantes) viendront avec les lots suivants, après avis du lead dev sur la méthode.
  - **208** : les fines bandes lumineuses verticales sous les phares (signature de la marque) sont encore visibles. Elles sont dans la texture des phares et seront traitées au lot suivant.
  - **Licences** : les pièces ajoutées sont faites par l'outil. La phrase de modification WFS-193 sera ajoutée aux fichiers de licence avec le lot final (X5 en CC BY-NC-SA, voir WFS-194).
  - **Paquets npm** pour régénérer le `.lod` (`meshoptimizer@0.22.0` MIT, `sharp@0.33.5` Apache-2.0, versions exactes, non commités) : notés au journal.

## Captures pour le lead dev (avant = `main`)

| Capture | Avant | Après |
|---|---|---|
| `vitrine-x5-a0` — X5, de face, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-a0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-a0.jpg?raw=true) |
| `vitrine-x5-a1` — X5, trois quarts avant, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-a1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-a1.jpg?raw=true) |
| `vitrine-x5-a2` — X5, de dos, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-a2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-a2.jpg?raw=true) |
| `vitrine-x5-a0-nuit` — X5, de face, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-a0-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-a0-nuit.jpg?raw=true) |
| `vitrine-x5-a1-nuit` — X5, trois quarts avant, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-a1-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-a1-nuit.jpg?raw=true) |
| `vitrine-x5-feux-nuit` — X5, feux de nuit (frein) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-x5-feux-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-x5-feux-nuit.jpg?raw=true) |
| `vitrine-compass-a0` — Compass, de face, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-compass-a0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-compass-a0.jpg?raw=true) |
| `vitrine-compass-a1` — Compass, trois quarts avant, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-compass-a1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-compass-a1.jpg?raw=true) |
| `vitrine-compass-a2` — Compass, de dos, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-compass-a2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-compass-a2.jpg?raw=true) |
| `vitrine-compass-a0-nuit` — Compass, de face, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-compass-a0-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-compass-a0-nuit.jpg?raw=true) |
| `vitrine-compass-a1-nuit` — Compass, trois quarts avant, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-compass-a1-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-compass-a1-nuit.jpg?raw=true) |
| `vitrine-compass-feux-nuit` — Compass, feux de nuit (frein) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-compass-feux-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-compass-feux-nuit.jpg?raw=true) |
| `vitrine-p208-a0` — 208, de face, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-p208-a0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-p208-a0.jpg?raw=true) |
| `vitrine-p208-a1` — 208, trois quarts avant, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-p208-a1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-p208-a1.jpg?raw=true) |
| `vitrine-p208-a2` — 208, de dos, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-p208-a2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-p208-a2.jpg?raw=true) |
| `vitrine-p208-a0-nuit` — 208, de face, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-p208-a0-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-p208-a0-nuit.jpg?raw=true) |
| `vitrine-p208-a1-nuit` — 208, trois quarts avant, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-p208-a1-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-p208-a1-nuit.jpg?raw=true) |
| `vitrine-p208-feux-nuit` — 208, feux de nuit (frein) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/avant-vitrine-p208-feux-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-193/qa/pr/WFS-193/apres-vitrine-p208-feux-nuit.jpg?raw=true) |

Les autres paires (`vitrine-*-0`, `vitrine-*-feux` de jour) sont dans le dossier `qa/pr/WFS-193/`.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv2MVfXZF3cPf1r8qRq
