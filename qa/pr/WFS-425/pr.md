Agent : Claude, agent dev (01Us6uCtwf)

**Ticket** : WFS-425 — règles : `regles-de-gestion/WFS-425.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : avec l'interrupteur « MSAA ×1 » de WFS-419, la scène passe de 62 à 108 ms sur le S21 FE au lieu de baisser : trouver la cause et corriger.
- **Cause (RG-01)** — passe, attachements, chargement / stockage :
  - Le chemin ×1 a été suivi de bout en bout : `diag::MSAA_X1` → `Renderer::appliquerNettete` → `createTargets` → `createScenePipelines`. Pipelines, shaders, tri des objets et autres passes sont **les mêmes** en ×1 et en ×2 / ×4. Une seule chose change : les attachements de la passe de scène.
  - **×2 / ×4** : les shaders écrivent dans une couleur MSAA (`loadOp CLEAR`, `storeOp DONT_CARE`) et une profondeur (`CLEAR` / `DONT_CARE`), toutes deux **temporaires** (`TRANSIENT_ATTACHMENT`, mémoire « paresseuse ») : elles restent dans la mémoire de tuile. La cible HDR (`hdr_`, `R16G16B16A16`, `STORE`) n'est écrite qu'**une fois par pixel**, par la résolution de fin de passe.
  - **×1 (avant)** : 2 attachements seulement. Les shaders écrivaient **directement dans `hdr_`** (`CLEAR` / `STORE`), une vraie image en mémoire, échantillonnée ensuite par le bloom et la passe finale. Sur un GPU à tuiles, le pilote peut alors rendre cette passe **hors de la mémoire de tuile** (rendu « direct » de l'Adreno). Il perd alors deux choses : le tri des pixels cachés fait par la passe de tri des tuiles (chaque pixel caché passe dans le shader de nuit), et le fait que les écritures et les mélanges des transparents restent en tuile (ils vont en mémoire principale). Une passe MSAA, elle, reste en tuiles.
  - Preuve sur le banc PC (lavapipe, GPU **sans** tuiles), Ville de nuit, Basse, 60 %, Rue des Mimosas, code de `main` : scène **218 ms en ×4, 98 ms en ×1**. Le chemin ×1 n'a donc aucun défaut de calcul : la lenteur n'existe que sur le GPU à tuiles du téléphone. Le mode choisi par le pilote Qualcomm ne se voit pas depuis le PC. **La cause est donc établie par élimination ; la mesure sur le téléphone (RG-02) la confirmera ou l'infirmera.**
- **Ce qui change** :
  - `src/render/passe_scene.h` / `.cpp` (nouveaux) : la description et la création de la passe de scène sont réunies à un seul endroit, utilisé par `createTargets` et par le cache des variantes MSAA de WFS-262 (`prechaufferMsaa`). Avant, ce cache décrivait une passe à part.
  - **×1 seulement**, même schéma que le ×2, en 2 sous-passes. Sous-passe 0 : la scène, dans une couleur ×1 et une profondeur **temporaires** (`CLEAR` / `DONT_CARE`). Sous-passe 1 : copie de cette couleur (attachement d'entrée, dépendance `BY_REGION` : même tuile) dans `hdr_` (`DONT_CARE` / `STORE`). Les shaders n'écrivent plus jamais dans `hdr_`.
  - `shaders/scene_copie.frag` (nouveau) : `subpassLoad`, copie exacte (demi-flottant → flottant → demi-flottant).
  - `renderer.cpp` : `msaaColor_` (couleur temporaire) est créée aussi en ×1, framebuffer à 3 attachements pour tous les MSAA, sous-passe de copie enregistrée après l'étape Transparents (`vkCmdNextSubpass`), pipeline de copie refait avec la passe et détruit avec elle (changement de MSAA, arrêt).
  - Au passage, la passe finale n'est plus recréée (sans détruire l'ancienne) à chaque changement de MSAA : elle est créée une fois.
  - Accroches hors de `src/render/**` : une ligne dans la liste des shaders de `CMakeLists.txt`, une ligne `add_executable(test_passe_scene …)`.
  - **×2 et ×4 (RG-03)** : passe décrite au bit près comme avant (test). **MSAA par défaut de la Basse (RG-04)** : inchangé, ×2. **À l'écran** : rien ne change, l'image est identique au pixel près en ×1 comme en ×2 / ×4. Pas d'avis de l'agent 3D demandé, l'image ne changeant pas.
- **Comment c'est vérifié** :
  - `tests/test_passe_scene.cpp` (nouveau). En ×2 et ×4, attachements, sous-passe et dépendances sont identiques à une copie littérale de l'ancien `createTargets`. En ×1, la cible HDR n'est écrite que par la sous-passe de copie, la couleur et la profondeur ne sont jamais gardées, et les dépendances sont correctes (0 → 1 `BY_REGION`, image précédente → copie, copie → bloom).
  - `bash tools/qa/tests_cpu.sh build` : **104 tests réussis** (103 + le nouveau).
  - Couche de validation Vulkan (`VK_LAYER_KHRONOS_validation` + validation de la synchronisation), Ville de nuit, Basse, en ×4 puis en ×1 en course : **0 message**.
  - Captures du banc (ci-dessous) : **18 PNG avant / après identiques pixel par pixel** (Ville, Corniche et Campagne de nuit ; MSAA du niveau et ×1 ; Haute et Basse).
  - Temps sur le banc PC (lavapipe, 10 images, bruit ≈ ±5 %), scène en ×1, avant → après :

    | Lieu | Avant | Après |
    |---|---|---|
    | Ville, Basse | 190 ms | 185 ms |
    | Corniche, Basse | 133 ms | 138 ms |
    | Campagne, Basse | 132 ms | 127 ms |
    | Grand Prix, Basse | 147 ms | 157 ms |

    La copie ne coûte presque rien sur le PC. En ×4, avant → après : Ville Haute 783 → 787 ms, Ville Basse aux Mimosas 218 → 215 ms (inchangé).
  - **Ligne technique de WFS-419 (capture n° 3 du ticket)** : sur le banc en 1280 × 720, le texte de la ligne fait moins de 6 pixels de haut, illisible en capture ; voici son contenu, relevé dans le journal du banc (« GPU ms »), Ville de nuit, Basse, ×1 :
    - avant : `omb 2,1 refl 0,4 scène 190,4 sondes 0,4` ;
    - après : `omb 1,9 refl 0,2 scène 184,7 sondes 0,3`.

    Les vrais chiffres sont ceux du téléphone (RG-02).
- **Mesure à faire sur le téléphone (RG-02, après la fusion, APK de `main`)** :
  - Réglages de départ : Ville de nuit, puis Corniche de nuit ; Basse ; vérifier « 45 % » dans la ligne « Rendu » ; même endroit ; téléphone froid.
  - Dans le menu caché (5 touchers sur « RÉGLAGES »), relever « scène » (ligne technique) avec l'interrupteur « MSAA ×1 » éteint (×2), puis allumé (×1) ; vérifier que la ligne « Rendu » garde le même pourcentage.
  - But : **×1 plus rapide que ×2** (avant : 62 → 108 ms en Ville).
- **Limites et risques connus** :
  - Le choix entre tuiles et rendu direct appartient au pilote Qualcomm. Si, après cette PR, le ×1 reste plus lent que le ×2 sur le S21 FE, l'hypothèse est fausse : l'étape suivante serait une pré-passe de profondeur pour le ×1 (autre ticket, à décider par le PO).
  - En ×1, la copie coûte une lecture et une écriture par pixel, dans la tuile (presque rien sur le téléphone). Le ×1 sert aussi au mode « GPU logiciel » de l'émulateur de la QA (25 %) : une copie plein écran en plus.
  - Sur un GPU sans mémoire paresseuse (PC), la couleur temporaire ×1 occupe de la vraie mémoire (≈ 4 Mo à 960 × 540).
  - Constat hors ticket, signalé au PO dans le journal : la clé `diagnostic.msaa_x1` de `configgame.json` ne prend effet qu'au prochain changement de cibles (qualité ou menu caché), alors que la ligne technique affiche déjà « diag : MSAA1 ». C'est visible sur la 1re course des captures `x1-*` (Ville Haute en ×4). Le menu caché recrée les cibles tout de suite : la mesure de l'utilisateur n'est pas en cause.
- **Statut après la fusion** : 🟡 À valider (utilisateur).

## Captures pour le lead dev (7 × avant / après)

Dossier : [`qa/pr/WFS-425`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-425/qa/pr/WFS-425). Il a été produit par deux lancements de `tools/qa/captures_pr.sh WFS-425 perf` (réglages dans `README.md` du dossier). Le banc n'a pas de MSAA ×2 (lavapipe) : la netteté de la Basse y est en ×4.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `defaut-scene-depart-basse` — Ville de nuit, Basse, MSAA du niveau (banc : ×4, lavapipe n'a pas de ×2 ; téléphone : ×2), départ | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-425/qa/pr/WFS-425/avant-defaut-scene-depart-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-425/qa/pr/WFS-425/apres-defaut-scene-depart-basse.jpg?raw=true) |
| `defaut-scene-mimosas-basse` — Ville de nuit, Basse, MSAA du niveau, Rue des Mimosas (lieu de la mesure du téléphone) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-425/qa/pr/WFS-425/avant-defaut-scene-mimosas-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-425/qa/pr/WFS-425/apres-defaut-scene-mimosas-basse.jpg?raw=true) |
| `x1-perf-ville-Haute` — Ville de nuit, Haute, MSAA ×4 (1re course du banc : la clé MSAA ×1 du fichier n'est lue qu'au chargement) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-425/qa/pr/WFS-425/avant-x1-perf-ville-Haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-425/qa/pr/WFS-425/apres-x1-perf-ville-Haute.jpg?raw=true) |
| `x1-perf-ville-Basse` — Ville de nuit, Basse, interrupteur MSAA ×1 | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-425/qa/pr/WFS-425/avant-x1-perf-ville-Basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-425/qa/pr/WFS-425/apres-x1-perf-ville-Basse.jpg?raw=true) |
| `x1-perf-plage-Basse` — Corniche de nuit, Basse, interrupteur MSAA ×1 | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-425/qa/pr/WFS-425/avant-x1-perf-plage-Basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-425/qa/pr/WFS-425/apres-x1-perf-plage-Basse.jpg?raw=true) |
| `x1-perf-plage-Haute` — Corniche de nuit, Haute, interrupteur MSAA ×1 | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-425/qa/pr/WFS-425/avant-x1-perf-plage-Haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-425/qa/pr/WFS-425/apres-x1-perf-plage-Haute.jpg?raw=true) |
| `x1-perf-campagne-Basse` — Campagne de nuit, Basse, interrupteur MSAA ×1 | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-425/qa/pr/WFS-425/avant-x1-perf-campagne-Basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-425/qa/pr/WFS-425/apres-x1-perf-campagne-Basse.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01Us6uCtwfvRJMyFzdJaXYeM
