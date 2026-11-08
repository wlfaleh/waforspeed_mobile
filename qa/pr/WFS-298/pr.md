Agent : arwa-4 (Claude, session 01Us6uCw)

**Ticket** : WFS-298 — règles : `regles-de-gestion/WFS-298.md`

## Demande
Ticket du PO (axe Réalisme, son), tiré de la comparaison au réel : aujourd'hui, un lampadaire, un arbre, un banc, un mur et une voiture font tous le même coup sourd, au centre. Il faut que chaque choc s'entende **comme ce qu'on touche** : plus fort et plus clair quand on va plus vite, et du côté du choc. Le coup sourd actuel de la carrosserie est gardé tel quel.

## Ce qui change
- **Nouveau module `src/audio/chocs_sons.*`**, branché dans `src/audio/audio_system.cpp`.
- **Décision du PO** sur le conflit RG-03 / RG-05 (option A, écrite dans le Suivi de WFS-298) : le son est **synthétisé en direct dans le thread audio** (synthèse modale). La clarté suit exactement la vitesse. Il ne passe pas par `SonsVoitures::jouer` et n'utilise aucun tampon par vitesse.
- **RG-01, familles** : poteau (métal sonore), arbre (tronc mat puis feuillage qui tremble), mobilier (tôle mince et bois), béton (mur, bâtiment, îlot : coup grave et bref), voiture (tôle et plastique, plus des éclats de verre et de plastique à l'accident). « Autre » : rien n'est ajouté.
- **RG-02, matières sourcées** : chaque mode décroît en T60 = 2,2 / (f·η), avec le facteur de perte **en place** du tableau C.1 de Bies, *Engineering Noise Control* (relu dans `jmrplens/phonometry`, MIT) :
  - acier boulonné 0,01, tôle d'acier 0,02 ;
  - bois 0,05, plastique 0,1, verre 0,02.

  T60 retenus :
  - poteau 1,17 s ;
  - arbre 0,142 s ;
  - mobilier 0,229 s ;
  - béton 0,08 s ;
  - voiture 0,379 s.

  Les fréquences des modes sont [à vérifier]. Celles du poteau sont calculées pour un mât d'éclairage en tube d'acier de 120 × 3,5 mm sur 6 m. Détails dans `references/reel/son/chocs.md`.
- **RG-03, vitesse** :
  - amplitude proportionnelle à la vitesse d'impact (de 7 à 80 km/h) ;
  - force du contact en demi-sinus de Hertz, dont la durée suit v^(−1/5) : la coupure de l'excitation suit donc v^(1/5) ;
  - 4 variantes par famille (modes ±5 %, graines), jamais deux fois la même de suite.
- **RG-04, ce qui est touché** :
  - quand `bumps` ou `accidents` de la course montent, on prend l'obstacle du registre `chocs::registre()` le plus proche du rectangle de la voiture (≤ 0,5 m, lecture seule) ;
  - sinon, l'autre voiture la plus proche (≤ 1 m) ;
  - le côté vient de l'azimut, passé à `voitures::panExistant` (WFS-272) ;
  - aucune accroche dans `src/game` : le repli n'a pas servi.
- **RG-05, mélange** :
  - le coup sourd de `sfx.cpp` n'est pas touché ;
  - le son de l'obstacle s'ajoute à `G`, `D` et au bus de l'écho, juste après les voix de WFS-272 (le tunnel s'y applique) ;
  - crête à 30 km/h : −4,5 dB de celle du coup sourd au centre, −1,6 dB du côté de l'obstacle ;
  - à l'accident : de 5,7 à 9,6 dB sous le son d'accident.
- **RG-00** : la ligne `(void)cad;` est retirée de `pluie_sons.cpp`. Le son de la pluie est inchangé et `test_audio_pluie` reste vert.
- **Zone** :
  - fichiers neufs `src/audio/chocs_sons*` et `tests/test_audio_chocs.cpp` ;
  - branchement dans `audio_system.cpp` ;
  - `CMakeLists.txt` : 1 ligne ;
  - `pluie_sons.cpp` : 1 ligne retirée ;
  - `voitures_sons.*`, `sfx.*` et `src/game/**` ne sont pas modifiés.

## Comment c'est vérifié
- **`test_audio_chocs`** (nouveau) : 13 cas, tous verts, avec la même synthèse que le jeu :
  - RG-01 : chaque genre donne la bonne famille ;
  - RG-02 : T60 mesuré (courbe de Schroeder) à ±6 % au plus de la valeur retenue, sur les 4 variantes ; T60 dans les plages du PO ;
  - RG-03 : +5,8 à +6,3 dB par doublement de la vitesse ; coupure en 2^(1/5) exactement ; du poteau de 15 à 60 km/h, les aigus montent de +15,1 dB contre +12,5 dB pour le grave ; jamais deux variantes de suite ;
  - RG-04 : distances exactes (poteau, mur, voiture) ; 16 dB du bon côté ;
  - RG-05 : crêtes conformes ; pire cas (moteur au rupteur, accident et poteau à 80 km/h) : 0,48, sous 1 ;
  - RG-06 : sortie et bus identiques à l'échantillon près sans choc ;
  - RG-07 : coût 0,031 ms par bloc avec les 4 voix actives, plafond 0,05.
- **66 tests CPU** verts après synchronisation avec `main` (903ba09).
- **Parcours complet** (`WFS_BANC=parcours`) : « parcours complet : 0 écart(s) OK » et « parcours complet en vue Cockpit (WFS-197) : 0 écart(s) OK ».
- **Captures avant / après** (`tools/qa/captures_pr.sh WFS-298 partie2a`, filtre sur les courbes audio) : les 27 courbes audio qui existaient déjà sont **identiques octet pour octet** à `main`, comme les .wav comparés (pluie-arriere, pluie-cockpit, pluie-tunnel, moteur, parois-ville). Le son de la pluie est donc inchangé après RG-00. Les 11 `chocs-*` sont nouvelles. L'« avant » de chaque famille est la scène `chocs-<famille>-avant` du banc (le coup sourd seul, comme `main`), d'où le « — » dans la colonne de gauche : une seconde passe `SANS_AVANT=1` n'apporterait rien.
  - Lampadaire à 30 km/h à droite : `chocs-poteau` (2e choc) contre `chocs-poteau-avant`.
  - Mur et arbre à 50 km/h : `chocs-mur` et `chocs-arbre` (3e choc).
  - Voiture à gauche à 80 km/h, accident : `chocs-voiture` (8e choc, éclats de 2 à 8 kHz sur la courbe verte).
  - Les 5 familles à 30 km/h : `chocs-familles`.
- **Banc `chocs`** (WFS-256, en local) : chaque « choc contre X » est suivi d'un « SON CHOC » de la bonne famille, soit **4 sur 4** (arbre 34 km/h, arbre 8 km/h, poteau 59 km/h deux fois, du bon côté).
  - Ce banc rend **« 4 écart(s) ÉCHEC »** (« 0 capture(s) sur 3 (le choc n'a pas eu lieu ?) » sur les 4 cartes).
  - **`main` fait exactement la même chose**, vérifié en le compilant à part : ce n'est pas dû à ce ticket. C'est à signaler au PO pour WFS-256.
- **Banc audio** (`benchChocs`, groupe `partie2a`) :
  - pour chaque famille, `chocs-<famille>` et `chocs-<famille>-avant` (le coup sourd seul, comme `main`), en .wav et .png ;
  - `chocs-familles` : les 5 familles à 30 km/h ;
  - au journal, un tableau : crête, T60, fréquence dominante, écart gauche / droite.

## Limites et risques
- Valeurs [à vérifier] :
  - fréquences et amplitudes des modes ;
  - durées de contact à 30 km/h ;
  - T60 du béton (0,08 s), du feuillage (0,8 s) et des éclats ;
  - loi « +6 dB par doublement » (hypothèse du PO).
- Le volume absolu dépend du téléphone. Les graves (188 Hz pour le mât) passent mal par un petit haut-parleur.
- La vitesse d'un accident n'est pas donnée par la course (`lastImpact` ne change qu'aux petits chocs) : on prend la vitesse du joueur au pas d'avant, au moins 14 m/s.
- Le parcours avec un choc voulu contre un lampadaire puis une voiture du trafic n'existe pas dans les bancs, et je ne peux pas l'ajouter (`src/game` hors zone). Les chocs sont prouvés par le banc `chocs` et par le test ; le parcours complet standard est lancé.

## Captures pour le lead dev (38 après)

Dossier : [`qa/pr/WFS-298`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `ambiance-campagne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-ambiance-campagne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-ambiance-campagne.jpg?raw=true) |
| `ambiance-circuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-ambiance-circuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-ambiance-circuit.jpg?raw=true) |
| `ambiance-plage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-ambiance-plage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-ambiance-plage.jpg?raw=true) |
| `ambiance-ville` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-ambiance-ville.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-ambiance-ville.jpg?raw=true) |
| `chocs-arbre-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-chocs-arbre-avant.jpg?raw=true) |
| `chocs-arbre` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-chocs-arbre.jpg?raw=true) |
| `chocs-familles` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-chocs-familles.jpg?raw=true) |
| `chocs-mobilier-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-chocs-mobilier-avant.jpg?raw=true) |
| `chocs-mobilier` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-chocs-mobilier.jpg?raw=true) |
| `chocs-mur-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-chocs-mur-avant.jpg?raw=true) |
| `chocs-mur` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-chocs-mur.jpg?raw=true) |
| `chocs-poteau-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-chocs-poteau-avant.jpg?raw=true) |
| `chocs-poteau` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-chocs-poteau.jpg?raw=true) |
| `chocs-voiture-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-chocs-voiture-avant.jpg?raw=true) |
| `chocs-voiture` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-chocs-voiture.jpg?raw=true) |
| `lieu-ferme` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-lieu-ferme.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-lieu-ferme.jpg?raw=true) |
| `lieu-fontaine` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-lieu-fontaine.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-lieu-fontaine.jpg?raw=true) |
| `lieu-terrasse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-lieu-terrasse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-lieu-terrasse.jpg?raw=true) |
| `lieu-tribune` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-lieu-tribune.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-lieu-tribune.jpg?raw=true) |
| `lieu-tunnel` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-lieu-tunnel.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-lieu-tunnel.jpg?raw=true) |
| `lieu-vagues` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-lieu-vagues.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-lieu-vagues.jpg?raw=true) |
| `masque-campagne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-masque-campagne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-masque-campagne.jpg?raw=true) |
| `masque-corniche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-masque-corniche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-masque-corniche.jpg?raw=true) |
| `masque-stands` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-masque-stands.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-masque-stands.jpg?raw=true) |
| `masque-ville` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-masque-ville.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-masque-ville.jpg?raw=true) |
| `moteur-courbe` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-moteur-courbe.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-moteur-courbe.jpg?raw=true) |
| `musique-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-musique-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-musique-course.jpg?raw=true) |
| `musique-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-musique-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-musique-menu.jpg?raw=true) |
| `parois-casino` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-parois-casino.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-parois-casino.jpg?raw=true) |
| `parois-falaise` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-parois-falaise.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-parois-falaise.jpg?raw=true) |
| `parois-stands` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-parois-stands.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-parois-stands.jpg?raw=true) |
| `parois-ville` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-parois-ville.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-parois-ville.jpg?raw=true) |
| `pluie-arriere` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-pluie-arriere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-pluie-arriere.jpg?raw=true) |
| `pluie-cockpit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-pluie-cockpit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-pluie-cockpit.jpg?raw=true) |
| `pluie-tunnel` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-pluie-tunnel.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-pluie-tunnel.jpg?raw=true) |
| `voitures-course5` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-voitures-course5.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-voitures-course5.jpg?raw=true) |
| `voitures-police` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-voitures-police.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-voitures-police.jpg?raw=true) |
| `voitures-trafic` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/avant-voitures-trafic.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-298/qa/pr/WFS-298/apres-voitures-trafic.jpg?raw=true) |
