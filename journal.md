# WaForSpeed Mobile — journal

Journal du projet (tickets WFS-XX de `TACHES.md`) : une entrée par étape (début, construction, PR, verdict QA, fin…) avec
fait / fichiers / vérification / limites. **Nouvelles entrées à la fin du fichier.** Toujours partir de la dernière version de
`main` (`git pull`), ajouter seulement ses entrées, puis envoyer tout de suite ; ne jamais écraser le fichier.
**Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
Exemple : `### 2026-11-03 19:43:10 — WFS-58 construit, essai simulateur lancé (Claude, session trafic)`.
> **Archive (règle de l'utilisateur, 2026-11-06)** : ce fichier ne garde que les **2 derniers jours**. Les entrées plus anciennes sont dans `journal/archive/AAAA-MM-JJ.md` (un fichier par jour) ; ne les relire qu'en cas de besoin précis. Le PO déplace chaque jour les entrées de plus de 2 jours.

### 2026-11-18 10:37:34 — Statuts automatiques, bancs du matin #50 (robot statuts)
- Run rouge (schedule, commit 0d13d2e), aucun statut changé. Job(s) en échec : endurance et chauffe, banc (realisme-2), banc (complements-c).
- Tickets 🟣 contenus dans ce run : WFS-555, WFS-564, WFS-496, WFS-503.

### 2026-11-18 11:29:49 — Archives automatiques (robot archives)
- Journal : 74 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-16).
- TACHES.md : 0 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md`.

### 2026-11-18 19:09:56 — ANO-314 : écran « Choisis ton mode » noir et sans images (Agent qa (session_01UY1Vv3tQoXdU3HwAjakb9t))

- Retour de l’utilisateur (téléphone) : arrière-plan noir, aucun visuel. Comparé à la maquette 02 : les cartes doivent montrer une image de course chacune. Cause : solution provisoire de WFS-557 (fond flouté assombri à 62 %). ANO-314 (Majeure) ajoutée, correction = **WFS-562** (P1, débloquée par la fusion de WFS-557) : le PO peut l’attribuer à une nouvelle session `@dev` et ajouter le critère « fond pas noir sur le téléphone ».

### 2026-11-18 19:25:46 — WFS-562 réécrit : des images qui disent le mode (po-scrum-master)

- Demande de l'utilisateur (transmise par l'Agent qa) : « Même la maquette ça va pas il faut mettre des photo parlant genre checkpoint voiture avec Check point poursuite voiture avec police ». Les images de la maquette 02 sont refusées ; la mise en page des cartes est gardée.
- `regles-de-gestion/WFS-562.md` : RG-01 Checkpoint = voiture sous un portique de checkpoint ; Poursuite = police aux gyrophares derrière, soir ; Course à 5 = voiture au milieu des concurrents (choix du PO). RG-05 bis (arbitrage) : on ne photographie pas le portique actuel en formes nues (ANO-279) ; le ticket le refait d'abord en vrai modèle (exception de zone pour ce ticket). RG-07 : fond pas noir sur le téléphone (ANO-314). Captures et critères réécrits.
- `TACHES.md` : WFS-562 ⏳ Prêt (WFS-557 fusionné), P1, pour une nouvelle session `@dev` + avis `@3d`. ANO-279 📋 Planifiée (partie checkpoint) dans WFS-562. Réponse dans `tache utilisateur.md`.

### 2026-11-18 19:27:36 — ANO-315 : checkpoints et bandeaux cyan refusés par l’utilisateur (Agent qa (session_01UY1Vv3tQoXdU3HwAjakb9t))

- Demande de l’utilisateur : « il faut évoluer les checkpoints et les interfaces, texte avec la couleur bleue, c’est n’importe quoi ». Vérifié sur les captures du run #46 : portique néon cyan, ligne cyan sur la route, bandeaux « CHECKPOINT N +30 s » en aplat cyan (0x00E5FF). ANO-315 (Majeure) ajoutée avec preuve ; demande transmise au PO pour le ticket.

### 2026-11-18 19:31:21 — ANO-315 étendue à tous les messages de course (Agent qa (session_01UY1Vv3tQoXdU3HwAjakb9t))

- Précision de l’utilisateur : il parle des textes **dans le jeu**. Planche des messages de course (`qa/rapports/VABF-2026-11-17/preuves/ANO-315-messages-en-course.jpg`) : bandeaux cyan / orange / rouge empilés sur la route, « CAMION EN TRAVERS » rouge à gauche, conseil de départ caché. ANO-315 mise à jour, PO prévenu.

### 2026-11-18 19:25:57 — Avis juridique : peut-on publier sur le Play Store ? (juriste)

- Rapport `juridique/rapport-2026-11-18.md` (mise à jour de J-01 à J-17, nouveaux J-18 à J-20). Test interne : oui. Sortie publique gratuite : pas encore, 4 points 🔴 (J-02 fiche, J-04 confidentialité et contact, J-05 Sécurité des données, J-18 boutique payante déjà dans l'APK alors que les CGU disent « sans achat »). Réglés : J-01, J-03, J-06, J-07, J-13 (WFS-404, 406).
- Au PO : proposer une US « Market caché dans la version publique tant que WFS-516 n'est pas fini » (J-18) ; WFS-407 à 409 restent à faire le jour de publication.
- (Report sur `main` par le PO : la session du juriste n'avait pas pu pousser, HTTP 403 ; rapport et entrée copiés de la branche `claude/nice-faraday-3j03xj`, commit 47e1e54.)

### 2026-11-18 19:31:09 — Avis du juriste du 2026-11-18 traité : WFS-566 créé, WFS-407 à 409 et 516 complétés (po-scrum-master)

- **WFS-566** (P1, Publication app, ⏸ En attente de la décision de l'utilisateur, question 1, et du jour de publication) : Market caché dans la version publique tant que WFS-516 n'est pas fini (J-18), même réglage de construction que WFS-405 ; APK d'aperçu inchangé ; rien de supprimé. Règles : `regles-de-gestion/WFS-566.md`. Annulé si l'utilisateur choisit « avec achats ».
- RG complétées : WFS-407 (J-02 : 17 voitures ; J-20 : fiche anglaise avec WFS-509), WFS-408 (J-20 : CGU et confidentialité EN ; achats / remboursements / rétractation si sortie avec achats), WFS-409 (J-05 variante selon la question 1 ; J-15 test fermé à vérifier), WFS-516 (renvoi à WFS-566).
- Réglés selon le juriste : J-01, J-07 (WFS-404), J-03, J-06, J-13 (WFS-406). J-19 (pub pas encore dans l'APK) : déjà couvert par WFS-512 / 516. 🟠 inchangés : J-08, J-09, J-10.
- 5 questions ouvertes ajoutées à `tache utilisateur.md` (Market, éditeur et contact, adresse de la confidentialité, 13+, antériorité du nom), sans les trancher. La sortie publique reste reportée (ordre du 2026-11-08 16:59 UTC).

### 2026-11-18 19:35:39 — Décision de l'utilisateur : 1re sortie publique avec achats ; WFS-566 annulé (po-scrum-master)

- Réponse à la question 1 du juriste (2026-11-18 ~19:45 UTC, transmise par le coordinateur) : « La première version est pas gratuit ya des achat ». La 1re sortie publique se fait **avec achats intégrés** (Market E21).
- **WFS-566** → ❌ Annulé (décision de l'utilisateur), RG mise à jour. `tache utilisateur.md` : question 1 marquée répondue ; questions 2 à 5 ouvertes.
- **WFS-408** (RG-05, 06) et **WFS-409** (RG-04) en variante « avec achats » : CGU et confidentialité FR / EN (achats, prix TTC, remboursements, rétractation avec renonciation expresse, mineurs) ; Sécurité des données et IARC avec achats intégrés ; pub ajoutée quand WFS-512 arrive.
- Sortie publique : dépend aussi de WFS-514, 515, 516, de WFS-512 si la pub est dans la 1re version, du compte marchand Google Play, du statut « commerçant » UE (adresse et téléphone sur la fiche) et d'un avis d'avocat (RC-E21-12). Non tranché : pub dans la 1re version, éditeur, contact. Sortie toujours reportée (ordre du 2026-11-08).

### 2026-11-18 19:36:28 — WFS-408 élargi et attribué à la session juriste sur ordre de l'utilisateur (po-scrum-master)

- Ordre de l'utilisateur à la session juriste (~19:50 UTC), mot pour mot : « Tu doit corriger les autres anomalies ». Décision du PO : **WFS-408 élargi** (un seul ticket) et attribué à Claude, agent juriste (01UY1Vv2) (session_01UY1Vv2qVRDXE9sA9mNENZD), qui devient développeur pour ce seul ticket. Exceptions justifiées par l'ordre direct de l'utilisateur : le juriste code ce ticket, et WFS-408 sort de l'attente du jour de publication (rien n'est publié ; sortie toujours reportée).
- Contenu (RG-E1 à E5) : CGU et confidentialité FR + EN et texte du jeu (`cgu_texte.cpp`) « avec achats » (prix TTC, remboursements, rétractation avec renonciation expresse, mineurs, duel, droit applicable, données réelles) ; repères uniques pour éditeur / contact / adresse / pub, rien d'inventé ; `store/fiche-fr.md` (17 voitures inspirées de la route, achats intégrés) ; préparation écrite Sécurité des données + IARC + public cible.
- Zone : `store/**`, `src/ui/ecrans/cgu_texte.cpp`. Branche `feature/waforspeed-408`. WFS-407 garde les captures neuves, WFS-409 la saisie dans la Console. Rappel : renommer la session « WFS-408 — juriste (documents avec achats) », captures, PR, `lead-dev`, arrêt après la fusion.

### 2026-11-18 19:37:28 — 1re version publique = achats + pub ; E21 monté en priorité (po-scrum-master)

- Ordre de l'utilisateur (~19:55 UTC), mot pour mot : « Dans la première version je veux gagner l'argent je veux tout en place pour gagner l'argent ». Question « pub oui / non » répondue (oui) dans `tache utilisateur.md`.
- Priorité : bandeau 💶 en tête de `TACHES.md` : WFS-512 → 514 → 515 → 516 donnés avant les autres tickets prêts, sans retirer les P1 en cours ; **WFS-516 passé P2 → P1**.
- WFS-408 (élargi, session juriste, `feature/waforspeed-408`), 409, 516 : variante « achats + pub » (consentement RGPD aux traceurs, identifiant publicitaire, « Contient des annonces », IARC refait).


### 2026-11-18 19:39:46 — ANO-316 : enseignes néon bleues du décor (Agent qa (session_01UY1Vv3tQoXdU3HwAjakb9t))

- Demande de l’utilisateur (planche validée par lui) : enlever ou changer les enseignes à texte bleu du décor (« GARAGE 88 », « GALERIE DES ARCADES », « NUIT BLANCHE », cyan 0x00E5FF dans `src/world/enseignes.cpp` et `src/world/ville.cpp`). ANO-316 (Majeure) ajoutée avec preuve `qa/rapports/VABF-2026-11-17/preuves/ANO-316-enseignes-bleues-decor.jpg` ; le PO crée l’US de correction.

### 2026-11-18 19:43:03 — ANO-315 et ANO-316 : WFS-567, 568, 569 créés, WFS-562 RG-05 ter (po-scrum-master)

- Demande de l'utilisateur transmise par l'Agent qa (session_01UY1Vv3tQoXdU3HwAjakb9t) : « il faut évoluer les checkpoint et les interface de merde text avec le couleur blue de merde c'est de n'importe quoi » (« je parle dans le jeu ») ; enseignes bleues du décor : « les enlever ou changer ».
- **WFS-567** (P1, E8, ⏳ Prêt, `@dev`) : messages de course (`src/ui/toasts.cpp`, HUD) sans cyan, texte clair sur fond sombre, en haut, hors de la route, 2 au plus, regroupés ; « CAMION EN TRAVERS » et conseil de départ (ANO-290) compris. Menus inchangés.
- **WFS-562 RG-05 ter** : le portique de checkpoint déjà refait par WFS-562 n'a plus de néon ni de bande cyan au sol (pas de doublon avec WFS-567, qui ne touche pas `portique.cpp`).
- **WFS-568** (P2, E15, ⏳ Prêt, `@dev` dev-artiste-3d + avis `@3d`) : enseignes du décor changées en vraies enseignes (couleur propre à chaque commerce, sans cyan, aucune forme nue) ; rien retiré du code. **WFS-569** (P1, `@cicd`, ⏳ En attente de 568) : publication des paquets (ordre permanent du 2026-11-12), annulé si rien de cuit ne change.
- `anno.md` : ANO-315 et ANO-316 → 📋 Planifiée ; ANO-290 → US WFS-567. Tous 🟡 par l'utilisateur après fusion. Aucune attribution (pas de demande de développeur).

### 2026-11-18 20:35:11 — WFS-408 : PR #434 ouverte (Claude, agent juriste (01UY1Vv2))

- Fait : CGU et confidentialité FR / EN en version « achats + pub » (achats Google Play, prix TTC, renonciation à la rétractation, remboursements, mineurs, AdMob et consentement, duel, droit applicable, bases légales, destinataires, transferts, droits, CNIL) ; `store/fiche-fr.md` (17 voitures, Corniche seule, achats et annonces) ; nouveau `store/play-console-declarations.md` ; `store/checklist.md`. Aucun code (l'écran des CGU lit `store/`).
- Vérifié : `test_cgu` OK, `verifier_licences.py` 0 manque, captures bancs `cgu` et `textes` (FR / EN) avant / après (`qa/pr/WFS-408/`).
- Limites, signalées au PO : renonciation expresse à faire confirmer dans l'écran Market (code hors zone) ; `VERSION_CGU` à passer à 2 le jour de la publication (`cgu.h` hors zone) ; la pub est décrite avant WFS-512 / 514 ; `wss://` du relais à vérifier. Repères « À REMPLIR PAR L'UTILISATEUR » (éditeur, contact, adresse) visibles dans le jeu jusqu'aux réponses de l'utilisateur.

### 2026-11-18 20:35:38 — Points du juriste (PR #434) rangés : WFS-567 créé, RG de 319, 409, 514, 516 (po-scrum-master)

- **WFS-567** (P1, E21, ⏳ Prêt, nouvelle session `@dev`) : renonciation expresse au droit de rétractation avant chaque achat sur l'écran Market (L221-28 13°), case active + lien CGU, FR / EN ; préalable à la vente (WFS-516) ; ajouté au bandeau 💶 avec WFS-512.
- WFS-319 : `VERSION_CGU` 1 → 2 et « Version 2 » des CGU le jour de la publication. WFS-514 : « Confidentialité des pubs » / « Ad privacy ». WFS-409 : vérifier le chiffrement du duel (relais en TLS vu dans `reseau_android.cpp` ; PeerJS à confirmer).

### 2026-11-18 20:36:37 — Doublon WFS-567 corrigé : la renonciation expresse devient WFS-570 (po-scrum-master)

- Erreur du PO : à 20:35:38 UTC, la renonciation expresse avant achat a reçu le numéro **WFS-567**, déjà pris à 19:43:03 UTC par la correction ANO-315 (messages de course), et `regles-de-gestion/WFS-567.md` a été écrasé.
- Correction : `WFS-567.md` restauré depuis 539399fc (ANO-315, cité dans `anno.md`) ; la renonciation expresse devient **WFS-570** (`regles-de-gestion/WFS-570.md`, `feature/waforspeed-570`) dans `TACHES.md`, le bandeau 💶, la dépendance de WFS-516 et `track-agent.md`. L'entrée du journal de 20:35:38 parle de WFS-570 sous l'ancien numéro.


### 2026-11-18 20:45:53 — US des anomalies de la VABF du 2026-11-17 et ordre « 562, 567, 568 urgents » (po-scrum-master)

- Ordres de l'utilisateur transmis par l'Agent qa : « La priorité c'est les anno dans le gameplay : décor, conduite, interface » (des US, pas seulement des anomalies) ; « 562, 567, 568 sont urgents » (566 corrigé en 562 par l'utilisateur).
- Nouvelles US (règles dans `regles-de-gestion/WFS-571.md` à `WFS-589.md`) : décor P1 **571** (ANO-280), **573** (ANO-281 + 297 plage), **575** (ANO-284, 285, 286) ; décor P2 **576** (294), **577** (295), **579** (299 Ville), **581** (300), **582** (296), **583** (301) ; conduite P1 **584** (288, 289, `@conduite`) ; interface **585** (287), **586** (291), **587** responsive (302, 303, 304, 307), **588** P3 (305, 306) ; technique P1 **589** (277). Paquets `@cicd` : **572, 574, 578, 580** (après 571, 573, 577, 579 ; ordre permanent du 2026-11-12).
- Tickets complétés au lieu d'un doublon : WFS-563 RG-08 (ANO-278), WFS-554 RG-10 (ANO-292), WFS-444 RG-07 (ANO-293), WFS-454 RG-05 (ANO-276).
- Pas d'US : ANO-282 (Campagne) et ANO-283 (Grand Prix), parties Campagne d'ANO-297 et circuit d'ANO-299 : ⏸ en pause (ordre de l'utilisateur du 2026-11-13).
- Arbitrage CI : la règle « CI de `main` rouge → P1 » est gardée (589, 454), mais ces tickets vont à des sessions `@cicd`, en parallèle : ils ne passent devant aucun ticket de jeu.
- Ordre `@dev` (bandeau 🎮 de `TACHES.md`) : 562 → 567 → 568 (P2 → P1) → 571 → 573 → 575 → 563 → E21 (512, 570) → P2. `anno.md` : 29 anomalies en 📋 Planifiée ou ⏸ avec leur US.

### 2026-11-18 20:48:50 — WFS-408 : PR #434 refusée par le lead dev (lead-dev)
- Commit relu 67afeeed ; 17 paires regardées, texte store/ lu en entier, fond juste ; construit et testé (154 tests CPU OK, test_cgu OK, licences 0 manque).
- Refus : (1) capture « Tests CPU » et `qa/pr/WFS-408/tests-cpu.log` absentes (règle du 2026-11-11) ; (2) captures manquantes des parties duel (CGU), responsable / données qui quittent le téléphone / bases légales (confidentialité) et achats EN. Remarques au PO : `VERSION_CGU` à passer à 2 avant publication ; pub décrite avant WFS-512 / 514.

### 2026-11-18 20:58:48 — WFS-408 : PR #434 corrigée après le retour du lead dev (Claude, agent juriste (01UY1Vv2))

- Tests CPU : 154 réussis, 0 échec (`qa/pr/WFS-408/tests-cpu.log`) ; 10 paires de captures ciblées FR / EN (achats et rétractation, duel, responsable, données qui quittent le téléphone, bases légales), défilement ajouté localement au banc `textes`, non commité ; description et pied de page corrigés ; réponse point par point dans la PR. Commit 29a43444.
- Incident : l'agent lead dev avait laissé le dépôt local sur `main` ; 1re série de captures refaite sur la bonne branche.

### 2026-11-18 21:00:59 — WFS-408 fusionné par le lead dev (lead-dev)

- PR #434, 2e revue sur 29a43444 : tests CPU (154 réussis) et 10 paires de captures ciblées FR / EN regardées ; `store/` et `src/` inchangés depuis la 1re revue (construite et testée par le lead dev). « ✅ Approuvé » en commentaire (même compte GitHub), fusion en squash 7adcbb0d. Statut 🟣, `qa/Check.md` à jour, ligne retirée de `suivi-pr.md`.

### 2026-11-18 21:01:19 — WFS-408 fusionné (#434), fin de session (Claude, agent juriste (01UY1Vv2))

- PR #434 fusionnée en squash par le lead dev (7adcbb0d). Session terminée (un seul sujet par session). Restent pour la vente : WFS-570 (renonciation expresse), WFS-512 / 514 (pub), WFS-319 (`VERSION_CGU` 2), WFS-409, WFS-516 ; valeurs à remplir par l'utilisateur (éditeur, contact, adresse de la politique).

### 2026-11-18 21:07:57 — Questions 2 à 5 du juriste en attente de l'utilisateur (po-scrum-master)
L'utilisateur prépare ses réponses (éditeur et e-mail, adresse de la politique de confidentialité, public cible, recherche d'antériorité « WaForSpeed ») et reviendra vers le juriste avant de livrer. Le PO ne relance plus ; quand l'utilisateur annonce ses réponses, le PO rappelle l'agent `juriste` (avant WFS-516 et la sortie publique).

### 2026-11-18 23:06:37 — WFS-562 attribué à Claude, agent dev3d (01UY1Vv3kc) (po-scrum-master)
Nouvelle session `@dev` (session_01UY1Vv3kcVfmbaBKf14LPpJ), aucun ticket avant : **WFS-562** (P1, E23), 1er de l'ordre de l'utilisateur (562 → 567 → 568). Zone : écran « Choisis ton mode » (images des 3 modes) et portique de checkpoint 3D (vrai modèle, sans néon ni bande cyan, RG-05 ter). Règles : `regles-de-gestion/WFS-562.md`. Avis « VALIDÉ » de l'agent 3D (`@3d`) en commentaire de PR avant fusion, captures ciblées jour et nuit, fusion par le lead dev, 🟡 à valider par l'utilisateur après fusion, puis arrêt de la session. `track-agent.md` : lignes des sessions terminées enlevées.

### 2026-11-19 00:33:51 — WFS-562 : PR #435 ouverte (Claude, agent dev3d (01UY1Vv3kc))
- Fait : portique de checkpoint refait en vrai modèle (treillis 290 mm du portique de départ réutilisé, 2 poutres, bâche PVC imprimée « CHECKPOINT N » noir sur orange, œillets, tendeurs, 2 projecteurs LED la nuit, ligne de chronométrage peinte en blanc) : plus aucun cyan ni néon (RG-05 bis, RG-05 ter) ; 3 photos des modes faites dans le jeu par le banc `modes-photos` (`assets/ui/modes/*.jpg`, Ville, Golf du joueur : sous le portique / au milieu de 4 concurrents / poursuivie par la police aux gyrophares de nuit) et affichées dans les cartes de « Choisis ton mode » ; voile du fond de 0,62 à 0,22.
- Mesure RG-07 : luminosité moyenne du fond hors cartes, capture 2400 × 1080 : **25,7 %** (≈ 13 % avec l'ancien voile).
- Accroches hors zone (annoncées) : `src/render/renderer.*` + `shaders/ui.frag` (affichage d'une photo dans l'interface, binding 1 du jeu de la police), `src/game/game.*` (2 lignes de banc), `portique_depart_modele.*` (`platine` publique), `desktop_main.cpp` (bancs `portique-checkpoint`, `modes-photos`, `modes-ecran`).
- Vérifié : compilation complète, 155 tests CPU réussis (nouveau `test_portique_checkpoint`), captures avant / après dans `qa/pr/WFS-562/`. Aucun téléchargement.
- Suite : avis de l'agent 3D (RG-08), puis lead dev.

### 2026-11-19 00:55:41 — WFS-562 : PR #435 refusée par le lead dev (lead-dev)
- Revue du commit 23d3e721 : toutes les captures ouvertes, tout le diff lu, compilation et `tests_cpu.sh` refaits (155 réussis), avis de l'agent 3D « VALIDÉ avec réserves » lu. Rendu conforme (photos, portique sans cyan, fond à 35 % au lieu de 20 %).
- Points à corriger : (1) section « Tests CPU » et `tests-cpu.log` absents (règle du 2026-11-11) ; (2) `apres-parcours-mode-3-cartes` périmée (anciennes photos) ; (3) `Renderer::shutdown` : remettre `atlasVue_` et `photosVue_` à zéro ; (4) accroche du rendu d'environ 30 lignes : accord écrit du PO dans `regles-de-gestion/WFS-562.md`.
- Revue : https://github.com/wlfaleh/waforspeed_mobile/pull/435

### 2026-11-19 00:56:32 — WFS-562 : accord du PO pour l'accroche du rendu hors zone (po-scrum-master)
- Demande de Claude, agent dev3d (01UY1Vv3kc), PR #435, point 4 du lead dev : accroche d'environ 30 lignes dans `src/render/renderer.h/.cpp` et 6 lignes dans `shaders/ui.frag` pour afficher les photos des modes (RG-03 : l'interface n'avait qu'un atlas de police).
- Décision : **accordé**, limité à ces fichiers et fonctions (`uiSetPhotos`, `uiPhoto`, branche « photo » du shader), sans autre changement du rendu ; point 3 du lead dev (`Renderer::shutdown`) à corriger. Écrit dans la section « Suivi » de `regles-de-gestion/WFS-562.md`.

### 2026-11-19 01:35:28 — WFS-562 : PR #435 corrigée après la revue du lead dev (Claude, agent dev3d (01UY1Vv3kc))
- Points du lead dev : journal des tests CPU (`qa/pr/WFS-562/tests-cpu.log`, 155 réussis) et section « Tests CPU » ; capture `parcours-mode-3-cartes` refaite ; `Renderer::shutdown` remet `atlasVue_` et `photosVue_` à zéro ; accroche du rendu couverte par l'accord du PO (97672604).
- Avant : avis de l'agent 3D « VALIDÉ avec réserves » (23d3e721) après correction des photos Checkpoint et Poursuite, de la bâche (orange de sécurité, lisible la nuit) et du lest.
- Limite : le conteneur a redémarré pendant les tests ; suite complète relancée après la synchronisation avec `main` (14634dad).

### 2026-11-19 01:43:50 — WFS-562 fusionné par le lead dev (lead-dev)
- 2e revue du commit 5fe8cbd0 : les 4 points du refus sont corrigés (section « Tests CPU » et `tests-cpu.log`, capture `apres-parcours-mode-3-cartes` refaite, `atlasVue_` et `photosVue_` remis à zéro dans `Renderer::shutdown`, accord du PO pour l'accroche du rendu). Compilation et `tests_cpu.sh` refaits : 155 réussis. Photos et portique inchangés depuis l'avis « VALIDÉ » de l'agent 3D.
- « ✅ Approuvé » posté en commentaire (même compte GitHub), puis fusion en squash : 2620196d. Statut 🟡 À valider (utilisateur), ligne ajoutée à `qa/Check.md`, ligne retirée de `suivi-pr.md`.

### 2026-11-19 01:44:19 — WFS-562 fusionné (PR #435, 2620196d) : session terminée (Claude, agent dev3d (01UY1Vv3kc))
- Fusion en squash par le lead dev à la 2e revue ; ticket 🟡 À valider (utilisateur) sur son téléphone. Réserves (agent 3D, `qa/Check.md`) : bâche un peu lumineuse la nuit, ligne sur un passage piéton (Ville, CP 1), Course à 5 à faire voir au juriste (formes, plaque).
- Réponse écrite à l'utilisateur dans `tache utilisateur.md` (demande « il faut évoluer les checkpoint »).
- Un seul sujet par session : je m'arrête, aucun autre ticket demandé.

### 2026-11-19 02:29:36 — Statuts automatiques, CI main #1155 (robot statuts)
- Run rouge (push, commit 2620196), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-555, WFS-564, WFS-496, WFS-503, WFS-408.

### 2026-11-19 11:09:33 — Statuts automatiques, bancs du matin #54 (robot statuts)
- Run rouge (schedule, commit 3d216d7), aucun statut changé. Job(s) en échec : tests CPU et licences, endurance et chauffe, banc (complements-c), banc (ecrans-2400), banc (realisme-2), rapports du réalisme (fusion des deux parties), publication-captures.
- Tickets 🟣 contenus dans ce run : WFS-555, WFS-564, WFS-496, WFS-503, WFS-408.

### 2026-11-19 11:28:27 — Archives automatiques (robot archives)
- Journal : 8 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-17).
- TACHES.md : 1 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md` (WFS-566).

### 2026-11-19 19:09:44 — WFS-567 attribué à Claude, agent dev3d (01UY1Vv5BA) (po-scrum-master)
- Demande d'une nouvelle session `@dev` (session_01UY1Vv5BAe81SAYYdNAVWTi, aucun ticket avant) : **WFS-567** (P1, E8 HUD de course, 2e de l'ordre `@dev` du 2026-11-18 20:44 ; WFS-562 déjà fusionné), correction ANO-315 + ANO-290.
- Zone autorisée : `src/ui/toasts.cpp`, `src/ui/hud.cpp` ; textes des messages dans `src/game/poursuite.cpp` et `src/game/trace_cartes_systeme.cpp` seulement pour leur affichage. Interdit : `src/world/portique.cpp` (fait par WFS-562), menus et autres couleurs de l'interface.
- Critères : plus de cyan dans les messages de course, texte clair sur fond sombre, en haut sous le HUD, jamais dans la zone centrale de la route, 2 messages au plus à la fois, aucun message retiré (`regles-de-gestion/WFS-567.md`). Après fusion : 🟡 À valider (utilisateur).
- Branche `feature/waforspeed-567`, premier commit poussé tout de suite ; captures ciblées avant / après (jour et nuit), PR, appel du `lead-dev`, arrêt de la session après la fusion.

### 2026-11-19 20:36:44 — WFS-567 : PR #436 ouverte, lead dev appelé (Claude, agent dev3d (01UY1Vv5BA))
- Fait : messages de course refaits (ANO-315, ANO-290) : plus de cyan, texte clair sur fond sombre translucide avec filet de couleur (orange gain, rouge danger, blanc info), en haut sous le HUD (en Poursuite : à gauche sous le chrono), 2 au plus, quasi-contacts regroupés (« QUASI-CONTACT ×3 +750 ») ; bandeau des surprises et compte d'arrestation de la Poursuite passés dans les mêmes messages ; conseil de départ dans le même style, sous l'encart SCORE / POLICE, entier en 1280x720 et en 20:9 ; titres des surprises ≤ 18 caractères (FR / EN).
- Fichiers : `src/ui/toasts.h`, `toasts.cpp`, `messages_banc.cpp` (nouveau, banc `WFS_BANC=messages`), `src/game/poursuite.h`, `poursuite_system.cpp` (dessin 2D seulement), `trace_cartes_systeme.cpp` (1 ligne), `src/world/portique_depart.cpp` (dessin de la consigne seulement), `src/platform/desktop_main.cpp` (accroche ≤ 2 lignes), `assets/textes/fr.txt`, `en.txt`, `tests/test_messages.cpp`, `CMakeLists.txt`.
- Zone : la consigne de départ est dessinée par `src/world/portique_depart.cpp` (WFS-559) et le bandeau des surprises par `poursuite_system.cpp` (pas `hud.cpp` / `poursuite.cpp` comme écrit dans la zone) : seul leur dessin 2D change ; `src/world/portique.cpp` (WFS-562) non touché.
- Vérifié : 156 tests CPU réussis (dont `test_messages`) ; parcours complet `textes-course` FR et EN : 0 écart, 0 échec du garde-fou ; captures avant / après (8 en 1280x720 + 4 en 2400x1080, jour et nuit) dans `qa/pr/WFS-567/`.
- Limites : pas vu sur téléphone (🟡 après fusion) ; « COMBO ×N » séparé fondu dans le titre regroupé.


### 2026-11-19 20:47:39 — WFS-567 : PR #436 refusée par le lead dev (lead-dev)
- Revue du commit `da79a574` : 12 paires de captures ouvertes (jour, nuit, 20:9). Plus de cyan, 2 messages au plus, en haut et hors de la route ; conseil de départ entier en 1280x720 et en 20:9 (ANO-290 corrigé). Diff lu en entier, branche construite, `tests_cpu.sh` = 156 réussis.
- **Un seul défaut, bloquant par la règle du 2026-11-11** : pas de section « Tests CPU » dans la PR ni de `qa/pr/WFS-567/tests-cpu.log`. Aucune nouvelle capture d'écran demandée. Revue postée en commentaire (même compte GitHub). Statut ↩️ Retour lead dev (PR #436).

### 2026-11-19 20:57:27 — WFS-567 : retour du lead dev corrigé (Claude, agent dev3d (01UY1Vv5BA))
- Défaut unique (tests CPU absents de la PR) corrigé dans dec0e550 : branche synchronisée (11a0ab9e), `tests_cpu.sh` : 156 réussis, journal `qa/pr/WFS-567/tests-cpu.log`, section « Tests CPU » dans `pr.md` et la description. Réponse sous la revue ; PR #436 en 🔁, lead dev rappelé.
- Remarque : la revue précédente avait laissé l'arbre de travail du développeur sur `main` (branche du ticket intacte) : tests relancés sur la bonne branche.

### 2026-11-19 20:59:00 — WFS-567 fusionné par le lead dev (lead-dev)
- 2e revue de la PR #436 (commit dec0e550) : le seul défaut de la 1re revue (tests CPU absents) est corrigé : section « Tests CPU » dans la PR et `qa/pr/WFS-567/pr.md`, journal complet `qa/pr/WFS-567/tests-cpu.log` (156 réussis, `test_messages` compris). Réponses « Bien reçu » et « corrigé dans dec0e550 » présentes. Depuis da79a574, aucun code ni écran ne change (fichiers de suivi seulement).
- « ✅ Approuvé par le lead dev » posté en commentaire de revue (GitHub refuse l'approbation : même compte), puis fusion en squash : **e9a9901a**.
- TACHES : WFS-567 🟡 À valider (utilisateur) (fusionné #436) ; ligne #436 retirée de `suivi-pr.md` ; ligne ajoutée à `qa/Check.md`.

### 2026-11-19 20:59:34 — WFS-567 : fin, session terminée (Claude, agent dev3d (01UY1Vv5BA))
- PR #436 fusionnée en squash par le lead dev (e9a9901a) après 2 revues (1er refus : tests CPU absents de la PR, corrigé dans dec0e550). Statut 🟡 À valider (utilisateur) ; ligne ajoutée à `qa/Check.md` par le lead dev.
- Réponse à l'utilisateur sous sa demande (ANO-315) dans `tache utilisateur.md`. Ligne de `track-agent.md` en ⏹️ session terminée : un seul ticket par session, aucun autre ticket demandé.

### 2026-11-19 21:40:22 — Statuts automatiques, CI main #1156 (robot statuts)
- Run rouge (push, commit e9a9901), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-555, WFS-564, WFS-496, WFS-503, WFS-408.

### 2026-11-20 00:17:22 — WFS-568 attribué à Claude, agent dev3d (01UY1Vv3p8) (po-scrum-master)
- Demande d'une nouvelle session `@dev` (session_01UY1Vv3p8auTZYZDh8Q5G5N, aucun ticket avant). Ordre `@dev` du bandeau 🎮 : 562 et 567 fusionnés (🟡), donc **WFS-568** (P1, E15, ANO-316 : enseignes du décor réalistes, plus de néon cyan, Ville et Corniche) est le 1er ⏳ Prêt.
- Zone : `src/world/enseignes.cpp`, enseignes des lieux de `src/world/ville.cpp` (l. ≈ 1325-1332) et leurs actifs. Règles : `regles-de-gestion/WFS-568.md` (RG-01 à RG-08, captures demandées). Avis « Avis agent 3D : VALIDÉ » obligatoire avant fusion (E15). Skills en plus : `expert-realisme-3d` (lecture), `expert-cartes` si un paquet `.wfcarte` change (tâche compagne WFS-569). Après fusion : 🟡 À valider (utilisateur).
- `track-agent.md` : sessions 01UY1Vv5BA (WFS-567) et 01UY1Vv3kc (WFS-562) terminées, lignes enlevées. CI de `main` #1156 rouge (banc-essai texte, commit e9a9901) : relève de WFS-589 (`@cicd`, P1, ⏳ Prêt).

### 2026-11-20 00:19:21 — WFS-568 : branche créée, étude des enseignes (Claude, agent dev3d (01UY1Vv3p8))
- Ticket reçu du PO (enseignes du décor réalistes, plus de cyan, ANO-316). Branche `feature/waforspeed-568` poussée. Lecture de `src/world/enseignes.cpp` et `src/world/ville.cpp`.
