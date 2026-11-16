Agent : Claude, agent dev3d (01Us6uCwHf)

**Ticket** : WFS-496 (épic E18, P1) — règles : `regles-de-gestion/WFS-496.md`, règles communes `regles-de-gestion/E18-personnalisation.md`.

## Description

- **Demande de l'utilisateur** (en une phrase) : un atelier d'autocollants au garage, où le joueur choisit des dessins dans une bibliothèque (bandes, numéros, flammes, éclairs, étoiles, lettres), les place, les tourne, les agrandit et les colore sur sa voiture, en 20 calques au plus, puis valide pour rouler avec.
- **Ce qui change** :
  - **À l'écran** :
    - Garage : nouveau bouton **« ATELIER »** sous « MARKET », au même format ; les autres boutons du garage ne changent pas (RG-01).
    - Écran **Atelier**, posé sur la scène du garage :
      - en haut : 5 vues (Gauche, Droite, Avant, Arrière, Dessus), Annuler, Refaire ;
      - à gauche : la **bibliothèque** en 6 onglets ; les 10 motifs gratuits d'abord, puis les motifs des packs avec leur cadenas et leur prix en ★ (RG-02) ;
      - à droite : la **liste des calques** (« Calques n/20 ») : choisir, Masquer / Afficher, Monter, Descendre (RG-04) ;
      - en bas : –5°, +5°, –5 %, +5 %, Miroir, Couleur (les 28 couleurs), Supprimer, « Quitter » (autocollant) et « Valider » (Start/Stop) (RG-03).
  - **Gestes** (RG-03) :
    - Glisser sur l'autocollant choisi le **place** sur le plan de son côté de la voiture. Pendant le geste, seul son **contour cyan** (interface 2D) suit le doigt ; la cuisson est refaite au doigt levé.
    - **Deux doigts** le tournent et l'agrandissent.
    - Glisser ailleurs **tourne la vue** autour de la voiture (RG-01).
    - Un geste continu compte pour **un seul pas** d'annulation.
  - **Valider, Quitter, Retour** (RG-05) :
    - « Valider » range le texte des calques dans la sauvegarde (nouvelle clé `autocollants-<voiture>`), puis le **cuit par WFS-495** sur la voiture du garage. La cuisson est refaite au départ de la course (`Game::setCar`) et au retour au menu.
    - « Quitter » après un changement ouvre la fenêtre « Quitter sans valider ? » (Quitter / Rester).
    - La touche **Retour** du téléphone fait la même chose que « Quitter » (zone retour de la manette, WFS-116).
  - **Textes** : FR et EN, clés `atelier.*` dans `assets/textes/fr.txt` et `assets/textes/en.txt` (RG-06, E20).
  - **Fichiers nouveaux** (zone E8) :
    - `src/ui/ecrans/atelier.*` : l'écran, système « atelier », ordre 467 ;
    - `src/ui/ecrans/atelier_calques.*` : la logique sans écran (calques, historique de 20 pas, motifs gratuits et packs, sauvegarde) ;
    - `src/ui/ecrans/atelier_banc.cpp` : le banc `WFS_BANC=atelier` ;
    - `tests/test_atelier.cpp` : le test CPU, 41 cas.
  - **Réutilisé sans le recréer** :
    - les calques, le format texte, la bibliothèque de 50 motifs et la cuisson de WFS-495 ;
    - les 28 couleurs `PEINTURES` de WFS-487 ;
    - les packs et leurs prix de `perso::catalogue()` ;
    - les boutons R19 de WFS-549 et le verre de WFS-548.
  - **Accroches** (≤ 10 lignes par fichier, annoncées au journal) :
    - `src/ui/garage.cpp` : 8 lignes (bouton, touchers, caméra, aperçu, voiture fixe dans l'atelier) ;
    - `src/game/game.cpp` : 2 lignes, sur la variable `WFS_AUTOCOLLANTS` que WFS-495 avait prévue pour l'atelier ;
    - `src/ui/garage_skin.cpp` : 2 lignes (retour au menu) ;
    - `src/game/sauvegarde.*` : 4 lignes (clé `autocollants-<voiture>`) ;
    - `src/platform/desktop_main.cpp` : 2 lignes (banc) ;
    - `CMakeLists.txt` : 1 ligne (test).
  - **Choix (dans le cadre de RG-02)** :
    - Les 10 gratuits sont `bande-large`, `bande-double`, `chiffre-1`, `chiffre-2`, `chiffre-3`, `flamme-1`, `eclair-1`, `etoile-1`, `lettre-a` et `lettre-b`.
    - Les autres motifs vont dans les packs de `configgame.h` : numéros et lettres, bandes, flammes et éclairs, étoiles et formes.
    - Un motif d'un pack non possédé ne se pose pas : la vente des packs est dans WFS-497. Le toucher affiche le message « Autocollant du pack « … » ».
- **Comment c'est vérifié** :
  - Tests CPU : 150 tests, dont `test_atelier` (41 cas). Le journal complet est dans `qa/pr/WFS-496/tests-cpu.log`.
  - Banc `WFS_BANC=atelier` (1280 × 720) : **0 écart**. Tout s'y fait par de vrais touchers, avec une vraie sauvegarde :
    - garage, bouton, atelier ;
    - 5 calques : bande, numéro déplacé au doigt (60, 25 cm obtenus, 60, 25 attendus), flamme à deux doigts (20° et 140 % obtenus), éclair à droite en miroir, bande sur le capot ;
    - cadenas, masquer, descendre, annuler, refaire ;
    - Valider : texte gardé et cuit ;
    - 21e calque refusé ;
    - Quitter, Rester, touche Retour ;
    - parcours **garage → atelier → 5 calques → Valider → Choisir → Rouler → course de jour → retour au garage → relance** (RC-E18-09), habillage gardé ;
    - 500 et GT86 habillées ;
    - courses de **jour et de nuit** avec les 3 voitures ;
    - anglais.
  - Cuisson mesurée sur PC (4 cœurs) : **43 à 56 ms**, au garage et au chargement, jamais en course.
- **Limites et risques connus** :
  - Le blanc d'un autocollant paraît gris-bleu sous les projecteurs colorés du garage. C'est le rendu de WFS-495 (adhésif sous le vernis), il n'est pas changé ici.
  - En course, la caméra de poursuite montre surtout l'arrière : les autocollants des flancs se voient peu sur les captures de course. Pour cette raison, les 500 et GT86 du banc portent aussi un numéro à l'arrière.
  - La position par défaut d'une bande sur le capot (ancrage de WFS-495, à 20 % de l'avant) déborde en partie devant la voiture. Le joueur la glisse ; la partie hors carrosserie n'est jamais peinte.
  - RC-E18-03 (images/s en Basse) n'est pas concerné en course : la texture d'habillage est celle de WFS-495, cuite au chargement. L'atelier ne cuit qu'au garage.
  - Les packs payants, le texte libre et « Mes créations » sont hors périmètre (WFS-497).
  - Pas d'avis `@3d` obligatoire (épic E18, pas E15).

## Captures pour le lead dev (21 après, 1 paire avant / après)

Dossier : [`qa/pr/WFS-496`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496) — produit par `tools/qa/captures_pr.sh WFS-496 atelier garage`, sélection ciblée.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `garage-0` : garage : bouton « ATELIER » sous « MARKET » (avant : absent), autres boutons inchangés | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/avant-garage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-garage-0.jpg?raw=true) |
| `atelier-garage-bouton` : garage de la 208 : bouton « ATELIER » | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-garage-bouton.jpg?raw=true) |
| `atelier-ouvert` : atelier ouvert : vues, bibliothèque (2 bandes gratuites, 3 avec cadenas et prix), liste vide | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-ouvert.jpg?raw=true) |
| `atelier-glisser` : glisser : le numéro suit le doigt (contour cyan), cuisson au doigt levé | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-glisser.jpg?raw=true) |
| `atelier-deux-doigts` : deux doigts : flamme en cours de rotation et d'agrandissement (20°, 140 %) | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-deux-doigts.jpg?raw=true) |
| `atelier-calques` : 3 calques sur le flanc gauche, flamme choisie | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-calques.jpg?raw=true) |
| `atelier-vue-droite` : vue droite : éclair posé, en miroir | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-vue-droite.jpg?raw=true) |
| `atelier-vue-dessus` : vue de dessus : bande double sur le capot, liste des 5 calques | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-vue-dessus.jpg?raw=true) |
| `atelier-verrou` : motif d'un pack non possédé : message, rien posé | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-verrou.jpg?raw=true) |
| `atelier-liste-masque` : liste : calque masqué (« Afficher »), non cuit | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-liste-masque.jpg?raw=true) |
| `atelier-max` : 21e autocollant : « 20 calques au maximum » | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-max.jpg?raw=true) |
| `atelier-confirmation` : « Quitter » après changement : « Quitter sans valider ? » | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-confirmation.jpg?raw=true) |
| `atelier-garage-apres` : « Valider » : retour au garage, la 208 porte ses 5 autocollants | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-garage-apres.jpg?raw=true) |
| `atelier-garage-gt86` : GT86 au garage : étoile, lettre A, bande noire sur le capot, numéro à l'arrière | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-garage-gt86.jpg?raw=true) |
| `atelier-parcours-course` : parcours : la 208 en course de jour après Choisir → Rouler | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-parcours-course.jpg?raw=true) |
| `atelier-relance-garage` : après relance du jeu : habillage gardé | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-relance-garage.jpg?raw=true) |
| `atelier-course-gt86-jour` : course de jour, GT86 (numéro orange à l'arrière) | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-course-gt86-jour.jpg?raw=true) |
| `atelier-course-gt86-nuit` : course de nuit, GT86 | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-course-gt86-nuit.jpg?raw=true) |
| `atelier-course-p208-nuit` : course de nuit, 208 | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-course-p208-nuit.jpg?raw=true) |
| `atelier-en-atelier` : English: decal workshop | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-en-atelier.jpg?raw=true) |
| `atelier-en-confirmation` : English: « Leave without applying? » | — (nouvel écran) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-496/qa/pr/WFS-496/apres-atelier-en-confirmation.jpg?raw=true) |
