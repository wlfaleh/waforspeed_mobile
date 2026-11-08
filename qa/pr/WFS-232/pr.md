Agent : arwa-3 (Claude, session 01Us6uCt)

**Ticket** : WFS-232 — règles : `regles-de-gestion/WFS-232.md`

## Description

- **Demande de l'utilisateur** (2026-11-07 12:51 UTC, « il n'y a pas de motos dans la rue ») : voir en Ville et sur la Corniche, de jour comme de nuit, de vrais scooters et de vraies motos dans la circulation, avec un pilote casqué, qui se penchent dans les virages et qui ont leurs feux la nuit.
- **Ce qui change** :
  - 3 deux-roues réels CC BY 4.0 (`assets/cars/deux_roues/`) : scooter (OLA S1 Pro), **routière Honda CB500X 2019** (remplace la CB750 de la 1re version : roue avant braquée de 20° dans ce modèle, selle à +9 %), sportive (Kawasaki ZX-6R) ; chacun en 3 fichiers : corps, roue avant, roue arrière (`tools/convert_car.mjs` : `WFS_ROUES_2R`, `WFS_FEU`, `WFS_PHARE`, `WFS_EXCLURE_NOEUD`, `WFS_ALLEGER`, `WFS_TEXTURE_MAX`, `WFS_PIECE`), niveaux de détail ; licences et SHA-256 au journal.
  - Trafic (`src/game/traffic.*`) : quota par carte (Ville et Corniche 12,5 %, Campagne 5 %, Grand Prix 0) à nombre de véhicules égal ; côté droit de la voie ; inclinaison φ = atan(v²/(gR)) lissée (roulis ≤ 60°/s) ; **ralentissement avant les courbes** à l'inclinaison de confort de 25° (75 % des inclinaisons de tous les jours, étude BASt) par un freinage doux de 4 m/s² ; **accident** : chute sur le flanc (pendule inversé, couché à 77° en ≈ 0,4 s), glissade, peu de rotation ; sélection des véhicules dessinés partagée avec les tests (`Traffic::dessines`).
  - Rendu (`src/game/traffic_system.cpp`, `src/game/deux_roues.*`) : **roues qui tournent** à v/r autour de leur axe ; **un pilote sur chaque deux-roues dessiné**, à toute distance (16 places ≥ 14 véhicules dessinés), buste penché selon la moto (sportive ≈ 30°), casque porté par l'os de la tête ; après une chute, pilote couché au sol à côté ; **feu arrière et phare des 3 modèles** ; roues dessinées sans les feux d'une voiture (corrige les taches blanches de nuit) ; modèle ou pilote manquant → aucun deux-roues dans le trafic.
  - **Sans marque** (2e revue) :
    - sportive : autocollants Kawasaki / Ninja / ZX-6R retirés de la planche d'autocollants (le compteur, l'écran et le commodo restent) ;
    - scooter : logo « ola » repeint, sur la couleur et sur le métal / rugosité ;
    - images dans `tools/deux_roues/`, posées par l'option `WFS_TEXTURES`, qui accepte maintenant `mr=` ;
    - licences corrigées.
  - **3e revue** :
    - routière : les lettres « 500X » des deux flancs sont retirées (pièces du modèle supprimées à la conversion, flanc refermé : vérifié en 2340 × 1080, vues en bas de page) ; 32 matériaux, 48,6 k triangles ;
    - sportive : pixels transparents de la planche d'autocollants remis à RVB 0, et mêmes zones repeintes dans la texture métal / rugosité (`sportive-autocollants-metal-rugosite-sans-marque.png`, option `mr=`) : plus aucune lettre de marque cachée dans le fichier ; rendu identique au pixel près (banc `motos`, 48 images comparées avant / après ce changement) ;
    - accident : image `choc-2` prise 0,13 s après le choc (moto déjà inclinée à 27°, pas encore couchée) et commentaire du banc corrigé ;
    - test du placement du pilote éjecté (`lateralPiloteEjecte` : jamais à moins de 1 m du bord, au plus 6,50 m de l'axe) ;
    - version de draco notée (`draco3dgltf@1.5.9`) dans le script de conversion ; commentaire périmé de `deux_roues.cpp` mis à jour.
  - **Pilote après la chute** (2e revue) : couché en travers de la chaussée, 1,8 m derrière le deux-roues, du côté opposé à la chute, et jamais à moins de 1 m du bord de la chaussée.
  - Conversion rejouable : `tools/convertir_deux_roues.sh` (toutes les commandes et options) et `tools/deux_roues/dedraco.mjs`. Les fichiers déjà livrés sont reproduits au bit près.
  - `deux_roues::preparer` : si un personnage échoue, ceux déjà chargés sont rendus (plus de fuite).
  - Banc `motos` (accroche d'une ligne dans `desktop_main.cpp`, aussi à la fin du banc « trafic vivant ») : trafic normal, ligne droite, de près, courbe à la vitesse du trafic (4 images), phare vu de face la nuit, accident (3 images), **chaque modèle de côté, de près et de jour**. Chaque vue existe aussi sans deux-roues.
- **Comment c'est vérifié** :
  - 65 tests CPU réussis (branche synchronisée avec `main` d2d33c87, commit 89ba3a5e) ; `test_trace` : parts 11,2 / 12,4 / 4,7 / 0 % ; **mêmes véhicules aux mêmes places sans deux-roues** (5 graines × 4 cartes) ; inclinaison ≤ 25,0° en 2 min de trafic, saut ≤ 1°/pas ; choc : couché en 0,37 s puis immobile ; 2 × 14 400 images : chaque deux-roues dessiné a son pilote (jusqu'à 300 m).
  - Banc `motos` : 0 écart entre deux-roues dessinés et pilotes dessinés (toutes les vues).
  - Parcours accueil → course → fin → retour : **0 écart** avant et après (vue normale et vue Cockpit).
  - Coût (mesuré au 2e tour ; le 3e tour retire seulement des triangles de la routière) (banc `perf`, Ville, 150 images, lavapipe, `main` 5264639 contre la branche) : Haute 1025 → 988 appels de dessin, processeur 3,0 → 2,7 ms (pire 10,4 → 4,4) ; Basse 348 → 348 appels, 2,4 → 2,4 ms (pire 5,3 → 4,0).
  - Hauteur de selle mesurée sur les modèles (point le plus bas du dessus de la selle) : routière 839 mm (fiche 830 à 834 : +0,6 % de 834), sportive 815 mm (830 : −1,8 %), scooter 772 mm (792 : −2,5 %). Longueurs et empattements à ±1 %. L'écart des selles vient des proportions des modèles : seule une échelle uniforme est permise, une échelle par axe rendrait les roues ovales.
- **Limites et risques connus** :
  - **Décision du PO** (commit 4267247, `regles-de-gestion/WFS-232.md`, « Décision du PO », 2026-11-08 03:22 UTC) :
    - écart de selle accepté pour le scooter (−2,5 %) et la sportive (−1,8 %) ; correction à tenter dans WFS-280 (RG-05) ; écart noté dans `deux_roues.json` (`ecart_accepte`) ;
    - ralentissement des deux-roues avant les courbes (25°) accepté, comme un écart voulu à RG-03 ;
    - 7 pilotes sur 10 hommes : gardé.
  - La borne de 45° n'est plus atteinte dans le trafic (les deux-roues ralentissent avant les courbes).
  - Le chargement écrit « feu non trouvé : *-roue-* » pour les 6 roues (journal du moteur, sans effet : les roues n'ont pas de feux).
  - `trafic-motos-virage-plage-nuit` : la Golf claire et la bande sombre du ciel sont aussi sur la vue « sans deux-roues », donc hors de ce ticket.
  - Poids : 11,4 Mo pour les 3 deux-roues (corps, roues, niveaux de détail), contre 11,9 Mo au 2e tour.

## Captures pour le lead dev (39 vues après, avec leur « avant » ou leur vue « sans deux-roues »)

Dossier : [`qa/pr/WFS-232`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `trafic-motos-choc-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-choc-1.jpg?raw=true) |
| `trafic-motos-choc-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-choc-2.jpg?raw=true) |
| `trafic-motos-choc-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-choc-3.jpg?raw=true) |
| `trafic-motos-cote-routiere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-cote-routiere.jpg?raw=true) |
| `trafic-motos-cote-scooter` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-cote-scooter.jpg?raw=true) |
| `trafic-motos-cote-sportive` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-cote-sportive.jpg?raw=true) |
| `trafic-motos-normal-plage-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-normal-plage-jour.jpg?raw=true) |
| `trafic-motos-normal-plage-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-normal-plage-nuit.jpg?raw=true) |
| `trafic-motos-normal-ville-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-normal-ville-jour.jpg?raw=true) |
| `trafic-motos-normal-ville-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-normal-ville-nuit.jpg?raw=true) |
| `trafic-motos-phare-plage-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-phare-plage-nuit.jpg?raw=true) |
| `trafic-motos-phare-ville-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-phare-ville-nuit.jpg?raw=true) |
| `trafic-motos-plage-jour` | ![sans deux-roues](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-plage-jour-sans.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-plage-jour.jpg?raw=true) |
| `trafic-motos-plage-nuit` | ![sans deux-roues](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-plage-nuit-sans.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-plage-nuit.jpg?raw=true) |
| `trafic-motos-pres-plage-jour` | ![sans deux-roues](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-pres-plage-jour-sans.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-pres-plage-jour.jpg?raw=true) |
| `trafic-motos-pres-plage-nuit` | ![sans deux-roues](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-pres-plage-nuit-sans.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-pres-plage-nuit.jpg?raw=true) |
| `trafic-motos-pres-ville-jour` | ![sans deux-roues](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-pres-ville-jour-sans.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-pres-ville-jour.jpg?raw=true) |
| `trafic-motos-pres-ville-nuit` | ![sans deux-roues](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-pres-ville-nuit-sans.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-pres-ville-nuit.jpg?raw=true) |
| `trafic-motos-ville-jour` | ![sans deux-roues](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-ville-jour-sans.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-ville-jour.jpg?raw=true) |
| `trafic-motos-ville-nuit` | ![sans deux-roues](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-ville-nuit-sans.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-ville-nuit.jpg?raw=true) |
| `trafic-motos-virage-plage-jour-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-plage-jour-1.jpg?raw=true) |
| `trafic-motos-virage-plage-jour-2` | ![sans deux-roues](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-plage-jour-sans.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-plage-jour-2.jpg?raw=true) |
| `trafic-motos-virage-plage-jour-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-plage-jour-3.jpg?raw=true) |
| `trafic-motos-virage-plage-jour-4` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-plage-jour-4.jpg?raw=true) |
| `trafic-motos-virage-plage-nuit-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-plage-nuit-1.jpg?raw=true) |
| `trafic-motos-virage-plage-nuit-2` | ![sans deux-roues](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-plage-nuit-sans.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-plage-nuit-2.jpg?raw=true) |
| `trafic-motos-virage-plage-nuit-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-plage-nuit-3.jpg?raw=true) |
| `trafic-motos-virage-plage-nuit-4` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-plage-nuit-4.jpg?raw=true) |
| `trafic-motos-virage-ville-jour-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-ville-jour-1.jpg?raw=true) |
| `trafic-motos-virage-ville-jour-2` | ![sans deux-roues](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-ville-jour-sans.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-ville-jour-2.jpg?raw=true) |
| `trafic-motos-virage-ville-jour-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-ville-jour-3.jpg?raw=true) |
| `trafic-motos-virage-ville-jour-4` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-ville-jour-4.jpg?raw=true) |
| `trafic-motos-virage-ville-nuit-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-ville-nuit-1.jpg?raw=true) |
| `trafic-motos-virage-ville-nuit-2` | ![sans deux-roues](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-ville-nuit-sans.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-ville-nuit-2.jpg?raw=true) |
| `trafic-motos-virage-ville-nuit-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-ville-nuit-3.jpg?raw=true) |
| `trafic-motos-virage-ville-nuit-4` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-trafic-motos-virage-ville-nuit-4.jpg?raw=true) |
| `parcours-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/avant-parcours-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-parcours-course.jpg?raw=true) |
| `parcours-fin` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/avant-parcours-fin.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-parcours-fin.jpg?raw=true) |
| `course-cam0-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/avant-course-cam0-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/apres-course-cam0-0.jpg?raw=true) |

> Colonne « Avant » des vues `trafic-motos-*` : le banc `motos` est nouveau (absent de `main`) ; à sa place, la **même vue au même instant, deux-roues retirés** (captures `…-sans` du même banc ; pour les courbes, à côté de l'image n° 2). `course-cam0-0` et `parcours-*` : vrai « avant » de `main` (passage du 2026-11-08 02:30, code de ces vues inchangé depuis).

### Agrandissements et vues de côté en 2340 × 1080 (résolution du téléphone)

![zoom-cote-routiere](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/zoom-cote-routiere.jpg?raw=true)
![zoom-hd2340-reservoir-routiere](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/zoom-hd2340-reservoir-routiere.jpg?raw=true)
![hd2340-cote-routiere](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/hd2340-cote-routiere.jpg?raw=true)
![hd2340-cote-sportive](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/hd2340-cote-sportive.jpg?raw=true)
![hd2340-cote-scooter](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/hd2340-cote-scooter.jpg?raw=true)
![zoom-cote-sportive](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/zoom-cote-sportive.jpg?raw=true)
![zoom-cote-scooter](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/zoom-cote-scooter.jpg?raw=true)
![zoom-choc-2](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/zoom-choc-2.jpg?raw=true)
![zoom-choc-3](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/zoom-choc-3.jpg?raw=true)
![zoom-pres-ville-jour](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/zoom-pres-ville-jour.jpg?raw=true)
![zoom-pres-ville-nuit](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/zoom-pres-ville-nuit.jpg?raw=true)
![zoom-pres-plage-nuit](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/zoom-pres-plage-nuit.jpg?raw=true)
![zoom-virage-ville-jour-2](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/zoom-virage-ville-jour-2.jpg?raw=true)
![zoom-phare-plage-nuit](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-232/qa/pr/WFS-232/zoom-phare-plage-nuit.jpg?raw=true)
