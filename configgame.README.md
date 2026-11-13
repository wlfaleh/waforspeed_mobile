# configgame.json — réglages du jeu (WFS-128)

Le fichier `configgame.json` (racine du dépôt) règle le jeu **sans toucher au code**. Il est copié dans l'APK.

- **Sur le téléphone, sans reconstruire** : poser un `configgame.json` modifié dans `Android/data/com.waforspeed.game/files/` (câble USB ou gestionnaire de fichiers), puis relancer le jeu : il remplace celui de l'APK. Le retirer = retour aux valeurs de l'APK.
- **Sur PC (banc d'essai)** : `./configgame.json`, ou le fichier donné par la variable `WFS_CONFIG` ; relu à chaque partie.
- Les nombres s'écrivent avec un **point** (`0.5`, jamais `0,5`), comme en JSON.
- Une clé absente, un mauvais type ou une valeur hors bornes : le jeu garde sa valeur et l'écrit au journal (`configuration : …` dans logcat). Un fichier cassé : toutes les valeurs du jeu. Le jeu ne plante jamais à cause du fichier.
- Les clés qui commencent par `_` (comme `_aide`) sont des commentaires.
- **Noms des voitures** (section `voitures`, WFS-191) : `marque` (1 à 16 caractères) et `modele` (1 à 24) de chaque voiture, entre guillemets : lettres (accents compris), chiffres, espace, `-` et `.`. Ce sont les noms affichés dans le jeu (garage, menu, salon, course à 5) ; l'identifiant (`golf`, `m4`…) ne change jamais (sauvegardes et records gardés). Un nom invalide : le nom par défaut et une ligne au journal.
- **Boutique** (section `boutique`, WFS-487) : prix de **base** en étoiles des couleurs, finitions, skins et packs d'autocollants. Le prix payé pour une voiture = prix de base × coefficient de sa classe (`coefficients_classe`, `voitures.<id>.classe`), arrondi à l'étoile ; les packs d'autocollants n'ont pas de coefficient. `gratuit` = 1 : offert sur toutes les voitures ; `cadeau` = 1 : seulement dans le cadeau du jour ou un succès. `voitures` d'une skin : identifiants séparés par des espaces.
- **Étoiles et voitures** (section `boutique`, WFS-498) : `voitures.<id>.prix` = prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; x5, a45, elantra retirées, jamais vendues) ; `gain_course` = étoiles gagnées à la fin d'une course terminée ; `minutes_par_course` sert seulement au banc d'équilibrage (`WFS_BANC=equilibrage`) ; `medailles` = packs du market (étoiles et prix en euros), qui donnent l'équivalent en médailles et en euros de chaque prix.
- **Cadeau du jour** (section `cadeau`, WFS-500) : un cadeau gratuit par jour (minuit, heure du téléphone). `tranches.t1` à `t4` : étoiles tirées au hasard entre `min` et `max`, avec leur `chance` en % ; `coloris`, `skin`, `voiture` : chance en % d'un coloris, d'une skin ou d'une voiture prêtée. **Le total des 7 chances doit faire 100** : sinon le jeu garde ses chances et l'écrit au journal. `etoiles_min` : jamais moins d'étoiles par cadeau (aussi quand tout est déjà possédé) ; `garantie` : le cadeau numéro N ouvert sans voiture ni skin en donne une ; `pret_heures` et `activation_heures` : durée du prêt et délai pour l'activer (WFS-502).
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
| `nitro.recharge_temps` | Course à 5 et Duel : recharge de la nitro par seconde de course | par s | 0.0 | 0.2 | 0.015 |
| `nitro.recharge_depassement` | Course à 5 : recharge à chaque adversaire dépassé | réserve | 0.0 | 1.0 | 0.1 |
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
| `poursuite.score_depart` | score au départ de la poursuite | points | 1 | 1000 | 1000 |
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
| `camera.poursuite.regard_devant` | vue Poursuite : point visé devant la voiture | m | 0.0 | 40.0 | 10.0 |
| `camera.poursuite.regard_hauteur` | vue Poursuite : hauteur du point visé (négatif : sous la route, caméra inclinée vers le bas) | m | -20.0 | 10.0 | -4.5 |
| `camera.poursuite.zoom` | vue Poursuite : champ de vision (plus grand = plus large) | degrés | 30.0 | 110.0 | 64.0 |
| `camera.proche.recul` | vue Proche : distance derrière la voiture | m | 2.0 | 30.0 | 6.0 |
| `camera.proche.hauteur` | vue Proche : hauteur de la caméra | m | 0.5 | 20.0 | 2.67 |
| `camera.proche.regard_devant` | vue Proche : point visé devant la voiture | m | 0.0 | 40.0 | 7.0 |
| `camera.proche.regard_hauteur` | vue Proche : hauteur du point visé | m | -20.0 | 10.0 | -2.1 |
| `camera.proche.zoom` | vue Proche : champ de vision | degrés | 30.0 | 110.0 | 60.0 |
| `camera.capot.zoom` | vue Capot : champ de vision | degrés | 30.0 | 110.0 | 72.0 |
| `camera.cockpit.zoom` | vue Cockpit : champ de vision (WFS-323 : 62°, cadrage de la référence de l'utilisateur) | degrés | 30.0 | 110.0 | 62.0 |
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
| `qualite.reflet_route_mouillee` | force du reflet de la rue sur la route mouillée (pluie) | × | 0.0 | 1.5 | 1.0 |
| `diagnostic.msaa_x1` | diagnostic : MSAA ×1 (1 = coupé) | 0 / 1 | 0 | 1 | 0 |
| `diagnostic.sans_prepasse` | diagnostic : sans pré-passe de profondeur | 0 / 1 | 0 | 1 | 0 |
| `diagnostic.sans_reflet` | diagnostic : sans reflet de la route mouillée | 0 / 1 | 0 | 1 | 0 |
| `diagnostic.sans_ombres` | diagnostic : sans ombres du soleil | 0 / 1 | 0 | 1 | 0 |
| `diagnostic.sans_pluie` | diagnostic : sans pluie (gouttes et flaques) | 0 / 1 | 0 | 1 | 0 |
| `diagnostic.sans_halos` | diagnostic : sans halos additifs | 0 / 1 | 0 | 1 | 0 |
| `diagnostic.sans_post` | diagnostic : sans bloom ni post-traitement | 0 / 1 | 0 | 1 | 0 |
| `diagnostic.sans_lumieres` | diagnostic : 0 lumière de nuit | 0 / 1 | 0 | 1 | 0 |
| `diagnostic.resolution_30` | diagnostic : scène rendue à 30 % | 0 / 1 | 0 | 1 | 0 |
| `diagnostic.couleur_unie` | diagnostic : shader couleur unie (coût des triangles seul) | 0 / 1 | 0 | 1 | 0 |
| `diagnostic.sans_decor` | diagnostic : décor coupé (route, sol et voitures gardés) | 0 / 1 | 0 | 1 | 0 |
| `diagnostic.distance_demi` | diagnostic : distance d'affichage divisée par 2 | 0 / 1 | 0 | 1 | 0 |
| `paquets.adresse_liste` | adresse https de la liste des paquets de cartes (cartes.json) | caractères | 12 | 200 | "https://github.com/wlfaleh/waforspeed_mobile/releases/download/paquets/cartes.json" |
| `confidentialite_url` | adresse https de la politique de confidentialité (écran Crédits) ; vide : ligne cachée | caractères | 0 | 200 | "" |
| `voitures.golf.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Norvik" |
| `voitures.golf.modele` | modèle affiché | caractères | 1 | 24 | "Lume" |
| `voitures.m4.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Draven" |
| `voitures.m4.modele` | modèle affiché | caractères | 1 | 24 | "Vantor GT" |
| `voitures.p911.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Pfeiler" |
| `voitures.p911.modele` | modèle affiché | caractères | 1 | 24 | "9F S" |
| `voitures.gallardo.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Toro" |
| `voitures.gallardo.modele` | modèle affiché | caractères | 1 | 24 | "Ventura V10 Spyder" |
| `voitures.p208.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Lionne" |
| `voitures.p208.modele` | modèle affiché | caractères | 1 | 24 | "L2" |
| `voitures.abarth.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Scorpio" |
| `voitures.abarth.modele` | modèle affiché | caractères | 1 | 24 | "Cinquina" |
| `voitures.tcross.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Vollmer" |
| `voitures.tcross.modele` | modèle affiché | caractères | 1 | 24 | "Trek-X" |
| `voitures.compass.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Ranger" |
| `voitures.compass.modele` | modèle affiché | caractères | 1 | 24 | "Bussola" |
| `voitures.s3.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Aurel" |
| `voitures.s3.modele` | modèle affiché | caractères | 1 | 24 | "R3 berline" |
| `voitures.x5.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Bavaro" |
| `voitures.x5.modele` | modèle affiché | caractères | 1 | 24 | "XR5" |
| `voitures.giulia.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Alvar" |
| `voitures.giulia.modele` | modèle affiché | caractères | 1 | 24 | "Selene TS" |
| `voitures.a45.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Merkur" |
| `voitures.a45.modele` | modèle affiché | caractères | 1 | 24 | "A-Sport 45" |
| `voitures.model3.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Volta" |
| `voitures.model3.modele` | modèle affiché | caractères | 1 | 24 | "E3" |
| `voitures.corsa.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Orion" |
| `voitures.corsa.modele` | modèle affiché | caractères | 1 | 24 | "Corsaire" |
| `voitures.gt86.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Tamura" |
| `voitures.gt86.modele` | modèle affiché | caractères | 1 | 24 | "Hachi" |
| `voitures.elantra.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Hanseo" |
| `voitures.elantra.modele` | modèle affiché | caractères | 1 | 24 | "Elanta R" |
| `voitures.v60.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Valka" |
| `voitures.v60.modele` | modèle affiché | caractères | 1 | 24 | "V6 Nordstern" |
| `voitures.rrsport.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Highland" |
| `voitures.rrsport.modele` | modèle affiché | caractères | 1 | 24 | "Sport" |
| `voitures.mustang.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Stallion" |
| `voitures.mustang.modele` | modèle affiché | caractères | 1 | 24 | "GT5" |
| `voitures.sf90.marque` | marque affichée (garage, menu, salon, course à 5) | caractères | 1 | 16 | "Cavallo" |
| `voitures.sf90.modele` | modèle affiché | caractères | 1 | 24 | "SF-X" |
| `boutique.coefficients_classe.gratuites` | coefficient des prix de personnalisation des voitures gratuites | × | 0.1 | 10.0 | 1.0 |
| `boutique.coefficients_classe.classe1` | coefficient des prix de personnalisation pour cette classe de voiture | × | 0.1 | 10.0 | 1.0 |
| `boutique.coefficients_classe.classe2` | coefficient des prix de personnalisation pour cette classe de voiture | × | 0.1 | 10.0 | 1.5 |
| `boutique.coefficients_classe.classe3` | coefficient des prix de personnalisation pour cette classe de voiture | × | 0.1 | 10.0 | 2.0 |
| `boutique.coefficients_classe.classe4` | coefficient des prix de personnalisation pour cette classe de voiture | × | 0.1 | 10.0 | 3.0 |
| `boutique.coefficients_classe.classe5` | coefficient des prix de personnalisation pour cette classe de voiture | × | 0.1 | 10.0 | 4.0 |
| `boutique.voitures.golf.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 2 |
| `boutique.voitures.golf.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 600 |
| `boutique.voitures.m4.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 3 |
| `boutique.voitures.m4.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 2200 |
| `boutique.voitures.p911.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 4 |
| `boutique.voitures.p911.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 3500 |
| `boutique.voitures.gallardo.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 4 |
| `boutique.voitures.gallardo.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 3000 |
| `boutique.voitures.p208.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 0 |
| `boutique.voitures.p208.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 0 |
| `boutique.voitures.abarth.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 0 |
| `boutique.voitures.abarth.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 0 |
| `boutique.voitures.tcross.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 1 |
| `boutique.voitures.tcross.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 300 |
| `boutique.voitures.compass.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 1 |
| `boutique.voitures.compass.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 300 |
| `boutique.voitures.s3.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 2 |
| `boutique.voitures.s3.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 1000 |
| `boutique.voitures.x5.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 3 |
| `boutique.voitures.x5.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 0 |
| `boutique.voitures.giulia.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 3 |
| `boutique.voitures.giulia.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 2400 |
| `boutique.voitures.a45.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 3 |
| `boutique.voitures.a45.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 0 |
| `boutique.voitures.model3.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 2 |
| `boutique.voitures.model3.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 1200 |
| `boutique.voitures.corsa.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 1 |
| `boutique.voitures.corsa.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 250 |
| `boutique.voitures.gt86.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 0 |
| `boutique.voitures.gt86.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 0 |
| `boutique.voitures.elantra.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 2 |
| `boutique.voitures.elantra.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 0 |
| `boutique.voitures.v60.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 2 |
| `boutique.voitures.v60.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 1000 |
| `boutique.voitures.rrsport.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 2 |
| `boutique.voitures.rrsport.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 800 |
| `boutique.voitures.mustang.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 3 |
| `boutique.voitures.mustang.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 1800 |
| `boutique.voitures.sf90.classe` | classe de la voiture (0 gratuite, 1 à 5) : choisit le coefficient des prix | classe | 0 | 5 | 5 |
| `boutique.voitures.sf90.prix` | prix de la voiture en étoiles (0 : gratuite, possédée dès le départ ; voitures retirées jamais vendues) | ★ | 0 | 1000000 | 6000 |
| `boutique.couleurs.blanc.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 0 |
| `boutique.couleurs.blanc.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 1 |
| `boutique.couleurs.noir.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 0 |
| `boutique.couleurs.noir.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 1 |
| `boutique.couleurs.gris.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 0 |
| `boutique.couleurs.gris.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 1 |
| `boutique.couleurs.rouge.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 0 |
| `boutique.couleurs.rouge.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 1 |
| `boutique.couleurs.bleu.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 0 |
| `boutique.couleurs.bleu.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 1 |
| `boutique.couleurs.orange.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 0 |
| `boutique.couleurs.orange.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 1 |
| `boutique.couleurs.jaune.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 0 |
| `boutique.couleurs.jaune.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 1 |
| `boutique.couleurs.vert.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 0 |
| `boutique.couleurs.vert.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 1 |
| `boutique.couleurs.nacre.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.nacre.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.bleunuit.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.bleunuit.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.anglais.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.anglais.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.bordeaux.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.bordeaux.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.sable.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.sable.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.violet.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.violet.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.turquoise.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.turquoise.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.taxi.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.taxi.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.bleuelectrique.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.bleuelectrique.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.vertacide.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.vertacide.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.orangefluo.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.orangefluo.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.rose.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.rose.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.jaunecitron.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.jaunecitron.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.violetneon.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.violetneon.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.rougevif.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.rougevif.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.cyan.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.cyan.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.vertmenthe.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.vertmenthe.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.magenta.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.magenta.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.corail.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.corail.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleurs.bleuciel.prix` | prix de base de la couleur (× coefficient de la classe) | ★ | 0 | 100000 | 50 |
| `boutique.couleurs.bleuciel.gratuit` | couleur gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.couleur_secondaire.prix` | prix de base de la couleur secondaire (toit, rétroviseurs ; × coefficient) | ★ | 0 | 100000 | 50 |
| `boutique.finitions.brillant.prix` | prix de base de la finition (× coefficient de la classe) | ★ | 0 | 100000 | 0 |
| `boutique.finitions.brillant.gratuit` | finition gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 1 |
| `boutique.finitions.metallise.prix` | prix de base de la finition (× coefficient de la classe) | ★ | 0 | 100000 | 100 |
| `boutique.finitions.metallise.gratuit` | finition gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.finitions.nacree.prix` | prix de base de la finition (× coefficient de la classe) | ★ | 0 | 100000 | 150 |
| `boutique.finitions.nacree.gratuit` | finition gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.finitions.mat.prix` | prix de base de la finition (× coefficient de la classe) | ★ | 0 | 100000 | 150 |
| `boutique.finitions.mat.gratuit` | finition gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.finitions.satine.prix` | prix de base de la finition (× coefficient de la classe) | ★ | 0 | 100000 | 150 |
| `boutique.finitions.satine.gratuit` | finition gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.finitions.candy.prix` | prix de base de la finition (× coefficient de la classe) | ★ | 0 | 100000 | 250 |
| `boutique.finitions.candy.gratuit` | finition gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.finitions.chrome.prix` | prix de base de la finition (× coefficient de la classe) | ★ | 0 | 100000 | 400 |
| `boutique.finitions.chrome.gratuit` | finition gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.finitions.cameleon.prix` | prix de base de la finition (× coefficient de la classe) | ★ | 0 | 100000 | 400 |
| `boutique.finitions.cameleon.gratuit` | finition gratuite sur toutes les voitures | 0 / 1 | 0 | 1 | 0 |
| `boutique.skins.rallye.prix` | prix de base de la skin (× coefficient ; valeur si cadeau) | ★ | 0 | 100000 | 300 |
| `boutique.skins.rallye.cadeau` | skin seulement dans le cadeau du jour ou un succès (jamais en vente) | 0 / 1 | 0 | 1 | 0 |
| `boutique.skins.rallye.voitures` | voitures qui ont cette skin (identifiants séparés par des espaces) | caractères | 1 | 160 | "p208 abarth gt86 golf mustang giulia" |
| `boutique.skins.drifttokyo.prix` | prix de base de la skin (× coefficient ; valeur si cadeau) | ★ | 0 | 100000 | 600 |
| `boutique.skins.drifttokyo.cadeau` | skin seulement dans le cadeau du jour ou un succès (jamais en vente) | 0 / 1 | 0 | 1 | 0 |
| `boutique.skins.drifttokyo.voitures` | voitures qui ont cette skin (identifiants séparés par des espaces) | caractères | 1 | 160 | "p208 abarth gt86 golf mustang giulia" |
| `boutique.skins.neonnuit.prix` | prix de base de la skin (× coefficient ; valeur si cadeau) | ★ | 0 | 100000 | 800 |
| `boutique.skins.neonnuit.cadeau` | skin seulement dans le cadeau du jour ou un succès (jamais en vente) | 0 / 1 | 0 | 1 | 1 |
| `boutique.skins.neonnuit.voitures` | voitures qui ont cette skin (identifiants séparés par des espaces) | caractères | 1 | 160 | "p208 abarth gt86 golf mustang giulia" |
| `boutique.skins.camouflage.prix` | prix de base de la skin (× coefficient ; valeur si cadeau) | ★ | 0 | 100000 | 450 |
| `boutique.skins.camouflage.cadeau` | skin seulement dans le cadeau du jour ou un succès (jamais en vente) | 0 / 1 | 0 | 1 | 0 |
| `boutique.skins.camouflage.voitures` | voitures qui ont cette skin (identifiants séparés par des espaces) | caractères | 1 | 160 | "p208 abarth gt86 golf mustang giulia" |
| `boutique.dessins.essentiels.prix` | prix du pack d'autocollants (sans coefficient, pour toutes les voitures) | ★ | 0 | 100000 | 0 |
| `boutique.dessins.essentiels.gratuit` | pack d'autocollants gratuit | 0 / 1 | 0 | 1 | 1 |
| `boutique.dessins.numeros.prix` | prix du pack d'autocollants (sans coefficient, pour toutes les voitures) | ★ | 0 | 100000 | 150 |
| `boutique.dessins.numeros.gratuit` | pack d'autocollants gratuit | 0 / 1 | 0 | 1 | 0 |
| `boutique.dessins.bandes.prix` | prix du pack d'autocollants (sans coefficient, pour toutes les voitures) | ★ | 0 | 100000 | 200 |
| `boutique.dessins.bandes.gratuit` | pack d'autocollants gratuit | 0 / 1 | 0 | 1 | 0 |
| `boutique.dessins.flammes.prix` | prix du pack d'autocollants (sans coefficient, pour toutes les voitures) | ★ | 0 | 100000 | 250 |
| `boutique.dessins.flammes.gratuit` | pack d'autocollants gratuit | 0 / 1 | 0 | 1 | 0 |
| `boutique.dessins.etoiles.prix` | prix du pack d'autocollants (sans coefficient, pour toutes les voitures) | ★ | 0 | 100000 | 300 |
| `boutique.dessins.etoiles.gratuit` | pack d'autocollants gratuit | 0 / 1 | 0 | 1 | 0 |
| `boutique.gain_course` | étoiles gagnées à la fin d'une course terminée (en attendant l'étape « score ») | ★ | 0 | 100000 | 10 |
| `boutique.minutes_par_course` | durée moyenne d'une course (banc d'équilibrage seulement) | min | 0.1 | 60.0 | 2.96 |
| `boutique.medailles.bronze.etoiles` | étoiles du pack de médailles (market) ; sert à l'équivalent en médailles des prix | ★ | 1 | 1000000 | 100 |
| `boutique.medailles.bronze.euros` | prix du pack de médailles en euros (bronze : sert à l'équivalent en euros des prix) | € | 0.01 | 1000.0 | 0.99 |
| `boutique.medailles.argent.etoiles` | étoiles du pack de médailles (market) ; sert à l'équivalent en médailles des prix | ★ | 1 | 1000000 | 300 |
| `boutique.medailles.argent.euros` | prix du pack de médailles en euros (bronze : sert à l'équivalent en euros des prix) | € | 0.01 | 1000.0 | 2.49 |
| `boutique.medailles.or.etoiles` | étoiles du pack de médailles (market) ; sert à l'équivalent en médailles des prix | ★ | 1 | 1000000 | 600 |
| `boutique.medailles.or.euros` | prix du pack de médailles en euros (bronze : sert à l'équivalent en euros des prix) | € | 0.01 | 1000.0 | 4.49 |
| `cadeau.etoiles_min` | étoiles au moins par cadeau, replis compris | ★ | 1 | 1000 | 10 |
| `cadeau.garantie` | le cadeau ouvert numéro N sans voiture ni skin donne une voiture prêtée ou une skin | cadeaux | 2 | 1000 | 30 |
| `cadeau.tranches.t1.min` | plus petit nombre d'étoiles de la tranche | ★ | 1 | 100000 | 10 |
| `cadeau.tranches.t1.max` | plus grand nombre d'étoiles de la tranche (≥ min) | ★ | 1 | 100000 | 15 |
| `cadeau.tranches.t1.chance` | chance de tirer cette tranche d'étoiles | % | 0.0 | 100.0 | 45.0 |
| `cadeau.tranches.t2.min` | plus petit nombre d'étoiles de la tranche | ★ | 1 | 100000 | 16 |
| `cadeau.tranches.t2.max` | plus grand nombre d'étoiles de la tranche (≥ min) | ★ | 1 | 100000 | 25 |
| `cadeau.tranches.t2.chance` | chance de tirer cette tranche d'étoiles | % | 0.0 | 100.0 | 28.0 |
| `cadeau.tranches.t3.min` | plus petit nombre d'étoiles de la tranche | ★ | 1 | 100000 | 26 |
| `cadeau.tranches.t3.max` | plus grand nombre d'étoiles de la tranche (≥ min) | ★ | 1 | 100000 | 40 |
| `cadeau.tranches.t3.chance` | chance de tirer cette tranche d'étoiles | % | 0.0 | 100.0 | 9.0 |
| `cadeau.tranches.t4.min` | plus petit nombre d'étoiles de la tranche | ★ | 1 | 100000 | 41 |
| `cadeau.tranches.t4.max` | plus grand nombre d'étoiles de la tranche (≥ min) | ★ | 1 | 100000 | 50 |
| `cadeau.tranches.t4.chance` | chance de tirer cette tranche d'étoiles | % | 0.0 | 100.0 | 3.0 |
| `cadeau.coloris.chance` | chance d'un coloris (couleur ou finition) pas encore possédé, pour une voiture du joueur | % | 0.0 | 100.0 | 8.0 |
| `cadeau.skin.chance` | chance d'une skin pas encore possédée, pour une voiture du joueur | % | 0.0 | 100.0 | 4.0 |
| `cadeau.voiture.chance` | chance d'une voiture que le joueur n'a pas, prêtée | % | 0.0 | 100.0 | 3.0 |
| `cadeau.pret_heures` | durée du prêt d'une voiture gagnée (WFS-502) | h | 0.1 | 72.0 | 3.0 |
| `cadeau.activation_heures` | délai pour activer une voiture prêtée (WFS-502) | h | 1.0 | 720.0 | 24.0 |

Pas encore dans le fichier (tickets en cours sur ces zones) : taille et place des commandes tactiles (WFS-102), nombre de joueurs du duel et distances proposées (écrans de WFS-102). À ajouter quand ces tickets seront fusionnés.
