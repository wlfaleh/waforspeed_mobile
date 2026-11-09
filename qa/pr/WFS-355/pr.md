## Agent : Claude, agent 2 (01UY1Vv5)

**Ticket** : WFS-355 — règles : `regles-de-gestion/WFS-355.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : après une installation neuve, l'appui sur ACCEPTER des CGU (case cochée) ne fait rien. L'utilisateur doit arrêter l'appli et la relancer pour entrer dans le jeu (APK 0.1.954, ANO-201, bloquant).
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - **Cause (RG-01, nommée au journal sur `main`)** : dans la boucle d'`android_main.cpp`, la lecture des événements sans attente dépendait de `cgu().premierLancement()`. Cette condition devient fausse dès « Accepter » (état `Accepte`). La boucle attendait alors sans fin le prochain événement (`ALooper_pollOnce(-1)`) et `Cgu::image`, qui note l'acceptation et lance le chargement, n'était plus appelée :
    - l'écran restait figé ;
    - l'acceptation n'était notée qu'en quittant l'appli (`perdreFenetre`).

    C'est exactement ce que décrit l'utilisateur. Même piège sur l'étape des paquets : « Plus tard » et « Continuer » mettent l'état `Fini`.
  - **Correction** (`android_main.cpp`, accroche d'environ 10 lignes) : la condition passe par `cgu_appui::boucleSansAttente(…, cgu().tientLaFenetre(), paquets tient ou Fini avant le jeu)`.
  - **Appuis rejoués** : les appuis lus par le veilleur de WFS-284 (image de plus d'1 s, émulateur surtout) sont rejoués aux CGU et aux paquets. Avant, ils étaient perdus. La destination est commune à `onInput` et au rejeu (`cgu_appui::destination`).
  - **`cgu.cpp`** :
    - cibles de 48 dp pour la case, Accepter, Refuser et Fermer (`appui::dansCible`, comme les Réglages) ;
    - au premier lancement, un appui dont la pose ou le lever s'est perdu compte quand même ;
    - « Accepter » s'éclaircit sous le doigt ;
    - chaque pose et chaque lever sont notés au journal, avec chaque image de plus de 0,5 s (diagnostic logcat).
  - **Nouveaux fichiers** : `src/ui/ecrans/cgu_appui.h` (calculs seuls) et `tests/test_cgu_appui.cpp`.
  - **À l'écran** : même écran, aucun bouton déplacé (RG-03), seule la teinte d'« Accepter » change sous le doigt.
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) :
  - `test_cgu_appui` (nouveau) couvre :
    - la boucle sans attente avec CGU acceptées, paquets finis ou jeu actif, et l'attente en pause ;
    - la destination des appuis ;
    - les bords de la cible de 48 dp ;
    - la pose ou le lever perdu.
  - Banc `cgu` en 2340×1080, installation neuve, **0 écart**. Nouveaux contrôles :
    - case avec pose perdue ;
    - doigt glissé hors d'Accepter : annulé ;
    - lever perdu sur Refuser puis Accepter au bord bas de la cible : accepté ;
    - boucle sans attente après Accepter, et ancienne condition bloquée (reproduction de la cause) ;
    - relance après acceptation : pas d'écran des CGU.
  - Contrôles de WFS-308 inchangés. La lecture depuis les Réglages ne se ferme pas toute seule ; ce cas a été trouvé et corrigé pendant le banc.
  - `tests_cpu.sh` : 88 tests OK.
  - Expressions modifiées d'`android_main.cpp` compilées à part avec les vrais en-têtes, sans NDK sur cette machine.
- **Limites et risques connus** :
  - Le banc PC ne passe pas par la boucle Android. La correction de la boucle est prouvée par le test et le raisonnement, pas par un téléphone. L'APK de `main` doit être essayé par l'utilisateur après la fusion (ticket 🟡).
  - L'émulateur (WFS-351) en profite aussi.
  - L'APK est compilé par la CI de `main` seulement (pas de NDK ici).
  - Pas de jour / nuit : écran d'interface.

Captures à regarder en priorité : `cgu-accepter-appuye` (teinte sous le doigt). Toutes les autres sont identiques à `main`, c'est voulu (RG-03).

## Captures pour le lead dev (9 après)

Dossier : [`qa/pr/WFS-355`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `cgu-accepter-appuye` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/apres-cgu-accepter-appuye.jpg?raw=true) |
| `cgu-coche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/avant-cgu-coche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/apres-cgu-coche.jpg?raw=true) |
| `cgu-defile` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/avant-cgu-defile.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/apres-cgu-defile.jpg?raw=true) |
| `cgu-premier` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/avant-cgu-premier.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/apres-cgu-premier.jpg?raw=true) |
| `cgu-refus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/avant-cgu-refus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/apres-cgu-refus.jpg?raw=true) |
| `cgu-reprise` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/avant-cgu-reprise.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/apres-cgu-reprise.jpg?raw=true) |
| `cgu-retour-jeu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/avant-cgu-retour-jeu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/apres-cgu-retour-jeu.jpg?raw=true) |
| `reglages-cgu-bouton` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/avant-reglages-cgu-bouton.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/apres-reglages-cgu-bouton.jpg?raw=true) |
| `reglages-cgu-lecture` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/avant-reglages-cgu-lecture.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-355/qa/pr/WFS-355/apres-reglages-cgu-lecture.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv5pG5V2148UwYk53fJ
