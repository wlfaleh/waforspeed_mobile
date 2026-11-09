Agent : agent expert ci cd

**Demande de l'utilisateur** : la CI cuit les paquets de la Ville et les publie dans la Release GitHub `paquets` avec `cartes.json`, l'adresse étant à un seul endroit (« Valider », 2026-11-08), pour l'essai réel du téléchargement (WFS-306) sur le téléphone.

## Ce qui change
- `.github/workflows/paquets.yml` (neuf, lancé à la main sur `main`, ou appelé par un autre workflow) :
  - job `cuisson` ×2 (deux machines, deux compilations) : `WFS_PAQUETS=paquets WFS_BANC=cuire ./build/wfs_bench` (mode de WFS-301, PR #290), 3 fichiers attendus ;
  - job `publication` : test de l'outil, puis `tools/qa/publier_paquets.sh`.
- `tools/qa/publier_paquets.sh` (neuf) : RG-01 les deux cuissons ont les mêmes fichiers, identiques octet par octet, sinon rouge et rien n'est publié ; RG-02 Release fixe `paquets` (créée si absente, jamais supprimée, `GITHUB_TOKEN` seul) ; RG-03 `cartes.json` ; ordre : paquets d'abord, `cartes.json` ensuite, puis retrait des paquets qui ne sont ni dans la nouvelle liste ni dans la précédente ; mêmes paquets que la liste publiée → rien n'est envoyé ; RG-05 résumé du run. Hors de `main` : vérification seulement.
- `tools/qa/cartes_json.py` + `tools/qa/test_cartes_json.py` (neufs) : lit et vérifie chaque en-tête `.wfcarte` (magie, version du format, CRC de l'en-tête et de chaque bloc, carte et format = nom du fichier), écrit la liste ; 18 contrôles.
- **Format de `cartes.json`** : celui que lit le jeu (`ListePaquets.java`, WFS-306 : `version`, `paquets[]` avec carte, nom, format, version_carte, empreinte, version_min_apk, taille, sha256, adresse), comme prévu par `regles-de-gestion/WFS-306.md` (« WFS-302 le reprend tel quel »), plus les champs de RG-03 que le jeu ignore : `version_liste`, `carte_par_defaut` = `ville`, `version_format`, `commit`. `version_min_apk` = 927 (1er APK avec WFS-301, #278).
- **Noms publiés versionnés** : `ville_astc-<12 chiffres du SHA-256>.wfcarte` ; un contenu nouveau a une adresse nouvelle, donc la liste ne montre jamais un fichier remplacé entre-temps. Le téléphone range toujours le fichier sous `ville_<format>.wfcarte` (`Paquets.java`), rien à changer côté jeu (confirmé avec agent 3).
- RG-04 : l'adresse de la liste est dans `configgame.json` → `paquets.adresse_liste` (WFS-306, déjà sur `main` ; même valeur par défaut dans `configgame.h`). Aucun fichier du jeu, de `build.yml` ni de `matin.yml` n'est changé ici.

## Comment c'est vérifié (sur la branche synchronisée avec `main`, a790c046)
1. `python3 tools/qa/test_cartes_json.py` : tout est vert (3 formats, ordre, taille, SHA-256, adresse directe et versionnée, stabilité octet par octet ; refus : bloc abîmé, en-tête abîmé, format futur, nom faux, fichier tronqué, dossier vide, adresse non https).
2. Deux cuissons réelles `WFS_BANC=cuire` (code 0 toutes les deux) :
    [info] cuire (WFS-301) : ville_astc.wfcarte : 22779180 octets, sha256 843a6e9c57c46e09e4f8b05dbaab520be69f0ec0e0a701117c1ec14ce03a3c7a, empreinte c30f0f40c276b6c1
    [info] cuire (WFS-301) : ville_etc2.wfcarte : 5562514 octets, sha256 f8e3c56c2a9e1851416530e0165d98a220fe737a28b1f0b2d6ab8142d2946055, empreinte c30f0f40c276b6c1
    [info] cuire (WFS-301) : ville_rgba8.wfcarte : 27593777 octets, sha256 f982838af5a22c2402d888b89107a4766bf3a44f727b0b930321bc27233cbf7a, empreinte c30f0f40c276b6c1
    [info] cuire (WFS-301) : 0 écart(s) OK, paquets dans /tmp/claude-0/-home-user-waforspeed-mobile/070ef7de-37b2-5d16-98f9-06f94090b427/scratchpad/ca
3. `publier_paquets.sh` en vérification seulement : « Deux cuissons identiques octet par octet : 3 fichier(s) », code 0 ; avec un octet changé dans une cuisson : `::error::deux cuissons différentes : ville_etc2.wfcarte (RG-01)`, code 1, rien publié.
4. La vraie liste lue par `ListePaquets.choisir` du jeu (javac, empreinte `c30f0f40c276b6c1`) :
    astc -> astc 22779180 https://github.com/wlfaleh/waforspeed_mobile/releases/download/paquets/ville_astc-843a6e9c57c4.wfcarte
    etc2 -> etc2 5562514 https://github.com/wlfaleh/waforspeed_mobile/releases/download/paquets/ville_etc2-f8e3c56c2a9e.wfcarte
    rgba8 -> rgba8 27593777 https://github.com/wlfaleh/waforspeed_mobile/releases/download/paquets/ville_rgba8-f982838af5a2.wfcarte
    apk 926 -> null
5. Résumé du run tel qu'il sera affiché : `qa/pr/WFS-302/resume-run.md` ; liste produite : `qa/pr/WFS-302/cartes-exemple.json`.

**Captures** : ticket de CI sans changement à l'écran ; les preuves sont les journaux ci-dessus (l'écran de téléchargement est celui de WFS-306, déjà relu).

## Limites et risques
- La Release `paquets` n'existe pas encore : le 1er lancement de `paquets.yml` sur `main` (feu vert de l'utilisateur donné) la crée, juste après la fusion. Jusque-là le jeu reçoit 404 → secours silencieux (Ville du code), comme prévu par WFS-306.
- Le paquet ETC2 ne contient pas de textures (aucune texture ETC2 dans le dépôt) : 5,3 Mo ; un téléphone ETC2 lit ses textures dans l'APK.
- Pas encore branché sur `matin.yml` (PO : après la fusion du mode `cuire` ; ce sera une suite, avec validation de l'utilisateur) : la liste est republiée à la main quand la carte change ; un paquet d'une ancienne génération est refusé par l'empreinte → Ville du code, jamais de plantage.
- Critère RG-06 (téléchargement réel sur le téléphone, redirection et reprise) : après publication, par l'utilisateur → 🟡 À valider.
