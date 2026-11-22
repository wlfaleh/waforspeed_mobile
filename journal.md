# WaForSpeed Mobile — journal

Journal du projet (tickets WFS-XX de `TACHES.md`) : une entrée par étape (début, construction, PR, verdict QA, fin…) avec
fait / fichiers / vérification / limites. **Nouvelles entrées à la fin du fichier.** Toujours partir de la dernière version de
`main` (`git pull`), ajouter seulement ses entrées, puis envoyer tout de suite ; ne jamais écraser le fichier.
**Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
Exemple : `### 2026-11-03 19:43:10 — WFS-58 construit, essai simulateur lancé (Claude, session trafic)`.
> **Archive (règle de l'utilisateur, 2026-11-06)** : ce fichier ne garde que les **2 derniers jours**. Les entrées plus anciennes sont dans `journal/archive/AAAA-MM-JJ.md` (un fichier par jour) ; ne les relire qu'en cas de besoin précis. Le PO déplace chaque jour les entrées de plus de 2 jours.

### 2026-11-21 10:15:58 — Statuts automatiques, bancs du matin #62 (robot statuts)
- Run rouge (schedule, commit 2ade8f9), aucun statut changé. Job(s) en échec : endurance et chauffe, tests CPU et licences, banc (realisme-2), banc (complements-c).
- Tickets 🟣 contenus dans ce run : WFS-555, WFS-564, WFS-496, WFS-503, WFS-408.

### 2026-11-21 10:53:50 — Archives automatiques (robot archives)
- Journal : 16 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-19).
- TACHES.md : 1 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md` (WFS-569).

### 2026-11-21 15:44:07 — WFS-590 attribué à Claude, agent dev3d (01UY1Vv5QX) : Début (po-scrum-master)
- **Ticket** : WFS-590 (P1, E15 « Décors », ANO-317, KO de WFS-568) : caissons sur mâts retirés de l'affichage (collisions comprises, code gardé désactivé), puis enseignes bandeau et drapeau en façade des commerces (Ville, Corniche), sucette / colonne Morris facultatives en Ville, rien sur la Corniche. Règles : `regles-de-gestion/WFS-590.md`.
- **Session** : Claude, agent dev3d (01UY1Vv5QX) (session_01UY1Vv5QXpZPb23RjZEJ4hP), nouvelle session `@dev`, aucun ticket avant ; n'est pas la session exclue 01UY1Vv3p8. Branche `feature/waforspeed-590`.
- **Zone de code** : `src/world/enseignes.cpp`, enseignes de `src/world/ville.cpp`, `src/world/commerces.cpp`, enseignes de `src/world/plage.cpp`, `src/world/mobilier_ville.*`, bancs `carte-*-enseigne-*`.
- **Plan court** : 1er commit = suppression de l'affichage (RG-00 a / RG-01) ; puis enseignes en façade aux mesures réelles (RG-02 à 06) avec 3 photos réelles libres (RG-08) ; captures ciblées jour / nuit ; avis « VALIDÉ » de l'agent 3D ; PR et lead dev ; 🟡 à valider par l'utilisateur.
- **Bancs du matin #62 rouges** (endurance et chauffe, tests CPU et licences, realisme-2, complements-c) : rangés dans WFS-589 (P1, `@cicd`, ⏳ Prêt), aucune nouvelle US ; une nouvelle session `@cicd` est nécessaire.

### 2026-11-21 16:40:41 — WFS-590 : PR #438 ouverte (Claude, agent dev3d (01UY1Vv5QX))
- **Fait** : (1) caissons sur mâts retirés de l'affichage en Ville et sur la Corniche, collision de leur mât retirée (`ville::CAISSONS_SUR_MATS = false`, code gardé) ; **accroche de 5 lignes hors zone annoncée ici** : `src/world/ville_quartiers.h` (la constante) et `ville_quartiers.cpp` (collision sous condition), tirages inchangés. (2) Enseignes bandeau à plat en façade (Ville : tous les commerces ; Corniche : cafés, location de scooters), enseigne drapeau sur potence par commerce de la Ville, croix verte de pharmacie allumée la nuit ; de jour, plus aucune lettre allumée. Modèles partagés dans `enseignes.cpp` (`vie::enseigneBandeau`, `vie::enseigneDrapeau`, `vie::croixPharmacie`), pavés chanfreinés et tubes de `mobilier_ville`.
- **Vérifié** : compilation, 156 tests CPU verts, 18 captures ciblées (`qa/pr/WFS-590/`) regardées ; rien n'est cuit dans un paquet `.wfcarte`.
- **Pour le PO** : (a) **photos réelles de RG-08 impossibles** : `commons.wikimedia.org` et `upload.wikimedia.org` refusés par le proxy (403), aucun contournement ; mesures sourcées à la place (`qa/pr/WFS-590/references/README.md`, statut `a_confirmer`). (b) **Parcours complet** (`WFS_BANC=parcours`) : 1 écart « fin puis Menu → accueil → écran course », **déjà présent sur `main` 869082d4** (rejoué avec le binaire de `main`, même résultat) : pas causé par WFS-590, à ranger dans un ticket. (c) Sucette, colonne Morris et 4 × 3 mural (facultatifs) non posés ; « TURBO CAFÉ », « NUIT BLANCHE », « PNEUS 24/7 », « WAFORSPEED » n'ont pas de commerce : plus affichés.
- **Suite** : avis de l'agent 3D (E15), puis lead dev.

### 2026-11-21 16:53:14 — WFS-590 : PR #438 refusée par le lead dev (lead-dev)
- **Revue** du commit `7fbdb129` : 26 captures ouvertes (rendu conforme à la demande : caissons sur mâts disparus en Ville et sur la Corniche, bandeaux et drapeaux en façade, rien d'allumé de jour), diff complet lu, branche construite et **156 tests CPU réussis** chez le lead dev ; avis agent 3D « VALIDÉ avec réserves » lu.
- **Refus** (revue https://github.com/wlfaleh/waforspeed_mobile/pull/438#pullrequestreview-6006721622) : (1) **RG-01 non tenue** : `src/game/chocs_declaration.cpp:76` déclare toujours 2 poteaux invisibles par panneau de la Ville dans le registre des chocs du joueur (segments 389 et 4679 : 5015 obstacles contre 5011 sans la ligne) ; `CAISSONS_SUR_MATS` ne coupe que l'ancienne collision `Aide::Web` ; (2) aucun test CPU de la nouvelle logique ; (3) capture « Tests CPU » et `qa/pr/WFS-590/tests-cpu.log` absentes. Remarques : commentaires WFS-369 / WFS-356 déplacés dans `commerces.cpp`, capture « GARAGE 88 » à expliquer. Nouvel avis agent 3D « VALIDÉ » requis sur le prochain commit.
- `TACHES.md` : WFS-590 → ↩️ Retour lead dev (PR #438) ; `track-agent.md` et `suivi-pr.md` à jour.

### 2026-11-21 17:04:39 — WFS-590 : PR #438 corrigée après le refus du lead dev (Claude, agent dev3d (01UY1Vv5QX))
- **Cause racine** : les caissons sur mâts avaient deux collisions ; je n'avais coupé que celle du plan (`ville_quartiers.cpp`, conduite d'avant), pas les 2 poteaux par panneau du registre des chocs (`src/game/chocs_declaration.cpp:76`), qui sert à la voiture du joueur : 4 poteaux invisibles (segments 389 et 4679).
- **Corrigé dans a51c6a62** : **accroche hors zone de 3 lignes** dans `src/game/chocs_declaration.cpp` (poteaux seulement si `ville::CAISSONS_SUR_MATS`) ; nouveau test `test_enseignes_retirees` (échoue sans la correction, passe avec : 5011 obstacles au lieu de 5015) ; `qa/pr/WFS-590/tests-cpu.log` (157 tests verts) ; commentaires de `commerces.cpp` remis en place ; `pr.md` corrigé (collisions, « GARAGE 88 »).
- **Suite** : nouvel avis de l'agent 3D sur dbf0cbf9, puis lead dev.

### 2026-11-21 17:14:30 — WFS-590 fusionné par le lead dev (lead-dev)
- **2e revue** de la PR #438 sur `dbf0cbf9` : les 3 défauts sont corrigés (poteaux du registre des chocs soumis à `ville::CAISSONS_SUR_MATS`, 5011 obstacles ; `test_enseignes_retirees`, qui passe, et qui échoue bien sans la correction, vérifié par le lead dev ; section « Tests CPU » et `tests-cpu.log`), et les commentaires de `commerces.cpp` sont remis en place. Construction sans avertissement ; **157 tests CPU** réussis chez le lead dev. Rendu inchangé depuis la 1re revue. Avis agent 3D « VALIDÉ » sur dbf0cbf9.
- Approbation en commentaire (même compte GitHub) : https://github.com/wlfaleh/waforspeed_mobile/pull/438#pullrequestreview-6006863082 ; **fusion en squash 5311bfe6**.
- `TACHES.md` : WFS-590 → 🟡 À valider (utilisateur) ; ligne retirée de `suivi-pr.md` ; `qa/Check.md` et `track-agent.md` à jour. Pour le PO (US de suite) : RG-08 (photos réelles), réserves de l'agent 3D (croix, lettres en relief), textes « TURBO CAFÉ », « NUIT BLANCHE », « PNEUS 24/7 » sans commerce, RG-04 / RG-05.

### 2026-11-21 17:15:04 — WFS-590 : fin, session terminée (Claude, agent dev3d (01UY1Vv5QX))
- PR #438 fusionnée en squash par le lead dev (5311bfe6), après une correction (poteaux invisibles du registre des chocs, test `test_enseignes_retirees`) ; avis agent 3D « VALIDÉ » sur dbf0cbf9. WFS-590 en 🟡 À valider (utilisateur) ; réponse écrite dans `tache utilisateur.md`.
- **Pour le PO (US de suite)** : réserves de l'agent 3D et du lead dev : photos RG-08 (Wikimedia bloqué) et R581-60 / R581-61 `a_confirmer` ; croix de pharmacie d'un seul matériau, éteinte de jour (RG-06 à trancher) ; lettres imprimées à plat ; commerces « TURBO CAFÉ », « NUIT BLANCHE », « PNEUS 24/7 » à créer ou non ; RG-04 / RG-05 facultatifs non faits ; écart du parcours complet « fin puis Menu → accueil » déjà présent sur `main`.
- Un seul ticket par session : je m'arrête, je ne demande pas d'autre ticket.

### 2026-11-21 17:54:23 — Statuts automatiques, CI main #1158 (robot statuts)
- Run rouge (push, commit 5311bfe), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-555, WFS-564, WFS-496, WFS-503, WFS-408.

### 2026-11-22 10:27:44 — Statuts automatiques, bancs du matin #67 (robot statuts)
- Run rouge (schedule, commit 1b1c3ac), aucun statut changé. Job(s) en échec : endurance et chauffe, tests CPU et licences, banc (realisme-2), banc (complements-c).
- Tickets 🟣 contenus dans ce run : WFS-555, WFS-564, WFS-496, WFS-503, WFS-408.

### 2026-11-22 11:06:16 — Archives automatiques (robot archives)
- Journal : 12 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-20).
- TACHES.md : 0 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md`.

### 2026-11-22 11:45:33 — WFS-589 attribué à agent expert ci cd (01UY1Vv43t) (po-scrum-master)
- Début : **WFS-589** (P1, E7, [TECHNIQUE]) attribué à agent expert ci cd (01UY1Vv43t) (session_01UY1Vv43tZmAYr8FEMaKo7S), nouvelle session `@cicd` (aucun ticket avant), à sa demande (« fix les problèmes de ci »).
- Constat : CI de `main` rouge depuis #1155 (WFS-562, PR #435) jusqu'à #1158, job « banc-essai (texte) » : `ui/modes/checkpoint.jpg`, `course5.jpg`, `poursuite.jpg` sans crédit (absence vérifiée par le PO dans `tools/qa/verifier_licences.py`, c52261e). Bancs du matin #67 rouges : tests CPU et licences (même cause), complements-c et chauffe (parcours « fin → Menu »), realisme-2.
- Ajout aux règles : **RG-06** (crédits des 3 images dans `tools/qa/verifier_licences.py` seulement, provenance d'après la PR #435, aucun contrôle affaibli). Zone autorisée : `tools/qa/**` et parcours du banc dans `src/platform/desktop_main.cpp`. Branche `feature/waforspeed-589`. US technique : CI de branche autorisée, une à la fois, chaque run au journal.
- Session de WFS-590 (01UY1Vv5QX) terminée : ligne enlevée de `track-agent.md`.

### 2026-11-22 11:45:36 — Liste des 30 prochaines tâches donnée à l'utilisateur (po-scrum-master)
- Aucune session de développeur active (WFS-590 fusionné #438, 🟡). CI de `main` #1158 (banc texte) et bancs du matin #67 (endurance, tests CPU et licences, realisme-2, complements-c) rouges : mêmes jobs que #62, déjà rangés dans **WFS-589** (P1, `@cicd`), pas de nouvelle US.
- **WFS-587** : ⏳ En attente → ⏳ Prêt (WFS-567 fusionné, #436).
- Créés : **WFS-591** (P1, `@cicd`, publication des paquets après WFS-590, ordre permanent du 2026-11-12) et **WFS-592** (P2, `@dev` + avis `@3d`, réserves de WFS-590 : lettres en relief, croix de pharmacie, mesures R581 ; ordre « on ne jette pas un travail »).
- Ordre donné : `@dev` 571 → 573 → 575 → 563 → 512 → 570 → 554 → 444 → 576 → 577 → 581 → 583 → 585 → 586 → 587 → 592 → 588 ; `@conduite` 584 → 394 ; `@cicd` en parallèle 589 → 454 → 591 → 466 → 471 → 527 → 287.

### 2026-11-22 11:50:10 — Menus et écrans avant la course bloqués (ordre de l'utilisateur) (po-scrum-master)
- Ordre : « Bloque toutes les tâches qui touchent le menu ou les espaces avant de jouer : garage, réglages, menu… ». Noté dans les ordres permanents du skill PO et dans le bandeau ⛔ de `TACHES.md`.
- ⛔ Bloqués : WFS-563, 554, 586, 587, 588, 570, 553, 538, 556, 497, 486, 357, 347, 332, 345 (statuts d'avant gardés ici : 563, 554, 586, 587, 588, 570, 553, 538, 357, 332 ⏳ Prêt ; 556, 497, 486, 347, 345 ⏳ En attente).
- Non bloqués (hors de ces écrans) : 512, 514, 585, 584, 589 (en cours), 454. Ordre `@dev` : 571 → 573 → 575 → 512 → 444 → 576 → 577 → 581 → 583 → 585 → 592.

### 2026-11-22 11:52:55 — Pub et Google Play bloqués (ordre de l'utilisateur) (po-scrum-master)
- Ordre : « pareil pour les tâches publicité Google machin, à bloquer pour l'instant ». Noté dans les ordres permanents du skill PO et dans le bandeau ⛔ de `TACHES.md`.
- ⛔ Bloqués : WFS-512 (⏳ Prêt avant), 514, 515 (⏳ En attente avant), 516, 319, 320, 405, 407, 409, 509 (⏸ jour de publication avant). WFS-408 (🟣) inchangé.
- Ordre `@dev` : 571 → 573 → 575 → 444 → 576 → 577 → 581 → 583 → 585 → 592.

### 2026-11-22 11:56:09 — Liste des 30 prochaines tâches écrite dans `TACHES.md` sur `main` (po-scrum-master)
- Demande de l'utilisateur : « Donne-moi la liste 30 dans main ». Bandeau 📋 en tête de `TACHES.md`, sans les tickets ⛔ (menus et écrans avant la course, pub et Google Play) ; la liste de 11:45 UTC est barrée.

### 2026-11-22 12:00:28 — WFS-593 créé : seulement 10 % des personnes dans les cartes (po-scrum-master)
- Demande de l'utilisateur : « Crée un ticket pour passer à seulement 10 % des personnes dans les cartes ». **WFS-593** (P1, E11, ⏳ Prêt, nouvelle session `@dev`), 1er de la liste `@dev` ; 341 sort de la liste des 30.
- Règles : `regles-de-gestion/WFS-593.md` (10 % de `main` par catégorie et par carte, un seul facteur, tirage existant `garder()` réutilisé, rien créé en course, conducteurs et deux-roues inchangés). WFS-576 RG-02 mis à jour (le nombre suit `main`, donc WFS-593).

### 2026-11-22 12:02:36 — WFS-594 créé : routes et sols réalistes de près (po-scrum-master)
- Demande de l'utilisateur : « Créer une US pour améliorer le graphique » ; partie choisie par l'utilisateur (question du PO) : **routes et sols**.
- **WFS-594** (P1, E9 + sols, ⏳ Prêt, `@dev` dev-artiste-3d + avis `@3d`), 2e de la liste `@dev` ; tâche compagne **WFS-595** (`@cicd`, ⏳ En attente). Règles : réutiliser les textures de `assets/textures/route/` et `sols.glsl` (WFS-205, 206, 383), E16 (⛔) non touchée, textures libres seulement par un dépôt GitHub public (`regles-reseau.md`).
- **WFS-156** : ⏳ En attente → ⏳ Prêt (WFS-206 ✅) ; sa partie marquages est faite par WFS-594 (RG-03).

### 2026-11-22 12:03:57 — WFS-596 créé : mini-carte sur l'écran central en vue Cockpit (po-scrum-master)
- Demande de l'utilisateur : « Créer une US spécifique mode cockpit pour mettre la même map dans l'écran central des voitures et l'enlever de l'écran de jeu ». **WFS-596** (P1, E5 + E10, ⏳ Prêt, `@dev`), 3e de la liste `@dev`.
- Réutiliser `src/ui/minicarte.*` (WFS-173) et l'écran central de `vehicule_cockpit.*` (WFS-201 RG-05, carte simplifiée remplacée) ; mêmes fichiers que WFS-583 : le 2e fusionné se synchronise ; WFS-583 RG-03 renvoie la mini-carte à WFS-596. Pas de tâche `@cicd` (rien de cuit dans les paquets).

### 2026-11-22 12:08:26 — WFS-597 créé : fond de l'écran de chargement, 5 maquettes (po-scrum-master)
- Demande de l'utilisateur : image d'arrière-plan de l'écran de chargement avec l'esprit des voitures de course, 5 maquettes, rien d'autre ne change. **WFS-597** (P2, E8, ⏳ En attente du choix de l'utilisateur) ; exception explicite au blocage des menus de 11:50 UTC (fond seulement).
- Maquettes faites par le PO (esquisses dessinées par le code, aucun téléchargement) : `regles-de-gestion/maquettes/WFS-597/proposition-1` à `5` (feux de départ, filé de nuit, gomme et fumée, vibreur au couchant, drapeau à damier).

### 2026-11-22 12:12:00 — WFS-597 : 2e série de maquettes, faites avec les images du jeu (po-scrum-master)
- 1re série refusée par l'utilisateur (« C'est nul tes propositions »). 2e série : captures des bancs du matin (Release `captures-matin` du dépôt, groupes `nuit` et `vitrines`, téléchargées depuis GitHub, aucune licence tierce), recadrées et retouchées ; anciennes maquettes retirées du dossier. RG-05 : image finale = rendu du moteur en 2400 × 1080 natif. ⏳ En attente du choix de l'utilisateur.

### 2026-11-22 12:13:04 — WFS-597 : choix de l'utilisateur, la voiture du menu dans une rue en 3D (po-scrum-master)
- L'utilisateur : « inspirer le fond de notre voiture du menu, mais dans un plan 3D dans la rue, style réaliste 100 % ». Règles ajoutées (RG-08 à RG-11 : SF-X caméléon et skin bleu ciel de WFS-552, rue de la Ville, trois quarts avant caméra basse, soir / nuit humide, rendu 2400 × 1080 par le banc, 3 variantes + avis `@3d`). **WFS-597 ⏳ Prêt** (P2, `@dev` dev-artiste-3d).

### 2026-11-22 12:17:09 — WFS-597 : maquettes de l'idée de l'utilisateur, en attente de son accord (po-scrum-master)
- L'utilisateur : « tes maquettes nulles, je dois accepter la maquette avant le changement ». WFS-597 ⏳ Prêt → **⏳ En attente** (RG-00) ; ordre permanent noté dans le skill PO.
- 3e version : SF-X du menu découpée de `references/maquettes-R19/01-menu.jpg` et posée dans 3 rues de la Ville la nuit (captures `captures-matin`, voiture du jeu effacée) : maquettes A, B, C ; séries 1 et 2 retirées du dossier.

### 2026-11-22 12:24:36 — WFS-597 : maquettes de l'utilisateur ajoutées, en attente de son accord (po-scrum-master)
- L'utilisateur a fait 2 images avec un générateur d'images (SF-X du menu, boulevard parisien la nuit) : ajoutées à `regles-de-gestion/maquettes/WFS-597/` avec l'écran par-dessus ; référence seulement. Réserve : éclairage de nuit de la tour Eiffel protégé. Pas encore d'accord écrit : WFS-597 reste ⏳ En attente.

### 2026-11-22 12:27:19 — WFS-597 : maquette 2 acceptée par l'utilisateur, ⏳ Prêt (po-scrum-master)
- L'utilisateur : « Ok go pour 2 ». Référence : `regles-de-gestion/maquettes/WFS-597/utilisateur-2-boulevard-pluie.png` (RG-08 bis). WFS-597 ⏳ En attente → **⏳ Prêt** (P2, `@dev` dev-artiste-3d + avis `@3d`).

### 2026-11-22 12:39:23 — WFS-597 simplifié : l'image de l'utilisateur telle quelle (po-scrum-master)
- Ordre de l'utilisateur : « pas besoin de reproduire, c'est une image de fond, il la prend et la met directement ». RG-05 : l'image `utilisateur-2-boulevard-pluie.png` mise en fond telle quelle (taille et compression seulement) ; rendu 3D, RG-08 à 11 et avis `@3d` retirés.

### 2026-11-22 12:41:26 — WFS-597 en tête de la liste `@dev` (ordre de l'utilisateur) (po-scrum-master)
- WFS-597 passe P1 et 1er de la liste `@dev`, avant WFS-593, 594 et 596.

### 2026-11-22 12:45:35 — Liste des 30 prochaines tâches avec le type de session (po-scrum-master)
- Demande de l'utilisateur : « Liste 30 avec type dev ». Nouveau bandeau 📋 en tête de `TACHES.md` (17 `@dev`, 2 `@conduite`, 2 `@cartes`, 9 `@cicd`) ; la liste de 11:56 UTC est barrée.

### 2026-11-22 12:49:51 — WFS-597 attribué à Claude, agent dev3d (01UY1Vv1zn) (po-scrum-master)
- Début : **WFS-597** (P1, E8, écran de chargement : l'image 2 de l'utilisateur, SF-X sur un boulevard sous la pluie, en fond, telle quelle) attribué à Claude, agent dev3d (01UY1Vv1zn) (session_01UY1Vv1znETyWL8wzqa2tEb), nouvelle session `@dev` (aucun ticket avant), à sa demande (« @dev go ») : 1er ticket prêt de la liste `@dev` de 12:45 UTC (WFS-597 passé en tête sur ordre de l'utilisateur à 12:41 UTC ; 571, 573, 575 suivent après 593, 594, 596 ; 563 ⛔ bloqué).
- Zone autorisée : `src/ui/ecrans/chargement.*` (dessin du fond seulement), `assets/ui/chargement/fond.*`, `tests/**`. Exception au blocage des menus : fond seulement. Maquette acceptée par l'utilisateur (12:27 UTC). Pas E15 : pas d'avis `@3d` obligatoire. Règles : `regles-de-gestion/WFS-597.md`. Branche `feature/waforspeed-597`. Après fusion : 🟡 À valider (utilisateur).

### 2026-11-22 12:50:44 — WFS-593 attribué à Claude, agent dev3d (01UY1Vv4Di) (po-scrum-master)
- Début : **WFS-593** (P1, E11, seulement 10 % des personnes dans toutes les cartes) attribué à Claude, agent dev3d (01UY1Vv4Di) (session_01UY1Vv4DiuxbNaC43HNZKKg), nouvelle session `@dev` (aucun ticket avant), à sa demande (« @dev go »). WFS-597 était déjà pris (01UY1Vv1zn) : 1er ticket prêt de la liste `@dev`.
- Zone autorisée : `src/world/pietons.*`, `src/world/foule_poussette.*`, `src/world/apparition*`, réglage dans `configgame.json` s'il y en a, `tests/**` ; accroches ≤ 10 lignes annoncées au journal dans `ville.cpp`, `plage.cpp`, `circuit.cpp`. Pas E15 : pas d'avis `@3d` obligatoire. Après fusion : 🟡 À valider (utilisateur). Règles : `regles-de-gestion/WFS-593.md`.
- Plan : un facteur commun 0,10 appliqué au tirage existant (`garder()`, `apparition::VILLE_GARDES_SUR_10`), comptes avant / après au journal du chargement, test CPU des comptes, captures Ville et Corniche avant / après.


### 2026-11-22 12:51:35 — WFS-594 attribué à Claude, agent dev3d (01UY1Vv49f) (po-scrum-master)
- **Ticket** : WFS-594 (P1, E9 + sols) — routes et sols réalistes de près. WFS-597 et WFS-593 venaient d'être pris par d'autres sessions : 1er ticket `@dev` prêt de la liste de 12:45 UTC.
- **Session** : session_01UY1Vv49fLTvKRexaJUt2wy, nouvelle session `@dev` (dev-artiste-3d), aucun ticket avant (un sujet par session).
- **Règles** : `regles-de-gestion/WFS-594.md` (zone de code et critères). Branche : `feature/waforspeed-594`. Avis `@3d` demandé ; captures avant / après ciblées, PR, appel du `lead-dev` ; arrêt de la session après la fusion.

### 2026-11-22 12:52:35 — WFS-596 attribué à Claude, agent dev3d (01UY1Vv2sH) (po-scrum-master)
- Début : **WFS-596** (P1, E5 + E10, vue Cockpit : la mini-carte du jeu sur l'écran central de la voiture, et plus à l'écran) attribué à Claude, agent dev3d (01UY1Vv2sH) (session_01UY1Vv2sHodzrS5FyXVX9uR), nouvelle session `@dev` (aucun ticket avant), à sa demande (« @dev go »). WFS-597, 593 et 594 déjà pris : 1er ticket prêt de la liste `@dev`.
- Zone autorisée : `src/ui/minicarte.*`, `src/ui/hud.*` (affichage de la mini-carte seulement), `src/render/vehicule_cockpit.*` (écran central et carte), `tests/test_minicarte*`, `tests/test_cockpit*` ; accroche ≤ 10 lignes annoncée au journal dans `src/game/camera_vue.cpp`. Fichiers du cockpit partagés avec WFS-583 : synchroniser avec `main`. Taille des boutons inchangée ; aucun menu modifié (⛔). Pas E15 : pas d'avis `@3d` obligatoire. Après fusion : 🟡 À valider (utilisateur). Règles : `regles-de-gestion/WFS-596.md`.

### 2026-11-22 12:53:06 — WFS-571 attribué à Claude, agent dev3d (01UY1Vv4Xe) (po-scrum-master)
- Début : **WFS-571** (P1, E15 Ville, correction ANO-280 : tours en boîtes et cubes nus sur les toits remplacés par de vrais volumes) attribué à Claude, agent dev3d (01UY1Vv4Xe) (session_01UY1Vv4Xeus56avzEmiAhcZ), nouvelle session `@dev` (aucun ticket avant), à sa demande (« Go @dev »). Il demandait WFS-597 ; 597 (01UY1Vv1zn), 593 (01UY1Vv4Di), 594 (01UY1Vv49f) et 596 (01UY1Vv2sH) ont été pris entre 12:49 et 12:52 UTC : 1er ticket prêt suivant de la liste `@dev`.
- Zone autorisée : `src/world/ville.cpp`, `ville_quartiers.*`, `archi_*`, `decor_ville_*`, `assets/world/modeles/ville/**`, `assets/textures/decor/ville/**`, `tests/**`. Skills : `agent-developer`, `waforspeed-natif`, `artiste-3d`, `expert-cartes` ; `expert-realisme-3d` en lecture. E15 : avis « VALIDÉ » de l'agent 3D sur le dernier commit avant la fusion, 700 Mo maximum. Modèles CC0 / CC BY seulement, notés au journal. Après fusion : 🟡 ; WFS-572 (`@cicd`) publie ensuite les paquets. Règles : `regles-de-gestion/WFS-571.md`.

### 2026-11-22 13:02:22 — WFS-571 : inventaire (RG-02) et plan (Claude, agent dev3d (01UY1Vv4Xe))
- Branche `feature/waforspeed-571` créée et poussée. Banc local (lavapipe) : `realisme-ville-large` de `main` refait, même image que la preuve d'ANO-280 ; Ville sur le **kit de secours** (paquet absent) : 625 bâtiments, 609 738 triangles au niveau 1, 302 330 au niveau 2.
- **Inventaire (RG-02)** :
  1. Tours = immeubles `Style::Moderne` (`ville::poserImmeublesKit`, `ville.cpp`, hauteur ≥ 20 m) assemblés par `archi::assembler` (`archi_kit.cpp`) et construits par `archi::construire` (`archi_geo.cpp`) : un seul prisme du sol au toit, tous niveaux de détail ; niveaux 1 et 2 (> 120 m) sans les nez de dalle (`facade()`, traits au niveau 0 seulement) → façade lisse vue de loin.
  2. Cube de toit = édicule de `details::terrasse` (`archi_details.cpp`) aux niveaux 1 et 2 : pavé + dalle, porte peinte à plat, sans ventilation ni linteau ; aucun autre équipement de toiture (CTA, garde-corps).
  3. Façades lisses aux fenêtres peintes : niveau 2 (> 300 m) seulement (`ouverture()`, baies peintes 1 cm devant le mur) ; à moins de 150 m, trous en retrait (niveaux 0 et 1) : conforme à RG-05. Façades arrière et latérales : peintes à tous les niveaux (WFS-455, choix de performance), hors des vues de la rue.
  4. Repli `WFS_ARCHI=0` (`box()` de `ville.cpp`) : inactif en jeu, gardé tel quel (rien retiré du code).
- **Plan** : (a) tours découpées en tronçons (socle, fût en retrait, attique) dans le kit (`archi::troncons`), posés comme bâtiments du kit (cuisson inchangée, une variante par graine : pas deux tours identiques côte à côte) ; (b) toits-terrasses : acrotère découpé sous les tronçons, édicule complet à tous les niveaux (dalle, porte métallique à cadre, linteau, grille de ventilation), centrale de traitement d'air sur rails, garde-corps de sécurité des toits des tours ; (c) nez de dalle des tours gardés au niveau 1. Version du bloc « batiments » montée (géométrie changée) : le paquet de la Ville est à republier (WFS-572).

### 2026-11-22 13:04:35 — WFS-589 : causes trouvées, corrections poussées, bancs du matin lancés sur la branche (agent expert ci cd (01UY1Vv43t))
- **CI `main` rouge depuis #1155** (job « banc-essai (texte) ») et **bancs du matin #67** (« tests CPU et licences ») : 3 photos des modes de WFS-562 (`assets/ui/modes/*.jpg`) sans ligne dans `tools/qa/verifier_licences.py` → ligne ajoutée (rendu du jeu, aucune image extérieure), 0 manque en local.
- **complements-c et chauffe** : le parcours du banc appuyait sur l'ancienne place du bouton « Menu » de l'écran de fin (déplacé par R19) → `desktop_main.cpp` (parcoursFinMenu) et `chauffe_systeme.cpp` (parcours du banc seulement) lisent la place réelle (`courseBouton("menu")`) ; en local : parcours 0 écart en 1280x720 et 2400x1080 (vue Cockpit comprise), chauffe code 0.
- **realisme-2 (2 erreurs)** : erreur du banc, pas du soleil : depuis les Réglages en 2 colonnes (R19), la ligne « Graphismes » partage sa hauteur avec « Fantôme du record », le banc comptait 6 choix au lieu de 4 et ne trouvait plus « Ultra » → `bench_passes.cpp` ne compte que le bloc de pastilles du réglage ; en local : « Ultra trouvé », 5 / 5 étapes. Aucun seuil changé, aucun changement du jeu.
- **CI de branche (US technique)** : bancs du matin lancés à la main sur `feature/waforspeed-589` (c63457f), run #70 https://github.com/wlfaleh/waforspeed_mobile/actions/runs/45025551433 : vérifie licences, complements-c, chauffe, realisme-2 (publications réservées à `main`).

### 2026-11-22 13:15:30 — WFS-596 : début, plan et accroches annoncées (Claude, agent dev3d (01UY1Vv2sH))
- Branche `feature/waforspeed-596` poussée (1er commit). Plan : en vue Cockpit, la mini-carte du HUD (même système `MiniCarte`, mêmes calculs, mêmes signalements) n'est plus dessinée par-dessus l'image : elle est dessinée dans un cadre virtuel aux proportions de l'écran réel, puis chaque sommet de l'interface est projeté en perspective (homographie) sur l'écran central vu de l'œil (SF90 : tiers droit du combiné ; Model 3 : 62 % de droite, à côté de la vitesse). L'ancienne carte simplifiée de l'écran (route et flèche de WFS-197) est retirée. Pas de cyan sur l'écran (itinéraire bleu), plus sombre la nuit, recalcul au plus 30 fois par seconde.
- Accroches annoncées (hors `minicarte.*` / `vehicule_cockpit.*`) : `src/render/renderer.h` (2 fonctions en ligne : `uiSommets()`, `uiDepuis()`, + 2 includes) ; `src/game/camera_vue.cpp` (tracé de l'ancienne carte retiré : −20 lignes, appel de `dessiner` simplifié) ; `src/render/vehicule_cockpit_vitrine.cpp` (1 ligne, même appel) ; `src/platform/desktop_main.cpp` (banc `carte-cockpit`, 2 lignes).
- Risque : la carte est dessinée avec l'interface (après la scène 3D) : un objet 3D qui passerait devant l'écran central (jante du volant) ne la cacherait pas ; à vérifier sur les captures (Giulia, Model 3, SF90).

### 2026-11-22 13:47:54 — WFS-597 : PR #439 ouverte, fond de l'écran de chargement (Claude, agent dev3d (01UY1Vv1zn))
- Fait : `assets/ui/chargement/fond.jpg` = image 2 de l'utilisateur (`regles-de-gestion/maquettes/WFS-597/utilisateur-2-boulevard-pluie.png`, SHA-256 bcc7aa72e60e220c3abb7122dce16db630be2da9b08e4a006e94046b5e121a5, origine : image fournie par l'utilisateur, aucun téléchargement) mise en 2400 × 1080 et JPEG q88, 563 242 octets, SHA-256 26c680619faeee198a372eecc42425b85858e99efea395bb0417829db3c5e7a7, sans retouche. `src/ui/ecrans/chargement.*` : image décodée sur un autre fil pendant la police, dessinée en couverture (sans déformation) à la place du fond uni et des reflets ; tout le reste de l'écran inchangé ; texture rendue après le fondu. Réutilise l'emplacement photo de l'interface de WFS-562 (aucun changement du rendu).
- Accroche annoncée (3 lignes, hors zone) : `tools/qa/verifier_licences.py`, motif `ui/chargement/fond\.jpg` (fourni par le projet, comme `ui/icone-pistolet.png`), pour ne pas rendre rouge le contrôle des licences.
- Vérifié : 158 tests CPU verts (`test_fond_chargement` nouveau) ; banc `chargement` 2400 × 1080 avant / après (8 paires) + 16:9 et 4:3 ; 1re image non retardée (décodage caché derrière la police, attente 0,000 s). Parcours : seul « fin → Menu » échoue, à l'identique sur `main` 699b39e9 (WFS-589).
- Limites : « FOR » du logo garde son remplissage du fond d'avant (RG-01) ; non vu sur téléphone. Lead dev appelé.

### 2026-11-22 13:52:38 — WFS-594 : routes et sols de près, code poussé, accroches annoncées (Claude, agent dev3d (01UY1Vv49f))
- Fait (branche `feature/waforspeed-594`) : `shaders/route.glsl` : défauts de l'enrobé (fissures transversales pontées, joint de répandage ponté, rustines découpées, gouttes d'huile), marquages de 15 cm à bords irréguliers, usés par plaques, peinture d'albédo 0,68, bordure T2 sur toutes les cartes (granit en Ville, béton ailleurs, joints de 1 m, arête du chanfrein, face sans étirement) ; `shaders/sols.glsl` : touffes d'herbe (10 à 30 cm) et brins de près, gravillons, relief par dérivées écran, sable de la Corniche (rides, grain, coquillages), neige fraîche au sol de la carte d'hiver (congères, crêtes de vent), bande de gravier au pied du trottoir du sol de Game. Textures faites par le code (Poly Haven et ambientCG bloqués, RG-05). Nouveau banc `routes-sols` (`src/render/route_sols_banc.cpp`).
- Accroches hors zone (≤ 10 lignes chacune) : `shaders/eclairage.glsl` (4 lignes : part de peinture → rétroréflexion des marquages aux phares), `shaders/scene.frag` (1 ligne : normale passée au sol), `src/game/game.cpp` (1), `src/world/ville.cpp` (2 : sol de la Ville = sol de Game texturé), `src/world/plage.cpp` (4 : partie « sable »), `src/world/decor_carte.cpp` / `.h` (4 : neige au sol des cartes enneigées), `src/platform/desktop_main.cpp` (2 : banc).
- Vérifié : `test_sols` et `test_fidelite_routes` verts (26 / 26 mesures) ; banc `routes-sols` en 1280 × 720 avant / après regardé. En cours : tous les tests CPU, coût GPU, captures de la PR.

### 2026-11-22 14:01:36 — WFS-597 fusionné par le lead dev (lead-dev)
- PR #439 relue sur 6e5f64d0 : 14 captures ouvertes (8 paires avant / après 2400 × 1080, 16:9 et 4:3), image livrée comparée à la maquette de l'utilisateur (compression seule), tout le diff lu ; branche construite et `tests_cpu.sh` refait : 158 test(s) réussi(s). « ✅ Approuvé » posté en commentaire (même compte GitHub), fusion en squash 528ed3b6.
- Remarques non bloquantes : texture du fond non détruite si l'application est fermée sur l'écran d'erreur ; mention de version « dev » moins lisible sur les reflets. WFS-597 → 🟡 À valider (utilisateur) ; ligne retirée de `suivi-pr.md` ; ajouté à `qa/Check.md`.
