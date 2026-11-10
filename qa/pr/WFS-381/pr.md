Agent : Claude, agent dev A (01UY1Vv42R)

**Ticket** : WFS-381 — règles : `regles-de-gestion/WFS-381.md` (ANO-221). Règles appliquées = **décisions du PO du 2026-11-10** (section « Suivi ») :
- **07:30:48** (commit 0522a566) : RG-02 « jour de brume de chaleur » d'été : Corniche V 8 km, Ville 10 km, Grand Prix 11 km, Campagne 12 km, autres cartes 10 km ; premier plan (< 100 m) ≤ 5 % ; contraste mesuré à l'écran à 1 et 2 km dans les bornes à ± 5 points ;
- **07:42:21** (commit 887d7da3) : seuil des 20 niveaux retiré ; écart à l'écran noté sans seuil ; Ville jugée à l'œil seulement (rien au-delà de 800 m) ; jugement à l'œil du lead dev.

## Description

- **Demande de l'utilisateur** (en une phrase) : mettre la vraie brume de l'air sur les lointains (perspective atmosphérique), pour que les collines et le fond ne ressemblent plus à une maquette, sans changer le premier plan ni la nuit.
- **Ce qui change** :
  - `shaders/eclairage.glsl` (E13) : brume exponentielle selon la **distance et la hauteur** (`brumeTau`, intégrale exacte d'une densité en exp(−y / H)), couleur = horizon du moment, **plus chaude et plus claire du côté du soleil** (lobe en cos⁸), inchangée la nuit et sous la pluie (`appliquerBrume`, `fogAt`). Sans hauteur (`fogLin.z` = 0 : garage, scènes dédiées), le calcul d'avant à l'identique.
  - `src/render/atmosphere_brume.h` (nouveau, E13) : réglage par carte (RG-04) et calcul de référence testé : Corniche β0 4,89·10⁻⁴ m⁻¹ (V 8 km, couche marine H 800 m), Ville 3,91·10⁻⁴ (10 km, H 1 000 m), Grand Prix 3,56·10⁻⁴ (11 km), Campagne 3,26·10⁻⁴ (12 km, H 1 200 m), autres cartes 10 km.
  - `src/render/eclairage_physique.*` (E13) : de jour, la brume prend ce réglage au lieu de l'extinction au sol du modèle de ciel (8,7·10⁻⁵ m⁻¹, V 45 km) ; forçage de banc `Forcage::brume` (sans effet hors banc) ; brume envoyée relevée dans `etat()`.
  - Accroches annoncées au journal (≤ 10 lignes chacune) : `shaders/scene.frag` et `scene_skin.frag` (5 lignes de brouillard → 1 appel), `renderer.h` (+1 ligne `fogHauteurInv`, `fogRef`), `renderer.cpp` (1 ligne `u.fogLin.zw`), `desktop_main.cpp` (+2 lignes, banc), `CMakeLists.txt` (+1 ligne, test).
  - Banc `WFS_BANC=brume` (`src/render/atmosphere_brume_banc.*`) avec **mesure du contraste à l'écran** et test `tests/test_brume.cpp`.
  - **Nuit** : la brume de nuit reste celle de `nuit.cpp` (brouillard bleu nuit sombre de la carte ; WFS-382 non touché) ; mon code n'agit que de jour : capture de nuit identique au pixel près (RMSE 0), RG-03 tenue.
- **Comment c'est vérifié** :
  - `test_brume` : bornes de la RG-02 de 07:30:48 pour les 4 cartes + une carte non citée (200 m, 1, 2, 5 km ; ≤ 5 % à 100 m ; visibilités), hauteur, intégrale exacte (contre 20 000 tranches), symétrie, continuité ; **97 tests CPU verts** (`tools/qa/tests_cpu.sh`).
  - **Contraste mesuré à l'écran (critère a)** : banc `brume`, cible HDR avant tonemapping, 5 rendus de la même image figée à exposition fixée (brume de la carte, brume de main, sans brume, brume totale, sonde homogène qui donne la distance de chaque pixel). Perte de contraste moyenne, main → branche :

    | Vue | 0,8-1,2 km | 1,6-2,4 km | Bornes RG-02 (± 5 points) | Écart à l'écran, lointains ≥ 1 km (critère b, sans seuil) |
    |---|---|---|---|---|
    | Corniche (capture 1) | 8,3 → **38,1 %** | 15,7 → **60,1 %** | 35-42 / 58-67 % | 8,4 niveaux |
    | Campagne (capture 2) | 8,9 → **29,0 %** | 16,1 → **47,4 %** | 25-31 / 44-52 % | 14,4 niveaux |
    | Corniche vue haute | 8,2 → **37,0 %** | 14,8 → **57,6 %** | 35-42 / 58-67 % | 13,3 niveaux |
    | Campagne vue haute | 8,2 → **26,6 %** | 15,4 → **45,2 %** | 25-31 / 44-52 % | 13,9 niveaux |
    | Ville (capture 3) | aucun pixel au-delà de 800 m : jugée à l'œil (décision 07:42:21) | | | |

    Les valeurs de main (8 % à 1 km) et de la branche suivent le calcul à ≤ 2,6 points près : la mesure est fiable.
  - `brume-mesures.txt` : un seul fichier (corrige le défaut 3 de la revue : plus de « avant » avec des valeurs que main n'utilise pas) ; il donne le réglage réel de chaque carte, le contraste mesuré main → branche et le coût.
  - **RG-05 coût** : passe de scène en Basse, scène figée, plusieurs mesures avant / après sur lavapipe : écart dans le bruit de la mesure (±3 %) ; calcul ajouté ≈ 25 opérations par pixel dont 2 exp (aucune texture, aucun triangle, rien créé en course).
- **Limites et risques connus** :
  - Les cartes ne montrent rien au-delà de ≈ 2 km (Ville : 800 m) : le vrai fond lointain (reliefs, côte à 5-20 km) viendra des US E15 (règle RG-E15-10 du PO).
  - L'avant est la brume de `main` reproduite dans la même construction (forçage 4 du banc : même calcul du shader que main) pour que seule la brume change entre les deux images ; écart avec la vraie capture de `origin/main` : RMSE 0,15 %. Nuit : capture de `origin/main`.
  - Avec la brume, `masquage::finBrouillard` coupe les objets de 3 à 10 m de rayon vers 8 km (Corniche) : sans effet aujourd'hui, noté pour E15 (remarque du lead dev, reprise par le PO).
  - Coût GPU mesuré sur lavapipe (processeur), pas sur téléphone.

## Captures pour le lead dev (7 après)

Dossier : [`qa/pr/WFS-381`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-381/qa/pr/WFS-381). Captures demandées : 1 = `brume-corniche-jour` (+ `-haut`), 2 = `brume-campagne-jour` (+ `-haut`), 3 = `brume-ville-jour` (+ `-haut`), 4 = `brume-corniche-nuit`, 5 = `brume-mesures.txt`.

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
