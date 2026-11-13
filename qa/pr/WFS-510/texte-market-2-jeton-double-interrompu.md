# WFS-510 — capture 2 : même jeton deux fois, puis achat interrompu et relance

Banc `WFS_BANC=market` (sans rendu, ≈ 11 s), commit 7a7b57e8, `configgame.json` du dépôt.

```
MARKET   etoiles_argent : crédité, confirmé auprès de la boutique
MARKET   etoiles_argent : déjà crédité, confirmé auprès de la boutique
MARKET solde après « argent, même jeton reçu deux fois » : 5980 ★
MARKET   etoiles_or : rien de crédité ; message « Achat en attente de paiement »
MARKET solde après « or en attente de paiement » : 5980 ★
MARKET   etoiles_coffre : rien de crédité
MARKET solde après « coffre annulé » : 5980 ★
MARKET   etoiles_bronze : crédité, confirmé auprès de la boutique
MARKET solde après « bronze, application coupée avant la confirmation » : 6080 ★
MARKET   achats pas encore confirmés : 1
MARKET   etoiles_bronze : déjà crédité, confirmé auprès de la boutique
MARKET solde après « relance du jeu (rattrapage) » : 6080 ★
MARKET   achats pas encore confirmés : 0
```

Sur `main` : le banc n'existe pas (`WFS_BANC=market` inconnu).
