Agent : Claude, agent dev3d (01Us6uCwDY)

**Ticket** : WFS-490 — règles : `regles-de-gestion/WFS-490.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : le joueur choisit la finition de sa peinture (brillant, métallisé, nacré, mat, satiné, chromé, caméléon, candy) et chacune doit ressembler à la vraie peinture automobile du même nom, de jour comme de nuit (épic E18, RG-01 à RG-12 de `regles-de-gestion/WFS-490.md`).
- **Ce qui change** :
  - `shaders/vehicule.glsl` : `finitionSurface`, `vernisFinition`, `vernisCouche` (ancien `vernisPeinture` paramétré, que `vernisPeinture` appelle avec les valeurs d'avant). Paillettes procédurales ; **aucune texture ajoutée**.
  - Transport : finition codée dans le métal des pièces peintes (`pc.params.x = métal + 2 × finition`, `src/render/vehicule_finitions.h`) ; joueur seulement (RG-11) ; la couleur 2 (toit, rétros) suit. API pour WFS-491 : `Renderer::finitionJoueur(int)` ; bancs : `WFS_FINITION=<id>`.
  - Finitions (pas le brillant) : carrosserie pleine (`specK = 1`). Les caisses très denses (Corvette, 208), classées « très fines », perdaient leurs reflets.
  - **Chromé** : métal pur, **aucune part diffuse**. La sonde floue est retirée (`env = 0`), la lumière de nuit de la chaussée aussi. Miroir net : ciel au-dessus, chaussée sombre en dessous, horizon net (`smoothstep(-0.05, 0.02, R.y)`) ; teinte 15 %.
  - **Paillettes** : seulement dans la tache du soleil (± 10°) et de près (pixel ≤ 3 mm), jamais plus petites qu'un pixel.
  - Accroches hors zone (au journal) :
    - `shaders/scene.frag` : 6 lignes ;
    - `src/render/renderer.cpp` : 4 lignes ;
    - `renderer.h` : 5 lignes ;
    - `desktop_main.cpp` : 2 lignes ;
    - `CMakeLists.txt` : 2 lignes.
  - Banc `finitions`, test `test_fidelite_voitures_finitions`, valeurs sourcées dans `references/reel/voitures/peintures.md`.
- **Comment c'est vérifié** :
  - **Brillant identique au pixel près** (RG-01) : banc `peinture2` avant / après, 14 images, 0 pixel différent.
  - **Mesures du chromé** (luminance affichée 0..1, sur les captures de ce commit) :

    | Mesure | GT86 | Corvette | 208 |
    |---|---|---|---|
    | Jour, carrosserie chromée (médiane) | 0,287 | 0,319 | 0,292 |
    | **Nuit, chromé / brillant** | **0,027 / 0,100** | **0,025 / 0,042** | **0,033 / 0,058** |
    | Portière (gros plan) haut / bas | 0,301 / 0,048 : **écart 0,253** | — | — |

  - Images regardées une à une :
    - planches des 8 finitions (3 voitures, jour et nuit) et gros plans des 8 finitions ;
    - caméléon sous 3 angles ;
    - 208 en course en chromé et en mat ;
    - 208 avec toit et rétros Noir en métallisé (**plus de « ciel étoilé »**) et en chromé ;
    - de nuit, aucune tache blanche.
  - Tests CPU : section ci-dessous. Performances : tableau ci-dessous.
- **Limites et risques connus** :
  - Le choix au garage et la sauvegarde relèvent de WFS-491 : rien ne change sur l'APK avant ce ticket.
  - Chromé : miroir réaliste, donc sombre là où il reflète la chaussée (bas de caisse). Couleur choisie limitée à 15 % de teinte.
  - Caméléon : quelques plages or suivent les facettes du capot de la Corvette (mineur, noté par l'agent 3D).
  - Hors ticket, signalés au PO dans le journal :
    - en brillant, `specK` atténue les reflets des caisses très denses ;
    - logo au centre des jantes de la GT86.
  - Mesure des img/s sur lavapipe, pas sur téléphone.

## Tests CPU

Commit testé : `cf5c142e`, branche synchronisée avec `main`. Journal complet : [`qa/pr/WFS-490/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/tests-cpu.log).

```
[OK] test_fidelite_voitures_finitions
tests CPU : 125 test(s) réussi(s) en 348 s
```

## Performances en Basse (RG-10, RC-E18-03)

Banc `peinture2-mesure` : 208, Ville, 600 images, lavapipe, 3 passages de chaque côté, mesuré sur c48dc1e6.

Les corrections suivantes ne changent pas le coût en Basse :
- le chromé lit une sonde de moins ;
- les paillettes sont éteintes en Basse.

| | Passage 1 | Passage 2 | Passage 3 | Moyenne | 1 % les plus lents (moyenne) |
|---|---|---|---|---|---|
| main | 3,734 | 3,762 | 3,485 | **3,660 img/s** | 2,591 img/s |
| branche, brillant | 3,701 | 3,827 | 3,754 | **3,761 img/s** | 2,710 img/s |
| branche, métallisé | 3,710 | 3,662 | 3,235 | **3,536 img/s** | 2,457 img/s |

Écart avec `main` : ≤ 0,13 img/s (limite : 1 img/s). Sur `main` seul, l'écart entre passages atteint déjà 0,28 img/s.

## Captures pour le lead dev (37 après)

Dossier : [`qa/pr/WFS-490`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490) — produit par `tools/qa/captures_pr.sh`.

**Ordre des planches** : en haut, brillant, métallisé, nacré, mat ; en bas, satiné, chromé, caméléon, candy (Rouge). Le banc `finitions` n'existe pas sur `main` (« — »). Les `peinture2-*` montrent le brillant avant / après, identique au pixel près.

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
