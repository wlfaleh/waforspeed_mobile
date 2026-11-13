Agent : Claude, agent dev3d (01Us6uCt5A)

**Ticket** : WFS-397 — règles : `regles-de-gestion/WFS-397.md` (épic E10, exception PO : fichiers neufs `remplacants/` seulement)

## Demande
Retoucher les deux modèles libres trouvés par WFS-396 (Classe E W212 de Peter_D, VW Virtus GT de BHP3D, CC BY 4.0) pour les ramener à ±1 % sur les mesures de la fiche, sans aucune marque et à 8 Mo au plus, puis les ranger dans `assets/cars/remplacants/` (remplaçants de l'A 45 et de l'Elantra N). Le branchement dans le jeu reste dans WFS-360.

## Ce qui change
- **Rien dans le jeu** (RG-07) : `src/`, `shaders/`, `cars.h`, les modèles actuels et les outils existants ne sont pas touchés.
- `assets/cars/remplacants/w212.wfm` + `.lod` (7,95 Mo, 155 k triangles) et `virtus.wfm` + `.lod` (7,74 Mo, 155 k triangles).
- `assets/licences/remplacants/w212.txt`, `virtus.txt` (licence d'origine, dépôt, SHA-256, « modified (ride height) »), plus une ligne dans le `README.md`.
- `tools/remplacants/` (fichiers neufs, chaîne rejouable) :
  - `convertir_remplacants.sh` : repeint → `simplifier_voiture.mjs` → `convert_car.mjs` → relèvement → `corriger_voitures` → logos → `alleger_voiture.mjs` ;
  - `relever_caisse.mjs` : caisse remontée par rapport aux roues, roues fixes, aucun triangle étiré (contrôle : l'outil s'arrête sinon) ;
  - `retirer_pieces.mjs` + `pieces.json` : retire les pièces de logo, prolonge les barres de calandre, lisse un disque de logo à fleur de tôle (chaque pièce est contrôlée : boîte et nombre de triangles) ;
  - `repeindre_textures.mjs` + `textures.json` : logos imprimés dans les textures, repeints sur une copie de la source ;
  - `wfm.mjs` : lecture / écriture d'un `.wfm` (vérifié : relire puis écrire redonne le fichier à l'octet près).

## Mesures (RG-03, RG-08 ; `build/corriger_voitures --essai`, même mesure que `test_fidelite_voitures`, fiches de `references/reel/voitures/remplacants-wfs396.json`)
| Voiture | Longueur | Largeur | Hauteur | Empattement | Voie AV | Voie AR | Poids | Triangles |
|---|---|---|---|---|---|---|---|---|
| **W212** avant retouche | 4846 (−0,5 %) | 1872 (+1,0 %) | **1430 (−2,7 %)** ❌ | 2851 (−0,8 %) | 1596 (+0,7 %) | 1596 (−0,5 %) | 7,95 Mo | 74 k |
| **W212** après (fiche 4868 / 1854 / 1470 / 2874 / 1585 / 1604) | 4853 (−0,3 %) | 1870 (+0,9 %) | **1472 (+0,2 %)** ✅ | 2855 (−0,7 %) | 1598 (+0,8 %) | 1598 (−0,4 %) | 7,95 Mo | 155 k |
| **Virtus** avant retouche | 4561 (0 %) | 1740 (−0,7 %) | **1467 (−2,6 %)** ❌ | 2639 (−0,5 %) | 1498 (−0,9 %) | 1498 (+0,1 %) | 12,7 Mo ❌ | 212 k |
| **Virtus** après (fiche 4561 / 1752 / 1507 / 2651 / 1511 / 1496) | 4561 (0 %) | 1737 (−0,8 %) | **1507 (0 %)** ✅ | 2638 (−0,5 %) | 1498 (−0,9 %) | 1498 (+0,1 %) | 7,74 Mo ✅ | 155 k |

**6 mesures sur 6 dans ±1 % pour chaque voiture, sans dérogation.** Jeu entre le haut du pneu et l'aile : W212 22 / 13 mm → 62 / 53 mm (avant / arrière), Virtus 36 / 33 mm → 75 / 72 mm. Les deux modèles étaient posés « bas », le pneu presque collé à l'aile : on remonte toute la caisse, roues fixes, sans rien étirer.

## Sans marque (RG-05)
- **W212** : étoiles de la calandre, du capot et du coffre retirées, avec l'anneau et le disque noir de la calandre ; les deux barres de la calandre sont prolongées jusqu'au milieu : grille à barres continues, rien de posé devant la carrosserie (leçon de la X5) ; écusson « MERCEDES-BENZ » et étoiles repeints dans la texture chromée.
- **Virtus** : logo de la calandre et son fond, « VW » des portes et du coffre, « GT » (calandre, ailes, coffre), logo du volant retirés ; mot « Virtus » des plaques et bouchon à lettres repeints ; disque du logo du coffre, modelé dans la tôle, effacé (normales lissées, positions inchangées).
- Noms inventés à proposer par le PO au branchement (WFS-360) ; suggestion : « Berline Kaïros » (W212) et « Vela GT » (Virtus).

## Comment c'est vérifié
- Mesures : `corriger_voitures --essai` ci-dessus (6/6 ± 1 %) ; poids < 8 Mo (8 000 000 octets) ; `relever_caisse.mjs` : 0 triangle étiré.
- `bash tools/qa/tests_cpu.sh build` : 123 tests réussis (dont `test_fidelite_voitures`, `test_voitures_retirees`), sur la branche sans l’accroche locale.
- Captures : banc `vitrines-feux` (lavapipe, 1280×720, même rendu que la CI), **jour et nuit**, regardées une par une. Sur `main`, l'A 45 et l'Elantra sont retirées du jeu (WFS-404) et le banc les saute : pour les deux séries, une **accroche locale jamais poussée** (`VOITURES_RETIREES` vidée, correctifs de feux de l'A 45 / Elantra coupés) et une **permutation locale** des fichiers (`a45.wfm` ← `w212.wfm`, `elantra.wfm` ← `virtus.wfm`), restaurée ensuite (`git status` propre). « Avant » = A 45 et Elantra N actuelles de `main` ; « après » = les remplaçants. `tools/qa/captures_pr.sh` ne peut pas faire la série « avant » (voitures retirées sur `main`) : mêmes noms et même format que lui.

## Limites et risques
- **Petit défaut restant (RG-09, tâche séparée à créer par le PO)** : Virtus, petite encoche (≈ 8 mm) dans le bord avant du capot, là où s'emboîtait le logo ; elle est tissée dans la tôle et la corriger sans déformer le capot demande une retouche à la main. Le centre de la grille de la Virtus reste noir uni (les lamelles s'arrêtaient au logo), comme une plaque de radar.
- Formes des phares et des feux d'origine gardées (signature lumineuse reconnaissable de près) : hors de portée de ce ticket, à arbitrer par le PO pour WFS-360 si besoin.
- 155 k triangles par voiture (repère de 120 k dépassé, comme la S3, la X5 et la 208) : la carrosserie n'est pas simplifiée (à l'erreur 0,002 puis 0,0004 elle sortait froissée) ; les niveaux de détail `.lod` (40 %, 16 %, 9 %) servent à distance. À mesurer au banc perf au branchement (WFS-360).
- Défaut de `tools/convert_car.mjs` (hors zone, non modifié) : le test des roues `/rim/` attrape la peinture « p**rim**ary » des modèles de BHP3D (et la Brezza déjà dans le jeu) ; mes outils l'évitent. À signaler au PO.
- Garde au sol mesurée : W212 200 mm, Virtus 252 mm (au point bas de la caisse, mesure du moteur) ; hauteur exactement celle de la fiche.

## Captures pour le lead dev (avant = `main`, après = remplaçant permuté en local ; dossier [`qa/pr/WFS-397`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397))

### A 45 → Classe E W212

| Vue | Avant (main) | Après (branche) |
|---|---|---|
| face | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-jour-vitrine-a45-a0.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-jour-vitrine-a45-a0.jpg?raw=true) |
| 3/4 avant | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-jour-vitrine-a45-a1.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-jour-vitrine-a45-a1.jpg?raw=true) |
| profil (hauteur, jeu des roues) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-jour-vitrine-a45-a2.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-jour-vitrine-a45-a2.jpg?raw=true) |
| 3/4 arrière | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-jour-vitrine-a45-a3.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-jour-vitrine-a45-a3.jpg?raw=true) |
| vue basse | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-jour-vitrine-a45-bas.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-jour-vitrine-a45-bas.jpg?raw=true) |
| vue basse 3/4 (roue dans l'aile) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-jour-vitrine-a45-bas3.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-jour-vitrine-a45-bas3.jpg?raw=true) |
| zoom ×2 calandre | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-zoom-calandre-a45.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-zoom-calandre-a45.jpg?raw=true) |
| zoom ×2 coffre | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-zoom-coffre-a45.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-zoom-coffre-a45.jpg?raw=true) |
| feux stop | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-jour-vitrine-a45-feux.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-jour-vitrine-a45-feux.jpg?raw=true) |
| nuit, face (phares) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-nuit-vitrine-a45-a0.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-nuit-vitrine-a45-a0.jpg?raw=true) |
| nuit, feux arrière | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-nuit-vitrine-a45-feux.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-nuit-vitrine-a45-feux.jpg?raw=true) |

### Elantra N → Virtus GT

| Vue | Avant (main) | Après (branche) |
|---|---|---|
| face | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-jour-vitrine-elantra-a0.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-jour-vitrine-elantra-a0.jpg?raw=true) |
| 3/4 avant | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-jour-vitrine-elantra-a1.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-jour-vitrine-elantra-a1.jpg?raw=true) |
| profil (hauteur, jeu des roues) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-jour-vitrine-elantra-a2.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-jour-vitrine-elantra-a2.jpg?raw=true) |
| 3/4 arrière | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-jour-vitrine-elantra-a3.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-jour-vitrine-elantra-a3.jpg?raw=true) |
| vue basse | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-jour-vitrine-elantra-bas.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-jour-vitrine-elantra-bas.jpg?raw=true) |
| vue basse 3/4 (roue dans l'aile) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-jour-vitrine-elantra-bas3.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-jour-vitrine-elantra-bas3.jpg?raw=true) |
| zoom ×2 calandre | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-zoom-calandre-elantra.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-zoom-calandre-elantra.jpg?raw=true) |
| zoom ×2 coffre | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-zoom-coffre-elantra.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-zoom-coffre-elantra.jpg?raw=true) |
| feux stop | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-jour-vitrine-elantra-feux.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-jour-vitrine-elantra-feux.jpg?raw=true) |
| nuit, face (phares) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-nuit-vitrine-elantra-a0.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-nuit-vitrine-elantra-a0.jpg?raw=true) |
| nuit, feux arrière | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/avant-nuit-vitrine-elantra-feux.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-397/qa/pr/WFS-397/apres-nuit-vitrine-elantra-feux.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01Us6uCt5AqkHmCmnMByoftq
