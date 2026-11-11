Agent : Claude, agent dev (01UY1Vv4BW)

**Ticket** : WFS-423 — [TECHNIQUE] Lumières bornées en Basse : boucle des projecteurs au vrai nombre, éclairage autour des voitures calculé une fois par voiture. Règles : `regles-de-gestion/WFS-423.md` (RG-01 à RG-05).

## Description

- **Demande de l'utilisateur** : la Ville de nuit est trop lente en Basse sur le S21 FE (0 lumière de nuit = −10 ms) ; on retire, en Basse, deux calculs inutiles des lumières sans changer l'image.
- **Ce qui change** :
  - **RG-02** — `src/render/eclairement_voiture.h` (nouveau, sans Vulkan) : éclairement de la chaussée autour d'une voiture (même formule et mêmes 3 points à 4 / 6,5 / 10 m que `eclairementRouteAutour` de `vehicule.glsl`, répartition du feu de croisement par `phares::intensiteRelative`, déjà la source des constantes du shader).
    `src/render/renderer.cpp` / `renderer.h` : en **Basse et de nuit**, `Renderer::draw(Model)` le calcule **une fois par matrice de voiture** (pièces peintes, cache par matrice) à partir des lumières exactement envoyées au shader (copie de `FrameUBO` dans `beginFrame`, interrupteurs du banc et de WFS-419 compris) ; la valeur part dans une nouvelle constante de poussée `PC::nuit` (128 octets : taille garantie par Vulkan, `static_assert`).
    `shaders/vehicule.glsl` : `pc.nuit.w > 0.5 ? pc.nuit.rgb : eclairementRouteAutour()` ; `scene.frag` et `scene_skin.frag` déclarent `vec4 nuit` dans leur bloc. Moyenne, Haute et toute pièce hors voiture : `w = 0`, même calcul qu'avant (RG-04).
  - **RG-01** — `shaders/eclairage.glsl` (`spotLights`, `spotLights2`) et `shaders/eau.glsl` : boucles bornées au vrai nombre de lumières `n` (et au plafond de 4 par pixel en Basse : `i < n && vus < parPixel`) au lieu de `for (i < 12) { if (i >= n) break; }`.
  - Banc court `WFS_BANC=lumieres-basse` : `src/render/lumieres_basse_banc.cpp` (copie du banc `halos-sol` de WFS-421) + **accroche de 2 lignes** dans `src/platform/desktop_main.cpp` (annoncée au journal).
  - `tests/test_eclairement_voiture.cpp` (nouveau, 12 contrôles) + 2 lignes dans `CMakeLists.txt`.
  - **À l'écran : rien ne change** (voir les mesures ci-dessous).
- **Comment c'est vérifié** :
  - Tests CPU : `bash tools/qa/tests_cpu.sh build` → **108 tests réussis** (dont `test_eclairement_voiture` : lampadaire calculé à la main, même valeur quels que soient la place et le cap de la voiture, feu de croisement sous / au-dessus de la coupure, lecture de `pc.nuit` par le shader, mêmes 3 points, boucles bornées).
  - Banc `lumieres-basse` (lavapipe, 1280 x 720, nuit, vue Poursuite), avant (`origin/main` + le fichier du banc seul) / après (branche synchronisée avec `main`) comparés pixel par pixel (`compare -metric AE`) : **0 pixel différent sur les 10 captures**, Basse comprise (Ville et Corniche, suite de 3 images à 0,2 s, Moyenne, Haute) : RG-03 (même lumière, aucun saut, voiture éclairée) et RG-04 (Moyenne / Haute identiques).
  - Preuve que la nouvelle voie est bien prise (essai local, **non commité**) : valeur du CPU × 20 → **80 214 pixels** changent sur `lumbasse-ville-basse-1` (caisse de la voiture) et 6 030 sur `lumbasse-plage-basse-1` ; **0** sur `lumbasse-ville-moyenne`. Donc en Basse, la caisse est bien éclairée par la valeur du CPU, et elle donne exactement la même image que le calcul par pixel.
- **Limites et risques connus** :
  - **RG-05 non mesuré sur le téléphone** : je n'ai pas le S21 FE ; le temps du banc PC (rendu logiciel) ne dit rien de l'Adreno 660. Mesure à faire par l'utilisateur avec l'APK de `main` (interrupteurs de WFS-419) : Ville et Corniche de nuit, Basse, 45 %, MSAA ×2, scène / médiane / img/s avant et après. Le gain viendra surtout de **RG-02** (plus de 3 × n lumières par pixel peint de voiture). Il est **au plus** des −10 ms de l'interrupteur « 0 lumière » (qui coupait aussi la lumière de la route et du décor, gardée ici).
  - **RG-01** : avec `n` uniforme, l'ancienne boucle sortait déjà au `break` : le gain dépend du compilateur de l'Adreno (déroulage en 12 copies) et peut être nul ; résultat identique dans tous les cas.
  - Le ticket demandait une capture « derrière une voiture du trafic » : le banc n'a pas de trafic devant le joueur à cet endroit ; c'est la voiture du joueur (même chemin `Renderer::draw(Model)` que le trafic, les adversaires et la police) qui est vue de dos sous les lampadaires.
  - Image inchangée : pas d'« Avis agent 3D » nécessaire selon `regles-de-gestion/WFS-423.md` (« si l'image change »).
  - Ticket 🟡 À valider (utilisateur) après la fusion.

## Captures pour le lead dev (10 après, 10 avant)

Dossier : [`qa/pr/WFS-423`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423) — banc `lumieres-basse`, 1280 x 720, nuit, temps sec, vue Poursuite. Les suites `-basse-1` à `-3` sont prises à 0,2 s d'écart en roulant vers le lampadaire suivant (16,8 → 7,5 m en Ville ; 20,6 → 10,3 m en Corniche, eau visible en haut à droite).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `lumbasse-ville-basse-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/avant-lumbasse-ville-basse-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/apres-lumbasse-ville-basse-1.jpg?raw=true) |
| `lumbasse-ville-basse-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/avant-lumbasse-ville-basse-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/apres-lumbasse-ville-basse-2.jpg?raw=true) |
| `lumbasse-ville-basse-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/avant-lumbasse-ville-basse-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/apres-lumbasse-ville-basse-3.jpg?raw=true) |
| `lumbasse-ville-moyenne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/avant-lumbasse-ville-moyenne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/apres-lumbasse-ville-moyenne.jpg?raw=true) |
| `lumbasse-ville-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/avant-lumbasse-ville-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/apres-lumbasse-ville-haute.jpg?raw=true) |
| `lumbasse-plage-basse-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/avant-lumbasse-plage-basse-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/apres-lumbasse-plage-basse-1.jpg?raw=true) |
| `lumbasse-plage-basse-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/avant-lumbasse-plage-basse-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/apres-lumbasse-plage-basse-2.jpg?raw=true) |
| `lumbasse-plage-basse-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/avant-lumbasse-plage-basse-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/apres-lumbasse-plage-basse-3.jpg?raw=true) |
| `lumbasse-plage-moyenne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/avant-lumbasse-plage-moyenne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/apres-lumbasse-plage-moyenne.jpg?raw=true) |
| `lumbasse-plage-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/avant-lumbasse-plage-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-423/qa/pr/WFS-423/apres-lumbasse-plage-haute.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv4BWx2FaXBYrKwi2KD
