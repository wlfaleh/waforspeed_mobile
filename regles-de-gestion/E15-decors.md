# E15 — Décors : amélioration graphique (99 % de réalisme, 200 à 700 Mo par décor)

Créée par le PO le 2026-11-09 (demande de l'utilisateur, `@po`) : « créer une épic spécifique pour l'amélioration graphique des décors. Un décor doit avoir minimum 200 Mo et maximum 700 Mo. Le décor doit être réaliste à 99 %. Pour cette épic spécifiquement, un agent 3D doit la valider avant de merger : il doit regarder les captures des PR et laisser un commentaire pour que le lead dev puisse regarder et merger. Tu commences cette épic le plus tôt possible, créer des US pour chaque décor. »

## Règles de l'épic (valables pour chaque US de E15)
- **RG-E15-01 — Taille du décor : 200 Mo minimum, 700 Mo maximum.** « Décor » = tout ce qui entoure la route d'une carte (terrain et sols, bâtiments, mobilier, végétation, eau, objets, textures et matériaux du décor), **mesuré sur disque dans les données de la carte** (paquet `.wfcarte` cuit par l'outil de cuisson de E14, partie décor ; tant que le paquet de la carte n'est pas publié, mesure de la même cuisson en local). La taille est écrite dans la PR et au journal (Mo, avec la commande de mesure). Moins de 200 Mo : le décor n'est pas assez riche → refus ; plus de 700 Mo : à alléger → refus. Les routes (E9), voitures (E10) et personnes (E11) ne comptent pas. *(Cette règle de l'utilisateur remplace, pour E15, la consigne « pas de seuil en Mo dans une US » du 2026-11-07 ; la règle « tout préparé au chargement, jamais de plantage par manque de mémoire, repli » reste.)*
- **RG-E15-02 — 99 % de réalisme** (`regles-de-gestion/DA-realisme.md`) : chaque partie du décor est comparée à des références réelles (photos sourcées dans `references/reel/decor/<carte>/`), mesures ±1 %, matériaux PBR justes, jour **et** nuit.
- **RG-E15-03 — Formes géométriques interdites** (règle du 2026-11-09 19:41 UTC) : aucun cube, boîte, triangle plat, disque, cylindre, cône ou boule nus ; vrais modèles 3D libres **CC0 / CC BY** (licences dans `assets/licences/`, téléchargements au journal selon `regles-reseau.md`), sans marque, ou objets modélisés au niveau du vrai objet.
- **RG-E15-04 — Le téléphone tient** : tout est préparé au chargement, rien n'est créé en course ; instances et LOD ; aucune baisse d'images par seconde en Basse (banc perf avant / après dans la PR).
- **RG-E15-05 — Réutiliser** le code et les actifs du dépôt (`src/world/archi_*`, `mobilier_*`, `vegetation.*`, `decor_carte.*`, `assets/world/modeles/**`, outils `tools/` de conversion, cuisson E14) ; ne rien recréer qui existe.
- **RG-E15-06 — VALIDATION PAR UN AGENT 3D AVANT LA FUSION (règle de l'utilisateur, propre à E15)** :
  1. le développeur ouvre sa PR avec ses captures (`bash tools/qa/captures_pr.sh`, au moins 3, avant / après, jour et nuit) et la taille du décor ;
  2. il appelle **l'agent 3D** (outil Agent, `subagent_type: expert-realisme-3d`, alias `@3d`) avec le numéro de PR et le ticket ;
  3. l'agent 3D ouvre **chaque capture** de `qa/pr/WFS-XX/`, les compare aux références réelles et laisse **un commentaire dans la PR** qui commence par **« Avis agent 3D : VALIDÉ »** ou **« Avis agent 3D : À REPRENDRE »**, avec la liste des défauts capture par capture (pied de page Claude Code) ;
  4. « À REPRENDRE » : le développeur corrige, régénère les captures et rappelle l'agent 3D ; « VALIDÉ » : il appelle le **lead dev** ;
  5. le **lead dev** lit l'avis de l'agent 3D, regarde les captures et le code, puis fusionne (squash) ou refuse. **Sans commentaire « Avis agent 3D : VALIDÉ » sur le dernier commit, le lead dev (y compris le passage automatique de chaque heure) ne fusionne pas une PR de E15.**
- **RG-E15-07 — Agent** : `dev-artiste-3d` (`@devartiste`, `@dev3d`), qui charge aussi `expert-cartes` (décor d'une carte) ; un ticket par session, jusqu'à la fusion.
- **RG-E15-08 — Parallélisme** : **un ticket actif par décor** (une carte = une file), pour avancer sur plusieurs décors en même temps. Fichiers propres à chaque carte (`ville*.cpp`, `plage.cpp`, `campagne.cpp`, `circuit.cpp`, `paquets/sources/<carte>/**`, nouveaux fichiers `src/world/decor_<carte>_*`, `assets/world/modeles/<carte>/**`, `assets/textures/decor/<carte>/**`) ; fichiers communs (`archi_kit.*`, `vegetation.*`, `mobilier_*` communs, shaders `decor*`) : seulement des ajouts ≤ 10 lignes annoncés au journal, ou de nouveaux fichiers.
- **RG-E15-09 — Petit défaut** : un travail presque bon n'est pas jeté (ordre du 2026-11-09) ; l'agent 3D peut écrire « VALIDÉ avec réserves » : fusion, et le PO crée une tâche séparée.

## US de l'épic (une file par décor)
| Décor | US, dans l'ordre |
|---|---|
| Ville | WFS-369 mobilier → WFS-370 immeubles → **WFS-385** décor complet 200 à 700 Mo |
| Corniche | (WFS-379, 376, 380 en cours / prêts, hors E15) → WFS-371 plage et abords → **WFS-386** décor complet |
| Campagne | WFS-372 vignes, village, clôtures → **WFS-387** décor complet |
| Grand Prix | WFS-373 tribunes, stands, barrières → **WFS-388** décor complet |
| Neige scandinave (Noël) | (WFS-344 contenu, E14) → **WFS-389** décor complet |

Zone de code de l'épic : celle de E12 et la partie décor de E2 / E14 (voir RG-E15-08).
