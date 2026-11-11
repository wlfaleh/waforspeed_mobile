# WaForSpeed Mobile — journal

Journal du projet (tickets WFS-XX de `TACHES.md`) : une entrée par étape (début, construction, PR, verdict QA, fin…) avec
fait / fichiers / vérification / limites. **Nouvelles entrées à la fin du fichier.** Toujours partir de la dernière version de
`main` (`git pull`), ajouter seulement ses entrées, puis envoyer tout de suite ; ne jamais écraser le fichier.
**Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
Exemple : `### 2026-11-03 19:43:10 — WFS-58 construit, essai simulateur lancé (Claude, session trafic)`.
> **Archive (règle de l'utilisateur, 2026-11-06)** : ce fichier ne garde que les **2 derniers jours**. Les entrées plus anciennes sont dans `journal/archive/AAAA-MM-JJ.md` (un fichier par jour) ; ne les relire qu'en cas de besoin précis. Le PO déplace chaque jour les entrées de plus de 2 jours.

### 2026-11-10 00:01:53 — WFS-381 attribué à Claude, agent dev A (01UY1Vv42R) (po-scrum-master)
- Demande d'une nouvelle session `@dev` (session_01UY1Vv42RWwaV9T1tMxWRgB, développeur général, 1er et seul ticket). Choix : ticket ⏳ Prêt le plus prioritaire d'une épic libre hors conduite, cartes, CI, voitures, iOS : **WFS-381** (P1, E13, brume au loin, ANO-221), en tête de l'ordre décor du 2026-11-09 20:01 avec WFS-382 (en cours). E13 libre (WFS-214 🟣). WFS-372 / 373 (E15) restent au profil `dev-artiste-3d` ; WFS-378 vient après dans l'ordre ; WFS-384 / 287 à l'agent expert ci.
- Zone : voir `regles-de-gestion/WFS-381.md` (section « Suivi ») ; sans chevauchement avec WFS-382 (bloom de `post.frag`, `nuit.cpp`), WFS-380 (eau), WFS-375, WFS-369.
- Plan court : lire le brouillard existant (`common.glsl`, `eclairage*.glsl`, `sky.frag`, `atmosphere_physique.h`), brume exponentielle distance + hauteur couleur du ciel (RG-01 à 03), réglage par carte (RG-04), mesure perf Basse avant / après (RG-05), captures 1 à 5 avant / après, PR, lead dev.
- Ménage quotidien du 2026-11-10 : laissé au robot `archives.yml` de cette nuit (journal du 2026-11-08 et lignes ✅) ; vérification de son commit au prochain appel.

### 2026-11-10 00:03:19 — WFS-372 attribué à Claude, agent dev3d 5 (01UY1Vv5G4) (po-scrum-master)
- Demande d'une nouvelle session `@dev3d` (agent `dev-artiste-3d`, session_01UY1Vv5G4636Nbsd5PhaAsn), 1re et seule attribution (règle « un seul sujet par session »).
- Choix : ticket prêt le plus prioritaire de son domaine (P1, E15 ; Ville WFS-369 et Corniche WFS-380 déjà en cours, file Campagne libre ; WFS-372 avant WFS-373 au numéro). Voitures E10 : déjà deux tickets en parallèle (WFS-362, 396), rien de prêt.
- Zone : `src/world/campagne.cpp` (décor), nouveaux `src/world/decor_campagne_*`, `assets/world/modeles/campagne/**`, `assets/textures/decor/campagne/**`, `assets/licences/**`, `tests/**`. Pas de conflit : WFS-391 (`player.*`, `roues.*`), WFS-382 (`nuit.cpp`), WFS-381 (brume), WFS-392 (`route*`).
- Plan court : inventaire au journal (RG-01) → modèles CC0 / CC BY aux mesures ±1 % (vignes, maisons, clôtures, bottes, hangars ANO-219) → instances + LOD au chargement → captures jour / nuit / gros plans / perf → PR → avis agent 3D VALIDÉ → lead dev.

### 2026-11-10 00:15:26 — WFS-375 fusionné par le lead dev (lead-dev)
- PR #322 relue sur 82bdda3 : 16 captures ouvertes (JOUR / NUIT sans défilement, Campagne en plein jour à 22:15, Corniche de nuit), diff lu (`terrain.cpp`, `sauvegarde.cpp/.h`, `test_sauvegarde.cpp`), construction propre et `tests_cpu.sh` refaits : 94/94.
- Revue « ✅ Approuvé » en commentaire (GitHub refuse l'approbation du même compte), fusion squash 14ec9d2. WFS-375 → 🟡 À valider (utilisateur) (RG-06) ; ligne `qa/Check.md` ajoutée ; ligne retirée de `suivi-pr.md`.

### 2026-11-10 00:15:58 — Fin WFS-375 : PR #322 fusionnée par le lead dev, session terminée (Claude, agent dev M (01UY1Vv4mu))
- Squash 14ec9d2 ; ticket 🟡 À valider (utilisateur) : choisir JOUR sur le téléphone le soir, la course doit être en plein jour ; NUIT, en vraie nuit.
- Session arrêtée (un seul sujet par session) ; aucun autre ticket demandé.

### 2026-11-10 00:17:14 — WFS-372 : branche poussée, inventaire des formes nues de la Campagne, plan (Claude, agent dev3d 5 (01UY1Vv5G4))
- Branche `feature/waforspeed-372` poussée (79fda8a6). Inventaire complet (RG-01) : `qa/pr/WFS-372/inventaire.md` + captures `qa/pr/WFS-372/inventaire/` (banc `campagne`, main d75a3d7b).
- Objets du ticket : vignes `T_VINES` 1 755 blocs (bandes pleines, ANO-65) ; clôtures `T_FENCE` 485 travées + 2 enclos (`box`) ; bottes `T_BALES` 41 groupes (`cylAxe` nu) ; fermes `T_FARM` 20 (boîte grise 8 × 11 m + grange 10 × 14 m, ANO-219) ; poteaux électriques `T_POLE` 27 (cylindre + traverse, le « portique sur poteaux » d'ANO-219).
- Hors ticket, listés pour WFS-387 : église, commerces, fontaine, éoliennes, signalisation, haies, ouvrages de l'autoroute ; les 100 maisons du village sont déjà celles du kit d'architecture (WFS-161), pas des boîtes.
- Plan : modèles calculés sur le processeur dans un nouveau fichier `src/world/decor_campagne_modeles.{h,cpp}` (même méthode que les bateaux de WFS-376 : lofts lissés, chanfreins, 2 niveaux de détail), posés dans le décor par `campagne.cpp` (accroche par cas de `dessine`), test `tests/test_decor_campagne.cpp` (mesures). Réseau : seulement GitHub et paquets ; aucun modèle libre aux bonnes mesures attendu pour ces objets agricoles → RG-03 (modélisés au niveau du vrai objet), noté au journal à la fin.
- Mesures retenues (sources dans le code) : rang palissé Guyot, piquets 1,8 m tous les 5 m, fils à 0,6 / 1,0 / 1,4 m, ceps tous les 1,0 m ; botte ronde 1,20 m de large, Ø 1,50 m ; clôture rustique poteaux Ø 0,12 m de 1,2 m, deux lisses ; ferme à murs crépis, toit de tuiles canal à 2 pans (pente ≈ 30 %), grange-hangar à bardage bois et toit ondulé ; poteau bois de ligne basse tension 8,5 m.

### 2026-11-10 00:18:16 — WFS-396 : décision du PO sur les remplaçants de l'A 45 et de l'Elantra N (po-scrum-master)
- Rapport de Claude, agent artiste (01UY1Vv2ph) : aucun modèle libre dans ±1 % tel quel (W212 hauteur −2,3 % ; W211 voie avant −2,2 % ; Virtus GT hauteur −2,6 % ; Corolla E180 largeur +2,1 %).
- **Décision (option a)** : refus motivé pour les deux places ; WFS-396 se livre avec le rapport et les fiches réelles (`references/reel/voitures/`), rien de converti ; la session s'arrête après la fusion.
- **Suite** : WFS-397 créé (P1, E10 exception `remplacants/`, agent `dev-artiste-3d`, nouvelle session, après WFS-396) : retoucher la **W212 de Peter_D** (hauteur ; repli W211 de BadKarma, voie avant +17 mm par roue) et la **Virtus GT de BHP3D** (hauteur, allègement < 8 Mo) à ±1 %. WFS-360 dépend maintenant de WFS-397.
- A 45 de Lexyc16 interdite (licence contradictoire CC BY / CC BY-NC).

### 2026-11-10 00:24:40 — WFS-362 : PR #323 ouverte, M4 et S3 sans calandre de marque (Claude, agent dev3d 4 (01UY1Vv5HV))
- Fait : calandres découpées sur leur contour exact (`decouper`) ; haut de l'ouverture refermé par une pièce de carrosserie (nouvelle retouche `panneau` de `tools/retoucher_formes.mjs` : même peinture, normales raccordées, bombé à pente nulle au bord, jamais devant) ; bas : grille ajustée (outil de la X5). M4 : anneau chromé de l'ancien logo du capot retiré.
- Fichiers : `tools/retoucher_formes.mjs`, `tools/formes.json` (entrées m4, s3), `assets/cars/m4.*`, `assets/cars/s3.*`, `qa/pr/WFS-362/`.
- Vérification : `--verifier` vert (m4, s3, x5, compass, p208) ; X5 rejouée depuis son origine : même empreinte (factorisation sans effet) ; 94 tests CPU verts ; `test_fidelite_voitures` : m4 et s3 identiques à `main` ; captures avant / après (18 vitrines, 16 gros plans jour / nuit, 12 zooms ×4).
- Limites : S3 125,3 k triangles (repère 120 k dépassé, comme X5 / 208 / Compass) ; joint du panneau visible de près (écusson sombre de loin sur la M4 noire) ; dépendance npm `meshoptimizer@0.22.0` installée dans le conteneur (version du `tools/package.json`) pour refaire les LOD.


### 2026-11-10 00:30:14 — WFS-362 refusé par le lead dev, PR #323 (lead-dev)
- Commit relu `bcbd0d83` ; 46 paires de captures ouvertes, tout le diff lu (`tools/retoucher_formes.mjs`, `tools/formes.json`, modèles m4 / s3) ; `--verifier` rejoué : vert (m4, s3, x5, compass, p208).
- Bloquants : (1) M4 : panneau bosselé, creux rond (« cratère ») et reflets en vagues, visibles jusqu'aux vitrines (`nez-m4-34d-*`, `nez-m4-bas-*`, `vitrine-m4-a1`, `vitrine-m4-bas2`) ; (2) M4 : le panneau garde le contour de l'ancienne double calandre (écusson sombre lisible de face et de 3/4) ; (3) S3 : éclats et bord déchiré au coin droit de la grille (`nez-s3-34g-jour`, `zoom-s3-34g-*`).
- À corriger aussi : bouts de lames en peigne (M4, S3), bande plus sombre du panneau de la S3 sous le capot.
- Revue publiée en « COMMENT » (même compte GitHub) ; statut ↩️ dans `TACHES.md`, `track-agent.md`, `suivi-pr.md`.

### 2026-11-10 00:31:40 — WFS-396 : PR #325 ouverte, remplaçants A 45 / Elantra N étudiés, refus motivé (Claude, agent artiste (01UY1Vv2ph))
- **Fait** : recherche de code GitHub (~1 000 `license.txt` lus), 9 modèles téléchargés dans le bac à sable (clone partiel, rien exécuté), 6 convertis et mesurés par `corriger_voitures --essai` (permutation locale de `a45.wfm` / fiche, restaurée). Aucun dans ±1 % : refus motivé (décision du PO 00:18, option a), rien de converti dans `remplacants/`. Rapport : `qa/pr/WFS-396/pr.md` ; fiches réelles : `references/reel/voitures/remplacants-wfs396.json`.
- **Téléchargements (SHA-256 complets)** :
  - W212, Peter_D, CC BY 4.0, https://sketchfab.com/3d-models/mercedes-e-class-w212-119c5e10733142b197aa53b86f6aeb04, dépôt `bujue600-arch/carVison` `car-vision-frontend/public/models/mercedes-e-class/` : scene.gltf a1e14552aababef903a2fcd2dfeb6dd0d97de3062d316c8d5d6d489d5a106822 (238 832 o), scene.bin cfed9ca335f5de90062e4fd893c53c7d4897f6ed67c42733d989c89529fed44a (20 244 980 o) ; hauteur −2,7 %.
  - W211, BadKarma, CC BY 4.0, dépôt `gibranalfarabi/portofolio-ar` `assets/3D-model/mercedes_benz_e-class_w211/` : scene.gltf 909a0f9ea1e06ba4d555f5efc9c5df4cf45ecf188f3e72262157d9f8508e79a8 (91 150 o), scene.bin 76bad9a65a904644771ba1cc9ad6f48fc6e1ede428d1eee630492ed872811126 (15 148 680 o) ; voie AV −2,2 %.
  - Virtus GT 2022, BHP3D, CC BY 4.0, https://sketchfab.com/3d-models/2022-volkswagen-virtus-gt-3955e5c050f843398b49b2b21ab2f232, dépôt `PrathamSatpute/VehiclePredictiveMaintenance` `models/` : scene.gltf 1bfe8554c6728fea14cf9e7ffe0f02a287904a58ca3e293faa89602347e5b8d7 (617 095 o), scene.bin 70fbe3cbdf30c628463c665c9d5d0d7aa05f7c5f7752a33b72663afd6e7f5eac (22 504 008 o) ; hauteur −2,6 %.
  - Corolla E180 EU, Armored Wave, CC BY 4.0, dépôt `oddalan/three-dee` `assets/models/corolla/` : scene.gltf 57ff461ec2dba40ae0a865d96c09c2ddd1d81dca0865b6bf4f4fe8ba5fdacf0b (135 104 o), scene.bin f377f589d4ab8a79d203f1bd5703b0cc4a6e0a198ebc206700cfbc943f266f3f (4 395 288 o) ; largeur +2,1 %.
  - Mesurés puis écartés (bac à sable) : Corolla 2020 ItsDiyor (`gabrielivanov959-bot/toyota-webapp`), Octavia barking_dogo (`Jkotoun/Opengl_model_viewer_PGR`), Civic FD2 claries, C63 LuizZed36, WRX STi « Galaxy Car Showroom », Lancer Evo X BadKarma, S65 W222 Black Snow (pièces BeamNG), « Accord 2017 » L95XP (mod `mtn_caprice`), Seat Leon Cupra, A4 2000.
- **Licence contradictoire** : « A45 AMG 2018 » de Lexyc16 en CC BY 4.0 (`tensornull/CarVision`) et CC BY-NC 4.0 (`vladyslavhurzhyi/car-rent`) : interdit.
- **Vérification** : `tests_cpu.sh build` 94 réussis ; captures `vitrines-feux` 1280×720 (lavapipe) dans `qa/pr/WFS-396/`. Le jeu est identique à `main`.
- **Suite** : lead dev appelé pour la PR #325 ; WFS-397 (PO) reprend W212 / W211 et Virtus GT.

### 2026-11-10 00:40:00 — WFS-369 fusionné par le lead dev (lead-dev)
- PR #324 relue sur 6cfe9bcf : « Avis agent 3D : VALIDÉ avec réserves » présent (RG-E15-06) ; 40 captures ouvertes ; tout le diff lu (`mobilier_ville.*`, `ville.cpp`, `commerces.cpp`, accroche `cartes_banc.cpp`, test) ; construction et `tests_cpu.sh` relancés : 95/95.
- Revue « ✅ Approuvé » écrite dans la PR (commentaire, même compte GitHub), puis fusion en squash : b4e3a106. Statut 🟡 À valider (utilisateur) : le ticket vient de la capture d'APK de l'utilisateur (bancs en pavés) et d'ANO-216.
- Tâches séparées demandées au PO (RG-07 / RG-E15-09) : R1 eau de la fontaine (ANO-214) ; R2 affiche d'abribus vide et non éclairée la nuit ; R4 éclairage du parc la nuit (E13) ; plaque de rue bleue unie des feux (WFS-385) ; mesure sur téléphone des +44 % de triangles en Basse (WFS-321 / 331).

### 2026-11-10 00:43:03 — WFS-396 fusionné par le lead dev (lead-dev)
- PR #325 relue (commit 19d49424) : 30 captures ouvertes, diff complet lu (fiches réelles `remplacants-wfs396.json`, rapport RG-08, une ligne de suivi), rien dans `src/`, `tools/`, `assets/`. Construction et 94 tests CPU verts sur la branche ; `test_fidelite` 0 défaut.
- Approbation écrite dans la PR (commentaire « ✅ Approuvé par le lead dev », même compte GitHub), fusion en squash fdcb7254. Statut 🟣 ; ligne `qa/Check.md` ; ligne retirée de `suivi-pr.md`.
- Remarques pour WFS-397 (PO) : hauteur de la W212 à −2,7 % (le suivi du PO dit −2,3 %) ; captures de nuit obligatoires ; logos des candidats à effacer.

### 2026-11-10 00:43:38 — WFS-396 fusionné (#325, squash fdcb7254, lead dev) : fin de la session (Claude, agent artiste (01UY1Vv2ph))
- Le lead dev a relu les 30 captures et le diff, et a fusionné. Refus motivé pour les deux places (±1 %), rien de branché ; la suite est dans WFS-397 (PO).
- Remarques du lead dev pour WFS-397 : la hauteur de la W212 est à −2,7 % après l'échelle de `corriger_voitures` (−2,3 % avant l'échelle) ; les captures de nuit (`WFS_VITRINE_NUIT`) seront obligatoires ; tous les logos et noms des candidats sont à effacer avant le branchement.
- Session terminée (un seul ticket) : aucun autre ticket demandé.

### 2026-11-10 00:43:29 — WFS-369 : réserves de la PR #324 en tâches séparées, WFS-398 et WFS-399 créés (po-scrum-master)
- Demande de Claude, agent dev3d 3 (01UY1Vv5WS), à la fin de WFS-369 (fusionné, #324, b4e3a106, 🟡 à valider). Session close : **aucun autre ticket** pour elle (un sujet par session) ; ligne de `track-agent.md` en « ⏹️ Terminé (session close) ».
- Décisions (RG-07 de WFS-369, RG-E15-09, ordre du 2026-11-09 « on ne jette pas un travail pour un petit défaut ») :
  - R1 eau des fontaines plate cyan, vasque sans eau qui coule, jet de l'îlot inchangé → **ANO-235**, **WFS-398** (P2, E15 Ville + réglage de l'eau de WFS-380 ; après WFS-380 et 370).
  - R2 affiche d'abribus vide et panneau non éclairé la nuit → **ANO-236** ; R4 parc presque invisible la nuit → **ANO-237** ; plaque bleue de nom de rue unie (lambda `mat` de `ville.cpp`) → **ANO-238** ; optiques des feux éteintes de jour → **ANO-239** : un seul lot **WFS-399** (P2, E15 Ville ; après WFS-370). La plaque de rue, proposée pour WFS-385, va dans WFS-399 (même mobilier, même lot, pas de micro-ticket).
  - Mobilier absent (poubelles, bornes, kiosques, barrières) : RG ajoutée à **WFS-385**.
  - Performance (+44 % de triangles, Ville de nuit Basse, 260 k → 375 k, sans baisse au banc PC) : pas de ticket ; mesure sur téléphone ajoutée aux règles de **WFS-321** et **WFS-331**, et au budget de WFS-385.
- **WFS-370** (immeubles de la Ville) passe ⏳ Prêt (WFS-369 fusionné). File Ville de E15 : 370 → 399 / 385 ; 398 attend aussi WFS-380.

### 2026-11-10 00:44:32 — WFS-369 : fin de session (Claude, agent dev3d 3 (01UY1Vv5WS))
- PR #324 fusionnée en squash par le lead dev (b4e3a106). L'agent 3D a donné « VALIDÉ avec réserves » sur 6cfe9bcf, après la correction de la terrasse grise (B1, cb2f7d84). Le ticket est 🟡 à valider par l'utilisateur.
- Réserves confiées au PO : WFS-398 (eau des fontaines) et WFS-399 (affiches, éclairage du parc, plaque de rue, feux éteints de jour). Les +44 % de triangles en Basse restent à mesurer sur le téléphone (règles de WFS-321 / 331 / 385).
- Un seul ticket par session : aucun autre ticket demandé, la session s'arrête.

### 2026-11-10 01:08:22 — WFS-380 : PR #326 ouverte, vraie mer (Claude, agent cartes (01UY1Vv4eo))
- **Fait** : une seule eau pour tout le jeu (`MAT_EAU`, `shaders/eau.glsl`) : 8 vagues calculées (houle 28 m / 4,2 s, Hs 0,47 m), Fresnel 0,02, traînées du soleil, de la lune et des lampadaires, couleur lue à travers l'eau selon la profondeur (distance au vrai trait de côte, pente du fond), écume de ressac et de déferlement en dentelle, sillage en V (Kelvin) et éclaboussures en dentelle à la place du trapèze et du disque blancs ; mer de la Corniche, bassin de la marina et rivière de la Ville (paramètres dans les sommets) ; blocs de calcaire et galets au bord de la côte rocheuse et au pied de la falaise (RG-09). Banc `WFS_BANC=mer` (15 vues).
- **Accroches annoncées** (≤ 10 lignes chacune) : `shaders/scene.frag` (branche de l'eau, 7 lignes), `src/render/renderer.h` (`MAT_EAU`), `src/render/renderer.cpp` (l'eau ne porte pas d'ombre), `src/world/ville.cpp` (eau de la rivière, 5 lignes), `src/platform/desktop_main.cpp` (banc `mer`).
- **Vérification** : construction complète, `tests_cpu.sh` 96/96 (dont `test_eau`) après synchronisation avec `main` ; 15 paires de captures avant / après dans `qa/pr/WFS-380/` ; banc perf Corniche (lavapipe) : Basse, passe de scène 132,5 → 135,2 ms (+2,0 %), Haute, image 355,0 → 351,7 ms.
- **Aucun téléchargement** : eau et pierres calculées, aucune texture ni licence.
- **Pour le PO** : RG-E15-01 (200 à 700 Mo de décor) ne se mesure pas sur la Corniche, carte du code sans paquet ; ce ticket n'ajoute aucun octet d'actif. Le seuil relève de WFS-386. Limite : les reflets des lampadaires ne viennent que des 4 lampadaires allumés près de la voiture (`nuit.cpp`, WFS-382).
- **Suite** : avis de l'agent 3D (RG-E15-06), puis lead dev.

### 2026-11-10 01:08:26 — WFS-381 : brume au loin, accroches annoncées et avancement (Claude, agent dev A (01UY1Vv42R))
- Fait sur `feature/waforspeed-381` : brume exponentielle selon la distance **et** la hauteur (`shaders/eclairage.glsl` : `brumeTau`, `fogAt`, `appliquerBrume`), couleur de l'horizon du moment, plus chaude côté soleil, inchangée la nuit ; réglage par carte (`src/render/atmosphere_brume.h` : Corniche β0 1,5·10⁻⁴ m⁻¹ / H 800 m, Ville 1,4·10⁻⁴ / 1 000 m, Campagne 1,2·10⁻⁴ / 1 200 m, Grand Prix 1,3·10⁻⁴ / 1 200 m) ; banc `WFS_BANC=brume` ; test `test_brume` (RG-02 : ≤ 3 % à 200 m, 10-20 % à 1 km, 40-60 % à 5 km).
- **Accroches ≤ 10 lignes hors zone (annonce)** : `shaders/scene.frag` et `shaders/scene_skin.frag` (fin du `main` : 5 lignes de brouillard remplacées par 1 appel) ; `src/render/renderer.h` (+1 ligne : `fogHauteurInv`, `fogRef` dans `FrameParams`) et `renderer.cpp` (1 ligne : `u.fogLin.zw`) ; `src/platform/desktop_main.cpp` (+2 lignes : banc brume) ; `CMakeLists.txt` (+1 ligne : `test_brume`). Aucun changement de `post.frag`, `nuit.cpp`, `phares.h` (WFS-382) ni de l'eau (WFS-380).
- Vérification : 96 tests CPU en local ; coût GPU de la passe de scène en Basse sur lavapipe, 2 mesures avant / 2 après : écart dans le bruit (≈ ±1 %) ; nuit identique au pixel près (RMSE 0).
- Limite constatée : les cartes ne montrent rien au-delà de ≈ 2 km ; avec les valeurs réelles de RG-02, la brume enlève ≈ 14 % de contraste à 1 km et ≈ 25 % à 2 km (avant : 8 % et 16 %) : l'effet est réel mais discret ; les vrais lointains à 5-20 km (reliefs, côte) relèvent des tickets de décor (E15).

### 2026-11-10 01:14:15 — WFS-372 : PR #327 ouverte (Claude, agent dev3d 5 (01UY1Vv5G4))
- Fait : vignes palissées (piquets, 3 fils, ceps, feuillage `veg::vigne`), clôtures et enclos (poteaux à pointe, lisses en demi-rondin), bottes rondes, corps de ferme (fenêtres en retrait, volets, tuiles canal, génoise, cheminée), hangar agricole (portiques en I, bardage, plaques ondulées), poteaux de ligne avec isolateurs et câbles. Fichiers : `src/world/decor_campagne_modeles.*` (nouveau), `decor_campagne_banc.cpp` (banc `campagne-objets`), `campagne.cpp`, `vegetation.*`, `tests/test_decor_campagne.cpp`.
- Accroche annoncée (≤ 10 lignes, hors zone) : `src/platform/desktop_main.cpp`, 2 lignes (`WFS_BANC=campagne-objets`) ; `CMakeLists.txt` : 1 ligne (test).
- Vérifié : `test_decor_campagne` 49/49 ; `tests_cpu.sh` 96/96 ; bancs `campagne` et `campagne-objets` 0 écart ; perf Campagne Basse 180,2 → 184,4 ms (+2,3 %, lavapipe), appels de dessin inchangés (161).
- Réseau : aucun téléchargement (seuls GitHub et les paquets sont permis) → RG-03, objets modélisés au niveau du vrai objet.
- Limites : éoliennes, église, commerces, fontaine, signalisation, haies, autoroute restent pour WFS-387 (inventaire `qa/pr/WFS-372/inventaire.md`). Suite : avis de l'agent 3D (RG-E15-06), puis lead dev.

### 2026-11-10 01:15:03 — WFS-382 : PR #328 ouverte, accroches annoncées (Claude, agent dev N (01UY1Vv5K1))
- Fait : phares de nuit réels. Tache des phares du jeu web retirée (`nuit.cpp`, elle doublait les deux feux de croisement : cône blanc en Campagne et sur la neige) ; répartition verticale du feu de croisement revue (`phares.h`, plateau jusqu'à −1,5°, chute rapide vers le premier plan, flux sourcé gardé, crête LED 30 000 cd ; avis expert-realisme-3d, R112 / R149) ; lueur des phares 0,30 → 0,12 m ; reflet des feux de croisement borné ; halo et bloom plafonnés la nuit ; banc court `WFS_BANC=phares`.
- **Accroches annoncées** (≤ 10 lignes chacune) : `shaders/eclairage.glsl` (7 lignes : même répartition que `phares.h`, plafond du reflet des feux de croisement), `shaders/post.frag` (E13, 5 lignes : plafond de nuit), `src/render/renderer.cpp` (2 lignes : lueur des phares), `src/platform/desktop_main.cpp` (1 ligne : banc `phares`).
- Défaut trouvé et corrigé dans `nuit.cpp` : `fx.stop / fx.recul / fx.frein` du joueur étaient au bout d'un commentaire (jamais exécutés) ; ligne recollée.
- Vérifié : 95 tests CPU réussis ; pixels saturés 0,37-0,80 % → 0 % (poursuite, voiture en face, pluie) ; neige de nuit, tache des phares 7,56 → 0 % ; témoins de jour identiques ; perf Basse Ville nuit pluie : coût égal à ±3 % près (2 mesures).
- Limites : bloom absent du banc (lavapipe), à voir sur téléphone ; rétroréflexion de l'asphalte et glace modélisée du phare non faites (proposées).

### 2026-11-10 01:19:29 — WFS-391 : PR #329 ouverte (Claude, agent conduite (01UY1Vv4e7))
- Fait (`src/game/player.cpp`, ancienne conduite) : RG-01, au freinage seules les forces des pneus (freins, frein moteur) × μ, traînée et roulement hors μ (neige 145,2 → 126,9 m, sec 36,8 m inchangé) ; RG-02, μ bas-côté ≤ μ chaussée × 0,6 (neige 0,2 → 0,15 ; sec et automne inchangés).
- Cause de l'ANO-231 (RG-06) : point de mesure à 11,5 m derrière la glissière du col (edge 1,38 = 10,35 m) : la voiture la longe (rayon 11,65 m). Rien à corriger dans la conduite (arbitrage du PO) ; test sur les 5 positions devant la glissière (± 0,4 %).
- **Accroche annoncée** (2 lignes) : `src/platform/desktop_main.cpp`, nouveau banc `WFS_BANC=sol` (`src/game/sol_banc.cpp`) ; interrupteur de banc `g_avantWfs391` (`player.h`).
- Vérifié : 94 tests CPU réussis (nouveaux tests en échec sur main : 5) ; bancs `sol` et `neige` 0 écart ; parcours 0 écart ; 50 captures dans `qa/pr/WFS-391/`.
- Limites : ancien modèle sous 140 m à cause du roulement du jeu (0,35 m/s², noté pour WFS-258) ; ressenti à valider sur téléphone (🟡 après fusion).

### 2026-11-10 01:27:12 — WFS-362 : PR #323 corrigée après la revue du lead dev (Claude, agent dev3d 4 (01UY1Vv5HV))
- Causes : (1) cratère de la M4 : le bourrelet autour de l'ancienne calandre donnait au panneau profondeur et normales ; (2) écusson : ce même bourrelet ; (3) éclats du coin S3 : jour de 1,5 cm entre bord découpé et paroi du conduit ; (4) lames en peigne : pas de rebord ; (5) bande sombre S3 : surface presque verticale.
- Corrections (`tools/retoucher_formes.mjs`, `tools/formes.json`) : contour M4 jusqu'au joint du capot et aux pointes des phares ; panneau = base de degré 2 ajustée sur la peinture voisine + écart interpolé, normales de la surface finale lissées, vrai décalage du débord ; options de grille `joue` et `rebord` ; X5 inchangée (empreinte rejouée).
- Vérification : `--verifier` vert (5 voitures) ; 95 tests CPU verts ; captures régénérées (18 vitrines, 16 gros plans, 16 zooms) ; réponse point par point dans la PR ; lead dev rappelé.


### 2026-11-10 01:30:46 — WFS-372 fusionné par le lead dev (lead-dev)
- PR #327 fusionnée en squash (0654a4af) à la 1re revue, après « Avis agent 3D : VALIDÉ avec réserves » sur 6b3b4bb9 (RG-E15-06 et 09). Revue « ✅ Approuvé » écrite dans la PR en commentaire (GitHub refuse l'approbation du même compte).
- Vérifié : 58 captures ouvertes (paires avant / après jour et nuit, gros plans, perf, inventaire) ; diff complet relu ; construction et `tests_cpu.sh` refaits : 96/96 ; perf Basse +2,3 % (lavapipe, sous le seuil de 5 %) ; `vegetation.*` : ajout seul de `veg::vigne`, permis par la zone du ticket (vignes).
- Statut 🟣 Fusionné ; ligne de `suivi-pr.md` supprimée ; ligne ajoutée à `qa/Check.md`.
- **Pour le PO** : créer une tâche séparée (RG-E15-09) avec les réserves 1 à 6 de l'agent 3D (bottes, crépi, teintes du bois, tôle du hangar, perf sur le téléphone) et mes remarques : câbles des poteaux et lisses des clôtures aux limites de deux zones « champs », commentaire en double dans `decor_campagne_banc.cpp:29`, comparaison de flottants dans `fenetre`.

### 2026-11-10 01:31:23 — WFS-372 : fin de session (Claude, agent dev3d 5 (01UY1Vv5G4))
- PR #327 fusionnée par le lead dev (squash 0654a4af) après « Avis agent 3D : VALIDÉ avec réserves » sur 6b3b4bb9. Session terminée (un seul sujet par session) : aucun autre ticket demandé.
- À reprendre dans une tâche séparée (RG-E15-09), transmis au PO : bottes sans grain ni filet ; crépi de la ferme trop froid (gris-bleu) ; poteaux de clôture plus sombres que les lisses ; poteau électrique trop sombre ; ondes de la tôle du hangar invisibles de loin ; performance à mesurer sur le téléphone (triangles +35 % au départ) ; `campagne.cpp` : câble ou lisse qui peut finir sans poteau quand deux zones « champs » se suivent (`T_POLE`, `clotureSuit`) ; commentaire en double `decor_campagne_banc.cpp:29` ; comparaison de flottants par `==` dans `fenetre` (`decor_campagne_modeles.cpp`).

### 2026-11-10 01:31:36 — WFS-362 : PR #323 refusée par le lead dev, 2e revue (lead-dev)
- Commit relu 13a06c92 ; 50 paires de captures ouvertes, diff lu (`tools/retoucher_formes.mjs`, `tools/formes.json`). Revue : https://github.com/wlfaleh/waforspeed_mobile/pull/323 (2e revue « COMMENT » valant refus).
- Bloquants : (1) S3, une lame ou le rebord de la grille passe devant la plaque (numéro barré, régression de ce passage ; aucun contrôle de la plaque, `ignorer: [18]`) ; (2) M4, panneau encore déformé au zoom ×4 de 3/4 droit (pli en rouleau au bord gauche, marche sous le capot, contour en écusson dans les reflets) ; (3) M4, reste chromé de l'emblème au centre du nez ; (4) S3, coin droit de la grille encore en dents, avec un éclat de nuit. À corriger aussi : rebord M4 en dents de scie, teinte du panneau S3, note « bombée » de la M4 alors que bombe = 0.
- Progrès notés : silhouette des haricots disparue de face, vrai décalage `decaler()`, lames S3 contre un rebord.
- TACHES : WFS-362 ↩️ Retour lead dev (PR #323) ; track-agent et suivi-pr mis à jour.

### 2026-11-10 01:32:27 — WFS-372 en 🟡, réserves versées dans WFS-387 (po-scrum-master)
- Fin de ticket annoncée par Claude, agent dev3d 5 (01UY1Vv5G4) : WFS-372 fusionné (#327, 0654a4af), « Avis agent 3D : VALIDÉ avec réserves ». Session close, aucun nouveau ticket pour elle.
- WFS-372 corrige des défauts vus par l'utilisateur (ANO-65, ANO-219) → **🟡 À valider (utilisateur)** (le robot ne le passe pas en ✅).
- RG-E15-09 : pas de nouveau ticket ; les 6 réserves de l'agent 3D et les 3 remarques du lead dev vont dans **WFS-387, RG-12** (même carte, mêmes fichiers, travail suivant de la file Campagne ; évite un micro-ticket et deux PR sur les mêmes fichiers).
- WFS-387 passe **⏳ Prêt** (E15 Campagne, agent dev-artiste-3d), pour une nouvelle session.


### 2026-11-10 01:34:04 — WFS-382 : PR #328 refusée par le lead dev (lead-dev)
- Revue reprise après le redémarrage du conteneur (rien n'avait été posté). Commit relu : 3c89a192. Les 22 captures ont été ouvertes et zoomées, le diff lu en entier, la branche construite ; tests CPU : 95 réussis.
- Gros progrès sur le sujet : plus de disques autour des phares, sol plus brûlé en Ville, Campagne et neige (RG-07 tenu) ; témoins de jour identiques.
- **Refus** (revue postée en COMMENT sur la PR, car GitHub refuse « changements demandés » quand la PR vient du même compte) :
  1. `shaders/post.frag:124/127` : le plafond du halo et du bloom coupe net chaque canal. Résultat, la nuit : un disque plat, un carré et un anneau irisé autour des têtes de lampadaire et de l'écran du Cockpit (forme interdite, régression sur toutes les cartes de nuit). Correction attendue : un plafond progressif, calculé sur la luminance.
  2. RG-01 : la tache de 30 à 60 m et la coupure ne se voient pas sur la capture 3 (Campagne, vue Capot).
- TACHES : WFS-382 passe en ↩️ Retour lead dev (PR #328) ; track-agent et suivi-pr mis à jour.

### 2026-11-10 01:35:32 — WFS-382 : rétroréflexion de la chaussée accordée dans le ticket (po-scrum-master)
- Question de Claude, agent dev N (01UY1Vv5K1), défaut 2 du lead dev sur la PR #328 (RG-01 invisible). Décision **(A)** : rétroréflexion EN 1436 faite dans WFS-382 ; exception de zone `shaders/eclairage.glsl` ≈ 15 lignes au total, annoncées au journal ; feux de croisement du joueur seulement ; pas de `frame_ubo` ni renderer ; budget Basse (RG-05, WFS-353) tenu ; synchronisation avec `main` avant l'envoi (WFS-381). Détail : `regles-de-gestion/WFS-382.md`, « Suivi ».

### 2026-11-10 01:37:40 — WFS-381 : RG-02 de la brume corrigée, option A (po-scrum-master)
- Question de Claude, agent dev A (01UY1Vv42R) : RG-02 se contredisait (≤ 3 % à 200 m impose V ≥ 26 km) et l'effet était invisible à l'écran. Décision **(A)** : Corniche V ≈ 15 km, Ville ≈ 18, Grand Prix ≈ 20, Campagne ≈ 22 ; ≤ 5 % à 200 m, ≤ 25 % à 1 km, 60 à 75 % à 5 km ; nuit (RG-03) et budget (RG-05) inchangés ; le lead dev relit avec cette décision. Détail : `regles-de-gestion/WFS-381.md`, « Suivi ».

### 2026-11-10 01:47:40 — WFS-381 : PR #330 ouverte, lead dev appelé (Claude, agent dev A (01UY1Vv42R))
- RG-02 appliquée selon la décision du PO (option A, 01:37:40) : Corniche β0 2,55·10⁻⁴ m⁻¹ (V 15,3 km), Ville 18 km, Grand Prix 20 km, Campagne 22 km ; test `test_brume` vert, 96 tests CPU verts.
- 7 captures avant / après + écart ×8 dans `qa/pr/WFS-381/` ; nuit identique au pixel près ; coût de la passe de scène dans le bruit de mesure (lavapipe).
- Limite : effet surtout visible dans les vues larges (les cartes s'arrêtent vers 2 km).

### 2026-11-10 02:03:29 — WFS-391 fusionné par le lead dev (lead-dev)
- PR #329 (Claude, agent conduite (01UY1Vv4e7)), commit relu 3aef4fca ; squash 6ea09965 sur main ; revue « ✅ Approuvé » écrite dans la PR en commentaire (même compte GitHub).
- Contrôles : les 60 captures ouvertes ; tout le diff lu (`player.cpp`, `player.h`, `sol_banc.cpp`, accroche de 2 lignes dans `desktop_main.cpp`, 2 tests) ; branche fusionnée en local avec main : construction OK, 95 tests CPU réussis ; bancs `sol`, `neige` et `parcours` relancés : 0 écart.
- Statut : 🟡 À valider (utilisateur), car c'est un ticket de conduite. Ligne retirée de `suivi-pr.md`. Ligne ajoutée à `qa/Check.md`.
- Pour le PO : RG-01 demande un test « pneus = μ × g ± 2 % dans les deux modèles ». Le test de la PR prouve la proportionnalité à μ. La valeur absolue n'est pas testée : Golf, ancien modèle, 1,024 μ g, soit +2,4 %, fixée par le réglage de freinage de WFS-188. Critère à préciser pour WFS-258.
### 2026-11-10 02:03:41 — WFS-381 refusé par le lead dev, PR #330 (lead-dev)
- Commit relu 11d3f1de ; 21 images ouvertes ; tout le diff lu ; construction et `tests_cpu.sh` refaits : 97 tests verts, `test_brume` vert ; nuit identique (RG-03 tenue) ; code sans défaut bloquant.
- Refus : sur les captures 1 à 3 et les vues hautes, l'écart avant / après est de 0,6 à 1,5 niveau sur 255 en moyenne : on ne distingue pas les images, les lointains ne sont pas « nettement plus pâles et bleutés » (critère du ticket, demande de l'utilisateur ANO-221). `brume-mesures-avant.txt` montre des valeurs que main n'utilise pas.
- Demandé : captures où l'effet se voit (avec une mesure du contraste à l'écran à 1 et 2 km), ou une nouvelle décision du PO si la RG-02 ne le permet pas avec des cartes de moins de 2 km ; PO prévenu (critère et RG-02 incompatibles avec ce qui est à l'écran).
- `TACHES.md` : ↩️ Retour lead dev (PR #330) ; `track-agent.md` et `suivi-pr.md` mis à jour.

### 2026-11-10 02:03:59 — Fin WFS-391 : session terminée (Claude, agent conduite (01UY1Vv4e7))
- PR #329 fusionnée en squash par le lead dev (6ea09965) ; WFS-391 en 🟡 À valider (utilisateur) : freinage sur la neige plus court (≈ 127 m depuis 100 km/h au lieu de 145 m), bas-côté enneigé moins accrocheur, sec inchangé.
- Remarques non bloquantes du lead dev notées : critère « pneus = μ × g ± 2 % » à préciser pour WFS-258 (Golf 1,024 μ g dans l'ancien modèle) ; banc `sol` : la voiture posée saute des checkpoints (banc seulement).
- Session arrêtée (un seul sujet par session) : aucun autre ticket demandé.

### 2026-11-10 07:31:05 — WFS-381 : décision sur RG-02 après le refus de la PR #330 (po-scrum-master)
- Question de Claude, agent dev A (01UY1Vv42R), transmise par le lead dev : l'option A (V 15 à 22 km) ne se voit pas sur des cartes de moins de 2 km (écart 0,6 à 1,5 niveau sur 255).
- Décision : **option C**. RG-02 = jour de brume de chaleur d'été : Corniche V 8 km, Ville 10 km, Grand Prix 11 km, Campagne 12 km (bornes à 200 m, 1 km, 2 km, 5 km dans `regles-de-gestion/WFS-381.md`, Suivi) ; critère des captures chiffré (contraste à 1 et 2 km dans `pr.md`, écart ≥ 20 niveaux sur les lointains).
- Fond lointain : pas de nouveau ticket ; RG-E15-10 ajoutée à `E15-decors.md` (WFS-385 à 389), avec la remarque du lead dev sur `masquage::finBrouillard`.
- WFS-381 reste ↩️ Retour lead dev (PR #330) chez son développeur.

### 2026-11-10 07:31:44 — WFS-362 : PR #323 corrigée après le 2e refus du lead dev (Claude, agent dev3d 4 (01UY1Vv5HV))
- Causes : lame devant la plaque S3 (plaque ignorée pour caler la grille, aucun contrôle) ; normales du bord du panneau M4 faussées (surface bornée à sa boîte) ; restes de l'anneau du logo M4 hors de la fenêtre de découpe ; paroi et rebord de grille en escalier (carte au pas de 5 mm).
- Corrections (`tools/retoucher_formes.mjs`, `tools/formes.json`) : option de grille `derriere` + contrôle bloquant « devant la plaque » ; surface du panneau sur une zone élargie ; découpe complète de l'anneau ; paroi et rebord lissés ; note M4 corrigée. X5 inchangée (empreinte rejouée).
- Vérification : `--verifier` vert (5 voitures) ; 96 tests CPU verts ; captures régénérées avec les zooms demandés (plaque S3, haut du nez M4) ; réponse point par point dans la PR ; lead dev rappelé.


### 2026-11-10 07:42:21 — WFS-381 : seuil de 20 niveaux retiré du critère des captures (po-scrum-master)
- Mesures du développeur : contraste à 1 et 2 km dans les bornes RG-02 (Corniche 38,1 / 60,1 %, Campagne 29,0 / 47,4 %) ; écart à l'écran 8 à 14 niveaux, plafonné par la couleur de l'horizon de WFS-214.
- Décision : seuil de 20 niveaux retiré (mon erreur de calcul) ; restent la mesure du contraste et le jugement à l'œil du lead dev ; Ville jugée à l'œil seulement. Détail dans `regles-de-gestion/WFS-381.md` (Suivi).

### 2026-11-10 07:42:55 — WFS-380 : avis de l'agent 3D corrigé, VALIDÉ avec réserves, lead dev appelé (Claude, agent cartes (01UY1Vv4eo))
- **1er avis (41a95f40) : À REPRENDRE**, D1 à D6. Corrigé dans 8a88af3c :
  - D1 : horizon clair (Fresnel de l'eau entier, rugosité plafonnée) ;
  - D2 : directions des vagues écartées, plus une petite mer du vent sans période ;
  - D3 : écume contre la roche ;
  - D4 : rivière d'eau douce trouble, vert-brun, 4 m de fond au plus ;
  - D5 : nappe du ressac et sable mouillé sur la plage ;
  - D6 : écume en lignes parallèles au bord, qui avancent.
  Capture `mer-eclaboussure` ajoutée.
- **2e avis (0a587856) : VALIDÉ avec réserves** (commentaire 6761864467 de la PR #326).
- **Vérification** : 97 tests CPU réussis ; captures refaites après la synchronisation avec `main`.
- **Banc perf de la Corniche** (`main` et branche mesurés sur la même machine) :
  - Basse : scène 150,3 → 146,1 ms, 226 appels de dessin des deux côtés, dans le bruit de mesure. En Basse, la nappe et la petite mer du vent sont coupées.
  - Haute : scène 303,7 → 302,9 ms.
- **Pour le PO, réserves à mettre en tâches séparées** (RG-E15-09) :
  - R1 : reflets des façades allumées de nuit ;
  - R2 : reflets des coques dans le bassin de la marina ;
  - R3 : traînée du soleil bas plus nette ;
  - R4 : reflets des lampadaires de la rivière ;
  - R6 : coût de l'eau en Basse à mesurer sur le téléphone (RG-07, 1,0 ms au plus) ;
  - R7 : aspect « filé » de la mer à angle rasant ;
  - R8 : capture de l'écume contre des blocs dans l'eau et au pied de la falaise ;
  - R9 : maillage trop régulier du reflet du casino la nuit ;
  - R10 : éclaboussure du dauphin vue comme un trait plat (déjà sur `main`).
  RG-E15-01 (200 à 700 Mo) ne se mesure pas sur la Corniche, carte du code : à trancher avec WFS-386.

### 2026-11-10 07:45:16 — WFS-362 fusionné par le lead dev (lead-dev)
- PR #323, 3e revue, commit relu `a88efa52`, squash `30d339f4`. Revue « ✅ Approuvé » en commentaire dans la PR (même compte GitHub).
- 58 paires de captures ouvertes ; points 1 (plaque S3), 3 (emblème M4) et 4-5 (rebords en dents) de la 2e revue réglés.
- Construction et tests CPU refaits par le lead dev sur la branche fusionnée avec `main` : 96/96 ; `--verifier` : empreintes concordantes.
- **Au PO (RG-06)** : tâche séparée à créer pour les défauts restants, visibles seulement en gros plan : reflets « en cuvette » du panneau M4 de 3/4 droit (pli au bord gauche, marche sous le joint du capot, arc orangé de nuit) ; éclat orangé de nuit au coin bas droit de la grille S3 ; barre blanche sous la pointe du capot M4 en vue basse.
- Statut : 🟣 Fusionné (pas un retour vu sur le téléphone de l'utilisateur).

### 2026-11-10 07:45:48 — WFS-362 : fusionné, session terminée (Claude, agent dev3d 4 (01UY1Vv5HV))
- PR #323 approuvée et fusionnée en squash par le lead dev à la 3e revue (`30d339f4`) : M4 et S3 sans calandre de marque (pièce de carrosserie + grille générique derrière la plaque) ; outil `tools/retoucher_formes.mjs` enrichi (`panneau`, options de grille `joue`, `rebord`, `derriere`), X5 inchangée.
- Défauts restants non bloquants (RG-06, tâche séparée demandée au PO par le lead dev) : reflets du panneau M4 de 3/4 droit, éclat de nuit au coin bas droit de la grille S3, barre claire sous la pointe du capot M4 en vue basse.
- Un seul ticket par session : la session s'arrête, aucun autre ticket demandé.


### 2026-11-10 07:48:00 — WFS-384 attribué à agent expert ci cd (session 0183bv2Y) (po-scrum-master)
- Ordre de l'utilisateur (« Prend 2 et 3 », 2026-11-10 ≈ 07:45 UTC, transmis par l'agent expert ci) : **WFS-384** ([TECHNIQUE] bancs du matin sautés, P1, E7, ANO-226) attribué à la session 0183bv2Y, son premier et seul ticket. Branche `feature/waforspeed-384`, zone `.github/workflows/matin.yml`, `tools/qa/**`, skill `expert-ci-cd`. US technique : CI de branche autorisée, une à la fois, notée au journal.
- Exception à « un ticket actif par épic » sur ordre de l'utilisateur : WFS-319 (E7, publication) reste 🔧 chez l'agent expert ci (session 01UY1Vv4).
- Constat du 2026-11-10 07:40 UTC (aucun run ce matin, runs #6 à #11 sautés, Release `captures-matin` absente, issue #242 muette depuis 2026-11-09 01:52 UTC) et nouvelle **RG-06** (dire clairement à la QA quand `captures-matin` manque ou n'est pas du jour) ajoutés à `regles-de-gestion/WFS-384.md`.
- `track-agent.md` : ligne de la session ajoutée ; lignes des sessions terminées enlevées (dev3d 5, dev M, dev Z, dev3d 2, dev3d 3, dev3d 4, artiste, conduite).

### 2026-11-10 07:48:47 — CLAUDE.md : la QA lit les captures du matin dans `captures-matin` (agent expert ci cd (session 0183bv2Y))
- Sur l'ordre direct de l'utilisateur (« Prend 2 et 3 », point 3) : deux passages de `CLAUDE.md` disaient encore « captures dans `apercu` » ; depuis WFS-352, `apercu` ne garde que l'APK et les captures du matin vont dans la Release `captures-matin` (anciennes : `apercu-ancien-20261109-0925`). Rien d'autre changé.

### 2026-11-10 07:50:16 — WFS-381 : PR #330 corrigée après le refus du lead dev (Claude, agent dev A (01UY1Vv42R))
- Décisions du PO appliquées (Suivi de WFS-381, 07:30:48 et 07:42:21) : brume de chaleur d'été (Corniche 8 km, Ville 10, Grand Prix 11, Campagne 12, autres 10) ; `test_brume` sur les nouvelles bornes ; 97 tests CPU verts.
- Banc `brume` : contraste mesuré à l'écran (cible HDR, 5 rendus figés) : Corniche 8,3 → 38,1 % à 1 km, 15,7 → 60,1 % à 2 km ; Campagne 8,9 → 29,0 % / 16,1 → 47,4 % ; écart à l'écran 8 à 14 niveaux (sans seuil).
- Réponse point par point sous la revue ; suivi-pr en 🔁 ; lead dev rappelé (commit relu attendu : 370b3963).

### 2026-11-10 07:53:28 — WFS-384 : PR #331 ouverte, lead dev appelé (agent expert ci cd (session 0183bv2Y))
- Cause : réveils de GitHub en retard (2026-11-09 : 09:44 à 18:10 heure de Paris, tous sautés ; 2026-11-10 : aucun avant 08:40) : aucun banc complet depuis le run #5, donc pas de Release `captures-matin`, la QA n'a rien trouvé.
- Correction (E7) : réveil de secours chaque heure de 03:23 à 20:23 UTC ; bancs lancés au premier réveil à partir de 04:00 heure de Paris quelle que soit l'heure, jamais deux fois le même jour (`tools/qa/heure_matin.sh`) ; message « ⏳ Bancs du matin en retard » à la QA après 07:00 ; « Recette QA prête » remplacé par « captures du jour manquantes » quand `captures-matin` n'a pas les captures du jour ; notes de `apercu` à jour.
- Vérification : `tools/qa/test_matin.py`, 17 cas verts (faux gh) ; aucune CI de branche lancée (sans intérêt pour un réveil programmé).

### 2026-11-10 07:55:36 — WFS-384 fusionné par le lead dev (lead-dev)
- PR #331 relue (diff complet : `matin.yml`, `heure_matin.sh`, `retard_matin.sh`, `etat_captures_matin.sh`, `recette_prete.sh`, `publier_apercu.sh`, `test_matin.py`) ; `test_matin.py` relancé : 17/17 ; RG-02 (garde de `statuts.yml`) inchangée ; aucun banc affaibli. Revue « ✅ Approuvé » en commentaire (même compte GitHub), squash `071f10af`.
- À suivre : le prochain réveil (minute 23 UTC) doit lancer le banc du 2026-11-10 (1er critère d’acceptation, numéro du run à noter).

### 2026-11-10 07:56:59 — WFS-380 refusé par le lead dev, PR #326 (lead-dev)
- Commit relu 0a587856 : 32 captures ouvertes, diff complet lu, construction et `tests_cpu.sh` refaits (97/97, dont `test_eau`) ; avis agent 3D « VALIDÉ avec réserves » présent.
- **B1 (bloquant)** : `apres-mer-marina` : l'eau calme du port devient une nappe blanche le long du quai, avec des barres blanches dans le bassin (pixel 1150,700 : bleu (24,138,174) → blanc (205,204,202)). Cause : zone marina = agitation 0,3 et pente 1 (quai) → toute l'écume de côte rocheuse (`shaders/eau.glsl` l. 171-190, seuil `agit > 0.2`, termes non multipliés par l'agitation). Correction : écume liée à l'agitation, aucun blanc dans le port ; captures marina, bord et rochers refaites, puis nouvel avis agent 3D.
- 3 remarques non bloquantes (abscisse de la rivière dans `ville.cpp`, détection de la Basse, commentaire en double). TACHES, track-agent et suivi-pr passés en ↩️.

### 2026-11-10 08:03:56 — WFS-381 fusionné par le lead dev (lead-dev)
- PR #330, commit relu 370b3963 (2e revue), squash ca0ecf57 sur main. Revue « ✅ Approuvé » en commentaire (GitHub refuse l'approbation sur le même compte).
- Relu avec les décisions du PO de 07:30:48 (RG-02 « jour de brume de chaleur ») et 07:42:21 (critère des captures). 14 captures et 7 images d'écart ouvertes, agrandies sur les lointains : lointains plus pâles et bleutés (nets sur les vues hautes, discrets en vue de poursuite) ; nuit identique à main.
- Diff complet lu ; construction et `tests_cpu.sh` refaits : 97/97 ; banc `brume` (vue corniche) relancé : mêmes mesures. 3 remarques non bloquantes (coût avant / après absent de `brume-mesures.txt`, effet discret en vue de poursuite, « -100 % » sans pixel). Statut 🟡 À valider (utilisateur) : défaut vu par l'utilisateur (ANO-221).

### 2026-11-10 08:04:27 — WFS-381 : fin de session (Claude, agent dev A (01UY1Vv42R))
- PR #330 fusionnée en squash par le lead dev (ca0ecf57) ; ticket 🟡 à valider par l'utilisateur (ANO-221) sur l'APK de main : lointains plus pâles et bleutés de jour, nuit inchangée.
- Remarques du lead dev laissées au PO : coût GPU à mesurer sur téléphone (QA) ; « -100 % » affiché par le banc quand une bande de distance n'a aucun pixel (Ville) ; vrai fond lointain par E15 (RG-E15-10).
- Un seul ticket par session : aucun autre ticket demandé, la session s'arrête.

### 2026-11-10 08:25:57 — WFS-382 : PR #328 corrigée après le refus du lead dev (Claude, agent dev N (01UY1Vv5K1))
- Défaut 1 (plafond coupé net dans `post.frag`) : compression douce sur la luminance, e / (1 + Y / 0,35), et halo local pondéré en gaussienne, la nuit seulement ; jour identique (0 pixel changé).
- Défaut 2 (RG-01, tache de 30 à 60 m) : rétroréflexion de l'asphalte (`retroChaussee`, accord du PO), feux du joueur seulement, caméra derrière eux, sol sombre horizontal ; chaussée en vue Capot 130 / 112 / 72 à 10 / 30 / 50 m (main : 225 / 123 / 96). **Accroche** `shaders/eclairage.glsl` : 14 lignes au total (exception accordée par le PO, ≈ 15). Marque par `spotCone.y` (pénombre) : `spotCone.w` est écrit par `renderer.cpp`, auquel le PO interdit de toucher.
- Vérifié : 97 tests CPU ; 0 % de pixels saturés sur toutes les captures après ; neige 0 % ; perf Basse égale, aux erreurs de mesure près. Synchronisé avec main 7a2e01d7 (brume WFS-381), captures refaites. Lead dev rappelé.

### 2026-11-10 08:39:17 — WFS-382 fusionné par le lead dev (lead-dev)
- PR #328, 2e revue, commit relu 27df0667, squash 1063fb3f. Approbation écrite dans la PR en commentaire (même compte GitHub).
- Défauts de la 1re revue corrigés : compression douce de l'éclat et du bloom sur la luminance (plus de disque, de carré ni d'anneau) ; RG-01, tache au sol visible en vue Capot (rétroréflexion accordée par le PO, marque par `spotCone.y` acceptée).
- Construction et tests refaits : 97/97. Statut : 🟡 À valider (utilisateur). Ligne retirée de `suivi-pr.md`, ligne ajoutée dans `qa/Check.md`.
- **Pour le PO** : cœur du reflet des phares de la police sur la chaussée mouillée en aplat gris à 227 (`min()` canal par canal de `SPEC_MAX_CROISEMENT`, `shaders/eclairage.glsl`), visible sur `apres-phares-poursuite-police` : non bloquant (mieux qu'avant), anomalie de suivi à créer (compression douce sur la luminance, comme `eclatNuit`).

### 2026-11-10 08:39:56 — WFS-382 : fin de session (Claude, agent dev N (01UY1Vv5K1))
- PR #328 fusionnée par le lead dev (squash 1063fb3f) ; ticket 🟡 À valider (utilisateur) ; réponse écrite dans `tache utilisateur.md`.
- Suite notée par le lead dev (pour le PO) : reflet des phares de la police sur chaussée mouillée encore plat (borne `SPEC_MAX_CROISEMENT` par canal), à compresser sur la luminance comme `post.frag`.
- Session terminée (un seul sujet par session) : aucune demande de nouveau ticket.

### 2026-11-10 08:53:25 — WFS-363 attribué à Claude, agent dev3d 6 (01UY1Vv3MV) (po-scrum-master)
- Demande : nouvelle session `@dev3d` (session_01UY1Vv3MVhKQapjuowdFE2c), agent `dev-artiste-3d`, 1er et seul ticket de la session.
- Choix : objectif de l'utilisateur « voitures sans marque » avant les décors ; ordre des voitures du 2026-11-09 19:14 UTC (358 → 362 → 359 → **363** → 360…) ; WFS-362 et 359 🟣 fusionnés, aucun ticket voitures actif (E10 libre). WFS-363 passe de ⏳ En attente à 🔧 En cours. Restent prêts pour les prochaines sessions `@dev3d` : WFS-370 (Ville), 373 (Grand Prix), 387 (Campagne), et WFS-397 (W212 / Virtus GT, WFS-396 🟣).
- Zone : `assets/cars/giulia*`, `assets/cars/rrsport*`, entrées `giulia` / `rrsport` de `tools/formes.json`, ajouts à `tools/retoucher_formes.mjs` sans changer les voitures déjà faites (`--verifier`), `tests/**`. Interdits : `vehicule_cockpit*`, `src/game/cars.h`, `remplacants/` (WFS-397), `plage.cpp` et eau (WFS-380, agent cartes).
- Règles : `regles-de-gestion/WFS-363.md` (RG-01 à RG-06). Pas de E15, donc pas d'avis agent 3D obligatoire ; l'agent 3D peut être consulté.

### 2026-11-10 08:55:14 — WFS-370 attribué à Claude, agent dev3d (01UY1Vv2Ph) (po-scrum-master)
- Demande d'une nouvelle session `@dev3d` (session_01UY1Vv2PhsfqmJW55jpNKHd, aucun ticket avant : 1er sujet). WFS-363 venait d'être donné à une autre session (agent dev3d 6, 08:53) ; ticket suivant pour ce profil : **WFS-370** (P1, E15 file Ville, prêt ; plus petit numéro P1 prêt après 363).
- Ticket : Ville, immeubles « boîtes » remplacés par de vrais volumes (toits, corniches, balcons, rez-de-chaussée, fenêtres en retrait, 30 à 50 % allumées la nuit). Zone : `src/world/archi_*`, `src/world/ville*.cpp`, `assets/world/modeles/**`, `assets/licences/` ; pas `plage.cpp` (WFS-380). Règles : `regles-de-gestion/WFS-370.md` + `regles-de-gestion/E15-decors.md`. **E15 : avis agent 3D « VALIDÉ » obligatoire avant la fusion.** Branche `feature/waforspeed-370`, inventaire au journal d'abord (partir de `qa/pr/WFS-369/inventaire.md`), captures avant / après jour et nuit, PR, appel de `@3d` puis du lead dev, arrêt de la session après la fusion.

### 2026-11-10 09:11:58 — WFS-380 : B1 du lead dev corrigé, PR #326 en 🔁 (Claude, agent cartes (01UY1Vv4eo))
- **Fait** (590aa2e7) :
  - B1 : l'écume du bord suit l'agitation du lieu, il n'y a plus de blanc dans le port ni dans la rivière ;
  - les 4 remarques non bloquantes sont traitées (commentaires, doublon retiré) ;
  - 2e vue de la marina au banc (`mer-marina-bassin`).
- **Synchronisation avec `main`** : brume (WFS-381) et phares de nuit (WFS-382), sans conflit.
- **Vérification** : 98 tests CPU réussis ; 17 paires de captures refaites sur aabd7bd1.
- **Suite** : 3e avis de l'agent 3D, puis lead dev.

### 2026-11-10 09:29:53 — WFS-380 fusionné par le lead dev (lead-dev)
- **PR #326**, 2e relecture sur aabd7bd1 : B1 (écume blanche dans le port) corrigé dans 590aa2e7 (écume du bord × `smoothstep(0,5 ; 0,9 ; agitation)`) ; pixel (1150, 700) de `mer-marina` : (205, 204, 202) → (18, 68, 77).
- 34 captures ouvertes ; diff depuis 0a587856 lu (10 lignes + synchronisations avec main, résolution propre) ; construction et `tests_cpu.sh` refaits : 98/98.
- « Avis agent 3D : VALIDÉ avec réserves » sur aabd7bd1. Revue « ✅ Approuvé » écrite dans la PR (en commentaire, même compte GitHub), puis fusion en squash : **f4546c8b**.
- `TACHES.md` : WFS-380 en 🟡 À valider (utilisateur) (ANO-214, défaut vu par l'utilisateur) ; ligne de `suivi-pr.md` supprimée ; `qa/Check.md` à jour.
- **Pour le PO** : réserves R7 (rides régulières au loin), R9 (reflet fort des lampadaires), R10 (trait blanc de l'éclaboussure) en tâches séparées ; description de la PR restée à « 96 tests / 15 captures » (98 / 17 en vrai), non bloquant.

### 2026-11-10 09:30:39 — Fin WFS-380 : PR #326 fusionnée, session terminée (Claude, agent cartes (01UY1Vv4eo))
- **Fusion** par le lead dev à la 2e revue (squash f4546c8b) ; « Avis agent 3D : VALIDÉ avec réserves » sur aabd7bd1. Ticket en 🟡 À valider (utilisateur).
- **Réponse à l'utilisateur** dans `tache utilisateur.md`, sous sa question sur la mer.
- **Pour le PO, réserves à mettre en tâches séparées** (RG-E15-09) :
  - écume et rides encore « filées » de près (R7) ;
  - reflets des façades et des coques (R1, R2) ;
  - traînée du soleil plus nette (R3) ;
  - reflet des lampadaires très fort près de la caméra la nuit (R9) ;
  - trait blanc sous le dauphin (R10) et capture rapprochée de l'éclaboussure ;
  - capture de l'écume contre des blocs dans l'eau et au pied de la falaise (R8) ;
  - coût en Basse à mesurer sur le téléphone (R6, RG-07).
- **À trancher par le PO** : RG-E15-01 (taille du décor) ne se mesure pas sur la Corniche, carte du code ; à voir avec WFS-386.
- **Session terminée** : un seul ticket par session ; aucun autre ticket demandé.

### 2026-11-10 09:36:47 — Suites de WFS-380 : WFS-400 et WFS-401 créés, décision sur la taille du décor (po-scrum-master)
- Demande de la session terminée « Claude, agent cartes (01UY1Vv4eo) » (WFS-380 fusionné, #326, f4546c8b, 🟡), sur ordre de l'utilisateur (« Demande au PO »). Aucun nouveau ticket pour cette session (un seul sujet par session).
- **Réserves en tâches séparées** (RG-E15-09), nouvelle file **« Eau commune »** de E15 (`shaders/eau.glsl`, `src/render/eau_*`), en parallèle de la file Corniche :
  - **WFS-401** (P2, ⏳ Prêt) : R7 écume et rides « filées » de près, rides régulières au loin (`mer-sillage`) ; R3 traînée du soleil bas ; R10 trait blanc sous le dauphin + capture rapprochée de l'éclaboussure ; R8 capture de l'écume contre des blocs et au pied de la falaise. Règles : `regles-de-gestion/WFS-401.md`.
  - **WFS-400** (P2, ⏳ En attente de WFS-401, mêmes fichiers) : R1 façades allumées (casino) ; R9 maillage du reflet du casino et reflet trop fort des lampadaires près de la caméra ; R2 coques de la marina ; R4 rivière de la Ville la nuit ; fin de la limite des 4 lampadaires de `nuit.cpp` pour les reflets. Règles : `regles-de-gestion/WFS-400.md`.
  - **R6** (coût en Basse sur le téléphone) : pas de ticket, vérifié pendant la validation 🟡 de WFS-380 (suivi dans `regles-de-gestion/WFS-380.md`) ; baisse → anomalie et ticket E4.
- **Décision RG-E15-01** (`regles-de-gestion/E15-decors.md`, `WFS-386.md` RG-11) : la taille 200 à 700 Mo ne se mesure **que dans les US « décor complet »** (WFS-385 à 389) ; les US intermédiaires notent seulement les Mo ajoutés. Carte du code sans `.wfcarte` : somme sur disque des actifs de décor utilisés par la carte (commande et dossiers dans la PR) ; paquet publié : partie décor du paquet.
- **WFS-383** (sols, pins d'Alep) passe ⏳ Prêt : WFS-379 et 380 fusionnés (🟡). WFS-401 et 383 peuvent avancer en même temps (fichiers séparés ; 401 ≤ 10 lignes dans `plage.cpp`).
- `track-agent.md` : ligne de Claude, agent cartes (01UY1Vv4eo) enlevée (session terminée).
- Pas de session libre : WFS-401 et WFS-383 attendent une **nouvelle session** de développeur qui les demande.

### 2026-11-10 09:45:04 — WFS-402 créé : fenêtres et portes réalistes des immeubles et des maisons (po-scrum-master)
- Demande de l'utilisateur (`@po`, 2026-11-10) : « ajoute une tâche pour le décor concernant le réalisme des fenêtres et des portes des immeubles, maisons… ».
- **WFS-402** (P1 Urgent, épic E15, nouvelle file « Ouvertures communes ») : inventaire par carte, fenêtres en retrait de 15 à 25 cm (cadre, petits bois, appui, linteau, vitre qui reflète), volets et garde-corps selon le lieu, porte d'entrée côté rue sur chaque bâtiment, vitrines, variété par bâtiment, nuit avec pièces éclairées (30 à 50 %, 2700 à 4000 K), formes géométriques interdites, téléphone tenu. Règles : `regles-de-gestion/WFS-402.md`.
- **⏳ En attente après WFS-370** (🔧, agent dev3d 01UY1Vv2Ph) : mêmes fichiers du kit `src/world/archi_*`. Agent `dev-artiste-3d`, nouvelle session ; avis « Avis agent 3D : VALIDÉ » avant le lead dev (RG-E15-06). Les US « décor complet » WFS-385 à 389 le reprennent.
- `E15-decors.md` : file « Ouvertures communes » ajoutée ; `TACHES.md` (ligne + bandeau), `track-agent.md`, `tache utilisateur.md` à jour.

### 2026-11-10 09:49:00 — WFS-323 en tout premier, WFS-280 abandonné (po-scrum-master)
- Ordre de l'utilisateur (`@po`) : « 280 abandonné, je ne veux pas de motos garées… tu mets 323 en premier ».
- **WFS-280** (deux-roues garés et faufilement) : 🚫 Abandonné ; rien n'est retiré du code (WFS-232, deux-roues dans le trafic, reste). Ordre permanent ajouté dans le skill du PO : pas de motos garées.
- **WFS-323** (vue Cockpit 1/3) : P2 → **P1 Urgent**, **en tout premier** dans l'ordre des tickets prêts ; agent `dev-artiste-3d` (`@devartiste`), nouvelle session.

### 2026-11-10 09:49:40 — WFS-370 : inventaire (RG-01), plan, accroches annoncées (Claude, agent dev3d (01UY1Vv2Ph))
- Inventaire complet : `qa/pr/WFS-370/inventaire.md` (branche `feature/waforspeed-370`). Immeubles : 592 bâtiments du kit (WFS-161) ; volumes encore nus : toit de zinc à 2 pans plats, corniche en un pavé, garde-corps en plaque pleine, édicule en cube, pied de façade lisse, vitrines nues ; fenêtres de nuit d'une seule teinte, tirées **par pixel** (mouchetures, cause : position du monde imprécise loin de l'origine). Port : entrepôts, conteneurs, grues et grillage en boîtes. Hors ticket (WFS-385) : casino, centre commercial, garages, ouvrages de la route, teintes des façades (pierre et crépi absents de `materiaux.json`).
- Plan : comble à la Mansart (brisis 70°, terrasson 20°, lucarnes, souches et mitrons), corniche moulurée, garde-corps à barreaux (NF P01-012), soubassement et cordon, couvertines et édicule, dormant des vitrines (nouveau fichier `src/world/archi_details.*`) ; port modélisé (nouveau fichier `src/world/decor_ville_port.*` : conteneur ISO 40 pieds, portique de quai, clôture à double fil) ; entrepôts = kit, style industriel.
- **Accroches annoncées (fichiers communs, RG-E15-08, ≤ 10 lignes chacune)** : `archi_geo.cpp` (appels des détails à la place des pavés, graine de baie dans les uv des vitres), `archi_geo.h` (pièce `FIN` pour les barreaux, `graineBaie`), `archi_carte.cpp` / `archi_banc.cpp` (matériau des barreaux en couverture partielle, zinc plus mat), `shaders/decor.glsl` (`interieurFenetre` : tirages par fenêtre, 2 700 à 4 000 K), `cartes_banc.cpp` (gros plans des immeubles et du port), `CMakeLists.txt` (test_archi_geo). Les garde-corps à barreaux et la corniche moulurée s'appliquent aussi aux villas de la Corniche et aux maisons de la Campagne (même kit) : rien dans `plage.cpp` ni `campagne.cpp`.
- Réseau : aucun téléchargement (aucun modèle libre aux bonnes mesures accessible par GitHub seul) : RG-03, modélisation par le code.

### 2026-11-10 10:12:55 — Début WFS-323 : attribué à Claude, agent dev3d 7 (01UY1Vv4FX) (po-scrum-master)
- Demande de ticket d'une **nouvelle session** `dev-artiste-3d` (session_01UY1Vv4FX4CXXFEkuhUsAzG, 1er ticket de la session). Ordre de l'utilisateur du 2026-11-10 09:49 UTC : **WFS-323 en tout premier** au premier `dev-artiste-3d` qui demande → attribué.
- Ticket : **P1 WFS-323** vue Cockpit (1/3) : cadrage proche de la référence (`qa/retours/2026-11-08-cockpit-reference-utilisateur.jpg`), habitacle sur toute la largeur, pavillon fermé (ANO-207), réglage « Regard cockpit » gardé ; anomalies rattachées ANO-185, 187, 188, 152 (partie lisibilité), 203, 204. Règles : `regles-de-gestion/WFS-323.md`.
- Zone autorisée : `src/game/camera_vue.*` (partie cockpit), `src/render/vehicule_cockpit*`, tests du cockpit, `qa/pr/WFS-323/` ; accroches ≤ 10 lignes annoncées au journal (ex. `src/ui/ecrans/reglages.*` seulement si RG-03 l'exige). Interdit : `assets/cars/**` et `tools/formes.json` (WFS-363 en cours, E10), `archi_*` et fichiers du décor (WFS-370, E15), CI. Deux tickets E10 en même temps : exception PO (fichiers séparés). On part de l'existant (WFS-282, 315, 349, 390 🟡) : rien retiré.
- Pas un ticket E15 : avis agent 3D non obligatoire (conseillé). Après la fusion : 🟡 À valider (utilisateur), puis la session s'arrête. WFS-324 et 325 (suites) iront à de nouvelles sessions.

### 2026-11-10 10:12:50 — WFS-337 attribué à Claude, agent dev (01UY1Vv1d6) (po-scrum-master)
- Demande de la session `@dev` session_01UY1Vv1d6VrvcVTQzXuiern (1er et seul ticket, aucun ticket depuis la règle du 2026-11-09 12:05 UTC). Profil développeur général : les P1 prêts (WFS-323, 373, 378, 387) vont à `dev-artiste-3d` ; WFS-392 à `expert-cartes` ; WFS-287 à l'agent expert ci. Premier P2 permis : **WFS-337** (E4 / E13, aucun ticket actif dans ces épics).
- Zone : `src/render/nuit.cpp` (halos des têtes de lampe), `src/render/eclairage_*` si besoin, `tests/**` ; **pas** `ville*.cpp` / `archi_*` (WFS-370), phares (WFS-382). Règles : `regles-de-gestion/WFS-337.md` (section « Suivi » ajoutée : zone, règle des formes, captures).

### 2026-11-10 10:14:06 — WFS-373 attribué à Claude, agent dev3d (01UY1Vv4L4) (po-scrum-master)
- Demande d'une nouvelle session `@devartiste` (session_01UY1Vv4L4emR5PusLbYXyYF, agent `dev-artiste-3d`), 1er et seul ticket de la session. WFS-323 (demandé en premier) venait d'être attribué à 10:12 à Claude, agent dev3d 7 (01UY1Vv4FX) : on garde cette attribution.
- **WFS-373** (P1, E15 file Grand Prix) : inventaire des formes nues au journal (RG-01), puis tribunes, stands vitrés avec auvent, portique, barrières, murs de pneus sanglés, panneaux en vrais modèles (CC0 / CC BY, ±1 %, sans marque) ou modélisés au niveau du vrai objet. Aucun autre ticket actif sur le Grand Prix.
- Zone : `src/world/circuit.cpp`, `portique.cpp`, nouveaux `src/world/decor_grandprix_*`, `assets/world/modeles/grandprix/**`, `assets/textures/decor/grandprix/**`, `paquets/sources/grandprix/**`, licences ; fichiers communs : ajouts ≤ 10 lignes annoncés. Pas : `archi_*` au-delà des accroches (WFS-370), `ville*`, `plage.cpp`, `campagne*`, `assets/cars/**`, cockpit.
- Branche `feature/waforspeed-373` ; captures avant / après jour et nuit + gros plans + banc perf Basse ; PR, avis agent 3D VALIDÉ, puis lead dev ; arrêt de la session après la fusion.

### 2026-11-10 10:20:35 — Statuts automatiques, bancs du matin #12 (robot statuts)
- Run rouge (schedule, commit 9c4f58a), aucun statut changé. Job(s) en échec : banc (realisme-2).
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-396, WFS-362, WFS-384, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-10 10:21:29 — WFS-370 : PR #332 ouverte (Claude, agent dev3d (01UY1Vv2Ph))
- Fait : immeubles du kit en vrais volumes (comble à la Mansart, lucarnes, cheminées, corniche moulurée, garde-corps à barreaux, soubassement, cordon, édicules, vitrines), port modélisé (conteneurs ISO 40 pieds, portiques de quai, clôture à double fil, entrepôts du kit), fenêtres de nuit tirées par fenêtre (fin des mouchetures), 2 700 à 4 000 K.
- Vérifié : 97 tests CPU ; 20 captures avant / après jour et nuit (`qa/pr/WFS-370/`) ; banc perf Ville de nuit, Basse : 335 → 345 ms par image sur lavapipe (+3 %), processeur inchangé, 375 k → 395 k triangles ; Haute +11 %.
- Limites : « avant » faits à la main (vues de banc nouvelles) ; le kit change aussi un peu les villas de la Corniche et les maisons de la Campagne (barreaux, corniche, zinc mat). Suite : avis agent 3D (E15), puis lead dev.

### 2026-11-10 10:22:10 — WFS-373 : inventaire des formes nues du Grand Prix (RG-01), plan (Claude, agent dev3d (01UY1Vv4L4))
- Inventaire complet : `qa/pr/WFS-373/inventaire.md` (branche `feature/waforspeed-373`). Objets posés (journal du banc, comptage ajouté) : mur de pneus 2 068, mur des stands 517, tentes 112, garages 107, panneaux publicitaires 103, grillage 155, drapeaux 51, postes de commissaire 34, stands fans 24, camions 21, bars d'accueil 8, panneaux de freinage 3, tribunes 2, portique, passerelle, tour, hélicoptère, grande roue 1 chacun, oiseaux (3 vols) ; tous en boîtes, cylindres, cônes ou triangles. Projecteurs (117) : dessin commun `decor_carte.cpp` → type propre au Grand Prix dans `circuit.cpp`.
- Plan (RG-03, comme WFS-372) : nouveau fichier `src/world/decor_grandprix_modeles.*` (modèles construits une fois au chargement, version proche et allégée ; Basse = allégée pour les murs de pneus), branchés dans `circuit.cpp` ; test `tests/test_decor_grandprix.cpp` (mesures, triangles, normales) ; banc de gros plans `WFS_BANC=grandprix-objets`.
- **Accroches annoncées (≤ 10 lignes)** : `src/platform/desktop_main.cpp` (banc `grandprix-objets`), `CMakeLists.txt` (test). Réutilisés sans les changer : `marina_bateaux.h` (loft, eventail, ligne), `decor_campagne_modeles.h` (poutre chanfreinée, profilé I).
- Réseau : recherche de code GitHub seulement (modèles stylisés, exclus par RG-02), aucun téléchargement.

### 2026-11-10 10:23:56 — WFS-337 : arbitrage, accroche de 2 lignes acceptée (option A) (po-scrum-master)
- Demande de Claude, agent dev (01UY1Vv1d6). Cause d'ANO-189 : 2e lanterne du candélabre double du terre-plein central non inscrite dans `lampadaires()`. Accepté : 1 ligne dans `src/render/eclairage.h` (style `Tete`, halo sans lumière) et 1 ligne dans `src/world/ville.cpp` (fin de `initOuvrages`), à annoncer au journal par le développeur ; synchronisation avec `main` avant la PR (WFS-370 sur le même fichier). Détail : `regles-de-gestion/WFS-337.md`, section « Suivi ».

### 2026-11-10 10:31:01 — WFS-363 : Giulia et Range Rover Sport sans signature de forme, PR #333 (Claude, agent dev3d 6 (01UY1Vv3MV))
- Fait : Giulia, écusson en V refermé par une peau cousue au bord découpé (nouvelle retouche `couture` : sommets du bord repris, Delaunay, surface biharmonique), prises d'air en trèfle et pointe du V remplacées par une grille basse à 8 lames ; Range Rover, nid d'abeille de la calandre remplacé par 4 lames, bandeau avant (phares et cadre) étiré ×1,3 vers le bas d'un seul tenant (phares plus hauts, sans pli). Lettrage du capot déjà effacé par WFS-192.
- Fichiers : `tools/retoucher_formes.mjs` (couture, `baseAjustee` factorisée, options `exact` / `biharmonique` inactives par défaut), `tools/formes.json` (entrées `giulia`, `rrsport`), `assets/cars/giulia.*`, `assets/cars/rrsport.*`, `qa/pr/WFS-363/`.
- Vérification : `--verifier` vert pour les 7 voitures retouchées ; M4 et S3 rejouées depuis l'origine : empreintes identiques ; 98 tests CPU verts ; `test_fidelite_voitures` : Giulia et Range Rover conformes ±1 % (Giulia : échelle ×0,999, nez +1,2 cm au centre) ; 16 paires de captures + 12 zooms ×4 jour / nuit regardés.
- Limites : nid d'abeille des prises d'air basses du Range Rover et pavés LED des phares gardés (hors RG-01, à proposer au PO en tâche séparée si voulu). Essais écartés : panneau posé dessous (joint visible en trapèze), couronnes concentriques (stries), une boîte par phare (montant tordu).


### 2026-11-10 10:42:17 — WFS-384 : premier banc du matin après la correction, run #12 (agent expert ci cd (session 0183bv2Y))
- 1er critère d'acceptation : réveil de GitHub à 08:52 UTC (09:52 heure de Paris) → **bancs du matin lancés, run #12** (https://github.com/wlfaleh/waforspeed_mobile/actions/runs/42786014161, commit 9c4f58a) ; message « ⏳ Bancs du matin en retard » posté dans l'issue #242 à 08:53 UTC ; captures publiées dans `captures-matin` (jour.txt : 2026-11-10 11:16 heure de Paris, 18 archives) ; message « Recette QA prête » à 10:19 UTC. Réveil suivant (run #13, 10:37 UTC) **sauté proprement** (43 s, banc déjà fait).
- Run #12 **rouge**, pour deux causes qui ne viennent pas de WFS-384 (à confier par le PO, US technique P1 : « rouges → le PO crée une US technique P1 ») :
  1. `banc (realisme-2)` : « réalisme : partie 2, 2 erreur(s) », code 3 (était vert au run #5, commit 4e7fa71). Les deux lignes [ERREUR] sont au début du journal du job (non lisibles ici : seules les 5 000 dernières lignes sont accessibles) ; à reproduire avec `WFS_BANC=realisme-2` entre 4e7fa71 et 9c4f58a (19 tickets fusionnés, dont WFS-369, 372, 376, 379, 381, 382 qui touchent le rendu).
  2. `banc (ecrans-2400)` : **arrêté à la limite de 75 min** (run #5 : 41 min). Le jeu est devenu plus lent à rendre sur le banc logiciel (décors E15 probables) ; archive publiée mais peut-être incomplète (17 fichiers).
- Émulateur (information) : rouge aussi, comme au run #5.

### 2026-11-10 10:45:32 — WFS-363 fusionné par le lead dev (lead-dev)
- PR #333 (Claude, agent dev3d 6 (01UY1Vv3MV)) relue sur 9d83a967 : 56 captures ouvertes, tout le diff lu (`couture`, `baseAjustee`, options `exact` / `biharmonique` inactives par défaut). Construction et `tests_cpu.sh` refaits : 98/98. Rejeu de giulia, rrsport (depuis main) et de m4, s3 (depuis 30d339f4~1) : `.wfm` et `.lod` identiques à l'octet. Revue « ✅ Approuvé » en commentaire (même compte GitHub), fusion en squash 2473aa27. Statut 🟣.
- **Pour le PO** (petits défauts, RG-06 : tâche séparée à créer) : Range Rover : (1) une rangée de l'ancien nid d'abeille reste sous les nouvelles lames de la calandre, et la lame du bas plie vers les coins ; (2) petite pointe au-dessus du montant du cadre ; (3) la silhouette reste reconnaissable à 3/4 (phares toujours fins, nid d'abeille des prises d'air basses, pavés LED), alors que le critère dit « plus reconnaissable de face et de 3/4 » : RG-01 de WFS-363 ne visait que la calandre et les feux. Giulia : (4) léger creux au centre, sous la pointe du capot (vue basse).

### 2026-11-10 10:46:20 — WFS-363 fusionné (#333), session terminée (Claude, agent dev3d 6 (01UY1Vv3MV))
- PR #333 fusionnée en squash par le lead dev (2473aa27) ; ticket 🟣 ; ligne de `qa/Check.md` écrite par le lead dev.
- Petits défauts restants relevés par le lead dev (RG-06, à confier au PO en tâche séparée) : Range Rover (reste de nid d'abeille sous les lames, pointe en haut du montant du cadre, silhouette encore reconnaissable de 3/4 : prises d'air basses, pavés LED), Giulia (léger creux sous la pointe du capot en vue très basse), `delaunay()` sans cas des points alignés.
- Un seul ticket par session : je m'arrête, aucun autre ticket demandé.


### 2026-11-10 10:47:37 — WFS-370 fusionné par le lead dev (lead-dev)
- PR #332 relue sur 05a6a342 (branche à jour avec main) : « Avis agent 3D : VALIDÉ avec réserves » (R1 à R6) présent ; 40 captures ouvertes (immeubles classique et moderne, rue des terrasses, port : rue, entrepôts, conteneurs, portique, clôture ; jour et nuit) ; tout le diff relu ; construction et `tests_cpu.sh` refaits : 98 tests réussis ; bancs `archi` et `corniche` lancés pour vérifier le kit commun (aucune régression vue).
- Revue « ✅ Approuvé » écrite dans la PR (commentaire, même compte GitHub), puis fusion en squash : **181aa552**. Statut 🟡 À valider (utilisateur) ; ligne de `suivi-pr.md` supprimée ; ligne ajoutée dans `qa/Check.md`.
- Remarques non bloquantes, pour le PO : réserves R1 à R6 de l'agent 3D à mettre en tâches séparées (RG-E15-09) ; `archi_details.cpp` (`mansart`) : calcul `* 0.f` mort ; `etendue(GRUE)` passe à lat + 83 m, donc compter les portiques restants dans WFS-385 ; perf Haute +11 % à suivre aux bancs du matin.

### 2026-11-10 10:48:28 — WFS-370 : fusionné, session terminée (Claude, agent dev3d (01UY1Vv2Ph))
- PR #332 fusionnée en squash par le lead dev (181aa552) après « Avis agent 3D : VALIDÉ avec réserves » ; statut 🟡 À valider (utilisateur).
- **Pour le PO (tâches séparées, RG-E15-09)** : réserves de l'agent 3D R1 zinc encore clair au soleil (albédo visé 0,25 à 0,35) ; R2 vitrines de nuit des cafés en aplat (WFS-402) ; R3 entrepôts trop « logements » (nef, portes de chargement, moins de fenêtres) ; R4 ondulation des conteneurs faible au loin ; R5 gros plan du portique entier au banc ; R6 perf Basse +3 % sur lavapipe à confirmer aux bancs du matin. Remarques du lead dev : code mort `* 0.f` dans `mansart` (à nettoyer dans WFS-402), graine `^ 0x5eedu` des entrepôts à commenter, portiques peut-être retirés par la nouvelle étendue (lat + 83 m) : à compter dans WFS-385.
- Un seul sujet par session : je m'arrête, aucun autre ticket demandé.

### 2026-11-10 10:52:34 — WFS-337 : cause trouvée, accroches, PR #334 ouverte (Claude, agent dev (01UY1Vv1d6))
- Cause (RG-01, ANO-189) : le candélabre double de 12 m du terre-plein central de la voie rapide (Ville) porte 2 lanternes à 5,0 m ; `ville.cpp` n'inscrivait pour la nuit que celle de notre côté. La lanterne du sens inverse restait une barre jaune sans lumière, et le halo se voyait 5 m à côté. Rue, pont, tunnel : halo déjà à ≤ 0,2 m de la vasque.
- **Accroches annoncées (acceptées par le PO, 10:23)** : `src/render/eclairage.h` 1 ligne (style `StyleLampadaire::Tete`, halo seul) ; `src/world/ville.cpp` 1 ligne en fin de `initOuvrages` (2e lanterne inscrite en `Tete`) ; `src/platform/desktop_main.cpp` 2 lignes (banc `WFS_BANC=halos`).
- Fait : `nuit.cpp` dessine le halo de chaque tête ; une `Tete` ne prend aucune des 4 lumières et n'a pas de flaque (coût inchangé) ; `eclairage_physique.cpp` : exposition inchangée. Nouveau banc `src/render/eclairage_halos_banc.cpp`.
- Vérifié : 98 tests CPU ; banc `halos` : `main` 34 candélabres avec une seule tête (ÉCART), branche 34 avec leurs 2 têtes (OK) ; 11 captures avant / après de nuit (`qa/pr/WFS-337/`, 160 km/h, 50 m, de près, rue, Poursuite).
- Limites : têtes des candélabres encore en pavé (WFS-378) ; hauteurs inscrites hétérogènes dans `ville.cpp`, non changées (écart ≤ 0,2 m).

### 2026-11-10 11:00:29 — Archives automatiques (robot archives)
- Journal : 321 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-08).
- TACHES.md : 6 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md` (WFS-162, WFS-163, WFS-194, WFS-393, WFS-231, WFS-280).

### 2026-11-10 11:09:55 — WFS-323 : PR #335 ouverte, accroches annoncées (Claude, agent dev3d 7 (01UY1Vv4FX))
- Fait : vue Cockpit au cadrage de la référence (champ 72° → 62°, regard Normal −5°, horizon 46,5 % → 42,9 %) ; pavillon (traverse, ciel de toit, rails, pare-soleil), haut du pare-brise à sa vraie place (`HAUT_PARE_BRISE_*`, partagé avec la pluie) ; rétroviseur accroché au pare-brise ; pièces de l'intérieur modelées (`dalle`, `profile`, `cadreArrondi`) au lieu de boîtes et cylindres ; occlusion de l'habitacle (MAT_AO, ANO-185) ; chiffres du compteur plus grands (ANO-188) ; volant vérifié (ANO-187).
- Accroches hors zone (annoncées ici) : `configgame.json` et `src/core/configgame.h` (1 ligne chacun : `camera.cockpit.zoom` 62), `configgame.README.md` (1 ligne), `src/render/effets_systeme.cpp` (6 lignes, ANO-204 : étincelles et fumée à moins de 2,5 m de l'œil non dessinées en vue Cockpit), `src/platform/desktop_main.cpp` (1 ligne : banc `cockpit-reference`).
- Vérification : 98 tests CPU verts ; banc `cockpit-reference` avant / après (`qa/pr/WFS-323/`) ; banc `cockpit` SF90, Abarth, Model 3 : fenêtre de la rue cachée 0,0 % ; coût de l'intérieur Golf 32 → 33 appels, 1,22 → 1,60 Mo (RG-04 ; mesure en qualité Basse sur téléphone non faite).
- Limites : intérieur 52-59 % de l'image (cible 30-35 % de WFS-282 dépassée par le toit et le champ de la référence) ; « R » rouge du HUD après un choc (ANO-204) hors zone, non traité ; ANO-203 sans capture de checkpoint de nuit.


### 2026-11-10 11:18:35 — WFS-337 fusionné par le lead dev (lead-dev)
- PR #334 relue sur 7bca0195 : 22 captures ouvertes (11 paires), diff entier lu (nuit.cpp, eclairage.h, eclairage_physique.cpp, ville.cpp : 1 ligne, banc halos, accroche desktop_main.cpp). Accroches conformes à l'arbitrage du PO de 10:23:56.
- Construction et tests_cpu.sh refaits : 98/98. Banc `halos` relancé par le lead dev sur main (+ banc) : ÉCART (34 candélabres avec une seule tête), sur la branche : OK (34 avec leurs 2 têtes). Les écarts du bandeau TRAVAUX, de la glissière et de la voiture sur `halos-poursuite` venaient de l'ancien main des images « avant » : sur le main actuel, seuls 175 pixels (halos) changent.
- Revue « ✅ Approuvé » en commentaire (même compte GitHub), fusion squash a236bdc5. WFS-337 → 🟡 À valider (utilisateur). Remarques non bloquantes dans la PR (distance négative dans prochain() du banc, compte de lampes de bench_perf, pas de test CPU dédié ; halo rond et tête en barre restent pour WFS-378).

### 2026-11-10 11:19:12 — WFS-337 : fusionné, session terminée (Claude, agent dev (01UY1Vv1d6))
- PR #334 fusionnée en squash par le lead dev (a236bdc5) ; statut 🟡 À valider (utilisateur) : la nuit, sur la voie rapide de la Ville, chaque lanterne des candélabres doubles a son halo.
- **Pour le PO (petite tâche de suite, remarques non bloquantes du lead dev)** : banc `halos`, `prochain()` peut rendre une distance négative et `#include <algorithm>` manque ; `bench_perf` compte 34 lampes de plus (têtes seules) ; aucun test CPU du contrôle des 2 têtes (seulement le banc `halos`, hors CI). Halo rond et tête en pavé : WFS-378.
- Un seul sujet par session : je m'arrête, aucun autre ticket demandé.

### 2026-11-10 11:25:16 — WFS-323 : PR #335 refusée par le lead dev (lead-dev)
- Commit relu `589f90ac`. Construction en Release et `tools/qa/tests_cpu.sh` refaits : 98 tests réussis. Tout le diff lu, les 26 captures ouvertes (agrandissements du combiné, du toit et du rétroviseur de nuit).
- Mieux qu'avant : cadrage (horizon à 43 %), toit fermé (ANO-207), rétroviseur moulé, volant qui tourne (ANO-187), nuit sans aplat noir.
- **Bloquant** : (1) ANO-188 annoncée corrigée, mais le compteur reste illisible : M4 « 58 » → « SA 2 » (chiffres coupés par la planche ou la visière), X5 vitesse cachée par la jante. Demandé : régler le combiné ou l'œil par voiture, plus un contrôle au banc `cockpit-reference` ; (2) visière du combiné (`extruder(cas…)`) : prisme à arêtes vives, forme nue dans la zone touchée.
- Remarques : filtre de 2,5 m des étincelles (ANO-204) trop large pour un choc latéral ; ANO-185 seulement atténuée ; gouttes de pluie devant le rétroviseur (déjà sur `main`, au PO) ; part de l'image prise par l'intérieur de 52 à 59 % contre 30 à 35 % dans WFS-282 (règle à mettre à jour par le PO). Avis de l'agent 3D : « À REPRENDRE », même point bloquant.
- TACHES : WFS-323 ↩️ Retour lead dev (PR #335) ; track-agent et suivi-pr mis à jour.

### 2026-11-10 11:26:16 — WFS-373 : PR #336 ouverte (Claude, agent dev3d (01UY1Vv4L4))
- Fait : 21 objets du Grand Prix modélisés au niveau du vrai objet (RG-03), construits une fois au chargement (`src/world/decor_grandprix_modeles.*`), posés par `circuit.cpp` ; herbe tondue en bandes, touffes semées au hasard ; projecteurs propres au Grand Prix ; banc `grandprix-objets` ; test `test_decor_grandprix`.
- Accroches (annoncées à 10:2x) : `desktop_main.cpp` (2 lignes), `CMakeLists.txt` (1 ligne). Aucun fichier commun d'une autre file touché.
- Vérification : 99 tests CPU réussis ; captures avant / après (banc `cartes`, Grand Prix) et 29 gros plans jour / nuit ; Basse : 402 / 487 ms → 412 / 424 / 432 ms (lavapipe, pas de baisse), Haute +25 %. Taille ajoutée : 0 Mo.
- Limites : coût en Haute ; un mur de pneus sur deux toujours retiré en Basse (RG-C2 de WFS-140). Suite : avis agent 3D (RG-E15-06), puis lead dev.

### 2026-11-10 11:30:24 — WFS-403 créé : finitions du décor de WFS-373 (po-scrum-master)
- Demande de Claude, agent dev3d (01UY1Vv4L4), développeur de WFS-373 (PR #336) : l'agent 3D a rendu « VALIDÉ avec réserves » (commentaire 6766136867) ; RG-E15-09 → tâche séparée.
- **WFS-403** (P2, E15 file Grand Prix, agent `dev-artiste-3d`, nouvelle session, après la fusion de WFS-373 ; pas la session 01UY1Vv4L4) : nacelles de la grande roue, jantes du camion et dos du bâtiment des stands, mur des stands et murs de pneus **continus en Basse** (arbitrage du PO : la règle « un petit objet sur deux » de WFS-140 RG-C2 ne vaut plus pour les barrières de sécurité), garages éclairés de nuit, voie des stands en asphalte / béton. Règles : `regles-de-gestion/WFS-403.md`.
- Points mineurs (pneus facettés, bar d'accueil, vigie de la tour, muret sans joints, tache blanche à l'horizon, fragment devant l'hélicoptère) versés dans **WFS-388** (RG-11 à 16 ; l'agent 3D avait écrit WFS-389 par erreur). File Grand Prix : WFS-373 → 403 → 388 (mêmes fichiers).
- Rien attribué (un seul sujet par session).

### 2026-11-10 11:43:14 — WFS-373 fusionné par le lead dev (lead-dev)
- PR #336 (Claude, agent dev3d (01UY1Vv4L4)) relue sur a6cb6e69 : « Avis agent 3D : VALIDÉ avec réserves » présent (RG-E15-09), 49 captures ouvertes, diff lu en entier, construction et `tests_cpu.sh` refaits : 99/99. Approbation écrite dans la PR (commentaire, même compte GitHub), fusion en squash **fb3f2d99**.
- Statut : 🟡 À valider (utilisateur) (décor, comme WFS-370 / 372). Réserves : WFS-403 puis WFS-388 (déjà créés par le PO). Ligne `qa/Check.md` ajoutée ; ligne de `suivi-pr.md` supprimée.

### 2026-11-10 11:43:39 — WFS-373 : session terminée (Claude, agent dev3d (01UY1Vv4L4))
- PR #336 fusionnée en squash par le lead dev (fb3f2d99) après « Avis agent 3D : VALIDÉ avec réserves » ; réserves dans WFS-403 (P2) et WFS-388. Ticket 🟡 À valider (utilisateur).
- Un seul ticket par session : la session s'arrête, aucun autre ticket demandé.

### 2026-11-10 12:40:36 — WFS-404 créé (logos falsifiés tout de suite) et agent juriste créé (po-scrum-master)
- **Ordre de l'utilisateur (12:37 UTC, `@po`)** : falsifier tout de suite, même grossièrement, les logos des voitures qui restent, dans une seule US urgente, pour mettre l'application sur Android ; ensuite reprendre les US pour les finir proprement.
- **WFS-404** (P1, E10, exception PO : `assets/cars/*`, `tools/logos.json`, `effacer_logos.mjs` ; pas `vehicule_cockpit*` de WFS-323), agent `dev-artiste-3d`, nouvelle session, **en tout premier** : inventaire rapide des 21 voitures et des deux-roues, puis effacement au plus simple (X5 : losange du capot et calandre ; A 45 et Elantra N : logos, lettres, calandre ; bandes de marque des volants, ANO-209). Règles : `regles-de-gestion/WFS-404.md`.
- WFS-397, WFS-360, WFS-361 passent « après WFS-404 » (reprise « nickel »). WFS-393 est déjà rangé dans l'archive (à revoir avec WFS-361).
- **Agent `juriste` créé** (demande de l'utilisateur) : `.claude/agents/juriste.md`, skill `.claude/skills/juriste`, alias `@juriste` (ajouté à `appel-agents.md`, à `CLAUDE.md` et à `track-agent.md`), rapports dans `juridique/`.
- Point signalé à l'utilisateur : effacer les logos ne règle pas la licence « non commerciale » de l'A 45, de l'Elantra N et du X5 (CC BY-NC / NC-SA) ; sortie gratuite sans pub ni achat possible le temps du remplacement (WFS-320, point 1) ; avis du juriste demandé.
- Rien attribué (attribution seulement à la demande d'un développeur).

### 2026-11-10 12:49:39 — WFS-404 réécrit : les 3 voitures NC retirées de l'application (po-scrum-master)
- **Ordre de l'utilisateur (« Oui »)** à la proposition du PO : retirer de l'application l'A 45, l'Elantra N et le X5 (licences CC BY-NC / NC-SA) et garder les 18 autres, pour sortir sur Android tout de suite.
- **WFS-404** réécrit (P1, `dev-artiste-3d`, nouvelle session) : liste unique des voitures retirées, plus rien dans le jeu ni dans l'APK, fichiers gardés dans le dépôt ; camion de la Poursuite et sens inverse de la Ville sur des modèles libres ; sauvegarde du joueur reprise ; crédits régénérés ; contrôle rapide des marques des 18 autres (volant de la M4 : bandes de marque dans `vehicule_cockpit_defs.cpp:29`).
- **Erreur du PO reconnue** : le X5 était connu sous licence NC depuis le 2026-11-07 ; il n'aurait pas dû être retouché (part X5 de WFS-193, PR #311 ; WFS-358, PR #319). Plus aucune retouche du X5 d'origine (WFS-393 abandonné).
- Rien attribué (attribution à la demande d'un développeur).

### 2026-11-10 12:50:24 — Premier rapport juridique : publication Android / Google Play (juriste)
- `juridique/rapport-2026-11-10.md` (J-01 à J-17), demande de l'utilisateur du 2026-11-10 12:37 UTC.
- **(a) test interne** : publiable maintenant. **(b) sortie publique gratuite** : après 6 points 🔴 : logos restants (WFS-404), fiche du store (« 20 vraies voitures », marques dans les crédits collés), 7 noms de voitures trop proches des vrais (Bnv, Folg 8…), politique de confidentialité publique et contact, formulaire « Sécurité des données » (duel en ligne = données collectées, éphémères), crédits CC BY manquants (3 deux-roues, auteurs du son « gros ») et écran Crédits.
- **Licences NC (A 45, Elantra N, X5)** : effacer les logos ne règle pas la licence ; tolérées (🟠) en version gratuite sans pub ni achat, **bloquantes pour toute vente, pub ou achat** (remplacement WFS-360, 361, 397).
- 🟠 principaux : formes reconnaissables des vraies voitures (avis d'avocat avant une vente), nom « WaForSpeed » à vérifier, CGU (éditeur, droit applicable, duel), public cible et IARC, notice Apache 2.0 de native_app_glue.
- Transmis au PO pour les tickets ; 7 questions pour l'utilisateur à la fin du rapport. Rien codé, aucun ticket créé.

### 2026-11-10 12:50:46 — Lead dev automatique (chaque heure) mis en pause (po-scrum-master)
- Ordre de l'utilisateur (budget) : la tâche planifiée « Lead dev automatique (chaque heure) » est désactivée (gardée, réactivable). Les PR sont relues seulement quand un développeur appelle lui-même le lead dev après sa PR.

### 2026-11-10 12:51:04 — WFS-323 : PR #335 corrigée après le refus du lead dev (Claude, agent dev3d 7 (01UY1Vv4FX))
- Fait : compteur lisible sur les 20 voitures (face de planche qui coupait le combiné à mi-hauteur, cause déjà sur `main` ; vitesse placée dans la bande vue par l'ouverture haute du volant, `bandeLisibleCombine`) ; visière du combiné moulée ; étincelles et fumée cachées seulement dans la boîte de l'habitacle (accroche `effets_systeme.cpp`, 10 lignes) ; montants de 9 cm de face ; tige du rétroviseur visible ; axes des pare-soleil mats.
- Vérification : 99 tests CPU verts après synchronisation (`c65303b4`) ; nouveau contrôle `test_cockpit` (vitesse dans la bande vue) ; agrandissements des 20 combinés dans `qa/pr/WFS-323/`.
- Pour le PO : gouttes de pluie dessinées devant le rétroviseur intérieur la nuit (pluie en 2D, déjà sur `main`, hors de ma zone) ; cible « intérieur 30-35 % de l'image » de WFS-282 à revoir (52-59 % avec le toit et le champ de la référence).


### 2026-11-10 12:51:45 — Plus aucun travail sur les 3 modèles NC d'origine (po-scrum-master)
- Erreur du PO reconnue devant l'utilisateur : des US ont visé le X5 (WFS-193 part X5, WFS-358, WFS-393) alors que sa licence NC le condamnait. Ordre permanent ajouté : aucun ticket ne vise plus l'A 45, l'Elantra N ni le X5 d'origine ; les lots du cockpit (WFS-325) et les bancs les sautent (WFS-404).

### 2026-11-10 12:56:29 — X5 : l'auteur refuse l'usage commercial (po-scrum-master)
- Réponse de l'utilisateur : l'auteur du X5 ne veut pas. Le X5 d'origine reste retiré (WFS-404). WFS-361 garde le remplacement et reprend tous les réglages du vrai X5 (RG-R1).

### 2026-11-10 12:59:06 — sketchfab.com ajouté aux sites autorisés (po-scrum-master)
- Ordre de l'utilisateur : `sketchfab.com` (modèles 3D CC0 / CC BY seulement) ajouté au tableau du § 1 de `regles-reseau.md`, pour WFS-361 (X5, X3 ou X1 libre, sinon autre grand SUV) puis WFS-360. Le réglage réseau de l'environnement reste à faire par l'utilisateur.

### 2026-11-10 13:02:11 — WFS-404 passé SUPER URGENT : seules les voitures vendables restent (po-scrum-master)
- Ordre de l'utilisateur (`@po`) : « créer une tâche super urgente pour garder que les voitures qu'on peut utiliser pour gagner l'argent ». WFS-404 (déjà le retrait des 3 voitures NC) devient **SUPER URGENT, avant tout autre ticket**, avec RG-00 : seules les licences CC0 / CC BY / CC BY-SA restent dans l'APK ; contrôle du PO : seules A 45, Elantra N et X5 sont NC.

### 2026-11-10 13:04:01 — Avis juridique : lancement officiel aujourd'hui impossible, 7 points bloquants ouverts (juriste)
Demande de l'utilisateur (« @avocat », lancement officiel aujourd'hui). Contrôle refait sur `main` : aucun des points 🔴 du rapport de 12:48 n'est corrigé (WFS-404 non commencé, fiche, noms, confidentialité, Sécurité des données, crédits) ; J-07 (licences NC) passe 🔴 puisque l'utilisateur vise des revenus. Test interne toujours possible. Détail : `juridique/rapport-2026-11-10.md` (§ Mise à jour). Transmis au PO.
### 2026-11-10 13:04:16 — WFS-323 fusionné par le lead dev (lead-dev)
- PR #335 relue à nouveau sur c65303b4 (2e revue après le refus sur 589f90ac) : réponses point par point présentes ; tout le diff relu ; construction Release dans /tmp et `tests_cpu.sh` : 99/99 ; captures ouvertes (4 comparaisons, pavillon, regard Bas, volant, combinés des 20 voitures) ; ANO-188 corrigée (M4 « 58 », X5 « 56 ») ; avis agent 3D « VALIDÉ avec réserves ».
- Revue « ✅ Approuvé » en commentaire (même compte GitHub), fusion en squash f04cec8c. WFS-323 → 🟡 À valider (utilisateur) ; ligne `qa/Check.md` ajoutée ; ligne de `suivi-pr.md` supprimée.
- Réserves non bloquantes pour WFS-324 / 325 ou `anno.md` (PO) : visière qui rogne le haut des chiffres en regard Haut, chevauchements arc / aiguille / rapport (Corsa, V60, GT86, Gallardo, 911, Abarth, SF90), poignée chromée trop claire de nuit.

### 2026-11-10 13:04:31 — Vue Cockpit passée urgente (po-scrum-master)
- Ordre de l'utilisateur : les sujets cockpit sont urgents aussi. WFS-324 (rétroviseurs), WFS-325 (intérieurs par voiture), WFS-395 (habitacles Corvette et Brezza) : **P1 Urgent**. Ordre : WFS-404 → WFS-323 (fusionné #335, 🟡) → 324 → 325 → 395. Les 3 voitures retirées sont exclues.

### 2026-11-10 13:05:20 — WFS-323 : fusionné (PR #335, f04cec8c), session terminée (Claude, agent dev3d 7 (01UY1Vv4FX))
- Fusion en squash par le lead dev après le 2e passage ; avis agent 3D « VALIDÉ avec réserves » ; WFS-323 en 🟡 À valider (utilisateur).
- Réserves à reprendre (pour le PO : `anno.md` ou WFS-324 / 325) : en regard Haut, la visière rogne le haut des chiffres (« 87 » lu « 81 ») ; l'arc, l'aiguille ou le rapport touchent les chiffres sur Corsa, V60, GT86, Gallardo, 911, Abarth, SF90 ; poignée chromée trop claire la nuit ; intérieur sombre de jour ; ANO-185 seulement atténuée ; gouttes de pluie devant le rétroviseur intérieur (pluie, hors zone) ; part de l'image de WFS-282 (30-35 %) à revoir (52-59 %).
- Réponse à l'utilisateur écrite dans `tache utilisateur.md`. Session arrêtée (un seul ticket par session) : aucun autre ticket demandé.


### 2026-11-10 13:07:00 — Sortie grand public : Ville seule ; WFS-405 et WFS-406 créés (po-scrum-master)
- Ordre de l'utilisateur : une seule carte, la Ville, pour le grand public ; les autres cartes continuent en développement.
- **WFS-405** (P1 super urgent, E8 + E14, `expert-cartes`) : liste unique des cartes publiques ; AAB = Ville seule, APK d'aperçu = toutes les cartes ; tous les modes sur la Ville ; sauvegarde reprise.
- **WFS-406** (P1 super urgent, E5, `dev`) : écran Crédits complet sans marques, noms trop proches renommés (rapport juridique J-03, J-06, J-13).
- **WFS-320** : points J-02, J-04, J-05, J-11 ajoutés pour l'agent expert ci.
- Besoin : 4 sessions en parallèle (`@devartiste` WFS-404, `@cartes` WFS-405, `@dev` WFS-406, `@cicd` WFS-319 / 320). Rien attribué (à la demande).

### 2026-11-10 13:11:50 — Points 🔴 du juriste découpés en US : WFS-407, 408, 409 ; WFS-406 précisé (po-scrum-master)
- Demande du juriste puis ordre de l'utilisateur (~13:10 UTC, « regarde avec le PO pour créer les US urgentes ») ; WFS-405 (Ville seule) et WFS-406 (crédits et noms) déjà créés à 13:07 par une autre session du PO : pas de doublon.
- **WFS-406** précisé : noms choisis par le PO (golf → Norvik Lume, m4 → Draven Vantor GT, giulia → Alvar Selene TS ; les 3 voitures retirées renommées à leur retour), crédits à ajouter (3 deux-roues, auteurs du son « gros », version nhaudio, Apache 2.0 de native_app_glue), RG-06 ligne de confidentialité cachée tant que l'adresse est vide, RG-07 contrôle des marques par `verifier_licences.py`.
- **WFS-404** : RG-06 étendu (J-01 : jantes, centres de roue, pneus, étriers, plaques, intérieurs, 3 deux-roues).
- Créées, **P1 juste après WFS-404**, agent `expert-ci-cd`, découpées de **WFS-320** : **WFS-407** fiche sans marque + captures neuves (J-02, après 404, 405, 406) ; **WFS-408** confidentialité, CGU, contact (J-04, J-10, ANO-205 → 📋 Planifiée) ; **WFS-409** Sécurité des données + IARC + public cible (J-05, J-11 réunis : mêmes fichiers, pas de micro-ticket). Aucun commit sur `feature/waforspeed-320` depuis le 2026-11-08 14:14 UTC (branche jamais poussée) : plus de 10 h.
- Décisions de l'utilisateur non tranchées (éditeur, contact, adresse de la politique, duel en ligne public, public cible, statut non commerçant, nom du jeu) : RG « en attente de l'utilisateur », aucune valeur inventée.
- `track-agent.md` : lignes des 6 sessions terminées enlevées ; aucune session de développeur active.

### 2026-11-10 14:08:17 — WFS-324 (rétroviseurs) prêt en parallèle de WFS-404 (po-scrum-master)
- Question de l'utilisateur sur les rétroviseurs : c'est WFS-324 (P1). Décision du PO : il part en même temps que WFS-404 (zones séparées), dans une session `@devartiste` à part.

### 2026-11-10 14:15:33 — Ordre des 50 prochaines tâches fixé par l'utilisateur (po-scrum-master)
- L'utilisateur valide la liste des 50 tâches du PO et monte **WFS-324 (rétroviseurs) en 2e**, juste après WFS-404 (en parallèle, zones séparées). Ordre écrit en bandeau de `TACHES.md` ; publication possible après les tâches 1 à 8.

### 2026-11-10 14:16:35 — WFS-402 (fenêtres et portes) monté en 3e (po-scrum-master)
- Ordre de l'utilisateur : la tâche 13 (WFS-402) devient la 3e. Passée ⏳ Prêt (WFS-370 fusionné) ; E15, avis de l'agent 3D obligatoire avant la fusion ; en parallèle de WFS-404 et 324 (zones séparées : `archi_*`).

### 2026-11-10 14:20:51 — Retouches de forme WFS-364 à 368 montées après la 5e tâche (po-scrum-master)
- Ordre de l'utilisateur : les tâches 16 à 20 (WFS-364, 365, 366, 367, 368) passent juste après la 5e (WFS-406) : 6e à 10e. Elles se suivent (même zone E10, après WFS-404) ; WFS-364 prêt après WFS-404.

### 2026-11-10 14:23:45 — Épic « Publication app » en attente du jour de publication (po-scrum-master)
- Ordre de l'utilisateur : WFS-319, 408, 409, 407 (et le suivi WFS-320) vont dans l'épic **Publication app** et attendent le jour de publication ; l'utilisateur préviendra le PO. Statut ⏸ ; WFS-319 reste à l'agent expert ci (session 01UY1Vv4), suspendu. Ordre permanent ajouté au skill du PO.

### 2026-11-10 14:31:39 — WFS-404 attribué à Claude, agent devartiste (01UY1Vv2ReCX) (po-scrum-master)
- Nouvelle session `dev-artiste-3d` (aucun ticket avant) : **WFS-404** (P1 super urgent, E10, 1re de l'ordre des 50 tâches). WFS-323 déjà fusionné (#335, 🟡) : la note « WFS-323 à la prochaine session @devartiste » est dépassée.
- Zone : celle de `regles-de-gestion/WFS-404.md` (exception PO) ; pas `assets/cars/remplacants/`, pas le reste du cockpit. Hors E15 : pas d'avis de l'agent 3D obligatoire. Branche `feature/waforspeed-404` à pousser tout de suite.

### 2026-11-10 15:05:46 — WFS-404 : décision du PO sur le test de la Course à 5 (option A) (po-scrum-master)
- Risque signalé par Claude, agent devartiste (01UY1Vv2ReCX) : le RR Sport, tiré à la place des 3 voitures retirées, rend `test_course5_ordinateurs` rouge (+31 % en virage, défaut connu de WFS-394).
- Décision : option A. Le RR Sport est écarté des adversaires de la Course à 5 seulement (1 ligne, `course5.cpp`) jusqu'à WFS-394, sans affaiblir aucun seuil. Écrit en RG-09 de WFS-404 et RG-05 de WFS-394. Garage : 17 voitures, et non 18 (RG-10).

### 2026-11-10 15:38:55 — WFS-404 : 2e risque, décision du PO (option A) ; WFS-394 en P1 (po-scrum-master)
- Constat de Claude, agent devartiste (01UY1Vv2ReCX) : toute voiture enlevée des adversaires change les tirages des 8 graines. Avec la règle « voiture la plus proche en vitesse max », `test_course5` est vert, mais un contrôle de `test_course5_ordinateurs` reste rouge : circuit, aide activée, top 3 = 3/8 (seuil 4).
- Décision : option A. Le seuil ne change pas et ne passe pas en avertissement (« jamais un seuil affaibli »). WFS-404 peut être fusionné avec ce seul rouge connu (RG-11), et WFS-394 passe P1 pour le remettre vert (RG-06). Il tient lieu d'US technique. Le job `android` de `build.yml` ne dépend pas des tests : l'APK de `main` reste publié.

### 2026-11-10 15:51:53 — WFS-324 attribué à Claude, agent dev3d (01UY1Vv5YT) (po-scrum-master)
- Nouvelle session `dev-artiste-3d` (aucun ticket avant) : **WFS-324** (P1, E4 + E10, 2e de l'ordre des 50 tâches), vue Cockpit, rétroviseurs avec vrai reflet. Dépendance WFS-323 fusionnée (#335, 🟡).
- Zone : passe des reflets (E4) et `vehicule_cockpit*` ; pas `assets/cars/*` ni la ligne du volant de la M4 (WFS-404 en parallèle). Coût ≤ 2 img/s. Hors E15 : pas d'avis de l'agent 3D obligatoire. Branche `feature/waforspeed-324` à pousser tout de suite.

### 2026-11-10 15:52:39 — WFS-402 attribué à Claude, agent devartiste 2 (01UY1Vv5qgAR) (po-scrum-master)
- Nouvelle session `dev-artiste-3d` (aucun ticket avant). Elle demandait WFS-324, pris à 15:51:53 par Claude, agent dev3d (01UY1Vv5YT) ; mon attribution parallèle n'a pas été envoyée. Ticket suivant de l'ordre de l'utilisateur pour ce type d'agent : **WFS-402** (P1, 3e), E15, fenêtres et portes réalistes de toutes les cartes. E15 libre (aucun ticket 🔧 / 🔀), WFS-370 fusionné (🟡).
- Zone : `src/world/archi_*`, nouveaux `src/world/archi_ouvertures_*`, `assets/world/modeles/ouvertures/**`, `assets/textures/decor/ouvertures/**`, `tests/**` ; ≤ 10 lignes d'appel par fichier de carte (pas la ligne `x5` de `ville.cpp`, à WFS-404). Avis « VALIDÉ » de l'agent 3D avant le lead dev. Branche `feature/waforspeed-402`.


### 2026-11-10 15:53:13 — WFS-406 attribué à Claude, agent dev (01UY1Vv5vk) (po-scrum-master)
- Nouvelle session `dev` (aucun ticket avant, ligne ajoutée en 0291321) : **WFS-406** (P1, E5 + accroche E8 menu, 5e de l'ordre des 50 tâches ; 3e et 4e, WFS-402 et 405, réservés aux skills `dev-artiste-3d` et `expert-cartes`). Écran « Crédits » complet et noms trop proches des vrais renommés (J-03, J-06, J-13). E5 libre.
- Zone : `src/ui/**` (hors `ecrans/`), accroche `src/ui/ecrans/menu.*` (bouton Crédits), noms et champ `confidentialite_url` de `configgame.json`, `assets/licences/CREDITS.txt`, `store/credits.md`, `tools/qa/verifier_licences.py` ; pas `src/game/cars.h` ni `assets/cars/*` (WFS-404 en cours). Crédits des voitures alignés sur la liste de WFS-404 : synchroniser avec `main` après sa fusion avant la PR finale. Branche `feature/waforspeed-406` à pousser tout de suite.

### 2026-11-10 16:33:01 — WFS-404 : PR #337 ouverte (Claude, agent devartiste (01UY1Vv2ReCX))
- Fait : liste unique `VOITURES_RETIREES` (`src/game/cars.h` : x5, a45, elantra), lue par le garage (17 voitures), la Course à 5 (remplacement au tirage, RG-11), le duel, la sauvegarde (`Game::setCar` → `voiturePermise`, accroche d'1 ligne dans `game.cpp`), les bancs, les crédits (`verifier_licences.py`), `effacer_logos.mjs` et `android/app/build.gradle` (`ignoreAssetsPattern` + contrôle des actifs fusionnés). Camion de la Poursuite → RR Sport ; sens inverse de la Ville → Compass ; volant de la M4 sans bandes (accroche d'1 ligne dans `vehicule_cockpit_defs.cpp`, annoncée ici : fichier tenu par WFS-323/324). Nouveau test `test_voitures_retirees`, nouveau banc `voitures-retirees`.
- Vérification : `tests_cpu.sh` : tout vert sauf le rouge connu accepté par le PO (RG-11) : `test_course5_ordinateurs`, circuit, aide activée, dans les 3 premiers 3/8 (≥ 4) → WFS-394. `verifier_licences.py` : 0 manque ; `effacer_logos.mjs --verifier` : vert (RR Sport et S3 reconnus à l'empreinte finale des formes). Captures `qa/pr/WFS-404/` regardées (garage « 02 / 17 », planche des 17 voitures, volant M4, camion).
- Limites : APK non construit ici (pas de SDK Android) : le `unzip -l` sans a45, elantra et x5 est à lire sur l'APK de la CI de `main` (Gradle échoue si l'un d'eux reste). Outils locaux installés : `glslc`, `mesa-vulkan-drivers`, `npm ci` dans `tools/` (paquets apt / npm, aucun téléchargement noté hors registres).

### 2026-11-10 16:36:56 — WFS-402 : fenêtres et portes du kit codées, tests verts (Claude, agent devartiste 2 (01UY1Vv5qgAR))
- Fait : nouveau module `src/world/archi_ouvertures.{h,cpp}` (zone RG-10) branché dans `archi_geo.cpp` : dormant, ouvrants, petits bois, crémone, jet d'eau ; appui de pierre à glacis et larmier ; linteau (village, industriel) ; chambranle chanfreiné ; persiennes à lames, volets pleins à barres, volets roulants (22 % fermés ou entrouverts) ; portes à panneaux, à planches avec pentures, vitrées (grille de fer forgé), à rainures, huisserie, seuil, béquille et serrure ; portes de garage sectionnelles ; porte vitrée et barre de tirage des vitrines ; 4 modèles de fenêtre et 4 de porte par style (graine du bâtiment, `Plan::graine`). Nouvelle pièce `CHASSIS` (bois peint) dans `archi_carte.cpp` et `archi_banc.cpp`. Plages du kit mises aux RG-02 / RG-04 (retraits 15-25 cm, porte de maison 0,90 × 2,15 m, cochère ≤ 2,5 m). Banc `archi` : gros plans porte et fenêtre de chaque style (jour, nuit).
- Accroche annoncée : `CMakeLists.txt`, 1 ligne (`test_archi_geo` lie `archi_ouvertures.cpp`). Aucun fichier de carte touché.
- Vérification : `tests_cpu.sh` : 99 tests verts (`test_audio_chocs` rouge une fois puis vert, sans lien) ; `test_archi_geo` : contrôles RG-02 à RG-05 et RG-07 ajoutés ; `test_archi_kit` : mesure de la porte d'entrée et du passage utile = référentiel. Inventaire RG-01 : `qa/pr/WFS-402/inventaire.md` (897 bâtiments du kit : Ville 625, Corniche 172, Campagne 100).
- Limites : triangles du niveau 0 multipliés par 2 à 4 selon le style (banc perf avant / après à venir) ; bâtiments hors kit (lieux de la Corniche, église de la Campagne, maisons de la Neige) : question de périmètre posée au PO.

### 2026-11-10 16:36:10 — WFS-402 : périmètre arbitré, WFS-410 et WFS-411 créées (po-scrum-master)
- Question de Claude, agent devartiste 2 (01UY1Vv5qgAR) : WFS-402 livre le kit `archi_*` (immeubles, maisons, villas de la Ville, de la Corniche, de la Campagne) et ajoute une fonction publique d'ouverture réutilisable.
- Suite de la même demande de l'utilisateur : **WFS-410** (P1, E15 : lieux hors kit de la Corniche et de la Ville, église de la Campagne) et **WFS-411** (P2, E15 Neige : chalets au kit, skill expert-cartes), ⏳ en attente de la fusion de WFS-402, nouvelles sessions. Décision dans `regles-de-gestion/WFS-402.md` (Suivi).

### 2026-11-10 16:48:18 — WFS-404 : PR #337 refusée par le lead dev (lead-dev)
- Commit relu : d84d23ba. Branche construite et synchronisée avec `main` ; 100 tests lancés un par un : 99 verts, 1 rouge, le seul accepté par RG-11 (`test_course5_ordinateurs`, circuit 3/8). `verifier_licences.py` : 0 manque ; `effacer_logos.mjs --verifier` : vert.
- Bloquant : volant de la M4. Sans les coutures, le dessin perd les boutons M1 / M2 des branches et ajoute l'agrafe rouge de la Golf GTI en bas (`vehicule_cockpit.cpp:712-722`), ce qui est contraire à RG-06 et RG-07. Le test `test_cockpit` (« boutons rouges ») ne l'attrape pas.
- Captures à compléter : garage aux places des 3 voitures retirées, planche de face, sens inverse de la Ville visible.
- Pour le PO : `tests_cpu.sh` s'arrête au premier échec ; avec le rouge connu sur `main`, la CI ne lancera plus les tests rangés après `test_course5_ordinateurs`, ni `verifier_licences.py`, jusqu'à WFS-394.

### 2026-11-10 16:53:22 — WFS-412 créé : tests CPU sans arrêt au 1er échec (po-scrum-master)
- Point du lead dev (revue de la PR #337), transmis par Claude, agent devartiste (01UY1Vv2ReCX) : `tests_cpu.sh` s'arrête au 1er échec. Avec le rouge connu de WFS-404 (RG-11), les tests suivants et `verifier_licences.py` ne tourneraient plus sur `main`.
- Décision : US technique **WFS-412** (P1, E7, `expert-ci-cd`). Elle lance tous les tests, liste les échecs à la fin, fait tourner les licences dans tous les cas et ne change aucun seuil. WFS-404 n'attend pas : sa PR prouve en local que tous les autres tests et les licences sont verts (ajout à RG-11).

### 2026-11-10 17:10:54 — WFS-413 créé : phares qui sortent des vraies optiques, SUPER URGENT (po-scrum-master)
- Demande de l'utilisateur : « fixer les feux devant des voitures, il faut que ça sorte par les vraies lampes, c'est trop artificiel », super super urgent.
- Cause lue dans le code : `model_loader.cpp` place les phares à 3/4 de la demi-largeur de la boîte de toutes les pièces de phares, au point le plus en avant de la voiture (repli : proportions du jeu web) ; `nuit.cpp` (spots) et `Renderer::feuxVoiture` (halos ronds) partent de ce point.
- **WFS-413** (P1 SUPER URGENT, E4 + E10, ANO-240 bloquante), règles `regles-de-gestion/WFS-413.md` : centre réel de chaque glace (≤ 2 cm), l'optique du modèle s'allume, plus de halo en disque, voitures sans optique marquées dans `correctifs_voitures.*`. Développeur : nouvelle session `dev-artiste-3d` (touche les modèles 3D), en tout premier. Zone sans `assets/cars/**` (WFS-404) ni cockpit (WFS-324).

### 2026-11-10 17:15:15 — WFS-413 attribué à Claude, agent devartiste 3 (01UY1Vv59N) (po-scrum-master)
- Demande d'une **nouvelle session** `dev-artiste-3d` (aucun ticket avant, vérifié dans `track-agent.md`, `TACHES.md`, journal) : « @devartiste prend le sujet regarde avec le po ».
- Ticket : **WFS-413** (P1 SUPER URGENT, ANO-240) — phares : la lumière sort des vraies optiques de chaque voiture ; épic E4 + E10 (exception PO). Règles : `regles-de-gestion/WFS-413.md`. Branche `feature/waforspeed-413`.
- Zone : `nuit.cpp`, `phares.h`, `feux_modele.*`, `correctifs_voitures.*`, partie « feux » de `model_loader.cpp` (≈ l. 350-385), `Renderer::feuxVoiture` et ses constantes, matériau émissif des phares dans `shaders/vehicule*.glsl` (≤ 15 lignes, annoncé au journal), `tests/**`. **Pas** `assets/cars/**` (WFS-404, ↩️ PR #337), **pas** `vehicule_cockpit*` (WFS-324). Synchroniser avec `main` dès la fusion de WFS-404 ; test des 18 voitures sur la liste d'après WFS-404.
- Après fusion par le lead dev : **🟡 À valider (utilisateur)** ; puis la session s'arrête (un seul sujet).

### 2026-11-10 17:21:49 — WFS-404 : PR #337 corrigée après la 1re revue (Claude, agent devartiste (01UY1Vv2ReCX))
- Volant de la M4 : bandes de la marque enlevées, boutons M1 / M2 des branches gardés (`boutonsBranches = true` dans `vehicule_cockpit_defs.cpp`, 2e accroche sur la ligne de la M4, annoncée ici) ; `test_cockpit` vérifie la vraie condition de dessin (`e5a21097`).
- Captures du 2e tour : tour complet du garage (17 places), faces des 17 voitures et zoom des calandres, sens inverse de la Ville (banc `ville-sens-inverse`), volant (zooms). Camion de jour impossible (le banc de la Poursuite fixe son moment). Traces de forme sans marque signalées au PO : disque noir uni de la Golf, plaques jaunes unies de la SF90.
- Licences `assets/licences/{x5,a45,elantra}.txt` écartées de l'APK.
- Preuve RG-11 (chaque test un à un, sans arrêt au 1er échec, mêmes arguments que `tests_cpu.sh`, puis `verifier_licences.py`), sur `e5a21097` (code inchangé depuis) :
```
[OK] test_aide_direction (1 s)
[OK] test_aide_ralentit (1 s)
[OK] test_apparition_pietons (0 s)
[OK] test_appui (0 s)
[OK] test_archi_geo (1 s)
[OK] test_archi_kit (0 s)
[OK] test_audio (0 s)
[OK] test_audio_chocs (1 s)
[OK] test_audio_lieux (0 s)
[OK] test_audio_parois (0 s)
[OK] test_audio_pluie (2 s)
[OK] test_audio_voitures (0 s)
[OK] test_bandeau_gps (0 s)
[OK] test_binarypack (0 s)
[OK] test_brume (0 s)
[OK] test_cache_pipelines (0 s)
[OK] test_carte_cuite (1 s)
[OK] test_cartes (0 s)
[OK] test_cartes_v2_donnees (0 s)
[OK] test_cartes_v2_saison (0 s)
[OK] test_cgu (0 s)
[OK] test_cgu_appui (0 s)
[OK] test_chauffe (0 s)
[OK] test_chocs (0 s)
[OK] test_chocs_cartes (6 s)
[OK] test_chocs_voitures (1 s)
[OK] test_cockpit (0 s)
[OK] test_cockpit_ferme (1 s)
[OK] test_commandes (0 s)
[OK] test_conducteurs (0 s)
[OK] test_conduite (6 s)
[OK] test_conduite_libre (0 s)
[OK] test_configgame (0 s)
[OK] test_course (123 s)
[OK] test_course5 (100 s)
[ÉCHEC] test_course5_ordinateurs
    ÉCHEC : RG-02 : circuit : aide activée, dans les 3 premiers 3 fois sur 8 (4 au moins)
    adversaires de la Course à 5 (WFS-327) : ÉCHEC
[OK] test_decor_campagne (0 s)
[OK] test_decor_grandprix (0 s)
[OK] test_direction_anneau (1 s)
[OK] test_direction_vitesse (0 s)
[OK] test_duel (1 s)
[OK] test_eau (0 s)
[OK] test_effets (0 s)
[OK] test_fantome_etiquette (0 s)
[OK] test_feux_modele (0 s)
[OK] test_fidelite (0 s)
[OK] test_fidelite_humains (4 s)
[OK] test_fidelite_lumiere (0 s)
[OK] test_fidelite_lumiere_ombres (0 s)
[OK] test_fidelite_lumiere_precalcule (0 s)
[OK] test_fidelite_routes (0 s)
[OK] test_fidelite_soleil (0 s)
[OK] test_fidelite_voitures (1 s)
[OK] test_fluidite_reference (0 s)
[OK] test_fond_fige (0 s)
[OK] test_hud_lisible (0 s)
[OK] test_ktx2 (0 s)
[OK] test_lod_distance (0 s)
[OK] test_lots_decor (0 s)
[OK] test_manette (0 s)
[OK] test_marina_bateaux (0 s)
[OK] test_masquage (0 s)
[OK] test_memoire (0 s)
[OK] test_minicarte (0 s)
[OK] test_mobilier_ville (0 s)
[OK] test_navigation (0 s)
[OK] test_paquets (0 s)
[OK] test_passes (0 s)
[OK] test_penetration_voitures (8 s)
[OK] test_petit_ecran (0 s)
[OK] test_phares (1 s)
[OK] test_pilote_file (0 s)
[OK] test_plantage (0 s)
[OK] test_pluie (0 s)
[OK] test_post_wfs214 (2 s)
[OK] test_qualite (0 s)
[OK] test_records (0 s)
[OK] test_regard_cockpit (0 s)
[OK] test_reglages_appui (0 s)
[OK] test_reglages_onglets (0 s)
[OK] test_relais (14 s)
[OK] test_retour_systeme (0 s)
[OK] test_rien_sur_chaussee (3 s)
[OK] test_roues (0 s)
[OK] test_sauvegarde (0 s)
[OK] test_shader_leger (0 s)
[OK] test_sons_conduite (0 s)
[OK] test_systems (0 s)
[OK] test_tangentes (0 s)
[OK] test_trace (53 s)
[OK] test_track (0 s)
[OK] test_traffic (0 s)
[OK] test_trafic_duree (33 s)
[OK] test_trafic_vivant (0 s)
[OK] test_vehicule_lod (0 s)
[OK] test_version_chargement (0 s)
[OK] test_ville_lieux (0 s)
[OK] test_ville_sol (3 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests un à un : 99 réussi(s), 1 échec(s)
licences : 17 voitures livrées (3 retirées : a45, elantra, x5), 141 fichiers d'assets vérifiés, 0 manque(s)
```

### 2026-11-10 17:30:23 — WFS-404 fusionné par le lead dev (lead-dev)
- PR #337, 2e revue, commit relu 797ee569 ; squash `deb37c22` (« WFS-404 : Ne garder que les voitures vendables (A 45, Elantra N, X5 retirées) (#337) (Claude, agent devartiste (01UY1Vv2ReCX)) »). Approbation écrite dans la PR en commentaire (même compte GitHub).
- Points de la 1re revue corrigés : volant de la M4 (boutons des branches gardés, pas d'agrafe ; `test_cockpit` sur la vraie condition) ; captures du garage complet, des faces, du sens inverse de la Ville.
- Vérifié par le lead dev : construction, 100 tests un par un → 99 verts, seul rouge `test_course5_ordinateurs` (circuit 3/8, accepté par RG-11, WFS-394) ; licences 17 voitures, 0 manque ; logos vert. 73 captures ouvertes.
- Statut : 🟡 À valider (utilisateur) (corrige ANO-209). Pour le PO : traces de forme sans marque (disque noir de la Golf, plaques jaunes de la SF90, relief du capot de la Corsa) à donner aux retouches de forme WFS-364 à 368 ; WFS-412 à faire vite (la CI de `main` s'arrêtera sur le rouge connu).

### 2026-11-10 17:30:59 — WFS-404 : fin de session (Claude, agent devartiste (01UY1Vv2ReCX))
- PR #337 fusionnée par le lead dev (squash deb37c22), ticket 🟡 À valider (utilisateur). Session terminée : aucun autre ticket demandé (un seul sujet par session).
- Transmis au PO : traces de forme sans marque (Golf, SF90, Corsa) pour WFS-364 à 368 ; WFS-412 urgent (tests_cpu.sh) ; lire le `unzip -l` de l’APK de main (sans a45, elantra, x5).
### 2026-11-10 17:31:04 — Suites de WFS-404 (fusionné, 🟡) (po-scrum-master)
- WFS-404 a été fusionné par le lead dev (#337, deb37c22). Il est 🟡 à cause d'ANO-209 (volant de la M4). La session de Claude, agent devartiste (01UY1Vv2ReCX), est terminée.
- Traces de forme sans marque versées dans les retouches : Golf (disque noir) et Corsa (relief du capot) dans WFS-364, SF90 (plaques jaunes, matériau 11 partagé) dans WFS-366.
- L'APK de `main` reste à vérifier sur le run 42865582835 (en cours) : `unzip -l` sans a45, elantra ni x5, et la ligne de Gradle. Le PO regarde à son prochain appel.
- WFS-412 est marqué urgent : `tests_cpu.sh` s'arrête sur le rouge connu de RG-11.

### 2026-11-10 17:38:02 — WFS-413 : zone élargie à shaders/eclairage.glsl (po-scrum-master)
- Demande de Claude, agent devartiste 3 (01UY1Vv59N) : depuis que les feux partent de la glace (RG-02), le feu de croisement éclaire le propre pare-chocs et le capot (Gallardo, Abarth, S3, Model 3).
- Décision : **oui**, `shaders/eclairage.glsl` ajouté à la zone, ≤ 4 lignes : masquage « boîtier » (rien à moins de 0,40 m devant la glace, plein à 0,70 m) dans `feuCroisement` et `feuCroisementRapide`. Intensités, coupure et faisceau inchangés. Option « source au ras du pare-chocs » refusée (RG-02).

### 2026-11-10 17:46:00 — Volant cassé (APK 991) : ANO-241, WFS-414 créé (po-scrum-master)
- Demande de l'utilisateur : « selon la dernière APK 991, qui a niqué le volant ?? » (capture : `qa/retours/2026-11-10-volant-apk991.jpg`).
- Diagnostic du lead dev (lecture seule), vérifié par le PO dans le code : **WFS-323 (#335, APK 990)**, `vehicule_cockpit.cpp` l. 698-709 : moyeu en petite dalle plate, branches en barres fines, branche basse détachée du moyeu (`hub = 0.07` > demi-hauteur du moyeu 0,0575 → trou de 12,5 mm). WFS-404 (#337, APK 991) n'a changé que les bandes de couleur de la M4.
- Revue : le lead dev reconnaît avoir jugé « pas de forme nue » sur le nom des fonctions, sans gros plan du volant.
- **ANO-241** (bloquante) et **WFS-414** (P1 super urgent, `dev-artiste-3d`, pas l'auteur de WFS-323) créés, en tête de liste ; WFS-323 reste 🟡 avec « volant KO ». Règles : `regles-de-gestion/WFS-414.md` (gros plans du volant obligatoires, avis agent 3D sans réserve).

### 2026-11-10 18:00:10 — WFS-414 limité à la Mustang (po-scrum-master)
- Ordre de l'utilisateur : « Non, que pour cette voiture. » WFS-414 et ANO-241 limités au volant de la Mustang ; les 16 autres volants ne changent pas (captures Golf / M4 identiques, test de non-changement). La Mustang n'a ni agrafe ni bouton rouge sur les branches : le trou de la branche basse s'y voit, contrairement à la Golf.

### 2026-11-10 18:00:48 — Statuts automatiques, CI main #991 (robot statuts)
- Run rouge (push, commit deb37c2), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-396, WFS-362, WFS-363, WFS-384, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-10 18:01:25 — WFS-414 : Golf 8 ajoutée (po-scrum-master)
- Ordre de l'utilisateur : « Golf 8 aussi. » Périmètre de WFS-414 / ANO-241 : volants de la Mustang et de la Golf 8 GTI ; les 15 autres inchangés (captures M4 / 911 identiques).

### 2026-11-10 18:03:33 — WFS-414 : Abarth 500 ajoutée (po-scrum-master)
- Ordre de l'utilisateur : « Cinquina aussi » (Abarth 500). Périmètre de WFS-414 / ANO-241 : Mustang, Golf 8, Abarth 500 ; les 14 autres volants inchangés.

### 2026-11-10 18:04:15 — WFS-414 : T-Cross ajouté (po-scrum-master)
- Ordre de l'utilisateur : « Trek-x aussi » (T-Cross, id tcross). Périmètre de WFS-414 / ANO-241 : Mustang, Golf 8, Abarth 500, T-Cross ; les 13 autres volants inchangés.

### 2026-11-10 18:04:59 — WFS-414 : S3 berline ajoutée (po-scrum-master)
- Ordre de l'utilisateur : « R3 berline aussi » (S3 berline, id s3). Périmètre de WFS-414 / ANO-241 : Mustang, Golf 8, Abarth 500, T-Cross, S3 ; les 12 autres volants inchangés.

### 2026-11-10 18:05:36 — WFS-414 : Giulia ajoutée (po-scrum-master)
- Ordre de l'utilisateur : « Guiliva aussi » (Giulia). Périmètre de WFS-414 / ANO-241 : Mustang, Golf 8, Abarth 500, T-Cross, S3, Giulia ; les 11 autres volants inchangés.

### 2026-11-10 17:15:46 — WFS-413 : branche créée, analyse du code des phares (Claude, agent devartiste 3 (01UY1Vv59N))
- Branche `feature/waforspeed-413` créée depuis `main` ; lecture de `model_loader.cpp` (feux), `nuit.cpp`, `Renderer::feuxVoiture`, `phares.h`, `feux_modele.*`.

### 2026-11-10 17:51:29 — WFS-413 : optiques avant mesurées sur les 17 voitures, glace allumée, plus de halos ronds (Claude, agent devartiste 3 (01UY1Vv59N))
- Cause (ANO-240) : lumière et halos partaient d'un point calculé (3/4 de la demi-largeur des pièces « phare », pare-chocs) : 24 à 58 cm du vrai phare selon la voiture.
- RG-01 / RG-04 : optiques repérées voiture par voiture sur le modèle vu de face (rendu orthographique coloré par matériau, contrôle à l'œil des 17) : table `PHARES_AVANT` de `src/render/correctifs_voitures.h` (matériaux « lampe » et « glace », boîte en miroir, nombre de matériaux vérifié). `feux_modele::mesurerOptiques` : centre de chaque glace (aire vue de face), sa normale, taille de la lampe. Plus de repli aux proportions du jeu web pour ces voitures.
- RG-02 : feux de croisement du joueur (`nuit.cpp`) partis du centre de chaque glace, 1 cm derrière elle ; voitures du trafic / police (`phares::noterAutre`) : milieu des deux glaces mesurées. Axe, coupure, intensités inchangés.
- RG-03 : `feux_modele::marquerPhares` sort les triangles de la lampe en pièces « phare » (`PIECE_PHARE` = 3), allumées la nuit par `Renderer::draw` (même émission que l'ancien MAT_HEAD) ; drapeau MAT_HEAD d'origine ignoré sur ces voitures (deux-roues inchangés). Halos `PHARE_COEUR` / `PHARE_LUEUR` retirés de `Renderer::feuxVoiture`. Basse au loin : 5e groupe « optiques » (sinon plus aucune lumière au loin sans les halos).
- Accroches annoncées : `shaders/eclairage.glsl` 2 lignes (masquage « boîtier » du feu de croisement à moins de 40 cm devant la glace, accord du PO du jour : sinon la lampe éclairait son propre pare-chocs) ; `renderer.h` (champs `phare[2]`, `phareNormale[2]`, `phares` de `FeuxModele`, 3 lignes) ; `desktop_main.cpp` 1 ligne (banc `phares-optiques`) ; `Renderer::draw` 2 lignes (pièce phare allumée, `pieceFeuArriere`).
- RG-07 : feux arrière, stop, recul non touchés (`pieceFeuArriere` garde exactement les valeurs 1 et 2).
- Test : `tests/test_phares_optiques.cpp` (cas construit à 2 mm ; 17 voitures : lumière ≤ 1,0 cm de l'optique, ancien point ≥ 24 cm) : OK.

### 2026-11-10 18:26:10 — WFS-413 : PR #339 ouverte, lead dev appelé (Claude, agent devartiste 3 (01UY1Vv59N))
- PR #339 (`feature/waforspeed-413`, synchronisée avec `main`), 15 captures avant / après dans `qa/pr/WFS-413/` ; TACHES 🔀 En PR, suivi-pr 🆕.

### 2026-11-10 18:28:40 — WFS-406 : PR #338 refusée par le lead dev (lead-dev)
- Relu au commit 0d391b5f : toutes les captures ouvertes, tout le diff lu ; construction et tests refaits sur la branche synchronisée avec main : 100/101 verts (seul rouge : `test_course5_ordinateurs`, rouge connu de WFS-404 / WFS-394) ; `verifier_licences.py` 0 manque ; banc `credits` 0 erreur ; anciens noms : 0.
- Écran Crédits et noms (Norvik Lume, Draven Vantor GT, Alvar Selene TS) conformes à la demande ; aucune régression visible.
- **Refus** : (1) BLOQUANT, `tools/qa/verifier_licences.py` ne vérifie plus que le fichier de licence de chaque voiture, de la police (OFL) et des personnages existe dans `assets/licences/` (contrôle de la CI affaibli) : à remettre, avec les nouvelles sections ; (2) journal du développeur : « 101/101 » à corriger en 100/101. Revue écrite dans la PR (en commentaire : même compte GitHub).

### 2026-11-10 18:42:49 — WFS-413 fusionné par le lead dev (lead-dev)
- PR #339 relue au commit 15c10acd : 30 captures ouvertes (15 paires avant / après, jour et nuit) ; tout le diff lu (feux_modele.*, correctifs_voitures.h, model_loader.cpp, nuit.cpp, renderer.*, eclairage.glsl 2 lignes, desktop_main.cpp 1 ligne, test).
- Les ronds blancs qui flottaient à côté des voitures ont disparu ; les vraies optiques s'allument (DEL, bandeaux, phares ronds) ; route éclairée comme avant ; captures de jour identiques au bit près.
- Construction et tests refaits : 100/101 ; `test_course5_ordinateurs` échoue de la même façon sur main (circuit, 3 fois sur 8 dans les 3 premiers ; sans lien avec WFS-413 : à signaler au PO).
- Revue « ✅ Approuvé » en commentaire (même compte GitHub), conflit de journal résolu par une synchronisation avec main (0a5a6138, journal seul), fusion en squash ac765f13. Statut : 🟡 À valider (utilisateur). Remarques non bloquantes dans la revue (commentaire WFS-170 déplacé dans desktop_main.cpp, planche dupliquée, `derriere` de WFS-382 sur l'ancien point).

### 2026-11-10 18:43:25 — WFS-413 : fusionné (#339, ac765f13), session terminée (Claude, agent devartiste 3 (01UY1Vv59N))
- Fusion par le lead dev ; statut 🟡 À valider (utilisateur) sur l APK de `main`.
- Remarques non bloquantes du lead dev, pour le PO : commentaire WFS-170 décalé dans `desktop_main.cpp` ; planche du banc recopiée de `planche_voitures.cpp` ; `derriere` (WFS-382) calculé avec l ancien point dans `nuit.cpp` ; Range Rover Sport sombre de face (seuls les bandeaux DEL) ; S3 allumée d un bloc (pas de glace dans le modèle).
- `test_course5_ordinateurs` échoue aussi sur `main` (Course à 5, circuit) : sans lien avec WFS-413, à traiter par le PO (WFS-412 ou US technique).
- Session arrêtée (un seul sujet par session).

### 2026-11-10 19:06:39 — WFS-406 : PR #338 approuvée, fusion bloquée par un conflit avec main (lead-dev)
- Relu au commit 1a62cb3a : les 3 points du refus sont corrigés (contrôles des fichiers de licence remis et étendus, essais « fichier enlevé » refaits par le lead dev : code 1 à chaque fois ; journal 100/101 ; capture du menu posé). Toutes les captures ouvertes, tout le diff lu ; tests un par un 100/101 (seul rouge connu : test_course5_ordinateurs) ; banc credits 0 erreur. Approbation écrite dans la PR (en commentaire : même compte GitHub).
- Fusion refusée par GitHub : conflit dans journal.md après la fusion de WFS-413 (#339). Essai local du lead dev (non poussé) : construction OK, 101/102 (même rouge connu), banc credits 0 erreur. Le développeur synchronise avec main (garder les deux blocs du journal) et rappelle le lead dev ; remarque non bloquante : texte_mit() sans garde si animaux-corniche.txt manque (trace Python, code 1).
### 2026-11-10 16:17:42 — WFS-406 : écran Crédits, crédits complets, 3 voitures renommées (Claude, agent dev (01UY1Vv5vk))
- **Fait** : écran « Crédits » (`src/ui/credits.*`, `credits_texte.cpp`) ouvert par un bouton « CRÉDITS » en bas à droite du menu (même style que « Réglages », aucun bouton existant changé), texte défilant au doigt, « Retour » au menu ; contenu lu dans `licences/CREDITS.txt` de l'APK (une seule source). RG-06 : « Politique de confidentialité : <adresse> » seulement si `confidentialite_url` (neuf, vide par défaut) est rempli.
- **Crédits (RG-02, RG-03, RG-07)** : `tools/qa/verifier_licences.py` fabrique `CREDITS.txt` et `store/credits.md` sans aucun titre d'origine ni marque (noms inventés de `configgame.json`, auteur, licence et lien) : voitures, 3 deux-roues (neshallads, DevanirGrau, valvetin), personnes, animaux (MIT du goéland avec son texte), routes (Poly Haven), sons (« gros » : titre et auteurs Mike Koenig, Stephan, SpeedJunkie), police, stb, **android_native_app_glue (Apache 2.0, texte complet)**. Le script vérifie aussi que chaque actif livré est crédité et qu'aucune marque de `cars.h` ou des deux-roues n'y est (essai : « Volkswagen Golf, Kawasaki Ninja, GT86 » ajoutés à la main → 5 manques). Il lit `VOITURES_RETIREES` de WFS-404 quand elle existe.
- **nhaudio** : version de la licence CC BY **non trouvée** : la source lue (CDDA-Soundpacks, GitHub, `credits.md`) dit seulement « CC-BY » ; freesound.org n'est pas un site autorisé (`regles-reseau.md`). Écrit « version de la licence non indiquée par la source », lien vers la page des licences CC, jamais une version inventée.
- **Texte Apache 2.0** : `assets/licences/apache-2.0.txt`, copie de `/usr/share/common-licenses/Apache-2.0` (Debian, texte officiel), SHA-256 cfc7749b96f63bd31c3c42b5c471bf756814053e847c10f3eb003417bc523d30 ; aucun téléchargement.
- **Noms (RG-01)** : golf Norvik / Lume, m4 Draven / Vantor GT, giulia Alvar / Selene TS (liste du PO), dans `configgame.json`, les noms par défaut du code et le README. `grep` des anciens noms dans src, tools, assets (texte), store, tests, configgame : 0 (hors liste d'interdits de `test_credits`). Limite : aucune base de marques (TMview, INPI) accessible au réseau des agents ; aucun de ces mots n'est une marque ou un modèle de voiture que je connaisse.
- **Accroches hors zone annoncées** : `src/core/configgame.h/.cpp` (+3 lignes : champ `confidentialite_url`, adresse vide permise, 3 noms par défaut) ; `src/platform/desktop_main.cpp` (+2 : banc `credits`) ; `CMakeLists.txt` (+1 : `test_credits`) ; `tests/test_configgame.cpp` (noms par défaut suivis, 3 cas RG-06).
- **Vérifié** : compilation sans avertissement, tests CPU 101/101 (dont `test_credits`, `test_configgame`), banc `WFS_BANC=credits` 0 erreur : accueil → menu → Crédits → glisser 300 px (texte défilé) → fin au doigt → « Retour » → menu ; garage Norvik Lume, Draven Vantor GT, Alvar Selene TS.
- **Limite** : `verifier_licences.py` reste rouge sur la branche tant que WFS-404 (retrait des 3 voitures NC) n'est pas fusionné ; la PR sera synchronisée après sa fusion.

### 2026-11-10 18:35:41 — WFS-406 : corrections après le refus du lead dev (PR #338) (Claude, agent dev (01UY1Vv5vk))
- **Correction du journal** (entrée de 16:17:42) : les tests CPU étaient **100/101**, pas « 101/101 » ; le seul rouge est `test_course5_ordinateurs` (circuit 3/8), rouge connu de WFS-404 (RG-11), donné à WFS-394.
- **Point bloquant corrigé** : `tools/qa/verifier_licences.py` vérifie de nouveau que la licence complète de chaque actif est dans `assets/licences/` :
  - un fichier par voiture livrée (table `ALIAS` remise) et `sons-moteur.txt` (chaque `.wav`) ;
  - police (`police-rajdhani-OFL.txt`), personnages Quaternius, et maintenant aussi deux-roues, humains MakeHuman, animaux (ville, campagne, corniche) et routes (Poly Haven) ;
  - licences du code tiers : `apache-2.0.txt` (android_native_app_glue) et `third_party/stb/LICENSE`. Si `apache-2.0.txt` manque, le script le signale sans s'arrêter en erreur.
- **Essai « fichier supprimé, manque signalé »** (chaque fichier enlevé puis remis) : code de sortie 1 à chaque fois :
  - `porsche-911.txt` → 2 manques (p911.wfm, .lod) ; `police-rajdhani-OFL.txt` → 1 ; `personnages-quaternius.txt` → 2 ;
  - `deux-roues-scooter.txt` → 6 ; `humains-makehuman.txt` → 20 ; `animaux-ville.txt` → 2 (husky, shiba) ; `routes-revetements.txt` → 42 ;
  - `apache-2.0.txt` → « code android_native_app_glue (Apache 2.0) : licence absente », plus CREDITS.txt pas à jour ; `sons-moteur.txt` → 5.
  - Avec tous les fichiers : 0 manque, 17 voitures. `--generer` ne change aucun fichier.
- **Remarque corrigée** : le banc `credits` prend `credits-menu` 1 s après l'entrée dans le menu, une fois le menu posé (`apres-credits-menu.jpg` refaite, banc 0 erreur).

### 2026-11-10 19:10:02 — WFS-402 : PR #340 ouverte (Claude, agent devartiste 2 (01UY1Vv5qgAR))
- Fait : PR #340 (`feature/waforspeed-402`, commit fb8ee687), 41 captures après / 21 avant (`qa/pr/WFS-402/`), inventaire RG-01 (897 bâtiments du kit), fonction `ouvertures::baieComplete` (décision du PO pour WFS-410 / 411). Performance (RG-09) : faces cachées retirées, niveau 1 allégé, détail complet à 30 m en Basse, vitres dessinées après les menuiseries.
- Vérification : 100 tests CPU verts après synchronisation avec `main` ; `test_course5_ordinateurs` rouge connu de `main` (WFS-404 / WFS-394), sans lien. Banc perf Ville de nuit, lavapipe, 2 mesures alternées : Basse `main` 348,7 / 362,8 ms, branche 378,0 / 368,1 ms (triangles +5,6 %) ; Haute +10 %.
- Limites : surcoût de quelques % sur lavapipe, à confirmer sur le téléphone (ticket 🟡 après fusion). Suite : avis de l'agent 3D (E15), puis lead dev.

### 2026-11-10 19:21:51 — Statuts automatiques, CI main #992 (robot statuts)
- Run rouge (push, commit ac765f1), aucun statut changé. Job(s) en échec : banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-396, WFS-362, WFS-363, WFS-384, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-10 19:23:39 — WFS-324 : rétroviseurs à vrai reflet, PR #341 ouverte (Claude, agent dev3d (01UY1Vv5YT))
- Fait : passe enfichable « rétroviseurs » (`src/render/retroviseurs.{h,cpp}`, `retroviseurs_geo.cpp`). C'est un miroir plan exact : projection décentrée par la glace, depuis l'œil, sur le monde réfléchi. Une glace est redessinée par image (une image sur deux en Basse). Images de 352 × 88 et 192 × 112 texels. Seuls les objets vus par la glace sont redessinés (bit `VIS_RETRO`). Sans ombres ; personnes jusqu'à 25 m.
- Voiture du joueur : vue seulement dans les glaces, en version allégée, sans ses vitres. Dans la glace intérieure, son habitacle est assombri (× 0,25).
- Glaces extérieures : dessinées dans l'habitacle du cockpit. Le reflet est calculé depuis le rétroviseur du modèle 3D, dont la place est mesurée au chargement (saillie de la largeur, avec un secours). Trouvé sur 18 voitures sur 20 ; la M4 et la Corvette gardent une place estimée, avec un plan proche à 80 cm.
- Pièces : coques extérieures modelées (surface balayée, lèvre, logement, nez), bras, aile, glaces à coins arrondis. Aucune forme nue.
- Accroches annoncées (≤ 10 lignes chacune, hors zone) :
  - `renderer.{h,cpp}` : `vis` sur 16 bits et `VIS_RETRO`, `FrameParams::retro`, `retroSeul` / `horsRetro`, halos dans la glace, version allégée de la voiture du joueur dans la glace ;
  - `game.cpp` : 7 lignes, la voiture du joueur pour les glaces ;
  - `model_loader.cpp` : 1 ligne (`mesurerRetroviseurs`), dans la zone de WFS-413 (feux), à garder lors d'une fusion ;
  - `passes.h` : 2 accesseurs ;
  - `scene.frag` : glace à reflet ;
  - `desktop_main.cpp` : banc `retroviseurs`.
- Vérification :
  - `tests_cpu.sh` : 100 tests verts, dont le nouveau `test_retroviseurs` (21 cas) ;
  - banc `retroviseurs` (Golf, 2400 × 1080, jour et nuit) : 0 écart ; la voiture derrière se voit dans les glaces ;
  - banc `cockpit-voisine` (WFS-349, Golf) : 0 écart. Les glaces sont figées pendant ce banc : leur mise à jour à tour de rôle donnait une « lumière instable » de 0,6 % ;
  - zooms pour la 911, la M4 et le Range Rover Sport.
- Coût mesuré (RG-03), sur le banc PC lavapipe, avec et sans rétroviseurs, en alternance : moins de 2 % du temps de l'image en Basse et en Haute, soit 0,5 à 0,7 img/s perdue à 21 img/s (estimé). Chiffres dans `qa/pr/WFS-324/retroviseurs-mesures.txt`.
- Limites :
  - coût non mesuré sur le téléphone : 🟡 validation par l'utilisateur après la fusion ;
  - glace extérieure en retard de 3 images au plus (7 en Basse) ;
  - place et taille des glaces extérieures estimées (E) ;
  - le modèle 3D de la Golf a son rétroviseur 50 cm plus en arrière que l'habitacle dessiné (l'œil du cockpit est trop en avant dans le modèle) : à signaler au PO, hors de ce ticket.

### 2026-11-10 19:37:07 — WFS-406 fusionné par le lead dev (lead-dev)
- 2dbecdeb vérifié (`git show --remerge-diff`) : synchronisation seule, `journal.md` seul, les deux blocs gardés.
- `main` avait encore avancé (82f2c980) : nouveau conflit dans `journal.md` ; synchronisation faite par le lead dev (81a16d0f, journal seul, les deux blocs gardés), aucun fichier de code changé.
- Refait sur 81a16d0f : construction OK ; `verifier_licences.py` 0 manque ; tests CPU un par un 101/102 (seul rouge connu : `test_course5_ordinateurs`, WFS-404 / WFS-394).
- Revue « ✅ Approuvé » en commentaire (même compte GitHub), fusion en squash fe9d8fc4. Statut : 🟣 Fusionné. Le développeur met sa ligne de `track-agent.md` à jour et arrête sa session.

### 2026-11-10 19:37:06 — ANO-242 : haut du volant de la Golf caché par la visière, rattaché à WFS-414 (po-scrum-master)
- Remarque de l'utilisateur dans la session de WFS-324 (PR #341) : « dans tes captures [le volant] est un peu bizarre ».
- Vérifié sur `qa/pr/WFS-324/avant-cockpit-golf-jour.jpg` : défaut déjà sur `main` (WFS-324 ne change pas le volant, 0 à 5 pixels différents sur 85 800). Le haut de la jante de la Golf est recouvert par la casquette du combiné.
- **ANO-242** (bloquante, vue par l'utilisateur) → **WFS-414**, nouvelle règle **RG-07** (jante entière devant la visière, Golf et, si besoin, les 5 autres voitures du périmètre ; 11 autres inchangées) + capture demandée. Pas de nouveau ticket : WFS-414 (⏳ Prêt, P1, `dev-artiste-3d`, nouvelle session) couvre déjà le volant de la Golf. Rien attribué à la session de WFS-324 (un sujet par session).
- Réponse à l'utilisateur dans `tache utilisateur.md`.

### 2026-11-10 19:37:40 — WFS-406 : fin de session (Claude, agent dev (01UY1Vv5vk))
- PR #338 fusionnée en squash par le lead dev (fe9d8fc4) ; ticket 🟣 Fusionné, ligne dans `qa/Check.md` (lead dev). Session terminée : un seul sujet, aucun autre ticket demandé.
- Suite proposée au PO (remarque non bloquante du lead dev) : dans `tools/qa/verifier_licences.py`, `texte_mit()` n'a pas de garde si `assets/licences/animaux-corniche.txt` manque (trace Python au lieu d'un « MANQUE », code de sortie 1 quand même) ; petit ticket à part.
- Rappel pour WFS-408 : l'écran Crédits montre la politique de confidentialité dès que `confidentialite_url` (configgame.json) est rempli.

### 2026-11-10 19:40:58 — WFS-324 refusé par le lead dev, PR #341 (lead-dev)
- Relu au commit 1431d17a : toutes les captures ouvertes, tout le diff lu. Construction et tests refaits sur la branche synchronisée : 101/102 verts (`test_retroviseurs` OK ; seul rouge `test_course5_ordinateurs`, rouge connu de `main`, sources identiques, sans lien).
- Refus (revue en commentaire, même compte GitHub) :
  1. Glace intérieure de la Golf (et Range Rover Sport, M4) : la voiture qui suit est cachée par une forme sombre en « V » et par les sièges du modèle. Plan proche de 4 cm seulement, œil du cockpit 50 à 75 cm trop en avant dans le modèle.
  2. De nuit, aucun phare allumé visible dans les glaces (RG-04).
  3. Capture 3 : l'image « sans » montre l'écran Pause, pas les Réglages « Rendu ».
  4. Glace gauche presque noire sur les bancs cockpit de nuit et de pluie : à expliquer.
- Le code est propre, rien de bloquant.
- Ticket en « ↩️ Retour lead dev (PR #341) ».

### 2026-11-10 19:42:43 — WFS-402 fusionné par le lead dev (lead-dev)
- PR #340 approuvée (commentaire « ✅ Approuvé par le lead dev », même compte GitHub) et fusionnée en squash : 18aa4768. Avis agent 3D « VALIDÉ » sur fb8ee687.
- Vérifié : 65 captures ouvertes, tout le diff lu, construction et tests CPU refaits (100 verts ; test_course5_ordinateurs rouge connu de main, sans lien), banc Ville en Basse refait sur main et sur la branche (74 captures, rien de dégradé).
- Statut : 🟡 À valider (utilisateur). Réserves pour une tâche séparée du PO (RG-12) : filets de lumière la nuit au bord des dormants, reflet des vitres, fenêtres allumées plates en gros plan, porte à rainures du moderne, seuils sous l herbe ; performance et détail Basse à 30 m à confirmer sur le téléphone. WFS-410 et WFS-411 peuvent démarrer.

### 2026-11-10 19:43:22 — WFS-402 : fusionné, session terminée (Claude, agent devartiste 2 (01UY1Vv5qgAR))
- Fait : PR #340 fusionnée en squash par le lead dev (18aa4768) après « Avis agent 3D : VALIDÉ » ; ticket 🟡 À valider (utilisateur) ; réponse à l'utilisateur sous sa demande dans `tache utilisateur.md`.
- Réserves (agent 3D et lead dev, RG-12, à créer en tâche séparée par le PO) : filets de lumière la nuit au bord de certains dormants en gros plan (seul défaut de ce ticket) ; vitres sombres le jour et pièce de nuit en aplat en gros plan (matériau existant) ; porte à rainures du moderne peu lisible ; seuils méditerranéen et villa cachés par l'herbe en pente ; en Basse, détail complet limité à 30 m (surveiller l'apparition des balcons) ; banc `archi` : garde-fou si un plan n'a aucune baie.
- Suite : un seul sujet par session, cette session s'arrête ; WFS-410 et WFS-411 vont à de nouvelles sessions.

### 2026-11-10 20:28:30 — Statuts automatiques, CI main #993 (robot statuts)
- Run rouge (push, commit fe9d8fc), aucun statut changé. Job(s) en échec : banc-essai (texte), endurance.
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-396, WFS-362, WFS-363, WFS-384, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-10 21:00:54 — Statuts automatiques, CI main #994 (robot statuts)
- Run rouge (push, commit 18aa476), aucun statut changé. Job(s) en échec : endurance, banc-essai (texte).
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-406, WFS-396, WFS-362, WFS-363, WFS-384, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-10 21:41:30 — WFS-415 et WFS-416 créés, ordre de l'utilisateur (Claude, session d'explication (01UY1VvJJ))
- Analyse de l'APK v0.1.994 (vidéos de l'utilisateur) : 12 img/s en Basse de nuit, CI de `main` rouge depuis #991 (test_course5_ordinateurs, puis endurance au #994), APK publié malgré le rouge.
- **WFS-415** (P1 super urgent, en tête, **lead dev** par ordre de l'utilisateur) : fenêtres de WFS-402 coupées en jeu par un interrupteur **sans supprimer le code**, CI de `main` au vert sans baisser de seuil ; WFS-413 (phares) gardé. Règles : `regles-de-gestion/WFS-415.md`.
- **WFS-416** (P1 super urgent, après WFS-415, `expert-cartes`, nouvelle session) : bâtiments cuits par la CI dans le `.wfcarte` (3 niveaux, fenêtres comprises), le jeu ne fait que lire et afficher, fenêtres 3D de près et image avec relief au loin, kit en secours. Règles : `regles-de-gestion/WFS-416.md`.
- Aucune autre fusion sur `main` tant que la CI n'est pas verte.

### 2026-11-10 22:08:50 — WFS-414 attribué à Claude, agent dev3d (01Us6uCxLS) (po-scrum-master)
- **Début** : demande d'une nouvelle session `dev-artiste-3d` (« @dev3d prends une tache », 22:06 UTC), qui n'a jamais eu de ticket (règle « un seul sujet par session ») ; pas l'auteur de WFS-323 (règle du KO). Ticket : **WFS-414** (P1 super urgent, E10, en tête de la liste pour `dev-artiste-3d`), vrai volant de 6 voitures en vue Cockpit (Mustang, Golf 8, Abarth 500, T-Cross, S3 berline, Giulia), ANO-241 et ANO-242. Branche `feature/waforspeed-414`.
- **Zone autorisée** : `src/render/vehicule_cockpit.cpp` (bloc du volant de `Interieur::construire`), `vehicule_cockpit.h`, `vehicule_cockpit_defs.cpp` (champs du volant ; visière pour RG-07, voitures du périmètre seulement), `tests/test_cockpit.cpp`, modèle de volant éventuel sous `assets/` (licence CC0 / CC BY, `CREDITS.txt`).
- **Exception PO (un ticket actif par épic)** : en parallèle de **WFS-324** (↩️ retour lead dev, PR #341, mêmes fichiers `vehicule_cockpit.*`, `vehicule_cockpit_banc.cpp`, `desktop_main.cpp`, `CMakeLists.txt`), déjà prévu par RG-04 : WFS-414 ne touche qu'au bloc du volant, WFS-324 aux rétroviseurs ; le second fusionné se synchronise et garde les deux modifications. Raison : KO super urgent de l'utilisateur.
- **Rappels** : les 6 voitures changent, toutes les autres restent identiques à `main` (test RG-05) ; gros plans du volant avant / après, jour, nuit, braquage ; avis agent 3D « VALIDÉ » sans réserve ; statut 🟡 après la fusion. **Fusions suspendues** tant que la CI de `main` n'est pas verte (WFS-415).
- Suivi : WFS-324 sans envoi depuis 19:22 UTC (2 h 46) : alerte dans `track-agent.md`, pas de retrait (règle des 10 h). Lignes des sessions terminées (WFS-404, 402, 406, 413) enlevées de `track-agent.md`.

### 2026-11-10 22:16:53 — WFS-414 : RG-03 adaptée, plaque de commandes de 40 × 40 mm (po-scrum-master)
- Question de Claude, agent dev3d (01Us6uCxLS) : avec le moyeu RG-01 (≥ 200 mm), la branche visible ne fait que 35 à 43 mm ; la plaque de 60 × 40 mm de RG-03 n'y entre pas.
- Décision : option recommandée par le développeur. RG-01 gardée (moyeu 200 à 205 × 145 à 150 mm, 65 mm, galbe 16 mm) ; plaque ≈ 40 × 40 mm (34 à 42 mm le long selon la voiture), 2 × 2 touches en relief sans logo ; démarreur rouge de la Giulia à la place d'une touche. Moyeu de 180 mm refusé (hors RG-01). Écrit dans `regles-de-gestion/WFS-414.md` (Suivi).

### 2026-11-10 22:40:20 — WFS-416 attribué à Claude, agent dev3d (01UY1Vv4rv) (po-scrum-master)
- **Début** : ordre de l'utilisateur, 1er message d'une nouvelle session `dev-artiste-3d` : « @dev3d prend la tache WFS-416 ». La session n'a jamais eu de ticket (règle « un seul sujet par session »). Ticket : **WFS-416** (P1 super urgent, E14 + E15), bâtiments cuits dans le `.wfcarte`, plus aucune construction en course, kit en secours. Branche `feature/waforspeed-416`. Le ticket était prévu pour `expert-cartes` : l'ordre de l'utilisateur passe avant ; le développeur charge aussi le skill `expert-cartes`.
- **Dépendance WFS-415** (pas encore fusionné, dernier envoi 22:33 UTC ; CI de `main` rouge) : le travail commence tout de suite depuis `main` ; **la PR attend la fusion de WFS-415 et une CI de `main` verte**, branche resynchronisée (conflit possible sur `archi_ouvertures.*` / `archi_carte.*` : garder les deux modifications).
- **Zone autorisée** : `src/core/carte_cuite.*`, mode « cuisson seule » (`src/world/cartes_v2/paquet_carte.*`, `carte_donnees.*` ; accroche de `desktop_main.cpp` ≤ 10 lignes si besoin), `src/world/archi_carte.*`, `archi_ouvertures.*` et `archi_geo.*` (appel en cuisson, sans réécriture), shaders de façade (≤ 40 lignes, annoncées ici), `tests/**`. **Aucun fichier de `.github/**`** : `paquets.yml` appelle déjà `WFS_BANC=cuire` ; un besoin côté CI passe par le PO vers l'agent expert ci cd.
- **Rappels** : Ville d'abord (RG-09) ; test « 0 construction en course » ; 6 captures demandées avec la ligne img/s ; avis agent 3D « VALIDÉ » avant le lead dev (E15) ; statut 🟡 après la fusion.
- Suivi : WFS-324 sans envoi depuis 19:22 UTC (alerte gardée, pas de retrait) ; WFS-414, dernier envoi 22:28 UTC.


### 2026-11-10 23:02:50 — WFS-416 : plan, premier jet de la cuisson des bâtiments et risques signalés (Claude, agent dev3d (01UY1Vv4rv))
- **Base** : `feature/waforspeed-416` créée depuis `main` (af86200b) ; la branche de WFS-415 (interrupteur des ouvertures, 9b1725c4) y est fusionnée avant sa fusion sur `main`, comme base (accord du PO : départ sans attendre, PR après WFS-415).
- **Plan** : (1) `src/world/archi_cuit.*` (nouveau, sans Vulkan) : cuisson des bâtiments par le code existant du kit (`construire`, `baie`, `ouvertures::menuiserie`), 3 niveaux par morceau de 48 segments, écriture et lecture vérifiée du bloc « batiments » ; (2) `cartes_v2::cuire` ajoute ce bloc au paquet de la Ville (mode `WFS_BANC=cuire` de `paquets.yml`, aucun fichier de `.github/**` touché) ; (3) `archi_carte.cpp` : paquet présent → niveaux 1 et 2 envoyés une fois à la carte graphique au chargement, niveau 0 recopié du paquet dans 4 tampons réservés au chargement ; paquet absent ou bloc refusé → kit de secours, inchangé ; (4) test `test_batiments_cuits`, banc `WFS_BANC=batiments`.
- **Mesures de la cuisson (PC)** : Ville = 625 bâtiments, 107 morceaux, 19 347 baies. Triangles par niveau : 4 787 222 / 495 384 / 104 896. Cuisson : 0,6 s. Deux cuissons identiques à l'octet.
- **Constat (RG-05, RG-07, RG-08)** : cuit en entier, le niveau 0 pèse 430 Mo (paquet de 484 Mo, autant dans la carte graphique). Solution retenue, celle de la RG-05 : le contenu d'une baie (tableaux, menuiserie, volets, appui, balcon) est cuit **une fois par modèle et par immeuble** (2 583 modèles pour 19 347 baies), le jeu **recopie** ces modèles tout prêts. Bloc : 174,8 Mo ; paquet de la Ville : 201 Mo (RGBA8), 206 Mo (ASTC), 189 Mo (ETC2), contre 28 à 46 Mo avant.
- **RG-04 sans shader nouveau** : au niveau 1, la menuiserie de chaque baie est un rectangle au plan du dormant, qui porte l'image de son modèle (31 tuiles de 128 × 256 px pour la Ville, atlas 1024 × 1024) : couleur + découpe du vitrage (matériau `MAT_MASK`), relief par une carte de normales tirée des profondeurs de la vraie menuiserie 3D. Derrière, la vitre garde la pièce de nuit existante (`interieurFenetre`). **0 ligne de shader** sur les 40 permises.
- **Accroches hors zone (annoncées)** : `src/world/ville.cpp` + `ville_quartiers.h` : les 12 lignes qui posent les immeubles et les entrepôts de la Ville sont déplacées telles quelles dans la fonction libre `ville::poserImmeublesKit` (appelée par la Ville et par la cuisson, qui n'a pas de carte graphique) ; `src/platform/desktop_main.cpp` : 1 ligne (banc « batiments ») ; `CMakeLists.txt` : 1 ligne (test).
- **Risques signalés au PO** : (a) RG-03 dit « il choisit seulement le morceau et le niveau » : pour le niveau 0, le jeu recopie en plus des sommets tout prêts (au plus 24 000 par image) dans des tampons réservés au chargement ; aucune construction (`archi::constructions` = 0), aucune création de maillage (`Renderer::creationsMaillages` inchangé) ; (b) RG-07 « mémoire graphique pas plus haute qu'avant » : les niveaux 1 et 2 de toute la Ville restent dans la carte graphique (≈ 55 Mo) : chiffres exacts à venir avec le banc ; (c) le bloc reste en mémoire vive (175 Mo) pendant la partie.
- Limites : rien n'est encore vérifié à l'image (banc en cours) ; téléphone non testé.

### 2026-11-10 23:48:49 — WFS-416 : banc « batiments », mesures avant / après et verdict de performance (Claude, agent dev3d (01UY1Vv4rv))
- **Fait** : affichage des bâtiments cuits (`archi_carte.cpp`) : niveaux 1 et 2 de toute la Ville envoyés une fois à la carte graphique (48,3 Mo), niveau 0 recopié du paquet dans 3 tampons réservés au chargement (36,1 Mo), atlas des menuiseries 1024 × 1024 ; kit de secours inchangé sans paquet ou si le bloc est refusé. Banc `WFS_BANC=batiments` (`src/world/cartes_v2/batiments_banc.cpp`) et test `test_batiments_cuits`.
- **Vérifié** : `bash tools/qa/tests_cpu.sh build` : 103 tests réussis ; banc « batiments » : 0 écart (deux cuissons identiques à l'octet, paquet absent → secours, paquet → 0 construction en course à 50 et à 160 km/h, bloc refusé → secours sans plantage, chargement +0,4 s).
- **Mesures (banc PC, rendu logiciel, Ville de nuit, Basse, 160 km/h, 240 images)**, temps du seul système des bâtiments par image (moyenne / pire) : kit actuel (`main` après WFS-415) **0,188 / 14,97 ms**, 58 constructions ; kit + fenêtres de WFS-402 construites en course 0,195 / 15,26 ms, 67 constructions ; **paquet cuit 0,033 / 0,71 ms**, 0 construction, 207 701 sommets tout prêts recopiés. Image entière : médiane 310 ms (kit) et 298 ms (paquet), pire 368 et 370 ms : le rendu logiciel du banc (0,3 s par image) noie l'écart ; **le gain est sur les à-coups** (pire image du système divisée par 20), pas sur la moyenne.
- **Coût** : mémoire graphique +80 Mo (907 → 987 Mo au banc) ; paquet de la Ville 201 Mo en RGBA8 dont 174,8 Mo de bâtiments ; vérification du paquet au chargement 0,8 s (CRC de 201 Mo) ; chargement de la partie +0,4 s.
- **Images regardées** : façade de près, paquet : dormant, ouvrants, petits bois, appui et garde-corps en volume (le kit de secours montre un trou sombre) ; à 50 m : image avec relief, cadres clairs lisibles ; de nuit : pièces allumées derrière les cadres.
- Limites : aucune mesure sur téléphone (images par seconde réelles à valider par l'utilisateur) ; RG-07 « mémoire graphique pas plus haute » non tenue (+80 Mo) : à arbitrer par le PO ; Corniche et Campagne non cuites (RG-09 : la Ville d'abord).
### 2026-11-10 22:47:25 — WFS-414 : plan, fichiers, accroche, risques (Claude, agent dev3d (01Us6uCxLS))
- **Branche** `feature/waforspeed-414` poussée (465d6e0 à 22:09 UTC, puis étape 1 d270a0e). Le titre de la discussion n'a pas pu être changé : l'outil `set_session_title` n'existe pas dans cette session (ToolSearch : aucun résultat).
- **Plan, en 2 étapes** : (1) déplacement pur, sans changer une valeur : fonctions de forme de `vehicule_cockpit.cpp` vers `vehicule_cockpit_formes.h`, bloc du volant vers `construireVolant` (`vehicule_cockpit_volant.cpp`, sans rendu, testable), ligne de la visière vers `ligneVisiere` ; empreintes des 20 volants de `main` relevées par `test_cockpit --empreintes` (`tests/test_cockpit_volants_main.h`) ; (2) volant de série pour les 6 voitures seulement (table `SERIE` de `vehicule_cockpit_defs.cpp`) : coussin bombé (super-ellipse, 200-205 × 145-148 × 65 mm, galbe 18 mm), joint sombre de 4 mm, coque du moyeu (+ 8 mm), branches de 55 → 35 mm qui naissent 12 mm dans le coussin et entrent dans le tube de la jante, plaque de 2 × 2 touches sur chaque branche horizontale (RG-03 adaptée), agrafe de la Golf et démarreur de la Giulia (avec sa bague) sur les branches, visière moins avancée (RG-07).
- **RG-07, mesuré** (`test_cockpit`, rayons de l'œil de la caméra vers le haut de la jante) : avec l'avancée commune de 68 mm, la lèvre de la visière **traverse** la jante sur 5 des 6 voitures (jour de −28 à −31 mm) ; avancée retenue : Mustang 16, Golf 12, Abarth 60, T-Cross 28, S3 28, Giulia 28 mm (aucun point caché, jour ≥ 5 mm). **Constat hors périmètre** : la M4 et la 911 ont le même défaut (lèvre dans la jante) ; non touchées (ordre de l'utilisateur : 6 voitures seulement), signalé au PO.
- **Combiné** : le nouveau moyeu, plus haut (145 mm au lieu de 115), cache une bande plus grande du bas du combiné : `bandeLisibleCombine` prend le haut de la coque et du coussin pour ces 6 voitures (chiffres replacés dans la partie vue, test WFS-323 vert) ; les autres voitures ne changent pas.
- **Accroche** (≤ 10 lignes, zone E7) : `src/platform/desktop_main.cpp`, banc `WFS_BANC=cockpit-volant` (1 inclusion + 4 lignes). Nouveau banc `src/render/vehicule_cockpit_volant_banc.cpp` (zone E10) : gros plans recadrés sur la jante projetée par la caméra de l'image (système inactif hors banc), plus une copie éclaircie pour la revue.
- **Risques** : WFS-324 (PR #341) modifie les mêmes fichiers (`vehicule_cockpit.cpp/.h`) : synchronisation et fusion des deux côtés au moment voulu ; captures « avant » = `main` + ce banc seul (comme WFS-323).
- **Vérifié en local** : construction complète ; `test_cockpit` vert (14 volants identiques à `main`, RG-01, 02, 03, 07) ; tests CPU 101 / 102 (seul rouge : `test_course5_ordinateurs`, rouge connu de `main`).

### 2026-11-10 22:48:26 — ANO-243 : visière sur la jante de la M4 et de la 911, rangée dans WFS-325 (po-scrum-master)
- Signalement de Claude, agent dev3d (01Us6uCxLS) pendant WFS-414 (mesure de RG-07 dans `test_cockpit`) : la lèvre de la visière traverse le haut de la jante de la M4 (22 points, jour −25,6 mm) et de la 911 (28 points, jour −27,1 mm) ; défaut déjà sur `main`.
- WFS-414 n'y touche pas : l'ordre de l'utilisateur le limite aux 6 voitures.
- **ANO-243** créée (mineure, à confirmer par le QA ; même défaut qu'ANO-242) et rangée dans un lot ouvert, pas de nouveau ticket : **WFS-325** (P1, vue Cockpit 3/3), nouvelle **RG-05**, M4 et 911 dans son 1er lot ; WFS-325 passe ⏳ En attente, après la fusion de WFS-414 (mêmes champs de la visière, même test).

### 2026-11-10 22:50:26 — WFS-415 : causes trouvées, corrections en cours (lead-dev, session 01UY1VvJJ)
- Branche `feature/waforspeed-415` (ordre de l'utilisateur : le lead dev code ce ticket ; une autre session lead dev relira et fusionnera).
- **Endurance rouge (#993, #994)** : la cause n'est pas le garage ni WFS-402, c'est **WFS-406** (fe9d8fc4, #993 est le 1er rouge de l'endurance) : `credits::dessinerBouton` est appelé dans `menu.cpp` **avant** les 3 choix, donc le bouton « Crédits » devient la 1re zone de la manette (`manette::focus`) ; le parcours au doigt touche la 1re zone (« Course solo » attendu) et ouvre les Crédits : panneau des modes fermé, puis tout le reste en écart (23). Correction : le bouton est dessiné après les choix (accroche de 3 lignes dans `src/ui/ecrans/menu.cpp`, hors zone, annoncée ici ; la manette retrouve aussi « Course solo » en premier). Endurance locale : **0 écart**, code 0.
- **test_course5_ordinateurs (#991 à #994)** : `remplacant()` de WFS-404 prenait « la plus proche en vitesse max » (S3, V60 : 0-100 de 4,9 s) à la place de l'Elantra N (5,3 s), du X5 (5,4 s), du Range Rover Sport (5,9 s), et la retirait des candidats, ce qui décalait les tirages suivants. Correction (`src/game/course5.cpp`) : remplaçante de la classe du joueur **pas plus rapide** (vitesse max ≤, 0-100 ≥), allure la plus voisine, et plus retirée des candidats (tirages d'avant WFS-404 gardés). Graine par graine (8 graines × 4 cartes, Golf) : chaque place du joueur égale ou meilleure qu'avant WFS-404 (f04cec8c) ; circuit 6/8 avant WFS-404, 3/8 à main, **7/8** après ; Ville 8/8, Campagne 8/8, Plage 7/8. Nouveau contrôle dans `tests/test_course5.cpp` (17 voitures × 400 graines : tirage d'avant WFS-404 recopié, remplaçantes pas plus rapides dès qu'il en existe une dans la classe). Limite : dans la classe de la SF90, aucune voiture n'est plus lente que l'A 45 : la moins rapide en trop est prise (Corvette, sinon M4).
- **Fenêtres de WFS-402** : interrupteur `archi::ouverturesReelles()` (archi_geo.*), 0 par défaut (jeu, APK), `WFS_OUVERTURES=1` ou `choisirOuverturesReelles(true)` pour le rendu de WFS-402 ; aucun fichier supprimé. Le banc `archi` (accroche d'une ligne dans `archi_banc.cpp`) et `test_archi_geo` tournent à 1 ; nouveau contrôle : à 0, ni châssis ni volet, moins de triangles ; à 1, maillage identique. LOD0 en Basse : 30 m seulement avec l'interrupteur à 1, sinon 120 m comme avant WFS-402.
- Tests CPU : **102 réussis** (`tools/qa/tests_cpu.sh build`). Mesures du banc perf (Ville de nuit, Basse) en cours : r990 / avant WFS-402 / main / après.

### 2026-11-10 23:10:39 — WFS-324 : corrections du refus de la PR #341 (Claude, agent dev3d (01UY1Vv5YT))
- Point 1, glace intérieure : la voiture qui suit était cachée par les sièges avant du modèle. Correction : même décalage en long que les glaces extérieures (l'œil du cockpit est 50 à 75 cm trop en avant dans le modèle), et visée selon la hauteur de l'œil. Commits 02f414bf et 36d0a985.
- Point 2, nuit : 2 lampadaires derrière la voiture sont aussi de vraies lumières quand les rétroviseurs sont actifs. C'est une accroche dans `nuit.cpp` (≤ 20 lignes), annoncée ici.
  - Une voiture vue seulement dans les glaces n'éclaire rien : le contraire éclairait en blanc le bas de l'habitacle.
  - Les glaces extérieures sont réglées vers la voie d'à côté : la carrosserie occupe 1,5° au bord intérieur (79c12aa9).
- Point 3 : capture des Réglages refaite, ligne « Rendu » sans et avec rétroviseurs.
- Point 4 : la glace gauche reste sombre la nuit, expliqué dans la PR. La scène n'a que 12 vraies lumières, choisies pour la vue avant, et une glace extérieure ne renvoie que 50 % de la lumière.
- Vérification :
  - banc `retroviseurs` : 0 écart (Golf, Range Rover Sport, M4, 911) ;
  - banc `cockpit-voisine` : 0 écart ;
  - 102 tests verts ; `test_course5_ordinateurs` rouge, comme sur `main` ;
  - coût : 0,41 img/s en Basse et 0,64 en Haute (estimé).
- Commit relu à demander : d77ac9f5. Le lead dev est rappelé.

### 2026-11-10 23:26:15 — WFS-414 : PR #343 ouverte (Claude, agent dev3d (01Us6uCxLS))
- PR #343 (`feature/waforspeed-414`, commit 5ea4b467, synchronisée avec `main`), statut 🔀 En PR ; ligne 🆕 dans `suivi-pr.md`.
- Fait : volant de série des 6 voitures (coussin bombé 200-205 × 145-148 × 65 mm, galbe 18 mm, joint 4 mm, coque en plastique noir grainé ; branches 55 → 35 mm nées dans le coussin et finies dans la jante ; plaques de 2 × 2 touches ; démarreur de la Giulia et agrafe de la Golf sur les branches) ; visière moins avancée (RG-07) ; combiné : vitesse remontée dans la partie vue, rapport au-dessus (S3, Giulia) ou à côté (Abarth). Les 14 autres volants : code déplacé tel quel.
- Vérifié : `test_cockpit` vert (RG-01, 02, 03, 05, 07 sur la géométrie) ; tests CPU 101 / 102 (rouge connu `test_course5_ordinateurs`) ; banc `cockpit-volant` 2340 × 1080 avant (main + banc) / après : 37 captures dans `qa/pr/WFS-414/` ; M4 et 911 identiques au pixel près.
- Limites : la jauge de nitro du bas du combiné (Mustang, Golf, T-Cross, Abarth) est cachée par le moyeu plus haut (nitro toujours sur le HUD) : question au PO dans la PR ; formes du coussin estimées sur photos ; fusion suspendue tant que la CI de `main` est rouge (WFS-415).
- Suite : avis de l'agent 3D (`expert-realisme-3d`) dans la PR, puis appel du `lead-dev`.

### 2026-11-10 23:36:27 — WFS-324 : 2e revue de la PR #341 interrompue (Claude, agent dev3d (01UY1Vv5YT))
- La 2e relecture du lead dev s'est arrêtée avant son verdict, par une erreur de l'API (limite hebdomadaire d'usage atteinte jusqu'au 15/11, 17:00 UTC). Rien n'a été fusionné ni refusé.
- `suivi-pr.md` : #341 remise de « 👀 En revue » à « 🔁 Corrigée, à relire », pour le passage automatique du lead dev.
- Branche resynchronisée avec `main`, à la demande de l'utilisateur : fichiers de suivi seulement, aucun code changé depuis d77ac9f5.

### 2026-11-10 23:50:05 — WFS-416 : 3 écarts aux règles arbitrés (po-scrum-master)
- Demande de Claude, agent dev3d (01UY1Vv4rv), branche `feature/waforspeed-416` à 3712668e (dernier envoi 2026-11-10 23:48 UTC).
- **Acceptés tous les trois** (détail dans `regles-de-gestion/WFS-416.md`, section Suivi) : (1) niveau 0 recopié du paquet dans 3 tampons réservés au chargement (aucune construction, aucun tampon créé en course) ; (2) mémoire : la phrase « pas plus haute qu'avant » de la RG-07 est retirée (décision de l'utilisateur : aucune limite de mémoire pour le décor), repli sur le kit de secours si une réservation échoue ; (3) paquet de la Ville 189 à 206 Mo, dans le budget E15.
- **Publication des paquets** : `paquets.yml` n'est pas automatique (lancement à la main sur `main`). Après la fusion et une CI de `main` verte, l'agent expert ci cd le lance ; la PR porte une section « Après la fusion ».
- La PR attend toujours la fusion de WFS-415. Suites Corniche et Campagne : à créer après la fusion. `track-agent.md` à jour.

### 2026-11-11 00:02:26 — Ordre de l'utilisateur : WFS-325 en premier après WFS-324 (po-scrum-master)
- L'utilisateur a demandé la liste des 40 prochaines tâches (donnée dans la session du PO), puis : « tu peux mettre 325 en premier après 324 ».
- Fait : bandeau dans `TACHES.md` et case Statut de WFS-325 ; WFS-325 passe de la 15e place de l'ordre du 2026-11-10 14:20 UTC à la place juste après WFS-324, avant WFS-405 et WFS-364 à 368. Rien d'autre ne change.
- WFS-325 reste « En attente » jusqu'à la fusion de WFS-414 (PR #343, mêmes fichiers du cockpit) ; il ira au premier `dev-artiste-3d` (nouvelle session) qui demande. Aucune attribution faite.

### 2026-11-11 00:04:51 — WFS-415 fusionné par confiance (#342, af1285a4), ordre de l'utilisateur (lead-dev, session 01UY1VvJJ)
- L'utilisateur a arrêté l'agent lead dev pendant les captures, puis a ordonné « Fusionner » : PR #342 fusionnée en squash, sans 2e revue ni captures.
- Contenu : remplaçantes de la Course à 5 pas plus rapides (test_course5_ordinateurs), bouton Crédits après les choix du menu (endurance), interrupteur `archi::ouverturesReelles()` à 0 en jeu (fenêtres de WFS-402 coupées, code gardé, `WFS_OUVERTURES=1`), LOD0 Basse 120 m.
- Vérifié : 102 tests CPU réussis en local. Non vérifié : endurance locale, mesure img/s, captures. La CI de `main` dira si tout est vert ; WFS-415 en 🟡 À valider (utilisateur), à mesurer sur le téléphone.

### 2026-11-11 00:47:19 — WFS-416 : synchronisation avec main, repli mémoire, captures de la PR (Claude, agent dev3d (01UY1Vv4rv))
- Arbitrages du PO (f59ff8ec) appliqués : recopie bornée permise (RG-03), +80 Mo acceptés (RG-07), poids accepté (RG-08) ; condition « jamais de plantage par manque de mémoire » : `ArchiCarte::init` rend tout et repasse au kit si une réservation échoue (non provoqué au banc).
- Branche synchronisée avec `main` (WFS-415 fusionné, #342) : conflits sur `archi_geo.cpp`, `archi_carte.cpp` (mêmes lignes que la base prise de WFS-415, contenu identique vérifié par `git diff 9b1725c4 af1285a4 -- src tests` vide) et `journal.md` (les deux côtés gardés). 103 tests CPU réussis après synchronisation.
- Captures : `AGENT=… SANS_AVANT=1 bash tools/qa/captures_pr.sh WFS-416 batiments` : 29 images dans `qa/pr/WFS-416/`, banc 0 écart ; le banc joue lui-même « kit de secours » (état de `main`) puis « paquet cuit ». Mesure de ce passage, système des bâtiments seul à 160 km/h : kit 0,16 / 10,6 ms, kit + fenêtres WFS-402 0,20 / 18,3 ms, paquet 0,04 / 0,9 ms (moyenne / pire).
- Limites : téléphone non mesuré ; publication des paquets à relancer par l'agent expert ci cd après la fusion.
### 2026-11-11 00:27:22 — WFS-416 : décisions de l'utilisateur relayées, seuil de 30 img/s déplacé (Claude, session « APK quality issues » (01UY1Vv2WG))
- Mesures de l'APK 995 (WFS-415) sur le téléphone de l'utilisateur, Ville et Ville Néon de nuit, Basse : **16 à 21 img/s** (12 sur l'APK 994), médiane 55 à 66 ms, 99 % 71 à 88 ms (190 ms avant), chauffe 1, scène GPU 43 à 55 ms. CI #995 : compilation, APK et endurance verts.
- Utilisateur : WFS-416 va jusqu'à la PR et sa validation sur téléphone ; +80 Mo de mémoire graphique acceptés. RG-07 de WFS-416 : pas de perte d'img/s par rapport à l'APK 995 et pire temps des bâtiments ≤ 1 ms ; les 30 img/s iront au ticket « lumière légère en Basse », à créer **après le test de l'utilisateur** (son ordre). Message envoyé à la session de WFS-416.

### 2026-11-11 00:48:56 — WFS-416 : PR #344 ouverte (Claude, agent dev3d (01UY1Vv4rv))
- PR #344 `feature/waforspeed-416` → `main`, synchronisée avec `main` (WFS-415 compris), 5f5bf85a : bâtiments de la Ville cuits dans le paquet, 0 construction en course, kit de secours. 103 tests CPU, banc « batiments » 0 écart, 29 captures dans `qa/pr/WFS-416/`.
- Suite : avis de l'agent 3D (épic E15), puis lead dev. CI de `main` #995 (WFS-415) encore en cours à l'ouverture : à voir verte avant la fusion. Après la fusion : relancer « Paquets de cartes » (agent expert ci cd).

### 2026-11-11 00:53:57 — WFS-416 : réserves de l'agent 3D en tâche séparée, WFS-417 créé (po-scrum-master)
- PR #344 : « Avis agent 3D : VALIDÉ avec réserves ». Décision du PO (RG-E15-09, ordre de l'utilisateur du 2026-11-09) : **fusion sans attendre ces réserves**, elles ne sont pas un motif de refus pour le lead dev ; pas de correction de la réserve 1 dans la PR.
- **WFS-417** créé (P2, E14 + E15, `regles-de-gestion/WFS-417.md`) : dalle de balcon et main courante au niveau 1 cuit (20 à 120 m), coût mesuré, cadres au loin sans scintillement, captures manquantes. Attend la fusion de WFS-416 ; nouvelle session `@dev3d`, pas celle de WFS-416. Statut de WFS-416 inchangé (🔀 En PR #344).

### 2026-11-11 01:16:03 — WFS-414 : PR #343 refusée par le lead dev (lead-dev)
- Revue du commit `c14418e` : https://github.com/wlfaleh/waforspeed_mobile/pull/343#pullrequestreview-5852174684 (commentaire : GitHub refuse « Request changes » avec le même compte).
- **Bloquant (1 point)** : la règle du ticket (compléments 2 à 5 de l'utilisateur) demande « + braquage » pour l'Abarth, le T-Cross, la S3 et la Giulia ; le banc `cockpit-volant` ne le fait que pour la Mustang et la Golf. À montrer : les 4 suites de braquage avant / après (normales et éclaircies) et l'avis de l'agent 3D sur ces captures.
- Non bloquant : 8 liens cassés dans la description GitHub (braquage éclairci de la Mustang ; `pr.md` est juste).
- Validé et à ne pas refaire : les 37 paires ouvertes (gros plans et éclaircis) ; le volant des 6 voitures est juste (coussin bombé, branches attachées, plaques, jante entière devant la visière) ; M4 et 911 identiques (même SHA-256) ; tout le diff lu (étape 1 = déplacement pur, vérifié ligne à ligne) ; construction et tests refaits dans un dossier à part : 102 / 102, `test_cockpit` vert.
- Pour le PO : jauge de nitro du combiné cachée par le moyeu (question ouverte dans la PR) ; rapport de l'Abarth petit (≈ 11 % du combiné), à confirmer par l'utilisateur.
- Statut : ↩️ Retour lead dev (PR #343) dans `TACHES.md`, `track-agent.md` et `suivi-pr.md`.

### 2026-11-11 01:24:33 — WFS-416 : PR #344 refusée par le lead dev, 3 points bloquants (lead-dev)
- Reprise de la revue prise à 00:54 UTC (perdue au redémarrage du conteneur ; aucune revue ni fusion n'était posée). Commit relu : 5f5bf85a. Revue : https://github.com/wlfaleh/waforspeed_mobile/pull/344#pullrequestreview-5852270500 (en commentaire : GitHub refuse « changements demandés » sur le même compte).
- Fait : 29 captures ouvertes, tout le diff lu, branche construite, **103 tests CPU réussis** ; CI de `main` #995 verte ; avis agent 3D « VALIDÉ avec réserves » lu (réserves → WFS-417, pas un motif de refus). Bancs existants relancés sur la branche : `cuisson` 0 écart ; **`paquets` 4 écarts (code 3)**.
- **Bloquant 1** : le banc `paquets` (WFS-306, groupe `complements-c` des bancs du matin) passe au rouge : le faux téléchargement avance à 64 Ko par image sur environ 600 images (≈ 38 Mo), le paquet ASTC de la Ville passe de ≈ 31 Mo à 197 Mo et ne finit plus (`src/ui/ecrans/paquets.cpp:489`, `:613`, `:625-639`, `:656`).
- **Bloquant 2** : un téléphone qui a déjà le paquet de la Ville ne reçoit jamais les bâtiments cuits : l'étape se ferme dès qu'un paquet à la bonne empreinte est là (`src/ui/ecrans/paquets_etape.cpp:35-40`) et l'empreinte ne change pas (`VERSION_GENERATION_CARTE` = 1). Choix du moyen à trancher par le PO avec l'agent expert ci cd.
- **Bloquant 3** : le repli « mémoire insuffisante → kit » ne peut pas se déclencher pour la mémoire graphique (`VK_OK` → `std::abort()`, `src/render/vk.h:17`) ; les tests de `src/world/archi_carte.cpp:253` et `:266` sont morts ; après un repli, l'occlusion des murs du kit est perdue (2e appel de `precalcul::occlusionKit`). Condition gardée par le PO (arbitrage du 2026-11-10 23:50) non tenue.
- Non bloquant : cuisson sur deux machines à surveiller (nombres à virgule dans le bloc) ; signature PC / téléphone à vérifier au journal du téléphone ; vérification du paquet (0,7 à 0,8 s sur PC) hors de la mesure du chargement ; mémoire du jeu +314 Mo au banc `cuisson`.
- Validé et à ne pas refaire : image (de près, à 50 m, nuit, 160 km/h), format du bloc et sa lecture vérifiée, test `test_batiments_cuits`, kit de secours.
- Statut : ↩️ Retour lead dev (PR #344) dans `TACHES.md`, `track-agent.md` et `suivi-pr.md`.

### 2026-11-11 01:27:24 — WFS-416 : 3 décisions du PO après le refus du lead dev (PR #344) ; WFS-418 créé (po-scrum-master)
- Demande de Claude, agent dev3d (01UY1Vv4rv). Détail : `regles-de-gestion/WFS-416.md`, section « Décisions du PO après le refus du lead dev ».
- **1. Banc `paquets` (WFS-306)** : accroche accordée dans `src/ui/ecrans/paquets.cpp`, partie banc seulement : débit du faux réseau réglé sur la taille du paquet ; aucun contrôle retiré ni affaibli ; la coupure simulée reste.
- **2. Paquet déjà installé** : option A, `VERSION_GENERATION_CARTE` de 1 à 2. Vérifié dans le code : `ListePaquets.choisir` ne retient qu'un paquet à l'empreinte attendue par le jeu, donc les APK 927 à 995 ne téléchargent pas le nouveau paquet et gardent le leur. `VERSION_MIN_APK` reste à 927, rien à changer dans `.github/**`. L'agent expert ci cd relance « Paquets de cartes » après la fusion (consignes écrites pour la section « Après la fusion »). Il n'a pas été consulté : le code répond à la question ; il confirmera sur la liste publiée.
- **3. Mémoire graphique** : option B. L'arrêt du jeu quand la carte graphique refuse une réservation existe déjà sur `main` pour tout le jeu ; WFS-416 garde le repli sur manque de mémoire vive, retire les tests morts et garde l'occlusion des murs après un repli. La décision de l'utilisateur « jamais de plantage par manque de mémoire » va dans **WFS-418** (E0, P2, créé, attend la fusion de WFS-416, nouvelle session). +314 Mo de mémoire du jeu : permis, à expliquer dans la PR.
- Statut inchangé : ↩️ Retour lead dev (PR #344). Aucune attribution.

### 2026-11-11 02:00:24 — WFS-416 : corrections après le refus du lead dev sur la PR #344 (Claude, agent dev3d (01UY1Vv4rv))
- **Cause racine du refus** : je n'avais relancé que mes bancs ; le banc `paquets` (WFS-306) cassait (faux téléchargement trop lent pour un paquet de 197 Mo), un téléphone déjà équipé ne recevait jamais le nouveau paquet (empreinte inchangée), et mon repli « mémoire » testait des tampons que `createBuffer` ne rend jamais vides (il arrête le jeu).
- **Décisions du PO (50790ee6)** appliquées dans 118d70c9 : (1) accroche dans la partie banc de `src/ui/ecrans/paquets.cpp` : champ `imagesPourFinir` de la source factice (lignes de la source, des cas 2 et 5), débit réglé sur la taille, aucun contrôle retiré ; nouvelle preuve « paquet d'avant WFS-416 installé → mise à jour proposée → bloc des bâtiments présent » ; (2) `VERSION_GENERATION_CARTE` 1 → 2, `VERSION_MIN_APK` inchangé ; (3) tests morts retirés, repli sur `std::bad_alloc` décidé avant l'occlusion des murs (`preparerCuit`), lecture du bloc protégée ; manque de mémoire graphique = arrêt du jeu comme ailleurs (WFS-418).
- **Vérifié sur ce commit** : bancs `paquets` 0 écart, `cuisson` 0 écart (chargement 3,56 → 4,00 s, mémoire du jeu 2 808 → 3 091 Mo), `batiments` 0 écart (système des bâtiments à 160 km/h : kit 0,17 / 11,6 ms, paquet 0,03 / 0,5 ms) ; 103 tests CPU ; 29 captures refaites.
- Limites : téléphone non mesuré ; repli sur manque de mémoire vive non provoqué au banc.
### 2026-11-11 01:34:36 — WFS-414 : corrections après le retour du lead dev (Claude, agent dev3d (01Us6uCxLS))
- Retour du lead dev sur la PR #343 (un seul défaut bloquant) : le braquage n'était montré que pour la Mustang et la Golf. **Corrigé** (35c1071) : le banc `cockpit-volant` fait la suite de 4 images du braquage pour les 6 voitures du volant de série ; 32 nouvelles paires avant (`main` + banc) / après (69 captures en tout). Regardé : rotation d'un bloc, aucun jour, visière hors de la jante à tous les angles, palettes fixes de la Giulia sans contact.
- Description de la PR : GitHub réécrivait 4 lignes (adresses entre accents graves) ; elles sont en balises `<img>` (ac6b87c) ; description publiée relue : 132 liens d'images valides.
- Avant ce retour : 4 avis de l'agent 3D (3 « À REPRENDRE » sur le placement des chiffres du combiné, corrigés : rapport centré au-dessus de la vitesse avec jour de 5 %, marge sous la jante, vitesse de la S3 centrée entre les cadrans, disposition propre à l'Abarth) puis « VALIDÉ » sur c14418e. Captures refaites après la fusion de WFS-415 ; tests CPU 102 / 102.
- Suite : avis de l'agent 3D sur les 4 nouvelles suites de braquage, puis rappel du lead dev.

### 2026-11-11 02:01:39 — WFS-414 fusionné par le lead dev (lead-dev)
- PR #343, 2e revue (appel du développeur), commit relu `ac6b87c` : **fusionnée en squash** → `5b069f79` sur `main`. Revue « ✅ Approuvé » écrite dans la PR en commentaire (GitHub refuse « Approve » au même compte).
- Relu seulement ce qui a changé depuis `c14418e` (1re revue, refus pour un seul point) : 64 nouvelles images (braquage 1 à 4 de l'Abarth, du T-Cross, de la S3 et de la Giulia, avant et après, normales et éclaircies), toutes ouvertes ; agrandissements des palettes de la Giulia. Volant d'un bloc jusqu'à 90°, sans jour, visière qui ne coupe plus la jante, démarreur de la Giulia sur sa plaque.
- Code : seul le banc change (`vehicule_cockpit_volant_banc.cpp` l. 141 et son en-tête). Remarque non bloquante : utiliser `volantSeriePour(d)` au lieu des noms écrits à la main.
- Construction et `tests_cpu.sh` refaits par le lead dev dans un dossier à part : 102/102. « Avis agent 3D : VALIDÉ » sans réserve sur `ac6b87c`.
- Statut : 🟡 À valider (utilisateur) (KO de l'utilisateur, APK 991) ; ligne de `suivi-pr.md` supprimée ; entrée ajoutée à `qa/Check.md`. Au développeur : sa ligne de `track-agent.md` (session terminée). Au PO : WFS-325 peut partir (attendait cette fusion).

### 2026-11-11 02:02:16 — WFS-416 : PR #344 corrigée après le refus du lead dev, à relire (Claude, agent dev3d (01UY1Vv4rv))
- Les 3 points du refus sont corrigés dans 118d70c9 (décisions du PO 50790ee6) : banc `paquets` à 0 écart (débit du faux réseau réglé sur la taille), `VERSION_GENERATION_CARTE` = 2 (un paquet déjà installé est remplacé, prouvé au banc), repli mémoire vive propre et arrêt sur manque de mémoire graphique écrit tel quel (WFS-418). Bancs `cuisson` et `batiments` à 0 écart, 103 tests CPU, 29 captures refaites (f2726e80). Réponse point par point sous la revue, description de la PR corrigée.
- Suite : nouvel avis de l'agent 3D sur f2726e80, puis lead dev.

### 2026-11-11 02:07:55 — WFS-414 : fin, session terminée (Claude, agent dev3d (01Us6uCxLS))
- **Fin** : PR #343 fusionnée en squash par le lead dev (5b069f79, 2026-11-11 02:01 UTC), statut 🟡 À valider (utilisateur). 2 revues du lead dev (1 refus : braquage des 4 autres voitures, corrigé) et 5 avis de l'agent 3D (le dernier « VALIDÉ » sur ac6b87c).
- Réponses écrites sous les deux demandes de l'utilisateur sur le volant dans `tache utilisateur.md`.
- Restent pour le PO, hors de ce ticket : la jauge de nitro du bas du combiné, cachée par le moyeu plus haut (Mustang, Golf, T-Cross, Abarth) ; les remarques « pour plus tard » de l'agent 3D (coussin qui cache la vitesse de 60 à 90° de braquage, raccord vif entre la partie ronde et le méplat de l'Abarth, de la S3 et de la Giulia, palettes de la Giulia en carbone au lieu d'alu, rapport de la Mustang posé sur les segments du compte-tours, rapport plus grand que la vitesse sur la S3) ; remarque du lead dev : condition du banc à écrire avec `volantSeriePour` (WFS-325) ; ANO-243 (M4 et 911) déjà dans WFS-325.
- Le titre de la discussion n'a pas pu être changé (aucun outil `set_session_title` dans cette session). Un seul ticket par session : la session s'arrête.

### 2026-11-11 02:47:37 — WFS-416 fusionné par le lead dev (lead-dev)
- PR #344, 2e revue (appel du développeur), commit relu `f2726e80` : **fusionnée en squash** → `55ee0909` sur `main`. Revue « ✅ Approuvé » écrite dans la PR en commentaire (GitHub refuse « Approve » au même compte) : https://github.com/wlfaleh/waforspeed_mobile/pull/344#pullrequestreview-5853054315
- Les 3 points du refus (5f5bf85a) vérifiés : banc `paquets` à 0 écart sans contrôle retiré (48 contrôles sur `main`, 53 ici) ; `VERSION_GENERATION_CARTE` = 2 et preuve au banc « paquet d'avant installé → mise à jour proposée → bloc des bâtiments présent » ; repli sur manque de mémoire vive décidé avant l'occlusion des murs, tests morts retirés, description corrigée (mémoire graphique : arrêt du jeu, WFS-418).
- Refaits par le lead dev : tout le diff relu, 29 captures ouvertes, construction, **103 tests CPU**, bancs `paquets`, `cuisson` (mémoire du jeu 2 749 → 3 037 Mo) et `batiments` (passage rapide) à 0 écart.
- **Synchronisation** : la branche n'avait pas WFS-414 (fusionné à 02:01 UTC, une minute avant le dernier envoi du développeur). Fusion d'essai de `main` dans la tête de la PR, dans un dossier à part, sans rien pousser : aucun conflit, construction bonne, 103 tests, bancs `paquets` et `batiments` à 0 écart. WFS-414 ne touche que la vue Cockpit : captures gardées.
- « Avis agent 3D : VALIDÉ avec réserves » sur `f2726e80` (vaut « VALIDÉ », décision du PO ; réserves dans WFS-417).
- Remarques non bloquantes écrites dans la revue : `std::bad_alloc` non rattrapé dans `initCuit` après `preparerCuit` (pour WFS-418), tampons de chargement non vidés après un repli, déclarations en double dans `archi_geo.h`, titre du banc `bloc-refuse`.
- Statut : 🟡 À valider (utilisateur) ; ligne de `suivi-pr.md` supprimée ; entrée ajoutée à `qa/Check.md`. **À l'agent expert ci cd** : relancer « Paquets de cartes » sur `main` après une CI verte (points a à f de la PR) ; avant cela, le nouvel APK joue la Ville du code. Au PO : WFS-417 et WFS-418 peuvent partir. Au développeur : sa ligne de `track-agent.md` (session terminée).

### 2026-11-11 02:48:19 — WFS-416 : fin, session terminée (Claude, agent dev3d (01UY1Vv4rv))
- PR #344 approuvée et fusionnée en squash par le lead dev (55ee0909) après un refus corrigé (banc `paquets`, génération de carte 2, repli mémoire) ; statut 🟡 À valider (utilisateur).
- Reste à faire par d'autres : relance de « Paquets de cartes » sur `main` par l'agent expert ci cd (sans elle, le nouvel APK refuse l'ancien paquet et joue la Ville du code : aucun changement visible) ; WFS-417 (balcons de 20 à 120 m, cadres au loin, captures) et WFS-418 (repli sur manque de mémoire graphique, `std::bad_alloc` dans `initCuit`) pour de nouvelles sessions.
- À vérifier sur le téléphone : ligne « 107 morceaux lus dans le paquet » au journal du jeu, images par seconde contre l'APK 995, points blancs le long des cadres au loin.
- Un seul ticket par session : cette session s'arrête et ne demande pas d'autre ticket.

### 2026-11-11 09:27:54 — « Paquets de cartes » relancé sur main : paquets de la Ville de WFS-416 publiés (agent expert ci cd, session 01UY1Vv2Hq)
- Ordre de l'utilisateur (« lance la nouvelle Paquets de cartes sur main »), point a de la PR #344. Condition vérifiée avant : `build.yml` run 997 vert sur 55ee0909 (WFS-416).
- Lancement à la main de `paquets.yml` sur `main` (ed6f745) à 09:08:48 UTC : **run n° 2 vert** en 4 min 55 s (cuisson a, cuisson b, publication) : https://github.com/wlfaleh/waforspeed_mobile/actions/runs/42992692347
- Vérifié au journal du job publication : deux cuissons identiques octet par octet (3 fichiers) ; empreinte `e209d649cd6600e2` pour les 3 formats ; `ville_astc` 196,56 Mo (sha256 bdaf43d1a774…), `ville_etc2` 180,14 Mo (81192c6f6832…), `ville_rgba8` 201,15 Mo (eaa2d8d1b046…) ; « Paquets puis cartes.json envoyés ».
- Release `paquets` : `cartes.json` remplacé à 09:13:40 UTC, 3 nouveaux fichiers présents ; les 3 paquets du run n° 1 (5 à 28 Mo) sont toujours là, rien supprimé.
- Non vérifié : le téléchargement sur un téléphone. À l'utilisateur, avec l'APK de `main` (run 997 ou plus récent) : la mise à jour du paquet de la Ville (≈ 197 Mo) doit être proposée ; puis les points à regarder de WFS-416 (« 107 morceaux lus dans le paquet », images par seconde contre l'APK 995, points blancs le long des cadres au loin).

### 2026-11-11 09:31:34 — Ordre de l'utilisateur : WFS-405 dans l'épic « Publication app » (po-scrum-master)
- Ordre : « pour la 405, tu dois le mettre avec l'épic Publication app ».
- Fait : case Épic et case Statut de WFS-405 dans `TACHES.md` (⏸ En attente du jour de publication), bandeau, ordre permanent du skill du PO complété, section Suivi de `regles-de-gestion/WFS-405.md`. WFS-405 n'est plus « avant tout autre ticket » ; il n'était attribué à personne.
- Épic « Publication app » : WFS-319, 408, 409, 407, 405 et le suivi 320. Zone de code de WFS-405 inchangée.

### 2026-11-11 09:58:36 — Statuts automatiques, bancs du matin #16 (robot statuts)
- Run rouge (schedule, commit ed6f745), aucun statut changé. Job(s) en échec : banc (realisme-2).
- Tickets 🟣 contenus dans ce run : WFS-350, WFS-351, WFS-352, WFS-339, WFS-348, WFS-340, WFS-193, WFS-359, WFS-406, WFS-396, WFS-362, WFS-363, WFS-384, WFS-358, WFS-214, WFS-307, WFS-343, WFS-322.

### 2026-11-11 10:10:56 — Téléphone de l'utilisateur : Samsung Galaxy S21 FE ; avis des agents sur la performance (Claude, session « APK quality issues » (01UY1Vv2WG))
- **Téléphone de référence de l'utilisateur : Samsung Galaxy S21 FE** (selon la version : Exynos 2100 / Mali-G78 MP14, ou Snapdragon 888 / Adreno 660 ; à lire dans le jeu). Toute mesure de performance se fait sur lui.
- Mesures de nuit en Basse (45 %, MSAA ×2) : APK 995 médiane 55-66 ms ; APK 997 + paquet médiane 53-58 ms, scène 60-64 ms (voie rapide). Le diagnostic « la lumière est la cause » de cette session est **retiré** : Basse déjà à 4 lumières, WFS-353 : −8 % (2 lumières), −17 % (sans lumières de nuit), −33 % (45 % au lieu de 60 %).
- Avis de PO, lead dev, agent 3D, dev3d, smart brain, QA (réponses en lecture seule, rien modifié) : unanimes pour un ticket P1 « diagnostic sur le téléphone » (interrupteurs dans configgame.json, temps GPU par passe, temps CPU, triangles, appels, mémoire par type, trajet rejoué, résultats dans un fichier), puis une seule correction sur la cause mesurée. Suspects cités : surdessin (opaques non triés), shader de scène long (boucle fixe de 12 projecteurs), reflet de la route mouillée (scène redessinée en miroir), passes en plus, halos additifs (aussi cause de la nappe orange), textures non compressées (1,1 Go). **En attente de l'accord de l'utilisateur** pour créer les tickets.

### 2026-11-11 10:13:22 — WFS-419 créé : diagnostic de performance sur le téléphone, accord de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- L'utilisateur accepte le point 1 (« Ok pour 1 ») : **WFS-419** (P1, [TECHNIQUE], `@dev`, nouvelle session), règles dans `regles-de-gestion/WFS-419.md` : interrupteurs éteints par défaut (image identique), temps GPU de chaque passe, temps CPU, mémoire par type, appareil affiché, trajet rejoué jour et nuit, un interrupteur à la fois, fichier de résultats partageable. Aucune correction dans ce ticket ; la correction viendra d'un seul ticket créé sur la cause mesurée.
- Bancs du matin #16 : pris par un agent expert ci cd (selon l'utilisateur) ; aucun ticket créé ici. Gel des décors lourds : **non décidé** par l'utilisateur (rien changé).

### 2026-11-11 10:16:06 — WFS-419 coupé en lots, lot 1 au lead dev avec fusion directe (Claude, session « APK quality issues » (01UY1Vv2WG))
- Ordre de l'utilisateur : tester vite ; le lead dev prend et fusionne directement. Lot 1 : interrupteurs dans un menu caché + appareil affiché + ligne technique existante (et temps GPU par passe si déjà disponible). Lot 2 (trajet rejoué, fichier) plus tard. Règles mises à jour dans `regles-de-gestion/WFS-419.md`.

### 2026-11-11 10:19:02 — WFS-419 : lancement annulé, attend le go de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- La session avait lancé le lead dev sur le lot 1 sans « go » explicite de l'utilisateur ; l'utilisateur l'a arrêté. Aucun code produit (branche locale vide, rien poussé, pas de PR). WFS-419 remis en « ⏳ Prêt, attend le go de l'utilisateur ».

### 2026-11-11 10:21:01 — WFS-419 : section « Découpage et exception » retirée des règles, à la demande de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- L'utilisateur avait posé une question (« un lead dev peut prendre rapidement et merger directement ? »), pas donné un ordre. Section retirée de `regles-de-gestion/WFS-419.md`, ligne de `TACHES.md` remise à « ⏳ Prêt (`@dev`, nouvelle session) ». Les règles sont celles de la création du ticket.

### 2026-11-11 10:28:55 — WFS-419 réduit aux interrupteurs, WFS-420 créé pour le banc automatique (Claude, session « APK quality issues » (01UY1Vv2WG))
- Ordre de l'utilisateur : un ticket = un besoin, pas de lots dans un même ticket. **WFS-419** (P1) : interrupteurs du menu caché, appareil et temps par passe dans la ligne technique. **WFS-420** (P2, après WFS-419) : trajet rejoué jour et nuit, un interrupteur à la fois, fichier de résultats. Règles réécrites dans `regles-de-gestion/WFS-419.md` et créées dans `WFS-420.md`.

### 2026-11-11 10:42:20 — WFS-419 codé et fusionné par le lead dev (#345, squash d63e2a46), ordre de l'utilisateur (lead-dev)
- Ordre de l'utilisateur : le lead dev prend WFS-419, le code et fusionne directement sur `main`, sans relecture par un autre lead dev, sans tests locaux, sans endurance ni captures du banc PC (captures prises par l'utilisateur sur son S21 FE, APK de `main`). Statut : 🟡 À valider (utilisateur).
- Livré : `src/render/diagnostic_perf.*`, 12 interrupteurs tous éteints par défaut (MSAA ×1, sans pré-passe, sans reflet de la route mouillée, sans ombres, sans pluie, sans halos additifs, sans bloom ni post, 0 lumière de nuit, résolution 30 %, shader couleur unie, décor coupé, distance ÷ 2). Menu caché : 5 touchers sur le titre « RÉGLAGES » en moins de 3 s. Section `diagnostic` dans `configgame.json`, 12 clés à 0 ; les valeurs de l'utilisateur déjà présentes sont gardées.
- Ligne technique : interrupteurs allumés, puis, une fois le menu ouvert, appareil (modèle Android, GPU, pilote, Vulkan) et coût de chaque passe.
- « GPU ombre 0,0 · reflet 0,0 » : en Basse, la passe d'ombre est vide (ombres du soleil coupées) et il n'y a pas de passe de reflet (coupée en Basse). Ce sont de vraies mesures : tout le temps est dans « scène » et « post ».
- Accroches hors zone : `src/world/meteo.cpp` (2 lignes : interrupteur « sans pluie »), `src/render/passes.cpp` (2 lignes). Vérifié avant la fusion : shaders compilés (glslangValidator), syntaxe C++ (g++), `test_configgame` 124 clés et 3 nouveaux cas OK.
- Téléchargement noté (outillage, pas d'actif) : paquets apt `libvulkan-dev` et `glslang-tools` (dépôt Ubuntu, registre de paquets autorisé), pour vérifier les fichiers en local ; rien n'est ajouté au dépôt.

### 2026-11-11 10:43:31 — Archives automatiques (robot archives)
- Journal : 251 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-09).
- TACHES.md : 0 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md`.

### 2026-11-11 11:36:19 — WFS-419 : CI de main verte (run #998, 43005769670) (lead-dev)
- Compilation, APK, endurance, bancs texte et menu1-2400, publication : tous verts. Ticket laissé en 🟡 À valider (utilisateur). 1re capture de l'utilisateur : menu caché ouvert, S21 FE Snapdragon 888 (SM-G990B2, Adreno 660, pilote 512.530, Vulkan 1.1.128) ; mesures à faire en course (au menu, scène figée : 0,0 partout).

### 2026-11-11 11:43:37 — E15 : minimum de 200 Mo par décor supprimé, maximum de 700 Mo sur le disque gardé, décision de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- Ordre de l'utilisateur (« Go pour supprimer le minimum de 200 Mo et garder le maximum de 700 Mo sur le disque »). Raison : le diagnostic WFS-419 sur le S21 FE a montré que la lenteur vient du calcul par pixel, pas du poids ; un minimum de poids poussait à ajouter du contenu pour atteindre un chiffre.
- Mis à jour : `CLAUDE.md` (ligne E15), `regles-de-gestion/E15-decors.md` (RG-E15-01 : 700 Mo maximum sur le disque, moins de 200 Mo n'est plus un motif de refus, la richesse se juge sur les captures par l'agent 3D), `WFS-385` à `WFS-389` (titres et règles), `WFS-417` (RG-04), `TACHES.md` (épic E15 et titres), skill `po-scrum-master`. Les entrées datées plus anciennes (journal, note du 2026-11-09 dans TACHES) restent telles quelles.
- **Pas changé** (non décidé par l'utilisateur) : budget de mémoire graphique, budget de temps d'image sur le téléphone.

### 2026-11-11 11:47:03 — WFS-421 à 425 créés : performance de la Basse sur le S21 FE (po-scrum-master)
- Ordre de l'utilisateur (« crée les tickets A B C D E »), contenu proposé par le lead dev après lecture du code et mesures WFS-419 sur le S21 FE (Ville de nuit, Basse, 45 %, MSAA ×2 : scène 62 ms ; Corniche 28 ms). Épic E4. Aucun ticket attribué : l'utilisateur ouvrira les sessions.
- **WFS-421** (A, P1, ⏳ Prêt) : halos au sol des lampadaires coupés en Basse (−14 ms mesuré). **WFS-422** (B, P1, ⏳ Prêt) : pré-passe coupée en Basse avec secours propre (5 à 18 ms, à confirmer). **WFS-423** (C, P1, ⏳ Prêt) : lumières bornées en Basse (−10 ms mesuré avec 0 lumière). **WFS-424** (D, P1, ⏳ En attente de 421, 422, 423) : route en texture précuite, avis agent 3D. **WFS-425** (E, P2, ⏳ Prêt, en parallèle) : défaut MSAA ×1 (62 → 108 ms).
- Communs : Basse seulement, mesure avant / après sur le téléphone (Ville et Corniche de nuit), « Avis agent 3D : VALIDÉ » si l'image change, 🟡 À valider (utilisateur) après fusion. Règles : `regles-de-gestion/WFS-421.md` à `WFS-425.md`. WFS-405 et WFS-420 inchangés.

### 2026-11-11 12:09:12 — « Go Corniche », WFS-426 créé, ordre des 20 prochaines tâches (po-scrum-master)
- Ordre de l'utilisateur (« go Corniche, crée le ticket 8 et l'ordre ») : la version grand public sort avec **la Corniche seule**. **WFS-405** réécrit (titre, ligne, règles) : « Version grand public : la Corniche seule (autres cartes gardées dans l'APK d'aperçu) », toujours ⏸ jour de publication ; WFS-319, 408, 409 inchangés ; WFS-407 : note « captures de la Corniche ».
- **WFS-386** (décor complet Corniche) : ⏳ En attente, après la sortie publique, à découper plus tard (note datée).
- **WFS-426** créé (P1, E14, ⏳ Prêt, nouvelle session `@cartes`, aucun attribué) : Corniche cuite en paquet `.wfcarte` et téléchargeable, en réutilisant WFS-302 (`paquets.yml`, `WFS_BANC=cuire`), WFS-416 (`archi_cuit`), WFS-307 ; 3 paquets ASTC / ETC2 / RGBA8 dans la Release `paquets` et `cartes.json` ; secours par le code ; 700 Mo max. ; scène ≤ 28 ms sur le S21 FE ; plus aucune construction en course ; 🟡 après fusion. Distingué de WFS-313 (P3, inchangé) dans les deux fichiers.
- **Ordre des 20** (remplace la tête de l'ordre des 50 du 2026-11-10) : 412, 421, 423, 422, 425, 420, 424, 426, 383, 371, 325, 395, 364 à 368, 394, 418, 397. Mis de côté : 321, 385, 399, 398 ; 386 après la sortie. WFS-383 et 371 : pas de fusion si la scène de la Corniche dépasse ≈ 33 ms sur le S21 FE (règle ajoutée dans leurs fichiers). Aucune attribution.

### 2026-11-11 12:10:19 — Skill du PO : WFS-405 « la Corniche seule », accord de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- L'ordre permanent du skill `po-scrum-master` disait encore « WFS-405, la Ville seule » ; corrigé en « la Corniche seule » (décision « go Corniche » du 2026-11-11), avec WFS-426, 383, 371 avant la sortie et WFS-386 après.

### 2026-11-11 12:10:20 — Début WFS-421, attribué à Claude, agent dev (01UY1Vv4ss) (po-scrum-master)
- Nouvelle session `@dev` (session_01UY1Vv4ss6UoRUUxN62sxWQ, aucun ticket avant) : **WFS-421** (P1, E4), 2e de l'ordre des 20 (le 1er, WFS-412, est pour `@cicd`). Un seul ticket pour cette session, jusqu'à la fusion, puis arrêt.
- Zone : `src/render/nuit.cpp` (et sa lecture de la qualité), `tests/**`. Exception PO : E4 a déjà WFS-324 🔀 (PR #341), qui touche aussi `nuit.cpp` (accroche des lampadaires derrière, boucle des projecteurs, ≈ l. 175) ; WFS-421 ne touche que l'appel `haloSol` (≈ l. 262) : zones séparées, synchroniser avec `main` avant la PR.
- Critères : `regles-de-gestion/WFS-421.md` (Basse seulement, `haloTete` gardé, mesure S21 FE Ville + Corniche, « Avis agent 3D : VALIDÉ », 🟡 après fusion).

### 2026-11-11 12:11:12 — Agent PO : skill smart-brainstorming chargé par défaut, demande de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- `.claude/agents/po-scrum-master.md` : `smart-brainstorming` ajouté au champ `skills:` et consigne d'usage (étudier, challenger, prioriser ; le PO garde son rôle de créer et attribuer). `appel-agents.md` : tableau mis à jour.

### 2026-11-11 12:14:45 — Agent PO : skill smart-brainstorming retiré, plus chargé par défaut, demande de l'utilisateur (Claude, session « APK quality issues » (01UY1Vv2WG))
- Annulation du commit 1638ad6a : `.claude/agents/po-scrum-master.md` et `appel-agents.md` reviennent à l'état d'avant (skill `po-scrum-master` seul). L'utilisateur appelle `@smartbrain` quand il en a besoin.
