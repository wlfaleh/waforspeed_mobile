# Construction et tests

## Dans le conteneur Claude
- Compiler / vérifier le C++ : en-têtes Vulkan (`git clone --depth 1 https://github.com/KhronosGroup/Vulkan-Headers`) puis
  `g++ -std=c++20 -fsyntax-only -I<vulkan-headers>/include -I<dossier avec des .inc factices> src/...cpp`.
- Tests CPU : `g++ -std=c++20 -O2 tests/test_track.cpp src/game/track.cpp src/game/player.cpp -o /tmp/tt && /tmp/tt`
  (comparer avec le jeu web : script Node qui exécute `js/world/track.js` avec les mêmes paramètres).
- Lancer des binaires téléchargés (glslang, Godot…) est bloqué : laisser la CI compiler les shaders.

## GitHub Actions (`build.yml`)
- `banc-essai` : Ubuntu + `glslc` + Vulkan logiciel (lavapipe) → `test_track`, puis `wfs_bench captures assets/`
  (captures vitrine de chaque voiture + course en pilote automatique).
- `android` : JDK 17, Gradle 8.7, NDK 27.2, CMake 3.22 → `assembleRelease` (signé avec la clé de debug, installable).
- `publication` : Release `apercu` remplacée à chaque construction : `WaForSpeed.apk` + captures PNG.
  Lien fixe : `https://github.com/wlfaleh/waforspeed_mobile/releases/tag/apercu`.
- Lire les erreurs : outils GitHub (`actions_list`, `get_job_logs`). Les captures se téléchargent depuis la Release
  (`https://github.com/wlfaleh/waforspeed_mobile/releases/download/apercu/<nom>.png`) pour être regardées.

## Ce que la CI ne peut pas vérifier
Le rendu sur un vrai GPU mobile, la pré-rotation, le tactile et les performances : à faire tester par l'utilisateur
et le noter dans le journal de `TACHES.md`.
