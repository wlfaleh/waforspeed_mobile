## Agent : Claude, agent dev3d (01UY1Vv4Xe)

**Ticket** : WFS-571 — règles : `regles-de-gestion/WFS-571.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : ANO-280 (bloquante, confirmée par l'utilisateur le 2026-11-17) : dans la Ville, plus aucune tour en parallélépipède ni cube nu sur les toits ; de vrais volumes (RG-03, RG-04), sans forme géométrique nue.
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - **Tours en tronçons (RG-03)** — `archi_kit.*` (`archi::troncons`, `Plan::surSocle`, `Plan::couvert`) : chaque immeuble moderne (tours de la Ville, ≥ 3 étages, ≥ 8 m de profondeur) est découpé en **socle** (rez-de-chaussée + 1 ou 2 étages), **fût en retrait** de la rue (1,5 à 2,5 m) et/ou **attique** des 1-2 derniers étages (retrait 2 à 3 m, une travée de moins de chaque côté). Trois silhouettes selon la graine : deux tours voisines d'un même côté n'ont jamais la même. Mêmes niveaux, mêmes fenêtres, même hauteur totale ; nu arrière commun. `archi_carte.cpp` (`poser`) pose chaque tronçon comme un bâtiment du kit (cuisson du paquet inchangée dans son principe). Les autres styles ne changent pas.
  - **Toits équipés (RG-04)** — `archi_details.cpp` (`terrasse`) : acrotère et couvertines coupés sous le tronçon du dessus (aucun mur confondu) ; **édicule** complet à tous les niveaux de détail (dalle en débord, couvertine de rive, **lanterneau de désenfumage**, **porte métallique** à huisserie et linteau, **grille de ventilation à lames**) au lieu du « cube » des niveaux 1 et 2 ; **centrale de traitement d'air** (rails sur plots, caisson à cornières et joints, prise d'air à lames, 2 ventilateurs à virole et grille) sur les grands toits des immeubles modernes et industriels ; **garde-corps de toiture** (main courante 1,10 m, lisse, montants) sur les toits des tours et les terrasses des retraits.
  - **Façades (RG-05)** — `archi_geo.cpp` : les nez de dalle des tours sont gardés au niveau de détail 1 (jusqu'à 300 m) : plus de façade lisse vue de l'avenue. Pas de soubassement ni de socle sous le sol pour un tronçon haut.
  - **Paquet (RG-08)** : les bâtiments **sont cuits** dans le paquet `.wfcarte` de la Ville (bloc « batiments ») : `VERSION_BLOC` passe à 3, l'ancien bloc est refusé proprement (kit construit par le code en secours) → **WFS-572 doit republier les paquets** de la Ville.
  - Banc du kit (`archi_banc.cpp`) : la tour moderne en tronçons, 5 nouvelles vues `archi-tour-moderne-*` (toit de près, niveaux 1 et 2, rue de jour et de nuit), `WFS_ARCHI_FILTRE` pour l'essai local.
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) :
  - **Tests CPU : 158/158 verts** (`tools/qa/tests_cpu.sh build`), dont le nouveau `test_tours_ville` (3,9 millions de contrôles : tronçons conformes au kit, empilés, mêmes niveaux et hauteur, retraits croissants, profondeur ≥ 6 m, silhouettes différentes entre voisines, aucun acrotère ni couvertine sous le tronçon du dessus, porte et grille de l'édicule à chaque niveau de détail, centrales et garde-corps présents, sens des triangles) ; `test_archi_geo`, `test_archi_kit`, `test_batiments_cuits`, `test_carte_cuite` verts.
  - Banc `perf` (Ville, lavapipe, 200 images) `main` → branche : processeur 1,5 → 1,6-1,7 ms (Basse), `ArchiCarteSysteme.render` 0,05 → 0,07 ms : **dans la tolérance de ± 1 ms** ; triangles des bâtiments près du joueur 94 749 → 129 030 (scène Basse 456 776 → 491 059, + 7,5 %). Le temps « GPU » de lavapipe (rendu logiciel, très bruité d'un lancement à l'autre) n'est pas celui du téléphone.
  - Kit de la Ville : 625 → 905 bâtiments posés (tronçons) ; triangles niveau 1 : 609 738 → ≈ 790 000, niveau 2 : 302 330 → 334 946.
  - Parcours complet (banc `parcours`) : accueil, choix, course, pause, reprise, fin OK ; « fin → Menu » en écart **aussi sur `main`** (même banc, même résultat) : rouge connu suivi par **WFS-589** (`@cicd`), sans lien avec ce ticket.
  - Captures regardées une à une (ci-dessous) : `voiture-sonde-0` (fût en retrait et garde-corps sur la tour de droite), `realisme-ville-large` jour / nuit (tours à retraits, terrasses, centrales et édicules), banc du kit (tour en tronçons, toit de près et de loin, de nuit).
- **Limites et risques connus** :
  - Hauteur des tours inchangée : le style moderne du kit est borné à 8 étages (WFS-161) alors que le plan de la Ville demande jusqu'à 105 m ; monter cette borne change la ligne d'horizon et la performance : **à proposer au PO** (hors de ce ticket).
  - Façades arrière et latérales : toujours peintes à plat (choix de performance de WFS-455), non vues depuis l'avenue.
  - Les fenêtres du kit de secours restent des vitres sombres en retrait (l'image avec relief des menuiseries n'existe que dans le paquet cuit) : WFS-572 republie.
  - Mesure de performance sur lavapipe seulement : à confirmer par l'utilisateur sur son téléphone (ticket 🟡).
  - Taille du paquet : + ≈ 30 % de triangles de bâtiments au niveau 1 ; le paquet de la Ville (≈ 200 Mo) reste loin des 700 Mo.

## Captures pour le lead dev (18 après)

Dossier : [`qa/pr/WFS-571`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571) — produit par `tools/qa/captures_pr.sh`.

Captures « avant » : `origin/main` = `699b39e965f6c1fd3c8e0b1b5ac34a7f3d313b81` ; cache : 0 banc(s) relu(s), 3 banc(s) rejoué(s) ; main compilé : oui.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `archi-fenetre-moderne-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/avant-archi-fenetre-moderne-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-archi-fenetre-moderne-nuit.jpg?raw=true) |
| `archi-fenetre-moderne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/avant-archi-fenetre-moderne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-archi-fenetre-moderne.jpg?raw=true) |
| `archi-porte-moderne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/avant-archi-porte-moderne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-archi-porte-moderne.jpg?raw=true) |
| `archi-style-moderne-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/avant-archi-style-moderne-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-archi-style-moderne-nuit.jpg?raw=true) |
| `archi-style-moderne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/avant-archi-style-moderne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-archi-style-moderne.jpg?raw=true) |
| `archi-tour-moderne-rue-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-archi-tour-moderne-rue-nuit.jpg?raw=true) |
| `archi-tour-moderne-rue` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-archi-tour-moderne-rue.jpg?raw=true) |
| `archi-tour-moderne-toit-lod1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-archi-tour-moderne-toit-lod1.jpg?raw=true) |
| `archi-tour-moderne-toit-lod2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-archi-tour-moderne-toit-lod2.jpg?raw=true) |
| `archi-tour-moderne-toit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-archi-tour-moderne-toit.jpg?raw=true) |
| `voiture-sonde-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/avant-voiture-sonde-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-voiture-sonde-0.jpg?raw=true) |
| `voiture-sonde-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/avant-voiture-sonde-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-voiture-sonde-1.jpg?raw=true) |
| `voiture-sonde-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/avant-voiture-sonde-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-voiture-sonde-2.jpg?raw=true) |
| `voiture-sonde-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/avant-voiture-sonde-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-voiture-sonde-3.jpg?raw=true) |
| `realisme-ville-large-jour-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/avant-realisme-ville-large-jour-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-realisme-ville-large-jour-basse.jpg?raw=true) |
| `realisme-ville-large-jour-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/avant-realisme-ville-large-jour-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-realisme-ville-large-jour-haute.jpg?raw=true) |
| `realisme-ville-large-nuit-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/avant-realisme-ville-large-nuit-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-realisme-ville-large-nuit-basse.jpg?raw=true) |
| `realisme-ville-large-nuit-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/avant-realisme-ville-large-nuit-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-571/qa/pr/WFS-571/apres-realisme-ville-large-nuit-haute.jpg?raw=true) |
