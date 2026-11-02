# Ajouter une voiture

1. Trouver l'entrée dans le jeu web : `waforspeed/js/cars/catalog.js` (`CARS` pour la fiche, `CAR_MODELS` pour le fichier, rotateY, regex de peinture/feux).
2. Convertir (Node ≥ 18, `cd tools && npm install` la première fois) :
   ```
   node convert_car.mjs <waaps>/waforspeed/models/<dossier>/<nom>.json ../assets/cars/<id>.wfm <longueur_m> <rotateY|0|auto> '<regex peinture>' ['<regex feux arrière>']
   ```
   La sortie affiche taille, triangles et le matériau de peinture trouvé : il ne doit pas être « AUCUNE ».
   Vérifier que la longueur et la largeur (rétroviseurs compris) sont réalistes.
3. Copier la licence : `assets/licences/<dossier>.txt`.
4. Ajouter la voiture dans `CARS[]` (`src/game/cars.h`) : fiche (`spec`, `grip`, dimensions `d`), fichier, couleur, crédit.
5. Pousser : le banc d'essai de la CI fait une capture « vitrine » de chaque voiture → vérifier l'orientation (avant vers -Z) et la peinture.
