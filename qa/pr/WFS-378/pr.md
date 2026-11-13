## Agent : Claude, agent dev3d (01Us6uCwca) (session_01Us6uCwca4JJHJFfVwyADFv)

**Ticket** : WFS-378 — règles : `regles-de-gestion/WFS-378.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : « Les formes géométriques sont interdites […] tout créer en 3D, 99 % réalisme, 100 % 3D » : trouver toutes les formes nues hors décor des cartes (garage, habitacle, véhicules, effets, portiques, menus 3D) et proposer le découpage en tickets.
- **Ce qui change** : un seul document, `regles-de-gestion/WFS-378-inventaire.md` (inventaire par zone : objet, fichier:ligne, forme, nombre à l'écran, classement NUE / MODELÉE / EFFET / INVISIBLE, capture), plus ces captures. **Aucun code, aucun objet remplacé** (RG-04) : rien ne change à l'écran.
- **Résultat** : 13 formes nues visibles (garage ×4, rampe de gyrophares, portique, cône K5a, panneau AK5, ralentisseur, poussette, laisse du chien, dessus et face de la planche de bord à arêtes vives ; + 1 à arbitrer : casque en ellipsoïde) ; 14 effets (halos face caméra, particules, ombres floues, gouttes) avec une question au PO ; 7 formes invisibles (code mort, banc, cockpit de secours jamais atteint car les 20 voitures ont un intérieur) ; **8 tickets proposés au PO** (§ 9, une ou deux zones chacun), classés par visibilité en course.
- **Comment c'est vérifié** : recherche des appels `box( boite( tri( quad( cylindre( cylAxe( cone( disque( tore( anneau( plan( ellipsoide( cube_ halo(` dans `src/` ; chaque ligne relue dans le code de `main` (d33588ad) ; recoupé avec `E16-inventaire.md` § 4 (numéros de ligne remis à jour) ; objets confirmés sur les captures du banc PC ci-dessous (regardées une à une).
- **Limites et risques connus** : la laisse du chien est trop fine pour se voir à 1280 px (pas de capture dédiée) ; la poussette est petite sur `enfants-ville` ; les gouttes de pluie et flocons sont des quads 2D d'écran (rangés en EFFET) ; la frontière EFFET / forme nue est une proposition à trancher par le PO.

## Tests CPU

`bash tools/qa/tests_cpu.sh build` sur la branche synchronisée avec `main` : **124 test(s) réussi(s)**, 0 échec, code de sortie 0 (`qa/pr/WFS-378/tests-cpu.log`). Aucun code changé : les tests prouvent seulement que la branche ne casse rien.

## Captures pour le lead dev (21 après ; pas d'« avant » : aucun code changé)

Dossier : [`qa/pr/WFS-378`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378) — produit par `tools/qa/captures_pr.sh` (`SANS_AVANT=1`), réduit aux captures utiles.

| Capture | Ce qu'elle montre | Image |
|---|---|---|
| `garage-0` | Garage : sol en disque, plateau tournant plat, anneau néon (tore), barres lumineuses (boîtes) — § 1 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-garage-0.jpg?raw=true) |
| `garage-1` | Garage, autre angle : anneau néon et barres — § 1 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-garage-1.jpg?raw=true) |
| `vie-portique` | Portique du checkpoint : poteaux, poutre, néons et bande au sol en boîtes — § 4 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-vie-portique.jpg?raw=true) |
| `vie-enseigne-nuit` | Panneau néon de bord de route sur 2 poteaux (boîtes) : décor, signalé § 8 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-vie-enseigne-nuit.jpg?raw=true) |
| `poursuite-police-gros-plan-jour` | Rampe de gyrophares de la police en 3 cubes — § 3 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-poursuite-police-gros-plan-jour.jpg?raw=true) |
| `poursuite-police-gros-plan-nuit` | Rampe de gyrophares la nuit, halos (effet) — § 3 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-poursuite-police-gros-plan-nuit.jpg?raw=true) |
| `police-flanc-00` | Voiture de police de dos (rampe de gyrophares) ; les bandes POLICE du flanc ne se voient pas ici — § 3 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-police-flanc-00.jpg?raw=true) |
| `poursuite-travaux-jour-proche` | Chantier : cônes K5a sur socle-boîte — § 4 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-poursuite-travaux-jour-proche.jpg?raw=true) |
| `poursuite-travaux` | Chantier : cônes et panneau AK5 (triangles plats + cubes) — § 4 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-poursuite-travaux.jpg?raw=true) |
| `poursuite-ralentisseur` | Ralentisseur et 2 plots en boîtes — § 4 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-poursuite-ralentisseur.jpg?raw=true) |
| `enfants-ville` | Famille avec poussette (cylindres, boîtes, quads), trottoir de droite — § 5 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-enfants-ville.jpg?raw=true) |
| `trafic-motos-cote-scooter` | Casque du pilote en ellipsoïde (à arbitrer) — § 6 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-trafic-motos-cote-scooter.jpg?raw=true) |
| `trafic-motos-pres-ville-jour` | Deux-roues en trafic : casque, ombre au sol (effet) — § 6 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-trafic-motos-pres-ville-jour.jpg?raw=true) |
| `halos-rue` | Nuit : halos des têtes de lampadaires (disque face caméra, avis N2) — § 7 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-halos-rue.jpg?raw=true) |
| `halos-pres` | Halos des lampadaires de près — § 7 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-halos-pres.jpg?raw=true) |
| `choc-etincelles` | Étincelles des chocs (carrés face caméra) — § 7 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-choc-etincelles.jpg?raw=true) |
| `fumee-derapage-jour` | Fumée des pneus (rectangles face caméra texturés) — § 7 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-fumee-derapage-jour.jpg?raw=true) |
| `nitro-lignes` | Lignes de vitesse de la nitro — § 7 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-nitro-lignes.jpg?raw=true) |
| `vitrine-golf-centre` | Habitacle de la Golf : dessus et face de la planche à arêtes vives, facettes sombres sous le volant (NUE) — § 2 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-vitrine-golf-centre.jpg?raw=true) |
| `vitrine-golf-portiere-bas` | Contre-porte et plancher (dalles arrondies) — § 2 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-vitrine-golf-portiere-bas.jpg?raw=true) |
| `vitrine-m4-centre` | Habitacle de la M4 : planche et console — § 2 | ![capture](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-378/qa/pr/WFS-378/apres-vitrine-m4-centre.jpg?raw=true) |
