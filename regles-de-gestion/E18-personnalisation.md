# Épic E18 — Personnalisation de l'aspect des voitures

Créée le 2026-11-12 par le PO, sur demande de l'utilisateur (`@po`, 2026-11-12 22:17 UTC). Elle se place dans l'ordre déjà demandé :
**identité (E17) → score (étoiles) → voitures et couleurs → cadeau → market**. Les teintes et coloris vont avec l'étape « voitures et couleurs gratuites ».
Monnaie : **étoiles ★** (R13, option a validée par l'utilisateur : une seule monnaie). Proposition de départ : `recommandations-smart-brain.md` R15.

## Demande de l'utilisateur (résumé fidèle)
Seulement **teintes, coloris, finitions, skins et dessins**. **Aucun accessoire** : pas de jantes, néons, vitres, étriers ni mode photo.
1. **Teintes et coloris** : 16 couleurs actuelles + 12 vives (bleu électrique, vert acide, orange fluo, rose, jaune citron, violet néon…) ;
   couleur secondaire (toit et rétroviseurs) ; 8 couleurs gratuites, les autres 50 ★.
2. **Finitions** (épic E10, `@dev3d`) : brillant (gratuit), métallisé, nacré, mat, satiné, chromé, caméléon (couleur selon l'angle), candy ;
   100 à 400 ★ (chromé et caméléon les plus chers) ; rendu de vraie peinture automobile (`DA-realisme.md`).
3. **Skins** (habillages complets) : 1er lot de 4 (Rallye, Drift Tokyo, Néon nuit, Camouflage) sur 6 voitures (208, 500, GT86 + 3 choisies par le PO) ;
   1 skin offerte par voiture débloquée ; les autres 300 à 800 ★ ; certaines seulement dans le cadeau du jour ou les succès.
4. **Dessins** (atelier d'autocollants, après les skins) : bandes, numéros, flammes, éclairs, étoiles, lettres ; placer, tourner, agrandir, colorer,
   20 calques ; texte libre filtré (mots interdits) ; 10 gratuits, packs 150 à 300 ★ ; vrais adhésifs qui épousent la carrosserie (pas de bande plate
   qui dépasse, ANO-250) ; « Mes créations » : 5 habillages sauvegardés par voiture.

**Ordre** : teintes et coloris → finitions → skins → dessins.

## Règles communes à toute l'épic (de l'utilisateur, à rappeler dans chaque PR)
- RC-E18-01 **Aucun logo, aucun sponsor réel, aucune copie d'une vraie écurie** (ni livrée connue, ni nom, ni numéro associé à un pilote réel).
- RC-E18-02 **Mémoire** : **une seule texture d'habillage chargée en course** (celle de la voiture du joueur), textures **compressées** (ETC2 / ASTC,
  comme les autres textures du jeu) ; rien n'est créé en course (tout se prépare au chargement).
- RC-E18-03 **Aucune perte d'images/s en qualité Basse** : banc d'endurance avant / après, écart ≤ 1 img/s (moyenne et 1 % les plus lents).
- RC-E18-04 **Tout est gardé dans la sauvegarde du joueur** (`src/game/sauvegarde.*`, format `clé=valeur`) : couleurs, finitions, skins,
  autocollants, créations, objets possédés ; une sauvegarde d'avant se lit sans erreur ; aucune clé ancienne supprimée (les `paint-<voiture>` restent valables).
- RC-E18-05 **Captures** : avant / après **dans le garage et en course**, **de jour et de nuit**, sur **au moins 3 voitures** (`tools/qa/captures_pr.sh`).
- RC-E18-06 **Monde 3D** : vraie peinture et vrais adhésifs (`DA-realisme.md`, règle « pas de forme nue ») ; l'interface 2D (garage, atelier) suit `design-street-racing`.
- RC-E18-07 **Étoiles** : les prix sont des **données** dans **`configgame.json`** (section `boutique`, WFS-498 ; ordre de l'utilisateur du 2026-11-12 : réglables sans recompiler), jamais écrits dans le code ;
  l'achat débite le portefeuille d'étoiles de l'étape « score » ; un objet non possédé s'essaie au garage (aperçu) mais ne part pas en course.
- RC-E18-08 **Réutiliser** : `PEINTURES` (`sauvegarde.h`) et `PAINTS` (`garage.cpp`) — une seule liste à la fin (WFS-487) ; `vernisPeinture`
  (`shaders/vehicule.glsl`) ; liste des mots interdits de WFS-480 (`assets/profil/mots_interdits.txt`).
- RC-E18-09 **Parcours complet** testé (menu → garage → choix → course → retour garage → relance du jeu : choix gardé), noté au journal avant la PR.
- RC-E18-11 **Prix (ordre de l'utilisateur, 2026-11-12, R16)** : **prix de base × coefficient de la classe de la voiture** (gratuites et classe 1 ×1, classe 2 ×1,5, classe 3 ×2, classe 4 ×3, classe 5 ×4) ;
  une peinture, une finition ou une skin achetée vaut pour **une seule voiture** ; les packs de dessins valent pour toutes (sans coefficient). Chaque prix affiché au garage
  montre aussi l'**équivalent en médailles** (packs du market : bronze 100 ★, argent 300 ★, or 600 ★) et **en euros** (WFS-498 RG-02). Captures du garage **en français et en anglais**.
- RC-E18-10 Voitures : seulement des modèles libres (CC BY / CC0). **Exclues** : A 45, Elantra N, X5 (licence NC, ordre permanent du 2026-11-10).

## Prix (ordre de l'utilisateur du 2026-11-12, remplace les prix choisis par le PO le même jour)
Voitures, classes et coefficients : `regles-de-gestion/WFS-498.md` RG-04. Prix de base (× coefficient) : couleur vive ou secondaire 50 ★ ; métallisé 100 ★ ; nacré, mat, satiné 150 ★ ;
candy 250 ★ ; chromé, caméléon 400 ★ ; brillant gratuit (8 couleurs) ; skins 300 à 800 ★ ; packs de dessins 150 à 300 ★ (sans coefficient).

## Choix du PO (dans la grille de l'utilisateur)
- **3 voitures en plus pour les skins** : **Golf** (voiture de départ du jeu), **Corvette C7** (id `mustang`), **Giulia Quadrifoglio** :
  trois silhouettes différentes (compacte, coupé, berline), modèles CC BY 4.0. Avec 208, 500 et GT86 : 6 voitures.
- **Skins (prix de base, × coefficient)** : Rallye 300 ★ ; Camouflage 450 ★ ; Drift Tokyo 600 ★ ; **Néon nuit : seulement dans le cadeau du jour ou un succès** (valeur 800 ★,
  jamais en vente). Skin offerte : le joueur la choisit parmi les skins en vente quand il débloque la voiture (les 3 voitures gratuites : dès le départ).
- **Autocollants** : 10 gratuits ; packs « Numéros et lettres » 150 ★, « Bandes » 200 ★, « Flammes et éclairs » 250 ★, « Étoiles et formes » 300 ★.

## Découpage (un ticket ≤ 1 jour, une session par ticket)
| Ticket | Sujet | Zone de code | Agent | Dépend de |
|---|---|---|---|---|
| WFS-498 | Portefeuille d'étoiles, prix dans `configgame.json`, voitures à acheter, essai gratuit, banc d'équilibrage | E3 (+ `configgame.*`, exception PO) | `@dev` | — |
| WFS-499 | Garage : voitures bloquées, prix (★, médailles, €), « Acheter » avec confirmation, essai gratuit, FR / EN | E5 | `@dev` | WFS-498 |
| WFS-487 | Catalogue de personnalisation : 28 couleurs, prix, objets possédés, achat en ★, sauvegarde | E3 | `@dev` | WFS-498 |
| WFS-488 | Couleur secondaire : toit et rétroviseurs séparés sur les 17 voitures | E10 | `@dev` (artiste 3D) | WFS-487 |
| WFS-489 | Garage : 28 couleurs, couleur secondaire, cadenas, prix, achat | E5 (`src/ui/garage.*`) | `@dev` | WFS-487, WFS-488 |
| WFS-490 | Finitions : 8 peintures réelles dans le shader des voitures | E10 | `@dev` (artiste 3D) | WFS-489 |
| WFS-491 | Garage : choix de la finition, prix, aperçu | E5 | `@dev` | WFS-490 |
| WFS-492 | Skins : texture d'habillage par voiture + skin Rallye sur 6 voitures | E10 | `@dev` (artiste 3D) | WFS-491 |
| WFS-493 | Skins Drift Tokyo, Néon nuit et Camouflage sur 6 voitures | E10 (`assets/cars/**`) | `@dev` (artiste 3D) | WFS-492 |
| WFS-494 | Garage : skins, skin offerte, prix, skins du cadeau / des succès | E5 | `@dev` | WFS-493 |
| WFS-495 | Autocollants : pose qui épouse la carrosserie, 20 calques cuits en une texture | E10 | `@dev` (artiste 3D) | WFS-494 |
| WFS-496 | Atelier d'autocollants : bibliothèque, placer, tourner, agrandir, colorer, calques | E8 (`src/ui/ecrans/**`) | `@dev` | WFS-495 |
| WFS-497 | Atelier : texte libre filtré, packs en ★, « Mes créations » (5 par voiture) | E8 + E3 | `@dev` | WFS-496, WFS-480 |

## Langue (épic E20, ajout du PO, 2026-11-12 22:30 UTC, demande de l'utilisateur)
Tous les écrans de cette épic passent par le système de langue de **WFS-504** (`ui::tr`, tables `assets/textes/fr.txt` et `en.txt`) : ils **dépendent de WFS-504** et livrent leurs textes **en français et en anglais dans la même PR**, avec les captures dans les deux langues (`regles-de-gestion/E20-langue-anglaise.md`, RC-E20-01 à 07). Les tickets de données sans texte ne changent pas.
