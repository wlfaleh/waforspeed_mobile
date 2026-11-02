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
