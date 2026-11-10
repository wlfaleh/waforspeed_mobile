# WFS-323 — captures avant / après (banc `cockpit-reference`, 1560 × 720, rendu lavapipe comme la CI)

Branche `feature/waforspeed-323` comparée à `origin/main` (« avant » = `main` + le banc seul, sans aucun changement du cockpit).
Commande : `WFS_BANC=cockpit-reference WFS_TAILLE=1560x720 ./build/wfs_bench <dossier> assets/`.

## Côte à côte avec la référence de l'utilisateur (`qa/retours/2026-11-08-cockpit-reference-utilisateur.jpg`)

| Scène | Référence · Avant · Après |
|---|---|
| Golf, Corniche, jour, ligne droite | ![](comparaison-golf-corniche-jour.jpg) |
| M4, Corniche, jour | ![](comparaison-m4-corniche-jour.jpg) |
| X5, Corniche, jour | ![](comparaison-x5-corniche-jour.jpg) |
| Golf, Ville, nuit (pluie) | ![](comparaison-golf-ville-nuit.jpg) |

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
| Intérieur Golf : appels de dessin, mémoire | 32, 1,22 Mo | 33, 1,60 Mo |
| Volant (braquage −0,17 → −0,67) | +0,38 → +1,53 rad | +0,38 → +1,53 rad (tourne, ANO-187) |
