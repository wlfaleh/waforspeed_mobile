Agent : arwa-4 (Claude, session 01Us6uCw)

**Ticket** : WFS-288 — règles : `regles-de-gestion/WFS-288.md`

## Demande
WFS-288 (PO, axe Réalisme, domaine son). Il pleut en Ville la nuit, mais on n'entend que le chuintement des pneus, et en vue Cockpit les essuie-glaces balaient sans un bruit. Le but : entendre la pluie comme dans une vraie voiture, sans rien changer quand il ne pleut pas.

## Ce qui change
- **Nouveau module `src/audio/pluie_sons.*`**, branché dans `src/audio/audio_system.cpp`.
  - Il est actif quand `meteo::pluieActive` est vrai (même expression que `sfx.cpp`), sur l'écran Course seulement. Il est coupé dans les menus, en pause et sur les scènes dédiées.
  - Fondus de 1 s à l'entrée et de 0,4 s à la sortie.
  - La pluie est ajoutée à gauche et à droite après les sources des lieux. Elle n'entre pas dans l'écho des tunnels et des murs.
- **RG-01, pluie de référence de 5 mm/h**, sourcée (détails dans `references/reel/son/pluie.md`) :
  - lois : Marshall et Palmer, Atlas et coll. 1973 ;
  - 2 104 impacts par m² et par seconde, dont 2,65 % de grosses gouttes ;
  - sur le pare-brise de la Golf (33,2°), 1 760, 6 585 et 14 305 impacts par m² et par seconde à 0, 50 et 130 km/h.
- **Vue extérieure** :
  - averse stéréo calculée en direct : deux bruits indépendants, sans boucle ni motif qui revient, maximum entre 1 et 8 kHz ;
  - gouttes isolées : 56 par seconde ;
  - bulles des flaques (Minnaert, 13 à 16 kHz) : 58 par seconde, en petite part du mélange.
- **Vue Cockpit** :
  - averse étouffée : passe-bas 1 kHz et −20 dB [à vérifier] ;
  - tambourinage du toit (4 145 impacts par seconde) et du pare-brise (1 743 à 14 160 par seconde selon la vitesse) ;
  - les gouttes sont tirées à la cadence de RG-01, avec leur taille et leur vitesse.
- **Essuie-glaces** :
  - même modèle que l'image : `cockpit::EtatEssuie`, avancé à chaque pas avec les mêmes entrées que `camera_vue.cpp` ;
  - frottement selon la vitesse du balai, choc sourd en butée, choc plus léger en haut de la course, bourdonnement du moteur ;
  - le thread audio recale sa phase sur le modèle à chaque pas.
- **Tunnel** : averse, gouttes et tambourinage coupés en 0,3 s, puis repris en 0,5 s. Les essuie-glaces continuent.
- **Aucun fichier son ajouté.** Aucune allocation ni verrou dans le thread audio. Coût mesuré : 0,025 ms par bloc de 10 ms en vue Cockpit à 130 km/h (plafond 0,1).
- **Zone** :
  - fichiers neufs : `src/audio/pluie_*` et `tests/test_audio_pluie.cpp` ;
  - branchement dans `audio_system.cpp` ;
  - `CMakeLists.txt` : 1 ligne, pour le test ;
  - lecture seule de `meteo.h`, `vehicule_cockpit.h` et `camera_vue.h`.
- **WFS-282** a retiré `PENTE_PARE_BRISE` de `vehicule_cockpit.h`. La pente de la Golf (0,655) est donc gardée dans `pluie_sons.h`, avec son origine.

## Comment c'est vérifié
- `test_audio_pluie`, nouveau, est vert. Il compte 16 cas :
  - RG-01 : calculs à ±1 %, refaits autrement ;
  - RG-02 : silence hors pluie et hors écran Course, fondus de 1,00 s et 0,40 s ;
  - RG-03 : corrélation gauche / droite de 0,12, autocorrélation de 0,02 au plus entre 0,05 et 2 s, maximum à 2-4 kHz, cadence des gouttes et des bulles ;
  - RG-04 : rapports de cadence du pare-brise 1 / 3,73 / 8,10 pour 1 / 3,74 / 8,13 attendus (±2 %), aigus étouffés de 19,7 dB ;
  - RG-05 : phase du son égale à la phase de l'image après 10 min de pas (écart 0,000000), 2 chocs par cycle ;
  - RG-06 : coupure en 0,31 s, reprise en 0,51 s ;
  - RG-07 : crête de la pluie 9,0 dB sous celle du moteur à mi-régime ; chuintement des pneus 3,7 dB au-dessus de l'averse dans sa bande à 50 km/h ;
  - RG-08 : sortie identique à l'échantillon près sans pluie ;
  - RG-09 : coût.
- **64 tests CPU** au vert en local, après synchronisation avec `main` (5ff3afb, puis aa70da6).
- **Banc audio** (`benchPluie`, groupe `partie2a`) : `pluie-arriere`, `pluie-cockpit` et `pluie-tunnel`, en .wav et .png (niveaux gauche / droite, spectre au tiers d'octave).
- **Captures avant / après** (`tools/qa/captures_pr.sh WFS-288 partie2a`, filtre sur les courbes audio) : les 24 PNG audio qui existaient déjà (ambiance, lieux, masques, moteur, musique, parois, voitures) sont **identiques octet pour octet** à `main`, ainsi que les .wav comparés (moteur, parois-ville, masque-ville, voitures-course5). Les 3 `pluie-*` sont nouvelles ; elles sont déjà dans cette passe, avec « — » comme avant, donc la seconde passe `SANS_AVANT=1` n'apporte rien.
- **Parcours de nuit** (`WFS_MOMENT=nuit WFS_BANC=parcours`, lancé sur 91e0591, avant la dernière synchronisation qui n'a apporté que le code des autres tickets), qui passe par la Ville sous la pluie dans les deux vues :
  - « parcours complet : 0 écart(s) OK » ;
  - « parcours complet en vue Cockpit (WFS-197) : 0 écart(s) OK » ;
  - journal du banc : PLUIE vue extérieure, puis vue Cockpit.

## Limites et risques
- Valeurs sans source lisible sur GitHub, marquées [à vérifier] dans le code et dans `pluie.md` :
  - isolation de l'habitacle ;
  - résonances du toit (450 Hz) et du pare-brise (1,5 kHz) ;
  - sons des essuie-glaces ;
  - surfaces de toit et de pare-brise, déduites des proportions de la Golf ;
  - plage de gouttes qui laissent une bulle (Pumphrey et Crum).
- Les niveaux sont une décision : le niveau absolu n'est pas mesurable, il dépend du téléphone.
- Rien n'a été écouté sur un vrai téléphone. Au-dessus de 10 kHz (bulles), le haut-parleur rend mal.
- Le son de la pluie ne change aucune image. Les captures sont donc les courbes du banc audio : les PNG audio existants sont identiques à `main`, et les `pluie-*` sont nouvelles.

## Captures pour le lead dev (27 après)

Dossier : [`qa/pr/WFS-288`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `ambiance-campagne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-ambiance-campagne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-ambiance-campagne.jpg?raw=true) |
| `ambiance-circuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-ambiance-circuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-ambiance-circuit.jpg?raw=true) |
| `ambiance-plage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-ambiance-plage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-ambiance-plage.jpg?raw=true) |
| `ambiance-ville` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-ambiance-ville.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-ambiance-ville.jpg?raw=true) |
| `lieu-ferme` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-lieu-ferme.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-lieu-ferme.jpg?raw=true) |
| `lieu-fontaine` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-lieu-fontaine.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-lieu-fontaine.jpg?raw=true) |
| `lieu-terrasse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-lieu-terrasse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-lieu-terrasse.jpg?raw=true) |
| `lieu-tribune` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-lieu-tribune.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-lieu-tribune.jpg?raw=true) |
| `lieu-tunnel` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-lieu-tunnel.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-lieu-tunnel.jpg?raw=true) |
| `lieu-vagues` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-lieu-vagues.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-lieu-vagues.jpg?raw=true) |
| `masque-campagne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-masque-campagne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-masque-campagne.jpg?raw=true) |
| `masque-corniche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-masque-corniche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-masque-corniche.jpg?raw=true) |
| `masque-stands` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-masque-stands.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-masque-stands.jpg?raw=true) |
| `masque-ville` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-masque-ville.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-masque-ville.jpg?raw=true) |
| `moteur-courbe` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-moteur-courbe.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-moteur-courbe.jpg?raw=true) |
| `musique-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-musique-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-musique-course.jpg?raw=true) |
| `musique-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-musique-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-musique-menu.jpg?raw=true) |
| `parois-casino` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-parois-casino.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-parois-casino.jpg?raw=true) |
| `parois-falaise` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-parois-falaise.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-parois-falaise.jpg?raw=true) |
| `parois-stands` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-parois-stands.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-parois-stands.jpg?raw=true) |
| `parois-ville` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-parois-ville.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-parois-ville.jpg?raw=true) |
| `pluie-arriere` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-pluie-arriere.jpg?raw=true) |
| `pluie-cockpit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-pluie-cockpit.jpg?raw=true) |
| `pluie-tunnel` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-pluie-tunnel.jpg?raw=true) |
| `voitures-course5` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-voitures-course5.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-voitures-course5.jpg?raw=true) |
| `voitures-police` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-voitures-police.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-voitures-police.jpg?raw=true) |
| `voitures-trafic` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/avant-voitures-trafic.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-288/qa/pr/WFS-288/apres-voitures-trafic.jpg?raw=true) |
