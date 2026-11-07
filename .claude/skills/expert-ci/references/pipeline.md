# Pipeline de WaForSpeed Mobile — détail (à revérifier sur `origin/main` avant de citer)

## build.yml « Construction WaForSpeed »
- **Déclencheurs** : `push` sur `main` (ignore `**/*.md`, `qa/**`, `regles-de-gestion/**`, `.claude/**`) ; `workflow_dispatch` (branche choisie) ;
  `schedule` `17 2 * * *` (nuit : **seul le job `endurance`** tourne).
- **Concurrence** : `main` → un groupe par commit, rien n'est annulé ; branche → le nouvel envoi annule l'ancien. Les publications ont leur propre
  groupe (`apercu-apk-<ref>`, `apercu-captures-<ref>`) : un run plus récent peut annuler une publication en attente (normal).
- **Jobs** :
  - `banc-essai (<groupe>)` × 12 : `partie1a`, `partie1b`, `partie2a`, `partie2b`, `partie2c`, `texte` (+ test de la piste), `ecrans-2400`, `menu1-2400`,
    `menu2-2400`, `vitrines` (+ `trace`), `nuit`, `realisme` (+ textures KTX2 `tools/textures/ci_textures.sh`) ; Vulkan logiciel ; `WFS_BANC=<groupe> ./build/wfs_bench captures assets/` ;
    artefact `captures-<groupe>`.
  - `endurance` (35 min max) : couches de validation Vulkan, `tools/qa/endurance.sh` (cycles de course, mémoire, objets Vulkan ; verdict RG-13 / 14 / 17 de WFS-145),
    puis `tools/qa/chauffe.sh` (banc de chauffe WFS-123, limite `CHAUFFE_LIMITE`, 600 s par défaut, WFS-220) hors push.
  - `android` : NDK, CMake, SDK 35, APK + AAB, **signature fixe vérifiée** (WFS-118), pages de 16 Ko.
  - `publication-apk` : `tools/qa/publier_apercu.sh` (mode APK) → Release `apercu` (`main`) ou `apercu-<branche>` ; nom provisoire puis renommage (jamais de trou).
    Lien fixe de l'APK de main : https://github.com/wlfaleh/waforspeed_mobile/releases/download/apercu/WaForSpeed.apk
  - `publication-captures` : seulement les captures qui ont changé ; un échec = avertissement (job vert).
  - `résumé (commit et tickets)` : commit et tickets `WFS-XX` du run (pour le robot des statuts).
## statuts.yml — robot des statuts (WFS-196)
`workflow_run` de « Construction WaForSpeed » terminé, sur `main` ; `tools/qa/statuts.py` (test `test_statuts.py`) : run vert (hors nuit) → chaque
ligne 🟣 de `TACHES.md` dont le commit de fusion (PR de la ligne, sinon `(#NNN)`, sinon hash cité) est ancêtre du run passe ✅ « (CI main #N) » ;
rouge → entrée au journal (jobs en échec, tickets 🟣 du run) ; annulé → rien ; ligne mal formée → « à corriger par le PO ». Commit « (robot statuts) ».
## archives.yml — robot des archives (WFS-198)
03:07 UTC + à la main ; `tools/qa/archives.py` (test `test_archives.py`) ; comptages avant / après, sinon rien n'est envoyé ; ne touche pas `tache utilisateur.md`.
## Scripts utiles (`tools/qa/`)
`tests_cpu.sh` (tests CPU), `endurance.sh`, `chauffe.sh`, `emulateur.sh` (simulateur), `publier_apercu.sh`, `pages_16k.sh`, `verifier_licences.py`,
`luminance_route.py`, `validation_connues.txt` (erreurs de validation connues : **ne rien y ajouter pour faire passer**).
