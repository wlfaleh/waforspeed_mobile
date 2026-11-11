Agent : Claude, agent dev (01UY1Vv4ss)

**Ticket** : WFS-421 — [TECHNIQUE] Halos au sol des lampadaires coupés en Basse (petit halo de la tête gardé). Règles : `regles-de-gestion/WFS-421.md`.

## Description

- **Demande de l'utilisateur** : la Ville de nuit est trop lente en Basse sur le S21 FE ; on enlève, en Basse seulement, la « nappe orange » (grand disque additif) que chaque lampadaire posait au sol en plus de sa vraie lumière (−14 ms mesurés avec l'interrupteur « sans halos additifs » de WFS-419).
- **Ce qui change** :
  - `src/render/nuit.cpp` : en qualité Basse, l'appel `r.haloSol(...)` des lampadaires n'est plus fait (RG-01) ; le halo de la tête de lampe (`haloTete`) est toujours dessiné (RG-02) ; la vraie lumière des lampadaires (projecteurs du shader, `update`) ne change pas (RG-03). Moyenne et Haute suivent exactement le même code qu'avant.
  - `src/render/nuit_qualite.h` (nouveau, 17 lignes) : la règle en fonctions pures (`haloSolLampadaire`, `haloTeteLampadaire`), lues une fois par image, pour pouvoir la tester.
  - `tests/test_nuit_qualite.cpp` (nouveau) + 1 ligne dans `CMakeLists.txt` : test CPU de RG-01 et RG-02.
  - Banc court `WFS_BANC=halos-sol` : `src/render/nuit_halo_sol_banc.cpp` (nouveau) + accroche de 2 lignes dans `src/platform/desktop_main.cpp` (annoncée au journal) : Ville et Corniche de nuit, en Basse, Moyenne et Haute, même scène.
  - À l'écran (Basse seulement) : plus de disque orange au pied des lampadaires ; la rue reste éclairée par la lumière des lampadaires et les phares.
- **Comment c'est vérifié** :
  - Tests CPU : `bash tools/qa/tests_cpu.sh build` → **104 tests réussis** (dont `test_nuit_qualite`).
  - Banc `halos-sol`, avant / après comparés pixel par pixel (`compare -metric AE`) : **Moyenne et Haute : 0 pixel différent** en Ville et en Corniche (RG-01 : identiques) ; Basse : la nappe disparaît.
  - Appels de dessin en Basse (journal du banc) : Ville **234 → 213** (−21), Corniche **253 → 233** (−20) ; Moyenne / Haute inchangés (258, 314, 659).
- **Limites et risques connus** :
  - **RG-04 non mesuré sur le téléphone** : je n'ai pas le S21 FE. Le temps du banc PC (rendu logiciel) ne veut rien dire pour l'Adreno 660. Mesure à faire par l'utilisateur avec l'APK de `main`, interrupteurs de WFS-419 : Ville et Corniche de nuit, Basse, 45 %, MSAA ×2, scène / médiane / img/s avant et après. Le gain attendu est **inférieur ou égal** aux −14 ms de l'interrupteur « sans halos additifs », qui coupait aussi les autres halos (feux, lumière au sol des voitures) ; ces autres halos restent ici (hors périmètre).
  - **RG-03** : en Corniche, la chaussée juste derrière la voiture est plus sombre en Basse (capture `halosol-plage-basse`) : c'est la vraie lumière seule, sans le disque ; elle reste éclairée près des lampadaires et par les phares. Aucune forme ajoutée pour compenser.
  - Ticket 🟡 À valider (utilisateur) après la fusion.

## Captures pour le lead dev (6 après, 6 avant)

Dossier : [`qa/pr/WFS-421`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421/qa/pr/WFS-421) — banc `halos-sol`, 1280 x 720, nuit, vue Poursuite. « Avant » = `origin/main` + le fichier du banc seul.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `halosol-ville-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421/qa/pr/WFS-421/avant-halosol-ville-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421/qa/pr/WFS-421/apres-halosol-ville-basse.jpg?raw=true) |
| `halosol-ville-moyenne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421/qa/pr/WFS-421/avant-halosol-ville-moyenne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421/qa/pr/WFS-421/apres-halosol-ville-moyenne.jpg?raw=true) |
| `halosol-ville-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421/qa/pr/WFS-421/avant-halosol-ville-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421/qa/pr/WFS-421/apres-halosol-ville-haute.jpg?raw=true) |
| `halosol-plage-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421/qa/pr/WFS-421/avant-halosol-plage-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421/qa/pr/WFS-421/apres-halosol-plage-basse.jpg?raw=true) |
| `halosol-plage-moyenne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421/qa/pr/WFS-421/avant-halosol-plage-moyenne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421/qa/pr/WFS-421/apres-halosol-plage-moyenne.jpg?raw=true) |
| `halosol-plage-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421/qa/pr/WFS-421/avant-halosol-plage-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-421/qa/pr/WFS-421/apres-halosol-plage-haute.jpg?raw=true) |
