Agent : Claude, agent dev3d (01Us6uCvwS)

**Ticket** : WFS-546 — règles : `regles-de-gestion/WFS-546.md` (E17, zone E8, P1)

## Description

- **Demande de l'utilisateur** : « Créer le mode testeur : si je rentre dans Réglages et mode diagnostic, mettre un bouton qui permet d'ajouter 2500 étoiles pour chaque clic. » Précision (11:02:49 UTC) : « pas besoin de le faire en anglais et français, mettre un bouton avec une icône étoile, c'est tout ».
- **Ce qui change**
  - `src/ui/ecrans/reglages.cpp` : dans le menu caché de diagnostic (5 touchers sur « Réglages », WFS-419), un **bouton carré qui ne montre que l'icône étoile**, dans la rangée du haut à gauche de « Banc auto », même style `biseau` (RG-01). L'étoile est dessinée par `score_fin::dessinerEtoile`, comme celle du compteur du menu (WFS-537) ; le bouton n'a **aucun texte** (RG-05). Chaque appui appelle `boutique::crediter(s, 2500, "mode testeur")` puis `sauvegarde().enregistrer()` (RG-02, RG-03). Le bouton s'éclaire 0,4 s, et `crediter` écrit au journal technique la ligne `étoiles : +2500 ★ (mode testeur) : solde N ★` (RG-04). Rien d'autre ne change dans les Réglages, le menu ni le garage.
  - **Un appui = un crédit** : sur Android, un doigt posé qui bouge envoie d'autres événements « posé » (`ACTION_MOVE`, `down = vrai`) avec le même identifiant ; ils ne recréditent pas. Le doigt est libéré quand il se lève, quand le menu se ferme ou quand l'écran change. La logique seule est dans `src/ui/ecrans/reglages_etoiles.h` (`testeur::BoutonEtoiles`), sans dessin.
  - **Aucun fichier de textes touché** : `git diff origin/main -- assets/textes` est vide.
  - `tests/test_reglages_etoiles.cpp` (+ ligne `add_executable` dans `CMakeLists.txt`) (RG-06) : 3 appuis avec mouvements du doigt = 7500 ★, et la sauvegarde relue = 7500 ★. Il couvre aussi le doigt qui glisse depuis l'extérieur, le 2e doigt, le lever perdu, le lever d'un autre doigt, et 40 appuis = 100 000 ★.
  - Banc PC `WFS_BANC=testeur` (`src/ui/ecrans/reglages_testeur_banc.cpp`), avec une accroche d'une ligne dans `src/platform/desktop_main.cpp` (annoncée au journal) ; fonction de banc `boutonEtoilesDiag` dans `reglages.h`.
- **Comment c'est vérifié**
  - Tests CPU : 139 sur 139 verts (section ci-dessous).
  - Banc `testeur`, avec une vraie sauvegarde et de vrais touchers (posé, mouvements, levé) : 0 écart en 1280×720 et en 2400×1080. Le bouton est dessiné ; le 1er appui donne 2500 ★ une seule fois ; 2 appuis donnent 5000 ★ ; le fichier de sauvegarde relu contient 5000 ★ ; le compteur du menu affiche 5 000 ; au garage de la 208, le Chromé à 400 ★ s'achète et il reste 4 600 ★.
- **Limites et risques connus**
  - Comme décidé dans le ticket, le bouton reste dans la version Google Play, caché derrière les 5 touchers ; l'utilisateur dira avant la sortie publique s'il faut le retirer.
  - Les autres boutons du menu caché (interrupteurs, « Tout rétablir ») réagissent encore à chaque mouvement du doigt posé (comportement d'avant, hors périmètre) : signalé au PO.
  - Essai sur le téléphone de l'utilisateur après la fusion : ticket 🟡 À valider (utilisateur).

## Tests CPU

Journal complet : [`qa/pr/WFS-546/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/tests-cpu.log)

```
commit testé : b50b5b8a (branche feature/waforspeed-546, synchronisée avec main e60f217c)
[OK] test_version_chargement (0 s)
[OK] test_ville_lieux (0 s)
[OK] test_ville_sol (4 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 139 test(s) réussi(s) en 494 s
```

## Captures pour le lead dev

Dossier : [`qa/pr/WFS-546`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| Menu caché, français, 1280×720 (avant : sans bouton) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/avant-diagnostic-fr-1280x720.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-1-diagnostic-fr-1280x720.jpg?raw=true) |
| Menu caché, français, téléphone 2400×1080 : bouton étoile seul, à gauche de « Banc auto » | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-1-diagnostic-fr.jpg?raw=true) |
| Juste après le 2e appui : bouton étoile éclairé (téléphone) | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-2-etoile-eclairee.jpg?raw=true) |
| Juste après le 2e appui, 1280×720 | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-2-etoile-eclairee-1280x720.jpg?raw=true) |
| Menu principal après 2 appuis : compteur d'étoiles à 5 000 (téléphone) | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-3-menu-compteur.jpg?raw=true) |
| Menu caché en anglais, 1280×720 (avant : sans bouton) : même étoile, aucun texte | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/avant-diagnostic-en-1280x720.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-4-diagnostic-en-1280x720.jpg?raw=true) |
| Menu caché en anglais, téléphone | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-4-diagnostic-en.jpg?raw=true) |
| Garage de la 208 : Chromé (400 ★) en aperçu, solde 5 000 ★ (téléphone) | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-5-garage-chrome-apercu.jpg?raw=true) |
| Chromé acheté : cadenas tombé, solde 4 600 ★ (téléphone) | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-6-garage-chrome-achete.jpg?raw=true) |
| Chromé acheté, 1280×720 | — (pas de bouton ni de banc sur main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-546/qa/pr/WFS-546/apres-testeur-6-garage-chrome-achete-1280x720.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01Us6uCvwSLhHTdP5RNYXKda
