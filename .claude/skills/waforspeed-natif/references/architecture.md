# Architecture du moteur natif

## Boucle (Android)
`android_main` → `jobs().start()` → à chaque image : `Game::update(dt)` (pas fixes 1/120 s) puis `Game::render(t)`.
Fenêtre créée (`APP_CMD_INIT_WINDOW`) → `VkCtx::init` + `Renderer::init` + `Game::init`. Fenêtre détruite → tout est libéré.
Rotation / taille → `VkCtx::createSwapchain()` + `Renderer::resize()`.

## Rendu d'une image
1. `Renderer::beginFrame(FrameParams)` : attend la barrière de l'image, acquiert l'image de la swapchain, remplit le UBO (caméra, soleil,
   ciel, brouillard), démarre la passe de scène, dessine le ciel.
2. `Renderer::draw(...)` : objets opaques dessinés tout de suite ; transparents (vitres) mis en file.
3. `Renderer::uiRect/uiCircle` : interface en pixels logiques.
4. `Renderer::endFrame()` : transparents triés du plus loin au plus proche, fin de la passe de scène (résolution MSAA → `hdr_`),
   passe finale (tonemapping ACES + halo + vignette, puis UI), envoi et présentation.

## Données
- `Vertex` = position, normale, uv (32 octets). `Model` = un tampon de sommets + un d'indices + parties par matériau.
- `.wfm` : voir l'en-tête de `tools/convert_car.mjs`.
- `FrameUBO` (std140) et `PC` (constantes poussées, 112 octets) doivent rester identiques entre C++ et `shaders/common.glsl` / `scene.*`.

## Piste
`Track::build(map)` crée les segments (courbure, altitude) ; `pose(a)` calcule à la demande la position et le cap ;
`point(s, lat)` = point 3D. La route est dessinée par morceaux de `CH` segments, générés en parallèle (`Game::streamChunks`).

## Systèmes enfichables (WFS-30)
`src/core/system.h` : `System`, `GameView`, registre `systemRegistry()` rempli par `WFS_SYSTEM`.
`Game` : `systemsInit()` à la fin de `init`, `systemsUpdate()` à chaque pas fixe après la conduite, `systemsRender()`
après la route (avant la voiture du joueur), `systemsUi()` après les commandes, `systemsTouch()` avant les commandes,
`systemsShutdown()` au début de `shutdown`. Test : `tests/test_systems.cpp`.
