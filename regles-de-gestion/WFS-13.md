# WFS-13 — Les 20 voitures et 16 peintures
Épic : E5 · Priorité : 13 · Dépend de : WFS-11

## Besoin (qui, quoi, pourquoi)
Le garage propose les 20 vraies voitures du jeu web (4 déjà converties) avec leurs 16 peintures, leurs performances
et leurs crédits de licence.

## Règles de gestion
- RG-01 : 20 voitures (`CARS`) : golf, p208, abarth, tcross, compass, s3, x5, giulia, m4, p911, a45, model3, corsa, gt86, elantra, v60, rrsport, mustang, gallardo, sf90, avec fiche (`ch`, `s100`, `vmax`, `kg`), dimensions `d` et `grip`.
- RG-02 : performances tirées de la fiche (`carPerf`) : `top = vmax/260`, `a0 = 27.8/s100 × 1.45`, `handling = 0.75 + grip×0.05`, `mass = kg/1450`, `nitroMult 1.3`, `nitroGain 0.12`, `nitroBurn 0.26`.
- RG-03 : 16 peintures (`PAINTS`) : blanc, noir, gris, rouge, bleu, orange, jaune, vert, blanc nacré, bleu nuit, vert anglais, bordeaux, sable, violet, turquoise, jaune taxi (mêmes `hex`).
- RG-04 : conversion avec `tools/convert_car.mjs` ; matériau de peinture par motif (`CAR_MODELS[id].paint`), `rotateY: 180` quand la voiture arrive de face, feux arrière par `tail`.
- RG-05 : version allégée `-lod` pour le trafic ; modèle joueur ≤ 120 000 triangles ; taille de l'APK suivie au journal (objectif : par voiture bien en dessous de 10 Mo).
- RG-06 : licence de chaque modèle copiée dans `assets/licences/`, crédit affiché au garage (CC BY, SA, NC : le jeu reste gratuit).
- RG-07 : son moteur associé par voiture (`ENGINE_SET` : v8, sport, citadine, thermique, gros, électrique).

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/cars/catalog.js` (`CARS`, `PAINTS`, `CAR_MODELS`, `carPerf`, `carStats`) ; `js/cars/models.js` (repérage roues, peinture, feux) ; `js/audio/sfx.js` → `ENGINE_SET`.
- Modèles : `models/<voiture>/<voiture>.json` et `<voiture>-lod.json` + `license.txt` (liste dans `models/README.md`) ; **aucun nouveau modèle à chercher**.
- Retours connus : roues détectées ou découpées par coin ; Corsa, GT86, Range Rover, A 45 déjà simplifiés ; Volvo `lamp_*` renommé.

## Critères d'acceptation (captures du banc PC)
- [ ] `vitrine-<id>-0/1/2` pour les 20 voitures : sens correct, 4 roues, couleur appliquée, feux non allumés en blanc
- [ ] 16 peintures vues sur une planche (une voiture) ; rapport de taille de l'APK au journal
- [ ] aucun point blanc parasite en trame

## Hors périmètre
Interface du garage (WFS-11), trafic avec 20 modèles (WFS-20 pour le LOD), sons (WFS-10 ✅).

## Réouverture du 2026-11-04 (PO) — périmètre en vigueur (prime sur ce qui précède)
WFS-11 est fusionné (PR #43) avec le garage, les 20 fiches (`src/ui/garage.cpp`, `FICHES`) et les 16 peintures, mais seules 4 voitures
sont converties (golf, m4, p911, gallardo). WFS-13 est rouvert pour **les 16 autres voitures**, rien d'autre. Taille : 1 à 2 jours.

### Règles de gestion (en vigueur)
- RG-10 : convertir les 16 voitures manquantes (p208, abarth, tcross, compass, s3, x5, giulia, a45, model3, corsa, gt86, elantra, v60,
  rrsport, mustang, sf90) depuis `waaps/waforspeed/models/<voiture>/<voiture>.json` vers `assets/cars/<id>.wfm`, avec les longueurs
  `d.L`, `rotateY`, motifs `paint` / `tail` de `CAR_MODELS` (`js/cars/catalog.js`). Les commandes de conversion sont écrites dans un
  **nouveau script** `tools/convertir_voitures.sh` (une ligne par voiture, arguments de `convert_car.mjs`), rejouable d'un coup.
- RG-11 : **ne pas modifier `tools/convert_car.mjs`** (WFS-60 y travaille : roues séparées). Les cas particuliers passent par les
  arguments (regex de peinture, de feux) ; si une voiture est impossible sans changer le convertisseur, la noter au Journal et la
  laisser de côté (la fiche reste au garage, on n'arrête pas le ticket pour elle).
- RG-12 : quand WFS-60 est fusionné avant la fin de ce ticket : `git merge origin/main` puis relancer `tools/convertir_voitures.sh`
  pour que les 16 voitures aient aussi des roues séparées. Sinon, fusionner tel quel (le chargeur de WFS-60 est rétro-compatible) et
  le noter au Journal : WFS-60 ou WFS-67 relancera le script.
- RG-13 : ajouter les 16 entrées au catalogue `CARS` de `src/game/cars.h` (fiche `ch`, `s100`, `vmax`, `kg`, `grip`, dimensions `d`,
  peinture par défaut, crédit) : **données seulement**, aucune logique (accroche annoncée au Journal, E3 a WFS-70 / WFS-24 en cours).
  Les performances viennent de `carPerf` déjà porté (RG-02) : rien à recoder.
- RG-14 : son moteur par voiture : 16 lignes ajoutées à `VOICES` de `src/audio/engine_synth.cpp` (fichier de `ENGINE_SET` du jeu web :
  v8, sport, citadine, thermique, gros, électrique ; mêmes hauteurs) — données seulement, accroche annoncée au Journal.
- RG-15 : licence de chaque modèle copiée dans `assets/licences/<id>.txt` ; crédit affiché au garage (déjà prévu par la fiche).
- RG-16 : taille : textures à 1024 px (réglage actuel du convertisseur). Noter au Journal la taille de chaque `.wfm` et de l'APK avant /
  après. Si l'APK dépasse 150 Mo, prévenir le PO au Journal (on décidera d'utiliser les versions `-lod` du jeu web) sans bloquer le ticket.
- RG-17 : banc : **une seule capture par nouvelle voiture** (`vitrine-<id>-0`, vue de 3/4 avant) + `garage-<id>` pour 2 voitures au
  choix (preuve que le garage les montre). Les 3 angles restent pour les 4 voitures actuelles. Ces captures vont dans un job qui n'est
  **pas** `ecrans-2400` (déjà 11 min, le plus long) : groupe `partie1` ou nouveau groupe `vitrines` en job parallèle.

### Critères d'acceptation (en vigueur)
- [ ] `vitrine-<id>-0` pour les 16 voitures : voiture entière, dans le bon sens, 4 roues au sol, peinture appliquée, feux arrière non blancs
- [ ] garage : les 20 voitures défilent ; `garage-<id>` sur 2 nouvelles voitures ; course lancée avec une nouvelle voiture sans plantage
- [ ] tailles des `.wfm` et de l'APK au Journal ; autres captures inchangées

### Hors périmètre
Versions `-lod` et variété du trafic (WFS-67), roues animées (WFS-60), modification de `convert_car.mjs`, interface du garage (WFS-11 ✅),
finitions de matériaux au pixel près.

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P2 Haute | WFS-13 | Les 16 autres voitures (conversion, catalogue, sons, licences) — rouvert après WFS-11 | E5 | ✅ Fusionné (fusion par confiance, QA de nuit à faire) (exception : 2e ticket E5, fichiers séparés de WFS-60) | agent 3 (session 011cpqZd), 2026-11-04 | WFS-11 ✅ | `feature/waforspeed-13` |  |  |

Critères et notes de suivi :

**WFS-13 — Les 16 autres voitures (rouvert après WFS-11)** · E5 · priorité P2 Haute · ✅ Fusionné (fusion par confiance, QA de nuit à faire) — agent 3 (session 011cpqZd), 2026-11-04, branche `feature/waforspeed-13` (règles : `regles-de-gestion/WFS-13.md`, section « Réouverture du 2026-11-04 »)
- [ ] `vitrine-<id>-0` pour les 16 voitures : bon sens, 4 roues au sol, peinture, feux arrière non blancs
- [ ] garage : 20 voitures ; `garage-<id>` sur 2 nouvelles ; course avec une nouvelle voiture sans plantage
- [ ] tailles des `.wfm` et de l'APK au Journal ; autres captures inchangées
