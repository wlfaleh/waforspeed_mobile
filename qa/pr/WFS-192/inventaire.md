# WFS-192 — inventaire RG-01 des logos des 20 voitures (arwa-2)

Méthodes : **effacer** = zone de texture repeinte (couleur de la pièce voisine, ΔE2000 ≤ 2) ; **retirer** = triangles de la pièce de logo retirés ; **gardé** = pièce laissée (RG-10 : ouverture derrière), sa texture effacée. Données : `tools/logos.json` (notes de chaque zone et pièce) ; outil : `tools/effacer_logos.mjs`. Contrôle des trous : 100 points × 5 rayons par pièce retirée (% de rayons qui ne trouvent derrière ni face opaque ni face de l'endroit ; > 5 % : vue de près regardée, voir la ligne de la pièce).

Lecture : pour 12 voitures, l'inventaire détaillé fait à l'essai (tableau où / quoi / pièce ou texture / méthode / contrôle, avec les sondes de face pour les lettres et badges dont les rayons inclinés frôlent la surface) est repris tel quel, suivi de la ligne « mesures » du passage final depuis `main` ; pour la M4 et les 7 voitures du dernier lot, la liste vient des notes de `tools/logos.json`. Changements après l'essai : Elantra, texture 23 (étiquettes du moteur, invisibles capot fermé) laissée telle quelle ; Giulia, texture 22 : couleur prise sur la référence (ΔE2000 0,00) ; Golf, texture 8 zone 1 : sans référence (zone plus affichée) ; M4 : RG-12.

## a45
RG-09 : 25 matériaux = 25 de la source (mercedes-a45, primitives « Object_N ») ; triangles identiques matériau par matériau
(matériau 10 : 11 primitives, total 37450). Aucune texture de logo (6 textures : rétroviseurs, plaques) : tous les logos sont des pièces 3D.

| où | quoi | pièce | méthode | contrôle |
|---|---|---|---|---|
| calandre | étoile (branches chromées) | mat 16 [0, 6] + fond noir mat 10 [37444, 6] | retirer | sonde +z : grille (mat 10, 16, 17) vue de face, 0 trou ; trous.py faussé (pièces derrière la grille) |
| calandre | disque transparent sur l'étoile | mat 13 [0, 114] (verre, alpha < 1) | garder (transparent, rien de lisible) | à revoir sur les vues après |
| calandre | anneau chromé de l'étoile | fondu dans la barre chromée (mat 16, composante de 417 tri, x ±0,46) | garder (un cercle seul n'est pas une marque) | — |
| hayon | étoile (anneau + branches) | mat 10 [0, 218] | retirer | sonde -z : carrosserie (mat 1) face avant, 0 trou |
| hayon | « AMG » | mat 10 [18+…] plage contiguë 175 tri | retirer | sonde : carrosserie, 0 trou |
| hayon | « A 45 » | mat 10, 139 tri | retirer | sonde : carrosserie, 0 trou |
| caches de roue ×4 | étoile (6 tri chacune) | mat 16 [109], [374], [658], [976] ×6 | retirer (le disque chromé du cache reste) | trous.py 100 %, 0 trou |
| aile avant droite | « TURBO » | mat 9 entier (638 tri, maillage retiré) | retirer | trous.py 95 %, sonde -x : aile (mat 1) |
| aile avant gauche | « TURBO » | mat 10, 265 tri | retirer | 99 % |
| ailes avant | « 4MATIC » | mat 23 entier (875 tri, maillage retiré) | retirer | sonde ±x : aile (mat 1) face avant, 0 trou (trous.py 66 % « envers » : normales des lettres inversées) |

Pas de logo trouvé : volant (pas de pièce séparée), étriers (rouges unis), intérieur.
LOD : 112269 triangles (niveau 1 à 58,6 % : c'est ce que donne alleger_voiture.mjs, à comparer au .lod d'origine).

## Questions
- Calandre : le disque transparent (mat 13) et l'anneau chromé restent ; à juger sur les vues après (si une forme d'étoile se lit encore, retirer aussi mat 13 [0, 114]).

Passage final : mesures : 0 zone(s) mesurée(s), ΔE2000 max 0.00 ; taille 6 920 268 → 6 793 020 octets ; aucune alerte

## abarth
RG-09 : 57 matériaux = 57 de la conversion (abarth-500, matériaux nommés : CAR_Loghi, INT_Logo_500, INT_Volante…, plus les 2 plaques) ;
triangles identiques matériau par matériau.

| où | quoi | texture / pièce | méthode | contrôle |
|---|---|---|---|---|
| calandre | écusson au scorpion + mot ABARTH (barre chromée « moustache » + écusson, mat 13 [0, 202]) | pièce gardée (RG-10 : sonde +z, sous-caisse 5 à 7 cm derrière : ouverture) ; tex 11 (couleur) zones intérieur + texte → noir de la bordure #2f2f31 ; tex 12 (normale) mêmes zones → plat | effacer | ΔE2000 0,55 contre la bordure carbone (grain σ 12 : écusson uni noir dans un cadre carbone) |
| hayon | écusson au scorpion | mat 13 [565, 145] | retirer | sonde -z : carrosserie (mat 9) 0 trou ; trous.py 85 % ≤ 15 mm, 0 % trou |
| ailes | petits écussons tricolores ×2 | mat 13 [431, 67], [498, 67] | retirer ; texture effacée (#efeff9, zone plus affichée) | 99 %, 0 trou |
| barre chromée du hayon | « 500 » | tex 11 / 12 rect | effacer : chrome autour #e9eaed ; normale plate | ΔE 1,32 |
| jantes (3 textures : nette, floue, floue alpha) | scorpion du centre | tex 2, 3, 4, disque [0,499 ; 0,499 ; r 0,057] | effacer : gris de l'anneau du cache #3e3e3e | ΔE 0,33 |
| volant | écusson au centre (mat 24, 160 tri) | pièce gardée (RG-10 : 16 % de rayons vers le moyeu creux) ; tex 19 entière → plastique de la planche autour #39343a | effacer | « comparer » mat 24 ↔ mat 16 : ΔE 0,00 |
| planche de bord | « 500 » (mat 50, 8 tri) | pièce | retirer | 92 %, 3 % trou |
| écran de la planche | « ASSETTO CORSA » (jeu source) | tex 26 (mat 35) | effacer #040000 | ΔE 1,16 |
| ancienne plaque arrière (cachée par la plaque de WFS-158, sonde : 25/25 sur la plaque neuve) | « ASSETTO CORSA » + logo du jeu | tex 32 (mat 52) entière | effacer : blanc uni | texture entière |
| atlas des plastiques | mot ABARTH dans les bandes rouges (uv non utilisés par le modèle) | tex 5, 2 rect | effacer : rouge de la bande | ΔE 0,21 / 0,48 |

Gardé : bande « AIRBAG » (tex 16), grille du levier (tex 24), « SPORT / SHIFT UP » de l'écran (tex 23) : pas des marques.
Hors zones (RG-07 a) : moyenne ≤ 0,052, 99e centile ≤ 1,56. abarth.wfm 5009676 → 4933827 (e8959d61ca6a6e17), lod e023b4830344265f ; 2e passe : rien à faire.

## Questions
- Pneus (tex 1) : marque en relief peint sur tout le flanc (contraste faible : 22 à 58 autour de 40) ; le flanc a un grain fort (σ 7 à 16) :
  un anneau uni ferait un flanc lisse. Non traité ; proposition : laisser (illisible au téléphone) ou accepter un flanc uni.
- Grain : écusson de la calandre noir uni dans un cadre carbone ; fond du cache de jante uni (l'outil n'ajoute pas de bruit).

Passage final : mesures : 12 zone(s) mesurée(s), ΔE2000 max 19.42 ; hors zones : moyenne max 0.100, 99e centile max 1.56 ; taille 5 009 676 → 4 933 827 octets ; alertes : zone 1 : grain σ 4.70 > 1,5 (l'uni ferait une tache lisse) | zone 1 : grain σ 3.68 > 1,5 (l'uni ferait une tache lisse) | zone 1 : grain σ 3.71 > 1,5 (l'uni ferait une tache lisse) | zone 1 : grain σ 11.88 > 1,5 (l'uni ferait une tache lisse) | zone 2 : grain σ 6.02 > 1,5 (l'uni ferait une tache lisse) | zone 1 : grain σ 12.02 > 1,5 (l'uni ferait une tache lisse) | zone 2 : grain σ 12.02 > 1,5 (l'uni ferait une tache lisse) | zone 3 : grain σ 12.02 > 1,5 (l'uni ferait une tache lisse)

## compass
RG-09 : 15 matériaux = 13 de la source (jeep-compass, une primitive « Object_N » par matériau) + 2 plaques ; triangles identiques.
Une seule texture (la plaque) : tous les logos sont des pièces 3D dans les grands maillages (00_-_CHROME, 00_-_PLASTIC_BLACK).

| où | quoi | pièces | méthode | contrôle |
|---|---|---|---|---|
| capot (au-dessus de la calandre) | lettres « Jeep » | mat 2 : [15608, 244], [15852, 243], [16643, 152], [16795, 239] | retirer | sonde +z : peinture (mat 0) face avant, 25/25 |
| hayon | lettres « Jeep » + leur fond noir | mat 2 : [2098, 112], [2210, 120], [2330, 59], [2389, 106] ; mat 7 [2420, 170] | retirer | sonde -z : peinture, 25/25 |
| hayon, à gauche | « 4x4 » | mat 2 : [3394, 159], [3553, 158], [3711, 104] | retirer | peinture, 25/25 |
| hayon, à droite | « LIMITED » | mat 2 : 7 lettres (2495 à 3327) | retirer | peinture, 25/25 |
| portes avant (bas) | nom en lettres chromées (9 lettres, « TRAILHAWK » probable) + fonds noirs | mat 2 : 11 plages à droite, 9 à gauche ; mat 7 : 7 + 7 petites pièces | retirer | sonde ±x : peinture, 25/25 ; trous.py : 0 à 8 % (rayons inclinés au pli de la porte) |

Gardé : calandre à 7 fentes (forme de la face avant, pas un logo ni un mot) ; caches de roue unis (disque de 22 tri, pas de logo) ;
anciennes lettres de plaque (mat 3, 12) cachées derrière la plaque de WFS-158.
Petites pièces couleur caisse (mat 0 [264, 50], [314, 33], [1626, 2]) dans les « 4x4 » / « LIMITED » : gardées, à regarder sur les vues après.
compass.wfm 8453203 → 8306731 (0b10f1bde51fc97e), lod c187a8957e8842b2, 12206 triangles retirés ; 2e passe : rien à faire.

## Questions
- La calandre à 7 fentes est la signature de la marque (forme, pas un logo) : hors de ce ticket (WFS-193, retouches de forme).

Passage final : mesures : 0 zone(s) mesurée(s), ΔE2000 max 0.00 ; taille 8 453 203 → 8 306 731 octets ; aucune alerte

## corsa
RG-09 : 17 matériaux = 17 de la source (opel-corsa, une primitive par matériau, dont « plate » d'origine) ; triangles identiques.
Une texture (plaque) : logos en pièces 3D (matériau « chrome », mat 3).

| où | quoi | pièces | méthode | contrôle |
|---|---|---|---|---|
| calandre | éclair + anneau chromés | mat 3 [0, 247] | retirer (le disque noir du badge, mat 2, reste : badge uni) | sonde +z : disque noir et grille (mat 2, 11) de face, 0 trou |
| hayon | éclair chromé | mat 3 [10677, 131] | retirer | sonde -z : disque noir (mat 7) et caisse (mat 4) de face ; même 2 rayons « dos » qu'avant (coins de la grille) |
| hayon | anneau chromé | mat 3 [10425, 252] | garder (RG-10 : sans lui, un rayon de plus voit l'envers du creux) ; un cercle seul n'est pas une marque | — |
| hayon, à gauche | « CORSA » + symbole | mat 3 [9750, 675] | retirer | sonde : caisse (mat 4), 0 trou ; trous.py 100 % |
| caches de roue ×4 | éclair + anneau | mat 3 : [515, 270], [247, 268], [1185, 316], [1750, 240], [859, 326], [1501, 249] | retirer (le disque noir du cache reste) | sonde ±x : disque noir (mat 7), 0 trou ; trous.py 0 à 4 % |

corsa.wfm 4866876 → 4834212 (6c0a519256795bab), lod 56992d532910c2bf ; 2e passe : rien à faire.

## Questions
- aucune.

Passage final : mesures : 0 zone(s) mesurée(s), ΔE2000 max 0.00 ; taille 4 866 876 → 4 834 212 octets ; aucune alerte

## elantra
RG-09 : 27 matériaux = 26 de la source (hyundai-elantra-n, matériaux nommés : Badge, Callipers, Wheel…) + 1 plaque ; triangles identiques.

| où | quoi | texture / pièce | méthode | contrôle |
|---|---|---|---|---|
| capot | logo H en relief | mat 0 [9, 166] | retirer | sonde +z : peinture (mat 11), 0 trou ; trous.py 98 % |
| coffre | logo H en relief | mat 0 [262, 158] | retirer | peinture, 0 trou ; 99 % |
| coffre | lettres « ELANTRA » (7 décalques) | mat 0 [248, 14] | retirer | peinture, 0 trou ; 100 % |
| coffre, à droite | « N » | mat 0 [243, 5] | retirer | peinture, 0 trou |
| bouclier avant | « N » | mat 0 [6, 3] | retirer | sonde : bouclier (mat 13, 18) |
| volant | logo H (décalque) | mat 0 [426, 20] | retirer | 96 %, 4 % |
| étriers avant | décalque (barres rouge et noire) | mat 24 [0, 2], [2, 2] (maillage vidé, retiré) | retirer | 88 % |
| atlas des badges | H, N, ELANTRA, N du panneau gris (uv plus affichés) | tex 0 (couleur, RGBA) + tex 1 (normale) | effacer RVB (#222222 / #7f7f7f) ; normale plate | zones plus affichées (ΔE sans objet) |
| intérieur | « BOSE », 2 logos N, « ELANTRA » | tex 14 (couleur) + tex 15 (normale) | effacer (#272727, #171717, #282828) ; normale plate | ΔE 0,00 |
| moteur | grand et petit logo N, étiquette HYUNDAI / Shell Helix | tex 23 (couleur) | effacer #262626 | ΔE 0,00 |
| moteur | relief N ×2 et « TURBO » du cache | tex 24 (normale) | plat | — |
| caches de roue | logo N (12 triangles, 3 par roue) | tex 34 (couleur) #1b1b1b, tex 35 (normale) plate, tex 36 (métal / rugosité) #ffff1b | effacer | ΔE 0,31 |
| pneus | « MICHELIN PILOT SPORT » et marquages | tex 34, anneau r 0,2305 à 0,3789 (flanc uni à 36) | effacer #242424 (zone « anneau », ajout à l'outil) | ΔE 1,89 |
| texture non utilisée | « ELANTRA N » | tex 18 | effacer (blanc) | — |

Gardé : « N » au centre du compte-tours (tex 33 : rapport de boîte « point mort »), grille en chevrons (tex 27, 28), « NFC » (tex 3),
petites rondelles près des vitres (décalques ronds, pas une marque), ventilateur (disque de 16 tri derrière le bouclier).
Hors zones : moyenne ≤ 0,175 ; 99e centile 3,18 sur tex 23 (moteur, sous le capot : invisible) ; autres ≤ 1,6.
elantra.wfm 6017928 → 5916257 (9c93c4f9436d9a40), lod 92e1e638e4519a47 ; 2e passe : rien à faire.

## Questions
- tex 0 (RGBA) : l'outil repeint le RVB mais garde l'alpha : la silhouette des logos reste dans le canal alpha (zones plus affichées).
  Proposition : option « alpha » de zone (0 ou 255) dans l'outil, si le lead dev le demande.
- tex 23 : 99e centile 3,18 (> 3) au réencodage JPEG q88 (étiquettes nettes) ; la texture est sous le capot. Garder q88 ou demander q92 au PO.

Passage final : mesures : 12 zone(s) mesurée(s), ΔE2000 max 22.70 ; hors zones : moyenne max 0.057, 99e centile max 1.94 ; taille 6 017 928 → 5 925 902 octets ; alertes : zone 1 : grain σ 3.98 > 1,5 (l'uni ferait une tache lisse) | zone 2 : grain σ 1.99 > 1,5 (l'uni ferait une tache lisse)

## gallardo
RG-09 : source `lambo-gallardo` : 16 matériaux = les 16 premiers du .wfm, nombre de triangles égal pour les 16 (16 et 17 = plaques WFS-158). Une pièce par matériau dans la source (maillages fusionnés) : les logos sont repérés dans le matériau 5 (D_Body_33 MAT_Details_EXT) par leur position 3D et leur zone de texture.

| Où | Pièce / texture | Matériau / rang | Méthode | Pourquoi / contrôle |
|---|---|---|---|---|
| Capot avant : bouclier Lamborghini | pièce [1093,162] du mat 5, texture 4 (zone bouclier) + normale 5 | mat 5, tex 4 et 5 | **garder la pièce, effacer** : plaque noire brillante unie #303030 (albédo 0,03), relief plat | RG-10 : 48 % des rayons ne trouvent aucune face derrière (carrosserie ouverte sous le badge) ; conseil de l'expert (cache de radar). ΔE « zone ↔ anneau » 23 sans objet (plaque voulue, voisin = carrosserie d'un autre matériau, peinte au garage). La silhouette du bouclier reste (sans taureau ni texte). |
| Capot arrière : lettrage « Lamborghini » | pièces [196,897], [1255,1], [1608,985] du mat 5 | mat 5 | **retirer** | rayons : 99 %, 97 % de faces opaques ≤ 15 mm ; le triangle isolé [1255,1] : 40 % sans face (1 triangle, à regarder de près) |
| Bas de caisse G et D : « LP 560-4 » | pièces [0,98], [98,98] (lettrage) et [2593,68], [2661,68] (fond blanc du badge) | mat 5 | **retirer** | lettrage 99 % ; fonds 64-78 % opaques, 12-20 % sans face (bord inférieur du bas de caisse, vu par-dessous seulement) |
| Caches des 4 roues : bouclier | texture 18, 2 zones (le bouclier sans les coins de l'atlas) | mat 13 (Tire_Hub), tex 18 | **effacer** #1c1c1c | couleur de la jante autour du cache (zone sombre 28 de l'atlas où sont les rayons, mesurée par l'aire 3D) : ΔE2000 0,00 contre cette référence, grain σ 0,06 |
| Étriers : « Lamborghini » | texture 6, 1 zone | mat 7 (Tire_Brake) | **effacer** « auto » | ΔE2000 0,00 (anneau), grain 0,26 |
| Tableau de bord : bouclier, « Lamborghini », « GALLARDO » | texture 7 (3 zones) + normale 8 (bouclier et lettrage en relief) | mat 8 (Details_INT) | **effacer** : bouclier « auto » (panneau), lettrage #232323 (bandeau 35), nom #ababab (bandeau 171) | ΔE2000 0,00 contre la surface voisine (référence), grain 0,11 / 0,41 / 1,95 (le 1,95 : bruit JPEG au bord des lettres, bandeau de 2 cm dans l'habitacle) ; carte métal / rugosité 9 vide à ces endroits |
| Moteur : « Lamborghini » sur la plaque | texture 10, 1 zone | mat 9 (Details_Engine) | **effacer** #bdbdbd (gris de la plaque) | ΔE2000 0,00 contre la plaque ; normale 11 et carte 12 vides à cet endroit |

Hors zones (RG-07 a) : tex 18 moyenne 0,089 / 99e centile 1,80 ; tex 6 0,000 / 0,00 ; tex 7 0,355 / 2,96 ; tex 10 0,004 / 0,24 ; tex 4 0,041 / 0,45 (toutes ≤ 0,5 et ≤ 3).
Rejouable (RG-08) : 2e passage « rien à faire », .wfm 74d15cce1a154c17, .lod 1d0e780d7ea48b02 identiques.
Gardé tel quel : flanc des pneus (normale 19 : sculpture, aucun texte vu), textures 0-3 (capote, détails, feux : aucun logo).

## Questions
- Bouclier du capot avant gardé en plaque noire unie (sa silhouette reste) : acceptable pour le PO, ou faut-il une autre forme (logo inventé, RG-02) ?

## Changements d'outil utilisés dans l'atelier (à reporter dans tools/effacer_logos.mjs si gardés)
- `zones[].couleur` : couleur propre à une zone (sinon celle de la texture).
- `zones[].reference` : [u0, v0, u1, v1] : surface voisine à imiter pour la mesure ΔE et le grain quand l'anneau de 3 % tombe sur d'autres pièces de l'atlas.

Passage final : mesures : 8 zone(s) mesurée(s), ΔE2000 max 23.26 ; hors zones : moyenne max 0.355, 99e centile max 2.96 ; taille 4 656 240 → 4 567 261 octets ; alertes : zone 3 : grain σ 1.95 > 1,5 (l'uni ferait une tache lisse)

## giulia
RG-09 : source `alfa-giulia` : 66 matériaux, nombre de triangles égal pour les 66 (64 et 65 = plaques WFS-158). Un prim par matériau ; pièces repérées par amas connexes.

| Où | Pièce / texture | Matériau / rang | Méthode | Pourquoi / contrôle |
|---|---|---|---|---|
| Coffre : écusson Alfa Romeo ; calandre : écusson ; ailes avant : trèfles Quadrifoglio ; 4 caches de roue : écussons | 8 pièces du mat 45 (EXT_logo), atlas tex 37 + normale 38 | mat 45, tex 37 / 38 | **garder les pièces, effacer l'atlas** : uni noir #272727 (albédo 0,02 = jante, matériau 58), relief plat (u < 0,945) | RG-10 : coffre 36 %, trèfles 32-55 % de rayons sans face (carrosserie ouverte derrière) ; calandre et caches : 0 % mais creux de 4 à 15 cm (grille, moyeu) : gardés aussi, même atlas. ΔE2000 zone ↔ jante 1,32. Restent des disques / triangles noirs unis sans dessin. |
| Coffre : lettrage « Giulia » | mat 47 (EXT_badge, 6 lettres, 389 triangles) | mat 47 | **retirer** | 86 % opaque ≤ 15 mm, 1 % sans face |
| Centre du volant : écusson | mat 36 (INT_logo) gardé, tex 31 + normale 26 | mat 36, tex 31 / 26 | **garder, effacer** #242424 | creux de 13 cm derrière (40 % des rayons > 15 mm) ; ΔE2000 0,00 contre le cuir du volant (matériau 21, albédo 0,018) |
| Dossiers des sièges : écussons brodés | mat 28 (INT_logo_seat, 2 quads à masque) | mat 28 | **retirer** | 100 % opaque ≤ 15 mm (le siège derrière) |
| Seuils de porte : « Alfa Romeo » | mat 27 (INT_threshold, 2 décalques à masque) | mat 27 | **retirer** | décalque posé sur le seuil (le seuil se voit déjà à travers ses parties transparentes) ; les rayons voient l'envers d'une face à 2 mm (normales du décalque retournées) |
| Portes : badges « harman/kardon » | mat 9 (INT_decals_chrome, 2 quads) | mat 9 | **retirer** | 99 % opaque ≤ 15 mm |
| Compteurs : « Alfa Romeo », trèfle ; « harman/kardon » | texture 7 (PNG à masque) 3 zones **rendues transparentes** + normale 6 | mat 10 (INT_decals) | **effacer (transparence)** | décalques à masque alpha : repeindre laisserait la silhouette ; le reste des cadrans (chiffres, voyants) ne change pas |
| Écran central : écusson | texture 22, 1 zone | mat 25 (display) | **effacer** « auto » (noir de l'écran) | ΔE2000 0,39 contre le fond noir voisin (référence), grain 0,03 |
| Étriers : « Alfa Romeo » en relief | normale 44, 1 zone (la couleur 43 n'a pas le lettrage) | mat 57 | **effacer** (normale plate) | — |
| Flancs des pneus : « MICHELIN » (2), bibendums, « PILOT SPORT », dimensions, « Total Performance » (2) | texture 45 (10 zones) + normale 46 | mat 59 (tyre) | **effacer** #1b1b1b (gris du flanc) | ΔE2000 0,31, grain 1,37 ; contraste d'origine ΔL* ≈ 10 (lettres L* 3 sur fond L* 13) : peu visible, mais effacé par prudence |

Hors zones (RG-07 a) : tex 22 0,050 / 1,05 ; tex 7 0,000 / 0,00 ; tex 45 0,003 / 0,16 (tex 37 et 31 : texture entière).
Rejouable (RG-08) : 2e passage « rien à faire », .wfm 44f22aea9266d20e, .lod 2ddd8f9b4a7adfad identiques.
Gardé tel quel : mat 50 (EXT_logo_AT dans les phares) : n'échantillonne que le bord droit de la tex 41 (u 0,95-1), aucun dessin lisible ; la tex 41 (grand écusson) reste dans le fichier sans être vue. Disques (tex 47 : boulons, pas de texte).

## Questions
- Écussons gardés en disques / triangles noirs unis (coffre, calandre, ailes, caches) : acceptable, ou un logo inventé (RG-02) ?

## Changement d'outil utilisé dans l'atelier
- `zones[].transparent: true` (texture PNG à canal alpha d'un matériau MAT_MASK) : alpha 0 dans la zone, le décalque disparaît ; arrêt si la texture n'a pas d'alpha. Plus `zones[].couleur` et `zones[].reference` (voir gallardo.md).

Passage final : mesures : 13 zone(s) mesurée(s), ΔE2000 max 1.32 ; hors zones : moyenne max 0.049, 99e centile max 1.05 ; taille 6 529 929 → 6 264 781 octets ; aucune alerte

## golf
RG-09 : 21 matériaux dans le .wfm = 19 matériaux de la source (golf-8, une primitive « Object_N » par matériau) + 2 plaques de WFS-158 ;
nombre de triangles identique matériau par matériau ; les noms de la source sont génériques : pièces repérées par l'uv et la position.

| où | quoi | texture / pièce | méthode | contrôle |
|---|---|---|---|---|
| calandre et hayon | rondelle VW (disque en relief, logo peint) | tex 8 (mat 10) et sa copie tex 10 (mat 13, 14), disque [0,835 ; 0,764 ; r 0,072] | effacer : disque sombre #101010 (pièce gardée : un disque uni) | ΔE2000 0,00 contre le fond sombre autour |
| hayon | lettres « GTI » en relief (6 plages, 190 tri, mat 10) | pièces | retirer ; texture effacée aussi (#949494, zone plus affichée) | sonde -z en 7 points : carrosserie (mat 4) face avant 1 à 3 mm derrière ; trous.py faussé (lettres creuses, flancs) |
| calandre | badge « GTI » plat (plaque sombre) | tex 8 / 10 rect | effacer #101010 | ΔE 0,60 / 0,00 |
| ailes avant | « GTI » sur la bande rouge | tex 8 / 10, 2 rect | effacer #78100e | ΔE 0,00 à 0,78 |
| phares | « IQ. » ×2, « IQ.LIGHT » ×2 | tex 8 / 10, 4 rect | effacer (gris de l'onglet, rouge de la bande) | ΔE 0,29 à 0,63 |
| feux arrière | « IQ.LIGHT » ×2 | tex 8 / 10, 2 rect | effacer #310101 | ΔE 0,00 |
| caches de roue | rondelle VW | tex 2 (mat 2) + carte métal / rugosité tex 3 + copie tex 11 (mat 15, non affichée), disque [0,095 ; 0,852 ; r 0,092] | effacer : noir du moyeu #101010 (voisins réels : 379 triangles du moyeu, médiane #111111) ; métal / rugosité : #ffff1a du moyeu | ΔE 0,00 contre le noir du moyeu |
| volant | rondelle VW du centre (le mot AIRBAG reste) | tex 9 (mat 12), disque | effacer « auto » | ΔE 0,00 |
| pneus | marque et dimensions peintes sur le flanc | tex 1 (mat 1), anneau r 0,385 à 0,465 | effacer : flanc uni #1e1e1e (zone « anneau », ajout à l'outil) | ΔE 0,00, grain σ 0,05 |

Hors zones (RG-07 a) : moyenne ≤ 0,069, 99e centile ≤ 1,68 sur les 7 textures. Tableau de bord (tex 6), freins (tex 0) : rien de lisible.
Alertes restantes : grain σ > 1,5 sur des références étroites (bords d'éléments de l'atlas, pas un vrai grain : les éléments sont unis) ;
zone 1 de tex 8 ΔE 2,50 contre une bordure de 2 px (zone plus affichée).

golf.wfm 5389046 → 5231594 octets (f027aaf0d774f091), golf.lod 94db2933f17e1eac ; 2e passe : mêmes octets.

## Questions
- aucune bloquante.

Passage final : mesures : 26 zone(s) mesurée(s), ΔE2000 max 29.72 ; hors zones : moyenne max 0.069, 99e centile max 1.68 ; taille 5 389 046 → 5 231 594 octets ; alertes : zone 1 : grain σ 2.37 > 1,5 (l'uni ferait une tache lisse) | zone 1 : grain σ 3.96 > 1,5 (l'uni ferait une tache lisse) | zone 4 : grain σ 1.76 > 1,5 (l'uni ferait une tache lisse) | zone 1 : grain σ 2.47 > 1,5 (l'uni ferait une tache lisse)

## gt86
RG-09 : source `toyota-gt86` : 13 matériaux = les 13 premiers du .wfm, triangles égaux (13 et 14 = plaques WFS-158). Une seule texture (la plaque) : tous les logos sont en relief.

| Où | Pièce | Matériau | Méthode | Contrôle |
|---|---|---|---|---|
| Hayon : emblème Toyota | Object_3 [0,1472] | mat 1 (gris 0,38) | **retirer** | 77 % opaque ≤ 15 mm, 3 % sans face |
| Calandre : emblème Toyota | Object_3 [1472,287] | mat 1 | **retirer** | 87 %, 2 % sans face |
| Ailes avant : badges « 86 » (G et D) | Object_6 [0,684] (tout le matériau 4) | mat 4 | **retirer** | par côté : 99 % et 98 % opaques, 0 % sans face |

Mesh vidés retirés (mat 1 et mat 4 : plus aucun appel de dessin). Rejouable : 2e passage « rien à faire », .wfm 4acd857dc91434ac, .lod 8fe2eb0c306e790e.
Gardé tel quel : jantes (Object_58-65, pas de cache à logo séparé, aucun logo vu), volant (pas d'emblème modélisé).

Passage final : mesures : 0 zone(s) mesurée(s), ΔE2000 max 0.00 ; taille 5 391 844 → 5 288 264 octets ; aucune alerte

## m4
- RG-09 : source ↔ `.wfm` : 51 matériaux des deux côtés, rangs et nombres de triangles identiques matériau par matériau ; pièces repérées par leur nom source.
- Coffre, rondelle (disque `Cylinder001` #538, matériau 28 ; fond #787, dans le matériau 5 ; cerclage chromé #786, matériau 27) : **retirés** ; rayons : 2 % / 2 % de trou, la peinture du coffre est 13 à 14 mm derrière (logement de l'emblème) ; vue de près : coffre lisse.
- Capot, rondelle (disque `Cylinder002` #574, matériau 29) : **retirée** (rayons 0 % de trou, peinture 4 mm derrière). Cerclage chromé `Cylinder002` #790 (matériau 6) : **gardé** (RG-10 : 33 % des rayons passent dans le jour entre capot et calandre) ; un anneau chromé uni, sans marque.
- Calandre, badge « M » tricolore (`Frontlogo`, matériaux 30 à 34) : **retiré** ; la grille continue derrière (1 % de trou, grille 23 à 69 mm plus loin) ; vue de près : grille entière.
- Coffre, monogramme « M4 » (`rear_logo`, matériaux 30 à 34) : **retiré** (0 % de trou, coffre 5 mm derrière).
- Ailes avant, badges « M » des ouïes (`sidelogos`, matériaux 30 à 34) : **retirés** ; 40 % des rayons entrent dans l'ouïe, déjà ouverte et sombre dans le modèle ; vue de près : ouïe inchangée autour, seul le petit badge disparaît.
- Étriers, 4 décalcomanies « M » (`Brake` … `Brake003`, matériau 25, texture 6) : **retirées** (0 à 15 % : l'autocollant déborde du bord de l'étrier, on voit le disque derrière, comme sur un étrier sans autocollant) ; vue de près : rouge de l'étrier continu. Texture 6 (logo M), sans autre usage : **mise en uni** (aucun logo ne reste dans le fichier).
- Caches des 4 roues (`tire06` … `tire012`, matériau 22) : texture 4 (image de la rondelle BMW, entière) **effacée** en uni `#000000` = albédo de la jante (matériau 21, couleur de base 0) : ΔE2000 0,00 ; métal 0,61 / rugosité 0 gardés.
- Centre du volant (`Steering Wheel` #1063, matériau 41) : **gardé** (RG-10 : 38 % des rayons voient l'envers du volant) ; RG-12 (décision du PO) : sa texture de couleur est retirée (4 → −1) et sa couleur de base devient celle du tour du volant (matériau 9, albédo 0,035) : ΔE2000 0,00 ; métal et rugosité gardés. La vue Cockpit ne dessine pas ce volant (capture du banc `cockpit` identique avant / après).
- Gardé, sans marque lisible : bandes tricolores des sièges, ceinture et volant (couleurs sans lettre), écran central (titre de chanson), plaque du Texas (texture 7, sans matériau : rien ne l'utilise).
- Sortie de l'outil (depuis l'original de `main`) : texture 4 : 92 183 → 6 410 octets (`889cd01b527ee389`) ; texture 6 : 1 150 → 278 octets ; matériau 41 retouché (RG-12) ; 10 maillages touchés, 6 vidés et retirés du fichier ; `.lod` régénéré ; 2e passage : « rien à faire ».
- Budget : 51 → 45 maillages (−6 appels de dessin), 117 182 → 112 554 triangles, 133 087 → 125 080 sommets (ceux des maillages retirés), 6 774 689 → 6 376 212 octets.

## model3
RG-09 : **pas de source locale** : pièces repérées par leur position dans le .wfm (amas connexes au centre avant / arrière) et par les textures ; boîtes notées dans model3.json. 37 matériaux (36 = plaque WFS-158).

| Où | Pièce (boîte) | Matériau | Méthode | Contrôle |
|---|---|---|---|---|
| Coffre : emblème « T » | [2816,704] (x ±0,05, y 0,88-0,97, z 2,29-2,31) | mat 4 (gris foncé métal) | **retirer** | 94 % opaque ≤ 15 mm, 0 % sans face |
| Nez avant : emblème « T » | [4810,682] (x ±0,05, y 0,65-0,70, z -2,26/-2,18) | mat 4 | **retirer** | 95 %, 0 % |
| Sous le bord du coffre : lettrage « TESLA » | [2,74] (x -0,21/0,19, y 0,69-0,75, z 2,30-2,32) | mat 6 | **retirer** | 100 %, 0 % |
| Étriers avant : décalques « TESLA » | mat 2 [0,2], [2,2] (tex 1) | mat 2 | **retirer** | 84 % / 82 %, 0 % |

Rejouable : 2e passage « rien à faire », .wfm 3ba10e373d5ff555, .lod df10d37e6a3ac324.
Gardé tel quel : flancs des pneus (tex 11 : texte très pâle, illisible), caches de roue (tex 13 / 14 : pas de « T »), mats 25 et 29 (couleurs vert / jaune vif, petites pièces sans logo, hors sujet : signalé seulement).

Passage final : mesures : 0 zone(s) mesurée(s), ΔE2000 max 0.00 ; taille 7 119 289 → 7 101 421 octets ; aucune alerte

## mustang
RG-09 : source `ford-mustang` (Shelby GT500) : 24 matériaux = les 24 premiers du .wfm, triangles égaux (24 = plaque WFS-158).

| Où | Pièce / texture | Matériau / rang | Méthode | Contrôle |
|---|---|---|---|---|
| Calandre, coffre, ailes avant : cobras ; planche de bord « GT500 » / « PERFORMANCE » ; seuils de porte ; volant : cobra | mat 5 (BadgeA, décalques à transparence, 10 pièces, 84 triangles : tout le matériau) | mat 5, tex 6 / 7 | **retirer** | décalques posés sur la surface : 80-100 % opaques ≤ 15 mm, 0 % sans face (règle « deux faces » : les seuils sont des matériaux à deux faces) |
| 4 étriers : décalques de marque | mat 0 (CalliperBadge, 4 quads, tout le matériau) | mat 0, tex 0 | **retirer** | 69-87 % opaques, 0 % sans face |
| Capot : cobra de la grille d'aération | mat 15 (Grille6) **gardé**, tex 21 + normale 22 | mat 15 | **effacer** « auto » (gris du fond) | la grille ferme l'aération (moteur 4 cm plus bas) : la retirer ouvrirait le capot ; ΔE2000 0,00, grain 0,07 |
| Caches de roue : ovale Ford | carte métal / rugosité tex 5 (l'ovale y est dessiné en métal : reflet) | mat 3 (Wheel1A), rôle « metal » | **effacer** « auto » (jaune = rugueux, non métal) | couleur (tex 3) noire et normale (tex 4) plates à cet endroit |
| Habitacle : cobra (dossier / ciel de toit), petit cobra du volant, « RECARO » des sièges | tex 23, 3 zones | mat 16 (InteriorA) | **effacer** #2d2d2d / #262626 (fond 45 / 38) | ΔE2000 0,64 / 0,00 / 0,00 contre le fond voisin ; grain 0,98 / 2,02 / 1,31 (2,02 : petit cobra au milieu d'icônes, 1 cm au volant) ; normale 24 plate à ces endroits |

Hors zones (RG-07 a) : tex 21 0,000 / 0,00 ; tex 23 0,051 / 1,77.
Rejouable : 2e passage « rien à faire », .wfm 77b533dd58f87948, .lod 1d292a429883d456.
Gardé tel quel (non visible) : tex 9 / 10 du moteur (ovales Ford, plaque « GT500 HAND-BUILT WITH PRIDE », « FORD PERFORMANCE ») : moteur sous le capot fermé, la grille d'aération est opaque ; tex 27 (« FORD PERFORMANCE ») : n'est utilisée par aucun matériau ; flancs des pneus (tex 3 : texte illisible). Feux (tex 25) : aucun logo.

## Questions
- Moteur et tex 27 gardés (invisibles en jeu) : le PO veut-il qu'on les efface aussi « au cas où » (aucun effet à l'écran) ?

Passage final : mesures : 4 zone(s) mesurée(s), ΔE2000 max 0.64 ; hors zones : moyenne max 0.051, 99e centile max 1.77 ; taille 5 877 942 → 5 843 889 octets ; alertes : zone 2 : grain σ 2.02 > 1,5 (l'uni ferait une tache lisse)

## p208
RG-09 : source `peugeot-208` : 17 matériaux, triangles égaux (15 et 16 = plaques WFS-158). Aucune texture de logo (grain, métal brossé) : logos en relief dans Object_2 (mat 0) et Object_90 (mat 13), repérés par amas connexes.

| Où | Pièce | Matériau | Méthode | Contrôle |
|---|---|---|---|---|
| Calandre : lion | [274,146] | mat 0 | **retirer** | 48 % opaque, 25 % sans face : la calandre est ajourée partout (74 % des rayons passent à travers la grille à côté du lion, modèle d'origine) : à sa place on voit la même grille ; à confirmer sur les vues de près |
| Capot : « 208 » (3 chiffres) | [909,32], [963,18], [1373,35] | mat 0 | **retirer** | 77-85 % opaques, 4-7 % sans face |
| Hayon : « PEUGEOT » (7 lettres) | [1002,14] [1063,19] [1082,23] [1108,17] [1131,20] [1158,21] [1299,25] | mat 0 | **retirer** | 61-72 % opaques, 4-27 % sans face (lettres posées sur le bandeau noir ; à confirmer sur les vues de près) |
| Hayon : lion | [1179,84] | mat 0 | **retirer** | 84 %, 4 % |
| Hayon : « 208 » (3 chiffres) | [231,41], [872,22], [1337,32] | mat 0 | **retirer** | 76-85 %, 0 % |
| 4 caches de roue : lion en relief | mat 13 [1108,312], [2530,336], [4980,352], [7288,376] | mat 13 | **retirer** | 89-90 % opaques, 0 % sans face (le moyeu fermé 2-3 mm derrière) |

Rejouable : 2e passage « rien à faire », .wfm 52bfc587a33d6de5, .lod 01472b1a2913d105.
Gardé tel quel : volant (aucun lion séparé trouvé), autres petites pièces chromées de mat 0 (barrettes de calandre, feux).

Passage final : mesures : 0 zone(s) mesurée(s), ΔE2000 max 0.00 ; taille 6 726 268 → 6 703 168 octets ; aucune alerte

## p911 (source `porsche-911`)
- RG-09 : source 13 matériaux, `.wfm` 13 ; triangles identiques pour 13 des 13 matériaux de la source.
- effacer : texture 3 (couleur, 1 zone(s), couleur auto) : bandeau des feux arrière (matériau 9, tex_shiny) : inscriptions « 911 Carrera S » et « PORSCHE » sur le fond noir transparent : effacées (couleur et alpha de l'anneau)
- effacer : texture 1 (normale, 6 zone(s), couleur #8B81FE) : carte de normales des pneus (matériau 5) : textes moulés du flanc (« BFGoodrich » ×2, DOT, dimensions) : relief effacé ; couleur = normale médiane du flanc autour (139, 129, 254), pas la normale plate
- effacer : texture 2 (metal, 6 zone(s), couleur #FF41FD) : carte métal / rugosité des pneus (matériau 5) : mêmes textes du flanc ; couleur = valeur médiane du flanc autour (255, 65, 253) : anneau trop hétérogène pour « auto »
- retirer : boot.007 (écusson Porsche du capot avant, quadrilatère texturé) (matériau 10, 4 triangles) ; trous : 0 %
- mesures : 1 zone(s) mesurée(s), ΔE2000 max 0.00 ; hors zones : moyenne max 0.000, 99e centile max 0.00 ; taille 5 584 940 → 5 521 917 octets ; aucune alerte

## rrsport (source `range-rover-sport`)
- RG-09 : source 21 matériaux, `.wfm` 21 ; triangles identiques pour 21 des 21 matériaux de la source.
- retirer : Object_2729 / Badges (ovale de marque du hayon, à droite) (matériau 13, 670 triangles) ; trous : 67 %
- retirer : Object_2729 / Badges (ovale de marque de la calandre, à gauche) (matériau 13, 668 triangles) ; trous : 35 %
- retirer : Object_9416 / ChromeTrims1_2 (socle chromé de l'ovale du hayon) (matériau 20, 188 triangles) ; trous : 66 %
- retirer : Object_263 / BlackChrome1 (socle de l'ovale de la calandre) (matériau 8, 130 triangles) ; trous : 43 %
- retirer : Object_263 / BlackChrome1 (lettres « RANGE » du bord du capot) (matériau 8, 214 triangles) ; trous : 59 %
- retirer : Object_263 / BlackChrome1 (lettres « ROVER » du bord du capot) (matériau 8, 284 triangles) ; trous : 54 %
- retirer : Object_263 / BlackChrome1 (lettres « RANGE ROVER » du bandeau du hayon) (matériau 8, 3690 triangles) ; trous : 46 %
- mesures : 0 zone(s) mesurée(s), ΔE2000 max 0.00 ; taille 6 613 706 → 6 499 142 octets ; aucune alerte

## s3 (source `audi-a3`)
- RG-09 : source 29 matériaux, `.wfm` 29 ; triangles identiques pour 29 des 29 matériaux de la source.
- effacer : texture 18 (couleur, 1 zone(s), couleur auto) : caches des roues (matériau 25, RIMLOGO) : quatre anneaux Audi sur fond gris : effacés (gris du fond)
- effacer : texture 19 (couleur, 3 zone(s), couleur #1E1E1E) : couleur des pneus (matériau 26) : « BRIDGESTONE », « POTENZA », « RE050A » sur le flanc : effacés ; couleur fixe (30, 30, 30) = médiane des anneaux (28 à 30) : l'anneau de « RE050A » est trop hétérogène pour « auto »
- effacer : texture 20 (normale, 3 zone(s), couleur par zone) : normales des pneus (matériau 26) : relief des mêmes textes : plat (normale médiane de l'anneau (128, 126, 253), presque plate)
- effacer : texture 11 (couleur, 1 zone(s), couleur #000000) : boutons de l'intérieur (matériau 14, BOTOES) : nom de marque audio « BANG & OLUFSEN » : effacé (fond noir ; anneau trop chargé d'autres icônes pour « auto »)
- retirer : Object_4 / INTCROM (anneaux du centre du volant, 1/6) (matériau 2, 102 triangles) ; trous : 3 %
- retirer : Object_4 / INTCROM (anneaux du volant, 2/6) (matériau 2, 49 triangles) ; trous : 18 %
- retirer : Object_4 / INTCROM (anneaux du volant, 3/6) (matériau 2, 121 triangles) ; trous : 7 %
- retirer : Object_4 / INTCROM (anneaux du volant, 4/6) (matériau 2, 49 triangles) ; trous : 4 %
- retirer : Object_4 / INTCROM (anneaux du volant, 5/6) (matériau 2, 22 triangles) ; trous : 14 %
- retirer : Object_4 / INTCROM (anneaux du volant, 6/6) (matériau 2, 57 triangles) ; trous : 5 %
- retirer : Object_17 / BLACK (fond noir en forme d'anneaux de la calandre) (matériau 12, 328 triangles) ; trous : 1 %
- retirer : Object_17 / BLACK (fond noir du monogramme « S3 » de la calandre) (matériau 12, 36 triangles) ; trous : 0 %
- retirer : Object_23 / CROM (anneaux et monogramme « S3 » du coffre) (matériau 16, 933 triangles) ; trous : 0 %
- retirer : Object_23 / CROM (anneaux de la calandre) (matériau 16, 536 triangles) ; trous : 4 %
- retirer : Object_23 / CROM (monogramme « S3 » de la calandre) (matériau 16, 392 triangles) ; trous : 0 %
- retirer : Object_40 / RED2 (fond rouge du « S » du monogramme du coffre) (matériau 23, 2 triangles) ; trous : 0 %
- mesures : 5 zone(s) mesurée(s), ΔE2000 max 0.63 ; hors zones : moyenne max 0.066, 99e centile max 0.97 ; taille 6 340 765 → 6 240 238 octets ; aucune alerte

## sf90 (source `ferrari-sf90`)
- RG-09 : source 53 matériaux, `.wfm` 54 ; triangles identiques pour 53 des 53 matériaux de la source ; 1 matériau(x) ajouté(s) par WFS-158 (plaques).
- effacer : texture 0 (couleur, 4 zone(s), couleur #1C1B1B) : atlas des écussons (matériau 0) : écusson à cheval, plaque « FERRARI » à cheval, rond à cheval, motif rouge du haut : fond sombre de la texture (28, 27, 27). Pièces qui la voient, gardées (RG-10 : découpe de la carrosserie ou envers derrière) : écusson du nez avant, rond de la planche de bord ; caches des roues avant : sombres comme les arrière (qui voient déjà ce fond)
- effacer : texture 5 (couleur, 2 zone(s), couleur #211F20) : atlas de l'intérieur (matériau 8) : « SF90 XX STRADALE » (seuils de porte) et « JBL » (haut-parleurs) : fond sombre (33, 31, 32)
- effacer : texture 16 (couleur, 1 zone(s), couleur auto) : écran du tableau de bord (matériau 27) : « SF90 XX STRADALE » : fond noir
- effacer : texture 37 (couleur, 4 zone(s), couleur #080808) : couleur des pneus (matériau 49) : « MICHELIN » ×2 (avec le bonhomme) et « PILOT SPORT 4 S » ×2 : flanc uni (8, 8, 8)
- effacer : texture 38 (normale, 4 zone(s), couleur par zone) : normales des pneus (matériau 49) : relief des mêmes textes : plat (normale de l'anneau (128, 127, 255))
- effacer : texture 1 (couleur, 1 zone(s), couleur #303030) : écussons des ailes avant (matériau 1, seul à l'utiliser) : la carrosserie est découpée sous l'écusson (rayons : 94 à 96 % sans rien derrière) : pièce gardée (RG-10), texture unie, couleur des garnitures sombres voisines (matériau 4, albédo 0,03)
- effacer : texture 13 (couleur, 1 zone(s), couleur #303030) : écusson du volant (matériau 24, seul à l'utiliser) : 15 % de rayons sans surface derrière : pièce gardée (RG-10), unie, couleur du volant autour (matériau 4, albédo 0,03)
- effacer : texture 27 (couleur, 1 zone(s), couleur #6B6B6B) : atlas du moteur (matériau 39) : plaque « FERRARI » (2 triangles du couvercle moteur, sans surface derrière à 40 % : gardée) : plaque grise unie (107, 107, 107)
- effacer : texture 28 (couleur, 1 zone(s), couleur #999999) : décor du moteur (matériau 40, seul à l'utiliser) : cheval, « SF90 XX STRADALE » : 60 % de rayons sans surface derrière : pièce gardée, texture unie, gris du fond (153)
- retirer : Object_21 / PLASTIC_INT_2 (décor du capot avant, motif de la marque) (matériau 2, 4 triangles) ; trous : 0 %
- retirer : Object_1291 / PLASTIC_INT_47 (inscription « Brembo » de l'étrier avant gauche) (matériau 46, 2 triangles) ; trous : 0 %
- retirer : Object_1291 (« Brembo », étrier arrière gauche) (matériau 46, 2 triangles) ; trous : 2 %
- retirer : Object_1291 (« Brembo », étrier avant droit) (matériau 46, 2 triangles) ; trous : 0 %
- retirer : Object_1291 (« Brembo », étrier arrière droit) (matériau 46, 2 triangles) ; trous : 3 %
- mesures : 14 zone(s) mesurée(s), ΔE2000 max 13.75 ; hors zones : moyenne max 0.041, 99e centile max 1.51 ; taille 7 329 892 → 7 050 844 octets ; aucune alerte

## tcross (source `vw-t-cross`)
- RG-09 : source 20 matériaux, `.wfm` 22 ; triangles identiques pour 20 des 20 matériaux de la source ; 2 matériau(x) ajouté(s) par WFS-158 (plaques).
- effacer : texture 3 (couleur, 1 zone(s), couleur #181818) : atlas des roues (matériau 11) : logo VW des caches de roue : gris très sombre de la couronne autour (24, 24, 24) ; anneau chromé du cache gardé
- effacer : texture 6 (couleur, 1 zone(s), couleur #141414) : atlas de l'intérieur (matériau 13) : logo VW du centre du volant : sombre du moyeu (20, 20, 20) ; anneau chromé gardé
- retirer : 08_inner_metal_fj_chrome / chrome (logo VW de la calandre) (matériau 3, 202 triangles) ; trous : 0 %
- retirer : chrome (hayon : logo VW et trois inscriptions) (matériau 3, 2289 triangles) ; trous : 0 %
- mesures : 2 zone(s) mesurée(s), ΔE2000 max 46.25 ; hors zones : moyenne max 0.039, 99e centile max 0.99 ; taille 6 478 330 → 6 442 873 octets ; aucune alerte

## v60 (source `volvo-v60`)
- RG-09 : source 23 matériaux, `.wfm` 23 ; triangles identiques pour 23 des 23 matériaux de la source.
- effacer : texture 0 (couleur, 1 zone(s), couleur auto) : volvo_logo.tex (matériau 15) : « VOLVO » blanc sur bleu marine, vu par les caches des 4 roues : bleu marine uni
- effacer : texture 1 (normale, 1 zone(s), couleur par zone) : normales de volvo_logo.tex : relief des lettres : plat
- effacer : texture 4 (couleur, 1 zone(s), couleur #3E414C) : cadre des plaques (matériau 17) : logo et nom « Sketchfab » du haut du cadre : gris du cadre (62, 65, 76) ; relief plat déjà
- retirer : body_body / body_chrome (lettres « VOLVO » du hayon et cercle fléché de la calandre) (matériau 7, 184 triangles) ; trous : 11 %
- retirer : body_body / body_chrome (lettres du hayon, barre diagonale et cercle de la calandre) (matériau 7, 242 triangles) ; trous : 8 %
- retirer : body_volvo_logo.tex (plaque « VOLVO » de la barre de calandre) (matériau 15, 8 triangles) ; trous : 7 %
- mesures : 2 zone(s) mesurée(s), ΔE2000 max 34.23 ; hors zones : moyenne max 0.014, 99e centile max 0.43 ; taille 2 736 036 → 2 678 753 octets ; aucune alerte

## x5 (source `bmw-x5`)
- RG-09 : source 27 matériaux, `.wfm` 29 ; triangles identiques pour 27 des 27 matériaux de la source ; 2 matériau(x) ajouté(s) par WFS-158 (plaques).
- effacer : texture 34 (couleur, 2 zone(s), couleur #0E0E0E) : jantes (matériau 26) : rondelle BMW des caches (lettres, quartiers) et logo M tricolore : noir de la couronne de la rondelle (13 à 15) ; anneau argent du cache gardé
- effacer : texture 35 (normale, 2 zone(s), couleur par zone) : normales des jantes : relief de la rondelle (lettres, croix) et du M : plat (le centre du cache devient plat, le bord bombé reste)
- effacer : texture 29 (couleur, 2 zone(s), couleur #09306B) : étriers (matériau 24, DISK) : logos M tricolores (deux) : bleu de l'étrier (9, 48, 107)
- effacer : texture 30 (normale, 2 zone(s), couleur par zone) : normales des étriers : relief des M : plat
- effacer : texture 0 (couleur, 2 zone(s), couleur #1E1E1E) : atlas de l'intérieur (matériaux 0, 1, 6, 16) : rondelle BMW (volant, intérieur) et logo M : sombre du fond (30) ; anneau blanc de la rondelle gardé
- effacer : texture 4 (couleur, 2 zone(s), couleur #1E1E1E) : variante de l'atlas de l'intérieur (matériau 3) : mêmes rondelle et M
- effacer : texture 14 (couleur, 1 zone(s), couleur #101010) : BADGES (matériau 10) : rondelles BMW du capot et du hayon : la carrosserie est percée sous la rondelle (rayons : 82 à 97 % sans surface, ou l'envers de l'intérieur) : pièces gardées (RG-10), centre et lettres unis du noir de la couronne (16) ; anneau argent gardé
- effacer : texture 15 (normale, 1 zone(s), couleur par zone) : normales de BADGES : relief des lettres et de la croix : plat
- retirer : x5:SK_Trunk_101 / x5dummy_material_7 (lettres « X5 » du hayon) (matériau 15, 50 triangles) ; trous : 10 %
- retirer : lettres « xDrive40i » du hayon (1/6) (matériau 15, 32 triangles) ; trous : 0 %
- retirer : lettres « xDrive40i » (2/6) (matériau 15, 24 triangles) ; trous : 1 %
- retirer : lettres « xDrive40i » (3/6) (matériau 15, 19 triangles) ; trous : 2 %
- retirer : lettres « xDrive40i » (4/6) (matériau 15, 57 triangles) ; trous : 1 %
- retirer : lettres « xDrive40i » (5/6) (matériau 15, 53 triangles) ; trous : 6 %
- retirer : lettres « xDrive40i » (6/6) (matériau 15, 28 triangles) ; trous : 0 %
- mesures : 9 zone(s) mesurée(s), ΔE2000 max 63.46 ; hors zones : moyenne max 0.215, 99e centile max 2.57 ; taille 7 270 036 → 7 204 946 octets ; aucune alerte

## Grain (RG-13, décision du PO) : zones unies sur une surface au grain marqué (σ de L* > 1,5), ΔE2000 ≤ 2
- abarth : texture 2 (matériau 2), zone 1 : σ 4.70 — scorpion du centre de jante : gris de l'anneau du cache
- abarth : texture 3 (matériau 3), zone 1 : σ 3.68 — scorpion du centre de jante : gris de l'anneau du cache
- abarth : texture 4 (matériau 4), zone 1 : σ 3.71 — scorpion du centre de jante : gris de l'anneau du cache
- abarth : texture 5 (matériau 5), zone 1 : σ 11.88 — mot ABARTH dans la bande rouge (zone non affichée)
- abarth : texture 5 (matériau 5), zone 2 : σ 6.02 — mot ABARTH dans la bande rouge (zone non affichée)
- abarth : texture 11 (matériau 13), zone 1 : σ 12.02 — écusson au scorpion de la calandre (pièce gardée, RG-10) : intérieur c
- abarth : texture 11 (matériau 13), zone 2 : σ 12.02 — haut de l'écusson
- abarth : texture 11 (matériau 13), zone 3 : σ 12.02 — mot ABARTH de l'écusson
- elantra : texture 14 (matériau 7), zone 1 : σ 3.98 — « BOSE » (marque de la sono)
- elantra : texture 14 (matériau 7), zone 2 : σ 1.99 — petit logo N (sellerie, volant)
- gallardo : texture 7 (matériau 8), zone 3 : σ 1.95 — tableau de bord (MAT_Details_INT) : bouclier Lamborghini, lettrage « L
- golf : texture 2 (matériau 2), zone 1 : σ 2.37 — rondelle VW des caches de roue : noir du moyeu autour
- golf : texture 10 (matériau 13), zone 1 : σ 3.96 — GTI grand (lettres en relief du hayon, retirées) : plaque grise ; zone
- golf : texture 10 (matériau 13), zone 4 : σ 1.76 — GTI de l'aile avant gauche : bande rouge
- golf : texture 11 (matériau 15), zone 1 : σ 2.47 — copie de la texture des roues, rondelle non affichée (effacée aussi)
- mustang : texture 23 (matériau 16), zone 2 : σ 2.02 — habitacle (InteriorA) : cobra des dossiers / ciel de toit (fond 45), p

## Budget du téléphone (RG-03), avant (main) → après
| voiture | maillages (appels de dessin) | triangles | sommets | octets du .wfm |
|---|---|---|---|---|
| a45 | 25 → 23 | 114 615 → 112 269 | 133 249 → 130 497 | 6 920 268 → 6 793 020 |
| abarth | 57 → 56 | 113 243 → 112 956 | 78 425 → 78 416 | 5 009 676 → 4 933 827 |
| compass | 15 → 15 | 153 781 → 141 575 | 205 478 → 205 478 | 8 453 203 → 8 306 731 |
| corsa | 17 → 17 | 115 439 → 112 717 | 107 715 → 107 715 | 4 866 876 → 4 834 212 |
| elantra | 27 → 26 | 115 487 → 115 117 | 111 464 → 111 452 | 6 017 928 → 5 925 902 |
| gallardo | 18 → 18 | 101 426 → 99 211 | 89 281 → 89 281 | 4 656 240 → 4 567 261 |
| giulia | 66 → 62 | 122 268 → 121 833 | 91 861 → 91 524 | 6 529 929 → 6 264 781 |
| golf | 21 → 21 | 110 074 → 109 884 | 94 940 → 94 940 | 5 389 046 → 5 231 594 |
| gt86 | 15 → 13 | 114 323 → 111 880 | 124 573 → 122 253 | 5 391 844 → 5 288 264 |
| m4 | 51 → 45 | 117 182 → 112 554 | 133 087 → 125 080 | 6 774 689 → 6 376 212 |
| model3 | 37 → 36 | 160 807 → 159 343 | 128 732 → 128 724 | 7 119 289 → 7 101 421 |
| mustang | 25 → 23 | 117 640 → 117 548 | 118 010 → 117 848 | 5 877 942 → 5 843 889 |
| p208 | 17 → 17 | 131 629 → 129 704 | 131 808 → 131 808 | 6 726 268 → 6 703 168 |
| p911 | 13 → 12 | 124 144 → 124 140 | 91 857 → 91 851 | 5 584 940 → 5 521 917 |
| rrsport | 21 → 20 | 112 056 → 106 212 | 145 345 → 144 111 | 6 613 706 → 6 499 142 |
| s3 | 29 → 29 | 119 719 → 117 092 | 136 902 → 136 902 | 6 340 765 → 6 240 238 |
| sf90 | 54 → 52 | 118 143 → 118 131 | 117 467 → 117 445 | 7 329 892 → 7 050 844 |
| tcross | 22 → 22 | 114 352 → 111 861 | 110 560 → 110 560 | 6 478 330 → 6 442 873 |
| v60 | 23 → 23 | 53 130 → 52 696 | 51 073 → 51 073 | 2 736 036 → 2 678 753 |
| x5 | 29 → 29 | 138 412 → 138 149 | 136 983 → 136 983 | 7 270 036 → 7 204 946 |
| **total** | 582 → 559 | 2 367 870 → 2 324 872 | 2 338 810 → 2 323 941 | 122 086 903 → 119 808 995 |

## RG-08 : SHA-256 des 40 fichiers après le 1er passage (depuis main) = après un passage de plus
```
ef1e313fc71ae56ae8308176769aca9537682f13e1b700770d40da7fc30d5e72  a45.wfm
58ad626037e292b7aeba6d54440476835434b680b830f1c8d5fa05d087733dd5  a45.lod
e8959d61ca6a6e17112e1286b4021e7feabd582f998eff8350dea13a1c09619a  abarth.wfm
e023b4830344265fd85e14128aed22c68ae76516a7a9565e02175fac8062c3e0  abarth.lod
0b10f1bde51fc97e3b8a33428eea3b4559a369c31fe6134236d02ccb72ba4484  compass.wfm
c187a8957e8842b20b189bf674b4851e0e8b0dfe60296683ec1184caf8078272  compass.lod
6c0a519256795babf5a25e0dfad2136657a97ab873ba2954867e8cd64dbfb2b3  corsa.wfm
56992d532910c2bfdb2e0c812cc3b2746683c1cc6f0bf2bbcb04220a7f49fe80  corsa.lod
ff17d8929bd782de5f181cc93143100699d82cd1aec4e4a25fb890e779c8bd66  elantra.wfm
92e1e638e4519a47c165e3568ddd3e679bd947d79ebe6446fe8763ba26ceba3b  elantra.lod
74d15cce1a154c17f97dec507e470ce56106d6d3a18f2fd99efce26f927e357c  gallardo.wfm
1d0e780d7ea48b0240f3ed8f818f8f0c1e4e665c1ca2b926ccca176afc646066  gallardo.lod
ab47e082d2d1cf05ac9a9880852bd78a1d943333b1bf36cc6a98d72f9bdacf9c  giulia.wfm
2ddd8f9b4a7adfad2fff5d772dea5849fbe1e997a8ba112ec65f66c1b699742d  giulia.lod
f027aaf0d774f091b24a7b3357875e5caf5902d3f288e70631835def751c119f  golf.wfm
94db2933f17e1eac38e2f0a003dffa977aa2e1deb24facac1aaf6ee23a9608fc  golf.lod
4acd857dc91434aca722dbb6ed0b2449b30a8f841b2ba2c43957ad8aed49c1fc  gt86.wfm
8fe2eb0c306e790e49b2b49e1bc376c5f01b8b437be65d7be9eeba9053808393  gt86.lod
6c8512b9fded6cdfd7f7acbde7b4d4d71015d120896df6e331d99ca6df84180d  m4.wfm
60edf392adca7fe2eaf045fe64fd902b4d88c36a1edc5eb5cd697d9dc0f66b22  m4.lod
3ba10e373d5ff55505e023a80ff27f40fef679928123859326f446d0ffec7baf  model3.wfm
df10d37e6a3ac324c8e47c545902ffc0fcbfb7e1d6a6e63516d2c0232126592d  model3.lod
77b533dd58f87948f29c75a45a8a1198f85b43f404afd50f6338eef3bb93557a  mustang.wfm
1d292a429883d45651938246be4a4ee7261e8a220f92d5818659dbb3d111c088  mustang.lod
52bfc587a33d6de5657a54321620c23b606c26982d2ba3ddb4a17d1beb5f57ca  p208.wfm
01472b1a2913d1055bd83a870e7c3419a7515239ab23f5374ff57c548d09df2f  p208.lod
96970dddb401dcb75bc64537ce843c670e63e6425c428308a2a96f717d3939dd  p911.wfm
518aa44cb409b70fa96bb24329351b4cd543dbd2f2d2cb2a35982b40ad6c1e12  p911.lod
e4f83124c3ab79b848d65d231e383464f841d2a09f6e5df1dc6547cfc6d8cca4  rrsport.wfm
06880f09cf97065fe5dc1842d3852f0ac49be7f6d44d7bde5c75b106d5c78587  rrsport.lod
841e22f79b8e9a68330448f1ce486bc54dec76ac229f4c6a381c41d632fa11be  s3.wfm
cb1b8b176e7c638e81f45d8286af23ab3eaf3c43df3c94f393bc7dd9a96b8acb  s3.lod
fd2d5deef4c9cda981722cd5e1600adf07c0178e97f3d1c86aba59cd6771c4e1  sf90.wfm
73c8f4442aca99184ea6a29686fbd60c7f8399af23d3290dff5b4453cee21c73  sf90.lod
b14fae5da7fc03aa2d7038ae0a40b198a6fa3bd4ababbf67fbb4472fce4dd21c  tcross.wfm
f9d96450641054991d3deec4ef72fa9a486595d0a0c223ae6b5a326af5dc323f  tcross.lod
7f0dd1716d75eb72a238288313cbedc720cd11edd11e9a949eaacbdeac03e950  v60.wfm
b56141fd310fbb67917c6fd97e36a88582813fd80c40e9c67cd71248c9a531d6  v60.lod
8860f6f2a43df4db3ebafb2b4df20dd6b044fb32794f9ba9d65c1e7c858b4565  x5.wfm
4370b345c4e8806033793ef4922303364a86d8054a6e44f9ab8e51693507dcc9  x5.lod
```
