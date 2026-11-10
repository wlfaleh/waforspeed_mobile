Agent : Claude, agent dev3d 7 (01UY1Vv4FX)

**Ticket** : WFS-323 — règles : `regles-de-gestion/WFS-323.md` (P1, ordre de l'utilisateur du 2026-11-10 09:49 UTC)

## Description

- **Demande de l'utilisateur** : une vue Cockpit proche de sa référence (`qa/retours/2026-11-08-cockpit-reference-utilisateur.jpg`) : angle de caméra, habitacle large et lisible, toit au-dessus du pare-brise (ANO-207), plus les anomalies rattachées (ANO-185, 187, 188, 152, 203, 204).
- **Ce qui change** :
  - **Cadrage (RG-01, RG-03)** : champ vertical 72° → **62°** (`configgame.json` `camera.cockpit.zoom`, défaut de `configgame.h`, README) ; regard Normal −3° → **−5°**, Bas −9°, Haut −1° (`camera_vue.h`) : horizon à **42,9 %** depuis le haut (46,5 % avant), volant et compteur plus grands, en bas.
  - **Pavillon (RG-ANO-207)** : haut du pare-brise ramené à sa vraie place (0,21 m au-dessus de l'œil, 0,52 m devant : avant 0,40 / 0,42, hors de l'image), constantes partagées `HAUT_PARE_BRISE_*` (`vehicule_cockpit.h`) pour le dessin, les essuie-glaces et la pluie ; **ciel de toit** bombé, traverse avant, rails de toit au-dessus des portières, **deux pare-soleil** repliés ; on ne voit plus le ciel au-dessus du pare-brise.
  - **Rétroviseur intérieur (ANO-152, 207)** : coque moulée de 25 × 7 cm accrochée au pare-brise par un pied collé et une tige (avant : boîte en l'air, tige dans le ciel), glace claire à coins arrondis (le vrai reflet reste WFS-324).
  - **Formes nues remplacées (règle du 2026-11-09)** dans l'intérieur : nouvelles pièces modelées `dalle` (contour arrondi, chanfreins, dépouille), `profile` (section arrondie balayée), `cadreArrondi` ; montants, console, sélecteur, boîte à gants, airbag passager, jonc, aérateurs (lunettes et lames), cadres d'écran, coussin du volant, branches, palettes, boutons, molettes, essuie-glaces, portières (arêtes arrondies + appui de vitre, panneau gainé, accoudoir, poignée chromée). Plus aucune `boite`, `cylindre`, `anneau` ni `plan` dessinés dans `vehicule_cockpit.cpp` (seul le gabarit invisible du banc `cockpit-voisine` en garde).
  - **ANO-185 (planche qui clignote blanc / noir)** : occlusion de l'habitacle rangée dans la normale (`MAT_AO`, 0,3 à 0,8 selon l'orientation) : la planche ne prend plus la sonde d'éclairage et le ciel en plein ; avec le toit, le soleil n'éclaire plus la planche par le dessus.
  - **ANO-188 (compteur illisible)** : chiffres de la vitesse plus grands (VW 30 → 40 % de la hauteur du combiné, BMW 34 → 42 %), en plus du champ plus serré (×1,2).
  - **ANO-187 (volant fixe)** : vérifié : le volant tourne avec la direction (suite de 4 images, +0,38 → +1,53 rad) ; aucun changement de code nécessaire.
  - **ANO-204 (disques flous au choc)** : accroche de 6 lignes dans `src/render/effets_systeme.cpp` : en vue Cockpit, étincelles et volutes nées à moins de 2,5 m de l'œil (donc dans l'habitacle) ne sont pas dessinées.
  - **ANO-203 (checkpoint à travers l'habitacle)** : le portique est dessiné avec la profondeur, avant l'intérieur ; ce qu'on voyait passait par la vitre latérale et au-dessus du pare-brise sans toit : le toit et les portières le cachent maintenant (pas de capture de checkpoint de nuit dans ce banc : à confirmer sur téléphone).
  - **Banc** `WFS_BANC=cockpit-reference` (accroche d'1 ligne dans `desktop_main.cpp`) : scènes de la référence, mesure de l'horizon, du champ et du coût de l'intérieur.
- **Comment c'est vérifié** : 98 tests CPU verts (`tools/qa/tests_cpu.sh`), dont `test_regard_cockpit` (nouveau contrôle : horizon entre 40 et 45 %), `test_systems` (62°), `test_pluie` (pare-brise aux nouvelles cotes), `test_cockpit_ferme` ; banc `cockpit-reference` avant / après ; banc `cockpit` sur SF90, Abarth et Model 3 : **fenêtre de la rue cachée 0,0 %** pour les 3 regards ; coût : +1 appel de dessin, +0,38 Mo pour l'intérieur (RG-04).
- **Limites et risques connus** :
  - La part de l'image prise par l'intérieur monte à 52-59 % (mesure « toute l'image » du banc `cockpit`), au-dessus de la cible 30-35 % de WFS-282 : c'est le prix du toit, des portières et du champ de la référence (RG-01) ; la fenêtre de la rue reste à 0 %.
  - Intérieur encore sombre de jour (plastiques réels presque noirs, sous le toit) : plus sombre que la référence, qui est un jeu ; à juger sur téléphone.
  - Performance en qualité Basse non mesurée sur téléphone (seulement le coût CPU / mémoire du banc) ; le « R » rouge après un choc (ANO-204, 2e partie) est dans le HUD, hors de ma zone : non traité.
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
| Regard Bas | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/avant-golf-regard-bas.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-323/qa/pr/WFS-323/apres-golf-regard-bas.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv4FX4CXXFEkuhUsAzG
