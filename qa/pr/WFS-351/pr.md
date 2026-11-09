Agent : agent expert ci cd

**Demande de l'utilisateur** : « Go » (2026-11-09) sur le plan de l'expert ; ANO-199 (recette du QA du 2026-11-09) : depuis WFS-308, l'émulateur des bancs du matin reste bloqué sur l'écran des CGU ; il doit les accepter **comme un joueur, sans raccourci dans le jeu**.

## Ce qui change (`tools/qa/emulateur.sh` seulement)
- Avant l'écran de chargement : si le journal du jeu dit « CGU (WFS-308) : écran ouvert », le parcours **appuie sur la case** « J'ai lu et j'accepte », vérifie « case … cochée » au journal, puis **appuie sur « Accepter »** et vérifie « « Accepter » (version N) ». Captures `00-cgu`, `00-cgu-case-cochee`. CGU déjà acceptées (2e essai) : l'écran n'apparaît pas, le parcours continue.
- Étape « Mise à jour et installation des paquets » (WFS-306) : proposition → **« Installer »** (vrai téléchargement depuis la Release `paquets`, WFS-302) puis attente de « fin de l'étape » (300 s ; « échec » = étape rouge) ; captures `01b-paquets-proposition`, `01c-paquets-fin`. Paquet déjà là ou pas de liste : l'étape finit seule.
- Points d'appui calculés comme la mise en page du jeu (`cgu.cpp`, `paquets.cpp`, `petit_ecran.h` : densité lue par `adb shell wm density`, taille logique de la fenêtre annoncée par le jeu), comme le script le faisait déjà pour le menu.

## Comment c'est vérifié
- `bash -n`. Bancs `cgu` et `paquets` lancés en local en 2400×1080 (420 dpi, comme le Pixel 6 de l'émulateur), points calculés par le même `awk` : case (141, 984), « Accepter » (2250, 984), « Installer » (2079, 838).
- Couleur des pixels aux points calculés : case grise avant / **cyan cochée** après (`cgu-coche`), « Accepter » **orange** une fois actif, « Installer » **orange** ; images : `qa/pr/WFS-351/cgu-points-case-accepter.png`, `paquets-point-installer.png`, `cgu-premier-2400.png`.
- Essai réel : aux prochains bancs du matin (émulateur, Release `simulateur-matin`) ; l'émulateur ne tourne pas dans cette session.

## Limites et risques
- Marges de l'écran supposées nulles en paysage (le point visé est loin des bords de chaque zone) ; si un appui est perdu, l'étape échoue franchement avec les coordonnées au journal.
- Le téléchargement réel (≈ 22 Mo) ajoute environ une minute au parcours.
