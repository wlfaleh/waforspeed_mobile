# QA WFS-34 : Arbres réalistes portés du jeu web

**Verdict : VALIDÉ** (banc d'essai PC), 4e passage

- Construction regardée : commit `43e626e` (branche `feature/waforspeed-34`, synchronisée avec `main` qui contient WFS-07).
  Release `apercu-waforspeed-34` (cible `43e626edc7c9…`, texte « Branche feature/waforspeed-34, commit 43e626e », captures du 2026-11-03 00:41 UTC).
  Run Actions 41552953367 : réussi.
- Passages précédents : REFUSÉ sur `1420ffc` (défauts 1 et 2), `68ffdaf` (défaut 1), `deb85f2` (défaut 1 : cœur en volumes lisses à bord net).
- Correction annoncée : cœur du houppier dessiné avec la texture de feuilles découpée (même matériau alpha découpé que les cartes), assombri.
- Nouveau : ombres du soleil (WFS-07) actives ; route plus claire au soleil, attendu.

## Captures regardées

`trafic-cam0`, `trafic-cam1`, `trafic-choc`, `course-cam0-0/1/2`, `course-cam1-0`, `vitrine-p911-0`, `ombres-a`.
Houppiers agrandis ×4 à ×12 aux mêmes endroits que les passages précédents.

## Journal du job `banc-essai`

- `mode exact : 39 voitures comparées au jeu web, 0 écart(s)` : OK.
- `Ville : 899 immeubles, 222 lampadaires, 439 arbres` ; `Ville : 160 appels de dessin (25 morceaux)` : OK, ≤ 300 (RG-09), en baisse (184 avant).
- Aucun « Atlas de feuillage introuvable ».

## Suivi des défauts

| Défaut | Résultat |
|---|---|
| 1. Cœur du houppier visible comme volume lisse à bord net | **CORRIGÉ.** Agrandi ×10-12, le cœur porte maintenant la texture de feuilles : variation de tons, trous découpés laissant voir le ciel / l'immeuble, bord dentelé. Il se lit comme l'intérieur ombragé du houppier, plus comme un objet (`trafic-cam1` arbre de gauche, `course-cam0-2` arbres de gauche et de droite, `course-cam0-1`, `vitrine-p911-0`). |
| 2. Cartes de feuilles délavées / « givrées » | **Toujours corrigé.** Pas de plaque pâle. Quelques cartes vert clair vif côté soleil (`trafic-cam1` haut de l'arbre de gauche, `trafic-choc`) : éclairage direct, pas du délavé. |
| 3. (mineur) charpentières en « baleines de parapluie » | Inchangé, non bloquant. |

## Remarque mineure (non bloquante)

- `trafic-cam0`, arbre de droite (x ≈ 960-1000, y ≈ 150-190) : un lobe du cœur affleure au bord du houppier et reste un peu plus
  sombre et plus rond que le feuillage voisin. Texturé et découpé, il se lit comme une masse de feuillage à l'ombre : acceptable.
  Piste éventuelle plus tard : le rentrer légèrement pour qu'il ne touche pas le contour.

## Autres critères

| Critère | Résultat |
|---|---|
| Platanes : tronc, branches, houppier irrégulier | OK |
| Contours découpés, pas de carré opaque ni liseré noir | OK (cartes et cœur) |
| De dos / de côté même qualité | OK (`trafic-choc`, `course-cam0-2` vu de dessous, `vitrine-p911-0`) |
| Au loin, pas d'éclaircissement, variation de teinte | OK (`course-cam1-0`, `vitrine-p911-0`, `course-cam0-0`) |
| Ombres (WFS-07) avec les arbres | Aucune ombre en carré plein ; pas de bloc sombre anormal au pied des arbres (`ombres-a`, `course-cam0-1`, `vitrine-p911-0`). |
| Régressions | Aucune : voitures, route, immeubles, lampadaires, HUD normaux ; pas d'écran noir ni de couleur bizarre. |

Fusion autorisée.
