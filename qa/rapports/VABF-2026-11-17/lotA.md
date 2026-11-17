# Campagne VABF du 2026-11-17 : lot A (partie1a, partie1b)

## Périmètre
- Source : Release `captures-matin`, `jour.txt` = bancs du matin run 46, commit 33885704f1e1c5438593edba55a9ad9bea553611, 2026-11-17 12:01 heure de Paris (captures du jour).
- Groupes : `partie1a` (58 fichiers), `partie1b` (282 fichiers) : 340 fichiers, tous des PNG 1280x720. Aucun fichier .txt ni journal du banc dans ces deux archives : pas de [ERREUR] ni d'ÉCHEC à lire pour ce lot.
- Contrôle automatique sur les 340 images (luminosité moyenne et contraste) : aucune image noire. Une seule image presque uniforme : `partie1b/carte-campagne-epingle-nuit.png` (moyenne 22/255). Une fois éclaircie, elle reste lisible.
- Images regardées à l'œil : environ 240 (60 planches 2x2 avec le nom de chaque fichier), plus 15 agrandissements de zones suspectes. Toutes les familles sont couvertes : chargement, cockpit (6 voitures, jour, nuit, pluie, virage, braquage, carte), course-cam0 et cam1, vitrines, menu, ombres, trafic, trafic-motos (jour, nuit, plage, ville, virage, choc), braquage, carte-campagne, carte-circuit, carte-plage, carte-ville (lieux, mobilier, port, tunnel, virages), fumée, nitro, étincelles.

## Anomalies

| # | Gravité | Capture (groupe/fichier) | Ce qui ne va pas | Correction proposée | Ticket d'origine probable |
|---|---|---|---|---|---|
| 1 | Bloquante | partie1b/carte-plage-marina.png | Les bateaux du port sont faits de pavés empilés (plaques blanches, noires et bois, sans coque) : des boîtes nues. | Remplacer par de vrais modèles de voiliers et de bateaux à moteur (CC0/CC BY, vraies mesures). | Carte Plage (marina), WFS-369 à 373 |
| 2 | Bloquante | partie1b/carte-plage-cafe-vue.png | Grand bâtiment bleu sur la colline, simple boîte sans fenêtre, sans texture et sans toit. | Remplacer par un vrai bâtiment modélisé, ou supprimer le bloc. | Carte Plage, décor E15 |
| 3 | Bloquante | partie1b/carte-plage-bateaux.png | À droite, bâtiment gris sans fenêtre ni détail : une boîte nue. | Vrai modèle de bâtiment. | Carte Plage, décor E15 |
| 4 | Bloquante | partie1b/carte-plage-galerie.png, carte-plage-galerie-vue.png, carte-plage-scooters.png, carte-plage-scooters-vue.png | Les côtés et l'arrière de la galerie et de la boutique de scooters sont des pans gris lisses (boîte nue). Seule la façade est détaillée. | Modéliser toutes les faces visibles depuis la route (matériaux, fenêtres, toiture). | Carte Plage, décor E15 |
| 5 | Bloquante | partie1b/carte-campagne-col-haut.png (aussi carte-campagne-col.png) | Les montagnes du fond sont des pyramides à grandes facettes plates : des formes géométriques nues. | Relief en maillage de terrain (carte d'altitude réelle), avec texture de roche et de neige. | Carte Campagne (col) |
| 6 | Bloquante | partie1b/carte-campagne-cascade.png | Derrière les sapins, à droite, de grandes dalles grises verticales lisses (falaise en boîtes). Aucune cascade d'eau n'est visible sur la capture. | Falaise modélisée (roche), cascade visible (eau animée). | Carte Campagne (cascade) |
| 7 | Bloquante | partie1b/carte-campagne-peage.png | La cabine de péage est une boîte grise avec un trou pour fenêtre, et les piliers de l'auvent sont des dalles nues. | Vrai modèle de gare de péage (cabine, auvent, barrières). | Carte Campagne (péage) |
| 8 | Bloquante | partie1b/carte-plage-jeux-vue.png | Sur l'aire de jeux de la plage, le filet de volley est un rectangle gris plein qui flotte : un panneau nu. | Vrai filet (mailles, poteaux, bandes). | Carte Plage (jeux) |
| 9 | Bloquante | partie1b/trafic-clignotant.png, trafic-accident.png, partie1a/course-cam1-2.png | Les clignotants allumés sont des boules orange lumineuses qui flottent à côté de la carrosserie, à l'avant et à l'arrière. C'est un halo rond, interdit. | Allumer le feu lui-même (matériau émissif du bloc optique), avec un léger bloom collé au feu. | Trafic, clignotants |
| 10 | Majeure | partie1b/carte-circuit-epingle.png | Dans l'épingle du circuit, le bord de l'asphalte fait une marche en dents de scie qui chevauche le vibreur : les morceaux de route ne se raccordent pas. | Recoudre le maillage de la piste et des vibreurs dans l'épingle. | Carte Circuit |
| 11 | Majeure | partie1b/carte-campagne-rondpoint-nuit.png, carte-ville-apk916-5-nuit.png | La nuit, un halo bleu-blanc très fort sort de l'arrière de la voiture du joueur et éclaire le sol en rond. Une voiture qui roule en marche avant n'a pas cet éclairage (feu de recul ou plaque bien trop forts). | Ramener l'éclairage de plaque à sa vraie valeur, et le feu de recul seulement en marche arrière. | Éclairage voiture nuit |
| 12 | Majeure | partie1b/trafic-motos-phare-ville-nuit.png | Le phare de la moto donne une colonne verticale de lumière blanche très forte au lieu d'un faisceau au sol. Dans le noir, la moto et son pilote ne se voient presque pas. | Orienter le cône du phare vers la route, limiter son intensité et éclairer un peu la moto. | Trafic motos (phare) |
| 13 | Majeure | partie1b/carte-plage-galerie-vue.png, carte-plage-galerie-vue-nuit.png | Une longue poutre grise traverse le ciel en diagonale, en haut de l'image : un objet égaré qui flotte. | Retrouver l'objet (câble ou poutre mal placés) et le remettre en place ou le retirer. | Carte Plage (galerie) |
| 14 | Mineure | partie1a/cockpit-model3-virage.png (et toutes les cockpit-model3-*) | L'écran central de la Model 3 affiche « Id I » au lieu de « 111 » (chiffres mal dessinés). L'écran est aussi caché en partie derrière les boutons frein et nitro. | Corriger la police de l'écran, ou décaler la caméra cockpit pour dégager l'écran. | Cockpit Model 3 |
| 15 | Mineure | partie1b/carte-circuit-gravier.png | Le sapin de gauche, au fond, est cassé : tronc noir épais en bloc, quelques branches plates. | Revoir le LOD lointain du sapin. | Végétation, circuit |
| 16 | Mineure | partie1b/carte-ville-lieu-garage.png | La façade du « GARAGE 88 » est un mur blanc lisse sans détail (presque une boîte, avec seulement l'enseigne et la porte). | Ajouter le détail de façade (bardage, gouttières, vitrages). | Ville, garage |
| 17 | Mineure | partie1b/carte-ville-port-conteneurs.png | Les conteneurs vus de près sont des aplats sans ondulations ni ferrures, sur un sol vert uni. | Vrai modèle de conteneur ISO, texture de sol de port. | Ville, port |
| 18 | Mineure | partie1b/carte-ville-mobilier-fontaine-nuit.png, carte-ville-port-conteneurs-nuit.png, carte-ville-port-grue-nuit.png, carte-campagne-epingle-nuit.png | La nuit, le sujet de la capture (fontaine, conteneurs, grue, épingle) est presque noir et illisible. Il manque un éclairage public près de ces lieux. | Lampadaires ou éclairage d'ambiance près de ces lieux. | Éclairage nuit des cartes |
| 19 | Mineure | partie1b/carte-ville-immeuble-classique(-nuit).png, carte-ville-mobilier-abribus(-nuit).png | Des enseignes 3D (« GARAGE 88 », « RESTAURANT ») passent sous les boutons du HUD (gaz, cartouche du temps) et sont coupées. C'est surtout un effet du cadrage. | Aucune, sauf si le cadrage de jeu montre la même chose (à vérifier sur l'émulateur). | HUD / cadrage |
| 20 | Mineure | partie1b/choc-etincelles.png | Au choc, on ne voit presque pas d'étincelles (quelques points à peine visibles). | Rendre les étincelles plus nombreuses et plus visibles au contact. | Effets de choc |

## OK vérifiés
- Écrans de chargement (0, 1, 2, route, voitures, ville, fondu) : barre de progression, étapes cochées, textes lisibles, rien de coupé. L'écran d'erreur affiche un message clair et un bouton « Réessayer ».
- Menu (ecran-menu-vide, scene-dediee-vide) : boutons Course solo, Course en ligne, Garage et cadeau du jour lisibles. La voiture du garage est en 3D.
- HUD en course : temps, score, guidage (distance et virage), nitro, frein, gaz, volant, compteur km/h et rapport présents sur toutes les captures de course, sans chevauchement entre eux.
- Vitrines gallardo, golf, m4, p911 : voitures entières, roues au sol, plaque « WFS » sans marque, ombres justes.
- Cockpit des 6 voitures (gallardo, model3, mustang, abarth, compass, gt86) : habitacle, volant, rétroviseurs et compteur visibles, de jour, de nuit et sous la pluie (gouttes sur le pare-brise). Aucune vue noire.
- Ombres (a, b, coupées) : ombres des arbres et de la voiture cohérentes, pas d'ombre coupée visible.
- Trafic voitures et motos : motos et pilotes en vrais modèles (routière, scooter, sportive). Chute de moto visible (choc-3). Feux arrière rouges la nuit.
- Fumée de dérapage (jour 0,5 s, 1 s, nuit) et lignes de nitro visibles. Accident avec fumée de jour et de nuit.
- Carte Ville : immeubles classiques et modernes, terrasse, abribus, banc, grille, fontaine (jour), feux, tunnel (bouche, milieu, sortie, de jour et de nuit), virages 1 à 3, port (entrepôts, grue), rond-point : rendu correct, fenêtres éclairées la nuit, pluie de nuit visible.
- Carte Circuit : départ, tribunes avec public, paddock, raidillon, chicane, panneaux, de jour et de nuit : corrects (sauf n° 10 et 15).
- Carte Plage : corniche de nuit (enseigne, lampadaires, feu de plage), mouettes, palmiers, plage avec baigneurs, tunnel, talus : corrects (sauf n° 1 à 4, 8 et 13).
- Carte Campagne : autoroute, forêt, pins, vignes, village, animaux (vaches, moutons, chevaux), rond-point de jour : corrects (sauf n° 5 à 7).
- Aucune capture noire, aucune image cassée, aucun écran de plantage dans ce lot.
