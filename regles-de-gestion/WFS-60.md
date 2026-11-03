# WFS-60 — Roues séparées dans les modèles de voiture (.wfm)
Épic : E5 · Priorité : 17.6 (hors MVP) · Dépend de : — · Taille : ≈ 3 h

## Besoin (qui, quoi, pourquoi)
Aujourd'hui les roues d'une voiture sont dessinées avec la carrosserie (simple drapeau « roue » au niveau du matériau) : on ne peut ni les faire tourner ni les braquer. Il faut que chaque voiture ait **4 roues distinctes**, chacune avec son centre, pour que WFS-61 / WFS-62 les animent.

## Règles de gestion
- RG-01 : à la conversion, chaque roue devient un **maillage à part** (4 par voiture : avant gauche, avant droite, arrière gauche, arrière droite), repéré par un drapeau et un numéro de roue (0 à 3) dans le `.wfm`.
- RG-02 : pour chaque roue on enregistre son **centre** (x, y, z, en mètres, repère de la voiture) et son **rayon** ; la géométrie de la roue est recentrée sur son centre (elle tourne autour de son axe).
- RG-03 : l'étrier de frein ne tourne pas avec la roue : il reste dans la carrosserie.
- RG-04 : voiture rendue **à l'identique** qu'avant la conversion (aucune différence visible sur les captures du garage / de la course).
- RG-05 : le chargeur de modèles lit les roues et les expose (centre, rayon, maillage) sans casser les anciens `.wfm` (version de format augmentée, lecture des deux).
- RG-06 : les 4 voitures actuelles sont reconverties ; les autres voitures (WFS-13) utiliseront la même conversion.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/cars/models.js` → `splitWheels(...)` (découpe d'un maillage « 4 roues en un » par coin : LF, RF, LB, RB), `looksLikeWheel`, `WHEEL_RX` ; `wheelRx` des versions LOD.
- Modèles : `models/` (mêmes fichiers), conversion par `tools/convert_car.mjs` (déjà `wheel:` regex et drapeau 128).
- Retours connus : certaines voitures (Corsa, GT86, Range Rover, A 45) sont déjà simplifiées ; roues parfois découpées par coin.

## Critères d'acceptation (captures du banc PC)
- [ ] Captures de voiture (garage / course) identiques à avant (0 écart visible), 4 roues présentes sur chaque voiture
- [ ] Test : chaque `.wfm` donne 4 roues avec centre et rayon cohérents (rayon 0,25 à 0,45 m, centres symétriques gauche/droite)

## Hors périmètre
Animation (WFS-61, WFS-62), flou de roue, les 16 autres voitures.
