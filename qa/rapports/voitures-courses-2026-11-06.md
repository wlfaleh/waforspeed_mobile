# Recette transversale : les voitures en course, dans chaque type de course (2026-11-06)

QA lead, à la demande de l'utilisateur : « Lance le QA sur le fonctionnement des voitures dans les différents types de course ».
Ce n'est pas la recette d'un ticket. L'aspect visuel des 20 voitures n'est pas refait (`voitures-2026-11-06.md`, `WFS-170.md`) :
on regarde seulement comment elles se comportent en course.

## Verdict global : **PASSABLE AVEC ANOMALIES MINEURES**

Aucune anomalie bloquante. Dans les 4 modes, la voiture choisie roule, accélère, freine, tourne, recule et prend la nitro.
Les adversaires, la police et l'adversaire du duel ont le bon modèle et roulent les roues au sol. Les courses vont jusqu'à
la fin, et « Rejouer » comme « Menu » marchent. On relève 6 anomalies mineures : chiffres de la fiche, freinage, départ et
équilibre de la Course à 5, nitro sans trafic, et le banc d'endurance qui fait passer la CI de `main` au rouge.

## Sources

| Source | Détail |
|---|---|
| Release `apercu` de `main` | description : « construction 751, commit 2effafa (459/459 à jour) ». Le run 42253351587 (2effafa) finit en **échec**, à cause du job `endurance` (fuite de mémoire +11,2 %). Tous les jobs `banc-essai` sont verts. `main` est aujourd'hui à d3d687b, 60 commits plus loin. Depuis 2effafa, `src/game` n'a changé que dans `duel*.cpp` (+11 lignes), `sauvegarde.*` et `trace_cartes*` (2 lignes) ; la conduite, la Course à 5 et la Poursuite n'ont pas bougé. |
| Journaux CI | jobs `endurance` des runs 2effafa, 01fff1d, a5faca5, 6517cdf, 57fac94 ; `banc-essai` (partie1a/1b/2a/2b/2c, nuit, vitrines, realisme, ecrans-2400) de 2effafa. |
| Tests CPU (d3d687b, compilés en local) | `test_track`, `test_course`, `test_course5` (contient `test_poursuite`), `test_duel`, `test_relais`, `test_traffic`, `test_trafic_vivant`, `test_conduite_libre`, `test_commandes`, `test_roues`, `test_effets`, `test_feux_modele`, `test_records`, `test_trace`, `test_minicarte`, `test_systems` : **16/16 verts**. |
| Banc `wfs_bench` local (lavapipe, d3d687b) | `WFS_BANC=course5` : Course à 5 (2 km), parcours en Difficile (4 km), Poursuite complète, « Rejouer », « Menu », redémarrage. 0 écart, 12 min 25 s. |
| Mesures hors dépôt (scratchpad, classes du jeu, rien n'est committé) | `mesure` (0-100, 0-200, vitesse max, nitro et freinage des 20 voitures), `depart` (départ de la Course à 5), `course5sim` (Course à 5 de 4 km, 7 voitures × 3 difficultés × 5 graines × 2 cartes), `checkpoint` (Checkpoint avec trafic et chocs, 20 voitures × 3 graines sur Ville et Corniche, 8 voitures sur Campagne). |

## Inventaire des types de course

Modes (`course5.cpp` `nomMode`) : **Checkpoint** (60 s au départ, +30 s par checkpoint), **Course à 5** (2 / 4 / 6 / 10 km ;
Facile / Normale / Difficile ; sans trafic), **Poursuite** (10 km ; police en Peugeot 208, ou en Golf si le joueur a la 208 ;
renforts, embouteillages, 6 surprises dont le camion en travers (X5) et le ralentisseur), **Duel entre amis** (2 à 4
pilotes, Wi-Fi local ou relais Internet, sans trafic). Terrains actifs : Ville, Corniche (plage), Campagne, Grand Prix
(circuit), de jour et de nuit. La pluie suit le moment choisi (route mouillée, `route-*-pluie`). Qualités Basse / Moyenne /
Haute (+ Ultra, WFS-150). Caméras : Poursuite, Proche, Capot, Cockpit, et une vue de dessus pour la vitrine.

## Résultat par type de course

| Type | Ce qui a été vérifié | Preuve | Résultat | Anomalies |
|---|---|---|---|---|
| **Checkpoint**, Ville de jour | décompte 3 → GO ; la Golf accélère (0 → 2 km/h au GO) ; fin « temps écoulé » avec distance, vitesse max (211 km/h, sous les 236 de la Golf), checkpoints ; « Rejouer » / « Menu » | `course-decompte-3`, `course-decompte-go`, `course-cam0-*`, `course-fin`, `fin-menu`, `fin-retour-menu` | OK | — |
| Checkpoint, voiture choisie au garage | Golf turquoise choisie au garage → en course ; 911 choisie en Gris → en course ; SF90 en course | `garage-course`, `course-p911-gris`, `course-sf90` ; journal `endurance` : « garage : voiture m4 » (cycle 3), « voiture p911 » (cycle 6), voiture gardée après le retour d'arrière-plan et le changement de carte ; code : `Game::setCar` (garage), `changerCarte` garde `carDef_`, `android_main` démarre avec `sauvegarde().car` | OK, pas toujours la Golf | — |
| Checkpoint, les 20 voitures | elles roulent toutes, accrochent des checkpoints, prennent des chocs et des accidents sans rester bloquées ; vitesse max mesurée en course de 185 (Compass) à 328 km/h (SF90 avec nitro) | sim. `checkpoint` (Ville, Corniche : 20/20 voitures ; Campagne : 8/20) : de 15,7 à 52 checkpoints, aucune voiture à 0 km | OK | anomalie (1) (2) |
| Checkpoint, 4 terrains, jour / nuit | une course par terrain et par moment, jusqu'à la fin, avec la voiture du cycle (Golf, M4, 911) : 205 km/h (Golf, Corniche), 273 (M4, Grand Prix), 265 (911, Campagne de nuit) | `endurance-fin-*` (8), `course-nuit-ville/campagne/circuit-*`, `terrain-rouler` ; journal `endurance` (10 cycles, 3 pauses, 3 retours d'arrière-plan) | OK | anomalie (6) (CI) |
| **Course à 5** (Normale, 2 km) | grille de 4 ordinateurs (911, X5, T-Cross, Compass : bons modèles, roues au sol, peintures tirées) ; décompte, puis course sans trafic ; nitro des ordinateurs (journal) ; aspiration ; fin avec classement (arrivés, puis distance) ; « Rejouer » avec d'autres adversaires | `course5-grille`, `course5-course`, `course5-fin`, `course5-rejouer`, banc local (même graine : 3e place en 46,2 s ; 911 en 36,3 s) | OK | anomalie (3) (4) (5) |
| Course à 5, difficultés | Difficile / 4 km par le parcours au doigt : retenus, course finie, « Menu » ; même graine : Difficile 155,7 s < Normale 164,6 s < Facile 184,9 s | `parcours5-difficile`, banc local « parcours course à 5 (RG-10) : 0 écart », `test_course5` | OK | anomalie (5) |
| Course à 5, avec d'autres voitures que la Golf | sim. `course5sim` : SF90, M4, Mustang, X5, P208, Compass, Golf ; chaque voiture finit les 4 km (0 non-arrivée sur 210 courses), place selon la voiture | voir le tableau des mesures | fonctionne | anomalie (5) |
| **Poursuite** | police (208) aux gyrophares, au contact, qui tente de pousser ; barrage, embouteillage (le joueur à 2-4 km/h dans la file), ralentisseur (−3 au-dessus de 90 km/h), camion X5 en travers ; ARRÊTÉ à score 0 ; VICTOIRE à 10 km ; « Rejouer », « Menu », mode gardé au redémarrage | `poursuite-*` (10), `records-poursuite`, `voiture-basse-poursuite`, banc local « parcours complet et redémarrage : 0 écart », `test_course5` (« poursuite police : tout est OK ») | OK | — (ANO-57 et ANO-59 en partie, déjà ouvertes) |
| **Duel entre amis** à 2 | adversaire réseau en M4 (bon modèle, roues au sol, étiquette « Nadia ») ; classement à 2 ; voiture et peinture envoyées = celles du garage (`salon.cpp` l. 157 / 172, `g.car->id`) | `duel-course`, `duel-reseau-course(-2400)`, `duel-resultat`, `test_duel`, `test_relais` | OK | anomalie (4) |
| Duel à 4 | arrivée des 4 pilotes, classement 1 à 4 avec les écarts, « Revanche » / « Retour » | `duel-resultat-4(-2400)`, journal « parcours duel : conforme (0 écart) » | OK | — (ANO-93, ANO-94 déjà ouvertes) |
| Commandes : gaz, frein, braquage, recul, nitro | braquage aux flèches à 30, 120 et 250 km/h, de jour et de nuit, sans voile ; marche arrière ; demi-tour ; contresens ; nitro (lignes de vitesse, jauge) | `braquage-*` (12), `commandes-marche-arriere`, `conduite-demitour`, `conduite-contresens`, `nitro-lignes`, `hud-nitro`, `test_conduite_libre`, `test_commandes` | OK | anomalie (2) |
| Roues, feux | les roues tournent (arrêt et roulage différents) et braquent ; feux stop au freinage (jour et nuit) ; feux de recul | `roues-*`, `feux-frein-jour/nuit`, `course-nuit-ville-stop`, `feux-recul`, `course-nuit-ville-recul`, `test_roues`, `test_feux_modele` | OK | ANO-78 encore là |
| Chocs avec le trafic, accidents | choc latéral : les voitures restent côte à côte sans s'interpénétrer ; accident : tête-à-queue, fumée, étincelles, ralenti ; reprise de la course | `choc-etincelles`, `trafic-choc`, `trafic-accident`, `accident-fumee`, `ressenti-choc`, `test_traffic`, `test_trafic_vivant` | OK | — |
| Murs, bordures | à 250 km/h, la voiture glisse le long de la bordure, redressée, sans traverser la route ni monter sur le trottoir | `braquage-droite-250-jour` (Golf contre la ligne de rive), `player.cpp` (bordure : cap × −0,25, vitesse × (1 − 2 dt)) | OK | — |
| Caméras | Poursuite, Proche, Capot, Cockpit (gauche et droite), à l'arrêt et en course | `camera-*`, `course-cam0/1-*` | OK | — |
| Qualités | Haute / Moyenne / Basse : la voiture est la même, phares allumés, et la police reste visible en Basse | `voiture-course-haute/moyenne/basse`, `qualite-*`, `poursuite-police-basse` | OK | — |

## Mesures par voiture (banc `mesure`, classes `Player` et `carPerf` du jeu, ligne droite, pas de 1/60 s)

« jeu » = mesuré dans le jeu ; « fiche » = chiffres affichés au garage (`cars.h`). Nitro : vitesse de pointe avec la nitro
tenue (jauge pleine). Le freinage 100 → 0 est le même pour toutes les voitures. IA : 0-100 d'un ordinateur de la Course à 5
(accélération constante a0 × 0,9) et vitesse qu'il vise en Normale.

| Voiture | ch | 0-100 fiche | 0-100 jeu | 0-200 jeu | Vmax fiche | Vmax jeu | Vmax nitro | Frein 100-0 | IA 0-100 | IA visée (Normale) |
|---|---|---|---|---|---|---|---|---|---|---|
| Golf 8 GTI | 245 | 6,2 s | 4,6 s | 12,2 s | 250 | 236 | 307 | 28 m | 4,7 s | 225 |
| M4 Competition | 510 | 3,9 | 2,8 | 6,6 | 290 | 277 | 360 | 28 | 3,0 | 261 |
| 911 Carrera 4S | 450 | 3,6 | 2,6 | 6,0 | 306 | 292 | 380 | 28 | 2,8 | 275 |
| Gallardo | 560 | 4,0 | 2,9 | 6,5 | 324 | 306 | 398 | 28 | 3,1 | 292 |
| 208 | 130 | 8,7 | 6,7 | — | 208 | 196 | 255 | 28 | 6,7 | 187 |
| 500 Abarth | 135 | 7,9 | 6,0 | — | 205 | 195 | 253 | 28 | 6,0 | 184 |
| T-Cross | 150 | 8,9 | 6,9 | — | 200 | 189 | 246 | 28 | 6,8 | 180 |
| Compass | 150 | 9,5 | 7,4 | — | 196 | 185 | 240 | 28 | 7,3 | 176 |
| S3 berline | 300 | 4,9 | 3,6 | 9,5 | 250 | 239 | 311 | 28 | 3,8 | 225 |
| X5 | 381 | 5,4 | 4,0 | 10,9 | 243 | 232 | 302 | 28 | 4,1 | 219 |
| Giulia QV | 510 | 3,9 | 2,8 | 6,5 | 307 | 292 | 379 | 28 | 3,0 | 276 |
| A 45 | 381 | 4,2 | 3,0 | 7,5 | 270 | 259 | 336 | 28 | 3,2 | 243 |
| Model 3 | 440 | 4,4 | 3,2 | 9,3 | 233 | 225 | 292 | 28 | 3,4 | 210 |
| Corsa | 130 | 8,7 | 6,7 | — | 208 | 196 | 255 | 28 | 6,7 | 187 |
| GT86 | 200 | 7,6 | 5,7 | 18,6 | 226 | 213 | 277 | 28 | 5,8 | 203 |
| Elantra N | 280 | 5,3 | 3,9 | 10,3 | 250 | 238 | 310 | 28 | 4,1 | 225 |
| V60 Polestar | 350 | 4,9 | 3,6 | 9,5 | 250 | 239 | 311 | 28 | 3,8 | 225 |
| RR Sport | 400 | 5,9 | 4,4 | 12,1 | 242 | 230 | 299 | 28 | 4,5 | 218 |
| Mustang GT500 | 760 | 3,6 | 2,6 | 6,1 | 290 | 278 | 361 | 28 | 2,8 | 261 |
| SF90 XX | 1030 | 2,3 | 1,6 | 3,7 | 320 | 310 | 403 | 28 | 1,8 | 288 |

Lecture : l'ordre des voitures est respecté, et elles sont bien différentes les unes des autres (0-100 de 1,6 à 7,4 s, vitesse
max de 185 à 310 km/h). Mais le 0-100 du jeu vaut **0,74 fois celui de la fiche pour toutes les voitures** (facteur 1,45 de
`carPerf`, `cars.h` l. 72), et la vitesse max fait 94 à 97 % de la fiche. 208 et Corsa sont identiques, comme leurs fiches.
Le freinage ne dépend pas de la voiture. Sans nitro, la Golf plafonne à 236 km/h (le banc `braquage` le dit aussi).

### Course à 5, place moyenne du joueur selon sa voiture (`course5sim`, 4 km, pilote automatique du jeu à fond, sans nitro, 5 graines)

| Voiture du joueur | Facile (Ville) | Normale (Ville) | Difficile (Ville) | Normale (Grand Prix) | Temps moyen (Ville) |
|---|---|---|---|---|---|
| SF90 | 1,0 | 1,0 | 1,2 | 1,2 | 51,2 s |
| M4 | 1,2 | 2,2 | 2,4 | 2,2 | 59,0 s |
| Mustang | 1,2 | 2,2 | 2,4 | 2,4 | 58,6 s |
| Golf | 2,8 | 3,0 | 3,6 | 3,0 | 68,2 s |
| X5 | 2,4 | 3,0 | 3,8 | 3,6 | 68,4 s |
| 208 | 3,8 | 4,2 | 5,0 | 4,2 | 81,8 s |
| Compass | 4,2 | 5,0 | 5,0 | 5,0 | 86,6 s |

Départ (graine 20261105 du banc, `depart`) : à 4 s, la Golf du joueur est à 89 km/h. La 911 n'est qu'à 53 km/h, bloquée
derrière le T-Cross dans la même file, et le X5 à 50 km/h, derrière la Compass. Ces deux voitures sont lancées à 11 s
seulement. C'est ce que montre `course5-course` : le pilote est 1er, et la 911 et le X5 sont derniers.

## Anomalies déjà ouvertes (`anno.md`)

| ANO | État constaté |
|---|---|
| ANO-78 (feux de recul de la Golf sur le hayon) | **encore là** (`feux-recul`, `course-nuit-ville-recul`) |
| ANO-57, ANO-59 (bandeau de la Poursuite, fins de la Course à 5 et de la Poursuite) | inchangées, en attente du test des tailles (ordre de l'utilisateur) |
| ANO-70 (tache ovale au sol derrière les voitures, la nuit) | pas visible sur `course-nuit-ville-stop` ; non tranché (pas regardé sur toutes les captures citées) |
| ANO-80 (garage-course), ANO-87 (police en Basse) | toujours corrigées |
| ANO-93, ANO-94 (duel à 4) | non revues (hors comportement des voitures) |
| ANO-75 (Model 3) | non revue (visuel, recetté ce matin) |

## Ce qui n'a pas pu être vérifié, et pourquoi

- **Course à 5, Poursuite et Duel avec une autre voiture que la Golf, en images** : les bancs `course5`, `poursuite` et
  `duel` démarrent tous avec `g.init(r, "golf", …)`, et le parcours au doigt choisit la voiture affichée au garage (la
  Golf). Le code montre que ces modes utilisent `g.car` et `g.player` (la voiture choisie), et la simulation CPU fait rouler
  les autres voitures. Mais aucune capture ne montre ces 3 modes avec une autre voiture : il faudrait toucher au code du
  banc, et c'est hors de mon rôle.
- **Poursuite bloquée près de l'arrivée** : dans l'endurance du run 6517cdf, la Poursuite est restée à **9,89 km, 0 km/h,
  pendant 148 s**, sans fin et sans « Menu ». Ce n'est pas reproduit sur 2effafa, ni sur 01fff1d (arrivée à 9,90 km,
  136 km/h). Cause probable : le pilote automatique du banc fonce sur un obstacle fixe au lieu de le contourner. Un joueur
  peut braquer ou reculer. Pas d'anomalie créée, à essayer sur le téléphone (voir plus bas).
- **Tremblements et petits sauts de la voiture** : une capture fixe ne les montre pas. Les tests CPU et les captures à
  plusieurs vitesses n'en montrent pas, mais seule une vidéo le prouverait.
- **Checkpoint sur le Grand Prix et sur la Campagne entière** : la simulation `checkpoint` s'est arrêtée à 600 s (8/20
  voitures faites sur Campagne, Grand Prix non fait). Le Grand Prix est couvert par `endurance-fin-circuit-*`,
  `terrain-rouler` et `course5sim`.
- **Pluie** : vue seulement sur la chaussée (`route-*-pluie`) et en course de nuit. On n'a pas mesuré si la pluie change
  l'adhérence : aucune règle ne le demande.
- **Sons** des voitures en course : hors de cette recette.

## À essayer sur le téléphone (APK de `main`)

1. Garage → choisir le **Compass** ou la **208** → Course à 5 en Facile : peut-on finir dans les 3 premiers ? (simulation :
   4e à 5e en moyenne)
2. Course à 5 et Duel : appuyer sur Nitro au départ, puis de nouveau plus tard. La jauge reste vide toute la course.
3. Course à 5 : au départ, regarder si une voiture rapide reste derrière une lente pendant plusieurs secondes.
4. Freiner à fond à 100 km/h avec l'Abarth, puis avec le Range Rover Sport : la distance est la même.
5. Garage : 0-100 de la Golf affiché 6,2 s, mais au compteur elle passe 100 km/h vers 4,5 s.
6. Poursuite : à l'approche de l'arrivée, contre un camion en travers ou un barrage, peut-on toujours passer, ou en
   reculant ? (le banc est resté une fois à 0 km/h à 110 m de l'arrivée)
7. Choisir la SF90, changer de terrain, puis « Rejouer » : on garde bien la SF90 et sa peinture.
8. Poursuite avec la SF90 et la nitro : la police (une 208) suit-elle à plus de 300 km/h ? (voulu : « la poursuite ne se
   perd pas », jusqu'à 377 km/h)

## Anomalies relevées

| ANO | Gravité | Preuve | Ce qui ne va pas |
|---|---|---|---|
| ANO-97 | Mineure | banc `mesure` (tableau ci-dessus) ; `cars.h` l. 72 | 0-100 du jeu = 0,74 × fiche du garage pour les 20 voitures (Golf 4,6 s pour 6,2 ; Compass 7,4 pour 9,5 ; SF90 1,6 pour 2,3) ; vitesse max à 94-97 % de la fiche |
| ANO-98 | Mineure | banc `mesure` ; `player.cpp` (freinage `Phys::brake` seul) | freinage de 100 à 0 en 28 m pour les 20 voitures : la masse (de 1 035 à 2 330 kg) et l'adhérence ne comptent pas |
| ANO-99 | Mineure | `course5-course`, banc local, `depart` | grille de la Course à 5 dans l'ordre du tirage : une voiture rapide derrière une lente, dans la même file, reste bloquée environ 10 s (911 à 53 km/h à 4 s, contre 89 km/h pour la Golf) |
| ANO-100 | Mineure | `traffic.cpp` l. 301 (seule recharge), `course.cpp` l. 58, `course5_system.cpp` | en Course à 5 et en Duel (sans trafic), la nitro du joueur ne se recharge jamais : 0,4 au départ, soit environ 1,5 s pour toute la course, alors que les ordinateurs ont 0, 1 ou 2 nitros de 2,5 s |
| ANO-101 | Mineure | `course5sim` (tableau ci-dessus) | adversaires de la Course à 5 tirés parmi les 19 autres voitures, quelle que soit celle du joueur : la SF90 gagne toujours ; avec la Compass ou la 208, on est 4e à 5e en moyenne, même en Facile |
| ANO-102 | Mineure | journaux `endurance` des runs a5faca5 (+10,8 %), 2effafa (+11,2 %), 57fac94 (+12,6 %) ; vert sur 01fff1d (+9,6 %) | CI de `main` rouge une fois sur deux : « fuite de mémoire » au cycle 6 (Corniche de nuit), alors que la mémoire redescend ensuite (+2,1 % en fin de banc) : c'est un pic de la carte, pas une fuite qui grandit |
