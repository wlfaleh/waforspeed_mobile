 instruction dans ce fichier je veux laisse mes remarques et mes ordes 

il faut me laisse un commentaire après chaque tiret entre parenthèses 

exemple :

- j'ai merger la pr 87 il faut mettre a jour le tache et le journal 
( agent 3 session xyxyxt : c'est fait )

demande utilisateur :

> **Règle de l'utilisateur (2026-11-06)** : ce fichier ne garde que les demandes **ouvertes** (sans réponse, ou en attente d'une réponse de l'utilisateur), en haut. Une demande répondue et close part dans `archive/tache-utilisateur-repondu.md` (le PO la déplace). Nouvelle demande : un tiret à la fin de la liste ci-dessous ; réponse entre parenthèses juste après.

## Demandes ouvertes

- Le guide auto est une catastrophe dans les virages et les ronds-points : 1) le corriger ; 2) pouvoir l'activer ou le désactiver dans les réglages (conversation avec le PO, 2026-11-07 09:05 UTC)
( PO : c'est créé — **WFS-217** (P1 Urgent, prêt). Cause probable trouvée dans le code : le « guide » est un ensemble d'aides cachées qui ramènent la voiture vers l'axe de la route et limitent son angle ; sur les épingles et les ronds-points, l'axe tourne vite et ces aides luttent contre toi. Correction : l'aide suit la vraie direction de la route là où est la voiture, s'élargit dans les virages serrés, sans à-coups. Et une ligne **« Aide à la direction : Activée / Désactivée »** dans les Réglages (Activée par défaut, gardée d'une partie à l'autre) ; désactivée, la voiture va exactement où tu la diriges. Aucun bouton ne change de taille. )
( PO : précision prise en compte — le réglage est seulement dans le menu Réglages du jeu, rien dans `configgame.json`. )
( Claude, agent arwa (session trafic) : c'est fait et fusionné (WFS-217, PR #189). Avant, la route faisait tourner la voiture toute seule : quand tu braquais pour prendre l'épingle ou le rond-point, la voiture tournait deux fois, puis des aides cachées la ramenaient de force. Maintenant, c'est ton volant qui fait tourner la voiture. Quand tu lâches le volant dans un virage, l'aide suit la vraie direction de la route, sans à-coups, et la borne s'élargit dans les virages serrés. Dans Réglages, une nouvelle ligne « Aide à la direction : Activée / Désactivée » (Activée par défaut, gardée d'une partie à l'autre). Désactivée, la voiture va exactement où tu la diriges, et elle sort du virage si tu ne braques pas. Aucun autre bouton n'a bougé. À essayer avec l'APK de `main` : l'épingle du Grand Prix et un rond-point, avec l'aide activée puis désactivée. )

- Grand problème graphique : en caméra proche et arrière, la voiture a un comportement bizarre et ce n'est pas fluide ; trouver comment le tester et me proposer en urgence quoi faire (conversation avec le PO, 2026-11-07 10:12 UTC)
( PO : c'est créé — **WFS-221** (P1 Urgent, US technique, prêt). Deux causes probables trouvées dans le code : 1) la conduite avance par petits pas fixes, mais l'image montre le dernier pas sans lissage : sur ton téléphone, la voiture avance par à-coups devant la caméra ; 2) la pente de la route change d'un coup à chaque morceau de route : la voiture « bascule » dans les montées et descentes. Le test : un nouveau banc « fluidité » qui imite les images irrégulières du téléphone et mesure les à-coups. La correction : image lissée entre deux pas, pente et virages lissés. Preuve : vidéo avant / après. Ce qui m'aiderait : une vidéo de 10 s de ton écran (enregistreur d'écran d'Android), si tu peux. )
( Claude, agent 2 (01UY1Vv5), 2026-11-07 11:30 UTC : **corrigé et fusionné (WFS-221, PR #193)**. Il y avait trois causes, toutes confirmées au nouveau banc « fluidité ». 1) L'image montrait la voiture du dernier « pas » de calcul : sur le téléphone, certaines images avancent de 2 pas, d'autres de 3. 2) La caméra, qui suit la voiture avec un peu de retard, avait un retard qui changeait à chaque image : c'est le « avance, recule, avance ». 3) La pente et les virages de la route cassaient tous les 1,2 m. Maintenant, l'image est calculée entre deux pas, la caméra garde exactement le même retard (même distance et même hauteur qu'avant) et la route est lissée pour la voiture. Mesure en ligne droite : à-coup de 30 cm avant, 3 mm après ; aller-retour voiture ↔ caméra de 30 cm avant, 3 mm après. Le trafic, les adversaires et la police sont lissés aussi. Reste à part : les images par seconde basses (10 à 20 sur ton téléphone), traitées dans WFS-222. À tester sur ton téléphone avec l'APK de main. )
