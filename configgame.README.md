# configgame.json — réglages du jeu (WFS-128)

Le fichier `configgame.json` (racine du dépôt) règle le jeu **sans toucher au code**. Il est copié dans l'APK.

- **Sur le téléphone, sans reconstruire** : poser un `configgame.json` modifié dans `Android/data/com.waforspeed.game/files/` (câble USB ou gestionnaire de fichiers), puis relancer le jeu : il remplace celui de l'APK. Le retirer = retour aux valeurs de l'APK.
- **Sur PC (banc d'essai)** : `./configgame.json`, ou le fichier donné par la variable `WFS_CONFIG` ; relu à chaque partie.
- Les nombres s'écrivent avec un **point** (`0.5`, jamais `0,5`), comme en JSON.
- Une clé absente, un mauvais type ou une valeur hors bornes : le jeu garde sa valeur et l'écrit au journal (`configuration : …` dans logcat). Un fichier cassé : toutes les valeurs du jeu. Le jeu ne plante jamais à cause du fichier.
- Les clés qui commencent par `_` (comme `_aide`) sont des commentaires.
- Ce README et le fichier sont fabriqués par `./build/test_configgame --ecrire` : ajouter une clé = une ligne dans `src/core/configgame.cpp` (table `champs`), puis relancer cette commande.

| Clé | Sens | Unité | Min | Max | Défaut |
|---|---|---|---|---|---|
| `score.points_par_metre` | points gagnés par mètre parcouru | points | 0 | 100 | 5 |
| `score.quasi_contact` | points d'un quasi-contact, multipliés par le combo | points | 0 | 10000 | 250 |
| `score.combo_duree` | le combo continue si le quasi-contact suivant arrive avant ce délai | s | 0.5 | 10.0 | 3.0 |
| `score.choc_perd_combo` | un petit choc plus fort que ça fait perdre le combo | impact (0 à 1) | 0.0 | 1.0 | 0.08 |
| `checkpoint.depart` | chrono au départ de la course Checkpoint | s | 10.0 | 600.0 | 60.0 |
| `checkpoint.bonus` | secondes ajoutées à chaque checkpoint | s | 0.0 | 300.0 | 30.0 |
| `checkpoint.distance` | distance entre deux checkpoints | m | 100.0 | 5000.0 | 600.0 |
| `nitro.depart` | réserve de nitro au départ | réserve (0 à 1) | 0.0 | 1.0 | 0.4 |
| `nitro.poussee` | accélération multipliée pendant la nitro | × | 1.0 | 4.0 | 1.6 |
| `course5.ordinateurs` | voitures pilotées par l'ordinateur (course à 5 : 4) | voitures | 1 | 4 | 4 |
| `course5.vitesse_facile` | vitesse visée par les ordinateurs, difficulté facile | × vitesse de pointe | 0.3 | 1.2 | 0.8 |
| `course5.vitesse_normale` | vitesse visée par les ordinateurs, difficulté normale | × vitesse de pointe | 0.3 | 1.2 | 0.9 |
| `course5.vitesse_difficile` | vitesse visée par les ordinateurs, difficulté difficile | × vitesse de pointe | 0.3 | 1.2 | 0.97 |
| `course5.ecart_min` | les ordinateurs ne s'approchent jamais plus de la voiture de devant | m | 0.2 | 10.0 | 1.2 |
| `course5.rattrapage_derriere` | ordinateur plus loin que ça derrière le joueur : il accélère (WFS-133) | m | 0.0 | 5000.0 | 150.0 |
| `course5.rattrapage_devant` | ordinateur plus loin que ça devant le joueur : il ralentit | m | 0.0 | 5000.0 | 250.0 |
| `course5.rattrapage_bonus` | vitesse visée en plus d'un ordinateur trop loin derrière | part de la vitesse | 0.0 | 0.5 | 0.08 |
| `course5.rattrapage_malus` | vitesse visée en moins d'un ordinateur trop loin devant | part de la vitesse | 0.0 | 0.5 | 0.06 |
| `course5.rattrapage_malus_facile` | même chose en difficulté facile | part de la vitesse | 0.0 | 0.5 | 0.1 |
| `course5.aspiration_distance` | aspiration à moins de cette distance derrière une voiture | m | 0.0 | 100.0 | 25.0 |
| `course5.aspiration_bonus` | vitesse visée en plus dans l'aspiration | part de la vitesse | 0.0 | 0.3 | 0.04 |
| `course5.nitro_bonus` | vitesse en plus pendant la nitro d'un ordinateur | part de la vitesse | 0.0 | 0.5 | 0.15 |
| `course5.nitro_duree` | durée de la nitro d'un ordinateur | s | 0.0 | 10.0 | 2.5 |
| `course5.nitro_fin` | nitro permise avant l'arrivée à moins de cette distance | m | 0.0 | 5000.0 | 500.0 |
| `poursuite.score_depart` | score au départ de la poursuite | points | 1 | 1000 | 100 |
| `poursuite.distance` | distance à parcourir pour gagner | km | 1.0 | 50.0 | 10.0 |
| `poursuite.choc_trafic` | points perdus sur un choc avec le trafic | points | 0 | 100 | 5 |
| `poursuite.choc_glissiere` | points perdus sur un choc contre la glissière | points | 0 | 100 | 3 |
| `poursuite.choc_police` | points perdus sur un choc avec la police | points | 0 | 100 | 10 |
| `poursuite.colle_distance` | police plus près que ça : un point perdu par période | m | 1.0 | 50.0 | 12.0 |
| `poursuite.colle_periode` | période de la perte d'un point quand la police colle | s | 0.2 | 10.0 | 1.0 |
| `poursuite.police_depart` | voitures de police au départ | voitures | 1 | 6 | 2 |
| `poursuite.police_max` | voitures de police au plus | voitures | 1 | 6 | 6 |
| `poursuite.renfort` | une voiture de police de plus tous les … km | km | 0.5 | 10.0 | 2.0 |
| `poursuite.densite_trafic` | trafic multiplié pendant la poursuite | × | 1.0 | 4.0 | 2.5 |
| `camera.poursuite.recul` | vue Poursuite : distance derrière la voiture | m | 2.0 | 30.0 | 9.0 |
| `camera.poursuite.hauteur` | vue Poursuite : hauteur de la caméra | m | 0.5 | 20.0 | 6.3 |
| `camera.poursuite.regard_devant` | vue Poursuite : point visé devant la voiture | m | 0.0 | 40.0 | 14.0 |
| `camera.poursuite.regard_hauteur` | vue Poursuite : hauteur du point visé (négatif : sous la route, caméra inclinée vers le bas) | m | -20.0 | 10.0 | -6.3 |
| `camera.poursuite.zoom` | vue Poursuite : champ de vision (plus grand = plus large) | degrés | 30.0 | 110.0 | 66.0 |
| `camera.proche.recul` | vue Proche : distance derrière la voiture | m | 2.0 | 30.0 | 6.0 |
| `camera.proche.hauteur` | vue Proche : hauteur de la caméra | m | 0.5 | 20.0 | 2.67 |
| `camera.proche.regard_devant` | vue Proche : point visé devant la voiture | m | 0.0 | 40.0 | 7.0 |
| `camera.proche.regard_hauteur` | vue Proche : hauteur du point visé | m | -20.0 | 10.0 | -2.1 |
| `camera.proche.zoom` | vue Proche : champ de vision | degrés | 30.0 | 110.0 | 60.0 |
| `camera.capot.zoom` | vue Capot : champ de vision | degrés | 30.0 | 110.0 | 72.0 |
| `camera.cockpit.zoom` | vue Cockpit : champ de vision | degrés | 30.0 | 110.0 | 72.0 |
| `camera.zoom_vitesse` | champ de vision ajouté à pleine vitesse | degrés | 0.0 | 30.0 | 7.0 |
| `camera.zoom_nitro` | champ de vision ajouté pendant la nitro | degrés | 0.0 | 30.0 | 6.0 |
| `camera.suivi_oeil` | vitesse à laquelle la caméra suit la voiture (plus grand = plus raide) | /s | 1.0 | 60.0 | 14.0 |
| `camera.suivi_regard` | vitesse à laquelle le regard suit la route | /s | 1.0 | 60.0 | 16.0 |
| `trafic.densite` | nombre de voitures du trafic de chaque carte multiplié par ce facteur | × | 0.2 | 3.0 | 1.0 |
| `conduite.frein` | freinage | m/s² | 1.0 | 40.0 | 12.5 |
| `conduite.roulement` | ralentissement au lever de pied (roulement) | m/s² | 0.0 | 5.0 | 0.35 |
| `conduite.trainee` | freinage de l'air (croît avec la vitesse au carré) |  | 0.0 | 0.01 | 0.00042 |
| `conduite.frein_moteur` | frein moteur | m/s² | 0.0 | 10.0 | 0.9 |
| `conduite.adherence` | adhérence en virage (au-delà, la voiture glisse) | m/s² | 5.0 | 60.0 | 18.0 |
| `conduite.braquage_max` | braquage le plus fort |  | 0.1 | 1.5 | 0.6 |
| `conduite.braquage_vitesse` | vitesse au-delà de laquelle le braquage diminue | m/s | 1.0 | 60.0 | 14.0 |
| `conduite.braquage_entree` | rapidité du volant quand on braque | /s | 0.5 | 30.0 | 4.0 |
| `conduite.braquage_sortie` | rapidité du volant quand on relâche | /s | 0.5 | 30.0 | 6.0 |
| `conduite.realignement` | la voiture se remet dans l'axe de la route | /s | 0.0 | 20.0 | 3.0 |
| `conduite.cap_max` | angle le plus grand avec la route en course | rad | 0.1 | 1.6 | 0.5 |
| `conduite.hors_route` | ralentissement hors de la route | m/s² | 0.0 | 40.0 | 7.0 |
| `conduite.vitesse_libre` | conduite libre (demi-tour) sous cette vitesse | m/s | 0.0 | 40.0 | 10.0 |
| `conduite.vitesse_recul` | marche arrière enchaînée sous cette vitesse | m/s | 0.0 | 20.0 | 3.0 |
| `qualite.plancher_moyenne` | résolution la plus basse en qualité Moyenne | × résolution | 0.5 | 1.0 | 0.9 |

Pas encore dans le fichier (tickets en cours sur ces zones) : taille et place des commandes tactiles (WFS-102), nombre de joueurs du duel et distances proposées (écrans de WFS-102). À ajouter quand ces tickets seront fusionnés.
