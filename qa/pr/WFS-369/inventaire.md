# WFS-369 — inventaire des formes géométriques nues du mobilier de la Ville (RG-01)

Relevé dans le code de `main` (2026-11-09), avant tout changement. « Boîte » = `box` / `boite`, « cylindre » = `cyl` / `cylindre`,
« cône » = `cone`, « disque » = triangles en éventail.

| Objet | Fichier : ligne (main) | Forme actuelle | Où | Traité ici |
|---|---|---|---|---|
| Banc public | `src/world/ville.cpp` (`case ville::BANC`) | 2 boîtes brunes (assise + dossier) | parc, tous les 22 segments | ✅ banc à lattes, flancs en fonte |
| Abribus | `ville.cpp` (`case ville::ABRIBUS`) | 4 boîtes (paroi, toit, vitre, banc) | résidentiel et vieux quartier, tous les 160 segments | ✅ poteaux, vitrages encadrés, toit, panneau d'affichage, banc |
| Grille du parc | `ville.cpp` (`case ville::GRILLE`) | poteau boîte + lisse boîte | parc, tous les 4 segments | ✅ poteaux, lisses, barreaux à pointes |
| Fontaine du parc | `ville.cpp` (`case ville::FONTAINE`) | 3 cylindres | parc, tous les 200 segments | ✅ bassin mouluré, piédestal, vasque |
| Fontaine de l'îlot du rond-point | `ville.cpp` (`lieux`) | 3 cylindres + 2 disques | îlots | ✅ même modèle, plus petit |
| Lampadaire de rue | `ville.cpp` (`case ville::LAMPE`) | cylindre 6 côtés + 2 boîtes | toutes les rues | ✅ mât conique, crosse, luminaire |
| Panneau « sens giratoire » | `ville.cpp` (`lieux`) | cylindre + disque + triangles | avant chaque rond-point | ✅ panonceau à bord roulé, colliers |
| Feux tricolores des carrefours | `ville.cpp` (`lieux`, `mat`) | cylindre + boîte | carrefours | ✅ caisson à 3 feux et visières |
| Table de terrasse | `src/world/commerces.cpp` (`commerce`) | cylindre + disque | terrasses des cafés | ✅ table de bistrot |
| Chaises de terrasse | `commerces.cpp` | 2 boîtes par chaise | terrasses | ✅ chaise de bistrot |
| Parasol (ANO-216) | `commerces.cpp` | cylindre + cône | terrasses (jour) | ✅ parasol 8 baleines, pied lesté ; fermé la nuit |
| Bougie | `commerces.cpp` | boîte | terrasses (nuit) | ✅ photophore |
| Pots de fleurs | `commerces.cpp` | cylindre + boîte verte | bouts des terrasses | ✅ jardinière et buis taillé |

## Absents de la carte (rien à remplacer ici)
Poubelles, bornes et potelets, kiosques, barrières de trottoir : la carte Ville n'en pose aucun aujourd'hui (aucun type dans
`ville_quartiers.h`). Les ajouter est un changement du décor : US « décor complet » WFS-385.

## Hors de ce ticket (formes nues relevées, autre US)
- Immeubles, balcons, entrepôts, conteneurs, grues du port, stores et enseignes des commerces : WFS-370 (immeubles) / WFS-385.
- Autoroute, tunnel, pont (glissières, mâts sodium, portiques) : ouvrages de la route, WFS-385.
