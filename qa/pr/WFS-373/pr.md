## Agent : Claude, agent dev3d (01UY1Vv4L4)

**Ticket** : WFS-373 (P1, épic E15 « Décors », file Grand Prix) — règles : `regles-de-gestion/WFS-373.md`, `regles-de-gestion/E15-decors.md`

## Description

- **Demande de l'utilisateur** : plus aucune forme géométrique nue dans le décor du Grand Prix (tribunes, stands, portique, barrières, piles de pneus, panneaux) : de vrais objets 3D fidèles au réel ; avis de l'expert 3D (ANO-220) : stands vitrés avec auvent, murs de pneus empilés et sanglés, touffes semées au hasard, herbe tondue en bandes.
- **Inventaire (RG-01)** : `qa/pr/WFS-373/inventaire.md` (21 sortes d'objets, 5 000 objets posés, tous en boîtes / cylindres / cônes / triangles), aussi au journal de `main`.
- **Ce qui change** :
  - nouveau `src/world/decor_grandprix_modeles.*` : 21 modèles construits **une fois au chargement** (version proche et version allégée), aux vraies mesures (sources dans l'en-tête) : mur de pneus (7 piles de 4 pneus 205/55 R16 Ø 632 mm, flancs bombés, bande de roulement, pneu du haut peint rouge / blanc, 2 sangles jaunes), panneau publicitaire (bâche 11 × 1,2 m sur cadre d'aluminium, jambes de force, platines), tribune (gradins de béton à nez chanfreinés, sièges coques EN 13200, garde-corps en tubes, poteaux HEA / tubes, arbalétriers IPE, couverture et bardage en bac acier), mur des stands (muret préfabriqué + grillage de protection replié vers la piste, câbles, maille), **stand vitré** (rideaux métalliques à lames à demi levés, garage éclairé, linteau aux couleurs de l'équipe, auvent de 3 m sur consoles, terrasse à garde-corps vitré, mur-rideau à l'étage), drapeau (mât conique, pommeau, toile ondulée), poste de commissaire, panneau de freinage, portique des feux en treillis (caisson, visières, lentilles bombées), passerelle, semi-remorque d'équipe sans marque (16,5 m × 4,0 m, roues modélisées), bar d'accueil (murs-rideaux, parasols à baleines), tour de contrôle (vigie évasée), hélicoptère léger sans marque (fuselage profilé, rotor Ø 10,7 m, patins), stands de l'espace fans (toit de toile, lambrequin festonné), portails, grande roue en treillis (nacelles toujours d'aplomb), tentes dôme (arceaux, avancée, haubans), grillage à maille losangée, oiseaux, projecteurs de piste (mât de 19 m, 6 projecteurs) ;
  - `src/world/circuit.cpp` : chaque `box` / `cyl` / `cone` / `toit` / `tri` remplacé par la pose du modèle (palette de la carte, aucun appel de dessin en plus) ; projecteurs propres au Grand Prix (`T_PROJ`, même lumière) ; plus de rangées de touffes au bord de la piste : herbe tondue en bandes de 6 m et touffes semées au hasard de 6 à 45 m du bord ; la place des objets des sections est réservée sur leur longueur (plus de garage autour d'un poste de commissaire) ; comptage des objets par sorte au journal ;
  - banc `WFS_BANC=grandprix-objets` (`decor_grandprix_banc.cpp`, accroche de 2 lignes dans `desktop_main.cpp`) ; test `tests/test_decor_grandprix.cpp` (accroche CMake).
- **Taille ajoutée** : **0 Mo** d'actifs (tout est calculé au chargement ; aucun téléchargement : la recherche GitHub n'a donné que des modèles stylisés, exclus par RG-02 → RG-03).
- **Comment c'est vérifié** : `test_decor_grandprix` (mesures ±1 % de chaque objet, budget de triangles, aucun triangle dégénéré ni inversé, normales unitaires, version loin plus légère) ; `tools/qa/tests_cpu.sh` : 99 tests réussis ; captures avant / après (banc `cartes`, Grand Prix seulement) et gros plans de chaque sorte d'objet, jour et nuit (banc `grandprix-objets`, nouveau : pas d'« avant »).
- **Performance (RG-04)**, même vue « tribune du départ », rendu logiciel lavapipe (bruit ±10 %) : **Basse** 402 et 487 ms par image sur `main` → 412, 424 et 432 ms sur la branche (pas de baisse), 136 → 132 appels de dessin ; **Haute** 553 et 595 ms → 719 et 748 ms (+25 %, triangles des modèles proches) ; nuit 385 et 442 → 423 et 465 ms.
- **Limites et risques connus** : coût en Haute plus élevé (modèles proches dans les 200 m) ; en Basse, un mur de pneus sur deux reste retiré (règle RG-C2 de WFS-140, inchangée) ; le banc de gros plans écrit une erreur « pipelines créés pendant la course » propre à sa vue libre (même chose que `campagne-objets`) ; arbres et lampadaires communs hors ticket (végétation : WFS-388).

## Captures pour le lead dev et l'agent 3D

### Avant / après (banc `cartes`, Grand Prix)

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `carte-circuit-foret` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/avant-carte-circuit-foret.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-carte-circuit-foret.jpg?raw=true) |
| `carte-circuit-gravier` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/avant-carte-circuit-gravier.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-carte-circuit-gravier.jpg?raw=true) |
| `carte-circuit-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/avant-carte-circuit-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-carte-circuit-jour.jpg?raw=true) |
| `carte-circuit-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/avant-carte-circuit-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-carte-circuit-nuit.jpg?raw=true) |
| `carte-circuit-tribune-depart-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/avant-carte-circuit-tribune-depart-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-carte-circuit-tribune-depart-basse.jpg?raw=true) |
| `carte-circuit-tribune-depart-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/avant-carte-circuit-tribune-depart-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-carte-circuit-tribune-depart-nuit.jpg?raw=true) |
| `carte-circuit-tribune-depart` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/avant-carte-circuit-tribune-depart.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-carte-circuit-tribune-depart.jpg?raw=true) |
| `carte-circuit-tribune-epingle-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/avant-carte-circuit-tribune-epingle-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-carte-circuit-tribune-epingle-nuit.jpg?raw=true) |
| `carte-circuit-tribune-epingle` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/avant-carte-circuit-tribune-epingle.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-carte-circuit-tribune-epingle.jpg?raw=true) |
| `carte-circuit-zone-paddock` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/avant-carte-circuit-zone-paddock.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-carte-circuit-zone-paddock.jpg?raw=true) |

### Gros plans de chaque objet (banc `grandprix-objets`, après seulement)

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `grandprix-objets-accueil-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-accueil-nuit.jpg?raw=true) |
| `grandprix-objets-accueil` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-accueil.jpg?raw=true) |
| `grandprix-objets-camion` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-camion.jpg?raw=true) |
| `grandprix-objets-commissaire` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-commissaire.jpg?raw=true) |
| `grandprix-objets-drapeau` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-drapeau.jpg?raw=true) |
| `grandprix-objets-fans` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-fans.jpg?raw=true) |
| `grandprix-objets-freinage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-freinage.jpg?raw=true) |
| `grandprix-objets-grande-roue-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-grande-roue-nuit.jpg?raw=true) |
| `grandprix-objets-grande-roue` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-grande-roue.jpg?raw=true) |
| `grandprix-objets-grillage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-grillage.jpg?raw=true) |
| `grandprix-objets-helicoptere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-helicoptere.jpg?raw=true) |
| `grandprix-objets-mur-stands` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-mur-stands.jpg?raw=true) |
| `grandprix-objets-passerelle` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-passerelle.jpg?raw=true) |
| `grandprix-objets-pneus-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-pneus-nuit.jpg?raw=true) |
| `grandprix-objets-pneus` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-pneus.jpg?raw=true) |
| `grandprix-objets-portique-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-portique-nuit.jpg?raw=true) |
| `grandprix-objets-portique` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-portique.jpg?raw=true) |
| `grandprix-objets-projecteur-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-projecteur-nuit.jpg?raw=true) |
| `grandprix-objets-projecteur` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-projecteur.jpg?raw=true) |
| `grandprix-objets-publicite` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-publicite.jpg?raw=true) |
| `grandprix-objets-stands-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-stands-nuit.jpg?raw=true) |
| `grandprix-objets-stands` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-stands.jpg?raw=true) |
| `grandprix-objets-tentes` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-tentes.jpg?raw=true) |
| `grandprix-objets-tour-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-tour-nuit.jpg?raw=true) |
| `grandprix-objets-tour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-tour.jpg?raw=true) |
| `grandprix-objets-tribune-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-tribune-nuit.jpg?raw=true) |
| `grandprix-objets-tribune` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-objets-tribune.jpg?raw=true) |
| `grandprix-perf-basse` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-perf-basse.jpg?raw=true) |
| `grandprix-perf-haute` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-373/qa/pr/WFS-373/apres-grandprix-perf-haute.jpg?raw=true) |
