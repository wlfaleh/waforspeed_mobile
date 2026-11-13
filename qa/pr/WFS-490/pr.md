Agent : Claude, agent dev3d (01Us6uCwDY)

**Ticket** : WFS-490 — règles : `regles-de-gestion/WFS-490.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : le joueur choisit la finition de sa peinture (brillant, métallisé, nacré, mat, satiné, chromé, caméléon, candy) et chacune doit ressembler à la vraie peinture automobile du même nom, de jour comme de nuit (épic E18, RG-01 à RG-12 de `regles-de-gestion/WFS-490.md`).
- **Ce qui change** :
  - `shaders/vehicule.glsl` :
    - `finitionSurface` (couleur, métal, rugosité selon la finition, angle pris sur la normale lissée) ;
    - `vernisFinition` (paillettes procédurales, vernis satiné flou, miroir du chrome, pas de vernis pour mat et chromé) ;
    - `vernisCouche` (ancien `vernisPeinture`, paramétré ; `vernisPeinture` l'appelle avec les valeurs d'avant) ;
    - **aucune texture ajoutée**.
  - Transport : la finition est codée dans le métal des pièces peintes (`pc.params.x = métal + 2 × finition`, `src/render/vehicule_finitions.h`), sans constante de poussée en plus. Seule la voiture du joueur la reçoit (RG-11) ; la couleur 2 (toit, rétros) suit la même finition. API pour WFS-491 : `Renderer::finitionJoueur(int)` ; bancs : `WFS_FINITION=<id>`.
  - Pour les finitions (pas le brillant), la carrosserie couvre toujours le pixel (`specK = 1`). Sinon, les caisses très denses (Corvette 2,4 mm, 208) classées « très fines » perdaient leurs reflets : c'était la cause du chromé noir.
  - Accroches hors zone (journal, entrée 14:52:03 et suivante) :
    - `shaders/scene.frag` : 5 lignes ;
    - `src/render/renderer.cpp` : 4 lignes ;
    - `renderer.h` : 5 lignes ;
    - `desktop_main.cpp` : 2 lignes ;
    - `CMakeLists.txt` : 2 lignes.
  - Banc `finitions`, test `test_fidelite_voitures_finitions`, valeurs sourcées dans `references/reel/voitures/peintures.md`.
- **Comment c'est vérifié** :
  - **Brillant identique au pixel près** (RG-01) : banc `peinture2` avant / après, 14 images, 0 pixel différent.
  - **Chromé homogène** : luminance moyenne de la carrosserie sur les planches de jour 0,41 (GT86), 0,44 (Corvette), 0,42 (208) ; avant correction : 0,41, 0,13 et 0,13.
  - Planches des 8 finitions (3 voitures, jour et nuit) et **gros plans des 8 finitions** (GT86, satiné et mat compris) regardés image par image ; de nuit, aucune tache blanche.
  - Caméléon sous 3 angles ; 208 en course en chromé et en mat ; **208 avec toit et rétros Noir (couleur 2)** en métallisé et en chromé (RG-11).
  - Tests CPU : section ci-dessous.
  - Performances : tableau ci-dessous.
- **Limites et risques connus** :
  - Le choix au garage et la sauvegarde relèvent de WFS-491 : rien ne change sur l'APK avant ce ticket.
  - Caméléon : la couleur suit la courbure ; sur le capot très creusé de la Corvette, les changements de teinte sont rapprochés.
  - Constat hors ticket : en **brillant**, les reflets des caisses très denses sont aussi atténués (`specK`, même cause que WFS-460). Je ne le corrige pas ici (RG-01 : brillant identique). Je le signale au PO.
  - La mesure des img/s est faite sur lavapipe (processeur), pas sur téléphone.

## Tests CPU

Commit testé : `c48dc1e6` (branche synchronisée avec `main`). Journal complet : [`qa/pr/WFS-490/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/tests-cpu.log).

```
[OK] test_fidelite_voitures_finitions
tests CPU : 123 test(s) réussi(s) en 376 s
```

## Performances en Basse (RG-10, RC-E18-03)

Banc `peinture2-mesure` : 208, Ville, 600 images, lavapipe, 3 passages de chaque côté.

| | Passage 1 | Passage 2 | Passage 3 | Moyenne | 1 % les plus lents (moyenne) |
|---|---|---|---|---|---|
| main | 3,734 | 3,762 | 3,485 | **3,660 img/s** | 2,591 img/s |
| branche, brillant | 3,701 | 3,827 | 3,754 | **3,761 img/s** | 2,710 img/s |
| branche, métallisé | 3,710 | 3,662 | 3,235 | **3,536 img/s** | 2,457 img/s |

Écart avec `main` : +0,10 img/s en brillant, −0,12 img/s en métallisé (1 % les plus lents : +0,12 et −0,13), dans la limite de ≤ 1 img/s. L'écart d'un passage à l'autre sur `main` atteint 0,28 img/s.

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
