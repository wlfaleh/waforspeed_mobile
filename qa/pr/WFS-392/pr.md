## Agent : Claude, agent cartes (01UY1Vv2ghpU)

**Ticket** : WFS-392 — règles : `regles-de-gestion/WFS-392.md`

## Description

- **Demande** (revue du lead dev de la PR #315, tâche séparée sur ordre de l'utilisateur, ANO-230) : sur une carte de données enneigée, la chaussée doit être sèche sous **tous** les tunnels (seul le 1er l'était), et le tunnel générique doit être **éclairé** par de vrais luminaires.
- **Ce qui change à l'écran** :
  1. **Chaussée et trottoir secs sous chaque tunnel** de la carte, avec le même fondu de neige sur 8 m à chaque bouche (avant : le 2e tunnel était enneigé dedans).
  2. **Deux files de luminaires** sous la dalle, au milieu de chaque demi-chaussée : corps en profilé, embouts, cadre, vitre allumée, étriers, grenouillères, presse-étoupe, câble, chemin de câbles continu pendu par des tiges filetées (aucune forme nue ; ≈ 230 triangles de près, 26 de loin). Un luminaire tous les 12 m, tous les 6 m sur les 60 premiers mètres de chaque bouche (zone de seuil, bornée au tiers du tunnel).
  3. **La nuit, le tunnel est éclairé** : un luminaire sur deux (files alternées) est inscrit au registre des lampadaires du moteur ; les 4 plus proches éclairent vraiment (lumière sodium du moteur), les autres ont leur vitre allumée. Aucune lumière ajoutée au moteur, aucun disque de halo.
  4. **Sol sec entre la chaussée et les piédroits** (trottoir de service en béton) : il restait couvert de la neige des bas-côtés.
  5. **Plus de lampadaire de rue sous un tunnel** : les mâts de 7,6 m de la carte d'essai traversaient la dalle.
  6. **Dalle du tunnel générique à 7,6 m** au lieu de 5,6 m (même hauteur que les tunnels de la Ville et de la Corniche) : à 5,6 m, la caméra de la vue Arrière (6,3 m de haut) passait **au-dessus** de la dalle et ne montrait que le dessus du tunnel (voir « avant » de `neige-tunnel2-arriere-*`).
- **Tout se lit du paquet (RG-03)** : tunnels = sections `tunnel` du tracé ; nouvelle ligne du bloc decor `tunnel_eclairage sodium|led|aucun pas_m pas_seuil_m seuil_m` (défaut sans la ligne : sodium, 12 m, 6 m, 60 m ; valeurs hors limites refusées). Format des blocs 2 → 3 (un jeu plus ancien refuse proprement un bloc 3). Rien de propre à une carte dans le code.
- **Fichiers** : `shaders/route.glsl` (`neigeRoute`, `abriRoute` seulement), `src/render/route_revetements{.h,.cpp,_defs.cpp}` (codage des abris), `src/world/cartes_v2/carte_donnees{.h,.cpp}` (lecture), `carte_donnees_decor.cpp` (décor), `tunnel_luminaire.{h,cpp}` (nouveau : modèle), `neige_banc.cpp` (captures, mesure), `paquets/sources/noel-test/{trace,decor}.txt` (2e tunnel, ligne d'éclairage), `tests/test_cartes_v2_abris.cpp` (nouveau), `tests/test_cartes_v2_{donnees,saison}.cpp` (7 sections, format 4 refusé), `CMakeLists.txt` (une ligne : le test), `references/reel/cartes_v2/tunnel.md` (valeurs réelles et sources).
- **Comment c'est vérifié** :
  - compilation complète ; `bash tools/qa/tests_cpu.sh build` : **111 tests réussis** (dont le nouveau `test_cartes_v2_abris` : sec au milieu de chaque tunnel, neige dehors, fondu aux bouches, tours suivants, tunnel de 3 km sans bande de neige, tunnels en trop comptés, code du matériau inchangé sans tunnel, lecture et refus de `tunnel_eclairage`, place des luminaires) ;
  - banc `WFS_NEIGE=tunnels WFS_BANC=neige` : 0 écart, 7 captures ci-dessous, toutes regardées ;
  - **RG-04 (coût en Basse)** : `WFS_NEIGE=cout`, traversée du 2e tunnel en qualité Basse, 413 images, 2 passages en alternance main / branche, passe de scène en fin de traversée : **jour 294,2 et 291,6 ms sur main, 271,1 et 273,2 ms sur la branche ; nuit 265,9 et 261,4 ms sur main, 254,2 et 251,4 ms sur la branche**. Pas de surcoût mesuré (bruit ≈ ± 3 %). Mesure en **rendu logiciel sur PC** (lavapipe) : ce n'est pas un GPU de téléphone.
- **Limites et risques connus** :
  - **3 tunnels au plus par carte** restent secs (le matériau de la route n'a que 3 nombres libres ; un tunnel de plus de 1,2 km en prend 2) ; au-delà : une ligne au journal du jeu, le tunnel est enneigé dedans, pas de plantage. Lever la limite demande une donnée par sommet de la route (noyau, E0) : signalé au PO.
  - **De jour**, le tunnel reste sombre : l'exposition automatique ne descend pas sous EV 10 (`eclairage_unites.h`, E13, hors zone) et aucune lumière locale n'existe de jour dans le moteur. Les vitres sont allumées et se lisent, mais elles n'éclairent pas la chaussée de jour ; le « renforcement à l'entrée de jour » de RG-02 est donc fait par l'espacement (6 m au seuil), pas par un niveau de lumière. Avis de l'expert-realisme-3d : ne pas compenser en gonflant l'émissif ; signalé au PO.
  - **Sources de l'espacement** (CIE 88:2004, dossier pilote du CETU) : citées **de mémoire**, le réseau des agents étant limité à GitHub ; valeurs marquées « à confirmer » dans `references/reel/cartes_v2/tunnel.md`. Les cotes du luminaire ne sont pas vérifiées à ± 1 % sur une fiche de fabricant.
  - Chaussée mouillée (pluie) sous un tunnel : non traitée, il ne pleut sur aucune carte de données aujourd'hui.
  - Parcours complet accueil → choix du terrain (neige) → course → retour : **non rejoué à la main** ; le banc charge la carte depuis son paquet, court et quitte 8 fois sans erreur. La carte de neige est une carte d'essai cachée aux joueurs.
  - Les bandes claires et sombres de la dalle (recouvrement des tronçons de WFS-346) existaient avant et ne sont pas touchées.
  - Hors épic E15 : avis de l'agent 3D non exigé ; il a été consulté pour le modèle et l'espacement.

## Captures pour le lead dev (7 après)

Dossier : [`qa/pr/WFS-392`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-392/qa/pr/WFS-392) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `neige-tunnel-entree-arriere` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-392/qa/pr/WFS-392/avant-neige-tunnel-entree-arriere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-392/qa/pr/WFS-392/apres-neige-tunnel-entree-arriere.jpg?raw=true) |
| `neige-tunnel2-arriere-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-392/qa/pr/WFS-392/avant-neige-tunnel2-arriere-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-392/qa/pr/WFS-392/apres-neige-tunnel2-arriere-jour.jpg?raw=true) |
| `neige-tunnel2-arriere-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-392/qa/pr/WFS-392/avant-neige-tunnel2-arriere-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-392/qa/pr/WFS-392/apres-neige-tunnel2-arriere-nuit.jpg?raw=true) |
| `neige-tunnel2-cockpit-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-392/qa/pr/WFS-392/avant-neige-tunnel2-cockpit-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-392/qa/pr/WFS-392/apres-neige-tunnel2-cockpit-jour.jpg?raw=true) |
| `neige-tunnel2-cockpit-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-392/qa/pr/WFS-392/avant-neige-tunnel2-cockpit-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-392/qa/pr/WFS-392/apres-neige-tunnel2-cockpit-nuit.jpg?raw=true) |
| `neige-tunnel2-luminaire-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-392/qa/pr/WFS-392/apres-neige-tunnel2-luminaire-jour.jpg?raw=true) |
| `neige-tunnel2-luminaire-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-392/qa/pr/WFS-392/apres-neige-tunnel2-luminaire-nuit.jpg?raw=true) |
