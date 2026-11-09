Agent : Claude, agent dev W (01UY1Vv4Ed)

**Ticket** : WFS-214 (P2, E13) — règles : `regles-de-gestion/WFS-214.md` ; anomalies ANO-109 et ANO-106.

## Demande
Ciel en paliers autour du soleil et ciel de midi gris-bleu sombre (ANO-109) ; chaussée bleue en Ultra (ANO-106) ; la nuit, la Golf ne doit plus virer au gris bleuté sous un lampadaire (RG-05).

## Ce qui change
- **RG-01, tramage** (`shaders/post.frag`) : le bruit de ±0,5 / 255 était ajouté en lumière **linéaire**, alors que l'écran encode en sRGB (téléphone et banc). Dans un ciel clair, un pas de code vaut alors ≈ 1,6 / 255 en linéaire : le bruit ne cachait plus les marches, d'où les anneaux. Maintenant, un bruit **triangulaire de ±1 code** est ajouté **au code sRGB**, puis ramené en linéaire pour l'écran. Test : marches du dégradé du ciel **0,23 → 0,008 code**.
- **RG-02, ciel de midi** (option A du PO, avis de l'expert réalisme 3D) (`eclairage_unites.h`, `photometrie.json`, `SOURCES.md`) :
  - Le ciel était calé sur 6 000 lx, valeur mesurée **à l'ombre**. Le ciel entier suit maintenant Krochmann et Seidl 1974 : **15 825 lx** à h 70°.
  - Les 6 000 lx restent la valeur de contrôle « point à l'ombre » : au pied d'un mur, le même ciel donne **5 498 lx** (−8 %).
  - Le référentiel gagne l'entrée `lum.ciel_clair.plein_ciel` (±15 %, « à confirmer » : article non ouvert).
  - Le crépuscule et la nuit sont inchangés (raccord au niveau d'avant, `eclairage_physique.cpp`).
- **RG-03, chaussée en Ultra** : rien à corriger dans le rendu. Sur `main`, Ultra et Haute donnent déjà la même chaussée grise (ANO-106 a été corrigée en chemin par un autre ticket). Ajouts : une **preuve au banc** (accroche de 10 lignes dans `bench_passes.cpp` : Ultra puis Haute posés directement, captures `chaussee-*-course`) et la mesure du ΔE.
- **RG-05, vision de nuit loin des lumières** (`post.frag`, `eclairage_exposition.h`, `eclairage_physique.*`, accroche d'une ligne dans `renderer.cpp`) :
  - Le poids de la vision de nuit suit maintenant la **luminance d'adaptation de l'œil**, tirée de l'EV mesuré autour du joueur, ou celle du pixel si elle est plus forte.
  - Avant, il suivait seulement la luminance du pixel : une peinture foncée sous une lampe était traitée comme un paysage sous la lune.
  - Golf sous un lampadaire (15 lx, pire cas EV 3) : **ΔE2000 de 5,6 à 3,1** par rapport à la même image sans vision de nuit (seuil 10).
  - En rase campagne : poids 0,87 au lieu de 0,90, soit ΔE2000 ≤ 1,2 (sous le seuil de perception).
- RG-04 : rien de WFS-208 n'est retiré (nuages, 6 moments).

## Comment c'est vérifié
- **Tests CPU** : nouveau `test_post_wfs214` (tramage, vision de nuit, constantes identiques au shader). `test_fidelite_lumiere` reçoit 5 vérifications de plus : ciel entier, contrôle à l'ombre, zénith 6 à 8 kcd/m², rapport soleil / ombre 2,5 à 3,2 IL, référentiel. Aucune vérification n'est supprimée ni affaiblie. `test_fidelite` est vert. Les 92 tests passent (après synchronisation avec main) (`tools/qa/tests_cpu.sh`).
- **Banc `realisme` (mesures)** :

  | Mesure | Avant | Après | Cible |
  |---|---|---|---|
  | Zénith à h 70° | 2 667 cd/m² | **7 034 cd/m²** | 6 à 8 kcd/m² |
  | Ciel entier (modèle) | 5 965 lx | **15 732 lx** | 15,8 klx ±15 % |
  | Rapport soleil / ombre rendu | — | **7,76, soit 2,96 IL** | 2,5 à 3,2 IL |
  | Teinte du zénith | x 0,2838 ; y 0,2923 | inchangée | gardée |
  | EV de jour (Ville) | 15 | 15 | l'exposition n'annule pas le gain |

- **Chaussée Ultra / Haute** : ΔE2000 de **0,02 à 0,03** sur la chaussée et de **0,92** sur le trottoir (seuil 3).
- **Banc `depart`** : nuit (Ville, Corniche, Campagne) et jour (Ville, Campagne).
- **Banc `perf`** : voir le point 3 du retour ci-dessous.

## Limites et risques
- **M-03 du banc `realisme`** (rapport soleil / ombre contre le modèle) : écart de 5,5 % pour un seuil de 5 %, donc NON CONFORME, signalé « écart attendu ». La cause : la sonde de la Campagne voit le relief et les arbres, qui cachent environ 10 % du ciel (14 204 lx contre 15 732). Le seuil est gardé tel quel. Le rapport mesuré (2,96 IL) est dans la cible du PO.
- **Pas d'occlusion du ciel sur les objets mobiles** (voiture, plaque du banc) : les ombres de jour sont un peu plus claires partout ; au soleil bas (h 10°), les murs à l'ombre s'éclaircissent nettement. Le décor fixe garde son occlusion précalculée (WFS-269). Si le lead dev ou l'utilisateur trouve les ombres délavées, le PO crée l'US « occlusion du ciel » (P2), comme il l'a prévu.
- **Défaut déjà présent sur `main`, hors de ce ticket** : le banc des passes ne trouve plus le choix « Ultra » dans le panneau à onglets (« introuvable, 21 zones », parcours 2/5 avant comme après). Sa capture `parcours-ultra-course` est donc en Haute ; signalé au PO. Mes captures `chaussee-*` posent Ultra directement.
- **Captures** : elles sont faites selon la méthode de `captures_pr.sh`, mais avec les bancs lancés en parallèle (voir `qa/pr/WFS-214/README.md`). Pas de CI de branche (ce n'est pas une US technique). Rien n'a été essayé sur téléphone.

## Retour du lead dev (revue du commit 75749e6e) : corrections
1. **Moments de jour plus blancs** : la cause est un **défaut du banc**, déjà présent sur main. Le banc des moments fige le temps et ne recrée pas la scène, si bien que l'exposition restait celle de l'aube : EV 8,2 à midi au lieu de 15, soit 7 IL de trop. Les images étaient donc blanches avant comme après, et un ciel plus clair les blanchissait encore. Correction (`eclairage_banc.cpp`) : l'exposition atteint sa cible avant la capture, comme dans le jeu, où le temps avance. Résultat : midi EV 14,98, heure dorée EV 13,2 à 14,6. Avec ce banc corrigé des deux côtés, la luminance moyenne des moments de jour va de 46-84 à 60-130 sur 255. Rien n'est blanc : la colline, l'éolienne et les vaches se voient. L'heure dorée de la Campagne ne change pas (60 → 60) ; l'aube, l'heure bleue et la nuit sont identiques.
2. **Captures ajoutées** : heure dorée (Campagne et circuit), midi (Campagne et circuit), matin du circuit, et `depart-jour-campagne-4`. Pour ce dernier, le banc `depart` fait maintenant aussi un départ de jour en Campagne.
3. **Performance** (banc `perf`, 4 cartes × Haute / Basse, 2 mesures avant, 1 après) : image entière de **−4,1 % à +0,2 %** par rapport à la moyenne d'avant, dans le bruit de mesure (les deux mesures d'avant diffèrent elles-mêmes de 2 à 7 %). La part hors des passes mesurées, qui contient la passe finale, bouge de 2,1 ms au plus sur des images de 420 à 650 ms (lavapipe). Le tramage n'est donc pas simplifié. Limite : lavapipe n'est pas le GPU du téléphone ; le coût ajouté est de 2 `pow(vec3)` et 2 hachages par pixel dans la seule passe finale.
4. **Remarques** :
   - M-03 est noté dans le Suivi de `regles-de-gestion/WFS-214.md`.
   - Le banc des passes qui ne trouve plus « Ultra » est signalé au PO : **ANO-211**, US technique **WFS-377**.
   - La branche est synchronisée avec main (WFS-338 compris).

## Captures pour le lead dev (18 après)

Dossier : [`qa/pr/WFS-214`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214) — tableau complet : [`README.md`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/README.md).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `realisme-ciel-campagne-h70` — RG-02 : ciel de midi (soleil à 70°, EV 15 imposé) : gris-bleu sombre → bleu clair (zénith 2 667 → 7 034 cd/m²) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-realisme-ciel-campagne-h70.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-realisme-ciel-campagne-h70.jpg?raw=true) |
| `realisme-ciel-circuit-h10` — RG-01 / RG-02 : ciel du soir (soleil à 10°) autour du soleil : dégradé continu | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-realisme-ciel-circuit-h10.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-realisme-ciel-circuit-h10.jpg?raw=true) |
| `zoom-tramage-ciel-h10` — RG-01 : zoom × 2 autour du soleil, contraste renforcé (niveaux 25-75 %) : plus de marches | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-zoom-tramage-ciel-h10.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-zoom-tramage-ciel-h10.jpg?raw=true) |
| `realisme-ciel-midi-campagne` — moment midi, Campagne (retour du lead dev) : luminance moyenne 46 → 65 / 255 | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-realisme-ciel-midi-campagne.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-realisme-ciel-midi-campagne.jpg?raw=true) |
| `realisme-ciel-midi-circuit` — moment midi, circuit : 79 → 123 / 255 | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-realisme-ciel-midi-circuit.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-realisme-ciel-midi-circuit.jpg?raw=true) |
| `realisme-ciel-matin-campagne` — moment matin (soleil à 25°, vue « h 30 » du PO), Campagne : 61 → 90 / 255 | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-realisme-ciel-matin-campagne.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-realisme-ciel-matin-campagne.jpg?raw=true) |
| `realisme-ciel-matin-circuit` — moment matin, circuit : 84 → 130 / 255 (la plus claire : rien de blanc) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-realisme-ciel-matin-circuit.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-realisme-ciel-matin-circuit.jpg?raw=true) |
| `realisme-ciel-doree-campagne` — heure dorée (coucher, condition 4), Campagne : 60 → 60 / 255 | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-realisme-ciel-doree-campagne.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-realisme-ciel-doree-campagne.jpg?raw=true) |
| `realisme-ciel-doree-circuit` — heure dorée, circuit : 71 → 78 / 255 | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-realisme-ciel-doree-circuit.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-realisme-ciel-doree-circuit.jpg?raw=true) |
| `realisme-ciel-aube-campagne` — condition 3 : aube (soleil sous l'horizon) identique (66 → 66) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-realisme-ciel-aube-campagne.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-realisme-ciel-aube-campagne.jpg?raw=true) |
| `depart-jour-campagne-4` — jeu de jour hors de la Ville (retour du lead dev) : ombres un peu plus claires, relief gardé (97 → 104 / 255) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-depart-jour-campagne-4.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-depart-jour-campagne-4.jpg?raw=true) |
| `depart-jour-ville-4` — RG-02 : Ville de jour, ombres (dessous de la Golf, façades) ; EV 15 avant et après | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-depart-jour-ville-4.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-depart-jour-ville-4.jpg?raw=true) |
| `parcours-ultra-course` — RG-03 : capture d'origine d'ANO-106 (sur main, ce banc n'atteint plus Ultra : ANO-211 / WFS-377) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-parcours-ultra-course.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-parcours-ultra-course.jpg?raw=true) |
| `chaussee-ultra-course` — RG-03 : vrai Ultra (sonde 256 px), même instant que la suivante | — | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-chaussee-ultra-course.jpg?raw=true) |
| `chaussee-haute-course` — RG-03 : Haute (sonde 128 px) : chaussée ΔE2000 0,02 à 0,03, trottoir 0,92 avec Ultra | — | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-chaussee-haute-course.jpg?raw=true) |
| `depart-nuit-ville-4` — RG-05 : nuit, Golf sous les lampadaires de la Ville : garde le bleu de sa peinture | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-depart-nuit-ville-4.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-depart-nuit-ville-4.jpg?raw=true) |
| `depart-nuit-campagne-4` — RG-05 / condition 3 : nuit en Campagne loin des lumières : vision de nuit gardée (66 → 66) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-depart-nuit-campagne-4.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-depart-nuit-campagne-4.jpg?raw=true) |
| `realisme-ciel-campagne-nuit` — condition 3 : ciel de nuit inchangé | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/avant-realisme-ciel-campagne-nuit.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-214/qa/pr/WFS-214/apres-realisme-ciel-campagne-nuit.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv4EdEBdMyt3hmgrMoT
