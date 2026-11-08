Agent : Claude, agent dev Y

**Ticket** : WFS-326 (ANO-175) — règles : `regles-de-gestion/WFS-326.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : la nuit en Ville, plus aucun grand « losange » lumineux bordé de rouge qui flotte à côté des immeubles (captures `qa/retours/2026-11-08-apk916-5.jpg` et `-2.jpg`).
- **Cause (RG-01)** :
  - Ce n'étaient pas les fenêtres du kit d'architecture : les vitres sont dans leurs façades. Contrôle par le calcul : 518 424 sommets de vitre, tous styles, tailles et niveaux de détail, 0 hors façade.
  - Avant un virage serré, `ville::adapter` retire l'immeuble qui déborde la limite du virage (R + 6,5 m + profondeur). Mais il ne retirait son **commerce** que si la limite passait sous R + 10,5 m.
  - Entre les deux, la vitrine allumée (`commerces.cpp`), le store rayé rouge et blanc, l'enseigne et la terrasse restaient seuls, dans le vide.
  - La nuit, cette vitrine est un grand rectangle blanc saturé, bordé du rouge du store : les « losanges » de l'utilisateur.
  - Essai A/B : en coupant les vitres du kit ou les fenêtres des abords, les rectangles restent. En retirant ces commerces, ils disparaissent.
- **Ce qui change** :
  - `ville_quartiers.cpp` (accroche E2, 10 lignes ajoutées et 3 modifiées dans `adapter()`, annoncée au journal) : un commerce ou un panneau n'est gardé que devant un immeuble encore posé, du même côté et **sur toute sa longueur** : vitrine de `seg + décalage` sur `len`, panneau sur 6 m. La portée de la recherche est la longueur du plus long immeuble du plan (`lmax`, calculée), au lieu de 60 m en dur. Commerces retirés : 108 → 122 ; panneaux : 3 → 4.
  - `tests/test_ville_lieux.cpp` : nouveau contrôle « chaque commerce et chaque panneau gardé a un immeuble derrière lui, du même côté, sur toute sa longueur ». Sans le correctif : 15 échecs ; avec : vert.
  - `cartes_banc.cpp` : les vues `carte-ville-virage-N` sont aussi prises de nuit ; 2 vues aux lieux de l'utilisateur (`carte-ville-apk916-5`, vue Arrière ; `carte-ville-apk916-2`, vue Proche) ; `WFS_QUALITE_BASSE=1` est lu par le banc Ville.
  - Le jour, ces mêmes commerces isolés (stores et terrasses sur la pelouse) disparaissent aussi ; rien d'autre ne change.
- **Comment c'est vérifié** :
  - 73 tests CPU verts (après synchronisation avec `main`), parcours complet 0 écart ;
  - banc `ville` complet comparé avant / après : seules changent les vues des virages, `ao-3-d` et `chien`, où ces commerces étaient visibles ;
  - Corniche et Campagne de nuit regardées : pas de commerce de ce type, aucun rectangle dans le vide.
- **Limites et risques connus** :
  - Les fenêtres et vitrines **dans** les immeubles restent très lumineuses la nuit : la pièce derrière la vitre est à 31,8 cd/m², pour une exposition de nuit vers EV 4. C'est un autre sujet (exposition, WFS-230), non traité ici.
  - **Aux deux lieux exacts de l'utilisateur, le banc ne montre aucun rectangle flottant, ni avant ni après**, en Haute comme en Basse. Les fenêtres du rez-de-chaussée y sont dans leurs encadrements, mais très lumineuses. Si le mur reste noir sur le téléphone (rendu à 60 %, en mouvement), elles peuvent sembler flotter. La cause corrigée ici, les vitrines orphelines, est prouvée ailleurs : virages 1 et 2. À confirmer par l'utilisateur sur l'APK de `main`.
  - Le banc est en rendu logiciel (lavapipe).
  - Fusion par le lead dev ; le ticket passera ensuite en 🟡 À valider (utilisateur).

## Captures pour le lead dev (9 paires, dont les 2 lieux de l'utilisateur en Haute et en Basse)

Dossier : [`qa/pr/WFS-326`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326)

| Capture | Ce qu'on voit | Avant (main) | Après (branche) |
|---|---|---|---|
| `apk916-5-nuit` | **lieu de `apk916-5`** : avenue du Belvédère, 100 m avant le virage serré à gauche, vue Arrière, nuit, Haute. Avant et après identiques (0 pixel) : les fenêtres du rez-de-chaussée sont dans leurs encadrements, dans la façade, rien ne flotte | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/avant-carte-ville-apk916-5-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/apres-carte-ville-apk916-5-nuit.jpg?raw=true) |
| `apk916-5-nuit-basse` | même lieu en **Basse** (qualité du téléphone) : identique avant / après (0 pixel), rien ne flotte | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/avant-carte-ville-apk916-5-nuit-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/apres-carte-ville-apk916-5-nuit-basse.jpg?raw=true) |
| `apk916-2-nuit` | **lieu de `apk916-2`** : avenue des Mimosas, 20 m avant « tourner à gauche », vue Proche, nuit, Haute. Seul un commerce lointain change (100 pixels au fond) ; à gauche, les vitrines lumineuses sont dans leurs façades | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/avant-carte-ville-apk916-2-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/apres-carte-ville-apk916-2-nuit.jpg?raw=true) |
| `apk916-2-nuit-basse` | même lieu en Basse : 92 pixels au fond, rien ne flotte | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/avant-carte-ville-apk916-2-nuit-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/apres-carte-ville-apk916-2-nuit-basse.jpg?raw=true) |
| `virage-1-nuit` | rue du Belvédère, 30 m avant le virage serré à droite, nuit : à droite, la vitrine allumée et le store rouge d'un commerce sans immeuble flottaient dans le vide ; après : disparus | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/avant-carte-ville-virage-1-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/apres-carte-ville-virage-1-nuit.jpg?raw=true) |
| `virage-2-nuit` | rue des Glycines, nuit : à gauche, deux grands rectangles lumineux sans façade ; après : disparus | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/avant-carte-ville-virage-2-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/apres-carte-ville-virage-2-nuit.jpg?raw=true) |
| `virage-3-nuit` | rue des Mimosas, nuit : témoin, aucun commerce isolé, image identique | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/avant-carte-ville-virage-3-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/apres-carte-ville-virage-3-nuit.jpg?raw=true) |
| `virage-1` | jour : stores rayés, terrasse et parasols sur la pelouse sans immeuble ; après : pelouse dégagée, le reste identique | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/avant-carte-ville-virage-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/apres-carte-ville-virage-1.jpg?raw=true) |
| `virage-2` | jour : commerces verts et stores rouges sur la pelouse sans immeuble ; après : pelouse dégagée | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/avant-carte-ville-virage-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/apres-carte-ville-virage-2.jpg?raw=true) |
