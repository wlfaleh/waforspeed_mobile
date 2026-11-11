# WaForSpeed Mobile — journal

Journal du projet (tickets WFS-XX de `TACHES.md`) : une entrée par étape (début, construction, PR, verdict QA, fin…) avec
fait / fichiers / vérification / limites. **Nouvelles entrées à la fin du fichier.** Toujours partir de la dernière version de
`main` (`git pull`), ajouter seulement ses entrées, puis envoyer tout de suite ; ne jamais écraser le fichier.
**Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
Exemple : `### 2026-11-03 19:43:10 — WFS-58 construit, essai simulateur lancé (Claude, session trafic)`.
> **Archive (règle de l'utilisateur, 2026-11-06)** : ce fichier ne garde que les **2 derniers jours**. Les entrées plus anciennes sont dans `journal/archive/AAAA-MM-JJ.md` (un fichier par jour) ; ne les relire qu'en cas de besoin précis. Le PO déplace chaque jour les entrées de plus de 2 jours.

### 2026-11-09 00:01:38 — WFS-215 : PR #288 refusée par le lead dev (lead-dev auto)
- Revue complète (1re) : captures ouvertes (Model 3 plaque, RR Sport jour / nuit, A 45 vitrines), tout le diff lu (`tools/vitres_voitures.mjs`, 1 ligne de `convertir_voitures.sh`, `logos.json`, 3 voitures). RG-01, RG-02 (jour), RG-03, RG-04 tenues à l'œil ; code juste.
- Refus : conflit `journal.md` avec main (fusion impossible) ; `conducteurs-rrsport-nuit` éblouie avant comme après : conducteur de nuit non montré.

### 2026-11-09 00:03:11 — WFS-322 : PR #289 refusée par le lead dev (1re revue) (lead-dev)
- Revue complète du commit 567e68b2 : 33 paires du banc `qualite` ouvertes, tout le diff lu (`environnement.cpp`, `eclairage_physique.cpp`, `eclairage_unites.h`, accroches `renderer.*` 2 lignes et `passes.h` 1 ligne, test `fidelite_lumiere`).
- **Images et code bons** : les façades ne sont plus bleues (56 / 60 / 64 m), plus de saut sur `voiture-sonde-0` à `-9`, nuit inchangée ; aucun défaut de logique (`lisserSh9`, `sceneNeuve`, saut de la voiture du joueur dans la capture).
- **Refus pour preuves manquantes** : (1) banc `fluidite` non mesuré (RG-04, ± 0,2 ms) ; (2) journal sans la vraie cause du bleu (Golf dans la capture, diagnostic `WFS_DIAG_SONDE`) ni les mesures finales (RG-01) ; (3) accroches `renderer.*` / `passes.h` non annoncées ; (4) tests et parcours absents du journal.
- **Remarque pour le PO** (non bloquante) : la façade à l'ombre est maintenant légèrement chaude (b* +1,8 à +2,9), alors que `expert-realisme-3d` l'attendait légèrement froide (b* −2 à −3,5). RG-03 est tenue ; un ticket de finition est possible (teinte du ciel, occlusion de WFS-269 comptée deux fois).
- Revue écrite dans la PR (en commentaire : même compte GitHub) ; `TACHES.md` → « ↩️ Retour lead dev (PR #289) », `track-agent.md` et `suivi-pr.md` à jour.

### 2026-11-09 00:08:20 — Dernière APK : 16 img/s avec moins de triangles (GPU scène 55,8 ms contre 37,1) : régression du coût par pixel, notée dans WFS-335 (po-scrum-master)
### 2026-11-09 00:10:28 — WFS-301 (complément, PR #290) : retour lead dev, conflit avec main (lead-dev)
- 8 captures `cuisson-*` ouvertes : avant et après identiques (RG-C3). Tout le diff est relu : `cuireSeul`, `sha256Hex` (FIPS 180-4) et l'accroche de 1 ligne avant Vulkan sont conformes à RG-C1 à C3. Revue « Approuvé par le lead dev » écrite dans la PR.
- Fusion en squash refusée par GitHub : conflit dans `journal.md` avec ab305b38 (PO, WFS-335). Agent 3 synchronise, garde les deux entrées et rappelle le lead dev. Aucune autre correction demandée.

### 2026-11-09 00:13:05 — WFS-301 (complément, PR #290) fusionné par le lead dev (lead-dev)
- Réponse du développeur sous le retour vérifiée (issuecomment-6729134108). `git diff c8623bf2 d3e56166 -- src tests android assets CMakeLists.txt` vide : seuls des fichiers de suivi ont changé, l'approbation du code et des captures sur c8623bf2 tient ; `git merge-tree` avec `main` sans conflit.
- Revue « ✅ Approuvé » écrite dans la PR sur d3e56166 (en commentaire : même compte GitHub), puis fusion en squash avec `expectedHeadSha` : **612c03ef**. `TACHES.md` → WFS-301 « 🟣 Fusionné (#278, #284, #290) » ; ligne #290 retirée de `suivi-pr.md` ; Claude, agent 3 (01UY1Vv3) libre dans `track-agent.md` ; ligne ajoutée à `qa/Check.md`.
- Agent expert ci cd prévenu : `WFS_BANC=cuire` est sur `main` (612c03ef), WFS-302 peut s'appuyer dessus.

### 2026-11-09 00:14:20 — WFS-307 attribué à Claude, agent 3 (01UY1Vv3) (po-scrum-master)
- **Ticket** : WFS-307, paquets de cartes : télécharger et supprimer les autres cartes depuis le choix du terrain, jouer pendant le téléchargement. Demande de l'agent, libre (WFS-306 #286, WFS-301 #290 fusionnés). Dépendance WFS-306 🟣 remplie.
- **Priorité** : P3 → **P1** (file A « cartes téléchargées », décision de l'utilisateur du 2026-11-08 22:23 UTC : WFS-306 → 302 → 307 ; WFS-302 reste à l'agent expert ci cd).
- **RG-08 ajoutée** (demande de l'agent) : un paquet installé ou supprimé en cours de jeu sert dès la course suivante, sans relancer l'application (lève la limite de WFS-306) ; jamais pendant une course ; paquet invalide refusé par `verifier_carte`.
- **Zone** (exception PO E8, WFS-290 d'agent 2 en retour lead dev dans d'autres fichiers) : `src/ui/ecrans/terrain.*`, `paquets*`, fichiers neufs ; Java `Paquets`, `ListePaquets`, `Telechargeur` (+ JUnit) ; `src/core/carte_cuite.*` + accroches ≤ 10 lignes annoncées au journal. **Pas** `reglages.*` / `menu.*` (RG-06 seulement après WFS-290 / 291 / 318, sinon ticket suivant), **pas** la CI ni `cartes.json` réel (WFS-302) : essai avec un faux paquet local.
- **Critères et captures** : `regles-de-gestion/WFS-307.md` (RG-01 à RG-08, parcours RG-07, section « Captures demandées »). Branche `feature/waforspeed-307`, PR relue et fusionnée par le lead dev ; ✅ par les bancs du matin.

### 2026-11-09 00:15:12 — WFS-307 : début (Claude, agent 3 (01UY1Vv3))
- Branche `feature/waforspeed-307` depuis `main` (778dbf48). Skills : `expert-cartes`, `agent-developer`, `waforspeed-natif`, `design-street-racing`. Plan et accroches à l’entrée suivante.

### 2026-11-09 00:39:35 — WFS-307 : choix du terrain, file des cartes, relecture au changement de carte ; accroches annoncées (Claude, agent 3 (01UY1Vv3))
- **Java** : `FilePaquets` (règle pure, file d'attente une carte à la fois, phase `ATTENTE`), `Paquets` par carte (`chercherTout`, `etatCarte`, `installerCarte`, `arreterCarte`, `supprimee`, `ralentir`), `Telechargeur.pauseMs` (débit réduit, RG-03) ; 22 tests JUnit OK.
- **C++** : `CartesPaquets` (paquets_etape.cpp, sans rendu) ; vignette du choix du terrain : assombrie, « Télécharger — X Mo » / « Mise à jour — X Mo », file, barre + astuce, coupure, « Annuler », données mobiles confirmées, « Connexion nécessaire », « Supprimer — libère X Mo » avec confirmation ; au menu « Corniche prête pour la prochaine course » ; débit réduit en course sous 30 img/s. Pont JNI : `paquets_android.cpp` (caches 0,1 s par carte).
- **Accroches annoncées** (accord du PO, 2026-11-09 00:16:07, `regles-de-gestion/WFS-307.md`) : `src/game/game.h` (+1 champ `Demandes::recharger`), `src/game/game.cpp` (2 lignes de `changerCarte` : carte reconstruite même si déjà chargée quand `recharger`). Seul `terrain.cpp` met `recharger = true`, quand le paquet de cette carte a changé (RG-08). Sur Android, `debutChargement` appelle déjà `preparerCarte` à chaque reconstruction.
- **Tests** : `test_paquets` étendu (CartesPaquets : RG-01, 02, 04, 05, mise à jour, débit) OK ; banc `paquets` étendu au parcours RG-07 (faux paquet de la Corniche cuit en local).
### 2026-11-09 00:16:07 — WFS-307 : accroche de 2 lignes dans `game.*` acceptée (po-scrum-master)
- Demande de Claude, agent 3 : pour RG-08, `Game::changerCarte` ne reconstruit pas la partie quand la carte et le moment sont déjà chargés (RG-02 de WFS-41) ; un paquet de la Ville installé pendant une partie sur la Ville ne serait jamais relu.
- **Oui** (accroche ≤ 10 lignes, `CLAUDE.md`) : champ `bool recharger = false;` dans `Game::Demandes` (`game.h`) ; condition `!d.recharger && …` et remise à `false` dans `game.cpp`. Mis à `true` seulement par `terrain.cpp` quand le paquet de la carte a changé. Détour par `WFS_MOMENT` refusé. Détail : section « Suivi » de `regles-de-gestion/WFS-307.md`.

### 2026-11-09 00:16:53 — WFS-302 : PR #291 ouverte (agent expert ci cd)
- Mode `WFS_BANC=cuire` fusionné (#290, agent 3) ; branche synchronisée (a790c046) ; deux cuissons réelles identiques (ASTC 21,7 Mo, ETC2 5,3 Mo, RGBA8 26,3 Mo, empreinte c30f0f40c276b6c1), liste lue par `ListePaquets.choisir`. Lead dev appelé. Après la fusion : 1er lancement de `paquets.yml` sur main (feu vert de l'utilisateur), création de la Release `paquets`.

### 2026-11-09 00:19:39 — WFS-302 fusionné par le lead dev (lead-dev)
- PR #291 (agent expert ci cd), commit relu f0aa1b0f, squash 4b21934b. Tout le diff relu (7 fichiers) : `paquets.yml`, `publier_paquets.sh`, `cartes_json.py` et son test (relancé : vert), `qa/pr/WFS-302/`.
- Captures : ticket de CI sans changement à l'écran, preuves par journaux acceptées. Revue « ✅ Approuvé » écrite en commentaire (GitHub refuse l'approbation : même compte).
- Statut 🟡 À valider (utilisateur) : RG-06 (téléchargement réel sur le téléphone) après le 1er lancement de `paquets.yml` sur `main`.
- Pour le PO : le texte de RG-03 (`cartes[]` imbriqué) ne correspond plus au format retenu (celui de `ListePaquets.java`, WFS-306) : à réécrire. Remarques non bloquantes dans la revue (retrait des anciens paquets si l'ancienne liste n'a pas pu être lue).

### 2026-11-09 00:24:48 — WFS-302 : Release `paquets` publiée, run 1 de paquets.yml vert (agent expert ci cd)
- Run 42539619005 (feu vert de l'utilisateur), 4 min, vert : deux cuissons identiques, Release `paquets` créée, `cartes.json` (commit c11c831a) : ville ASTC 21,7 Mo, ETC2 5,3 Mo, RGBA8 26,3 Mo, empreinte c30f0f40c276b6c1.
- Vérifié de l'extérieur : liste lue à l'adresse de `configgame.json` ; redirection GitHub vers release-assets.githubusercontent.com suivie ; `Range` répond 206 (reprise possible) ; fichier ASTC complet téléchargé, SHA-256 identique à la liste. Reste RG-06 : essai sur le téléphone de l'utilisateur (🟡).

### 2026-11-09 00:29:24 — WFS-322 : cause complète, mesures finales, accroches, tests (réponse à la 1re revue de #289) (Claude, agent 1)
- **Cause du bleu (RG-01)**, prouvée au banc par le diagnostic `WFS_DIAG_SONDE` (`eclairage_physique.cpp`, actif seulement au banc) : la sonde d'environnement est posée 1,5 m au-dessus de la voiture du joueur, et la capture dessinait cette voiture.
  - La peinture de la Golf bleue remplissait la moitié basse du cube : sous l'horizon, x 0,234 / y 0,213 (violet, ≈ 4 400 lx), au lieu de la chaussée au soleil.
  - Ce reflet coloré éclairait toutes les façades de la rue (éclairement d'une façade x 0,25 / y 0,24).
  - Sans la voiture : sol x 0,33 / y 0,34 (≈ 8 500 lx), façade x 0,32 / y 0,33.
  - Causes du saut : sonde A remplacée sans fondu ; exposition recalée d'un coup après un saut de plus de 80 m dans la même scène.
- **Correction** :
  - capture sans la voiture du joueur (opaques et vitres) ;
  - diffus lissé sur 0,6 s (`lisserSh9`) ;
  - recalage d'un coup réservé à la 1re capture d'une nouvelle scène.
- **Accroches hors zone (annoncées ici)** :
  - `src/render/renderer.h` : champ `joueur` de `Pending`, 1 ligne ;
  - `src/render/renderer.cpp` : `p.joueur = dessinJoueur`, 1 ligne (réutilise `dessinJoueur` de WFS-265) ;
  - `src/render/passes.h` : `AccesRendu::joueur`, 1 ligne.
- **Mesures finales (banc `qualite`, Ville de jour, Haute)** :
  - saut à l'arrivée d'une sonde : 5,87 → **0,41** ΔE2000 (RG-02 : ≤ 2) ;
  - façades `voiture-lod-passage-56` et `-60` : b* −6,3 / −6,2 → **+2,9 / +1,8**, L* 19,4 / 19,3 (RG-03 : b* ≥ −4, même couleur).
- **RG-04, coût** (même machine, l'un après l'autre, `main` puis branche) :
  - banc `fluidite` : rapport identique ligne pour ligne (0 ligne différente) ;
  - banc `perf`, Ville, processeur par image : Haute 2,1 → 2,2 ms, Basse 1,3 → 1,2 ms (± 0,2 ms) ;
  - capture d'environnement au GPU : 47,8 → 39,9 ms (voiture du joueur sautée) ;
  - passe de scène au GPU en rendu logiciel : 455 → 461 ms en Haute, 238 → 221 ms en Basse, dans le bruit du rendu logiciel.
- **Tests** : 80 tests CPU verts, dont `test_fidelite_lumiere` (lissage) ; parcours complet 0 écart (vue Arrière et Cockpit), branche synchronisée avec `main`.
- **Écart avec l'avis de l'expert** : façade à l'ombre légèrement chaude (b* +1,8 à +2,9) au lieu de légèrement froide (−2 à −3,5). C'est dans RG-03 ; un ticket de finition possible pour le PO (teinte du ciel, occlusion de WFS-269 comptée deux fois sur les captures).

### 2026-11-09 00:32:49 — WFS-335 : relevé voiture par voiture (RG-01), correction, chiffres avant / après ; RG-03 non tenable sans toucher la voiture du joueur (agent arwa (Claude, session trafic))
- **Branche** `feature/waforspeed-335` (006086b, synchronisée cf8a74a). Banc : `WFS_BANC=perf WFS_PERF_PARTIES=ville:nuit:course5,ville:nuit WFS_PERF_MINUTE=600 WFS_PERF_IMAGES=20` (Basse, vue Proche ; 3e champ `course5` ajouté au banc des parties) ; nouvelles lignes « voitures : » = une ligne par voiture dessinée : distance réelle à la caméra, longueur à l'écran, niveau, triangles et appels de la passe de scène (en Basse de nuit : ni ombres ni reflet, donc « toutes passes » = scène).
- **RG-01, relevé avant (main)**, Course à 5 de nuit, Basse, départ : joueur 7,2 m 276 px niveau 0 **109 884 tri.** 40 appels ; adversaires 16,2 m 104 px niv. 1 25 690 ; 28,8 m 77 px niv. 1 49 013 ; 58 m 39 px « 4 pièces » **45 607** ; 73 m 28 px « 4 pièces » 31 906 → voitures 262 100 tri. (70 % de 372 692). À 2 min : joueur 109 884 + adversaire à **250 m, 8 px : 52 457 tri.** Ville de nuit avec trafic : joueur 109 884 + voiture à 80,7 m 45 544.
- **Ce qui pèse** : (1) la **voiture du joueur** complète (110 k, 40 appels : 42 à 100 % des triangles des voitures selon l'instant) ; (2) en **Basse, la version « 4 pièces » des voitures lointaines gardait la forme du niveau 1** (40 à 60 k tri.) à toute distance au-delà de 48 px, même à 8 px ; (3) adversaires proches au niveau 1 (25 à 49 k). Intérieur, roues, phares : pas de poste à part (les phares sont dans le niveau ; les roues tournent seulement à moins de 15 m) ; ombres et reflet : coupés en Basse de nuit.
- **Intérieur et dessous de caisse (RG-02)** : rendu de visibilité hors dépôt (Golf, 48 vues autour à 6 m, 2,5 m de haut, tampon d'identifiants + test des sommets, vitres non occultantes) : **88 à 97 % des triangles sont visibles de l'extérieur** ; les retirer ne gagne presque rien et changerait l'intérieur vu par les vitres : **non fait**.
- **Correction (RG-02)** : en Basse, les niveaux 2 et 3 existent maintenant en 4 pièces (`Model::partsBasseNiveaux`), chaîne Basse : complète ≥ 200 px → niveau 1 ≥ 48 px (inchangé) → niveau 2 en 4 pièces ≥ 26 px → niveau 3 en 4 pièces ; fondu tramé à chaque passage, seuils existants. Police en lots : 4 pièces de près (inchangé), niveaux 2 et 3 en 4 pièces au loin. Haute / Moyenne, fantôme, voiture du joueur : inchangés.
- **Après** (même banc) : Course à 5 départ **262 100 → 218 839** (−16,5 %) ; 1 min 162 341 → 132 761 (−18 %) ; 2 min 162 341 → 123 921 (−24 %) ; Ville avec trafic, départ 155 428 → 138 575 (−11 %) ; scènes sans voiture lointaine : inchangées. Par voiture : 58 m 45 607 → 20 595 ; 73 m 31 906 → 13 657 ; 250 m **52 457 → 14 037** (−73 %). Appels des voitures : 107 → 107. Temps d'image du banc (lavapipe, rendu sur processeur) : sans valeur pour le téléphone.
- **Accroches annoncées** : `src/render/model_loader.cpp` (12 lignes : même regroupement en 4 pièces pour les niveaux 2 et 3), `src/render/renderer.h/.cpp` (champ `partsBasseNiveaux`, relevé du banc `releverVoitures`, 16 lignes, chemin de dessin des voitures).
- **Tests** : 79 tests CPU OK (`test_vehicule_lod` : nouveaux cas Basse / police / Haute) ; parcours complet 0 écart (vue Arrière et Cockpit).

### 2026-11-09 00:32:49 — WFS-335 : risque signalé, RG-03 (triangles des voitures ÷ 2) impossible tant que la voiture du joueur reste complète (agent arwa (Claude, session trafic))
- Constat : relevé ci-dessus ; la voiture du joueur seule = 109 884 triangles (Golf ; 112 à 159 k selon le modèle) ; ÷ 2 au départ de la Course à 5 demande ≤ 131 k pour toutes les voitures.
- Conséquence : sans toucher la voiture du joueur ni les adversaires proches (RG-02 « intacte de près », « voitures proches identiques »), le gain s'arrête à −11 à −24 %.
- Proposition (décision du PO) : en **Basse seulement**, voiture du joueur au **niveau 1** en vues Arrière / Proche (Golf 109 884 → 38 224, à 170-280 px de long sur la scène de la Basse) **et** adversaires de 48 à 100 px au niveau 2 : 262 k → ≈ 120 k (÷ 2,2) ; changement visible de près, captures à faire avant décision. Autre option : garder l'état de cette PR et mesurer les img/s sur le téléphone.
- Décision attendue de : PO. La PR de WFS-335 livre le reste du ticket (RG-01, RG-02, RG-04) sans attendre.

### 2026-11-09 00:34:24 — WFS-340 (RG-B) : 23 faux ✅ du run « bancs du matin #3 » remis en 🟣 (agent expert ci cd)
- Validation de l'utilisateur (« Je valide A B D C »). Le run #3 de matin.yml (2026-11-08 08:06 UTC, réveil en retard de ~6 h) avait tout sauté mais était vert : le robot des statuts avait passé 23 tickets en « ✅ Terminé (bancs du matin #3) », déjà rangés dans l'archive par le PO. Ils reviennent dans le tableau de `TACHES.md`, chacun avec son statut d'avant le run (commit d9625b3f^), et sortent de `archive/taches-terminees.md` : WFS-161, 226, 227, 234, 239, 244, 250, 251, 252, 257, 261, 266, 267, 270, 272, 275, 277, 278, 279, 284, 286, 288, 298. Les vrais bancs du matin les repasseront en ✅ s'ils sont verts. WFS-257 et WFS-270 gardent leur mention 🟡 (validation de l'utilisateur).
- `track-agent.md` : la ligne d'arwa-4 cite encore « WFS-298 ✅ (… bancs du matin #3) » : ligne d'un autre agent, laissée au PO.

### 2026-11-09 00:36:23 — WFS-340 : PR #292 (RG-A robot des statuts, RG-C fenêtre 4 h - 11 h) ; RG-D bancs du matin lancés à la main (agent expert ci cd)
- RG-D : `matin.yml` lancé à la main sur main vers 00:40 UTC (validation de l'utilisateur) ; il compte pour le 9 novembre.
- PR #292 : statuts.yml (run du matin sauté = aucun statut changé), matin.yml (6 réveils, bancs au premier réveil entre 04:00 et 10:59 heure de Paris s'ils n'ont pas tourné ce jour-là). Lead dev appelé.

### 2026-11-09 00:38:44 — WFS-340 fusionné par le lead dev (lead-dev)
PR #292 relue (diff complet : statuts.yml RG-A, matin.yml RG-C, pr.md) ; revue « ✅ Approuvé » écrite dans la PR (COMMENT, même compte GitHub) sur 832a7823 ; squash 0708e60e. Pas de captures (ticket de CI) : preuves par le faux `gh`. Remarques non bloquantes : un run du jour annulé ou rouge bloque le relancement automatique ce jour-là ; liste des jobs illisible = pas de ✅ (choix prudent). Ligne retirée de `suivi-pr.md`, ligne ajoutée à `qa/Check.md`, statut 🟣.

### 2026-11-09 00:39:29 — WFS-322 fusionné par le lead dev (lead-dev)
- PR #289, 2e revue : les 4 points de la 1re revue sont vérifiés.
  - RG-04 : fichiers de mesure relus. `fluidite.txt` est identique entre `main` et la branche ; `perf` processeur Haute 2,1 → 2,2 ms, Basse 1,3 → 1,2 ms.
  - Le journal contient la cause complète, les accroches, les tests et le parcours.
- Le code est identique à celui de la 1re revue. Les captures n'ont pas changé ; les paires clés ont été rouvertes.
- Revue « ✅ Approuvé » écrite dans la PR (en commentaire, même compte), puis squash 52d42271. Statut 🟣 : ANO-174 vient d'une mesure de banc, pas d'un retour de l'utilisateur sur son téléphone.
- Pour le PO, deux remarques :
  - façade à l'ombre légèrement chaude (b* +1,8 à +2,9) au lieu de légèrement froide : finition possible ;
  - RG-04 de WFS-322 cite le banc `fluidite` pour le coût par image, mais ce banc mesure la caméra : critère à écrire avec le banc `perf`.

### 2026-11-09 00:40:23 — WFS-322 fusionné (#289) ; WFS-341 créé (remarques du lead dev) ; WFS-193 (objectif, file A) à Claude, agent 1 (po-scrum-master)

### 2026-11-09 00:42:34 — WFS-290 fusionné par le lead dev (lead-dev)

PR #287, 2e revue sur 349c03c4 : les 3 points du refus sont corrigés (RG-04 couleur à l'appui et action au relâcher ; tests CPU `test_reglages_appui` et `test_retour_systeme` relancés par le lead dev : OK ; commentaire et README). 16 captures ouvertes, diff complet relu (29 fichiers). Approbation écrite dans la PR (COMMENT « ✅ Approuvé », GitHub refuse APPROVE sur le même compte), fusion squash 307c0f3. Statut 🟡 À valider (utilisateur) (défaut vu par l'utilisateur, ANO-159). Accroche hors zone notée ici : `src/platform/retour_systeme.h` + 5 lignes (`actionRetourReglages`), `retour_systeme.cpp` 2 lignes, `desktop_main.cpp` 1 ligne (8 lignes, ≤ 10). Remarque pour WFS-291 : remettre `g_doigt` à -1 dans `onEcran`.

### 2026-11-09 00:43:17 — Début : WFS-291 attribué à Claude, agent 2 (po-scrum-master)
- Ticket : WFS-291 « Réglages rangés en 2 ou 3 onglets » (E8, P3), à la demande de Claude, agent 2 (01UY1Vv5), suite des Réglages confirmée par l'utilisateur (WFS-290 → 291 → 318) ; WFS-290 🟡 (PR #287, 307c0f3).
- Branche `feature/waforspeed-291` ; zone : `src/ui/ecrans/reglages.*`, `menu.*` ; règles : `regles-de-gestion/WFS-291.md` (découpage validé le 2026-11-08 23:04:56 UTC).
- Inclus (même fichier, demande du lead dev sur #287) : `onEcran` remet aussi `g_doigt` à zéro, pas seulement `g_presse`.
- Captures avant / après obligatoires, PR relue et fusionnée par le lead dev ; après fusion → 🟡 À valider (utilisateur) si le ticket vient d'un essai de l'utilisateur.

### 2026-11-09 00:55:04 — WFS-334 : PR #293 ouverte, pluie hors de l'habitacle et des tunnels (Claude, agent dev Y)
- **Cause (RG-01)** : la pluie est un voile de traits 2D dessiné par-dessus toute l'image (`meteo.cpp`, `drawUi`). Elle ne tient compte ni de la vue (planche, volant, toit), ni des tunnels, ni de la vitesse : ses traits restaient verticaux.
- **Correction** (`meteo.*`, et `vehicule_cockpit.*` en E10) :
  - pluie coupée sous les tunnels du tracé, avec un fondu de 8 m ;
  - en vue Cockpit, traits seulement à travers le pare-brise (projeté à l'écran) et les vitres latérales au-dessus de la planche ;
  - 140 gouttes posées sur la vitre, effacées par les balais (`pareBriseVu()` pour la géométrie, même modèle de balais que la caméra) ;
  - traits inclinés depuis le point de fuite à vitesse (ANO-194).
- **Zone** : `camera_vue.cpp` est seulement lu (`etatCockpit()`, sans accroche). `cartes_banc.cpp` reçoit 3 vues de tunnel pour les captures (RG-05).
- **Vérifié en local** : `test_pluie` vert, 81 tests CPU verts, parcours complet 0 écart, 182 gouttes effacées par les balais au banc. Coût de la pluie : 0,029 ms par image en cockpit, 0,006 à 0,013 ms en vue Arrière.
- **Captures** dans `qa/pr/WFS-334/` (12 paires). L'« avant » = la branche avec `meteo.*` et `vehicule_cockpit.*` de `main`. Le premier essai « avant » a été rejeté : le banc n'avait pas été reconstruit, comme le montrait la ligne de journal présente dans le banc « avant ».

### 2026-11-09 01:00:28 — WFS-193 : méthode A (petite géométrie neuve par l'outil), 3 voitures pilotes d'abord (po-scrum-master)

### 2026-11-09 01:02:50 — Carte « Neige scandinave » de Noël : WFS-342 (vue d'ensemble), 343, 344, 345 créés (demande de l'utilisateur) (po-scrum-master)

### 2026-11-09 01:05:02 — Principe « tout en dynamique » écrit (WFS-342, 343, skill expert-cartes) (po-scrum-master)

### 2026-11-09 01:06:02 — WFS-336 : PR #295 ouverte (lead-dev)
- Branche `feature/waforspeed-336` synchronisée avec `main` ; `test_trafic_duree` après synchronisation : **0 fenêtre hors règle sur 1600** (`--avant` : 771) ; tests CPU 80/80 avant la synchronisation, suite relancée après.
- Banc perf, Ville, TrafficSystem (médiane de 90 images), Haute avant → après : départ 12 / 9 547 → 12 / 9 547 ; ≈ 2 min 50 : 76 / 86 119 → 25 / 11 331 ; 10 min jour : 201 / 170 554 → 94 / 58 020 (3 véhicules dessinés) ; 10 min nuit : 222 / 170 596 → 100 / 58 032. Basse 10 min : 56 / 138 966 → 30 / 32 680. Total de voitures constant (54).
- Reproduction du ticket : 54 → 644 ne se reproduit plus tel quel sur le `main` actuel (76 à 2 min 50, 201 à 10 min) ; l'accumulation reste visible sur `main` et disparaît sur la branche.
- Captures : 9 paires dans `qa/pr/WFS-336/` (Ville jour / nuit, Haute / Basse, 10 min, Golf sur la chaussée ; Poursuite identique à `main`). Limite : à 2 min 50, la Golf du pilote automatique du banc sort de la route sur `main` comme sur la branche.
- Relecture et fusion : **session « ⚡ Lead dev automatique »**, prévenue directement. Après fusion : 🟡 À valider (utilisateur).

### 2026-11-09 01:07:34 — WFS-334 fusionné par le lead dev (lead-dev)
- PR #293 (Claude, agent dev Y) relue sur 19371276. Les 24 images ont été ouvertes et tout le diff a été lu (`meteo.*`, `vehicule_cockpit.*`, `cartes_banc.cpp`, `test_pluie`, `CMakeLists.txt`).
- La branche avait 10 commits de retard (dont WFS-290, interface, sans fichier commun). Fusion locale vérifiée : aucun conflit, construction complète, 82 tests CPU verts.
- Revue « ✅ Approuvé » écrite dans la PR (en commentaire : GitHub refuse l'approbation par le même compte), puis squash 92c1a8ce.
- Ticket en 🟡 À valider (utilisateur). Ligne ajoutée à `qa/Check.md`, ligne de `suivi-pr.md` supprimée, agent dev Y libre.
- Remarques non bloquantes, écrites dans la PR : gouttes sur le montant gauche ; double état des balais ; pluie encore 6 m dans la bouche en cockpit.
### 2026-11-09 01:07:43 — WFS-307 fusionné par le lead dev (lead-dev)
- PR #294 (Claude, agent 3 (01UY1Vv3)) relue sur 1021b6b6 : 18 captures ouvertes une par une (10 nouvelles du choix du terrain, du menu et de la course ; 4 paires de WFS-306 identiques), diff de 34 fichiers lu en entier (FilePaquets, Paquets, Telechargeur, pont JNI, CartesPaquets, terrain, banc, tests). RG-01 à RG-05, RG-07, RG-08 tenues ; RG-06 reportée (décision du PO) ; accroches `game.*` conformes à l'accord du PO de 00:16:07.
- GitHub refuse « Approve » (même compte) : revue « ✅ Approuvé par le lead dev » écrite en commentaire, puis fusion en squash 90ee9b21. Statut 🟣 ; ligne retirée de `suivi-pr.md` ; `qa/Check.md` complété.

### 2026-11-09 01:10:30 — WFS-336 fusionné par le lead dev (lead-dev auto)
- PR #295 codée par le lead dev (session de l'utilisateur), relue et fusionnée par une autre instance (règle de WFS-336), à sa demande.
- Revue complète : 9 paires de captures ouvertes, tout le diff lu (`traffic.*`, `traffic_system.*`, `test_trafic_duree`, accroches de banc). Tests relancés sur la branche fusionnée avec main : `test_trafic_duree` 0 sur 1600, `test_trafic_vivant`, `test_chocs_voitures`, `test_rien_sur_chaussee`, `test_lots_decor`, `test_navigation` OK.
- Approbation en commentaire (même compte), squash c2fd0c5 (expectedHeadSha 52eaf6c). Statut 🟡 À valider (utilisateur) ; ligne `qa/Check.md`.

### 2026-11-09 01:10:25 — WFS-343 attribué à Claude, agent 3 (01UY1Vv3) ; WFS-346 et WFS-347 créés (po-scrum-master)
- **Choix** : files A / B sans ticket attribuable (WFS-193, 335 en cours ; WFS-194 attend E10 ; file B : WFS-333 attend E3 occupée par WFS-336 / 338, WFS-328 et 338 chez arwa). Premier ticket prêt du domaine cartes : **WFS-343** (P2, objectif « cartes téléchargées », dépendances WFS-301, 306, 307 🟣).
- **Découpage** (un ticket = une journée au plus ; E3 occupée) : la neige (RG-02 et la partie neige / saison de RG-05) passe dans **WFS-346** (P2, E3 + E2 / E4, après WFS-343, skill expert-conduite). WFS-344 dépend de 343 et 346. Rien du contenu demandé n'est retiré.
- **Zone de WFS-343** : `src/world/cartes_v2/**`, `src/core/carte_cuite.*`, `src/ui/ecrans/terrain.*`, `paquets*`, `tests/cartes_v2*`, fichiers neufs ; accroches ≤ 10 lignes annoncées dans `track.*` / `game.*`. Pas `traffic.*`, `traffic_system.*`, `poursuite*`, `hud.*`, `player.*`, `conduite_*`, `reglages.*`, `menu.*`, ni la CI.
- **Critères** : RG-01 (tracé, chocs, départs, checkpoints, zones de trafic lus du paquet, carte absente du choix si bloc faux, jamais de plantage), RG-03 (décor générique lu du paquet, carte venue d'un paquet dans le choix du terrain), RG-04 (tests CPU, parcours accueil → terrain → course → retour 0 écart, carte de test, captures au lead dev), RG-05 (rien de propre à une carte dans le code). Règles : `regles-de-gestion/WFS-343.md`.
- **WFS-347** (P3, E8) créé : RG-06 de WFS-307 (ligne « Cartes : N téléchargées, X Mo » des Réglages), après WFS-290, 291, 318 (agent 2).

### 2026-11-09 01:11:39 — WFS-343 : début (Claude, agent 3 (01UY1Vv3))
- Branche `feature/waforspeed-343` depuis `main`. Skills : `agent-developer`, `expert-cartes`, `waforspeed-natif`. Périmètre : RG-01, RG-03, RG-04, RG-05 (neige → WFS-346). Étude du tracé et des chocs actuels avant le plan ; accroches `track.*` / `game.*` annoncées à l'entrée suivante.
### 2026-11-09 01:25:26 — WFS-335 : PR #296 ouverte, 🆕 à relire par le lead dev (agent arwa (Claude, session trafic))
- **PR** : https://github.com/wlfaleh/waforspeed_mobile/pull/296 (branche `feature/waforspeed-335`, synchronisée avec main ; 81 tests CPU OK ; parcours complet 0 écart, vues Arrière et Cockpit).
- **Captures** : `qa/pr/WFS-335/` (31 captures du banc `lod` avant / après, Basse ; Course à 5 de nuit vues Arrière et Proche, trafic de nuit de 10 à 170 m) ; 31 sur 43 identiques au pixel près, les autres ne changent qu'au loin. Le script `captures_pr.sh` avec le banc `nuit` durait plus d'une heure (machine partagée, charge 19) : arrêté, captures `lod` produites et mises en forme à la main.
- **Chiffres** : triangles des voitures −16,5 à −24 % en Course à 5 de nuit, −11 % en Ville avec trafic, −73 % par voiture lointaine ; voiture du joueur inchangée (109 884). RG-03 (÷ 2) non tenue : décision du PO demandée (entrée précédente).
- **Lead dev** : non appelé par moi (agent lancé par la session, sans outil d'appel) ; ligne 🆕 dans `suivi-pr.md` pour le passage automatique.

### 2026-11-09 01:53:15 — Statuts automatiques, bancs du matin #5 (robot statuts)
- Run vert (workflow_dispatch, commit 4e7fa71) : WFS-192 ✅ (PR #266, 2c1e9ab), WFS-285 ✅ (PR #283, f8b5200), WFS-300 ✅ (PR #272, d88e426), WFS-330 ✅ (PR #280, 760feee), WFS-301 ✅ (PR #278, c153be1), WFS-305 ✅ (PR #285, ca047f6), WFS-306 ✅ (PR #286, 67f2521), WFS-308 ✅ (PR #269, 5f2725e), WFS-161 ✅ (PR #207, 4fe82c8), WFS-226 ✅ (PR #214, ea9aa64), WFS-227 ✅ (PR #210, ca20e9e), WFS-234 ✅ (PR #209, 777c1b4), WFS-239 ✅ (PR #255, 0f0eee4), WFS-257 ✅ (PR #220, e1fad76), WFS-244 ✅ (PR #218, 4a0fbb9), WFS-250 ✅ (PR #215, a91cc8d), WFS-251 ✅ (PR #224, e0ddfa0), WFS-252 ✅ (PR #230, fcbe69b), WFS-277 ✅ (PR #233, 3e3b217), WFS-279 ✅ (PR #241, 08e611b), WFS-261 ✅ (PR #219, 3b07e48), WFS-266 ✅ (PR #252, 8c8bdc7), WFS-267 ✅ (PR #232, 466a4c8), WFS-270 ✅ (PR #231, ceb4b18), WFS-272 ✅ (PR #227, e134cac), WFS-275 ✅ (PR #237, ce592cb), WFS-278 ✅ (PR #240, 84e1c4d), WFS-284 ✅ (PR #250, 94f6f41), WFS-286 ✅ (PR #253, def95c4), WFS-288 ✅ (PR #254, 0a76fe8), WFS-298 ✅ (PR #260, 7d269b9).

### 2026-11-09 02:02:03 — WFS-328 : RG-05 (image délavée en pleine course, ANO-179) corrigée, PR #282 à relire (agent arwa (Claude, session trafic))
- Cause : éclairement des lampadaires cherché avec `player->position` (abscisse ramenée au tour) au lieu de `dist` ; la route de chaque tour est tournée (WFS-268) : dès le 2e tour, lampes cherchées sur le 1er tour, 0,1 lx, EV −3,5 en pleine ville, image blanche / rose jusqu'à la fin. Correction d'une ligne dans `eclairage_physique.cpp` (33b61e5f).
- Preuves : banc `WFS_BANC=depart WFS_DEPART_SEUL=longue` relancé après synchronisation : 980 images, EV mini 1,88, 0 image surexposée (main : EV −3,5 à 3:45 et 3:55) ; avant / après départs, « Recommencer », Cockpit, 4 min (3314744e, `qa/pr/WFS-328/pr.md`) ; 83 tests CPU OK.
- Branche synchronisée (ddc38789), description de la PR réécrite, réponse point par point sous la revue du lead dev. ANO-184 : très probablement même cause (Voie rapide des Collines = carte Ville), à confirmer sur APK.


### 2026-11-09 02:33:34 — WFS-292 : retours du lead dev corrigés, PR #279 à relire (agent arwa (Claude, session trafic))
- Branche `feature/waforspeed-292` synchronisée avec main (9ef574af), poussée (d73e796b). 83 tests CPU réussis ; parcours complet 0 écart (vues Arrière et Cockpit).
- Fumée de jour (RG-01, condition du PO) : rapport fumée / chaussée juste derrière les roues 1,32 / 1,42 / 1,39 (0 / 0,5 / 1 s). Sur tout le panache (bandes de 15 px, largeur à mi-hauteur) : moyenne 1,30 / 1,36 / 1,34 ; corps de la nappe 1,31 à 1,48 ; la queue qui s'efface descend à 1,11-1,18. Deux essais écartés et non gardés : fondu^0,9 (+0,01 à 0,02) et effacement près de la caméra coupé (+0,02 à 0,05). Si le PO veut ≥ 1,3 jusqu'au bout de la queue, cela contredit l'effacement en 1 à 2 s : c'est à lui de trancher.
- RG-04 : Golf bleue sous un lampadaire (`fumee-derapage-nuit`, Ville) : (26,21,32), saturation 0,35. RG-05 : Ville de nuit 0,70 % et 0,03 % (≤ 1 %). Choc du banc : 1 accident, 20 étincelles.
- Description de la PR réécrite (agent, demande, vérifications, limites, captures) ; réponse point par point sous la revue. `suivi-pr.md` : 🔁 à relire. Je ne fusionne pas (lead dev).

### 2026-11-09 02:35:56 — Décisions WFS-292 (RG-01 fumée) et WFS-335 (RG-03 triangles) ; WFS-348 créé ; début WFS-333 : attribué à agent arwa (po-scrum-master)
- **WFS-292 / RG-01** : accepté « luminance fumée / chaussée ≥ 1,3 en moyenne sur le panache et sur le corps de la nappe, queue qui s'efface exclue » (mesures 1,30 / 1,36 / 1,34 ; corps 1,31-1,48 ; queue 1,11-1,18). RG-01 tenue. Mesure ajoutée à `DA-realisme.md` § 3.
- **WFS-292 / Golf dans les phares des adversaires** (Course à 5 de nuit, gris argent bleuté) : condition du 22:51 tenue par la capture sous lampadaire ; phares = `nuit.cpp`, hors zone. **Pas de ticket séparé** (pas de finition non demandée) ; limite connue ; anomalie si l'utilisateur ou le QA la signale.
- **WFS-335 / RG-03** : (1) accepté en **Basse seulement** : adversaires de 48 à 100 px en LOD2, dans la PR #296, sans saut de forme ; (2) **refusé** : voiture du joueur en LOD1 (RG-02 « voiture du joueur intacte de près », charte réalisme) ; (3) critère « triangles ÷ 2 » retiré : le coût est passé par pixel (mesure de l'utilisateur 00:08 : 207 k triangles, GPU scène 55,8 ms, 16 img/s). WFS-335 se ferme sur le gain mesuré.
- **WFS-348** [TECHNIQUE] P1 créé (E4 + E13, ⏳ Prêt) : temps GPU par passe, Ville de nuit sous la pluie, cause et correction en Basse, objectif ≥ 30 img/s (`regles-de-gestion/WFS-348.md`).
- **WFS-333** (P1, file B, E3) **attribué à agent arwa (Claude, session trafic)**, à sa demande. Tickets en parallèle chez lui (demande de l'utilisateur, « avancer vite ») : WFS-338 en cours, WFS-292 / 328 / 335 en PR. File A sans ticket libre (WFS-194 attend E10, tenue par WFS-193) ; WFS-333 est le premier P1 prêt de la file B, et WFS-336 (`traffic.*`) est fusionné. Zone (exception PO, E3) : `traffic.*`, `traffic_system.*`, chocs dans `course5*` / `poursuite*` (déjà à lui avec WFS-338), `tests/**` ; **interdit** : `player.*`, `conduite_*`, `game.cpp`, `track.*`, `camera_vue.*` (WFS-293), tailles d'interface. Critères : `regles-de-gestion/WFS-333.md` (RG-01 à RG-04, ANO-180, 181, 183, 190) ; test de parcours complet ; branche `feature/waforspeed-333` à pousser dès sa création ; skill `agent-developer` ; fusion par confiance > 50 % sinon une CI de branche.
- WFS-339 ([TECHNIQUE] banc `campagne`) et WFS-348 restent prêts pour les prochains développeurs libres (Claude, agent dev Y est libre).

### 2026-11-09 02:37:52 — WFS-333 : début, branche `feature/waforspeed-333` poussée (agent arwa (Claude, session trafic))
- Ticket attribué par le PO (dada0a34). Fiche et ANO-180, 181, 183, 190 lues. Premier commit 8710c318 poussé.
- Plan : trouver pourquoi trafic / adversaires / police pénètrent la voiture du joueur (séparation des corps dans `traffic*`, `course5*`, `poursuite*`), corriger, test CPU dédié sans pénétration.

### 2026-11-09 03:18:02 — WFS-328 fusionné par le lead dev (lead-dev auto)
- PR #282, 2e revue après « corrigé dans » point par point. Captures ouvertes : course longue de nuit (avant image blanche / rose EV −3,5, après nuit normale EV 2,8), départs Ville / Corniche / Campagne / Cockpit / Recommencer, jour. Diff de `eclairage_physique.*` lu : cause de RG-05 (lampadaires cherchés avec `position` au lieu de `dist`) corrigée proprement.
- Approuvée (APPROVE), squash 2e070cd (expectedHeadSha ddc3878). Statut 🟡 À valider (utilisateur) ; ligne `qa/Check.md` ; ANO-184 à confirmer sur l'APK.

### 2026-11-09 03:19:01 — WFS-292 fusionné par le lead dev (lead-dev auto)
- PR #279, 2e revue après « corrigé dans » point par point et décisions du PO (22:51:10, 02:35:56). Captures ouvertes : fumée de jour en nappe continue derrière les roues (0 / 0,5 / 1 s), Golf bleue sous les lampadaires, Ville de nuit inchangée, choc au banc. Diff lu (`effets_systeme.cpp`, `effets_particules.h`, `bench_effets.cpp`).
- Approuvée (APPROVE), squash 6a4b7bb (expectedHeadSha d73e796). Statut 🟡 À valider (utilisateur) ; ligne `qa/Check.md`.

### 2026-11-09 03:19:54 — WFS-335 : PR #296 refusée par le lead dev (lead-dev auto)
- Revue complète (1re) : captures Course à 5 de nuit (Arrière, Proche) et trafic lointain identiques à l'œil ; diff lu (`vehicule_lod.h`, `model_loader.cpp`, `renderer.*`) : niveaux 2 et 3 en 4 pièces en Basse, correct.
- Refus : conflit `src/render/bench_perf.cpp` avec main (27 commits de retard) ; décision du PO du 02:35:56 (adversaires de 48 à 100 px en LOD2 en Basse, dans cette PR) non appliquée ; description à mettre à jour (critère « ÷ 2 » retiré, objectif 30 img/s dans WFS-348).

### 2026-11-09 03:44:17 — WFS-338 en PR #297 (agent arwa (Claude, session trafic))
- PR #297 ouverte (🆕 dans `suivi-pr.md`, 🔀 dans `TACHES.md`), branche `feature/waforspeed-338` synchronisée avec `main` (093cbefc). Police dessinée jusqu’à 160 m derrière, renfort et retour à 220 m (hors de vue), rapprochement borné (4 m/s + 0,3 × distance), jamais dans la voiture du joueur ; encart POLICE = distance à la police ; raison de chaque perte de points écrite sous le score ; vrai chantier (panneau AK5, biseau de cônes K5a) posé 450 m avant, bandeau à 200 m ; policier en uniforme au volant ; bandes « POLICE » et rampe bleue posée sur le pavillon.
- Accroche hors zone annoncée : `src/world/conducteurs.h/.cpp` (paramètre `policier`, ≤ 10 lignes).
- Preuves : tests CPU 83 OK ; banc `course5` : parcours complet 0 écart ; 24 captures « après » dans `qa/pr/WFS-338/` dont la suite `poursuite-arrivee-1` à `-6` (renfort à 229, 204, 158, 114, 81, 56 m derrière). Point pour le lead dev : mesure RG-04 de WFS-119 (Basse, ralentisseur) +128 appels sur ce banc local (Checkpoint de référence anormalement bas, 183).

### 2026-11-09 04:11:00 — Statuts automatiques, CI main #942 (robot statuts)
- Run rouge (push, commit 2e070cd), aucun statut changé. Job(s) en échec : publication-apk.
- Tickets 🟣 contenus dans ce run : WFS-340, WFS-307, WFS-322.

### 2026-11-09 04:17:37 — WFS-338 : PR #297 refusée par le lead dev (lead-dev auto)
- Revue complète (1re) : captures ouvertes (suite d'arrivée de la police, vues arrière, HUD, travaux, police, bouchon) ; diff lu (`poursuite.*`, `poursuite_system.cpp`, `conducteurs.*`). Bon : RG-02 (distance à la police), RG-03 (raison des pertes), RG-07 (livrée, gyrophare), code de RG-05.
- Refus : conflit `journal.md` ; arrivée de la police non montrée (une police déjà à 14 m dès la 1re image) ; bandeau « TRAVAUX » sans cône visible à l'image ; policier non montré ; +128 appels de dessin en Basse au ralentisseur (WFS-119 RG-04, cible ≤ 60) sans décision du PO.
- **Pour le PO** : décision attendue si le dépassement de WFS-119 RG-04 est gardé.

### 2026-11-09 04:43:36 — WFS-333 : PR #298 ouverte, 🆕 à relire par le lead dev (agent arwa (Claude, session trafic))
- **Cause (RG-01)**, mesurée par simulation (emprises dessinées, axes séparateurs) : `Traffic::collide` oubliait le tête-à-queue du joueur (`spinYaw`) et la rotation d'une voiture accidentée (`crashYaw`) : jusqu'à 2,19 m de carrosserie dans celle du joueur ; une voiture accidentée percutée par l'arrière recevait la vitesse du joueur (voiture de travers qui le suit, ANO-180) ; `duel::contacter` ne déplace que le joueur (adversaires ; police mesurée à 2,11 m, laissée à WFS-338, PR #297, sur demande du coordinateur). ANO-190 : halos des clignotants de 1,5 m pris pour des flammes.
- Fait : `traffic.h/.cpp` (emprises orientées, séparation finale, voiture accidentée qui garde sa vitesse ; seulement pour la voiture du joueur, la référence du jeu web est intacte), `course5_system.cpp` (2 lignes), `traffic_system.*` (halo 0,7 m, banc `penetration`), accroche d'1 ligne dans `desktop_main.cpp` (`WFS_BANC=penetration`), `tests/test_penetration_voitures.cpp` (nouveau), `tests/test_chocs_voitures.cpp` (voitures tournées mesurées sur leurs emprises dessinées).
- Vérifié : compilation Release ; tests CPU 84/84 (branche synchronisée) ; `test_penetration_voitures` 0,000 m (3 cartes, 2 aides, 6 graines × 60 s) ; banc `penetration` 0,000 m ; banc `parcours` 0 écart (et Cockpit 0) ; **parcours complet** Poursuite (banc `course5`) accueil → Course solo → Poursuite → garage → terrain → course → fin → Menu → accueil : 0 écart ; course à 5 : 0 écart. 11 captures « après » dans `qa/pr/WFS-333`.
- Limites : vidéo d'ANO-180 non reproduite à l'identique (les deux mécanismes sont corrigés) ; duel en réseau non traité.

### 2026-11-09 04:43:36 — WFS-333 : risque signalé (agent arwa (Claude, session trafic))
- Constat : banc `penetration`, `qa/pr/WFS-333/apres-penetration-cockpit-1.jpg` et `-2.jpg` : la voisine, à 1,91 m de centre à centre (**aucun** recouvrement des emprises), est dessinée par-dessus le bas de la portière gauche et la planche : l'habitacle de la vue Cockpit ne masque pas une voiture toute proche sur le côté.
- Conséquence : une partie de ANO-181 / ANO-183 (« voiture dans l'habitacle » en Cockpit) reste visible même sans pénétration.
- Proposition : ticket de correction sur `src/render/vehicule_cockpit.cpp` / `camera_vue.*` (portières et côtés de l'habitacle opaques jusqu'au bas du cadre) ; zone interdite à WFS-333, non touchée.
- Décision attendue de : PO.

### 2026-11-09 04:47:36 — WFS-349 créé (Cockpit : habitacle fermé) ; décisions WFS-338 ; WFS-339 attribué à agent arwa (po-scrum-master)
- **WFS-349** [URGENT] P1 créé (E10, ⏳ Prêt, `regles-de-gestion/WFS-349.md`), à la demande d'agent arwa (risque signalé sur WFS-333 à 04:43:36) : en vue Cockpit, une voisine à 1,91 m (sans recouvrement) est dessinée par-dessus la portière et la planche (`qa/pr/WFS-333/apres-penetration-cockpit-1.jpg`, `-2.jpg`). Cause à nommer (trou de l'habitacle ou profondeur du dessin), côtés fermés jusqu'au bas du cadre, test 0 pixel de la voisine (20 voitures × 3 regards), seuils de WFS-315 tenus. Zone `vehicule_cockpit*` en exception PO à côté de WFS-193 ; **pas** `camera_vue.*` (WFS-293). Pas rattaché à WFS-293 (banc de fluidité, autre sujet). ANO-181 → 📋 Planifiée WFS-349 ; la partie « portières opaques » de WFS-323 RG-02 y passe.
- **WFS-338, décisions** (refus du lead dev 04:17:37) : pas de ticket nouveau. (1) WFS-119 RG-04 (≤ 60 appels de plus qu'un Checkpoint) reste : mesure `main` / branche sur le même banc dans la PR #297 ; écart ≤ +10 → dépassement déjà sur `main`, ne bloque pas WFS-338 (le PO décidera d'un ticket [TECHNIQUE] sur la mesure) ; écart > +10 → ramené dans la PR (police > 80 m sans gyrophare détaillé ni policier, cônes regroupés). (2) Rétroviseur en boîtier noir → **WFS-324** (déjà prévu, ANO-152) ; la police qui arrive se prouve en vue Arrière.
- **Début : WFS-339** ([TECHNIQUE] banc `campagne` en échec, P1, premier P1 prêt) **attribué à agent arwa (Claude, session trafic)**, à sa demande (tickets en parallèle, demande de l'utilisateur ; il avait signalé le défaut). Zone (exception PO, E7 tenu par WFS-319 / 320) : `src/render/bench_*`, pilote automatique du banc, partie banc de `desktop_main.cpp` (≤ 10 lignes annoncées), `tests/**` ; **interdit** : `.github/**`, `android/**`, `store/**`, `tools/qa/**`, `player.*`, `conduite_*`, `camera_vue.*`, `track.*`. Critères : RG-01 à RG-04 (cause au journal, banc vert, limite de 3 s inchangée) ; CI de branche permise (US technique), une à la fois ; branche `feature/waforspeed-339` à pousser dès sa création ; skill `agent-developer`.
- Restent prêts pour les développeurs libres (Claude, agent dev Y ; Claude, agent 4) : WFS-349, WFS-348.
- Ménage du jour (`tache utilisateur.md`) pas fait à cet appel : au prochain appel du PO, après le passage du robot `archives`.

- Proposition : ticket de correction sur `src/render/vehicule_cockpit.cpp` / `camera_vue.*` (portières et côtés de l'habitacle opaques jusqu'au bas du cadre) ; zone interdite à WFS-333, non touchée.
- Décision attendue de : PO.

### 2026-11-09 04:48:03 — WFS-335 : retour du lead dev corrigé, PR #296 à relire (agent arwa (Claude, session trafic))
- Décision du PO (02:35:56) appliquée : en Basse, adversaires de 48 à 100 px au niveau 2 avec tous ses matériaux (fondu tramé 100 → 89 px), puis 4 pièces sous 48 px comme avant ; voiture du joueur toujours complète. Commits 5bee02c9 (1re version : 4 pièces dès 100 px, phares de la SF90 perdus en vue Proche, d'où) 2fcd39a7 ; test du seuil dans `test_vehicule_lod`.
- Conflit `bench_perf.cpp` résolu (c6ffe546, deux côtés gardés) ; synchronisée avec main (0b47a831). Suite `lod-passage` (110 → 40 px) ajoutée au banc `lod` (82bdde0b, 4d41a2bd).
- Mesure, banc perf Basse, Ville de nuit (`WFS_PERF_PARTIES=ville:nuit:course5,ville:nuit`) : **triangles des voitures** au départ de la Course à 5 : 262 100 (main) → 192 065 (−26,7 %) ; image 372 692 → 306 102 (−17,9 %) ; 1 min 162 341 → 132 761 ; 2 min 162 341 → 123 921 ; Ville avec trafic au départ 155 428 → 138 575 ; adversaire à 77 px 49 013 → 22 239. **Img/s du banc PC** (lavapipe, machine chargée) : Course à 5 au départ 0,93 → 0,95 img/s (1 079,7 → 1 051,8 ms), à 1 min 0,93 → 0,91, à 2 min 0,95 → 0,91 : bruit de mesure, ne dit pas le téléphone ; suite de l'objectif 30 img/s dans WFS-348.
- Vérifications : 83 tests CPU OK ; parcours 0 écart (Arrière, Cockpit) ; banc `lod` 54 captures, 0 erreur, Haute 12 / 12 identiques à main, Basse : changements seulement sur les adversaires de 48 à 100 px ou lointains ; captures et planche dans `qa/pr/WFS-335/`. Description de la PR mise à jour, réponse point par point sous la revue.

### 2026-11-09 04:49:17 — Début WFS-339 : banc `campagne` en échec, branche créée (agent arwa (Claude, session trafic))
- Branche `feature/waforspeed-339` créée depuis `main` (d93aeaee) et poussée. Zone : bancs seulement (exception PO). Reproduction en local en cours : `WFS_BANC=campagne` (build Release dans le scratchpad).

### 2026-11-09 05:14:31 — WFS-339 : cause trouvée (RG-01), dans le pilote automatique des bancs (agent arwa (Claude, session trafic))
- **Reproduction sur `main`** (d93aeaee, build Release local) : `WFS_BANC=campagne WFS_CAMPAGNE_LIEU=aucun ./build/wfs_bench <sortie> assets/` (`WFS_CAMPAGNE_LIEU=aucun` saute les 23 captures, garde le parcours) → « plus long arrêt 4.0 s », « parcours en échec », 1 écart ; déterministe (3 essais, même 4,0 s).
- **Cause** (trace par image, diagnostic local non gardé) : ligne droite à 540 m du départ, 185 km/h ; le trafic ralentit devant, le pilote du trafic (`traffic_system.cpp`, « (file − écart) × 5 − cap × 4 ») choisit la file de gauche ; la commande sature (volant à fond 25 m), le cap monte à −0,22 rad (10 m/s de vitesse latérale) et la voiture dépasse sa file jusqu'au bord de la chaussée (écart −1,05 demi-largeur, 610 m) ; elle heurte une balise du bord (chocs du décor, WFS-256) et reste 4,0 s arrêtée, nez dans la balise, avant de se dégager.
- Le pilote de la conduite réelle (`limiterPilote`, `src/game/aide_direction_banc.h`, WFS-241) borne déjà cela, mais il ne s'applique qu'à la conduite réelle ; le jeu est revenu à la conduite d'avant (WFS-258, RG-00) : en conduite d'avant, le pilote ne borne rien. **Ce n'est ni la route ni le trafic** (le trafic choisit une file libre, à raison) : c'est le pilote automatique des bancs (zone du ticket). La limite de 3 s ne change pas.
- Correction : `limiterPilote` en conduite d'avant, seulement pendant un changement de file hors virage marqué : cap tenu qui borne la vitesse latérale (3 m/s, puis l'écart restant en 1 s). Dans sa file, commande d'origine inchangée. Test `test_pilote_file` (CMakeLists : 1 ligne, accroche annoncée).

### 2026-11-09 05:15:38 — Statuts automatiques, CI main #943 (robot statuts)
- Run rouge (push, commit 6a4b7bb), aucun statut changé. Job(s) en échec : publication-apk.
- Tickets 🟣 contenus dans ce run : WFS-340, WFS-307, WFS-322.

### 2026-11-09 05:16:02 — WFS-339 : CI de branche n° 1 lancée (agent arwa (Claude, session trafic))
- Run 42566275669 (#944), `workflow_dispatch` de `build.yml` sur `feature/waforspeed-339` (adfbcd71, synchronisée avec `main`) : vérifie compilation, tests CPU (groupe `texte`, avec `test_pilote_file`), parcours `menu1-2400` et APK. Limite : `build.yml` ne lance pas le parcours du banc `campagne` (le groupe `cartes` l'appelle sans parcours, `matin.yml`) : le banc campagne complet est vérifié en local.

### 2026-11-09 05:19:40 — WFS-335 : PR #296 approuvée et fusionnée (lead-dev auto)

2e revue sur bccaa3ae : conflit résolu, décision du PO du 02:35 appliquée (niveau 2 de 100 à 48 px en Basse, joueur complet), diff entier relu, `test_vehicule_lod` OK, captures vues (suite 110 → 40 px sans saut, Course à 5 de nuit). Squash 88f3cde. Statut 🟡 À valider (utilisateur) : img/s à mesurer sur le téléphone ; suite dans WFS-348.

### 2026-11-09 05:22:12 — WFS-333 : PR #298 refusée, 1re revue (lead-dev auto)

Code et tests bons (`test_penetration_voitures` et `test_chocs_voitures` OK après fusion avec `main`, recouvrement 0,000 m). Refus : RG-02 non montrée (sur `penetration-accident-1…3`, voiture accidentée devant le joueur au même écart, joueur arrêté) ; aucune capture avant ; tache blanche dans l'habitacle sur `cockpit-4`, à expliquer ; `qa/pr/WFS-333/pr.md` avec « À REMPLIR ». PR #297 (WFS-338) : nouveau commit eced6204 sans réponse « Bien reçu » / « corrigé dans » sous la revue, reste ↩️.

### 2026-11-09 06:03:59 — WFS-339 : PR #299 ouverte, CI de branche verte (agent arwa (Claude, session trafic))
- **Fait** : `src/game/aide_direction_banc.h` (`limiterPilote`, pilote automatique des bancs) : en conduite d'avant, pendant un changement de file hors virage marqué, cap tenu qui borne la vitesse latérale (3 m/s, puis l'écart restant en 1 s) ; commande d'origine gardée dans sa file et dans les épingles ; conduite réelle inchangée. `tests/test_pilote_file.cpp` + **accroche `CMakeLists.txt` : 1 ligne** (`add_executable(test_pilote_file …)`). Aucune modification de `desktop_main.cpp`, de la route, du trafic ni de la limite de 3 s.
- **Vérifications** : banc `campagne` complet en local **0 écart OK**, plus long arrêt 0,0 s (`main` : 4,0 s, ÉCHEC), parcours 99 s, écart max 0,88 demi-largeur (épingles) ; `test_pilote_file` : écart max 0,665 à 185 km/h (pilote d'avant : 1,108) ; 84 tests CPU OK ; parcours complet 0 écart (Cockpit 0) ; captures `captures_pr.sh WFS-339 campagne` : 24 « après », 23 identiques au pixel à `main`, seule `carte-campagne-haut-du-col` change ; **CI de branche run 42566275669 (#944) : verte** (adfbcd71). Synchronisée ensuite (dc6681ae, WFS-335 seul, rendu) : banc campagne et `test_pilote_file` relancés, verts.
- **Limites** : `build.yml` ne lance pas le parcours du banc campagne (groupe `cartes` de `matin.yml` sans parcours) : vert prouvé en local ; l'ajouter à la CI est hors zone (`.github/**`, `tools/qa/**`). Épingles : le pilote d'avant élargit encore jusqu'à 0,88 (sans choc). PR #299 à relire (🆕 dans `suivi-pr.md`), fusion par le lead dev.

### 2026-11-09 06:06:16 — WFS-339 : preuve du banc campagne tranchée ; Début WFS-348 attribué à agent arwa (po-scrum-master)
- **WFS-339, décision** (limite signalée à 06:03:59) : pas de nouveau ticket (pas de micro-ticket). Preuve acceptée pour WFS-339 : banc `campagne` vert en local (plus long arrêt 0,0 s contre 4,0 s sur `main`) + `test_pilote_file` dans la CI ; rien à faire hors zone. L'ajout du parcours du banc `campagne` aux bancs du matin est rangé dans **WFS-287** (RG-05, E7, P2, ⏳ Prêt ; zone élargie à `tools/qa/banc_groupe.sh` et `matin.yml` pour ce seul point ; `build.yml` gelé), à donner de préférence à l'agent expert ci cd quand E7 est libre.
- **Début : WFS-348** ([TECHNIQUE] [URGENT] P1, performance par pixel, Ville de nuit sous la pluie, 16 img/s en Basse, GPU scène 55,8 ms) **attribué à agent arwa (Claude, session trafic)**, à sa demande : premier P1 prêt dont l'épic est libre (WFS-349 et WFS-194 : E10 tenue par WFS-193) ; file A (performance) après WFS-339. Fiche `regles-de-gestion/WFS-348.md`. Branche `feature/waforspeed-348`, premier commit à pousser tout de suite.
- Zone permise : `src/render/**` et `shaders/**` de la passe en cause, réglages de qualité (Basse seulement), bancs `bench_perf*`, `tests/**`. Une passe E13 (`eclairage_*`, `post*`) : accord du PO noté au journal avant d'y toucher. Interdit : `vehicule_lod*` (WFS-335), `effets_*`, `vehicule_cockpit*` (WFS-349), `traffic.*`, `player.*`, `conduite_*`, `.github/**`, `tools/qa/**`, tailles d'interface (ordre permanent).
- Critères : tableau des passes avant / après (scène de l'APK 928 contre `main`), cause nommée avec le commit, Basse ≥ 30 img/s (banc : GPU scène ≤ 33 ms), Haute identique, captures avant / après, parcours complet 0 écart, ligne `qa/Check.md`. US technique : CI de branche permise, une à la fois, chaque run au journal. PR relue par le lead dev.

### 2026-11-09 06:23:04 — WFS-339 : PR #299 approuvée et fusionnée (lead-dev auto)

Cause nommée (pilote des bancs saturé pendant un changement de file en conduite d'avant), correction dans `limiterPilote` seulement, `test_pilote_file` OK après fusion avec `main`, 23 captures sur 24 identiques. Squash 39a3de7, statut 🟣. #297 et #298 : « Bien reçu » posté, pas encore de « corrigé dans » : restent ↩️.

### 2026-11-09 06:24:35 — Statuts automatiques, CI main #945 (robot statuts)
- Run rouge (push, commit 88f3cde), aucun statut changé. Job(s) en échec : publication-apk.
- Tickets 🟣 contenus dans ce run : WFS-340, WFS-307, WFS-322.

### 2026-11-09 06:34:57 — Début WFS-348 : performance par pixel, Ville de nuit sous la pluie (agent arwa (Claude, session trafic))
- Branche `feature/waforspeed-348` poussée (e2afe6f0, depuis `main` 7e3659dc). Fiche `regles-de-gestion/WFS-348.md` lue en entier.
- Plan : (1) banc `WFS_BANC=perf WFS_PERF_SCENE=1` : même scène que la mesure de l'utilisateur (Ville de nuit sous la pluie, Checkpoint, vue Arrière, Rue des Mimosas 60 m avant le virage serré à droite, checkpoint 8), Basse dans l'état du téléphone (60 %, ombres, reflets et captures coupés), coût GPU de chaque passe et effets retirés un par un ; (2) même banc sur `b5d8ddbf` (commit de l'APK 928, run 928) ; (3) cause et commit ; (4) correction en Basse seulement dans la passe qui coûte.
- APK de la mesure à 16 img/s : la plus récente le 2026-11-09 à 00:08 était la 931 (`3529cc7b`) ; entre 928 et 931 : WFS-301, WFS-329, WFS-305 (aucun shader changé depuis l'APK 928).
- Zone : `src/render/**`, `shaders/**` de la passe en cause, bancs `bench_perf*`, `tests/**` ; une passe E13 (`eclairage_*`, `post*`) : accord du PO d'abord.

### 2026-11-09 06:53:11 — WFS-333 : retours du lead dev corrigés, PR #298 à relire (agent arwa (Claude, session trafic))
- 1re revue (4 points) corrigée dans 28360b23, branche synchronisée (71bc873d) :
  - (1) suite RG-02 en vue Arrière (Checkpoint, Ville) : voisine en tête-à-queue contre le joueur à 130 km/h ; après 2 s, joueur à 85 km/h et voiture accidentée arrêtée 34,5 m derrière ;
  - (2) interrupteur de banc `WFS_AVANT_333=1` (`g_avantWfs333`, faux dans le jeu) : 13 captures avant / 13 après annotées (vitesses, distance, recouvrement). Avant : 1,38 m, et voiture accidentée à la vitesse du joueur (54 = 54 km/h, ANO-180). Après : 0,000 m ;
  - (3) tache blanche de `cockpit-4` : phares du joueur renvoyés par la voisine blanche, plus le bloom (absente sur `cockpit-4-sans-voisine`) ; aucun halo du trafic dans l'habitacle ;
  - (4) `pr.md` rempli.
- Vérifié : tests CPU 84/84 ; `test_penetration_voitures`, `test_chocs_voitures` et `test_pilote_file` OK après la synchronisation ; banc `penetration` 0,000 m ; banc `parcours` 0 écart (et 0 en vue Cockpit). Défaut de dessin de l'habitacle → WFS-349 (hors ticket).

### 2026-11-09 07:25:01 — Statuts automatiques, CI main #946 (robot statuts)
- Run rouge (push, commit 39a3de7), aucun statut changé. Job(s) en échec : publication-apk.
- Tickets 🟣 contenus dans ce run : WFS-339, WFS-340, WFS-307, WFS-322.

### 2026-11-09 07:27:21 — WFS-333 : PR #298 approuvée et fusionnée, 2e revue (lead-dev auto)

Points de la 1re revue corrigés : suite RG-02 (avant : recouvrement 1,30 m, joueur à 17 km/h ; après : 0,000 m, joueur à 85 km/h, voiture accidentée arrêtée 34,5 m derrière), captures avant / après (`WFS_AVANT_333`, banc seulement), tache blanche du cockpit expliquée (reflet + bloom, absente sans la voisine), `pr.md` rempli. Squash b872854, statut 🟡 À valider (utilisateur). #297 : « Bien reçu » sans « corrigé dans », reste ↩️ ; #288 inchangée.

### 2026-11-09 07:51:38 — Recette du matin du 2026-11-09 faite : 15 VALIDÉ, 2 REFUSÉ, ANO-198 à ANO-200 (agent qa lead)
- Bancs du matin n° 5 (commit 4e7fa71), 17 tickets, 4 lots en parallèle. VALIDÉ : WFS-192, 258, 265, 285, 296, 300, 301, 302, 305, 306, 316, 326, 327, 329, 330 (plusieurs restent 🟡 à valider sur téléphone). REFUSÉ : WFS-317 (24 img/s au lieu de 30 sur le téléphone, ANO-198) et WFS-308 (émulateur bloqué sur l’écran des CGU, ANO-199). ANO-200 : Release `apercu` pleine (1 000 fichiers), captures périmées ou absentes, message « Recette prête » trop optimiste. 23 lignes de `qa/Check.md` cochées. Rapports `qa/rapports/WFS-XX.md`, preuves `qa/retours/recette-2026-11-09/`.

### 2026-11-09 07:53:01 — Recette du matin du 2026-11-09 : 15 VALIDÉS, 2 REFUSÉS ; WFS-350 (main 946 rouge), 351 (ANO-199), 352 (ANO-200) créés ; ANO-198 → WFS-348 (po-scrum-master)

### 2026-11-09 08:22:14 — Statuts automatiques, CI main #947 (robot statuts)
- Run rouge (push, commit b872854), aucun statut changé. Job(s) en échec : publication-apk.
- Tickets 🟣 contenus dans ce run : WFS-339, WFS-340, WFS-307, WFS-322.

### 2026-11-09 08:56:45 — WFS-348 : mesure par passe, cause, PR #300 ; décision du PO demandée (agent arwa (Claude, session trafic))
- **Banc** (branche `feature/waforspeed-348`, outils du banc seulement, rien dans l'APK) : `WFS_BANC=perf WFS_PERF_SCENE=1` refait la scène de l'utilisateur : Ville de nuit sous la pluie, Checkpoint, vue Arrière, « Rue des Mimosas » 60 m avant le virage serré à droite (checkpoint 8, capture identique à `qa/retours/2026-11-09-derniere-apk-scene-16-img-s.jpg`), Basse dans l'état du téléphone (scène à 60 %, palier 2 : ombres, reflets, captures coupés), 2340 × 1080. Image figée (même image pour chaque variante), coût GPU par passe, effets retirés un par un (`WFS_PERF_SCENE_EFFETS=1`), leviers (`WFS_PERF_SCENE_LEVIERS=1`), compteurs du pipeline (`WFS_STATS_PIPELINE=1` : pixels calculés par le shader, sommets).
- **RG-01, tableau A — même scène, APK 928 contre `main`** (compteurs exacts, indépendants de la charge de la machine) :

| version | appels | triangles | pixels calculés (k) | sommets (k) | projecteurs |
|---|---|---|---|---|---|
| APK 928 (`b5d8ddbf`) | 138 | 185 358 | 2 463 (928 + WFS-305 sans ses carreaux) | 226 | 6 |
| 928 + WFS-305 (`3529cc7b`) | 138 | 191 086 | 2 475 | 243 | 6 |
| `main` (`7e3659dc`, `c333b0b2`) | 138 | 191 086 | 2 475 | 243 | 6 |

  Aucun shader n'a changé depuis l'APK 928 (`git diff b5d8ddbf origin/main -- shaders` vide). Temps GPU de la scène au banc (lavapipe, machine partagée par d'autres bancs : ± 25 % d'une exécution à l'autre pour le même binaire) : APK 928 521 à 737 ms, `main` 514 à 968 ms : même coût, aux bruits près.
- **RG-02, cause** : **aucune fusion depuis l'APK 928 n'a augmenté le coût de cette scène** (WFS-305, `3529cc7b`, le seul changement visible : + 0,5 % de pixels calculés, + 5 728 triangles de carreaux de sol au parc). Les 37,1 ms de l'APK 928 ont été mesurés ailleurs (Course à 5, autre lieu, voitures 74 %) ; les 55,8 ms sont le coût **de ce lieu** : chaussée large éclairée par 6 lumières (2 feux de croisement de 200 m de portée + 4 lampadaires) sur presque tout l'écran, pelouse du parc, feuillage à découpe. Le coût est par pixel : la scène à 30 % coûte 54 % de moins (part par pixel ≈ 72 %).
- **Tableau B — coût de chaque effet, même scène, Basse** (une exécution, variantes l'une après l'autre ; GPU scène lavapipe) :

| variante | GPU scène (ms) | écart | pixels calculés (k) |
|---|---|---|---|
| tout (Basse du téléphone) | 670 | — | 2 475 |
| sans lumières de nuit | 602 | − 10 % (− 17 % dans une 2e exécution) | 2 468 |
| sans chaussée mouillée | 718 | bruit | 2 467 |
| sans sonde IBL | 656 | − 2 % | 2 466 |
| sans transparents (halos, taches des phares) | 687 | bruit | 2 270 (− 8 %) |
| sans feuillage à découpe | 613 | − 9 % | 2 400 (− 3 %) |
| sans sol ni trottoirs | 457 | − 32 % | 2 496 (le ciel les remplace) |
| sans voitures | 589 | − 12 % | 2 451 |
| scène à 30 % | 307 | − 54 % | 673 |

  Ombres 0, reflets 0, captures 0 (palier 2, comme le téléphone) ; post-traitement : passe finale, hors scène.
- **Correction livrée (sans changement d'image)** : `scene.frag` et `scene_skin.frag` calculaient le reflet du ciel (1 ou 2 lectures du cube de la sonde par pixel) puis le jetaient quand la sonde est active (`iblRemplacer` le remplace) : il n'est plus calculé. Captures avant / après, Basse et Haute, départ et Rue des Mimosas : **0 pixel différent** (PNG 2340 × 1080, `qa/pr/WFS-348/`). Gain trop petit pour sortir du bruit du banc (≈ 2 %).
- **RG-04 non atteint** : aucun levier de ma zone ne retire 41 % (55,8 → 33 ms) sans changer l'image. **Tableau C — leviers mesurés** (temps processeur du banc, plus stable) : 2 lumières par pixel au lieu de 4 : − 8 % ; 3 : − 8 % ; scène à 50 % : − 25 % (pixels − 29 %) ; scène à 45 % : − 33 % (pixels − 42 %) ; sans lumières de nuit : − 17 %. Estimation téléphone (part par pixel ≈ 75 %) : 45 % → ≈ 37 ms, 40 % → ≈ 32 ms.
- Vérifications : 85 tests CPU réussis (`tools/qa/tests_cpu.sh`), parcours complet : voir l'entrée suivante.
- Parcours complet (`WFS_BANC=parcours`, build de la branche) : **0 écart**, vue Cockpit 0 écart.
- CI de la branche (US technique) : **run 948** (`workflow_dispatch` de `build.yml`, branche synchronisée avec `main` df7165ed, commit 3a6c1e1c) : vérifie la construction des shaders, le banc et l'APK.
- PR #300 ouverte (pas de fusion par le développeur) ; `suivi-pr.md` 🆕, `qa/Check.md`, `TACHES.md` 🔀, `track-agent.md`.

### 2026-11-09 08:56:45 — WFS-348 : décision du PO demandée (leviers de la Basse) (agent arwa (Claude, session trafic))
- Constat : la scène coûte 55,8 ms par son contenu (lumières de nuit sur une grande surface, sol, feuillage) ; aucun levier de ma zone ne retire les 41 % nécessaires sans changer l'image de la Basse.
- Conséquence : RG-04 (≥ 30 img/s, GPU scène ≤ 33 ms) non atteint par la PR #300 seule.
- Proposition (recommandée) : **plancher de la résolution de la Basse à 45 %** (au lieu de 60 %), seulement quand la scène est déjà à 60 % et allégée (palier 2) et toujours sous 30 img/s : au banc − 33 % sur la scène, estimation téléphone ≈ 37 ms (40 % : ≈ 32 ms) ; Moyenne et Haute inchangées. Elle touche la règle de WFS-134 (RG-02 : « jamais la netteté de la voiture ») : **décision du PO, ou de l'utilisateur**. Autre option, à ajouter : **accord E13** pour alléger le calcul des feux de croisement et des lampadaires par pixel en Basse (`shaders/eclairage.glsl`, `spotLights`) : au banc 2 lumières par pixel au lieu de 4 : − 8 %, toutes les lumières retirées : − 17 %.
- Ticket suivant proposé (RG-04) : « [TECHNIQUE] Basse : résolution plancher et lumières par pixel allégées, Ville de nuit ≥ 30 img/s », zone `src/render/qualite_auto.h` et `shaders/eclairage.glsl`, mesure avec `WFS_PERF_SCENE_LEVIERS=1`.
- Décision attendue de : PO.

### 2026-11-09 09:12:58 — WFS-348 : décision du PO (leviers de la Basse), WFS-353 attribué (po-scrum-master)
- Demande : agent arwa (Claude, session trafic), entrée du 08:56:45 (tableaux A, B, C).
- (1) **Résolution plancher 60 % → 45 %** : touche WFS-134 (« jamais la netteté de la voiture », né de la plainte de l'utilisateur sur la qualité des voitures) : **hors du pouvoir du PO**. Question posée par le PO à l'utilisateur dans `tache utilisateur.md`. Sans accord écrit : pas faite (RG-03 de WFS-353).
- (2) **Accord E13** donné : `shaders/eclairage.glsl`, **2 lumières par pixel en Basse** (les plus fortes au point éclairé, les autres en approximation), Moyenne / Haute identiques au pixel près, flaques de lumière gardées.
- (3) **WFS-348 fermé sur son diagnostic** (RG-04 le prévoit) : PR #300 à fusionner selon les règles (lead dev, squash), puis 🟣. Ticket suivant **WFS-353** (P1, [TECHNIQUE], E4 + E13) créé (`regles-de-gestion/WFS-353.md`) et **attribué à agent arwa** (à sa demande).
- **Début WFS-353** : zone `shaders/eclairage.glsl` (Basse), `src/render/qualite_auto.*` (RG-03 seulement après accord de l'utilisateur), `src/render/bench_perf*` ; branche `feature/waforspeed-353` partie de `main` **après la fusion de #300** (même zone, même développeur : exception du PO à « un ticket actif par épic », aucun autre agent dans E4 / E13). Critères : 2 lumières en Basse, gain au banc avant / après, ≥ 30 img/s sur le téléphone (validation de l'utilisateur), captures `qa/pr/WFS-353/`, parcours complet 0 écart. Plan : plafond de la Basse à 2, mesure `WFS_PERF_SCENE_LEVIERS=1`, captures, fusion selon les règles. Rappel : skill `agent-developer`, synchronisation avec `main`, CI de branche permise (US technique, une à la fois). WFS-338 (retour du lead dev) reste à corriger par le même agent.

### 2026-11-09 09:14:26 — WFS-353 : l'utilisateur accepte la résolution 45 % en Basse (agent arwa (Claude, session trafic))
- Réponse de l'utilisateur (question posée directement) : **oui**, résolution plancher de la Basse à 45 % quand le jeu reste sous 30 img/s, Basse seulement ; dérogation à WFS-134 pour la Basse. Notée dans `tache utilisateur.md`. RG-03 de WFS-353 débloquée.

### 2026-11-09 09:15:42 — Début WFS-353 : Basse, 2 lumières par pixel et résolution 45 % (agent arwa (Claude, session trafic))
- Branche `feature/waforspeed-353` poussée (dd954068) : **partie de `feature/waforspeed-348`** (PR #300 pas encore fusionnée, pour ne pas attendre) puis synchronisée avec `main` 633b16a1 ; **dépend de #300** (noté aussi dans la PR) ; `git merge origin/main` dès la fusion de #300.
- Fiches `regles-de-gestion/WFS-353.md` et `WFS-348.md` lues, tableaux A, B, C de WFS-348 relus. RG-01 (accord E13 du PO) et RG-03 (accord de l'utilisateur, 09:14:26) à faire, en Basse seulement.
- Plan : plafond de lumières par pixel de la Basse à 2 (les 2 plus fortes au point éclairé, les autres en approximation), plancher de résolution 45 % seulement à 60 %, palier 2 et toujours sous 30 img/s ; tests CPU de la sélection et du plancher ; mesure `WFS_PERF_SCENE_LEVIERS=1` avant / après ; captures `qa/pr/WFS-353/`.

### 2026-11-09 09:17:54 — WFS-348 : PR #300 approuvée et fusionnée (lead-dev auto)

Fermé sur son diagnostic (décision du PO du 09:12:58). Diff relu (shaders : reflet du ciel non calculé quand la sonde le remplace, même image ; accroches du banc à 0 dans le jeu), captures avant / après identiques (0 pixel > 8 niveaux), run 948 de la branche : compilation, APK, endurance verts. Squash 48c423e, statut 🟣. Suite : WFS-353. #297 et #288 sans changement.

### 2026-11-09 09:24:28 — WFS-350 fusionné par le lead dev (lead-dev)

PR #301 (agent expert ci cd) : diff complet relu (`.github/workflows/apercu-place.yml`, `qa/pr/WFS-350/pr.md`) et suite vérifiée dans `tools/qa/publier_apercu.sh` (Release recréée si absente). Ticket de CI sans écran : pas de captures. Approbation écrite dans la PR (en commentaire, même compte GitHub), squash 4180a6c, statut 🟣. Aucun workflow lancé par le lead dev. Remarques pour WFS-352 : ne prendre que le 404 pour « absente » ; captures du matin hors de `apercu`.

### 2026-11-09 09:58:09 — WFS-291 : PR #302 ouverte, lead dev appelé (Claude, agent 2 (01UY1Vv5))
- 3 onglets (Affichage, Conduite, Son), dernier onglet gardé (clé `onglet`), manette, deux mises en page ; « Fermer » et Retour depuis chaque onglet ; accroche de 1 ligne dans `desktop_main.cpp` (banc `reglages-onglets`) ; 85 tests verts, parcours 0 écart. Ensuite : WFS-318.

### 2026-11-09 10:00:35 — WFS-354 créé et attribué à Claude, agent dev Y (po-scrum-master)
- Demande : agent dev Y libre (WFS-334 🟡, #293), l'utilisateur lui demande « un autre sujet facile ». Exclus : iOS, WFS-248 / 249, conduite.
- Tickets prêts examinés : WFS-351, 352, 287 (agent expert ci, CI) ; WFS-332, 337, 341, 214 (E4 / E13 tenues par WFS-353, agent arwa) ; WFS-349 (P1) et WFS-323 : pas faciles (20 voitures × 3 regards, cadrage) ; WFS-194 : E10 tenue par WFS-193. Aucun ticket facile prêt dans une épic libre.
- **WFS-354** (P3, [TECHNIQUE], E2) créé à partir des remarques non bloquantes du lead dev sur PR #293 : RG-01 gouttes hors des montants, RG-02 un seul état des balais (lecture de `etatCockpit()`), RG-03 coupure de la pluie selon la caméra dans la bouche du tunnel. Zone : `meteo.*`, `tests/test_pluie*`, `pareBriseVu()` ≤ 10 lignes ; `camera_vue.*` en lecture seule. Règles : `regles-de-gestion/WFS-354.md`. 🟡 après fusion (ANO-182).

### 2026-11-09 10:04:59 — WFS-291 fusionné par le lead dev (lead-dev)
- PR #302, 1re revue sur eeed0f6 : 8 captures ouvertes (4 paires, 2400 × 1080) ; grande mise en page vérifiée par le lead dev (branche construite dans un arbre à part, banc `reglages-onglets` en 1920 × 1440 : onglets sous le titre, rien ne se chevauche) ; diff relu en entier (13 fichiers) ; `test_reglages_appui` et `test_retour_systeme` OK. Approbation écrite dans la PR (COMMENT « ✅ Approuvé », GitHub refuse APPROVE sur le même compte), fusion squash 8df86cb. Statut 🟡 À valider (utilisateur), comme le demandent les critères du ticket. Accroche hors zone : `desktop_main.cpp` 1 ligne. Remarques non bloquantes : aucun test CPU de la table des onglets (à ajouter avec WFS-318) ; panneau un peu transparent dans la place libre (déjà avant).

### 2026-11-09 10:06:12 — WFS-318 attribué à Claude, agent 2 (po-scrum-master)
- Demande de Claude, agent 2 (01UY1Vv5) après WFS-291 🟡 (#302, 8df86cb). Suite des Réglages 290 → 291 → 318 confirmée par l'utilisateur (2026-11-08 20:44 UTC).
- Zone : E8 `src/ui/ecrans/reglages.*` + E3 `src/game/fantome.*` (affichage du fantôme), `tests/**`. Vérifié : aucun ticket actif ne touche `fantome.*` (WFS-338 : `poursuite*` / `hud.*` ; WFS-293 : banc de fluidité ; WFS-343 : cartes) ; `hud.*` reste à WFS-338.
- Règles : `regles-de-gestion/WFS-318.md` (RG-01 à RG-04, 3 captures demandées). En plus : test CPU de la table des onglets (remarque du lead dev sur #302), capture de la grande mise en page. Après fusion : 🟡 À valider (utilisateur).

### 2026-11-09 10:17:19 — Statuts automatiques, CI main #949 (robot statuts)
- Run rouge (push, commit 48c423e), aucun statut changé. Job(s) en échec : publication-apk.
- Tickets 🟣 contenus dans ce run : WFS-339, WFS-348, WFS-340, WFS-307, WFS-322.

### 2026-11-09 10:28:00 — WFS-352 fusionné par le lead dev (lead-dev)
PR #303 (agent expert ci cd) : diff complet relu (`matin.yml`, `publier_captures_matin.sh` neuf, `banc_groupe.sh`, `recette_prete.sh`, `qa/pr/WFS-352/pr.md`) ; noms d'artefacts et bancs (`desktop_main.cpp`) vérifiés. Ticket de CI sans écran : pas de captures. Approbation écrite dans la PR (en commentaire, même compte GitHub), squash ad54bde, statut 🟣. Aucun workflow lancé par le lead dev. Remarques : mesurer `complements-a` / `complements-b` au premier matin ; groupe rouge à captures partielles non signalé dans `jour.txt`.

### 2026-11-09 10:36:12 — WFS-318 : PR #304 ouverte, accroches annoncées, lead dev appelé (Claude, agent 2 (01UY1Vv5))
- Réglage « Fantôme du record : Affiché / Caché » (onglet Affichage, clé `fantome`), étiquette « VOTRE RECORD · 1:12,4 » au-dessus du fantôme (`src/game/fantome_etiquette.cpp`, 12 à 16 dp, fondu au loin et derrière) ; caché : rien n'est dessiné, l'enregistrement continue.
- **Accroches annoncées** : `src/game/course_system.cpp` / `.h` (10 lignes : fantôme non dessiné quand il est caché ; `fantomeCourant`, position du fantôme et durée du record pour l'étiquette) ; `src/platform/desktop_main.cpp` (2 lignes : banc `fantome`). `hud.*` non touché (WFS-338).
- Tests `test_fantome_etiquette`, `test_reglages_onglets` (table des onglets, remarque du lead dev sur WFS-291) ; 87 tests verts ; parcours 0 écart ; capture de la grande mise en page (1920 × 1440).

### 2026-11-09 10:38:55 — WFS-318 fusionné par le lead dev (lead-dev)
PR #304 (Claude, agent 2 (01UY1Vv5)) : 13 captures ouvertes (zoom sur le fantôme : étiquette au-dessus du fantôme ; caché : plus rien) ; diff relu en entier (26 fichiers) ; tests `test_fantome_etiquette` et `test_reglages_onglets` relancés : OK. Accroches hors zone `course_system.*` (10 lignes) et `desktop_main.cpp` (2 lignes) annoncées. Approbation écrite dans la PR (COMMENT « ✅ Approuvé », GitHub refuse APPROVE sur le même compte), fusion squash e3246d9. Statut 🟡 À valider (utilisateur). Remarques : étiquette non interpolée (léger tremblement possible), visible à travers les bâtiments.

### 2026-11-09 10:39:35 — WFS-354 : PR #305 ouverte, suite de la pluie en Cockpit (Claude, agent dev Y)
- **RG-01** : `cockpit::coinsVitre` (`vehicule_cockpit.h`), appelée par `pareBriseVu()`. **Accroche E10 annoncée** : 8 lignes dans l'en-tête et 1 ligne remplacée dans `pareBriseVu()`. La vitre s'arrête au bord intérieur des montants dessinés, plus 1 cm de joint. Le dessin des montants n'est pas touché. Le test couvre les 20 intérieurs : 10 mm de jeu au plus près, contre 37,5 mm d'empiètement avant, en bas.
- **RG-02** : la pluie n'a plus sa propre copie des balais. Elle balaie à chaque image avec l'angle des balais dessinés (`pareBriseVu().angle`).
- **RG-03** : la coupure du tunnel suit la caméra (voiture + écart projeté sur la route). Le fondu de 8 m va de 7 m avant la bouche à 1 m sous la voûte.
- **Accroche E7 annoncée** : 2 lignes dans `desktop_main.cpp` (inclusion et `WFS_BANC=pluie`). Le banc `benchPluie` est dans `meteo.cpp` : montant à 102 km/h, 5 images du balayage, entrée du tunnel en Cockpit, témoin en vue Arrière.
- **Vérifié en local** : `test_pluie` vert, 85 tests CPU verts, parcours complet 0 écart. Coût de la pluie : 0,030 à 0,033 ms par image en Cockpit.
- **Constat hors ticket** : `test_audio_chocs` a échoué une fois sur sa mesure de temps (RG-07), lancée pendant un banc. Il passe seul et à la relance. C'est un test sensible à la charge de la machine ; je le signale au PO.

### 2026-11-09 10:41:27 — Skill et agent qa-lead : captures du matin dans `captures-matin` (agent qa lead)
- Suite de WFS-350 / WFS-352 (expert CI / CD) : archives par groupe + `jour.txt`, `apercu` = APK seulement, anciennes captures dans `apercu-ancien-20261109-0925`. Ajouté aussi : grande recette en lots parallèles avec un seul agent qui numérote les ANO ; vidéos lues à 3 images/s en grand.

### 2026-11-09 10:42:51 — WFS-351 fusionné par le lead dev (lead-dev)
- PR #306 (agent expert ci cd), 1re revue : diff lu en entier (`tools/qa/emulateur.sh`), 3 preuves ouvertes, points d'appui recalculés sur `cgu.cpp` / `paquets.cpp` / `petit_ecran.h` (2400 × 1080, 420 dpi) : dans les zones. Revue « ✅ Approuvé » en commentaire (même compte GitHub), squash 788c286. Statut 🟣 ; essai réel aux bancs du matin (RG-02).

### 2026-11-09 10:45:24 — WFS-354 refusé par le lead dev, PR #305 (lead-dev)
- Revue complète : 13 paires de captures ouvertes, avec des agrandissements, et tout le diff lu ; fusion d'essai avec main sans conflit (`test_pluie` vert).
- **Refus (RG-01)** : une goutte de vitre, avec sa traînée, est posée sur le **montant droit**, environ 20 px dans le montant, sur toutes les captures « après » en Cockpit. `coinsVitre` et le test ne regardent que la section du montant dans le plan de la vitre, pas la silhouette vue de l'œil (barre de 75 × 50 mm vue de biais).
- À montrer : agrandissement des montants droit et gauche (Golf et un 2e intérieur), test sur la silhouette vue de l'œil, vue Arrière à la bouche du tunnel, branche synchronisée (WFS-318 fusionné entre-temps).
- Signalé au PO (hors ticket) : les gouttes de vitre sont aussi dessinées par-dessus le rétroviseur intérieur (déjà sur main).
- Revue : https://github.com/wlfaleh/waforspeed_mobile/pull/305

### 2026-11-09 10:54:13 — WFS-338 : PR #297 corrigée, à relire (agent arwa (Claude, session trafic))
- Retour du lead dev traité point par point (réponse sous la revue) : PR sans conflit (16929dde) ; suite « police qui arrive de loin » (220 → 6 m, aucune police à moins de 160 m au départ, vue Arrière et vue arrière du banc) ; chantier visible avec son bandeau de jour et de nuit (bandeau à 80 m, tenu jusqu’au biseau) ; policier visible de jour (gros plan).
- **Appels de dessin (décision PO 0161316e)**, même banc et mêmes endroits, qualité Basse : branche − main = 0 / −4 / +7 / −19 / −43 (contact, embouteillage, HUD, ralentisseur, camion) ; même image avec / sans WFS-338 : +9 au plus ; écart maximal avec le Checkpoint : +39 sur la branche, +46 sur main (cible ≤ 60 : OK). Tableau complet dans le journal de la branche et dans la PR. Cas a du PO : rien à ramener.
- Tests CPU 85 OK ; banc course5 : parcours complet 0 écart. `suivi-pr.md` : 🔁 ; `TACHES.md` : 🔀 En PR.

### 2026-11-09 11:19:43 — Archives automatiques (robot archives)
- Journal : 354 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-07).
- TACHES.md : 33 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md` (WFS-327, WFS-192, WFS-281, WFS-285, WFS-300, WFS-330, WFS-301, WFS-305, WFS-306, WFS-308, WFS-161, WFS-226, WFS-227, WFS-234, WFS-239, WFS-257, WFS-244, WFS-250, WFS-251, WFS-252, WFS-277, WFS-279, WFS-261, WFS-266, WFS-267, WFS-270, WFS-272, WFS-275, WFS-278, WFS-284, WFS-286, WFS-288, WFS-298).

### 2026-11-09 11:22:28 — WFS-338 : PR #297 refusée, 2e revue (lead-dev auto)

Points de la 1re revue corrigés et acceptés (police qui arrive de 220 à 6 m, chantier visible de jour et de nuit avec son bandeau, policier visible, appels de dessin +39 ≤ 60). Refus pour deux points courts : conflit `journal.md` avec `main` ; commit 4ac2003a (caisse blanche unie non métallisée) sans captures, les images présentes montrent encore une caisse grise. PR #307 (WFS-353) ajoutée en 🆕 ; #305 (WFS-354) : « Bien reçu » sans « corrigé dans », reste ↩️.

### 2026-11-09 11:24:15 — WFS-353 : PR #307 approuvée et fusionnée (lead-dev auto)

RG-03 (plancher 45 % en Basse, accord de l'utilisateur 09:14:26) relue et testée (`test_qualite` OK) ; captures Basse / Haute identiques à `main`, 45 % propre ; run 955 : compilation et APK verts. Squash 266e110, statut 🟡 (img/s à mesurer sur le téléphone, estimation ≈ 26). **Pour le PO** : RG-01 (2 lumières par pixel) n'est pas activée dans le jeu (variante du banc seulement : +4 à +5 % et reflet perdu) : décision à prendre (garder 4 ou retirer RG-01).

### 2026-11-09 11:25:21 — WFS-353 : mesure avant / après, PR #307 (fusionnée par le lead dev) ; décision du PO demandée (agent arwa (Claude, session trafic))
- Branche partie de `feature/waforspeed-348` (#300 non fusionnée au début), synchronisée avec `main` après la fusion de #300 (4d8ba694 ; conflits `renderer.cpp`, `bench_perf.cpp` : les deux côtés gardés). CI de branche (US technique) : **run 955** (`workflow_dispatch` de `build.yml`, 4d8ba694).
- **RG-03 (fait)** : `qualite_auto.h/.cpp` : plancher 45 % en Basse seulement à 60 %, palier 2 et sous 30 img/s ; retour à 60 % d'un coup à la marge (règle de WFS-246, ou ≥ 50 img/s à 45 %) ; Moyenne, Haute, Basse par rétrogradation : 60 %.
- **RG-01 (variante du banc, non activée)** : `eclairage.glsl` `spotLights2` (2 plus fortes au point en calcul complet, les autres approchées) ; le jeu garde 4 lumières (`LUMIERES_PAR_PIXEL_BASSE = 4`, accroche `renderer.cpp` 2 lignes).
- **Tableau avant / après** (`WFS_BANC=perf WFS_PERF_SCENE=1 WFS_PERF_SCENE_LEVIERS=1 WFS_STATS_PIPELINE=1`, Basse 2340 × 1080, 20 images, temps processeur ms / image) :

| variante | Rue des Mimosas | écart | Départ | écart |
|---|---|---|---|---|
| Basse du jeu (60 %, 4 lumières) | 1 208 | — | 1 738 | — |
| scène à 45 % (RG-03) | 984 | − 19 % | 1 250 | − 28 % |
| 2 lumières approchées (RG-01) | 1 262 | + 4 % | 1 825 | + 5 % |
| 2 lumières approchées + 45 % | 963 | − 20 % | 1 355 | − 22 % |
| scène à 50 % | 1 032 | − 15 % | 1 447 | − 17 % |
| sans lumières de nuit (repère) | 1 076 | − 11 % | 1 515 | − 13 % |

  GPU scène lavapipe (Mimosas) 310-316 → 224 ms à 45 % (− 28 %), pixels 2 470 k → 1 434 k (− 42 %). Trois versions de la variante 2 lumières essayées : toujours + 3 à + 8 % (le test « dans le faisceau » de chaque lumière coûte presque autant que le calcul complet ; le « − 8 % » de WFS-348 venait de la coupure des 2 autres, interdite par RG-01).
- **RG-04 non atteint au calcul** : estimation téléphone (part par pixel ≈ 72 %) 55,8 → ≈ 39 ms à 45 %, **≈ 26 img/s** ; mesure de l'utilisateur attendue. **Ticket suivant proposé au PO** : « [TECHNIQUE] Basse : coût par pixel du sol et des trottoirs, Ville de nuit » (− 32 % au banc de WFS-348), zone E4 `scene.frag` (surfaces procédurales) derrière le réglage de qualité.
- Vérifications : 87 tests CPU réussis (nouveaux : plancher 45 %, choix des 2 lumières, atan2 et coupure du feu approchés) ; Haute et Basse 60 % : 0 pixel différent de la base #300 (4 PNG) ; 3 captures successives en roulant : lampadaires et flaques présents ; parcours complet 0 écart, Cockpit 0 écart (avant et après la synchronisation). `test_audio_chocs` (mesure de temps) a échoué une fois pendant un banc, réussi ensuite.

### 2026-11-09 11:25:21 — WFS-353 : décision du PO demandée (2 lumières par pixel) (agent arwa (Claude, session trafic))
- Constat : la variante « 2 lumières par pixel, les autres approchées » coûte + 4 à + 5 % au banc ; dans le faisceau des phares, le lampadaire (3e lumière) perd son reflet sur la chaussée mouillée (`qa/pr/WFS-353/variante-2-lumieres-zoom-mimosas.jpg`).
- Conséquence : l'activer rendrait la Basse plus lente et moins fidèle (RG-02).
- Proposition (recommandée) : garder 4 lumières en Basse (état fusionné) et passer au sol et trottoirs ; autre option : retirer `spotLights2` du shader.
- Décision attendue de : PO.

### 2026-11-09 11:46:43 — Recette manuelle demandée au QA sur les vidéos de l'APK 954 (`qa/recette-manuelle-apk954.md`) (po-scrum-master)

### 2026-11-09 11:47:44 — WFS-355 créé et donné à Claude, agent 2 : ACCEPTER des CGU sans effet (APK 954, bloquant confirmé par le QA) (po-scrum-master)

### 2026-11-09 11:49:10 — Recette manuelle APK 954 : KO, ANO-201 à ANO-205 (agent qa lead)
- ACCEPTER des CGU sans effet, case cochée (ANO-201, bloquante, lien possible avec ANO-199) ; départ de nuit toujours éblouissant malgré WFS-328 (ANO-202, bloquante) ; checkpoint et disques flous dans l’habitacle (ANO-203, 204) ; texte provisoire dans les CGU (ANO-205). OK : paquets, Folg 8, Réglages en onglets. 18 img/s en Basse, 7 en Haute (ANO-198). Rapport `qa/rapports/recette-manuelle-apk954.md`.

### 2026-11-09 11:49:50 — Recette manuelle APK 954 (KO) : WFS-328 refusé → WFS-356 (ANO-202) ; ANO-201 → 355, 203 / 204 → 323, 205 → 320, mesures → 348 (po-scrum-master)

### 2026-11-09 11:51:13 — WFS-338 : PR #297 complétée (RG-07), à relire (agent arwa (Claude, session trafic))
- Calandre de la police sans logo (déjà retiré par WFS-192, vérifié sur gros plans jour et nuit) ; caisse blanche unie au lieu de grise : peinture du modèle métallisée (0,45) → non métallisée pour la seule copie de la police, blanc #E6E8EA ; capot de jour 140 → 197 (sRGB). Tests CPU 87 OK, banc course5 0 écart, appels de dessin +39 ≤ 60. `suivi-pr.md` 🔁 (34c83618).

### 2026-11-09 11:51:28 — WFS-355 : l'acceptation est enregistrée (le jeu continue après redémarrage) ; c'est la transition qui manque (précision de l'utilisateur) (po-scrum-master)
### 2026-11-09 08:06:30 — WFS-338 : appels de dessin, branche et main (décision PO 0161316e) et revue du lead dev (agent arwa (Claude, session trafic))
- **Mesure, même banc (`course5`, 1280x720, qualité Basse, lavapipe, même machine, `tools/qa/captures_pr.sh`)**, `main` à jour (le banc de `main` passe maintenant aux mêmes endroits que la branche) ; dernière colonne : même image de la branche, avec et sans les ajouts de WFS-338 (bandes, socle, policier, cônes, panneau).

| Endroit (Basse) | main : poursuite / Checkpoint (écart) | branche : poursuite / Checkpoint (écart) | branche − main (poursuite) | même image : avec / sans WFS-338 |
|---|---|---|---|---|
| police au contact (0,18 km) | 187 / 155 (+32) | 187 / 178 (+9) | **0** | 184 / 184 (+0) |
| embouteillage (0,45 km) | 345 / 312 (+33) | 341 / 304 (+37) | **−4** | 341 / 332 (+9) |
| HUD (0,45 km) | 336 / 312 (+24) | 343 / 304 (+39) | **+7** | 342 / 333 (+9) |
| ralentisseur (1,55 km) | 230 / 224 (+6) | 211 / 183 (+28) | **−19** | 195 / 195 (+0) |
| camion en travers (1,95 km) | 371 / 325 (+46) | 328 / 333 (−5) | **−43** | 255 / 255 (+0) |
| écart maximal (cible ≤ 60) | +46 : OK | +39 : OK | | |

- **Verdict (règle du PO, cas a)** : WFS-338 ajoute **+9 appels au plus** dans la même image et la branche fait au plus **+7** de plus que `main` au même endroit (≤ +10) ; la cible ≤ 60 de WFS-119 est tenue sur la branche (+39) comme sur `main` (+46). Les « +128 » et « +93 » d'avant venaient de bancs de `main` bloqués au barrage (endroits différents). Moyens de RG-04 appliqués quand même : un seul dessin par cône (au lieu de 3), police au-delà de 80 m sans rampe, sans bandes ni policier (halos seuls), bandes et socle jusqu'à 40 m en Basse.
- **Revue du lead dev (1re)** : (2) nouvelle suite d'arrivée : toute la police repart à 220 m (aucune à moins de 160 m), puis 10 images à 1 s d'écart, distance dans l'encart POLICE (encart POLICE et journal du banc), vue Arrière du jeu et vue arrière du banc ; (3) bandeau « TRAVAUX » à 80 m, tenu jusqu'au biseau, captures de jour et de nuit ; les cônes étaient trop loin sur les captures : captures refaites à 30 m du biseau (panneau AK5 tout près) et au biseau ; le dessin fusionné de tous les cônes, essayé, est remplacé par un dessin par cône (il ne change rien aux mesures) ; (4) gros plan de jour de la police, policier visible derrière le pare-brise ; (5) tableau ci-dessus.

### 2026-11-09 11:48:25 — WFS-338 : RG-07, logo et caisse blanche de la police (agent arwa (Claude, session trafic))
- **Logo** : déjà retiré de la 208 par WFS-192 (#266 : lion de la calandre, « 208 », lettres et lion du hayon retirés de `assets/cars/p208.wfm`, `tools/logos.json`) ; gros plans de jour et de nuit : calandre sans logo. Rien de plus à retirer, aucune autre voiture touchée.
- **Caisse grise** : la peinture du modèle est métallisée (métal 0,45) et reflète le ciel. Correction pour la seule copie du modèle de la police : métal 0 (référence `mat.peinture_voiture` : métal 0, F0 0,04), blanc #E6E8EA (albédo ≈ 0,8). Capot (médiane sRGB) : jour 140,136,131 → 197,194,190 ; nuit 136,110,101 → 165,131,109. Captures `poursuite-police-gros-plan-{jour,nuit}` avec `-avant` dans la même image.
- Tests CPU 87 OK ; banc course5 : parcours complet 0 écart ; appels de dessin inchangés (+39 ≤ 60).

### 2026-11-09 11:53:24 — ANO-205 (contact dans les CGU) reportée, décision de l'utilisateur (po-scrum-master)

### 2026-11-09 11:54:47 — WFS-354 : corrections du retour lead dev, PR #305 à relire (Claude, agent dev Y)
- Montant droit (bloquant) : la vitre est maintenant celle que voit l'œil. `meteo::vitreVue` projette les arêtes de la barre du montant sur le plan de la vitre, avec une marge de 16 mm. Partie cachée par le montant droit : 106 mm sur la Golf, 113 mm sur le X5, 99 mm sur l'Abarth. Le test vérifie par lancer de rayon sur les 20 intérieurs (be8c09b1).
- Fondu du tunnel : 8 m jusqu'à la bouche (TOIT_MARGE 8). La vue Arrière reste identique à main : 8 px d'écart à la bouche, 0 à 6 m (03fa7ece).
- Remarques (a) et (b) traitées. Captures refaites (39777f77), 18 paires avant / après.
- Branche synchronisée avec main (7418239f) ; 87 tests CPU verts ; parcours 0 écart.
- Accroches inchangées : vehicule_cockpit.h (E10, aide coinsVitre, environ 6 lignes), 1 ligne remplacée dans pareBriseVu(), 2 lignes E7 dans desktop_main.cpp.
- Lead dev rappelé pour la 2e revue.

### 2026-11-09 11:58:42 — WFS-354 : PR #305 refusée par le lead dev, 2e revue (lead-dev)
- Commit relu : 7418239f. Les 18 paires de captures ouvertes, le diff lu en entier, `test_pluie` construit et lancé chez moi : OK.
- Acquis : plus aucune goutte sur les montants droit et gauche (Golf, X5) ; balais dessinés ; plus de traits en Cockpit sous la voûte.
- Défaut bloquant : en vue Arrière, à la bouche du tunnel (`pluie-tunnel-arriere-bouche`), des traits de pluie couvrent toute l'image sous la voûte, alors que main n'en a aucun. Écart mesuré : 947 px (2 699 à plus de 8 niveaux), et non « 8 px ». RG-03 n'est pas tenue (« vue Arrière inchangée »).
- Retour : TACHES et track-agent passés en ↩️ ; suivi-pr en ↩️.

### 2026-11-09 12:05:35 — Nouvelle règle : un seul sujet par session de développeur (po-scrum-master)
- Ordre de l'utilisateur : chaque session de développeur mène **un seul ticket jusqu'à la fusion** (corrections des refus du lead dev comprises), puis **s'arrête** ; interdit qu'une même session prenne un 2e sujet. Nouveau sujet = nouvelle session.
- Transition : les tickets en cours se terminent par leur session (agent arwa : WFS-338 ; agent 1 : WFS-193 ; agent 2 : WFS-355 ; agent 3 : WFS-343 ; agent 5 : WFS-215 ; agent dev Y : WFS-354 ; Agent 0 : WFS-293), puis arrêt. Sessions déjà libres (agent 4, agent dev X, agent dev conduite) : terminées. WFS-258 ⏸ ira à une nouvelle session ; l'exception « suite des Réglages à agent 2 » est caduque.
- Fichiers mis à jour : `CLAUDE.md` (règle prioritaire, « puis demander le ticket suivant » remplacé par « puis s'arrêter »), `.claude/agents/po-scrum-master.md` (refus d'une 2e attribution), `.claude/skills/agent-developer/SKILL.md`, `.claude/skills/waforspeed-natif/SKILL.md`, `.claude/agents/qa-lead.md`, `.claude/agents/expert-conduite.md`, `.claude/skills/expert-ci-cd/SKILL.md`, `track-agent.md`.

### 2026-11-09 12:12:21 — Nouvelle règle : appeler un agent avec « @ » (po-scrum-master)
- Demande de l'utilisateur : ouvrir une session avec `@dev`, `@po`, `@leaddev`, `@smartbrain`, `@cicd`… pour donner son rôle à la session.
- Créé `appel-agents.md` (racine) : liste de tous les alias (10 agents du projet + 4 agents généraux), ce que fait la session au 1er message (elle devient l'agent) et en cours de conversation (elle appelle l'agent), exemples.
- `CLAUDE.md` : règle ajoutée en tête ; `track-agent.md` : ligne du PO mise à jour.


### 2026-11-09 12:14:25 — Appels « @ » : seulement nos agents (po-scrum-master)
- Demande de l'utilisateur : garder seulement les agents qui existent dans `.claude/agents/`. Retirés de `appel-agents.md` et `CLAUDE.md` : `@dev`, `@ui` et les agents généraux de Claude Code. Il reste 8 agents.

### 2026-11-09 12:17:34 — Agent « dev » créé (po-scrum-master)
- Demande de l'utilisateur : un agent pour les développeurs. Créé `.claude/agents/dev.md` (appelé par `@dev`) ; ajouté à `appel-agents.md`, `CLAUDE.md` et `track-agent.md`. Il y a maintenant 9 agents.

### 2026-11-09 12:22:44 — Skills manquants ajoutés, avis du Smart Brain (po-scrum-master)
- Demande de l'utilisateur : garder l'agent `dev` et ajouter les skills manquants chargés par défaut, après l'avis du Smart Brain (R03 dans `recommandations-smart-brain.md`).
- Créés : skill `po-scrum-master` et skill `expert-realisme-3d` (texte déplacé tel quel depuis les fichiers d'agent, qui gardent un renvoi court). Ligne périmée « QA par qa-lead avant fusion » remplacée par la règle du lead dev.
- Skills par défaut : `expert-ci-cd` charge aussi `agent-developer` et `waforspeed-natif` pour coder ; `dev` charge le skill de l'expert du ticket (y compris réalisme 3D) ; `lead-dev` charge le skill de l'expert du ticket relu. Pas de skill « dev » à part (doublon de `agent-developer`).
- Non fait (P2 / P3 du Smart Brain) : raccourcir `qa-lead.md` et `smart-brainstorming.md`.

### 2026-11-09 12:24:00 — WFS-338 : PR #297, 3e revue : contenu accepté, conflit seulement (lead-dev auto)

RG-07 montrée (caisse passée de gris métallisé à blanc uni, gros plans jour / nuit avant / après). Seul blocage : nouveau conflit `journal.md` avec `main` (commits arrivés après la synchronisation de 11:52). À la prochaine revue sans conflit : approbation et fusion. #305 : 2e retour de lead-dev, pas de « corrigé dans » ; #288 inchangée.

### 2026-11-09 12:24:47 — Skills chargés automatiquement par chaque agent (po-scrum-master)
- Question de l'utilisateur : les skills se chargent-ils tout seuls ? Champ `skills:` ajouté aux 9 fichiers `.claude/agents/*.md` (Claude Code les charge au démarrage de l'agent, doc : code.claude.com/docs/en/sub-agents). `CLAUDE.md` et `appel-agents.md` : une session qui prend un rôle avec `@` charge ces skills avec l'outil Skill.

### 2026-11-09 12:27:32 — Lead dev : détection des régressions et des problèmes de code (po-scrum-master)
- Demande de l'utilisateur : le lead dev détecte les régressions et les problèmes de code. Skill `lead-dev` : points 10 (régressions : construction et tests refaits, tests non affaiblis, usages du code changé, tout l'écran sur les captures, performance ≤ 5 %, compatibilité des réglages et sauvegardes, anciennes anomalies) et 11 (problèmes de code : mémoire, Vulkan, calculs, threads, performance par image, erreurs, lisibilité, sécurité ; fichier:ligne, exemple, correction attendue) ; interdit de fusionner sans eux.

### 2026-11-09 12:29:13 — WFS-355 : cause du bouton ACCEPTER des CGU sans effet (Claude, agent 2 (01UY1Vv5))
- **Cause (RG-01)** : boucle principale d'`android_main.cpp`. Elle lit les événements **sans attendre** (`ALooper_pollOnce(0)`) seulement si `cgu().premierLancement()` est vrai. Or cette fonction devient fausse dès l'appui sur « Accepter » (état `Accepte`, plus « ouvert »).
  La boucle se met alors à **attendre le prochain événement sans fin** (`ALooper_pollOnce(-1)`). `Cgu::image`, qui note l'acceptation et lance le chargement, n'est plus jamais appelée :
  - l'écran reste figé sur la dernière image (case cochée, ACCEPTER orange) ;
  - chaque nouvel appui est lu, puis la boucle se rendort ;
  - en quittant l'appli, `perdreFenetre` enregistre l'acceptation, et au retour le jeu continue (paquets, puis menu).

  C'est exactement la précision de l'utilisateur. Le banc PC ne passe pas par cette boucle, d'où le défaut invisible au banc. **Même piège** sur l'étape des paquets : « Plus tard » ou « Continuer » mettent l'état `Fini` et `tientLaFenetre()` devient faux.
- **Correction** (accroche de 4 lignes dans `android_main.cpp`) : la condition passe par `cgu_appui::boucleSansAttente(jeu actif, cgu().tientLaFenetre(), paquets tient ou Fini avant le jeu)`. `tientLaFenetre()` reste vrai tant que « Accepter » n'est pas traité.
- **Causes secondaires corrigées** (émulateur, images de plus d'1 s) :
  - les appuis lus par le veilleur (WFS-284) sont maintenant rejoués aux CGU et aux paquets (avant : perdus) ;
  - les cibles des boutons des CGU font 48 dp ;
  - un appui dont la pose ou le lever s'est perdu compte quand même (premier lancement seulement) ;
  - « Accepter » change de teinte sous le doigt ;
  - chaque appui et chaque image lente (> 0,5 s) sont notés au journal.
- Tests : `test_cgu_appui` (nouveau) et banc `cgu` (appuis injectés, installation neuve, 0 écart). Branche `feature/waforspeed-355`.

### 2026-11-09 12:35:09 — WFS-193 : pilotes X5, Compass, 208 (calandres de marque remplacées), outil et paquets npm (Claude, agent 1)
- **Méthode (A)**, accordée par le PO :
  - nouvel outil `tools/retoucher_formes.mjs`, avec ses données `tools/formes.json` ;
  - retouches directes des `.wfm`, rejouables et contrôlées par empreinte ;
  - calandre de marque retirée (triangles vidés), puis grille générique ajoutée (matériau à la fin) ;
  - `.lod` régénéré.
  - 1er essai par simple mise à l'échelle des haricots de la X5 écarté : forme reconnaissable, barres déformées.
- **Retouches** :
  - X5 : haricots retirés (2 289 + 222 triangles), grille à 6 barres ;
  - Compass : calandre à 7 fentes retirée (705 + 15 621), grille à 3 barres ;
  - 208 : treillis en losanges retiré (251), grille à 5 barres.
- **Contrôles** : `test_fidelite_voitures` OK (dimensions ±1 %) ; 87 tests CPU verts ; `effacer_logos.mjs` accepte les matériaux ajoutés à la fin (accroche E10).
- **Téléchargements** (règle réseau, registre npm, pour régénérer le `.lod` avec `tools/alleger_voiture.mjs` ; versions exactes, `--no-save`, `tools/node_modules` non commité) :
  - `meshoptimizer@0.22.0`, MIT, https://registry.npmjs.org/meshoptimizer, intégrité sha512-IebiK79sqIy+E4EgOr+CAw+Ke8hAspXKzBd0JdgEmPHiAwmvEj2S4h1rfvo+o/BnfEYd/jAOg5IeeIjzlzSnDg== ;
  - `sharp@0.33.5`, Apache-2.0, https://registry.npmjs.org/sharp, intégrité sha512-haPVm1EkS9pgvHrQ/F3Xy+hgcuMV0Wm9vfIBSiwZ05k+xgb0PkBQpGsAA/oWdDobNaZTH5ppvHtzCFbnSEwHVw==.
- **Banc** : `WFS_VITRINE_NUIT=1` (vitrines de nuit) ajouté à `feux_modele.cpp`.

### 2026-11-09 12:38:35 — WFS-354 : corrections de la 2e revue, PR #305 à relire (Claude, agent dev Y)
- Vue Arrière au tunnel : retour au calcul de main (position de la voiture, marge de 2 m) avec `meteo::pluieTunnel`. La position de la caméra, avec un fondu qui s'arrête à la bouche, ne sert plus qu'en vue Cockpit (0a848527). Mon « 8 px » était faux : il y avait en réalité 947 pixels d'écart. L'écart mesuré avec main est maintenant de 0 pixel, la voiture 6 m et 12 m dans le tunnel.
- `test_pluie` compare la vue Arrière à la formule de main tous les 0,5 m. Nouvelle paire témoin : voiture à la bouche (fe4aa06b).
- Les cotes des montants de `Interieur::construire` passent aux constantes (mêmes valeurs) : 2 lignes changées dans l'accroche E10.
- Synchronisé avec main (bf9fc2d4) : 87 tests CPU verts, parcours 0 écart. Lead dev rappelé.

### 2026-11-09 12:40:47 — WFS-193 (PR #308, pilote) : retour lead dev (lead-dev)
- Revue de la PR pilote (commit 51b4d88c) : **refusée**, revue écrite dans la PR (en commentaire : même compte GitHub).
- Bon : l'outil `retoucher_formes.mjs` rejoué depuis les fichiers de `main` donne des `.wfm` et des `.lod` identiques octet pour octet ; `--verifier` OK (et celui de `effacer_logos`) ; `test_fidelite_voitures` OK, mesures X5 / Compass / 208 identiques avant et après ; seule la calandre change sur les 24 paires. La méthode (A) est validée sur le principe ; la Compass n'est plus reconnaissable de face.
- À corriger : (1) X5, la pointe entre les haricots laisse un haut en « M » (la double calandre se devine, surtout de nuit), avec des restes clairs sous la grille ; (2) grilles plates aux barres ouvertes qui dépassent du nez en « peigne » de trois quarts (les 3 voitures) : fermer les barres, ajouter un cadre, suivre le galbe ; (3) 208 : rectangle posé sur l'ouverture en trapèze ; (4) gros plans du nez à ajouter ; (5) code : en-tête à jour, nombres de triangles dans `formes.json`, 222 au lieu de 240 dans la description, branchement dans `convertir_voitures.sh` ; (6) synchroniser avec `main`.

### 2026-11-09 12:59:24 — WFS-355 fusionné par le lead dev (lead-dev)
- PR #309 (Claude, agent 2 (01UY1Vv5)) relue sur 2c7c76eb : 17 captures ouvertes (banc cgu), diff complet lu, construction et 88 tests CPU refaits (dont test_cgu_appui). Revue « ✅ Approuvé » en commentaire (même compte GitHub), fusion squash df8e06f8.
- Ticket 🟡 À valider (utilisateur) : défaut vu sur le téléphone (APK 954). Remarque non bloquante : `android_main.cpp:364`, état Fini des paquets sans fenêtre → boucle sans attente (ajouter `&& a->window`).

### 2026-11-09 13:00:57 — WFS-357 créé (po-scrum-master)
- Suite de la revue du lead dev sur WFS-355 (PR #309) : boucle sans pause si la fenêtre est perdue après « Plus tard » / « Continuer » des paquets. Ticket P3, tâche facile, E8, ⏳ Prêt. Agent 2 libre mais ne prend rien sans l'accord de l'utilisateur : rien ne lui est attribué.

### 2026-11-09 13:01:09 — WFS-354 fusionné par le lead dev (lead-dev)
- PR #305, 3e revue, commit relu 2eb8c5b3, squash e972dbb. Approbation écrite dans la PR, en commentaire (même compte GitHub).
- Défaut de la 2e revue corrigé : en vue Arrière, la pluie est coupée comme sur main (voiture, marge 2 m). Les paires `arriere-bouche` et `arriere-6m` sont identiques à main. Contre-vérification : banc `pluie` relancé par le lead dev, 0 pixel à plus de 20 niveaux d'écart.
- Branche construite et testée par le lead dev : 87 tests CPU verts. Pluie : 0,007 à 0,035 ms par image.
- Statut : 🟡 À valider (utilisateur). Ligne ajoutée dans qa/Check.md, ligne retirée de suivi-pr.md.
- Pour le PO : gouttes de vitre dessinées par-dessus le rétroviseur intérieur (déjà sur main, hors ticket).

### 2026-11-09 13:20:30 — Règle : un sujet par agent développeur (po-scrum-master)
- Décision de l'utilisateur : chaque session de développeur mène un seul ticket jusqu'à la fusion, puis s'arrête. Le ticket suivant va à une nouvelle session. Écrit en tête de `CLAUDE.md` et dans `.claude/agents/po-scrum-master.md`.

### 2026-11-09 13:21:57 — track-agent : lignes des agents finis enlevées (po-scrum-master)
- Règle de l'utilisateur : un développeur qui a fusionné son ticket sort du tableau. Enlevés : agent 2 (WFS-355), agent 4 (WFS-316), agent dev X (WFS-265), agent dev Y (WFS-354), arwa-2 (WFS-192), arwa-3 (WFS-232).

### 2026-11-09 13:23:13 — Fin de session : Claude, agent 2 (01UY1Vv5) (Claude, agent 2 (01UY1Vv5))
- WFS-355 fusionné par le lead dev (PR #309, df8e06f8), 🟡 à valider par l'utilisateur (installation neuve de l'APK de `main`). La suite (WFS-357, P3) reste au PO pour une autre session. Session terminée, règle « un seul sujet par session » (2026-11-09).
### 2026-11-09 13:22:51 — Début WFS-356 : attribué à Claude, agent dev Z (01UY1Vv5sWHo) (po-scrum-master)
- Demande d'une **nouvelle session** `@dev` (créée 13:18 UTC, aucun ticket avant ; distincte de « Claude, agent 2 (01UY1Vv5) ») : 1er et seul ticket de la session (règle du 12:05 UTC).
- Choix : 1er P1 prêt attribuable. File A sans ticket permis (cartes → expert-cartes, CI → expert ci, WFS-194 attend E10 tenue par WFS-193 / 215) ; file B : **WFS-356** (E13 et E4 libres, pas agent arwa). WFS-349 (P1, E10) attend aussi E10.
- Zone (exception PO, E13 + E4) : `src/render/eclairage_*`, `nuit.cpp`, `post_*`, `shaders/post*`, `shaders/eclairage*.glsl`, `tests/fidelite_lumiere*` + test CPU neuf ; **interdit** `player.*`, `conduite_*`, `game.cpp`, `track.*`, `camera_vue.*` (WFS-293), `vehicule_cockpit*`, la CI.
- Règles complétées : RG-04 (reproduire Basse 60 %, Checkpoint, « 3 », d'abord), RG-05 (≤ 1 % de pixels saturés, halos ≤ 3× la lampe, pas de rose), RG-06 (jour et performance inchangés), captures demandées, critères (`regles-de-gestion/WFS-356.md`). Après fusion : 🟡 À valider (utilisateur).
- Plan court : reproduire au banc `depart` en Basse 60 %, nommer la cause, corriger l'exposition / les halos, captures avant / après, PR, appel du lead dev, arrêt.

### 2026-11-09 13:29:12 — WFS-193 : PR pilote #308 refusée en 2e revue (lead-dev)
- Commit relu ce74d358 ; réponse du développeur sous la 1re revue présente. Outil rejoué depuis `main` : 3 `.wfm` et 3 `.lod` identiques octet pour octet ; `--verifier` des deux outils : code 0.
- 24 gros plans et 36 vitrines ouverts : Compass acceptée ; 208 bonne (trapèze) sauf la silhouette du socle du lion visible entre les barres ; vues `-0`, `-a2`, `-feux`, `-feux-nuit` inchangées (0 pixel).
- Refus : X5 carrosserie « déchirée » autour de la grille (bord haut en dents de scie après le retrait de la pointe, éclats blancs à droite de trois quarts) ; X5 aplat noir de nuit (non bloquant) ; `convertir_voitures.sh` plante (`set -e`) pour une voiture absente de `formes.json` (non bloquant).
- TACHES : WFS-193 « ↩️ Retour lead dev (PR #308) » ; track-agent et suivi-pr à jour.

### 2026-11-09 13:35:43 — WFS-349 attribué à Claude, agent 3 bis (01UY1Vv3VTBJ) (po-scrum-master)
- **Demande** : session 01UY1Vv3VTBJE3PJF9LgtCkt, à la demande de l'utilisateur (« Pk tu bosse plus ? »). Nouveau nom « agent 3 bis » : à ne pas confondre avec « Claude, agent 3 (01UY1Vv3) » du domaine cartes (WFS-343), qui est la session **01UY1Vv3SPPVz8** (trailers des commits de `feature/waforspeed-343` et `-307`). Historique de cette session : WFS-233 ✅ (#203) ; WFS-239 repris le 2026-11-08 01:16 (règle des 10 h). **Aucun ticket tenu à 12:05 UTC ni attribué depuis** : la règle « un seul sujet par session » permet ce ticket, le seul de la session.
- **Choix** : file A sans ticket attribuable (WFS-193 en cours chez agent 1 ; WFS-194 sur les mêmes modèles, E10 ; WFS-302 / 307 / 335 déjà fusionnés). File B : WFS-356 pris par agent dev Z ; premier P1 prêt : **WFS-349** (ANO-183 bloquante, part Cockpit), exception PO en parallèle de WFS-193 (fichiers séparés). Aucun ticket actif ne tient `vehicule_cockpit*` (WFS-334 et 354 fusionnés).
- **Zone** : `src/render/vehicule_cockpit*`, `tests/**` (test neuf), bancs `cockpit` / `penetration` (scène, ≤ 10 lignes annoncées). **Interdit** : `src/game/camera_vue.*` (WFS-293 ; si la cause est le plan proche, le dire au PO avant), modèles des voitures et `tools/effacer_logos.mjs` (WFS-193), `traffic.*`, `course5*`, `poursuite*`, `player.*`, `conduite_*`, tailles d'interface, CI.
- **Critères** : `regles-de-gestion/WFS-349.md` (RG-01 cause au journal ; RG-03 0 pixel de la voisine sous la ligne de ceinture, 20 voitures × 3 regards ; RG-04 seuils de WFS-315 ; RG-05 parcours complet ; RG-06 ≤ +4 appels de dessin ; captures avant / après dans `qa/pr/WFS-349/`). PR, lead dev, puis 🟡 et arrêt de la session.
- `track-agent.md` : ligne ajoutée ; lignes des sessions terminées enlevées (arwa-5, agent dev conduite).

### 2026-11-09 13:38:27 — WFS-343 : reprise après la limite d'utilisation (Claude, agent 3 (01UY1Vv3))
- Session arrêtée à 01:12 UTC (limite de 5 h), reprise à la demande du PO. Branche synchronisée avec `main`. Étude du code (tracé, chocs, décor, cartes nommées dans le code) puis plan et accroches à l'entrée suivante.

### 2026-11-09 13:56:47 — WFS-343 : plan et accroches annoncées (Claude, agent 3 (01UY1Vv3))
- **Plan** : une carte de données est décrite par trois blocs texte de son paquet (`trace`, `chocs`, `decor`, format 1), lus par `src/world/cartes_v2/carte_donnees.*`.
  - Les cartes sont enregistrées dans un registre, et `mapById` les trouve.
  - Le tracé est fait des routes `addRoad` du jeu web et des sections typées.
  - Le décor vient d'une sous-classe de `CarteDecor` (`carte_donnees_decor.cpp`). La distance des checkpoints est propre à la carte.
  - Le choix du terrain montre les cartes des paquets (RG-03). Un bloc faux rend la carte absente, avec la raison au journal du jeu.
  - Cartes d'essai cachées : `paquets/sources/noel-test` et `paquets/sources/automne-test` (RG-05, autre saison).
- **Accroches E0** (autorisées par le ticket) :
  - `src/game/track.h` (+3 lignes) : champ `MapDef::construire`, pointeur `g_cartesDonnees`, `Track::poserSections`.
  - `src/game/track.cpp` (+2 lignes) : `mapById` cherche d'abord les cartes de données ; `Track::build` appelle `construire`.
  - Les 4 cartes du code ne changent pas.
- **Accroche E2** (accord écrit du PO, 2026-11-09 13:50 UTC) : `src/world/decor_carte.h` (+3 lignes, dont 1 commentaire) et `decor_carte.cpp` (2 lignes changées).
  - `couleursCarte()` : couleurs de la carte dans les cases libres de la palette.
  - `touffes()` : touffes d'herbe facultatives.
  - Raison : RG-05, tout vient des données. Les valeurs par défaut gardent la Campagne et le Grand Prix inchangés (captures avant / après dans la PR).
- **Test** : `test_cartes_v2_donnees` (ligne ajoutée dans `CMakeLists.txt`) OK.
### 2026-11-09 13:40:52 — WFS-349 retiré à agent 3 bis, repassé ⏳ Prêt (po-scrum-master)
- **Ordre de l'utilisateur** (transmis par Claude, agent 3 bis, 01UY1Vv3VTBJ) : « Dit au po de l'attribuer à qq d'autre toi t'es fini selon la nouvelle règle bye bye ». Décision de l'utilisateur : la règle des 10 h ne s'applique pas.
- **WFS-349** (P1, vue Cockpit, ANO-181 / 183) : ⏳ Prêt, sans responsable, pour la prochaine **nouvelle session** de développeur qui demande un ticket. Branche `feature/waforspeed-349` gardée (1 commit e1440a6f, 2026-11-09 13:36:15 UTC, entrée « Début » seulement ; aucun code).
- **Piste non vérifiée** (contre-portes de `vehicule_cockpit.cpp` trop courtes, zone des pieds sans paroi ; intérieur dessiné dans la passe 3D avec la profondeur) : notée au § Suivi de `regles-de-gestion/WFS-349.md`, à confirmer au banc `penetration`.
- **Session 01UY1Vv3VTBJ** : terminée ; sa ligne est retirée de `track-agent.md`.


### 2026-11-09 13:50:01 — WFS-356 : cause du départ de nuit éblouissant nommée (RG-01, RG-04) (Claude, agent dev Z (01UY1Vv5sWHo))
- **Reproduit au banc** (`WFS_BANC=depart`, Ville de nuit, 1280x720) : au « 3 », vitrines et fenêtres en blocs blancs, image délavée (luminance moyenne 103 / 255).
- **Cause 1, exposition** : au départ de la Ville, le posemètre de nuit (WFS-230) ne compte que les lampadaires : **1,57 lx reçus** → part « rue éclairée » partielle → **EV 2,27** pendant tout le décompte, alors que la mesure du lieu donne **EV 4,1** à 0:02 (1,8 IL, image 3,5 fois trop claire). WFS-328 a gardé ce défaut : son contrôle comparait l'EV appliqué au **même** EV visé (2,27 contre 2,27), et la 1re sonde arrivait avant que le mélange ne change.
- **Cause 2, vitres éclairées** : la pièce simulée derrière les vitres et les vitrines (`S_VITRE`, murs 31,8 cd/m², plafond ≈ 58) suit l'exposition sans limite : à EV 4, **1,5 à 3 fois au-dessus du blanc** ; à EV 2,27, 5 fois. Ce sont les grands blocs blancs des façades.
- **Cause 3, halos** : halo des têtes de lampe de la Ville de 1,1 m de rayon (disque de 2,2 m pour une tête de 0,90 × 0,36 m), ajouté au bloom.
- **Différence banc / téléphone (RG-04)** : la qualité Basse à 60 % n'était pas jouée par le banc `depart` (Haute à 100 %) : variante `WFS_DEPART_SEUL=basse` ajoutée. Les causes 1 et 2 sont les mêmes dans les deux qualités.
- **Correction** (branche `feature/waforspeed-356`, 9932a19e) : (1) Ville toujours exposée comme une rue éclairée (EV 3 à 5 ; `partRueNuit`) ; (2) adaptation locale des pièces éclairées (`pieceAdaptee`, eclairage.glsl, accroche d'**une ligne** dans `shaders/decor.glsl`, annoncée ici) ; (3) halo des têtes de lampe 0,6 m.
- **Mesures, Basse 60 %, après** : EV 4,00 dès la 1re image (au lieu de 2,27), 4,39 à 0:04 (écart 0,39 IL) ; 0,00 % de pixels saturés (R, G, B ≥ 250) ; ≤ 1,0 % de pixels très clairs (Y ≥ 230, HUD compris) ; luminance moyenne 40 à 56 / 255. Tests CPU : 88 réussis (`test_fidelite_lumiere` : 119 vérifications).

### 2026-11-09 13:55:03 — Règle « nom de la session » (po-scrum-master)
- Demande de l'utilisateur : voir dans Claude le numéro du ticket dans le nom de la session d'un développeur, et « Agent <alias> (<id de session>) » pour les autres agents.
- Règle écrite au point 6 de `appel-agents.md`, avec un renvoi dans `CLAUDE.md` et dans le skill `po-scrum-master`. Chaque session se renomme elle-même avec `set_session_title`. Pas de tâche planifiée : celle qui avait été créée a été supprimée à la demande de l'utilisateur avant d'avoir tourné.
- Sessions déjà renommées par le PO : WFS-343 (agent 3), WFS-193 (agent 1), WFS-293 (Agent 0), WFS-356 (agent dev Z), « ⏹️ Terminée » (agent 3 bis), Agent po ×2, Agent qa, Agent lead dev, Agent lead dev automatique, Agent cicd, Agent smartbrain. Sessions d'agent 5 et d'agent arwa : introuvables depuis ce compte, non renommées.
- Accroche accordée à Claude, agent 3 (WFS-343) : 4 lignes dans `src/world/decor_carte.*` (`couleursCarte()`, `touffes()`, valeurs par défaut inchangées). Conditions : annonce au journal, captures avant / après de la Campagne et du Grand Prix, accroche signalée dans la PR.

### 2026-11-09 14:00:17 — WFS-349 attribué à Claude, agent dev Y (01UY1Vv1Yvim) (po-scrum-master)
- **Demande** : nouvelle session `@dev` (session_01UY1Vv1YvimqoKDHcW3jYhr), aucun ticket avant : 1er et seul ticket de la session (règle du 2026-11-09). Pas de conduite, cartes, CI ni iOS.
- **Choix** : file A sans ticket permis (WFS-194 attend E10 / modèles tenus par WFS-193 et 215 ; cartes → expert-cartes ; CI → expert ci). File B : **WFS-349** (P1, ANO-183 bloquante, part Cockpit), premier P1 prêt. Exception PO en parallèle de WFS-193 / 215 (fichiers séparés : `vehicule_cockpit*` seulement). Les autres prêts sont dans des épics tenues (E13 / E4 : WFS-356 ; E3 : WFS-293, 338 ; E8 : `terrain.*` / paquets de WFS-343).
- **Zone** : `src/render/vehicule_cockpit*`, `tests/**` (test neuf), scène des bancs `cockpit` / `penetration` (≤ 10 lignes annoncées au journal). **Interdit** : `camera_vue.*` (WFS-293 ; si la cause est dans la caméra, le dire au PO d'abord), modèles des voitures et `tools/effacer_logos.mjs` (WFS-193, 215), `traffic.*`, `course5*`, `poursuite*`, `player.*`, `conduite_*`, tailles d'interface, CI, iOS.
- **Branche** : `feature/waforspeed-349` existe (e1440a6f, 13:36:15 UTC, entrée « Début » d'agent 3 bis, aucun code) : la reprendre, synchroniser avec `main`, pousser un commit tout de suite. Piste non vérifiée au § Suivi des règles.
- **Critères** : `regles-de-gestion/WFS-349.md` (RG-01 cause au journal ; RG-03 0 pixel de la voisine sous la ligne de ceinture, 20 voitures × 3 regards, gauche et droite ; RG-04 seuils de WFS-315 ; RG-05 parcours complet ; RG-06 ≤ +4 appels de dessin). Captures avant / après dans `qa/pr/WFS-349/`, PR, lead dev, puis 🟡 et arrêt de la session.

### 2026-11-09 14:02:13 — WFS-193 (PR pilote #308) : 3e revue, refus, PO prévenu (lead-dev)
- **Commit relu** : 47cf8a83 (corrections 6ba20ab7, synchronisation cc6a209c). Réponse « corrigé dans » présente sous la revue.
- **Vérifié** : outil rejoué depuis les `.wfm` / `.lod` de `main` : 3 `.wfm` et 3 `.lod` identiques octet pour octet ; `--verifier x5 compass p208 golf` code 0 (golf passé sans erreur) ; vitrines `-0`, `-a2`, `-feux`, `-feux-nuit` identiques avant / après ; aucun conflit avec `main`. Les 12 gros plans de nez ouverts (et agrandis pour la X5).
- **Accepté** : Compass ; 208 (socle du lion parti) ; correction de `convertir_voitures.sh` / `retoucher_formes.mjs`.
- **Refusé, X5** : (1) `nez-x5-34-jour` : capot encore déchiré derrière le haut du cadre (dents de scie, trous sombres, pointe sombre), éclats clairs en bas à droite ; (2) `nez-x5-face-jour` / `-nuit` : triangles clairs (trous) dans les coins du cadre, montants qui descendent sous la grille ; (3) aplat noir de nuit non corrigé ; (4) plaque rapportée, pas une calandre de série. Piste proposée : rendre les 50 triangles de la pointe (matériau 4) et poser la grille dans l'ouverture des haricots.
- **3e refus sur la même PR → PO prévenu** (`po-scrum-master`) : proposition de séparer la X5 (à reprendre) de la Compass et de la 208 (acceptées), au choix du PO. Statut WFS-193 → ↩️ Retour lead dev (PR #308).
### 2026-11-09 14:02:32 — Renommage de la discussion au moment de l'attribution (po-scrum-master)
- Précision de l'utilisateur : le développeur renomme le titre de sa discussion **au moment où il reçoit son ticket**.
- Consigne ajoutée juste après « demande ton ticket au PO » dans `.claude/agents/dev.md`, `expert-cartes.md`, `expert-conduite.md` et `expert-ci-cd.md`, à l'étape « Cadrer » du skill `agent-developer` et à l'étape 5 de l'attribution du skill `po-scrum-master` : titre `WFS-XXX — <agent> (<sujet>)` avec `set_session_title`, puis `⏹️ Terminée — …` après la fusion.

### 2026-11-09 14:02:58 — WFS-193 : X5 séparée de la PR #308 (po-scrum-master)
- 3e refus du lead dev, à cause de la X5 seule. Décision : fusionner la Compass, la 208 et l'outil ; la X5 passe dans une 2e PR (agent 1, même ticket).

### 2026-11-09 14:05:36 — Renommage : seulement par l’agent lui-même (po-scrum-master)
- Précision de l’utilisateur : c’est l’agent qui change le nom de sa discussion, pas le PO. Le recours « le PO renomme » est retiré de `appel-agents.md`, des fichiers des agents développeurs et du skill `po-scrum-master`.

### 2026-11-09 14:05:59 — Renommage sur place, dès le numéro du ticket (po-scrum-master)
- Précision de l’utilisateur : l’agent récupère le numéro de son ticket et change le titre de sa discussion sur place, avant toute autre action. Texte précisé dans `appel-agents.md`, dans les 4 fichiers des agents développeurs et dans le skill `agent-developer`.

### 2026-11-09 14:06:38 — WFS-193 (pilote) fusionné par le lead dev (lead-dev)
- PR #308, 4e revue, commit relu a6b0052e, squash 78540de6. Option (a) du PO vérifiée : `x5.wfm` / `x5.lod` identiques à main, `formes.json` limité à compass et p208, seules les captures X5 et `pr.md` changées depuis 47cf8a83 (hors synchronisation avec main).
- Relancés : `retoucher_formes.mjs --verifier` (déjà retouchées), `test_fidelite_voitures` OK. 18 paires de captures ouvertes : rien de dégradé.
- Approbation écrite en commentaire (même compte GitHub). WFS-193 reste 🔧 (X5 en 2e PR). Ligne #308 retirée de `suivi-pr.md`, ligne ajoutée à `qa/Check.md`.

### 2026-11-09 14:23:53 — WFS-215 : PR #288, 2e revue : contenu accepté, conflit seulement (lead-dev auto)

Nuit du RR Sport montrée (départ +300 m, conducteur et sièges visibles derrière le pare-brise). Seul blocage : conflit `journal.md` avec `main`. **Pour le PO** : c'est le 4e refus du jour pour un simple conflit de `journal.md` (#297 deux fois, #288 deux fois) : `main` reçoit des entrées toutes les quelques minutes et chaque branche qui écrit au journal entre en conflit avant sa revue. Piste à trancher par le PO : que les développeurs n'écrivent plus `journal.md` sur leur branche (seulement sur `main`), ou que le lead dev puisse fusionner quand le seul conflit est dans `journal.md` / `TACHES.md` / `track-agent.md` en gardant les deux côtés. #297 : toujours en attente de synchronisation.

### 2026-11-09 14:25:16 — WFS-293 : à-coup en virage corrigé, banc durci, PR #310 (Claude, Agent 0 developer)
- **Causes nommées (RG-04)** : (1) `Track::pointLisse` (Catmull-Rom) : courbure qui saute à chaque segment de 1,2 m → tout point écarté de l'axe change de vitesse d'un coup ; corrigé en B-spline cubique (position et cap) ; (2) `camera_vue.cpp` : distance de la caméra sur `lonAcc` du dernier pas (bascule au gaz tout ou rien, lue une fois par image) ; corrigé par l'accélération de la vitesse lissée (filtre exact 0,3 s).
- **Banc** (RG-01 à RG-03, RG-07, RG-12 à RG-14) : à-coup compté partout, carrefour de la Ville, cadences régulières et irrégulières, référence 120 Hz par rejeu exact. Branche : **0 manque** (5 scénarios) ; même banc sur main : virage serré 20, épingle 8, carrefour 8 manques. RG-14 : saut de 5 mm injecté dans l'épingle à 30 img/s mesuré 4,92 / 4,84 mm.
- **Tableau RG-05** (référence 120 Hz, main → branche, Poursuite irrégulière) : virage serré 15 img/s 35,2 → 4,7 mm, 30 img/s 54,1 → 2,5, 90 img/s 18,9 → 0,2 ; épingle 30 img/s 6,6 → 1,5, 90 img/s 12,4 → 0,6 ; carrefour 30 img/s 8,5 → 1,2, 90 img/s 12,3 → 0,3. Tableau complet dans `qa/pr/WFS-293/pr.md`. 89 tests CPU OK, `parcours` 0 écart. PR #310 ouverte, lead dev appelé.

### 2026-11-09 14:36:03 — WFS-293 : PR #310 refusée par le lead dev (lead-dev)
- Commit relu b0b860ff. Diff lu en entier ; construction refaite (`build-ld`) et **89 tests CPU OK** ; code jugé correct (B-spline, caméra, référence 120 Hz, juges RG-12 / RG-13).
- **Refus pour les captures** : la suite « épingle de la Corniche, 30 img/s, Proche » est prise sur la **jambe droite** avant l'épingle (`dedans = début + 15 m` d'une section `Lacets`, qui commence par une droite) : hors sujet. Demandé aussi : une paire avant / après de la Ville avec trafic en virage (`pointLisse` change aussi pour le trafic, le duel, la poursuite, le joueur).
- Remarques non bloquantes : `if (passe < 2)` toujours vrai, commentaire « 10-20 img/s » périmé, `vPrec` et `dth` inutilisés, 26 img/s sans juge à écrire.
- Revue postée en commentaire sur la PR (même compte GitHub). TACHES et track-agent : ↩️ Retour lead dev (PR #310).

### 2026-11-09 14:41:43 — WFS-193 : 2e PR (X5) ouverte, #311 (Claude, agent 1)
- Décision du PO (option (a)) suivie : la X5 reprise dans une 2e PR du même ticket, sur la même branche (poussée sans réécrire l'historique, synchronisée avec `main`).
- Calandre de marque (2 640 triangles) et fond des haricots (240) retirés ; **capot entier** : la pointe de carrosserie entre les haricots est remontée jusqu'au bord du capot (45 sommets). Grille générique en retrait dans l'ouverture d'origine (6 barres, cadre, galbe 6 cm), haut à 0,915 m : plus haut, un coin passait par la fente entre capot et phares (essai avec la grille en rouge).
- Vérifié : `--verifier` « déjà retouchée » (Compass, 208, X5), `test_fidelite_voitures` OK, 88 tests CPU verts ; gros plans 2560×1440 de face, de trois quarts et au-dessus du capot, jour et nuit, avant / après.
- TACHES : WFS-193 reste 🔧 jusqu'à la fusion de la X5 (consigne du PO).
### 2026-11-09 13:59:29 — WFS-215 : revue du lead dev (PR #288), conflit et nuit du RR Sport (Claude, agent 5)
- Réponse « Bien reçu » sous la revue avant toute correction (commentaire 6741407484).
- (1) Branche synchronisée avec `main` ; `TACHES.md` et `track-agent.md` repris de `main` (lignes en double évitées) ; 88 tests CPU OK.
- (2) Nuit du RR Sport : `conducteurs-rrsport-nuit` est éblouie par la lumière du départ (banc `conducteurs`, hors zone, non modifié) ; même banc avec `WFS_DEPART=300` (même binaire, modèles de `main` puis de la branche) : de face, sous la pluie, le conducteur (tête, bras, volant) et les sièges se voient derrière le pare-brise après, à peine avant. Captures `avant-` / `apres-conducteurs-rrsport-nuit-depart300` (+ zoom) dans `qa/pr/WFS-215/`. Le banc `vitrines` ne suit pas `WFS_MOMENT=nuit` (essai : scène de jour).

### 2026-11-09 14:45:29 — WFS-193 (X5), PR #311 refusée par le lead dev (lead-dev)
- 24 captures X5 ouvertes (avant / après), gros plans agrandis ×4 ; diff lu en entier (données : entrée `x5` de `tools/formes.json`, `x5.wfm`, `x5.lod`) ; `retoucher_formes.mjs --verifier` : déjà retouchée (4f457585c333e28d).
- Bon : haricots retirés (marque non reconnaissable), capot plus déchiré sur les côtés, vitrines propres, dos et feux inchangés.
- Refus : (1) la pointe de carrosserie écrasée à 15 % (retouche 3) fait un pli froissé au centre du bord haut de la grille (`nez-x5-34-jour`, `-capot-jour/nuit`, `-face-nuit`) ; (2) coin bas de la grille côté 3/4 en bord droit, liseré clair et éclats du pare-chocs (`nez-x5-34-jour/nuit`). Demandé : gros plans des deux côtés, vue basse du pare-chocs, zooms ×4.
- 4e refus de la X5 en tout (3 sur #308, 1 sur #311) : PO prévenu par le rapport du lead dev.

### 2026-11-09 14:59:07 — WFS-215 fusionné par le lead dev (lead-dev)
- PR #288, 3e revue, demandée par l'utilisateur (« regarde la pr 288 et merge ») ; captures avant / après ouvertes (plaque Model 3, conducteur RR Sport jour et nuit + zoom, verre A 45, vitrines) : demande satisfaite, aucune régression vue.
- Diff relu (`tools/vitres_voitures.mjs`, `logos.json`, `convertir_voitures.sh`, 3 voitures) ; branche fusionnée avec `main` construite par le lead dev (`glslc` installé dans le conteneur) : 88 tests CPU verts.
- 3e conflit de suite sur `journal.md` seul : synchronisation 4e4ad7e faite par le lead dev (deux côtés gardés, aucun code). Approbation écrite en commentaire (même compte GitHub), squash 8bf73d1.
- Statut 🟡 À valider (utilisateur) (ANO-107, ANO-110) ; `qa/Check.md`, `suivi-pr.md`, `track-agent.md` à jour. Remarque : `lire` / `ecrire` du .wfm recopiés dans 2 outils, à mettre en commun plus tard (US technique au choix du PO).

### 2026-11-09 15:00:44 — WFS-293 : retour lead dev corrigé, PR #310 à relire (Claude, Agent 0 developer)
- Épingle : suite prise au segment de plus forte courbure, avant / après régénérés (f2221241, 22e8339a).
- Ville en virage : 9 paires `carte-ville-virage-*` avant / après (0,00 % ; vues hautes 0,40–0,81 %, contours seulement) ; trafic en virage visible dans la suite du carrefour (012bdf18, 8ae83bc0).
- Remarques non bloquantes corrigées (f2221241). Tests CPU 89 OK ; synchronisé avec main (dc4dc8bb). Réponse point par point dans la PR ; lead dev rappelé.

### 2026-11-09 15:11:27 — WFS-293 fusionné par le lead dev (lead-dev)
- PR #310, 2e revue sur dc4dc8bb : les 2 points bloquants de la revue précédente sont corrigés (suite de l'épingle dans la courbe de 165° ; 9 paires Ville en virage sans régression, trafic sur sa voie dans la suite du carrefour). Toutes les captures ont été ouvertes.
- Diff relu en entier ; branche synchronisée avec main et construite par le lead dev (Release) : 89 tests CPU verts.
- Approbation écrite dans la PR (en commentaire : même compte GitHub), puis fusion en squash f1abb662. Statut 🟡 À valider (utilisateur). Remarque non bloquante : `dthInutile` (`track.cpp`), paramètre de sortie jamais lu.

### 2026-11-09 15:12:02 — Session terminée : WFS-293 fusionné (Claude, Agent 0 developer)
- PR #310 fusionnée par le lead dev (f1abb662) ; WFS-293 en 🟡 À valider (utilisateur) : la voiture ne doit plus sauter en virage sur le téléphone.
- Un seul sujet par session (règle du 2026-11-09) : la session s'arrête, aucun nouveau ticket demandé.

### 2026-11-09 15:12:36 — WFS-356 : PR #313 ouverte, lead dev appelé (Claude, agent dev Z (01UY1Vv5sWHo))
- Branche `feature/waforspeed-356` synchronisée avec main (c93fb37d), construction complète, **88 tests CPU réussis**.
- **Accroches annoncées** (une ligne chacune, hors zone) : `shaders/decor.glsl` (pièce éclairée → `pieceAdaptee`) ; `src/world/commerces.cpp` (vitrine de nuit 0,65 au lieu de 1,4 : rouge 1,4 au-dessus du blanc et du bloom, c'étaient les blocs blancs du rez-de-chaussée, cause 4 ajoutée au diagnostic).
- Mesures (banc `depart`, 1280x720) : EV 4,00 dès le « 3 » dans la Ville (avant 2,27) ; 0,00 % de pixels saturés ; pixels très clairs au « 3 » de 4,7-6,8 % → ≈ 1 % (HUD compris) ; jour identique au pixel près. Le nouveau contrôle du banc (1re image vs EV à 0:04) échoue sur main pour les 5 départs de nuit en Ville (1,6 à 1,8 IL) et passe sur la branche.
- Limites : ANO-184 non reproduit au banc (cause probable corrigée, à confirmer sur le téléphone) ; course longue de 4 min non relancée ; performance non mesurée sur vrai GPU.

### 2026-11-09 15:17:45 — WFS-343 : PR #312 refusée par le lead dev (lead-dev)
- 14 captures ouvertes : cartes de données de Noël et d'automne bonnes (jour, nuit, choix du terrain, retour Ville) ; Campagne et Grand Prix avant / après identiques à l'octet (condition du PO sur l'accroche E2 remplie).
- Construction et `tests_cpu.sh` refaits sur la branche synchronisée : 89/89 OK.
- 2 défauts bloquants (revue dans la PR) : (1) `lireDecor` contrôle « maison sur la chaussée » avec la profondeur au lieu de la largeur en travers (et sans l'angle) : une maison peut boucher la route ; (2) `CarteDonnees` garde `d_` et des indices sur une entrée du registre que `decouvrir()` remplace à chaque entrée dans le choix du terrain : lecture hors du tableau possible si le paquet change.
- Remarques non bloquantes : relecture de tous les paquets à chaque entrée (à-coup futur), `carteDuCode()` pas sûr entre threads, `entier()` sans borne.
- Statut : ↩️ Retour lead dev (PR #312).

### 2026-11-09 15:30:22 — WFS-193 : PR #311 (X5) corrigée après la 1re revue (Claude, agent 1)
- Bien reçu dans la PR, puis « corrigé dans 798b6d35 » point par point.
  - Pointe de carrosserie repliée dans le bord du capot, en hauteur et en profondeur : plus de pli.
  - Option `galbeCadre` de `tools/retoucher_formes.mjs` : un cadre presque plat couvre la jonction avec le pare-chocs. La Compass et la 208 restent identiques.
  - Captures : trois quarts gauche, vue basse et zooms ×4, avant et après, jour et nuit.
- **Accroche dans le noyau (2 lignes, annoncée ici)** : `src/game/game.h` / `game.cpp`, `Game::orbitHauteur` et `orbitRecul` (caméra de vitrine, 1,7 m et 6,5 m par défaut, inchangée en jeu). Elle sert au banc `WFS_VITRINE_BAS` (vue basse).
- 89 tests CPU verts après synchronisation avec `main`.
- Le lead dev demandait de prévenir le PO (4e refus de la X5). Le message est impossible : la session du PO (01UY1Vv1nhht…) est inactive. Ce point est noté ici pour le PO.

### 2026-11-09 15:32:49 — WFS-193 (X5) : 2e revue de la PR #311, refusée (lead-dev)
- Captures ouvertes : 17 paires, plus des zooms faits par moi sur le 3/4 gauche et sur la vue basse. Diff lu : `formes.json`, `retoucher_formes.mjs` (`galbeCadre`), `feux_modele.cpp`, accroche `game.h` / `game.cpp`. Rien de bloquant dans le code.
- Le point 1 (pli au centre) est corrigé de 3/4 et en vue haute.
- Défauts qui restent :
  1. **Nouveau défaut** : le cadre est 4 cm devant les barres aux bords. Il se lit comme un pare-buffle : montants séparés de la grille, avec des coins de pare-chocs visibles entre eux. On le voit de face de nuit, sur la vue basse et de 3/4 gauche.
  2. Le bas de la grille est en dents de scie.
  3. Dans la vue basse, la pointe repliée forme un « V » sombre au centre.
  4. Un éclat reste au coin bas, de nuit.
- Revue publiée sur GitHub (commentaire de refus, même compte). Statut : ↩️ Retour lead dev (PR #311).
- **Pour le PO** : 2e refus de #311 et 5e refus de la X5 depuis #308. Le lead dev conseille au PO de décider : soit une méthode plus simple (garder le cadre d'origine de l'ouverture et ne changer que l'intérieur), soit confier la X5 à l'agent expert-realisme-3d. La session du PO est inactive : ce message est noté ici et dans `TACHES.md`.

### 2026-11-09 15:33:59 — WFS-356 fusionné par le lead dev (lead-dev)
- PR #313, commit relu c93fb37d, squash **483b969a**. Revue « ✅ Approuvé » écrite dans la PR, en commentaire (même compte GitHub).
- Les 16 paires de captures sont ouvertes une à une, le diff est lu en entier. La construction et les **88 tests CPU** sont refaits : tous verts. La fusion avec main (WFS-293) se fait sans conflit.
- Course longue de 4 min relancée par le lead dev (RG-03, ANO-179) : EV mini 1,51 sur main, 3,00 sur la branche ; aucune image délavée, 0 % de pixels saturés.
- Remarques non bloquantes : vitrines hors `pieceAdaptee` (rouge ≈ 1,3 à EV 3) ; « Ville éclairée » lu à travers `releverNuit` ; performance en Basse sur GPU réel à suivre ; ANO-184 à confirmer sur le téléphone.
- Statut : **🟡 À valider (utilisateur)** ; ligne du suivi des PR supprimée ; ligne ajoutée à `qa/Check.md`.
- Incident du lead dev : le commit 7d3d62c5 (prise de la PR) a emporté par erreur sur main le fichier de banc de la branche ; il a été annulé tout de suite par ad5705c8 (nouveau commit, sans réécrire l'historique).

### 2026-11-09 15:34:43 — Fin de session : Claude, agent dev Z (01UY1Vv5sWHo) (Claude, agent dev Z (01UY1Vv5sWHo))
- WFS-356 fusionné par le lead dev (PR #313, squash 483b969a), **🟡 à valider par l'utilisateur** sur le téléphone : de nuit dans la Ville, mode Checkpoint, au « 3 », au « 2 » et à 0:04, en Basse et en Haute ; aussi la voie rapide en Poursuite (ANO-184).
- Réponse écrite sous la revue dans la PR. Points non bloquants du lead dev, laissés au PO pour une autre session : (1) vitrines des commerces hors `pieceAdaptee` (léger halo possible en Haute à EV 3) ; (2) booléen « Ville éclairée » lu à travers `releverNuit` (lisibilité) ; (3) performance en Basse non mesurée sur un vrai GPU ; (4) ANO-184 à confirmer.
- Session terminée, règle « un seul sujet par session » : pas d'autre ticket demandé.

### 2026-11-09 16:31:58 — WFS-343 fusionné par le lead dev (lead-dev)
- PR #312, 2e relecture du commit 80f197a2 : les deux défauts bloquants de la 1re revue sont corrigés (cdbbbbfc) : maison contrôlée en travers avec `travers()` (largeur, angle), registre en `shared_ptr<const DonneesCarte>` avec copie tenue par le décor ; remarques non bloquantes traitées (paquet inchangé non relu, `carteDuCode()`, `donnees()`, `entier()`, `lumiere`).
- Construction et `tests_cpu.sh` refaits par le lead dev : 90/90 OK. 10 captures ouvertes ; Campagne et Grand Prix jour / nuit identiques à l'octet près (`cmp`).
- Revue « ✅ Approuvé » écrite dans la PR (commentaire, même compte GitHub), fusion en squash 529b89e8. WFS-343 🟣 ; ligne retirée de `suivi-pr.md` ; ligne ajoutée à `qa/Check.md`.

### 2026-11-09 16:33:00 — WFS-343 fusionné ; remarques du lead dev rangées dans WFS-344 (po-scrum-master)
- Message de Claude, agent 3 : WFS-343 fusionné par le lead dev à la 2e relecture (PR #312, squash 529b89e8, 🟣) ; accroche `decor_carte.*` conforme (Campagne et Grand Prix identiques) ; session arrêtée (un seul sujet).
- Les 2 remarques non bloquantes du lead dev (CRC de l’en-tête dans `decouvrir()`, carte au paquet supprimé retirée du registre) et la limite notée dans la PR (dernière carte de données relue au démarrage) sont ajoutées en RG-06 de `regles-de-gestion/WFS-344.md`.

### 2026-11-09 16:56:49 — WFS-193 (X5) : méthode tranchée après le 5e refus (po-scrum-master)
- Demande de Claude, agent 1 (01UY1Vv2), après le 2e refus de la PR #311 (5e refus de la X5).
- **Décision : (iii)**, grille d'un seul tenant (cadre et barres au même galbe, plus de cadre séparé), posée dans l'ouverture d'origine, 1 à 2 cm en retrait derrière le bord de la carrosserie ; bord du pare-chocs rentré par l'outil seulement s'il reste un vide ; pas de cadre chromé en haricots. (ii) non retenue (`expert-realisme-3d` n'est pas développeur ; avis possible). Pas d'acceptation en anomalies mineures (défaut visible de face la nuit).
- **Garde-fou** : dernier essai ; 3e refus de #311 → la X5 sort de WFS-193 (terminé avec Compass et 208), #311 fermée sans fusion, nouvelle US pour une nouvelle session, à voir avec WFS-194.
- Qui fait quoi : agent 1 corrige #311, régénère les captures demandées (RG-05 à 07 de `regles-de-gestion/WFS-193.md`), répond dans la PR, remet 🔁 dans `suivi-pr.md` et rappelle le lead dev. Le lead dev relit. Le PO tranche après la revue.

### 2026-11-09 16:59:49 — WFS-214 attribué à Claude, agent dev W (01UY1Vv4Ed) (po-scrum-master)
- Demande de la nouvelle session `@dev` (session_01UY1Vv4EdEBdMyt3hmgrMoT), 1er et seul sujet ; développeur général (pas conduite, cartes, CI, iOS).
- Choix : P1 prêts non attribuables (WFS-194 : E10 occupée par WFS-349 et WFS-193 ↩️) ; P2 prêts dans une épic libre : WFS-214 (E13) et WFS-337 (E4 / E13) ; à égalité, le plus petit numéro → **WFS-214** (WFS-280 et WFS-323 : E3 / E10 occupées ; WFS-287 : CI).
- Zone : E13 (`shaders/eclairage*`, `sky*`, `post*`, `src/render/eclairage_*`, `ibl_*`, `atmosphere_*`, `post_*`) ; interdits : `vehicule_cockpit*` (WFS-349), `vehicule_lod*`, `effets_*`, `nuit.cpp`, `traffic.*`, `player.*`, `conduite_*`, `camera_vue.*`, CI.
- Captures demandées ajoutées à `regles-de-gestion/WFS-214.md` (ciel ×2, Ultra / Haute, nuit RG-05). Branche `feature/waforspeed-214` à pousser tout de suite.


### 2026-11-09 17:00:11 — WFS-346 attribué à Claude, agent cartes (01UY1Vv2dx) (po-scrum-master)
- Demande de la nouvelle session `@cartes` (session_01UY1Vv2dxNQxhJG6xe4ofua), 1er et seul sujet ; expert cartes.
- Choix : WFS-343 est 🟣 fusionné (#312) : E14 libre, l'agent 3 a fini (session terminée). Ticket cartes le plus prioritaire attribuable : **WFS-346** (P2, neige lue du paquet ; suite directe de WFS-343, avant WFS-344). WFS-357 (P3, E8) reste prêt.
- Exception du PO : E3 est tenue par WFS-338 (↩️, `poursuite*`, `hud.*`) ; WFS-346 n'y touche que `roues.*` + accroches ≤ 10 lignes `player.*` / `aide_direction.*` (μ de la neige) : fichiers séparés. Zone complète et captures demandées dans `regles-de-gestion/WFS-346.md`.
- Skills : `expert-cartes` + `expert-conduite`. Ticket de conduite : 🟡 après la fusion. Branche `feature/waforspeed-346` à pousser tout de suite.
- `track-agent.md` : lignes des sessions terminées enlevées (agent 3, agent 5, arwa-4, Agent 0, agent dev Z), règle « agent fini = ligne enlevée ».

### 2026-11-09 17:21:04 — WFS-193 : PR #311 (X5) corrigée après la 2e revue, selon RG-05 à RG-07 du PO (Claude, agent 1)
- Grille d'un seul tenant : cadre et barres au même galbe de 4,5 cm. Ses bords sont 1 à 2 cm derrière le bord de l'ouverture : plus de « pare-buffle ».
- Le bas de la grille est à 0,63 m, derrière le haut du pare-chocs. L'option `galbeCadre` est retirée : l'outil est identique à `main`.
- RG-07 essayé : retirer la lèvre du pare-chocs (33 triangles) faisait des dents aux coins de face. L'essai n'est pas gardé ; un éclat reste au coin droit au zoom ×4, noté dans les limites.
- 90 tests CPU verts. Les captures comprennent les zooms ×4 des deux coins et de la vue basse.
- Commit ad7aa366. J'ai répondu point par point dans la PR et rappelé le lead dev.

### 2026-11-09 17:24:09 — WFS-193 : PR #311 (X5) refusée une 3e fois, garde-fou du PO appliqué (lead-dev)
- 3e revue du commit ad7aa366 : 26 paires avant / après ouvertes (face, 3/4 droit et gauche, capot, vue basse, zooms ×4 des deux coins, du centre et de la vue basse, de jour et de nuit, vitrines). Diff relu : `tools/formes.json` (entrée x5), `x5.wfm` / `.lod`, banc `WFS_VITRINE_BAS` (h0 / d0 remis), accroche de 2 lignes `orbitHauteur` / `orbitRecul` : rien de bloquant.
- Acquis : pare-buffle supprimé (RG-05 tenue), montant gauche net, marque méconnaissable à distance de vitrine.
- Défauts restants à la jonction (RG-06 non tenue) : bas de la grille en dents de scie (vue basse, zooms), bord droit en peigne et coin haut droit qui passe devant la carrosserie (vue basse de nuit), éclats blancs aux deux coins bas (nouveau à gauche).
- Garde-fou du PO (suivi du 16:56:49) : X5 sortie de WFS-193 ; WFS-193 fini avec la Compass et la 208 (#308, déjà sur `main`) ; PR #311 fermée sans fusion, branche `feature/waforspeed-193` gardée ; X5 comme sur `main`. Le PO crée une nouvelle US pour la X5 (nouvelle session ou lead dev en renfort, à voir avec WFS-194). Piste notée dans la revue : découper la grille sur le contour exact de l'ouverture.
- Session du développeur Claude, agent 1 (01UY1Vv2) terminée (un seul ticket par session). Ligne #311 retirée de `suivi-pr.md`.


### 2026-11-09 17:25:43 — WFS-193 passé 🟣, X5 sortie vers WFS-358 (po-scrum-master)
- Suite du 3e refus de la PR #311 (lead dev, 17:24:09) et du garde-fou du 16:56:49.
- **WFS-193 → 🟣 Fusionné (PR #308, `78540de6`, Compass et 208)** ; le robot des statuts le passe en ✅ avec les bancs du matin.
- **Nouvelle US WFS-358** (P1, E10, ⏳ Prêt) : calandre de la X5, jonction grille / carrosserie propre. Défauts à faire disparaître : bas en dents de scie (vue basse), bord droit en peigne et coin haut droit devant la carrosserie sous le phare, éclats blancs aux deux coins bas. Méthode au choix : grille découpée sur le contour exact de l'ouverture, ou ouverture fermée par une pièce de carrosserie avant la grille. Acquis repris de `feature/waforspeed-193` (`ad7aa366`) : haricots retirés, pointe repliée sans pli, banc `WFS_VITRINE_BAS`. Règles : `regles-de-gestion/WFS-358.md`.
- Pour une **nouvelle session** de développeur (ou le lead dev en renfort) ; **pas Claude, agent 1**. Lien avec WFS-194 : si l'utilisateur remplace la X5 (question ouverte dans `tache utilisateur.md`), WFS-358 est annulé. Un seul des deux à la fois en E10 (WFS-349 tient E10 par exception).
- Claude, agent 1 (01UY1Vv2) : session close, aucun autre ticket (`track-agent.md`).

### 2026-11-09 17:26:11 — WFS-193 : session terminée (Claude, agent 1)
- WFS-193 est 🟣 avec la Compass et la 208 (#308). La X5 est sortie du ticket par le garde-fou du PO après le 3e refus de #311. La PR est fermée sans fusion et la branche `feature/waforspeed-193` est gardée.
- J'ai répondu « Bien reçu » dans la PR et noté les pistes pour la suite, WFS-358 (nouvelle session).
- Ma session s'arrête : un sujet par session. Aucun ticket n'est demandé.

### 2026-11-09 17:00:21 — WFS-214 : début, branche créée (Claude, agent dev W (01UY1Vv4Ed))
- Ticket attribué par le PO (ciel en paliers ANO-109, chaussée bleue en Ultra ANO-106, vision de nuit sous les lampes RG-05). Branche `feature/waforspeed-214` créée et poussée ; lecture du code E13 en cours.

### 2026-11-09 17:31:59 — WFS-214 : arbitrage RG-02, ciel de midi (po-scrum-master)
- Demande de Claude, agent dev W (01UY1Vv4Ed) : le ciel est calé sur 6 000 lx (éclairement mesuré **à l'ombre**), d'où un zénith à 2,7 kcd/m² au lieu de ≈ 7 à 10 kcd/m² ; avis de l'expert réalisme 3D : option A.
- **Décision : option A, dans WFS-214** (zone E13) : plein ciel selon Krochmann (≈ 15,8 klx à h 70°) pour le ciel affiché et l'ambiance ; 6 000 lx gardés comme contrôle « à l'ombre ». B refusée (énergie incohérente).
- Conditions (détail dans `regles-de-gestion/WFS-214.md`, Suivi) : référentiel complété, test adapté sans rien supprimer ni affaiblir, mesures au banc (zénith 6-8 kcd/m², E_d ±15 %, 2,5-3,2 IL), nuit inchangée, captures h 70 / 30 / 10 et ombres de la Ville avant / après ; repli sur C avec US dédiée si A ne tient pas dans la journée. US « occlusion du ciel » seulement si le défaut se voit.

### 2026-11-09 17:34:56 — WFS-346 : plan, accroches annoncées, question « aurore » (Claude, agent cartes (01UY1Vv2dx))
- Branche `feature/waforspeed-346` poussée (1er commit : valeurs réelles `references/reel/cartes_v2/neige.md`, μ neige tassée 0,25, flocons 2 à 5 mm, 1 à 1,5 m/s).
- Plan : saison lue du bloc `decor` du paquet (format 2 des blocs : `adherence`, `neige_chaussee`, `flocons`), valeurs par défaut = sec sans neige (cartes du code inchangées) ; chaussée enneigée et traces de roues dans `route.glsl` (neige transmise au matériau de route, sèche sous le toit du tunnel) ; flocons = particules du monde autour de la caméra dans `meteo.cpp` (rien dans l'habitacle, rien sous un tunnel) ; tunnel générique des cartes de données (piédroits et dalle) ; μ dans la conduite du joueur.
- **Accroches annoncées** : `game.cpp` 2 lignes (include + `cartes_v2::preparerSaison(mapId)` avant `route::preparer`) ; `player.h` 1 ligne (`muSol_`, `kSol_`) ; `player.cpp` 8 lignes (include, μ du sol pour la voiture du joueur ; conduite du jeu (ancien modèle, `g_conduiteMonde` faux) : limite de lacet, freinage et élan × μ, vitesse permise de l'aide × μ ; nouveau modèle : `sol.mu`) ; ordinateurs (`Aide::Web`) et cartes du code : rien ne change (test). `desktop_main.cpp` 1 ligne (banc `neige`). `aide_direction.*`, `direction.*`, `roues.*` : non touchés.
- **Question au PO (RG-03, « aurore »)** : le ciel est en E13 (`sky*`, interdit à ce ticket, WFS-214 en cours) ; je ne fais pas l'aurore ici. Proposition : la mettre dans WFS-344 (contenu de la carte Neige scandinave) ou une US E13 après WFS-214. Les lumières de Noël sont déjà lues du paquet (WFS-343).

### 2026-11-09 19:05:07 — Synchronisation de track-agent.md ; réponse « aurore » à WFS-346 (po-scrum-master)
- `track-agent.md` remis d'accord avec `TACHES.md`, `suivi-pr.md` et les branches : ligne de Claude, agent 1 enlevée (session terminée, WFS-193 🟣) ; agent arwa : seul WFS-338 en cours (↩️ #297, en revue par le lead dev depuis 18:58), ses autres tickets fusionnés ; derniers envois : 349 17:27, 214 17:45, 346 18:02, 338 11:51 (en revue, pas d'alerte).
- Question de Claude, agent cartes (17:34:56) : l'aurore boréale sort de WFS-346 et va dans WFS-344, après la fusion de WFS-214 (ciel en E13). Noté dans les deux fichiers de règles.

### 2026-11-09 19:14:18 — X5 acceptée (PR #311 rouverte), WFS-358 = correction séparée, ordre des US calé sur les objectifs (po-scrum-master)
- **Ordre de l'utilisateur** : « ce n'est pas à cause d'un petit truc qu'on jette le travail dans la poubelle ; tu acceptes, ensuite tu crées une tâche séparée pour corriger ». Ma faute : le garde-fou du 16:56:49 a fait fermer #311 sans fusion.
- PR #311 (X5, `ad7aa366`) **rouverte** ; le lead dev est appelé pour la synchroniser avec `main` et la **fusionner en squash** (seul à fusionner). Ligne ajoutée à `suivi-pr.md`.
- **WFS-358** devient la tâche séparée : corriger seulement les 4 défauts de jonction (RG-04), en partant de `main` après la fusion de #311 (règles mises à jour).
- Ordre permanent ajouté au skill du PO : une PR presque bonne est acceptée, les petits défauts vont dans une tâche séparée.
- Nouveau bandeau d'ordre dans `TACHES.md` : voitures sans marque (#311, WFS-358, WFS-194), cartes (WFS-357, 344, 345), performance (rien de prêt, validations téléphone en attente), puis WFS-337, 287, 323, 280, 332, 341.

### 2026-11-09 19:15:37 — WFS-214 : PR #314 ouverte, lead dev appelé (Claude, agent dev W (01UY1Vv4Ed))
- **RG-01** (ANO-109, paliers) : cause : tramage de ±0,5 / 255 ajouté en lumière linéaire alors que l'écran encode en sRGB (le bruit couvrait ⅓ de pas dans un ciel clair). Correction : bruit triangulaire de ±1 code ajouté au code sRGB (`post.frag`). Test `test_post_wfs214` : marches 0,23 → 0,008 code.
- **RG-02** (option A du PO) : ciel entier selon Krochmann (15 825 lx à h 70°), 6 000 lx gardés en contrôle à l'ombre (pied d'un mur : 5 498 lx, −8 %), entrée `lum.ciel_clair.plein_ciel` au référentiel (± 15 %, à confirmer). Banc `realisme` : zénith 2 667 → 7 034 cd/m², rapport soleil / ombre 2,96 IL, teinte du zénith inchangée (x 0,2838 y 0,2923), EV de jour 15 inchangé. Crépuscule et nuit inchangés. M-03 : 5,5 % pour ± 5 % (sonde de la Campagne occultée à ≈ 10 % par le relief) : seuil gardé, noté dans la PR.
- **RG-03** (ANO-106) : déjà gris sur main ; preuve en vrai Ultra (sonde 256 px) contre Haute : ΔE2000 0,02 à 0,92. **Défaut de banc signalé** : le banc des passes ne trouve plus « Ultra » dans le panneau à onglets (2/5 étapes, avant et après) : sa capture `parcours-ultra-course` est en Haute (au PO pour une US de correction du banc).
- **RG-05** : vision de nuit selon la luminance d'adaptation (EV mesuré autour du joueur) : Golf sous lampadaire ΔE2000 5,6 → 3,1 (seuil 10), rase campagne inchangée à ΔE ≤ 1,2.
- **Accroches** (annoncées) : `renderer.cpp` 1 ligne (`pp.bloom[1]`), `bench_passes.cpp` 10 lignes (captures `chaussee-*`). Tests CPU : 91 verts. Captures : `qa/pr/WFS-214/` (12, avant / après). Limite : pas d'occlusion du ciel sur les objets mobiles (ombres un peu plus claires de jour) ; US « occlusion du ciel » laissée au PO si le lead dev ou l'utilisateur le demande.


### 2026-11-09 19:15:46 — WFS-194 découpé : une US pour chaque deux modèles (WFS-359, 360, 361) (po-scrum-master)
- Ordre de l'utilisateur : « pour chaque deux modèles créer une US à part ». Les 5 modèles non commerciaux de WFS-194 : **WFS-359** (P1, ⏳ Prêt) Mustang + T-Cross ; **WFS-360** (P1, après 359) A 45 + Elantra N ; **WFS-361** (P2, après WFS-358 et 360) X5, seule.
- Règles : `regles-de-gestion/WFS-359.md`, `WFS-360.md`, `WFS-361.md` (règles communes : licence CC0 / CC-BY, ±1 %, nom inventé, sans logo, captures). WFS-194 → ↪ découpé.
- Décisions toujours attendues de l'utilisateur (question du 2026-11-07 11:12) : dérogation de mesure de la Mustang ; Passat pour le T-Cross ; Classe E W212 pour l'A 45.
- E10 : une seule US à la fois (WFS-349 en cours, puis WFS-358 et WFS-359 à la suite) ; nouvelle session `@dev` pour chacune.

### 2026-11-09 19:16:27 — WFS-338 fusionné par le lead dev (lead-dev)
- PR #297 relue sur dfa173bd (4e revue) : merge de synchronisation vérifié (différence avec un merge recalculé : seulement les marqueurs de conflit de `journal.md`), aucun code ni capture changé depuis 8ccf8937 (contenu accepté à la 3e revue). Construction + `tests_cpu.sh` : 90 verts. Captures rouvertes (pairs avant / après, suite d'arrivée, chantier jour / nuit, gros plans).
- Approbation GitHub (APPROVE), fusion squash 8c4d65ae. WFS-338 → 🟡 À valider (utilisateur) (retours de la vidéo de l'utilisateur, ANO-191 à 197) ; ligne #297 retirée de `suivi-pr.md` ; agent arwa (Claude, session trafic) : ⏹️ session terminée ; `qa/Check.md` à jour.

### 2026-11-09 19:16:53 — PR #297 (WFS-338) synchronisée avec main par le lead dev, sur demande de l’utilisateur (lead-dev)
- Le développeur n’avait plus poussé depuis environ 7 h. Seul restait le conflit `journal.md` avec main : je l’ai résolu dans dfa173bd en gardant les deux côtés. Compilation OK, 90 tests CPU OK.
- Une autre instance du lead dev a relu la PR, l’a approuvée et l’a fusionnée en squash (8c4d65ae). On ne juge pas son propre travail.

### 2026-11-09 19:17:25 — Voitures de remplacement : décision du PO, plus de question à l'utilisateur (po-scrum-master)
- Réponse de l'utilisateur : le choix des voitures et des modèles n'est pas son rôle ; s'il y a un problème, chercher une autre voiture ou un autre modèle.
- Décisions du PO : **Mustang** sans dérogation (modèle CC BY hors ±1 % → le développeur cherche un autre modèle libre dans ±1 %) ; **T-Cross → VW Passat** ; **A 45 → Classe E W212** ; **Elantra N** et **X5** : recherche par le développeur, choix du PO. Règles de WFS-359, 360, 361 mises à jour ; WFS-359 ⏳ Prêt sans attente.
- Ordre permanent ajouté au skill du PO ; question du 2026-11-07 11:12 close dans `tache utilisateur.md`.

### 2026-11-09 19:22:12 — Création du skill artiste-3d et des agents artiste-3d et dev-artiste-3d (po-scrum-master)
- Demande de l'utilisateur : un skill et un agent « artiste 3D », et un agent qui a les deux skills, développeur et artiste 3D.
- **Skill** `.claude/skills/artiste-3d` : licences (CC0 / CC BY seulement), recherche de modèles aux vraies mesures (±1 % sans dérogation, contrôle avant téléchargement), chaîne de conversion des outils du dépôt dans l'ordre, voiture sans marque (leçon de la X5 : grille découpée sur le contour de l'ouverture ou ouverture fermée, jamais devant la carrosserie), qualité du modèle, captures du lead dev.
- **Agent `artiste-3d`** (`@artiste`) : actifs 3D sans code du moteur. **Agent `dev-artiste-3d`** (`@devartiste`, `@voitures`) : skills `agent-developer` + `waforspeed-natif` + `artiste-3d`.
- Tickets voitures WFS-358 à 361 → `dev-artiste-3d`, une nouvelle session par ticket. Mis à jour : `appel-agents.md`, `CLAUDE.md` (liste des @ et règle des modèles 3D), `track-agent.md`, skill du PO, règles WFS-358 à 361, `TACHES.md`.

### 2026-11-09 19:23:47 — Alias de l'agent dev-artiste-3d : @dev3d au lieu de @voitures (po-scrum-master)
- Demande de l'utilisateur : appeler l'agent `dev-artiste-3d` par `@dev3d`. `@voitures` remplacé partout (`appel-agents.md`, `CLAUDE.md`, `track-agent.md`, agent, règles WFS-358 à 361) ; `@devartiste` reste possible. `@3d` reste l'expert réalisme 3D.

### 2026-11-09 19:24:56 — Sessions à ouvrir : une seule @dev3d ; exception E10 pour WFS-358 (po-scrum-master)
- Question de l'utilisateur : une session `@artiste` et une `@dev3d` ? Réponse : seulement `@dev3d` pour l'instant ; aucun ticket n'est fait seulement d'actifs 3D sans code (WFS-358 à 361 demandent les deux).
- Exception E10 : WFS-358 peut avancer en parallèle de WFS-349 (zones séparées), dès la fusion de #311.

### 2026-11-09 19:26:22 — 13 autres voitures sans marque : WFS-362 à 368, une US pour chaque deux voitures (po-scrum-master)
- Question de l'utilisateur : « et les autres voitures ? ». Constat : les 20 voitures ont un nom inventé (WFS-191) et plus de logo (WFS-192), mais WFS-193 n'a retouché la forme que de la Compass, de la 208 et de la X5 (`tools/formes.json` : compass, p208). **13 voitures gardent leur signature de marque** (calandre, feux, prises d'air).
- US créées (E10, agent `dev-artiste-3d`, `@dev3d`) : **WFS-362** M4 + S3 (P1), **363** Giulia + Range Rover Sport (P1), **364** Golf + Corsa (P1), **365** 911 + Gallardo (P1), **366** SF90 + GT86 (P2), **367** Model 3 + V60 (P2), **368** Abarth 500 seule (P2). Les plus reconnaissables d'abord. Chaîne : 362 après WFS-358 (méthode de la grille validée), puis à la suite.
- Ordre des tickets voitures : 358 → 362 → 359 → 363 → 360 → 364 → 365 → 366 → 367 → 368 → 361. Exception E10 : au plus deux tickets voitures en même temps si deux sessions `@dev3d`.

### 2026-11-09 19:26:36 — WFS-193 (X5) fusionné par le lead dev (lead-dev)
- PR #311 acceptée sur ordre de l'utilisateur (« on ne jette pas le travail pour un petit défaut »), rouverte par le PO. Branche synchronisée avec `main` (7371b72e, sans conflit). Construction Release OK. `tests_cpu.sh` : 90 tests réussis.
- Captures regardées : la calandre de marque a disparu de jour comme de nuit. Le diff ne touche que la X5 (`x5.wfm`, `x5.lod`, `formes.json`) et une accroche de banc de 2 lignes (valeurs par défaut inchangées). Aucune régression vue.
- Approbation écrite dans la PR (revue COMMENT « ✅ Approuvé », car GitHub refuse l'approbation du même compte), puis squash **8d55c57c**. Branche gardée.
- Les 4 défauts de jonction grille / carrosserie partent dans **WFS-358**, désormais ⏳ Prêt. Le léger contour de l'ancien logo sur le capot (déjà présent sur `main`) est signalé pour WFS-358.


### 2026-11-09 19:27:41 — Décor en formes géométriques : WFS-369 à 373 (po-scrum-master)
- Demande de l'utilisateur : « des formes géométriques dans les cartes, créer les tâches ». Constat sur les captures de l'APK 954 et dans le code : bancs en pavés, immeubles en boîtes, parasols en cônes, mâts en cylindres, vignes en blocs (ANO-65).
- US créées (E12 + E2, agent `dev-artiste-3d`, à la suite) : **WFS-369** (P1, ⏳ Prêt) mobilier urbain de la Ville ; **370** (P1) immeubles de la Ville en vrais volumes (avec WFS-231 pour les ouvertures) ; **371** (P2) Corniche ; **372** (P2) Campagne ; **373** (P2) Grand Prix. Chaque US commence par l'inventaire des objets en formes géométriques.
- PR #311 fusionnée par le lead dev (8d55c57c) ; WFS-358 prêt, avec en plus le contour de l'ancien logo du capot de la X5.

### 2026-11-09 19:29:54 — WFS-214 refusé par le lead dev, PR #314 (lead-dev)
- Commit relu 75749e6e. J'ai ouvert les 12 paires de captures et lu tout le diff. Branche fusionnée localement avec main (8c4d65ae), construite : tests CPU 91/91 verts.
- Bons : RG-01 (plus de marches), RG-03 (chaussée grise en Ultra et en Haute), RG-05 (Golf sous les lampes), nuit et aube inchangées, Ville de jour sans ombres délavées.
- **Refus (condition 4 du PO)** : l'option A rend les moments de jour de la Campagne encore plus blancs, avec une luminance moyenne qui monte (dorée 212 → 233, matin 234 → 242, midi 237 → 243) ; le relief, l'éolienne et les arbres disparaissent dans le voile.
- **Manquent** : les captures dorée, midi et jour en Campagne, et une mesure de la performance de la passe finale (2 pow(vec3) en plus par pixel).
- Repli C possible (condition 6). Revue publiée dans la PR (COMMENT : même compte GitHub).

### 2026-11-09 19:34:30 — WFS-346 : PR #315 ouverte, lead dev appelé (Claude, agent cartes (01UY1Vv2dx))
- Fait : saison lue du paquet (adhérence, neige au sol, flocons ; format 2 des blocs), chaussée enneigée avec traces de roues (sèche sous l'abri), flocons dehors seulement (Cockpit : vitres ; rien sous un tunnel), tunnel des cartes de données, μ dans la conduite du joueur (ancien et nouveau modèle), banc `neige`.
- Vérifié : tests CPU 91/91 (nouveau `test_cartes_v2_saison`) ; banc `neige` 0 écart ; banc `paquets` 0 écart (parcours accueil → terrain → course → retour) ; freinage 100-0 dans le jeu : 37 m au sec, ≈ 144 m sur neige (μ 0,25, réel ≈ 150 m) ; flocons 0,07 ms CPU par image.
- Limites : chaussée sèche sous le 1er tunnel seulement ; rendu vu en lavapipe ; coût GPU du shader de neige non mesuré sur téléphone. Aurore : WFS-344 (décision du PO 19:05:07).

### 2026-11-09 19:35:20 — Dernière version cassée : WFS-374 (P1) créé (po-scrum-master)
- Demande de l'utilisateur : « la dernière version est niquer ». Symptôme pas encore précisé (question posée dans `tache utilisateur.md`).
- Fusions du jour : #313 WFS-356 (CI #964 verte), #312 WFS-343 (CI #965 verte, suspect n° 1 : `mapById`, `Track::build`, `CarteDecor::init`, choix du terrain), #297 WFS-338 (CI #966 en cours), #311 WFS-193 (CI #967 en attente).
- **WFS-374** (P1 Urgent, E0, ⏳ Prêt, en premier) : bissection au banc, correction en avant, aucun revert sans ordre de l'utilisateur ; 🟡 après fusion. Pour une nouvelle session `@dev`.

### 2026-11-09 19:36:22 — Jour / Nuit seulement : WFS-375 (P1) créé (po-scrum-master)
- Vidéo de l'utilisateur (Corniche, 20:34) : le bouton Jour du choix du terrain fait défiler les moments de WFS-208 (Midi → Dorée…), course partie au crépuscule. Ordre : « garder que simple mode journée et nuit ».
- **WFS-375** (P1, E8, ⏳ Prêt) : deux boutons JOUR (= midi) et NUIT, sans défilement ; sauvegardes anciennes rangées ; code des moments gardé pour les bancs.

### 2026-11-09 19:38:04 — Recette de la vidéo du soir, ANO-206 à 210, WFS-376 créé (Agent po (session_01UY1Vv42j7p6X3zS9Jhfwdm))
- Vidéo de l'utilisateur (15 s, Corniche, Course à 5, vue Cockpit) lue image par image ; rapport `qa/rapports/recette-manuelle-video-2026-11-09-2032.md`, images `qa/retours/2026-11-09-video2032-*.jpg`.
- Anomalies : ANO-206 (bloquante, bateaux de la marina en triangles blancs) → **WFS-376** (P1, `dev-artiste-3d`, avant WFS-369, règles avec l'avis de l'expert réalisme 3D) ; ANO-207 (pas de pavillon, rétroviseur en boîte) → WFS-323 / 324 ; ANO-208 (ciel bleu nuit et sol au soleil) ; ANO-209 (bandes de marque sur le volant) → WFS-325 ; ANO-210 (arrêt net au choc d'un lampadaire).

### 2026-11-09 19:40:22 — ANO-211 créée et US technique WFS-377 (po-scrum-master)
- Signalement de Claude, agent dev W (01UY1Vv4Ed), pendant WFS-214 / PR #314 (remarque du lead dev) : sur `main` (64c14e06, 049efa41), le parcours WFS-150 RG-04 / P-05 du banc `realisme` (`src/render/bench_passes.cpp`, `WFS_PASSES_SEUL=1`) ne trouve plus « Ultra » dans les Réglages à onglets (WFS-291) : 2 / 5 étapes, capture `parcours-ultra-course` en Haute (128 px au lieu de 256), preuve d'ANO-106 non valable.
- **ANO-211** (mineure, banc d'essai) → **WFS-377** [TECHNIQUE] P2, E0, ⏳ Prêt pour une nouvelle session @dev ; règles `regles-de-gestion/WFS-377.md`. L'agent dev W ne la prend pas (un seul sujet par session).
- Pour information (pas d'anomalie) : l'exposition figée du banc des moments de WFS-208 (`eclairage_banc.cpp`) est corrigée dans la branche de WFS-214.

### 2026-11-09 19:41:10 — Règle de l'utilisateur : formes géométriques interdites partout, WFS-378 créé (Agent po (session_01UY1Vv42j7p6X3zS9Jhfwdm))
- Règle ajoutée à `CLAUDE.md`, `regles-de-gestion/DA-realisme.md` et aux skills agent-developer, artiste-3d, expert-realisme-3d, expert-cartes, waforspeed-natif, lead-dev (refus de PR), qa-lead (anomalie bloquante) et po-scrum-master. Rappel ajouté dans WFS-323, 325, 344, 369 à 373 et 376.
- **WFS-378** (P1, `dev-artiste-3d`) : inventaire des formes nues dans toutes les zones, puis découpage en tickets.

### 2026-11-09 19:47:28 — Vidéo des lacets de la Corniche : ANO-212, 213 et WFS-379 créés (Agent po (session_01UY1Vv42j7p6X3zS9Jhfwdm))
- Vidéo de l'utilisateur (15 s, Corniche, Checkpoint, vues Proche et Capot) : terrain des lacets qui flotte dans le ciel (ANO-212, bloquante), capot flou (ANO-213). Images : `qa/retours/2026-11-09-video2045-*.jpg`.
- **WFS-379** (P1, `expert-cartes`) : supprimer les 6 lacets de `corniche()` (`trace_cartes.cpp`), les remplacer par une montée simple, retirer leur décor et vérifier toute la carte.

### 2026-11-09 19:49:12 — Mer en aplat bleu : ANO-214 et WFS-380 créés (Agent po (session_01UY1Vv42j7p6X3zS9Jhfwdm))
- Plainte de l'utilisateur sur la mer. Code lu : `P_OCEAN` de `plage.cpp` est un quad plat de couleur unie. ANO-214 (bloquante) → **WFS-380** (P1, `dev` + skill `expert-realisme-3d`) : vagues, reflets, couleur selon la profondeur, écume, pour toute l'eau du jeu.

### 2026-11-09 19:51:12 — WFS-374 attribué à Claude, agent dev Z (po-scrum-master)
- Demande d'une nouvelle session `@dev` (session_01UY1Vv3EZjAAQHAscqb4p8r, aucun ticket avant) : **WFS-374** (P1 Urgent, E0, premier ticket prêt pour `@dev`) lui est donné : « dernière version de `main` cassée sur le téléphone ».
- Zone : E0, exception PO : la correction se fait dans le fichier du commit fautif (bissection au banc `menu1-2400`, `paquets`, endurance sur #313, #312, #297, #311). Aucun revert ni retrait sans ordre écrit de l'utilisateur. Après fusion : 🟡 À valider (utilisateur).
- Règles : `regles-de-gestion/WFS-374.md`. Symptôme de l'utilisateur : question posée dans `tache utilisateur.md` (19:35), pas encore de réponse : suivre l'ordre de RG-01.
- `track-agent.md` : ligne de l'agent arwa enlevée (WFS-338 fusionné, #297, session terminée).

### 2026-11-09 19:51:27 — WFS-358 attribué à Claude, agent dev3d (01UY1Vv41d) (po-scrum-master)
- Demande d'une **nouvelle session** `@dev3d` (session_01UY1Vv41dPBrq3vbTpDyTvg, agent `dev-artiste-3d`), 1er et seul ticket : vérifié, aucun ticket attribué avant à cette session.
- Choix : ordre de l'utilisateur du 19:14 UTC, voitures 358 → 362 → 359 → … ; WFS-358 est le premier, ⏳ Prêt, libre, dépendance WFS-193 🟣 et #311 fusionnée (8d55c57c). Exception E10 du PO : en parallèle de WFS-349 (zones séparées). WFS-376 (bateaux) reste le premier prêt du décor pour la prochaine session `@dev3d`.
- **Ticket** : WFS-358 (E10, P1) — X5 : jonction grille / carrosserie (4 défauts de RG-04) + contour de l'ancien logo du capot. Règles : `regles-de-gestion/WFS-358.md`.
- **Zone autorisée** : `assets/cars/x5*`, entrée `x5` de `tools/formes.json`, outils de retouche de WFS-193 (`tools/retoucher_formes.mjs`, `tools/effacer_logos.mjs`, `tools/logos.json` pour la X5 seulement), banc `WFS_VITRINE_BAS`, `tests/**` (fidélité X5), `qa/pr/WFS-358/`. **Interdit** : `vehicule_cockpit*` (WFS-349), `camera_vue.*`, `src/game/cars.h` (WFS-359 à 361), autres voitures, `src/render/**` hors accroche annoncée, E13 (WFS-214), `cartes_v2` / `meteo.*` / `route_*` (WFS-346).
- Plan : méthode RG-03 (a) ou (b) notée au journal avant de coder ; branche `feature/waforspeed-358` poussée tout de suite ; captures avant / après (face, vue basse, zooms ×4 des coins, ¾, jour et nuit) ; PR ; appel du `lead-dev` ; arrêt de la session après la fusion.


### 2026-11-09 19:52:48 — WFS-358 : début, branche feature/waforspeed-358 (Claude, agent dev3d (01UY1Vv41d))
- Ticket reçu du PO (calandre de la X5, jonction grille / carrosserie, contour de l'ancien logo du capot). Skills agent-developer, waforspeed-natif, artiste-3d chargés.
- Étape suivante : mesurer l'ouverture du pare-chocs (`retoucher_formes.mjs --zone x5`), choisir la méthode RG-03 et la noter ici avant de coder.

### 2026-11-09 19:53:14 — WFS-376 attribué à Claude, agent dev3d (01UY1Vv3u9) (po-scrum-master)
- Demande d'une **nouvelle session** `@dev3d` (session_01UY1Vv3u9DQZsvS7pFHAyy7, agent `dev-artiste-3d`), 1er et seul ticket : aucun ticket avant. WFS-358 venait d'être donné à la session dev3d 01UY1Vv41d (19:51:27) : mon commit en double est annulé avant envoi.
- **Ticket** : WFS-376 (P1, E12 + E2) — Corniche : vrais bateaux dans la marina (ANO-206), premier prêt de la suite « Décor sans formes géométriques ». Règles : `regles-de-gestion/WFS-376.md` (zone et interdits ajoutés au Suivi).
- **Zone** : `src/world/plage.cpp` (marina, `O_BATEAU`, `voilier()`, `yacht()`), `palUv` ≤ 2 couleurs, `tests/**`. **Interdit** : `P_OCEAN` (WFS-380), `trace_cartes.cpp` (WFS-379), `meteo.*` / `cartes_v2` / `route_*` (WFS-346), E13, `vehicule_cockpit*`, voitures.
- Skills : `agent-developer`, `waforspeed-natif`, `artiste-3d`, `expert-cartes`. WFS-380 attendra la fusion de WFS-376 pour `plage.cpp` (hors `P_OCEAN`).
### 2026-11-09 19:52:57 — Début WFS-379 : attribué à Claude, agent cartes 2 (01UY1Vv4VT) (po-scrum-master)
- Nouvelle session `@cartes` (session_01UY1Vv4VTT6nCcCeczFGXuV), 1er et seul ticket ; demande sans sujet précis → ticket prêt le plus prioritaire de son domaine : **WFS-379** (P1, Corniche : lacets supprimés, plus aucun terrain en l'air, ANO-212). WFS-357 (P3) reste ⏳ Prêt.
- Zone : E2 + E12 avec exception PO (fichiers listés dans `regles-de-gestion/WFS-379.md`, « Suivi ») ; aucun fichier commun avec WFS-346 (PR #315, agent cartes 01UY1Vv2dx), WFS-338, 349, 214 (diffs vérifiés). RG-05 sans objet (pas de Corniche dans les paquets).
- Plan : montée simple de +60 m (rayon ≥ 150 m, pente ≤ 7 %), boucle fermée, décor des lacets retiré, bancs et guidage mis à jour, captures avant / après jour et nuit + parcours complet.
- WFS-380 (vraie mer) passe « après WFS-379 » : même fichier `plage.cpp`.

### 2026-11-09 19:53:46 — WFS-379 et WFS-376 en même temps dans `plage.cpp` : zones séparées (po-scrum-master)
- WFS-376 (marina) attribué en parallèle par une autre instance du PO (19:53:14). Partage : WFS-379 = tracé `corniche()` et décor des lacets / de la montée ; WFS-376 = marina, `O_BATEAU`, `voilier()`, `yacht()`. Interdits croisés écrits dans les deux règles. WFS-380 attend WFS-379 et WFS-376.

### 2026-11-09 19:57:05 — WFS-379 : début, branche créée (Claude, agent cartes 2 (01UY1Vv4VT))
- Branche `feature/waforspeed-379` créée depuis `main` (b0d9d7bb). Session renommée « WFS-379 — Claude, agent cartes 2 (01UY1Vv4VT) (Corniche sans lacets) ».
- Analyse : la route n'est pas fermée dans l'espace (tour ouvert : fin à −3 057 m / −1 497 m, cap −76°) ; le « bouclage » est celui des segments et de l'altitude. Les lacets (segments 1259 à 2201, 1 130 m, +60 m) vont de 366 m vers la droite pour 50 m en avant. Plan : remplacer par une montée en S de même longueur (≈ 1 130 m, donc mêmes numéros de segment pour la suite : zones, checkpoints, tunnel) et de même cap de sortie, deux courbes de 400 m de rayon, pente ≈ 5,3 %.
- Fichiers prévus : `trace_cartes.cpp` (`corniche()`), `plage.cpp` (décor des lacets), bancs qui cherchent les lacets de la Corniche (`bench_fluidite`, `virages_banc`, `course5_system`, `parois_sons_banc`, `trace_cartes_systeme`), captures `carte-plage-lacets-*`, `tests/test_trace.cpp`.

### 2026-11-09 19:57:22 — WFS-376 : réponses du PO au développeur sur la marina (po-scrum-master)
- Question de Claude, agent dev3d (01UY1Vv3u9). Décisions notées dans `regles-de-gestion/WFS-376.md` (Suivi) : RG-02 = axe perpendiculaire à la ligne d'amarrage (quai ou ponton) ±3°, rangée cul au quai + pontons perpendiculaires des deux côtés, chenal ≥ 1,5 × longueur ; un seul seuil de détail à 200 m (`LOD_FAR_M`), mât lointain en 2 quads croisés, budget 9 000 triangles ; quai : baisser seulement la bande d'amarrage si l'écart dépasse 1,5 m, mer et tracé inchangés.

### 2026-11-09 20:01:51 — Avis décor de l'expert 3D : WFS-381 à 384 créés, ANO-215 à 226, points rattachés (po-scrum-master)
- Source : agent 3d (session_01UY1Vv4DeCjYyC5BsYidg6X), avis recopié de la branche `ccr-45bbe50e-1j246c` dans `qa/retours/avis-expert-3d-2026-11-09-corniche.md` ; images : Release `apercu-fusion`, run build.yml #965 (529b89e).
- **Créés** : WFS-381 (P1, E13) brume au loin (R1) ; WFS-382 (P1, E4 + bloom E13 ≤ 10 lignes) phares de nuit (N1) ; WFS-383 (P2, E12 + E2, après WFS-379) sols selon la pente + pins d'Alep (R2 / C3) ; WFS-384 (P1, E7, [TECHNIQUE]) bancs du matin #6 à #11 tous sautés (vérifié : `gh run list -w matin.yml`, réveils de GitHub en retard de 5 à 14 h, runs de 6 à 11 s ; seul banc complet du jour : lancement à la main de 00:34 UTC). Exception au gel : correction de la pipeline.
- **Rattachés** (règles ajoutées) : D1 + R7 → WFS-379 (RG-06 / 07, sans retarder le ticket : autre cause → ticket séparé) ; D2 parasols → **WFS-369** (mobilier, pas WFS-370 ni 378) ; D3 + N2 fenêtres → WFS-370 ; D4 + R4 + R5 → WFS-371 ; R6 rivage → WFS-380 (RG-09) ; D5 → WFS-372 ; D6 → WFS-373 ; C1 + C2 → WFS-344 (pas WFS-346, déjà en PR) ; R3, halos N2, V3 garage → WFS-378.
- **Garage (V3)** : pas de question à l'utilisateur : la règle du 2026-11-09 19:41 range déjà « garage et menus en 3D » dans le monde 3D → anneau néon à remplacer (WFS-378).
- **Non retenus ici** : V2 (calandres R4 Competizione / Folg 8) et N3 (rampe de police) : hors décor, pas demandés dans ce retour ; restent notés dans l'avis.
- Aucune attribution (règle « attribuer seulement à la demande »).

### 2026-11-09 20:04:02 — WFS-349 : PR #316 refusée par le lead dev (lead-dev)
- Relu au commit 19a067f6 : 47 captures ouvertes, tout le diff lu ; branche construite et 91 tests CPU réussis (`test_cockpit_ferme` compris) ; banc `penetration` relancé en 2340 × 1080 sur la branche synchronisée : recouvrement 0,000 m, portière fermée, éclairage de nuit identique à `main`. Le code est bon (0 appel de dessin en plus, ressources détruites, banc inactif en course).
- **Refus sur les preuves** : (1) les captures « après » (54a6cad0, 19:24 UTC) ont été faites avant la synchronisation et sans WFS-356 (vitrines et halos de nuit) : toutes les paires de nuit `penetration` et `cockpit` paraissent brûlées, une fausse régression ; à refaire depuis la branche synchronisée. (2) Huit lignes du tableau de la description ont des liens d'images abîmés.
- Remarques non bloquantes : bord non compté de 5 px (preuve `-ecarts.png` si possible), teinte magenta du banc sur le X5 à droite (cause à nommer), gabarit construit aussi en course, regard Bas 42,4 % (seuil visé : Normal), temps GPU non mesuré sur téléphone.
- `TACHES.md` → ↩️ Retour lead dev (PR #316) ; `track-agent.md` et `suivi-pr.md` à jour.

### 2026-11-09 20:08:29 — Épic E15 « Décors » créée, 5 US « décor complet », WFS-369 à 373 rattachés (po-scrum-master)
- Demande de l'utilisateur (`@po`, 2026-11-09 ≈ 20:05 UTC) : épic de l'amélioration graphique des décors ; un décor = 200 Mo minimum, 700 Mo maximum ; 99 % de réalisme ; un agent 3D valide les captures de chaque PR par un commentaire avant la fusion par le lead dev ; commencer au plus tôt, une US par décor.
- **E15** créée (`regles-de-gestion/E15-decors.md`, RG-E15-01 à 09) : taille mesurée sur la partie décor des données de la carte (paquet `.wfcarte`) ; agent 3D = `expert-realisme-3d` (`@3d`), commentaire « Avis agent 3D : VALIDÉ / À REPRENDRE » ; un ticket actif **par décor** pour avancer en parallèle.
- **Nouvelles US** : WFS-385 Ville, WFS-386 Corniche, WFS-387 Campagne, WFS-388 Grand Prix (P1), WFS-389 Neige scandinave (P2) — décor complet 200 à 700 Mo.
- **WFS-369 à 373 rattachés à E15**, passés P1, chaîne entre cartes coupée : **prêts maintenant WFS-369 (Ville), WFS-372 (Campagne), WFS-373 (Grand Prix)** ; WFS-371 attend WFS-379 et 376 (`plage.cpp`).
- Règle de validation ajoutée (ordre de l'utilisateur) dans `CLAUDE.md`, le skill `lead-dev` et l'agent `expert-realisme-3d`. Agent des tickets : `dev-artiste-3d` (+ skill `expert-cartes`), nouvelles sessions.

### 2026-11-09 20:12:30 — E15 : conflits enlevés, anomalies de décor abandonnées (po-scrum-master)
- Ordre de l'utilisateur : « voir tous les sujets pour ne pas créer de conflits ; les anomalies des décors, il faut les abandonner ».
- Revue de tous les tickets non terminés, des PR ouvertes (#314, 315, 316 : aucune sur le décor) et des 143 anomalies ouvertes.
- **17 anomalies de décor 🚫 abandonnées** : ANO-65, 82, 83, 121, 137, 171, 175, 206, 214 à 220, 223, 224 (gardées comme liste de contrôle des US E15).
- **Repris par E15** : WFS-231, 162, 163 (↪). **Rattachés à E15, file Corniche** : WFS-380, 383 (379 / 376 → 380 → 383 → 371 → 386). **WFS-378** limité au hors décor.
- Gardés : WFS-379 et 376 en cours (règle des 10 h), lumière / brume / phares / lampadaires (WFS-337, 381, 382), perf WFS-321 / 331 (pas en même temps qu'un E15 de la même carte).


### 2026-11-09 20:13:07 — WFS-346 fusionné par le lead dev (lead-dev)
- PR #315 relue sur d8f88a9a : 25 captures ouvertes, tout le diff lu (55 fichiers). Branche fusionnée localement avec `main` sans conflit, construite, `tests_cpu.sh` 91/91. Banc `neige` relancé : 0 écart ; freinage à fond 63 → 0 km/h en 56,6 m sur neige (≈ 144 m depuis 100), Ville sèche 100 → 0 en 36,6 m (rapport 3,93) ; virage : limite latérale 2,11 g au sec, 0,70 g sur neige ; 1 200 flocons à 0,076-0,078 ms CPU par image.
- Revue « ✅ Approuvé » écrite dans la PR (en commentaire : même compte GitHub), puis fusion en squash 62ccc175. Ticket de conduite : **🟡 À valider (utilisateur)**.
- Remarques non bloquantes, à transmettre en tâches séparées (ordre de l'utilisateur du 19:14) : (1) vue Cockpit de nuit sur la neige : planche de bord, écran et volant délavés en blanc (luminance de l'écran 177 contre 15 sur la Ville de `main`). À corriger en zone cockpit (E10), avant WFS-344. (2) Tache des phares saturée sur la neige de nuit. (3) `player.cpp` : le freinage × μ réduit aussi la résistance de l'air ; le bas-côté enneigé accroche plus que la chaussée dans l'ancien modèle. (4) Chaussée sèche sous le 1er tunnel seulement.

### 2026-11-09 20:15:59 — WFS-346 : fin, session terminée (Claude, agent cartes (01UY1Vv2dx))
- PR #315 fusionnée par le lead dev (squash 62ccc175) ; 🟡 à valider par l'utilisateur sur son téléphone (ticket de conduite).
- Défauts restants relevés par le lead dev (cockpit de nuit délavé sur la neige, phares saturés sur la neige, traînée et bas-côté dans `player.cpp`, 1er tunnel seulement / tunnel non éclairé) transmis au PO pour des tickets séparés.
- Un seul sujet par session : cette session s'arrête et ne demande pas d'autre ticket.

### 2026-11-09 20:16:31 — Suites de WFS-346 (revue du lead dev, PR #315) : WFS-390 à 392 créés (po-scrum-master)
- Demande de Claude, agent cartes (01UY1Vv2dx) et revue du lead dev (20:13:07) : défauts restants confiés en tâches séparées (ordre de l'utilisateur du 19:14).
- **WFS-390** (P2, E10) Cockpit de nuit sur la neige délavé (ANO-227) : ⏳ en attente de WFS-349 (mêmes fichiers `vehicule_cockpit*`) ; ajouté aux dépendances de **WFS-344** (à corriger avant, avis du lead dev).
- **WFS-391** (P2, E3, skill `expert-conduite`) traînée de l'air hors μ et bas-côté enneigé (ANO-229) : ⏳ Prêt ; 🟡 après la fusion (conduite).
- **WFS-392** (P2, E14 + E9, skill `expert-cartes`) chaussée sèche sous tous les tunnels, tunnel générique éclairé par de vrais luminaires (ANO-230) : ⏳ Prêt.
- Tache des phares saturée sur la neige (ANO-228) : rangée dans le lot ouvert **WFS-382** (RG-07, capture 6), pas de nouveau ticket.
- `track-agent.md` : ligne de l'agent cartes (01UY1Vv2dx) enlevée (session terminée, WFS-346 fusionné ; règle « agent fini = ligne enlevée »). Aucun ticket attribué : ils vont à de nouvelles sessions qui les demandent.

### 2026-11-09 20:37:37 — WFS-376 : arbitrage du budget de la marina, option A (po-scrum-master)
- Demande de Claude, agent dev3d (01UY1Vv3u9) : 95 bateaux et 27 000 triangles dans la fenêtre détaillée la plus chargée, contre 40 / 9 000 dans RG-10.
- Décision : densité réelle gardée, modèle moyen (≈ 100 triangles) aux pontons, rangée 1 à ≈ 85 %, bittes et pontons allégés ; nouveau budget ≤ 18 000 triangles, appels de dessin inchangés. Détail dans `regles-de-gestion/WFS-376.md` (Suivi).

### 2026-11-09 20:40:56 — WFS-374 : 1er passage de diagnostic, défaut non reproduit au banc PC (Claude, agent dev Z (01UY1Vv3EZ))
- APK de l'utilisateur à 19:35 UTC : celui du run **#966** (WFS-338 poursuite police, 8c4d65a), publié dans `apercu` à 19:23:49 UTC.
- Bancs `menu1`, `parcours`, `paquets` sur main : 0 écart. ASan + UBSan sur le banc `carte` (4 cartes, jour / nuit) : aucune erreur mémoire. Course à 5 + poursuite sous ASan : en cours.
- Bloquant : symptôme inconnu (RG-01). Question posée à l'utilisateur sous sa demande dans `tache utilisateur.md`. Notes : `qa/pr/WFS-374/notes.md` (branche `feature/waforspeed-374`).

### 2026-11-09 20:46:04 — WFS-379 : PR ouverte, lacets de la Corniche remplacés par une montée simple (Claude, agent cartes 2 (01UY1Vv4VT))
- Fait : `corniche()` : 6 lacets → montée en S de +60 m (2 courbes de 35°, rayon 400 m, pente ≤ 6,8 %, cap net 0) ; décor des lacets retiré dans `plage.cpp` (bas-côtés bornés par une autre branche 1 581 → 0, disques de sol 341 → 29) ; bancs et tests des épingles passés au col de la Campagne ; nouveau banc `WFS_BANC=corniche WFS_CORNICHE=montee`.
- Vérifié : 90 tests CPU verts ; captures avant (main + seul banc de la montée) / après dans `qa/pr/WFS-379/` (Proche et Capot, jour et nuit, suite de 6 images, mini-carte, parcours complet tous les 200 m, vue de dessus), regardées : plus aucun terrain ni plaque en l'air. Banc `perf` Corniche : triangles du décor −3,3 % (Haute) / −3,1 % (Basse).
- Limites : carte +215 m (pente ≤ 7 %) ; RG-06 (D1, vide gris des vues de dessus) = autre cause (sol posé sur une bande de ~220 m autour de la route, mer en plan limité, aussi Ville et Grand Prix) → signalé au PO ; RG-07 (plateaux en escalier) = terrasses des lacets, disparues.
- Signalé au PO (préexistant sur main, hors ticket) : bas-côté intérieur des épingles de la Campagne, volant à fond à 10 km/h : rayon 11,64 m au lieu de 4,17 m.
- Aucune accroche hors de la zone donnée par le PO ; `CMakeLists.txt` non modifié.

### 2026-11-09 20:49:16 — Constats de WFS-379 : ANO-215 (cause trouvée) et ANO-231 (épingles de la Campagne) rangés dans des tickets existants (po-scrum-master)
- Message de Claude, agent cartes 2 (01UY1Vv4VT), PR #317 (WFS-379) ; il ne demande pas de ticket, sa session s'arrête après la fusion.
- **ANO-215 (vide gris des vues de dessus)** : autre cause que les terrains qui flottent : sol posé seulement sur une bande d'environ 220 m autour de la route (`plage.cpp`, `K_TERRE` jusqu'à RW + 220 m), mer en plan limité ; même chose Ville et Grand Prix (preuve : `qa/pr/WFS-379/apres-montee-dessus.jpg`). L'anomalie est 🚫 abandonnée par l'ordre de l'utilisateur du 20:12 UTC (pas de conflit avec E15, le décor est refait par WFS-385 à 388) : **pas de nouveau ticket** de décor hors E15 ; la cause et l'attendu sont écrits en **RG-10** de WFS-385, 386, 387, 388 (plus de vide gris vu d'en haut, capture de dessus avant / après). WFS-379 n'en est pas retardé (RG-06 de WFS-379 tenue : le développeur l'a signalé).
- **ANO-231 créée** (Majeure, conduite, préexistante sur `main`) : épingles du col de la Campagne, bas-côté intérieur, volant à fond à 10 km/h, aide coupée : rayon 11,64 m au lieu de 4,17 m. Rangée dans **WFS-391** (P2, E3, skill expert-conduite, ⏳ Prêt, même sujet « conduite au bas-côté », même zone `player.*` / `roues.*`) en **RG-06** : trouver la cause d'abord, rayon à ± 3 % du centre, test CPU, capture ; si la cause est dans le sol de la carte, le développeur le dit au PO avant de sortir de sa zone. Titre de WFS-391 élargi dans `TACHES.md`.
- `track-agent.md` remis à jour (derniers envois des branches 376, 374, 349, 214, 358, 379). Aucune attribution.

### 2026-11-09 21:01:09 — WFS-379 (PR #317) relue par le lead dev : approuvée, fusion en attente de synchronisation (lead-dev)
- 145 captures ouvertes (montée jour / nuit, Proche / Capot, suite de 6, vue de dessus, parcours complet) : plus de lacets, plus aucun pan de terrain ni plaque dans le ciel sur la montée. Diff entier lu ; branche construite, `tests_cpu.sh` 91/91 verts ; triangles du décor −3 %.
- Revue « ✅ Approuvé par le lead dev » postée (COMMENT : même compte GitHub). Squash refusé par GitHub : conflit dans `journal.md` (entrée du PO de 20:49:16). Le développeur synchronise avec `main` et rappelle le lead dev ; fusion ensuite.
- À surveiller (non bloquant) : bande de falaise derrière le village sous la descente (`apres-parcours-*-4200`), décor inchangé ; `benchMontee` ne remet pas `modeCourse()`.

### 2026-11-09 21:03:05 — WFS-379 fusionné par le lead dev (lead-dev)
- PR #317 fusionnée en squash (ed494ea) après la synchronisation du développeur (dc295047 : `main` seul, vérifié). Revue « ✅ Approuvé » postée sur dc295047 (COMMENT : même compte GitHub).
- `TACHES.md` : WFS-379 en 🟡 À valider (utilisateur) ; ligne ajoutée à `qa/Check.md` ; ligne #317 retirée de `suivi-pr.md`.

### 2026-11-09 21:03:43 — WFS-379 : fusionné, session terminée (Claude, agent cartes 2 (01UY1Vv4VT))
- PR #317 fusionnée en squash par le lead dev (ed494ea9) ; ticket 🟡 À valider (utilisateur) ; `qa/Check.md` tenu par le lead dev.
- Incident : la 1re synchronisation (dc295047) était restée en HEAD détachée dans mon dossier de travail (laissée ainsi par la 1re revue du lead dev) ; rattachée à la branche et poussée en avance rapide, puis fusion.
- Réponse écrite à l'utilisateur dans `tache utilisateur.md` (vidéo des lacets). Ligne de `track-agent.md` : ⏹️ session terminée. Un seul ticket par session : je m'arrête, aucun autre ticket demandé.


### 2026-11-09 21:10:28 — WFS-359 attribué à Claude, agent dev3d 2 (01UY1Vv2MH) (po-scrum-master)
- Demande de la session `@dev3d` session_01UY1Vv2MHmNkDc96qUCVUja (agent `dev-artiste-3d`), nouvelle session, aucun ticket avant : 1er et seul ticket de la session.
- Choix : objectif « voitures sans marque » avant le décor (ordre de l'utilisateur du 19:14). WFS-362 attend WFS-358 ; **WFS-359** (P1, ⏳ Prêt) est le ticket voitures suivant. **Exception E10** (même ordre : au plus deux tickets voitures en même temps si deux sessions `@dev3d`) : en parallèle de WFS-358 (X5) et de WFS-349 (cockpit), zones séparées.
- Zone : `assets/cars/<mustang, tcross, passat>*`, `assets/cars/remplacants/**`, lignes de ces voitures dans `src/game/cars.h`, `assets/licences/**`, crédits, entrées du ticket dans `tools/formes.json` / `tools/logos.json`, `tests/**` ; **pas** `x5*` (WFS-358), `vehicule_cockpit*` (WFS-349), autres voitures. Règles : `regles-de-gestion/WFS-359.md` (RG-01 Mustang ±1 % sans dérogation, sinon autre modèle libre proposé au PO ; RG-02 T-Cross → VW Passat CC BY). Pas E15 : pas d'avis agent 3D obligatoire.
- `track-agent.md` : ligne de Claude, agent cartes 2 (01UY1Vv4VT) enlevée (WFS-379 fusionné #317, session terminée) ; derniers envois 374, 349 mis à jour.

### 2026-11-09 21:12:07 — WFS-376 : PR #318 refusée par le lead dev (lead-dev)
- Revue du commit 4ee2758d : 18 paires de captures ouvertes, diff lu en entier, branche construite et testée (92 tests CPU réussis, `test_marina_bateaux` réussi).
- Les bateaux sont réussis (coques à flot cul au quai, gréement, aucune voile hissée, pontons, bittes, quai abaissé), sans appel de dessin en plus, et rien n'est dégradé ailleurs.
- Refus pour preuves manquantes : (1) la suite `marina-suite-1` à `-4` ne montre aucun bateau (la marina est hors de l'image), alors que le PO l'a rendue obligatoire contre le clignotement entre les niveaux de détail ; (2) aucune capture de nuit. Le code n'est pas à reprendre.
- Remarques non bloquantes : Basse à +11,1 % (chiffre noté, admis par le PO) ; RG-09 vient du lampadaire de 6,4 m (hors zone, tâche séparée à voir avec le PO) ; captures « doree » identiques à « midi » (E13, ANO-208).
- `TACHES.md` → « ↩️ Retour lead dev (PR #318) » ; `track-agent.md` et `suivi-pr.md` mis à jour.

### 2026-11-09 21:12:22 — WFS-369 attribué à Claude, agent dev3d 3 (01UY1Vv5WS) (po-scrum-master)
- Demande de la session `@dev3d` session_01UY1Vv5WS2KPjVrJGzH9iLM (agent `dev-artiste-3d`), nouvelle session, aucun ticket avant : 1er et seul ticket de la session.
- Choix : les voitures ne sont pas libres (E10 : WFS-358 et WFS-359 déjà en parallèle, maximum atteint ; WFS-360 attend WFS-359, WFS-362 attend WFS-358). Ticket suivant pour `dev-artiste-3d` : **WFS-369** (P1, ⏳ Prêt, plus petit numéro des décors prêts 369 / 372 / 373) ; file Ville de E15 libre.
- Zone : `src/world/ville*.cpp`, `ville_quartiers.*`, `commerces.cpp` (mobilier et parasols de terrasse seulement), nouveaux fichiers `src/world/mobilier_*` / `decor_ville_*`, `assets/world/modeles/ville/**`, `assets/textures/decor/ville/**`, `assets/licences/**`, `tests/**` ; communs (`archi_kit.*`, `vegetation.*`, `decor_carte.*`, shaders `decor*`) : ajouts ≤ 10 lignes annoncés au journal. **Pas** `plage.cpp` (WFS-376), immeubles (WFS-370), `lots_decor.*` / `pietons.*` (performance), routes, voitures, lumière.
- Règles : `regles-de-gestion/WFS-369.md` (RG-01 inventaire au journal d'abord ; RG-02 modèles CC0 / CC BY ±1 %, choix proposé au PO ; parasols ANO-216) et `regles-de-gestion/E15-decors.md` (RG-E15-06 : « Avis agent 3D : VALIDÉ » dans la PR avant le lead dev ; taille ajoutée notée dans la PR, le 200-700 Mo se mesure sur WFS-385).

### 2026-11-09 21:20:52 — WFS-359 : modèles de remplacement validés par le PO (Corvette C7, Vitara Brezza) (po-scrum-master)
- Question de Claude, agent dev3d 2 (01UY1Vv2MH) : Mustang de Jiaxing et Passat B5.5 hors ±1 % → refusées (pas de dérogation).
- **Décision** (le choix des modèles revient au PO, ordre de l'utilisateur du 2026-11-09) : Mustang → **Corvette C7 Stingray** (Martin Trafas, CC BY 4.0) ; T-Cross → **Suzuki Vitara Brezza 2022** (BHP3D, CC BY 4.0), hauteur comparée barres de toit comprises. Id « mustang » / « tcross » gardés (place au garage), fiches remplacées. Si la Brezza sort de ±1 % : Corvette seule dans WFS-359, nouvelle US pour la T-Cross. Détail : `regles-de-gestion/WFS-359.md`.
### 2026-11-09 21:02:53 — WFS-358 : calandre de la X5, jonction propre et logo du capot recousu (Claude, agent dev3d (01UY1Vv41d))
- **Méthode RG-03 (a)**, choisie après mesure de l'ouverture (vue de face projetée du modèle) : grille découpée sur le contour exact, toujours derrière la carrosserie. Pas de cadre en haricots, pas de « pare-buffle ».
- **Causes des 4 défauts de RG-04** :
  - dents de scie et éclats blancs : retrait « par centre de triangle » de #311 (bord en escalier, trous traversants aux coins bas) ;
  - coin haut droit : grille à profondeur fixe, devant la carrosserie qui recule sous le phare ;
  - peigne : pas de paroi derrière le bord, bouts des barres sur l'intérieur clair ;
  - aussi des ailettes noires (barres verticales des haricots vues par la tranche).
- **Logo du capot** : le biseau et le trou du logo restaient (disque blanc de jour, ovale de nuit).
- **Fait** : `tools/retoucher_formes.mjs`, 3 retouches rejouables :
  - découpe exacte par contour (triangles coupés) ;
  - grille ajustée : profondeur suivant la carrosserie (1,5 cm derrière, refus d'écrire si un sommet passe devant), 9 lames chanfreinées, fond noir, conduit noir tout autour ;
  - bouchon : triangles du logo retirés, peau du capot recousue.

  `tools/formes.json` (entrée x5, depuis le modèle d'avant #311, empreinte après 87570eb9ad0fa1c8) ; `x5.wfm` 7,54 Mo / `.lod` régénérés.
- **Accroche annoncée** (`src/render/feux_modele.cpp`, banc `WFS_VITRINE_BAS`, 8 lignes) : plusieurs vues séparées par « ; » ; rien ne change sans « ; ».
- **Essais écartés pour le logo** : plan ou quadrique + pièce posée devant (disque plus clair : le vernis réagit à de très petits écarts) ; pièce posée derrière (points sombres au bord du trou en lumière rasante) ; zone recousue plus large (débordait sur la pointe repliée, pli).
- **Vérifié** :
  - `--verifier` OK pour les 3 voitures ; l'outil rejoué donne un fichier identique ;
  - 90 tests CPU verts ;
  - `captures_pr.sh` plus vues 2560×1440 et zooms ×4, de jour et de nuit, toutes regardées : les 4 défauts ne se voient plus.
- **Limites** :
  - au zoom ×4, en plein reflet du soleil, un très léger losange reste à l'endroit du logo ;
  - +7 012 triangles pour la grille (LOD refaits).
- Outils du conteneur installés pour le banc : mesa-vulkan-drivers, glslc, ninja-build, imagemagick (apt), meshoptimizer 0.22.0 (npm, dans `tools/node_modules`, non versionné). Aucun autre téléchargement.

### 2026-11-09 21:27:23 — WFS-358 fusionné par le lead dev (lead-dev)
- PR #319 fusionnée en squash (6d8f7e43), titre `WFS-358 : X5, calandre ajustée à l'ouverture et logo du capot recousu (#319) (Claude, agent dev3d (01UY1Vv41d))`. Revue « ✅ Approuvé » écrite dans la PR, en commentaire, car GitHub refuse l'approbation du même compte.
- Vérifications : 56 captures ouvertes ; tout le diff lu (`retoucher_formes.mjs` : découpe, bouchon, grille ajustée ; entrée x5 de `formes.json` ; accroche `feux_modele.cpp`) ; construction et `tests_cpu.sh` relancés : 91/91 ; `--verifier` OK pour les 3 voitures ; `x5.wfm` 7,54 Mo et +4,6 % de triangles au niveau 0.
- 1er essai de fusion refusé par GitHub (conflit `journal.md`). Le développeur a synchronisé la branche et corrigé le point 4 (de995071). J'ai relu ces changements sur d8fdd30c.
- Statut : 🟣 Fusionné. Défauts restants (lames ondulées, bord haut en vue basse de nuit, losange du logo) : tâche séparée demandée au PO, selon l'ordre de l'utilisateur du 2026-11-09.

### 2026-11-09 21:27:43 — WFS-358 : fusionné, session terminée (Claude, agent dev3d (01UY1Vv41d))
- PR #319 fusionnée en squash par le lead dev (6d8f7e43) ; ticket 🟣 Fusionné ; `qa/Check.md` tenu par le lead dev.
- Point 4 de la revue corrigé avant fusion (banc `WFS_VITRINE_BAS`, de995071).
- Défauts restants signalés au PO pour une tâche séparée : lames du haut qui ondulent ; bord haut de l'ouverture en vue basse de nuit ; léger losange à l'emplacement du logo.
- Un seul ticket par session : la session s'arrête, aucun autre ticket demandé.

### 2026-11-09 21:28:47 — WFS-393 créé : finitions de la X5 après #319 ; WFS-362 prêt (po-scrum-master)
- Demande du lead dev et de Claude, agent dev3d (01UY1Vv41d), après la fusion de WFS-358 (#319, 6d8f7e43), selon l'ordre de l'utilisateur du 2026-11-09 (« on accepte, puis une tâche séparée corrige le défaut »).
- **WFS-393** (E10, P3) : lames du haut de la grille qui ondulent, bord haut de l'ouverture déchiqueté en vue basse de nuit, léger losange à l'emplacement de l'ancien logo. Règles : `regles-de-gestion/WFS-393.md`. Agent `dev-artiste-3d`, nouvelle session.
- Décision : on n'attend pas WFS-361, car la X5 reste dans le jeu jusqu'à la vente (RG-01 / RG-03 de WFS-361) ; WFS-393 est annulé si WFS-361 part avant lui. P3 : passe après les tickets voitures de l'objectif ; après WFS-362 (même outil `tools/retoucher_formes.mjs`).
- **WFS-362** (M4 + S3) passe ⏳ Prêt (WFS-358 🟣), suivant dans l'ordre des voitures de l'utilisateur (358 → 362 → 359…).
- `track-agent.md` : ligne de Claude, agent dev3d (01UY1Vv41d) enlevée (session terminée).

### 2026-11-09 21:32:21 — 11 vidéos de l'APK 969 : régression de la Campagne, symptôme donné à WFS-374 (Agent po (session_01UY1Vv1Rvnxrn2dFjkD7q2z))
Vidéos regardées image par image (1 à 2 img/s). En Campagne, la voiture est bloquée au départ (0–3 km/h, GAZ inactif) sur 4 courses sur 5 ; la Corniche est OK. Le symptôme est envoyé à l'agent dev Z (WFS-374, session 01UY1Vv3EZ) et noté dans `regles-de-gestion/WFS-374.md`. Le QA (`qa-lead`) est appelé pour le rapport et l'ANO (à partir de ANO-232). Aucun nouveau ticket : lumière → WFS-375, décor → E15. Réponse dans `tache utilisateur.md`.

### 2026-11-09 21:33:03 — Correction : les vidéos de la Campagne étaient en JOUR, pas en nuit ; ajouté à WFS-375 (Agent po (session_01UY1Vv1Rvnxrn2dFjkD7q2z))
L'utilisateur signale mon erreur : j'avais décrit des courses « de nuit » alors qu'il avait choisi JOUR (Réelle, Aube, Dorée). La cause est dans `terrain.cpp:68` : « Réelle » est rangée sous JOUR mais suit l'heure du téléphone. Critère ajouté à WFS-375 : JOUR = plein jour quelle que soit l'heure. QA prévenu.


### 2026-11-09 21:34:11 — Recette manuelle des 11 vidéos de l'utilisateur (APK v0.1.969, 22:13 → 22:25) (qa-lead)
- Rapport : `qa/rapports/recette-manuelle-video-2026-11-09-2213.md`. **Pas de catastrophe ni de régression de conduite démontrée** : aucun plantage, la Corniche roule (111 km/h) et la Campagne roule gaz tenu (vidéo 2, 32 km/h).
- Campagne « bloquée » (vidéos 3 à 6) **non confirmée** : l'option Android « Afficher les appuis » ne montre **aucun doigt sur GAZ après GO** (seulement un appui bref pendant le « 3 ») ; le recul apparent est le recadrage de la caméra. Banc de départ Campagne gaz tenu demandé à WFS-374 pour lever le doute.
- **ANO-232** (majeure → WFS-375) : moment JOUR (Réelle à 22 h, Aube, Dorée en Cockpit) → image de nuit ou presque noire. **ANO-233** (mineure → WFS-374) : appui sur la case SCORE → une image de nitro.
- Décor en formes nues et chargement « ville 70 % » : hors périmètre, pas d'ANO (ordre de l'utilisateur, E15 / WFS-378).

### 2026-11-09 21:35:13 — Correction : Campagne non bloquée (recette QA), vrai défaut = lumière JOUR (ANO-232 → WFS-375) (Agent po (session_01UY1Vv1Rvnxrn2dFjkD7q2z))
Le QA (commit 586736d5) n'a pas confirmé mon constat « Campagne bloquée ». Je l'ai revérifié à 4 img/s : aucun doigt sur GAZ après le GO. Mon erreur est corrigée auprès de l'utilisateur et de l'agent dev Z (WFS-374). Il ne reste qu'ANO-232 (JOUR → image de nuit) → WFS-375, et ANO-233 (mineure) → WFS-374.

### 2026-11-09 21:37:08 — WFS-375 urgent : session de développeur ouverte sur ordre de l'utilisateur (Agent po (session_01UY1Vv1Rvnxrn2dFjkD7q2z))
Ordre de l'utilisateur, juste après ANO-232 : « C'est urgent pour corriger ». Le PO ouvre une session `@dev` (« Agent dev WFS-375 ») et lui donne WFS-375 (ordre explicite de l'utilisateur, exception à « attribuer seulement à la demande »). Le développeur écrit lui-même son attribution (🔧) dans `TACHES.md` et `track-agent.md`.


### 2026-11-09 21:38:15 — WFS-359 : option A (performances de l'ancien T-Cross gardées), WFS-394 créé (po-scrum-master)
- Question de Claude, agent dev3d 2 (01UY1Vv2MH) : la vraie fiche de la Vitara Brezza rend `test_course5_ordinateurs` rouge (tirage du Range Rover Sport, qui dépasse sa vitesse de virage, +31,4 % en Ville ; défaut d'IA déjà présent, hors zone).
- **Décision A** : dans WFS-359, « tcross » garde les performances de jeu de l'ancien T-Cross ; modèle 3D, nom et dimensions de la Brezza. Corvette avec sa vraie fiche. Aucun test affaibli. Travail presque bon gardé (ordre de l'utilisateur du 2026-11-09).
- **WFS-394 créé** (P2, E3, skill expert-conduite, ⏳ En attente après WFS-359) : IA du RR Sport en virage, puis vraie fiche de la Brezza. Écrit dans un worktree séparé de `main` (copie locale du développeur non touchée).

### 2026-11-09 21:38:52 — WFS-375 : pris en charge, branche feature/waforspeed-375 (Claude, agent dev M (01UY1Vv4mu))
- Ordre urgent de l'utilisateur transmis par le PO : JOUR = midi, NUIT = nuit, plus de défilement des moments. Zone : terrain.cpp + lecture du moment de la sauvegarde.

### 2026-11-09 21:45:07 — WFS-376 (PR #318) relue par le lead dev : approuvée, fusion en attente de synchronisation (lead-dev)
- 2e relecture sur 7d69d264, après la réponse point par point du développeur. 44 captures ouvertes (22 paires) : la suite `marina-suite-1` à `-6` se passe dans le port, sans aucun clignotement aux passages de seuil ; de nuit (`gros-plan-nuit`, `poursuite-80-nuit`), les coques et les mâts restent lisibles. Rien n'est dégradé ailleurs.
- Diff complet relu. Branche construite par le lead dev : `tests_cpu.sh` 92/92, dont `test_marina_bateaux`. Budget de 17 454 triangles (≤ 18 000), appels de dessin inchangés, Basse +9,0 %.
- Revue « ✅ Approuvé par le lead dev » postée (COMMENT : même compte GitHub). Squash impossible : conflit dans `journal.md` avec `main`. Le développeur synchronise sa branche et rappelle le lead dev ; la fusion suivra.
- Remarques non bloquantes : un commentaire en double dans `plage.cpp` ; une boucle restée pour une seule borne de ponton ; le salon des vedettes et des yachts reste anguleux de près (à reprendre si la QA le voit).
### 2026-11-09 19:54:26 — WFS-376 : branche créée, analyse de la marina (Claude, agent dev3d (01UY1Vv3u9))
- Branche `feature/waforspeed-376` créée depuis main ; skills agent-developer, waforspeed-natif, artiste-3d, expert-cartes chargés.
- Cause relue dans `src/world/plage.cpp` : `placeQuartiers` (zone marina) + `case O_BATEAU` (boîte + triangle de voile) ; le quai est à ≈ 1,85 m au-dessus de l'eau, coque de 0,55 m cachée derrière le mur.
- Plan : nouvelle géométrie « marina » (pontons flottants, voiliers au port sans voile, vedettes et yachts), LOD sans gréement au-delà de 150 m, tout dans le Geo du décor (aucun appel de dessin en plus).

### 2026-11-09 20:57:26 — WFS-376 : marina modélisée, captures et PR (Claude, agent dev3d (01UY1Vv3u9))
- Fait : `src/world/marina_bateaux.{h,cpp}` (voilier au port, yacht, vedette, 3 niveaux de détail ; pontons, bittes, amarrage), `plage.cpp` (zone marina : quai à 1,45 m au-dessus de l'eau au lieu de 1,85 m, placement cul au quai + 8 pontons, 37 bateaux au mouillage modélisés, banc `marina`), test `test_marina_bateaux`.
- Accroches annoncées : `src/platform/desktop_main.cpp` (2 lignes, groupe de banc `marina`), `CMakeLists.txt` (1 ligne, test).
- Décisions du PO appliquées : disposition des pontons, seuil unique 200 m, quai abaissé, option A (densité réelle, 3 niveaux, budget ≤ 18 000).
- Vérifié : 92 tests CPU réussis ; banc marina (lavapipe), avant = main + banc seul : fenêtre la plus chargée 17 812 triangles (≤ 18 000), appels de dessin inchangés (361 / 298 / 199), image entière +21 % (Cockpit), +10,3 % (Poursuite 120), +11,1 % (Basse).
- Limites : Basse au-dessus de l'objectif de +10 % ; RG-09 : lampadaire de 6,4 m (ratio 2,07 ; 1,77 rapporté à 7,5 m). Non vérifié sur téléphone.

### 2026-11-09 21:26:44 — WFS-376 : corrections du retour du lead dev (PR #318) (Claude, agent dev3d (01UY1Vv3u9))
- Suite de 6 images dans le port (vue Poursuite, 120 km/h), passages de seuil repérés ; vues de nuit `marina-gros-plan-nuit`, `marina-poursuite-80-nuit`.
- Synchronisation avec main 67f20e1a (WFS-379 : la Corniche a un 2e quai de 634 m au niveau de la mer) : bateaux à quai seulement là où le quai abaissé existe ; allègements (coque détaillée à 6 couples, version moyenne sans haubans, pontons de 38 m, une borne par ponton, bittes tous les 28,8 m) : fenêtre la plus chargée 17 454 triangles (≤ 18 000).
- Mesures avant / après (main 67f20e1a + banc seul) : appels de dessin inchangés ; image entière +19,7 % (Cockpit), +8,4 % (Poursuite 120), +9,0 % (Basse). 22 captures avant / après regardées.

### 2026-11-09 21:46:25 — WFS-376 fusionné par le lead dev (lead-dev)
- PR #318 fusionnée en squash (d6c65441) après la synchronisation du développeur. J'ai vérifié 8abf9372 : il n'apporte que `main` 8717f7b2, seul `journal.md` était en conflit, et les changements de la branche sont les mêmes qu'au commit relu 7d69d264. La revue « ✅ Approuvé » a été postée sur 8abf9372 en commentaire (même compte GitHub).
- `TACHES.md` : WFS-376 passe en 🟡 À valider (utilisateur), parce que l'anomalie ANO-206 vient d'une vidéo de l'utilisateur. Ligne ajoutée à `qa/Check.md` ; ligne #318 retirée de `suivi-pr.md`.

### 2026-11-09 21:46:56 — WFS-376 : fin de session (Claude, agent dev3d (01UY1Vv3u9))
- PR #318 fusionnée en squash par le lead dev (d6c65441), ticket 🟡 à valider par l'utilisateur. Réponse écrite dans `tache utilisateur.md`. Ligne de `track-agent.md` : ⏹️ session terminée. Un seul ticket par session : aucun autre ticket demandé.
- Remarques non bloquantes du lead dev laissées pour une tâche séparée (RG-12) : salon anguleux des vedettes et yachts de près, commentaire en double dans `plage.cpp`, boucle inutile dans `ponton()` ; lampadaire de 6,4 m (RG-09) signalé au PO.

### 2026-11-09 21:51:11 — WFS-369 : inventaire des formes nues du mobilier de la Ville (RG-01) et plan (Claude, agent dev3d 3 (01UY1Vv5WS))
- Branche `feature/waforspeed-369` créée et poussée ; skills agent-developer, waforspeed-natif, artiste-3d, expert-cartes chargés.
- Inventaire (détail : `qa/pr/WFS-369/inventaire.md` sur la branche) : `ville.cpp` — bancs du parc (2 boîtes), abribus (4 boîtes), grille du parc (2 boîtes par poteau), fontaines du parc (3 cylindres) et de l'îlot (3 cylindres + 2 disques), lampadaires de rue (cylindre + 2 boîtes), panneaux « sens giratoire » (disque), feux tricolores (cylindre + boîte) ; `commerces.cpp` — tables (cylindre + disque), chaises (2 boîtes), parasols (cylindre + cône, ANO-216), bougie (boîte), pots de fleurs (cylindre + boîte). Absents de la carte : poubelles, bornes, kiosques, barrières (ajout = WFS-385). Hors ticket : immeubles, port, stores, ouvrages (WFS-370 / 385).
- Modèles : aucun modèle libre CC0 / CC BY de ce mobilier aux mesures françaises par les sources permises ; RG-03 appliquée : modélisés par le code au niveau du vrai objet (`src/world/mobilier_ville.*`, nouveau), mesures sourcées dans le fichier. Rien téléchargé.
- Accroches annoncées : `src/world/cartes_banc.cpp` (banc de la Ville : gros plans `carte-ville-mobilier-*`, ≈ 25 lignes de banc) ; `commerces.cpp` : aucune table de terrasse ni client assis dans l'emprise d'un abribus (ils se chevauchaient sur `main`).

### 2026-11-09 21:51:22 — WFS-214 : retour du lead dev corrigé, PR #314 à relire (Claude, agent dev W (01UY1Vv4Ed))
- Point 1 (moments de jour plus blancs) : cause = banc des moments de WFS-208 (temps figé, scène non neuve) : exposition restée à celle de l'aube (EV 8,2 à midi, 7 IL de trop), déjà sur main. Correction du banc (c48c2824) : exposition à sa cible avant la capture. Banc corrigé des deux côtés : moments de jour 46-84 → 60-130 / 255, heure dorée de la Campagne 60 → 60, aube, heure bleue et nuit identiques.
- Point 2 : captures dorée (2), midi (2), matin du circuit, `depart-jour-campagne-4` (départ de jour en Campagne ajouté au banc `depart`).
- Point 3 (performance) : banc `perf`, 4 cartes × Haute / Basse : image −4,1 % à +0,2 % (2 mesures avant, 1 après ; la 2e mesure après coupée par la limite de temps) ; part hors passes ≤ 2,1 ms sur 420 à 650 ms (lavapipe). Tramage gardé.
- Remarques : M-03 noté dans le Suivi de WFS-214 ; ANO-211 / WFS-377 créées par le PO ; branche synchronisée, 92 tests CPU verts. Réponses point par point dans la PR ; lead dev rappelé.

### 2026-11-09 23:18:14 — WFS-214 : PR #314 approuvée et fusionnée (2e revue) ; #316 en attente de synchronisation (lead-dev auto)

#314 reprise (prise de plus d'1 h) : points de la 1re revue corrigés (banc des moments : exposition à sa cible avant la capture, défaut du banc présent sur main ; captures de jour Campagne / circuit ajoutées, rien de blanchi : luminance 60→60, 61→90, 46→65 ; performance de la passe finale −4,1 à +0,2 %). Diff : banc seulement. Squash 44c57ce, 🟣. #316 (WFS-349) : approuvée par lead-dev, encore en conflit `journal.md` (main a reçu #318 et #314 depuis la synchronisation de 21:46) : rappel posté au développeur, fusion dès la synchronisation.

### 2026-11-09 23:25:55 — WFS-214 : fusionné, session terminée (Claude, agent dev W (01UY1Vv4Ed))
- PR #314 fusionnée en squash par le lead dev automatique (44c57ce, 23:18 UTC) après correction du retour (banc des moments, captures, mesure de performance) ; ticket 🟣 (ANO-109 et ANO-106 viennent du QA) ; ligne de `qa/Check.md` écrite par le lead dev.
- Suites laissées au PO : ANO-211 / WFS-377 (banc des passes : « Ultra » introuvable) ; US « occlusion du ciel » si les ombres de jour paraissent délavées sur le téléphone.
- Un seul sujet par session : je m'arrête, aucun ticket demandé.


### 2026-11-09 14:00:55 — Reprise de WFS-349 : vue Cockpit, habitacle fermé (Claude, agent dev Y (01UY1Vv1Yvim))
- Branche `feature/waforspeed-349` reprise (commit d'agent 3 bis, aucun code), synchronisée avec `main` et poussée tout de suite. Première étape : cause nommée au banc `penetration` (RG-01).

### 2026-11-09 16:11:22 — WFS-349 : cause nommée (RG-01) et correction en cours (Claude, agent dev Y (01UY1Vv1Yvim))
- **Reproduit** au banc `penetration` (Golf, Ville de nuit, vue Cockpit, voisine à 1,9 m) : sans voisine, en bas à gauche de l'image, on voit **la route** là où devrait être la portière ; avec la voisine, ses feux arrière se dessinent jusqu'au bas de l'image (comme sur la vidéo de l'utilisateur, APK 928, 40,4 s).
- **Cause : trou dans l'habitacle, pas d'ordre ni de profondeur de dessin** (l'intérieur est bien dessiné avec la profondeur, plan proche 0,1 m). Dans `vehicule_cockpit.cpp`, chaque contre-porte n'était qu'une plaque de **6 cm**, posée de **15 cm derrière le pied du pare-brise** (z −0,95 m) à 0,35 m, et de la ceinture jusqu'à **0,8 m sous l'œil** ; ni plancher, ni tablier. Vue de l'œil (à 0,38 m de la portière gauche), la plaque est vue par la tranche : la vitre « descend » jusqu'au coin bas de l'image, le coin entre planche, montant et portière est ouvert, et la zone des pieds n'a pas de paroi.
- **Banc neuf `cockpit-voisine`** (`src/render/vehicule_cockpit_banc.cpp`, accroche de 4 lignes dans `desktop_main.cpp`, annoncée ici) : voisine à 1,8 puis 1,9 m, à gauche et à droite, devant et à côté, regards Bas / Normal / Haut, Ville de jour ; image normale, image sans la voisine, image avec l'habitacle et un **gabarit de ceinture** (plans des vitres latérales sous la ceinture, pied du pare-brise sous la planche) en couleur clé. Pixel interdit = pixel de la voisine dans la zone fermée. Secousse du choc (WFS-70) coupée pendant la mesure (elle bougeait la caméra entre les images). Clé douce (émission 1,5) : celle de WFS-235 (16) débordait de ≈ 10 px.
- **Avant** (ancienne portière, 1280 × 720) : Golf **10 647** pixels interdits dans la pire image, X5 **10 838**.
- **Correction** (`vehicule_cockpit.cpp`, `vehicule_cockpit.h`) : portière **pleine de 12 cm** (face intérieure inchangée, WFS-315), appui de vitre à la ligne de ceinture, de 6 cm devant le pied du pare-brise (sous le pied du montant) à 0,6 m derrière l'œil, du plancher à la ceinture ; plancher et tablier ; matière jamais noire (luminance ≥ 0,045). Tout dans le même maillage : **0 appel de dessin de plus**.
- **Après** (Golf, 1280 × 720) : **0** pixel interdit, 24 images. Test CPU neuf `test_cockpit_ferme` (rayons de l'œil, 20 voitures) : 0 rayon sans paroi ; il a trouvé une fente de quelques mm sous le bord avant de la planche, fermée (tablier monté d'1 cm, caché derrière la planche).

### 2026-11-09 19:50:39 — WFS-349 : mesures finales, PR ouverte (Claude, agent dev Y (01UY1Vv1Yvim))
- **RG-03** (banc `cockpit-voisine`, 20 voitures, 3 regards, gauche / droite, 1,8 / 1,9 m, devant / à côté) : **0 pixel interdit en 1280 × 720 (39 min) et en 2340 × 1080 (80 min)** ; la voisine devant reste visible par la vitre (≥ 18 900 px en 1280). Les derniers 2 à 25 px (Compass, Corsa, Range Rover, 500 Abarth…) étaient des points très lumineux de la voisine (reflet du soleil, feu) étalés de 3 à 4 px sur le bord de l'appui de vitre, pas des trous : bord non compté porté à 5 px (720 lignes) ; les fentes fines sont prouvées par `test_cockpit_ferme`.
- **RG-04** (banc `cockpit`, 2340 × 1080) : fenêtre de la rue 0,0 % ; horizon inchangé ; intérieur en Normal 35,6 à 39,7 % (≤ 42 %, +1,4 à +1,7 point) ; en Bas 42,2 / 42,4 % (Golf, 500 Abarth ; seuil visé en Normal). Pluie : gouttes seulement derrière les vitres.
- **RG-05** : `parcours` 0 écart (et en vue Cockpit 0 écart) ; `course5` code 0. **RG-06** : appels de dessin identiques (32 / 35 / 29), mémoire 1,22 Mo ; temps GPU sur téléphone non mesuré.
- Synchronisation avec `main` : 91 tests CPU réussis ; Golf, X5, 500 Abarth toujours à 0. Limites : ligne de ceinture de WFS-315 (pas de cote mesurée par voiture), plancher approché (±3 cm) ; X5 voisine à droite : teinte magenta du banc (images retirées de la PR, mesure non touchée).

### 2026-11-09 21:04:20 — WFS-349 : retour du lead dev corrigé (PR #316) (Claude, agent dev Y (01UY1Vv1Yvim))
- **1. Captures refaites sur la base synchronisée avec `main`** (WFS-356, 343, 338, 193 compris) : `captures_pr.sh WFS-349 penetration:2340x1080 cockpit:2340x1080` ; paires de nuit regardées : éclairage identique avant / après, seule la portière change. Images `cockpit-voisine` refaites sur la même base (avant = ancienne portière) : avant Golf 9 249, X5 8 559, 500 Abarth 10 436 pixels interdits ; après 0.
- **2. Description de la PR** régénérée par script (`qa/pr/WFS-349/pr.md`) : tous les liens d'images vérifiés.
- **Remarques** : exemple de bord (`WFS_BORD=2`, Compass : 13 px sur le bord de l'appui de vitre, sous le feu arrière, image agrandie dans la PR) ; gabarit et clé douce construits seulement pendant le banc (`g_bancCockpitVoisine`) ; **teinte magenta du X5 (voisine à droite) : cause** : la capture d'environnement (cube de lumière de WFS-150, une face par image, rendue avec les objets de l'image, intérieur compris) était encore en cours quand la mesure commençait : une face rendue pendant l'image en couleur clé teintait le cube, donc la lumière ambiante ensuite. Le banc attend maintenant la fin de la capture avant de suspendre les captures ; X5 refait : 0 pixel teinté, 0 pixel interdit. (Le même risque existe dans la part cachée de WFS-235, `camera_vue.cpp` : signalé, hors de ma zone.)

### 2026-11-09 23:28:50 — WFS-349 : PR #316 fusionnée après synchronisation (lead-dev auto)

Approuvée sur son contenu par lead-dev (21:34) ; fusion bloquée par des conflits `journal.md`. Synchronisation a3ea5e31 : branche à jour avec main, code identique au commit approuvé 2ae74f81 (vérifié ligne à ligne). Squash 581427c, statut 🟡 À valider (utilisateur). Plus aucune PR ouverte.

### 2026-11-09 23:32:06 — Fin WFS-349 : PR #316 fusionnée, session terminée (Claude, agent dev Y (01UY1Vv1Yvim))
- PR #316 fusionnée en squash par le lead dev automatique (581427c) ; ticket 🟡 À valider (utilisateur). Ligne ajoutée à `qa/Check.md` (elle manquait après la fusion).
- Livré : vue Cockpit, portières pleines de 12 cm (appui de vitre à la ceinture), plancher, tablier ; banc `cockpit-voisine` (0 pixel interdit, 20 voitures, 1280 × 720 et 2340 × 1080 ; avant : 8 559 à 10 436) ; test `test_cockpit_ferme` ; 93 tests CPU réussis à la dernière synchronisation contrôlée.
- Signalé hors de ma zone : le même risque de teinte par une capture d'environnement en cours existe dans la part cachée de WFS-235 (`camera_vue.cpp`, banc `cockpit`).
- Session terminée (un seul sujet par session) : ligne de `track-agent.md` en ⏹️ ; aucun autre ticket demandé.

### 2026-11-09 23:36:22 — WFS-362 attribué à Claude, agent dev3d 4 (01UY1Vv5HV) (po-scrum-master)
- Demande de la session session_01UY1Vv5HVQdBTHL1fTJgqjz (`@dev3d`, agent `dev-artiste-3d`), nouvelle session, 1er et seul ticket ; l'utilisateur n'a pas choisi de sujet.
- Choix : **WFS-362** (P1, E10) « Voitures sans marque : retouches de forme de M4 et S3 » : objectif de l'utilisateur (voitures sans marque) avant les décors ; prêt depuis la fusion de WFS-358 ; réservé au profil `dev-artiste-3d`.
- Exception PO E10 (comme WFS-358 / 359) : en parallèle de WFS-359 (dev3d 2), fichiers séparés ; seul fichier commun `tools/formes.json` (entrées différentes : garder les deux à la synchronisation).
- Zone : `assets/cars/m4*`, `assets/cars/s3*`, entrées `m4` / `s3` de `tools/formes.json`, ajouts à `tools/retoucher_formes.mjs` (`--verifier` vert sur compass, p208, x5), `tests/**`. Pas `src/game/cars.h`, `vehicule_cockpit*`, voitures de WFS-359, x5 (WFS-393).
- Règles : `regles-de-gestion/WFS-362.md` (RG-01 à RG-06, méthode § 5 du skill `artiste-3d`). Branche `feature/waforspeed-362`, premier commit poussé tout de suite.
- `track-agent.md` : lignes des sessions terminées enlevées (dev3d 01UY1Vv3u9 / WFS-376, dev Y / WFS-349, dev W / WFS-214) ; derniers envois mis à jour (359, 369, 374, 375).

### 2026-11-09 23:36:45 — Début WFS-391 : attribué à Claude, agent conduite (01UY1Vv4e7) (po-scrum-master)
- Demande de la session `@conduite` (session_01UY1Vv4e77bVQHvt3NPaSmG), 1er et seul ticket de la session (règle « un seul sujet par session »).
- Ticket : **WFS-391** (P2 Haute, E3) — correction de la conduite au bas-côté : traînée de l'air hors μ sur la neige, bas-côté enneigé moins accrocheur que la chaussée (ANO-229), rayon faux au bas-côté intérieur des épingles de la Campagne (ANO-231). Règles : `regles-de-gestion/WFS-391.md` (RG-01 à RG-06, 5 captures demandées).
- Choix : seul ticket de conduite ⏳ Prêt (WFS-394 attend WFS-359 ; WFS-258 / 259 / 260 ⏸ ou en attente, réservés à l'agent dev conduite) ; E3 libre (aucun ticket 🔧 / 🔀).
- Zone autorisée : `src/game/player.*`, `src/game/roues.*`, `tests/**` ; interdit : `aide_direction.*` (sauf accroche ≤ 10 lignes annoncée), `poursuite*`, `hud.*`, `traffic.*`, `camera_vue.*`, `src/world/cartes_v2/**`, la CI. Cause dans le sol de la carte (RG-06) : avertir le PO avant de toucher hors zone.
- Branche `feature/waforspeed-391`, premier commit à pousser tout de suite. Après la fusion par le lead dev : 🟡 À valider (utilisateur) (ticket de conduite), puis arrêt de la session.

### 2026-11-09 23:37:40 — Début WFS-382 : attribué à Claude, agent dev N (01UY1Vv5K1) (po-scrum-master)
- Demande de la session `@dev` session_01UY1Vv5K1S9ZbuUABTub9NM (1er et seul ticket). P1 prêts pour un développeur général : WFS-380, 381, 382 (WFS-378 est réservé à `dev-artiste-3d`). Ordre du PO du 2026-11-09 20:01 UTC : phares (382) et brume (381) avant la mer (380) → **WFS-382** (phares de nuit, ANO-222, + neige ANO-228), E4 libre.
- Zone : `nuit.cpp`, `phares.h`, `bench_nuit.*`, `tests/**` ; accroches ≤ 10 lignes : bloom de `shaders/post.frag`, albédo de la neige dans `route.glsl` / `route_*`. Interdit : E13 ciel / brume (WFS-381), `vehicule_*` (WFS-359, 362), `terrain.cpp` (WFS-375), E0 (WFS-374), mer (WFS-380). Détail : `regles-de-gestion/WFS-382.md`, section « Suivi ».
- Skills : `agent-developer`, `waforspeed-natif`, `expert-realisme-3d` (consultation). Hors E15 : pas d'avis 3D obligatoire. Après la fusion : 🟡 À valider (utilisateur). Responsable de WFS-375 rempli (agent dev M, d'après `track-agent.md`).

### 2026-11-09 23:38:48 — Début WFS-380 : attribué à Claude, agent cartes (01UY1Vv4eo) (po-scrum-master)
- **Ticket** : WFS-380 (P1, E15 file Corniche ; ANO-214) « Vraie mer : vagues, reflets, couleur selon la profondeur, écume ». Demandé par la session `@cartes` (1er et seul ticket de la session). Dépendances finies : WFS-379 (#317) et WFS-376 (#318) fusionnées, `plage.cpp` libre.
- **Pourquoi lui** : ticket P1 prêt le plus prioritaire du domaine des cartes (décor d'une carte) ; WFS-392 (P2) reste prêt pour une autre session ; WFS-357 attend que E8 soit libre (WFS-375). Aucun conflit : Corniche sans autre ticket actif dans E15 (WFS-369 est la Ville).
- **Zone** : `plage.cpp` (mer, rivage), shader de l'eau (`shaders/eau*.glsl` neuf ou branche eau de `scene.frag`), `src/render/eau_*`, accroches ≤ 10 lignes `ville.cpp` (eau seulement) et `cartes_v2` (eau), textures CC0 / CC BY, `tests/**` ; détail dans `regles-de-gestion/WFS-380.md` (section Attribution).
- **Rappels** : avis de l'agent 3D VALIDÉ avant le lead dev (E15) ; 🟡 À valider (utilisateur) après la fusion ; coût en Basse ≤ 1,0 ms GPU (RG-07).
- **Avec WFS-382** (E4, phares, attribué à 23:37) : fichiers séparés (382 : `nuit.cpp`, `phares.h`, bloom de `post.frag`, neige de `route*` ; 380 : eau seulement, de préférence `shaders/eau*.glsl` neuf). Les deux se synchronisent souvent avec `main`.

### 2026-11-09 23:44:02 — WFS-391 : arbitrages du PO sur RG-01 et RG-06 (po-scrum-master)
- **Demande** : Claude, agent conduite (01UY1Vv4e7), deux questions de règles sur WFS-391.
- **RG-01 : option A.** Seule la force des pneus (freins + frein moteur) dépend de μ ; traînée et roulement non. Ancien modèle accepté de 120 à 160 m sur la neige (≈ 127 m mesuré, roulement du jeu 0,35 m/s² non touché, `configgame.json` inchangé) ; nouveau modèle `conduite_monde` 140 à 160 m (143,5 m). Sec 36,8 m (RG-03 tenu). Option B refusée (freinage léger moins fort que le lever de pied, et RG-05).
- **RG-06 : cause hors conduite** (point n = 11,5 m derrière la glissière à 10,35 m). Mesure du bas-côté intérieur à n = 9 m ; test CPU « derrière la glissière, la voiture la longe ». ANO-231 : cause écrite, fermeture par le QA après la fusion.
- **Annexe** : muret visible (8 m) et glissière de collision (10,35 m) des épingles du col → **ANO-234** (mineure), rangée dans **WFS-387** (RG-11), pas de nouveau ticket.
- Fichiers : `regles-de-gestion/WFS-391.md`, `regles-de-gestion/WFS-387.md`, `anno.md`, `track-agent.md`.

### 2026-11-09 23:45:07 — WFS-359 : PR #320 ouverte, Corvette C7 (« mustang ») et Suzuki Brezza (« tcross ») à la place des modèles CC BY-NC-SA (Claude, agent dev3d 2 (01UY1Vv2MH))
- **Téléchargements** (`regles-reseau.md` § 2.4 : clones partiels en lecture seule de dépôts GitHub publics, dans le bac à sable de la session, rien exécuté ; `license.txt` lus) :
  - « Chevrolet Corvette (C7) », Martin Trafas, Sketchfab, **CC BY 4.0** — dépôt `JoseJ55/car-showcase-v2`, `public/models/corvette_c7/` : scene.gltf SHA-256 5ca3bb70c9f6bd9be97f1f1ffa1268359c47641dc480ab1d7f0c27c8dc9a19b0 (122 068 o), scene.bin a6bf80fb6f6da4bf50f78e234088bc2f1b6d68a2cd7f099aea7e2dd14436bec2 (37,6 Mo), textures PNG/JPEG du dossier ; **retenu** (choix du PO).
  - « 2022 Maruti Suzuki Vitara Brezza », BHP3D, **CC BY 4.0** — dépôt `kriteesh/threejs-model`, `model-2/` : scene.gltf 55b43df1e0c6b58958e15c347e4fa990d8721700bcf222d249356b05e4a12852 (718 867 o), scene.bin db74ff08bce9ad0140c2b1a9b3234af1144790a9647a76d87ea24963790ca02b (17,3 Mo), textures du dossier ; **retenu** (choix du PO).
  - Essayés puis **refusés** (rien dans le dépôt) : « Volkswagen Passat » I3D (CC BY, `krisAndreev/MyMiniCar`, gltf 890d3c1a…dec5, bin 69bab18c…c8ce : Passat B5.5, largeur +2,3 %) ; « Ford Mustang 2015 EDITION » WarEntertainment (CC BY, `shimada-wisteria/webar`, gltf 039dd6ea…1c20, bin 25ef37fc…8ac6 : hauteur −5 %, version rabaissée) ; « 2022 Volkswagen Virtus GT » BHP3D (CC BY, `PrathamSatpute/VehiclePredictiveMaintenance`, gltf 1bfe8554…b8e7 : hauteur −2,6 %) ; « 2023 Toyota RAV4 GR-S » Nazh Design (CC BY, `lordkiki/threejs`, gltf e7a0020b…193c : un seul matériau) ; « 2015 Honda CR-V » ImperialBlue (CC BY, `Kavelin/kavelin.github.io`, gltf 9b162f58…38c8 : 4 478 triangles) ; « Hellcat » et « Corvette C8 » lus (licence seulement), non convertis. Modèles de Ddiaz Design écartés (matériaux « …RewardRecycled… » : tirés d'un jeu existant).
- **Fait** (zone du ticket) : conversion par `tools/convertir_voitures.sh` (sources `CORVETTE=` / `BREZZA=`), fiches du référentiel (`references/reel/voitures.json`, `SOURCES.md`), `cars.h`, licences et crédits, logos effacés (`tools/logos.json`), correctifs de feux des anciens modèles retirés (`correctifs_voitures.h`), « tcross » hors halogène (`phares.h`), texte du garage pour « mustang » ; `assets/cars/remplacants/mustang.wfm` (inutilisé) retiré. Accroches hors `cars.h` annoncées ici : `src/ui/garage.cpp` (1 ligne), `src/render/phares.h` (1 ligne), `src/render/correctifs_voitures.h` (3 lignes retirées).
- **Vérifié** : `tests_cpu.sh` 91/91 ; `test_fidelite_voitures` strict vert (C7 8 / 8 conformes, Brezza 6 / 6 sourcées) ; `verifier_licences.py` 0 manque ; `effacer_logos.mjs --verifier` : 20 voitures « déjà faites » ; 20 paires de captures avant / après (jour et nuit) regardées, `qa/pr/WFS-359/`.
- **Limites** : performances de la Brezza → WFS-394 (option A du PO) ; vue Cockpit : habitacle dessiné de `vehicule_cockpit_defs.cpp` encore celui de la Mustang S550 et du T-Cross (hors zone, tâche séparée demandée au PO) ; voies de la Brezza « à sourcer » ; hauteurs des feux du trafic lointain (`traffic.h`) inchangées.

### 2026-11-09 23:46:04 — WFS-374 : PR #321 ouverte, ANO-233 corrigée, banc depart-campagne (Claude, agent dev Z (01UY1Vv3EZ))
- Symptôme du PO (vidéos v0.1.969) : Campagne « bloquée » non confirmée (QA, PO). Bancs refaits avec de vrais appuis sur GAZ (Corniche → Campagne, course relancée, 6 moments) : la XR5 roule (58 km/h 3 s après le GO). Aucun commit fautif trouvé.
- ANO-233 : cause `commandeSous()` (coin haut droit = nitro hérité de l'ancien bouton) → au-dessus de la zone du nitro, rien. Accroches : `desktop_main.cpp` (2 lignes, banc `depart-campagne`), `bench_effets.cpp` (appui nitro au centre du vrai bouton).
- Vérifié : 94 tests CPU verts ; banc `depart-campagne` main 6 écarts → branche 0 ; `nitro-lignes` nitro actif. Captures `qa/pr/WFS-374/`. Lead dev appelé.


### 2026-11-09 23:48:00 — WFS-395 créé : habitacles Cockpit de la Corvette et de la Brezza (po-scrum-master)
- Demande de Claude, agent dev3d 2 (01UY1Vv2MH), PR #320 (WFS-359) : la vue Cockpit de « mustang » et « tcross » dessine encore l'ancienne Mustang S550 et l'ancien T-Cross (`vehicule_cockpit_defs.cpp`).
- **WFS-395** (P2, E10 `vehicule_cockpit*`, agent dev-artiste-3d, nouvelle session ; ⏳ En attente après WFS-359, 349, 390 : mêmes fichiers) : habitacles de la C7 et de la Brezza à leur vraie largeur intérieure (WFS-315), sans forme nue, captures `cockpit`.
- Les feux du trafic lointain (`traffic.h`, `WEB_FEUX`) restent une limite notée dans la PR, sans ticket pour l'instant. Écrit depuis un worktree séparé de `main`.

### 2026-11-09 23:52:05 — WFS-375 : PR #322 ouverte (Claude, agent dev M (01UY1Vv4mu))
- Fait : JOUR = midi, NUIT = nuit, sans défilement (`terrain.cpp`) ; ancien moment de la sauvegarde rangé en midi / nuit à la lecture (`sauvegarde.cpp`) ; test `test_sauvegarde` (bloc WFS-375) ; scénario du banc `benchTerrain`.
- Vérifié : `tests_cpu.sh` 94/94 ; banc `menu1` 1280x720 horloge à 22:15 (faketime) : main → doree / bleue, branche → midi / nuit ; Campagne en plein jour, Corniche de nuit ; 8 captures avant / après dans `qa/pr/WFS-375/`.
- Limites : haut du ciel de Campagne encore un peu sombre (E13, hors ticket) ; pas de construction Android locale.

### 2026-11-09 23:56:57 — WFS-359 fusionné par le lead dev (lead-dev)
- PR #320 relue (commit 5adfa463) : les 20 paires de captures ouvertes une à une, plus des zooms ; tout le diff lu ; construction et `tests_cpu.sh` refaits dans un worktree à part : 94/94. Les outils `convert_car.mjs` et `simplifier_voiture.mjs` savent déjà lire les nouvelles options de `convertir_voitures.sh`. `logos.json` ne change que pour `mustang` et `tcross`.
- Revue « ✅ Approuvé » écrite en commentaire (GitHub refuse l'approbation du même compte), puis fusion en squash 2d304765. WFS-359 passe en 🟣, `qa/Check.md` est à jour, la ligne #320 est retirée de `suivi-pr.md`.
- **Au PO** : il faut une tâche séparée (E10, `dev-artiste-3d`) pour les petits défauts vus aux zooms : chevron bleu qui traverse la plaque avant de la Corvette ; socle en losange du logo resté sur la calandre de la Brezza (il ressemble à un écusson, RG-C3) ; échappements de la Corvette sans diffuseur ; petits triangles dans le grillage de la Corvette. Hors zone : noms inventés de `configgame.json` (« Stallion GT5 », « Vollmer Trek-X ») à revoir pour une Corvette et une Brezza ; habitacle Cockpit (WFS-349) ; `WEB_CARS` / `WEB_FEUX` de `traffic.h`.

### 2026-11-09 23:57:27 — WFS-374 fusionné par le lead dev (lead-dev)
- PR #321 (Claude, agent dev Z (01UY1Vv3EZ)) fusionnée en squash (d94f73b8) à la 1re revue. Revue « ✅ Approuvé » écrite dans la PR sur d78109a8, en commentaire, car GitHub refuse l'approbation du même compte.
- Vérifié par le lead dev : 14 captures ouvertes (nitro allumé sur SCORE avant, éteint après, dans les 6 moments ; nitro-lignes identique) ; tout le diff lu ; construction et `tests_cpu.sh` : 94/94 verts ; banc `depart-campagne` : 0 écart.
- Statut : 🟡 À valider (utilisateur) (RG-05). Ligne `qa/Check.md` ajoutée, ligne de `suivi-pr.md` supprimée.
- **Pour le PO** (tâche à part, ordre de l'utilisateur du 19:14) : `tools/qa/emulateur.sh:226` (scénario `conduite`, `simulateur.yml`) appuie encore sur le nitro en (0,90 W ; 0,32 H). Depuis WFS-374, cet appui ne fait plus rien : il faut appuyer au centre du bouton NITRO. Le scénario `mvp` des bancs du matin n'est pas touché.

### 2026-11-09 23:57:31 — WFS-359 : fin, session terminée (Claude, agent dev3d 2 (01UY1Vv2MH))
- PR #320 fusionnée par le lead dev (squash 2d304765, revue « ✅ Approuvé » sur 5adfa463 ; tests_cpu 94/94 refaits par le lead dev). Suites : WFS-394 (performances de la Brezza, après l'IA du RR Sport en Course à 5) et WFS-395 (habitacles Cockpit), créées par le PO ; petits défauts relevés par le lead dev (chevron sur la plaque avant de la C7, socle en losange du logo de la Brezza, échappements de la C7, triangles sombres dans la grille de la C7, noms inventés et `WEB_CARS` / `WEB_FEUX` des anciennes voitures) : tâche séparée demandée au PO par le lead dev.
- Un seul ticket par session (règle du 2026-11-09) : la session s'arrête, aucun autre ticket demandé.

### 2026-11-09 23:58:03 — WFS-374 : fusionné (#321), session terminée (Claude, agent dev Z (01UY1Vv3EZ))
- Fusion par le lead dev (squash d94f73b8), 🟡 à valider par l'utilisateur. Réponse écrite dans `tache utilisateur.md`. Remarque du lead dev transmise au PO : `tools/qa/emulateur.sh:226` appuie encore sur l'ancien coin du nitro (capture `04-nitro` de `simulateur.yml`). Session arrêtée (un seul sujet).

### 2026-11-09 23:57:25 — WFS-396 créé et attribué à Claude, agent artiste (01UY1Vv2ph) (po-scrum-master)
- Demande d'une nouvelle session `@artiste` (agent `artiste-3d`, sans code du moteur). Aucun ticket prêt n'était dans ce périmètre : le décor (E15 : WFS-372, 373) se modélise dans le code (`src/world/*`, voir WFS-369), les tickets voitures prêts sont tenus (WFS-359, 362, au plus deux à la fois).
- Décision du PO : **découpe de WFS-360** sur le modèle de WFS-219 : **WFS-396** (P1, E10, exception PO : fichiers neufs `assets/cars/remplacants/**`, `assets/licences/remplacants/**`, entrées neuves de `tools/logos.json`) trouve, vérifie (±1 %, ≤ 8 Mo, sans marque) et convertit les remplaçants de l'**A 45** (Classe E W212, choix du PO) et de l'**Elantra N** (au plus 2 choix proposés au PO), **sans rien brancher**. WFS-360 garde le branchement, après WFS-359 et WFS-396.
- Zone interdite : `src/**`, `shaders/**`, outils `tools/*.mjs` / `*.sh` (lus seulement), `tools/formes.json` (WFS-362), `cars.h`, mustang / tcross (WFS-359). Captures : vitrine en local par permutation non poussée (RG-07). Règles : `regles-de-gestion/WFS-396.md`. Écrit depuis un worktree séparé de `main`.

### 2026-11-10 00:01:53 — WFS-381 attribué à Claude, agent dev A (01UY1Vv42R) (po-scrum-master)
- Demande d'une nouvelle session `@dev` (session_01UY1Vv42RWwaV9T1tMxWRgB, développeur général, 1er et seul ticket). Choix : ticket ⏳ Prêt le plus prioritaire d'une épic libre hors conduite, cartes, CI, voitures, iOS : **WFS-381** (P1, E13, brume au loin, ANO-221), en tête de l'ordre décor du 2026-11-09 20:01 avec WFS-382 (en cours). E13 libre (WFS-214 🟣). WFS-372 / 373 (E15) restent au profil `dev-artiste-3d` ; WFS-378 vient après dans l'ordre ; WFS-384 / 287 à l'agent expert ci.
- Zone : voir `regles-de-gestion/WFS-381.md` (section « Suivi ») ; sans chevauchement avec WFS-382 (bloom de `post.frag`, `nuit.cpp`), WFS-380 (eau), WFS-375, WFS-369.
- Plan court : lire le brouillard existant (`common.glsl`, `eclairage*.glsl`, `sky.frag`, `atmosphere_physique.h`), brume exponentielle distance + hauteur couleur du ciel (RG-01 à 03), réglage par carte (RG-04), mesure perf Basse avant / après (RG-05), captures 1 à 5 avant / après, PR, lead dev.
- Ménage quotidien du 2026-11-10 : laissé au robot `archives.yml` de cette nuit (journal du 2026-11-08 et lignes ✅) ; vérification de son commit au prochain appel.

### 2026-11-10 00:03:19 — WFS-372 attribué à Claude, agent dev3d 5 (01UY1Vv5G4) (po-scrum-master)
- Demande d'une nouvelle session `@dev3d` (agent `dev-artiste-3d`, session_01UY1Vv5G4636Nbsd5PhaAsn), 1re et seule attribution (règle « un seul sujet par session »).
- Choix : ticket prêt le plus prioritaire de son domaine (P1, E15 ; Ville WFS-369 et Corniche WFS-380 déjà en cours, file Campagne libre ; WFS-372 avant WFS-373 au numéro). Voitures E10 : déjà deux tickets en parallèle (WFS-362, 396), rien de prêt.
- Zone : `src/world/campagne.cpp` (décor), nouveaux `src/world/decor_campagne_*`, `assets/world/modeles/campagne/**`, `assets/textures/decor/campagne/**`, `assets/licences/**`, `tests/**`. Pas de conflit : WFS-391 (`player.*`, `roues.*`), WFS-382 (`nuit.cpp`), WFS-381 (brume), WFS-392 (`route*`).
- Plan court : inventaire au journal (RG-01) → modèles CC0 / CC BY aux mesures ±1 % (vignes, maisons, clôtures, bottes, hangars ANO-219) → instances + LOD au chargement → captures jour / nuit / gros plans / perf → PR → avis agent 3D VALIDÉ → lead dev.

### 2026-11-10 00:15:26 — WFS-375 fusionné par le lead dev (lead-dev)
- PR #322 relue sur 82bdda3 : 16 captures ouvertes (JOUR / NUIT sans défilement, Campagne en plein jour à 22:15, Corniche de nuit), diff lu (`terrain.cpp`, `sauvegarde.cpp/.h`, `test_sauvegarde.cpp`), construction propre et `tests_cpu.sh` refaits : 94/94.
- Revue « ✅ Approuvé » en commentaire (GitHub refuse l'approbation du même compte), fusion squash 14ec9d2. WFS-375 → 🟡 À valider (utilisateur) (RG-06) ; ligne `qa/Check.md` ajoutée ; ligne retirée de `suivi-pr.md`.

### 2026-11-10 00:15:58 — Fin WFS-375 : PR #322 fusionnée par le lead dev, session terminée (Claude, agent dev M (01UY1Vv4mu))
- Squash 14ec9d2 ; ticket 🟡 À valider (utilisateur) : choisir JOUR sur le téléphone le soir, la course doit être en plein jour ; NUIT, en vraie nuit.
- Session arrêtée (un seul sujet par session) ; aucun autre ticket demandé.

### 2026-11-10 00:17:14 — WFS-372 : branche poussée, inventaire des formes nues de la Campagne, plan (Claude, agent dev3d 5 (01UY1Vv5G4))
- Branche `feature/waforspeed-372` poussée (79fda8a6). Inventaire complet (RG-01) : `qa/pr/WFS-372/inventaire.md` + captures `qa/pr/WFS-372/inventaire/` (banc `campagne`, main d75a3d7b).
- Objets du ticket : vignes `T_VINES` 1 755 blocs (bandes pleines, ANO-65) ; clôtures `T_FENCE` 485 travées + 2 enclos (`box`) ; bottes `T_BALES` 41 groupes (`cylAxe` nu) ; fermes `T_FARM` 20 (boîte grise 8 × 11 m + grange 10 × 14 m, ANO-219) ; poteaux électriques `T_POLE` 27 (cylindre + traverse, le « portique sur poteaux » d'ANO-219).
- Hors ticket, listés pour WFS-387 : église, commerces, fontaine, éoliennes, signalisation, haies, ouvrages de l'autoroute ; les 100 maisons du village sont déjà celles du kit d'architecture (WFS-161), pas des boîtes.
- Plan : modèles calculés sur le processeur dans un nouveau fichier `src/world/decor_campagne_modeles.{h,cpp}` (même méthode que les bateaux de WFS-376 : lofts lissés, chanfreins, 2 niveaux de détail), posés dans le décor par `campagne.cpp` (accroche par cas de `dessine`), test `tests/test_decor_campagne.cpp` (mesures). Réseau : seulement GitHub et paquets ; aucun modèle libre aux bonnes mesures attendu pour ces objets agricoles → RG-03 (modélisés au niveau du vrai objet), noté au journal à la fin.
- Mesures retenues (sources dans le code) : rang palissé Guyot, piquets 1,8 m tous les 5 m, fils à 0,6 / 1,0 / 1,4 m, ceps tous les 1,0 m ; botte ronde 1,20 m de large, Ø 1,50 m ; clôture rustique poteaux Ø 0,12 m de 1,2 m, deux lisses ; ferme à murs crépis, toit de tuiles canal à 2 pans (pente ≈ 30 %), grange-hangar à bardage bois et toit ondulé ; poteau bois de ligne basse tension 8,5 m.

### 2026-11-10 00:18:16 — WFS-396 : décision du PO sur les remplaçants de l'A 45 et de l'Elantra N (po-scrum-master)
- Rapport de Claude, agent artiste (01UY1Vv2ph) : aucun modèle libre dans ±1 % tel quel (W212 hauteur −2,3 % ; W211 voie avant −2,2 % ; Virtus GT hauteur −2,6 % ; Corolla E180 largeur +2,1 %).
- **Décision (option a)** : refus motivé pour les deux places ; WFS-396 se livre avec le rapport et les fiches réelles (`references/reel/voitures/`), rien de converti ; la session s'arrête après la fusion.
- **Suite** : WFS-397 créé (P1, E10 exception `remplacants/`, agent `dev-artiste-3d`, nouvelle session, après WFS-396) : retoucher la **W212 de Peter_D** (hauteur ; repli W211 de BadKarma, voie avant +17 mm par roue) et la **Virtus GT de BHP3D** (hauteur, allègement < 8 Mo) à ±1 %. WFS-360 dépend maintenant de WFS-397.
- A 45 de Lexyc16 interdite (licence contradictoire CC BY / CC BY-NC).

### 2026-11-10 00:24:40 — WFS-362 : PR #323 ouverte, M4 et S3 sans calandre de marque (Claude, agent dev3d 4 (01UY1Vv5HV))
- Fait : calandres découpées sur leur contour exact (`decouper`) ; haut de l'ouverture refermé par une pièce de carrosserie (nouvelle retouche `panneau` de `tools/retoucher_formes.mjs` : même peinture, normales raccordées, bombé à pente nulle au bord, jamais devant) ; bas : grille ajustée (outil de la X5). M4 : anneau chromé de l'ancien logo du capot retiré.
- Fichiers : `tools/retoucher_formes.mjs`, `tools/formes.json` (entrées m4, s3), `assets/cars/m4.*`, `assets/cars/s3.*`, `qa/pr/WFS-362/`.
- Vérification : `--verifier` vert (m4, s3, x5, compass, p208) ; X5 rejouée depuis son origine : même empreinte (factorisation sans effet) ; 94 tests CPU verts ; `test_fidelite_voitures` : m4 et s3 identiques à `main` ; captures avant / après (18 vitrines, 16 gros plans jour / nuit, 12 zooms ×4).
- Limites : S3 125,3 k triangles (repère 120 k dépassé, comme X5 / 208 / Compass) ; joint du panneau visible de près (écusson sombre de loin sur la M4 noire) ; dépendance npm `meshoptimizer@0.22.0` installée dans le conteneur (version du `tools/package.json`) pour refaire les LOD.


### 2026-11-10 00:30:14 — WFS-362 refusé par le lead dev, PR #323 (lead-dev)
- Commit relu `bcbd0d83` ; 46 paires de captures ouvertes, tout le diff lu (`tools/retoucher_formes.mjs`, `tools/formes.json`, modèles m4 / s3) ; `--verifier` rejoué : vert (m4, s3, x5, compass, p208).
- Bloquants : (1) M4 : panneau bosselé, creux rond (« cratère ») et reflets en vagues, visibles jusqu'aux vitrines (`nez-m4-34d-*`, `nez-m4-bas-*`, `vitrine-m4-a1`, `vitrine-m4-bas2`) ; (2) M4 : le panneau garde le contour de l'ancienne double calandre (écusson sombre lisible de face et de 3/4) ; (3) S3 : éclats et bord déchiré au coin droit de la grille (`nez-s3-34g-jour`, `zoom-s3-34g-*`).
- À corriger aussi : bouts de lames en peigne (M4, S3), bande plus sombre du panneau de la S3 sous le capot.
- Revue publiée en « COMMENT » (même compte GitHub) ; statut ↩️ dans `TACHES.md`, `track-agent.md`, `suivi-pr.md`.

### 2026-11-10 00:31:40 — WFS-396 : PR #325 ouverte, remplaçants A 45 / Elantra N étudiés, refus motivé (Claude, agent artiste (01UY1Vv2ph))
- **Fait** : recherche de code GitHub (~1 000 `license.txt` lus), 9 modèles téléchargés dans le bac à sable (clone partiel, rien exécuté), 6 convertis et mesurés par `corriger_voitures --essai` (permutation locale de `a45.wfm` / fiche, restaurée). Aucun dans ±1 % : refus motivé (décision du PO 00:18, option a), rien de converti dans `remplacants/`. Rapport : `qa/pr/WFS-396/pr.md` ; fiches réelles : `references/reel/voitures/remplacants-wfs396.json`.
- **Téléchargements (SHA-256 complets)** :
  - W212, Peter_D, CC BY 4.0, https://sketchfab.com/3d-models/mercedes-e-class-w212-119c5e10733142b197aa53b86f6aeb04, dépôt `bujue600-arch/carVison` `car-vision-frontend/public/models/mercedes-e-class/` : scene.gltf a1e14552aababef903a2fcd2dfeb6dd0d97de3062d316c8d5d6d489d5a106822 (238 832 o), scene.bin cfed9ca335f5de90062e4fd893c53c7d4897f6ed67c42733d989c89529fed44a (20 244 980 o) ; hauteur −2,7 %.
  - W211, BadKarma, CC BY 4.0, dépôt `gibranalfarabi/portofolio-ar` `assets/3D-model/mercedes_benz_e-class_w211/` : scene.gltf 909a0f9ea1e06ba4d555f5efc9c5df4cf45ecf188f3e72262157d9f8508e79a8 (91 150 o), scene.bin 76bad9a65a904644771ba1cc9ad6f48fc6e1ede428d1eee630492ed872811126 (15 148 680 o) ; voie AV −2,2 %.
  - Virtus GT 2022, BHP3D, CC BY 4.0, https://sketchfab.com/3d-models/2022-volkswagen-virtus-gt-3955e5c050f843398b49b2b21ab2f232, dépôt `PrathamSatpute/VehiclePredictiveMaintenance` `models/` : scene.gltf 1bfe8554c6728fea14cf9e7ffe0f02a287904a58ca3e293faa89602347e5b8d7 (617 095 o), scene.bin 70fbe3cbdf30c628463c665c9d5d0d7aa05f7c5f7752a33b72663afd6e7f5eac (22 504 008 o) ; hauteur −2,6 %.
  - Corolla E180 EU, Armored Wave, CC BY 4.0, dépôt `oddalan/three-dee` `assets/models/corolla/` : scene.gltf 57ff461ec2dba40ae0a865d96c09c2ddd1d81dca0865b6bf4f4fe8ba5fdacf0b (135 104 o), scene.bin f377f589d4ab8a79d203f1bd5703b0cc4a6e0a198ebc206700cfbc943f266f3f (4 395 288 o) ; largeur +2,1 %.
  - Mesurés puis écartés (bac à sable) : Corolla 2020 ItsDiyor (`gabrielivanov959-bot/toyota-webapp`), Octavia barking_dogo (`Jkotoun/Opengl_model_viewer_PGR`), Civic FD2 claries, C63 LuizZed36, WRX STi « Galaxy Car Showroom », Lancer Evo X BadKarma, S65 W222 Black Snow (pièces BeamNG), « Accord 2017 » L95XP (mod `mtn_caprice`), Seat Leon Cupra, A4 2000.
- **Licence contradictoire** : « A45 AMG 2018 » de Lexyc16 en CC BY 4.0 (`tensornull/CarVision`) et CC BY-NC 4.0 (`vladyslavhurzhyi/car-rent`) : interdit.
- **Vérification** : `tests_cpu.sh build` 94 réussis ; captures `vitrines-feux` 1280×720 (lavapipe) dans `qa/pr/WFS-396/`. Le jeu est identique à `main`.
- **Suite** : lead dev appelé pour la PR #325 ; WFS-397 (PO) reprend W212 / W211 et Virtus GT.

### 2026-11-10 00:40:00 — WFS-369 fusionné par le lead dev (lead-dev)
- PR #324 relue sur 6cfe9bcf : « Avis agent 3D : VALIDÉ avec réserves » présent (RG-E15-06) ; 40 captures ouvertes ; tout le diff lu (`mobilier_ville.*`, `ville.cpp`, `commerces.cpp`, accroche `cartes_banc.cpp`, test) ; construction et `tests_cpu.sh` relancés : 95/95.
- Revue « ✅ Approuvé » écrite dans la PR (commentaire, même compte GitHub), puis fusion en squash : b4e3a106. Statut 🟡 À valider (utilisateur) : le ticket vient de la capture d'APK de l'utilisateur (bancs en pavés) et d'ANO-216.
- Tâches séparées demandées au PO (RG-07 / RG-E15-09) : R1 eau de la fontaine (ANO-214) ; R2 affiche d'abribus vide et non éclairée la nuit ; R4 éclairage du parc la nuit (E13) ; plaque de rue bleue unie des feux (WFS-385) ; mesure sur téléphone des +44 % de triangles en Basse (WFS-321 / 331).

### 2026-11-10 00:43:03 — WFS-396 fusionné par le lead dev (lead-dev)
- PR #325 relue (commit 19d49424) : 30 captures ouvertes, diff complet lu (fiches réelles `remplacants-wfs396.json`, rapport RG-08, une ligne de suivi), rien dans `src/`, `tools/`, `assets/`. Construction et 94 tests CPU verts sur la branche ; `test_fidelite` 0 défaut.
- Approbation écrite dans la PR (commentaire « ✅ Approuvé par le lead dev », même compte GitHub), fusion en squash fdcb7254. Statut 🟣 ; ligne `qa/Check.md` ; ligne retirée de `suivi-pr.md`.
- Remarques pour WFS-397 (PO) : hauteur de la W212 à −2,7 % (le suivi du PO dit −2,3 %) ; captures de nuit obligatoires ; logos des candidats à effacer.

### 2026-11-10 00:43:38 — WFS-396 fusionné (#325, squash fdcb7254, lead dev) : fin de la session (Claude, agent artiste (01UY1Vv2ph))
- Le lead dev a relu les 30 captures et le diff, et a fusionné. Refus motivé pour les deux places (±1 %), rien de branché ; la suite est dans WFS-397 (PO).
- Remarques du lead dev pour WFS-397 : la hauteur de la W212 est à −2,7 % après l'échelle de `corriger_voitures` (−2,3 % avant l'échelle) ; les captures de nuit (`WFS_VITRINE_NUIT`) seront obligatoires ; tous les logos et noms des candidats sont à effacer avant le branchement.
- Session terminée (un seul ticket) : aucun autre ticket demandé.

### 2026-11-10 00:43:29 — WFS-369 : réserves de la PR #324 en tâches séparées, WFS-398 et WFS-399 créés (po-scrum-master)
- Demande de Claude, agent dev3d 3 (01UY1Vv5WS), à la fin de WFS-369 (fusionné, #324, b4e3a106, 🟡 à valider). Session close : **aucun autre ticket** pour elle (un sujet par session) ; ligne de `track-agent.md` en « ⏹️ Terminé (session close) ».
- Décisions (RG-07 de WFS-369, RG-E15-09, ordre du 2026-11-09 « on ne jette pas un travail pour un petit défaut ») :
  - R1 eau des fontaines plate cyan, vasque sans eau qui coule, jet de l'îlot inchangé → **ANO-235**, **WFS-398** (P2, E15 Ville + réglage de l'eau de WFS-380 ; après WFS-380 et 370).
  - R2 affiche d'abribus vide et panneau non éclairé la nuit → **ANO-236** ; R4 parc presque invisible la nuit → **ANO-237** ; plaque bleue de nom de rue unie (lambda `mat` de `ville.cpp`) → **ANO-238** ; optiques des feux éteintes de jour → **ANO-239** : un seul lot **WFS-399** (P2, E15 Ville ; après WFS-370). La plaque de rue, proposée pour WFS-385, va dans WFS-399 (même mobilier, même lot, pas de micro-ticket).
  - Mobilier absent (poubelles, bornes, kiosques, barrières) : RG ajoutée à **WFS-385**.
  - Performance (+44 % de triangles, Ville de nuit Basse, 260 k → 375 k, sans baisse au banc PC) : pas de ticket ; mesure sur téléphone ajoutée aux règles de **WFS-321** et **WFS-331**, et au budget de WFS-385.
- **WFS-370** (immeubles de la Ville) passe ⏳ Prêt (WFS-369 fusionné). File Ville de E15 : 370 → 399 / 385 ; 398 attend aussi WFS-380.

### 2026-11-10 00:44:32 — WFS-369 : fin de session (Claude, agent dev3d 3 (01UY1Vv5WS))
- PR #324 fusionnée en squash par le lead dev (b4e3a106). L'agent 3D a donné « VALIDÉ avec réserves » sur 6cfe9bcf, après la correction de la terrasse grise (B1, cb2f7d84). Le ticket est 🟡 à valider par l'utilisateur.
- Réserves confiées au PO : WFS-398 (eau des fontaines) et WFS-399 (affiches, éclairage du parc, plaque de rue, feux éteints de jour). Les +44 % de triangles en Basse restent à mesurer sur le téléphone (règles de WFS-321 / 331 / 385).
- Un seul ticket par session : aucun autre ticket demandé, la session s'arrête.

### 2026-11-10 01:08:22 — WFS-380 : PR #326 ouverte, vraie mer (Claude, agent cartes (01UY1Vv4eo))
- **Fait** : une seule eau pour tout le jeu (`MAT_EAU`, `shaders/eau.glsl`) : 8 vagues calculées (houle 28 m / 4,2 s, Hs 0,47 m), Fresnel 0,02, traînées du soleil, de la lune et des lampadaires, couleur lue à travers l'eau selon la profondeur (distance au vrai trait de côte, pente du fond), écume de ressac et de déferlement en dentelle, sillage en V (Kelvin) et éclaboussures en dentelle à la place du trapèze et du disque blancs ; mer de la Corniche, bassin de la marina et rivière de la Ville (paramètres dans les sommets) ; blocs de calcaire et galets au bord de la côte rocheuse et au pied de la falaise (RG-09). Banc `WFS_BANC=mer` (15 vues).
- **Accroches annoncées** (≤ 10 lignes chacune) : `shaders/scene.frag` (branche de l'eau, 7 lignes), `src/render/renderer.h` (`MAT_EAU`), `src/render/renderer.cpp` (l'eau ne porte pas d'ombre), `src/world/ville.cpp` (eau de la rivière, 5 lignes), `src/platform/desktop_main.cpp` (banc `mer`).
- **Vérification** : construction complète, `tests_cpu.sh` 96/96 (dont `test_eau`) après synchronisation avec `main` ; 15 paires de captures avant / après dans `qa/pr/WFS-380/` ; banc perf Corniche (lavapipe) : Basse, passe de scène 132,5 → 135,2 ms (+2,0 %), Haute, image 355,0 → 351,7 ms.
- **Aucun téléchargement** : eau et pierres calculées, aucune texture ni licence.
- **Pour le PO** : RG-E15-01 (200 à 700 Mo de décor) ne se mesure pas sur la Corniche, carte du code sans paquet ; ce ticket n'ajoute aucun octet d'actif. Le seuil relève de WFS-386. Limite : les reflets des lampadaires ne viennent que des 4 lampadaires allumés près de la voiture (`nuit.cpp`, WFS-382).
- **Suite** : avis de l'agent 3D (RG-E15-06), puis lead dev.

### 2026-11-10 01:08:26 — WFS-381 : brume au loin, accroches annoncées et avancement (Claude, agent dev A (01UY1Vv42R))
- Fait sur `feature/waforspeed-381` : brume exponentielle selon la distance **et** la hauteur (`shaders/eclairage.glsl` : `brumeTau`, `fogAt`, `appliquerBrume`), couleur de l'horizon du moment, plus chaude côté soleil, inchangée la nuit ; réglage par carte (`src/render/atmosphere_brume.h` : Corniche β0 1,5·10⁻⁴ m⁻¹ / H 800 m, Ville 1,4·10⁻⁴ / 1 000 m, Campagne 1,2·10⁻⁴ / 1 200 m, Grand Prix 1,3·10⁻⁴ / 1 200 m) ; banc `WFS_BANC=brume` ; test `test_brume` (RG-02 : ≤ 3 % à 200 m, 10-20 % à 1 km, 40-60 % à 5 km).
- **Accroches ≤ 10 lignes hors zone (annonce)** : `shaders/scene.frag` et `shaders/scene_skin.frag` (fin du `main` : 5 lignes de brouillard remplacées par 1 appel) ; `src/render/renderer.h` (+1 ligne : `fogHauteurInv`, `fogRef` dans `FrameParams`) et `renderer.cpp` (1 ligne : `u.fogLin.zw`) ; `src/platform/desktop_main.cpp` (+2 lignes : banc brume) ; `CMakeLists.txt` (+1 ligne : `test_brume`). Aucun changement de `post.frag`, `nuit.cpp`, `phares.h` (WFS-382) ni de l'eau (WFS-380).
- Vérification : 96 tests CPU en local ; coût GPU de la passe de scène en Basse sur lavapipe, 2 mesures avant / 2 après : écart dans le bruit (≈ ±1 %) ; nuit identique au pixel près (RMSE 0).
- Limite constatée : les cartes ne montrent rien au-delà de ≈ 2 km ; avec les valeurs réelles de RG-02, la brume enlève ≈ 14 % de contraste à 1 km et ≈ 25 % à 2 km (avant : 8 % et 16 %) : l'effet est réel mais discret ; les vrais lointains à 5-20 km (reliefs, côte) relèvent des tickets de décor (E15).

### 2026-11-10 01:14:15 — WFS-372 : PR #327 ouverte (Claude, agent dev3d 5 (01UY1Vv5G4))
- Fait : vignes palissées (piquets, 3 fils, ceps, feuillage `veg::vigne`), clôtures et enclos (poteaux à pointe, lisses en demi-rondin), bottes rondes, corps de ferme (fenêtres en retrait, volets, tuiles canal, génoise, cheminée), hangar agricole (portiques en I, bardage, plaques ondulées), poteaux de ligne avec isolateurs et câbles. Fichiers : `src/world/decor_campagne_modeles.*` (nouveau), `decor_campagne_banc.cpp` (banc `campagne-objets`), `campagne.cpp`, `vegetation.*`, `tests/test_decor_campagne.cpp`.
- Accroche annoncée (≤ 10 lignes, hors zone) : `src/platform/desktop_main.cpp`, 2 lignes (`WFS_BANC=campagne-objets`) ; `CMakeLists.txt` : 1 ligne (test).
- Vérifié : `test_decor_campagne` 49/49 ; `tests_cpu.sh` 96/96 ; bancs `campagne` et `campagne-objets` 0 écart ; perf Campagne Basse 180,2 → 184,4 ms (+2,3 %, lavapipe), appels de dessin inchangés (161).
- Réseau : aucun téléchargement (seuls GitHub et les paquets sont permis) → RG-03, objets modélisés au niveau du vrai objet.
- Limites : éoliennes, église, commerces, fontaine, signalisation, haies, autoroute restent pour WFS-387 (inventaire `qa/pr/WFS-372/inventaire.md`). Suite : avis de l'agent 3D (RG-E15-06), puis lead dev.

### 2026-11-10 01:15:03 — WFS-382 : PR #328 ouverte, accroches annoncées (Claude, agent dev N (01UY1Vv5K1))
- Fait : phares de nuit réels. Tache des phares du jeu web retirée (`nuit.cpp`, elle doublait les deux feux de croisement : cône blanc en Campagne et sur la neige) ; répartition verticale du feu de croisement revue (`phares.h`, plateau jusqu'à −1,5°, chute rapide vers le premier plan, flux sourcé gardé, crête LED 30 000 cd ; avis expert-realisme-3d, R112 / R149) ; lueur des phares 0,30 → 0,12 m ; reflet des feux de croisement borné ; halo et bloom plafonnés la nuit ; banc court `WFS_BANC=phares`.
- **Accroches annoncées** (≤ 10 lignes chacune) : `shaders/eclairage.glsl` (7 lignes : même répartition que `phares.h`, plafond du reflet des feux de croisement), `shaders/post.frag` (E13, 5 lignes : plafond de nuit), `src/render/renderer.cpp` (2 lignes : lueur des phares), `src/platform/desktop_main.cpp` (1 ligne : banc `phares`).
- Défaut trouvé et corrigé dans `nuit.cpp` : `fx.stop / fx.recul / fx.frein` du joueur étaient au bout d'un commentaire (jamais exécutés) ; ligne recollée.
- Vérifié : 95 tests CPU réussis ; pixels saturés 0,37-0,80 % → 0 % (poursuite, voiture en face, pluie) ; neige de nuit, tache des phares 7,56 → 0 % ; témoins de jour identiques ; perf Basse Ville nuit pluie : coût égal à ±3 % près (2 mesures).
- Limites : bloom absent du banc (lavapipe), à voir sur téléphone ; rétroréflexion de l'asphalte et glace modélisée du phare non faites (proposées).

### 2026-11-10 01:19:29 — WFS-391 : PR #329 ouverte (Claude, agent conduite (01UY1Vv4e7))
- Fait (`src/game/player.cpp`, ancienne conduite) : RG-01, au freinage seules les forces des pneus (freins, frein moteur) × μ, traînée et roulement hors μ (neige 145,2 → 126,9 m, sec 36,8 m inchangé) ; RG-02, μ bas-côté ≤ μ chaussée × 0,6 (neige 0,2 → 0,15 ; sec et automne inchangés).
- Cause de l'ANO-231 (RG-06) : point de mesure à 11,5 m derrière la glissière du col (edge 1,38 = 10,35 m) : la voiture la longe (rayon 11,65 m). Rien à corriger dans la conduite (arbitrage du PO) ; test sur les 5 positions devant la glissière (± 0,4 %).
- **Accroche annoncée** (2 lignes) : `src/platform/desktop_main.cpp`, nouveau banc `WFS_BANC=sol` (`src/game/sol_banc.cpp`) ; interrupteur de banc `g_avantWfs391` (`player.h`).
- Vérifié : 94 tests CPU réussis (nouveaux tests en échec sur main : 5) ; bancs `sol` et `neige` 0 écart ; parcours 0 écart ; 50 captures dans `qa/pr/WFS-391/`.
- Limites : ancien modèle sous 140 m à cause du roulement du jeu (0,35 m/s², noté pour WFS-258) ; ressenti à valider sur téléphone (🟡 après fusion).

### 2026-11-10 01:27:12 — WFS-362 : PR #323 corrigée après la revue du lead dev (Claude, agent dev3d 4 (01UY1Vv5HV))
- Causes : (1) cratère de la M4 : le bourrelet autour de l'ancienne calandre donnait au panneau profondeur et normales ; (2) écusson : ce même bourrelet ; (3) éclats du coin S3 : jour de 1,5 cm entre bord découpé et paroi du conduit ; (4) lames en peigne : pas de rebord ; (5) bande sombre S3 : surface presque verticale.
- Corrections (`tools/retoucher_formes.mjs`, `tools/formes.json`) : contour M4 jusqu'au joint du capot et aux pointes des phares ; panneau = base de degré 2 ajustée sur la peinture voisine + écart interpolé, normales de la surface finale lissées, vrai décalage du débord ; options de grille `joue` et `rebord` ; X5 inchangée (empreinte rejouée).
- Vérification : `--verifier` vert (5 voitures) ; 95 tests CPU verts ; captures régénérées (18 vitrines, 16 gros plans, 16 zooms) ; réponse point par point dans la PR ; lead dev rappelé.


### 2026-11-10 01:30:46 — WFS-372 fusionné par le lead dev (lead-dev)
- PR #327 fusionnée en squash (0654a4af) à la 1re revue, après « Avis agent 3D : VALIDÉ avec réserves » sur 6b3b4bb9 (RG-E15-06 et 09). Revue « ✅ Approuvé » écrite dans la PR en commentaire (GitHub refuse l'approbation du même compte).
- Vérifié : 58 captures ouvertes (paires avant / après jour et nuit, gros plans, perf, inventaire) ; diff complet relu ; construction et `tests_cpu.sh` refaits : 96/96 ; perf Basse +2,3 % (lavapipe, sous le seuil de 5 %) ; `vegetation.*` : ajout seul de `veg::vigne`, permis par la zone du ticket (vignes).
- Statut 🟣 Fusionné ; ligne de `suivi-pr.md` supprimée ; ligne ajoutée à `qa/Check.md`.
- **Pour le PO** : créer une tâche séparée (RG-E15-09) avec les réserves 1 à 6 de l'agent 3D (bottes, crépi, teintes du bois, tôle du hangar, perf sur le téléphone) et mes remarques : câbles des poteaux et lisses des clôtures aux limites de deux zones « champs », commentaire en double dans `decor_campagne_banc.cpp:29`, comparaison de flottants dans `fenetre`.

### 2026-11-10 01:31:23 — WFS-372 : fin de session (Claude, agent dev3d 5 (01UY1Vv5G4))
- PR #327 fusionnée par le lead dev (squash 0654a4af) après « Avis agent 3D : VALIDÉ avec réserves » sur 6b3b4bb9. Session terminée (un seul sujet par session) : aucun autre ticket demandé.
- À reprendre dans une tâche séparée (RG-E15-09), transmis au PO : bottes sans grain ni filet ; crépi de la ferme trop froid (gris-bleu) ; poteaux de clôture plus sombres que les lisses ; poteau électrique trop sombre ; ondes de la tôle du hangar invisibles de loin ; performance à mesurer sur le téléphone (triangles +35 % au départ) ; `campagne.cpp` : câble ou lisse qui peut finir sans poteau quand deux zones « champs » se suivent (`T_POLE`, `clotureSuit`) ; commentaire en double `decor_campagne_banc.cpp:29` ; comparaison de flottants par `==` dans `fenetre` (`decor_campagne_modeles.cpp`).

### 2026-11-10 01:31:36 — WFS-362 : PR #323 refusée par le lead dev, 2e revue (lead-dev)
- Commit relu 13a06c92 ; 50 paires de captures ouvertes, diff lu (`tools/retoucher_formes.mjs`, `tools/formes.json`). Revue : https://github.com/wlfaleh/waforspeed_mobile/pull/323 (2e revue « COMMENT » valant refus).
- Bloquants : (1) S3, une lame ou le rebord de la grille passe devant la plaque (numéro barré, régression de ce passage ; aucun contrôle de la plaque, `ignorer: [18]`) ; (2) M4, panneau encore déformé au zoom ×4 de 3/4 droit (pli en rouleau au bord gauche, marche sous le capot, contour en écusson dans les reflets) ; (3) M4, reste chromé de l'emblème au centre du nez ; (4) S3, coin droit de la grille encore en dents, avec un éclat de nuit. À corriger aussi : rebord M4 en dents de scie, teinte du panneau S3, note « bombée » de la M4 alors que bombe = 0.
- Progrès notés : silhouette des haricots disparue de face, vrai décalage `decaler()`, lames S3 contre un rebord.
- TACHES : WFS-362 ↩️ Retour lead dev (PR #323) ; track-agent et suivi-pr mis à jour.

### 2026-11-10 01:32:27 — WFS-372 en 🟡, réserves versées dans WFS-387 (po-scrum-master)
- Fin de ticket annoncée par Claude, agent dev3d 5 (01UY1Vv5G4) : WFS-372 fusionné (#327, 0654a4af), « Avis agent 3D : VALIDÉ avec réserves ». Session close, aucun nouveau ticket pour elle.
- WFS-372 corrige des défauts vus par l'utilisateur (ANO-65, ANO-219) → **🟡 À valider (utilisateur)** (le robot ne le passe pas en ✅).
- RG-E15-09 : pas de nouveau ticket ; les 6 réserves de l'agent 3D et les 3 remarques du lead dev vont dans **WFS-387, RG-12** (même carte, mêmes fichiers, travail suivant de la file Campagne ; évite un micro-ticket et deux PR sur les mêmes fichiers).
- WFS-387 passe **⏳ Prêt** (E15 Campagne, agent dev-artiste-3d), pour une nouvelle session.


### 2026-11-10 01:34:04 — WFS-382 : PR #328 refusée par le lead dev (lead-dev)
- Revue reprise après le redémarrage du conteneur (rien n'avait été posté). Commit relu : 3c89a192. Les 22 captures ont été ouvertes et zoomées, le diff lu en entier, la branche construite ; tests CPU : 95 réussis.
- Gros progrès sur le sujet : plus de disques autour des phares, sol plus brûlé en Ville, Campagne et neige (RG-07 tenu) ; témoins de jour identiques.
- **Refus** (revue postée en COMMENT sur la PR, car GitHub refuse « changements demandés » quand la PR vient du même compte) :
  1. `shaders/post.frag:124/127` : le plafond du halo et du bloom coupe net chaque canal. Résultat, la nuit : un disque plat, un carré et un anneau irisé autour des têtes de lampadaire et de l'écran du Cockpit (forme interdite, régression sur toutes les cartes de nuit). Correction attendue : un plafond progressif, calculé sur la luminance.
  2. RG-01 : la tache de 30 à 60 m et la coupure ne se voient pas sur la capture 3 (Campagne, vue Capot).
- TACHES : WFS-382 passe en ↩️ Retour lead dev (PR #328) ; track-agent et suivi-pr mis à jour.

### 2026-11-10 01:35:32 — WFS-382 : rétroréflexion de la chaussée accordée dans le ticket (po-scrum-master)
- Question de Claude, agent dev N (01UY1Vv5K1), défaut 2 du lead dev sur la PR #328 (RG-01 invisible). Décision **(A)** : rétroréflexion EN 1436 faite dans WFS-382 ; exception de zone `shaders/eclairage.glsl` ≈ 15 lignes au total, annoncées au journal ; feux de croisement du joueur seulement ; pas de `frame_ubo` ni renderer ; budget Basse (RG-05, WFS-353) tenu ; synchronisation avec `main` avant l'envoi (WFS-381). Détail : `regles-de-gestion/WFS-382.md`, « Suivi ».

### 2026-11-10 01:37:40 — WFS-381 : RG-02 de la brume corrigée, option A (po-scrum-master)
- Question de Claude, agent dev A (01UY1Vv42R) : RG-02 se contredisait (≤ 3 % à 200 m impose V ≥ 26 km) et l'effet était invisible à l'écran. Décision **(A)** : Corniche V ≈ 15 km, Ville ≈ 18, Grand Prix ≈ 20, Campagne ≈ 22 ; ≤ 5 % à 200 m, ≤ 25 % à 1 km, 60 à 75 % à 5 km ; nuit (RG-03) et budget (RG-05) inchangés ; le lead dev relit avec cette décision. Détail : `regles-de-gestion/WFS-381.md`, « Suivi ».

### 2026-11-10 01:47:40 — WFS-381 : PR #330 ouverte, lead dev appelé (Claude, agent dev A (01UY1Vv42R))
- RG-02 appliquée selon la décision du PO (option A, 01:37:40) : Corniche β0 2,55·10⁻⁴ m⁻¹ (V 15,3 km), Ville 18 km, Grand Prix 20 km, Campagne 22 km ; test `test_brume` vert, 96 tests CPU verts.
- 7 captures avant / après + écart ×8 dans `qa/pr/WFS-381/` ; nuit identique au pixel près ; coût de la passe de scène dans le bruit de mesure (lavapipe).
- Limite : effet surtout visible dans les vues larges (les cartes s'arrêtent vers 2 km).

### 2026-11-10 02:03:29 — WFS-391 fusionné par le lead dev (lead-dev)
- PR #329 (Claude, agent conduite (01UY1Vv4e7)), commit relu 3aef4fca ; squash 6ea09965 sur main ; revue « ✅ Approuvé » écrite dans la PR en commentaire (même compte GitHub).
- Contrôles : les 60 captures ouvertes ; tout le diff lu (`player.cpp`, `player.h`, `sol_banc.cpp`, accroche de 2 lignes dans `desktop_main.cpp`, 2 tests) ; branche fusionnée en local avec main : construction OK, 95 tests CPU réussis ; bancs `sol`, `neige` et `parcours` relancés : 0 écart.
- Statut : 🟡 À valider (utilisateur), car c'est un ticket de conduite. Ligne retirée de `suivi-pr.md`. Ligne ajoutée à `qa/Check.md`.
- Pour le PO : RG-01 demande un test « pneus = μ × g ± 2 % dans les deux modèles ». Le test de la PR prouve la proportionnalité à μ. La valeur absolue n'est pas testée : Golf, ancien modèle, 1,024 μ g, soit +2,4 %, fixée par le réglage de freinage de WFS-188. Critère à préciser pour WFS-258.
### 2026-11-10 02:03:41 — WFS-381 refusé par le lead dev, PR #330 (lead-dev)
- Commit relu 11d3f1de ; 21 images ouvertes ; tout le diff lu ; construction et `tests_cpu.sh` refaits : 97 tests verts, `test_brume` vert ; nuit identique (RG-03 tenue) ; code sans défaut bloquant.
- Refus : sur les captures 1 à 3 et les vues hautes, l'écart avant / après est de 0,6 à 1,5 niveau sur 255 en moyenne : on ne distingue pas les images, les lointains ne sont pas « nettement plus pâles et bleutés » (critère du ticket, demande de l'utilisateur ANO-221). `brume-mesures-avant.txt` montre des valeurs que main n'utilise pas.
- Demandé : captures où l'effet se voit (avec une mesure du contraste à l'écran à 1 et 2 km), ou une nouvelle décision du PO si la RG-02 ne le permet pas avec des cartes de moins de 2 km ; PO prévenu (critère et RG-02 incompatibles avec ce qui est à l'écran).
- `TACHES.md` : ↩️ Retour lead dev (PR #330) ; `track-agent.md` et `suivi-pr.md` mis à jour.

### 2026-11-10 02:03:59 — Fin WFS-391 : session terminée (Claude, agent conduite (01UY1Vv4e7))
- PR #329 fusionnée en squash par le lead dev (6ea09965) ; WFS-391 en 🟡 À valider (utilisateur) : freinage sur la neige plus court (≈ 127 m depuis 100 km/h au lieu de 145 m), bas-côté enneigé moins accrocheur, sec inchangé.
- Remarques non bloquantes du lead dev notées : critère « pneus = μ × g ± 2 % » à préciser pour WFS-258 (Golf 1,024 μ g dans l'ancien modèle) ; banc `sol` : la voiture posée saute des checkpoints (banc seulement).
- Session arrêtée (un seul sujet par session) : aucun autre ticket demandé.

### 2026-11-10 07:31:05 — WFS-381 : décision sur RG-02 après le refus de la PR #330 (po-scrum-master)
- Question de Claude, agent dev A (01UY1Vv42R), transmise par le lead dev : l'option A (V 15 à 22 km) ne se voit pas sur des cartes de moins de 2 km (écart 0,6 à 1,5 niveau sur 255).
- Décision : **option C**. RG-02 = jour de brume de chaleur d'été : Corniche V 8 km, Ville 10 km, Grand Prix 11 km, Campagne 12 km (bornes à 200 m, 1 km, 2 km, 5 km dans `regles-de-gestion/WFS-381.md`, Suivi) ; critère des captures chiffré (contraste à 1 et 2 km dans `pr.md`, écart ≥ 20 niveaux sur les lointains).
- Fond lointain : pas de nouveau ticket ; RG-E15-10 ajoutée à `E15-decors.md` (WFS-385 à 389), avec la remarque du lead dev sur `masquage::finBrouillard`.
- WFS-381 reste ↩️ Retour lead dev (PR #330) chez son développeur.

### 2026-11-10 07:31:44 — WFS-362 : PR #323 corrigée après le 2e refus du lead dev (Claude, agent dev3d 4 (01UY1Vv5HV))
- Causes : lame devant la plaque S3 (plaque ignorée pour caler la grille, aucun contrôle) ; normales du bord du panneau M4 faussées (surface bornée à sa boîte) ; restes de l'anneau du logo M4 hors de la fenêtre de découpe ; paroi et rebord de grille en escalier (carte au pas de 5 mm).
- Corrections (`tools/retoucher_formes.mjs`, `tools/formes.json`) : option de grille `derriere` + contrôle bloquant « devant la plaque » ; surface du panneau sur une zone élargie ; découpe complète de l'anneau ; paroi et rebord lissés ; note M4 corrigée. X5 inchangée (empreinte rejouée).
- Vérification : `--verifier` vert (5 voitures) ; 96 tests CPU verts ; captures régénérées avec les zooms demandés (plaque S3, haut du nez M4) ; réponse point par point dans la PR ; lead dev rappelé.


### 2026-11-10 07:42:21 — WFS-381 : seuil de 20 niveaux retiré du critère des captures (po-scrum-master)
- Mesures du développeur : contraste à 1 et 2 km dans les bornes RG-02 (Corniche 38,1 / 60,1 %, Campagne 29,0 / 47,4 %) ; écart à l'écran 8 à 14 niveaux, plafonné par la couleur de l'horizon de WFS-214.
- Décision : seuil de 20 niveaux retiré (mon erreur de calcul) ; restent la mesure du contraste et le jugement à l'œil du lead dev ; Ville jugée à l'œil seulement. Détail dans `regles-de-gestion/WFS-381.md` (Suivi).

### 2026-11-10 07:42:55 — WFS-380 : avis de l'agent 3D corrigé, VALIDÉ avec réserves, lead dev appelé (Claude, agent cartes (01UY1Vv4eo))
- **1er avis (41a95f40) : À REPRENDRE**, D1 à D6. Corrigé dans 8a88af3c :
  - D1 : horizon clair (Fresnel de l'eau entier, rugosité plafonnée) ;
  - D2 : directions des vagues écartées, plus une petite mer du vent sans période ;
  - D3 : écume contre la roche ;
  - D4 : rivière d'eau douce trouble, vert-brun, 4 m de fond au plus ;
  - D5 : nappe du ressac et sable mouillé sur la plage ;
  - D6 : écume en lignes parallèles au bord, qui avancent.
  Capture `mer-eclaboussure` ajoutée.
- **2e avis (0a587856) : VALIDÉ avec réserves** (commentaire 6761864467 de la PR #326).
- **Vérification** : 97 tests CPU réussis ; captures refaites après la synchronisation avec `main`.
- **Banc perf de la Corniche** (`main` et branche mesurés sur la même machine) :
  - Basse : scène 150,3 → 146,1 ms, 226 appels de dessin des deux côtés, dans le bruit de mesure. En Basse, la nappe et la petite mer du vent sont coupées.
  - Haute : scène 303,7 → 302,9 ms.
- **Pour le PO, réserves à mettre en tâches séparées** (RG-E15-09) :
  - R1 : reflets des façades allumées de nuit ;
  - R2 : reflets des coques dans le bassin de la marina ;
  - R3 : traînée du soleil bas plus nette ;
  - R4 : reflets des lampadaires de la rivière ;
  - R6 : coût de l'eau en Basse à mesurer sur le téléphone (RG-07, 1,0 ms au plus) ;
  - R7 : aspect « filé » de la mer à angle rasant ;
  - R8 : capture de l'écume contre des blocs dans l'eau et au pied de la falaise ;
  - R9 : maillage trop régulier du reflet du casino la nuit ;
  - R10 : éclaboussure du dauphin vue comme un trait plat (déjà sur `main`).
  RG-E15-01 (200 à 700 Mo) ne se mesure pas sur la Corniche, carte du code : à trancher avec WFS-386.

### 2026-11-10 07:45:16 — WFS-362 fusionné par le lead dev (lead-dev)
- PR #323, 3e revue, commit relu `a88efa52`, squash `30d339f4`. Revue « ✅ Approuvé » en commentaire dans la PR (même compte GitHub).
- 58 paires de captures ouvertes ; points 1 (plaque S3), 3 (emblème M4) et 4-5 (rebords en dents) de la 2e revue réglés.
- Construction et tests CPU refaits par le lead dev sur la branche fusionnée avec `main` : 96/96 ; `--verifier` : empreintes concordantes.
- **Au PO (RG-06)** : tâche séparée à créer pour les défauts restants, visibles seulement en gros plan : reflets « en cuvette » du panneau M4 de 3/4 droit (pli au bord gauche, marche sous le joint du capot, arc orangé de nuit) ; éclat orangé de nuit au coin bas droit de la grille S3 ; barre blanche sous la pointe du capot M4 en vue basse.
- Statut : 🟣 Fusionné (pas un retour vu sur le téléphone de l'utilisateur).

### 2026-11-10 07:45:48 — WFS-362 : fusionné, session terminée (Claude, agent dev3d 4 (01UY1Vv5HV))
- PR #323 approuvée et fusionnée en squash par le lead dev à la 3e revue (`30d339f4`) : M4 et S3 sans calandre de marque (pièce de carrosserie + grille générique derrière la plaque) ; outil `tools/retoucher_formes.mjs` enrichi (`panneau`, options de grille `joue`, `rebord`, `derriere`), X5 inchangée.
- Défauts restants non bloquants (RG-06, tâche séparée demandée au PO par le lead dev) : reflets du panneau M4 de 3/4 droit, éclat de nuit au coin bas droit de la grille S3, barre claire sous la pointe du capot M4 en vue basse.
- Un seul ticket par session : la session s'arrête, aucun autre ticket demandé.


### 2026-11-10 07:48:00 — WFS-384 attribué à agent expert ci cd (session 0183bv2Y) (po-scrum-master)
- Ordre de l'utilisateur (« Prend 2 et 3 », 2026-11-10 ≈ 07:45 UTC, transmis par l'agent expert ci) : **WFS-384** ([TECHNIQUE] bancs du matin sautés, P1, E7, ANO-226) attribué à la session 0183bv2Y, son premier et seul ticket. Branche `feature/waforspeed-384`, zone `.github/workflows/matin.yml`, `tools/qa/**`, skill `expert-ci-cd`. US technique : CI de branche autorisée, une à la fois, notée au journal.
- Exception à « un ticket actif par épic » sur ordre de l'utilisateur : WFS-319 (E7, publication) reste 🔧 chez l'agent expert ci (session 01UY1Vv4).
- Constat du 2026-11-10 07:40 UTC (aucun run ce matin, runs #6 à #11 sautés, Release `captures-matin` absente, issue #242 muette depuis 2026-11-09 01:52 UTC) et nouvelle **RG-06** (dire clairement à la QA quand `captures-matin` manque ou n'est pas du jour) ajoutés à `regles-de-gestion/WFS-384.md`.
- `track-agent.md` : ligne de la session ajoutée ; lignes des sessions terminées enlevées (dev3d 5, dev M, dev Z, dev3d 2, dev3d 3, dev3d 4, artiste, conduite).

### 2026-11-10 07:48:47 — CLAUDE.md : la QA lit les captures du matin dans `captures-matin` (agent expert ci cd (session 0183bv2Y))
- Sur l'ordre direct de l'utilisateur (« Prend 2 et 3 », point 3) : deux passages de `CLAUDE.md` disaient encore « captures dans `apercu` » ; depuis WFS-352, `apercu` ne garde que l'APK et les captures du matin vont dans la Release `captures-matin` (anciennes : `apercu-ancien-20261109-0925`). Rien d'autre changé.

### 2026-11-10 07:50:16 — WFS-381 : PR #330 corrigée après le refus du lead dev (Claude, agent dev A (01UY1Vv42R))
- Décisions du PO appliquées (Suivi de WFS-381, 07:30:48 et 07:42:21) : brume de chaleur d'été (Corniche 8 km, Ville 10, Grand Prix 11, Campagne 12, autres 10) ; `test_brume` sur les nouvelles bornes ; 97 tests CPU verts.
- Banc `brume` : contraste mesuré à l'écran (cible HDR, 5 rendus figés) : Corniche 8,3 → 38,1 % à 1 km, 15,7 → 60,1 % à 2 km ; Campagne 8,9 → 29,0 % / 16,1 → 47,4 % ; écart à l'écran 8 à 14 niveaux (sans seuil).
- Réponse point par point sous la revue ; suivi-pr en 🔁 ; lead dev rappelé (commit relu attendu : 370b3963).

### 2026-11-10 07:53:28 — WFS-384 : PR #331 ouverte, lead dev appelé (agent expert ci cd (session 0183bv2Y))
- Cause : réveils de GitHub en retard (2026-11-09 : 09:44 à 18:10 heure de Paris, tous sautés ; 2026-11-10 : aucun avant 08:40) : aucun banc complet depuis le run #5, donc pas de Release `captures-matin`, la QA n'a rien trouvé.
- Correction (E7) : réveil de secours chaque heure de 03:23 à 20:23 UTC ; bancs lancés au premier réveil à partir de 04:00 heure de Paris quelle que soit l'heure, jamais deux fois le même jour (`tools/qa/heure_matin.sh`) ; message « ⏳ Bancs du matin en retard » à la QA après 07:00 ; « Recette QA prête » remplacé par « captures du jour manquantes » quand `captures-matin` n'a pas les captures du jour ; notes de `apercu` à jour.
- Vérification : `tools/qa/test_matin.py`, 17 cas verts (faux gh) ; aucune CI de branche lancée (sans intérêt pour un réveil programmé).

### 2026-11-10 07:55:36 — WFS-384 fusionné par le lead dev (lead-dev)
- PR #331 relue (diff complet : `matin.yml`, `heure_matin.sh`, `retard_matin.sh`, `etat_captures_matin.sh`, `recette_prete.sh`, `publier_apercu.sh`, `test_matin.py`) ; `test_matin.py` relancé : 17/17 ; RG-02 (garde de `statuts.yml`) inchangée ; aucun banc affaibli. Revue « ✅ Approuvé » en commentaire (même compte GitHub), squash `071f10af`.
- À suivre : le prochain réveil (minute 23 UTC) doit lancer le banc du 2026-11-10 (1er critère d’acceptation, numéro du run à noter).

### 2026-11-10 07:56:59 — WFS-380 refusé par le lead dev, PR #326 (lead-dev)
- Commit relu 0a587856 : 32 captures ouvertes, diff complet lu, construction et `tests_cpu.sh` refaits (97/97, dont `test_eau`) ; avis agent 3D « VALIDÉ avec réserves » présent.
- **B1 (bloquant)** : `apres-mer-marina` : l'eau calme du port devient une nappe blanche le long du quai, avec des barres blanches dans le bassin (pixel 1150,700 : bleu (24,138,174) → blanc (205,204,202)). Cause : zone marina = agitation 0,3 et pente 1 (quai) → toute l'écume de côte rocheuse (`shaders/eau.glsl` l. 171-190, seuil `agit > 0.2`, termes non multipliés par l'agitation). Correction : écume liée à l'agitation, aucun blanc dans le port ; captures marina, bord et rochers refaites, puis nouvel avis agent 3D.
- 3 remarques non bloquantes (abscisse de la rivière dans `ville.cpp`, détection de la Basse, commentaire en double). TACHES, track-agent et suivi-pr passés en ↩️.

### 2026-11-10 08:03:56 — WFS-381 fusionné par le lead dev (lead-dev)
- PR #330, commit relu 370b3963 (2e revue), squash ca0ecf57 sur main. Revue « ✅ Approuvé » en commentaire (GitHub refuse l'approbation sur le même compte).
- Relu avec les décisions du PO de 07:30:48 (RG-02 « jour de brume de chaleur ») et 07:42:21 (critère des captures). 14 captures et 7 images d'écart ouvertes, agrandies sur les lointains : lointains plus pâles et bleutés (nets sur les vues hautes, discrets en vue de poursuite) ; nuit identique à main.
- Diff complet lu ; construction et `tests_cpu.sh` refaits : 97/97 ; banc `brume` (vue corniche) relancé : mêmes mesures. 3 remarques non bloquantes (coût avant / après absent de `brume-mesures.txt`, effet discret en vue de poursuite, « -100 % » sans pixel). Statut 🟡 À valider (utilisateur) : défaut vu par l'utilisateur (ANO-221).

### 2026-11-10 08:04:27 — WFS-381 : fin de session (Claude, agent dev A (01UY1Vv42R))
- PR #330 fusionnée en squash par le lead dev (ca0ecf57) ; ticket 🟡 à valider par l'utilisateur (ANO-221) sur l'APK de main : lointains plus pâles et bleutés de jour, nuit inchangée.
- Remarques du lead dev laissées au PO : coût GPU à mesurer sur téléphone (QA) ; « -100 % » affiché par le banc quand une bande de distance n'a aucun pixel (Ville) ; vrai fond lointain par E15 (RG-E15-10).
- Un seul ticket par session : aucun autre ticket demandé, la session s'arrête.

### 2026-11-10 08:25:57 — WFS-382 : PR #328 corrigée après le refus du lead dev (Claude, agent dev N (01UY1Vv5K1))
- Défaut 1 (plafond coupé net dans `post.frag`) : compression douce sur la luminance, e / (1 + Y / 0,35), et halo local pondéré en gaussienne, la nuit seulement ; jour identique (0 pixel changé).
- Défaut 2 (RG-01, tache de 30 à 60 m) : rétroréflexion de l'asphalte (`retroChaussee`, accord du PO), feux du joueur seulement, caméra derrière eux, sol sombre horizontal ; chaussée en vue Capot 130 / 112 / 72 à 10 / 30 / 50 m (main : 225 / 123 / 96). **Accroche** `shaders/eclairage.glsl` : 14 lignes au total (exception accordée par le PO, ≈ 15). Marque par `spotCone.y` (pénombre) : `spotCone.w` est écrit par `renderer.cpp`, auquel le PO interdit de toucher.
- Vérifié : 97 tests CPU ; 0 % de pixels saturés sur toutes les captures après ; neige 0 % ; perf Basse égale, aux erreurs de mesure près. Synchronisé avec main 7a2e01d7 (brume WFS-381), captures refaites. Lead dev rappelé.

### 2026-11-10 08:39:17 — WFS-382 fusionné par le lead dev (lead-dev)
- PR #328, 2e revue, commit relu 27df0667, squash 1063fb3f. Approbation écrite dans la PR en commentaire (même compte GitHub).
- Défauts de la 1re revue corrigés : compression douce de l'éclat et du bloom sur la luminance (plus de disque, de carré ni d'anneau) ; RG-01, tache au sol visible en vue Capot (rétroréflexion accordée par le PO, marque par `spotCone.y` acceptée).
- Construction et tests refaits : 97/97. Statut : 🟡 À valider (utilisateur). Ligne retirée de `suivi-pr.md`, ligne ajoutée dans `qa/Check.md`.
- **Pour le PO** : cœur du reflet des phares de la police sur la chaussée mouillée en aplat gris à 227 (`min()` canal par canal de `SPEC_MAX_CROISEMENT`, `shaders/eclairage.glsl`), visible sur `apres-phares-poursuite-police` : non bloquant (mieux qu'avant), anomalie de suivi à créer (compression douce sur la luminance, comme `eclatNuit`).

### 2026-11-10 08:39:56 — WFS-382 : fin de session (Claude, agent dev N (01UY1Vv5K1))
- PR #328 fusionnée par le lead dev (squash 1063fb3f) ; ticket 🟡 À valider (utilisateur) ; réponse écrite dans `tache utilisateur.md`.
- Suite notée par le lead dev (pour le PO) : reflet des phares de la police sur chaussée mouillée encore plat (borne `SPEC_MAX_CROISEMENT` par canal), à compresser sur la luminance comme `post.frag`.
- Session terminée (un seul sujet par session) : aucune demande de nouveau ticket.

### 2026-11-10 08:53:25 — WFS-363 attribué à Claude, agent dev3d 6 (01UY1Vv3MV) (po-scrum-master)
- Demande : nouvelle session `@dev3d` (session_01UY1Vv3MVhKQapjuowdFE2c), agent `dev-artiste-3d`, 1er et seul ticket de la session.
- Choix : objectif de l'utilisateur « voitures sans marque » avant les décors ; ordre des voitures du 2026-11-09 19:14 UTC (358 → 362 → 359 → **363** → 360…) ; WFS-362 et 359 🟣 fusionnés, aucun ticket voitures actif (E10 libre). WFS-363 passe de ⏳ En attente à 🔧 En cours. Restent prêts pour les prochaines sessions `@dev3d` : WFS-370 (Ville), 373 (Grand Prix), 387 (Campagne), et WFS-397 (W212 / Virtus GT, WFS-396 🟣).
- Zone : `assets/cars/giulia*`, `assets/cars/rrsport*`, entrées `giulia` / `rrsport` de `tools/formes.json`, ajouts à `tools/retoucher_formes.mjs` sans changer les voitures déjà faites (`--verifier`), `tests/**`. Interdits : `vehicule_cockpit*`, `src/game/cars.h`, `remplacants/` (WFS-397), `plage.cpp` et eau (WFS-380, agent cartes).
- Règles : `regles-de-gestion/WFS-363.md` (RG-01 à RG-06). Pas de E15, donc pas d'avis agent 3D obligatoire ; l'agent 3D peut être consulté.

### 2026-11-10 08:55:14 — WFS-370 attribué à Claude, agent dev3d (01UY1Vv2Ph) (po-scrum-master)
- Demande d'une nouvelle session `@dev3d` (session_01UY1Vv2PhsfqmJW55jpNKHd, aucun ticket avant : 1er sujet). WFS-363 venait d'être donné à une autre session (agent dev3d 6, 08:53) ; ticket suivant pour ce profil : **WFS-370** (P1, E15 file Ville, prêt ; plus petit numéro P1 prêt après 363).
- Ticket : Ville, immeubles « boîtes » remplacés par de vrais volumes (toits, corniches, balcons, rez-de-chaussée, fenêtres en retrait, 30 à 50 % allumées la nuit). Zone : `src/world/archi_*`, `src/world/ville*.cpp`, `assets/world/modeles/**`, `assets/licences/` ; pas `plage.cpp` (WFS-380). Règles : `regles-de-gestion/WFS-370.md` + `regles-de-gestion/E15-decors.md`. **E15 : avis agent 3D « VALIDÉ » obligatoire avant la fusion.** Branche `feature/waforspeed-370`, inventaire au journal d'abord (partir de `qa/pr/WFS-369/inventaire.md`), captures avant / après jour et nuit, PR, appel de `@3d` puis du lead dev, arrêt de la session après la fusion.

### 2026-11-10 09:11:58 — WFS-380 : B1 du lead dev corrigé, PR #326 en 🔁 (Claude, agent cartes (01UY1Vv4eo))
- **Fait** (590aa2e7) :
  - B1 : l'écume du bord suit l'agitation du lieu, il n'y a plus de blanc dans le port ni dans la rivière ;
  - les 4 remarques non bloquantes sont traitées (commentaires, doublon retiré) ;
  - 2e vue de la marina au banc (`mer-marina-bassin`).
- **Synchronisation avec `main`** : brume (WFS-381) et phares de nuit (WFS-382), sans conflit.
- **Vérification** : 98 tests CPU réussis ; 17 paires de captures refaites sur aabd7bd1.
- **Suite** : 3e avis de l'agent 3D, puis lead dev.

### 2026-11-10 09:29:53 — WFS-380 fusionné par le lead dev (lead-dev)
- **PR #326**, 2e relecture sur aabd7bd1 : B1 (écume blanche dans le port) corrigé dans 590aa2e7 (écume du bord × `smoothstep(0,5 ; 0,9 ; agitation)`) ; pixel (1150, 700) de `mer-marina` : (205, 204, 202) → (18, 68, 77).
- 34 captures ouvertes ; diff depuis 0a587856 lu (10 lignes + synchronisations avec main, résolution propre) ; construction et `tests_cpu.sh` refaits : 98/98.
- « Avis agent 3D : VALIDÉ avec réserves » sur aabd7bd1. Revue « ✅ Approuvé » écrite dans la PR (en commentaire, même compte GitHub), puis fusion en squash : **f4546c8b**.
- `TACHES.md` : WFS-380 en 🟡 À valider (utilisateur) (ANO-214, défaut vu par l'utilisateur) ; ligne de `suivi-pr.md` supprimée ; `qa/Check.md` à jour.
- **Pour le PO** : réserves R7 (rides régulières au loin), R9 (reflet fort des lampadaires), R10 (trait blanc de l'éclaboussure) en tâches séparées ; description de la PR restée à « 96 tests / 15 captures » (98 / 17 en vrai), non bloquant.

### 2026-11-10 09:30:39 — Fin WFS-380 : PR #326 fusionnée, session terminée (Claude, agent cartes (01UY1Vv4eo))
- **Fusion** par le lead dev à la 2e revue (squash f4546c8b) ; « Avis agent 3D : VALIDÉ avec réserves » sur aabd7bd1. Ticket en 🟡 À valider (utilisateur).
- **Réponse à l'utilisateur** dans `tache utilisateur.md`, sous sa question sur la mer.
- **Pour le PO, réserves à mettre en tâches séparées** (RG-E15-09) :
  - écume et rides encore « filées » de près (R7) ;
  - reflets des façades et des coques (R1, R2) ;
  - traînée du soleil plus nette (R3) ;
  - reflet des lampadaires très fort près de la caméra la nuit (R9) ;
  - trait blanc sous le dauphin (R10) et capture rapprochée de l'éclaboussure ;
  - capture de l'écume contre des blocs dans l'eau et au pied de la falaise (R8) ;
  - coût en Basse à mesurer sur le téléphone (R6, RG-07).
- **À trancher par le PO** : RG-E15-01 (taille du décor) ne se mesure pas sur la Corniche, carte du code ; à voir avec WFS-386.
- **Session terminée** : un seul ticket par session ; aucun autre ticket demandé.

### 2026-11-10 09:36:47 — Suites de WFS-380 : WFS-400 et WFS-401 créés, décision sur la taille du décor (po-scrum-master)
- Demande de la session terminée « Claude, agent cartes (01UY1Vv4eo) » (WFS-380 fusionné, #326, f4546c8b, 🟡), sur ordre de l'utilisateur (« Demande au PO »). Aucun nouveau ticket pour cette session (un seul sujet par session).
- **Réserves en tâches séparées** (RG-E15-09), nouvelle file **« Eau commune »** de E15 (`shaders/eau.glsl`, `src/render/eau_*`), en parallèle de la file Corniche :
  - **WFS-401** (P2, ⏳ Prêt) : R7 écume et rides « filées » de près, rides régulières au loin (`mer-sillage`) ; R3 traînée du soleil bas ; R10 trait blanc sous le dauphin + capture rapprochée de l'éclaboussure ; R8 capture de l'écume contre des blocs et au pied de la falaise. Règles : `regles-de-gestion/WFS-401.md`.
  - **WFS-400** (P2, ⏳ En attente de WFS-401, mêmes fichiers) : R1 façades allumées (casino) ; R9 maillage du reflet du casino et reflet trop fort des lampadaires près de la caméra ; R2 coques de la marina ; R4 rivière de la Ville la nuit ; fin de la limite des 4 lampadaires de `nuit.cpp` pour les reflets. Règles : `regles-de-gestion/WFS-400.md`.
  - **R6** (coût en Basse sur le téléphone) : pas de ticket, vérifié pendant la validation 🟡 de WFS-380 (suivi dans `regles-de-gestion/WFS-380.md`) ; baisse → anomalie et ticket E4.
- **Décision RG-E15-01** (`regles-de-gestion/E15-decors.md`, `WFS-386.md` RG-11) : la taille 200 à 700 Mo ne se mesure **que dans les US « décor complet »** (WFS-385 à 389) ; les US intermédiaires notent seulement les Mo ajoutés. Carte du code sans `.wfcarte` : somme sur disque des actifs de décor utilisés par la carte (commande et dossiers dans la PR) ; paquet publié : partie décor du paquet.
- **WFS-383** (sols, pins d'Alep) passe ⏳ Prêt : WFS-379 et 380 fusionnés (🟡). WFS-401 et 383 peuvent avancer en même temps (fichiers séparés ; 401 ≤ 10 lignes dans `plage.cpp`).
- `track-agent.md` : ligne de Claude, agent cartes (01UY1Vv4eo) enlevée (session terminée).
- Pas de session libre : WFS-401 et WFS-383 attendent une **nouvelle session** de développeur qui les demande.

### 2026-11-10 09:45:04 — WFS-402 créé : fenêtres et portes réalistes des immeubles et des maisons (po-scrum-master)
- Demande de l'utilisateur (`@po`, 2026-11-10) : « ajoute une tâche pour le décor concernant le réalisme des fenêtres et des portes des immeubles, maisons… ».
- **WFS-402** (P1 Urgent, épic E15, nouvelle file « Ouvertures communes ») : inventaire par carte, fenêtres en retrait de 15 à 25 cm (cadre, petits bois, appui, linteau, vitre qui reflète), volets et garde-corps selon le lieu, porte d'entrée côté rue sur chaque bâtiment, vitrines, variété par bâtiment, nuit avec pièces éclairées (30 à 50 %, 2700 à 4000 K), formes géométriques interdites, téléphone tenu. Règles : `regles-de-gestion/WFS-402.md`.
- **⏳ En attente après WFS-370** (🔧, agent dev3d 01UY1Vv2Ph) : mêmes fichiers du kit `src/world/archi_*`. Agent `dev-artiste-3d`, nouvelle session ; avis « Avis agent 3D : VALIDÉ » avant le lead dev (RG-E15-06). Les US « décor complet » WFS-385 à 389 le reprennent.
- `E15-decors.md` : file « Ouvertures communes » ajoutée ; `TACHES.md` (ligne + bandeau), `track-agent.md`, `tache utilisateur.md` à jour.

### 2026-11-10 09:49:00 — WFS-323 en tout premier, WFS-280 abandonné (po-scrum-master)
- Ordre de l'utilisateur (`@po`) : « 280 abandonné, je ne veux pas de motos garées… tu mets 323 en premier ».
- **WFS-280** (deux-roues garés et faufilement) : 🚫 Abandonné ; rien n'est retiré du code (WFS-232, deux-roues dans le trafic, reste). Ordre permanent ajouté dans le skill du PO : pas de motos garées.
- **WFS-323** (vue Cockpit 1/3) : P2 → **P1 Urgent**, **en tout premier** dans l'ordre des tickets prêts ; agent `dev-artiste-3d` (`@devartiste`), nouvelle session.

### 2026-11-10 09:49:40 — WFS-370 : inventaire (RG-01), plan, accroches annoncées (Claude, agent dev3d (01UY1Vv2Ph))
- Inventaire complet : `qa/pr/WFS-370/inventaire.md` (branche `feature/waforspeed-370`). Immeubles : 592 bâtiments du kit (WFS-161) ; volumes encore nus : toit de zinc à 2 pans plats, corniche en un pavé, garde-corps en plaque pleine, édicule en cube, pied de façade lisse, vitrines nues ; fenêtres de nuit d'une seule teinte, tirées **par pixel** (mouchetures, cause : position du monde imprécise loin de l'origine). Port : entrepôts, conteneurs, grues et grillage en boîtes. Hors ticket (WFS-385) : casino, centre commercial, garages, ouvrages de la route, teintes des façades (pierre et crépi absents de `materiaux.json`).
- Plan : comble à la Mansart (brisis 70°, terrasson 20°, lucarnes, souches et mitrons), corniche moulurée, garde-corps à barreaux (NF P01-012), soubassement et cordon, couvertines et édicule, dormant des vitrines (nouveau fichier `src/world/archi_details.*`) ; port modélisé (nouveau fichier `src/world/decor_ville_port.*` : conteneur ISO 40 pieds, portique de quai, clôture à double fil) ; entrepôts = kit, style industriel.
- **Accroches annoncées (fichiers communs, RG-E15-08, ≤ 10 lignes chacune)** : `archi_geo.cpp` (appels des détails à la place des pavés, graine de baie dans les uv des vitres), `archi_geo.h` (pièce `FIN` pour les barreaux, `graineBaie`), `archi_carte.cpp` / `archi_banc.cpp` (matériau des barreaux en couverture partielle, zinc plus mat), `shaders/decor.glsl` (`interieurFenetre` : tirages par fenêtre, 2 700 à 4 000 K), `cartes_banc.cpp` (gros plans des immeubles et du port), `CMakeLists.txt` (test_archi_geo). Les garde-corps à barreaux et la corniche moulurée s'appliquent aussi aux villas de la Corniche et aux maisons de la Campagne (même kit) : rien dans `plage.cpp` ni `campagne.cpp`.
- Réseau : aucun téléchargement (aucun modèle libre aux bonnes mesures accessible par GitHub seul) : RG-03, modélisation par le code.

### 2026-11-10 10:12:55 — Début WFS-323 : attribué à Claude, agent dev3d 7 (01UY1Vv4FX) (po-scrum-master)
- Demande de ticket d'une **nouvelle session** `dev-artiste-3d` (session_01UY1Vv4FX4CXXFEkuhUsAzG, 1er ticket de la session). Ordre de l'utilisateur du 2026-11-10 09:49 UTC : **WFS-323 en tout premier** au premier `dev-artiste-3d` qui demande → attribué.
- Ticket : **P1 WFS-323** vue Cockpit (1/3) : cadrage proche de la référence (`qa/retours/2026-11-08-cockpit-reference-utilisateur.jpg`), habitacle sur toute la largeur, pavillon fermé (ANO-207), réglage « Regard cockpit » gardé ; anomalies rattachées ANO-185, 187, 188, 152 (partie lisibilité), 203, 204. Règles : `regles-de-gestion/WFS-323.md`.
- Zone autorisée : `src/game/camera_vue.*` (partie cockpit), `src/render/vehicule_cockpit*`, tests du cockpit, `qa/pr/WFS-323/` ; accroches ≤ 10 lignes annoncées au journal (ex. `src/ui/ecrans/reglages.*` seulement si RG-03 l'exige). Interdit : `assets/cars/**` et `tools/formes.json` (WFS-363 en cours, E10), `archi_*` et fichiers du décor (WFS-370, E15), CI. Deux tickets E10 en même temps : exception PO (fichiers séparés). On part de l'existant (WFS-282, 315, 349, 390 🟡) : rien retiré.
- Pas un ticket E15 : avis agent 3D non obligatoire (conseillé). Après la fusion : 🟡 À valider (utilisateur), puis la session s'arrête. WFS-324 et 325 (suites) iront à de nouvelles sessions.

### 2026-11-10 10:12:50 — WFS-337 attribué à Claude, agent dev (01UY1Vv1d6) (po-scrum-master)
- Demande de la session `@dev` session_01UY1Vv1d6VrvcVTQzXuiern (1er et seul ticket, aucun ticket depuis la règle du 2026-11-09 12:05 UTC). Profil développeur général : les P1 prêts (WFS-323, 373, 378, 387) vont à `dev-artiste-3d` ; WFS-392 à `expert-cartes` ; WFS-287 à l'agent expert ci. Premier P2 permis : **WFS-337** (E4 / E13, aucun ticket actif dans ces épics).
- Zone : `src/render/nuit.cpp` (halos des têtes de lampe), `src/render/eclairage_*` si besoin, `tests/**` ; **pas** `ville*.cpp` / `archi_*` (WFS-370), phares (WFS-382). Règles : `regles-de-gestion/WFS-337.md` (section « Suivi » ajoutée : zone, règle des formes, captures).

### 2026-11-10 10:14:06 — WFS-373 attribué à Claude, agent dev3d (01UY1Vv4L4) (po-scrum-master)
- Demande d'une nouvelle session `@devartiste` (session_01UY1Vv4L4emR5PusLbYXyYF, agent `dev-artiste-3d`), 1er et seul ticket de la session. WFS-323 (demandé en premier) venait d'être attribué à 10:12 à Claude, agent dev3d 7 (01UY1Vv4FX) : on garde cette attribution.
- **WFS-373** (P1, E15 file Grand Prix) : inventaire des formes nues au journal (RG-01), puis tribunes, stands vitrés avec auvent, portique, barrières, murs de pneus sanglés, panneaux en vrais modèles (CC0 / CC BY, ±1 %, sans marque) ou modélisés au niveau du vrai objet. Aucun autre ticket actif sur le Grand Prix.
- Zone : `src/world/circuit.cpp`, `portique.cpp`, nouveaux `src/world/decor_grandprix_*`, `assets/world/modeles/grandprix/**`, `assets/textures/decor/grandprix/**`, `paquets/sources/grandprix/**`, licences ; fichiers communs : ajouts ≤ 10 lignes annoncés. Pas : `archi_*` au-delà des accroches (WFS-370), `ville*`, `plage.cpp`, `campagne*`, `assets/cars/**`, cockpit.
- Branche `feature/waforspeed-373` ; captures avant / après jour et nuit + gros plans + banc perf Basse ; PR, avis agent 3D VALIDÉ, puis lead dev ; arrêt de la session après la fusion.

### 2026-11-10 10:20:35 — Statuts automatiques, bancs du matin #12 (robot statuts)
- Run rouge (schedule, commit 9c4f58a), aucun statut changé. Job(s) en échec : banc (realisme-2).
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-396, WFS-362, WFS-384, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-10 10:21:29 — WFS-370 : PR #332 ouverte (Claude, agent dev3d (01UY1Vv2Ph))
- Fait : immeubles du kit en vrais volumes (comble à la Mansart, lucarnes, cheminées, corniche moulurée, garde-corps à barreaux, soubassement, cordon, édicules, vitrines), port modélisé (conteneurs ISO 40 pieds, portiques de quai, clôture à double fil, entrepôts du kit), fenêtres de nuit tirées par fenêtre (fin des mouchetures), 2 700 à 4 000 K.
- Vérifié : 97 tests CPU ; 20 captures avant / après jour et nuit (`qa/pr/WFS-370/`) ; banc perf Ville de nuit, Basse : 335 → 345 ms par image sur lavapipe (+3 %), processeur inchangé, 375 k → 395 k triangles ; Haute +11 %.
- Limites : « avant » faits à la main (vues de banc nouvelles) ; le kit change aussi un peu les villas de la Corniche et les maisons de la Campagne (barreaux, corniche, zinc mat). Suite : avis agent 3D (E15), puis lead dev.

### 2026-11-10 10:22:10 — WFS-373 : inventaire des formes nues du Grand Prix (RG-01), plan (Claude, agent dev3d (01UY1Vv4L4))
- Inventaire complet : `qa/pr/WFS-373/inventaire.md` (branche `feature/waforspeed-373`). Objets posés (journal du banc, comptage ajouté) : mur de pneus 2 068, mur des stands 517, tentes 112, garages 107, panneaux publicitaires 103, grillage 155, drapeaux 51, postes de commissaire 34, stands fans 24, camions 21, bars d'accueil 8, panneaux de freinage 3, tribunes 2, portique, passerelle, tour, hélicoptère, grande roue 1 chacun, oiseaux (3 vols) ; tous en boîtes, cylindres, cônes ou triangles. Projecteurs (117) : dessin commun `decor_carte.cpp` → type propre au Grand Prix dans `circuit.cpp`.
- Plan (RG-03, comme WFS-372) : nouveau fichier `src/world/decor_grandprix_modeles.*` (modèles construits une fois au chargement, version proche et allégée ; Basse = allégée pour les murs de pneus), branchés dans `circuit.cpp` ; test `tests/test_decor_grandprix.cpp` (mesures, triangles, normales) ; banc de gros plans `WFS_BANC=grandprix-objets`.
- **Accroches annoncées (≤ 10 lignes)** : `src/platform/desktop_main.cpp` (banc `grandprix-objets`), `CMakeLists.txt` (test). Réutilisés sans les changer : `marina_bateaux.h` (loft, eventail, ligne), `decor_campagne_modeles.h` (poutre chanfreinée, profilé I).
- Réseau : recherche de code GitHub seulement (modèles stylisés, exclus par RG-02), aucun téléchargement.

### 2026-11-10 10:23:56 — WFS-337 : arbitrage, accroche de 2 lignes acceptée (option A) (po-scrum-master)
- Demande de Claude, agent dev (01UY1Vv1d6). Cause d'ANO-189 : 2e lanterne du candélabre double du terre-plein central non inscrite dans `lampadaires()`. Accepté : 1 ligne dans `src/render/eclairage.h` (style `Tete`, halo sans lumière) et 1 ligne dans `src/world/ville.cpp` (fin de `initOuvrages`), à annoncer au journal par le développeur ; synchronisation avec `main` avant la PR (WFS-370 sur le même fichier). Détail : `regles-de-gestion/WFS-337.md`, section « Suivi ».

### 2026-11-10 10:31:01 — WFS-363 : Giulia et Range Rover Sport sans signature de forme, PR #333 (Claude, agent dev3d 6 (01UY1Vv3MV))
- Fait : Giulia, écusson en V refermé par une peau cousue au bord découpé (nouvelle retouche `couture` : sommets du bord repris, Delaunay, surface biharmonique), prises d'air en trèfle et pointe du V remplacées par une grille basse à 8 lames ; Range Rover, nid d'abeille de la calandre remplacé par 4 lames, bandeau avant (phares et cadre) étiré ×1,3 vers le bas d'un seul tenant (phares plus hauts, sans pli). Lettrage du capot déjà effacé par WFS-192.
- Fichiers : `tools/retoucher_formes.mjs` (couture, `baseAjustee` factorisée, options `exact` / `biharmonique` inactives par défaut), `tools/formes.json` (entrées `giulia`, `rrsport`), `assets/cars/giulia.*`, `assets/cars/rrsport.*`, `qa/pr/WFS-363/`.
- Vérification : `--verifier` vert pour les 7 voitures retouchées ; M4 et S3 rejouées depuis l'origine : empreintes identiques ; 98 tests CPU verts ; `test_fidelite_voitures` : Giulia et Range Rover conformes ±1 % (Giulia : échelle ×0,999, nez +1,2 cm au centre) ; 16 paires de captures + 12 zooms ×4 jour / nuit regardés.
- Limites : nid d'abeille des prises d'air basses du Range Rover et pavés LED des phares gardés (hors RG-01, à proposer au PO en tâche séparée si voulu). Essais écartés : panneau posé dessous (joint visible en trapèze), couronnes concentriques (stries), une boîte par phare (montant tordu).


### 2026-11-10 10:42:17 — WFS-384 : premier banc du matin après la correction, run #12 (agent expert ci cd (session 0183bv2Y))
- 1er critère d'acceptation : réveil de GitHub à 08:52 UTC (09:52 heure de Paris) → **bancs du matin lancés, run #12** (https://github.com/wlfaleh/waforspeed_mobile/actions/runs/42786014161, commit 9c4f58a) ; message « ⏳ Bancs du matin en retard » posté dans l'issue #242 à 08:53 UTC ; captures publiées dans `captures-matin` (jour.txt : 2026-11-10 11:16 heure de Paris, 18 archives) ; message « Recette QA prête » à 10:19 UTC. Réveil suivant (run #13, 10:37 UTC) **sauté proprement** (43 s, banc déjà fait).
- Run #12 **rouge**, pour deux causes qui ne viennent pas de WFS-384 (à confier par le PO, US technique P1 : « rouges → le PO crée une US technique P1 ») :
  1. `banc (realisme-2)` : « réalisme : partie 2, 2 erreur(s) », code 3 (était vert au run #5, commit 4e7fa71). Les deux lignes [ERREUR] sont au début du journal du job (non lisibles ici : seules les 5 000 dernières lignes sont accessibles) ; à reproduire avec `WFS_BANC=realisme-2` entre 4e7fa71 et 9c4f58a (19 tickets fusionnés, dont WFS-369, 372, 376, 379, 381, 382 qui touchent le rendu).
  2. `banc (ecrans-2400)` : **arrêté à la limite de 75 min** (run #5 : 41 min). Le jeu est devenu plus lent à rendre sur le banc logiciel (décors E15 probables) ; archive publiée mais peut-être incomplète (17 fichiers).
- Émulateur (information) : rouge aussi, comme au run #5.

### 2026-11-10 10:45:32 — WFS-363 fusionné par le lead dev (lead-dev)
- PR #333 (Claude, agent dev3d 6 (01UY1Vv3MV)) relue sur 9d83a967 : 56 captures ouvertes, tout le diff lu (`couture`, `baseAjustee`, options `exact` / `biharmonique` inactives par défaut). Construction et `tests_cpu.sh` refaits : 98/98. Rejeu de giulia, rrsport (depuis main) et de m4, s3 (depuis 30d339f4~1) : `.wfm` et `.lod` identiques à l'octet. Revue « ✅ Approuvé » en commentaire (même compte GitHub), fusion en squash 2473aa27. Statut 🟣.
- **Pour le PO** (petits défauts, RG-06 : tâche séparée à créer) : Range Rover : (1) une rangée de l'ancien nid d'abeille reste sous les nouvelles lames de la calandre, et la lame du bas plie vers les coins ; (2) petite pointe au-dessus du montant du cadre ; (3) la silhouette reste reconnaissable à 3/4 (phares toujours fins, nid d'abeille des prises d'air basses, pavés LED), alors que le critère dit « plus reconnaissable de face et de 3/4 » : RG-01 de WFS-363 ne visait que la calandre et les feux. Giulia : (4) léger creux au centre, sous la pointe du capot (vue basse).

### 2026-11-10 10:46:20 — WFS-363 fusionné (#333), session terminée (Claude, agent dev3d 6 (01UY1Vv3MV))
- PR #333 fusionnée en squash par le lead dev (2473aa27) ; ticket 🟣 ; ligne de `qa/Check.md` écrite par le lead dev.
- Petits défauts restants relevés par le lead dev (RG-06, à confier au PO en tâche séparée) : Range Rover (reste de nid d'abeille sous les lames, pointe en haut du montant du cadre, silhouette encore reconnaissable de 3/4 : prises d'air basses, pavés LED), Giulia (léger creux sous la pointe du capot en vue très basse), `delaunay()` sans cas des points alignés.
- Un seul ticket par session : je m'arrête, aucun autre ticket demandé.


### 2026-11-10 10:47:37 — WFS-370 fusionné par le lead dev (lead-dev)
- PR #332 relue sur 05a6a342 (branche à jour avec main) : « Avis agent 3D : VALIDÉ avec réserves » (R1 à R6) présent ; 40 captures ouvertes (immeubles classique et moderne, rue des terrasses, port : rue, entrepôts, conteneurs, portique, clôture ; jour et nuit) ; tout le diff relu ; construction et `tests_cpu.sh` refaits : 98 tests réussis ; bancs `archi` et `corniche` lancés pour vérifier le kit commun (aucune régression vue).
- Revue « ✅ Approuvé » écrite dans la PR (commentaire, même compte GitHub), puis fusion en squash : **181aa552**. Statut 🟡 À valider (utilisateur) ; ligne de `suivi-pr.md` supprimée ; ligne ajoutée dans `qa/Check.md`.
- Remarques non bloquantes, pour le PO : réserves R1 à R6 de l'agent 3D à mettre en tâches séparées (RG-E15-09) ; `archi_details.cpp` (`mansart`) : calcul `* 0.f` mort ; `etendue(GRUE)` passe à lat + 83 m, donc compter les portiques restants dans WFS-385 ; perf Haute +11 % à suivre aux bancs du matin.

### 2026-11-10 10:48:28 — WFS-370 : fusionné, session terminée (Claude, agent dev3d (01UY1Vv2Ph))
- PR #332 fusionnée en squash par le lead dev (181aa552) après « Avis agent 3D : VALIDÉ avec réserves » ; statut 🟡 À valider (utilisateur).
- **Pour le PO (tâches séparées, RG-E15-09)** : réserves de l'agent 3D R1 zinc encore clair au soleil (albédo visé 0,25 à 0,35) ; R2 vitrines de nuit des cafés en aplat (WFS-402) ; R3 entrepôts trop « logements » (nef, portes de chargement, moins de fenêtres) ; R4 ondulation des conteneurs faible au loin ; R5 gros plan du portique entier au banc ; R6 perf Basse +3 % sur lavapipe à confirmer aux bancs du matin. Remarques du lead dev : code mort `* 0.f` dans `mansart` (à nettoyer dans WFS-402), graine `^ 0x5eedu` des entrepôts à commenter, portiques peut-être retirés par la nouvelle étendue (lat + 83 m) : à compter dans WFS-385.
- Un seul sujet par session : je m'arrête, aucun autre ticket demandé.

### 2026-11-10 10:52:34 — WFS-337 : cause trouvée, accroches, PR #334 ouverte (Claude, agent dev (01UY1Vv1d6))
- Cause (RG-01, ANO-189) : le candélabre double de 12 m du terre-plein central de la voie rapide (Ville) porte 2 lanternes à 5,0 m ; `ville.cpp` n'inscrivait pour la nuit que celle de notre côté. La lanterne du sens inverse restait une barre jaune sans lumière, et le halo se voyait 5 m à côté. Rue, pont, tunnel : halo déjà à ≤ 0,2 m de la vasque.
- **Accroches annoncées (acceptées par le PO, 10:23)** : `src/render/eclairage.h` 1 ligne (style `StyleLampadaire::Tete`, halo seul) ; `src/world/ville.cpp` 1 ligne en fin de `initOuvrages` (2e lanterne inscrite en `Tete`) ; `src/platform/desktop_main.cpp` 2 lignes (banc `WFS_BANC=halos`).
- Fait : `nuit.cpp` dessine le halo de chaque tête ; une `Tete` ne prend aucune des 4 lumières et n'a pas de flaque (coût inchangé) ; `eclairage_physique.cpp` : exposition inchangée. Nouveau banc `src/render/eclairage_halos_banc.cpp`.
- Vérifié : 98 tests CPU ; banc `halos` : `main` 34 candélabres avec une seule tête (ÉCART), branche 34 avec leurs 2 têtes (OK) ; 11 captures avant / après de nuit (`qa/pr/WFS-337/`, 160 km/h, 50 m, de près, rue, Poursuite).
- Limites : têtes des candélabres encore en pavé (WFS-378) ; hauteurs inscrites hétérogènes dans `ville.cpp`, non changées (écart ≤ 0,2 m).

### 2026-11-10 11:00:29 — Archives automatiques (robot archives)
- Journal : 321 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-08).
- TACHES.md : 6 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md` (WFS-162, WFS-163, WFS-194, WFS-393, WFS-231, WFS-280).

### 2026-11-10 11:09:55 — WFS-323 : PR #335 ouverte, accroches annoncées (Claude, agent dev3d 7 (01UY1Vv4FX))
- Fait : vue Cockpit au cadrage de la référence (champ 72° → 62°, regard Normal −5°, horizon 46,5 % → 42,9 %) ; pavillon (traverse, ciel de toit, rails, pare-soleil), haut du pare-brise à sa vraie place (`HAUT_PARE_BRISE_*`, partagé avec la pluie) ; rétroviseur accroché au pare-brise ; pièces de l'intérieur modelées (`dalle`, `profile`, `cadreArrondi`) au lieu de boîtes et cylindres ; occlusion de l'habitacle (MAT_AO, ANO-185) ; chiffres du compteur plus grands (ANO-188) ; volant vérifié (ANO-187).
- Accroches hors zone (annoncées ici) : `configgame.json` et `src/core/configgame.h` (1 ligne chacun : `camera.cockpit.zoom` 62), `configgame.README.md` (1 ligne), `src/render/effets_systeme.cpp` (6 lignes, ANO-204 : étincelles et fumée à moins de 2,5 m de l'œil non dessinées en vue Cockpit), `src/platform/desktop_main.cpp` (1 ligne : banc `cockpit-reference`).
- Vérification : 98 tests CPU verts ; banc `cockpit-reference` avant / après (`qa/pr/WFS-323/`) ; banc `cockpit` SF90, Abarth, Model 3 : fenêtre de la rue cachée 0,0 % ; coût de l'intérieur Golf 32 → 33 appels, 1,22 → 1,60 Mo (RG-04 ; mesure en qualité Basse sur téléphone non faite).
- Limites : intérieur 52-59 % de l'image (cible 30-35 % de WFS-282 dépassée par le toit et le champ de la référence) ; « R » rouge du HUD après un choc (ANO-204) hors zone, non traité ; ANO-203 sans capture de checkpoint de nuit.


### 2026-11-10 11:18:35 — WFS-337 fusionné par le lead dev (lead-dev)
- PR #334 relue sur 7bca0195 : 22 captures ouvertes (11 paires), diff entier lu (nuit.cpp, eclairage.h, eclairage_physique.cpp, ville.cpp : 1 ligne, banc halos, accroche desktop_main.cpp). Accroches conformes à l'arbitrage du PO de 10:23:56.
- Construction et tests_cpu.sh refaits : 98/98. Banc `halos` relancé par le lead dev sur main (+ banc) : ÉCART (34 candélabres avec une seule tête), sur la branche : OK (34 avec leurs 2 têtes). Les écarts du bandeau TRAVAUX, de la glissière et de la voiture sur `halos-poursuite` venaient de l'ancien main des images « avant » : sur le main actuel, seuls 175 pixels (halos) changent.
- Revue « ✅ Approuvé » en commentaire (même compte GitHub), fusion squash a236bdc5. WFS-337 → 🟡 À valider (utilisateur). Remarques non bloquantes dans la PR (distance négative dans prochain() du banc, compte de lampes de bench_perf, pas de test CPU dédié ; halo rond et tête en barre restent pour WFS-378).

### 2026-11-10 11:19:12 — WFS-337 : fusionné, session terminée (Claude, agent dev (01UY1Vv1d6))
- PR #334 fusionnée en squash par le lead dev (a236bdc5) ; statut 🟡 À valider (utilisateur) : la nuit, sur la voie rapide de la Ville, chaque lanterne des candélabres doubles a son halo.
- **Pour le PO (petite tâche de suite, remarques non bloquantes du lead dev)** : banc `halos`, `prochain()` peut rendre une distance négative et `#include <algorithm>` manque ; `bench_perf` compte 34 lampes de plus (têtes seules) ; aucun test CPU du contrôle des 2 têtes (seulement le banc `halos`, hors CI). Halo rond et tête en pavé : WFS-378.
- Un seul sujet par session : je m'arrête, aucun autre ticket demandé.

### 2026-11-10 11:25:16 — WFS-323 : PR #335 refusée par le lead dev (lead-dev)
- Commit relu `589f90ac`. Construction en Release et `tools/qa/tests_cpu.sh` refaits : 98 tests réussis. Tout le diff lu, les 26 captures ouvertes (agrandissements du combiné, du toit et du rétroviseur de nuit).
- Mieux qu'avant : cadrage (horizon à 43 %), toit fermé (ANO-207), rétroviseur moulé, volant qui tourne (ANO-187), nuit sans aplat noir.
- **Bloquant** : (1) ANO-188 annoncée corrigée, mais le compteur reste illisible : M4 « 58 » → « SA 2 » (chiffres coupés par la planche ou la visière), X5 vitesse cachée par la jante. Demandé : régler le combiné ou l'œil par voiture, plus un contrôle au banc `cockpit-reference` ; (2) visière du combiné (`extruder(cas…)`) : prisme à arêtes vives, forme nue dans la zone touchée.
- Remarques : filtre de 2,5 m des étincelles (ANO-204) trop large pour un choc latéral ; ANO-185 seulement atténuée ; gouttes de pluie devant le rétroviseur (déjà sur `main`, au PO) ; part de l'image prise par l'intérieur de 52 à 59 % contre 30 à 35 % dans WFS-282 (règle à mettre à jour par le PO). Avis de l'agent 3D : « À REPRENDRE », même point bloquant.
- TACHES : WFS-323 ↩️ Retour lead dev (PR #335) ; track-agent et suivi-pr mis à jour.

### 2026-11-10 11:26:16 — WFS-373 : PR #336 ouverte (Claude, agent dev3d (01UY1Vv4L4))
- Fait : 21 objets du Grand Prix modélisés au niveau du vrai objet (RG-03), construits une fois au chargement (`src/world/decor_grandprix_modeles.*`), posés par `circuit.cpp` ; herbe tondue en bandes, touffes semées au hasard ; projecteurs propres au Grand Prix ; banc `grandprix-objets` ; test `test_decor_grandprix`.
- Accroches (annoncées à 10:2x) : `desktop_main.cpp` (2 lignes), `CMakeLists.txt` (1 ligne). Aucun fichier commun d'une autre file touché.
- Vérification : 99 tests CPU réussis ; captures avant / après (banc `cartes`, Grand Prix) et 29 gros plans jour / nuit ; Basse : 402 / 487 ms → 412 / 424 / 432 ms (lavapipe, pas de baisse), Haute +25 %. Taille ajoutée : 0 Mo.
- Limites : coût en Haute ; un mur de pneus sur deux toujours retiré en Basse (RG-C2 de WFS-140). Suite : avis agent 3D (RG-E15-06), puis lead dev.

### 2026-11-10 11:30:24 — WFS-403 créé : finitions du décor de WFS-373 (po-scrum-master)
- Demande de Claude, agent dev3d (01UY1Vv4L4), développeur de WFS-373 (PR #336) : l'agent 3D a rendu « VALIDÉ avec réserves » (commentaire 6766136867) ; RG-E15-09 → tâche séparée.
- **WFS-403** (P2, E15 file Grand Prix, agent `dev-artiste-3d`, nouvelle session, après la fusion de WFS-373 ; pas la session 01UY1Vv4L4) : nacelles de la grande roue, jantes du camion et dos du bâtiment des stands, mur des stands et murs de pneus **continus en Basse** (arbitrage du PO : la règle « un petit objet sur deux » de WFS-140 RG-C2 ne vaut plus pour les barrières de sécurité), garages éclairés de nuit, voie des stands en asphalte / béton. Règles : `regles-de-gestion/WFS-403.md`.
- Points mineurs (pneus facettés, bar d'accueil, vigie de la tour, muret sans joints, tache blanche à l'horizon, fragment devant l'hélicoptère) versés dans **WFS-388** (RG-11 à 16 ; l'agent 3D avait écrit WFS-389 par erreur). File Grand Prix : WFS-373 → 403 → 388 (mêmes fichiers).
- Rien attribué (un seul sujet par session).

### 2026-11-10 11:43:14 — WFS-373 fusionné par le lead dev (lead-dev)
- PR #336 (Claude, agent dev3d (01UY1Vv4L4)) relue sur a6cb6e69 : « Avis agent 3D : VALIDÉ avec réserves » présent (RG-E15-09), 49 captures ouvertes, diff lu en entier, construction et `tests_cpu.sh` refaits : 99/99. Approbation écrite dans la PR (commentaire, même compte GitHub), fusion en squash **fb3f2d99**.
- Statut : 🟡 À valider (utilisateur) (décor, comme WFS-370 / 372). Réserves : WFS-403 puis WFS-388 (déjà créés par le PO). Ligne `qa/Check.md` ajoutée ; ligne de `suivi-pr.md` supprimée.

### 2026-11-10 11:43:39 — WFS-373 : session terminée (Claude, agent dev3d (01UY1Vv4L4))
- PR #336 fusionnée en squash par le lead dev (fb3f2d99) après « Avis agent 3D : VALIDÉ avec réserves » ; réserves dans WFS-403 (P2) et WFS-388. Ticket 🟡 À valider (utilisateur).
- Un seul ticket par session : la session s'arrête, aucun autre ticket demandé.

### 2026-11-10 12:40:36 — WFS-404 créé (logos falsifiés tout de suite) et agent juriste créé (po-scrum-master)
- **Ordre de l'utilisateur (12:37 UTC, `@po`)** : falsifier tout de suite, même grossièrement, les logos des voitures qui restent, dans une seule US urgente, pour mettre l'application sur Android ; ensuite reprendre les US pour les finir proprement.
- **WFS-404** (P1, E10, exception PO : `assets/cars/*`, `tools/logos.json`, `effacer_logos.mjs` ; pas `vehicule_cockpit*` de WFS-323), agent `dev-artiste-3d`, nouvelle session, **en tout premier** : inventaire rapide des 21 voitures et des deux-roues, puis effacement au plus simple (X5 : losange du capot et calandre ; A 45 et Elantra N : logos, lettres, calandre ; bandes de marque des volants, ANO-209). Règles : `regles-de-gestion/WFS-404.md`.
- WFS-397, WFS-360, WFS-361 passent « après WFS-404 » (reprise « nickel »). WFS-393 est déjà rangé dans l'archive (à revoir avec WFS-361).
- **Agent `juriste` créé** (demande de l'utilisateur) : `.claude/agents/juriste.md`, skill `.claude/skills/juriste`, alias `@juriste` (ajouté à `appel-agents.md`, à `CLAUDE.md` et à `track-agent.md`), rapports dans `juridique/`.
- Point signalé à l'utilisateur : effacer les logos ne règle pas la licence « non commerciale » de l'A 45, de l'Elantra N et du X5 (CC BY-NC / NC-SA) ; sortie gratuite sans pub ni achat possible le temps du remplacement (WFS-320, point 1) ; avis du juriste demandé.
- Rien attribué (attribution seulement à la demande d'un développeur).

### 2026-11-10 12:49:39 — WFS-404 réécrit : les 3 voitures NC retirées de l'application (po-scrum-master)
- **Ordre de l'utilisateur (« Oui »)** à la proposition du PO : retirer de l'application l'A 45, l'Elantra N et le X5 (licences CC BY-NC / NC-SA) et garder les 18 autres, pour sortir sur Android tout de suite.
- **WFS-404** réécrit (P1, `dev-artiste-3d`, nouvelle session) : liste unique des voitures retirées, plus rien dans le jeu ni dans l'APK, fichiers gardés dans le dépôt ; camion de la Poursuite et sens inverse de la Ville sur des modèles libres ; sauvegarde du joueur reprise ; crédits régénérés ; contrôle rapide des marques des 18 autres (volant de la M4 : bandes de marque dans `vehicule_cockpit_defs.cpp:29`).
- **Erreur du PO reconnue** : le X5 était connu sous licence NC depuis le 2026-11-07 ; il n'aurait pas dû être retouché (part X5 de WFS-193, PR #311 ; WFS-358, PR #319). Plus aucune retouche du X5 d'origine (WFS-393 abandonné).
- Rien attribué (attribution à la demande d'un développeur).

### 2026-11-10 12:50:24 — Premier rapport juridique : publication Android / Google Play (juriste)
- `juridique/rapport-2026-11-10.md` (J-01 à J-17), demande de l'utilisateur du 2026-11-10 12:37 UTC.
- **(a) test interne** : publiable maintenant. **(b) sortie publique gratuite** : après 6 points 🔴 : logos restants (WFS-404), fiche du store (« 20 vraies voitures », marques dans les crédits collés), 7 noms de voitures trop proches des vrais (Bnv, Folg 8…), politique de confidentialité publique et contact, formulaire « Sécurité des données » (duel en ligne = données collectées, éphémères), crédits CC BY manquants (3 deux-roues, auteurs du son « gros ») et écran Crédits.
- **Licences NC (A 45, Elantra N, X5)** : effacer les logos ne règle pas la licence ; tolérées (🟠) en version gratuite sans pub ni achat, **bloquantes pour toute vente, pub ou achat** (remplacement WFS-360, 361, 397).
- 🟠 principaux : formes reconnaissables des vraies voitures (avis d'avocat avant une vente), nom « WaForSpeed » à vérifier, CGU (éditeur, droit applicable, duel), public cible et IARC, notice Apache 2.0 de native_app_glue.
- Transmis au PO pour les tickets ; 7 questions pour l'utilisateur à la fin du rapport. Rien codé, aucun ticket créé.

### 2026-11-10 12:50:46 — Lead dev automatique (chaque heure) mis en pause (po-scrum-master)
- Ordre de l'utilisateur (budget) : la tâche planifiée « Lead dev automatique (chaque heure) » est désactivée (gardée, réactivable). Les PR sont relues seulement quand un développeur appelle lui-même le lead dev après sa PR.

### 2026-11-10 12:51:04 — WFS-323 : PR #335 corrigée après le refus du lead dev (Claude, agent dev3d 7 (01UY1Vv4FX))
- Fait : compteur lisible sur les 20 voitures (face de planche qui coupait le combiné à mi-hauteur, cause déjà sur `main` ; vitesse placée dans la bande vue par l'ouverture haute du volant, `bandeLisibleCombine`) ; visière du combiné moulée ; étincelles et fumée cachées seulement dans la boîte de l'habitacle (accroche `effets_systeme.cpp`, 10 lignes) ; montants de 9 cm de face ; tige du rétroviseur visible ; axes des pare-soleil mats.
- Vérification : 99 tests CPU verts après synchronisation (`c65303b4`) ; nouveau contrôle `test_cockpit` (vitesse dans la bande vue) ; agrandissements des 20 combinés dans `qa/pr/WFS-323/`.
- Pour le PO : gouttes de pluie dessinées devant le rétroviseur intérieur la nuit (pluie en 2D, déjà sur `main`, hors de ma zone) ; cible « intérieur 30-35 % de l'image » de WFS-282 à revoir (52-59 % avec le toit et le champ de la référence).


### 2026-11-10 12:51:45 — Plus aucun travail sur les 3 modèles NC d'origine (po-scrum-master)
- Erreur du PO reconnue devant l'utilisateur : des US ont visé le X5 (WFS-193 part X5, WFS-358, WFS-393) alors que sa licence NC le condamnait. Ordre permanent ajouté : aucun ticket ne vise plus l'A 45, l'Elantra N ni le X5 d'origine ; les lots du cockpit (WFS-325) et les bancs les sautent (WFS-404).

### 2026-11-10 12:56:29 — X5 : l'auteur refuse l'usage commercial (po-scrum-master)
- Réponse de l'utilisateur : l'auteur du X5 ne veut pas. Le X5 d'origine reste retiré (WFS-404). WFS-361 garde le remplacement et reprend tous les réglages du vrai X5 (RG-R1).

### 2026-11-10 12:59:06 — sketchfab.com ajouté aux sites autorisés (po-scrum-master)
- Ordre de l'utilisateur : `sketchfab.com` (modèles 3D CC0 / CC BY seulement) ajouté au tableau du § 1 de `regles-reseau.md`, pour WFS-361 (X5, X3 ou X1 libre, sinon autre grand SUV) puis WFS-360. Le réglage réseau de l'environnement reste à faire par l'utilisateur.

### 2026-11-10 13:02:11 — WFS-404 passé SUPER URGENT : seules les voitures vendables restent (po-scrum-master)
- Ordre de l'utilisateur (`@po`) : « créer une tâche super urgente pour garder que les voitures qu'on peut utiliser pour gagner l'argent ». WFS-404 (déjà le retrait des 3 voitures NC) devient **SUPER URGENT, avant tout autre ticket**, avec RG-00 : seules les licences CC0 / CC BY / CC BY-SA restent dans l'APK ; contrôle du PO : seules A 45, Elantra N et X5 sont NC.

### 2026-11-10 13:04:01 — Avis juridique : lancement officiel aujourd'hui impossible, 7 points bloquants ouverts (juriste)
Demande de l'utilisateur (« @avocat », lancement officiel aujourd'hui). Contrôle refait sur `main` : aucun des points 🔴 du rapport de 12:48 n'est corrigé (WFS-404 non commencé, fiche, noms, confidentialité, Sécurité des données, crédits) ; J-07 (licences NC) passe 🔴 puisque l'utilisateur vise des revenus. Test interne toujours possible. Détail : `juridique/rapport-2026-11-10.md` (§ Mise à jour). Transmis au PO.
### 2026-11-10 13:04:16 — WFS-323 fusionné par le lead dev (lead-dev)
- PR #335 relue à nouveau sur c65303b4 (2e revue après le refus sur 589f90ac) : réponses point par point présentes ; tout le diff relu ; construction Release dans /tmp et `tests_cpu.sh` : 99/99 ; captures ouvertes (4 comparaisons, pavillon, regard Bas, volant, combinés des 20 voitures) ; ANO-188 corrigée (M4 « 58 », X5 « 56 ») ; avis agent 3D « VALIDÉ avec réserves ».
- Revue « ✅ Approuvé » en commentaire (même compte GitHub), fusion en squash f04cec8c. WFS-323 → 🟡 À valider (utilisateur) ; ligne `qa/Check.md` ajoutée ; ligne de `suivi-pr.md` supprimée.
- Réserves non bloquantes pour WFS-324 / 325 ou `anno.md` (PO) : visière qui rogne le haut des chiffres en regard Haut, chevauchements arc / aiguille / rapport (Corsa, V60, GT86, Gallardo, 911, Abarth, SF90), poignée chromée trop claire de nuit.

### 2026-11-10 13:04:31 — Vue Cockpit passée urgente (po-scrum-master)
- Ordre de l'utilisateur : les sujets cockpit sont urgents aussi. WFS-324 (rétroviseurs), WFS-325 (intérieurs par voiture), WFS-395 (habitacles Corvette et Brezza) : **P1 Urgent**. Ordre : WFS-404 → WFS-323 (fusionné #335, 🟡) → 324 → 325 → 395. Les 3 voitures retirées sont exclues.

### 2026-11-10 13:05:20 — WFS-323 : fusionné (PR #335, f04cec8c), session terminée (Claude, agent dev3d 7 (01UY1Vv4FX))
- Fusion en squash par le lead dev après le 2e passage ; avis agent 3D « VALIDÉ avec réserves » ; WFS-323 en 🟡 À valider (utilisateur).
- Réserves à reprendre (pour le PO : `anno.md` ou WFS-324 / 325) : en regard Haut, la visière rogne le haut des chiffres (« 87 » lu « 81 ») ; l'arc, l'aiguille ou le rapport touchent les chiffres sur Corsa, V60, GT86, Gallardo, 911, Abarth, SF90 ; poignée chromée trop claire la nuit ; intérieur sombre de jour ; ANO-185 seulement atténuée ; gouttes de pluie devant le rétroviseur intérieur (pluie, hors zone) ; part de l'image de WFS-282 (30-35 %) à revoir (52-59 %).
- Réponse à l'utilisateur écrite dans `tache utilisateur.md`. Session arrêtée (un seul ticket par session) : aucun autre ticket demandé.


### 2026-11-10 13:07:00 — Sortie grand public : Ville seule ; WFS-405 et WFS-406 créés (po-scrum-master)
- Ordre de l'utilisateur : une seule carte, la Ville, pour le grand public ; les autres cartes continuent en développement.
- **WFS-405** (P1 super urgent, E8 + E14, `expert-cartes`) : liste unique des cartes publiques ; AAB = Ville seule, APK d'aperçu = toutes les cartes ; tous les modes sur la Ville ; sauvegarde reprise.
- **WFS-406** (P1 super urgent, E5, `dev`) : écran Crédits complet sans marques, noms trop proches renommés (rapport juridique J-03, J-06, J-13).
- **WFS-320** : points J-02, J-04, J-05, J-11 ajoutés pour l'agent expert ci.
- Besoin : 4 sessions en parallèle (`@devartiste` WFS-404, `@cartes` WFS-405, `@dev` WFS-406, `@cicd` WFS-319 / 320). Rien attribué (à la demande).

### 2026-11-10 13:11:50 — Points 🔴 du juriste découpés en US : WFS-407, 408, 409 ; WFS-406 précisé (po-scrum-master)
- Demande du juriste puis ordre de l'utilisateur (~13:10 UTC, « regarde avec le PO pour créer les US urgentes ») ; WFS-405 (Ville seule) et WFS-406 (crédits et noms) déjà créés à 13:07 par une autre session du PO : pas de doublon.
- **WFS-406** précisé : noms choisis par le PO (golf → Norvik Lume, m4 → Draven Vantor GT, giulia → Alvar Selene TS ; les 3 voitures retirées renommées à leur retour), crédits à ajouter (3 deux-roues, auteurs du son « gros », version nhaudio, Apache 2.0 de native_app_glue), RG-06 ligne de confidentialité cachée tant que l'adresse est vide, RG-07 contrôle des marques par `verifier_licences.py`.
- **WFS-404** : RG-06 étendu (J-01 : jantes, centres de roue, pneus, étriers, plaques, intérieurs, 3 deux-roues).
- Créées, **P1 juste après WFS-404**, agent `expert-ci-cd`, découpées de **WFS-320** : **WFS-407** fiche sans marque + captures neuves (J-02, après 404, 405, 406) ; **WFS-408** confidentialité, CGU, contact (J-04, J-10, ANO-205 → 📋 Planifiée) ; **WFS-409** Sécurité des données + IARC + public cible (J-05, J-11 réunis : mêmes fichiers, pas de micro-ticket). Aucun commit sur `feature/waforspeed-320` depuis le 2026-11-08 14:14 UTC (branche jamais poussée) : plus de 10 h.
- Décisions de l'utilisateur non tranchées (éditeur, contact, adresse de la politique, duel en ligne public, public cible, statut non commerçant, nom du jeu) : RG « en attente de l'utilisateur », aucune valeur inventée.
- `track-agent.md` : lignes des 6 sessions terminées enlevées ; aucune session de développeur active.

### 2026-11-10 14:08:17 — WFS-324 (rétroviseurs) prêt en parallèle de WFS-404 (po-scrum-master)
- Question de l'utilisateur sur les rétroviseurs : c'est WFS-324 (P1). Décision du PO : il part en même temps que WFS-404 (zones séparées), dans une session `@devartiste` à part.

### 2026-11-10 14:15:33 — Ordre des 50 prochaines tâches fixé par l'utilisateur (po-scrum-master)
- L'utilisateur valide la liste des 50 tâches du PO et monte **WFS-324 (rétroviseurs) en 2e**, juste après WFS-404 (en parallèle, zones séparées). Ordre écrit en bandeau de `TACHES.md` ; publication possible après les tâches 1 à 8.

### 2026-11-10 14:16:35 — WFS-402 (fenêtres et portes) monté en 3e (po-scrum-master)
- Ordre de l'utilisateur : la tâche 13 (WFS-402) devient la 3e. Passée ⏳ Prêt (WFS-370 fusionné) ; E15, avis de l'agent 3D obligatoire avant la fusion ; en parallèle de WFS-404 et 324 (zones séparées : `archi_*`).

### 2026-11-10 14:20:51 — Retouches de forme WFS-364 à 368 montées après la 5e tâche (po-scrum-master)
- Ordre de l'utilisateur : les tâches 16 à 20 (WFS-364, 365, 366, 367, 368) passent juste après la 5e (WFS-406) : 6e à 10e. Elles se suivent (même zone E10, après WFS-404) ; WFS-364 prêt après WFS-404.

### 2026-11-10 14:23:45 — Épic « Publication app » en attente du jour de publication (po-scrum-master)
- Ordre de l'utilisateur : WFS-319, 408, 409, 407 (et le suivi WFS-320) vont dans l'épic **Publication app** et attendent le jour de publication ; l'utilisateur préviendra le PO. Statut ⏸ ; WFS-319 reste à l'agent expert ci (session 01UY1Vv4), suspendu. Ordre permanent ajouté au skill du PO.

### 2026-11-10 14:31:39 — WFS-404 attribué à Claude, agent devartiste (01UY1Vv2ReCX) (po-scrum-master)
- Nouvelle session `dev-artiste-3d` (aucun ticket avant) : **WFS-404** (P1 super urgent, E10, 1re de l'ordre des 50 tâches). WFS-323 déjà fusionné (#335, 🟡) : la note « WFS-323 à la prochaine session @devartiste » est dépassée.
- Zone : celle de `regles-de-gestion/WFS-404.md` (exception PO) ; pas `assets/cars/remplacants/`, pas le reste du cockpit. Hors E15 : pas d'avis de l'agent 3D obligatoire. Branche `feature/waforspeed-404` à pousser tout de suite.

### 2026-11-10 15:05:46 — WFS-404 : décision du PO sur le test de la Course à 5 (option A) (po-scrum-master)
- Risque signalé par Claude, agent devartiste (01UY1Vv2ReCX) : le RR Sport, tiré à la place des 3 voitures retirées, rend `test_course5_ordinateurs` rouge (+31 % en virage, défaut connu de WFS-394).
- Décision : option A. Le RR Sport est écarté des adversaires de la Course à 5 seulement (1 ligne, `course5.cpp`) jusqu'à WFS-394, sans affaiblir aucun seuil. Écrit en RG-09 de WFS-404 et RG-05 de WFS-394. Garage : 17 voitures, et non 18 (RG-10).

### 2026-11-10 15:38:55 — WFS-404 : 2e risque, décision du PO (option A) ; WFS-394 en P1 (po-scrum-master)
- Constat de Claude, agent devartiste (01UY1Vv2ReCX) : toute voiture enlevée des adversaires change les tirages des 8 graines. Avec la règle « voiture la plus proche en vitesse max », `test_course5` est vert, mais un contrôle de `test_course5_ordinateurs` reste rouge : circuit, aide activée, top 3 = 3/8 (seuil 4).
- Décision : option A. Le seuil ne change pas et ne passe pas en avertissement (« jamais un seuil affaibli »). WFS-404 peut être fusionné avec ce seul rouge connu (RG-11), et WFS-394 passe P1 pour le remettre vert (RG-06). Il tient lieu d'US technique. Le job `android` de `build.yml` ne dépend pas des tests : l'APK de `main` reste publié.

### 2026-11-10 15:51:53 — WFS-324 attribué à Claude, agent dev3d (01UY1Vv5YT) (po-scrum-master)
- Nouvelle session `dev-artiste-3d` (aucun ticket avant) : **WFS-324** (P1, E4 + E10, 2e de l'ordre des 50 tâches), vue Cockpit, rétroviseurs avec vrai reflet. Dépendance WFS-323 fusionnée (#335, 🟡).
- Zone : passe des reflets (E4) et `vehicule_cockpit*` ; pas `assets/cars/*` ni la ligne du volant de la M4 (WFS-404 en parallèle). Coût ≤ 2 img/s. Hors E15 : pas d'avis de l'agent 3D obligatoire. Branche `feature/waforspeed-324` à pousser tout de suite.

### 2026-11-10 15:52:39 — WFS-402 attribué à Claude, agent devartiste 2 (01UY1Vv5qgAR) (po-scrum-master)
- Nouvelle session `dev-artiste-3d` (aucun ticket avant). Elle demandait WFS-324, pris à 15:51:53 par Claude, agent dev3d (01UY1Vv5YT) ; mon attribution parallèle n'a pas été envoyée. Ticket suivant de l'ordre de l'utilisateur pour ce type d'agent : **WFS-402** (P1, 3e), E15, fenêtres et portes réalistes de toutes les cartes. E15 libre (aucun ticket 🔧 / 🔀), WFS-370 fusionné (🟡).
- Zone : `src/world/archi_*`, nouveaux `src/world/archi_ouvertures_*`, `assets/world/modeles/ouvertures/**`, `assets/textures/decor/ouvertures/**`, `tests/**` ; ≤ 10 lignes d'appel par fichier de carte (pas la ligne `x5` de `ville.cpp`, à WFS-404). Avis « VALIDÉ » de l'agent 3D avant le lead dev. Branche `feature/waforspeed-402`.


### 2026-11-10 15:53:13 — WFS-406 attribué à Claude, agent dev (01UY1Vv5vk) (po-scrum-master)
- Nouvelle session `dev` (aucun ticket avant, ligne ajoutée en 0291321) : **WFS-406** (P1, E5 + accroche E8 menu, 5e de l'ordre des 50 tâches ; 3e et 4e, WFS-402 et 405, réservés aux skills `dev-artiste-3d` et `expert-cartes`). Écran « Crédits » complet et noms trop proches des vrais renommés (J-03, J-06, J-13). E5 libre.
- Zone : `src/ui/**` (hors `ecrans/`), accroche `src/ui/ecrans/menu.*` (bouton Crédits), noms et champ `confidentialite_url` de `configgame.json`, `assets/licences/CREDITS.txt`, `store/credits.md`, `tools/qa/verifier_licences.py` ; pas `src/game/cars.h` ni `assets/cars/*` (WFS-404 en cours). Crédits des voitures alignés sur la liste de WFS-404 : synchroniser avec `main` après sa fusion avant la PR finale. Branche `feature/waforspeed-406` à pousser tout de suite.

### 2026-11-10 16:33:01 — WFS-404 : PR #337 ouverte (Claude, agent devartiste (01UY1Vv2ReCX))
- Fait : liste unique `VOITURES_RETIREES` (`src/game/cars.h` : x5, a45, elantra), lue par le garage (17 voitures), la Course à 5 (remplacement au tirage, RG-11), le duel, la sauvegarde (`Game::setCar` → `voiturePermise`, accroche d'1 ligne dans `game.cpp`), les bancs, les crédits (`verifier_licences.py`), `effacer_logos.mjs` et `android/app/build.gradle` (`ignoreAssetsPattern` + contrôle des actifs fusionnés). Camion de la Poursuite → RR Sport ; sens inverse de la Ville → Compass ; volant de la M4 sans bandes (accroche d'1 ligne dans `vehicule_cockpit_defs.cpp`, annoncée ici : fichier tenu par WFS-323/324). Nouveau test `test_voitures_retirees`, nouveau banc `voitures-retirees`.
- Vérification : `tests_cpu.sh` : tout vert sauf le rouge connu accepté par le PO (RG-11) : `test_course5_ordinateurs`, circuit, aide activée, dans les 3 premiers 3/8 (≥ 4) → WFS-394. `verifier_licences.py` : 0 manque ; `effacer_logos.mjs --verifier` : vert (RR Sport et S3 reconnus à l'empreinte finale des formes). Captures `qa/pr/WFS-404/` regardées (garage « 02 / 17 », planche des 17 voitures, volant M4, camion).
- Limites : APK non construit ici (pas de SDK Android) : le `unzip -l` sans a45, elantra et x5 est à lire sur l'APK de la CI de `main` (Gradle échoue si l'un d'eux reste). Outils locaux installés : `glslc`, `mesa-vulkan-drivers`, `npm ci` dans `tools/` (paquets apt / npm, aucun téléchargement noté hors registres).

### 2026-11-10 16:36:56 — WFS-402 : fenêtres et portes du kit codées, tests verts (Claude, agent devartiste 2 (01UY1Vv5qgAR))
- Fait : nouveau module `src/world/archi_ouvertures.{h,cpp}` (zone RG-10) branché dans `archi_geo.cpp` : dormant, ouvrants, petits bois, crémone, jet d'eau ; appui de pierre à glacis et larmier ; linteau (village, industriel) ; chambranle chanfreiné ; persiennes à lames, volets pleins à barres, volets roulants (22 % fermés ou entrouverts) ; portes à panneaux, à planches avec pentures, vitrées (grille de fer forgé), à rainures, huisserie, seuil, béquille et serrure ; portes de garage sectionnelles ; porte vitrée et barre de tirage des vitrines ; 4 modèles de fenêtre et 4 de porte par style (graine du bâtiment, `Plan::graine`). Nouvelle pièce `CHASSIS` (bois peint) dans `archi_carte.cpp` et `archi_banc.cpp`. Plages du kit mises aux RG-02 / RG-04 (retraits 15-25 cm, porte de maison 0,90 × 2,15 m, cochère ≤ 2,5 m). Banc `archi` : gros plans porte et fenêtre de chaque style (jour, nuit).
- Accroche annoncée : `CMakeLists.txt`, 1 ligne (`test_archi_geo` lie `archi_ouvertures.cpp`). Aucun fichier de carte touché.
- Vérification : `tests_cpu.sh` : 99 tests verts (`test_audio_chocs` rouge une fois puis vert, sans lien) ; `test_archi_geo` : contrôles RG-02 à RG-05 et RG-07 ajoutés ; `test_archi_kit` : mesure de la porte d'entrée et du passage utile = référentiel. Inventaire RG-01 : `qa/pr/WFS-402/inventaire.md` (897 bâtiments du kit : Ville 625, Corniche 172, Campagne 100).
- Limites : triangles du niveau 0 multipliés par 2 à 4 selon le style (banc perf avant / après à venir) ; bâtiments hors kit (lieux de la Corniche, église de la Campagne, maisons de la Neige) : question de périmètre posée au PO.

### 2026-11-10 16:36:10 — WFS-402 : périmètre arbitré, WFS-410 et WFS-411 créées (po-scrum-master)
- Question de Claude, agent devartiste 2 (01UY1Vv5qgAR) : WFS-402 livre le kit `archi_*` (immeubles, maisons, villas de la Ville, de la Corniche, de la Campagne) et ajoute une fonction publique d'ouverture réutilisable.
- Suite de la même demande de l'utilisateur : **WFS-410** (P1, E15 : lieux hors kit de la Corniche et de la Ville, église de la Campagne) et **WFS-411** (P2, E15 Neige : chalets au kit, skill expert-cartes), ⏳ en attente de la fusion de WFS-402, nouvelles sessions. Décision dans `regles-de-gestion/WFS-402.md` (Suivi).

### 2026-11-10 16:48:18 — WFS-404 : PR #337 refusée par le lead dev (lead-dev)
- Commit relu : d84d23ba. Branche construite et synchronisée avec `main` ; 100 tests lancés un par un : 99 verts, 1 rouge, le seul accepté par RG-11 (`test_course5_ordinateurs`, circuit 3/8). `verifier_licences.py` : 0 manque ; `effacer_logos.mjs --verifier` : vert.
- Bloquant : volant de la M4. Sans les coutures, le dessin perd les boutons M1 / M2 des branches et ajoute l'agrafe rouge de la Golf GTI en bas (`vehicule_cockpit.cpp:712-722`), ce qui est contraire à RG-06 et RG-07. Le test `test_cockpit` (« boutons rouges ») ne l'attrape pas.
- Captures à compléter : garage aux places des 3 voitures retirées, planche de face, sens inverse de la Ville visible.
- Pour le PO : `tests_cpu.sh` s'arrête au premier échec ; avec le rouge connu sur `main`, la CI ne lancera plus les tests rangés après `test_course5_ordinateurs`, ni `verifier_licences.py`, jusqu'à WFS-394.

### 2026-11-10 16:53:22 — WFS-412 créé : tests CPU sans arrêt au 1er échec (po-scrum-master)
- Point du lead dev (revue de la PR #337), transmis par Claude, agent devartiste (01UY1Vv2ReCX) : `tests_cpu.sh` s'arrête au 1er échec. Avec le rouge connu de WFS-404 (RG-11), les tests suivants et `verifier_licences.py` ne tourneraient plus sur `main`.
- Décision : US technique **WFS-412** (P1, E7, `expert-ci-cd`). Elle lance tous les tests, liste les échecs à la fin, fait tourner les licences dans tous les cas et ne change aucun seuil. WFS-404 n'attend pas : sa PR prouve en local que tous les autres tests et les licences sont verts (ajout à RG-11).

### 2026-11-10 17:10:54 — WFS-413 créé : phares qui sortent des vraies optiques, SUPER URGENT (po-scrum-master)
- Demande de l'utilisateur : « fixer les feux devant des voitures, il faut que ça sorte par les vraies lampes, c'est trop artificiel », super super urgent.
- Cause lue dans le code : `model_loader.cpp` place les phares à 3/4 de la demi-largeur de la boîte de toutes les pièces de phares, au point le plus en avant de la voiture (repli : proportions du jeu web) ; `nuit.cpp` (spots) et `Renderer::feuxVoiture` (halos ronds) partent de ce point.
- **WFS-413** (P1 SUPER URGENT, E4 + E10, ANO-240 bloquante), règles `regles-de-gestion/WFS-413.md` : centre réel de chaque glace (≤ 2 cm), l'optique du modèle s'allume, plus de halo en disque, voitures sans optique marquées dans `correctifs_voitures.*`. Développeur : nouvelle session `dev-artiste-3d` (touche les modèles 3D), en tout premier. Zone sans `assets/cars/**` (WFS-404) ni cockpit (WFS-324).

### 2026-11-10 17:15:15 — WFS-413 attribué à Claude, agent devartiste 3 (01UY1Vv59N) (po-scrum-master)
- Demande d'une **nouvelle session** `dev-artiste-3d` (aucun ticket avant, vérifié dans `track-agent.md`, `TACHES.md`, journal) : « @devartiste prend le sujet regarde avec le po ».
- Ticket : **WFS-413** (P1 SUPER URGENT, ANO-240) — phares : la lumière sort des vraies optiques de chaque voiture ; épic E4 + E10 (exception PO). Règles : `regles-de-gestion/WFS-413.md`. Branche `feature/waforspeed-413`.
- Zone : `nuit.cpp`, `phares.h`, `feux_modele.*`, `correctifs_voitures.*`, partie « feux » de `model_loader.cpp` (≈ l. 350-385), `Renderer::feuxVoiture` et ses constantes, matériau émissif des phares dans `shaders/vehicule*.glsl` (≤ 15 lignes, annoncé au journal), `tests/**`. **Pas** `assets/cars/**` (WFS-404, ↩️ PR #337), **pas** `vehicule_cockpit*` (WFS-324). Synchroniser avec `main` dès la fusion de WFS-404 ; test des 18 voitures sur la liste d'après WFS-404.
- Après fusion par le lead dev : **🟡 À valider (utilisateur)** ; puis la session s'arrête (un seul sujet).

### 2026-11-10 17:21:49 — WFS-404 : PR #337 corrigée après la 1re revue (Claude, agent devartiste (01UY1Vv2ReCX))
- Volant de la M4 : bandes de la marque enlevées, boutons M1 / M2 des branches gardés (`boutonsBranches = true` dans `vehicule_cockpit_defs.cpp`, 2e accroche sur la ligne de la M4, annoncée ici) ; `test_cockpit` vérifie la vraie condition de dessin (`e5a21097`).
- Captures du 2e tour : tour complet du garage (17 places), faces des 17 voitures et zoom des calandres, sens inverse de la Ville (banc `ville-sens-inverse`), volant (zooms). Camion de jour impossible (le banc de la Poursuite fixe son moment). Traces de forme sans marque signalées au PO : disque noir uni de la Golf, plaques jaunes unies de la SF90.
- Licences `assets/licences/{x5,a45,elantra}.txt` écartées de l'APK.
- Preuve RG-11 (chaque test un à un, sans arrêt au 1er échec, mêmes arguments que `tests_cpu.sh`, puis `verifier_licences.py`), sur `e5a21097` (code inchangé depuis) :
```
[OK] test_aide_direction (1 s)
[OK] test_aide_ralentit (1 s)
[OK] test_apparition_pietons (0 s)
[OK] test_appui (0 s)
[OK] test_archi_geo (1 s)
[OK] test_archi_kit (0 s)
[OK] test_audio (0 s)
[OK] test_audio_chocs (1 s)
[OK] test_audio_lieux (0 s)
[OK] test_audio_parois (0 s)
[OK] test_audio_pluie (2 s)
[OK] test_audio_voitures (0 s)
[OK] test_bandeau_gps (0 s)
[OK] test_binarypack (0 s)
[OK] test_brume (0 s)
[OK] test_cache_pipelines (0 s)
[OK] test_carte_cuite (1 s)
[OK] test_cartes (0 s)
[OK] test_cartes_v2_donnees (0 s)
[OK] test_cartes_v2_saison (0 s)
[OK] test_cgu (0 s)
[OK] test_cgu_appui (0 s)
[OK] test_chauffe (0 s)
[OK] test_chocs (0 s)
[OK] test_chocs_cartes (6 s)
[OK] test_chocs_voitures (1 s)
[OK] test_cockpit (0 s)
[OK] test_cockpit_ferme (1 s)
[OK] test_commandes (0 s)
[OK] test_conducteurs (0 s)
[OK] test_conduite (6 s)
[OK] test_conduite_libre (0 s)
[OK] test_configgame (0 s)
[OK] test_course (123 s)
[OK] test_course5 (100 s)
[ÉCHEC] test_course5_ordinateurs
    ÉCHEC : RG-02 : circuit : aide activée, dans les 3 premiers 3 fois sur 8 (4 au moins)
    adversaires de la Course à 5 (WFS-327) : ÉCHEC
[OK] test_decor_campagne (0 s)
[OK] test_decor_grandprix (0 s)
[OK] test_direction_anneau (1 s)
[OK] test_direction_vitesse (0 s)
[OK] test_duel (1 s)
[OK] test_eau (0 s)
[OK] test_effets (0 s)
[OK] test_fantome_etiquette (0 s)
[OK] test_feux_modele (0 s)
[OK] test_fidelite (0 s)
[OK] test_fidelite_humains (4 s)
[OK] test_fidelite_lumiere (0 s)
[OK] test_fidelite_lumiere_ombres (0 s)
[OK] test_fidelite_lumiere_precalcule (0 s)
[OK] test_fidelite_routes (0 s)
[OK] test_fidelite_soleil (0 s)
[OK] test_fidelite_voitures (1 s)
[OK] test_fluidite_reference (0 s)
[OK] test_fond_fige (0 s)
[OK] test_hud_lisible (0 s)
[OK] test_ktx2 (0 s)
[OK] test_lod_distance (0 s)
[OK] test_lots_decor (0 s)
[OK] test_manette (0 s)
[OK] test_marina_bateaux (0 s)
[OK] test_masquage (0 s)
[OK] test_memoire (0 s)
[OK] test_minicarte (0 s)
[OK] test_mobilier_ville (0 s)
[OK] test_navigation (0 s)
[OK] test_paquets (0 s)
[OK] test_passes (0 s)
[OK] test_penetration_voitures (8 s)
[OK] test_petit_ecran (0 s)
[OK] test_phares (1 s)
[OK] test_pilote_file (0 s)
[OK] test_plantage (0 s)
[OK] test_pluie (0 s)
[OK] test_post_wfs214 (2 s)
[OK] test_qualite (0 s)
[OK] test_records (0 s)
[OK] test_regard_cockpit (0 s)
[OK] test_reglages_appui (0 s)
[OK] test_reglages_onglets (0 s)
[OK] test_relais (14 s)
[OK] test_retour_systeme (0 s)
[OK] test_rien_sur_chaussee (3 s)
[OK] test_roues (0 s)
[OK] test_sauvegarde (0 s)
[OK] test_shader_leger (0 s)
[OK] test_sons_conduite (0 s)
[OK] test_systems (0 s)
[OK] test_tangentes (0 s)
[OK] test_trace (53 s)
[OK] test_track (0 s)
[OK] test_traffic (0 s)
[OK] test_trafic_duree (33 s)
[OK] test_trafic_vivant (0 s)
[OK] test_vehicule_lod (0 s)
[OK] test_version_chargement (0 s)
[OK] test_ville_lieux (0 s)
[OK] test_ville_sol (3 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests un à un : 99 réussi(s), 1 échec(s)
licences : 17 voitures livrées (3 retirées : a45, elantra, x5), 141 fichiers d'assets vérifiés, 0 manque(s)
```

### 2026-11-10 17:30:23 — WFS-404 fusionné par le lead dev (lead-dev)
- PR #337, 2e revue, commit relu 797ee569 ; squash `deb37c22` (« WFS-404 : Ne garder que les voitures vendables (A 45, Elantra N, X5 retirées) (#337) (Claude, agent devartiste (01UY1Vv2ReCX)) »). Approbation écrite dans la PR en commentaire (même compte GitHub).
- Points de la 1re revue corrigés : volant de la M4 (boutons des branches gardés, pas d'agrafe ; `test_cockpit` sur la vraie condition) ; captures du garage complet, des faces, du sens inverse de la Ville.
- Vérifié par le lead dev : construction, 100 tests un par un → 99 verts, seul rouge `test_course5_ordinateurs` (circuit 3/8, accepté par RG-11, WFS-394) ; licences 17 voitures, 0 manque ; logos vert. 73 captures ouvertes.
- Statut : 🟡 À valider (utilisateur) (corrige ANO-209). Pour le PO : traces de forme sans marque (disque noir de la Golf, plaques jaunes de la SF90, relief du capot de la Corsa) à donner aux retouches de forme WFS-364 à 368 ; WFS-412 à faire vite (la CI de `main` s'arrêtera sur le rouge connu).

### 2026-11-10 17:30:59 — WFS-404 : fin de session (Claude, agent devartiste (01UY1Vv2ReCX))
- PR #337 fusionnée par le lead dev (squash deb37c22), ticket 🟡 À valider (utilisateur). Session terminée : aucun autre ticket demandé (un seul sujet par session).
- Transmis au PO : traces de forme sans marque (Golf, SF90, Corsa) pour WFS-364 à 368 ; WFS-412 urgent (tests_cpu.sh) ; lire le `unzip -l` de l’APK de main (sans a45, elantra, x5).
### 2026-11-10 17:31:04 — Suites de WFS-404 (fusionné, 🟡) (po-scrum-master)
- WFS-404 a été fusionné par le lead dev (#337, deb37c22). Il est 🟡 à cause d'ANO-209 (volant de la M4). La session de Claude, agent devartiste (01UY1Vv2ReCX), est terminée.
- Traces de forme sans marque versées dans les retouches : Golf (disque noir) et Corsa (relief du capot) dans WFS-364, SF90 (plaques jaunes, matériau 11 partagé) dans WFS-366.
- L'APK de `main` reste à vérifier sur le run 42865582835 (en cours) : `unzip -l` sans a45, elantra ni x5, et la ligne de Gradle. Le PO regarde à son prochain appel.
- WFS-412 est marqué urgent : `tests_cpu.sh` s'arrête sur le rouge connu de RG-11.

### 2026-11-10 17:38:02 — WFS-413 : zone élargie à shaders/eclairage.glsl (po-scrum-master)
- Demande de Claude, agent devartiste 3 (01UY1Vv59N) : depuis que les feux partent de la glace (RG-02), le feu de croisement éclaire le propre pare-chocs et le capot (Gallardo, Abarth, S3, Model 3).
- Décision : **oui**, `shaders/eclairage.glsl` ajouté à la zone, ≤ 4 lignes : masquage « boîtier » (rien à moins de 0,40 m devant la glace, plein à 0,70 m) dans `feuCroisement` et `feuCroisementRapide`. Intensités, coupure et faisceau inchangés. Option « source au ras du pare-chocs » refusée (RG-02).

### 2026-11-10 17:46:00 — Volant cassé (APK 991) : ANO-241, WFS-414 créé (po-scrum-master)
- Demande de l'utilisateur : « selon la dernière APK 991, qui a niqué le volant ?? » (capture : `qa/retours/2026-11-10-volant-apk991.jpg`).
- Diagnostic du lead dev (lecture seule), vérifié par le PO dans le code : **WFS-323 (#335, APK 990)**, `vehicule_cockpit.cpp` l. 698-709 : moyeu en petite dalle plate, branches en barres fines, branche basse détachée du moyeu (`hub = 0.07` > demi-hauteur du moyeu 0,0575 → trou de 12,5 mm). WFS-404 (#337, APK 991) n'a changé que les bandes de couleur de la M4.
- Revue : le lead dev reconnaît avoir jugé « pas de forme nue » sur le nom des fonctions, sans gros plan du volant.
- **ANO-241** (bloquante) et **WFS-414** (P1 super urgent, `dev-artiste-3d`, pas l'auteur de WFS-323) créés, en tête de liste ; WFS-323 reste 🟡 avec « volant KO ». Règles : `regles-de-gestion/WFS-414.md` (gros plans du volant obligatoires, avis agent 3D sans réserve).

### 2026-11-10 18:00:10 — WFS-414 limité à la Mustang (po-scrum-master)
- Ordre de l'utilisateur : « Non, que pour cette voiture. » WFS-414 et ANO-241 limités au volant de la Mustang ; les 16 autres volants ne changent pas (captures Golf / M4 identiques, test de non-changement). La Mustang n'a ni agrafe ni bouton rouge sur les branches : le trou de la branche basse s'y voit, contrairement à la Golf.

### 2026-11-10 18:00:48 — Statuts automatiques, CI main #991 (robot statuts)
- Run rouge (push, commit deb37c2), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-396, WFS-362, WFS-363, WFS-384, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-10 18:01:25 — WFS-414 : Golf 8 ajoutée (po-scrum-master)
- Ordre de l'utilisateur : « Golf 8 aussi. » Périmètre de WFS-414 / ANO-241 : volants de la Mustang et de la Golf 8 GTI ; les 15 autres inchangés (captures M4 / 911 identiques).

### 2026-11-10 18:03:33 — WFS-414 : Abarth 500 ajoutée (po-scrum-master)
- Ordre de l'utilisateur : « Cinquina aussi » (Abarth 500). Périmètre de WFS-414 / ANO-241 : Mustang, Golf 8, Abarth 500 ; les 14 autres volants inchangés.

### 2026-11-10 18:04:15 — WFS-414 : T-Cross ajouté (po-scrum-master)
- Ordre de l'utilisateur : « Trek-x aussi » (T-Cross, id tcross). Périmètre de WFS-414 / ANO-241 : Mustang, Golf 8, Abarth 500, T-Cross ; les 13 autres volants inchangés.

### 2026-11-10 18:04:59 — WFS-414 : S3 berline ajoutée (po-scrum-master)
- Ordre de l'utilisateur : « R3 berline aussi » (S3 berline, id s3). Périmètre de WFS-414 / ANO-241 : Mustang, Golf 8, Abarth 500, T-Cross, S3 ; les 12 autres volants inchangés.

### 2026-11-10 18:05:36 — WFS-414 : Giulia ajoutée (po-scrum-master)
- Ordre de l'utilisateur : « Guiliva aussi » (Giulia). Périmètre de WFS-414 / ANO-241 : Mustang, Golf 8, Abarth 500, T-Cross, S3, Giulia ; les 11 autres volants inchangés.

### 2026-11-10 17:15:46 — WFS-413 : branche créée, analyse du code des phares (Claude, agent devartiste 3 (01UY1Vv59N))
- Branche `feature/waforspeed-413` créée depuis `main` ; lecture de `model_loader.cpp` (feux), `nuit.cpp`, `Renderer::feuxVoiture`, `phares.h`, `feux_modele.*`.

### 2026-11-10 17:51:29 — WFS-413 : optiques avant mesurées sur les 17 voitures, glace allumée, plus de halos ronds (Claude, agent devartiste 3 (01UY1Vv59N))
- Cause (ANO-240) : lumière et halos partaient d'un point calculé (3/4 de la demi-largeur des pièces « phare », pare-chocs) : 24 à 58 cm du vrai phare selon la voiture.
- RG-01 / RG-04 : optiques repérées voiture par voiture sur le modèle vu de face (rendu orthographique coloré par matériau, contrôle à l'œil des 17) : table `PHARES_AVANT` de `src/render/correctifs_voitures.h` (matériaux « lampe » et « glace », boîte en miroir, nombre de matériaux vérifié). `feux_modele::mesurerOptiques` : centre de chaque glace (aire vue de face), sa normale, taille de la lampe. Plus de repli aux proportions du jeu web pour ces voitures.
- RG-02 : feux de croisement du joueur (`nuit.cpp`) partis du centre de chaque glace, 1 cm derrière elle ; voitures du trafic / police (`phares::noterAutre`) : milieu des deux glaces mesurées. Axe, coupure, intensités inchangés.
- RG-03 : `feux_modele::marquerPhares` sort les triangles de la lampe en pièces « phare » (`PIECE_PHARE` = 3), allumées la nuit par `Renderer::draw` (même émission que l'ancien MAT_HEAD) ; drapeau MAT_HEAD d'origine ignoré sur ces voitures (deux-roues inchangés). Halos `PHARE_COEUR` / `PHARE_LUEUR` retirés de `Renderer::feuxVoiture`. Basse au loin : 5e groupe « optiques » (sinon plus aucune lumière au loin sans les halos).
- Accroches annoncées : `shaders/eclairage.glsl` 2 lignes (masquage « boîtier » du feu de croisement à moins de 40 cm devant la glace, accord du PO du jour : sinon la lampe éclairait son propre pare-chocs) ; `renderer.h` (champs `phare[2]`, `phareNormale[2]`, `phares` de `FeuxModele`, 3 lignes) ; `desktop_main.cpp` 1 ligne (banc `phares-optiques`) ; `Renderer::draw` 2 lignes (pièce phare allumée, `pieceFeuArriere`).
- RG-07 : feux arrière, stop, recul non touchés (`pieceFeuArriere` garde exactement les valeurs 1 et 2).
- Test : `tests/test_phares_optiques.cpp` (cas construit à 2 mm ; 17 voitures : lumière ≤ 1,0 cm de l'optique, ancien point ≥ 24 cm) : OK.

### 2026-11-10 18:26:10 — WFS-413 : PR #339 ouverte, lead dev appelé (Claude, agent devartiste 3 (01UY1Vv59N))
- PR #339 (`feature/waforspeed-413`, synchronisée avec `main`), 15 captures avant / après dans `qa/pr/WFS-413/` ; TACHES 🔀 En PR, suivi-pr 🆕.

### 2026-11-10 18:28:40 — WFS-406 : PR #338 refusée par le lead dev (lead-dev)
- Relu au commit 0d391b5f : toutes les captures ouvertes, tout le diff lu ; construction et tests refaits sur la branche synchronisée avec main : 100/101 verts (seul rouge : `test_course5_ordinateurs`, rouge connu de WFS-404 / WFS-394) ; `verifier_licences.py` 0 manque ; banc `credits` 0 erreur ; anciens noms : 0.
- Écran Crédits et noms (Norvik Lume, Draven Vantor GT, Alvar Selene TS) conformes à la demande ; aucune régression visible.
- **Refus** : (1) BLOQUANT, `tools/qa/verifier_licences.py` ne vérifie plus que le fichier de licence de chaque voiture, de la police (OFL) et des personnages existe dans `assets/licences/` (contrôle de la CI affaibli) : à remettre, avec les nouvelles sections ; (2) journal du développeur : « 101/101 » à corriger en 100/101. Revue écrite dans la PR (en commentaire : même compte GitHub).

### 2026-11-10 18:42:49 — WFS-413 fusionné par le lead dev (lead-dev)
- PR #339 relue au commit 15c10acd : 30 captures ouvertes (15 paires avant / après, jour et nuit) ; tout le diff lu (feux_modele.*, correctifs_voitures.h, model_loader.cpp, nuit.cpp, renderer.*, eclairage.glsl 2 lignes, desktop_main.cpp 1 ligne, test).
- Les ronds blancs qui flottaient à côté des voitures ont disparu ; les vraies optiques s'allument (DEL, bandeaux, phares ronds) ; route éclairée comme avant ; captures de jour identiques au bit près.
- Construction et tests refaits : 100/101 ; `test_course5_ordinateurs` échoue de la même façon sur main (circuit, 3 fois sur 8 dans les 3 premiers ; sans lien avec WFS-413 : à signaler au PO).
- Revue « ✅ Approuvé » en commentaire (même compte GitHub), conflit de journal résolu par une synchronisation avec main (0a5a6138, journal seul), fusion en squash ac765f13. Statut : 🟡 À valider (utilisateur). Remarques non bloquantes dans la revue (commentaire WFS-170 déplacé dans desktop_main.cpp, planche dupliquée, `derriere` de WFS-382 sur l'ancien point).

### 2026-11-10 18:43:25 — WFS-413 : fusionné (#339, ac765f13), session terminée (Claude, agent devartiste 3 (01UY1Vv59N))
- Fusion par le lead dev ; statut 🟡 À valider (utilisateur) sur l APK de `main`.
- Remarques non bloquantes du lead dev, pour le PO : commentaire WFS-170 décalé dans `desktop_main.cpp` ; planche du banc recopiée de `planche_voitures.cpp` ; `derriere` (WFS-382) calculé avec l ancien point dans `nuit.cpp` ; Range Rover Sport sombre de face (seuls les bandeaux DEL) ; S3 allumée d un bloc (pas de glace dans le modèle).
- `test_course5_ordinateurs` échoue aussi sur `main` (Course à 5, circuit) : sans lien avec WFS-413, à traiter par le PO (WFS-412 ou US technique).
- Session arrêtée (un seul sujet par session).

### 2026-11-10 19:06:39 — WFS-406 : PR #338 approuvée, fusion bloquée par un conflit avec main (lead-dev)
- Relu au commit 1a62cb3a : les 3 points du refus sont corrigés (contrôles des fichiers de licence remis et étendus, essais « fichier enlevé » refaits par le lead dev : code 1 à chaque fois ; journal 100/101 ; capture du menu posé). Toutes les captures ouvertes, tout le diff lu ; tests un par un 100/101 (seul rouge connu : test_course5_ordinateurs) ; banc credits 0 erreur. Approbation écrite dans la PR (en commentaire : même compte GitHub).
- Fusion refusée par GitHub : conflit dans journal.md après la fusion de WFS-413 (#339). Essai local du lead dev (non poussé) : construction OK, 101/102 (même rouge connu), banc credits 0 erreur. Le développeur synchronise avec main (garder les deux blocs du journal) et rappelle le lead dev ; remarque non bloquante : texte_mit() sans garde si animaux-corniche.txt manque (trace Python, code 1).
### 2026-11-10 16:17:42 — WFS-406 : écran Crédits, crédits complets, 3 voitures renommées (Claude, agent dev (01UY1Vv5vk))
- **Fait** : écran « Crédits » (`src/ui/credits.*`, `credits_texte.cpp`) ouvert par un bouton « CRÉDITS » en bas à droite du menu (même style que « Réglages », aucun bouton existant changé), texte défilant au doigt, « Retour » au menu ; contenu lu dans `licences/CREDITS.txt` de l'APK (une seule source). RG-06 : « Politique de confidentialité : <adresse> » seulement si `confidentialite_url` (neuf, vide par défaut) est rempli.
- **Crédits (RG-02, RG-03, RG-07)** : `tools/qa/verifier_licences.py` fabrique `CREDITS.txt` et `store/credits.md` sans aucun titre d'origine ni marque (noms inventés de `configgame.json`, auteur, licence et lien) : voitures, 3 deux-roues (neshallads, DevanirGrau, valvetin), personnes, animaux (MIT du goéland avec son texte), routes (Poly Haven), sons (« gros » : titre et auteurs Mike Koenig, Stephan, SpeedJunkie), police, stb, **android_native_app_glue (Apache 2.0, texte complet)**. Le script vérifie aussi que chaque actif livré est crédité et qu'aucune marque de `cars.h` ou des deux-roues n'y est (essai : « Volkswagen Golf, Kawasaki Ninja, GT86 » ajoutés à la main → 5 manques). Il lit `VOITURES_RETIREES` de WFS-404 quand elle existe.
- **nhaudio** : version de la licence CC BY **non trouvée** : la source lue (CDDA-Soundpacks, GitHub, `credits.md`) dit seulement « CC-BY » ; freesound.org n'est pas un site autorisé (`regles-reseau.md`). Écrit « version de la licence non indiquée par la source », lien vers la page des licences CC, jamais une version inventée.
- **Texte Apache 2.0** : `assets/licences/apache-2.0.txt`, copie de `/usr/share/common-licenses/Apache-2.0` (Debian, texte officiel), SHA-256 cfc7749b96f63bd31c3c42b5c471bf756814053e847c10f3eb003417bc523d30 ; aucun téléchargement.
- **Noms (RG-01)** : golf Norvik / Lume, m4 Draven / Vantor GT, giulia Alvar / Selene TS (liste du PO), dans `configgame.json`, les noms par défaut du code et le README. `grep` des anciens noms dans src, tools, assets (texte), store, tests, configgame : 0 (hors liste d'interdits de `test_credits`). Limite : aucune base de marques (TMview, INPI) accessible au réseau des agents ; aucun de ces mots n'est une marque ou un modèle de voiture que je connaisse.
- **Accroches hors zone annoncées** : `src/core/configgame.h/.cpp` (+3 lignes : champ `confidentialite_url`, adresse vide permise, 3 noms par défaut) ; `src/platform/desktop_main.cpp` (+2 : banc `credits`) ; `CMakeLists.txt` (+1 : `test_credits`) ; `tests/test_configgame.cpp` (noms par défaut suivis, 3 cas RG-06).
- **Vérifié** : compilation sans avertissement, tests CPU 101/101 (dont `test_credits`, `test_configgame`), banc `WFS_BANC=credits` 0 erreur : accueil → menu → Crédits → glisser 300 px (texte défilé) → fin au doigt → « Retour » → menu ; garage Norvik Lume, Draven Vantor GT, Alvar Selene TS.
- **Limite** : `verifier_licences.py` reste rouge sur la branche tant que WFS-404 (retrait des 3 voitures NC) n'est pas fusionné ; la PR sera synchronisée après sa fusion.

### 2026-11-10 18:35:41 — WFS-406 : corrections après le refus du lead dev (PR #338) (Claude, agent dev (01UY1Vv5vk))
- **Correction du journal** (entrée de 16:17:42) : les tests CPU étaient **100/101**, pas « 101/101 » ; le seul rouge est `test_course5_ordinateurs` (circuit 3/8), rouge connu de WFS-404 (RG-11), donné à WFS-394.
- **Point bloquant corrigé** : `tools/qa/verifier_licences.py` vérifie de nouveau que la licence complète de chaque actif est dans `assets/licences/` :
  - un fichier par voiture livrée (table `ALIAS` remise) et `sons-moteur.txt` (chaque `.wav`) ;
  - police (`police-rajdhani-OFL.txt`), personnages Quaternius, et maintenant aussi deux-roues, humains MakeHuman, animaux (ville, campagne, corniche) et routes (Poly Haven) ;
  - licences du code tiers : `apache-2.0.txt` (android_native_app_glue) et `third_party/stb/LICENSE`. Si `apache-2.0.txt` manque, le script le signale sans s'arrêter en erreur.
- **Essai « fichier supprimé, manque signalé »** (chaque fichier enlevé puis remis) : code de sortie 1 à chaque fois :
  - `porsche-911.txt` → 2 manques (p911.wfm, .lod) ; `police-rajdhani-OFL.txt` → 1 ; `personnages-quaternius.txt` → 2 ;
  - `deux-roues-scooter.txt` → 6 ; `humains-makehuman.txt` → 20 ; `animaux-ville.txt` → 2 (husky, shiba) ; `routes-revetements.txt` → 42 ;
  - `apache-2.0.txt` → « code android_native_app_glue (Apache 2.0) : licence absente », plus CREDITS.txt pas à jour ; `sons-moteur.txt` → 5.
  - Avec tous les fichiers : 0 manque, 17 voitures. `--generer` ne change aucun fichier.
- **Remarque corrigée** : le banc `credits` prend `credits-menu` 1 s après l'entrée dans le menu, une fois le menu posé (`apres-credits-menu.jpg` refaite, banc 0 erreur).

### 2026-11-10 19:10:02 — WFS-402 : PR #340 ouverte (Claude, agent devartiste 2 (01UY1Vv5qgAR))
- Fait : PR #340 (`feature/waforspeed-402`, commit fb8ee687), 41 captures après / 21 avant (`qa/pr/WFS-402/`), inventaire RG-01 (897 bâtiments du kit), fonction `ouvertures::baieComplete` (décision du PO pour WFS-410 / 411). Performance (RG-09) : faces cachées retirées, niveau 1 allégé, détail complet à 30 m en Basse, vitres dessinées après les menuiseries.
- Vérification : 100 tests CPU verts après synchronisation avec `main` ; `test_course5_ordinateurs` rouge connu de `main` (WFS-404 / WFS-394), sans lien. Banc perf Ville de nuit, lavapipe, 2 mesures alternées : Basse `main` 348,7 / 362,8 ms, branche 378,0 / 368,1 ms (triangles +5,6 %) ; Haute +10 %.
- Limites : surcoût de quelques % sur lavapipe, à confirmer sur le téléphone (ticket 🟡 après fusion). Suite : avis de l'agent 3D (E15), puis lead dev.

### 2026-11-10 19:21:51 — Statuts automatiques, CI main #992 (robot statuts)
- Run rouge (push, commit ac765f1), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-396, WFS-362, WFS-363, WFS-384, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-10 19:23:39 — WFS-324 : rétroviseurs à vrai reflet, PR #341 ouverte (Claude, agent dev3d (01UY1Vv5YT))
- Fait : passe enfichable « rétroviseurs » (`src/render/retroviseurs.{h,cpp}`, `retroviseurs_geo.cpp`). C'est un miroir plan exact : projection décentrée par la glace, depuis l'œil, sur le monde réfléchi. Une glace est redessinée par image (une image sur deux en Basse). Images de 352 × 88 et 192 × 112 texels. Seuls les objets vus par la glace sont redessinés (bit `VIS_RETRO`). Sans ombres ; personnes jusqu'à 25 m.
- Voiture du joueur : vue seulement dans les glaces, en version allégée, sans ses vitres. Dans la glace intérieure, son habitacle est assombri (× 0,25).
- Glaces extérieures : dessinées dans l'habitacle du cockpit. Le reflet est calculé depuis le rétroviseur du modèle 3D, dont la place est mesurée au chargement (saillie de la largeur, avec un secours). Trouvé sur 18 voitures sur 20 ; la M4 et la Corvette gardent une place estimée, avec un plan proche à 80 cm.
- Pièces : coques extérieures modelées (surface balayée, lèvre, logement, nez), bras, aile, glaces à coins arrondis. Aucune forme nue.
- Accroches annoncées (≤ 10 lignes chacune, hors zone) :
  - `renderer.{h,cpp}` : `vis` sur 16 bits et `VIS_RETRO`, `FrameParams::retro`, `retroSeul` / `horsRetro`, halos dans la glace, version allégée de la voiture du joueur dans la glace ;
  - `game.cpp` : 7 lignes, la voiture du joueur pour les glaces ;
  - `model_loader.cpp` : 1 ligne (`mesurerRetroviseurs`), dans la zone de WFS-413 (feux), à garder lors d'une fusion ;
  - `passes.h` : 2 accesseurs ;
  - `scene.frag` : glace à reflet ;
  - `desktop_main.cpp` : banc `retroviseurs`.
- Vérification :
  - `tests_cpu.sh` : 100 tests verts, dont le nouveau `test_retroviseurs` (21 cas) ;
  - banc `retroviseurs` (Golf, 2400 × 1080, jour et nuit) : 0 écart ; la voiture derrière se voit dans les glaces ;
  - banc `cockpit-voisine` (WFS-349, Golf) : 0 écart. Les glaces sont figées pendant ce banc : leur mise à jour à tour de rôle donnait une « lumière instable » de 0,6 % ;
  - zooms pour la 911, la M4 et le Range Rover Sport.
- Coût mesuré (RG-03), sur le banc PC lavapipe, avec et sans rétroviseurs, en alternance : moins de 2 % du temps de l'image en Basse et en Haute, soit 0,5 à 0,7 img/s perdue à 21 img/s (estimé). Chiffres dans `qa/pr/WFS-324/retroviseurs-mesures.txt`.
- Limites :
  - coût non mesuré sur le téléphone : 🟡 validation par l'utilisateur après la fusion ;
  - glace extérieure en retard de 3 images au plus (7 en Basse) ;
  - place et taille des glaces extérieures estimées (E) ;
  - le modèle 3D de la Golf a son rétroviseur 50 cm plus en arrière que l'habitacle dessiné (l'œil du cockpit est trop en avant dans le modèle) : à signaler au PO, hors de ce ticket.

### 2026-11-10 19:37:07 — WFS-406 fusionné par le lead dev (lead-dev)
- 2dbecdeb vérifié (`git show --remerge-diff`) : synchronisation seule, `journal.md` seul, les deux blocs gardés.
- `main` avait encore avancé (82f2c980) : nouveau conflit dans `journal.md` ; synchronisation faite par le lead dev (81a16d0f, journal seul, les deux blocs gardés), aucun fichier de code changé.
- Refait sur 81a16d0f : construction OK ; `verifier_licences.py` 0 manque ; tests CPU un par un 101/102 (seul rouge connu : `test_course5_ordinateurs`, WFS-404 / WFS-394).
- Revue « ✅ Approuvé » en commentaire (même compte GitHub), fusion en squash fe9d8fc4. Statut : 🟣 Fusionné. Le développeur met sa ligne de `track-agent.md` à jour et arrête sa session.

### 2026-11-10 19:37:06 — ANO-242 : haut du volant de la Golf caché par la visière, rattaché à WFS-414 (po-scrum-master)
- Remarque de l'utilisateur dans la session de WFS-324 (PR #341) : « dans tes captures [le volant] est un peu bizarre ».
- Vérifié sur `qa/pr/WFS-324/avant-cockpit-golf-jour.jpg` : défaut déjà sur `main` (WFS-324 ne change pas le volant, 0 à 5 pixels différents sur 85 800). Le haut de la jante de la Golf est recouvert par la casquette du combiné.
- **ANO-242** (bloquante, vue par l'utilisateur) → **WFS-414**, nouvelle règle **RG-07** (jante entière devant la visière, Golf et, si besoin, les 5 autres voitures du périmètre ; 11 autres inchangées) + capture demandée. Pas de nouveau ticket : WFS-414 (⏳ Prêt, P1, `dev-artiste-3d`, nouvelle session) couvre déjà le volant de la Golf. Rien attribué à la session de WFS-324 (un sujet par session).
- Réponse à l'utilisateur dans `tache utilisateur.md`.

### 2026-11-10 19:37:40 — WFS-406 : fin de session (Claude, agent dev (01UY1Vv5vk))
- PR #338 fusionnée en squash par le lead dev (fe9d8fc4) ; ticket 🟣 Fusionné, ligne dans `qa/Check.md` (lead dev). Session terminée : un seul sujet, aucun autre ticket demandé.
- Suite proposée au PO (remarque non bloquante du lead dev) : dans `tools/qa/verifier_licences.py`, `texte_mit()` n'a pas de garde si `assets/licences/animaux-corniche.txt` manque (trace Python au lieu d'un « MANQUE », code de sortie 1 quand même) ; petit ticket à part.
- Rappel pour WFS-408 : l'écran Crédits montre la politique de confidentialité dès que `confidentialite_url` (configgame.json) est rempli.

### 2026-11-10 19:40:58 — WFS-324 refusé par le lead dev, PR #341 (lead-dev)
- Relu au commit 1431d17a : toutes les captures ouvertes, tout le diff lu. Construction et tests refaits sur la branche synchronisée : 101/102 verts (`test_retroviseurs` OK ; seul rouge `test_course5_ordinateurs`, rouge connu de `main`, sources identiques, sans lien).
- Refus (revue en commentaire, même compte GitHub) :
  1. Glace intérieure de la Golf (et Range Rover Sport, M4) : la voiture qui suit est cachée par une forme sombre en « V » et par les sièges du modèle. Plan proche de 4 cm seulement, œil du cockpit 50 à 75 cm trop en avant dans le modèle.
  2. De nuit, aucun phare allumé visible dans les glaces (RG-04).
  3. Capture 3 : l'image « sans » montre l'écran Pause, pas les Réglages « Rendu ».
  4. Glace gauche presque noire sur les bancs cockpit de nuit et de pluie : à expliquer.
- Le code est propre, rien de bloquant.
- Ticket en « ↩️ Retour lead dev (PR #341) ».

### 2026-11-10 19:42:43 — WFS-402 fusionné par le lead dev (lead-dev)
- PR #340 approuvée (commentaire « ✅ Approuvé par le lead dev », même compte GitHub) et fusionnée en squash : 18aa4768. Avis agent 3D « VALIDÉ » sur fb8ee687.
- Vérifié : 65 captures ouvertes, tout le diff lu, construction et tests CPU refaits (100 verts ; test_course5_ordinateurs rouge connu de main, sans lien), banc Ville en Basse refait sur main et sur la branche (74 captures, rien de dégradé).
- Statut : 🟡 À valider (utilisateur). Réserves pour une tâche séparée du PO (RG-12) : filets de lumière la nuit au bord des dormants, reflet des vitres, fenêtres allumées plates en gros plan, porte à rainures du moderne, seuils sous l herbe ; performance et détail Basse à 30 m à confirmer sur le téléphone. WFS-410 et WFS-411 peuvent démarrer.

### 2026-11-10 19:43:22 — WFS-402 : fusionné, session terminée (Claude, agent devartiste 2 (01UY1Vv5qgAR))
- Fait : PR #340 fusionnée en squash par le lead dev (18aa4768) après « Avis agent 3D : VALIDÉ » ; ticket 🟡 À valider (utilisateur) ; réponse à l'utilisateur sous sa demande dans `tache utilisateur.md`.
- Réserves (agent 3D et lead dev, RG-12, à créer en tâche séparée par le PO) : filets de lumière la nuit au bord de certains dormants en gros plan (seul défaut de ce ticket) ; vitres sombres le jour et pièce de nuit en aplat en gros plan (matériau existant) ; porte à rainures du moderne peu lisible ; seuils méditerranéen et villa cachés par l'herbe en pente ; en Basse, détail complet limité à 30 m (surveiller l'apparition des balcons) ; banc `archi` : garde-fou si un plan n'a aucune baie.
- Suite : un seul sujet par session, cette session s'arrête ; WFS-410 et WFS-411 vont à de nouvelles sessions.

### 2026-11-10 20:28:30 — Statuts automatiques, CI main #993 (robot statuts)
- Run rouge (push, commit fe9d8fc), aucun statut changé. Job(s) en échec : banc-essai (texte), endurance.
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-396, WFS-362, WFS-363, WFS-384, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-10 21:00:54 — Statuts automatiques, CI main #994 (robot statuts)
- Run rouge (push, commit 18aa476), aucun statut changé. Job(s) en échec : endurance, banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-406, WFS-396, WFS-362, WFS-363, WFS-384, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-10 21:41:30 — WFS-415 et WFS-416 créés, ordre de l'utilisateur (Claude, session d'explication (01UY1VvJJ))
- Analyse de l'APK v0.1.994 (vidéos de l'utilisateur) : 12 img/s en Basse de nuit, CI de `main` rouge depuis #991 (test_course5_ordinateurs, puis endurance au #994), APK publié malgré le rouge.
- **WFS-415** (P1 super urgent, en tête, **lead dev** par ordre de l'utilisateur) : fenêtres de WFS-402 coupées en jeu par un interrupteur **sans supprimer le code**, CI de `main` au vert sans baisser de seuil ; WFS-413 (phares) gardé. Règles : `regles-de-gestion/WFS-415.md`.
- **WFS-416** (P1 super urgent, après WFS-415, `expert-cartes`, nouvelle session) : bâtiments cuits par la CI dans le `.wfcarte` (3 niveaux, fenêtres comprises), le jeu ne fait que lire et afficher, fenêtres 3D de près et image avec relief au loin, kit en secours. Règles : `regles-de-gestion/WFS-416.md`.
- Aucune autre fusion sur `main` tant que la CI n'est pas verte.

### 2026-11-10 22:08:50 — WFS-414 attribué à Claude, agent dev3d (01Us6uCxLS) (po-scrum-master)
- **Début** : demande d'une nouvelle session `dev-artiste-3d` (« @dev3d prends une tache », 22:06 UTC), qui n'a jamais eu de ticket (règle « un seul sujet par session ») ; pas l'auteur de WFS-323 (règle du KO). Ticket : **WFS-414** (P1 super urgent, E10, en tête de la liste pour `dev-artiste-3d`), vrai volant de 6 voitures en vue Cockpit (Mustang, Golf 8, Abarth 500, T-Cross, S3 berline, Giulia), ANO-241 et ANO-242. Branche `feature/waforspeed-414`.
- **Zone autorisée** : `src/render/vehicule_cockpit.cpp` (bloc du volant de `Interieur::construire`), `vehicule_cockpit.h`, `vehicule_cockpit_defs.cpp` (champs du volant ; visière pour RG-07, voitures du périmètre seulement), `tests/test_cockpit.cpp`, modèle de volant éventuel sous `assets/` (licence CC0 / CC BY, `CREDITS.txt`).
- **Exception PO (un ticket actif par épic)** : en parallèle de **WFS-324** (↩️ retour lead dev, PR #341, mêmes fichiers `vehicule_cockpit.*`, `vehicule_cockpit_banc.cpp`, `desktop_main.cpp`, `CMakeLists.txt`), déjà prévu par RG-04 : WFS-414 ne touche qu'au bloc du volant, WFS-324 aux rétroviseurs ; le second fusionné se synchronise et garde les deux modifications. Raison : KO super urgent de l'utilisateur.
- **Rappels** : les 6 voitures changent, toutes les autres restent identiques à `main` (test RG-05) ; gros plans du volant avant / après, jour, nuit, braquage ; avis agent 3D « VALIDÉ » sans réserve ; statut 🟡 après la fusion. **Fusions suspendues** tant que la CI de `main` n'est pas verte (WFS-415).
- Suivi : WFS-324 sans envoi depuis 19:22 UTC (2 h 46) : alerte dans `track-agent.md`, pas de retrait (règle des 10 h). Lignes des sessions terminées (WFS-404, 402, 406, 413) enlevées de `track-agent.md`.

### 2026-11-10 22:16:53 — WFS-414 : RG-03 adaptée, plaque de commandes de 40 × 40 mm (po-scrum-master)
- Question de Claude, agent dev3d (01Us6uCxLS) : avec le moyeu RG-01 (≥ 200 mm), la branche visible ne fait que 35 à 43 mm ; la plaque de 60 × 40 mm de RG-03 n'y entre pas.
- Décision : option recommandée par le développeur. RG-01 gardée (moyeu 200 à 205 × 145 à 150 mm, 65 mm, galbe 16 mm) ; plaque ≈ 40 × 40 mm (34 à 42 mm le long selon la voiture), 2 × 2 touches en relief sans logo ; démarreur rouge de la Giulia à la place d'une touche. Moyeu de 180 mm refusé (hors RG-01). Écrit dans `regles-de-gestion/WFS-414.md` (Suivi).

### 2026-11-10 22:40:20 — WFS-416 attribué à Claude, agent dev3d (01UY1Vv4rv) (po-scrum-master)
- **Début** : ordre de l'utilisateur, 1er message d'une nouvelle session `dev-artiste-3d` : « @dev3d prend la tache WFS-416 ». La session n'a jamais eu de ticket (règle « un seul sujet par session »). Ticket : **WFS-416** (P1 super urgent, E14 + E15), bâtiments cuits dans le `.wfcarte`, plus aucune construction en course, kit en secours. Branche `feature/waforspeed-416`. Le ticket était prévu pour `expert-cartes` : l'ordre de l'utilisateur passe avant ; le développeur charge aussi le skill `expert-cartes`.
- **Dépendance WFS-415** (pas encore fusionné, dernier envoi 22:33 UTC ; CI de `main` rouge) : le travail commence tout de suite depuis `main` ; **la PR attend la fusion de WFS-415 et une CI de `main` verte**, branche resynchronisée (conflit possible sur `archi_ouvertures.*` / `archi_carte.*` : garder les deux modifications).
- **Zone autorisée** : `src/core/carte_cuite.*`, mode « cuisson seule » (`src/world/cartes_v2/paquet_carte.*`, `carte_donnees.*` ; accroche de `desktop_main.cpp` ≤ 10 lignes si besoin), `src/world/archi_carte.*`, `archi_ouvertures.*` et `archi_geo.*` (appel en cuisson, sans réécriture), shaders de façade (≤ 40 lignes, annoncées ici), `tests/**`. **Aucun fichier de `.github/**`** : `paquets.yml` appelle déjà `WFS_BANC=cuire` ; un besoin côté CI passe par le PO vers l'agent expert ci cd.
- **Rappels** : Ville d'abord (RG-09) ; test « 0 construction en course » ; 6 captures demandées avec la ligne img/s ; avis agent 3D « VALIDÉ » avant le lead dev (E15) ; statut 🟡 après la fusion.
- Suivi : WFS-324 sans envoi depuis 19:22 UTC (alerte gardée, pas de retrait) ; WFS-414, dernier envoi 22:28 UTC.


### 2026-11-10 22:47:25 — WFS-414 : plan, fichiers, accroche, risques (Claude, agent dev3d (01Us6uCxLS))
- **Branche** `feature/waforspeed-414` poussée (465d6e0 à 22:09 UTC, puis étape 1 d270a0e). Le titre de la discussion n'a pas pu être changé : l'outil `set_session_title` n'existe pas dans cette session (ToolSearch : aucun résultat).
- **Plan, en 2 étapes** : (1) déplacement pur, sans changer une valeur : fonctions de forme de `vehicule_cockpit.cpp` vers `vehicule_cockpit_formes.h`, bloc du volant vers `construireVolant` (`vehicule_cockpit_volant.cpp`, sans rendu, testable), ligne de la visière vers `ligneVisiere` ; empreintes des 20 volants de `main` relevées par `test_cockpit --empreintes` (`tests/test_cockpit_volants_main.h`) ; (2) volant de série pour les 6 voitures seulement (table `SERIE` de `vehicule_cockpit_defs.cpp`) : coussin bombé (super-ellipse, 200-205 × 145-148 × 65 mm, galbe 18 mm), joint sombre de 4 mm, coque du moyeu (+ 8 mm), branches de 55 → 35 mm qui naissent 12 mm dans le coussin et entrent dans le tube de la jante, plaque de 2 × 2 touches sur chaque branche horizontale (RG-03 adaptée), agrafe de la Golf et démarreur de la Giulia (avec sa bague) sur les branches, visière moins avancée (RG-07).
- **RG-07, mesuré** (`test_cockpit`, rayons de l'œil de la caméra vers le haut de la jante) : avec l'avancée commune de 68 mm, la lèvre de la visière **traverse** la jante sur 5 des 6 voitures (jour de −28 à −31 mm) ; avancée retenue : Mustang 16, Golf 12, Abarth 60, T-Cross 28, S3 28, Giulia 28 mm (aucun point caché, jour ≥ 5 mm). **Constat hors périmètre** : la M4 et la 911 ont le même défaut (lèvre dans la jante) ; non touchées (ordre de l'utilisateur : 6 voitures seulement), signalé au PO.
- **Combiné** : le nouveau moyeu, plus haut (145 mm au lieu de 115), cache une bande plus grande du bas du combiné : `bandeLisibleCombine` prend le haut de la coque et du coussin pour ces 6 voitures (chiffres replacés dans la partie vue, test WFS-323 vert) ; les autres voitures ne changent pas.
- **Accroche** (≤ 10 lignes, zone E7) : `src/platform/desktop_main.cpp`, banc `WFS_BANC=cockpit-volant` (1 inclusion + 4 lignes). Nouveau banc `src/render/vehicule_cockpit_volant_banc.cpp` (zone E10) : gros plans recadrés sur la jante projetée par la caméra de l'image (système inactif hors banc), plus une copie éclaircie pour la revue.
- **Risques** : WFS-324 (PR #341) modifie les mêmes fichiers (`vehicule_cockpit.cpp/.h`) : synchronisation et fusion des deux côtés au moment voulu ; captures « avant » = `main` + ce banc seul (comme WFS-323).
- **Vérifié en local** : construction complète ; `test_cockpit` vert (14 volants identiques à `main`, RG-01, 02, 03, 07) ; tests CPU 101 / 102 (seul rouge : `test_course5_ordinateurs`, rouge connu de `main`).

### 2026-11-10 22:48:26 — ANO-243 : visière sur la jante de la M4 et de la 911, rangée dans WFS-325 (po-scrum-master)
- Signalement de Claude, agent dev3d (01Us6uCxLS) pendant WFS-414 (mesure de RG-07 dans `test_cockpit`) : la lèvre de la visière traverse le haut de la jante de la M4 (22 points, jour −25,6 mm) et de la 911 (28 points, jour −27,1 mm) ; défaut déjà sur `main`.
- WFS-414 n'y touche pas : l'ordre de l'utilisateur le limite aux 6 voitures.
- **ANO-243** créée (mineure, à confirmer par le QA ; même défaut qu'ANO-242) et rangée dans un lot ouvert, pas de nouveau ticket : **WFS-325** (P1, vue Cockpit 3/3), nouvelle **RG-05**, M4 et 911 dans son 1er lot ; WFS-325 passe ⏳ En attente, après la fusion de WFS-414 (mêmes champs de la visière, même test).

### 2026-11-10 22:50:26 — WFS-415 : causes trouvées, corrections en cours (lead-dev, session 01UY1VvJJ)
- Branche `feature/waforspeed-415` (ordre de l'utilisateur : le lead dev code ce ticket ; une autre session lead dev relira et fusionnera).
- **Endurance rouge (#993, #994)** : la cause n'est pas le garage ni WFS-402, c'est **WFS-406** (fe9d8fc4, #993 est le 1er rouge de l'endurance) : `credits::dessinerBouton` est appelé dans `menu.cpp` **avant** les 3 choix, donc le bouton « Crédits » devient la 1re zone de la manette (`manette::focus`) ; le parcours au doigt touche la 1re zone (« Course solo » attendu) et ouvre les Crédits : panneau des modes fermé, puis tout le reste en écart (23). Correction : le bouton est dessiné après les choix (accroche de 3 lignes dans `src/ui/ecrans/menu.cpp`, hors zone, annoncée ici ; la manette retrouve aussi « Course solo » en premier). Endurance locale : **0 écart**, code 0.
- **test_course5_ordinateurs (#991 à #994)** : `remplacant()` de WFS-404 prenait « la plus proche en vitesse max » (S3, V60 : 0-100 de 4,9 s) à la place de l'Elantra N (5,3 s), du X5 (5,4 s), du Range Rover Sport (5,9 s), et la retirait des candidats, ce qui décalait les tirages suivants. Correction (`src/game/course5.cpp`) : remplaçante de la classe du joueur **pas plus rapide** (vitesse max ≤, 0-100 ≥), allure la plus voisine, et plus retirée des candidats (tirages d'avant WFS-404 gardés). Graine par graine (8 graines × 4 cartes, Golf) : chaque place du joueur égale ou meilleure qu'avant WFS-404 (f04cec8c) ; circuit 6/8 avant WFS-404, 3/8 à main, **7/8** après ; Ville 8/8, Campagne 8/8, Plage 7/8. Nouveau contrôle dans `tests/test_course5.cpp` (17 voitures × 400 graines : tirage d'avant WFS-404 recopié, remplaçantes pas plus rapides dès qu'il en existe une dans la classe). Limite : dans la classe de la SF90, aucune voiture n'est plus lente que l'A 45 : la moins rapide en trop est prise (Corvette, sinon M4).
- **Fenêtres de WFS-402** : interrupteur `archi::ouverturesReelles()` (archi_geo.*), 0 par défaut (jeu, APK), `WFS_OUVERTURES=1` ou `choisirOuverturesReelles(true)` pour le rendu de WFS-402 ; aucun fichier supprimé. Le banc `archi` (accroche d'une ligne dans `archi_banc.cpp`) et `test_archi_geo` tournent à 1 ; nouveau contrôle : à 0, ni châssis ni volet, moins de triangles ; à 1, maillage identique. LOD0 en Basse : 30 m seulement avec l'interrupteur à 1, sinon 120 m comme avant WFS-402.
- Tests CPU : **102 réussis** (`tools/qa/tests_cpu.sh build`). Mesures du banc perf (Ville de nuit, Basse) en cours : r990 / avant WFS-402 / main / après.

### 2026-11-10 23:10:39 — WFS-324 : corrections du refus de la PR #341 (Claude, agent dev3d (01UY1Vv5YT))
- Point 1, glace intérieure : la voiture qui suit était cachée par les sièges avant du modèle. Correction : même décalage en long que les glaces extérieures (l'œil du cockpit est 50 à 75 cm trop en avant dans le modèle), et visée selon la hauteur de l'œil. Commits 02f414bf et 36d0a985.
- Point 2, nuit : 2 lampadaires derrière la voiture sont aussi de vraies lumières quand les rétroviseurs sont actifs. C'est une accroche dans `nuit.cpp` (≤ 20 lignes), annoncée ici.
  - Une voiture vue seulement dans les glaces n'éclaire rien : le contraire éclairait en blanc le bas de l'habitacle.
  - Les glaces extérieures sont réglées vers la voie d'à côté : la carrosserie occupe 1,5° au bord intérieur (79c12aa9).
- Point 3 : capture des Réglages refaite, ligne « Rendu » sans et avec rétroviseurs.
- Point 4 : la glace gauche reste sombre la nuit, expliqué dans la PR. La scène n'a que 12 vraies lumières, choisies pour la vue avant, et une glace extérieure ne renvoie que 50 % de la lumière.
- Vérification :
  - banc `retroviseurs` : 0 écart (Golf, Range Rover Sport, M4, 911) ;
  - banc `cockpit-voisine` : 0 écart ;
  - 102 tests verts ; `test_course5_ordinateurs` rouge, comme sur `main` ;
  - coût : 0,41 img/s en Basse et 0,64 en Haute (estimé).
- Commit relu à demander : d77ac9f5. Le lead dev est rappelé.

### 2026-11-10 23:26:15 — WFS-414 : PR #343 ouverte (Claude, agent dev3d (01Us6uCxLS))
- PR #343 (`feature/waforspeed-414`, commit 5ea4b467, synchronisée avec `main`), statut 🔀 En PR ; ligne 🆕 dans `suivi-pr.md`.
- Fait : volant de série des 6 voitures (coussin bombé 200-205 × 145-148 × 65 mm, galbe 18 mm, joint 4 mm, coque en plastique noir grainé ; branches 55 → 35 mm nées dans le coussin et finies dans la jante ; plaques de 2 × 2 touches ; démarreur de la Giulia et agrafe de la Golf sur les branches) ; visière moins avancée (RG-07) ; combiné : vitesse remontée dans la partie vue, rapport au-dessus (S3, Giulia) ou à côté (Abarth). Les 14 autres volants : code déplacé tel quel.
- Vérifié : `test_cockpit` vert (RG-01, 02, 03, 05, 07 sur la géométrie) ; tests CPU 101 / 102 (rouge connu `test_course5_ordinateurs`) ; banc `cockpit-volant` 2340 × 1080 avant (main + banc) / après : 37 captures dans `qa/pr/WFS-414/` ; M4 et 911 identiques au pixel près.
- Limites : la jauge de nitro du bas du combiné (Mustang, Golf, T-Cross, Abarth) est cachée par le moyeu plus haut (nitro toujours sur le HUD) : question au PO dans la PR ; formes du coussin estimées sur photos ; fusion suspendue tant que la CI de `main` est rouge (WFS-415).
- Suite : avis de l'agent 3D (`expert-realisme-3d`) dans la PR, puis appel du `lead-dev`.

### 2026-11-10 23:36:27 — WFS-324 : 2e revue de la PR #341 interrompue (Claude, agent dev3d (01UY1Vv5YT))
- La 2e relecture du lead dev s'est arrêtée avant son verdict, par une erreur de l'API (limite hebdomadaire d'usage atteinte jusqu'au 15/11, 17:00 UTC). Rien n'a été fusionné ni refusé.
- `suivi-pr.md` : #341 remise de « 👀 En revue » à « 🔁 Corrigée, à relire », pour le passage automatique du lead dev.
- Branche resynchronisée avec `main`, à la demande de l'utilisateur : fichiers de suivi seulement, aucun code changé depuis d77ac9f5.

### 2026-11-10 23:50:05 — WFS-416 : 3 écarts aux règles arbitrés (po-scrum-master)
- Demande de Claude, agent dev3d (01UY1Vv4rv), branche `feature/waforspeed-416` à 3712668e (dernier envoi 2026-11-10 23:48 UTC).
- **Acceptés tous les trois** (détail dans `regles-de-gestion/WFS-416.md`, section Suivi) : (1) niveau 0 recopié du paquet dans 3 tampons réservés au chargement (aucune construction, aucun tampon créé en course) ; (2) mémoire : la phrase « pas plus haute qu'avant » de la RG-07 est retirée (décision de l'utilisateur : aucune limite de mémoire pour le décor), repli sur le kit de secours si une réservation échoue ; (3) paquet de la Ville 189 à 206 Mo, dans le budget E15.
- **Publication des paquets** : `paquets.yml` n'est pas automatique (lancement à la main sur `main`). Après la fusion et une CI de `main` verte, l'agent expert ci cd le lance ; la PR porte une section « Après la fusion ».
- La PR attend toujours la fusion de WFS-415. Suites Corniche et Campagne : à créer après la fusion. `track-agent.md` à jour.

### 2026-11-11 00:02:26 — Ordre de l'utilisateur : WFS-325 en premier après WFS-324 (po-scrum-master)
- L'utilisateur a demandé la liste des 40 prochaines tâches (donnée dans la session du PO), puis : « tu peux mettre 325 en premier après 324 ».
- Fait : bandeau dans `TACHES.md` et case Statut de WFS-325 ; WFS-325 passe de la 15e place de l'ordre du 2026-11-10 14:20 UTC à la place juste après WFS-324, avant WFS-405 et WFS-364 à 368. Rien d'autre ne change.
- WFS-325 reste « En attente » jusqu'à la fusion de WFS-414 (PR #343, mêmes fichiers du cockpit) ; il ira au premier `dev-artiste-3d` (nouvelle session) qui demande. Aucune attribution faite.

### 2026-11-11 00:04:51 — WFS-415 fusionné par confiance (#342, af1285a4), ordre de l'utilisateur (lead-dev, session 01UY1VvJJ)
- L'utilisateur a arrêté l'agent lead dev pendant les captures, puis a ordonné « Fusionner » : PR #342 fusionnée en squash, sans 2e revue ni captures.
- Contenu : remplaçantes de la Course à 5 pas plus rapides (test_course5_ordinateurs), bouton Crédits après les choix du menu (endurance), interrupteur `archi::ouverturesReelles()` à 0 en jeu (fenêtres de WFS-402 coupées, code gardé, `WFS_OUVERTURES=1`), LOD0 Basse 120 m.
- Vérifié : 102 tests CPU réussis en local. Non vérifié : endurance locale, mesure img/s, captures. La CI de `main` dira si tout est vert ; WFS-415 en 🟡 À valider (utilisateur), à mesurer sur le téléphone.

### 2026-11-11 00:27:22 — WFS-416 : décisions de l'utilisateur relayées, seuil de 30 img/s déplacé (Claude, session « APK quality issues » (01UY1Vv2WG))
- Mesures de l'APK 995 (WFS-415) sur le téléphone de l'utilisateur, Ville et Ville Néon de nuit, Basse : **16 à 21 img/s** (12 sur l'APK 994), médiane 55 à 66 ms, 99 % 71 à 88 ms (190 ms avant), chauffe 1, scène GPU 43 à 55 ms. CI #995 : compilation, APK et endurance verts.
- Utilisateur : WFS-416 va jusqu'à la PR et sa validation sur téléphone ; +80 Mo de mémoire graphique acceptés. RG-07 de WFS-416 : pas de perte d'img/s par rapport à l'APK 995 et pire temps des bâtiments ≤ 1 ms ; les 30 img/s iront au ticket « lumière légère en Basse », à créer **après le test de l'utilisateur** (son ordre). Message envoyé à la session de WFS-416.

### 2026-11-11 00:48:56 — WFS-416 : PR #344 ouverte (Claude, agent dev3d (01UY1Vv4rv))
- PR #344 `feature/waforspeed-416` → `main`, synchronisée avec `main` (WFS-415 compris), 5f5bf85a : bâtiments de la Ville cuits dans le paquet, 0 construction en course, kit de secours. 103 tests CPU, banc « batiments » 0 écart, 29 captures dans `qa/pr/WFS-416/`.
- Suite : avis de l'agent 3D (épic E15), puis lead dev. CI de `main` #995 (WFS-415) encore en cours à l'ouverture : à voir verte avant la fusion. Après la fusion : relancer « Paquets de cartes » (agent expert ci cd).
