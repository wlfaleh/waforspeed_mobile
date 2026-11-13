Agent : Claude, agent dev3d (01Us6uCwDY)

**Ticket** : WFS-490 — règles : `regles-de-gestion/WFS-490.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : le joueur choisit la finition de sa peinture (brillant, métallisé, nacré, mat, satiné, chromé, caméléon, candy) et chacune doit ressembler à la vraie peinture automobile du même nom, de jour comme de nuit (épic E18, RG-01 à RG-12 de `regles-de-gestion/WFS-490.md`).
- **Ce qui change** :
  - `shaders/vehicule.glsl` :
    - `finitionSurface` : couleur, métal, rugosité selon la finition ;
    - `vernisFinition` : paillettes, vernis satiné, rien pour le mat et le chromé ;
    - `vernisCouche` : l'ancien `vernisPeinture`, paramétré ;
    - `chausseeReflechie` : chaussée vue dans le chrome ;
    - **aucune texture ajoutée**.
  - Transport : finition codée dans le métal des pièces peintes (`pc.params.x = métal + 2 × finition`, `src/render/vehicule_finitions.h`), pour la voiture du joueur seulement (RG-11) ; la couleur 2 suit la même finition. API pour WFS-491 : `Renderer::finitionJoueur(int)`.
  - Finitions (hors brillant) : la carrosserie est traitée comme pleine (`specK = 1`). Sinon, les caisses très denses perdaient leurs reflets.
  - **Chromé** : métal PBR pur, sans part diffuse. Au-dessus de l'horizon, la sonde d'environnement à pleine force (RG-06) ; en dessous, la chaussée réelle (asphalte ρ 0,14, DA-realisme § 3) ; teinte 15 %.
  - **Paillettes** : seulement dans la tache du soleil, et seulement de près.
  - Accroches hors zone (au journal) : `shaders/scene.frag` 8 lignes, `src/render/renderer.cpp` 4, `renderer.h` 5, `desktop_main.cpp` 2, `CMakeLists.txt` 2.
  - Ajouts : banc `finitions`, test `test_fidelite_voitures_finitions`, valeurs sourcées dans `references/reel/voitures/peintures.md`.
- **Arbitrage du PO (2026-11-13, commentaire 6862309388, `regles-de-gestion/WFS-490.md`)** : chromé physique accepté. Le critère « haut de portière plus clair que le bas » et le « décor reconnaissable dans le reflet » sont retirés de WFS-490 et confiés à **WFS-539** : la sonde ne contient que le ciel (preuve : `qa/pr/WFS-490/preuve-sonde-brute-gt86.jpg`), et son ciel est plus sombre que l'asphalte au soleil. **Limite : le chrome reflète le ciel seul ; le décor et un ciel calé sur le réel viendront de WFS-539.**
- **Comment c'est vérifié** :
  - **Brillant identique au pixel près** (RG-01) : banc `peinture2` avant / après, 14 images, 0 pixel différent.
  - **Chromé, critères de l'arbitrage** (luminance linéaire, captures de ce commit) :

    | Mesure | Bas de caisse | Chaussée au soleil | Rapport (≥ 40 %) |
    |---|---|---|---|
    | `p208-toit-chrome` | 0,130 | 0,234 | **56 %** |
    | `gt86-chrome-pres` | 0,171 | 0,110 | **155 %** |
    | Planche GT86 (case chromé) | 0,134 | 0,246 | **54 %** |
    | Planche Corvette (case chromé) | 0,118 | 0,246 | **48 %** |
    | Planche 208 (case chromé) | 0,123 | 0,247 | **50 %** |

    Nuit (luminance affichée, médiane) : chromé contre brillant **0,027 / 0,099** (GT86), **0,025 / 0,041** (Corvette), **0,034 / 0,058** (208). Le chromé est plus sombre partout.
  - Les 7 autres finitions sont inchangées depuis la 3e revue (validées par le lead dev et par l'agent 3D).
  - Tests CPU et performances : sections ci-dessous.
- **Limites et risques connus** :
  - Le choix de la finition au garage et la sauvegarde relèvent de WFS-491 : rien ne change sur l'APK avant ce ticket.
  - Le chrome reflète le ciel seul (WFS-539).
  - Caméléon : quelques plages or suivent les facettes du capot de la Corvette (mineur).
  - Hors ticket : ANO-266 (logo des jantes de la GT86), et `specK` en brillant (journal).
  - Img/s mesurées sur lavapipe, pas sur téléphone.

## Tests CPU

Commit testé : `99657589`, branche synchronisée avec `main`. Journal complet : [`qa/pr/WFS-490/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/tests-cpu.log).

```
[OK] test_fidelite_voitures_finitions
tests CPU : 127 test(s) réussi(s) en 345 s
```

## Performances en Basse (RG-10, RC-E18-03)

Banc `peinture2-mesure`, 208, Ville, 600 images, lavapipe, 3 passages de chaque côté, sur c48dc1e6. Depuis, le chromé ne lit qu'une sonde, comme les autres matériaux, et les paillettes sont éteintes en Basse.

| | Passage 1 | Passage 2 | Passage 3 | Moyenne | 1 % les plus lents |
|---|---|---|---|---|---|
| main | 3,734 | 3,762 | 3,485 | **3,660 img/s** | 2,591 img/s |
| branche, brillant | 3,701 | 3,827 | 3,754 | **3,761 img/s** | 2,710 img/s |
| branche, métallisé | 3,710 | 3,662 | 3,235 | **3,536 img/s** | 2,457 img/s |

Écart avec `main` : ≤ 0,13 img/s (limite : 1 img/s).

## Captures pour le lead dev (37 après)

Dossier : [`qa/pr/WFS-490`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490) — produit par `tools/qa/captures_pr.sh`.

**Ordre des planches** : en haut, brillant, métallisé, nacré, mat ; en bas, satiné, chromé, caméléon, candy (Rouge). Le banc `finitions` n'existe pas sur `main` (« — »). Les `peinture2-*` montrent le brillant avant / après, identique au pixel près. Preuve que la sonde ne contient que le ciel : `preuve-sonde-brute-gt86.jpg`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `finitions-gt86-brillant-pres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-gt86-brillant-pres.jpg?raw=true) |
| `finitions-gt86-cameleon-pres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-gt86-cameleon-pres.jpg?raw=true) |
| `finitions-gt86-candy-pres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-gt86-candy-pres.jpg?raw=true) |
| `finitions-gt86-chrome-pres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-gt86-chrome-pres.jpg?raw=true) |
| `finitions-gt86-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-gt86-jour.jpg?raw=true) |
| `finitions-gt86-mat-pres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-gt86-mat-pres.jpg?raw=true) |
| `finitions-gt86-metallise-pres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-gt86-metallise-pres.jpg?raw=true) |
| `finitions-gt86-nacre-pres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-gt86-nacre-pres.jpg?raw=true) |
| `finitions-gt86-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-gt86-nuit.jpg?raw=true) |
| `finitions-gt86-satine-pres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-gt86-satine-pres.jpg?raw=true) |
| `finitions-mustang-cameleon-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-mustang-cameleon-1.jpg?raw=true) |
| `finitions-mustang-cameleon-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-mustang-cameleon-2.jpg?raw=true) |
| `finitions-mustang-cameleon-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-mustang-cameleon-3.jpg?raw=true) |
| `finitions-mustang-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-mustang-jour.jpg?raw=true) |
| `finitions-mustang-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-mustang-nuit.jpg?raw=true) |
| `finitions-p208-course-chrome` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-p208-course-chrome.jpg?raw=true) |
| `finitions-p208-course-mat` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-p208-course-mat.jpg?raw=true) |
| `finitions-p208-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-p208-jour.jpg?raw=true) |
| `finitions-p208-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-p208-nuit.jpg?raw=true) |
| `finitions-p208-toit-chrome-dessus` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-p208-toit-chrome-dessus.jpg?raw=true) |
| `finitions-p208-toit-chrome` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-p208-toit-chrome.jpg?raw=true) |
| `finitions-p208-toit-metallise-dessus` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-p208-toit-metallise-dessus.jpg?raw=true) |
| `finitions-p208-toit-metallise` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-p208-toit-metallise.jpg?raw=true) |
| `peinture2-course-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/avant-peinture2-course-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-peinture2-course-jour.jpg?raw=true) |
| `peinture2-course-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/avant-peinture2-course-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-peinture2-course-nuit.jpg?raw=true) |
| `peinture2-gt86-34` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/avant-peinture2-gt86-34.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-peinture2-gt86-34.jpg?raw=true) |
| `peinture2-gt86-dessus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/avant-peinture2-gt86-dessus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-peinture2-gt86-dessus.jpg?raw=true) |
| `peinture2-gt86-joint` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/avant-peinture2-gt86-joint.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-peinture2-gt86-joint.jpg?raw=true) |
| `peinture2-gt86-retro` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/avant-peinture2-gt86-retro.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-peinture2-gt86-retro.jpg?raw=true) |
| `peinture2-mustang-34` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/avant-peinture2-mustang-34.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-peinture2-mustang-34.jpg?raw=true) |
| `peinture2-mustang-dessus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/avant-peinture2-mustang-dessus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-peinture2-mustang-dessus.jpg?raw=true) |
| `peinture2-mustang-joint` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/avant-peinture2-mustang-joint.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-peinture2-mustang-joint.jpg?raw=true) |
| `peinture2-p208-34` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/avant-peinture2-p208-34.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-peinture2-p208-34.jpg?raw=true) |
| `peinture2-p208-dessus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/avant-peinture2-p208-dessus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-peinture2-p208-dessus.jpg?raw=true) |
| `peinture2-p208-joint` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/avant-peinture2-p208-joint.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-peinture2-p208-joint.jpg?raw=true) |
| `peinture2-p208-retro` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/avant-peinture2-p208-retro.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-peinture2-p208-retro.jpg?raw=true) |
| `peinture2-planche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/avant-peinture2-planche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-peinture2-planche.jpg?raw=true) |
