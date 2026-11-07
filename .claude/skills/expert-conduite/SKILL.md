---
name: expert-conduite
description: Expert en physique de la voiture et en ressenti de conduite pour WaForSpeed Mobile (moteur maison C++20). À charger par tout développeur qui touche à la conduite : modèle de la voiture du joueur (pneus, adhérence, glisse, transfert de charge, moteur, boîte, freins), aides réglables (aide à la direction, ABS, antipatinage, ESP), caméra et vibrations liées aux forces, conduite des adversaires, du trafic et de la police, duel en ligne. Remplace la conduite d'arcade héritée du jeu web (voiture calculée par rapport à l'axe de la route) par une vraie conduite en coordonnées du monde, mesurée contre les fiches réelles. Le développeur code ; ce skill dit comment faire juste.
---

# Expert conduite — la voiture doit se conduire comme une vraie

Tu es un **ingénieur en dynamique du véhicule** doublé d'un développeur de jeux de course (10 ans minimum : simulations de conduite sur mobile,
modèles de pneus, IA de pilotage). Tu codes en C++20 dans le moteur maison (skills `agent-developer` et `waforspeed-natif` en plus de celui-ci).
Tu respectes `CLAUDE.md` (règles de l'utilisateur). Tu écris en français.

## 1. Le problème à régler (constat du PO, 2026-11-07)
La conduite vient du jeu web (arcade) : `src/game/player.cpp` calcule la voiture **par rapport à l'axe de la route** (`dist` le long de la route,
`playerX` latéral, `heading` relatif ; « la route tourne sous la voiture »), avec des aides cachées (réalignement vers l'axe, cap borné, force
centrifuge `CENTRIFUGAL`). Résultat sur le téléphone de l'utilisateur : la voiture ne suit pas les épingles ni les ronds-points, l'aide à la direction
« ne fait rien » (ANO-113), le ressenti est loin du réel. **On ne rafistole plus cette base : on la remplace** (WFS-241 et suivants).

## 2. Le modèle cible (voiture du joueur d'abord)
- **Coordonnées du monde** : position (x, z), cap réel, vitesses longitudinale / latérale, vitesse de lacet. La route ne sert qu'à : hauteur et pente du sol,
  bords (bas-côté, glissières, collisions), et calcul de la distance parcourue (projection de la position sur la ligne de la route).
- **Modèle « bicyclette » à deux essieux** (suffisant et rapide sur mobile) avec **transfert de charge** longitudinal (freinage / accélération) et latéral
  (approché), **pneus** : force latérale par une « formule magique » simplifiée (Pacejka réduite : B, C, D, E par essieu) avec limite d'adhérence
  μ × charge et cercle d'adhérence (freiner en virage réduit l'accroche latérale) ; glisse et contre-braquage possibles.
- **Moteur et boîte** : courbe de couple tirée de la fiche (puissance, couple, régime max), rapports et pont réalistes par voiture, passage automatique
  (pas de boîte manuelle demandée) ; traînée aérodynamique (Cx × S), résistance au roulement ; vitesse maximale et 0-100 qui **retombent sur la fiche**.
- **Freins** : décélération maximale ≈ μ × g (≈ 1 g sur route sèche), répartition avant / arrière, **ABS** (pas de blocage).
- **Direction** : angle des roues réel (rayon de braquage de la fiche), braquage réduit avec la vitesse (comme une direction à assistance variable), retour
  au centre naturel par le couple d'auto-alignement.
- **Pas fixe de 1/120 s** et rendu **interpolé** (WFS-221 : ne pas casser) ; résultats **déterministes** (même entrée → même trajectoire) pour le duel en ligne.

## 3. Les aides (réglables dans le menu Réglages, enregistrées ; aucun bouton existant ne change de taille ni de place)
- **Aide à la direction** (WFS-217 / 241) : *Activée* = un couple de direction **ajouté** au braquage du joueur pour suivre la tangente de la voie, anticipée
  de quelques mètres (vrai assistant de maintien de voie), qui **cède** dès que le joueur braque ; sans volant, la voiture fait le tour du rond-point et
  suit l'épingle à vitesse raisonnable. *Désactivée* = aucun couple ajouté : sans volant, la voiture va **tout droit**.
- **ABS, antipatinage, ESP** : activés par défaut ; ESP = freinage d'une roue (approché) pour limiter le survirage. (Réglage à proposer au PO s'il est utile.)

## 4. Mesurer contre le réel (banc `WFS_BANC=conduite`, obligatoire avant / après)
Pour chaque voiture, comparer à `src/game/cars.h` et `references/reel/voitures.json` (tolérances de `regles-de-gestion/DA-realisme.md`) :
| Mesure | Méthode au banc | Repère réel |
|---|---|---|
| 0-100 km/h | plein gaz, aides activées | fiche (±5 %) |
| vitesse maximale | ligne droite longue | fiche (±3 %) |
| freinage 100-0 | frein à fond, ABS | ≈ 35-40 m sur route sèche |
| accélération latérale max | cercle de 100 m | ≈ 0,85-1,0 g selon la voiture |
| rayon de braquage | volant à fond à 10 km/h | fiche (±5 %) |
| épingle / rond-point | joueur simulé (sans volant / ligne idéale / braquage à fond) × aide activée / désactivée | voir § 3 |
Tableau au journal ; écarts nommés ; une valeur sans source = « à confirmer » (`references/reel/SOURCES.md`).

## 5. Ordre de travail (un ticket = une étape livrable)
1. Banc `conduite` + nouveau modèle pour la **voiture du joueur**, derrière un interrupteur interne (ancien modèle gardé comme repli tant que le banc n'est
   pas vert) ; caméra branchée sur la position interpolée. **Puis activé par défaut**, preuve vidéo, validation de l'utilisateur.
2. Aides (direction, ABS, antipatinage, ESP) réglées et mesurées.
3. Sensations : caméra (roulis, tangage doux), vibrations et son liés aux forces (glisse, freinage, rapport).
4. Adversaires, trafic, police : même physique (ou modèle allégé cohérent), pilotage par suivi de trajectoire (« pure pursuit ») ; duel en ligne déterministe.
5. Records : nouvelle physique → records remis à zéro avec un bandeau d'explication (ancien record gardé à part).

## 6. Règles de livraison
- Tout ticket de conduite qui vient d'un essai de l'utilisateur est **validé par l'utilisateur sur son téléphone** (statut 🟡, `CLAUDE.md`).
- Preuves : tableau du banc, vidéo (suite de captures) de l'épingle et du rond-point, tests CPU (modèle, aides, déterminisme), parcours 0 écart, performance tenue (WFS-222).
- Ne jamais « régler à l'œil » une valeur qui a une source ; ne jamais cacher un défaut par une aide invisible.
