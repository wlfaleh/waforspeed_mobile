Agent : Claude, agent dev3d (01Us6uCvwS)

**Ticket** : WFS-546 — règles : `regles-de-gestion/WFS-546.md` (E17, zone E8, P1)

## Description

- **Demande de l'utilisateur** : « Créer le mode testeur : si je rentre dans Réglages et mode diagnostic, mettre un bouton qui permet d'ajouter 2500 étoiles pour chaque clic. »
- **Ce qui change**
  - `src/ui/ecrans/reglages.cpp` : dans le menu caché de diagnostic (5 touchers sur « Réglages », WFS-419), bouton **« +2500 ★ »** dans la rangée du haut, à gauche de « Banc auto », même style `biseau` (RG-01). Chaque appui appelle `boutique::crediter(s, 2500, "mode testeur")` puis `sauvegarde().enregistrer()` (RG-02, RG-03). Le bouton s'éclaire 0,4 s (orange clair) et **« Solde : 12 500 ★ »** s'affiche à sa gauche (clé existante `garage.solde`, nombre avec `garage::nombreLangue`, ★ dessinée par `garage::texteEtoiles`) (RG-04). Rien d'autre ne change dans les Réglages, le menu ni le garage.
  - **Un appui = un crédit** : sur Android, un doigt posé qui bouge envoie d'autres événements « posé » (`ACTION_MOVE`, `down = vrai`) avec le même identifiant ; ils ne recréditent pas. Le doigt est libéré quand il se lève, quand le menu se ferme ou quand l'écran change. Logique seule dans `src/ui/ecrans/reglages_etoiles.h` (`testeur::BoutonEtoiles`), sans dessin.
  - `assets/textes/fr.txt`, `en.txt` : clé `diag.etoiles` = « +2500 ★ » (RG-05).
  - `tests/test_reglages_etoiles.cpp` (+ ligne `add_executable` dans `CMakeLists.txt`) : 3 appuis avec mouvements du doigt = 7500 ★, sauvegarde relue = 7500 ★ (RG-06) ; doigt qui glisse depuis l'extérieur, 2e doigt, lever perdu, lever d'un autre doigt, 40 appuis = 100 000 ★.
  - Banc PC `WFS_BANC=testeur` (`src/ui/ecrans/reglages_testeur_banc.cpp`) + accroche d'une ligne dans `src/platform/desktop_main.cpp` (annoncée au journal) ; fonction de banc `boutonEtoilesDiag` dans `reglages.h`.
- **Comment c'est vérifié**
  - Tests CPU : `bash tools/qa/tests_cpu.sh build` : 139 tests, tous verts (`test_audio_chocs` a échoué une fois sur sa mesure de temps RG-07 pendant que la compilation de `main` tournait à côté, puis passe seul : sans lien avec ce ticket).
  - Banc `testeur` (vraie sauvegarde, vrais touchers posé / mouvements / levé), 0 écart en 1280×720 et en 2400×1080 : bouton dessiné ; 1er appui = 2500 ★ une seule fois ; 2 appuis = 5000 ★ ; fichier de sauvegarde relu = 5000 ★ ; compteur du menu à 5 000 ; garage de la 208 : Chromé 400 ★ acheté, solde 4 600 ★.
- **Limites et risques connus**
  - Comme décidé dans le ticket, le bouton reste dans la version Google Play, caché derrière les 5 touchers ; l'utilisateur dira avant la sortie publique s'il faut le retirer.
  - Les autres boutons du menu caché (interrupteurs, « Tout rétablir ») réagissent encore à chaque mouvement du doigt posé (comportement d'avant, hors périmètre) : signalé au PO.
  - Essai sur le téléphone de l'utilisateur après la fusion : ticket 🟡 À valider (utilisateur).

## Captures pour le lead dev

Dossier : [`qa/pr/WFS-546`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| Menu caché, français, 1280×720 (avant : sans bouton) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/avant-diagnostic-fr-1280x720.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-1-diagnostic-fr-1280x720.jpg?raw=true) |
| Menu caché, français, téléphone 2400×1080 : « +2500 ★ » à gauche de « Banc auto », solde 0 ★ | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-1-diagnostic-fr.jpg?raw=true) |
| Après 2 appuis (posé + 2 mouvements + levé, chacun) : « Solde : 5 000 ★ », bouton éclairé (téléphone) | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-2-solde-5000.jpg?raw=true) |
| Après 2 appuis, 1280×720 | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-2-solde-5000-1280x720.jpg?raw=true) |
| Menu principal : compteur d'étoiles à 5 000 (téléphone) | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-3-menu-compteur.jpg?raw=true) |
| Menu caché en anglais, 1280×720 (avant : sans bouton) : « Balance: 5,000 ★ » | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/avant-diagnostic-en-1280x720.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-4-diagnostic-en-1280x720.jpg?raw=true) |
| Menu caché en anglais, téléphone | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-4-diagnostic-en.jpg?raw=true) |
| Garage de la 208 : Chromé (400 ★) en aperçu, solde 5 000 ★ (téléphone) | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-5-garage-chrome-apercu.jpg?raw=true) |
| Chromé acheté : cadenas tombé, solde 4 600 ★ (téléphone) | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-6-garage-chrome-achete.jpg?raw=true) |
| Chromé acheté, 1280×720 | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-6-garage-chrome-achete-1280x720.jpg?raw=true) |
