# Pipeline de WaForSpeed Mobile — détail (à revérifier sur `origin/main` avant de citer)

## build.yml « Construction WaForSpeed »
- **Déclencheurs** : `push` sur `main` (ignore `**/*.md`, `qa/**`, `regles-de-gestion/**`, `.claude/**`) ; `workflow_dispatch` (branche choisie) ;
  `schedule` `17 2 * * *` (nuit : **seul le job `endurance`** tourne).
- **Concurrence** (WFS-237) : `main` → un seul groupe `build-main` : un run commencé va toujours au bout (`cancel-in-progress: false`), un seul run
  en attente, remplacé par un envoi plus récent (qui contient toutes les fusions) ; la nuit a son groupe `build-main-nuit` ; branche → le nouvel
  envoi annule l'ancien. Les publications ont leur propre groupe (`apercu-apk-<ref>`, `apercu-captures-<ref>`) : un run plus récent peut annuler
  une publication en attente (le robot le compte vert, WFS-236).
- **Jobs** :
  - `compilation` (WFS-250) : le jeu est compilé **une seule fois** par run (apt-get en 2 essais, `cmake --build build`) ; archive tar `compilation`
    (`wfs_bench`, tous les `test_*`, `liste-tests.txt` ; shaders inclus dans le binaire), gardée 1 jour ; les 12 bancs et `endurance` en dépendent
    (`needs: compilation`), la décompressent et la vérifient (`tools/qa/verifier_compilation.sh` : binaire ou test manquant = rouge).
  - `banc-essai (<groupe>)` × 2 (WFS-277) : `texte` (+ tests CPU `tools/qa/tests_cpu.sh`, licences) et `menu1-2400` (parcours complet du jeu) ;
    captures par `tools/qa/banc_groupe.sh <groupe>` (même script que `matin.yml`) ; **plus de publication des captures** à chaque envoi.
  - `endurance` (35 min max) : couches de validation Vulkan, `tools/qa/endurance.sh` (cycles de course, mémoire, objets Vulkan ; verdict RG-13 / 14 / 17 de WFS-145),
    puis `tools/qa/chauffe.sh` (banc de chauffe WFS-123, limite `CHAUFFE_LIMITE`, 600 s par défaut, WFS-220) hors push.
  - `android` : NDK, CMake, SDK 35, APK + AAB, **signature fixe vérifiée** (WFS-118), pages de 16 Ko.
  - `publication-apk` : `tools/qa/publier_apercu.sh` (mode APK) → Release `apercu` (`main`) ou `apercu-<branche>` ; nom provisoire puis renommage (jamais de trou).
    Lien fixe de l'APK de main : https://github.com/wlfaleh/waforspeed_mobile/releases/download/apercu/WaForSpeed.apk
  - Notification (WFS-238) : dernière étape de `publication-apk`, **seulement sur main** et seulement si ce run a publié l'APK (titre de la Release) :
    `tools/qa/notifier_apk.sh` ajoute un commentaire `@wlfaleh` (construction, commit, tickets, lien de l'APK, lien du run) dans l'issue fixe
    « 📱 Nouvel APK de main » (créée et épinglée la première fois ; numéro à noter ici) ; un seul commentaire par construction (marque cachée) ;
    permissions du job : `contents: write`, `issues: write`, `actions: read` ; échec = `::warning::`, `continue-on-error`, 2 min au plus.
  - `résumé (commit et tickets)` : commit et tickets `WFS-XX` du run ; sur main (WFS-237), **tous les commits depuis le dernier run de main qui a
    tourné** (envoi, terminé, non annulé ; `gh api .../compare`), sinon « liste incomplète » et les commits du dernier envoi.
  - `banc-essai` (WFS-237) : installation `apt-get` en 2 essais de 5 min au plus (comme `endurance`), `timeout-minutes: 75`.
  - Run rouge qui regroupe plusieurs tickets : l'expert CI trouve le coupable (journaux d'erreur, puis au besoin CI d'une branche commit par commit).
## matin.yml « Bancs du matin (captures, réalisme, QA) » (WFS-252, WFS-277)
Chaque matin à **04:00 heure de Paris** (cron 02:00 et 03:00 UTC, le job `heure` garde celui qui tombe à 4 h ; QA à 07:00) + à la main, sur la dernière version de main.
`compilation` (wfs_bench) → `banc (<groupe>)` × 14 (`partie1a`, `partie1b`, `partie2a`, `partie2b`, `partie2c`, `texte`, `ecrans-2400`, `menu1-2400`,
`menu2-2400`, `vitrines`, `nuit`, `conduite-chocs`, `realisme-1`, `realisme-2` ; `tools/qa/banc_groupe.sh`) → `fusion` (rapports du réalisme, WFS-251)
→ `publication-captures` (toutes les captures dans la Release `apercu`, lue par la QA à 07:00, fin ≈ 05:15 Paris). Écart = rouge ; artefacts 30 jours.
Une seule publication des captures (ici) : `publier_apercu.sh` retire les captures absentes du run. Le robot des statuts ne suit que `build.yml`.
## statuts.yml — robot des statuts (WFS-196)
`workflow_run` de « Construction WaForSpeed » terminé, sur `main` ; `tools/qa/statuts.py` (test `test_statuts.py`) : run vert (hors nuit) → chaque
ligne 🟣 de `TACHES.md` dont le commit de fusion (PR de la ligne, sinon `(#NNN)`, sinon hash cité) est ancêtre du run passe ✅ « (CI main #N) » ;
rouge → entrée au journal (jobs en échec, tickets 🟣 du run) ; annulé → rien ; ligne mal formée → « à corriger par le PO ». Commit « (robot statuts) ».
## archives.yml — robot des archives (WFS-198)
03:07 UTC + à la main ; `tools/qa/archives.py` (test `test_archives.py`) ; comptages avant / après, sinon rien n'est envoyé ; ne touche pas `tache utilisateur.md`.
## Scripts utiles (`tools/qa/`)
`tests_cpu.sh` (tests CPU), `endurance.sh`, `chauffe.sh`, `emulateur.sh` (simulateur), `publier_apercu.sh`, `pages_16k.sh`, `verifier_licences.py`,
`luminance_route.py`, `validation_connues.txt` (erreurs de validation connues : **ne rien y ajouter pour faire passer**).
