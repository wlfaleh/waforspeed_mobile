Agent : Claude, Agent 0 developer

## Demande de l'utilisateur
« Les grands jeux préparent tout au chargement » (ordre du 2026-11-07, 17:32 UTC) : ce qui ne bouge pas se calcule une fois au chargement, pas à chaque image. WFS-269 : éclairage du décor fixe précalculé (arbitrage du PO : A + C, B refusé).

## Décisions du PO (2026-11-08 04:42 UTC, `regles-de-gestion/WFS-269.md`)
1. Effet discret accepté en Ville, avec les immeubles du kit WFS-161 dans la grille. Il est jugé sur des gros plans sans / avec (même cadrage, carte d'écart, luminance mesurée), jamais poussé au-delà du physique. Conditions : chargement de la Ville à +3 s au plus sur téléphone (WFS-268 compris), coût en course neutre, ticket en 🟡 après la fusion.
2. Zone : calcul dans `src/render/eclairage_precalcule.*` ; dans `src/world/archi_carte.cpp`, une accroche de 10 lignes au plus, annoncée au journal (commit 4a01299f sur main).

## Ce qui change
**Calcul** (`src/render/eclairage_precalcule.*`, nouveau) : au chargement, pour chaque sommet qui le reçoit, la part du ciel vue (occlusion ambiante).
- Grille d'occupation à deux niveaux : voxels de 0,5 m, cellules de 8 m. Elle est remplie en parallèle, une grille par tâche, puis les grilles sont fusionnées.
- 8 rayons en cosinus, portée 40 m (28 m séparent les façades d'une rue), pas d'un voxel (aucun mur sauté), saut direct des cellules vides.
- Les rayons vers le bas sont libres : la sonde SH9 tient déjà compte du sol.
- Le résultat est rangé dans la longueur de la normale : aucun octet de plus par sommet.
- Murs des bâtiments du kit : `occlusionKit()` calcule leur occlusion au chargement (niveaux de détail 0 et 1) et la garde. `appliquerKit()` multiplie les normales quand un morceau est construit en course : aucun rayon en course.

**Accroches** :
- `scene.frag` (3 lignes, après le mélange de WFS-263) et `MAT_AO` (`renderer.h`) : l'occlusion touche l'ambiance et le reflet du ciel seulement ; le soleil, les ombres, les lampes et les phares ne changent pas (RG-03).
- `archi_carte.cpp` : +10 lignes, 1 modifiée, aucune ligne de WFS-268 déplacée.
- `lots_decor.*` : préparation des blocs et occultants en plus.
- `cartes_banc.cpp` : gros plans `carte-ville-ao-*` et `WFS_SANS_AO=1`, pour le banc seulement.

**Cartes** :
- **Ville** : façades, béton et trottoirs du décor fixe reçoivent l'occlusion, ainsi que les murs des 580 immeubles du kit (dont la version allégée cache le ciel).
- **Corniche** : seuls le chaume, le bois et les villas du kit la reçoivent. Le décor uni, les rochers et les trottoirs ne la reçoivent plus : c'était la cause des dents de scie sous le surplomb, des traînées sur la falaise et des taches (2 revues du lead dev).
- **Campagne et Grand Prix** : rien ne change à l'écran (`LOTS_CARTES_SOCLE=false`, pas de blocs).

## Comment c'est vérifié
- **Test CPU** `test_fidelite_lumiere_precalcule`, 10 contrôles, dont une rue de 28 m : bas de façade 0,81 contre 0,91 en haut. **67 tests CPU OK.**
- **Luminance mesurée sur les gros plans** (zone de façade, sans → avec) : pied de façade à l'ombre −14,3 % et −5,7 % ; façades au soleil −1,2 % et 0,0 % ; nuit −0,2 %.
- **Chargement (RG-04)**, Ville, PC : décor fixe préparé en ≈ 1,1 s (main ≈ 0,35 s), dont occlusion ≈ 0,7 s et 12,7 Mo temporaires. Murs du kit : 0,2 s. Soit **≈ +1,0 s sur PC**.
- **Coût en course (C)**, banc `perf` Ville, même point de main, 2 passages : main Haute 771 et 771 ms, Basse 334 et 332 ms ; branche Haute 784 et 771 ms, Basse 343 et 330 ms. **Neutre** (bruit du GPU logiciel).
- **Captures** : 75 paires avant / après (Ville, Corniche, jour et nuit), toutes regardées. Corniche : `bateaux-vue`, `jeux-vue`, `rond-point-dessus`, `epingle-dessus`, `lacets-dessus`, `cafe-vue` identiques à main (plus de dents de scie, de traînée ni de tache) ; le gain restant est sur les villas du kit à l'ombre (`marina`, `bateaux`, `bar`), discret. Ville : façades et trottoirs à l'ombre plus sombres, aucune bande ni tache vue.
- 8 gros plans `carte-ville-ao-*` (après seulement : vues nouvelles) et 5 comparaisons sans / avec, même binaire.

## Limites et risques
- Effet discret par nature (lumière du ciel seulement) : net au pied des façades à l'ombre, presque nul au soleil et la nuit.
- Chargement sur téléphone non mesuré : ≈ +1,0 s sur PC, à la limite du budget de +3 s sur téléphone. L'utilisateur le mesure (chiffre au journal, 🟡). Réglages de repli : portée 20 m, ou murs du kit au niveau 1 seulement.
- 8 rayons par sommet : paliers de 1/8, lissés par l'interpolation.

## Captures pour le lead dev
Tableau complet : [`qa/pr/WFS-269/README.md`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/README.md), produit par `AGENT='Claude, Agent 0 developer' bash tools/qa/captures_pr.sh WFS-269 ville corniche`.

### Gros plans au pied des façades (même cadrage, même binaire ; écart × 16 : blanc = 6 % ou plus)

| Gros plan | Luminance de la façade | Sans occlusion | Avec occlusion | Écart × 16 |
|---|---|---|---|---|
| `carte-ville-ao-3-g` (jour, pied de façade à l'ombre) | −14,3 % | ![sans](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/sans-ao-carte-ville-ao-3-g.jpg?raw=true) | ![avec](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avec-ao-carte-ville-ao-3-g.jpg?raw=true) | ![écart](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/ecart-x16-carte-ville-ao-3-g.jpg?raw=true) |
| `carte-ville-ao-1-g` (jour, pied de façade à l'ombre) | −5,7 % | ![sans](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/sans-ao-carte-ville-ao-1-g.jpg?raw=true) | ![avec](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avec-ao-carte-ville-ao-1-g.jpg?raw=true) | ![écart](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/ecart-x16-carte-ville-ao-1-g.jpg?raw=true) |
| `carte-ville-ao-1-d` (jour, sous la terrasse (façade au soleil)) | −1,2 % | ![sans](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/sans-ao-carte-ville-ao-1-d.jpg?raw=true) | ![avec](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avec-ao-carte-ville-ao-1-d.jpg?raw=true) | ![écart](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/ecart-x16-carte-ville-ao-1-d.jpg?raw=true) |
| `carte-ville-ao-2-g` (jour, façade au soleil) | 0,0 % | ![sans](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/sans-ao-carte-ville-ao-2-g.jpg?raw=true) | ![avec](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avec-ao-carte-ville-ao-2-g.jpg?raw=true) | ![écart](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/ecart-x16-carte-ville-ao-2-g.jpg?raw=true) |
| `carte-ville-ao-1-g-nuit` (nuit) | −0,2 % | ![sans](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/sans-ao-carte-ville-ao-1-g-nuit.jpg?raw=true) | ![avec](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avec-ao-carte-ville-ao-1-g-nuit.jpg?raw=true) | ![écart](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/ecart-x16-carte-ville-ao-1-g-nuit.jpg?raw=true) |

### Paires avant (main) / après (branche)

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `carte-ville-virage-2` (façades et trottoir à l'ombre, à droite) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avant-carte-ville-virage-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/apres-carte-ville-virage-2.jpg?raw=true) |
| `carte-ville-feux` (façades à l'ombre) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avant-carte-ville-feux.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/apres-carte-ville-feux.jpg?raw=true) |
| `carte-plage-marina` (villas du kit à l'ombre, à gauche) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avant-carte-plage-marina.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/apres-carte-plage-marina.jpg?raw=true) |
| `carte-plage-bateaux-vue` (surplomb : plus de dents de scie (identique à main)) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avant-carte-plage-bateaux-vue.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/apres-carte-plage-bateaux-vue.jpg?raw=true) |
| `carte-plage-rond-point-dessus` (giratoire : plus de taches (identique à main)) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avant-carte-plage-rond-point-dessus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/apres-carte-plage-rond-point-dessus.jpg?raw=true) |
| `carte-plage-casino-vue` (casino au soleil : inchangé) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avant-carte-plage-casino-vue.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/apres-carte-plage-casino-vue.jpg?raw=true) |
| `carte-ville-casino-nuit` (nuit) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avant-carte-ville-casino-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/apres-carte-ville-casino-nuit.jpg?raw=true) |
