# WaForSpeed Mobile

Version native du jeu de course **WaForSpeed** : moteur maison en **C++20 + Vulkan**, sans moteur de jeu tiers,
pour Android (puis iOS et consoles).

- Tableau des tâches et livraisons : [TACHES.md](TACHES.md)
- Dernier APK à installer : page **Releases → apercu**

## Organisation
| Dossier | Contenu |
|---|---|
| `src/core` | journal, mémoire (arènes, réserves), système de tâches multi-thread, maths |
| `src/render` | contexte Vulkan, rendu HDR/PBR, chargement des voitures |
| `src/game` | piste, conduite, partie, commandes tactiles |
| `src/platform` | Android (NativeActivity) et banc d'essai bureau (captures hors écran) |
| `shaders` | GLSL compilés en SPIR-V à la construction |
| `assets` | voitures converties (`.wfm`), sons, licences |
| `tools` | convertisseurs d'assets (Node.js) |
| `android` | projet Gradle de l'APK |

## Construire
- Android : `cd android && gradle assembleRelease` (NDK 27, CMake 3.22).
- Banc d'essai Linux : `cmake -S . -B build -G Ninja && cmake --build build && ./build/wfs_bench captures assets/`
  (Vulkan logiciel possible avec `mesa-vulkan-drivers`).

## Crédits
Modèles 3D des voitures : voir `assets/licences/` (Sketchfab, licences CC BY / CC BY-SA). Bibliothèque `stb` (domaine public).
