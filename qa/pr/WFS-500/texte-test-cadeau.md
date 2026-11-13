# WFS-500 — sortie de `test_cadeau` (RG-11)

Commande : `./build/test_cadeau` (aussi lancé par `bash tools/qa/tests_cpu.sh build`, groupe `texte` de la CI).

```
OK    1970-01-01 = jour 0
OK    2024-02-29 existe, 2025-02-29 et 2026-04-31 refusés
OK    2026-11-13 aller-retour
OK    jour local à Paris (00:30) ≠ jour UTC (23:30 la veille)
OK    jour local à New York (23:00, décalage négatif)
OK    maintenant() : heure et décalage plausibles
  100 000 tirages (graine 500, garantie coupée) :
  | gain | mesuré | attendu | écart |
  |---|---|---|---|
  | 10-15 ★ | 44.93 % | 45.00 % | -0.07 pt |
  | 16-25 ★ | 27.91 % | 28.00 % | -0.09 pt |
  | 26-40 ★ | 9.16 % | 9.00 % | +0.16 pt |
  | 41-50 ★ | 3.07 % | 3.00 % | +0.07 pt |
  | coloris | 7.90 % | 8.00 % | -0.10 pt |
  | skin | 4.06 % | 4.00 % | +0.06 pt |
  | voiture prêtée | 2.97 % | 3.00 % | -0.03 pt |
OK    (a) chaque chance à ±0,5 point sur 100 000 tirages
  (b) 92965 cadeaux en étoiles, minimum 10 ★
OK    (b) aucun gain < 10 ★ sur 100 000 tirages (replis et tranche 1-15 compris)
OK    étoiles jamais au-dessus de 50 ★ (tranches du jeu)
  (c) 2 000 garanties : 1112 skins, 888 voitures (poids 4 / 3 : 55.6 % de skins)
OK    (c) 29 sans voiture ni skin → le 30e donne une voiture ou une skin, compteur à 0
OK    (c) garantie : skin et voiture tirées avec les poids 4 et 3 (±3 points)
OK    (c) une seule catégorie possible (voiture) : celle-là
OK    (c) une seule catégorie possible (skin) : celle-là
OK    (c) aucune possible : étoiles (≥ 10) et compteur remis à 0
OK    une voiture ou une skin gagnée remet le compteur à 0
OK    (d) catégories vides : toujours des étoiles ≥ 10
OK    (d) replis ≈ 15 % des tirages (coloris + skin + voiture)
OK    (d) repli : tranches renormalisées 45 / 28 / 9 / 3 (10-15 ★ ≈ 52,9 %, ±1,5 pt)
OK    (e) partie neuve : cadeau disponible tout de suite
OK    (e) 23:59 ouvert, 2e ouverture le même jour refusée
OK    (e) état « déjà ouvert » à 23:59:40
OK    (e) 00:01 le lendemain : nouveau cadeau
OK    (e) sauvegarde à jour avant le gain (dernier = gain rendu)
OK    (e) une semaine sans jouer : un seul cadeau (pas d'accumulation)
OK    même graine, même cadeau (RG-10)
OK    (f) heure reculée de 2 jours : aucun cadeau
OK    (f) 1 s avant l'heure du dernier cadeau : toujours bloqué
OK    (f) heure repassée, même jour : déjà ouvert
OK    (f) le lendemain : de nouveau disponible
OK    (f) fuseau à l'est (Dubaï, 02:30 le 14 local) : nouveau jour local → disponible
OK    (f) fuseau à l'ouest (New York, 18:30 le 13 local) : même jour → refusé
OK    (f) fuseau changé mais heure reculée : bloqué
OK    (g) 9 clés cadeau-* / pret-* écrites
OK    (g) toutes les clés relues à l'identique
OK    (g) relu → texte identique
OK    (g) aucun cadeau ouvert : aucune clé cadeau-* écrite
OK    (g) ancienne sauvegarde sans clés cadeau-* : lue, cadeau disponible
OK    (g) valeurs mal écrites ignorées une à une (défauts), sans erreur
OK    (h) total 101 % : refusé, chances du jeu gardées, message au journal
OK    (h) chances changées avec un total de 100 % : acceptées
OK    (h) tranche min > max : tranche du jeu gardée
OK    (h) garantie < 2 et minimum < 1 : hors bornes, valeurs du jeu
OK    valeurs du jeu : total des chances = 100 %, minimum 10 ★, garantie 30, prêt 3 h, activation 24 h
OK    résultat « type;valeur » écrit puis relu (4 types)
OK    entrée : les 3 voitures gratuites du joueur, avec des coloris à gagner
OK    entrée : 14 voitures à prêter, jamais une retirée (licence NC) ni une possédée
OK    entrée : les coloris gratuits (déjà possédés) n'y sont pas
OK    entrée : la voiture déjà prêtée n'est pas reprêtée
OK : 0 échec(s)
```
