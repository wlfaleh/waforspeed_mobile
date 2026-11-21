## Agent : Claude, agent dev3d (01UY1Vv5QX)

**Ticket** : WFS-590 — règles : `regles-de-gestion/WFS-590.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : sur les caissons d'enseigne sur mâts de WFS-568, « C'est de la merde. Dit au PO soit de créer un décor logique soit de supprimer carrément ça » (2026-11-20, ANO-317) : on supprime d'abord, puis on pose ce qui existe vraiment dans une rue française (RG-00).
- **Ce qui change** :
  - **RG-01, suppression (1er commit)** : plus aucun caisson sur mâts au bord de la route, en Ville et sur la Corniche ; la collision de leur mât part aussi (`ville_quartiers.cpp`, plus d'obstacle invisible). Code gardé, désactivé par `ville::CAISSONS_SUR_MATS = false` (`ville_quartiers.h`, accroche de 5 lignes annoncée au journal) ; tirages du plan inchangés (aucun autre objet ne bouge).
  - **RG-02, enseignes bandeau** (`enseignes.cpp` : `vie::enseigneBandeau`) : caisson à plat sur la façade, dos au mur (saillie 0,15 m), cadre en profilé d'aluminium de 50 mm chanfreiné, joues, face imprimée en retrait de 20 mm, lettres de ≈ 30 cm. Ville : chaque commerce (`commerces.cpp`) au-dessus de sa devanture et du store (bas à 3,15 m), à la couleur du commerce ; avant : une plaque de 6 cm posée 11 cm devant le mur (forme nue). Corniche (`plage.cpp`) : « CAFÉ DU PORT / DE LA PLAGE » (face bleu marine) et « LOCATION SCOOTERS » (face anthracite) ; avant : des plaques d'aluminium posées devant le mur.
  - **RG-03, enseignes drapeau** (`vie::enseigneDrapeau`) : une par commerce de la Ville, à un bout de la devanture, platine vissée (4 vis), potence en tube Ø 33,7 mm et tirant Ø 24 mm, caisson double face 0,60 × 0,60 × 0,12 m, bas à 3,20 m (au-dessus du store), saillie 0,79 m ; mot court lisible (« CAFÉ », « RESTO », « PAIN »…). **Pharmacie** : croix verte de 0,62 m (`vie::croixPharmacie`), allumée la nuit seulement.
  - **RG-06** : de jour, plus aucune lettre allumée (avant : émission 0,6 en Ville et sur la Corniche) ; la nuit, lettres rétroéclairées, sans halo. Aucun cyan.
  - **Textes de `BOARD_TEXTS`** : « GARAGE 88 » est déjà sur la façade du garage (WFS-568) ; « TURBO CAFÉ », « NUIT BLANCHE », « PNEUS 24/7 » et « WAFORSPEED » n'ont aucun commerce de ce nom dans les cartes : ils ne sont plus affichés nulle part (pas de commerce inventé hors ticket).
  - **Non fait (facultatif)** : sucette, colonne Morris (RG-04) et panneau 4 × 3 m mural (RG-05) : rien posé.
  - **Paquets `.wfcarte`** : **rien de cuit** (seules les villas de la Corniche le sont, `plage::poserVillasKit`) : aucune publication nécessaire.
  - **Bancs** : `carte-ville-enseigne-facade-1|2` (1er commerce, 1re pharmacie), `carte-ville-enseigne-rue`, `carte-plage-enseigne-facade-cafe|scooters`, jour et nuit ; les anciennes vues `carte-*-enseigne-N` gardées pour l'avant / après à l'endroit des caissons.
- **Comment c'est vérifié** : compilation sans nouvel avertissement ; **tests CPU : 156 réussis** (`tools/qa/tests_cpu.sh build`) ; bancs `ville` et `corniche` (lavapipe, comme la CI) : 18 captures, chacune regardée ; **parcours complet** `WFS_BANC=parcours` : 1 écart « fin puis Menu → accueil → écran course », **identique sur `main` 869082d4** (même ligne, même résultat, rejoué avec le binaire de `main`) : il n'est pas causé par ce ticket, signalé au PO. Mesures et sources : `qa/pr/WFS-590/references/README.md` (R581-60, R581-61, statut `a_confirmer`).
- **Limites et risques connus** : **photos réelles de référence (RG-08) non jointes** : Wikimedia Commons est refusé par le réseau de la session (403 du proxy), aucun contournement ; le PO est prévenu, catégories Commons proposées dans `references/README.md`. Lettres imprimées sur la face (pas de lettres découpées en relief). Le store et le coffre de rideau (hors enseignes) restent tels quels. Coût : quelques centaines de triangles par commerce, dans les morceaux déjà construits (aucun appel de dessin de plus, sauf la croix verte : 1 par morceau qui en a une).

## Captures pour le lead dev (18 après)

Dossier : [`qa/pr/WFS-590`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590) — produit par `tools/qa/captures_pr.sh`.

Captures « avant » : `origin/main` = `869082d461f1ea78112273d6cd35722732187dff` ; cache : 0 banc(s) relu(s), 2 banc(s) rejoué(s) ; main compilé : oui.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `carte-plage-enseigne-1-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/avant-carte-plage-enseigne-1-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-plage-enseigne-1-nuit.jpg?raw=true) |
| `carte-plage-enseigne-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/avant-carte-plage-enseigne-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-plage-enseigne-1.jpg?raw=true) |
| `carte-plage-enseigne-2-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/avant-carte-plage-enseigne-2-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-plage-enseigne-2-nuit.jpg?raw=true) |
| `carte-plage-enseigne-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/avant-carte-plage-enseigne-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-plage-enseigne-2.jpg?raw=true) |
| `carte-plage-enseigne-facade-cafe-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-plage-enseigne-facade-cafe-nuit.jpg?raw=true) |
| `carte-plage-enseigne-facade-cafe` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-plage-enseigne-facade-cafe.jpg?raw=true) |
| `carte-plage-enseigne-facade-scooters-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-plage-enseigne-facade-scooters-nuit.jpg?raw=true) |
| `carte-plage-enseigne-facade-scooters` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-plage-enseigne-facade-scooters.jpg?raw=true) |
| `carte-ville-enseigne-1-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/avant-carte-ville-enseigne-1-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-ville-enseigne-1-nuit.jpg?raw=true) |
| `carte-ville-enseigne-1-route-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/avant-carte-ville-enseigne-1-route-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-ville-enseigne-1-route-nuit.jpg?raw=true) |
| `carte-ville-enseigne-1-route` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/avant-carte-ville-enseigne-1-route.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-ville-enseigne-1-route.jpg?raw=true) |
| `carte-ville-enseigne-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/avant-carte-ville-enseigne-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-ville-enseigne-1.jpg?raw=true) |
| `carte-ville-enseigne-facade-1-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-ville-enseigne-facade-1-nuit.jpg?raw=true) |
| `carte-ville-enseigne-facade-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-ville-enseigne-facade-1.jpg?raw=true) |
| `carte-ville-enseigne-facade-2-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-ville-enseigne-facade-2-nuit.jpg?raw=true) |
| `carte-ville-enseigne-facade-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-ville-enseigne-facade-2.jpg?raw=true) |
| `carte-ville-enseigne-rue-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-ville-enseigne-rue-nuit.jpg?raw=true) |
| `carte-ville-enseigne-rue` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-590/qa/pr/WFS-590/apres-carte-ville-enseigne-rue.jpg?raw=true) |
