# Suivi des agents — qui fait quoi

> **Règle de l'utilisateur (2026-11-06)** : ce fichier est **toujours synchronisé** avec `main`.
> - **Chaque agent développeur** met à jour **sa propre ligne** à chaque étape (ticket attribué, branche créée et poussée, PR ouverte,
>   fusion, CI de `main` verte, ticket fini, agent libre) et l'envoie sur `main` **dans le même commit** que `TACHES.md`.
> - **Le PO** remet tout le fichier à jour à chaque appel (attribution, point d'avancement) d'après `TACHES.md` et l'heure du dernier
>   envoi de chaque branche (`git log -1 --format=%ci origin/feature/waforspeed-XX`), et met à jour « Dernière mise à jour : 2026-11-13 00:54:15 UTC (po-scrum-master)
> - Mêmes règles que `TACHES.md` : partir de `main` (`git pull`), ne modifier que sa ligne, ne jamais copier ce fichier d'une autre branche.
> - **Colonne « Ticket en cours »** (règle de l'utilisateur, 2026-11-06) : seulement les tickets 🔧 En cours ou 🔀 En PR ; **vide** si l'agent n'en a
>   aucun (les tickets 🟣 fusionnés ou ✅ terminés n'y figurent pas ; la CI de `main` suivie s'écrit dans « État »).
> - **Nom de l'agent** : la colonne « Agent » donne le nom à mettre à la fin de chaque titre de commit, entre parenthèses (règle de l'utilisateur, 2026-11-06).
> - **Ce fichier suit les agents, pas les tickets** (règle de l'utilisateur, 2026-11-06) : une ligne par agent (ce qu'il fait, sa branche,
>   son dernier envoi, son état). La liste des tickets et leurs statuts restent dans `TACHES.md`.
> - **Après une fusion, l'agent est libre** (règle de l'utilisateur, 2026-11-06 23:20 UTC) : il ne « suit » plus la CI de `main` ; la colonne État ne le dit plus.
> - **Un seul sujet par session (règle de l'utilisateur, 2026-11-09 12:05 UTC)** : chaque session de développeur mène **un seul ticket** jusqu'à la fusion, puis **s'arrête** (État « ⏹️ session terminée ») ; elle ne reprend pas de ticket. Les sessions qui tiennent un ticket à 12:05 UTC le terminent, puis s'arrêtent. Nouveau sujet = nouvelle session, nouvelle ligne.
> - **Agent fini = ligne enlevée** (règle de l'utilisateur, 2026-11-09) : dès qu'un développeur a fusionné son ticket (session terminée), le PO **enlève sa ligne** de ce tableau ; l'historique reste dans `TACHES.md` et `journal.md`.
> - Alerte ⚠️ quand un ticket n'a rien reçu depuis plus de 2 h, ou n'a pas de branche 30 min après l'attribution ; 🔴 pour un ticket P1.


**Dernière mise à jour** : 2026-11-22 12:53:06 UTC (po-scrum-master)

> 🏙️ **12:53 UTC (2026-11-22)** : **WFS-571** (P1, E15 Ville, ANO-280 : tours en boîtes et cubes nus des toits remplacés) attribué à Claude, agent dev3d (01UY1Vv4Xe) (session_01UY1Vv4Xeus56avzEmiAhcZ), nouvelle session `@dev`, à sa demande (597, 593, 594 et 596 déjà pris : 1er prêt de la liste `@dev`). Prochains `@dev` : 573 → 575 → 444.

> 🧭 **12:52 UTC (2026-11-22)** : **WFS-596** (P1, E5 + E10, mini-carte sur l'écran central en vue Cockpit, plus à l'écran) attribué à Claude, agent dev3d (01UY1Vv2sH) (session_01UY1Vv2sHodzrS5FyXVX9uR), nouvelle session `@dev`, à sa demande (597, 593 et 594 déjà pris). Prochains `@dev` : 571 → 573 → 575.

> 🛣️ **12:51 UTC (2026-11-22)** : **WFS-594** (P1, routes et sols réalistes de près) attribué à Claude, agent dev3d (01UY1Vv49f) (session_01UY1Vv49fLTvKRexaJUt2wy), nouvelle session `@dev`, à sa demande (WFS-597 et 593 déjà pris) ; avis `@3d` obligatoire. Prochains `@dev` : 596 → 571 → 573.

> 👥 **12:50 UTC (2026-11-22)** : **WFS-593** (P1, E11, seulement 10 % des personnes dans toutes les cartes) attribué à Claude, agent dev3d (01UY1Vv4Di) (session_01UY1Vv4DiuxbNaC43HNZKKg), nouvelle session `@dev`, à sa demande (WFS-597 déjà pris : 1er prêt de la liste `@dev`). Prochains `@dev` : 594 → 596 → 571 → 573 → 575.

> 🖼️ **12:49 UTC (2026-11-22)** : **WFS-597** (P1, E8, fond de l'écran de chargement : image 2 de l'utilisateur telle quelle) attribué à Claude, agent dev3d (01UY1Vv1zn) (session_01UY1Vv1znETyWL8wzqa2tEb), nouvelle session `@dev`, à sa demande (1er de la liste `@dev`). Prochains `@dev` : 593 → 594 → 596 → 571 → 573 → 575.

> 📋 **11:45 UTC (2026-11-22)** : WFS-590 fusionné (#438), session 01UY1Vv5QX terminée ; WFS-589 pris par 01UY1Vv43t. Liste des 30 prochaines tâches donnée à l'utilisateur ; ouvrir de nouvelles sessions `@dev` (1re : WFS-571), `@conduite` (WFS-584), `@cicd` (WFS-454, en parallèle).

> 🛠️ **11:45 UTC (2026-11-22)** : **WFS-589** (P1, [TECHNIQUE] CI de `main` rouge : licences `ui/modes/*.jpg` de WFS-562, parcours « fin → Menu », realisme-2) attribué à agent expert ci cd (01UY1Vv43t), nouvelle session `@cicd`, à sa demande. Session de WFS-590 terminée (fusionné #438) : ligne enlevée. Prochains `@dev` : 571 → 573 → 575 → 563.

> 🪧 **15:44 UTC (2026-11-21)** : **WFS-590** (P1, E15 : caissons sur mâts retirés, enseignes en façade) attribué à Claude, agent dev3d (01UY1Vv5QX), nouvelle session `@dev`, à sa demande ; avis « VALIDÉ » de l'agent 3D obligatoire. Prochains `@dev` : 571 → 573 → 575 → 563. Bancs du matin #62 rouges (endurance et chauffe, tests CPU et licences, realisme-2, complements-c) : rangés dans WFS-589, ⏳ Prêt pour une nouvelle session `@cicd`.

> 🪧 **18:05 UTC (2026-11-20)** : **KO de l'utilisateur sur WFS-568** : ❌ Refusé (ANO-317), WFS-569 annulé ; nouvelle US **WFS-590** (P1, E15 : plus de caissons sur mâts, enseignes en façade des commerces) ⏳ Prêt pour une **nouvelle** session `@dev` (jamais la session 01UY1Vv3p8, dont la ligne est enlevée : session terminée). Prochains `@dev` : **WFS-590** → 571 → 573 → 575 → 563. CI de `main` #1157 et bancs du matin #58 rouges : suivis par WFS-589 (`@cicd`).

> 🪧 **00:17 UTC (2026-11-20)** : **WFS-568** (P1, E15, enseignes du décor réalistes, plus de néon cyan ; ANO-316) attribué à Claude, agent dev3d (01UY1Vv3p8) (session_01UY1Vv3p8auTZYZDh8Q5G5N), nouvelle session `@dev`, à sa demande ; avis « VALIDÉ » de l'agent 3D obligatoire (E15). Sessions de WFS-562 et WFS-567 terminées (fusionnés) : lignes enlevées. CI de `main` #1156 rouge (banc-essai texte) : suivie par WFS-589 (`@cicd`). Prochains `@dev` : WFS-571 → 573 → 575 → 563.

> 💬 **19:09 UTC (2026-11-19)** : **WFS-567** (P1, E8, messages de course lisibles, sans cyan, hors de la route ; ANO-315 + ANO-290) attribué à Claude, agent dev3d (01UY1Vv5BA) (session_01UY1Vv5BAe81SAYYdNAVWTi), nouvelle session `@dev`, à sa demande. Prochains `@dev` : WFS-568 → 571 → 573 → 575 → 563.

> 🖼️ **23:06 UTC (2026-11-18)** : **WFS-562** (P1, E23, images des modes, portique de checkpoint, fond pas noir) attribué à Claude, agent dev3d (01UY1Vv3kc) (session_01UY1Vv3kcVfmbaBKf14LPpJ), nouvelle session `@dev`, à sa demande. Sessions terminées (408, 550, 557, 558, 559, 561, 564) : lignes enlevées. Prochains pour de nouvelles sessions `@dev` : WFS-567 → 568 → 571 → 573 → 575 → 563.

> 🎮 **20:45 UTC (2026-11-18)** : priorité aux anomalies du gameplay ; ordre `@dev` : **WFS-562 → 567 → 568** (urgents, ordre de l'utilisateur), puis 571, 573, 575, 563 ; `@conduite` : WFS-584 ; `@cicd` : WFS-589, 454 en parallèle. Aucune session libre : ouvrir de nouvelles sessions.

> 🧾 **20:35 UTC (2026-11-18)** : **WFS-570** (P1, renonciation avant achat) ⏳ Prêt pour une nouvelle session `@dev` ; WFS-408 en PR #434 (session juriste).

> 💶 **19:37 UTC (2026-11-18)** : 1re version = achats + pub ; E21 en priorité (512 → 514 → 515 → 516, 516 passé P1) ; WFS-408 élargi en cours (session juriste).

> ⚖️ **19:36 UTC (2026-11-18)** : **WFS-408 élargi** (documents « avec achats », fiche FR, préparation Play Console) attribué à Claude, agent juriste (01UY1Vv2) sur ordre direct de l'utilisateur ; branche `feature/waforspeed-408`.

> ⚖️ **19:35 UTC (2026-11-18)** : 1re sortie publique **avec achats** (décision de l'utilisateur) : WFS-566 annulé ; WFS-408, 409 en variante « avec achats » ; aucune attribution.

> ⚖️ **19:31 UTC (2026-11-18)** : avis du juriste traité ; **WFS-566** (P1, Market caché dans la version publique) créé, ⏸ en attente de la décision de l'utilisateur et du jour de publication ; aucune attribution.

> 🖼️ **19:25 UTC (2026-11-18)** : **WFS-562** (P1, E23) ⏳ Prêt pour une nouvelle session `@dev` : images qui disent le mode (portique de checkpoint refait en vrai modèle, police aux gyrophares, peloton), fond pas noir (ANO-314) ; demande de l'utilisateur transmise par l'Agent qa.

> 🚫 **10:58 UTC (2026-11-16)** : **WFS-565 annulé** (ordre de l'utilisateur : traité dans WFS-557, RG-23) ; Claude, agent dev3d (01UY1Vv4Fd) garde son seul ticket WFS-557, élargi au choix de la carte. Restent ⏳ Prêt : **WFS-563** (P1, E23), **WFS-512** (P1, E21), puis WFS-554, 538, 553.

> 🗺️ **10:57 UTC (2026-11-16)** : **WFS-565** (P1, E23, choix de la carte à 100 % des maquettes 03 et 18a à 18d) créé, ⏳ Prêt pour une nouvelle session `@dev`, sur signalement de Claude, agent dev3d (01UY1Vv4Fd) (WFS-557, qui garde son seul ticket). CI de `main` #1149 en cours après WFS-564. Restent ⏳ Prêt : **WFS-563**, **WFS-565** (P1, E23), **WFS-512** (P1, E21), puis WFS-554, 538, 553.

> 🏁 **10:48 UTC (2026-11-16)** : **WFS-561** (P1, E23, résultats : tableau LED ambre, maquette 11) attribué à Claude, agent dev3d (01Us6uCxBL) (session_01Us6uCxBLnhYyVos7wEtk5U), nouvelle session `@dev`. Restent ⏳ Prêt pour de nouvelles sessions `@dev` : **WFS-563** (P1, E23), **WFS-512** (P1, E21), puis WFS-554, 538, 553.

> ⚖️ **09:47 UTC (2026-11-16)** : arbitrage de WFS-558 (Claude, agent dev3d (01Us6uCtQN)) : digicode à vraie taille avec vue rapprochée, pavé 6 × 6, listes dans le panneau de verre (RG-05 à RG-08).

> 🚦 **09:46 UTC (2026-11-16)** : **WFS-559** (P1, E23, portique de départ à 5 feux) attribué à Claude, agent dev3d (01Us6uCwnA), nouvelle session `@dev`.

> 🔴 **09:45 UTC (2026-11-16)** : CI de `main` #1147 rouge (WFS-551 : `ui/icone-pistolet.png` sans crédit) : US technique **WFS-564** (P1) attribuée à Claude, agent dev3d (01Us6uCxFh) (session_01Us6uCxFhiJ8TJGKy9QTPw7), nouvelle session `@dev`. Sessions de WFS-551, 496, 503 et du lead dev de WFS-555 terminées (fusionnés), lignes enlevées. Restent ⏳ Prêt pour de nouvelles sessions `@dev` : **WFS-559**, 561, 563 (P1, E23), WFS-512 (P1, E21).

> 🅿️ **09:44 UTC (2026-11-16)** : **WFS-558** (P1, E23, scène 3D du duel en ligne) attribué à Claude, agent dev3d (01Us6uCtQN), nouvelle session `@dev`. Sessions 01Us6uCwHf (496), 01Us6uCwe9 (503), 01UY1Vv2KF (551) et lead-dev 555 : tickets fusionnés, lignes enlevées. Restent ⏳ Prêt pour de nouvelles sessions `@dev` : WFS-559, 561, 563, puis 512.
> ⚖️ **01:05 UTC (2026-11-16)** : arbitrage WFS-550 / 551 / 557 : onglets du garage à WFS-551 (déjà sur sa branche), contenu de l'onglet Voiture à WFS-550 ; menu (maquette 01, colonne Market / Profil / Crédits retirée) passe de WFS-557 à WFS-550.

> 🖼️ **01:01 UTC (2026-11-16)** : arbitrage RG-21 / RG-22 de WFS-557 (Claude, agent dev3d (01UY1Vv4Fd)) : choix du mode (maquette 02), Distance et Difficulté dans la carte du mode choisi ; vraies images des modes dans la nouvelle US **WFS-562** (P1, E23, après WFS-557).

> 🏗️ **00:57 UTC (2026-11-16)** : arbitrage RG-20 de WFS-557 (Claude, agent dev3d (01UY1Vv4Fd)) : scènes 3D des maquettes 04, 09, 10, 11 sorties du ticket ; nouvelles US **WFS-558 à 561** (P1, E23) pour de nouvelles sessions `@dev`.

> 🔍 **23:55 UTC (2026-11-15)** : **WFS-557** (P1, E23, vérifier 548, 549, 552 contre les maquettes R19 à 100 % et corriger) attribué à Claude, agent dev3d (01UY1Vv4Fd), nouvelle session `@dev`, sur ordre de l'utilisateur (« @dev prend le WFS-557 »).

> 🎁 **23:15 UTC (2026-11-15)** : **WFS-503** (P1, E19, cadeau du jour : gains réels) attribué à Claude, agent dev3d (01Us6uCwe9), nouvelle session `@dev`. Restent ⏳ Prêt pour de nouvelles sessions `@dev` : **WFS-512** (P1), puis WFS-554, 538, 553.

> 🏷️ **23:14 UTC (2026-11-15)** : **WFS-496** (P1, E18, atelier d'autocollants) attribué à Claude, agent dev3d (01Us6uCwHf), nouvelle session `@dev`. Restent ⏳ Prêt pour de nouvelles sessions `@dev` : **WFS-503**, **WFS-512** (P1), puis WFS-554, 553 (P2).

> ⏱️ **22:58 UTC (2026-11-15)** : **WFS-555** `[TECHNIQUE]` (P1, cache des captures « avant » et compilation de main dans `tools/qa/captures_pr.sh`) confié au **lead-dev (session_01UY1Vv2AmFHmyurYWvyN3oV)** en renfort (ordre de l'utilisateur) ; PR relue par une autre instance du lead dev. Règles 3 et 4 (captures ciblées, 2e revue limitée aux points corrigés) en vigueur : bandeau de `TACHES.md`. WFS-550 ↩️ Retour lead dev (PR #424).

> 💧 **21:01 UTC (2026-11-15)** : **WFS-551** (P1, E23) attribué à Claude, agent dev3d (01UY1Vv2KF), nouvelle session `@dev`. Restent ⏳ Prêt pour de nouvelles sessions `@dev` : **WFS-554**, WFS-553 (P2).

> 📊 **20:43 UTC (2026-11-15)** : **WFS-550** (P1, E23) attribué à Claude, agent dev3d (01UY1Vv1yg), nouvelle session `@dev`. WFS-548, 549, 552 et 541 fusionnés : sessions 01Us6uCucP, 01Us6uCvCC, 01Us6uCwZ7, 01UY1Vv1WU terminées, lignes enlevées. Restent ⏳ Prêt pour de nouvelles sessions `@dev` : **WFS-551**, puis WFS-554.

> 🧩 **16:25 UTC (2026-11-15)** : WFS-552 fusionné (#421), session 01Us6uCwZ7 terminée ; réserves mineures → **ANO-275** et **WFS-554** (P2, ⏳ Prêt, pour une nouvelle session `@dev`).

> 🧪 **14:33 UTC (2026-11-15)** : anomalie **ANO-274** (banc `score-fin` en retard sur le Market de WFS-513, 40 échecs) signalée par l'agent dev3d (01Us6uCucP) pendant WFS-548 ; ticket **WFS-553** `[TECHNIQUE]` ⏳ Prêt (P2) pour une **nouvelle session** `@dev`. 01Us6uCucP continue WFS-548 seul.

> 🚗 **13:12 UTC (2026-11-15)** : **WFS-541** (P1, trafic de la Poursuite) attribué à Claude, agent conduite (01UY1Vv1WU), nouvelle session `@conduite`. Reste ⏳ Prêt pour une nouvelle session `@conduite` : **WFS-394**.

> 🅿️ **12:01 UTC (2026-11-15)** : **WFS-552** attribué à Claude, agent dev3d (01Us6uCwZ7), nouvelle session `@dev`. Les 3 tickets prêts de E23 sont pris (548, 549, 552) ; 550 et 551 attendent la fusion de 548 + 549.

> 🔘 **12:01 UTC (2026-11-15)** : **WFS-549** attribué à Claude, agent dev3d (01Us6uCvCC), nouvelle session `@dev`. Il reste **1 session `@dev`** à ouvrir pour **WFS-552** (P1, E23, menu 3D).

> 🪟 **11:58 UTC (2026-11-15)** : **WFS-548** attribué à Claude, agent dev3d (01Us6uCucP), nouvelle session `@dev`. Il reste **2 sessions `@dev`** à ouvrir pour **WFS-549** et **WFS-552** (P1, E23).

> 🎨 **11:13 UTC (2026-11-15)** : épic **E23 Interface R19** créée : **WFS-548**, **WFS-549**, **WFS-552** ⏳ Prêt (P1, en parallèle), **WFS-550** et **WFS-551** en attente de 548 + 549. **Aucune session attribuée** : il faut **3 nouvelles sessions `@dev`** (une par ticket prêt) pour tenir la demi-journée.

> 🔴 **Alerte** : 18:31 UTC : **WFS-494** (P1, garage : skins et skin offerte, 6e du classement 📋) attribué à Claude, agent dev3d (01Us6uCwV6), nouvelle session `@dev` ; WFS-538 et WFS-496 passent ⏳ Prêt. Avant : 18:29 UTC : **WFS-446** (P2, manque de mémoire graphique suite 1, 24e du classement 📋) attribué à Claude, agent dev3d (01Us6uCuVU), nouvelle session `@dev` ; WFS-493 fusionné (#413), session 01Us6uCwTQ terminée, ligne enlevée. Avant : 18:12 UTC : **WFS-513** (P1, écran Market FR / EN, 17e du classement 📋) attribué à Claude, agent dev3d (01Us6uCvpo), nouvelle session `@dev` ; WFS-511 fusionné (#416), session 01Us6uCtfa terminée, ligne enlevée. Avant : 16:30 UTC : **WFS-511** (P1, Google Play Billing, 15e du classement 📋) attribué à Claude, agent dev3d (01Us6uCtfa), nouvelle session `@dev` ; WFS-502 fusionné (#414, 🟡) ; lignes des sessions terminées (482, 495, 502) enlevées. Avant : 13:21 UTC : **WFS-502** (P1, voiture prêtée 3 h, 10e du classement 📋, prêt depuis 13:18) attribué à Claude, agent dev3d (01Us6uCv9g) à la place de WFS-511 (15e), qui revient ⏳ Prêt (erreur du PO à 13:20 corrigée). Avant : 13:21 UTC : **WFS-495** (P1, autocollants, 7e du classement 📋) attribué à Claude, agent dev3d (01Us6uCv3C). 13:21 UTC : **WFS-493** (P1, skins Drift Tokyo / Néon nuit / Camouflage, 5e du classement 📋) attribué à Claude, agent dev3d (01Us6uCwTQ). 13:20 UTC : **WFS-511** (P1, Google Play Billing, 15e du classement 📋, 1re tâche prête pour `@dev`) attribué à Claude, agent dev3d (01Us6uCv9g), nouvelle session `@dev`, à sa demande ; CI de `main` #1069 rouge = licences, déjà corrigé par WFS-543 (run de WFS-542 en cours). Avant : 13:18 UTC : **WFS-482** (P1, écran « Profil », 4e du classement 📋) attribué à Claude, agent dev3d (01Us6uCv3H). CI de `main` #1069 rouge = licences (corrigé par WFS-543, #1073 en cours). Sessions terminées (WFS-542 à 546) retirées du tableau. Avant : 10:58 UTC : **WFS-546** (P1, mode testeur « +2500 ★ » dans le diagnostic des Réglages) attribué à Claude, agent dev3d (01Us6uCvwS) (bandeau ⭐, demande urgente de l'utilisateur). Avant : 10:51 UTC : **WFS-545** (P1, clavier : une touche écrit plusieurs fois) attribué à Claude, agent dev3d (01Us6uCthf). Avant : 10:42 UTC : **WFS-544** (P1, bancs du matin #32 : pipelines en course + parcours `records`) attribué à Claude, agent dev3d (01Us6uCwQU). Avant : 10:42 UTC : **WFS-543** (P1, CI de `main` rouge, licences W212 / Virtus GT) attribué à Claude, agent dev3d (01Us6uCwcF) ; **WFS-544** (P1, pipelines du banc « vitrines », bancs du matin #32) ⏳ Prêt pour une nouvelle session `@dev`. Avant : 10:39 UTC : **WFS-542** (P1, vrai camion au lieu du Range Rover géant) attribué à Claude, agent dev3d (01Us6uCtQB) ; **CI de `main` rouge** (#1062 à #1065, licences : `cars/remplacants/virtus` et `w212` sans crédit ; bancs du matin #32 rouges, #33 vert) → **WFS-543** [TECHNIQUE] P1 ⏳ Prêt pour une nouvelle session `@cicd` ; lignes des sessions terminées (540, 484, 501, 499, 492, 491, 537) enlevées. Avant : 00:26 UTC : **WFS-540** (P1, police = vraie voiture) attribué à Claude, agent conduite (01UY1Vv3RA). Avant : 00:11 UTC : **WFS-542** (vrai camion au lieu du Range Rover géant, P1, `@dev`) ⏳ Prêt ; glisse de la police ajoutée à WFS-540 (ANO-272). Avant : 23:24 UTC : **WFS-484** (E17, plaques au hasard des autres voitures) attribué à Claude, agent dev3d (01Us6uCvb4) ; prochaine `@dev` : WFS-511 (15e de la liste 📋, 1re prête restante pour `@dev` : 482, 493 à 497, 502, 503, 538 attendent). Avant : 23:23 UTC : **WFS-501** (E19, écran du cadeau du jour) attribué à Claude, agent dev3d (01Us6uCviA) ; prochaine `@dev` : WFS-484 (3e de la liste 📋). Avant : 22:55 UTC : vidéos de l'utilisateur (Poursuite coincée) : **WFS-461 ❌ refusé** ; **WFS-540** et **WFS-541** (P1, `@conduite`, pas 01UY1Vv1A7) ⏳ Prêt en tête de tout, à deux nouvelles sessions `@conduite`. Avant : 21:25 UTC : **WFS-492** (E18, skins : texture d'habillage + skin Rallye sur 6 voitures) attribué à Claude, agent dev3d (01Us6uCutV). Avant : 21:24 UTC : **WFS-491** (E18, garage : choix de la finition) attribué à Claude, agent dev3d (01Us6uCwfT) ; WFS-490 et 483, 500 fusionnés : **WFS-492, 539, 484, 501** passent ⏳ Prêt ; **WFS-511** reste ⏳ Prêt. Avant : 20:45 UTC : **WFS-537** (E22, écran de fin du score + compteur d'étoiles du menu) attribué à Claude, agent dev3d (01UY1Vv4Hv). **WFS-511** (Google Play Billing) reste ⏳ Prêt pour la prochaine session `@dev`. Avant : 18:45 UTC : **WFS-536** (E22, score : les ★ du score remplacent les 10 ★ fixes) attribué à Claude, agent dev3d (01UY1Vv3A5). **WFS-535** fusionné : **WFS-537** (écran de fin du score) ⏳ Prêt ; **WFS-510** fusionné : **WFS-511** (Google Play Billing) ⏳ Prêt, pour de nouvelles sessions `@dev`. Retours lead dev à corriger : WFS-481 (#379), 489 (#395), 490 (#388), 507 (#396), 397 (#393). En PR : 500 (#392), 401 (#394), 483 (#397). WFS-410 🔧. Autres prêts : WFS-454 (`@cicd`), 394 (`@conduite`), 532, 446. WFS-405, 319, 407, 408, 409 en attente du jour de publication ; Campagne et Grand Prix ⛔ (ordre de l'utilisateur).

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| Claude, agent dev3d (01UY1Vv49f) (session_01UY1Vv49fLTvKRexaJUt2wy) | WFS-594 | Routes et sols réalistes de près (asphalte, marquages, trottoirs, herbe, terre, sable) | `feature/waforspeed-594`, poussée (dernier envoi 13:52 UTC) | 🔧 En cours : code et banc `routes-sols` faits, tests et captures en cours |
| Claude, agent dev3d (01UY1Vv1zn) (session_01UY1Vv1znETyWL8wzqa2tEb) | — | — | `feature/waforspeed-597` (fusionnée, #439, 528ed3b6) | ⏹️ session terminée (WFS-597 fusionné) |
| Claude, agent dev3d (01UY1Vv4Di) (session_01UY1Vv4DiuxbNaC43HNZKKg) | WFS-593 | 10 % des personnes dans toutes les cartes (facteur commun, conducteurs et deux-roues inchangés) | `feature/waforspeed-593`, poussée | 🔀 En PR (#442), lead dev appelé (14:32 UTC) |
| Claude, agent dev3d (01UY1Vv2sH) (session_01UY1Vv2sHodzrS5FyXVX9uR) | WFS-596 | Vue Cockpit : la mini-carte du HUD sur l'écran central de la voiture, plus à l'écran (autres vues inchangées) | `feature/waforspeed-596`, poussée 2026-11-22 14:27 UTC | 🔀 En PR #441, lead dev appelé |
| Claude, agent dev3d (01UY1Vv4Xe) (session_01UY1Vv4Xeus56avzEmiAhcZ) | WFS-571 | Ville : tours en boîtes et cubes nus sur les toits remplacés par de vrais volumes (ANO-280) | `feature/waforspeed-571`, à créer | 🔧 En cours (attribué 2026-11-22 12:53 UTC) |
| agent expert ci cd (01UY1Vv43t) (session_01UY1Vv43tZmAYr8FEMaKo7S) | WFS-589 | CI de `main` rouge : crédits des images `ui/modes`, parcours « fin → Menu » du banc, realisme-2 | `feature/waforspeed-589` | 🔀 PR #440 ouverte (14:20 UTC), bancs du matin #70 de la branche verts ; lead dev appelé |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| dev | développeur général (`.claude/agents/dev.md`, appelé par `@dev`) : un ticket du PO, jusqu'à la fusion, puis arrêt | créé le 2026-11-09 (demande de l'utilisateur) |
| expert-conduite | développeur expert en physique de la voiture (skill `expert-conduite`) : chantier « Conduite réaliste » | créé le 2026-11-07 |
| lead-dev | relit chaque PR : ouvre les captures avant / après (≥ 3, `qa/pr/WFS-XX/`), compare à la demande de l'utilisateur, **seul à fusionner** (squash) ou refuse ; appelé par le développeur juste après sa PR (`.claude/agents/lead-dev.md`) | 21:27 (11/11) : PR #363 (WFS-451) relue par une autre instance, approuvée et fusionnée (df461365) ; 18:58 : PR #297 (WFS-338) en revue ; 17:24 : #311 (X5) refusée une 3e fois, fermée |
| dev-artiste-3d | développeur **et** artiste 3D (skills `agent-developer`, `waforspeed-natif`, `artiste-3d`) : voitures sans marque, modèles 3D + code ; demande son ticket au PO (`@devartiste`, `@dev3d`) | créé le 2026-11-09 19:22 UTC (demande de l'utilisateur) ; tickets voitures WFS-358 à 361 |
| artiste-3d | artiste 3D (skill `artiste-3d`) : choisit, retouche, intègre les modèles et actifs 3D, sans code du moteur ; demande son ticket au PO (`@artiste`) | créé le 2026-11-09 19:22 UTC (demande de l'utilisateur) |
| expert-cartes | développeur expert des cartes et de leur téléchargement (paquets de cartes, E14, WFS-301 à 314) ; skill `expert-cartes` ; demande son ticket au PO | créé le 2026-11-08 14:14 UTC (demande de l'utilisateur) |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette du 2026-11-08 nuit terminée (01:06) : 25 tickets ; ANO-160 à 167 créées, planifiées par le PO à 01:12 |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | à consulter par arwa-2 pour WFS-192 (méthode d'effacement des logos, ΔE2000 ≤ 2) |
| juriste | juriste (skill `juriste`, `@juriste`) : regarde l'application du côté juridique (marques, licences NC, crédits, RGPD, CGU, Google Play) ; rapports dans `juridique/` ; ne code pas | créé le 2026-11-10 12:40 UTC (demande de l'utilisateur) ; 1er travail attendu : avis sur la sortie Android (WFS-320, WFS-404) |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | 2026-11-07 : constats et recommandations performance (ANO-125) transmis au PO sur ordre de l'utilisateur |
| agent expert ci cd (session 01UY1Vv4) | expert CI / CD (pipeline et livraison) (`.claude/agents/expert-ci-cd.md`) : diagnostics, US techniques de la CI | 2026-11-09 10:42:51 : WFS-351 🟣 PR #306 fusionnée par le lead dev (788c286) ; libre |
| agent expert ci cd (session 01UY1Vv2Hq) | expert CI / CD (pipeline et livraison) (`.claude/agents/expert-ci-cd.md`) : diagnostics, US techniques de la CI | 2026-11-11 12:57 : WFS-427 attribué (bancs du matin, `ecrans-2400` scindé), voir le tableau des développeurs ; 09:13 : « Paquets de cartes » relancé sur `main` (ordre de l'utilisateur), run n° 2 vert, paquets de la Ville de WFS-416 publiés |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
