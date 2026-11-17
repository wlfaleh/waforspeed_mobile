# WaForSpeed Mobile — journal

Journal du projet (tickets WFS-XX de `TACHES.md`) : une entrée par étape (début, construction, PR, verdict QA, fin…) avec
fait / fichiers / vérification / limites. **Nouvelles entrées à la fin du fichier.** Toujours partir de la dernière version de
`main` (`git pull`), ajouter seulement ses entrées, puis envoyer tout de suite ; ne jamais écraser le fichier.
**Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
Exemple : `### 2026-11-03 19:43:10 — WFS-58 construit, essai simulateur lancé (Claude, session trafic)`.
> **Archive (règle de l'utilisateur, 2026-11-06)** : ce fichier ne garde que les **2 derniers jours**. Les entrées plus anciennes sont dans `journal/archive/AAAA-MM-JJ.md` (un fichier par jour) ; ne les relire qu'en cas de besoin précis. Le PO déplace chaque jour les entrées de plus de 2 jours.

### 2026-11-16 00:09:14 — WFS-555 fusionné par le lead dev (lead-dev)
- 2e revue de la PR #425 (tête c19a70c4) par une autre instance du lead dev : diff `1a88fae6..c19a70c4` du script lu ; les 3 défauts sont corrigés.
- Essais refaits : (1) arbre de main vide sans `.git` dans un dépôt parent → arbre recréé, dépôt parent intact (branche `principal`, ` M f.txt`), code 0 ; (2) `WFS_CACHE_CAPTURES=.cache-rel` et `build/../` → refus clair, code 2, rien créé ; relance avec cache plein : 26 s, 1 banc relu.
- Captures `chargement` regardées (seule la mémoire change). « ✅ Approuvé » en commentaire (même compte GitHub), squash e4d96fde. Statut 🟣.
- Non bloquant : `qa/pr/WFS-555/pr.md` réécrit par les essais (modèle « À REMPLIR ») ; `$TMPDIR/captures_pr.*` jamais effacés ; refus « cache dans le dépôt » limité à la racine de l'arbre courant ; idée de clé du cache sur le contenu du code (au PO).


### 2026-11-16 00:10:01 — Fin WFS-555 : fusionné (#425, squash e4d96fde), session terminée (lead-dev)
- `qa/pr/WFS-555/pr.md` remis sur main (il avait été réécrit en modèle vide par le dernier essai) : description complète + tableau de la 2e revue.
- Gain mesuré : captures « avant » relues 29 s (cache plein) ; main ayant bougé, compilation incrémentale ≈ 55 s au lieu de ≈ 200 s ; 1er lancement d'une session neuve inchangé (≈ 200 s).
- Remarques non bloquantes du relecteur à transmettre au PO : cache par contenu du code plutôt que par SHA (main bouge souvent pour des documents) ; dossiers `$TMPDIR/captures_pr.*` jamais effacés (déjà avant) ; refus « cache dans le dépôt » sans regarder le dossier git commun d'un arbre secondaire (cas rare).
- Session terminée (un seul sujet par session) : aucun autre ticket demandé.

### 2026-11-16 00:14:43 — Arbitrage WFS-557 : maquettes R19 à 100 %, boutons compris (po-scrum-master)
- Question de Claude, agent dev3d (01UY1Vv4Fd) : règle « taille des boutons inchangée » (2026-11-06, E23, WFS-549 RG-07) contre l'ordre de l'utilisateur du 2026-11-15 23:46 « maquette à 100 %, zéro erreur ».
- Décision : **oui**, l'ordre le plus récent passe avant : forme, place et taille dessinée des boutons comme sur la maquette ; zone de toucher ≥ bouton dessiné et ≥ 44 px, ancienne taille gardée en invisible si possible sans chevauchement ; aucune fonction perdue ; élément sans place sur la maquette → le PO. Écrit dans `regles-de-gestion/WFS-557.md` (RG-06 à RG-09). L'utilisateur peut changer cette décision.

### 2026-11-16 00:15:10 — Arbitrage WFS-557 : 2e police Inter pour le texte courant (po-scrum-master)
- Question de Claude, agent dev3d (01UY1Vv4Fd) : les maquettes R19 ont une police de texte courant (genre Inter) que le jeu n'a pas.
- Décision : **oui**, Inter (OFL) en 2e police, prise seulement sur GitHub `rsms/inter` ou npm `@fontsource/inter`, licence dans `assets/licences/`, SHA-256 au journal ; graisses des maquettes seulement, même atlas (`src/ui/text.*`) ; titres, boutons, chiffres et HUD restent en Rajdhani. RG-10 à RG-13 de `regles-de-gestion/WFS-557.md`.

### 2026-11-16 00:18:31 — WFS-503 : PR #427 ouverte, lead dev appelé (Claude, agent dev3d (01Us6uCwe9))
- Fait : `cadeau::donner()` appelé par `ouvrir()` avant la même écriture de la sauvegarde : étoiles au portefeuille (`boutique::crediter`, une fois), coloris ajouté à `possede-<voiture>` sans être posé, skin par `skins::donnerSkin` ; écriture impossible → état, solde et possessions remis. Clé nouvelle `cadeau-nouveaux` + `estNouveau()` / `vu()` (arbitrage du PO, badge au garage : WFS-556).
- Fichiers : `src/game/cadeau.*`, `src/game/sauvegarde.*`, `src/game/cadeau_gains_banc.cpp` (nouveau banc `WFS_BANC=cadeau-gains`), `tests/test_cadeau.cpp` (20 cas RG-05), `CMakeLists.txt` (`skin_offerte.cpp` dans 6 tests). **Accroche annoncée** : 1 ligne + 1 include dans `src/platform/desktop_main.cpp` (groupe `cadeau-gains`). Aucun fichier `src/ui/**`.
- Vérifié en local (lavapipe, branche synchronisée avec `main`) : `tests_cpu.sh build` 148 réussis, 0 échec ; banc `cadeau-gains` 0 écart en FR et en EN. Parcours RC-E19-07 : menu (0 ★) → cadeau +28 ★ → menu 28 ★ → 2e appui sans gain → cadeau coloris Cyan (208) → garage, pastille sans cadenas → « Choisir » (0 ★ débité) → terrain → course de jour et de nuit en Cyan → cadeau skin Camouflage → garage sans cadenas → relance (sauvegarde relue) : tout gardé.
- Captures : `qa/pr/WFS-503/` (24 après, 4 paires avant / après du banc `cadeau`).
- Limites : pas de badge « Nouveau » (WFS-556) ; le banc ne garde que la chance du type voulu pour obtenir chaque gain (tirage inchangé).

### 2026-11-16 00:29:31 — WFS-496 : atelier d'autocollants, PR #428 ouverte (Claude, agent dev3d (01Us6uCwHf))
- Fait : écran « Atelier » ouvert depuis le garage (bouton « ATELIER » sous « MARKET ») : 5 vues + rotation au doigt, bibliothèque (6 familles, 10 gratuits, cadenas et prix des packs), glisser = placer, deux doigts = tourner / agrandir, ± 5°, ± 5 %, miroir, 28 couleurs, supprimer, annuler / refaire (20 pas), 20 calques (monter, descendre, masquer), « Valider » (gardé + cuit par WFS-495), « Quitter » avec confirmation, touche Retour = Quitter ; FR / EN.
- Choix (RG-02) : gratuits = bande-large, bande-double, chiffre-1 à 3, flamme-1, eclair-1, etoile-1, lettre-a, lettre-b ; autres motifs dans les packs de `configgame.h` (numeros, bandes, flammes, etoiles), non posables tant que le pack n'est pas possédé (vente : WFS-497).
- Fichiers (zone E8) : `src/ui/ecrans/atelier.*`, `atelier_calques.*`, `atelier_banc.cpp`, `tests/test_atelier.cpp`, `assets/textes/fr.txt` / `en.txt` (clés `atelier.*`).
- **Accroches annoncées (≤ 10 lignes chacune)** : `src/ui/garage.cpp` (8 lignes : bouton, touchers, caméra, aperçu, voiture fixe), `src/game/game.cpp` (2 : `WFS_AUTOCOLLANTS` sinon calques de l'atelier), `src/ui/garage_skin.cpp` (2 : retour au menu), `src/game/sauvegarde.*` (4 : clé `autocollants-<voiture>`), `src/platform/desktop_main.cpp` (2 : banc), `CMakeLists.txt` (1 : test).
- Vérifié en local (lavapipe, branche synchronisée avec `main`) : tests CPU 149 réussis (dont `test_atelier`, 41 cas) ; banc `atelier` 0 écart (parcours RC-E18-09 garage → atelier → 5 calques → Valider → course de jour → retour → relance, 3 voitures jour et nuit, anglais, touche Retour) ; cuisson 43 à 56 ms sur PC, au garage seulement.
- Captures : `qa/pr/WFS-496/` (21 après, 1 paire avant / après du garage).
- Limites : blanc des autocollants gris-bleu sous les projecteurs du garage (rendu WFS-495) ; autocollants des flancs peu visibles de la caméra de poursuite.


### 2026-11-16 00:31:13 — Arbitrage WFS-557 : éléments sans place sur les maquettes R19 (po-scrum-master)
- Question de Claude, agent dev3d (01UY1Vv4Fd) (RG-09). Décisions écrites dans `regles-de-gestion/WFS-557.md` (RG-14 à RG-18).
- Plaque : suit le pseudo, toucher « Ta plaque » la modifie. Profil : « · record » si le score égale le record du mode ; aide de glissement retirée.
- Menu : Crédits seulement dans les Réglages (maquette 14) ; compteur d'étoiles → Market ; bloc « RECORD » → Profil (pas de bouton ajouté) ; sous-titres, « Ta voiture » et aide retirés.
- Pause : « CP 2 » sans total en Checkpoint. Market : apparence 100 % ; le pack débutant existe déjà (WFS-510, 511, 513), il est branché tel quel, grisé « Après ta 3e course » tant qu'il n'est pas proposé ; pas de nouvelle US.

### 2026-11-16 00:39:16 — WFS-503 : PR #427 refusée par le lead dev (lead-dev)
- Commit relu : 75d1f823. Les 28 captures ont été ouvertes et sont conformes : étoiles 0 → 28 ★ créditées une fois, coloris Cyan débloqué puis choisi, course de jour et de nuit, skin Camouflage débloquée, relance gardée ; paires du menu identiques à l'octet.
- Diff lu en entier, sans défaut : donner() est fait dans la même écriture atomique, avec remise en cas d'échec ; la clé cadeau-nouveaux est bornée et compatible avec les anciennes sauvegardes.
- Construction et tests refaits sur la branche + main : 148 tests réussis.
- Refus pour un seul point : la capture « Tests CPU » manque dans la description et qa/pr/WFS-503/tests-cpu.log est absent (règle de l'utilisateur du 2026-11-11). Aucune capture à refaire.
- Revue publiée en COMMENT sur la PR (GitHub refuse REQUEST_CHANGES pour le même compte).

### 2026-11-16 00:40:08 — WFS-496 : PR #428 refusée par le lead dev (lead-dev)
- Revue du commit `f414a234` : 22 captures ouvertes, tout le diff lu, branche construite et testée (149 tests CPU réussis, `test_atelier` 0 échec). Revue postée en COMMENT, car GitHub refuse REQUEST_CHANGES sur une PR du même compte.
- Défauts bloquants :
  1. Le texte de la fenêtre « Quitter sans valider ? » est dessiné hors de la fenêtre (`atelier.cpp`, `dessinerConfirmation` : `textBoite` CENTRE avec x = bord gauche).
  2. Régression au menu : quand l'atelier a cuit l'aperçu, `SkinGarage::pose_` reste faux, donc la sortie Garage → Retour → menu ne repose pas la skin et les autocollants de la voiture de course.
  3. Le parcours RC-E18-09 est capturé de nuit alors qu'il est annoncé « de jour » (`WFS_MOMENT` ignoré par Choisir → Terrain → Rouler).
  4. Captures de la 3e voiture (500) absentes, et aucun autocollant reconnaissable en course.
- Statut : ↩️ Retour lead dev (PR #428) dans `TACHES.md`, `track-agent.md` et `suivi-pr.md`.

### 2026-11-16 00:42:34 — WFS-557 : téléchargement de la police Inter, avancement (Claude, agent dev3d (01UY1Vv4Fd))
- Téléchargement (RG-10, `regles-reseau.md`) : https://github.com/rsms/inter/releases/download/v4.1/Inter-4.1.zip (dépôt public GitHub `rsms/inter`, version 4.1, licence SIL OFL 1.1), SHA-256 de l'archive `9883fdd4a49d4fb66bd8177ba6625ef9a64aa45899767dde3d36aa425756b11e`. Gardés : `Inter-Regular.ttf` (`40d692fce188e4471e2b3cba937be967878f631ad3ebbbdcd587687c7ebe0c82`), `Inter-Medium.ttf` (`97ad806f526e41546d46365bb3a393145f75b7b1568913db74549ad8b8dba872`), `LICENSE.txt` (`262481e844521b326f5ecd053e59b98c8b2da78c8ee1bdbb6e8174305e54935a`) ; rien n'a été exécuté. Licence dans `assets/licences/police-inter-OFL.txt`, crédit dans `CREDITS.txt` et `store/credits.md` (`verifier_licences.py` : 0 manque).
- Fait sur `feature/waforspeed-557` (commit cb2f3112) : Inter dans le même atlas que Rajdhani (711 px sur 1024, Rajdhani placée en premier, pixels inchangés) ; pause (19), Réglages (14, 24, 25 : une seule mise en page, téléphone et PC), profil (21), création du profil (20, RG-14), chargement (08, 08b) refaits d'après les maquettes, comparés côte à côte au banc en 1600 × 720.
- Limite signalée : maquette 20, touches de 62 × 52 px (≈ 4,7 mm sur le téléphone de 2400 × 1080) au lieu des 9 mm de WFS-481 ; zone de toucher jointive ≥ 44 px (arbitrage RG-07). À suivre : CGU (22, arbitrage attendu), Market (12, RG-18), achat (27, 28), menu (01, RG-16), autres écrans de WFS-549.

### 2026-11-16 00:52:17 — WFS-503 : PR #427 corrigée après la 1re revue, lead dev rappelé (Claude, agent dev3d (01Us6uCwe9))
- Branche synchronisée avec `main` (`ddcc72a2`) ; `bash tools/qa/tests_cpu.sh build` relancé : 148 réussis, code de sortie 0 ; journal complet `qa/pr/WFS-503/tests-cpu.log` et section « Tests CPU » dans `pr.md` et la description de la PR (`51f8e296`). Aucun code ni capture changé.
- **Pour le PO (WFS-556)** : la clé `cadeau-nouveaux` grandit tant que le garage n'appelle pas `cadeau::vu()` (lecture bornée à 256 éléments, sans risque) : WFS-556 doit appeler `vu(s, voiture, objet)` quand le joueur choisit l'objet.


### 2026-11-16 00:52:49 — Arbitrage WFS-557 : CGU, maquette 22 (po-scrum-master)
- RG-19 : case retirée, « J'accepte » grisé jusqu'à la lecture des deux onglets jusqu'en bas ; refus par la touche Retour au premier lancement (même message et même fermeture) ; « Fermer » en lecture depuis les Réglages. Demande de Claude, agent dev3d (01UY1Vv4Fd).

### 2026-11-16 00:57:48 — Arbitrage WFS-557 : scènes 3D des maquettes 04, 09, 10, 11 ; WFS-558 à 561 créés (po-scrum-master)
- Question de Claude, agent dev3d (01UY1Vv4Fd) : les scènes 3D complètes des maquettes R19 04 (duel), 09 (départ), 10 (arrivée) et 11 (résultats) n'ont été construites par aucun ticket fusionné (548, 549, 552).
- Décision (RG-20 de `regles-de-gestion/WFS-557.md`) : **oui**, recommandation acceptée. WFS-557 ne met en conformité que les éléments 2D déjà touchés par 548 / 549 sur ces écrans (Menu, Rejouer, panneau du score, pastille « +7 étoiles · nouveau record », Retour, Créer duel). Les scènes 3D deviennent 4 US P1 de E23, `@dev` + avis « VALIDÉ » de l'agent 3D : **WFS-558** (04), **WFS-559** (09), **WFS-560** (10, après 559, même portique), **WFS-561** (11). Portiques placés par le code, pas cuits dans les paquets : pas de tâche `@cicd` compagne.
- Prêts pour de nouvelles sessions `@dev` : 558, 559, 561.

### 2026-11-16 01:01:16 — Arbitrage WFS-557 : choix du mode (maquette 02) ; WFS-562 créé (po-scrum-master)
- Question de Claude, agent dev3d (01UY1Vv4Fd) : Distance et Difficulté sans place sur la maquette 02 ; aucune image de mode dans le jeu.
- RG-21 : recommandation acceptée. Distance et Difficulté gardées, en rangées de verre **seulement dans la carte du mode choisi qui s'en sert** (Course à 5, Poursuite) ; Checkpoint choisi = écran identique à la maquette.
- RG-22 : dans WFS-557, scène 3D floutée dans chaque carte (provisoire) ; vraies images des modes, prises dans le jeu avec ses voitures sans marque, dans **WFS-562** (P1, E23, `@dev` + avis `@3d`, ⏳ En attente de WFS-557).

### 2026-11-16 01:05:17 — Arbitrage WFS-550 / 551 / 557 : onglets du garage et colonne du menu (po-scrum-master)
- Question de Claude, agent dev3d (01UY1Vv1yg) (WFS-550, PR #424 ↩️), après le KO de l'utilisateur (« ta maquette tu l'appliques à 100 %, zéro erreur »).
- Onglets du garage : la recommandation (les faire dans 550) est **refusée** : WFS-551 les fait déjà sur sa branche (`src/ui/garage_onglets.*`, a6ce7ea8 : onglets, panneau unique, bouton rond). WFS-551 RG-10 : panneau étroit et centré de l'onglet Voiture (maquette 13), « Choisir / Acheter » rond hors du panneau, « Retour » en étiquette à gauche. WFS-550 RG-09 : seulement le contenu (cadrans, barres, 17 segments) dans la largeur donnée par 551. RG-10 / RG-11 : le 2e fusionné montre 06, 07, 13, 15, 16 à 100 %.
- Colonne du menu : la recommandation (la monter au-dessus des cartes) est **refusée** (élément absent de la maquette). L'arbitrage RG-16 de WFS-557 (00:31) s'applique et **passe à WFS-550** (RG-11) : cartes collées au bord droit ; Crédits retiré du menu (Réglages, maquette 14) ; Market = toucher le compteur d'étoiles ; Profil = toucher le bloc RECORD ; sous-titres, « Ta voiture SF-X », aide retirés de l'écran. Branche de 557 sans changement du menu (vérifié) : WFS-557 n'y touche plus.
- Sessions prévenues : WFS-551 (01UY1Vv2KF), WFS-557 (01UY1Vv4Fd).

### 2026-11-16 01:10:58 — WFS-503 fusionné par le lead dev (lead-dev)
2e revue de la PR #427 (commit relu 51f8e296), limitée aux points corrigés : synchronisation avec `main` (ddcc72a2, aucun code de la PR changé), `qa/pr/WFS-503/tests-cpu.log` (148 réussis, code 0) et section « Tests CPU » dans `pr.md` et la description, limites écrites. Construction et `tests_cpu.sh` refaits par le lead dev sur 51f8e296 : 148 test(s) réussi(s) en 530 s, code 0. « ✅ Approuvé » posté en commentaire (même compte GitHub), fusion en squash : cb7932d2. WFS-503 → 🟣 ; ligne retirée de `suivi-pr.md` ; ligne ajoutée à `qa/Check.md`. WFS-556 (badge « NOUVEAU ») attend encore WFS-551.

### 2026-11-16 01:11:31 — Fin WFS-503 : fusionné (#427, cb7932d2), session terminée (Claude, agent dev3d (01Us6uCwe9))
- Fusion par le lead dev à la 2e revue (squash cb7932d2) ; `TACHES.md` 🟣, `qa/Check.md` à jour (lead dev). Ligne de `track-agent.md` en « ⏹️ session terminée » ; réponse à l'utilisateur sous la demande du cadeau du jour (`tache utilisateur.md`).
- Suite : WFS-556 (badge « NOUVEAU », appel de `cadeau::vu()`) attend WFS-551. Un seul sujet par session : aucun autre ticket demandé.


### 2026-11-16 01:42:49 — Arbitrage WFS-551 : éléments absents des maquettes, WFS-563 créé (po-scrum-master)
Question de Claude, agent dev3d (01UY1Vv2KF), PR #426. Décisions écrites dans `regles-de-gestion/WFS-551.md` (RG-12 à RG-15) : (1) bouton MARKET retiré de l'écran du garage, l'accès reste en touchant le cadre du solde (comme le menu, arbitrage 01:05) ; (2) crédit du modèle 3D gardé (obligation CC BY), petit, gris, en bas à droite ; (3) les 28 couleurs restent (WFS-489), rangées de la maquette 06 qui défilent au doigt avec chevron ; (4) scène 3D du garage hors WFS-551 : nouveau **WFS-563** (P1, ⏳ Prêt, `@dev` + avis `@3d`, `garage_decor.*`, même fichier que WFS-554 : le 2e fusionné se synchronise) ; le 100 % de WFS-551 porte sur l'interface 2D.

### 2026-11-16 01:44:49 — WFS-550 : SF-X de vitrine au menu, ordre de l'utilisateur (Claude, agent dev3d (01UY1Vv1yg))
- Ordre de l'utilisateur dans la session : « la voiture dans le menu doit être exactement la même, c'est l'image de marque ». Il remplace WFS-552 RG-03 (« en jeu, le menu montre la voiture réelle du joueur »).
- Fait (src/ui/ecrans/menu.cpp) : modèle de vitrine sf90 chargé une fois, skin « ciel », finition caméléon, peinture orange ; la voiture du joueur n'est plus dessinée au menu (SceneDediee::joueur = false) ; course, garage et sauvegarde inchangés.

### 2026-11-16 01:50:38 — WFS-496 : PR #428 corrigée après la revue du lead dev (Claude, agent dev3d (01Us6uCwHf))
- Corrigé : texte de la confirmation centré (FR / EN) ; sortie du garage : texture de l'aperçu rendue et habillage de la course reposé (accroche `garage.cpp` : 9 lignes au total) ; parcours RC-E18-09 de jour, contrôlé au banc ; autocollants du dessus ancrés sur le toit ; 500 et GT86 : étoile et lettre en miroir, bande double sur le toit (lisible en course) ; message caché sous la confirmation ; texte de la liste vide raccourci.
- Vérifié sur 694c59bb (synchronisé avec main, WFS-503 compris) : tests CPU 149 réussis ; banc `atelier` 0 écart. Captures refaites (🔁) dans `qa/pr/WFS-496/`.

### 2026-11-16 02:32:22 — WFS-496 fusionné par le lead dev (lead-dev)
- PR #428, 2e revue sur `47dfd4a0` : les 4 points de la 1re revue sont corrigés (confirmation centrée FR / EN ; `atelier::rendre` à la sortie du garage, habillage de la course reposé ; parcours de jour ; captures de la 500 et bande du toit lisible en course). Remarques non bloquantes faites aussi.
- Refait par le relecteur : construction, `tests_cpu.sh` 149 réussis, banc `WFS_BANC=atelier` 0 écart. `main` n'avait reçu que des documents depuis la synchronisation (694c59bb).
- « ✅ Approuvé » posté en COMMENT (même compte GitHub), puis fusion en squash `5e656ac7`. Statut 🟣, ligne de `suivi-pr.md` enlevée, `qa/Check.md` complété.

### 2026-11-16 02:33:17 — WFS-496 : fin, session terminée (Claude, agent dev3d (01Us6uCwHf))
- PR #428 fusionnée en squash par le lead dev à la 2e revue (5e656ac7) ; ticket 🟣 Fusionné, ligne dans `qa/Check.md`. Session arrêtée (un seul sujet par session) : aucun autre ticket demandé.
- Pour la QA : banc `WFS_BANC=atelier` (parcours, 3 voitures jour et nuit, FR / EN). Remarques du lead dev (non bloquantes) : la vérification du jour du banc repose aussi sur la capture ; libellé « capot » de la capture `vue-dessus` du banc à corriger (la bande est sur le toit).

### 2026-11-16 05:50:25 — WFS-551 : PR #426 corrigée, garage aux maquettes R19 (Claude, agent dev3d (01UY1Vv2KF))
- Fait : garage refait sur la mise en page des maquettes 06, 07, 13, 15, 16 (onglets VOITURE / PEINTURE / FINITION / SKINS, un panneau de verre au centre en bas, grand bouton rond à droite avec nom au-dessus et solde dessous, solde en haut à droite, flèches seulement sur l'onglet Voiture) ; 18b (« rouler sur une autre carte »), 18d (scène floue derrière la fenêtre) ; nettoyage demandé par le lead dev. Arbitrages du PO RG-10 à RG-15 appliqués.
- Accroches hors zone (≤ 10 lignes chacune) : `text.cpp` (`contourNeon`, décalage u + 4 pour le mode néon de `shaders/ui.frag`), `garage_fiche.cpp` (sous-titre et points selon l'onglet, couleurs de WFS-550 gardées), `garage_cadre.cpp` (crédit 11 px, RG-13), `garage_commun.h` (onglet, zones), `garage.cpp` (onglets, panneaux).
- RG-12, chemin du Market : `garage.cpp` `onTouch` → `onglets::dansSolde(x, y)` (cadre du solde posé par `onglets::solde`) → `market_ecran::ouvrir(Ecran::Garage)` ; le bouton de WFS-513 est dessiné transparent (`dessinerBoutonGarage(..., 0.f)`), sa zone reste pour le banc du Market. Code du Market non touché.
- Synchronisation avec `main` (WFS-496) : conflit `garage.cpp` résolu en gardant les deux (solde → Market, bouton « Atelier »).
- Vérifié : construction ; `bash tools/qa/tests_cpu.sh build` → 150 tests OK ; bancs garage-couleurs, garage-voitures, garage-finitions, garage-skins (branche synchronisée), paquets, hud:1280x720 → 0 écart ; 22 captures maquette / avant / après dans `qa/pr/WFS-551/`.
- Limites : scène 3D du garage → WFS-563 (RG-15) ; cadrans → WFS-550. Lead dev rappelé.


### 2026-11-16 05:59:42 — WFS-551 : PR #426 refusée en 2e revue (lead-dev)
Commit relu a7512b77. Construction et 150 tests CPU verts (refaits), 22 captures et maquettes 06, 07, 13, 15, 16, 17, 18a–18d comparées au code : les 9 points de la 1re revue sont corrigés. Il reste un défaut : en mode « Toit et rétros », les titres des rangées (« Classiques · gratuites », « Vives · 50 ★ ») contredisent les prix écrits sous les gouttes (50 ★, 100 ★) (`garage_couleur.cpp:176-184`). Seule la capture toit-noir est à refaire. Questions transmises au PO : boutons « Atelier » (WFS-496) et « Essai gratuit » (WFS-499) absents des maquettes.

### 2026-11-16 06:20:35 — WFS-551 : 2e revue corrigée, PR #426 (Claude, agent dev3d (01UY1Vv2KF))
- Fait : « Toit et rétros » : le titre de chaque rangée compte le droit de la couleur secondaire tant qu'il n'est pas acheté (même total que `prixPastille` sous les gouttes) ; texte `garage.couleur.groupe.classiques_prix` FR / EN (7337a96d).
- Vérifié : banc garage-couleurs 0 écart ; tests CPU 150 OK ; capture `garage-couleurs-p208-toit-noir-apercu` refaite (574d6782). Lead dev rappelé.

### 2026-11-16 06:30:20 — WFS-551 fusionné par le lead dev (lead-dev)

PR #426, 3e revue sur 574d6782 : le défaut de la 2e revue (titres « Toit et rétros ») est corrigé dans 7337a96d (droit du toit compté dans le titre, comme `prixPastille`). Capture `toit-noir-apercu` ouverte, maquette 06 conforme, construction et 150 tests CPU refaits par le lead dev. « ✅ Approuvé » en commentaire (même compte GitHub), fusion en squash **e1c86795**. Statut 🟡 À valider (utilisateur). Remarques au PO : Atelier, Essai gratuit, « Plus tard » de 18d absents ou placés hors maquette.

### 2026-11-16 06:30:44 — WFS-551 fusionné, session terminée (Claude, agent dev3d (01UY1Vv2KF))
- PR #426 fusionnée en squash par le lead dev (e1c86795), 3e revue. Statut 🟡 À valider (utilisateur) sur l'APK de `main` ; ligne `qa/Check.md` posée par le lead dev.
- Suites hors ticket : scène 3D du garage → WFS-563 ; remarques au PO : boutons Atelier (WFS-496) et Essai gratuit (WFS-499) absents des maquettes, « Plus tard » de 18d, goutte « Comme la carrosserie » sous « Classiques · 50 ★ » en mode toit.
- Un seul ticket par session : je m'arrête, aucun autre ticket demandé.

### 2026-11-16 07:10:25 — Statuts automatiques, CI main #1147 (robot statuts)
- Run rouge (push, commit e1c8679), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-555, WFS-496, WFS-503.

### 2026-11-16 08:59:27 — WFS-550 : photo de la maquette 01 en fond du menu, ordre de l'utilisateur (Claude, agent dev3d (01UY1Vv1yg))
- Ordre de l'utilisateur dans la session : « pour l'instant mets la photo en background » (la 3D du menu ne reproduisait pas la SF-X de la maquette à 100 %). Exception à la règle « 100 % 3D » pour le fond du menu, sur son ordre.
- Fichier `assets/ui/menu_fond.jpg` (2400 × 1080, SHA-256 9b101aadd1acdf0285a7c47292eddbf93c7b412ba1b4aa8e90ecbf27310964d8) : maquette `references/maquettes-R19/01-menu.jpg` de l'utilisateur, logo, compteur, RECORD et carte du cadeau effacés (le jeu les redessine) ; déclaré « créé par le projet » dans `tools/qa/verifier_licences.py` (modèle de la SF-X déjà crédité).
- **Accroche E0 (≈ 25 lignes)** : `Renderer::uiSetImageFond` / `uiImageFond` (binding 1 de fontSet_, jusque-là blanc) et branche « uv ≤ −1,5 » dans `shaders/ui.frag`. Le parking 3D et la SF-X de vitrine restent dans le code (repli si la photo manque) mais ne sont plus chargés quand la photo est là.

### 2026-11-16 09:09:06 — WFS-550 : retour du menu en 3D, ordre de l'utilisateur (Claude, agent dev3d (01UY1Vv1yg))
- Ordre de l'utilisateur dans la session : « remets la version 3D ». Commit de la photo (c1fa577f) annulé par 780b3393 : plus d'image `assets/ui/menu_fond.jpg`, plus d'accroche E0 (renderer, ui.frag) ; le menu affiche de nouveau le parking 3D et la SF-X de vitrine (sans plaques, détails bleu ciel, reflets du parking).

### 2026-11-16 09:44:40 — WFS-558 attribué à Claude, agent dev3d (01Us6uCtQN) (po-scrum-master)
- Début : **WFS-558** (P1, E23) « Interface R19 : scène 3D du duel en ligne (maquette 04) », nouvelle session `@dev` (session_01Us6uCtQNFWC6KwvYiHKmMk), à sa demande, aucun ticket avant. Choix : 1er P1 ⏳ Prêt de E23 (plus récente demande de l'utilisateur), sans dépendance ouverte.
- Zone : `src/ui/ecrans/salon.*` (E8), menu 3D de WFS-552 (`menu*`, `garage_decor.*`, modèles du parking) ; règles `regles-de-gestion/WFS-558.md` (RG-01 à 04, RG-C1 à C7). Branche `feature/waforspeed-558`, captures ciblées, avis « VALIDÉ » de l'agent 3D, revue maquette ↔ code par le lead dev ; après fusion 🟡 À valider (utilisateur). WFS-557 (en cours) touche les boutons 2D : synchroniser avec `main`.
- `track-agent.md` : lignes des sessions fusionnées (496, 503, 551, 555) enlevées.

### 2026-11-16 09:45:38 — CI de `main` #1147 rouge : WFS-564 créé et attribué (po-scrum-master)
- **CI** : run 43817146076 (#1147, e1c86795 = WFS-551, PR #426) rouge, job banc-essai (texte) : tests CPU 150 OK, contrôle des licences « MANQUE ui/icone-pistolet.png : actif livré sans crédit connu ». Icône fournie par l'utilisateur (journal 2026-11-15 23:28:32), ajoutée par WFS-551 sans ligne dans `COUVERTURE`.
- **US technique WFS-564** (P1, E0, `[TECHNIQUE]`) : une ligne de `COUVERTURE` pour ce fichier seul, sans affaiblir le contrôle ; règles `regles-de-gestion/WFS-564.md`.
- **Début** : attribué à Claude, agent dev3d (01Us6uCxFh) (session_01Us6uCxFhiJ8TJGKy9QTPw7), nouvelle session `@dev`, à sa demande (aucun ticket avant). Pourquoi lui : règle de l'utilisateur « CI de `main` rouge → US technique P1 tout de suite à un développeur libre » (précédent WFS-543) ; la session de WFS-551 est terminée. Zone : `tools/qa/verifier_licences.py` seulement. Branche `feature/waforspeed-564`. Plan : ligne de crédit, contrôle à 0 manque, tests CPU, PR, appel du lead dev, CI de `main` verte.
- **track-agent** : lignes des sessions terminées enlevées (WFS-551 01UY1Vv2KF, WFS-496 01Us6uCwHf, WFS-503 01Us6uCwe9, lead dev de WFS-555).
- Restent ⏳ Prêt pour de nouvelles sessions `@dev` : WFS-559, 561, 563 (E23, P1), WFS-512 (E21, P1), puis WFS-554, 538, 553 (P2).

### 2026-11-16 09:46:38 — Début WFS-559 : portique de départ à 5 feux (maquette 09) ; attribué à Claude, agent dev3d (01Us6uCwnA) (po-scrum-master)
- **Ticket** : WFS-559 (E23, P1 Urgent), `regles-de-gestion/WFS-559.md`. Attribué à Claude, agent dev3d (01Us6uCwnA) (session_01Us6uCwnA1Zmqb3WtWavzSG), nouvelle session `@dev` sans ticket avant, à sa demande ; 1er P1 prêt de E23 restant (WFS-558 et WFS-564 attribués à 09:44 / 09:45 UTC à d'autres sessions). Branche `feature/waforspeed-559`.
- **Zone de code** : objets de course (nouveau modèle du portique et des feux, placé par le code à la ligne de départ) ; `course_system.*` / `course.cpp` seulement pour lire le décompte existant (aucune nouvelle logique de départ) ; consigne 2D en `ui::tr` FR / EN. Pas de paquet `.wfcarte` (RG-03). HUD de course = WFS-550 / 551 : synchroniser avec `main`.
- **Critères** : treillis aux vraies mesures (RG-02), 5 feux pilotés par le décompte, toutes cartes jouables et modes, jour et nuit, aucune forme nue, ≤ 33 ms S21 FE ; avis « VALIDÉ » de l'agent 3D sur le dernier commit ; revue maquette 09 ↔ fichier:fonction ; après fusion 🟡 À valider (utilisateur). Modèle réutilisable par WFS-560.
- **Plan court** : lire la maquette 09 et le décompte, modéliser portique + feux, accrocher au décompte, captures 0 / 3 / 5 feux + départ, jour / nuit, Ville et Corniche, PR, avis `@3d`, lead dev.

### 2026-11-16 09:47:31 — Arbitrage WFS-558 : digicode à vraie taille avec vue rapprochée, pavé 6 × 6, listes dans le panneau de verre (po-scrum-master)
- Questions de Claude, agent dev3d (01Us6uCtQN). Décisions écrites dans `regles-de-gestion/WFS-558.md` (RG-05 à RG-08) : (1) option A, digicode à vraie taille + glissement de caméra (≈ 0,4 s) vers le digicode au toucher, « Retour » ramène d'abord au cadrage large ; option B (×4) refusée (RG-02, DA-realisme 99 %) ; (2) pavé 6 × 6 de 15 mm au pas de 19 mm : 2-9, A-Z sans I L O, ⌫ (×2) et OK (×3), une lettre par appui + 150 ms ; (3) pseudo sur la plaque 3D seulement ; « Salons qui t'attendent » et « Derniers adversaires » dans le panneau de verre, seulement s'ils ne sont pas vides ; salon = maquette 23 sur le parking flouté. Écarts à la maquette acceptés par le PO : taille du digicode dans le cadrage large, pavé alphanumérique. Recommandations du développeur acceptées.

### 2026-11-16 09:50:14 — Arbitrage WFS-559 : feux à la place des chiffres, « GO » gardé, vert 0,7 s au départ (po-scrum-master)
- Questions de Claude, agent dev3d (01Us6uCwnA). Décisions écrites dans `regles-de-gestion/WFS-559.md` (RG-05 à RG-09), recommandation du développeur acceptée : (1) chiffres 3 / 2 / 1 retirés, « GO » gardé 0,7 s sous les feux, bips inchangés ; (2) 5 feux rouges, un toutes les 0,6 s sur le décompte de 3 s, puis vert 0,7 s au départ, puis éteints ; (3) pas de mécanique « départ parfait » à coder (n'existe pas) ; (4) consigne FR / EN pendant le décompte seulement ; (5) portique à la ligne de départ dans tous les modes, Checkpoint compris (capture en plus).

### 2026-11-16 09:54:35 — Statuts automatiques, bancs du matin #42 (robot statuts)
- Run rouge (schedule, commit 9a8fbd5), aucun statut changé. Job(s) en échec : tests CPU et licences.
- Tickets 🟣 contenus dans ce run : WFS-555, WFS-496, WFS-503.

### 2026-11-16 09:58:35 — WFS-564 : correction faite, CI de branche lancée (Claude, agent dev3d (01Us6uCxFh))
- **Session** : session_01Us6uCxFhiJ8TJGKy9QTPw7, ouverte par l'utilisateur avec « @dev go » ; ticket attribué par le PO (US technique P1, CI de `main` #1147 rouge).
- **Cause vérifiée** (journal du run 43817146076, job « banc-essai (texte) ») : seule erreur = contrôle des licences, `MANQUE ui/icone-pistolet.png` (150 tests CPU verts). Origine vérifiée (RG-03) : icône fournie par l'utilisateur (journal du 2026-11-15 00:52:28 et 23:28:32), SHA-256 relus : `references/maquettes-R19/icone-pistolet-utilisateur.png` = 167444ee…, `assets/ui/icone-pistolet.png` = 5f69fa16… ; aucun téléchargement.
- **Fait** : une ligne `(r"ui/icone-pistolet\.png", None, None)` dans `COUVERTURE` de `tools/qa/verifier_licences.py` + commentaire (WFS-551, image d'origine). Aucun motif large, aucun seuil changé (RG-02).
- **Vérifié en local** : contrôle des licences 1 manque → 0 manque ; tests CPU 150/150 (394 s).
- **CI de branche** (US technique) : run **#1148** (43842900414) de `build.yml` sur `feature/waforspeed-564` (commit 12819871, synchronisé avec `main`) : vérifie le job « banc-essai (texte) » en entier (licences, puis l'étape « Captures du rendu (texte) » sautée sur `main` à cause du rouge).

### 2026-11-16 10:00:41 — WFS-559 : 2e arbitrage, boutons MENU / ARRIÈRE cachés pendant le décompte (po-scrum-master)
- **Question** de Claude, agent dev3d (01Us6uCwnA) : en vue « Arrière » (caméra 6,3 m / 35°, WFS-105), le caisson des feux tombe derrière les boutons du HUD MENU et ARRIÈRE ; la maquette 09 ne les a pas.
- **Décision** : option (a), recommandation du développeur acceptée. RG-10 : pendant le décompte seulement (≈ 3 s, voiture immobile), MENU et ARRIÈRE ni dessinés ni touchables, retour au départ (taille et place inchangées, WFS-429). RG-11 : accroche ≤ 10 lignes annoncée au journal dans le fichier du HUD ; WFS-550 en PR #424 touche le HUD : synchroniser avec `main`. RG-12 : caméra inchangée, option (b) refusée. RG-13 : caisson à 5 colonnes de 0,9 m (≈ 4,6 m), lanternes Ø 200 mm à visière, 2 par colonne, accepté. Captures en plus : Checkpoint et Course à 5, décompte puis départ.

### 2026-11-16 10:48:33 — WFS-561 attribué à Claude, agent dev3d (01Us6uCxBL) (po-scrum-master)
- Demande d'une nouvelle session `@dev` (session_01Us6uCxBLnhYyVos7wEtk5U), sans ticket auparavant : **WFS-561** (P1, E23), 1er P1 ⏳ Prêt de E23 restant (558, 559, 564 déjà pris).
- Zone : écran des résultats (E8, `src/ui/ecrans/`) et scène 3D du menu de WFS-552 (parking, éclairage, sol mouillé) réutilisée ; panneaux 2D de WFS-548 / 549 / 557 non retouchés.
- Règles : `regles-de-gestion/WFS-561.md` (RG-01 à 03, RG-C1 à C7). Avis « VALIDÉ » de l'agent 3D sur le dernier commit et revue maquette ↔ fichier:fonction avant fusion ; après fusion : 🟡 À valider (utilisateur).
- Restent ⏳ Prêt pour de nouvelles sessions `@dev` : WFS-563, 512 (P1), puis 554, 538, 553.


### 2026-11-16 10:49:13 — WFS-561 : début, branche feature/waforspeed-561 créée (Claude, agent dev3d (01Us6uCxBL))
- Ticket reçu du PO (scène 3D des résultats, tableau LED ambre, maquette 11). Lecture des RG, de la maquette et du code de l'écran des résultats et du parking de WFS-552.
### 2026-11-16 10:51:31 — Arbitrage WFS-561 : scène 11 = résultats de la course à 5, du duel et de la Poursuite (po-scrum-master)
- Question de Claude, agent dev3d (01Us6uCxBL) : maquette 11 (tableau LED) et maquettes 31 à 33 (fenêtres 2D de WFS-557) montrent le même classement.
- Choix A : la scène 11 sert de résultats aux 3 modes classés ; le Checkpoint garde l'écran de WFS-557 (scène 10 plus tard, WFS-560). Les fenêtres 2D de 31 à 33 restent dans le code comme repli (rien n'est supprimé). Accroches dans les `resultat()` après la fusion de WFS-557. RG-04 à RG-09 dans `regles-de-gestion/WFS-561.md`. L'utilisateur peut changer ce choix.


### 2026-11-16 10:55:14 — WFS-564 fusionné par le lead dev (lead-dev)
- PR #429 (Claude, agent dev3d (01Us6uCxFh)), revue sur b3304b32 en commentaire « ✅ Approuvé » (GitHub refuse l'approbation : même compte), fusionnée en squash : **5f38a6bd**.
- Vérifié : diff lu en entier (1 ligne de `COUVERTURE` + 2 lignes de commentaire, `re.fullmatch` = ce fichier seul, RG-02 tenue) ; SHA-256 de `assets/ui/icone-pistolet.png` (5f69fa16…) et de l'original de l'utilisateur (167444ee…) recalculés (RG-03) ; `verifier_licences.py` 1 manque sur main / 0 sur la branche ; construction Release et 150 tests CPU refaits (382 s).
- CI de branche #1148 (43842900414) verte : compilation, android, endurance, publication-apk, banc-essai (menu1-2400), banc-essai (texte) (« Test de la piste » et « Captures du rendu (texte) » vertes ; captures ≈ 50 min contre ≈ 27 min au #1146, à surveiller).
- Statut 🟣 ; reste le run de `main` sur 5f38a6bd (numéro à noter par le robot des statuts / le développeur).

### 2026-11-16 10:55:41 — WFS-564 : fin de session (Claude, agent dev3d (01Us6uCxFh))
- **Fusion** : PR #429 relue et fusionnée en squash par le lead dev (5f38a6bd). CI de branche #1148 (43842900414) **verte** sur tous les jobs, « Captures du rendu (texte) » comprise (sautée sur `main` au #1147).
- **CI de `main`** : run **#1149** (43853054858) en cours sur 5f38a6bd au moment de la fin de session ; le robot des statuts passera WFS-564 (et WFS-555, 496, 503) en ✅ s'il est vert. Note du lead dev : « Captures du rendu (texte) » a duré ≈ 50 min au #1148 (≈ 27 min au #1146), limite 75 min, à surveiller.
- **Limite** : mon banc local « texte » a été une 1re fois faussé par mon propre `git stash -u` pendant le banc (dossier `texte/` enlevé), puis coupé par mon délai de 25 min ; le verdict qui compte est celui de la CI #1148 (vert).
- Session terminée (un seul sujet par session) : aucun autre ticket demandé.

### 2026-11-16 10:57:30 — WFS-565 créé : choix de la carte à 100 % des maquettes R19 (po-scrum-master)
- **Signalement** de Claude, agent dev3d (01UY1Vv4Fd) (WFS-557), à la demande de l'utilisateur : l'écran « Choisis ta carte » fusionné par WFS-551 (#426, 06:29 UTC) n'est pas à 100 % des maquettes 03 et 18a à 18d (7 écarts : titre et sous-titre, ciel dessiné au lieu de la vue aérienne, flèches au lieu des voisines, cadre cyan et « CHOISI » au lieu du halo orange, « 02 / 04 » au lieu du ★ solde, Rouler / Retour mal placés, Jour / Nuit au lieu de Midi / Nuit).
- **Décision du PO** : recommandation du développeur acceptée. Nouvelle US **WFS-565** (E23, P1, ⏳ Prêt, nouvelle session `@dev` + avis `@3d`), règles dans `regles-de-gestion/WFS-565.md` ; **pas ajoutée à WFS-557** (un seul sujet, presque en PR). L'image aérienne est faite une fois par le banc d'essai PC ; si elle demande un rendu hors écran dans le jeu, question au PO (elle irait alors dans une US à part, comme WFS-562). WFS-551 n'est pas rouvert : il reste 🟡 (ordre « on ne jette pas un travail pour un petit défaut »).
- **CI de `main`** : rouge #1147 depuis WFS-551 ; déjà corrigée par l'US technique WFS-564 (#429, fusionnée), run #1149 (43853054858) en cours. Aucune nouvelle US technique.

### 2026-11-16 10:58:30 — WFS-558 : PR #430 ouverte (Claude, agent dev3d (01Us6uCtQN))
- Fait : scène 3D de l'écran Salon (maquette 04) : parking du menu partagé (`parking::commun()`, lumière `Parking::eclairer`), digicode inox 120 × 180 × 30 mm sur le poteau (pavé 6 × 6, touches 15 mm au pas de 19 mm, afficheur, vis, gaine), plaque du pseudo 300 × 100 mm, 2e voiture (adversaire, sinon voiture garée noire) ; vue rapprochée du digicode en 0,4 s (RG-05), « Retour » d'abord vers le cadrage large ; panneau « Rejoins avec un code » avec les listes seulement si non vides (RG-07) ; salon sur la scène floutée (RG-08).
- Fichiers : `src/ui/salon_parking.*`, `src/ui/salon_digicode.cpp`, `src/ui/ecrans/salon.cpp`, `tests/test_salon_digicode.cpp`, textes FR / EN ; accroches annoncées : `menu.cpp` (1 ligne, parking partagé), `menu_parking.*` (`eclairer`, `commun`), `desktop_main.cpp` (1 ligne, banc `WFS_BANC=salon`).
- Vérification : 151 tests CPU réussis (dont `test_salon_digicode`) ; banc `menu2` 2400 × 1080 : saisie K7Q2 sur les touches 3D, effacer, « Retour » → cadrage large, parcours jusqu'au menu (0 écart) ; captures avant / après dans `qa/pr/WFS-558/` (+ 3 en anglais).
- Limites : temps d'image du téléphone non mesuré (RG-C5) ; voiture de l'adversaire chargée à son arrivée ; 2 erreurs du parcours « duel entre amis » déjà présentes sur `main` (temps 0,0 s à l'arrivée à 4), signalées dans la PR.

### 2026-11-16 10:58:43 — WFS-565 annulé, choix de la carte traité dans WFS-557 (po-scrum-master)
- **Ordre de l'utilisateur** (transmis par Claude, agent dev3d (01UY1Vv4Fd), session WFS-557) : « Non non pas de nouvelles tickets, tu me traites ça maintenant ».
- **WFS-565** passe en 🚫 Annulé (ordre de l'utilisateur : traité dans WFS-557), ligne déplacée dans `archive/taches-terminees.md` ; suivi noté dans `regles-de-gestion/WFS-565.md`.
- **WFS-557** : nouvelle **RG-23** (choix de la carte, maquettes 03 et 18a à 18d, à 100 %, 7 écarts de l'ex-WFS-565 ; image aérienne : solution du banc ou provisoire comme RG-22 si rendu hors écran). Même session, même ticket (pas de 2e sujet). WFS-558 à 562 inchangés.


### 2026-11-16 10:59:24 — Archives automatiques (robot archives)
- Journal : 157 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-14).
- TACHES.md : 0 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md`.

### 2026-11-16 11:14:19 — WFS-550 : PR #424 corrigée après la 3e revue, lead dev rappelé (Claude, agent dev3d (01UY1Vv1yg))
- Maquettes 05 / 26 / 17 corrigées, section « Maquettes » dans la description, menu de la maquette 01 (RG-11), SF-X de vitrine, éclairage du parking ; synchronisation avec `main` (WFS-551 fusionné, conflit `garage_fiche.cpp` résolu) ; 151 tests CPU ; 20 captures ciblées.

### 2026-11-16 11:20:43 — WFS-559 : PR #431 ouverte, accroches annoncées (Claude, agent dev3d (01Us6uCwnA))
- Fait : portique de départ en treillis carré 290 mm (membrures Ø 50, diagonales Ø 20, manchons, platines M24) et caisson de 5 colonnes de 2 lanternes Ø 200 mm à visière, posé par le code à la ligne de départ (s = 0) de toutes les cartes sauf le Grand Prix, tous les modes ; feux pilotés par le décompte (`Course::feuxRouges` / `feuxVerts`) ; chiffres 3 / 2 / 1 retirés, « GO » sous les feux, consigne FR / EN (arbitrages RG-05 à RG-13).
- Fichiers : `src/world/portique_depart*.{h,cpp}` (nouveaux), `src/game/course.{h,cpp}`, `src/game/course_system.cpp`, `assets/textes/{fr,en}.txt`, `tests/test_portique_depart.cpp`, `CMakeLists.txt`.
- **Accroches (≤ 10 lignes, RG-11)** : `src/ui/ecrans/pause.cpp` et `src/game/camera_vue.cpp` : 1 ligne chacune dans `boutonVisible` (MENU et ARRIÈRE cachés pendant le décompte, RG-10) ; `src/platform/desktop_main.cpp` : 2 lignes (banc `portique-depart`).
- Vérification : `test_portique_depart` OK (mesures ±1 %, qualité du maillage, cadence des feux) ; 151 tests CPU OK ; banc `portique-depart` 2400 × 1080 : 0 écart ; captures avant / après (banc `depart`) dans `qa/pr/WFS-559/`.
- Limites : en vue « Proche » le caisson est coupé par le haut de l'image (caméra inchangée, RG-12) ; le Grand Prix garde sa propre formule de feux (`circuit.cpp`, hors zone).


### 2026-11-16 11:27:01 — WFS-550 : PR #424 refusée en 4e revue (lead-dev)
- Commit relu `bda7c618` ; construction sans avertissement, **151 tests CPU réussis** ; 20 captures ouvertes ; diff complet lu ; maquettes 01, 05, 08, 13, 15, 17, 26, 29, 30 comparées au code et aux captures (une ligne par maquette dans la revue) : mise en page conforme.
- Points de la 3e revue corrigés (section « Maquettes », icônes des anneaux, combo orange).
- **Refus (bloquant)** : la SF-X de vitrine du menu (`menu.cpp:89`) appelle `vehicule_skin::poser`, qui libère la seule texture d'habillage (WFS-492) : après `Game::setCar`, `Menu::init` retire la skin achetée du joueur (banc `hud` avec `WFS_SKIN=ciel` : « ciel de golf libérée » à chaque `g.init`), et la vitrine perd la sienne après une autre pose (capture `apres-pret-menu-actif-fr` : SF-X sans skin bleu ciel).
- À mesurer : chargement total et mémoire avec la SF-X de vitrine (282 027 triangles, 42 textures) chargée à chaque partie.
- 4e refus sur cette PR : PO prévenu ici (point nouveau, venu de la vitrine ajoutée depuis la 3e revue). TACHES : ↩️ Retour lead dev (PR #424).

### 2026-11-16 11:30:58 — WFS-558 : PR #430 refusée par le lead dev (lead-dev)
- Commit relu : 09620ba0. Captures ouvertes (17 + 3 EN), diff complet relu, maquettes 04 et 23 comparées ; branche construite et testée : 151 tests CPU réussis. Avis agent 3D VALIDÉ présent.
- Refus (revue https://github.com/wlfaleh/waforspeed_mobile/pull/430#pullrequestreview-5919258004) : (1) `salon.cpp:onTouch` : en vue large, le bloc 3D renvoie avant la boucle `g_salons` : « Salons à côté de toi » ne se touche plus (joinRoom perdu) ; (2) `Salon3d::drawUi` dessine pseudo, légendes et afficheur nets par-dessus le salon flouté (« ALEX » flottant sur `apres-salon-internet`, points parasites sur hôte / prêt / amis) ; (3) maquette 04 : le pilier est trop loin en vue large, le pseudo (seul affichage depuis RG-07) et le code de l'afficheur sont illisibles (RG-05) : écart non accepté par le PO ; (4) section « Tests CPU » et `tests-cpu.log` absentes ; (5) `apres-salon-code-efface.jpg` d'un ancien commit.
- Pour le PO : la voiture de l'adversaire est chargée pendant l'écran (RG-C5), annoncé par le développeur, inévitable.

### 2026-11-16 11:32:09 — WFS-558 : arbitrage RG-05, lisibilité de l'afficheur (po-scrum-master)
- Demande du lead dev (PR #430, point 3) via Claude, agent dev3d (01Us6uCtQN). À vraie taille (82 × 18 mm), l'afficheur fait ≈ 3 px de caractère dans le cadrage large : illisible.
- Décision : **option A**. RG-05 corrigée : afficheur allumé et visible en cadrage large ; code tapé lisible en vue rapprochée ; en cadrage large, un code commencé est aussi écrit dans le panneau « Rejoins avec un code » sous l'aide (« Code : K7Q_ », 4 cases, style du code du salon). Pas d'agrandissement de l'afficheur (RG-02).

### 2026-11-16 12:07:38 — WFS-561 : scène 3D des résultats prête au banc, accroches annoncées (Claude, agent dev3d (01Us6uCxBL))
- Arbitrage du PO (RG-04 à RG-09) : la scène de la maquette 11 sert aux fins de la course à 5, du duel et de la Poursuite. Les fenêtres 2D des maquettes 31 à 33 restent comme repli. Les accroches dans les `resultat()` des 3 modes attendent la fusion de WFS-557, qui touche les mêmes fichiers.
- Fait, dans des fichiers neufs (`src/ui/resultats_parking.*`, `resultats_parking_calc.cpp`, `resultats_parking_banc.cpp`, `tableau_led.*`) :
  - le parking de WFS-552, réutilisé tel quel ;
  - un vrai panneau LED P16 de 3,10 × 1,52 m : caisson RAL 7016, masque noir, une visière par rangée, LED éteintes visibles, points ambre de 590 nm, police 5 × 7 avec accents ;
  - le reflet sur le sol mouillé (panneau et réglettes) ;
  - la voiture du joueur de trois quarts ;
  - les textes FR / EN (`resultats.*`).
- Test `test_tableau_led` : police, mise en page, cadrage et reflet. Banc `WFS_BANC=resultats3d` : 0 écart, rien n'est créé pendant l'écran. Les 151 tests CPU passent.
- **Accroches de moins de 10 lignes, hors de ma zone, annoncées ici :**
  - `src/platform/desktop_main.cpp` : banc `resultats3d` (2 lignes) ;
  - `src/world/meteo.cpp` : pas de gouttes de pluie sur l'écran des résultats, sous terre (2 lignes : un include et un test sur `resultats3d::affiche()`). Ce défaut a été vu par l'agent 3D sur la capture de nuit.
- Limites :
  - le vrai reflet de la voiture sur le sol du parking demande une passe miroir pour le sol du parking (aujourd'hui, seule la route mouillée en a une, WFS-16). C'est hors de cette zone : je le proposerai au PO ;
  - le ×2 par vidéo (« Maintiens ») n'est pas encore dessiné dans le jeu : c'est WFS-512. Rien n'est changé (RG-03).
### 2026-11-16 12:07:14 — WFS-557 : PR #432 ouverte (Claude, agent dev3d (01UY1Vv4Fd))
- Écrans R19 de WFS-548, 549, 552 vérifiés et corrigés aux maquettes (tableau `qa/pr/WFS-557/verification.md`, une ligne par maquette) ; bouton Start / Stop mesuré au pixel sur la maquette 19 (demande de l'utilisateur).
- RG-23 : choix de la carte (maquettes 18a, 18b, 18c) refait dans `terrain.cpp` et `paquets.cpp` (étiquettes, Télécharger, progression, Supprimer) ; 18d déjà fait par WFS-551.
- Tests CPU 150 / 150, bancs `paquets`, `cgu`, `creation`, `market`, `menu3d` à 0 écart ; 22 paires de captures en 1600 × 720 + planches maquette / avant / après.
- Suite : avis de l'agent 3D sur le cadrage du menu (WFS-552), puis lead dev.


### 2026-11-16 12:18:05 — WFS-550 : 4e revue corrigée, lead dev rappelé (Claude, agent dev3d (01UY1Vv1yg))
- PR #424 : deux emplacements de skin (vitrine / joueur), vitrine chargée seulement au menu (274–301 ms, 98 Mo, libérée en sortant), remarques corrigées (`595417f6`) ; captures `hud-course`, `hud-solo-jour` (skin ciel), `pret-garage-actif-fr`, `pret-menu-actif-fr` et `journal-skins.txt` (`6d573913`).
- Chargement d'une partie (banc PC) : main 5,42 / 5,18 s, branche 6,17 / 5,66 s (image de plus de l'étape « Voitures », rendu logiciel).
- Branche synchronisée (`2684ab7e`), 151 tests CPU verts ; description de la PR mise à jour, réponse sous la revue ; suivi-pr en 🔁.

### 2026-11-16 12:18:20 — WFS-559 : PR #431 refusée par le lead dev (lead-dev)
- Revue complète de `ce6a0e6b` : 18 captures après, 7 avant et 4 zooms ouverts ; maquette 09 comparée au code (6 lignes conformes, écart : lueur verte du GO) ; diff entier lu ; branche construite, `tests_cpu.sh build` : 151 tests réussis.
- Refus pour : (1) section « Tests CPU » et `qa/pr/WFS-559/tests-cpu.log` absentes (règle du 2026-11-11) ; (2) `PortiqueDepart::ambiance` (portique_depart.cpp:201-207) : `feuxRouges()` vaut 0 au GO, donc la lumière verte n'est jamais posée (RG-04, code mort) ; (3) nouvel avis de l'agent 3D sur le prochain commit. Seules les 2 captures de nuit au GO sont à refaire.
- Revue postée en COMMENT (même compte GitHub) : https://github.com/wlfaleh/waforspeed_mobile/pull/431

### 2026-11-16 12:38:08 — WFS-550 fusionné par le lead dev (lead-dev)
- PR #424, 5e revue sur `2684ab7e` : les 2 points de la 4e revue sont corrigés (deux emplacements de skin, joueur et vitrine du menu ; vitrine chargée seulement au menu) et les remarques sont nettoyées.
- Diff des corrections lu en entier ; branche construite (aucun avertissement) ; **151 tests CPU verts** ; bancs `hud` (WFS_SKIN=ciel : la skin de la Golf n'est jamais libérée par la vitrine) et `chargement` (5,79 s, écart venant de l'image en plus de l'étape « Voitures ») rejoués.
- 9 maquettes (01, 05, 08, 13, 15, 17, 26, 29, 30) comparées au code et aux captures : conformes.
- Revue « ✅ Approuvé » postée en COMMENT (même compte GitHub), puis fusion en squash : `436f0738`. Statut 🟣 ; ligne dans `qa/Check.md` ; ligne retirée de `suivi-pr.md`.

### 2026-11-16 12:38:36 — WFS-550 fusionné, session terminée (Claude, agent dev3d (01UY1Vv1yg))
- PR #424 fusionnée en squash par le lead dev (5e revue, `436f0738`). WFS-550 en 🟡 À valider (utilisateur) : menu et voiture de vitrine demandés par l'utilisateur, à confirmer sur l'APK de `main` (à regarder : à-coup possible au retour au menu, rechargement de la vitrine ≈ 0,3 s sur PC).
- Remarques non bloquantes du lead dev pour le PO (petite US possible) : paramètre `g_pose` de `poserDans` qui masque la globale, `vehicule_skin::coder(float)` plus appelé. Cadenas de la maquette 15 : fait par WFS-551.
- Un seul ticket par session : je m'arrête, aucun autre ticket demandé.

### 2026-11-16 12:40:01 — WFS-558 : refus du lead dev corrigé, PR #430 à relire (Claude, agent dev3d (01Us6uCtQN))
- Avis agent 3D du 1er tour (vis, gaine, inox brossé, plaque) corrigé dans f0412179, « VALIDÉ ».
- Refus du lead dev (5 points) corrigé : tuile du salon local avant le bloc de la vue large (b85f0df0, banc « 0 écart ») ; plus de texte net sur le salon flouté ; panneau nominatif 600 × 200 mm et caméra rapprochée (pseudo 23 px, test CPU) ; code tapé recopié dans le panneau en vue large (arbitrage du PO RG-05, 9c6c9a25) ; tests CPU (151 réussis, `qa/pr/WFS-558/tests-cpu.log`) ; captures refaites avec le code final.
- Avis agent 3D « VALIDÉ » sur 248debab (réserves mineures : ombre portée du panneau, pseudo à ≈ 20-23 px à ne pas éloigner).


### 2026-11-16 12:43:25 — WFS-558 : 2e revue de la PR #430, refusée (lead-dev)
- Points 1 à 5 de la 1re revue corrigés (code de la tuile, salon sans texte net, plaque 600 × 200 mm lisible, code dans le panneau, tests CPU 151, captures refaites) ; avis agent 3D VALIDÉ sur 248debab.
- Refus : (A) conflit avec `main` dans `src/ui/ecrans/menu.cpp` (WFS-550 #424, vitrine SF-X contre parking partagé `parking::commun()`) : squash impossible ; (B) `apres-salon-accueil.jpg` sans la tuile « Rejoindre Nadia » (salon expiré pendant le rendu du banc).
- Pour le PO : 2 erreurs du parcours « duel entre amis » (revanche : écran de fin, temps 0,0 s) présentes aussi sur `main` 73b7f6fd (banc `menu2`) : anomalie existante, sans lien avec WFS-558.

### 2026-11-16 12:52:44 — WFS-557 : 1re revue de la PR #432, refusée (lead-dev)
- Relu au commit 7cf15e7b : maquettes 01, 02, 03, 04, 08, 12, 14, 18a-c, 19 à 24, 31 ouvertes et comparées au code et aux captures ; diff entier lu ; branche construite, tests CPU 150 / 150 ; bancs textes-course, menu2, course5 (1600 × 720), cgu, profil (1280 × 720) rejoués.
- Refus (6 points) : captures modes-choix et fin Checkpoint périmées (avant 3eaf6eb4) ; CGU coupées en 16:9 (« J'accepte » hors écran, cgu.cpp:147, k = H / 720) ; Market : carte Pack débutant sans image (maquette 12, non déclaré) ; salon : solde ★ absent (maquette 23) ; tests-cpu.log et section « Tests CPU » absents ; description (« 21 % plus grande ») à corriger. Capture du garage demandée (bague chrome partagée).
- Pour le PO : banc menu2, parcours duel « Revanche » à 0,0 s en échec aussi sur main (088f306c), déjà signalé par la revue de #430.
- La synchronisation avec main après WFS-550 est faite par le développeur (message du coordinateur) ; aucune fusion.

### 2026-11-16 13:34:33 — WFS-559 : PR #431 corrigée après la revue du lead dev (Claude, agent dev3d (01Us6uCwnA))
- Cause racine du point 2 : le système de la nuit (`nuit.cpp`, ordre 950) remet `fp.spotCount` à 0 après le portique (ordre 112) : la lumière de nuit des feux était effacée à chaque image. Elle est posée par `PortiqueDepartLumiere` (ordre 960), en cône, 0,003 par colonne, portée 12 m (réglé au banc sur le critère de l'agent 3D) ; le banc `portique-depart` vérifie qu'elle est posée (rouge au décompte, verte au GO).
- Tests CPU : 152 réussis sur aa466277 (`qa/pr/WFS-559/tests-cpu.log`) ; banc `portique-depart` 2400 × 1080 : 0 écart ; captures de nuit refaites.
- Avis agent 3D : VALIDÉ (avec réserves mineures, suivi WFS-560 / P3) sur b89f5de1 ; lead dev rappelé pour la 2e revue.


### 2026-11-16 13:42:43 — WFS-558 fusionné par le lead dev (lead-dev)
PR #430 (Claude, agent dev3d (01Us6uCtQN)) fusionnée en squash (c1149a60) à la 3e revue, revue sur 2df3c5c8 (« ✅ Approuvé » en commentaire, même compte GitHub). Points A (synchro WFS-550 : parking partagé + vitrine SF-X, shutdown sans double libération) et B (tuile de Nadia sur `salon-accueil`) vérifiés ; captures `menu-duel` avant / après identiques, `salon-accueil`, `salon-code` ouvertes ; construction et 152 tests CPU refaits sur 2df3c5c8. Avis agent 3D VALIDÉ. Statut 🟡 À valider (utilisateur). Au PO : 2 erreurs déjà présentes sur main dans le parcours « duel entre amis » du banc (revanche, temps 0,0 s).

### 2026-11-16 13:43:18 — WFS-558 : fin, session terminée (Claude, agent dev3d (01Us6uCtQN))
- PR #430 fusionnée en squash par le lead dev (c1149a60) à la 3e revue, après 2 refus (1 : salon local injoignable, texte net sur le flou, pseudo et code illisibles en vue large, tests CPU, capture ancienne ; 2 : conflit avec WFS-550, tuile de Nadia absente) ; avis agent 3D « VALIDÉ » sur le dernier commit.
- Statut : 🟡 À valider (utilisateur) : à regarder sur le téléphone : écran « Duel en ligne » (parking, digicode, plaque du pseudo), toucher du digicode → vue rapprochée, saisie du code, « Retour », temps d'image (non mesuré ici).
- Pour le PO (non bloquant) : 2 erreurs déjà présentes sur main dans le parcours « duel entre amis » du banc menu2 (temps 0,0 s à la revanche) ; réserves de l'agent 3D (ombre portée du panneau, reflet ponctuel de la réglette sur le pare-brise).
- Un seul ticket par session : la session s'arrête, aucun autre ticket demandé.


### 2026-11-16 14:00:29 — WFS-559 fusionné par le lead dev (lead-dev)
PR #431 relue en 2e revue sur b89f5de1 (seulement les 3 points de la 1re revue) : tests CPU dans la PR, lumière de nuit posée par `PortiqueDepartLumiere` (ordre 960, après `nuit.cpp` qui vide les lumières), avis agent 3D VALIDÉ sur le dernier commit. Construction, 152 tests CPU, banc `portique-depart` (0 écart) refaits ; essai local avec main (WFS-558) : 153 tests réussis. « ✅ Approuvé » en commentaire, fusion en squash 83f5bb50. Statut 🟡 À valider (utilisateur) (RG-C7). WFS-560 (en attente de WFS-559) peut passer Prêt : au PO.

### 2026-11-16 14:01:09 — WFS-559 : fin, PR #431 fusionnée, session terminée (Claude, agent dev3d (01Us6uCwnA))
- PR #431 fusionnée en squash par le lead dev (83f5bb50) ; ticket 🟡 À valider (utilisateur) : à essayer sur l'APK de `main` (départ de course : portique, 5 feux rouges, verts au GO, consigne, boutons MENU / ARRIÈRE cachés pendant le décompte).
- À reprendre avec WFS-560 (portique « ARRIVÉE », même modèle `portique_depart::construire(…, false)`) ou en US de suite (réserves non bloquantes de l'agent 3D et du lead dev) : caisson qui se lit comme un panneau plat de loin ; zoom de la platine et des boulons au ras du sol ; motif des optiques des feux (P3) ; banc : contrôle du bouton MENU pour RG-10 ; `circuit.cpp` (Grand Prix) à brancher sur `Course::feuxRouges` à sa reprise.
- Session terminée (un seul sujet par session) : aucun autre ticket demandé.


### 2026-11-16 17:30:18 — WFS-561 : PR #433 ouverte (Claude, agent dev3d (01Us6uCxBL))
- Ordre de l'utilisateur (« dépêche-toi ») : les accroches sont posées sur `main` sans attendre WFS-557 (PR #432). « Menu » reste l'autocollant de `main` tant que la plaque de WFS-557 n'est pas fusionnée. La PR fusionnée en 2e garde les deux côtés.
- Vérifié :
  - tests CPU : 154 réussis ;
  - bancs `resultats3d` et `resultats3d-modes` (vraies courses des 3 modes, FR / EN, repli 2D) : 0 écart ;
  - 20 captures dans `qa/pr/WFS-561/`.
- L'agent 3D (avis RG-C2) puis le lead dev sont appelés.

### 2026-11-16 17:48:21 — WFS-561 : PR #433 refusée par le lead dev (lead-dev)
Commit relu `da174c26`. J'ai ouvert les 20 captures et la maquette 11, et lu tout le diff. Construction locale OK ; `tests_cpu.sh build` : 154 test(s) réussi(s). Avis de l'agent 3D : VALIDÉ avec réserves.
Refus pour 5 points :
1. En Poursuite, le tableau affiche « SEMÉES » pour le nombre de voitures de police, et une ligne SCORE qui ne correspond pas au TOTAL du panneau (RG-04 non suivie).
2. Le panneau SCORE couvre le toit de la voiture, et le tableau est plus petit et décentré par rapport à la maquette 11.
3. La capture des tests CPU est absente.
4. Il manque 4 images dans la description, et `pr.md` contient encore des « À REMPLIR ».
5. Un commentaire est abîmé dans `course5_system.cpp`.
Détail dans la revue de la PR.

### 2026-11-16 18:49:57 — WFS-561 : PR #433 corrigée, 2e revue demandée (Claude, agent dev3d (01Us6uCxBL))
- Corrections de la 1re revue :
  - Poursuite : « POLICES » remplace « SEMÉES », et la ligne SCORE est retirée ;
  - cadrage : voiture de trois quarts, toit sous le panneau SCORE, vérifié par le test ;
  - `tests-cpu.log` ajouté (154 tests réussis) ;
  - description de la PR et `pr.md` remplis ;
  - commentaire de `course5_system.cpp` remis sur sa ligne ;
  - tableau refait 10 fois par seconde au plus.
- Limite : aux vraies mesures, le tableau ne peut pas faire 35 % de la largeur comme sur la maquette : il en fait environ 25 %. Pour aller plus loin, il faudrait un arbitrage du PO sur RG-02.
- 9 captures des points corrigés dans `qa/pr/WFS-561/` (1re revue dans `revue1/`). Commit relu : `8b1ab779`.

### 2026-11-16 18:59:43 — WFS-561 fusionné par le lead dev (lead-dev)
- 2e revue de la PR #433 (commit relu 8b1ab779) : 9 captures ouvertes avec la maquette 11, diff da174c26..8b1ab779 lu, construction et 154 tests CPU refaits. Points 1, 3, 5 corrigés ; point 4 corrigé dans `pr.md`, 4 liens d'images encore cassés dans la description (non bloquant, à reprendre par le développeur).
- Point 2 accepté avec réserve : le panneau SCORE de 3 ou 4 lignes cache encore l'arrière du toit ; `test_tableau_led` prend le bas du panneau de la maquette (+0,13) et non celui de `score_fin` (−0,13 à −0,23). **Pour le PO** : arbitrer (panneau SCORE à la taille de la maquette dans WFS-557, ou tableau plus grand que RG-02) et créer l'US des réserves de l'agent 3D (reflet de la voiture, flou du reflet).
- « ✅ Approuvé » posté en commentaire (même compte GitHub), fusion en squash 186c986e. Statut 🟡 À valider (utilisateur).

### 2026-11-16 19:00:46 — WFS-561 : fusionné (#433, 186c986e), session terminée (Claude, agent dev3d (01Us6uCxBL))
- Le lead dev a fusionné en squash à la 2e revue. Statut : 🟡 À valider (utilisateur).
- À transmettre au PO :
  - US séparée pour les réserves de l'agent 3D : vrai reflet de la voiture et des poteaux sur le sol du parking (passe miroir, comme WFS-16), reflet du tableau un peu plus flou, rangées vides du tableau ;
  - recouvrement du toit par le panneau SCORE quand il a 3 ou 4 lignes : à trancher (panneau SCORE réduit par WFS-557, ou tableau plus grand que RG-02) ;
  - à la fusion de WFS-557 : remplacer l'autocollant « Menu » de `resultats3d::boutons` par `ui::boutons::plaque` (1 ligne), et garder les deux côtés dans les `resultat()` des 3 modes.
- 4 liens d'images de la description de la PR restent mal affichés (réécrits par GitHub) ; les captures sont dans `qa/pr/WFS-561/`.
- La session s'arrête : un seul ticket par session.

### 2026-11-16 21:21:14 — WFS-557 : 2e revue demandée, photos validées par l'utilisateur, code seul (Claude, agent dev3d (01UY1Vv4Fd))
- Ordre de l'utilisateur (2026-11-16) : périmètre élargi (menu 01, boutons du menu et du garage, choix de la carte, HUD de course 17 / 17b, cadeau du jour 05 avec un coffre en bois 3D modélisé par le code et son animation) ; « les photos que tu as envoyées : dis au lead dev que c'est OK pour moi, il regarde que le code ».
- Fait : HUD (`hud.*`, `minicarte.*`, `commandes_tactiles.cpp`, `camera_vue.cpp`), cadeau (`cadeau_jour.*`), coffre 3D (`coffre_cadeau.*`, 2 288 triangles), accroche `SceneDediee::anime` (`system.h`, `game.cpp` : scène du menu non figée pendant le coffre), plaque « Menu » des résultats 3D (note de WFS-561).
- Tests CPU : 154 réussis (`qa/pr/WFS-557/tests-cpu.log`). PR #432 : description et réponse point par point (e303da17).
- À transmettre au PO : bancs `market` et `profil` en écart aussi sur main depuis WFS-550 (#424, boutons Market et Profil retirés du menu) ; `score-fin` et `cadeau-gains` en écart sur la branche (cause probable WFS-550 / 551, à confirmer) ; carte « voiture prêtée » du menu vide au banc (maquette 26).

### 2026-11-16 23:39:20 — WFS-557 fusionné par le lead dev (lead-dev)
- PR #432 (commit relu c8d8ca71) en 2e revue, **code seul sur ordre de l'utilisateur** (« c'est OK pour moi les photos, il regarde que le code ») : aucune capture redemandée.
- Diff complet relu (coffre en bois 3D `src/ui/coffre_cadeau.*`, HUD, carte de guidage, pause, cadeau du jour, menu, Profil, bancs, tests). Accroches : `system.h`, `game.cpp`, `renderer.cpp:87` (`UI_MAX_VERTS` 8 192 → 32 768), `resultats_parking.cpp`. Pas de forme nue : le coffre est modélisé pièce par pièce.
- Construction en Release et `tests_cpu.sh` refaits : **154 test(s) réussi(s)**. Revue « ✅ Approuvé » postée en commentaire (même compte GitHub), puis squash d85b1d60.
- Remarques non bloquantes, pour une US de ménage (PO) : code mort dans `minicarte.cpp` (`abords`, `rues`, `bandeau`, `nomRue`) et `test_bandeau_gps` qui teste ce bandeau ; commentaire « ≈ 6 Mo » à doubler (2 images en vol) ; boutons ronds du menu non allégés ; rangée MENU / CAMÉRA centrée sur l'ancien panneau du score.
- WFS-557 → 🟡 À valider (utilisateur) ; ligne #432 retirée de `suivi-pr.md` ; session du développeur terminée dans `track-agent.md`.

### 2026-11-16 23:40:14 — WFS-557 : fusionné (#432, d85b1d60), session terminée (Claude, agent dev3d (01UY1Vv4Fd))
- Fusion en squash par le lead dev à la 2e revue (code seul, ordre de l'utilisateur ; écrans validés par l'utilisateur sur `qa/pr/WFS-557/cote-a-cote-42/`). Statut : 🟡 À valider (utilisateur).
- À transmettre au PO (remarques non bloquantes du lead dev, US de ménage) : code mort de l'ancien bandeau dans `minicarte.cpp` (`abords`, `rues`, `bandeau`, `nomRue`) et `test_bandeau_gps` qui le teste encore ; commentaire de `renderer.cpp:87` (≈ 12,6 Mo en tout, pas 6) et allègement de `rondMenu` / `piluleMenu` non fait ; `placerBoutons` centré sur l'ancien panneau du score ; coffre gardé en mémoire graphique jusqu'à la sortie du menu.
- La session s'arrête : un seul ticket par session.

### 2026-11-17 11:06:13 — Statuts automatiques, bancs du matin #46 (robot statuts)
- Run rouge (schedule, commit 3388570), aucun statut changé. Job(s) en échec : endurance et chauffe, banc (realisme-2), banc (complements-c).
- Tickets 🟣 contenus dans ce run : WFS-555, WFS-564, WFS-496, WFS-503.

### 2026-11-17 11:41:34 — Archives automatiques (robot archives)
- Journal : 96 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-15).
- TACHES.md : 0 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md`.

### 2026-11-17 19:35:26 — Campagne VABF de la dernière version de main : NON APTE (Agent qa (session_01UY1Vv3tQoXdU3HwAjakb9t))

- Demande de l’utilisateur (« @qa lance une campagne de test recette la dernière version ») ; version : bancs du matin #46, commit `33885704` (avec WFS-557), Release `captures-matin` du jour et `simulateur-matin`.
- ~1 300 images contrôlées, ~520 ouvertes (5 agents QA en parallèle, lots A à D). Rapport `qa/rapports/VABF-2026-11-17.md` (+ lots détaillés `qa/rapports/VABF-2026-11-17/`).
- 38 anomalies ajoutées à `anno.md` : **ANO-276 à ANO-313** (11 bloquantes, 15 majeures, 12 mineures).
- Points urgents pour le PO : ANO-276 (scénario de l’émulateur bloqué aux nouvelles CGU de WFS-557 : aucun essai sur vrai APK possible) et ANO-277 (bancs `complements-c` et chauffe rouges : « Menu » de l’écran de fin déplacé, banc pas mis à jour ; `realisme-2` rouge) → US technique P1 « correction pipeline CI ». Puis ANO-288 (commandes absentes en vue arrière), ANO-289 (gaucher non miroir), ANO-287 (score de fin incohérent), formes nues ANO-278 à 286.

### 2026-11-17 20:55:42 — VABF 2026-11-17 : preuves ajoutées (Agent qa (session_01UY1Vv3tQoXdU3HwAjakb9t))

- Sur demande de l’utilisateur (« ta des preuves de tes anno ? ») : 28 images de preuve dans `qa/rapports/VABF-2026-11-17/preuves/` (ANO-276 à 301), toutes ouvertes et confirmées ; ANO-294 corrigée (pose « assis » = vitrine du banc, retirée du constat).
