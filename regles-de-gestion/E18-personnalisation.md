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
- RC-E18-07 **Étoiles** : les prix sont des **données** (fichier `assets/perso/catalogue.txt` créé par WFS-487), jamais écrits dans le code ;
  l'achat débite le portefeuille d'étoiles de l'étape « score » ; un objet non possédé s'essaie au garage (aperçu) mais ne part pas en course.
- RC-E18-08 **Réutiliser** : `PEINTURES` (`sauvegarde.h`) et `PAINTS` (`garage.cpp`) — une seule liste à la fin (WFS-487) ; `vernisPeinture`
  (`shaders/vehicule.glsl`) ; liste des mots interdits de WFS-480 (`assets/profil/mots_interdits.txt`).
- RC-E18-09 **Parcours complet** testé (menu → garage → choix → course → retour garage → relance du jeu : choix gardé), noté au journal avant la PR.
- RC-E18-10 Voitures : seulement des modèles libres (CC BY / CC0). **Exclues** : A 45, Elantra N, X5 (licence NC, ordre permanent du 2026-11-10).

## Choix du PO
- **3 voitures en plus pour les skins** : **Golf** (voiture de départ du jeu), **Corvette C7** (id `mustang`), **Giulia Quadrifoglio** :
  trois silhouettes différentes (compacte, coupé, berline), modèles CC BY 4.0. Avec 208, 500 et GT86 : 6 voitures.
- **Prix des finitions** : brillant 0 ; satiné 100 ; mat 150 ; métallisé 150 ; nacré 250 ; candy 300 ; chromé 400 ; caméléon 400 ★.
- **Skins** : Rallye 300 ★ ; Camouflage 450 ★ ; Drift Tokyo 600 ★ ; **Néon nuit : seulement dans le cadeau du jour ou un succès** (valeur 800 ★,
  jamais en vente). Skin offerte : le joueur la choisit parmi les skins en vente quand il débloque la voiture (les 3 voitures gratuites : dès le départ).
- **Autocollants** : 10 gratuits ; packs « Numéros et lettres » 150 ★, « Bandes » 200 ★, « Flammes et éclairs » 250 ★, « Étoiles et formes » 300 ★.

## Découpage (un ticket ≤ 1 jour, une session par ticket)
| Ticket | Sujet | Zone de code | Agent | Dépend de |
|---|---|---|---|---|
| WFS-487 | Catalogue de personnalisation : 28 couleurs, prix, objets possédés, achat en ★, sauvegarde | E3 (+ `assets/perso/**`) | `@dev` | étape « score » (étoiles) |
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
