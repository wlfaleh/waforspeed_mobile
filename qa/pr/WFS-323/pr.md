Agent : Claude, agent dev3d 7 (01UY1Vv4FX)

**Ticket** : WFS-323 — règles : `regles-de-gestion/WFS-323.md` (P1, ordre de l'utilisateur du 2026-11-10 09:49 UTC)

## Description

- **Demande de l'utilisateur** : une vue Cockpit proche de sa référence (`qa/retours/2026-11-08-cockpit-reference-utilisateur.jpg`) : angle de caméra, habitacle large et lisible, toit au-dessus du pare-brise (ANO-207), plus les anomalies rattachées (ANO-185, 187, 188, 152, 203, 204).
- **Ce qui change** :
  - **Cadrage (RG-01, RG-03)** : champ vertical 72° → **62°** (`configgame.json` `camera.cockpit.zoom`, défaut de `configgame.h`, README) ; regard Normal −3° → **−5°**, Bas −9°, Haut −1° (`camera_vue.h`) : horizon à **42,9 %** depuis le haut (46,5 % avant), volant et compteur plus grands, en bas.
  - **Pavillon (RG-ANO-207)** : haut du pare-brise ramené à sa vraie place (0,21 m au-dessus de l'œil, 0,52 m devant : avant 0,40 / 0,42, hors de l'image), constantes partagées `HAUT_PARE_BRISE_*` (`vehicule_cockpit.h`) pour le dessin, les essuie-glaces et la pluie ; **ciel de toit** bombé, traverse avant, rails de toit au-dessus des portières, **deux pare-soleil** repliés ; on ne voit plus le ciel au-dessus du pare-brise.
  - **Rétroviseur intérieur (ANO-152, 207)** : coque moulée de 25 × 7 cm accrochée au pare-brise par un pied collé et une tige de 8 cm, visible au-dessus de la coque (avant : boîte en l'air, tige dans le ciel), glace claire à coins arrondis (le vrai reflet reste WFS-324).
  - **Formes nues remplacées (règle du 2026-11-09)** dans l'intérieur : nouvelles pièces modelées `dalle` (contour arrondi, chanfreins, dépouille), `profile` (section arrondie balayée), `lisse` (pièce balayée le long d'une ligne brisée), `cadreArrondi` ; **visière du combiné** moulée (revue du lead dev) ; **montants** à garniture de 9 cm de face (5 avant, avis agent 3D : « arceau ») ; axes des pare-soleil en plastique mat (point blanc) ; montants, console, sélecteur, boîte à gants, airbag passager, jonc, aérateurs (lunettes et lames), cadres d'écran, coussin du volant, branches, palettes, boutons, molettes, essuie-glaces, portières (arêtes arrondies + appui de vitre, panneau gainé, accoudoir, poignée chromée). Plus aucune `boite`, `cylindre`, `anneau` ni `plan` dessinés dans `vehicule_cockpit.cpp` (seul le gabarit invisible du banc `cockpit-voisine` en garde).
  - **ANO-185 (planche qui clignote blanc / noir)** : occlusion de l'habitacle rangée dans la normale (`MAT_AO`, 0,3 à 0,8 selon l'orientation) : la planche ne prend plus la sonde d'éclairage et le ciel en plein ; avec le toit, le soleil n'éclaire plus la planche par le dessus.
  - **ANO-188 (compteur illisible)**, 2e passage après le refus du lead dev : (1) cause trouvée : devant le combiné, la face de la planche partait de la lèvre, 8 cm devant l'écran, et son bord coupait les chiffres à mi-hauteur (« 58 » lu « SA », déjà sur `main`) : elle part maintenant du bas du creux ; (2) la bande du combiné vue par l'ouverture haute du volant est calculée pour chaque voiture (`bandeLisibleCombine` : rayons de l'œil de la caméra vers le bord intérieur du haut de la jante et vers le haut du coussin) et la vitesse y est placée, en chiffres d'au moins 22 % de la hauteur (30 % sur l'écran Ferrari, 40 % VW, 42 % BMW), pour tous les styles (X5 : sous la jante) ; le rapport se pose au-dessus de la vitesse (SF90, Abarth). Nouveau test dans `test_cockpit` (20 voitures), bande au journal du banc, agrandissement du combiné de chaque voiture.
  - **ANO-187 (volant fixe)** : vérifié : le volant tourne avec la direction (suite de 4 images, +0,38 → +1,53 rad) ; aucun changement de code nécessaire.
  - **ANO-204 (disques flous au choc)** : accroche de 10 lignes dans `src/render/effets_systeme.cpp` : en vue Cockpit, une étincelle ou une volute **dans la boîte de l'habitacle** (repère de l'intérieur) n'est pas dessinée ; dehors (choc sur le côté, vu par la vitre), elle reste (revue du lead dev : plus de filtre à 2,5 m).
  - **ANO-203 (checkpoint à travers l'habitacle)** : le portique est dessiné avec la profondeur, avant l'intérieur ; ce qu'on voyait passait par la vitre latérale et au-dessus du pare-brise sans toit : le toit et les portières le cachent maintenant (pas de capture de checkpoint de nuit dans ce banc : à confirmer sur téléphone).
  - **Banc** `WFS_BANC=cockpit-reference` (accroche d'1 ligne dans `desktop_main.cpp`) : scènes de la référence, mesure de l'horizon, du champ et du coût de l'intérieur.
- **Comment c'est vérifié** : 98 tests CPU verts (relancés après le 2e passage) (`tools/qa/tests_cpu.sh`), dont `test_regard_cockpit` (nouveau contrôle : horizon entre 40 et 45 %), `test_systems` (62°), `test_pluie` (pare-brise aux nouvelles cotes), `test_cockpit_ferme` ; banc `cockpit-reference` avant / après ; banc `cockpit` sur SF90, Abarth et Model 3 : **fenêtre de la rue cachée 0,0 %** pour les 3 regards ; coût : +2 appels de dessin, +0,38 Mo pour l'intérieur Golf (RG-04) ; combinés des 20 voitures regardés en agrandissement.
- **Limites et risques connus** :
  - La part de l'image prise par l'intérieur monte à 52-59 % (mesure « toute l'image » du banc `cockpit`), au-dessus de la cible 30-35 % de WFS-282 : c'est le prix du toit, des portières et du champ de la référence (RG-01) ; la fenêtre de la rue reste à 0 %.
  - Intérieur encore sombre de jour (plastiques réels presque noirs, sous le toit) : plus sombre que la référence, qui est un jeu ; à juger sur téléphone.
  - Performance en qualité Basse non mesurée sur téléphone (seulement le coût CPU / mémoire du banc) ; le « R » rouge après un choc (ANO-204, 2e partie) est dans le HUD, hors de ma zone : non traité.
  - Pour le PO (hors de ma zone) : les gouttes de pluie sont dessinées devant le rétroviseur intérieur la nuit (déjà sur `main`, pluie en 2D) ; la cible 30-35 % de WFS-282 est à revoir avec la référence.
  - V60 : écran du combiné de 7 cm de haut, vitesse lisible mais petite ; sur l'Abarth et la SF90, le haut du rapport passe sous la jante.
  - L'œil reste à la place du conducteur (RG-01) : le rétroviseur est sur l'axe de la voiture, donc plus à droite que dans la référence.

## Captures pour le lead dev

Dossier : [`qa/pr/WFS-323`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323) (README avec toutes les captures et les mesures).

| Scène | Référence · Avant (main) · Après (branche) |
|---|---|
| Golf, Corniche, jour | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/comparaison-golf-corniche-jour.jpg?raw=true) |
| M4, Corniche, jour | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/comparaison-m4-corniche-jour.jpg?raw=true) |
| X5, Corniche, jour | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/comparaison-x5-corniche-jour.jpg?raw=true) |
| Golf, Ville, nuit | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/comparaison-golf-ville-nuit.jpg?raw=true) |

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| Pavillon (regard Haut) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/avant-golf-pavillon.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/apres-golf-pavillon.jpg?raw=true) |
| Volant (braquage, 4 images) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/avant-volant-suite.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/apres-volant-suite.jpg?raw=true) |
| Combiné de la M4 (agrandi) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/avant-combine-m4.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/apres-combine-m4.jpg?raw=true) |
| Combiné de la X5 (agrandi) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/avant-combine-x5.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/apres-combine-x5.jpg?raw=true) |
| Combinés des 20 voitures | — | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/apres-combines-20-voitures.jpg?raw=true) |
| Regard Bas | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/avant-golf-regard-bas.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/apres-golf-regard-bas.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv4FX4CXXFEkuhUsAzG
