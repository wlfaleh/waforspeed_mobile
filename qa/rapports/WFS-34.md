# QA WFS-34 : Arbres réalistes portés du jeu web

**Verdict : REFUSÉ** (banc d'essai PC), 3e passage

- Construction regardée : commit `deb85f2` (branche `feature/waforspeed-34`). Release `apercu-waforspeed-34`
  (cible `deb85f26129b…`, texte « Branche feature/waforspeed-34, commit deb85f2 », captures du 2026-11-03 00:08 UTC).
  Run Actions 41549740318 : jobs `banc-essai`, `android`, `publication` verts.
- Passages précédents : REFUSÉ sur `1420ffc` (défauts 1 et 2), REFUSÉ sur `68ffdaf` (défaut 1).
- Correction annoncée : cœur du houppier réduit (×0,7) et teinté comme le feuillage à l'ombre (moyenne de l'atlas × teinte × 0,45).

## Captures regardées

`trafic-cam0`, `trafic-cam1`, `trafic-choc`, `course-cam0-0/1/2`, `course-cam1-0/1/2`, `vitrine-p911-0`, `moteur-courbe`.
Houppiers agrandis ×3 à ×4 aux mêmes endroits que le 2e passage.

## Journal du job `banc-essai`

- `mode exact : 39 voitures comparées au jeu web, 0 écart(s)` : OK.
- `Ville : 899 immeubles, 222 lampadaires, 439 arbres` ; `Ville : 184 appels de dessin (25 morceaux)` : OK, ≤ 300 (RG-09).
- Aucun « Atlas de feuillage introuvable ».

## Suivi des défauts

| Défaut | Résultat |
|---|---|
| 2. Cartes de feuilles délavées / « givrées » | **Toujours corrigé.** Aucune plaque pâle dans les houppiers ; dessus des arbres bien verts. |
| 1. Cœur du houppier visible comme un objet | **AMÉLIORÉ, NON CORRIGÉ.** Il n'est plus vert presque noir (vert moyen-foncé), mais il reste lisible comme des volumes lisses, ombrés en dégradé, **sans texture de feuilles et à contour net**, qui ressortent des cartes ou dépassent du feuillage. |
| 3. (mineur) charpentières en « baleines de parapluie » | Inchangé, non bloquant. |

## Défaut bloquant restant

1. **Cœurs du houppier toujours visibles** (blocs lisses vert foncé, bord net, pas de feuilles) :
   - `trafic-cam0`, arbre de droite : 3 boules lisses bien détachées, x ≈ 1000-1030 / y ≈ 110-145, x ≈ 1070-1100 / y ≈ 65-95,
     x ≈ 1110-1135 / y ≈ 85-110 ;
   - `course-cam0-2`, arbre de droite : bloc x ≈ 920-960 / y ≈ 30-80 et bloc x ≈ 1150-1180 / y ≈ 40-70 qui dépasse du feuillage
     sur le fond d'immeuble, plus un en haut au centre (y ≈ 0-35) ;
   - `trafic-choc`, arbre de gauche : bloc en haut (x ≈ 245-275, y ≈ 75-100) et bloc près du lampadaire (x ≈ 300-335, y ≈ 120-150) ;
   - `trafic-cam1`, arbre de gauche : grand bloc lisse en haut à gauche du houppier (x ≈ 135-205, y ≈ 40-95), le plus visible de tous ;
   - `course-cam0-1`, arbre de droite : à peine visible désormais (x ≈ 850-880, y ≈ 140-165), acceptable seul.
   Le problème n'est plus la couleur mais la **forme lisse et le contour net** du cœur. Pistes : lui appliquer l'atlas de feuilles
   avec le même découpage alpha que les cartes (comme le cœur du jeu web, qui se lit comme du feuillage), ou le rentrer/réduire
   encore pour qu'il soit entièrement caché par les cartes sous tous les angles.

## Autres critères

| Critère | Résultat |
|---|---|
| Platanes : tronc, branches, houppier irrégulier | OK |
| Contours découpés, pas de carré opaque ni liseré noir | OK pour les cartes (mais défaut 1) |
| De dos / de côté même qualité | Défaut 1 sous plusieurs angles |
| Au loin, pas d'éclaircissement, variation de teinte | OK (`course-cam1-0`, `course-cam1-2`, `vitrine-p911-0`) |
| Régressions | Aucune : voitures, route, immeubles, lampadaires, HUD normaux ; pas d'écran noir ni de couleur bizarre. |

## Pour la reprise

Corriger le défaut 1 uniquement (ne pas toucher aux cartes ni aux appels de dessin), relancer le banc ; je regarderai les mêmes vues.
