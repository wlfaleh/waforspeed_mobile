Agent : Claude, agent dev3d (01UY1Vv49f)

**Ticket** : WFS-594 — règles : `regles-de-gestion/WFS-594.md` (épic E9 + sols, P1)

## Description

- **Demande de l'utilisateur** (2026-11-22) : améliorer le graphisme, partie « routes et sols » : vue depuis la voiture, la route et le sol autour doivent ressembler à une photo (grain et usure de l'enrobé, marquages un peu effacés, vraies bordures et trottoirs, herbe, terre et sable qui ne sont plus des aplats).
- **Ce qui change** :
  - `shaders/route.glsl` (E9) :
    - **RG-02 enrobé** : nouveaux défauts `defautsChaussee` : fissures transversales de retrait tous les 8 à 25 m (une sur deux pontée : bande de bitume coulé de 4 à 8 cm, noire et brillante), joint de répandage ponté le long des lignes de voie, rustines découpées à la scie (0,4 à 1,2 m, coins arrondis, joint brillant), gouttes d'huile au milieu des voies. Densité par carte : Ville > Campagne > Corniche, rien au Grand Prix. Tout ce qui est plus fin que le pixel s'efface au prorata (pas de scintillement). Grain, usure des bandes de roulement, raccords et pavage sans répétition de WFS-205 / 206 gardés ; filtrage anisotrope déjà ×8 (Moyenne, Haute, Ultra) et ×4 (Basse) depuis WFS-144 : inchangé.
    - **RG-03 marquages** : lignes de 15 cm (`LARGEUR_MARQUAGE_M`, IISR 2u), bords anticrénelés au pixel et irréguliers (± 6 mm), peinture blanche d'albédo 0,68 (avant : 0,78, hors de la plage 0,6 à 0,75), arrachée par plaques (chaque tiret son âge, plus d'usure dans les bandes de roulement), poussière par endroits. **Nuit** : la peinture garde la rétroréflexion des phares du joueur (`retroChaussee`, même facteur que l'enrobé au lieu de 1) : la ligne reste ≈ 8 fois plus lumineuse que l'enrobé (EN 1436) ; mouillée : 30 % ; sous la neige : rien.
    - **RG-04 bordures et trottoirs** : bordure T2 sur toutes les cartes (granit en Ville, béton vibré ailleurs, bulles d'air), joints au mortier tous les 1 m (`JOINT_BORDURE_M`), ton propre à chaque bloc, arête du chanfrein polie et éclatée par endroits, face verticale texturée sans étirement ; trottoir : taches et coulures, chewing-gums écrasés en Ville. Géométrie inchangée (chanfrein de 2 cm de `game.cpp` / `ville.cpp`).
  - `shaders/sols.glsl` (sols) — **RG-05** : touffes d'herbe (15 à 30 cm) visibles jusqu'à ≈ 30 m et brins de près, gravillons et cailloux de la terre et des talus, grain de la roche, relief par dérivées écran (Mikkelsen 2010) ; **sable** de la Corniche (couleur mesurée de la palette gardée, rides de vent par l'ombrage, creux des pas, bosses, grain, coquillages et débris rares ; sable mouillé plus lisse) ; **neige fraîche** au sol de la carte d'hiver (albédo ≈ 0,8, congères, crêtes de vent, creux bleutés) au lieu de bandes blanches unies ; bande de terre et de gravier (30 à 80 cm) au pied du trottoir du sol de Game. Chaque échelle de bruit s'efface avant d'être plus fine que 4 pixels : ni moiré, ni points.
  - Textures **faites par le code** (RG-05) : aucune texture libre atteignable (Poly Haven et ambientCG bloqués par `regles-reseau.md`, on ne contourne pas) ; aucun fichier téléchargé.
  - `src/world/sols.*` : options du sol (16 sable, 32 neige, 64 bord de route), `neigeCarte`, carte enneigée → sol naturel.
  - Accroches annoncées au journal (≤ 10 lignes chacune) : `shaders/eclairage.glsl` (rétroréflexion de la peinture), `shaders/scene.frag` (normale passée au sol), `src/game/game.cpp`, `src/world/ville.cpp` (le sol de la Ville passe au sol texturé de Game), `src/world/plage.cpp` (partie « sable » : sable sec, mouillé et promenade), `src/world/decor_carte.*` (neige au sol), `src/render/bench_realisme.h` (mesure des lignes relue), `src/platform/desktop_main.cpp` (banc).
  - Nouveau banc `routes-sols` (`src/render/route_sols_banc.cpp`) : Ville, Corniche, Neige, jour et nuit ; vues arrière, cockpit, rasante (2 m), bas-côté de près, et Ville en Basse.
- **Comment c'est vérifié** :
  - Tests CPU : 159 réussis sur 159 (`bash tools/qa/tests_cpu.sh build`), dont `test_fidelite_routes` (26 / 26 mesures : largeur 15 cm, albédo 0,68, joints 1 m, pontage 0,55 ; constantes du shader = C++), `test_sols` (codes, constantes `sols.glsl` = C++, carte enneigée), `test_fidelite` (mesure des lignes relue).
  - Banc `routes-sols` en 1280 × 720 (lavapipe, rendu de la CI), avant (`main`) / après aux mêmes endroits, **avec les couches de validation Vulkan : 0 erreur** avant et après ; chaque image regardée, zooms ×2 dans la planche de détails.
  - Coût (RG-07) : banc `perf` (lavapipe, 1280 × 720, 120 images par carte), passe de scène en ms, `main` → branche (2 mesures) :
    | Carte | Haute | Basse |
    |---|---|---|
    | Ville | 601 → 673 / 692 (+12 à +15 %) ; rapportée à la passe d'ombres (non modifiée, elle-même +5 à +11 % d'un run à l'autre) : +4 à +6 % | 322 → 314 / 325 (stable) |
    | Corniche | 412 → 421 / 424 (+2 à +3 %) | 219 → 210 / 220 (stable) |
    | Campagne | 417 → 407 / 413 (stable) | 242 → 244 / 241 (stable) |
    | Grand Prix | 491 → 475 / 470 (stable) | 226 → 230 / 232 (stable) |
    La hausse de la Ville en Haute vient du sol de la Ville, qui passe du bruit simple de `surfaceSol` au sol texturé : cailloux et roche ne sont plus calculés sur le pré plat (dernier commit). Basse : aucune hausse (RG-07). Le GPU logiciel est très bruité : la mesure sur téléphone reste à confirmer (banc `perf` de la CI de `main`).
- **Limites et risques connus** :
  - Textures par le code, pas de scans : de près, l'herbe reste un gazon sans brins en 3D (pas de géométrie d'herbe : hors ticket).
  - Neige : l'écart avec avant est faible de jour (neige claire, l'image sature) ; le relief se voit la nuit.
  - La bande de gravier ne se voit sur aucune carte jouable : la Ville a ses trottoirs plus larges, la Corniche et la Neige dessinent leurs propres bas-côtés (elle sert au sol de Game, Campagne et Grand Prix en pause). Le reste de la Corniche (promenade, sable) est texturé.
  - Coût mesuré sur lavapipe (GPU logiciel), pas sur téléphone : à confirmer par le banc `perf` de la CI de `main` et par l'utilisateur (🟡).
  - Hors périmètre (RG-09) : dents de scie du bord (WFS-577), flaques (WFS-157), reflets de nuit sur route mouillée (WFS-459), menus. Paquets des cartes à republier : WFS-595 (`@cicd`).

## Captures pour le lead dev (17 après, 16 paires + planche de détails)

Dossier : [`qa/pr/WFS-594`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594) — banc `routes-sols`, 1280 × 720.

Captures « avant » : `origin/main` = `039c251177cc0be1d98aa76f3f36547eb7f239c6`, compilé avec le banc `routes-sols` ajouté (le banc n'existe pas sur `main` : même code que `main` pour tout le reste, mêmes endroits, même graine).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `routes-sols-corniche-jour-arriere` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-corniche-jour-arriere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-corniche-jour-arriere.jpg?raw=true) |
| `routes-sols-corniche-jour-bascote` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-corniche-jour-bascote.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-corniche-jour-bascote.jpg?raw=true) |
| `routes-sols-corniche-jour-rasante-gauche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-corniche-jour-rasante-gauche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-corniche-jour-rasante-gauche.jpg?raw=true) |
| `routes-sols-corniche-jour-rasante` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-corniche-jour-rasante.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-corniche-jour-rasante.jpg?raw=true) |
| `routes-sols-corniche-nuit-rasante` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-corniche-nuit-rasante.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-corniche-nuit-rasante.jpg?raw=true) |
| `routes-sols-neige-jour-arriere` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-neige-jour-arriere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-neige-jour-arriere.jpg?raw=true) |
| `routes-sols-neige-jour-bascote` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-neige-jour-bascote.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-neige-jour-bascote.jpg?raw=true) |
| `routes-sols-neige-nuit-arriere` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-neige-nuit-arriere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-neige-nuit-arriere.jpg?raw=true) |
| `routes-sols-planche-details` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-planche-details.jpg?raw=true) |
| `routes-sols-ville-jour-arriere` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-ville-jour-arriere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-ville-jour-arriere.jpg?raw=true) |
| `routes-sols-ville-jour-bascote` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-ville-jour-bascote.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-ville-jour-bascote.jpg?raw=true) |
| `routes-sols-ville-jour-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-ville-jour-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-ville-jour-basse.jpg?raw=true) |
| `routes-sols-ville-jour-cockpit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-ville-jour-cockpit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-ville-jour-cockpit.jpg?raw=true) |
| `routes-sols-ville-jour-rasante` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-ville-jour-rasante.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-ville-jour-rasante.jpg?raw=true) |
| `routes-sols-ville-nuit-arriere` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-ville-nuit-arriere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-ville-nuit-arriere.jpg?raw=true) |
| `routes-sols-ville-nuit-cockpit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-ville-nuit-cockpit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-ville-nuit-cockpit.jpg?raw=true) |
| `routes-sols-ville-nuit-rasante` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/avant-routes-sols-ville-nuit-rasante.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-594/qa/pr/WFS-594/apres-routes-sols-ville-nuit-rasante.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv49fLTvKRexaJUt2wy
