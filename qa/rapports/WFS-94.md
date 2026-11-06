# QA WFS-94 : Corniche (Port de plaisance, Côte rocheuse, Village de pêcheurs) + ville de nuit + finitions de WFS-93

## QA après fusion, recette de nuit, lot 4 (2026-11-06 08:30:09 UTC, qa-lead)

**Verdict : PASSABLE AVEC ANOMALIES MINEURES**

- Sources : Release `apercu` de `main`, captures du banc de la construction 689 (commit `eb8152a`, qui contient la fusion a722e6c de WFS-94 ; 346/346 à jour) ; pour comparer « avant », Release `apercu-waforspeed-134` (construction 676, `d9d150f`, antérieure à WFS-94) ; journaux des jobs du run 689 (vert). La construction 693 (`0d923d5`, WFS-148) ne change pas l'image du jeu (WFS-148, RG-08).
- Fusion par confiance : recette faite après la fusion. Aucune anomalie bloquante ; les mineures vont au tableau `anno.md` (le PO les priorise).

### Points vérifiés

| Point (`qa/Check.md`) | Capture | Constat |
|---|---|---|
| (1) Corniche : villas côté terre, tours de sauveteur | `carte-plage-corniche`, `vie-plage` | OK : villas à étages côté terre, palmiers ; tour de sauveteur rouge sur pilotis visible sur la plage (`vie-plage`) |
| (1) Marina : bornes, bateaux à voile, villas colorées | `carte-plage-marina` | OK en partie : bornes noires le long du quai, voiliers, villas colorées côté terre ; mais les bateaux sont posés sur le quai gris, pas dans l'eau (ANO-? n° 2) |
| (1) Côte rocheuse : rochers, pins, glissière | `carte-plage-rochers` | OK : rochers côté mer, pins et arbres ronds côté terre, glissière continue |
| (1) Village : maisons et barques | `carte-plage-village` | OK en partie : maisons des deux côtés, parasols ; aucune barque visible, les maisons côté mer cachent la mer (ANO-? n° 3) |
| (2) Ville : trottoir gris continu jusqu'aux façades (ANO-49) | `carte-ville-port`, `vie-terrasse`, `carte-ville-parc`, `carte-ville-residentiel`, `course-cam0-1` | OK au port et sur `vie-terrasse` (plus d'herbe) ; parc : il reste un mince liseré vert au pied de l'immeuble ; **résidentiel (côté gauche) et centre (`course-cam0-1`, côté droit) : une bande d'herbe reste entre le trottoir et les façades, et une terrasse est posée sur l'herbe** (ANO-? n° 1) |
| (3) Ville de nuit : façades sombres, environ une fenêtre sur trois allumée (ANO-37) | `course-nuit-ville-0`, `-1`, `-haut`, `poursuite-police` | OK au fond de rue : façades sombres même dans l'axe des phares, fenêtres chaudes et quelques froides, néons qui ressortent ; de près, les façades sont noires et unies (fenêtres éteintes invisibles) et bien moins d'une fenêtre sur trois est allumée (ANO-? n° 4) |
| (4) Panneaux du circuit : mots entiers de loin (ANO-51) | `carte-circuit-jour`, `carte-circuit-nuit` | OK : les lettres des panneaux moyens et lointains sont pleines, plus en morceaux ; le texte très lointain reste petit (limite de la taille à l'écran, pas un défaut) ; enseignes de la ville inchangées de près (`vie-terrasse`, `carte-ville-residentiel`) |
| (5) `vie-portique` : « CHECKPOINT 1 » lisible ; `vie-plage` : serviettes et baigneurs (ANO-40) | `vie-portique`, `vie-plage` | OK : « CHECKPOINT 1 » se lit ; caméra tournée vers la mer, serviettes au sol, parasols et baigneurs visibles |
| Captures de jour de la ville inchangées hors trottoir | `carte-ville-*`, `course-cam0-*` contre `apercu-waforspeed-134` | OK : seul le sol entre le trottoir et les façades change |
| ≤ 300 appels de dessin | journal `texte` (689) : `Ville : 273 appels de dessin (25 morceaux)` la nuit ; Plage 133 | OK |
| Rien de cassé | toutes les captures ci-dessus | OK : voiture visible, pas d'écran noir, pas de déformation |

### Anomalies existantes vérifiées corrigées

- **ANO-37** (fin, façades et fenêtres de nuit) : `course-nuit-ville-0`, `course-nuit-ville-haut` (reste un détail de près : ANO-? n° 4).
- **ANO-40** : `vie-portique`, `vie-plage`.
- **ANO-51** : `carte-circuit-jour`, `carte-circuit-nuit`.
- **ANO-49** : corrigée pour la bande RW + 4 à RW + 6 (RG-06) : `carte-ville-port`, `vie-terrasse` ; mais il reste de l'herbe au-delà, devant les immeubles en retrait (ANO-? n° 1).

### Anomalies

| ANO | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-? (1) | Mineure | `carte-ville-residentiel`, `course-cam0-1`, `carte-ville-parc` | Il reste une bande d'herbe entre le trottoir et les façades quand l'immeuble est en retrait (résidentiel côté gauche, centre côté droit avec une terrasse posée sur l'herbe, liseré vert au parc) |
| ANO-? (2) | Mineure | `carte-plage-marina` | Les voiliers de la marina sont posés sur le quai gris, à sec, et non dans l'eau |
| ANO-? (3) | Mineure | `carte-plage-village` | Aucune barque visible au village ; les maisons côté mer cachent la mer et les barques |
| ANO-? (4) | Mineure | `course-nuit-ville-1`, `course-nuit-ville-haut`, `poursuite-police` | La nuit, les façades proches sont noires et unies (fenêtres éteintes invisibles) et peu de fenêtres sont allumées de près, bien moins d'une sur trois |

### À essayer sur le téléphone (APK de `main`)

- [ ] Corniche de jour : traverser les 4 quartiers (Corniche, Port de plaisance, Côte rocheuse, Village de pêcheurs).
- [ ] Ville de nuit : façades sombres, fenêtres allumées, images par seconde en Basse.
