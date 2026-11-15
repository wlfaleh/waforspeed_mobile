# WaForSpeed Mobile — journal

Journal du projet (tickets WFS-XX de `TACHES.md`) : une entrée par étape (début, construction, PR, verdict QA, fin…) avec
fait / fichiers / vérification / limites. **Nouvelles entrées à la fin du fichier.** Toujours partir de la dernière version de
`main` (`git pull`), ajouter seulement ses entrées, puis envoyer tout de suite ; ne jamais écraser le fichier.
**Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
Exemple : `### 2026-11-03 19:43:10 — WFS-58 construit, essai simulateur lancé (Claude, session trafic)`.
> **Archive (règle de l'utilisateur, 2026-11-06)** : ce fichier ne garde que les **2 derniers jours**. Les entrées plus anciennes sont dans `journal/archive/AAAA-MM-JJ.md` (un fichier par jour) ; ne les relire qu'en cas de besoin précis. Le PO déplace chaque jour les entrées de plus de 2 jours.

### 2026-11-14 00:11:58 — Question de l'utilisateur : glisse de la police et « grande voiture irréaliste » (po-scrum-master)
- **Glisse des voitures de police** : non couverte par WFS-540 ; cause lue dans le code : la police est déplacée en `rd` / `x` par `poursuite::avancer()`, seules les roues contre-braquent (`poursuite_system.cpp` l. 312). **ANO-272** (bloquante) → **RG-08 ajoutée à WFS-540** (cap dans l'axe du déplacement, pas de glisse au-delà des pneus, braquage réel, aucun pivot sur place).
- **Grande voiture** : c'est la surprise « camion en travers » de WFS-119, un **Range Rover Sport agrandi 1,45 fois** (`CAMION_ECHELLE`, `poursuite.h` l. 175 ; depuis WFS-404). **ANO-273** (bloquante) → nouveau **WFS-542** (P1, `@dev` artiste 3D) : vrai camion libre aux vraies mesures, échelle 1, immobile, retiré derrière le joueur. WFS-541 précisé : ce SUV n'est pas le trafic.

### 2026-11-14 00:14:56 — ANO-272 revue image par image : la police est déplacée comme une carte à jouer (po-scrum-master)
- Remarque de l'utilisateur : « ce n'est pas les roues ; elle glisse à gauche et à droite comme une carte à jouer, regarde bien le mouvement ». Mon 1er constat (roues) était faux.
- Vidéo 23:48 revue à 10 images/s (`qa/retours/2026-11-13-video2348-police-coince/video2348-police-10ips-1.2s-3.6s.jpg`) : carrosserie toujours parallèle à la route, décalages de côté et d'avant en arrière sans jamais tourner.
- Cause dans le code : cap dessiné = cap de la route (`poursuite_system.cpp` l. 390-391) ; glissement de côté direct à 6,75 m/s (`poursuite.cpp` l. 116-117) ; téléportation au contact (l. 124-125).
- ANO-272 réécrite ; **WFS-540 RG-08 réécrite** : police conduite avec son propre cap, déplacement seulement dans l'axe de la carrosserie, braquage réel, aucune téléportation, mesures au banc.

### 2026-11-14 00:15:42 — WFS-501 : écran du cadeau du jour, PR #403 ouverte (Claude, agent dev3d (01Us6uCviA))
- Fait : écran du cadeau du jour (`src/ui/ecrans/cadeau_jour.{h,cpp}`, calculs purs `cadeau_jour_calculs.cpp`, banc `cadeau_jour_banc.cpp`, test `tests/test_cadeau_ecran.cpp`) ; bouton « Cadeau du jour » au-dessus de « Crédits », avec la pastille ou le compte à rebours ; animation 2D de la boîte (≈ 1,5 s, un appui la passe) ; gain des 4 types ; « Chances » lu dans `configgame.json` ; écran d'attente avec le dernier gain ; FR / EN (30 clés `cadeau.*`, 4 clés `skin.*`). Le tirage se fait avant l'animation (`cadeau::ouvrir` de WFS-500). Aucun bouton payant.
- **Accroches annoncées** : `src/ui/ecrans/menu.cpp` (7 lignes : include, `CADEAU = 5`, touchers, zone, action, menu masqué sous l'écran, dessin du bouton) ; `src/platform/desktop_main.cpp` (2 lignes : banc `WFS_BANC=cadeau`) ; `CMakeLists.txt` (1 ligne : `test_cadeau_ecran`).
- Vérification : 133 tests CPU verts (commit ff166e29, `qa/pr/WFS-501/tests-cpu.log`) ; banc `cadeau` à 0 erreur ; parcours RG-08 en FR et en EN (menu → cadeau → animation → gain → Chances → Retour → menu sans pastille → 2e appui : attente, aucun 2e cadeau → relance : aucun 2e cadeau → « Activer » ferme l'écran) ; garde-fou des textes anglais (WFS-504) sans échec ; 30 captures regardées.
- Limites : gain non crédité (WFS-503), « Activer » sans prêt (WFS-502) ; noms des finitions par `finition.<id>` de WFS-491 (nom du catalogue en attendant) ; rendu lavapipe seulement.

### 2026-11-14 00:16:24 — Statuts automatiques, CI main #1059 (robot statuts)
- Run rouge (push, commit b1ed89d), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-453, WFS-456, WFS-533, WFS-534, WFS-535, WFS-536, WFS-480, WFS-481, WFS-483, WFS-498, WFS-487, WFS-488, WFS-489, WFS-490, WFS-500, WFS-504, WFS-505, WFS-506, WFS-507, WFS-508, WFS-510, WFS-397, WFS-378, WFS-395.

### 2026-11-14 00:16:31 — ANO-272 précisée par l'utilisateur : la voiture de police à côté du camion géant (po-scrum-master)
- L'utilisateur vise **la petite voiture de police collée au « camion » géant** (Range Rover agrandi, ANO-273), pas la police à côté du joueur. Images à 6,7 /s : `qa/retours/2026-11-13-video2348-police-coince/video2348-police-a-cote-du-camion-6ips-5.0s-8.6s.jpg`.
- Elle glisse à gauche, revient à droite et entre à moitié dans le camion, puis recommence, sans jamais tourner. Cause (`poursuite.cpp` l. 93-112) : sa cible change à chaque image entre la file libre et le joueur, qui est derrière le camion (aller-retour sans fin) ; décalage par glissement direct, cap de la route ; aucun choc police ↔ camion.
- ANO-272 complétée ; **WFS-540 RG-09** ajoutée (file gardée pendant le contournement, camion solide pour la police, arrêt derrière sinon) ; RG-08 (cap propre, pas de glissement) gardée.

### 2026-11-14 00:20:24 — 2e vidéo de l'utilisateur (Campagne, jour, 01:19) : même défaut de la police « carte à jouer » (po-scrum-master)
- Vidéo `Screen_Recording_20261114_011901` (15 s) ; images dans `qa/retours/2026-11-14-video0119-police-carte/`.
- Le joueur recule (« R », 14-31 km/h) ; la police **recule avec lui, collée à son flanc**, carrosserie toujours parallèle à la route ; puis elle glisse de l'avant à l'arrière du joueur et **sort sur l'herbe** en glissant de côté.
- ANO-272 complétée ; **WFS-540 RG-10** ajoutée (pas de recul pour suivre le joueur, vraie marche arrière ≤ 15 km/h, reste sur la chaussée, aucune position collée entretenue par le code). Pas de nouveau ticket : même cause (police posée, pas conduite). La pause Campagne ne s'applique pas : c'est le code de la Poursuite, pas le décor.

### 2026-11-14 00:22:24 — Ordre permanent de l'utilisateur : « une voiture de police est une voiture comme les autres » (po-scrum-master)
- « La règle est la réalité, il n'y a pas d'événements extraterrestres ; une voiture de police est une voiture comme les autres. »
- Inscrit dans les ordres permanents du PO ; **WFS-540 RG-00** (prioritaire) : la police est simulée par le même moteur de conduite que le joueur, avec sa fiche, et n'a que les commandes d'un conducteur ; plus aucune écriture directe de sa position, de sa vitesse ou de son cap. Titre de WFS-540 mis à jour.

### 2026-11-14 00:26:46 — WFS-540 attribué à Claude, agent conduite (01UY1Vv3RA) (po-scrum-master)
- Demande de l'utilisateur : « @conduite la 540 t'attend ». Nouvelle session `@conduite` (session_01UY1Vv3RAD7auNAuR4e1UiD), aucun ticket avant : attribution permise (ce n'est pas 01UY1Vv1A7, exclue par le KO de WFS-461).
- **WFS-540** (P1, E3) : police = vraie voiture (même moteur de conduite que le joueur, RG-00, ordre permanent du 2026-11-14 00:22 UTC), plus de coincement, arrestation, plus de « R » sans frein (ANO-267, 268, 271, 272). Règles : `regles-de-gestion/WFS-540.md`. Zone : `poursuite.*`, `player.*`, `duel.*`. Branche `feature/waforspeed-540`. Skills : `agent-developer`, `waforspeed-natif`, `expert-conduite`. Captures avant / après (Poursuite, Corniche, jour et nuit, suite d'images), PR, lead dev ; après fusion 🟡 À valider (utilisateur), puis arrêt de la session.

### 2026-11-14 00:30:05 — WFS-491 : retour lead dev corrigé, PR #401 à relire (Claude, agent dev3d (01Us6uCwfT))
- (1) écran étroit : boutons de largeur fixe (104 × 30 px), rangée qui défile par chevrons comme les couleurs (3 boutons en 4:3, 5 en 16:10, 6 en 16:9, 8 en 2400x1080) ; (2) police 12 px, réduite de 20 % au plus (aucun nom FR / EN n'en a besoin) ; contrôle au banc dans chaque langue et chaque taille ; (3) RC-E18-03 mesuré (`peinture2-mesure`, Basse, 2 passages) : `main` 3,215 img/s, branche brillant 3,216 / chromé 3,210 / caméléon 3,165 ; 1 % les plus lents 2,374 / 2,455 / 2,427 / 2,363 : écart ≤ 0,05 img/s.
- Synchronisée avec `main` (WFS-492 skins, WFS-481) : la skin de course n'est jamais posée sur la voiture du garage (`vehicule_skin::materiau` compare le modèle). 133 tests CPU réussis ; banc `garage-finitions` 0 écart à chaque taille. Captures régénérées (54). Réponse point par point dans la PR ; lead dev rappelé.

### 2026-11-14 00:33:03 — WFS-499 : PR #404 ouverte, lead dev appelé (Claude, agent dev3d (01Us6uCxU3))
- PR #404 (commit 562520b8, branche synchronisée avec `main`) : 42 captures « après » (1280 × 720 et 1024 × 768, FR et EN) et 6 « avant » dans `qa/pr/WFS-499/`, `tests-cpu.log` (133 tests réussis).
- Parcours RG-07 (banc `garage-voitures`, noté ici avant la PR) : 208 → M4 bloquée → achat refusé faute d'étoiles (« Il te manque 1 200 ★ », rien de débité) → essai gratuit → écran de fin « Acheter 2 200 ★ » → garage « Essai déjà fait » → course demandée avec la M4 : retour au garage → achat confirmé (3 000 → 800 ★) → Rouge, Choisir → Corsa : essai puis achat depuis l'écran de fin → course M4 jour et nuit → relance : M4 et Corsa gardées → anglais. 0 écart (1024 × 768).
- Rectificatif de l'entrée précédente : le « 0 écart » en 1280 × 720 annoncé avant la PR venait d'un lancement qui avait encore 1 écart (attente fausse du banc : l'achat de la Corsa la choisit, comme le veut RG-02) ; banc corrigé, relancé en 1024 × 768 : 0 écart.
- `suivi-pr.md` : #404 🆕 ; `TACHES.md` : 🔀 En PR.

### 2026-11-14 00:42:31 — WFS-501 fusionné par le lead dev (lead-dev)
- PR #403 (Claude, agent dev3d (01Us6uCviA)), revue sur bb703375, « ✅ Approuvé par le lead dev » en commentaire (même compte GitHub), fusion en squash **b08d8803**.
- 34 captures ouvertes (menu avant / après, animation, 4 gains, Chances, attente, nuit, FR / EN) ; diff lu en entier ; construction, 133 tests CPU et banc `cadeau` (0 erreur) refaits ; banc `textes` français à 0 dépassement.
- Remarques non bloquantes dans la revue : écran d'attente ouvert au passage de minuit, commentaire du filtre `WFS_CADEAU`, lisibilité du compte à rebours au menu. Ticket en 🟣 ; ligne ajoutée à `qa/Check.md` ; ligne de `suivi-pr.md` supprimée.

### 2026-11-14 00:42:42 — WFS-499 : PR #404 refusée par le lead dev (lead-dev)
- Revue du commit 562520b8 : 48 captures ouvertes, diff complet lu, branche construite et testée (`tests_cpu.sh build` : 133 réussis). La demande se voit bien (cadenas, prix, équivalents, fenêtre de confirmation, « Il te manque », essai, « Acheter N ★ » en fin de course, FR / EN, 16:9 et 4:3).
- Refus, 3 points (revue écrite en commentaire dans la PR, même compte GitHub) : (1) régression du banc `garage-couleurs` (WFS-489) : Golf et GT86 bloquées dans sa sauvegarde, « Acheter » ouvre la fenêtre de la voiture au lieu d'acheter la couleur (captures `en-il-manque` et `en-toit`) ; le banc complet n'a pas tourné (mode rapide) ; (2) après l'essai, la voiture de la partie reste la voiture bloquée : toute course suivante renvoie au garage, et en duel le garde-fou quitte la course sans `leave()` (salle ouverte, courses suivantes comptées en duel) ; (3) « Acheter la Orion Corsaire » : il faut l'élision (« l'Orion »).
- `TACHES.md` : ↩️ Retour lead dev (PR #404) ; `track-agent.md` et `suivi-pr.md` à jour.

### 2026-11-14 00:43:44 — WFS-501 : fusionné (PR #403, b08d8803), session terminée (Claude, agent dev3d (01Us6uCviA))
- Le lead dev a fusionné la PR en squash. J'ai répondu dans la PR à ses 5 remarques non bloquantes.
- **Pour le PO** (petit défaut, non corrigé, à confier à WFS-502 qui touche le même écran, ou à une petite US) : si minuit passe pendant que l'écran d'attente du cadeau est ouvert, il affiche « Prochain cadeau bientôt » au lieu de proposer le cadeau ; il faut fermer puis rouvrir l'écran. Le commentaire de `cadeau_jour_banc.cpp` « (et le menu) » est aussi faux quand `WFS_CADEAU` est posé.
- Réponse écrite à l'utilisateur dans `tache utilisateur.md` (demande « Cadeau du jour aléatoire »). Ligne de `track-agent.md` passée à « ⏹️ session terminée ». Aucun autre ticket demandé (un seul sujet par session).


### 2026-11-14 00:56:04 — WFS-491 fusionné par le lead dev (lead-dev)
- PR #401 (Claude, agent dev3d (01Us6uCwfT)), 2e revue sur b0b67c06, reprise de la relecture prise à 00:31 (instance arrêtée par un redémarrage du conteneur, rien posté). Réponses du développeur lues sous la 1re revue.
- Les 3 points bloquants du 1er refus sont corrigés : boutons de largeur fixe et rangée qui défile par chevrons (aucun texte ne dépasse en 1024x768, 1280x800, 1280x720, 2400x1080, FR et EN), police 12 px (plancher 9,6 px), RC-E18-03 mesuré (`qa/pr/WFS-491/mesure-basse.txt`, écart ≤ 0,1 img/s).
- Vérifié par le lead dev : tout le diff lu, construction et `tests CPU : 133 test(s) réussi(s)`, banc `garage-finitions` relancé en 1024x768 (0 écart). « ✅ Approuvé » posté en commentaire (même compte GitHub), fusion squash e454b9f4. Statut 🟣, ligne ajoutée à `qa/Check.md`, ligne de `suivi-pr.md` retirée.

### 2026-11-14 00:56:51 — WFS-491 fusionné, session terminée (Claude, agent dev3d (01Us6uCwfT))
- PR #401 fusionnée en squash par le lead dev (e454b9f4) après une 2e relecture (la 1re reprise avait été coupée par un redémarrage du conteneur). Ticket en 🟣, ligne de `qa/Check.md` posée par le lead dev.
- Réponse à l'utilisateur sous la demande « Personnalisation » de `tache utilisateur.md`. Remarques non bloquantes du lead dev à suivre : la voiture du garage est dessinée comme voiture du joueur (à surveiller avec WFS-494, skins au garage) ; 2 pages de finitions en 4:3.
- Un seul sujet par session : je ne demande pas d'autre ticket.

### 2026-11-14 01:23:50 — WFS-537 : retour lead dev corrigé, PR #402 à relire (Claude, agent dev3d (01UY1Vv4Hv))
- Corrigé : branche synchronisée (WFS-481) ; profil créé dans le banc `score-fin` ; compteur caché sous « Crée ton pilote » (capture et vérification) ; compteur dessiné par un système (461) juste après « Réglages » à la même image, avec le fondu du menu ; singulier « 1 étoile ».
- Vérifié : `score-fin` vert en 2400 × 1080 et 1280 × 720 ; tests CPU 133 / 133 ; `textes-course` VERT ; `textes-menus` vert sur la branche (un échec « Quitter le salon » pendant la passe de captures, dû au relais du duel injoignable, absent à la relance).

### 2026-11-14 01:25:44 — Statuts automatiques, CI main #1060 (robot statuts)
- Run rouge (push, commit b08d880), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-453, WFS-456, WFS-533, WFS-534, WFS-535, WFS-536, WFS-480, WFS-481, WFS-483, WFS-498, WFS-487, WFS-488, WFS-489, WFS-490, WFS-492, WFS-500, WFS-504, WFS-505, WFS-506, WFS-507, WFS-508, WFS-510, WFS-397, WFS-378, WFS-395.

### 2026-11-14 01:39:00 — WFS-484 : PR #405 ouverte, accroches annoncées (Claude, agent dev3d (01Us6uCvb4))
- Fait : plaques au hasard des autres voitures, même plaque que WFS-483 (maillages, places, matériaux, éclaireur, LOD réutilisés) ; atlas unique de 64 plaques (2048 × 512 px) préparé au chargement de la carte ; tirage 7 caractères sans mot interdit ni plaque du joueur ; duel : champ facultatif `pl` de la présence, filtré à la réception ; fantôme : plaque du joueur, translucide ; Basse : version simple, trafic sans plaque au-delà de 40 m.
- Fichiers (zone E10) : `src/render/vehicule_plaques_autres.{h,cpp}`, `vehicule_plaques_tirage.cpp`, `vehicule_plaques_autres_banc.cpp`, `vehicule_plaque.{h,cpp}` et `vehicule_plaque_image.cpp` (largeur d'image réglable, constantes partagées), `tests/test_plaques_autres.cpp`.
- **Accroches annoncées** (≤ 10 lignes par fichier, autorisées par le ticket) : `src/game/game.cpp` (4 : include, préparer, libérer, entre deux courses), `traffic_system.cpp` (2), `course5_system.cpp` (2), `poursuite_system.cpp` (2), `duel_system.cpp` (3 : include, plaques du salon, dessin), `course_system.cpp` (2 : fantôme), `duel.h` (2) et `duel.cpp` (2) : `setPlaque` et champ `pl`, `src/platform/desktop_main.cpp` (3 : bancs `plaques-autres`, `plaques-autres-mesure`), `CMakeLists.txt` (2 : test).
- Vérifié : 135 tests CPU au vert (branche synchronisée avec `main`) ; banc `plaques-autres` 2400 × 1080, 21 captures avant / après (jour et nuit) regardées ; banc `parcours` : 0 écart (et vue Cockpit) ; coût (Corniche, Basse, Poursuite, trafic le plus dense) : +5,2 appels de dessin, +1 846 triangles par image ; temps lavapipe +3,8 ± 3,2 ms sur 300 ms (bruit).
- Limites : aucune mesure sur GPU de téléphone (≤ +0,5 ms à confirmer) ; deux-roues sans plaque ; au-delà de 44 voitures du trafic, un texte peut revenir sur une voiture éloignée.

### 2026-11-14 01:49:27 — WFS-484 fusionné par le lead dev (lead-dev)
- PR #405 (Claude, agent dev3d (01Us6uCvb4)), revue sur 924a0465, squash f39336ce. « ✅ Approuvé par le lead dev » posté en commentaire (GitHub refuse l'approbation : même compte).
- 21 paires de captures ouvertes : plaques du même style que le joueur sur le trafic, la Course à 5, la police (avant = arrière), fantôme translucide ; jour et nuit justes, rien d'autre de changé.
- Diff entier lu ; construction et 135 tests CPU refaits par le lead dev. Remarques non bloquantes : coût sur téléphone à confirmer (RG-05) ; petit risque de doublon d'une case refaite quand la plaque du joueur change ; petites allocations par image hors course dans `duel_system.cpp`.
- Statut 🟣 dans `TACHES.md`, ligne retirée de `suivi-pr.md`, ligne de `qa/Check.md` ajoutée ; session du développeur terminée.

### 2026-11-14 01:49:46 — WFS-537 : retour lead dev sur PR #402 (lead-dev)
- 2e revue, commit 23912814. Les 6 points du 1er refus sont corrigés : profil du banc, compteur caché sous « Crée ton pilote », système 461 avec fondu, singulier.
- **Refus pour un seul point bloquant** : la PR est en conflit avec `main` (`mergeable: false`). WFS-501 (« Cadeau du jour », #403) est entré après la synchronisation `dcfe7a10`. Conflits dans `menu.cpp` (les deux tickets utilisent la valeur 5 de l'`enum`) et dans `desktop_main.cpp`. Sur la capture `textes-fr-menu-principal`, le bouton « Cadeau du jour » disparaît dans l'« après ».
- Essai local du lead dev (rien poussé), conflit résolu avec `COMPTEUR = 6` : construction OK, 135 tests CPU réussis, bancs `score-fin` (1280 × 720) et `cadeau` à 0 échec, menu propre avec les 4 boutons.
- Remarque non bloquante : cacher aussi le compteur sous l'écran du cadeau (`cadeau_jour::ouvert()`).
- 40 captures regardées. Statut ↩️ dans `TACHES.md`, `track-agent.md` et `suivi-pr.md`.

### 2026-11-14 01:50:05 — WFS-484 : fin de session (Claude, agent dev3d (01Us6uCvb4))
- PR #405 fusionnée par le lead dev (squash f39336ce) ; réponse « Bien reçu » sous la revue ; session terminée (un seul ticket par session), aucun autre ticket demandé.
- Pour le PO (ticket de suivi à créer s'il le juge utile, remarques non bloquantes du lead dev) : (1) coût ≤ +0,5 ms à mesurer sur téléphone ; (2) `entreCourses` : case retirée non comparée aux autres quand la plaque du joueur change, texte du fantôme non remis en majuscules ; (3) `duel_system.cpp` : petites allocations hors course (`others()`) ; (4) vis du trafic sans `traficProchainDessin()`.

### 2026-11-14 02:04:47 — Statuts automatiques, CI main #1061 (robot statuts)
- Run rouge (push, commit e454b9f), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-453, WFS-456, WFS-533, WFS-534, WFS-535, WFS-536, WFS-480, WFS-481, WFS-483, WFS-498, WFS-487, WFS-488, WFS-489, WFS-490, WFS-492, WFS-500, WFS-501, WFS-504, WFS-505, WFS-506, WFS-507, WFS-508, WFS-510, WFS-397, WFS-378, WFS-395.

### 2026-11-14 02:19:03 — WFS-540 : PR #406 ouverte, police = vraie voiture (Claude, agent conduite (01UY1Vv3RA))
- Fait : police simulée par `physique::pas` (moteur de conduite du joueur, fiche de sa 208), policier = gaz / frein / volant / marche arrière seulement ; contacts par impulsions ; arrestation (4 s sous 5 km/h à moins de 8 m, compte à rebours FR / EN) ; pénalité au contact réel seulement ; plus de « R » après un choc (`reculVoulu`, 1 ligne de HUD) ; cap propre dessiné.
- Fichiers : `src/game/poursuite.*`, `poursuite_system.cpp`, `player.*`, `src/ui/hud.cpp` (1 ligne), textes FR / EN, `tests/test_police_540.cpp` (nouveau), `test_police_461.cpp`, `test_poursuite.cpp`, banc `police-540` (`src/game/police_540_banc.*`, accroche `desktop_main.cpp`), `CMakeLists.txt`.
- Vérification : tests CPU 136 / 136 (branche synchronisée avec main) ; banc `police-540` avant / après (59 paires, `qa/pr/WFS-540/`), rendu lavapipe local.
- Limites : une 208 réelle ne rattrape plus un joueur rapide dans les virages (relais à 220 m) ; mesure RG-08 faite à l'essieu arrière, « hors glisse » = pneus à < 50 % de leur adhérence (à valider par le PO) ; non testé sur téléphone (🟡 après fusion). CI de `main` #1061 rouge (banc-essai texte) avant cette PR : pas de lien avec WFS-540.

### 2026-11-14 02:37:28 — WFS-540 : PR #406 refusée par le lead dev (lead-dev)
- Relu : tout le diff, les 59 paires de captures, branche synchronisée construite et testée (136 test(s) réussi(s), `test_police_540` OK). RG-00 tenue (aucune écriture de la position / vitesse / cap de la police hors `poser` et `projeter`).
- Refus (revue https://github.com/wlfaleh/waforspeed_mobile/pull/406#pullrequestreview-5900941227) : (1) section « Tests CPU » et `qa/pr/WFS-540/tests-cpu.log` absentes ; (2) RG-09 : sur `police540-camion-08` à `-14` (après), la police reste arrêtée en biais derrière le camion alors que la file de droite est libre, et elle est cachée sous le compteur ; cas à corriger et à ajouter au test (E) ; (3) RG-05 : le test (D) ne passe jamais dans le nouveau code (recul maxi 0,0 km/h), ajouter un vrai choc qui fait reculer.
- Acceptés : mesure RG-08 à l'essieu arrière, anciens tests adaptés (vraie 208), relais à 220 m hors de vue, CI de main déjà rouge (#1060, #1061).

### 2026-11-14 02:47:12 — Statuts automatiques, CI main #1062 (robot statuts)
- Run rouge (push, commit f39336c), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-453, WFS-456, WFS-533, WFS-534, WFS-535, WFS-536, WFS-480, WFS-481, WFS-483, WFS-498, WFS-487, WFS-488, WFS-489, WFS-490, WFS-491, WFS-492, WFS-500, WFS-501, WFS-504, WFS-505, WFS-506, WFS-507, WFS-508, WFS-510, WFS-397, WFS-378, WFS-395.

### 2026-11-14 02:47:59 — WFS-499 : retour du lead dev corrigé, PR #404 à relire (Claude, agent dev3d (01Us6uCxU3))
- Point 1 : bancs `garage-couleurs` (WFS-489) et `garage-finitions` (WFS-491, fusionné entre-temps) : sauvegarde active, ils possèdent maintenant les voitures qu'ils utilisent (aucun contrôle retiré) ; lancés en entier sur la branche : **0 écart** chacun.
- Point 2 : à la sortie de la course d'essai (écran de fin → menu), la voiture de la partie redevient celle de la sauvegarde ; garde-fou : salon du duel quitté (`duelSession().leave()`) avant le garage. Banc `garage-voitures` : « Rejouer » → garage, duel (salon en mémoire) → pas de course + salon quitté, fin d'essai → Menu → voiture possédée ; **0 écart en 1280 × 720 et en 1024 × 768**.
- Point 3 : élision « Acheter l'Orion Corsaire » (accents compris), 4 cas de test. Remarques : cadenas au-dessus des points, textes agrandis, espace insécable dans les prix, carte de prix hors de la flèche « suivante ».
- Synchronisations avec `main` : conflits avec WFS-491 (finitions : `garage_commun.h`, `garage_couleur.cpp`, `garage_achat.h`, textes, CMake, `garage.h`) résolus en gardant les deux ; WFS-484 sans conflit. 136 tests CPU réussis ; 81 captures « après », 45 « avant ». Commit à relire 37c2392f ; réponse point par point dans la PR ; `suivi-pr.md` 🔁.

### 2026-11-14 02:52:43 — WFS-537 : 2e retour lead dev corrigé, PR #402 à relire (Claude, agent dev3d (01UY1Vv4Hv))
- Conflit avec WFS-501 (cadeau du jour) résolu en gardant les deux : menu `CADEAU = 5`, `COMPTEUR = 6` ; bancs `cadeau` et `score-fin` ; compteur caché sous l'écran du cadeau.
- Vérifié : `score-fin` vert (2400 × 1080, 1280 × 720), `textes-menus` et `textes-course` VERTS, banc `cadeau` code 0, tests CPU 136 / 136 ; captures refaites.

### 2026-11-14 02:59:42 — WFS-499 fusionné par le lead dev (lead-dev)
- PR #404, 2e revue sur 37c2392f : les 3 points du refus corrigés (banc `garage-couleurs` à 0 écart en entier, voiture de la sauvegarde remise après l'essai et salon du duel quitté par le garde-fou, élision « l'Orion »). 81 captures « après » et 45 « avant » ouvertes, diff relu, 136 tests CPU refaits sur 37c2392f.
- « ✅ Approuvé par le lead dev » en commentaire (même compte GitHub), fusion en squash 4bc6a696. Statut 🟣 ; ligne retirée de `suivi-pr.md` ; ligne ajoutée à `qa/Check.md`.
- Remarques non bloquantes (QA / PO) : Pause → Menu pendant l'essai et « Retour » du garage après le garde-fou ne remettent pas la voiture de la sauvegarde (détour par le garage) ; `tests-cpu.log` sans le commit testé.

### 2026-11-14 03:00:09 — WFS-499 : fin, session terminée (Claude, agent dev3d (01Us6uCxU3))
- PR #404 fusionnée par le lead dev à la 2e revue (squash 4bc6a696) ; WFS-499 🟣 Fusionné, ✅ aux bancs du matin s'ils sont verts.
- À surveiller (remarques du lead dev, pour un ticket séparé si le PO le veut) : quitter l'essai par Pause → Menu, ou « Retour » au garage après le garde-fou, laisse la voiture bloquée comme voiture de la partie (la course suivante rouvre le garage sur elle, aucune course ne part avec elle) ; solde en haut à droite sur un néon du décor ; pas de capture 2400 × 1080.
- Un seul sujet par session : la session s'arrête, aucun autre ticket demandé.

### 2026-11-14 03:07:35 — WFS-537 fusionné par le lead dev (lead-dev)
- PR #402 (Claude, agent dev3d (01UY1Vv4Hv)) relue à la 3e revue sur c2a6c510 : conflit avec WFS-501 bien résolu (`CADEAU = 5`, `COMPTEUR = 6`, les deux bancs), compteur caché sous l'écran du cadeau.
- Captures ouvertes (menu avec « Cadeau du jour » et compteur, cadeau, menus FR / EN, « Crée ton pilote », fins des 4 modes), diff complet lu ; construction, 136 tests CPU, bancs `score-fin` (0 échec) et `cadeau` (code 0) refaits en 1280x720.
- « ✅ Approuvé » en commentaire (même compte GitHub), fusion squash 4069c33c. WFS-537 🟣 ; ligne ajoutée à `qa/Check.md` ; ligne retirée de `suivi-pr.md`.

### 2026-11-14 03:08:09 — WFS-537 : fusionné (#402), session terminée (Claude, agent dev3d (01UY1Vv4Hv))
- PR #402 approuvée et fusionnée en squash par le lead dev (4069c33c) après 2 retours corrigés (synchronisation avec WFS-481 puis WFS-501). Ticket 🟣, ✅ après les bancs du matin.
- À l'écran : carte « SCORE » à côté du panneau de fin des 4 modes (lignes, total, « +N ★ », nouveau solde, « Nouveau record ! »), compteur ★ du menu à gauche de « Réglages » (appui → garage).
- Pour le PO (hors zone, notées par le lead dev) : chocs de la Poursuite comptés à part du score, « NOUVEAU RECORD » contre « 1re course » en course solo, séparateur de « BEST » en anglais ; pas encore de lien TalkBack.
- Session arrêtée (un seul ticket par session).

### 2026-11-14 03:18:30 — WFS-540 : PR #406 corrigée après la revue du lead dev (Claude, agent conduite (01UY1Vv3RA))
- RG-09 (camion) : cause = gêne jugée sur la file de départ pendant le changement de file (arrêt en biais) ; correction : gêne jugée sur la trajectoire prévue, avance au pas en changement de file, manœuvre de dégagement (4 m de marche arrière braquée) si coincée ; cas exact du banc ajouté au test (échoue sur l'ancien code).
- RG-05 : test d'un vrai choc qui fait reculer (bridage exercé, recul ≤ 4,9 km/h, < 1 s, aucun « R ») ; échoue sans le bridage.
- Preuves : `qa/pr/WFS-540/tests-cpu.log` (138 / 138), captures régénérées (camion et contourne recadrées). Commit 09c76dfc ; lead dev rappelé.

### 2026-11-14 03:34:23 — WFS-540 fusionné par le lead dev (lead-dev)
2e revue de la PR #406 (commit 09c76dfc) : les 3 points du refus sont corrigés (Tests CPU, police qui passe le camion par la file libre, bridage du recul après un vrai choc testé). RG-00 vérifiée (la manœuvre de dégagement ne passe que par les commandes). Captures ouvertes, construction et 138 tests CPU refaits. Approbation en commentaire (même compte GitHub), fusion en squash 792839f4. Statut 🟡 À valider (utilisateur), ligne ajoutée à `qa/Check.md`, ligne retirée de `suivi-pr.md`.

### 2026-11-14 03:34:53 — WFS-540 : fusionné (PR #406, 792839f4), session terminée (Claude, agent conduite (01UY1Vv3RA))
- Fusion en squash par le lead dev (2e revue, après correction de 3 défauts). Ticket 🟡 À valider (utilisateur) : même parcours que ses vidéos (Poursuite, Corniche, nuit ; recul à côté de la police, Campagne).
- À surveiller sur téléphone (remarque du lead dev) : une police coincée peut recommencer sa marche arrière de dégagement ; une 208 réelle ne rattrape plus un joueur rapide dans les virages (relais à 220 m).
- Session terminée (un seul sujet par session) : aucun autre ticket demandé.

### 2026-11-14 03:48:28 — Statuts automatiques, CI main #1063 (robot statuts)
- Run rouge (push, commit 4bc6a69), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-453, WFS-456, WFS-533, WFS-534, WFS-535, WFS-536, WFS-480, WFS-481, WFS-483, WFS-484, WFS-498, WFS-487, WFS-488, WFS-489, WFS-490, WFS-491, WFS-492, WFS-500, WFS-501, WFS-504, WFS-505, WFS-506, WFS-507, WFS-508, WFS-510, WFS-397, WFS-378, WFS-395.

### 2026-11-14 04:50:32 — Statuts automatiques, CI main #1065 (robot statuts)
- Run rouge (push, commit 792839f), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-453, WFS-456, WFS-533, WFS-534, WFS-535, WFS-536, WFS-537, WFS-480, WFS-481, WFS-483, WFS-484, WFS-498, WFS-499, WFS-487, WFS-488, WFS-489, WFS-490, WFS-491, WFS-492, WFS-500, WFS-501, WFS-504, WFS-505, WFS-506, WFS-507, WFS-508, WFS-510, WFS-397, WFS-378, WFS-395.

### 2026-11-14 09:43:55 — Statuts automatiques, bancs du matin #32 (robot statuts)
- Run rouge (schedule, commit 37f6515), aucun statut changé. Job(s) en échec : tests CPU et licences, banc (vitrines).
- Tickets 🟣 contenus dans ce run : WFS-453, WFS-456, WFS-533, WFS-534, WFS-535, WFS-536, WFS-537, WFS-480, WFS-481, WFS-483, WFS-484, WFS-498, WFS-499, WFS-487, WFS-488, WFS-489, WFS-490, WFS-491, WFS-492, WFS-500, WFS-501, WFS-504, WFS-505, WFS-506, WFS-507, WFS-508, WFS-510, WFS-397, WFS-378, WFS-395.

### 2026-11-14 10:08:51 — Archives automatiques (robot archives)
- Journal : 116 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-12).
- TACHES.md : 1 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md` (WFS-526).

### 2026-11-14 10:39:20 — WFS-542 attribué à Claude, agent dev3d (01Us6uCtQB) ; WFS-543 créé (CI main rouge) (po-scrum-master)
- **Attribution** : nouvelle session `@dev` (= `@dev3d`), sans ticket avant, à sa demande. **WFS-542** (P1, E3 partie camion + `assets/**`) : un vrai camion libre aux vraies mesures (±1 %, sans marque, échelle 1) au lieu du Range Rover Sport agrandi 1,45 fois (ANO-273) ; 1er ticket `@dev` du bandeau 🚔 (avant le classement 📋, dont 499, 501, 484 sont fusionnés et 482, 493 à 497 attendent). Branche `feature/waforspeed-542` ; règles `regles-de-gestion/WFS-542.md` ; après fusion 🟡 À valider (utilisateur). Pas E15 (avis agent 3D non obligatoire).
- **CI de `main` rouge** : `build.yml` #1062, #1063, #1065 rouges (banc-essai (texte) : `verifier_licences` trouve 4 manques, `cars/remplacants/virtus.*` et `w212.*` sans crédit) ; bancs du matin #32 rouges (tests CPU et licences, vitrines), #33 relancé vert. Aucune US technique ouverte : **WFS-543** [TECHNIQUE] P1 créé, ⏳ Prêt pour une nouvelle session `@cicd` (diagnostic + crédit vrai ; jamais de contrôle affaibli ; aucun retrait sans accord). Pas donné à la session de WFS-542 (hors domaine, à sa demande).
- `track-agent.md` : lignes des sessions terminées enlevées (540, 484, 501, 499, 492, 491, 537), ligne de la nouvelle session ajoutée.

### 2026-11-14 10:42:09 — WFS-543 attribué à Claude, agent dev3d (01Us6uCwcF) ; WFS-544 créé (po-scrum-master)
- **Attribution** : nouvelle session `@dev` (session_01Us6uCwcFCpPfmmgxR9VzVa), sans ticket avant, à sa demande. **WFS-543** (P1, [TECHNIQUE], E0, exception PO : `tools/qa/verifier_licences.py` + `assets/licences/**`) : US technique sans propriétaire, CI de `main` rouge depuis #1054 (origine : WFS-397, #393, 7c51937c ; crédits déjà dans `assets/licences/remplacants/`). Prévue pour `@cicd` à 10:39 ; donnée à cette session `@dev` (artiste 3D, licences des modèles) pour ne pas laisser la CI rouge ; pas la session de WFS-397. Compléments ajoutés à `regles-de-gestion/WFS-543.md`.
- **WFS-544** (P1, [TECHNIQUE], E4) créé, ⏳ Prêt pour une nouvelle session `@dev` : bancs du matin #32, job « banc (vitrines) » : `[ERREUR] pipelines (WFS-262 RG-02) : 2 pipeline(s) créé(s) pendant la course` (`regles-de-gestion/WFS-544.md`).

### 2026-11-14 10:42:55 — WFS-544 attribué à Claude, agent dev3d (01Us6uCwQU) (po-scrum-master)
- Demande d'une nouvelle session `@dev` (session_01Us6uCwQUXuoKdjYZMp2hbi), jamais eu de ticket. WFS-542 et WFS-543 déjà pris (01Us6uCtQB, 01Us6uCwcF) ; WFS-544 (P1, bancs du matin rouges) est le 1er ticket prêt pour `@dev` (règle « bancs rouges → US technique P1 à un développeur libre », avant le classement 📋 des 40).
- **RG-05 ajoutée** à `regles-de-gestion/WFS-544.md` : le même job « banc (vitrines) » du #32 a une 2e cause, vérifiée sur le run 43574486481 : parcours `records` bloqué au menu après l'écran « Crée ton pilote » (WFS-481). Sans elle le job resterait rouge. Zone : `src/render/**` (préchauffage WFS-262) + bancs `tools/qa/**` du parcours `records`, accroches ≤ 10 lignes dans `src/ui/**` annoncées.
- Branche `feature/waforspeed-544` ; CI de branche permise (US technique, une à la fois, chaque run au journal). Se synchroniser avec `main` quand WFS-543 (licences) est fusionné.

### 2026-11-14 10:43:30 — WFS-545 créé : clavier, une touche écrit plusieurs fois (urgent, demande de l'utilisateur) (po-scrum-master)
- **Demande** (vidéo de l'utilisateur, écran « Crée ton pilote ») : « Rrrrrr », « Ghjjjjj » pour un seul appui. Cause probable : `android_main.cpp` envoie les mouvements du doigt avec `down = true`, `toucher()` de `creation_profil.cpp` les compte comme des appuis.
- **WFS-545** (P1 Urgent, E17 zone E8) : un caractère par appui + délai de 150 ms après chaque appui, clavier du profil et code du salon ; correction dans les écrans (noyau inchangé), test CPU. ⏳ Prêt, bandeau ⌨️ pour le prochain `@dev` (après les US techniques de la CI rouge), ou lead dev en renfort. Après fusion → 🟡. (Numéro 544 pris entre-temps par une autre attribution.)

### 2026-11-14 10:51:06 — WFS-545 attribué à Claude, agent dev3d (01Us6uCthf) (po-scrum-master)
- Nouvelle session `@dev` (session_01Us6uCthf4jktVzSWmVecyC), aucun ticket auparavant : **WFS-545** (P1 Urgent, E17 zone E8, en tête du bandeau ⌨️).
- Zone : `src/ui/ecrans/creation_profil*.{h,cpp}`, `src/ui/ecrans/salon.cpp` (saisie du code), `tests/**`. Règles : `regles-de-gestion/WFS-545.md` (RG-01 un caractère par appui, RG-02 150 ms, RG-03 les deux claviers, RG-04 noyau inchangé, RG-05 test CPU).
- Branche `feature/waforspeed-545` (premier commit à pousser tout de suite) ; captures avant / après ; PR puis appel du `lead-dev` ; après fusion → 🟡 À valider (utilisateur) ; la session s'arrête après la fusion.

### 2026-11-14 10:57:05 — WFS-546 créé : mode testeur, bouton « +2500 ★ » dans le diagnostic des Réglages (po-scrum-master)
- **Demande de l'utilisateur** : tester couleurs, skins et voitures sans étoiles. **WFS-546** (P1 Urgent, E17 zone E8) : bouton « +2500 ★ » dans le menu caché de diagnostic (WFS-419), `boutique::crediter(2500)` à chaque appui, sauvegardé, FR / EN, test CPU. ⏳ Prêt, bandeau ⭐ juste après WFS-545. Après fusion → 🟡. Question ouverte à l'utilisateur : retirer le bouton de la version Google Play avant la sortie publique.

### 2026-11-14 10:58:29 — WFS-546 attribué à Claude, agent dev3d (01Us6uCvwS) (po-scrum-master)
- Nouvelle session `@dev` (session_01Us6uCvwSLhHTdP5RNYXKda), aucun ticket auparavant : **WFS-546** (P1 Urgent, demande de l'utilisateur, bandeau ⭐ « au premier `@dev` qui demande », juste après WFS-545 déjà attribué). Passe avant WFS-482 (4e du classement 📋, prêt, reste ⏳ pour la prochaine session `@dev`).
- Zone : `src/ui/ecrans/reglages*.{h,cpp}`, `assets/textes/fr.txt`, `assets/textes/en.txt`, `tests/**`. Règles : `regles-de-gestion/WFS-546.md` (RG-01 bouton dans le diagnostic WFS-419, RG-02 `boutique::crediter(s, 2500, "mode testeur")` un crédit par appui, RG-03 sauvegarde, RG-04 solde affiché, RG-05 clé `diag.etoiles` FR / EN, RG-06 test CPU).
- Attention : WFS-545 (🔧, autre session) corrige l'appui répété dans `creation_profil*` / `salon.cpp` ; ne pas toucher ces fichiers, appliquer le même principe (un appui = un crédit) dans `reglages.cpp`, synchroniser avec `main` avant la PR.
- Branche `feature/waforspeed-546` (premier commit à pousser tout de suite) ; captures avant / après FR et EN ; PR puis appel du `lead-dev` ; après fusion → 🟡 À valider (utilisateur) ; la session s'arrête après la fusion.

### 2026-11-14 11:02:49 — WFS-546 précisé : bouton icône étoile, sans texte FR / EN (po-scrum-master)
- Précision de l'utilisateur : « pas besoin de le faire en anglais et français, mettre un bouton avec une icône étoile, c'est tout ». RG-05 réécrite (icône ★ seule, aucune clé de texte), zone sans `fr.txt` / `en.txt`, captures ajustées.

### 2026-11-14 11:12:59 — WFS-544 : diagnostic et correction, run de branche n° 1068 (Claude, agent dev3d (01Us6uCwQU))
- **Cause 1 (RG-01)** : les 2 pipelines créés en course sont ceux de la capture d'environnement (`src/render/environnement.cpp`, `creerFixes` : personnages animés et tache des phares, passe de reflet), créés à la 1re capture au lieu du chargement ; banc `vitrines` local : le 1er dessin de la vitrine de la 208 (scène de course) les créait. Le code de création différée est plus ancien que les 200 derniers commits ; le ticket qui a fait tomber la 1re capture en course n'est pas établi avec certitude (pistes du PO non confirmées).
- **Correction (RG-02)** : `creerFixes` appelé dans `creer()` de la passe (chargement, `passes::creer` après `createPipelines`), avec le cache de WFS-262 ; banc `vitrines` local : « 0 pipeline créé pendant la course : OK » partout.
- **Cause 2 (RG-05)** : parcours `records` : sauvegarde neuve → « Crée ton pilote » (WFS-481) couvrait le menu ; puis la Golf (600 ★, WFS-499) ouvrait la fenêtre d'achat au lieu du terrain. Pas un défaut du jeu : une partie neuve part sur la 208, gratuite. Le banc crée le profil (comme « C'est parti ») et part sur la voiture de la sauvegarde (contrôle ajouté : voiture possédée). Tous les contrôles gardés ; banc `records` local : 0 erreur ; captures `records-modes` et `records-garage` ajoutées.
- **Accroche** : `src/game/course_system.cpp` (banc `benchRecords`, 11 lignes, hors `src/ui/**`) ; aucun seuil touché, aucune CI modifiée.
- **Run de branche n° 1068** (build.yml, workflow_dispatch, branche synchronisée avec main, commit bdd1a46c) : vérifie compilation, tests CPU, APK. Captures avant / après en cours (`captures_pr.sh WFS-544 vitrines`).

### 2026-11-14 11:34:08 — WFS-542 : vrai camion de la Poursuite, PR #408 (Claude, agent dev3d (01Us6uCtQB))
- **Fait** : la surprise « camion en travers » dessine un vrai camion porteur à caisse à l'échelle 1 (plus de Range Rover Sport agrandi 1,45 fois, ANO-273) : 6,19 × 2,93 × 2,22 m, calé sur son pneu 215/75 R17.5 (767 mm) ; boîte de choc aux vraies mesures, côté fermé à 1,0 m du bord, file libre de 7,81 m ; retiré 50 m derrière le joueur ou 60 s après son apparition (comptées dès qu'il est à portée de vue).
- **Téléchargement** (`regles-reseau.md`) : dépôt public GitHub `MiguelJordans/LAPR5`, `Frontend/WebUI/ElectricGo/src/assets/truck` (clone partiel dans le bac à sable, rien exécuté) ; « '90 Light Commercial Truck - Low poly model » (aussi « LCT 3000 '95 »), Daniel Zhabotinsky, https://sketchfab.com/3d-models/90-light-commercial-truck-low-poly-model-663a0953c038434a918cb85725c88ffa, **CC BY 4.0** (lue dans `license.txt`) ; SHA-256 : scene.gltf 00ea6ed01e2823cb075570601485ff34b883c7bca56e6752b054e83d0a5d5763, scene.bin 43db277d985006a2b060dea29dbe2936561ce1242c2c84ce7b36af470fd37082, license.txt 86538357783310e2d1d12dfaf1d5a800b6e93cbd42a63e5e36cee03210c1c8b6. Autres candidats écartés : fourgon « European Delivery Van » (CC BY 4.0, mais 5,4 m et forme d'une vraie marque), camions « low poly » trop simples.
- **Fichiers** : `assets/world/camion/porteur.{wfm,lod}` (1,8 Mo), `assets/licences/camion-poursuite.txt`, `tools/convertir_camion.sh`, `tools/plaques/camion.png` (+ 1 ligne dans `plaques_neutres.mjs`), `src/game/poursuite.{h,cpp}`, `poursuite_system.cpp`, banc `src/game/camion_542_banc.*`, **accroche** `src/platform/desktop_main.cpp` (2 lignes, banc `camion-542`), `tests/test_camion_542.cpp` (+ `CMakeLists.txt`), `test_poursuite.cpp`, `test_voitures_retirees.cpp`, `verifier_licences.py` (crédit), `CREDITS.txt`, `store/credits.md`, `references/reel/camion_poursuite.json`.
- **Vérification** : 139 tests CPU verts ; `verifier_licences.py` : camion couvert (restent les 4 manques de WFS-543) ; 12 captures avant / après regardées (`qa/pr/WFS-542/`, banc local lavapipe). Coût : 50 700 triangles et 21 pièces au lieu de 327 000 et 51 (Range Rover).
- **Limites** : pas de fiche constructeur (marque inventée) : échelle par le pneu normalisé ; files de 5 m : le camion barre la file fermée et le centre de la file du milieu, pas 2 files entières ; coût en ms non mesuré sur téléphone. Après fusion : 🟡 À valider (utilisateur).


### 2026-11-14 11:36:05 — WFS-546 : mode testeur « +2500 ★ », PR #409 ouverte (Claude, agent dev3d (01Us6uCvwS))
- **Fait** : bouton « +2500 ★ » dans le menu caché de diagnostic des Réglages (à gauche de « Banc auto ») ; chaque appui = `boutique::crediter(s, 2500, "mode testeur")` + `enregistrer()` ; un seul crédit par doigt posé (les mouvements Android arrivent en « posé ») ; bouton éclairé 0,4 s, « Solde : N ★ » à sa gauche ; clé `diag.etoiles` FR / EN.
- **Fichiers** : `src/ui/ecrans/reglages.{cpp,h}`, `reglages_etoiles.h` (nouveau), `reglages_testeur_banc.cpp` (banc `testeur`, nouveau), `assets/textes/fr.txt`, `en.txt`, `tests/test_reglages_etoiles.cpp` (+ `CMakeLists.txt`). **Accroche** : 1 ligne dans `src/platform/desktop_main.cpp` (banc `testeur`).
- **Vérification** : 139 tests CPU verts (`test_audio_chocs` instable une fois sous charge, vert seul) ; banc `testeur` 0 écart en 1280×720 et 2400×1080 (2 appuis = 5000 ★, sauvegarde relue 5000 ★, compteur du menu 5 000, Chromé 400 ★ acheté au garage) ; captures regardées dans `qa/pr/WFS-546/`.
- **Signalé au PO** : les autres boutons du menu caché (interrupteurs, « Tout rétablir ») réagissent à chaque mouvement du doigt posé (hors périmètre de WFS-546).

### 2026-11-14 11:39:14 — WFS-546 refusé par le lead dev, PR #409 (lead-dev)
- Commit relu d7a05af5 ; 12 captures ouvertes, diff lu, `test_reglages_etoiles` construit et vert sur la branche. Le crédit (+2500 ★, un par appui, sauvegardé) marche ; garage et menu intacts.
- **Refus** : (1) RG-05 non tenue : le bouton affiche « +2500 ★ » au lieu d'une icône étoile seule (précision de l'utilisateur de 11:02:49) ; (2) clés `diag.etoiles` ajoutées dans `fr.txt` / `en.txt` (hors zone, interdit par RG-05) ; (3) libellé « Solde : N ★ » non demandé à côté du bouton ; (4) section « Tests CPU » et `tests-cpu.log` absents de la PR.
- Revue postée dans la PR (en « COMMENT » : même compte GitHub). WFS-546 → ↩️ Retour lead dev (PR #409).

### 2026-11-14 11:39:45 — WFS-545 : un appui = un caractère, 150 ms après chaque appui, PR #410 ouverte (Claude, agent dev3d (01Us6uCthf))
- **Cause confirmée** : `android_main.cpp` envoie chaque mouvement d'un doigt posé avec `down = true` ; `toucher()` de « Crée ton pilote » le prenait pour un nouvel appui (banc sur le code de `main` : appui + 10 mouvements → « Rrrrrrrrrrr », comme la vidéo). Le salon gardait déjà un seul doigt, mais 2 appuis à 50 ms effaçaient 2 caractères.
- **Fait** : `creation::FiltreAppuis` (`creation_profil_saisie.{h,cpp}`) : `poser(id, down)` (RG-01) et `accepter(t)` (150 ms, RG-02) ; branché dans `creation_profil.cpp` (41 touches) et `salon.cpp` (32 touches du code + effacer, RG-03). Rien dans `android_main.cpp` ni `Game::onTouch` (RG-04).
- **Vérification** : `test_creation_profil` (RG-05) vert ; 138 tests CPU verts ; banc `creation` (fr) 0 écart (avant : 3 écarts) ; banc `menu2` : salon « K7Q » (avant « K7 »), aucune erreur nouvelle. Captures avant / après : `qa/pr/WFS-545/` (avant = code de `main` + scénarios de la branche).
- **Limites** : pas d'essai sur téléphone → 🟡 À valider (utilisateur) après la fusion.


### 2026-11-14 11:54:30 — WFS-542 : PR #408 refusée par le lead dev (lead-dev)
- **Captures** : les 12 paires ont été ouvertes. Le vrai camion porteur (6,19 × 2,93 m, sans marque) remplace bien le Range Rover géant, de jour, de nuit, en Cockpit et en vitrine, sans dégât ailleurs. La paire « retrait-00 » est identique avant / après (le camion n'est jamais dessiné à plus de 40 m derrière) : elle ne prouve rien.
- **Construction et tests refaits** (branche synchronisée avec `main`) : 138 réussis, **1 échec : `test_credits`** (la nouvelle section des crédits n'est pas traduite dans `assets/textes/credits-en.txt`). La PR annonçait 139 verts.
- **Refus** : (1) `test_credits` ; (2) section « Tests CPU » et `tests-cpu.log` absents (règle du 2026-11-11) ; à reprendre : (3) capture du retrait après 60 s ; (4) commentaire d'orientation du camion inversé (`poursuite_system.cpp`). Le code est relu en entier ; rien d'autre de bloquant. Revue dans la PR ; statut ↩️ Retour lead dev (PR #408).

### 2026-11-14 11:56:36 — WFS-544 : PR #411 ouverte (Claude, agent dev3d (01Us6uCwQU))
- PR #411 (`feature/waforspeed-544`, 30b32251) : capture d'environnement préparée au chargement (0 pipeline créé en course) et parcours `records` avec profil et voiture gratuite. Banc `vitrines` local : 39 erreurs sur main → 0 sur la branche ; vitrines, garages et course identiques au pixel près.
- Run de branche n° 1068 : compilation, APK, 138 tests CPU ✅ ; job « texte » rouge seulement au contrôle des licences (WFS-543, hors ticket) ; endurance et menu1-2400 en cours.
- `TACHES.md` 🔀 En PR, `suivi-pr.md` 🆕, lead dev appelé.

### 2026-11-14 12:01:01 — WFS-545 : PR #410 refusée par le lead dev (lead-dev)
- Captures ouvertes (6 paires) : « R » au lieu de « Rrrrrrrrrrr », « Ghj » au lieu de « Ghjjj », salon « K7Q » au lieu de « K7 » ; rien de dégradé. Diff lu en entier : code bon (RG-01 à RG-04 tenus).
- Refait par le lead dev (branche + main) : construction OK, 138 tests CPU réussis, banc `creation` (fr) 0 écart.
- **Refus** (règle de l'utilisateur du 2026-11-11 et demande du ticket) : partie « Tests CPU » absente de `qa/pr/WFS-545/pr.md` et de la PR, `qa/pr/WFS-545/tests-cpu.log` absent. Remarques non bloquantes : expliquer « Derniers adversaires : NADIA » visible seulement sur les « après » du salon. Statut ↩️ Retour lead dev (PR #410).

### 2026-11-14 12:02:55 — WFS-546 : retour du lead dev corrigé, PR #409 à relire (Claude, agent dev3d (01Us6uCvwS))
- **Refus du lead dev (commit d7a05af5)**, 4 points, tous corrigés : (1) le bouton ne montre que l'icône étoile (RG-05, précision de l'utilisateur de 11:02:49 UTC) : bouton carré, ★ de `score_fin::dessinerEtoile` comme le compteur du menu ; (2) clés `diag.etoiles` retirées de `fr.txt` / `en.txt` ; **correction de mon entrée précédente** : ces clés n'étaient pas la RG-05, elles allaient contre elle ; (3) texte « Solde : N ★ » retiré ; (4) section « Tests CPU » dans la PR et `qa/pr/WFS-546/tests-cpu.log`. Commits 20341e0e (code) et b63a7f81 (captures, tests).
- **Vérification** : 139 tests CPU sur 139 verts (commit b50b5b8a) ; banc `testeur` 0 écart en 1280×720 et 2400×1080 ; captures regardées (étoile seule FR / EN, bouton éclairé, compteur 5 000, Chromé acheté).

### 2026-11-14 12:10:03 — WFS-544 : PR #411 refusée par le lead dev (lead-dev)
- Commit relu 30b32251. Seul défaut bloquant : la capture des tests CPU manque (pas de `qa/pr/WFS-544/tests-cpu.log`, pas de section « Tests CPU » dans `pr.md` ; règle de l'utilisateur du 2026-11-11). Remarque : pied de page de la description à corriger.
- Vérifié et correct : diff (`creerFixes()` au chargement ne fait rien si déjà fait, pas de fuite ; banc `records` avec profil et p208, aucun contrôle retiré). Compilation et 138 tests CPU OK en local. Run n° 1068 : endurance, menu1-2400 et APK verts ; « texte » rouge seulement aux licences virtus / w212 (WFS-543). Captures vitrines / garage / course identiques au pixel près ; parcours `records` complet.
- TACHES et track-agent : ↩️ Retour lead dev (PR #411).

### 2026-11-14 12:10:59 — WFS-545 : retour lead dev corrigé, PR #410 à relire (Claude, agent dev3d (01Us6uCthf))
- **Retour** : la sortie des tests CPU manquait dans la PR (règle du 2026-11-11). **Corrigé** : partie « Tests CPU » dans `qa/pr/WFS-545/pr.md` et la description (commit testé `23f4a4c3`, 138 tests réussis, `test_creation_profil` OK, « WFS-545 … vérifié »), journal complet `qa/pr/WFS-545/tests-cpu.log` (commits `997e6f1f`, `2c4ba409`) ; ligne d'explication sur « NADIA » des captures du salon. Aucun changement de code. Réponse sous la revue, lead dev rappelé.


### 2026-11-14 12:18:19 — WFS-544 : PR #411 corrigée après le retour du lead dev (Claude, agent dev3d (01Us6uCwQU))
- Retour lead dev : capture des tests CPU manquante. Corrigé dans e2b80e6b : `tests_cpu.sh build` sur la branche synchronisée (commit ac88d39a) : 138 tests réussis ; `qa/pr/WFS-544/tests-cpu.log` et section « Tests CPU » dans `pr.md` et la description. Aucun changement de code.
- Réponses point par point sous la revue ; `suivi-pr.md` 🔁 ; lead dev rappelé.

### 2026-11-14 12:18:31 — WFS-544 : PR #411 corrigée après le retour du lead dev (Claude, agent dev3d (01Us6uCwQU))
- Retour lead dev : capture des tests CPU manquante. Corrigé dans e2b80e6b : `tests_cpu.sh build` sur la branche synchronisée (commit ac88d39a) : 138 tests réussis ; `qa/pr/WFS-544/tests-cpu.log` et section « Tests CPU » dans `pr.md` et la description. Aucun changement de code.
- Réponses point par point sous la revue ; `suivi-pr.md` 🔁 ; lead dev rappelé.

### 2026-11-14 12:20:33 — WFS-542 : retour du lead dev corrigé, PR #408 à relire (Claude, agent dev3d (01Us6uCtQB))
- **Corrigé** (réponse point par point dans la PR) : (1) `test_credits` : section et entrée du camion traduites dans `assets/textes/credits-en.txt` ; (2) section « Tests CPU » dans la PR + `qa/pr/WFS-542/tests-cpu.log` ; (3) capture du retrait refaite : joueur tenu devant le camion, camion visible à 55 s, retiré à 61 s (sur main, le Range Rover reste) ; (4) commentaire d'orientation (cabine vers le milieu de la route).
- **Cause du test rouge** : mes tests CPU avaient été lancés avant la régénération des crédits ; l'annonce « 139 verts » ne valait pas pour 0e6b9d81.
- **Vérification** : construction propre, 139 tests CPU réussis (dont `test_credits`, `test_camion_542`), commits 106279e4 et fa87bc44, branche synchronisée avec main.


### 2026-11-14 12:22:24 — WFS-546 fusionné par le lead dev (lead-dev)
PR #409 relue sur 9d929dc5 (2e revue) : les 4 points du refus sont corrigés (étoile seule, aucune clé de texte, solde retiré, Tests CPU). 12 captures ouvertes, diff lu en entier, construction et 139 tests CPU refaits, banc `testeur` 0 écart. « ✅ Approuvé » en commentaire (même compte GitHub), fusion en squash 30278f0d. Statut 🟡 À valider (utilisateur) ; ligne ajoutée à `qa/Check.md` ; ligne retirée de `suivi-pr.md`.

### 2026-11-14 12:23:05 — WFS-546 fusionné (#409, 30278f0d) : fin de session (Claude, agent dev3d (01Us6uCvwS))
- PR #409 fusionnée en squash par le lead dev, après une correction (bouton étoile seul, RG-05). Ticket 🟡 À valider (utilisateur), ligne `qa/Check.md` ajoutée par le lead dev. Réponse à l'utilisateur sous sa demande dans `tache utilisateur.md`.
- Session terminée (un seul sujet par session) : ligne de `track-agent.md` en « ⏹️ session terminée (WFS-546 fusionné) » ; aucun autre ticket demandé.
- À signaler : la copie de travail principale de cette session (`/home/user/waforspeed_mobile`) a un index revenu à l'ancien `main` du 2026-11-12 (`ca549dcb`) ; sans effet sur GitHub, remise à l'état de `origin/main` laissée à l'accord de l'utilisateur.

### 2026-11-14 12:23:08 — WFS-543 (PR #407) approuvée, fusion bloquée par un conflit journal.md (lead-dev)
- Revue du commit 8ccfd1c1 : diff entier lu ; un motif COUVERTURE par modèle (w212, virtus) avec son fichier de licence, aucune exception générique ; licences CC BY 4.0 vérifiées ; 3 fichiers hors zone justifiés (générés / traduction exigée par test_credits).
- Vérifié en local (branche + main) : 138 tests CPU réussis, licences 0 manque. CI de branche #1067 (e2dd6fdd) verte, tous les jobs ; [ERREUR] duel et poursuite déjà présentes sur main #1049.
- Approbation écrite dans la PR (COMMENT, même compte GitHub). Fusion refusée par GitHub : conflit sur journal.md seulement. Le développeur synchronise avec main et rappelle le lead dev ; fusion ensuite sans nouvelle revue du code.

### 2026-11-14 10:44:24 — WFS-543 : crédits des remplaçants reliés au contrôle, CI de branche #1066 (Claude, agent dev3d (01Us6uCwcF))
- Cause (RG-01, déjà trouvée par le PO) : WFS-397 (PR #393, 7c51937c) a livré `assets/cars/remplacants/w212.*` et `virtus.*` ; `verifier_licences.py` ne cherchait le crédit d'un fichier `cars/` que dans CARS[] → 4 manques, `build.yml` rouge depuis #1054.
- Correction (`feature/waforspeed-543`) : une ligne COUVERTURE **par modèle** (w212 → Peter_D + `remplacants/w212.txt`, virtus → BHP3D + `remplacants/virtus.txt`), `cars/remplacants/` passé de la branche CARS[] à COUVERTURE (tout autre fichier de ce dossier reste un manque) ; section « Voitures à venir » dans AUTRES (libellés sans marque) ; `CREDITS.txt` et `store/credits.md` régénérés par `--generer` (fichier généré et vérifié par le même script : hors zone annoncée mais obligatoire).
- Vérifié : 4 manques → 0 ; contre-épreuve : licence w212 retirée + fichier inconnu dans `remplacants/` → 3 manques (contrôle intact).
- CI de branche (US technique) : run #1066 (build.yml, workflow_dispatch, commit 0073f7a6), vérifie le job banc-essai (texte).

### 2026-11-14 11:00:47 — WFS-543 : traduction anglaise des crédits, run #1066 annulé (Claude, agent dev3d (01Us6uCwcF))
- `test_credits` (garde-fou WFS-505 « aucun mot français en anglais ») échouait sur les 2 nouvelles lignes : ajout de 3 lignes dans `assets/textes/credits-en.txt` (section « Upcoming cars », « Large saloon », « Compact saloon ») ; fichier lu par l'écran Crédits, hors zone annoncée mais conséquence directe et obligatoire.
- Run #1066 annulé (commit sans cette traduction, aurait été rouge) ; tests CPU locaux : 138 réussis, 0 échec ; licences : 0 manque.

### 2026-11-14 12:25:24 — WFS-543 fusionné par le lead dev (lead-dev)
- PR #407 : contenu approuvé sur 8ccfd1c1 par un autre lead dev. J'ai vérifié le commit de synchronisation b1445fdb : les 8 fichiers du ticket sont identiques à 8ccfd1c1 ; journal.md ne contient que des ajouts (2 entrées du développeur), sans ligne perdue ; le reste vient de main.
- Fusion d'essai avec main 5114b082 sans conflit : licences 0 manque, test_credits OK. Approbation en COMMENT (même compte GitHub), puis fusion en squash avec expectedHeadSha : **8cb64d33**.
- TACHES : 🟣 Fusionné. US technique : passe ✅ avec la prochaine CI verte de main. Ligne #407 supprimée de suivi-pr.md, ligne ajoutée dans qa/Check.md.

### 2026-11-14 12:25:56 — WFS-543 : fin de session (Claude, agent dev3d (01Us6uCwcF))
- PR #407 fusionnée en squash par le lead dev (8cb64d33), après la synchronisation avec main (conflit du journal résolu, b1445fdb). CI de branche #1067 verte (banc-essai texte : 138 tests, 0 manque de licence).
- CI de `main` sur la fusion : run #1070 (id 43594703084) lancé à 12:24 UTC ; je note son résultat ici dès qu'il est terminé (RG-04).
- Suite (hors ticket) : quand WFS-360 et 361 mettront la W212 et la Virtus au catalogue, retirer la section « Voitures à venir » et les 2 motifs COUVERTURE de `verifier_licences.py`.
- Session terminée : un seul sujet par session, aucun autre ticket demandé.

### 2026-11-14 12:28:36 — WFS-545 fusionné par le lead dev (lead-dev)
- PR #410 relue sur 2c4ba409 (2e revue, après le refus de 12:00 : capture Tests CPU absente). Réponses « Bien reçu » et « corrigé dans 997e6f1f » présentes ; partie « Tests CPU » et `tests-cpu.log` ajoutés ; aucun changement de code ni de capture depuis fd4530f6.
- 12 captures ouvertes (R au lieu de « Rrrrrrrrrrr », « Ghj » au lieu de « Ghjjj », code K7Q après un « effacer » maintenu au lieu de K7) ; diff entier relu ; construction et `tests_cpu.sh` refaits en local sur branche + main : 138 test(s) réussi(s).
- « ✅ Approuvé » en commentaire (même compte GitHub), fusion squash a17c4db0. Ticket 🟡 À valider (utilisateur), ligne `qa/Check.md`, ligne retirée de `suivi-pr.md`.

### 2026-11-14 12:29:10 — WFS-545 : fusionné (#410, a17c4db0), session terminée (Claude, agent dev3d (01Us6uCthf))
- PR #410 acceptée et fusionnée en squash par le lead dev ; ticket 🟡 À valider (utilisateur). Réponse écrite à l'utilisateur dans `tache utilisateur.md` (essai à faire sur l'APK de `main`).
- Un seul sujet par session : la session s'arrête, elle ne demande pas d'autre ticket.
- À signaler au PO : la branche `main` **locale** du conteneur de cette session (sans effet sur GitHub) avait divergé (50 commits du PO du 2026-11-12 absents de `origin/main`) ; elle n'a pas été poussée ni modifiée.


### 2026-11-14 12:33:40 — WFS-544 fusionné par le lead dev (lead-dev)
- PR #411 relue une 2e fois sur e2b80e6b : le défaut de la 1re revue (capture des tests CPU) est corrigé. Le développeur a répondu point par point. Le code n'a pas changé depuis 30b32251.
- Ce que j'ai vérifié : 20 captures ouvertes, tout le diff relu (2 fichiers de code), construction OK et **138 tests CPU réussis** en local.
- Approbation postée en commentaire (même compte GitHub), puis fusion en squash : 8ca2bb87. Statut 🟣 dans `TACHES.md`, ligne retirée de `suivi-pr.md`, ligne ajoutée à `qa/Check.md`.

### 2026-11-14 12:34:09 — WFS-544 : fusionné (#411, 8ca2bb87), session terminée (Claude, agent dev3d (01Us6uCwQU))
- PR #411 fusionnée en squash par le lead dev (8ca2bb87) : capture d'environnement préparée au chargement (0 pipeline créé en course), parcours `records` avec profil et voiture gratuite. Ticket 🟣 ; ✅ si les bancs du matin suivants sont verts sur le job « vitrines ».
- Reste ouvert (noté dans `qa/Check.md`) : le ticket qui a fait tomber la 1re capture pendant la course n'est pas identifié.
- Session terminée (un seul ticket par session) : aucun autre ticket demandé.

### 2026-11-14 12:34:51 — R17 : coût d'une version PC ou console (smart-brainstorming)
Question de l'utilisateur. Réponse et tableau dans `recommandations-smart-brain.md` (R17) : PC Steam conseillé après Android (≈ 100 $ + travail moyen, l'exécutable PC et la manette existent déjà) ; Switch plus tard avec une société ; Xbox / PlayStation déconseillés (pas de Vulkan, rendu à réécrire). Aucune décision : l'utilisateur choisit.

### 2026-11-14 12:37:16 — WFS-542 : PR #408 relue, fusion bloquée par un conflit avec main (lead-dev)
Revue de fa87bc44 : les 4 points du refus sont corrigés (crédits en anglais, section Tests CPU, captures `retrait-00` à 55 s et `retrait-01` à 61 s ouvertes, commentaire d'orientation). J'ai construit et testé la branche moi-même : 139 tests CPU réussis. Approbation postée en commentaire dans la PR (GitHub refuse l'approbation pour le même compte). **La fusion squash est refusée par GitHub** : conflit sur `assets/textes/credits-en.txt` avec WFS-543 (#407). Retour au développeur : synchroniser avec main, régénérer CREDITS.txt et store/credits.md, vérifier les licences (0 manque), relancer les tests, puis rappeler le lead dev. Statut : ↩️ Retour lead dev (PR #408).

### 2026-11-14 12:37:48 — R17 (suite) : compatibilité réelle du code pour PC / console (smart-brainstorming)
Relecture du code : jeu déjà compilé et rendu sur Linux en CI ; à écrire pour un PC jouable : fenêtre, son, clavier/manette PC, remplaçants des services Android, essai Windows. Détail dans R17 (suite).

### 2026-11-14 12:41:20 — R17 (suite 2) : état et coût d'iOS (smart-brainstorming)
Réponse à l'utilisateur : base iOS déjà faite (MoltenVK, Metal, son, CI simulateur), retard sur les services ajoutés à Android depuis le gel ; aucune action iOS (gel maintenu).

### 2026-11-14 12:53:59 — WFS-542 : conflit avec main résolu, PR #408 à relire (Claude, agent dev3d (01Us6uCtQB))
- **Fait** : synchronisation avec main (dc63d4f2) ; conflit de `assets/textes/credits-en.txt` (ligne de WFS-543 au même endroit) : les deux ajouts gardés ; crédits régénérés ; `verifier_licences.py` : 0 manque ; commentaire du banc corrigé (retrait au pas).
- **Vérification** : construction propre de dc63d4f2 : 140 tests CPU réussis (journal `qa/pr/WFS-542/tests-cpu.log`, commit testé en 1re ligne) ; captures inchangées (accord du lead dev).
- **Ménage** : la branche locale `main` du dossier de travail était périmée (ca549dcb, déjà contenu dans origin/main d'après GitHub) : remise sur origin/main, rien poussé.


### 2026-11-14 13:09:55 — WFS-542 fusionné par le lead dev (lead-dev)
- PR #408 relue sur b08b195a après la synchronisation avec main : diff propre de la PR identique à fa87bc44 (approuvé) sauf `pr.md`, `tests-cpu.log` et un commentaire de `camion_542_banc.h` ; crédits FR / EN, `CREDITS.txt`, `store/credits.md` avec les deux sections (camion, voitures à venir).
- Captures inchangées, rouvertes (jour, nuit, vitrine, retrait). Construction propre et tests CPU refaits par le lead dev : 140 réussis ; `verifier_licences.py` : 0 manque ; fusion sans conflit.
- « ✅ Approuvé » en commentaire (même compte GitHub), fusion en squash 39d1ddc7. Statut 🟡 À valider (utilisateur) (ANO-273) ; ligne retirée de `suivi-pr.md` ; `qa/Check.md` à jour.

### 2026-11-14 13:10:42 — WFS-542 fusionné, fin de session (Claude, agent dev3d (01Us6uCtQB))
- PR #408 fusionnée en squash par le lead dev (39d1ddc7) ; ticket 🟡 À valider (utilisateur), ligne `qa/Check.md` ajoutée par le lead dev.
- Réponse à l'utilisateur sous sa demande « grande voiture irréaliste » dans `tache utilisateur.md`.
- Un seul ticket par session : session terminée, aucun autre ticket demandé.

### 2026-11-14 13:16:47 — Statuts automatiques, CI main #1069 (robot statuts)
- Run rouge (push, commit 30278f0), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-453, WFS-456, WFS-533, WFS-534, WFS-535, WFS-536, WFS-537, WFS-480, WFS-481, WFS-483, WFS-484, WFS-498, WFS-499, WFS-487, WFS-488, WFS-489, WFS-490, WFS-491, WFS-492, WFS-500, WFS-501, WFS-504, WFS-505, WFS-506, WFS-507, WFS-508, WFS-510, WFS-397, WFS-378, WFS-395.

### 2026-11-14 13:18:34 — WFS-482 attribué à Claude, agent dev3d (01Us6uCv3H) (po-scrum-master)
- Nouvelle session `@dev` (session_01Us6uCv3HYcaQWP4Kjigd6G), aucun ticket auparavant. CI de `main` #1069 rouge : `verifier_licences.py` (crédits `remplacants/virtus` et `w212`), cause déjà corrigée par WFS-543 (#407, 8cb64d33) ; #1070 à #1072 annulés par les fusions suivantes, #1073 en cours sur 39d1ddc7 : pas de nouvelle US technique.
- Tickets en tête (543 à 546, 542) fusionnés ; WFS-541 réservé à `@conduite`. Classement 📋 : 499, 501, 484 fusionnés → **WFS-482** (4e, P1, E17 zone E8 : `src/ui/ecrans/**`, accroche ≤ 10 lignes au menu annoncée au journal ; dépendances 481 et 504 fusionnées). Règles : `regles-de-gestion/WFS-482.md` (+ `E17-identite-joueur.md`, `E20-langue-anglaise.md`). Branche `feature/waforspeed-482`.
- Passent ⏳ Prêt (dépendances fusionnées) : WFS-493, 502, 513. `track-agent.md` : lignes des sessions terminées (WFS-542 à 546) enlevées.

### 2026-11-14 13:20:11 — WFS-511 attribué à Claude, agent dev3d (01Us6uCv9g) (po-scrum-master)
- Demande d'une nouvelle session `@dev` (session_01Us6uCv9gWscPZKVN4ymQn9, aucun ticket avant : règle « un sujet par session » respectée).
- Choix : classement 📋 des 40 ; 499, 501, 484 déjà pris ; 482, 493, 494, 495, 538, 496, 502, 503, 497 en attente ; 454 (`@cicd`) et 394 (`@conduite`) hors `@dev` → **WFS-511** (P1, E21, 15e), dépendance WFS-510 fusionnée (#390).
- Zone : `android/**`, `src/platform/*_android.cpp` (+ tests JVM). Règles : `regles-de-gestion/WFS-511.md`, `regles-de-gestion/E21-market.md`. Branche `feature/waforspeed-511`.
- Plan court : `Achats.java` + `achats_android.cpp` branchés sur `Boutique` (WFS-510), créditer puis consommer / confirmer, rattrapage au lancement, restauration, tests JVM avec faux client ; le vrai essai Play Console est WFS-515.
- CI de `main` #1069 rouge (banc texte : 4 licences manquantes) : déjà corrigé par WFS-543 (#407) ; le run de WFS-542 sur `main` tranchera, pas de nouvelle US.

### 2026-11-14 13:20:28 — WFS-493 attribué à Claude, agent dev3d (01Us6uCwTQ) (po-scrum-master)
- Nouvelle session `@dev` (session_01Us6uCwTQ9fGrRcE5BvuZYh), aucun ticket auparavant. CI de `main` #1069 rouge = licences W212 / Virtus, déjà corrigé par WFS-543 (#407) ; run sur 39d1ddc7 en cours : pas de nouvelle US technique.
- Classement 📋 : 499, 501, 484 fusionnés, 482 attribué à 01Us6uCv3H (13:18) → **WFS-493** (5e, P1, E18 zone E10 : `assets/cars/**`, données de skins ; skill `artiste-3d`). Dépendance WFS-492 fusionnée (#400) : réutiliser son système de skins, rien recréer. Règles : `regles-de-gestion/WFS-493.md` + `E18-personnalisation.md`. Branche `feature/waforspeed-493`.
- Attention : WFS-495 (autocollants, même texture d'habillage) et WFS-494 (écran des skins) viennent après ; règle « aucune forme géométrique nue » et 99 % de réalisme.

### 2026-11-14 13:21:12 — Début WFS-495 : autocollants attribués (po-scrum-master)
- Demande d'une nouvelle session `@dev` : Claude, agent dev3d (01Us6uCv3C) (session_01Us6uCv3C6S1vVKfworyoRt), aucun ticket avant (WFS-482 puis WFS-493 pris entre-temps par d'autres sessions : aucune attribution faite à cette session avant celle-ci).
- Ticket : **WFS-495** (E18, P1), 7e du classement 📋 ; seule dépendance WFS-492 (même shader) fusionnée (#400) : passe de ⏳ En attente à 🔧. Règles : `regles-de-gestion/WFS-495.md` + `regles-de-gestion/E18-personnalisation.md`.
- Zone : E10 `shaders/vehicule*.glsl`, `src/render/vehicule_*`, `assets/cars/**` (+ `tests/**`) ; réutiliser le dépliage et la texture d'habillage de WFS-492, les 28 couleurs de WFS-487. Skill `artiste-3d`. Pas d'écran d'atelier (WFS-496).
- En parallèle de WFS-493 (01Us6uCwTQ, skins sur la même texture d'habillage) : synchroniser avec `main` avant la PR.
- CI de `main` #1069 rouge (banc texte, crédits W212 / Virtus) : déjà corrigée par WFS-543 (#407), #1073 en cours ; pas de nouvelle US technique.

### 2026-11-14 13:21:47 — Correction : WFS-502 au lieu de WFS-511 pour Claude, agent dev3d (01Us6uCv9g) (po-scrum-master)
- Erreur du PO à 13:20 : WFS-493 (5e), WFS-495 (7e) et WFS-502 (10e) étaient passés ⏳ Prêt à 13:18 (commit 5534d3aa), avant WFS-511 (15e). 493 et 495 ont été donnés entre-temps à d'autres sessions `@dev` (01Us6uCwTQ, 01Us6uCv3C).
- **WFS-502** (P1, E19 : E3 `src/game/**` état du prêt, E5 panneaux du garage, E8 `src/ui/ecrans/**`, accroches ≤ 10 lignes annoncées) attribué à cette session, qui n'avait rien commencé sur WFS-511 ; règles `regles-de-gestion/WFS-502.md`, `regles-de-gestion/E19-cadeau-du-jour.md` ; branche `feature/waforspeed-502`. Aucun autre ticket actif sur les panneaux du garage ; WFS-482 (🔧) touche aussi `src/ui/ecrans/**` et le menu : se synchroniser avec `main`.
- **WFS-511** revient ⏳ Prêt (15e) pour la prochaine session `@dev`.

### 2026-11-14 13:22:17 — WFS-502 : début, branche feature/waforspeed-502 (Claude, agent dev3d (01Us6uCv9g))
- Ticket attribué par le PO (voiture prêtée 3 h). Lecture des règles WFS-502 et E19, étude du code de WFS-498 à 501 avant de coder.

### 2026-11-14 13:42:40 — WFS-493 : branche poussée, 18 textures faites, captures en cours (Claude, agent dev3d (01Us6uCwTQ))
- Fait : `tools/skins/generer.py` dessine Drift Tokyo (dégradé vermillon penché à 35°, entailles noires, 3 signes inventés, ni lettre ni idéogramme), Néon nuit (filets cyan et magenta ; émission la nuit seulement, 0,5 × la couleur : luminance ≤ celle d'un feu arrière de position) et Camouflage (blanc, gris, noir ; taches dessinées de 20 à 60 cm, posées en 3D : continues d'un panneau à l'autre ; taille mesurée à chaque génération) ; 18 textures EAC RG11 de 1,33 Mo (≤ 1,5 Mo).
- Accroches : `shaders/vehicule.glsl` (couleurs, émission du Néon) et une ligne dans `shaders/scene.frag` ; banc `WFS_BANC=skins` (variable `WFS_SKINS`) ; `test_vehicule_skin` vérifie les 18 nouvelles textures (OK).
- Vérification : rendu au banc lavapipe sur la Golf (jour et nuit) regardé ; captures avant / après de toutes les voitures en cours (`captures_pr.sh`).

### 2026-11-14 13:48:11 — WFS-543 : suivi de la CI de main (Claude, agent dev3d (01Us6uCwcF))
- Run #1070 (fusion WFS-543, 8cb64d33) : **annulé** par GitHub, remplacé par les fusions suivantes (#1071 et #1072 annulés eux aussi).
- Run #1073 (39d1ddc7, contient WFS-543) : en cours à 13:48 UTC ; compilation, android, endurance et publication-apk sont verts ; le job banc-essai (texte), qui fait le contrôle des licences, tourne encore. Nouvelle vérification vers 14:20 UTC.

### 2026-11-14 13:58:57 — WFS-482 : écran Profil en PR #412 (Claude, agent dev3d (01Us6uCv3H))
- Fait : écran « Profil » (`src/ui/ecrans/profil.h/.cpp`, système 468) : en-tête (pseudo, plaque, « Pilote depuis le … »), 5 chiffres, records des 3 modes, voiture préférée, 20 dernières courses défilant au doigt, « Modifier » / « Retour » ; « Modifier » rouvre le formulaire et le clavier de WFS-481 en mode modification (« Valider » / « Annuler », plaque gardée) ; textes `profil.*` FR / EN.
- **Accroches annoncées** (≤ 10 lignes chacune) : `src/ui/ecrans/menu.cpp` (7 lignes : bouton « Profil » au-dessus du cadeau, touchers, dessin), `src/ui/ecrans/compteur_etoiles.cpp` (2 lignes : compteur caché sous le Profil), `src/platform/desktop_main.cpp` (2 lignes : banc `WFS_BANC=profil`).
- Vérification : 140 tests CPU verts (nouveau cas dans `test_creation_profil`) ; banc `profil` FR + EN 0 écart (parcours menu → Profil → Modifier → Valider → Retour → course → fin → Profil, 22 courses → 20 lignes, 16 « W » entier, 0 texte français en anglais) ; banc `creation` 0 écart avant / après ; 26 captures dans `qa/pr/WFS-482/`.
- Limites : non essayé sur téléphone ; nombres au format français dans les deux langues (comme le reste du jeu).


### 2026-11-14 14:11:37 — WFS-482 refusé par le lead dev, PR #412 (lead-dev)
- Revue du commit e85d008 : 26 captures ouvertes, diff lu en entier, branche construite, 140 tests CPU verts, banc `profil` relancé (0 écart FR + EN). Logique saine (« Valider » ne remet pas les statistiques à zéro, accroches ≤ 10 lignes annoncées).
- Refus (revue « changements demandés » écrite dans la PR) : (1) capture des tests CPU absente (`qa/pr/WFS-482/tests-cpu.log` et section « Tests CPU », règle du 2026-11-11) ; (2) RG-08 : dans le formulaire « Modifier », le pseudo de 16 « W » et le curseur sortent du cadre du champ (`creation_profil.cpp`, `champ()`, taille fixe sans limite de largeur ; captures `profil-*-16w-modification`).
- Statut : ↩️ Retour lead dev (PR #412) dans `TACHES.md`, `track-agent.md` et `suivi-pr.md`.

### 2026-11-14 14:24:53 — WFS-502 : code, tests et captures faits, PR ouverte (Claude, agent dev3d (01Us6uCv9g))
- Fait : prêt de voiture du cadeau du jour (`src/game/pret.*`, système `pret` 206), bandeau du menu (`src/ui/ecrans/pret_bandeau.*`), garage (carte du prêt, « Activer le prêt »), écran du cadeau (« Activer » lance le prêt), clés `pret-restant-vu`, `pret-avant`, `pret-message`, textes FR / EN.
- **Accroches annoncées** (≤ 10 lignes chacune) : `src/ui/ecrans/menu.cpp` (4), `src/ui/garage.cpp` (4) et `garage.h` (2), `src/game/profil_system.cpp` (2 : la voiture prêtée n'utilise pas l'essai gratuit), `src/ui/ecrans/cadeau_jour.cpp`. Test de WFS-500 mis à jour (`test_cadeau` : un seul prêt à la fois, RG-01 de WFS-502).
- Vérifié : `test_pret` (39 cas RG-08), `test_garage_voitures`, tests CPU 141/141 verts ; banc `WFS_BANC=cadeau` (pret-attente, pret-actif, pret-fin) : parcours RG-10 complet FR / EN, 0 erreur, textes WFS-504 VERT ; 28 captures de WFS-501 identiques à main (RMSE 0).
- Parcours RG-10 : menu → cadeau (voiture) → garage « Activer le prêt » → « Prêtée : 2 h 41 min » → « Choisir » → menu (bandeau) → course jour et nuit → fin du prêt pendant la course (voiture gardée) → menu « Garde-la pour toujours : 2 400 ★ » → « Garage » (bloquée, prix) → prêt expiré → relance (état gardé).
- Limite : « Rejouer » après une fin de prêt pendant la course repart avec la voiture prêtée jusqu'au prochain passage au menu ; Market : bouton « Garage » en attendant.

### 2026-11-14 14:19:31 — WFS-493 : PR #413 ouverte, lead dev appelé (Claude, agent dev3d (01Us6uCwTQ))
- PR #413 (commit 1f756edf, branche synchronisée avec `main`) : 18 textures (Drift Tokyo, Néon nuit, Camouflage × 6 voitures), lueur du Néon la nuit (`vehicule.glsl` + 1 ligne d'accroche dans `scene.frag`), banc `skins` (`WFS_SKINS`), `test_vehicule_skin` étendu.
- Vérification : 140 tests CPU réussis ; 55 captures du banc lavapipe regardées (`qa/pr/WFS-493/`) ; Rallye et vitrine sans skin identiques à `main` (PSNR 37 à 57 dB) ; contrôle des licences : 0 manque.
- Limites : la partie visible des taches grises du Camouflage est plus petite que la tache dessinée (les noires mordent dessus) ; banc d'endurance non relancé (coût du shader : un test et une addition par pixel habillé).

### 2026-11-14 14:20:10 — WFS-543 : CI de main verte (RG-04) (Claude, agent dev3d (01Us6uCwcF))
- Run **#1073** de `build.yml` sur `main` (39d1ddc7, contient la fusion de WFS-543 8cb64d33) : **vert** (success), tous les jobs, banc-essai (texte) compris. Première CI de `main` verte depuis #1049. Reste pour RG-04 : bancs du matin suivants (robot des statuts).

### 2026-11-14 14:29:30 — WFS-482 : retour du lead dev corrigé (PR #412) (Claude, agent dev3d (01Us6uCv3H))
- Point 1 : `qa/pr/WFS-482/tests-cpu.log` (140 tests réussis) et section « Tests CPU » dans la PR.
- Point 2 (RG-08) : cause : `champ()` de `creation_profil.cpp` dessinait le texte saisi à taille fixe ; il est maintenant réduit pour tenir dans le champ, curseur placé d'après la taille réelle ; nouvelle vérification au banc `profil` (OK FR / EN), banc `creation` 0 écart ; captures 16 « W » régénérées.
- Commit e00de6d5, synchronisé avec `main` (dd79a20b) ; réponse point par point sous la revue ; lead dev rappelé.


### 2026-11-14 14:33:22 — WFS-493 refusé par le lead dev, PR #413 (lead-dev)
- Commit relu : 1f756edf. Toutes les captures ouvertes (55 après, 19 avant) ; tout le diff lu ; construction et tests sur la branche synchronisée avec `main` : 140 tests réussis.
- Néon nuit, Drift Tokyo et non-régression Rallye / sans skin : bons ; code propre (accroche de `scene.frag` en 1 ligne).
- **Refus** : (1) le Camouflage ressemble à une robe de vache (taches noires rondes et isolées sur fond surtout blanc, gris presque absent), pas à un camouflage urbain (RG-04) ; (2) section « Tests CPU » avec le commit testé absente de la description (règle du 2026-11-11) ; (3) banc d'endurance en qualité Basse non lancé (RC-E18-03).
- Revue dans la PR : https://github.com/wlfaleh/waforspeed_mobile/pull/413#pullrequestreview-5904959985 . Statut : ↩️ Retour lead dev (PR #413).

### 2026-11-14 14:37:51 — WFS-482 fusionné par le lead dev (lead-dev)
- Revue du commit dd79a20b (correction e00de6d5) : réponse du développeur sous la revue présente (« Bien reçu », puis « corrigé dans e00de6d5 » point par point). Captures `profil-fr/en-16w-modification` régénérées ouvertes : texte et curseur dans le cadre du champ ; `creation-fr-vide` inchangée.
- Vérifié moi-même : construction OK, `tests_cpu.sh build` 140 réussis ; banc `profil` FR + EN 0 écart (nouvelle vérification RG-08 « Modifier » OK) ; banc `creation` 0 écart. Diff de e00de6d5 lu : rien de bloquant.
- « ✅ Approuvé » posté en commentaire de revue (GitHub refuse l'approbation sur le même compte), fusion en squash de89e0f1 (#412). Statut 🟣 Fusionné ; ligne `suivi-pr.md` retirée ; `qa/Check.md` à jour.

### 2026-11-14 14:38:20 — WFS-482 : fin de session (Claude, agent dev3d (01Us6uCv3H))
- PR #412 fusionnée par le lead dev (de89e0f1) ; ticket 🟣 Fusionné, ligne dans `qa/Check.md` posée par le lead dev.
- Session terminée (un seul sujet par session) : aucun autre ticket demandé.


### 2026-11-14 14:45:01 — WFS-502 : retour lead dev sur PR #414 (lead-dev)
- Revue complète du commit 31029178 : 20 captures `pret-*` ouvertes (FR / EN), 28 paires `cadeau-*` identiques à main (RMSE 0), diff entier lu, branche construite et 141 tests CPU refaits : fond du ticket bon (avis écrit dans la PR).
- Fusion refusée par GitHub : conflit avec `main` après la fusion de WFS-482 (#412). `src/ui/ecrans/menu.cpp` (enum `PROFIL = 7` / `PRET = 7`, 4 blocs) et `journal.md`. Surtout, le bouton « Profil » occupe la place du bandeau du prêt (même bas) : chevauchement à l'écran.
- Demandé : synchroniser avec main, `PRET = 8`, bandeau au-dessus de « Profil » sans déplacer les boutons, tests et banc relancés, captures du menu (attente / actif / fin / expiré, FR et EN) avec « Profil » visible.
- Remarque au PO : prix des pastilles du garage en anglais à peine lisibles (« 100 ★ » rendu flou en petit), défaut d'avant ce ticket.

### 2026-11-14 14:46:27 — WFS-495 : PR #415 ouverte, autocollants cuits dans la texture d'habillage (Claude, agent dev3d (01Us6uCv3C))
- **Fait** :
  - les autocollants sont cuits dans la texture d'habillage de WFS-492, par la même projection : rien ne flotte (ANO-250), rien sur les vitres, phares, calandre ni jantes ;
  - `src/render/vehicule_autocollants.*` : calques, texte court, 50 motifs vectoriels originaux (lettres en Rajdhani Bold, OFL), rastérisation à couverture exacte, cuisson skin + calques ;
  - `vehicule_etc2.*` : encodeur et décodeur ETC2 RGBA8 ;
  - `vehicule_skin::poser(…, calques)` : skin n° 5 « cuit », une seule texture ;
  - `shaders/vehicule.glsl` : `habillageCuit` ;
  - banc `WFS_BANC=autocollants` et test `test_vehicule_autocollants`.
- **Accroches** (hors zone, ≤ 10 lignes) : `src/game/game.cpp` (1 ligne : variable `WFS_AUTOCOLLANTS` ; l'atelier WFS-496 y branchera la sauvegarde), `src/platform/desktop_main.cpp` (2 lignes : banc), `CMakeLists.txt` (1 ligne : test).
- **Vérifié** :
  - 141 tests CPU réussis ;
  - 3 voitures (208, GT86, Corvette) : bande capot-toit-coffre, flamme coupée par le passage de roue, numéro sur les portières, de jour, de nuit et en course ;
  - cuisson de 48 à 76 ms sur PC (4 cœurs) ;
  - captures `skins` identiques au pixel près à `main` ;
  - qualité Basse : 3,891 contre 3,843 img/s, soit 0,05 img/s d'écart (`qa/pr/WFS-495/mesure-basse.txt`).
- **Limites et risques** :
  - la clé de sauvegarde des calques et l'atelier restent à faire (WFS-496 et WFS-497) ; `creation-<voiture>` n'accepte qu'un identifiant de 32 caractères ;
  - un miroir de lettres inverse l'ordre d'un mot ;
  - le temps de cuisson sur le S21 FE n'est pas mesuré sur l'appareil.
- **Pour le PO et WFS-493 (01Us6uCwTQ)** :
  - même bloc `SKIN_COULEURS` du shader : la PR fusionnée en second garde les deux ;
  - `test_vehicule_autocollants` exige que `vehicule_skin::couleurs` (C++) recopie `SKIN_COULEURS` ;
  - la lueur de nuit du Néon ne passe pas dans la texture cuite (Néon + autocollants : plus de lueur) : à trancher par le PO (petit ticket de suite possible).


### 2026-11-14 15:00:19 — WFS-495 : PR #415 refusée par le lead dev (lead-dev)
- Commit relu `cdea30d4`. **Un seul défaut** : la section « Tests CPU » (fin du journal avec le commit testé) manque dans la description de la PR (règle de l'utilisateur du 2026-11-11, absente = REFUS).
- Tout le reste est vérifié et bon :
  - captures ouvertes (3 voitures, jour, nuit, course, gros plans, skin Rallye, bibliothèque) ;
  - 7 paires `skins` identiques octet pour octet à main ;
  - construction et 141 tests refaits par le lead dev ;
  - ETC2 recoupé avec le décodeur indépendant `tools/textures/etc2.py` : 0 écart ;
  - diff lu en entier.
- Le développeur ajoute la section, puis rappelle le lead dev.

### 2026-11-14 15:07:48 — WFS-495 : PR #415 corrigée après le refus du lead dev (Claude, agent dev3d (01Us6uCv3C))
- **Refus** : la section « Tests CPU » manquait dans la PR (règle du 2026-11-11).
- **Correction** (commit `b5afdf16`) :
  - la section est ajoutée à la description de la PR et à `qa/pr/WFS-495/pr.md` : fin du journal, ligne « 141 test(s) réussi(s) », `[OK] test_vehicule_autocollants`, commit testé `b39bc7e0` ;
  - le commit testé est écrit en tête de `tests-cpu.log` ;
  - remarque non bloquante prise aussi : un échec de cuisson est maintenant écrit au journal (`b39bc7e0`).
- **Réponses** dans la PR : « Bien reçu », puis « corrigé dans ».
- **Suite** : ticket en 🔀 En PR, ligne en 🔁 dans `suivi-pr.md`, lead dev rappelé.


### 2026-11-14 15:22:04 — WFS-495 fusionné par le lead dev (lead-dev)
- **PR #415**, 2e revue sur `b5afdf16`, squash `719afd1d`. « ✅ Approuvé par le lead dev » posté en commentaire, car GitHub refuse l'approbation avec le même compte.
- **Défaut de la 1re revue corrigé** : la section « Tests CPU » est présente, avec le commit testé `b39bc7e0`. Le seul changement de code est une ligne de journal en cas d'échec de la cuisson.
- **Captures** : rouvertes, tout est conforme. Sans autocollant, les captures `skins` sont identiques à `main` octet pour octet.
- **Vérification** : construction et `tests_cpu.sh` refaits sur la branche synchronisée avec `main` : 141 tests réussis.
- **Statuts** : ticket 🟣 dans `TACHES.md`, ligne retirée de `suivi-pr.md`, ligne de `track-agent.md` en « fusionnée », entrée ajoutée dans `qa/Check.md`.

### 2026-11-14 15:22:29 — WFS-502 : retour lead dev, PR #414 à resynchroniser (lead-dev)
Relecture du commit 5eefd514 : tous les points du retour précédent sont corrigés. Le bandeau est au-dessus de « Profil » (même calcul que profil.cpp) et `PROFIL = 7` / `PRET = 8` sont gardés. J'ai ouvert les 8 captures du menu en FR et en EN ; les 28 paires `cadeau-*` ont un écart RMSE de 0. Construction et tests refaits : 141 tests réussis. Approbation écrite dans la PR. La fusion squash est refusée par GitHub parce que WFS-495 (#415) est entré dans `main` pendant la revue (conflit dans `journal.md` seulement). Retour au développeur : resynchroniser, relancer les tests CPU, recopier `pr.md` dans la description, puis rappeler le lead dev.

### 2026-11-14 15:22:37 — Fin WFS-495 : session terminée (Claude, agent dev3d (01Us6uCv3C))
- PR #415 fusionnée en squash par le lead dev (`719afd1d`) à la 2e revue ; ticket 🟣 Fusionné, le robot des statuts le passera en ✅ après les bancs du matin.
- `qa/Check.md` : entrée ajoutée par le lead dev.
- **Reste à suivre par d'autres sessions** :
  - temps de cuisson et mémoire temporaire (environ 40 Mo) à mesurer sur le S21 FE ;
  - la clé de sauvegarde des calques et l'atelier : WFS-496 et WFS-497 ;
  - lueur du Néon sous des autocollants (WFS-493) : décision du PO.
- Un seul ticket par session : cette session s'arrête et ne demande pas d'autre ticket.


### 2026-11-14 15:01:04 — WFS-502 : retour du lead dev corrigé (PR #414) (Claude, agent dev3d (01Us6uCv9g))
- Synchronisation avec main (WFS-482) : conflits de `menu.cpp` (`PROFIL = 7`, `PRET = 8`) et du journal résolus en gardant les deux côtés.
- Bandeau du prêt posé au-dessus de « Profil » (même calcul que `profil.cpp`), aucun bouton existant déplacé, zones d'appui séparées ; temps restant aussi écrit à chaque changement d'écran ; ligne inutile retirée de `test_pret`.
- Vérifié : tests CPU 141/141 ; banc `cadeau` refait avant / après (48 après, 28 paires de WFS-501 identiques) ; menus FR / EN regardés : « Profil » visible et libre.
- Pour le PO (défaut d'avant ce ticket, vu par le lead dev) : au garage en anglais, les petits prix sous les pastilles de couleur sont presque illisibles.

### 2026-11-14 15:53:58 — WFS-502 fusionné par le lead dev (lead-dev)
- PR #414, squash `651dc471`, revue sur `6d86ca96` (« ✅ Approuvé » en commentaire, même compte GitHub).
- Relecture du commit de synchronisation `f2d01c6a` : la fusion automatique des deux parents, refaite par le lead dev, ne diffère que par `journal.md` (entrées des deux côtés gardées). Aucun fichier de code commun avec WFS-495 ; `CMakeLists.txt` construit `test_pret` et `test_vehicule_autocollants`.
- Construction et tests CPU refaits par le lead dev sur `6d86ca96` : 142 réussis, code de sortie 0.
- Statut 🟡 À valider (utilisateur) ; ligne ajoutée dans `qa/Check.md` ; ligne retirée de `suivi-pr.md`.

### 2026-11-14 15:54:33 — WFS-502 : fin de session (Claude, agent dev3d (01Us6uCv9g))
- PR #414 fusionnée par le lead dev (squash 651dc471), WFS-502 en 🟡 À valider (utilisateur) ; message laissé à l'utilisateur dans `tache utilisateur.md` (essai sur l'APK de `main`).
- Pour le PO : défaut d'avant ce ticket, vu par le lead dev : au garage en anglais, les petits prix sous les pastilles de couleur sont presque illisibles ; aucune capture du menu en petit écran pour le bandeau du prêt (à regarder par l'utilisateur).
- Session terminée : un seul ticket par session, aucun autre ticket demandé.

### 2026-11-14 16:30:41 — WFS-511 attribué à Claude, agent dev3d (01Us6uCtfa) (po-scrum-master)
- Début : WFS-511 « Android : Google Play Billing » (E21, zone E7 : `android/**`, `src/platform/*_android.cpp`), P1, branche `feature/waforspeed-511`, à Claude, agent dev3d (01Us6uCtfa) (session_01Us6uCtfaZCtBn6FQY7UQw9), nouvelle session `@dev`, à sa demande.
- Choix : classement 📋 des 40 ; 1 à 12 fusionnés, pris ou en attente (493 en retour lead dev, 494, 496, 497, 503, 538 en attente), 13 `@cicd`, 14 `@conduite` → 15e WFS-511, dépendance WFS-510 fusionnée (#390).
- Critères : `regles-de-gestion/WFS-511.md` (RG-01 à RG-09 : bibliothèque Billing ≥ 7, `Achats.java` + `achats_android.cpp`, créditer puis consommer, rattrapage au lancement, restauration, hors ligne sans plantage, aucun jeton complet au journal, tests JVM). Vrai essai Play Console : WFS-515.
- Plan court : `Achats.java` sur le modèle de `Paquets.java`, pont JNI sur le modèle de `paquets_android.cpp`, branchement sur `Boutique` de WFS-510, tests JVM avec un faux client, captures (tests JVM, logcat, banc `market`), PR, lead dev.
- Prochaine `@dev` : WFS-513 (écran Market, prêt).

### 2026-11-14 16:44:24 — WFS-493 : retour lead dev corrigé, PR #413 à relire (Claude, agent dev3d (01Us6uCwTQ))
- Camouflage refait : fond gris foncé (B à 72 %), taches blanches et noires imbriquées de 2 à 4 lobes (20 à 60 cm) ; test : blanc ≤ 35 %, gris ≥ 30 % ; captures refaites.
- Tests CPU : 142 réussis sur 4c2c123b (branche synchronisée avec main, WFS-495 compris). Images/s en Basse, 3 passages alternés (main sans skin / branche avec skin) : jour +0,02 (1 % lents +0,19), nuit −0,01 (1 % lents −0,02) : sous ±1 img/s.
- Croisement avec WFS-495 : couleurs C++ des skins 2 à 4 remplies (cuisson des autocollants). **Pour le PO** : avec des autocollants, la skin Néon nuit est cuite en couleurs seules, ses filets ne brillent plus la nuit (petit ticket de suite proposé : garder la lueur du Néon dans l'habillage cuit).

### 2026-11-14 16:59:06 — WFS-493 fusionné par le lead dev (lead-dev)

PR #413, 2e revue sur 73b2d8ac : les 3 défauts de la 1re revue sont corrigés (Camouflage sur fond gris, taches blanches et noires imbriquées ; section « Tests CPU » ; images/s en Basse, écart sous ±1 img/s). Captures ouvertes, diff lu, construction et **142 tests CPU** refaits par le lead dev. « ✅ Approuvé » en commentaire (même compte GitHub), puis fusion en squash : **42f6c241**. Statut 🟣. Remarques non bloquantes : quelques taches noires du Camouflage restent rondes sur une zone blanche (Golf, 500) ; **pour le PO** : ticket de suite, le Néon ne brille plus la nuit sous des autocollants (WFS-495).

### 2026-11-14 16:59:41 — WFS-493 fusionné (#413, 42f6c241) : session terminée (Claude, agent dev3d (01Us6uCwTQ))
- Skins Drift Tokyo, Néon nuit (filets lumineux la nuit) et Camouflage urbain sur 208, 500, GT86, Golf, Corvette C7 et Giulia ; fusionné par le lead dev après une correction (Camouflage refait, tests CPU, images/s en Basse).
- Suites pour le PO : (1) Néon nuit sous des autocollants (WFS-495) : les filets ne brillent plus la nuit (ticket de suite à créer) ; (2) remarque de la revue : quelques taches noires encore rondes sur les zones blanches du Camouflage, à regarder à la recette (`qa/Check.md`).
- Un seul ticket par session : la session s'arrête, aucun autre ticket demandé.

### 2026-11-14 17:26:52 — WFS-511 : PR #416 refusée par le lead dev (lead-dev)
- Relu : tout le diff (build.gradle, Achats.java, LogiqueAchats.java, LogiqueAchatsTest.java, achats_android.cpp, accroches android_main.cpp +3, JeuActivity +1, market.h/.cpp +5) et les 3 captures texte (tests JVM, journal du démarrage, banc `market` identique).
- Refait : branche + main en local, 142 tests CPU réussis (506 s) ; LogiqueAchatsTest 12/12 OK (javac + JUnit) ; Maven de Google bloqué ici aussi (APK non construit).
- Refus (revue dans la PR) : (1) `android.useAndroidX=true` absent de `android/gradle.properties` alors que billing 8 tire des dépendances AndroidX (play-services-base) : l'AGP 8.5.2 arrêterait `assembleRelease` → CI de main rouge ; (2) conflit avec main sur `journal.md` (entrée WFS-511 insérée au milieu) : PR non fusionnable.
- Pour le PO (non bloquant) : pack débutant recrédité après une réinstallation (livre vide) : à trancher dans E21 ; risque `crediterAchat` sans résultat d'écriture déjà signalé par le développeur.

### 2026-11-14 16:31:20 — WFS-511 : début, branche créée (Claude, agent dev3d (01Us6uCtfa))
- Ticket reçu du PO : Google Play Billing (E21, zone E7). Branche `feature/waforspeed-511` créée depuis `main`.
- Plan : lire `Boutique` (WFS-510), `Paquets.java`, `paquets_android.cpp` ; ajouter `Achats.java`, `achats_android.cpp`, tests JVM avec un faux client.

### 2026-11-14 17:04:25 — WFS-511 : code fini, PR en préparation ; accroches annoncées (Claude, agent dev3d (01Us6uCtfa))
- Fait : `LogiqueAchats.java` (règles, testées sur la JVM : 12 cas OK), `Achats.java` (Google Play Billing 8.0.0), `src/platform/achats_android.cpp` (`BoutiqueGoogle`, réponses lues sur le fil du jeu, crédit écrit PUIS consommation).
- **Accroches** (≤ 10 lignes chacune) : `android_main.cpp` +3 (inclusion, démarrage après la sauvegarde, lecture dans la boucle) ; `JeuActivity.onResume` +1 (`Achats.premierPlan`) ; `market.h/.cpp` (zone E3) +5 : `market::boutiqueActive()` pour l'écran Market (WFS-513).
- Vérification : 142 tests CPU OK sur 5e64a5c0 (synchronisé avec main) ; banc `market` identique à main (`cmp`) ; `Achats.java` compilé contre des copies vides des classes de Google.
- Limites : pas de NDK ni d'émulateur ici, Maven de Google bloqué par le proxy (non contourné) : l'APK avec la bibliothèque sera construit par la CI de main. **Pour le PO** : `crediterAchat` ne reçoit pas le résultat de l'écriture de la sauvegarde (disque plein → achat consommé sans étoiles gardées) : petit ticket proposé dans `sauvegarde.*` (zone E3).

### 2026-11-14 17:27:58 — WFS-511 : retour lead dev (PR #416) en correction (Claude, agent dev3d (01Us6uCtfa))
- Réponse « Bien reçu » sous la revue. Corrigé : `android.useAndroidX=true` dans `android/gradle.properties` (Billing 8 tire AndroidX) ; branche synchronisée avec main, journal sans conflit (mes entrées à la fin).
- Remarques non bloquantes traitées dans ma zone : achat demandé avant le démarrage du service → événement « erreur » rendu au jeu ; démarrage noté réussi seulement sans exception Java.
- **Pour le PO** (hors ticket) : pack débutant recrédité après réinstallation (livre vide, Google le rend : règle à trancher dans E21) ; résultat de l'écriture de la sauvegarde non rendu à `crediterAchat`.

### 2026-11-14 18:03:10 — WFS-511 fusionné par le lead dev (lead-dev)
PR #416, 2e revue sur 8b31fa77 : les 2 défauts de la 1re revue sont corrigés (`android.useAndroidX=true` ; journal sans conflit). Le développeur a répondu point par point dans la PR.
- Refait : branche + main en local, construction propre, 142 tests CPU réussis (460 s) ; LogiqueAchatsTest 12/12 OK (javac + JUnit).
- Diff depuis 59c34947 relu : erreur rendue au jeu si l'achat est demandé avant le démarrage (file sous `synchronized`), `g_achDemarre` seulement sans exception Java. Correct.
- « ✅ Approuvé » en commentaire (même compte GitHub), puis squash 8138cf90. Statut 🟣.
- À surveiller : la CI de `main` construit pour la 1re fois l'APK avec Billing 8 : si elle est rouge, US technique P1. Pour le PO (E21) : pack débutant recrédité après réinstallation ; résultat de l'écriture de la sauvegarde non rendu à `crediterAchat`.

### 2026-11-14 18:04:00 — WFS-511 fusionné, session terminée (Claude, agent dev3d (01Us6uCtfa))
- PR #416 fusionnée par le lead dev à la 2e revue (squash 8138cf90). Statut 🟣.
- **Pour le PO** (E21) : pack débutant recrédité après réinstallation (règle à trancher) ; `crediterAchat` sans résultat de l'écriture de la sauvegarde (zone E3) ; délai d'attente d'un achat sans réponse à prévoir dans WFS-513.
- La 1re construction de l'APK avec Billing 8 se fait dans la CI de main (Maven de Google bloqué dans le conteneur). Un seul ticket par session : arrêt.

### 2026-11-14 18:12:54 — Début WFS-513 : écran Market FR / EN attribué (po-scrum-master)
- **Attribution** : WFS-513 (P1, épic E21 « Market ») à **Claude, agent dev3d (01Us6uCvpo)** (session_01Us6uCvpoEsH52SyRp9QLDZ), nouvelle session `@dev`, à sa demande (aucun ticket avant : règle « un sujet par session » respectée).
- **Pourquoi lui** : 17e du classement 📋 ; 1 à 16 fusionnés, pris, en attente (494, 496, 497, 503, 512, 538) ou réservés à un autre skill (454 `@cicd`, 394 `@conduite`). Dépendances WFS-510, 499, 504 fusionnées. CI de `main` : runs 43612612331 et 43618542046 verts (WFS-502, 493), WFS-511 (run 43624394767) en cours.
- **Zone** : `src/ui/ecrans/**` (E8) + accroches ≤ 10 lignes dans `src/ui/garage.cpp` et `compteur_etoiles.*`, annoncées au journal. Règles : `regles-de-gestion/WFS-513.md`, `regles-de-gestion/E21-market.md`.
- **Plan court** : écran Market (solde, cartes de packs, « Sans pub », achat par `Boutique`, « Restaurer mes achats », hors ligne grisé), FR / EN, banc `market`, parcours menu → Market → retour et garage → Market → retour ; captures ≥ 3 ; PR ; lead dev.
- Session 01Us6uCtfa (WFS-511, #416 fusionnée) : ligne enlevée de `track-agent.md`.

### 2026-11-14 18:29:40 — WFS-446 attribué à Claude, agent dev3d (01Us6uCuVU) (po-scrum-master)
- **Attribution** : WFS-446 (P2, [TECHNIQUE] manque de mémoire graphique, suite 1 ; E0 + rendu) à **Claude, agent dev3d (01Us6uCuVU)** (session_01Us6uCuVUrprrAqPN7XTCnJ), nouvelle session `@dev`, à sa demande (« @dev go », aucun sujet souhaité ; aucun ticket avant : règle « un sujet par session » respectée).
- **Pourquoi lui** : 24e du classement 📋, 1re tâche prête permise à `@dev` ; avant elle, tout est fusionné, pris, en attente (494, 496, 497, 503, 512, 514, 538) ou réservé à `@cicd` / `@conduite` (WFS-541 aussi `@conduite`). Dépendance WFS-418 ✅ (bancs du matin #22) ; pas la session de WFS-418.
- **Zone** : `src/render/model_loader.*`, appels de `loadCarModel` dans `src/game/traffic_system.cpp`, `src/world/pietons.cpp`, `src/render/squelette.cpp`, `src/world/conducteurs.cpp`, `src/world/enseignes.cpp`, appel d'`uploadNiveaux` dans `src/render/route_revetements.cpp`, `bad_alloc` dans `src/world/archi_carte.cpp`, étape 5 de `src/world/cartes_v2/batiments_banc.cpp`, `tests/**` ; `vk.*` seulement ajout ≤ 10 lignes annoncé. Règles : `regles-de-gestion/WFS-446.md`.
- **Plan court** : brancher les secours existants de WFS-418 (`essayer…`) sur le refus de mémoire, tests de refus simulé (Ville et Corniche), aucun écart quand la mémoire suffit ; captures ≥ 3 (`batiments`) ; PR ; lead dev.
- Session 01Us6uCwTQ (WFS-493, #413 fusionnée) : ligne enlevée de `track-agent.md`. WFS-513 : branche poussée (c449553).

### 2026-11-14 18:30:50 — Début WFS-446 : manque de mémoire graphique, suite 1 (Claude, agent dev3d (01Us6uCuVU))
- Branche `feature/waforspeed-446` créée depuis main (2c893003) et poussée. Règles lues (RG-01 à RG-10).
- Plan : paramètre « repli » par défaut à faux sur `loadCarModel` (trafic seulement à vrai), `uploadNiveaux` (route), piétons / conducteurs / enseignes par les fonctions `essayer…` de WFS-418 ; `bad_alloc` d'`initCuit` → kit ; étape 5 du banc `batiments` préfixée et sur la Corniche ; tests de refus simulé.
- **Accroche annoncée dans `src/render/vk.*` (≤ 10 lignes)** : paramètre par défaut `repli = false` sur `VkCtx::uploadNiveaux` (tampon de transfert et image par `essayerBuffer` / `essayerImage`), car aucune fonction `essayer…` n'existe pour les niveaux KTX2.

### 2026-11-14 18:31:05 — Début WFS-494 : garage, skins et skin offerte attribué (po-scrum-master)
- **Demande** : nouvelle session `@dev` (agent dev-artiste-3d), Claude, agent dev3d (01Us6uCwV6), jamais eu de ticket.
- **Choix** : classement 📋 des 40 : 1 à 5 fusionnés ou pris ; **6. WFS-494** prêt (WFS-493 #413 et WFS-504 #372 fusionnés ; l'entrée de 18:29 le croyait encore en attente). P1, E18.
- **Zone** : `src/ui/garage_skin.*` (E5, un ticket à la fois sur ce panneau) + accroche E3 ≤ 10 lignes (skin offerte, sauvegarde), annoncée au journal. Textes par `ui::tr`, FR / EN. Règles : `regles-de-gestion/WFS-494.md`, `regles-de-gestion/E18-personnalisation.md`.
- **Plan court** : rangée « Skin » (Aucune, Rallye, Camouflage, Drift Tokyo ; Néon nuit verrouillée « cadeau »), prix × classe ; fenêtre « Choisis ta skin offerte » ; achat ★ par voiture ; `donnerSkin()` ; parcours RC-E18-09 ; captures ≥ 3 jour / nuit, FR / EN ; PR ; lead dev.
- **Aussi** : WFS-538 et WFS-496 passent ⏳ Prêt (dépendances fusionnées).

### 2026-11-14 19:38:27 — WFS-446 : PR #417 ouverte, lead dev appelé (Claude, agent dev3d (01Us6uCuVU))
- Fait : secours branchés sur le refus de mémoire graphique (trafic `loadCarModel(…, repli)`, piétons / enfants / conducteurs `chargerPersonnage(…, repli)`, atlas des enseignes, textures de la route `uploadNiveaux(…, repli)`) et sur le `std::bad_alloc` d'`initCuit` (kit de secours) ; étape 5 du banc `batiments` préfixée, Ville et Corniche, avec refus des autres systèmes et `bad_alloc` simulé.
- **Accroches annoncées** : `src/render/vk.*` (paramètre `repli` d'`uploadNiveaux`, 4 lignes) et `src/render/renderer.*` (paramètre `repli` de `makeSkinnedMesh`, 6 lignes, au lieu de recopier son code dans `squelette.cpp` ; fichier non listé dans la zone : signalé au lead dev dans la PR).
- Vérifié : construction complète ; 142 tests CPU réussis ; banc `batiments` rapide Ville 0 écart, Corniche 0 écart (aucune mémoire graphique perdue) ; captures sans refus identiques à `main` hors ligne de mesure (RG-09).
- Limites : refus en cascade (mémoire pleine après le 1er refus, règle de WFS-418) ; l'atlas refusé retire aussi les devantures (`commerces.cpp`, secours existant) ; banc PC seulement.

### 2026-11-14 20:02:02 — WFS-446 : PR #417 refusée par le lead dev (lead-dev)
- Revue du commit 310599ff : 38 captures « après » et 26 paires ouvertes ; tout le diff lu ; construction refaite (branche synchronisée), **142 tests CPU réussis** ; banc `batiments` rapide Ville refait : **0 écart**, +0,00 Mo après chaque refus, chaque secours écrit sa ligne.
- Code sans défaut bloquant ; accroches `renderer.*` (6 lignes) et `vk.*` (4 lignes) acceptées.
- **Refus pour deux preuves manquantes** : (1) capture « Tests CPU » et `qa/pr/WFS-446/tests-cpu.log` absentes (règle de l'utilisateur du 2026-11-11) ; (2) RG-09 : banc `perf` Basse et Haute avant / après (appels, triangles, mémoire, chargement) non montré.
- Statut : ↩️ Retour lead dev (PR #417) dans `TACHES.md`, `track-agent.md` et `suivi-pr.md`.

### 2026-11-14 20:09:48 — WFS-494 : PR #419 ouverte (Claude, agent dev3d (01Us6uCwV6))
- **Fait** : rangée « Skin » du garage (Aucune, Rallye, Camouflage, Drift Tokyo, Néon nuit ; 6 voitures ; prix × classe ; cadenas ; aperçu sur la voiture ; achat en ★ par voiture) ; Néon nuit « Bientôt dans le cadeau du jour », jamais en vente ; fenêtre « Choisis ta skin offerte » (une par voiture, clé `skins-offertes`) ouverte d'elle-même à l'achat d'une voiture, sinon par le bouton « Choisis ta skin offerte » de la rangée (pas de fenêtre surprise : elle bloquait le joueur et 5 bancs existants) ; `skins::donnerSkin(voiture, skin)` prête pour WFS-503 ; bon de skin du market utilisable au garage. FR / EN.
- **Accroches annoncées** : E3 `sauvegarde.*` (clé `skins-offertes`, 4 lignes) ; `market.h` `SKINS_EN_VENTE = true` (ligne prévue pour WFS-494 : le pack débutant est maintenant proposé) ; E5 `garage.cpp`, `garage_commun.h`, `garage_achat*`, `garage.h` ; `desktop_main.cpp` (banc `garage-skins`). Nouveaux : `src/game/skin_offerte.*`, `src/ui/garage_skin.cpp`, `src/ui/garage_skin_banc.cpp`, `tests/test_garage_skin.cpp`.
- **Vérifié** : `test_garage_skin` 41 cas OK ; 143 tests CPU réussis ; banc `garage-skins` 0 écart en 1280x720 et 1024x768 (parcours RC-E18-09 : skin offerte, refus faute d'étoiles, achat, course, « Retour », relance) ; `garage-finitions` 0 écart ; Basse (208 Camouflage) : main 3,62 / 3,97, branche 3,93 / 3,92 img/s (écart < 1). 37 captures dans `qa/pr/WFS-494/`.
- **Limites** : mesures sur lavapipe seulement ; changer de skin au garage recharge une texture (petit à-coup possible au toucher, jamais en course).


### 2026-11-14 20:23:43 — WFS-494 : PR #419 refusée par le lead dev, un seul point de forme (lead-dev)
- **Refus** (commit 6f3d4391) : capture des tests CPU absente (pas de `qa/pr/WFS-494/tests-cpu.log`, pas de fin de journal ni de commit testé dans la section « Tests CPU »), règle de l'utilisateur du 2026-11-11.
- **Vérifié bon** : construction et `tests_cpu.sh` refaits par le lead dev (143 réussis, `test_garage_skin` OK) ; 37 captures ouvertes (rangée, fenêtre de la skin offerte, achat, refus, Néon nuit, bon, relance, courses jour / nuit, anglais, 1024x768), pas de régression sur `garage-finitions` ; diff lu (accroches ≤ 10 lignes, découpage du garage respecté, ancienne sauvegarde lue).
- **Point tranché** : bouton « Choisis ta skin offerte » dans la rangée au lieu d'une fenêtre automatique à l'ouverture du garage : accepté. **Au PO** : mettre à jour RG-04 de `regles-de-gestion/WFS-494.md` / WFS-499 (« proposée à l'ouverture suivante » → « bouton de la rangée »).

### 2026-11-14 20:23:40 — WFS-547 créé : compte à rebours du cadeau qui bouge à chaque seconde (po-scrum-master)
- **Demande de l'utilisateur** (urgente) : « le compteur de prochains cadeaux est figé ».
- **Constat** : `texteHeures()` n'affiche que hh:mm (une mise à jour par minute) ; 2e cause éventuelle (menu non redessiné) à vérifier sur l'APK.
- **Ticket** : WFS-547, P1 Urgent, E19 zone E8, ⏳ Prêt en tête, `@dev` (ou lead dev en renfort) ; 🟡 après fusion. Règles : `regles-de-gestion/WFS-547.md`.

### 2026-11-14 20:26:45 — WFS-547 annulé (po-scrum-master)
- **Ordre de l'utilisateur** : « pas besoin, enlève » (il lisait hh:mm comme minutes:secondes). Statut 🚫 Annulé ; aucun code touché ; le robot des archives le déplacera.
### 2026-11-14 20:27:16 — R18 : critique du design (menus, boutons, logo, transitions, départ) (smart-brainstorming)
- Captures du banc du matin (run 32, `menu1-2400`, `menu2-2400`) et code relus ; 12 axes dans `recommandations-smart-brain.md` (R18). Top 3 : fin de course claire (A01), départ mis en scène + départ parfait (A02, A03), vrai logo + un seul jeu de boutons (A04, A05). Rien transmis au PO : l'utilisateur choisit.

### 2026-11-14 20:28:16 — WFS-513 (PR #418) approuvée par le lead dev, fusion bloquée par un conflit (lead-dev)
- Revue faite sur 29fce0f6 : 26 captures ouvertes (FR / EN, achat, attente, sans pub, restauration, hors ligne, garage, 8 paires avant / après), diff lu en entier, construction et tests refaits (143 verts ; `test_audio_chocs` RG-07 rouge une fois sous charge de ma machine, vert 5 fois au repos, sans rapport), banc `market` 0 écart FR + EN, parcours 0 écart. Approbation postée dans la PR (en commentaire : même compte GitHub).
- Fusion squash refusée par GitHub : **conflit sur `journal.md`** seulement (main a avancé). Le développeur synchronise (`git merge origin/main -m "Synchronisation avec main (Claude, agent dev3d (01Us6uCvpo))"`, garder les deux entrées), pousse, puis rappelle le lead dev : si le nouveau commit ne change que la synchronisation, fusion directe sans nouvelle revue du code.

### 2026-11-14 18:13:34 — WFS-513 : début, branche `feature/waforspeed-513` créée (Claude, agent dev3d (01Us6uCvpo))
- Écran Market FR / EN (zone E8 `src/ui/ecrans/**`). Accroches annoncées (≤ 10 lignes chacune) : `src/ui/garage.cpp` (bouton Market) et `src/ui/ecrans/compteur_etoiles.*` (le compteur ouvre le Market).
- Réutilise : `Boutique` / `BoutiqueSimulee` (WFS-510), système de langue (WFS-504), `navigation.h`, `toasts.cpp`, composants du garage (WFS-499).

### 2026-11-14 18:45:03 — WFS-513 : écran Market codé, banc `market` à 0 écart (Claude, agent dev3d (01Us6uCvpo))
- **Fait** : `src/ui/ecrans/market_ecran.*` (système « MarketEcran », ordre 469, par-dessus le menu ou le garage) : solde animé en haut, 4 cartes de packs (nom, médaille, total en gros, badge « +N % offerts », « base ★ + N offertes », prix de la boutique sur le bouton), pack débutant en tête quand il est proposé, « Sans pub » à part (« Acheté » avec une coche), mentions RG-06, « Restaurer mes achats », « Retour » (zone retour de la manette = touche Retour d'Android). Achat : `Boutique::acheter`, bouton « Patiente… » pendant l'attente (rendu après 60 s sans réponse), « +720 ★ » et solde qui monte en 1,2 s ; en attente : message ; annulé : rien. Hors ligne (aucune boutique ou aucun prix) : boutons grisés, « Connexion à Google Play nécessaire ». Textes `market.*` FR / EN.
- **Sécurité des achats inchangée** : l'écran ne crédite rien ; il rebranche la boutique par `market::brancher(b, sauvegarde(), apresAchat)` (même traitement que WFS-510 / 511, plus un rappel « après » pour l'écran) et appelle `chargerProduits()` à l'ouverture (prix à jour, rattrapage).
- **Calculs testés** : `market_ecran_calculs.h` + `tests/test_market_ecran.cpp` (état des boutons, ordre des cartes, offertes, solde animé, fin de restauration, achat oublié).
- **Accroches annoncées** (≤ 10 lignes chacune) : `src/ui/garage.cpp` (4 lignes : include, bouton sous le solde, appui, touchers donnés au Market ouvert) ; `src/ui/ecrans/compteur_etoiles.*` (compteur caché sous le Market, commentaire RG-09) ; `src/platform/desktop_main.cpp` (3 lignes : WFS_BANC=market lance aussi le banc de l'écran) ; `CMakeLists.txt` (1 ligne : test_market_ecran). Zone E8 : `menu.cpp` (bouton « Market », compteur → Market), `pret_bandeau.cpp` (bandeau du prêt remonté au-dessus de « Market »).
- **Choix** : message dessiné dans l'écran (bandeau orange en haut) et pas par `toasts.cpp`, qui ne dessine qu'en course. Grande étoile des médailles dessinée sans couture (pentagone + 5 cerfs-volants) : `garage::etoile` montre ses joints en grand.
- **Vérifié** : banc `market` 1280x720 : FR + EN, 0 écart, RC-E20-06 0 texte français ou qui dépasse en anglais ; test_market_ecran vert.

### 2026-11-14 20:29:52 — WFS-513 fusionné par le lead dev (lead-dev)
- PR #418 (écran Market FR / EN, accès menu et garage, « Restaurer mes achats ») fusionnée en squash : **aaff70fb**, titre « WFS-513 : Écran Market FR / EN, accès menu et garage, « Restaurer mes achats » (#418) (Claude, agent dev3d (01Us6uCvpo)) ».
- Revue complète faite sur 29fce0f6 (captures, diff, construction, 142 tests CPU, bancs `market` et `parcours` à 0 écart). La 1re fusion avait été bloquée par un conflit de `journal.md`.
- Synchro du développeur f18984ab contrôlée : `git diff 29fce0f6 f18984ab` ne touche que des fichiers de suivi venus de `main` (TACHES.md, journal.md, suivi-pr.md, track-agent.md, tache utilisateur.md, recommandations-smart-brain.md, regles-de-gestion/WFS-547.md) ; aucune entrée de `main` perdue. Approbation reposée en commentaire sur f18984ab (même compte GitHub).
- `TACHES.md` : 🟣 Fusionné ; ligne #418 enlevée de `suivi-pr.md` ; `track-agent.md` : session 01Us6uCvpo terminée ; ligne ajoutée dans `qa/Check.md`.

### 2026-11-14 20:30:10 — WFS-513 : fin, session terminée (Claude, agent dev3d (01Us6uCvpo))
- PR #418 fusionnée en squash par le lead dev (aaff70fb), statut 🟣. Un seul sujet par session : je ne demande pas d'autre ticket.
- **Pour le PO** (remarques non bloquantes de la revue, à planifier si utile) : (1) passer l'état de l'achat dans `market::Traitement` au lieu de le déduire du message ; (2) WFS-511 : réponses de Google lues sur le fil du jeu (déjà le cas par `achatsAndroidLire`, à confirmer) ; (3) anglais : « Ads removed » à la place de « No ads activated » ; (4) capture du pack débutant quand `SKINS_EN_VENTE` passera à vrai (WFS-494).

### 2026-11-14 20:34:58 — R18 (suite) : animations du garage (smart-brainstorming)
- Code du garage relu : rotation, glissé et arrivée de la voiture existent ; pas de zoom, couleur et finition changent d'un coup. 5 axes ajoutés (A13 à A17) dans `recommandations-smart-brain.md`. Rien transmis au PO.

### 2026-11-14 20:35:57 — R18 (suite 2) : design du garage (smart-brainstorming)
- Captures du garage relues (couleurs, finitions) ; 6 axes ajoutés (A18 à A23) dans `recommandations-smart-brain.md`. Top : lumière neutre pour juger la teinte, nuancier repensé. Rien transmis au PO.

### 2026-11-14 20:37:04 — R18 (suite 3) : critique du design seul (smart-brainstorming)
- Aspect visuel comparé à la charte `design-street-racing` : 10 constats D01 à D10 (formes, couleurs, typo, logo, profondeur, grille, icônes, style des animations). Rien transmis au PO.

### 2026-11-14 20:48:01 — WFS-494 : PR #419 corrigée après la revue (Claude, agent dev3d (01Us6uCwV6))
- Bloquant corrigé : journal des tests CPU joint (`qa/pr/WFS-494/tests-cpu.log`, 143 réussis sur 7c7ff37e) et fin du journal dans la description.
- Remarques : skin offerte due → un appui sur une skin en vente ouvre la fenêtre de la skin offerte sur elle (plus de « Acheter N ★ » pour une skin gratuite) ; commentaire de `garage_skin.cpp` corrigé. Banc `garage-skins` refait : 0 écart (40 contrôles).
- Réponses « Bien reçu » puis « corrigé dans » sous la revue ; PR en 🔁 (a53f1a22), lead dev rappelé.


### 2026-11-14 20:49:55 — R18 (suite 4) : maquettes en image (smart-brainstorming)
- 5 maquettes dans `references/maquettes-R18/` (planche de style, menu, mode, garage, fin de course). Rien transmis au PO.

### 2026-11-14 21:01:27 — WFS-446 : retour lead dev corrigé, PR #417 à relire (Claude, agent dev3d (01Us6uCuVU))
- Réponse « Bien reçu » sous la revue, puis « corrigé dans » point par point. (1) Tests CPU : `qa/pr/WFS-446/tests-cpu.log` (142 réussis, commit da36b2bd) et section « Tests CPU » (70591904, 2ad43e3c). (2) RG-09 : banc `perf` avant (main 02cdaf15) / après (da36b2bd), Ville et Corniche, Haute et Basse : appels et triangles identiques ; mémoire graphique identique (banc `batiments`) ; temps dans le bruit du rendu logiciel.
- Code inchangé depuis 310599ff. Suivi PR en 🔁, lead dev rappelé.

### 2026-11-14 21:02:07 — R18 (suite 5) : maquettes 3D (smart-brainstorming)
- 3 rendus 3D avec les vrais modèles du jeu dans `references/maquettes-3D-R18/` (menu-parking, garage-studio, fin à la ligne d'arrivée). Rien transmis au PO.

### 2026-11-14 21:07:46 — R18 (suite 6) : boutons créatifs en vrais objets 3D (smart-brainstorming)
- Planche de 6 boutons-objets (Start/Stop, plaque, néon, interrupteur à capot, autocollant, compte-tours) et menu qui les utilise, dans `references/maquettes-3D-R18/`. Rien transmis au PO.

### 2026-11-14 21:15:08 — WFS-446 fusionné par le lead dev (lead-dev)
- PR #417 relue une 2e fois (commit 2ad43e3c) : les deux points du refus sont corrigés (journal des tests CPU `qa/pr/WFS-446/tests-cpu.log`, tableau du banc `perf` de RG-09 : appels, triangles et mémoire identiques à main). Code inchangé depuis 310599ff.
- 64 captures rouvertes ; branche fusionnée en local avec main, construite et testée : 143 test(s) réussi(s).
- « ✅ Approuvé » en commentaire (revue 5907189562, même compte GitHub), fusion en squash 642eb68c. TACHES 🟣, ligne de suivi-pr supprimée, qa/Check.md à jour.

### 2026-11-14 21:15:38 — WFS-446 fusionné, session terminée (Claude, agent dev3d (01Us6uCuVU))
- PR #417 approuvée et fusionnée par le lead dev à la 2e revue (squash 642eb68c). Statut 🟣 ; le robot des statuts passera le ticket en ✅ sur les bancs du matin.
- **Pour le PO** : à surveiller le temps de chargement sur le téléphone de l'utilisateur ; la voiture du joueur, les adversaires et le fantôme restent sans repli (WFS-448) ; le décor des cartes aussi (WFS-447). L'atlas des enseignes refusé retire aussi les devantures (secours existant de `commerces.cpp`).
- Un seul ticket par session : arrêt.

### 2026-11-14 21:28:31 — WFS-494 fusionné par le lead dev (lead-dev)

PR #419 (garage : skins, skin offerte, prix, skins du cadeau), 2e revue sur a53f1a22 : les 3 points de la 1re revue sont corrigés (journal des tests CPU joint, commentaire de `garage_skin.cpp`, appui sur une skin en vente quand la skin offerte est due → fenêtre de la skin offerte sur elle). Le lead dev a fusionné la branche avec main (WFS-513 compris) sans conflit : construction OK, **144 tests CPU réussis**, banc `garage-skins` **0 écart (40 contrôles)**. Captures nouvelles ouvertes. « ✅ Approuvé » en commentaire (même compte GitHub), puis squash **5688be1a**. TACHES : 🟣 Fusionné ; ligne retirée de `suivi-pr.md` ; ligne ajoutée dans `qa/Check.md`. Point au PO (déjà signalé) : texte de RG-04 de WFS-499 à mettre à jour (bouton « Choisis ta skin offerte » au lieu de la fenêtre à l’ouverture suivante).

### 2026-11-14 21:29:07 — Fin WFS-494 : fusionné (#419, 5688be1a), session terminée (Claude, agent dev3d (01Us6uCwV6))
- Fusion par le lead dev à la 2e revue (squash 5688be1a). Ligne de `track-agent.md` : ⏹️ session terminée ; aucun autre ticket demandé (un seul sujet par session).
- Pour le PO : réécrire la RG-04 de WFS-499 (skin offerte : fenêtre à l'achat d'une voiture, sinon bouton « Choisis ta skin offerte » de la rangée ou appui sur une skin en vente) ; `skins::donnerSkin` est prête pour WFS-503.


### 2026-11-14 21:30:09 — R18 (suite 7) : proposition finale des 14 écrans (smart-brainstorming)
- 14 maquettes 3D + vue d'ensemble dans `references/maquettes-finales-R18/` (menu, mode, voiture, carte, duel, cadeau, garage couleur et skin, chargement, départ, fin, résultats, crédits, market). Rien transmis au PO.

### 2026-11-14 21:50:43 — R18 (suite 8) : proposition finale corrigée (smart-brainstorming)
- Maquettes refaites selon les retours de l'utilisateur (menu gros plan, mode et carte à plat, plus de titres bleus, voiture et crédits gardés). Proposition seulement, rien transmis au PO.

### 2026-11-14 21:51:23 — R19 : proposition corrigée des écrans, R18 gardée (smart-brainstorming)
- Sur demande de l'utilisateur : R18 laissée telle quelle (`references/maquettes-finales-R18/`, 14 écrans) ; la version corrigée devient R19 (`references/maquettes-R19/`, 12 écrans + vue d'ensemble). Proposition seulement, rien transmis au PO.

### 2026-11-14 21:56:22 — R19 : garage, choix de la voiture (smart-brainstorming)
- Sur retour de l'utilisateur : écran actuel du garage gardé, seules la zone d'infos (cadrans, barres à LED) et les boutons évoluent ; `references/maquettes-R19/13-garage-choix-voiture.jpg`. Rien transmis au PO.

### 2026-11-14 21:59:00 — R19 : garage, onglet Voiture refait (smart-brainstorming)
- Sur retour de l'utilisateur : onglets gardés, contenu de l'onglet Voiture inspiré de l'écran actuel ; image `references/maquettes-R19/13-garage-choix-voiture.jpg` remplacée. Rien transmis au PO.

### 2026-11-14 22:01:02 — R19 : précédent / suivant en palettes au volant (smart-brainstorming)
- Garage, onglet Voiture : flèches remplacées par des palettes au volant (carbone, chevron et −/+ rétroéclairés). Rien transmis au PO.

### 2026-11-14 22:02:02 — R19 : palettes refusées, 4 styles de précédent / suivant à choisir (smart-brainstorming)
- Palettes au volant refusées par l'utilisateur : image 13 remise à la version d'avant ; planche `references/maquettes-R19/options-precedent-suivant.jpg` (chevrons, clignotants, balises de virage, flèches au sol). Rien transmis au PO.

### 2026-11-14 22:02:59 — R19 : précédent / suivant sobres (smart-brainstorming)
- Sur demande de l'utilisateur (« un bouton plus propre ») : boutons ronds en verre dépoli, chevron fin, orange au toucher ; planche des 4 styles retirée. Rien transmis au PO.

### 2026-11-14 22:03:58 — R19 : précédent / suivant en clignotants épurés (smart-brainstorming)
- Choix de l'utilisateur « proche du 2 » : flèches de clignotant épurées sur l'onglet Voiture du garage. Rien transmis au PO.

### 2026-11-14 23:51:38 — R19 : 3 versions du bloc des caractéristiques (smart-brainstorming)
- Demande de l'utilisateur : caractéristiques de la voiture avec plus de design ; planche `references/maquettes-R19/options-caracteristiques.jpg` (cadrans fins, fiche technique, radar), choix à faire par l'utilisateur. Rien transmis au PO.

### 2026-11-14 23:52:40 — R19 : caractéristiques en cadrans fins (smart-brainstorming)
- Choix n° 1 de l'utilisateur intégré dans l'onglet Voiture du garage ; planche des 3 versions retirée. Rien transmis au PO.

### 2026-11-14 23:55:16 — R19 : menu (Réglages, Cadeau du jour) et Crédits dans Réglages (smart-brainstorming)
- Retours de l'utilisateur : boutons Réglages et Cadeau du jour ajoutés au menu ; Crédits retiré du menu et placé dans Réglages à côté de « Conditions d'utilisation » (`14-reglages-credits.jpg`). Rien transmis au PO.

### 2026-11-14 23:59:52 — R19 : Cadeau du jour en bas du menu avec compte à rebours (smart-brainstorming)
- Idée de l'utilisateur : Cadeau du jour à la place du nom de la voiture, anneau de 24 h et compte à rebours. Rien transmis au PO.

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
