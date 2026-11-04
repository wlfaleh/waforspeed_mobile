# WFS-78 — iOS : affichage direct dans une surface Metal et démarrage jusqu'au menu
Épic : E7 (+ accroche E4 dans `src/render/vk.cpp`) · Priorité : P4 Basse · Dépend de : WFS-22 ✅

## Besoin (qui, quoi, pourquoi)
WFS-22 compile le jeu pour iOS, mais l'image est rendue hors écran et recopiée en PNG dans un `UIImageView` deux fois par
seconde (`src/platform/ios_main.mm`), et le simulateur iOS n'écrit jamais « QA pret » en 150 s (journal arrêté après la
création du VkDevice). Ce ticket rend l'app iOS réellement jouable : image affichée à la cadence de l'écran, démarrage
jusqu'au menu. Les points 1 et 2 de la demande sont regroupés : même fichier, même cause probable (initialisation
synchrone dans `viewDidLoad` + chemin hors écran).

## Règles de gestion
- RG-01 : la vue du jeu est adossée à un `CAMetalLayer` (`+layerClass` d'une `UIView`) ; la surface Vulkan est créée par
  `VK_EXT_metal_surface` (`vkCreateMetalSurfaceEXT`, à défaut `VK_MVK_ios_surface`) dans `VkCtx::init`, en recevant le
  calque comme `window` (`void*`), sur le modèle exact du bloc Android de `vk.cpp` (extension d'instance + création de
  surface sous `#if defined(__APPLE__)`). Accroche ≤ 10 lignes annoncée au Journal ; rien d'autre ne change dans `vk.cpp`.
- RG-02 : taille du rendu = taille du calque en pixels (`bounds × contentScaleFactor`), plafonnée comme sur Android par la
  qualité automatique (WFS-15) ; rotation / changement de taille → `surfaceChanged()` existant. Tactile et marges de
  sécurité convertis dans ce repère (plus de 1280x720 fixe).
- RG-03 : la recopie PNG deux fois par seconde et le `UIImageView` sont supprimés ; les captures CI `ios-menu.png` et
  `ios-course.png` restent (via `capturePng`, comme aujourd'hui).
- RG-04 : diagnostic « QA pret » : horodater chaque étape de l'initialisation (journal `iOS : étape … n ms`). Le chargement
  du moteur ne bloque plus le fil principal plus de 0,5 s d'un coup : même écran de chargement que sur Android (WFS-35),
  étapes réparties sur plusieurs images. Si la cause est la lenteur du GPU logiciel du simulateur, le dire au Journal avec
  les durées mesurées et relever le délai d'attente de `ios.yml` (pas d'optimisation propre au simulateur).
- RG-05 : le workflow `ios.yml` reste à part et non bloquant pour Android (WFS-22 RG-08). Aucun ticket « vérification
  simulateur » séparé : le run `ios.yml` de ce ticket est la preuve.
- RG-06 (zone) : `src/platform/ios_main.mm`, `ios/**`, `.github/workflows/ios.yml` ; accroche ≤ 10 lignes dans
  `src/render/vk.cpp` (et `vk.h` si un champ est nécessaire). Ne pas toucher aux lignes audio de `ios/ios.cmake`
  (WFS-79).

## À réutiliser
- Natif : bloc Android de `VkCtx::init` (`vk.cpp`, extension `VK_KHR_android_surface`, `vkCreateAndroidSurfaceKHR`),
  boucle et écran de chargement d'`android_main.cpp` (étapes de WFS-35), `surfaceChanged()`.
- Jeu web (`wlfaleh/waaps`, `waforspeed/`) : **rien à réutiliser** (question propre à la plateforme Apple).

## Critères d'acceptation
- [ ] run `ios.yml` vert ; journal du simulateur : durées des étapes puis `QA pret` ; `ios-menu.png` montre le menu,
      `ios-course.png` la course (pas d'écran noir)
- [ ] journal : surface Metal créée, swapchain à la taille de la vue (plus de « hors écran ») ; plus de recopie PNG
- [ ] CI Android verte, captures du banc PC inchangées (`0 écart(s)`)

## Hors périmètre
TestFlight, signature réelle (action de l'utilisateur, `store/signature.md`), son iOS (WFS-79), manettes iOS, iPad.
