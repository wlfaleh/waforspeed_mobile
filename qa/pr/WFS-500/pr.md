Agent : Claude, agent dev3d (01Us6uCv7g) (session_01Us6uCv7gTobdvCxCSWWV5j)

**Ticket** : WFS-500 — règles : `regles-de-gestion/WFS-500.md`, règles communes de l'épic : `regles-de-gestion/E19-cadeau-du-jour.md` (RC-E19-01 à 07)

## Description

- **Demande de l'utilisateur** (en une phrase) : un cadeau gratuit par jour, à minuit, heure du téléphone, avec des chances que tu as fixées (10-15 ★ 45 %, 16-25 ★ 28 %, 26-40 ★ 9 %, 41-50 ★ 3 %, coloris 8 %, skin 4 %, voiture prêtée 3 %), jamais moins de 10 ★, une garantie au 30e cadeau, une protection si l'heure du téléphone recule, et tout gardé dans la sauvegarde. Ce ticket ne fait que la **logique, sans écran** (l'écran : WFS-501 ; la voiture prêtée : WFS-502 ; le gain donné pour de vrai : WFS-503).
- **Ce qui change** :
  - **nouveau module `src/game/cadeau.h/.cpp`** (zone E3) :
    - `etat` / `disponible` : RG-06 (jour local après celui du dernier cadeau, pas d'accumulation, 1er lancement : disponible) et RG-07 (heure reculée sous l'heure UTC du dernier cadeau → bloqué, quel que soit le fuseau) ;
    - `tirer` : fonction pure qui fait le tirage. RG-01 : 7 chances lues dans `config().cadeau`. RG-02 : entier uniforme dans la tranche, au moins `etoiles_min`. RG-03 : voiture du joueur au hasard parmi celles qui ont encore un objet à gagner, puis l'objet. RG-04 : catégorie vide → étoiles, tranches renormalisées. RG-05 : garantie au 30e, skin contre voiture avec les poids 4 et 3, une seule possible → celle-là, aucune → étoiles et compteur à 0 ;
    - `ouvrir` : **la seule action** (RG-08). Elle tire, met à jour l'état, **écrit la sauvegarde et attend la fin de l'écriture** (`enregistrer()` puis `attendre()`), puis seulement rend le gain. Le gain n'est pas donné au joueur ici (WFS-502 / 503). **Aucune fonction ne relance, double ou rachète un cadeau** (RC-E19-01) ;
    - `entreeDepuis` : liste ce qu'il reste à gagner, d'après la sauvegarde. Coloris = couleurs et finitions non possédées (les gratuites sont déjà possédées) ; skins non possédées, `neonnuit` « cadeau seulement » comprise ; voitures à prêter = vendues, non possédées, pas déjà prêtées. **Les 3 voitures retirées (licence NC) sont exclues** ;
    - `maintenant` : heure UTC et décalage local du téléphone, calculé sans `tm_gmtoff` (RC-E19-04). Calendrier : algorithme `days_from_civil` de H. Hinnant (domaine public), recopié dans le fichier sans aucune dépendance ;
    - hasard : graine `Mulberry32` (RG-10). Même graine, même cadeau ; au jeu, graine tirée à l'ouverture (`std::random_device`), sans aucun lien avec les images ;
  - **`src/game/sauvegarde.h/.cpp`** : struct `EtatCadeau` et clés **nouvelles** `cadeau-heure`, `cadeau-decalage`, `cadeau-jour`, `cadeau-sans-rare`, `cadeau-total`, `cadeau-dernier`, ainsi que `pret-voiture`, `pret-recu`, `pret-fin` (seulement déclarées, lues et écrites : leur usage est pour WFS-502). Chaque valeur est contrôlée : une valeur fausse prend sa valeur par défaut, sans erreur. Aucune clé ancienne n'est retirée ; une ancienne sauvegarde se lit sans erreur et le cadeau y est disponible. Les clés ne sont écrites qu'après un 1er cadeau. `cadeau-decalage` vient de RC-E19-04 (« le décalage local du moment ») ;
  - **`src/core/configgame.h/.cpp`** (section `cadeau` après la section `market` de WFS-510, conflits résolus en gardant les deux), exception du PO (section `cadeau` seulement) : 18 clés `cadeau.*`. Après la lecture, `verifierCadeau` refuse un **total des chances ≠ 100 %** (les chances du jeu sont gardées, avec un message au journal) et une tranche min > max. La durée du prêt (3 h) et le délai d'activation (24 h) sont déjà dans les données (RC-E19-02), pour WFS-502 ;
  - **`configgame.json` / `configgame.README.md`** : seulement la section `cadeau` ajoutée à la fin, plus un paragraphe et 18 lignes au README. **Aucune de tes valeurs réglées n'est touchée** (le fichier n'est pas régénéré en entier) ;
  - **`src/game/course_system.cpp`**, banc `sauvegarde` seulement : au 1er lancement, un cadeau est ouvert à une heure fixe du banc et une 2e ouverture le même jour est refusée ; au 2e lancement, les clés `cadeau-*` relues sont écrites au journal ;
  - **`tests/test_cadeau.cpp`** (+ 1 ligne `CMakeLists.txt`) ;
  - **à l'écran, rien ne change** : 40 paires de captures, toutes identiques au pixel près.
- **Comment c'est vérifié** :
  - **`test_cadeau`**, RG-11 (a) à (h) et plus, 49 cas : [sortie complète](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/texte-test-cadeau.md) ;
    - (a) **100 000 tirages, chaque chance à ±0,16 point au plus**, pour ±0,5 demandé (tableau ci-dessous) ;
    - (b) minimum mesuré 10 ★ sur 100 000 tirages, replis compris, même avec une tranche réglée à 1-15 ;
    - (c) 2 000 garanties au 30e, compteur remis à 0 ; une seule catégorie possible → celle-là ; aucune → étoiles ;
    - (d) catégories vides → étoiles ≥ 10, tranches renormalisées ;
    - (e) 23:59 puis 00:01 → nouveau cadeau ; 2e ouverture le même jour refusée ; une semaine sans jouer → un seul cadeau ;
    - (f) heure reculée de 2 jours → bloqué, puis repassée → « déjà ouvert », puis le lendemain → disponible ; changement de fuseau Paris → Dubaï / New York ;
    - (g) 9 clés écrites puis relues à l'identique, ancienne sauvegarde → disponible, valeurs fausses ignorées ;
    - (h) total 101 % refusé, 100 % accepté, min > max refusé ;
  - **125 tests CPU réussis** après la synchronisation avec `main` (WFS-510 Market compris) (bloc « Tests CPU » ci-dessous), construction CMake complète sans avertissement dans les fichiers touchés ;
  - **banc `sauvegarde`** lancé deux fois : [journal avant / après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/texte-sauvegarde.md) ;
    - 1er lancement : « disponible », « cadeau du jour ouvert (2026-11-13) : etoiles;14 », « 2e ouverture le même jour refusée » ;
    - 2e lancement : `cadeau-heure=1794560400 cadeau-decalage=3600 cadeau-jour=2026-11-13 cadeau-sans-rare=1 cadeau-total=1 cadeau-dernier=etoiles;14`, « déjà ouvert aujourd'hui » ;
  - **captures** : bancs `sauvegarde` (×2) et `menu1` (1280x720). **40 paires comparées au pixel : 0 pixel différent** ([tableau](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/texte-pixels.md)).
- **Limites et risques connus** :
  - pas d'écran : le joueur ne voit encore aucun cadeau (WFS-501) ;
  - « maintenant » vient de l'heure du téléphone. Avancer l'horloge donne un cadeau plus tôt, puis bloque jusqu'à ce que l'heure réelle rattrape (c'est RG-07) ;
  - `ouvrir()` attend la fin de l'écriture du fichier sur le thread principal, soit quelques ms une fois par jour : c'est voulu (RG-08) ;
  - la capture « avant » `parcours-course` (dernière image du banc `menu1`) manque : le script a été arrêté par la limite de 30 min de ma tâche. Les 40 autres paires sont faites ;
  - pour construire et lancer les bancs ici, j'ai installé les paquets Ubuntu `libvulkan-dev`, `glslc`, `mesa-vulkan-drivers`, `ninja-build` et `imagemagick` (dépôt apt du conteneur, rien dans le dépôt Git).

### 100 000 tirages (graine 500, garantie coupée)

| gain | mesuré | attendu | écart |
|---|---|---|---|
| 10-15 ★ | 44,93 % | 45 % | −0,07 pt |
| 16-25 ★ | 27,91 % | 28 % | −0,09 pt |
| 26-40 ★ | 9,16 % | 9 % | +0,16 pt |
| 41-50 ★ | 3,07 % | 3 % | +0,07 pt |
| coloris | 7,90 % | 8 % | −0,10 pt |
| skin | 4,06 % | 4 % | +0,06 pt |
| voiture prêtée | 2,97 % | 3 % | −0,03 pt |

## Tests CPU

```
[OK] test_boutique (0 s)
[OK] test_cadeau (0 s)
[OK] test_configgame (0 s)
[OK] test_market (0 s)
[OK] test_personnalisation (0 s)
[OK] test_profil (0 s)
[OK] test_sauvegarde (0 s)
tests CPU : 125 test(s) réussi(s) en 412 s
```

## Captures pour le lead dev (8 paires montrées, 40 comparées au pixel : toutes identiques)

Dossier : [`qa/pr/WFS-500`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `fin-record` (banc `sauvegarde`, 1er lancement (cadeau ouvert dans ce lancement)) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/avant-fin-record.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/apres-fin-record.jpg?raw=true) |
| `sauvegarde-relue` (banc `sauvegarde`, 2e lancement (clés `cadeau-*` relues)) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/avant-sauvegarde-relue.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/apres-sauvegarde-relue.jpg?raw=true) |
| `menu-0` (menu principal) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/avant-menu-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/apres-menu-0.jpg?raw=true) |
| `menu-anim` (menu principal, animation) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/avant-menu-anim.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/apres-menu-anim.jpg?raw=true) |
| `garage-0` (garage) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/avant-garage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/apres-garage-0.jpg?raw=true) |
| `terrain-0` (choix du terrain) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/avant-terrain-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/apres-terrain-0.jpg?raw=true) |
| `pause-0` (pause) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/avant-pause-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/apres-pause-0.jpg?raw=true) |
| `fin-retour-menu` (retour au menu après la course) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/avant-fin-retour-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-500/qa/pr/WFS-500/apres-fin-retour-menu.jpg?raw=true) |

