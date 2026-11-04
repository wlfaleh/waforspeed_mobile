# WFS-74 — Changement de carte avec écran de chargement, course figée hors de l'écran Course
Épic : E0 (noyau, `Game`) · Priorité : P2 Haute · Dépend de : WFS-41 ✅, WFS-35 ✅

## Besoin (qui, quoi, pourquoi)
Le joueur choisit une carte et un moment sur l'écran Terrain (WFS-41) puis appuie sur « Rouler ». Aujourd'hui, `Game::update`
reconstruit toute la partie entre deux images (`Game::demandes()`, `shutdown()` puis `init()` sans progression) : l'écran se
fige plusieurs secondes sur téléphone, sans rien montrer. Et derrière l'écran Terrain (et Réglages), la course continue de
tourner : chrono, checkpoints et score bougent alors que le joueur n'est pas en course. Le joueur doit voir un vrai
chargement, puis arriver sur une course neuve, au départ, avec le décompte.

## Règles de gestion
- RG-01 (chargement visible) : « Rouler » avec une carte **ou** un moment différent de la partie en cours → l'écran de
  chargement de WFS-35 (`src/ui/ecrans/chargement.*` : fond `#0A0B10`, logo, barre cyan → orange, étape, « n % ») s'affiche
  **dès l'image suivant l'appui**, puis la barre avance avec les vraies étapes de `Game::init` (même fonction de progression
  `ecrans::Chargement::progression` qu'au démarrage, mêmes libellés, jamais en recul), puis fondu de 300 ms sur la nouvelle carte.
- RG-02 (même carte) : même carte et même moment → aucune reconstruction ; on va directement à la course neuve (RG-04).
- RG-03 (raccordement) : `Game` ne dépend pas de `src/ui/ecrans/` ; la plateforme (ou l'écran) lui donne la fonction de
  progression par un point d'extension dans `game.h` (ex. `Game::Demandes::progression`, `std::function<void(float,const char*)>`,
  ou paramètre déjà existant de `init`). Sans fonction branchée (tests), le comportement actuel reste.
- RG-04 (course neuve) : après le chargement (ou tout de suite si RG-02), on part d'une course neuve : voiture au départ,
  chrono, score, checkpoints et nitro remis à zéro, même graine de trafic, **décompte 3-2-1-GO** de WFS-12, comme « Rejouer » (WFS-38).
- RG-05 (course figée) : la course ne tourne que sur l'écran `Course`. Sur **tous** les autres écrans (Menu, Garage, Terrain,
  Réglages, Pause) : ni chrono, ni temps restant, ni checkpoint, ni score, ni accident, ni fin de course. Le trafic peut continuer
  à rouler en décor (comme derrière le menu aujourd'hui). Retour à `Course` depuis la Pause : la course reprend où elle était
  (pas de remise à zéro) ; depuis le Menu ou le Terrain : course neuve (RG-04).
- RG-06 (gardé) : voiture et peinture choisies au garage, réglages (caméra, qualité, son) inchangés après le changement de carte.
- RG-07 (commandes) : commandes tactiles lâchées pendant le chargement (`lacherCommandes`) : aucun gaz « collé » à l'arrivée.
- RG-08 (échec) : si la nouvelle carte ne se charge pas : message rouge de WFS-35 (« Le moteur 3D n'a pas pu démarrer sur cet
  appareil. »), aucun plantage.
- RG-09 (journal) : `terrain : carte = X, jour|nuit, chargée en N ms` ; `course : figée (écran Terrain)` une fois à l'entrée.
- RG-10 (zone) : `src/game/game.{h,cpp}` (E0). Accroches ≤ 10 lignes chacune, annoncées au Journal : `src/game/course_system.cpp`
  (E3 : `onEcran` et le test `g.ecran == Ecran::Menu` de `collide` → tout écran autre que `Course` ; **WFS-24 et WFS-70 sont en
  cours en E3** : synchroniser avec `main` avant de pousser, garder leurs lignes), `src/ui/ecrans/chargement.cpp` / `terrain.cpp`
  (E8, libre), `src/platform/android_main.cpp` et `desktop_main.cpp` (brancher la progression ; scène de banc). Pas de toucher à la
  partie « volant » de `Game::onTouch` / `drawControls` (déplacée par WFS-70).

## À réutiliser
- Natif (déjà là, ne rien refaire) : `ecrans::Chargement` (`debut`, `etape`, `fin`, `progression`, `opaciteFondu`), écran de
  WFS-35 ; `Game::init(..., progression)` déjà appelé ainsi par `android_main.cpp` ; « Rejouer » de `CourseSystem`
  (`restartReq_`, `trafficRestart`) pour la course neuve ; `Game::lacherCommandes`.
- Jeu web (`wlfaleh/waaps`, `waforspeed/`) : `js/main.js` `boot()` (écran `#loading`, progression par étapes) et
  `js/world/maps.js` (changement de carte) : même enchaînement chargement → course. La logique « course figée hors du jeu »
  vient de `js/game/update.js` (la boucle ne fait avancer la course que dans l'état de jeu). Aucun modèle 3D ni son.

## Critères d'acceptation (captures du banc PC)
- [ ] `terrain-chargement` : écran de chargement avec barre partielle et étape, après « Rouler » vers une autre carte (ex. Corniche)
- [ ] `terrain-arrivee` : la nouvelle carte, décompte « 3 » ou « GO », chrono et score à zéro, voiture de garage gardée
- [ ] `terrain-fige-1` / `terrain-fige-2` (écran Terrain, 2 s d'écart) : journal montrant le même chrono et le même score
- [ ] `pause` → reprise : chrono repris sans remise à zéro (journal) ; autres captures inchangées, tests CPU passés

## Hors périmètre
- Chargement en tâche de fond pendant qu'on joue ; écran de chargement différent de WFS-35 ; disposition gaucher (WFS-75).
