Agent : Claude, agent dev3d 3 (01UY1Vv5WS)

**Ticket** : WFS-369 — règles : `regles-de-gestion/WFS-369.md` (épic E15, RG-E15-06 : avis de l'agent 3D avant le lead dev)

## Description

- **Demande de l'utilisateur** : il reste des formes géométriques dans les cartes ; sur la Ville, le mobilier urbain (bancs, abribus, grilles, parasols des terrasses…) doit devenir de vrais objets en 3D, réalistes à 99 %.
- **Inventaire (RG-01)** : `qa/pr/WFS-369/inventaire.md` (copié au journal). Treize sortes d'objets étaient en boîtes, cylindres, cônes ou disques. Les poubelles, bornes, kiosques et barrières n'existent pas sur la carte : rien à remplacer ici (WFS-385).
- **Ce qui change** :
  - `src/world/mobilier_ville.{h,cpp}` (nouveau) : modèles construits pièce par pièce aux vraies mesures (sources dans le fichier). Pavés chanfreinés, profils tournés et tubes cintrés, jamais une forme nue.
    - banc public (1,80 m, assise 0,45 m, lattes, flancs en fonte, accoudoirs) ;
    - abribus (poteaux 80 × 80, toit en porte-à-faux, verre trempé encadré, panneau d'affichage double face, banc) ;
    - grille de parc (barreaux à pointes de lance, vide de 10,2 cm, lisses, poteaux à pomme) ;
    - fontaine (bassin mouluré, piédestal, vasque, pomme de pin) ;
    - lampadaire (mât conique de 7,40 m, crosse cintrée, luminaire) ;
    - feu tricolore R11 (caisson, écran de contraste, visières ; les optiques allumées sont redessinées à leur place) ;
    - panneau rond Ø 700 (bord roulé, dos galvanisé, colliers) ;
    - terrasse : table de bistrot, chaises de bistrot, parasol Ø 2,70 m à 8 baleines (lambrequin, baleines, jambes de force, pied lesté), fermé et sanglé la nuit, photophore la nuit, jardinière en terre cuite avec un buis taillé.
  - `src/world/ville.cpp` : les mêmes objets aux mêmes places appellent ces modèles. Ajouts : matériau P_GLACE (verre clair des abribus), 2 couleurs (fonte RAL 6009, affiche). La fonction `cylinder` devenue inutile est retirée.
  - `src/world/commerces.cpp` : la terrasse utilise les modèles, avec une version allégée au-delà de 150 m (même silhouette). La nuit, les parasols sont fermés. **Correction en plus** : sur `main`, des tables et des clients assis étaient posés dans l'emprise d'un abribus. Ils y sont maintenant retirés (`sousAbribus`).
  - `src/world/cartes_banc.cpp` (accroche annoncée au journal) : gros plans `carte-ville-mobilier-*` (cam 4), de jour et de nuit, pris après le décompte.
  - `tests/test_mobilier_ville.cpp` et 1 ligne dans `CMakeLists.txt` : mesures ±1 %, vide de la grille < 11 cm, et sur chaque modèle aucun triangle dégénéré ni inversé.
- **Modèles et licences** : aucun modèle libre CC0 / CC BY de ce mobilier français aux bonnes mesures n'existe sur les sources permises. J'ai appliqué RG-03 : chaque objet est modélisé par le code. Rien n'a été téléchargé, il n'y a aucune licence à ajouter et aucune marque.
- **Comment c'est vérifié** :
  - `tests_cpu.sh` : 91 réussis avant l'ajout du test, puis `test_mobilier_ville` : 0 échec. Banc assise 0,450 m, table Ø 0,700 m, parasol Ø 2,700 m, mât 7,400 m, bassin Ø 8,20 m.
  - Captures avant / après prises avec les mêmes vues : pour l'« avant », `main` a été compilé avec seulement les vues du banc ajoutées.
  - Banc `perf`, Ville **de nuit**, lavapipe :

    | Qualité | Image avant | Image après | Appels de dessin | Triangles |
    |---|---|---|---|---|
    | Basse | 303,8 ms | 302,3 ms | 231 → 238 | 260 k → 375 k |
    | Haute | 529,9 ms | 520,8 ms | 472 → 485 | 542 k → 778 k |

    Pas de baisse d'images par seconde au banc.
- **Taille ajoutée au décor** : 0 Mo sur disque. Tout est du code, préparé au chargement avec le décor de la Ville. Le budget de 200 à 700 Mo concerne WFS-385.
- **Limites et risques** :
  - Les triangles augmentent de 44 % en Basse. Ce n'est pas mesuré sur le téléphone : le banc PC est limité par les pixels.
  - Le HUD reste sur les gros plans : il n'existe pas d'option du banc pour le cacher.
  - Les optiques éteintes des feux et le jet de la fontaine de l'îlot ne changent pas (lumière, E13).
  - Ce ticket ne traite pas les immeubles, le port, les stores et les ouvrages (WFS-370 / 385).

## Captures pour le lead dev et l'agent 3D (20 paires, jour et nuit)

Dossier : [`qa/pr/WFS-369`](https://github.com/wlfaleh/waforspeed_mobile/tree/feature/waforspeed-369/qa/pr/WFS-369)

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `carte-ville-mobilier-abribus-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-abribus-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-abribus-nuit.jpg?raw=true) |
| `carte-ville-mobilier-abribus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-abribus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-abribus.jpg?raw=true) |
| `carte-ville-mobilier-banc-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-banc-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-banc-nuit.jpg?raw=true) |
| `carte-ville-mobilier-banc` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-banc.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-banc.jpg?raw=true) |
| `carte-ville-mobilier-feux-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-feux-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-feux-nuit.jpg?raw=true) |
| `carte-ville-mobilier-feux` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-feux.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-feux.jpg?raw=true) |
| `carte-ville-mobilier-fontaine-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-fontaine-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-fontaine-nuit.jpg?raw=true) |
| `carte-ville-mobilier-fontaine` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-fontaine.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-fontaine.jpg?raw=true) |
| `carte-ville-mobilier-grille-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-grille-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-grille-nuit.jpg?raw=true) |
| `carte-ville-mobilier-grille` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-grille.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-grille.jpg?raw=true) |
| `carte-ville-mobilier-lampadaire-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-lampadaire-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-lampadaire-nuit.jpg?raw=true) |
| `carte-ville-mobilier-lampadaire` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-lampadaire.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-lampadaire.jpg?raw=true) |
| `carte-ville-mobilier-rue-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-rue-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-rue-nuit.jpg?raw=true) |
| `carte-ville-mobilier-rue` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-rue.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-rue.jpg?raw=true) |
| `carte-ville-mobilier-terrasse-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-terrasse-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-terrasse-nuit.jpg?raw=true) |
| `carte-ville-mobilier-terrasse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-carte-ville-mobilier-terrasse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-carte-ville-mobilier-terrasse.jpg?raw=true) |
| `vie-enseigne-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-vie-enseigne-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-vie-enseigne-nuit.jpg?raw=true) |
| `vie-plage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-vie-plage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-vie-plage.jpg?raw=true) |
| `vie-portique` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-vie-portique.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-vie-portique.jpg?raw=true) |
| `vie-terrasse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/avant-vie-terrasse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-369/qa/pr/WFS-369/apres-vie-terrasse.jpg?raw=true) |
