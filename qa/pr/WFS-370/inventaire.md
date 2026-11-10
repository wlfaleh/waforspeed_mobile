# WFS-370 — inventaire des immeubles et volumes « boîtes » de la Ville (RG-01)

Relevé dans le code de `main` (2026-11-10, commit 04508b2b) et au banc d'essai (captures « avant » de `qa/pr/WFS-370/`), avant tout
changement. Suite de l'inventaire de WFS-369 (`qa/pr/WFS-369/inventaire.md`, ligne « Hors de ce ticket »). « Boîte » = `box` / `pave`
sans autre détail, « plaque » = pavé mince plein.

## Immeubles (592 bâtiments du kit d'architecture WFS-161 posés sur la Ville)

Les immeubles ne sont plus des boîtes depuis WFS-161 (ouvertures en retrait, appuis, encadrements), mais il restait des volumes nus :

| Élément | Fichier : fonction (main) | Forme actuelle | Où | Traité ici |
|---|---|---|---|---|
| Toit de zinc du classique | `src/world/archi_geo.cpp` (`toiture`) | deux pans plats à 35° et deux pignons triangulaires (« boîte à toit ») | tous les immeubles classiques (résidentiel, parc, centre < 20 m) | ✅ comble à la Mansart : brisis à 70°, terrasson à 20°, croupes, une lucarne à fronton par travée (fenêtre allumée la nuit), souches de cheminée avec mitrons |
| Corniche | `archi_geo.cpp` (`construire`) | un pavé de 0,6 m (classique) ou 0,3 m (moderne) | tous | ✅ corniche moulurée en 6 ressauts (cordon, doucine, larmier, couronnement) |
| Garde-corps des balcons | `archi_geo.cpp` (`ouverture`, `facade`) | plaque pleine de 1,00 × 0,04 m | balcons, balcons filants (2e et dernier étage du classique), portes-fenêtres | ✅ main courante 50 × 40 mm, lisse basse, barreaux de 16 mm tous les 12 cm (vide 10,4 cm ≤ 11 cm, NF P01-012), retours de côté |
| Toiture-terrasse et édicule | `archi_geo.cpp` (`toiture`) | dalle plate, cube de 3,2 × 2,8 × 2,6 m | immeubles modernes de 3 étages et plus | ✅ couvertines sur l'acrotère, édicule avec dalle en débord, porte, linteau, grille de ventilation |
| Pied de façade | `archi_geo.cpp` (`facade`) | mur lisse jusqu'au sol | classique, moderne | ✅ soubassement de 0,6 m en saillie de 3 cm ; cordon de pierre sous le 1er étage (classique) |
| Vitrines du rez-de-chaussée | `archi_geo.cpp` (`ouverture`) | vitre nue au fond du tableau | commerces du kit | ✅ dormant métallique, meneaux tous les 2 m au plus, imposte |
| Fenêtres de nuit | `shaders/decor.glsl` (`interieurFenetre`) | une seule teinte (2 850 K) ; tirage « allumée » fait **par pixel** (position du monde imprécise à quelques km de l'origine : fenêtres mouchetées) | toutes | ✅ tirage par fenêtre (graine de la baie dans ses uv), 35 % allumées, teinte de 2 700 à 4 000 K, intensité de 0,55 à 1,3 |
| Repli `WFS_ARCHI=0` (anciennes boîtes et dalles de balcon) | `src/world/ville.cpp` (`case ville::IMMEUBLE`) | boîtes, dalles et garde-corps en pavés | **jamais affiché** (repli de secours du kit) | — laissé tel quel (repli) |
| Immeubles autour des ronds-points | `ville.cpp` (`abordsRondPoint`) | boîtes | **0 rond-point** sur la Ville (WFS-283) : rien d'affiché | — |

## Port (zone portuaire de la Ville)

| Objet | Fichier : ligne (main) | Forme actuelle | Traité ici |
|---|---|---|---|
| Entrepôts | `ville.cpp` (`case ville::ENTREPOT`) | une boîte, un toit en losange, deux plaques de portes | ✅ bâtiments du kit, style industriel (brique, baies d'atelier à petits bois, portes de garage, toit de zinc ou terrasse), mêmes place et taille |
| Conteneurs | `ville.cpp` (`case ville::CONTENEURS`) | boîtes de 2,45 × 2,55 × 12 m | ✅ conteneurs ISO de 40 pieds (12,192 × 2,438 × 2,591 m) : poteaux d'angle, 8 pièces de coin, longerons, parois en tôle ondulée trapézoïdale, portes à 4 barres de verrouillage, gâches, charnières ; loin : parois lisses |
| Grues | `ville.cpp` (`case ville::GRUE`) | 4 poteaux, une tête et une flèche en boîtes, une cabine cube | ✅ portique de quai : bogies et galets, pieds en caissons, portique, poutres de 75 m (avant-bec 45 m, arrière-bec 15 m), chevalet et haubans, salle des machines, chariot, cabine vitrée, câbles, palonnier ; rail côté terre 6 m plus loin de la route (hors des piles de conteneurs) |
| Grillage | `ville.cpp` (`case ville::GRILLAGE`) | poteau et lisse en boîtes | ✅ clôture en panneaux rigides à double fil (h 2,03 m, maille 200 × 50 mm, deux plis en V), poteaux tube 60 × 40 avec chapeau |

## Hors de ce ticket (relevé, autre US)
- Casino, centre commercial, garages (`ville.cpp`, `lieuVille`) : façades montées en pavés (pilastres, bandeaux, marquise) : **WFS-385** (décor complet de la Ville).
- Ouvrages de la route (autoroute, tunnel, pont : glissières, mâts sodium, portiques) : **WFS-385**.
- Teintes des façades : un seul albédo (béton 0,30, brique pour l'industriel) faute de valeur de la pierre et du crépi au référentiel (`materiaux.json`) : à sourcer dans WFS-385.
- Sol du port (terre-plein en herbe sous les conteneurs) : abords, **WFS-385**.
