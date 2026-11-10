Agent : Claude, agent dev A (01UY1Vv42R)

**Ticket** : WFS-381 — règles : `regles-de-gestion/WFS-381.md` (ANO-221). RG-02 appliquée = **décision du PO du 2026-11-10, option A** (section « Suivi » : « RG-02 de la brume corrigée, option A », commit 8fda3f17 sur main) : brume d'été, Corniche ≈ 15 km, Ville ≈ 18 km, Grand Prix ≈ 20 km, Campagne ≈ 22 km.

## Description

- **Demande de l'utilisateur** (en une phrase) : mettre la vraie brume de l'air sur les lointains (perspective atmosphérique), pour que les collines et le fond ne ressemblent plus à une maquette, sans changer le premier plan ni la nuit.
- **Ce qui change** :
  - `shaders/eclairage.glsl` (E13) : brume exponentielle selon la **distance et la hauteur** (`brumeTau`, intégrale exacte d'une densité en exp(−y / H)), couleur = horizon du moment, **plus chaude et plus claire du côté du soleil** (lobe en cos⁸), inchangée la nuit et sous la pluie (`appliquerBrume`, `fogAt`). Sans hauteur (`fogLin.z` = 0 : garage, scènes dédiées), le calcul d'avant à l'identique.
  - `src/render/atmosphere_brume.h` (nouveau, E13) : réglage par carte (RG-04) et calcul de référence testé : Corniche β0 2,55·10⁻⁴ m⁻¹ (V 15,3 km, couche marine H 800 m), Ville 2,17·10⁻⁴ (18 km, H 1 000 m), Grand Prix 1,96·10⁻⁴ (20 km), Campagne 1,78·10⁻⁴ (22 km, H 1 200 m). Corniche à 2,55 et non 2,6·10⁻⁴ : 2,6 donnerait 5,07 % à 200 m, au-dessus de la borne de 5 %.
  - `src/render/eclairage_physique.cpp` (E13) : de jour, la brume prend ce réglage au lieu de l'extinction au sol du modèle de ciel (8,7·10⁻⁵ m⁻¹, V 45 km : presque rien à 1 km).
  - Accroches annoncées au journal (≤ 10 lignes chacune) : `shaders/scene.frag` et `scene_skin.frag` (5 lignes de brouillard → 1 appel), `renderer.h` (+1 ligne `fogHauteurInv`, `fogRef`), `renderer.cpp` (1 ligne `u.fogLin.zw`), `desktop_main.cpp` (+2 lignes, banc), `CMakeLists.txt` (+1 ligne, test).
  - Nouveau banc `WFS_BANC=brume` (`src/render/atmosphere_brume_banc.*`) et test `tests/test_brume.cpp`.
  - **Nuit** : la brume reste celle de la nuit (brouillard de `nuit.cpp`, bleu nuit sombre de la carte, WFS-382 non touché) ; mon code n'agit que de jour : capture de nuit identique au pixel près (RMSE 0), RG-03 tenue.
- **Comment c'est vérifié** :
  - `test_brume` : bornes de la nouvelle RG-02 (Corniche : 4,97 % à 200 m, 22,5 % à 1 km, 40,0 % à 2 km, 72,1 % à 5 km ; autres cartes ≤ 5 % / 25 % / 45 % / 75 %), visibilités ± 10 %, hauteur, intégrale exacte (contre 20 000 tranches), symétrie, continuité ; **96 tests CPU verts** (`tools/qa/tests_cpu.sh`).
  - Banc `brume` (lavapipe, 1280x720) : 7 captures avant / après + écart ×8 ; `brume-mesures-*.txt`.
  - **RG-05 coût** : passe de scène en Basse, scène figée : 4 mesures avant / 4 après sur lavapipe : écart dans le bruit de la mesure (±3 %), ex. dernière paire Corniche 182,8 → 188,8 ms, Campagne 233,5 → 228,7, Ville 307,2 → 292,1. Calcul ajouté ≈ 25 opérations par pixel dont 2 exp (aucune texture, aucun triangle, rien créé en course).
- **Limites et risques connus** :
  - Les cartes ne montrent rien au-delà de ≈ 2 km : l'effet se voit surtout dans les vues larges (plaine de la Campagne, vue haute) ; la vue de poursuite de la Corniche (décor à moins de 500 m) change peu. Les vrais lointains à 5-20 km relèvent des décors E15.
  - Le banc « brume » n'existe pas sur main : les captures « avant » sont faites sur `origin/main` + ce seul banc (aucun autre changement).
  - Coût GPU mesuré sur lavapipe (processeur), pas sur téléphone ; à confirmer au banc perf du téléphone.

## Captures pour le lead dev (7 après)

Dossier : [`qa/pr/WFS-381`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381). Captures demandées : 1 = `brume-corniche-jour`, 2 = `brume-campagne-jour` (+ `-haut`), 3 = `brume-ville-jour` (+ `-haut`), 4 = `brume-corniche-nuit`, 5 = `brume-mesures-avant.txt` / `brume-mesures-apres.txt`.

| Capture | Avant (main) | Après (branche) | Écart ×8 |
|---|---|---|---|
| `brume-campagne-haut-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/avant-brume-campagne-haut-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/apres-brume-campagne-haut-jour.jpg?raw=true) | ![écart ×8](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/ecart-brume-campagne-haut-jour.jpg?raw=true) |
| `brume-campagne-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/avant-brume-campagne-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/apres-brume-campagne-jour.jpg?raw=true) | ![écart ×8](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/ecart-brume-campagne-jour.jpg?raw=true) |
| `brume-corniche-haut-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/avant-brume-corniche-haut-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/apres-brume-corniche-haut-jour.jpg?raw=true) | ![écart ×8](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/ecart-brume-corniche-haut-jour.jpg?raw=true) |
| `brume-corniche-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/avant-brume-corniche-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/apres-brume-corniche-jour.jpg?raw=true) | ![écart ×8](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/ecart-brume-corniche-jour.jpg?raw=true) |
| `brume-corniche-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/avant-brume-corniche-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/apres-brume-corniche-nuit.jpg?raw=true) | ![écart ×8](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/ecart-brume-corniche-nuit.jpg?raw=true) |
| `brume-ville-haut-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/avant-brume-ville-haut-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/apres-brume-ville-haut-jour.jpg?raw=true) | ![écart ×8](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/ecart-brume-ville-haut-jour.jpg?raw=true) |
| `brume-ville-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/avant-brume-ville-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/apres-brume-ville-jour.jpg?raw=true) | ![écart ×8](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381/ecart-brume-ville-jour.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv42RWwaV9T1tMxWRgB
