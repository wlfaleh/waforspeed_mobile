# WFS-510 — capture 4 : configgame.json modifié (argent à 350 ★), sans recompiler

Banc `WFS_BANC=market` (sans rendu, ≈ 11 s), commit 7a7b57e8, `configgame.json` du dépôt modifié seulement à `market.packs.etoiles_argent.etoiles` = 350 (variable `WFS_CONFIG`, même exécutable).

```
MARKET : configuration config350.json (copie de configgame.json) (291 clés lues, 0 erreur(s))
MARKET catalogue : etoiles_bronze — Médaille de bronze / Bronze medal : 100 ★ + 0 % = 100 ★ crédités (prix de secours PC : 0,99 €)
MARKET catalogue : etoiles_argent — Médaille d'argent / Silver medal : 350 ★ + 10 % = 385 ★ crédités (prix de secours PC : 2,49 €)
MARKET catalogue : etoiles_or — Médaille d'or / Gold medal : 600 ★ + 20 % = 720 ★ crédités (prix de secours PC : 4,49 €)
…
MARKET catalogue : etoiles_argent — Médaille d'argent / Silver medal : 350 ★ + 10 % = 385 ★ crédités (prix de secours PC : 2,49 €)
MARKET   etoiles_argent : crédité, confirmé auprès de la boutique
MARKET solde après « achat etoiles_argent » : 485 ★
MARKET   etoiles_argent : crédité, confirmé auprès de la boutique
MARKET   etoiles_argent : déjà crédité, confirmé auprès de la boutique
MARKET solde après « argent, même jeton reçu deux fois » : 6090 ★
MARKET : OK (chaque achat crédité une seule fois)
```

Sur `main` : le banc n'existe pas (`WFS_BANC=market` inconnu).
