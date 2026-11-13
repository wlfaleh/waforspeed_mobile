# WFS-498 — texte du banc sauvegarde (portefeuille réel), lancé deux fois

1er lancement (partie neuve du banc : 250 ★ données au portefeuille, voiture m4 choisie, course Checkpoint jusqu'au record) :

```
étoiles : +250 ★ (banc) : solde 250 ★
boutique : couleur rose acheté pour m4 : 100 ★ (solde 150 ★)
boutique : achat refusé (finition chrome, m4) : 800 ★ demandées, 150 ★ au solde
BOUTIQUE essai (portefeuille 250 ★) : couleur rose sur m4 (100 ★) : acheté ; finition chromé sur m4 (800 ★) : étoiles insuffisantes (rien de débité) ; solde 150 ★
étoiles : débit refusé (voiture m4) : 2200 ★ demandées, 150 ★ au solde
BOUTIQUE essai : voiture m4 (2200 ★ = 3 or + 1 argent + 1 bronze = 21,78 €) : étoiles insuffisantes (rien de débité) ; solde 150 ★
BOUTIQUE essai : essai gratuit de la m4 disponible avant la course : oui
boutique : essai gratuit de la m4 utilisé (une seule course)
étoiles : +10 ★ (course terminée) : solde 160 ★
profil : course notée (Checkpoint, m4, 1450 pts, 0.295 km, 157 km/h, 0 frôlement(s)) : 1 course(s), 0.3 km au total
SAUVEGARDE écrite : car=m4 paint=rouge best=1450 (<sortie du banc>/sauvegarde-banc.txt)
PORTEFEUILLE : 160 ★
PORTEFEUILLE : voitures possédées : p208 abarth gt86
PORTEFEUILLE : essais gratuits faits : m4
```

2e lancement (sauvegarde relue) :

```
SAUVEGARDE relue : car=m4 paint=rouge best=1450
CATALOGUE couleur : 28 (8 gratuit(s)) : blanc gratuit, noir gratuit, gris gratuit, rouge gratuit, bleu gratuit, orange gratuit, jaune gratuit, vert gratuit, nacre 50★, bleunuit 50★, anglais 50★, bordeaux 50★, sable 50★, violet 50★, turquoise 50★, taxi 50★, bleuelectrique 50★, vertacide 50★, orangefluo 50★, rose 50★, jaunecitron 50★, violetneon 50★, rougevif 50★, cyan 50★, vertmenthe 50★, magenta 50★, corail 50★, bleuciel 50★
CATALOGUE couleur secondaire : 1 (0 gratuit(s)) : secondaire 50★
CATALOGUE finition : 8 (1 gratuit(s)) : brillant gratuit, metallise 100★, nacree 150★, mat 150★, satine 150★, candy 250★, chrome 400★, cameleon 400★
CATALOGUE skin : 4 (0 gratuit(s)) : rallye 300★, drifttokyo 600★, neonnuit cadeau, camouflage 450★
CATALOGUE autocollants : 5 (1 gratuit(s)) : essentiels gratuit, numeros 150★, bandes 200★, flammes 250★, etoiles 300★
CATALOGUE coefficients des classes : gratuites ×1, 1 ×1, 2 ×1.5, 3 ×2, 4 ×3, 5 ×4
PORTEFEUILLE : 160 ★
PORTEFEUILLE : voitures possédées : p208 abarth gt86
PORTEFEUILLE : essais gratuits faits : m4
PROFIL relu : pseudo=Banc 480 plaque=WFS-480 profil=1 arrivée=2026-11-13 12:25:02 | courses=1 km=0.3 vmax=157 km/h frôlements=0 victoires=0 voiture préférée=m4 | 1 dernière(s) course(s) : Checkpoint, m4, 1450 pts, 2026-11-13 12:25:43
```

Sur `main` : solde fictif de 250 ★ (« BOUTIQUE essai (solde fictif 250 ★) … »), aucun portefeuille, aucune ligne PORTEFEUILLE.
