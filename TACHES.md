# WaForSpeed Mobile — tableau des tâches

** RÈGLE IMPORTANT : il faut voir systématiquement le fichier tache utilisateur.md et répondre en cas de besoin à l'utilisateur

Version native (C++20, Vulkan) du jeu WaForSpeed, pour Android puis iOS, portable sur consoles. Moteur maison, aucun moteur
de jeu tiers. Organisation type Jira / Scrum :
**épics** (une zone de code chacune), **tickets** WFS-XX priorisés, plusieurs agents en parallèle, en autonomie.

## Rôles
| Rôle | Agent | Ce qu'il fait |
|---|---|---|
| PO / Scrum Master | `po-scrum-master` (`.claude/agents/po-scrum-master.md`) | étudie le besoin, découpe en épics et tickets, priorise, **attribue les tickets**, débloque |
| Développeur | toute session Claude | réalise le ticket attribué, sur sa branche, dans la zone de son épic |
| QA lead | `qa-lead` (`.claude/agents/qa-lead.md`) | regarde les captures de chaque APK, classe les anomalies (bloquante / mineure), verdict VALIDÉ / À CORRIGER / PASSABLE AVEC ANOMALIES MINEURES / REFUSÉ |

## Statuts
| Statut | Signification |
|---|---|
| ⏳ En attente | dépendances pas encore terminées |
| ⏳ Prêt | dépendances terminées, critères clairs : peut être attribué |
| 🔧 En cours | attribué (Responsable), branche créée et poussée |
| 🔀 En PR | PR ouverte vers `main` (aucune CI de branche : règle de l'utilisateur du 2026-11-06) |
| 🟣 Fusionné | PR fusionnée dans `main` ; CI de `main` lancée, pas encore verte |
| ✅ Terminé | CI de `main` verte sur le commit de la fusion (compilation, tests, APK) et captures du ticket regardées ; la recette QA de nuit reste dans la colonne QA |
| ⛔ Bloqué | raison au Journal |

> **Règle de l'utilisateur (2026-11-06)** : la colonne **Statut** suit toujours ce cycle : 🔧 En cours → 🔀 En PR → 🟣 Fusionné → ✅ Terminé.
> Le développeur fusionne, **puis valide sur la CI de `main`** : il ne passe le ticket en ✅ qu'avec le numéro du run vert (ex. « ✅ Terminé (CI main #725) »).
> CI de `main` rouge à cause du ticket : il reste 🟣 et le développeur corrige tout de suite. Toujours **remplacer** le contenu de la case Statut
> (jamais l'écrire dans la case Responsable, qui garde le nom du développeur). Les lignes fusionnées avant cette règle gardent leur libellé.
| ⛔ Bloqué | raison au Journal |

## Règles de travail en parallèle (obligatoires)
1. **Attribution par le PO** : avant de créer une branche, un agent développeur **demande un ticket à l'agent
   `po-scrum-master`**. Le PO choisit le ticket `⏳ Prêt` le plus prioritaire **dont l'épic n'a aucun ticket en cours**,
   l'écrit dans ce fichier (🔧, Responsable, Branche, entrée « Début » au Journal) et l'envoie sur `main`. Personne ne
   se réserve un ticket seul.
2. **Un ticket en cours maximum par épic** et **un ticket maximum par agent** : chaque épic a sa zone de code, donc deux
   agents ne modifient jamais les mêmes fichiers.
3. **Zone de code** : un ticket ne modifie que les fichiers de la zone de son épic (tableau des épics). Exception unique :
   une « ligne d'accroche » (≤ 10 lignes, ex. enregistrer un système dans `Game`) annoncée au Journal.
   Tout autre changement du noyau passe par un ticket de l'épic E0.
4. **Branche** `feature/waforspeed-XX` par ticket (même si la session propose `claude/…`). Synchroniser avec `main` avant
   chaque validation et avant chaque PR.
   **Toujours synchroniser la branche avec `main` avant de lancer une CI** : avant chaque `git push` de la branche, chaque PR et chaque lancement de workflow (`build.yml`, `simulateur.yml`…), faire `git fetch origin && git merge origin/main`, résoudre les conflits en gardant les deux modifications, recompiler / relancer les tests locaux, puis seulement pousser. Une CI lancée sur une branche en retard sur `main` ne compte pas (captures et verdict non valables).
5. **QA** : après chaque construction, le développeur **lance l'agent `qa-lead`**, qui regarde les captures du
   **banc d'essai PC** et écrit `qa/rapports/WFS-XX.md`. Le développeur ne fait jamais la QA de son propre ticket.
   **Le QA décide de la gravité de chaque anomalie** :
   - au moins une anomalie **bloquante** → `REFUSÉ` : pas de fusion, le développeur corrige jusqu'à ce qu'il n'y en ait plus ;
   - anomalies **mineures** seulement → `À CORRIGER` : le QA donne la main au développeur **deux fois au plus** pour les corriger ;
     si elles restent après le 2e tour → `PASSABLE AVEC ANOMALIES MINEURES` : fusion autorisée ; le QA inscrit chaque anomalie
     restante au **tableau des anomalies** (`anno.md`) ; le **PO** les priorise et crée des **US de correction dédiées** ;
   - aucune anomalie → `VALIDÉ`.
   **Fusion directe par confiance (règle de l’utilisateur, 2026-11-04, prioritaire sur les règles de fusion ci-dessous)** : si le développeur est **confiant à plus de 50%** que son ticket fonctionne, il **fusionne directement**, sans attendre la fin de la CI ni les captures de sa branche. Conditions minimales, toujours : branche synchronisée avec `main` (`git fetch origin && git merge origin/main`), compilation et tests CPU passés en local, aucune modification hors de sa zone (sauf accroches ≤ 10 lignes annoncées au Journal). Au moment de la fusion : ligne obligatoire dans `qa/Check.md` (ticket, PR, date, « fusion par confiance », points à regarder), statut ✅ dans `TACHES.md`, entrée au Journal. Après la fusion, le développeur **suit la CI de `main`** : si elle est rouge ou si une capture est cassée par son ticket, il corrige **tout de suite**, en priorité sur tout le reste. Confiance ≤ 50% (gros changement du rendu ou du noyau, aucun moyen de tester en local, doute sur le résultat) : il attend la CI verte et regarde les captures avant de fusionner.
   **Fusion rapide (décision de l'utilisateur, 2026-11-03, MVP)** : sur ordre de l'utilisateur, le développeur peut fusionner sans
   attendre le verdict du QA dès que la compilation, les tests et l'APK sont verts et que ses captures ne montrent rien de bloquant.
   Le QA fait alors son passage **après la fusion**, sur `main` ; une anomalie bloquante trouvée ensuite devient un ticket de
   correction prioritaire, une mineure va dans `anno.md`. Noter chaque fusion rapide au Journal.
   **Fin d'épic** : le PO crée le ticket « Vérification simulateur EX » (émulateur Android,
   workflow `simulateur.yml`, rapport `qa/rapports/simulateur-EX.md`).
6. **Statuts à jour** après chaque avancement, envoyés sur `main` tout de suite, en modifiant seulement ses lignes à partir
   de la dernière version de `main` (ne jamais écraser le fichier).
7. **Fin de ticket** : ✅, Responsable libéré, entrée « Fin » au Journal ; le PO passe en `⏳ Prêt` les tickets débloqués.
8. **Ne jamais supprimer de branche** (gérées par l'utilisateur).
9. Une réservation sans nouvelle depuis 24 h peut être réattribuée par le PO (noté au Journal).
   **Branche poussée dans l'heure** (ordre de l'utilisateur, `tache utilisateur.md` ; précisé par le PO le 2026-11-05) :
   l'agent pousse `feature/waforspeed-XX` dans l'heure qui suit l'attribution (le commit de début suffit). Un ticket 🔧 sans
   branche poussée 2 h après l'attribution est remis en `⏳ Prêt` par le PO, avec une trace au Journal.

10. **Autonomie (règle de l’utilisateur)** : les agents **ne demandent pas de confirmation à l’utilisateur** pour suivre le processus. Ils enchaînent seuls : demander un ticket au PO, créer la branche, lire les règles, coder, construire, fusionner selon la règle en vigueur, mettre à jour `TACHES.md`, `journal.md`, `qa/Check.md`, puis demander le ticket suivant. Pas de « je lance ? », « je continue ? » en fin de message : on agit, puis on rend compte. L’utilisateur n’est sollicité que pour un **vrai blocage** : décision produit que ni les règles de gestion ni le PO ne tranchent, accès ou droits manquants, action irréversible hors processus (supprimer, réécrire l’historique…). Dans ce cas : une seule question précise, avec une recommandation. Une question de règles ou de priorité se pose au PO (`po-scrum-master`), pas à l’utilisateur.

**Définition de « Prêt »** : fichier `regles-de-gestion/WFS-XX.md` écrit par le PO (règles de gestion + éléments du dépôt à réutiliser), dépendances terminées, critères d'acceptation vérifiables sur captures, zone de code connue,
taille ≤ 1 journée (sinon le PO découpe).
**Règle de non-réattribution (utilisateur, 2026-11-06)** : un ticket « 🔧 En cours » ne change pas de développeur avant **10 heures sans aucun commit** de celui-ci sur sa branche (le PO vérifie la date du dernier commit et la note au Journal).

**Définition de « Fini »** (sauf fusion par confiance, voir ci-dessus : la CI et la recette viennent après la fusion) : critères cochés, CI verte (banc d'essai, APK, émulateur), verdict QA `VALIDÉ` ou `PASSABLE AVEC ANOMALIES MINEURES` (anomalies mineures restantes inscrites au tableau des anomalies), synchronisé, fusionné,
Journal à jour.

## Épics et zones de code
| Épic | Nom | Zone de code (fichiers modifiables) | But |
|---|---|---|---|
| E0 | Noyau partagé (catalyseur) | `src/core/**`, `src/game/game.*`, `src/render/renderer.h` (API), `CMakeLists.txt` | Stabiliser les points d'extension pour que les autres épics ajoutent des fichiers sans toucher au noyau. |
| E1 | Socle moteur (livraison 1) | — | Terminé. |
| E2 | Monde et décor | `src/world/**` (sauf fichiers E11 / E12 ci-dessous), `assets/world/**` (sauf `assets/world/modeles/**`), `shaders/world_*` | Ville, cartes, météo, piétons : tout ce qui entoure la route. |
| E3 | Gameplay et simulation | `src/game/` sauf `game.*` (trafic, course, duel, sauvegarde), `tests/**` | Règles du jeu, tests de fidélité. |
| E4 | Rendu et performances | `src/render/**` (sauf `ui_*` et fichiers E9 à E13 ci-dessous), `shaders/scene.*` (orchestration ; `sky*`, `post*`, `shadow*` passent à E13 le 2026-11-06) | Ombres, nuit, reflets, post-traitement, qualité, LOD. |
| E5 | Interface et contenu | `src/ui/**` (sauf `src/ui/ecrans/`), `shaders/ui*`, `assets/fonts/**` (`assets/cars/**` et `tools/convert_car.mjs` passent à E10 le 2026-11-06), `tools/convert_car.mjs` | Texte, menus, garage, HUD, voitures et peintures. |
| E6 | Audio | `src/audio/**`, `assets/audio/**` | Moteur, bruitages. |
| E7 | Plateformes et publication | `src/platform/**`, `android/**`, `ios/**`, `.github/**`, `tools/qa/**` | Android, iOS, manettes, stores, CI. |
| E8 | Écrans et menus | `src/ui/ecrans/**` | Chargement de l'application, menu principal, navigation, pause, choix du terrain. |
| E9 | Réalisme — Routes | `shaders/route*.glsl`, `src/render/route_*`, `assets/textures/route/**`, `references/reel/routes/**`, `tests/fidelite_routes*` (+ accroches ≤ 10 lignes `src/game/track.cpp`) | Revêtements, marquages, imperfections, états de surface au plus près du réel (axe Réalisme, `regles-de-gestion/DA-realisme.md`). |
| E10 | Réalisme — Véhicules | `shaders/vehicule*.glsl`, `src/render/vehicule_*`, `assets/cars/**`, `tools/convert_car.mjs`, `references/reel/voitures/**`, `tests/fidelite_voitures*` | Dimensions exactes, matériaux, dynamique visible des voitures. |
| E11 | Réalisme — Personnages | `assets/people/**`, `tools/convert_people*`, `src/world/pietons.*`, `src/world/foule_*`, `src/render/squelette.*`, `shaders/scene_skin*`, `references/reel/humains/**`, `tests/fidelite_humains*` | Humains réalistes, animations, comportements de foule. |
| E12 | Réalisme — Environnements | `src/world/archi_*`, `src/world/mobilier_*`, `src/world/vegetation.*`, `shaders/decor*.glsl`, `assets/textures/decor/**`, `assets/world/modeles/**`, `references/reel/decor/**`, `tests/fidelite_decor*` (+ accroches ≤ 10 lignes par carte) | Bâtiments, mobilier et signalisation, végétation, terrain, mer. |
| E13 | Réalisme — Lumière et image | `shaders/eclairage*.glsl`, `shaders/sky*`, `post*`, `shadow*`, `src/render/eclairage_*`, `ibl_*`, `atmosphere_*`, `post_*`, `references/reel/lumiere/**`, `tests/fidelite_lumiere*` | Unités physiques, IBL, ciel et soleil calculés, cycle jour / nuit, ombres, reflets, post-traitement. |

> Zones E9 à E13 créées le 2026-11-06 par le PO (axe Réalisme). Le ticket en cours WFS-144 (E4) garde ses droits sur ses fichiers jusqu'à sa fusion.

## MVP du 2026-11-03 (décision du PO, détail au Journal)
Parcours jouable sur téléphone : **chargement → menu → Course solo → décompte → course (trafic, chrono, checkpoints, HUD,
notifications) → fin → Rejouer / Menu**, en plein écran Android. **Décision de l'utilisateur (2026-11-03) : le MVP n'attend plus le simulateur ; l'utilisateur teste lui-même l'APK de `main` sur téléphone** (WFS-50, WFS-58 et WFS-64 sans urgence). Tickets marqués **[MVP]** (le tableau est trié par numéro de ticket ; la colonne « Priorité » donne l'urgence), dans l'ordre :
WFS-35 → WFS-27 → WFS-37 → WFS-29 → WFS-38 → WFS-47 → WFS-57 (centrage fin, ANO-14) → WFS-56 (scénario simulateur) → WFS-58 + WFS-59 (corrections simulateur) → WFS-50 (vérification simulateur ; WFS-64 suspendu : le gel ne s'est pas reproduit, run 41719456821). Hors MVP : garage, terrain, pause,
duel, sauvegarde, compteur en arc, italique, nuit, météo, bruitages.

## Axe stratégique « Réalisme » (directive de l'utilisateur, 2026-11-05 et 2026-11-06) — priorité HAUTE
Il n'y a plus de version Web : le jeu natif est la seule référence. Objectif : routes, voitures, personnages, environnements et lumière au plus près
du réel, **fidélité de 99 % mesurée** (part des mesures dans leur tolérance face à des données réelles sourcées), **jamais l'apparence au
détriment de la précision**. Charte, analyse de l'existant, limitations, tolérances et règles communes RC-01 à RC-10 :
**`regles-de-gestion/DA-realisme.md`** (à lire avant toute US de l'axe). Toutes les US de l'axe sont en **P2 Haute**.
**Étape par étape** : une US par partie, dans l'ordre des dépendances ci-dessous. Attribution, QA et fusion : règles de travail en vigueur (`CLAUDE.md`), inchangées.
| Palier | US dans l'ordre (→ = validée avant la suivante) | Épic | Profil d'agent |
|---|---|---|---|
| 0 — Référence et socle | WFS-148 → WFS-149 → WFS-150 | E0 | moteur Vulkan |
| 1 — Lumière | WFS-151 → WFS-152 → WFS-153 → WFS-154 | E13 | rendu / éclairage |
| 2 — Véhicules | WFS-158 (dès WFS-148) → WFS-159 (après WFS-151) → WFS-160 | E10 | technical artist véhicules |
| 2 — Routes | WFS-155 (après WFS-151) → WFS-156 → WFS-157 (après WFS-154) | E9 | rendu matériaux / route |
| 3 — Environnements | WFS-161 (après WFS-151) → WFS-162 → WFS-163 (après WFS-153) | E12 | technical artist décor |
| 4 — Personnages | WFS-164 (après WFS-151) → WFS-165 → WFS-166 | E11 | technical artist personnages, animation |
| 5 — Harmonisation | WFS-167 (après toutes) | E13 | rendu |

> 🔴 **WFS-222 (performance) attribué à agent arwa (Claude, session trafic) (10:42 UTC)**, premier développeur libre (ordre de l'utilisateur). WFS-221 (fluidité) attribué à Claude, agent 2 (10:27 UTC).

## Tableau (par numéro de ticket)

**Priorités (comme Jira)** : **P1 Urgent** (bloque le MVP) · **P2 Haute** · **P3 Moyenne** · **P4 Basse**. À priorité égale, on prend le plus petit numéro de ticket.
> **Format court (règle de l'utilisateur, 2026-11-06)** : une ligne par ticket **non terminé**, statut en quelques mots, titre court. Les critères, règles et notes de suivi sont dans `regles-de-gestion/WFS-XX.md` (jamais ici). Un ticket ✅ Terminé ou annulé part dans `archive/taches-terminees.md` (le PO le déplace). Colonnes inchangées (le robot des statuts `tools/qa/statuts.py` les lit).

| Priorité | Clé | Titre | Épic | Statut | Responsable | Dépend de | Branche | PR | QA |
|---|---|---|---|---|---|---|---|---|---|
| P4 Basse | WFS-50 | [MVP] Vérification simulateur du MVP (parcours complet sur Android) | E7 | 🔎 Fusionné | Claude (session trafic) | WFS-35 ✅, WFS-27 ✅, WFS-37 ✅, WFS-38 ✅ … | `feature/waforspeed-50` | #47 | ⛔ REFUSÉ (SIM-MVP-1 à 4) … |
| P4 Basse | WFS-64 | Diagnostic : gel intermittent juste après le chargement sur le simulateur | E0 | ⏸ Suspendu |  | WFS-59 ✅ | `feature/waforspeed-64` |  |  |
| P4 Basse | WFS-88 | iOS : `Renderer::init` ne se bloque plus, démarrage jusqu'au menu | E4 | ⏸ Suspendu | Claude (session trafic) | WFS-78 ✅ | `feature/waforspeed-88` | #76 |  |
| P2 Haute | WFS-92 | Duel par Internet : pair PeerJS réel (WebSocket + WebRTC, libdatachannel + Mbed TLS dans … | E3 | ⏸ Remplacé par WFS-112 |  | WFS-89 ✅ | `feature/waforspeed-92` |  |  |
| P2 Haute | WFS-152 | [RÉALISME 1.2] Lumière : ciel physique, soleil et lune calculés (NOAA), 6 moments + heure … | E13 (+ accroche E8 … | ⏳ En attente (découpée : lot 1 WFS-207 ✅, lot 2 WFS-208 🔧) |  | WFS-151 validée | `feature/waforspeed-152` |  |  |
| P2 Haute | WFS-153 | [RÉALISME 1.3] Lumière : ombres en cascades à pénombre physique, ombres des lumières la … | E13 | ⏳ En attente |  | WFS-151 validée (après WFS-152, même … | `feature/waforspeed-153` |  |  |
| P2 Haute | WFS-154 | [RÉALISME 1.4] Lumière : reflets d'écran (SSR), bloom par énergie, flou de mouvement par … | E13 | ⏳ En attente |  | WFS-153 validée | `feature/waforspeed-154` |  |  |
| P2 Haute | WFS-155 | [RÉALISME 2.R1] Routes : revêtements réels scannés et calibrés par carte, usure … | E9 (+ accroche `track.cpp`) | ⏳ En attente (découpée : lot 1 WFS-205 🔧, lot 2 WFS-206) |  | WFS-151 validée | `feature/waforspeed-155` |  |  |
| P2 Haute | WFS-156 | [RÉALISME 2.R2] Routes : marquages réglementaires usés et rétroréfléchissants, fissures … | E9 | ⏳ En attente |  | WFS-206 validée (lot 2 de WFS-155) | `feature/waforspeed-156` |  |  |
| P2 Haute | WFS-157 | [RÉALISME 2.R3] Routes : états de surface (sec, humide, mouillé, flaques, sable … | E9 (+ accroche `meteo.cpp`) | ⏳ En attente |  | WFS-156, WFS-154 validées | `feature/waforspeed-157` |  |  |
| P2 Haute (**RG-B / ANO-75 en P1 Urgent**, à livrer en premier, PO 2026-11-06 14:26) | WFS-158 | [RÉALISME 2.V1] Véhicules : dimensions exactes des 20 voitures (fiches constructeur ±1 … | E10 | 🔧 En cours | arwa-2 (Claude, session 01Us6uCu) | WFS-148 ✅ (fusionné, décision PO … | `feature/waforspeed-158` | #157 #162 #163 #166 #167 #186 #187 |  |
| P2 Haute | WFS-159 | [RÉALISME 2.V2] Véhicules : peinture multicouche (flocons, vernis), verre physique … | E10 | ⏳ En attente |  | WFS-158, WFS-151 validées | `feature/waforspeed-159` |  |  |
| P2 Haute | WFS-160 | [RÉALISME 2.V3] Véhicules : suspension visible (roulis, plongée), roues, salissures … | E10 (+ lecture E3) | ⏳ En attente |  | WFS-159 validée | `feature/waforspeed-160` |  |  |
| P2 Haute | WFS-161 | [RÉALISME 3.E1] Environnements : kit d'architecture (6 styles), fenêtres en profondeur … | E12 (+ accroches cartes) | 🔧 En cours (CI de branche run 822) | Claude, Agent 0 developer (session 01UY1Vv2SSLK), 2026-11-07 01:46 UTC | WFS-151 ✅ | `feature/waforspeed-161` |  |  |
| P2 Haute | WFS-162 | [RÉALISME 3.E2] Environnements : signalisation réglementaire, feux tricolores … | E12 | ⏳ En attente |  | WFS-161 validée | `feature/waforspeed-162` |  |  |
| P2 Haute | WFS-163 | [RÉALISME 3.E3] Environnements : végétation par espèce, herbe au vent, terrain mélangé … | E12 | ⏳ En attente |  | WFS-162, WFS-153 validées | `feature/waforspeed-163` |  |  |
| P2 Haute | WFS-164 | [RÉALISME 4.P1] Personnages : humains réalistes, 12 morphotypes, vêtements par lieu … | E11 | ⏳ En attente (découpée : WFS-209 ✅, WFS-211 🟣, WFS-210 🔧) |  | WFS-151 ✅ | `feature/waforspeed-164` |  |  |
| P2 Haute | WFS-165 | [RÉALISME 4.P2] Personnages : animations en capture de mouvement, mélanges, pieds au sol … | E11 | ⏳ En attente |  | WFS-164 validée | `feature/waforspeed-165` |  |  |
| P2 Haute | WFS-166 | [RÉALISME 4.P3] Personnages : comportements (trajets, feux, groupes, réactions, densité … | E11 (+ lecture E3) | ⏳ En attente |  | WFS-165 validée | `feature/waforspeed-166` |  |  |
| P2 Haute | WFS-167 | [RÉALISME 5] Harmonisation et validation finale par carte : rapport complet ≥ 99 % … | E13 | ⏳ En attente |  | WFS-148 à WFS-166 validées | `feature/waforspeed-167` |  |  |
| P2 Haute | WFS-192 | Voitures sans marque (2/4) : logos effacés ou remplacés sur les 20 modèles | E10 | ⏳ En attente |  | WFS-191, WFS-158 | `feature/waforspeed-192` |  |  |
| P3 Moyenne | WFS-193 | Voitures sans marque (3/4) : petites retouches de forme (calandre, feux, jantes) … | E10 | ⏳ En attente |  | WFS-192 | `feature/waforspeed-193` |  |  |
| P2 Haute | WFS-194 | Voitures sans marque (4/4) — lot 2 : brancher les 5 nouveaux modèles à la place des modèles non commerciaux | E10 | ⏳ En attente (après WFS-158, même épic E10 ; décision de l'utilisateur prise) |  | WFS-158, WFS-219 | `feature/waforspeed-194` |  |  |
| P2 Haute | WFS-195 | [RÉALISME] Vue cockpit réaliste : volant en cuir et cockpit digital propres à chaque … | E10 (+ E3 `camera_vue.*`) | ⏳ En attente (découpée : lot 1 WFS-197 🟣, lot 2 WFS-201 ✅, lot 3 WFS-202 🟣) |  | WFS-66 ✅, WFS-98 ✅ | `feature/waforspeed-195` |  |  |
| P2 Haute | WFS-206 | [RÉALISME 2.R1b] Routes (2/2) : usure, raccords, bordures et revêtement propre à chaque carte | E9 (+ accroche `track.cpp`) | 🟣 Fusionné (#188) | Claude, agent 2 (01UY1Vv5), 2026-11-07 08:20 UTC | WFS-205 ✅ | `feature/waforspeed-206` |  |  |
| P2 Haute | WFS-210 | [RÉALISME 4.P1b] Personnages (3/3) : vêtements par lieu, peau translucide, cheveux et yeux | E11 | 🟣 Fusionné (PR #185, be33c2f) | Claude, agent 5 (011cpqZaDiS2), 2026-11-07 08:40 UTC | WFS-209 ✅ | `feature/waforspeed-210` |  |  |
| P1 Urgent | WFS-213 | Correction : écran blanc après le chargement d'une voiture (exposition de WFS-151) | E13 | 🟣 Fusionné (PR #184, fusion par confiance) | Claude, agent 1 (01UY1Vv2), 2026-11-07 08:24 UTC | — | `feature/waforspeed-213` | #184 |  |
| P2 Haute | WFS-214 | Correction (lot E13) : ciel en paliers, chaussée bleue en Ultra (ANO-109, ANO-106) | E13 | ⏳ En attente |  | WFS-213 | `feature/waforspeed-214` |  |  |
| P3 Moyenne | WFS-215 | Correction (lot E10) : plaque arrière Model 3, vitres avant RR Sport (ANO-107, ANO-110) | E10 | ⏳ Prêt (dès que E10 est libre) |  | — | `feature/waforspeed-215` |  |  |
| P3 Moyenne | WFS-216 | Correction (lot E2) : col Campagne, foule et bateaux Plage, troupeaux de nuit (ANO-103, 105, 108) | E2 | ⏳ Prêt |  | WFS-212 ✅ | `feature/waforspeed-216` |  |  |
| P1 Urgent | WFS-217 | Conduite : corriger le « guide auto » (aide à la direction) dans les virages serrés et ronds-points + réglage Activée / Désactivée | E3 (+ accroche E8 `reglages.cpp`) | 🟣 Fusionné (PR #189, fusion par confiance, squash a63d209) | agent arwa (Claude, session trafic), 2026-11-07 09:48 UTC | — | `feature/waforspeed-217` | #189 |  |
| P1 Urgent | WFS-218 | Phares du joueur : **deux** vrais feux de croisement (gauche et droit, coupure), plus une seule lampe au centre | E4 | ⏳ Prêt |  | — | `feature/waforspeed-218` |  |  |
| P2 Haute | WFS-219 | Voitures non commerciales — lot 1 : trouver, vérifier, convertir les 5 nouveaux modèles (rien de branché) | E10 (exception PO : fichiers neufs `remplacants/`, à côté de WFS-158) | 🟣 Fusionné (PR #191, 4448d7c ; Mustang retenu sous dérogation à confirmer, 4 refus) | Claude, agent 5 (011cpqZaDiS2), 2026-11-07 10:53 UTC | — | `feature/waforspeed-219` |  |  |
| P1 Urgent | WFS-220 | [TECHNIQUE] Correction pipeline CI : banc de chauffe arrêté après 4 min (endurance de nuit, run #816) | E7 | 🟣 Fusionné (PR #190) | Claude, agent 1 (01UY1Vv2), 2026-11-07 10:03 UTC | — | `feature/waforspeed-220` | #190 |  |
| P1 Urgent | WFS-221 | [TECHNIQUE] [URGENT] Voiture saccadée en vue Proche / Poursuite : banc de fluidité, rendu interpolé, pente et virages lissés | E0 + E3 (exception PO : `game.cpp`, `track.*`, `camera_vue.*` ; WFS-217 garde `player.*`) | 🟣 Fusionné (#193) | Claude, agent 2 (01UY1Vv5), 2026-11-07 10:27 UTC | — | `feature/waforspeed-221` |  |  |
| P1 Urgent | WFS-222 | [TECHNIQUE] [URGENT] Performance sur téléphone : de 10-26 img/s à ≥ 30 stables (profilage, allègement, baisse auto efficace) | E4 | 🔧 En cours | agent arwa (Claude, session trafic), 2026-11-07 10:42 UTC | — | `feature/waforspeed-222` |  |  |
| P1 Urgent | WFS-223 | Ronds-points : corriger les défauts graphiques (marquages, îlot, blocs de couleur, raccords) sur les 4 cartes | E2 | 🟣 Fusionné (PR #192, squash 00be4ab) | Claude, session dev 5bx, 2026-11-07 10:52 UTC | — | `feature/waforspeed-223` | #192 |  |
| P1 Urgent | WFS-224 | Correction : boules grises (fumée des pneus) qui sortent derrière la voiture quand on tourne | E4 | ⏳ Prêt |  | — | `feature/waforspeed-224` |  |  |
| P1 Urgent | WFS-225 | Campagne de nuit : lampadaires allumés, nuit lisible (aujourd'hui tout noir) | E2 | 🔧 En cours | Claude, session dev 5bx, 2026-11-07 11:28 UTC | — | `feature/waforspeed-225` |  |  |
| P1 Urgent | WFS-226 | Nuit : phares allumés sur toutes les autres voitures (trafic, adversaires, police) | E4 + E3 | ⏳ Prêt |  | — | `feature/waforspeed-226` |  |  |

## Critères d'acceptation

Dans `regles-de-gestion/WFS-XX.md` de chaque ticket (section « Critères d'acceptation » et, pour l'historique, « Suivi repris de `TACHES.md` »).

## Anomalies et journal (fichiers à part)
- **Anomalies mineures** (QA → PO) : `anno.md`.
- **Journal** du projet (une entrée par étape, nouvelles entrées à la fin) : `journal.md`. **Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
Dans ce fichier, « au Journal » veut dire « dans `journal.md` ».
