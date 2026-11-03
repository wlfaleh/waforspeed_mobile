# QA WFS-47 : notifications de course (checkpoint, quasi-contact, accident)

**Verdict : VALIDÉ** (banc d'essai PC), 1er passage, aucune anomalie. **Fusion autorisée.**

- Construction regardée : commit `c813a56` (`feature/waforspeed-47`, PR #25), CI au vert (`banc-essai`, `android`, `publication`).
  Release `apercu-waforspeed-47` (« commit c813a56 »), 55 captures et `WaForSpeed.apk`.
- La branche contient `main` après WFS-42 (`bfa7c76`) et se fusionne sur `main` (`d068040`) sans conflit (`git merge-tree`).
- Règles : `regles-de-gestion/WFS-47.md` (RG-01 à RG-07). Référence web : `js/ui/toasts.js`, `css/style.css` (.toasts/.toast), `js/game/update.js`.
- Code changé : `src/ui/toasts.{h,cpp}` (nouveau système, ordre 425) et une ligne d'accroche dans `benchHud` (`src/ui/hud.cpp`).
  `src/game/` n'est pas modifié (RG-01).

## Captures

| Capture | Constat |
|---|---|
| `toast-checkpoint` (nouvelle) | Un bandeau « CHECKPOINT 1 » « +11,0 s » à droite, sous le panneau du score (haut du bandeau à y ≈ 124, bas du score à y ≈ 83). Bordure gauche et valeur orange, fond vitré sombre, titre en capitales espacées, valeur plus petite sur la même ligne de base. Le HUD affiche ensuite « CHECKPOINT 2 » (prochain objectif), ce qui est cohérent. Golf visible, image non déformée. |
| `toast-pile` (nouvelle) | 3 bandeaux alignés à droite, du haut vers le bas : « QUASI-CONTACT » « +250 » (cyan), « CHECKPOINT 2 » « +10,6 s » (orange ; 11 − 0,4 = 10,6, correct), « ACCIDENT ! » (rouge, sans valeur). Le plus récent est en haut, l'espacement est régulier. Bord droit de la pile à x ≈ 1093, bord gauche du bouton Nitro à x ≈ 1105 : **rien ne recouvre une commande** (volant, frein, gaz et nitro dégagés). |
| `hud-combo` (changée, voulu) | Mêmes HUD, voiture et décor que sur `main`, plus deux bandeaux cyan « QUASI-CONTACT ×2 » « +500 » (en haut) et « QUASI-CONTACT » « +250 ». Cohérent avec le « Combo ×2 » et le score 1505 du HUD. Les bandeaux passent sous le panneau du score agrandi par le combo, sans le toucher. |
| Les 52 autres captures | Comparaison pixel par pixel avec `apercu-waforspeed-42` (commit `1081487`, code identique à `main` après WFS-42 ; le seul code en plus ici est `src/ui/toasts.*` et l'accroche) : mêmes tailles, 10 captures identiques au pixel, les autres avec un écart **maximal de 16/255** sur quelques pixels (bruit de rendu d'un lancement à l'autre), invisible. Aucun bandeau ailleurs (`course-fin`, `encoche-course`, `hud-2400`, menus, vitrines). |
| Tests du banc | `mode exact : 39 voitures… 0 écart(s)`, `plage`, `campagne`, `circuit` : 0 écart(s), `course automatique : 22 lignes… 0 écart(s)`. Journal : `capture toast-checkpoint : 1 bandeau(x) « CHECKPOINT 1 » « +11,0 s »`, `capture toast-pile : 3 bandeau(x)` avec les trois textes attendus. |

## Critères d'acceptation et règles

| Critère / règle | Constat | Résultat |
|---|---|---|
| `toast-checkpoint` : « CHECKPOINT 1 » « +11,0 s » à droite, sous le score | voir Captures | OK |
| `toast-pile` : 3 bandeaux (cyan, orange, rouge), aucun sur les commandes | voir Captures | OK |
| Banc `0 écart(s)`, autres captures inchangées | `hud-combo` changé comme prévu (choix 2), le reste identique au bruit près | OK |
| RG-02 checkpoint, `checkpointBonus(N)`, 1 décimale, virgule, orange | « +11,0 s », « +10,6 s » | OK |
| RG-03 quasi-contact, « ×N » si combo > 1, « +250×combo », cyan | `hud-combo`, `toast-pile` | OK |
| RG-04 accident, combo perdu, rouge | « ACCIDENT ! » vu ; « COMBO PERDU » seulement sur un choc (choix 3), comme `update.js` l. 46 (le délai écoulé, l. 241, remet le combo à 0 sans bandeau) | OK |
| RG-05 pile à droite sous le score, 3 au plus, le plus récent en haut, au-dessus du Nitro sans le recouvrir | voir Captures et choix 1 | OK |
| RG-06 fond vitré, bordure gauche 4 px de la couleur du genre, titre 18 px capitales, valeur 14 px, droit (WFS-48 pas livré) | conforme sur les captures | OK |

## Choix du développeur

1. **Bord droit de la pile à gauche du bouton Nitro** (et non contre la gouttière droite comme sur le web) : accepté. RG-05 demande
   explicitement de ne pas recouvrir le Nitro ; à 720 p, la pile alignée sur la gouttière l'aurait couvert. Conséquence visible :
   le bord droit des bandeaux n'est pas aligné sur celui du panneau du score. Ce n'est pas une anomalie, la règle l'impose.
   À revoir éventuellement avec WFS-52 (marges de sécurité) si le PO veut un autre placement.
2. **`hud-combo` avec deux bandeaux** : accepté, c'est le comportement du jeu web (un bandeau par quasi-contact).
3. **« COMBO PERDU » sur un choc seulement** : accepté, identique au jeu web.

## Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — | — | — | aucune |
