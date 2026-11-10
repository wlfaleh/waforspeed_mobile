# WFS-373 — inventaire des formes nues du Grand Prix (RG-01)

Relevé sur `main` (117dd0e), carte `circuit`, tracé 2 (5 159 segments, 6 191 m). Nombres : journal du banc
(`Circuit (WFS-373) : objets posés par sorte`, ajouté par ce ticket). Captures « avant » : `qa/pr/WFS-373/avant-*.jpg` (à la PR).

| Objet (type dans `circuit.cpp`) | Nombre | Dessin actuel (forme nue) | Remplacement prévu (RG-02 / RG-03) |
|---|---|---|---|
| Mur de pneus `T_TIRES` | 2 068 (4,8 m chacun) | 12 cylindres Ø 1,2 m (2× trop gros) | piles de 4 pneus 205/55 R16 (Ø 632 mm, 205 mm), flancs bombés et bande de roulement, façade en bande transporteuse boulonnée et sanglée (ANO-220) |
| Panneau publicitaire `T_ADBOARD` | 103 | boîte + 2 aplats | cadre en profilés d'aluminium, montants, jambes de force et platines, bâche tendue |
| Tribune `T_STAND` | 2 | gradins en boîtes, sièges en 2 pavés, toit en plaque | gradins de béton chanfreinés à nez de marche, sièges coques moulés, garde-corps en tubes, charpente en profilés I, couverture ondulée, bardage du fond |
| Mur des stands `T_PITWALL` | 517 (3,6 m) | boîte + plaque « vitre » | muret de béton préfabriqué chanfreiné, grillage de protection (poteaux tubes, câbles, maille) |
| Garage des stands `T_GARAGE` | 107 | 3 boîtes | stand vitré (ANO-220) : rideaux métalliques à lames, étage en façade vitrée à meneaux, auvent en porte-à-faux sur consoles, acrotère, bandeau d'équipe |
| Drapeau `T_FLAG` + drapeaux des tribunes | 51 + 5 animés | cylindre + boîte | mât conique en aluminium, platine, pomme, toile ondulée |
| Poste de commissaire `T_MARSHAL` | 34 | 2 boîtes | plateforme sur ossature, garde-corps, abri à toit à pente, marches |
| Panneau de freinage `T_FREIN` | 3 | 3 boîtes | panneau à cadre chanfreiné sur 2 poteaux tubes à platines |
| Portique de départ `T_PORTIQUE` + feux | 1 | 4 boîtes + 5 « disques » en boîte | pylônes et poutre en treillis de tubes, caisson à 5 feux avec visières et lentilles bombées |
| Passerelle `T_PASSERELLE` | 1 | 6 boîtes | piles chanfreinées, tablier caisson, poutres en treillis, garde-corps |
| Camion d'équipe `T_CAMION` | 21 | 4 boîtes + 12 cylindres | tracteur (cabine galbée, pare-brise, calandre sans marque, rétroviseurs), semi-remorque 13,6 m chanfreinée, jupes, roues modélisées |
| Bar d'accueil `T_ACCUEIL` | 8 | boîtes + parasols en cônes | façade vitrée à meneaux, nez de dalle, garde-corps vitrés, parasols à baleines et toile en 8 pans |
| Tour de contrôle `T_TOUR` | 1 | boîtes + cylindre | fût de béton à pilastres, vigie à vitres inclinées, toit débordant, mât d'antenne en treillis |
| Hélicoptère `T_HELICO` | 1 | 9 boîtes | hélicoptère léger sans marque (fuselage profilé, poutre de queue, dérive, patins tubes, rotors à 3 et 2 pales) |
| Stand de l'espace fans `T_STANDFAN` | 24 | boîtes | stand de marché : comptoir, ossature, toit de toile à 2 pans et lambrequin festonné ; portails en treillis |
| Grande roue `T_ROUE` | 1 | jante en aplats, nacelles en boîtes | jante en tubes, rayons en câbles, nacelles fermées modélisées, pieds en A en tubes |
| Tente `T_TENTE` | 112 | prisme | tente dôme (arceaux croisés, toile tendue, auvent, haubans) |
| Grillage `T_GRILLAGE` | 155 | boîtes fines | poteaux ronds à chapeau, fils de tension, maille losangée |
| Oiseaux (animés) | 3 vols de 7 | triangles plats | oiseau modélisé (corps fuselé, ailes en deux parties) |
| Projecteur de piste `T_LAMP` (commun) | 117 | cylindre + boîtes (`decor_carte.cpp`, commun) | mât et rampe de projecteurs modélisés dans `circuit.cpp` (type propre au Grand Prix ; lumière inchangée) |
| Portique de checkpoint (`portique.cpp`) | 0 | non posé au Grand Prix (Ville et Corniche seulement, RG-09 de WFS-71) | rien à faire ici (WFS-378, hors décor) |
| Grille de départ `T_GRILLE` | 10 | marquage peint au sol | inchangé (peinture au sol, pas un objet) |
| Arbres, touffes (communs, `vegetation.*`) | 568 | cartes de feuillage | hors ticket (végétation : WFS-388) ; touffes d'herbe en grille → semis au hasard et herbe tondue en bandes (ANO-220) dans `circuit.cpp` |

Réseau (`regles-reseau.md`) : recherche de code GitHub (« grandstand », « tire barrier glb », « race track props CC0 gltf »,
« helicopter gltf CC0 ») : seulement des modèles stylisés de jeux (exclus par RG-02) ; aucun téléchargement. RG-03 : tout est
modélisé par le code au niveau du vrai objet, construit une fois au chargement (un maillage par sorte d'objet, version proche et
version allégée), comme WFS-372 (Campagne) et WFS-376 (marina).
