## Agent : Claude, agent conduite (01UY1Vv4e7)

**Ticket** : WFS-391 — règles : `regles-de-gestion/WFS-391.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : corriger la conduite au bas-côté (tâche séparée, ordre du 2026-11-09 19:14 UTC) : sur la neige, la résistance de l'air ne doit pas dépendre de l'adhérence et le bas-côté enneigé doit accrocher moins que la chaussée (ANO-229) ; dans les épingles du col de la Campagne, le rayon au bas-côté intérieur était faux (ANO-231).
- **Ce qui change** :
  - `src/game/player.cpp` (ancienne conduite, celle du jeu) :
    - **RG-01** : au freinage, seules les forces qui passent par les pneus (freins et frein moteur) sont multipliées par μ. La traînée de l'air (½ ρ S Cx v²) et le roulement ne dépendent plus du sol. Avant, toute la résistance était multipliée par μ.
    - **RG-02** : le μ du bas-côté vaut au plus μ chaussée × 0,6 (rapport du sec). Neige : 0,2 → 0,15. Sec et automne inchangés. Les valeurs viennent du paquet, aucune constante propre à une carte. Vaut pour les deux modèles de conduite.
  - **RG-06 (ANO-231)** : rien à corriger dans la conduite (arbitrage du PO dans `regles-de-gestion/WFS-391.md`). Le point mesuré à 11,5 m est **derrière la glissière** du col, qui est à 10,35 m (edge 1,38). La voiture y est ramenée dès le 1er pas et longe la glissière, d'où le rayon de 22 − 10,35 = 11,65 m. Le test mesure maintenant les 5 positions devant la glissière (bas-côtés à 9 m). Il vérifie aussi que la voiture posée derrière la glissière y est ramenée et la longe.
  - `src/game/player.h` : interrupteur réservé au banc `g_avantWfs391`, qui rejoue main pour les captures « avant » (même principe que `g_avantWfs295` et `g_avantWfs316`) ; toujours faux dans le jeu.
  - Nouveau banc `WFS_BANC=sol` (`src/game/sol_banc.cpp`, ≈ 200 lignes), branché par une accroche de 2 lignes dans `src/platform/desktop_main.cpp`.
  - Tests : `tests/test_cartes_v2_saison.cpp` (traînée identique quel que soit μ à 100 et 50 km/h ; bas-côté enneigé ≤ chaussée dans les deux modèles ; plage de 120 à 160 m ; sec 36,6 m ± 3 %) et `tests/test_virages_fleches.cpp` (5 positions des 4 épingles du col, ± 3 % ; voiture posée derrière la glissière).
  - **À l'écran** : sur la neige, la voiture s'arrête plus tôt (environ 127 m au lieu de 145 m depuis 100 km/h) et tourne moins sur le bas-côté enneigé. Au sec, rien ne change.
- **Comment c'est vérifié** :
  - les 94 tests CPU passent (`tools/qa/tests_cpu.sh build`) ; les nouveaux tests **échouent sur le code de main** (5 échecs), donc ils détectent bien le défaut ;
  - banc `sol` : 0 écart ; banc `neige` : 0 écart ;
  - parcours complet (`WFS_BANC=parcours`) ;
  - chiffres ci-dessous et dans [`sol.txt`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/sol.txt).

| Mesure (Golf, ancienne conduite) | Avant (main) | Après | Règle |
|---|---|---|---|
| Freinage 100 → 0, neige μ 0,25 (banc `sol`) | 145,2 m | **126,9 m** | RG-01 : 120 à 160 m (arbitrage du PO) |
| Freinage depuis 63 km/h, neige (banc `neige`) | 56,6 m (≈ 144 m depuis 100) | 50,7 m (≈ 129 m) | RG-01 |
| Freinage 100 → 0, Ville au sec | 36,8 m | **36,8 m** | RG-03 : 36,6 m ± 3 % |
| Limite latérale au sec (banc `neige`) | 2,11 g | 2,11 g | RG-03 |
| Limite latérale, virage sur la neige (banc `neige`) | 0,70 g (rapport 0,33) | 0,53 g (rapport 0,25 = μ) | RG-02 |
| Cap gagné en 0,6 s sur le bas-côté enneigé, 50 km/h, volant à fond | 0,331 rad | 0,251 rad | RG-02 |
| Part du freinage sans μ (air + roulement), 100 km/h | 0 (tout × μ) | 0,674 m/s², la même pour tout μ | RG-04 |
| Épingles du col, 10 km/h, volant à fond : centre / bords / bas-côtés à 9 m | 4,13 / 4,11 à 4,13 / 4,11 à 4,13 m | identique (± 0,4 %) | RG-06 : ± 3 % |
| Voiture posée à 11,5 m, derrière la glissière | 11,64 m (elle longe la glissière) | 11,64 m (même chose, voulu) | RG-06 : cause de l'ANO-231 |

- **Limites et risques connus** :
  - Le roulement du jeu (`conduite.roulement` 0,35 m/s², environ 3 fois le réel) explique que l'ancien modèle donne 127 m et non 140 à 157 m. Ce point est noté par le PO pour WFS-258 ; `configgame.json` n'est pas touché.
  - Le nouveau modèle (`conduite_monde`, inactif dans le jeu) avait déjà la traînée juste : 143,5 m.
  - Dans les captures du col, la voiture roule sur l'herbe au-delà du muret visible, qui est à 8 m. C'est l'ANO-234, rangée par le PO dans WFS-387 : hors de ce ticket.
  - Le ressenti sur téléphone reste à faire valider par l'utilisateur (ticket de conduite : 🟡 après la fusion).
  - Dans les captures `sol-*`, « avant » et « après » sont dans le **nom** de l'image : le banc `sol` est nouveau et rejoue main avec `g_avantWfs391`. Il n'existe pas sur main, d'où la colonne « Avant (main) » vide pour ces lignes. Les captures `neige-freinage-*` et `neige-virage-*` viennent du vrai main.

**À regarder** :
- `sol-neige-freinage-avant` / `-apres` : la voiture s'arrête avant (même distance entre les images, vitesse plus basse) ;
- `sol-ville-freinage-*` : identiques ;
- `sol-neige-bascote-*` : le cap grandit moins ;
- `sol-col-bascote-9m-*` : cercle serré au bas-côté intérieur ;
- `sol-col-glissiere-*` : la voiture posée à 11,5 m longe la glissière.

## Captures pour le lead dev (50 après)

Dossier : [`qa/pr/WFS-391`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `neige-freinage-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/avant-neige-freinage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-neige-freinage-0.jpg?raw=true) |
| `neige-freinage-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/avant-neige-freinage-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-neige-freinage-1.jpg?raw=true) |
| `neige-freinage-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/avant-neige-freinage-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-neige-freinage-2.jpg?raw=true) |
| `neige-freinage-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/avant-neige-freinage-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-neige-freinage-3.jpg?raw=true) |
| `neige-freinage-4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/avant-neige-freinage-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-neige-freinage-4.jpg?raw=true) |
| `neige-virage-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/avant-neige-virage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-neige-virage-0.jpg?raw=true) |
| `neige-virage-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/avant-neige-virage-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-neige-virage-1.jpg?raw=true) |
| `neige-virage-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/avant-neige-virage-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-neige-virage-2.jpg?raw=true) |
| `neige-virage-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/avant-neige-virage-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-neige-virage-3.jpg?raw=true) |
| `neige-virage-4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/avant-neige-virage-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-neige-virage-4.jpg?raw=true) |
| `sol-col-bascote-9m-00` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-col-bascote-9m-00.jpg?raw=true) |
| `sol-col-bascote-9m-01` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-col-bascote-9m-01.jpg?raw=true) |
| `sol-col-bascote-9m-02` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-col-bascote-9m-02.jpg?raw=true) |
| `sol-col-bascote-9m-03` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-col-bascote-9m-03.jpg?raw=true) |
| `sol-col-bascote-9m-04` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-col-bascote-9m-04.jpg?raw=true) |
| `sol-col-glissiere-00` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-col-glissiere-00.jpg?raw=true) |
| `sol-col-glissiere-01` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-col-glissiere-01.jpg?raw=true) |
| `sol-col-glissiere-02` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-col-glissiere-02.jpg?raw=true) |
| `sol-col-glissiere-03` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-col-glissiere-03.jpg?raw=true) |
| `sol-col-glissiere-04` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-col-glissiere-04.jpg?raw=true) |
| `sol-neige-bascote-apres-00` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-bascote-apres-00.jpg?raw=true) |
| `sol-neige-bascote-apres-01` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-bascote-apres-01.jpg?raw=true) |
| `sol-neige-bascote-apres-02` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-bascote-apres-02.jpg?raw=true) |
| `sol-neige-bascote-apres-03` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-bascote-apres-03.jpg?raw=true) |
| `sol-neige-bascote-apres-04` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-bascote-apres-04.jpg?raw=true) |
| `sol-neige-bascote-avant-00` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-bascote-avant-00.jpg?raw=true) |
| `sol-neige-bascote-avant-01` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-bascote-avant-01.jpg?raw=true) |
| `sol-neige-bascote-avant-02` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-bascote-avant-02.jpg?raw=true) |
| `sol-neige-bascote-avant-03` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-bascote-avant-03.jpg?raw=true) |
| `sol-neige-bascote-avant-04` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-bascote-avant-04.jpg?raw=true) |
| `sol-neige-freinage-apres-00` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-freinage-apres-00.jpg?raw=true) |
| `sol-neige-freinage-apres-01` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-freinage-apres-01.jpg?raw=true) |
| `sol-neige-freinage-apres-02` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-freinage-apres-02.jpg?raw=true) |
| `sol-neige-freinage-apres-03` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-freinage-apres-03.jpg?raw=true) |
| `sol-neige-freinage-apres-04` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-freinage-apres-04.jpg?raw=true) |
| `sol-neige-freinage-avant-00` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-freinage-avant-00.jpg?raw=true) |
| `sol-neige-freinage-avant-01` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-freinage-avant-01.jpg?raw=true) |
| `sol-neige-freinage-avant-02` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-freinage-avant-02.jpg?raw=true) |
| `sol-neige-freinage-avant-03` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-freinage-avant-03.jpg?raw=true) |
| `sol-neige-freinage-avant-04` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-neige-freinage-avant-04.jpg?raw=true) |
| `sol-ville-freinage-apres-00` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-ville-freinage-apres-00.jpg?raw=true) |
| `sol-ville-freinage-apres-01` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-ville-freinage-apres-01.jpg?raw=true) |
| `sol-ville-freinage-apres-02` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-ville-freinage-apres-02.jpg?raw=true) |
| `sol-ville-freinage-apres-03` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-ville-freinage-apres-03.jpg?raw=true) |
| `sol-ville-freinage-apres-04` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-ville-freinage-apres-04.jpg?raw=true) |
| `sol-ville-freinage-avant-00` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-ville-freinage-avant-00.jpg?raw=true) |
| `sol-ville-freinage-avant-01` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-ville-freinage-avant-01.jpg?raw=true) |
| `sol-ville-freinage-avant-02` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-ville-freinage-avant-02.jpg?raw=true) |
| `sol-ville-freinage-avant-03` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-ville-freinage-avant-03.jpg?raw=true) |
| `sol-ville-freinage-avant-04` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-391/qa/pr/WFS-391/apres-sol-ville-freinage-avant-04.jpg?raw=true) |
