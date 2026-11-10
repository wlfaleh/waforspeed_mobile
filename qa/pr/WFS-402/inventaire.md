# WFS-402 — inventaire des fenêtres et des portes (RG-01)

> Claude, agent devartiste 2 (01UY1Vv5qgAR), 2026-11-10. Chiffres lus dans le code et dans le journal des bancs
> (`archi (WFS-161) : N bâtiments du kit posés`), branche `feature/waforspeed-402` comparée à `main`.

## 1. Bâtiments du kit d'architecture (WFS-161) : traités par ce ticket

| Carte | Bâtiments du kit | Styles | Avant (`main`) | Après (branche) |
|---|---|---|---|---|
| Ville | **625** | classique, moderne, industriel | trou en retrait, vitre plate au fond, porte peinte en aplat brun, meneau ou grille en barres (moderne, industriel) | dormant, ouvrants, petits bois, crémone, appui à glacis, volets roulants (moderne), porte à panneaux / vitrée / à rainures, béquille, seuil, portes de garage sectionnelles, porte vitrée des vitrines |
| Corniche | **172** | méditerranéen | idem, volets en pavés pleins toujours ouverts | persiennes à lames (22 % fermées ou entrouvertes), chambranle chanfreiné, portes à panneaux, à planches ou vitrées à grille |
| Campagne | **100** | village | idem, sans volets | volets pleins à barres et écharpe, linteau de pierre, portes à planches avec pentures, 0,90 × 2,15 m |
| **Total** | **897** | 6 styles | 0 bâtiment avec fenêtres et portes « réelles » | **897 / 897** avec dormant, ouvrants et porte modélisés (contrôle `RG-07` de `test_archi_geo` : chaque baie a sa menuiserie) |

Contrôles automatiques (`test_archi_geo`, 400 bâtiments par style) : au moins une porte côté rue par bâtiment ; retrait 15 à 25 cm ;
fenêtre d'habitation 0,80-1,40 × 1,15-2,15 m, porte-fenêtre ≥ 2,15 m ; porte de maison 0,90 × 2,15 m (± 1 %), passage utile 0,83 m
(référentiel `archi.porte_entree.*`) ; porte d'immeuble 1,0-1,4 × 2,2-3,0 m, cochère ≤ 2,5 m ; 4 modèles de fenêtre et 4 de porte par
style tirés ; deux bâtiments voisins jamais identiques ; volets fermés ou entrouverts : 21,5 à 22,2 % selon le style (RG-03 : 10 à 30 %).

## 2. Bâtiments hors du kit : renvoyés à WFS-410 et WFS-411 (décision du PO, 2026-11-10)

Le critère « 0 bâtiment sans porte ni fenêtres réelles » de WFS-402 vaut pour les bâtiments du kit (décision du PO). Les bâtiments
ci-dessous sont traités par **WFS-410** (lieux hors kit de la Corniche et de la Ville, église de la Campagne) et **WFS-411** (maisons
de la Neige au kit, style « chalet »), qui réutiliseront `ouvertures::baieComplete` (fonction publique livrée et testée ici).

| Carte | Bâtiments | État des ouvertures | Suite |
|---|---|---|---|
| Corniche | lieux de `plage.cpp` : casino, café, galerie, bars de plage, fête | fenêtres en rectangles sombres peints à plat | WFS-410 |
| Ville | lieux propres (casino, centre commercial, garage) | à vérifier un par un | WFS-410 |
| Campagne | église (`T_CHURCH`) : boîte et cône | **aucune** porte ni fenêtre | WFS-410 (vraie église modélisée) |
| Campagne | fermes et hangars (WFS-372), commerces du village | fermes : fenêtres avec dormant et volets (WFS-372) ; commerces : vitrines | rien |
| Grand Prix | stands, bar d'accueil, tour, stands de l'espace fans | modélisés par WFS-373 (vitrages, rideaux métalliques) | rien |
| Neige (paquet `noel-test`) | ≈ 60 « maison » de `decor.txt` (27 + 20 + 13) | boîtes de couleur et toit, **aucune** ouverture | WFS-411 (style « chalet » du kit) |
| Campagne, Corniche | pignons et arrières des maisons du kit | murs aveugles (le kit les traite en mitoyens) | à signaler au PO pour WFS-410 (fenêtres des pignons isolés) |
