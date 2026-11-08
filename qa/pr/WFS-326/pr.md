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
  - `ville_quartiers.cpp` (accroche E2, 6 lignes, annoncée au journal) : un commerce ou un panneau n'est gardé que devant un immeuble encore posé, du même côté et sur sa longueur. 11 commerces de plus sont retirés (108 → 119) ; les panneaux ne changent pas.
  - `cartes_banc.cpp` : les vues `carte-ville-virage-N` sont aussi prises de nuit.
  - Le jour, ces mêmes commerces isolés (stores et terrasses sur la pelouse) disparaissent aussi ; rien d'autre ne change.
- **Comment c'est vérifié** :
  - tests CPU verts, parcours complet 0 écart ;
  - banc `ville` complet comparé avant / après : seules changent les vues des virages, `ao-3-d` et `chien`, où ces commerces étaient visibles ;
  - Corniche et Campagne de nuit regardées : pas de commerce de ce type, aucun rectangle dans le vide.
- **Limites et risques connus** :
  - Les fenêtres et vitrines **dans** les immeubles restent très lumineuses la nuit : la pièce derrière la vitre est à 31,8 cd/m², pour une exposition de nuit vers EV 4. C'est un autre sujet (exposition, WFS-230), non traité ici.
  - Le banc est en rendu logiciel (lavapipe) : à confirmer sur l'APK de `main`, avenue du Belvédère de nuit, en vue Arrière.
  - Fusion par le lead dev ; le ticket passera ensuite en 🟡 À valider (utilisateur).

## Captures pour le lead dev (5 après, jour et nuit)

Dossier : [`qa/pr/WFS-326`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326)

| Capture | Ce qu'on voit | Avant (main) | Après (branche) |
|---|---|---|---|
| `virage-1-nuit` | Rue du Belvédère, 30 m avant le virage serré, nuit : à droite, la vitrine allumée et le store rouge d'un commerce sans immeuble flottaient dans le vide ; après : disparus, il ne reste que les fenêtres des immeubles | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/avant-carte-ville-virage-1-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/apres-carte-ville-virage-1-nuit.jpg?raw=true) |
| `virage-2-nuit` | Rue des Glycines, nuit : à gauche, deux grands rectangles lumineux sans façade ; après : disparus | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/avant-carte-ville-virage-2-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/apres-carte-ville-virage-2-nuit.jpg?raw=true) |
| `virage-3-nuit` | Rue des Mimosas, nuit : témoin, aucun commerce isolé ici, image identique | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/avant-carte-ville-virage-3-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/apres-carte-ville-virage-3-nuit.jpg?raw=true) |
| `virage-1` | même lieu de jour : stores rayés, terrasse et devanture posés sur le trottoir sans immeuble ; après : trottoir et pelouse dégagés, le reste identique | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/avant-carte-ville-virage-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/apres-carte-ville-virage-1.jpg?raw=true) |
| `virage-2` | même lieu de jour : commerces verts et stores rouges sur la pelouse, sans immeuble ; après : pelouse dégagée | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/avant-carte-ville-virage-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-326/qa/pr/WFS-326/apres-carte-ville-virage-2.jpg?raw=true) |
