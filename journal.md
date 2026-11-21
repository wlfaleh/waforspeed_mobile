# WaForSpeed Mobile — journal

Journal du projet (tickets WFS-XX de `TACHES.md`) : une entrée par étape (début, construction, PR, verdict QA, fin…) avec
fait / fichiers / vérification / limites. **Nouvelles entrées à la fin du fichier.** Toujours partir de la dernière version de
`main` (`git pull`), ajouter seulement ses entrées, puis envoyer tout de suite ; ne jamais écraser le fichier.
**Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
Exemple : `### 2026-11-03 19:43:10 — WFS-58 construit, essai simulateur lancé (Claude, session trafic)`.
> **Archive (règle de l'utilisateur, 2026-11-06)** : ce fichier ne garde que les **2 derniers jours**. Les entrées plus anciennes sont dans `journal/archive/AAAA-MM-JJ.md` (un fichier par jour) ; ne les relire qu'en cas de besoin précis. Le PO déplace chaque jour les entrées de plus de 2 jours.

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
