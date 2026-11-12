Agent : agent dev-artiste-3d (01UY1Vv26c)

## Demande de l'utilisateur
« Jeu dans les roues de la M4 du trafic qui roule droit » (WFS-479, P1), puis, dans la même session : « Corrige le même problème sur la 9fs » (911, Pfeiler 9F S) et « Pour bussola c'est une catastrophe arrière et avant » (Ranger Bussola, `compass`). RG-04 élargi par le PO (M4, 911, Bussola).

## Causes trouvées (mesurées dans les modèles)
- **M4** et **911** : les deux roues avant sont modélisées **braquées de 20°** (axe mesuré +20,0°). Le moteur fait rouler chaque roue autour de l'axe X de la voiture : une roue dont l'axe est tourné bat à chaque tour (le « jeu »). Sur la M4, des morceaux de pneu que le moteur laisse fixes restaient en plus braqués ; sur la 911, le braquage décalait aussi les moyeux avant de 7,6 cm (gauche / droite).
- **Bussola** : roues droites, mais 7 à 9 **triangles égarés** par roue dans le matériau du pneu (lames étirées jusqu'au passage de roue, à 44 cm du moyeu) tournaient avec la roue (grandes lames noires qui balaient) et faussaient le centre de rotation ; les 4 **disques de frein** étaient décentrés de 8 cm.

## Ce qui change
- `tools/redresser_roues.cpp` (nouvel outil de la chaîne de conversion, cible CMake `redresser_roues`, appelé par `tools/convertir_voitures.sh` pour m4, p911, compass) : 0) lames égarées neutralisées sur place (triangles dégénérés, même nombre de triangles : `.lod` valable), pièce ronde décentrée recentrée sur le moyeu (seulement si les deux roues d'un essieu ont le même défaut) ; 1) roues braquées tournées autour du pivot de fusée commun à l'essieu (les deux moyeux reviennent au même niveau), positions, normales et tangentes TAN1 ; pièces déjà droites laissées en place. Rejouable : 2e passage « inchangée » sur les 3 voitures. Aucun cas particulier de voiture dans le moteur.
- `assets/cars/m4.wfm`, `p911.wfm`, `compass.wfm` : refaits par l'outil (depuis les fichiers de `main`).
- `tools/formes.json`, `tools/retoucher_formes.mjs`, `tools/effacer_logos.mjs` : empreinte « roues redressées » acceptée comme état final (la chaîne reste rejouable).
- `tests/test_roues.cpp` : RG-05 pour les 3 voitures : axe de la roue (normale de sa pièce la plus plate) et écart pivot du moteur / moyeu (cercle de la bande de roulement).
- `src/game/roues_banc.cpp/.h` : banc `roues-suivies` : vitrines `roues-<id>-tour-00…07` (roue avant gauche, un tour) et scènes `m4`, `m4-nuit`, `p911` (voiture du trafic tout droit) et `compass` (voiture du joueur : la Bussola n'est jamais dans le trafic, qui n'affiche que 4 modèles).

## Vérifié
- `test_roues` (main → branche) : M4 avant 19,98° → 0,01° ; 911 avant 20° → < 0,1° ; Bussola pivot / moyeu 52 / 15 / 25 / 19 mm → 6,4 / 0,3 / 0,3 / 0,8 mm. Toutes les roues :
```
m4 : roue 0 : pivot à 1.67 mm du moyeu, axe à 0.01° de l'axe X (WFS-479)
m4 : roue 1 : pivot à 1.11 mm du moyeu, axe à 0.01° de l'axe X (WFS-479)
m4 : roue 2 : pivot à 1.72 mm du moyeu, axe à 0.00° de l'axe X (WFS-479)
m4 : roue 3 : pivot à 1.67 mm du moyeu, axe à 0.00° de l'axe X (WFS-479)
p911 : roue 0 : pivot à 0.37 mm du moyeu, axe à 0.06° de l'axe X (WFS-479)
p911 : roue 1 : pivot à 0.30 mm du moyeu, axe à 0.01° de l'axe X (WFS-479)
p911 : roue 2 : pivot à 0.07 mm du moyeu, axe à 0.34° de l'axe X (WFS-479)
p911 : roue 3 : pivot à 0.32 mm du moyeu, axe à 0.05° de l'axe X (WFS-479)
compass : roue 0 : pivot à 6.42 mm du moyeu, axe à 0.06° de l'axe X (WFS-479)
compass : roue 1 : pivot à 0.33 mm du moyeu, axe à 0.33° de l'axe X (WFS-479)
compass : roue 2 : pivot à 0.34 mm du moyeu, axe à 0.12° de l'axe X (WFS-479)
compass : roue 3 : pivot à 0.82 mm du moyeu, axe à 0.56° de l'axe X (WFS-479)
```
- `test_fidelite_voitures` vert ; Bussola : voie −8,7 % → −0,9 % ; 911 : 2e mesure +5,2 % → 0 %. `test_roues_suivies` vert. `corriger_voitures --essai` : M4 inchangée.
- Captures ci-dessous : vitrines d'un tour de roue (avant : la roue se dandine, lames noires sous la Bussola ; après : stable), suites à 30 img/s.

## Limites et risques
- Bussola : roue avant gauche encore à 6,4 mm du moyeu (pneu à crampons asymétrique dans le modèle d'origine) et roue arrière droite à 0,56° de pince d'origine : seuils 7 mm / 1° pour elle (acceptés par le PO).
- 911 : une petite pièce noire derrière la jante (déjà droite, laissée en place) tourne avec la roue à ~2 cm de son centre (règle du moteur, non modifiée).
- 911 : pneus avant 0,670 m au lieu de 0,679 m, révélé par le redressement ; hors ticket (PO).
- Scène de nuit sur la Corniche : le banc plante (main compris) : nuit faite en Ville.
- Validation de l'utilisateur sur l'APK de `main` (🟡).

## Captures (avant = main, après = branche)
### tour-m4
![planche-tour-m4](https://raw.githubusercontent.com/wlfaleh/waforspeed_mobile/feature/waforspeed-479/qa/pr/WFS-479/planche-tour-m4.jpg)

### tour-p911
![planche-tour-p911](https://raw.githubusercontent.com/wlfaleh/waforspeed_mobile/feature/waforspeed-479/qa/pr/WFS-479/planche-tour-p911.jpg)

### tour-compass
![planche-tour-compass](https://raw.githubusercontent.com/wlfaleh/waforspeed_mobile/feature/waforspeed-479/qa/pr/WFS-479/planche-tour-compass.jpg)

### suite-m4
![planche-suite-m4](https://raw.githubusercontent.com/wlfaleh/waforspeed_mobile/feature/waforspeed-479/qa/pr/WFS-479/planche-suite-m4.jpg)

### suite-m4-nuit
![planche-suite-m4-nuit](https://raw.githubusercontent.com/wlfaleh/waforspeed_mobile/feature/waforspeed-479/qa/pr/WFS-479/planche-suite-m4-nuit.jpg)

### suite-p911
![planche-suite-p911](https://raw.githubusercontent.com/wlfaleh/waforspeed_mobile/feature/waforspeed-479/qa/pr/WFS-479/planche-suite-p911.jpg)

### suite-compass
![planche-suite-compass](https://raw.githubusercontent.com/wlfaleh/waforspeed_mobile/feature/waforspeed-479/qa/pr/WFS-479/planche-suite-compass.jpg)

Toutes les images une par une : `qa/pr/WFS-479/README.md`.
