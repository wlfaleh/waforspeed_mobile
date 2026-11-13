Agent : Claude, agent dev3d (01Us6uCuD6)

**Ticket** : WFS-517 — règles : `regles-de-gestion/WFS-517.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : « c'est bizarre, dans le mode solo je trouve jouer avec des amis » : enlever la carte « Duel entre amis » de la Course solo, le duel restant au menu principal (« Duel en ligne »).
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - `src/game/modes_ecran.cpp` : `CARTES` passe à 3 entrées (`NB_CARTES = 3` : Checkpoint, Course à 5, Poursuite) ; la branche « Duel → salon » de `valider` est retirée (RG-01). Le bloc des 3 cartes garde la largeur des cartes (42 u au plus) et reste **centré** ; « Retour » et « Continuer » suivent les bords du bloc : pas de trou (RG-03). La manette déclare maintenant les cartes, puces, « Retour » (touche B) et « Continuer » ; le panneau ouvert efface les zones du menu dessous, comme les réglages (RG-03). Dernier mode = duel → Checkpoint présélectionné (RG-04, la règle existait déjà : commentée). `centre("carteN")` n'accepte plus que 0 à 2. `choixModeZone` (seulement utilisé par le banc du duel) est retiré.
  - `src/game/duel_system.cpp` : « Retour » de la fin du duel → **menu principal** (n'ouvre plus le panneau des modes) (RG-05).
  - `src/ui/ecrans/salon.cpp` (banc `benchDuelAmis`) : part de l'accueil → « Duel en ligne » → salon, y revient pour « Derniers adversaires » ; vérifie « Retour » → menu principal, panneau fermé ; nouvelle capture `duel-retour-menu` (RG-06).
  - Bancs qui recopiaient la mise en page à 4 cartes : `src/platform/chauffe_systeme.cpp` et `src/game/poursuite_system.cpp` lisent maintenant les zones sur le panneau (`choixModeCentre`) : accroches de quelques lignes, sans effet sur le jeu. `benchParcours` (modes_ecran.cpp) : étape 1 = « Duel en ligne » du menu → salon, contrôle des 3 cartes (plus de `carte3`), dernier mode duel → Checkpoint → garage ; capture `parcours-mode-3-cartes`.
  - `src/ui/ecrans/menu.cpp`, salon, relais et duel lui-même : **inchangés** (RG-02).
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) :
  - `bash tools/qa/tests_cpu.sh build` : tous verts.
  - Banc `parcours` (1280x720, avant et après) : « parcours complet : 0 écart(s) OK » et « en vue Cockpit : 0 écart(s) OK ». Il fait les deux parcours de RG-07 : accueil → « Duel en ligne » → salon ; accueil → Course solo → 3 cartes → Checkpoint → garage → terrain → course → fin → « Menu » → accueil.
  - Banc `menu2` (salon + duel entre amis, relais local) : « parcours duel : conforme (0 écart(s)) », avec « Duel en ligne » → salon, puis 2 manches, « Retour » → menu principal (panneau fermé), et « Derniers adversaires ».
  - Banc `chauffe` : parcours Checkpoint → garage → course → menu, 0 écart.
  - Banc `course5` : parcours de la Poursuite par la carte 03 « accueil → … → poursuite → fin → Menu → accueil : 0 écart(s) OK » ; redémarrage : « le panneau des modes rouvre sur Poursuite ».
  - Après la dernière synchronisation avec `main` (0bea2f53) : 116 tests CPU verts, parcours, menu2 et course5 relancés, mêmes résultats.
  - Lignes « ERREUR » des bancs qui ne viennent pas de ce ticket : « 2 pipeline(s) créé(s) pendant la course » (menu2) apparaît aussi sur `main` (captures « avant ») ; « aucune police à 10-30 m pour poursuite-police-basse » (course5) vient d'une scène fixe de la poursuite, que ce ticket ne touche pas.
  - `test_langue` ne se construit plus sur `main` depuis la fusion de WFS-480 (`profil.cpp` manque à la cible) : ce n'est pas lié à ce ticket. Le PO l'a signalé et a créé l'US technique **WFS-526**.
- **Limites et risques connus** :
  - Le banc du duel trouve « Duel en ligne » comme 2e zone de focus du menu (même méthode que l'endurance pour « Course solo », qui est la 1re). Si l'ordre des boutons du menu change un jour, ce banc échouera en le disant (« bouton absent » ou mauvais écran), sans rien casser en silence.
  - Pas d'essai manette sur un vrai téléphone. La manette appuie déjà au centre des zones déclarées : c'est le même principe que les réglages.
  - Écran des modes en anglais : WFS-505, hors périmètre.

## Captures pour le lead dev (6 après, 4 avant)

Dossier : [`qa/pr/WFS-517`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-517/qa/pr/WFS-517) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `duel-retour-menu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-517/qa/pr/WFS-517/apres-duel-retour-menu.jpg?raw=true) |
| `salon-amis-fin` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-517/qa/pr/WFS-517/avant-salon-amis-fin.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-517/qa/pr/WFS-517/apres-salon-amis-fin.jpg?raw=true) |
| `salon-amis` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-517/qa/pr/WFS-517/avant-salon-amis.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-517/qa/pr/WFS-517/apres-salon-amis.jpg?raw=true) |
| `salon-derniers` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-517/qa/pr/WFS-517/avant-salon-derniers.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-517/qa/pr/WFS-517/apres-salon-derniers.jpg?raw=true) |
| `parcours-mode-3-cartes` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-517/qa/pr/WFS-517/apres-parcours-mode-3-cartes.jpg?raw=true) |
| `parcours-mode` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-517/qa/pr/WFS-517/avant-parcours-mode.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-517/qa/pr/WFS-517/apres-parcours-mode.jpg?raw=true) |
