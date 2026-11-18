# Réponses préparées pour la Google Play Console — WaForSpeed (WFS-408, RG-E4)

Version « achats + pub » (décisions de l'utilisateur du 2026-11-18 : 1re version publique avec achats intégrés et vidéos
publicitaires). Préparé par le juriste ; **saisie dans la Console : WFS-409** ; relecture d'un avocat : WFS-516.
Chaque réponse cite sa source. Une réponse qui dépend d'une décision encore ouverte est marquée **[DÉCISION OUVERTE]**.
Les règles de Google changent : **relire chaque question dans la Console** le jour de la saisie.

## 1. Contenu de l'application
| Question de la Console | Réponse | Source |
|---|---|---|
| Contient des annonces | **Oui** | décision de l'utilisateur 2026-11-18 ; WFS-512, 514 (vidéo récompensée AdMob) |
| Achats intégrés | **Oui** (packs d'étoiles consommables, « sans pub » non consommable) | `android/app/build.gradle` (Billing 8.0.0), `configgame.json` section `market`, WFS-510, 511, 513 |
| Accès à l'application | Tout est accessible sans compte ni identifiant | aucun compte dans le jeu |
| Politique de confidentialité (URL) | [À REMPLIR PAR L'UTILISATEUR : adresse publique de la politique] ; même adresse dans `configgame.json` (`confidentialite_url`) | WFS-408 RG-03 |
| Coordonnées du développeur | [À REMPLIR PAR L'UTILISATEUR : nom de l'éditeur], [À REMPLIR PAR L'UTILISATEUR : e-mail de contact] ; statut **commerçant** (UE) obligatoire avec des ventes : adresse et téléphone publiés | règlement (UE) 2022/2065 (DSA) art. 30 ; aide Play Console « Statut de commerçant » |

## 2. Sécurité des données
Principe de Google : une donnée qui **quitte le téléphone** vers le développeur ou une bibliothèque intégrée (SDK) est
« collectée », même si elle n'est pas gardée ; elle peut alors être déclarée « traitée de façon éphémère ».
Source : aide Play Console « Remplir le formulaire Sécurité des données » ; pour la pub : page Google AdMob
« Informations pour la section Sécurité des données de Google Play » (**à relire le jour de la saisie**, la liste des données
de la bibliothèque AdMob y est donnée par Google).

| Donnée (catégorie Google) | Collectée | Partagée | Éphémère | Obligatoire | Usage | Source dans le dépôt |
|---|---|---|---|---|---|---|
| Identifiants de l'appareil ou autres (identifiant publicitaire) | Oui (AdMob) | selon la page AdMob | Non | Non (pub facultative) | Publicité ou marketing | WFS-512 |
| Position approximative (déduite de l'adresse IP par AdMob) | selon la page AdMob | selon la page AdMob | — | Non | Publicité | WFS-512 |
| Activité dans l'application (interactions avec les annonces) | selon la page AdMob | selon la page AdMob | — | Non | Publicité, analyse | WFS-512 |
| Informations et performances de l'application (diagnostics AdMob) | selon la page AdMob | selon la page AdMob | — | Non | Fonctionnement de l'app | WFS-512 |
| Informations personnelles : pseudo (duel en ligne) | Oui (relais) | Non (renvoyé aux autres joueurs à ta demande : fonction de l'app) | **Oui** | Non (duel facultatif) | Fonctionnement de l'app | `serveur/src/salon.js` (aucune adresse IP gardée, pas de chat) |
| Activité dans l'application : présence en course du duel (position, vitesse, score) | Oui (relais) | Non | **Oui** | Non | Fonctionnement de l'app | `src/game/duel*.cpp`, `serveur/src/salon.js` |
| Informations financières : historique d'achats | Non pour le jeu (Google Play traite le paiement ; le jeu garde seulement la preuve d'achat **sur le téléphone**) | Non | — | — | — | `src/game/sauvegarde.cpp` (jetons d'achat locaux), WFS-511 |
| Plantages | Non (rapport gardé sur le téléphone, partagé seulement par le joueur, avec l'appli de son choix) | Non | — | — | — | `store/confidentialite.md` |
| Adresse IP vue par GitHub au téléchargement des cartes | Non déclarée comme collecte du développeur (simple téléchargement d'un fichier public) ; mentionnée dans la politique | — | — | — | — | `configgame.json` `adresse_liste` |

Autres réponses :
- **Données chiffrées en transit** : Oui (HTTPS pour les cartes, connexion sécurisée au relais du duel, Google Play, AdMob) —
  **à vérifier** : le relais en ligne doit être joint en `wss://` (vérification à faire dans `src/game/duel*.cpp` avant la saisie).
- **Possibilité de demander la suppression des données** : aucune donnée gardée par le développeur (relais sans stockage) ;
  la demande se fait au contact de la politique ; pour Google : outils du compte Google.
- **Programme Familles / engagement** : voir § 4.

## 3. Classification du contenu (questionnaire IARC)
| Point du questionnaire | Réponse | Source |
|---|---|---|
| Catégorie | Jeu (course) | `store/fiche-fr.md` |
| Violence | Seulement des chocs entre voitures, sans blessure ni sang ; mode **Poursuite** (police qui poursuit le joueur, barrages) | modes du jeu (`configgame.json`) |
| Sexe, nudité, langage grossier, drogue, alcool, tabac | Non | — |
| Jeux d'argent simulés | Non (pas de caisse au hasard payante : RC-E21 de `regles-de-gestion/E21-market.md`) | E21 |
| Interactions entre utilisateurs | **Oui** : duel en ligne, pseudo visible par les joueurs du salon, sans chat | `serveur/src/salon.js` |
| Partage de la position | Non | `AndroidManifest.xml` (aucune permission de position) |
| Achats numériques | **Oui** | § 1 |
| Annonces | **Oui** | § 1 |
Classement attendu : **PEGI 3 à 7 / Tout public** ; la Console ajoutera les mentions « Achats intégrés » et « Interactions
entre utilisateurs ». Le résultat exact est donné par l'IARC, pas par nous.

## 4. Public cible et contenu  [DÉCISION OUVERTE : question 4 de `tache utilisateur.md`]
- **Variante A (recommandée par le juriste) : 13 ans et plus.** Le jeu n'entre pas dans le programme « Familles » ;
  la pub reste celle d'AdMob avec consentement (UMP) en Europe ; pas d'attrait particulier pour les enfants dans la fiche.
- **Variante B : aussi les moins de 13 ans.** Programme « Familles » obligatoire : seulement des SDK de pub certifiés
  « Familles », pas de pub personnalisée pour les enfants, pas d'identifiant publicitaire pour eux, règles plus strictes
  pour le pseudo du duel ; plus de travail (WFS-512 à reprendre).
Source : règles Google Play « Familles » et aide Play Console « Public cible et contenu » ; RGPD art. 8 et loi Informatique
et Libertés art. 45 (consentement d'un mineur de moins de 15 ans en France).

## 5. Fiche du store
- Texte : `store/fiche-fr.md` (17 voitures, aucune marque, achats et annonces annoncés). Fiche anglaise : WFS-509.
- Captures neuves : WFS-407.
