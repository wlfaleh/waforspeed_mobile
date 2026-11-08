Agent : Claude, agent dev Y

**Ticket** : WFS-285 (ANO-154). Règles : `regles-de-gestion/WFS-285.md`.

## Description

- **Demande** (campagne VABF, ANO-154) : quand Android retire puis rend la fenêtre (arrière-plan, écran verrouillé, appel), le jeu ne doit plus tout recharger. Plus d'écran noir de 80 s, et retour au même écran.
- **Ce qui change** (`src/platform/android_main.cpp`, zone E7)
  - **RG-01, fenêtre perdue** (`APP_CMD_TERM_WINDOW`) : `termGraphics(app, false)` ne détruit que la swapchain et la surface Vulkan.
    - Le device, le `Renderer` (pipelines, textures, maillages) et la partie (carte, voiture, course, menus) restent en mémoire.
    - `App::fenetre` passe à faux : la boucle ne dessine plus et attend sans consommer de batterie. Le redimensionnement est ignoré tant qu'il n'y a pas de surface.
  - **RG-02, fenêtre rendue** (`APP_CMD_INIT_WINDOW`, moteur prêt) :
    - nouvelle surface sur le même GPU, avec contrôle que la même file sait présenter dessus ;
    - nouvelle swapchain, puis `Renderer::resize()` (cibles d'écran), et reprise au même écran ;
    - le journal écrit `surface recréée en N ms` à la **première image dessinée**.
  - **RG-03** : en course, la Pause posée à `APP_CMD_PAUSE` (WFS-39) est gardée ; position et chrono ne bougent pas.
  - **RG-04, repli** : si la surface ou la swapchain ne se refait pas, le chemin d'avant (tout arrêter, tout recharger) reprend, avec la ligne `surface : repli, rechargement complet`. Il n'est pas retiré.
  - **RG-05** : à la fin de l'application, `termGraphics(app, true)` fait l'arrêt complet comme avant.
  - **Aucune accroche dans le contexte Vulkan ni le `Renderer`** : les membres de `VkCtx` sont publics, donc la surface est refaite dans `android_main.cpp`.
  - **`src/platform/endurance_banc.cpp`** : le banc suit le nouveau chemin.
    - Retours « en course » et « au menu » : cibles d'écran refaites (`Renderer::resize`), avec contrôle du même écran, et de la même position et du même chrono en Pause.
    - Retour « au garage » : repli complet, pour que RG-04 reste testé.
- **Comment c'est vérifié**
  - Banc d'endurance avec les couches de validation Khronos, par le script de la CI (`tools/qa/endurance.sh`) : **0 erreur de validation**, code 0, 3 retours d'arrière-plan.
    ```
    surface recréée en 17 ms
    endurance : retour d'arrière-plan 1 (en course), surface seule, écran pause gardé, RSS 1578.9 → 1578.8 Mo
    surface : repli, rechargement complet (banc : retour 2)
    surface recréée en 31 ms
    endurance : retour d'arrière-plan 3 (au menu), surface seule, écran menu gardé, RSS 1457.8 → 1453.9 Mo
    ```
  - 77 tests CPU verts après synchronisation avec `main` (avec WFS-301 dans le même fichier, sans conflit), parcours complet 0 écart.
  - **Compilation Android** : pas de NDK dans le conteneur. `android_main.cpp` est passé à `g++ -fsyntax-only -D__ANDROID__` avec l'en-tête Vulkan Android du système, `jni.h` du JDK et des en-têtes Android minimaux. Aucune erreur dans le fichier, et une erreur glissée exprès est bien détectée.
- **Limites et risques connus**
  - Le banc tourne en rendu hors écran : il refait les cibles d'écran, pas une vraie surface Android. La vraie preuve est sur téléphone ou sur l'émulateur des bancs du matin : une seule ligne « QA pret » par processus, et `surface recréée en N ms` avec N < 1000.
  - L'APK n'est compilé que par la CI de `main` après la fusion (pas de CI de branche).
  - `vkGetPhysicalDeviceSurfaceCapabilitiesKHR` dans `createSwapchain` passe par `VK_OK`, comme avant : une surface perdue pendant la recréation arrête l'appli au lieu de passer au repli. C'est un cas rare, non traité ici (code partagé).

## Captures pour le lead dev

Dossier : [`qa/pr/WFS-285`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-285/qa/pr/WFS-285)

| Capture | Ce qu'on voit | Avant (main) | Après (branche) |
|---|---|---|---|
| `endurance-avant-retour-1` | course en Ville, juste avant le passage en arrière-plan (voiture, chrono 63 s) : capture nouvelle, pas d'« avant » | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-285/qa/pr/WFS-285/apres-endurance-avant-retour-1.jpg?raw=true) |
| `endurance-retour-1` | **retour d'arrière-plan en course**. Avant : moteur rechargé, la course est perdue, retour au menu. Après : la Pause sur la même scène, même position et même chrono ; surface refaite en 17 ms | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-285/qa/pr/WFS-285/avant-endurance-retour-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-285/qa/pr/WFS-285/apres-endurance-retour-1.jpg?raw=true) |
| `endurance-retour-3` | **retour d'arrière-plan au menu**. Avant : rechargement complet puis menu. Après : le menu reste affiché sans chargement ; surface refaite en 31 ms. Le fond animé diffère seulement par l'instant | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-285/qa/pr/WFS-285/avant-endurance-retour-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-285/qa/pr/WFS-285/apres-endurance-retour-3.jpg?raw=true) |
| `endurance-retour-2` | **repli (RG-04)**, gardé au banc au garage : rechargement complet comme avant, puis menu ; les deux images montrent le même chemin | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-285/qa/pr/WFS-285/avant-endurance-retour-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-285/qa/pr/WFS-285/apres-endurance-retour-2.jpg?raw=true) |
