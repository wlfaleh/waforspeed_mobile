# Épic E19 — Cadeau du jour aléatoire

Créée le 2026-11-12 par le PO, sur demande de l'utilisateur (`@po`, 2026-11-12 22:24 UTC). Étape **« cadeau »** de l'ordre de l'utilisateur :
**identité (E17) → score (étoiles) → voitures et couleurs (E18) → cadeau (E19) → market**. Monnaie : étoiles ★ (R13 : une seule monnaie).

## Demande de l'utilisateur (texte fidèle, résumé)
- **Règle** : 1 cadeau par jour (minuit, heure du téléphone), ouvert depuis le menu. Rien ne se perd si le joueur ne joue pas un jour.
  Le cadeau est **gratuit** : il ne s'achète jamais et ne se rouvre jamais contre de l'argent, une pub ou des étoiles.
- **Tirage** (valeurs dans `configgame.json`) : 10-15 ★ 45 % ; 16-25 ★ 28 % ; 26-40 ★ 9 % ; 41-50 ★ 3 % ; coloris (couleur ou finition) pas encore
  possédé, pour une voiture du joueur 8 % ; skin pas encore possédée, pour une voiture du joueur 4 % ; voiture que le joueur n'a pas, **prêtée 3 heures** 3 %.
  Minimum 10 ★ par cadeau. **Garantie** : si en 30 cadeaux le joueur n'a eu ni voiture ni skin, le 30e en donne une. Catégorie déjà toute possédée → étoiles (10 ★ au moins).
- **Voiture prêtée 3 h** : 24 h pour appuyer sur « Activer », puis compte à rebours de 3 h visible au garage et au menu ; utilisable dans tous les modes,
  étoiles gagnées normalement ; à la fin : « Garde-la pour toujours : <prix> ★ » avec un bouton vers le garage ou le market.
- **Écran** : animation d'ouverture du cadeau, puis le gain ; bouton « Chances » qui affiche le tableau des probabilités.
- **Protection** : si l'heure du téléphone recule, pas de nouveau cadeau avant d'avoir repassé l'heure du dernier cadeau.
- **Sauvegarde** : date du dernier cadeau, compteur de la garantie, voiture prêtée et fin du prêt.
- **Critères** : test CPU de 100 000 tirages (chances à ±0,5 %, jamais < 10 ★, garantie au 30e) ; captures : ouverture (étoiles, coloris, skin, voiture),
  compte à rebours, fin du prêt, en français et en anglais.

## Règles communes à toute l'épic (à rappeler dans chaque PR)
- RC-E19-01 **Gratuit pour toujours** : aucun bouton, aucune offre, aucun écran ne permet d'acheter, de doubler, de relancer ou de rouvrir un cadeau
  (ni argent, ni pub, ni étoiles). Le lead dev refuse toute PR qui en ajoute un.
- RC-E19-02 **Valeurs en données** : chances, tranches d'étoiles, minimum, garantie (30), durée du prêt (3 h), délai d'activation (24 h) dans la section
  `cadeau` de `configgame.json` (+ une ligne par clé dans `configgame.README.md`) ; rien d'écrit en dur dans le code.
- RC-E19-03 **Sauvegarde** : tout dans `src/game/sauvegarde.*` (format `clé=valeur`), une sauvegarde d'avant se lit sans erreur, aucune clé ancienne supprimée.
- RC-E19-04 **Heure** : le jour change à **minuit, heure locale du téléphone**. Les heures sont gardées en secondes UTC (epoch) **et** le décalage local du moment.
- RC-E19-05 **Interface 2D** (menu, écran du cadeau, garage) : `design-street-racing`, français **et** anglais (même mécanisme de langue que WFS-481).
  Toute vue 3D d'une voiture (prêtée, coloris, skin) suit `DA-realisme.md` et la règle « pas de forme nue ».
- RC-E19-06 **Aucune perte d'images/s** : rien de l'écran du cadeau n'est chargé en course ; banc d'endurance inchangé (écart ≤ 1 img/s en qualité Basse).
- RC-E19-07 **Parcours complet** testé et noté au journal avant la PR : menu → cadeau → gain → garage / course → retour menu → relance du jeu (état gardé).

## Choix du PO (l'utilisateur peut les changer)
- **« Rien ne se perd »** : un jour sans jouer ne fait rien perdre (compteur de garantie, voiture prêtée non activée dans ses 24 h, étoiles) ;
  il n'y a **pas de série** de jours à garder. Les cadeaux **ne s'accumulent pas** : un seul cadeau disponible à la fois (« 1 cadeau par jour »).
- **Étoiles dans une tranche** : tirage uniforme entier (10-15 = 6 valeurs, chacune 1/6).
- **Repli d'une catégorie toute possédée** : tirage parmi les 4 tranches d'étoiles avec leurs poids (45 / 28 / 9 / 3, renormalisés).
- **Garantie** : compteur des cadeaux ouverts **sans voiture ni skin** ; remis à 0 dès qu'une voiture prêtée ou une skin sort. Au 30e : tirage entre
  skin (poids 4) et voiture (poids 3) ; si l'une est toute possédée, l'autre ; si les deux, étoiles et compteur remis à 0.
- **Voiture prêtée** : jamais une voiture déjà possédée ni déjà prêtée ; voitures sous licence NC exclues (ordre du 2026-11-10). Le prix affiché à la fin
  est celui de `boutique.voitures` (WFS-498, R16). Le prêt est distinct de l'**essai gratuit** d'une course (WFS-498 RG-08).

## Découpage (un ticket ≤ 1 jour, une session par ticket)
| Ticket | Sujet | Zone de code | Agent | Dépend de |
|---|---|---|---|---|
| WFS-500 | Tirage du cadeau : chances en données, garantie, replis, jour et protection de l'heure, sauvegarde, test CPU 100 000 tirages | E3 (`src/game/cadeau.*`, `tests/**`) + `configgame.*` (section `cadeau`) | `@dev` | WFS-498 |
| WFS-501 | Écran du cadeau : bouton du menu, animation d'ouverture, gain, bouton « Chances », FR / EN | E8 (`src/ui/ecrans/**`) | `@dev` | WFS-500 |
| WFS-502 | Voiture prêtée 3 h : « Activer » sous 24 h, compte à rebours au garage et au menu, tous les modes, fin « Garde-la pour toujours » | E3 + E5 (`src/ui/garage.*`) + E8 | `@dev` | WFS-501 ; WFS-498 (voitures possédées, prix), WFS-499 (garage) |
| WFS-503 | Gains réels : étoiles au portefeuille, coloris et skins donnés pour de vrai | E3 | `@dev` | WFS-501 ; WFS-498 (portefeuille) ; WFS-487, WFS-494 |
