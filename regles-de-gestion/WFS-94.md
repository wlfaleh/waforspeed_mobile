# WFS-94 — Corniche : quartiers Port de plaisance, Côte rocheuse et Village de pêcheurs + ville de nuit : façades sombres et fenêtres allumées (fin d'ANO-37) + finitions de WFS-93
Épic : E2 · Priorité : P2 Haute · Dépend de : WFS-93 ✅ (2026-11-05) · Taille : 3 jours (2 jours de corniche + ANO-40 + 1 jour de façades de nuit ; ANO-49 et ANO-51 : quelques heures) · 🔧 attribué le 2026-11-05 à Copilot
(session 0e5c192d) : lire d'abord la section « Attribution du 2026-11-05 » en fin de fiche
Anomalies corrigées : **ANO-40** (cadrage du banc), **ANO-37** (fin : façades et fenêtres de nuit, RG-A2 à RG-A5), **ANO-49** (trottoir de la ville),
**ANO-51** (texte lointain des panneaux du circuit).

## Besoin
La Corniche (`src/world/plage.cpp`) porte les bas-côtés de chaque zone mais pas leurs objets : pas de villas côté terre,
pas de bateaux ni de bornes d'amarrage dans la marina, pas de rochers, de pins ni de glissières sur la côte rocheuse, pas
de maisons ni de barques au village, pas de tour de sauveteur. La carte paraît vide comparée au jeu web.

## Règles de gestion
- RG-01 : `beach.corniche` : villas côté terre (`villa`, RW + 12-18, une fois sur 0,75), palmiers côté terre, tours de
  sauveteur (`tower`, tous les 160-240 tronçons, RW + 22) ; palmiers et parasols côté mer déjà faits.
- RG-02 : `beach.marina` : bornes (`bollard`) tous les 3 tronçons à RW + 6,5, bateaux (`boat`, 7-14 m, voile 6 fois sur 10,
  4 couleurs) à RW + 13-31, villas colorées côté terre à RW + 7 avec commerces (`placeShop`, déjà porté par WFS-71).
- RG-03 : `beach.rochers` : rochers (`rock`, taille 1-4,5) côté mer, pins et arbres ronds côté terre (`vegetation.*`),
  glissière (`guard`) tous les 3 tronçons à RW + 1,2.
- RG-04 : `beach.village` : maisons (`villa`) des deux côtés, barques (`boat` sans voile) à RW + 30-40.
- RG-05 : formes et couleurs = `DECOR.villa`, `tower`, `bollard`, `boat`, `rock`, `guard` de `decor.js` (primitives de
  `decor_carte.*`) ; ≤ 300 appels de dessin.

## À réutiliser du jeu web
`js/world/decor.js` : `beach.corniche`, `marina`, `rochers`, `village` (l. 99-148), `DECOR.villa`, `tower`, `bollard`,
`boat`, `rock`, `guard`. Natif : `decor_carte.*` (villa déjà portée pour la campagne), `vegetation.*`, `commerces.cpp`.

## Critères d'acceptation
- [ ] `carte-plage-marina`, `carte-plage-rochers`, `carte-plage-village` (nouvelles captures du tableau `CAPS` de
  `cartes_banc.cpp`) : objets de la zone visibles ; `carte-plage-corniche` avec villas côté terre ; ≤ 300 appels de dessin ;
  tests CPU verts

## Ajout du PO (2026-11-04) — ANO-40 (mineure) : captures de la vie de la plage lisibles
- RG-A1 : banc `vie-portique` pris à **25 m** du portique « CHECKPOINT 1 » (texte lisible) et `vie-plage` à **15 m** de la plage
  (serviettes et baigneurs visibles) ; seul le cadrage du banc change (banc de `src/world/commerces.cpp`, ajouté à la zone de ce ticket).
- Critère : - [ ] `vie-portique` : « CHECKPOINT 1 » lisible ; `vie-plage` : serviettes et baigneurs visibles.

## Ajout du PO (2026-11-05) — ANO-37 (fin) : ville de nuit, façades sombres et fenêtres allumées
Constat de WFS-97 (Copilot dc0cdd3f, banc) : l'ambiance de nuit est corrigée. Mais les immeubles du fond de rue restent clairs
dans l'axe des phares et des lampadaires : `ville.cpp` les éclaire comme de jour. Au web, la nuit, les façades sont **non
éclairées** (`MAT.bld` = `MeshBasicMaterial` + texture de nuit `TEXS.facade`, `js/world/renderer.js` l. 304 et 741).
- RG-A2 : la nuit (`WFS_MOMENT` = `nuit`, lu comme dans `traffic_system.cpp` l. 27 ; la carte est reconstruite quand le
  moment change, `game.h` l. 39), les façades de la ville prennent les couleurs de la texture de nuit du web
  (`makeTextures`, `js/world/renderer.js` l. 85-99) : fond `#0d0f17`, fenêtres éteintes `#161a26`, coin des toits
  `#07080c`. Elles restent sombres même dans l'axe des phares et des lampadaires.
- RG-A3 : fenêtres allumées comme au web, au hasard mais toujours les mêmes (graine fixe, comme `mulberry32(31)`) :
  - 24 % chaudes, rgb(95-115, 68-82, 40-50) ;
  - 8 % froides, rgb(55-70, 78-92, 100-115) ;
  - 1,5 % sarcelle `#007f8c` ou orange `#8c3b00`.
  Elles se placent sur la grille de fenêtres de la façade native (`facadeTexture` : 2 fenêtres par tuile de 4 m), ou
  sur celle du web (une case = 3,2 m × 3,4 m). Elles brillent **sans dépendre des phares**.
- RG-A4 (moyen imposé, sans toucher au moteur) : le moteur n'a pas de carte émissive. `makeMaterial(m, color, normal, mr)`,
  et `scene.frag` ajoute une émission **constante** par matériau (`col += pc.emissive.rgb`). Les fenêtres allumées sont donc
  de **petits quads** posés sur les façades. Ils utilisent 2 ou 3 matériaux émissifs (chaud, froid, rare), avec une couleur
  convertie par `effets::versHdr` (`src/render/effets_particules.h`, à inclure sans le modifier). Ils sont regroupés avec
  la géométrie de chaque morceau de carte : 3 appels de dessin de plus par morceau au plus. Interdit : `renderer.*` et
  `shaders/**` (WFS-100).
- RG-A5 : de jour, rien ne change (`facadeTexture` actuelle) ; le critère des 300 appels de dessin tient toujours ; la
  vitesse d'affichage ne baisse pas de façon visible au banc `qualite-*`. Vitrines du rez-de-chaussée éclairées 2 immeubles
  sur 3 la nuit (`js/world/decor.js` l. 678, `BLD_C.shopLit #7a5a34`), seulement si les vitrines existent déjà dans le
  natif.
- Critère : - [ ] `course-nuit-ville-*` : façades sombres jusqu'au fond de rue, même dans l'axe des phares, et environ une
  fenêtre sur trois allumée (surtout chaudes, quelques froides). Les néons ressortent. Captures de jour inchangées.
- Zone ajoutée : `src/world/ville.cpp` (façades et fenêtres de nuit seulement) ; `#include "effets_particules.h"` permis.

## Hors périmètre
Bateaux animés, mer animée ; copies fantômes des textes lumineux la nuit (ANO-50, bloom de `src/render`, WFS-100) ;
images par seconde sur le téléphone (à mesurer par l'utilisateur, voir `qa/Check.md`, ligne WFS-93).

## Zone de code
`src/world/plage.cpp`, `src/world/cartes_banc.cpp`, nouveaux fichiers `src/world/plage_*.cpp`, `src/world/commerces.cpp` (banc, ANO-40), `src/world/ville.cpp` (façades et fenêtres de nuit, ANO-37). Interdit : `src/render/**` (sauf l'inclusion de `effets_particules.h`), `shaders/**`, `src/game/**`, `src/platform/**`, iOS. Élargie le 2026-11-05 à l'attribution (voir ci-dessous).

## Attribution du 2026-11-05 (PO) — à lire en premier
Attribué à **Copilot (session 0e5c192d)**, branche `feature/waforspeed-94`, à pousser tout de suite (règle 9 de
`TACHES.md` : branche poussée dans l'heure). WFS-93 est ✅ (PR #89). Priorité relevée à P2 (comme au tableau). Le ticket
garde RG-01 à RG-05 et RG-A1, et reprend deux finitions de WFS-93 signalées par le développeur (ANO-49, ANO-51), ainsi
que la fin d'ANO-37 (façades et fenêtres de nuit : RG-A2 à RG-A5 ci-dessus, déjà écrites par le PO à 01:13:50).

### Étape 0 (avant le code) : fusionner la PR #76 (WFS-88, iOS)
Confiée par le PO (RG-10 et RG-12 de `WFS-103.md` : ordre de l'utilisateur, seule exception au gel iOS ; Claude (session
trafic) sans activité depuis 2 h). Si #76 est déjà fusionnée, rien à faire.
1. Relire le diff : `renderer.cpp` (+44) et `vk.cpp` (+9), tout sous `#if defined(__APPLE__)` ; sur Android,
   `vkCmdDrawIndexed(..., vofs = p.vofs, ...)` comme avant. Ranger seulement le commentaire collé après
   `ETAPE_RENDU("ombres");` (« ressources par image en vol » sur sa propre ligne).
2. `git merge origin/main` dans `feature/waforspeed-88` (fusion sans conflit avec `main` 6e09c9f, vérifiée par le PO),
   compiler, tests CPU.
3. Fusionner (fusion par confiance), ligne `qa/Check.md` (« WFS-88 / PR #76, fusion par confiance, rien ne change pour
   Android ; iOS gelé »), WFS-103 et WFS-88 au Journal. **Aucun autre travail iOS** : WFS-88 reste ⏸.

### Règles ajoutées
- **RG-06 (ANO-49, trottoir de la ville)** : comme `GROUND.city` (`decor.js` l. 241-247 : trottoir `walk` de RW + 0,5 à
  **RW + 6**, hauteur 0,15, couleur `GCOL.walk` #8a8884), la ville ajoute sa propre bande de trottoir de **RW + 4 à RW + 6**
  à 0,15 m, des deux côtés, **dans les 6 quartiers** (identifiants `centre`, `residentiel`, `parc`, `port` de `CITY`,
  `zones.cpp` l. 11 : Centre-ville, Quartier résidentiel, Parc urbain, Zone portuaire, Quartier d'affaires, Vieux quartier), au même
  endroit que les bandes du parc et du port (`ville::bandes()` / `solY()` de `ville_quartiers.cpp`, dessin dans
  `buildChunk` de `ville.cpp`). Aspect identique au trottoir de Game : matériau `MAT_WALK` (dalles de 0,6 m dessinées par
  le shader) avec uv en mètres, u = latéral, v = segment × `SEG_M` (comme `Game::buildChunkCpu`, `game.cpp` l. 93) ; la
  jointure à RW + 4 ne doit pas se voir. **Ne pas toucher `WALK_W`** de `game.cpp` (zone E0, accroches de WFS-102 ; la
  Corniche commence à RW + 4). Les terrasses des commerces restent où elles sont : elles se retrouvent sur le trottoir.
- **RG-07 (ANO-51, texte lointain des panneaux du circuit)** : comme `TEXS.ads` (`renderer.js` l. 150-155 : canevas
  **opaque** 1024 × 112 par texte, fond `bg`, texte `fg`, écrit deux fois) et `MAT.ads` (l. 309 : sans test alpha), le
  panneau du circuit (`T_ADBOARD`, `circuit.cpp` l. 129-140) devient une **texture opaque** fond + lettres : de loin, les
  mipmaps fondent les lettres dans le fond au lieu de les couper par le seuil alpha 0,45 (`AtlasTextes::materiau`,
  `enseignes.cpp` l. 166). Variante permise si le résultat est le même : seuil plus bas (0,25-0,3) et/ou glyphes plus épais,
  **seulement pour les panneaux du circuit**. Les enseignes de la ville (même atlas) ne doivent pas changer de près.
- RG-08 : une seule CI à la fin ; fusion par confiance si confiance > 50 % (branche synchronisée avec `main`, compilation
  et tests CPU en local, ligne `qa/Check.md`, ✅ dans `TACHES.md`, Journal), puis suivre la CI de `main`.

### Critères d'acceptation ajoutés
- [ ] (ANO-49) `carte-ville-residentiel`, `carte-ville-parc`, `carte-ville-port`, `course-cam0-*`, `course-cam1-*`,
  `vie-terrasse` : trottoir gris continu de la chaussée aux façades, plus de bande d'herbe ; terrasses sur le trottoir.
- [ ] (ANO-51) `carte-circuit-jour`, `carte-circuit-nuit` : mots entiers sur les panneaux lointains ; enseignes de la ville
  toujours nettes de près (`carte-ville-*`, `vie-terrasse`).
- [ ] #76 fusionnée (ou déjà fusionnée) avec sa ligne `qa/Check.md` ; CI de `main` verte après la fusion de WFS-94.

### Zone de code (complète)
- **Autorisé** : `src/world/**` : `plage.cpp`, nouveaux `src/world/plage_*.cpp`, `cartes_banc.cpp`, banc de
  `commerces.cpp` (RG-A1), `ville_quartiers.*` et `ville.cpp` (RG-06, RG-A2 à RG-A5), `enseignes.*` et `circuit.cpp` (RG-07),
  `decor_carte.*` (primitives `villa`, `boat`, `rock`, `guard`…) ; à l'étape 0 seulement, les fichiers de la PR #76.
- `#include "effets_particules.h"` permis pour `effets::versHdr` (RG-A4), sans modifier ce fichier.
- **Interdit** : le reste de `src/render/**` et `shaders/**` (WFS-100, WFS-109) ; `src/game/**`, dont `WALK_W` (accroches de
  WFS-102) ; `src/ui/**` (WFS-102) ; `src/platform/**`, `.github/**` et le routage `WFS_BANC` de `desktop_main.cpp`
  (WFS-106) ; noms et API de `zones.h` (lus par WFS-102 pour le bandeau du quartier) ; fichiers du duel ; tout autre
  travail iOS (gel iOS).
- `decor_carte.cpp` l. 320 : garder l'ordre des indices `{k, k+1, k+3, k+1, k+2, k+3}` posé par WFS-97 (fusionné).

## Reprise du 2026-11-05 (PO) — à lire en premier
Sur demande de l'utilisateur, le ticket passe de Copilot (session 0e5c192d) à **Claude (session trafic, agent reprise)**,
qui le démarre **à la fusion de WFS-109** (un ticket à la fois).
- **État de la branche** `feature/waforspeed-94` : un seul commit, d42c3f0 (attribution, 01:17 UTC), déjà dans `main`.
  **Fait** : étape 0 (PR #76, WFS-88, fusionnée en 1c28ae5, ligne `qa/Check.md` écrite, commit 23a1f00). **Aucun code**
  du ticket n'a été écrit ; aucun fichier de `src/world/` n'a changé depuis. Reprendre la branche :
  `git checkout feature/waforspeed-94 && git merge origin/main`. Ne pas supprimer la branche.
- **Reste à faire** (une seule CI à la fin) : RG-01 à RG-05 (quartiers Corniche, Port de plaisance, Côte rocheuse, Village
  de pêcheurs, primitives `villa`, `boat`, `rock`, `bollard`, `guard`), RG-A1 (ANO-40, captures de la plage), RG-A2 à
  RG-A5 (ville de nuit), RG-06 (ANO-49, trottoir), RG-07 (ANO-51, panneaux du circuit). L'étape 0 est close.
- **Ne pas anticiper WFS-136 à WFS-140** (nouveau tracé, casino, beach parties…) : WFS-94 garde le tracé actuel ; WFS-139
  ajoutera la falaise et les animations de plage après lui.
- Parcours complet (règle 000000) : `WFS_BANC=parcours` à 0 écart avant fusion, preuve au Journal.

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P2 Haute | WFS-94 | Corniche : quartiers Port de plaisance, Côte rocheuse, Village de pêcheurs (villas, bateaux, rochers) + ville de nuit : façades sombres et fenêtres allumées (fin d'ANO-37) ; cadrage ANO-40 ; finitions de WFS-93 : trottoir de la ville jusqu'à RW + 6 m (ANO-49), texte lointain des panneaux du circuit (ANO-51) | E2 | ✅ Fusionné (QA de nuit à faire ; fusion par confiance, PR #123, commit a722e6c) | Claude (session trafic, agent reprise), 2026-11-05 (repris de Copilot sur demande de l'utilisateur) | WFS-93 ✅ | `feature/waforspeed-94` | #123 | ⏳ Recette QA de nuit (`qa/Check.md`) |

Critères et notes de suivi :

**WFS-94 — Corniche : quartiers marina, rochers, village + ville de nuit (fin d'ANO-37) + finitions de WFS-93 (ANO-49, ANO-51)** · E2 · P2 Haute · ✅ Fusionné (QA de nuit à faire), PR #123, commit a722e6c, 2026-11-06, fusion par confiance — Claude (session trafic, agent reprise), 2026-11-05 (repris de Copilot sur demande de l'utilisateur), ex-Copilot (session 0e5c192d), démarre à la fusion de WFS-109 ; branche `feature/waforspeed-94` reprise (seule l'étape 0, PR #76, a été faite ; aucun commit de code sur la branche, d42c3f0 d'attribution : section « Reprise du 2026-11-05 » des règles) (règles : `regles-de-gestion/WFS-94.md` : RG-A2 à RG-A5, puis section « Attribution du 2026-11-05 », RG-06 et RG-07). **Étape 0** : regarder et fusionner la PR #76 (WFS-88, iOS ; confiée par le PO, RG-10 de WFS-103), sauf si elle est déjà fusionnée. Zone : `src/world/**` (`plage.cpp`, nouveaux `src/world/plage_*.cpp`, `cartes_banc.cpp`, banc de `commerces.cpp` (ANO-40), `ville.cpp` (façades et fenêtres de nuit, trottoir), `ville_quartiers.*` (trottoir, ANO-49), `enseignes.*` / `circuit.cpp` (ANO-51), `decor_carte.*` pour les primitives) ; `#include "effets_particules.h"` permis, sans le modifier ; interdit : le reste de `src/render/**`, `shaders/**` (WFS-100, WFS-109), `src/game/**` dont `WALK_W` (accroches de WFS-102), `src/ui/**` (WFS-102), `src/platform/**` et `.github/**` (WFS-106), noms et API de `zones.h`, fichiers du duel, iOS (hormis la fusion de #76)
- [ ] `carte-plage-marina`, `-rochers`, `-village` : bateaux, bornes, villas, rochers, glissières visibles ; ≤ 300 appels de dessin
- [ ] (ajout PO, ANO-40) `vie-portique` à 25 m (« CHECKPOINT 1 » lisible), `vie-plage` à 15 m (serviettes et baigneurs visibles)
- [ ] (ajout PO, ANO-37 fin) `course-nuit-ville-*` : façades sombres jusqu'au fond de rue, même dans l'axe des phares ; environ une fenêtre sur trois allumée (quads émissifs, RG-A4) ; captures de jour inchangées
- [ ] (ANO-49) `carte-ville-residentiel`, `-parc`, `-port`, `course-cam0-*`, `vie-terrasse` : trottoir gris continu de la chaussée jusqu'aux façades (RW + 6 m), plus de bande d'herbe ; terrasses posées sur le trottoir
- [ ] (ANO-51) `carte-circuit-jour`, `carte-circuit-nuit` : mots entiers sur les panneaux lointains (plus de lettres en morceaux) ; enseignes de la ville toujours nettes de près
- [ ] #76 fusionnée (ou déjà fusionnée) avec sa ligne `qa/Check.md` ; tests CPU verts
