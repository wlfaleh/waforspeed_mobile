## Agent : Claude, agent dev3d (01Us6uCv3H)

**Ticket** : WFS-482 — règles : `regles-de-gestion/WFS-482.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : un écran « Profil » où le joueur voit son identité et son historique (chiffres, records, voiture préférée, 20 dernières courses) et peut changer son pseudo et sa plaque (épic E17, `regles-de-gestion/WFS-482.md`).
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - nouveau `src/ui/ecrans/profil.h/.cpp` (zone E8) : système « ProfilEcran » (ordre 468), panneau plein écran au style des Crédits : en-tête (pseudo, aperçu de la plaque de WFS-481, « Pilote depuis le 12 nov. 2026 »), 5 chiffres (courses, km à 1 décimale, vitesse max, frôlements, victoires), records des 3 modes (mêmes textes qu'à la fin de course, « — » si vide), voiture préférée (nom du garage, sans marque) et ses km, liste des 20 dernières courses qui défile au doigt (mode, voiture, résultat, date ; « Aucune course pour l'instant »), boutons « Modifier » (orange) et « Retour » (vitré) ;
  - RG-01 : bouton « PROFIL » au menu, au-dessus de « Cadeau du jour » ; « Retour » et la touche Retour d'Android (zone retour de la manette) ramènent au menu ;
  - RG-07 : « Modifier » rouvre **le même formulaire et le même clavier** que WFS-481 (`creation_profil.cpp`, mode modification : titre « Modifier ton pilote », « Valider » + « Annuler » ; la plaque actuelle ne suit plus le pseudo) ; « Valider » écrit tout de suite (clé `nick`, donc aussi le duel), « Annuler » et la touche Retour d'Android ne changent rien ;
  - textes `profil.*` en français et en anglais (`assets/textes/fr.txt`, `en.txt`, même ligne) ;
  - **accroches annoncées au journal** : `menu.cpp` (7 lignes : include, bouton, touchers, dessin), `compteur_etoiles.cpp` (2 lignes : compteur caché sous le Profil), `desktop_main.cpp` (2 lignes : banc `WFS_BANC=profil`).
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) :
  - 140 tests CPU verts (`tools/qa/tests_cpu.sh`), dont `test_creation_profil` (nouveau cas : formulaire en modification) et `test_langue` (clés FR / EN) ;
  - banc `WFS_BANC=profil` (FR puis EN, 0 écart) : profil neuf ; Retour d'Android → menu ; parcours **menu → Profil → Modifier → nouveau pseudo → Valider → Retour → course → fin → Profil** (la course en tête de liste, km et vitesse max augmentés) ; « Annuler » et Retour d'Android pendant la modification ne changent rien ; **profil plein : 22 courses → 20 lignes**, défilement au doigt ; **pseudo de 16 « W » dessiné en entier** ; en anglais : 0 texte français, 0 texte qui dépasse (RC-E20-06) ;
  - banc `creation` de WFS-481 : 0 écart avant comme après (aucune régression de l'écran « Crée ton pilote »).
- **Limites et risques connus** :
  - pas de capture « avant » pour le banc `profil` (il n'existe pas sur `main` : erreur attendue du script) ;
  - nombres au format français dans les deux langues (« 79,9 », comme le reste du jeu : formats des nombres hors périmètre de E20) ;
  - date en heure locale du téléphone (banc : UTC) ; la plaque 3D change à la course suivante (WFS-483, hors ticket) ;
  - non essayé sur téléphone (pas d'APK de branche) : à regarder sur l'APK de `main` après la fusion.

## Tests CPU

`bash tools/qa/tests_cpu.sh build` sur la branche synchronisée avec `main` : **140 test(s) réussi(s) en 335 s**, 0 échec — journal complet : [`qa/pr/WFS-482/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/tests-cpu.log).

## Captures pour le lead dev (26 après)

Dossier : [`qa/pr/WFS-482`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `creation-fr-court` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/avant-creation-fr-court.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-creation-fr-court.jpg?raw=true) |
| `creation-fr-installe` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/avant-creation-fr-installe.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-creation-fr-installe.jpg?raw=true) |
| `creation-fr-interdit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/avant-creation-fr-interdit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-creation-fr-interdit.jpg?raw=true) |
| `creation-fr-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/avant-creation-fr-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-creation-fr-menu.jpg?raw=true) |
| `creation-fr-rapide` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/avant-creation-fr-rapide.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-creation-fr-rapide.jpg?raw=true) |
| `creation-fr-rempli` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/avant-creation-fr-rempli.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-creation-fr-rempli.jpg?raw=true) |
| `creation-fr-repetition` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/avant-creation-fr-repetition.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-creation-fr-repetition.jpg?raw=true) |
| `creation-fr-vide` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/avant-creation-fr-vide.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-creation-fr-vide.jpg?raw=true) |
| `profil-en-16w-modification` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-en-16w-modification.jpg?raw=true) |
| `profil-en-16w` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-en-16w.jpg?raw=true) |
| `profil-en-apres-course` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-en-apres-course.jpg?raw=true) |
| `profil-en-apres-modification` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-en-apres-modification.jpg?raw=true) |
| `profil-en-menu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-en-menu.jpg?raw=true) |
| `profil-en-modification` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-en-modification.jpg?raw=true) |
| `profil-en-neuf` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-en-neuf.jpg?raw=true) |
| `profil-en-plein-defile` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-en-plein-defile.jpg?raw=true) |
| `profil-en-plein` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-en-plein.jpg?raw=true) |
| `profil-fr-16w-modification` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-fr-16w-modification.jpg?raw=true) |
| `profil-fr-16w` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-fr-16w.jpg?raw=true) |
| `profil-fr-apres-course` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-fr-apres-course.jpg?raw=true) |
| `profil-fr-apres-modification` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-fr-apres-modification.jpg?raw=true) |
| `profil-fr-menu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-fr-menu.jpg?raw=true) |
| `profil-fr-modification` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-fr-modification.jpg?raw=true) |
| `profil-fr-neuf` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-fr-neuf.jpg?raw=true) |
| `profil-fr-plein-defile` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-fr-plein-defile.jpg?raw=true) |
| `profil-fr-plein` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-482/qa/pr/WFS-482/apres-profil-fr-plein.jpg?raw=true) |
