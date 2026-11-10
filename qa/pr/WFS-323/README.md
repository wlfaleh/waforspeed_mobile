# WFS-323 — captures avant / après (banc `cockpit-reference`, rendu lavapipe comme la CI ; après : 2340 × 1080, avant : 1560 × 720)

Branche `feature/waforspeed-323` comparée à `origin/main` (« avant » = `main` + le banc seul, sans aucun changement du cockpit).
Commande : `WFS_BANC=cockpit-reference WFS_TAILLE=2340x1080 ./build/wfs_bench <dossier> assets/`.

## Côte à côte avec la référence de l'utilisateur (`qa/retours/2026-11-08-cockpit-reference-utilisateur.jpg`)

| Scène | Référence · Avant · Après |
|---|---|
| Golf, Corniche, jour, ligne droite | ![](comparaison-golf-corniche-jour.jpg) |
| M4, Corniche, jour | ![](comparaison-m4-corniche-jour.jpg) |
| X5, Corniche, jour | ![](comparaison-x5-corniche-jour.jpg) |
| Golf, Ville, nuit (pluie) | ![](comparaison-golf-ville-nuit.jpg) |

## Combiné de chaque voiture (revue du lead dev, ANO-188) : agrandissements sans réduction (600 × 300 px pris dans l'image 2340 × 1080)

Planche des 20 voitures (ordre alphabétique des identifiants) : ![](apres-combines-20-voitures.jpg)

| Voiture | Avant (main, image 1560 × 720 agrandie) | Après (2340 × 1080, sans réduction) |
|---|---|---|
| Golf | ![](avant-combine-golf.jpg) | ![](apres-combine-golf.jpg) |
| M4 | ![](avant-combine-m4.jpg) | ![](apres-combine-m4.jpg) |
| X5 | ![](avant-combine-x5.jpg) | ![](apres-combine-x5.jpg) |

Les 17 autres : `apres-combine-<voiture>.jpg`. Bande vue par l'ouverture haute du volant (journal du banc) : Golf, Elantra, T-Cross, V60 100 % ; Corsa 97 % ; Mustang 90 % ; M4, 911, Compass 89 % ; GT86 86 % ; S3 84 % ; A 45 82 % ; Giulia 80 % ; Gallardo 79 % ; Range Rover 74 % ; SF90 70 % ; X5 59 % ; Abarth 57 % ; 208 (i-Cockpit, lu au-dessus du volant) et Model 3 (vitesse sur l'écran central) : sans objet.

## Toutes les captures

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `golf-corniche-jour` | ![avant](avant-golf-corniche-jour.jpg) | ![après](apres-golf-corniche-jour.jpg) |
| `m4-corniche-jour` | ![avant](avant-m4-corniche-jour.jpg) | ![après](apres-m4-corniche-jour.jpg) |
| `x5-corniche-jour` | ![avant](avant-x5-corniche-jour.jpg) | ![après](apres-x5-corniche-jour.jpg) |
| `golf-ville-nuit` | ![avant](avant-golf-ville-nuit.jpg) | ![après](apres-golf-ville-nuit.jpg) |
| `golf-pavillon` (regard Haut) | ![avant](avant-golf-pavillon.jpg) | ![après](apres-golf-pavillon.jpg) |
| `golf-regard-bas` (regard Bas) | ![avant](avant-golf-regard-bas.jpg) | ![après](apres-golf-regard-bas.jpg) |
| `volant-suite` (braquage à gauche, 4 images) | ![avant](avant-volant-suite.jpg) | ![après](apres-volant-suite.jpg) |
| `golf-volant-1` à `-4` | ![avant](avant-golf-volant-1.jpg) ![avant](avant-golf-volant-4.jpg) | ![après](apres-golf-volant-1.jpg) ![après](apres-golf-volant-4.jpg) |

## Mesures écrites par le banc (journal)

| | Avant | Après |
|---|---|---|
| Horizon (regard Normal), depuis le haut | 46,5 % | **42,9 %** (RG-01 : 40 à 45 %) |
| Champ vertical / horizontal (2,17:1) | 73,4° / 116,5° | 63,4° / 106,4° |
| Regard Bas / Haut : horizon | 42,0 % / 50,0 % | 37,6 % / 48,6 % |
| Intérieur Golf : appels de dessin, mémoire | 32, 1,22 Mo | 34, 1,60 Mo |
| Volant (braquage −0,17 → −0,67) | +0,38 → +1,53 rad | +0,38 → +1,53 rad (tourne, ANO-187) |
