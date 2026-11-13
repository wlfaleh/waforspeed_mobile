# Modèles remplaçants (WFS-219, lot 1 de WFS-194)

Licences des modèles 3D trouvés pour remplacer les 5 voitures dont le modèle actuel est sous licence « non commerciale »
(T-Cross, X5, A 45, Elantra N, Mustang Shelby GT500). Un fichier `<id>.txt` par voiture retenue : texte de licence d'origine
(`license.txt` de Sketchfab), dépôt GitHub où il a été trouvé, SHA-256 du fichier source, modifications pour WaForSpeed.
Les modèles convertis sont dans `assets/cars/remplacants/` et **ne sont pas encore utilisés par le jeu** (branchement : WFS-194).

**WFS-359 (2026-11-09)** : la Mustang de Jiaxing (`remplacants/mustang.wfm`) est retirée sans avoir servi : hauteur −2,5 % et voie
avant +1,8 %, hors de la tolérance de ± 1 % (aucune dérogation, décision du PO). La voiture « mustang » du jeu est maintenant une
Chevrolet Corvette C7 (Martin Trafas, CC BY 4.0) et « tcross » une Suzuki Vitara Brezza (BHP3D, CC BY 4.0) : licences dans
`assets/licences/mustang.txt` et `assets/licences/tcross.txt`. Ce dossier ne contient plus de modèle.

**WFS-397 (2026-11-13)** : deux remplaçants convertis dans `assets/cars/remplacants/`, **pas encore branchés** (WFS-360) :
- `w212.wfm` / `.lod` : « Mercedes E Class W212 » de Peter_D, CC BY 4.0 (`w212.txt`), place de l'A 45 ; caisse relevée de 40 mm, sans marque ;
- `virtus.wfm` / `.lod` : « 2022 Volkswagen Virtus GT » de BHP3D, CC BY 4.0 (`virtus.txt`), place de l'Elantra N ; caisse relevée de 39 mm, sans marque.
Chaîne rejouable : `tools/remplacants/convertir_remplacants.sh`.
