## Agent : Claude, agent dev3d (01UY1Vv3k4)

**Ticket** : WFS-420 — règles : `regles-de-gestion/WFS-420.md`

## Description (à remplir par le développeur avant d'ouvrir la PR)

- **Demande de l'utilisateur** (en une phrase) : rendre la mesure de performance sur le téléphone reproductible : même trajet, mêmes conditions, chiffres écrits dans un fichier, pour comparer deux APK ou deux interrupteurs de WFS-419 sans conduire à la main.
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - Menu caché de WFS-419 (5 touchers sur « Réglages ») : nouveau bouton **« Banc auto »** → écran du **protocole** (RG-05 : batterie > 50 %, pas en charge, mode avion, luminosité fixe, téléphone froid ; relevés batterie / charge / température quand le téléphone les donne ; avertissement si des interrupteurs sont allumés dans `configgame.json`) → « Commencer » / « Annuler ».
  - Le banc roule seul (RG-01, RG-02) : Ville de nuit (pluie) puis de jour, caméra Arrière, graine de trafic fixe (`0x5eed1`), course relancée au départ à chaque essai, pilote automatique (`Game::piloteBanc`) ; par moment : tout éteint, chacun des 12 interrupteurs seul, tout éteint (contrôle de la chauffe) ; 30 s de chauffe + 60 s de mesure (≈ 42 min). Bandeau « Banc auto · essai n/28 » avec **« Arrêter »** ; quitter la course (pause, appel, arrière-plan) interrompt aussi, les essais finis sont gardés. Qualité automatique figée pendant le banc (essais à résolution égale).
  - Mesures (RG-03) : médiane, 95 %, 99 %, img/s ; GPU **brut** par passe (ombre, reflet, scène, post) et total, avec la règle « passes ≥ 90 % du total » écrite à l'écran sinon ; processeur (logique, préparation, envoi, attente du GPU) ; triangles, appels, matériaux distincts ; **mémoire graphique par type** (textures par format, maillages, cibles, autres : nouveau `src/render/memoire_gpu.{h,cpp}`, compté dans `vk.cpp`) ; statut thermique d'Android et température de la batterie.
  - Résultats (RG-04) : tableau nuit / jour à l'écran (écart à la référence en %), remarques (90 %, contrôle qui dérive = chauffe), fichier `diagnostic-AAAA-MM-JJ-hhmm.txt` dans le dossier de `rendu-mesure.txt`, bouton **« Partager »** (feuille de partage existante).
  - RG-06 : hors banc, rien n'est mesuré ni écrit (chronos de `Game` lus seulement pendant le banc, matériaux comptés seulement pendant la mesure ; cumul GPU = 5 additions par image).
  - Fichiers : `src/render/banc_telephone.{h,cpp}` (déroulé, statistiques, texte du fichier), `src/ui/ecrans/reglages_banc.{h,cpp}` (conduite du banc, écrans), `reglages.{h,cpp}` (bouton et branchement), `renderer.{h,cpp}`, `vk.{h,cpp}`, `qualite_systeme.cpp` ; accroches annoncées au journal : `game.{h,cpp}` (+8), `course_system.{h,cpp}` (+4 : `courseRejouer`), `desktop_main.cpp` (+2 : banc PC). `android_main.cpp` inchangé.
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) : nouveau test `test_banc_telephone` (déroulé de 28 essais, statistiques, règle des 90 %, écart à la référence, remarques, nom et texte du fichier) ; 110 tests CPU verts ; nouveau banc PC `WFS_BANC=banc-telephone` (2400x1080, durées raccourcies à 1,5 s + 4 s) : menu caché → protocole → 28 essais sur 28 (rechargement de la Ville nuit → jour compris) → tableau → fichier écrit → 2e banc arrêté par « Arrêter » (essais finis gardés), interrupteurs tous rétablis, pilote coupé : **0 écart**.
- **Limites et risques connus** : les chiffres des captures sont ceux du rendu logiciel du PC (une image ≈ 1,5 s), pas du téléphone ; la vraie durée (≈ 42 min), les horodatages GPU et les relevés batterie / température ne se vérifient que sur le téléphone (ticket 🟡 À valider) ; les cubemaps d'environnement ne sont pas comptées dans la mémoire par type (comme le total de WFS-297) ; « MSAA ×1 » et « Résolution 30 % » recréent les cibles en course (journal « pipelines créés pendant la course », attendu pour ces deux essais) ; sur le banc PC, le « 3 » du départ est encore visible parce que la chauffe est raccourcie à 1,5 s (30 s sur le téléphone).

## Captures pour le lead dev (6 après)

Dossier : [`qa/pr/WFS-420`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-420/qa/pr/WFS-420) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `banc-tel-1-menu-cache` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-420/qa/pr/WFS-420/apres-banc-tel-1-menu-cache.jpg?raw=true) |
| `banc-tel-2-protocole` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-420/qa/pr/WFS-420/apres-banc-tel-2-protocole.jpg?raw=true) |
| `banc-tel-3-nuit-en-cours` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-420/qa/pr/WFS-420/apres-banc-tel-3-nuit-en-cours.jpg?raw=true) |
| `banc-tel-4-jour-en-cours` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-420/qa/pr/WFS-420/apres-banc-tel-4-jour-en-cours.jpg?raw=true) |
| `banc-tel-5-resultats` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-420/qa/pr/WFS-420/apres-banc-tel-5-resultats.jpg?raw=true) |
| `banc-tel-6-arrete` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-420/qa/pr/WFS-420/apres-banc-tel-6-arrete.jpg?raw=true) |
