# Prototype Godot (WFS-610, épic E25) — comparaison de moteur

Branche `godot` uniquement : **aucun fichier du jeu actuel n'est modifié**, rien n'est fusionné dans `main`.
But : une voiture, la route de la Ville, du trafic et la caméra de course sous **Godot 4.3** (licence MIT), en APK Android
installable à côté du jeu actuel, pour que l'utilisateur compare sur son téléphone : fluidité, à-coups, voitures qui
apparaissent progressivement, lisibilité de la voiture, qualité d'image.

- `outils/extraire_trace.cpp` : tracé de la Ville tiré du code actuel (même calcul), en JSON (`projet/donnees/ville.json`).
- `outils/wfm_vers_glb.mjs` : voitures du jeu (`assets/cars/*.wfm`) converties en glTF binaire (`projet/voitures/*.glb`).
- `projet/` : projet Godot.
