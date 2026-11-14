# WFS-511 — capture 1 : tests JVM des achats (LogiqueAchatsTest)

Lancés en local le 2026-11-14 16:40:53 UTC (javac + JUnit 4.13.2, sans Android ni Google : faux client de facturation qui note chaque appel à Google).
En CI : `gradle testReleaseUnitTest` (déjà dans `build.yml` et `matin.yml`). Chaque ligne `ACHATS` est ce que le téléphone écrira dans `logcat` (jetons coupés à 8 caractères, RG-08).

```

▶ crediterPuisConsommer
ACHATS etoiles_or abcdefgh… : acheté, envoyé au jeu
ACHATS etoiles_or abcdefgh… : crédité par le jeu, consommation demandée
ACHATS abcdefgh… : consommé / confirmé chez Google
  OK

▶ horsLigne
ACHATS rattrapage impossible (code 3) : nouvel essai au prochain retour au jeu
ACHATS produit introuvable chez Google : etoiles_bronze (pack masqué)
ACHATS produit introuvable chez Google : sans_pub (pack masqué)
ACHATS produits : 0 trouvé(s) sur 2 demandé(s)
  OK

▶ restaurationSansPubApresReinstallation
ACHATS sans_pub abcdefgh… : acheté, envoyé au jeu (déjà confirmé)
ACHATS rattrapage : 1 achat(s) renvoyé(s) au jeu
ACHATS sans_pub abcdefgh… : déjà confirmé chez Google (restauration)
  OK

▶ enAttentePuisAchete
ACHATS etoiles_coffre abcdefgh… : en attente de paiement (rien de crédité)
ACHATS rattrapage : 0 achat(s) renvoyé(s) au jeu
ACHATS etoiles_coffre abcdefgh… : acheté, envoyé au jeu
  OK

▶ nonConsommableConfirme
ACHATS sans_pub abcdefgh… : acheté, envoyé au jeu
ACHATS sans_pub abcdefgh… : crédité par le jeu, confirmation demandée
  OK

▶ memeJetonDeuxFois
ACHATS etoiles_or abcdefgh… : acheté, envoyé au jeu
ACHATS etoiles_or abcdefgh… : acheté, envoyé au jeu
ACHATS rattrapage : 1 achat(s) renvoyé(s) au jeu
ACHATS etoiles_or abcdefgh… : crédité par le jeu, consommation demandée
ACHATS etoiles_or abcdefgh… : confirmation déjà faite ou en cours (rien de plus)
ACHATS abcdefgh… : consommé / confirmé chez Google
ACHATS rattrapage : 0 achat(s) renvoyé(s) au jeu
  OK

▶ dejaPossedeRelanceLeRattrapage
ACHATS achat en erreur (code 7) : sans_pub (rien de crédité)
  OK

▶ rattrapageAuLancement
ACHATS etoiles_bronze abcdefgh… : acheté, envoyé au jeu
ACHATS pack_debutant zyxwvuts… : acheté, envoyé au jeu
ACHATS rattrapage : 2 achat(s) renvoyé(s) au jeu
ACHATS etoiles_bronze abcdefgh… : crédité par le jeu, consommation demandée
ACHATS pack_debutant zyxwvuts… : crédité par le jeu, confirmation demandée
  OK

▶ consommationRefuseeRepriseAuRattrapage
ACHATS etoiles_or abcdefgh… : acheté, envoyé au jeu
ACHATS etoiles_or abcdefgh… : crédité par le jeu, consommation demandée
ACHATS abcdefgh… : consommation / confirmation refusée (code 6), reprise au prochain rattrapage
ACHATS etoiles_or abcdefgh… : acheté, envoyé au jeu
ACHATS rattrapage : 1 achat(s) renvoyé(s) au jeu
ACHATS etoiles_or abcdefgh… : crédité par le jeu, consommation demandée
  OK

▶ prixDeGoogle
ACHATS produit introuvable chez Google : etoiles_or (pack masqué)
ACHATS produits : 2 trouvé(s) sur 3 demandé(s)
  OK

▶ annuleParLeJoueur
ACHATS achat annulé par le joueur : etoiles_argent (rien de crédité)
  OK

▶ journalSansJetonComplet
ACHATS etoiles_or abcdefgh… : acheté, envoyé au jeu
ACHATS etoiles_or abcdefgh… : crédité par le jeu, consommation demandée
ACHATS abcdefgh… : consommé / confirmé chez Google
ACHATS rattrapage : 0 achat(s) renvoyé(s) au jeu
  OK

LogiqueAchatsTest : 12 test(s), 0 échec(s)
```
