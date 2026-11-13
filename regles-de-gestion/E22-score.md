# Épic E22 — Score de chaque course, converti en étoiles

Créée le 2026-11-13 par le PO, sur demande de l'utilisateur (« crée l'épic E22 score avec ta proposition »). Étape **« score »** de l'ordre de l'utilisateur :
**identité (E17) → score (E22) → voitures et couleurs (E18) → cadeau (E19) → market (E21)**, anglais (E20) en parallèle. Monnaie : étoiles ★ (portefeuille de WFS-498).

## Demande (proposition du PO acceptée par l'utilisateur)
- Chaque course **terminée** donne un **score** fait de : la **place** d'arrivée (ou victoire / défaite), le **temps** contre le record, la **conduite propre** (peu ou pas de chocs) et **2 défis par mode**.
- Le score est **converti en étoiles : 5 à 40 ★ par course terminée**, au lieu des 10 ★ fixes posés par WFS-498 (`boutique.gain_course`). Course quittée : 0 ★.
- Le détail du score et les étoiles gagnées s'affichent **en fin de course**, en français et en anglais, dans les 4 modes.
- La cadence d'achat reste celle de WFS-498 : le **banc d'équilibrage** (1re voiture en 22 à 28 courses, tout le jeu en 108 à 132 h) reste vert pour un joueur moyen ; un bon joueur avance plus vite (jusqu'à 40 ★ par course).

## Règles communes (RC-E22)
- **RC-E22-01** Toutes les valeurs (points, barèmes, défis, conversion, bornes 5 et 40) dans `configgame.json`, section **`score_etoiles`** (+ `configgame.README.md`), lues par `src/core/configgame.*` ; aucun chiffre en dur.
- **RC-E22-02** Le calcul est une **fonction pure** (`src/game/score_course.{h,cpp}`) : entrée = résultat de la course (mode, terminée / quittée, place, victoire, temps, record, chocs, compteurs des défis), sortie = lignes du score + total + étoiles. Testée en CPU, sans rendu.
- **RC-E22-03** Le score existant de chaque mode (points de distance de la course solo, score de la Poursuite, records de la sauvegarde) **n'est pas cassé** : il reste affiché et gardé ; E22 ajoute le score « étoiles ».
- **RC-E22-04** **Duel en ligne** : mêmes règles que la course à 2 ; aucun gain si l'adversaire quitte avant 30 s (pas de partie arrangée) ; la valeur reçue du réseau n'est jamais crue : le score se calcule sur le téléphone du joueur.
- **RC-E22-05** Textes par `ui::tr`, français et anglais ; captures dans les deux langues ; style `design-street-racing` pour l'écran.
- **RC-E22-06** Aucun achat, aucune pub dans E22 (×2 par vidéo : WFS-514, épic E21).

## Tickets
| Ticket | Titre | Zone | Agent | Dépend de |
|---|---|---|---|---|
| WFS-535 | Calcul du score par course et conversion en étoiles (4 modes, défis, tests) | E3 (`src/game/score_course.*`, `tests/**`) + section `score_etoiles` de `configgame.*` | `@dev` | WFS-498 (fusionné) |
| WFS-536 | Fin de course : les étoiles du score remplacent les 10 ★ fixes ; banc d'équilibrage adapté | E3 (`src/game/profil_system.cpp`, `boutique.*`, banc `equilibrage`) | `@dev` | WFS-535 |
| WFS-537 | Écran de fin de course : détail du score et « +N ★ », FR / EN, 4 modes | E5 / E8 (écran de fin des 4 modes ; accroches annoncées) | `@dev` | WFS-535 |
| WFS-538 | Records de score par mode et par carte, « Nouveau record », vus dans l'écran Profil | E3 (`sauvegarde.*` : clés seulement) + E8 (écran Profil) | `@dev` | WFS-535, WFS-482 |

Après E22 : **WFS-486** (e-mail contre 100 ★) et **WFS-503** (gains réels du cadeau) ne l'attendent plus que pour le portefeuille, déjà là (WFS-498).
