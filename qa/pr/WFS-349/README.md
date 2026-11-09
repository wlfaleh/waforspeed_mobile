# WFS-349 — captures avant / après

Base : branche synchronisée avec `main` (même éclairage avant / après). `penetration` et `cockpit` : `tools/qa/captures_pr.sh` (2340 × 1080) ;
`cockpit-voisine` (banc neuf, 1280 × 720) : « avant » = même banc construit avec l'ancienne portière. Vues Poursuite et accident (inchangées) retirées.

Exemple de bord (remarque du lead dev) : Compass, voisine à gauche, banc lancé avec un bord de 2 px (`WFS_BORD=2`) au lieu de 5 : 13 pixels comptés dans cette image (51 dans la pire image de la Compass),
en magenta, tous sur le bord de l'appui de vitre, sous le feu arrière rouge de la voisine (lueur du feu, pas un trou) ; avec le bord de 5 px : 0.

![exemple](exemple-bord-2px-compass-zoom.jpg) ![image entière](exemple-bord-2px-compass-ecarts.jpg)

| Capture | Avant | Après |
|---|---|---|
| `penetration-cockpit-1` | ![avant](avant-penetration-cockpit-1.jpg) | ![après](apres-penetration-cockpit-1.jpg) |
| `penetration-cockpit-2` | ![avant](avant-penetration-cockpit-2.jpg) | ![après](apres-penetration-cockpit-2.jpg) |
| `penetration-cockpit-3` | ![avant](avant-penetration-cockpit-3.jpg) | ![après](apres-penetration-cockpit-3.jpg) |
| `penetration-cockpit-4` | ![avant](avant-penetration-cockpit-4.jpg) | ![après](apres-penetration-cockpit-4.jpg) |
| `penetration-cockpit-4-sans-voisine` | ![avant](avant-penetration-cockpit-4-sans-voisine.jpg) | ![après](apres-penetration-cockpit-4-sans-voisine.jpg) |
| `cockpit-voisine-golf-gauche-1m9-devant-normal` | ![avant](avant-cockpit-voisine-golf-gauche-1m9-devant-normal.jpg) | ![après](apres-cockpit-voisine-golf-gauche-1m9-devant-normal.jpg) |
| `cockpit-voisine-golf-droite-1m9-devant-normal` | ![avant](avant-cockpit-voisine-golf-droite-1m9-devant-normal.jpg) | ![après](apres-cockpit-voisine-golf-droite-1m9-devant-normal.jpg) |
| `cockpit-voisine-golf-nuit-gauche-1m9-devant-normal` | ![avant](avant-cockpit-voisine-golf-nuit-gauche-1m9-devant-normal.jpg) | ![après](apres-cockpit-voisine-golf-nuit-gauche-1m9-devant-normal.jpg) |
| `cockpit-voisine-x5-gauche-1m9-devant-normal` | ![avant](avant-cockpit-voisine-x5-gauche-1m9-devant-normal.jpg) | ![après](apres-cockpit-voisine-x5-gauche-1m9-devant-normal.jpg) |
| `cockpit-voisine-x5-droite-1m9-devant-normal` | ![avant](avant-cockpit-voisine-x5-droite-1m9-devant-normal.jpg) | ![après](apres-cockpit-voisine-x5-droite-1m9-devant-normal.jpg) |
| `cockpit-voisine-x5-nuit-gauche-1m9-devant-normal` | ![avant](avant-cockpit-voisine-x5-nuit-gauche-1m9-devant-normal.jpg) | ![après](apres-cockpit-voisine-x5-nuit-gauche-1m9-devant-normal.jpg) |
| `cockpit-voisine-abarth-gauche-1m9-devant-normal` | ![avant](avant-cockpit-voisine-abarth-gauche-1m9-devant-normal.jpg) | ![après](apres-cockpit-voisine-abarth-gauche-1m9-devant-normal.jpg) |
| `cockpit-voisine-abarth-droite-1m9-devant-normal` | ![avant](avant-cockpit-voisine-abarth-droite-1m9-devant-normal.jpg) | ![après](apres-cockpit-voisine-abarth-droite-1m9-devant-normal.jpg) |
| `cockpit-voisine-abarth-nuit-gauche-1m9-devant-normal` | ![avant](avant-cockpit-voisine-abarth-nuit-gauche-1m9-devant-normal.jpg) | ![après](apres-cockpit-voisine-abarth-nuit-gauche-1m9-devant-normal.jpg) |
| `cockpit-golf-jour` | ![avant](avant-cockpit-golf-jour.jpg) | ![après](apres-cockpit-golf-jour.jpg) |
| `cockpit-golf-virage` | ![avant](avant-cockpit-golf-virage.jpg) | ![après](apres-cockpit-golf-virage.jpg) |
| `cockpit-golf-nuit` | ![avant](avant-cockpit-golf-nuit.jpg) | ![après](apres-cockpit-golf-nuit.jpg) |
| `cockpit-x5-jour` | ![avant](avant-cockpit-x5-jour.jpg) | ![après](apres-cockpit-x5-jour.jpg) |
| `cockpit-x5-virage` | ![avant](avant-cockpit-x5-virage.jpg) | ![après](apres-cockpit-x5-virage.jpg) |
| `cockpit-x5-nuit` | ![avant](avant-cockpit-x5-nuit.jpg) | ![après](apres-cockpit-x5-nuit.jpg) |
| `cockpit-abarth-jour` | ![avant](avant-cockpit-abarth-jour.jpg) | ![après](apres-cockpit-abarth-jour.jpg) |
| `cockpit-abarth-virage` | ![avant](avant-cockpit-abarth-virage.jpg) | ![après](apres-cockpit-abarth-virage.jpg) |
| `cockpit-abarth-nuit` | ![avant](avant-cockpit-abarth-nuit.jpg) | ![après](apres-cockpit-abarth-nuit.jpg) |
