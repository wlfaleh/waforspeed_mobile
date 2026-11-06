 instruction dans ce fichier je veux laisse mes remarques et mes ordes 

il faut me laisse un commentaire après chaque tiret entre parenthèses 

exemple :

- j'ai merger la pr 87 il faut mettre a jour le tache et le journal 
( agent 3 session xyxyxt : c'est fait )

demande utilisateur :

> **Règle de l'utilisateur (2026-11-06)** : ce fichier ne garde que les demandes **ouvertes** (sans réponse, ou en attente d'une réponse de l'utilisateur), en haut. Une demande répondue et close part dans `archive/tache-utilisateur-repondu.md` (le PO la déplace). Nouvelle demande : un tiret à la fin de la liste ci-dessous ; réponse entre parenthèses juste après.

## Demandes ouvertes

- WFS-158 : la carrosserie de la Model 3 reste en facettes (ANO-75) ; ordre de l'utilisateur dans la conversation avec le PO (2026-11-06 19:52:29 UTC) : « Ok remplace avec un autre modèle »
( PO : c'est noté — décision écrite dans `regles-de-gestion/WFS-158.md` (RG-D) : la Model 3, et seulement elle, est remplacée par un autre modèle libre de Tesla Model 3, vérifié aux dimensions réelles avant d'entrer dans le jeu, sans facettes, avec ses feux, ses peintures et le même poids pour ton téléphone. Claude (session 01Us6uCu) le fait dans WFS-158. Si le réseau bloque le téléchargement, il te demandera ici de déposer le fichier. )
( arwa-2 (ex-Claude, session 01Us6uCu) : j'ai besoin de toi pour un fichier. Le réseau des agents ne peut pas télécharger sur Sketchfab (connexion obligatoire, site bloqué). Modèle que je propose : « Tesla Model 3 (FREE TO USE) » de itsvipergfx, https://sketchfab.com/3d-models/tesla-model-3-free-to-use-91cc094d0d80402480445d70f3a53580 ; vérifie sur la page que la licence est bien CC Attribution (je n'ai pas pu l'ouvrir). Autre choix : « Tesla model 3 » de teslaracecar (https://sketchfab.com/teslaracecar). **Ce qu'il faut faire** : connecte-toi sur Sketchfab, bouton « Download 3D model », format **glTF** (ou .glb), puis dépose le fichier dans le dépôt, dossier `assets/sources/model3/` (ou envoie-le-moi ici). Je le mesure aux vraies dimensions avec `test_fidelite_voitures` avant de le mettre dans le jeu. En attendant, j'avance sur les dimensions des autres voitures. )
( PO, 2026-11-06 22:31 UTC : **plus besoin de toi pour ce fichier.** J'ai retrouvé dans le dépôt du jeu web (`wlfaleh/waaps`) comment les 20 voitures ont été récupérées : elles viennent de dépôts GitHub publics qui contiennent les modèles Sketchfab avec leur `license.txt`. Les agents peuvent chercher et lire ces dépôts sans ouvrir de site ni utiliser de compte. Pour la Model 3, 27 dépôts sont déjà trouvés. La méthode est écrite dans `.claude/skills/waforspeed-natif/references/modeles-voitures-github.md` et autorisée dans `regles-reseau.md` ; arwa-2 l'applique dans WFS-158 (licence CC0 ou CC BY seulement). )

- Changer les noms et les logos des voitures, en phases ; garder les modèles et les retoucher un peu (conversation avec le PO, 2026-11-06 20:02:57 UTC)
( PO : c'est créé — 4 tickets, dans l'ordre : **WFS-191** noms inventés, **WFS-192** logos effacés ou remplacés, **WFS-193** petites retouches de forme (dimensions inchangées), **WFS-194** remplacement des 5 modèles « non commerciaux » (T-Cross, X5, A 45, Elantra N, Mustang), seulement si tu décides un jour de vendre le jeu ou d'y mettre pubs ou achats. **Une question pour démarrer WFS-191** : valides-tu la liste des 20 noms inventés que je propose dans `regles-de-gestion/WFS-191.md` (ex. Tesla Model 3 → Volta E3, Porsche 911 → Pfeiler 9F S, Ferrari SF90 → Cavallo SF-X) ? Réponds « ok » ou écris les noms à changer. Limite : renommer et effacer les logos ne garantit rien juridiquement si la forme reste reconnaissable ; avant une publication payante, fais vérifier par un professionnel. )
( PO, 2026-11-06 22:05 UTC : c'est pris en compte — plus besoin de valider la liste : les noms des 20 voitures seront dans `configgame.json`, nouvelle section `voitures` (marque et modèle pour chaque voiture). Par défaut, ce sont les noms inventés que j'ai proposés (Volta E3, Pfeiler 9F S, Cavallo SF-X…) ; tu les changes quand tu veux dans le fichier, même sur le téléphone sans reconstruire l'APK. WFS-191 est prêt et sera donné au prochain développeur libre. )
