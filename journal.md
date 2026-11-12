# WaForSpeed Mobile — journal

Journal du projet (tickets WFS-XX de `TACHES.md`) : une entrée par étape (début, construction, PR, verdict QA, fin…) avec
fait / fichiers / vérification / limites. **Nouvelles entrées à la fin du fichier.** Toujours partir de la dernière version de
`main` (`git pull`), ajouter seulement ses entrées, puis envoyer tout de suite ; ne jamais écraser le fichier.
**Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
Exemple : `### 2026-11-03 19:43:10 — WFS-58 construit, essai simulateur lancé (Claude, session trafic)`.
> **Archive (règle de l'utilisateur, 2026-11-06)** : ce fichier ne garde que les **2 derniers jours**. Les entrées plus anciennes sont dans `journal/archive/AAAA-MM-JJ.md` (un fichier par jour) ; ne les relire qu'en cas de besoin précis. Le PO déplace chaque jour les entrées de plus de 2 jours.

### 2026-11-11 00:02:26 — Ordre de l'utilisateur : WFS-325 en premier après WFS-324 (po-scrum-master)
- L'utilisateur a demandé la liste des 40 prochaines tâches (donnée dans la session du PO), puis : « tu peux mettre 325 en premier après 324 ».
- Fait : bandeau dans `TACHES.md` et case Statut de WFS-325 ; WFS-325 passe de la 15e place de l'ordre du 2026-11-10 14:20 UTC à la place juste après WFS-324, avant WFS-405 et WFS-364 à 368. Rien d'autre ne change.
- WFS-325 reste « En attente » jusqu'à la fusion de WFS-414 (PR #343, mêmes fichiers du cockpit) ; il ira au premier `dev-artiste-3d` (nouvelle session) qui demande. Aucune attribution faite.

### 2026-11-11 00:04:51 — WFS-415 fusionné par confiance (#342, af1285a4), ordre de l'utilisateur (lead-dev, session 01UY1VvJJ)
- L'utilisateur a arrêté l'agent lead dev pendant les captures, puis a ordonné « Fusionner » : PR #342 fusionnée en squash, sans 2e revue ni captures.
- Contenu : remplaçantes de la Course à 5 pas plus rapides (test_course5_ordinateurs), bouton Crédits après les choix du menu (endurance), interrupteur `archi::ouverturesReelles()` à 0 en jeu (fenêtres de WFS-402 coupées, code gardé, `WFS_OUVERTURES=1`), LOD0 Basse 120 m.
- Vérifié : 102 tests CPU réussis en local. Non vérifié : endurance locale, mesure img/s, captures. La CI de `main` dira si tout est vert ; WFS-415 en 🟡 À valider (utilisateur), à mesurer sur le téléphone.

### 2026-11-11 00:47:19 — WFS-416 : synchronisation avec main, repli mémoire, captures de la PR (Claude, agent dev3d (01UY1Vv4rv))
- Arbitrages du PO (f59ff8ec) appliqués : recopie bornée permise (RG-03), +80 Mo acceptés (RG-07), poids accepté (RG-08) ; condition « jamais de plantage par manque de mémoire » : `ArchiCarte::init` rend tout et repasse au kit si une réservation échoue (non provoqué au banc).
- Branche synchronisée avec `main` (WFS-415 fusionné, #342) : conflits sur `archi_geo.cpp`, `archi_carte.cpp` (mêmes lignes que la base prise de WFS-415, contenu identique vérifié par `git diff 9b1725c4 af1285a4 -- src tests` vide) et `journal.md` (les deux côtés gardés). 103 tests CPU réussis après synchronisation.
- Captures : `AGENT=… SANS_AVANT=1 bash tools/qa/captures_pr.sh WFS-416 batiments` : 29 images dans `qa/pr/WFS-416/`, banc 0 écart ; le banc joue lui-même « kit de secours » (état de `main`) puis « paquet cuit ». Mesure de ce passage, système des bâtiments seul à 160 km/h : kit 0,16 / 10,6 ms, kit + fenêtres WFS-402 0,20 / 18,3 ms, paquet 0,04 / 0,9 ms (moyenne / pire).
- Limites : téléphone non mesuré ; publication des paquets à relancer par l'agent expert ci cd après la fusion.
### 2026-11-11 00:27:22 — WFS-416 : décisions de l'utilisateur relayées, seuil de 30 img/s déplacé (Claude, session « APK quality issues » (01UY1Vv2WG))
- Mesures de l'APK 995 (WFS-415) sur le téléphone de l'utilisateur, Ville et Ville Néon de nuit, Basse : **16 à 21 img/s** (12 sur l'APK 994), médiane 55 à 66 ms, 99 % 71 à 88 ms (190 ms avant), chauffe 1, scène GPU 43 à 55 ms. CI #995 : compilation, APK et endurance verts.
- Utilisateur : WFS-416 va jusqu'à la PR et sa validation sur téléphone ; +80 Mo de mémoire graphique acceptés. RG-07 de WFS-416 : pas de perte d'img/s par rapport à l'APK 995 et pire temps des bâtiments ≤ 1 ms ; les 30 img/s iront au ticket « lumière légère en Basse », à créer **après le test de l'utilisateur** (son ordre). Message envoyé à la session de WFS-416.

### 2026-11-11 00:48:56 — WFS-416 : PR #344 ouverte (Claude, agent dev3d (01UY1Vv4rv))
- PR #344 `feature/waforspeed-416` → `main`, synchronisée avec `main` (WFS-415 compris), 5f5bf85a : bâtiments de la Ville cuits dans le paquet, 0 construction en course, kit de secours. 103 tests CPU, banc « batiments » 0 écart, 29 captures dans `qa/pr/WFS-416/`.
- Suite : avis de l'agent 3D (épic E15), puis lead dev. CI de `main` #995 (WFS-415) encore en cours à l'ouverture : à voir verte avant la fusion. Après la fusion : relancer « Paquets de cartes » (agent expert ci cd).

### 2026-11-11 00:53:57 — WFS-416 : réserves de l'agent 3D en tâche séparée, WFS-417 créé (po-scrum-master)
- PR #344 : « Avis agent 3D : VALIDÉ avec réserves ». Décision du PO (RG-E15-09, ordre de l'utilisateur du 2026-11-09) : **fusion sans attendre ces réserves**, elles ne sont pas un motif de refus pour le lead dev ; pas de correction de la réserve 1 dans la PR.
- **WFS-417** créé (P2, E14 + E15, `regles-de-gestion/WFS-417.md`) : dalle de balcon et main courante au niveau 1 cuit (20 à 120 m), coût mesuré, cadres au loin sans scintillement, captures manquantes. Attend la fusion de WFS-416 ; nouvelle session `@dev3d`, pas celle de WFS-416. Statut de WFS-416 inchangé (🔀 En PR #344).

### 2026-11-11 01:16:03 — WFS-414 : PR #343 refusée par le lead dev (lead-dev)
- Revue du commit `c14418e` : https://github.com/wlfaleh/waforspeed_mobile/pull/343#pullrequestreview-5852174684 (commentaire : GitHub refuse « Request changes » avec le même compte).
- **Bloquant (1 point)** : la règle du ticket (compléments 2 à 5 de l'utilisateur) demande « + braquage » pour l'Abarth, le T-Cross, la S3 et la Giulia ; le banc `cockpit-volant` ne le fait que pour la Mustang et la Golf. À montrer : les 4 suites de braquage avant / après (normales et éclaircies) et l'avis de l'agent 3D sur ces captures.
- Non bloquant : 8 liens cassés dans la description GitHub (braquage éclairci de la Mustang ; `pr.md` est juste).
- Validé et à ne pas refaire : les 37 paires ouvertes (gros plans et éclaircis) ; le volant des 6 voitures est juste (coussin bombé, branches attachées, plaques, jante entière devant la visière) ; M4 et 911 identiques (même SHA-256) ; tout le diff lu (étape 1 = déplacement pur, vérifié ligne à ligne) ; construction et tests refaits dans un dossier à part : 102 / 102, `test_cockpit` vert.
- Pour le PO : jauge de nitro du combiné cachée par le moyeu (question ouverte dans la PR) ; rapport de l'Abarth petit (≈ 11 % du combiné), à confirmer par l'utilisateur.
- Statut : ↩️ Retour lead dev (PR #343) dans `TACHES.md`, `track-agent.md` et `suivi-pr.md`.

### 2026-11-11 01:24:33 — WFS-416 : PR #344 refusée par le lead dev, 3 points bloquants (lead-dev)
- Reprise de la revue prise à 00:54 UTC (perdue au redémarrage du conteneur ; aucune revue ni fusion n'était posée). Commit relu : 5f5bf85a. Revue : https://github.com/wlfaleh/waforspeed_mobile/pull/344#pullrequestreview-5852270500 (en commentaire : GitHub refuse « changements demandés » sur le même compte).
- Fait : 29 captures ouvertes, tout le diff lu, branche construite, **103 tests CPU réussis** ; CI de `main` #995 verte ; avis agent 3D « VALIDÉ avec réserves » lu (réserves → WFS-417, pas un motif de refus). Bancs existants relancés sur la branche : `cuisson` 0 écart ; **`paquets` 4 écarts (code 3)**.
- **Bloquant 1** : le banc `paquets` (WFS-306, groupe `complements-c` des bancs du matin) passe au rouge : le faux téléchargement avance à 64 Ko par image sur environ 600 images (≈ 38 Mo), le paquet ASTC de la Ville passe de ≈ 31 Mo à 197 Mo et ne finit plus (`src/ui/ecrans/paquets.cpp:489`, `:613`, `:625-639`, `:656`).
- **Bloquant 2** : un téléphone qui a déjà le paquet de la Ville ne reçoit jamais les bâtiments cuits : l'étape se ferme dès qu'un paquet à la bonne empreinte est là (`src/ui/ecrans/paquets_etape.cpp:35-40`) et l'empreinte ne change pas (`VERSION_GENERATION_CARTE` = 1). Choix du moyen à trancher par le PO avec l'agent expert ci cd.
- **Bloquant 3** : le repli « mémoire insuffisante → kit » ne peut pas se déclencher pour la mémoire graphique (`VK_OK` → `std::abort()`, `src/render/vk.h:17`) ; les tests de `src/world/archi_carte.cpp:253` et `:266` sont morts ; après un repli, l'occlusion des murs du kit est perdue (2e appel de `precalcul::occlusionKit`). Condition gardée par le PO (arbitrage du 2026-11-10 23:50) non tenue.
- Non bloquant : cuisson sur deux machines à surveiller (nombres à virgule dans le bloc) ; signature PC / téléphone à vérifier au journal du téléphone ; vérification du paquet (0,7 à 0,8 s sur PC) hors de la mesure du chargement ; mémoire du jeu +314 Mo au banc `cuisson`.
- Validé et à ne pas refaire : image (de près, à 50 m, nuit, 160 km/h), format du bloc et sa lecture vérifiée, test `test_batiments_cuits`, kit de secours.
- Statut : ↩️ Retour lead dev (PR #344) dans `TACHES.md`, `track-agent.md` et `suivi-pr.md`.

### 2026-11-11 01:27:24 — WFS-416 : 3 décisions du PO après le refus du lead dev (PR #344) ; WFS-418 créé (po-scrum-master)
- Demande de Claude, agent dev3d (01UY1Vv4rv). Détail : `regles-de-gestion/WFS-416.md`, section « Décisions du PO après le refus du lead dev ».
- **1. Banc `paquets` (WFS-306)** : accroche accordée dans `src/ui/ecrans/paquets.cpp`, partie banc seulement : débit du faux réseau réglé sur la taille du paquet ; aucun contrôle retiré ni affaibli ; la coupure simulée reste.
- **2. Paquet déjà installé** : option A, `VERSION_GENERATION_CARTE` de 1 à 2. Vérifié dans le code : `ListePaquets.choisir` ne retient qu'un paquet à l'empreinte attendue par le jeu, donc les APK 927 à 995 ne téléchargent pas le nouveau paquet et gardent le leur. `VERSION_MIN_APK` reste à 927, rien à changer dans `.github/**`. L'agent expert ci cd relance « Paquets de cartes » après la fusion (consignes écrites pour la section « Après la fusion »). Il n'a pas été consulté : le code répond à la question ; il confirmera sur la liste publiée.
- **3. Mémoire graphique** : option B. L'arrêt du jeu quand la carte graphique refuse une réservation existe déjà sur `main` pour tout le jeu ; WFS-416 garde le repli sur manque de mémoire vive, retire les tests morts et garde l'occlusion des murs après un repli. La décision de l'utilisateur « jamais de plantage par manque de mémoire » va dans **WFS-418** (E0, P2, créé, attend la fusion de WFS-416, nouvelle session). +314 Mo de mémoire du jeu : permis, à expliquer dans la PR.
- Statut inchangé : ↩️ Retour lead dev (PR #344). Aucune attribution.

### 2026-11-11 02:00:24 — WFS-416 : corrections après le refus du lead dev sur la PR #344 (Claude, agent dev3d (01UY1Vv4rv))
- **Cause racine du refus** : je n'avais relancé que mes bancs ; le banc `paquets` (WFS-306) cassait (faux téléchargement trop lent pour un paquet de 197 Mo), un téléphone déjà équipé ne recevait jamais le nouveau paquet (empreinte inchangée), et mon repli « mémoire » testait des tampons que `createBuffer` ne rend jamais vides (il arrête le jeu).
- **Décisions du PO (50790ee6)** appliquées dans 118d70c9 : (1) accroche dans la partie banc de `src/ui/ecrans/paquets.cpp` : champ `imagesPourFinir` de la source factice (lignes de la source, des cas 2 et 5), débit réglé sur la taille, aucun contrôle retiré ; nouvelle preuve « paquet d'avant WFS-416 installé → mise à jour proposée → bloc des bâtiments présent » ; (2) `VERSION_GENERATION_CARTE` 1 → 2, `VERSION_MIN_APK` inchangé ; (3) tests morts retirés, repli sur `std::bad_alloc` décidé avant l'occlusion des murs (`preparerCuit`), lecture du bloc protégée ; manque de mémoire graphique = arrêt du jeu comme ailleurs (WFS-418).
- **Vérifié sur ce commit** : bancs `paquets` 0 écart, `cuisson` 0 écart (chargement 3,56 → 4,00 s, mémoire du jeu 2 808 → 3 091 Mo), `batiments` 0 écart (système des bâtiments à 160 km/h : kit 0,17 / 11,6 ms, paquet 0,03 / 0,5 ms) ; 103 tests CPU ; 29 captures refaites.
- Limites : téléphone non mesuré ; repli sur manque de mémoire vive non provoqué au banc.
### 2026-11-11 01:34:36 — WFS-414 : corrections après le retour du lead dev (Claude, agent dev3d (01Us6uCxLS))
- Retour du lead dev sur la PR #343 (un seul défaut bloquant) : le braquage n'était montré que pour la Mustang et la Golf. **Corrigé** (35c1071) : le banc `cockpit-volant` fait la suite de 4 images du braquage pour les 6 voitures du volant de série ; 32 nouvelles paires avant (`main` + banc) / après (69 captures en tout). Regardé : rotation d'un bloc, aucun jour, visière hors de la jante à tous les angles, palettes fixes de la Giulia sans contact.
- Description de la PR : GitHub réécrivait 4 lignes (adresses entre accents graves) ; elles sont en balises `<img>` (ac6b87c) ; description publiée relue : 132 liens d'images valides.
- Avant ce retour : 4 avis de l'agent 3D (3 « À REPRENDRE » sur le placement des chiffres du combiné, corrigés : rapport centré au-dessus de la vitesse avec jour de 5 %, marge sous la jante, vitesse de la S3 centrée entre les cadrans, disposition propre à l'Abarth) puis « VALIDÉ » sur c14418e. Captures refaites après la fusion de WFS-415 ; tests CPU 102 / 102.
- Suite : avis de l'agent 3D sur les 4 nouvelles suites de braquage, puis rappel du lead dev.

### 2026-11-11 02:01:39 — WFS-414 fusionné par le lead dev (lead-dev)
- PR #343, 2e revue (appel du développeur), commit relu `ac6b87c` : **fusionnée en squash** → `5b069f79` sur `main`. Revue « ✅ Approuvé » écrite dans la PR en commentaire (GitHub refuse « Approve » au même compte).
- Relu seulement ce qui a changé depuis `c14418e` (1re revue, refus pour un seul point) : 64 nouvelles images (braquage 1 à 4 de l'Abarth, du T-Cross, de la S3 et de la Giulia, avant et après, normales et éclaircies), toutes ouvertes ; agrandissements des palettes de la Giulia. Volant d'un bloc jusqu'à 90°, sans jour, visière qui ne coupe plus la jante, démarreur de la Giulia sur sa plaque.
- Code : seul le banc change (`vehicule_cockpit_volant_banc.cpp` l. 141 et son en-tête). Remarque non bloquante : utiliser `volantSeriePour(d)` au lieu des noms écrits à la main.
- Construction et `tests_cpu.sh` refaits par le lead dev dans un dossier à part : 102/102. « Avis agent 3D : VALIDÉ » sans réserve sur `ac6b87c`.
- Statut : 🟡 À valider (utilisateur) (KO de l'utilisateur, APK 991) ; ligne de `suivi-pr.md` supprimée ; entrée ajoutée à `qa/Check.md`. Au développeur : sa ligne de `track-agent.md` (session terminée). Au PO : WFS-325 peut partir (attendait cette fusion).

### 2026-11-11 02:02:16 — WFS-416 : PR #344 corrigée après le refus du lead dev, à relire (Claude, agent dev3d (01UY1Vv4rv))
- Les 3 points du refus sont corrigés dans 118d70c9 (décisions du PO 50790ee6) : banc `paquets` à 0 écart (débit du faux réseau réglé sur la taille), `VERSION_GENERATION_CARTE` = 2 (un paquet déjà installé est remplacé, prouvé au banc), repli mémoire vive propre et arrêt sur manque de mémoire graphique écrit tel quel (WFS-418). Bancs `cuisson` et `batiments` à 0 écart, 103 tests CPU, 29 captures refaites (f2726e80). Réponse point par point sous la revue, description de la PR corrigée.
- Suite : nouvel avis de l'agent 3D sur f2726e80, puis lead dev.

### 2026-11-11 02:07:55 — WFS-414 : fin, session terminée (Claude, agent dev3d (01Us6uCxLS))
- **Fin** : PR #343 fusionnée en squash par le lead dev (5b069f79, 2026-11-11 02:01 UTC), statut 🟡 À valider (utilisateur). 2 revues du lead dev (1 refus : braquage des 4 autres voitures, corrigé) et 5 avis de l'agent 3D (le dernier « VALIDÉ » sur ac6b87c).
- Réponses écrites sous les deux demandes de l'utilisateur sur le volant dans `tache utilisateur.md`.
- Restent pour le PO, hors de ce ticket : la jauge de nitro du bas du combiné, cachée par le moyeu plus haut (Mustang, Golf, T-Cross, Abarth) ; les remarques « pour plus tard » de l'agent 3D (coussin qui cache la vitesse de 60 à 90° de braquage, raccord vif entre la partie ronde et le méplat de l'Abarth, de la S3 et de la Giulia, palettes de la Giulia en carbone au lieu d'alu, rapport de la Mustang posé sur les segments du compte-tours, rapport plus grand que la vitesse sur la S3) ; remarque du lead dev : condition du banc à écrire avec `volantSeriePour` (WFS-325) ; ANO-243 (M4 et 911) déjà dans WFS-325.
- Le titre de la discussion n'a pas pu être changé (aucun outil `set_session_title` dans cette session). Un seul ticket par session : la session s'arrête.

### 2026-11-11 02:47:37 — WFS-416 fusionné par le lead dev (lead-dev)
- PR #344, 2e revue (appel du développeur), commit relu `f2726e80` : **fusionnée en squash** → `55ee0909` sur `main`. Revue « ✅ Approuvé » écrite dans la PR en commentaire (GitHub refuse « Approve » au même compte) : https://github.com/wlfaleh/waforspeed_mobile/pull/344#pullrequestreview-5853054315
- Les 3 points du refus (5f5bf85a) vérifiés : banc `paquets` à 0 écart sans contrôle retiré (48 contrôles sur `main`, 53 ici) ; `VERSION_GENERATION_CARTE` = 2 et preuve au banc « paquet d'avant installé → mise à jour proposée → bloc des bâtiments présent » ; repli sur manque de mémoire vive décidé avant l'occlusion des murs, tests morts retirés, description corrigée (mémoire graphique : arrêt du jeu, WFS-418).
- Refaits par le lead dev : tout le diff relu, 29 captures ouvertes, construction, **103 tests CPU**, bancs `paquets`, `cuisson` (mémoire du jeu 2 749 → 3 037 Mo) et `batiments` (passage rapide) à 0 écart.
- **Synchronisation** : la branche n'avait pas WFS-414 (fusionné à 02:01 UTC, une minute avant le dernier envoi du développeur). Fusion d'essai de `main` dans la tête de la PR, dans un dossier à part, sans rien pousser : aucun conflit, construction bonne, 103 tests, bancs `paquets` et `batiments` à 0 écart. WFS-414 ne touche que la vue Cockpit : captures gardées.
- « Avis agent 3D : VALIDÉ avec réserves » sur `f2726e80` (vaut « VALIDÉ », décision du PO ; réserves dans WFS-417).
- Remarques non bloquantes écrites dans la revue : `std::bad_alloc` non rattrapé dans `initCuit` après `preparerCuit` (pour WFS-418), tampons de chargement non vidés après un repli, déclarations en double dans `archi_geo.h`, titre du banc `bloc-refuse`.
- Statut : 🟡 À valider (utilisateur) ; ligne de `suivi-pr.md` supprimée ; entrée ajoutée à `qa/Check.md`. **À l'agent expert ci cd** : relancer « Paquets de cartes » sur `main` après une CI verte (points a à f de la PR) ; avant cela, le nouvel APK joue la Ville du code. Au PO : WFS-417 et WFS-418 peuvent partir. Au développeur : sa ligne de `track-agent.md` (session terminée).

### 2026-11-11 02:48:19 — WFS-416 : fin, session terminée (Claude, agent dev3d (01UY1Vv4rv))
- PR #344 approuvée et fusionnée en squash par le lead dev (55ee0909) après un refus corrigé (banc `paquets`, génération de carte 2, repli mémoire) ; statut 🟡 À valider (utilisateur).
- Reste à faire par d'autres : relance de « Paquets de cartes » sur `main` par l'agent expert ci cd (sans elle, le nouvel APK refuse l'ancien paquet et joue la Ville du code : aucun changement visible) ; WFS-417 (balcons de 20 à 120 m, cadres au loin, captures) et WFS-418 (repli sur manque de mémoire graphique, `std::bad_alloc` dans `initCuit`) pour de nouvelles sessions.
- À vérifier sur le téléphone : ligne « 107 morceaux lus dans le paquet » au journal du jeu, images par seconde contre l'APK 995, points blancs le long des cadres au loin.
- Un seul ticket par session : cette session s'arrête et ne demande pas d'autre ticket.

### 2026-11-11 09:27:54 — « Paquets de cartes » relancé sur main : paquets de la Ville de WFS-416 publiés (agent expert ci cd, session 01UY1Vv2Hq)
- Ordre de l'utilisateur (« lance la nouvelle Paquets de cartes sur main »), point a de la PR #344. Condition vérifiée avant : `build.yml` run 997 vert sur 55ee0909 (WFS-416).
- Lancement à la main de `paquets.yml` sur `main` (ed6f745) à 09:08:48 UTC : **run n° 2 vert** en 4 min 55 s (cuisson a, cuisson b, publication) : https://github.com/wlfaleh/waforspeed_mobile/actions/runs/42992692347
- Vérifié au journal du job publication : deux cuissons identiques octet par octet (3 fichiers) ; empreinte `e209d649cd6600e2` pour les 3 formats ; `ville_astc` 196,56 Mo (sha256 bdaf43d1a774…), `ville_etc2` 180,14 Mo (81192c6f6832…), `ville_rgba8` 201,15 Mo (eaa2d8d1b046…) ; « Paquets puis cartes.json envoyés ».
- Release `paquets` : `cartes.json` remplacé à 09:13:40 UTC, 3 nouveaux fichiers présents ; les 3 paquets du run n° 1 (5 à 28 Mo) sont toujours là, rien supprimé.
- Non vérifié : le téléchargement sur un téléphone. À l'utilisateur, avec l'APK de `main` (run 997 ou plus récent) : la mise à jour du paquet de la Ville (≈ 197 Mo) doit être proposée ; puis les points à regarder de WFS-416 (« 107 morceaux lus dans le paquet », images par seconde contre l'APK 995, points blancs le long des cadres au loin).

### 2026-11-11 09:31:34 — Ordre de l'utilisateur : WFS-405 dans l'épic « Publication app » (po-scrum-master)
- Ordre : « pour la 405, tu dois le mettre avec l'épic Publication app ».
- Fait : case Épic et case Statut de WFS-405 dans `TACHES.md` (⏸ En attente du jour de publication), bandeau, ordre permanent du skill du PO complété, section Suivi de `regles-de-gestion/WFS-405.md`. WFS-405 n'est plus « avant tout autre ticket » ; il n'était attribué à personne.
- Épic « Publication app » : WFS-319, 408, 409, 407, 405 et le suivi 320. Zone de code de WFS-405 inchangée.

### 2026-11-11 09:58:36 — Statuts automatiques, bancs du matin #16 (robot statuts)
- Run rouge (schedule, commit ed6f745), aucun statut changé. Job(s) en échec : banc (realisme-2).
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-406, WFS-396, WFS-362, WFS-363, WFS-384, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-11 10:10:56 — Téléphone de l'utilisateur : Samsung Galaxy S21 FE ; avis des agents sur la performance (Claude, session « APK quality issues » (01UY1Vv2WG))
- **Téléphone de référence de l'utilisateur : Samsung Galaxy S21 FE** (selon la version : Exynos 2100 / Mali-G78 MP14, ou Snapdragon 888 / Adreno 660 ; à lire dans le jeu). Toute mesure de performance se fait sur lui.
- Mesures de nuit en Basse (45 %, MSAA ×2) : APK 995 médiane 55-66 ms ; APK 997 + paquet médiane 53-58 ms, scène 60-64 ms (voie rapide). Le diagnostic « la lumière est la cause » de cette session est **retiré** : Basse déjà à 4 lumières, WFS-353 : −8 % (2 lumières), −17 % (sans lumières de nuit), −33 % (45 % au lieu de 60 %).
- Avis de PO, lead dev, agent 3D, dev3d, smart brain, QA (réponses en lecture seule, rien modifié) : unanimes pour un ticket P1 « diagnostic sur le téléphone » (interrupteurs dans configgame.json, temps GPU par passe, temps CPU, triangles, appels, mémoire par type, trajet rejoué, résultats dans un fichier), puis une seule correction sur la cause mesurée. Suspects cités : surdessin (opaques non triés), shader de scène long (boucle fixe de 12 projecteurs), reflet de la route mouillée (scène redessinée en miroir), passes en plus, halos additifs (aussi cause de la nappe orange), textures non compressées (1,1 Go). **En attente de l'accord de l'utilisateur** pour créer les tickets.

### 2026-11-11 10:13:22 — WFS-419 créé : diagnostic de performance sur le téléphone, accord de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- L'utilisateur accepte le point 1 (« Ok pour 1 ») : **WFS-419** (P1, [TECHNIQUE], `@dev`, nouvelle session), règles dans `regles-de-gestion/WFS-419.md` : interrupteurs éteints par défaut (image identique), temps GPU de chaque passe, temps CPU, mémoire par type, appareil affiché, trajet rejoué jour et nuit, un interrupteur à la fois, fichier de résultats partageable. Aucune correction dans ce ticket ; la correction viendra d'un seul ticket créé sur la cause mesurée.
- Bancs du matin #16 : pris par un agent expert ci cd (selon l'utilisateur) ; aucun ticket créé ici. Gel des décors lourds : **non décidé** par l'utilisateur (rien changé).

### 2026-11-11 10:16:06 — WFS-419 coupé en lots, lot 1 au lead dev avec fusion directe (Claude, session « APK quality issues » (01UY1Vv2WG))
- Ordre de l'utilisateur : tester vite ; le lead dev prend et fusionne directement. Lot 1 : interrupteurs dans un menu caché + appareil affiché + ligne technique existante (et temps GPU par passe si déjà disponible). Lot 2 (trajet rejoué, fichier) plus tard. Règles mises à jour dans `regles-de-gestion/WFS-419.md`.

### 2026-11-11 10:19:02 — WFS-419 : lancement annulé, attend le go de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- La session avait lancé le lead dev sur le lot 1 sans « go » explicite de l'utilisateur ; l'utilisateur l'a arrêté. Aucun code produit (branche locale vide, rien poussé, pas de PR). WFS-419 remis en « ⏳ Prêt, attend le go de l'utilisateur ».

### 2026-11-11 10:21:01 — WFS-419 : section « Découpage et exception » retirée des règles, à la demande de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- L'utilisateur avait posé une question (« un lead dev peut prendre rapidement et merger directement ? »), pas donné un ordre. Section retirée de `regles-de-gestion/WFS-419.md`, ligne de `TACHES.md` remise à « ⏳ Prêt (`@dev`, nouvelle session) ». Les règles sont celles de la création du ticket.

### 2026-11-11 10:28:55 — WFS-419 réduit aux interrupteurs, WFS-420 créé pour le banc automatique (Claude, session « APK quality issues » (01UY1Vv2WG))
- Ordre de l'utilisateur : un ticket = un besoin, pas de lots dans un même ticket. **WFS-419** (P1) : interrupteurs du menu caché, appareil et temps par passe dans la ligne technique. **WFS-420** (P2, après WFS-419) : trajet rejoué jour et nuit, un interrupteur à la fois, fichier de résultats. Règles réécrites dans `regles-de-gestion/WFS-419.md` et créées dans `WFS-420.md`.

### 2026-11-11 10:42:20 — WFS-419 codé et fusionné par le lead dev (#345, squash d63e2a46), ordre de l'utilisateur (lead-dev)
- Ordre de l'utilisateur : le lead dev prend WFS-419, le code et fusionne directement sur `main`, sans relecture par un autre lead dev, sans tests locaux, sans endurance ni captures du banc PC (captures prises par l'utilisateur sur son S21 FE, APK de `main`). Statut : 🟡 À valider (utilisateur).
- Livré : `src/render/diagnostic_perf.*`, 12 interrupteurs tous éteints par défaut (MSAA ×1, sans pré-passe, sans reflet de la route mouillée, sans ombres, sans pluie, sans halos additifs, sans bloom ni post, 0 lumière de nuit, résolution 30 %, shader couleur unie, décor coupé, distance ÷ 2). Menu caché : 5 touchers sur le titre « RÉGLAGES » en moins de 3 s. Section `diagnostic` dans `configgame.json`, 12 clés à 0 ; les valeurs de l'utilisateur déjà présentes sont gardées.
- Ligne technique : interrupteurs allumés, puis, une fois le menu ouvert, appareil (modèle Android, GPU, pilote, Vulkan) et coût de chaque passe.
- « GPU ombre 0,0 · reflet 0,0 » : en Basse, la passe d'ombre est vide (ombres du soleil coupées) et il n'y a pas de passe de reflet (coupée en Basse). Ce sont de vraies mesures : tout le temps est dans « scène » et « post ».
- Accroches hors zone : `src/world/meteo.cpp` (2 lignes : interrupteur « sans pluie »), `src/render/passes.cpp` (2 lignes). Vérifié avant la fusion : shaders compilés (glslangValidator), syntaxe C++ (g++), `test_configgame` 124 clés et 3 nouveaux cas OK.
- Téléchargement noté (outillage, pas d'actif) : paquets apt `libvulkan-dev` et `glslang-tools` (dépôt Ubuntu, registre de paquets autorisé), pour vérifier les fichiers en local ; rien n'est ajouté au dépôt.

### 2026-11-11 10:43:31 — Archives automatiques (robot archives)
- Journal : 251 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-09).
- TACHES.md : 0 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md`.

### 2026-11-11 11:36:19 — WFS-419 : CI de main verte (run #998, 43005769670) (lead-dev)
- Compilation, APK, endurance, bancs texte et menu1-2400, publication : tous verts. Ticket laissé en 🟡 À valider (utilisateur). 1re capture de l'utilisateur : menu caché ouvert, S21 FE Snapdragon 888 (SM-G990B2, Adreno 660, pilote 512.530, Vulkan 1.1.128) ; mesures à faire en course (au menu, scène figée : 0,0 partout).

### 2026-11-11 11:43:37 — E15 : minimum de 200 Mo par décor supprimé, maximum de 700 Mo sur le disque gardé, décision de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- Ordre de l'utilisateur (« Go pour supprimer le minimum de 200 Mo et garder le maximum de 700 Mo sur le disque »). Raison : le diagnostic WFS-419 sur le S21 FE a montré que la lenteur vient du calcul par pixel, pas du poids ; un minimum de poids poussait à ajouter du contenu pour atteindre un chiffre.
- Mis à jour : `CLAUDE.md` (ligne E15), `regles-de-gestion/E15-decors.md` (RG-E15-01 : 700 Mo maximum sur le disque, moins de 200 Mo n'est plus un motif de refus, la richesse se juge sur les captures par l'agent 3D), `WFS-385` à `WFS-389` (titres et règles), `WFS-417` (RG-04), `TACHES.md` (épic E15 et titres), skill `po-scrum-master`. Les entrées datées plus anciennes (journal, note du 2026-11-09 dans TACHES) restent telles quelles.
- **Pas changé** (non décidé par l'utilisateur) : budget de mémoire graphique, budget de temps d'image sur le téléphone.

### 2026-11-11 11:47:03 — WFS-421 à 425 créés : performance de la Basse sur le S21 FE (po-scrum-master)
- Ordre de l'utilisateur (« crée les tickets A B C D E »), contenu proposé par le lead dev après lecture du code et mesures WFS-419 sur le S21 FE (Ville de nuit, Basse, 45 %, MSAA ×2 : scène 62 ms ; Corniche 28 ms). Épic E4. Aucun ticket attribué : l'utilisateur ouvrira les sessions.
- **WFS-421** (A, P1, ⏳ Prêt) : halos au sol des lampadaires coupés en Basse (−14 ms mesuré). **WFS-422** (B, P1, ⏳ Prêt) : pré-passe coupée en Basse avec secours propre (5 à 18 ms, à confirmer). **WFS-423** (C, P1, ⏳ Prêt) : lumières bornées en Basse (−10 ms mesuré avec 0 lumière). **WFS-424** (D, P1, ⏳ En attente de 421, 422, 423) : route en texture précuite, avis agent 3D. **WFS-425** (E, P2, ⏳ Prêt, en parallèle) : défaut MSAA ×1 (62 → 108 ms).
- Communs : Basse seulement, mesure avant / après sur le téléphone (Ville et Corniche de nuit), « Avis agent 3D : VALIDÉ » si l'image change, 🟡 À valider (utilisateur) après fusion. Règles : `regles-de-gestion/WFS-421.md` à `WFS-425.md`. WFS-405 et WFS-420 inchangés.

### 2026-11-11 12:09:12 — « Go Corniche », WFS-426 créé, ordre des 20 prochaines tâches (po-scrum-master)
- Ordre de l'utilisateur (« go Corniche, crée le ticket 8 et l'ordre ») : la version grand public sort avec **la Corniche seule**. **WFS-405** réécrit (titre, ligne, règles) : « Version grand public : la Corniche seule (autres cartes gardées dans l'APK d'aperçu) », toujours ⏸ jour de publication ; WFS-319, 408, 409 inchangés ; WFS-407 : note « captures de la Corniche ».
- **WFS-386** (décor complet Corniche) : ⏳ En attente, après la sortie publique, à découper plus tard (note datée).
- **WFS-426** créé (P1, E14, ⏳ Prêt, nouvelle session `@cartes`, aucun attribué) : Corniche cuite en paquet `.wfcarte` et téléchargeable, en réutilisant WFS-302 (`paquets.yml`, `WFS_BANC=cuire`), WFS-416 (`archi_cuit`), WFS-307 ; 3 paquets ASTC / ETC2 / RGBA8 dans la Release `paquets` et `cartes.json` ; secours par le code ; 700 Mo max. ; scène ≤ 28 ms sur le S21 FE ; plus aucune construction en course ; 🟡 après fusion. Distingué de WFS-313 (P3, inchangé) dans les deux fichiers.
- **Ordre des 20** (remplace la tête de l'ordre des 50 du 2026-11-10) : 412, 421, 423, 422, 425, 420, 424, 426, 383, 371, 325, 395, 364 à 368, 394, 418, 397. Mis de côté : 321, 385, 399, 398 ; 386 après la sortie. WFS-383 et 371 : pas de fusion si la scène de la Corniche dépasse ≈ 33 ms sur le S21 FE (règle ajoutée dans leurs fichiers). Aucune attribution.

### 2026-11-11 12:10:19 — Skill du PO : WFS-405 « la Corniche seule », accord de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- L'ordre permanent du skill `po-scrum-master` disait encore « WFS-405, la Ville seule » ; corrigé en « la Corniche seule » (décision « go Corniche » du 2026-11-11), avec WFS-426, 383, 371 avant la sortie et WFS-386 après.

### 2026-11-11 12:10:20 — Début WFS-421, attribué à Claude, agent dev (01UY1Vv4ss) (po-scrum-master)
- Nouvelle session `@dev` (session_01UY1Vv4ss6UoRUUxN62sxWQ, aucun ticket avant) : **WFS-421** (P1, E4), 2e de l'ordre des 20 (le 1er, WFS-412, est pour `@cicd`). Un seul ticket pour cette session, jusqu'à la fusion, puis arrêt.
- Zone : `src/render/nuit.cpp` (et sa lecture de la qualité), `tests/**`. Exception PO : E4 a déjà WFS-324 🔀 (PR #341), qui touche aussi `nuit.cpp` (accroche des lampadaires derrière, boucle des projecteurs, ≈ l. 175) ; WFS-421 ne touche que l'appel `haloSol` (≈ l. 262) : zones séparées, synchroniser avec `main` avant la PR.
- Critères : `regles-de-gestion/WFS-421.md` (Basse seulement, `haloTete` gardé, mesure S21 FE Ville + Corniche, « Avis agent 3D : VALIDÉ », 🟡 après fusion).

### 2026-11-11 12:11:12 — Agent PO : skill smart-brainstorming chargé par défaut, demande de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- `.claude/agents/po-scrum-master.md` : `smart-brainstorming` ajouté au champ `skills:` et consigne d'usage (étudier, challenger, prioriser ; le PO garde son rôle de créer et attribuer). `appel-agents.md` : tableau mis à jour.

### 2026-11-11 12:14:45 — Agent PO : skill smart-brainstorming retiré, plus chargé par défaut, demande de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- Annulation du commit 1638ad6a : `.claude/agents/po-scrum-master.md` et `appel-agents.md` reviennent à l'état d'avant (skill `po-scrum-master` seul). L'utilisateur appelle `@smartbrain` quand il en a besoin.

### 2026-11-11 12:17:38 — Début WFS-412 : attribué à agent expert ci cd (session 01UY1Vv4jR6p) (po-scrum-master)
- Demande d'une nouvelle session `@cicd` (session_01UY1Vv4jR6pAfrr3rURiQVx, ouverte par l'utilisateur vers 12:14 UTC, aucun ticket avant : 1er et seul ticket). Choix : **n° 1 de l'ordre des 20** fixé par l'utilisateur à 12:08 UTC, réservé à `@cicd` : **WFS-412** (P1, E7, [TECHNIQUE]). Épic E7 libre (aucun ticket 🔧 / 🔀 ; WFS-319 et 407 à 409 en ⏸ jour de publication).
- Zone : `tools/qa/tests_cpu.sh`, la ligne des tests de `build.yml` (≈ l. 98) et de `matin.yml` (≈ l. 132), un test du script si besoin ; rien d'autre de la CI (gel). Règles : `regles-de-gestion/WFS-412.md` (RG-01 à RG-06 ; aucun seuil, test ni `IGNORES` changé).
- État vérifié sur `main` : CI #998 (run 43005769670, WFS-419) verte ; `test_course5_ordinateurs` **vert** depuis WFS-415 (103 tests réussis, licences : 17 voitures, 0 manque). Le rouge connu de WFS-404 n'existe plus : la capture n° 3 des règles (run de `main` avec seul ce test en échec) est sans objet ; le ticket garde son but (un échec ne cache plus les tests suivants ni les licences). Info : le parcours `duel` du groupe `texte` signale 2 écarts sur l'écran de fin (hors périmètre).
- US technique : CI de branche permise, une seule à la fois, chaque run noté au journal. Plan : boucle sans `exit` au 1er échec, liste finale des échecs et code 1 ; `verifier_licences.py` lancé quel que soit le résultat des tests, étape rouge si l'un échoue ; essai local avec un test rendu rouge hors commit ; PR, lead dev.

### 2026-11-11 12:21:09 — Début WFS-425, attribué à Claude, agent dev (01Us6uCtwf) (po-scrum-master)
- Demande d'une nouvelle session `@dev` (session_01Us6uCtwfvRJMyFzdJaXYeM, « @dev go », aucun ticket avant : 1er et seul ticket, jusqu'à la fusion, puis arrêt).
- Choix dans l'**ordre des 20** de l'utilisateur (12:08 UTC) : n° 3 **WFS-423** et n° 4 **WFS-422** (P1, E4) ne sont pas donnés maintenant : l'ordre dit « `@dev` 2-7 l'un après l'autre, E4 » et E4 a déjà **WFS-421** 🔧 (Claude, agent dev (01UY1Vv4ss)) et **WFS-324** 🔀 (PR #341) ; ils restent ⏳ Prêt pour la session `@dev` suivante, après la fusion de WFS-421. N° 5 **WFS-425** (P2, E4) a l'exception écrite (« en parallèle de WFS-421 à 423, fichiers différents », ordre des 20 et `regles-de-gestion/WFS-425.md`) : attribué. Aucune autre session ne le tient (pas de branche `feature/waforspeed-425` sur `origin`).
- Zone : création des passes et cibles de la scène (`src/render/passes.*`, `src/render/vk.*`, lecture de `src/render/diagnostic_perf.*`), `tests/**`. **Pas** `src/render/nuit.cpp` (WFS-421, WFS-324), ni les shaders d'éclairage (`eclairage.glsl`, `eau.glsl`, `vehicule.glsl` : WFS-423), ni la pré-passe en Basse (WFS-422). MSAA par défaut de la Basse inchangé (×2).
- Critères (`regles-de-gestion/WFS-425.md`) : cause écrite dans la PR (passe, attachements, chargement / stockage) ; MSAA ×1 plus rapide que ×2 sur le S21 FE, Ville et Corniche de nuit, chiffres avant / après dans la PR (mesure de l'utilisateur avec les interrupteurs de WFS-419) ; ×2 et ×4 : même image, même temps. Captures : 3 scènes de la section « Captures demandées » ; « Avis agent 3D : VALIDÉ » seulement si l'image change ; 🟡 À valider (utilisateur) après la fusion.
- Plan court : suivre le chemin `MSAA_X1` de `diagnostic_perf.*` jusqu'à la création des passes et cibles (variantes de WFS-262) ; comparer ×1 et ×2 (opérations de chargement / stockage, résolution, mémoire des attachements) ; corriger le chemin ×1 ; banc PC avant / après ; PR, lead dev.

### 2026-11-11 12:22:24 — WFS-425 : branche poussée, début de l'analyse (Claude, agent dev (01Us6uCtwf))
- Branche `feature/waforspeed-425` créée depuis `main` (68e4e36) et poussée (4970e80, premier commit). Skills chargés : `agent-developer`, `waforspeed-natif`.
- Titre de la discussion : l'outil `set_session_title` n'existe pas dans cette session (recherche `ToolSearch` sans résultat) ; titre voulu, non appliqué : « WFS-425 — Claude, agent dev (01Us6uCtwf) (défaut MSAA ×1 lent) ».
- Banc PC local en préparation (lavapipe + glslc installés) pour les captures avant / après.

### 2026-11-11 12:22:57 — Début WFS-426 : attribué à Claude, agent cartes (01UY1Vv2HrQF) (po-scrum-master)
- Demande d'une nouvelle session `@cartes` (session_01UY1Vv2HrQF5b231SiWzwgU, ouverte par l'utilisateur vers 12:20 UTC, aucun ticket avant : 1er et seul ticket). Choix : **n° 8 de l'ordre des 20** fixé par l'utilisateur à 12:08 UTC, réservé à `@cartes` (skill spécial) : **WFS-426** (P1, E14). Épic E14 libre (aucun ticket 🔧 / 🔀 / ↩️ en E14 ou E15) ; dépendances WFS-301, 302, 307, 416 fusionnées. Hors iOS, hors épic « Publication app ».
- Zone : E14 (`src/world/cartes_v2/**`, `paquets/sources/**`, `tests/cartes_v2*`) + ajout de la Corniche aux chemins existants de cuisson (`WFS_BANC=cuire`, `archi_cuit.*`) et à `paquets.yml` / `cartes.json` (publication coordonnée avec l'agent expert ci ; aucune autre modification de la CI). Règles : `regles-de-gestion/WFS-426.md` (RG-01 à RG-09).
- Critères : 3 paquets Corniche (astc, etc2, rgba8) publiés et dans `cartes.json` ; téléchargement / suppression depuis le choix du terrain ; secours sans paquet comme aujourd'hui ; ≤ 700 Mo ; scène ≤ 28 ms sur le S21 FE ; aucune construction de décor en course ; aucune forme nue ajoutée. Pas E15 (pas d'avis 3D obligatoire). Après fusion : 🟡 À valider (utilisateur).
- Plan : réutiliser la cuisson de la Ville (WFS-302 / 416), ajouter la Corniche, comparer image avec et sans paquet, captures avant / après, PR, lead dev ; arrêt de la session après la fusion.

### 2026-11-11 12:24:32 — Début WFS-383 : attribué à Claude, agent dev3d (01UY1Vv1zt) (po-scrum-master)
- Demande d'une nouvelle session `@devartiste` (session_01UY1Vv1ztBP5V8pNZenM1Vd, ouverte par l'utilisateur le 2026-11-11, message « @devartiste go », aucun ticket avant : 1er et seul ticket). Choix : **n° 9 de l'ordre des 20** du 2026-11-11 12:08 UTC, premier `@devartiste` de la liste (n° 1 à 8 : `@cicd`, `@dev`, `@cartes`, déjà pris ou d'un autre profil) ; file Corniche de E15 libre ; WFS-379 et 380 fusionnés (🟡).
- Zone : E15 file Corniche + terrain (E12, E2 : accroches ≤ 10 lignes par carte, annoncées au journal) ; matériaux du sol et instances des pins ; **pas** les fichiers de neige de WFS-346, ni `nuit.cpp` (WFS-421), ni les fichiers de WFS-425. Règles : `regles-de-gestion/WFS-383.md` (RG-01 à RG-06) et `regles-de-gestion/E15-decors.md`.
- Critères : plus de sol d'une seule couleur (captures 1 à 4), pins d'Alep visibles en Corniche (modèle CC0 / CC BY, 2 LOD, licence + SHA-256), budget ≈ 0,5 ms GPU et + 15 k triangles en Basse ; **pas de fusion si la scène de la Corniche dépasse ≈ 33 ms sur le S21 FE** (mesure avant / après dans la PR) ; « Avis agent 3D : VALIDÉ » avant la fusion par le lead dev ; aucune forme géométrique nue.
- Lien avec WFS-426 (Corniche cuite en paquet, agent cartes, en parallèle selon l'ordre de l'utilisateur) : fichiers séparés ; se synchroniser souvent avec `main` ; si WFS-426 fusionne d'abord, vérifier que les sols et les pins apparaissent aussi avec le paquet (sinon le dire dans la PR, le paquet se recuit).
- Plan : branche `feature/waforspeed-383` poussée tout de suite, captures avant / après (`captures_pr.sh`), PR, `@3d`, lead dev ; arrêt de la session après la fusion.

### 2026-11-11 12:24:45 — Début WFS-418, attribué à Claude, agent dev (01Us6uCt95) (po-scrum-master)
- Demande d'une nouvelle session `@dev` (session_01Us6uCt95, « @dev go », aucun souhait de sujet, aucun ticket avant : 1er et seul ticket, jusqu'à la fusion, puis arrêt).
- Choix dans l'**ordre des 20** de l'utilisateur (12:08 UTC) : n° 3 **WFS-423** et n° 4 **WFS-422** (P1, E4) restent ⏳ Prêt derrière **WFS-421** (« `@dev` l'un après l'autre, E4 », même décision qu'à 12:21 pour WFS-425) ; n° 5 WFS-425 déjà pris ; n° 6 WFS-420 et n° 7 WFS-424 en E4 (en attente) ; n° 8 à 18 pour `@cartes`, `@devartiste`, `@conduite`. Premier `@dev` permis : n° 19 **WFS-418** (P2, E0 + `vk.*`, [TECHNIQUE]) : dépendance remplie (WFS-416 fusionné, #344, 55ee090), session différente de celle de WFS-416. E0 libre (aucun ticket 🔧 / 🔀). Pas de branche `feature/waforspeed-418` sur `origin`.
- Zone : `src/render/vk.*` (réservation des tampons seulement), `makeMesh` (`src/render/renderer.cpp`), `src/world/archi_carte.*` (appel du repli), `tests/**`. Exception PO : WFS-425 touche les passes et cibles dans `vk.*`, WFS-426 la cuisson (`archi_cuit.*`) : pas les mêmes fonctions ; synchroniser souvent avec `main`.
- Critères (`regles-de-gestion/WFS-418.md`, complété) : réservation refusée au chargement des bâtiments cuits → kit de secours, sans arrêt du jeu (test automatique, moyen d'essai PC) ; aucun écart d'image ni de temps quand la mémoire suffit (bancs `cuisson`, `batiments`, `perf`) ; liste des autres réservations sans repli remise au PO. 🟣 puis ✅ par le robot (US technique).
- Plan court : voie de `createBuffer` qui rend un tampon vide sur `VK_ERROR_OUT_OF_DEVICE_MEMORY` / `OUT_OF_HOST_MEMORY` ; `makeMesh` qui le signale ; repli vers le kit dans `archi_carte.cpp` ; variable du banc qui fait échouer la N-ième réservation ; test ; captures `batiments` jour / nuit et sans refus ; PR, lead dev.

### 2026-11-11 12:27:16 — WFS-418 : branche poussée, début de l'analyse (Claude, agent dev (01Us6uCt95))
- Branche `feature/waforspeed-418` créée depuis `main` (82d7bfc) et poussée (68b7566, premier commit). Skills chargés : `agent-developer`, `waforspeed-natif`.
- Titre de la discussion : l'outil `set_session_title` n'existe pas dans cette session (recherche `ToolSearch` sans résultat) ; titre voulu, non appliqué : « WFS-418 — Claude, agent dev (01Us6uCt95) (repli mémoire graphique) ».
- Banc PC local en préparation (lavapipe + glslc installés, construction de `main` en cours) pour les captures avant / après.

### 2026-11-11 12:32:54 — WFS-421 : PR #346 ouverte (Claude, agent dev (01UY1Vv4ss))
- Fait : en Basse, `nuit.cpp` n'appelle plus `haloSol` pour les lampadaires (RG-01) ; `haloTete` gardé (RG-02) ; vraie lumière inchangée (RG-03). Règle en fonctions pures dans `src/render/nuit_qualite.h`, test `tests/test_nuit_qualite.cpp`.
- **Accroches hors zone annoncées** : 1 ligne `add_executable(test_nuit_qualite …)` dans `CMakeLists.txt` ; banc court `WFS_BANC=halos-sol` (`src/render/nuit_halo_sol_banc.cpp`, nouveau) + 2 lignes dans `src/platform/desktop_main.cpp` (aucun banc existant ne mettait la Corniche de nuit en Basse).
- Vérifié : 104 tests CPU réussis ; banc `halos-sol` avant / après : Moyenne et Haute 0 pixel différent (Ville et Corniche) ; Basse : nappe orange disparue, appels de dessin Ville 234 → 213, Corniche 253 → 233.
- Limite : RG-04 (gain sur le S21 FE) non mesurable ici, à mesurer par l'utilisateur avec l'APK de `main` (interrupteurs de WFS-419).

### 2026-11-11 12:33:45 — WFS-324 : 2e revue de la PR #341, refusée (lead-dev)
- Commit relu e924aac1. Branche fusionnée en local avec `main` sans conflit ; construction OK ; tests CPU 104 / 104 verts.
- Réglés : glace intérieure (la voiture qui suit est entière sur la Golf, le Range Rover Sport, la M4 et la 911), capture des Réglages « Rendu », explication de la glace gauche sombre la nuit.
- **Refus sur un seul point (RG-04, capture 2)** : de nuit, les phares de la voiture qui suit restent éteints dans les glaces (anneaux sans lumière ni halo, rien sur le Range Rover Sport, voiture de la voie de gauche illisible), alors qu'ils éblouissent en vue directe (WFS-413).
- Statut : ↩️ Retour lead dev (PR #341) dans `TACHES.md`, `track-agent.md` et `suivi-pr.md`.

### 2026-11-11 12:38:28 — WFS-425 : analyse et plan (Claude, agent dev (01Us6uCtwf))
- **Chemin suivi** : `diag::MSAA_X1` → `Renderer::appliquerNettete` (MSAA ×1) → `createTargets` (passe et cibles de la scène) → `createScenePipelines`. Les pipelines, les shaders, le tri des objets et les autres passes sont **les mêmes** en ×1 et en ×2 / ×4. Seule différence : en ×2 / ×4, les shaders écrivent dans une couleur MSAA et une profondeur **temporaires** (mémoire de tuile), résolues une fois dans la cible HDR à la fin ; en ×1, ils écrivent **directement dans la cible HDR**, une vraie image en mémoire (64 bits par pixel), relue ensuite par le bloom et la passe finale.
- **Mesure sur le banc PC** (lavapipe, GPU qui n'a pas de tuiles), Ville de nuit, Basse, 60 %, Rue des Mimosas : scène **218 ms en ×4, 98 ms en ×1**. Sur un GPU classique, le ×1 est donc bien plus rapide : aucun défaut de calcul dans le chemin ×1 ; la lenteur de 62 → 108 ms ne vient que du GPU à tuiles du téléphone (Adreno 660).
- **Cause retenue** (par élimination, à confirmer par la mesure sur le S21 FE) : avec une passe sans MSAA qui écrit dans une vraie image, le pilote Adreno peut rendre la scène hors de la mémoire de tuile (rendu « direct ») : plus de tri des pixels cachés de la passe de tri, chaque pixel caché passe dans le shader de nuit et chaque écriture va en mémoire principale. Une passe MSAA reste toujours en tuiles.
- **Plan** : en ×1 seulement, même schéma que le ×2 : sous-passe 0 = couleur et profondeur **temporaires** ×1 ; sous-passe 1 = copie de la couleur (attachement d'entrée, même tuile) dans la cible HDR. Image identique au bit près. Création de la passe de scène réunie dans une fonction (`src/render/passe_scene.*`, nouveau fichier), réutilisée par le cache des variantes de WFS-262. ×2 et ×4 : passe identique. MSAA par défaut de la Basse inchangé (×2).
- **Accroches hors de `src/render/**`** : `shaders/scene_copie.frag` (nouveau fichier, 1 ligne utile), sa ligne dans la liste des shaders de `CMakeLists.txt`.
- **Risque** : le choix du mode (tuiles ou direct) appartient au pilote Qualcomm ; rien ne permet de le voir sur le PC. Si la mesure du téléphone après la PR ne baisse pas, l'hypothèse est fausse et la suite sera une pré-passe de profondeur pour le ×1 (autre ticket). La copie coûte une lecture et une écriture par pixel, dans la tuile (presque rien) ; sur le PC et l'émulateur (×1 aussi), une copie plein écran en plus.
- **Constat signalé au PO (hors ticket)** : l'interrupteur `diagnostic.msaa_x1` lu dans `configgame.json` ne prend effet qu'au prochain changement de cibles (qualité ou menu caché), alors que la ligne technique affiche déjà « diag : MSAA1 » avec « MSAA ×4 » (vu au banc : 1re course en ×4, la suivante en ×1). Le menu caché, lui, recrée tout de suite les cibles : la mesure de l'utilisateur n'est pas en cause. Proposition : petite US pour recréer les cibles quand la configuration change. Décision attendue de : PO.

### 2026-11-11 12:42:49 — WFS-418 : arbitrage RG-02 / RG-03 et accroches (po-scrum-master)
- Question de Claude, agent dev (01Us6uCt95) : les 2 atlas des bâtiments cuits (≈ 11 Mo) passent par `uploadTexture` / `createImage`, hors de « la voie des tampons » de la précision de 12:24.
- **Décision : (a) et (b)**. (a) RG-02 couvre tout le chargement des bâtiments cuits (critère 1 : « réservation de mémoire graphique refusée ») : voie de repli des images dans `vk.*` (`essayerImage` / `essayerUploadTexture`), ajout seulement, pour les 2 atlas seulement ; `createImage`, `uploadTexture`, cibles et passes (WFS-425) inchangés. (b) Textures de la route (`route_revetements.cpp`, secours procédural) et le reste : liste RG-03 remise dans la PR, tickets à suivre créés par le PO.
- **Accroches validées** : `eclairage_precalcule.{h,cpp}` (5 lignes), `batiments_banc.cpp` étape 5 (à ramener à 10 lignes au plus ; un des 2 refus porte sur un atlas), `desktop_main.cpp` (1 ligne), `CMakeLists.txt` (2 lignes du test). Détail : `regles-de-gestion/WFS-418.md` (Suivi).

### 2026-11-11 12:44:02 — WFS-421 fusionné par le lead dev (lead-dev)
- PR #346, 1re revue, commit relu 24328c32, squash d5c13495. Revue « ✅ Approuvé » écrite dans la PR en commentaire (même compte GitHub).
- 12 captures ouvertes : en Basse, la nappe orange (Ville) et le disque à bord net (Corniche) ont disparu, la rue reste éclairée ; Moyenne et Haute identiques octet par octet.
- Diff lu en entier ; branche fusionnée en local avec `main`, construction OK, tests CPU tous verts (dont `test_nuit_qualite`). « Avis agent 3D : VALIDÉ avec réserves ».
- Statut 🟡 À valider (utilisateur) : gain RG-04 à mesurer sur le S21 FE. **Pour le PO** : disque au sol encore en Moyenne / Haute (forme nue, réserve de l'agent 3D) → anomalie ou US à créer.

### 2026-11-11 12:44:15 — WFS-421 : fin, session terminée (Claude, agent dev (01UY1Vv4ss))
- PR #346 approuvée et fusionnée en squash par le lead dev (d5c13495), après « Avis agent 3D : VALIDÉ avec réserves ». Ticket 🟡 À valider (utilisateur) : gain RG-04 à mesurer sur le S21 FE (Ville et Corniche de nuit, Basse, 45 %, MSAA ×2, interrupteurs de WFS-419).
- Suite pour le PO : disque orange au sol encore dessiné en Moyenne et Haute (forme nue, réserve de l'agent 3D et du lead dev), hors périmètre de WFS-421.
- Un seul sujet par session : je m'arrête, aucun autre ticket demandé.

### 2026-11-11 12:49:34 — WFS-412 : branche poussée (75a99915), run de branche #1000 lancé (agent expert ci cd (session 01UY1Vv4jR6p))
- Validation de l'utilisateur (« si c'est ok tu passes la PR automatiquement »). `tools/qa/tests_cpu.sh` lance tous les tests même après un échec et finit par la liste des tests en échec (code 1 s'il y en a, 0 sinon, « aucun test trouvé » reste une erreur) ; `build.yml` (étape des tests, groupe `texte`) et `matin.yml` (job des tests) lancent `verifier_licences.py` même si un test échoue et gardent le pire des deux codes. Aucun seuil, test ni `IGNORES` changé.
- US technique : run `workflow_dispatch` de `build.yml` sur `feature/waforspeed-412` n° 1000 (id 43023690539), branche synchronisée avec `main` (889159b6). Il vérifie : la nouvelle étape tourne verte avec le même résumé qu'avant (103 tests) et les licences sont vérifiées.

### 2026-11-11 12:55:08 — WFS-364 attribué à Claude, agent dev3d (01UY1Vv2nQ) (po-scrum-master)
- Demande d'une nouvelle session `dev-artiste-3d` (session 01UY1Vv2nQ…, premier et unique ticket). Ordre des 20 du 2026-11-11 12:08 UTC, côté `@devartiste` : 9. WFS-383 pris (01UY1Vv1zt), 10. WFS-371 attend WFS-383 (même file Corniche), 11. WFS-325 attend WFS-324 (↩️ PR #341), 12. WFS-395 attend WFS-359 / 349 / 390 → **13. WFS-364** (P1, E10), premier attribuable.
- Début : WFS-364, retouches de forme de la Golf et de la Corsa ; épic E10 ; branche `feature/waforspeed-364`. **Exception PO** : en parallèle de WFS-324 (E4 + E10, `vehicule_cockpit*`), zones séparées : `assets/cars/golf*`, `assets/cars/corsa*`, entrées de `tools/formes.json`, ajouts à `tools/retoucher_formes.mjs`. Critères : marque plus reconnaissable de face et de 3/4, ±1 %, `--verifier` vert, disque noir de la Golf et relief du capot de la Corsa refaits (RG-SUIVI-404). Captures : vitrine face, 3/4 avant, vue basse, zooms ×4, jour et nuit, une image en course.
- Ménage : ligne de Claude, agent dev (01UY1Vv4ss) enlevée de `track-agent.md` (WFS-421 fusionné #346, session terminée).

### 2026-11-11 12:56:19 — WFS-421 (suite) codé et fusionné par le lead dev, ordre de l'utilisateur (lead-dev)
- L'utilisateur a refusé l'acceptation de #346 : le grand cercle orange au sol des lampadaires restait en Moyenne et Haute (forme nue). Le lead dev l'avait jugé à tort hors périmètre.
- Correction (PR #347, squash 78df1b0f) : `r.haloSol` des lampadaires supprimé à toutes les qualités dans `nuit.cpp` ; têtes de lampe gardées ; `nuit_qualite.h` et `test_nuit_qualite` mis à jour (le test vérifie qu'il n'y a plus d'appel).
- Vérifié : construction sans avertissement, tests CPU tous verts, banc `halos-sol` (Basse identique à `main`, cercle disparu en Moyenne / Haute). Fusion directe sur ordre de l'utilisateur (comme WFS-419). Statut 🟡 À valider.

### 2026-11-11 12:57:22 — WFS-418 : repli codé, accroches annoncées (Claude, agent dev (01Us6uCt95))
- Fait (branche `feature/waforspeed-418`, f76ca05 + suite) : voie de repli `VkCtx::essayerBuffer` / `essayerUploadBuffer` (tampon vide si Vulkan répond « plus de mémoire », toute autre erreur arrête le jeu comme `VK_OK`), `essayerImage` / `essayerUploadTexture` pour les 2 atlas (décision du PO de 12:42), `Renderer::essayerMesh` ; `archi_carte.cpp` : un refus pendant le chargement des bâtiments cuits rend tout ce qui a été pris et le kit de secours reprend (« mémoire graphique insuffisante : kit de secours » au journal du jeu). `createBuffer`, `uploadBuffer`, `makeMesh`, `createImage`, `uploadTexture` : comportement inchangé.
- **Accroches annoncées** (validées par le PO à 12:42) : `src/render/eclairage_precalcule.{h,cpp}` (5 lignes : `occlusionKit(..., garderGrille)` + `rendreGrille()`, pour que le kit garde l'occlusion de ses murs après un refus) ; `src/platform/desktop_main.cpp` (1 ligne : `WFS_MANQUE_MEMOIRE_GPU=N`, banc PC seulement) ; `src/world/cartes_v2/batiments_banc.cpp` (10 lignes : étape 5 du banc « batiments », 3 refus : image du 2e atlas, milieu des maillages fixes, dernier tampon du niveau 0 de nuit) ; `CMakeLists.txt` (2 lignes : `test_repli_memoire`).
- Vérifié (PC, lavapipe) : `test_repli_memoire` vert (1079 vérifications) ; banc « batiments » rapide (version sans les atlas) : 5802 réservations des bâtiments cuits par chargement, refus n° 2902 et n° 5802 → kit de secours, aucun arrêt, mémoire graphique rendue (+0,00 Mo), **0 écart**. Nouveau passage avec le refus d'un atlas en cours.
- Mesure pour RG-03 (Ville, sans paquet, PC) : trafic +316 Mo, Ville (décor) +373 Mo, voiture du joueur +63 Mo, piétons +33 Mo, conducteurs +15 Mo, textures de route +1,5 Mo ; liste complète dans la PR.

### 2026-11-11 12:57:55 — Bancs du matin rouges (runs #12 et #16) : WFS-427 créé et attribué, WFS-428 créé, WFS-377 passé P1 (po-scrum-master)
- **Demande** : agent expert ci cd (session 01UY1Vv2Hq), sur ordre de l'utilisateur du 2026-11-11 (« Répare les bancs du matin »). Diagnostic reproduit par lui sur `main` : 2 groupes rouges sur 17 (`ecrans-2400`, `realisme-2`) ; les 15 autres, les tests, l'endurance et l'APK sont verts. Le diagnostic du 2026-11-10 10:42:17 n'avait pas donné de ticket : **oubli du PO**, corrigé ici.
- **Début — WFS-427** `[TECHNIQUE]` P1, E7 : « Bancs du matin : le groupe `ecrans-2400` dépasse 75 minutes, le scinder en deux » (ANO-244). Attribué à **agent expert ci cd (session 01UY1Vv2Hq)**, branche `feature/waforspeed-427`, 1er ticket de cette session. Zone : `tools/qa/banc_groupe.sh`, matrice et liste `GROUPES` de `matin.yml`, doc du skill `expert-ci-cd`. Critères : `ecrans-2400` (`chargement` + `hud` + `camera`) et `fluidite` verts en moins de 75 min chacun, `camera` rejoué, mêmes fichiers produits, archive du nouveau groupe publiée, aucune limite ni seuil ni test changé. Règles : `regles-de-gestion/WFS-427.md`. Exception à « un ticket actif par épic » : WFS-412 (E7) est en cours dans une autre session, sur d'autres fichiers (`tests_cpu.sh`, étape des tests). Gel de la CI : correction d'un rouge, permise.
- **WFS-428** `[TECHNIQUE]` P1, E13, ⏳ Prêt : « Banc `realisme-2` rouge : face +Z de nuit hors tolérance (M-01) et capture d'éclairage abandonnée en Campagne » (ANO-245, ANO-246). Un seul ticket pour les deux erreurs (même banc, même reproduction) ; RG-08 prévoit la coupe si les deux causes sont sans lien. Interdits : élargir la tolérance, retirer la garde de WFS-213, annuler du code fusionné. Suspects cités : WFS-382, WFS-381, décors E15 ; pour la Campagne aussi WFS-413, 380, 337, 373, 402 / 415, 416. Règles : `regles-de-gestion/WFS-428.md`.
- **WFS-377** (choix « Ultra » du banc, ANO-211) : **P2 → P1**, règles inchangées.
- **Ordre** : WFS-428 puis WFS-377 vont aux prochaines sessions `@dev` qui demandent un ticket, en parallèle de la file E4 de l'ordre des 20 (WFS-423, 422…), car ce sont d'autres épics (E13, E0). L'ordre des 20 de l'utilisateur n'est pas changé. **Pour l'utilisateur** : 2 sessions `@dev` à ouvrir pour ces deux tickets ; tant que `realisme-2` est rouge, aucun ticket 🟣 ne passe ✅.
- **À suivre, sans ticket pour l'instant** (constat de l'agent expert ci cd) : le banc `hud` met 34 min pour 22 parties et 29 images ; le chargement d'une carte s'est allongé d'environ 11 % depuis le run #5 (4e7fa716), sur le banc PC. Cela touche aussi le joueur. Le PO regarde la durée de chargement aux prochains bancs du matin et sur les mesures du téléphone (écran de chargement, WFS-420) ; si l'allongement se confirme, ticket de performance du chargement.
- `track-agent.md` remis à jour : ligne de Claude, agent dev (01UY1Vv4ss) enlevée (WFS-421 fusionné, session close) ; dernier envoi des branches : WFS-412 12:49 (75a99915), WFS-426 12:54 (37749ba6), WFS-383 12:25 (e59ea1e6), WFS-425 12:22 (4970e806), WFS-418 12:41 (f76ca05c).

### 2026-11-11 12:59:48 — WFS-427 : correction poussée, bancs du matin lancés sur la branche (agent expert ci cd, session 01UY1Vv2Hq)
- **Branche** `feature/waforspeed-427` poussée (adf935fa) : `tools/qa/banc_groupe.sh` (le banc `fluidite` sort d'`ecrans-2400` et devient le groupe `fluidite`), `.github/workflows/matin.yml` (matrice et liste `GROUPES` : 18 groupes), doc du skill `expert-ci-cd`. Aucune limite, aucun seuil, aucun test changé ; aucun fichier du jeu.
- **Mesures qui fondent la correction** : sur la CI (run #16, job 130415389361), `fluidite` avance à 5,1 s par partie (parties 402 à 417 horodatées), soit ≈ 48 min pour ses 568 parties ; en local sur `main` : `chargement` 65 s, `hud` 2059 s, `fluidite` 2321 s, `camera` 233 s, tous en code 0. Version du run #5 (4e7fa716), même machine : `hud` 1860 s.
- **CI de branche (US technique, RG-08)** : `matin.yml` lancé à la main sur la branche, **run #19** (https://github.com/wlfaleh/waforspeed_mobile/actions/runs/43025086545). Il vérifie la durée d'`ecrans-2400` et de `fluidite` sur la vraie CI (moins de 75 min chacun) et que `camera` est rejoué. Hors de `main` : rien n'est publié (ni captures, ni APK, ni message à la QA) et le robot des statuts ne regarde que `main` : aucun ✅ donné. `realisme-2` y sera rouge (WFS-377, WFS-428 : code du jeu, hors de ce ticket).
- Les deux groupes tournent aussi en local (journaux pour `qa/pr/WFS-427/pr.md`).

### 2026-11-11 13:06:43 — WFS-429 créé : boutons Caméra et Menu à côté du score, +5 % (po-scrum-master)
- **Demande de l'utilisateur** (session du PO) : « fonctionnalité simple, dans les 20 premières : déplacer les boutons caméra et menu, les mettre côte à côte avec le score et les agrandir de 5 % ».
- **WFS-429** (P1, E5 + E8, + E3 exception PO : `camera_vue.cpp` `drawUi`) ⏳ Prêt, règles `regles-de-gestion/WFS-429.md` : rangée [MENU] [CAMÉRA] [SCORE] en haut à droite, toutes les dimensions ×1,05 (40 → 42 px), zones de toucher qui suivent, rien ne recouvre le GPS, le combo ni les notifications, vue Cockpit et Poursuite traitées.
- **Ordre des 20** : WFS-429 en **8e**, pour une nouvelle session `@dev`, en parallèle de la file E4 (autres fichiers) ; WFS-397 passe au 21e rang.
- **Exception à l'ordre permanent « taille des boutons » (2026-11-06)** : demandée par l'utilisateur, limitée à ces deux boutons ; notée dans les ordres permanents du PO.

### 2026-11-11 13:08:39 — WFS-429 : pas de validation sur téléphone, captures suffisantes (po-scrum-master)
- Remarque de l'utilisateur : un petit changement comme celui-là se valide sur une capture du jeu. Erreur du PO reconnue : la règle 🟡 (2026-11-07) ne vaut que pour un défaut vu sur le téléphone ou la conduite. WFS-429 suit le cycle normal : captures avant / après regardées par le lead dev → 🟣 → ✅ (bancs du matin). Règles mises à jour.

### 2026-11-11 13:18:00 — WFS-364 : branche poussée, retouches de la Golf et de la Corsa faites (Claude, agent dev3d (01UY1Vv2nQ))
- Fait : Golf, emblème rond (disque noir de WFS-404, 4 cm devant le bouclier), bandeau lumineux, liseré rouge et plaque GTI de la fente entre les phares retirés par découpe exacte ; encoche ronde de la peinture refermée par une peau cousue ; grille générique à 2 lames dans la fente. Corsa, nid d'abeille, barres chromées en V et porte-emblème de la calandre retirés ; grille générique à 4 lames ; relief du centre du capot lissé (nouvelle retouche `lisser` de `tools/retoucher_formes.mjs`, hauteur ajustée rangée par rangée sur la peau voisine).
- Vérification en cours : `--verifier` vert pour les 9 voitures ; Golf et Corsa rejouées depuis l'origine (octet pour octet) ; Giulia et Range Rover rejouées depuis leur modèle d'avant WFS-363 : identiques à `main`. Tests CPU et captures avant / après en cours.
- Outil : dépendances de `tools/package.json` installées en local (`npm install --no-save`, rien d'ajouté au dépôt) pour régénérer les LOD.

### 2026-11-11 13:24:10 — WFS-425 : PR #348 ouverte, lead dev appelé (Claude, agent dev (01Us6uCtwf))
- **Fait** : en MSAA ×1, la passe de scène suit le schéma du ×2. Sous-passe 0 : couleur et profondeur temporaires (mémoire de tuile). Sous-passe 1 : copie dans la cible HDR (attachement d'entrée, `BY_REGION`). Les shaders n'écrivent plus jamais directement dans la cible HDR. ×2 et ×4 : passe identique au bit près (test). MSAA par défaut de la Basse inchangé (×2). La passe finale n'est plus recréée à chaque changement de MSAA (avant : l'ancienne n'était jamais détruite).
- **Fichiers** : `src/render/passe_scene.h` / `.cpp` (nouveaux), `src/render/renderer.cpp` / `.h`, `shaders/scene_copie.frag` (nouveau), `tests/test_passe_scene.cpp` (nouveau), `CMakeLists.txt` (2 lignes : shader et test).
- **Vérifié** :
  - 104 tests CPU verts.
  - Couche de validation Vulkan (synchronisation comprise), ×4 puis ×1 en course : 0 message.
  - 18 captures PNG avant / après identiques pixel par pixel : Ville, Corniche et Campagne de nuit, MSAA du niveau et ×1.
  - Banc PC, ×1, avant → après : Ville Basse 190 → 185 ms, Corniche Basse 133 → 138 ms (bruit ≈ ±5 %).
- **Outils installés** (registre de paquets Ubuntu, pour le banc local ; rien n'est ajouté au dépôt) : `mesa-vulkan-drivers`, `glslc`, `libvulkan-dev`, `vulkan-tools`, `vulkan-validationlayers`, `ccache`.
- **Limite** : le gain n'est pas mesurable sur le PC (lavapipe n'a pas de tuiles). La cause n'est confirmée que si le ×1 devient plus rapide que le ×2 sur le S21 FE. La mesure à faire est décrite dans la PR. Statut après la fusion : 🟡 À valider (utilisateur).

### 2026-11-11 13:26:54 — WFS-412 : PR #349 ouverte (agent expert ci cd (session 01UY1Vv4jR6p))
- Essai local sur les vrais tests compilés, `test_audio` rendu rouge exprès hors commit : 103 autres tests lancés, liste finale « test_audio », licences vérifiées (0 manque), code 1 (`qa/pr/WFS-412/extrait-2-essai-local-rouge.log`). Run de branche #1000 : étape des tests verte. Capture n° 3 sans objet (`test_course5_ordinateurs` vert sur `main` depuis WFS-415).
- `TACHES.md` 🔀 En PR (#349), `suivi-pr.md` 🆕, lead dev appelé. À savoir pour WFS-427 (bancs du matin, autre session expert ci) : cette PR change les 3 lignes des tests CPU de `matin.yml` (≈ l. 132), rien d'autre.

### 2026-11-11 13:59:11 — WFS-324 : 2e refus de la PR #341 corrigé (Claude, agent dev3d (01UY1Vv5YT))
- Point bloquant, les phares de la voiture qui suit, la nuit, dans les glaces : un cœur et une lueur sur chaque optique mesurée par WFS-413, dessinés dans l'image d'une glace seulement. En vue directe, rien ne change (WFS-413 intact).
  - Accroche dans `Renderer::feuxVoiture`, annoncée ici.
  - Un retrait par méprise (3946d1d9 : j'avais pris une question de l'utilisateur pour un ordre) a été annulé (8726d790) ; l'utilisateur confirme que c'est un ajout dans les glaces, pas une modification des voitures.
- Capture des Réglages : réglage Caméra sur Cockpit. La scène « voie de gauche » du banc est à 14 m (à 9 m, la voiture est dans l'angle mort).
- Vérification :
  - tests : 105 / 105 ;
  - banc `retroviseurs` : 0 écart (Golf, Range Rover Sport, M4, 911) ;
  - banc `cockpit` de nuit : sans régression ;
  - coût : 0,43 img/s en Basse, 0,46 en Haute (estimé).
- Commit à relire : 72e5ce51 ; lead dev rappelé.

### 2026-11-11 13:59:26 — WFS-425 fusionné par le lead dev (lead-dev (session 01Us6uCtwf))
- PR #348 relue sur 51f14f8a (Claude, agent dev (01Us6uCtwf)), **fusionnée en squash : 01712be4**. GitHub refuse l'approbation (même compte) : revue « ✅ Approuvé par le lead dev » écrite en commentaire dans la PR.
- Lu : tout le diff (`passe_scene.h/.cpp`, `renderer.cpp/.h`, `scene_copie.frag`, `test_passe_scene.cpp`, `CMakeLists.txt`) ; 7 paires de captures ouvertes, identiques. RG-01 (cause écrite : passe ×1 qui écrivait directement dans la cible HDR, hypothèse du rendu hors tuiles de l'Adreno, par élimination), RG-03 (×2 / ×4 identiques au bit près, test), RG-04 (×2 par défaut en Basse) : OK. RG-02 : à mesurer par l'utilisateur sur le S21 FE.
- Refait : construction, `tests_cpu.sh` 105 verts ; banc `perf` Ville de nuit en ×1 (couche de validation + synchronisation : 0 message ; PNG identiques à `main`) ; endurance en ×1 avec validation (code 0, 0 erreur, objets Vulkan stables).
- Remarque non bloquante écrite dans la revue : `prechaufferMsaa` (renderer.cpp:937-939) détruirait la copie ×1 en service si la passe en service était en ×1 pendant une variante ×2 / ×4 (impossible aujourd'hui ; à reprendre si `diagnostic.msaa_x1` est appliqué dès le chargement, suite proposée au PO par le développeur).
- `TACHES.md` : WFS-425 → 🟡 À valider (utilisateur) (PR #348) ; `suivi-pr.md` : ligne supprimée ; `qa/Check.md` : ligne ajoutée.

### 2026-11-11 14:00:07 — WFS-364 : PR #351 ouverte, accroche de 10 lignes annoncée (Claude, agent dev3d (01UY1Vv2nQ))
- Fait : Golf (emblème rond, bandeau lumineux, liseré rouge et plaque de la fente retirés ; encoche de la peinture refermée par une peau cousue ; grille à 2 lames) ; Corsa (calandre en nid d'abeille, barres en V et porte-emblème retirés ; grille à 4 lames ; relief du capot lissé par la nouvelle retouche `lisser`).
- **Accroche hors zone (10 lignes)** : `src/render/correctifs_voitures.h`, nombre de matériaux des optiques avant de la Golf (21 → 23) et de la Corsa (17 → 19), car chaque grille ajoute 2 matériaux à la fin ; rangs et zones inchangés. Sans elle, `test_phares_optiques` échoue.
- Vérification : 104 tests CPU réussis ; `--verifier` vert pour 9 voitures ; Golf et Corsa rejouées octet pour octet ; Giulia et Range Rover rejouées depuis leur origine : identiques à `main` ; 16 captures vitrine avant / après + 20 zooms ×4 jour et nuit regardés.
- **Pour le PO (RG-06, tâche séparée proposée)** : le liseré rouge de la Golf continue en fine ligne dans les deux phares, imprimé dans la texture de la lampe (matériau 10, texture 8) ; à repeindre par `tools/effacer_logos.mjs` / `tools/logos.json`, hors de la zone de WFS-364.

### 2026-11-11 14:00:33 — WFS-425 : fin de session après la fusion (#348) (Claude, agent dev (01Us6uCtwf))
- PR #348 fusionnée en squash par le lead dev (01712be4). Statut : 🟡 À valider (utilisateur). J'ai répondu sous la revue, point par point (3 remarques non bloquantes).
- **Pour le PO** :
  - Remarque n° 1 du lead dev : `prechaufferMsaa` détruirait la copie ×1 en service si la passe en service était en ×1 pendant la préparation d'une variante ×2 / ×4. Ce cas n'existe pas aujourd'hui (la fonction n'est appelée qu'au chargement). À corriger dans la même US que le constat « `diagnostic.msaa_x1` lu dans `configgame.json` ne prend effet qu'au prochain changement de cibles » (entrée du journal de 13:00 environ), si cette US est créée : `copieX1_.pipeline = VK_NULL_HANDLE` avant `createScenePipelines()` dans la boucle.
  - L'émulateur des bancs du matin (GPU logiciel, toujours en ×1) fait maintenant une copie plein écran en plus : à regarder.
  - Si la mesure du téléphone ne montre pas un ×1 plus rapide que le ×2, la cause supposée (rendu hors tuiles du pilote Adreno) est fausse. La suite proposée est une pré-passe de profondeur pour le ×1.
- Question de mesure posée à l'utilisateur dans `tache utilisateur.md` (une seule).
- Titre de la discussion non changé : l'outil `set_session_title` n'existe pas dans cette session (titre voulu : « ⏹️ Terminée — Claude, agent dev (01Us6uCtwf) (WFS-425 fusionné) »).
- Session terminée : un seul ticket par session, je ne demande pas d'autre ticket.

### 2026-11-11 14:01:09 — WFS-324 : 3e revue de la PR #341, refusée (lead-dev)
- Commit relu 72e5ce51. Réglés : phares allumés visibles dans les 3 glaces à taille réelle, réglage Caméra sur Cockpit pour la capture des Réglages.
- **Refus** : en gros plan, la lueur de 25 cm ajoutée sur chaque phare (`PHARE_LUEUR`, `feuxVoiture`) est un disque gris pâle au bord net (Golf), ou un demi-disque plat (Range Rover Sport) : halo en disque interdit (règle du 2026-11-09). Attendu : cœur net gardé, aucune forme ronde lisible.
- Branche non reconstruite (refus sur les images).

### 2026-11-11 14:01:43 — WFS-324 : 3e refus de la PR #341 noté (po-scrum-master)
- Noté sur message du lead dev : PR #341 (dev3d, 01UY1Vv5YT) refusée 3 fois ; reste seulement le halo en disque des phares dans les glaces. Statut inchangé (↩️ Retour lead dev). Pas de réattribution (règle des 10 h). WFS-325 attend WFS-324.
- WFS-421 complété par la PR #347 (disque au sol des lampadaires enlevé à toutes les qualités) : la réserve de l'agent 3D sur #346 est traitée, aucune anomalie créée.
### 2026-11-11 14:02:16 — Épic E16 « Évolution des objets 3D » créée, WFS-430 à 443 bloqués par l'utilisateur (po-scrum-master)
- Ordre de l'utilisateur (2026-11-11) : créer les US dans une épic « évolution des objets 3D », en bloqué, pas prioritaire, en dernier ; il dira quand commencer.
- Créés : `regles-de-gestion/E16-evolution-objets-3d.md` (but, zone `src/world/biblio_*`, `assets/world/biblio/**`, `tools/biblio/**`, `tests/biblio*`, accroche renderer ≤ 10 lignes ; porte utilisateur après le pin d'Alep ; 33 ms S21 FE ; avis agent 3D ; avis lead dev et smart brain ; lots 2 à 10 en prévision non créée : ≈ 105 objets, 54,5 sessions, grand total ≈ 70,5), `regles-de-gestion/E16-inventaire.md` (copie de l'inventaire), `regles-de-gestion/WFS-430.md` à `WFS-443.md`.
- Numéros WFS-430 à 443 vérifiés libres. Statut de tous : ⛔ Bloqué par l'utilisateur, P4 Basse, en dernier du tableau, dans aucun ordre de priorité, aucun attribué.
- WFS-383 et WFS-371 inchangés (au déblocage, WFS-371 serait réduit s'il n'est pas fait : noté dans l'épic).
### 2026-11-11 14:13:36 — WFS-426 : PR #350 refusée par le lead dev (lead-dev)
- Relu b1db4c34 : 22 captures ouvertes, diff complet lu, construction + tests CPU 104/104, bancs cuire / batiments (WFS_CARTE=plage) / paquets relancés (0 écart chacun).
- Refus : la cuisson de la Corniche (`batimentsCuits` → `plage::poserVillasKit` → `archi::carte::vider()`) vide le kit global de la partie en cours ; capture `paquets-menu-prete` : la Ville n'a plus ses immeubles derrière le menu (main : présents). Banc et captures faussés (pas d'effet dans le jeu du téléphone, qui ne cuit pas).
- Remarques : garde `cuisson_` à ajouter aux lampadaires de `placerEtabs` (plage.cpp:1586, 1589) ; `#include <memory>` mal placé ; noms `plage_*` au lieu de `corniche_*` (RG-02) à faire noter par le PO.

### 2026-11-11 14:14:02 — WFS-427 : PR #352 ouverte, les deux groupes verts sur la CI de la branche (agent expert ci cd, session 01UY1Vv2Hq)
- **Run #19 de la branche** (https://github.com/wlfaleh/waforspeed_mobile/actions/runs/43025086545, commit adf935fa) : `banc (ecrans-2400)` **vert en 61,3 min** (`chargement`, `hud`, `camera` rejoué), `banc (fluidite)` **vert en 35,6 min** ; limite 75 min inchangée. Les autres groupes terminés sont verts, sauf `realisme-2` (rouge attendu : WFS-377, WFS-428, code du jeu). Rien publié, aucun ✅ (hors de `main`).
- **En local** (`bash tools/qa/banc_groupe.sh`) : `ecrans-2400` code 0 en 2348 s, 25 fichiers ; `fluidite` code 0 en 2053 s, 568 parties, 49 fichiers, « objectifs manqués : 0 ». Mêmes 74 fichiers qu'avant, mêmes noms.
- **PR #352** ouverte (f86da0ee, synchronisée avec `main`), `qa/pr/WFS-427/pr.md` (journaux + 3 captures du banc `camera`), ligne 🆕 dans `suivi-pr.md`, ticket 🔀 En PR ; lead dev appelé.
- **Point à suivre pour le PO** : `ecrans-2400` reste à 61 min sur la CI (marge de 14 min) à cause du banc `hud` ; s'il s'allonge encore, il faudra le sortir à son tour.

### 2026-11-11 14:14:31 — WFS-364 fusionné par le lead dev (lead-dev)
- PR #351 relue sur c09d7ab8 : 52 captures ouvertes (26 paires avant / après : vitrines de face, de 3/4, vues basses, en course ; zooms ×4 de jour et de nuit) ; tout le diff lu (`tools/retoucher_formes.mjs`, retouche `lisser` ; `tools/formes.json` ; accroche de 10 lignes dans `src/render/correctifs_voitures.h`, annoncée au journal).
- Vérifié : fusion locale avec `main` (WFS-425 compris, sans conflit), construction et `tools/qa/tests_cpu.sh build` : 105/105 ; `test_fidelite_voitures` (Golf ×1,001, Corsa inchangée) ; `test_phares_optiques` (optiques trouvées) ; `retoucher_formes.mjs --verifier` vert (9 voitures) ; Golf et Corsa rejouées depuis les modèles de `main` : `.wfm` et `.lod` identiques à l'octet.
- Revue « ✅ Approuvé » écrite dans la PR en commentaire (GitHub refuse l'approbation au même compte), puis squash b9e8c47. Statut 🟣 Fusionné.
- Pour le PO : (1) liseré rouge imprimé dans la texture des phares de la Golf → tâche séparée (`tools/logos.json`, RG-06) ; (2) marche d'environ 1 mm au raccord de l'encoche de la Golf ; (3) petit disque sombre sur le hayon de la Golf, déjà sur `main`, hors RG-01 (avant).

### 2026-11-11 14:15:26 — WFS-364 fin, session terminée (Claude, agent dev3d (01UY1Vv2nQ))
- PR #351 fusionnée en squash par le lead dev (b9e8c47) ; ticket 🟣, la suite (✅ ou 🟡) revient au robot des statuts et au PO.
- Petits défauts restants, pour le PO (RG-06, tâches séparées) : liseré rouge dans la texture des phares de la Golf (`tools/logos.json`) ; marche d'environ 1 mm au raccord de l'ancienne encoche de la Golf ; petit disque sombre sur le hayon de la Golf (déjà sur `main`, hors du périmètre).
- Session terminée (un seul sujet par session) : aucun autre ticket demandé.

### 2026-11-11 14:20:55 — WFS-412 fusionné par le lead dev (lead-dev)
- PR #349 (agent expert ci cd (session 01UY1Vv4jR6p)), revue du commit 7b107c63, squash **ae7089ee** sur `main`. GitHub refuse l'approbation (même compte) : revue « ✅ Approuvé par le lead dev » postée en commentaire.
- Diff entier lu (`tools/qa/tests_cpu.sh`, 3 lignes de `build.yml` et de `matin.yml`) : RG-01 à RG-05 tenues, aucun test, seuil ni `IGNORES` changé. Extraits relus ; run n° 1000 vérifié vert sur GitHub.
- Refait : faux dossiers (tout vert code 0 ; 2 / 4 échecs listés, code 1 ; vide → « aucun test trouvé », code 1) ; construction de la branche fusionnée avec `main` : 105 tests CPU verts, licences 0 manque, code 0.
- À surveiller : bancs du matin du 12/11 (1er passage du bloc de `matin.yml`) ; WFS-427 doit se synchroniser sur ces lignes.

### 2026-11-11 14:21:32 — WFS-412 fusionné (#349, ae7089ee), session terminée (agent expert ci cd (session 01UY1Vv4jR6p))
- Lead dev : PR acceptée et fusionnée en squash. Sur `main` à la prochaine fusion (`build.yml`) et aux bancs du matin du 12/11 (`matin.yml`) : un test CPU en échec n'empêche plus les autres tests ni la vérification des licences ; liste des échecs en fin de journal ; job toujours rouge s'il y a un échec.
- Pour WFS-427 (autre session expert ci, `matin.yml`) : se synchroniser avec `main` (3 lignes des tests CPU changées, ≈ l. 132).
- Un seul ticket par session : cette session s'arrête, aucun autre ticket demandé.

### 2026-11-11 14:30:13 — WFS-427 fusionné par le lead dev (lead-dev)
- PR #352 (agent expert ci cd (session 01UY1Vv2Hq)), revue du commit f86da0ee, squash **2ccc37c7** sur `main`. GitHub refuse l'approbation (même compte) : revue « ✅ Approuvé par le lead dev » postée en commentaire.
- Diff entier lu (`tools/qa/banc_groupe.sh`, matrice / `GROUPES` / « 18 groupes » de `matin.yml`, doc du skill) : RG-01 à RG-09 tenues, limite de 75 min, seuils et tests inchangés. 3 captures du banc `camera` ouvertes : rien d'anormal. Run #19 de `matin.yml` vérifié sur GitHub : `ecrans-2400` vert en 61,3 min, `fluidite` vert en 35,6 min ; `realisme-2` rouge hors ticket (WFS-377, WFS-428).
- Refait : branche fusionnée avec `main` en local, construction et 105 tests CPU verts ; `test_matin.py` vert. Après le squash (WFS-412 fusionné entre-temps) : `matin.yml` de `main` relu, les deux changements sont là, 18 groupes, limite 75.
- **Pour le PO (défaut ancien, hors de cette PR)** : dans `tools/qa/banc_groupe.sh`, un banc placé au milieu d'une liste `mkdir … && ./build/wfs_bench … && cp …` qui échoue n'arrête pas le script malgré `set -e` (règle de bash) : le job peut finir vert, captures du banc absentes. Exemple : `bash -c 'set -e; true && (exit 3) && echo copie; echo suite'` affiche « suite », code 0. Lignes concernées : `chargement`, `hud`, `fluidite`, `camera`, `conduite`, etc. Je recommande une US technique P1 (sortir le code du banc de la liste `&&`, sans rien d'autre changer).
- À surveiller : bancs du matin du 12/11 (les deux groupes verts en moins de 75 min, archive `captures-fluidite.zip`) ; marge d'`ecrans-2400` (61 min sur 75).

### 2026-11-11 14:30:53 — WFS-427 fusionné : session terminée (agent expert ci cd, session 01UY1Vv2Hq)
- PR #352 fusionnée en squash par le lead dev (2ccc37c). Ma ligne de `track-agent.md` passe en « ⏹️ session terminée » ; je ne demande aucun autre ticket (un seul sujet par session).
- **Reste à constater** : `ecrans-2400` et `fluidite` verts aux bancs du matin de `main` du 2026-11-12 (moins de 75 min chacun), archive `captures-fluidite.zip` publiée.
- **Les bancs du matin resteront rouges** tant que `realisme-2` n'est pas réparé : WFS-377 et WFS-428 (P1, ⏳ Prêt, code du jeu), à donner à deux sessions `@dev`.
- **Pour le PO** (remarque du lead dev, défaut ancien) : dans `tools/qa/banc_groupe.sh`, un banc en échec au milieu d'une suite `… && … && cp …` n'arrête pas le script malgré `set -e` (seule la dernière commande d'un groupe rend le job rouge) : US technique à créer. Autre point : `ecrans-2400` à 61 min sur la CI, marge de 14 min (banc `hud` lent).

### 2026-11-11 14:32:17 — WFS-426 : écart RG-02 noté (paquets `plage_*`) (po-scrum-master)
À la demande du développeur (agent cartes, PR #350), écart accepté par le lead dev ajouté sous RG-02 de `regles-de-gestion/WFS-426.md` : paquets `plage_astc/etc2/rgba8.wfcarte` (identifiant « plage » du format WFS-301), nom affiché « Corniche ». Statut et ligne du développeur inchangés.

### 2026-11-11 14:58:16 — WFS-428 attribué à Claude, agent dev (01UY1Vv5Nm) (po-scrum-master)
Nouvelle session `@dev` (session_01UY1Vv5NmAE439dke3iNvDi, aucun ticket avant : 1er et seul sujet). Ticket donné : **WFS-428** (P1, [TECHNIQUE], E13) : banc `realisme-2` rouge (M-01 face +Z de nuit −6,34 %, capture d'éclairage abandonnée en Campagne ; ANO-245, 246), ordre de l'utilisateur « Répare les bancs du matin ». Choix : WFS-423 / 422 (E4) restent derrière WFS-324 (↩️, E4) ; WFS-377 (E0) attend, E0 tenu par WFS-418. Zone : `src/render/environnement.*`, `src/render/eclairage_physique.*` + fichier de la cause annoncé au journal ; pas le bloc RG-04 / P-05 de `bench_passes.cpp` (WFS-377), ni `passe_scene.*`, `createTargets`, `vk.*`, la CI. Règles : `regles-de-gestion/WFS-428.md`. US technique : CI de branche permise, une à la fois. Lignes des sessions terminées retirées de `track-agent.md` (WFS-412, 427, 425, 364).

### 2026-11-11 14:59:22 — WFS-426 approuvé, fusion bloquée par un conflit de journal (lead-dev)
- PR #350, commit a883c89a relu : réponses « corrigé dans e3650201 » présentes ; tests CPU 105/105 ; banc paquets 0 écart (Ville 625 / 625 bâtiments après la cuisson de la Corniche) ; cuisson 0 écart, 6 paquets identiques à l'octet à la revue précédente ; captures `paquets-menu-prete` (immeubles de la Ville revenus) et `paquets-course-pendant` ouvertes. Approbation écrite dans la PR (commentaire, même compte GitHub).
- Fusion refusée par GitHub : conflit dans `journal.md` (9a6789c0). Le développeur synchronise avec main et rappelle le lead dev ; l'approbation tient si seuls `journal.md` et les fichiers de suivi changent.

### 2026-11-11 14:59:28 — WFS-365 attribué à Claude, agent dev3d (01UY1Vv3Ji) (po-scrum-master)
- Demande d'une nouvelle session `dev-artiste-3d` (session_01UY1Vv3Ji…, aucun ticket avant : 1er et seul sujet). Ordre des 20 du 2026-11-11 12:08 UTC, côté `@devartiste` : 10. WFS-383 pris (01UY1Vv1zt) ; 11. WFS-371 attend WFS-383 (même file Corniche E15) ; 12. WFS-325 attend WFS-324 (↩️ PR #341) ; 13. WFS-395 attend WFS-359 (🟣) / 349 / 390 (🟡, habitacle `vehicule_cockpit*`, même zone que WFS-324) ; 14. WFS-364 🟣 fusionné (#351) → **15. WFS-365** (P1, E10), premier attribuable.
- Début : WFS-365, retouches de forme de la 911 et de la Gallardo ; épic E10 (pas E15 : pas d'avis agent 3D obligatoire) ; branche `feature/waforspeed-365`. Exception PO E10 (deux tickets voitures au plus) : en parallèle de WFS-324 (E4 + E10, `vehicule_cockpit*`). Zone : `assets/cars/p911*`, `assets/cars/gallardo*`, entrées `p911` et `gallardo` de `tools/formes.json`, ajouts à `tools/retoucher_formes.mjs` sans changer les voitures déjà faites (`--verifier`). Interdit : `vehicule_cockpit*`, passe des reflets (WFS-324), `src/game/cars.h`, fichiers des sols / Corniche (WFS-383), autres voitures. Règles : `regles-de-gestion/WFS-365.md`.

### 2026-11-11 15:00:26 — WFS-429 attribué à Claude, agent dev (01UY1Vv3Vj) (po-scrum-master)
Début : **WFS-429** (P1, 8e de l'ordre des 20) « HUD : boutons Caméra et Menu à côté du score, +5 % », donné à la nouvelle session `@dev` (session 01UY1Vv3Vj, jamais eu de ticket) comme prévu dans `tache utilisateur.md`. Épic E5 + E8 (+ E3 exception : `camera_vue.cpp`, `drawUi` et zone de toucher seulement) ; aucun autre ticket actif sur ces épics. Zone : `src/ui/hud.cpp`, `src/ui/ecrans/pause.cpp`, `src/game/camera_vue.cpp` (partie bouton), accroches de bancs ≤ 10 lignes annoncées. Règles : `regles-de-gestion/WFS-429.md` (RG-01 à RG-10). Branche `feature/waforspeed-429`. Pas de 🟡 (validation par captures, lead dev).
### 2026-11-11 12:23:45 — WFS-426 : branche créée (Claude, agent cartes (01UY1Vv2HrQF))
- Branche `feature/waforspeed-426` créée depuis `main` (c96e1f08) et poussée ; étude de la cuisson de la Ville (WFS-301, 302, 416, 307) pour y ajouter la Corniche.

### 2026-11-11 13:38:35 — WFS-426 : Corniche cuite en paquet, bancs et captures (Claude, agent cartes (01UY1Vv2HrQF))
- Fait : villas de la Corniche posées sans carte graphique (`plage::poserVillasKit`, même pose que l'init) et cuites dans le bloc `batiments` ; `WFS_BANC=cuire` cuit Ville + Corniche ; `paquets.yml` (6 noms exacts, relu par l'agent expert ci cd : OK) et `publier_paquets.sh` (`--nom plage=Corniche`) ; banc « batiments » sur la Corniche (`WFS_CARTE=plage`).
- Tailles : plage_astc 54,6 Mo, plage_etc2 38,2 Mo, plage_rgba8 59,2 Mo (bâtiments 32,9 Mo, 172 villas) ; deux cuissons identiques à l'octet.
- Vérifié : tests CPU 103/103 ; banc batiments Corniche 0 écart, 0 construction en course avec le paquet (50 et 160 km/h), bloc refusé → secours sans plantage ; banc paquets 0 écart : parcours terrain → télécharger la Corniche → course sur la Ville pendant le téléchargement → course sur la Corniche avec son paquet → supprimer → course en secours.
- Limites : fichiers nommés `plage_*` (identifiant du code) au lieu de `corniche_*` (signalé au PO) ; mesure S21 FE (≤ 28 ms) à faire sur le téléphone ; publication = lancer paquets.yml à la main après la fusion.

### 2026-11-11 14:32:16 — WFS-426 : correction après le retour du lead dev (PR #350) (Claude, agent cartes (01UY1Vv2HrQF))
- Cause racine : `batimentsCuits` vidait la liste globale du kit (`g_bats`, `g_cuit`) de la carte chargée ; le banc `paquets` cuit la Corniche pendant une partie sur la Ville → immeubles de la Ville effacés (capture `paquets-menu-prete`).
- **Accroche annoncée (zone partagée, 6 lignes)** : `src/world/archi_carte.{h,cpp}` : `mettreDeCote()` / `remettre()` (échange des tampons, aucune copie), appelés autour de la cuisson dans `batimentsCuits`.
- Contrôle ajouté au banc `paquets` : bâtiments de la Ville avant / après la cuisson de la Corniche 625 / 625, même signature : OK ; banc `paquets` 0 écart ; tests CPU 104/104 ; cuisson 0 écart, paquets identiques à l'octet.
- Aussi : lumières des bars et du casino non déclarées en cuisson ; `<memory>` avec les en-têtes standard ; captures paquets-* refaites (+ `paquets-course-pendant`).

### 2026-11-11 15:02:15 — WFS-426 fusionné par le lead dev (lead-dev)
PR #350 fusionnée en squash (7aa1535) sur la tête a3857a34 (expectedHeadSha). Approbation donnée sur a883c89a ; les têtes 62d02924 puis a3857a34 n'apportent que la synchronisation avec main (aucun fichier de code changé, vérifié). La 1re tentative sur 62d02924 avait échoué sur un conflit de journal.md. Ticket en 🟡 À valider (utilisateur), ligne retirée de suivi-pr.md, ligne ajoutée à qa/Check.md.

### 2026-11-11 15:02:50 — WFS-426 : fusionné, session terminée (Claude, agent cartes (01UY1Vv2HrQF))
- PR #350 fusionnée en squash par le lead dev (7aa1535) après un refus corrigé (cuisson qui vidait les bâtiments de la partie en cours) ; ticket 🟡 À valider (utilisateur).
- Reste : lancer paquets.yml sur main (accord de l'utilisateur demandé dans `tache utilisateur.md`) ; mesure S21 FE par l'utilisateur.
- Leçon : une branche ne doit pas garder d'entrées de `journal.md` (conflit à chaque écriture d'un autre agent) : les reporter sur `main` avant la fusion.
- Session arrêtée (un seul sujet par session).

### 2026-11-11 15:03:37 — WFS-324 : 3e refus de la PR #341 corrigé (Claude, agent dev3d (01UY1Vv5YT))
- Phares dans les glaces : plus de lueur en disque (règle « pas de halo en disque »). Il reste un point blanc net de 7 cm, très vif ; l'éblouissement de l'image fait la lumière. En vue directe, rien ne change (WFS-413).
- Glace intérieure des SUV : visée plus basse quand l'œil est haut (5,1° pour le Range Rover Sport), pour que les deux phares de la voiture qui suit soient dans la glace. Golf et M4 sont inchangées.
- Vérification : 106 / 106 tests ; banc `retroviseurs` 0 écart ; banc `cockpit` de nuit sans régression ; coût 0,97 img/s en Basse, 0,62 en Haute (estimé).
- Commit à relire : 234f472f. Lead dev rappelé.

### 2026-11-11 15:05:29 — Statuts automatiques, CI main #1005 (robot statuts)
- Run rouge (push, commit 2ccc37c), aucun statut changé. Job(s) en échec : compilation.
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-406, WFS-396, WFS-362, WFS-363, WFS-364, WFS-384, WFS-412, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-11 15:11:57 — WFS-324 fusionné par le lead dev (lead-dev)
PR #341, 4e revue sur 234f472f : la lueur en disque a disparu, il reste un point net sur chaque phare. Les deux points du Range Rover Sport sont visibles. Captures ouvertes une à une, diff relu depuis 72e5ce51 (f1c87069), construction et tests refaits : 106/106. « ✅ Approuvé » en commentaire, fusion en squash 6b476114. Statut 🟡 À valider (utilisateur), ligne ajoutée à qa/Check.md, ligne retirée de suivi-pr.md.

### 2026-11-11 15:12:34 — WFS-324 fusionné (#341), session terminée (Claude, agent dev3d (01UY1Vv5YT))
- Fusionné en squash par le lead dev à la 4e revue (6b476114). Ticket en 🟡 À valider (utilisateur), réponse ajoutée dans `tache utilisateur.md`.
- Pour le PO, hors de ce ticket :
  - l'œil du cockpit est placé 50 à 75 cm trop en avant dans les modèles 3D des voitures : les glaces calculent le reflet avec un décalage pour le compenser ;
  - M4 de nuit : sa plage arrière cache, dans la glace intérieure, les phares bas de la voiture qui suit ;
  - le coût des rétroviseurs est à confirmer sur le téléphone (0,97 img/s estimé en Basse).
- Session terminée (un seul sujet par session) : aucun autre ticket demandé.

### 2026-11-11 15:21:32 — Statuts automatiques, CI main #1006 (robot statuts)
- Run rouge (push, commit 7aa1535), aucun statut changé. Job(s) en échec : compilation.
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-406, WFS-396, WFS-362, WFS-363, WFS-364, WFS-384, WFS-412, WFS-427, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-11 15:22:24 — WFS-418 : PR #353 ouverte, lead dev appelé (Claude, agent dev (01Us6uCt95))
- PR #353 (`feature/waforspeed-418`, ef25158, synchronisée avec `main` d0715fa) : voie de repli de la mémoire graphique (tampons et images des 2 atlas), bâtiments cuits → kit de secours si une réservation est refusée, occlusion du kit gardée ; accroches annoncées au journal à 12:57:22 et validées par le PO.
- Vérifié (PC, lavapipe) : `tests_cpu.sh` 106 tests verts ; banc `batiments` (rapide) 0 écart : 5806 réservations des bâtiments cuits, refus n° 3 (atlas), 2904, 5806 (de nuit) → kit, aucun arrêt, mémoire graphique rendue ; 11 captures communes avant / après identiques au pixel ; `perf` réduit (Ville) : mêmes appels et triangles, temps dans le bruit ; `cuisson` 0 écart (parcours complet avec et sans paquet).
- Liste RG-03 (autres réservations sans repli, pour le PO) dans la description de la PR : décor de la Ville 373 Mo, trafic 316 Mo (secours existant : modèle sauté), voiture du joueur 63 Mo, piétons 33 Mo et conducteurs 15 Mo (secours existants), textures de la route (secours procédural), cibles et passes.

### 2026-11-11 15:29:00 — WFS-365 : arbitrage RG-01 (phares de la 911), WFS-444 créé (po-scrum-master)
- Question de Claude, agent dev3d (01UY1Vv3Ji) : les phares de la 911 ne se retouchent pas proprement (glace de 64 cm le long de l'aile ; ≥ 5 cm de déformation plisse l'aile et le capot, ≤ 3 cm ne se voit pas).
- **Décision** : proposition du développeur retenue. Dans WFS-365, pas de retouche des phares de la 911 (pas de déformation de 3 cm) ; 911 : bandeau arrière retiré (deux feux séparés) + une 2e retouche visible de face (ex. DEL de jour) ; Gallardo inchangée dans le plan. Détail dans `regles-de-gestion/WFS-365.md` (Suivi).
- **WFS-444** créé (E10, P2, ⏳ En attente après WFS-365, `dev-artiste-3d`, nouvelle session) : phares de la 911 remplacés, par remodelage de l'aile et du capot ou changement de modèle libre ; règles dans `regles-de-gestion/WFS-444.md`.

### 2026-11-11 15:34:16 — WFS-423 attribué à Claude, agent dev (01UY1Vv4BW) (po-scrum-master)
Nouvelle session `@dev` (session_01UY1Vv4BWx2FaXBYrKwi2KD, aucun ticket avant : 1er et seul sujet). Ticket donné : **WFS-423** (P1, [TECHNIQUE], E4) : lumières bornées en Basse (boucle des projecteurs au vrai nombre dans `eclairage.glsl` / `eau.glsl`, éclairement autour de chaque voiture calculé une fois sur le CPU).
- Choix : n° 3 de l'**ordre des 20** (12:08 UTC) ; WFS-421 fusionné, aucun autre ticket E4 🔧 / 🔀 (WFS-415 🟡). WFS-377 reste en attente (E0 tenu par WFS-418, 🔀 PR #353) ; WFS-422 suit WFS-423 dans la file E4.
- Zone : `shaders/eclairage.glsl`, `shaders/eau.glsl`, `shaders/vehicule.glsl`, code CPU qui remplit les données par voiture (`src/render/**`), `tests/**`. Règles : `regles-de-gestion/WFS-423.md` (RG-01 à RG-05, 3 captures demandées, gain mesuré, 🟡 après fusion ; « Avis agent 3D : VALIDÉ » si l'image change).
- Plan court : borner la boucle en Basse au nombre réel de projecteurs ; calcul CPU des 3 points devant chaque voiture passé au shader ; captures avant / après de nuit (Ville, Corniche) ; PR, lead dev.
- **CI de `main` rouge (#1005, #1006, compilation)** : panne réseau du lanceur GitHub (`Temporary failure resolving 'azure.archive.ubuntu.com'` à l'installation apt de glslc / mesa), pas un défaut du code ; pas d'US de code ; à relancer et suivre par l'agent expert ci (US technique si le rouge persiste au run suivant).
- `track-agent.md` : lignes des sessions terminées WFS-324 (01UY1Vv5YT) et WFS-426 (01UY1Vv2HrQF) enlevées (règle « agent fini = ligne enlevée »).


### 2026-11-11 15:35:33 — WFS-423 : branche créée, lecture du code (Claude, agent dev (01UY1Vv4BW))
- Branche `feature/waforspeed-423` créée depuis `main`, premier envoi. Lecture de `eclairage.glsl`, `eau.glsl`, `vehicule.glsl` et `src/render/nuit.cpp` avant de coder.

### 2026-11-11 15:56:14 — WFS-423 : code fait, tests CPU verts, accroche du banc annoncée (Claude, agent dev (01UY1Vv4BW))
- **RG-02** : `src/render/eclairement_voiture.h` (nouveau) calcule l'éclairement de la chaussée autour d'une voiture (même formule et mêmes 3 points que `eclairementRouteAutour`, répartition du feu de croisement par `phares::intensiteRelative`). En **Basse et de nuit**, `Renderer::draw` le calcule une fois par matrice de voiture (pièces peintes) et le passe au shader dans une nouvelle constante de poussée `PC::nuit` (128 octets : taille garantie par Vulkan). `vehicule.glsl` lit `pc.nuit` si `w = 1`, sinon garde son calcul (Moyenne, Haute, pièces hors voiture : même code qu'avant).
- **RG-01** : boucles des projecteurs de `eclairage.glsl` (`spotLights`, `spotLights2`) et de `eau.glsl` bornées au vrai nombre de lumières `n` (et au plafond par pixel en Basse), plus de `for (i < 12) if (i >= n) break`. Limite dite franchement : avec `n` uniforme, l'ancienne boucle sortait déjà au `break` ; le gain de RG-01 dépend du compilateur de l'Adreno (déroulage en 12 copies) et ne peut être mesuré que sur le téléphone. Le gain attendu vient surtout de RG-02.
- **Accroche annoncée (2 lignes, hors zone E4)** : `src/platform/desktop_main.cpp` lance le nouveau banc court `WFS_BANC=lumieres-basse` (`src/render/lumieres_basse_banc.cpp`, copie du banc `halos-sol` de WFS-421 : Ville et Corniche de nuit, Basse en suite de 3 images, Moyenne, Haute).
- Vérifié : `tests/test_eclairement_voiture.cpp` (nouveau, 12 contrôles) ; `bash tools/qa/tests_cpu.sh build` → **107 tests réussis**. Non vérifié : gain sur le S21 FE (RG-05), à mesurer par l'utilisateur.
### 2026-11-11 15:36:07 — WFS-401 attribué à Claude, agent dev (01UY1Vv1da) (po-scrum-master)
- Demande d'une nouvelle session `@dev` (session_01UY1Vv1da2SVjp6jJMr9z6e, « @dev go », aucun sujet souhaité) ; aucun ticket avant (1er et seul sujet).
- Choix : WFS-423 venait d'être donné à 01UY1Vv4BW (15:34:16, E4 occupé, WFS-422 attend donc la fin de WFS-423). Dans l'ordre des 20 puis des 50, les autres tickets sont tenus, en attente, mis de côté, réservés à un skill spécial (`@devartiste` : 378, 387, 371, 325, 395, 364 à 368, 397 ; `@cartes` : 392 ; `@cicd` : 287 ; `@conduite` : 394, 259, 260) ou dans une épic occupée (377 : E0 tenu par WFS-418 ; 357 : E8 tenu par WFS-429 ; 332 : E4 ; 341 : E13 tenu par WFS-428). Premier ticket permis à un développeur général : **WFS-401** (P2, E15 file « Eau commune », n° 50 de l'ordre des 50, ⏳ Prêt, WFS-380 fusionné) ; file Eau commune libre (WFS-383 est la file Corniche, en parallèle).
- **Début** : branche `feature/waforspeed-401` (premier commit à pousser tout de suite). Zone (RG-07) : `shaders/eau.glsl`, `src/render/eau_*`, `tests/**` ; `src/world/plage.cpp` seulement le placement de l'éclaboussure et les vues `mer-*` (≤ 10 lignes annoncées au journal). Règles : `regles-de-gestion/WFS-401.md` (RG-01 à RG-08) ; avis « Avis agent 3D : VALIDÉ » (`@3d`) dans la PR avant le lead dev ; 🟡 après fusion.
- **Point d'attention** : WFS-423 (01UY1Vv4BW) touche aussi `shaders/eau.glsl` (boucle des projecteurs en Basse) : WFS-401 ne touche pas à cette boucle ; synchroniser souvent avec `main` et garder les deux modifications en cas de conflit.

### 2026-11-11 15:37:40 — Statuts automatiques, CI main #1007 (robot statuts)
- Run rouge (push, commit 6b47611), aucun statut changé. Job(s) en échec : android, compilation.
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-406, WFS-396, WFS-362, WFS-363, WFS-364, WFS-384, WFS-412, WFS-427, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-11 15:53:20 — WFS-365 : écart de la 911 accepté (po-scrum-master)
- Information de Claude, agent dev3d (01UY1Vv3Ji) sur un écart à l'arbitrage de 15:29. Arrière : le bandeau lumineux est retiré par découpe nette et il reste deux feux séparés ; entre eux, une fine rainure noire de 1,5 cm, éteinte (creux d'origine), au lieu d'une carrosserie continue : 5 essais de couture ont laissé une bande sombre et des encoches (RG-02). Avant : les DEL de jour du bouclier sont retirées. Phares inchangés (WFS-444).
- **Décision : accepté.** Conditions : la rainure reste noire et éteinte de jour comme de nuit (freinage compris), bords nets au zoom ×4, capture de l'arrière de nuit dans la PR. Noté dans `regles-de-gestion/WFS-365.md` (Suivi).

### 2026-11-11 16:02:54 — WFS-418 fusionné par le lead dev (lead-dev)
- PR #353 fusionnée en squash (**ad003aa**) à la 1re revue, sur la tête ef25158 (`sha` attendu). Approbation « ✅ Approuvé par le lead dev » écrite dans la PR (revue 5861630952, commentaire : même compte GitHub). Ticket en **🟣 Fusionné** (US technique, pas un défaut vu sur le téléphone) ; ligne retirée de `suivi-pr.md` ; ligne ajoutée à `qa/Check.md`.
- Revue : 26 captures ouvertes (refus d'atlas, du milieu et du dernier tampon de nuit → kit de secours, même image que le kit sans paquet ; paquet sans refus identique à `main`) ; tout le diff lu (`vk.*`, `renderer.*`, `archi_carte.*`, accroches validées par le PO à 12:42). La branche était en retard sur WFS-426 (#350) et WFS-324 (#341), sans conflit : le lead dev a construit et testé l'arbre fusionné, dont le code est identique à `ad003aa` (vérifié par git diff) : 107 tests CPU verts, banc `batiments` rapide 0 écart, occlusion du kit recalculée après chaque refus (535 004 sommets, comme sans paquet).
- **Au PO** : (1) la liste RG-03 (autres réservations du chargement sans repli : décor de la Ville +373 Mo, trafic +316 Mo, voiture du joueur +63 Mo, piétons, conducteurs, enseignes, cibles et passes) est dans la description de la PR #353, pour les tickets à suivre (critère 3) ; (2) `std::bad_alloc` toujours non rattrapé dans `initCuit` (`occlusionKit`), hors des RG de WFS-418 : ticket à décider ; (3) remarques non bloquantes écrites dans la revue : blit de `uploadTexture` recopié dans `envoyerAvecBlit` (à regrouper dans un ticket E0), grille d'occlusion gardée 13,5 Mo de plus pendant le chargement (prix de RG-02), captures de l'étape 5 du banc sans le préfixe de la Corniche (WFS-426).
- CI de `main` #1006 et #1007 rouges à l'installation des outils (panne réseau du lanceur, déjà notée à 15:21) : sans lien avec cette PR ; le run de `ad003aa` est à suivre (robot des statuts).
- Au développeur (Claude, agent dev (01Us6uCt95)) : session terminée après la fusion (sa ligne de `track-agent.md`).

### 2026-11-11 15:59:37 — Règle : `@dev` = `@dev3d` ; retour d'expérience WFS-401 / WFS-422 (po-scrum-master (session 01UY1Vv4oe))
- **Règle de l'utilisateur** : dorénavant `@dev` est la même chose que `@dev3d` (agent `dev-artiste-3d`). Mis à jour : `appel-agents.md` (tableau des alias), `CLAUDE.md`, skill `po-scrum-master` (les tickets `@devartiste` ne sont plus sautés pour une session `@dev`).
- **Retour d'expérience** : l'utilisateur a lancé deux `@dev` pour WFS-422 et WFS-423 en parallèle. L'exception « 422 et 423 en parallèle dans E4 » avait été proposée dans la conversation de cette session mais **pas écrite sur `main`** ; le PO de la 2e session a donc vu E4 occupé par WFS-423 et a donné WFS-401 (n° 50 de l'ordre des 50, P2). WFS-422 (P1) n'a pas démarré. Leçon : une décision prise avec l'utilisateur s'écrit sur `main` **avant** qu'il ouvre les sessions. Suite de WFS-401 / WFS-422 : en attente de la décision de l'utilisateur.

### 2026-11-11 16:01:28 — WFS-401 remis en ⏳ Prêt, WFS-422 permis en parallèle de WFS-423 (po-scrum-master (session 01UY1Vv4oe))
- L'utilisateur a **arrêté la session de WFS-401** (Claude, agent dev (01UY1Vv1da)) : ticket remis en ⏳ Prêt, responsable vidé, ligne retirée de `track-agent.md`. Rien de WFS-401 n'est sur `main` ; la branche `feature/waforspeed-401` (2 commits, travail en cours) est gardée (seul l'utilisateur supprime les branches) et ne sera **pas reprise** : un futur développeur repart de `main`.
- **Exception E4 écrite** (décision de l'utilisateur) : WFS-422 se fait en parallèle de WFS-423 (zones séparées, `regles-de-gestion/WFS-422.md`). Le PO donne **WFS-422 à la prochaine session `@dev`**.


### 2026-11-11 16:08:56 — WFS-418 : fin, session terminée (Claude, agent dev (01Us6uCt95))
- PR #353 fusionnée en squash par le lead dev (ad003aa, revue 5861630952) ; ticket 🟣 Fusionné (US technique : ✅ par le robot des statuts sur un run vert de `main`).
- Réponse au lead dev écrite sous sa revue (commentaire 6800854346) : ses 4 remarques non bloquantes (copie du blit d'`uploadTexture` à regrouper par un ticket E0, +13,5 Mo de mémoire vive au pic du chargement, étape 5 du banc sans préfixe de carte depuis WFS-426, `std::bad_alloc` de `initCuit` non rattrapé) et la **liste RG-03** de la PR (décor de la Ville 373 Mo, trafic 316 Mo, voiture du joueur 63 Mo, piétons 33 Mo, conducteurs 15 Mo, petits systèmes, textures de la route, cibles et passes) sont **remises au PO** pour les tickets à suivre (critère 3).
- Titre de la discussion : outil `set_session_title` absent de cette session ; titre voulu, non appliqué : « ⏹️ Terminée — Claude, agent dev (01Us6uCt95) (WFS-418 fusionné) ».
- Un seul sujet par session : je ne demande pas d'autre ticket ; la session s'arrête.

### 2026-11-11 16:09:16 — WFS-422 attribué à Claude, agent dev (01UY1Vv5hT) (po-scrum-master)
- Nouvelle session `@dev` (session_01UY1Vv5hT44YCvm3KwBfCa1, « @dev go », aucun ticket avant : 1er et seul sujet).
- Choix : n° 4 de l'**ordre des 20** (12:08 UTC) ; WFS-412, 421, 423 déjà pris ou fusionnés ; **exception E4 de 16:01** (décision de l'utilisateur) : WFS-422 en parallèle de WFS-423 (01UY1Vv4BW), fichiers séparés. WFS-377 (E0) reste ⏳ Prêt pour la prochaine session `@dev`.
- Zone : `src/render/passes.*`, `src/render/passe_essai.cpp`, secours en Basse des shaders qui lisent `prePasseNormales` / `prePasseProfondeur`, `tests/**`. Fichier commun avec WFS-423 : une seule accroche ≤ 10 lignes annoncée au journal, le second à fusionner garde les deux modifications.
- Règles : `regles-de-gestion/WFS-422.md` (RG-01 à RG-04 ; 3 captures demandées ; gain mesuré sur le S21 FE dans la PR ; « Avis agent 3D : VALIDÉ » si l'image change ; 🟡 après fusion).
- Plan court : interrupteur « sans pré-passe » de WFS-419 transformé en chemin de qualité Basse ; liste des effets lecteurs et de leur secours dans la PR ; captures avant / après (Ville et Corniche de nuit en Basse, Ville en Moyenne identique) ; PR, lead dev.
- `track-agent.md` : ligne de la session terminée WFS-418 (01Us6uCt95) enlevée.

### 2026-11-11 16:14:01 — Alerte « CI rouge » : WFS-445 créé (po-scrum-master)
- Alerte de l'utilisateur (`@po alert ci rouge`). Vérifié sur GitHub : `build.yml` de `main` rouge 3 fois de suite : **#1005** (2ccc37c), **#1006** (7aa1535), **#1007** (6b47611) ; **#1008** (ad003aa, WFS-418) bloqué au même endroit à 16:13.
- Cause : téléchargements du lanceur GitHub, pas le code : job `compilation`, `apt-get install` de glslc / mesa (`Temporary failure resolving 'azure.archive.ubuntu.com'`, après les 2 essais de 5 min) ; job `android` (#1007), NDK 27.2 `Error on ZipFile unknown archive`. Bancs et endurance sautés depuis #1005.
- Le rouge a persisté au run suivant (note de 15:21) : **WFS-445** créé (P1 Urgent, [TECHNIQUE], E7, `regles-de-gestion/WFS-445.md`) : diagnostic et relance d'abord (RG-01), sinon installation robuste aux pannes passagères (apt, NDK) sans rien affaiblir. ⏳ Prêt, pour une **nouvelle session `@cicd`** (l'agent expert ci de WFS-427 a fini son sujet). Aucun ticket fusionné n'est mis en cause.

### 2026-11-11 16:16:15 — WFS-423 : PR #354 ouverte, lead dev appelé (Claude, agent dev (01UY1Vv4BW))
- PR #354 (`feature/waforspeed-423`, synchronisée avec `main`, commit 7744be24) : éclairement autour des voitures calculé sur le CPU en Basse (RG-02), boucles des projecteurs bornées (RG-01). Banc `lumieres-basse` : **0 pixel différent** avant / après sur 10 captures (Ville, Corniche ; Basse en suite de 3, Moyenne, Haute) ; essai ×20 local : la voie du CPU est bien prise en Basse seulement. 108 tests CPU réussis. Gain sur le S21 FE (RG-05) à mesurer par l'utilisateur.

### 2026-11-11 16:17:14 — WFS-418 : remise au PO (RG-03), WFS-446 à 449 créés (po-scrum-master)
- Remise de Claude, agent dev (01Us6uCt95) reçue (critère 3 de WFS-418) : PR #353 fusionnée en squash par le lead dev (ad003aa), ticket 🟣 ; liste RG-03 lue dans la description de la PR ; session terminée (un seul sujet), aucun ticket attribué. Ligne datée dans « Suivi » de `regles-de-gestion/WFS-418.md`.
- **Créés** (règles dans `regles-de-gestion/WFS-446.md` à `WFS-449.md`) : **WFS-446** (P2, ⏳ Prêt, E0 + rendu, en parallèle de WFS-377 : fichiers séparés) secours existants branchés sur le refus de mémoire graphique (trafic +316 Mo, piétons, conducteurs, enseignes, textures de la route) + `std::bad_alloc` d'`initCuit` (remarque 4 du lead dev) + étape 5 du banc `batiments` avec préfixe de carte et sur la Corniche (remarque 3) ; **WFS-447** (P2, après 446 et 383 : mêmes fichiers `decor_carte.*`) décor des cartes +373 Mo, portique, commerces ; **WFS-448** (P3, après 446) voiture du joueur +63 Mo, adversaires, fantôme en textures réduites (1/2 puis 1/4), sinon retour au salon avec un message ; **WFS-449** (P3, après la file E4 422 → 423 → 424) cibles de rendu et passes en qualité juste en dessous.
- **Décision du PO** (arbitrage demandé par la liste RG-03) : décor refusé = **lot non dessiné**, pas de niveau de détail fabriqué en repli (plus simple, rare, mieux qu'un arrêt) ; l'utilisateur peut la changer.
- **Notés sans ticket** : remarque 1 (copie de la suite de commandes d'`uploadTexture` dans `envoyerAvecBlit`) : la regrouper = réécrire du code fusionné, interdit sans ordre de l'utilisateur ; les 4 tickets interdisent toute nouvelle copie. Remarque 2 (+13,5 Mo de mémoire vive au pic du chargement) : pour information (aucune limite de mémoire, décision de l'utilisateur). Police de l'interface (5 Mo au démarrage), pool de descripteurs : hors périmètre.
- Ordre : ces tickets se donnent **après** ceux de l'ordre des 20 permis à la session qui demande (bandeau de `TACHES.md`). `track-agent.md` remis à jour (ligne WFS-415 enlevée : fusionné ; derniers envois des branches ; ⚠️ WFS-383 sans envoi depuis 13:42).
- Numéros : prévus 445 à 448, décalés à **446 à 449** parce que WFS-445 a été pris au même moment par l'autre session du PO (correction de la CI rouge, b67b3ff0).

### 2026-11-11 16:19:11 — Statuts automatiques, CI main #1008 (robot statuts)
- Run rouge (push, commit ad003aa), aucun statut changé. Job(s) en échec : compilation.
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-406, WFS-396, WFS-362, WFS-363, WFS-364, WFS-384, WFS-412, WFS-427, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-11 16:30:16 — WFS-422 : risque signalé, la pré-passe ne tourne jamais en jeu (Claude, agent dev (01UY1Vv5hT))
- Constat : la pré-passe (`src/render/passes.cpp`, `PrePasse`) ne tourne que si une passe active la demande (`avantScene`). Seule la passe d'essai (`src/render/passe_essai.cpp:27`) la demande, et elle n'est active que par `passes::activerEssai(true)`, appelé seulement par le banc `bench_passes.cpp`. Aucun shader du jeu ne lit `prePasseNormales` / `prePasseProfondeur`. Banc PC `halos-sol` (Ville et Corniche de nuit, Basse / Moyenne / Haute) : 0 création de la pré-passe sur 6 rendus (`qa/pr/WFS-422/verification.md` sur `feature/waforspeed-422`).
- Conséquence : en jeu, la pré-passe est déjà absente à tous les niveaux ; l'interrupteur « sans pré-passe » de WFS-419 ne change rien. Le « gain de 5 à 18 ms » (44 / 57 ms contre 62 ms) est l'écart d'une mesure à l'autre, pas un effet de la pré-passe. WFS-422 n'a rien à couper ni de secours à écrire (RG-01 à RG-03 déjà vraies sur `main`), et RG-04 ne peut pas montrer de gain.
- Proposition : clore WFS-422 « sans objet » (aucun code), preuve sur la branche ; reporter l'effort sur WFS-423 / 424. Autre option : PR qui borne la pré-passe à Moyenne et plus (`qualiteMin`), pour qu'un futur lecteur ne l'allume pas en Basse ; je ne la recommande pas (code « pour plus tard », aucun gain).
- Décision attendue de : PO.

### 2026-11-11 16:31:15 — WFS-422 annulé (sans objet) (po-scrum-master)
- Arbitrage demandé par Claude, agent dev (01UY1Vv5hT) : en jeu, la pré-passe ne tourne **jamais** (seule la passe d'essai du banc `bench_passes.cpp` la demande ; aucun shader du jeu ne lit ses images ; banc `halos-sol` : 0 création en Basse / Moyenne / Haute). Vérifié par le PO dans le code (`demandePrePasse`, `activerEssai`).
- **Décision : WFS-422 « ❌ Annulé (sans objet) »**, sans code ni PR (pas de code « pour plus tard »). La preuve reste sur la branche `feature/waforspeed-422` (gardée). Le gain de 5 à 18 ms annoncé était l'écart entre deux mesures.
- Dépendances mises à jour : WFS-424 (après WFS-421, 423) et WFS-449 (après WFS-423, 424). Ordre des 20 : la place n° 4 est sans objet, la suite ne change pas.
- Session 01UY1Vv5hT terminée (un seul sujet) : ligne enlevée de `track-agent.md`. Information ajoutée pour l'utilisateur dans `tache utilisateur.md`.

### 2026-11-11 16:32:40 — WFS-423 fusionné par le lead dev (lead-dev)
- PR #354 relue sur 7744be24 : 10 paires de captures ouvertes (banc `lumieres-basse`, Ville et Corniche de nuit ; Basse en suite de 3 images, Moyenne, Haute) : image identique à `main`, caisse éclairée, aucun saut. Diff lu en entier (`eclairement_voiture.h` = même formule que `eclairementRouteAutour`, `PC` à 128 octets avec `static_assert`, `nuit.w = 0` hors Basse, boucles bornées à `min(n, 12)`, accroche de 2 lignes annoncée).
- Vérifié par le lead dev : construction de la branche, `tests_cpu.sh` → 108 tests réussis ; banc `lumieres-basse` relancé : 0 pixel différent avec les captures « avant ».
- « ✅ Approuvé » en commentaire (même compte GitHub), fusion squash 529e24e. Statut **🟡 À valider (utilisateur)** : gain à mesurer sur le S21 FE (RG-05).

### 2026-11-11 16:33:06 — WFS-423 : fusionné (#354, 529e24e), session terminée (Claude, agent dev (01UY1Vv4BW))
- PR #354 fusionnée en squash par le lead dev ; ticket 🟡 À valider (utilisateur). Question de mesure posée à l'utilisateur dans `tache utilisateur.md` (RG-05). Un seul sujet par session : la session s'arrête, aucun autre ticket demandé.

### 2026-11-11 16:48:44 — Statuts automatiques, CI main #1009 (robot statuts)
- Run rouge (push, commit 529e24e), aucun statut changé. Job(s) en échec : compilation.
- Tickets 🟣 contenus dans ce run : WFS-418, WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-406, WFS-396, WFS-362, WFS-363, WFS-364, WFS-384, WFS-412, WFS-427, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-11 16:53:06 — WFS-445 attribué à agent expert ci cd (session 01UY1Vv4Ch) (po-scrum-master)
- Ordre de l'utilisateur dans la session `@cicd` 01UY1Vv4Ch : « Prend le 445 ». Nouvelle session, aucun ticket avant : attribution permise (un seul sujet par session).
- **WFS-445** ([TECHNIQUE] correction pipeline CI, P1, E7) : 🔧 En cours, branche `feature/waforspeed-445`. E7 n'avait aucun autre ticket actif. Règles : `regles-de-gestion/WFS-445.md`. US technique : CI de branche permise, une à la fois, chaque run noté au journal.
- Constat de l'agent sur GitHub : **#1008 (ad003aa)** et **#1009 (529e24e, WFS-423)** sont aussi rouges, même cause (job compilation, apt `Temporary failure resolving 'azure.archive.ubuntu.com'`). La CI de `main` est donc rouge de #1005 à #1009.
- Rappel à l'agent : renommer lui-même sa session en `WFS-445 — agent expert ci cd (correction pipeline CI)`, skill `expert-ci-cd`, PR + captures si utiles, appel du `lead-dev` (seul à fusionner), arrêt de la session après la fusion.

### 2026-11-11 16:54:55 — WFS-428 : reproduit, causes et commits fautifs, correction poussée (Claude, agent dev (01UY1Vv5Nm))
- **RG-01 (reproduit sur `main` 6f1cd6e, lavapipe, `banc_groupe.sh realisme-2`, 32 min 42)** : « réalisme : partie 2, 3 erreur(s) » ; lignes [ERREUR] : `M-01 (nuit) : 5 / 6 faces conformes` (face **+Z** 0,0345 contre 0,0368, **−6,37 %**) ; `capture abandonnée (face sans exposition connue)` ×2 (Campagne, juste après le banc des passes) ; et les 2 lignes de WFS-377 (choix « Ultra » introuvable, parcours 2 / 5), hors ticket.
- **(b) cause, commit fautif 52d42271 (WFS-322, #289, fusionné 7 min après le run #5 vert)** : toute capture d'environnement exclut la voiture du joueur. La caméra du banc M-01 est posée 6 m devant la Golf ; sa face +Z regarde la voiture : la scène la montre (phares allumés la nuit), la capture non (on voit l'ombre de la Golf sans la Golf sur la planche `realisme-cubemap-essai`). Correction à la source (`environnement.*`) : l'exclusion ne vaut que pour les captures demandées par les sondes d'éclairage (`demander(p, 0, true)`, seul appel du jeu) ; une capture depuis un autre point voit toute la scène. Image du jeu inchangée.
- **(c) cause, commits 00bd5807 (WFS-151) et 941e4756 (WFS-213, qui rend l'abandon visible)** : la passe des sondes (`SondesIbl`) ne compte les captures finies que lorsqu'elle tourne. Les captures 5 à 11 du banc des passes sont faites éclairage désactivé : compteur en retard ; à la 1re image en Campagne, la capture 12, **encore en cours**, est prise pour une capture finie : relue trop tôt, faces sans k, abandonnée, puis la 12 aussi (une face non notée) : 2 abandons. **Pas une régression récente** : reproduit en local sur 9c4f58a (run #12) et 4e7fa716 (run #5, vert) ; ces lignes ne comptent pas dans le nombre d'erreurs du banc (WFS_ERR seul), d'où « entrée entre 9c4f58a et ed6f745 » dans le ticket. Le journal complet du run #12 n'a pas pu être relu ici (outil tronqué à 5 000 lignes, archive bloquée par le réseau). Correction (`eclairage_physique.cpp`, 1 ligne) : si une capture est en cours alors que le compteur est en retard, il est remis à jour et la capture en cours suivie face par face. Garde `kFacesValides` inchangée.
- **Branche** (6bbbbce4, synchronisée avec `main`) : `realisme-2` → **2 erreurs, seulement WFS-377** ; M-01 nuit **6 / 6** (+Z −0,06 %), jour +Z −0,47 % (au lieu de −0,72 %) ; plus aucune « capture abandonnée » (sonde 3 prise sur la capture 12). Rapports `realisme-fidelite-2` / `mesures-2` identiques à `main` hors M-01 et durées ; étalonnage identique (sonde 14 244 lx). Images du jeu du banc : écart ≤ 1 niveau sur quelques pixels.
- Tests CPU : 106 / 107 sous charge ; `test_audio_chocs` RG-07 (chronométrage 0,0504 ms contre ≤ 0,05) rouge seulement pendant un essai en parallèle, vert relancé seul (0,0424 ms) : sans lien.
- Fichiers : `src/render/environnement.h`, `environnement.cpp`, `eclairage_physique.cpp` (zone du ticket). Aucun contrôle, tolérance ni valeur attendue changés ; `bench_passes.cpp` non touché. Suite : captures avant / après, PR, lead dev.

### 2026-11-11 16:55:35 — WFS-445 : relancement de main (#1010), correction prête sur la branche (agent expert ci cd (session 01UY1Vv4Ch))
- **RG-01** : `build.yml` relancé une fois à la main sur `main` (run **#1010**, 43062782782, commit 2e8690a1).
- Cause précise (journal du job compilation de #1009) : `packages.microsoft.com` répond, mais tous les serveurs Ubuntu passent par la liste de miroirs de l'image (`mirror+file:/etc/apt/apt-mirrors.txt`), dont le 1er, `azure.archive.ubuntu.com`, ne se résout plus ; 20 s perdues par fichier, `apt-get update` ne sort qu'en avertissement, puis l'installation échoue (2 essais de 5 min). Panne du lanceur GitHub, pas le code.
- Correction préparée (RG-02, RG-03) sur `feature/waforspeed-445` (197581a8), utilisée seulement si #1010 reste rouge : `tools/qa/installer_paquets.sh` (serveur officiel `archive.ubuntu.com` écrit directement si le miroir Azure ne se résout pas, 3 essais à attente croissante, serveur suivant à chaque essai, `update --error-on=any`, 15 min au plus, code 1 à la fin) et `tools/qa/installer_sdk_android.sh` (NDK : 3 essais, fichiers partiels effacés) ; branchés dans `build.yml` (4 jobs), `matin.yml` (6), `paquets.yml` (1). Aucun job, test, banc ni seuil changé.

### 2026-11-11 17:08:14 — WFS-365 : PR #355 ouverte, accroche de 8 lignes annoncée (Claude, agent dev3d (01UY1Vv3Ji))
- Fait : **911** : partie centrale du bandeau lumineux arrière retirée (découpe exacte ; rainure noire de 1,5 cm, éteinte de jour, de nuit et au freinage : écart accepté par le PO à 15:53) ; DEL de jour du bouclier retirées. Phares non touchés (WFS-444). **Gallardo** : écusson du nez retiré, trou recousu (peau cousue, 0 point libre) ; chevrons et « Y » des feux arrière effacés (nouvelle retouche `uvPlan` de `tools/retoucher_formes.mjs` : uv en projection plane sur le verre rouge strié de la même texture, sommets neufs) ; nid d'abeille des prises d'air remplacé par une grille à 4 lames (grille ajustée, aucun sommet devant la carrosserie).
- **Accroche hors zone (8 lignes)** : `src/render/correctifs_voitures.h`, nombre de matériaux des optiques avant de la Gallardo 18 → 22 (2 grilles × 2 matériaux ajoutés à la fin) ; rangs et zones inchangés. Sans elle, `test_phares_optiques` échoue.
- Vérification : `--verifier` vert (11 voitures) ; 911 et Gallardo rejouées à l'octet ; Golf et Corsa rejouées depuis leurs modèles d'avant WFS-364 avec l'outil modifié : identiques à `main` ; 108 tests CPU réussis ; fidélité inchangée ; 911 122 070 triangles (124 140 avant), Gallardo 105 725 ; 16 vitrines + 16 zooms ×4 jour / nuit, refaits sur `main` à jour (WFS-423 compris).
- Essais écartés (pour WFS-444) : déformation lisse de l'espace autour des phares de la 911 (pli de l'aile et facettes du capot dès 5 cm) ; peau cousue dans la rainure arrière (lèvre arrondie, peinture double face : encoches au zoom ×4).
- **Pour le PO (RG-06, tâche séparée proposée)** : feux arrière de la Gallardo, un faible relief en chevrons reste modelé dans le verre (visible de jour au zoom ×4, pas de nuit) : à aplanir par remodelage du verre.

### 2026-11-11 17:13:13 — WFS-429 : boutons Menu et Caméra à côté du score, +5 %, PR ouverte (Claude, agent dev (01UY1Vv3Vj))
- Fait : rangée [MENU] [CAMÉRA] [SCORE] en haut à droite, centrée sur la hauteur du panneau de score, écarts de 8 px, dans la zone sûre (RG-01) ; toutes les mesures des boutons × `hud::BOUTON_ECHELLE` = 1,05 (RG-02) ; zones de toucher = rectangles dessinés (RG-03) ; Cockpit : vitesse sous le temps (RG-05) ; Poursuite : même placement (RG-06) ; gaucher : score à droite, donc rangée inchangée (RG-07).
- Fichiers : `src/ui/hud.h`, `src/ui/hud.cpp` (`hud::rangeeBoutons`), `src/ui/ecrans/pause.cpp`, `src/game/camera_vue.cpp` (`drawUi` du bouton seulement), `tests/test_hud_boutons.cpp` + 1 ligne `CMakeLists.txt`.
- **Accroche annoncée** (1 ligne, `src/platform/desktop_main.cpp`) : `WFS_BANC=pause` lance seul le banc du bouton Menu et de la pause (`ecrans::benchPause`).
- Vérification : `test_hud_boutons` OK (2400×1080, 1600×720, 2340×1080, encoche 90 px, gaucher, Poursuite) ; 109 tests CPU OK après synchronisation avec `main` ; captures `tools/qa/captures_pr.sh WFS-429 hud:1600x720 pause:1600x720 course5:1600x720` regardées (combo, notifications, GPS, Nitro sans chevauchement ; Cockpit jour / nuit ; Poursuite ; toucher Menu → pause → Reprendre / Recommencer / Quitter, RG-09 au banc).
- Limites : pas de capture « avant » du banc `pause` (absent de `main`) ; ce banc lancé seul écrit « 2 pipeline(s) créé(s) pendant la course » (pas de course chauffée avant) ; pas d'essai sur téléphone.

### 2026-11-11 17:17:53 — WFS-365 fusionné par le lead dev (lead-dev)
- PR #355, revue du commit 3e02fe38, fusion en squash 6576b42 (titre `WFS-365 : … (#355) (Claude, agent dev3d (01UY1Vv3Ji))`). « ✅ Approuvé par le lead dev » écrit dans la PR en commentaire (GitHub refuse l'approbation : même compte).
- Vérifié : 32 paires de captures ouvertes une par une ; les 3 conditions du PO (15:53) sont tenues : rainure noire éteinte de jour, de nuit et au freinage, bords nets au zoom ×4, capture de l'arrière de nuit. Diff lu en entier (`uvPlan`, `formes.json`, accroche de 8 lignes 18 → 22 matériaux) ; construction et `tests_cpu.sh` : 108/108 ; `--verifier` vert ; rejeu depuis les modèles de `main` identique à l'octet (`.wfm` et `.lod` des deux voitures).
- **Pour le PO** (remarques non bloquantes, RG-06) : (1) feux arrière de la Gallardo, relief en chevrons modelé dans le verre : tâche séparée à créer ; (2) 911, bout gris de la lame haute visible au bord extérieur de la prise d'air (zoom de face) : avec WFS-444 ou dans une tâche à part ; (3) Gallardo, liseré jaune irrégulier et petit éclat au bord des prises d'air ; (4) hors ticket, déjà sur `main` : halo rond des stops au freinage (« pas de halo en disque »), anomalie existante pour le QA.
- Statut : 🟣 Fusionné (#355) ; ligne retirée de `suivi-pr.md` ; ligne `qa/Check.md` ajoutée.

### 2026-11-11 17:18:41 — WFS-365 : fin, session terminée (Claude, agent dev3d (01UY1Vv3Ji))
- PR #355 fusionnée en squash par le lead dev (6576b42). Statut : 🟣 Fusionné (pas un défaut vu sur le téléphone : pas de 🟡).
- Réponse écrite sous la revue dans la PR (4 remarques non bloquantes, notées pour le PO) : relief en chevrons du verre des feux de la Gallardo (tâche séparée proposée) ; petit embout gris au bord extérieur de la prise d'air de la 911 (avec WFS-444 ou à part) ; liseré et éclat au bord des prises d'air de la Gallardo ; halo rond des feux stop au freinage, déjà sur `main` (au QA).
- Session terminée : un seul ticket par session, je ne demande pas d'autre ticket.

### 2026-11-11 17:40:16 — WFS-445 : CI de `main` de nouveau verte (#1010), ticket fermé sans code (agent expert ci cd (session 01UY1Vv4Ch))
- Run **#1010** (43062782782, relancé à la main, commit 2e8690a1) **vert** : 8 jobs sur 8 (compilation, android, publication-apk, endurance, banc-essai texte et menu1-2400, publication-captures, résumé). Installation apt en 52 s, NDK en 21 s : la panne du miroir `azure.archive.ubuntu.com` du lanceur GitHub est finie. Bancs et endurance de nouveau joués (sautés de #1005 à #1009).
- **RG-01 appliquée** : vert sans changement → WFS-445 **✅ Terminé**, aucun code fusionné, pas de PR. Le code de la branche n'a rien à voir avec le rouge : les tickets 🟣 de #1005 à #1009 sont couverts par #1010.
- **Pour le PO** : la correction de secours reste sur `feature/waforspeed-445` (197581a8, branche gardée, non relue) : apt sur le serveur officiel si le miroir Azure ne répond pas, 3 essais, NDK refait s'il arrive abîmé. Si la panne revient, elle peut devenir une US technique (relecture par le lead dev, CI de branche). Le bandeau rouge de `TACHES.md` et `track-agent.md` peut être enlevé.
- Session terminée (un seul sujet).

### 2026-11-11 17:43:54 — WFS-429 approuvé par le lead dev, fusion bloquée par un conflit de journal (lead-dev)
- PR #356 relue sur b17d9e69 : 84 paires de captures qui diffèrent et 5 `pause-*` ouvertes, diff entier lu, construction et `tests_cpu.sh` refaits (109/109), bancs `hud`, `pause` et `camera` relancés sur la tête (rétroviseurs présents, Menu → pause, 4 appuis Caméra → 4 vues). Revue « ✅ Approuvé » en COMMENT (même compte GitHub).
- Fusion refusée par GitHub : conflit dans `journal.md` seulement (main a avancé : WFS-365 #355, WFS-445). Le développeur synchronise (aucun code changé), remet la PR en 🔁 et rappelle le lead dev, qui vérifiera que la nouvelle tête = b17d9e69 + main, puis fusionnera.

### 2026-11-11 17:52:26 — WFS-429 fusionné par le lead dev (lead-dev)
- PR #356 : tête 8f7a76a6 vérifiée (merge de b17d9e69 et de main, diff du ticket identique, journal sans marqueur de conflit) ; « ✅ Approuvé » en COMMENT ; fusion en squash **ee266c6** (`WFS-429 : boutons Caméra et Menu à côté du score, +5 % (#356) (Claude, agent dev (01UY1Vv3Vj))`).
- `suivi-pr.md` : ligne #356 supprimée ; `TACHES.md` : 🟣 Fusionné ; `qa/Check.md` : ligne ajoutée (points à regarder aux bancs du matin). Pas de 🟡 (précision de l'utilisateur dans WFS-429.md) : ✅ par le robot des statuts après les bancs du matin verts.

### 2026-11-11 17:52:57 — WFS-429 : fin, session terminée (Claude, agent dev (01UY1Vv3Vj))
- PR #356 fusionnée en squash par le lead dev (ee266c6c) après synchronisation (8f7a76a6, conflit de journal seulement). WFS-429 est en 🟣 : il passera ✅ après des bancs du matin verts (pas de 🟡). La ligne est dans `qa/Check.md` (ajoutée par le lead dev). J'ai répondu sous la demande dans `tache utilisateur.md`.
- Session terminée (un seul sujet par session) : aucun autre ticket demandé.

### 2026-11-11 17:53:36 — WFS-428 : PR #357 ouverte, lead dev appelé (Claude, agent dev (01UY1Vv5Nm))
- PR #357 (branche synchronisée avec `main`, 8ebad8a2) : `realisme-2` passe de 3 à 2 erreurs (seulement WFS-377) ; M-01 nuit 6 / 6 (+Z −0,06 %) ; plus aucune « capture abandonnée ». Après la dernière synchronisation (WFS-423 compris), M-01 (passes seules) 6 / 6 jour et nuit.
- Captures `tools/qa/captures_pr.sh WFS-428 phares realisme` (passes seules) : 22 paires ; vues du jeu en Haute identiques au pixel ; Basse `poursuite-basse*` : même écart entre deux lancements de la même branche (non-déterminisme du banc en Basse) ; planche du cube : la Golf apparaît dans la face +Z.
- Correction de mon entrée précédente : la reproduction sur `main` s'est faite sur 9a6789c0 (et non 6f1cd6e).
- Aucune CI de branche lancée : le groupe `realisme-2` a été joué en local avec le même rendu (lavapipe) que la CI.

### 2026-11-11 17:55:17 — Début WFS-377 : attribué à Claude, agent dev3d (01UY1Vv51w) (po-scrum-master)
- Demande d'une nouvelle session `@dev` (dev-artiste-3d, session session_01UY1Vv51wmeuvcJnDy1kkSB), aucun ticket reçu avant : premier et seul sujet de cette session.
- Choix : WFS-377 (P1, US technique, ⏳ Prêt) : les bancs du matin sont rouges deux jours de suite et bloquent tout passage 🟣 → ✅ ; le bandeau du 12:57 le donne aux prochaines sessions `@dev`, en parallèle de la file E4. E0 libre (WFS-418 🟣). Ordre des 20 : les tickets E4 restants (WFS-420 P2, WFS-424) passent après ce P1.
- Épic E0, zone : `src/render/bench_passes.cpp` seulement (bloc « RG-04 / P-05 », l. ≈ 261-320) ; accroche ≤ 10 lignes annoncée au journal si indispensable. Ne pas toucher au bloc M-01 du même fichier (WFS-428, session 01UY1Vv5Nm ; sa branche touche `environnement.*` et `eclairage_physique.cpp`, pas `bench_passes.cpp`).
- Critères : banc `realisme` (`WFS_PASSES_SEUL=1`) 5 / 5 étapes, aucune ligne « introuvable », `parcours-ultra-course` en Ultra (256 px). US technique : CI de branche permise, une à la fois, notée au journal. Règles : `regles-de-gestion/WFS-377.md`.
- Au passage : WFS-420 (après WFS-419, fusionné) et WFS-424 (après WFS-421 et 423, fusionnés ; 422 annulé) passent ⏳ Prêt, pour les prochaines sessions `@dev` (420 puis 424, file E4). `track-agent.md` : lignes des sessions closes enlevées (WFS-365, 429, 445, 423).

### 2026-11-11 18:33:01 — Début WFS-420 : attribué à Claude, agent dev3d (01UY1Vv3k4) (po-scrum-master)
- Demande d'une nouvelle session `@dev3d` (dev-artiste-3d, session session_01UY1Vv3k4ksChrFLKosm8CS), aucun ticket reçu avant : premier et seul sujet de cette session.
- Choix : WFS-420 (P2, US technique, ⏳ Prêt), 6e de l'ordre des 20 de l'utilisateur (12:08 UTC), le prochain `@dev` de la file E4 ; WFS-424 (P1) vient après lui dans cette file. Aucun autre ticket E4 en cours (WFS-383 est en E15).
- Zone : `src/render/**` (mesures), `src/game/**` (rejeu, accroche ≤ 30 lignes annoncée au journal), `src/ui/ecrans/reglages*`, `src/platform/android_main.cpp` (fichier et partage, ≤ 30 lignes), `tests/**`. Pas de changement de qualité, de décor, de voiture ni de CI.
- Critères : `regles-de-gestion/WFS-420.md` (RG-01 à RG-06 : banc lancé du menu caché de WFS-419, Ville de nuit puis de jour, 30 s de chauffe + 60 s de mesure, un interrupteur à la fois, tableau à l'écran + `diagnostic-AAAA-MM-JJ-hhmm.txt` partageable, aucun coût hors banc). Après fusion : 🟡 À valider (utilisateur). US technique : CI de branche permise, une à la fois, notée au journal.

### 2026-11-11 18:32:57 — WFS-424 permis en parallèle de WFS-420 (po-scrum-master (session 01UY1Vv4oe))
- Décision de l'utilisateur : deux sessions `@dev` ouvertes pour WFS-420 et WFS-424. Exception E4 écrite dans `TACHES.md` et `regles-de-gestion/WFS-424.md` (fichiers séparés). Le PO donne **WFS-420 à la 1re session `@dev`, WFS-424 à la 2e** ; ne pas sauter WFS-424 pour un autre ticket.


### 2026-11-11 18:34:55 — WFS-424 : pris sur ordre direct de l'utilisateur (Claude, agent dev3d (01UY1Vv3cC))
- L'utilisateur a ouvert cette session `@dev` en demandant explicitement WFS-424 ; c'est la 2e session `@dev` prévue par l'exception PO de 18:32 (en parallèle de WFS-420).
- Branche `feature/waforspeed-424`. Plan : cuire le `fbm` de `shaders/route.glsl` dans une texture répétable (outil de cuisson sur PC, même bruit), lue en Basse seulement.

### 2026-11-11 18:39:28 — Début WFS-392 : attribué à Claude, agent cartes (01UY1Vv2ghpU) (po-scrum-master)
- Demande d'une nouvelle session `@cartes` (expert-cartes, session session_01UY1Vv2ghpUrn9AR3Uxd8bo), aucun ticket reçu avant : premier et seul sujet de cette session. Souhait de l'utilisateur : « prend un sujet », sans ticket précis.
- Choix : WFS-392 (P2, ⏳ Prêt), le premier ticket de l'ordre de l'utilisateur qui demande le skill `expert-cartes` : l'ordre des 20 n'a plus de ticket `@cartes` (WFS-426 est 🟡), WFS-405 attend le jour de publication, puis l'ordre des 50 donne WFS-392 (31e). WFS-357 (30e, P3) n'est pas donné à cette session : c'est une ligne de la boucle Android (`android_main.cpp`, fichier touché par WFS-420 en cours), sans skill cartes ; il reste ⏳ Prêt pour une session `@dev`. E14 libre (aucun 🔧 ni 🔀).
- Épic E14 + E9 (exception PO), zone : `src/world/cartes_v2/**`, `paquets/sources/**`, `tests/cartes_v2*`, `shaders/route.glsl`, `src/render/route_*` ; `src/world/meteo.*` en accroche ≤ 10 lignes si besoin.
- Point d'attention : `shaders/route.glsl` est partagé avec WFS-424 (en cours, session 01UY1Vv3cC) : changement minimal, valable en Basse aussi, synchronisations fréquentes (écrit dans la section « Suivi » de `regles-de-gestion/WFS-392.md`).
- Critères : `regles-de-gestion/WFS-392.md` (RG-01 à RG-05 ; 4 captures avant / après dans le 2e tunnel de la carte de neige, jour, Cockpit, nuit, entrée du 1er tunnel ; coût en Basse ≤ +5 %). Après fusion : 🟣 puis ✅ par les bancs du matin. Hors E15 : avis de l'agent 3D non exigé.
- `track-agent.md` : derniers envois remis à jour (WFS-377 18:17, WFS-383 16:32, WFS-420 18:34).


### 2026-11-11 18:40:06 — WFS-392 : début, branche créée (Claude, agent cartes (01UY1Vv2ghpU))
- Ticket reçu du PO (commit bd1a08db), discussion renommée, branche `feature/waforspeed-392` créée depuis `main`.
- Skills chargés : `agent-developer`, `waforspeed-natif`, `expert-cartes` ; règles `regles-de-gestion/WFS-392.md` lues en entier.
- Suite : analyse de `carte_donnees_decor.cpp`, de l'abri tunnel / toit (WFS-334, WFS-354), de `shaders/route.glsl` et de `neige_banc.cpp` ; plan écrit ici avant de coder.

### 2026-11-11 19:03:55 — WFS-392 : plan, premier envoi du code et risques signalés (Claude, agent cartes (01UY1Vv2ghpU))
- Cause de RG-01 : `route::preparer` ne donnait au shader que le 1er tunnel (`g_abri`, 3 nombres dans la couleur du matériau). Fait : chaque tunnel est rangé dans une case (début × 1024 + longueur, en segments), la longueur du tour passe dans le code du matériau (`codeMateriau`, + 256 × tour) ; `abriRoute` de `route.glsl` relit les 3 cases. Changement de `route.glsl` limité à `neigeRoute` et `abriRoute` (aucun appel `fbm`, grain ni fissure touché : WFS-424).
- RG-02 / RG-03 : nouvelle ligne du bloc decor `tunnel_eclairage type pas_m pas_seuil_m seuil_m` (format des blocs 3 ; défaut : sodium, 12 m, 6 m, 60 m) ; luminaires modélisés pièce par pièce (`tunnel_luminaire.*`), deux files sous la dalle, chemin de câbles ; lumière de nuit par le registre `lampadaires()` (style Plage : sans disque de halo), aucune lumière ajoutée au moteur.
- Trouvé en regardant les captures de `main` : la bande de sol entre la chaussée et les piédroits restait enneigée, et les lampadaires de rue (mât de 7,6 m) traversaient la dalle à 5,6 m : corrigés dans le décor générique (sol de béton sec sous le tunnel, pas de lampadaire de rue sous un tunnel).
- Carte d'essai `noel-test` : 2e tunnel (segments 1060 à 1150, en courbe), captures `neige-tunnel2-*` et `neige-tunnel-entree-arriere` ; `WFS_NEIGE=tunnels` ne lance que les scènes des tunnels.
- Fichiers : `shaders/route.glsl`, `src/render/route_revetements*`, `src/world/cartes_v2/*`, `paquets/sources/noel-test/*`, `tests/test_cartes_v2_*`, `CMakeLists.txt` (une ligne : nouveau test), `references/reel/cartes_v2/tunnel.md`.
- Vérifié : compilation du banc, `test_cartes_v2_abris` (nouveau), `test_cartes_v2_saison`, `test_cartes_v2_donnees` : OK. **Pas encore vérifié** : le rendu (banc en cours), le coût en Basse (RG-04), tous les tests CPU.
- **Risque 1 (limite)** : le matériau de la route n'a que 3 nombres libres : **3 tunnels au plus par carte** restent secs (un tunnel de plus de 1,2 km en prend 2) ; au-delà, une ligne au journal du jeu et le tunnel reste enneigé dedans. Lever la limite demande une donnée par sommet de la route (noyau, ticket E0). Décision attendue de : PO.
- **Risque 2 (avis de l'expert-realisme-3d)** : de jour, l'exposition automatique ne descend pas sous EV 10 (`eclairage_unites.h`, E13, hors zone) : l'intérieur du tunnel reste plus sombre qu'en vrai, luminaires allumés compris. À juger sur les captures ; pas de compensation en gonflant l'émissif. Décision attendue de : PO.
- Sources de l'espacement : CIE 88:2004 et dossier pilote du CETU, **de mémoire** (réseau limité à GitHub) : valeurs marquées « à confirmer » dans `references/reel/cartes_v2/tunnel.md`.
### 2026-11-11 18:42:11 — WFS-428 fusionné par le lead dev (lead-dev)
- PR #357, revue du commit 8ebad8a2, fusion en squash e4cbb9f1 (titre `WFS-428 : … (#357) (Claude, agent dev (01UY1Vv5Nm))`). « ✅ Approuvé par le lead dev » écrit dans la PR en commentaire (GitHub refuse l'approbation : même compte).
- Vérifié : 22 paires de captures ouvertes (planche du cube : la Golf est maintenant dans la face +Z ; vues du jeu identiques à l'octet ; Basse : bruit de 57 pixels) ; diff lu en entier (3 fichiers : `environnement.h/.cpp`, `eclairage_physique.cpp`) ; seul appel du jeu en `sansJoueur = true` (WFS-322 gardé), garde `kFacesValides` et `bench_passes.cpp` inchangés, aucune tolérance touchée.
- Refait en local : construction et `tests_cpu.sh` 108/108 ; essai de fusion avec `main` (WFS-429) 109/109 ; `banc_groupe.sh realisme-2` : M-01 nuit 6/6 (+Z −0,06 %), jour 6/6, aucune « capture abandonnée », « partie 2, 2 erreur(s) » = WFS-377 seulement.
- Remarques non bloquantes : capture de la Campagne de jour non fournie (défaut sans effet visible, preuve par le journal du banc) ; pas de test CPU (passe Vulkan, contrôlée par `realisme-2`).
- Statut : 🟣 Fusionné (#357) ; ligne retirée de `suivi-pr.md` ; ligne `qa/Check.md` ajoutée. `realisme-2` ne sera vert aux bancs du matin qu'avec WFS-377.

### 2026-11-11 18:42:41 — WFS-428 : fusionné (#357, e4cbb9f1), session terminée (Claude, agent dev (01UY1Vv5Nm))
- PR #357 fusionnée en squash par le lead dev ; ticket 🟣 (US technique). Remarques non bloquantes du lead dev notées : pas de capture Campagne de jour (défaut sans trace à l'image, preuve par le journal du banc), pas de test CPU nouveau (logique dans une passe Vulkan, contrôlée par `realisme-2`), choix explicite de `sansJoueur` pour toute future capture.
- `realisme-2` reste rouge tant que **WFS-377** (choix « Ultra ») n'est pas fusionné ; ensuite, seules les règles M-01 et WFS-213 couvertes ici.
- Un seul sujet par session : je m'arrête, aucun autre ticket demandé.

### 2026-11-11 18:59:44 — WFS-424 : risque signalé, gain impossible (Claude, agent dev3d (01UY1Vv3cC))
- Constat : le `fbm` visé (route.glsl l. 129, 191, 193, 222) ne tourne pas sur la chaussée de la Ville ni de la Corniche (222 : repli sans textures ; 191-193 : rive de la Campagne ; 129 : neige). En Basse la chaussée lit déjà une seule texture précuite (scan CC0 256 px). Banc PC (WFS_PERF_SCENE, Mimosas, Basse, nuit, pluie) : tout le bruit retiré → aucun gain mesurable ; route plate → ≈ 8 % au mieux. Détail : `qa/pr/WFS-424/mesures.md` (branche `feature/waforspeed-424`).
- Conséquence : RG-01 est déjà le cas ; l'objectif « Ville sous 30 ms » ne peut pas venir de la route.
- Proposition : annuler WFS-424 (sans objet, comme WFS-422), aucun code changé ; chercher le coût ailleurs avec le banc du téléphone (WFS-420).
- Décision attendue de : utilisateur (il a donné le ticket lui-même), puis PO.

### 2026-11-11 19:19:59 — WFS-420 : banc de performance du téléphone codé, accroches annoncées (Claude, agent dev3d (01UY1Vv3k4))
- Fait : menu caché de WFS-419 → bouton « Banc auto » → écran du protocole (RG-05, relevés batterie / charge / température quand le téléphone les donne) → banc seul : Ville de nuit (pluie) puis de jour, caméra Arrière, graine de trafic fixe, pilote automatique, 14 essais par moment (tout éteint, 12 interrupteurs un par un, contrôle tout éteint), 30 s de chauffe + 60 s de mesure → tableau à l'écran + fichier `diagnostic-AAAA-MM-JJ-hhmm.txt` (dossier de `rendu-mesure.txt`) + « Partager ». « Arrêter » ou quitter la course garde les essais finis.
- Mesures par essai : médiane, 95 %, 99 %, img/s ; GPU brut par passe (ombre, reflet, scène, post) et total, règle des 90 % ; processeur (logique, préparation, envoi, attente du GPU) ; triangles, appels, matériaux distincts ; mémoire graphique par type (textures par format, maillages, cibles, autres) ; statut thermique et température de la batterie. Qualité automatique figée pendant le banc (essais à résolution égale).
- Accroches annoncées : `src/game/game.{h,cpp}` (+8 lignes : `Game::piloteBanc`, temps processeur de update / render lus seulement pendant le banc), `src/game/course_system.{h,cpp}` (+4 : `courseRejouer()`), `src/platform/desktop_main.cpp` (+2 : banc PC `banc-telephone`). Rendu (zone E4) : `renderer.{h,cpp}` (cumul GPU brut, matériaux distincts sur demande), `vk.{h,cpp}` + nouveau `memoire_gpu.{h,cpp}` (mémoire par type), `qualite_systeme.cpp` (1 ligne). Aucun changement d'`android_main.cpp` (partage par `reseau::partager` existant).
- Vérifié : compilation, tests CPU (nouveau `test_banc_telephone`), banc PC `banc-telephone` en 2400x1080 : 28 essais sur 28, fichier écrit, « Arrêter » OK, 0 écart. Limites : vraies durées (≈ 42 min) et horodatages GPU seulement vérifiables sur le téléphone ; cubemaps d'environnement hors du compte de mémoire (comme le total de WFS-297).

### 2026-11-11 19:22:10 — WFS-424 annulé (sans objet), arbitrage demandé par l'utilisateur (po-scrum-master)
- L'utilisateur a renvoyé la question de WFS-424 au PO (« Regarde avec po »). Faits vérifiés dans `shaders/route.glsl` : le `fbm` visé (l. 129, 191, 193, 222) ne tourne pas sur la chaussée de la Ville ni de la Corniche (222 = repli sans textures de l'enrobé, présentes ; 191-193 = Campagne ; 129 = neige) ; en Basse, la route lit déjà une texture précuite (RG-01 déjà vraie). Mesures du développeur (`qa/pr/WFS-424/mesures.md`, branche) : gain nul en retirant tout le bruit, ≈ 8 % au mieux avec une route plate.
- Décision : **❌ Annulé (sans objet)**, comme WFS-422. Aucun code, aucune PR, branche gardée. Session Claude, agent dev3d (01UY1Vv3cC) terminée (un seul sujet). WFS-449 dépend maintenant de WFS-423 seul. Le vrai coût de la Ville se cherche avec WFS-420 (banc sur téléphone, en cours).

### 2026-11-11 19:29:08 — WFS-383 : PR #358 ouverte (sols selon la pente, pins d'Alep) (Claude, agent dev3d (01UY1Vv1zt))
- Fait : shader de sol `shaders/sols.glsl` (pré / herbe sèche < 25°, terre et maquis 25 à 40°, roche > 40°, couleurs mesurées, feuilles mortes en automne), `src/world/sols.{h,cpp}` (matériau par climat, lissage des normales, climat des cartes), Corniche (sol naturel, pins d'Alep au lieu des cônes, bosquets aléatoires sur les pentes), Campagne / Grand Prix / cartes de données (ligne `sol` du bloc decor ; automne-test : `sol automne` ; Noël inchangé), test `test_sols`.
- **Accroches annoncées (≤ 10 lignes chacune)** : `shaders/scene.frag` (2 lignes : include, choix du sol), `src/game/game.cpp` (2 lignes : sol de la carte), `src/world/decor_carte.h` (pièce P_TERRE, `solNaturel`), `src/world/circuit.cpp` (1 ligne), `src/world/cartes_v2/carte_donnees*` et `carte_donnees_decor.cpp` (ligne `sol`).
- Vérification : 108 tests CPU verts sur la branche synchronisée ; 28 captures avant / après (corniche, campagne, neige, perf) refaites après la fusion de WFS-426, + Corniche jouée avec son paquet (banc `batiments`, `WFS_CARTE=plage`, 0 écart, sol et pins visibles).
- Limites : performance du S21 FE non mesurée (banc PC lavapipe seulement : scène Corniche Basse +1 % puis +12 % selon la série, bruit du même ordre) ; pin d'Alep procédural (`veg::pin`) faute de modèle CC0 / CC BY sur les sites autorisés (écart à RG-03 expliqué dans la PR) ; aucun téléchargement fait.
- Suite : avis de l'agent 3D (E15), puis lead dev.

### 2026-11-11 19:42:03 — WFS-377 : PR #360 ouverte (banc realisme, choix « Ultra » retrouvé) (Claude, agent dev3d (01UY1Vv51w))
- Fait : `src/render/bench_passes.cpp` seul (bloc RG-04 / P-05) : le banc touche l'onglet qui contient « Graphismes » (`ongletDe(L_QUALITE)`), puis retrouve « Ultra » par son rôle (la rangée qui passe de 3 à 4 choix quand la Haute a tenu, choix le plus à droite), plus par la « 8e zone ». Étape « course en Ultra » : les captures des sondes d'éclairage (WFS-151) sont comptées à part ; toujours 1 capture du banc et 256 px exigés. Aucune accroche hors du fichier, aucun seuil changé, jeu inchangé.
- Vérifié (lavapipe, `WFS_BANC=realisme WFS_PASSES_SEUL=1`, main à jour avec #357) : main « Ultra introuvable », parcours 2 / 5, 2 écarts → branche « Ultra trouvé (zone 11 sur 21) », Rendu Ultra, capture de 256 px, parcours 5 / 5, **0 écart**. Tests CPU 109 / 109. Captures `qa/pr/WFS-377/`.
- Limite : groupe CI `realisme-2` complet non relancé (passes seules en local) ; confirmation aux bancs du matin. Aucune CI de branche lancée.

### 2026-11-11 19:44:56 — WFS-450 créé : lampadaires qui s'éteignent d'un coup la nuit en Basse, demande de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- Défaut vu par l'utilisateur (APK 1013, S21 FE) : Corniche, chaussée qui devient noire d'un coup puis se rallume quand des adversaires sont derrière ; Ville, taches blanches à bords nets qui apparaissent d'un coup. Cause trouvée par le lead dev : quota de 4 lumières par pixel rempli par les phares des voitures derrière (tri par distance à la caméra), aucun fondu sur les phares des autres voitures. Défaut ancien (WFS-263, 226), rendu visible par WFS-421.
- **WFS-450** (P1, E13 + E4, nouvelle session `@dev`, juste après WFS-420) : ordre lampadaires avant les autres voitures, quota compté seulement si la lumière éclaire, fondu de 0,3 s ; vérification en mouvement (suite d'images, aucune chute de plus de 20 %) ; pas de perte d'images sur le S21 FE. Règles : `regles-de-gestion/WFS-450.md`.

### 2026-11-11 19:52:54 — WFS-420 fusionné par le lead dev (lead-dev)
- PR #359 relue sur e0a6939f : 6 captures ouvertes, tout le diff lu (28 fichiers), branche construite et 110 tests CPU verts refaits (dont `test_banc_telephone`). RG-01 à RG-06 couverts ; accroches hors zone annoncées (19:19:59).
- « ✅ Approuvé » en commentaire (GitHub refuse l'approbation : même compte), puis squash 99b63a61 « WFS-420 : banc de performance automatique sur le téléphone (#359) (Claude, agent dev3d (01UY1Vv3k4)) ».
- Statut : 🟡 À valider (utilisateur) (vrai banc sur le téléphone). 5 remarques non bloquantes écrites dans la revue. Ligne retirée de `suivi-pr.md`, `qa/Check.md` complété.

### 2026-11-11 19:53:23 — WFS-420 : fusionné (#359), session terminée (Claude, agent dev3d (01UY1Vv3k4))
- Fusion en squash par le lead dev (99b63a61), statut 🟡 À valider (utilisateur) : sur le téléphone, menu caché (5 touchers sur « Réglages ») → « Banc auto » → protocole → ≈ 42 min → tableau, fichier `diagnostic-*.txt` et « Partager ».
- Pour le PO (remarques non bloquantes du lead dev, à mettre dans un ticket de suite si utile) : commentaire de `cameraTempsSimule` déplacé (game.h) ; remarque « colonnes GPU vides » affichée dès qu'un seul essai n'a pas de temps GPU ; tri des matériaux à chaque image pendant la mesure (coût égal pour tous les essais) ; banc bloqué en « chargement » si la Ville ne se charge pas (pas de bouton « Arrêter » à ce moment) ; jeu tué pendant le banc → sauvegarde sur la Ville.
- Session terminée : un seul sujet par session, aucun autre ticket demandé.

### 2026-11-11 19:54:05 — WFS-420 : fusion #359 faite sur ordre de l'utilisateur, remarque pour le PO (lead-dev, session 01UY1Vv5Nu)
- L'utilisateur a ordonné « merge la 420 avant » : squash 99b63a61 par cette session, pendant que l'autre lead dev finissait sa revue (ses contrôles : 110 tests verts, lignes `TACHES.md`, `qa/Check.md`, `suivi-pr.md` déjà écrites par lui).
- Remarque en plus, non bloquante, pour le PO : le banc ne fixe pas le mode de course (`modeCourse()`, `reglages_banc.cpp`, `demarrerEssai`) ; lancé après une Course à 5 ou une Poursuite, il mesure avec des adversaires ou la police (en Poursuite, un « ARRÊTÉ » fige la course pendant la mesure). À corriger avant de comparer deux fichiers : forcer Checkpoint pendant le banc, ou écrire le mode dans le fichier. Détail dans la revue de la PR #359.

### 2026-11-11 19:56:20 — WFS-450 et WFS-451 attribués au lead dev, ordre de l'utilisateur (po-scrum-master)
- Ordre de l'utilisateur (19:54:10 UTC, session lead dev 01UY1Vv5Nu, vérifié dans les messages de la session) : « Si ta des corrections de 420 tu les fait ici + 450 ».
- **WFS-450** (P1, E13 + E4) : 🔧 En cours, **Claude, lead dev (01UY1Vv5Nu)**, branche `feature/waforspeed-450`. Zone, règles et critères : `regles-de-gestion/WFS-450.md` (inchangés).
- **WFS-451 créé** (P2, [TECHNIQUE], E4) : corrections de WFS-420 venues de la revue de la PR #359 : mode de course forcé en Checkpoint pendant le banc, rétabli à la fin et écrit dans le fichier ; commentaire de `cameraTempsSimule` remis en place ; remarques « horodatages non disponibles » et « le téléphone a chauffé » affichées seulement dans leur vrai cas ; test CPU. Zone : `src/ui/ecrans/reglages_banc.*`, `src/render/banc_telephone.*`, `src/game/game.h` (commentaire), `tests/test_banc_telephone.cpp`. Même session, branche `feature/waforspeed-451`, PR à part. Règles : `regles-de-gestion/WFS-451.md`.
- **Décision du PO** : ticket à part plutôt que dans la PR de WFS-450 (deux sujets sans rapport, WFS-450 attend la validation de l'utilisateur, revue plus simple). **Ordre : WFS-451 d'abord** (court, et la mesure RG-05 de WFS-450 se fait avec ce banc), PR ouverte, puis WFS-450 sans attendre la fusion.
- **Exceptions, sur ordre de l'utilisateur seulement** : deux sujets dans une même session ; deux tickets E4 en même temps (fichiers séparés). Les deux PR sont relues et fusionnées par une autre instance du lead dev.
- `track-agent.md` remis à jour : lignes des sessions closes enlevées (WFS-428, 420, 424), derniers envois de WFS-383 (19:41) et WFS-392 (19:47).


### 2026-11-11 20:10:21 — WFS-377 fusionné par le lead dev (lead-dev)
- PR #360 relue sur ae957fcd : 4 paires de captures ouvertes, diff lu (seul `src/render/bench_passes.cpp`), construction et banc `realisme` (`WFS_PASSES_SEUL=1`) refaits : 5 / 5 étapes, Ultra 256 px, 0 écart ; tests CPU 109/109 (`test_audio_chocs` instable sous charge, vert relancé seul). « ✅ Approuvé » en commentaire (même compte GitHub), fusion en squash 6bb5ef3f. Statut 🟣 ; ligne retirée de `suivi-pr.md` ; `qa/Check.md` à jour.

### 2026-11-11 20:10:51 — WFS-377 : fin, PR #360 fusionnée par le lead dev (6bb5ef3f), session terminée (Claude, agent dev3d (01UY1Vv51w))
- Le banc `realisme` (passes) retrouve « Ultra » dans l'onglet Affichage : parcours WFS-150 5 / 5, 0 écart. À confirmer aux bancs du matin (groupe `realisme-2`, avec WFS-428). Session arrêtée (un seul sujet par session).


### 2026-11-11 20:40:40 — WFS-392 : code fini, captures et mesures, PR à ouvrir (Claude, agent cartes (01UY1Vv2ghpU))
- Fait : chaussée et trottoir secs sous chaque tunnel (3 au plus), luminaires modélisés en deux files, lumière de nuit par le registre des lampadaires, sol sec jusqu'aux piédroits, plus de lampadaire de rue sous la dalle. **Dalle du tunnel générique relevée de 5,6 à 7,6 m** (comme la Ville et la Corniche) : à 5,6 m, la caméra de la vue Arrière (6,3 m) passait au-dessus de la dalle (défaut de WFS-346 vu sur les captures de `main`).
- Vérifié : compilation complète ; `tests_cpu.sh` : 111 tests réussis ; banc `WFS_NEIGE=tunnels WFS_BANC=neige` : 0 écart, 7 captures regardées (`qa/pr/WFS-392/`) ; images « avant » faites sur `origin/main` + 2e tunnel d'essai + captures du banc, sans la correction.
- RG-04 : `WFS_NEIGE=cout` (2e tunnel, qualité Basse, 413 images, 2 passages alternés) : passe de scène de jour 294,2 / 291,6 ms sur main, 271,1 / 273,2 ms sur la branche ; de nuit 265,9 / 261,4 ms sur main, 254,2 / 251,4 ms sur la branche : pas de surcoût (rendu logiciel lavapipe, pas un GPU de téléphone).
- Synchronisé avec `main` (conflit du journal : les deux côtés gardés). WFS-424 annulé entre-temps : plus de partage de `route.glsl`.
- Limites : 3 tunnels secs au plus par carte ; tunnel sombre de jour (exposition ≥ EV 10, E13) ; sources de l'espacement citées de mémoire ; parcours complet non rejoué à la main (carte d'essai cachée).
### 2026-11-11 20:24:59 — WFS-450 : codé, PR #361 ouverte ; accroches annoncées (Claude, lead dev (01UY1Vv5Nu))
- Ordre de l'utilisateur en cours de session : « dev la 450 en urgence » : WFS-450 passe avant WFS-451 (dont le code est poussé sur `feature/waforspeed-451`, captures à faire ensuite).
- Cause trouvée au nouveau banc en mouvement (`WFS_BANC=lumieres-mouvement`) : les 4 lampadaires éclairants étaient pris de 4 segments derrière le joueur à 160 devant ; un lampadaire sortait de la liste 5 m derrière la voiture alors qu'il éclaire 26 m autour de lui : chaussée noire d'un coup (Corniche −45 % d'une image à la suivante, Ville −85 %), cachée jusque-là par les disques au sol retirés par WFS-421. La cause du ticket (quota de 4 lumières pris par les voitures de derrière) est corrigée aussi.
- Fait (Basse seulement) : lampadaires gardés jusqu'à 36 m derrière, entrée et sortie en fondu de 0,3 s ; ordre joueur, lampadaires, autres voitures (RG-01) ; quota de 4 juste et doux dans `eclairage.glsl` (RG-02, RG-04) ; phares des autres voitures en fondu 0,3 s et 55-70 m (RG-03) ; tests dans `tests/test_phares.cpp` (RG-07).
- Accroches hors zone annoncées : `src/render/renderer.h` (+2 lignes : champ `rang` d'une lumière), `src/game/course5.h` et `course5_system.cpp` (+13 lignes : `course5BancSuiveurs`, adversaires tenus derrière le joueur, banc seulement), `src/platform/desktop_main.cpp` (+2 : banc `lumieres-mouvement`).
- Vérifié : banc en mouvement, 2 scènes × 45 images à 10 img/s, avant / après sur le même trajet : Corniche 45 % → 9 %, Ville 85 % → 17 % (RG-06 : 20 % au plus), 0 écart ; 110 tests CPU verts, branche synchronisée avec `main`. Paquets installés pour construire : `glslc`, `mesa-vulkan-drivers`, `ninja-build`, `imagemagick`, `libvulkan-dev` (dépôts Ubuntu).
- Limites : RG-05 (images par seconde sur le S21 FE) non mesuré (pas de téléphone) ; le même retrait du lampadaire existe en Moyenne et en Haute (code gardé, RG-01) : décision du PO ; captures montées à la main (le banc n'existe pas sur `main`, « avant » rejoué par `phares::avant450`).
- Suite : avis de l'agent 3D (RG-08), puis revue par une autre instance du lead dev.

### 2026-11-11 20:41:50 — WFS-392 : PR #362 ouverte, lead dev appelé (Claude, agent cartes (01UY1Vv2ghpU))
- PR #362 (`feature/waforspeed-392`, commit 179db9ac) : chaussée sèche sous chaque tunnel des cartes de données, tunnel générique éclairé. 7 captures « après », 5 « avant » dans `qa/pr/WFS-392/`.
- `TACHES.md` 🔀 En PR, `track-agent.md`, `suivi-pr.md` (🆕) mis à jour. Détail du travail, mesures et limites : entrées WFS-392 du journal sur la branche.
- **Pour le PO** : (1) 3 tunnels secs au plus par carte (cases du matériau de la route) : lever la limite demande un ticket E0 ; (2) tunnel sombre de jour : plancher d'exposition EV 10 et pas de lumière locale de jour (E13) : à décider (ticket d'exposition sous tunnel, ou accepter) ; (3) dalle du tunnel générique relevée de 5,6 à 7,6 m pour la caméra de la vue Arrière.

### 2026-11-11 20:51:14 — WFS-450 fusionné par le lead dev (lead-dev)
- PR #361 (squash 5219c17a), relue par une autre instance que l'auteur, sur ordre de l'utilisateur. Avis agent 3D VALIDÉ. Statut 🟡 À valider (RG-05 sur le S21 FE).
- Pour le PO : retrait du lampadaire 5 m derrière la voiture encore en Moyenne / Haute ; taches jaunes ovales sur la chaussée de la Ville (antérieures).

### 2026-11-11 20:52:57 — Règle de l'utilisateur : capture des tests CPU dans chaque PR (lead-dev)
- Ajoutée à la checklist de livraison des développeurs (`agent-developer/references/checklist-livraison.md`, section Preuves) et au point 9 de la revue du lead dev : la fin du journal de `tests_cpu.sh` (ligne « N test(s) réussi(s) », commit testé) est collée dans la PR sous « Tests CPU », journal complet dans `qa/pr/WFS-XX/tests-cpu.log` ; sans elle, le lead dev refuse.

### 2026-11-11 20:53:37 — Capture des tests CPU : exception pour les PR en cours (lead-dev)
- Sur demande de l'utilisateur, les PR déjà ouvertes le 2026-11-11 à 21:00 UTC (ex. #358) ne sont pas tenues de fournir la capture des tests CPU.

### 2026-11-11 21:01:27 — WFS-392 fusionné par le lead dev, ordre de l'utilisateur (lead-dev)
- PR #362, squash 4e3320ad, après l'approbation écrite dans la PR sur 179db9ac par le lead dev appelé par le développeur. Statut 🟣 Fusionné.

### 2026-11-11 21:14:41 — WFS-451 : codé, PR #363 ouverte (Claude, lead dev (01UY1Vv5Nu))
- Fait : mode Checkpoint forcé pendant tout le banc du téléphone et mode du joueur rétabli à la fin (`ModeForce`, `reglages_banc.cpp`), mode écrit dans l'en-tête du fichier ; commentaire de `cameraTempsSimule` remis (`game.h`) ; remarques du tableau : essais sans temps GPU nommés, « horodatages non disponibles » seulement si aucun essai n'en a, « a chauffé » seulement si le contrôle est plus lent de 5 % ou plus (`banc_telephone.cpp`) ; tests dans `test_banc_telephone.cpp`. Aucun fichier hors de la zone du ticket.
- Vérifié : banc PC `banc-telephone` (2400x1080) lancé après une Poursuite : 28 essais sur 28, 0 écart, mode Poursuite rendu à la fin et après « Arrêter » ; 111 tests CPU verts sur 530d7f60 (branche synchronisée avec `main`, WFS-450 compris), journal dans `qa/pr/WFS-451/tests-cpu.log` ; captures avant / après par `tools/qa/captures_pr.sh`.
- Limites : les « avant » de `main` partent en Checkpoint (pas de police à l'image) ; le menu ne montre pas le mode (preuve : journal et contrôle du banc).
- Suite : revue par une autre instance du lead dev ; après la fusion des deux tickets, cette session s'arrête.

### 2026-11-11 21:20:03 — WFS-392 : revue du lead dev appelé par le développeur, close après la fusion (lead-dev)
- PR #362, commit relu 179db9ac : 12 captures ouvertes, diff lu en entier (32 fichiers), construction et `tests_cpu.sh` 111/111, banc `WFS_NEIGE=tunnels WFS_BANC=neige` rejoué 0 écart. « ✅ Approuvé » écrit à 20:53, puis **retiré à 20:54** (« ↩️ Retour lead dev ») : `main` venait de recevoir WFS-450 (lumière des lampadaires de nuit, le mécanisme qui éclaire le tunnel) et la règle de la capture des tests CPU.
- La PR a été **fusionnée à 21:01 par une autre instance du lead dev** (ordre de l'utilisateur, 4e3320ad), sur l'approbation de 20:53, alors que la ligne de `suivi-pr.md` était 👀 à mon nom depuis 20:42 et que le retour de 20:54 était dans la PR. Pour le PO : avant de fusionner sur l'approbation d'un autre, relire le **dernier** commentaire de la PR.
- Vérifié après coup, sur un arbre identique à 4e3320ad (branche + `main`, hors journal) : construction complète, 111 tests réussis, banc tunnels 0 écart, captures de nuit bonnes avec WFS-450 ; capture en **qualité Basse** faite par moi (ligne locale non poussée) : chaussée sèche de jour, tunnel éclairé de nuit. Capture des tests CPU : PR dispensée (ouverte avant 21:00). **Retour levé, rien à corriger** : écrit dans la PR. Statut inchangé : 🟣 Fusionné.
- Pour le PO : (1) de jour le tunnel reste sombre, les luminaires n'éclairent pas la chaussée (plancher d'exposition EV 10, E13) : ticket E13 à créer ; (2) 3 tunnels secs au plus par carte (noyau, E0) ; (3) `profil()` perd la dernière bande de sol sous un tunnel si la carte en a 6 ; `coderAbris` attend des tunnels triés ; commentaire « dalle à 5,6 m » faux dans `carte_donnees_decor.cpp` ; (4) sources de l'espacement « à confirmer » ; (5) boîte marron nue sur le trottoir du tunnel de la carte d'essai (obstacle antérieur, carte cachée).

### 2026-11-11 21:20:47 — WFS-392 : fin, session terminée (Claude, agent cartes (01UY1Vv2ghpU))
- PR #362 fusionnée en squash par le lead dev (4e3320ad), ticket 🟣 Fusionné ; il passera ✅ par les bancs du matin (pas de 🟡 : défaut vu par le lead dev, pas par l'utilisateur).
- Reste pour le PO (rien de bloquant, non corrigé car la session s'arrête après la fusion) : limite de 3 tunnels secs par carte (ticket E0 pour la lever) ; tunnel sombre de jour (ticket E13) ; sources de l'espacement à confirmer ; remarques du lead dev : commentaire périmé « dalle à 5,6 m » dans `place()` de `carte_donnees_decor.cpp`, dernière bande de sol perdue sous un tunnel si la carte a 6 bandes, alternance des files inscrites qui repart à chaque zone.
- Un seul sujet par session : cette session ne demande plus de ticket.

### 2026-11-11 21:27:54 — WFS-451 fusionné par le lead dev (lead-dev)
- PR #363 (codée par « Claude, lead dev (01UY1Vv5Nu) » sur ordre de l'utilisateur) relue par **une autre instance du lead dev** : 13 captures ouvertes, tout le diff lu (5 fichiers de code), construction complète, **111 tests CPU verts**, banc `banc-telephone` 2400x1080 relancé (28 essais sur 28, 0 écart, mode Poursuite rendu à la fin et après « Arrêter », ligne « mode de course : Checkpoint (forcé par le banc) » dans le fichier).
- « ✅ Approuvé par le lead dev » écrit dans la PR (commentaire : même compte GitHub), puis fusion en squash : **df461365**. WFS-451 → 🟣 Fusionné (US technique) ; WFS-420 reste 🟡.
- Pour le PO (non bloquant) : (1) après le banc, « Reprendre » en mode Poursuite ou Course à 5 fait démarrer la poursuite ou la grille en pleine course, sans décompte (`reglages_banc.cpp:174`) ; (2) le banc PC ne joue pas une vraie Poursuite avant le banc : à voir sur le téléphone ; (3) le fantôme du record de la Ville peut s'afficher pendant le banc en Checkpoint (déjà vrai depuis WFS-420) ; (4) écart de 4,6 % affiché « +5 % » sans remarque de chauffe.

### 2026-11-11 21:28:28 — WFS-450 et WFS-451 fusionnés, session terminée (Claude, lead dev (01UY1Vv5Nu))
- WFS-450 : fusionné (#361, 5219c17a), 🟡 À valider (utilisateur) sur l'APK de `main` (Corniche et Ville de nuit, Basse, voitures derrière ; images par seconde sur le S21 FE à mesurer). WFS-451 : fusionné (#363, df461365), 🟣.
- Pour le PO : (1) le retrait du lampadaire 5 m derrière la voiture existe aussi en Moyenne et en Haute (non changé, RG-01 de WFS-450) ; (2) remarques non bloquantes de la revue #363 : « Reprendre » après le banc relance le mode du joueur en pleine course sans décompte (`reglages_banc.cpp`), fantôme du record visible pendant le banc, écart de 4,6 % affiché « +5 % » sans remarque de chauffe ; (3) avis de l'agent 3D sur #361 : taches jaunes ovales isolées sur la chaussée de la Ville et tête de lampadaire brûlée sur la Corniche, déjà là avant le ticket.
- Session terminée : les deux sujets confiés par l'utilisateur sont fusionnés, aucun autre ticket demandé.

### 2026-11-11 21:32:01 — WFS-452 créé : roues des adversaires figées, demande de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- Défaut vu par l'utilisateur (APK 1018, Course à 5) : les roues de l'adversaire ne tournent pas. Cause lue dans le code : roues animées (WFS-60) branchées seulement pour le joueur et le trafic ; Course à 5 (`course5_system.cpp:207`), duel (`duel_system.cpp:129`), police (`poursuite_system.cpp:348`) et fantôme (`course_system.cpp:111`) dessinés sans pose de roues.
- **WFS-452** (P1, E3 + E10, nouvelle session `@dev`) : réutiliser `roues::pose*` et `roues::matrice`, roulage et braquage pour les 4 cas, vérification en suite d'images à 30 km/h. Gyrophares en cubes de la police : hors périmètre (non demandé). Règles : `regles-de-gestion/WFS-452.md`. (Numéro 452 : 450 et 451 pris.)

### 2026-11-11 22:20:29 — Début WFS-325 (lot 1) : attribué à Claude, agent dev3d (01Us6uCbUS) (po-scrum-master)
- Demande d'une nouvelle session `@dev` (= `@dev3d`, règle du 2026-11-11 15:59 ; message « @devgo », aucun sujet souhaité), session session_01Us6uCbUSwvJmwKUXB1sbNs, aucun ticket reçu avant : 1er et seul sujet de cette session.
- Choix d'après l'ordre des 20 de l'utilisateur (12:08 UTC) : 1 à 9 fusionnés ou annulés ; 10. WFS-383 en PR #358 (2e « À REPRENDRE » de l'agent 3D à 21:46) ; 11. WFS-371 attend WFS-383 (même file Corniche) → **12. WFS-325**, premier permis : dépendances WFS-323, WFS-414 et WFS-324 fusionnées (🟡), E10 libre (aucun 🔧 / 🔀), et depuis 15:59 une session `@dev` reçoit les tickets `@devartiste` de l'ordre. WFS-452 (P1, créé à 21:32, hors de l'ordre des 20), WFS-378 (29e de l'ordre des 50) et WFS-446 (« après les tickets de l'ordre des 20 », bandeau de 16:15) passent après.
- **WFS-325 découpé** (une journée par ticket) : WFS-325 = **lot 1 : Golf 8 GTI, M4, 911, SF90** (fiches du 1er lot de WFS-197, RG-05 de la M4 et de la 911 compris) ; lots 2 (S3, Giulia, Gallardo, GT86), 3 (Model 3, V60, Range Rover Sport, Compass), 4 (208, Corsa, Abarth 500) : numéro donné à la fusion du lot précédent, nouvelle session chacun. Corvette C7 et Brezza : WFS-395, après le lot 1 (mêmes fichiers).
- Règles complétées dans `regles-de-gestion/WFS-325.md` : RG-06 à RG-09 (pièces par voiture, matières de la charte, rien de retiré : volant de la Golf de WFS-414, rétroviseurs de WFS-324, habitacle de WFS-349 ; performance), zone, fichiers à ne pas toucher, captures demandées (bancs `cockpit`, `cockpit-volant`, `cockpit-reference`, témoins hors lot), critères. Après fusion : 🟡 À valider (utilisateur).
- Zone : `src/render/vehicule_cockpit*`, `shaders/vehicule*.glsl` (≤ 10 lignes si une matière l'exige), `assets/cars/interieurs/**`, `tests/test_cockpit*.cpp` ; accroches ≤ 10 lignes annoncées. Pas `camera_vue.*`, pas les fichiers de la PR #358 (`scene.frag`, `sols.glsl`, `game.cpp`, `src/world/**`), pas les `.wfm` extérieurs, pas les entrées `mustang` / `tcross`.
- **Décision du PO pour la suite** : **WFS-452** (P1, E3 + E10) peut partir en parallèle de WFS-325 vers la prochaine session `@dev` : sa zone (`src/game/course5_system.cpp`, `duel_system.cpp`, `poursuite_system.cpp`, `course_system.cpp`, `roues.*`, `tests/**`) ne contient aucun fichier de E10 ni de `vehicule_cockpit*`. Les autres tickets E10 (WFS-366, 395, 397, 444) attendent la fusion du lot 1.
- `track-agent.md` remis à jour : lignes des sessions closes enlevées (WFS-377 #360, WFS-392 #362, WFS-450 #361 et WFS-451 #363) ; ⚠️ WFS-383 : rien poussé depuis 19:41 (plus de 2 h, pas de réattribution avant 10 h).

### 2026-11-11 23:02:43 — WFS-325 : branche poussée, plan, accroche annoncée (Claude, agent dev3d (01Us6uCbUS))
- Branche `feature/waforspeed-325` poussée à 22:12 (premier commit 8dfa495) ; skills chargés : `agent-developer`, `waforspeed-natif`, `artiste-3d`.
- **Plan** : nouveau module sans rendu `src/render/vehicule_cockpit_details.{h,cpp}` (pièces propres des 4 voitures : aérateurs du modèle, commandes de la planche, console, garnitures de portière, plafonnier), branché dans `Interieur::construire` (`vehicule_cockpit.cpp`) **seulement** pour golf, m4, p911, sf90 (`detailsPropres`) : les 16 autres voitures gardent exactement les pièces communes de `main` (test). Visière propre de la M4, de la 911 et de la SF90 (RG-05, arbitrage du PO de 22:42) : table `avanceVisierePropre` (`vehicule_cockpit_defs.cpp`), lue par `ligneVisiere`.
- **Accroche annoncée** (≤ 10 lignes) : `CMakeLists.txt`, le nouveau `vehicule_cockpit_details.cpp` ajouté aux sources de `test_cockpit` (une ligne). Aucune autre accroche.
- **Limites** : (1) titre de la discussion non changé : l'outil `set_session_title` n'existe pas dans cette session (recherche faite) ; (2) **aucune photo de référence n'a pu être ouverte** : le réseau des agents refuse sketchfab.com et tout site hors GitHub (proxy, 403) ; les formes des 4 intérieurs suivent donc mes souvenirs de photos de presse (statut E de WFS-197), décrits au journal de fin pour chaque voiture, sans source ouvrable : à confirmer par l'utilisateur ou l'agent 3D.

### 2026-11-12 04:48:03 — WFS-325 : PR #364 ouverte, lead dev appelé (Claude, agent dev3d (01Us6uCbUS))
- **Fait** : pièces propres des 4 voitures du lot 1 (`src/render/vehicule_cockpit_details.{h,cpp}`, géométrie sans rendu) branchées dans `Interieur::construire` seulement pour golf, m4, p911, sf90 : aérateurs du modèle (rectangulaires à lunette chromée, à molette BMW, en stade pour la 911, ronds « turbine » pour la SF90 ; conduit sombre, lames, curseur), commandes de la planche (feux de détresse, climatisation, éclairage, bascules de la 911, démarrage tournant), console (bac, sélecteur du modèle, commande rotative de la M4, grille en H de la SF90, porte-gobelets, accoudoir), portières (décor, lumière d'ambiance Golf rouge et M4 blanche la nuit, poignée dans sa niche, accoudoir et jupe, lève-vitres, haut-parleur, tweeter, vide-poche), plafonnier. Visière propre (RG-05, arbitrage du PO de 22:42) : M4 32 mm (jour 8,1 mm), 911 32 mm (7,1 mm), SF90 40 mm (7,5 mm), 0 point caché. Matières du lot 1 dans la charte (RG-07) : chrome 0,12, noir laqué et carbone vernis 0,06, alu brossé 0,38 (nouveau). Nouveau banc `cockpit-vitrine` (aide à la revue).
- **Accroches** (annoncées) : `CMakeLists.txt` (1 ligne, source du test) ; `desktop_main.cpp` (5 lignes, banc `cockpit-vitrine`).
- **Vérifié** : 111 tests CPU verts (`qa/pr/WFS-325/tests-cpu.log`) ; `menu1-2400` code 0, parcours complet et parcours en vue Cockpit 0 écart ; captures 2340 × 1080 avant / après (4 passages de `captures_pr.sh`, 142 lignes + 8 planches recadrées) ; témoins S3, Giulia, Abarth : 24 captures identiques pixel pour pixel à `main` ; combinés des 13 autres voitures non retirées identiques ; part de la rue cachée identique à `main` (fenêtre 0,0 %).
- **Mesures (RG-09)** : appels de dessin Golf 42 → 46, M4 38 → 43, 911 38 → 41, SF90 37 → 40 ; mémoire de l'intérieur +0,58 à +0,85 Mo ; 21 840 à 28 896 triangles ajoutés ; durée d'image en vue Cockpit au banc PC (lavapipe, essai local non poussé) : aucune perte mesurable (Golf et M4, Moyenne et Basse, écarts de −1 à −4 %, dans le bruit). Basse ≥ 30 img/s : à mesurer sur le téléphone (banc PC à 1-2,6 img/s).
- **Photos de référence (RG-04)** : aucune n'a pu être ouverte (réseau des agents : GitHub seulement ; sketchfab.com refusé par le proxy, 403 ; les sites de presse et des constructeurs sont hors de la liste de `regles-reseau.md`). Les formes suivent mon souvenir des photos de presse de chaque intérieur (dossiers de presse des constructeurs et essais publiés à la sortie de chaque modèle), **statut E** au sens de WFS-197 (estimé, à confirmer), cotes estimées à ±1 cm ; ce que chaque photo montre :
  - **Golf 8 GTI (2020-2024)**, vue du siège conducteur : aérateurs rectangulaires aux deux bouts de la planche, à la hauteur des écrans ; deux aérateurs plats sous l'écran central, feux de détresse entre eux ; barre tactile de climatisation dessous ; commande d'éclairage tactile à gauche du volant ; liseré rouge sur toute la largeur et décor noir brillant ; console : bac de recharge, petit sélecteur DSG à bascule, démarreur, porte-gobelets, accoudoir ; portière : décor et lumière d'ambiance sous l'appui de vitre, poignée chromée dans une niche, accoudoir avec poignée de tirage et lève-vitres, haut-parleur, tweeter dans l'angle du rétroviseur ; plafonnier avec liseuses et bouton d'appel d'urgence.
  - **M4 Competition (G82, 2021)** : aérateurs à lunette chromée et molette, deux aux bouts et deux sous l'écran central posé sur la planche ; panneau de climatisation à touches et bascules ; commande d'éclairage à molette à gauche ; carbone sur la largeur ; console : sélecteur M gainé, commande rotative moletée avec ses touches, touches des modes, accoudoir ; portière : décor carbone, poignée chromée, grille de haut-parleur en alu ; lumière d'ambiance.
  - **911 Carrera 4S (992.1, 2019)** : planche plate et basse ; aérateurs étroits aux bouts ; écran encastré au milieu, aérateurs plats et rangée de 5 interrupteurs à bascule dessous (la place exacte des aérateurs du milieu, au-dessus ou au-dessous de l'écran, est mon point le moins sûr : à confirmer) ; commande de démarrage tournante en forme de clé à gauche du volant ; console : petit sélecteur électrique, touches ; portière : décor alu, grille alu.
  - **SF90 Stradale (2019)** : pas d'écran central ; aérateurs ronds à bague alu ; console carbone avec la platine alu en forme de grille de boîte manuelle (R, mode, départ arrêté) ; Alcantara ; portière : décor carbone, levier d'ouverture alu.
- **Limites** : titre de la discussion non changé (outil absent) ; aérateurs du milieu de la Golf et de la 911 placés juste sous l'ombre de l'écran central (fiches de WFS-197, écrans 6 à 17 cm devant la face) ; console et bas des portières visibles sur les vitrines, pas dans la vue droit devant.

### 2026-11-12 05:13:39 — WFS-325 (lot 1) fusionné par le lead dev (lead-dev)
- **PR #364** (Claude, agent dev3d (01Us6uCbUS)), commit relu c5258fed, reprise de la revue prise à 04:48 (instance arrêtée par un redémarrage, rien écrit dans la PR). Revue « ✅ Approuvé par le lead dev » postée en commentaire (GitHub refuse l'approbation : même compte), puis **fusion en squash : 20169653**. Branche gardée.
- **Captures toutes ouvertes** (PNG d'origine comparés pixel par pixel) : 32 paires `cockpit` des 4 voitures (jour, virage, braquage, carte, pluie, nuit, nuit en virage, plage de nuit), 36 paires `cockpit-volant` (M4, 911, SF90 : haut de la jante entier devant la visière ; Golf : volant identique), 30 paires `cockpit-reference` (15 identiques à 0 pixel), 24 paires témoins S3 / Giulia / Abarth (0 pixel), 20 vitrines, 8 planches. Aucune régression vue (rue, HUD, rétroviseurs, nuit) ; part de la rue cachée et pipelines identiques à `main` (journaux des bancs).
- **Diff entier lu** ; construction refaite (aucun avertissement, fichiers recompilés sans cache) et `bash tools/qa/tests_cpu.sh build` : **111 test(s) réussi(s)**, code 0 (branche fusionnée localement avec `main`). Aucun test affaibli, aucune référence changée, aucun fichier iOS ; accroches (`desktop_main.cpp` 5 lignes, `CMakeLists.txt` 1 ligne) annoncées.
- Remarques non bloquantes (écrites dans la PR) : lentilles du plafonnier en matière de glace de rétroviseur ; commentaires « 4 places » (5 vues) et « carbone verni (0,3) » ; triangles ajoutés en grande partie invisibles, Basse ≥ 30 img/s à confirmer aux bancs du matin et sur le téléphone ; matériau d'ambiance de nuit créé sans usage pour la 911 et la SF90.
- **Pour le PO** : RG-04 (photo de référence à côté de chaque capture) ne peut pas être tenue avec `regles-reseau.md` (sites de presse, des constructeurs et Wikipédia interdits ; le développeur l'a écrit au journal) : formes au statut E, la validation de l'utilisateur (🟡) tranche. Pour les lots 2 à 4, soit l'utilisateur ajoute un site de photos libres à la liste, soit la règle est réécrite (statut E accepté, à valider sur le téléphone). Points à confirmer : aérateurs du milieu de la 911 et de la SF90, commande d'éclairage de la M4.
- Statut : **🟡 À valider (utilisateur)** (`TACHES.md`), ligne ajoutée à `qa/Check.md`, ligne #364 enlevée de `suivi-pr.md`. La session du développeur se termine (un seul sujet) : à lui de mettre sa ligne de `track-agent.md` en « ⏹️ session terminée ».

### 2026-11-12 05:14:12 — WFS-325 (lot 1) : fin, session terminée (Claude, agent dev3d (01Us6uCbUS))
- PR #364 fusionnée en squash par le lead dev (20169653) sur le commit relu c5258fed ; statut **🟡 À valider (utilisateur)** ; réponse sous la revue écrite dans la PR ; réponse à l'utilisateur sous sa demande « Vue cockpit : … intérieur propre à chaque voiture » (`tache utilisateur.md`).
- **Pour le PO, lot 2 de WFS-325** (mêmes fichiers, nouvelle session) — remarques non bloquantes du lead dev, non corrigées ici : (1) lentilles des liseuses du plafonnier en matière de glace (`M_GLACE`) → plastique givré diffusant ; (2) commentaires « 4 places » (5 vues) dans `vehicule_cockpit_vitrine.h` et `desktop_main.cpp`, « carbone verni (0,3) » sur la ligne de `M_ACCENT` ; (3) `M_AMBIANCE_NUIT` créé aussi sans lumière d'ambiance (sans effet) ; (4) 22 000 à 29 000 triangles ajoutés, en partie invisibles en vue Cockpit (console, bas des portières, molettes à 120 pas) : à surveiller aux bancs du matin et sur le téléphone (Basse ≥ 30 img/s), alléger si besoin ; (5) RG-04 (photo de référence) impossible avec `regles-reseau.md` : formes au statut E, à confirmer sur photo : aérateurs du milieu de la 911 et de la SF90, commande d'éclairage de la M4.
- Outils réutilisables pour les lots suivants : banc `cockpit-vitrine` (intérieur seul, 5 vues), `PiecesDetails` / `detailsPropres` (une fonction par voiture dans `vehicule_cockpit_details.cpp`).
- Un seul sujet par session : cette session ne demande plus de ticket.

### 2026-11-12 10:10:48 — Statuts automatiques, bancs du matin #22 (robot statuts)
- Run vert (schedule, commit fb4f158) : WFS-451 ✅ (PR #363, df46136), WFS-418 ✅ (PR #353, 08bb76c), WFS-350 ✅ (PR #301, 4180a6c), WFS-351 ✅ (PR #306, 788c286), WFS-352 ✅ (PR #303, ad54bde), WFS-339 ✅ (PR #299, 39a3de7), WFS-348 ✅ (PR #300, 48c423e), WFS-340 ✅ (PR #292, 0708e60), WFS-193 ✅ (PR #308, 13c7cdb), WFS-359 ✅ (PR #320, 649c7c8), WFS-406 ✅ (PR #338, 1c39d69), WFS-396 ✅ (PR #325, 595ae0b), WFS-362 ✅ (PR #323, f89273f), WFS-363 ✅ (PR #333, 6cf133a), WFS-364 ✅ (PR #351, ea80aea), WFS-365 ✅ (PR #355, b24a9c4), WFS-384 ✅ (PR #331, 573ecff), WFS-392 ✅ (PR #362, 4e3320a), WFS-412 ✅ (PR #349, ae7089e), WFS-427 ✅ (PR #352, db45e5b), WFS-377 ✅ (PR #360, d58df71), WFS-428 ✅ (PR #357, 77a35b6), WFS-429 ✅ (PR #356, 9187bd9), WFS-358 ✅ (PR #319, 6d8f7e4), WFS-214 ✅ (PR #314, 44c57ce), WFS-307 ✅ (PR #294, e87bb4c), WFS-343 ✅ (PR #312, feb2d9d), WFS-322 ✅ (PR #289, b47514e).

### 2026-11-12 10:24:28 — Recette QA du matin : 92 lignes de `qa/Check.md` (Agent qa (session_01UY1Vv5i7UtkGveRXL23feQ))
- Base : message « 🧪 Recette QA prête » du 12/11 11:10 (issue #242), bancs du matin #22 (run 43186573506, commit fb4f158), tous verts sauf l'émulateur (information) ; 18 groupes de `captures-matin` (`jour.txt` 11:07), aucun ABSENT. Recette partagée en 8 lots entre agents `qa-lead` (règle « grande recette »), numérotation et dépôt par cette session.
- Verdicts : **81 VALIDÉ**, **8 PASSABLE AVEC ANOMALIES MINEURES** (WFS-334, 338, 359, 362, 364, 323, 382, 404), **3 REFUSÉ** : WFS-239 (ANO-247, voiture à 107 m non dessinée en Basse Poursuite), WFS-351 (ANO-248, l'émulateur appuie sur « Duel en ligne » au lieu de « Course solo » : cause de l'émulateur rouge), WFS-370 (ANO-254, immeubles du fond de la Ville en pavés nus).
- Nouvelle forme nue hors ticket : ANO-259 (disque noir plat sur le hayon de la Golf, origine WFS-404 / WFS-192), bloquante.
- Anomalies créées dans `anno.md` : ANO-247 à ANO-259 (4 bloquantes : 247, 248, 254, 259 → tickets de correction prioritaires à créer par le PO ; 9 mineures). Vérifiées corrigées : ANO-134, 244, 245, 246.
- Tickets 🟡 : captures conformes, le passage ✅ reste à l'utilisateur sur l'APK de `main`. Beaucoup de critères « non vérifiables » : bancs propres aux tickets absents du matin (`marina`, `neige`, `cockpit-voisine`, `nez-*`, `hud`…), voitures retirées (X5, A 45) encore demandées par le banc `vitrines` (ANO-257) ; à voir avec l'expert CI / CD.
- Rapports : `qa/rapports/WFS-XX.md` (90 fichiers).

### 2026-11-12 10:30:39 — Recette du matin du 2026-11-12 : tickets de correction créés (po-scrum-master)
- **Entrée** : recette du QA (commits e556750, 3684bdb) : 92 lignes de `qa/Check.md`, 81 VALIDÉ, 8 PASSABLE (WFS-334, 338, 359, 362, 364, 323, 382, 404), 3 REFUSÉ (WFS-239, 351, 370) ; ANO-247 à 259.
- **Bloquantes → tickets P1 prêts** (file B, en alternance avec l'ordre des 20, chacun à une **nouvelle session** du bon type, jamais à la session du ticket refusé) :
  - **WFS-453** (E4, `@dev`, [TECHNIQUE]) ← ANO-247 : voiture du trafic à 107 m non dessinée en Basse (régression après WFS-239) ; bisection, contrôle du banc `qualite` non affaibli.
  - **WFS-454** (E7, `@cicd`, [TECHNIQUE]) ← ANO-248 : `tools/qa/emulateur.sh` appuie sur « Duel en ligne » ; correction d'un rouge des bancs du matin, permise malgré le gel. WFS-287 (même fichier) après lui.
  - **WFS-455** (E15, `@devartiste` + avis `@3d`) ← ANO-254 : immeubles du fond de la Ville en pavés gris nus vus de haut. Ticket séparé de WFS-385 (trop gros pour une bloquante) ; WFS-385 passe « après WFS-455 ».
  - **WFS-456** (E10, `@devartiste`) ← ANO-259 (disque noir du hayon de la Golf, contrôle Corsa et toutes les voitures), avec ANO-258 (liseré rouge des phares de la Golf) et ANO-257 (banc `vitrines` qui demande encore A 45, Elantra, X5), mêmes captures et outils.
- **Mineures → lots P3** : **WFS-457** (E10, après 456) ← ANO-251, 252, 253 (Brezza, Corvette, M4, S3) ; **WFS-458** (E2 + E10, prêt) ← ANO-249, 256 (gouttes sous le pare-brise, compte-tours de la Corsa) ; **WFS-459** (E3 + E13, après WFS-452, même `poursuite_system.cpp`) ← ANO-250, 255 (bandes POLICE, reflet des phares sur route mouillée).
- `anno.md` : ANO-247 à 259 passées **📋 Planifiée** avec leur US. Règles : `regles-de-gestion/WFS-453.md` à `WFS-459.md`.
- **Note du QA gardée** : les bancs du matin ne lancent pas plusieurs bancs propres aux tickets (marina, neige, cockpit-voisine, nez-*, hud, campagne-objets, brume, mer, halos…), d'où des critères « non vérifiables ». À proposer à l'agent expert ci cd **quand le gel de la CI le permet** (pas de ticket créé aujourd'hui : gel en cours). Les tickets 🟡 restent à valider par l'utilisateur sur l'APK de `main`.
- `track-agent.md` remis à jour (session 01Us6uCbUS enlevée, WFS-383 : envoi à 08:26). Robot des archives : pas de commit cette nuit, ménage à surveiller au prochain passage.

### 2026-11-12 10:54:37 — Archives automatiques (robot archives)
- Journal : 159 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-10).
- TACHES.md : 32 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md` (WFS-451, WFS-422, WFS-424, WFS-418, WFS-449, WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-406, WFS-396, WFS-362, WFS-363, WFS-364, WFS-365, WFS-384, WFS-392, WFS-412, WFS-427, WFS-445, WFS-377, WFS-428, WFS-429, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322).

### 2026-11-12 11:08:40 — Vidéo de l'utilisateur (12:04, Corniche, Poursuite) : ANO-250 confirmée, ANO-260 à 262 (Agent qa (session_01UY1Vv5i7UtkGveRXL23feQ))
- 62 images extraites à 3 img/s (21 s). **ANO-250 reclassée bloquante** : bande « POLICE » côté gauche en rectangle plat qui dépasse de la caisse, aucune bande côté droit (bancs PC : ni l'une ni l'autre visibles).
- **ANO-260** (majeure) : la voiture de police passe de gris argent à blanc en 0,2 s. **ANO-261** (bloquante, conduite) : glisse irréaliste au contact de la police. **ANO-262** (bloquante, à préciser) : citadine à 300 km/h, non visible dans la vidéo (fiches : 205 à 208 km/h).
- Le décor plus pauvre des bancs vient de la Ville de secours (« paquet absent » aux journaux), pas d'un retrait. Preuves : `qa/retours/2026-11-12-video1204-*`, `qa/retours/2026-11-12-police-bandes-banc.png`. Transmis au PO.

### 2026-11-12 11:11:24 — Vidéo de l'utilisateur (Poursuite) : WFS-460 et WFS-461 créés, ANO-250 sortie de WFS-459 (po-scrum-master)
- Constats du QA (11:08:40) pris en compte. **ANO-250** (bloquante, forme nue) et **ANO-260** (majeure, peinture argent / blanc) → **WFS-460** (P1, E3 + E10, nouvelle session `@dev`, ⏳ Prêt) : bandes « POLICE » sur la tôle des deux côtés, une seule peinture blanche à toutes les distances. Exception PO : en parallèle de WFS-452 (même fichier, lignes différentes : roues pour 452, bandes et peinture pour 460).
- **ANO-261** (bloquante, glisse au contact de la police) et **ANO-262** (bloquante, citadine à 300 km/h, à reproduire d'abord) → **WFS-461** (P1, E3 `player.*` / `roues.*` / `duel.*`, skill `expert-conduite`, nouvelle session `@conduite`, ⏳ Prêt).
- **WFS-459** ne garde que ANO-255 (reflet des phares, E13, P3) et ne dépend plus de WFS-452 : ⏳ Prêt.
- Après fusion : WFS-460 et WFS-461 → **🟡 À valider (utilisateur)**. Règles : `regles-de-gestion/WFS-460.md`, `WFS-461.md`. `anno.md`, `TACHES.md`, `track-agent.md` à jour. Aucun ticket attribué (attribution seulement à la demande d'une nouvelle session).

### 2026-11-12 11:55:25 — ANO-262 précisée : c'est la police qui roule vers 300 km/h (Agent qa (session_01UY1Vv5i7UtkGveRXL23feQ))
- Précision de l'utilisateur : les « 300 km/h » sont ceux de la voiture de police. Code lu : `poursuite.cpp` `avancer()` plafonne la police à MAX_SPEED × 1,45 ≈ 377 km/h et lui permet la vitesse du joueur + 162 km/h ; la 208 de police est donnée à 208 km/h dans `cars.h`. Ligne ANO-262 réécrite ; à corriger dans WFS-461.

### 2026-11-12 11:56:54 — ANO-263 : roues de la voiture de police vues par l'utilisateur (Agent qa (session_01UY1Vv5i7UtkGveRXL23feQ))
- Vidéo 12:04 : pneu et jante en une masse noire, plis de tôle autour des passages de roue, roues qui ne tournent pas quand la police glisse ; rattachée à WFS-452 (roues de la police), à confirmer par le PO.

### 2026-11-12 11:57:49 — ANO-264 : roues de la M4 (vidéo de l'utilisateur du 2026-11-12 00:11) (Agent qa (session_01UY1Vv5i7UtkGveRXL23feQ))
- 40 images à 3 img/s : anneau gris clair dans les passages de roue de la M4 rouge, pneu sans flanc ; à rattacher par le PO (WFS-457 ou WFS-452).

### 2026-11-12 11:58:24 — WFS-383 fusionné par le lead dev (lead-dev)
- PR #358 relue sur 163f2bb8 (synchronisée avec `main`) : « Avis agent 3D : VALIDÉ avec réserves » présent ; 27 captures ouvertes une à une (avant / après, jour et nuit) ; tout le diff lu (`sols.glsl`, `sols.{h,cpp}`, `plage.cpp`, `decor_carte.*`, `circuit.cpp`, `carte_donnees*`, `vegetation.*`, `game.cpp`, `scene.frag`, `test_sols.cpp`, `CMakeLists.txt`) ; construction et **112 tests CPU verts** refaits par le lead dev.
- « ✅ Approuvé » en commentaire (même compte GitHub), fusion squash **150b1997**. Statut **🟡 À valider (utilisateur)** : la règle « Corniche ≤ 33 ms sur le S21 FE » n'est pas mesurable sur le PC (lavapipe : +2,5 %, +5,7 k triangles en Basse) ; si la Corniche dépasse 33 ms sur le téléphone, le PO rouvre.
- **Pour le PO (tickets séparés à créer)** : R1 dalles de calcaire trop régulières vues de haut ; R2 pins d'Alep (vraie carte d'aiguilles au lieu de la case 3 de l'atlas, houppier plus garni, dégradé d'écorce à hauteur variable ; RG-03 modèle CC0 / CC BY et instances toujours visés) ; R3 pré des champs de la Campagne encore vert vif uni (`carte-campagne-vaches`) ; R4 aplat du fond de l'épingle de la Campagne (maillage). Remarque : normales lissées par morceau (jointure possible entre deux morceaux, rien de visible).

### 2026-11-12 11:59:08 — WFS-383 : fin de session après la fusion (Claude, agent dev3d (01UY1Vv1zt))
- Fait : 2e avis de l'agent 3D (A à D) corrigé (79efa3ba : calcaire en dalles de Voronoï, part de roche par sommet, talus caillouteux, générateur `veg::pinAlep`) ; captures série 4 et paquet refaites ; « Avis agent 3D : VALIDÉ avec réserves » sur 163f2bb8 ; fusion par le lead dev en squash (#358, 150b1997).
- Vérification : 112 tests CPU verts ; banc perf PC (lavapipe) série 4, Corniche Basse 235,4 → 241,2 ms (+2,5 %), +5,7 k triangles.
- Limites : Corniche ≤ 33 ms sur le S21 FE non mesurée (🟡, à l'utilisateur) ; réserves R1 à R4 de l'agent 3D à transformer en tickets par le PO.
- Session terminée (un seul sujet par session) : aucun autre ticket demandé.

### 2026-11-12 11:59:29 — ANO-263 et ANO-264 rattachées (po-scrum-master)
- **ANO-263** (roues de la 208 de police : pneu et jante en une masse noire, plis de tôle, hauteur de caisse) → **WFS-452** (P1, même sujet : roues de la police), RG-08 et critère 3 ajoutés, zone élargie au modèle de la 208 ; 🟡 À valider (utilisateur) après fusion.
- **ANO-264** (passages de roue gris clair et pneus sans flanc de la M4) → **WFS-457** (retouches M4, `dev-artiste-3d`), RG-05 et RG-06 (contrôle des 17 voitures, de nuit) ; priorité P3 → **P2** (vue par l'utilisateur) ; 🟡 après fusion ; toujours après WFS-456.
- Anomalies en 📋 Planifiée dans `anno.md` ; aucune attribution (tickets donnés à la demande d'une nouvelle session).

### 2026-11-12 12:00:03 — ANO-265 : roues avant du trafic qui braquent en ligne droite (vidéo de l'utilisateur 00:11) (Agent qa (session_01UY1Vv5i7UtkGveRXL23feQ))
- Précision de l'utilisateur sur la M4 rouge : ce sont les roues avant qui « jouent » alors que la voiture est droite ; confirmé à 6 img/s ; à rattacher par le PO (WFS-452).

### 2026-11-12 12:00:32 — ANO-265 rattachée à WFS-452 (po-scrum-master)
- **ANO-265** (roues avant du trafic qui braquent puis se redressent en ligne droite, M4 rouge) → **WFS-452** (P1, roues des autres voitures) : RG-09 (braquage du trafic lissé, 0 en ligne droite, piste `roues.h` l. 45), critère 4, zone élargie à `roues.h` et `traffic_system.cpp` pour ce point ; 🟡 À valider (utilisateur) après fusion. Anomalie en 📋 Planifiée.

### 2026-11-12 12:01:30 — Gravités fixées par l'utilisateur : ANO-265 bloquante, ANO-263 et ANO-264 mineures (Agent qa (session_01UY1Vv5i7UtkGveRXL23feQ))
- Seule la roue avant de la M4 rouge qui « joue » en ligne droite est bloquante ; les autres défauts de roues sont des anomalies mineures. Transmis au PO pour les priorités (WFS-452, WFS-457).

### 2026-11-12 12:02:12 — Gravités des roues fixées par l'utilisateur : priorités remises d'accord (po-scrum-master)
- **ANO-265** (bloquante) reste dans **WFS-452** (P1, RG-09). **ANO-263** (mineure) sort de WFS-452 (RG-08 retirée, la rotation des roues de la police y reste) et va dans **WFS-457** (RG-07). **WFS-457** repasse **P3** (ANO-251 à 253, 263, 264 ; RG-05 à RG-07), toujours après WFS-456, 🟡 après fusion. `anno.md` (colonne US de correction) et `TACHES.md` à jour.

### 2026-11-12 12:02:49 — WFS-452 bloquant en entier (précision de l'utilisateur) (po-scrum-master)
- Le cœur de **WFS-452** (roues de la Course à 5, du duel, de la police et du fantôme qui tournent et braquent) est **bloquant** : P1 et périmètre de départ gardés, plus RG-09 (ANO-265, bloquante). **ANO-263** coupée en deux : roues immobiles pendant la glisse → WFS-452 (bloquant, RG-08 et critère 3) ; aspect (pneu et jante, tôle, hauteur) → WFS-457 RG-07 (mineur). WFS-457 reste P3 (ANO-264 mineure). `anno.md`, `TACHES.md`, règles à jour.

### 2026-11-12 12:04:24 — WFS-461 : les 300 km/h sont ceux de la police (précision de l'utilisateur) (po-scrum-master)
- Titre et règles réécrits : RG-03 police plafonnée à la vitesse, à l'accélération et au freinage de sa fiche (208, 208 km/h ; fin de `MAX_SPEED × 1,45` et du « joueur + 45 m/s » dans `poursuite.cpp` / `poursuite.h`, replacement après perte compris) ; RG-04 jeu jouable (rattrapage par les virages, bouchons, renforts). Zone élargie à `src/game/poursuite.cpp` / `.h`. Critères : banc CPU de vitesse maxi de la police ≤ 208 km/h, suite d'images d'un rattrapage. RG-01 / RG-02 (ANO-261) gardées. P1, 🟡 après fusion, toujours `@conduite` ; ANO-262 à jour dans `anno.md`.

### 2026-11-12 12:05:35 — Synchronisation : 10 tickets passent ⏳ Prêt (po-scrum-master)
- Dépendances fusionnées (🟣 / 🟡) : WFS-371 (380, 383), 395 (359, 349, 390), 366 et 444 (365), 397 (404, 396), 394 (359), 410 et 411 (402), 403 (373), 344 (390).
- `track-agent.md` : ligne de la session 01UY1Vv1zt (WFS-383 fusionné, #358) enlevée ; aucun ticket 🔧 / 🔀 en cours.
- Nouvelle liste des 30 prochaines tâches donnée à l'utilisateur (file B des bloquantes WFS-452 à 461 en alternance avec l'ordre des 20).

### 2026-11-12 12:33:54 — WFS-452 attribué à Claude, agent dev3d (01Us6uCwQm) (po-scrum-master)
- Demande : nouvelle session `@dev` (session_01Us6uCwQmu1MxX3i7ZDXCEk, agent `dev-artiste-3d`), aucun ticket reçu avant : règle « un seul sujet » respectée.
- Choix : file B des bloquantes (la dernière attribution, WFS-325 lot 1, venait de l'ordre des 20) ; **WFS-452** (P1, bloquant en entier selon l'utilisateur), plus petit numéro des P1 prêts permis à `@dev` (453, 455, 456, 460 après lui). E3 et E10 libres (aucun 🔧 / 🔀).
- Zone : appels de dessin et pose des roues de `course5_system.cpp`, `duel_system.cpp`, `poursuite_system.cpp`, `course_system.cpp` ; `roues.*` ; `traffic_system.cpp` pour RG-09 seulement ; `tests/**`. WFS-460 (bandes et peinture de la police, même `poursuite_system.cpp`) peut partir en parallèle : lignes différentes, synchroniser souvent.
- Règles : `regles-de-gestion/WFS-452.md` (RG-01 à RG-09). Après fusion : 🟡 À valider (utilisateur). Branche `feature/waforspeed-452`, premier commit poussé tout de suite.


### 2026-11-12 12:55:45 — WFS-462 créé : publier les paquets de la Corniche (po-scrum-master)
- Capture de l'utilisateur (choix du terrain, Corniche sans « Télécharger ») : vérifié sur GitHub, dernier run de `paquets.yml` = #2 (2026-11-11 09:08 UTC, avant la fusion de WFS-426 / #350) ; Release `paquets` sans `plage_*.wfcarte`.
- **WFS-462** (P1, [TECHNIQUE], E14, `@cicd`) créé à la demande de l'utilisateur ; règles : `regles-de-gestion/WFS-462.md` (la demande vaut accord pour lancer `paquets.yml` une fois sur `main`). Premier ticket pour la prochaine session `@cicd`, avant WFS-454. WFS-426 reste 🟡.

### 2026-11-12 12:56:48 — Ordre permanent : une tâche `@cicd` de publication des paquets après chaque fonctionnalité de décor (po-scrum-master)
- Ordre de l'utilisateur : « créer après chaque fonctionnalité qui touche le décor des cartes une tâche pour un cicd ». Inscrit dans les ordres permanents du PO (skill `po-scrum-master`) et en bandeau de `TACHES.md`.
- À chaque fusion d'un ticket qui change le contenu d'une carte, le PO crée aussitôt `[TECHNIQUE] Publier les paquets après WFS-XX` (P1, `@cicd`, modèle `regles-de-gestion/WFS-462.md`).
- WFS-462 couvre déjà les changements de décor fusionnés depuis le dernier run de `paquets.yml` (2026-11-11 09:08 UTC) : WFS-426, 383, 402, 370, 372, 373, 416, 380.

### 2026-11-12 12:58:02 — Ordre permanent précisé par l'utilisateur : la tâche `@cicd` se rédige avec la tâche de décor (po-scrum-master)
- Précision de l'utilisateur : la tâche de publication des paquets est rédigée **par le PO au moment où il rédige** la tâche qui touche le décor des cartes (tâche compagne, dépend du ticket de décor), pas après coup. Skill PO, bandeau de `TACHES.md` corrigés.
- Correction du PO : la liste de WFS-462 citait à tort 402, 370, 372, 373, 416, 380 (fusionnés avant le dernier run de `paquets.yml`) ; corrigée en WFS-426, 383, 392.

### 2026-11-12 13:04:15 — 15 tâches compagnes `@cicd` de publication des paquets (po-scrum-master)
- Demande de l'utilisateur (« je vois pas les tâches cicd après les US de décor ») : WFS-463 à 477 créées, une par tâche de décor ouverte (371, 455, 387, 401, 400, 385, 398, 399, 410, 411, 403, 388, 389, 386, 417), P1, `@cicd`, ⏳ En attente jusqu'à la fusion de leur tâche de décor ; règles `regles-de-gestion/WFS-463.md` à `WFS-477.md` (modèle WFS-462).

### 2026-11-12 13:05:59 — WFS-462 retiré, fusionné dans WFS-463 (po-scrum-master)
- Ordre de l'utilisateur : « enlève la première tâche pour cicd, la tâche 3 suffit ». WFS-462 ↪ fusionné dans WFS-463 : un seul run de `paquets.yml` après la fusion de WFS-371 publie la Corniche cuite (WFS-426), WFS-383, 392 et 371. Prochaine tâche `@cicd` prête : WFS-454.

### 2026-11-12 13:09:04 — WFS-371 attribué à Claude, agent dev3d (01UY1Vv2rp) (po-scrum-master)
- Demande : nouvelle session `@devartiste` (session_01UY1Vv2rpnG6QPHQ9jg9MnC, agent `dev-artiste-3d`), aucun ticket reçu avant : règle « un seul sujet » respectée.
- Choix : alternance des files ; la dernière attribution (WFS-452) venait de la file B des bloquantes, donc cette fois l'**ordre des 20** : **WFS-371** (11e, P1, `@devartiste` + avis `@3d`), premier encore prêt (WFS-383 fusionné). Sa fusion libère **WFS-463** (publication des paquets de la Corniche, demande de l'utilisateur). E15 libre (aucun 🔧 / 🔀). Les P1 de la file B (455, 456, 460) restent aux prochaines sessions.
- Zone : `src/world/plage.cpp`, `portique.cpp`, `enseignes.*` et le décor Corniche (E15, + E12, E2 selon les règles), `assets/world/modeles/**`, `assets/licences/`, outils de cuisson du paquet pour ces modèles seulement, `tests/**`. PR de l'épic E15 : avis agent 3D obligatoire ; pas de fusion si la Corniche dépasse ≈ 33 ms sur le S21 FE.
- Règles : `regles-de-gestion/WFS-371.md` (RG-01 à RG-07 + ajouts D4, R4, R5 ; section Suivi). Après fusion : 🟡 À valider (utilisateur). Branche `feature/waforspeed-371`, premier commit poussé tout de suite.

### 2026-11-12 13:37:19 — WFS-452 : arbitrage police en Basse, décision A ; WFS-478 créé (po-scrum-master)
- Demande de Claude, agent dev3d (01Us6uCwQm) : en Basse, la police dessinée en lots a ses roues fondues dans la carrosserie, RG-08 non tenue.
- **Décision A** : lots seulement au-delà de 15 m ; de près, dessin pièce par pièce (roues qui tournent et braquent). Coût accepté : +37 appels de dessin et +85 000 triangles par police à moins de 15 m en Basse, chiffré dans la PR. Écrit dans `regles-de-gestion/WFS-452.md` (Suivi).
- **WFS-478** créé (E4, P2, ⏳ En attente après la fusion de WFS-452, nouvelle session `@dev`) : option B, roues gardées en pièces séparées dans les lots, pour supprimer ce surcoût. Note ajoutée à WFS-457 (jante visible de près en Basse, ANO-263 en partie).

### 2026-11-12 13:46:31 — WFS-371 : inventaire des formes nues de la plage de la Corniche (RG-01) (Claude, agent dev3d (01UY1Vv2rp))
- Branche `feature/waforspeed-371` (premier envoi 13:1x UTC). Inventaire complet : `qa/pr/WFS-371/inventaire.md` (sur la branche) ; nombres lus au journal du banc (`WFS_BANC=corniche`).
- Remplacés par des modèles au niveau du vrai objet (`src/world/decor_corniche_modeles.*`, méthode de WFS-372 / 373 / 376) : 57 parasols (cône Ø 3 m → Ø 1,80 m à 8 baleines) et 57 transats (boîte → bain de soleil 1,90 × 0,65 m) ; 3 bars de plage (pieux, poteaux, comptoir, étagère, tabourets, tables, poufs, bouteilles, parasols de paille, ampoules) ; 2 postes de secours (boîtes) ; 134 glissières et la glissière de la falaise (R5 : lisse double onde 310 × 83 mm en volume, poteaux C 100 tous les 4 m) ; 136 rochers (boules → blocs de calcaire du rivage) ; 28 arbres ronds (→ pins d'Alep de WFS-383) ; 2 scènes (D4 : praticables, portique treillis 29 cm, fond tendu, DJ, enceintes, lyres) ; guirlandes et feux de camp ; 74 lampadaires (R4 : candélabres de style à lanterne vitrée, lumière inchangée à 5,6 m) ; 183 couronnes de palmiers (gaines et noix de coco).
- D4 sable : couleur 0xD8C8A0 et laisse de mer (posidonies) ; les traces dans le sable demandent une texture : reste à faire (signalé dans la PR).
- Hors ticket, remis au PO dans l'inventaire : bateaux au large et barques du village, jeux de plage (volley, pédalos, toboggan, château de sable), scooters, façades du casino / café / galerie, feu tricolore, ancre, luminaires du tunnel, faisceaux et flamme (effets de lumière).
- Le paquet `.wfcarte` de la Corniche (WFS-426) ne cuit que les villas du kit : ces objets sont construits par le code au chargement (blocs préparés WFS-268), avec ou sans paquet.

### 2026-11-12 13:54:00 — WFS-452 : PR #365 ouverte (Claude, agent dev3d (01Us6uCwQm))
- Fait : braquage des roues avant calculé d'après le vrai mouvement (`roues::SuiviRoues` : atan(empattement × courbure de la trajectoire) + écart mouvement / caisse, ±35°, lissé, 1,5 rad/s au plus) pour la Course à 5, le duel, la police, le fantôme et le trafic (RG-09, ANO-265) ; roulage = −distance / rayon pour les 4 premiers (avant : roues figées). Décision A du PO : police en Basse dessinée sans lots à moins de 15 m (les lots n'ont pas de roues séparées) ; WFS-478 créé par le PO.
- Cause d'ANO-265 mesurée : braquage du trafic = cap de file × 1,6, jusqu'à 2,5 rad (voitures qui traversent le carrefour, voitures lentes qui changent de file) et en sauts.
- Fichiers : `src/game/roues.*`, `course5_system.cpp`, `duel_system.cpp`, `poursuite_system.cpp`, `course_system.cpp`, `traffic_system.cpp`, banc `roues-suivies` (`roues_banc.*`), `tests/test_roues_suivies.cpp`. **Accroches annoncées** : 1 ligne dans `src/platform/desktop_main.cpp` (groupe de banc `roues-suivies`) et 2 lignes dans `CMakeLists.txt` (nouveau test).
- Vérifié : 113 / 113 tests CPU ; banc `roues-suivies` 0 écart ; appels de dessin identiques sauf police-basse (116 → 153, accepté par le PO) ; 120 captures + 6 zooms dans `qa/pr/WFS-452/`.
- Limites : non mesuré sur le téléphone ; duel sans scène au banc (même code que la Course à 5).

### 2026-11-12 14:08:31 — WFS-452 : PR #365 refusée par le lead dev (lead-dev)
- Relu sur 0a2b88d1 : 6 zooms et les suites avant / après ouverts ; tout le diff lu ; construction refaite et `bash tools/qa/tests_cpu.sh build` : **113 test(s) réussi(s)** (branche fusionnée localement avec `main`).
- Bon : roues qui tournent (Course à 5 jour et nuit, police Haute et Basse, fantôme), braquage de la police pendant la glisse, décision A du PO appliquée, calcul `SuiviRoues` juste.
- **Refus, 2 points** : (1) capture « Tests CPU » et `qa/pr/WFS-452/tests-cpu.log` absentes (règle du 2026-11-11) ; (2) RG-09 / ANO-265 non prouvée à l'image : la voiture du trafic est vue de face, roues avant cachées ; il faut une suite où les roues avant du trafic se voient (ligne droite puis changement de file, avant / après).
- Remarques non bloquantes dans la revue (reprise du braquage après un saut, commentaire de `g_avant452`, cap du fantôme, `suivi_` du duel).
- **Pour le PO** : halo du clignotant du trafic en disque orange flou (code existant WFS-67 / ANO-48), forme nue visible sur les captures du trafic : à rattacher à une US de correction.

### 2026-11-12 14:19:22 — WFS-371 : PR #366 ouverte (Claude, agent dev3d (01UY1Vv2rp))
- Fait : 21 modèles au niveau du vrai objet (`src/world/decor_corniche_modeles.*`), branchés dans `plage.cpp` (inventaire RG-01 au journal précédent) ; D4 (scène, sable 0xD8C8A0, laisse de mer), R4 (candélabres à lanterne), R5 (glissière double onde en volume).
- Vérifié : `test_decor_corniche` et 113 tests CPU verts ; bancs `corniche` avant / après et `corniche-objets` (14 gros plans) regardés ; perf Corniche nuit (lavapipe) : Basse 318,6 → 312,9 ms, Haute 508,7 → 481,0 ms, triangles +27 %, 0 création en course.
- Limites : traces dans le sable non faites (texture) ; formes nues hors ticket listées pour le PO (`qa/pr/WFS-371/inventaire.md`) ; mesure du S21 FE à faire sur l'APK de `main`.
- Suite : avis de l'agent 3D (E15), puis lead dev.

### 2026-11-12 14:30:05 — WFS-371 fusionné par le lead dev (lead-dev)
- PR #366 relue sur d5b4d469 (prise dans `suivi-pr.md` à 14:21) : « Avis agent 3D : VALIDÉ avec réserves » présent sur ce commit (E15) ; 48 captures ouvertes, diff complet lu (`decor_corniche_modeles.{h,cpp}`, `plage.cpp`, test, `CMakeLists.txt`) ; construction et `tests_cpu.sh` refaits : 113 réussis, code 0.
- GitHub refuse l'approbation (même compte) : revue « ✅ Approuvé par le lead dev » postée en commentaire, puis fusion en squash : **9710670** `WFS-371 : Corniche, objets de la plage en vrais modèles (#366) (Claude, agent dev3d (01UY1Vv2rp))`.
- Statut : 🟡 À valider (utilisateur) ; **Corniche ≤ 33 ms sur le S21 FE à mesurer** (+27 % de triangles, lavapipe Basse 318,6 → 312,9 ms). Ligne #366 retirée de `suivi-pr.md` ; ligne ajoutée à `qa/Check.md`.
- Pour le PO : réserves R-1 à R-5 de l'agent 3D et formes nues hors ticket (`qa/pr/WFS-371/inventaire.md`) à mettre en tâches séparées ; WFS-463 (publication des paquets, `@cicd`) peut passer ⏳ Prêt.

### 2026-11-12 14:30:51 — WFS-371 : fusionné, session terminée (Claude, agent dev3d (01UY1Vv2rp))
- PR #366 fusionnée par le lead dev (9710670), avis agent 3D « VALIDÉ avec réserves » sur d5b4d469 ; ticket 🟡 À valider (utilisateur).
- Pour le PO (tâches séparées, non faites ici) : réserves de l'agent 3D R-1 à R-5 (rochers et galets trop sombres et facettés, cendres du feu, lanterne qui entre dans un palmier : écart ≥ 1,5 m, bout de glissière abaissé et enterré, poufs trop cylindriques) ; traces dans le sable (D4, texture) ; formes nues hors ticket de `qa/pr/WFS-371/inventaire.md` ; WFS-463 (publication des paquets) peut passer prêt.
- À mesurer par l'utilisateur sur l'APK de `main` : Corniche de nuit en Basse ≤ ≈ 33 ms sur le S21 FE (+27 % de triangles sur la Corniche ; si ça dépasse, alléger d'abord les versions de loin).
- Session terminée (un seul sujet par session).

### 2026-11-12 14:41:07 — WFS-452 : PR #365 corrigée après le refus du lead dev (Claude, agent dev3d (01Us6uCwQm))
- Corrigé : `qa/pr/WFS-452/tests-cpu.log` (114 tests réussis sur d49c948a, synchronisé avec main) et section « Tests CPU » ; scène « trafic » du banc vue de 3/4 avant, basse (roue avant visible : avant, pivot de 0,23 rad d'un coup à l'image 06 ; après, 0,010 → 0,029 rad progressif) ; remarques : roues droites après une réapparition, abscisse continue (relZ) pour le trafic, fantôme sur fantYaw_, duel vidé à chaque manche.
- Réponses dans la PR (« Bien reçu », puis « corrigé dans »). Lead dev rappelé sur f016dc8d.

### 2026-11-12 14:53:04 — WFS-452 fusionné par le lead dev (lead-dev)
- PR #365, 2e relecture sur f016dc8d : les 2 points du refus sont corrigés (tests CPU, roue avant du trafic visible avant / après). Captures ouvertes, diff lu, 114 tests CPU et banc `roues-suivies` (trafic, 0 écart) refaits. « ✅ Approuvé » écrit en commentaire (GitHub refuse l'approbation : même compte), revue 5876526550.
- Fusion en squash : 1164d243. Statut 🟡 À valider (utilisateur). Ligne de `suivi-pr.md` supprimée.
- Pour le PO et le QA : halo orange du clignotant du trafic qui flotte à côté de la voiture (halo en disque, déjà sur `main`) ; remarque de code sur la régulation du trafic (`traffic.cpp:366`, sans effet visible).

### 2026-11-12 14:53:54 — WFS-452 : fin, session terminée (Claude, agent dev3d (01Us6uCwQm))
- PR #365 fusionnée par le lead dev (squash 1164d243) à la 2e revue ; statut 🟡 À valider (utilisateur), ligne dans `qa/Check.md` (lead dev).
- À valider par l'utilisateur sur l'APK de `main` : roues des adversaires (Course à 5, duel), de la police (glisse comprise, aussi en Basse) et du fantôme qui tournent et braquent ; roues avant du trafic droites en ligne droite (ANO-265).
- **Pour le PO** (remarques du lead dev, hors ticket) : (1) `traffic_system.cpp:377-378` : une voiture recyclée par la régulation du peloton (`traffic.cpp:366`, `c.z = c.zPrec = z`) ne saute pas de > 20 m pour `SuiviRoues` : elle garde un instant le braquage de son ancienne place (invisible : replacée à plus de 800 m, revient à 0 en moins d'1 s) ; à corriger en remettant les roues à zéro au recyclage (petit ticket E3) ; (2) halo orange du clignotant du trafic en disque flou (code existant WFS-67 / ANO-48) : forme nue à créer en anomalie. Option B de la police en lots : WFS-478 (déjà créé).
- Session terminée : un seul sujet, aucun autre ticket demandé.

### 2026-11-12 15:32:00 — WFS-463 prêt puis attribué à agent expert ci cd (01UY1Vv3nf) (po-scrum-master)
- WFS-371 fusionné (#366, squash 9710670) : WFS-463 passe ⏳ Prêt, puis 🔧 attribué à **agent expert ci cd (01UY1Vv3nf)** (session_01UY1Vv3nf2w9GSf6R8UWb3U), nouvelle session `@cicd`, à sa demande ; première tâche de cette session, un seul sujet.
- Pourquoi lui plutôt que WFS-454 (P1 aussi) : objectif de l'utilisateur « cartes téléchargées » (file A, avant les bloquantes à égalité) ; il reprend WFS-462, mis en tête de la liste `@cicd` avant WFS-454 ; la Corniche (WFS-426) n'est toujours pas publiée.
- Épic E14 (exception PO : aucun fichier de code ni de CI modifié). Règles : `regles-de-gestion/WFS-463.md` + `regles-de-gestion/WFS-462.md` (RG-01 à RG-06). Branche `feature/waforspeed-463`. Plan : un lancement de « Paquets de cartes » sur `main` (un seul run à la fois), vérifier la Release `paquets` (`plage_astc`, `plage_etc2`, `plage_rgba8`, cartes touchées par WFS-371, 383, 392) et `cartes.json` (taille, SHA-256, version min.), message de test à l'utilisateur dans `tache utilisateur.md`. Après : 🟡 avec WFS-371 et WFS-426.
- Précision de la demande : l'agent n'agit (commit, PR, lancement de run) qu'après la validation explicite de l'utilisateur dans sa session ; le PO n'a lancé aucun run.
- WFS-454 reste ⏳ Prêt pour la prochaine nouvelle session `@cicd`. Lignes des sessions closes (WFS-452, WFS-371) enlevées de `track-agent.md`.

### 2026-11-12 15:37:00 — WFS-455 attribué à Claude, agent dev3d (01UY1Vv3N7) (po-scrum-master)
- Nouvelle session `@devartiste` (session_01UY1Vv3N7QgGeDnA3Mw5Ajg), jamais de ticket auparavant : **WFS-455** (P1, E15, ANO-254 : immeubles du fond de la Ville en pavés gris nus vus de haut). Aucun ticket E15 actif (WFS-371 fusionné).
- Pourquoi lui plutôt que WFS-456 (P1 aussi) : plus petit numéro à priorité égale ; forme nue sur une carte (règle prioritaire du 2026-11-09) ; débloque WFS-385 et WFS-464 (publication des paquets). WFS-456 reste ⏳ Prêt pour la prochaine session `@devartiste`.
- Zone : `src/world/archi_*`, `src/world/ville*.cpp`, `assets/textures/decor/ville/**`, `tests/**`. Règles : `regles-de-gestion/WFS-455.md` (RG-01 à RG-05) + `regles-de-gestion/E15-decors.md`. Branche `feature/waforspeed-455`. Avis « VALIDÉ » de l'agent 3D en commentaire de PR avant la fusion par le lead dev ; après fusion, 🟡 et WFS-464 (`@cicd`) publie les paquets.

### 2026-11-12 15:37:23 — WFS-453 attribué à Claude, agent dev3d (01UY1Vv5g8) (po-scrum-master)
- Nouvelle session `@dev` (= `@dev3d`, session_01UY1Vv5g8BEsBUN32npRLKs), à sa demande ; 1re attribution de cette session, un seul sujet.
- Choix : alternance des files ; les dernières attributions venaient de l'ordre des 20 (WFS-371) puis de l'objectif « cartes téléchargées » (WFS-463), donc la **file B des bloquantes** : P1 prêts permis à `@dev` : 453, 455, 456, 460 ; **WFS-453**, plus petit numéro. E4 libre (aucun 🔧 / 🔀). Pas la session de WFS-239 : respecté.
- Épic E4 ; zone : le fichier qui contient la cause (trafic, rendu des voitures, coupe à distance) + `tests/**` ; `src/render/qualite_banc.cpp` en lecture ; accroche hors épic ≤ 10 lignes annoncée au journal. Pas dans E15 : pas d'avis agent 3D obligatoire. US technique : CI de branche permise (une à la fois, notée au journal).
- Règles : `regles-de-gestion/WFS-453.md` (RG-01 à RG-05). Branche `feature/waforspeed-453`. Plan : banc `qualite` en local, bisection depuis la fusion de WFS-239, correction du dessin (jamais du contrôle du banc), test CPU 107 m / 118 m, captures avant / après, PR, lead dev. Après fusion : 🟣 puis ✅ par les bancs du matin.

### 2026-11-12 18:02:10 — WFS-453 : PR #367 ouverte (Claude, agent dev3d (01UY1Vv5g8))
- Cause : dcd173c9 (WFS-265), coupe des petits objets à 110 m de la caméra en Basse, voitures du trafic comprises ; correction : distance du trafic (130 m de la caméra) pour les seuls dessins du trafic. Banc `qualite` : 0 « ERREUR placement » ; banc `perf` : Ville et Campagne Basse dessinaient 0 voiture du trafic sur 1 choisie, maintenant 1 ; 114 tests CPU verts. Captures dans `qa/pr/WFS-453/`. Lead dev appelé.
- **Pour le PO** : le contrôle WFS-239 RG-03 du banc `qualite` peut se tromper : il a compté « dessinée » la voiture à 120 m de `voiture-distance-120-basse` sur `main` alors qu'elle n'était pas à l'écran (il compare des nombres d'appels de dessin). Non touché (RG-03 de WFS-453) ; à toi de voir s'il faut un ticket.

### 2026-11-12 18:16:33 — WFS-453 : PR #367 refusée par le lead dev (lead-dev)
- Commit relu dd4bf14c. Fond bon : 6 paires de captures ouvertes (voiture à 107 m et à 120 m en Basse maintenant dessinées, Haute et trafic dense identiques, 0 « ERREUR placement ») ; diff lu en entier ; branche fusionnée avec `main` construite en local, 114 tests CPU verts.
- Refus pour 2 points de forme : (1) PR non fusionnable, conflit sur `journal.md` avec 912dda3d ; (2) ligne « 2 bis » de la description et de `qa/pr/WFS-453/pr.md` : images zoom ×8 absentes de la branche. Remarque : remettre `traficSuivant_` à faux en début d'image (renderer.cpp l. 1309). Revue « changements demandés » postée en commentaire (même compte GitHub).
