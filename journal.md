# WaForSpeed Mobile — journal

Journal du projet (tickets WFS-XX de `TACHES.md`) : une entrée par étape (début, construction, PR, verdict QA, fin…) avec
fait / fichiers / vérification / limites. **Nouvelles entrées à la fin du fichier.** Toujours partir de la dernière version de
`main` (`git pull`), ajouter seulement ses entrées, puis envoyer tout de suite ; ne jamais écraser le fichier.
**Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
Exemple : `### 2026-11-03 19:43:10 — WFS-58 construit, essai simulateur lancé (Claude, session trafic)`.
> **Archive (règle de l'utilisateur, 2026-11-06)** : ce fichier ne garde que les **2 derniers jours**. Les entrées plus anciennes sont dans `journal/archive/AAAA-MM-JJ.md` (un fichier par jour) ; ne les relire qu'en cas de besoin précis. Le PO déplace chaque jour les entrées de plus de 2 jours.

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

### 2026-11-20 01:15:23 — WFS-568 : enseignes en vrais caissons, plus de cyan, PR à ouvrir (Claude, agent dev3d (01UY1Vv3p8))
- Fait : panneaux sur mâts de la Ville et de la Corniche refaits en caissons d'enseigne double face (massif, platine, tiges M24, mât Ø 219,1, selle, cadre alu chanfreiné, plexiglas imprimé), une impression par commerce, rétroéclairage 4 000 K la nuit seulement ; façades : lettres colorées par commerce, lettres découpées en relief (galerie laiton), caisson bleu marine du garage ; casino inchangé. Atlas de textes en couleurs (sRGB), `Palette::variante()`.
- Fichiers : `src/world/enseignes.cpp/.h`, `src/world/ville.cpp`, `src/world/cartes_banc.cpp` et `src/world/plage.cpp` (vues de banc).
- Vérification : compilation complète, 156 tests CPU réussis, bancs ville / corniche sans erreur, 17 paires de captures (`qa/pr/WFS-568/`), appels de dessin = main ±5.
- Limites : rendu vérifié sur PC (lavapipe) seulement ; banc perf complet non rejoué. Paquets `.wfcarte` inchangés : WFS-569 sans objet.

### 2026-11-20 01:28:07 — WFS-568 : réserves de l'agent 3D traitées (Claude, agent dev3d (01UY1Vv3p8))
- Avis agent 3D sur b5ba8d94 : VALIDÉ avec réserves (PR #437). Traité : caisson abaissé (bas 4,6 m, haut 7,8 m : totem 4 à 8 m, RG-03), mâts Ø 273,0 mm, lettres allumées 1,5 → 1,25 (liseré rose de « NUIT BLANCHE »), face jaune de « PNEUS 24/7 » 0,9 → 0,45 (plus de halo), capture « PNEUS 24/7 » ajoutée (`carte-plage-enseigne-2`, jour et nuit).
- Gardé pour plus tard (hors périmètre, mineur) : lettres de la galerie en volume plein, cadre du caisson éclairé la nuit, bâtiment du garage (WFS-369 et suivants).
- Captures refaites seulement pour les vues touchées (règle du 2026-11-15) ; appels de dessin = main ±6.
### 2026-11-20 01:39:39 — WFS-568 : PR #437 refusée par le lead dev (lead-dev)
Commit relu `53163d77` (code `bf55e33d`). 17 paires de captures ouvertes : plus de cyan, une couleur par commerce, vrais caissons sur mâts, nuit bornée, rien de dégradé ailleurs. Avis agent 3D VALIDÉ présent. Diff entier lu, sans défaut bloquant. Construction à jour et `tests_cpu.sh` relancés par le lead dev : 156 réussis. **Refus pour pièces manquantes seulement** : (1) pas de `qa/pr/WFS-568/tests-cpu.log` ni de section « Tests CPU » dans la PR (règle du 2026-11-11) ; (2) description périmée (mât Ø 219,1 → 273, tests sur l'ancien code) ; (3) nouvel avis `@3d` à demander sur le commit qui ajoute le journal. Aucune capture à refaire. Revue sur GitHub (en commentaire, même compte).

### 2026-11-20 02:04:46 — WFS-568 fusionné par le lead dev (lead-dev)
PR #437, 2e revue sur `ff06d60c` : les 3 points du refus sont corrigés (`tests-cpu.log` et section « Tests CPU », description à jour, avis agent 3D VALIDÉ sur ff06d60c) ; clé du cache des matériaux corrigée (6b6d9443, rendu inchangé). Tests CPU relancés par le lead dev sur ff06d60c : 156 réussis. « ✅ Approuvé » en commentaire (même compte GitHub), fusion en squash **71c7add3**. Statut **🟡 À valider (utilisateur)** ; ligne de `suivi-pr.md` enlevée ; `qa/Check.md` complété. Paquets `.wfcarte` inchangés : WFS-569 sans objet (au PO de l'annuler).

### 2026-11-20 02:05:17 — WFS-568 : fusionné, session terminée (Claude, agent dev3d (01UY1Vv3p8))
- PR #437 fusionnée en squash par le lead dev (71c7add3) après un refus (dossier : tests CPU, description) corrigé ; avis agent 3D VALIDÉ sur le dernier commit. Ticket 🟡 À valider (utilisateur). Réponse écrite dans `tache utilisateur.md`.
- WFS-569 sans objet (paquets `.wfcarte` inchangés) : au PO de l'annuler.
- Session terminée (un seul sujet par session) : aucun autre ticket demandé.

### 2026-11-20 02:48:15 — Statuts automatiques, CI main #1157 (robot statuts)
- Run rouge (push, commit 71c7add), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-555, WFS-564, WFS-496, WFS-503, WFS-408.

### 2026-11-20 10:56:28 — Statuts automatiques, bancs du matin #58 (robot statuts)
- Run rouge (schedule, commit 28bf5ed), aucun statut changé. Job(s) en échec : endurance et chauffe, tests CPU et licences, banc (realisme-2), banc (complements-c).
- Tickets 🟣 contenus dans ce run : WFS-555, WFS-564, WFS-496, WFS-503, WFS-408.

### 2026-11-20 11:22:50 — Archives automatiques (robot archives)
- Journal : 23 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-18).
- TACHES.md : 0 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md`.

### 2026-11-20 18:05:32 — KO de l'utilisateur sur WFS-568 : WFS-590 créé (po-scrum-master)
- Phrase de l'utilisateur (2026-11-20, captures `qa/pr/WFS-568/`) : « Zéro réalisme dans la vie j'ai jamais vu un truc pareil », transmise par Claude, agent dev3d (01UY1Vv3p8) (session terminée). Analyse vérifiée dans `src/world/enseignes.cpp` : caisson 8,40 × 3,20 m sur deux mâts (bas à 4,6 m), placé par `vie::panneauxRoute` au bord de la route : objet irréel.
- **WFS-568** : ❌ Refusé par l'utilisateur (code fusionné gardé, aucun revert sans ordre). **ANO-317** (bloquante) créée ; ANO-316 renvoie à WFS-590. **WFS-569** : ❌ Annulé (RG-00 : paquets `.wfcarte` inchangés, PR #437).
- **WFS-590** (P1, E15, `regles-de-gestion/WFS-590.md`) : caissons sur mâts retirés de l'affichage (collisions comprises, code désactivé et gardé ; enlèvement autorisé par l'utilisateur le 2026-11-18), enseignes bandeau et drapeau sur les façades des commerces, sucette / colonne Morris facultatives en Ville, 4 × 3 m seulement mural en retrait, rien sur la Corniche ; photos réelles de référence exigées ; avis `@3d` VALIDÉ avant fusion ; 🟡 après fusion. Exclu : session 01UY1Vv3p8. Pas de tâche de paquets (rien de cuit ; à créer si la PR dit le contraire). 1er dans l'ordre `@dev`, avant WFS-571.
- Réponse à l'utilisateur dans `tache utilisateur.md` ; `track-agent.md` à jour (ligne de la session 01UY1Vv3p8 enlevée).

### 2026-11-20 18:06:04 — WFS-590 : ordre « décor logique ou suppression » (po-scrum-master)
- Ordre de l'utilisateur (2026-11-20) : « C'est de la merde. Dit au PO soit de créer un décor logique soit de supprimer carrément ça ». Ajouté en **RG-00** de WFS-590 : suppression de l'affichage des caissons sur mâts d'abord (collisions comprises, code gardé désactivé), décor logique seulement à 99 % dans la journée ; sinon PR avec la suppression seule et US séparée pour le décor. Réponse dans `tache utilisateur.md`.
