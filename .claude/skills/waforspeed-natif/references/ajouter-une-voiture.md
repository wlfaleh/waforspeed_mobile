# Ajouter une voiture

1. Trouver un modèle glTF sous licence libre (licence notée) et la fiche constructeur de la voiture (dimensions : `references/reel/voitures/`).
2. Convertir (Node ≥ 18, `cd tools && npm install` la première fois) :
   ```
   node convert_car.mjs <modèle>.json ../assets/cars/<id>.wfm <longueur_m> <rotateY|0|auto> '<regex peinture>' ['<regex feux arrière>']
   ```
   La sortie affiche taille, triangles et le matériau de peinture trouvé : il ne doit pas être « AUCUNE ».
   Vérifier que la longueur et la largeur (rétroviseurs compris) sont réalistes.
3. Copier la licence : `assets/licences/<dossier>.txt`.
4. Ajouter la voiture dans `CARS[]` (`src/game/cars.h`) : fiche (`spec`, `grip`, dimensions `d`), fichier, couleur, crédit.
5. Pousser : le banc d'essai de la CI fait une capture « vitrine » de chaque voiture → vérifier l'orientation (avant vers -Z) et la peinture.
