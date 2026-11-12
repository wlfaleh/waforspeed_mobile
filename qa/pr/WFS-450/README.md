# WFS-450 — captures pour le lead dev

Banc `WFS_BANC=lumieres-mouvement` (1280x576, Basse, nuit), une seule exécution sur la branche : la suite « avant » rejoue le même trajet avec l'ordre, le quota et le choix des lampadaires d'avant le ticket (`phares::avant450`), la suite « après » avec la correction. Planches montées avec ImageMagick (`montage`).

| Scène | Plus forte chute d'une image à la suivante, avant | après | RG-06 |
|---|---|---|---|
| Corniche, 85 → 105 km/h, 2 adversaires derrière | **45 %** (86 → 49) | **9 %** | ≤ 20 % : tenu |
| Ville, ≈ 90 km/h, voiture voisine | **85 %** (54 → 9) | **17 %** | ≤ 20 % : tenu |

Mesure : luminosité moyenne (luma des pixels, 0 à 255) du rectangle « chaussée autour de la voiture », sur chaque image ; tableaux complets : [`mouv-corniche.txt`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/mouv-corniche.txt), [`mouv-ville.txt`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/mouv-ville.txt).

**Avant en haut, après en bas, une image sur trois**

Corniche : ![compare-mouv-corniche.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/compare-mouv-corniche.jpg?raw=true)

Ville : ![compare-mouv-ville.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/compare-mouv-ville.jpg?raw=true)

| Capture | Avant (même trajet, `phares::avant450`) | Après (branche) |
|---|---|---|
| `mouv-corniche-planche`<br>Corniche, 45 images à 10 img/s (4,5 s), 85 → 105 km/h, 2 adversaires collés derrière | ![avant-mouv-corniche-planche.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-corniche-planche.jpg?raw=true) | ![apres-mouv-corniche-planche.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-corniche-planche.jpg?raw=true) |
| `mouv-corniche-04`<br>Corniche, image 04 : juste avant la chute | ![avant-mouv-corniche-04.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-corniche-04.jpg?raw=true) | ![apres-mouv-corniche-04.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-corniche-04.jpg?raw=true) |
| `mouv-corniche-05`<br>Corniche, image 05 (0,1 s plus tard) : avant, la chaussée est noire ; après, elle reste éclairée | ![avant-mouv-corniche-05.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-corniche-05.jpg?raw=true) | ![apres-mouv-corniche-05.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-corniche-05.jpg?raw=true) |
| `mouv-corniche-07`<br>Corniche, image 07 | ![avant-mouv-corniche-07.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-corniche-07.jpg?raw=true) | ![apres-mouv-corniche-07.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-corniche-07.jpg?raw=true) |
| `mouv-ville-planche`<br>Ville, 45 images à 10 img/s, ≈ 90 km/h, une voiture remonte sur la file voisine (85 m → 4 m) | ![avant-mouv-ville-planche.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-ville-planche.jpg?raw=true) | ![apres-mouv-ville-planche.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-ville-planche.jpg?raw=true) |
| `mouv-ville-08`<br>Ville, image 08 : juste avant la chute | ![avant-mouv-ville-08.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-ville-08.jpg?raw=true) | ![apres-mouv-ville-08.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-ville-08.jpg?raw=true) |
| `mouv-ville-09`<br>Ville, image 09 (0,1 s plus tard) : avant, rue noire ; après, rue éclairée | ![avant-mouv-ville-09.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-ville-09.jpg?raw=true) | ![apres-mouv-ville-09.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-ville-09.jpg?raw=true) |
| `mouv-ville-11`<br>Ville, image 11 | ![avant-mouv-ville-11.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-ville-11.jpg?raw=true) | ![apres-mouv-ville-11.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-ville-11.jpg?raw=true) |
| `mouv-ville-42`<br>Ville, image 42 : la voiture voisine arrive, ses phares montent en fondu | ![avant-mouv-ville-42.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-ville-42.jpg?raw=true) | ![apres-mouv-ville-42.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-ville-42.jpg?raw=true) |
| `mouv-ville-44`<br>Ville, image 44 : avant, la rue s'éteint encore ; après, non | ![avant-mouv-ville-44.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/avant-mouv-ville-44.jpg?raw=true) | ![apres-mouv-ville-44.jpg](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-450/qa/pr/WFS-450/apres-mouv-ville-44.jpg?raw=true) |
