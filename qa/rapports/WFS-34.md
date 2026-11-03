# QA WFS-34 : Arbres réalistes portés du jeu web

**Verdict : REFUSÉ** (banc d'essai PC), 2e passage

- Construction regardée : commit `68ffdaf` (branche `feature/waforspeed-34`). Release `apercu-waforspeed-34`
  (cible `68ffdafeddb3…`, texte « Branche feature/waforspeed-34, commit 68ffdaf », captures du 2026-11-02 23:55 UTC).
  Run Actions 41548455588 (push) : jobs `banc-essai`, `android`, `publication` verts.
- Passage précédent : REFUSÉ sur `1420ffc` (défauts 1 et 2 ci-dessous).

## Captures regardées

Mêmes vues que la fois précédente : `trafic-cam0`, `trafic-cam1`, `trafic-choc`, `course-cam0-0/1/2`, `course-cam1-0/1/2`,
`vitrine-p911-0`, plus `moteur-courbe`. Houppiers agrandis ×2 à ×4 (pixel par pixel), comptage des pixels pâles
(vert-gris clair) et vert très sombre dans la moitié haute de chaque image.

## Journal du job `banc-essai`

- `mode exact : 39 voitures comparées au jeu web, 0 écart(s)` : OK.
- `Ville : 899 immeubles, 222 lampadaires, 439 arbres` ; `Ville : 184 appels de dessin (25 morceaux)` (186 au plus) : OK, ≤ 300 (RG-09).
- Aucun « Atlas de feuillage introuvable ».

## Suivi des défauts du 1er passage

| Défaut | Résultat |
|---|---|
| 2. Cartes de feuilles délavées / « givrées » | **CORRIGÉ.** Plus aucune plaque pâle : 0 à 6 pixels pâles par capture (contre ~970 sur le seul arbre gauche de `course-cam0-2` avant). Dessus des houppiers verts dans `course-cam0-2`, `course-cam1-1`, `course-cam1-2`, `trafic-cam0`, `vitrine-p911-0`. |
| 1. Masses sombres qui « flottent » dans les houppiers | **NON CORRIGÉ.** Le cœur est plus lisse et un peu plus vert, mais il se voit toujours comme des blocs pleins vert très foncé, à bord net, qui dépassent du feuillage. Voir ci-dessous. |
| 3. (mineur) charpentières en « baleines de parapluie », cartes vues par la tranche | Inchangé, non bloquant (`course-cam0-2` arbre de droite, `trafic-cam0` arbre de droite). |

## Défaut bloquant restant

1. **Cœurs du houppier visibles comme des objets** (blocs arrondis vert presque noir, contour net, sans texture de feuilles) :
   - `trafic-cam0` : arbre de droite (x ≈ 990-1150, y ≈ 60-170) : 4 blocs nettement détachés, dont 2 en haut du houppier
     qui sortent du feuillage ;
   - `course-cam0-2` : arbre de droite, blocs à x ≈ 910-950 / y ≈ 30-80 et x ≈ 1150-1190 / y ≈ 35-90, plus un en haut au centre (y ≈ 0-40) ;
   - `trafic-choc` : arbre de gauche, bloc collé au lampadaire (x ≈ 297-340, y ≈ 118-152) et un autre en haut (x ≈ 237-270, y ≈ 67-100) ;
   - `course-cam0-1` : arbre de droite (x ≈ 845-885, y ≈ 80-108 et x ≈ 940-965, y ≈ 95-115) ;
   - `trafic-cam1` : arbre de gauche, bloc sombre en haut à gauche du houppier (x ≈ 140-220, y ≈ 50-110).
   Au jeu web le cœur ne se lit que comme de l'ombre au fond du houppier. Pistes : le réduire / le rentrer pour qu'il reste
   entièrement couvert par les cartes, ou le rendre nettement plus clair et proche de la teinte des feuilles ombrées ; vérifier
   sa taille et sa position par rapport à `CORE` dans `life.js`.

## Autres critères

| Critère | Résultat |
|---|---|
| Platanes : tronc, branches, houppier irrégulier | OK |
| Contours découpés, pas de carré opaque ni liseré noir | OK pour les cartes (mais défaut 1) |
| De dos / de côté même qualité | OK, même défaut 1 sous tous les angles |
| Au loin pas d'éclaircissement, variation de teinte | OK (`course-cam1-0`, `course-cam1-2`, `trafic-cam1`) |
| Régressions | Aucune vue : voitures, route, immeubles, lampadaires, HUD normaux ; pas d'écran noir ni de couleur bizarre. |

## Pour la reprise

Corriger le défaut 1 uniquement (le 2 est réglé, ne pas revenir dessus), relancer le banc ; je regarderai les mêmes vues.
