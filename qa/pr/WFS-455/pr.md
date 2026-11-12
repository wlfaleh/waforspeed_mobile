Agent : Claude, agent dev3d (01UY1Vv3N7)

**Ticket** : WFS-455 — règles : `regles-de-gestion/WFS-455.md` (épic E15, ANO-254)

## Description

- **Demande de l'utilisateur** : plus aucun immeuble de la Ville en pavé gris nu vu de haut ou de l'arrière (recette QA du 2026-11-12, ANO-254) : des fenêtres sur les façades arrière et latérales, de vrais pignons, une toiture complète, à toutes les distances, jour et nuit.
- **Ce qui change** :
  - `archi_kit.*` : `facadeSecondaire()` : façade arrière (sur cour) ou latérale d'un bâtiment, mêmes niveaux et mêmes fenêtres d'étage que la rue (une fenêtre par travée et par étage, porte de service sur cour) ; champ `Plan::secondaire` (pas de balcon filant sur cour).
  - `archi_geo.*` : les trois murs aveugles (arrière et côtés) de WFS-161 sont remplacés par trois façades du kit, construites par `facade()` puis tournées (rotation pure, sens des triangles gardé). Baies **peintes** 1 cm devant le mur à tous les niveaux, avec leur pièce de nuit (35 % allumées) ; nouveau mode `Baies::Simples`. La façade sur rue ne change pas.
  - `archi_details.cpp` : lucarnes du comble à la Mansart aussi sur le brisis arrière (de près, niveau 0) ; souches de cheminée et édicule des terrasses visibles à tous les niveaux (avant : seulement à moins de 120 m, d'où les toits plats vides vus de haut).
  - `ville.cpp` : flancs du casino rythmés par des pilastres tous les 4 m et des vitrages (avant : pan beige aveugle de 24 m, pilastre d'angle trop profond).
  - `archi_cuit.h` : `VERSION_BLOC` 1 → 2 (géométrie changée) : un paquet cuit avant cette PR est refusé proprement et le kit construit les bâtiments (RG-06 de WFS-416) jusqu'à la republication des paquets (WFS-464, `@cicd`).
  - `bench_realisme.cpp` (accroche de 4 lignes) : `WFS_REALISME_VUE=<vue>` (essai local d'une seule vue) et 13e vue `ville-arriere` (arrière des immeubles à hauteur d'homme ; +4 captures au banc du matin).
  - `archi_carte.cpp` : journal des triangles des 3 niveaux (preuve RG-04).
- **Comment c'est vérifié** :
  - tests CPU : 114 tests, tous OK (`test_archi_geo` : nouveau contrôle WFS-455 sur les 6 styles, 3 niveaux : vitres sur l'arrière et les deux côtés, façades secondaires dans les règles du kit, sens des triangles, édicule visible à chaque niveau ; `test_batiments_cuits` OK) ;
  - captures avant / après ci-dessous (lavapipe, comme la CI), jour et nuit, Haute et Basse ;
  - **RG-04, performance (banc perf, Ville, qualité Basse)** : triangles dessinés par image 371 825 → 398 131 (+7 %), dont bâtiments 63 135 → 89 339 ; processeur 1,4 → 1,4 ms ; image lavapipe 330,9 → 363,9 ms au 1er passage, 344,7 → 350,7 ms (+1,7 %) au 2e (bruit de la machine ±5 %) ; Haute : 1 219 443 → 1 295 805 triangles. Triangles de la Ville (625 bâtiments, tous) : niveau 0 1 744 402 → 1 998 906, niveau 2 : 302 330. Une 1re version (vrais trous en retrait sur les 4 faces) coûtait +150 % de triangles aux bâtiments : refaite en baies peintes.
  - **Taille du décor** : 0 Mo d'actif ajouté (tout est calculé par le code) ; décor de la Ville toujours sous 700 Mo.
- **Limites et risques connus** :
  - les fenêtres sur cour et sur les côtés sont **peintes** (pas de tableau en retrait) : choix de performance ; de très près (arrière, à hauteur d'homme), elles paraissent plates ;
  - références réelles (RG-05) : aucune photo téléchargée (réseau limité) ; règles reprises du kit (étage 2,7 à 3,2 m, fenêtres alignées par travée, comme les façades sur cour haussmanniennes) ;
  - centre commercial et garage (lieux de `ville.cpp`) : non touchés (hors ANO-254, WFS-385) ;
  - paquets cuits publiés : refusés jusqu'à leur republication (WFS-464) : le téléphone construit alors les bâtiments par le code, comme sans paquet.

## Captures pour le lead dev et l'agent 3D

Dossier : [`qa/pr/WFS-455`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `realisme-ville-large-jour-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/avant-realisme-ville-large-jour-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/apres-realisme-ville-large-jour-haute.jpg?raw=true) |
| `realisme-ville-large-nuit-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/avant-realisme-ville-large-nuit-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/apres-realisme-ville-large-nuit-haute.jpg?raw=true) |
| `realisme-ville-large-jour-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/avant-realisme-ville-large-jour-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/apres-realisme-ville-large-jour-basse.jpg?raw=true) |
| `realisme-ville-large-nuit-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/avant-realisme-ville-large-nuit-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/apres-realisme-ville-large-nuit-basse.jpg?raw=true) |
| `realisme-ville-arriere-jour-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/avant-realisme-ville-arriere-jour-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/apres-realisme-ville-arriere-jour-haute.jpg?raw=true) |
| `realisme-ville-arriere-nuit-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/avant-realisme-ville-arriere-nuit-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/apres-realisme-ville-arriere-nuit-haute.jpg?raw=true) |
| `realisme-ville-arriere-jour-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/avant-realisme-ville-arriere-jour-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/apres-realisme-ville-arriere-jour-basse.jpg?raw=true) |
| `perf-ville-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/avant-perf-ville-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/apres-perf-ville-basse.jpg?raw=true) |
| `perf-ville-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/avant-perf-ville-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-455/qa/pr/WFS-455/apres-perf-ville-haute.jpg?raw=true) |
