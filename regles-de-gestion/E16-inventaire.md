# Inventaire des objets 3D à transformer — bibliothèque commune du décor

Fait le 2026-11-11 par 4 agents, en lecture seule (rien n'a été modifié dans le dépôt). Chaque ligne a été vérifiée dans le code.

**Actions :**
- **A** : convertir l'objet fabriqué par le code en fichier 3D, puis le retoucher dans Blender (objet déjà fait aux vraies mesures).
- **B** : remplacer par un vrai modèle 3D libre (CC0 / CC BY).
- **C** : garder dans le code (suit la route ou le terrain mètre par mètre, effet animé, bâtiment généré par règles).
- **D** : texture photo (sols, surfaces, façades).

## Totaux

| Partie | A convertir | B vrai modèle | C garder | D texture | Lignes |
|---|---|---|---|---|---|
| 1. Campagne, Grand Prix, cartes de données | 25 | 26 | 12 | 8 | 71 |
| 2. Corniche | 12 | 34 | 13 | 2 | 61 |
| 3. Ville | 21 | 14 | 24 | 6 | 65 |
| 4. Végétation, commun, hors cartes | 10 | 18 | 15 | 1 | 44 |
| **Total** | **68** | **92** | **64** | **17** | **≈ 241** |

Plusieurs objets reviennent sur plusieurs cartes et ne seront faits qu'une fois dans la bibliothèque : feu tricolore, lampadaire, tables et chaises de terrasse, parasol, store, fontaine, poteaux de glissière, luminaire de tunnel, rochers, palmier, pins.

## 1. Campagne, Grand Prix, cartes de données (agent artiste 3D)

Fichiers dans `src/world/` ; « modeles_c » = `decor_campagne_modeles.cpp`, « modeles_gp » = `decor_grandprix_modeles.cpp`.

| Objet | Carte | Fichier:ligne | Fait aujourd'hui avec | Action | Réutilisable ailleurs | Visibilité |
|---|---|---|---|---|---|---|
| Clôture rustique et enclos des chevaux | Campagne | campagne.cpp:324, 665 ; modeles_c:270, 193 | modèle du code | A convertir | toute carte rurale | forte |
| Rang de vigne palissé (piquets, ceps, fils) | Campagne | campagne.cpp:428 ; modeles_c:222, 238 | modèle du code | A convertir | oui | forte |
| Botte ronde de foin | Campagne | campagne.cpp:351 ; modeles_c:290 | modèle du code | A convertir | oui | moyenne |
| Corps de ferme | Campagne | campagne.cpp:345 ; modeles_c:332 | modèle du code | A convertir | oui | moyenne |
| Hangar agricole | Campagne | campagne.cpp:347 ; modeles_c:403 | modèle du code | A convertir | oui | moyenne |
| Poteau de ligne électrique et câbles | Campagne | campagne.cpp:365 ; modeles_c:499, 211 | modèle du code | A convertir | oui | forte |
| Maison de village | Campagne | campagne.cpp:207, 399 (secours) | kit d'architecture ; secours box + toit | A convertir | Ville, Corniche | forte |
| Haie champêtre (masse) | Campagne | campagne.cpp:337 | box + cartes de feuilles | B modèle libre | oui | forte |
| Éolienne (pales qui tournent) | Campagne | campagne.cpp:359, 711-727 | cyl + box | B modèle libre | oui | moyenne |
| Balise J6 à catadioptres | Campagne | campagne.cpp:380 | box | B modèle libre | toute route hors ville | forte |
| Panneau de la forêt | Campagne | campagne.cpp:392 | 3 box | B modèle libre | oui | forte |
| Panneau A1 « virage dangereux » | Campagne | campagne.cpp:646 | cyl + triangles plats | B modèle libre | oui | forte |
| Feu tricolore | Campagne | campagne.cpp:519, 125 | cyl + box + disques | B modèle libre (ou celui de la Ville) | Ville | forte |
| Lampadaire de village | Campagne + données | decor_carte.cpp:347-349 | 3 box | B modèle libre | socle commun | forte |
| Église (nef, clocher, flèche) | Campagne | campagne.cpp:410 | box + toit + cône | B modèle libre | oui | moyenne |
| Commerce : bâtiment, vitrines, enseigne | Campagne | campagne.cpp:525-532, 559 | box + toit | B modèle libre | Ville | forte |
| Store banne | Campagne | campagne.cpp:535-537 | 2 quads plats | B modèle libre | Ville, Corniche | forte |
| Tables rondes et chaises de terrasse | Campagne | campagne.cpp:539-542 | cyl + box | B modèle libre (ou ceux de la Ville) | Ville, Corniche | moyenne |
| Station-service : pompes, colonnes, auvent | Campagne | campagne.cpp:544-547 | box + cyl | B modèle libre | Ville | moyenne |
| Fontaine du rond-point | Campagne | campagne.cpp:476-479 | 4 cyl | B modèle libre (ou celle de la Ville) | oui | forte |
| Fleurs du massif | Campagne | campagne.cpp:495-504 | triangles plats | B modèle libre | oui | moyenne |
| Péage (auvent, cabines, barrières, lampes) | Campagne | campagne.cpp:587-613, 750 | box + disques | B modèle libre | oui | forte |
| Panneau bleu d'autoroute | Campagne | campagne.cpp:615 | box | B modèle libre | oui | forte |
| Poteaux de glissière | Campagne | campagne.cpp:634 | box | B modèle libre | oui | forte |
| Cascade (roche, chute d'eau, vasque) | Campagne | campagne.cpp:655 | box + quads | B modèle libre | Corniche | moyenne |
| Sommets du fond du col | Campagne | campagne.cpp:1068 | tronc de cône irrégulier | B modèle libre | oui | faible |
| Lisse de glissière | Campagne | campagne.cpp:632-633 | 2 quads plats (pas de profil en W) | C garder (profil à refaire) | oui | forte |
| Séparateur béton de l'autoroute | Campagne | campagne.cpp:580-584 | profil extrudé | C garder | oui | forte |
| Marquage de l'autoroute | Campagne | campagne.cpp:572-579 | quads au sol | C garder | oui | forte |
| Parapet de pierre de l'épingle | Campagne | campagne.cpp:637 | quads | C garder | oui | forte |
| Passage piéton | Campagne | campagne.cpp:419 | quads au sol | C garder | oui | forte |
| Giratoire | Campagne | decor_carte.cpp:405-414 ; campagne.cpp:488 | triangles sur le tracé | C garder | oui | forte |
| Forme des bas-côtés (fossé, champs, collines, roche) | Campagne | campagne.cpp:157 ; decor_carte.cpp:383-403 | bandes de quads | C garder | oui | forte |
| Ligne de départ en damier | toutes | decor_carte.cpp:375 | quads au sol | C garder | oui | forte |
| Parking du supermarché | Campagne | campagne.cpp:549-553 | couleur unie | D texture | oui | moyenne |
| Terre du massif fleuri | Campagne | campagne.cpp:491 | couleur unie | D texture | oui | moyenne |
| Sol de fond (prairie) | Campagne | campagne.cpp:1121 | 1 grand quad | D texture | oui | faible |
| Surfaces du sol (herbe, champs, terre, roche, enrobé) | Campagne | campagne.cpp:157-176 | couleur unie | D texture | oui | forte |
| Mur de pneus sanglé | Grand Prix | circuit.cpp:643 ; modeles_gp:214 | modèle du code | A convertir | oui | forte |
| Panneau publicitaire (bâche, cadre) | Grand Prix | circuit.cpp:646 ; modeles_gp:246 | modèle du code | A convertir | Ville | forte |
| Tribune (gradins, charpente, sièges) | Grand Prix | circuit.cpp:661 ; modeles_gp:293 | modèle du code | A convertir | stade | forte |
| Mur des stands | Grand Prix | circuit.cpp:701 ; modeles_gp:351 | modèle du code | A convertir | non | forte |
| Stand d'écurie | Grand Prix | circuit.cpp:704 ; modeles_gp:380 | modèle du code | A convertir | non | forte |
| Drapeaux (mât, à la main, animés) | Grand Prix | circuit.cpp:710, 592 ; modeles_gp:428, 440 | modèle du code | A convertir | oui | moyenne |
| Poste de commissaire | Grand Prix | circuit.cpp:711 ; modeles_gp:448 | modèle du code | A convertir | non | moyenne |
| Panneau de freinage 150/100/50 | Grand Prix | circuit.cpp:713 ; modeles_gp:480 | modèle du code | A convertir | non | forte |
| Portique des feux de départ | Grand Prix | circuit.cpp:730, 575 ; modeles_gp:495, 515 | modèle du code | A convertir | non | forte |
| Passerelle publicitaire | Grand Prix | circuit.cpp:731 ; modeles_gp:523 | modèle du code | A convertir | non | forte |
| Bâches de la passerelle | Grand Prix | circuit.cpp:737 | quad plat | A convertir (bâche du panneau pub) | non | forte |
| Bar d'accueil et terrasse VIP | Grand Prix | circuit.cpp:743 ; modeles_gp:610 | modèle du code | A convertir | non | moyenne |
| Tour de contrôle | Grand Prix | circuit.cpp:752 ; modeles_gp:667 | modèle du code | A convertir | non | moyenne |
| Stand de l'espace fans et portail | Grand Prix | circuit.cpp:767 ; modeles_gp:756, 787 | modèle du code | A convertir | marché | moyenne |
| Grande roue | Grand Prix | circuit.cpp:778, 585 ; modeles_gp:799, 829, 852 | modèle du code | A convertir | Corniche | moyenne |
| Tente dôme du camping | Grand Prix | circuit.cpp:783 ; modeles_gp:870 | modèle du code | A convertir | oui | faible |
| Grillage à maille losangée | Grand Prix | circuit.cpp:784 ; modeles_gp:923 | modèle du code | A convertir | oui | moyenne |
| Projecteur de piste | Grand Prix | circuit.cpp:785 ; modeles_gp:964 | modèle du code | A convertir | stade | forte |
| Camion d'équipe (semi-remorque) | Grand Prix | circuit.cpp:742 ; modeles_gp:544 | modèle du code | B modèle libre | trafic | moyenne |
| Hélicoptère posé | Grand Prix | circuit.cpp:759 ; modeles_gp:703 | modèle du code | B modèle libre | oui | moyenne |
| Oiseaux en vol | Grand Prix | circuit.cpp:608 ; modeles_gp:939, 953 | modèle du code | B modèle libre | toutes | faible |
| Grille de départ peinte | Grand Prix | circuit.cpp:722 | quads au sol | C garder | non | forte |
| Vibreurs | Grand Prix | circuit.cpp:194 | bande du sol | C garder | non | forte |
| Aire de l'hélicoptère (H peint) | Grand Prix | circuit.cpp:761-763 | quads | D texture | oui | faible |
| Herbe tondue, bac à gravier, voie des stands | Grand Prix | circuit.cpp:185-200 | couleur unie | D texture | oui | forte |
| Chaussée du circuit | Grand Prix | decor_carte.cpp:167-181 | texture calculée | D texture | non | forte |
| Chalet ou maison (Noël, Automne) | Données | cartes_v2/carte_donnees_decor.cpp:109 | box + toit | B modèle libre | oui | forte |
| Banc enneigé | Données | cartes_v2/carte_donnees_decor.cpp:114-119 | box | B modèle libre | oui | moyenne |
| Borne de rue, fontaine de la place | Données | cartes_v2/carte_donnees_decor.cpp:114-119 | cyl | B modèle libre | oui | moyenne |
| Ampoules des guirlandes, fenêtres éclairées | Données | cartes_v2/carte_donnees_decor.cpp:118 | losange émissif | B modèle libre | oui | forte (nuit) |
| Muret de pierre (Automne) | Données | cartes_v2/carte_donnees_decor.cpp:114-119 | box le long de la route | C garder | oui | forte |
| Tunnel ou abri | Données | cartes_v2/carte_donnees_decor.cpp:96 | box | C garder | oui | forte |
| Sol enneigé ou d'automne | Données | cartes_v2/carte_donnees_decor.cpp:52 | couleur unie | D texture | oui | forte |

Végétation par carte (voir partie Commun) : Campagne = chêne, sapin, pin sylvestre, cyprès, sous-bois, touffes, haies, vigne ; Grand Prix = sapin, chêne, touffes ; données = sapin, feuillu, cyprès, touffes. Déjà de vrais modèles : vaches, chevaux, moutons, voitures, spectateurs.

**Totaux Campagne / Grand Prix / données** : A = 25 · B = 26 · C = 12 · D = 8 (71 lignes).
## 2. Corniche (agent expert réalisme 3D)

| Objet | Carte | Fichier:ligne | Fait aujourd'hui avec | Action | Réutilisable ailleurs | Visibilité |
|---|---|---|---|---|---|---|
| Forme du terrain des bas-côtés (promenade, plage, dune, collines, côte rocheuse) | Corniche | src/world/plage.cpp:261, :963, :2297-2316 | quad en couleur unie | C garder | non | forte |
| Revêtements des sols : sable, sable mouillé, promenade, dune, collines, herbe, garrigue, gazon | Corniche | src/world/plage.cpp:194, :2316 | aplats d'une couleur | D texture | Ville, Campagne, Grand Prix | forte |
| Roche : falaise, éboulis, paroi, talus, massif, têtes du tunnel, muret | Corniche | src/world/plage.cpp:1013-1040, :2099-2106, :2129-2146 | quads + texture de roche | C garder | non | forte |
| Texture de roche calcaire | Corniche | src/world/plage.cpp:398 | texture calculée 256 × 256 | D texture | Campagne, tunnels | forte |
| Quai de la marina | Corniche | src/world/plage.cpp:261-272 | quad, profil | C garder | non | moyenne |
| Mer et ressac | Corniche | src/world/plage.cpp:2329, :2342 | shader d'eau | C garder | rivière de la Ville | forte |
| Blocs d'enrochement et galets | Corniche | src/world/plage.cpp:2348-2361 ; src/render/eau_rivage.cpp:35 | icosaèdre déformé | B modèle libre | rivière, digues | moyenne |
| Trottoirs et bordures | Corniche | src/world/plage.cpp:2216 | bande le long de la route | C garder | Ville | forte |
| Bouchons de sol des virages serrés | Corniche | src/world/plage.cpp:2376 | disque | C garder | non | faible |
| Voûte du tunnel | Corniche | src/world/plage.cpp:2109-2117 | profil en arc | C garder | autres tunnels | forte |
| Luminaires sodium du tunnel | Corniche | src/world/plage.cpp:2118-2127 | box émissive | B modèle libre | tous les tunnels | moyenne |
| Rue transversale du carrefour | Corniche | src/world/plage.cpp:2202-2209 | quads | C garder | non | moyenne |
| Giratoire (anneau, couronne pavée, bordure, îlot, marquages) | Corniche | src/world/giratoire.cpp:67-153 ; plage.cpp:2365, :2149, :2167 | triangles au sol | C garder | tout giratoire | forte |
| Glissière double onde (lisse) | Corniche | src/world/plage.cpp:2090-2093 | 4 quads | C garder | Campagne, montagne | forte |
| Poteaux de glissière | Corniche | src/world/plage.cpp:2096, :1292-1297 | box | B modèle libre | toutes les routes | moyenne |
| Ancre géante du rond-point | Corniche | src/world/plage.cpp:2174-2185 | box + boule + tri | B modèle libre | non | forte |
| Palmiers | Corniche | src/world/plage.cpp:2402, :2189, :1568 → vegetation.cpp:136 | veg::palmier | B modèle libre | Ville, Grand Prix | forte |
| Gaines des palmes, noix de coco | Corniche | src/world/plage.cpp:2403-2406 | boules | B modèle libre | avec le palmier | moyenne |
| Lampadaire de plage (poteau 5,8 m + boule) | Corniche | src/world/plage.cpp:2410-2416 | box + boule | B modèle libre | Ville, promenades | forte |
| Parasol de plage | Corniche | src/world/plage.cpp:2418-2423 | cylindre + cône | B modèle libre | Ville | moyenne |
| Transat | Corniche | src/world/plage.cpp:2424, :1735 | box | B modèle libre | oui | moyenne |
| Villas de secours | Corniche | src/world/plage.cpp:1233-1245 | box + quad + tri | B modèle libre | Ville, Campagne | faible |
| Tour de sauveteur | Corniche | src/world/plage.cpp:1247-1252 | box | B modèle libre | non | moyenne |
| Bitte d'amarrage en fonte | Corniche | src/world/marina_bateaux.cpp:430 | modélisé | A convertir | ports, rivière | faible |
| Ponton flottant | Corniche | src/world/marina_bateaux.cpp:437 | modélisé, 2 LOD | A convertir | rivière | moyenne |
| Voilier de port (mesures Oceanis 30.1), 3 LOD | Corniche | src/world/marina_bateaux.cpp:174 | modélisé | A convertir | oui | forte |
| Yacht 18 m à flybridge et vedette | Corniche | src/world/marina_bateaux.cpp:286 | modélisé | A convertir | oui | forte |
| Amarres, pendille, passerelle | Corniche | src/world/marina_bateaux.cpp:414 | lignes | A convertir | avec les bateaux | faible |
| Barques échouées | Corniche | src/world/plage.cpp:1273-1280 | 2 box | B modèle libre | rivière, lac | moyenne |
| Rochers de la côte rocheuse | Corniche | src/world/plage.cpp:1287-1289 | boule aplatie | B modèle libre | Campagne, montagne | forte |
| Pins (2 cônes) et arbres ronds de la côte rocheuse | Corniche | src/world/plage.cpp:1299-1309 | cylindre + cône / boule | B modèle libre | Campagne | forte |
| Voiliers et yachts au large | Corniche | src/world/plage.cpp:447, :456 | coque + box + tri | A convertir (reprendre la marina + voiles) | oui | moyenne |
| Jet-ski et pilote | Corniche | src/world/plage.cpp:464 | coque + box + cylindre + boule | B modèle libre | lac | moyenne |
| Paquebot au large | Corniche | src/world/plage.cpp:473 | coque + 6 box | B modèle libre | non | moyenne |
| Sillage, écume des dauphins | Corniche | src/world/plage.cpp:500, :1949 | shader | C garder | rivière | faible |
| Mouettes et dauphins | Corniche | src/world/plage.cpp:1924-1948 | déjà `.wfa` | A retoucher | port, rivière | moyenne |
| Scène de concert | Corniche | src/world/plage.cpp:1410-1420 | box | B modèle libre | Ville | forte |
| Guirlandes de fête | Corniche | src/world/plage.cpp:1421-1434, :1725-1729 | cylindre + boules | B modèle libre | Ville, Noël | moyenne |
| Lyres de scène et faisceaux | Corniche | src/world/plage.cpp:1349-1354, :1525-1535 | boule + cône | B modèle libre | oui | moyenne |
| Feu de camp | Corniche | src/world/plage.cpp:1436-1438 | boule + box + cône | B modèle libre | Campagne | faible |
| Lignes du beach-volley | Corniche | src/world/plage.cpp:1442-1446 | quad au sol | C garder | non | faible |
| Filet, poteaux, ballon de beach-volley | Corniche | src/world/plage.cpp:1447-1453 | quad + box + cylindre + boule | B modèle libre | non | moyenne |
| Château de sable | Corniche | src/world/plage.cpp:1455-1458 | cône + cylindre | B modèle libre | non | faible |
| Pédalos | Corniche | src/world/plage.cpp:1460-1465 | box + quad | B modèle libre | lac | moyenne |
| Toboggan gonflable | Corniche | src/world/plage.cpp:1467-1471 | box + quad + boule | B modèle libre | non | moyenne |
| Paillote du bar de plage | Corniche | src/world/plage.cpp:1676-1706, :1722 | box texturées (chaume, bois) | A convertir | non | forte |
| Bouteilles du comptoir | Corniche | src/world/plage.cpp:1707-1708 | cylindres | B modèle libre | bars de la Ville | faible |
| Tabourets de bar | Corniche | src/world/plage.cpp:1710-1714 | cylindre + disque | B modèle libre | oui | faible |
| Tables basses et poufs | Corniche | src/world/plage.cpp:1716-1720 | cylindre + boule | B modèle libre | oui | faible |
| Parasols de paille | Corniche | src/world/plage.cpp:1736-1737 | cylindre + cône | B modèle libre | non | moyenne |
| Enseignes lumineuses | Corniche | src/world/plage.cpp:1639, :1723, :1780-1783, :1823, :1866, :1896 | quad + texte | C garder | code commun | moyenne |
| Casino (façade, pilastres, balustrade, colonnes, dôme, lanternon) | Corniche | src/world/plage.cpp:1744-1786 | box + cylindre + boule (dôme) | A convertir | non | forte |
| Candélabres du parvis du casino | Corniche | src/world/plage.cpp:1788-1791 | cylindre + boule | B modèle libre | Ville | moyenne |
| Cafés (corps, toit, terrasse, vitrine, store, volets) | Corniche | src/world/plage.cpp:1797-1828 | box + quad + tri | A convertir | Ville | forte |
| Tables de bistrot, chaises de rotin, bougies | Corniche | src/world/plage.cpp:1832-1843 | cylindre + box | B modèle libre (ou ceux de la Ville) | Ville | moyenne |
| Galerie marchande vitrée | Corniche | src/world/plage.cpp:1847-1870 | box + vitre | A convertir | Ville | forte |
| Bacs de plantes | Corniche | src/world/plage.cpp:1862-1865 | box + boule | B modèle libre | Ville | faible |
| Boutique de location de scooters | Corniche | src/world/plage.cpp:1885-1897 | box + vitre | A convertir | Ville | moyenne |
| Scooters 125 garés | Corniche | src/world/plage.cpp:1874-1883, :1900-1905 | ellipsoïde + box | B modèle libre | Ville | moyenne |
| Feux tricolores latéraux | Corniche | src/world/plage.cpp:2193-2200, :2493 | cylindre + box + boule | B modèle libre (ou celui de la Ville) | Ville | forte |

Hors de cette partie mais visibles sur la Corniche : villas du kit (`archi_carte.cpp`), commerces, enseignes de rue, piétons et serviettes. Portique de checkpoint : partie Commun.

**Totaux Corniche** : A = 12 · B = 34 · C = 13 · D = 2 (61 lignes).
## 3. Ville (agent dev artiste 3D)

« modélisé » = déjà fait au niveau du vrai objet (profils tournés, tubes, pavés à coins arrondis) ; « forme nue » = box, cyl, tri, quad, disque. Fichiers dans `src/world/`.

| Objet | Carte | Fichier:ligne | Fait aujourd'hui avec | Action | Réutilisable ailleurs | Visibilité |
|---|---|---|---|---|---|---|
| Immeubles d'habitation et tours (Classique à balcons et toit à la Mansart, Moderne à toit-terrasse), fenêtres, portes, volets, garde-corps, corniches, cheminées | Ville | ville.cpp:120-127 ; archi_geo.cpp:147-222 ; archi_details.cpp:119-285 ; archi_ouvertures.cpp:201-574 | kit d'architecture (bâtiment généré par règles) | C règles | toutes les cartes | forte |
| Murs des immeubles (béton, brique) | Ville | archi_carte.cpp:120-124 | appareil procédural (decor.glsl) | D texture | toutes les cartes | forte |
| Fenêtre en bois (dormant, ouvrants, petits bois, crémone) | Ville | archi_ouvertures.cpp:186, 201 | modélisé | A fichier | toutes les cartes | moyenne |
| Porte d'immeuble, porte de garage sectionnelle, vitrine | Ville | archi_ouvertures.cpp:248, 326, 364, 390 | modélisé | A fichier | toutes les cartes | moyenne |
| Persienne, volet plein, volet roulant | Ville | archi_ouvertures.cpp:420-525 | modélisé | A fichier | Corniche, Campagne | moyenne |
| Appui et linteau de pierre, encadrement | Ville | archi_ouvertures.cpp:526-558 | modélisé | A fichier | toutes les cartes | faible |
| Garde-corps de balcon | Ville | archi_details.cpp:119 | barreaux en pavés | A fichier | toutes les cartes | moyenne |
| Mitron de cheminée | Ville | archi_details.cpp:88 | prisme à 8 pans | A fichier | Campagne, Corniche | faible |
| Entrepôts du port (brique, baies d'atelier) | Ville | ville.cpp:128-133 | kit d'architecture | C règles | Grand Prix, Campagne | forte |
| Ancien immeuble de secours (`WFS_ARCHI=0`) | Ville | ville.cpp:382-395 | box + texture | C (à retirer) | non | faible |
| Ancien entrepôt de secours | Ville | ville.cpp:457-465 | box + toit | C (à retirer) | non | faible |
| Immeubles des abords du rond-point | Ville | ville.cpp:657-659 | box + texture de façade | C (à passer au kit) | non | moyenne |
| Fenêtres allumées la nuit (façades en boîte) | Ville | ville.cpp:352, 387, 659 | quads émissifs | D texture | non | moyenne |
| Lampadaire de rue (mât conique 7,4 m, crosse, luminaire) | Ville | mobilier_ville.cpp:298 ; ville.cpp:397 | modélisé | A fichier | toutes les cartes | forte |
| Grille d'arbre 1,3 × 1,3 m | Ville | ville.cpp:404 | box plate | B modèle libre | toutes les villes | faible |
| Passage piéton | Ville | ville.cpp:410-416, 678-683 | quads au sol | C route | toutes les cartes | forte |
| Ligne d'arrêt, rue transversale, tirets du carrefour | Ville | ville.cpp:693-706 | quads au sol | C route | toutes les cartes | forte |
| Abribus | Ville | mobilier_ville.cpp:233 ; ville.cpp:418 | modélisé | A fichier | Corniche, Campagne | moyenne |
| Grille du parc | Ville | mobilier_ville.cpp:265 ; ville.cpp:424 | modélisé (suit la route) | C route (poteau et travée : A) | Campagne | moyenne |
| Banc public à lattes, flancs en fonte | Ville | mobilier_ville.cpp:210 ; ville.cpp:447 | modélisé | A fichier | toutes les cartes | moyenne |
| Fontaine (bassin, piédestal, vasque) | Ville | mobilier_ville.cpp:281 ; ville.cpp:452, 668 | modélisé | A fichier | Corniche, Campagne | moyenne |
| Jet d'eau de la fontaine (animé) | Ville | ville.cpp:274-284, 1709-1715 | cyl + tri + quad translucides | B modèle libre (effet d'eau) | Corniche | faible |
| Îlot du rond-point de secours | Ville | ville.cpp:663-666 | cyl + disque | C route | non | faible |
| Ouvrage du rond-point | Ville | ville.cpp:624-634 (giratoire.cpp) | triangles | C route | Corniche | forte |
| Panneau « sens giratoire » B21c1 | Ville | mobilier_ville.cpp:311 ; ville.cpp:603 | modélisé | A fichier | toutes les cartes | moyenne |
| Flèches blanches de ce panneau | Ville | ville.cpp:606-610 | tri plats | D texture | toutes les cartes | faible |
| Feu tricolore R11 et optiques | Ville | mobilier_ville.cpp:322, 326 ; ville.cpp:687, 1700 | modélisé | A fichier | Corniche, Campagne | forte |
| Plaque de rue bleue | Ville | ville.cpp:689 | box | B modèle libre | toutes les villes | faible |
| Conteneur ISO 40 pieds | Ville | decor_ville_port.cpp:62 ; ville.cpp:467-472 | modélisé | A fichier | Corniche, Grand Prix | forte |
| Portique de quai (grue) | Ville | decor_ville_port.cpp:54, 114 ; ville.cpp:481-485 | modélisé | A fichier | Corniche | forte |
| Clôture du port à double fil | Ville | decor_ville_port.cpp:190 ; ville.cpp:474-479 | modélisé (suit la route) | C route (panneau : A) | Campagne, Grand Prix | moyenne |
| Trottoir et bordure | Ville | ville.cpp:493-519, 912-951 | bandes le long de la route | C route + D texture | toutes les cartes | forte |
| Bas-côtés, pelouse, béton du port, carreaux de sol | Ville | ville.cpp:521-545, 974-1020 | bandes et carreaux | C route + D texture | toutes les cartes | forte |
| Chaussée opposée de l'autoroute et marquage | Ville | ville.cpp:1042-1060 | quads | C route | Corniche | forte |
| Terre-plein central et séparateur « New Jersey » | Ville | ville.cpp:1062-1067 | profil extrudé | C route | Corniche, Campagne | forte |
| Glissière en W et supports | Ville | ville.cpp:1033-1040 | profil extrudé + box | C route (support : A) | toutes les cartes | forte |
| Écran antibruit et poteaux HEA | Ville | ville.cpp:1070-1073 | extrudé + box | C route (poteau : A) | Campagne | moyenne |
| Candélabre d'autoroute de 12 m | Ville | ville.cpp:1074-1082 | cyl + box | B modèle libre | Corniche, Campagne | forte |
| Tunnel : tubes, voûte, parements, têtes, murs | Ville | ville.cpp:1086-1135 | profils extrudés + quads | C route + D texture | Corniche | forte |
| Luminaire de tunnel au sodium | Ville | ville.cpp:1104-1111 | box | B modèle libre | Corniche | moyenne |
| Pont : corniche, tablier, quais | Ville | ville.cpp:1136-1139, 1166-1179 | extrudé + quads | C route | Campagne | forte |
| Garde-corps du pont | Ville | ville.cpp:1140-1144 | box | C route (montant : A) | Corniche, Campagne | moyenne |
| Candélabre du pont de 8 m | Ville | ville.cpp:1145-1151 | cyl + box | B modèle libre | Corniche | moyenne |
| Pile centrale à avant-becs | Ville | ville.cpp:1180-1187 | box + cyl | C route | Campagne | faible |
| Arcs bow-string, tirants, suspentes | Ville | ville.cpp:1188-1212 | caissons + cyl | C route | non | forte |
| Rivière | Ville | ville.cpp:1156-1165 | quads, matériau eau | C terrain + D | Campagne | moyenne |
| Portique de signalisation | Ville | ville.cpp:1215-1246 | cyl + box + quad + textes | B modèle libre + D (panneau) | Corniche, Campagne | forte |
| Casino | Ville | ville.cpp:1382-1403 | blocs + quads + texte | C (à passer au kit) + D façade | Corniche | forte |
| Centre commercial | Ville | ville.cpp:1406-1424 | blocs + texte | C (à passer au kit) | Campagne | forte |
| Parking du centre commercial | Ville | ville.cpp:1410-1415 | quads au sol | D texture | Grand Prix, Campagne | moyenne |
| Garage « GARAGE 88 » | Ville | ville.cpp:1427-1446 | blocs + quads | C (à passer au kit) | Campagne | moyenne |
| Rideau métallique enroulé | Ville | ville.cpp:1437 | cylindre | B modèle libre | Campagne | faible |
| Pont élévateur à 2 colonnes | Ville | ville.cpp:1438-1442 | blocs | B modèle libre | Grand Prix | faible |
| Voitures garées, voitures en sens inverse | Ville | ville.cpp:1319, 794 | `.wfm` | déjà fichier | toutes | moyenne |
| Pigeons | Ville | ville.cpp:1332 | `mouette.wfa` réduite | B modèle libre (vrai pigeon) | Corniche | faible |
| Chiens shiba et husky | Ville | ville.cpp:1339 | `.wfa` | déjà fichier | toutes | faible |
| Laisse du chien | Ville | ville.cpp:1342 | box fine | B modèle libre | toutes | faible |
| Devanture de commerce (cadre, vitre, store, lambrequin) | Ville | commerces.cpp:214-220 | boîtes | B modèle libre (store) + D (vitrine) | Corniche | forte |
| Enseigne de commerce | Ville | commerces.cpp:222-226 | boîte + texte | D texture | Corniche | forte |
| Table de bistrot | Ville | mobilier_ville.cpp:343 ; commerces.cpp:242 | modélisé | A fichier | Corniche, Campagne | moyenne |
| Chaise de bistrot (cannage) | Ville | mobilier_ville.cpp:352 ; commerces.cpp:244 | modélisé | A fichier | Corniche, Campagne | moyenne |
| Parasol de terrasse (ouvert / fermé) | Ville | mobilier_ville.cpp:374-429 ; commerces.cpp:245-246 | modélisé | A fichier | Corniche | moyenne |
| Photophore de table (nuit) | Ville | mobilier_ville.cpp:430 ; commerces.cpp:248 | modélisé | A fichier | Corniche | faible |
| Jardinière en terre cuite et buis | Ville | mobilier_ville.cpp:434 ; commerces.cpp:255 | modélisé | A fichier | Corniche, Campagne | faible |
| Panneau publicitaire néon de la route | Ville (et Corniche) | enseignes.cpp:192-225 | box + texte | B modèle libre + D (affiche) | Corniche, Grand Prix | forte |

Platanes, tilleuls, pins du parc : voir la partie Végétation. Piétons et clients : partie Commun.

**Totaux Ville** : A = 21 · B = 14 · C = 24 (dont 5 bâtiments en boîte à passer au kit) · D = 6 seules (+ 8 en 2e action) · déjà en fichier = 3.
## 4. Végétation, objets communs et hors des cartes (agent expert cartes)

| Objet | Zone | Fichier:ligne | Fait aujourd'hui avec | Action | Réutilisable ailleurs | Visibilité |
|---|---|---|---|---|---|---|
| Feuillu (tronc, branches, houppier, bouquets) | Végétation | src/world/vegetation.cpp:176 | cylindres effilés + icosaèdre + cartes alpha | B vrai modèle | Ville, Campagne, Grand Prix, parcs | forte |
| Platane d'alignement (2 teintes) | Végétation | src/world/vegetation.cpp:215 | même générateur que le feuillu | B vrai modèle | Ville, Corniche, villages | forte |
| Palmier (stipe courbé, palmes vertes et sèches) | Végétation | src/world/vegetation.cpp:136 | tronçons de cylindre + bandes alpha | B vrai modèle | Corniche, marina, Ville | forte |
| Sapin / épicéa | Végétation | src/world/vegetation.cpp:224 | cylindre + étages de cartes + icosaèdre | B vrai modèle | Campagne, Grand Prix | forte |
| Pin sylvestre et pin parasol | Végétation | src/world/vegetation.cpp:272 | cylindre + plateaux de cartes | B vrai modèle | Corniche, Campagne, Grand Prix | forte |
| Cyprès | Végétation | src/world/vegetation.cpp:305 | fuseau de cartes + icosaèdre | B vrai modèle | Campagne, Corniche | moyenne |
| Sous-bois (fougère, buisson) | Végétation | src/world/vegetation.cpp:328 | cartes alpha croisées | B vrai modèle | toutes les forêts | moyenne |
| Touffe d'herbe | Végétation | src/world/vegetation.cpp:341 | 2 cartes croisées | B vrai modèle | toutes | faible |
| Feuillage de haie | Végétation | src/world/vegetation.cpp:401 | cartes alpha sur un volume | B vrai modèle | Campagne, villages, Ville | moyenne |
| Feuillage de vigne palissée | Végétation | src/world/vegetation.cpp:354 | masse projetée + cartes | B vrai modèle | Campagne | moyenne |
| Disques de sol au centre des épingles | Sol | src/world/bouchons_sol.h:38 | disques plats | C garder (terrain) | toute carte à épingles | faible |
| Chaussée, trottoir, sol en morceaux | Sol | src/game/game.cpp:167 | maillages qui suivent le tracé | C garder (route) | toutes | forte |
| Gouttes de pluie | Météo | src/world/meteo.cpp:90 | quads 2D | C garder (effet) | toutes | moyenne |
| Gouttes sur le pare-brise | Météo (Cockpit) | src/world/meteo.cpp:214 | quads 2D | C garder (effet) | toutes | moyenne |
| Flocons de neige | Météo | src/world/meteo.cpp:234 | quads 2D | C garder (effet) | hiver | moyenne |
| Poussette | Personnes | src/world/foule_poussette.cpp:25-62 | cylindres, boîtes, quads | B vrai modèle | toutes | moyenne |
| Casque des pilotes de deux-roues | Trafic | src/game/deux_roues.cpp:168-174 | morceaux d'ellipsoïde | B vrai modèle | tout le trafic deux-roues | moyenne |
| Portique du checkpoint | Course | src/world/portique.cpp:69-74, 95 | boîtes | A convertir puis retoucher | toutes | forte |
| Rampe de gyrophares de la police | Poursuite | src/game/poursuite_system.cpp:365-367 | cube mis à l'échelle | B vrai modèle | toutes | forte |
| Bandes « POLICE » des portières | Poursuite | src/game/poursuite_system.cpp:358 | quad plat + texture du code | D texture | voitures de police | moyenne |
| Cône de chantier K5a | Poursuite | src/game/poursuite_system.cpp:99-134 | troncs de cône + boîte | B vrai modèle | toutes | moyenne |
| Panneau AK5 « travaux » | Poursuite | src/game/poursuite_system.cpp:383-386 | 2 boîtes + 2 triangles plats | B vrai modèle | toutes | moyenne |
| Ralentisseur + 2 plots | Poursuite | src/game/poursuite_system.cpp:403-406 | boîtes | B vrai modèle | Ville, villages | moyenne |
| Halos des gyrophares | Effets | src/game/poursuite_system.cpp:369-370 | carré face caméra | C garder (effet) | police | moyenne |
| Halos des clignotants du trafic | Effets | src/game/traffic_system.cpp:317-350 | carrés face caméra | C garder (effet) | trafic | moyenne |
| Lueur des phares et feux | Effets | src/render/renderer.cpp:1553-1560 | carrés face caméra | C garder (effet) | toutes les voitures | forte la nuit |
| Halo des lampadaires | Effets | src/render/nuit.cpp:269 | carré face caméra | C garder (effet) | toutes | moyenne |
| Ombre de contact des véhicules | Effets | src/game/game.cpp:392 ; traffic_system.cpp:175, 232, 243 | rectangle semi-transparent | C garder | tous les véhicules | faible |
| Étincelles et fumée des pneus | Effets | src/render/effets_systeme.cpp:237, 271 | particules | C garder (effet) | toutes | moyenne |
| Lignes de vitesse (nitro) | Effets | src/render/effets_systeme.cpp:282-301 | quads fins | C garder (effet) | toutes | moyenne |
| Garage : sol et plateau tournant | Garage 3D | src/ui/garage.cpp:118, 175 | disques | B vrai modèle | non | forte |
| Garage : anneau néon du plateau | Garage 3D | src/ui/garage.cpp:176, 265 | tore | A convertir puis retoucher | non | forte |
| Garage : barres néon du fond | Garage 3D | src/ui/garage.cpp:176, 271 | boîtes de 9 m | B vrai modèle | non | moyenne |
| Cockpit commun de secours | Habitacle | src/game/camera_vue.cpp:281-292 | boîtes, disques, tore, cylindre | A convertir puis retoucher | voitures sans intérieur | forte (Cockpit) |
| Planche de bord modelée | Habitacle | src/render/vehicule_cockpit.cpp:294 | profils extrudés chanfreinés | A convertir puis retoucher | tous les intérieurs | forte (Cockpit) |
| Aérateurs | Habitacle | src/render/vehicule_cockpit.cpp:73, 331 | dalles arrondies | A convertir puis retoucher | tous les intérieurs | moyenne |
| Boîte à gants, sac gonflable passager | Habitacle | src/render/vehicule_cockpit.cpp:340-353 | pièces moulées par le code | A convertir puis retoucher | tous les intérieurs | faible |
| Console centrale, sélecteur de vitesse | Habitacle | src/render/vehicule_cockpit.cpp:355-364 | pièces moulées par le code | A convertir puis retoucher | tous les intérieurs | moyenne |
| Montants, traverse, rails de toit, pare-soleil | Habitacle | src/render/vehicule_cockpit.cpp:366-416 | pièces arrondies | A convertir puis retoucher | tous les intérieurs | forte (Cockpit) |
| Rétroviseur intérieur | Habitacle | src/render/vehicule_cockpit.cpp:418 | coque faite par le code | B vrai modèle | toutes les voitures | moyenne |
| Contre-portes, accoudoirs, plancher, tablier | Habitacle | src/render/vehicule_cockpit.cpp:438-470 | blocs pleins | A convertir puis retoucher | tous les intérieurs | moyenne |
| Volant (jante, moyeu, branches, boutons) | Habitacle | src/render/vehicule_cockpit_volant.cpp:15, 53 | tore aplati + profils | A convertir puis retoucher | tous les intérieurs | forte (Cockpit) |
| Essuie-glaces | Habitacle | src/render/vehicule_cockpit.cpp:485-495 | profils du code | B vrai modèle | toutes les voitures | moyenne |
| Compteurs et écrans | Habitacle | src/render/vehicule_cockpit.cpp:131-190 | dessin 2D animé | C garder (affichage) | tous les combinés | forte (Cockpit) |

Hors tableau : piétons et spectateurs déjà en `.wfp` ; interface 2D permise ; `dessineLoin` (voiture en cubes du trafic, traffic_system.cpp:310) n'est plus appelée ; formes des bancs d'essai jamais vues en jeu ; gaines et noix du palmier : partie Corniche.

**Totaux commun** : A = 10 · B = 18 · C = 15 · D = 1 (44 lignes).
