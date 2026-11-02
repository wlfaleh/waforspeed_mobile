# QA WFS-34 : Arbres réalistes portés du jeu web

**Verdict : REFUSÉ** (banc d'essai PC)

- Construction regardée : commit `1420ffc` (branche `feature/waforspeed-34`). Release `apercu-waforspeed-34`
  (cible `1420ffcd06e5…`, texte « Branche feature/waforspeed-34, commit 1420ffc »).
  Run Actions 41546811953 (push) : jobs `android`, `banc-essai`, `publication` verts.
- Référence AVANT : Release `apercu-waforspeed-32` (commit `f7e86cd`, anciens arbres boule + tronc). La Release `apercu`
  de `main` ne contient que l'APK (aucune capture), elle n'a pas pu servir de comparaison.

## Captures regardées

Les 22 PNG du banc : `course-cam0-0/1/2`, `course-cam1-0/1/2`, `trafic-cam0`, `trafic-cam1`, `trafic-choc`,
`vitrine-golf/p911/gallardo/m4-0/1/2`, `moteur-courbe`. Houppiers agrandis ×3 à ×4 (pixel par pixel) et différence
pixel par pixel avec la construction de référence.

Différences avant / après : limitées aux arbres (zone haute de l'image, 1,8 % à 11 % des pixels selon la capture) ;
`moteur-courbe` identique (0 pixel changé). Voitures, route, immeubles, lampadaires, HUD inchangés.

## Constats par critère

| Critère | Preuve | Résultat |
|---|---|---|
| Platanes avec tronc, branches et houppier feuillu irrégulier | `course-cam0-0` (arbre de droite), `course-cam0-1`, `trafic-choc` | OK sur le principe : tronc visible, empattement, 6 à 8 charpentières, houppier en bouquets de cartes de feuilles irrégulier. Plus de boule simple. |
| Contours découpés, sans carré opaque ni liseré noir | toutes les captures `course-*`, `trafic-*` | Contours des cartes découpés, pas de carré opaque ni de liseré noir autour des feuilles. **Mais** des masses noires anguleuses sont visibles (défaut 1). |
| Arbre de dos / de côté identique en qualité | `course-cam0-*` (de dos), `trafic-choc` / `vitrine-m4-2` (de côté) | Même qualité sous les deux angles, mêmes défauts 1 et 2. |
| Au loin, le feuillage ne s'éclaircit pas | `course-cam0-0` (fond de rue), `course-cam1-0`, `trafic-cam1` | OK : arbres lointains verts et pleins, seulement voilés par le brouillard comme les immeubles. |
| Variation de teinte entre voisins | `course-cam0-0`, `trafic-cam1` | OK : verts plus ou moins jaunes / foncés d'un arbre à l'autre. |
| Journal : `0 écart(s)` | job `banc-essai` | OK : `mode exact : 39 voitures comparées au jeu web, 0 écart(s)` ; `Ville : 899 immeubles, 222 lampadaires, 439 arbres`. Atlas chargé (aucun « Atlas de feuillage introuvable »). |
| ≤ 300 appels de dessin, fluidité | journal + code | Non vérifiable : le banc ne journalise ni appels de dessin ni temps d'image. Par lecture du code, 8 maillages par tronçon (dont 4 pour les arbres), regroupés : vraisemblable, à journaliser. |
| Aucune régression ailleurs | différence pixel, `moteur-courbe`, `vitrine-*` | OK. |

## Défauts (motif du refus)

1. **Masses noires à facettes qui « flottent » dans les houppiers** (le cœur sombre `CORE` `#2a4020`, icosaèdre).
   Elles apparaissent comme des polyèdres presque noirs, à arêtes nettes, posés à travers le feuillage, sur presque tous
   les arbres proches :
   - `trafic-cam0` : arbre de droite (x ≈ 980-1180, y ≈ 60-170), 4 blocs noirs nettement visibles ;
   - `course-cam0-2` : arbre de droite (x ≈ 900-1180, y ≈ 0-150), 3 blocs ;
   - `course-cam0-1` : arbre de droite (x ≈ 840-980, y ≈ 120-180) ;
   - `course-cam0-0` : arbre de droite (x ≈ 740-800, y ≈ 180-210) ;
   - `trafic-cam1` : arbre de gauche (x ≈ 130-300, y ≈ 50-130) ; `trafic-choc` : arbre de gauche, bloc à côté du lampadaire
     (x ≈ 290-340, y ≈ 115-150) ; `vitrine-p911-0` : arbre du centre (x ≈ 740-840, y ≈ 110-160).
   Le cœur doit rester caché dans la masse de cartes (au jeu web il ne se lit que comme de l'ombre au fond du houppier),
   pas se voir comme un objet.
2. **Cartes de feuilles délavées, presque blanches** (gris-vert pâle, ≈ RVB 160/176/152) sur le dessus et les bords de
   certains houppiers, qui donnent un aspect « givré » :
   - `course-cam0-2` : arbre de gauche, tout le dessus (x ≈ 380-470, y ≈ 130-160 : ~970 pixels pâles) ;
   - `course-cam1-1` : arbre de droite (dessus et bord gauche) ;
   - `course-cam1-2` : arbre de droite (haut gauche du houppier) ;
   - `trafic-cam0` : arbre de gauche (sous le houppier) ; `vitrine-p911-0` : arbre de gauche (x ≈ 500-600, y ≈ 0-90).
   Piste : le jeu web éclaire le feuillage en Lambert (`MeshLambertMaterial`, sans reflet) alors que le natif utilise le
   matériau PBR (rugosité 0,8) : reflet du ciel sur les cartes vues en rasant.
3. (mineur, pas bloquant) Les charpentières partent toutes d'un seul point en haut du tronc, droites, en « baleines de
   parapluie » (`course-cam0-2` arbre de droite, `course-cam1-1` arbre de gauche) ; et quelques cartes vues par la tranche
   forment de longues traînées vertes (`trafic-cam0` arbre de droite). À vérifier par rapport au jeu web.

## Pour la reprise

Corriger 1 et 2, puis relancer le banc ; je regarderai les nouvelles captures (mêmes vues). Ajouter si possible au journal
du banc le nombre d'appels de dessin de la ville pour vérifier RG-09.
