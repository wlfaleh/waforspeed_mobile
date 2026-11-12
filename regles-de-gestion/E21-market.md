# Épic E21 — Market : étoiles achetées, « sans pub », pub volontaire

Créée le 2026-11-12 par le PO, sur demande de l'utilisateur (dernière étape de l'ordre **identité → score → voitures et couleurs → cadeau → market**).
Monnaie unique : les **étoiles ★** (R13, R16 de `recommandations-smart-brain.md`) ; portefeuille et prix des objets : WFS-498.

## Demande de l'utilisateur (résumé fidèle)
1. **Packs vendus** (Google Play Billing, produits consommables) : Médaille de bronze 100 ★ (0,99 €) ; Médaille d'argent 300 ★ + 10 % = 330 ★ (2,49 €) ;
   Médaille d'or 600 ★ + 20 % = 720 ★ (4,49 €) ; Coffre de champion 3 600 ★ + 25 % = 4 500 ★ (19,99 €) ; **Pack débutant** une seule fois par joueur,
   proposé après la 3e course : 500 ★ + 1 skin (1,99 €). Prix en euros convertis par Google ; **toutes les valeurs dans `configgame.json`, sauf les prix** (Play Console).
2. **Sans pub** (non consommable, 2,99 €, une fois pour toujours) : plus de proposition de vidéo ; le ×2 de fin de course est donné directement, toujours 5 par jour au plus.
3. **Pub volontaire** : en fin de course seulement, bouton « ×2 étoiles : regarder une vidéo », 5 par jour au plus, jamais imposée, jamais en course, jamais au lancement ;
   consentement RGPD au 1er lancement (Europe), refus possible (le jeu marche sans pub personnalisée) ; pas de pub disponible ou pas d'Internet → bouton caché, sans message.
4. **Écran Market** : depuis le menu, le garage et la fin de course (« Il te manque 240 ★ ») ; packs avec étoiles, bonus offert, prix en euros ;
   au garage, prix en ★ et équivalent approximatif en euros (règles UE) ; « Restaurer mes achats » ; français et anglais.
5. **Sécurité des achats** : étoiles ajoutées seulement après la confirmation de Google ; achat confirmé (acknowledge) ; jamais deux fois les étoiles pour un même achat ;
   achat interrompu rattrapé au lancement suivant ; « sans pub » revient seul après une réinstallation.
6. **Interdit** : caisses au hasard payantes, minuteurs payants, « énergie » ; objet vendu seulement contre de l'argent réel ; pub imposée ou pendant la course.
7. **Avant la mise en vente (bloquant, utilisateur + juriste)** : compte marchand Google Play ; avis d'avocat ; CGU et confidentialité à jour (achats, pub, remboursements, contact) ;
   Play Console : « Contient des annonces », « Achats intégrés », « Sécurité des données » (identifiant publicitaire), classification d'âge refaite ; e-mail de contact sur la fiche ; relecture du `@juriste`.
8. **Critères** : achats testés avec les comptes de test de la Play Console (chaque pack, sans pub, restauration, achat interrompu, achat annulé) ; test CPU : étoiles créditées une seule fois ;
   pub de test : ×2 seulement si la vidéo est vue en entier, 5 par jour au plus ; captures market, achat réussi, garage ★ + €, fin de course avec le ×2, en français et en anglais.

## Découpage (un ticket ≤ 1 jour, une session par ticket)
| Ticket | Sujet | Zone de code | Agent | Dépend de |
|---|---|---|---|---|
| WFS-510 | Market : catalogue des packs, livre des achats (une seule fois), « sans pub », compteur ×2 (5 / jour), boutique simulée pour PC et bancs | E3 + section `market` de `configgame.*` | `@dev` | WFS-498 |
| WFS-511 | Android : Google Play Billing (achat, confirmation, consommation, rattrapage, restauration) | E7 (`android/**`, `src/platform/*_android.cpp`) | `@dev` | WFS-510 |
| WFS-512 | Android : vidéo récompensée et consentement RGPD (UMP), bouton caché sans pub disponible | E7 | `@dev` | WFS-511 (mêmes fichiers) |
| WFS-513 | Écran Market (FR / EN), accès depuis le menu et le garage, « Restaurer mes achats » | E8 (+ accroche ≤ 10 lignes `garage.cpp`) | `@dev` | WFS-510, WFS-499, WFS-504 (langue) |
| WFS-514 | Fin de course : « ×2 étoiles : regarder une vidéo », ×2 direct avec « sans pub », « Il te manque N ★ » → Market ; ligne « Confidentialité des pubs » des Réglages | E8 | `@dev` | WFS-512, WFS-513 |
| WFS-515 | [TECHNIQUE] Essais réels : produits créés dans la Play Console, comptes de test, piste de test interne, scénarios d'achat et de pub (🟡 essai de l'utilisateur) | E7 / Publication app (aucun code du jeu) | agent expert ci cd (`@cicd`) | WFS-511 à 514, compte marchand et compte AdMob de l'utilisateur |
| WFS-516 | [TECHNIQUE] Déclarations Play Console et documents avant la vente (annonces, achats intégrés, sécurité des données, âge, contact, CGU / confidentialité) | Publication app (`store/**`) | agent expert ci cd (`@cicd`) | WFS-515, WFS-408, WFS-409, avis d'avocat, relecture `@juriste` ; **jour de publication** |

Ordre : WFS-510 → (WFS-511 → WFS-512) en parallèle de WFS-513 → WFS-514 → WFS-515 → WFS-516. Toutes **P2 Haute**, après E17, E18, E19 (cadeau) et E20 (anglais) (ordre de l'utilisateur : identité → score → voitures et couleurs → cadeau → market), derrière les P1.
Aucune tâche `@cicd` compagne de publication des paquets (le market n'est pas dans les paquets de cartes).

## Règles communes à toute l'épic
- RC-E21-01 **Une seule monnaie : les étoiles.** On n'achète jamais un objet avec de l'argent : l'argent achète des étoiles (packs), les étoiles achètent les objets (WFS-498, E18).
- RC-E21-02 **Tout reste gagnable en jouant** : aucun objet n'est vendu seulement contre de l'argent réel. Seul le « sans pub » est un achat en argent, et ce n'est pas un objet du jeu.
  Exception déjà décidée : la skin « Néon nuit » reste réservée au cadeau du jour (E19, gratuit) / aux succès (E18), donc gagnable en jouant, jamais vendue.
- RC-E21-03 **Interdits** : caisse ou tirage au hasard payant (en argent ou en étoiles), minuteur à payer pour attendre moins, « énergie » ou vies, pub imposée, pub en course, pub au lancement,
  pub interstitielle ou bannière. Le lead dev refuse toute PR qui en ajoute.
- RC-E21-04 **Valeurs dans `configgame.json`, section `market`** (documentée dans `configgame.README.md`), sauf les prix en argent, gérés dans la Play Console :
  identifiants des produits, étoiles de base, % offert, étoiles totales, pack débutant (étoiles, skin, course d'apparition), ×2 par jour, prix de secours en euros pour l'affichage sur PC / bancs seulement.
- RC-E21-05 **Le prix affiché vient de Google** (`ProductDetails`, prix déjà converti dans la monnaie du joueur). Le jeu n'écrit jamais un prix en argent en dur.
  L'équivalent en euros des objets au garage (règles UE) reste celui de WFS-498 (`equivalentEuros`, sur le pack de bronze).
- RC-E21-06 **Sécurité des achats** : crédit seulement à l'état « acheté » confirmé par Google (jamais à l'état « en attente ») ; un **livre des achats** dans la sauvegarde garde le jeton
  de chaque achat crédité : un jeton déjà dans le livre ne crédite plus jamais ; ordre fixe : (1) écrire crédit + jeton dans la sauvegarde, (2) consommer (consommables) ou confirmer
  (« sans pub », pack débutant) auprès de Google ; une étape coupée est reprise au lancement suivant. Tout achat non confirmé sous 3 jours est remboursé par Google : le rattrapage au lancement l'évite.
- RC-E21-07 **Pas de serveur** en v1.0 : les achats sont vérifiés par la bibliothèque Google sur le téléphone. Limite connue et acceptée (jeu solo) ; une vérification par serveur sera une US à part si l'utilisateur la demande.
- RC-E21-08 **Français et anglais** sur tous les écrans de l'épic (système de langue de WFS-504, épic E20 « Le jeu en anglais »), interface `design-street-racing`.
- RC-E21-09 **Aucune clé, aucun mot de passe, aucun identifiant de compte** dans le dépôt, un message ou le journal ; les identifiants publics des produits et des blocs de pub
  sont dans `configgame.json` ; en attendant ceux de l'utilisateur : **identifiants de test publics de Google** seulement.
- RC-E21-10 **Réseau** (`regles-reseau.md`) : les bibliothèques Google viennent du dépôt Maven de Google déjà utilisé par Gradle ; un blocage se signale au PO, jamais contourné.
- RC-E21-11 **Captures** (`tools/qa/captures_pr.sh`, banc PC avec la boutique simulée de WFS-510) dans **les deux langues** ; parcours complet menu → market → achat → retour, et course → fin de course → ×2 → market.
- RC-E21-12 **Avant la mise en vente** (point 7) : rien n'est publié avec achats ou pub tant que le compte marchand, le compte AdMob, l'avis d'avocat, les documents et la relecture du `@juriste` ne sont pas faits (WFS-516).
