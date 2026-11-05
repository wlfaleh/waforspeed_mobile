# Construction et tests

## Dans le conteneur Claude
- Compiler / vérifier le C++ : en-têtes Vulkan (`git clone --depth 1 https://github.com/KhronosGroup/Vulkan-Headers`) puis
  `g++ -std=c++20 -fsyntax-only -I<vulkan-headers>/include -I<dossier avec des .inc factices> src/...cpp`.
- Tests CPU : `g++ -std=c++20 -O2 tests/test_track.cpp src/game/track.cpp src/game/player.cpp -o /tmp/tt && /tmp/tt`
  (comparer avec le jeu web : script Node qui exécute `js/world/track.js` avec les mêmes paramètres).
- Lancer des binaires téléchargés (glslang, Godot…) est bloqué : laisser la CI compiler les shaders.

## GitHub Actions (`build.yml`)
- `banc-essai` : Ubuntu + `glslc` + Vulkan logiciel (lavapipe) → `test_track`, puis `wfs_bench captures assets/`
  découpé en 11 jobs parallèles par `WFS_BANC` (`partie1a`…`partie2c`, `texte`, `ecrans-2400`, `menu1-2400`, `menu2-2400`,
  `vitrines`, `nuit`) ; sans `WFS_BANC`, tout le banc. Nouveau groupe : dans `desktop_main.cpp` avant le contrôle
  « groupe inconnu » (erreur), et dans la matrice (WFS-106).
- `android` : JDK 17, Gradle 8.7, NDK 27.2, CMake 3.22 → `assembleRelease` (signé avec la clé de debug, installable).
- `publication-apk` (après `android`, ≈ 10 appels à l'API) et `publication-captures` (après le banc) :
  `tools/qa/publier_apercu.sh` (`PUBLIER=apk|captures`) sur la Release `apercu` (`main`) ou `apercu-XX` (branche).
  APK jamais absent (nom provisoire puis renommage), seules les captures modifiées partent (`empreintes-*.sha256`),
  200 appels de l'heure gardés pour les APK, jamais un commit plus récent écrasé. Lien fixe :
  `https://github.com/wlfaleh/waforspeed_mobile/releases/tag/apercu`.
- Lire les erreurs : outils GitHub (`actions_list`, `get_job_logs`). Les captures se téléchargent depuis la Release
  (`https://github.com/wlfaleh/waforspeed_mobile/releases/download/apercu/<nom>.png`) pour être regardées.

## Ce que la CI ne peut pas vérifier
Le rendu sur un vrai GPU mobile, la pré-rotation, le tactile et les performances : à faire tester par l'utilisateur
et le noter dans le Journal (`journal.md`).
