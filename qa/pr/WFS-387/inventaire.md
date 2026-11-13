# WFS-387 — Inventaire du décor de la Campagne (RG-02, périmètre du découpage du 2026-11-13)

Relevé sur `main` (2898877f) dans `src/world/campagne.cpp` (`dessine()`, `place()`, `placerCol()`) et `decor_carte.cpp`.
Les nombres d'objets sur le tour sont ceux de `qa/pr/WFS-372/inventaire.md` (même graine).

États possibles :
- **forme nue** : boîte, cylindre, cône ou plan posé tel quel (interdit, règle du 2026-11-09 19:41 UTC) ;
- **stylisé** : modélisé, mais avec des proportions ou une matière simplifiées ;
- **teinte unie** : une seule couleur de la palette, sans texture ;
- **bon** : vrai modèle ou objet modélisé au niveau du vrai objet, à revoir seulement pour les textures PBR (WFS-521).

Les photos réelles sourcées ne sont pas demandées dans ce ticket : elles sont passées à WFS-521, en attente de l'ouverture d'un site
par l'utilisateur. Le réseau autorisé n'ouvre que GitHub (dépôts publics) et `sketchfab.com`. Ce site n'est inscrit dans
`regles-reseau.md` que pour les voitures : son usage pour le décor est à faire confirmer par le PO dans WFS-518 à 520.

## 1. Objets traités par WFS-387 (réserves de WFS-372, RG-11)

| Partie | Code | État avant | État après WFS-387 | Ticket suivant |
|---|---|---|---|---|
| Bottes rondes | `T_BALES`, `campagne::botte` | stylisé : flanc lisse d'une seule teinte | flanc à fibres de paille (luminosité ± 14 %, touffes), filet blanc sur les épaules, bandes des brins, bouts à brins coupés | WFS-521 (texture PBR de paille) |
| Corps de ferme | `T_FARM`, `campagne::ferme` | teinte unie : enduit gris-bleu froid | enduit ocre clair chaud (0xdcbd8d, albédo ≈ 0,53), taches et grain (± 10 % et ± 7 %), pied de mur plus sombre, tuiles de cuissons variées | WFS-521 |
| Hangar agricole | `T_FARM` (`k = 1`), `campagne::hangar` | ondes de la tôle invisibles à moyenne distance | creux plus sombres que les crêtes, recouvrement des plaques tous les 1,06 m, teinte propre à chaque plaque ; version « loin » en bandes de plaques | WFS-521 |
| Clôture des champs | `T_FENCE`, `poteauCloture`, `lisse` | poteaux presque noirs, lisses brunes | même bois gris-brun (0x9c8466) pour les poteaux et les lisses, veines dans le fil ; chaque lisse finit sur un poteau posé | WFS-519 (enclos, haies) |
| Poteau de ligne électrique | `T_POLE`, `poteauLigne` | bois trop sombre (0x56463a) | bois traité vieilli gris-brun (0x8d7d68), gerces, pied plus sombre ; chaque câble finit sur un poteau posé | — |
| Parapet des épingles du col | `T_MURET` | **forme nue** (boîte grise de 0,4 × 0,5 m), choc au même endroit | parapet de moellons (une teinte par pierre, joints creux), chaperon arrondi ; boîte de choc tirée des mêmes mesures (`chocMuret`) | — |
| Sol au-delà de la bande de 220 m | `construireSolFond`, bandes de `profil()` | plan uni (`C_FARHILL`) 1 m sous le point le plus bas de la route | voir la PR (RG-10) | WFS-520 (collines) |

## 2. Parties renvoyées aux tickets suivants

| Partie | Code | État | Ticket |
|---|---|---|---|
| Maisons du village | `T_VILLA` → kit d'architecture (WFS-161) | bon (kit commun), textures PBR à venir | WFS-518, WFS-521 |
| Église | `T_CHURCH` : `box` + clocher `box` + flèche `cone` | **forme nue** | WFS-518 |
| Commerces du village | `T_COMMERCE` : `box`, store, tables `cyl` | **forme nue** | WFS-518 |
| Fontaine du carrefour | `T_FONTAINE` : 4 `cyl` | **forme nue** | WFS-518 |
| Feux tricolores, passages piétons, cédez-le-passage | `T_FEU`, `T_ZEBRA`, `T_CEDEZ` | **forme nue** (feux) ; marquage plat (normal) | WFS-518 |
| Panneaux de forêt, A1 et d'autoroute | `T_SIGN`, `T_DANGER`, `T_PANNEAU_A` | **forme nue** (`box`, `cyl`) | WFS-518 (signalisation) |
| Balises J6 | `T_J6` : 2 `box` + catadioptres | **forme nue** | WFS-518 (signalisation) |
| Champs (blé, colza, labours) | bandes de sol (`profil`, `fieldCol`) | teinte unie par champ | WFS-519 |
| Rangs de vigne | `T_VINES`, `piquetVigne`, `cep` | bon (WFS-372) ; feuillage à revoir | WFS-519 |
| Haies | `T_HEDGE` : masse `box` + cartes de feuilles | **forme nue** (masse visible entre les feuilles) | WFS-519 |
| Arbres (chênes, pins, cyprès) | `arbre()` commun (WFS-34) | stylisé (cartes de feuilles) | WFS-519 |
| Fossés et bas-côtés | bandes `C_VERGE`, `C_DITCH`, touffes | teinte unie + touffes | WFS-519 |
| Enclos des chevaux | `T_ENCLOS` | à vérifier (poteaux et lisses) | WFS-519 |
| Collines et sommets du col | `placerSommets`, `sommetsMesh_` | **forme nue** (pyramides) | WFS-520 |
| Éoliennes | `T_TURBINE` : mât `cyl`, nacelle `box`, pales `box` | **forme nue** | WFS-520 |
| Autoroute : séparateur, glissières, péage, panneaux | `T_AUTO`, `T_GLISS`, `T_PEAGE` | séparateur et piliers en **boîtes** | WFS-520 |
| Cascade du pont | `T_CASCADE` | **forme nue** (blocs gris) | WFS-520 |
| Animaux (vaches, moutons, chevaux) | `.wfa` (WFS-186) | bon (vrais modèles CC0) | — |
| Textures PBR haute définition, photos de référence | tout le décor | teintes de la palette seulement | WFS-521 (attend l'ouverture d'un site CC0) |

## 3. Références disponibles par le réseau autorisé
- **GitHub (dépôts publics)** : méthode de `.claude/skills/waforspeed-natif/references/modeles-3d-github.md`. Les packs CC0 courants
  (Quaternius, Kenney) sont **stylisés** : ils ne tiennent pas 99 % de réalisme. Ils peuvent servir de base de forme, jamais
  d'aspect final.
- **Mesures réelles** : celles des objets de WFS-372 et de WFS-387 sont écrites en tête de `src/world/decor_campagne_modeles.h`
  (sources publiques : guides de la viticulture, fiches des presses à balles rondes, plaques ondulées de 177 × 51 mm, poteaux HTA
  en bois, parapets de montagne de 0,45 m et plus).
