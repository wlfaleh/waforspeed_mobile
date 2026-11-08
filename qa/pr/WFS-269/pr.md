Agent : Claude, Agent 0 developer

## Demande de l'utilisateur
« Les grands jeux préparent tout au chargement » (ordre du 2026-11-07, 17:32 UTC) : ce qui ne bouge pas se calcule une fois, au chargement, pas à chaque image. WFS-269 : éclairage du décor fixe précalculé (arbitrage du PO : A + C, B refusé).

## Ce qui change
**Calcul** (nouveau module `src/render/eclairage_precalcule.*`) : au chargement, pour chaque sommet qui le reçoit, la part du ciel vue (occlusion ambiante).
- Grille d'occupation à deux niveaux (voxels de 0,5 m, cellules de 8 m), remplie en parallèle (une grille par tâche, fusion par OU).
- 8 rayons en cosinus, **portée 40 m** (la rue entière : 28 m de façade à façade), **pas d'un voxel** (aucun mur sauté) avec saut direct des cellules vides ; rayons vers le bas libres (la sonde SH9 tient déjà compte du sol : sinon toute façade dégagée serait assombrie deux fois).
- Rangement dans la **longueur de la normale** : aucun octet de plus par sommet.

**Ce qui cache le ciel et ce qui reçoit l'occlusion** :
- **Ville** (`ville.cpp`) : décor fixe des blocs (façades, béton, trottoirs) **et les 580 immeubles du kit d'architecture** (WFS-161). Ces immeubles sont construits à la volée en course : leur géométrie allégée est ajoutée à la grille, puis l'occlusion de leurs murs (niveaux 0 et 1, 452 000 sommets) est calculée **au chargement** (`archi_carte.cpp`, matériaux des murs en `MAT_AO`), la grille est ensuite rendue. En course, la construction d'un morceau ne fait que multiplier la normale par la valeur gardée.
- **Corniche** (`plage.cpp`) : décor uni, chaume, bois et les villas du kit. Rochers et trottoirs ne reçoivent plus (dents de scie et taches vues par le lead dev).
- **Campagne et Grand Prix : rien ne change à l'écran.** Leur décor ne passe pas par les blocs (`LOTS_CARTES_SOCLE=false`) ; la ligne de `decor_carte.cpp` ne servira que le jour où il y passera.

**Lecture** : `shaders/scene.frag` (accroche de 3 lignes) et `MAT_AO` (`renderer.h`, 1 ligne) : l'occlusion multiplie l'ambiance et le reflet du ciel **seulement** ; soleil, ombres, lampes et phares restent calculés à chaque image (RG-03). Matériaux MAT_VTINT : occlusion atténuée (0,6).

**Banc** (`cartes_banc.cpp`) : gros plans `carte-ville-ao-*` (œil à 1,7 m, pied de façade) et `WFS_SANS_AO=1` (banc seul) pour comparer la même vue sans occlusion.

**À l'écran** : façades à l'ombre plus sombres en bas de la rue qu'en haut, dessous des arcades et des auvents plus sombres, trottoirs au pied des immeubles un peu plus sombres. Les façades au soleil changent peu (le soleil direct n'est pas touché, c'est voulu). De nuit, presque rien (ciel de lune très faible devant les lampes).

## Comment c'est vérifié
- **Test CPU** `test_fidelite_lumiere_precalcule`, 10 contrôles : sol dégagé 1,00 ; pied du mur 0,62 ; sous l'auvent 0,53 ; mur dégagé uniforme ; **rue de 28 m : bas de façade 0,81 contre 0,91 en haut** (nouveau) ; plancher ; végétation intacte ; reproductible ; durée.
- **66 tests CPU OK** (`build/test_*`).
- **Chargement (RG-04)**, Ville, PC : décor fixe préparé en 1,1 s (dont occlusion 0,77 s, grille 0,65 s, 12,7 Mo temporaires) + murs du kit 0,2 s ; avant WFS-269 : 0,33 s. Soit **+1,0 s sur PC**.
- **Coût en course (C)**, banc `perf` Ville, même machine (lavapipe) : **neutre** : main à jour Haute 832 et 789 ms / Basse 348 et 334 ms (2 passages) ; branche Haute 792 et 782 ms / Basse 334 et 337 ms. Écarts dans le bruit du GPU logiciel (±5 %). Aucun calcul ajouté par image.
- **Captures** : 83 paires avant / après (Ville, Corniche, jour et nuit) regardées : aucune bande, aucune tache, aucune façade dégagée assombrie en entier ; + 5 gros plans sans / avec occlusion (même binaire, même caméra) avec carte d'écart × 16.

## Limites et risques
- **Effet volontairement discret** : seule la lumière du ciel est touchée (au plus ≈ 6 % de luminance sur les façades à l'ombre). C'est la limite de l'option A choisie par le PO.
- **Chargement sur téléphone non mesuré** : +1,0 s sur PC, estimé à +2 à +3 s sur le téléphone, **à la limite du budget RG-04 (+3 s)**. À confirmer par l'utilisateur (🟡). Si c'est trop, réglage simple : immeubles du kit sans niveau 0 reçu, ou portée 20 m.
- 8 rayons par sommet : valeurs par paliers de 1/8 (lissées par l'interpolation entre sommets).
- À valider par l'utilisateur sur son téléphone (🟡).

## Captures pour le lead dev
Tableau complet (83 paires) : [`qa/pr/WFS-269/README.md`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/README.md), produit par `AGENT='Claude, Agent 0 developer' bash tools/qa/captures_pr.sh WFS-269 ville corniche`.

### Gros plans de l'occlusion (Ville, même caméra, même binaire : WFS_SANS_AO=1 / occlusion)

Œil à 1,7 m au bord opposé de la chaussée, visé : pied de la façade 15 m plus loin (banc `ville`, lieux `carte-ville-ao-*`). Écart × 16 : différence de luminance amplifiée 16 fois (blanc = 6 % d'écart ou plus).

| Gros plan | Sans occlusion | Avec occlusion | Écart × 16 |
|---|---|---|---|
| `carte-ville-ao-3-d` (jour, façade à l'ombre à droite) | ![sans](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/sans-ao-carte-ville-ao-3-d.jpg?raw=true) | ![avec](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avec-ao-carte-ville-ao-3-d.jpg?raw=true) | ![écart](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/ecart-x16-carte-ville-ao-3-d.jpg?raw=true) |
| `carte-ville-ao-1-d` (jour, rue du centre) | ![sans](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/sans-ao-carte-ville-ao-1-d.jpg?raw=true) | ![avec](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avec-ao-carte-ville-ao-1-d.jpg?raw=true) | ![écart](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/ecart-x16-carte-ville-ao-1-d.jpg?raw=true) |
| `carte-ville-ao-1-g` (jour, autre côté) | ![sans](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/sans-ao-carte-ville-ao-1-g.jpg?raw=true) | ![avec](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avec-ao-carte-ville-ao-1-g.jpg?raw=true) | ![écart](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/ecart-x16-carte-ville-ao-1-g.jpg?raw=true) |
| `carte-ville-ao-2-d` (jour, façades au soleil (effet faible : le soleil direct n'est pas touché)) | ![sans](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/sans-ao-carte-ville-ao-2-d.jpg?raw=true) | ![avec](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avec-ao-carte-ville-ao-2-d.jpg?raw=true) | ![écart](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/ecart-x16-carte-ville-ao-2-d.jpg?raw=true) |
| `carte-ville-ao-1-d-nuit` (nuit (ciel de lune : effet presque nul, lampes et phares inchangés)) | ![sans](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/sans-ao-carte-ville-ao-1-d-nuit.jpg?raw=true) | ![avec](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avec-ao-carte-ville-ao-1-d-nuit.jpg?raw=true) | ![écart](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/ecart-x16-carte-ville-ao-1-d-nuit.jpg?raw=true) |

### Paires avant (main) / après (branche)

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `carte-ville-virage-2` (façades à l'ombre à droite) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avant-carte-ville-virage-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/apres-carte-ville-virage-2.jpg?raw=true) |
| `carte-plage-casino-vue` (arcade du casino) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avant-carte-plage-casino-vue.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/apres-carte-plage-casino-vue.jpg?raw=true) |
| `carte-ville-casino-nuit` (nuit) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/avant-carte-ville-casino-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-269/qa/pr/WFS-269/apres-carte-ville-casino-nuit.jpg?raw=true) |

---
_Generated by [Claude Code](https://claude.ai/code/session_01UY1Vv2SSLKX4bCmA8vcJJc)_
