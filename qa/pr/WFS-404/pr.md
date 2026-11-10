Agent : Claude, agent devartiste (01UY1Vv2ReCX)

**Ticket** : WFS-404 — règles : `regles-de-gestion/WFS-404.md` (RG-00 à RG-11)

## Description

- **Demande de l'utilisateur** : ne garder dans l'application que les voitures qu'on peut utiliser pour gagner de l'argent. L'A 45 et l'Elantra N (CC BY-NC) et le X5 (CC BY-NC-SA) sortent de l'APK ; leurs fichiers restent dans le dépôt ; les autres voitures sont sans marque.
- **Ce qui change** :
  - **Liste unique** `VOITURES_RETIREES` dans `src/game/cars.h` (RG-02), avec `voitureRetiree()`, `voituresPermises()` et `voiturePermise()`. Elle est lue par le garage, la Course à 5, le duel, la sauvegarde, les bancs, les tests, les crédits (`tools/qa/verifier_licences.py`), le contrôle des logos (`tools/effacer_logos.mjs`) et la copie des actifs de l'APK (`android/app/build.gradle` relit la ligne). Pour faire revenir une voiture, il suffit d'enlever son id de la liste.
  - **Garage** : il propose **17 voitures** (« 02 / 17 », 17 points). `CARS` en compte 20, et non 21 (RG-10).
  - **APK** : `.wfm` et `.lod` des 3 voitures écartés par `ignoreAssetsPattern` (motifs d'Android par défaut gardés). Contrôle à la construction : `merge*Assets` s'arrête en erreur si l'un d'eux est encore là, et écrit « WFS-404 : voitures retirées hors de l'APK » dans le journal de Gradle.
  - **Sauvegarde** (RG-04) : `Game::setCar` prend `voiturePermise()` (1 ligne d'accroche dans `game.cpp`). Une voiture retirée ou inconnue donne la Golf ; la sauvegarde, les records et la progression ne sont pas touchés.
  - **Usages remplacés** (RG-03) :
    - camion de la Poursuite : X5 → Range Rover Sport (Compass si le joueur roule en RR Sport) ;
    - sens inverse de la Ville : X5 → Compass ;
    - bancs : X5 → RR Sport ;
    - listes `COCKPIT_LOT*` : sans les 3 voitures ;
    - duel : voiture retirée reçue → Golf.
  - **Course à 5** (RG-09, RG-11, décisions du PO) :
    - les candidats ne changent pas ;
    - une voiture retirée, ou le RR Sport (écarté jusqu'à WFS-394), qui sort du tirage est remplacée par la voiture possible la plus proche en vitesse max ;
    - les courses sans ces voitures restent identiques à `main`.
  - **Crédits** (RG-05) : `store/credits.md` et `assets/licences/CREDITS.txt` régénérés avec 17 voitures ; la phrase « Jeu gratuit (licences non commerciales) » est enlevée. Le vérificateur refuse toute voiture livrée sous licence NC ou ND.
  - **M4** (RG-06, ANO-209) : bandes bleu clair, bleu foncé et rouge du volant enlevées (1 ligne de `vehicule_cockpit_defs.cpp`).
  - **Contrôle des logos** (RG-06) : `effacer_logos.mjs --verifier` saute les voitures retirées. Le RR Sport et la S3, retouchés par les formes après les logos (WFS-362, WFS-363), sont reconnus à l'empreinte exacte de l'état final des formes : le contrôle est vert.
  - **Nouveau banc** `voitures-retirees` : planche des voitures permises et Poursuite.
- **Comment c'est vérifié** :
  - `tools/qa/tests_cpu.sh build` : tous les tests passent, sauf le rouge connu ci-dessous. Nouveau test `test_voitures_retirees` :
    - 17 voitures permises, aucune sous licence NC ou ND ;
    - sauvegarde x5 / a45 / elantra → Golf ;
    - 13 600 adversaires tirés sans voiture retirée ni RR Sport ;
    - camion et duel ;
    - listes du cockpit.
  - Autres contrôles :
    - `python3 tools/qa/verifier_licences.py` : 0 manque ;
    - `node tools/effacer_logos.mjs --verifier` : vert ;
    - la lecture de la liste par Gradle a été essayée seule (`[x5, a45, elantra]` et les motifs).
- **Limites et risques connus** :
  - **Rouge connu, accepté par le PO (RG-11)** : `test_course5_ordinateurs`, « circuit : aide activée, dans les 3 premiers ≥ 4/8 », mesuré à 3/8 (6/8 sur `main`). Sans le X5, l'A 45 et l'Elantra (adversaires plus faciles), les remplaçants sont plus forts. Aucun seuil n'est changé. Le correctif est confié à WFS-394 (P1). Les autres contrôles de ce test sont verts (ville 5/8, campagne 5/8, plage 4/8, 1 victoire sans aide par carte).
  - `test_course5`, tableau RG-02 / RG-06 : X5 → T-Cross (RG-09). Le RR Sport, la S3 et la V60 échouaient sur ses seuils.
  - L'APK n'a pas pu être construit ici (pas de SDK Android) : le `unzip -l` se fera sur l'APK de la CI de `main` (contrôle de Gradle à la construction).
  - Les voitures en sens inverse de la Ville sont trop loin sur la capture de l'autoroute pour qu'on les voie. Le Compass utilisé est un modèle déjà validé (planche, garage).
  - La capture du camion est de nuit : c'est le banc de la Poursuite tel qu'il existe.
  - Le contrôle des marques (« même bêtement ») se fait sur la planche et les vitrines : aucune marque visible n'a été trouvée.

## Captures pour le lead dev

Dossier : [`qa/pr/WFS-404`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404) — produit par `tools/qa/captures_pr.sh` ; seules les captures du ticket sont gardées.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `vitrines-planche` | — (banc nouveau) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/apres-vitrines-planche.jpg?raw=true) |
| `garage-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/avant-garage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/apres-garage-0.jpg?raw=true) |
| `garage-m4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/avant-garage-m4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/apres-garage-m4.jpg?raw=true) |
| `garage-suivant` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/avant-garage-suivant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/apres-garage-suivant.jpg?raw=true) |
| `garage-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/avant-garage-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/apres-garage-1.jpg?raw=true) |
| `garage-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/avant-garage-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/apres-garage-2.jpg?raw=true) |
| `showroom-suivante-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/avant-showroom-suivante-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/apres-showroom-suivante-1.jpg?raw=true) |
| `showroom-suivante-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/avant-showroom-suivante-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/apres-showroom-suivante-2.jpg?raw=true) |
| `showroom-suivante-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/avant-showroom-suivante-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/apres-showroom-suivante-3.jpg?raw=true) |
| `poursuite-camion` | — (banc nouveau) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/apres-poursuite-camion.jpg?raw=true) |
| `carte-ville-autoroute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/avant-carte-ville-autoroute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/apres-carte-ville-autoroute.jpg?raw=true) |
| `carte-ville-autoroute-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/avant-carte-ville-autoroute-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/apres-carte-ville-autoroute-nuit.jpg?raw=true) |
| `cockpit-m4-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/avant-cockpit-m4-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/apres-cockpit-m4-jour.jpg?raw=true) |
| `cockpit-m4-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/avant-cockpit-m4-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/apres-cockpit-m4-nuit.jpg?raw=true) |
| `cockpit-m4-braquage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/avant-cockpit-m4-braquage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-404/qa/pr/WFS-404/apres-cockpit-m4-braquage.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv2ReCXUB7JSys1VPc7
