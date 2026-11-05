# WFS-91 — [STABLE] Endurance : parcours automatique long sans plantage ni fuite, validation Vulkan à zéro erreur, retours d'arrière-plan
Épic : E7 · Priorité : P2 Haute · Dépend de : — (WFS-87 ✅, WFS-69 ✅)

## Besoin (qui, quoi, pourquoi)
L'utilisateur veut une **version stable** sur son téléphone. Toutes les fonctionnalités du jeu web sont portées, mais rien
ne vérifie qu'une **longue session** tient : enchaîner courses, changements de carte, garage, pause, nuit/jour, retour
d'arrière-plan. Un plantage ou une fuite de mémoire au bout de 10 minutes ne se voit sur aucune capture actuelle (chaque
scène du banc dure quelques secondes). Ce ticket ajoute un **banc d'endurance** dans la CI, active les **couches de
validation Vulkan** sur le banc, et **corrige** ce qu'il trouve (petites corrections) ou le remonte au PO (grosses).

## Règles de gestion
- RG-01 : nouveau groupe de banc `WFS_BANC=endurance` (dans `src/platform/desktop_main.cpp`, code dans un nouveau fichier
  `src/platform/endurance_banc.*`). Il enchaîne **au moins 12 cycles** : Menu → Terrain (carte et moment) → Course en
  pilote automatique (`Game::setAutopilot`) jusqu'à la fin (temps écoulé ou accident) → Fin → Rejouer ou Menu, en
  faisant tourner les **4 cartes** (ville, plage, campagne, circuit) et **jour / nuit** ; un cycle sur trois passe
  par le Garage (changer de voiture et de peinture) et par la Pause (reprendre). Temps de jeu simulé ≥ 20 min au total
  (pas de 1/60 s) ; rendu d'une image sur N (N au choix pour tenir dans le temps de la CI).
- RG-02 : **retour d'arrière-plan simulé** : au moins 4 fois (dont une en course et une au garage), le banc reproduit
  ce que fait Android sur `APP_CMD_TERM_WINDOW` / `APP_CMD_INIT_WINDOW` (`android_main.cpp`, `termGraphics` /
  `initGraphics`) : passage en Pause si en course, libération puis recréation de ce que l'appli libère sur téléphone
  (cible de rendu / swapchain, ou `Renderer::shutdown` + `init` + rechargement si c'est ce que fait Android), puis reprise.
  Le jeu doit repartir sans plantage, avec une image correcte (capture).
- RG-03 : **mémoire** : le banc relève la mémoire résidente (`VmRSS` de `/proc/self/status`) au début de chaque cycle et
  l'écrit au journal (`endurance : cycle N, carte X, moment Y, RSS Z Mo`). À partir du cycle 3, la croissance totale
  doit rester **≤ 10 %** du RSS du cycle 2 ; sinon le banc sort en échec (code ≠ 0) en nommant le cycle.
- RG-04 : **validation Vulkan** : le job installe les couches de validation (paquet `vulkan-validationlayers`) et lance
  le banc d'endurance avec `VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation` (variable d'environnement du chargeur :
  **aucune modification de `src/render/**` nécessaire**). Les messages d'erreur de validation sont comptés dans le
  journal ; objectif **0 erreur**. Les avertissements de performance ne comptent pas.
- RG-05 : **job CI** `endurance` ajouté à la matrice `banc-essai` de `.github/workflows/build.yml` (ou job séparé, en
  parallèle) : durée cible ≤ 15 min ; il publie `captures-endurance` avec : une capture par carte (fin de course),
  une capture juste après chaque retour d'arrière-plan (`endurance-retour-N.png`), et `endurance-journal.txt`
  (lignes RSS, nombre d'erreurs de validation, nombre de cycles). Il **échoue** si plantage, fuite (RG-03) ou erreur de
  validation non corrigée. Il ne doit pas bloquer la publication de l'APK (`publication` ne dépend pas de lui).
- RG-06 : **corrections** : tout défaut trouvé (plantage, fuite, erreur de validation, image noire après retour) est
  corrigé dans ce ticket s'il tient dans la zone E7 ou dans une **accroche ≤ 10 lignes annoncée au Journal** dans une
  épic libre. **Ne pas toucher** `src/render/**` (WFS-88 en cours, E4) ni `src/game/player.cpp`, `course*`, `camera_vue.*` (WFS-90 en cours) ni `src/game/duel*`, `reseau_*`, `binarypack*`,
  `tests/**`, `src/ui/ecrans/salon.*` (WFS-89 en cours) : un défaut dans ces zones est décrit au Journal avec la pile /
  le message de validation, et le PO crée le ticket de correction. Si une erreur de validation ne peut pas être corrigée
  ici, la mettre dans une **liste d'erreurs connues** (`tools/qa/validation_connues.txt`, message exact + ticket) qui ne
  fait pas échouer le job, et prévenir le PO.
- RG-07 : rien ne change dans le jeu livré : le banc et les couches de validation n'existent que dans `wfs_bench` (PC).
  L'APK n'est pas modifié, sauf correction de défaut (RG-06).

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- **Rien à porter** : le jeu web n'a ni banc d'endurance ni validation (WebGL). Seul point de référence : le pilote
  automatique `autopilot(dt)` de `js/game/update.js`, **déjà porté** (`Game::setAutopilot`, `System::autopilot`) : s'en servir,
  ne pas en écrire un autre.
- Natif à réutiliser : scènes existantes du banc (`benchChangementCarte` de WFS-74 pour le changement de carte avec
  écran de chargement, `ecrans::benchPause`, `ecrans::benchGarage`, `benchSauvegardeScene`), `Game::allerA(Ecran::…)`,
  chemin Android `termGraphics` / `initGraphics` de `src/platform/android_main.cpp`, modèle de job de `build.yml` (WFS-69).

## Critères d'acceptation (vérifiables sur les captures et le journal du job `endurance`)
- [ ] Job `endurance` vert sur `main`, durée ≤ 15 min, artefact `captures-endurance` publié.
- [ ] `endurance-journal.txt` : ≥ 12 cycles, les 4 cartes, jour et nuit, ≥ 4 retours d'arrière-plan, RSS du dernier
      cycle ≤ RSS du cycle 2 + 10 %, **0 erreur de validation** (hors liste d'erreurs connues, chacune avec son ticket).
- [ ] `endurance-retour-*.png` : image de jeu correcte après chaque retour (ni noire, ni figée sur l'écran précédent, HUD présent en course).
- [ ] Une capture de fin de course par carte, sans défaut visible.
- [ ] Les autres jobs de la CI inchangés et verts ; l'APK toujours publié.

## Ajout du PO (2026-11-04) — ANO-29 : Release `apercu` jamais incomplète
Rattachée à ce ticket (même zone `.github/**`, pas de micro-ticket) ; à faire dans la même PR.
- RG-A1 : job de publication de `build.yml` : `concurrency` avec **`cancel-in-progress: false`** (un run de `main` qui publie
  n'est jamais annulé par le suivant ; le suivant attend).
- RG-A2 : les anciens fichiers de la Release `apercu` ne sont supprimés **qu'après l'envoi complet** des nouveaux (ou :
  publication dans `apercu-tmp`, puis bascule en une seule étape) : à aucun moment la Release n'a moins de fichiers que la précédente.
- Critère : - [ ] deux fusions rapprochées sur `main` : la Release `apercu` garde tous ses fichiers (APK + captures) pendant
  toute la publication (nombre de fichiers noté au journal du job avant / après).

## Hors périmètre
- Tests sur le simulateur Android (décision de l'utilisateur du 2026-11-04 : plus de simulateur).
- Optimisations de performance (WFS-82 fait) ; refonte du rendu ; iOS (WFS-88).
- Corriger dans ce ticket un défaut situé dans une zone d'un ticket en cours (WFS-88, WFS-89) : remonté au PO.
