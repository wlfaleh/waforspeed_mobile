 instruction dans ce fichier je veux laisse mes remarques et mes ordes 

il faut me laisse un commentaire après chaque tiret entre parenthèses 

exemple :

- j'ai merger la pr 87 il faut mettre a jour le tache et le journal 
( agent 3 session xyxyxt : c'est fait )

demande utilisateur :

> **Règle de l'utilisateur (2026-11-06)** : ce fichier ne garde que les demandes **ouvertes** (sans réponse, ou en attente d'une réponse de l'utilisateur), en haut. Une demande répondue et close part dans `archive/tache-utilisateur-repondu.md` (le PO la déplace). Nouvelle demande : un tiret à la fin de la liste ci-dessous ; réponse entre parenthèses juste après.

## Demandes ouvertes

- Changer les noms et les logos des voitures, en phases ; garder les modèles et les retoucher un peu (conversation avec le PO, 2026-11-06 20:02:57 UTC)
( PO : c'est créé — 4 tickets, dans l'ordre : **WFS-191** noms inventés, **WFS-192** logos effacés ou remplacés, **WFS-193** petites retouches de forme (dimensions inchangées), **WFS-194** remplacement des 5 modèles « non commerciaux » (T-Cross, X5, A 45, Elantra N, Mustang), seulement si tu décides un jour de vendre le jeu ou d'y mettre pubs ou achats. **Une question pour démarrer WFS-191** : valides-tu la liste des 20 noms inventés que je propose dans `regles-de-gestion/WFS-191.md` (ex. Tesla Model 3 → Volta E3, Porsche 911 → Pfeiler 9F S, Ferrari SF90 → Cavallo SF-X) ? Réponds « ok » ou écris les noms à changer. Limite : renommer et effacer les logos ne garantit rien juridiquement si la forme reste reconnaissable ; avant une publication payante, fais vérifier par un professionnel. )
( PO, 2026-11-06 22:05 UTC : c'est pris en compte — plus besoin de valider la liste : les noms des 20 voitures seront dans `configgame.json`, nouvelle section `voitures` (marque et modèle pour chaque voiture). Par défaut, ce sont les noms inventés que j'ai proposés (Volta E3, Pfeiler 9F S, Cavallo SF-X…) ; tu les changes quand tu veux dans le fichier, même sur le téléphone sans reconstruire l'APK. WFS-191 est prêt et sera donné au prochain développeur libre. )

- Demande de deux nouveaux sites pour le réseau des agents (présentée par le PO, 2026-11-06 23:47 UTC, `regles-reseau.md` § 8)
( PO : **une question pour toi, rien ne presse.** Claude, agent 2 voudrait que tu autorises deux sites, pour la vue cockpit réaliste (WFS-201 et WFS-202) :
  1. `eurospares.co.uk` : site de pièces détachées ; on y lirait la **longueur des balais d'essuie-glace** et des cotes de pièces d'habitacle des voitures du jeu ;
  2. `wipersizechart.com` : tableau des **tailles d'essuie-glaces** par modèle de voiture.
  On n'y télécharge **aucun fichier** : on y lit seulement des chiffres (des dimensions), notés au journal avec l'adresse de la page. Licence : ce sont des données de fait, pas un contenu qu'on copie ; aucun modèle, image ou texte n'entre dans le dépôt.
  Mon avis : utile mais pas indispensable. Sans ces sites, la longueur des balais reste notée « non mesurable » et le ticket avance quand même.
  Si tu es d'accord, ajoute ces deux sites (sans étoile) dans les réglages de l'environnement cloud ; je mettrai ensuite à jour la liste de `regles-reseau.md`. Si tu refuses, rien ne change. )
