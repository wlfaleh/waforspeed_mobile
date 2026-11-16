# WaForSpeed Mobile — journal

Journal du projet (tickets WFS-XX de `TACHES.md`) : une entrée par étape (début, construction, PR, verdict QA, fin…) avec
fait / fichiers / vérification / limites. **Nouvelles entrées à la fin du fichier.** Toujours partir de la dernière version de
`main` (`git pull`), ajouter seulement ses entrées, puis envoyer tout de suite ; ne jamais écraser le fichier.
**Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
Exemple : `### 2026-11-03 19:43:10 — WFS-58 construit, essai simulateur lancé (Claude, session trafic)`.
> **Archive (règle de l'utilisateur, 2026-11-06)** : ce fichier ne garde que les **2 derniers jours**. Les entrées plus anciennes sont dans `journal/archive/AAAA-MM-JJ.md` (un fichier par jour) ; ne les relire qu'en cas de besoin précis. Le PO déplace chaque jour les entrées de plus de 2 jours.

### 2026-11-15 00:02:50 — R19 : 3 versions du bouton Garage (smart-brainstorming)
- Demande de l'utilisateur : bouton Garage du menu à améliorer ; planche `references/maquettes-R19/options-bouton-garage.jpg` (plaque actuelle, clé de voiture, pilule chrome lumineuse), choix à faire. Rien transmis au PO.

### 2026-11-15 00:03:14 — R19 : bouton Garage en pilule chrome (smart-brainstorming)
- Choix n° 3 de l'utilisateur intégré au menu ; planche des 3 versions retirée. Rien transmis au PO.

### 2026-11-15 00:05:50 — R19 : garage, exemple de voiture bloquée (smart-brainstorming)
- Demande de l'utilisateur : voiture avec cadenas dans l'onglet Voiture (`15-garage-voiture-bloquee.jpg`). Rien transmis au PO.

### 2026-11-15 00:08:36 — R19 : 3 façons de montrer une voiture bloquée (smart-brainstorming)
- Cadenas 3D refusé par l'utilisateur ; planche `references/maquettes-R19/options-voiture-bloquee.jpg` (sobre, vitrine sombre, rideau de garage), choix à faire. Rien transmis au PO.

### 2026-11-15 00:10:13 — R19 : voiture bloquée, cadenas-jauge à côté d'Acheter (smart-brainstorming)
- Les 3 versions refusées ; demande de l'utilisateur : icône de cadenas créative à côté du bouton Acheter. Cadenas-jauge (progression des étoiles) dans `15-garage-voiture-bloquee.jpg`. Rien transmis au PO.

### 2026-11-15 00:12:36 — R19 : voiture bloquée, cadenas fusionné dans le bouton Acheter (smart-brainstorming)
- Retour de l'utilisateur (« encombrement des icônes ») : cadenas et jauge intégrés au bouton Acheter. Rien transmis au PO.

### 2026-11-15 00:16:42 — R19 : cadenas-jauge à côté du nom de la voiture (smart-brainstorming)
- Choix de l'utilisateur : le cadenas-jauge (avant-dernière version) placé juste à côté du nom. Rien transmis au PO.

### 2026-11-15 00:19:36 — R19 : cadenas seul à côté du nom, étoiles près d'Acheter (smart-brainstorming)
- Retour de l'utilisateur : seulement l'icône à côté du nom, les étoiles près du bouton Acheter. Rien transmis au PO.

### 2026-11-15 00:20:54 — R19 : étoiles sous le bouton Acheter (smart-brainstorming)
- Icône validée par l'utilisateur ; « ★ 1 250 / 6 000 » déplacé sous le bouton Acheter, dans le coin. Rien transmis au PO.

### 2026-11-15 00:22:31 — R19 : position de la voiture dans la liste (smart-brainstorming)
- Demande de l'utilisateur : indicateur de position (17 segments + « 17 / 17 ») sous le plateau. Rien transmis au PO.

### 2026-11-15 00:24:20 — R19 : onglet Peinture du garage (smart-brainstorming)
- Demande de l'utilisateur : proposition pour la tab Peinture, dans le style de l'onglet Voiture validé (`06-garage-couleur.jpg` remplacé). Rien transmis au PO.

### 2026-11-15 00:27:39 — R19 : onglet Peinture avec bombes, graffiti et seau (smart-brainstorming)
- Retour de l'utilisateur (« pas de créativité ») : couleurs en bombes de peinture, graffiti sur le mur, seau et pinceau. Rien transmis au PO.

### 2026-11-15 00:28:35 — R19 : graffiti retiré de l'onglet Peinture (smart-brainstorming)
- Graffiti refusé par l'utilisateur, retiré ; bombes et seau gardés. Rien transmis au PO.

### 2026-11-15 00:30:03 — R19 : bombes de peinture façon graffiti (smart-brainstorming)
- Précision de l'utilisateur (« déodorant à graffiti » = la bombe) : bombes sans capuchon, embout blanc, coulures. Rien transmis au PO.

### 2026-11-15 00:39:57 — R19 : onglet Peinture en taches de peinture (smart-brainstorming)
- Retour de l'utilisateur : seau retiré, bombes refusées ; une tache de peinture par couleur. Rien transmis au PO.

### 2026-11-15 00:41:07 — R19 : 4 styles d'icônes pour les couleurs (smart-brainstorming)
- Demande de l'utilisateur : une icône par couleur (goutte, pistolet, pot, bombe) ; planche `references/maquettes-R19/options-icones-couleurs.jpg`, choix à faire. Rien transmis au PO.

### 2026-11-15 00:43:13 — R19 : couleurs en gouttes de peinture (smart-brainstorming)
- Choix de l'utilisateur (style 1, goutte) intégré à l'onglet Peinture ; planche des 4 styles retirée. Rien transmis au PO.

### 2026-11-15 00:45:59 — R19 : onglets Finition et Skins du garage (smart-brainstorming)
- Demande de l'utilisateur : tabs Finition et Skins dans le style de l'onglet Peinture (gouttes) ; `16-garage-finition.jpg`, `07-garage-skin.jpg` remplacé. Rien transmis au PO.

### 2026-11-15 00:49:40 — R19 : skins en grandes pastilles, finitions en pistolets (smart-brainstorming)
- Idées de l'utilisateur (image de référence d'un pistolet de peinture) : onglet Skins en grandes pastilles, onglet Finition en pistolets de carrossier. Rien transmis au PO.

### 2026-11-15 00:52:28 — R19 : finitions avec l'icône de pistolet de l'utilisateur (smart-brainstorming)
- Retour de l'utilisateur : utiliser son image ; icône reprise telle quelle, seul le godet et le jet changent de finition ; image gardée dans `references/maquettes-R19/icone-pistolet-utilisateur.png`. Rien transmis au PO.

### 2026-11-15 00:58:39 — R19 : proposition pour l'interface de conduite (smart-brainstorming)
- Demande de l'utilisateur : améliorer boutons et infos en course ; maquette `17-conduite-hud.jpg` et avant / après. Rien transmis au PO.

### 2026-11-15 01:04:31 — R19 : interface de conduite corrigée (smart-brainstorming)
- Retours de l'utilisateur : nitro gardé (couleur seulement), volant gardé, flèches retirées (autre mode), pédales refusées → cartes Frein / Gaz en verre. Rien transmis au PO.

### 2026-11-15 01:05:53 — R19 : 3 versions de Gaz et Frein (smart-brainstorming)
- Retour de l'utilisateur ambigu (« tu changes gaz et frein ») ; planche `references/maquettes-R19/options-gaz-frein.jpg` (ronds en verre, famille Start/Stop, pilules chrome), choix à faire. Rien transmis au PO.

### 2026-11-15 01:08:32 — R19 : Gaz et Frein d'aujourd'hui au style du GPS, sans texte (smart-brainstorming)
- Choix de l'utilisateur : garder l'existant, style proche du GPS, enlever « FREIN » / « GAZ ». Planche des 3 versions retirée. Rien transmis au PO.

### 2026-11-15 01:11:12 — R19 : boutons Menu et Caméra agrandis (smart-brainstorming)
- Demande de l'utilisateur : Menu et Caméra plus grands dans l'interface de conduite. Rien transmis au PO.

### 2026-11-15 01:13:13 — R19 : Menu et Caméra au style du GPS avec texte (smart-brainstorming)
- Demande de l'utilisateur : même cadre que le GPS, icône pause + « MENU », icône caméra + vue active. Rien transmis au PO.

### 2026-11-15 01:15:29 — R19 : volant au style du GPS (smart-brainstorming)
- Demande de l'utilisateur : même style que le GPS pour le cadre du volant. Rien transmis au PO.

### 2026-11-15 01:18:22 — R19 : menu avec la SF-X rose et bleu ciel, plus près (smart-brainstorming)
- Demande de l'utilisateur : meilleure voiture, meilleure skin, rose et bleu ciel, zoom. Rien transmis au PO.

### 2026-11-15 01:20:53 — R19 : menu, SF-X en caméléon (smart-brainstorming)
- Demande de l'utilisateur : finition caméléon (rose → violet → bleu ciel selon l'angle), skin bleu ciel gardée. Rien transmis au PO.

### 2026-11-15 01:22:48 — R19 : caméléon rose, orange, bleu ciel (smart-brainstorming)
- Demande de l'utilisateur : orange au lieu de violet dans le caméléon du menu. Rien transmis au PO.

### 2026-11-15 01:24:21 — R19 : caméléon orange et bleu ciel (smart-brainstorming)
- Demande de l'utilisateur : orange aussi à la place du rose. Rien transmis au PO.

### 2026-11-15 01:25:18 — R19 : caméléon seulement orange et bleu (smart-brainstorming)
- Demande de l'utilisateur : uniquement orange et bleu. Rien transmis au PO.

### 2026-11-15 01:27:52 — R19 : menu sur la Corniche de nuit, près des bateaux (smart-brainstorming)
- Demande de l'utilisateur : arrière-plan de nuit sur la Corniche à côté des bateaux ; vraie capture du banc du matin (groupe nuit) en fond, SF-X posée sur la route. Rien transmis au PO.

### 2026-11-15 01:29:08 — R19 : menu, seul l'arrière-plan passe à la Corniche (smart-brainstorming)
- Retour de l'utilisateur : ne pas changer l'angle de la voiture ; angle, zoom et console remis, seul le fond (Corniche de nuit, bateaux) remplace le parking. Rien transmis au PO.

### 2026-11-15 01:32:50 — R19 : menu devant la marina de nuit (smart-brainstorming)
- Retour de l'utilisateur (« je ne vois pas la vue du port ») : fond remplacé par la marina de nuit (capture WFS-376), voiture inchangée. Rien transmis au PO.

### 2026-11-15 09:41:39 — Statuts automatiques, bancs du matin #37 (robot statuts)
- Run vert (schedule, commit 7319f26) : WFS-453 ✅ (PR #367, f0d3fc7), WFS-456 ✅ (PR #385, 0170349), WFS-543 ✅ (PR #407, 9b36961), WFS-544 ✅ (PR #411, ba46732), WFS-533 ✅ (PR #381, 9153136), WFS-534 ✅ (PR #387, 081046f), WFS-535 ✅ (PR #389, e58f143), WFS-536 ✅ (PR #399, 58e2e77), WFS-537 ✅ (PR #402, 7adc226), WFS-480 ✅ (PR #375, 3595a68), WFS-481 ✅ (PR #379, 09a8ecf), WFS-482 ✅ (PR #412, 1ae3522), WFS-483 ✅ (PR #397, ee2c406), WFS-484 ✅ (PR #405, 42d1525), WFS-498 ✅ (PR #382, 16d0133), WFS-499 ✅ (PR #404, 8f0fc34), WFS-487 ✅ (PR #373, 2f08d2c), WFS-488 ✅ (PR #380, 973abee), WFS-489 ✅ (PR #395, a768856), WFS-490 ✅ (PR #388, c11a31b), WFS-491 ✅ (PR #401, d94049f), WFS-492 ✅ (PR #400, 41f3481), WFS-493 ✅ (PR #413, deeefa3), WFS-494 ✅ (PR #419, 48c6727), WFS-495 ✅ (PR #415, 9c3c729), WFS-500 ✅ (PR #392, 011cb9f), WFS-501 ✅ (PR #403, a4baf6f), WFS-504 ✅ (PR #372, b87349a), WFS-505 ✅ (PR #383, 0bcee2e), WFS-506 ✅ (PR #386, 08cc1fe), WFS-507 ✅ (PR #396, 7d11eb5), WFS-508 ✅ (PR #384, 19c1d0b), WFS-510 ✅ (PR #390, 2de12ab), WFS-511 ✅ (PR #416, dd02ea5), WFS-513 ✅ (PR #418, 70900a7), WFS-446 ✅ (PR #417, 7cdc98d), WFS-397 ✅ (PR #393, 50e5b27), WFS-378 ✅ (PR #391, a56b68c), WFS-395 ✅ (PR #374, f16e064).

### 2026-11-15 10:24:49 — R19 : menu de nouveau au garage, écrans des cartes à télécharger (smart-brainstorming)
- Demandes de l'utilisateur : menu remis au garage ; écrans des paquets de cartes (grisée, téléchargement, installée, mise à jour) inspirés de l'existant avec les nouveaux boutons (18a à 18d). Rien transmis au PO.

### 2026-11-15 10:27:05 — R19 : écran de chargement d'après l'existant (smart-brainstorming)
- Demande de l'utilisateur : chargement inspiré de l'écran actuel (WFS-297) avec les nouveaux boutons ; états normal et erreur. Rien transmis au PO.

### 2026-11-15 10:28:52 — R19 : chargement au lancement de l'application (smart-brainstorming)
- Précision de l'utilisateur : écran de chargement de l'application, pas d'une course ; carte, mode et astuce retirés. Rien transmis au PO.

### 2026-11-15 10:30:38 — R19 : Réglages, seulement les boutons (smart-brainstorming)
- Demande de l'utilisateur : petite modification, juste les boutons des Réglages (sélecteurs de verre, Fermer, Crédits, Conditions). Rien transmis au PO.

### 2026-11-15 10:33:31 — R19 : page Market façon boutique de jeu mobile (smart-brainstorming)
- Demande de l'utilisateur : Market inspiré des boutiques d'autres applications (pas comme la borne de R18) ; packs et prix de configgame.json, sans pression d'achat. Rien transmis au PO.

### 2026-11-15 10:42:37 — Archives automatiques (robot archives)
- Journal : 220 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-13).
- TACHES.md : 40 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md` (WFS-453, WFS-456, WFS-543, WFS-544, WFS-547, WFS-533, WFS-534, WFS-535, WFS-536, WFS-537, WFS-480, WFS-481, WFS-482, WFS-483, WFS-484, WFS-498, WFS-499, WFS-487, WFS-488, WFS-489, WFS-490, WFS-491, WFS-492, WFS-493, WFS-494, WFS-495, WFS-500, WFS-501, WFS-504, WFS-505, WFS-506, WFS-507, WFS-508, WFS-510, WFS-511, WFS-513, WFS-446, WFS-397, WFS-378, WFS-395).

### 2026-11-15 10:53:37 — R19 : écrans complémentaires (smart-brainstorming)
- Maquettes des 15 écrans qui manquaient, faites d'après les écrans actuels avec le nouveau modèle (verre, fonds floutés, Start/Stop, autocollants) : pause, création du profil, profil, conditions, salon du duel, réglages Conduite et Son, voiture prêtée, confirmation et manque d'étoiles, gain d'étoiles, messages en course, fins Course à 5, Poursuite et Duel. Images `references/maquettes-R19/19-…` à `33-…` et planche `19-33-ecrans-complementaires.jpg`, tableau R19 de `recommandations-smart-brain.md` complété. Proposition seulement : aucun code, aucun ticket.

### 2026-11-15 11:13:19 — Épic E23 « Interface R19 » et 5 tickets créés (po-scrum-master)
- Demande de l'utilisateur (transmise par smart-brainstorming) : « Ok crée les 5 tâches », découpage par thème, exactement 5 (17 puis 6 refusés).
- Épic **E23** (`regles-de-gestion/E23-interface-r19.md`) ; tickets **WFS-548** fonds et panneaux, **WFS-549** boutons, **WFS-552** menu 3D (SF-X caméléon, parking souterrain, `@dev` = dev-artiste-3d, avis `@3d`) : ⏳ Prêt, P1, en parallèle ; **WFS-550** jauges et **WFS-551** icônes / HUD : ⏳ En attente (après 548 et 549).
- 548 et 549 sur les mêmes écrans (exception voulue par l'utilisateur au garde-fou « un ticket par fichier de panneau du garage ») : le 2e fusionné se synchronise avec `main` et refait ses captures. 550 / 551 : fichiers répartis dans leurs règles (RG-07 / RG-08).
- Taille des boutons inchangée, sauf Menu et Caméra du HUD (validé par l'utilisateur dans la R19). 548 à 551 = interface 2D (design-street-racing) ; 552 = monde 3D (formes géométriques interdites, 99 %).
- Aucun ticket attribué (aucune session ouverte) : 3 nouvelles sessions `@dev` à ouvrir par l'utilisateur.

### 2026-11-15 11:15:14 — Statuts remis à jour, liste des 30 prochaines tâches donnée à l'utilisateur (po-scrum-master)
- WFS-503, 512, 457, 448, 447 : ⏳ En attente → ⏳ Prêt (dépendances fusionnées : 501, 487, 494, 511, 456, 446, 383).
- Ordre donné : E23 (548, 549, 552 prêts ; 550, 551 après 548 + 549), WFS-541 (bloquante), puis le classement 📋 restant (538 → 368).

### 2026-11-15 11:58:12 — Début WFS-548 : attribué à Claude, agent dev3d (01Us6uCucP) (po-scrum-master)
- **Ticket** : WFS-548, Interface R19 (1/5) : panneaux en verre dépoli, fond flouté (écrans arrêtés), sur-titres gris, couleurs d'état ; épic **E23**, P1 Urgent. Nouvelle session `@dev` (session_01Us6uCucPqZPRDwd6pkb4Z6), qui n'a jamais eu de ticket : attribution permise (un seul sujet par session).
- **Pourquoi lui** : 1er ticket prêt (plus petit numéro) de l'épic E23, plus récente demande de l'utilisateur (2026-11-15 11:10 UTC, cible : tout fusionné en une demi-journée).
- **Zone de code** : `src/ui/verre.*` (nouveau), `src/ui/ecrans/**` (écrans de RG-06), `src/ui/garage_achat*`, `shaders/ui*`, `tests/**` ; accroche renderer ≤ 10 lignes annoncée au journal. Taille des boutons inchangée ; les boutons sont WFS-549.
- **Règles** : `regles-de-gestion/WFS-548.md` et `regles-de-gestion/E23-interface-r19.md` ; maquettes `references/maquettes-R19/`.
- **Branche** : `feature/waforspeed-548`. WFS-549 touche les mêmes écrans : le 2e fusionné se synchronise avec `main` et refait ses captures.
- Restent ⏳ Prêt pour de nouvelles sessions `@dev` : WFS-549, WFS-552.

### 2026-11-15 12:01:26 — Début WFS-549 : attribué à Claude, agent dev3d (01Us6uCvCC) (po-scrum-master)
- **Demande** : nouvelle session `@dev` (session_01Us6uCvCCKjrpBx7JNmrkCa), jamais de ticket avant (« @dev go »).
- **Pourquoi lui** : 1er ticket prêt de l'épic E23 (plus récente demande de l'utilisateur, P1) après WFS-548, déjà à 01Us6uCucP.
- **Zone de code** : `src/ui/boutons.*` (nouveau), `src/ui/ecrans/**`, `src/ui/garage_cadre.*`, `garage_achat*`, `garage_couleur.*`, `garage_skin.*`, `garage_finition.*`, `garage_fiche.*` (boutons seulement), `src/ui/hud.*` (taille Menu / Caméra seulement), `shaders/ui*`, `tests/**`.
- **Règles** : `regles-de-gestion/WFS-549.md`, `regles-de-gestion/E23-interface-r19.md`, maquettes `references/maquettes-R19/`. Taille de chaque bouton identique à `main`, sauf Menu et Caméra du HUD.
- **Branche** : `feature/waforspeed-549`. Partage d'écrans avec WFS-548 : le 2e fusionné se synchronise avec `main` et refait ses captures. Après fusion : 🟡 À valider (utilisateur).
- Reste ⏳ Prêt pour une nouvelle session `@dev` : WFS-552.

### 2026-11-15 12:01:56 — Début WFS-552 : attribué à Claude, agent dev3d (01Us6uCwZ7) (po-scrum-master)
- **Ticket** : WFS-552 — Interface R19 (5/5) : menu en 3D, Cavallo SF-X caméléon orange / bleu, skin bleu ciel, parking souterrain, caméra rapprochée. Épic E23, P1 Urgent. Session `@dev` (= dev-artiste-3d) neuve (session_01Us6uCwZ7SWYSgoD4LUfvxv), jamais de ticket avant : attribution permise.
- **Choix** : la session demandait WFS-549 ou WFS-552 ; WFS-549 a été attribué à 12:01 UTC à la session 01Us6uCvCC (mon envoi a été refusé, `main` avait avancé) ; WFS-552 est le dernier prêt de E23 et demande justement le skill `artiste-3d`.
- **Zone de code** : `src/ui/ecrans/menu*`, `src/ui/garage_decor.*`, nouveaux fichiers du parking (zone interface ou `assets/world/modeles/garage/**`), catalogue des skins (E18) si la skin bleu ciel manque, `tests/**` ; accroche renderer ≤ 10 lignes annoncée au journal. Ne pas toucher `verre.*` (WFS-548) ni `boutons.*` (WFS-549).
- **Règles** : `regles-de-gestion/WFS-552.md`, `regles-de-gestion/E23-interface-r19.md`, `regles-de-gestion/DA-realisme.md`. Aucune forme géométrique nue ; modèles CC0 / CC BY (licence et SHA-256 au journal, `regles-reseau.md`) ; ≥ 30 i/s ; avis « VALIDÉ » de l'agent 3D sur le dernier commit avant fusion ; 🟡 après fusion.
- `track-agent.md` : lignes des sessions terminées (WFS-513, 446, 494) enlevées.

### 2026-11-15 12:30:40 — WFS-549 : boutons R19, avancement et accroches annoncées (Claude, agent dev3d (01Us6uCvCC))
- **Branche** `feature/waforspeed-549` poussée (commits f44e9dc9, 24595cf1, synchronisée avec `main`). Dessin commun `src/ui/boutons.{h,cpp}` : Start/Stop (RG-01), autocollant (RG-02), pilule chrome (RG-03), prix orange (RG-04), sélecteur de verre (RG-05, RG-06), verre cerclé d'orange (« ✕ Fermer »), sur les primitives lissées existantes `dessin2d` (aucun nouveau shader).
- **Écrans** : menu, pause, garage (Choisir / Acheter, Retour, Principale / Toit et rétros, skin offerte, fenêtre d'achat, fin d'essai), Réglages (choix, onglets, Fermer, « Crédits » ajouté à côté des conditions au menu principal), Market (prix, Retour), CGU (J'accepte), « Crée ton pilote », salon (Prêt, Inviter, Quitter, Retour, distance), terrain (Jour / Nuit, Retour, Rouler), profil, crédits, cadeau, paquets. Chaque zone de toucher et chaque rectangle restent ceux de `main` (RG-07) ; seuls Menu et Caméra du HUD passent à × 1,5 (`hud::BOUTON_ECHELLE`, maquette 17b).
- **Accroches hors zone (≤ 10 lignes chacune, remplacement du dessin d'un bouton, zones inchangées)** : `src/game/course_system.cpp`, `course5_system.cpp`, `poursuite_system.cpp` (« Rejouer » en Start/Stop), `duel_system.cpp` (« Revanche » Start/Stop, « Retour » autocollant), `modes_ecran.cpp` (« Continuer » Start/Stop, « Retour » autocollant).
- **Vérification** : compilation (Release, lavapipe) sans erreur ; tests CPU 145 / 145 (dont `test_hud_boutons` mis à 1,5 et nouveau `test_boutons`, contour arrondi) ; banc `menu1` regardé (menu, garage, pause, terrain, modes, fin). Captures avant / après en cours (`captures_pr.sh`).
- **Limites** : le Market n'a pas d'onglets aujourd'hui (Étoiles / Offres / Sans pub de la maquette 12) : non créés (changement de disposition, hors RG-07) ; icônes des onglets des Réglages laissées à WFS-551 (icônes) ; « Crédits » seulement au menu principal (l'écran Crédits n'existe qu'au menu).

### 2026-11-15 12:52:45 — WFS-552 : menu en 3D, avancement et accroches annoncées (Claude, agent dev3d (01Us6uCwZ7))
- **Branche** `feature/waforspeed-552` poussée (0984d07, synchronisée avec `main`). Derrière le menu : la voiture du joueur (sa peinture, sa finition, sa skin, dessinées par Game) garée en marche arrière dans un **parking souterrain** (`src/ui/menu_parking.*`), en **caméra rapprochée** trois quarts avant (`menu_parking_cadrage.*`, objectif décentré, voiture à droite de la console).
- **Parking (RG-01, RG-02)** : aucun modèle libre de parking aux vraies mesures n'est atteignable (GitHub public ; Sketchfab réservé aux voitures, `regles-reseau.md`) : tout est **modélisé par le code au niveau du vrai objet**, aux mesures NF P 91-100 (place 2,50 × 5,00 m, allée 6 m, dalle à 2,60 m, poutres retombées de 40 cm → 2,20 m libres) : poteaux 50 × 50 cm chanfreinés à bande de danger jaune / noire peinte, prédalles à joints en V, voiles banchés, dalle sciée époxy, bordures chasse-roues, marquage 12 cm, butées de roue 550 × 150 × 100 mm, réglettes LED IP65 1 500 mm (boîtier, diffuseur opale, embouts, clips), sprinklers DN 80 / DN 25 rouges avec suspentes et têtes à déflecteur denté. **Aucun téléchargement** (pas de licence ni de SHA-256 à noter). 29 172 triangles, 11 appels de dessin, construits au chargement ; repli sans plantage si la mémoire graphique manque (fond d'avant).
- **Accroches hors zone (annoncées, ≤ 10 lignes)** : `src/core/system.h` (1 ligne : `SceneDediee::interface`, l'interface du menu reste dessinée sur son fond 3D) ; `src/game/game.cpp` (3 lignes : fond du menu figé aussi avec ce fond, `sceneDediee` faux pendant `drawUi`) ; `src/platform/desktop_main.cpp` (2 lignes : banc `menu3d`) ; `shaders/vehicule.glsl` (caméléon à deux teintes : couleur choisie de face, teinte opposée sur les flancs ; **change aussi le caméléon des autres voitures**, ex. rouge → cyan) ; catalogue des skins E18 (`configgame.*`, `vehicule_skin*`, `garage_skin.h` : ordre) : 5e skin **Bleu ciel** (identifiant `ciel` : « bleuciel » est déjà une couleur), 500 ★, sur les 7 voitures à skins, la SF-X recevant aussi les 4 skins d'avant (11 textures, 1,33 Mo chacune, `tools/skins/generer.py`).
- **Vérification** : compilation sans erreur ; tests CPU (dont nouveau `test_menu_parking`, `test_vehicule_skin`, `test_garage_skin`, `test_personnalisation` mis à 5 skins) ; banc `menu3d` : parcours menu → garage → menu → course → menu OK, repli mémoire OK, 37 à 42 i/s en 1280 × 720 une fois le fond posé. Captures avant / après en cours.

### 2026-11-15 13:12:24 — Début WFS-541 : trafic de la Poursuite, attribué à Claude, agent conduite (01UY1Vv1WU) (po-scrum-master)
- Demande d'une **nouvelle session** `@conduite` (session_01UY1Vv1WUtoyPMiBqhjFrvU), aucun ticket avant : règle « un sujet par session » respectée ; ni 01UY1Vv1A7 (KO WFS-461) ni 01UY1Vv3RA (WFS-540).
- Choix : **WFS-541** (P1 Urgent, E3), 1er ticket prêt `@conduite` (bandeau 🚔 du 2026-11-13 22:54 : avant le classement 📋) ; WFS-394 (14e du classement) reste ⏳ Prêt pour la prochaine session `@conduite`.
- Zone : `src/game/traffic.*`, `src/game/traffic_system.*`, ≤ 10 lignes dans `src/game/poursuite_system.cpp`, `tests/**` ; pas `poursuite.cpp` / `player.*`.
- Critères : `regles-de-gestion/WFS-541.md` (RG-01 à RG-05 : arrêt ≤ 3 s puis redressement, retrait hors de la vue après 8 s, aucune voiture arrêtée > 10 s, pénétration ≤ 5 cm, +0,5 ms CPU max). Après fusion : 🟡 À valider (utilisateur).

### 2026-11-15 13:15:57 — WFS-541 : plan et branche (Claude, agent conduite (01UY1Vv1WU))
- **Causes trouvées** (`src/game/traffic.cpp`) : (1) une voiture accidentée ne se relève que si `crashT > 7 s` **et** le joueur est à plus de 60 m : joueur arrêté près d'elle (police, bouchon) → elle reste en travers sans fin, et le relèvement se faisait d'un coup, à 60 m, sous les yeux du joueur ; (2) **aucun contact entre voitures du trafic** : la glissade d'une accidentée (`crashVx`, jusqu'à ±1,12 de demi-largeur) et les changements de file passent dans les autres voitures ; (3) rien n'empêche une voiture du trafic de contenir la caméra.
- **Plan** : RG-01 : après la glissade (≤ 2,5 s), la voiture se redresse en roulant (modèle bicyclette, braquage ≤ 35°, ≤ 15 km/h, marche arrière si elle regarde vers l'arrière) jusqu'à sa file, puis reprend sa route ; pas redressée en 8 s → retirée seulement hors de la vue (> 150 m devant ou > 20 m derrière) et replacée loin (comme la régulation WFS-281). RG-03 : contacts trafic / trafic à chaque pas (emprises orientées WFS-333, pénétration ≤ 5 cm), trafic écarté de la police sans jamais déplacer la police (accroche ≤ 10 lignes annoncée dans `poursuite_system.cpp`), voiture qui contient la caméra non dessinée. RG-02 : vérifié au banc CPU (200 chocs, aucune voiture arrêtée > 10 s).
- Fichiers : `src/game/traffic.{h,cpp}`, `src/game/traffic_system.cpp`, `src/game/poursuite_system.cpp` (≤ 10 lignes), nouveau test `tests/test_trafic_chocs541.cpp`.

### 2026-11-15 14:19:16 — WFS-552 : PR #421 ouverte (Claude, agent dev3d (01Us6uCwZ7))
- **PR** #421 (`feature/waforspeed-552`, 5aa1b1b, synchronisée avec `main`) : 24 captures « après » et 6 « avant » dans `qa/pr/WFS-552/` (`captures_pr.sh … menu3d:2400x1080 menu3d:1280x720 menu1:2400x1080`).
- **Correction depuis l'entrée précédente** : le suivi du fond figé est remis à zéro à chaque `Game::init` et à chaque `setCar` (2 lignes de plus dans `src/game/game.cpp` : 5 en tout) ; sans cela, une partie rechargée directement sur le menu gardait l'image figée de la partie d'avant (vu au banc : nuit, Golf, repli).
- **Parcours (RG-06, banc `menu3d` 2400 × 1080)** : lancement → menu (parking, SF-X) → garage → retour au menu → course (la ville) → retour au menu : écrans OK ; nuit : même parking ; mémoire graphique refusée : fond d'avant, sans plantage.
- **Suite** : avis de l'agent 3D (`expert-realisme-3d`) dans la PR, puis lead dev. `TACHES.md` : 🔀 En PR (#421) ; `suivi-pr.md` : 🆕.

### 2026-11-15 14:20:20 — WFS-549 : PR #420 refusée par le lead dev (lead-dev)
- Commit relu ce342a2e. Branche construite et testée par le lead dev (Release) : compilation sans erreur, **145 test(s) réussi(s)**. 57 paires de captures regardées : menu, pause, Réglages, garage, Market, CGU, terrain, modes, fin et HUD (Menu et Caméra × 1,5, sans recouvrement) conformes à la R19, rectangles inchangés.
- **Défauts bloquants** : (1) capture des tests CPU absente (`tests-cpu.log`, section « Tests CPU ») ; (2) `reglages.cpp:348-349` : « Crédits » garde `g_doigt`, le relâcher va aux Crédits (Menu 450 avant Réglages 460), donc le 1er appui après le retour dans les Réglages est perdu ; (3) écrans modifiés sans capture : salon, fins du duel, de la Course à 5 et de la Poursuite, « Crée ton pilote », profil, cadeau, paquets, skin offerte, fenêtre d'achat, fin d'essai, Crédits ouverts depuis les Réglages ; (4) Market hors ligne : bouton inactif « Acheter / Buy » illisible (texte sombre sur orange atténué).
- Revue postée dans la PR (en commentaire : GitHub refuse REQUEST_CHANGES avec le même compte). TACHES : ↩️ Retour lead dev (PR #420) ; track-agent et suivi-pr mis à jour. À signaler au PO : la maquette 12 a des onglets au Market, absents ici (hors RG-07), à trancher.

### 2026-11-15 14:32:00 — WFS-548 : PR #422 ouverte, accroches annoncées (Claude, agent dev3d (01Us6uCucP))
- **Fait** : `src/ui/verre.{h,cpp}` (panneau de verre dépoli, sur-titres gris, couleurs d'état, dégradé de la marque), fond flouté des écrans arrêtés, écrans de RG-06 au modèle R19 ; test `test_verre`.
- **Accroches annoncées** : rendu `src/render/renderer.{h,cpp}` **8 lignes** (≤ 10, demande de flou `flouFond`, chaîne du bloom sans seuil, image floutée gardée comme le fond figé de WFS-329) et `shaders/post.frag` (mode 1 sans seuil, passe finale `params.z < 0`) ; `src/game/game.cpp` 1 ligne (jamais de flou en course) ; écrans de fin listés par RG-06 mais dessinés dans `src/game/` : `course_system.cpp`, `course5_system.cpp`, `poursuite_system.cpp`, `duel_system.cpp`, `score_fin.cpp` (voile → fond flouté, surface → `verre::panneau`, sur-titre gris), 3 à 8 lignes chacun.
- **Vérification** : 145 tests CPU réussis ; captures avant / après de 8 bancs (`captures_pr.sh`, FR et EN) + après seules (fins Course à 5, Poursuite, duel ; salon ; achat ; pause de nuit), dans `qa/pr/WFS-548/`.
- **Limites** : seule la scène 3D est floutée (l'interface dessous est voilée, pas floutée) ; pas vu sur téléphone (🟡 après fusion).
- **Anomalie trouvée, déjà sur `main`** : banc `score-fin` : 40 échecs identiques sur `main` et sur la branche (l'appui sur le compteur d'étoiles du menu n'ouvre plus le garage, puis le banc reste au garage et toutes ses fins de course Course à 5, Poursuite et duel échouent). Signalée au PO.

### 2026-11-15 14:33:34 — ANO-274 et WFS-553 : banc `score-fin` en retard sur WFS-513 (po-scrum-master)
- Signalement de Claude, agent dev3d (01Us6uCucP), pendant WFS-548 (PR #422) : `WFS_BANC=score-fin` donne 40 échecs identiques sur `main` et sur sa branche (code 3) ; 1er échec « appui sur le compteur → garage (RG-09) ». Il ne le corrige pas (hors de son ticket) : bonne décision.
- Constat du PO dans le code : le jeu fait ce qui était prévu (WFS-537 RG-09 : le compteur ouvre le garage **tant que** le Market n'existe pas ; WFS-513 fusionné, `menu.cpp` l. 107 ouvre le Market). Le banc attend encore `Ecran::Garage` et ne referme pas le Market : c'est le banc qui est en retard.
- Le banc `score-fin` n'apparaît pas dans `build.yml` ni `matin.yml` (recherche « score ») : CI de `main` et bancs du matin non touchés ; pas d'US P1. À confirmer par le développeur (RG-05).
- Créés : **ANO-274** (`anno.md`, 📋 Planifiée) et **WFS-553** `[TECHNIQUE]` (P2 Haute, E22, ⏳ Prêt, `@dev`, règles `regles-de-gestion/WFS-553.md`) : seul `src/game/score_fin_banc.cpp` change, aucun contrôle retiré. À donner à une **nouvelle session** `@dev` qui le demande (pas à 01Us6uCucP, qui garde son seul ticket WFS-548).


### 2026-11-15 14:51:13 — WFS-548 : PR #422 refusée par le lead dev (lead-dev)
- **Revue** du commit `be3919ed` : 152 captures ouvertes, tout le diff lu, branche synchronisée avec `main` construite et testée (145 tests CPU réussis, `test_verre` OK). Rendu bien fait (flou calculé une fois puis relu, net en course, 8 lignes dans le renderer).
- **Refus** (revue postée en commentaire, GitHub refusant « changements demandés » avec le même compte) : (1) l'interface de l'écran du dessous se voit à travers le verre (logo du menu derrière « RÉGLAGES », garage à travers le Market, menu à travers les CGU, écran de fin autour de la fenêtre d'achat) ; (2) le tableau « POSITION n/N » (titre cyan) reste au premier plan sur les fins Course à 5 et duel ; (3) capture « Tests CPU » et `tests-cpu.log` absentes. Demandé aussi : des captures « avant » des fins de course prises sur `main` avec les bancs `course5` et `menu2`.
- **Statut** : ↩️ Retour lead dev (PR #422) dans `TACHES.md`, `track-agent.md` et `suivi-pr.md`.

### 2026-11-15 15:22:36 — WFS-552 : PR #421 refusée par le lead dev (lead-dev)
Commit relu eb05307b. Avis agent 3D VALIDÉ ; 30 captures ouvertes, rendu conforme à la maquette 01, aucune forme nue, aucune régression vue. Branche synchronisée, construite et testée par le lead dev : 145 tests CPU réussis. Refus pour 2 points : (1) capture « Tests CPU » absente de la PR et `qa/pr/WFS-552/tests-cpu.log` manquant (règle du 2026-11-11) ; (2) `src/game/personnalisation.cpp:18` : `NOMS_SKINS` a 4 noms pour 5 skins, nom `nullptr` pour « ciel » (`std::string(o->nom)` dans `cadeau_jour.cpp:65` si la traduction manque). Remarques non bloquantes : réserves A à C de l'agent 3D, Golf un peu sous la console en 1280 × 720 (seuil de `test_menu_parking` trop large), triangles du parking au journal. Revue : https://github.com/wlfaleh/waforspeed_mobile/pull/421

### 2026-11-15 16:09:54 — WFS-552 : PR #421 corrigée après le retour du lead dev (Claude, agent dev3d (01Us6uCwZ7))
- **Refus du lead dev sur eb05307b** : (1) journal des tests CPU absent de la PR ; (2) 5e skin sans nom dans `NOMS_SKINS` (risque de plantage du cadeau du jour).
- **Corrigé dans 25107961** : nom « Bleu ciel » dans `src/game/personnalisation.cpp` (**accroche annoncée** : 1 ligne, plus les commentaires de `personnalisation.h` et de `garage_skin.h`) et contrôle « chaque objet du catalogue a un nom » dans `test_personnalisation` ; cadrage resserré : la voiture reste à droite de la console (bord gauche à x NDC ≥ −0,26 ; seuil du test −0,28, remarque du lead dev sur la Golf en 1280 × 720).
- **Tests CPU** sur 25107961 : 145 réussis, 0 échec (`qa/pr/WFS-552/tests-cpu.log`). Captures refaites (ecba9dfa). Décor : 37 692 triangles, 11 appels de dessin.
- **Avis de l'agent 3D** sur eb05307 : VALIDÉ avec réserves mineures (zones jaunes du capot et sous la plaque, qui sont des pièces d'origine du modèle ; vernis ; variation du béton du voile du fond) : tâche séparée à proposer au PO. Nouvel avis demandé sur ecba9dfa, puis lead dev.

### 2026-11-15 16:11:04 — WFS-541 : PR #423 ouverte, accroches annoncées (Claude, agent conduite (01UY1Vv1WU))
- **Fait** : voitures du trafic heurtées qui se redressent en roulant (≤ 15 km/h, demi-tour en plusieurs temps), retrait seulement hors de la vue après 8 s ; contacts entre voitures du trafic à chaque pas (emprises WFS-333) ; trafic écarté de la police sans la déplacer ; voitures qui passent à côté d'une accidentée hors de leur couloir ; changement de file au pas sans glisse de côté ; voiture qui contient la caméra non dessinée ; banc `trafic-541` (avant : `WFS_AVANT_541=1`).
- **Fichiers** : `src/game/traffic.{h,cpp}`, `src/game/traffic_system.{h,cpp}`, `tests/test_trafic_chocs541.cpp` (nouveau), `tests/test_penetration_voitures.cpp` (12 graines au lieu de 6, aucune borne changée), `CMakeLists.txt` (1 test).
- **Accroches hors zone (annoncées)** : `src/game/poursuite_system.cpp` (2 lignes : `separerDe` pour chaque voiture de police) ; `src/platform/desktop_main.cpp` (1 ligne : groupe `trafic-541`).
- **Vérification** : tests CPU 145 / 145 (branche synchronisée) ; banc CPU 200 chocs (Corniche, trafic × 2,5) : pénétration 0,000 m, arrêt max 7,9 s, 0,090 ms par image (main 0,074) ; carambolage : pénétration ≤ 0,032 m ; captures avant / après regardées (32 + tests).
- **Limites** : deux-roues couchés relevés seulement hors de la vue (comme avant) ; demi-tour d'une voiture tournée de plus de 90° jusqu'à 10 s ; après fusion 🟡 À valider (utilisateur).

### 2026-11-15 16:23:56 — WFS-552 fusionné par le lead dev (lead-dev)
PR #421 relue sur ecba9dfa (2e revue) : les deux défauts du refus sont corrigés (Tests CPU joints, nom « Bleu ciel »). Captures ouvertes, diff relu, construction et 145 tests CPU refaits (339 s, code 0), avis agent 3D « VALIDÉ ». Approbation postée en commentaire (même compte GitHub), fusion en squash 4594f3f7. Statut 🟡 À valider (utilisateur). Remarques non bloquantes : double commentaire `menu_parking_cadrage.cpp:12`, contact avec le bloc « Showroom » en 1280 × 720 (WFS-549), réserves A à C de l'agent 3D pour le PO.

### 2026-11-15 16:24:58 — Fin WFS-552 : session terminée (Claude, agent dev3d (01Us6uCwZ7))
- PR #421 fusionnée en squash par le lead dev (4594f3f7) après l'avis « VALIDÉ » de l'agent 3D sur ecba9dfa ; ticket 🟡 À valider (utilisateur) sur l'APK de `main` ; ligne `qa/Check.md` ajoutée par le lead dev.
- Réserves mineures transmises au PO pour une tâche séparée : (A) pièces jaunes d'origine de `sf90.wfm` (prises d'air du capot, crochet sous la plaque) qui ne prennent pas la skin ; (B) reflet du vernis trop concentré ; (C) béton du voile du fond trop uni (trous de banche, salissures) ; (D) en 1280 × 720 l'avant de la voiture touche le bloc « Showroom » (à revoir avec WFS-549) ; double commentaire `menu_parking_cadrage.cpp:12`.
- Un seul ticket par session : la session s'arrête, aucun autre ticket demandé.

### 2026-11-15 16:25:46 — WFS-554 créé : finitions du menu 3D de WFS-552 (po-scrum-master)
- Sur information de Claude, agent dev3d (01Us6uCwZ7) (WFS-552 fusionné #421, 4594f3f7, 🟡, session terminée) : réserves mineures de l'agent 3D et du lead dev regroupées dans **ANO-275** (mineure) et **une seule tâche séparée WFS-554** (P2 Haute, E23, ⏳ Prêt, nouvelle session `@dev` = dev-artiste-3d, avis `@3d` avant fusion, 🟡 après), ordre permanent « on ne jette pas un travail pour un petit défaut ». Règles : `regles-de-gestion/WFS-554.md` — RG-01 pièces jaunes de `sf90.wfm` peignables par la skin, RG-02 reflet du vernis élargi, RG-03 plus de liseré violet du caméléon (toutes les voitures caméléon), RG-04 béton du voile (trous de banche, joints, salissures), RG-05 cadrage 1280 × 720 sans toucher le bloc « Showroom » (dans le cadrage 3D, pas dans la console de WFS-549), RG-06 double commentaire de `menu_parking_cadrage.cpp`.
- Aucun ticket attribué (la session 01Us6uCwZ7 s'arrête, règle « un sujet par session »). Note ajoutée à `regles-de-gestion/WFS-552.md` : au moment de la validation de l'utilisateur sur l'APK, lui dire que le nouveau caméléon vaut pour toutes les voitures caméléon.

### 2026-11-15 16:31:11 — WFS-541 refusé par le lead dev (PR #423) (lead-dev)
- Revue du commit bf573504 : les 64 captures regardées (avant / après) ; tout le diff relu ; construction et tests CPU refaits sur la branche synchronisée avec `main` : 145 / 145 ; scène `bouchon` du banc `trafic-541` relancée par le lead dev.
- Bon : RG-01 (le SUV se redresse et repart), RG-03 (plus aucune voiture dans une autre, scène `pres`), coût CPU +0,02 ms.
- Refus : (1) dans le bouchon, une voiture reste à 0 km/h pendant ≈ 5,6 s en glissant de 3 m sur le côté (plancher de 0,6 m/s dans `step`, bloc `lent`) et le bouchon dure ≈ 7 s contre ≈ 2,4 s sur `main` ; (2) la voiture 2 hésite entre deux files ; (3) deux commentaires déplacés dans `traffic.h` ; (4) pas de test CPU du changement de file au pas ; (5) `qa/pr/WFS-541/tests-cpu.log` absent ; (6) les captures `suv-*` montrent une berline, pas un SUV.
- Statut : ↩️ Retour lead dev (PR #423) dans `TACHES.md`, `track-agent.md` et `suivi-pr.md`.


### 2026-11-15 17:23:27 — WFS-548 : PR #422 corrigée après le retour du lead dev (Claude, agent dev3d (01Us6uCucP))
- Fait : fenêtre de verre qui couvre l'écran du dessous (`verre::fenetreSurEcran` / `verre::couvert()`, système `verre-couvre` d'ordre 1) ; tableau « POSITION n/N » retiré des fins Course à 5 et duel ; remarques non bloquantes (commentaires, marge du clavier, test `contourArrondi`).
- Accroches hors zone (≤ 10 lignes chacune) : `src/ui/garage.cpp` (1 ligne : fenêtre d'achat seule quand elle couvre), `src/ui/ecrans/menu.cpp` (1 ligne), `src/ui/ecrans/compteur_etoiles.cpp` (1 condition), en plus des fins de course de `src/game/` déjà annoncées.
- Vérification : 146 tests CPU réussis sur `5b190750` (synchronisé avec `main`) ; captures avant / après refaites (188 paires, 1280x720) et regardées, dont les 8 paires demandées ; réglages relancés après l'arrivée de WFS-552.
- Commits : 897f8704, af74f8f6, 806b3672, debb6320, b3325221. Réponse point par point dans la PR ; lead dev rappelé.

### 2026-11-15 17:41:02 — WFS-548 fusionné par le lead dev (lead-dev)

PR #422 relue sur `b3325221` (2e revue) : les 3 défauts du refus sont corrigés (rien de l'écran du dessous sous le verre, tableau « POSITION » retiré des fins, section Tests CPU). Captures ouvertes, diff complet lu, branche construite et testée par le lead dev (146 tests réussis). Approbation en commentaire (même compte GitHub), fusion en squash `b06ce1a0`. Statut 🟡 À valider (utilisateur).

### 2026-11-15 17:41:35 — WFS-548 : fin de session après la fusion (Claude, agent dev3d (01Us6uCucP))
- PR #422 fusionnée en squash par le lead dev (b06ce1a0) ; statut 🟡 À valider (utilisateur), ligne de `qa/Check.md` écrite par le lead dev.
- Session terminée (un seul ticket par session) : aucun autre ticket demandé. À valider par l'utilisateur sur l'APK de `main` : fond flouté et panneaux en verre sur téléphone.

### 2026-11-15 18:31:21 — WFS-541 : corrections de la revue 1 (PR #423) (Claude, agent conduite (01UY1Vv1WU))
- **Corrigé** : (1) plus de déplacement de côté sans avancer (v · tan 35°), déboîtement au pas (7 km/h) derrière une voiture accidentée seulement ; (2) une seule décision de changement de file (les contacts ne l'annulent plus) ; (3) commentaires de `traffic.h` ; (4) cas D de `test_trafic_chocs541` (changement de file au pas : 0,000 m de côté à l'arrêt, 1,5 s d'arrêt au plus) ; (5) `qa/pr/WFS-541/tests-cpu.log` + capture « Tests CPU » ; (6) scène `heurtee` en M4 (le trafic n'a que 4 modèles, aucun SUV possible).
- **Cause racine notée** : une relance du changement de file toutes les 0,5 s cassait RG-02 de WFS-281 (`test_trafic_duree`) : retirée.
- **Vérification** : 147 / 147 tests CPU (branche synchronisée, WFS-548 compris) ; banc `trafic-541` régénéré, captures regardées ; réponses point par point dans la PR ; lead dev rappelé.

### 2026-11-15 18:49:08 — WFS-541 fusionné par le lead dev (lead-dev)
- PR #423, 2e revue sur aecbf199 : les 6 points de la 1re revue sont corrigés et vérifiés. « ✅ Approuvé par le lead dev » posté en commentaire (GitHub refuse l'approbation formelle : même compte). Fusion en squash : **eed11f17**.
- Captures ouvertes : 32 paires avant / après et la capture « Tests CPU ». Construction et tests refaits sur aecbf199 : **147 / 147** (cas D, `test_traffic`, `test_trafic_duree` verts).
- Statut : **🟡 À valider (utilisateur)** (ticket de conduite). Ligne ajoutée à `qa/Check.md`, ligne de la PR retirée de `suivi-pr.md`.
- Remarque non bloquante : la capture « Tests CPU » a été prise à 081f9e6e (146 tests), le journal `tests-cpu.log` à aecbf199 (147 tests).

### 2026-11-15 18:49:32 — Fin WFS-541 : fusionné, session terminée (Claude, agent conduite (01UY1Vv1WU))
- PR #423 fusionnée en squash par le lead dev (eed11f17) à la 2e revue ; statut 🟡 À valider (utilisateur) : à essayer sur l'APK de `main`, même parcours que les vidéos du 2026-11-13 (Poursuite, Corniche, nuit).
- Remarque du lead dev retenue : capture « Tests CPU » prise avant le dernier commit (146 tests) alors que `tests-cpu.log` est au commit final (147) ; à prendre au commit final la prochaine fois.
- Session terminée (un seul sujet par session) : aucun autre ticket demandé.

### 2026-11-15 20:40:54 — WFS-549 fusionné par le lead dev (lead-dev)
- PR #420 (commit relu f90e01bb) fusionnée en squash (c20f5c06) **sur ordre de l'utilisateur, après revue du code seul** (« Stop regarde que le code … si ok merge »). Le PR était pris 👀 depuis 20:34 par une autre instance du lead dev : l'ordre de l'utilisateur passe avant.
- Diff lu en entier : boutons communs `src/ui/boutons.*`, zones de toucher inchangées, seule `hud::BOUTON_ECHELLE` passe à 1,5 (voulu, test mis à jour) ; points 2 (Crédits au relâcher) et 4 (inactifs lisibles, `test_boutons`) de la 1re revue corrigés. Captures pas rouvertes ; construction locale arrêtée avant la fin (ordre de l'utilisateur) : la CI de `main` fait foi.
- Statut : 🟡 À valider (utilisateur). Ligne ajoutée à `qa/Check.md`, ligne #420 retirée de `suivi-pr.md`.


### 2026-11-15 20:43:28 — WFS-550 attribué à Claude, agent dev3d (01UY1Vv1yg) (po-scrum-master)
- Demande d'une nouvelle session `@dev` (session_01UY1Vv1ygW1sN5qWWTdaUJc, jamais de ticket ; « @dev go », pas de sujet souhaité).
- WFS-548 (#422) et WFS-549 (#420) fusionnés : **WFS-550 et WFS-551 passent ⏳ Prêt**. E23 est la plus récente demande de l'utilisateur (P1) : **WFS-550** (Interface R19 3/5 : cadrans fins, jauges, anneaux, 17 segments, compteur d'étoiles animé, bandeaux en course) attribué, plus petit numéro des deux.
- Zone : fichiers de RG-07 (`garage_fiche.*`, `garage_cadre.*`, `toasts.*`, `cadeau_jour*`, `pret_bandeau*`, `compteur_etoiles*`, `chargement*`, nouveau `src/ui/jauges.*`), `shaders/ui*`, `tests/**` ; `hud.*` seulement l'appel de la jauge de combo (≤ 10 lignes). Interface 2D : skill `design-street-racing` ; pas d'avis agent 3D. Taille des boutons inchangée.
- Critères : `regles-de-gestion/WFS-550.md` (RG-01 à RG-08, captures avant / après, parcours FR / EN). Après fusion : 🟡 À valider (utilisateur).
- `track-agent.md` : lignes des sessions terminées (01Us6uCucP, 01Us6uCvCC, 01Us6uCwZ7, 01UY1Vv1WU) enlevées. Restent ⏳ Prêt pour de nouvelles sessions `@dev` : WFS-551, puis WFS-554.

### 2026-11-15 20:48:39 — WFS-549 : revue complète du lead dev sur f90e01bb (lead-dev)
- Revue faite en parallèle de la fusion sur ordre de l'utilisateur (PR prise 👀 à 20:34) : même commit f90e01bb, même conclusion. Approbation « ✅ Approuvé par le lead dev » postée en commentaire sur la PR #420 (GitHub refuse APPROVE avec le même compte) ; squash c20f5c06.
- Construction Release et `bash tools/qa/tests_cpu.sh build` refaits : **148 test(s) réussi(s)** ; avertissements seulement dans du code déjà sur `main`. Banc `reglages-onglets` relancé : « Crédits » → ouvert, « Retour » → Réglages ouverts, 1er appui (doigt 0) → « appui compté ».
- Captures ouvertes : les 4 points du refus sur ce342a2e sont corrigés (tests-cpu.log, « Crédits » au relâcher, 185 captures des écrans touchés, inactifs lisibles). Remarques non bloquantes : « Inviter » jamais grisé hors ligne (comme sur `main`, description inexacte), quelques « avant » marqués « main à venir », commentaire en double dans `reglages.cpp`.
- `qa/Check.md` complété ; statut 🟡 À valider (utilisateur) inchangé.

### 2026-11-15 20:49:38 — Fin WFS-549 : fusionné (#420, squash c20f5c06), session terminée (Claude, agent dev3d (01Us6uCvCC))
- Boutons R19 sur tous les écrans (Start/Stop, autocollants, pilule chrome, prix, sélecteurs de verre, « ✕ Fermer », « Crédits » dans les Réglages), Menu et Caméra du HUD × 1,5 ; corrections de la revue #420 (tests-cpu.log, « Crédits » au relâcher, captures de tous les écrans, états inactifs lisibles) ; synchronisé avec WFS-548 et WFS-552.
- Statut : 🟡 À valider (utilisateur) ; demande de test ajoutée dans `tache utilisateur.md`. Restes notés par le lead dev (non bloquants) : « Inviter » hors ligne reste orange (phrase de la PR inexacte), quelques captures « avant » marquées « main à venir », un commentaire en double dans `reglages.cpp`.
- Session terminée : un seul ticket par session, plus aucune demande au PO.

### 2026-11-15 21:01:43 — Début WFS-551 : attribué à Claude, agent dev3d (01UY1Vv2KF) (po-scrum-master)
- **Ticket** : WFS-551 — Interface R19 (4/5) : gouttes de peinture, pastilles des skins, pistolet de l'utilisateur recoloré par finition, cadenas-jauge de la voiture bloquée, flèches qui clignotent, HUD Menu / Caméra en verre, gaz et frein sans texte, cartes grisées et couleur qui revient au téléchargement. Épic **E23** (P1 Urgent, plus récente demande de l'utilisateur).
- **Session** : nouvelle session `@dev` (session_01UY1Vv2KFV5cJpjafKsfwV5), à sa demande ; aucune attribution antérieure à cette session. Dépendances WFS-548 (#422) et WFS-549 (#420) fusionnées.
- **Zone de code** : `garage_couleur.*`, `garage_skin.*`, `garage_finition.*`, `garage_achat*`, `hud.*`, `terrain.*`, `paquets*` ; accroche ≤ 10 lignes dans `garage_fiche.*` (fichier de WFS-550) pour le cadenas-jauge ; icône `references/maquettes-R19/icone-pistolet-utilisateur.png` copiée dans `assets/` de l'interface ; `shaders/ui*`, `tests/**`. Réutiliser `src/ui/verre.*` (548) et `src/ui/boutons.*` (549).
- **Règles** : `regles-de-gestion/WFS-551.md` (RG-01 à RG-09, parcours complet FR et EN). WFS-550 est en cours en parallèle (01UY1Vv1yg) : le 2e fusionné se synchronise et refait ses captures.
- **Plan** : branche `feature/waforspeed-551` poussée tout de suite ; captures avant / après (`captures_pr.sh WFS-551 garage-couleurs garage-finitions garage-skins garage-voitures paquets hud:1280x720`, jour et nuit pour le HUD) ; PR ; appel du lead dev ; après la fusion **🟡 À valider (utilisateur)** ; arrêt de la session.

### 2026-11-15 22:43:41 — WFS-550 : PR #424 ouverte (Claude, agent dev3d (01UY1Vv1yg))
- Fait : nouveau `src/ui/jauges.{h,cpp}` (cadran 240°, anneau, barre fine graduée, barre d'étape, segments inclinés) ; garage : 3 cadrans fins, barres fines graduées, panneau de verre, 17 segments de la position sous le plateau ; chargement : étape + grand pourcentage, 4 étapes Moteur / Modèles 3D / Voitures / Ville (4 textes FR / EN) ; menu : pilule + anneau du cadeau du jour, voiture prêtée en pilule bordée de cyan avec anneau ; compteur d'étoiles animé après un gain (affichage seulement) ; bandeaux de course inclinés centrés (checkpoint cyan, points orange, accident / combo perdu rouge) ; jauge du combo.
- Accroche : `src/ui/hud.cpp`, 5 lignes (2 inclusions + jauge du combo, RG-07) ; `CMakeLists.txt`, 1 ligne (`test_jauges`).
- Vérification : construction complète ; 149 tests CPU verts (dont `test_jauges`, `test_langue`) ; bancs `garage-voitures`, `cadeau` (FR / EN, parcours RG-08, solde inchangé par l'animation), `chargement`, `hud` (nouvelle capture `toast-pile-nuit`) ; 23 captures avant / après dans `qa/pr/WFS-550/`.
- Limites : jauge « 1 250 / 6 000 » de l'achat (maquette 28) non faite (fichier `garage_achat*` de WFS-551, RG-07) ; cadenas au-dessus des segments dessinés par `garage_achat.cpp` (WFS-551) ; aucun message « police en approche » dans le jeu (le rouge sert aux accidents et combo perdu).


### 2026-11-15 22:53:35 — WFS-550 : PR #424 refusée par le lead dev (lead-dev)
- 23 captures ouvertes, tout le diff lu, branche construite (aucun avertissement) et `tests_cpu.sh` refait : 149 tests verts.
- Refus pour 2 points : (1) capture « Tests CPU » et `qa/pr/WFS-550/tests-cpu.log` absentes (règle du 2026-11-11) ; (2) à l'écran de chargement, le texte de l'étape et les 4 segments se contredisent (à 20 %, « Chargement du modèle 3D… » alors que « Modèles 3D » est coché), car les bornes de `jauges::avancementEtape` ne suivent pas les paliers de `Game::init`.
- Accepté : garage (cadrans, barres, 17 segments), anneaux du cadeau et du prêt, étoiles animées, bandeaux en course, jauge du combo. Jauge « 1 250 / 6 000 » de l'achat (maquette 28) laissée à WFS-551 (`garage_achat*`, RG-07) : à noter par le PO dans WFS-551.
- Revue dans la PR (commentaire, même compte GitHub). Commit relu cb0b89e9. Statut ↩️ Retour lead dev (PR #424).

### 2026-11-15 22:58:16 — Captures de PR allégées : règles 3 et 4, WFS-555 confié au lead dev (po-scrum-master)
- Demande du lead dev (session_01UY1Vv2AmFHmyurYWvyN3oV), avec l'accord de l'utilisateur qu'il cite (2026-11-15 ≈ 22:50 UTC : « Ok ca me vas ta proposition … on l'applique toutes de suite ») : tickets simples de 4 h 22 à 8 h 40, surtout à cause de `tools/qa/captures_pr.sh`.
- **Règles 3 et 4** écrites en bandeau ⏱️ de `TACHES.md` (dès maintenant, PR en cours comprises) : captures ciblées (écrans du ticket, une taille par défaut, 10 à 20 paires, minimum de 3 gardé) ; après un refus, seulement les captures des points corrigés.
- **CLAUDE.md et skills (`.claude/skills/lead-dev`, `agent-developer`) non modifiés par le PO** : un message d'agent ne suffit pas pour changer `CLAUDE.md` ou la configuration ; à faire par la session qui a reçu l'accord de l'utilisateur, ou après sa confirmation directe au PO.
- **WFS-555** `[TECHNIQUE]` P1 créé (`regles-de-gestion/WFS-555.md` : cache des captures « avant » par SHA de main, compilation de main persistante, rendu identique, temps mesurés et captures identiques au pixel) et **confié au lead-dev (session_01UY1Vv2AmFHmyurYWvyN3oV)** en renfort (règle du 2026-11-08 / 2026-11-13) ; zone `tools/qa/captures_pr.sh` seulement, gel de la CI respecté ; PR relue et fusionnée par une autre instance du lead dev.

### 2026-11-15 22:59:31 — Règles des captures ciblées écrites dans CLAUDE.md et les skills (lead-dev)
- Sur accord direct de l'utilisateur dans la session lead-dev (session_01UY1Vv2AmFHmyurYWvyN3oV) : « Ok ca me vas ta proposition … on l'applique toutes de suite ».
- Règles 3 et 4 (déjà au bandeau de `TACHES.md` par le PO, 155f29a0) recopiées dans `CLAUDE.md`, `.claude/skills/lead-dev/SKILL.md` (point 2 de la revue) et `.claude/skills/agent-developer/references/checklist-livraison.md` (case `captures_pr.sh`).
- Points 1 et 2 (cache) : WFS-555, en cours par le lead dev.

### 2026-11-15 23:04:32 — WFS-550 : PR #424 corrigée après le refus du lead dev (Claude, agent dev3d (01UY1Vv1yg))
- Point 1 : section « Tests CPU » et `qa/pr/WFS-550/tests-cpu.log` (149 réussis, 0 échec).
- Point 2 : segments du chargement alignés sur les textes annoncés (Moteur 0 à 20 %, Modèles 3D 20 %, Voitures 70 %, Ville 85 %) ; **accroche d'1 ligne dans `src/game/game.cpp`** (annonce à 70 % : « Chargement des voitures », avant le trafic, système 50) ; « Construction de la ville » annoncée à 85 % par un système d'ordre 99 de `chargement.cpp`, seulement pendant un chargement affiché ; clé `chargement.etape.voitures` FR / EN ; `test_jauges` : un cas par palier ; captures du chargement refaites (10, 20, 70, 85, 100 %).
- Commits 434b279e (corrections) et bdabd2b9 (synchronisation avec `main`) ; lead dev rappelé.

### 2026-11-15 23:13:06 — Revue des PR WFS-550 et WFS-551 : maquettes comparées au code (lead-dev)
- Ordre de l'utilisateur : « pour les PR 550 et 551 vous devez checker les maquettes aussi avec le code ». Ajouté au skill `lead-dev` (point 0 bis de la revue) et à `regles-de-gestion/WFS-550.md` et `WFS-551.md` : chaque maquette citée par les RG est ouverte et comparée au code et aux captures, une ligne par maquette dans la revue ; sinon pas de fusion. Message posté sur la PR #424 (WFS-550, en revue).


### 2026-11-15 23:14:02 — WFS-550 : 2e revue de la PR #424, retour lead dev (lead-dev)
Commit relu `bdabd2b9`. Les corrections du code sont acceptées : les segments du chargement suivent le texte affiché (bornes 0 / 0,20 / 0,70 / 0,85 / 1, annonce de la ville par le système 99 seulement pendant un chargement), il y a un cas `test_jauges` par palier, et l'orange vient de `ui::ORANGE`. Les 5 captures du chargement sont regardées (10, 20, 70, 85 et 100 %) : le segment en cours correspond au texte. La branche est construite et `tests_cpu.sh` lancé : 149 réussis. **Refus sur un seul point** : la description de la PR sur GitHub n'a pas été mise à jour. Elle n'a pas de section « Tests CPU » (la section n'est que dans `qa/pr/WFS-550/pr.md`), et son tableau des captures est l'ancien. Correction attendue : coller `pr.md` à jour dans la description, sans nouveau commit.

### 2026-11-15 23:14:20 — Début WFS-496 : atelier d'autocollants, attribué à Claude, agent dev3d (01Us6uCwHf) (po-scrum-master)
- Demande : nouvelle session `@dev` (session_01Us6uCwHfmVNHGT7xVcZKgC), « @dev go », sans sujet choisi ; aucun ticket déjà reçu par cette session.
- Choix : 1er ticket **P1** ⏳ Prêt permis à `@dev` = **WFS-496** (E18, priorité absolue 🚨, 9e du classement 📋 ; WFS-495 et 504 fusionnés). Passe avant WFS-554 et 553 (P2), cités par erreur dans les bandeaux précédents comme seuls restants.
- Zone : E8 `src/ui/ecrans/**` (+ `tests/**`) ; accroche ≤ 10 lignes dans `src/ui/garage.cpp` (annoncée au journal) ; réutiliser les calques et la texture d'habillage de WFS-492 / 495 et la variable `WFS_AUTOCOLLANTS` de `game.cpp`. Règles : `regles-de-gestion/WFS-496.md`. Pas d'avis `@3d` obligatoire (épic E18, pas E15).
- Restent ⏳ Prêt pour de nouvelles sessions `@dev` : WFS-503, WFS-512 (P1), puis WFS-554, 553 (P2).

### 2026-11-15 23:15:26 — WFS-550 : description de la PR #424 corrigée après la 2e revue (Claude, agent dev3d (01UY1Vv1yg))
- Description de la PR remplacée par `qa/pr/WFS-550/pr.md` à jour (section « Tests CPU », 26 captures dont le chargement à 10, 70 et 85 %) ; aucun nouveau commit de code (bdabd2b9) ; lead dev rappelé.

### 2026-11-15 23:15:29 — Début WFS-503 : cadeau du jour, gains réels, attribué à Claude, agent dev3d (01Us6uCwe9) (po-scrum-master)
- Demande : nouvelle session `@dev` (dev-artiste-3d, session_01Us6uCwe9onHeoCoMCPSWj3), « @dev go », jamais de ticket avant : demande acceptée (un seul sujet par session).
- Choix : WFS-496 venait d'être donné à la même minute à 01Us6uCwHf par une autre instance du PO ; 1er ticket **P1** ⏳ Prêt permis à `@dev` ensuite = **WFS-503** (E19, 11e du classement 📋 ; 501, 498, 487, 494 fusionnés). Une première attribution de WFS-554 (P2) préparée par cette instance n'a pas été envoyée (P1 avant P2).
- Zone : E3 `src/game/**` hors `game.*` (`cadeau.*`, portefeuille de WFS-498, catalogue de WFS-487, `skins::donnerSkin` de WFS-494), `tests/**` ; écran du cadeau (WFS-501) seulement par accroche ≤ 10 lignes annoncée au journal.
- Règles : `regles-de-gestion/WFS-503.md` (RG-01 à RG-05) et `regles-de-gestion/E19-cadeau-du-jour.md` ; captures ciblées FR / EN (règle du 2026-11-15) ; parcours menu → cadeau → garage → course → relance prouvé au journal ; PR puis appel du `lead-dev` ; arrêt de la session après la fusion.
- Restent ⏳ Prêt pour de nouvelles sessions `@dev` : WFS-512 (P1), puis WFS-554, 538, 553 (P2).

### 2026-11-15 23:17:05 — Arbitrage WFS-503 : badge « Nouveau » au garage → WFS-556 (po-scrum-master)
- Question de Claude, agent dev3d (01Us6uCwe9) : RG-02 de WFS-503 demande « Nouveau » au garage, qui n'existe pas et touche la zone E5.
- Décision : **option 2**. WFS-503 livre seulement les données en E3 (clé `cadeau-nouveaux`, `estNouveau` / `vu`, tests CPU) ; aucun fichier `src/ui/garage_*` ni `assets/textes/**`.
  Raison : `garage_couleur.*`, `garage_finition.*`, `garage_skin.*` sont tenus par WFS-551 (🔧) et la règle du 2026-11-13 est « un ticket à la fois par fichier de panneau ».
- **WFS-556** créé (P1, E19, zone E5, `@dev`, ⏳ En attente de WFS-503 et 551) : badge « NOUVEAU » / « NEW » et appel de `vu()` au choix. Règles : `regles-de-gestion/WFS-556.md` ; RG-02 et critères de `regles-de-gestion/WFS-503.md` mis à jour.

### 2026-11-15 23:18:17 — WFS-550 : 3e revue de la PR #424, refusée (lead-dev)
- Commit relu bdabd2b9. Point de la 2e revue corrigé : la description de la PR contient la section « Tests CPU » et les 26 captures.
- Nouvelle règle de l'utilisateur (23:13 UTC, maquettes comparées au code), appliquée pour la première fois : maquettes 13, 15, 08, 29 et 30 conformes ; écarts sur **05 et 26** (anneaux vides, sans icône de voiture ni de cadeau) et sur **17** (« Combo ×N » en cyan au lieu d'orange) ; la description de la PR ne cite pas fichier:fonction pour chaque maquette.
- Revue « changements demandés » postée en COMMENT (même compte GitHub). TACHES.md : ↩️ Retour lead dev (PR #424) ; suivi-pr.md : ↩️.
- **PO** : 3e refus sur cette PR (point 0 bis, nouvelle règle). À savoir pour WFS-551 : les cadenas au-dessus des segments gris (garage_achat.cpp) ne sont pas sur la maquette 15 ; RG-04 de 551 ne garde que le cadenas-jauge à côté du nom.

### 2026-11-15 23:27:23 — WFS-555 : PR #425 ouverte (lead-dev)
- Codé par le lead dev en renfort (ordre de l'utilisateur) ; seul `tools/qa/captures_pr.sh` change : cache des captures « avant » par SHA complet de main (hors du dépôt, témoin `.complet`, renommage, verrou `flock`, 3 commits gardés), arbre de main et `build/` gardés (compilation incrémentale), `SANS_CACHE=1` = ancien chemin.
- Mesures (banc `chargement` 1280x720, lavapipe, même conteneur) : ancien script 242 s ; nouveau cache vide 203 s ; **cache plein 29 s** ; `SANS_CACHE=1` 196 s ; **main a bougé 54 s** (nouveau dossier de cache, compilation incrémentale).
- Cache rendu octet pour octet (`cmp`) ; avec / sans cache : 596 à 857 pixels d'écart, dans la variation du banc lui-même (804 à 967 pixels entre deux lancements sans cache : mémoire affichée à l'écran).
- Tests CPU : 148 réussis (branche synchronisée). Installé dans le conteneur pour mesurer : `mesa-vulkan-drivers`, `glslc`, `libvulkan-dev` (apt, dépôts Ubuntu).
- Relecture et fusion : **une autre instance du lead dev** (appelée tout de suite).

### 2026-11-15 23:28:32 — WFS-551 : PR #426 ouverte, lead dev appelé (Claude, agent dev3d (01UY1Vv2KF))
- **Fait** (RG-01 à RG-07) : gouttes de peinture groupées par prix (config), pastilles de skin à motif, icône du pistolet de l'utilisateur teintée par finition, cadenas-jauge solde / prix à côté du nom d'une voiture bloquée, flèches du garage qui clignotent, HUD en verre (Menu / Caméra centrés, pédales et volant en verre sans texte, nitro anthracite, pause et vue caméra), cartes grisées avec retour de la couleur pendant le téléchargement, « Rouler » éteint sur une carte à télécharger, écran 18d restylé ; textes FR / EN.
- **Icône** : `references/maquettes-R19/icone-pistolet-utilisateur.png` (fournie par l'utilisateur, SHA-256 167444ee878cd00395b6b06a9d7d77a1c0c92e286fb3416e280b6100cf4cc117) réduite en 256 px dans `assets/ui/icone-pistolet.png` (SHA-256 5f69fa16290b92dff4edaa14d7366661f315af60b7c253c2721edbab1889f0ea) ; rangée dans les canaux G/B/A libres de l'atlas de police, nouveau mode du shader `ui.frag` (u ≥ 3,5). Aucun téléchargement Internet.
- **Accroches ≤ 10 lignes hors zone** : `src/ui/text.cpp` (rangement de l'icône dans l'atlas), `src/ui/garage_fiche.cpp` (cadenas-jauge après le nom, fichier de WFS-550), `src/ui/garage.cpp` (clignotement des flèches). Fichiers du HUD hors liste RG-08, touchés pour RG-06 : `src/game/commandes_tactiles.cpp`, `src/game/direction.cpp`, `src/ui/ecrans/pause.cpp`, `src/game/camera_vue.cpp`.
- **Décisions** : groupes de couleurs lus dans `configgame.json` (Classiques et Vives existent aujourd'hui) ; carte « voiture bloquée » retirée hors attente (remplacée par le cadenas-jauge) ; banc `paquets` : le contrôle de WFS-306 qui roulait sur la Corniche supprimée vérifie maintenant « Rouler » éteint (RG-07), et le secours par le code reste vérifié par la demande directe (comme `bench_changement_carte.cpp`).
- **Vérifié** : construction sans avertissement ; tests CPU : 149 réussis (branche synchronisée), dont `test_icones_r19` (nouveau) et `test_hud_boutons` (RG-06) ; bancs `garage-couleurs`, `garage-voitures`, `paquets`, `hud:1280x720` : 0 écart sur la branche et sur `main` ; 21 captures avant / après dans `qa/pr/WFS-551/`, regardées.
- **Parcours RG-09 (FR / EN)** : menu → garage (Peinture, Finition, Skins, voiture bloquée, acheter : captures `garage-couleurs-*`, `garage-voitures-*`, versions `-en-`) → choix de la carte (télécharger, progression, installée, rouler : `paquets-terrain-*`) → course (Menu, Caméra, gaz, frein, nitro : `hud-*`) → pause → retour ; aucun écart aux bancs.
- **Limites** : pas de banc séparé Finition / Skins (visibles sur les captures du garage) ; rendu Android non vu ici ; après fusion : 🟡 À valider (utilisateur).

### 2026-11-15 23:39:13 — WFS-551 : PR #426 refusée par le lead dev (lead-dev)
- Revue du commit ec034448 : 21 paires de captures ouvertes, tout le diff lu, construction refaite et 149 tests CPU OK. Refus aussi demandé par l'utilisateur (via le coordinateur) : le garage ne reprend pas les maquettes 06, 07, 16, 15 et 18d.
- Maquettes : 17/17b (HUD) et 18a-c (cartes grisées, téléchargement, installée) conformes ; écarts sur 06 (gouttes petites, sans panneau de verre), 07 (pastilles 32 px, nom à côté, aperçu qui cache « AUCUNE »), 16 (pistolets de 34 px, sans panneau ; Chromé et Caméléon jamais capturés), 15 (« ★ solde / prix » sous Retour au lieu de sous Acheter ; petits cadenas des points en trop), 18b (texte « Tu peux rouler pendant ce temps » alors que « Rouler » est éteint), 18d (fond = menu assombri au lieu de la scène floue).
- Manquent : les bancs `garage-finitions` et `garage-skins` (ils existent, la PR dit le contraire), la section « Tests CPU » et `qa/pr/WFS-551/tests-cpu.log`, et la citation fichier / fonction pour chaque maquette.
- Refus posté en commentaire sur la PR (GitHub n'accepte pas « changements demandés » quand la PR est ouverte avec le même compte). Statut : ↩️ Retour lead dev (PR #426).

### 2026-11-15 23:46:35 — WFS-550 : ordre de l'utilisateur, maquettes appliquées à 100 % (Claude, agent dev3d (01UY1Vv1yg))
- Ordre de l'utilisateur dans la session : « ta maquette tu l'appliques à 100 %, zéro erreur ». Il passe avant la RG commune de E23 « taille des boutons et textes inchangés » pour les écrans de WFS-550.
- Refait (commit ec2db1c1) : cartes « Cadeau du jour » et « Voiture prêtée » aux mesures de la maquette 26 (nouvelles clés FR / EN cadeau.carte.*, pret.carte.*), bandeaux de course de la maquette 30 (droits, fondus, titre sombre, valeur blanche), combo de la maquette 17 (capitales orange, barre orange), cadrans du garage plus grands (13), barre arrondie du chargement (08), pilule arrondie du compteur d'étoiles (26, 29). Accroche dans `boutons.*` retirée (icône de voiture vue de face dessinée dans `pret_bandeau.cpp`).
- Hors de mes fichiers et absent des maquettes : colonne Market / Profil / Crédits du menu (les cartes se posent à sa gauche) ; onglets du garage (13) et cadenas (15) : WFS-551.

### 2026-11-15 23:48:02 — WFS-557 créé : vérification à 100 % du travail R19 fusionné contre les maquettes, et corrections (po-scrum-master)
- Demande de l'utilisateur, une seule tâche. RG : `regles-de-gestion/WFS-557.md`.

### 2026-11-15 23:49:27 — WFS-557 limité à 548, 549, 552 ; 550 et 551 refusés s'ils ne respectent pas les maquettes R19 à 100 % (po-scrum-master)


### 2026-11-15 23:51:31 — WFS-555 : PR #425 refusée par le lead dev (lead-dev)
- Revue par une autre instance du lead dev (le codeur était le lead dev en renfort), commit relu 1a88fae6, revue en commentaire (même compte GitHub).
- Vérifié : 5 paires `chargement` ouvertes (identiques sauf la ligne « Mémoire »), diff entier, construction + 148 tests CPU (code 0), script relancé (199 s cache vide ; 56 s et 54 s après changement de main, compilation incrémentale), ménage à 3 commits. Critère « identique au pixel » accepté tel que prouvé (cache octet pour octet ; banc non déterministe) : à réécrire par le PO.
- Défauts : (1) `preparer_main_src` peut faire `checkout --force --detach` dans le dépôt du développeur si l'arbre de main est cassé et le cache dans le dépôt (reproduit : branche quittée, modification perdue) ; (2) `WFS_CACHE_CAPTURES` relatif accepté : arbre de main créé dans le dépôt, « avant » absents, code 0 ; (3) arbre verrouillé non recréé (`fatal: missing but locked worktree`).
- Remarque au PO : main bouge souvent par des commits de documents, le cache par SHA se relit rarement ; le gain vient surtout de la compilation incrémentale (≈ 200 s → ≈ 55 s).

### 2026-11-15 23:55:21 — WFS-557 attribué à Claude, agent dev3d (01UY1Vv4Fd) (po-scrum-master)
- Ordre de l'utilisateur : « @dev prend le WFS-557 ». Nouvelle session `@dev` (session_01UY1Vv4Fd1hq8E2LRsGZhgL), aucun ticket avant : attribution permise (un sujet par session).
- Ticket : P1, E23, vérifier que WFS-548, 549 et 552 fusionnés respectent à 100 % les maquettes `references/maquettes-R19/`, et corriger. Règles : `regles-de-gestion/WFS-557.md` (RG-01 à RG-05).
- Zone autorisée : les fichiers fusionnés de WFS-548, 549 et 552 (E5 `src/ui/**`, `shaders/ui*` ; E8 `src/ui/ecrans/**` ; menu 3D `menu*`, `garage_decor.*`, modèles du parking), plus `qa/pr/WFS-557/`. Interdit : les éléments de WFS-550 et 551 (en cours, RG-03).
- Plan : captures 2400 × 1080 côte à côte avec chaque maquette, tableau `qa/pr/WFS-557/verification.md`, corrections, PR, avis agent 3D pour le menu 3D, lead dev (comparaison maquette ↔ code). Après fusion : 🟡 À valider (utilisateur).

### 2026-11-15 23:59:57 — WFS-555 : PR #425 corrigée après le retour du lead dev (lead-dev)
- 3 défauts de la revue sur 1a88fae6, corrigés dans c157d394 : (1) checkout seulement si l'arbre de main est sa propre racine git (plus jamais le dépôt parent) ; (2) cache rendu absolu, refus clair (code 2) s'il est dans le dépôt ; (3) arbre verrouillé déverrouillé puis recréé.
- Essais refaits (`qa/pr/WFS-555/essais-revue-2.log`) : dépôt parent intact (branche et modification gardées), `.cache-rel` refusé sans rien créer, arbre verrouillé recréé ; 2 lancements normaux de `chargement` à code 0 (186 s, 188 s).
- Réponses point par point dans la PR ; branche synchronisée (c19a70c4) ; autre instance du lead dev rappelée.

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
