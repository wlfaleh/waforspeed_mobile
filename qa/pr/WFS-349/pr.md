Agent : Claude, agent dev Y (01UY1Vv1Yvim)

**Ticket** : WFS-349 (P1, ANO-181 et part Cockpit de ANO-183) — règles : `regles-de-gestion/WFS-349.md`

## Demande de l'utilisateur
En vue Cockpit, une voiture toute proche (à côté de la sienne) se dessinait par-dessus le bas de la portière et la planche de bord : l'habitacle doit être fermé, on ne voit dehors que par les vitres.

## Cause (RG-01, au journal)
Un **trou dans l'habitacle**, pas un ordre ni une profondeur de dessin (l'intérieur est dessiné avec la profondeur, plan proche 0,1 m) :
chaque contre-porte n'était qu'une plaque de 6 cm, de 15 cm derrière le pied du pare-brise jusqu'à 0,8 m sous l'œil, sans plancher ni tablier.
Vue de l'œil (à ≈ 0,38 m de la portière gauche), la plaque est vue par la tranche : la vitre « descendait » jusqu'au coin bas de l'image,
le coin planche / montant / portière était ouvert, et la zone des pieds n'avait pas de paroi (vidéo de l'utilisateur, APK 928, 40,4 s).

## Ce qui change
- `src/render/vehicule_cockpit.cpp`, `vehicule_cockpit.h` : chaque portière est un **bloc plein de 12 cm** (face intérieure inchangée : largeur aux coudes de WFS-315),
  son dessus (appui de vitre) à la ligne de ceinture, de 6 cm devant le pied du pare-brise (sous le pied du montant) à 0,6 m derrière l'œil, du plancher à la ceinture ;
  **plancher** et **tablier** ferment la zone des pieds. Cotes dans une fonction pure (`boitesFermeture`), utilisée par le dessin et par le test.
  Matière des portières : plastique grainé du bas de planche, luminance ramenée à 0,045 au moins (jamais noir pur). Accoudoirs à la même place.
  Tout dans le même maillage : **0 appel de dessin de plus** (Golf 32, X5 35, 500 Abarth 29, avant comme après ; 1,22 Mo).
- Banc neuf `WFS_BANC=cockpit-voisine` (`src/render/vehicule_cockpit_banc.*`, accroche de 4 lignes dans `desktop_main.cpp`) : voisine à 1,8 puis 1,9 m de centre à centre,
  à gauche et à droite, devant et à côté, regards Bas / Normal / Haut, Ville de jour (images de nuit pour la Golf, le X5, la 500 Abarth). Trois images du même instant :
  normale, sans la voisine, habitacle + gabarit de ceinture (plans des vitres sous la ceinture, pied du pare-brise sous la planche) en couleur clé.
  Pixel interdit = pixel de la voisine dans la zone fermée. Secousse du choc coupée pendant la mesure ; clé douce (émission 1,5) ;
  bord de 5 px (pour 720 lignes) non compté : pixels de bord et points très lumineux de la voisine (reflet du soleil, feu) qui s'étalent de 3 à 4 px.
- Test CPU neuf `test_cockpit_ferme` (+ `CMakeLists.txt`) : rayons de l'œil (azimut ±80°, site −70° à +15°, pas de 0,2°, 4 positions de tête),
  20 voitures : aucun rayon ne sort sous la ceinture ni sous la planche sans paroi ; cotes (portière 10-15 cm, ceinture ±2 cm), jonctions, matière.
  Il a trouvé une fente de quelques mm sous le bord avant de la planche : fermée (tablier monté d'1 cm, caché derrière la planche).

## Comment c'est vérifié
- **RG-03, banc `cockpit-voisine`, 20 voitures × 3 regards × gauche / droite × 1,8 / 1,9 m × devant / à côté** :
  **0 pixel interdit en 1280 × 720 et en 2340 × 1080** ; voisine devant toujours visible par la vitre (≥ 18 900 px en 1280, ≥ 59 700 px en 2340).
  Avant (ancienne portière, même banc, 1280 × 720, bord de 3 px) : Golf 10 647, X5 10 838, 500 Abarth 12 388 pixels interdits.
- **RG-04** (banc `cockpit`, 2340 × 1080, Golf / X5 / 500 Abarth) : fenêtre de la rue cachée 0,0 % ; horizon inchangé (46,4 % Normal, 41,7 % Bas, 50,0 % Haut) ;
  intérieur en regard Normal 35,6 à 39,7 % (≤ 42 %), +1,4 à +1,7 point (la portière se voit) ; pluie (WFS-334) : sur `cockpit-golf-pluie` (après), gouttes seulement derrière les vitres, aucune sur la portière ni la planche (capture regardée, non retenue ici).
- **RG-05** : `WFS_BANC=parcours` : parcours complet 0 écart, et en vue Cockpit 0 écart ; `WFS_BANC=course5` (Course à 5, Poursuite) : code 0.
- `penetration` : recouvrement 0,000 m (WFS-333 inchangé). Après synchronisation avec `main` (WFS-293 fusionné, `camera_vue.cpp` changé) : tests CPU 91 réussis (avec `test_cockpit_ferme`), `cockpit-voisine` Golf / X5 / 500 Abarth toujours à 0.

## Limites et risques
- Ligne de ceinture : celle de WFS-315 (haut de planche − 1 cm), pas de cote mesurée par voiture ; plancher à 0,98 m sous l'œil (approximation SAE, ±3 cm).
- Temps GPU de la vue Cockpit en Basse **non mesuré sur téléphone** (banc logiciel seulement) : appels de dessin identiques, quelques triangles de plus (5 boîtes au lieu de 2).
- Le seuil « intérieur ≤ 42 % » vise le regard Normal ; en regard Bas, l'intérieur passe de 40,6 à 42,4 % (Golf, 500 Abarth).
- Banc : le X5 avec la voisine à droite garde une teinte magenta (couleur clé d'une mesure précédente) avant comme après ; mesure non touchée (images comparées teintées de même), images retirées de la PR.
- Le journal des bancs `parcours` et `course5` contient une ligne « [ERREUR] éclairage (WFS-213) : capture abandonnée » ; non vérifié sur `main` ; sans lien avec l'habitacle.
- À valider sur le téléphone de l'utilisateur (🟡) : une voiture qui double tout près en vue Cockpit, de jour et de nuit.

## Captures pour le lead dev
Dossier : [`qa/pr/WFS-349`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349) — `penetration` et `cockpit` par `tools/qa/captures_pr.sh` (2340 × 1080),
`cockpit-voisine` par le banc neuf (1280 × 720 ; « -2340 » : 2340 × 1080 ; « avant » : ancienne portière).

| Capture | Avant | Après |
|---|---|---|
| `penetration-cockpit-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-penetration-cockpit-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-penetration-cockpit-1.jpg?raw=true) |
| `penetration-cockpit-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-penetration-cockpit-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-penetration-cockpit-2.jpg?raw=true) |
| `penetration-cockpit-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-penetration-cockpit-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-penetration-cockpit-3.jpg?raw=true) |
| `penetration-cockpit-4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-penetration-cockpit-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-penetration-cockpit-4.jpg?raw=true) |
| `penetration-cockpit-4-sans-voisine` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-penetration-cockpit-4-sans-voisine.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-penetration-cockpit-4-sans-voisine.jpg?raw=true) |
| `cockpit-voisine-golf-gauche-1m9-devant-normal` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-voisine-golf-gauche-1m9-devant-normal.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-voisine-golf-gauche-1m9-devant-normal.jpg?raw=true) |
| `cockpit-voisine-golf-gauche-1m9-devant-normal-2340` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-voisine-golf-gauche-1m9-devant-normal-2340.jpg?raw=true) |
| `cockpit-voisine-golf-droite-1m9-devant-normal` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-voisine-golf-droite-1m9-devant-normal.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-voisine-golf-droite-1m9-devant-normal.jpg?raw=true) |
| `cockpit-voisine-golf-nuit-gauche-1m9-devant-normal` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-voisine-golf-nuit-gauche-1m9-devant-normal.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-voisine-golf-nuit-gauche-1m9-devant-normal.jpg?raw=true) |
| `cockpit-voisine-x5-gauche-1m9-devant-normal` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-voisine-x5-gauche-1m9-devant-normal.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-voisine-x5-gauche-1m9-devant-normal.jpg?raw=true) |
| `cockpit-voisine-x5-gauche-1m9-devant-normal-2340` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-voisine-x5-gauche-1m9-devant-normal-2340.jpg?raw=true) |
| `cockpit-voisine-x5-nuit-gauche-1m9-devant-normal` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-voisine-x5-nuit-gauche-1m9-devant-normal.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-voisine-x5-nuit-gauche-1m9-devant-normal.jpg?raw=true) |
| `cockpit-voisine-abarth-gauche-1m9-devant-normal` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-voisine-abarth-gauche-1m9-devant-normal.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-voisine-abarth-gauche-1m9-devant-normal.jpg?raw=true) |
| `cockpit-voisine-abarth-gauche-1m9-devant-normal-2340` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-voisine-abarth-gauche-1m9-devant-normal-2340.jpg?raw=true) |
| `cockpit-voisine-abarth-droite-1m9-devant-normal` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-voisine-abarth-droite-1m9-devant-normal.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-voisine-abarth-droite-1m9-devant-normal.jpg?raw=true) |
| `cockpit-voisine-abarth-nuit-gauche-1m9-devant-normal` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-voisine-abarth-nuit-gauche-1m9-devant-normal.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-voisine-abarth-nuit-gauche-1m9-devant-normal.jpg?raw=true) |
| `cockpit-golf-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-golf-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-golf-jour.jpg?raw=true) |
| `cockpit-golf-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-golf-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-golf-virage.jpg?raw=true) |
| `cockpit-golf-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-golf-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-golf-nuit.jpg?raw=true) |
| `cockpit-x5-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-x5-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-x5-jour.jpg?raw=true) |
| `cockpit-x5-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-x5-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-x5-virage.jpg?raw=true) |
| `cockpit-x5-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-x5-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-x5-nuit.jpg?raw=true) |
| `cockpit-abarth-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-abarth-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-abarth-jour.jpg?raw=true) |
| `cockpit-abarth-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-abarth-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-abarth-virage.jpg?raw=true) |
| `cockpit-abarth-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/avant-cockpit-abarth-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-349/qa/pr/WFS-349/apres-cockpit-abarth-nuit.jpg?raw=true) |
