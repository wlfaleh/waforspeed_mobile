Agent : Claude, lead dev (01UY1Vv3au) (session_01UY1Vv3aubodvdU7e7M2as8), en renfort (sujet confié en urgence par l'utilisateur, « tu n'as que deux heures »). **Cette PR doit être relue et fusionnée par une autre instance du lead dev.**

## Demande de l'utilisateur
Une version PC Windows du jeu, livrée par un **installateur** (pas de Steam), avec les mêmes fonctions qu'Android et, plus tard, le duel PC ↔ Android (épic E24). Ce ticket (WFS-602) est la 1re étape : **fenêtre, boucle de jeu, commandes, installateur**.

## Ce qui change
- **`src/platform/windows_main.cpp`** (nouveau) : point d'entrée Win32 (`WinMain`). Il ouvre une fenêtre 1280 × 720 redimensionnable (F11 = plein écran), crée la surface Vulkan Win32 et branche **tels quels** le jeu, le renderer, les CGU au 1er lancement et l'écran de chargement, comme `android_main.cpp`. La sauvegarde, le cache des pipelines et les paquets vont dans `%APPDATA%\WaForSpeed` (gardé à la désinstallation). La langue suit celle de Windows (fr, sinon en). Si Vulkan manque, un message clair demande de mettre à jour le pilote.
- **Commandes (RG-02)** : le clavier passe par le module manette existant (`manette.*`, mêmes actions que la manette Android).
  - Flèches, ZQSD ou WASD : volant, gaz, frein, et déplacement dans les menus.
  - Entrée : valider. Maj ou N : nitro. C : changer de vue.
  - Échap : même effet que la touche Retour du téléphone (`retourSysteme`).
  - Souris : le clic joue le doigt. Manette : **XInput** (chargée à la demande, aucun pilote requis).
  - Fenêtre quittée (Alt+Tab) : touches relâchées et pause en course.
- **`src/render/vk.cpp`** (accroche de 12 lignes) : branche `_WIN32` (`VK_KHR_win32_surface`), sur le modèle des branches Android et iOS.
- **`src/game/reseau_relais.cpp`** (accroche de 10 lignes) : sous Windows, pas de prises réseau avant WFS-604. Le flux reste fermé et le duel affiche « Aucun réseau », comme `reseau_udp.cpp` le faisait déjà. **`src/render/bench_perf.cpp`** (accroche de 6 lignes) : `getrusage` seulement sous Linux. Ces 3 accroches ne changent rien sur Android ni sur le banc Linux (toutes derrière `#if defined(_WIN32)`).
- **`CMakeLists.txt`** : bloc `elseif(WIN32)` (cible `WaForSpeed.exe`, liaison statique, bibliothèque d'importation `vulkan-1.dll` fabriquée depuis les en-têtes). Aucune ligne Android ni iOS changée.
- **`tools/pc/`** : `mingw-w64.cmake` (chaîne MinGW), `compat_windows.h` (`setenv` / `unsetenv`), `vulkan-1.sh`, `waforspeed.rc` et `.ico` (icône du Play Store), `installateur.nsi` (NSIS) et **`construire_installateur.sh`** (une commande : `.exe` puis `WaForSpeed-Setup.exe`).
- **Installateur (RG-04)** :
  - installe dans `Program Files\WaForSpeed`, avec un raccourci sur le **bureau** et une entrée dans le **menu Démarrer** (dont « Désinstaller ») ;
  - s'inscrit dans **Paramètres → Applications** ; la désinstallation retire tout sauf la sauvegarde du joueur ;
  - contenu : le même que l'APK (`assets/`, `configgame.json`, textes des CGU), **sans les voitures retirées de WFS-404** (le script s'arrête si l'une d'elles est présente).
- **Pas de Steam, pas d'achats, pas de publicité** sur PC (RG-05).

## Comment c'est vérifié
- `WaForSpeed.exe` compilé par MinGW-w64 13 (308 fichiers, 0 erreur). DLL demandées : `KERNEL32`, `msvcrt`, `USER32` et `vulkan-1` (celle du pilote), donc rien à livrer à côté.
- `WaForSpeed-Setup.exe` construit par `makensis` (96 Mo). Le contenu est listé dans `qa/pr/WFS-602/installateur.md`.
- Les fichiers modifiés se recompilent sans erreur ni avertissement pour le banc Linux. Les captures avant / après du banc PC (`menu1`, 1280 × 720) montrent que le jeu n'a pas changé.

## Tests CPU
`bash tools/qa/tests_cpu.sh build` sur le commit `f745acee` (branche synchronisée avec `main`) ; journal complet : `qa/pr/WFS-602/tests-cpu.log`.
```
[OK] test_ville_sol (2 s)
[OK] test_virages_fleches (1 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 159 test(s) réussi(s) en 444 s
```

## Corrections après la 1re revue du lead dev
- Nitro au clavier : Maj / N relâchent `A` sur tous les écrans (plus d'appui Entrée perdu à l'écran de fin) — commit 99b16fd7.
- Manette XInput : état précédent propre à la manette, seuls ses changements sont envoyés (A, B, Y tenus au clavier gardés ; remis à zéro à la déconnexion) — commit 99b16fd7.
- Section « Tests CPU » et `tests-cpu.log` ajoutés.

## Limites et risques
- **Pas d'essai sur un vrai Windows dans ce conteneur** : Wine 9 plante ici dès son démarrage (« free(): invalid pointer », même sur `cmd /c echo`). Le comportement réel de la fenêtre, du clavier et de l'installateur est donc **à valider par l'utilisateur** sur son PC (ticket 🟡 après la fusion). L'installateur lui a été envoyé.
- Installateur **non signé** : Windows affiche « Windows a protégé votre ordinateur » (« Informations complémentaires » → « Exécuter quand même »).
- Pas de son (WFS-603), pas de duel (WFS-604), pas de téléchargement de paquets (WFS-605) : les cartes du code servent de secours.
- Interface pensée pour le tactile : les boutons à l'écran restent visibles tant qu'on joue à la souris. Au clavier ou à la manette, ils se masquent, comme avec la manette Android.
- Pas de frein à main (RG-02 parle d'Espace) : `DriveInput` n'en a pas aujourd'hui.

## Captures pour le lead dev (12 paires, banc `menu1` 1280 × 720)

Les **39 paires** produites par `captures_pr.sh` sont **identiques** avant (main 8ea80620) et après (comparaison pixel à pixel : 0 paire différente) ; 12 paires ciblées gardées (règle du 2026-11-15). Elles prouvent que le jeu Android / banc ne change pas ; aucune capture Windows possible ici (Wine indisponible, voir Limites). Contenu de l'installateur : `qa/pr/WFS-602/installateur.md`.

| Écran | Avant (main) | Après (branche) |
|---|---|---|
| menu-0 | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/avant-menu-0.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/apres-menu-0.jpg?raw=true) |
| menu-showroom | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/avant-menu-showroom.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/apres-menu-showroom.jpg?raw=true) |
| menu-depart-go | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/avant-menu-depart-go.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/apres-menu-depart-go.jpg?raw=true) |
| parcours-mode | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/avant-parcours-mode.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/apres-parcours-mode.jpg?raw=true) |
| parcours-terrain | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/avant-parcours-terrain.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/apres-parcours-terrain.jpg?raw=true) |
| parcours-course | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/avant-parcours-course.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/apres-parcours-course.jpg?raw=true) |
| pause-0 | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/avant-pause-0.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/apres-pause-0.jpg?raw=true) |
| pause-quitter | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/avant-pause-quitter.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/apres-pause-quitter.jpg?raw=true) |
| garage-0 | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/avant-garage-0.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/apres-garage-0.jpg?raw=true) |
| garage-peintures | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/avant-garage-peintures.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/apres-garage-peintures.jpg?raw=true) |
| fin-menu | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/avant-fin-menu.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/apres-fin-menu.jpg?raw=true) |
| fin-retour-menu | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/avant-fin-retour-menu.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-602/qa/pr/WFS-602/apres-fin-retour-menu.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv3aubodvdU7e7M2as8
