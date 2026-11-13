# WFS-378 — Inventaire des formes géométriques nues (hors décor des cartes)

Fait le 2026-11-13 par Claude, agent dev3d (01Us6uCwca), en lecture seule : **aucun objet n'est remplacé** (RG-04).
Périmètre : ordre de l'utilisateur du 2026-11-09 20:12 UTC (« hors décor ») : **garage, habitacle et vue Cockpit, véhicules,
effets visibles, portiques des checkpoints, menus 3D**, plus les personnes et animaux (RG-01). Le décor des cartes (E15, WFS-369 à 373,
376, 380, 383, 344) n'est pas redécoupé ; les objets de décor croisés en chemin sont seulement signalés (§ 8).

**Méthode (RG-05)** : recherche des appels `box(`, `boite(`, `tri(`, `quad(`, `cylindre(` / `cylAxe(`, `cone(`, `disque(`, `tore(`,
`anneau(`, `plan(`, `ellipsoide(`, `cube_`, `halo(` dans `src/game`, `src/render`, `src/ui`, `src/world`, `src/core` ; chaque ligne relue
dans le code (numéros de ligne de `main` au commit d33588ad) ; objets visibles confirmés par les captures du banc PC (lavapipe, même rendu que la CI)
dans `qa/pr/WFS-378/`. Recoupé avec `regles-de-gestion/E16-inventaire.md` § 4 (épic E16 bloquée par l'utilisateur ; ses numéros de ligne
sont anciens, par exemple le garage avant le découpage WFS-534).

**Classement** :
- **NUE** = forme géométrique posée telle quelle (boîte sans chanfrein, cube mis à l'échelle, disque, tore, triangle plat, cylindre) : **anomalie** à remplacer.
- **MODELÉE** = pièce faite par le code mais au niveau du vrai objet (contour arrondi, chanfrein, profil balayé, pièce tournée) : conforme, rien à faire.
- **EFFET** = carré face caméra ou grille au sol qui n'est pas un objet (lumière, particule, ombre) : voir la question au PO (§ 9).
- **INVISIBLE** = code mort, banc seulement ou secours jamais atteint : à supprimer plutôt qu'à remplacer.

## Totaux

| Zone | NUE | MODELÉE | EFFET | INVISIBLE |
|---|---|---|---|---|
| 1. Garage et menus 3D | 4 | 0 | 0 | 1 |
| 2. Habitacle et vue Cockpit | 1 (à arbitrer) | ≈ 25 familles de pièces | 0 | 2 |
| 3. Véhicules : police (Poursuite) | 1 (rampe : 3 cubes) | 1 (bandes POLICE) | 1 | 1 |
| 4. Objets de course : portique, chantier, ralentisseur | 4 | 1 (feux du Grand Prix) | 0 | 0 |
| 5. Personnes et animaux | 2 | 0 | 0 | 0 |
| 6. Deux-roues | 1 (casque, à arbitrer) | 0 | 1 | 0 |
| 7. Effets visibles | 0 | 0 | 12 | 3 |
| **Total** | **13** | | **14** | **7** |

## 1. Garage et menus 3D (fort : premier écran vu)

| Objet | Fichier:ligne | Forme | À l'écran | Classement | Capture |
|---|---|---|---|---|---|
| Sol du garage | `src/ui/garage_decor.cpp:70` (forme `disque` :17, dessin :117) | **disque** plat de 30 m, mat | 1 | **NUE** | `apres-garage-0.jpg` |
| Plateau tournant | `src/ui/garage_decor.cpp:70` (`disque` épais de 8 cm, dessin :118) | **cylindre plat** sans chanfrein ni dessous | 1 | **NUE** (ANO-225 V3) | `apres-garage-0.jpg` |
| Anneau néon sous la voiture | `src/ui/garage_decor.cpp:71` (`tore` :37, dessin :122) | **tore** R 3,62 m, r 3,5 cm | 1 | **NUE** (ANO-225 V3) | `apres-garage-0.jpg`, `apres-garage-1.jpg` |
| Barres lumineuses du fond | `src/ui/garage_decor.cpp:71` (`boite` :51, dessin :123-128) | **boîtes** 8 cm × 9 m × 8 cm, émissives | 9 (≈ 4 vues en 16:9) | **NUE** | `apres-garage-0.jpg` |
| Cube orange de la scène d'essai | `src/core/scene_essai.cpp:24-37` (dessin :62) | cube de 1 m | 0 en jeu (banc `scene-dediee-vide`) | INVISIBLE | — |

Le même décor sert au menu (`menu1`), au showroom, aux crédits et au banc `qualite`. Copie pour le banc seulement : `src/render/bench_spots.cpp:22-51`.
Les écrans `src/ui/ecrans/**`, la fiche, la couleur et l'achat sont de l'interface 2D (permise). Le tableau des peintures
(`garage-peintures*`) ne dessine que les voitures (.wfm).

## 2. Habitacle et vue Cockpit (fort en vue Cockpit)

Les 20 voitures ont un intérieur propre (`src/render/vehicule_cockpit_defs.cpp:21-192`, les 20 identifiants de `CARS[]`). Presque tout est
**MODELÉ** : formes de `src/render/vehicule_cockpit_formes.h` (`dalle` / `dalleFace` :51 / :85 contour arrondi et chanfrein, `profile` :101
section arrondie balayée, `lisse` :134, `cadreArrondi` :172, `jante` :217) et pièces propres aux modèles (`vehicule_cockpit_details.cpp:61-505` :
aérateurs, boutons, molettes, porte-gobelets, haut-parleurs… **aucune forme nue**). Volant : `vehicule_cockpit_volant.cpp` (jante tube à méplats,
moyeu `moule` lissé) : MODELÉ. Rétroviseurs : coques balayées (`coqueRetro`, `vehicule_cockpit.cpp:168`), glaces plates à coins arrondis
(une glace est plate dans le réel) : MODELÉ.

| Objet | Fichier:ligne | Forme | À l'écran | Classement | Capture |
|---|---|---|---|---|---|
| Dessus, creux et face de la planche de bord | `src/render/vehicule_cockpit.cpp:451-452, 457-458` (`extruder` :46) | profil de 4 points extrudé, **arêtes vives**, bouts plats | 2 à 5 bandes | **à arbitrer** : pas une primitive, mais un « bloc » (le commentaire :463 dit la même chose de l'ancienne visière) | `apres-cockpit-vitrine-*.jpg` |
| Contre-portes, plancher, tablier | `vehicule_cockpit.cpp:608` (`boitesFermeture`, `vehicule_cockpit.h:201`) | `dalle` arrondie (r 1,5 cm, chanfrein 8 mm), mais pièces pleines | 4 | MODELÉE (E16 les voyait « blocs pleins » : à regarder dans le ticket de la planche) | `apres-cockpit-vitrine-*.jpg` |
| Gabarit magenta du banc (portes, tablier) | `vehicule_cockpit.cpp:693-694` | boîtes de 2 mm | 0 en jeu (banc `cockpit-voisine`) | INVISIBLE | — |
| Cockpit commun de secours (planche, casquette, montants, pavillon en `boite` ; cadran en `anneau` ; aiguille en `plan` ; volant `tore` + `cylindre` + 3 `boite`) | `src/game/camera_vue.cpp:284-298` (dessin :339-347) | **boîtes, disques, tore, cylindre** | **0 en jeu** : seulement pour une voiture sans intérieur, or les 20 en ont un | INVISIBLE (secours) : à supprimer ou à garder hors jeu, pas à remodeler | — |
| Chiffres, barres et aiguilles des écrans | `vehicule_cockpit.cpp:244-300, 755-915` | quads émissifs sur l'écran | dynamique | affichage (comme une image d'écran), permis | `apres-cockpit-vitrine-*.jpg` |

## 3. Véhicules — police (Poursuite)

Roues, phares, feux, voiture fantôme, camion de la Poursuite et voitures de police viennent des vrais modèles (.wfm) : rien à faire.
Pas de trace de pneus dans le code. Plaques : prévues en vrai 3D par **WFS-483 / 484** (déjà prévu).

| Objet | Fichier:ligne | Forme | À l'écran | Classement | Capture |
|---|---|---|---|---|---|
| Rampe de gyrophares : socle gris + 2 feux bleus | `src/game/poursuite_system.cpp:428-430` (cube :111-123) | **cube mis à l'échelle** ×3 (1 m × 5 cm × 30 cm ; feux 40 × 7 × 26 cm) | 1 rampe par voiture de police à moins de 80 m (≤ 6) | **NUE** (WFS-338 a seulement remplacé les anciennes boîtes rouge et bleue) | `apres-police-*.jpg`, `apres-poursuite-police*.jpg` |
| Bandes « POLICE » des portières | `poursuite_system.cpp:756-788`, `src/game/police_bandes.h` | maillage collé à la tôle | 1 par voiture | MODELÉE (WFS-460, fusionné) | `apres-police-bandes*.jpg` |
| Anciennes bandes plates (comparaison avant / après) | `poursuite_system.cpp:741-752, 417-420` | quads plats | 0 en jeu (`g_bancBandesAvant`) | INVISIBLE | — |
| Halos des gyrophares | `poursuite_system.cpp:432-433` | carré face caméra (≈ 0,6 m) | ≤ 2 par voiture | EFFET | `apres-poursuite-police*.jpg` |

## 4. Objets de course : portique, chantier, ralentisseur (fort : sur la route)

| Objet | Fichier:ligne | Forme | À l'écran | Classement | Capture |
|---|---|---|---|---|---|
| Portique du checkpoint : 2 poteaux 0,6 × 9 m, poutre 2,4 m, 2 barres néon cyan, bande au sol | `src/world/portique.cpp:69-74` (`boite` de `enseignes.cpp:34`) ; secours virage serré / tunnel :95 (bande au sol seule) ; texte « CHECKPOINT n » :86 (quads) | **6 boîtes** | 1 (le prochain checkpoint), mode Checkpoint, Ville et Corniche | **NUE** (ANO-225 R3) | `apres-vie-portique*.jpg` |
| Cône de chantier K5a | `poursuite_system.cpp:143-176` (maillage), dessin :439 ; pose `src/game/poursuite.cpp:173-187` | troncs de cône à 14 côtés + **socle en boîte nue** 40 × 40 × 4 cm | 7 cônes de biseau + 8 à 16 sur 120 m par chantier | **NUE** (socle ; cône trop simple : pas de bord de socle, pas de bague) | `apres-poursuite-travaux*.jpg` |
| Panneau AK5 « travaux » sur pied | `poursuite_system.cpp:446-449` (triangle :177-180) | **2 triangles plats** (rouge, jaune) + **2 cubes** (mât, pied lesté) | 1 par chantier | **NUE** | `apres-poursuite-travaux*.jpg` |
| Ralentisseur + 2 plots orange | `poursuite_system.cpp:466-469` | **boîtes** : bande jaune, 7 bandes noires (hors qualité Basse), 2 plots 0,38 × 0,75 m | 1 par surprise « Ralentisseur » | **NUE** (WFS-119) | `apres-poursuite-ralentisseur*.jpg` |
| Ligne de départ en damier, repères de la grille du Grand Prix | `src/world/decor_carte.cpp:389-393`, `src/world/circuit.cpp:723-729` | quads posés au sol | 1 ligne ; 10 repères | marquage peint (comme une ligne blanche), permis | — |
| Portique des 5 feux du Grand Prix | `src/world/decor_grandprix_modeles.cpp:495-515` | pylônes en treillis, poutre, boîtier congé, optiques tournées | 1 | MODELÉE (Grand Prix en pause, E15) | — |

Le compte à rebours 3-2-1-GO est de l'interface 2D (`course_system.cpp:135-147`). Aucune flèche 3D de course.

## 5. Personnes et animaux

Piétons, enfants, clients, spectateurs, conducteurs : vrais `.wfp` (le niveau « très simple » est le vrai maillage allégé, `squelette.cpp:60-80`).
Chiens, pigeons, mouettes, animaux de ferme : vrais `.wfa`. Restent :

| Objet | Fichier:ligne | Forme | À l'écran | Classement | Capture |
|---|---|---|---|---|---|
| Poussette (tubes, roues, siège, panier, repose-pieds, dossier, capote à 6 pans) | `src/world/foule_poussette.cpp:25-62` (`carte::cylAxe` 6 à 8 côtés, `carte::box`, `carte::quad`) ; maillages `pietons.cpp:638-648`, dessin :384 | **cylindres, boîtes, quads** (≤ 400 triangles) | ≈ 1 famille sur 4, 8 au plus | **NUE** | `apres-pietons-*.jpg` |
| Laisse du chien | `src/world/ville.cpp:1342` (dessin :1526-1533) | **boîte** 12 × 12 mm × 1 m étirée main → collier | 1 par promeneur (≈ 1 tous les 450 m, Ville) | **NUE** (fichier du décor de la Ville : voir § 8) | — (trop fine au banc 1280 px) |

## 6. Deux-roues

| Objet | Fichier:ligne | Forme | À l'écran | Classement | Capture |
|---|---|---|---|---|---|
| Casque du pilote (coque + visière) | `src/game/deux_roues.cpp:170, 173` (`ellipsoide` :85), dessin ≈ :225 | morceaux d'**ellipsoïde** (24 × 12, visière 12 × 4) sur le vrai pilote | 1 par deux-roues (≤ 16) | **à arbitrer** : boule lisse, pas de vraie forme de casque (mentonnière, bord de visière, aération) | `apres-motos-*.jpg` |
| Ombre du deux-roues + tache sous chaque roue | `src/game/traffic_system.cpp:239, 251` | carré flou au sol | 3 par deux-roues | EFFET | `apres-motos-*.jpg` |

## 7. Effets visibles

Tous passent par `Renderer::halo` (`src/render/renderer.cpp:1586`) ou `drawQuads` (:1570) : un carré face caméra que le shader
`haloLumiere` (`shaders/eclairage.glsl:278`) remplit d'un fondu radial (il paraît rond), ou par une grille additive posée sur la route.
Ce ne sont pas des objets mais de la lumière ou des particules ; E16 les classait « C garder ».

| Effet | Fichier:ligne | Forme | À l'écran | Capture |
|---|---|---|---|---|
| Ombre de contact du joueur, des adversaires, du trafic | `src/game/game.cpp:60-65, 406` ; `renderer.cpp:286-292, 1399-1401` ; `traffic_system.cpp:60-64, 177` | carré flou au sol (`ombreContact`, `shaders/vehicule.glsl:26`) | 1 par voiture | toute capture de course — **déjà prévu (WFS-229, ombre calculée)** |
| Halos des feux arrière, stop, recul | `renderer.cpp:1603-1626` (tailles :1363-1376) | carré face caméra (cœur 7 cm, lueur 20 cm) | 2 à 4 par voiture | `apres-halos-*.jpg` |
| Point de phare vu dans le rétroviseur | `renderer.cpp:1645-1652` | carré face caméra (3,5 cm) | 2 par voiture suiveuse, nuit | — |
| Lueur rouge des feux sur la route | `renderer.cpp:1660-1680` | grille 8 × 6 additive au sol | 1 par voiture à moins de 80 m, nuit | `apres-halos-*.jpg` |
| Lumière des phares des autres voitures sur la route | `src/render/nuit.cpp:289-311` | grille 7 × 8 additive au sol | 1 par voiture à moins de 160 m, nuit | `apres-halos-*.jpg` |
| Halos des clignotants et warnings du trafic | `traffic_system.cpp:296-306, 326-335, 352-367` | carré texturé face caméra (0,7 m) | ≤ 64 | — |
| **Halo des têtes de lampadaires** | `nuit.cpp:343-351` (rayon `HALO_TETE_M` 0,6 m) | **carré face caméra qui se lit comme un disque** | 1 par tête vue, coupé en qualité Basse | `apres-halos-*.jpg` — **demandé par l'avis de l'expert 3D (N2)** |
| Étincelles des chocs | `src/render/effets_systeme.cpp:216-237`, `effets_particules.cpp:12` | carrés face caméra additifs (0,22 m) | ≤ 220 | `apres-effets-*.jpg` |
| Fumée des pneus et des accidents | `effets_systeme.cpp:74-98, 239-275` | rectangles face caméra, volute gaussienne + bruit, éclairés comme un nuage | ≤ 140 | `apres-effets-*.jpg` |
| Lignes de vitesse (nitro) | `effets_systeme.cpp:286-302` | quads fins devant la caméra | 28 sous nitro | `apres-effets-*.jpg` |
| Gouttes de pluie, gouttes sur le pare-brise | `src/world/meteo.cpp:87-99, 166-227` | quads 2D d'écran (`uiQuad`) | intensité de la carte | — (banc `pluie`) |
| Flocons de neige | `meteo.cpp:229-283` | particules 3D projetées, dessinées en quads 2D | intensité de la carte | — (banc `neige`) |

**INVISIBLE (code mort, à supprimer)** : silhouette du trafic lointain en 2 cubes (`traffic_system.cpp:274-284, 318-321`, `dessineLoin`,
`[[maybe_unused]]`, WFS-134 RG-11 « jamais de cubes ») ; matériau des « flaques » de phares (`renderer.cpp:283-285`, `poolMat`, plus dessiné
depuis WFS-382) ; `Renderer::haloSol` (`renderer.cpp:1594`, plus d'appel depuis WFS-421).

## 8. Formes nues du décor des cartes croisées en chemin (pas de ticket ici, ordre du 2026-11-09 20:12)

À transmettre aux tickets du décor (E15) s'ils ne les ont pas :
- panneaux néon « WAFORSPEED », « TURBO CAFÉ », « GARAGE 88 » sur 2 poteaux : **boîtes nues** (`src/world/enseignes.cpp:211-215`), ≈ 1 tous les 130 à 200 m, Ville et Corniche (banc `vie`, capture `vie-enseigne-nuit`) — ANO-225 R3 « panneaux orange sur boîtes » ;
- laisse du chien (`ville.cpp:1342`, § 5) ;
- nombre d'appels de formes par fichier de décor (pour mémoire) : `decor_grandprix_modeles.cpp` 211, `plage.cpp` 186, `decor_corniche_modeles.cpp` 122, `campagne.cpp` 71, `ville.cpp` 69, `decor_campagne_modeles.cpp` 61, `marina_bateaux.cpp` 30, `decor_carte.cpp` 30, `archi_ouvertures.cpp` 20, `enseignes.cpp` 16, `mobilier_ville.cpp` 12 (beaucoup sont des pièces MODELÉES : à trier par les tickets E15).

## 9. Proposition de découpage au PO (RG-03 : une zone par ticket, deux au plus, ordre de visibilité en course)

| Ordre | Ticket proposé | Zones | Objets | Agent | Taille |
|---|---|---|---|---|---|
| 1 | **Police : rampe de gyrophares réelle** | 3 | rampe à LED de série (socle profilé, 2 dômes bleus, fixation au toit), halos gardés ; jour et nuit | `@dev` (artiste 3D) | 0,5 j |
| 2 | **Chantier et ralentisseur de la Poursuite** | 4 | cône K5a réel (socle chanfreiné à bord, bagues rétroréfléchissantes), panneau AK5 sur chevalet (panneau à bord tombé, mât, pied lesté), ralentisseur modulaire (dos d'âne en caoutchouc, bandes, plots J11) | `@dev` (artiste 3D) | 1 j |
| 3 | **Portique des checkpoints réel** | 4 | portique en treillis ou en tube (pieds lestés, poutre caisson, bandeau « CHECKPOINT » rétroéclairé, néons dans un profilé) ; secours mémoire WFS-447 gardé | `@dev` (artiste 3D) | 1 j |
| 4 | **Garage 3D : sol, plateau tournant, néons** | 1 | sol en résine ou béton ciré (texture), plateau tournant réel (disque métal chanfreiné, joint, éclairage encastré, ANO-225 V3), réglettes LED dans un profilé alu au lieu des barres | `@dev` (artiste 3D) | 1 j |
| 5 | **Poussette et casque des deux-roues** | 5 + 6 | vraie poussette (CC0 / CC BY ou modelée au réel) ; vrai casque intégral (mentonnière, visière, aération) | `@devartiste` | 1 j |
| 6 | **Planche de bord à arêtes arrondies** | 2 | dessus et face de la planche : arêtes vives → rayons réels (4 à 10 mm), bouts fermés ; contre-portes, plancher, tablier vérifiés | `@dev` (artiste 3D) | 0,5 j |
| 7 | **[TECHNIQUE] Formes invisibles retirées** | 1 + 2 + 3 + 7 | supprimer le code mort (`dessineLoin`, `poolMat`, `haloSol`) ; cockpit commun de secours : supprimé ou réservé au banc | `@dev` | 0,25 j |

**Question au PO (à trancher, pas à l'utilisateur sauf doute)** : les **EFFETS** du § 7 (halos face caméra, particules, ombres floues, gouttes)
sont-ils permis comme « effets visibles » ? Proposition : les garder (ce sont des lumières et des particules, pas des objets ; la règle
interdit le « halo en disque » : le seul halo qui se lit comme un disque posé est celui des **têtes de lampadaires** (avis N2) ; il pourrait
rejoindre un ticket « halos des lampadaires » (forme suivant le vrai verre de la lanterne, éblouissement plutôt que disque), à ranger avec
le décor E15 puisque les lampadaires sont du décor). L'ombre de contact est déjà prévue par WFS-229.

**Déjà prévu (RG-02)** : plaques 3D (WFS-483, 484) ; ombre de contact calculée (WFS-229) ; décor des cartes (WFS-369 à 373, 376, 380, 383, 344 et E15) ;
bandes POLICE (WFS-460, fait) ; secours mémoire du portique (WFS-447).
