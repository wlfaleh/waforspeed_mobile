# E16 — Évolution des objets 3D (bibliothèque commune d'objets en vrais fichiers)

Créée par le PO le 2026-11-11 sur ordre de l'utilisateur : « Ok créer les US, mets-les dans une épic évolution des objets 3D, mais tu les mets en bloqué, c'est pas une priorité, je te dis à quel moment on va le commencer, tu le mets en dernier et avec statut bloqué par l'utilisateur. »

> ⛔ **ÉPIC BLOQUÉE PAR L'UTILISATEUR (2026-11-11)** : toutes ses US (WFS-430 à WFS-443) sont **« ⛔ Bloqué par l'utilisateur »**, priorité la plus basse (P4 Basse), placées en dernier dans `TACHES.md`. Elles ne sont dans **aucun ordre de priorité** (ni l'ordre des 20, ni celui des 50). **Aucune n'est attribuée, aucune ne se débloque** sans un **ordre écrit de l'utilisateur** qui dira quand commencer.

Inventaire de départ (≈ 241 lignes vérifiées dans le code, actions A convertir / B vrai modèle / C garder / D texture) : `regles-de-gestion/E16-inventaire.md`.

## But
- Une **bibliothèque commune d'objets 3D en vrais fichiers** (modèles libres CC0 / CC BY ou objets du code convertis puis retouchés), aux vraies mesures (±1 %), **réutilisés par toutes les cartes** : un objet (feu tricolore, lampadaire, palmier, table de terrasse…) n'est fait **qu'une fois**.
- **Le code ne garde que ce qui suit la route ou le terrain** mètre par mètre (chaussée, trottoirs, bordures, glissières en continu, talus), les effets animés et les bâtiments générés par règles (action C de l'inventaire).
- Applique la règle « formes géométriques interdites » (2026-11-09 19:41 UTC) et `regles-de-gestion/DA-realisme.md` (99 %).

## Zone de code
- `src/world/biblio_*` (lecteur commun, copies / instances, LOD, secours) ;
- `assets/world/biblio/**` (modèles, textures, catalogue) ;
- `tools/biblio/**` (outil de conversion, script de contrôle du catalogue) ;
- `tests/biblio*` ;
- accroche dans le renderer **≤ 10 lignes**, annoncée au journal.
- Le branchement dans les cartes (WFS-441) et le code des routes (WFS-443) touchent les fichiers des cartes : ticket confié à un `expert-cartes`, un seul ticket actif à la fois sur une même carte avec E15.

## Règles de l'épic (valables pour chaque US de E16)
- **RG-E16-01 — Porte de validation de l'utilisateur** : après la **preuve du pin d'Alep sur la Corniche (WFS-435)**, tout s'arrête. Les lots suivants (WFS-436 à 443) ne commencent **qu'après le « oui » écrit de l'utilisateur** sur les captures et sur la mesure de WFS-435.
- **RG-E16-02 — 33 ms sur le S21 FE** : aucune fusion si la scène de la carte touchée dépasse **≈ 33 ms** sur le Samsung S21 FE (même règle que WFS-383 / 371) ; mesure avant / après dans chaque PR qui pose des objets dans une carte.
- **RG-E16-03 — Avis agent 3D** : chaque PR qui ajoute ou remplace un objet visible appelle l'agent 3D (`subagent_type: expert-realisme-3d`, `@3d`) ; le lead dev ne fusionne qu'avec **« Avis agent 3D : VALIDÉ »** sur le dernier commit (comme E15).
- **RG-E16-04 — Licences** : seulement CC0 / CC BY (« gratuit » ne suffit pas, NC interdit) ; chaque fichier a sa source, sa licence, son auteur et son SHA-256 dans le catalogue ; avis du juriste (WFS-431) avant le premier modèle importé ; téléchargements selon `regles-reseau.md`.
- **RG-E16-05 — Formes géométriques interdites** : aucun cube, boîte, disque, cylindre, cône ou boule nus visibles ; objets aux vraies mesures ±1 %, sans marque.
- **RG-E16-06 — Tout au chargement** : objets lus, copiés et mis en LOD au chargement de la carte ; rien n'est créé en course ; **secours** propre (objet du code d'aujourd'hui) si un fichier manque ou est illisible, jamais de plantage.
- **RG-E16-07 — Captures** : au moins 3 par PR, avant (main) / après (branche), jour et nuit, à l'endroit exact de l'objet (`bash tools/qa/captures_pr.sh`).
- **RG-E16-08 — Un ticket = une journée au plus** ; chiffrage en sessions de développeur.

## Avis des agents (repris dans les US)
- **Lead dev** : prouver le socle (lecteur, conversion, catalogue) **sur un seul objet** avant de multiplier ; les **routes à part** (WFS-443, ticket technique distinct, jamais mélangé aux objets) ; **ne pas toucher maintenant aux objets déjà bien faits par le code** (ils restent tant qu'ils ne sont pas une forme nue signalée).
- **Smart brain** : avancer **lot par lot**, avec un **contrôle de performance à chaque lot** (RG-E16-02) ; on n'ouvre pas le lot suivant si le précédent coûte trop.

## Lot 1 (créé en tickets, tous ⛔ Bloqué par l'utilisateur)
| US | Sujet | Qui | Sessions |
|---|---|---|---|
| WFS-430 | Cadrage (liste des objets du lot 1, mesures, références) | PO + `@3d` | 0,5 |
| WFS-431 | Licences (règle, crédits, sources autorisées) | `@juriste` | 0,25 |
| WFS-432 | [TECHNIQUE] Socle : lecteur commun, copies, LOD, secours | `@dev` | 1,25 |
| WFS-433 | [TECHNIQUE] Outil de conversion vers la bibliothèque | `@devartiste` | 1,25 |
| WFS-434 | [TECHNIQUE] Catalogue + règle de revue + script de contrôle | `@dev` + lead dev | 0,5 |
| WFS-435 | Preuve : pin d'Alep sur la Corniche, **puis PORTE utilisateur** | `@devartiste` | 1,5 |
| WFS-436 | Sols en textures photo | `@devartiste` | 1 |
| WFS-437 | Palmier, olivier | `@devartiste` | 1,5 |
| WFS-438 | Rochers | `@devartiste` | 1 |
| WFS-439 | Lampadaire, feu tricolore, poteau de glissière | `@devartiste` | 1,5 |
| WFS-440 | Parasol, transat, table, chaise | `@devartiste` | 1,5 |
| WFS-441 | Branchement Ville / Campagne / Grand Prix, doublons retirés | `@cartes` | 1,5 |
| WFS-442 | Mesure finale, QA, émulateur | `@dev` + QA | 0,5 |
| WFS-443 | [TECHNIQUE] Code des routes mis en commun, Corniche puis Ville | `@cartes` | 2,5 |
| **Total lot 1** | | | **≈ 16** (16,25) |

## Lots 2 à 10 — PRÉVISION, NON CRÉÉE EN TICKETS
Ces lots ne seront découpés en US qu'après le lot 1, et seulement sur ordre de l'utilisateur. Nombres indicatifs, à recalculer au cadrage de chaque lot.
| Lot | Contenu (d'après l'inventaire) | Objets ≈ | Sessions ≈ |
|---|---|---|---|
| 2 | Végétation restante : feuillu, platane, sapin, pins, cyprès, sous-bois, herbe, haie, vigne | 12 | 6 |
| 3 | Mobilier urbain de la Ville : bancs, poubelles, abribus, fontaine, stores, bornes, panneaux | 14 | 7 |
| 4 | Corniche bord de mer : bateaux, bouées, pontons, garde-corps, mobilier de plage restant | 14 | 7 |
| 5 | Campagne et villages : clôtures, ballots, matériel agricole, mobilier de village | 12 | 6 |
| 6 | Grand Prix : barrières, piles de pneus, gradins, stands, nacelles | 12 | 6 |
| 7 | Chantier, police, course : cônes, panneau AK5, ralentisseur, rampe de gyrophares, portique, casque, poussette | 8 | 4 |
| 8 | Habitacle commun : pièces du cockpit, rétroviseur, essuie-glaces, volant (sans toucher les intérieurs déjà bien faits) | 12 | 7,5 |
| 9 | Garage 3D : sol, plateau tournant, néons | 4 | 2,5 |
| 10 | Textures photo restantes (façades, surfaces) + mesure finale de toutes les cartes | 17 | 8,5 |
| **Total lots 2 à 10** | | **≈ 105** | **54,5** |

**Grand total de l'épic** : lot 1 ≈ 16 + lots 2 à 10 ≈ 54,5 = **≈ 70,5 sessions**.

## Liens avec les tickets existants (rien changé le 2026-11-11)
- **WFS-383 et WFS-371 ne sont pas modifiés** tant que l'épic est bloquée.
- **Au déblocage** : si **WFS-371** n'est pas déjà fait, il serait **réduit** à la pose des objets du catalogue sur la Corniche + le bar de plage et les cabines ; le PO réécrira alors ses règles.
- Les décors de E15 gardent leurs règles (700 Mo maximum, avis agent 3D) ; E16 leur fournit des objets communs.
