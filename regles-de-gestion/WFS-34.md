# WFS-34 — Arbres réalistes portés du jeu web
Épic : E2 · Priorité : 4.5 · Dépend de : WFS-06 ✅

## Besoin
Les arbres de la ville native (WFS-06, `src/world/ville.cpp`) sont simplifiés. Le jeu web a déjà des arbres réalistes :
on les porte au lieu de les redessiner (gain de temps et de jetons).

## Règles de gestion (valeurs de `waaps/waforspeed/js/world/life.js` ~250-690)
- RG-01 : atlas de feuillage 1024x1024 RGBA, 4 cases de 512 (feuillus, rameau de sapin, palme, éventail d'aiguilles/herbe), graine 4242.
- RG-02 : couleur sous les pixels transparents = couleur moyenne de la case (pas de liseré noir) ; mipmaps qui conservent la couverture.
- RG-03 : feuillage en cartes alpha découpées, double face, un seul matériau partagé ; tronc et branches en vraie géométrie.
- RG-04 : platane d'alignement (`DECOR.streettree`) : hauteur = h*1,4+1,2 ; grille 1,3x1,3 m au pied ; tronc clair `#a59a80`/`#6f6a55`, rayon 0,06+h*0,017, tronc 45 % de h, houppier cw 0,36 / ch 0,26 / cy 0,7, 6 bouquets + 11 cartes.
- RG-05 : feuillu de ville (`vegBroadleaf`) : écorce `#5c4c3c`/`#4a3b2e`, cw 0,3 / ch 0,3 / cy 0,65 ; bouquets 6 à 8 (3 en LOD) ; teintes tilleul [1,08 ; 1,12 ; 0,82] et [1 ; 1,08 ; 0,8] ; coeur `#2a4020`.
- RG-06 : mêmes générateurs pour les autres cartes (WFS-14) : sapin `vegFir`, pin `vegPine` (parasol sur plage), cyprès `vegCypress`, palmier `vegPalm`, chêne ; teintes `VEG_TINT`.
- RG-07 : LOD : au-delà de 200 m (`LOD_FAR_M`) ou latéral > RW+28 m, 3 à 5 fois moins de cartes, tronc à 5 côtés au lieu de 8.
- RG-08 : graine par arbre (mulberry32 sur hauteur, tranche, côté) : mêmes arbres à chaque lancement ; position/cadence actuelles de `ville.cpp` conservées.
- RG-09 : budget : la ville reste ≤ 300 appels de dessin (instanciation/regroupement par tronçon).

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Port C++ des générateurs en `src/world/vegetation.{h,cpp}` : `vegBroadleaf`, `vegFir`, `vegPine`, `vegCypress`, `vegPalm`, `vegLimb`, `vegCard`, `vegRibbon`, `VEG_TINT`, `BARK`, `CORE` ; appel depuis `decor.js` (`DECOR.tree`, `DECOR.streettree`).
- Atlas : NE PAS le redessiner. Exécuter `vegDraw*`/`makeFoliageAtlas` (canvas) dans le Chromium du banc de test, exporter le PNG tel quel vers `assets/world/feuillage.png` (script dans `tools/`, PNG commité).
- Le moteur gère déjà l'alpha découpé (MAT_MASK) et le double face : rien à ajouter au noyau.
- Rien à réutiliser côté ombres (WFS-07) ni vent (hors périmètre).

## Critères d'acceptation (sur captures du banc PC)
- Rue de ville : platanes avec tronc visible, branches et houppier feuillu irrégulier (plus de boule/cône simple).
- Contours du feuillage découpés, sans carré opaque ni liseré noir ; arbre vu de dos et de côté identique en qualité.
- Au loin, le feuillage ne s'éclaircit pas ; variation de teinte entre arbres voisins.
- Journal du banc : `mode exact : … 0 écart(s)` inchangé ; ≤ 300 appels de dessin ; fluidité non dégradée.

## Hors périmètre
Vent, ombres portées, sous-bois/haies/fougères, nouvelles cartes (WFS-14), arbres des cartes hors ville.
