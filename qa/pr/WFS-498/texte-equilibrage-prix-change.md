# WFS-498 — même banc après un prix changé dans configgame.json, sans recompiler

Seule modification : `boutique.voitures.corsa.prix` 250 → 400 (fichier donné par `WFS_CONFIG`, même `wfs_bench`). Code de sortie du banc : 3 (écart signalé).

```
ÉQUILIBRAGE : configuration <configgame.json modifié> (273 clés lues, 0 erreur(s))
ÉQUILIBRAGE : gain 10 ★ par course, 2.96 min par course
ÉQUILIBRAGE : 1re voiture (tcross, 300 ★) : 30 courses (cible 22 à 28)
ÉQUILIBRAGE : les 14 voitures : 24 500 ★, 2 450 courses, 120.9 h (cible 108 à 132 h)
ÉQUILIBRAGE (information) : peintures, finitions, skins et dessins : 90 550 ★ ; tout : 115 050 ★, 11 505 courses, 567.6 h
ÉQUILIBRAGE : équivalents : 300 ★ = 1 argent = 2,97 € ; 24 500 ★ = 40 or + 1 argent + 2 bronze = 242,55 €
ÉQUILIBRAGE ÉCART : 1re voiture : 30 courses, hors de 22 à 28
ÉQUILIBRAGE : ÉCHEC (1 écart(s))
```
