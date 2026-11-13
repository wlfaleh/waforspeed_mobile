Agent : Claude, agent dev3d (01Us6uCuDH)

**Ticket** : WFS-480 — règles : `regles-de-gestion/WFS-480.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : le joueur a une identité (pseudo, plaque, date d'arrivée) et un historique qui ne se perd pas ; ce ticket fait **les données et les règles, sans écran** (écrans : WFS-481 / 482, plaque 3D : WFS-483).
- **Ce qui change** :
  - `src/game/profil.h/.cpp` (nouveau) : RG-01 pseudo 3 à 16 caractères (A-Z a-z 0-9, espace simple, `-`, `_`), motifs « trop court / trop long / caractère interdit / mot interdit » ; RG-02 plaque 1 à 8 (A-Z 0-9 `-`, majuscules, pas de tiret au bord ni double) ; RG-03 plaque proposée d'après le pseudo, sinon `WFS-` + 3 chiffres ; RG-04 mots interdits (sans majuscules, sans accents, leet 0→o 1→i 3→e 4→a 5→s 7→t @→a $→s, séparateurs retirés, « contenu dans », liste blanche) ; RG-05 date d'arrivée écrite une fois (ancien joueur : 1er lancement de cette version, pseudo actuel proposé s'il n'est pas « Pilote ») ; RG-06 drapeau `profil=1` ; RG-07 statistiques ; RG-09 voiture préférée ; RG-10 20 dernières courses ; RG-11 clés dans la sauvegarde.
  - `assets/profil/mots_interdits.txt` (nouveau) : 191 mots FR / EN écrits pour le jeu (aucune liste extérieure recopiée), 39 mots de liste blanche (Scunthorpe, Classe, Bassin, Cockpit, Negresco…). Les mots très courts (con, cul, ass, nique…) ne sont pas listés : ils refuseraient Conor, Calcul, Technique…
  - `src/game/profil_system.cpp/.h` (nouveau, système 230) : relève la course en cours sans rien modifier : distance réelle (|Δ odo|, au mètre), vitesse max du compteur (`Course::maxKmh`), frôlements (`Course::nearCount`, même compteur que les messages du HUD) ; note la course à l'arrivée ou quand elle est quittée après le départ (retour au menu, course remise à zéro) ; rien si la sauvegarde est inactive (bancs).
  - Accroches d'**une ligne** à l'arrivée de chaque mode (résultat : score, place / temps, échappé / arrêté, gagné / perdu) : `course_system.cpp`, `course5_system.cpp`, `poursuite_system.cpp`, `duel_system.cpp` (2 lignes de plus pour les accolades) ; `poursuite_system.cpp` ne reçoit qu'1 ligne (WFS-460 y passera).
  - `sauvegarde.h/.cpp` : membre `Profil profil` ; accroches (défauts, écriture, lecture, 2 × date d'arrivée, 2 includes) ; pseudo **14 → 16** (`nettoyerPseudo` prend une limite ; les noms des adversaires du duel restent à 14). Aucune clé supprimée ; écriture déjà sûre (temporaire puis renommage). Le duel coupe toujours le pseudo envoyé à 14 (`duel::cleanName`), il ne le refuse jamais.
  - Banc `WFS_BANC=sauvegarde` : profil de test fixe (« Banc 480 », `WFS-480`) et le profil écrit / relu au journal.
  - `CMakeLists.txt` : `profil.cpp` ajouté aux 14 tests qui lient la sauvegarde ; nouveau test `test_profil`. `tests/test_sauvegarde.cpp` : la valeur attendue du pseudo trop long passe de 14 à 16 caractères (RG-01).
- **Comment c'est vérifié** :
  - `test_profil` (RG-12) : limites 2/3/16/17 et 0/1/8/9, caractères interdits, mots interdits avec majuscules, accents et leet, liste blanche, plaque par défaut, 3 courses simulées, 21 → 20 gardées, voiture préférée (égalité), écriture / relecture, sauvegarde d'avant le ticket (records gardés, `profil=0`), lignes mal formées ignorées.
  - **116 tests CPU verts** en local (`tools/qa/tests_cpu.sh build`), construction complète `wfs_bench` sans erreur, branche synchronisée avec `main`.
  - Banc `sauvegarde` lancé 2 fois : journal ci-dessous (`qa/pr/WFS-480/journal-profil.txt`) ; la course du banc est notée (Checkpoint, m4, 1450 pts, 0,295 km, 157 km/h) et relue au 2e lancement.
  - Captures avant / après : **identiques au pixel près** (0 pixel différent) : fin de course, sauvegarde relue et menu principal inchangés (rien à l'écran dans ce ticket).
- **Limites et risques connus** :
  - Accroches dans `sauvegarde.*` : 8 lignes ajoutées et 6 modifiées (2 includes, commentaires de la limite 14 → 16 et du pseudo, `nettoyerPseudo` avec sa limite) : un peu au-dessus de la limite de 10 lignes donnée par le PO ; tout le reste est dans `profil.*`.
  - Pas d'écran : la création du profil (WFS-481) utilisera `Profil::creer`, `plaqueProposee` et `Profil::pseudoPropose`.
  - Vérifié sur PC seulement (pas d'essai sur téléphone).
  - Pas de nouvelle clé « mode » pour le duel dans la sauvegarde : le mode d'une course est lu au départ (`duelSession().inRoom()` puis `modeCourse()`).

### Journal du banc `sauvegarde` (2 lancements)
```
profil : 191 mots interdits chargés
profil : course notée (Checkpoint, m4, 1450 pts, 0.295 km, 157 km/h, 0 frôlement(s)) : 1 course(s), 0.3 km au total
SAUVEGARDE écrite : car=m4 paint=rouge best=1450
PROFIL écrit : pseudo=Banc 480 plaque=WFS-480 profil=1 arrivée=2026-11-12 23:38:49 | courses=1 km=0.3 vmax=157 km/h frôlements=0 victoires=0 voiture préférée=m4 | 1 dernière(s) course(s) : Checkpoint, m4, 1450 pts, 2026-11-12 23:40:00
SAUVEGARDE relue : car=m4 paint=rouge best=1450
PROFIL relu : pseudo=Banc 480 plaque=WFS-480 profil=1 arrivée=2026-11-12 23:38:49 | courses=1 km=0.3 vmax=157 km/h frôlements=0 victoires=0 voiture préférée=m4 | 1 dernière(s) course(s) : Checkpoint, m4, 1450 pts, 2026-11-12 23:40:00
profil : 191 mots interdits chargés
```

## Captures pour le lead dev (4 après)

Dossier : [`qa/pr/WFS-480`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-480/qa/pr/WFS-480) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `menu-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-480/qa/pr/WFS-480/avant-menu-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-480/qa/pr/WFS-480/apres-menu-0.jpg?raw=true) |
| `menu-anim` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-480/qa/pr/WFS-480/avant-menu-anim.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-480/qa/pr/WFS-480/apres-menu-anim.jpg?raw=true) |
| `fin-record` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-480/qa/pr/WFS-480/avant-fin-record.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-480/qa/pr/WFS-480/apres-fin-record.jpg?raw=true) |
| `sauvegarde-relue` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-480/qa/pr/WFS-480/avant-sauvegarde-relue.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-480/qa/pr/WFS-480/apres-sauvegarde-relue.jpg?raw=true) |
