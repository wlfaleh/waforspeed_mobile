# WFS-371 — inventaire des formes nues de la plage de la Corniche (RG-01)

Relevé sur `main` (00a5173), carte `plage` (Corniche), tracé 2, fichier `src/world/plage.cpp`. Nombres : journal du banc
(`Plage (WFS-371) : …`, ligne ajoutée par ce ticket, `WFS_BANC=corniche`). Captures « avant » : `qa/pr/WFS-371/avant-*.jpg` (à la PR).

## Objets du ticket (remplacés)

| Objet (code dans `plage.cpp`) | Nombre sur le tour | Dessin sur `main` (forme nue) | Remplacement (RG-02 / RG-03), `decor_corniche_modeles.*` |
|---|---|---|---|
| Parasol de plage (`umbrellas_`) | 57 | cylindre (mât) + cône Ø 3 m | parasol Ø 1,80 m, 8 baleines et tiges de coulisse, toile tendue en 8 pans creusés, lambrequin festonné, mât en 2 parties, noix, pointe, vrille d'ancrage |
| Transat (`umbrellas_`) | 57 | boîte | bain de soleil 1,90 × 0,65 m : longerons cintrés en tube Ø 30 mm, toile textilène tendue, dossier relevé à 40°, pieds, 2 roues |
| Bar de plage : pieux et poteaux (`paillote`) | 3 bars, 9 pieux + 8 poteaux chacun | cylindres à 6 faces | rondins de pin écorcé bosselés |
| Bar de plage : comptoir, étagère | 3 + 3 | boîte texturée + boîtes (plateau, étagère pleine) | comptoir à lames verticales, plateau de teck chanfreiné débordant, socle, tablette ; étagère à montants et 5 tablettes |
| Bar de plage : tabourets | 7 par bar | 2 cylindres | tabouret à embase bombée, fût Ø 50 mm, repose-pied (anneau et rayons), platine, assise de bois arrondie |
| Bar de plage : tables basses et poufs | 3 + 6 par bar | cylindre + boules | table en teck à lattes, ceinture et pieds chanfreinés ; pouf rond rembourré à plis et passepoils |
| Bar de plage : bouteilles | 12 par bar | cylindres | bouteille de 75 cl (fût, épaule, col, capsule), sur une tablette |
| Bar de plage : transats et parasols de paille | 3 à 4 par bar | boîte + cylindre + cône | bain de soleil (ci-dessus) ; parasol de paille Ø 2,40 m : mât de bois, 3 rangs de chaume à franges, ligature, tiges de bambou |
| Bar de plage : guirlande | ≈ 20 ampoules par bar | boules | ampoule G60 et douille |
| Poste de secours (`O_TOUR`, « cabines ») | 2 | 4 boîtes (pieds) + boîte (cabine) + plaque | poste surélevé : 4 poteaux et croix de Saint-André, plancher à lames sur solives, cabine à clin, baies vitrées et porte, cornières, toit à une pente et chevrons, garde-corps, échelle |
| Glissière du bord de mer (`O_GLISSIERE`, zone « rochers ») | 134 tronçons de 3,6 m | 2 boîtes (poteau, lisse) | **R5** : lisse à double onde de 310 × 83 mm en volume (3 mm, bords roulés), poteaux C 100 × 50 et écarteurs tous les 4 m, boulons |
| Glissière de la falaise (`decorFalaise`) | toute la falaise côté mer | profil en W d'une seule face + boîtes | même glissière (R5) |
| Rochers (`O_ROCHER`) | 136 | boule aplatie | bloc de calcaire taillé (modèle de roche du rivage de WFS-380, texture de roche) |
| Arbres ronds (`O_ARBRE`, zone « rochers ») | 28 | cylindre + 2 boules | pin d'Alep de WFS-383 (réutilisé, rien de recréé) |
| Scène de concert (`decorLieux`, fête) | 2 | boîtes (plancher, fond, table, poteaux et poutres) | **D4** : praticables de 2 × 1 m sur pieds, jupe plissée, escalier, portique en poutres treillis de 29 cm, fond de scène tendu, table du DJ (platines, table de mixage), 4 lyres pendues |
| Enceintes de la scène | 4 | 2 boîtes chacune | 2 caissons de basses (grille, poignées) + enceinte à 2 haut-parleurs et pavillon |
| Lyres de la scène (lentilles colorées) | 8 | boules colorées | têtes de lyre modélisées (embase, bras, tête) et lentille bombée éclairée |
| Mâts et ampoules des guirlandes (fête) | 8 mâts, ≈ 80 ampoules | cylindres, boules | mât de pin écorcé et piton ; ampoule G60 et douille |
| Feu de camp (fête) | 2 | boules (pierres), boîtes (bûches) | galets roulés (modèle du rivage), bûches de 50 cm écorcées en faisceau |
| Lampadaires de la promenade (`lamps_`) | 74 | boîte (poteau) + boule | **R4** : candélabre de style en fonte : socle mouluré, fût cannelé, bague, chapiteau, crosse à volute, lanterne vitrée à 4 faces sous chapeau (verres éclairés à 5,6 m, lumière inchangée) |
| Couronne des palmiers (`palms_`) | 183 | boule (gaines) + 5 boules (noix) | manchon de gaines fibreuses, 10 pétioles, grappe de noix de coco Ø 0,20 × 0,26 m (calice, pédoncule) |

Sable de plage (D4, 0xD8C8A0, traces et laisse de mer) : déjà traité par le sol de WFS-383 (sols des cartes), hors de ce ticket.

## Formes nues encore présentes sur la Corniche, hors de ce ticket (remises au PO)

| Objet | Où | Pourquoi pas ici |
|---|---|---|
| Bateaux au large : voilier, yacht, jet-ski, paquebot (boîtes) ; barques du village (`O_BATEAU`, boîtes) | `voilier()`, `yacht()`, `jetski()`, `paquebot()`, `O_BATEAU` | bateaux : pas dans le titre ; la marina a été faite par WFS-376 ; à reprendre par une tâche « bateaux au large et barques » |
| Jeux de plage : poteaux de volley, ballon, château de sable, pédalos, toboggan gonflable | `decorLieux` (jeux) | pas dans le titre ni dans D4 / R4 / R5 ; tâche séparée proposée |
| Scooters de la location (boîtes, ellipsoïdes) | `scooters()` (WFS-181) | véhicules (E10) |
| Casino, café, galerie (façades en boîtes) | `casino()`, `cafe()`, `galerie()` | bâtiments : WFS-410 (fenêtres et portes hors kit) et WFS-386 (décor complet) |
| Villas en boîtes | `O_VILLA` | seulement si le kit d'architecture est coupé (`WFS_ARCHI=0`) : sinon villas du kit (WFS-161) |
| Feu tricolore (cylindre + boîte), ancre du carrefour (boîtes, boules), luminaires du tunnel (boîtes) | `decorFalaise` | signalisation, tunnel (E9 / E13) |
| Faisceaux des lyres (cônes), flamme du feu de camp (cônes) | `dessineMer` | effets de lumière (E13, hors périmètre de ce ticket) |

Réseau (`regles-reseau.md`) : aucun téléchargement. Même méthode que WFS-372 (Campagne), WFS-373 (Grand Prix) et WFS-376 (marina),
acceptée par le lead dev et l'agent 3D : RG-03, chaque objet est modélisé par le code au niveau du vrai objet, construit une fois au
chargement (version proche et version allégée), recopié dans les blocs du décor préparés au chargement (WFS-268). Le paquet `.wfcarte`
de la Corniche (WFS-426) ne contient que les villas du kit : ces objets sont construits par le code au chargement, avec ou sans paquet.
