# WFS-500 — journal du banc `sauvegarde` (lancé deux fois), avant / après

## Avant (main) : lignes CADEAU / PORTEFEUILLE / SAUVEGARDE
```
[info] SAUVEGARDE relue : car=m4 paint=rouge best=1450
[info] CATALOGUE skin : 4 (0 gratuit(s)) : rallye 300★, drifttokyo 600★, neonnuit cadeau, camouflage 450★
[info] PORTEFEUILLE : 160 ★
[info] PORTEFEUILLE : voitures possédées : p208 abarth gt86
[info] PORTEFEUILLE : essais gratuits faits : m4
```

## Après (branche) : mêmes lignes
```
[info] SAUVEGARDE relue : car=m4 paint=rouge best=1450
[info] CATALOGUE skin : 4 (0 gratuit(s)) : rallye 300★, drifttokyo 600★, neonnuit cadeau, camouflage 450★
[info] PORTEFEUILLE : 160 ★
[info] PORTEFEUILLE : voitures possédées : p208 abarth gt86
[info] PORTEFEUILLE : essais gratuits faits : m4
[info] CADEAU : cadeau-heure=1794560400 cadeau-decalage=3600 cadeau-jour=2026-11-13 cadeau-sans-rare=1 cadeau-total=1 cadeau-dernier=etoiles;14
[info] CADEAU : pret-voiture=— pret-recu=0 pret-fin=0
[info] CADEAU : déjà ouvert aujourd'hui le 2026-11-13 ; garantie 1/30 ; à gagner : 81 coloris, 12 skin(s) sur 3 voiture(s), 14 voiture(s) à prêter
```

## Après (branche), banc relancé deux fois à la main : 1er lancement (cadeau ouvert, 2e ouverture refusée)
```
[info] CADEAU : cadeau-heure=0 cadeau-decalage=0 cadeau-jour=— cadeau-sans-rare=0 cadeau-total=0 cadeau-dernier=—
[info] CADEAU : pret-voiture=— pret-recu=0 pret-fin=0
[info] CADEAU : disponible le 2026-11-13 ; garantie 0/30 ; à gagner : 81 coloris, 12 skin(s) sur 3 voiture(s), 14 voiture(s) à prêter
[info] cadeau du jour ouvert (2026-11-13) : etoiles;14 ; garantie 1/30 ; 1 cadeau(x) ouvert(s)
[info] cadeau du jour : pas de cadeau (déjà ouvert aujourd'hui)
[info] CADEAU essai : 1re ouverture etoiles;14 ; 2e ouverture le même jour refusée
[info] SAUVEGARDE écrite : car=m4 paint=rouge best=1450 (/tmp/claude-0/b/sauv/sauvegarde-banc.txt)
```
2e lancement (clés relues, même jour du banc : pas de 2e cadeau) :
```
[info] SAUVEGARDE relue : car=m4 paint=rouge best=1450
[info] CADEAU : cadeau-heure=1794560400 cadeau-decalage=3600 cadeau-jour=2026-11-13 cadeau-sans-rare=1 cadeau-total=1 cadeau-dernier=etoiles;14
[info] CADEAU : pret-voiture=— pret-recu=0 pret-fin=0
[info] CADEAU : déjà ouvert aujourd'hui le 2026-11-13 ; garantie 1/30 ; à gagner : 81 coloris, 12 skin(s) sur 3 voiture(s), 14 voiture(s) à prêter
```
