# WFS-510 — capture 3 : 7 demandes de ×2 le même jour (5 accordées), minuit, heure qui recule, sans pub

Banc `WFS_BANC=market` (sans rendu, ≈ 11 s), commit 7a7b57e8, `configgame.json` du dépôt.

```
MARKET catalogue : sans_pub — Sans pub / No ads : plus de proposition de vidéo, ×2 direct, une seule fois (prix de secours PC : 2,99 €)
MARKET ×2 n° 1 le 13/11 (course 1) : accordé, +10 ★ ; solde 10 ★, 4 restant(s)
MARKET ×2 n° 2 le 13/11 (course 2) : accordé, +10 ★ ; solde 20 ★, 3 restant(s)
MARKET ×2 n° 3 le 13/11 (course 3) : accordé, +10 ★ ; solde 30 ★, 2 restant(s)
MARKET ×2 n° 4 le 13/11 (course 4) : accordé, +10 ★ ; solde 40 ★, 1 restant(s)
MARKET ×2 n° 5 le 13/11 (course 5) : accordé, +10 ★ ; solde 50 ★, 0 restant(s)
MARKET ×2 n° 6 le 13/11 (course 6) : refusé ; solde 50 ★, 0 restant(s)
MARKET ×2 n° 7 le 13/11 (course 7) : refusé ; solde 50 ★, 0 restant(s)
MARKET ×2 après minuit (14/11, course 8) : accordé, 4 restant(s)
MARKET ×2 une 2e fois pour la même course : refusé
MARKET ×2 heure du téléphone reculée au 13/11 après 5 ×2 le 14/11 : refusé (compteur pas remis à zéro)
MARKET ×2 direct avec « sans pub » : 5 accordés sur 7 courses le même jour
MARKET : OK (chaque achat crédité une seule fois)
```

Sur `main` : le banc n'existe pas (`WFS_BANC=market` inconnu).
