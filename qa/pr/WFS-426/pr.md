## Agent : Claude, agent cartes (01UY1Vv2HrQF)

**Ticket** : WFS-426 — règles : `regles-de-gestion/WFS-426.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : « go Corniche » (ticket n° 8 de l'ordre du 2026-11-11) : la Corniche actuelle cuite en paquet `.wfcarte`, publiée et téléchargeable depuis le choix du terrain, comme la Ville, avec son décor (bâtiments compris) préparé à l'avance.
- **Ce qui change** :
  - `src/world/plage.cpp` + nouveau `src/world/plage_villas.h` : la pose des villas du kit (WFS-161) est sortie dans `poserVillas()`, et `plage::poserVillasKit(track)` la rejoue **sans carte graphique** (mêmes tirages, mêmes filtres que l'init : même signature des bâtiments) ; en cuisson, aucun lampadaire de nuit ni objet de choc n'est déclaré (`cuisson_`) : la partie en cours n'est pas touchée. Le jeu, lui, pose les villas exactement comme avant.
  - `src/world/cartes_v2/paquet_carte.{h,cpp}` : `CARTES_CUITES = {ville, plage}` ; le bloc `batiments` (WFS-416) est cuit aussi pour la Corniche ; `WFS_BANC=cuire` cuit la Ville **et** la Corniche (3 formats chacune, deux cuissons comparées à l'octet).
  - `.github/workflows/paquets.yml` (ajout de la Corniche seulement, RG-02, relu par l'agent expert ci cd : OK) : 6 paquets attendus, les 6 noms exacts vérifiés ; `tools/qa/publier_paquets.sh` : `--nom plage=Corniche` dans `cartes.json`.
  - `src/world/cartes_v2/batiments_banc.cpp` : le banc « batiments » accepte `WFS_CARTE=plage` (captures `batiments-plage-*`) ; sans variable, banc de la Ville inchangé.
  - `src/ui/ecrans/paquets.cpp` (banc seulement) : débit du téléchargement simulé de la Corniche réglé sur la taille de son paquet (≈ 55 Mo au lieu de quelques Mo).
  - Rien à changer au choix du terrain : « Télécharger / progression / Supprimer » de WFS-307 marche pour la Corniche (captures).
- **Comment c'est vérifié** :
  - Tests CPU : 103 / 103 (`test_audio_chocs` : seuil de temps dépassé une fois sous charge, vert 3 fois sur 3 ensuite, sans lien).
  - `WFS_BANC=cuire` : 0 écart, 25 s ; **tailles (RG-04, 700 Mo max)** : `plage_astc` 54,6 Mo, `plage_etc2` 38,2 Mo, `plage_rgba8` 59,2 Mo (dont bâtiments 32,9 Mo : 172 villas, 44 morceaux) ; Ville inchangée (196,6 / 180,1 / 201,2 Mo).
  - `WFS_CARTE=plage WFS_BANC=batiments` : **0 écart** ; avec le paquet, **0 construction de bâtiment en course** à 50 et 160 km/h (RG-06) ; système des bâtiments à 160 km/h de nuit, Basse : paquet 0,019 ms par image (pire 0,25 ms) contre kit de secours 0,044 ms (pire 3,34 ms, 26 constructions) ; chargement +165 ms ; bloc refusé → kit de secours sans plantage (RG-03).
  - `WFS_BANC=paquets` : **0 écart** : choix du terrain → Corniche « Télécharger — 55 Mo » → progression pendant une course sur la Ville → « Corniche prête » → course sur la Corniche lue dans son paquet → « Supprimer » → course suivante en secours (carte du code) (RG-08).
  - `cartes.json` construit en local sur les 6 paquets (Ville, Corniche ×3), `test_cartes_json.py` vert.
- **Limites et risques connus** :
  - Nom des fichiers : l'identifiant de la Corniche dans le code est `plage` : les paquets s'appellent `plage_<format>.wfcarte` (RG-02 écrivait `corniche_*`) ; renommer demanderait une table de noms dans le format (WFS-301) : signalé au PO.
  - Comme la Ville (WFS-416) : avec le paquet, menuiseries et volets en 3D jusqu'à 20 m même en Basse (captures « façade de près ») ; au-delà, même image. Aucune forme nue ajoutée (villas du kit déjà en place).
  - **Mesure sur le S21 FE (RG-05, ≤ 28 ms)** : impossible ici (banc PC en rendu logiciel) ; le coût mesuré du système des bâtiments baisse avec le paquet. À confirmer par l'utilisateur sur l'APK : 🟡 après fusion.
  - **Publication** : la fusion ne publie rien ; il faut lancer « Paquets de cartes » (paquets.yml) à la main sur `main` après la fusion (avec l'accord de l'utilisateur).

## Captures pour le lead dev (22 après)

Dossier : [`qa/pr/WFS-426`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `batiments-plage-bloc-refuse` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-batiments-plage-bloc-refuse.jpg?raw=true) |
| `batiments-plage-paquet-160kmh-suite-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-batiments-plage-paquet-160kmh-suite-1.jpg?raw=true) |
| `batiments-plage-paquet-160kmh-suite-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-batiments-plage-paquet-160kmh-suite-2.jpg?raw=true) |
| `batiments-plage-paquet-160kmh-suite-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-batiments-plage-paquet-160kmh-suite-3.jpg?raw=true) |
| `batiments-plage-paquet-160kmh` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-batiments-plage-paquet-160kmh.jpg?raw=true) |
| `batiments-plage-paquet-facade-pres-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-batiments-plage-paquet-facade-pres-nuit.jpg?raw=true) |
| `batiments-plage-paquet-facade-pres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-batiments-plage-paquet-facade-pres.jpg?raw=true) |
| `batiments-plage-paquet-rue-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-batiments-plage-paquet-rue-jour.jpg?raw=true) |
| `batiments-plage-paquet-rue-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-batiments-plage-paquet-rue-nuit.jpg?raw=true) |
| `batiments-plage-secours-facade-pres-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-batiments-plage-secours-facade-pres-nuit.jpg?raw=true) |
| `batiments-plage-secours-facade-pres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-batiments-plage-secours-facade-pres.jpg?raw=true) |
| `batiments-plage-secours-rue-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-batiments-plage-secours-rue-jour.jpg?raw=true) |
| `batiments-plage-secours-rue-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-batiments-plage-secours-rue-nuit.jpg?raw=true) |
| `paquets-course-corniche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/avant-paquets-course-corniche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-paquets-course-corniche.jpg?raw=true) |
| `paquets-menu-prete` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/avant-paquets-menu-prete.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-paquets-menu-prete.jpg?raw=true) |
| `paquets-terrain-apres-suppression` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/avant-paquets-terrain-apres-suppression.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-paquets-terrain-apres-suppression.jpg?raw=true) |
| `paquets-terrain-installee` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/avant-paquets-terrain-installee.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-paquets-terrain-installee.jpg?raw=true) |
| `paquets-terrain-mobile` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/avant-paquets-terrain-mobile.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-paquets-terrain-mobile.jpg?raw=true) |
| `paquets-terrain-progression` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/avant-paquets-terrain-progression.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-paquets-terrain-progression.jpg?raw=true) |
| `paquets-terrain-sans-connexion` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/avant-paquets-terrain-sans-connexion.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-paquets-terrain-sans-connexion.jpg?raw=true) |
| `paquets-terrain-supprimer` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/avant-paquets-terrain-supprimer.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-paquets-terrain-supprimer.jpg?raw=true) |
| `paquets-terrain-telecharger` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/avant-paquets-terrain-telecharger.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-426/qa/pr/WFS-426/apres-paquets-terrain-telecharger.jpg?raw=true) |
