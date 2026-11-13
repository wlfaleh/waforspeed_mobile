Agent : Claude, agent dev3d (01Us6uCwDY)

**Ticket** : WFS-490 — règles : `regles-de-gestion/WFS-490.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : le joueur choisit la finition de sa peinture (brillant, métallisé, nacré, mat, satiné, chromé, caméléon, candy) et chacune doit ressembler à la vraie peinture automobile du même nom, de jour comme de nuit (épic E18, RG-01 à RG-12 de `regles-de-gestion/WFS-490.md`).
- **Ce qui change** :
  - `shaders/vehicule.glsl` : `finitionSurface` (couleur, métal, rugosité selon la finition : flop du métallisé, mica du nacré, mat, satiné, chrome teinté F0 0,60, caméléon qui tourne la teinte avec l'angle de vue, candy Beer-Lambert sur base argent), `vernisFinition` (paillettes, vernis satiné flou, pas de vernis pour mat et chromé, miroir du chrome), `vernisCouche` (ancien `vernisPeinture` avec la rugosité, la force et la couleur de nuit en paramètres ; `vernisPeinture` l'appelle avec les valeurs d'avant). Paillettes **procédurales** dans le repère de la voiture (taille de cellule ≥ 1 pixel, 2 tailles mêlées en fondu, éteintes au loin et en Basse) : **aucune texture ajoutée** (RG-10 permettait 64 Ko, pas utilisés).
  - Transport sans constante de poussée en plus : finition codée dans le métal des pièces peintes (`pc.params.x = métal + 2 × finition`, `src/render/vehicule_finitions.h`) ; brillant = valeur d'origine intacte. Seule la voiture du joueur (`dessinJoueur`) la reçoit : trafic, adversaires, police, fantôme restent brillants (RG-11) ; la couleur 2 (toit, rétros, WFS-488) suit la même finition.
  - API pour WFS-491 : `Renderer::finitionJoueur(int)` (gardée d'une image à l'autre), identifiants `vehicule_finitions::IDS` ; bancs : `WFS_FINITION=<id>`.
  - Accroches hors zone, annoncées au journal : `shaders/scene.frag` (3 lignes), `src/render/renderer.cpp` (4 lignes), `renderer.h` (5 lignes), `desktop_main.cpp` (2 lignes), `CMakeLists.txt` (2 lignes).
  - Nouveau banc `finitions` (`src/render/vehicule_finitions_banc.cpp`), nouveau test `test_fidelite_voitures_finitions`, valeurs sourcées dans `references/reel/voitures/peintures.md`.
- **Comment c'est vérifié** :
  - **Brillant identique au pixel près** (RG-01) : banc `peinture2` (GT86, Corvette, 208 ; vitrine, gros plans, planche, course jour et nuit) avant (main) / après (branche) : **14 images, 0 pixel différent**.
  - `test_fidelite_voitures_finitions` vert (codage aller-retour des 8 finitions pour 1001 valeurs de métal, brillant au bit près, vernis 0,03 gardé, chrome 0,55-0,65 et rugosité 0,05-0,15, ordre mat > satiné > brillant, `SPEC_MAX` gardé, aucun échantillonneur ajouté) ; **tous les tests CPU** (`tools/qa/tests_cpu.sh build`) : **122 réussis**, 0 échec.
  - Banc `finitions` regardé image par image (planches jour et nuit des 3 voitures, gros plans, caméléon sous 3 angles, 208 en course chromé et mat) ; nuit : aucune tache blanche.
  - Endurance en Basse (banc `peinture2-mesure`, 208, Ville, 600 images, lavapipe) : main 3,540 et 3,767 img/s (deux passages : bruit ± 0,23) ; branche brillant 3,326, métallisé 3,307, candy 3,495 img/s : écarts < 0,5 img/s (RG-10, ≤ 1 img/s), dans le bruit du banc.
- **Limites et risques connus** :
  - Le choix de la finition au garage et sa sauvegarde sont **WFS-491** (hors périmètre) : sur l'APK, rien ne change à l'écran tant que WFS-491 n'est pas fusionné (le joueur reste en brillant).
  - Chromé teinté : un miroir reflète ce qui l'entoure ; dans une rue à l'ombre (course) il paraît sombre, en vitrine il est clair. Teinte limitée à 45 % de la couleur (au-delà, un chrome rouge sous un ciel bleu est presque noir).
  - Caméléon : la couleur suit la courbure de la carrosserie (normale lissée du maillage) ; sur les formes très creusées (capot de la Corvette) les passages de teinte sont rapprochés.
  - Mesure des img/s faite sur lavapipe (processeur), pas sur téléphone ; valeurs du réel citées d'après leurs références publiques (aucun téléchargement).
  - RC-E18 : aucun logo ni livrée ajoutés ; aucune sauvegarde touchée (RC-E18-04 sans objet ici) ; captures en course de nuit : banc `peinture2-course-nuit` (brillant).

## Captures pour le lead dev (28 après)

Dossier : [`qa/pr/WFS-490`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `finitions-gt86-candy-pres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-gt86-candy-pres.jpg?raw=true) |
| `finitions-gt86-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-gt86-jour.jpg?raw=true) |
| `finitions-gt86-metallise-pres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-gt86-metallise-pres.jpg?raw=true) |
| `finitions-gt86-nacre-pres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-gt86-nacre-pres.jpg?raw=true) |
| `finitions-gt86-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-gt86-nuit.jpg?raw=true) |
| `finitions-mustang-cameleon-1` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-mustang-cameleon-1.jpg?raw=true) |
| `finitions-mustang-cameleon-2` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-mustang-cameleon-2.jpg?raw=true) |
| `finitions-mustang-cameleon-3` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-mustang-cameleon-3.jpg?raw=true) |
| `finitions-mustang-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-mustang-jour.jpg?raw=true) |
| `finitions-mustang-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-mustang-nuit.jpg?raw=true) |
| `finitions-p208-course-chrome` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-p208-course-chrome.jpg?raw=true) |
| `finitions-p208-course-mat` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-p208-course-mat.jpg?raw=true) |
| `finitions-p208-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-p208-jour.jpg?raw=true) |
| `finitions-p208-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-490/qa/pr/WFS-490/apres-finitions-p208-nuit.jpg?raw=true) |
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
