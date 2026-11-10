# WFS-372 — Inventaire des formes géométriques nues de la Campagne (RG-01)

Relevé sur `main` (d75a3d7b), `src/world/campagne.cpp`, fonction `dessine()` ; nombre d'objets placés sur le tour (compté au
chargement de la carte, graine de la carte) ; captures du banc `campagne` (1280x720) dans `inventaire/`.

## Objets du ticket (remplacés par WFS-372)
| Objet | Code actuel (`dessine`) | Nombre | Capture |
|---|---|---|---|
| Rangs de vigne (ANO-65) | `T_VINES` : bande pleine verte de 0,5 × 1,2 m (3 faces), en blocs de 14,4 m | 1 755 blocs (16 rangs × 2 côtés) | `inventaire/vignes.jpg` |
| Clôtures des champs | `T_FENCE` : poteau = `box` 0,12 × 1,2 m, 2 lisses = `box` | 485 travées | `inventaire/ferme-clotures-bottes.jpg` |
| Enclos des chevaux | `T_ENCLOS` : poteaux et lisses en `box` | 2 enclos | banc `campagne` (chevaux) |
| Bottes de foin | `T_BALES` : `cylAxe` nu (Ø 1,5 × 1,4 m) | 41 groupes de 2 à 5 bottes | `inventaire/ferme-clotures-bottes.jpg` |
| Fermes (« grandes boîtes grises », ANO-219) | `T_FARM` : `box` 8 × 4,5 × 11 m gris uni + `toit` en losange | 20 fermes | `inventaire/ferme-clotures-bottes.jpg` |
| Granges (abris) | `T_FARM` (`k = 1`) : `box` 10 × 6 × 14 m + `toit` | ≈ la moitié des fermes des champs | idem (au fond) |
| Poteaux électriques (« portique sur poteaux », ANO-219) | `T_POLE` : `cyl` 8 côtés + traverse `box` | 27 | idem (à droite) |

## Objets vus, hors de ce ticket (pour WFS-387 « décor complet », au choix du PO)
| Objet | Code | Nombre | Remarque |
|---|---|---|---|
| Maisons du village | `T_VILLA` → kit d'architecture (`archi_carte.cpp`, WFS-161) | 100 | déjà modélisées par le kit (pierre, toits, ouvertures) : pas des boîtes ; le kit est commun aux cartes (hors zone) |
| Église | `T_CHURCH` : `box` + clocher `box` + flèche `cone` | 1 | forme nue |
| Commerces du village | `T_COMMERCE` : `box`, store, tables `cyl` | 5 | formes nues |
| Fontaine du rond-point | `T_FONTAINE` : 4 `cyl` | 1 | formes nues |
| Éoliennes | `T_TURBINE` : mât `cyl` 8 côtés, nacelle `box` | 9 | formes nues |
| Panneaux (forêt, A1, autoroute), feux, balises J6 | `T_SIGN`, `T_DANGER`, `T_PANNEAU_A`, `T_FEU`, `T_J6` | 16 + 4 + 4 + 2 + 75 | `box` ; signalisation |
| Haies | `T_HEDGE` : masse `box` + cartes de feuilles | 36 | masse cachée par les feuilles |
| Péage, séparateur, glissières, murets | `T_PEAGE`, `T_AUTO`, `T_GLISS`, `T_MURET` | 1 + 1 715 + 2 507 + 369 | ouvrages de la route |
